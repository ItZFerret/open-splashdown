// PSXTextureMapManager.cpp  @ 0x002B2B70 (text 0x2B2B70-0x2B8490)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's PSXTextureMapManager.cpp as one translation unit: TextureMapList,
// the transfer list (TransferListHandler), the IPU cache (IpuCacheTree,
// IpuCacheHandle, IpuCache), PSXTextureMapManager, then the static initializer
// (the transfer list and the timing peaks). Its .rodata runs from @607 (after
// JetskiAIVehicle.cpp's RTTI) to the TextureMapManager/PSXTextureMapManager
// RTTI.


#include "ManagedTextureGroup/ManagedTextureGroup.h"
#include "TextureMap/TextureMap.h"
#include "PSXTextureMap/PSXTextureMap.h"
#include "Game/Game.h"
#include "PSXTextureMapManager/PSXTextureMapManager.h"
#include "IpuCacheTree/IpuCacheTree.h"
#include "IpuCache/IpuCache.h"
#include "GsTextureTransfer/GsTextureTransfer.h"
#include "IpuTextureTransfer/IpuTextureTransfer.h"
#include "sceGifTag/sceGifTag.h"
#include "PSXVideoCard/PSXVideoCard.h"
#include "DebugOverlay/DebugOverlay.h"
#include "JetskiGame/JetskiGame.h"
#include "GameObject/GameObject.h"
#include "TextureMapManager/TextureMapManager.h"
#include "DmaRenderItem/DmaRenderItem.h"
#include "ControlInterface/ControlTypes.h"
#include "PSXRenderTarget/PSXRenderTarget.h"
#include "TransferListHandler/TransferListHandler.h"
#include "PeakInt/PeakInt.h"
#include "soultree_assert_block.h"

// Read by PSXMediaControl.cpp. Retail's .data puts it just before this TU's
// own, after JetskiInGameOverlay.cpp's; AuralScape.cpp or JetskiAIVehicle.cpp
// could hold it instead.
int AllowIPUInterrupts = 1;

class PeakFloat {
public:
    PeakFloat(unsigned int window);
    void Record(float value);
    float GetPeak();

    unsigned int m_time;            // +0x0
    float m_peak;                   // +0x4
    unsigned int m_window;          // +0x8
};

extern "C" void free(void *p);

// The static initializer's objects: the texture transfer list and the texture timing peaks.
static TransferListHandler TransferList;

// One queued IPU decompression: the texture, its mip level and the cache handle that receives it.
struct IPUSlot {
    PSXTextureMap *m_texture;       // +0x00
    PSXMipSurface *m_mip;           // +0x04
    IpuCacheHandle *m_handle;       // +0x08
};

extern PSXTextureMapManager *LpTextureMapManager;
extern JetskiGame *LpGame;
extern PSXVideoCard *LpVideoCard;
extern volatile int LockRenderQueueState;
int KEYDOWN_HANDLER_TEST(unsigned int key, DeviceType device, ProcessNode *node, unsigned int mods);

// Defined here for its retail .bss slot.
static PSXTextureMap *LastTextureMap;
static int LastSlot;
static int AddInterrupt;
static IPUSlot IPUSlotList[1024];
static int CurrentIPUIndex;
static int MaxIPUIndex;
static int DebugDump;
static int nTextureReqBytes;
static int nTextureReqBytesMax;

static PeakInt PeakTextureReqBytes(5000);

// Defined here for its retail .bss slot.
static int n256Req;
static int n128Req;
static int n64Req;
static int n32Req;
static int n16Req;
static int n256ReqMax;
static int n128ReqMax;
static int n64ReqMax;
static int n32ReqMax;
static int n16ReqMax;
static int nTexturesDecompressed;
static int nTexturesDecompressedMax;

static PeakInt PeakTexturesDecompressed(5000);

// Defined here for its retail .bss slot.
static int nBytesDecompressed;
static int nBytesDecompressedMax;

static PeakInt PeakBytesDecompressed(5000);

// Defined here for its retail .bss slot.
static int n256;
static int n128;
static int n64;
static int n32;
static int n16;
static int n256free;
static int n128free;
static int n64free;
static int n32free;
static int n16free;

static PeakFloat PeakDecompressionTime(5000);

// Defined here for its retail .bss slot.
static int nTotalTransferedTextures;
static _CacheTreeNode *LpCacheMem;


// ---- src/ManagedTextureGroup/TextureMapList_ctor.cpp
class TextureMap;


TextureMapList::TextureMapList()
{
    m_head = m_tail = m_cur = 0;
    m_count = 0;
}


// ---- src/TextureMapList/dtor.cpp


// Unlinks every map; the maps themselves belong to someone else.
TextureMapList::~TextureMapList()
{
    for (m_cur = m_head; m_cur; ) {
        TextureMap *next = m_cur->m_next;
        m_cur->m_next = 0;
        m_cur->m_prev = 0;
        m_cur = next;
    }
}


// ---- src/TextureMapList/First.cpp

TextureMap *TextureMapList::First()
{
    m_cur = m_head;
    return m_cur;
}


// ---- src/TextureMapList/Last.cpp

TextureMap *TextureMapList::Last()
{
    m_cur = m_tail;
    return m_cur;
}


// ---- src/TextureMapList/Next.cpp

TextureMap *TextureMapList::Next()
{
    if (m_cur)
        m_cur = m_cur->m_next;
    return m_cur;
}


// ---- src/TextureMapList/Add.cpp


void TextureMapList::Add(TextureMap *texture)
{
    if (!m_head) {
        m_head = m_tail = texture;
        texture->m_next = texture->m_prev = 0;
    } else {
        m_tail->m_next = texture;
        texture->m_prev = m_tail;
        texture->m_next = 0;
        m_tail = texture;
    }
    m_count++;
}


// ---- src/TextureMapList/Remove.cpp


void TextureMapList::Remove(TextureMap *map)
{
    if (map->m_prev)
        map->m_prev->m_next = map->m_next;
    if (map->m_next)
        map->m_next->m_prev = map->m_prev;
    if (m_head == map)
        m_head = map->m_next;
    if (m_tail == map)
        m_tail = map->m_prev;
    m_count--;
}


// ---- src/global/WillFitInSlot.cpp

int WillFitInSlot(PSXTextureMap *map)
{
    return (map->m_bitsPerPixel * (map->m_width * map->m_height)) >> 3 <= 0x20000;
}


// ---- src/global/MapCurrentRenderLoop.cpp
#pragma alignlabel off


// Bit of the texture "used by render loop" mask for the current loop.
int MapCurrentRenderLoop(int)
{
    int mask = 0;
    switch (LpGame->m_currentPlayer) {
    case 0:
        mask = 1;
        break;
    case 1:
        mask = 2;
        break;
    case 2:
        mask = 4;
        break;
    case 3:
        mask = 8;
        break;
    default:
#line 101 "PSXTextureMapManager.cpp"
        ASSERT(0, "MapCurrentRenderLoop: currentLoop (=%d)  > USED_BY_LOOP4, currentLoop");
    }
    return mask;
}
#pragma alignlabel reset


// ---- src/global/ManualTextureSynchronizationInterrupt.cpp
#pragma alignlabel off

typedef int int128_t __attribute__((mode(TI)));

extern "C" void iFlushCache(int);


// Interrupt handler: the FPU isn't saved on interrupt entry, so the handler
// saves and restores all FPU registers and control registers itself.
int ManualTextureSynchronizationInterrupt(int)
{
    float fpuRegs[32];
    int128_t fpuControl[2];
    asm("swc1 $f0, 0x0(%0); swc1 $f1, 0x4(%0); swc1 $f2, 0x8(%0); swc1 $f3, 0xC(%0); swc1 $f4, 0x10(%0); swc1 $f5, 0x14(%0); swc1 $f6, 0x18(%0); swc1 $f7, 0x1C(%0); swc1 $f8, 0x20(%0); swc1 $f9, 0x24(%0); swc1 $f10, 0x28(%0); swc1 $f11, 0x2C(%0); swc1 $f12, 0x30(%0); swc1 $f13, 0x34(%0); swc1 $f14, 0x38(%0); swc1 $f15, 0x3C(%0); swc1 $f16, 0x40(%0); swc1 $f17, 0x44(%0); swc1 $f18, 0x48(%0); swc1 $f19, 0x4C(%0); swc1 $f20, 0x50(%0); swc1 $f21, 0x54(%0); swc1 $f22, 0x58(%0); swc1 $f23, 0x5C(%0); swc1 $f24, 0x60(%0); swc1 $f25, 0x64(%0); swc1 $f26, 0x68(%0); swc1 $f27, 0x6C(%0); swc1 $f28, 0x70(%0); swc1 $f29, 0x74(%0); swc1 $f30, 0x78(%0); swc1 $f31, 0x7C(%0); cfc1 $t0, $0; cfc1 $t1, $31; sq $t0, 0x0(%1); sq $t1, 0x10(%1)"
        : : "r"(fpuRegs), "r"(fpuControl) : "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$f10", "$f11", "$f12", "$f13", "$f14", "$f15", "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23", "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31", "$8", "$9", "memory");
    iFlushCache(0);
#line 166 "PSXTextureMapManager.cpp"
    ASSERT(LockRenderQueueState == 0, "Sending texture while getting texture");
    LpTextureMapManager->SendCurrentTransfer(0);
    *(volatile int *)0x10003C10 = 8;    // VIF1_FBRST: STC (cancel the stall)
    asm("lwc1 $f0, 0x0(%0); lwc1 $f1, 0x4(%0); lwc1 $f2, 0x8(%0); lwc1 $f3, 0xC(%0); lwc1 $f4, 0x10(%0); lwc1 $f5, 0x14(%0); lwc1 $f6, 0x18(%0); lwc1 $f7, 0x1C(%0); lwc1 $f8, 0x20(%0); lwc1 $f9, 0x24(%0); lwc1 $f10, 0x28(%0); lwc1 $f11, 0x2C(%0); lwc1 $f12, 0x30(%0); lwc1 $f13, 0x34(%0); lwc1 $f14, 0x38(%0); lwc1 $f15, 0x3C(%0); lwc1 $f16, 0x40(%0); lwc1 $f17, 0x44(%0); lwc1 $f18, 0x48(%0); lwc1 $f19, 0x4C(%0); lwc1 $f20, 0x50(%0); lwc1 $f21, 0x54(%0); lwc1 $f22, 0x58(%0); lwc1 $f23, 0x5C(%0); lwc1 $f24, 0x60(%0); lwc1 $f25, 0x64(%0); lwc1 $f26, 0x68(%0); lwc1 $f27, 0x6C(%0); lwc1 $f28, 0x70(%0); lwc1 $f29, 0x74(%0); lwc1 $f30, 0x78(%0); lwc1 $f31, 0x7C(%0); lq $t0, 0x0(%1); lq $t1, 0x10(%1); ctc1 $t0, $0; ctc1 $t1, $31; sync; ei"
        : : "r"(fpuRegs), "r"(fpuControl) : "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$f10", "$f11", "$f12", "$f13", "$f14", "$f15", "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23", "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31", "$8", "$9", "memory");
    return -1;
}
#pragma alignlabel reset


