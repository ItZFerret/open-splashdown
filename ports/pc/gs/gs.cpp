// The GS: GIF tags, registers, local memory and a software rasterizer
// (ports/pc/gs/gs.h). Drawing goes into the emulated 4 MB local memory with the
// PS2's swizzled layouts, so anything the game reads back (CLUTs, textures
// rendered to, BltDisplayToZ's framebuffer copies) behaves as on the PS2.
// The GIF stream is parsed on the caller's thread (the DMA thread); the
// primitives are drawn by worker threads, each owning bands of rows
// (Workers below), behind the parser.
//
// Not yet: mipmapping (level 0 only), dithering, AA1, the interlaced field
// modes, PSMZ formats beyond the block-table XOR, local->host for formats
// other than 32/16/8 bits.
#include "gs.h"

#include "port.h"
#include "port_ptr32.h"
#include "gs_tables.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include "port_simd.h"
#include "port_os.h"
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// No fused multiply-adds: every lane and path rounds as the scalar code.
#pragma clang fp contract(off)

namespace {

// ---- local memory ---------------------------------------------------------

const uint32_t kVramBytes = 4 << 20;
alignas(64) uint8_t g_vram[kVramBytes];
std::recursive_mutex g_mutex;

enum Psm : uint32_t {
    CT32 = 0x00, CT24 = 0x01, CT16 = 0x02, CT16S = 0x0A, T8 = 0x13, T4 = 0x14, T8H = 0x1B, T4HL = 0x24,
    T4HH = 0x2C, Z32 = 0x30, Z24 = 0x31, Z16 = 0x32, Z16S = 0x3A,
};

inline uint32_t &Word(uint32_t w)
{
    return ((uint32_t *)g_vram)[w & 0xFFFFF];
}

inline uint16_t &Half(uint32_t h)
{
    return ((uint16_t *)g_vram)[h & 0x1FFFFF];
}

// Word index of a 32-bit pixel. zx is 0x18 for the PSMZ layouts.
inline uint32_t Addr32(uint32_t bp, uint32_t bw, uint32_t x, uint32_t y, uint32_t zx = 0)
{
    uint32_t page = (y >> 5) * bw + (x >> 6);
    uint32_t block = bp + page * 32 + (kBlockTable32[(y >> 3) & 3][(x >> 3) & 7] ^ zx);
    return block * 64 + kColumnTable32[y & 7][x & 7];
}

// Halfword index of a 16-bit pixel (PSMCT16, or 16S with s set).
inline uint32_t Addr16(uint32_t bp, uint32_t bw, uint32_t x, uint32_t y, bool s, uint32_t zx = 0)
{
    uint32_t page = (y >> 6) * bw + (x >> 6);
    uint32_t b = s ? kBlockTable16S[(y >> 3) & 7][(x >> 4) & 3] : kBlockTable16[(y >> 3) & 7][(x >> 4) & 3];
    uint32_t block = bp + page * 32 + (b ^ zx);
    return block * 128 + kColumnTable16[y & 7][x & 15];
}

inline uint32_t Addr8(uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t pw = std::max(bw >> 1, 1u);
    uint32_t page = (y >> 6) * pw + (x >> 7);
    uint32_t block = bp + page * 32 + kBlockTable8[(y >> 4) & 3][(x >> 4) & 7];
    return (block * 256 + kColumnTable8[y & 15][x & 15]) & (kVramBytes - 1);
}

// Nibble index.
inline uint32_t Addr4(uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t pw = std::max(bw >> 1, 1u);
    uint32_t page = (y >> 7) * pw + (x >> 7);
    uint32_t block = bp + page * 32 + kBlockTable4[(y >> 4) & 7][(x >> 5) & 3];
    return (block * 512 + kColumnTable4[y & 15][x & 31]) & (kVramBytes * 2 - 1);
}

// A pixel's stored bits (32/24-bit colour or Z, 16-bit, or an index).
uint32_t ReadPixel(uint32_t psm, uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    switch (psm) {
    case CT32: return Word(Addr32(bp, bw, x, y));
    case CT24: return Word(Addr32(bp, bw, x, y)) & 0xFFFFFF;
    case Z32: return Word(Addr32(bp, bw, x, y, 0x18));
    case Z24: return Word(Addr32(bp, bw, x, y, 0x18)) & 0xFFFFFF;
    case CT16: return Half(Addr16(bp, bw, x, y, false));
    case CT16S: return Half(Addr16(bp, bw, x, y, true));
    case Z16: return Half(Addr16(bp, bw, x, y, false, 0x18));
    case Z16S: return Half(Addr16(bp, bw, x, y, true, 0x18));
    case T8: return g_vram[Addr8(bp, bw, x, y)];
    case T4: {
        uint32_t n = Addr4(bp, bw, x, y);
        return (g_vram[n >> 1] >> ((n & 1) * 4)) & 15;
    }
    case T8H: return Word(Addr32(bp, bw, x, y)) >> 24;
    case T4HL: return (Word(Addr32(bp, bw, x, y)) >> 24) & 15;
    case T4HH: return Word(Addr32(bp, bw, x, y)) >> 28;
    }
    return 0;
}

void WritePixel(uint32_t psm, uint32_t bp, uint32_t bw, uint32_t x, uint32_t y, uint32_t v, uint32_t mask = 0)
{
    switch (psm) {
    case CT32: case Z32: {
        uint32_t &w = Word(Addr32(bp, bw, x, y, psm == Z32 ? 0x18 : 0));
        w = (w & mask) | (v & ~mask);
        return;
    }
    case CT24: case Z24: {
        uint32_t &w = Word(Addr32(bp, bw, x, y, psm == Z24 ? 0x18 : 0));
        uint32_t m = mask | 0xFF000000;
        w = (w & m) | (v & ~m);
        return;
    }
    case CT16: case CT16S: case Z16: case Z16S: {
        uint16_t &h = Half(Addr16(bp, bw, x, y, psm == CT16S || psm == Z16S, psm >= Z32 ? 0x18 : 0));
        h = (uint16_t)((h & mask) | (v & ~mask));
        return;
    }
    case T8: g_vram[Addr8(bp, bw, x, y)] = (uint8_t)v; return;
    case T4: {
        uint32_t n = Addr4(bp, bw, x, y);
        uint8_t &b = g_vram[n >> 1];
        b = (n & 1) ? (uint8_t)((b & 0x0F) | (v << 4)) : (uint8_t)((b & 0xF0) | (v & 15));
        return;
    }
    case T8H: { uint32_t &w = Word(Addr32(bp, bw, x, y)); w = (w & 0x00FFFFFF) | (v << 24); return; }
    case T4HL: { uint32_t &w = Word(Addr32(bp, bw, x, y)); w = (w & 0xF0FFFFFF) | ((v & 15) << 24); return; }
    case T4HH: { uint32_t &w = Word(Addr32(bp, bw, x, y)); w = (w & 0x0FFFFFFF) | ((v & 15) << 28); return; }
    }
}

uint32_t BitsPerPixel(uint32_t psm)
{
    switch (psm) {
    case CT32: case Z32: case T8H: case T4HL: case T4HH: return 32;
    case CT24: case Z24: return 24;
    case CT16: case CT16S: case Z16: case Z16S: return 16;
    case T8: return 8;
    case T4: return 4;
    }
    return 32;
}

// ---- registers ------------------------------------------------------------

inline uint64_t Bits(uint64_t v, int lo, int n)
{
    return (v >> lo) & ((1ull << n) - 1);
}

struct Context {
    uint64_t tex0, tex1, clamp, xyoffset, scissor, alpha, test, fba, frame, zbuf, miptbp1, miptbp2;
};

struct Vertex {
    int x, y;      // 12.4, window coordinates (XYOFFSET applied)
    uint32_t z;
    uint8_t r, g, b, a, f;
    float s, t, q;
    int u, v;      // 10.4 texels
};

struct State {
    Context ctx[2];
    uint64_t prim, prmode, prmodecont = 1, texclut, texa, fogcol, dthe, colclamp = 1, pabe, dimx;
    uint64_t bitbltbuf, trxpos, trxreg;
    // vertex attributes being built
    uint8_t r = 0x80, g = 0x80, b = 0x80, a = 0x80, fog = 0;
    float s = 0, t = 0, q = 1.0f;
    int u = 0, v = 0;
    Vertex queue[3];
    int count;
    // CLUT buffer: 512 halfwords (a 32-bit entry's low half at e, high at
    // e + 256), and the CBP0/CBP1 the CLD modes compare against.
    uint16_t clut[512];
    uint32_t cbp0, cbp1;
    // host<->local transfer
    int trxDir = 3;
    uint32_t trxX, trxY, trxLeft;
    uint8_t trxPartial[4];
    int trxPartialN;
    // bumped by every register write a draw depends on (DrawState), and by
    // every CLUT load
    uint64_t version = 1, clutVersion = 1;
} g;

uint64_t g_finishes, g_prims;
uint64_t g_frameSeq, g_flips;
uint64_t g_lastDispfb[2];

bool DisplayOk();

// ---- drawing state ----------------------------------------------------------
//
// Primitives are drawn by worker threads (Workers below), each owning
// interleaved bands of rows, while the GIF parser runs ahead: a primitive
// carries the registers it is drawn with as a DrawState snapshot (and the
// CLUT as one of its own), made when they change.

struct Attr {
    bool iip, tme, fge, abe, fst;
    int ctxt;
    bool operator==(const Attr &) const = default;
};

Attr CurrentAttr()
{
    uint64_t p = Bits(g.prmodecont, 0, 1) ? g.prim : g.prmode;
    Attr a;
    a.iip = Bits(p, 3, 1);
    a.tme = Bits(p, 4, 1);
    a.fge = Bits(p, 5, 1);
    a.abe = Bits(p, 6, 1);
    a.fst = Bits(p, 8, 1);
    a.ctxt = Bits(p, 9, 1);
    return a;
}

struct DrawState {
    Context c;
    Attr at;
    uint64_t texa, fogcol, pabe, colclamp;
    const uint16_t *clut;
    // decoded
    uint32_t tbp, tbw, tpsm, cpsm, csa;
    int tw, th, twLog, thLog;
    uint32_t wms, wmt;
    int minu, maxu, minv, maxv;
    bool linear, tcc;
    uint32_t tfx;
    uint32_t fbp, fbw, fpsm, fbmsk, zbp, zpsm;
    bool zwrite;
    // TEST, ALPHA, FOGCOL, FBA, COLCLAMP, PABE decoded
    bool ate, date, datm, zte, fb16, blend, pabeOn, clamp, fba;
    uint8_t atst, afail, ztst;
    int aref;
    uint32_t zmax;
    uint8_t ba, bb, bc, bd;
    int bfix;
    int fogr, fogg, fogb;
    // the texture decoded (Decoded textures below), or null: FetchTexel
    const uint32_t *tex;
    int tox, toy, tstride;
    bool fast;  // the fast path can draw it (given a decoded texture)
    uint64_t clutKey;  // the CLUT snapshot's content hash
};

struct Range {
    uint32_t a, b;  // 256-byte blocks [a, b) of local memory
    bool Overlaps(const Range &o) const { return a < o.b && o.a < b; }
};

const uint32_t kBlocks = kVramBytes / 256;

// The blocks a buffer's pixels (x0..x1, y0..y1) can lie in: whole pages from
// the first to the last (the block tables permute blocks within a page). A
// range past the end of memory (which wraps) is all of it.
Range BufRange(uint32_t psm, uint32_t bp, uint32_t bw, uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1)
{
    uint32_t pw = 64, ph = 32, ppr = bw;
    switch (psm) {
    case CT16: case CT16S: case Z16: case Z16S: ph = 64; break;
    case T8: pw = 128; ph = 64; ppr = std::max(bw >> 1, 1u); break;
    case T4: pw = 128; ph = 128; ppr = std::max(bw >> 1, 1u); break;
    }
    uint64_t a = bp + ((uint64_t)(y0 / ph) * ppr + x0 / pw) * 32;
    uint64_t b = bp + ((uint64_t)(y1 / ph) * ppr + x1 / pw) * 32 + 32;
    if (b > kBlocks)
        return {0, kBlocks};
    return {(uint32_t)a, (uint32_t)b};
}

// ---- texturing --------------------------------------------------------------

// The CLUT load TEX0/TEX2 requests (CLD).
void Hazard(const Range &r, bool write);

void LoadClut(uint64_t tex0)
{
    uint32_t cld = Bits(tex0, 61, 3);
    uint32_t cbp = Bits(tex0, 37, 14);
    switch (cld) {
    case 0: return;
    case 1: break;
    case 2: g.cbp0 = cbp; break;
    case 3: g.cbp1 = cbp; break;
    case 4: if (cbp == g.cbp0) return; g.cbp0 = cbp; break;
    case 5: if (cbp == g.cbp1) return; g.cbp1 = cbp; break;
    default: return;
    }
    uint32_t psm = Bits(tex0, 20, 6);
    uint32_t cpsm = Bits(tex0, 51, 4);
    uint32_t csm = Bits(tex0, 55, 1);
    uint32_t csa = Bits(tex0, 56, 5);
    bool i8 = psm == T8 || psm == T8H;
    if (!i8 && psm != T4 && psm != T4HL && psm != T4HH)
        return;
    int n = i8 ? 256 : 16;
    bool c32 = cpsm == CT32 || cpsm == CT24;
    // the drawing in flight must be done with the CLUT's source
    {
        uint32_t lpsm = c32 && !csm ? CT32 : cpsm == CT16S ? CT16S : CT16;
        if (csm == 0)
            Hazard(BufRange(lpsm, cbp, 1, 0, 15, 0, 15), false);
        else {
            uint32_t x = Bits(g.texclut, 6, 6) * 16, y = Bits(g.texclut, 12, 10);
            Hazard(BufRange(lpsm, cbp, Bits(g.texclut, 0, 6), x, x + n - 1, y, y), false);
        }
    }
    g.clutVersion++;
    for (int i = 0; i < n; i++) {
        uint32_t x, y;
        if (csm == 0) {
            if (i8) {
                // CSM1: 16x16, entries 8-15 and 16-23 of each 32 swapped
                uint32_t j = (i & 0xE7) | ((i & 8) << 1) | ((i & 16) >> 1);
                x = j & 15;
                y = j >> 4;
            } else {
                x = i & 7;
                y = i >> 3;
            }
        } else {
            // CSM2: one line at (COU * 16, COV), CBW * 64 wide
            x = Bits(g.texclut, 6, 6) * 16 + i;
            y = Bits(g.texclut, 12, 10);
        }
        uint32_t bw = csm ? Bits(g.texclut, 0, 6) : 1;
        if (c32 && !csm) {
            uint32_t c = ReadPixel(CT32, cbp, bw, x, y);
            uint32_t e = (i8 ? i : csa * 16 + i) & 255;
            g.clut[e] = (uint16_t)c;
            g.clut[e + 256] = (uint16_t)(c >> 16);
        } else {
            uint32_t c = ReadPixel(cpsm == CT16S ? CT16S : CT16, cbp, bw, x, y);
            uint32_t e = (i8 ? i : csa * 16 + i) & 511;
            g.clut[e] = (uint16_t)c;
        }
    }
}

inline uint32_t Expand16(uint32_t c, uint64_t texa)
{
    uint32_t r = (c & 31) << 3, gg = ((c >> 5) & 31) << 3, b = ((c >> 10) & 31) << 3;
    uint32_t a;
    if (c & 0x8000)
        a = Bits(texa, 32, 8);
    else
        a = (Bits(texa, 15, 1) && !(c & 0x7FFF)) ? 0 : Bits(texa, 0, 8);
    return r | gg << 8 | b << 16 | a << 24;
}

inline uint32_t ClutColor(const DrawState &S, uint32_t idx);

// A texel as RGBA8 (A as the GS's 0-0x80-0xFF).
inline uint32_t FetchTexel(const DrawState &S, int x, int y)
{
    uint32_t tbp = S.tbp, tbw = S.tbw, psm = S.tpsm;
    uint32_t cpsm = S.cpsm, csa = S.csa;
    uint64_t texa = S.texa;
    switch (psm) {
    case CT32: return Word(Addr32(tbp, tbw, x, y));
    case CT24: case Z24: {
        uint32_t c = ReadPixel(psm, tbp, tbw, x, y);
        uint32_t a = (Bits(texa, 15, 1) && !c) ? 0 : Bits(texa, 0, 8);
        return c | a << 24;
    }
    case Z32: return ReadPixel(psm, tbp, tbw, x, y);
    case CT16: case CT16S: case Z16: case Z16S: return Expand16(ReadPixel(psm, tbp, tbw, x, y), texa);
    }
    return ClutColor(S, ReadPixel(psm, tbp, tbw, x, y));
}

inline uint32_t ClutColor(const DrawState &S, uint32_t idx)
{
    uint32_t psm = S.tpsm, cpsm = S.cpsm, csa = S.csa;
    uint64_t texa = S.texa;
    bool i8 = psm == T8 || psm == T8H;
    const uint16_t *clut = S.clut;
    if (cpsm == CT32 || cpsm == CT24) {
        uint32_t e = (i8 ? idx : (csa & 15) * 16 + idx) & 255;
        uint32_t c = clut[e] | (uint32_t)clut[e + 256] << 16;
        if (cpsm == CT24)
            c = (c & 0xFFFFFF) | (((Bits(texa, 15, 1) && !(c & 0xFFFFFF)) ? 0 : Bits(texa, 0, 8)) << 24);
        return c;
    }
    uint32_t e = (i8 ? idx : csa * 16 + idx) & 511;
    return Expand16(clut[e], texa);
}

inline int Wrap(int c, int size, uint32_t mode, int mn, int mx)
{
    switch (mode) {
    case 0: return c & (size - 1);
    case 1: return std::clamp(c, 0, size - 1);
    case 2: return std::clamp(c, mn, mx);
    case 3: return (c & mn) | mx;
    }
    return c;
}

// floorf without the libm call (SSE2 has no round instruction): the same
// value for every input, but -0 for -0 (the callers take an int of it).
inline float FloorF(float u)
{
    if (!(std::fabs(u) < 8388608.0f))  // integral already (or NaN, inf)
        return u;
    float t = (float)(int)u;
    return t > u ? t - 1.0f : t;
}

// A texel of the decoded copy if the state has one.
bool g_texCheck = getenv("PORT_GS_TEXCHECK");
inline uint32_t Texel(const DrawState &S, int x, int y)
{
    if (S.tex) {
        uint32_t v = S.tex[(y - S.toy) * S.tstride + (x - S.tox)];
        if (__builtin_expect(g_texCheck, 0)) {
            uint32_t m = FetchTexel(S, x, y);
            static std::atomic<int> n;
            if (m != v && __atomic_load_n(&g_finishes, __ATOMIC_RELAXED) > (uint64_t)atoi(getenv("PORT_GS_TEXCHECK")) && n++ < 300)
                port_log("texcheck: finish %llu tex0 %llx clamp %llx at %d,%d copy %08x memory %08x", (unsigned long long)__atomic_load_n(&g_finishes, __ATOMIC_RELAXED),
                         (unsigned long long)S.c.tex0, (unsigned long long)S.c.clamp, x, y, v, m);
        }
        return v;
    }
    return FetchTexel(S, x, y);
}

// Sample at texel coordinates (u, v) in texels (float).
inline uint32_t SampleTexture(const DrawState &S, float u, float v)
{
    int tw = S.tw, th = S.th;
    uint32_t wms = S.wms, wmt = S.wmt;
    int minu = S.minu, maxu = S.maxu, minv = S.minv, maxv = S.maxv;
    if (!S.linear) {
        int x = Wrap((int)FloorF(u), tw, wms, minu, maxu);
        int y = Wrap((int)FloorF(v), th, wmt, minv, maxv);
        return Texel(S, x, y);
    }
    u -= 0.5f;
    v -= 0.5f;
    int x0 = (int)FloorF(u), y0 = (int)FloorF(v);
    int fu = (int)((u - x0) * 256), fv = (int)((v - y0) * 256);
    int xa = Wrap(x0, tw, wms, minu, maxu), xb = Wrap(x0 + 1, tw, wms, minu, maxu);
    int ya = Wrap(y0, th, wmt, minv, maxv), yb = Wrap(y0 + 1, th, wmt, minv, maxv);
    uint32_t t00 = Texel(S, xa, ya), t10 = Texel(S, xb, ya);
    uint32_t t01 = Texel(S, xa, yb), t11 = Texel(S, xb, yb);
    uint32_t out = 0;
    for (int sh = 0; sh < 32; sh += 8) {
        int a0 = (t00 >> sh) & 255, a1 = (t10 >> sh) & 255, b0 = (t01 >> sh) & 255, b1 = (t11 >> sh) & 255;
        int top = a0 * (256 - fu) + a1 * fu, bot = b0 * (256 - fu) + b1 * fu;
        out |= (uint32_t)(((top * (256 - fv) + bot * fv) >> 16) & 255) << sh;
    }
    return out;
}

// ---- pixel pipeline -------------------------------------------------------

struct Pixel {
    int r, g, b, a;  // 0-255
    uint32_t z;
    int f;
};

// Pixel access for a buffer format, the common ones without a switch.
inline uint32_t ReadBuf(uint32_t psm, uint32_t bp, uint32_t bw, int x, int y)
{
    switch (psm) {
    case CT32: return Word(Addr32(bp, bw, x, y));
    case Z32: return Word(Addr32(bp, bw, x, y, 0x18));
    case Z24: return Word(Addr32(bp, bw, x, y, 0x18)) & 0xFFFFFF;
    }
    return ReadPixel(psm, bp, bw, x, y);
}

// Everything after texturing for one pixel at (x, y).
inline void PutPixel(const DrawState &S, int x, int y, Pixel p)
{
    uint32_t fbp = S.fbp, fbw = S.fbw, fpsm = S.fpsm;
    uint32_t fbmsk = S.fbmsk;
    bool writeFb = true, writeZ = S.zwrite, rgbOnly = false;
    uint32_t zbp = S.zbp, zpsm = S.zpsm;
    // fog
    if (S.at.fge) {
        int f = p.f;
        p.r = (p.r * f + S.fogr * (255 - f)) >> 8;
        p.g = (p.g * f + S.fogg * (255 - f)) >> 8;
        p.b = (p.b * f + S.fogb * (255 - f)) >> 8;
    }
    // alpha test
    if (S.ate) {
        int ref = S.aref;
        bool pass;
        switch (S.atst) {
        case 0: pass = false; break;
        case 1: pass = true; break;
        case 2: pass = p.a < ref; break;
        case 3: pass = p.a <= ref; break;
        case 4: pass = p.a == ref; break;
        case 5: pass = p.a >= ref; break;
        case 6: pass = p.a > ref; break;
        default: pass = p.a != ref; break;
        }
        if (!pass) {
            switch (S.afail) {
            case 0: return;
            case 1: writeZ = false; break;
            case 2: writeFb = false; break;
            case 3: writeZ = false; rgbOnly = true; break;
            }
        }
    }
    // destination alpha test
    if (S.date) {
        uint32_t d = ReadBuf(fpsm, fbp, fbw, x, y);
        bool bit = S.fb16 ? (d >> 15) & 1 : (d >> 31) & 1;
        if (bit != S.datm)
            return;
    }
    // depth test
    if (S.zte) {
        uint32_t zt = S.ztst;
        uint32_t z = std::min(p.z, S.zmax);
        if (zpsm == Z32 || zpsm == Z24) {
            uint32_t &w = Word(Addr32(zbp, fbw, x, y, 0x18));
            if (zt != 1) {
                uint32_t d = zpsm == Z24 ? w & 0xFFFFFF : w;
                if (zt == 0 || (zt == 2 && z < d) || (zt == 3 && z <= d))
                    return;
            }
            if (writeZ)
                w = zpsm == Z24 ? (w & 0xFF000000) | (z & 0xFFFFFF) : z;
        } else {
            if (zt != 1) {
                uint32_t d = ReadPixel(zpsm, zbp, fbw, x, y);
                if (zt == 0 || (zt == 2 && z < d) || (zt == 3 && z <= d))
                    return;
            }
            if (writeZ)
                WritePixel(zpsm, zbp, fbw, x, y, z);
        }
    }
    if (!writeFb)
        return;
    bool fb16 = S.fb16;
    uint32_t *fb32 = fpsm == CT32 ? &Word(Addr32(fbp, fbw, x, y)) : nullptr;
    uint32_t dst = 0;
    bool needDst = S.at.abe || fbmsk;
    if (needDst)
        dst = fb32 ? *fb32 : ReadPixel(fpsm, fbp, fbw, x, y);
    int r = p.r, gg = p.g, b = p.b;
    if (S.blend && !(S.pabeOn && p.a < 0x80)) {
        int dr, dg, db, da;
        if (fb16) {
            dr = (dst & 31) << 3; dg = ((dst >> 5) & 31) << 3; db = ((dst >> 10) & 31) << 3; da = (dst & 0x8000) ? 0x80 : 0;
        } else {
            dr = dst & 255; dg = (dst >> 8) & 255; db = (dst >> 16) & 255; da = fpsm == CT24 ? 0x80 : dst >> 24;
        }
        auto sel = [](int s, int cs, int cd) { return s == 0 ? cs : s == 1 ? cd : 0; };
        int A = S.ba, B = S.bb, D = S.bd;
        int ca = S.bc == 0 ? p.a : S.bc == 1 ? da : S.bfix;
        r = ((sel(A, p.r, dr) - sel(B, p.r, dr)) * ca >> 7) + sel(D, p.r, dr);
        gg = ((sel(A, p.g, dg) - sel(B, p.g, dg)) * ca >> 7) + sel(D, p.g, dg);
        b = ((sel(A, p.b, db) - sel(B, p.b, db)) * ca >> 7) + sel(D, p.b, db);
    }
    if (S.clamp) {
        r = std::clamp(r, 0, 255); gg = std::clamp(gg, 0, 255); b = std::clamp(b, 0, 255);
    } else {
        r &= 255; gg &= 255; b &= 255;
    }
    int a = p.a & 255;
    uint32_t out;
    if (fb16) {
        out = (r >> 3) | (gg >> 3) << 5 | (b >> 3) << 10 | ((a & 0x80) || S.fba ? 0x8000 : 0);
        uint32_t m = (fbmsk >> 3 & 31) | (fbmsk >> 11 & 31) << 5 | (fbmsk >> 19 & 31) << 10 | (fbmsk >> 31) << 15;
        if (rgbOnly)
            m |= 0x8000;
        WritePixel(fpsm, fbp, fbw, x, y, out, m);
    } else {
        if (S.fba)
            a |= 0x80;
        out = r | gg << 8 | b << 16 | (uint32_t)a << 24;
        uint32_t mask = fbmsk | (rgbOnly ? 0xFF000000 : 0);
        if (fb32)
            *fb32 = (*fb32 & mask) | (out & ~mask);
        else
            WritePixel(fpsm, fbp, fbw, x, y, out, mask);
    }
}

// PORT_PIXEL_DEBUG=x,y: log every pixel drawn there
int g_dbgX = -1, g_dbgY = -1;

// PutPixel's depth test alone (Z32/Z24, GEQUAL/GREATER): true if it fails.
inline bool DepthFails(const DrawState &S, int x, int y, uint32_t pz)
{
    uint32_t z = std::min(pz, S.zmax);
    uint32_t w = Word(Addr32(S.zbp, S.fbw, x, y, 0x18));
    uint32_t d = S.zpsm == Z24 ? w & 0xFFFFFF : w;
    return S.ztst == 2 ? z < d : z <= d;
}

// Texture and combine (TFX) for a pixel with vertex colour p and texel
// coordinates (u, v); then PutPixel.
inline void ShadePixel(const DrawState &S, int x, int y, Pixel p, float u, float v)
{
    if (__builtin_expect(x == g_dbgX && y == g_dbgY, 0))
        port_log("pixel %d,%d: flip %llu tex0 %llx uv %g,%g rgba %d,%d,%d,%d texel %08x", x, y,
                 (unsigned long long)__atomic_load_n(&g_finishes, __ATOMIC_RELAXED), (unsigned long long)S.c.tex0, u, v,
                 p.r, p.g, p.b, p.a, S.at.tme ? SampleTexture(S, u, v) : 0);
    if (S.at.tme) {
        uint32_t t = SampleTexture(S, u, v);
        int tr = t & 255, tg = (t >> 8) & 255, tb = (t >> 16) & 255, ta = t >> 24;
        bool tcc = S.tcc;
        switch (S.tfx) {
        case 0: // MODULATE
            p.r = std::min(tr * p.r >> 7, 255);
            p.g = std::min(tg * p.g >> 7, 255);
            p.b = std::min(tb * p.b >> 7, 255);
            if (tcc)
                p.a = std::min(ta * p.a >> 7, 255);
            break;
        case 1: // DECAL
            p.r = tr; p.g = tg; p.b = tb;
            if (tcc)
                p.a = ta;
            break;
        case 2: // HIGHLIGHT
            p.r = std::min((tr * p.r >> 7) + p.a, 255);
            p.g = std::min((tg * p.g >> 7) + p.a, 255);
            p.b = std::min((tb * p.b >> 7) + p.a, 255);
            if (tcc)
                p.a = std::min(ta + p.a, 255);
            break;
        case 3: // HIGHLIGHT2
            p.r = std::min((tr * p.r >> 7) + p.a, 255);
            p.g = std::min((tg * p.g >> 7) + p.a, 255);
            p.b = std::min((tb * p.b >> 7) + p.a, 255);
            if (tcc)
                p.a = ta;
            break;
        }
    }
    PutPixel(S, x, y, p);
}

struct Scissor {
    int x0, x1, y0, y1;  // inclusive, pixels
};

Scissor GetScissor(const Context &c)
{
    uint64_t s = c.scissor;
    return {(int)Bits(s, 0, 11), (int)Bits(s, 16, 11), (int)Bits(s, 32, 11), (int)Bits(s, 48, 11)};
}

// The texel coordinates a vertex carries, in texels.
void TexCoord(const Context &c, const Attr &at, float s, float t, float q, int u, int v, float &tu, float &tv)
{
    if (at.fst) {
        tu = u / 16.0f;
        tv = v / 16.0f;
    } else {
        float tw = (float)(1 << std::min<int>(Bits(c.tex0, 26, 4), 10));
        float th = (float)(1 << std::min<int>(Bits(c.tex0, 30, 4), 10));
        float qq = q != 0 ? q : 1e-20f;
        tu = s / qq * tw;
        tv = t / qq * th;
    }
}

// ---- primitives -------------------------------------------------------------
//
// A primitive is set up once (the parser's thread) and rasterized by every
// worker over the rows it owns (Rows): the per-pixel arithmetic is the same
// whoever draws a pixel, so the picture doesn't depend on the split.

enum PrimKind : uint8_t { P_POINT, P_LINE, P_TRI, P_SPRITE };

struct Prim {
    uint8_t kind;
    uint32_t state;
    int py0, py1;          // rows it may touch
    Vertex v[3];           // triangle: in drawing order (counter-clockwise)
    // triangle
    int px0, px1;
    int b0, b1, b2;
    double inv;
    float tu[3], tv[3], tq[3], tw, th;
    uint8_t lr, lg, lb, la; // flat colour (the last vertex)
    // sprite
    float u0, t0, dudx, dvdy;
};

// The rows a thread draws: those whose 8-row band b has b % n == k.
struct Rows {
    int n, k;
    bool Owns(int y) const { return n == 1 || ((y >> 3) % n) == k; }
    // the first row >= y it owns
    int Next(int y) const
    {
        if (n == 1)
            return y;
        int band = y >> 3, off = (k - band % n + n) % n;
        return off ? (band + off) << 3 : y;
    }
};

// The port's big buffers are mapped outside the game's memory (the brk heap
// below 0x10000000 and the executable's .bss: the race needs all of it).
template <class T> T *BigArray(size_t n)
{
    void *p = port_map(n * sizeof(T));
    if (!p)
        port_fatal("GS: can't map %zu bytes", n * sizeof(T));
    return (T *)p;
}

const int kStates = 1 << 16;
DrawState *const g_states = BigArray<DrawState>(kStates);
const int kCluts = 8192;
uint16_t (*const g_cluts)[512] = (uint16_t (*)[512])BigArray<uint16_t>(kCluts * 512);

void RasterPoint(const Prim &pr, const DrawState &S, Rows rows)
{
    const Vertex &a = pr.v[0];
    const Context &c = S.c;
    Scissor sc = GetScissor(c);
    int x = a.x >> 4, y = a.y >> 4;
    if (x < sc.x0 || x > sc.x1 || y < sc.y0 || y > sc.y1 || !rows.Owns(y))
        return;
    float u, v;
    TexCoord(c, S.at, a.s, a.t, a.q, a.u, a.v, u, v);
    ShadePixel(S, x, y, {a.r, a.g, a.b, a.a, a.z, a.f}, u, v);
}

void RasterLine(const Prim &pr, const DrawState &S, Rows rows)
{
    const Vertex &a = pr.v[0], &b = pr.v[1];
    const Context &c = S.c;
    const Attr &at = S.at;
    Scissor sc = GetScissor(c);
    int x0 = a.x >> 4, y0 = a.y >> 4, x1 = b.x >> 4, y1 = b.y >> 4;
    int n = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    float ua, va, ub, vb;
    TexCoord(c, at, a.s, a.t, a.q, a.u, a.v, ua, va);
    TexCoord(c, at, b.s, b.t, b.q, b.u, b.v, ub, vb);
    for (int i = 0; i < n; i++) {  // the last pixel is not drawn
        float k = n ? (float)i / n : 0;
        int x = x0 + (int)std::lround((x1 - x0) * k), y = y0 + (int)std::lround((y1 - y0) * k);
        if (x < sc.x0 || x > sc.x1 || y < sc.y0 || y > sc.y1 || !rows.Owns(y))
            continue;
        Pixel p;
        if (at.iip) {
            p = {(int)(a.r + (b.r - a.r) * k), (int)(a.g + (b.g - a.g) * k), (int)(a.b + (b.b - a.b) * k),
                 (int)(a.a + (b.a - a.a) * k), (uint32_t)(a.z + ((double)b.z - a.z) * k), (int)(a.f + (b.f - a.f) * k)};
        } else {
            p = {b.r, b.g, b.b, b.a, (uint32_t)(a.z + ((double)b.z - a.z) * k), b.f};
        }
        ShadePixel(S, x, y, p, ua + (ub - ua) * k, va + (vb - va) * k);
    }
}

void RasterSprite(const Prim &pr, const DrawState &S, Rows rows)
{
    const Vertex &v0 = pr.v[0], &v1 = pr.v[1];
    Pixel p = {v1.r, v1.g, v1.b, v1.a, v1.z, v1.f};
    float u0 = pr.u0, t0 = pr.t0, dudx = pr.dudx, dvdy = pr.dvdy;
    for (int y = rows.Next(pr.py0); y <= pr.py1; y = rows.Next(y + 1)) {
        float tv = t0 + (y * 16 - v0.y) * dvdy;
        for (int x = pr.px0; x <= pr.px1; x++) {
            float tu = u0 + (x * 16 - v0.x) * dudx;
            ShadePixel(S, x, y, p, tu, tv);
        }
    }
}

// Narrows [xs, xe] to the x where w + d * (x - x0) >= 0 (exact integer
// bounds); false if no x satisfies it.
inline bool Span(int64_t w, int64_t d, int64_t x0, int64_t &xs, int64_t &xe)
{
    if (d == 0)
        return w >= 0;
    if (d > 0) {
        int64_t k = -w;  // x - x0 >= ceil(k / d)
        xs = std::max(xs, x0 + (k <= 0 ? -((-k) / d) : (k + d - 1) / d));
    } else {
        int64_t nd = -d;  // x - x0 <= floor(w / nd)
        xe = std::min(xe, x0 + (w >= 0 ? w / nd : -((-w + nd - 1) / nd)));
    }
    return true;
}

// Edge functions with a top-left rule in exact integers: each row's covered
// pixels are one run (the triangle is convex), solved for (Span) instead of
// scanned. Barycentrics and attributes per pixel as before.
void RasterTriangle(const Prim &pr, const DrawState &S, Rows rows)
{
    const Attr &at = S.at;
    const Vertex *v[3] = {&pr.v[0], &pr.v[1], &pr.v[2]};
    double inv = pr.inv;
    int b0 = pr.b0, b1 = pr.b1, b2 = pr.b2;
    auto edge = [](const Vertex *p, const Vertex *q, int x, int y) {
        return (int64_t)(q->x - p->x) * (y - p->y) - (int64_t)(q->y - p->y) * (x - p->x);
    };
    // per pixel step in x: -(q.y - p.y) * 16
    int64_t d0 = -(int64_t)(v[2]->y - v[1]->y) * 16, d1 = -(int64_t)(v[0]->y - v[2]->y) * 16,
            d2 = -(int64_t)(v[1]->y - v[0]->y) * 16;
    const float *tu = pr.tu, *tv = pr.tv, *tq = pr.tq;
    float tw = pr.tw, th = pr.th;
    const bool earlyZ = S.zte && (S.ztst == 2 || S.ztst == 3) && (S.zpsm == Z32 || S.zpsm == Z24);
    // the vertices' attributes as double pairs: (r, g), (b, a), (u, v)
    __m128d crg[3], cba[3], cuv[3];
    for (int k = 0; k < 3; k++) {
        crg[k] = _mm_set_pd(v[k]->g, v[k]->r);
        cba[k] = _mm_set_pd(v[k]->a, v[k]->b);
        cuv[k] = _mm_set_pd(tv[k], tu[k]);
    }
    const __m128d twh = _mm_set_pd(th, tw);
    for (int y = rows.Next(pr.py0); y <= pr.py1; y = rows.Next(y + 1)) {
        int sy = y * 16;
        int sx = pr.px0 * 16;
        int64_t w0 = edge(v[1], v[2], sx, sy), w1 = edge(v[2], v[0], sx, sy), w2 = edge(v[0], v[1], sx, sy);
        // the row's run: the x where all three w + b >= 0, solved exactly
        int64_t xs = pr.px0, xe = pr.px1;
        if (!Span(w0 + b0, d0, pr.px0, xs, xe) || !Span(w1 + b1, d1, pr.px0, xs, xe) ||
            !Span(w2 + b2, d2, pr.px0, xs, xe) || xs > xe)
            continue;
        w0 += d0 * (xs - pr.px0);
        w1 += d1 * (xs - pr.px0);
        w2 += d2 * (xs - pr.px0);
        for (int x = (int)xs; x <= (int)xe; x++, w0 += d0, w1 += d1, w2 += d2) {
            double l0 = w0 * inv, l1 = w1 * inv, l2 = w2 * inv;
            Pixel p;
            // two attributes per SSE2 op: each lane as the scalar
            // (a0 * l0 + a1 * l1) + a2 * l2, so the same roundings
            const __m128d L0 = _mm_set1_pd(l0), L1 = _mm_set1_pd(l1), L2 = _mm_set1_pd(l2);
            auto lerp2 = [&](const __m128d *c) {
                return _mm_add_pd(_mm_add_pd(_mm_mul_pd(c[0], L0), _mm_mul_pd(c[1], L1)), _mm_mul_pd(c[2], L2));
            };
            if (at.iip) {
                __m128i rg = _mm_cvttpd_epi32(lerp2(crg)), ba = _mm_cvttpd_epi32(lerp2(cba));
                p.r = _mm_cvtsi128_si32(rg);
                p.g = _mm_cvtsi128_si32(_mm_shuffle_epi32(rg, 1));
                p.b = _mm_cvtsi128_si32(ba);
                p.a = _mm_cvtsi128_si32(_mm_shuffle_epi32(ba, 1));
            } else {
                p.r = pr.lr; p.g = pr.lg; p.b = pr.lb; p.a = pr.la;
            }
            // z and fog only where they are used (the depth test, FGE)
            p.z = S.zte ? (uint32_t)std::min(4294967295.0, v[0]->z * l0 + v[1]->z * l1 + v[2]->z * l2) : 0;
            // a pixel the depth test rejects writes nothing: before texturing
            if (earlyZ && DepthFails(S, x, y, p.z))
                continue;
            p.f = at.fge ? (int)(v[0]->f * l0 + v[1]->f * l1 + v[2]->f * l2) : 0;
            float u = 0, vv = 0;
            if (at.tme) {
                __m128d UV = lerp2(cuv);
                if (!at.fst) {
                    double Q = tq[0] * l0 + tq[1] * l1 + tq[2] * l2;
                    if (Q == 0)
                        Q = 1e-20;
                    UV = _mm_mul_pd(_mm_div_pd(UV, _mm_set1_pd(Q)), twh);
                }
                __m128 f2 = _mm_cvtpd_ps(UV);
                u = _mm_cvtss_f32(f2);
                vv = _mm_cvtss_f32(_mm_shuffle_ps(f2, f2, 1));
            }
            ShadePixel(S, x, y, p, u, vv);
        }
    }
}

// ---- the fast path (AVX2) ---------------------------------------------------------
//
// Triangles and sprites four pixels at a time, for the common states: a
// PSMCT32 frame, Z32/Z24 (or no depth test), no DATE, a decoded texture (or
// none). Each lane does the scalar path's operations in its order (the
// barycentrics and attributes in doubles, U/Q and V/Q, the float texel
// coordinates, the integer filter and pixel arithmetic), so the pixels are
// the same; a group whose texel coordinates are out of the filter's range
// (Q near 0: the scalar code's integer overflow) is sampled by the scalar
// code. PORT_GS_NOFAST=1 draws everything with the scalar code.

#define GS_AVX2 PORT_TARGET_AVX2
#define GS_AVX2_INLINE PORT_TARGET_AVX2 __attribute__((always_inline)) inline

bool g_fastOk;  // the CPU has AVX2, no PORT_GS_NOFAST or PORT_PIXEL_DEBUG

// Word offsets within a 32-line row of pages: [layout][y & 31][x & 63],
// layout 1 the PSMZ block order (Addr32's zx = 0x18)
struct Offsets32 {
    uint32_t t[2][32][64];
};
const Offsets32 g_off32 = [] {
    Offsets32 o;
    for (int z = 0; z < 2; z++)
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 64; x++)
                o.t[z][y][x] = (kBlockTable32[(y >> 3) & 3][(x >> 3) & 7] ^ (z ? 0x18 : 0)) * 64 + kColumnTable32[y & 7][x & 7];
    return o;
}();

