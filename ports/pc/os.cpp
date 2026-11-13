// The host OS under the port (include/port_os.h): Linux, macOS, Windows.
//
// What each gives the game's memory model (ports/pc/hw.cpp: every address the
// game sees fits in 32 bits, its heap and globals in 28):
// - Linux: a non-PIE executable at 0x400000, the brk heap after it,
//   MAP_32BIT for stacks.
// - macOS: x86-64 only (Rosetta on Apple silicon: arm64 processes can't map
//   anything below 4 GB). Linked with -pagezero_size 0x1000 -image_base
//   0x400000 -no_pie; low addresses are then free and asked for by hint.
// - macOS on Apple silicon, native: an arm64 process can't map anything
//   below 4 GB at all, so the game's memory is the 4 GB window at
//   PORT_GUEST_BASE (include/port_ptr32.h), which is where the executable
//   loads once address randomization is off (port_os_fix_address_space
//   re-executes for that); "below 4 GB" means "in the window" there.
// - Windows: linked with --image-base 0x400000 and no dynamic base;
//   VirtualAlloc at chosen addresses. A thread's stack can't be placed, so
//   a low-stack thread switches to one it mapped (and tells the TEB).
#include "port_os.h"

#include "port.h"
#include "port_ptr32.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <csignal>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#if defined(__linux__)
#include <climits>
#include <linux/futex.h>
#include <sys/syscall.h>
#elif defined(__APPLE__)
#include <crt_externs.h>
#include <mach-o/dyld.h>
#include <spawn.h>
#include <os/os_sync_wait_on_address.h>
#endif

// ---- futex ----------------------------------------------------------------

void port_futex_wait(const void *word, uint32_t expected, long long ns)
{
#if defined(__linux__)
    struct timespec ts = {(time_t)(ns / 1000000000), (long)(ns % 1000000000)};
    syscall(SYS_futex, (uint32_t *)word, FUTEX_WAIT_PRIVATE, expected, ns ? &ts : nullptr, nullptr, 0);
#elif defined(__APPLE__)
    if (ns)
        os_sync_wait_on_address_with_timeout((void *)word, expected, 4, OS_SYNC_WAIT_ON_ADDRESS_NONE,
                                             OS_CLOCK_MACH_ABSOLUTE_TIME, (uint64_t)ns);
    else
        os_sync_wait_on_address((void *)word, expected, 4, OS_SYNC_WAIT_ON_ADDRESS_NONE);
#else
    // WaitOnAddress counts in milliseconds: round up, so a short wait still
    // sleeps (the callers loop on their condition).
    DWORD ms = ns ? (DWORD)((ns + 999999) / 1000000) : INFINITE;
    WaitOnAddress((volatile VOID *)word, &expected, 4, ms);
#endif
}

void port_futex_wake(const void *word, bool all)
{
#if defined(__linux__)
    syscall(SYS_futex, (uint32_t *)word, FUTEX_WAKE_PRIVATE, all ? INT_MAX : 1, nullptr, nullptr, 0);
#elif defined(__APPLE__)
    if (all)
        os_sync_wake_by_address_all((void *)word, 4, OS_SYNC_WAKE_BY_ADDRESS_NONE);
    else
        os_sync_wake_by_address_any((void *)word, 4, OS_SYNC_WAKE_BY_ADDRESS_NONE);
#else
    if (all)
        WakeByAddressAll((PVOID)word);
    else
        WakeByAddressSingle((PVOID)word);
#endif
}

// ---- memory ---------------------------------------------------------------

namespace {

size_t RoundUp(size_t n, size_t to)
{
    return (n + to - 1) & ~(to - 1);
}

} // namespace

