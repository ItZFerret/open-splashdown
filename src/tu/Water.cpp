// Water.cpp  @ 0x002F6760 (text 0x2F6760-0x2F7EF0)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's Water.cpp as one translation unit: its functions in
// retail order, then its static initializer. `make link` places the
// object's data as well as its code (tools/link_c.py).



#include "GameObject/GameObject.h"
#include "Water/Water.h"
#include "SearchableObject/SearchableObject.h"
#include "SoultreeObject/SoultreeObject.h"
#include "PSXSignatureWave/PSXSignatureWave.h"
#include "d3dtypes.h"
#include "PSXTextureMap/PSXTextureMap.h"
#include "WaterWakePatch/WaterWakePatch.h"
#include "PSXWaterDisplacement/PSXWaterDisplacement.h"
#include "Game/Game.h"
// The axis vectors every Soultree TU defines (and __sinit constructs).
#include "sinit/AxisVectors.h"

// retail never calls fptosi: float->int is an inline cvt.w.s
#pragma fast_fptosi on
// Three of the old files had inline_depth(4). A whole TU needs one value
// throughout (the value at the end of the file also applies to functions
// before it), and every function matches at 4.
#pragma inline_depth(4)

typedef int int128_t __attribute__((mode(TI)));

extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" int strcasecmp(const char *a, const char *b);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);
extern "C" void *memset(void *dst, int c, unsigned int n);
extern "C" double fmod(double x, double y);
void PSXprintf(const char *fmt, ...);
int BytesPerPixel(int format);
float FastSin(float x);

extern char WaterContentDirectory[];
extern char SceneContentDirectory[];
extern char BaseWaterContentDirectory[];
extern Water *GameWater;
extern Game *LpGame;
int gComputeWake = 1;
float gWaterScale = 1.0f;
extern float gfBaseTextureConstant;
extern float gWakeScale;
extern float gBaseScale;
extern int gbRenderInfinitePlane;
extern float gfOriginalBaseHeight;
extern float gPoolDepth;
float gWakeForce = 2000.0f;
short __attribute__((section(".sdata"))) VELOCITY_SCALE = 3;
extern float gfTotalTime;
extern int gComputeWake;
extern float gfWaterBaseSwellPeriod;
extern float gfWaterBaseSwellHeight;

// Defined here for its retail .bss slot.
Water *GameWater;
float gWakeScale;
float gBaseScale;
float gPoolDepth;

// The XZ diagonal, set by the static initializer with the axis vectors (in
// .bss, so not const); its .bss precedes Tick's static in retail.
_D3DVECTOR XZVector(1.0f, 0.0f, 1.0f);

// Defined here for its retail .bss slot.
float gfTotalTime;


// ---- src/Water/ctor.cpp

Water::Water(int enabled) : SearchableGameObject(enabled)
{
    m_searchVisible = 1;
    m_addToList = 1;
    m_lights = 0;
    m_wakePatches = 0;
    m_firstPatch = 0;
    m_lastPatch = 0;
    m_baseMap = 0;
    m_x50 = 0;
    m_texture = 0;
    m_waves = 0;
    m_geometry = 0;
    m_numWakePatches = 0;
    m_objects = 0;
    for (int i = 0; i < 5; i++)
        m_x70[i] = 0;
    m_numWaves = 0;
    m_weights[0] = 0;
    m_weights[1] = 0;
    m_weights[2] = 0;
    m_weights[3] = 0;
    m_baseMapStride = 0;
    sprintf(WaterContentDirectory, "%s%s", SceneContentDirectory, BaseWaterContentDirectory);
}


// ---- src/Water/dtor.cpp

Water::~Water()
{
    if (m_wakePatches) {
        for (int i = 0; i < m_numWakePatches; i++) {
            if (m_wakePatches[i].m_texture)
                m_wakePatches[i].m_texture->Release();
            if (m_wakePatches[i].m_vertices[0])
                free(m_wakePatches[i].m_vertices[0]);
            if (m_wakePatches[i].m_vertices[1])
                free(m_wakePatches[i].m_vertices[1]);
        }
        delete[] m_wakePatches;
        m_wakePatches = 0;
    }
    if (m_x50)
        m_x50->Release();
    if (m_waves) {
        for (unsigned int i = 0; i < m_numWaves; i++) {
            delete m_waves[i];
            m_waves[i] = 0;
        }
        delete[] m_waves;
        m_waves = 0;
    }
    if (m_objects) {
        SoultreeObject **object = m_objects;
        while (*object)
            (*object++)->Release();
        delete[] m_objects;
        m_objects = 0;
    }
}


