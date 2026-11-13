// The IPU (image processing unit) on the PC, as far as the game uses it.
//
// The game decompresses its IPU-compressed textures (the terrain's, e.g.
// Hawaii.tri) one mip level at a time (PSXTextureMapManager::IPUDecompress):
// a source chain on DMA ch4 (toIPU) carries the bitstream, then
// BCLR, FDEC 0 (read the header byte), FDEC 8 (skip it), IPU_CTRL = the
// header's IDP/AS/IVF/QST/MP1 bits, IDEC (RGB16 out, dithered, QSC 1, DTD
// from the header) and a normal-mode DMA ch3 (fromIPU) of 512 bytes per
// macroblock into the IPU cache, which PlaceDecompressedTextureIntoCache
// rearranges into a linear texture.
//
// The IPU registers are plain memory here (ports/pc/hw.cpp maps the window), so
// a command write isn't seen as it happens: a command runs when the game
// next calls sceIpuSync or starts ch3, if IPU_CMD holds a value other than
// the one the IPU last left there (FDEC's result). ch4 runs to completion
// first, so the whole bitstream is in the "FIFO" when a command runs; a ch4
// transfer starts a new bitstream (the game always BCLRs before one).
//
// Commands: BCLR, FDEC, IDEC (RGB32/RGB16 output, dithering, the threshold
// alpha), SETIQ/SETVQ/SETTH from the FIFO. BDEC, VDEC, CSC and PACK (MPEG
// movie playback through libmpeg) are logged once: ports/pc/sdk/mpeg.cpp
// replaces libmpeg and decodes movies on the host (libavcodec); while it
// waits for a ch4 transfer, the data goes to it (port_ipu_set_movie_sink).
#include "port.h"
#include "port_simd.h"
#include "port_ptr32.h"

#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>

