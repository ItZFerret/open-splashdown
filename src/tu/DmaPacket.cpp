// DmaPacket.cpp  @ 0x002C55D0 (text 0x2C55D0-0x2C5E80)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// DmaPacket as one translation unit without a static initializer.
// PSXRenderStateBlock, GsTextureTransfer, IpuTextureTransfer and DmaPacket sit
// between DataAccess.cpp and JetskiGameOptions.cpp; their .rodata (@504-@818)
// has no label restart, but PSXRenderStateBlock's 16-byte constant 0xA (@507)
// and DmaPacket's 8-byte 0xA (@818) are separate copies, which one mwcc TU
// pools into one. So they are more than one TU; one per class is a guess, and
// so are the file names.



#include "PSXRenderStateBlock/PSXRenderStateBlock.h"
#include "sceGifTag/sceGifTag.h"
#include "PSXVideoCard/PSXVideoCard.h"
#include "DmaRenderItem/DmaRenderItem.h"
#include "GsTextureTransfer/GsTextureTransfer.h"
#include "DmaPacket/DmaPacket.h"
#include "_RECT/_RECT.h"
#include "Camera/Camera.h"
#include "IpuTextureTransfer/IpuTextureTransfer.h"

#include "PSXTextureMap/PSXTextureMap.h"
#include "PSXRenderTarget/PSXRenderTarget.h"
#include "GsTextureTransfer/GsTextureTransfer.h"
#include "DmaPacket/DmaPacket.h"
#include "soultree_assert_block.h"

// A texture transfer packet as GsTextureTransfer builds it (DmaPacket data).
struct TransferPacket {
    char pad0[0x20];
    long m_bitbltbuf;               // +0x20
    char pad28[0x30 - 0x28];
    long m_trxreg;                  // +0x30
    char pad38[0x70 - 0x38];
    unsigned short m_nloop : 15;    // +0x70 (image GIFtag)
    unsigned short m_eop : 1;
    char pad72[0x80 - 0x72];
    unsigned short m_qwc;           // +0x80 (DMA tag)
    short pad82;
    unsigned int m_addr;            // +0x84
    char pad88[0xB0 - 0x88];
    long m_clutBitbltbuf;           // +0xB0
    char padB8[0x104 - 0xB8];
    unsigned int m_clutAddr;        // +0x104
};

// Retail's .rodata has the file name and "\n" (@816, @818) long before
// BuildRawDmaTag's message (@1020), with @817 never written: a function with
// an ASSERT came first, which nothing called, so retail's linker stripped it
// with its message and statics.
#line 100 "PSXTextureTransfer.cpp"
static void CheckQwords(int numQwords, int maxQwords)
{
    ASSERT(numQwords <= maxQwords, "we are trying to send more qwords than we have allocated");
}

// ---- src/DmaPacket/ctor.cpp

extern "C" void *memalign(unsigned int align, unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);

// size is in quadwords, rounded up to a multiple of 8. An uncached packet is accessed through
// the EE's uncached mirror (0x20000000).
DmaPacket::DmaPacket(int size, int uncached)
{
    size = (size + 7) / 8 * 8;  // round up to 8 quadwords
    unsigned int *buffer = (unsigned int *)memalign(64, size * 16);
    if (uncached)
        m_start = (unsigned int *)PS2_ADDR((((unsigned int)buffer & 0x0FFFFFFF) | EE_UNCACHED));
    else
        m_start = buffer;
    memset(m_start, 0, size * 16);
    m_maxQwords = size;
    ResetPacket();
}


// ---- src/DmaPacket/dtor.cpp

extern "C" void free(void *p);

DmaPacket::~DmaPacket()
{
    // The packet buffer is addressed through an uncached mirror; free the
    // cached address.
    if (m_start)
        free((void *)PS2_ADDR(((unsigned int)m_start & 0x0FFFFFFF)));
}


// ---- src/DmaPacket/ResetPacket_002C56F0.cpp