void *port_map(size_t size)
{
#if defined(_WIN32)
    return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#elif defined(__APPLE__)
    // Above 4 GB by hint: with a small __PAGEZERO the kernel's own choice
    // starts at the bottom, in the room the game's data needs.
    static std::atomic<uintptr_t> next{0x200000000ull};
    size_t step = RoundUp(size, 1 << 20) + (1 << 20);
    void *p = mmap((void *)next.fetch_add(step), size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    return p == MAP_FAILED ? nullptr : p;
#else
    void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return p == MAP_FAILED ? nullptr : p;
#endif
}

bool port_map_at(uintptr_t addr, size_t size)
{
#if defined(_WIN32)
    return VirtualAlloc((void *)addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE) == (void *)addr;
#elif defined(__linux__)
    void *p = mmap((void *)addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p == (void *)addr)
        return true;
    if (p != MAP_FAILED)
        munmap(p, size);
    return false;
#else
    // No MAP_FIXED (it would replace what is there): a hint, kept only if
    // the kernel took it.
    void *p = mmap((void *)addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == (void *)addr)
        return true;
    if (p != MAP_FAILED)
        munmap(p, size);
    return false;
#endif
}

void *port_map_low(size_t size)
{
#if defined(__linux__)
    void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_32BIT, -1, 0);
    return p == MAP_FAILED ? nullptr : p;
#else
    // Between the EE register windows' neighbourhood and the scratchpad's
    // (0x10000000, 0x12000000, 0x70000000), then above it, by first fit.
    // (offsets in the game's window: PORT_GUEST_BASE is 0 on x86-64)
    static std::mutex lock;
    static uintptr_t next = 0x20000000;
    std::lock_guard<std::mutex> g(lock);
    size = RoundUp(size, 1 << 16);
    for (uintptr_t a = next; a + size <= 0xF0000000u; a += 1 << 24) {
        if (a < 0x70010000u && a + size > 0x6FF00000u) {
            a = 0x71000000u - (1 << 24);
            continue;
        }
        if (port_map_at((uintptr_t)(PORT_GUEST_BASE + a), size)) {
            next = RoundUp(a + size, 1 << 16);
            return (void *)(uintptr_t)(PORT_GUEST_BASE + a);
        }
    }
    return nullptr;
#endif
}

void port_unmap(void *p, size_t size)
{
#if defined(_WIN32)
    VirtualFree(p, 0, MEM_RELEASE);
#else
    munmap(p, size);
#endif
}

void port_os_fix_address_space(void)
{
#if defined(__APPLE__) && defined(__aarch64__)
    // The executable must sit at the window's start: run without address
    // randomization, re-executing once to get that.
    if (_dyld_get_image_vmaddr_slide(0) != 0) {
        if (getenv("PORT_NOASLR_REEXEC"))
            port_fatal("the executable is not at its link address even without address randomization");
        char path[4096];
        uint32_t size = sizeof path;
        if (_NSGetExecutablePath(path, &size) != 0)
            port_fatal("the executable's path is too long");
        setenv("PORT_NOASLR_REEXEC", "1", 1);
        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETEXEC | 0x0100 /* _POSIX_SPAWN_DISABLE_ASLR */);
        posix_spawn(nullptr, path, nullptr, &attr, *_NSGetArgv(), *_NSGetEnviron());
        port_fatal("can't re-execute %s without address randomization", path);
    }
    if ((uintptr_t)_dyld_get_image_header(0) != PORT_GUEST_BASE)
        port_fatal("the executable is at %p, not at the game's window (0x%llx)", (void *)_dyld_get_image_header(0),
                   (unsigned long long)PORT_GUEST_BASE);
#endif
}

// ---- threads --------------------------------------------------------------

#if defined(_WIN32)

extern "C" void *port_call_on_stack(void *(*fn)(void *), void *arg, void *top);
// rcx fn, rdx arg, r8 the new stack's top (16-aligned)
__asm__(".text\n"
        ".globl port_call_on_stack\n"
        "port_call_on_stack:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    movq %r8, %rsp\n"
        "    subq $32, %rsp\n"
        "    movq %rcx, %rax\n"
        "    movq %rdx, %rcx\n"
        "    callq *%rax\n"
        "    movq %rbp, %rsp\n"
        "    popq %rbp\n"
        "    retq\n");

namespace {

struct LowStart {
    void *(*fn)(void *);
    void *arg;
    char *stack;
    size_t size;
};

DWORD WINAPI LowThread(LPVOID p)
{
    LowStart s = *(LowStart *)p;
    HeapFree(GetProcessHeap(), 0, p);
    // The TEB's stack bounds (stack probes, exception dispatch and
    // GetCurrentThreadStackLimits go by them) and its DeallocationStack.
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    void **dealloc = (void **)((char *)tib + 0x1478);
    void *base = tib->StackBase, *limit = tib->StackLimit, *d = *dealloc;
    tib->StackBase = s.stack + s.size;
    tib->StackLimit = s.stack;
    *dealloc = s.stack;
    port_call_on_stack(s.fn, s.arg, s.stack + s.size);
    tib->StackBase = base;
    tib->StackLimit = limit;
    *dealloc = d;
    return 0;
}

} // namespace

