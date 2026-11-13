// CollisionObject.cpp  @ 0x00218FA0 (text 0x218FA0-0x226BA0)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's CollisionObject.cpp as one translation unit: CollisionObject's
// methods, the delta-transform helpers, SetCurrentCollisionData,
// TestSphereForCollision, the trivial rejection tests and
// TestRayBundleForCollision, then the static initializer. `make link` places
// the object's data as well as its code (tools/link_c.py). The rest of the
// tu_map row, from KEYDOWN_HANDLER_TEST (0x226BA0) on, is
// ControlInterface.cpp, a TU without an initializer. The VU0 files' own world
// (CollisionObject/vu0col.h: its own _D3DVECTOR, _D3DMATRIX, SoultreeObject,
// CollisionObject and collision shapes) is folded onto d3dtypes.h and the
// shared collision headers. vu0col's v * s operator multiplied v.x * s,
// d3dtypes' multiplies s * v.x: those call sites use ScaleVS below. The two
// C-mode UpdateCollisionInfo stubs are members again.



// inline_depth is TU-wide (the VU0 helpers nest; the per-function files had
// 4 and 5).
#pragma inline_depth(5)

#include "d3dtypes.h"
#include "TypeRegistry/TypeRegistry.h"
#include "GameObject/GameObject.h"
#include "SearchableObject/SearchableObject.h"
#include "CollisionObject/CollisionObject.h"
#include "CollisionTypeRegistry/CollisionTypeRegistry.h"
#include "SearchPlacement/SearchPlacement.h"
#include "SearchNode/SearchNode.h"
#include "global/Vu0Inline.h"
#include "global/Vu0More.h"
#include "DataAccess/DataAccess.h"
#include "SoultreeObject/SoultreeObject.h"
#include "SoultreeCollisionObject/SoultreeCollisionObject.h"
#include "SoultreeHierarchyCollisionObject/SoultreeHierarchyCollisionObject.h"
#include "SoultreeCollisionInstanceData/SoultreeCollisionInstanceData.h"
#include "SoultreeHierarchyCollisionInstanceData/SoultreeHierarchyCollisionInstanceData.h"
#include "BoundingBoxTree/BoundingBoxTree.h"
#include "RayCollisionObject/RayCollisionObject.h"
#include "VertexRayToFaceIntersectionData/VertexRayToFaceIntersectionData.h"
#include "global/CollisionTree.h"
#include "soultree_assert.h"
#include "EcoSystem/EcoSystem.h"
#include "Vegetation/Vegetation.h"
#include "RenderTarget/RenderTarget.h"
#include "sinit/AxisVectors.h"

// The static initializer's objects, after the axis vectors
// (__sinit_CollisionObject.cpp constructs them in this order): the world
// object type registry and the world object type IDs.
CollisionTypeRegistry CollisionObjectTypeRegistry;
CollisionTypeRegistry *LpCollisionTypeRegistry = &CollisionObjectTypeRegistry;

int WORLDOBJECT_SOFTFOLIAGE = LpCollisionTypeRegistry->GetType("SOFTFOLIAGE");
int WORLDOBJECT_SOLIDFOLIAGE = LpCollisionTypeRegistry->GetType("SOLIDFOLIAGE");
int WORLDOBJECT_PHYSICSBODY = LpCollisionTypeRegistry->GetType("PHYSICSBODY");
int WORLDOBJECT_STATICOBJECT = LpCollisionTypeRegistry->GetType("STATICOBJECT");
int WORLDOBJECT_TRIGGER = LpCollisionTypeRegistry->GetType("TRIGGER");

// m_collisionData of a COLLISION_BOX object.
struct BoxCollisionObject {
    _D3DVECTOR center;              // +0x00
    _D3DVECTOR half;                // +0x10
    _D3DMATRIX m_matrix;            // +0x20
};

// m_collisionData of a COLLISION_RADIUS_LINE_SEGMENT object.
struct RadiusLineSegmentCollisionObject {
    _D3DVECTOR start;               // +0x00
    _D3DVECTOR end;                 // +0x10
    float radius;                   // +0x20
    float radiusSquared;            // +0x24
    float m_radiusScale;            // +0x28
    float m_scale;                  // +0x2C
    _D3DMATRIX m_matrix;            // +0x30
};

// The spatial search iterator (0x40 bytes on the stack).
class SpatialSearch {
public:
    SpatialSearch();
    void OpenSearch(SearchPlacement *placement, unsigned int flags, _D3DVECTOR *center, _D3DVECTOR *half,
                    _D3DVECTOR *outMin, _D3DVECTOR *outMax);
    SearchableObject *NextSearchObject();

#ifdef SPLASHDOWN_PC
    _D3DVECTOR pad[6];  // x86-64: the real class (SpatialSearch.h) is 0x58 bytes, 16-aligned
#else
    char pad[0x40];
#endif
};

// A face-tree leaf's face (global/CollisionTree.h), stored from +0x04 of its node.
#define TREE_FACE(tree) ((BoundingBoxTreeFace *)&(tree)->m_radius)

void ModifyBoundingBox(_D3DVECTOR *min, _D3DVECTOR *max, _D3DMATRIX *matrix);
void ModifyBoundingBox(_D3DVECTOR *center, _D3DVECTOR *half, _D3DVECTOR, _D3DVECTOR, _D3DMATRIX *matrix);
int OBBOverlapTestWithDelta(_D3DVECTOR center, _D3DVECTOR half, _D3DVECTOR *center2, _D3DVECTOR *half2, _D3DMATRIX *matrix, _D3DMATRIX *delta);
int SphereVsSphereOverlapTest(_D3DVECTOR &centerA, float radiusA, _D3DMATRIX *matrixA, _D3DVECTOR &centerB, float radiusB, _D3DMATRIX *matrixB);
int TestBoundingBoxTreesForOverlap(BoundingBoxTree *a, BoundingBoxTree *b, _D3DMATRIX *matrixA, _D3DMATRIX *matrixB);
int OBBOverlapTest(_D3DVECTOR *center, _D3DVECTOR *half, _D3DVECTOR center2, _D3DVECTOR half2, _D3DMATRIX *matrix);
int BoxIntersectsBoundingBoxTree(_D3DVECTOR *center, _D3DVECTOR *half, _D3DMATRIX *matrix, BoundingBoxTree *tree, _D3DMATRIX *treeMatrix, int);
int BoxIntersectsBoundingBoxTree(_D3DVECTOR *center, _D3DVECTOR *half, _D3DMATRIX *matrix, BoundingBoxTree *tree,
                                 _D3DMATRIX *treeMatrix, int, _D3DVECTOR *vertices);
int OBBVsRadiusedLineSegmentOverlapTest(_D3DVECTOR *center, _D3DVECTOR *half, _D3DVECTOR *points, float radius, float radiusSquared, _D3DMATRIX *matrix);
int OBBVsSphereOverlapTest(_D3DVECTOR *center, _D3DVECTOR *half, _D3DVECTOR &point, float radius, float radiusSquared, _D3DMATRIX *matrix);
int SphereIntersectsBoundingBoxTree(_D3DVECTOR *center, float radius, float radiusSquared, BoundingBoxTree *tree, _D3DMATRIX *matrix, int, int);
int SphereIntersectsBoundingBoxTree(_D3DVECTOR *center, float radius, float radiusSquared, BoundingBoxTree *tree, _D3DMATRIX *matrix, int, _D3DVECTOR *vertices, int);
int RadiusedLineSegmentIntersectsBoundingBoxTree(_D3DVECTOR *point, float radius, float radiusSquared, BoundingBoxTree *tree, _D3DMATRIX *matrix, int, _D3DVECTOR *);
void CreateDeltaTransform(_D3DMATRIX *delta, _D3DMATRIX *matrix, _D3DMATRIX *previous);
_D3DVECTOR GetMatrixRow(_D3DMATRIX *matrix, int row);
void SetCurrentCollisionData(void *data);
void CreateDeltaTransformForStaticObject(_D3DMATRIX *delta, _D3DMATRIX *matrix, _D3DMATRIX *a, _D3DMATRIX *b);
int TestBoundingBoxTreesForIntersection(BoundingBoxTree *a, BoundingBoxTree *b, _D3DMATRIX *matrixA, _D3DMATRIX *matrixB, BoundingBoxTreeType type, _D3DVECTOR *vertices, _D3DMATRIX *delta);
float TrivialCollisionObjectRejectionTest(CollisionObject *a, CollisionObject *b);
float Trivial2DCollisionObjectRejectionTest(CollisionObject *a, _D3DVECTOR *center, float radius);

// -v (vu0col.h's member operator).
inline _D3DVECTOR operator-(const _D3DVECTOR &v) { return _D3DVECTOR(-v.x, -v.y, -v.z); }

// v * s as vu0col.h's operator multiplied it (v.x * s, not s * v.x).
static inline _D3DVECTOR ScaleVS(const _D3DVECTOR &v, float s) { return _D3DVECTOR(v.x * s, v.y * s, v.z * s); }

// out = m * in (point: w = 1), libvu0 sceVu0ApplyMatrix(out, m, in) plus the
// translation row. Operand order (out, m, in) matters for register choice.
static inline void TransformPoint(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR *in)
{
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x10(%1); lqc2 vf6, 0x20(%1); lqc2 vf7, 0x30(%1); lqc2 vf8, 0x0(%2); vmulw.w vf9, vf0, vf0w; vmulax.xyz ACC, vf4, vf8x; vmadday.xyz ACC, vf5, vf8y; vmaddz.xyz vf9, vf6, vf8z; vadd.xyz vf9, vf9, vf7; sqc2 vf9, 0x0(%0)"
        : : "r"(out), "r"(m), "r"(in) : "memory");
}

// out = b * a (row vectors): libvu0 sceVu0MulMatrix(out, a, b). mwcc hands
// out operand registers from the last operand down, hence (out, b, a).
static inline void MulMatrix(_D3DMATRIX *out, _D3DMATRIX *a, _D3DMATRIX *b)
{
    asm("lqc2 vf5, 0x0(%1); lqc2 vf1, 0x0(%2); lqc2 vf2, 0x10(%2); lqc2 vf3, 0x20(%2); lqc2 vf4, 0x30(%2); lqc2 vf6, 0x10(%1); vmulax.xyzw ACC, vf1, vf5x; vmadday.xyzw ACC, vf2, vf5y; vmaddaz.xyzw ACC, vf3, vf5z; vmaddw.xyzw vf9, vf4, vf5w; lqc2 vf7, 0x20(%1); vmulax.xyzw ACC, vf1, vf6x; vmadday.xyzw ACC, vf2, vf6y; vmaddaz.xyzw ACC, vf3, vf6z; vmaddw.xyzw vf10, vf4, vf6w; lqc2 vf8, 0x30(%1); vmulax.xyzw ACC, vf1, vf7x; vmadday.xyzw ACC, vf2, vf7y; vmaddaz.xyzw ACC, vf3, vf7z; vmaddw.xyzw vf11, vf4, vf7w; vmulax.xyzw ACC, vf1, vf8x; vmadday.xyzw ACC, vf2, vf8y; vmaddaz.xyzw ACC, vf3, vf8z; vmaddw.xyzw vf12, vf4, vf8w; sqc2 vf9, 0x0(%0); sqc2 vf10, 0x10(%0); sqc2 vf11, 0x20(%0); sqc2 vf12, 0x30(%0)"
        : : "r"(out), "r"(b), "r"(a) : "memory");
}

// out = m's 3x3 part * v (w ends up 1), v passed by value.
static inline void ApplyMatrixVector(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR v)
{
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x10(%1); lqc2 vf6, 0x20(%1); lqc2 vf8, 0x0(%2); vmulax.xyz ACC, vf4, vf8x; vmadday.xyz ACC, vf5, vf8y; vmaddz.xyz vf9, vf6, vf8z; vadd.xyzw vf9, vf9, vf0; sqc2 vf9, 0x0(%0)"
        : : "r"(out), "r"(m), "r"(&v) : "memory");
}

// libvu0 sceVu0Normalize.
static inline void Normalize(_D3DVECTOR *out, _D3DVECTOR *in)
{
    asm("lqc2 vf4, 0x0(%1); vmul.xyz vf5, vf4, vf4; vaddy.x vf5, vf5, vf5y; vaddz.x vf5, vf5, vf5z; vrsqrt Q, vf0w, vf5x; vsub.xyzw vf6, vf0, vf0; vaddw.xyzw vf6, vf6, vf4w; vwaitq; vmulq.xyz vf6, vf4, Q; sqc2 vf6, 0x0(%0)"
        : : "r"(out), "r"(in) : "memory");
}

// The same multiply with the matrices passed by value (retail copies them to
// the stack first: an aligned local with lq/sq, anything else word by word).
static inline void MulMatrixCopy(_D3DMATRIX *out, _D3DMATRIX a, _D3DMATRIX b)
{
    asm("lqc2 vf5, 0x0(%1); lqc2 vf1, 0x0(%2); lqc2 vf2, 0x10(%2); lqc2 vf3, 0x20(%2); lqc2 vf4, 0x30(%2); lqc2 vf6, 0x10(%1); vmulax.xyzw ACC, vf1, vf5x; vmadday.xyzw ACC, vf2, vf5y; vmaddaz.xyzw ACC, vf3, vf5z; vmaddw.xyzw vf9, vf4, vf5w; lqc2 vf7, 0x20(%1); vmulax.xyzw ACC, vf1, vf6x; vmadday.xyzw ACC, vf2, vf6y; vmaddaz.xyzw ACC, vf3, vf6z; vmaddw.xyzw vf10, vf4, vf6w; lqc2 vf8, 0x30(%1); vmulax.xyzw ACC, vf1, vf7x; vmadday.xyzw ACC, vf2, vf7y; vmaddaz.xyzw ACC, vf3, vf7z; vmaddw.xyzw vf11, vf4, vf7w; vmulax.xyzw ACC, vf1, vf8x; vmadday.xyzw ACC, vf2, vf8y; vmaddaz.xyzw ACC, vf3, vf8z; vmaddw.xyzw vf12, vf4, vf8w; sqc2 vf9, 0x0(%0); sqc2 vf10, 0x10(%0); sqc2 vf11, 0x20(%0); sqc2 vf12, 0x30(%0)"
        : : "r"(out), "r"(&b), "r"(&a) : "memory");
}

// sceVu0InversMatrix for a rigid transform (transpose the rotation, negate
// the rotated translation), in place.
static inline void InvertMatrix(_D3DMATRIX *m)
{
    asm("lqc2 vf1, 0x0(%0); lqc2 vf2, 0x10(%0); lqc2 vf3, 0x20(%0); vsub.xyzw vf30, vf30, vf30; vadd.xyzw vf4, vf1, vf30; vadd.xyzw vf5, vf2, vf30; vadd.xyzw vf6, vf3, vf30; vaddy.x vf5, vf0, vf1y; vaddz.x vf6, vf0, vf1z; vaddz.y vf6, vf0, vf2z; vaddx.y vf4, vf0, vf2x; vaddx.z vf4, vf0, vf3x; vaddy.z vf5, vf0, vf3y; lqc2 vf10, 0x30(%0); vsub.xyzw vf11, vf11, vf11; vadd.xyzw vf11, vf10, vf11; vmulax.xyz ACC, vf4, vf10x; vmadday.xyz ACC, vf5, vf10y; vmaddz.xyz vf11, vf6, vf10z; vsub.xyzw vf12, vf12, vf12; vsub.w vf12, vf12, vf0; vmulw.xyz vf11, vf11, vf12w; sqc2 vf4, 0x0(%0); sqc2 vf5, 0x10(%0); sqc2 vf6, 0x20(%0); sqc2 vf11, 0x30(%0)"
        : : "r"(m) : "memory");
}

// out = transpose(m's 3x3 part) * v: rotate by the inverse of a rigid m. v by
// value; w ends up 1.
static inline void InverseRotate(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR v)
{
    asm("lqc2 vf1, 0x0(%1); lqc2 vf2, 0x10(%1); lqc2 vf3, 0x20(%1); lqc2 vf4, 0x30(%1); vmulw.w vf21, vf0, vf0w; vsub.xyzw vf30, vf30, vf30; vadd.xyzw vf5, vf1, vf30; vadd.xyzw vf6, vf2, vf30; vadd.xyzw vf7, vf3, vf30; vadd.xyzw vf8, vf4, vf30; vaddy.x vf6, vf0, vf1y; vaddz.x vf7, vf0, vf1z; vaddz.y vf7, vf0, vf2z; vaddx.y vf5, vf0, vf2x; vaddx.z vf5, vf0, vf3x; vaddy.z vf6, vf0, vf3y; lqc2 vf20, 0x0(%2); vmulax.xyz ACC, vf5, vf20x; vmadday.xyz ACC, vf6, vf20y; vmaddz.xyz vf21, vf7, vf20z; sqc2 vf21, 0x0(%0)"
        : : "r"(out), "r"(m), "r"(&v) : "memory");
}


// CollisionTypeRegistry and the Clear*CollisionObject helpers open this TU in
// retail's text (0x218CB0-0x218FA0): they follow CollisionCharacter.cpp's
// end-of-TU @640@48@ thunks, which mwcc writes only at a TU's end. Moved here
// from CollisionCharacter.cpp.
extern "C" char *strcpy(char *dst, const char *src);
void DeleteBoundingBoxTree(BoundingBoxTree *tree);

// ---- src/CollisionTypeRegistry/ctor.cpp

CollisionTypeRegistry::CollisionTypeRegistry()
{
    m_count = 0;
}


// ---- src/CollisionTypeRegistry/GetType.cpp

int CollisionTypeRegistry::GetType(char *name)
{
    int i;
    char *p = m_names[0];
    for (i = 0; i < m_count; i++) {
        if (strcasecmp(name, p) == 0)
            return 1 << i;
        p += 0x20;
    }
    // The new type's bit is built in i's register ahead of the strcpy.
    i = 1;
    i <<= m_count;
    strcpy(m_names[m_count], name);
    m_count++;
    return i;
}


// ---- src/global/ClearSoultreeCollisionObject.cpp

