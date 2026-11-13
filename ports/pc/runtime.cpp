// What the game takes from mwcc's MSL runtime, newlib and retail's linker
// script, on the PC.
#include "port.h"
#include "port_os.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

void port_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("[port] ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void port_fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char text[1024];
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    fprintf(stderr, "[port] fatal: %s\n", text);
    fflush(nullptr);
    port_show_error(text);
    abort();
}

// The game never exits on the PS2: its static destructors (GlobalGame's
// ~JetskiGame frees the font manager, waits on DMA ...) were never meant to
// run, and here they would run on the exiting thread while the game, DMA and
// timer threads are still using what they free. So the port leaves without
// exit handlers or destructors, after flushing stdio.
void port_exit(int code)
{
    port_profile_dump();
    port_audio_shutdown();
    fflush(nullptr);
    port_os_exit(code);
}

extern "C" {

// ASSERT_BREAK's __I_break (port_prelude.h). The assert message has been
// printed by then; stop in the debugger if there is one.
void port_break(int code)
{
    port_log("ASSERT break %d", code);
    port_profile_dump();
    port_debug_break();
}

// A pointer above 4 GB reached a 32-bit field (PS2_PTR, port_ptr32.h).
void port_ptr32_overflow(const void *p)
{
    port_fatal("pointer %p stored in a 32-bit field", p);
}

// A DMA tag or quadword address that isn't 16-aligned went into a PS2_PTR
// (port_ptr32.h): the DMAC reads it aligned down, so it is a layout bug.
// Logged once per call site (addr2line -e build/port/splashdown <caller>).
void port_ptr32_unaligned(const void *p, const void *caller)
{
    static const void *seen[64];
    static int n;
    for (int i = 0; i < n; i++)
        if (seen[i] == caller)
            return;
    if (n < 64)
        seen[n++] = caller;
    port_log("unaligned quadword pointer %p stored from %p", p, caller);
}

// newlib's ctype table as Parameterblocks.cpp's isalpha reads it:
// (_ctype_ + 1)[c], _U 01, _L 02, _N 04, _S 010, _P 020, _C 040, _X 0100,
// _B 0200. (Filled in before main; the game declares it const.)
char _ctype_[1 + 256];
static char MakeCtype()
{
    char *t = _ctype_ + 1;
    for (int c = 0; c < 128; c++) {
        char f = 0;
        if (isupper(c)) f |= 01;
        if (islower(c)) f |= 02;
        if (isdigit(c)) f |= 04;
        if (isspace(c)) f |= 010;
        if (ispunct(c)) f |= 020;
        if (iscntrl(c)) f |= 040;
        if (isxdigit(c)) f |= 0100;
        if (c == ' ') f |= 0200;
        t[c] = f;
    }
    return 1;
}
static char g_ctypeInit = MakeCtype();

// newlib's reentrancy struct as PSXTexMap.cpp's `stderr` reads it.
struct PortReent {
    int _errno;
    void *_stdin;
    void *_stdout;
    void *_stderr;
};
static PortReent g_reent = {0, stdin, stdout, stderr};
PortReent *_impure_ptr = &g_reent;

char *strlwr(char *s)
{
    for (char *p = s; *p; p++)
        *p = (char)tolower((unsigned char)*p);
    return s;
}

char *strupr(char *s)
{
    for (char *p = s; *p; p++)
        *p = (char)toupper((unsigned char)*p);
    return s;
}

// The static initializer and exception tables retail's linker built.
// The host runs the static constructors itself, before main, and the game
// is built without exceptions, so both tables are empty.
char __static_init[1];
char __static_init_end[1];
char __exception_table_start__[1];
char __exception_table_end__[1];

} // extern "C"

// MSL's start-up and shut-down (GameObject.cpp calls them by these names).
extern "C" void __initialize_cpp_rts(void *si, void *sie, void *ets, void *ete)
{
}

extern "C" void __destroy_global_chain(void)
{
}