void DmaPacket::ResetPacket()
{
    m_cur = m_start;
    m_openDmaTag = 0;
    m_openTag = 0;
    m_numTags = 0;
    m_openTagCount = 0;
    m_numQwords = 0;
}


// ---- src/DmaPacket/AddSourceDmaTag.cpp

// Closes both open tags and opens a CNT tag (id 1) at the write cursor; its
// QWC is filled in when the tag is closed.
void DmaPacket::AddSourceDmaTag()
{
    CloseDmaTag();
    CloseGifTag();
    m_openDmaTag = m_cur;
    BuildRawDmaTag(0, 0, 0, 1, 0, 0);
}


// ---- src/DmaPacket/AddGifTag.cpp

void DmaPacket::AddGifTag(int eop)
{
    unsigned int end = 0;
    if (eop)
        end = 1;
    CloseGifTag();
    m_openTag = m_cur;
    BuildRawGifTag(0xE, 1, 0, 0, 0, end, 0);
    m_numTags++;
}


// ---- src/DmaPacket/AddGifData.cpp

unsigned int *DmaPacket::AddGifData(unsigned int a, unsigned long data)
{
    unsigned int *start = BuildRawGifData(a, data);
    m_numTags++;
    m_openTagCount++;
    return start;
}


// ---- src/DmaPacket/AddGifData_002C5850.cpp

void DmaPacket::AddGifData(unsigned int *dst, unsigned int a, unsigned long data)
{
    BuildRawGifData(dst, a, data);
}


// ---- src/DmaPacket/AddGifTextureTag.cpp

void DmaPacket::AddGifTextureTag(unsigned int nloop)
{
    CloseGifTag();
    BuildRawGifTag(0, 0, 2, 0, 0, 1, nloop);
    m_numTags++;
}


// ---- src/DmaPacket/AddSourceDmaTextureTag.cpp

// DMA source-chain tag, id 3 (REF), closing any open GIF tag first.
void DmaPacket::AddSourceDmaTextureTag(unsigned char *data, unsigned int qwc)
{
    CloseGifTag();
    BuildRawDmaTag(0, data, 0, 3, 0, qwc);
}


// ---- src/DmaPacket/AddSourceDmaIpuTag.cpp

// DMA source-chain tag, id 3 (REF): transfer qwc quadwords from data.
void DmaPacket::AddSourceDmaIpuTag(unsigned char *data, unsigned int qwc)
{
    BuildRawDmaTag(0, data, 0, 3, 0, qwc);
}


// ---- src/DmaPacket/AddDmaEndTag.cpp

// Closes both open tags and ends the chain with an END tag (id 7).
void DmaPacket::AddDmaEndTag()
{
    CloseDmaTag();
    CloseGifTag();
    BuildRawDmaTag(0, 0, 0, 7, 0, 0);
}


// ---- src/DmaPacket/SendGifPacket.cpp
extern RenderTarget *LpRenderTarget;
extern int LockRenderQueueState;


#define D2_CHCR ((volatile unsigned int *)0x1000A000)
#define D2_QWC  ((volatile unsigned int *)0x1000A020)
#define D2_TADR ((volatile unsigned int *)0x1000A030)

// Kick the packet down DMA channel 2 (GIF) as a source chain.
void DmaPacket::SendGifPacket(int wait, int waitForQueue)
{
    if (waitForQueue) {
        static int WaitForGet2 = -1;    // never used, but retail initializes it
        while (LockRenderQueueState) {
            if (((PSXRenderTarget *)LpRenderTarget)->m_finishCallback)
                ((PSXRenderTarget *)LpRenderTarget)->m_finishCallback();
        }
    }
    while (*D2_CHCR & 0x100)
        ;
    *D2_TADR = (unsigned int)m_start & 0x0FFFFFFF;
    *D2_QWC = 0;
    *D2_CHCR = 0x104;
    if (wait) {
        while (*D2_CHCR & 0x100)
            ;
    }
}


// ---- src/DmaPacket/SendIpuPacket_audit.cpp