// Addr32(bp, bw, x, y, zx) & 0xFFFFF for four x on row y
GS_AVX2_INLINE __m128i Addr32x4(uint32_t bp, uint32_t bw, __m128i X, int y, int z)
{
    uint32_t base = (bp + (uint32_t)(y >> 5) * bw * 32) * 64;
    __m128i page = _mm_slli_epi32(_mm_srli_epi32(X, 6), 11);  // (x >> 6) * 32 * 64
    __m128i off = _mm_i32gather_epi32((const int *)g_off32.t[z][y & 31], _mm_and_si128(X, _mm_set1_epi32(63)), 4);
    __m128i a = _mm_add_epi32(_mm_add_epi32(_mm_set1_epi32((int)base), page), off);
    return _mm_and_si128(a, _mm_set1_epi32(0xFFFFF));
}

GS_AVX2_INLINE __m128i Gather(const uint32_t *base, __m128i idx)
{
    return _mm_i32gather_epi32((const int *)base, idx, 4);
}

GS_AVX2_INLINE __m128i WrapX4(__m128i c, int size, uint32_t mode, int mn, int mx)
{
    switch (mode) {
    case 0: return _mm_and_si128(c, _mm_set1_epi32(size - 1));
    case 1: return _mm_min_epi32(_mm_max_epi32(c, _mm_setzero_si128()), _mm_set1_epi32(size - 1));
    case 2: return _mm_min_epi32(_mm_max_epi32(c, _mm_set1_epi32(mn)), _mm_set1_epi32(mx));  // mn <= mx (TexLookup)
    default: return _mm_or_si128(_mm_and_si128(c, _mm_set1_epi32(mn)), _mm_set1_epi32(mx));
    }
}

