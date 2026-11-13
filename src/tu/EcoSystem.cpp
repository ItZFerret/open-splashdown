// EcoSystem.cpp  @ 0x0022DC70 (text 0x22DC70-0x235030)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's EcoSystem.cpp as one translation unit: its functions in retail
// order, then its static initializer (only the axis vectors).
// Create3DVegetationTransformRenderItem opens it (the vegetation VIF items:
// Create2DTransformRenderItem and Create2DTweakItem follow); its .rodata
// runs from @1482 ("Method") to the EcoSystem RTTI. Fog.cpp (no initializer)
// follows at 0x235030.



#include "sceGifTag/sceGifTag.h"
#include "VegetationDescriptor/VegetationDescriptor.h"
#include "EcoSystem/EcoSystem.h"
#include "PSXVideoCard/PSXVideoCard.h"
#include "d3dtypes.h"
#include "PSXRenderTarget/PSXRenderTarget.h"
#include "PSXCamera/PSXCamera.h"
#include "LightManager/LightManager.h"
#include "LightEmitter/LightEmitter.h"
#include "NormalDistribution/NormalDistribution.h"
#include "DmaRenderItem/DmaRenderItem.h"
#include "Vegetation/Vegetation.h"
#include "PSXSurfaceShaderInstance/PSXSurfaceShaderInstance.h"
#include "PSXSurfaceGeometry/PSXSurfaceGeometry.h"
#include "RenderableObjectData/RenderableObjectData.h"
#include "CollisionObject/CollisionObject.h"
#include "VifDataItem/VifDataItem.h"
#include "GameObject/GameObject.h"
#include "soultree_assert_block.h"
#include "SearchableObject/SearchableObject.h"
#include "ParameterBlock/ParameterBlock.h"
#include "DataAccess/DataAccess.h"
#include "TypeRegistry/TypeRegistry.h"
#include "QuadTree/QuadTree.h"
#include "Terrain/Terrain.h"
#include "TgaFile/TgaFile.h"
#include "Game/Game.h"
#include "Camera/Camera.h"
#include "RenderTarget/RenderTarget.h"
#include "VisibilityQuadTree/VisibilityQuadTree.h"
#include "VisibleTest/VisibleTest.h"
#include "StringTable/StringTable.h"
#include "SurfaceShaderList/SurfaceShaderList.h"
#include "SurfaceMaterial/SurfaceMaterial.h"
#include "ShaderDescriptor/ShaderDescriptor.h"
#include "TextureMap/TextureMap.h"
#include "SurfaceGeometry/SurfaceGeometry.h"
#include "DebugOverlay/DebugOverlay.h"
#include "sinit/AxisVectors.h"

extern "C" void *memalign(unsigned int alignment, unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);
extern "C" void *malloc(unsigned int size);
extern "C" void *realloc(void *p, unsigned int size);
extern "C" void free(void *p);
typedef int int128_t __attribute__((mode(TI)));

extern VisibilityQuadTree *LpVisibilityQuadTree;

// EcoSystem.cpp's globals, in retail's .data/.bss order.
_D3DMATRIX *LpViewMatrix;
int TotalRendered3D;
int TotalRendered2D;
EcoSystem *LpCurrentEcoSystem;
int EcoSystemTextureCount;
int ScaledDistance2D;
int ScaledDistance3D;
int ScaledDistance3DSquared;
int ScaledDistance2DSquared;
int ScaledCameraPositionX;
int ScaledCameraPositionZ;
float AlphaScalar;
float trigScalarOverHyp;
static int OverlayId = -1;
int RenderEcoSystem = 1;
int Render3DEcoSystem = 1;
int Render2DEcoSystem = 1;
int Render2D3DEcoSystem = 1;

// ---- src/global/Create3DVegetationTransformRenderItem.cpp

// The VifDataItem this function builds.
struct VegetationVifDataItem {
    short m_numQwords;          // +0x00
    unsigned char m_02;         // +0x02
    unsigned char m_03;         // +0x03
    PS2_PTR(void) m_data;               // +0x04
    int m_08;                   // +0x08
    int m_0C;                   // +0x0C
    unsigned int m_code[(0xB0 - 0x10) / 4];    // +0x10, VIF codes and unpacked data
    sceGifTag m_gifTag;         // +0xB0
    long m_test;                // +0xC0
    long m_testReg;             // +0xC8
    unsigned int m_D0[4];       // +0xD0
    int128_t m_light;           // +0xE0
    unsigned int m_F0[4];       // +0xF0
};

#define CODE(offset) item->m_code[((offset) - 0x10) / 4]

VifDataItem *Create3DVegetationTransformRenderItem()
{
    VegetationVifDataItem *item = (VegetationVifDataItem *)memalign(0x40, 0x100);
    memset(item, 0, 0x100);
    CODE(0x10) = 0x11000000;
    CODE(0x14) = 0x03000000;
    CODE(0x18) = 0x02000000;
    CODE(0x1C) = 0x6C018029;
    CODE(0x2C) = 0;
    CODE(0x3C) = 0x6C048000;
    CODE(0x8C) = 0x6C01802A;
    CODE(0x90) = 0x38000100;
    CODE(0x94) = 0x3C010204;
    CODE(0x98) = 0x3A001002;
    CODE(0xAC) = 0x6C028320;
    *(int128_t *)&item->m_gifTag = 0;
    item->m_gifTag.FLG = 0;
    item->m_gifTag.EOP = 1;
    item->m_gifTag.NREG = 1;
    item->m_gifTag.NLOOP = 1;
    item->m_gifTag.REGS0 = 0xE;     // A+D
    item->m_test = 0x5800D;
    item->m_testReg = 0x47;         // TEST_1
    item->m_D0[0] = 0x14000000;
    item->m_D0[3] = 0x6C01801B;
    item->m_light = LpLightManager->m_ambientValue;
    item->m_F0[0] = 0x14000019;
    item->m_F0[1] = 0x10000000;
    item->m_F0[2] = 0x030000B5;
    item->m_F0[3] = 0x02000190;
    item->m_numQwords = 0xF;
    item->m_02 = 0;
    item->m_03 = 0x30;
    item->m_data = item->m_code;
    item->m_08 = 0;
    item->m_0C = 0;
    return (VifDataItem *)item;
}


// ---- src/EcoSystem/Create2DTransformRenderItem.cpp
extern "C" void *memalign(unsigned int alignment, unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);


typedef int int128_t __attribute__((mode(TI)));


extern RenderTarget *LpRenderTarget;

extern EcoSystem *LpCurrentEcoSystem;

struct Eco2DTransformItem {
    short m_numQwords;          // +0x00
    unsigned char m_02;         // +0x02
    unsigned char m_03;         // +0x03
    PS2_PTR(void) m_data;               // +0x04
    int m_08;                   // +0x08
    int m_0C;                   // +0x0C
    unsigned int m_code[(0x60 - 0x10) / 4];    // +0x10
    sceGifTag m_gifTag;         // +0x60
    float m_70[(0xE0 - 0x70) / 4];             // +0x70
    unsigned int m_unpack[4];   // +0xE0
};

// Builds the VIF packet for the 2D (billboard) vegetation: a GIFtag, constant
// vectors, then three quadwords of per-descriptor parameters for each of the
// first 16 vegetation descriptors.
void *EcoSystem::Create2DTransformRenderItem()
{
    int size;
    int count;
    int i;
    int rows;
    Eco2DTransformItem *item;
    float *p;
    count = 0;
    for (i = 0; i < 16; i++) {
        if (m_descriptors[i])
            count++;
    }
    rows = count * 3;
    size = (rows + 15) * 16 + 16;
    item = (Eco2DTransformItem *)memalign(0x40, size);
    memset(item, 0, size);
    item->m_code[0] = 0x11000000;
    item->m_code[1] = 0x03000000;
    item->m_code[2] = 0x02000000;
    item->m_code[3] = 0x6C0C802E;
    p = (float *)item->m_code;
    *(int128_t *)&item->m_gifTag = 0;
    item->m_gifTag.EOP = 1;
    item->m_gifTag.PRE = 1;
    item->m_gifTag.PRIM = 0x7D;
    item->m_gifTag.FLG = 0;
    item->m_gifTag.NREG = 3;
    item->m_gifTag.REGS0 = 2;
    item->m_gifTag.REGS1 = 1;
    item->m_gifTag.REGS2 = 4;
    item->m_gifTag.NLOOP = 6;
    item->m_70[0] = 1.0f;
    item->m_70[1] = 1.0f;
    item->m_70[2] = 1.0f;
    item->m_70[3] = 0.0f;
    item->m_70[(0x8C - 0x70) / 4] = LpCurrentEcoSystem->m_positionScale;
    item->m_70[(0xD0 - 0x70) / 4] = 2.0e-5f;
    item->m_70[(0xD4 - 0x70) / 4] = -16.0f;
    item->m_70[(0xD8 - 0x70) / 4] = ((PSXRenderTarget *)LpRenderTarget)->m_fogEnd;
    item->m_70[(0xDC - 0x70) / 4] = ((PSXRenderTarget *)LpRenderTarget)->m_fogScale;
    item->m_unpack[0] = 0x11000000;
    item->m_unpack[1] = 0x03000000;
    item->m_unpack[2] = 0x02000000;
    item->m_unpack[3] = 0x6C008300 | rows << 16;
    p += 0xE0 / 4;
    for (i = 0; i < 16; i++) {
        if (m_descriptors[i]) {
            p[0] = m_descriptors[i]->m_uLeft;
            p[1] = m_descriptors[i]->m_uCenter;
            p[2] = m_descriptors[i]->m_uRight;
            p[3] = m_descriptors[i]->m_x1D4;
            p[4] = m_descriptors[i]->m_vTop;
            p[5] = m_descriptors[i]->m_vBottom;
            p[6] = 0;
            p[7] = 0;
            p[8] = m_descriptors[i]->m_meanRadius;
            p[9] = (m_descriptors[i]->m_maxRadius - m_descriptors[i]->m_meanRadius) / 128.0f;
            p[10] = m_descriptors[i]->m_meanHeight;
            p[11] = (m_descriptors[i]->m_meanHeight - m_descriptors[i]->m_minHeight) / 128.0f;
            p += 12;
        }
    }
    unsigned int *end = (unsigned int *)p;
    end[0] = 0x140000D7;
    end[1] = 0x10000000;
    end[2] = 0x030000B5;
    end[3] = 0x02000124;
    item->m_numQwords = (unsigned int)(size - 16) >> 4;
    item->m_02 = 0;
    item->m_03 = 0x30;
    item->m_data = item->m_code;
    item->m_08 = 0;
    item->m_0C = 0;
    return item;
}


// ---- src/global/Create2DTweakItem.cpp
extern "C" void *memalign(unsigned int alignment, unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);

typedef int int128_t __attribute__((mode(TI)));


extern PSXVideoCard *LpVideoCard;

struct TweakPacket {
    unsigned int m_vif[4];      // +0x00
    sceGifTag m_gifTag;         // +0x10
    long m_zbuf;                // +0x20
    long m_zbufReg;             // +0x28
    long m_test;                // +0x30
    long m_testReg;             // +0x38
    unsigned int m_mscal;       // +0x40
};

struct Eco2DTweakItem {
    short m_numQwords;          // +0x00
    unsigned char m_02;         // +0x02
    unsigned char m_03;         // +0x03
    PS2_PTR(TweakPacket) m_data;        // +0x04
    int m_08;                   // +0x08
    int m_0C;                   // +0x0C
    TweakPacket m_packet;       // +0x10
};