void ClearSoultreeCollisionObject(SoultreeCollisionObject *object)
{
    if (object->m_instance) {
        object->m_instance->m_refCount--;
        if (object->m_instance->m_refCount <= 0) {
            if (object->m_faceTree)
                DeleteBoundingBoxTree(object->m_faceTree);
            if (object->m_vertices)
                free(object->m_vertices);
            object->m_faceTree = 0;
            object->m_vertices = 0;
            free(object->m_instance);
            object->m_instance = 0;
        }
    } else {
        if (object->m_faceTree)
            DeleteBoundingBoxTree(object->m_faceTree);
        if (object->m_vertices)
            free(object->m_vertices);
        object->m_faceTree = 0;
        object->m_vertices = 0;
    }
}


// ---- src/global/ClearSoultreeHierarchyCollisionObject.cpp

void ClearSoultreeHierarchyCollisionObject(SoultreeHierarchyCollisionObject *object)
{
    int i;

    if (object->m_instanceData) {
        object->m_instanceData->m_refCount--;
        if (object->m_instanceData->m_refCount <= 0) {
            for (i = 0; i < object->m_count; i++)
                ClearSoultreeCollisionObject(&object->m_objects[i]);
            free(object->m_instanceData->m_x0C);
            free(object->m_instanceData->m_x08);
            free(object->m_instanceData->m_x14);
            free(object->m_instanceData->m_matrices);
            free(object->m_instanceData->m_objectIndices);
            free(object->m_instanceData);
            object->m_instanceData = 0;
        }
        delete[] object->m_objects;
        delete[] object->m_x04;
        delete[] object->m_x08;
    } else {
        for (i = 0; i < object->m_count; i++)
            ClearSoultreeCollisionObject(&object->m_objects[i]);
        delete[] object->m_objects;
        delete[] object->m_x04;
        delete[] object->m_x08;
    }
}


// ---- src/global/ClearRayCollisionObject.cpp

void ClearRayCollisionObject(RayCollisionObject *object)
{
    if (object->m_tree)
        DeleteBoundingBoxTree(object->m_tree);
}


// ---- src/CollisionObject/ctor.cpp
extern TypeRegistry *LpTypeRegistry;

// The first .sdata of the program (0x44E980): this is the first TU in link
// order with .sdata; VehicleAI.cpp uses them too.
unsigned char CollisionTypeID = 0xFF;   // 0xFF until registered
unsigned char VegetationTypeID = 0xFF;


CollisionObject::CollisionObject(int enabled) : GameObject(enabled)
{
    m_fileHandle = -1;
    if (CollisionTypeID == 0xFF)
        CollisionTypeID = LpTypeRegistry->GetTypeID("CollisionObject");
    if (VegetationTypeID == 0xFF)
        VegetationTypeID = LpTypeRegistry->GetTypeID("Vegetation");
    m_typeID = LpTypeRegistry->GetTypeID("CollisionObject");
    m_collisionType = 0;
    m_collisionData = 0;
    m_intersectionData = 0;
    m_collided = 0;
    m_callback = 0;
    m_otherCallback = 0;
    m_debugIndex = 0;
    m_debugMode = 0;
    m_ignoreVegetation = 0;
    m_searchable = 1;
    m_collidable = 1;
    m_testMask = -1;
    m_testListType = 1;
    m_testList2Type = 1;
    m_objectTypeListType = 1;
    m_testList = 0;
    m_testList2 = 0;
    m_numTestObjects = 0;
    m_numTestObjects2 = 0;
    m_objectTypes = 0;
    m_numObjectTypes = 0;
    m_owner = 0;
    m_ownerData = 0;
    m_ownerType = 0;
    m_x80 = 0;
    m_tested = 0;
    m_testObject = 0;
    m_collisionPosition = ZeroVector;
    m_collisionNormal = ZeroVector;
    m_x64 = 0;
    m_half.x = 0;
    m_half.y = -10000.0f;
    m_half.z = 0;
    m_half.w = 1.0f;
    m_searchPlacement = 0;
}


// ---- src/CollisionObject/dtor.cpp

CollisionObject::~CollisionObject()
{
    if (m_searchPlacement)
        delete m_searchPlacement;
    if (m_testList)
        free(m_testList);
    if (m_testList2)
        free(m_testList2);
    if (m_objectTypes)
        free(m_objectTypes);
    ClearCollisionData();
}


// ---- src/CollisionObject/Create.cpp

// CollisionObject's first base: 8 bytes in front of its GameObject subobject.


GameObject *CollisionObject::Create(unsigned int flags)
{
    m_tempPlacement = (flags & 8) ? 1 : 0;
    m_searchable = (flags & 2) ? 1 : 0;
    m_collidable = (flags & 4) ? 1 : 0;
    return GameObject::Create();
}


// ---- src/global/CreateDeltaTransform.cpp


// delta = inverse(from) * to: the transform that takes `from` to `to`.
void CreateDeltaTransform(_D3DMATRIX *delta, _D3DMATRIX *from, _D3DMATRIX *to)
{
    _D3DMATRIX inv __attribute__((aligned(16)));
    inv = *from;
    Vu0InversMatrix(&inv);
    Vu0MulMatrix(delta, &inv, to);
}


// ---- src/global/CreateDeltaTransformForStaticObject.cpp


void CreateDeltaTransform(_D3DMATRIX *delta, _D3DMATRIX *from, _D3DMATRIX *to);

// As CreateDeltaTransform, for an object placed at `local` inside a parent
// moving from `from` to `to`.
void CreateDeltaTransformForStaticObject(_D3DMATRIX *delta, _D3DMATRIX *local, _D3DMATRIX *from, _D3DMATRIX *to)
{
    _D3DMATRIX inv __attribute__((aligned(16)));
    _D3DMATRIX localFrom __attribute__((aligned(16)));
    _D3DMATRIX localTo __attribute__((aligned(16)));
    inv = *from;
    Vu0InversMatrix(&inv);
    Vu0MulMatrix(&localFrom, &inv, local);
    inv = *to;
    Vu0InversMatrix(&inv);
    Vu0MulMatrix(&localTo, &inv, local);
    CreateDeltaTransform(delta, &localFrom, &localTo);
}


// ---- src/CollisionObject/ClearCollisionData.cpp

extern DataAccess *LpDataAccess;

void ClearSoultreeCollisionObject(SoultreeCollisionObject *object);
void ClearSoultreeHierarchyCollisionObject(SoultreeHierarchyCollisionObject *object);
void ClearRayCollisionObject(RayCollisionObject *object);


inline void CollisionObject::ClearIntersectionData()
{
    RayToFaceIntersectionData *data = (RayToFaceIntersectionData *)m_intersectionData;
    if (data) {
        if (data->m_rayT) {
            delete[] data->m_rayT;
            data->m_t = 0;  // retail clears the wrong field
        }
        if (data->m_rays) {
            delete[] data->m_rays;
            data->m_rays = 0;
        }
        if (data->m_rayNormals) {
            delete[] data->m_rayNormals;
            data->m_rayNormals = 0;
        }
        if (data->m_rayHit) {
            delete[] data->m_rayHit;
            data->m_rayHit = 0;
        }
        if (data->m_rayTransformed) {
            delete[] data->m_rayTransformed;
            data->m_rayTransformed = 0;
        }
        if (data->m_rayPoints) {
            delete[] data->m_rayPoints;
            data->m_rayPoints = 0;
        }
    }
}

void CollisionObject::ClearCollisionData()
{
    if (m_collisionData) {
        switch (m_collisionType) {
        case COLLISION_SOULTREE: {
            SoultreeCollisionObject *data = (SoultreeCollisionObject *)m_collisionData;
            LpDataAccess->DetachObject(data->m_instance);
            ClearSoultreeCollisionObject(data);
            break;
        }
        case COLLISION_SOULTREE_HIERARCHY: {
            SoultreeHierarchyCollisionObject *data = (SoultreeHierarchyCollisionObject *)m_collisionData;
            LpDataAccess->DetachObject(data->m_instanceData);
            ClearSoultreeHierarchyCollisionObject(data);
            break;
        }
        case COLLISION_DYNAMIC_RAYS: {
            DynamicRayCollisionObject *data = (DynamicRayCollisionObject *)m_collisionData;
            delete[] data->m_objects;
            delete[] data->m_localDirections;
            delete[] data->m_lengths;
            delete[] data->m_directions;
            delete[] data->m_starts;
            delete[] data->m_localStarts;
            ClearIntersectionData();
            break;
        }
        case COLLISION_RAYS:
            ClearRayCollisionObject((RayCollisionObject *)m_collisionData);
            ClearIntersectionData();
            break;
        case COLLISION_RADIUS_LINE_SEGMENT:
        case COLLISION_SPHERE:
        case COLLISION_BOX:
            break;
        }
        operator delete(m_collisionData);
    }
    if (m_intersectionData)
        delete m_intersectionData;
    m_collisionData = 0;
    m_intersectionData = 0;
}


// ---- src/CollisionObject/SetSoultreeCollision.cpp

extern "C" char *strcpy(char *dst, const char *src);
extern "C" char *strcat(char *dst, const char *src);
extern "C" char *strncat(char *dst, const char *src, unsigned int n);
extern "C" unsigned int strlen(const char *s);
void PSXprintf(const char *fmt, ...);

extern char CollisionContentDirectory[];


// The last '.' in the name, or 0.
static inline char *FindExtension(char *s)
{
    char *start = s;
    while (*s++)
        ;
    do {
        s--;
    } while (s != start && *s != '.');
    if (*s == '.')
        return s;
    return 0;
}

// Loads "<name>.col" from the collision content directory: either already in
// memory (instanced) or read from the file.
int CollisionObject::SetSoultreeCollision(SoultreeObject *object, char *name)
{
    char path[0x110];
    long handle;
    void *data;

    if (CollisionContentDirectory[0]) {
        strcpy(path, CollisionContentDirectory);
        if (CollisionContentDirectory[strlen(CollisionContentDirectory) - 1] != '\\')
            strncat(path, "\\", 0x7F);
        strcat(path, name);
    } else {
        strcpy(path, name);
    }
    char *extension = FindExtension(path);
    if (extension)
        strcpy(extension, ".col");
    else
        strcat(path, (char *)".col");
    handle = -1;
    data = 0;
    // The redundant casts reproduce retail's argument load order.
    if (!LpDataAccess->GetDataOrFileHandle(path, "rb", &handle, (void **)&data, 1)) {
        PSXprintf("ERROR !: Unable to load %s\n", name);
        return 0;
    }
    if (data) {
        InstanceCollisionData(object, data);
        return 1;
    }
    m_fileHandle = handle;
    ReadCollisionData(object, handle);
    LpDataAccess->FClose(handle);
    return 1;
}


// ---- src/CollisionObject/SetSoultreeCollisionRays.cpp
// SetSoultreeCollisionRays__15CollisionObjectFP14SoultreeObjectPcP10_D3DVECTOR  @ 0x00219D80
// Retail reads data->m_numVertices again for the new[] size (not the
// local): that is what gives origin $s2 and numVertices $s3.


// Turns a soultree collision mesh into rays from a center (the given origin or
// the middle of the vertices' bounding box) to every vertex.
void CollisionObject::SetSoultreeCollisionRays(SoultreeObject *object, char *name, _D3DVECTOR *origin)
{
    SoultreeCollisionObject *data;
    SoultreeObject *owner;
    int numVertices;
    float cz;
    float cy;
    float cx;
    float cw;
    SetSoultreeCollision(object, name);
    data = (SoultreeCollisionObject *)m_collisionData;
    numVertices = data->m_numVertices;
    owner = data->m_object;
    if (!origin) {
        _D3DVECTOR *v = data->m_vertices;
        float maxZ;
        float maxX;
        float maxY;
        float minX;
        float minZ;
        float minY;
        maxX = minX = v[0].x;
        maxY = minY = v[0].y;
        maxZ = minZ = v[0].z;
        for (int i = 1; i < numVertices; i++) {
            if (v[i].x > maxX)
                maxX = v[i].x;
            else if (v[i].x < minX)
                minX = v[i].x;
            if (v[i].y > maxY)
                maxY = v[i].y;
            else if (v[i].y < minY)
                minY = v[i].y;
            if (v[i].z > maxZ)
                maxZ = v[i].z;
            else if (v[i].z < minZ)
                minZ = v[i].z;
        }
        cz = 0.5f * (minZ + maxZ);
        cx = 0.5f * (minX + maxX);
        cy = 0.5f * (minY + maxY);
        cw = 1.0f;
    } else {
        cx = origin->x;
        cy = origin->y;
        cz = origin->z;
        cw = origin->w;
    }
    _D3DVECTOR *rays = new _D3DVECTOR[data->m_numVertices * 2];
    for (int i = 0; i < data->m_numVertices; i++) {
        rays[i * 2].x = cx;
        rays[i * 2].y = cy;
        rays[i * 2].z = cz;
        rays[i * 2].w = cw;
        rays[i * 2 + 1] = data->m_vertices[i];
    }
    SetRayCollision(numVertices, rays, owner);
    delete[] rays;
}


// ---- src/CollisionObject/SetGeometryCollision.cpp
extern "C" void *malloc(unsigned int size);


_D3DMATRIX IdentityMatrix();


void BuildFaceTree(BoundingBoxTree *tree, _D3DMATRIX *matrix, _D3DVECTOR *vertices, unsigned int *faces,
                   int numFaces, int numVertices, int depth);


void CollisionObject::SetGeometryCollision(_D3DVECTOR *vertices, unsigned int *faces, int numFaces, int numVertices)
{
    ClearCollisionData();
    SoultreeCollisionObject *object = new SoultreeCollisionObject;
    object->m_tree = 0;
    object->m_transform = IdentityMatrix();
    object->m_x00 = 0;
    object->m_instance = 0;
    m_collisionType = COLLISION_SOULTREE;
    m_collisionData = object;
    object->m_object = 0;
    object->m_vertices = (_D3DVECTOR *)malloc(numVertices * sizeof(_D3DVECTOR));
    for (int i = 0; i < numVertices; i++) {
        object->m_vertices[i] = *vertices;
        vertices++;
    }
    object->m_faceTree = new BoundingBoxTree;
    BuildFaceTree(object->m_faceTree, &object->m_x90, object->m_vertices, faces, numFaces, numVertices, 0);
}


// ---- src/CollisionObject/SetSphereCollision.cpp
extern "C" void *memset(void *dst, int c, unsigned int n);


void CollisionObject::SetSphereCollision(_D3DVECTOR center, float radius, SoultreeObject *object)
{
    ClearCollisionData();
    SphereCollisionObject *sphere = new SphereCollisionObject;
    m_collisionType = COLLISION_SPHERE;
    m_collisionData = sphere;
    SphereIntersectionData *data = new SphereIntersectionData;
    data->m_maxHits = 3;
    data->m_count = 0;
    m_intersectionData = (VertexRayToFaceIntersectionData *)data;
    sphere->m_object = object;
    sphere->center = center;
    sphere->radius = radius;
    sphere->radiusSquared = radius * radius;
    sphere->m_radiusScale = 1.0f;
    sphere->m_scale = 1.0f;
    memset(&sphere->m_matrix, 0, sizeof(sphere->m_matrix));
    sphere->m_matrix._11 = sphere->m_matrix._22 = sphere->m_matrix._33 = sphere->m_matrix._44 = 1.0f;
}


// ---- src/CollisionObject/UpdateSphereCollision.cpp

void CollisionObject::UpdateSphereCollision(_D3DVECTOR *center, float *radius)
{
    SphereCollisionObject *sphere = (SphereCollisionObject *)m_collisionData;
    if (center)
        sphere->center = *center;
    if (radius) {
        float r = *radius;
        sphere->radius = r;
        sphere->radiusSquared = r * r;
    }
}


// ---- src/CollisionObject/SetBoxCollision.cpp


void CollisionObject::SetBoxCollision(_D3DVECTOR min, _D3DVECTOR max)
{
    ClearCollisionData();
    BoxCollisionObject *box = new BoxCollisionObject;
    m_collisionType = COLLISION_BOX;
    m_collisionData = box;
    box->center = min;
    box->half = max;
    memset(&box->m_matrix, 0, sizeof(box->m_matrix));
    box->m_matrix._11 = box->m_matrix._22 = box->m_matrix._33 = box->m_matrix._44 = 1.0f;
}


// ---- src/CollisionObject/SetRadiusLineSegmentCollision.cpp


void CollisionObject::SetRadiusLineSegmentCollision(_D3DVECTOR start, _D3DVECTOR end, float radius)
{
    ClearCollisionData();
    RadiusLineSegmentCollisionObject *segment = new RadiusLineSegmentCollisionObject;
    m_collisionType = COLLISION_RADIUS_LINE_SEGMENT;
    m_collisionData = segment;
    segment->start = start;
    segment->end = end;
    segment->radius = radius;
    segment->radiusSquared = radius * radius;
    segment->m_radiusScale = 1.0f;
    segment->m_scale = 1.0f;
    memset(&segment->m_matrix, 0, sizeof(segment->m_matrix));
    segment->m_matrix._11 = segment->m_matrix._22 = segment->m_matrix._33 = segment->m_matrix._44 = 1.0f;
}


// ---- src/CollisionObject/SetRayCollision.cpp
void BuildRayTree(BoundingBoxTree *tree, _D3DMATRIX *matrix, _D3DVECTOR *rays, int numRays);


void CollisionObject::SetRayCollision(int numRays, _D3DVECTOR *rays, SoultreeObject *object)
{
    ClearCollisionData();
    RayCollisionObject *data = new RayCollisionObject;
    m_collisionType = COLLISION_RAYS;
    m_collisionData = data;
    data->m_object = object;
    data->m_tree = new BoundingBoxTree;
    memset(&data->m_matrix, 0, sizeof(data->m_matrix));
    data->m_matrix._11 = data->m_matrix._22 = data->m_matrix._33 = data->m_matrix._44 = 1.0f;
    BuildRayTree(data->m_tree, &data->m_matrix, rays, numRays);
    RayToFaceIntersectionData *isect = new RayToFaceIntersectionData;
    isect->m_maxRays = numRays * 3;
    isect->m_rayTransformed = new int[isect->m_maxRays];
    isect->m_rayHit = new int[isect->m_maxRays];
    isect->m_rayT = new float[isect->m_maxRays];
    isect->m_rays = new RaySegment *[isect->m_maxRays];
    isect->m_rayNormals = new _D3DVECTOR[isect->m_maxRays];
    isect->m_rayPoints = new _D3DVECTOR[isect->m_maxRays];
    m_intersectionData = (VertexRayToFaceIntersectionData *)isect;
}