// ---- src/TransferListHandler/ctor.cpp

TransferListHandler::TransferListHandler()
{
    Reset();
}


// ---- src/TransferListHandler/Reset.cpp

void TransferListHandler::Reset()
{
    m_current = 0;
    m_buffers[0].m_count = 0;
    m_buffers[1].m_count = 0;
}


// ---- src/IpuCacheTree/ctor.cpp

IpuCacheTree::IpuCacheTree()
{
    m_freeCounts[0] = 1;
    m_freeCounts[1] = 4;
    m_freeCounts[2] = 0x10;
    m_freeCounts[3] = 0x40;
    m_freeCounts[4] = 0x100;
}


// ---- src/IpuCacheTree/dtor.cpp

IpuCacheTree::~IpuCacheTree()
{
}


// ---- src/IpuCacheTree/Create.cpp
#pragma alignlabel off


extern "C" void *memset(void *, int, unsigned int);
extern "C" void *malloc(unsigned int);



void IpuCacheTree::Create()
{
    memset(&m_root, 0, sizeof(m_root));
    m_root.m_data = (unsigned char *)malloc(0x20000);
#line 441 "PSXTextureMapManager.cpp"
    ASSERT(m_root.m_data, "IpuCacheTree::Create: RootNode.LpImageData == NULL");
    BuildCacheTree(256, &m_root, m_root.m_data, 0);
}
#pragma alignlabel reset


// ---- src/IpuCacheTree/Release.cpp

void IpuCacheTree::Release()
{
    if (m_root.m_data) {
        free(m_root.m_data);
        m_root.m_data = 0;
    }
}


// ---- src/IpuCacheTree/BuildCacheTree.cpp



// Splits a size x size 16-bit block into quadrants down to 16x16 leaves,
// taking nodes from the LpCacheMem pool.
void IpuCacheTree::BuildCacheTree(int size, _CacheTreeNode *node, unsigned char *data, int index)
{
    int blockSize = size * size * 2;
    node->m_data = data + blockSize * index;
    node->m_free = 1;
    node->m_size = size;
    if (size == 16) {
        node->m_used = 0;
        node->m_children[0] = 0;
        node->m_children[1] = 0;
        node->m_children[2] = 0;
        node->m_children[3] = 0;
        return;
    }
    if (!node->m_children[0]) {
        node->m_children[0] = LpCacheMem;
        LpCacheMem++;
        BuildCacheTree(size / 2, node->m_children[0], node->m_data, 0);
    }
    if (!node->m_children[1]) {
        node->m_children[1] = LpCacheMem;
        LpCacheMem++;
        BuildCacheTree(size / 2, node->m_children[1], node->m_data, 1);
    }
    if (!node->m_children[2]) {
        node->m_children[2] = LpCacheMem;
        LpCacheMem++;
        BuildCacheTree(size / 2, node->m_children[2], node->m_data, 2);
    }
    if (!node->m_children[3]) {
        node->m_children[3] = LpCacheMem;
        LpCacheMem++;
        BuildCacheTree(size / 2, node->m_children[3], node->m_data, 3);
    }
}


// ---- src/IpuCacheTree/RetrieveCacheAddress.cpp
// Retail built IpuCacheTree.cpp with strength reduction off (see
// RemoveFromCacheTree): the free-count loop indexes freeCounts[level] each pass.
#pragma opt_strength_reduction off




// Takes a child's block (bit `mask` of m_used marks the child as full).
#define TRY_CHILD(index, mask)                                                  \
    if (!(node->m_used & mask)) {                                               \
        unsigned char *address = RetrieveCacheAddress(node->m_children[index], size, freeCounts, level + 1); \
        if (node->m_free == 1)                                                  \
            freeCounts[level]--;                                                \
        node->m_free = 0;                                                       \
        if (node->m_children[index]->m_used == 0xF)                             \
            node->m_used |= mask;                                               \
        if (address)                                                            \
            return address;                                                     \
    }

// Finds a free size x size block below node, marking it and its parents used
// and updating the per-level free counts.
unsigned char *IpuCacheTree::RetrieveCacheAddress(_CacheTreeNode *node, int size, unsigned short *freeCounts,
                                                  unsigned char level)
{
    if (node->m_used == 0xF || node->m_size < size)
        return 0;
    if (node->m_size == size && !node->m_free)
        return 0;
    if (node->m_size == size) {
        if (node->m_free == 1) {
            node->m_free = 0;
            node->m_used = 0xF;
            freeCounts[level]--;
            level++;
            int count = 4;
            for (; level < 5; level++) {
                freeCounts[level] -= count;
                count *= 4;
            }
            return node->m_data;
        }
    }
    TRY_CHILD(0, 1)
    TRY_CHILD(1, 2)
    TRY_CHILD(2, 4)
    TRY_CHILD(3, 8)
    return 0;
}
#pragma opt_strength_reduction reset


// ---- src/IpuCacheTree/RemoveFromCacheTree.cpp
// Retail built this file with strength reduction off: the free-count loop
// recomputes &freeCounts[level] each pass instead of stepping a pointer.
#pragma opt_strength_reduction off




// After freeing inside a child: the node becomes free again once all four
// children are free.
#define MERGE_CHILDREN()                                                        \
    if (!node->m_free && node->m_children[0]->m_free && node->m_children[1]->m_free && \
        node->m_children[2]->m_free && node->m_children[3]->m_free) {         \
        node->m_free = 1;                                                       \
        if (!node->m_used)                                                      \
            freeCounts[level]++;                                                \
    }

// Frees the block at data (size x size) and updates the per-level free counts.
void IpuCacheTree::RemoveFromCacheTree(_CacheTreeNode *node, unsigned char *data, int size,
                                       unsigned short *freeCounts, unsigned char level)
{
    if (node->m_size < size)
        return;
    if (node->m_data == data && node->m_size == size) {
        node->m_free = 1;
        node->m_used = 0;
        freeCounts[level]++;
        level++;
        int count = 4;
        for (; level < 5; level++) {
            freeCounts[level] += count;
            count *= 4;
        }
        return;
    }
    if (data < node->m_children[1]->m_data) {
        node->m_used &= 0xE;
        RemoveFromCacheTree(node->m_children[0], data, size, freeCounts, level + 1);
        MERGE_CHILDREN();
    } else if (data < node->m_children[2]->m_data) {
        node->m_used &= 0xD;
        RemoveFromCacheTree(node->m_children[1], data, size, freeCounts, level + 1);
        MERGE_CHILDREN();
    } else if (data < node->m_children[3]->m_data) {
        node->m_used &= 0xB;
        RemoveFromCacheTree(node->m_children[2], data, size, freeCounts, level + 1);
        MERGE_CHILDREN();
    } else {
        node->m_used &= 0x7;
        RemoveFromCacheTree(node->m_children[3], data, size, freeCounts, level + 1);
        MERGE_CHILDREN();
    }
}
#pragma opt_strength_reduction reset


// ---- src/IpuCache/ctor.cpp
extern "C" void *malloc(unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);




IpuCache::IpuCache(int numTrees)
{
    m_lastHandle = 0;
    m_cacheMem = malloc(0x20000);
    if (numTrees < 12)
        m_numTrees = 12;
    else
        m_numTrees = numTrees;
    m_cacheSize = numTrees * 0x20000;
    m_trees = new IpuCacheTree[m_numTrees];
    int count = m_numTrees * 85;
    // 12 bytes per entry, spelled as retail's unfolded (n << 3) + (n << 2): a
    // constant * 12 would fold into the * 8.
    int size = count * 4;
    int wide = count * 8;
    size = (wide + size) * 8;
#ifdef SPLASHDOWN_PC
    size = count * 4 * sizeof(_CacheTreeNode); // the 12 * 8 above is 4 nodes of 0x18 bytes
#endif
    LpCacheMem = (_CacheTreeNode *)(m_entries = malloc(size));
    memset(LpCacheMem, 0, size);
    for (int i = 0; i < m_numTrees; i++)
        m_trees[i].Create();
    memset(m_handles, 0, sizeof(m_handles));
    memset(m_used, 0, sizeof(m_used));
    m_outOfSpace = 0;
}