GS_AVX2_INLINE __m128i TexIndex(const DrawState &S, __m128i X, __m128i Y)
{
    __m128i row = _mm_mullo_epi32(_mm_sub_epi32(Y, _mm_set1_epi32(S.toy)), _mm_set1_epi32(S.tstride));
    return _mm_add_epi32(row, _mm_sub_epi32(X, _mm_set1_epi32(S.tox)));
}

// SampleTexture for four lanes (decoded texture); false if a lane's
// bilinear weights are out of range (the caller samples with the scalar code)
GS_AVX2_INLINE bool Sample4(const DrawState &S, __m128 u, __m128 v, __m128i &out)
{
    if (!S.linear) {
        __m128i X = WrapX4(_mm_cvttps_epi32(_mm_floor_ps(u)), S.tw, S.wms, S.minu, S.maxu);
        __m128i Y = WrapX4(_mm_cvttps_epi32(_mm_floor_ps(v)), S.th, S.wmt, S.minv, S.maxv);
        out = Gather(S.tex, TexIndex(S, X, Y));
        return true;
    }
    u = _mm_sub_ps(u, _mm_set1_ps(0.5f));
    v = _mm_sub_ps(v, _mm_set1_ps(0.5f));
    __m128i x0 = _mm_cvttps_epi32(_mm_floor_ps(u)), y0 = _mm_cvttps_epi32(_mm_floor_ps(v));
    __m128i fu = _mm_cvttps_epi32(_mm_mul_ps(_mm_sub_ps(u, _mm_cvtepi32_ps(x0)), _mm_set1_ps(256.0f)));
    __m128i fv = _mm_cvttps_epi32(_mm_mul_ps(_mm_sub_ps(v, _mm_cvtepi32_ps(y0)), _mm_set1_ps(256.0f)));
    // fu, fv in [0, 256]: else (garbage coordinates, Q near 0) the scalar
    // code, whose products overflow
    __m128i lim = _mm_set1_epi32(256);
    __m128i bad = _mm_or_si128(_mm_cmpgt_epi32(fu, lim), _mm_cmpgt_epi32(fv, lim));
    bad = _mm_or_si128(bad, _mm_srai_epi32(_mm_or_si128(fu, fv), 31));
    if (__builtin_expect(!_mm_testz_si128(bad, bad), 0))
        return false;
    __m128i one = _mm_set1_epi32(1);
    __m128i xa = WrapX4(x0, S.tw, S.wms, S.minu, S.maxu), xb = WrapX4(_mm_add_epi32(x0, one), S.tw, S.wms, S.minu, S.maxu);
    __m128i ya = WrapX4(y0, S.th, S.wmt, S.minv, S.maxv), yb = WrapX4(_mm_add_epi32(y0, one), S.th, S.wmt, S.minv, S.maxv);
    __m128i t00 = Gather(S.tex, TexIndex(S, xa, ya)), t10 = Gather(S.tex, TexIndex(S, xb, ya));
    __m128i t01 = Gather(S.tex, TexIndex(S, xa, yb)), t11 = Gather(S.tex, TexIndex(S, xb, yb));
    // per channel: top = a0 * (256 - fu) + a1 * fu (pmaddwd on 16-bit pairs),
    // then (top * (256 - fv) + bot * fv) >> 16
    __m128i wu = _mm_or_si128(_mm_sub_epi32(lim, fu), _mm_slli_epi32(fu, 16));
    __m128i gv = _mm_sub_epi32(lim, fv);
    __m128i m8 = _mm_set1_epi32(0xFF), o = _mm_setzero_si128();
    for (int sh = 0; sh < 32; sh += 8) {
        // (a run-time count: the shift-by-register forms, which NEON has too)
        const __m128i shv = _mm_cvtsi32_si128(sh);
        __m128i a0 = _mm_and_si128(_mm_srl_epi32(t00, shv), m8), a1 = _mm_and_si128(_mm_srl_epi32(t10, shv), m8);
        __m128i b0 = _mm_and_si128(_mm_srl_epi32(t01, shv), m8), b1 = _mm_and_si128(_mm_srl_epi32(t11, shv), m8);
        __m128i top = _mm_madd_epi16(_mm_or_si128(a0, _mm_slli_epi32(a1, 16)), wu);
        __m128i bot = _mm_madd_epi16(_mm_or_si128(b0, _mm_slli_epi32(b1, 16)), wu);
        __m128i s = _mm_add_epi32(_mm_mullo_epi32(top, gv), _mm_mullo_epi32(bot, fv));
        o = _mm_or_si128(o, _mm_sll_epi32(_mm_and_si128(_mm_srli_epi32(s, 16), m8), shv));
    }
    out = o;
    return true;
}

GS_AVX2_INLINE __m128i Min255(__m128i a)
{
    return _mm_min_epi32(a, _mm_set1_epi32(255));
}

// A group of four pixels on row y from x (lanes in m): colour, depth, fog,
// texel coordinates as the scalar path has them before ShadePixel.
struct Quad {
    __m128i m;          // lanes to draw (all-ones)
    __m128i r, g, b, a;
    __m128i z, f;
    __m128 u, v;
};