// A VIF packet that unpacks a GIFtag plus A+D writes of ZBUF (masked) and
// TEST into VU1 memory.
VifDataItem *Create2DTweakItem()
{
    Eco2DTweakItem *item = (Eco2DTweakItem *)memalign(0x40, 0x60);
    memset(item, 0, 0x60);
    item->m_packet.m_vif[0] = 0x11000000;   // FLUSH
    item->m_packet.m_vif[1] = 0x03000000;   // BASE
    item->m_packet.m_vif[2] = 0x02000000;   // OFFSET
    item->m_packet.m_vif[3] = 0x6C028320;   // UNPACK
    *(int128_t *)&item->m_packet.m_gifTag = 0;
    item->m_packet.m_gifTag.FLG = 0;
    item->m_packet.m_gifTag.EOP = 1;
    item->m_packet.m_gifTag.NREG = 2;
    item->m_packet.m_gifTag.NLOOP = 1;
    item->m_packet.m_gifTag.REGS0 = 0xE;    // A+D
    item->m_packet.m_gifTag.REGS1 = 0xE;    // A+D
    TweakPacket *packet = &item->m_packet;
    packet->m_zbuf = (long)LpVideoCard->GetZBufferAddress() | ((long)LpVideoCard->GetZBufferFormat() << 24) | (1L << 32);
    packet->m_zbufReg = 0x4E;               // ZBUF_1
    packet->m_test = 0x5801D;
    packet->m_testReg = 0x47;               // TEST_1
    packet->m_mscal = 0x14000000;           // MSCAL
    item->m_numQwords = 5;
    item->m_02 = 0;
    item->m_03 = 0x30;
    item->m_data = &item->m_packet;
    item->m_08 = 0;
    item->m_0C = 0;
    return (VifDataItem *)item;
}


// ---- src/EcoSystem/Calculate2DParameters.cpp



extern RenderTarget *LpRenderTarget;
extern LightManager *LpLightManager;

typedef int int128_t __attribute__((mode(TI)));

// A colour quadword, copied with lq/sq.
union Colour {
    int128_t q;
    float c[4];                         // r, g, b, a
};

// d3dtypes.h has no operator-=.
static inline void operator-=(_D3DVECTOR &a, const _D3DVECTOR &b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
}

static inline float RSqrt(float x)
{
    float r;
    asm("rsqrt.s $f8, %0, %1; swc1 $f8, 0(%2)" : : "f"(1.0f), "f"(x), "r"(&r) : "$f8", "memory");
    return r;
}

static inline void Normalize(_D3DVECTOR &v)
{
    float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len2 == 0.0f)
        v = ZeroVector;
    else
        v *= RSqrt(len2);
}

// xyz dot product on VU0.
static inline float DotProduct(_D3DVECTOR *a, _D3DVECTOR *b)
{
    float r;
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x0(%2); vmul.xyz vf5, vf4, vf5; vaddy.x vf5, vf5, vf5y; vaddz.x vf5, vf5, vf5z; qmfc2.ni $2, vf5; mtc1 $2, %0"
        : "=f"(r) : "r"(a), "r"(b) : "$2");
    return r;
}

// One sprite-corner light colour: ambient plus the sun's diffuse by how much
// the corner faces it, clamped to 127; w is 0x40.
static inline float *LightCorner(float *p, _D3DVECTOR *corner, _D3DVECTOR *sunDir, Colour &ambient, Colour &sun)
{
    float d = -DotProduct(corner, sunDir);
    if (d < 0.0f) {
        *(int128_t *)p = ambient.q;
    } else {
        float r = ambient.c[0] + d * sun.c[0];
        float g = ambient.c[1] + d * sun.c[1];
        float b = ambient.c[2] + d * sun.c[2];
        if (r > 127.0f)
            r = 127.0f;
        p[0] = r;
        if (g > 127.0f)
            g = 127.0f;
        p[1] = g;
        if (b > 127.0f)
            b = 127.0f;
        p[2] = b;
    }
    ((int *)p)[3] = 0x40;
    return p + 4;
}

// Fill the 2D vegetation VIF item's per-frame constants: the camera's
// matrix and facing, the three corner light colours, the camera position
// and the fog range.
void EcoSystem::Calculate2DParameters()
{
    char *item = (char *)m_2DItem;
    float *p = (float *)(item + 0x20);
    PSXCamera *camera = (PSXCamera *)LpRenderTarget->m_camera;
    // 64-byte copy through $at, $v0, $v1, $a0 (hand-written).
    asm("lq $1, 0x0(%1); lq $2, 0x10(%1); lq $3, 0x20(%1); lq $4, 0x30(%1); sq $1, 0x0(%0); sq $2, 0x10(%0); sq $3, 0x20(%0); sq $4, 0x30(%0)"
        : : "r"(item + 0x20), "r"(&camera->m_x250) : "$1", "$2", "$3", "$4", "memory");
    p += 0x18;
    float s = RSqrt(camera->m_lookAt.x * camera->m_lookAt.x + camera->m_lookAt.z * camera->m_lookAt.z);
    p[0] = camera->m_lookAt.z * s;
    p[2] = camera->m_lookAt.x * s;
    _D3DVECTOR left, right, back;
    left.x = -camera->m_lookAt.z;
    left.y = 0.0f;
    left.z = camera->m_lookAt.x;
    right.x = camera->m_lookAt.z;
    right.y = 0.0f;
    right.z = -camera->m_lookAt.x;
    back.x = -camera->m_lookAt.x;
    back.y = 0.0f;
    back.z = -camera->m_lookAt.z;
    _D3DVECTOR sunDir;
    left -= 0.1f * back;
    right -= 0.1f * back;
    sunDir.x = LpLightManager->m_sun->m_dir[0];
    sunDir.y = 0.0f;
    sunDir.z = LpLightManager->m_sun->m_dir[2];
    p += 4;
    Normalize(sunDir);
    Normalize(left);
    Normalize(right);
    Normalize(back);
    Colour ambient;
    ambient.q = LpLightManager->m_ambientValue;
    Colour sun;
    sun.c[0] = LpLightManager->m_sun->m_diffuse.r;
    sun.c[1] = LpLightManager->m_sun->m_diffuse.g;
    sun.c[2] = LpLightManager->m_sun->m_diffuse.b;
    p = LightCorner(p, &left, &sunDir, ambient, sun);
    p = LightCorner(p, &back, &sunDir, ambient, sun);
    p = LightCorner(p, &right, &sunDir, ambient, sun);
    p[0] = LpRenderTarget->m_camera->m_position.x;
    p[1] = LpRenderTarget->m_camera->m_position.y;
    p[2] = LpRenderTarget->m_camera->m_position.z;
    p += 4;
    p[2] = ((PSXRenderTarget *)LpRenderTarget)->m_fogEnd;
    p[3] = ((PSXRenderTarget *)LpRenderTarget)->m_fogScale;
}


// ---- src/VegetationDescriptor/ctor.cpp
#pragma fast_fptosi on  // retail never calls fptosi: float->int is an inline cvt.w.s

VegetationDescriptor::VegetationDescriptor()
{
    m_name[0] = 0;
    m_billboard[0] = 0;
    m_probabilityTga[0] = 0;
    m_meanHeight = 0.0f;
    m_minHeight = 0.0f;
    m_maxHeight = 0.0f;
    m_meanRadius = 0.0f;
    m_minRadius = 0.0f;
    m_maxRadius = 0.0f;
    m_meanSlope = 0.0f;
    m_sdSlope = 0.0f;
    m_meanAspect = 0.0f;
    m_sdAspect = 0.0f;
    m_meanDrainage = 0.0f;
    m_sdDrainage = 0.0f;
    m_meanAltitude = 0.0f;
    m_sdAltitude = 0.0f;
    m_uLeft = 0.0f;
    m_uRight = 1.0f;
    m_vTop = 1.0f;
    m_vBottom = 0.0f;
    m_uCenter = 0.5f;
    m_numCollisionObjects = 0;
    m_collision = 0;
    m_collisionObjects = 0;
    m_object = 0;
}
#pragma fast_fptosi reset


// ---- src/VegetationDescriptor/dtor.cpp
#pragma fast_fptosi on  // retail never calls fptosi: float->int is an inline cvt.w.s

VegetationDescriptor::~VegetationDescriptor()
{
    if (m_object)
        m_object->Release();
    if (m_collisionObjects)
        free(m_collisionObjects);
    if (m_collision)
        free(m_collision);
    EcoSystemTextureCount = 0;
}
#pragma fast_fptosi reset


// ---- src/VegetationDescriptor/GenerateHeight.cpp

char VegetationDescriptor::GenerateHeight()
{
    return (rand() >> 7) - 0x7F;
}


// ---- src/VegetationDescriptor/GenerateRadius.cpp
#pragma fast_fptosi on  // retail never calls fptosi: float->int is an inline cvt.w.s

int VegetationDescriptor::GenerateRadius(float scale)
{
    return (int)((scale * (float)((rand() >> 7) - 127)) / m_maxHeight);
}
#pragma fast_fptosi reset


// ---- src/VegetationDescriptor/GetHeight.cpp
#pragma fast_fptosi on  // retail never calls fptosi: float->int is an inline cvt.w.s

float VegetationDescriptor::GetHeight(char code)
{
    return m_meanHeight + ((float)(int)code * (m_meanHeight - m_minHeight)) / 128.0f;
}
#pragma fast_fptosi reset


// ---- src/VegetationDescriptor/GetRadius.cpp
#pragma fast_fptosi on  // retail never calls fptosi: float->int is an inline cvt.w.s

float VegetationDescriptor::GetRadius(char code)
{
    return m_meanRadius + ((float)(int)code * (m_maxRadius - m_meanRadius)) / 128.0f;
}
#pragma fast_fptosi reset


// ---- src/VegetationDescriptor/Load.cpp

extern "C" char *strcpy(char *dst, const char *src);
extern "C" unsigned int strlen(const char *s);
extern "C" void *memset(void *dst, int c, unsigned int n);

extern char SltGeometryContentDirectory[];
extern char SltTextureContentDirectory[];
extern char EcoGeometryContentDirectory[];
extern char EcoTextureContentDirectory[];



struct RenderableModel {
    char pad0[0x18];
    int m_numTextures;                  // +0x18
};


extern EcoSystem *LpCurrentEcoSystem;


// An inlined strrchr(name, '.').
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

// Loads the descriptor's model (and the shared billboard geometry, once)
// from the ecosystem content directories.
int VegetationDescriptor::Load(int index)
{
    char geometryDirectory[0x110];
    char textureDirectory[0x110];
    char name[0x80];
    strcpy(geometryDirectory, SltGeometryContentDirectory);
    strcpy(SltGeometryContentDirectory, EcoGeometryContentDirectory);
    strcpy(textureDirectory, SltTextureContentDirectory);
    strcpy(SltTextureContentDirectory, EcoTextureContentDirectory);
    if (m_billboard[0] && !LpCurrentEcoSystem->m_2DGeometry) {
        strcpy(name, m_billboard);
        char *extension = FindExtension(name);
        if (extension)
            memset(extension, 0, strlen(extension));
        LpCurrentEcoSystem->m_2DGeometry = new PSXSurfaceGeometry;
        LpCurrentEcoSystem->m_2DGeometry->Create(name, 1);
        LpCurrentEcoSystem->m_2DGeometry->m_renderItem = new DmaRenderItem;
        LpCurrentEcoSystem->m_2D3DGeometry = new PSXSurfaceGeometry;
        LpCurrentEcoSystem->m_2D3DGeometry->Create(name, 1);
        LpCurrentEcoSystem->m_2D3DGeometry->m_renderItem = new DmaRenderItem;
    }
    if (m_name[0]) {
        m_object = new PSXRenderableObject(0);
        m_object->Create(m_name, 0, 0);
        m_lods = m_object->m_data->m_lods;
        m_heightScale = 0.5f / m_object->m_boundingBoxMax.y;
        m_radiusScale = 1.0f / m_object->m_boundingBoxMax.x;
        m_x1D4 = m_heightScale * (m_object->m_boundingBoxMin.y - m_object->m_boundingBoxMax.y);
        m_x1D8 = 1;
    }
    strcpy(SltGeometryContentDirectory, geometryDirectory);
    strcpy(SltTextureContentDirectory, textureDirectory);
    return 1;
}


// ---- src/Vegetation/ctor.cpp

Vegetation::Vegetation()
{
    m_x = 0;
    m_y = 0;
    m_z = 0;
    m_height = 0;
    m_radius = 0;
    m_type = 0;
    m_x14 = 0;
    m_typeID = LpCurrentEcoSystem->m_typeID;
}


// ---- src/Vegetation/Create.cpp

// A grid position as the binary ecosystem file stores it.
struct WordVector {
    unsigned short x, y, z;
};