// ---- src/IpuCacheHandle/ctor.cpp

IpuCacheHandle::IpuCacheHandle()
{
    m_data = 0;
    m_tree = 0;
    m_mip = 0;
    m_x14_bit0 = 0;
    m_used = 0;
    m_decompress = 0;
    m_x14_bits3 = 0;
}


// ---- src/IpuCache/dtor.cpp




IpuCache::~IpuCache()
{
    for (int i = 0; i < m_numTrees; i++)
        m_trees[i].Release();
    if (m_trees)
        delete[] m_trees;
    if (m_entries)
        free(m_entries);
    LpCacheMem = 0;
    if (m_cacheMem)
        free(m_cacheMem);
}


// ---- src/IpuCache/PrepareCacheForNewFrame.cpp
class PSXMipSurface;

int MapCurrentRenderLoop(int frame);


void IpuCache::PrepareCacheForNewFrame(int frame)
{
    int loop = MapCurrentRenderLoop(frame);
    for (int i = 0; i <= m_lastHandle; i++) {
        if (loop == -1 || loop == m_handles[i].m_x14_bits3) {
            m_handles[i].m_decompress = 0;
            m_handles[i].m_x14_bits3 ^= loop;
        }
    }
}


// ---- src/IpuCache/ClearUnusedTexturesFromCache.cpp

void IpuCache::ClearUnusedTexturesFromCache(int all)
{
    int i;
    int last = m_lastHandle;
    if (all) {
        for (i = 0; i < 256; i++) {
            if (m_handles[i].m_used == 1)
                RemoveCacheHandle(&m_handles[i]);
        }
        m_lastHandle = 0;
    } else {
        for (i = last; i >= 0; i--) {
            if (!m_handles[i].m_decompress && m_handles[i].m_used == 1) {
                RemoveCacheHandle(&m_handles[i]);
                if (m_lastHandle == i)
                    m_lastHandle--;
            }
        }
    }
}


// ---- src/IpuCache/RemoveTextureFromCache.cpp
class PSXTextureMap;


void IpuCache::RemoveTextureFromCache(PSXTextureMap *texture)
{
    int i;
    IpuCacheHandle *handle;
    for (i = 0, handle = m_handles; i <= m_lastHandle; handle++, i++) {
        if (texture == handle->m_texture && handle->m_used == 1) {
            RemoveCacheHandle(handle);
            if (m_lastHandle == i)
                m_lastHandle--;
        }
    }
}


// ---- src/IpuCache/AllocateCacheHandle.cpp
void PSXprintf(const char *fmt, ...);






void IpuCache::AllocateCacheHandle(int slot)
{
    TransferSlotAssignment *entries = ((PSXRenderTarget *)LpRenderTarget)->m_buildQueue == 1 ? TransferList.m_buffers[0].m_items : TransferList.m_buffers[1].m_items;
    int count = ((PSXRenderTarget *)LpRenderTarget)->m_buildQueue == 1 ? TransferList.m_buffers[0].m_count : TransferList.m_buffers[1].m_count;
    PSXMipSurface *mip = IPUSlotList[slot].m_mip;
    IpuCacheHandle *handle = 0;
    if (!GetAvailableIpuCacheHandle(handle, mip->m_width)) {
        PSXprintf("Ran out of IPU cache space!!!!!!\n");
        handle->m_data = (unsigned char *)m_cacheMem;
        m_outOfSpace = 1;
    }
    handle->m_used = 1;
    handle->m_decompress = 1;
    handle->m_texture = IPUSlotList[slot].m_texture;
    handle->m_mip = mip;
    IPUSlotList[slot].m_handle = handle;
    for (int i = 0; i < count; i++) {
        if (IPUSlotList[slot].m_texture == entries[i].m_texture)
            entries[i].m_handle = handle;
    }
}


// ---- src/IpuCache/PlaceDecompressedTextureIntoCache.cpp

typedef int int128_t __attribute__((mode(TI)));

#define D3_CHCR (*(volatile unsigned int *)0x1000B000)


// Rearranges the IPU's macroblock-ordered output (16x16 32-bit pixels each)
// into a linear texture.
int IpuCache::PlaceDecompressedTextureIntoCache(IpuCacheHandle *handle)
{
    PSXMipSurface *mip;
    int rowOffset;
    volatile unsigned int *chcr;
    int blocks;
    int last;
    int128_t *src;
    int128_t *dst;
    int y;
    int x;
    int row;
    int stride;
    mip = handle->m_mip;
    handle->m_x14_bit0 = 1;
    switch (mip->m_width) {
    case 256:
        blocks = 16;
        break;
    case 128:
        blocks = 8;
        break;
    case 64:
        blocks = 4;
        break;
    case 32:
        blocks = 2;
        break;
    case 16:
        blocks = 1;
        break;
    default:
#line 856 "PSXTextureMapManager.cpp"
        ASSERTF(0, ("PlaceDecompressedTextureIntoCache: mipSurface->TextureWidth [%d] out of range", mip->m_width));
        break;
    }
    src = (int128_t *)m_cacheMem;
    stride = blocks * 2;
    last = blocks - 1;
    y = 0;
    chcr = &D3_CHCR;
    for (; y < blocks; y++) {
        rowOffset = y * stride * 256;
        for (x = 0; x < blocks; x++) {
            // Wait for the IPU's DMA to finish before the last macroblock.
            if (y == last && x == last) {
                while (*chcr & 0x100)
                    ;
            }
            dst = (int128_t *)((char *)handle->m_data + rowOffset) + x * 2;
            for (row = 0; row < 16; row++) {
                // Two quadwords per row: both src++ go to fresh registers, as
                // retail's inline asm operands do.
                asm("lq $8, 0(%0); lq $9, 0(%1); sq $8, 0(%2); sq $9, 16(%2)" : : "r"(src++), "r"(src++), "r"(dst) : "$8", "$9", "memory");
                dst += stride;
            }
        }
    }
    return 1;
}


// ---- src/IpuCache/GetAvailableIpuCacheHandle.cpp





// Finds a free handle (unless one is passed in) and a width x width block in
// the cache trees for it: 256 blocks from the first tree with one free,
// smaller ones preferably from the trees nearest the end.
int IpuCache::GetAvailableIpuCacheHandle(IpuCacheHandle *&handle, int width)
{
    int i;
    if (!handle) {
        for (i = 0; i < 256; i++) {
            if (!m_handles[i].m_used) {
                handle = &m_handles[i];
                break;
            }
        }
        if (m_lastHandle < i)
            m_lastHandle = i;
    }
    if (handle) {
        handle->m_x10 = width;
        if (width == 256) {
            for (i = 0; i <= m_numTrees - 4; i++) {
                handle->m_data = m_trees[i].RetrieveCacheAddress(&m_trees[i].m_root, width, m_trees[i].m_freeCounts, 0);
                if (handle->m_data) {
                    handle->m_tree = &m_trees[i];
                    m_used[0]++;
                    return 1;
                }
            }
        } else {
            int level = 0;
            int best = -1;
            int fewest = 0x101;
            if (width == 128)
                i = m_numTrees - 3;
            else if (width == 64)
                i = m_numTrees - 2;
            else
                i = m_numTrees - 1;
            switch (width) {
            case 16:
                level = 4;
                break;
            case 32:
                level = 3;
                break;
            case 64:
                level = 2;
                break;
            case 128:
                level = 1;
                break;
            case 256:
                level = 0;
                break;
            default:
#line 1103 "PSXTextureMapManager.cpp"
                ASSERT(0, "Invalid width specified!\n");
                break;
            }
            if (m_trees[i].m_freeCounts[level] > 0) {
                best = i;
            } else if (m_trees[i - 1].m_freeCounts[level] > 0 && width <= 64) {
                best = i - 1;
            } else if (m_trees[i - 2].m_freeCounts[level] > 0 && width <= 32) {
                best = i - 2;
            } else {
                // The redundant initializer gives retail's entry test.
                for (i = i; i >= 0; i--) {
                    int count = m_trees[i].m_freeCounts[level];
                    if (count > 0 && count < fewest) {
                        fewest = count;
                        best = i;
                    }
                }
            }
            if (best != -1) {
                handle->m_data = m_trees[best].RetrieveCacheAddress(&m_trees[best].m_root, width,
                                                                    m_trees[best].m_freeCounts, 0);
                if (handle->m_data) {
                    handle->m_tree = &m_trees[best];
                    m_used[level]++;
                    return 1;
                }
            }
        }
        handle->m_data = 0;
        return 0;
    }
#line 1149 "PSXTextureMapManager.cpp"
    ASSERT(0, "GetAvailableIpuCacheHandle: No cache handles left");
    return 0;
}


// ---- src/IpuCache/RemoveCacheHandle.cpp
#pragma alignlabel off


struct _CacheTreeNode;



// Frees a cache handle and returns its block to the cache tree.
void IpuCache::RemoveCacheHandle(IpuCacheHandle *handle)
{
    int index;
    handle->m_used = 0;
    handle->m_decompress = 0;
    handle->m_x14_bit0 = 0;
    handle->m_mip = 0;
    handle->m_texture = 0;
    handle->m_x14_bits3 = 0;
    if (handle->m_data != m_cacheMem) {
        IpuCacheTree *tree = handle->m_tree;
        tree->RemoveFromCacheTree((_CacheTreeNode *)tree, handle->m_data, handle->m_x10, tree->m_freeCounts, 0);
        switch (handle->m_x10) {
        case 0x10:
            index = 4;
            break;
        case 0x20:
            index = 3;
            break;
        case 0x40:
            index = 2;
            break;
        case 0x80:
            index = 1;
            break;
        case 0x100:
            index = 0;
            break;
        default:
            // retail then decrements m_used[index] with index unset
#line 1191 "PSXTextureMapManager.cpp"
            ASSERT(0, "Invalid width specified!\n");
            break;
        }
        m_used[index]--;
    }
}
#pragma alignlabel reset