void *port_thread_start_low(void *(*fn)(void *), void *arg, size_t stackSize)
{
    char *stack = (char *)port_map_low(stackSize);
    if (!stack)
        return nullptr;
    LowStart *s = (LowStart *)HeapAlloc(GetProcessHeap(), 0, sizeof(LowStart));
    *s = {fn, arg, stack, stackSize};
    return CreateThread(nullptr, 1 << 16, LowThread, s, 0, nullptr);
}

void port_thread_join(void *handle)
{
    WaitForSingleObject((HANDLE)handle, INFINITE);
    CloseHandle((HANDLE)handle);
}

void port_os_thread_name(const char *name)
{
    wchar_t w[64];
    int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, w, 64);
    if (n > 0)
        SetThreadDescription(GetCurrentThread(), w);
}

#else

void *port_thread_start_low(void *(*fn)(void *), void *arg, size_t stackSize)
{
    void *stack = port_map_low(stackSize);
    if (!stack)
        return nullptr;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, stack, stackSize);
    pthread_t *t = new pthread_t;
    if (pthread_create(t, &attr, fn, arg) != 0) {
        delete t;
        return nullptr;
    }
    return t;
}

void port_thread_join(void *handle)
{
    pthread_t *t = (pthread_t *)handle;
    pthread_join(*t, nullptr);
    delete t;
}

void port_os_thread_name(const char *name)
{
#if defined(__APPLE__)
    pthread_setname_np(name);
#else
    pthread_setname_np(pthread_self(), name);
#endif
}

#endif

// ---- files, process -------------------------------------------------------

const char *port_data_dir(void)
{
    static std::string dir;
    static std::once_flag once;
    std::call_once(once, [] {
#if defined(_WIN32)
        if (const char *a = getenv("APPDATA"); a && *a)
            dir = std::string(a) + "/splashdown";
#elif defined(__APPLE__)
        if (const char *h = getenv("HOME"); h && *h)
            dir = std::string(h) + "/Library/Application Support/splashdown";
#else
        if (const char *x = getenv("XDG_DATA_HOME"); x && *x)
            dir = std::string(x) + "/splashdown";
        else if (const char *h = getenv("HOME"); h && *h)
            dir = std::string(h) + "/.local/share/splashdown";
#endif
        for (char &c : dir)
            if (c == '\\')
                c = '/';
    });
    return dir.c_str();
}

const char *port_exe_dir(void)
{
    static std::string dir;
    static std::once_flag once;
    std::call_once(once, [] {
        char path[4096] = "";
#if defined(_WIN32)
        GetModuleFileNameA(nullptr, path, sizeof path - 1);
#elif defined(__APPLE__)
        uint32_t size = sizeof path - 1;
        if (_NSGetExecutablePath(path, &size) != 0)
            path[0] = 0;
#else
        ssize_t n = readlink("/proc/self/exe", path, sizeof path - 1);
        path[n > 0 ? n : 0] = 0;
#endif
        dir = path;
        for (char &c : dir)
            if (c == '\\')
                c = '/';
        size_t k = dir.rfind('/');
        dir = k == std::string::npos ? "." : dir.substr(0, k);
    });
    return dir.c_str();
}

void port_setenv(const char *name, const char *value)
{
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void port_show_error(const char *text)
{
#if defined(_WIN32)
    MessageBoxA(nullptr, text, "Splashdown", MB_OK | MB_ICONERROR);
#else
    (void)text;
#endif
}

int port_mkdir(const char *path)
{
#if defined(_WIN32)
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

void port_os_exit(int code)
{
#if defined(_WIN32)
    TerminateProcess(GetCurrentProcess(), (UINT)code);
    for (;;)
        Sleep(1000);
#else
    _exit(code);
#endif
}

void port_debug_break(void)
{
#if defined(_WIN32)
    if (IsDebuggerPresent())
        DebugBreak();
#else
    raise(SIGTRAP);
#endif
}

long long port_pread(int fd, void *buf, size_t bytes, uint64_t offset)
{
#if defined(_WIN32)
    OVERLAPPED ov = {};
    ov.Offset = (DWORD)offset;
    ov.OffsetHigh = (DWORD)(offset >> 32);
    DWORD got = 0;
    DWORD want = bytes > 0x40000000 ? 0x40000000 : (DWORD)bytes;
    if (!ReadFile((HANDLE)_get_osfhandle(fd), buf, want, &got, &ov) && GetLastError() != ERROR_HANDLE_EOF)
        return -1;
    return got;
#else
    return pread(fd, buf, bytes, (off_t)offset);
#endif
}
