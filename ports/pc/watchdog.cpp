// The hang watchdog.
//
// PORT_WATCHDOG=N (seconds; default 10, 0 = off): when no frame is finished
// and no Flip happens for N seconds, write every thread's state and the
// hardware's to stderr (or PORT_WATCHDOG_DUMP=file): per thread its name,
// scheduler state, the syscall it is blocked in (futex words named), a
// backtrace taken on the thread itself by a signal (raw addresses for
// addr2line, then backtrace_symbols), the inline-asm statement it is
// interpreting and the bc0f wait it is in; then the DMA channels, the DMA
// thread's last transfers, VIF1, the GS queue and workers, the interrupt
// handlers in flight and the game's wait flags. PORT_WATCHDOG_EXIT=1 exits
// with status 86 after the dump; otherwise the run
// goes on and the watchdog re-arms once frames move again. SIGUSR2 dumps
// on demand (`kill -USR2 <pid>`), even with PORT_WATCHDOG=0.
#ifdef __linux__
#include "port.h"

#include "gs/gs.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <thread>
#include <ucontext.h>
#include <unistd.h>

__thread const char *port_tls_asm;
__thread uint32_t port_tls_bc0f;

// The game's wait flags (shared source).
extern volatile int LockRenderQueueState;
extern volatile int VSyncComplete;

namespace {

struct Named {
    std::atomic<const void *> addr;
    const char *name;
};
Named g_named[64];
std::atomic<int> g_nNamed;

const char *FutexName(uintptr_t a)
{
    int n = std::min(g_nNamed.load(), 64);
    for (int i = 0; i < n; i++)
        if ((uintptr_t)g_named[i].addr.load() == a)
            return g_named[i].name;
    return nullptr;
}

int g_dumpFd = 2;
std::atomic<int> g_ack;
int g_sig;

void Out(const char *s)
{
    size_t n = strlen(s);
    while (n) {
        ssize_t w = write(g_dumpFd, s, n);
        if (w <= 0)
            return;
        s += w;
        n -= (size_t)w;
    }
}

// On the thread being dumped.
void OnDump(int, siginfo_t *, void *uc)
{
    int saved = errno;
    char buf[1024];
    void *pc = (void *)((ucontext_t *)uc)->uc_mcontext.gregs[REG_RIP];
    snprintf(buf, sizeof buf, "  pc %p", pc);
    Out(buf);
    if (port_tls_bc0f)
        snprintf(buf, sizeof buf, ", in bc0f wait (D_PCR %03x)", port_tls_bc0f & 0x3FF), Out(buf);
    if (port_tls_asm) {
        const char *t = port_tls_asm;
        snprintf(buf, sizeof buf, ", asm \"%.160s\"", t);
        for (char *c = buf; *c; c++)
            if (*c == '\n' || *c == '\t')
                *c = ' ';
        Out(buf);
    }
    Out("\n  bt:");
    void *frames[64];
    int n = backtrace(frames, 64);
    // frames[0] is this handler, [1] the signal trampoline
    for (int i = 2; i < n; i++) {
        snprintf(buf, sizeof buf, " %p", frames[i]);
        Out(buf);
    }
    Out("\n");
    if (n > 2)
        backtrace_symbols_fd(frames + 2, n - 2, g_dumpFd);
    g_ack.store(1, std::memory_order_release);
    errno = saved;
}

std::string ReadSmall(const char *path)
{
    char buf[512];
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return "";
    ssize_t n = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n <= 0)
        return "";
    buf[n] = 0;
    if (n && buf[n - 1] == '\n')
        buf[n - 1] = 0;
    return buf;
}

void DumpThreads(FILE *f, int self)
{
    DIR *d = opendir("/proc/self/task");
    if (!d)
        return;
    int pid = getpid();
    while (dirent *e = readdir(d)) {
        int tid = atoi(e->d_name);
        if (tid <= 0)
            continue;
        char path[64];
        snprintf(path, sizeof path, "/proc/self/task/%d/comm", tid);
        std::string comm = ReadSmall(path);
        snprintf(path, sizeof path, "/proc/self/task/%d/stat", tid);
        std::string stat = ReadSmall(path);
        char state = '?';
        size_t rp = stat.rfind(')');
        if (rp != std::string::npos && rp + 2 < stat.size())
            state = stat[rp + 2];
        snprintf(path, sizeof path, "/proc/self/task/%d/syscall", tid);
        std::string sc = ReadSmall(path);
        std::string what;
        long nr = -2;
        unsigned long a0 = 0, a1 = 0, a2 = 0;
        if (sscanf(sc.c_str(), "%ld %lx %lx %lx", &nr, &a0, &a1, &a2) >= 1) {
            if (nr == SYS_futex) {
                const char *name = FutexName(a0);
                char b[160];
                snprintf(b, sizeof b, "futex(%s%s0x%lx, op %lu, val %lu)", name ? name : "", name ? " " : "", a0,
                         a1 & 0x7F, a2);
                what = b;
            } else if (nr == -1) {
                what = "running";
            } else {
                what = "syscall " + std::to_string(nr);
            }
        }
        fprintf(f, "thread %d %s [%c] %s%s\n", tid, comm.c_str(), state, what.c_str(), tid == self ? " (watchdog)" : "");
        fflush(f);
        if (tid == self)
            continue;
        g_ack.store(0);
        syscall(SYS_tgkill, pid, tid, g_sig);
        for (int k = 0; k < 500 && !g_ack.load(std::memory_order_acquire); k++)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (!g_ack.load())
            fprintf(f, "  (no answer to the signal)\n");
    }
    closedir(d);
}