namespace {

volatile uint32_t &Reg(uintptr_t a)
{
    return *(volatile uint32_t *)PORT_HW(a);
}

const uintptr_t kCmd = 0x10002000, kCmdHi = 0x10002004, kCtrl = 0x10002010, kBp = 0x10002020;
const uintptr_t kD4Chcr = 0x1000B400;

std::mutex g_lock;
std::vector<uint8_t> g_in;      // the bitstream ch4 delivered
size_t g_bit;                   // read position in bits
std::vector<uint8_t> g_out;     // output waiting for ch3
size_t g_outPos;
uint32_t g_lastCmdValue = 0xFFFFFFFF; // what the IPU left in IPU_CMD
uint8_t g_iq[64], g_niq[64];    // in zigzag order (SETIQ's)
uint16_t g_vqclut[16];
uint16_t g_th0, g_th1;
bool g_logIpu = getenv("PORT_IPU_LOG") != nullptr;

// ---- bit reader
uint32_t Peek(int n)
{
    uint32_t v = 0;
    size_t b = g_bit;
    for (int i = 0; i < n; i++, b++) {
        size_t byte = b >> 3;
        int bit = byte < g_in.size() ? (g_in[byte] >> (7 - (b & 7))) & 1 : 0;
        v = v << 1 | bit;
    }
    return v;
}

uint32_t Get(int n)
{
    uint32_t v = Peek(n);
    g_bit += n;
    return v;
}

bool Overrun()
{
    return g_bit > g_in.size() * 8;
}

// ---- VLC tables (MPEG-2 B-12/B-13 DC sizes, B-14/B-15 DCT coefficients)
struct Vlc {
    uint8_t len;    // 0: invalid
    int8_t run;     // -1 EOB, -2 escape
    uint8_t level;
};

struct VlcCode {
    const char *bits;
    int run, level;
};

// B-14 (intra_vlc_format 0), AC coefficients (the first-coefficient "1s"
// form is for non-intra blocks only).
const VlcCode kB14[] = {
    {"10", -1, 0}, {"000001", -2, 0},
    {"11", 0, 1}, {"011", 1, 1}, {"0100", 0, 2}, {"0101", 2, 1}, {"00101", 0, 3}, {"00111", 3, 1},
    {"00110", 4, 1}, {"000110", 1, 2}, {"000111", 5, 1}, {"000101", 6, 1}, {"000100", 7, 1},
    {"0000110", 0, 4}, {"0000100", 2, 2}, {"0000111", 8, 1}, {"0000101", 9, 1},
    {"00100110", 0, 5}, {"00100001", 0, 6}, {"00100101", 1, 3}, {"00100100", 3, 2},
    {"00100111", 10, 1}, {"00100011", 11, 1}, {"00100010", 12, 1}, {"00100000", 13, 1},
    {"0000001010", 0, 7}, {"0000001100", 1, 4}, {"0000001011", 2, 3}, {"0000001111", 4, 2},
    {"0000001001", 5, 2}, {"0000001110", 14, 1}, {"0000001101", 15, 1}, {"0000001000", 16, 1},
};

// B-15 (intra_vlc_format 1): its own short codes.
const VlcCode kB15[] = {
    {"0110", -1, 0}, {"000001", -2, 0},
    {"10", 0, 1}, {"010", 1, 1}, {"110", 0, 2}, {"00101", 2, 1}, {"0111", 0, 3}, {"00111", 3, 1},
    {"000110", 4, 1}, {"00110", 1, 2}, {"000111", 5, 1}, {"0000110", 6, 1}, {"0000100", 7, 1},
    {"11100", 0, 4}, {"0000111", 2, 2}, {"0000101", 8, 1}, {"1111000", 9, 1},
    {"11101", 0, 5}, {"000101", 0, 6}, {"1111001", 1, 3}, {"00100110", 3, 2},
    {"1111010", 10, 1}, {"00100001", 11, 1}, {"00100101", 12, 1}, {"00100100", 13, 1},
    {"000100", 0, 7}, {"00100111", 1, 4}, {"11111100", 2, 3}, {"11111101", 4, 2},
    {"000000100", 5, 2}, {"000000101", 14, 1}, {"000000111", 15, 1}, {"0000001101", 16, 1},
    {"1111011", 0, 8}, {"1111100", 0, 9}, {"00100011", 0, 10}, {"00100010", 0, 11},
    {"00100000", 1, 5}, {"0000001100", 2, 4},
    {"11111010", 0, 12}, {"11111011", 0, 13}, {"11111110", 0, 14}, {"11111111", 0, 15},
};

// The long codes both tables share (12 to 16 bits).
const VlcCode kLong[] = {
    {"000000011101", 0, 8}, {"000000011000", 0, 9}, {"000000010011", 0, 10}, {"000000010000", 0, 11},
    {"000000011011", 1, 5}, {"000000010100", 2, 4}, {"000000011100", 3, 3}, {"000000010010", 4, 3},
    {"000000011110", 6, 2}, {"000000010101", 7, 2}, {"000000010001", 8, 2}, {"000000011111", 17, 1},
    {"000000011010", 18, 1}, {"000000011001", 19, 1}, {"000000010111", 20, 1}, {"000000010110", 21, 1},
    {"0000000011010", 0, 12}, {"0000000011001", 0, 13}, {"0000000011000", 0, 14}, {"0000000010111", 0, 15},
    {"0000000010110", 1, 6}, {"0000000010101", 1, 7}, {"0000000010100", 2, 5}, {"0000000010011", 3, 4},
    {"0000000010010", 5, 3}, {"0000000010001", 9, 2}, {"0000000010000", 10, 2}, {"0000000011111", 22, 1},
    {"0000000011110", 23, 1}, {"0000000011101", 24, 1}, {"0000000011100", 25, 1}, {"0000000011011", 26, 1},
    {"00000000011111", 0, 16}, {"00000000011110", 0, 17}, {"00000000011101", 0, 18},
    {"00000000011100", 0, 19}, {"00000000011011", 0, 20}, {"00000000011010", 0, 21},
    {"00000000011001", 0, 22}, {"00000000011000", 0, 23}, {"00000000010111", 0, 24},
    {"00000000010110", 0, 25}, {"00000000010101", 0, 26}, {"00000000010100", 0, 27},
    {"00000000010011", 0, 28}, {"00000000010010", 0, 29}, {"00000000010001", 0, 30},
    {"00000000010000", 0, 31},
    {"000000000011000", 0, 32}, {"000000000010111", 0, 33}, {"000000000010110", 0, 34},
    {"000000000010101", 0, 35}, {"000000000010100", 0, 36}, {"000000000010011", 0, 37},
    {"000000000010010", 0, 38}, {"000000000010001", 0, 39}, {"000000000010000", 0, 40},
    {"000000000011111", 1, 8}, {"000000000011110", 1, 9}, {"000000000011101", 1, 10},
    {"000000000011100", 1, 11}, {"000000000011011", 1, 12}, {"000000000011010", 1, 13},
    {"000000000011001", 1, 14},
    {"0000000000010011", 1, 15}, {"0000000000010010", 1, 16}, {"0000000000010001", 1, 17},
    {"0000000000010000", 1, 18}, {"0000000000010100", 6, 3}, {"0000000000011010", 11, 2},
    {"0000000000011001", 12, 2}, {"0000000000011000", 13, 2}, {"0000000000010111", 14, 2},
    {"0000000000010110", 15, 2}, {"0000000000010101", 16, 2}, {"0000000000011111", 27, 1},
    {"0000000000011110", 28, 1}, {"0000000000011101", 29, 1}, {"0000000000011100", 30, 1},
    {"0000000000011011", 31, 1},
};

const char *kDcLuma[12] = {"100", "00", "01", "101", "110", "1110", "11110", "111110",
                           "1111110", "11111110", "111111110", "111111111"};
const char *kDcChroma[12] = {"00", "01", "10", "110", "1110", "11110", "111110", "1111110",
                             "11111110", "111111110", "1111111110", "1111111111"};

Vlc g_b14[1 << 16], g_b15[1 << 16];
uint8_t g_dcLen[2][1 << 10], g_dcSize[2][1 << 10];

void Fill(Vlc *t, const VlcCode &c)
{
    int len = (int)strlen(c.bits);
    uint32_t code = 0;
    for (int i = 0; i < len; i++)
        code = code << 1 | (c.bits[i] - '0');
    uint32_t first = code << (16 - len), count = 1u << (16 - len);
    for (uint32_t i = 0; i < count; i++) {
        if (t[first + i].len)
            port_fatal("ipu: VLC %s overlaps", c.bits);
        t[first + i] = {(uint8_t)len, (int8_t)c.run, (uint8_t)c.level};
    }
}

void BuildTables()
{
    for (const VlcCode &c : kB14)
        Fill(g_b14, c);
    for (const VlcCode &c : kB15)
        Fill(g_b15, c);
    for (const VlcCode &c : kLong) {
        Fill(g_b14, c);
        Fill(g_b15, c);
    }
    for (int k = 0; k < 2; k++) {
        const char **t = k ? kDcChroma : kDcLuma;
        for (int s = 0; s < 12; s++) {
            int len = (int)strlen(t[s]);
            uint32_t code = 0;
            for (int i = 0; i < len; i++)
                code = code << 1 | (t[s][i] - '0');
            for (uint32_t i = 0; i < 1u << (10 - len); i++) {
                g_dcLen[k][(code << (10 - len)) + i] = len;
                g_dcSize[k][(code << (10 - len)) + i] = s;
            }
        }
    }
}

const uint8_t kZigzag[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};
uint8_t g_zigzagIndex[64]; // raster -> zigzag position
const uint8_t kAlternate[64] = {
    0, 8, 16, 24, 1, 9, 2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18, 3, 11, 4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28, 5, 13, 6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30, 7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63,
};
const uint8_t kNonLinearQ[32] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22,
    24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 104, 112,
};

