// PSXMpegVideo.cpp  @ 0x002CD830 (text 0x2CD830-0x2D0160)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp)
//
// Retail's PSXMpegVideo.cpp as one translation unit: a TU without a static
// initializer, the last of PSXWater.cpp's tu_map row, after
// PSXMediaControl.cpp: the movie player's stream file, input buffer, video
// output buffer, video decoder and audio decoder (the ASSERTs say
// "PSXMpegVideo.cpp"; its .rodata labels run @580..@1218 after
// PSXMediaControl's RTTI and the PSXMpegDefs constants). `make link` places
// its data as well as its code.


#include "PSXMpegStreamFile/PSXMpegStreamFile.h"
#include "PSXMpegInputBuffer/PSXMpegInputBuffer.h"
#include "PSXMpegAudioDecoder/PSXMpegAudioDecoder.h"
#include "soultree_assert_block.h"
#include "DmaPacket/DmaPacket.h"
#include "PSXMpegVideoOutputBuffer/PSXMpegVideoOutputBuffer.h"
#include "PSXVideoCard/PSXVideoCard.h"
#include "PSXMpegVideoDecoder/PSXMpegVideoDecoder.h"
#include "PSXMpegVideoDecoder/PSXMpegDefs.h"
#include "PSXMpegVideoDecoder/libmpeg.h"
#include "PSXMediaControl/PSXMediaControl.h"
#include "FILE_HANDLE_STRUCT/FILE_HANDLE_STRUCT.h"
#include "RainbowPS2Snd/RainbowPS2Snd.h"
#include "PSXSoundInterface/PSXSoundInterface.h"
#include "AuralScape/AuralScape.h"
#include "Game/Game.h"
#include "mwcc_shim.h"

#pragma fast_fptosi on

extern "C" char *strcpy(char *dst, const char *src);
extern "C" char *strcat(char *dst, const char *src);
extern "C" void *memalign(unsigned int align, unsigned int size);
extern "C" void free(void *p);
extern "C" double log(double x);
extern "C" double ldexp(double x, int exp);

struct sceCdRMode {
    unsigned char trycount;
    unsigned char spindlctrl;
    unsigned char datapattern;
    unsigned char pad;
};

extern "C" int sceCdStatus(void);
extern "C" void *sceSifAllocIopHeap(unsigned int size);
extern "C" int sceSifFreeIopHeap(void *p);
extern "C" int sceCdStInit(unsigned int bufmax, unsigned int bankmax, unsigned int iop_bufaddr);
extern "C" int sceCdStStart(unsigned int lbn, sceCdRMode *mode);
extern "C" int sceCdStStop(void);
extern "C" int sceCdStSeek(unsigned int lbn);
extern "C" int sceCdStRead(unsigned int sectors, unsigned int *buf, unsigned int mode, unsigned int *err);

struct sceSifDmaData {
    unsigned int data;
    unsigned int addr;
    unsigned int size;
    unsigned int mode;
};
extern "C" void FlushCache(int mode);
extern "C" unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);
extern "C" int sceSifDmaStat(unsigned int id);

FILE_HANDLE_STRUCT *FOPEN(char *name, const char *mode);
int FCLOSE(FILE_HANDLE_STRUCT *f);

extern PSXVideoCard *LpVideoCard;
extern Game *LpGame;
extern int TotalNumberOfAudioSeconds;
extern int TimerOverflowCounterT0;

typedef unsigned long u_long;
typedef int u_long128 __attribute__((mode(TI)));

// libgraph's GS register packing.
#define SCE_GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
    ((u_long)(sbp) | ((u_long)(sbw) << 16) | ((u_long)(spsm) << 24) | \
     ((u_long)(dbp) << 32) | ((u_long)(dbw) << 48) | ((u_long)(dpsm) << 56))
#define SCE_GS_SET_TRXPOS(ssax, ssay, dsax, dsay, dir) \
    ((u_long)(ssax) | ((u_long)(ssay) << 16) | ((u_long)(dsax) << 32) | \
     ((u_long)(dsay) << 48) | ((u_long)(dir) << 59))
#define SCE_GS_SET_TRXREG(rrw, rrh) ((u_long)(rrw) | ((u_long)(rrh) << 32))
#define SCE_GS_SET_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
    ((u_long)(ate) | ((u_long)(atst) << 1) | ((u_long)(aref) << 4) | ((u_long)(afail) << 12) | \
     ((u_long)(date) << 14) | ((u_long)(datm) << 15) | ((u_long)(zte) << 16) | ((u_long)(ztst) << 17))
#define SCE_GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
    ((u_long)(prim) | ((u_long)(iip) << 3) | ((u_long)(tme) << 4) | ((u_long)(fge) << 5) | \
     ((u_long)(abe) << 6) | ((u_long)(aa1) << 7) | ((u_long)(fst) << 8) | ((u_long)(ctxt) << 9) | \
     ((u_long)(fix) << 10))
#define SCE_GS_SET_ALPHA(a, b, c, d, fix) \
    ((u_long)(a) | ((u_long)(b) << 2) | ((u_long)(c) << 4) | ((u_long)(d) << 6) | ((u_long)(fix) << 32))
#define SCE_GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    ((u_long)(tbp) | ((u_long)(tbw) << 14) | ((u_long)(psm) << 20) | ((u_long)(tw) << 26) | \
     ((u_long)(th) << 30) | ((u_long)(tcc) << 34) | ((u_long)(tfx) << 35) | ((u_long)(cbp) << 37) | \
     ((u_long)(cpsm) << 51) | ((u_long)(csm) << 55) | ((u_long)(csa) << 56) | ((u_long)(cld) << 61))
#define SCE_GS_SET_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
    ((u_long)(lcm) | ((u_long)(mxl) << 2) | ((u_long)(mmag) << 5) | ((u_long)(mmin) << 6) | \
     ((u_long)(mtba) << 9) | ((u_long)(l) << 19) | ((u_long)(k) << 32))