std::atomic<int> g_dumping;

void Dump(const char *why)
{
    if (g_dumping.exchange(1))
        return;
    FILE *f = stderr;
    if (const char *p = getenv("PORT_WATCHDOG_DUMP")) {
        if (FILE *o = fopen(p, "a"))
            f = o;
    }
    fflush(stderr);
    g_dumpFd = fileno(f);
    fprintf(f, "==== watchdog: %s ====\n", why);
    fprintf(f, "vblanks %llu, frames %llu, flips %llu, finishes %llu, prims %llu, GS waits %llu\n",
            (unsigned long long)port_vblanks(), (unsigned long long)gs_frame_seq(),
            (unsigned long long)gs_flip_count(), (unsigned long long)gs_finish_count(),
            (unsigned long long)gs_prim_count(), (unsigned long long)gs_barrier_count());
    fprintf(f, "game: LockRenderQueueState %d, VSyncComplete %d\n", LockRenderQueueState, VSyncComplete);
    port_hw_debug(f);
    port_dma_debug(f);
    vif1_debug(f);
    gs_debug(f);
    fflush(f);
    DumpThreads(f, (int)syscall(SYS_gettid));
    // the registers again: did anything move while the threads were dumped?
    fprintf(f, "-- after the thread dump --\n");
    port_dma_debug(f);
    fprintf(f, "==== end of watchdog dump ====\n");
    fflush(f);
    if (f != stderr)
        fclose(f);
    g_dumpFd = 2;
    g_dumping.store(0);
}

std::atomic<int> g_request;

void OnUsr2(int)
{
    g_request.store(1);
}

void Watch(long seconds, bool exitAfter)
{
    port_thread_name("watchdog");
    uint64_t last = ~0ull;
    auto since = std::chrono::steady_clock::now();
    bool fired = false;
    long warned = 0;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (g_request.exchange(0))
            Dump("SIGUSR2");
        if (seconds <= 0)
            continue;
        uint64_t progress = gs_frame_seq() + gs_flip_count();
        auto now = std::chrono::steady_clock::now();
        if (progress != last) {
            if (fired)
                port_log("watchdog: frames move again (frame %llu)", (unsigned long long)gs_frame_seq());
            last = progress;
            since = now;
            fired = false;
            warned = 0;
            continue;
        }
        double s = std::chrono::duration<double>(now - since).count();
        if (!fired && s >= seconds / 2.0 && !warned) {
            warned = 1;
            port_log("watchdog: no frame for %.1f s (frame %llu, vblank %llu)", s,
                     (unsigned long long)gs_frame_seq(), (unsigned long long)port_vblanks());
        }
        if (!fired && s >= seconds) {
            fired = true;
            char why[160];
            snprintf(why, sizeof why, "no frame for %.1f s at frame %llu (vblank %llu)", s,
                     (unsigned long long)gs_frame_seq(), (unsigned long long)port_vblanks());
            port_log("watchdog: %s: dumping", why);
            Dump(why);
            if (exitAfter) {
                port_log("watchdog: exiting (PORT_WATCHDOG_EXIT)");
                fflush(nullptr);
                _exit(86);
            }
        }
    }
}

} // namespace

void port_thread_name(const char *name)
{
    pthread_setname_np(pthread_self(), name);
    if (strcmp(name, "game"))
        port_heap_this_thread();
}

void port_watch_futex(const void *addr, const char *name)
{
    int k = g_nNamed.fetch_add(1);
    if (k >= 64)
        return;
    g_named[k].name = name;
    g_named[k].addr.store(addr);
}

void port_watchdog_dump(const char *why)
{
    Dump(why);
}

void port_watchdog_start(void)
{
    g_sig = SIGRTMIN + 5;
    struct sigaction sa = {};
    sa.sa_sigaction = OnDump;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(g_sig, &sa, nullptr);
    struct sigaction su = {};
    su.sa_handler = OnUsr2;
    su.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &su, nullptr);
    void *warm[4];
    backtrace(warm, 4);  // loads libgcc before the first signal
    const char *v = getenv("PORT_WATCHDOG");
    long seconds = v && *v ? atol(v) : 10;
    bool exitAfter = getenv("PORT_WATCHDOG_EXIT") && atoi(getenv("PORT_WATCHDOG_EXIT"));
    std::thread([seconds, exitAfter] { Watch(seconds, exitAfter); }).detach();
}

#endif // __linux__