double g_cos[8][8]; // C(u)/2 * cos((2x+1)u pi/16)

void BuildIdct()
{
    for (int x = 0; x < 8; x++)
        for (int u = 0; u < 8; u++)
            g_cos[x][u] = (u ? 0.5 : 0.5 / std::sqrt(2.0)) * std::cos((2 * x + 1) * u * M_PI / 16);
}

void Idct(const int *in, int *out)
{
    double tmp[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            double s = 0;
            for (int u = 0; u < 8; u++)
                s += g_cos[x][u] * in[y * 8 + u];
            tmp[y * 8 + x] = s;
        }
    for (int x = 0; x < 8; x++)
        for (int y = 0; y < 8; y++) {
            double s = 0;
            for (int v = 0; v < 8; v++)
                s += g_cos[y][v] * tmp[v * 8 + x];
            out[y * 8 + x] = (int)std::floor(s + 0.5);
        }
}

struct Decoder {
    int idp, as, ivf, qst, mp1;
    int qscale;
    int dcPred[3];
    bool error;
};

int DcDiff(int chroma)
{
    uint32_t p = Peek(10);
    int len = g_dcLen[chroma][p], size = g_dcSize[chroma][p];
    g_bit += len;
    if (!size)
        return 0;
    int v = (int)Get(size);
    if (!(v >> (size - 1)))
        v -= (1 << size) - 1;
    return v;
}