// ShadePixel + PutPixel for a quad. zd: the depth buffer's words
// (gathered by the caller for the early test), za their addresses.
template <bool TME, bool FGE, bool ZTE, bool ABE>
GS_AVX2_INLINE void Shade4(const DrawState &S, int x, int y, Quad q, __m128i zd, __m128i za)
{
    const __m128i ones = _mm_set1_epi32(-1), zero = _mm_setzero_si128();
    __m128i r = q.r, g = q.g, b = q.b, a = q.a;
    if constexpr (TME) {
        __m128i t;
        if (__builtin_expect(!Sample4(S, q.u, q.v, t), 0)) {
            alignas(16) float uu[4], vv[4];
            alignas(16) uint32_t tt[4];
            _mm_store_ps(uu, q.u);
            _mm_store_ps(vv, q.v);
            for (int k = 0; k < 4; k++)
                tt[k] = SampleTexture(S, uu[k], vv[k]);
            t = _mm_load_si128((const __m128i *)tt);
        }
        __m128i m8 = _mm_set1_epi32(0xFF);
        __m128i tr = _mm_and_si128(t, m8), tg = _mm_and_si128(_mm_srli_epi32(t, 8), m8);
        __m128i tb = _mm_and_si128(_mm_srli_epi32(t, 16), m8), ta = _mm_srli_epi32(t, 24);
        bool tcc = S.tcc;
        switch (S.tfx) {
        case 0:
            r = Min255(_mm_srai_epi32(_mm_mullo_epi32(tr, r), 7));
            g = Min255(_mm_srai_epi32(_mm_mullo_epi32(tg, g), 7));
            b = Min255(_mm_srai_epi32(_mm_mullo_epi32(tb, b), 7));
            if (tcc)
                a = Min255(_mm_srai_epi32(_mm_mullo_epi32(ta, a), 7));
            break;
        case 1:
            r = tr; g = tg; b = tb;
            if (tcc)
                a = ta;
            break;
        case 2: case 3:
            r = Min255(_mm_add_epi32(_mm_srai_epi32(_mm_mullo_epi32(tr, r), 7), a));
            g = Min255(_mm_add_epi32(_mm_srai_epi32(_mm_mullo_epi32(tg, g), 7), a));
            b = Min255(_mm_add_epi32(_mm_srai_epi32(_mm_mullo_epi32(tb, b), 7), a));
            if (tcc)
                a = S.tfx == 2 ? Min255(_mm_add_epi32(ta, a)) : ta;
            break;
        }
    }
    if constexpr (FGE) {
        __m128i f = q.f, nf = _mm_sub_epi32(_mm_set1_epi32(255), f);
        r = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(r, f), _mm_mullo_epi32(_mm_set1_epi32(S.fogr), nf)), 8);
        g = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(g, f), _mm_mullo_epi32(_mm_set1_epi32(S.fogg), nf)), 8);
        b = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(b, f), _mm_mullo_epi32(_mm_set1_epi32(S.fogb), nf)), 8);
    }
    __m128i m = q.m;
    __m128i zw = ZTE && S.zwrite ? m : zero;  // lanes writing depth
    __m128i fw = m;                           // ... colour
    __m128i rgbOnly = zero;
    if (S.ate) {
        __m128i ref = _mm_set1_epi32(S.aref), pass;
        switch (S.atst) {
        case 0: pass = zero; break;
        case 1: pass = ones; break;
        case 2: pass = _mm_cmpgt_epi32(ref, a); break;
        case 3: pass = _mm_xor_si128(_mm_cmpgt_epi32(a, ref), ones); break;
        case 4: pass = _mm_cmpeq_epi32(a, ref); break;
        case 5: pass = _mm_xor_si128(_mm_cmpgt_epi32(ref, a), ones); break;
        case 6: pass = _mm_cmpgt_epi32(a, ref); break;
        default: pass = _mm_xor_si128(_mm_cmpeq_epi32(a, ref), ones); break;
        }
        switch (S.afail) {
        case 0: fw = _mm_and_si128(fw, pass); zw = _mm_and_si128(zw, pass); break;
        case 1: zw = _mm_and_si128(zw, pass); break;
        case 2: fw = _mm_and_si128(fw, pass); break;
        case 3: zw = _mm_and_si128(zw, pass); rgbOnly = _mm_andnot_si128(pass, m); break;
        }
    }
    if constexpr (ZTE) {
        int zmask = _mm_movemask_ps(_mm_castsi128_ps(zw));
        if (zmask) {
            __m128i z = _mm_min_epu32(q.z, _mm_set1_epi32((int)S.zmax));
            __m128i w = S.zpsm == Z24 ? _mm_or_si128(_mm_and_si128(zd, _mm_set1_epi32((int)0xFF000000)),
                                                     _mm_and_si128(z, _mm_set1_epi32(0xFFFFFF)))
                                      : z;
            alignas(16) uint32_t ww[4], aa[4];
            _mm_store_si128((__m128i *)ww, w);
            _mm_store_si128((__m128i *)aa, za);
            for (int k = 0; k < 4; k++)
                if (zmask & (1 << k))
                    ((uint32_t *)g_vram)[aa[k]] = ww[k];
        }
    }
    int fmask = _mm_movemask_ps(_mm_castsi128_ps(fw));
    if (!fmask)
        return;
    __m128i X = _mm_add_epi32(_mm_set1_epi32(x), _mm_setr_epi32(0, 1, 2, 3));
    __m128i fa = Addr32x4(S.fbp, S.fbw, X, y, 0);
    bool anyRgbOnly = !_mm_testz_si128(rgbOnly, rgbOnly);
    bool needDst = ABE || S.fbmsk || anyRgbOnly;
    __m128i dst = needDst ? _mm_mask_i32gather_epi32(zero, (const int *)g_vram, fa, fw, 4) : zero;
    if constexpr (ABE) {
        // lanes blended: all, or with PABE those with A >= 0x80
        __m128i bm = S.pabeOn ? _mm_cmpgt_epi32(a, _mm_set1_epi32(0x7F)) : ones;
        __m128i m8 = _mm_set1_epi32(0xFF);
        __m128i dr = _mm_and_si128(dst, m8), dg = _mm_and_si128(_mm_srli_epi32(dst, 8), m8);
        __m128i db = _mm_and_si128(_mm_srli_epi32(dst, 16), m8), da = _mm_srli_epi32(dst, 24);
        auto sel = [&](int s, __m128i cs, __m128i cd) GS_AVX2 { return s == 0 ? cs : s == 1 ? cd : zero; };
        __m128i ca = S.bc == 0 ? a : S.bc == 1 ? da : _mm_set1_epi32(S.bfix);
        auto mix = [&](__m128i cs, __m128i cd) GS_AVX2 {
            __m128i d = _mm_sub_epi32(sel(S.ba, cs, cd), sel(S.bb, cs, cd));
            __m128i v = _mm_add_epi32(_mm_srai_epi32(_mm_mullo_epi32(d, ca), 7), sel(S.bd, cs, cd));
            return _mm_blendv_epi8(cs, v, bm);
        };
        r = mix(r, dr);
        g = mix(g, dg);
        b = mix(b, db);
    }
    __m128i m8 = _mm_set1_epi32(0xFF);
    if (S.clamp) {
        r = _mm_min_epi32(_mm_max_epi32(r, zero), m8);
        g = _mm_min_epi32(_mm_max_epi32(g, zero), m8);
        b = _mm_min_epi32(_mm_max_epi32(b, zero), m8);
    } else {
        r = _mm_and_si128(r, m8);
        g = _mm_and_si128(g, m8);
        b = _mm_and_si128(b, m8);
    }
    __m128i av = _mm_and_si128(a, m8);
    if (S.fba)
        av = _mm_or_si128(av, _mm_set1_epi32(0x80));
    __m128i out = _mm_or_si128(_mm_or_si128(r, _mm_slli_epi32(g, 8)), _mm_or_si128(_mm_slli_epi32(b, 16), _mm_slli_epi32(av, 24)));
    __m128i mask = _mm_or_si128(_mm_set1_epi32((int)S.fbmsk), _mm_and_si128(rgbOnly, _mm_set1_epi32((int)0xFF000000)));
    out = _mm_or_si128(_mm_and_si128(dst, mask), _mm_andnot_si128(mask, out));
    alignas(16) uint32_t oo[4], aa[4];
    _mm_store_si128((__m128i *)oo, out);
    _mm_store_si128((__m128i *)aa, fa);
    for (int k = 0; k < 4; k++)
        if (fmask & (1 << k))
            ((uint32_t *)g_vram)[aa[k]] = oo[k];
}

// The early depth test for a quad: narrows q.m; zd, za the buffer's words
// and their addresses. false: nothing left to draw.
template <bool ZTE>
GS_AVX2_INLINE bool DepthTest4(const DrawState &S, int x, int y, Quad &q, __m128i &zd, __m128i &za)
{
    if constexpr (!ZTE)
        return true;
    __m128i X = _mm_add_epi32(_mm_set1_epi32(x), _mm_setr_epi32(0, 1, 2, 3));
    za = Addr32x4(S.zbp, S.fbw, X, y, 1);
    zd = _mm_mask_i32gather_epi32(_mm_setzero_si128(), (const int *)g_vram, za, q.m, 4);
    __m128i z = _mm_min_epu32(q.z, _mm_set1_epi32((int)S.zmax));
    __m128i d = S.zpsm == Z24 ? _mm_and_si128(zd, _mm_set1_epi32(0xFFFFFF)) : zd;
    const __m128i sgn = _mm_set1_epi32((int)0x80000000);
    __m128i zs = _mm_xor_si128(z, sgn), ds = _mm_xor_si128(d, sgn);
    switch (S.ztst) {
    case 0: q.m = _mm_setzero_si128(); break;
    case 1: break;
    case 2: q.m = _mm_andnot_si128(_mm_cmpgt_epi32(ds, zs), q.m); break;  // z >= d
    case 3: q.m = _mm_and_si128(_mm_cmpgt_epi32(zs, ds), q.m); break;     // z > d
    }
    return !_mm_testz_si128(q.m, q.m);
}

GS_AVX2_INLINE __m256d Lerp4(__m256d c0, __m256d c1, __m256d c2, __m256d l0, __m256d l1, __m256d l2)
{
    return _mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(c0, l0), _mm256_mul_pd(c1, l1)), _mm256_mul_pd(c2, l2));
}

template <bool TME, bool IIP, bool FGE, bool ZTE, bool ABE>
GS_AVX2 void TriangleFast(const Prim &pr, const DrawState &S, Rows rows)
{
    const Vertex *v[3] = {&pr.v[0], &pr.v[1], &pr.v[2]};
    const __m256d INV = _mm256_set1_pd(pr.inv);
    int b0 = pr.b0, b1 = pr.b1, b2 = pr.b2;
    auto edge = [](const Vertex *p, const Vertex *q, int x, int y) {
        return (int64_t)(q->x - p->x) * (y - p->y) - (int64_t)(q->y - p->y) * (x - p->x);
    };
    int64_t d0 = -(int64_t)(v[2]->y - v[1]->y) * 16, d1 = -(int64_t)(v[0]->y - v[2]->y) * 16,
            d2 = -(int64_t)(v[1]->y - v[0]->y) * 16;
    const bool fst = S.at.fst;
    __m256d C[9][3];  // r g b a z f u v q
    for (int k = 0; k < 3; k++) {
        C[0][k] = _mm256_set1_pd(v[k]->r);
        C[1][k] = _mm256_set1_pd(v[k]->g);
        C[2][k] = _mm256_set1_pd(v[k]->b);
        C[3][k] = _mm256_set1_pd(v[k]->a);
        C[4][k] = _mm256_set1_pd(v[k]->z);
        C[5][k] = _mm256_set1_pd(v[k]->f);
        C[6][k] = _mm256_set1_pd(pr.tu[k]);
        C[7][k] = _mm256_set1_pd(pr.tv[k]);
        C[8][k] = _mm256_set1_pd(pr.tq[k]);
    }
    const __m256d TW = _mm256_set1_pd(pr.tw), TH = _mm256_set1_pd(pr.th);
    const __m256d STEP = _mm256_setr_pd(0, 1, 2, 3);
    const __m256d D0 = _mm256_set1_pd((double)d0), D1 = _mm256_set1_pd((double)d1), D2 = _mm256_set1_pd((double)d2);
    const __m256d D04 = _mm256_set1_pd(4.0 * d0), D14 = _mm256_set1_pd(4.0 * d1), D24 = _mm256_set1_pd(4.0 * d2);
    for (int y = rows.Next(pr.py0); y <= pr.py1; y = rows.Next(y + 1)) {
        int sy = y * 16;
        int sx = pr.px0 * 16;
        int64_t w0 = edge(v[1], v[2], sx, sy), w1 = edge(v[2], v[0], sx, sy), w2 = edge(v[0], v[1], sx, sy);
        int64_t xs = pr.px0, xe = pr.px1;
        if (!Span(w0 + b0, d0, pr.px0, xs, xe) || !Span(w1 + b1, d1, pr.px0, xs, xe) ||
            !Span(w2 + b2, d2, pr.px0, xs, xe) || xs > xe)
            continue;
        w0 += d0 * (xs - pr.px0);
        w1 += d1 * (xs - pr.px0);
        w2 += d2 * (xs - pr.px0);
        // the edge values of four pixels, exact in doubles (integers < 2^53)
        __m256d W0 = _mm256_add_pd(_mm256_set1_pd((double)w0), _mm256_mul_pd(STEP, D0));
        __m256d W1 = _mm256_add_pd(_mm256_set1_pd((double)w1), _mm256_mul_pd(STEP, D1));
        __m256d W2 = _mm256_add_pd(_mm256_set1_pd((double)w2), _mm256_mul_pd(STEP, D2));
        for (int x = (int)xs; x <= (int)xe; x += 4, W0 = _mm256_add_pd(W0, D04), W1 = _mm256_add_pd(W1, D14),
                 W2 = _mm256_add_pd(W2, D24)) {
            Quad q;
            int n = std::min(4, (int)xe - x + 1);
            q.m = _mm_cmpgt_epi32(_mm_set1_epi32(n), _mm_setr_epi32(0, 1, 2, 3));
            __m256d L0 = _mm256_mul_pd(W0, INV), L1 = _mm256_mul_pd(W1, INV), L2 = _mm256_mul_pd(W2, INV);
            auto lerp = [&](int i) GS_AVX2 { return Lerp4(C[i][0], C[i][1], C[i][2], L0, L1, L2); };
            if constexpr (ZTE) {
                __m256d zz = _mm256_min_pd(lerp(4), _mm256_set1_pd(4294967295.0));
                // (uint32_t) of a double in [0, 2^32): floor, then offset by 2^31
                __m256d fl = _mm256_sub_pd(_mm256_floor_pd(zz), _mm256_set1_pd(2147483648.0));
                q.z = _mm_xor_si128(_mm256_cvttpd_epi32(fl), _mm_set1_epi32((int)0x80000000));
            }
            __m128i zd, za;
            if (!DepthTest4<ZTE>(S, x, y, q, zd, za))
                continue;
            if constexpr (IIP) {
                q.r = _mm256_cvttpd_epi32(lerp(0));
                q.g = _mm256_cvttpd_epi32(lerp(1));
                q.b = _mm256_cvttpd_epi32(lerp(2));
                q.a = _mm256_cvttpd_epi32(lerp(3));
            } else {
                q.r = _mm_set1_epi32(pr.lr);
                q.g = _mm_set1_epi32(pr.lg);
                q.b = _mm_set1_epi32(pr.lb);
                q.a = _mm_set1_epi32(pr.la);
            }
            if constexpr (FGE)
                q.f = _mm256_cvttpd_epi32(lerp(5));
            if constexpr (TME) {
                __m256d U = lerp(6), V = lerp(7);
                if (!fst) {
                    __m256d Q = lerp(8);
                    Q = _mm256_blendv_pd(Q, _mm256_set1_pd(1e-20), _mm256_cmp_pd(Q, _mm256_setzero_pd(), _CMP_EQ_OQ));
                    U = _mm256_mul_pd(_mm256_div_pd(U, Q), TW);
                    V = _mm256_mul_pd(_mm256_div_pd(V, Q), TH);
                }
                q.u = _mm256_cvtpd_ps(U);
                q.v = _mm256_cvtpd_ps(V);
            }
            Shade4<TME, FGE, ZTE, ABE>(S, x, y, q, zd, za);
        }
    }
}

template <bool TME, bool FGE, bool ZTE, bool ABE>
GS_AVX2 void SpriteFast(const Prim &pr, const DrawState &S, Rows rows)
{
    const Vertex &v0 = pr.v[0], &v1 = pr.v[1];
    float u0 = pr.u0, t0 = pr.t0, dudx = pr.dudx, dvdy = pr.dvdy;
    for (int y = rows.Next(pr.py0); y <= pr.py1; y = rows.Next(y + 1)) {
        float tv = t0 + (y * 16 - v0.y) * dvdy;
        for (int x = pr.px0; x <= pr.px1; x += 4) {
            Quad q;
            int n = std::min(4, pr.px1 - x + 1);
            q.m = _mm_cmpgt_epi32(_mm_set1_epi32(n), _mm_setr_epi32(0, 1, 2, 3));
            q.r = _mm_set1_epi32(v1.r);
            q.g = _mm_set1_epi32(v1.g);
            q.b = _mm_set1_epi32(v1.b);
            q.a = _mm_set1_epi32(v1.a);
            q.z = _mm_set1_epi32((int)v1.z);
            q.f = _mm_set1_epi32(v1.f);
            __m128i zd, za;
            if (!DepthTest4<ZTE>(S, x, y, q, zd, za))
                continue;
            if constexpr (TME) {
                __m128i X16 = _mm_sub_epi32(_mm_slli_epi32(_mm_add_epi32(_mm_set1_epi32(x), _mm_setr_epi32(0, 1, 2, 3)), 4),
                                            _mm_set1_epi32(v0.x));
                q.u = _mm_add_ps(_mm_set1_ps(u0), _mm_mul_ps(_mm_cvtepi32_ps(X16), _mm_set1_ps(dudx)));
                q.v = _mm_set1_ps(tv);
            }
            Shade4<TME, FGE, ZTE, ABE>(S, x, y, q, zd, za);
        }
    }
}

using RasterFn = void (*)(const Prim &, const DrawState &, Rows);

template <int I> constexpr RasterFn TriEntry()
{
    return &TriangleFast<(I & 1) != 0, (I & 2) != 0, (I & 4) != 0, (I & 8) != 0, (I & 16) != 0>;
}
template <int I> constexpr RasterFn SpriteEntry()
{
    return &SpriteFast<(I & 1) != 0, (I & 4) != 0, (I & 8) != 0, (I & 16) != 0>;
}
template <int... I> constexpr std::array<RasterFn, 32> TriTable(std::integer_sequence<int, I...>)
{
    return {TriEntry<I>()...};
}
template <int... I> constexpr std::array<RasterFn, 32> SpriteTable(std::integer_sequence<int, I...>)
{
    return {SpriteEntry<I>()...};
}
const std::array<RasterFn, 32> g_triFast = TriTable(std::make_integer_sequence<int, 32>());
const std::array<RasterFn, 32> g_spriteFast = SpriteTable(std::make_integer_sequence<int, 32>());

// The fast path's table index for S, or -1
inline int FastIndex(const DrawState &S)
{
    if (!S.fast || (S.at.tme && !S.tex))
        return -1;
    return S.at.tme | S.at.iip << 1 | S.at.fge << 2 | S.zte << 3 | S.blend << 4;
}

