// The Sony SDK functions the game calls that the port doesn't implement yet.
// Each logs its name the first time it runs (PORT_STUB) and returns what the
// game takes for success. Signatures follow the game's own declarations;
// struct arguments are void * where the stub doesn't look inside.
#include "port.h"
#include "port_ptr32.h"
#include "audio/audio.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

// ---- libkernl: caches, printf (interrupts: ports/pc/hw.cpp)
void FlushCache(int mode)
{
}

void iFlushCache(int mode)
{
}

int scePrintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}

// ---- IOP: modules, heap, RPC (the sound driver's RPC: sound_rpc.cpp)
int sceSifRebootIop(const char *img)
{
    return 1;
}

int sceSifSyncIop(void)
{
    return 1;
}

void sceSifInitRpc(unsigned int mode)
{
}

int sceSifLoadModule(const char *name, int argLength, const char *args)
{
    port_log("IOP module %s", name);
    return 1;
}

int sceSifInitIopHeap(void)
{
    return 0;
}

// IOP memory is another address space: the game only passes these addresses
// back to the SDK (sceCdStInit, SIF DMA), never dereferences them, and frees
// some twice (PSXMpegStreamFile::Close). So they are IOP-like addresses
// below the executable, unmapped on the PC, from a bump allocator that
// wraps around the IOP's 2 MB.
void *sceSifAllocIopHeap(unsigned int size)
{
    static uintptr_t next = 0x10000;
    size = (size + 255) & ~255u;
    if (next + size > 0x200000)
        next = 0x10000;
    uintptr_t p = next;
    next += size;
    return (void *)p;
}

int sceSifFreeIopHeap(void *p)
{
    return 0;
}

// sceSifBindRpc, sceSifCallRpc, sceSifCheckStatRpc: ports/pc/sdk/sound_rpc.cpp

// SIF DMA to the IOP: the movie sound's PCM (PSXMpegAudioDecoder::
// SendDataToIOP) into the sound driver's buffer; done at once.
unsigned int sceSifSetDma(void *sdd, int len)
{
    struct SifDmaData {
        uint32_t data, addr, size, mode;
    };
    const SifDmaData *d = (const SifDmaData *)sdd;
    for (int i = 0; i < len; i++)
        audio::IopWrite(d[i].addr, (const void *)port_guest_addr(d[i].data), d[i].size);
    return 1;
}

int sceSifDmaStat(unsigned int id)
{
    return -1; // finished
}

int sceFsReset(void)
{
    return 0;
}

// ---- GS, DMA, VIF, VU, IPU
struct PortGsGParam {
    short sceGsInterMode;
    short sceGsOutMode;
    short sceGsFFMode;
    short sceGsVersion;
};
static PortGsGParam g_gsParam;

void sceGsResetGraph(short mode, short interlace, short omode, short ffmode)
{
    g_gsParam.sceGsInterMode = interlace;
    g_gsParam.sceGsOutMode = omode;
    g_gsParam.sceGsFFMode = ffmode;
}

void *sceGsGetGParam(void)
{
    return &g_gsParam;
}

void sceGsResetPath(void)
{
}

// sceGsSyncV: ports/pc/hw.cpp

// The DMA channels' registers at their PS2 addresses (mapped by
// ports/pc/hw.cpp). The game sets bits in CHCR itself.
void *sceDmaGetChan(int id)
{
    static const uintptr_t kBase[10] = {
        0x10008000, 0x10009000, 0x1000A000, 0x1000B000, 0x1000B400,
        0x1000C000, 0x1000C400, 0x1000C800, 0x1000D000, 0x1000D400,
    };
    if (id < 0 || id >= 10)
        port_fatal("sceDmaGetChan(%d)", id);
    return (void *)PORT_HW(kBase[id]);
}

void sceDmaReset(int mode)
{
}

void sceDevVu0Reset(void)
{
}

void sceDevVu1Reset(void)
{
}

int sceDevVif1Reset(void)
{
    return 0;
}

void sceVpu0Reset(void)
{
}

// sceIpuInit, sceIpuSync: ports/pc/ipu.cpp

// libpad, libmtap: ports/pc/sdk/pad.cpp

// libmc: ports/pc/sdk/memcard.cpp

// libmpeg: ports/pc/sdk/mpeg.cpp

} // extern "C"