#define SCE_GS_SET_RGBAQ(r, g, b, a, q) \
    ((u_long)(r) | ((u_long)(g) << 8) | ((u_long)(b) << 16) | ((u_long)(a) << 24) | ((u_long)(q) << 32))
#define SCE_GS_SET_UV(u, v) ((u_long)(u) | ((u_long)(v) << 16))
#define SCE_GS_SET_XYZ(x, y, z) ((u_long)(x) | ((u_long)(y) << 16) | ((u_long)(z) << 32))
#define SCE_GS_PRIM 0x00
#define SCE_GS_RGBAQ 0x01
#define SCE_GS_UV 0x03
#define SCE_GS_XYZ2 0x05
#define SCE_GS_TEX0_1 0x06
#define SCE_GS_TEX1_1 0x14
#define SCE_GS_TEXFLUSH 0x3F
#define SCE_GS_ALPHA_1 0x42
#define SCE_GS_TEST_1 0x47
#define SCE_GS_BITBLTBUF 0x50
#define SCE_GS_TRXPOS 0x51
#define SCE_GS_TRXREG 0x52
#define SCE_GS_TRXDIR 0x53
#define SCE_GS_FINISH 0x3F

// EE uncached segment address.
#define UNCACHED(p) ((void *)PS2_ADDR(((unsigned int)(p) & 0x0FFFFFFF) | EE_UNCACHED))

#define INPUT_BUFFER_SIZE 0x60000

// toIPU DMA channel (channel 4).
#define D4_CHCR (*(volatile unsigned int *)0x1000B400)
#define D4_MADR (*(volatile unsigned int *)0x1000B410)
#define D4_QWC  (*(volatile unsigned int *)0x1000B420)

// Timer 0 counts scanlines (525 per NTSC frame).
#define T0_COUNT ((volatile unsigned int *)0x10000000)

#define IPU_CTRL ((volatile int *)0x10002010)

#define STMBLK 1

#define LN2 0.69314718055994529

// Win32-style timeGetTime: EE timer 0 (T0_COUNT at 0x10000000) ticks plus the
// overflows counted by TimerHandlerT0, accumulated and scaled to milliseconds.
inline unsigned int timeGetTime()
{
    static long Time = 0;
    static int lastCount = 0;
    int count;
    int elapsed;
    int overflows;
    count = *(volatile int *)0x10000000;
    overflows = TimerOverflowCounterT0;
    TimerOverflowCounterT0 = 0;
    if (lastCount <= count)
        elapsed = count - lastCount;
    else
        elapsed = 0x10000 - lastCount + count;
    if (lastCount >= count && overflows > 0)
        overflows--;
    lastCount = count;
    Time += elapsed + (overflows << 16);
    return 0.0017066667f * Time;
}

#define SND (LpGame->m_auralScape->m_soundInterface->m_snd)

// ---- src/PSXMpegStreamFile/ctor.cpp

PSXMpegStreamFile::PSXMpegStreamFile()
{
    m_name[0] = 0;
    m_x104 = 0;
    m_iopBuffer = 0;
    m_file = -1;
}


// ---- src/PSXMpegStreamFile/Open.cpp
int PSXMpegStreamFile::Open(char *name)
{
    strcpy(m_name, name);
    PSXprintf("Attempting to open mpeg %s... ", name);
    FILE_HANDLE_STRUCT *f = FOPEN(name, "r");
    if (!f) {
        PSXprintf("file does not exist.\n");
        FCLOSE(f);
        return 0;
    }
    PSXprintf("file found.\n");
    m_cdFile = f->file;
    m_x104 = f->size;
    FCLOSE(f);
    if (sceCdStatus() == 1)
        return 0;
    m_iopBuffer = sceSifAllocIopHeap(0x40010);
    if (!sceCdStInit(0x80, 8, ((unsigned int)m_iopBuffer + 15) & ~15)) {
        PSXprintf("Could not initialize cd/dvd streaming\n");
        sceCdStStop();
        sceSifFreeIopHeap(m_iopBuffer);
        return 0;
    }
    sceCdRMode mode;
    mode.trycount = 0;
    mode.spindlctrl = 0;
    mode.datapattern = 0;
    if (!sceCdStStart(m_cdFile.lsn, &mode)) {
        PSXprintf("cd streaming start failed\n");
        sceCdStStop();
        sceSifFreeIopHeap(m_iopBuffer);
        return 0;
    }
    return 1;
}


// ---- src/PSXMpegStreamFile/Reset.cpp

int PSXMpegStreamFile::Reset()
{
    if (sceCdStSeek(m_cdFile.lsn) < 0) {
        PSXprintf("cd streaming seek failed\n");
        sceCdStStop();
        sceSifFreeIopHeap(m_iopBuffer);
        return 0;
    }
    return 1;
}

// ---- src/PSXMpegStreamFile/Close.cpp

int PSXMpegStreamFile::Close()
{
    sceCdStStop();
    sceSifFreeIopHeap(m_iopBuffer);
    return 1;
}

// ---- src/PSXMpegStreamFile/Read.cpp
// Read whole 2048-byte sectors from the CD stream; returns bytes read.
int PSXMpegStreamFile::Read(void *buffer, int size)
{
    unsigned int error;
    int sectors = sceCdStRead(size >> 11, (unsigned int *)buffer, STMBLK, &error);
    switch (error) {
    case -1:
        PSXprintf("Cd read failed.\n");
        return -1;
    case 0x32:
        PSXprintf("Cd End Of Media (outermost track reached).\n");
        return -1;
    case 0x31:
        PSXprintf("Cd drive opened during playback.\n");
        return -1;
    case 0x30:
        PSXprintf("Cd problem occured during read.\n");
        return -1;
    case 0x14:
        PSXprintf("Cd, not appropriate for disk in drive.\n");
        return -1;
    case 0x13:
        PSXprintf("Cd not ready.\n");
        return -1;
    case 1:
        PSXprintf("Cd abort received.\n");
        return -1;
    }
    return sectors << 11;
}


