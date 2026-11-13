// UIWater.cpp  @ 0x00374730 (text 0x374730-0x375590)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's UIWater.cpp as one translation unit: an unanchored TU at the end of
// JetskiStuntsKey.cpp's tu_map row, after Clut.cpp. Its .rodata holds the
// "UIWater::Create" messages (@770-) and the UIWaterPatch RTTI;
// __sinit_UIWater.cpp is the .ctor slot after JetskiStuntsKey.cpp's.
// UIWaterPatch, then the static initializer.
//
// Retail compiled this file at optimization level 1 (no scheduling, no constant
// reuse; the per-function files approximated it with schedule/opt_* pragmas);
// Clut.cpp before it is fully optimized.


#include "Clut4/Clut4.h"
#include "Clut8/Clut8.h"
#include "DataAccess/DataAccess.h"
#include "UIWaterPatch/UIWaterPatch.h"
#include "d3dtypes.h"
#include "GameObject/GameObject.h"

extern "C" void *malloc(unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);
extern "C" void free(void *p);
void PSXprintf(const char *fmt, ...);

extern DataAccess *LpDataAccess;

// The axis vectors (the static initializer constructs them; not const, or
// mwcc puts them in .rodata), copy-initialised
// from temporaries: the low optimization level's form.
static _D3DVECTOR ZeroVector = _D3DVECTOR(0.0f, 0.0f, 0.0f);
static _D3DVECTOR XVector = _D3DVECTOR(1.0f, 0.0f, 0.0f);
static _D3DVECTOR YVector = _D3DVECTOR(0.0f, 1.0f, 0.0f);
static _D3DVECTOR ZVector = _D3DVECTOR(0.0f, 0.0f, 1.0f);

float UIWaterPatch::gWakeForce = 2000.0f;
float UIWaterPatch::HeightScale = 0.01968504f;
short UIWaterPatch::VELOCITY_SCALE = 3;

#pragma optimization_level 1

// ---- src/UIWaterPatch/ctor.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order), without copy propagation and,
// as the ctor shows, without common-subexpression elimination.


UIWaterPatch::UIWaterPatch(int enabled)
    : GameObject(enabled)
{
    m_scaleX = 0;
    m_invScaleX = 0;
    m_scaleZ = 0;
    m_invScaleZ = 0;
    m_position.x = m_position.y = m_position.z = 0.0f;
    m_field50 = 0;
    m_current = 1;
    m_buffers[0] = m_buffers[1] = 0;
    m_height = 0;
    m_width = 0;
    m_halfHeight = 0;
}


// ---- src/UIWaterPatch/Create.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order), copy propagation, common
// subexpressions, strength reduction or loop-invariant hoisting.







UIWaterPatch *UIWaterPatch::Create(_D3DVECTOR position, int size, float scaleX, float scaleZ)
{
    GameObject::Create();
    m_position = position;
    m_width = (size + 40) / 8;
    m_height = m_width * 8;
    m_halfHeight = 0.5f * m_height;
    m_scaleX = scaleX;
    m_invScaleX = 1.0f / m_scaleX;
    m_scaleZ = scaleZ;
    m_invScaleZ = 1.0f / m_scaleZ;
    m_buffers[0] = (WaterPatchCell *)malloc(m_width * m_height * sizeof(WaterPatchCell));
    if (!m_buffers[0]) {
        PSXprintf("Insufficient memory to create UIWaterData for bucket 0 in UIWater::Create\n");
        return 0;
    }
    m_buffers[1] = (WaterPatchCell *)malloc(m_width * m_height * sizeof(WaterPatchCell));
    if (!m_buffers[1]) {
        PSXprintf("Insufficient memory to create UIWaterData for bucket 1 in UIWater::Create\n");
        return 0;
    }
    Reset();
    return this;
}


// ---- src/UIWaterPatch/dtor.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order), without copy propagation and
// without common-subexpression elimination.



struct WaterPatchCell;


UIWaterPatch::~UIWaterPatch()
{
    if (m_buffers[0])
        free(m_buffers[0]);
    if (m_buffers[1])
        free(m_buffers[1]);
}


// ---- src/UIWaterPatch/Reset.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order) and without copy propagation.



void UIWaterPatch::Reset()
{
    m_field50 = 0;
    m_current = 1;
    memset(m_buffers[0], 0, m_width * m_height * sizeof(WaterPatchCell));
    memset(m_buffers[1], 0, m_width * m_height * sizeof(WaterPatchCell));
}


// ---- src/UIWaterPatch/GetHeight.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order) and without copy propagation.


float UIWaterPatch::GetHeight(int x, int y)
{
    y += 9;
    x += 8;
    int cell = x >> 3;
    int sub = x - (cell << 3);
    short h = m_buffers[m_current][cell + y * m_width].height[sub];
    return h * HeightScale;
}


