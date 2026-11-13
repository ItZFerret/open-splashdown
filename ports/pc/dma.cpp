// The DMA controller on the PC.
//
// The game starts transfers the way the EE does: it writes MADR/TADR/QWC
// and sets STR (bit 8) in a channel's CHCR, at the PS2 register addresses
// (mapped by ports/pc/hw.cpp), then polls STR, D_STAT or its own flags. Like the
// DMAC, a host thread watches every channel's STR bit, performs the transfer
// and clears STR, setting the channel's bit in D_STAT. The game copes with
// the transfer running beside it because it does on the PS2.
//
// A transfer runs to completion before STR clears: VIF1 (ch1) decodes into
// VU1 and runs its microprograms (ports/pc/vu/), the GIF (ch2) feeds the GS
// (ports/pc/gs/), toIPU/fromIPU feed and drain ports/pc/ipu.cpp, toSPR/fromSPR
// copy. VIF0 (VU0 microcode uploads) is counted
// only. Chain mode follows the tags (cnt, next, ref, refs, refe, call, ret,
// end; TTE hands VIF1 a tag's upper 64 bits; a tag's IRQ bit ends the
// transfer when CHCR.TIE is set); normal mode sends MADR/QWC.
#include "port.h"
#include "port_simd.h"

#include "gs/gs.h"
#include "vu/vu.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <climits>
#include "port_os.h"
#include "port_ptr32.h"

extern uintptr_t g_vif1_src;
void port_ipu_dma_in(const void *data, uint32_t qwc, bool first);
void port_ipu_dma_out(void *dst, uint32_t qwc);