void Raster(const Prim &pr, Rows rows)
{
    const DrawState &S = g_states[pr.state];
    switch (pr.kind) {
    case P_POINT: RasterPoint(pr, S, rows); break;
    case P_LINE: RasterLine(pr, S, rows); break;
    case P_TRI: {
        int f = FastIndex(S);
        if (f >= 0)
            g_triFast[f](pr, S, rows);
        else
            RasterTriangle(pr, S, rows);
        break;
    }
    case P_SPRITE: {
        int f = FastIndex(S);
        if (f >= 0)
            g_spriteFast[f](pr, S, rows);
        else
            RasterSprite(pr, S, rows);
        break;
    }
    }
}

// ---- workers ----------------------------------------------------------------
//
// PORT_GS_THREADS=N (default 8, 0: draw on the parser's thread): N threads
// each draw every primitive's rows in its bands, in order, from a ring the
// parser fills. The parser waits for them (Barrier) before anything that
// reads or writes local memory outside the drawing: transfers, CLUT loads
// from memory drawn to, readbacks, the frame capture; and before a primitive
// whose texture was drawn to by a primitive in flight, which draws to a
// texture one in flight reads, or which draws to memory in flight under
// another buffer layout. A primitive that reads what it draws is drawn alone
// on the parser's thread, in the GS's order.

const int kRing = 1 << 15;
Prim *const g_ring = BigArray<Prim>(kRing);
uint64_t g_next;                    // primitives queued (parser)
std::atomic<uint64_t> g_published;  // ... visible to the workers
std::atomic<uint32_t> g_wakeWord, g_sleepers;
struct alignas(64) WorkerState {
    std::atomic<uint64_t> done;
};
const int kMaxWorkers = 64;
WorkerState g_workers[kMaxWorkers];
int g_nWorkers = -1;
uint64_t g_minDone;                 // parser's lower bound of every worker's done
uint64_t g_barriers;
int g_inBarrier;                    // the parser waits for the workers (watchdog dumps)

enum { FUTEX_WAIT_PRIVATE, FUTEX_WAKE_PRIVATE };

void Futex(std::atomic<uint32_t> &w, int op, uint32_t v, long ns)
{
    if (op == FUTEX_WAIT_PRIVATE)
        port_futex_wait(&w, v, ns);
    else
        port_futex_wake(&w, v != 1);
}

// Spins until w != v or about ns pass; returns w.
uint64_t SpinWait(const std::atomic<uint64_t> &w, uint64_t v, long ns)
{
    auto until = std::chrono::steady_clock::now() + std::chrono::nanoseconds(ns);
    for (;;) {
        for (int i = 0; i < 64; i++) {
            uint64_t x = w.load(std::memory_order_acquire);
            if (x != v)
                return x;
            port_cpu_pause();
        }
        if (std::chrono::steady_clock::now() > until)
            return w.load(std::memory_order_acquire);
    }
}

void Publish()
{
    if (g_published.load(std::memory_order_relaxed) == g_next)
        return;
    g_published.store(g_next, std::memory_order_release);
    // store, then load: without the fence the load may pass the store and
    // miss a worker going to sleep (it then slept its 2 ms timeout)
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (g_sleepers.load()) {
        g_wakeWord.fetch_add(1);
        Futex(g_wakeWord, FUTEX_WAKE_PRIVATE, INT32_MAX, 0);
    }
}

void WorkerMain(int k)
{
    char name[16];
    snprintf(name, sizeof name, "gs-worker%d", k);
    port_thread_name(name);
    Rows rows = {g_nWorkers, k};
    uint64_t i = 0;
    WorkerState &me = g_workers[k];
    for (;;) {
        uint64_t end = g_published.load(std::memory_order_acquire);
        if (i == end) {
            // spin a little, then sleep until the parser publishes more
            end = SpinWait(g_published, i, 5000);
            if (end == i) {
                uint32_t w = g_wakeWord.load();
                g_sleepers.fetch_add(1);
                if (g_published.load() == i)
                    Futex(g_wakeWord, FUTEX_WAIT_PRIVATE, w, 2000000);
                g_sleepers.fetch_sub(1);
                continue;
            }
        }
        for (; i < end; i++) {
            const Prim &pr = g_ring[i & (kRing - 1)];
            if (pr.py1 >= rows.Next(pr.py0))
                Raster(pr, rows);
            if ((i & 31) == 31)
                me.done.store(i + 1, std::memory_order_release);
        }
        me.done.store(i, std::memory_order_release);
    }
}

void StartWorkers()
{
    const char *v = getenv("PORT_GS_THREADS");
    int n = v && *v ? atoi(v) : 8;
    g_nWorkers = std::clamp(n, 0, kMaxWorkers);
    port_watch_futex(&g_wakeWord, "gs.workerWake");
    for (int k = 0; k < g_nWorkers; k++)
        std::thread(WorkerMain, k).detach();
    if (const char *v = getenv("PORT_PIXEL_DEBUG"))
        sscanf(v, "%d,%d", &g_dbgX, &g_dbgY);
    g_fastOk = PORT_HAVE_AVX2() && !getenv("PORT_GS_NOFAST") && g_dbgX < 0;
}

// Pending: the memory the primitives since the last barrier draw to (with
// their layout) and read as textures.
struct WriteRange {
    Range r;
    uint64_t layout;
};
std::vector<WriteRange> g_pendW;
// the blocks read, a bit each
uint64_t g_pendR[kBlocks / 64];
bool g_pendRAny;

void AddRead(Range r)
{
    g_pendRAny = true;
    for (uint32_t b = r.a; b < r.b;) {
        uint32_t w = b >> 6, lo = b & 63, n = std::min<uint32_t>(64 - lo, r.b - b);
        g_pendR[w] |= (n == 64 ? ~0ull : ((1ull << n) - 1)) << lo;
        b += n;
    }
}

bool ReadPending(Range r)
{
    if (!g_pendRAny)
        return false;
    for (uint32_t b = r.a; b < r.b;) {
        uint32_t w = b >> 6, lo = b & 63, n = std::min<uint32_t>(64 - lo, r.b - b);
        if (g_pendR[w] & ((n == 64 ? ~0ull : ((1ull << n) - 1)) << lo))
            return true;
        b += n;
    }
    return false;
}
uint32_t g_rangesAddedFor = ~0u;  // the state whose ranges are in the sets

// Why the parser waited (PORT_GS_STATS=1 logs the counts and the time
// waited every 2000 waits, with the decoded-texture counts).
enum Wait {
    W_XFER_DRAWN,   // a transfer into memory being drawn
    W_XFER_READ,    // ... into a texture read from memory (not decoded)
    W_RING,         // the state or CLUT ring wraps
    W_SELF,         // a primitive reading what it draws
    W_TEX_DRAWN,    // a texture being drawn (render to texture)
    W_LOCAL_COPY,   // local->local
    W_XFER_ODD,     // a transfer that wraps or is empty
    W_READBACK,     // local->host
    W_DISPLAY,      // the frame capture
    W_DEBUG,        // gs_read_texture
    W_CLUT,         // a CLUT load from memory being drawn
    W_LAYOUT,       // drawing to memory in flight under another layout
    W_DRAW_READ,    // drawing over a texture read from memory
    W_COUNT
};
const char *const kWaitNames[W_COUNT] = {"xfer-drawn", "xfer-read", "ring", "self", "tex-drawn", "local-copy",
                                         "xfer-odd", "readback", "display", "debug", "clut", "layout", "draw-read"};
uint64_t g_waits[W_COUNT], g_waitNs[W_COUNT];
uint64_t g_texDecoded, g_texHits;

// Waits until the workers have drawn everything queued.
void Barrier(Wait why)
{
    g_pendW.clear();
    if (g_pendRAny) {
        memset(g_pendR, 0, sizeof g_pendR);
        g_pendRAny = false;
    }
    g_rangesAddedFor = ~0u;
    if (g_nWorkers <= 0 || g_minDone == g_next)
        return;
    g_barriers++;
    static const bool stats = getenv("PORT_GS_STATS");
    auto t0 = stats ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point();
    Publish();
    __atomic_store_n(&g_inBarrier, 1, __ATOMIC_RELAXED);
    for (int k = 0; k < g_nWorkers; k++) {
        for (int spin = 0; g_workers[k].done.load(std::memory_order_acquire) != g_next; spin++) {
            if (spin > 2000)
                std::this_thread::yield();
            else
                port_cpu_pause();
        }
    }
    g_minDone = g_next;
    __atomic_store_n(&g_inBarrier, 0, __ATOMIC_RELAXED);
    if (stats) {
        g_waits[why]++;
        g_waitNs[why] += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
        if (g_barriers % 2000 == 0) {
            char b[1024];
            int o = snprintf(b, sizeof b, "textures decoded %llu, reused %llu; waits", (unsigned long long)g_texDecoded,
                             (unsigned long long)g_texHits);
            for (int i = 0; i < W_COUNT; i++)
                if (g_waits[i])
                    o += snprintf(b + o, sizeof b - o, " %s %llu (%llu ms)", kWaitNames[i], (unsigned long long)g_waits[i],
                                  (unsigned long long)(g_waitNs[i] / 1000000));
            port_log("GS: %s", b);
        }
    }
}

void AddWrite(Range r, uint64_t layout)
{
    for (WriteRange &o : g_pendW)
        if (o.layout == layout && r.a <= o.r.b && o.r.a <= r.b) {
            o.r.a = std::min(o.r.a, r.a);
            o.r.b = std::max(o.r.b, r.b);
            return;
        }
    g_pendW.push_back({r, layout});
}

// The parser is about to read (or write) r outside the drawing: wait for
// the primitives in flight that draw to it (or read it).
void Hazard(const Range &r, bool write)
{
    if (g_pendW.empty() && !g_pendRAny)
        return;
    for (const WriteRange &o : g_pendW)
        if (o.r.Overlaps(r)) {
            Barrier(write ? W_XFER_DRAWN : W_CLUT);
            return;
        }
    if (write && ReadPending(r))
        Barrier(W_XFER_READ);
}

// ---- decoded textures ------------------------------------------------------------
//
// A textured primitive samples a copy of its texture decoded to RGBA8 (the
// texels FetchTexel gives, over the rectangle its wrap modes can reach),
// made when its state is first queued (Submit, after any wait for drawing
// the texture depends on). Memory written later (uploads, local copies,
// drawing) then can't change what a queued primitive reads, so those writes
// don't wait for the workers, and the workers read a texel with one load.
// Copies are kept for reuse until memory under them is written, in a ring
// whose space is reused only once the workers are past the primitives that
// read it. Not for textures a primitive draws over (drawn alone, reading
// memory) or bigger than kTexMax texels (read from memory, the pending-read
// sets guarding them).

const size_t kTexRing = 32u << 20;  // texels
const uint32_t kTexMax = 512 * 512;
uint32_t *const g_texRing = BigArray<uint32_t>(kTexRing);

struct TexKey {
    uint32_t tbp, tbw, psm, cpsm, csa, ox, oy, w, h, pad;  // no padding bytes (HashKey)
    uint64_t texa, clut;
    bool operator==(const TexKey &) const = default;
};

struct TexEntry {
    TexKey key;
    size_t off, n;      // texels in the ring
    Range r;            // the blocks it was decoded from
    uint64_t lastUse;   // the primitives before it may read it
    bool live;          // findable (not overwritten in memory since)
    uint16_t clut[512]; // indexed: the CLUT it was decoded with
};
// in ring order; made by the parser's thread, whose allocations stay out
// of the game's heap (a static deque allocates at start-up, on the main
// thread, from the game's heap: its layout moved and a race diverged)
std::deque<TexEntry> &TexList()
{
    static std::deque<TexEntry> *list = new std::deque<TexEntry>;
    return *list;
}
std::unordered_multimap<uint64_t, TexEntry *> g_texMap;
size_t g_texPos;
const uint32_t kTexPages = kBlocks / 32;
uint16_t g_texPages[kTexPages];  // live copies decoded from each page
TexEntry *g_curTex;  // the copy the current state reads

uint64_t HashKey(const TexKey &k)
{
    uint64_t h = 0x9E3779B97F4A7C15ull;
    const uint32_t *w = (const uint32_t *)&k;
    for (size_t i = 0; i < sizeof k / 4; i++)
        h = (h ^ w[i]) * 0x100000001B3ull;
    return h ^ (h >> 29);
}

uint64_t HashClut(const uint16_t *c)
{
    uint64_t h = 0xCBF29CE484222325ull;
    const uint64_t *w = (const uint64_t *)c;
    for (int i = 0; i < 128; i++)
        h = (h ^ w[i]) * 0x100000001B3ull;
    return h ^ (h >> 31);
}

void TexUnlink(TexEntry &e)
{
    if (!e.live)
        return;
    e.live = false;
    for (uint32_t p = e.r.a / 32; p < (e.r.b + 31) / 32; p++)
        g_texPages[p]--;
    auto [a, b] = g_texMap.equal_range(HashKey(e.key));
    for (auto it = a; it != b; ++it)
        if (it->second == &e) {
            g_texMap.erase(it);
            break;
        }
}

// Memory in r is about to change: drop the copies decoded from it.
void TexInvalidate(const Range &r)
{
    bool any = false;
    for (uint32_t p = r.a / 32; p < (r.b + 31) / 32 && p < kTexPages; p++)
        if (g_texPages[p]) {
            any = true;
            break;
        }
    if (!any)
        return;
    for (TexEntry &e : TexList())
        if (e.live && e.r.Overlaps(r)) {
            TexUnlink(e);
            // the next primitive gets a state of its own, which looks the
            // texture up again (a transfer can come between two primitives
            // of one state)
            g.version++;
        }
}

// Waits until every worker has drawn the primitives before i.
void WaitDone(uint64_t i)
{
    if (g_nWorkers <= 0 || g_minDone >= i)
        return;
    Publish();
    for (int spin = 0;; spin++) {
        uint64_t mn = g_next;
        for (int k = 0; k < g_nWorkers; k++)
            mn = std::min(mn, g_workers[k].done.load(std::memory_order_acquire));
        g_minDone = mn;
        if (mn >= i)
            return;
        if (spin > 2000)
            std::this_thread::yield();
        else
            port_cpu_pause();
    }
}

// Room for n texels: the oldest copies give theirs up once no queued
// primitive reads them.
uint32_t *TexAlloc(size_t n)
{
    auto evict = [] {
        TexEntry &e = TexList().front();
        WaitDone(e.lastUse);
        TexUnlink(e);
        TexList().pop_front();
    };
    if (g_texPos + n > kTexRing) {
        // the ring's end is skipped: the copies there are the oldest
        while (!TexList().empty() && TexList().front().off >= g_texPos)
            evict();
        g_texPos = 0;
    }
    size_t a = g_texPos, b = a + n;
    while (!TexList().empty() && TexList().front().off >= a && TexList().front().off < b)
        evict();
    g_texPos = b;
    return g_texRing + a;
}

// FetchTexel over [ox, ox + w) x [oy, oy + h), the format's switch and the
// CLUT's expansion out of the loop.
void Decode(const DrawState &S, int ox, int oy, int w, int h, uint32_t *out)
{
    uint32_t psm = S.tpsm, bp = S.tbp, bw = S.tbw;
    auto rows = [&](auto texel) {
        for (int y = 0; y < h; y++, out += w)
            for (int x = 0; x < w; x++)
                out[x] = texel(ox + x, oy + y);
    };
    uint32_t pal[256];
    if (S.clut) {
        int n = psm == T8 || psm == T8H ? 256 : 16;
        for (int i = 0; i < n; i++)
            pal[i] = ClutColor(S, i);
    }
    switch (psm) {
    case CT32: rows([&](int x, int y) { return Word(Addr32(bp, bw, x, y)); }); return;
    case T8: rows([&](int x, int y) { return pal[g_vram[Addr8(bp, bw, x, y)]]; }); return;
    case T4:
        rows([&](int x, int y) {
            uint32_t n = Addr4(bp, bw, x, y);
            return pal[(g_vram[n >> 1] >> ((n & 1) * 4)) & 15];
        });
        return;
    case T8H: rows([&](int x, int y) { return pal[Word(Addr32(bp, bw, x, y)) >> 24]; }); return;
    case T4HL: rows([&](int x, int y) { return pal[(Word(Addr32(bp, bw, x, y)) >> 24) & 15]; }); return;
    case T4HH: rows([&](int x, int y) { return pal[Word(Addr32(bp, bw, x, y)) >> 28]; }); return;
    case CT16: rows([&](int x, int y) { return Expand16(Half(Addr16(bp, bw, x, y, false)), S.texa); }); return;
    }
    rows([&](int x, int y) { return FetchTexel(S, x, y); });
}