void Vegetation::Create(int type, WordVector *position, char height, char radius)
{
    m_type = type;
    m_x = position->x;
    m_y = position->y;
    m_z = position->z;
    m_radius = radius;
    m_height = height;
    m_is2D = 1;
}


// ---- src/Vegetation/Create_0022F0B0.cpp
// Compiler session state: retail compiled
// this file in an mwcc session that had already generated a two-argument runtime
// helper call (64-bit / or %, double arithmetic or compare). mwcc then models
// every later helper call (__floatdisf, fptoui, fptodp, ...) as reading $a0-$a1,
// so temporaries live before such a call skip $a0/$a1. This unused function
// recreates that state; it is not part of the retail file.
static void MWCC_SHIM_TEXT MwccSessionState(long *a) { a[0] = a[0] / a[1]; }


extern EcoSystem *LpCurrentEcoSystem;


void Vegetation::Create(int type, _D3DVECTOR *pos, char height, char radius)
{
    m_type = type;
    m_x = pos->x * LpCurrentEcoSystem->m_x594;
    m_y = pos->y * LpCurrentEcoSystem->m_x594;
    m_z = pos->z * LpCurrentEcoSystem->m_x594;
    m_radius = radius;
    m_height = height;
    m_is2D = 1;
}


// ---- src/Vegetation/NumCollisionObjects.cpp

extern EcoSystem *LpCurrentEcoSystem;


int Vegetation::NumCollisionObjects()
{
    return LpCurrentEcoSystem->m_descriptors[m_type]->m_numCollisionObjects;
}


// ---- src/Vegetation/GetCollisionObject.cpp

#ifdef SPLASHDOWN_PC
// x86-64: the collision data by type. Sphere (5): CollisionObject.h's
// SphereCollisionObject (m_radiusScale, m_scale, m_matrix); segment (4):
// no pointers, the PS2 offsets; Soultree (0): SoultreeCollisionObject's m_xD0.
#include "SoultreeCollisionObject/SoultreeCollisionObject.h"
struct CollisionShape {
    char pad0[0x18];
    float m_radius;
    float m_height;
    char pad20[0x28 - 0x20];
    float m_radius4;                // +0x28 (type 4)
    float m_height4;                // +0x2C
    char pad30[0x60 - 0x30];
    float m_x;                      // +0x60 (type 4)
    float m_y;
    float m_z;
    char pad6C[__builtin_offsetof(SoultreeCollisionObject, m_xD0) - 0x6C];
    _D3DMATRIX m_matrix;            // (type 0)
};
struct SphereShape {
    char pad0[__builtin_offsetof(SphereCollisionObject, m_radiusScale)];
    float m_radius;
    float m_height;
    char pad20[__builtin_offsetof(SphereCollisionObject, m_matrix) + 0x30 - __builtin_offsetof(SphereCollisionObject, m_radiusScale) - 8];
    float m_x;
    float m_y;
    float m_z;
};
#define SPHERE_SHAPE SphereShape
#else
struct CollisionShape {
    char pad0[0x18];
    float m_radius;                 // +0x18 (sphere/cylinder)
    float m_height;                 // +0x1C
    char pad20[0x28 - 0x20];
    float m_radius4;                // +0x28 (type 4)
    float m_height4;                // +0x2C
    char pad30[0x60 - 0x30];
    float m_x;                      // +0x60
    float m_y;                      // +0x64
    float m_z;                      // +0x68
    char pad6C[0xD0 - 0x6C];
    _D3DMATRIX m_matrix;            // +0xD0
};
#define SPHERE_SHAPE CollisionShape
#endif



extern EcoSystem *LpCurrentEcoSystem;


// Places the descriptor's collision object at this plant.
CollisionObject *Vegetation::GetCollisionObject(int index)
{
    CollisionObject *object = LpCurrentEcoSystem->m_descriptors[m_type]->m_collision[index];
    float x = m_x * LpCurrentEcoSystem->m_positionScale;
    float y = m_y * LpCurrentEcoSystem->m_positionScale;
    float z = m_z * LpCurrentEcoSystem->m_positionScale;
    float radius = LpCurrentEcoSystem->m_descriptors[m_type]->GetRadius(m_radius) /
                   LpCurrentEcoSystem->m_descriptors[m_type]->m_meanRadius;
    float height = LpCurrentEcoSystem->m_descriptors[m_type]->GetHeight(m_height) /
                   LpCurrentEcoSystem->m_descriptors[m_type]->m_meanHeight;
    if (object->m_collisionType == 5) {
        ((SPHERE_SHAPE *)object->m_collisionData)->m_radius = radius;
        ((SPHERE_SHAPE *)object->m_collisionData)->m_height = height;
        ((SPHERE_SHAPE *)object->m_collisionData)->m_x = x;
        ((SPHERE_SHAPE *)object->m_collisionData)->m_y = y;
        ((SPHERE_SHAPE *)object->m_collisionData)->m_z = z;
    } else if (object->m_collisionType == 4) {
        ((CollisionShape *)object->m_collisionData)->m_radius4 = radius;
        ((CollisionShape *)object->m_collisionData)->m_height4 = height;
        ((CollisionShape *)object->m_collisionData)->m_x = x;
        ((CollisionShape *)object->m_collisionData)->m_y = y;
        ((CollisionShape *)object->m_collisionData)->m_z = z;
    } else if (object->m_collisionType == 0) {
        _D3DMATRIX matrix;
        matrix = ((CollisionShape *)object->m_collisionData)->m_matrix;
        matrix._41 = x;
        matrix._42 = y;
        matrix._43 = z;
        ((CollisionShape *)object->m_collisionData)->m_matrix._41 = x;
        ((CollisionShape *)object->m_collisionData)->m_matrix._42 = y;
        ((CollisionShape *)object->m_collisionData)->m_matrix._43 = z;
        object->UpdateTransform(&matrix);
    }
    return object;
}


// ---- src/Vegetation/GetRadius.cpp

extern EcoSystem *LpCurrentEcoSystem;


float Vegetation::GetRadius()
{
    return LpCurrentEcoSystem->m_descriptors[m_type]->GetRadius(m_radius);
}


// ---- src/Vegetation/Render.cpp
typedef int int128_t __attribute__((mode(TI)));

static inline int FloatToInt(float f)
{
    int i;
    asm("cvt.w.s $f5, %1; mfc1 %0, $f5" : "=r"(i) : "f"(f) : "$f5");
    return i;
}


struct _sceDmaTag;
class SurfaceGeometry;




extern EcoSystem *LpCurrentEcoSystem;

extern LightManager *LpLightManager;

struct EcoVegetationItem {
    char pad0[0x20];
    unsigned int m_flags;               // +0x20
    char pad24[0x40 - 0x24];
    _D3DMATRIX m_transform;             // +0x40
    char pad80[0xC0 - 0x80];
    long m_test;                        // +0xC0
    long m_testReg;                     // +0xC8
    char padD0[0xE0 - 0xD0];
    union {
        int128_t m_light;               // +0xE0
        float m_lightf[4];
    };
};


#define UNCACHED(p) ((_D3DMATRIX *)PS2_ADDR(((unsigned int)(p) & 0x0FFFFFFF) | EE_UNCACHED))

// Fills a 3D vegetation render item (from Create3DVegetationTransformRenderItem)
// with this plant's transform and alpha test, and queues it on its shader.
int Vegetation::Render(VifDataItem *item)
{
    ((EcoVegetationItem *)item)->m_flags = (m_clipCodes ? 1 : 0) << 2 | 2 | (m_clipCodes ? 1 : 0);
    _D3DMATRIX *dst = UNCACHED(&((EcoVegetationItem *)item)->m_transform);
    VegetationDescriptor *desc = LpCurrentEcoSystem->m_descriptors[m_type];
    float radius = (1.0f / 32767.0f) * (desc->m_radiusScale * desc->GetRadius(m_radius));
    float height = desc->m_heightScale * desc->GetHeight(m_height);
    float c = m_rot.m_cos * radius;
    float s = m_rot.m_sin * radius;
    _D3DMATRIX m;
    m._11 = c;
    m._12 = 0;
    m._13 = s;
    m._14 = 0;
    m._21 = 0;
    m._22 = height;
    m._23 = 0;
    m._24 = 0;
    m._31 = -s;
    m._32 = 0;
    m._33 = c;
    m._34 = 0;
    m._41 = (unsigned int)m_x * LpCurrentEcoSystem->m_positionScale;
    m._42 = (unsigned int)m_y * LpCurrentEcoSystem->m_positionScale;
    m._43 = (unsigned int)m_z * LpCurrentEcoSystem->m_positionScale;
    m._44 = 1.0f;
    // Quadword copy into the uncached packet: hand-written asm (retail uses $at).
    asm("lq $1, 0x0(%0); lq $2, 0x10(%0); lq $3, 0x20(%0); lq %0, 0x30(%0); sq $1, 0x0(%1); sq $2, 0x10(%1); sq $3, 0x20(%1); sq %0, 0x30(%1)"
        : : "r"(&m), "r"(dst) : "$1", "$2", "$3");

    float alpha = 64.0f - 1.33333337f * (m_alpha - 16);
    if (!(alpha <= 64.0f))
        alpha = 64.0f;
    ((EcoVegetationItem *)item)->m_test = (long)FloatToInt(alpha) << 4 | 0x5800D;
    ((EcoVegetationItem *)item)->m_light = LpLightManager->m_ambientValue;
    ((EcoVegetationItem *)item)->m_lightf[3] = alpha;
    desc->m_shader->AddGeometry((SurfaceGeometry *)desc->m_geometry,  // redundant cast: retail loads it first
                                 (_sceDmaTag *)item, 0);
    return 1;
}


// ---- src/EcoSystem/ctor.cpp

extern "C" void *memset(void *dst, int c, unsigned int n);




extern EcoSystem *LpCurrentEcoSystem;

EcoSystem::EcoSystem(int enabled) : SearchableGameObject(enabled)
{
    m_searchVisible = 1;
    m_addToList = 1;
    m_maxVegetation = 0;
    m_vegetation = 0;
    m_visible2D = 0;
    m_visible3D = 0;
    m_visible2D3D = 0;
    m_terrain = 0;
    m_authored = 0;
    m_numVegetation = 0;
    m_x574 = 0x22B;
    memset(m_descriptors, 0, sizeof(m_descriptors));
    LpCurrentEcoSystem = this;
    m_positionScale = 1.0f;
    m_x594 = 1.0f;
    m_numDescriptors = 0;
    m_typeRemap = 0;
    m_vifItems = 0;
    m_maxVifItems = 0;
    m_2DGeometry = 0;
    m_2D3DGeometry = 0;
    m_2DItem = 0;
    m_2DShader = 0;
    m_2DTweakItem = 0;
    m_x5B4 = 0;
    for (int i = 0; i < 600; i++) {
        m_slots[i].m_0 = 0;
        m_slots[i].m_4 = 0;
    }
}


// ---- src/EcoSystem/dtor.cpp

extern "C" void free(void *p);




// One element of the vegetation buffer (constructed, never destroyed).

// A buffered render list: owns a DmaRenderItem.



EcoSystem::~EcoSystem()
{
    int i;
    for (i = 0; i < 256; i++) {
        if (m_descriptors[i])
            delete m_descriptors[i];
    }
    if (m_vegetation)
        delete[] m_vegetation;
    if (m_visible2D)
        free(m_visible2D);
    if (m_visible3D)
        free(m_visible3D);
    if (m_visible2D3D)
        free(m_visible2D3D);
    if (m_typeRemap)
        free(m_typeRemap);
    LpCurrentEcoSystem = 0;
    for (i = 0; i < m_maxVifItems; i++)
        free(m_vifItems[i]);
    if (m_vifItems)
        free(m_vifItems);
    if (m_2DGeometry) {
        delete m_2DGeometry->m_renderItem;
        delete m_2DGeometry;
    }
    if (m_2D3DGeometry) {
        delete m_2D3DGeometry->m_renderItem;
        delete m_2D3DGeometry;
    }
    if (m_2DItem)
        free(m_2DItem);
    if (m_2DTweakItem)
        free(m_2DTweakItem);
}


// ---- src/EcoSystem/GetEcosystemParameters.cpp

extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" int strcmp(const char *a, const char *b);
extern "C" int strcasecmp(const char *a, const char *b);
extern "C" char *strcpy(char *dst, const char *src);




// Reads an ecosystem text description: the method, per-vegetation descriptor
// parameters (and, for the automatic method, the placement statistics).
int EcoSystem::GetEcosystemParameters(char *fileName)
{
    ParameterBlock params;
    char block[0x80];
    char name[0x80];
    int billboardWidth;
    int billboardHeight;
    int red;
    int green;
    int blue;
    int i;
    if (!params.OpenFile(fileName, 0))
        return 0;
    params.ReadParameterBlock("EcoSystem");
    params.GetParameter("Method", "NONE", name, 0x80);
    if (!strcmp(name, "NONE")) {
        PSXprintf("[EcoSystem] Method not defined");
        return 0;
    }
    if (!strcasecmp(name, "Authored")) {
        m_authored = 1;
    } else if (!strcasecmp(name, "Auto")) {
        m_authored = 2;
    } else {
        PSXprintf("[EcoSystem] Method unknown");
        return 0;
    }
    params.GetParameter("TotalObjects", 40000, &m_maxVegetation);
    params.GetParameter("BillboardWidth", 256, &billboardWidth);
    params.GetParameter("BillboardHeight", 256, &billboardHeight);
    float total = 0.0f;
    for (i = 1; i < 256; i++) {
        sprintf(block, "Vegetation_%d", i);
        if (params.ReadParameterBlock(block))
            params.GetParameter("Name", "NONE", name, 0x80);
        else
            strcpy(name, "NONE");
        if (!strcmp(name, "NONE"))
            continue;
        m_descriptors[i] = new VegetationDescriptor;
        strcpy(m_descriptors[i]->m_name, name);
        params.GetParameter("BillboardName", "NONE", name, 0x80);
        strcpy(m_descriptors[i]->m_billboard, name);
        params.GetParameter("MeanHeight", 10.0f, &m_descriptors[i]->m_meanHeight);
        params.GetParameter("MinHeight", 5.0f, &m_descriptors[i]->m_minHeight);
        params.GetParameter("MaxHeight", 15.0f, &m_descriptors[i]->m_maxHeight);
        params.GetParameter("MeanRadius", 10.0f, &m_descriptors[i]->m_meanRadius);
        params.GetParameter("MinRadius", 5.0f, &m_descriptors[i]->m_minRadius);
        params.GetParameter("MaxRadius", 15.0f, &m_descriptors[i]->m_maxRadius);
        params.GetParameter("ULeft", 0.0f, &m_descriptors[i]->m_uLeft);
        m_descriptors[i]->m_uLeft /= billboardWidth;
        params.GetParameter("URight", 1.0f, &m_descriptors[i]->m_uRight);
        m_descriptors[i]->m_uRight /= billboardWidth;
        params.GetParameter("VBottom", 0.0f, &m_descriptors[i]->m_vBottom);
        m_descriptors[i]->m_vBottom /= billboardHeight;
        params.GetParameter("VTop", 1.0f, &m_descriptors[i]->m_vTop);
        m_descriptors[i]->m_vTop /= billboardHeight;
        params.GetParameter("UCenter", 1.0f, &m_descriptors[i]->m_uCenter);
        m_descriptors[i]->m_uCenter /= billboardWidth;
        params.GetParameter("UsePlanarLighting", 1, &m_descriptors[i]->m_planarLighting);
        params.GetParameter("BlendLODs", 1, &m_descriptors[i]->m_blendLODs);
        params.GetParameter("PercentBias", 1.0f, &m_descriptors[i]->m_percent);
        params.GetParameter("KeyColorRed", 0x5A, &red);
        params.GetParameter("KeyColorGreen", 0x5B, &green);
        params.GetParameter("KeyColorBlue", 0xB, &blue);
        m_descriptors[i]->m_keyColor = red << 16 | green << 8 | blue;
        total += m_descriptors[i]->m_percent;
        ReadVegetationCollisionDescriptors(&params, i, m_descriptors[i]);
    }
    if (total != 100.0f) {
        float scale = 100.0f / total;
        for (i = 1; i < 256; i++) {
            if (m_descriptors[i])
                m_descriptors[i]->m_percent *= scale;
        }
    }
    params.ReadParameterBlock("EcoSystem");
    params.GetParameter("PlacementBmp", "NONE", name, 0x80);
    if (!strcmp(name, "NONE")) {
        m_placementBmp[0] = 0;
    } else {
#line 1092 "EcoSystem.cpp"
        STRCPY(m_placementBmp, name);
    }
    if (m_authored == 2) {
        for (i = 0; i < 256; i++) {
            if (!m_descriptors[i])
                continue;
            sprintf(block, "Vegetation_%d", i);
            params.ReadParameterBlock(block);
            params.GetParameter("MeanSlope", 45.0f, &m_descriptors[i]->m_meanSlope);
            m_descriptors[i]->m_meanSlope /= 90.0f;
            params.GetParameter("StandardDeviationSlope", 45.0f, &m_descriptors[i]->m_sdSlope);
            m_descriptors[i]->m_sdSlope /= 90.0f;
            params.GetParameter("MeanAspect", 180.0f, &m_descriptors[i]->m_meanAspect);
            m_descriptors[i]->m_meanAspect = (m_descriptors[i]->m_meanAspect - 180.0f) / 90.0f;
            params.GetParameter("StandardDeviationAspect", 180.0f, &m_descriptors[i]->m_sdAspect);
            m_descriptors[i]->m_sdAspect = (m_descriptors[i]->m_sdAspect - 180.0f) / 90.0f;
            params.GetParameter("MeanDrainage", 0.5f, &m_descriptors[i]->m_meanDrainage);
            params.GetParameter("StandardDeviationDrainage", 0.5f, &m_descriptors[i]->m_sdDrainage);
            params.GetParameter("MeanAltitude", 0.5f, &m_descriptors[i]->m_meanAltitude);
            params.GetParameter("StandardDeviationAltitude", 0.5f, &m_descriptors[i]->m_sdAltitude);
            params.GetParameter("ProbabilityTga", "NONE", name, 0x80);
            if (!strcmp(name, "NONE"))
                m_descriptors[i]->m_probabilityTga[0] = 0;
            else
                strcpy(m_descriptors[i]->m_probabilityTga, name);
        }
        params.ReadParameterBlock("EcoSystem");
        params.GetParameter("NorthAngle", 0.0f, &m_northAngle);
        params.GetParameter("ProbabilityTga", "NONE", name, 0x80);
        if (!strcmp(name, "NONE")) {
            m_probabilityTga[0] = 0;
        } else {
#line 1162 "EcoSystem.cpp"
            STRCPY(m_probabilityTga, name);
        }
    }
    return 1;
}


// ---- src/EcoSystem/CreateVegetationCollisionObjects.cpp
// CreateVegetationCollisionObjects__9EcoSystemFP20VegetationDescriptor  @ 0x002305C0
// The GEOMETRY branch keeps the vertex count read by the index loop's test
// in a local (`j < (numVertices = ...)`) and divides that for the triangle count.


extern "C" void *malloc(unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);




void FastSinCos(float angle, float *s, float *c);

extern int WORLDOBJECT_SOFTFOLIAGE;
extern int WORLDOBJECT_SOLIDFOLIAGE;

class SoultreeObject;



struct CollisionTransform {
#ifdef SPLASHDOWN_PC
    char pad0[__builtin_offsetof(SoultreeCollisionObject, m_xD0)];
#else
    char pad0[0xD0];
#endif
    _D3DMATRIX m_matrix;                // +0xD0
};


struct VegetationCollisionDescriptor {
    _D3DVECTOR m_start;                 // +0x00
    _D3DVECTOR m_end;                   // +0x10
    int m_type;                         // +0x20
    float m_radius;                     // +0x24
    float m_height;                     // +0x28
    char pad2C[0x30 - 0x2C];
};

struct VegetationVertex {
    float x, y, z;
    char padC[0x20 - 0xC];
};



void EcoSystem::CreateVegetationCollisionObjects(VegetationDescriptor *descriptor)
{
    VegetationCollisionDescriptor *object;
    int i;
    if (!descriptor->m_numCollisionObjects) {
        descriptor->m_collision = 0;
        return;
    }
    descriptor->m_collision = (CollisionObject **)malloc(descriptor->m_numCollisionObjects * sizeof(CollisionObject *));
    for (i = 0; i < descriptor->m_numCollisionObjects; i++) {
        object = &descriptor->m_collisionObjects[i];
        descriptor->m_collision[i] = new CollisionObject(1);
        descriptor->m_collision[i]->Create(4);
        AddChild(descriptor->m_collision[i], -1);
        descriptor->m_collision[i]->m_ownerType = WORLDOBJECT_SOFTFOLIAGE;
        if (object->m_type == 0) {
            descriptor->m_collision[i]->m_ownerType = WORLDOBJECT_SOLIDFOLIAGE;
            _D3DVECTOR *points = new _D3DVECTOR[descriptor->m_numVertices];
            unsigned int *indices = new unsigned int[descriptor->m_numVertices];
            int numVertices;
            int j;
            for (j = 0; j < (numVertices = descriptor->m_numVertices); j++)
                indices[j] = j;
            for (j = 0; j < descriptor->m_numVertices; j++) {
                points[j].x = descriptor->m_vertices[j].x;
                points[j].y = descriptor->m_vertices[j].y;
                points[j].z = descriptor->m_vertices[j].z;
            }
            descriptor->m_collision[i]->m_ownerType = WORLDOBJECT_SOLIDFOLIAGE;
            int numTriangles = numVertices / 3;
            descriptor->m_collision[i]->SetGeometryCollision(points, indices, numTriangles, descriptor->m_numVertices);
            _D3DMATRIX *m = &((CollisionTransform *)descriptor->m_collision[i]->m_collisionData)->m_matrix;
            memset(m, 0, sizeof(*m));
            m->_11 = m->_22 = m->_33 = m->_44 = 1.0f;
            delete[] points;
            delete[] indices;
        } else if (object->m_type == 2) {
            descriptor->m_collision[i]->SetSphereCollision(object->m_start, object->m_radius, 0);
        } else if (object->m_type == 3) {
            descriptor->m_collision[i]->SetRadiusLineSegmentCollision(object->m_start, object->m_end, object->m_radius);
        } else if (object->m_type == 1) {
            descriptor->m_collision[i]->m_ownerType = WORLDOBJECT_SOLIDFOLIAGE;
            _D3DVECTOR points[32];
            unsigned int indices[16][6];
            float s, c;
            int j;
            for (j = 0; j < 16; j++) {
                FastSinCos(j * 0.39269908f, &s, &c);
                points[j].x = points[j + 16].x = c * object->m_radius;
                points[j].z = points[j + 16].z = s * object->m_radius;
                points[j].y = object->m_height;
                points[j + 16].y = 0.0f;
            }
            for (j = 0; j < 32; j++)
                points[j] = points[j] + object->m_start;
            for (j = 0; j < 16; j++) {
                int next = j + 1;
                if (next >= 16)
                    next = 0;
                indices[j][0] = j;
                indices[j][1] = next;
                indices[j][2] = next + 16;
                indices[j][3] = j;
                indices[j][4] = next + 16;
                indices[j][5] = j + 16;
            }
            descriptor->m_collision[i]->SetGeometryCollision(points, &indices[0][0], 32, 32);
            _D3DMATRIX *m = &((CollisionTransform *)descriptor->m_collision[i]->m_collisionData)->m_matrix;
            memset(m, 0, sizeof(*m));
            m->_11 = m->_22 = m->_33 = m->_44 = 1.0f;
        } else {
#line 1267 "EcoSystem.cpp"
            ASSERT(0, "CreateVegetationCollisionObjects: Unrecognized collision type in VegetationFile");
        }
    }
}


// ---- src/EcoSystem/ReadVegetationCollisionDescriptors.cpp

extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" void *malloc(unsigned int size);
extern "C" int strcasecmp(const char *a, const char *b);
extern "C" char *strtok(char *s, const char *delim);
extern "C" double atof(const char *s);






// Parses "x,y,z" into a point (w = 1).
#define READ_VECTOR(v)                                  \
    (v).x = atof(strtok(value, ","));                   \
    (v).y = atof(strtok(0, ","));                       \
    (v).z = atof(strtok(0, "\n"));                      \
    (v).w = 1.0f;

