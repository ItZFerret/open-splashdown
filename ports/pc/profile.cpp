// PORT_PROFILE=out.txt: a sampling profiler (no perf on the dev machine).
// A thread sends SIGPROF to every thread of the process each millisecond;
// the handler records the interrupted PC and a few return addresses. At
// exit (port_exit) the samples go to out.txt as "count thread pc caller..."
// lines; addr2line symbolizes them.
// PORT_PROFILE_SECONDS=N exits (and writes them) after N seconds;
// PORT_PROFILE_START=N starts sampling N seconds in.
#ifdef __linux__
#include "port.h"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <execinfo.h>
#include <map>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <thread>
#include <ucontext.h>
#include <unistd.h>
#include <vector>

namespace {

const int kDepth = 8;
struct Sample {
    int tid;
    int n;
    void *pc[kDepth];
};

const int kMax = 1 << 20;
Sample *g_samples;
std::atomic<int> g_count{0};
int g_self;

void OnProf(int, siginfo_t *, void *uc)
{
    int k = g_count.fetch_add(1, std::memory_order_relaxed);
    if (k >= kMax)
        return;
    Sample &s = g_samples[k];
    s.tid = (int)syscall(SYS_gettid);
    void *frames[kDepth + 3];
    int n = backtrace(frames, kDepth + 3);
    // frames[0] is this handler, [1] the signal trampoline; the interrupted
    // PC comes from the context (the caller of a leaf may be missing)
    s.pc[0] = (void *)((ucontext_t *)uc)->uc_mcontext.gregs[REG_RIP];
    int m = 1;
    for (int i = 2; i < n && m < kDepth; i++)
        s.pc[m++] = frames[i];
    s.n = m;
}

void Sampler()
{
    port_thread_name("profiler");
    g_self = (int)syscall(SYS_gettid);
    int pid = getpid();
    const char *secs = getenv("PORT_PROFILE_SECONDS");
    if (const char *start = getenv("PORT_PROFILE_START"))
        std::this_thread::sleep_for(std::chrono::seconds(atoi(start)));
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(secs ? atoi(secs) : 1 << 30);
    for (;;) {
        if (std::chrono::steady_clock::now() > end) {
            port_log("profile: PORT_PROFILE_SECONDS reached");
            port_exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        DIR *d = opendir("/proc/self/task");
        if (!d)
            continue;
        while (dirent *e = readdir(d)) {
            int tid = atoi(e->d_name);
            if (tid > 0 && tid != g_self)
                syscall(SYS_tgkill, pid, tid, SIGPROF);
        }
        closedir(d);
    }
}

const char *g_out = getenv("PORT_PROFILE");

struct Start {
    Start()
    {
        if (!g_out)
            return;
        // outside the game's brk heap (72 MB)
        g_samples = (Sample *)mmap(nullptr, kMax * sizeof(Sample), PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        void *warm[4];
        backtrace(warm, 4);  // loads libgcc before the first signal
        struct sigaction sa = {};
        sa.sa_sigaction = OnProf;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigaction(SIGPROF, &sa, nullptr);
        std::thread(Sampler).detach();
    }
} g_start;

} // namespace

void port_profile_dump()
{
    if (!g_out)
        return;
    int n = std::min(g_count.load(), kMax);
    std::map<int, int> perThread;
    std::map<std::vector<void *>, int> stacks;
    std::map<int, std::string> names;
    for (int i = 0; i < n; i++) {
        const Sample &s = g_samples[i];
        perThread[s.tid]++;
        std::vector<void *> key(s.pc, s.pc + s.n);
        key.insert(key.begin(), (void *)(intptr_t)s.tid);
        stacks[key]++;
    }
    FILE *f = fopen(g_out, "w");
    if (!f)
        return;
    fprintf(f, "# samples %d\n", n);
    for (auto &[tid, c] : perThread)
        fprintf(f, "# thread %d %d\n", tid, c);
    for (auto &[key, c] : stacks) {
        fprintf(f, "%d", c);
        for (void *p : key)
            fprintf(f, " %p", p);
        fputc('\n', f);
    }
    fclose(f);
    port_log("profile: %d samples to %s", n, g_out);
}

#endif // __linux__
