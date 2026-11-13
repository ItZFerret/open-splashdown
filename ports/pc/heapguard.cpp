// PORT_HEAPGUARD=1: a guard-page heap for finding overflows.
//
// The game's sizes were computed for 32-bit pointers in many places, so on
// the PC some allocations are too small. AddressSanitizer can't find these:
// its heap lies above 4 GB, where the game's 32-bit pointer casts fail first.
// This allocator keeps every block below 256 MB (ports/pc/hw.cpp), each ending
// at a guard page, so the first byte written past a block's end faults
// at the writing instruction. The bytes between the requested size and the
// end of the 16-byte-aligned block hold a canary that free() checks. Freed
// blocks become PROT_NONE, so a write after free faults too; their address
// space is reused only once the region is used up, and then only after
// 32 MB more have been freed (a quarantine).
//
// The guard page is readable (PORT_HEAPGUARD=1: retail reads past some
// arrays, harmlessly, e.g. RainbowPS2Snd::GetPlayStatus's m_streamUsed[sound
// index]) or not (PORT_HEAPGUARD=2: reads fault too).
//
// PORT_HEAPGUARD_SIZE=MIN-MAX guards only blocks of those sizes (the rest
// go to glibc, below PORT_HEAPGUARD_BASE, default 0x02000000: raise it to
// give glibc more room). PORT_HEAPGUARD_ALIGN=4 (or 8) places malloc's blocks at that alignment
// instead of 16, so small overflows fault at once.
//
// Without PORT_HEAPGUARD every call goes to glibc's allocator, except the
// port threads' (their own heap, below).
#ifdef __linux__
#include "port.h"
#include "port_os.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <malloc.h>
#include <mutex>
#include <sys/mman.h>
#include <unistd.h>

extern "C" {
void *__libc_malloc(size_t);
void __libc_free(void *);
void *__libc_calloc(size_t, size_t);
void *__libc_realloc(void *, size_t);
void *__libc_memalign(size_t, size_t);
}