// IPU_TO DMA channel (ipu0 = from memory to the IPU) registers.
#define D4_CHCR (*(volatile unsigned int *)0x1000B400)
#define D4_MADR (*(volatile unsigned int *)0x1000B430)
#define D4_QWC  (*(volatile unsigned int *)0x1000B420)

void DmaPacket::SendIpuPacket(int wait)
{
    while (D4_CHCR & 0x100) {
    }
    D4_MADR = (unsigned int)m_start & 0x0FFFFFFF;
    D4_QWC = 0;
    D4_CHCR = 0x104;
    if (wait) {
        while (D4_CHCR & 0x100) {
        }
    }
}


// ---- src/DmaPacket/BuildRawDmaTag.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off


#ifdef SPLASHDOWN_PC
extern "C" void port_dma_unaligned(const void *addr, const char *where);
#endif

// Standard DMAtag: QWC | PCE<<26 | ID<<28 | IRQ<<31, then ADDR | SPR<<31.
void DmaPacket::BuildRawDmaTag(unsigned int spr, void *addr, unsigned int irq,
                               unsigned int id, unsigned int pce, unsigned int qwc)
{
#ifdef SPLASHDOWN_PC
    // the address is masked to a qword: an unaligned one is a layout bug
    if ((unsigned long)addr & 0xF)
        port_dma_unaligned(addr, "BuildRawDmaTag");
#endif
    *m_cur = qwc | (pce << 26) | (id << 28) | (irq << 31);
    m_cur++;
    *m_cur = ((unsigned int)addr & ~0xF) | (spr << 31);
    m_cur++;
    *m_cur = 0;
    m_cur++;
    *m_cur = 0;
    m_cur++;
    m_numQwords++;
#line 1400 "PSXTextureTransfer.cpp"
    ASSERT(m_numQwords <= m_maxQwords, "BuildRawDmaTag: we are trying to send more qwords than we have allocated");
}
#pragma alignlabel reset


// ---- src/DmaPacket/BuildRawGifTag.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off


// Standard GIFtag: NLOOP | EOP<<15, then PRE<<46 | PRIM<<47 | FLG<<58 |
// NREG<<60 (as the high word), then the 64-bit REGS descriptor.
void DmaPacket::BuildRawGifTag(unsigned long regs, unsigned int nreg, unsigned int flg,
                               unsigned int prim, unsigned int pre, unsigned int eop,
                               unsigned int nloop)
{
    *m_cur = nloop | (eop << 15);
    m_cur++;
    *m_cur = (pre << 14) | (prim << 15) | (flg << 26) | (nreg << 28);
    m_cur++;
    *m_cur = regs & 0xFFFFFFFF;
    m_cur++;
    *m_cur = (int)(regs >> 32);
    m_cur++;
    m_numQwords++;
#line 1442 "PSXTextureTransfer.cpp"
    ASSERT(m_numQwords <= m_maxQwords, "BuildRawGifTag: we are trying to send more qwords than we have allocated");
}
#pragma alignlabel reset


// ---- src/DmaPacket/BuildRawGifData.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off


unsigned int *DmaPacket::BuildRawGifData(unsigned int a, unsigned long data)
{
    unsigned int *start = m_cur;
    *m_cur = data & 0xFFFFFFFF;
    m_cur++;
    *m_cur = (int)(data >> 32);
    m_cur++;
    *m_cur = a;
    m_cur++;
    *m_cur = 0;
    m_cur++;
    m_numQwords++;
#line 1475 "PSXTextureTransfer.cpp"
    ASSERT(m_numQwords <= m_maxQwords, "BuildRawGifData: we are trying to send more qwords than we have allocated");
    return start;
}
#pragma alignlabel reset


// ---- src/DmaPacket/BuildRawGifData_002C5E50.cpp

void DmaPacket::BuildRawGifData(unsigned int *dst, unsigned int a, unsigned long data)
{
    dst[0] = data & 0xFFFFFFFF;
    dst[1] = (int)(data >> 32);
    dst[2] = a;
    dst[3] = 0;
}