// ---- src/PSXMpegInputBuffer/ctor.cpp
PSXMpegInputBuffer::PSXMpegInputBuffer(PSXMpegStreamFile *file, PSXMpegVideoDecoder *video, PSXMpegAudioDecoder *audio)
{
    m_readBuffer = (unsigned char *)memalign(64, 0x4000);
    m_buffer = (char *)memalign(64, 0x60000);
    m_writePtr = m_buffer + 16;
    m_readPtr = m_buffer;
    m_10 = m_buffer + 16;
    m_bytesLeft = 0;
    m_file = file;
    m_video = video;
    m_audioDecoder = audio;
}


// ---- src/PSXMpegInputBuffer/dtor.cpp
PSXMpegInputBuffer::~PSXMpegInputBuffer()
{
    if (m_readBuffer)
        free(m_readBuffer);
    if (m_buffer)
        free(m_buffer);
}


// ---- src/PSXMpegInputBuffer/Initialize.cpp

void PSXMpegInputBuffer::Initialize()
{
    m_writePtr = m_buffer + 0x10;
    m_readPtr = m_buffer;
    m_10 = m_buffer + 0x10;
    m_bytesLeft = 0;
}


// ---- src/PSXMpegInputBuffer/Reset.cpp

void PSXMpegInputBuffer::Reset()
{
    m_writePtr = m_buffer + 0x10;
    m_readPtr = m_buffer;
    m_10 = m_buffer + 0x10;
    m_bytesLeft = 0;
}


// ---- src/PSXMpegInputBuffer/DemuxVideoData.cpp
// Copy a video packet into the ring buffer; 0 if it doesn't fit.
int PSXMpegInputBuffer::DemuxVideoData(sceMpeg *mpeg, sceMpegCbDataStr *cb, void *data)
{
    int space = m_readPtr - m_writePtr;
    if (space < 0)
        space += INPUT_BUFFER_SIZE;
    if (space == 0 && m_readPtr == m_10)
        space = INPUT_BUFFER_SIZE;
    if (space < cb->len)
        return 0;
    int overflow = cb->len + (m_writePtr - m_buffer) - INPUT_BUFFER_SIZE;
    if (overflow > 0) {
        memcpy(UNCACHED(m_writePtr), cb->data, cb->len - overflow);
        memcpy(UNCACHED(m_buffer), cb->data + (cb->len - overflow), overflow);
        m_writePtr = m_buffer + overflow;
    } else {
        memcpy(UNCACHED(m_writePtr), cb->data, cb->len);
        m_writePtr += cb->len;
    }
    return 1;
}


// ---- src/PSXMpegInputBuffer/DemuxAudioData.cpp
int PSXMpegInputBuffer::DemuxAudioData(sceMpeg *mpeg, sceMpegCbDataStr *cb, void *data)
{
    unsigned char *ptr0 = 0;
    unsigned char *ptr1 = 0;
    int len0 = 0;
    int len1 = 0;
    m_audioDecoder->BeginRingBufferPut(&ptr0, &len0, &ptr1, &len1);
    // Skip the 4-byte audio packet header.
    cb->len -= 4;
    cb->data += 4;
    if (len0 + len1 < cb->len)
        return 0;
    int overflow = cb->len - len0;
    if (overflow > 0) {
        memcpy(ptr0, cb->data, cb->len - overflow);
        memcpy(ptr1, cb->data + (cb->len - overflow), overflow);
    } else {
        memcpy(ptr0, cb->data, cb->len);
    }
    m_audioDecoder->EndRingBufferPut(cb->len);
    return 1;
}


// ---- src/PSXMpegInputBuffer/NoDataAvailable.cpp
// The decoder ran dry: make sure some data is buffered, then DMA up to 4K to the IPU.
int PSXMpegInputBuffer::NoDataAvailable(sceMpeg *mpeg, sceMpegCbData *, void *)
{
    m_readPtr = m_10;
    int available = m_writePtr - m_10;
    if (available < 0)
        available += INPUT_BUFFER_SIZE;
    while (available < 0x1000) {
        int result = FillInputBuffer(mpeg, 1);
        if (result == 0) {
            available += 15;    // end of stream: send the rest, rounded up
            break;
        }
        if (result == -1)
            return -1;
        available = m_writePtr - m_10;
        if (available < 0)
            available += INPUT_BUFFER_SIZE;
    }
    int size = available & ~15;
    if (size > 0x1000)
        size = 0x1000;
    if (m_readPtr + size > m_buffer + INPUT_BUFFER_SIZE)
        size = m_buffer + INPUT_BUFFER_SIZE - m_readPtr;
    D4_QWC = size / 16;
    D4_MADR = (unsigned int)m_readPtr;
    D4_CHCR = 0x101;
    m_10 = m_readPtr + size;
    if (m_10 >= m_buffer + INPUT_BUFFER_SIZE)
        m_10 -= INPUT_BUFFER_SIZE;
    return 1;
}


// ---- src/PSXMpegInputBuffer/FillInputBuffer.cpp
int PSXMpegInputBuffer::FillInputBuffer(sceMpeg *mp, int wait)
{
    unsigned char *data;
    if (m_bytesLeft) {
        data = m_readBuffer + 0x4000 - m_bytesLeft;
    } else {
        do {
            m_bytesLeft = m_file->Read(m_readBuffer, 0x4000);
            if (m_bytesLeft == 0 && !wait)
                return 0;
            if (m_bytesLeft == -1) {
                PSXprintf("problem with Cd! aborting movie\n");
                return -1;
            }
        } while (m_bytesLeft == 0);
        data = m_readBuffer;
    }
    int used = sceMpegDemuxPss(mp, data, m_bytesLeft);
    m_bytesLeft -= used;
    return used;
}