namespace {

uintptr_t kBase = 0x02000000; // above the brk heap's start, below 0x10000000 (PORT_HEAPGUARD_BASE)
const uintptr_t kEnd = 0x0FF00000;
const size_t kPage = 4096;
const unsigned char kCanary = 0xCB;

struct Header {
    size_t size;      // requested
    size_t mapped;    // bytes mapped for the block, guard page excluded
    uintptr_t start;  // first mapped byte
    uint64_t magic;
};
const uint64_t kMagic = 0x5053324755415244ull; // "PS2GUARD"

std::mutex g_lock;
int g_enabled = -1; // -1: not decided yet
uintptr_t g_next;
// PORT_HEAPGUARD_SIZE=MIN-MAX: guard only blocks of MIN..MAX bytes, the rest
// go to glibc (whose brk heap then has up to kBase)
size_t g_minSize = 0, g_maxSize = (size_t)-1;
// malloc's alignment: 16 as glibc's, or PORT_HEAPGUARD_ALIGN=4 or 8 to end
// blocks closer to the guard page (overflows smaller than 16 bytes fault
// at once instead of at free)
size_t g_mallocAlign = 16;

// Freed address ranges (guard page included): quarantined first, in a ring,
// then free for reuse. Fixed arrays: malloc can't allocate here.
struct Range {
    uintptr_t start;
    size_t size;
};
const int kRanges = 1 << 16;
const size_t kQuarantine = 32 << 20;
Range g_quar[kRanges];
int g_quarHead, g_quarCount;
size_t g_quarBytes;
Range g_free[kRanges];
int g_freeCount;

// coalesced with its neighbours (the game grows buffers by realloc, so each
// request is larger than any one freed block)
void AddFree(Range r)
{
    for (bool merged = true; merged;) {
        merged = false;
        for (int i = 0; i < g_freeCount; i++)
            if (g_free[i].start + g_free[i].size == r.start || r.start + r.size == g_free[i].start) {
                r.start = std::min(r.start, g_free[i].start);
                r.size += g_free[i].size;
                g_free[i] = g_free[--g_freeCount];
                merged = true;
                break;
            }
    }
    if (g_freeCount < kRanges)
        g_free[g_freeCount++] = r;  // else the range is lost: fine
}

void Quarantine(Range r)
{
    if (g_quarCount == kRanges || g_quarBytes + r.size > kQuarantine) {
        while (g_quarCount && (g_quarCount == kRanges || g_quarBytes + r.size > kQuarantine)) {
            Range old = g_quar[g_quarHead];
            g_quarHead = (g_quarHead + 1) % kRanges;
            g_quarCount--;
            g_quarBytes -= old.size;
            AddFree(old);
        }
    }
    g_quar[(g_quarHead + g_quarCount) % kRanges] = r;
    g_quarCount++;
    g_quarBytes += r.size;
}

// first fit among the freed ranges; 0 if none
uintptr_t TakeFree(size_t size)
{
    for (int i = 0; i < g_freeCount; i++)
        if (g_free[i].size >= size) {
            uintptr_t start = g_free[i].start;
            g_free[i].start += size;
            g_free[i].size -= size;
            if (!g_free[i].size)
                g_free[i] = g_free[--g_freeCount];
            return start;
        }
    return 0;
}

bool Enabled()
{
    if (g_enabled < 0) {
        const char *env = getenv("PORT_HEAPGUARD");
        g_enabled = env && *env && *env != '0' ? atoi(env) : 0;
        if (g_enabled) {
            if (const char *a = getenv("PORT_HEAPGUARD_ALIGN"))
                g_mallocAlign = std::max(4, atoi(a));
            if (const char *b = getenv("PORT_HEAPGUARD_BASE"))
                kBase = strtoul(b, nullptr, 0);
            if (const char *r = getenv("PORT_HEAPGUARD_SIZE")) {
                char *end;
                g_minSize = strtoul(r, &end, 0);
                if (*end == '-')
                    g_maxSize = strtoul(end + 1, nullptr, 0);
            }
            g_next = kBase;
            void *p = mmap((void *)kBase, kEnd - kBase, PROT_NONE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
            if (p != (void *)kBase) {
                static const char msg[] = "[port] heapguard: can't reserve its region\n";
                write(2, msg, sizeof msg - 1);
                abort();
            }
        }
    }
    return g_enabled;
}

bool Guarded(size_t size)
{
    return size >= g_minSize && size <= g_maxSize;
}

// Only with the guard heap on: glibc's brk heap grows past kBase too.
bool Ours(const void *p)
{
    return Enabled() && (uintptr_t)p >= kBase && (uintptr_t)p < kEnd;
}

Header *HeaderOf(void *p)
{
    return (Header *)p - 1;
}

void *GuardAlloc(size_t size, size_t align)
{
    if (align < 4)
        align = 4;
    size_t body = (size + align - 1) & ~(align - 1);
    if (body == 0)
        body = align;
    // The block ends at a page boundary; the header sits before the
    // (aligned) user pointer, all in mapped pages.
    size_t need = body + sizeof(Header) + align;
    size_t mapped = (need + kPage - 1) & ~(kPage - 1);
    std::lock_guard<std::mutex> lock(g_lock);
    uintptr_t start = g_next;
    if (start + mapped + kPage <= kEnd) {
        g_next = start + mapped + kPage; // leave the guard page unmapped
    } else if (!(start = TakeFree(mapped + kPage))) {
        static const char msg[] = "[port] heapguard: region exhausted\n";
        write(2, msg, sizeof msg - 1);
        abort();
    }
    if (mprotect((void *)start, mapped, PROT_READ | PROT_WRITE) != 0)
        abort();
    if (g_enabled == 1 && mprotect((void *)(start + mapped), kPage, PROT_READ) != 0)
        abort();
    uintptr_t end = start + mapped;
    uintptr_t user = (end - body) & ~(uintptr_t)(align - 1);
    // Overflow past `size` lands in the slack up to `end`: fill it with the
    // canary. With align > 16 the slack can be larger than 15 bytes.
    memset((void *)(user + size), kCanary, end - (user + size));
    Header *h = (Header *)user - 1;
    h->size = size;
    h->mapped = mapped;
    h->start = start;
    h->magic = kMagic;
    return (void *)user;
}

void Check(void *p)
{
    Header *h = HeaderOf(p);
    if (h->magic != kMagic)
        port_fatal("heapguard: free(%p): not a heapguard block, or its header was overwritten", p);
    unsigned char *tail = (unsigned char *)p + h->size;
    unsigned char *end = (unsigned char *)(h->start + h->mapped);
    for (unsigned char *q = tail; q < end; q++)
        if (*q != kCanary)
            port_fatal("heapguard: block %p (%zu bytes) overflowed: byte +%zu written", p, h->size,
                       (size_t)(q - (unsigned char *)p));
}

void GuardFree(void *p)
{
    Check(p);
    Header *h = HeaderOf(p);
    uintptr_t start = h->start;
    size_t mapped = h->mapped;
    mprotect((void *)start, mapped + kPage, PROT_NONE);
    madvise((void *)start, mapped, MADV_DONTNEED);  // zero when reused (calloc)
    std::lock_guard<std::mutex> lock(g_lock);
    Quarantine({start, mapped + kPage});
}

// ---- the port threads' heap ----------------------------------------------
//
// glibc's one arena is the game's heap (ports/pc/hw.cpp). The port's own threads
// (DMA, VU1's recompiler, GS, video, watchdog, ...) allocate as they run,
// and in the game's heap their blocks landed among the game's at points that
// depend on host timing: the game's layout then differed between two
// PORT_DETERMINISTIC runs, and so did what it read from memory it never
// initialised (the --audio golden session diverged at frame 7000 in some
// runs). So a thread named by port_thread_name (every thread but the game's)
// allocates here: power-of-two size classes with a free list each, in a
// region of their own below 2 GB; free and realloc go by address.
const size_t kPortRegion = 256u << 20;
uintptr_t g_portBase, g_portNext, g_portEnd;
std::mutex g_portLock;
void *g_portFree[40];
__thread int t_portHeap;

struct PortHeader {
    size_t cls;     // size class: block of 1 << cls bytes, this header included
    size_t size;    // requested
};

bool InPort(const void *p)
{
    return (uintptr_t)p >= g_portBase && (uintptr_t)p < g_portEnd;
}

void *PortAlloc(size_t size, size_t align)
{
    if (align < 16)
        align = 16;
    size_t need = size + std::max<size_t>(align, sizeof(PortHeader)) + (align > 16 ? align : 0);
    size_t cls = 5;
    while (((size_t)1 << cls) < need)
        cls++;
    std::lock_guard<std::mutex> lock(g_portLock);
    if (!g_portBase) {
        void *r = mmap(nullptr, kPortRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_32BIT,
                       -1, 0);
        if (r == MAP_FAILED)
            return nullptr;
        g_portBase = g_portNext = (uintptr_t)r;
        g_portEnd = g_portBase + kPortRegion;
    }
    uint8_t *block;
    if (cls < 40 && g_portFree[cls]) {
        block = (uint8_t *)g_portFree[cls];
        memcpy(&g_portFree[cls], block, sizeof(void *));
    } else {
        if (cls >= 40 || g_portEnd - g_portNext < ((size_t)1 << cls))
            return nullptr;  // the caller falls back to glibc
        block = (uint8_t *)g_portNext;
        g_portNext += (size_t)1 << cls;
    }
    // the header sits just before the (aligned) user pointer
    uintptr_t user = ((uintptr_t)block + sizeof(PortHeader) + align - 1) & ~(uintptr_t)(align - 1);
    PortHeader *h = (PortHeader *)(user - sizeof(PortHeader));
    h->cls = cls | ((user - (uintptr_t)block) << 8);
    h->size = size;
    return (void *)user;
}

void PortFree(void *p)
{
    PortHeader *h = (PortHeader *)((uintptr_t)p - sizeof(PortHeader));
    size_t cls = h->cls & 0xFF;
    uint8_t *block = (uint8_t *)p - (h->cls >> 8);
    std::lock_guard<std::mutex> lock(g_portLock);
    memcpy(block, &g_portFree[cls], sizeof(void *));
    g_portFree[cls] = block;
}

size_t PortSize(const void *p)
{
    return ((const PortHeader *)((uintptr_t)p - sizeof(PortHeader)))->size;
}

} // namespace

// ports/pc/watchdog.cpp's port_thread_name: this thread allocates from the port
// heap from now on.
void port_heap_this_thread(void)
{
    t_portHeap = 1;
}

// ports/pc/dma.cpp: the end of the game's heap.
// ports/pc/dma.cpp: the end of game memory when the guard region holds game
// blocks (PORT_HEAPGUARD), else 0 (the brk heap's end is the limit).
uintptr_t port_heapguard_end(void)
{
    return Enabled() ? kEnd : 0;
}

uintptr_t port_game_heap_end(void)
{
    return std::max((uintptr_t)sbrk(0), port_heapguard_end());
}

extern "C" {

void *malloc(size_t size)
{
    if (t_portHeap)
        if (void *p = PortAlloc(size, 16))
            return p;
    return Enabled() && Guarded(size) ? GuardAlloc(size, g_mallocAlign) : __libc_malloc(size);
}

void free(void *p)
{
    if (!p)
        return;
    if (InPort(p))
        PortFree(p);
    else if (Ours(p))
        GuardFree(p);
    else
        __libc_free(p);
}

void *calloc(size_t n, size_t size)
{
    if (t_portHeap && (!size || n <= (size_t)-1 / size))
        if (void *p = PortAlloc(n * size, 16)) {
            memset(p, 0, n * size);
            return p;
        }
    if (!Enabled() || (size && n <= (size_t)-1 / size && !Guarded(n * size)))
        return __libc_calloc(n, size);
    if (size && n > (size_t)-1 / size)
        return nullptr;
    return GuardAlloc(n * size, g_mallocAlign); // fresh pages are zero
}

void *realloc(void *p, size_t size)
{
    if (!p)
        return malloc(size);
    if (InPort(p)) {
        void *q = malloc(size);
        if (q) {
            memcpy(q, p, std::min(size, PortSize(p)));
            PortFree(p);
        }
        return q;
    }
    if (!Ours(p)) {
        if (!Enabled() || !Guarded(size))
            return __libc_realloc(p, size);
        void *q = GuardAlloc(size, g_mallocAlign);  // glibc block growing into the guarded sizes
        memcpy(q, p, std::min(size, malloc_usable_size(p)));
        __libc_free(p);
        return q;
    }
    if (!Guarded(size)) {
        void *q = __libc_malloc(size);
        memcpy(q, p, std::min(size, HeaderOf(p)->size));
        GuardFree(p);
        return q;
    }
    Check(p);
    void *q = GuardAlloc(size, g_mallocAlign);
    size_t old = HeaderOf(p)->size;
    memcpy(q, p, old < size ? old : size);
    GuardFree(p);
    return q;
}

void *memalign(size_t align, size_t size)
{
    if (t_portHeap && align <= 4096)
        if (void *p = PortAlloc(size, align))
            return p;
    return Enabled() && Guarded(size) ? GuardAlloc(size, align) : __libc_memalign(align, size);
}

// Libraries size buffers by it (systemd's libudev reads into the whole usable
// size, under SDL_Init's gamepad scan), so a block of the port heap or the
// guard region answers its requested size; glibc's own blocks, glibc's.
size_t malloc_usable_size(void *p)
{
    if (!p)
        return 0;
    if (InPort(p))
        return PortSize(p);
    if (Ours(p))
        return HeaderOf(p)->size;
    static size_t (*libc)(void *) = (size_t (*)(void *))dlsym(RTLD_NEXT, "malloc_usable_size");
    return libc(p);
}

void *aligned_alloc(size_t align, size_t size)
{
    return memalign(align, size);
}

int posix_memalign(void **out, size_t align, size_t size)
{
    void *p = memalign(align, size);
    if (!p)
        return ENOMEM;
    *out = p;
    return 0;
}

} // extern "C"

#endif // __linux__