// ---- src/IpuCache/CheckTextureForDecompression.cpp

IpuCacheHandle *IpuCache::CheckTextureForDecompression(PSXTextureMap *texture)
{
    IpuCacheHandle *found = 0;
    PSXMipSurface *mip = texture->GetReqdMipLevel();
    for (int i = 0; i <= m_lastHandle; i++) {
        if (texture == m_handles[i].m_texture && m_handles[i].m_used == 1) {
            found = &m_handles[i];
            if (mip->m_width == m_handles[i].m_x10) {
                m_handles[i].m_decompress = 1;
                return &m_handles[i];
            }
        }
    }
    if (found) {
        found->m_decompress = 1;
        return found;
    }
    return 0;
}


// ---- src/IpuCache/CheckMipSurfaceForDecompression.cpp
class PSXMipSurface;


IpuCacheHandle *IpuCache::CheckMipSurfaceForDecompression(PSXMipSurface *surface)
{
    for (int i = 0; i <= m_lastHandle; i++) {
        if (surface == m_handles[i].m_mip && m_handles[i].m_used == 1) {
            m_handles[i].m_decompress = 1;
            return &m_handles[i];
        }
    }
    return 0;
}


// ---- src/IpuCache/CheckForTextureInCache.cpp

int IpuCache::CheckForTextureInCache(IpuCacheHandle *&handle, PSXTextureMap *texture)
{
    static int MeterID = -1;    // left over from a compiled-out profiling meter
    PSXMipSurface *mip = texture->GetReqdMipLevel();
    for (int i = 0; i <= m_lastHandle; i++) {
        if (texture == m_handles[i].m_texture && m_handles[i].m_used == 1) {
            IpuCacheHandle *h = &m_handles[i];
            handle = h;
            return h->m_mip == mip;
        }
    }
    handle = 0;
    return 0;
}


// ---- src/IpuCache/GetAvailableSpaceFromCache.cpp


int IpuCache::GetAvailableSpaceFromCache(int size)
{
    static int MeterID = -1;            // unused profiler id
    int total = 0;
    int last = m_numTrees - 1;
    if (size == 0x100)
        last -= 3;
    else if (size == 0x80)
        last -= 2;
    else if (size == 0x40)
        last -= 1;
    for (int i = 0; i <= last; i++) {
        switch (size) {
        case 0x100:
            total += m_trees[i].m_freeCounts[0];
            break;
        case 0x80:
            total += m_trees[i].m_freeCounts[1];
            break;
        case 0x40:
            total += m_trees[i].m_freeCounts[2];
            break;
        case 0x20:
            total += m_trees[i].m_freeCounts[3];
            break;
        case 0x10:
            total += m_trees[i].m_freeCounts[4];
            break;
        }
    }
    return total;
}


// ---- src/IpuCache/GetOccupiedSpaceFromCache.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off



int IpuCache::GetOccupiedSpaceFromCache(int size)
{
    switch (size) {
    case 0x100:
        return m_used[0];
    case 0x80:
        return m_used[1];
    case 0x40:
        return m_used[2];
    case 0x20:
        return m_used[3];
    case 0x10:
        return m_used[4];
    default:
#line 1333 "PSXTextureMapManager.cpp"
        ASSERT(0, "Invalid size requested.\n");
        return 0;
    }
}
#pragma alignlabel reset


// ---- src/PSXTextureMapManager/ctor.cpp


PSXTextureMapManager::PSXTextureMapManager()
{
    m_currentSlot = 0;
    LastSlot = 0;
    m_ipuCache = 0;
    LastTextureMap = 0;
    m_gsTransfer = 0;
    m_ipuTransfer = 0;
    m_x110 = 0;
    m_x108 = 0;
    m_x114 = 0;
    m_x118 = 0;
    m_x11C = 0;
}


// ---- src/PSXTextureMapManager/dtor.cpp



PSXTextureMapManager::~PSXTextureMapManager()
{
    if (m_ipuCache)
        delete m_ipuCache;
    if (m_gsTransfer)
        delete m_gsTransfer;
    if (m_ipuTransfer)
        delete m_ipuTransfer;
    if (m_x118)
        delete m_x118;
    if (m_x114) {
        // Strip the segment bits (an uncached alias) before freeing.
        m_x114 = (unsigned int *)PS2_ADDR(((unsigned int)m_x114 & 0x0FFFFFFF));
        free(m_x114);
    }
}


// ---- src/PSXTextureMapManager/Create.cpp
#pragma alignlabel off


extern "C" void *memalign(unsigned int alignment, unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);
extern "C" void FlushCache(int operation);
extern "C" int AddIntcHandler(int cause, int (*handler)(int), int next);
extern "C" int EnableIntc(int cause);

int ManualTextureSynchronizationInterrupt(int cause);






PSXTextureMapManager *PSXTextureMapManager::Create()
{
    int mode = LpVideoCard->m_mode;
    int freeBlocks = 0x4000 - LpVideoCard->m_videoModes[mode].m_vramAddress / 256;
    if (freeBlocks > 0x3800) {
#line 1377 "PSXTextureMapManager.cpp"
        ASSERT(0, "Create: Video Resolution is too high; there is not sufficient texture memory.");
    }
    TextureMapManager::Create();
    m_gsTransfer = (new GsTextureTransfer())->Create(0x609);
    m_ipuTransfer = new IpuTextureTransfer();
    AddIntcHandler(5, ManualTextureSynchronizationInterrupt, 0);
    EnableIntc(5);
    m_x90.Start((DmaRenderDestination)1, 0x80);
    m_x90.DmaAddVifCode(0x93000000);
    m_x90.End();
    m_x50.Start((DmaRenderDestination)1, 0x80);
    m_x50.DmaAddVifCode(0x11000000);
    m_x50.DmaAddVifCode(0x03000000);
    m_x50.DmaAddVifCode(0x02000000);
    m_x50.DmaBeginUnpack(0x320, 0xC);
    sceGifTag tag;
    memset(&tag, 0, sizeof(tag));
    tag.NLOOP = 1;
    tag.EOP = 1;
    tag.PRE = 1;
    tag.FLG = 0;
    tag.NREG = 1;
    tag.REGS0 = 0xF;
    m_x50.DmaAddGifTagToUnpack(&tag);
    m_x50.DmaEndUnpack();
    m_x50.DmaAddVifCode(0x14000000);
    m_x50.DmaAddVifCode(0x10000000);
    m_x50.DmaAddVifCode(0x14000000);
    m_x50.End();
    m_xC0.Start((DmaRenderDestination)1, 0x80);
    m_xC0.DmaAddVifCode(0x13000000);
    m_xC0.End();
    m_x114 = (unsigned int *)memalign(16, 16);
    memset(m_x114, 0, 16);
    m_x118 = new GsTextureTransfer();
    m_x118->Create(10);
    FlushCache(0);
    // (redundant cast: retail loads the texture pointer first)
    m_x118->PrepareTransferTexture((unsigned char *)m_x114, 0x37FF, 2, 2, 0, 0, 0, 32, 0);
    m_x118->SendTexture(1);
    m_x114 = (unsigned int *)PS2_ADDR((((unsigned int)m_x114 & 0x0FFFFFFF) | EE_UNCACHED));
    return this;
}
#pragma alignlabel reset


// ---- src/PSXTextureMapManager/AllocateIpuCache.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off




void PSXTextureMapManager::AllocateIpuCache(int memInBytes)
{
    if (memInBytes < 0x100000) {
#line 1457 "PSXTextureMapManager.cpp"
        ASSERTF(0, ("invalid input? memInBytes=%d, less than 1 meg for the IPU cache...", memInBytes));
    }
    int blocks = (memInBytes * 8) / (1024 * 1024);  // 8 blocks per MB
    FreeIpuCache();
    m_ipuCache = new IpuCache(blocks);
}
#pragma alignlabel reset


// ---- src/PSXTextureMapManager/FreeIpuCache.cpp




void PSXTextureMapManager::FreeIpuCache()
{
    ((PSXRenderTarget *)LpRenderTarget)->StopRenderQueue();
    if (m_ipuCache)
        delete m_ipuCache;
    m_ipuCache = 0;
}


// ---- src/PSXTextureMapManager/RemoveTextureFromIpuCache.cpp

void PSXTextureMapManager::RemoveTextureFromIpuCache(PSXTextureMap *texture)
{
    if (m_ipuCache)
        m_ipuCache->RemoveTextureFromCache(texture);
}


// ---- src/PSXTextureMapManager/InvalidateIpuCache.cpp

void PSXTextureMapManager::InvalidateIpuCache()
{
    if (m_ipuCache)
        m_ipuCache->m_outOfSpace = 1;
}


// ---- src/PSXTextureMapManager/Tick.cpp




// This file's page in the debug overlay, claimed on first use.
static int DebugOverlayID = -1;