// One intra block into raster-order coefficients, dequantized.
void IntraBlock(Decoder &d, int cc, int *blk)
{
    memset(blk, 0, 64 * sizeof(int));
    d.dcPred[cc] += DcDiff(cc != 0);
    blk[0] = d.dcPred[cc] << (3 - d.idp);
    if (d.mp1)
        blk[0] = d.dcPred[cc] << 3;
    int sum = blk[0];
    const uint8_t *scan = d.as ? kAlternate : kZigzag;
    const Vlc *table = (d.ivf && !d.mp1) ? g_b15 : g_b14;
    int i = 0;
    for (;;) {
        const Vlc &v = table[Peek(16)];
        if (!v.len) {
            d.error = true;
            return;
        }
        g_bit += v.len;
        int run, level;
        if (v.run == -1)
            break;
        if (v.run == -2) {
            run = (int)Get(6);
            if (d.mp1) {
                level = (int)Get(8);
                if (level == 0)
                    level = (int)Get(8);
                else if (level == 128)
                    level = (int)Get(8) - 256;
                else if (level > 128)
                    level -= 256;
            } else {
                level = (int)Get(12);
                if (level & 0x800)
                    level -= 0x1000;
            }
        } else {
            run = v.run;
            level = Get(1) ? -v.level : v.level;
        }
        i += run + 1;
        if (i >= 64) {
            d.error = true;
            return;
        }
        int j = scan[i];
        // the matrix is held in zigzag order: raster j's weight
        int w = g_iq[g_zigzagIndex[j]];
        int val = (level * d.qscale * w) / 16;
        if (d.mp1) {
            if (val)
                val = (val - 1) | 1;
            if (level < 0 && val > 0)
                val = -val;
        }
        if (val > 2047)
            val = 2047;
        else if (val < -2048)
            val = -2048;
        blk[j] = val;
        sum += val;
    }
    if (!d.mp1 && !(sum & 1))
        blk[63] ^= 1;
}

uint8_t Clamp8(int v)
{
    return v < 0 ? 0 : v > 255 ? 255 : (uint8_t)v;
}