// ---- src/PSXMpegInputBuffer/BackgroundProcessing.cpp
int PSXMpegInputBuffer::BackgroundProcessing(sceMpeg *mp, sceMpegCbData *, void *)
{
#ifdef SPLASHDOWN_PC
    // The PS2 fills the rings for as long as the timer allows while the IPU
    // decodes; the PC's libmpeg (ports/pc/sdk/mpeg.cpp) calls this once a
    // picture: fill them until nothing more fits (the timer may not move:
    // PORT_DETERMINISTIC).
    while (FillInputBuffer(mp, 0) > 0)
        ;
#else
    while (*T0_COUNT % 525 < 480)
        FillInputBuffer(mp, 0);
#endif
    return 1;
}


// ---- src/PSXMpegInputBuffer/PreBuffer.cpp

int PSXMpegInputBuffer::PreBuffer()
{
    Reset();
    if (FillInputBuffer(m_video->m_mpeg, 1) == -1)
        return 0;
    return 1;
}

// ---- src/PSXMpegVideoOutputBuffer/ctor.cpp
PSXMpegVideoOutputBuffer::PSXMpegVideoOutputBuffer(int width, int height)
{
    m_data = new PSXMpegVideoOutputBufferData[2];
    m_width = width < 640 ? width : (width = 640);
    m_height = height < 448 ? height : (height = 448);
    for (int i = 0; i < 2; i++) {
        m_data[i].m_pixels = (sceIpuRGB32 *)memalign(0x40, m_width * m_height * 4);
#line 611 "PSXMpegVideo.cpp"
        ASSERT(m_data[i].m_pixels, "PSXMpegVideoOutputBuffer: memalign failed");
        m_data[i].m_texturePacket = new DmaPacket(500, 1);
        m_data[i].m_bitbltbuf = 0;
        m_data[i].m_spritePacket = new DmaPacket(100, 1);
    }
    m_readPos = 0;
    m_writePos = 0;
    m_count = 0;
    m_readIndex = 0;
    m_writeIndex = 0;
}


// ---- src/PSXMpegVideoOutputBuffer/InitializeTextureDmaPacket.cpp
// Builds the packet that uploads a decoded frame to the Z buffer area, one
// 16-pixel-wide column of macroblocks at a time.
void PSXMpegVideoOutputBuffer::InitializeTextureDmaPacket(PSXMpegVideoOutputBufferData *data, int width,
                                                          int height, int format)
{
    unsigned int zbp;
    unsigned int bufferWidth;
    unsigned int qwc;
    int columns;
    unsigned char *image;
    int x;
    int stride;
    zbp = LpVideoCard->GetZBufferAddress() << 5;
    image = (unsigned char *)data->m_pixels;
    columns = width >> 4;
    bufferWidth = LpVideoCard->m_width >> 6;
    qwc = height * 16 * 4 / sizeof(u_long128);

    data->m_texturePacket->ResetPacket();
    data->m_texturePacket->AddSourceDmaTag();
    data->m_texturePacket->AddGifTag(0);
    data->m_bitbltbuf = data->m_texturePacket->AddGifData(SCE_GS_BITBLTBUF,
                                                   SCE_GS_SET_BITBLTBUF(0, 0, 0, zbp, bufferWidth, 0));
    data->m_texturePacket->AddGifData(SCE_GS_TRXREG, SCE_GS_SET_TRXREG(16, (unsigned int)height));
    if (!format)
        format = height;
    stride = (format >> 4) << 10;
    for (x = 0; x < columns << 4; x += 16, image += stride) {
        data->m_texturePacket->AddSourceDmaTag();
        data->m_texturePacket->AddGifTag(0);
        data->m_texturePacket->AddGifData(SCE_GS_TRXPOS, SCE_GS_SET_TRXPOS(0, 0, (unsigned int)x, 0, 0));
        data->m_texturePacket->AddGifData(SCE_GS_TRXDIR, 0);
        data->m_texturePacket->AddGifTextureTag(qwc);
        data->m_texturePacket->AddSourceDmaTextureTag(image, qwc);
    }
    data->m_texturePacket->AddSourceDmaTag();
    data->m_texturePacket->AddGifTag(1);
    data->m_texturePacket->AddGifData(SCE_GS_FINISH, 0);
    data->m_texturePacket->AddDmaEndTag();
}


// ---- src/PSXMpegVideoOutputBuffer/InitializeSpriteDmaPacket.cpp

// Compiler session state: retail compiled
// this file in an mwcc session that had already generated a two-argument runtime
// helper call (64-bit / or %, double arithmetic or compare). mwcc then models
// every later helper call (__floatdisf, fptoui, fptodp, ...) as reading $a0-$a1,
// so temporaries live before such a call skip $a0/$a1. This unused function
// recreates that state; it is not part of the retail file.
static void MWCC_SHIM_TEXT MwccSessionState(long *a) { a[0] = a[0] / a[1]; }