int PSXTextureMapManager::Tick(float dt)
{
    if (LpGame->m_debugOverlay) {
        if (DebugOverlayID < 0)
            DebugOverlayID = LpGame->m_debugOverlay->m_numPages++;
        PeakTextureReqBytes.Record(nTextureReqBytesMax);
        PeakTexturesDecompressed.Record(nTexturesDecompressedMax);
        PeakBytesDecompressed.Record(nBytesDecompressedMax);
        int total = 0;
        int ipu = 0;
        int bloated = 0;
        int pal4 = 0;
        int pal8 = 0;
        for (PSXTextureMap *t = (PSXTextureMap *)m_store->m_textures.First(); t; t = (PSXTextureMap *)m_store->m_textures.Next()) {
            total++;
            if (t->m_compressed)
                ipu++;
            if (t->m_x4)
                bloated++;
            if (t->m_bitsPerPixel == 4)
                pal4++;
            if (t->m_bitsPerPixel == 8)
                pal8++;
        }
        LpGame->m_debugOverlay->PrintfHeading(DebugOverlayID, "PSXTextureMapManager");
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "TextureReqBytes: %08d (%08d)", nTextureReqBytesMax, PeakTextureReqBytes.GetPeak());
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "TextureReq256: %d", n256ReqMax);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "TextureReq128: %d", n128ReqMax);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "TextureReq64: %d", n64ReqMax);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "TextureReq32: %d", n32ReqMax);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "TextureReq16: %d", n16ReqMax);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "");
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "Total Textures: %d", total);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "IPU Count %d", ipu);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "PAL4 Count %d", pal4);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "PAL8 Count %d", pal8);
        LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "Bloated Count %d", bloated);
        if (m_ipuCache) {
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "");
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "IPUCacheSize: %d", m_ipuCache->m_numTrees << 17);
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "");
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "Tex Decompressed: %03d (%03d)", nTexturesDecompressedMax, PeakTexturesDecompressed.GetPeak());
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "Bytes Decompressed: %07d (%07d)", nBytesDecompressedMax, PeakBytesDecompressed.GetPeak());
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "");
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "Textures in Cache Trees:");
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "                  Taken     Free");
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "256x256 Textures: %04d      %04d", n256, n256free);
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "128x128 Textures: %04d      %04d", n128, n128free);
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "64 x 64 Textures: %04d      %04d", n64, n64free);
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "32 x 32 Textures: %04d      %04d", n32, n32free);
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "16 x 16 Textures: %04d      %04d", n16, n16free);
            LpGame->m_debugOverlay->PrintfDetail(DebugOverlayID, "nTotalTransferedTextures: %d", nTotalTransferedTextures);
        }
        nTotalTransferedTextures = 0;
    }
    return TextureMapManager::Tick(dt);
}


// ---- src/PSXTextureMapManager/PrepareGeometry.cpp


int PSXTextureMapManager::PrepareGeometry()
{
    // Profiling timer handle; its uses were compiled out of retail.
    static int timerID = -1;
    m_store->ClearUsage();
    return TextureMapManager::PrepareGeometry();
}


// ---- src/PSXTextureMapManager/RenderPre3D.cpp



// Fits as many compressed textures as possible into the IPU cache: counts the
// cache blocks each size of mip needs, and while they don't fit lowers the
// mip level of the biggest textures ("crushes" them) a step at a time.
int PSXTextureMapManager::RenderPre3D()
{
    static int timerID = -1;
    if (m_ipuCache) {
        static int Pre3DID = -1;
        int n32;
        PSXMipSurface *mip;
        int numHandles;
        PSXTextureMap *texture;
        int n256;
        int n128;
        int n64;
        int i;
        int crush;
        int width;
        int cachedWidth;
        int found;
        IpuCacheHandle *handle;
        int count;
        int n16;
        int avail256;
        int avail128;
        int avail64;
        int avail32;
        int avail16;
        int iteration;
        int recalc;
        int rewrite;
        float usage;
        IpuCacheHandle *handles[256];
        PSXTextureMap *textures[256];

        m_x110 = 0;
        count = 0;
        numHandles = 0;
        n256 = 0;
        n128 = 0;
        n64 = 0;
        n32 = 0;
        n16 = 0;
        if (m_ipuCache->m_outOfSpace) {
            m_ipuCache->ClearUnusedTexturesFromCache(1);
            m_ipuCache->PrepareCacheForNewFrame(-1);
            m_ipuCache->m_outOfSpace = 0;
        } else {
            m_ipuCache->PrepareCacheForNewFrame(LpGame->m_currentPlayer);
        }
        static int Pre3DGIID = -1;
        for (texture = (PSXTextureMap *)m_store->m_textures.First(); texture;
             texture = (PSXTextureMap *)m_store->m_textures.Next()) {
            if (texture && texture->m_compressed) {
                usage = texture->m_usage;
                if (usage > 0.0f) {
                    textures[count] = texture;
                    count++;
#line 1645 "PSXTextureMapManager.cpp"
                    ASSERT(count < 256, "Maximum IpuTexMapList exceeded. Must be increased.");
                    texture->CalculateReqdMipLevel(0.0f);
                    mip = texture->GetReqdMipLevel();
                    handle = m_ipuCache->CheckTextureForDecompression(texture);
                    if (!handle || (handle && handle->m_x10 != mip->m_width)) {
                        switch (mip->m_width) {
                        case 256:
                            n256++;
                            break;
                        case 128:
                            n128++;
                            break;
                        case 64:
                            n64++;
                            break;
                        case 32:
                            n32++;
                            break;
                        case 16:
                            n16++;
                            break;
                        default:
#line 1673 "PSXTextureMapManager.cpp"
                            ASSERTF(0, ("RenderPre3D: mipSurface->TextureWidth (%d) out of range", mip->m_width));
                        }
                        if (handle && handle->m_x10 > mip->m_width)
                            m_ipuCache->RemoveCacheHandle(handle);
                        else if (handle && handle->m_x10 < mip->m_width)
                            handles[numHandles++] = handle;
                    }
                }
            }
        }
        m_ipuCache->ClearUnusedTexturesFromCache(0);
        avail256 = m_ipuCache->GetAvailableSpaceFromCache(256);
        avail128 = m_ipuCache->GetAvailableSpaceFromCache(128);
        avail64 = m_ipuCache->GetAvailableSpaceFromCache(64);
        avail32 = m_ipuCache->GetAvailableSpaceFromCache(32);
        avail16 = m_ipuCache->GetAvailableSpaceFromCache(16);
        crush = 0;
        if (avail256 - n256 < 0)
            crush = 1;
        if (avail128 - n256 * 4 - n128 < 0)
            crush = 1;
        if (avail64 - n256 * 16 - n128 * 4 - n64 < 0)
            crush = 1;
        if (avail32 - n256 * 64 - n128 * 16 - n64 * 4 - n32 < 0)
            crush = 1;
        if (avail16 - n256 * 256 - n128 * 64 - n64 * 16 - n32 * 4 - n16 < 0)
            crush = 1;
        if (!crush) {
            for (i = 0; i < numHandles; i++)
                m_ipuCache->RemoveCacheHandle(handles[i]);
            // A statement that generates no code (retail's was probably a
            // compiled-out debug macro). It ends the if block with a
            // non-loop statement, so mwcc doesn't pad the label after the
            // loop to 8 bytes (labels get padded when the block before them
            // is loop code).
            i = i;
        }
        static int Pre3DCrushID = -1;
        iteration = 0;
        recalc = 0;
        while (crush) {
            n256 = 0;
            n128 = 0;
            n64 = 0;
            n32 = 0;
            n16 = 0;
            m_x110 = 1;
            for (i = 0; i < count; i++) {
                texture = textures[i];
                usage = texture->m_usage;
                if (usage > 4.0f && texture->m_numMips != 1) {
                    usage -= 0.25f;
                    texture->m_usage = usage;
                    texture->CalculateReqdMipLevel(0.0f);
                }
                mip = texture->GetReqdMipLevel();
                if (iteration == 0) {
                    found = m_ipuCache->CheckForTextureInCache(handle, texture);
                    handles[i] = handle;
                } else {
#line 1763 "PSXTextureMapManager.cpp"
                    ASSERT(i < 256, "Maximum TempHandleList exceeded. Must be increased.");
                    handle = handles[i];
                    if (handle && handle->m_x10 == mip->m_width)
                        found = 1;
                    else
                        found = 0;
                }
                if (!found) {
                    width = mip->m_width;
                    cachedWidth = 0;
                    rewrite = 0;
                    if (handle)
                        cachedWidth = handle->m_x10;
                    if (width == 256) {
                        n256++;
                        if (handle && avail256 < n256 && handle->m_x10 <= 128) {
                            usage = 7.4f;
                            n256--;
                            width = 128;
                            rewrite = 1;
                        }
                    }
                    if (width == 128 && cachedWidth != 128) {
                        n128++;
                        if (handle && avail128 < n128 && handle->m_x10 <= 64) {
                            usage = 6.4f;
                            n128--;
                            width = 64;
                            rewrite = 1;
                        }
                    }
                    if (width == 64 && cachedWidth != 64) {
                        n64++;
                        if (handle && avail64 < n64 && handle->m_x10 <= 32) {
                            usage = 5.4f;
                            n64--;
                            width = 32;
                            rewrite = 1;
                        }
                    }
                    if (width == 32 && cachedWidth != 32) {
                        n32++;
                        if (handle && avail32 < n32 && handle->m_x10 <= 16) {
                            usage = 4.4f;
                            n32--;
                            width = 16;
                            rewrite = 1;
                        }
                    }
                    if (width == 16 && cachedWidth != 16)
                        n16++;
                    if (width < 16 || width > 256) {
#line 1833 "PSXTextureMapManager.cpp"
                        ASSERTF(0, ("RenderPre3D: mipSurface->TextureWidth (%d) out of range", mip->m_width));
                    }
                    if (rewrite == 1) {
                        texture->m_usage = usage;
                        texture->CalculateReqdMipLevel(0.0f);
                    }
                    if (handle && handle->m_x10 != width) {
                        m_ipuCache->RemoveCacheHandle(handle);
                        handles[i] = 0;
                        recalc = 1;
                    }
                }
            }
            iteration++;
            crush = 0;
            if (iteration > 50) {
                m_ipuCache->ClearUnusedTexturesFromCache(1);
                recalc = 1;
                InvalidateIpuCache();
            }
            if (recalc) {
                avail256 = m_ipuCache->GetAvailableSpaceFromCache(256);
                avail128 = m_ipuCache->GetAvailableSpaceFromCache(128);
                avail64 = m_ipuCache->GetAvailableSpaceFromCache(64);
                avail32 = m_ipuCache->GetAvailableSpaceFromCache(32);
                avail16 = m_ipuCache->GetAvailableSpaceFromCache(16);
                recalc = 0;
            }
            if (avail256 - n256 < 0)
                crush = 1;
            if (avail128 - n256 * 4 - n128 < 0)
                crush = 1;
            if (avail64 - n256 * 16 - n128 * 4 - n64 < 0)
                crush = 1;
            if (avail32 - n256 * 64 - n128 * 16 - n64 * 4 - n32 < 0)
                crush = 1;
            if (avail16 - n256 * 256 - n128 * 64 - n64 * 16 - n32 * 4 - n16 < 0)
                crush = 1;
        }
    }
    return TextureMapManager::RenderPre3D();
}