const int kDither[4][4] = {{-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};

// Decode one macroblock and append it, converted, to the output.
void Macroblock(Decoder &d, bool dtd, bool ofm, bool dte, bool sgn)
{
    // macroblock_type for an I picture: 1 intra, 01 intra + quant
    int quant = 0;
    if (Get(1)) {
    } else if (Get(1)) {
        quant = 1;
    } else {
        d.error = true;
        return;
    }
    int dctType = dtd ? (int)Get(1) : 0;
    if (quant) {
        int code = (int)Get(5);
        d.qscale = d.qst ? kNonLinearQ[code] : code << 1;
    }
    uint8_t y[16][16], cb[8][8], cr[8][8];
    int coef[64], pix[64];
    for (int b = 0; b < 6; b++) {
        int cc = b < 4 ? 0 : b - 3;
        IntraBlock(d, cc, coef);
        if (d.error)
            return;
        Idct(coef, pix);
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++) {
                uint8_t v = Clamp8(pix[r * 8 + c]);
                if (b < 4) {
                    int bx = (b & 1) * 8, row;
                    if (dctType)
                        row = r * 2 + (b >> 1);
                    else
                        row = (b >> 1) * 8 + r;
                    y[row][bx + c] = v;
                } else if (b == 4) {
                    cb[r][c] = v;
                } else {
                    cr[r][c] = v;
                }
            }
    }
    // CSC (PCSX2's fixed-point BT.601 conversion)
    size_t at = g_out.size();
    g_out.resize(at + (ofm ? 512 : 1024));
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++) {
            int lum = (0x95 * std::max(0, (int)y[i][j] - 16)) >> 6;
            int vcr = cr[i >> 1][j >> 1] - 128, vcb = cb[i >> 1][j >> 1] - 128;
            int r = Clamp8((lum + ((0xCC * vcr) >> 6) + 1) >> 1);
            int g = Clamp8((lum + ((-0x68 * vcr) >> 6) + ((-0x32 * vcb) >> 6) + 1) >> 1);
            int bl = Clamp8((lum + ((0x102 * vcb) >> 6) + 1) >> 1);
            int a = 0x80;
            if (g_th0 && r < g_th0 && g < g_th0 && bl < g_th0) {
                r = g = bl = a = 0;
            } else if (g_th1 && r < g_th1 && g < g_th1 && bl < g_th1) {
                a = 0x40;
            }
            if (sgn) {
                r ^= 0x80;
                g ^= 0x80;
                bl ^= 0x80;
            }
            if (ofm) {
                if (dte) {
                    int k = kDither[i & 3][j & 3];
                    r = Clamp8(r + k);
                    g = Clamp8(g + k);
                    bl = Clamp8(bl + k);
                }
                uint16_t p = (uint16_t)((r >> 3) | (g >> 3) << 5 | (bl >> 3) << 10 | (a == 0x40) << 15);
                memcpy(&g_out[at + (i * 16 + j) * 2], &p, 2);
            } else {
                uint32_t p = (uint32_t)r | g << 8 | bl << 16 | (uint32_t)a << 24;
                memcpy(&g_out[at + (i * 16 + j) * 4], &p, 4);
            }
        }
}

void Idec(uint32_t cmd)
{
    uint32_t ctrl = Reg(kCtrl);
    Decoder d{};
    d.idp = ctrl >> 16 & 3;
    d.as = ctrl >> 20 & 1;
    d.ivf = ctrl >> 21 & 1;
    d.qst = ctrl >> 22 & 1;
    d.mp1 = ctrl >> 23 & 1;
    int qsc = cmd >> 16 & 31;
    d.qscale = d.qst ? kNonLinearQ[qsc] : qsc << 1;
    bool dtd = cmd >> 24 & 1, sgn = cmd >> 25 & 1, dte = cmd >> 26 & 1, ofm = cmd >> 27 & 1;
    for (int &p : d.dcPred)
        p = 128 << d.idp;
    if (d.mp1)
        for (int &p : d.dcPred)
            p = 128;
    g_bit += cmd & 0x3F;
    int count = 0;
    for (;;) {
        Macroblock(d, dtd, ofm, dte, sgn);
        if (d.error || Overrun()) {
            port_log("ipu: IDEC: bad macroblock %d at bit %zu of %zu", count, g_bit, g_in.size() * 8);
            break;
        }
        count++;
        // the next macroblock follows an address increment of 1; a start
        // code (or the stream's end) ends the command
        if (Peek(23) == 0 || g_bit + 8 > g_in.size() * 8)
            break;
        if (Get(1) != 1) {
            port_log("ipu: IDEC: macroblock address increment other than 1 after %d", count);
            break;
        }
    }
    if (g_logIpu)
        port_log("ipu: IDEC %08x ctrl %08x: %d macroblocks, %zu of %zu bytes", cmd, ctrl, count,
                 (g_bit + 7) / 8, g_in.size());
}