// ---- src/Water/Create.cpp
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

// Loads the water (a .wtt parameter block or a binary file), then allocates
// the wake patches: two vertex buffers and a 128x128 texture each.
Water *Water::Create(char *name, LightManager *lights, int numWakePatches, int patchSize, float scale)
{
    m_lights = lights;
    m_numWakePatches = numWakePatches;
    m_patchHeight = patchSize / 8;
    m_patchWidth = m_patchHeight * 8;
    m_wakeScroll = 0.5f * m_patchWidth;
    if (!strcasecmp(FindExtension(name), ".wtt")) {
        if (!LoadWaterFromParameterBlockFile(name)) {
            PSXprintf("Could not load file %s in Water::Create\n", name);
            return 0;
        }
    } else {
        if (!LoadWaterFromBinaryFile(name)) {
            PSXprintf("Could not load file %s in Water::Create\n", name);
            return 0;
        }
    }
    GameObject::Create();
    m_wakePatches = new WaterWakePatch[numWakePatches];
    if (!m_wakePatches) {
        PSXprintf("Insufficient memory to create wake patch bucket in Water::Create\n");
        return 0;
    }
    for (int i = 0; i < numWakePatches; i++) {
        m_wakePatches[i].m_vertices[0] = (WaterVertex *)malloc(m_patchHeight * m_patchWidth * sizeof(WaterVertex));
        if (!m_wakePatches[i].m_vertices[0]) {
            PSXprintf("Insufficient memory to create WaterWakeData for bucket %d in Water::Create\n", i);
            return 0;
        }
        m_wakePatches[i].m_vertices[1] = (WaterVertex *)malloc(m_patchHeight * m_patchWidth * sizeof(WaterVertex));
        if (!m_wakePatches[i].m_vertices[1]) {
            PSXprintf("Insufficient memory to create WaterWakeData for bucket %d in Water::Create\n", i);
            return 0;
        }
        m_wakePatches[i].m_texture = new PSXTextureMap(1);
        m_wakePatches[i].m_texture->Load(0, 0x80, 0x80, 0x80, 0x80, 0x22B8, 0x22B8, 0, 0, 0, 1, 0,
                                         (_D3DBLEND)5, (_D3DBLEND)6, 0, 0xFF00FF);
        ResetWakePatch(&m_wakePatches[i], ZeroVector, scale);
        PSXTextureMap *texture = m_wakePatches[i].m_texture;
        if (9.0f > texture->m_usage)
            texture->m_usage = 9.0f;
    }
    m_firstPatch = m_wakePatches;
    m_lastPatch = &m_wakePatches[numWakePatches - 1];
    m_heightScale = (1.0f / 4096.0f) * gWaterScale;
    long pitch;
    m_wakeBits = m_wakePatches[0].m_texture->LockSys(0, &pitch, 0);
    m_wakePitch = (int)pitch / 4;       // retail reads only the low word
    m_wakePatches[0].m_texture->UnlockSys(0);
    GameWater = this;
    return this;
}


// ---- src/Water/ResetWakePatch.cpp
// Clears a wake patch's height buffers and fills its texture with neutral grey.
void Water::ResetWakePatch(WaterWakePatch *patch, _D3DVECTOR position, float scale)
{
    patch->m_current = 0;
    patch->m_position = position;
    patch->m_scrollOrigin = position;
    patch->m_scrollScale = scale;
    patch->m_invScrollScale = 1.0f / patch->m_scrollScale;
    memset(patch->m_vertices[0], 0, m_patchHeight * m_patchWidth * sizeof(WaterVertex));
    memset(patch->m_vertices[1], 0, m_patchHeight * m_patchWidth * sizeof(WaterVertex));
    long pitch;
    unsigned int *bits = (unsigned int *)patch->m_texture->LockSys(0, &pitch, 0);
    if (bits) {
        for (int i = 0; i < 0x4000; i++)
            bits[i] = 0xFF7F7F7F;
    }
    patch->m_texture->UnlockSys(0);
}


// ---- src/Water/ResetWakes.cpp
void Water::ResetWakes()
{
    WaterWakePatch *patch = m_firstPatch;
    do {
        ResetWakePatch(patch, ZeroVector, patch->m_scrollScale);
        patch++;
    } while (patch <= m_lastPatch);
}