// The decoded copy for state S (whose texture memory is current), or null.
TexEntry *TexLookup(DrawState &S, const Range &read)
{
    // the texels the wrap modes reach, per axis: [o, o + n)
    auto axis = [](uint32_t mode, int size, int mn, int mx, int &o, int &n) {
        switch (mode) {
        case 0: case 1: o = 0; n = size; return true;
        case 2: if (mn > mx) return false; o = mn; n = mx - mn + 1; return true;
        default: o = mx; n = (mn | mx) - mx + 1; return true;
        }
    };
    int ox, oy, w, h;
    if (!axis(S.wms, S.tw, S.minu, S.maxu, ox, w) || !axis(S.wmt, S.th, S.minv, S.maxv, oy, h) ||
        (uint64_t)w * h > kTexMax)
        return nullptr;
    bool indexed = S.clut != nullptr;
    TexKey k = {S.tbp, S.tbw, S.tpsm, indexed ? S.cpsm : 0, indexed ? S.csa : 0, (uint32_t)ox, (uint32_t)oy,
                (uint32_t)w, (uint32_t)h, 0, S.texa & 0xFF000080FFull, indexed ? S.clutKey : 0};
    uint64_t hash = HashKey(k);
    static int dbg = getenv("PORT_GS_TEXDBG") ? atoi(getenv("PORT_GS_TEXDBG")) : 0;
    if (dbg & 8)
        return nullptr;
    auto [a, b] = g_texMap.equal_range(hash);
    for (auto it = dbg & 1 ? b : a; it != b; ++it) {
        TexEntry &e = *it->second;
        if (e.key == k && (!indexed || !memcmp(e.clut, S.clut, sizeof e.clut))) {
            g_texHits++;
            return &e;
        }
    }
    g_texDecoded++;
    size_t n = (size_t)w * h;
    uint32_t *out = TexAlloc(n);
    TexList().emplace_back();
    TexEntry &e = TexList().back();
    e.key = k;
    e.off = out - g_texRing;
    e.n = n;
    e.r = read;
    e.lastUse = 0;
    e.live = true;
    if (indexed)
        memcpy(e.clut, S.clut, sizeof e.clut);
    for (uint32_t p = read.a / 32; p < (read.b + 31) / 32; p++)
        g_texPages[p]++;
    g_texMap.emplace(hash, &e);
    if (dbg & 2) {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                out[y * w + x] = FetchTexel(S, ox + x, oy + y);
    } else
        Decode(S, ox, oy, w, h, out);
    return &e;
}

// The drawing state for the next primitive (a new snapshot when a register
// changed), with the memory it reads and draws to.
struct StateRanges {
    bool tex;
    Range read;
    Range fb, z;
    uint64_t fbLayout, zLayout;
    bool hasZ;
};
StateRanges *const g_stateRanges = BigArray<StateRanges>(kStates);
uint32_t g_curState = ~0u, g_stateNext;
uint64_t g_curVersion, g_curClutVersion;
Attr g_curAttr;
uint32_t g_clutNext;
int g_curClut = -1;
uint64_t g_curClutHash;

uint32_t CurrentState(const Attr &at)
{
    if (g_curState != ~0u && g_curVersion == g.version && g_curAttr == at &&
        (!g_states[g_curState].clut || g_curClutVersion == g.clutVersion))
        return g_curState;
    const Context &c = g.ctx[at.ctxt];
    uint64_t tex0 = c.tex0;
    uint32_t psm = Bits(tex0, 20, 6);
    bool indexed = psm == T8 || psm == T4 || psm == T8H || psm == T4HL || psm == T4HH;
    const uint16_t *clut = nullptr;
    if (at.tme && indexed) {
        if (g_curClut < 0 || g_curClutVersion != g.clutVersion) {
            if (g_clutNext == kCluts) {
                Barrier(W_RING);  // the ring wraps: no primitive in flight uses a slot
                g_clutNext = 0;
            }
            g_curClut = g_clutNext++;
            memcpy(g_cluts[g_curClut], g.clut, sizeof g.clut);
            g_curClutVersion = g.clutVersion;
            g_curClutHash = HashClut(g.clut);
        }
        clut = g_cluts[g_curClut];
    }
    if (g_stateNext == kStates) {
        Barrier(W_RING);
        g_stateNext = 0;
    }
    uint32_t id = g_stateNext++;
    DrawState &S = g_states[id];
    S.c = c;
    S.at = at;
    S.texa = g.texa;
    S.fogcol = g.fogcol;
    S.pabe = g.pabe;
    S.colclamp = g.colclamp;
    S.clut = clut;
    S.clutKey = clut ? g_curClutHash : 0;
    S.tex = nullptr;
    S.tbp = Bits(tex0, 0, 14);
    S.tbw = Bits(tex0, 14, 6);
    S.tpsm = psm;
    S.cpsm = Bits(tex0, 51, 4);
    S.csa = Bits(tex0, 56, 5);
    S.twLog = std::min<int>(Bits(tex0, 26, 4), 10);
    S.thLog = std::min<int>(Bits(tex0, 30, 4), 10);
    S.tw = 1 << S.twLog;
    S.th = 1 << S.thLog;
    uint64_t cl = c.clamp;
    S.wms = Bits(cl, 0, 2);
    S.wmt = Bits(cl, 2, 2);
    S.minu = Bits(cl, 4, 10);
    S.maxu = Bits(cl, 14, 10);
    S.minv = Bits(cl, 24, 10);
    S.maxv = Bits(cl, 34, 10);
    S.linear = Bits(c.tex1, 5, 1);
    S.tcc = Bits(tex0, 34, 1);
    S.tfx = Bits(tex0, 35, 2);
    S.fbp = Bits(c.frame, 0, 9) * 32;
    S.fbw = Bits(c.frame, 16, 6);
    S.fpsm = Bits(c.frame, 24, 6);
    S.fbmsk = (uint32_t)(c.frame >> 32);
    S.zwrite = !Bits(c.zbuf, 32, 1);
    S.zbp = Bits(c.zbuf, 0, 9) * 32;
    S.zpsm = Bits(c.zbuf, 24, 4) | 0x30;
    uint64_t test = c.test;
    S.ate = Bits(test, 0, 1);
    S.atst = Bits(test, 1, 3);
    S.aref = Bits(test, 4, 8);
    S.afail = Bits(test, 12, 2);
    S.date = Bits(test, 14, 1);
    S.datm = Bits(test, 15, 1);
    S.zte = Bits(test, 16, 1);
    S.ztst = Bits(test, 17, 2);
    S.zmax = S.zpsm == Z32 ? 0xFFFFFFFFu : S.zpsm == Z24 ? 0xFFFFFF : 0xFFFF;
    S.fb16 = S.fpsm == CT16 || S.fpsm == CT16S;
    S.blend = at.abe;
    S.pabeOn = Bits(g.pabe, 0, 1);
    S.clamp = Bits(g.colclamp, 0, 1);
    S.fba = Bits(c.fba, 0, 1);
    S.ba = Bits(c.alpha, 0, 2);
    S.bb = Bits(c.alpha, 2, 2);
    S.bc = Bits(c.alpha, 4, 2);
    S.bd = Bits(c.alpha, 6, 2);
    S.bfix = Bits(c.alpha, 32, 8);
    S.fogr = Bits(g.fogcol, 0, 8);
    S.fogg = Bits(g.fogcol, 8, 8);
    S.fogb = Bits(g.fogcol, 16, 8);
    S.fast = g_fastOk && S.fpsm == CT32 && !S.date && (!S.zte || S.zpsm == Z32 || S.zpsm == Z24);
    // the memory it reads and draws to
    StateRanges &R = g_stateRanges[id];
    Scissor sc = GetScissor(c);
    R.tex = at.tme;
    if (at.tme) {
        uint32_t umax = S.wms <= 1 ? S.tw - 1 : 1023, vmax = S.wmt <= 1 ? S.th - 1 : 1023;
        R.read = BufRange(psm, S.tbp, S.tbw, 0, umax, 0, vmax);
    }
    uint32_t x1 = std::max(sc.x0, sc.x1), y1 = std::max(sc.y0, sc.y1);
    R.fb = BufRange(S.fpsm, S.fbp, S.fbw, sc.x0, x1, sc.y0, y1);
    R.fbLayout = (uint64_t)S.fpsm << 32 | S.fbw << 16 | S.fbp;
    R.hasZ = Bits(c.test, 16, 1);
    if (R.hasZ) {
        R.z = BufRange(S.zpsm, S.zbp, S.fbw, sc.x0, x1, sc.y0, y1);
        R.zLayout = (uint64_t)S.zpsm << 32 | S.fbw << 16 | S.zbp;
    }
    // a depth buffer sharing memory with the frame: in the scalar order
    if (R.hasZ && R.z.Overlaps(R.fb))
        S.fast = false;
    g_curState = id;
    g_curVersion = g.version;
    g_curAttr = at;
    return id;
}

// Queues a primitive (or draws it here: no workers, or it reads what it
// draws).
bool g_xferChecked;  // a transfer's destination checked against the drawing

void Submit(Prim &pr)
{
    uint32_t id = pr.state;
    g_xferChecked = false;
    if (g_rangesAddedFor != id) {
        const StateRanges &R = g_stateRanges[id];
        DrawState &S = g_states[id];
        // copies of what it draws over are stale
        TexInvalidate(R.fb);
        if (R.hasZ)
            TexInvalidate(R.z);
        // reads what it draws: alone, in order, from memory
        if (R.tex && (R.read.Overlaps(R.fb) || (R.hasZ && R.read.Overlaps(R.z)))) {
            Barrier(W_SELF);
            S.tex = nullptr;
            g_curTex = nullptr;
            Raster(pr, {1, 0});
            return;
        }
        Wait conflict = W_COUNT;
        for (const WriteRange &o : g_pendW) {
            if (R.tex && o.r.Overlaps(R.read))
                conflict = W_TEX_DRAWN;
            if (conflict == W_COUNT && ((o.r.Overlaps(R.fb) && o.layout != R.fbLayout) ||
                                        (R.hasZ && o.r.Overlaps(R.z) && o.layout != R.zLayout)))
                conflict = W_LAYOUT;
        }
        if (conflict == W_COUNT && (ReadPending(R.fb) || (R.hasZ && ReadPending(R.z))))
            conflict = W_DRAW_READ;
        if (conflict != W_COUNT)
            Barrier(conflict);
        g_curTex = nullptr;
        if (R.tex) {
            g_curTex = TexLookup(S, R.read);
            if (g_curTex) {
                S.tex = g_texRing + g_curTex->off;
                S.tox = (int)g_curTex->key.ox;
                S.toy = (int)g_curTex->key.oy;
                S.tstride = (int)g_curTex->key.w;
                static bool keepReads = getenv("PORT_GS_TEXDBG") && (atoi(getenv("PORT_GS_TEXDBG")) & 4);
                if (keepReads)
                    AddRead(R.read);
            } else {
                S.tex = nullptr;
                AddRead(R.read);
            }
        }
        AddWrite(R.fb, R.fbLayout);
        if (R.hasZ)
            AddWrite(R.z, R.zLayout);
        g_rangesAddedFor = id;
    }
    if (g_nWorkers <= 0) {
        Raster(pr, {1, 0});
        return;
    }
    if (g_curTex)
        g_curTex->lastUse = g_next + 1;
    // room in the ring
    if (g_next - g_minDone >= kRing) {
        Publish();
        for (;;) {
            uint64_t mn = g_next;
            for (int k = 0; k < g_nWorkers; k++)
                mn = std::min(mn, g_workers[k].done.load(std::memory_order_acquire));
            g_minDone = mn;
            if (g_next - mn < kRing - 1024)
                break;
            std::this_thread::yield();
        }
    }
    g_ring[g_next & (kRing - 1)] = pr;
    g_next++;
    if ((g_next & 255) == 0)
        Publish();
}

void SetupSprite(Prim &pr, const DrawState &S)
{
    const Vertex &v0 = pr.v[0], &v1 = pr.v[1];
    const Context &c = S.c;
    const Attr &at = S.at;
    Scissor sc = GetScissor(c);
    int xa = std::min(v0.x, v1.x), xb = std::max(v0.x, v1.x);
    int ya = std::min(v0.y, v1.y), yb = std::max(v0.y, v1.y);
    // pixels whose top-left corner lies in [a, b)
    pr.px0 = std::max((xa + 15) >> 4, sc.x0);
    pr.px1 = std::min(((xb + 15) >> 4) - 1, sc.x1);
    pr.py0 = std::max((ya + 15) >> 4, sc.y0);
    pr.py1 = std::min(((yb + 15) >> 4) - 1, sc.y1);
    if (pr.px0 > pr.px1)
        pr.py1 = pr.py0 - 1;
    float u0, t0, u1, t1;
    TexCoord(c, at, v0.s, v0.t, v0.q, v0.u, v0.v, u0, t0);
    TexCoord(c, at, v1.s, v1.t, v1.q, v1.u, v1.v, u1, t1);
    if (!at.fst) {
        // ST sprites: Q from the second vertex for both
        TexCoord(c, at, v0.s, v0.t, v1.q, 0, 0, u0, t0);
    }
    float dx = (float)(v1.x - v0.x), dy = (float)(v1.y - v0.y);
    pr.u0 = u0;
    pr.t0 = t0;
    pr.dudx = dx ? (u1 - u0) / dx : 0;
    pr.dvdy = dy ? (t1 - t0) / dy : 0;
}

// false: nothing to draw
bool SetupTriangle(Prim &pr, const Vertex &a, const Vertex &b, const Vertex &cc, const DrawState &S)
{
    const Attr &at = S.at;
    const Context &c = S.c;
    Scissor sc = GetScissor(c);
    const Vertex *v[3] = {&a, &b, &cc};
    int64_t area = (int64_t)(b.x - a.x) * (cc.y - a.y) - (int64_t)(b.y - a.y) * (cc.x - a.x);
    if (area == 0)
        return false;
    if (area < 0) {
        std::swap(v[1], v[2]);
        area = -area;
    }
    int minx = std::min({a.x, b.x, cc.x}), maxx = std::max({a.x, b.x, cc.x});
    int miny = std::min({a.y, b.y, cc.y}), maxy = std::max({a.y, b.y, cc.y});
    pr.px0 = std::max((minx + 15) >> 4, sc.x0);
    pr.px1 = std::min((maxx - 1) >> 4, sc.x1);
    pr.py0 = std::max((miny + 15) >> 4, sc.y0);
    pr.py1 = std::min((maxy - 1) >> 4, sc.y1);
    if (pr.px0 > pr.px1 || pr.py0 > pr.py1)
        return false;
    if ((int64_t)(pr.px1 - pr.px0 + 1) * (pr.py1 - pr.py0 + 1) > 1000000) {
        static int n;
        if (n++ < 8)
            port_log("GS: triangle covers %dx%d pixels (scissor %d-%d,%d-%d, frame %016llx, xyoffset %016llx)",
                     pr.px1 - pr.px0 + 1, pr.py1 - pr.py0 + 1, sc.x0, sc.x1, sc.y0, sc.y1, (unsigned long long)c.frame,
                     (unsigned long long)c.xyoffset);
    }
    for (int i = 0; i < 3; i++) {
        if (at.fst) {
            pr.tu[i] = v[i]->u / 16.0f;
            pr.tv[i] = v[i]->v / 16.0f;
            pr.tq[i] = 1;
        } else {
            pr.tu[i] = v[i]->s;
            pr.tv[i] = v[i]->t;
            pr.tq[i] = v[i]->q;
        }
    }
    pr.tw = (float)S.tw;
    pr.th = (float)S.th;
    pr.lr = cc.r; pr.lg = cc.g; pr.lb = cc.b; pr.la = cc.a;  // flat shading: the last vertex
    pr.inv = 1.0 / (double)area;
    auto bias = [](const Vertex *p, const Vertex *q) {
        // top or left edges include their pixels
        int dy = q->y - p->y, dx = q->x - p->x;
        return (dy < 0 || (dy == 0 && dx > 0)) ? 0 : -1;
    };
    pr.b0 = bias(v[1], v[2]);
    pr.b1 = bias(v[2], v[0]);
    pr.b2 = bias(v[0], v[1]);
    Vertex t[3] = {*v[0], *v[1], *v[2]};
    pr.v[0] = t[0];
    pr.v[1] = t[1];
    pr.v[2] = t[2];
    return true;
}

