// The game's heap on macOS and Windows (Linux: glibc's brk heap,
// ports/pc/heapguard.cpp).
//
// The game keeps pointers in 32-bit fields and masks some with 0x0FFFFFFF
// (ports/pc/hw.cpp), so its blocks must lie below 256 MB. Neither system's
// malloc can be held there, so the game's malloc, calloc, realloc, memalign
// and free are compiled as port_malloc ... (tools/port/build.py: -Dmalloc=
// port_malloc), and operator new and delete are replaced, by an allocator
// over one region between the executable and 0x0FF00000:
// - the game heap: boundary tags, free blocks binned by size, first fit;
// - the port threads' heap, as on Linux (a thread named by port_thread_name
//   allocates there, so the game's layout doesn't depend on host timing):
//   power-of-two classes in a region of its own below 4 GB.
// A pointer from neither (the C library's, a system library's) goes to the
// C library's free.
#ifndef __linux__
#include "port.h"
#include "port_os.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>

namespace {

// ---- the game heap --------------------------------------------------------

const uintptr_t kEnd = 0x0FF00000;
const size_t kHdr = 16;
const size_t kMinBlock = 32;

struct Block {
    size_t size;      // header included; bit 0: in use
    size_t prevSize;  // the block before this one (0: the first)
};
struct FreeBlock : Block {
    FreeBlock *next, *prev;
};

std::mutex g_lock;
uintptr_t g_base, g_top;   // blocks lie in [g_base, g_top); the rest is untouched
size_t g_lastSize;         // size of the block ending at g_top
FreeBlock *g_bins[64];
size_t g_peak;

size_t SizeOf(const Block *b)
{
    return b->size & ~(size_t)1;
}

int BinOf(size_t size)
{
    return 63 - __builtin_clzll(size);
}

void Insert(Block *b)
{
    FreeBlock *f = (FreeBlock *)b;
    int k = BinOf(f->size);
    f->prev = nullptr;
    f->next = g_bins[k];
    if (f->next)
        f->next->prev = f;
    g_bins[k] = f;
}

void Remove(Block *b)
{
    FreeBlock *f = (FreeBlock *)b;
    if (f->prev)
        f->prev->next = f->next;
    else
        g_bins[BinOf(f->size)] = f->next;
    if (f->next)
        f->next->prev = f->prev;
}

// The block after b learns b's size (or the top does).
void SetNextPrev(Block *b)
{
    uintptr_t next = (uintptr_t)b + SizeOf(b);
    if (next == g_top)
        g_lastSize = SizeOf(b);
    else
        ((Block *)next)->prevSize = SizeOf(b);
}

void Init()
{
    // The first address the executable and what the loader put after it
    // leave free, in 16 MB steps; at least 96 MB must be left.
    for (uintptr_t base = 0x01000000; base <= kEnd - (96u << 20); base += 0x01000000)
        if (port_map_at(base, kEnd - base)) {
            g_base = g_top = base;
            return;
        }
    static const char msg[] = "[port] fatal: no room for the game heap below 256 MB\n";
    fwrite(msg, 1, sizeof msg - 1, stderr);
    abort();
}

// A free block of at least need bytes, taken off its bin, or carved from
// the top; nullptr when the region is used up.
Block *Take(size_t need)
{
    for (int k = BinOf(need); k < 64; k++) {
        int tries = 0;
        for (FreeBlock *f = g_bins[k]; f && tries < 32; f = f->next, tries++)
            if (f->size >= need) {
                Remove(f);
                return f;
            }
    }
    if (kEnd - g_top < need)
        return nullptr;
    Block *b = (Block *)g_top;
    b->size = need;
    b->prevSize = g_lastSize;
    g_top += need;
    g_lastSize = need;
    g_peak = std::max(g_peak, (size_t)(g_top - g_base));
    return b;
}

// Give back b's tail past size bytes, if a block fits there.
void SplitTail(Block *b, size_t size)
{
    size_t whole = SizeOf(b);
    if (whole < size + kMinBlock)
        return;
    Block *rest = (Block *)((uintptr_t)b + size);
    rest->size = whole - size;
    rest->prevSize = size;
    b->size = size | (b->size & 1);
    uintptr_t after = (uintptr_t)rest + rest->size;
    if (after == g_top) {
        g_top = (uintptr_t)rest;   // back to the wilderness
        g_lastSize = size;
        return;
    }
    Block *n = (Block *)after;
    if (!(n->size & 1)) {
        Remove(n);
        rest->size += n->size;
    }
    SetNextPrev(rest);
    Insert(rest);
}

void *GameAlloc(size_t size, size_t align)
{
    if (size > kEnd)
        return nullptr;
    size_t need = std::max(kMinBlock, ((size + 15) & ~(size_t)15) + kHdr);
    std::lock_guard<std::mutex> lock(g_lock);
    if (!g_base)
        Init();
    if (align <= 16) {
        Block *b = Take(need);
        if (!b)
            return nullptr;
        b->size |= 1;
        SplitTail(b, need);
        return (char *)b + kHdr;
    }
    // Room to cut a free block off the front, so the user pointer is aligned.
    Block *b = Take(need + align + kMinBlock);
    if (!b)
        return nullptr;
    uintptr_t user = (uintptr_t)b + kHdr;
    if (user & (align - 1)) {
        uintptr_t a = (user + kMinBlock + align - 1) & ~(uintptr_t)(align - 1);
        size_t front = a - kHdr - (uintptr_t)b;
        Block *nb = (Block *)(a - kHdr);
        nb->size = b->size - front;
        nb->prevSize = front;
        b->size = front;
        Insert(b);
        b = nb;
        SetNextPrev(b);
    }
    b->size |= 1;
    SplitTail(b, need);
    return (char *)b + kHdr;
}

bool InGame(const void *p)
{
    return g_base && (uintptr_t)p >= g_base && (uintptr_t)p < kEnd;
}

size_t GameSize(const void *p)
{
    return SizeOf((const Block *)((const char *)p - kHdr)) - kHdr;
}

void GameFree(void *p)
{
    std::lock_guard<std::mutex> lock(g_lock);
    Block *b = (Block *)((char *)p - kHdr);
    if (!(b->size & 1) || (uintptr_t)p >= g_top)
        port_fatal("free(%p): not an allocated block of the game heap (freed twice?)", p);
    b->size &= ~(size_t)1;
    uintptr_t after = (uintptr_t)b + b->size;
    if (after != g_top) {
        Block *n = (Block *)after;
        if (!(n->size & 1)) {
            Remove(n);
            b->size += n->size;
        }
    }
    if (b->prevSize) {
        Block *prev = (Block *)((uintptr_t)b - b->prevSize);
        if (!(prev->size & 1)) {
            Remove(prev);
            prev->size += b->size;
            b = prev;
        }
    }
    if ((uintptr_t)b + b->size == g_top) {
        g_top = (uintptr_t)b;
        g_lastSize = b->prevSize;
        return;
    }
    SetNextPrev(b);
    Insert(b);
}

// ---- the port threads' heap (as ports/pc/heapguard.cpp's) -------------------

const size_t kPortRegion = 256u << 20;
uintptr_t g_portBase, g_portNext, g_portEnd;
std::mutex g_portLock;
void *g_portFree[40];
__thread int t_portHeap;

struct PortHeader {
    size_t cls;     // size class | offset of the user pointer in the block << 8
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
    while (cls < 40 && ((size_t)1 << cls) < need)
        cls++;
    if (cls >= 40)
        return nullptr;
    std::lock_guard<std::mutex> lock(g_portLock);
    if (!g_portBase) {
        void *r = port_map_low(kPortRegion);
        if (!r)
            return nullptr;
        g_portBase = g_portNext = (uintptr_t)r;
        g_portEnd = g_portBase + kPortRegion;
    }
    uint8_t *block;
    if (g_portFree[cls]) {
        block = (uint8_t *)g_portFree[cls];
        memcpy(&g_portFree[cls], block, sizeof(void *));
    } else {
        if (g_portEnd - g_portNext < ((size_t)1 << cls))
            return nullptr;  // the caller falls back to the C library
        block = (uint8_t *)g_portNext;
        g_portNext += (size_t)1 << cls;
    }
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

// ---- both -------------------------------------------------------------------

void *Alloc(size_t size, size_t align)
{
    if (t_portHeap) {
        if (void *p = PortAlloc(size, align))
            return p;
        // a block too large for the port region: the C library's (above
        // 4 GB; the port's own data only)
        if (align <= 16)
            return malloc(size);
    }
    void *p = GameAlloc(size, align);
    if (!p)
        port_log("game heap: out of memory (%zu bytes asked, %zu MB in use)", size, (size_t)(g_top - g_base) >> 20);
    return p;
}

void Free(void *p)
{
    if (!p)
        return;
    if (InPort(p))
        PortFree(p);
    else if (InGame(p))
        GameFree(p);
    else
        free(p);
}

void *New(size_t size, size_t align)
{
    void *p = Alloc(size ? size : 1, align);
    if (!p)
        port_fatal("operator new(%zu): out of memory", size);
    return p;
}

} // namespace

void port_heap_this_thread(void)
{
    t_portHeap = 1;
}

uintptr_t port_heapguard_end(void)
{
    return 0;
}

uintptr_t port_game_heap_end(void)
{
    return kEnd;
}

extern "C" {

// The game declares these with 32-bit sizes (`malloc(unsigned int)`): the
// upper half of the register isn't its argument.
void *port_malloc(size_t size)
{
    return Alloc((uint32_t)size, 16);
}

void port_free(void *p)
{
    Free(p);
}

void *port_calloc(size_t n, size_t size)
{
    size_t bytes = (size_t)(uint32_t)n * (uint32_t)size;
    void *p = Alloc(bytes, 16);
    if (p)
        memset(p, 0, bytes);
    return p;
}

void *port_realloc(void *p, size_t size)
{
    size = (uint32_t)size;
    if (!p)
        return Alloc(size, 16);
    size_t old;
    if (InPort(p))
        old = PortSize(p);
    else if (InGame(p)) {
        old = GameSize(p);
        if (size <= old && !t_portHeap)
            return p;
    } else
        return realloc(p, size);
    void *q = Alloc(size, 16);
    if (q) {
        memcpy(q, p, std::min(size, old));
        Free(p);
    }
    return q;
}

void *port_memalign(size_t align, size_t size)
{
    return Alloc((uint32_t)size, std::max<size_t>((uint32_t)align, 16));
}

} // extern "C"

void *operator new(size_t size) { return New(size, 16); }
void *operator new[](size_t size) { return New(size, 16); }
void *operator new(size_t size, const std::nothrow_t &) noexcept { return Alloc(size ? size : 1, 16); }
void *operator new[](size_t size, const std::nothrow_t &) noexcept { return Alloc(size ? size : 1, 16); }
void *operator new(size_t size, std::align_val_t a) { return New(size, (size_t)a); }
void *operator new[](size_t size, std::align_val_t a) { return New(size, (size_t)a); }
void *operator new(size_t size, std::align_val_t a, const std::nothrow_t &) noexcept { return Alloc(size ? size : 1, (size_t)a); }
void *operator new[](size_t size, std::align_val_t a, const std::nothrow_t &) noexcept { return Alloc(size ? size : 1, (size_t)a); }
void operator delete(void *p) noexcept { Free(p); }
void operator delete[](void *p) noexcept { Free(p); }
void operator delete(void *p, size_t) noexcept { Free(p); }
void operator delete[](void *p, size_t) noexcept { Free(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { Free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { Free(p); }
void operator delete(void *p, std::align_val_t) noexcept { Free(p); }
void operator delete[](void *p, std::align_val_t) noexcept { Free(p); }
void operator delete(void *p, size_t, std::align_val_t) noexcept { Free(p); }
void operator delete[](void *p, size_t, std::align_val_t) noexcept { Free(p); }
void operator delete(void *p, std::align_val_t, const std::nothrow_t &) noexcept { Free(p); }
void operator delete[](void *p, std::align_val_t, const std::nothrow_t &) noexcept { Free(p); }

#endif // !__linux__