void EcoSystem::ReadVegetationCollisionDescriptors(ParameterBlock *block, int index, VegetationDescriptor *descriptor)
{
    char section[0x100];
    char value[0x80];
    char name[0x80];
    char type[0x80];

    sprintf(section, "Vegetation_%d", index);
    if (block->ReadParameterBlock(section))
        block->GetParameter("NumCollisionObjects", 0, &descriptor->m_numCollisionObjects);
    else
        descriptor->m_numCollisionObjects = 0;
    if (descriptor->m_numCollisionObjects > 0) {
        descriptor->m_collisionObjects =
            (VegetationCollisionDescriptor *)malloc(descriptor->m_numCollisionObjects * sizeof(VegetationCollisionDescriptor));
        for (int i = 0; i < descriptor->m_numCollisionObjects; i++) {
            VegetationCollisionDescriptor *object = &descriptor->m_collisionObjects[i];
            sprintf(name, "CollisionObject%i", i + 1);
            block->GetParameter(name, "", type, 0x80);
            if (strcasecmp(type, "GEOMETRY") == 0) {
                object->m_type = 0;
            } else if (strcasecmp(type, "SPHERE") == 0) {
                object->m_type = 2;
                sprintf(name, "CollisionObject%iCenter", i + 1);
                block->GetParameter(name, "", value, 0x80);
                READ_VECTOR(object->m_start);
                sprintf(name, "CollisionObject%iRadius", i + 1);
                block->GetParameter(name, 0.0f, &object->m_radius);
            } else if (strcasecmp(type, "RADIUSEDLINE") == 0) {
                object->m_type = 3;
                sprintf(name, "CollisionObject%iStart", i + 1);
                block->GetParameter(name, "", value, 0x80);
                READ_VECTOR(object->m_start);
                sprintf(name, "CollisionObject%iEnd", i + 1);
                block->GetParameter(name, "", value, 0x80);
                READ_VECTOR(object->m_end);
                sprintf(name, "CollisionObject%iRadius", i + 1);
                block->GetParameter(name, 0.0f, &object->m_radius);
            } else if (strcasecmp(type, "CYLINDER") == 0) {
                object->m_type = 1;
                sprintf(name, "CollisionObject%iBottom", i + 1);
                block->GetParameter(name, "", value, 0x80);
                READ_VECTOR(object->m_start);
                sprintf(name, "CollisionObject%iRadius", i + 1);
                block->GetParameter(name, 0.0f, &object->m_radius);
                sprintf(name, "CollisionObject%iHeight", i + 1);
                block->GetParameter(name, 0.0f, &object->m_height);
            } else {
#line 1355 "EcoSystem.cpp"
                ASSERT(0, "ReadVegetationCollisionDescriptors: Unrecognized collision type in VegetationFile");
            }
        }
    } else {
        descriptor->m_collisionObjects = 0;
    }
}


// ---- src/EcoSystem/WriteBinaryEcoSystemFile.cpp

int EcoSystem::WriteBinaryEcoSystemFile(char *name)
{
    return 0;
}


// ---- src/EcoSystem/ReadBinaryEcoSystemFile.cpp
extern "C" void *malloc(unsigned int size);
void PSXprintf(const char *fmt, ...);

extern DataAccess *LpDataAccess;




#define DESC m_descriptors[m_numDescriptors]

// Reads a .esb file: the vegetation descriptors (with their collision
// shapes), the placement bitmap name and the vegetation count. The file stays
// open for ReCreateEcoSystemFromFile.
int EcoSystem::ReadBinaryEcoSystemFile(char *name, long *outFile)
{
    unsigned char present;
    unsigned char length;
    long file = LpDataAccess->FOpen(name, "rb");
    *outFile = file;
    if (file == -1) {
        PSXprintf("%s not found anywhere!", name);
        return 0;
    }
    LpDataAccess->FRead(file, &m_authored, 4, 1);
    m_typeRemap = (unsigned char *)malloc(0x100);
    m_numDescriptors = 0;
    int i;
    for (i = 0; i < 256; i++)
        m_typeRemap[i] = i;
    for (i = 0; i < 256; i++) {
        LpDataAccess->FRead(file, &present, 1, 1);
        if (!present)
            continue;
        LpDataAccess->FRead(file, &length, 1, 1);
        m_typeRemap[i] = m_numDescriptors;
        DESC = new VegetationDescriptor;
        LpDataAccess->FRead(file, DESC->m_name, length, 1);
        LpDataAccess->FRead(file, &length, 1, 1);
        LpDataAccess->FRead(file, DESC->m_billboard, length, 1);
        LpDataAccess->FRead(file, &DESC->m_meanHeight, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_minHeight, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_maxHeight, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_meanRadius, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_minRadius, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_maxRadius, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_uLeft, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_uRight, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_vBottom, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_vTop, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_uCenter, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_planarLighting, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_blendLODs, 4, 1);
        LpDataAccess->FRead(file, &DESC->m_keyColor, 4, 1);
        if (m_authored == 2) {
            LpDataAccess->FRead(file, &DESC->m_percent, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_meanSlope, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_sdSlope, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_meanAspect, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_sdAspect, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_meanDrainage, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_sdDrainage, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_meanAltitude, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_sdAltitude, 4, 1);
            LpDataAccess->FRead(file, &present, 1, 1);
            if (present) {
                LpDataAccess->FRead(file, &length, 1, 1);
                LpDataAccess->FRead(file, DESC->m_probabilityTga, length, 1);
            }
        }
        LpDataAccess->FRead(file, &DESC->m_numCollisionObjects, 4, 1);
        DESC->m_collisionObjects = (VegetationCollisionDescriptor *)malloc(DESC->m_numCollisionObjects * 0x30);
        for (int j = 0; j < DESC->m_numCollisionObjects; j++) {
            LpDataAccess->FRead(file, &DESC->m_collisionObjects[j].m_type, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_collisionObjects[j].m_start, 0xC, 1);
            DESC->m_collisionObjects[j].m_start.w = 1.0f;
            LpDataAccess->FRead(file, &DESC->m_collisionObjects[j].m_end, 0xC, 1);
            DESC->m_collisionObjects[j].m_end.w = 1.0f;
            LpDataAccess->FRead(file, &DESC->m_collisionObjects[j].m_radius, 4, 1);
            LpDataAccess->FRead(file, &DESC->m_collisionObjects[j].m_height, 4, 1);
        }
        m_numDescriptors++;
    }
    LpDataAccess->FRead(file, &present, 1, 1);
    if (present) {
        LpDataAccess->FRead(file, &length, 1, 1);
        LpDataAccess->FRead(file, m_placementBmp, length, 1);
    }
    if (m_authored == 2) {
        LpDataAccess->FRead(file, &m_northAngle, 4, 1);
        LpDataAccess->FRead(file, &present, 1, 1);
        if (present) {
            LpDataAccess->FRead(file, &length, 1, 1);
            LpDataAccess->FRead(file, m_probabilityTga, length, 1);
        }
    }
    LpDataAccess->FRead(file, &m_numVegetation, 4, 1);
    m_maxVegetation = m_numVegetation;
    return 1;
}


// ---- src/EcoSystem/Create.cpp
// Create__9EcoSystemFP7TerrainPciiiffi  @ 0x002319F0
// FindExtension is inlined inside SetExtension.
// The register map hinged on spill choice: {this, name, format, collision, c, _n, binary}
// all reach degree 26, and retail pushes _n (lowest cost/degree) where the old STRCPY (reusing _n,
// cost 10) let c (cost 3) go first. The shared STRCPY now keeps the clamped
// length in its own variable (_c), which gives _n few enough references to be that pick.
#pragma inline_depth(5)

extern "C" char *strcpy(char *dst, const char *src);
extern "C" char *strcat(char *dst, const char *src);
extern "C" char *strstr(const char *s, const char *find);
extern "C" void *malloc(unsigned int size);


