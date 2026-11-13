// The PS2's memory map on the PC.
//
// The game reads and writes EE hardware registers by their PS2 addresses
// (`*(volatile int *)0x10000000` is T0_COUNT) and keeps pointers in 32-bit
// ints (`(unsigned int)p | 0x20000000`, `return (int)this`, DMA packets). So
// before any game code runs (static constructors included):
// - the register windows and the scratchpad are mapped at their PS2
//   addresses: 0x10000000 (timers, DMA, VIF, GIF, IPU, INTC), 0x12000000 (GS
//   privileged registers), 0x70000000 (16 KB scratchpad);
// - the process runs without address randomization (re-executed once), and
//   glibc malloc is held to its brk heap, which lies above the non-PIE
//   executable's .bss well below 4 GB (no mmap'd chunks, one arena), so heap
//   pointers survive a round trip through 32 bits; ports/pc/main.cpp runs the
//   game on a stack below 4 GB for the same reason.
// Timer 0 counts at BUSCLK/256 (1.7067 us per tick) in a host thread, which
// calls the game's overflow handler (INTC cause 9) when the 16-bit counter
// wraps, and raises the GS interrupt (INTC cause 0: the game's VSync handler)
// at NTSC's 59.94 Hz. PORT_DETERMINISTIC replaces the thread with virtual
// time (below). The game's waits for a vblank or a timer mark sleep
// (port_vsync_wait, port_time_spin) instead of spinning.
#include "port.h"
#include "port_os.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <mutex>
#include <thread>
#ifdef __linux__
#include <fcntl.h>
#include <malloc.h>
#include <sys/personality.h>
#include <unistd.h>
#endif

namespace {

struct Region {
    uintptr_t addr;
    size_t size;
    const char *name;
};

const Region kRegions[] = {
    {0x10000000, 0x10000, "EE registers"},
    {0x12000000, 0x2000, "GS privileged registers"},
    {0x70000000, 0x4000, "scratchpad"},
};

void MapRegion(const Region &r)
{
    if (!port_map_at(r.addr, r.size))
        port_fatal("can't map the %s at 0x%lx", r.name, (unsigned long)r.addr);
}

#ifdef __linux__
// The brk heap's start is randomized by up to 1 GB on x86-64, which can put
// it past 256 MB (the game masks pointers with 0x0FFFFFFF): run without
// address randomization, re-executing once to get it.
void NoAslr()
{
    int pers = personality(0xFFFFFFFF);
    if (pers == -1 || (pers & ADDR_NO_RANDOMIZE))
        return;
    if (personality(pers | ADDR_NO_RANDOMIZE) == -1)
        return;
    static char path[4096];
    ssize_t n = readlink("/proc/self/exe", path, sizeof path - 1);
    if (n <= 0)
        return;
    path[n] = 0;
    // argv from /proc/self/cmdline (main hasn't run yet)
    static char args[65536];
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd < 0)
        return;
    ssize_t len = read(fd, args, sizeof args - 1);
    close(fd);
    if (len <= 0)
        return;
    static char *argv[256];
    int argc = 0;
    for (ssize_t i = 0; i < len && argc < 255; i += strlen(args + i) + 1)
        argv[argc++] = args + i;
    argv[argc] = nullptr;
    execv(path, argv);
}
#endif

// Before every other static constructor of the port and the game. (macOS
// and Windows: the game's heap is ports/pc/lowheap.cpp's, which maps its
// region at the first allocation, wherever that comes from.)
__attribute__((constructor(101))) void HwInit()
{
#ifdef __linux__
    NoAslr();
    mallopt(M_MMAP_MAX, 0);
    mallopt(M_ARENA_MAX, 1);
#endif
    setvbuf(stdout, nullptr, _IOLBF, 0);  // the game's printfs among the port's logs
    for (const Region &r : kRegions)
        MapRegion(r);
}

// INTC handlers (AddIntcHandler), by cause.
const int kCauses = 16;
int (*g_handlers[kCauses])(int);
std::atomic<bool> g_enabled[kCauses];
std::atomic<uint64_t> g_vblanks;
// Interrupts delivered and handlers running now, by cause (watchdog dumps).
std::atomic<uint64_t> g_raised[kCauses];
std::atomic<int> g_running[kCauses];

// Handlers run on the thread that raises them (timer, VIF1's interrupt
// thread, the game thread in deterministic mode), one at a time: the EE
// runs a handler with interrupts off, so they never overlap; and
// DisableIntc/RemoveIntcHandler return only once a running handler is done.
std::recursive_mutex g_intcLock;

void RunHandler(int cause)
{
    if (!g_enabled[cause] || !g_handlers[cause])
        return;
    std::lock_guard<std::recursive_mutex> lock(g_intcLock);
    int (*h)(int) = g_handlers[cause];
    if (!g_enabled[cause] || !h)
        return;
    g_raised[cause].fetch_add(1, std::memory_order_relaxed);
    g_running[cause].fetch_add(1, std::memory_order_relaxed);
    h(cause);
    g_running[cause].fetch_sub(1, std::memory_order_relaxed);
}

const uint64_t kFrameNs = 1001000000ull / 60; // 59.94 Hz