// ---- src/CollisionObject/SetDynamicRayCollision.cpp


void CollisionObject::SetDynamicRayCollision(int numRays, _D3DVECTOR *rays, SoultreeObject **objects)
{
    ClearCollisionData();
    DynamicRayCollisionObject *data = new DynamicRayCollisionObject;
    m_collisionType = COLLISION_DYNAMIC_RAYS;
    m_collisionData = data;
    data->m_numRays = numRays;
    data->m_objects = new SoultreeObject *[numRays * 2];
    data->m_localDirections = new _D3DVECTOR[numRays * 2];
    data->m_lengths = new float[numRays];
    data->m_directions = new _D3DVECTOR[numRays];
    data->m_starts = new _D3DVECTOR[numRays];
    data->m_localStarts = new _D3DVECTOR[numRays];
    for (int i = 0; i < numRays * 2; i++) {
        if (objects)
            data->m_objects[i] = objects[i];
        else
            data->m_objects[i] = 0;
        if (rays)
            data->m_localDirections[i] = rays[i];
        else
            data->m_localDirections[i] = ZeroVector;
    }
    RayToFaceIntersectionData *isect = new RayToFaceIntersectionData;
    isect->m_maxRays = numRays * 3;
    isect->m_rayTransformed = new int[isect->m_maxRays];
    isect->m_rayHit = new int[isect->m_maxRays];
    isect->m_rayT = new float[isect->m_maxRays];
    isect->m_rays = new RaySegment *[isect->m_maxRays];
    isect->m_rayNormals = new _D3DVECTOR[isect->m_maxRays];
    isect->m_rayPoints = new _D3DVECTOR[isect->m_maxRays];
    m_intersectionData = (VertexRayToFaceIntersectionData *)isect;
}


// ---- src/CollisionObject/SetDynamicRays.cpp


void CollisionObject::SetDynamicRays(_D3DVECTOR *rays, SoultreeObject **objects)
{
    int i;
    // Retail tests the stored type (always true): the assignment's value
    // stays in a register and the branch on it isn't folded away.
    if ((m_collisionType = COLLISION_DYNAMIC_RAYS) != 0) {
        if (m_collisionData) {
            DynamicRayCollisionObject *data = (DynamicRayCollisionObject *)m_collisionData;
            for (i = 0; i < data->m_numRays * 2; i++) {
                if (objects)
                    data->m_objects[i] = objects[i];
                else
                    data->m_objects[i] = 0;
                data->m_localDirections[i] = rays[i];
            }
        }
    }
}


// ---- src/CollisionObject/RenderSoultreeHierarchyCollisionObject.cpp


void CollisionObject::RenderSoultreeHierarchyCollisionObject(SoultreeHierarchyCollisionObject *object, int depth, int mode)
{
    for (int i = 0; i < object->m_count; i++) {
        // Only modes 4 and 7 draw anything; retail still tests the other
        // modes one by one (their bodies are empty).
        if (mode == 1) {
        } else if (mode == 2) {
        } else if (mode == 3) {
        } else if (mode == 4) {
            RenderFaceTree(object->m_objects[i].m_faceTree, depth, &object->m_objects[i].m_matrix,
                           &object->m_objects[i].m_x10, object->m_objects[i].m_vertices);
        } else if (mode == 5) {
        } else if (mode == 6) {
        } else if (mode == 7) {
            RenderFaceTreeFaces(object->m_objects[i].m_faceTree, object->m_objects[i].m_vertices,
                                &object->m_objects[i].m_matrix);
        }
    }
}


// ---- src/CollisionObject/RenderSoultreeCollisionObject.cpp

// Debug draw of a soultree's collision data in one of several modes (some
// compiled out).
void CollisionObject::RenderSoultreeCollisionObject(SoultreeCollisionObject *soultree, int depth, int mode)
{
    if (mode == 1) {
        RenderFaceTree(soultree->m_faceTree, depth, &soultree->m_matrix, soultree->m_vertices);
    } else if (mode == 2) {
    } else if (mode == 3) {
        RenderFaceTree(soultree->m_faceTree, depth, &soultree->m_matrix, &soultree->m_x10, soultree->m_vertices);
    } else if (mode == 4) {
        RenderFaceTree(soultree->m_faceTree, depth, &soultree->m_matrix, &soultree->m_x10, soultree->m_vertices);
        _D3DMATRIX m;
        MulMatrix(&m, &soultree->m_x110, &soultree->m_x90);
        RenderFaceTree(soultree->m_faceTree, depth, &m, soultree->m_vertices);
    } else if (mode == 5) {
    } else if (mode == 6) {
    } else if (mode == 7) {
        RenderFaceTreeFaces(soultree->m_faceTree, soultree->m_vertices, &soultree->m_matrix);
    }
}


// ---- src/CollisionObject/RenderFaceTree.cpp

// Debug draw of one level of a face tree. The drawing is compiled out; the
// first leaf test is left with empty branches (retail keeps its compare).
void CollisionObject::RenderFaceTree(BoundingBoxTree *tree, int depth, _D3DMATRIX *matrix, _D3DVECTOR *vertices)
{
    if (depth == 0) {
        if (tree->m_size < 0.0f) {
        } else {
        }
        if (tree->m_size < 0.0f) {
            _D3DVECTOR corners[3];
            TransformPoint(&corners[0], matrix, &vertices[TREE_FACE(tree)->m_vertexIndex[0]]);
            TransformPoint(&corners[1], matrix, &vertices[TREE_FACE(tree)->m_vertexIndex[1]]);
            TransformPoint(&corners[2], matrix, &vertices[TREE_FACE(tree)->m_vertexIndex[2]]);
        }
    } else if (!(tree->m_size < 0.0f)) {
        RenderFaceTree(tree->m_left, depth - 1, matrix, vertices);
        RenderFaceTree(tree->m_right, depth - 1, matrix, vertices);
    }
}


// ---- src/CollisionObject/RenderFaceTree_0021AFF0.cpp

// Debug draw of one level of a face tree under two transforms. The drawing
// is compiled out; the first leaf test is left with empty branches.
void CollisionObject::RenderFaceTree(BoundingBoxTree *tree, int depth, _D3DMATRIX *matrix, _D3DMATRIX *matrix2, _D3DVECTOR *vertices)
{
    if (depth == 0) {
        if (tree->m_size < 0.0f) {
        } else {
        }
        _D3DVECTOR min;
        _D3DVECTOR max;
        min = tree->m_min;
        max = tree->m_max;
        _D3DVECTOR *pmin = &min;    // taken early: fills the bc1t delay slot
        if (!(tree->m_size < 0.0f)) {
            ModifyBoundingBox(pmin, &max, matrix2);
        } else {
            _D3DVECTOR corners[2][3];   // under matrix, then also under matrix2
            TransformPoint(&corners[0][0], matrix, &vertices[TREE_FACE(tree)->m_vertexIndex[0]]);
            TransformPoint(&corners[0][1], matrix, &vertices[TREE_FACE(tree)->m_vertexIndex[1]]);
            TransformPoint(&corners[0][2], matrix, &vertices[TREE_FACE(tree)->m_vertexIndex[2]]);
            TransformPoint(&corners[1][0], matrix2, &corners[0][0]);
            TransformPoint(&corners[1][1], matrix2, &corners[0][1]);
            TransformPoint(&corners[1][2], matrix2, &corners[0][2]);
        }
    } else if (!(tree->m_size < 0.0f)) {
        RenderFaceTree(tree->m_left, depth - 1, matrix, matrix2, vertices);
        RenderFaceTree(tree->m_right, depth - 1, matrix, matrix2, vertices);
    }
}


// ---- src/CollisionObject/RenderFaceTreeFaces.cpp


// Debug draw of a face tree's faces with their normals. The drawing is
// compiled out; only the transforms remain.
void CollisionObject::RenderFaceTreeFaces(BoundingBoxTree *tree, _D3DVECTOR *vertices, _D3DMATRIX *matrix)
{
    if (tree->m_size < 0.0f) {
        _D3DVECTOR a;
        _D3DVECTOR b;
        _D3DVECTOR c;
        a = vertices[TREE_FACE(tree)->m_vertexIndex[0]];
        b = vertices[TREE_FACE(tree)->m_vertexIndex[1]];
        c = vertices[TREE_FACE(tree)->m_vertexIndex[2]];
        float third = 0.33333334f;     // a local: gives retail's FPR choice
        _D3DVECTOR center = ScaleVS(a + b + c, third);
        // face leaf: the normal (copied, w forced to 1; the w load is dropped)
        _D3DVECTOR normal = tree->m_min;
        normal.w = 1.0f;
        ApplyMatrixVector(&normal, matrix, normal);
        TransformPoint(&a, matrix, &a);
        TransformPoint(&b, matrix, &b);
        TransformPoint(&c, matrix, &c);
        TransformPoint(&center, matrix, &center);
        normal = center + 0.1f * normal;
    } else {
        RenderFaceTreeFaces(tree->m_left, vertices, matrix);
        RenderFaceTreeFaces(tree->m_right, vertices, matrix);
    }
}


// ---- src/CollisionObject/RenderRays.cpp

// Debug draw of a ray tree: the line drawing is compiled out, only the
// transforms remain.
void CollisionObject::RenderRays(BoundingBoxTree *tree, _D3DMATRIX *matrix)
{
    if (tree->m_size < 0.0f) {
        _D3DVECTOR start;
        _D3DVECTOR end;
        _D3DVECTOR *ray = &tree->m_min;   // the leaf's ray: start, end
        TransformPoint(&start, matrix, &ray[0]);
        TransformPoint(&end, matrix, &ray[1]);
    } else {
        RenderRays(tree->m_left, matrix);
        RenderRays(tree->m_right, matrix);
    }
}


// ---- src/CollisionObject/RenderRayTree.cpp

// Debug draw of one level of a ray tree. The drawing is compiled out; the
// first leaf test is left with empty branches (retail keeps its compare;
// an empty if without else is removed entirely).
void CollisionObject::RenderRayTree(BoundingBoxTree *tree, int depth, _D3DMATRIX *matrix)
{
    if (depth == 0) {
        if (tree->m_size < 0.0f) {
        } else {
        }
        if (tree->m_size < 0.0f) {
            _D3DVECTOR *ray = &tree->m_min;   // the leaf's ray: start, end
            _D3DVECTOR start;
            _D3DVECTOR end;
            TransformPoint(&start, matrix, &ray[0]);
            TransformPoint(&end, matrix, &ray[1]);
        }
    } else if (!(tree->m_size < 0.0f)) {
        RenderRayTree(tree->m_left, depth - 1, matrix);
        RenderRayTree(tree->m_right, depth - 1, matrix);
    }
}


// ---- src/CollisionObject/KeyDownHandler.cpp

enum DeviceType {};

int KEYDOWN_HANDLER_TEST(unsigned int key, DeviceType device, ProcessNode *node, unsigned int flags);

int CollisionObject::KeyDownHandler(ProcessNode *node, KeyInfo *key)
{
    if (KEYDOWN_HANDLER_TEST(0x26, (DeviceType)0, node, 0x80)) {
        m_debugIndex--;
        if (m_debugIndex < 0)
            m_debugIndex = 0;
    }
    if (KEYDOWN_HANDLER_TEST(0x27, (DeviceType)0, node, 0x80))
        m_debugIndex++;
    if (KEYDOWN_HANDLER_TEST(0x6, (DeviceType)0, node, 0x80)) {
        m_debugMode++;
        if (m_debugMode > 7)
            m_debugMode = 0;
    }
    return GameObject::KeyDownHandler(node, key);
}


// ---- src/CollisionObject/ResetCollisionData.cpp

extern int NumberOfNormals;
extern int LastNormal;


static inline float Sqrt(float x)
{
    float r;
    asm("sqrt.s $f8, %1; swc1 $f8, 0(%0)" : : "r"(&r), "f"(x) : "$f8", "memory");
    return r;
}