// Builds the packet that draws a decoded frame (uploaded to the Z buffer
// area) as one textured sprite.
void PSXMpegVideoOutputBuffer::InitializeSpriteDmaPacket(PSXMpegVideoOutputBufferData *data, int x, int y,
                                                         int width, int height)
{
    int zbp = LpVideoCard->GetZBufferAddress() << 5;
    int bufferWidth = LpVideoCard->m_width >> 6;
    int tw = log(width) / LN2;
    if (width % (int)ldexp(1.0, tw))
        tw++;
    int th = log(height) / LN2;
    if (height % (int)ldexp(1.0, th))
        th++;
    int drawWidth = width;
    int drawHeight = height;
    if (width != 640 && height != 448) {
        drawWidth = (float)width;
        drawHeight = (float)height;
    }
    data->m_spritePacket->ResetPacket();
    data->m_spritePacket->AddSourceDmaTag();
    data->m_spritePacket->AddGifTag(1);
    data->m_spritePacket->AddGifData(SCE_GS_TEST_1, SCE_GS_SET_TEST(1, 0, 0, 1, 0, 0, 0, 0));
    data->m_spritePacket->AddGifData(SCE_GS_PRIM, SCE_GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    data->m_alpha = data->m_spritePacket->AddGifData(SCE_GS_ALPHA_1, SCE_GS_SET_ALPHA(0, 1, 0, 1, 0x80));
    data->m_spritePacket->AddGifData(SCE_GS_TEX0_1, SCE_GS_SET_TEX0(zbp, bufferWidth, 0, tw, th, 1, 0, 0, 0, 0, 0, 1));
    data->m_spritePacket->AddGifData(SCE_GS_TEXFLUSH, 0);
    data->m_spritePacket->AddGifData(SCE_GS_TEX1_1, SCE_GS_SET_TEX1(0, 0, 1, 5, 0, 0, -0x4B));
    data->m_spritePacket->AddGifData(SCE_GS_UV, SCE_GS_SET_UV(16, 16));
    data->m_spritePacket->AddGifData(SCE_GS_RGBAQ, SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x40, 0x3F800000));
    data->m_topLeft = data->m_spritePacket->AddGifData(SCE_GS_XYZ2,
                                                       SCE_GS_SET_XYZ((x << 4) + 0x6C00, (y << 4) + 0x7200, 0));
    data->m_spritePacket->AddGifData(SCE_GS_UV, SCE_GS_SET_UV(width << 4, height << 4));
    data->m_spritePacket->AddGifData(SCE_GS_RGBAQ, SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x40, 0x3F800000));
    data->m_bottomRight = data->m_spritePacket->AddGifData(
        SCE_GS_XYZ2, SCE_GS_SET_XYZ(((x + drawWidth) << 4) + 0x6C00, ((y + drawHeight) << 4) + 0x7200, 0));
    data->m_spritePacket->AddGifData(SCE_GS_TEST_1, SCE_GS_SET_TEST(1, 6, 0, 0, 0, 0, 1, 3));
    data->m_spritePacket->AddDmaEndTag();
}


// ---- src/PSXMpegVideoOutputBuffer/dtor.cpp
PSXMpegVideoOutputBuffer::~PSXMpegVideoOutputBuffer()
{
    if (m_data) {
        for (int i = 0; i < 2; i++) {
            if (m_data[i].m_pixels) {
                free(m_data[i].m_pixels);
                m_data[i].m_pixels = 0;
            }
            if (m_data[i].m_texturePacket) {
                delete m_data[i].m_texturePacket;
                m_data[i].m_texturePacket = 0;
            }
            if (m_data[i].m_spritePacket) {
                delete m_data[i].m_spritePacket;
                m_data[i].m_spritePacket = 0;
            }
        }
        delete[] m_data;
        m_data = 0;
    }
}


// ---- src/PSXMpegVideoOutputBuffer/Initialize.cpp



void PSXMpegVideoOutputBuffer::Initialize()
{
    m_readPos = 0;
    m_writePos = 0;
    m_count = 0;
    m_readIndex = 0;
    m_writeIndex = 0;
    for (int i = 0; i < 2; i++) {
        InitializeTextureDmaPacket(&m_data[i], m_width, m_height, m_height);
        InitializeSpriteDmaPacket(&m_data[i], 0, 0, m_width, m_height);
    }
}


// ---- src/PSXMpegVideoOutputBuffer/UpdateDmaPackets.cpp



void PSXMpegVideoOutputBuffer::UpdateDmaPackets(int x, int y, int width, int height, int format)
{
    for (int i = 0; i < 2; i++) {
        InitializeTextureDmaPacket(&m_data[i], width, height, format);
        InitializeSpriteDmaPacket(&m_data[i], x, y, width, height);
    }
}


// ---- src/PSXMpegVideoOutputBuffer/Reset.cpp

void PSXMpegVideoOutputBuffer::Reset()
{
    m_readPos = 0;
    m_writePos = 0;
    m_count = 0;
    m_readIndex = 0;
    m_writeIndex = 0;
}


// ---- src/PSXMpegVideoOutputBuffer/Write.cpp



PSXMpegVideoOutputBufferData *PSXMpegVideoOutputBuffer::Write()
{
    if (IsFull())
        return 0;
    m_writeIndex = m_writePos;
    m_writePos = (m_writePos + 1) % 2;
    m_count++;
    return &m_data[m_writeIndex];
}


// ---- src/PSXMpegVideoOutputBuffer/Read.cpp


PSXMpegVideoOutputBufferData *PSXMpegVideoOutputBuffer::Read()
{
    if (IsEmpty())
        return 0;
    m_readIndex = m_readPos;
    m_readPos = (m_readPos + 1) % 2;
    m_count--;
    return &m_data[m_readIndex];
}


// ---- src/PSXMpegVideoDecoder/ctor.cpp
PSXMpegVideoDecoder::PSXMpegVideoDecoder(int width, int height, PSXMediaControl *control)
{
    width = (width < LpVideoCard->m_width) ? width : LpVideoCard->m_width;
    height = (height < LpVideoCard->m_height) ? height : LpVideoCard->m_height;
    // 4.5 bytes per pixel plus 2 KB.
    m_workArea = (unsigned char *)memalign(0x40, width * height * 9 / 2 + 0x800);
#line 908 "PSXMpegVideo.cpp"
    ASSERT(m_workArea, "PSXMpegVideoDecoder: memalign failed");
    m_output = new PSXMpegVideoOutputBuffer(width, height);
    m_mpeg = 0;
    m_control = control;
}