// A vertex from XYZ(F)2/3; kick draws.
void AddVertex(uint32_t x, uint32_t y, uint32_t z, int f, bool kick)
{
    Attr at = CurrentAttr();
    const Context &c = g.ctx[at.ctxt];
    Vertex v;
    v.x = (int)x - (int)Bits(c.xyoffset, 0, 16);
    v.y = (int)y - (int)Bits(c.xyoffset, 32, 16);
    v.z = z;
    v.f = f >= 0 ? (uint8_t)f : g.fog;
    v.r = g.r; v.g = g.g; v.b = g.b; v.a = g.a;
    v.s = g.s; v.t = g.t; v.q = g.q;
    v.u = g.u; v.v = g.v;
    int type = Bits(g.prim, 0, 3);
    g.queue[g.count++] = v;
    int need = type == 0 ? 1 : (type == 1 || type == 2 || type == 6) ? 2 : type == 7 ? 1 : 3;
    if (g.count < need)
        return;
    if (kick) {
        g_prims++;
        // PORT_GS_TRACE=N[-M]: log the primitives drawn after flip N (to M)
        static long traceFrom = -1, traceTo = -1;
        static bool traceInit = [] {
            if (const char *v = getenv("PORT_GS_TRACE")) {
                char *end;
                traceFrom = traceTo = strtol(v, &end, 0);
                if (*end == '-')
                    traceTo = strtol(end + 1, nullptr, 0);
            }
            return true;
        }();
        (void)traceInit;
        if ((long)g_finishes >= traceFrom && (long)g_finishes <= traceTo) {
            const Vertex &a = g.queue[0], &b = g.queue[need - 1];
            port_log("GS flip %llu prim %d iip%d tme%d abe%d fst%d ctx%d (%d,%d)-(%d,%d) rgba %02x%02x%02x%02x uv %d,%d-%d,%d "
                     "st %g,%g,%g tex0 %llx clamp %llx tex1 %llx alpha %llx test %llx frame %llx scissor %llx",
                     (unsigned long long)g_finishes, type, at.iip, at.tme, at.abe, at.fst, at.ctxt, a.x >> 4, a.y >> 4, b.x >> 4, b.y >> 4, b.r, b.g,
                     b.b, b.a, a.u >> 4, a.v >> 4, b.u >> 4, b.v >> 4, a.s, a.t, a.q, (unsigned long long)c.tex0,
                     (unsigned long long)c.clamp, (unsigned long long)c.tex1, (unsigned long long)c.alpha,
                     (unsigned long long)c.test, (unsigned long long)c.frame, (unsigned long long)c.scissor);
        }
        if (type <= 6) {
            Prim pr;
            pr.state = CurrentState(at);
            const DrawState &S = g_states[pr.state];
            bool draw = true;
            switch (type) {
            case 0:
                pr.kind = P_POINT;
                pr.v[0] = g.queue[0];
                pr.py0 = pr.py1 = g.queue[0].y >> 4;
                break;
            case 1: case 2:
                pr.kind = P_LINE;
                pr.v[0] = g.queue[0];
                pr.v[1] = g.queue[1];
                pr.py0 = std::min(g.queue[0].y, g.queue[1].y) >> 4;
                pr.py1 = std::max(g.queue[0].y, g.queue[1].y) >> 4;
                break;
            case 3: case 4: case 5:
                pr.kind = P_TRI;
                draw = SetupTriangle(pr, g.queue[0], g.queue[1], g.queue[2], S);
                break;
            case 6:
                pr.kind = P_SPRITE;
                pr.v[0] = g.queue[0];
                pr.v[1] = g.queue[1];
                SetupSprite(pr, S);
                draw = pr.py0 <= pr.py1;
                break;
            }
            if (draw)
                Submit(pr);
        }
    }
    switch (type) {
    case 0: case 1: case 3: case 6: case 7: g.count = 0; break;
    case 2: g.queue[0] = g.queue[1]; g.count = 1; break;
    case 4: g.queue[0] = g.queue[1]; g.queue[1] = g.queue[2]; g.count = 2; break;
    case 5: g.queue[1] = g.queue[2]; g.count = 2; break;
    }
}

// ---- transfers --------------------------------------------------------------

void StartTransfer()
{
    g.trxX = 0;
    g.trxY = 0;
    g.trxPartialN = 0;
    g_xferChecked = false;
    uint32_t w = Bits(g.trxreg, 0, 12), h = Bits(g.trxreg, 32, 12);
    g.trxLeft = w * h;
    if (g.trxDir == 2) {
        // local -> local: through a buffer, which also covers overlaps
        Barrier(W_LOCAL_COPY);
        uint32_t sbp = Bits(g.bitbltbuf, 0, 14), sbw = Bits(g.bitbltbuf, 16, 6), spsm = Bits(g.bitbltbuf, 24, 6);
        uint32_t dbp = Bits(g.bitbltbuf, 32, 14), dbw = Bits(g.bitbltbuf, 48, 6), dpsm = Bits(g.bitbltbuf, 56, 6);
        uint32_t sx = Bits(g.trxpos, 0, 11), sy = Bits(g.trxpos, 16, 11);
        uint32_t dx = Bits(g.trxpos, 32, 11), dy = Bits(g.trxpos, 48, 11);
        if (!w || !h || dx + w > 2048 || dy + h > 2048)
            TexInvalidate({0, kBlocks});
        else
            TexInvalidate(BufRange(dpsm, dbp, dbw, dx, dx + w - 1, dy, dy + h - 1));
        std::vector<uint32_t> buf(w * h);
        for (uint32_t y = 0; y < h; y++)
            for (uint32_t x = 0; x < w; x++)
                buf[y * w + x] = ReadPixel(spsm, sbp, sbw, (sx + x) & 2047, (sy + y) & 2047);
        for (uint32_t y = 0; y < h; y++)
            for (uint32_t x = 0; x < w; x++)
                WritePixel(dpsm, dbp, dbw, (dx + x) & 2047, (dy + y) & 2047, buf[y * w + x]);
        g.trxDir = 3;
    }
}

// A host->local transfer's writes must not pass the drawing in flight that
// reads or draws there.
void CheckTransfer()
{
    g_xferChecked = true;
    uint32_t dbp = Bits(g.bitbltbuf, 32, 14), dbw = Bits(g.bitbltbuf, 48, 6), dpsm = Bits(g.bitbltbuf, 56, 6);
    uint32_t w = Bits(g.trxreg, 0, 12), h = Bits(g.trxreg, 32, 12);
    uint32_t dx = Bits(g.trxpos, 32, 11), dy = Bits(g.trxpos, 48, 11);
    if (!w || !h || dx + w > 2048 || dy + h > 2048) {
        Barrier(W_XFER_ODD);
        TexInvalidate({0, kBlocks});
    } else {
        Range r = BufRange(dpsm, dbp, dbw, dx, dx + w - 1, dy, dy + h - 1);
        Hazard(r, true);
        TexInvalidate(r);
    }
}

// BITBLTBUF/TRXPOS/TRXREG's destination fields, decoded when they change.
struct XferRegs {
    uint32_t dbp, dbw, dpsm, w, dx, dy;
} g_xr;

void DecodeXfer()
{
    g_xr = {(uint32_t)Bits(g.bitbltbuf, 32, 14), (uint32_t)Bits(g.bitbltbuf, 48, 6), (uint32_t)Bits(g.bitbltbuf, 56, 6),
            (uint32_t)Bits(g.trxreg, 0, 12), (uint32_t)Bits(g.trxpos, 32, 11), (uint32_t)Bits(g.trxpos, 48, 11)};
}

inline void PutTransferPixel(uint32_t v)
{
    if (!g.trxLeft)
        return;
    if (__builtin_expect(!g_xferChecked, 0))
        CheckTransfer();
    uint32_t x = (g_xr.dx + g.trxX) & 2047, y = (g_xr.dy + g.trxY) & 2047;
    switch (g_xr.dpsm) {
    case CT32: Word(Addr32(g_xr.dbp, g_xr.dbw, x, y)) = v; break;
    case T8: g_vram[Addr8(g_xr.dbp, g_xr.dbw, x, y)] = (uint8_t)v; break;
    case T4: {
        uint32_t n = Addr4(g_xr.dbp, g_xr.dbw, x, y);
        uint8_t &b = g_vram[n >> 1];
        b = (n & 1) ? (uint8_t)((b & 0x0F) | (v << 4)) : (uint8_t)((b & 0xF0) | (v & 15));
        break;
    }
    default: WritePixel(g_xr.dpsm, g_xr.dbp, g_xr.dbw, x, y, v); break;
    }
    if (++g.trxX >= g_xr.w) {
        g.trxX = 0;
        g.trxY++;
    }
    g.trxLeft--;
}

void ImageData(const uint8_t *q)
{
    if (g.trxDir != 0)
        return;
    uint32_t bpp = BitsPerPixel(Bits(g.bitbltbuf, 56, 6));
    switch (bpp) {
    case 32: for (int i = 0; i < 4; i++) PutTransferPixel(((const uint32_t *)q)[i]); break;
    case 16: for (int i = 0; i < 8; i++) PutTransferPixel(((const uint16_t *)q)[i]); break;
    case 8: for (int i = 0; i < 16; i++) PutTransferPixel(q[i]); break;
    case 4:
        for (int i = 0; i < 16; i++) {
            PutTransferPixel(q[i] & 15);
            PutTransferPixel(q[i] >> 4);
        }
        break;
    case 24:
        for (int i = 0; i < 16; i++) {
            g.trxPartial[g.trxPartialN++] = q[i];
            if (g.trxPartialN == 3) {
                PutTransferPixel(g.trxPartial[0] | g.trxPartial[1] << 8 | g.trxPartial[2] << 16);
                g.trxPartialN = 0;
            }
        }
        break;
    }
}

// ---- registers ----------------------------------------------------------------

void LogOnce(uint32_t reg)
{
    static bool seen[256];
    if (!seen[reg & 255]) {
        seen[reg & 255] = true;
        port_log("GS: register 0x%02x ignored", reg);
    }
}

void WriteReg(uint32_t reg, uint64_t v)
{
    switch (reg) {
    case 0x00:
        g.prim = v;
        g.count = 0;
        return;
    case 0x01:
        g.r = Bits(v, 0, 8); g.g = Bits(v, 8, 8); g.b = Bits(v, 16, 8); g.a = Bits(v, 24, 8);
        { uint32_t qb = (uint32_t)(v >> 32); memcpy(&g.q, &qb, 4); }
        return;
    case 0x02: {
        uint32_t sb = (uint32_t)v, tb = (uint32_t)(v >> 32);
        memcpy(&g.s, &sb, 4);
        memcpy(&g.t, &tb, 4);
        return;
    }
    case 0x03: g.u = Bits(v, 0, 14); g.v = Bits(v, 16, 14); return;
    case 0x04: AddVertex(Bits(v, 0, 16), Bits(v, 16, 16), Bits(v, 32, 24), Bits(v, 56, 8), true); return;
    case 0x05: AddVertex(Bits(v, 0, 16), Bits(v, 16, 16), (uint32_t)(v >> 32), -1, true); return;
    case 0x0C: AddVertex(Bits(v, 0, 16), Bits(v, 16, 16), Bits(v, 32, 24), Bits(v, 56, 8), false); return;
    case 0x0D: AddVertex(Bits(v, 0, 16), Bits(v, 16, 16), (uint32_t)(v >> 32), -1, false); return;
    case 0x0A: g.fog = Bits(v, 56, 8); return;
    case 0x18: case 0x19: g.ctx[reg - 0x18].xyoffset = v; return;  // applied by AddVertex
    case 0x1A: g.prmodecont = v; return;                             // CurrentAttr
    case 0x1B: g.prmode = v; return;
    }
    // the rest changes what a primitive is drawn with
    g.version++;
    switch (reg) {
    case 0x06: case 0x07:
        g.ctx[reg - 6].tex0 = v;
        LoadClut(v);
        break;
    case 0x08: case 0x09: g.ctx[reg - 8].clamp = v; break;
    case 0x14: case 0x15: g.ctx[reg - 0x14].tex1 = v; break;
    case 0x16: case 0x17: {
        // TEX2: TEX0's PSM and CLUT fields only
        uint64_t m = (0x3Full << 20) | (0x7FFFFFFFull << 37);
        uint64_t &t = g.ctx[reg - 0x16].tex0;
        t = (t & ~m) | (v & m);
        LoadClut(t);
        break;
    }
    case 0x1C: g.texclut = v; break;
    case 0x22: break;  // SCANMSK
    case 0x34: case 0x35: g.ctx[reg - 0x34].miptbp1 = v; break;
    case 0x36: case 0x37: g.ctx[reg - 0x36].miptbp2 = v; break;
    case 0x3B: g.texa = v; break;
    case 0x3D: g.fogcol = v; break;
    case 0x3F: break;  // TEXFLUSH
    case 0x40: case 0x41: g.ctx[reg - 0x40].scissor = v; break;
    case 0x42: case 0x43: g.ctx[reg - 0x42].alpha = v; break;
    case 0x44: g.dimx = v; break;
    case 0x45: g.dthe = v; break;
    case 0x46: g.colclamp = v; break;
    case 0x47: case 0x48: g.ctx[reg - 0x47].test = v; break;
    case 0x49: g.pabe = v; break;
    case 0x4A: case 0x4B: g.ctx[reg - 0x4A].fba = v; break;
    case 0x4C: case 0x4D: g.ctx[reg - 0x4C].frame = v; break;
    case 0x4E: case 0x4F: g.ctx[reg - 0x4E].zbuf = v; break;
    case 0x50: g.bitbltbuf = v; DecodeXfer(); break;
    case 0x51: g.trxpos = v; DecodeXfer(); break;
    case 0x52: g.trxreg = v; DecodeXfer(); break;
    case 0x53:
        g.trxDir = Bits(v, 0, 2);
        static const bool traceXfer = getenv("PORT_GS_TRACE_XFER");  // log every transfer
        if (traceXfer)
            port_log("GS transfer dir %d bitbltbuf %llx trxpos %llx trxreg %llx", g.trxDir,
                     (unsigned long long)g.bitbltbuf, (unsigned long long)g.trxpos, (unsigned long long)g.trxreg);
        StartTransfer();
        break;
    case 0x54: {
        // HWREG carries 64 bits at a time
        uint32_t bpp = BitsPerPixel(Bits(g.bitbltbuf, 56, 6));
        if (bpp == 32) {
            PutTransferPixel((uint32_t)v);
            PutTransferPixel((uint32_t)(v >> 32));
        } else {
            LogOnce(reg);
        }
        break;
    }
    case 0x60: break;  // SIGNAL
    case 0x61:
        // The game draws into the displayed buffer after its flip (the 2D
        // UI goes in after the FINISH), so a frame is complete when the next
        // FINISH comes, before DISPFB moves: that picture is frame seq, read
        // out only if the presenter wants it (a window, a screenshot).
        if (DisplayOk()) {
            uint64_t seq = g_frameSeq + 1;
            if (port_video_want(seq)) {
                PortFrame rgba;
                int w, h;
                if (gs_read_display(rgba, w, h))
                    port_video_frame(seq, std::move(rgba), w, h);
            }
            // a game frame (Flip) moves DISPFB; render-to-texture passes
            // and readbacks send FINISH too
            volatile uint64_t *priv = (volatile uint64_t *)PORT_HW(0x12000000);
            uint64_t d1 = priv[0x70 / 8], d2 = priv[0x90 / 8];
            if (d1 != g_lastDispfb[0] || d2 != g_lastDispfb[1])
                __atomic_add_fetch(&g_flips, 1, __ATOMIC_RELAXED);
            g_lastDispfb[0] = d1;
            g_lastDispfb[1] = d2;
            __atomic_store_n(&g_frameSeq, seq, __ATOMIC_RELEASE);
        }
        __atomic_add_fetch(&g_finishes, 1, __ATOMIC_RELAXED);
        // CSR.FINISH, atomically: the game thread and the vblank handler
        // write CSR too (port_gs_csr_ack)
        __atomic_or_fetch((uint64_t *)PORT_HW(0x12001000), 2, __ATOMIC_SEQ_CST);
        break;
    case 0x62: break;  // LABEL
    default: LogOnce(reg); break;
    }
}

// ---- GIF ------------------------------------------------------------------

struct GifState {
    uint32_t nloop;   // loops left in the current tag (0: expect a tag)
    uint32_t nreg, flg, reg;
    uint64_t regs;
    bool eop;
} g_gif[3];

// One qword of a GIF stream. Returns true at the end of a packet (EOP).
bool GifQword(GifState &st, const uint8_t *q)
{
    uint64_t lo, hi;
    memcpy(&lo, q, 8);
    memcpy(&hi, q + 8, 8);
    if (st.nloop == 0) {
        st.nloop = Bits(lo, 0, 15);
        st.eop = Bits(lo, 15, 1);
        st.flg = Bits(lo, 58, 2);
        st.nreg = Bits(lo, 60, 4);
        if (!st.nreg)
            st.nreg = 16;
        st.regs = hi;
        st.reg = 0;
        if (Bits(lo, 46, 1))
            WriteReg(0x00, Bits(lo, 47, 11));
        g.q = 1.0f;
        return st.nloop == 0 && st.eop;
    }
    if (st.flg == 0) {
        uint32_t r = (st.regs >> (st.reg * 4)) & 15;
        uint32_t w0 = (uint32_t)lo, w1 = (uint32_t)(lo >> 32), w2 = (uint32_t)hi, w3 = (uint32_t)(hi >> 32);
        switch (r) {
        case 0x0: WriteReg(0x00, lo); break;
        case 0x1:
            g.r = w0; g.g = w1; g.b = w2; g.a = w3;
            break;
        case 0x2:
            memcpy(&g.s, &w0, 4);
            memcpy(&g.t, &w1, 4);
            memcpy(&g.q, &w2, 4);
            break;
        case 0x3: g.u = w0 & 0x3FFF; g.v = w1 & 0x3FFF; break;
        case 0x4:
            AddVertex(w0 & 0xFFFF, w1 & 0xFFFF, (w2 >> 4) & 0xFFFFFF, (w3 >> 4) & 0xFF, !((w3 >> 15) & 1));
            break;
        case 0x5: AddVertex(w0 & 0xFFFF, w1 & 0xFFFF, w2, -1, !((w3 >> 15) & 1)); break;
        case 0xA: g.fog = (w3 >> 4) & 0xFF; break;
        case 0xC: AddVertex(w0 & 0xFFFF, w1 & 0xFFFF, (w2 >> 4) & 0xFFFFFF, (w3 >> 4) & 0xFF, false); break;
        case 0xD: AddVertex(w0 & 0xFFFF, w1 & 0xFFFF, w2, -1, false); break;
        case 0xE: WriteReg(hi & 0xFF, lo); break;
        case 0xF: break;
        default: WriteReg(r, lo); break;  // TEX0_1/2, CLAMP_1/2
        }
        if (++st.reg >= st.nreg) {
            st.reg = 0;
            st.nloop--;
        }
    } else if (st.flg == 1) {
        for (int half = 0; half < 2 && st.nloop; half++) {
            uint32_t r = (st.regs >> (st.reg * 4)) & 15;
            uint64_t v = half ? hi : lo;
            if (r != 0xE && r != 0xF)
                WriteReg(r, v);
            if (++st.reg >= st.nreg) {
                st.reg = 0;
                st.nloop--;
            }
        }
        // an odd count's last qword ends with a discarded half
    } else {
        ImageData(q);
        st.nloop--;
    }
    return st.nloop == 0 && st.eop;
}