// ---- src/UIWaterPatch/GetHeight_00374B00.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order), copy propagation, common
// subexpressions, strength reduction or loop-invariant hoisting (the last two
// show here as the unrotated inner loop).


// Copies one row of heights (8 per cell) into out.
int UIWaterPatch::GetHeight(int y, float *out)
{
    int result = 0;
    y += 9;
    int x = 1;
    WaterPatchCell *cell = &m_buffers[m_current][x + y * m_width];
    short *h;
    int i;
    for (i = 0; i < m_width; i++, cell++) {
        h = cell->height;
        for (int j = 0; j < 8; j++, h++, out++)
            *out = *h * HeightScale;
    }
    return result;
}


#pragma fast_fptosi on

// ---- src/UIWaterPatch/ApplyExternalForce.cpp
// ApplyExternalForce__12UIWaterPatchFf10_D3DVECTOR  @ 0x00374BB0
// The function compiles optimizer-off (Fmin/Fmax are asm helpers), where one
// variable is one register: retail reuses fz for the w01 weight and fx for the
// clamp limit, the clamped force and every scaled delta, which is what gives
// fz's $f4 and fx's $f5 their later values. The pragmas are the file's (see
// GetHeight.cpp). The shift is the 2-byte $gp static UIWaterPatch::VELOCITY_SCALE,
// and the force is added in float (short += float).

// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order) and without copy propagation.


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




void UIWaterPatch::ApplyExternalForce(float force, _D3DVECTOR position)
{
    WaterPatchCell *base;
    float gx;
    float gz;
    int z;
    float w11;
    float fz;
    WaterPatchCell *cell;
    float fx;
    _D3DVECTOR p;
    float w10;
    float w00;
    float f;
    int x;
    int col;
    p = position - m_position;
    p.x *= m_invScaleX;
    p.z *= m_invScaleZ;
    x = (int)p.x;
    z = (int)p.z;
    z += 9;
    x += 8;
    if (x >= 8 && x < m_height - 10 && z >= 9 && z < m_height - 10) {
        fx = p.x - x;
        fz = p.z - z;
        gx = 1.0f - fx;
        gz = 1.0f - fz;
        w11 = fx * fz;
        fz = gx * fz;
        w10 = fx * gz;
        w00 = gx * gz;
        fx = 10.0f;
        fx = Fmin(Fmax(force, -fx), fx);
        f = gWakeForce * fx;
        base = m_buffers[m_field50];
        col = x >> 3;
        x -= col << 3;
        cell = base + z * m_width + col;
        fx = ((short)(w11 * f) << VELOCITY_SCALE);
        cell->force[x] += fx;
        if (x < 7) {
            fx = ((short)(fz * f) << VELOCITY_SCALE);
            cell->force[x + 1] += fx;
            cell += m_width;
            fx = ((short)(w10 * f) << VELOCITY_SCALE);
            cell->force[x] += fx;
            fx = ((short)(w00 * f) << VELOCITY_SCALE);
            cell->force[x + 1] += fx;
        } else {
            cell++;
            fx = ((short)(fz * f) << VELOCITY_SCALE);
            cell->force[0] += fx;
            cell += m_width;
            fx = ((short)(w00 * f) << VELOCITY_SCALE);
            cell->force[0] += fx;
            cell--;
            fx = ((short)(w10 * f) << VELOCITY_SCALE);
            cell->force[7] += fx;
        }
    }
}


// ---- src/UIWaterPatch/Tick.cpp
// Retail compiled UIWaterPatch.cpp without instruction scheduling (delay
// slots stay nops, code is in source order), copy propagation, common
// subexpressions, strength reduction or loop-invariant hoisting.

typedef int int128_t __attribute__((mode(TI)));





static inline float Fabs(float x)
{
    float r;
    asm("abs.s $f8, %1; swc1 $f8, 0(%0)" : : "r"(&r), "f"(x) : "$f8", "memory");
    return r;
}

static inline int Max(int a, int b) { return a > b ? a : b; }
static inline int Min(int a, int b) { return a < b ? a : b; }

#define UNCACHED(p) ((WaterPatchCell *)PS2_ADDR((unsigned int)(p) | EE_UNCACHED_ACCEL))