static inline float Fmax(float a, float b)
{
    float r;
    asm("max.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}

// (Going through Length, not Sqrt directly, gives retail's stack slots.)
static inline float Length(const _D3DVECTOR &v)
{
    return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// Clear the per-test collision data before a round of tests (and, for
// dynamic rays, rebuild the world-space rays and their bounding sphere).
void CollisionObject::ResetCollisionData()
{
    _D3DVECTOR start;
    _D3DVECTOR end;
    _D3DVECTOR direction;
    m_collided = 0;
    NumberOfNormals = 0;
    LastNormal = 0;
    switch (m_collisionType) {
    case COLLISION_SOULTREE:
        if (!m_intersectionData)
            m_intersectionData = new VertexRayToFaceIntersectionData;
        SetCurrentCollisionData(m_intersectionData);
        {
            VertexRayToFaceIntersectionData *data = m_intersectionData;
            data->m_x00 = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            data->m_normal = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            data->m_count = 0;
            data->m_x30 = 1.0f;
        }
        break;
    case COLLISION_SOULTREE_HIERARCHY:
        if (!m_intersectionData)
            m_intersectionData = new VertexRayToFaceIntersectionData;
        SetCurrentCollisionData(m_intersectionData);
        {
            VertexRayToFaceIntersectionData *data = m_intersectionData;
            data->m_x00 = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            data->m_normal = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            data->m_count = 0;
            data->m_x30 = 1.0f;
        }
        break;
    case COLLISION_DYNAMIC_RAYS: {
        int i;
        DynamicRayCollisionObject *rays = (DynamicRayCollisionObject *)m_collisionData;
        m_center = ZeroVector;
        m_searchRadius = 0;
        for (i = 0; i < rays->m_numRays; i++) {
            if (rays->m_objects[i * 2])
                TransformPoint(&start, &rays->m_objects[i * 2]->m_worldMatrix, &rays->m_localDirections[i * 2]);
            else
                start = rays->m_localDirections[i * 2];
            if (rays->m_objects[i * 2 + 1])
                TransformPoint(&end, &rays->m_objects[i * 2 + 1]->m_worldMatrix, &rays->m_localDirections[i * 2 + 1]);
            else
                end = rays->m_localDirections[i * 2 + 1];
            direction = end - start;
            float length = Length(direction);
            rays->m_starts[i] = start;
            rays->m_lengths[i] = length;
            if (length > 0.0f)
                rays->m_directions[i] = ScaleVS(direction, 1.0f / length);
            else
                rays->m_directions[i] = ZeroVector;
            m_center.x += start.x;
            m_center.y += start.y;
            m_center.z += start.z;
            m_searchRadius = Fmax(m_searchRadius, length);
        }
        float scale = 1.0f / (float)rays->m_numRays;
        m_center.x *= scale;
        m_center.y *= scale;
        m_center.z *= scale;
        rays->m_center = m_center;
        rays->m_radius = m_searchRadius;
    }
        // fall through
    case COLLISION_RAYS:
        SetCurrentCollisionData(m_intersectionData);
        {
            RayToFaceIntersectionData *data = (RayToFaceIntersectionData *)m_intersectionData;
            data->m_ray = 0;
            data->m_t = 1.0f;
            data->m_numRays = 0;
        }
        break;
    case COLLISION_SPHERE:
        SetCurrentCollisionData(m_intersectionData);
        ((SphereIntersectionData *)m_intersectionData)->m_count = 0;
        break;
    }
}


// ---- src/CollisionObject/GenerateCollisionPositionAndNormal.cpp

extern _D3DVECTOR NormalList[];

// Take the collision position from the intersection data and average the
// face normals gathered since the last call into the collision normal.
void CollisionObject::GenerateCollisionPositionAndNormal()
{
    switch (m_collisionType) {
    case 0:
    case 1: {
        // A dead local picked by type: its loads go, but the branch stays
        // (retail's lone "bnez; nop" to the next instruction).
        void *unused;
        if (m_collisionType == 0)
            unused = m_collisionData;
        else
            unused = m_collisionData;
        VertexRayToFaceIntersectionData *data = m_intersectionData;
        m_collisionPosition = data->m_position;
        data->m_normal = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        for (int i = LastNormal; i < NumberOfNormals; i++) {
            data->m_normal.x += NormalList[i].x;
            data->m_normal.y += NormalList[i].y;
            data->m_normal.z += NormalList[i].z;
        }
        Normalize(&data->m_normal, &data->m_normal);
        m_collisionNormal = data->m_normal;
        LastNormal = NumberOfNormals;
        break;
    }
    case 2:
    case 3:
        break;
    }
}


// ---- src/CollisionObject/FinishCollisionData.cpp


// Average the gathered face normals into the intersection data's normal and
// nudge its +0x00 vector along its own direction.
void CollisionObject::FinishCollisionData()
{
    _D3DVECTOR direction;
    switch (m_collisionType) {
    case 0:
    case 1: {
        VertexRayToFaceIntersectionData *data = m_intersectionData;
        data->m_normal = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        if (NumberOfNormals > 0) {
            for (int i = 0; i < NumberOfNormals; i++) {
                data->m_normal.x += NormalList[i].x;
                data->m_normal.y += NormalList[i].y;
                data->m_normal.z += NormalList[i].z;
            }
            Normalize(&data->m_normal, &data->m_normal);
        }
        Normalize(&direction, &data->m_x00);
        data->m_x00 += 0.075f * direction;
        break;
    }
    case 2:
    case 3:
        break;
    }
}


// ---- src/CollisionObject/UpdateSoultreeHierarchyBoundingBox.cpp

static inline float Fabs(float x)
{
    float r;
    asm("abs.s $f8, %1; swc1 $f8, 0(%0)" : : "r"(&r), "f"(x) : "$f8", "memory");
    return r;
}

static inline _D3DVECTOR VectorMin(const _D3DVECTOR &a, const _D3DVECTOR &b)
{
    return _D3DVECTOR(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
}

static inline _D3DVECTOR VectorMax(const _D3DVECTOR &a, const _D3DVECTOR &b)
{
    return _D3DVECTOR(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
}

// Rebuild the hierarchy's box (in its own space) from its active soultrees'
// boxes, keeping the previous one in m_x40/m_x50.
void CollisionObject::UpdateSoultreeHierarchyBoundingBox()
{
    SoultreeHierarchyCollisionObject *h = (SoultreeHierarchyCollisionObject *)m_collisionData;
    h->m_x40 = h->m_center;
    h->m_x50 = h->m_half;
    _D3DMATRIX inv;
    inv = h->m_matrix;
    InvertMatrix(&inv);
    _D3DVECTOR min;
    _D3DVECTOR max;
    for (int i = 0; i < h->m_count; i++) {
        if (!h->m_x04[i])
            continue;
        SoultreeCollisionObject *st = &h->m_objects[i];
        _D3DVECTOR axes[3];
        _D3DVECTOR half;
        axes[0] = _D3DVECTOR(st->m_faceTree->m_max.x, 0.0f, 0.0f);
        axes[1] = _D3DVECTOR(0.0f, st->m_faceTree->m_max.y, 0.0f);
        axes[2] = _D3DVECTOR(0.0f, 0.0f, st->m_faceTree->m_max.z);
        ApplyMatrixVector(&axes[0], &st->m_matrix, axes[0]);
        ApplyMatrixVector(&axes[0], &inv, axes[0]);
        ApplyMatrixVector(&axes[1], &st->m_matrix, axes[1]);
        ApplyMatrixVector(&axes[1], &inv, axes[1]);
        ApplyMatrixVector(&axes[2], &st->m_matrix, axes[2]);
        ApplyMatrixVector(&axes[2], &inv, axes[2]);
        half.x = Fabs(axes[2].x) + (Fabs(axes[0].x) + Fabs(axes[1].x));
        half.y = Fabs(axes[2].y) + (Fabs(axes[0].y) + Fabs(axes[1].y));
        half.z = Fabs(axes[2].z) + (Fabs(axes[0].z) + Fabs(axes[1].z));
        _D3DVECTOR center;
        TransformPoint(&center, &st->m_matrix, &st->m_faceTree->m_min);
        TransformPoint(&center, &inv, &center);
        if (i) {
            min = VectorMin(min, center - half);
            max = VectorMax(max, center + half);
        } else {
            min = center - half;
            max = center + half;
        }
    }
    h->m_center = 0.5f * (min + max);
    h->m_half = 0.5f * (max - min);
}


// ---- src/CollisionObject/InitializeTransform_0021CC30.cpp


// Set up the collision data at a first world matrix (no movement yet: the
// previous matrices are the same and the deltas identity), then update.
void CollisionObject::InitializeTransform(_D3DMATRIX *matrix)
{
    SoultreeCollisionObject *st;    // shared by two cases: gets retail's register
    _D3DVECTOR half;
    switch (m_collisionType) {
    case COLLISION_SOULTREE: {
        st = (SoultreeCollisionObject *)m_collisionData;
        st->m_x110 = *matrix;
        st->m_xD0 = *matrix;
        MulMatrix(&st->m_matrix, matrix, &st->m_x90);
        st->m_x10 = IdentityMatrix();
        m_center = GetMatrixRow(matrix, 3);
        half = st->m_faceTree->m_max;
        ModifyBoundingBox(&m_center, &half, &st->m_x10);
        m_searchRadius = Length(half);
        TransformPoint(&m_center, matrix, &m_center);
        UpdateTransform(matrix);
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        int i;
        SoultreeHierarchyCollisionObject *h = (SoultreeHierarchyCollisionObject *)m_collisionData;
        h->m_prevMatrix = *matrix;
        h->m_matrix = *matrix;
        h->m_xE0 = IdentityMatrix();
        for (i = 0; i < h->m_count; i++) {
            st = &h->m_objects[i];
            st->m_x110 = st->m_xD0;
            st->m_object->GetMatrix(0, &st->m_xD0);
            MulMatrix(&st->m_matrix, matrix, &st->m_x90);
            st->m_x10 = IdentityMatrix();
        }
        UpdateSoultreeHierarchyBoundingBox();
        m_center = h->m_center;
        half = h->m_half;
        ModifyBoundingBox(&m_center, &half, &h->m_xE0);
        m_searchRadius = Length(half);
        TransformPoint(&m_center, matrix, &m_center);
        UpdateTransform(matrix);
        break;
    }
    case COLLISION_RAYS: {
        RayCollisionObject *rays = (RayCollisionObject *)m_collisionData;
        rays->m_matrix = *matrix;
        m_center = rays->m_tree->m_min;
        half = rays->m_tree->m_max;
        m_searchRadius = Length(half);
        TransformPoint(&m_center, matrix, &m_center);
        UpdateTransform(matrix);
        break;
    }
    case COLLISION_DYNAMIC_RAYS:
    case COLLISION_RADIUS_LINE_SEGMENT:
        break;
    case COLLISION_SPHERE: {
        SphereCollisionObject *sphere = (SphereCollisionObject *)m_collisionData;
        if (!matrix)
            sphere->m_matrix = IdentityMatrix();
        else
            sphere->m_matrix = *matrix;
        m_center = sphere->center;
        m_searchRadius = sphere->radius;
        TransformPoint(&m_center, &sphere->m_matrix, &m_center);
        UpdateTransform(matrix);
        break;
    }
    case COLLISION_BOX: {
        BoxCollisionObject *box = (BoxCollisionObject *)m_collisionData;
        if (!matrix)
            box->m_matrix = IdentityMatrix();
        else
            box->m_matrix = *matrix;
        m_center = box->center;
        m_searchRadius = Length(box->half);
        TransformPoint(&m_center, &box->m_matrix, &m_center);
        UpdateTransform(matrix);
        break;
    }
    }
}


// ---- src/CollisionObject/InitializeTransform.cpp


void CollisionObject::InitializeTransform()
{
    switch (m_collisionType) {
    case COLLISION_SOULTREE: {
        _D3DMATRIX matrix;
        ((SoultreeCollisionObject *)m_collisionData)->m_object->GetMatrix(0, &matrix);
        InitializeTransform(&matrix);
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        _D3DMATRIX matrix;
        ((SoultreeHierarchyCollisionObject *)m_collisionData)->m_object->GetMatrix(0, &matrix);
        InitializeTransform(&matrix);
        break;
    }
    case COLLISION_RAYS: {
        _D3DMATRIX matrix;
        ((RayCollisionObject *)m_collisionData)->m_object->GetMatrix(0, &matrix);
        InitializeTransform(&matrix);
        break;
    }
    case COLLISION_DYNAMIC_RAYS:
    case COLLISION_RADIUS_LINE_SEGMENT:
        break;
    case COLLISION_SPHERE: {
        SoultreeObject *object = ((SphereCollisionObject *)m_collisionData)->m_object;
        if (object) {
            _D3DMATRIX matrix;
            object->GetMatrix(0, &matrix);
            InitializeTransform(&matrix);
        } else {
            InitializeTransform(0);
        }
        break;
    }
    case COLLISION_BOX:
        InitializeTransform(0);
        break;
    }
}


// ---- src/CollisionObject/UpdateTransform_0021D920.cpp

// Move the collision data to a new world matrix: keep the previous one for
// the movement deltas and recompute the search sphere.
void CollisionObject::UpdateTransform(_D3DMATRIX *matrix)
{
    SoultreeCollisionObject *st;    // shared by two cases: gets retail's register
    _D3DVECTOR half;
    switch (m_collisionType) {
    case COLLISION_SOULTREE: {
        st = (SoultreeCollisionObject *)m_collisionData;
        st->m_x110 = st->m_xD0;
        st->m_xD0 = *matrix;
        MulMatrix(&st->m_matrix, matrix, &st->m_x90);
        CreateDeltaTransform(&st->m_x10, matrix, &st->m_x110);
        m_center = st->m_faceTree->m_min;
        half = st->m_faceTree->m_max;
        ModifyBoundingBox(&m_center, &half, &st->m_x10);
        m_searchRadius = Length(half);
        TransformPoint(&m_center, matrix, &m_center);
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        SoultreeHierarchyCollisionObject *h = (SoultreeHierarchyCollisionObject *)m_collisionData;
        h->m_prevMatrix = h->m_matrix;
        h->m_matrix = *matrix;
        CreateDeltaTransform(&h->m_xE0, matrix, &h->m_prevMatrix);
        for (int i = 0; i < h->m_count; i++) {
            st = &h->m_objects[i];
            st->m_x110 = st->m_xD0;
            st->m_object->GetMatrix(0, &st->m_xD0);
            MulMatrix(&st->m_matrix, &st->m_xD0, &st->m_x90);
            CreateDeltaTransform(&st->m_x10, &st->m_xD0, &st->m_x110);
        }
        UpdateSoultreeHierarchyBoundingBox();
        m_center = h->m_center;
        half = h->m_half;
        ModifyBoundingBox(&m_center, &half, &h->m_xE0);
        m_searchRadius = Length(half);
        TransformPoint(&m_center, matrix, &m_center);
        break;
    }
    case COLLISION_RAYS: {
        RayCollisionObject *rays = (RayCollisionObject *)m_collisionData;
        rays->m_matrix = *matrix;
        m_center = rays->m_tree->m_min;
        half = rays->m_tree->m_max;
        m_searchRadius = Length(half);
        TransformPoint(&m_center, matrix, &m_center);
        break;
    }
    case COLLISION_DYNAMIC_RAYS:
    case COLLISION_RADIUS_LINE_SEGMENT:
        break;
    case COLLISION_SPHERE: {
        SphereCollisionObject *sphere = (SphereCollisionObject *)m_collisionData;
        if (!matrix)
            sphere->m_matrix = IdentityMatrix();
        else
            sphere->m_matrix = *matrix;
        m_center = sphere->center;
        m_searchRadius = sphere->radius;
        TransformPoint(&m_center, &sphere->m_matrix, &m_center);
        break;
    }
    case COLLISION_BOX: {
        BoxCollisionObject *box = (BoxCollisionObject *)m_collisionData;
        if (!matrix)
            box->m_matrix = IdentityMatrix();
        else
            box->m_matrix = *matrix;
        m_center = box->center;
        m_searchRadius = Length(box->half);
        TransformPoint(&m_center, &box->m_matrix, &m_center);
        break;
    }
    }
}


// ---- src/CollisionObject/UpdateTransform.cpp


void CollisionObject::UpdateTransform()
{
    switch (m_collisionType) {
    case COLLISION_SOULTREE: {
        _D3DMATRIX matrix;
        ((SoultreeCollisionObject *)m_collisionData)->m_object->GetMatrix(0, &matrix);
        UpdateTransform(&matrix);
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        _D3DMATRIX matrix;
        ((SoultreeHierarchyCollisionObject *)m_collisionData)->m_object->GetMatrix(0, &matrix);
        UpdateTransform(&matrix);
        break;
    }
    case COLLISION_RAYS: {
        _D3DMATRIX matrix;
        ((RayCollisionObject *)m_collisionData)->m_object->GetMatrix(0, &matrix);
        UpdateTransform(&matrix);
        break;
    }
    case COLLISION_DYNAMIC_RAYS:
    case COLLISION_RADIUS_LINE_SEGMENT:
        break;
    case COLLISION_SPHERE: {
        SoultreeObject *object = ((SphereCollisionObject *)m_collisionData)->m_object;
        if (object) {
            _D3DMATRIX matrix;
            object->GetMatrix(0, &matrix);
            UpdateTransform(&matrix);
        } else {
            UpdateTransform(0);
        }
        break;
    }
    case COLLISION_BOX:
        UpdateTransform(0);
        break;
    }
}


// ---- src/CollisionObject/UpdateCollisionInfo.cpp

void CollisionObject::UpdateCollisionInfo()
{
    UpdateTransform();
    UpdateSearchPlacement();
}


// ---- src/CollisionObject/UpdateCollisionInfo_0021E480.cpp

void CollisionObject::UpdateCollisionInfo(_D3DMATRIX *matrix)
{
    UpdateTransform(matrix);
    UpdateSearchPlacement();
}


// ---- src/CollisionObject/AddSearchPlacement.cpp
#pragma alignlabel off
void CollisionObject::AddSearchPlacement(SearchNode *node, unsigned int flags)
{
    SearchPlacement temporary;
    SearchPlacement *placement;
    if (m_tempPlacement)
        placement = &temporary;
    else
        placement = m_searchPlacement = new SearchPlacement;
#line 1661 "CollisionObject.cpp"
    ASSERT(placement, "AddSearchPlacement: lpSearchPlacement is NULL");
    _D3DVECTOR center;
    _D3DVECTOR half;
    GetCenterHalf(&center, &half);
    placement->PerformInitialPlacement(this, node, flags, &center, &half, 0.0f);
#line 1669 "CollisionObject.cpp"
    ASSERT(placement->FirstPlacementItem.LpSearchNode != 0, "AddSearchPlacement: FirstPlacementItem.LpSearchNode == NULL.");
}
#pragma alignlabel reset


// ---- src/CollisionObject/UpdateSearchPlacement.cpp

void CollisionObject::UpdateSearchPlacement()
{
    if (m_searchPlacement) {
        _D3DVECTOR center;
        _D3DVECTOR half;
        GetCenterHalf(&center, &half);
        m_searchPlacement->UpdatePlacement(this, &center, &half, m_searchRadius);
    }
}


// ---- src/CollisionObject/RemoveFromSearch.cpp

void CollisionObject::RemoveFromSearch()
{
    if (m_searchPlacement) {
        m_searchPlacement->Remove(this);
        delete m_searchPlacement;
        m_searchPlacement = 0;
    }
}


// ---- src/CollisionObject/GetCenterHalf.cpp
// (Going through two helpers, not Sqrt directly, gives retail's stack slots.)
static inline float SqrtMagnitude(const _D3DVECTOR &v)
{
    return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float SqrtLength(const _D3DVECTOR &v)
{
    return SqrtMagnitude(v);
}

// out = m * (v.xyz, 1): rotate and translate a point.
static inline void ApplyMatrixPoint(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR *v)
{
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x10(%1); lqc2 vf6, 0x20(%1); lqc2 vf7, 0x30(%1); lqc2 vf8, 0x0(%0); vmulw.w vf9, vf0, vf0w; vmulax.xyz ACC, vf4, vf8x; vmadday.xyz ACC, vf5, vf8y; vmaddz.xyz vf9, vf6, vf8z; vadd.xyz vf9, vf9, vf7; sqc2 vf9, 0x0(%2)"
        : : "r"(v), "r"(m), "r"(out) : "memory");
}

// The shape's world bounding box as a center and an axis-aligned half size
// (dynamic rays first refit their own box and radius).
void CollisionObject::GetCenterHalf(_D3DVECTOR *center, _D3DVECTOR *half)
{
    _D3DVECTOR axes[3];
    _D3DMATRIX matrix;
    _D3DVECTOR localCenter;
    _D3DVECTOR localHalf;
    _D3DVECTOR min;
    _D3DVECTOR max;
    switch (m_collisionType) {
    case COLLISION_SOULTREE: {
        SoultreeCollisionObject *soultree = (SoultreeCollisionObject *)m_collisionData;
        localCenter = soultree->m_faceTree->m_min;
        localHalf = soultree->m_faceTree->m_max;
        matrix = soultree->m_matrix;
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        SoultreeHierarchyCollisionObject *hierarchy = (SoultreeHierarchyCollisionObject *)m_collisionData;
        localCenter = hierarchy->m_center;
        localHalf = hierarchy->m_half;
        matrix = hierarchy->m_matrix;
        break;
    }
    case COLLISION_RAYS: {
        RayCollisionObject *rays = (RayCollisionObject *)m_collisionData;
        localCenter = rays->m_tree->m_min;
        localHalf = rays->m_tree->m_max;
        matrix = rays->m_matrix;
        break;
    }
    case COLLISION_DYNAMIC_RAYS: {
        DynamicRayCollisionObject *rays = (DynamicRayCollisionObject *)m_collisionData;
        min.x = 3.4028235e38f;
        max.x = -3.4028235e38f;
        min.y = 3.4028235e38f;
        max.y = -3.4028235e38f;
        min.z = 3.4028235e38f;
        max.z = -3.4028235e38f;
        for (int i = 0; i < rays->m_numRays; i++) {
            _D3DVECTOR *start = &rays->m_starts[i];
            float length = rays->m_lengths[i];
            _D3DVECTOR *direction = &rays->m_directions[i];
            _D3DVECTOR end = *start + _D3DVECTOR(length * direction->x, length * direction->y, length * direction->z);
            if (end.x < min.x)
                min.x = end.x;
            if (end.x > max.x)
                max.x = end.x;
            if (end.y < min.y)
                min.y = end.y;
            if (end.y > max.y)
                max.y = end.y;
            if (end.z < min.z)
                min.z = end.z;
            if (end.z > max.z)
                max.z = end.z;
            if (start->x < min.x)
                min.x = start->x;
            if (start->x > max.x)
                max.x = start->x;
            if (start->y < min.y)
                min.y = start->y;
            if (start->y > max.y)
                max.y = start->y;
            if (start->z < min.z)
                min.z = start->z;
            if (start->z > max.z)
                max.z = start->z;
        }
        // Written out (not operator-/+): retail's FP registers.
        localHalf = 0.5f * _D3DVECTOR(max.x - min.x, max.y - min.y, max.z - min.z);
        localCenter = 0.5f * _D3DVECTOR(max.x + min.x, max.y + min.y, max.z + min.z);
        rays->m_center = localCenter;
        rays->m_radius = SqrtLength(localHalf);
        m_center = localCenter;
        m_searchRadius = SqrtLength(localHalf);
        matrix = IdentityMatrix();
        break;
    }
    case COLLISION_SPHERE: {
        SphereCollisionObject *sphere = (SphereCollisionObject *)m_collisionData;
        localCenter = sphere->m_scale * sphere->center;
        float radius = sphere->radius * sphere->m_radiusScale;
        localHalf = radius * _D3DVECTOR(1.0f, 1.0f, 1.0f);
        matrix = sphere->m_matrix;
        break;
    }
    case COLLISION_BOX: {
        BoxCollisionObject *box = (BoxCollisionObject *)m_collisionData;
        localCenter = box->center;
        localHalf = box->half;
        matrix = box->m_matrix;
        break;
    }
    case COLLISION_RADIUS_LINE_SEGMENT: {
        RadiusLineSegmentCollisionObject *segment = (RadiusLineSegmentCollisionObject *)m_collisionData;
        _D3DVECTOR lo;
        _D3DVECTOR hi;
        _D3DVECTOR start;
        _D3DVECTOR end;
        start = segment->m_scale * segment->start;
        end = segment->m_scale * segment->end;
        float radius = segment->radius * segment->m_radiusScale;
        if (start.x < end.x) {
            lo.x = start.x - radius;
            hi.x = end.x + radius;
        } else {
            lo.x = end.x - radius;
            hi.x = start.x + radius;
        }
        if (start.y < end.y) {
            lo.y = start.y - radius;
            hi.y = end.y + radius;
        } else {
            lo.y = end.y - radius;
            hi.y = start.y + radius;
        }
        if (start.z < end.z) {
            lo.z = start.z - radius;
            hi.z = end.z + radius;
        } else {
            lo.z = end.z - radius;
            hi.z = start.z + radius;
        }
        localCenter = 0.5f * _D3DVECTOR(lo.x + hi.x, lo.y + hi.y, lo.z + hi.z);
        localHalf = 0.5f * _D3DVECTOR(hi.x - lo.x, hi.y - lo.y, hi.z - lo.z);
        matrix = segment->m_matrix;
        break;
    }
    }
    axes[0] = _D3DVECTOR(localHalf.x, 0.0f, 0.0f);
    axes[1] = _D3DVECTOR(0.0f, localHalf.y, 0.0f);
    axes[2] = _D3DVECTOR(0.0f, 0.0f, localHalf.z);
    ApplyMatrixPoint(center, &matrix, &localCenter);
    ApplyMatrixVector(&axes[0], &matrix, axes[0]);
    ApplyMatrixVector(&axes[1], &matrix, axes[1]);
    ApplyMatrixVector(&axes[2], &matrix, axes[2]);
    half->x = Fabs(axes[2].x) + (Fabs(axes[0].x) + Fabs(axes[1].x));
    half->y = Fabs(axes[2].y) + (Fabs(axes[0].y) + Fabs(axes[1].y));
    half->z = Fabs(axes[2].z) + (Fabs(axes[0].z) + Fabs(axes[1].z));
}


// ---- src/CollisionObject/ValidTestObject.cpp
// Applies one include/exclude list: listed objects are rejected when the
// list type is 1 (exclude), unlisted ones when it is 0 (include only).
#define CHECK_TEST_LIST(list, count, type, object)              \
    if ((count) > 0) {                                          \
        found = 0;                                              \
        for (i = 0; i < (count); i++) {                     \
            if ((list)[i] == (object)) {                        \
                found = 1;                                      \
                break;                                          \
            }                                                   \
        }                                                       \
        if (found && (type) == 1)                               \
            return 0;                                           \
        if (!found && (type) == 0)                              \
            return 0;                                           \
    }

int CollisionObject::ValidTestObject(CollisionObject *other)
{
    int i;
    int found;
    if (other == this)
        return 0;
    if (!other->m_collidable)
        return 0;
    if (!other->m_enabled)
        return 0;
    if (!(m_testMask & other->m_ownerType))
        return 0;
    CHECK_TEST_LIST(m_testList, m_numTestObjects, m_testListType, other);
    CHECK_TEST_LIST(other->m_testList, other->m_numTestObjects, other->m_testListType, this);
    CHECK_TEST_LIST(m_testList2, m_numTestObjects2, m_testList2Type, other);
    CHECK_TEST_LIST(other->m_testList2, other->m_numTestObjects2, other->m_testList2Type, this);
    if (m_objectTypeListType == 1) {
        for (i = 0; i < m_numObjectTypes; i++) {
            if (other->m_ownerType & m_objectTypes[i])
                return 0;
        }
    } else {
        found = 0;
        for (i = 0; i < m_numObjectTypes; i++) {
            if (other->m_ownerType & m_objectTypes[i])
                found = 1;
        }
        if (!found)
            return 0;
    }
    if (other->m_objectTypeListType == 1) {
        for (i = 0; i < other->m_numObjectTypes; i++) {
            if (m_ownerType & other->m_objectTypes[i])
                return 0;
        }
    } else {
        found = 0;
        for (i = 0; i < other->m_numObjectTypes; i++) {
            if (m_ownerType & other->m_objectTypes[i])
                found = 1;
        }
        if (!found)
            return 0;
    }
    return 1;
}


// ---- src/CollisionObject/TestForCollision.cpp

int CollisionObject::TestForCollision(CollisionObject *other)
{
    switch (m_collisionType) {
    case COLLISION_SOULTREE:
        return TestSoultreeAgainst(other);
    case COLLISION_SOULTREE_HIERARCHY:
        return TestSoultreeHierarchyAgainst(other);
    case COLLISION_RAYS:
        return TestRaysAgainst(other);
    case COLLISION_DYNAMIC_RAYS:
        return TestRayBundleForCollision((DynamicRayCollisionObject *)m_collisionData, other,
                                         (RayToFaceIntersectionData *)m_intersectionData);
    case COLLISION_RADIUS_LINE_SEGMENT:
        break;
    case COLLISION_SPHERE: {
        SphereCollisionObject *sphere = (SphereCollisionObject *)m_collisionData;
        return TestSphereForCollision(&m_center, sphere->radius, sphere->radiusSquared, other);
    }
    case COLLISION_BOX:
        return TestBoxAgainst(other);
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeAgainstSoultree.cpp


// Soultree against soultree: test the two trees in b's space, averaging the
// hit positions back into world space.
int CollisionObject::TestSoultreeAgainstSoultree(SoultreeCollisionObject *a, SoultreeCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    if (a->m_x00)
        return TestSoultreeAgainstSoultreeReverse(a, b, data);
    InverseRotate(&data->m_x00, &b->m_matrix, data->m_x00);
    InverseRotate(&data->m_normal, &b->m_matrix, data->m_normal);
    data->m_position = _D3DVECTOR(0.0f, 0.0f, 0.0f);
    data->m_count = 0;
    for (int i = 0; i < NumberOfNormals; i++)
        InverseRotate(&NormalList[i], &b->m_matrix, NormalList[i]);
    int hit = TestBoundingBoxTreesForIntersection(a->m_tree, b->m_faceTree, &a->m_matrix, &b->m_matrix, (BoundingBoxTreeType)2, b->m_vertices, &a->m_x10);
    if (hit) {
        data->m_position = ScaleVS(data->m_position, 1.0f / (float)data->m_count);
        TransformPoint(&data->m_position, &b->m_matrix, &data->m_position);
    }
    ApplyMatrixVector(&data->m_x00, &b->m_matrix, data->m_x00);
    ApplyMatrixVector(&data->m_normal, &b->m_matrix, data->m_normal);
    for (int j = 0; j < NumberOfNormals; j++)
        ApplyMatrixVector(&NormalList[j], &b->m_matrix, NormalList[j]);
    return hit;
}


// ---- src/CollisionObject/TestSoultreeAgainstSoultreeReverse.cpp


// Soultree against soultree, the other way round: test b's tree against a's
// faces in a's space (directions flipped), averaging the hit positions back
// into world space.
int CollisionObject::TestSoultreeAgainstSoultreeReverse(SoultreeCollisionObject *a, SoultreeCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    int hit = 0;
    for (int i = 0; i < NumberOfNormals; i++)
        InverseRotate(&NormalList[i], &a->m_matrix, NormalList[i]);
    InverseRotate(&data->m_x00, &a->m_matrix, -data->m_x00);
    InverseRotate(&data->m_normal, &a->m_matrix, -data->m_normal);
    data->m_position = _D3DVECTOR(0.0f, 0.0f, 0.0f);
    data->m_count = 0;
    CreateDeltaTransformForStaticObject(&b->m_transform, &b->m_matrix, &a->m_xD0, &a->m_x110);
    // (retail ORs into a still-zero hit: "hit || ..." builds a bool)
    hit = hit || TestBoundingBoxTreesForIntersection(b->m_tree, a->m_faceTree, &b->m_matrix, &a->m_matrix, (BoundingBoxTreeType)2, a->m_vertices, &b->m_transform);
    if (hit) {
        data->m_position = ScaleVS(data->m_position, 1.0f / (float)data->m_count);
        TransformPoint(&data->m_position, &a->m_matrix, &data->m_position);
    }
    ApplyMatrixVector(&data->m_x00, &a->m_matrix, -data->m_x00);
    ApplyMatrixVector(&data->m_normal, &a->m_matrix, -data->m_normal);
    for (int j = 0; j < NumberOfNormals; j++)
        ApplyMatrixVector(&NormalList[j], &a->m_matrix, NormalList[j]);
    return hit;
}


// ---- src/CollisionObject/TestSoultreeAgainstSoultreeHierarchy.cpp
// TestSoultreeAgainstSoultreeHierarchy__15CollisionObjectFP23SoultreeCollisionObjectP32SoultreeHierarchyCollisionObjectP31VertexRayToFaceIntersectionData  @ 0x00220440


// Soultree against hierarchy: if the boxes overlap, test a against each of
// b's active soultrees (in that soultree's space, as TestSoultreeAgainstSoultree
// does) and average the hit positions into data.
int CollisionObject::TestSoultreeAgainstSoultreeHierarchy(SoultreeCollisionObject *a, SoultreeHierarchyCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    if (a->m_x00)
        return TestSoultreeAgainstSoultreeHierarchyReverse(a, b, data);
    // Declaration order sets the $s registers (i $s6, hit $s0, count $s1); one
    // `j` serves both NormalList loops, which puts it in $v0 as in retail.
    int i;
    int j;
    int hit;
    int count;
    hit = 0;
    _D3DMATRIX m;
    m = a->m_xD0;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, b->m_matrix);
    if (OBBOverlapTestWithDelta(a->m_faceTree->m_min, a->m_faceTree->m_max, &b->m_center, &b->m_half, &m, &a->m_x10)) {
        _D3DVECTOR sum = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        _D3DVECTOR position;       // fetched but unused
        count = 0;
        a->m_object->GetPosition(0, &position);
        for (i = 0; i < b->m_count; i++) {
            _D3DVECTOR objectPosition;      // fetched but unused
            b->m_objects[i].m_object->GetPosition(0, &objectPosition);
            if (!b->m_x04[i])
                continue;
            SoultreeCollisionObject *s = &b->m_objects[i];
            InverseRotate(&data->m_x00, &s->m_matrix, data->m_x00);
            InverseRotate(&data->m_normal, &s->m_matrix, data->m_normal);
            data->m_position = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            data->m_count = 0;
            for (j = 0; j < NumberOfNormals; j++)
                InverseRotate(&NormalList[j], &s->m_matrix, NormalList[j]);
            if (TestBoundingBoxTreesForIntersection(a->m_tree, s->m_faceTree, &a->m_matrix, &s->m_matrix, (BoundingBoxTreeType)2, s->m_vertices, &a->m_x10)) {
                data->m_position = ScaleVS(data->m_position, 1.0f / (float)data->m_count);
                TransformPoint(&data->m_position, &s->m_matrix, &data->m_position);
                count++;
                hit = 1;
                sum.x += data->m_position.x;
                sum.y += data->m_position.y;
                sum.z += data->m_position.z;
            }
            ApplyMatrixVector(&data->m_x00, &s->m_matrix, data->m_x00);
            ApplyMatrixVector(&data->m_normal, &s->m_matrix, data->m_normal);
            for (j = 0; j < NumberOfNormals; j++)
                ApplyMatrixVector(&NormalList[j], &s->m_matrix, NormalList[j]);
        }
        if (hit)
            data->m_position = ScaleVS(sum, 1.0f / (float)count);
        return hit;
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeAgainstSoultreeHierarchyReverse.cpp

// Soultree against hierarchy, the other way round: if the boxes overlap,
// test a against each of b's active soultrees and average the hit positions
// into data.
int CollisionObject::TestSoultreeAgainstSoultreeHierarchyReverse(SoultreeCollisionObject *a, SoultreeHierarchyCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    int hit = 0;
    _D3DMATRIX m;
    m = a->m_xD0;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, b->m_matrix);
    if (OBBOverlapTestWithDelta(a->m_faceTree->m_min, a->m_faceTree->m_max, &b->m_center, &b->m_half, &m, &a->m_x10)) {
        _D3DVECTOR sum = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        _D3DVECTOR position;       // fetched but unused
        int count = 0;
        a->m_object->GetPosition(0, &position);
        for (int i = 0; i < b->m_count; i++) {
            if (b->m_x04[i] && TestSoultreeAgainstSoultreeReverse(a, &b->m_objects[i], data)) {
                count++;
                hit = 1;
                sum.x += data->m_position.x;
                sum.y += data->m_position.y;
                sum.z += data->m_position.z;
            }
        }
        if (hit)
            data->m_position = ScaleVS(sum, 1.0f / (float)count);
        return hit;
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeAgainstSphere.cpp

// Scale and transform the sphere into place, then test it against the soultree's
// bounding-box tree.
int CollisionObject::TestSoultreeAgainstSphere(SoultreeCollisionObject *soultree, SphereCollisionObject *sphere)
{
    _D3DVECTOR center;
    float s = sphere->m_scale;
    center = _D3DVECTOR(s * sphere->center.x, s * sphere->center.y, s * sphere->center.z);
    TransformPoint(&center, &sphere->m_matrix, &center);
    float radius = sphere->radius * sphere->m_radiusScale;
    return SphereIntersectsBoundingBoxTree(&center, radius, radius * radius, soultree->m_tree, &soultree->m_matrix, 0, 0);
}


// ---- src/CollisionObject/TestSoultreeAgainstBox.cpp


int BoxIntersectsBoundingBoxTree(_D3DVECTOR *center, _D3DVECTOR *half, _D3DMATRIX *matrix, BoundingBoxTree *tree,
                                 _D3DMATRIX *treeMatrix, int, _D3DVECTOR *vertices);

int CollisionObject::TestSoultreeAgainstBox(SoultreeCollisionObject *soultree, BoxCollisionObject *box)
{
    return BoxIntersectsBoundingBoxTree(&box->center, &box->half, &box->m_matrix, soultree->m_faceTree,
                                        &soultree->m_matrix, 1, soultree->m_vertices);
}


// ---- src/CollisionObject/TestSoultreeAgainstRadiusLineSegment.cpp

// Scale and transform the segment into place, then test it against the
// soultree's bounding-box tree.
int CollisionObject::TestSoultreeAgainstRadiusLineSegment(SoultreeCollisionObject *soultree, RadiusLineSegmentCollisionObject *segment)
{
    _D3DVECTOR pts[2];             // start, end: the callee reads both through pts
    pts[0] = _D3DVECTOR(segment->m_scale * segment->start.x, segment->m_scale * segment->start.y, segment->m_scale * segment->start.z);
    pts[1] = _D3DVECTOR(segment->m_scale * segment->end.x, segment->m_scale * segment->end.y, segment->m_scale * segment->end.z);
    TransformPoint(&pts[0], &segment->m_matrix, &pts[0]);
    TransformPoint(&pts[1], &segment->m_matrix, &pts[1]);
    float radius = segment->radius * segment->m_radiusScale;
    return RadiusedLineSegmentIntersectsBoundingBoxTree(&pts[0], radius, radius * radius, soultree->m_tree, &soultree->m_matrix, 0, soultree->m_vertices);
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstSoultree.cpp

// Hierarchy against soultree: if the boxes overlap, test each of a's active
// soultrees against b and average the hit positions into data.
int CollisionObject::TestSoultreeHierarchyAgainstSoultree(SoultreeHierarchyCollisionObject *a, SoultreeCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    if (a->m_x0C)
        return TestSoultreeHierarchyAgainstSoultreeReverse(a, b, data);
    int hit = 0;
    _D3DMATRIX m;
    m = a->m_matrix;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, b->m_matrix);
    if (OBBOverlapTestWithDelta(a->m_center, a->m_half, &b->m_faceTree->m_min, &b->m_faceTree->m_max, &m, &a->m_xE0)) {
        _D3DVECTOR sum = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        int count = 0;
        for (int i = 0; i < a->m_count; i++) {
            if (a->m_x04[i] && TestSoultreeAgainstSoultree(&a->m_objects[i], b, data)) {
                count++;
                hit = 1;
                sum.x += data->m_position.x;
                sum.y += data->m_position.y;
                sum.z += data->m_position.z;
            }
        }
        if (hit)
            data->m_position = ScaleVS(sum, 1.0f / (float)count);
        return hit;
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstSoultreeReverse.cpp

// Hierarchy against soultree, the other way round: grow a's box by its
// movement, and if it overlaps b's, test each of a's active soultrees
// against b and average the hit positions into data.
int CollisionObject::TestSoultreeHierarchyAgainstSoultreeReverse(SoultreeHierarchyCollisionObject *a, SoultreeCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    int hit = 0;
    _D3DMATRIX m;
    m = a->m_matrix;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, b->m_xD0);
    _D3DVECTOR center = a->m_center;
    _D3DVECTOR half = a->m_half;
    ModifyBoundingBox(&center, &half, a->m_x40, a->m_x50, &a->m_xE0);
    a->m_sweptCenter = center;
    a->m_sweptHalf = half;
    if (OBBOverlapTestWithDelta(center, half, &b->m_faceTree->m_min, &b->m_faceTree->m_max, &m, &a->m_xE0)) {
        _D3DVECTOR sum = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        int count = 0;
        for (int i = 0; i < a->m_count; i++) {
            if (a->m_x04[i] && TestSoultreeAgainstSoultreeReverse(&a->m_objects[i], b, data)) {
                count++;
                hit = 1;
                sum.x += data->m_position.x;
                sum.y += data->m_position.y;
                sum.z += data->m_position.z;
            }
        }
        if (hit)
            data->m_position = ScaleVS(sum, 1.0f / (float)count);
        return hit;
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstSoultreeHierarchy.cpp

// Hierarchy against hierarchy: if the boxes overlap, test a against each of
// b's soultrees and average the hit positions into data.
int CollisionObject::TestSoultreeHierarchyAgainstSoultreeHierarchy(SoultreeHierarchyCollisionObject *a, SoultreeHierarchyCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    if (a->m_x0C)
        return TestSoultreeHierarchyAgainstSoultreeHierarchyReverse(a, b, data);
    int hit = 0;
    _D3DMATRIX m;
    m = a->m_matrix;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, b->m_matrix);
    if (OBBOverlapTestWithDelta(a->m_center, a->m_half, &b->m_center, &b->m_half, &m, &a->m_xE0)) {
        _D3DVECTOR sum = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        int count = 0;
        for (int i = 0; i < b->m_count; i++) {
            if (TestSoultreeHierarchyAgainstSoultree(a, &b->m_objects[i], data)) {
                count++;
                hit = 1;
                sum.x += data->m_position.x;
                sum.y += data->m_position.y;
                sum.z += data->m_position.z;
            }
        }
        if (hit)
            data->m_position = ScaleVS(sum, 1.0f / (float)count);
        return hit;
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstSoultreeHierarchyReverse.cpp

// Hierarchy against hierarchy, the other way round: grow a's box by its
// movement, and if it overlaps b's, test a against each of b's soultrees
// (the last hit position wins).
int CollisionObject::TestSoultreeHierarchyAgainstSoultreeHierarchyReverse(SoultreeHierarchyCollisionObject *a, SoultreeHierarchyCollisionObject *b, VertexRayToFaceIntersectionData *data)
{
    int hit = 0;
    _D3DMATRIX m;
    m = a->m_matrix;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, b->m_matrix);
    _D3DVECTOR center = a->m_center;
    _D3DVECTOR half = a->m_half;
    ModifyBoundingBox(&center, &half, a->m_x40, a->m_x50, &a->m_xE0);
    a->m_sweptCenter = center;
    a->m_sweptHalf = half;
    if (OBBOverlapTestWithDelta(center, half, &b->m_center, &b->m_half, &m, &a->m_xE0)) {
        _D3DVECTOR position = _D3DVECTOR(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < b->m_count; i++) {
            if (TestSoultreeHierarchyAgainstSoultreeReverse(a, &b->m_objects[i], data)) {
                hit = 1;
                position = data->m_position;
            }
        }
        if (hit)
            data->m_position = position;
        return hit;
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstSphere.cpp

// Scale and transform the sphere into place, test it against the hierarchy's
// box, then against each soultree's bounding-box tree.
int CollisionObject::TestSoultreeHierarchyAgainstSphere(SoultreeHierarchyCollisionObject *hierarchy, SphereCollisionObject *sphere)
{
    _D3DVECTOR center;
    float s = sphere->m_scale;
    center = _D3DVECTOR(s * sphere->center.x, s * sphere->center.y, s * sphere->center.z);
    TransformPoint(&center, &sphere->m_matrix, &center);
    float radius = sphere->radius * sphere->m_radiusScale;
    float radiusSquared = radius * radius;
    if (OBBVsSphereOverlapTest(&hierarchy->m_center, &hierarchy->m_half, center, radius, radiusSquared, &hierarchy->m_matrix)) {
        for (int i = 0; i < hierarchy->m_count; i++) {
            if (SphereIntersectsBoundingBoxTree(&center, radius, radiusSquared, hierarchy->m_objects[i].m_tree, &hierarchy->m_objects[i].m_matrix, 0, 0))
                return 1;
        }
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstBox.cpp

// Test the box against the hierarchy's box (in the box's space), then
// against each soultree's bounding-box tree.
int CollisionObject::TestSoultreeHierarchyAgainstBox(SoultreeHierarchyCollisionObject *hierarchy, BoxCollisionObject *box)
{
    _D3DMATRIX m;
    m = box->m_matrix;
    InvertMatrix(&m);
    MulMatrixCopy(&m, m, hierarchy->m_matrix);
    if (OBBOverlapTest(&box->center, &box->half, hierarchy->m_center, hierarchy->m_half, &m)) {
        for (int i = 0; i < hierarchy->m_count; i++) {
            if (BoxIntersectsBoundingBoxTree(&box->center, &box->half, &box->m_matrix, hierarchy->m_objects[i].m_tree, &hierarchy->m_objects[i].m_matrix, 0))
                return 1;
        }
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainstRadiusLineSegment.cpp

// Scale and transform the segment into place, test it against the
// hierarchy's box, then against each soultree's bounding-box tree.
int CollisionObject::TestSoultreeHierarchyAgainstRadiusLineSegment(SoultreeHierarchyCollisionObject *hierarchy, RadiusLineSegmentCollisionObject *segment)
{
    _D3DVECTOR pts[2];             // start, end: the callees read both through pts
    pts[0] = _D3DVECTOR(segment->m_scale * segment->start.x, segment->m_scale * segment->start.y, segment->m_scale * segment->start.z);
    pts[1] = _D3DVECTOR(segment->m_scale * segment->end.x, segment->m_scale * segment->end.y, segment->m_scale * segment->end.z);
    TransformPoint(&pts[0], &segment->m_matrix, &pts[0]);
    TransformPoint(&pts[1], &segment->m_matrix, &pts[1]);
    float radius = segment->radius * segment->m_radiusScale;
    float radiusSquared = radius * radius;
    if (OBBVsRadiusedLineSegmentOverlapTest(&hierarchy->m_center, &hierarchy->m_half, pts, radius, radiusSquared, &hierarchy->m_matrix)) {
        for (int i = 0; i < hierarchy->m_count; i++) {
            if (RadiusedLineSegmentIntersectsBoundingBoxTree(&pts[0], radius, radiusSquared, hierarchy->m_objects[i].m_tree, &hierarchy->m_objects[i].m_matrix, 0, hierarchy->m_objects[i].m_vertices))
                return 1;
        }
    }
    return 0;
}


// ---- src/CollisionObject/TestSoultreeAgainst.cpp

int CollisionObject::TestSoultreeAgainst(CollisionObject *other)
{
    SoultreeCollisionObject *soultree = (SoultreeCollisionObject *)m_collisionData;
    switch (other->m_collisionType) {
    case COLLISION_SOULTREE:
        return TestSoultreeAgainstSoultree(soultree, (SoultreeCollisionObject *)other->m_collisionData, m_intersectionData);
    case COLLISION_SOULTREE_HIERARCHY:
        return TestSoultreeAgainstSoultreeHierarchy(soultree, (SoultreeHierarchyCollisionObject *)other->m_collisionData, m_intersectionData);
    case COLLISION_SPHERE:
        return TestSoultreeAgainstSphere(soultree, (SphereCollisionObject *)other->m_collisionData);
    case COLLISION_BOX:
        return TestSoultreeAgainstBox(soultree, (BoxCollisionObject *)other->m_collisionData);
    case COLLISION_RADIUS_LINE_SEGMENT:
        return TestSoultreeAgainstRadiusLineSegment(soultree, (RadiusLineSegmentCollisionObject *)other->m_collisionData);
    case COLLISION_TYPE_7:
    default:
        return 0;
    }
}


// ---- src/CollisionObject/TestSoultreeHierarchyAgainst.cpp

int CollisionObject::TestSoultreeHierarchyAgainst(CollisionObject *other)
{
    SoultreeHierarchyCollisionObject *hierarchy = (SoultreeHierarchyCollisionObject *)m_collisionData;
    switch (other->m_collisionType) {
    case COLLISION_SOULTREE:
        return TestSoultreeHierarchyAgainstSoultree(hierarchy, (SoultreeCollisionObject *)other->m_collisionData, m_intersectionData);
    case COLLISION_SOULTREE_HIERARCHY:
        return TestSoultreeHierarchyAgainstSoultreeHierarchy(hierarchy, (SoultreeHierarchyCollisionObject *)other->m_collisionData, m_intersectionData);
    case COLLISION_SPHERE:
        return TestSoultreeHierarchyAgainstSphere(hierarchy, (SphereCollisionObject *)other->m_collisionData);
    case COLLISION_BOX:
        return TestSoultreeHierarchyAgainstBox(hierarchy, (BoxCollisionObject *)other->m_collisionData);
    case COLLISION_RADIUS_LINE_SEGMENT:
        return TestSoultreeHierarchyAgainstRadiusLineSegment(hierarchy, (RadiusLineSegmentCollisionObject *)other->m_collisionData);
    case COLLISION_TYPE_7:
    default:
        return 0;
    }
}


// ---- src/CollisionObject/TestRaysAgainst.cpp
// TestRaysAgainst__15CollisionObjectFP15CollisionObject  @ 0x00222880


// Moves the current ray hit data, found in the soultree's space, into world
// space (the shared parts once, then each ray not yet moved). The ray counter
// is the caller's: both expansions share one `i`, whose single node (the union
// of both loops' interferences) is coloured first and gets retail's $v0.
static inline void TransformRayData(SoultreeCollisionObject *soultree, int &i)
{
    RayToFaceIntersectionData *data = CurrentCollisionData;
    if (!data->m_x00) {
        ApplyMatrixVector(&data->m_normal, &soultree->m_matrix, data->m_normal);
        ApplyMatrixVector(&data->m_penetration, &soultree->m_matrix, data->m_penetration);
        TransformPoint(&data->m_point, &soultree->m_matrix, &data->m_point);
        data->m_x00 = 1;
    }
    for (i = 0; i < data->m_numRays; i++) {
        if (!data->m_rayTransformed[i]) {
            ApplyMatrixVector(&data->m_rayNormals[i], &soultree->m_matrix, data->m_rayNormals[i]);
            TransformPoint(&data->m_rayPoints[i], &soultree->m_matrix, &data->m_rayPoints[i]);
            data->m_rayTransformed[i] = 1;
        }
    }
}

// A ray set against another object: the ray tree against the other's face
// tree (soultrees), sphere or box. On a soultree hit the hit data, still in
// the soultree's space, is moved into world space.
int CollisionObject::TestRaysAgainst(CollisionObject *other)
{
    SoultreeCollisionObject *soultree;
    int i;
    RayCollisionObject *rays = (RayCollisionObject *)m_collisionData;
    switch (other->m_collisionType) {
    case COLLISION_SOULTREE: {
        soultree = (SoultreeCollisionObject *)other->m_collisionData;
        if (!TestBoundingBoxTreesForIntersection(rays->m_tree, soultree->m_faceTree, &rays->m_matrix, &soultree->m_matrix, (BoundingBoxTreeType)1, soultree->m_vertices, 0))
            break;
        TransformRayData(soultree, i);
        return 1;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        int hit;
        SoultreeHierarchyCollisionObject *hierarchy = (SoultreeHierarchyCollisionObject *)other->m_collisionData;
        hit = 0;
        if (CheckForOverlap(other)) {
        for (int j = 0; j < hierarchy->m_count; j++) {
            soultree = &hierarchy->m_objects[j];
            if (!hierarchy->m_x04[j])
                continue;
            if (!TestBoundingBoxTreesForIntersection(rays->m_tree, soultree->m_faceTree, &rays->m_matrix, &soultree->m_matrix, (BoundingBoxTreeType)1, soultree->m_vertices, 0))
                continue;
            hit = 1;
            TransformRayData(soultree, i);
        }
        }
        return hit;
    }
    case COLLISION_SPHERE: {
        SphereCollisionObject *sphere = (SphereCollisionObject *)other->m_collisionData;
        return SphereIntersectsBoundingBoxTree(&sphere->center, sphere->radius, sphere->radiusSquared, rays->m_tree, &rays->m_matrix, 0, 0);
    }
    case COLLISION_BOX: {
        BoxCollisionObject *box = (BoxCollisionObject *)other->m_collisionData;
        return BoxIntersectsBoundingBoxTree(&box->center, &box->half, &box->m_matrix, rays->m_tree, &rays->m_matrix, 0);
    }
    case COLLISION_RAYS:
    case COLLISION_DYNAMIC_RAYS:
    case COLLISION_RADIUS_LINE_SEGMENT:
    case COLLISION_TYPE_7:
    default:
        break;
    }
    return 0;
}


// ---- src/CollisionObject/TestBoxAgainst.cpp

int CollisionObject::TestBoxAgainst(CollisionObject *other)
{
    BoxCollisionObject *box = (BoxCollisionObject *)m_collisionData;
    switch (other->m_collisionType) {
    case COLLISION_SOULTREE:
        return TestSoultreeAgainstBox((SoultreeCollisionObject *)other->m_collisionData, box);
    case COLLISION_SOULTREE_HIERARCHY:
        return TestSoultreeHierarchyAgainstBox((SoultreeHierarchyCollisionObject *)other->m_collisionData, box);
    default:
        return 0;
    }
}


// ---- src/CollisionObject/CheckForCollision.cpp

extern EcoSystem *LpCurrentEcoSystem;
static inline float Fmin(float a, float b)
{
    float r;
    asm("min.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}


#define REPORT_COLLISION()                                                      \
    m_collided = 1;                                                             \
    GenerateCollisionPositionAndNormal();                                       \
    m_testObject->m_collisionPosition = m_collisionPosition;                    \
    m_testObject->m_collisionNormal = m_collisionNormal;                        \
    if (m_callback)                                                             \
        m_callback(this, m_testObject);                                         \
    if (m_testObject->m_otherCallback)                                          \
        m_testObject->m_otherCallback(m_testObject, this);

// Tests against everything near; returns the distance to the nearest object
// not touched (0 after a collision), which also sets how far this object may
// move before the next full check.
float CollisionObject::CheckForCollision(int flags)
{
    if (!m_searchable) {
        m_collided = 0;
        return 3.40282347e+38f;
    }
    static int meter = -1;
    m_x64 -= Magnitude(m_center - m_half);
    m_half = m_center;
    if (flags && m_x64 > 0.0f)
        return m_x64;
    m_x64 = 0;
    ResetCollisionData();
    m_tested = 1;
    _D3DVECTOR center;
    _D3DVECTOR half;
    _D3DVECTOR searchMin;
    _D3DVECTOR searchMax;
    GetCenterHalf(&center, &half);
    SpatialSearch search;
    search.OpenSearch(m_searchPlacement, 1, &center, &half, &searchMin, &searchMax);
    float lowX = center.x - half.x - searchMin.x;
    float lowZ = center.z - half.z - searchMin.z;
    float highX = searchMax.x - (center.x + half.x);
    float highZ = searchMax.z - (center.z + half.z);
    float best = Fmin(Fmin(Fmin(lowX, lowZ), highX), highZ);
    SearchableObject *obj;
    while ((obj = search.NextSearchObject())) {
        if (obj->m_typeID == CollisionTypeID) {
            static int meter1 = -1;
            m_testObject = (CollisionObject *)obj;
            m_testObject->m_tested = 1;
            float d = TrivialCollisionObjectRejectionTest(this, m_testObject);
            if (d == 0.0f) {
                best = 0;
                if (ValidTestObject(m_testObject) && TestForCollision(m_testObject)) {
                    REPORT_COLLISION()
                }
            } else if (d < best) {
                best = d;
                if (flags && best < m_x64)
                    flags <<= 1;    // (sic) as retail: shifts the flag
            }
        } else if (!m_ignoreVegetation && obj->m_typeID == VegetationTypeID) {
            static int meter2 = -1;
            Vegetation *veg = (Vegetation *)obj;
            _D3DVECTOR pos;
            pos.x = veg->m_x * LpCurrentEcoSystem->m_positionScale;
            pos.z = veg->m_z * LpCurrentEcoSystem->m_positionScale;
            float d = Trivial2DCollisionObjectRejectionTest(this, &pos, veg->GetRadius());
            if (d == 0.0f) {
                for (int i = 0; i < veg->NumCollisionObjects(); i++) {
                    m_testObject = veg->GetCollisionObject(i);
                    best = 0;
                    m_testObject->m_tested = 1;
                    if (TestForCollision(m_testObject)) {
                        REPORT_COLLISION()
                    }
                }
            } else if (flags && d < best) {
                best = d;
                if (flags && best < m_x64)
                    flags <<= 1;    // (sic) as retail: shifts the flag
            }
        }
    }
    int moved = 0;
    for (SearchPlacementItem *item = &m_searchPlacement->FirstPlacementItem; item; item = item->LpNext) {
        if (item->LpSearchNode->m_staticMethod != item->LpSearchNode->m_dynamicMethod)
            moved = 1;
    }
    if (moved) {
        search.OpenSearch(m_searchPlacement, 2, &center, &half, 0, 0);
        while ((obj = search.NextSearchObject())) {
            if (obj->m_typeID == CollisionTypeID) {
                static int meter1 = -1;
                m_testObject = (CollisionObject *)obj;
                m_testObject->m_tested = 1;
                if (TrivialCollisionObjectRejectionTest(this, m_testObject) == 0.0f && ValidTestObject(m_testObject) &&
                    TestForCollision(m_testObject)) {
                    REPORT_COLLISION()
                }
            }
        }
    }
    if (m_collided) {
        FinishCollisionData();
        m_half = _D3DVECTOR(0.0f, -10000.0f, 0.0f);
        best = 0;
    }
    m_x64 = best;
    return best;
}


// ---- src/CollisionObject/SearchForCollisions.cpp


// Collects up to `max` collision objects and vegetation that may touch this
// one (their bounds overlap) into `results`.
void CollisionObject::SearchForCollisions(int max, int *count, SearchableObject **results)
{
    *count = 0;
    if (!m_searchable) {
        m_collided = 0;
        return;
    }
    m_tested = 1;
    _D3DVECTOR center;
    _D3DVECTOR half;
    GetCenterHalf(&center, &half);
    SpatialSearch search;
    search.OpenSearch(m_searchPlacement, 3, &center, &half, 0, 0);
    SearchableObject *obj;
    while ((obj = search.NextSearchObject()) && *count < max) {
        if (obj->m_typeID == CollisionTypeID) {
            m_testObject = (CollisionObject *)obj;
            m_testObject->m_tested = 1;
            if (TrivialCollisionObjectRejectionTest(this, m_testObject) == 0.0f) {
                if (ValidTestObject(m_testObject)) {
                    results[*count] = obj;
                    (*count)++;
                }
            }
        } else if (!m_ignoreVegetation && obj->m_typeID == VegetationTypeID) {
            Vegetation *veg = (Vegetation *)obj;
            _D3DVECTOR pos;
            pos.x = veg->m_x * LpCurrentEcoSystem->m_positionScale;
            pos.y = 0;
            pos.z = veg->m_z * LpCurrentEcoSystem->m_positionScale;
            if (Trivial2DCollisionObjectRejectionTest(this, &pos, veg->GetRadius()) == 0.0f) {
                for (int i = 0; i < veg->NumCollisionObjects(); i++) {
                    m_testObject = veg->GetCollisionObject(i);
                    m_testObject->m_tested = 1;
                    results[*count] = obj;
                    (*count)++;
                }
            }
        }
    }
}


// ---- src/CollisionObject/CheckForCollision_002239A0.cpp


int CollisionObject::CheckForCollision(int count, SearchableObject **objects)
{
    m_tested = 1;
    ResetCollisionData();
    int i;
    int j;
    for (i = 0; i < count; i++) {
        SearchableObject *object = objects[i];
        if (CollisionTypeID == object->m_typeID) {
            m_testObject = (CollisionObject *)object;
            m_testObject->m_tested = 1;
            if (TrivialCollisionObjectRejectionTest(this, m_testObject) == 0.0f) {
                if (ValidTestObject(m_testObject) && TestForCollision(m_testObject)) {
                    m_collided = 1;
                    GenerateCollisionPositionAndNormal();
                    m_testObject->m_collisionPosition = m_collisionPosition;
                    m_testObject->m_collisionNormal = m_collisionNormal;
                    if (m_callback)
                        m_callback(this, m_testObject);
                    if (m_testObject->m_otherCallback)
                        m_testObject->m_otherCallback(m_testObject, this);
                }
            }
        } else if (!m_ignoreVegetation && VegetationTypeID == object->m_typeID) {
            Vegetation *vegetation = (Vegetation *)object;
            _D3DVECTOR position;
            position.x = (unsigned int)vegetation->m_x * LpCurrentEcoSystem->m_positionScale;
            position.y = 0;
            position.z = (unsigned int)vegetation->m_z * LpCurrentEcoSystem->m_positionScale;
            if (Trivial2DCollisionObjectRejectionTest(this, &position, vegetation->GetRadius()) == 0.0f) {
                for (j = 0; j < vegetation->NumCollisionObjects(); j++) {
                    m_testObject = vegetation->GetCollisionObject(j);
                    m_testObject->m_tested = 1;
                    if (TestForCollision(m_testObject)) {
                        m_collided = 1;
                        GenerateCollisionPositionAndNormal();
                        m_testObject->m_collisionPosition = m_collisionPosition;
                        m_testObject->m_collisionNormal = m_collisionNormal;
                        if (m_callback)
                            m_callback(this, m_testObject);
                        if (m_testObject->m_otherCallback)
                            m_testObject->m_otherCallback(m_testObject, this);
                    }
                }
            }
        }
    }
    if (m_collided)
        FinishCollisionData();
    return m_collided;
}


// ---- src/CollisionObject/CheckForOverlap.cpp
// CheckForOverlap__15CollisionObjectFP15CollisionObject  @ 0x00223D80
// Retail bug kept: this object's own sphere/box is stored in otherSphere/otherBox (then overwritten by the other object's), so only dynamic rays ever set `sphere`.


// Whether two collision objects' shapes overlap: sphere against sphere, box
// against box, either against a soultree/hierarchy/ray box, or the two
// bounding-box trees. Dynamic ray sets count as their bounding sphere.
int CollisionObject::CheckForOverlap(CollisionObject *other)
{
    SphereCollisionObject raySphere;
    SphereCollisionObject otherRaySphere;
    SphereCollisionObject *sphere = 0;
    SphereCollisionObject *otherSphere = 0;
    BoxCollisionObject *box = 0;
    BoxCollisionObject *otherBox = 0;
    _D3DVECTOR *min;
    _D3DVECTOR *max;
    _D3DMATRIX *matrix;
    if (m_collisionType == COLLISION_SPHERE)
        otherSphere = (SphereCollisionObject *)m_collisionData;
    else if (m_collisionType == COLLISION_BOX)
        otherBox = (BoxCollisionObject *)m_collisionData;
    if (other->m_collisionType == COLLISION_SPHERE)
        otherSphere = (SphereCollisionObject *)other->m_collisionData;
    else if (other->m_collisionType == COLLISION_BOX)
        otherBox = (BoxCollisionObject *)other->m_collisionData;
    if (m_collisionType == COLLISION_DYNAMIC_RAYS) {
        raySphere.center = ((DynamicRayCollisionObject *)m_collisionData)->m_center;
        raySphere.radius = ((DynamicRayCollisionObject *)m_collisionData)->m_radius;
        raySphere.radiusSquared = raySphere.radius * raySphere.radius;
        raySphere.m_matrix = IdentityMatrix();
        sphere = &raySphere;
    }
    if (other->m_collisionType == COLLISION_DYNAMIC_RAYS) {
        otherRaySphere.center = ((DynamicRayCollisionObject *)other->m_collisionData)->m_center;
        otherRaySphere.radius = ((DynamicRayCollisionObject *)other->m_collisionData)->m_radius;
        otherRaySphere.radiusSquared = otherRaySphere.radius * otherRaySphere.radius;
        otherRaySphere.m_matrix = IdentityMatrix();
        otherSphere = &otherRaySphere;
    }
    if (sphere && otherSphere)
        return SphereVsSphereOverlapTest(sphere->center, sphere->radius, &sphere->m_matrix, otherSphere->center, otherSphere->radius, &otherSphere->m_matrix);
    if (box && otherBox) {
        _D3DMATRIX m;
        m = box->m_matrix;
        InvertMatrix(&m);
        MulMatrixCopy(&m, m, otherBox->m_matrix);
        return OBBOverlapTest(&box->center, &box->half, otherBox->center, otherBox->half, &m);
    }
    if (otherSphere)
        sphere = otherSphere;
    else
        sphere = sphere;
    if (otherBox)
        box = otherBox;
    if (sphere && box) {
        _D3DVECTOR center;
        center = sphere->m_scale * sphere->center;
        TransformPoint(&center, &sphere->m_matrix, &center);
        return OBBVsSphereOverlapTest(&box->center, &box->half, center, sphere->radius, sphere->radius * sphere->radius, &box->m_matrix);
    }
    if (sphere) {
        // One nested ?: (an if around the inner ?: leaves a `b` to an empty join).
        other = sphere != m_collisionData ? sphere == &raySphere ? other : this : other;
        switch (other->m_collisionType) {
        case COLLISION_SOULTREE: {
            SoultreeCollisionObject *soultree = (SoultreeCollisionObject *)other->m_collisionData;
            min = &soultree->m_faceTree->m_min;
            max = &soultree->m_faceTree->m_max;
            matrix = &soultree->m_xD0;
            break;
        }
        case COLLISION_SOULTREE_HIERARCHY: {
            SoultreeHierarchyCollisionObject *hierarchy = (SoultreeHierarchyCollisionObject *)other->m_collisionData;
            min = &hierarchy->m_center;
            max = &hierarchy->m_half;
            matrix = &hierarchy->m_matrix;
            break;
        }
        case COLLISION_RAYS: {
            RayCollisionObject *rays = (RayCollisionObject *)other->m_collisionData;
            min = &rays->m_tree->m_min;
            max = &rays->m_tree->m_max;
            matrix = &rays->m_matrix;
            break;
        }
        }
        _D3DVECTOR center;
        TransformPoint(&center, &sphere->m_matrix, &sphere->center);
        return OBBVsSphereOverlapTest(min, max, center, sphere->radius, sphere->radiusSquared, matrix);
    }
    if (box) {
        other = box == m_collisionData ? other : this;
        switch (other->m_collisionType) {
        case COLLISION_SOULTREE: {
            SoultreeCollisionObject *soultree = (SoultreeCollisionObject *)other->m_collisionData;
            min = &soultree->m_faceTree->m_min;
            max = &soultree->m_faceTree->m_max;
            matrix = &soultree->m_xD0;
            break;
        }
        case COLLISION_SOULTREE_HIERARCHY: {
            SoultreeHierarchyCollisionObject *hierarchy = (SoultreeHierarchyCollisionObject *)other->m_collisionData;
            min = &hierarchy->m_center;
            max = &hierarchy->m_half;
            matrix = &hierarchy->m_matrix;
            break;
        }
        case COLLISION_RAYS: {
            RayCollisionObject *rays = (RayCollisionObject *)other->m_collisionData;
            min = &rays->m_tree->m_min;
            max = &rays->m_tree->m_max;
            matrix = &rays->m_matrix;
            break;
        }
        }
        _D3DMATRIX m;
        m = box->m_matrix;
        InvertMatrix(&m);
        MulMatrixCopy(&m, m, *matrix);
        return OBBOverlapTest(&box->center, &box->half, *min, *max, &m);
    }
    BoundingBoxTree box1;
    BoundingBoxTree box2;
    BoundingBoxTree *tree;
    BoundingBoxTree *otherTree;
    _D3DMATRIX *otherMatrix;
    _D3DMATRIX *thisMatrix;
    switch (m_collisionType) {
    case COLLISION_SOULTREE: {
        SoultreeCollisionObject *soultree = (SoultreeCollisionObject *)m_collisionData;
        tree = soultree->m_faceTree;
        thisMatrix = &soultree->m_xD0;
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        tree = &box1;
        box1.m_min = ((SoultreeHierarchyCollisionObject *)m_collisionData)->m_center;
        box1.m_max = ((SoultreeHierarchyCollisionObject *)m_collisionData)->m_half;
        thisMatrix = &((SoultreeHierarchyCollisionObject *)m_collisionData)->m_matrix;
        break;
    }
    case COLLISION_RAYS: {
        RayCollisionObject *rays = (RayCollisionObject *)m_collisionData;
        tree = rays->m_tree;
        thisMatrix = &rays->m_matrix;
        break;
    }
    }
    switch (other->m_collisionType) {
    case COLLISION_SOULTREE: {
        SoultreeCollisionObject *soultree = (SoultreeCollisionObject *)other->m_collisionData;
        otherTree = soultree->m_faceTree;
        otherMatrix = &soultree->m_xD0;
        break;
    }
    case COLLISION_SOULTREE_HIERARCHY: {
        otherTree = &box2;
        box2.m_min = ((SoultreeHierarchyCollisionObject *)other->m_collisionData)->m_center;
        box2.m_max = ((SoultreeHierarchyCollisionObject *)other->m_collisionData)->m_half;
        otherMatrix = &((SoultreeHierarchyCollisionObject *)other->m_collisionData)->m_matrix;
        break;
    }
    case COLLISION_RAYS: {
        RayCollisionObject *rays = (RayCollisionObject *)other->m_collisionData;
        otherTree = rays->m_tree;
        otherMatrix = &rays->m_matrix;
        break;
    }
    }
    return TestBoundingBoxTreesForOverlap(tree, otherTree, thisMatrix, otherMatrix);
}


// ---- src/CollisionObject/Tick.cpp
extern RenderTarget *LpRenderTarget;


// v scaled by s; the float is evaluated before the vector (right-to-left args).
static inline _D3DVECTOR Scale(const _D3DVECTOR &v, float s) { return _D3DVECTOR(s * v.x, s * v.y, s * v.z); }

// TransformPoint with the parameters in (out, in, m) order: retail evaluates
// the arguments (right to left) in that order here.
static inline void TransformPointOIM(_D3DVECTOR *out, _D3DVECTOR *in, _D3DMATRIX *m)
{
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x10(%1); lqc2 vf6, 0x20(%1); lqc2 vf7, 0x30(%1); lqc2 vf8, 0x0(%2); vmulw.w vf9, vf0, vf0w; vmulax.xyz ACC, vf4, vf8x; vmadday.xyz ACC, vf5, vf8y; vmaddz.xyz vf9, vf6, vf8z; vadd.xyz vf9, vf9, vf7; sqc2 vf9, 0x0(%0)"
        : : "r"(out), "r"(m), "r"(in) : "memory");
}

// Debug-draw the collision data (the drawing itself is compiled out, leaving
// the transforms), then tick the GameObject.
int CollisionObject::Tick(float dt)
{
    _D3DVECTOR center;
    LpRenderTarget->SetRenderState((_D3DRENDERSTATETYPE)0x1B, 0, 0);
    LpRenderTarget->SetTextureState((_D3DTEXTUREBLEND)7, 0);
    LpRenderTarget->SetTextureStageState(0, (_D3DTEXTURESTAGESTATETYPE)1, 1);
    if (m_collisionData && m_debugMode) {
        switch (m_collisionType) {
        case COLLISION_SOULTREE:
            RenderSoultreeCollisionObject((SoultreeCollisionObject *)m_collisionData, m_debugIndex, m_debugMode);
            break;
        case COLLISION_SOULTREE_HIERARCHY:
            RenderSoultreeHierarchyCollisionObject((SoultreeHierarchyCollisionObject *)m_collisionData, m_debugIndex, m_debugMode);
            break;
        case COLLISION_RAYS:
            RenderRayTree(((RayCollisionObject *)m_collisionData)->m_tree, m_debugIndex, &((RayCollisionObject *)m_collisionData)->m_matrix);
            RenderRays(((RayCollisionObject *)m_collisionData)->m_tree, &((RayCollisionObject *)m_collisionData)->m_matrix);
            break;
        case COLLISION_DYNAMIC_RAYS: {
            DynamicRayCollisionObject *rays = (DynamicRayCollisionObject *)m_collisionData;
            for (int i = 0; i < rays->m_numRays; i++) {
                _D3DVECTOR start;
                _D3DVECTOR end;
                start = rays->m_starts[i];
                end = start + Scale(rays->m_directions[i], rays->m_lengths[i]);
            }
            break;
        }
        case COLLISION_SPHERE: {
            SphereCollisionObject *sphere = (SphereCollisionObject *)m_collisionData;
            center = _D3DVECTOR(sphere->m_scale * sphere->center.x, sphere->m_scale * sphere->center.y, sphere->m_scale * sphere->center.z);
            TransformPoint(&center, &((SphereCollisionObject *)m_collisionData)->m_matrix, &center);
            break;
        }
        case COLLISION_RADIUS_LINE_SEGMENT: {
            _D3DVECTOR pts[2];
            TransformPointOIM(&pts[0], &((RadiusLineSegmentCollisionObject *)m_collisionData)->start, &((RadiusLineSegmentCollisionObject *)m_collisionData)->m_matrix);
            TransformPointOIM(&pts[1], &((RadiusLineSegmentCollisionObject *)m_collisionData)->end, &((RadiusLineSegmentCollisionObject *)m_collisionData)->m_matrix);
            _D3DVECTOR direction = pts[1] - pts[0];
            _D3DVECTOR unit;
            Normalize(&unit, &direction);
            float tenth = 0.1f;         // a variable, so the multiplies keep retail's operand order
            for (int i = 0; i < 10; i++) {
                _D3DVECTOR point;
                point = pts[0] + (float)i * _D3DVECTOR(direction.x * tenth, direction.y * tenth, direction.z * tenth);
            }
            break;
        }
        case COLLISION_BOX:
            break;
        }
    }
    m_tested = 0;
    return GameObject::Tick(dt);
}


// ---- src/CollisionObject/SetTestListType.cpp
void CollisionObject::SetTestListType(CollisionObjectTestListType type)
{
    m_testListType = type;
}


// ---- src/CollisionObject/AddToTestList.cpp

void CollisionObject::AddToTestList(CollisionObject *object)
{
    int i;
    for (i = 0; i < m_numTestObjects; i++) {
        if (m_testList[i] == object)
            return;
    }
    for (i = 0; i < m_numTestObjects; i++) {
        if (m_testList[i] == 0) {
            m_testList[i] = object;
            return;
        }
    }
    m_testList = (CollisionObject **)realloc(m_testList, (m_numTestObjects + 1) * sizeof(CollisionObject *));
    m_testList[m_numTestObjects] = object;
    m_numTestObjects++;
}


// ---- src/CollisionObject/RemoveFromTestList.cpp

void CollisionObject::RemoveFromTestList(CollisionObject *object)
{
    for (int i = 0; i < m_numTestObjects; i++) {
        if (m_testList[i] == object) {
            m_testList[i] = 0;
            return;
        }
    }
}


// ---- src/CollisionObject/AddToObjectTypeList.cpp

void CollisionObject::AddToObjectTypeList(unsigned int type)
{
    int i;
    for (i = 0; i < m_numObjectTypes; i++) {
        if (type == m_objectTypes[i])
            return;
    }
    for (i = 0; i < m_numObjectTypes; i++) {
        if (m_objectTypes[i] == 0L) {   // 0L: retail compares as 64-bit (lwu)
            m_objectTypes[i] = type;
            return;
        }
    }
    m_objectTypes = (unsigned int *)realloc(m_objectTypes, (m_numObjectTypes + 1) * sizeof(unsigned int));
    m_objectTypes[m_numObjectTypes] = type;
    m_numObjectTypes++;
}


// ---- src/CollisionObject/ClearObjectTypeList.cpp

void CollisionObject::ClearObjectTypeList()
{
    if (m_objectTypes) {
        free(m_objectTypes);
        m_objectTypes = 0;
    }
    m_numObjectTypes = 0;
}


// ---- src/CollisionObject/SetOwner.cpp


void CollisionObject::SetOwner(void *owner, unsigned int ownerType)
{
    m_owner = owner;
    m_ownerType = ownerType;
}



// ---- src/global/TestSphereForCollision.cpp
// The hits as TestSphereForCollision reads them (SphereIntersectionData with
// plain 4-float normals and positions): indexing the shared struct's
// _D3DVECTOR arrays, a class with constructors, adds base and index the other
// way round, and copying a _D3DVECTOR interleaves its loads and stores.
struct SphereHitVector {
    float x, y, z, w;
};

struct SphereHitsView {
    int m_maxHits;                      // +0x00
    int m_count;                        // +0x04
    char pad08[0x20 - 0x08];
    _D3DVECTOR m_center;                // +0x20
    SphereHitVector m_normals[3];       // +0x30
    SphereHitVector m_positions[3];     // +0x60
    int m_transformed[3];               // +0x90
};

// out = rotation part of m applied to v.
static inline void Vu0RotateVector(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR *v)
{
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x10(%1); lqc2 vf6, 0x20(%1); lqc2 vf8, 0x0(%2); vmulax.xyz ACC, vf4, vf8x; vmadday.xyz ACC, vf5, vf8y; vmaddz.xyz vf9, vf6, vf8z; vadd.xyzw vf9, vf9, vf0; sqc2 vf9, 0x0(%0)"
        : : "r"(out), "r"(m), "r"(v) : "memory");
}


// Record the hit: the sphere center, and the new contacts moved into world
// space by the soultree's matrix.
static inline void RecordCollision(_D3DVECTOR *center, _D3DMATRIX *m)
{
    int i;
    SphereHitsView *data = (SphereHitsView *)CurrentCollisionData;
    data->m_center = *center;
    for (i = 0; i < data->m_count; i++) {
        if (data->m_transformed[i])
            continue;
        SphereHitVector normal = data->m_normals[i];
        Vu0RotateVector((_D3DVECTOR *)&data->m_normals[i], m, (_D3DVECTOR *)&normal);
        Vu0TransformPoint(m, (_D3DVECTOR *)&data->m_positions[i], (_D3DVECTOR *)&data->m_positions[i]);
        data->m_transformed[i] = 1;
    }
}

// Does the sphere hit the object's geometry?  On a hit the collision data is
// completed in world space.
int TestSphereForCollision(_D3DVECTOR *center, float radius, float radiusSquared, CollisionObject *object)
{
    SoultreeHierarchyCollisionObject *hierarchy;
    int i;
    SoultreeCollisionObject *soultree;
    if (!object->m_enabled)
        return 0;
    switch (object->m_collisionType) {
    case 0:
        soultree = (SoultreeCollisionObject *)object->m_collisionData;
        if (SphereIntersectsBoundingBoxTree(center, radius, radiusSquared, soultree->m_faceTree, &soultree->m_matrix, 1, soultree->m_vertices, 1)) {
            RecordCollision(center, &soultree->m_matrix);
            return 1;
        }
        return 0;
    case 1:
        hierarchy = (SoultreeHierarchyCollisionObject *)object->m_collisionData;
        if (OBBVsSphereOverlapTest(&hierarchy->m_center, &hierarchy->m_half, *center, radius, radiusSquared, &hierarchy->m_matrix)) {
            for (i = 0; i < hierarchy->m_count; i++) {
                soultree = &hierarchy->m_objects[i];
                if (SphereIntersectsBoundingBoxTree(center, radius, radiusSquared, soultree->m_faceTree, &soultree->m_matrix, 1, soultree->m_vertices, 1)) {
                    RecordCollision(center, &soultree->m_matrix);
                    return 1;
                }
            }
        }
        return 0;
    case 2:
    case 3:
        return 0;
    }
    return 0;
}

void SetCurrentCollisionData(void *data)
{
    CurrentCollisionData = (RayToFaceIntersectionData *)data;
}



// ---- src/CollisionObject/ReadCollisionData.cpp

// Reads an object's collision data and registers the shareable part with
// the file so later instances can reuse it. (No return statement: retail
// leaves $v0 as AttachObject/FRead left it.)
int CollisionObject::ReadCollisionData(SoultreeObject *object, long handle)
{
    ClearCollisionData();
    LpDataAccess->FRead(handle, &m_collisionType, 4, 1);
    switch (m_collisionType) {
    case 0: {
        SoultreeCollisionObject *data = new SoultreeCollisionObject;
        data->m_object = object;
        data->m_tree = 0;
        data->m_transform = IdentityMatrix();
        m_collisionData = data;
        ReadSoultreeCollisionData(handle, (SoultreeCollisionObject *)m_collisionData);
        data->m_instance = (SoultreeCollisionInstanceData *)malloc(sizeof(SoultreeCollisionInstanceData));
        data->m_instance->m_isHierarchy = 0;
        data->m_instance->m_faceTree = data->m_faceTree;
        data->m_instance->m_x0C = 0;
        data->m_instance->m_x04 = data->m_numVertices;
        data->m_instance->m_vertices = data->m_vertices;
        data->m_instance->m_matrix = data->m_x90;
        data->m_instance->m_x60 = data->m_x00;
        data->m_instance->m_refCount = 1;
        LpDataAccess->AttachObject(handle, data->m_instance);
        break;
    }
    case 1: {
        SoultreeHierarchyCollisionObject *data = new SoultreeHierarchyCollisionObject;
        data->m_object = object;
        m_collisionData = data;
        data->m_instanceData = (SoultreeHierarchyCollisionInstanceData *)malloc(sizeof(SoultreeHierarchyCollisionInstanceData));
        ReadSoultreeHierarchyCollisionData(object, handle, data);
        data->m_instanceData->m_x00 = 1;
        data->m_instanceData->m_count = data->m_count;
        data->m_instanceData->m_x0C = (int *)malloc(data->m_count * 4);
        data->m_instanceData->m_x10 = 0;
        data->m_instanceData->m_x08 = (int *)malloc(data->m_count * 4);
        data->m_instanceData->m_x14 = (int *)malloc(data->m_count * 4);
        data->m_instanceData->m_matrices = (_D3DMATRIX *)malloc(data->m_count * 64);
        data->m_instanceData->m_refCount = 1;
        for (int i = 0; i < data->m_count; i++) {
            data->m_instanceData->m_x0C[i] = (int)data->m_objects[i].m_faceTree;
            data->m_instanceData->m_x08[i] = data->m_objects[i].m_numVertices;
            data->m_instanceData->m_x14[i] = (int)data->m_objects[i].m_vertices;
            data->m_instanceData->m_matrices[i] = data->m_objects[i].m_x90;
        }
        data->m_instanceData->m_x20 = data->m_objects->m_x00;
        LpDataAccess->AttachObject(handle, data->m_instanceData);
        break;
    }
    }
}


// ---- src/CollisionObject/InstanceCollisionData.cpp

int CollisionObject::InstanceCollisionData(SoultreeObject *object, void *data)
{
    // Both instance-data layouts start with an "is hierarchy" flag.
    if (!*(int *)data) {
        m_collisionType = COLLISION_SOULTREE;
        return InstanceSoultreeCollisionData(object, (SoultreeCollisionInstanceData *)data);
    } else {
        m_collisionType = COLLISION_SOULTREE_HIERARCHY;
        return InstanceSoultreeHierarchyCollisionData(object, (SoultreeHierarchyCollisionInstanceData *)data);
    }
}


// ---- src/CollisionObject/InstanceSoultreeCollisionData.cpp

// Builds this object's collision data as an instance of shared Soultree data.
int CollisionObject::InstanceSoultreeCollisionData(SoultreeObject *object, SoultreeCollisionInstanceData *data)
{
    SoultreeCollisionObject *collision = new SoultreeCollisionObject;
    collision->m_object = object;
    collision->m_transform = IdentityMatrix();
    m_collisionData = collision;
    collision->m_faceTree = data->m_faceTree;
    collision->m_tree = 0;
    collision->m_numVertices = data->m_x04;
    collision->m_vertices = (_D3DVECTOR *)data->m_vertices;
    collision->m_x90 = data->m_matrix;
    collision->m_x00 = data->m_x60;
    collision->m_instance = data;
    data->m_refCount++;
    // No return statement: retail returns whatever is left in $v0.
}


// ---- src/CollisionObject/InstanceSoultreeHierarchyCollisionData.cpp

int CollisionObject::InstanceSoultreeHierarchyCollisionData(SoultreeObject *object,
                                                             SoultreeHierarchyCollisionInstanceData *instance)
{
    int count;

    SoultreeHierarchyCollisionObject *data = new SoultreeHierarchyCollisionObject;
    data->m_object = object;
    m_collisionData = data;
    SoultreeObject *root = object;
    while (root->m_parent)
        root = root->m_parent;
    SoultreeObject **objects = new SoultreeObject *[root->NumberOfObjectsInHierarchy()];
    count = 1;
    objects[0] = root;
    root->FillObjectList(&count, objects);
    data->m_count = instance->m_count;
    data->m_objects = new SoultreeCollisionObject[data->m_count];
    data->m_objects->m_tree = 0;
    data->m_x04 = new int[data->m_count];
    data->m_x08 = new int[data->m_count];
    data->m_x0C = instance->m_x20;
    for (int i = 0; i < data->m_count; i++) {
        data->m_objects[i].m_object = objects[instance->m_objectIndices[i]];
        data->m_objects[i].m_transform = IdentityMatrix();
        data->m_x04[i] = 1;
        data->m_objects[i].m_faceTree = (BoundingBoxTree *)PS2_ADDR(instance->m_x0C[i]);
        data->m_objects[i].m_tree = 0;
        data->m_objects[i].m_numVertices = instance->m_x08[i];
        data->m_objects[i].m_vertices = (_D3DVECTOR *)PS2_ADDR(instance->m_x14[i]);
        data->m_objects[i].m_x90 = instance->m_matrices[i];
        data->m_objects[i].m_x00 = instance->m_x20;
    }
    data->m_instanceData = instance;
    instance->m_refCount++;
    delete[] objects;
}


// ---- src/CollisionObject/ReadSoultreeCollisionData.cpp

void ReadFaceTree(BoundingBoxTree **tree, _D3DVECTOR **vertices, int *numVertices, long handle);


void CollisionObject::ReadSoultreeCollisionData(long handle, SoultreeCollisionObject *data)
{
    int hasTree;
    LpDataAccess->FRead(handle, &data->m_x90._11, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._12, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._13, 4, 1);
    data->m_x90._14 = 0.0f;
    LpDataAccess->FRead(handle, &data->m_x90._21, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._22, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._23, 4, 1);
    data->m_x90._24 = 0.0f;
    LpDataAccess->FRead(handle, &data->m_x90._31, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._32, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._33, 4, 1);
    data->m_x90._34 = 0.0f;
    LpDataAccess->FRead(handle, &data->m_x90._41, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._42, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x90._43, 4, 1);
    data->m_x90._44 = 1.0f;
    LpDataAccess->FRead(handle, &data->m_x00, 4, 1);
    LpDataAccess->FRead(handle, &hasTree, 4, 1);
    if (hasTree)
        ReadFaceTree(&data->m_faceTree, &data->m_vertices, &data->m_numVertices, handle);
}


// ---- src/CollisionObject/ReadSoultreeHierarchyCollisionData.cpp

void CollisionObject::ReadSoultreeHierarchyCollisionData(SoultreeObject *object, long handle,
                                                         SoultreeHierarchyCollisionObject *data)
{
    int count;
    int index;

    SoultreeObject *root = object;
    while (root->m_parent)
        root = root->m_parent;
    SoultreeObject **objects = new SoultreeObject *[root->NumberOfObjectsInHierarchy()];
    count = 1;
    objects[0] = root;
    root->FillObjectList(&count, objects);
    LpDataAccess->FRead(handle, &data->m_count, 4, 1);
    LpDataAccess->FRead(handle, &data->m_x0C, 4, 1);
    data->m_objects = new SoultreeCollisionObject[data->m_count];
    data->m_objects->m_tree = 0;
    data->m_x04 = new int[data->m_count];
    data->m_x08 = new int[data->m_count];
    data->m_instanceData->m_objectIndices = (int *)malloc(data->m_count * 4);
    for (int i = 0; i < data->m_count; i++) {
        LpDataAccess->FRead(handle, &index, 4, 1);
        data->m_objects[i].m_object = objects[index];
        data->m_objects[i].m_transform = IdentityMatrix();
        ReadSoultreeCollisionData(handle, &data->m_objects[i]);
        data->m_objects[i].m_instance = 0;
        data->m_x04[i] = 1;
        data->m_instanceData->m_objectIndices[i] = index;
    }
    delete[] objects;
}


// ---- src/global/Trivial2DCollisionObjectRejectionTest_audit.cpp
// Distance in the XZ plane by which the two bounding circles are apart
// (0 if they overlap).
float Trivial2DCollisionObjectRejectionTest(CollisionObject *obj, _D3DVECTOR *point, float radius)
{
float r = obj->m_searchRadius + radius; float dx = obj->m_center.x - point->x; float dz = obj->m_center.z - point->z; float d = Sqrt(dx * dx + dz * dz) - r; if (d > 0.0f) return d; return 0.0f;
}


// ---- src/global/TrivialCollisionObjectRejectionTest_audit.cpp
// Distance by which the two bounding spheres are apart (0 if they overlap).
float TrivialCollisionObjectRejectionTest(CollisionObject *a, CollisionObject *b)
{
float r = a->m_searchRadius + b->m_searchRadius; float dx = a->m_center.x - b->m_center.x; float dy = a->m_center.y - b->m_center.y; float dz = a->m_center.z - b->m_center.z; float d = Sqrt(dx * dx + dy * dy + dz * dz) - r; if (d > 0.0f) return d; return 0.0f;
}


// ---- src/global/TestRayBundleForCollision.cpp

// Moves the ray bundle into the space of each soultree of the object, tests it
// against the soultree's face tree and moves the hits back to world space.
int TestRayBundleForCollision(DynamicRayCollisionObject *rays_, CollisionObject *other_, RayToFaceIntersectionData *data_)
{
    // Named copies of the parameters, declared in this order, give retail's
    // registers.
    DynamicRayCollisionObject *rays;
    int hit;
    int j;
    int count;
    int n;
    int i;
    float *t;
    SoultreeCollisionObject *soultree;
    SoultreeHierarchyCollisionObject *hierarchy;
    RayToFaceIntersectionData *data;
    CollisionObject *other;
    rays = rays_;
    other = other_;
    data = data_;
    _D3DVECTOR unused1, delta;          // unused1-3 are constructed, never used
    _D3DMATRIX inverse;
    _D3DVECTOR unused2, unused3;
    hit = 0;
    memset(data->m_rayHit, 0, rays->m_numRays * 4);
    if (!other->m_enabled)
        return hit;
    n = rays->m_numRays;
    delta = rays->m_center - other->m_center;
    float radius = other->m_searchRadius + rays->m_radius;
    radius *= radius;
    if (Vu0LengthSquared(&delta) < radius) {
        data->m_t = 1.0f;
        t = data->m_rayT;
        for (i = 0; i < n; i++)
            *t++ = -1.0f;
        data->m_numRays = 0;
        switch (other->m_collisionType) {
        case COLLISION_SOULTREE: {
            soultree = (SoultreeCollisionObject *)other->m_collisionData;
            inverse = soultree->m_matrix;
            Vu0InversMatrix(&inverse);
            _D3DVECTOR *localDir = rays->m_localDirections;
            _D3DVECTOR *dir = rays->m_directions;
            _D3DVECTOR *localStart = rays->m_localStarts;
            _D3DVECTOR *start = rays->m_starts;
            for (i = 0; i < n; i++) {
                Vu0ApplyMatrixVector(localDir, &inverse, *dir);
                Vu0TransformPointOut(localStart, &inverse, start);
                localDir++;
                dir++;
                localStart++;
                start++;
            }
            Vu0TransformPointOutIn(&rays->m_localCenter, &inverse, &rays->m_center);
            RayBundleIntersectsBoundingBoxTree(rays, soultree->m_faceTree, soultree->m_vertices, data);
            int *rayHit = data->m_rayHit;
            _D3DVECTOR *point = data->m_rayPoints;
            _D3DVECTOR *normal = data->m_rayNormals;
            for (i = 0; i < n; i++) {
                if (*rayHit) {
                    hit = 1;
                    Vu0TransformPointOut(point, &soultree->m_matrix, point);
                    Vu0ApplyMatrixVector(normal, &soultree->m_matrix, *normal);
                }
                rayHit++;
                point++;
                normal++;
            }
            if (hit) {
                Vu0TransformPointOut(&data->m_point, &soultree->m_matrix, &data->m_point);
                Vu0ApplyMatrixVector(&data->m_normal, &soultree->m_matrix, data->m_normal);
                Vu0ApplyMatrixVector(&data->m_penetration, &soultree->m_matrix, data->m_penetration);
            }
            return hit;
        }
        case COLLISION_SOULTREE_HIERARCHY: {
            hierarchy = (SoultreeHierarchyCollisionObject *)other->m_collisionData;
            count = hierarchy->m_count;
            soultree = hierarchy->m_objects;
            for (j = 0; j < count; j++) {
                inverse = soultree->m_matrix;
                Vu0InversMatrix(&inverse);
                _D3DVECTOR *localDir = rays->m_localDirections;
                _D3DVECTOR *dir = rays->m_directions;
                _D3DVECTOR *localStart = rays->m_localStarts;
                _D3DVECTOR *start = rays->m_starts;
                for (i = 0; i < n; i++) {
                    Vu0ApplyMatrixVector(localDir, &inverse, *dir);
                    Vu0TransformPointOut(localStart, &inverse, start);
                    localDir++;
                    dir++;
                    localStart++;
                    start++;
                }
                RayBundleIntersectsBoundingBoxTree(rays, soultree->m_faceTree, soultree->m_vertices, data);
                int *rayHit = data->m_rayHit;
                _D3DVECTOR *point = data->m_rayPoints;
                _D3DVECTOR *normal = data->m_rayNormals;
                for (i = 0; i < n; i++) {
                    if (*rayHit) {
                        hit = 1;
                        Vu0TransformPointOut(point, &soultree->m_matrix, point);
                        Vu0ApplyMatrixVector(normal, &soultree->m_matrix, *normal);
                    }
                    rayHit++;
                    point++;
                    normal++;
                }
                if (hit) {
                    Vu0TransformPointOut(&data->m_point, &soultree->m_matrix, &data->m_point);
                    Vu0ApplyMatrixVector(&data->m_normal, &soultree->m_matrix, data->m_normal);
                    Vu0ApplyMatrixVector(&data->m_penetration, &soultree->m_matrix, data->m_penetration);
                }
                soultree++;
            }
            return hit;
        }
        case COLLISION_RAYS:
        case COLLISION_DYNAMIC_RAYS:
            return hit;
        }
    }
    return 0;
}


// ---- src/sinit/CollisionObject.cpp
// Static initializer of CollisionObject.cpp: the axis vectors, the world
// object type registry and the world object type IDs (defined at the top).