// ---- src/PSXTextureMapManager/Render3D.cpp


int PSXTextureMapManager::Render3D()
{
    // Profiling timer handle; its uses were compiled out of retail.
    static int timerID = -1;
    DebugDump = 0;
    ResetFrame();
    return TextureMapManager::Render3D();
}


// ---- src/PSXTextureMapManager/RenderPost3D.cpp



int PSXTextureMapManager::RenderPost3D()
{
    return TextureMapManager::RenderPost3D();
}


// ---- src/PSXTextureMapManager/KeyDownHandler.cpp

int PSXTextureMapManager::KeyDownHandler(ProcessNode *node, KeyInfo *key)
{
    if (KEYDOWN_HANDLER_TEST(0xC, DEVICE_KEYBOARD, node, 0x80))
        m_ipuCache->m_outOfSpace = 1;
    return TextureMapManager::KeyDownHandler(node, key);
}


// ---- src/PSXTextureMapManager/Restore.cpp


int PSXTextureMapManager::Restore()
{
    return GameObject::Restore();     // TextureMapManager::Restore is pure
}


// ---- src/PSXTextureMapManager/RemoveFromVideoMemory.cpp

void PSXTextureMapManager::RemoveFromVideoMemory() {}


// ---- src/PSXTextureMapManager/AddTextureMap.cpp
class TextureMap;



void PSXTextureMapManager::AddTextureMap(TextureMap *map)
{
    m_store->AddManagedTexture(map);
}


// ---- src/PSXTextureMapManager/RemoveTextureMap.cpp
class TextureMap;



void PSXTextureMapManager::RemoveTextureMap(TextureMap *map)
{
    m_store->RemoveManagedTexture(map);
}


// ---- src/PSXTextureMapManager/SetupCache.cpp

int PSXTextureMapManager::SetupCache()
{
    return 1;
}


// ---- src/PSXTextureMapManager/DisplayTexInRAM.cpp

void PSXTextureMapManager::DisplayTexInRAM() {}


// ---- src/PSXTextureMapManager/ShowUsage.cpp

void PSXTextureMapManager::ShowUsage() {}


// ---- src/PSXTextureMapManager/AddTextureToDecompressionQueue.cpp
class PSXMipSurface;






IpuCacheHandle *PSXTextureMapManager::AddTextureToDecompressionQueue(PSXTextureMap *texture)
{
    if (!texture->m_compressed)
        return 0;
    PSXMipSurface *surface = texture->GetReqdMipLevel();
    IPUSlotList[MaxIPUIndex].m_texture = texture;
    IPUSlotList[MaxIPUIndex].m_mip = surface;
    IPUSlotList[MaxIPUIndex].m_handle = 0;
    for (int i = 0; i < MaxIPUIndex; i++) {
        if (IPUSlotList[i].m_texture == texture)
            return IPUSlotList[i].m_handle;
    }
    IpuCacheHandle *result = m_ipuCache->CheckMipSurfaceForDecompression(surface);
    if (result)
        return result;
    if (m_ipuCache)
        m_ipuCache->AllocateCacheHandle(MaxIPUIndex);
    result = IPUSlotList[MaxIPUIndex].m_handle;
    MaxIPUIndex++;
    return result;
}


// ---- src/PSXTextureMapManager/AddTextureToGifList.cpp

struct _sceDmaTag;






class PSXTextureMap;


struct GsTextureTransfer;


int WillFitInSlot(PSXTextureMap *map);
int MapCurrentRenderLoop(int player);



// Assigns the texture a VRAM slot (two adjacent slots when it does not fit
// in one) and queues its transfer.
void PSXTextureMapManager::AddTextureToGifList(PSXTextureMap *map)
{
//PB
    int fits;
    PSXMipSurface *mip;
    unsigned char *data;
    TransferSlotAssignment *entries;
    int count;
//PE
    AddInterrupt = 1;
    if (m_x108 == 1) {
        if (m_x104)
            ((PSXRenderTarget *)LpRenderTarget)->AddIndexedItemToRenderQueue(m_x104, &m_xC0);
        AddInterrupt = 1;
        if (!((PSXRenderTarget *)LpRenderTarget)->TrySendRenderQueue(0))
            AddTextureInterruptToRenderQueue();
        m_x108 = 0;
    }
    if (((PSXRenderTarget *)LpRenderTarget)->m_sendQueue == 1)
        entries = TransferList.m_buffers[0].m_items;
    else
        entries = TransferList.m_buffers[1].m_items;
    if (((PSXRenderTarget *)LpRenderTarget)->m_sendQueue == 1)
        count = TransferList.m_buffers[0].m_count;
    else
        count = TransferList.m_buffers[1].m_count;
    if (map == LastTextureMap) {
        map->GetReqdMipLevel();
        CalculateVideoMemoryAddresses(LastSlot, map, &entries[count - 1]);
        map->m_xEC = entries[count - 1].m_address;
        AddInterrupt = 0;
        return;
    }
    map->CalculateReqdMipLevel(0.0f);
    fits = WillFitInSlot(map);
    if (fits) {
        m_currentSlot++;
        if (m_currentSlot > 3)
            m_currentSlot = 0;
        while (m_slotLocked[m_currentSlot] == 1) {
            m_currentSlot++;
            if (m_currentSlot > 3)
                m_currentSlot = 0;
        }
    } else {
        m_currentSlot++;
        if (m_currentSlot == 1)
            m_currentSlot = 2;
        if (m_currentSlot >= 3)
            m_currentSlot = 0;
        while (m_slotLocked[m_currentSlot] == 1 || m_slotLocked[m_currentSlot + 1] == 1) {
            m_currentSlot += 2;
            if (m_currentSlot > 3)
                m_currentSlot = 0;
        }
    }
#line 2102 "PSXTextureMapManager.cpp"
    ASSERT(m_currentSlot != LastSlot, "AddTextureToGifList: CurrentSlot == LastSlot");
    LastSlot = m_currentSlot;
    TransferSlotAssignment *entry = &entries[count];
    entry->m_texture = map;
    entry->m_slot = m_currentSlot;
    CalculateVideoMemoryAddresses(m_currentSlot, map, entry);
    map->m_xEC = entries[count].m_address;
    map->m_xF0 = entries[count].m_clutAddress;
    mip = map->GetReqdMipLevel();
    data = (unsigned char *)mip->m_surface;
    if (m_ipuCache) {
        entries[count].m_handle = AddTextureToDecompressionQueue(map);
        if (entries[count].m_handle) {
            int loop = MapCurrentRenderLoop(LpGame->m_currentPlayer);
            entries[count].m_handle->m_x14_bits3 |= loop;
            data = entries[count].m_handle->m_data;
        }
    }
    map->PrepareTransferTexture(m_gsTransfer, map->m_xEC, map->m_xF0, mip, data);
    count++;
#line 2165 "PSXTextureMapManager.cpp"
    ASSERT(count < 0x400, "AddTextureToGifList: maxTransferIndex >= MAX_TRANSFER_SLOT_LIST");
    if (count < 0x400) {
        if (((PSXRenderTarget *)LpRenderTarget)->m_sendQueue == 1)
            TransferList.m_buffers[0].m_count = count;
        else
            TransferList.m_buffers[1].m_count = count;
        if (!fits)
            m_currentSlot++;
        if (DebugDump)
            PSXprintf("TMM::AddTextureToGifList\tS %d\tW %d\tBPP %d \tFit %d '%s'\n", LastSlot, mip->m_x2C,
                      map->m_bitsPerPixel, fits, map->m_name);
        LastTextureMap = map;
    }
}


// ---- src/PSXTextureMapManager/AddFillTextureToGifList.cpp
// AddFillTextureToGifList__20PSXTextureMapManagerFi  @ 0x002B7020


struct _sceDmaTag;