// One step of the wave simulation on the MMI unit: each 8-wide cell row of
// the current buffer is filtered with its neighbours into the other buffer
// (uncached writes), then the buffers swap. The asm is retail's; its operand
// order fixes the registers mwcc picks for the pointers. The bucket offset is
// always zero in retail (offset is never set before it is read).
int UIWaterPatch::Tick(float dt)
{
    static int UIWakeSimID = -1;   // unused profiler ID
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
    // Declaration order chosen for retail's register allocation.
    int skip;
    _D3DVECTOR offset;
    int128_t pad[2];                // unused: retail's frame has 32 unexplained bytes here
    int128_t carry;
    int dx;
    int dz;
    WaterPatchCell *srcRow;
    WaterPatchCell *end;
    WaterPatchCell *src;
    WaterPatchCell *dst;
    WaterPatchCell *base;
    WaterPatchCell *dstRow;
    WaterPatchCell *rowEnd;
    WaterPatchCell *below;
    WaterPatchCell *above;
    WaterPatchCell *prev;
    int next;
    int cur;
    if (0)
        pad[0] = pad[1];            // keeps pad's stack slot
    offset.x = offset.y = offset.z = 0.0f;
    if (!(Fabs(offset.x) <= m_height) || !(Fabs(offset.z) <= m_height))
        offset = ZeroVector;
    dx = (int)offset.x >> 3;
    dz = (int)offset.z >> 3;
    dx = Max(dx, -1);
    dx = Min(dx, 1);
    dz = Max(dz, -1);
    dz = Min(dz, 1);
    dz <<= 3;
    cur = m_field50;
    next = (cur + 1) % 2;
    m_current = next;
    src = m_buffers[cur];
    base = UNCACHED(m_buffers[next]);
    end = UNCACHED(base + m_width * m_height);
    skip = m_width * 9 + 1;
    srcRow = src + skip + dz * m_width + dx;
    dstRow = UNCACHED(base + skip);
    rowEnd = UNCACHED(dstRow + m_width - 2);
    end -= skip;
    while (dstRow < end) {
        src = srcRow;
        dst = dstRow;
        below = srcRow + m_width;
        above = src - m_width;
        prev = dst - 1;
        asm("sq $0, 0x0(%0)" : : "r"(&carry) : "memory");
        while (dst < rowEnd) {
            asm(".set noreorder; lq $8, 0x10(%0); lq $9, 0x20(%0); lq $10, 0x0(%1); lq $11, 0x0(%2); paddsh $12, $8, $9; paddsh $13, $10, $11; paddsh $8, $12, $13; lq $9, 0x0(%3); pmulth $10, $8, $9; lq $12, 0x0(%0); lq $13, 0x0(%4); pmaddh $10, $12, $13; lq $14, 0x30(%0); psrah $13, $14, 6; lq $9, 0x0(%8); pmaddh $10, $13, $9; pmflo $8; pmfhi $9; pmfhl.sh $8; psrah $8, $8, 2; paddsh $14, $8, $14; lq $10, 0x0(%9); lq $11, 0x0(%10); pmaxh $14, $14, $10; pminh $14, $14, $11; psrah $8, $14, 8; paddsh $12, $12, $8; lq $8, 0x0(%6); lq $9, 0x0(%7); pmaxh $12, $12, $8; pminh $12, $12, $9; lq $8, 0x0(%12); lq $9, 0x0(%13); pcgth $8, $12, $8; pcgth $9, $9, $12; pmulth $8, $8, $9; pmfhl.sh $8; lq $10, 0x0(%14); lq $11, 0x0(%15); pcgth $10, $14, $10; pcgth $11, $11, $14; pmulth $10, $10, $11; pmfhl.sh $10; pmulth $8, $8, $10; pmfhl.sh $8; pmulth $9, $8, $12; pmfhl.sh $9; psubh $12, $12, $9; pmulth $9, $8, $14; pmfhl.sh $9; psubh $14, $14, $9; sq $12, 0x0(%5); sq $14, 0x30(%5); lq $9, 0x0(%7); paddsh $9, $12, $9; lq $11, 0x0(%11); mtsah $0, 1; qfsrv $10, $12, $11; sq $10, 0x20(%16); nop; mtsah $0, 15; qfsrv $10, $12, $11; sq $10, 0x10(%5); sq $12, 0x0(%11); .set reorder"
                :
                : "r"(src), "r"(below), "r"(above), "r"(Constant0), "r"(Constant1), "r"(dst), "r"(Clamp0), "r"(Clamp1), "r"(MinusOnes), "r"(Clamp2), "r"(Clamp3), "r"(&carry), "r"(Deadband0), "r"(Deadband1), "r"(Deadband2), "r"(Deadband3), "r"(prev)
                : "$8", "$9", "$10", "$11", "$12", "$13", "memory");
            // pad +0x2E of the previous cell (the first cell of the row)
            if (dst == dstRow)
                *(short *)((char *)prev + 0x2E) = 0;
            below++;
            above++;
            prev++;
            src++;
            dst++;
        }
        asm("mtsah $0, 1; lq $11, 0x0(%0); qfsrv $10, $0, $11; sq $10, 0x20(%1)"
            : : "r"(&carry), "r"(prev) : "$10", "$11", "memory");
        srcRow += m_width;
        dstRow += m_width;
        rowEnd += m_width;
    }
    m_position += _D3DVECTOR((float)(dx << 3) * m_scaleX, 0.0f, (float)dz * m_scaleZ);
    m_field50 = m_current;
    return 1;
}

// ---- src/sinit/UIWater.cpp
// Static initializer of UIWater.cpp (the axis vectors above).