// ---- src/PSXMpegVideoDecoder/dtor.cpp
PSXMpegVideoDecoder::~PSXMpegVideoDecoder()
{
    delete m_output;
    m_output = 0;
    if (m_workArea) {
        free(m_workArea);
        m_workArea = 0;
    }
}


// ---- src/PSXMpegVideoDecoder/Initialize.cpp
void PSXMpegVideoDecoder::Initialize()
{
    m_mpeg = new sceMpeg;
    sceMpegCreate(m_mpeg, m_workArea, m_output->m_width * (int)m_output->m_height * 9 / 2 + 0x800);
    sceMpegAddCallback(m_mpeg, 0, (sceMpegCallback)MpegError, m_control);
    sceMpegAddCallback(m_mpeg, 1, (sceMpegCallback)MpegNodata, m_control);
    sceMpegAddCallback(m_mpeg, 4, (sceMpegCallback)MpegBackground, m_control);
    Reset();
    m_output->Initialize();
    m_control->m_inputBuffer->Initialize();
    m_10 = 0;
}


// ---- src/PSXMpegVideoDecoder/Reset.cpp

void PSXMpegVideoDecoder::Reset()
{
    m_state = 0;
    sceMpegReset(m_mpeg);
    m_control->m_inputBuffer->Reset();
}

// ---- src/PSXMpegVideoDecoder/Terminate.cpp
void PSXMpegVideoDecoder::Terminate()
{
    sceMpegDelete(m_mpeg);
    delete m_mpeg;
    m_mpeg = 0;
    int busy = *IPU_CTRL < 0;   // IPU_CTRL bit 31: IPU busy
    if (busy)
        sceMpegInit();
}


// ---- src/PSXMpegVideoDecoder/MpegError.cpp
// libmpeg error callback; `data` is the PSXMediaControl.
int PSXMpegVideoDecoder::MpegError(sceMpeg *mp, sceMpegCbDataError *cb, void *data)
{
    PSXMediaControl *self = (PSXMediaControl *)data;
    strcat(self->m_errorText, cb->errMessage);
    self->m_decodeError = 1;
    return 1;
}


// ---- src/PSXMpegVideoDecoder/MpegNodata.cpp

// sceMpeg callbacks: the context is the PSXMediaControl; forward to its input buffer.

int PSXMpegVideoDecoder::MpegNodata(sceMpeg *mp, sceMpegCbData *data, void *context)
{
    return ((PSXMediaControl *)context)->m_inputBuffer->NoDataAvailable(mp, data, context);
}


// ---- src/PSXMpegVideoDecoder/MpegBackground.cpp

// sceMpeg callbacks: the context is the PSXMediaControl; forward to its input buffer.

int PSXMpegVideoDecoder::MpegBackground(sceMpeg *mp, sceMpegCbData *data, void *context)
{
    return ((PSXMediaControl *)context)->m_inputBuffer->BackgroundProcessing(mp, data, context);
}


// ---- src/PSXMpegVideoDecoder/VideoStreamCallback.cpp

// sceMpeg callbacks: the context is the PSXMediaControl; forward to its input buffer.

int PSXMpegVideoDecoder::VideoStreamCallback(sceMpeg *mp, sceMpegCbDataStr *data, void *context)
{
    return ((PSXMediaControl *)context)->m_inputBuffer->DemuxVideoData(mp, data, context);
}


// ---- src/PSXMpegVideoDecoder/AudioStreamCallback.cpp

// sceMpeg callbacks: the context is the PSXMediaControl; forward to its input buffer.

int PSXMpegVideoDecoder::AudioStreamCallback(sceMpeg *mp, sceMpegCbDataStr *data, void *context)
{
    return ((PSXMediaControl *)context)->m_inputBuffer->DemuxAudioData(mp, data, context);
}


// ---- src/PSXMpegVideoDecoder/DecodeBitstream.cpp
int PSXMpegVideoDecoder::DecodeBitstream()
{
    PSXMpegVideoOutputBufferData *out;
    int result = 1;
    if (!sceMpegIsEnd(m_mpeg)) {
        if (m_state == 1) {     // aborted
            result = -1;
            PSXprintf("decoding of MPEG video ABORTED\n");
        } else {
            out = m_output->Write();
#line 1050 "PSXMpegVideo.cpp"
            ASSERT(out, "DecodeBitstream: LpOutputData == 0");
            ASSERT(out->m_pixels, "DecodeBitstream: LpOutputData->LpData == 0");
            // Decode into the buffer through its uncached mirror.
            sceIpuRGB32 *dst = (sceIpuRGB32 *)PS2_ADDR((((unsigned int)out->m_pixels & 0x0FFFFFFF) | EE_UNCACHED));
            PSXMpegVideoOutputBuffer *buf = m_output;
            int width = buf->m_width;
            int height = buf->m_height;
            int ret = sceMpegGetPicture(m_mpeg, dst, width / 16 * height / 16);
            out->m_frame = m_mpeg->frameCount;
            if (ret < 0) {
                PSXprintf("sceMpegGetPicture() decode error");
                result = -1;
            }
        }
        return result;
    } else {
        sceMpegReset(m_mpeg);
        return m_state = 3;     // ended
    }
}


// ---- src/PSXMpegAudioDecoder/ctor.cpp

PSXMpegAudioDecoder::PSXMpegAudioDecoder(PSXMediaControl *control)
{
    m_state = 0;
    m_hdrCount = 0;
    m_buffer = (unsigned char *)memalign(0x40, 0xC000);
    m_writePos = 0;
    m_used = 0;
    m_bufferSize = 0xC000;
    m_40 = 0;
    m_iopBuffSize = 0x6000;
    m_50 = 0x800;
    m_iopLastPos = 0;
    m_58 = 0;
    m_bytesRead = 0;
    m_playTime = 0;
    m_64 = 0;
    m_68 = 0;
    m_6C = 0;
    m_stream = -1;
    m_soundBuffer = 0;
    m_control = control;
    m_dataSize = 0;
}

