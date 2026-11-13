// What the port takes from the host OS (ports/pc/os.cpp): Linux, macOS
// (x86-64, under Rosetta on Apple silicon) and Windows (x86-64, MinGW).
#ifndef PORT_OS_H
#define PORT_OS_H

#include <cstddef>
#include <cstdint>

// A futex on a 32-bit word: sleep while *word == expected, for at most ns
// nanoseconds (0: no limit). May wake early.
void port_futex_wait(const void *word, uint32_t expected, long long ns);
// Wake the threads (all, or one) sleeping on word.
void port_futex_wake(const void *word, bool all);

// Zeroed read-write pages: anywhere; below 4 GB; exactly at addr (false if
// the range is taken). nullptr when there is no room.
void *port_map(size_t size);
void *port_map_low(size_t size);
bool port_map_at(uintptr_t addr, size_t size);
// Give a port_map* mapping back.
void port_unmap(void *p, size_t size);

// Before anything else at start-up: put the process where the game's
// 32-bit addresses work (arm64 macOS: no address randomization; elsewhere
// the executable's link options and ports/pc/hw.cpp do it).
void port_os_fix_address_space(void);

// A thread whose stack lies below 4 GB (the game keeps addresses of locals
// in 32 bits). Returns a handle for port_thread_join, or nullptr.
void *port_thread_start_low(void *(*fn)(void *), void *arg, size_t stackSize);
void port_thread_join(void *handle);

// The host's name for this thread (debuggers, ps).
void port_os_thread_name(const char *name);

// The directory saves go under when PORT_MEMCARD_DIR isn't set (it ends
// without a separator), or "" if the host gives none.
const char *port_data_dir(void);

// The directory the executable is in (no trailing separator), or ".".
const char *port_exe_dir(void);
// Set an environment variable for this process.
void port_setenv(const char *name, const char *value);
// A fatal error's text, shown to someone who started the game by double
// click (Windows: a message box; elsewhere stderr is enough).
void port_show_error(const char *text);

// mkdir (one level; an existing directory is not an error to the callers).
int port_mkdir(const char *path);

// End the process now: no atexit handlers, no static destructors.
[[noreturn]] void port_os_exit(int code);
// Stop in the debugger if there is one (SIGTRAP; Windows: a breakpoint
// only when a debugger is attached).
void port_debug_break(void);

// pread: bytes read from the file at offset, or -1. fd is a C runtime
// descriptor (fileno).
long long port_pread(int fd, void *buf, size_t bytes, uint64_t offset);

// ports/pc/lowheap.cpp (macOS, Windows) / heapguard.cpp (Linux): the end of
// the game's heap, for the DMA controller's address checks.
uintptr_t port_game_heap_end(void);

#endif