void Run(uint32_t cmd)
{
    int code = cmd >> 28;
    switch (code) {
    case 0: // BCLR
        g_bit = (cmd & 0x7F);
        break;
    case 1: // IDEC
        g_out.clear();
        g_outPos = 0;
        Idec(cmd);
        break;
    case 4: { // FDEC
        g_bit += cmd & 0x3F;
        uint32_t data = Peek(32);
        Reg(kCmd) = data;
        g_lastCmdValue = data;
        Reg(kCmdHi) = 0;
        return;
    }
    case 5: { // SETIQ
        g_bit += cmd & 0x3F;
        uint8_t *m = (cmd >> 27 & 1) ? g_niq : g_iq;
        for (int i = 0; i < 64; i++)
            m[i] = (uint8_t)Get(8);
        break;
    }
    case 6: // SETVQ
        for (int i = 0; i < 16; i++) {
            uint16_t lo = (uint16_t)Get(8);
            g_vqclut[i] = (uint16_t)(lo | Get(8) << 8);
        }
        break;
    case 9: // SETTH
        g_th0 = cmd & 0x1FF;
        g_th1 = cmd >> 16 & 0x1FF;
        break;
    default: {
        static bool logged[16];
        if (!logged[code]) {
            logged[code] = true;
            port_log("ipu: command %u (0x%08x) not emulated", code, cmd);
        }
        break;
    }
    }
    Reg(kBp) = (uint32_t)(g_bit & 0x7F);
    g_lastCmdValue = Reg(kCmd);
}

// Run IPU_CMD if the game wrote a new one.
void Poll(bool forDecode)
{
    uint32_t cmd = Reg(kCmd);
    if (cmd == g_lastCmdValue && !(forDecode && (cmd >> 28) == 1 && g_out.empty()))
        return;
    Run(cmd);
}

} // namespace

void port_ipu_init(void)
{
    static bool done;
    if (done)
        return;
    done = true;
    BuildTables();
    BuildIdct();
    for (int k = 0; k < 64; k++)
        g_zigzagIndex[kZigzag[k]] = (uint8_t)k;
    // sceIpuInit's SETIQ (libipu's iqval: MPEG's default intra matrix, in
    // zigzag order; 16s for non-intra) and SETTH 0, 0
    static const uint8_t kIntra[64] = {
        8, 16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27,
        27, 27, 26, 26, 26, 26, 27, 27, 27, 29, 29, 29, 34, 34, 34, 29,
        29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38, 37, 35, 35, 34,
        35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83,
    };
    memcpy(g_iq, kIntra, 64);
    memset(g_niq, 16, 64);
    g_th0 = g_th1 = 0;
}

namespace {
std::atomic<void (*)(const void *, size_t)> g_movieSink;
}

// ports/pc/sdk/mpeg.cpp, while libmpeg waits for its bitstream: ch4's data goes
// to it instead of the IPU.
void port_ipu_set_movie_sink(void (*sink)(const void *data, size_t bytes))
{
    g_movieSink.store(sink);
}

// DMA ch4 (toIPU): a new bitstream.
void port_ipu_dma_in(const void *data, uint32_t qwc, bool first)
{
    if (auto sink = g_movieSink.load()) {
        if (!first)
            sink(data, (size_t)qwc * 16);
        return;
    }
    std::lock_guard<std::mutex> l(g_lock);
    if (first) {
        g_in.clear();
        g_bit = 0;
    }
    const uint8_t *p = (const uint8_t *)data;
    g_in.insert(g_in.end(), p, p + qwc * 16);
}

// DMA ch3 (fromIPU): the decoder's output.
void port_ipu_dma_out(void *dst, uint32_t qwc)
{
    std::lock_guard<std::mutex> l(g_lock);
    Poll(true);
    size_t n = (size_t)qwc * 16, have = g_out.size() - g_outPos;
    if (have < n) {
        static int warned;
        if (warned++ < 10)
            port_log("ipu: ch3 wants %zu bytes, the IPU has %zu", n, have);
        memset((uint8_t *)dst + have, 0, n - have);
        n = have;
    }
    memcpy(dst, &g_out[g_outPos], n);
    g_outPos += n;
}

extern "C" {

int sceIpuSync(int mode, unsigned short timeout)
{
    // ch4 first: the command reads what it delivers
    while (Reg(kD4Chcr) & 0x100)
        port_cpu_pause();
    std::lock_guard<std::mutex> l(g_lock);
    Poll(false);
    return 0;
}

void sceIpuInit(void)
{
    port_ipu_init();
}

} // extern "C"