// ---- src/PSXMpegAudioDecoder/dtor.cpp
PSXMpegAudioDecoder::~PSXMpegAudioDecoder()
{
    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }
}


// Defined here for its retail .bss slot.
int TotalNumberOfAudioSeconds;

// ---- src/PSXMpegAudioDecoder/Initialize.cpp
void PSXMpegAudioDecoder::Initialize()
{
    m_state = 0;
    m_hdrCount = 0;
    m_writePos = 0;
    m_used = 0;
    m_bufferSize = 0xC000;
    m_40 = 0;
    m_iopBuffSize = 0x6000;
    m_50 = 0x800;
    m_iopLastPos = 0;
    m_58 = 0;
    m_bytesRead = 0;
    m_stream = SND.InitMovieSound();
    m_soundBuffer = SND.GetMovieSoundBuffer(m_stream);
#line 1215 "PSXMpegVideo.cpp"
    ASSERT(m_soundBuffer != 0, "Initialization of Movie Sound Driver Failed!");
    m_iopBuff = m_soundBuffer;
    m_playTime = 0;
    m_64 = 0;
    m_68 = 0;
    m_6C = 0;
    TotalNumberOfAudioSeconds = 0;
}


// ---- src/PSXMpegAudioDecoder/Terminate.cpp

void PSXMpegAudioDecoder::Terminate()
{
    SND.StopMovieSound(m_stream);
    PSXprintf("TotalNumberOfAudioSeconds = %d\n", TotalNumberOfAudioSeconds);
}


// ---- src/PSXMpegAudioDecoder/Reset.cpp



int PSXMpegAudioDecoder::Reset()
{
    SND.StopMovieSound(m_stream);
    m_6C = timeGetTime();
    m_state = 0;
    m_hdrCount = 0;
    m_writePos = 0;
    m_used = 0;
    m_40 = 0;
    m_iopLastPos = 0;
    m_58 = 0;
    m_bytesRead = 0;
    m_64 = 0;
    m_playTime = 0;
    m_68 = 0;
    m_6C = 0;
    return 1;
}


// ---- src/PSXMpegAudioDecoder/IsPreset.cpp

int PSXMpegAudioDecoder::IsPreset()
{
    return m_bytesRead >= m_iopBuffSize;
}


// ---- src/PSXMpegAudioDecoder/Start.cpp

// Starts the movie sound once the stream is preset. The start time goes to
// +0x68 as a float (the shared header declares that member as int).
int PSXMpegAudioDecoder::Start()
{
    if (m_state == 1 && IsPreset()) {
        SND.StartMovieSound(m_stream);
        m_state = 2;
        m_68 = timeGetTime();
        m_playTime = 0;
        m_64 = 1;
        return 1;
    }
    return 0;
}


// ---- src/PSXMpegAudioDecoder/Stop.cpp

int PSXMpegAudioDecoder::Stop()
{
    if (m_state == 2) {
        SND.StopMovieSound(m_stream);
        m_state = 4;
    }
    return 1;
}


// ---- src/PSXMpegAudioDecoder/BeginRingBufferPut.cpp

// Where the next data may be written: one or two pieces (the second when
// the free space wraps around the end of the ring buffer).
void PSXMpegAudioDecoder::BeginRingBufferPut(unsigned char **ptr1, int *size1, unsigned char **ptr2, int *size2)
{
    if (!m_state) {
        // The redundant cast makes mwcc form the array address first, as retail.
        *ptr1 = (unsigned char *)&m_header + m_hdrCount;
        *size1 = 40 - m_hdrCount;
        *ptr2 = m_buffer;
        *size2 = m_bufferSize;
    } else {
        int free = m_bufferSize - m_used;
        if (m_bufferSize - m_writePos >= free) {
            *ptr1 = m_buffer + m_writePos;
            *size1 = free;
            *ptr2 = 0;
            *size2 = 0;
        } else {
            *ptr1 = m_buffer + m_writePos;
            *size1 = m_bufferSize - m_writePos;
            *ptr2 = m_buffer;
            *size2 = free - (m_bufferSize - m_writePos);
        }
    }
}


// ---- src/PSXMpegAudioDecoder/EndRingBufferPut.cpp

void PSXMpegAudioDecoder::EndRingBufferPut(int size)
{
    if (m_state == 0) {
        int headerBytes = 40 - m_hdrCount;
        if (size < headerBytes)
            headerBytes = size;
        m_hdrCount += headerBytes;
        if (m_hdrCount >= 40) {
            m_state = 1;
            PSXprintf("-------- audio information --------------------\n");
            PSXprintf("[%c%c%c%c]\n"
                      "header size:                            %d\n"
                      "type(0:PCM big, 1:PCM little, 2:ADPCM): %d\n"
                      "sampling rate:                          %dHz\n"
                      "channels:                               %d\n"
                      "interleave size:                        %d\n"
                      "interleave start block address:         %d\n"
                      "interleave end block address:           %d\n",
                      m_header.id[0], m_header.id[1], m_header.id[2], m_header.id[3],
                      m_header.size, m_header.type, m_header.rate, m_header.ch,
                      m_header.interSize, m_header.loopStart, m_header.loopEnd);
            PSXprintf("[%c%c%c%c]\n"
                      "data size:                              %d\n",
                      m_body.id[0], m_body.id[1], m_body.id[2], m_body.id[3], m_body.size);
            m_dataSize = m_body.size;
        }
        size -= headerBytes;
    }
    m_writePos = (m_writePos + size) % m_bufferSize;
    m_used += size;
    m_40 += size;
}


// ---- src/PSXMpegAudioDecoder/SendToIOP.cpp

