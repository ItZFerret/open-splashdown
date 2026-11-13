// SearchList.cpp  @ 0x0030EE90 (text 0x30EE90-0x30F5F0)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's SearchList.cpp (its ASSERTs' file name) as one translation unit, a TU
// without a static initializer in BuoyObject.cpp's tu_map row, between
// JetskiThrillCam.cpp and VehicleAI.cpp: SearchList's 15 functions. Its .rodata
// (0x430CE0-0x430E20): "SearchList.cpp", the two ASSERT messages, then the
// SearchList/BaseObject/SearchMethod RTTI records. `make link` places the object's
// data as well as its code (tools/link_c.py).


#include "d3dtypes.h"
#include "SearchList/SearchList.h"
#include "SearchableObject/SearchableObject.h"
#include "soultree_assert.h"

extern "C" void *malloc(unsigned int size);
extern "C" void *realloc(void *p, unsigned int size);
extern "C" void free(void *p);

// A placement slot: an object pointer, or (bit 1 set) a free-list link.
struct SearchSlot {
    unsigned int link;
};

// ---- src/SearchList/ctor.cpp

SearchList::SearchList(int maxObjects)
{
    m_maxObjects = maxObjects;
    m_blocks = 0;
    m_numBlocks = 0;
    m_freeList = 0;
    m_lastBlockCount = m_maxObjects;
}

// ---- src/SearchList/dtor.cpp

SearchList::~SearchList()
{
    if (m_blocks) {
        for (int i = 0; i < m_numBlocks; i++) {
            if (m_blocks[i])
                free(m_blocks[i]);
        }
        free(m_blocks);
    }
}

// ---- src/SearchList/CalculatePointPlacementCode.cpp

unsigned int SearchList::CalculatePointPlacementCode(_D3DVECTOR *, unsigned int code)
{
    return NextPlacementCode(code);
}

// ---- src/SearchList/CalculatePointPlacementCode_0030EFD0.cpp

unsigned int SearchList::CalculatePointPlacementCode(float, float, float, unsigned int code)
{
    return NextPlacementCode(code);
}

// ---- src/SearchList/CalculateBoxPlacementCode.cpp

unsigned int SearchList::CalculateBoxPlacementCode(_D3DVECTOR *, _D3DVECTOR *, unsigned int code)
{
    return NextPlacementCode(code);
}

// ---- src/SearchList/NextPlacementCode.cpp
#pragma alignlabel off

// Hands out the next free placement-code slot, growing the block list as needed.
unsigned int SearchList::NextPlacementCode(unsigned int code)
{
    if (code)
        return code;
    if (m_freeList)
        return m_freeList;
    if (m_lastBlockCount == m_maxObjects) {
        SearchableObject ***blocks = (SearchableObject ***)realloc(m_blocks, (m_numBlocks + 1) * sizeof(SearchableObject **));
#line 85 "SearchList.cpp"
        ASSERT(blocks != 0, "NextPlacementCode: out of memory");
        m_blocks = blocks;
        m_blocks[m_numBlocks] = (SearchableObject **)malloc(m_maxObjects * sizeof(SearchableObject *));
#line 88 "SearchList.cpp"
        ASSERT(m_blocks[m_numBlocks] != 0, "NextPlacementCode: out of memory");
        m_numBlocks++;
        m_lastBlockCount = 0;
    }
    return (unsigned int)&m_blocks[m_numBlocks - 1][m_lastBlockCount];
}
#pragma alignlabel reset

// ---- src/SearchList/InsertObject.cpp
void SearchList::InsertObject(SearchableObject *obj, unsigned int flags, _D3DVECTOR *a, _D3DVECTOR *b)
{
    // flags is the slot NextPlacementCode handed out; a reused slot comes off
    // the free list (its link | 2), otherwise the last block grows.
    SearchSlot *slot = (SearchSlot *)PS2_ADDR(flags);
    if (m_freeList && flags == m_freeList)
        m_freeList = ((SearchSlot *)PS2_ADDR(m_freeList))->link & ~3;
    else
        m_lastBlockCount++;
    *(SearchableObject **)slot = obj;
}

// ---- src/SearchList/RemoveObject.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off

void SearchList::RemoveObject(SearchableObject *obj, unsigned int slot)
{
    SearchSlot *s = (SearchSlot *)PS2_ADDR(slot);
#line 130 "SearchList.cpp"
    ASSERT(s->link == (unsigned int)obj, "RemoveObject: the location in the list that code specifies does not contain lpObject! The caller is trying to remove from the wrong place!");
    s->link = m_freeList | 2;
    m_freeList = slot;
}
#pragma alignlabel reset

// ---- src/SearchList/OpenSearch.cpp

int SearchList::OpenSearch(_D3DVECTOR *, _D3DVECTOR *, _D3DVECTOR *, _D3DVECTOR *)
{
    m_searchIndex = 0;
    m_searchCount = 0;
    m_searchOpen = 1;
    return 1;
}

// ---- src/SearchList/OpenSearch_0030F2C0.cpp

int SearchList::OpenSearch(_D3DVECTOR *, _D3DVECTOR *, int *, SearchableObject ***, SearchableObject **)
{
    m_searchIndex = 0;
    m_searchCount = 0;
    m_searchOpen = 1;
    return 1;
}

// ---- src/SearchList/NextSearchObject.cpp

// Entries with bit 1 set are removed objects.
#define REMOVED(p) ((unsigned int)(p) & 2)

SearchableObject *SearchList::NextSearchObject()
{
    if (m_blocks) {
        SearchableObject **block = m_blocks[m_searchIndex];
        while (m_searchIndex < m_numBlocks - 1) {
            while (m_searchCount < m_maxObjects && REMOVED(block[m_searchCount]))
                m_searchCount++;
            if (m_searchCount < m_maxObjects)
                return block[m_searchCount++];
            m_searchIndex++;
            block = m_blocks[m_searchIndex];
            m_searchCount = 0;
        }
        if (m_searchIndex < m_numBlocks) {
            while (m_searchCount < m_lastBlockCount && REMOVED(block[m_searchCount]))
                m_searchCount++;
            if (m_searchCount < m_lastBlockCount)
                return block[m_searchCount++];
            m_searchIndex++;
        }
    }
    return 0;
}

// ---- src/SearchList/RewindSearch.cpp

void SearchList::RewindSearch()
{
    m_searchIndex = 0;
    m_searchCount = 0;
    m_searchOpen = 1;
}

// ---- src/SearchList/CloseSearch.cpp

void SearchList::CloseSearch()
{
    m_searchIndex = 0;
    m_searchCount = 0;
    m_searchOpen = 0;
}

// ---- src/SearchList/PerformPrepareGeometry.cpp

void SearchList::PerformPrepareGeometry()
{
    if (m_blocks) {
        int i;
        for (i = 0; i < m_numBlocks - 1; i++)
            PrepareGeometryOnSearchListContents(m_blocks[i], &m_maxObjects);
        PrepareGeometryOnSearchListContents(m_blocks[i], &m_lastBlockCount);
    }
}

// ---- src/SearchList/PerformRender3D.cpp

void SearchList::PerformRender3D()
{
    if (m_blocks) {
        int i;
        for (i = 0; i < m_numBlocks - 1; i++)
            Render3DOnSearchListContents(m_blocks[i], m_maxObjects);
        Render3DOnSearchListContents(m_blocks[i], m_lastBlockCount);
    }
}