int PSXTextureMapManager::AddFillTextureToGifList(int color)
{
    AddInterrupt = 1;
    if (m_x108 == 1) {
        if (m_x104)
            ((PSXRenderTarget *)LpRenderTarget)->AddIndexedItemToRenderQueue(m_x104, &m_xC0);
        AddInterrupt = 1;
        if (!((PSXRenderTarget *)LpRenderTarget)->TrySendRenderQueue(0))
            AddTextureInterruptToRenderQueue();
        m_x108 = 0;
    }
    TransferSlotAssignment *entries;
    if (((PSXRenderTarget *)LpRenderTarget)->m_sendQueue == 1)
        entries = TransferList.m_buffers[0].m_items;
    else
        entries = TransferList.m_buffers[1].m_items;
    int count;
    if (((PSXRenderTarget *)LpRenderTarget)->m_sendQueue == 1)
        count = TransferList.m_buffers[0].m_count;
    else
        count = TransferList.m_buffers[1].m_count;
    if (color == m_x11C) {
        AddInterrupt = 0;
        return 0x37FF;
    }
    m_currentSlot++;
    if (m_currentSlot > 3)
        m_currentSlot = 0;
    while (m_slotLocked[m_currentSlot] == 1) {
        m_currentSlot++;
        if (m_currentSlot > 3)
            m_currentSlot = 0;
    }
#line 2220 "PSXTextureMapManager.cpp"
    ASSERT(m_currentSlot != LastSlot, "AddTextureToGifList: CurrentSlot == LastSlot");
    LastSlot = m_currentSlot;
    TransferSlotAssignment *entry = &entries[count];
    entry->m_texture = 0;
    entry->m_slot = m_currentSlot;
    entry->m_fillColor = color;
    entry->m_address = 0x37FF;
    count++;
#line 2232 "PSXTextureMapManager.cpp"
    ASSERT(count < 0x400, "AddTextureToGifList: maxTransferIndex >= MAX_TRANSFER_SLOT_LIST");
    if (count >= 0x400)
        return 0;
    if (((PSXRenderTarget *)LpRenderTarget)->m_sendQueue == 1)
        TransferList.m_buffers[0].m_count = count;
    else
        TransferList.m_buffers[1].m_count = count;
    m_x11C = color;
    return 0x37FF;
}


// ---- src/PSXTextureMapManager/LockTextureSlot.cpp

void PSXprintf(const char *fmt, ...);




int WillFitInSlot(PSXTextureMap *map);

void PSXTextureMapManager::LockTextureSlot(PSXTextureMap *map)
{
    AddInterrupt = 1;
    map->CalculateReqdMipLevel(0.0f);
    PSXMipSurface *mip = map->GetReqdMipLevel();
    if (mip->m_slot != -1) {
        map->m_xEC = GetSlotAddress(mip->m_slot);
        AddInterrupt = 0;
        return;
    }
    AddTextureToGifList(map);
    int slot = LastSlot;
    mip->m_slot = slot;
    int fits = WillFitInSlot(map);
    if (fits) {
        m_slotLocked[slot] = 1;
    } else {
        // Too big for one slot: lock the next one as well.
        m_slotLocked[slot] = 1;
        slot++;
        if (slot >= 4)
            slot = 0;
        m_slotLocked[slot] = 1;
    }
    if (DebugDump)
        PSXprintf("TMM::LockTextureSlot\tS %d\tW %d\tBPP %d \tFit %d '%s'\n", LastSlot, mip->m_x2C, map->m_bitsPerPixel, fits, map->m_name);
}


// ---- src/PSXTextureMapManager/UnlockTextureSlot.cpp




int WillFitInSlot(PSXTextureMap *map);






class PSXTextureMap;

void PSXTextureMapManager::UnlockTextureSlot(PSXTextureMap *map)
{
    if (!map)
        return;
    PSXMipSurface *mip = map->GetReqdMipLevel();
    if (!mip)
        return;
    int slot = mip->m_slot;
    if (slot == -1)
        return;
    int fits = WillFitInSlot(map);
    if (fits) {
        m_slotLocked[slot] = 0;
    } else {
        // Took two slots: free the next one as well.
        m_slotLocked[slot] = 0;
        slot++;
        if (slot >= 4)
            slot = 0;
        m_slotLocked[slot] = 0;
    }
    if ((slot == 0 && m_currentSlot == 3) || slot == m_currentSlot + 1) {
        // Move the fill position to the next unlocked slot.
        slot = m_currentSlot;
        do {
            slot++;
            if (slot > 3)
                slot = 0;
#line 2347 "PSXTextureMapManager.cpp"
            ASSERT(slot != m_currentSlot, "no available unlocked slots!");
        } while (m_slotLocked[slot]);
        m_currentSlot = slot;
    }
    if (DebugDump)
        PSXprintf("TMM::UnlockTextureSlot\tS %d\tFit %d '%s'\n", mip->m_slot, fits, map->m_name);
    mip->m_slot = -1;
}


// ---- src/PSXTextureMapManager/CalculateVideoMemoryAddresses.cpp
class PSXTextureMap;
void PSXTextureMapManager::CalculateVideoMemoryAddresses(int slot, PSXTextureMap *map, TransferSlotAssignment *assign)
{
    assign->m_address = GetSlotAddress(slot);
    assign->m_clutAddress = GetClutAddress(slot);
}


// ---- src/PSXTextureMapManager/AddTextureInterruptToRenderQueue.cpp




void *PSXTextureMapManager::AddTextureInterruptToRenderQueue()
{
    if (AddInterrupt)
        return m_x104 = ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&m_x90, "");
    return 0;
}


// ---- src/PSXTextureMapManager/GetSlotAddress.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off



// VRAM block address of each of the four texture transfer slots.
int PSXTextureMapManager::GetSlotAddress(int slot)
{
    switch (slot) {
    case 0:
        return 0x3800;
    case 1:
        return 0x3A00;
    case 2:
        return 0x3C00;
    case 3:
        return 0x3E00;
    default:
#line 2397 "PSXTextureMapManager.cpp"
        ASSERTF(0, ("GetSlotAddress: Illegal slot (%d)", slot));
        return -1;
    }
}
#pragma alignlabel reset


// ---- src/PSXTextureMapManager/GetClutAddress.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off



// VRAM block address of each of the four texture transfer slots.
int PSXTextureMapManager::GetClutAddress(int slot)
{
    switch (slot) {
    case 0:
        return 0x3900;
    case 1:
        return 0x3B00;
    case 2:
        return 0x3D00;
    case 3:
        return 0x3F00;
    default:
#line 2414 "PSXTextureMapManager.cpp"
        ASSERTF(0, ("GetClutAddress: Illegal slot (%d)", slot));
        return -1;
    }
}
#pragma alignlabel reset


// ---- src/PSXTextureMapManager/ResetFrame.cpp



// Transfers queued for each of the two frame buffers.




void PSXTextureMapManager::ResetFrame()
{
    n256ReqMax = n256Req;
    n128ReqMax = n128Req;
    n64ReqMax = n64Req;
    n32ReqMax = n32Req;
    n16ReqMax = n16Req;
    nTextureReqBytesMax = nTextureReqBytes;
    n256Req = 0;
    n128Req = 0;
    n64Req = 0;
    n32Req = 0;
    n16Req = 0;
    nTextureReqBytes = 0;
    nTexturesDecompressedMax = nTexturesDecompressed;
    nBytesDecompressedMax = nBytesDecompressed;
    nTexturesDecompressed = 0;
    nBytesDecompressed = 0;
    if (m_ipuCache) {
        n256 = m_ipuCache->GetOccupiedSpaceFromCache(0x100);
        n128 = m_ipuCache->GetOccupiedSpaceFromCache(0x80);
        n64 = m_ipuCache->GetOccupiedSpaceFromCache(0x40);
        n32 = m_ipuCache->GetOccupiedSpaceFromCache(0x20);
        n16 = m_ipuCache->GetOccupiedSpaceFromCache(0x10);
        n256free = m_ipuCache->GetAvailableSpaceFromCache(0x100);
        n128free = m_ipuCache->GetAvailableSpaceFromCache(0x80);
        n64free = m_ipuCache->GetAvailableSpaceFromCache(0x40);
        n32free = m_ipuCache->GetAvailableSpaceFromCache(0x20);
        n16free = m_ipuCache->GetAvailableSpaceFromCache(0x10);
    }
    m_currentSlot = 0;
    LastSlot = 0;
    TransferList.m_current = 0;
    TransferList.m_buffers[0].m_count = 0;
    TransferList.m_buffers[1].m_count = 0;
    CurrentIPUIndex = 0;
    MaxIPUIndex = 0;
    LastTextureMap = 0;
    for (int i = 0; i < 4; i++)
        m_slotLocked[i] = 0;
    m_x104 = 0;
    m_x108 = 0;
}


// ---- src/PSXTextureMapManager/ResetSendList.cpp


void PSXTextureMapManager::ResetSendList()
{
    TransferList.m_current = 0;
}


// ---- src/PSXTextureMapManager/ResetBuildList.cpp



void PSXTextureMapManager::ResetBuildList(int list)
{
    if (list == 1)
        TransferList.m_buffers[0].m_count = 0;
    else
        TransferList.m_buffers[1].m_count = 0;
    CurrentIPUIndex = 0;
    MaxIPUIndex = 0;
}


// ---- src/PSXTextureMapManager/BeginTransfer.cpp