void Init()
{
    if (__builtin_expect(g_nWorkers < 0, 0))
        StartWorkers();
}

} // namespace

// ---- the GS thread ------------------------------------------------------------
//
// The DMA thread queues GIF data (PATH2/PATH3 writes, PATH1 packets copied
// out of VU1 memory at XGKICK) and goes on; the GS thread parses it and
// feeds the drawing workers. gs_sync() waits for the queue to empty: before
// a readback (gs_read_image), and with PORT_DETERMINISTIC before the DMA
// controller ends a VIF1 or GIF transfer, so that the game sees FINISH and
// the frame count PORT_INPUT uses at the same point in every run.
// PORT_GS_SYNC=1 parses on the DMA thread instead.

namespace {

const uint64_t kQueueQw = 1 << 20;  // 16 MB
uint8_t *g_queue;
std::atomic<uint64_t> g_qHead, g_qTail;  // qwords written / consumed
std::atomic<uint32_t> g_qWake, g_qSleeping, g_qDoneWake, g_qDoneWaiters;
bool g_async;

enum : uint32_t { Q_WRITE, Q_KICK, Q_WRAP };

void ProcessEntry(uint32_t type, uint32_t path, const uint8_t *q, size_t n)
{
    std::lock_guard lock(g_mutex);
    Init();
    if (type == Q_WRITE) {
        for (size_t i = 0; i < n; i++)
            GifQword(g_gif[path], q + i * 16);
    } else {
        GifState &st = g_gif[GS_PATH1];
        st = {};
        size_t i = 0;
        for (; i < n; i++)
            if (GifQword(st, q + i * 16))
                break;
        if (i == n)
            port_log("GS: PATH1 packet has no EOP");
    }
    Publish();
}

void GsThread()
{
    port_thread_name("gs");
    uint64_t tail = g_qTail.load();
    for (;;) {
        uint64_t head = g_qHead.load(std::memory_order_acquire);
        if (head == tail) {
            // spin ~50 us before sleeping: the DMA thread's futex wake
            // is a syscall per packet when this thread sleeps between them
            head = SpinWait(g_qHead, tail, 50000);
            if (head == tail) {
                uint32_t w = g_qWake.load();
                g_qSleeping.store(1);
                if (g_qHead.load() == tail)
                    Futex(g_qWake, FUTEX_WAIT_PRIVATE, w, 5000000);
                g_qSleeping.store(0);
                continue;
            }
        }
        while (tail != head) {
            const uint8_t *e = g_queue + (tail % kQueueQw) * 16;
            uint32_t type, path;
            uint64_t n;
            memcpy(&type, e, 4);
            memcpy(&path, e + 4, 4);
            memcpy(&n, e + 8, 8);
            if (type == Q_WRAP) {
                tail += kQueueQw - tail % kQueueQw;
                continue;
            }
            ProcessEntry(type, path, e + 16, n);
            tail += 1 + n;
            g_qTail.store(tail, std::memory_order_release);
            std::atomic_thread_fence(std::memory_order_seq_cst);  // as in Publish
            if (g_qDoneWaiters.load()) {
                g_qDoneWake.fetch_add(1);
                Futex(g_qDoneWake, FUTEX_WAKE_PRIVATE, INT32_MAX, 0);
            }
        }
        g_qTail.store(tail, std::memory_order_release);
    }
}

void StartQueue()
{
    static bool started;
    if (started)
        return;
    started = true;
    g_async = !getenv("PORT_GS_SYNC");
    if (!g_async)
        return;
    g_queue = BigArray<uint8_t>(kQueueQw * 16);
    port_watch_futex(&g_qWake, "gs.queueWake");
    port_watch_futex(&g_qDoneWake, "gs.queueDone");
    std::thread(GsThread).detach();
}

// Room for 1 + n qwords at the head (a wrap entry first if they don't fit
// before the end): returns where the entry goes.
uint8_t *Reserve(uint64_t n)
{
    uint64_t need = 1 + n;
    uint64_t head = g_qHead.load(std::memory_order_relaxed);
    uint64_t room = kQueueQw - head % kQueueQw;
    if (room < need) {
        // wrap
        while (head + room - g_qTail.load(std::memory_order_acquire) > kQueueQw)
            std::this_thread::yield();
        uint8_t *e = g_queue + (head % kQueueQw) * 16;
        uint32_t t = Q_WRAP;
        memcpy(e, &t, 4);
        head += room;
        g_qHead.store(head, std::memory_order_release);
    }
    while (head + need - g_qTail.load(std::memory_order_acquire) > kQueueQw)
        std::this_thread::yield();
    return g_queue + (head % kQueueQw) * 16;
}

void Commit(uint32_t type, uint32_t path, uint8_t *e, uint64_t n)
{
    memcpy(e, &type, 4);
    memcpy(e + 4, &path, 4);
    memcpy(e + 8, &n, 8);
    g_qHead.store(g_qHead.load(std::memory_order_relaxed) + 1 + n, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_seq_cst);  // as in Publish
    if (g_qSleeping.load()) {
        g_qWake.fetch_add(1);
        Futex(g_qWake, FUTEX_WAKE_PRIVATE, 1, 0);
    }
}

// A PATH1 packet's length in qwords: its tags up to EOP (at most 1024).
uint32_t PacketQwords(const uint8_t *vumem, uint32_t qaddr)
{
    uint32_t i = 0;
    while (i < 1024) {
        uint64_t lo;
        memcpy(&lo, vumem + ((qaddr + i) & 1023) * 16, 8);
        uint32_t nloop = Bits(lo, 0, 15), eop = Bits(lo, 15, 1), flg = Bits(lo, 58, 2), nreg = Bits(lo, 60, 4);
        if (!nreg)
            nreg = 16;
        i += 1 + (flg == 0 ? nloop * nreg : flg == 1 ? (nloop * nreg + 1) / 2 : nloop);
        if (eop)
            break;
    }
    return std::min(i, 1024u);
}

} // namespace

void gs_gif_write(GsPath path, const void *qwords, size_t n)
{
    StartQueue();
    if (!n)
        return;
    if (!g_async) {
        std::lock_guard lock(g_mutex);
        Init();
        ProcessEntry(Q_WRITE, path, (const uint8_t *)qwords, n);
        return;
    }
    // big writes in pieces (a queue entry is at most a quarter of the queue)
    const uint8_t *q = (const uint8_t *)qwords;
    while (n) {
        uint64_t m = std::min<uint64_t>(n, kQueueQw / 4);
        uint8_t *e = Reserve(m);
        memcpy(e + 16, q, m * 16);
        Commit(Q_WRITE, path, e, m);
        q += m * 16;
        n -= m;
    }
}

void gs_gif_kick(const uint8_t *vumem, uint32_t qaddr)
{
    StartQueue();
    uint32_t n = PacketQwords(vumem, qaddr);
    if (!g_async) {
        std::lock_guard lock(g_mutex);
        Init();
        uint8_t buf[1024 * 16];
        for (uint32_t i = 0; i < n; i++)
            memcpy(buf + i * 16, vumem + ((qaddr + i) & 1023) * 16, 16);
        ProcessEntry(Q_KICK, 0, buf, n);
        return;
    }
    uint8_t *e = Reserve(n);
    uint32_t first = std::min(n, 1024 - (qaddr & 1023));
    memcpy(e + 16, vumem + (qaddr & 1023) * 16, first * 16);
    memcpy(e + 16 + first * 16, vumem, (n - first) * 16);
    Commit(Q_KICK, 0, e, n);
}

void gs_sync()
{
    if (!g_async)
        return;
    uint64_t head = g_qHead.load(std::memory_order_acquire);
    while (g_qTail.load(std::memory_order_acquire) != head) {
        uint32_t w = g_qDoneWake.load();
        g_qDoneWaiters.fetch_add(1);
        if (g_qTail.load() != head)
            Futex(g_qDoneWake, FUTEX_WAIT_PRIVATE, w, 2000000);
        g_qDoneWaiters.fetch_sub(1);
    }
}

void gs_read_image(void *dst, size_t n)
{
    gs_sync();
    std::lock_guard lock(g_mutex);
    Init();
    Barrier(W_READBACK);
    uint32_t sbp = Bits(g.bitbltbuf, 0, 14), sbw = Bits(g.bitbltbuf, 16, 6), spsm = Bits(g.bitbltbuf, 24, 6);
    uint32_t sx = Bits(g.trxpos, 0, 11), sy = Bits(g.trxpos, 16, 11);
    uint32_t w = Bits(g.trxreg, 0, 12);
    uint32_t bpp = BitsPerPixel(spsm);
    uint8_t *out = (uint8_t *)dst;
    size_t bytes = n * 16;
    memset(out, 0, bytes);
    uint32_t x = 0, y = 0;
    for (size_t off = 0; off < bytes;) {
        uint32_t v = ReadPixel(spsm, sbp, sbw, (sx + x) & 2047, (sy + y) & 2047);
        if (bpp == 32) { memcpy(out + off, &v, 4); off += 4; }
        else if (bpp == 16) { uint16_t h = v; memcpy(out + off, &h, 2); off += 2; }
        else if (bpp == 8) { out[off++] = v; }
        else { port_log("GS: local->host for %u bpp", bpp); return; }
        if (++x >= w) {
            x = 0;
            y++;
        }
    }
}

uint64_t gs_flip_count()
{
    return __atomic_load_n(&g_flips, __ATOMIC_RELAXED);
}

void gs_lock()
{
    g_mutex.lock();
}

void gs_unlock()
{
    g_mutex.unlock();
}

uint64_t gs_frame_seq()
{
    return __atomic_load_n(&g_frameSeq, __ATOMIC_ACQUIRE);
}

uint64_t gs_finish_count()
{
    return __atomic_load_n(&g_finishes, __ATOMIC_RELAXED);
}

uint64_t gs_prim_count()
{
    return __atomic_load_n(&g_prims, __ATOMIC_RELAXED);
}

uint64_t gs_barrier_count()
{
    return __atomic_load_n(&g_barriers, __ATOMIC_RELAXED);
}

namespace {
// Would gs_read_display give a picture?
bool DisplayOk()
{
    volatile uint64_t *priv = (volatile uint64_t *)PORT_HW(0x12000000);
    uint64_t pmode = priv[0];
    for (int n = 1; n <= 2; n++) {
        if (!(pmode & n))
            continue;
        uint64_t display = priv[n == 1 ? 0x80 / 8 : 0xA0 / 8];
        uint32_t magh = Bits(display, 23, 4) + 1;
        int cw = (int)((Bits(display, 32, 12) + magh) / magh), ch = (int)Bits(display, 44, 11) + 1;
        if (cw > 0 && ch > 0 && cw <= 2048 && ch <= 2048)
            return true;
    }
    return false;
}
} // namespace

bool gs_read_display(PortFrame &rgba, int &w, int &h)
{
    volatile uint64_t *priv = (volatile uint64_t *)PORT_HW(0x12000000);
    uint64_t pmode = priv[0];
    bool en1 = pmode & 1, en2 = pmode & 2;
    if (!en1 && !en2)
        return false;
    // circuit 2 underneath, circuit 1 blended over it at ALP (MMOD 1)
    auto circuit = [&](int n, PortFrame &out, int &cw, int &ch) {
        uint64_t dispfb = priv[n == 1 ? 0x70 / 8 : 0x90 / 8], display = priv[n == 1 ? 0x80 / 8 : 0xA0 / 8];
        uint32_t fbp = Bits(dispfb, 0, 9) * 32, fbw = Bits(dispfb, 9, 6), psm = Bits(dispfb, 15, 5);
        uint32_t dbx = Bits(dispfb, 32, 11), dby = Bits(dispfb, 43, 11);
        uint32_t magh = Bits(display, 23, 4) + 1;
        // the game writes DW as (width - 1) * MAGH: round up
        cw = (int)((Bits(display, 32, 12) + magh) / magh);
        ch = (int)Bits(display, 44, 11) + 1;
        if (cw <= 0 || ch <= 0 || cw > 2048 || ch > 2048)
            return false;
        out.resize(cw * ch);
        for (int y = 0; y < ch; y++) {
            uint32_t *row = &out[y * cw];
            uint32_t yy = (dby + y) & 2047;
            if (psm == CT32 || psm == CT24) {
                for (int x = 0; x < cw; x++)
                    row[x] = Word(Addr32(fbp, fbw, (dbx + x) & 2047, yy)) | 0xFF000000;
                continue;
            }
            for (int x = 0; x < cw; x++) {
                uint32_t v = ReadPixel(psm, fbp, fbw, (dbx + x) & 2047, yy);
                if (psm == CT16 || psm == CT16S)
                    v = ((v & 31) << 3) | (((v >> 5) & 31) << 11) | (((v >> 10) & 31) << 19);
                row[x] = v | 0xFF000000;
            }
        }
        return true;
    };
    std::lock_guard lock(g_mutex);
    Init();
    Barrier(W_DISPLAY);
    static PortFrame c1;
    int w1 = 0, h1 = 0;
    bool ok2 = en2 && circuit(2, rgba, w, h);
    bool ok1 = en1 && circuit(1, c1, w1, h1);
    if (!ok2) {
        if (!ok1)
            return false;
        rgba.swap(c1);
        w = w1;
        h = h1;
        return true;
    }
    if (ok1 && Bits(pmode, 5, 1)) {
        uint32_t alp = Bits(pmode, 8, 8);
        for (int y = 0; y < std::min(h, h1); y++)
            for (int x = 0; x < std::min(w, w1); x++) {
                uint32_t a = c1[y * w1 + x], b = rgba[y * w + x], o = 0xFF000000;
                for (int sh = 0; sh < 24; sh += 8)
                    o |= ((((a >> sh) & 255) * alp + ((b >> sh) & 255) * (255 - alp)) / 255) << sh;
                rgba[y * w + x] = o;
            }
    }
    return true;
}

// Debugging: a texture as RGBA (A doubled to 0-255), as TEX0 describes it.
void gs_read_texture(uint64_t tex0, std::vector<uint32_t> &rgba, int &w, int &h)
{
    gs_sync();
    std::lock_guard lock(g_mutex);
    Init();
    Barrier(W_DEBUG);
    w = 1 << std::min<int>(Bits(tex0, 26, 4), 10);
    h = 1 << std::min<int>(Bits(tex0, 30, 4), 10);
    uint16_t clut[512];
    memcpy(clut, g.clut, sizeof clut);
    uint64_t t = tex0;
    if (Bits(t, 61, 3) == 0)
        t |= 1ull << 61;
    LoadClut(t);
    DrawState S = {};
    S.texa = g.texa;
    S.clut = g.clut;
    S.tbp = Bits(tex0, 0, 14);
    S.tbw = Bits(tex0, 14, 6);
    S.tpsm = Bits(tex0, 20, 6);
    S.cpsm = Bits(tex0, 51, 4);
    S.csa = Bits(tex0, 56, 5);
    rgba.resize(w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint32_t c = FetchTexel(S, x, y);
            uint32_t a = std::min<uint32_t>((c >> 24) * 2, 255);
            rgba[y * w + x] = (c & 0xFFFFFF) | a << 24;
        }
    memcpy(g.clut, clut, sizeof clut);
    g.clutVersion++;
}

void gs_debug(FILE *f)
{
    fprintf(f, "GS: queue %s head %llu tail %llu (GS thread sleeping %u, sync waiters %u); primitives queued %llu, "
               "published %llu, parser in Barrier %d; workers %d sleeping %u:",
            g_async ? "async" : "sync", (unsigned long long)g_qHead.load(), (unsigned long long)g_qTail.load(),
            g_qSleeping.load(), g_qDoneWaiters.load(), (unsigned long long)__atomic_load_n(&g_next, __ATOMIC_RELAXED),
            (unsigned long long)g_published.load(), __atomic_load_n(&g_inBarrier, __ATOMIC_RELAXED), g_nWorkers,
            g_sleepers.load());
    for (int k = 0; k < g_nWorkers; k++)
        fprintf(f, " %llu", (unsigned long long)g_workers[k].done.load());
    fprintf(f, "\n");
}

// GS CSR's event bits (SIGNAL 1, FINISH 2, HSINT 4, VSINT 8, EDWINT 16) are
// write-1-to-clear. The game's writes are plain stores to the PC's register
// memory, so the vblank handler's acknowledgement (`CSR = 8`, on the timer
// thread) would clear a FINISH set by the GS thread while Flip spins on it;
// it comes here instead. (The game's own `CSR |= 2` / `CSR = 2` before a
// FINISH wait set the bit on the PC, so those waits don't wait: the
// readbacks they guard sync with the GS anyway, gs_read_image.)
extern "C" void port_gs_csr_ack(unsigned int v)
{
    __atomic_and_fetch((uint64_t *)PORT_HW(0x12001000), ~(uint64_t)(v & 0x1F), __ATOMIC_SEQ_CST);
}