namespace {

struct Channel {
    const char *name;
    uintptr_t base;
};

const Channel kChannels[10] = {
    {"VIF0", 0x10008000},   {"VIF1", 0x10009000},   {"GIF", 0x1000A000},
    {"fromIPU", 0x1000B000}, {"toIPU", 0x1000B400},  {"SIF0", 0x1000C000},
    {"SIF1", 0x1000C400},   {"SIF2", 0x1000C800},   {"fromSPR", 0x1000D000},
    {"toSPR", 0x1000D400},
};

const uintptr_t kDStat = 0x1000E010;
const uint32_t kStr = 0x100;

std::atomic<uint64_t> g_kicks[10];

// Futex words: g_done counts finished transfers (bc0f waiters sleep on it),
// g_wake is bumped to wake an idle DMA thread.
std::atomic<uint32_t> g_done, g_wake, g_doneWaiters;

void FutexWait(std::atomic<uint32_t> &w, uint32_t v, long ns)
{
    port_futex_wait(&w, v, ns);
}

void FutexWake(std::atomic<uint32_t> &w)
{
    port_futex_wake(&w, true);
}


volatile uint32_t &Reg(uintptr_t addr)
{
    return *(volatile uint32_t *)PORT_HW(addr);
}

// A DMA address as a host pointer: the scratchpad (SPR bit or 0x7xxxxxxx),
// else the game's memory below 256 MB (the uncached segment bits dropped).
uint8_t *Ptr(uint32_t addr, bool spr = false)
{
    if (spr || (addr >> 28) == 7)
        return (uint8_t *)(uintptr_t)PORT_HW(0x70000000 | (addr & 0x3FF0));
    return (uint8_t *)(uintptr_t)PORT_HW(addr & 0x0FFFFFF0);
}

void Send(int ch, const uint8_t *data, uint32_t qwc)
{
    if (!qwc)
        return;
    switch (ch) {
    case 1:
        g_vif1_src = (uintptr_t)data;
        vif1_write((const uint32_t *)data, qwc * 4);
        break;
    case 2: gs_gif_write(GS_PATH3, data, qwc); break;
    case 4: port_ipu_dma_in(data, qwc, false); break;
    }
}

// Is [a, a + qwc * 16) game memory (the executable and the brk heap, the
// PORT_HEAPGUARD region, or the scratchpad)? A tag pointing elsewhere is a layout bug in the packet
// builder: log it and skip the data rather than fault on the DMA thread.
bool Valid(uint32_t addr, uint32_t qwc, bool spr)
{
    if (spr || (addr >> 28) == 7)
        return true;
    // (as 32-bit game addresses: the executable starts at 0x400000 on
    // x86-64, at the window's start on arm64)
    static uintptr_t top;
    const uintptr_t bottom = PORT_GUEST_BASE ? 0x4000 : 0x400000;
    uintptr_t a = addr & 0x0FFFFFF0, end = a + (uintptr_t)qwc * 16;
    if (a >= bottom && end <= top)
        return true;
    top = port_game_heap_end() - PORT_GUEST_BASE;
    return a >= bottom && end <= top;
}

void BadTag(int ch, uint32_t tadr, uint64_t t)
{
    static int n;
    if (n++ < 20)
        port_log("DMA %s: tag at 0x%x (%016llx) points outside game memory; skipped",
                 kChannels[ch].name, tadr, (unsigned long long)t);
}

// A DMA address with its low 4 bits set: the DMAC ignores them, so on the PC
// it is a layout bug (data the hardware reads that isn't qword-aligned on
// x86-64). Logged once per address.
void Unaligned(int ch, const char *what, uint32_t a, uint32_t tadr = 0)
{
    static int n;
    if ((a & 0xF) && n < 32) {
        n++;
        port_log("DMA %s: %s 0x%x is not qword-aligned (tag at 0x%x)", kChannels[ch].name, what, a, tadr);
        if (tadr) {
            const uint32_t *q = (const uint32_t *)(uintptr_t)PORT_HW(tadr - 0x20);
            port_log("  around the tag: %08x %08x %08x %08x | %08x %08x %08x %08x | [%08x %08x %08x %08x] | %08x %08x %08x %08x",
                     q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], q[8], q[9], q[10], q[11], q[12], q[13], q[14], q[15]);
            const uint32_t *d = (const uint32_t *)(uintptr_t)PORT_HW(a & ~7u);
            port_log("  data: %08x %08x %08x %08x %08x %08x", d[-2], d[-1], d[0], d[1], d[2], d[3]);
            a = tadr;   // name the object holding the tag
        }
        // the nearest vtable pointer below it names the object holding the
        // data (PORT_VTABLE_MIN/MAX: the executable's vtable range, from nm)
        static uint64_t lo = getenv("PORT_VTABLE_MIN") ? strtoull(getenv("PORT_VTABLE_MIN"), nullptr, 16) : 0;
        static uint64_t hi = getenv("PORT_VTABLE_MAX") ? strtoull(getenv("PORT_VTABLE_MAX"), nullptr, 16) : 0;
        for (int k = 0; lo && k < 1024; k++) {
            uint64_t w;
            uintptr_t at = (uintptr_t)PORT_HW(a & ~7u) - (uintptr_t)k * 8;
            memcpy(&w, (const void *)at, 8);
            if (w >= lo && w < hi) {
                port_log("  vtable %llx at -0x%lx", (unsigned long long)w, (unsigned long)(a - at));
                break;
            }
        }
    }
}

void Chain(int ch, uint32_t chcr)
{
    bool tte = chcr & 0x40, tie = chcr & 0x80;
    uint32_t tadr = Reg(kChannels[ch].base + 0x30);
    uint32_t asr[2];
    int sp = 0;
    for (int guard = 0; guard < 1000000; guard++) {
        Unaligned(ch, "tag", tadr);
        const uint8_t *tag = Ptr(tadr);
        uint64_t t;
        memcpy(&t, tag, 8);
        uint32_t qwc = t & 0xFFFF, id = (t >> 28) & 7, addr = (uint32_t)(t >> 32) & 0x7FFFFFFF;
        bool irq = t >> 31 & 1, spr = t >> 63;
        if (tte && ch == 1) {
            g_vif1_src = (uintptr_t)(tag + 8);
            vif1_write((const uint32_t *)(tag + 8), 2);
        }
        uint32_t next = 0;
        bool end = false;
        switch (id) {
        case 0: case 3: case 4:
            Unaligned(ch, "ref address", addr, tadr);
            if (!Valid(addr, qwc, spr)) {
                BadTag(ch, tadr, t);
                if (id == 0)
                    return;
                tadr += 16;
                continue;
            }
            break;
        case 2: case 5:
            Unaligned(ch, "next/call address", addr, tadr);
            if (!Valid(addr, 0, false)) {
                BadTag(ch, tadr, t);
                return;
            }
            break;
        }
        switch (id) {
        case 0: Send(ch, Ptr(addr, spr), qwc); end = true; break;                          // refe
        case 1: Send(ch, tag + 16, qwc); next = tadr + 16 + qwc * 16; break;                // cnt
        case 2: Send(ch, tag + 16, qwc); next = addr; break;                                // next
        case 3: case 4: Send(ch, Ptr(addr, spr), qwc); next = tadr + 16; break;             // ref, refs
        case 5:                                                                              // call
            Send(ch, tag + 16, qwc);
            if (sp < 2)
                asr[sp++] = tadr + 16 + qwc * 16;
            next = addr;
            break;
        case 6:                                                                              // ret
            Send(ch, tag + 16, qwc);
            if (sp)
                next = asr[--sp];
            else
                end = true;
            break;
        default: Send(ch, tag + 16, qwc); end = true; break;                                // end
        }
        if (end || (irq && tie))
            return;
        tadr = next;
    }
    port_log("DMA %s: chain at 0x%x doesn't end", kChannels[ch].name, Reg(kChannels[ch].base + 0x30));
}