// ---- src/Water/setWaterDisplacement.cpp
void Water::setWaterDisplacement(PSXWaterDisplacement *displacement)
{
    m_displacement = displacement;
    if (displacement->m_texture) {
        long pitch;
        m_baseMap = (unsigned char *)displacement->m_texture->LockSys(0, &pitch, 0);
        m_baseMapStride = BytesPerPixel(displacement->m_texture->m_format);
        m_baseMapSize = (int)pitch / m_baseMapStride;
        m_baseMapScale = m_baseMapSize / gfBaseTextureConstant;
        displacement->m_texture->UnlockSys(0);
    }
}


// ---- src/Water/GetGridPointIntersection.cpp
// Sets point->y to the water height at (x, z): wakes, the base height map
// and the signature waves.
void Water::GetGridPointIntersection(_D3DVECTOR *point, int *index)
{
    float x;
    float z;
    float wake;
    float base;
    int px;
    int pz;
    int tile;
    WaterWakePatch *patch;
    int i;
    x = point->x;
    z = point->z;
    wake = 0.0f;
    for (i = 0; i < m_numWakePatches; i++) {
        patch = &m_wakePatches[i];
        px = patch->m_invScrollScale * (x - patch->m_position.x);
        pz = patch->m_invScrollScale * (z - patch->m_position.z);
        if (px >= 0 && px < m_patchWidth && pz >= 0 && pz < m_patchWidth) {
            tile = px >> 3;
            wake += gWakeScale * patch->m_vertices[patch->m_current][tile + pz * m_patchHeight]
                                     .height[px - (tile << 3)];
        }
    }
    if (m_baseMap) {
        int bx = (int)(x * m_baseMapScale) % m_baseMapSize;
        int bz = (int)(z * m_baseMapScale) % m_baseMapSize;
        base = gBaseScale * (unsigned int)m_baseMap[m_baseMapStride * (bx + bz * m_baseMapSize)];
    } else {
        base = 0.0f;
    }
    if (!gbRenderInfinitePlane && base == 0.0f) {
        point->y = 0.0f;
        return;
    }
    float height = m_displacement->GetSignatureWaveHeight(x, z, index);
    height = m_heightScale * (height + (wake + base));
    if (base == 0.0f)
        height += m_baseHeight;
    else
        height += gfOriginalBaseHeight;
    if (height - point->y > gPoolDepth)
        point->y = m_baseHeight;
    else
        point->y = height;
}