// Sony ezmpeg's audioDecSendToIOP plus a bytes-per-second monitor.
int PSXMpegAudioDecoder::SendToIOP()
{
    int count = 0;
    static int lastTime = 0;
    static int currentTime = 0;
    static int numberOfBytesInOneSec = 0;
    int pd0, pd1, d0, d1;
    unsigned char *ps0, *ps1;
    int s0, s1;
    int countAdj;
    int pos;

    switch (m_state) {
    case 0:
        return 0;
    case 1:
        pd0 = m_iopBuff + m_bytesRead % m_iopBuffSize;
        d0 = m_iopBuffSize - m_bytesRead;
        pd1 = 0;
        d1 = 0;
        break;
    case 2:
        pos = SND.GetMovieSoundStatus(m_stream);
        IopGetArea(&pd0, &d0, &pd1, &d1, pos + m_iopBuff);
        break;
    case 3:
        return 0;
    case 4:
        return 0;
    }

    ps0 = m_buffer + (m_writePos - m_used + m_bufferSize) % m_bufferSize;
    ps1 = m_buffer;
    countAdj = m_used / 1024 * 1024;
    s0 = m_buffer + m_bufferSize - ps0;
    s0 = s0 < countAdj ? s0 : countAdj;
    s1 = countAdj - s0;
    if (d0 + d1 >= 1024 && s0 + s1 >= 1024)
        count = SendAreaToIop(pd0, d0, pd1, d1, ps0, s0, ps1, s1);

    m_used -= count;
    m_bytesRead += count;
    m_iopLastPos = (m_iopLastPos + count) % m_iopBuffSize;
    if (m_64)
        m_playTime = 0.010416667f * m_bytesRead;

    currentTime = timeGetTime();
    numberOfBytesInOneSec += count;
    if (currentTime - lastTime > 1000) {
        if (numberOfBytesInOneSec < 192000)
            PSXprintf("total number of bytes sent to iop in 1 sec = %d\n", numberOfBytesInOneSec);
        lastTime = currentTime;
        if (m_state == 2 && numberOfBytesInOneSec == 0)
            PSXprintf("PSXMpegAudioDecoder::SendToIOP ERROR\n0 bytes sent this second!  TotalNumberOfBytesSentToIop = %d\n", m_bytesRead);
        numberOfBytesInOneSec = 0;
        TotalNumberOfAudioSeconds++;
    }

    m_dataSize -= count;
    if (m_dataSize <= 0) {
        PSXprintf("No audio data left so stopping the decoder!\n");
        Stop();
    }
    return count;
}


// ---- src/PSXMpegAudioDecoder/IopGetArea.cpp

// Sony's ezmpeg iopGetArea: the free part of the IOP ring buffer, in whole
// 1KB units, as one or two areas.
void PSXMpegAudioDecoder::IopGetArea(int *pd0, int *d0, int *pd1, int *d1, int pos)
{
    int len = (pos + m_iopBuffSize - m_iopLastPos - 1024) % m_iopBuffSize;

    if (pos - m_iopLastPos >= 0 && pos - m_iopLastPos < 1024) {
        *pd0 = m_iopBuff;
        *d0 = 0;
        *pd1 = m_iopBuff;
        *d1 = 0;
        return;
    }

    len = len / 1024 * 1024;
    if (m_iopBuffSize - m_iopLastPos >= len) {
        *pd0 = m_iopBuff + m_iopLastPos;
        *d0 = len;
        *pd1 = 0;
        *d1 = 0;
    } else {
        *pd0 = m_iopBuff + m_iopLastPos;
        *d0 = m_iopBuffSize - m_iopLastPos;
        *pd1 = m_iopBuff;
        *d1 = len - (m_iopBuffSize - m_iopLastPos);
    }
}


// ---- src/PSXMpegAudioDecoder/SendAreaToIop.cpp

// Sony's ezmpeg sendToIOP2area: copy the two source areas (s0 bytes at ps0,
// s1 at ps1) into the two IOP areas (d0 bytes at pd0, d1 at pd1), dropping
// whatever doesn't fit. Returns the number of bytes sent.
int PSXMpegAudioDecoder::SendAreaToIop(int pd0, int d0, int pd1, int d1, unsigned char *ps0, int s0, unsigned char *ps1, int s1)
{
    if (d0 + d1 < s0 + s1) {
        int diff = (s0 + s1) - (d0 + d1);
        if (diff >= s1) {
            s0 -= diff - s1;
            s1 = 0;
        } else {
            s1 -= diff;
        }
    }
    if (s0 >= d0) {
        SendDataToIOP(pd0, ps0, d0);
        SendDataToIOP(pd1, ps0 + d0, s0 - d0);
        SendDataToIOP(pd1 + s0 - d0, ps1, s1);
    } else {
        int rest = d0 - s0;
        if (s1 >= rest) {
            SendDataToIOP(pd0, ps0, s0);
            SendDataToIOP(pd0 + s0, ps1, rest);
            SendDataToIOP(pd1, ps1 + d0 - s0, s1 - rest);
        } else {
            SendDataToIOP(pd0, ps0, s0);
            SendDataToIOP(pd0 + s0, ps1, s1);
        }
    }
    return s0 + s1;
}


// ---- src/PSXMpegAudioDecoder/SendDataToIOP.cpp

int PSXMpegAudioDecoder::SendDataToIOP(int iopAddress, unsigned char *data, int size)
{
    if (size <= 0)
        return 0;
    sceSifDmaData dma;
    dma.data = (unsigned int)data;
    dma.addr = iopAddress;
    dma.size = size;
    dma.mode = 0;
    FlushCache(0);
    unsigned int id = sceSifSetDma(&dma, 1);
    while (sceSifDmaStat(id) >= 0)
        ;
    return size;
}


// The movie player's shared constants are defined in PSXMpegDefs.cpp.