void Transfer(int ch)
{
    g_kicks[ch]++;
    uint32_t chcr = Reg(kChannels[ch].base);
    uint32_t madr = Reg(kChannels[ch].base + 0x10), qwc = Reg(kChannels[ch].base + 0x20);
    uint32_t mode = (chcr >> 2) & 3;
    switch (ch) {
    case 1: case 2:
        if (ch == 1 && !(chcr & 1)) {
            // VIF1 to memory: a GS local->host transfer
            gs_read_image(Ptr(madr), qwc);
        } else if (mode == 1) {
            Chain(ch, chcr);
        } else {
            Send(ch, Ptr(madr), qwc);
        }
        break;
    case 3: // fromIPU: the decoder's output, normal mode
        port_ipu_dma_out(Ptr(madr), qwc);
        break;
    case 4: // toIPU: a new bitstream
        port_ipu_dma_in(nullptr, 0, true);
        if (mode == 1)
            Chain(ch, chcr);
        else
            Send(ch, Ptr(madr), qwc);
        break;
    case 8: case 9: {
        uint32_t sadr = Reg(kChannels[ch].base + 0x80) & 0x3FF0;
        uint8_t *spr = (uint8_t *)(uintptr_t)PORT_HW(0x70000000 + sadr);
        if (mode != 0) {
            port_log("DMA %s: mode %u not supported", kChannels[ch].name, mode);
            break;
        }
        if (ch == 8)
            memcpy(Ptr(madr), spr, qwc * 16);
        else
            memcpy(spr, Ptr(madr), qwc * 16);
        break;
    }
    }
}

// The last transfers (watchdog dumps): when, which channel, CHCR, MADR,
// TADR, QWC as they started; done when they finished.
struct Event {
    uint64_t ns;
    uint32_t ch, chcr, madr, tadr, qwc, done;
};
const int kEvents = 64;
Event g_events[kEvents];
std::atomic<uint64_t> g_nEvents;
std::atomic<int> g_servicing{-1};
std::atomic<uint64_t> g_rewrites;  // starts that came while the channel ran  // the channel being transferred, -1 idle (nested: the innermost)