// ---- src/Water/ApplyExternalForce.cpp
// Round 27: matched. Retail compares the patch *pointer* against m_x48 (sltu), as written here.
// Retail's fmin/fmax: min.s/max.s into $f8, stored through a stack temporary.
static inline float Fmin(float a, float b)
{
    float r;
    asm("min.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}

static inline float Fmax(float a, float b)
{
    float r;
    asm("max.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}

// Adds a scaled force to one grid point. Written as an inline (not `dst += v`, which
// narrows v to short first): the argument is evaluated before dst is read, which gives
// retail's register numbering for the load/shift/add in each arm.
static inline void AddForce(short &dst, int v)
{
    dst = dst + v;
}

void Water::ApplyExternalForce(float force, _D3DVECTOR position)
{
    WaterVertex *base;
    int v;
    WaterWakePatch *patch;
    int x;
    int col;
    WaterVertex *cell;
    int z;
    int sub;
    int i;
    i = 0;
    _D3DVECTOR local;
    _D3DVECTOR origin;
    do {
        patch = &m_wakePatches[i];
        origin = patch->m_position;
        local = (position - origin) * patch->m_invScrollScale;
        x = (int)local.x;
        z = (int)local.z;
        if (x >= 8 && x < m_patchWidth - 10 && z >= 9 && z < m_patchWidth - 10) {
            float w11;
            float w01;
            float w10;
            float fz;
            float gx;
            float gz;
            float w00;
            float fx;
            float f;
            fx = local.x - x;
            fz = local.z - z;
            w11 = fx * fz;
            gz = 1.0f - fz;
            gx = 1.0f - fx;
            w01 = gx * fz;
            w10 = fx * gz;
            w00 = gx * gz;
            // One variable for the clamp and the scaled force: retail keeps both in one register.
            f = Fmin(Fmax(force, -1.0f), 1.0f);
            f = gWakeForce * f;
            // m_current is read before col/sub (a separate base pointer), as retail schedules it.
            base = patch->m_vertices[patch->m_current];
            col = x >> 3;
            sub = x - (col << 3);
            cell = base + z * m_patchHeight + col;
            // The first point is written out with a named v rather than AddForce(): a named
            // local numbers above sub, which is what gives sub/i retail's $a2/$a3.
            v = (int)(w11 * f) << VELOCITY_SCALE;
            cell->force[sub] = cell->force[sub] + v;
            if (sub < 7) {
                AddForce(cell->force[sub + 1], (int)(w01 * f) << VELOCITY_SCALE);
                cell += m_patchHeight;
                AddForce(cell->force[sub], (int)(w10 * f) << VELOCITY_SCALE);
                AddForce(cell->force[sub + 1], (int)(w00 * f) << VELOCITY_SCALE);
            } else {
                cell++;
                AddForce(cell->force[0], (int)(w01 * f) << VELOCITY_SCALE);
                cell += m_patchHeight;
                AddForce(cell->force[0], (int)(w00 * f) << VELOCITY_SCALE);
                cell--;
                AddForce(cell->force[7], (int)(w10 * f) << VELOCITY_SCALE);
            }
            i++;
        } else {
            i++;
        }
    } while ((unsigned int)patch < (unsigned int)m_lastPatch);
}


// ---- src/Water/ScrollWakePatch.cpp
void Water::ScrollWakePatch(int index, _D3DVECTOR position)
{
    if (index >= m_numWakePatches || index < 0)
        return;
    WaterWakePatch *patch = &m_wakePatches[index];
    patch->m_scrollOrigin = position - (m_wakeScroll * patch->m_scrollScale) * XZVector;
}


// ---- src/Water/GetBaseTime.cpp
float Water::GetBaseTime()
{
    return gfTotalTime;
}


// ---- src/Water/SetBaseTime.cpp
void Water::SetBaseTime(float time)
{
    gfTotalTime = time;
}


// ---- src/Water/Tick.cpp
// Same algorithm as the matched UIWaterPatch::Tick plus the wake texture writes.
// Codegen notes (this function has inline asm, so the global optimizer is off):
// - `dst` holds the uncached destination buffer before the row loop (retail reuses the
//   variable as the buffer base). One variable, one vreg: its loop range interferes with
//   the whole inner-loop clique, so the colourer pushes it late, pops it early and gives it
//
//   a CSE'd UNCACHED(...) is coloured after H*9 and `end` and lands in $a3/$t0).
// - end/skip/srcRow/dstRow in UIWaterPatch's order: H*W is the first multiply (`mult`), and
//   srcRow's dz*H is generated before the first skip<<6.
// - The inner loop's 20 registers are one clique coloured in vreg order: the big asm sits in
//   an inline function (WakeCell) so its operands number low-to-high, and named copies of the
// Clamp/Deadband addresses fix their rematerialisation order.
// - `move $9, $8` assembles to addu here; retail has paddub $9, $8, $0 (written out).
static inline float Fabs(float x)
{
    float r;
    asm("abs.s $f8, %1; swc1 $f8, 0(%0)" : : "r"(&r), "f"(x) : "$f8", "memory");
    return r;
}

static inline int Max(int a, int b) { return a > b ? a : b; }
static inline int Min(int a, int b) { return a < b ? a : b; }

#define UNCACHED(p) ((WaterVertex *)PS2_ADDR((unsigned int)(p) | EE_UNCACHED_ACCEL))

static inline void FlushRow(int128_t *carry, WaterVertex *prev)
{
    asm("mtsah $0, 1; lq $11, 0x0(%0); qfsrv $10, $0, $11; sq $10, 0x20(%1)"
            :
            : "r"(carry), "r"(prev)
            : "$10", "$11", "memory");
}

static inline void WakeCell(short *Deadband3, short *Deadband2, short *Deadband1, short *Deadband0, short *Clamp3, short *Clamp2, unsigned int *texHi, unsigned int *texLo, short *MinusOnes, short *Clamp1, WaterVertex *dst, short *Constant1, WaterVertex *above, short *Constant0, short *Clamp0, WaterVertex *below, WaterVertex *src)
{
    asm(".set noreorder; lq $8, 0x10(%16); lq $9, 0x20(%16); lq $10, 0x0(%15); lq $11, 0x0(%12); paddsh $12, $8, $9; paddsh $13, $10, $11; paddsh $8, $12, $13; lq $9, 0x0(%13); pmulth $10, $8, $9; lq $12, 0x0(%16); lq $13, 0x0(%11); pmaddh $10, $12, $13; lq $14, 0x30(%16); psrah $13, $14, 6; lq $9, 0x0(%8); pmaddh $10, $13, $9; pmflo $8; pmfhi $9; pmfhl.sh $8; psrah $8, $8, 2; paddsh $14, $8, $14; lq $10, 0x0(%5); lq $11, 0x0(%4); pmaxh $14, $14, $10; pminh $14, $14, $11; psrah $8, $14, 8; paddsh $12, $12, $8; lq $8, 0x0(%14); lq $9, 0x0(%9); pmaxh $12, $12, $8; pminh $12, $12, $9; lq $8, 0x0(%3); lq $9, 0x0(%2); pcgth $8, $12, $8; pcgth $9, $9, $12; pmulth $8, $8, $9; pmfhl.sh $8; lq $10, 0x0(%1); lq $11, 0x0(%0); pcgth $10, $14, $10; pcgth $11, $11, $14; pmulth $10, $10, $11; pmfhl.sh $10; pmulth $8, $8, $10; pmfhl.sh $8; pmulth $9, $8, $12; paddub $9, $8, $0; psubh $12, $12, $9; pmulth $9, $8, $14; paddub $9, $8, $0; psubh $14, $14, $9; sq $12, 0x0(%10); sq $14, 0x30(%10); lq $9, 0x0(%9); paddsh $9, $12, $9; pextlh $10, $0, $9; sq $10, 0x0(%7); pextuh $10, $0, $9; sq $10, 0x0(%6); .set reorder"
                    :
                    : "r"(Deadband3), "r"(Deadband2), "r"(Deadband1), "r"(Deadband0), "r"(Clamp3), "r"(Clamp2), "r"(texHi), "r"(texLo), "r"(MinusOnes), "r"(Clamp1), "r"(dst), "r"(Constant1), "r"(above), "r"(Constant0), "r"(Clamp0), "r"(below), "r"(src)
                    : "$8", "$9", "$10", "$11", "$12", "$13", "memory");
}

int Water::Tick(float dt)
{
    if (LpGame->m_paused)
        return 1;
    if (!gComputeWake)
        return GameObject::Tick(dt);
    static int WakeSimID = -1;   // unused profiler ID
    static short Clamp0[8] __attribute__((aligned(16))) = { -127, -127, -127, -127, -127, -127, -127, -127 };
    static short Clamp1[8] __attribute__((aligned(16))) = { 127, 127, 127, 127, 127, 127, 127, 127 };
    static short Clamp2[8] __attribute__((aligned(16))) = { -16000, -16000, -16000, -16000, -16000, -16000, -16000, -16000 };
    static short Clamp3[8] __attribute__((aligned(16))) = { 16000, 16000, 16000, 16000, 16000, 16000, 16000, 16000 };
    static short Deadband0[8] __attribute__((aligned(16))) = { -5, -5, -5, -5, -5, -5, -5, -5 };
    static short Deadband1[8] __attribute__((aligned(16))) = { 5, 5, 5, 5, 5, 5, 5, 5 };
    static short Deadband2[8] __attribute__((aligned(16))) = { -80, -80, -80, -80, -80, -80, -80, -80 };
    static short Deadband3[8] __attribute__((aligned(16))) = { 80, 80, 80, 80, 80, 80, 80, 80 };
    static short MinusOnes[8] __attribute__((aligned(16))) = { -1, -1, -1, -1, -1, -1, -1, -1 };
    static short Constant0[8] __attribute__((aligned(16))) = { 75, 75, 75, 75, 75, 75, 75, 75 };
    static short Constant1[8] __attribute__((aligned(16))) = { -301, -301, -301, -301, -301, -301, -301, -301 };
    WaterWakePatch *patch;
    int next;
    unsigned int *tex;
    WaterVertex *srcRow;
    WaterVertex *end;
    int dx;
    int dz;
    int cur;
    int skip;
    WaterVertex *src;
    WaterVertex *dst;
    WaterVertex *dstRow;
    WaterVertex *rowEnd;
    WaterVertex *below;
    WaterVertex *above;
    WaterVertex *prev;
    unsigned int *texLo;
    unsigned int *texHi;
    unsigned int *texels;

    gfTotalTime += dt;
#ifdef SPLASHDOWN_PC
    // The EE's FPU divides by zero to +-FLT_MAX, not inf/NaN: Loch Ness and
    // Bali have no swell (period 0, height 0), and 0 * sin(NaN) made the
    // water's height, then every jetski's position, NaN.
    {
        float phase = gfTotalTime / gfWaterBaseSwellPeriod;
        if (gfWaterBaseSwellPeriod == 0.0f)
            phase = (gfTotalTime < 0.0f) != (__builtin_signbit(gfWaterBaseSwellPeriod) != 0) ? -3.40282347e38f : 3.40282347e38f;
        m_baseHeight = gfOriginalBaseHeight + gfWaterBaseSwellHeight * FastSin(fmod(phase, 6.28318548f));
    }
#else
    m_baseHeight = gfOriginalBaseHeight + gfWaterBaseSwellHeight * FastSin(fmod(gfTotalTime / gfWaterBaseSwellPeriod, 6.28318548f));
#endif
    _D3DVECTOR offset;
    int128_t carry;
    patch = m_firstPatch;
    do {
        offset = (patch->m_scrollOrigin - patch->m_position) * patch->m_invScrollScale;
        if (!(Fabs(offset.x) <= m_patchWidth) || !(Fabs(offset.z) <= m_patchWidth)) {
            offset = ZeroVector;
            patch->m_position = patch->m_scrollOrigin;
        }
        dx = (int)offset.x >> 3;
        dz = (int)offset.z >> 3;
        dx = Max(dx, -1);
        dx = Min(dx, 1);
        dz = Max(dz, -1);
        dz = Min(dz, 1);
        dz <<= 3;
        texels = (unsigned int *)PS2_ADDR(((unsigned int)patch->m_texture->LockSys(0, 0, 0) | EE_UNCACHED_ACCEL));
        cur = patch->m_current;
        next = (cur + 1) % 2;
        src = patch->m_vertices[cur];
        tex = texels + 9 * 128 + 8;
        dst = UNCACHED(patch->m_vertices[next]);    // dst doubles as the buffer base (see top)
        end = UNCACHED(dst + m_patchHeight * m_patchWidth);
        skip = m_patchHeight * 9 + 1;
        srcRow = src + skip + dz * m_patchHeight + dx;
        dstRow = UNCACHED(dst + skip);
        rowEnd = UNCACHED(dstRow + m_patchHeight - 2);
        end -= skip;
        while (dstRow < end) {
            src = srcRow;
            dst = dstRow;
            below = src + m_patchHeight;
            texLo = tex;
            texHi = texLo + 4;
            above = src - m_patchHeight;
            prev = dst - 1;
            asm("sq $0, 0x0(%0)" : : "r"(&carry) : "memory");
            while (dst < rowEnd) {
                short *pConstant0 = Constant0;
                short *pConstant1 = Constant1;
                short *pClamp0 = Clamp0;
                short *pClamp1 = Clamp1;
                short *pMinusOnes = MinusOnes;
                short *pClamp2 = Clamp2;
                short *pClamp3 = Clamp3;
                short *pDeadband0 = Deadband0;
                short *pDeadband1 = Deadband1;
                short *pDeadband2 = Deadband2;
                short *pDeadband3 = Deadband3;
                WakeCell(pDeadband3, pDeadband2, pDeadband1, pDeadband0, pClamp3, pClamp2, texHi, texLo, pMinusOnes, pClamp1, dst, pConstant1, above, pConstant0, pClamp0, below, src);
                asm(".set noreorder; lq $11, 0x0(%0); mtsah $0, 1; qfsrv $10, $12, $11; sq $10, 0x20(%2); nop; mtsah $0, 15; qfsrv $10, $12, $11; sq $10, 0x10(%1); sq $12, 0x0(%0); .set reorder"
                    :
                    : "r"(&carry), "r"(dst), "r"(prev)
                    : "$10", "$11", "memory");
                // pad +0x2E of the previous cell (the first cell of the row)
                if (dst == dstRow)
                    *(short *)((char *)prev + 0x2E) = 0;
                texLo += 8;
                texHi += 8;
                below++;
                above++;
                prev++;
                src++;
                dst++;
            }
            FlushRow(&carry, prev);
            dstRow += m_patchHeight;
            srcRow += m_patchHeight;
            tex += 128;
            rowEnd += m_patchHeight;
        }
        patch->m_position += _D3DVECTOR((float)(dx << 3), 0.0f, (float)dz) * patch->m_scrollScale;
        patch->m_current = next;
        patch->m_texture->UnlockSys(0);
        patch++;
    } while (patch <= m_lastPatch);
    return 1;
}
