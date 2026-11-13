// SearchMethod.cpp  @ 0x002FBFD0 (text 0x2FBFD0-0x2FC730)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's SearchMethod.cpp as one translation unit: SearchMethod's methods
// and the ContainerList<GameObject *> members they use, a TU without an
// initializer in ParticleEmitter.cpp's tu_map row (its RTTI and the
// ContainerList.h strings follow a data-only TU's CD file list). The
// searchable views are SoultreeObject (SearchableObject first) and
// SearchableGameObject (GameObject first).



#include "ContainerList/ContainerList.h"
#include "TypeRegistry/TypeRegistry.h"
#include "SearchMethod/SearchMethod.h"
#include "GameObject/GameObject.h"
#include "GameObject/GameObjectGlobals.h"
#include "SearchableObject/SearchableObject.h"
#include "SearchableGameObject/SearchableGameObject.h"
#include "SoultreeObject/SoultreeObject.h"
#include "soultree_assert_block.h"
#define CONTAINERLIST_MEMBERS_ALL
#include "ContainerList/ContainerListMembers.h"

extern TypeRegistry *LpTypeRegistry;

// ---- src/SearchMethod/ctor.cpp

// Retail keeps `this` in a stack slot and reloads it for every use: the
// signature of a function containing inline asm (the global optimizer is
// off). The empty asm statement reproduces that.
SearchMethod::SearchMethod()
{
    asm("");
    m_type = 0;
    m_objects.Create(30, 30);
    m_soultreeObjectType = LpTypeRegistry->GetTypeID("SoultreeObject");
    m_searchableType = LpTypeRegistry->GetTypeID("SearchableGameObject");
}


// ---- src/SearchMethod/dtor.cpp

SearchMethod::~SearchMethod()
{
}


// ---- src/SearchMethod/PrepareGeometryOnSearchListContents.cpp

// Prepares geometry for every visible object the search found, collecting the
// ones that also want Render3D.
void SearchMethod::PrepareGeometryOnSearchListContents(SearchableObject **list, int *count)
{
    static int timerID = -1;    // unused profiler ID
    m_objects.Clear();
    for (int i = 0; i < *count; list++, i++) {
        unsigned int entry = (unsigned int)*list;
        if (entry & 2)
            continue;
        SearchableObject *object = (SearchableObject *)PS2_ADDR((entry & ~1));
        if (!object->m_searchVisible || object->m_hidden)
            continue;
        if (object->m_typeID == m_soultreeObjectType) {
            SoultreeObject *a = (SoultreeObject *)object;
            if (a->m_enabled) {
                a->PrepareGeometry();
                if (a->m_visible)
                    m_objects.AddItem(a);
            }
        } else if (object->m_typeID == m_searchableType) {
            SearchableGameObject *b = (SearchableGameObject *)object;
            if (b->m_enabled) {
                b->PrepareGeometry();
                if (object->m_addToList)
                    m_objects.AddItem(b);
            }
        }
    }
    LpRenderTarget->TrySendRenderQueue(0);
}


// ---- src/SearchMethod/Render3DOnSearchListContents.cpp

void SearchMethod::Render3DOnSearchListContents(SearchableObject **list, int count)
{
    for (int i = 0; i < m_objects.m_numItems; i++)
        m_objects.GetItem(i)->Render3D();
}


// ---- src/SearchMethod/UpdateExtents.cpp

void SearchMethod::UpdateExtents(SearchableObject *object, unsigned int flags, _D3DVECTOR *min, _D3DVECTOR *max)
{
}