void PSXTextureMapManager::BeginTransfer()
{
    static int BeginTransferID = -1;    // unused profiler event IDs
    static int IPU2ID = -1;
    int total;

    if (CurrentIPUIndex < MaxIPUIndex)
        BeginDecompression();
    static int ID = -1;
    if (((PSXRenderTarget *)LpRenderTarget)->m_buildQueue == 1)
        total = TransferList.m_buffers[0].m_count;
    else
        total = TransferList.m_buffers[1].m_count;
    if (TransferList.m_current < total)
        SendCurrentTransfer(1);
    static int EnableDebug = 0;
    if (total == -1000)     // (as in retail; a debug hook)
        EnableDebug = 0;
    if (EnableDebug) {
        for (int i = 0; i < m_ipuCache->m_lastHandle; i++) {
            if (m_ipuCache->m_handles[i].m_decompress)
                ViewImageData(m_ipuCache->m_handles[i].m_data, m_ipuCache->m_handles[i].m_x10);
        }
    }
}


// ---- src/PSXTextureMapManager/ViewImageData.cpp
extern "C" void *malloc(unsigned int size);
extern "C" void iFlushCache(int);




// Debug view: expands a size x size 16-bit (RGB555) image to 32-bit and
// sends it to the display buffer.
void PSXTextureMapManager::ViewImageData(unsigned char *data, int size)
{
    static unsigned int *lpBltSurface = (unsigned int *)malloc(0x40000);
    int y;
    int x;
    unsigned short *row;
    unsigned short pixel;
    for (y = 0; y < size; y++) {
        // guarded do/while: puts x = 0 in the guard's delay slot as retail does
        x = 0;
        if (x < size) {
            row = (unsigned short *)data;
            do {
                pixel = *row++;
                data += 2;
                lpBltSurface[y * size + x] = ((pixel << 9) & 0xF80000) | ((pixel << 6) & 0xF800) | ((pixel << 3) & 0xF8);
                x++;
            } while (x < size);
        }
    }
    iFlushCache(0);
    LpTextureMapManager->m_gsTransfer->PrepareTransferTextureToDisplayBuffer((unsigned char *)lpBltSurface, size, size, 0, 0, 0);
    LpTextureMapManager->m_gsTransfer->SendTexture(1);
}


// ---- src/PSXTextureMapManager/SendCurrentTransfer.cpp




// Transfers queued for each of the two frame buffers.





// Send the next queued texture (or fill) of this frame's transfer list.
int PSXTextureMapManager::SendCurrentTransfer(int wait)
{
    TransferSlotAssignment *items = (((PSXRenderTarget *)LpRenderTarget)->m_buildQueue == 1) ? TransferList.m_buffers[0].m_items : TransferList.m_buffers[1].m_items;
    int count = (((PSXRenderTarget *)LpRenderTarget)->m_buildQueue == 1) ? TransferList.m_buffers[0].m_count : TransferList.m_buffers[1].m_count;
    if (TransferList.m_current < count) {
        TransferSlotAssignment *item = &items[TransferList.m_current];
        PSXTextureMap *texture = item->m_texture;
        TransferList.m_current++;
        if (!texture) {
            m_x114[0] = item->m_fillColor;
            m_x114[1] = item->m_fillColor;
            m_x114[2] = item->m_fillColor;
            m_x114[3] = item->m_fillColor;
            m_x118->SendTexture(wait);
            return 1;
        }
        if (texture->m_xEC != item->m_address) {
            PSXMipSurface *surface = texture->GetReqdMipLevel();
            unsigned char *data;
            if (m_ipuCache && item->m_handle)
                data = item->m_handle->m_data;
            else
                data = (unsigned char *)surface->m_surface;
            texture->PrepareTransferTexture(m_gsTransfer, item->m_address, item->m_clutAddress, surface, data);
            texture->m_xEC = item->m_address;
            texture->m_xF0 = item->m_clutAddress;
        }
        static int sendTextures = 1;
        if (sendTextures)
            texture->m_transfer.SendTexture(wait);
        nTotalTransferedTextures++;
    }
    return 1;
}


// ---- src/PSXTextureMapManager/BeginDecompression.cpp

extern "C" void FlushCache(int operation);

#define D4_CHCR (*(volatile unsigned int *)0x1000B400)
#define D4_MADR (*(volatile unsigned int *)0x1000B410)

class TextureMap;
struct IpuCacheHandle;






void PSXTextureMapManager::BeginDecompression()
{
    // A dead if/else: retail keeps the compare and a branch to the next
    // instruction (both arms' stores are dropped after branch layout).
    int unused;
    if (((PSXRenderTarget *)LpRenderTarget)->m_buildQueue == 1)
        unused = 1;
    else
        unused = 2;
    if (MaxIPUIndex > 0) {
        D4_CHCR = 0;
        D4_MADR = 0;
    }
    while (CurrentIPUIndex < MaxIPUIndex) {
        IPUSlot slot = IPUSlotList[CurrentIPUIndex];
        PSXMipSurface *mip = slot.m_mip;
        if (!m_ipuCache) {
            PSXprintf("PSXTextureMapManager: No IpuCache has been created... Use 'AllocateIpuCache'\n");
#line 2674 "PSXTextureMapManager.cpp"
            ASSERT(0, "BeginDecompression: No IpuCache has been created... Use 'AllocateIpuCache");
        }
        static int ID2 = -1;            // unused profiler IDs
        TextureMap *decompressed = LpTextureMapManager->DecompressCurrent(1);
        FlushCache(0);
        static int ID = -1;
        if (decompressed) {
            LpTextureMapManager->m_ipuCache->PlaceDecompressedTextureIntoCache(IPUSlotList[CurrentIPUIndex].m_handle);
            FlushCache(0);
        }
        CurrentIPUIndex++;
        nTexturesDecompressed++;
        nBytesDecompressed += mip->m_compressedSize;
    }
}


// ---- src/PSXTextureMapManager/DecompressCurrent.cpp






TextureMap *PSXTextureMapManager::DecompressCurrent(int a)
{
    if (CurrentIPUIndex < MaxIPUIndex) {
        IPUSlot slot = IPUSlotList[CurrentIPUIndex];
        IPUDecompress((unsigned char *)m_ipuCache->m_cacheMem, (unsigned char *)slot.m_mip->m_data, slot.m_mip->m_compressedSize,
                      slot.m_mip->m_width, slot.m_mip->m_width,  // square images
                      slot.m_texture->m_bitsPerPixel, a);
        return slot.m_texture;
    }
    return 0;
}


// ---- src/PSXTextureMapManager/IPUDecompress.cpp
// Notes on the source shape:
// - The empty `asm(".set reorder")` after the qwc computation ends the basic block without
//   emitting code (any assembler directive does; an empty asm("") does not). Retail's entry
//   code is two scheduling blocks: prologue + qwc chain + parameter copies, then the
//   LpTextureMapManager load and the call. In one block, both schedulers hoist the global
//   load above sll/srl.
// - `t` holds height>>4 and is then reused for the IpuTextureTransfer pointer, so the copy
//   into $a0 for the call is coalesced and height>>4 lands in $a0. `bw` is reused for the
//   CHCR value after the calls, where $v0 is blocked, so width>>4 lands in $v1.

extern "C" int sceIpuSync(int mode, unsigned short timeout);

#define IPU_CMD  (*(volatile unsigned int *)0x10002000)
#define IPU_CTRL (*(volatile unsigned int *)0x10002010)
#define D3_CHCR  (*(volatile unsigned int *)0x1000B000)
#define D3_MADR  (*(volatile unsigned int *)0x1000B010)
#define D3_QWC   (*(volatile unsigned int *)0x1000B020)
#define D4_CHCR  (*(volatile unsigned int *)0x1000B400)



void PSXTextureMapManager::IPUDecompress(unsigned char *dst, unsigned char *src, int size, int width, int height,
                                         int unused, int wait)
{
    int bw = width >> 4;
    int t = height >> 4;
    unsigned int qwc = (unsigned int)(bw * t * 512) / 16;
    asm(".set reorder");
    t = (int)LpTextureMapManager->m_ipuTransfer;
    ((IpuTextureTransfer *)PS2_ADDR(t))->TransferTexture(src, size);
    sceIpuSync(0, 0);
    IPU_CMD = 0;                                // BCLR
    LpTextureMapManager->m_ipuTransfer->SendTexture(0);
    IPU_CMD = 0x40000000;                       // FDEC
    sceIpuSync(0, 0);
    long header = IPU_CMD;
    if (header & 0x8000000000000000)
        PSXprintf("IPU was busy and passed meaningless data in IPUDecompress\n");
    header = (header >> 24) & 0xFF;
    IPU_CMD = 0x40000008;                       // FDEC 8
    sceIpuSync(0, 0);
    IPU_CTRL = (header & ~4) << 16;
    int ofm = 1 << 27, dte = 1 << 26, sgn = 0 << 25, qsc = 1 << 16;
    int dtd = (int)(((header >> 2) & 1) << 24);
    int cmd = 1 << 28;
    IPU_CMD = qsc | (dtd | (sgn | (dte | (cmd | ofm))));   // IDEC
    D3_MADR = (unsigned int)dst & 0x0FFFFFFF;
    D3_QWC = qwc;
    bw = 0x100;
    D3_CHCR = bw;
    if (wait) {
        int status;
        int i;
#ifdef SPLASHDOWN_PC
        // the PC's IPU decodes on the DMA thread, slower than retail's
        // 0x10000 polls: wait for it
        for (i = 0;; i++) {
#else
        for (i = 0; i < 0x10000; i++) {
#endif
            status = D3_CHCR;
            if (!(status & 0x100))
                break;
        }
        if (status & 0x100) {
#line 2820 "PSXTextureMapManager.cpp"
            ASSERT(0, "IPU texture decompression did not complete!\n");
            IPU_CTRL = 0x40000000;
            D3_CHCR = 0;
            D4_CHCR = 0;
        }
    }
}

