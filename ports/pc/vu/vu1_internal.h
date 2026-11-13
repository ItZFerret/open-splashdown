// VU1 internals shared by the interpreter (ports/pc/vu/vu1.cpp) and the
// recompiler (ports/pc/vu/vu1_rec.cpp): the register file, the pipeline model,
// the per-word decode and the instruction semantics.
#ifndef PORT_VU1_INTERNAL_H
#define PORT_VU1_INTERNAL_H

#include "vu.h"

#include <cstdint>
#include <cstring>
#include "port_simd.h"

namespace vu1i {

struct Vec {
    float f[4];
};

struct Regs {
    Vec vf[32];
    uint16_t vi[16];
    Vec acc;
    float q, p, i;
    uint32_t r;
    uint32_t mac, status, clip;
    // the recompiler's (not VU state): Q/P results in flight, flag updates in
    // flight (a ring of 4), the upper half's result when the lower half must
    // run between its read and its write, a sink for writes to VF0, the
    // branch condition and a jump's target
    float qNext, pNext;
    uint32_t fl[4];
    Vec tmp, sink;
    uint32_t br, jt;
};
extern Regs R;

// The pipeline state (vu1.cpp's header): the cycle count, when each VF field
// and VI register lands, the Q and P results in flight, and the flag
// updates in flight.
struct Timing {
    uint64_t cyc;
    uint64_t vf[32][4];
    uint64_t vfMax[32];    // the latest of vf[r][*] (Stall's quick test)
    uint64_t vi[16];
    bool qPend, pPend;
    float qNext, pNext;
    uint64_t qDone, pDone;
    struct Flag {
        uint64_t ready;
        uint8_t kind;  // 0 MAC+status, 1 clip
        uint32_t v;
    } flags[64];
    unsigned head, tail;
};
extern Timing Pipe;

extern const bool g_noTiming;

inline uint32_t AsU(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

inline float AsF(uint32_t u)
{
    float f;
    memcpy(&f, &u, 4);
    return f;
}

// NaN -> +max, +-inf -> +-max, denormals -> +-0 (from the bits: no libm
// classification calls on the hot path)
inline float Fix(float v)
{
    uint32_t u = AsU(v);
    uint32_t e = u & 0x7F800000u;
    if (__builtin_expect(e == 0x7F800000u, 0))
        return (u & 0x7FFFFFu) ? 3.4028235e38f : (u >> 31) ? -3.4028235e38f : 3.4028235e38f;
    if (__builtin_expect(e == 0 && (u & 0x7FFFFFu), 0))
        return (u >> 31) ? -0.0f : 0.0f;
    return v;
}

inline uint8_t *Mem(uint32_t qaddr)
{
    return g_vu1.data + (qaddr & 1023) * 16;
}

// dest bits: x is bit 3 ... w bit 0 (the instruction's bits 24..21)
inline bool Has(uint32_t dest, int field)
{
    return dest & (8 >> field);
}

inline int Sx(uint32_t v, int bits)
{
    return (int)(v << (32 - bits)) >> (32 - bits);
}

inline void SetVi(int r, uint32_t v)
{
    if (r)
        R.vi[r] = (uint16_t)v;
}

inline int16_t Vi(int r)
{
    return (int16_t)R.vi[r];
}

// Lane k of an SSE vector is field k (x..w); dest has x at bit 3.
extern const __m128i kLaneMask[16];
// movmskps (lane k at bit k) -> MAC order (x at bit 3)
extern const uint8_t kRev4[16];

// Fix on four lanes: NaN -> +max, +-inf -> +-max, denormals -> +-0.
inline __m128 FixV(__m128 v)
{
    const __m128i expMask = _mm_set1_epi32(0x7F800000), manMask = _mm_set1_epi32(0x7FFFFF);
    const __m128i sign = _mm_set1_epi32((int)0x80000000), max = _mm_set1_epi32(0x7F7FFFFF), zero = _mm_setzero_si128();
    __m128i u = _mm_castps_si128(v);
    __m128i e = _mm_and_si128(u, expMask);
    __m128i man = _mm_and_si128(u, manMask);
    __m128i manZero = _mm_cmpeq_epi32(man, zero);
    __m128i special = _mm_cmpeq_epi32(e, expMask);
    __m128i nan = _mm_andnot_si128(manZero, special);
    __m128i inf = _mm_and_si128(manZero, special);
    __m128i den = _mm_andnot_si128(manZero, _mm_cmpeq_epi32(e, zero));
    __m128i s = _mm_and_si128(u, sign);
    // denormal: the sign alone; inf: sign | max; NaN: max
    u = _mm_or_si128(_mm_andnot_si128(den, u), _mm_and_si128(den, s));
    u = _mm_or_si128(_mm_andnot_si128(inf, u), _mm_and_si128(inf, _mm_or_si128(s, max)));
    u = _mm_or_si128(_mm_andnot_si128(nan, u), _mm_and_si128(nan, max));
    return _mm_castsi128_ps(u);
}

// FixV only where a lane needs it (NaN/inf: exponent 255; denormal:
// exponent 0 with a mantissa)
inline __m128 FixFast(__m128 r)
{
    const __m128i expMask = _mm_set1_epi32(0x7F800000);
    __m128i u = _mm_castps_si128(r);
    __m128i e = _mm_and_si128(u, expMask);
    __m128i bad = _mm_or_si128(_mm_cmpeq_epi32(e, expMask),
                               _mm_andnot_si128(_mm_cmpeq_epi32(_mm_slli_epi32(u, 1), _mm_setzero_si128()),
                                                _mm_cmpeq_epi32(e, _mm_setzero_si128())));
    if (__builtin_expect(_mm_movemask_epi8(bad), 0))
        r = FixV(r);
    return r;
}

// The MAC flags of r's dest fields (Z bits 3..0 = x..w, S bits 7..4).
inline uint32_t MacOf(__m128 r, uint32_t dest)
{
    uint32_t lanes = kRev4[dest & 15];
    uint32_t z = (uint32_t)_mm_movemask_ps(_mm_cmpeq_ps(r, _mm_setzero_ps())) & lanes;
    uint32_t sg = (uint32_t)_mm_movemask_ps(r) & lanes;
    return kRev4[z] | kRev4[sg] << 4;
}

// A MAC flag update landing: MAC, and status Z/S with their sticky copies.
inline void LandMac(uint32_t v)
{
    R.mac = v & 0xFF;
    // Z S (U O: not modelled) and their sticky copies
    uint32_t st = R.status & 0xFF0;
    if (v & 0xF)
        st |= 1 | 0x40;
    if (v & 0xF0)
        st |= 2 | 0x80;
    R.status = st;
}

inline void LandClip(uint32_t v)
{
    R.clip = ((R.clip << 6) | v) & 0xFFFFFF;
}

// Writes from the upper half, applied after the lower half ran.
struct Pending {
    int reg;  // -1 none, 0-31 vf, 32 acc
    uint32_t dest;
    Vec v;
    int flagKind;  // -1 none, 0 MAC, 1 clip
    uint32_t flagV;
};

// What an instruction reads, for the stall (decoded once per instruction
// word: Decode), and the VF register its lower half writes.
struct Dec {
    uint64_t word;
    bool valid;
    uint8_t nvf, nvi;
    uint8_t vfReg[4], vfMask[4];  // fields: x=8 ... w=1
    uint8_t viReg[4];
    bool waitQ, waitP;
    int8_t loVf;                  // LowerVfWrite, or -1
    // the upper half, for UpperFast: ADD/SUB/MUL/MADD/MSUB in their
    // vector, broadcast, Q and I forms, to fd or ACC
    uint8_t ucls, usrc, ubc;
    int8_t udst;                  // 0-31 vf, 32 acc
    uint8_t udest, ufs, uft;
    // the lower half: the common ops run inline (vu1_run), the rest by Lower
    uint8_t lk, lit, lis, ldest;
    int32_t limm;
    bool flagRead;                // reads MAC/status/clip (lands them first)
    bool qpRead;                  // reads Q (upper) or P (MFP): lands them first
};
enum { LK_OTHER, LK_NOP, LK_LQ, LK_SQ, LK_LQI, LK_SQI, LK_IADDIU, LK_ISUBIU, LK_NONE };
enum { UC_SLOW, UC_NOP, UC_ADD, UC_SUB, UC_MUL, UC_MADD, UC_MSUB };
enum { US_VEC, US_BC, US_Q, US_I };

// The cycle an instruction can issue at: after the VF fields and VI
// registers it reads have landed, and (DIV/SQRT/RSQRT/WAITQ, EFU ops/WAITP)
// after the FDIV unit or the EFU is free.
__attribute__((always_inline)) inline uint64_t StallOn(const Timing &P, const Dec &d)
{
    uint64_t t = P.cyc;
    for (int i = 0; i < d.nvf; i++) {
        if (P.vfMax[d.vfReg[i]] <= t)
            continue;
        const uint64_t *l = P.vf[d.vfReg[i]];
        uint32_t m = d.vfMask[i];
        if ((m & 8) && l[0] > t) t = l[0];
        if ((m & 4) && l[1] > t) t = l[1];
        if ((m & 2) && l[2] > t) t = l[2];
        if ((m & 1) && l[3] > t) t = l[3];
    }
    for (int i = 0; i < d.nvi; i++)
        if (P.vi[d.viReg[i]] > t)
            t = P.vi[d.viReg[i]];
    if (d.waitQ && P.qPend && P.qDone > t)
        t = P.qDone;
    if (d.waitP && P.pPend && P.pDone > t)
        t = P.pDone;
    return t;
}

// Fills d's read sets (the upper half's, and the lower's if `lower`).
void DecodeStall(uint32_t hi, uint32_t lo, bool lower, Dec &d);
// Fills d for the instruction word ins.
void Decode(uint64_t ins, Dec &d);
// The upper half (any op), into out.
void Upper(uint32_t w, Pending &out);
// The lower half (any op but those touching the pipeline: the recompiler
// handles DIV/SQRT/RSQRT, the EFU ops, ILW/ILWR and FCSET itself). Returns
// the branch target (instruction index) or -1.
int Lower(uint32_t w, uint32_t pc, bool &stop);
// What the upper half writes: VF register (32: ACC, -1: none), its fields,
// and its flag update (-1 none, 0 MAC, 1 clip).
void UpperWrites(uint32_t hi, int &reg, uint32_t &dest, int &flagKind);

// Q's and P's new values (DIV/SQRT/RSQRT; the EFU ops by their sub-opcode).
float QValue(uint32_t lo);
float PValue(uint32_t lo);
// Q and P latencies of those ops.
int QLatency(uint32_t lo);
int PLatency(uint32_t lo);

// XGKICK (the differential mode records them).
void Kick(uint32_t qaddr);

// The interpreter's pipeline: lands what has finished by `now` (~0: all).
void Commit(uint64_t now);

// The interpreter from pc: until [E] (returns true) or, with untilClean,
// until the first instruction boundary with no branch or [E] pending
// (false). steps counts the instructions run.
bool Interp(uint32_t &pc, bool untilClean, int &steps);

// The recompiler: runs the program from pc to its [E] (vu1_run's start has
// landed everything); steps counts the instructions run.
void RecRun(uint32_t pc, int &steps);
// PORT_VU1_STATS' line for the recompiler.
void RecStats();

extern const int kMaxSteps;
extern uint64_t g_vu1Steps;

} // namespace vu1i

#endif
