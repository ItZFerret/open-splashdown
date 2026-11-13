// macOS and Windows: what ports/pc/watchdog.cpp and ports/pc/profile.cpp
// give the rest of the port on Linux. The hang watchdog and the sampling
// profiler read /proc and signal threads by tid; here threads are only
// named, and a dump request logs its reason.
#ifndef __linux__
#include "port.h"
#include "port_os.h"

#include <cstring>

__thread const char *port_tls_asm;
__thread uint32_t port_tls_bc0f;

void port_thread_name(const char *name)
{
    port_os_thread_name(name);
    if (strcmp(name, "game"))
        port_heap_this_thread();
}

void port_watch_futex(const void *addr, const char *name)
{
}

void port_watchdog_dump(const char *why)
{
    port_log("watchdog dump asked (%s): not available on this OS", why);
    port_hw_debug(stderr);
    port_dma_debug(stderr);
    vif1_debug(stderr);
    gs_debug(stderr);
}

void port_watchdog_start(void)
{
}

void port_profile_dump(void)
{
}

#endif // !__linux__