static inline float Fmax(float a, float b)
{
    float r;
    asm("max.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}

// An inlined strrchr(name, '.').
static inline char *FindExtension2(char *s)
{
    char *start = s;
    while (*s++)
        ;
    do {
        s--;
    } while (s != start && *s != '.');
    s = *s == '.' ? s : 0;
    return s;
}

static inline void SetExtension(char *name, char *extension)
{
    char *ext;
    if ((ext = FindExtension2(name)))
        strcpy(ext, extension);
    else
        strcat(name, extension);
}

extern DataAccess *LpDataAccess;

extern TypeRegistry *LpTypeRegistry;



VifDataItem *Create3DVegetationTransformRenderItem();
VifDataItem *Create2DTweakItem();
int IsAlphaFormat(int format);





EcoSystem *EcoSystem::Create(Terrain *terrain, char *name, int format, int collision, int, float a, float b, int c)
{
    GameObject::Create();
#line 1665 "EcoSystem.cpp"
    ASSERT(LpVisibilityQuadTree, "Create: VisibilityQuadTree MUST be created before ecosystem");
    m_x578 = a;
    m_x57C = b;
    m_terrain = terrain;
    m_x574 = format;
    float width = LpVisibilityQuadTree->m_sizeX;
    float depth = LpVisibilityQuadTree->m_sizeZ;
    float size = Fmax(width, depth);
    m_x594 = 65536.0f / size;
    m_positionScale = size / 65536.0f;
    int binary = 0;
    if (strstr(name, ".esb")) {
        binary = 1;
    } else if (strstr(name, ".est")) {
        char path[0x104];
#line 1696 "EcoSystem.cpp"
        STRCPY(path, name);
        SetExtension(path, ".esb");
        if (LpDataAccess->FileExists(path)) {
            SetExtension(name, ".esb");
            binary = 1;
        }
    }
    long file = -1;
    if (binary) {
        if (!ReadBinaryEcoSystemFile(name, &file)) {
            Release();
            PSXprintf("EcoSystem not created.\n");
            return 0;
        }
    } else if (!GetEcosystemParameters(name)) {
        Release();
        PSXprintf("EcoSystem not created.\n");
        return 0;
    }
    int i;
    for (i = 0; i < 256; i++) {
        if (m_descriptors[i]) {
            m_descriptors[i]->Load(0x22B8);
            if (collision)
                CreateVegetationCollisionObjects(m_descriptors[i]);
            else
                m_descriptors[i]->m_numCollisionObjects = 0;
        }
    }
    m_typeID = LpTypeRegistry->GetTypeID("Vegetation");
    m_vegetation = new Vegetation[m_maxVegetation];
    m_maxVisible2D = 1500;
    m_maxVisible3D = 60;
    m_maxVisible2D3D = 60;
    m_visible2D = (Vegetation **)malloc(m_maxVisible2D * PS2_PTR_SIZE);
    m_visible3D = (Vegetation **)malloc(m_maxVisible3D * PS2_PTR_SIZE);
    m_visible2D3D = (Vegetation **)malloc(m_maxVisible2D3D * PS2_PTR_SIZE);
    if (!m_vegetation || !m_visible2D || !m_visible3D || !m_visible2D3D)
        goto fail;
    m_maxVifItems = 60;
    m_vifItems = (VifDataItem **)malloc(m_maxVifItems * PS2_PTR_SIZE);
    if (!m_vifItems)
        goto fail;
    for (i = 0; i < 60; i++)
        m_vifItems[i] = Create3DVegetationTransformRenderItem();
    m_2DItem = (VifDataItem *)Create2DTransformRenderItem();
    m_2DTweakItem = Create2DTweakItem();
    if (binary) {
        ReCreateEcoSystemFromFile(file, c);
        LpDataAccess->FClose(file);
    } else if (m_authored == 1) {
        CreateAuthoredEcoSystem();
        WriteBinaryEcoSystemFile(name);
    } else {
        GenerateEcoSystem(IsAlphaFormat(format));
        WriteBinaryEcoSystemFile(name);
    }
    {
        _D3DVECTOR min;
        _D3DVECTOR max;
        min.x = LpVisibilityQuadTree->m_sizeX / 2.0f;
        min.y = 0.0f;
        min.z = LpVisibilityQuadTree->m_sizeZ / 2.0f;
        max.x = min.x / 2.0f;
        max.y = 1000.0f;
        max.z = min.z / 2.0f;
        unsigned int code = LpVisibilityQuadTree->CalculateBoxPlacementCode(&min, &max, 0);
        LpVisibilityQuadTree->InsertObject(this, code, &min, &max);
    }
    return this;
fail:
    PSXprintf("EcoSystem not created.\n");
    Release();
    return 0;
}


// ---- src/EcoSystem/ReCreateEcoSystemFromFile.cpp


extern DataAccess *LpDataAccess;

class SearchableObject;





extern EcoSystem *LpCurrentEcoSystem;

// Position units to world units, through the current ecosystem.
static inline float ToWorld(unsigned short v)
{
    return v * LpCurrentEcoSystem->m_positionScale;
}

int EcoSystem::ReCreateEcoSystemFromFile(long file, int placeOnTerrain)
{
    for (int i = 0; i < m_numVegetation; i++) {
        char height;
        char radius;
        unsigned char type;
        WordVector position;
        LpDataAccess->FRead(file, &type, 1, 1);
        int desc;
        if (m_typeRemap)
            desc = m_typeRemap[type];
        else
            desc = type;
        LpDataAccess->FRead(file, &position, 6, 1);
        LpDataAccess->FRead(file, &height, 1, 1);
        LpDataAccess->FRead(file, &radius, 1, 1);
        if (placeOnTerrain) {
            _D3DVECTOR point;
            point.x = position.x * m_positionScale;
            point.y = position.y * m_positionScale;
            point.z = position.z * m_positionScale;
            m_terrain->GetGridPointIntersection(&point, 0, 0, 0);
            position.y = point.y * m_x594;
        }
        m_vegetation[i].Create(desc, &position, height, radius);
        _D3DVECTOR center;
        _D3DVECTOR extent;
        extent.x = extent.z = m_descriptors[desc]->GetRadius(radius);
        extent.y = m_descriptors[desc]->GetHeight(height) * 0.5;
        center.x = ToWorld(m_vegetation[i].m_x);
        center.y = extent.y + ToWorld(m_vegetation[i].m_y);
        center.z = ToWorld(m_vegetation[i].m_z);
        unsigned int code = LpVisibilityQuadTree->CalculateBoxPlacementCode(&center, &extent, 0);
        LpVisibilityQuadTree->InsertObject((SearchableObject *)&m_vegetation[i], code, &center, &extent);
    }
    return 1;
}


// ---- src/EcoSystem/CreateAuthoredEcoSystem.cpp
void PSXprintf(const char *fmt, ...);


struct BmpFile {
    char pad0[0x14];
    unsigned int m_width;               // +0x14
    unsigned int m_height;              // +0x18
    char pad1C[0x438 - 0x1C];
    unsigned char *m_pixels;            // +0x438
};

BmpFile *ImportBmpFile(char *name, BmpFile *file);
void FreeBmpFile(BmpFile *file);






// Places vegetation from the authored placement bitmap: each nonzero pixel
// is a descriptor index, positioned at its cell's centre.
static inline float CellSize(float size, float n) { return size / n; }

int EcoSystem::CreateAuthoredEcoSystem()
{
    if (!m_placementBmp[0])
        return 0;
    BmpFile *bmp;
    if (!(bmp = ImportBmpFile(m_placementBmp, 0))) {
        PSXprintf("PlacementBmp '%s' specified but not found\n", m_placementBmp);
        return 0;
    }
    unsigned char *pixel = bmp->m_pixels;
    float sizeX = LpVisibilityQuadTree->m_sizeX;
    float cellX = sizeX / bmp->m_width;
    float sizeZ = LpVisibilityQuadTree->m_sizeZ;
    float cellZ = sizeZ / bmp->m_height;
    for (int z = 0; z < bmp->m_height; z++) {
        float posZ = (0.5f + z) * cellZ;
        int type;
        for (int x = 0; x < bmp->m_width; x++) {
            type = *pixel++;
            if (type && m_descriptors[type]) {
                if (m_numVegetation == m_maxVegetation) {
                    PSXprintf("EcoSystem PlacementBmp has more than %d object placements", m_maxVegetation);
                    goto done;
                }
                _D3DVECTOR position;
                position.x = (0.5f + x) * cellX;
                position.z = posZ;
                position.y = 0.0f;
                m_terrain->GetGridPointIntersection(&position, 0, 0, 0);
                char height = m_descriptors[type]->GenerateHeight();
                char radius = m_descriptors[type]->GenerateRadius(m_descriptors[type]->GetHeight(height));
                m_vegetation[m_numVegetation].Create(type, &position, height, radius);
                m_numVegetation++;
            }
        }
    }
done:
    FreeBmpFile(bmp);
    return 1;
}


// ---- src/EcoSystem/GenerateEcoSystem.cpp
// GenerateEcoSystem__9EcoSystemFi  @ 0x002327C0
// Scatters vegetation over the terrain: picks a type by its percentage, a random position (or a
// clump on a 5x5/6x6 grid with the EcoGen registry setting), and keeps it with the probability
// from the probability TGAs, the slope and the altitude normal distributions.
#pragma fast_fptosi on


extern "C" void *memset(void *p, int c, unsigned int n);
extern "C" int rand();
extern "C" double acos(double);
extern "C" double atan2(double, double);

TgaFile *ImportTgaFile(char *name, TgaFile *file);
void FreeTgaFile(TgaFile *file);


extern Game *LpGame;
extern NormalDistribution *LpNormalDistribution;

// xyz dot product on VU0.

// The pixel index of a world position in a TGA stretched over the quad tree's extent. v comes
// first: inline arguments evaluate right to left, and retail computes u first.
static inline int TgaPixelIndex(TgaFile *tga, float v, float u)
{
    int y;
    int x;
    x = (int)(u * tga->width);
    y = (int)(v * tga->height);
    if (x >= tga->width)
        x = tga->width - 1;
    if (y >= tga->height)
        y = tga->height - 1;
    return x + tga->width * y;
}

// out = (or *= for TgaScale) the green channel at a world position, 0..1. Out-parameters, not
// return values: a float-returning inline's result merges with the caller's local, which then
// colours last. Each format arm reads through its own pixel pointer; the 16-bit arm doubles
// index in place (both as retail's registers show).
#define TGA_GREEN(tga, position, out, OP)                                                     \
    {                                                                                         \
        float sz = LpVisibilityQuadTree->m_sizeZ;                                             \
        float sx = LpVisibilityQuadTree->m_sizeX;                                             \
        int index = TgaPixelIndex(tga, 1.0f - (position)->z / sz, (position)->x / sx);        \
        unsigned int value;                                                                   \
        if (tga->bitsPerPixel == 32) {                                                        \
            unsigned char *p = (unsigned char *)tga->pixels;                                  \
            value = p[index * 4 + 1];                                                         \
            out OP value / 255.0f;                                                            \
        } else if (tga->bitsPerPixel == 24) {                                                 \
            unsigned char *p = (unsigned char *)tga->pixels;                                  \
            value = p[index * 3 + 1];                                                         \
            out OP value / 255.0f;                                                            \
        } else {                                                                              \
            unsigned char *p = (unsigned char *)tga->pixels;                                  \
            index *= 2;                                                                       \
            out OP ((*(unsigned short *)(p + index) & 0x3E0) >> 5) / 31.0f;                   \
        }                                                                                     \
    }

static inline void TgaValue(TgaFile *tga, _D3DVECTOR *position, float &out)
{
    TGA_GREEN(tga, position, out, =);
}

static inline void TgaScale(TgaFile *tga, _D3DVECTOR *position, float &out)
{
    TGA_GREEN(tga, position, out, *=);
}

// Leaves the enclosing loop when the green channel at a world position is zero. Here sx is
// declared before sz, and the 16-bit offset is a new local (retail's registers again).
#define BREAK_IF_TGA_PIXEL_ZERO(tga, position)                                                \
    {                                                                                         \
        float sx = LpVisibilityQuadTree->m_sizeX;                                             \
        float sz = LpVisibilityQuadTree->m_sizeZ;                                             \
        int index = TgaPixelIndex(tga, 1.0f - (position)->z / sz, (position)->x / sx);        \
        if (tga->bitsPerPixel == 32) {                                                        \
            unsigned char *p = (unsigned char *)tga->pixels;                                  \
            if (!p[index * 4 + 1])                                                            \
                break;                                                                        \
        } else if (tga->bitsPerPixel == 24) {                                                 \
            unsigned char *p = (unsigned char *)tga->pixels;                                  \
            if (!p[index * 3 + 1])                                                            \
                break;                                                                        \
        } else {                                                                              \
            unsigned char *p = (unsigned char *)tga->pixels;                                  \
            int offset = index * 2;                                                           \
            if (!(*(unsigned short *)(p + offset) & 0x3E0))                                   \
                break;                                                                        \
        }                                                                                     \
    }

int EcoSystem::GenerateEcoSystem(int singleTile)
{
    float slope;
    float spacing;
    float threshold;
    float probability;
    float altitude;
    float sizeX;
    float sizeZ;

    CreateAuthoredEcoSystem();

    TgaFile *probabilityTga = 0;
    TgaFile *typeTga[256];
    memset(typeTga, 0, sizeof(typeTga));
    if (m_probabilityTga[0])
        probabilityTga = ImportTgaFile(m_probabilityTga, 0);
    int i;
    for (i = 0; i < 256; i++) {
        if (m_descriptors[i] && m_descriptors[i]->m_probabilityTga[0])
            typeTga[i] = ImportTgaFile(m_descriptors[i]->m_probabilityTga, 0);
    }

    _D3DVECTOR up = _D3DVECTOR(0.0f, 1.0f, 0.0f);
    float cumulative[256];
    int typeIndex[256];
    unsigned char radius; // unsigned: each char argument reloads it from its spill slot through $v0
    unsigned int attempts = 0;
    float maxHeight;
    float minHeight;
    m_terrain->GetMinMaxHeight(&minHeight, &maxHeight);

    float total = 0.0f;
    int tries = 0;
    int numTypes = 0;
    for (i = 0; i < 256; i++) {
        if (m_descriptors[i]) {
            typeIndex[numTypes] = i;
            cumulative[numTypes] = total + m_descriptors[i]->m_percent;
            total = cumulative[numTypes];
            numTypes++;
        }
    }

    // EcoGen 5/6: plant clumps on a 5x5 / 6x6 grid pattern instead of single plants.
    int ecoGen = LpGame->GetRegistryInt("EcoGen", 0);
    sizeX = LpVisibilityQuadTree->m_sizeX;
    sizeZ = LpVisibilityQuadTree->m_sizeZ;
    if (ecoGen == 5) {
        spacing = sizeX / 5.0f;
        sizeZ = spacing;
        sizeX = spacing;
    } else if (ecoGen == 6) {
        spacing = sizeX / 6.0f;
        sizeZ = spacing;
        sizeX = spacing;
    }

    _D3DVECTOR grid[32];
    _D3DVECTOR *inner = &grid[(ecoGen + ecoGen - 2) * 2];
    if (ecoGen) {
        int n = 0;
        for (i = 0; i < ecoGen; i++) {
            grid[n++] = _D3DVECTOR(i * spacing, 0.0f, 0.0f);
            grid[n++] = _D3DVECTOR(i * spacing, 0.0f, (ecoGen - 1) * spacing);
        }
        for (i = 1; i < ecoGen - 1; i++) {
            grid[n++] = _D3DVECTOR(0.0f, 0.0f, i * spacing);
            grid[n++] = _D3DVECTOR((ecoGen - 1) * spacing, 0.0f, i * spacing);
        }
        n = 0;
        for (i = 1; i < ecoGen - 1; i++) {
            inner[n++] = _D3DVECTOR(i * spacing, 0.0f, spacing);
            inner[n++] = _D3DVECTOR(i * spacing, 0.0f, (ecoGen - 2) * spacing);
        }
        for (i = 2; i < ecoGen - 2; i++) {
            inner[n++] = _D3DVECTOR(spacing, 0.0f, i * spacing);
            inner[n++] = _D3DVECTOR((ecoGen - 2) * spacing, 0.0f, i * spacing);
        }
    }

    while (m_numVegetation < m_maxVegetation && tries < 10000) {
        attempts++;
        int type = typeIndex[numTypes - 1];
        tries++;
        float pick = 100.0f * rand() / 2147483648.0f;
        for (i = 0; i < numTypes; i++) {
            if (pick <= cumulative[i]) {
                type = typeIndex[i];
                break;
            }
        }

        _D3DVECTOR position;
        _D3DVECTOR normal;
        position.x = sizeX * (4.656613e-10f * rand());
        position.z = sizeZ * (4.656613e-10f * rand());
        position.y = 0.0f;
        int count = 1;
        _D3DVECTOR *offsets = 0;
        if (ecoGen == 5) {
            if (attempts % 3) {
                count = 24;
                offsets = grid;
            } else {
                position.x += 2.0f * spacing;
                position.z += 2.0f * spacing;
            }
        } else if (ecoGen == 6) {
            if (attempts % 6 == 0) {
                count = 20;
                offsets = grid;
            } else if (attempts % 6 == 1) {
                offsets = inner;
                count = 12;
                position.x += spacing;
                position.z += spacing;
            } else {
                position.x = 2.0f * position.x + 2.0f * spacing;
                position.z = 2.0f * position.z + 2.0f * spacing;
            }
        }
        if (m_numVegetation + count > m_maxVegetation)
            continue;

        unsigned char height = m_descriptors[type]->GenerateHeight();
        radius = m_descriptors[type]->GenerateRadius(m_descriptors[type]->GetHeight(height));
        _D3DVECTOR extent;
        extent.x = extent.z = m_descriptors[type]->GetRadius(radius);
        extent.y = 0.5 * m_descriptors[type]->GetHeight(height);
        _D3DVECTOR center;
        center = position;
        center.y += extent.y;
        unsigned int code = LpVisibilityQuadTree->CalculateBoxPlacementCode(&center, &extent, 0);
        if (singleTile && !LpVisibilityQuadTree->PlacementCodeIsInSingleTile(code))
            continue;

        threshold = 3.0f * (4.656613e-10f * rand());
        if (probabilityTga)
            TgaValue(probabilityTga, &center, probability);
        else
            probability = 1.0f;
        if (probability <= threshold)
            continue;
        if (typeTga[type])
            TgaScale(typeTga[type], &center, probability);
        if (probability <= threshold)
            continue;

        m_terrain->GetGridPointIntersection(&center, &normal, 0, 0);
        slope = acos(DotProduct(&normal, &up));
        float aspect = 3.1415927f - atan2(normal.x, normal.z);
        altitude = center.y - extent.y;
        if (slope < 0.0f)
            slope = 0.0f;
        slope = 2.0f * slope / 3.1415927f;
        probability *= LpNormalDistribution->GetProbability(slope, m_descriptors[type]->m_meanSlope,
                                                            m_descriptors[type]->m_sdSlope);
        if (probability <= threshold)
            continue;
        altitude = (altitude - minHeight) / (maxHeight - minHeight);
        probability *= LpNormalDistribution->GetProbability(altitude, m_descriptors[type]->m_meanAltitude,
                                                            m_descriptors[type]->m_sdAltitude);
        if (probability <= threshold)
            continue;

        // Every plant of the clump must land on a nonzero pixel of both maps.
        for (i = 1; i < count; i++) {
            center = position + offsets[i];
            if (probabilityTga)
                BREAK_IF_TGA_PIXEL_ZERO(probabilityTga, &center);
            TgaFile *tga = typeTga[type];
            if (tga)
                BREAK_IF_TGA_PIXEL_ZERO(tga, &center);
        }
        if (i != count)
            continue;

        for (i = 0; i < count; i++) {
            if (count != 1)
                center = position + offsets[i];
            else
                center = position;
            m_terrain->GetGridPointIntersection(&center, 0, 0, 0);
            unsigned int placement = LpVisibilityQuadTree->CalculateBoxPlacementCode(&center, &extent, 0);
            m_vegetation[m_numVegetation].Create(type, &center, height, radius);
            LpVisibilityQuadTree->InsertObject(&m_vegetation[m_numVegetation], placement, &center, &extent);
            m_numVegetation++;
        }
        tries = 0;
    }

    if (probabilityTga)
        FreeTgaFile(probabilityTga);
    for (i = 0; i < 256; i++) {
        if (typeTga[i])
            FreeTgaFile(typeTga[i]);
    }
    return 1;
}
#pragma fast_fptosi reset


// ---- src/EcoSystem/AddToBufferOf2DVegetation.cpp
#pragma alignlabel off

extern "C" void *realloc(void *p, unsigned int size);

class Vegetation;


void EcoSystem::AddToBufferOf2DVegetation(Vegetation *vegetation)
{
    if (m_numVisible2D == m_maxVisible2D) {
#line 2426 "EcoSystem.cpp"
        ASSERT(0, "AddToBufferOf2DVegetation:  reallocations no longer allowed!");
        m_visible2D = (Vegetation **)realloc(m_visible2D, (m_maxVisible2D + 100) * sizeof(Vegetation *));
        if (!m_visible2D) {
#line 2429 "EcoSystem.cpp"
            ASSERT(0, "AddToBufferOf2DVegetation: LpVisibleVegetation2D==NULL");
            return;
        }
        m_maxVisible2D += 100;
    }
    m_visible2D[m_numVisible2D++] = vegetation;
}
#pragma alignlabel reset


// ---- src/EcoSystem/AddToBufferOf3DVegetation.cpp

extern "C" void *realloc(void *p, unsigned int size);

class Vegetation;
class VifDataItem;

VifDataItem *Create3DVegetationTransformRenderItem();


void EcoSystem::AddToBufferOf3DVegetation(Vegetation *vegetation)
{
    if (m_numVisible3D == m_maxVisible3D) {
        m_visible3D = (Vegetation **)realloc(m_visible3D, (m_maxVisible3D + 20) * sizeof(Vegetation *));
        if (!m_visible3D) {
#line 2446 "EcoSystem.cpp"
            ASSERT(0, "AddToBufferOf3DVegetation: LpVisibleVegetation3D==NULL");
            return;
        }
        m_maxVisible3D += 20;
    }
    m_visible3D[m_numVisible3D++] = vegetation;
    if (m_numVisible3D - 1 == m_maxVifItems) {
        m_vifItems = (VifDataItem **)realloc(m_vifItems, (m_maxVifItems + 20) * sizeof(VifDataItem *));
        if (!m_vifItems) {
#line 2459 "EcoSystem.cpp"
            ASSERT(0, "AddToBufferOf3DVegetation: RenderList3D==NULL");
            return;
        }
        m_maxVifItems += 20;
        for (int i = 0; i < 20; i++)
            m_vifItems[m_numVisible3D - 1 + i] = Create3DVegetationTransformRenderItem();
    }
}


// ---- src/EcoSystem/AddToBufferOf2D3DVegetation.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off

extern "C" void *realloc(void *p, unsigned int size);
class Vegetation;


void EcoSystem::AddToBufferOf2D3DVegetation(Vegetation *veg)
{
    if (m_numVisible2D3D == m_maxVisible2D3D) {
        m_visible2D3D = (Vegetation **)realloc(m_visible2D3D, (m_maxVisible2D3D + 20) * sizeof(Vegetation *));
        if (!m_visible2D3D) {
#line 2480 "EcoSystem.cpp"
            ASSERT(0, "AddToBufferOf2D3DVegetation: LpVisibleVegetation2D3D==NULL");
            return;
        }
        m_maxVisible2D3D += 20;
    }
    m_visible2D3D[m_numVisible2D3D++] = veg;
}
#pragma alignlabel reset


// ---- src/EcoSystem/PrepareGeometry.cpp

extern int RenderEcoSystem;
extern RenderTarget *LpRenderTarget;
extern _D3DMATRIX *LpViewMatrix;
extern VisibleTest *VisTest;
extern EcoSystem *LpCurrentEcoSystem;
extern StringTable *LpMaterialStringTable;
extern StringTable *LpTextureMapStringTable;
extern SurfaceShaderList *LpSurfaceShaderList;

extern int ScaledDistance2D;
extern int ScaledDistance3D;
extern int ScaledDistance2DSquared;
extern int ScaledDistance3DSquared;
extern int ScaledCameraPositionX;
extern int ScaledCameraPositionZ;
extern float AlphaScalar;
extern float trigScalarOverHyp;

// Vegetation as PrepareGeometry sees it (Vegetation.h names +0x04 as one int).

// What VegetationDescriptor +0x244 points to: its model's geometry is at +0x8.

inline float Sqrt(float x)
{
    float r;
    asm("sqrt.s $f8, %1; swc1 $f8, 0(%0)" : : "r"(&r), "f"(x) : "$f8", "memory");
    return r;
}

#pragma fast_fptosi on

// Culls this EcoSystem's plants out of the visibility tree's search list,
// picks 2D, 3D or blended rendering by distance, and bucket-sorts them by
// depth (600 buckets, each a list linked through m_next) for the AddToBuffer
// calls. The sqrt/rsqrt asm helpers turn the global optimizer off.

int EcoSystem::PrepareGeometry()
{
    if (!RenderEcoSystem)
        return 1;
    if (!m_maxVegetation)
        return 1;
    // Profiler IDs (unused).
    static int PrepareGeometryID1 = -1;
    static int EcoPrprGeomID = -1;
    float farClip = LpRenderTarget->m_camera->m_farClip;
    LpViewMatrix = &LpRenderTarget->m_camera->m_view;
    m_depthScale = 65535.0f / farClip;
    m_depthScaleInv = farClip / 65535.0f;
    float lodScale = 600.0f * m_depthScaleInv / farClip;
    m_numVisible2D = 0;
    m_numVisible3D = 0;
    m_numVisible2D3D = 0;
    LpVisibilityQuadTree->RewindSearch();

    // Declared apart from their initialisation: this order (found by
    // a statement-order search) gives retail's register allocation.
    Vegetation *veg;
    int i;
    int count;
    float depthX;
    Camera *camera;
    float depthZ;
    float depthW;
    SearchableObject **out;
    _D3DMATRIX *viewProjection;
    _D3DMATRIX *guardBand;
    unsigned int index;
    i = 0;
    count = 0;
    ScaledDistance2D = m_x578 * m_x594;
    ScaledDistance3D = m_x57C * m_x594;
    ScaledDistance3DSquared = ScaledDistance3D * ScaledDistance3D;
    ScaledDistance2DSquared = ScaledDistance2D * ScaledDistance2D;
    ScaledCameraPositionX = LpRenderTarget->m_camera->m_position.x * m_x594;
    ScaledCameraPositionZ = LpRenderTarget->m_camera->m_position.z * m_x594;
    AlphaScalar = 64.0f / (ScaledDistance2D - ScaledDistance3D);
    out = LpVisibilityQuadTree->m_listCursor;
    depthX = LpViewMatrix->_13 * m_depthScale;
    depthZ = LpViewMatrix->_33 * m_depthScale;
    depthW = LpViewMatrix->_43 * m_depthScale;
    camera = LpRenderTarget->m_camera;
    viewProjection = &camera->m_viewProjection;
#ifdef SPLASHDOWN_PC
    guardBand = &((PSXCamera *)camera)->m_guardBandViewProjection;
#else
    guardBand = (_D3DMATRIX *)((char *)camera + 0x2D0);
#endif
    trigScalarOverHyp = 32767.0f * RSqrt(camera->m_lookAt.x * camera->m_lookAt.x + camera->m_lookAt.z * camera->m_lookAt.z);
    m_x21D8[0] = 0x7FFFFFFF;
    m_x21D8[1] = 0;
    m_x5B4++;

    for (;;) {
        SearchableObject **list = LpVisibilityQuadTree->m_listCursor;
        if (!list || i >= LpVisibilityQuadTree->m_listCount)
            break;
        unsigned int entry = (unsigned int)list[i++];
        veg = (Vegetation *)PS2_ADDR((entry & ~1));
        if (!veg)
            break;
        if (veg->m_typeID != m_typeID) {
            *out = (SearchableObject *)PS2_ADDR(entry);
            out++;
            count++;
            continue;
        }
        int is2D;
        int alpha;
        int dx = ScaledCameraPositionX - veg->m_x;
        int dz = ScaledCameraPositionZ - veg->m_z;
        int d2 = dx * dx + dz * dz;
        if (d2 > ScaledDistance2DSquared) {
            is2D = 1;
            alpha = 64;
        } else {
            is2D = 0;
            if (d2 < ScaledDistance3DSquared)
                alpha = 0;
            else
                alpha = FloatToInt(AlphaScalar * (Sqrt(d2) - ScaledDistance3D));
        }
        VegetationDescriptor *desc = m_descriptors[veg->m_type];
        _D3DVECTOR center;
        _D3DVECTOR half;
        int visible = 1;
        center.x = (unsigned int)veg->m_x * LpCurrentEcoSystem->m_positionScale;
        center.z = (unsigned int)veg->m_z * LpCurrentEcoSystem->m_positionScale;
        if (!is2D) {
            if (!(entry & 1)) {
                veg->m_clipCodes = 0;
                if (is2D != veg->m_is2D) {
                    desc->GetRadius(veg->m_radius);
                    desc->GetHeight(veg->m_height);
                }
            } else {
                static int IsVisID1 = -1;
                float radius = desc->GetRadius(veg->m_radius);
                float halfHeight = 0.5f * desc->GetHeight(veg->m_height);
                center.y = halfHeight + (unsigned int)veg->m_y * LpCurrentEcoSystem->m_positionScale;
                half.x = radius;
                half.y = halfHeight;
                half.z = radius;
                veg->m_clipCodes = 0;
                if ((visible = VisTest->BoundingBoxVisible(viewProjection, &center, &half, 0)) != 0)
                    VisTest->BoundingBoxVisible(guardBand, &center, &half, &veg->m_clipCodes);
            }
        }
        if (!visible)
            continue;
        if (is2D != veg->m_is2D && is2D < desc->m_x1D8) {
            float c, s;
            if (camera->m_lookAt.z != 0.0f) {
                c = camera->m_lookAt.z * trigScalarOverHyp;
                s = -camera->m_lookAt.x * trigScalarOverHyp;
            } else {
                c = 32767.0f;
                s = 0.0f;
            }
            veg->m_rot.m_cos = FloatToInt(c);
            veg->m_rot.m_sin = FloatToInt(s);
        }
        veg->m_is2D = is2D;
        veg->m_alpha = alpha;
        if (!is2D && alpha && (unsigned short)m_x5B4 - veg->m_next >= 4)
            veg->m_alpha = 0;
        float depth = center.x * depthX + center.z * depthZ + depthW;
        if (depth < 0.0f)
            veg->m_depth = 0;
        else if (!(depth < 65535.0f))
            veg->m_depth = 0xFFFF;
        else
            veg->m_depth = FloatToInt(depth);
        int bucket = FloatToInt(depth * lodScale);
        if (bucket >= 600)
            bucket = 599;
        else if (bucket < 0)
            bucket = 0;
        if ((unsigned int)m_slots[bucket].m_0 < (unsigned int)m_x5B4) {
            m_slots[bucket].m_0 = m_x5B4;
            m_slots[bucket].m_4 = (int)veg;
            m_slots[bucket].m_8 = 1;
            veg->m_next = 0xFFFF;
            if (bucket < m_x21D8[0])
                m_x21D8[0] = bucket;
            if (m_x21D8[1] < bucket)
                m_x21D8[1] = bucket;
            continue;
        }
        Vegetation *cur = (Vegetation *)PS2_ADDR(m_slots[bucket].m_4);
        if (veg->m_depth >= cur->m_depth) {
            index = ((unsigned int)cur - (unsigned int)m_vegetation) / sizeof(Vegetation);
            veg->m_next = index;
            m_slots[bucket].m_4 = (int)veg;
        } else {
            Vegetation *prev = cur;
            while (veg->m_depth < cur->m_depth) {
                if (cur->m_next == 0xFFFF) {
                    index = ((unsigned int)veg - (unsigned int)m_vegetation) / sizeof(Vegetation);
                    cur->m_next = index;
                    veg->m_next = 0xFFFF;
                    cur = 0;
                    break;
                }
                prev = cur;
                cur = &m_vegetation[cur->m_next];
            }
            if (cur) {
                veg->m_next = prev->m_next;
                index = ((unsigned int)veg - (unsigned int)m_vegetation) / sizeof(Vegetation);
                prev->m_next = index;
            }
        }
        m_slots[bucket].m_8++;
    }
    LpVisibilityQuadTree->m_listCount = count;
    static int PrepareGeometryID2 = -1;
    if (m_x21D8[1]) {
        int total = 0;
        for (i = m_x21D8[0]; i <= m_x21D8[1]; i++) {
            if (m_slots[i].m_0 == m_x5B4) {
                total = m_slots[i].m_8 + total;
                m_slots[i].m_8 = total;
            }
        }
        for (i = m_x21D8[1]; i >= m_x21D8[0]; i--) {
            if (m_slots[i].m_0 == m_x5B4 && m_slots[i].m_8 < 1500) {
                veg = (Vegetation *)PS2_ADDR(m_slots[i].m_4);
                while (veg) {
                    if (veg->m_alpha == 64) {
                        AddToBufferOf2DVegetation(veg);
                    } else if (veg->m_alpha == 0) {
                        AddToBufferOf3DVegetation(veg);
                    } else {
                        if (veg->m_alpha > 48)
                            AddToBufferOf2DVegetation(veg);
                        else
                            AddToBufferOf2D3DVegetation(veg);
                        AddToBufferOf3DVegetation(veg);
                    }
                    // Stamped with the frame counter read as unsigned (lhu).
                    if (veg->m_next == 0xFFFF) {
                        veg->m_next = (unsigned short)*(unsigned int *)&m_x5B4;
                        break;
                    }
                    int next = veg->m_next;
                    veg->m_next = (unsigned short)*(unsigned int *)&m_x5B4;
                    veg = &m_vegetation[next];
                }
            }
        }
    }
    static int PrepareGeometryID3 = -1;
    for (i = 0; i < 256; i++) {
        if (m_descriptors[i]) {
            SurfaceMaterial *material = (SurfaceMaterial *)LpMaterialStringTable->GetPointer(
                m_descriptors[i]->m_lods->m_geometries->m_materialIndex);
            TextureMap *texture = (TextureMap *)LpTextureMapStringTable->GetPointer(material->m_shaders[0]->m_textures[0]);
            if (texture && 10.0f > texture->m_usage)
                texture->m_usage = 10.0f;
            m_descriptors[i]->m_shader = (PSXSurfaceShaderInstance *)LpSurfaceShaderList->m_shaders[material->m_instances[0]];
            m_descriptors[i]->m_geometry = m_descriptors[i]->m_lods->m_geometries;
        }
    }
#ifdef SPLASHDOWN_PC
    // x86-64: the vptr comes first, not m_materialIndex
    SurfaceMaterial *material = (SurfaceMaterial *)LpMaterialStringTable->GetPointer(m_2DGeometry->m_materialIndex);
#else
    SurfaceMaterial *material = (SurfaceMaterial *)LpMaterialStringTable->GetPointer(*(int *)m_2DGeometry);
#endif
    TextureMap *texture = (TextureMap *)LpTextureMapStringTable->GetPointer(material->m_shaders[0]->m_textures[0]);
    if (texture && 10.0f > texture->m_usage)
        texture->m_usage = 10.0f;
    m_2DShader = (PSXSurfaceShaderInstance *)LpSurfaceShaderList->m_shaders[material->m_instances[0]];
    return 1;
}
#pragma fast_fptosi reset


// ---- src/EcoSystem/Render3DVegetation.cpp
class VifDataItem;


extern int TotalRendered3D;


void EcoSystem::Render3DVegetation(Vegetation **vegetation, int count)
{
    for (int i = 0; i < count; i++)
        vegetation[i]->Render(m_vifItems[i]);
    TotalRendered3D += count;
}


// ---- src/EcoSystem/Render2DVegetation.cpp
#pragma fast_fptosi on

struct _sceDmaTag;
class VifDataItem;
class SurfaceGeometry;






void EcoSystem::Render2DVegetation(Vegetation **vegetation, int count, PSXSurfaceGeometry *geometry,
                                   VifDataItem *start, VifDataItem *end)
{
    DmaRenderItem *item = geometry->m_renderItem;
    item->Start((DmaRenderDestination)1, count * 12);
    item->DmaAddVifCode(0x10000000);
    unsigned int *data = &item->m_dataList[item->m_dataListCurrentSize];
    for (int i = 0; i < count; i++) {
        Vegetation *v = vegetation[i];
        data[0] = 0x6C028000;
        data[1] = v->m_x;
        data[2] = v->m_y;
        data[3] = v->m_z;
        int size = 1.33333337f * (unsigned int)v->m_alpha;
        if (size > 64)
            size = 64;
        data[4] = size;
        data[5] = v->m_radius;
        data[6] = v->m_height;
        data[7] = v->m_radius;
        data[8] = v->m_type * 3;
        data[9] = 0x17000000;
        data += 10;
    }
    *data = 0x11000000;
    item->m_dataListCurrentSize += count * 10 + 1;
    item->End();
    // Retail null-checks only the end tag (a redundant `end ? end : 0`).
    m_2DShader->AddGeometry((SurfaceGeometry *)geometry, (_sceDmaTag *)start,
                            end ? (_sceDmaTag *)end : 0);
}
#pragma fast_fptosi reset


// ---- src/EcoSystem/Render3D.cpp
class Vegetation;
class VifDataItem;
class PSXSurfaceGeometry;


extern Game *LpGame;

extern RenderTarget *LpRenderTarget;

extern int RenderEcoSystem;
extern int Render2DEcoSystem;
extern int Render2D3DEcoSystem;
extern int Render3DEcoSystem;
extern int TotalRendered2D;
extern int TotalRendered3D;



int EcoSystem::Render3D()
{
    if (!RenderEcoSystem)
        return 1;
    TotalRendered3D = 0;
    TotalRendered2D = 0;
    if (m_numVisible2D || m_numVisible2D3D)
        Calculate2DParameters();
    if (Render2DEcoSystem && m_numVisible2D) {
        static int Render2DVegID = -1;     // unused profiler ID
        Render2DVegetation(m_visible2D, m_numVisible2D, m_2DGeometry, m_2DItem, 0);
    }
    if (Render2D3DEcoSystem && m_numVisible2D3D) {
        static int Render2D3DVegID = -1;   // unused profiler ID
        Render2DVegetation(m_visible2D3D, m_numVisible2D3D, m_2D3DGeometry, m_2DItem, m_2DTweakItem);
    }
    if (Render3DEcoSystem && m_numVisible3D) {
        static int Render3DVegID = -1;     // unused profiler ID
        Render3DVegetation(m_visible3D, m_numVisible3D);
    }
    LpRenderTarget->TrySendRenderQueue(0);
    if (LpGame->m_debugOverlay) {
        if (OverlayId < 0)
            OverlayId = LpGame->m_debugOverlay->m_numPages++;
        LpGame->m_debugOverlay->PrintfHeading(OverlayId, "Ecosystem:%d objects", m_maxVegetation);
        LpGame->m_debugOverlay->PrintfDetail(OverlayId, "Visible  2D %d 3D %d", m_numVisible2D, m_numVisible3D);
        LpGame->m_debugOverlay->PrintfDetail(OverlayId, "Rendered 2D %d 3D %d", TotalRendered2D, TotalRendered3D);
        LpGame->m_debugOverlay->PrintfDetail(OverlayId, "Memory %d", 0);
    }
    return 1;
}


// ---- src/EcoSystem/KeyDownHandler.cpp
class ProcessNode;
class KeyInfo;
enum DeviceType {};

int KEYDOWN_HANDLER_TEST(unsigned int key, DeviceType device, ProcessNode *node, unsigned int flags);

extern int RenderEcoSystem;



int EcoSystem::KeyDownHandler(ProcessNode *node, KeyInfo *key)
{
    if (GameObject::KeyDownHandler(node, key))
        return 1;
    if (KEYDOWN_HANDLER_TEST(0x20, (DeviceType)0, node, 0x80)) {
        RenderEcoSystem = 1 - RenderEcoSystem;
        return 1;
    }
    return 0;
}