uint64_t NowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool Service(int ch)
{
    volatile uint32_t &chcr = Reg(kChannels[ch].base);
    if (!(chcr & kStr))
        return false;
    std::atomic_thread_fence(std::memory_order_acquire);
    uint64_t ev = g_nEvents.load(std::memory_order_relaxed);
    {
        Event &e = g_events[ev % kEvents];
        e = {NowNs(), (uint32_t)ch, chcr, Reg(kChannels[ch].base + 0x10), Reg(kChannels[ch].base + 0x30),
             Reg(kChannels[ch].base + 0x20), 0};
        g_nEvents.store(ev + 1, std::memory_order_release);
    }
    int outer = g_servicing.exchange(ch);
    uint32_t tadr0 = Reg(kChannels[ch].base + 0x30), madr0 = Reg(kChannels[ch].base + 0x10);
    Transfer(ch);
    g_servicing.store(outer);
    // A start written while the channel ran (the writer saw it idle before
    // it started: two threads kicking one channel) is lost when STR clears
    if (Reg(kChannels[ch].base + 0x30) != tadr0 || Reg(kChannels[ch].base + 0x10) != madr0) {
        uint64_t n = g_rewrites.fetch_add(1);
        if (n < 10)
            port_log("DMA %s: TADR/MADR rewritten during a transfer (%08x/%08x -> %08x/%08x)", kChannels[ch].name,
                     tadr0, madr0, Reg(kChannels[ch].base + 0x30), Reg(kChannels[ch].base + 0x10));
    }
    if (g_nEvents.load(std::memory_order_relaxed) - ev <= kEvents)
        g_events[ev % kEvents].done = 1;
    // PORT_DETERMINISTIC: the GS has taken it all before the game sees the
    // channel finish (FINISH, the frame count PORT_INPUT uses); otherwise
    // the GS thread may lag behind, as the real GS does (readbacks wait for
    // it: gs_read_image)
    if ((ch == 1 || ch == 2) && port_deterministic())
        gs_sync();
    std::atomic_thread_fence(std::memory_order_release);
    Reg(kChannels[ch].base + 0x20) = 0; // QWC
    // atomic read-modify-writes: the game writes these registers too (a
    // plain load and store would undo a write that came in between)
    __atomic_and_fetch((uint32_t *)&chcr, ~kStr, __ATOMIC_SEQ_CST);
    __atomic_or_fetch((uint32_t *)&Reg(kDStat), 1u << ch, __ATOMIC_SEQ_CST);
    g_done.fetch_add(1);
    if (g_doneWaiters.load())
        FutexWake(g_done);
    return true;
}

// The game starts a channel by writing CHCR, which the PC can't see: the
// thread polls, spinning briefly after a transfer, then sleeping on g_wake
// for 50 us, growing to 1 ms after a long idle. A game thread that waits
// for a channel (bc0f) wakes it.
void DmaThread()
{
    port_thread_name("dma");
    int idle = 0;
    for (;;) {
        bool busy = false;
        // The GIF before VIF1: when both are pending the game started the
        // GIF first (PSXTextureMapManager::BeginTransfer sends the frame's
        // first texture, then SendRenderQueue the queue that draws with it).
        static const int kOrder[10] = {2, 0, 1, 3, 4, 5, 6, 7, 8, 9};
        uint32_t wake = g_wake.load();
        for (int ch : kOrder)
            busy |= Service(ch);
        if (busy) {
            idle = 0;
        } else if (++idle > 2048) {
            FutexWait(g_wake, wake, idle > 2048 + 2000 ? 1000000 : 50000);
        } else {
            port_cpu_pause();
        }
    }
}

} // namespace

// DmaPacket::BuildRawDmaTag got an address that isn't qword-aligned.
extern "C" void port_dma_unaligned(const void *addr, const char *where)
{
    static const char *seen[64];
    static int n;
    for (int i = 0; i < n; i++)
        if (seen[i] == where)
            return;
    if (n < 64)
        seen[n++] = where;
    // where: a name, or a caller's return address (addr2line -e build/port/splashdown)
    if ((uintptr_t)where - PORT_GUEST_BASE < 0x4000 || (uintptr_t)where - PORT_GUEST_BASE > 0x800000)
        port_log("DMA: %s: address %p is not qword-aligned", where, addr);
    else
        port_log("DMA: address %p is not qword-aligned, from %p", addr, (const void *)where);
}

// Debugging: the nearest vtable pointer at or below p (PORT_VTABLE_MIN/MAX,
// the executable's vtable range from nm) names the object holding p.
void port_describe_address(const void *p)
{
    static uint64_t lo = getenv("PORT_VTABLE_MIN") ? strtoull(getenv("PORT_VTABLE_MIN"), nullptr, 16) : 0;
    static uint64_t hi = getenv("PORT_VTABLE_MAX") ? strtoull(getenv("PORT_VTABLE_MAX"), nullptr, 16) : 0;
    uintptr_t a = (uintptr_t)p;
    for (int k = 0; lo && k < 1024; k++) {
        uint64_t w;
        uintptr_t at = (a & ~(uintptr_t)7) - (uintptr_t)k * 8;
        memcpy(&w, (const void *)at, 8);
        if (w >= lo && w < hi) {
            port_log("  %p: vtable %llx at -0x%lx", p, (unsigned long long)w, (unsigned long)(a - at));
            return;
        }
    }
}