// PORT_DETERMINISTIC=N (golden runs, tools/port/golden.py): no timer thread;
// time is virtual and moves only when the game waits for a vblank
// (WaitForVSync, sceGsSyncV: N vblanks pass at once, default 2, the game's
// 30 Hz) or spins on the timer (port_time_spin: one vblank). Timer 0, its
// overflow interrupt and the vblank interrupt then run on the game thread
// at those points, so a run does the same thing whatever the host's speed.
const long g_detVblanks = [] {
    const char *v = getenv("PORT_DETERMINISTIC");
    if (!v || !*v || !strcmp(v, "0"))
        return 0L;
    long n = atol(v);
    return n > 0 ? n : 2L;
}();

uint64_t g_lastWraps;

// Timer 0 at ns: T0_COUNT and its overflow interrupts.
void SetTimer(uint64_t ns)
{
    volatile uint32_t *t0Count = (volatile uint32_t *)0x10000000;
    uint64_t ticks = ns * 3 / 5120; // 1 tick = 5120/3 ns
    *t0Count = (uint32_t)(ticks & 0xFFFF);
    uint64_t wraps = ticks >> 16;
    for (; g_lastWraps < wraps; g_lastWraps++)
        RunHandler(9);
}

void Vblank()
{
    g_vblanks++;
    RunHandler(0);
    port_futex_wake(&g_vblanks, true);
}

// Deterministic mode: the next vblank, now, once the DMA channels are idle
// (handlers that look at them, LoadingIconInterrupt, see the same thing in
// every run).
void VirtualVblank()
{
    port_dma_wait_idle();
    SetTimer((g_vblanks + 1) * kFrameNs);
    Vblank();
    port_audio_vblank();
}

void TimerThread()
{
    port_thread_name("timer");
    // PORT_TEST_VBLANK_US=N (testing races with the vblank handler): a
    // vblank every N us (at least the 250 us tick) instead of 59.94 Hz
    const char *test = getenv("PORT_TEST_VBLANK_US");
    const uint64_t frameNs = test && atol(test) > 0 ? (uint64_t)atol(test) * 1000 : kFrameNs;
    auto start = std::chrono::steady_clock::now();
    uint64_t lastFrame = 0;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::microseconds(250));
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start)
                      .count();
        SetTimer((uint64_t)ns);
        uint64_t frame = (uint64_t)ns / frameNs;
        if (frame != lastFrame) {
            lastFrame = frame;
            Vblank();
        }
    }
}

} // namespace

void port_hw_start(void)
{
    port_watch_futex(&g_vblanks, "vblanks");
    if (g_detVblanks)
        port_log("deterministic: virtual time, %ld vblanks per vsync wait", g_detVblanks);
    else
        std::thread(TimerThread).detach();
}

bool port_deterministic(void)
{
    return g_detVblanks != 0;
}

// The game waits for a vblank (WaitForVSync's loop, sceGsSyncV): sleep until
// the next one (a futex on the vblank count, at most 2 ms: the vblank may
// have come between the caller's check and this call), or in deterministic
// mode let N pass now.
extern "C" void port_vsync_wait(void)
{
    if (g_detVblanks) {
        for (long i = 0; i < g_detVblanks; i++)
            VirtualVblank();
        return;
    }
    uint32_t now = (uint32_t)g_vblanks.load();
    port_futex_wait(&g_vblanks, now, 2000000);
}

// The game spins until the timer passes a mark (JetskiLoadManager's 5 s
// pause, JetskiMPInitEmpty's delay): sleep a millisecond, or in
// deterministic mode let a vblank pass.
extern "C" void port_time_spin(void)
{
    if (g_detVblanks) {
        VirtualVblank();
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

uint64_t port_vblanks(void)
{
    return g_vblanks;
}

void port_raise_intc(int cause)
{
    if (cause >= 0 && cause < kCauses)
        RunHandler(cause);
}

void port_hw_debug(FILE *f)
{
    fprintf(f, "INTC:");
    for (int c = 0; c < kCauses; c++)
        if (g_handlers[c] || g_raised[c])
            fprintf(f, " cause %d %s%s raised %llu running %d;", c, g_handlers[c] ? "handler" : "none",
                    g_enabled[c] ? " enabled" : " disabled", (unsigned long long)g_raised[c].load(), g_running[c].load());
    fprintf(f, "\n");
}

extern "C" {

int AddIntcHandler(int cause, int (*handler)(int), int next)
{
    if (cause < 0 || cause >= kCauses)
        port_fatal("AddIntcHandler(%d)", cause);
    {
        std::lock_guard<std::recursive_mutex> lock(g_intcLock);
        g_handlers[cause] = handler;
    }
    if (cause != 9 && cause != 0 && cause != 5)
        port_log("INTC cause %d handler added (not raised on the PC)", cause);
    return cause + 1;
}

int RemoveIntcHandler(int cause, int id)
{
    std::lock_guard<std::recursive_mutex> lock(g_intcLock);
    if (cause >= 0 && cause < kCauses)
        g_handlers[cause] = nullptr;
    return 0;
}

int EnableIntc(int cause)
{
    if (cause >= 0 && cause < kCauses)
        g_enabled[cause] = true;
    return 1;
}

// libgraph: wait for the next vertical blank (mode 0). Returns the field,
// always 0 (progressive).
int sceGsSyncV(int mode)
{
    if (g_detVblanks) {
        VirtualVblank();
        return 0;
    }
    uint64_t now = g_vblanks;
    while (g_vblanks == now) {
        port_futex_wait(&g_vblanks, (uint32_t)now, 20000000);
    }
    return 0;
}

int DisableIntc(int cause)
{
    std::lock_guard<std::recursive_mutex> lock(g_intcLock);
    if (cause >= 0 && cause < kCauses)
        g_enabled[cause] = false;
    return 1;
}

} // extern "C"