bool port_dma_cpcond0(void)
{
    uint32_t pcr = Reg(0x1000E020) & 0x3FF;  // D_PCR.CPC
    for (int ch = 0; ch < 10; ch++)
        if ((pcr >> ch & 1) && (Reg(kChannels[ch].base) & kStr))
            return false;
    return true;
}

// bc0f's wait: sleep until a transfer finishes (or 2 ms pass), after waking
// the DMA thread in case it hasn't seen the channel start.
void port_dma_wait_cpcond0(void)
{
    uint32_t seq = g_done.load();
    if (port_dma_cpcond0())
        return;
    port_tls_bc0f = 0x80000000u | (Reg(0x1000E020) & 0x3FF);
    g_wake.fetch_add(1);
    FutexWake(g_wake);
    g_doneWaiters.fetch_add(1);
    if (!port_dma_cpcond0())
        FutexWait(g_done, seq, 2000000);
    g_doneWaiters.fetch_sub(1);
    port_tls_bc0f = 0;
}

namespace {
// Sleep until done() (a transfer finished and it holds), waking the DMA
// thread first.
template <class F> void WaitUntil(F done)
{
    for (;;) {
        uint32_t seq = g_done.load();
        if (done())
            return;
        g_wake.fetch_add(1);
        FutexWake(g_wake);
        g_doneWaiters.fetch_add(1);
        if (!done())
            FutexWait(g_done, seq, 2000000);
        g_doneWaiters.fetch_sub(1);
    }
}
} // namespace

extern "C" void port_dma_poll(int ch)
{
    if (port_deterministic() && ch >= 0 && ch < 10)
        WaitUntil([ch] { return !(Reg(kChannels[ch].base) & kStr); });
}

void port_dma_wait_channel(int ch)
{
    WaitUntil([ch] { return !(Reg(kChannels[ch].base) & kStr); });
}

void port_dma_wait_idle(void)
{
    WaitUntil([] {
        for (int ch = 0; ch < 10; ch++)
            if (Reg(kChannels[ch].base) & kStr)
                return false;
        return true;
    });
}

void port_dma_service(int ch)
{
    Service(ch);
}

void port_dma_start(void)
{
    port_watch_futex(&g_done, "dma.done");
    port_watch_futex(&g_wake, "dma.wake");
    std::thread(DmaThread).detach();
}

void port_dma_debug(FILE *f)
{
    fprintf(f, "DMA: D_CTRL %08x D_STAT %08x D_PCR %08x; servicing %d, done %u (waiters %u), wake %u, "
               "starts during a transfer %llu\n",
            Reg(0x1000E000), Reg(kDStat), Reg(0x1000E020), g_servicing.load(), g_done.load(), g_doneWaiters.load(),
            g_wake.load(), (unsigned long long)g_rewrites.load());
    for (int ch = 0; ch < 10; ch++) {
        uintptr_t b = kChannels[ch].base;
        if (!g_kicks[ch] && !(Reg(b) & kStr))
            continue;
        fprintf(f, "  %-7s CHCR %08x MADR %08x QWC %04x TADR %08x%s kicks %llu\n", kChannels[ch].name, Reg(b),
                Reg(b + 0x10), Reg(b + 0x20), Reg(b + 0x30), (Reg(b) & kStr) ? " STR" : "    ",
                (unsigned long long)g_kicks[ch].load());
    }
    uint64_t n = g_nEvents.load(std::memory_order_acquire), now = NowNs();
    fprintf(f, "  last transfers (newest last; ms before now):\n");
    for (uint64_t i = n > 24 ? n - 24 : 0; i < n; i++) {
        const Event &e = g_events[i % kEvents];
        fprintf(f, "    %9.3f %-7s CHCR %08x MADR %08x TADR %08x QWC %04x%s\n", (now - e.ns) / 1e6, kChannels[e.ch].name,
                e.chcr, e.madr, e.tadr, e.qwc, e.done ? "" : " (running)");
    }
}

uint64_t port_dma_kicks(int ch)
{
    return g_kicks[ch];
}

const char *port_dma_channel_name(int ch)
{
    return kChannels[ch].name;
}
