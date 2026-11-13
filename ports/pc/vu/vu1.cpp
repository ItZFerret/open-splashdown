// A VU1 interpreter for the game's microprograms (the retail microcode,
// uploaded by MPG from the packets in build/port/gen/vupackets.s).
//
// Each 64-bit instruction is an upper (FMAC) and a lower (integer, load/
// store, branch, FDIV/EFU) half, run in order with the hardware's pairing
// rules: both halves read their sources before either writes, and if both
// write the same VF register the upper one wins. Branches have one delay
// slot; [E] ends the program after the next instruction. VU floats: no
// Inf/NaN (they saturate to +-max), denormals flush to zero.
//
// Timing (Timing below): the pipeline matters where the hardware does not
// interlock. VF registers are interlocked (an instruction that reads a field
// still in flight stalls until it lands: FMAC, loads, MOVE/MR32/MFIR/MFP
// take 4 cycles), so their values are as if immediate, but the stalls move
// the cycle count, which decides the rest: Q (DIV/SQRT 7 cycles, RSQRT 13)
// and P (the EFU: ELENG 17, ERSQRT 17, ...) keep their old value until the
// operation's latency has passed (WAITQ/WAITP, or a second DIV/EFU op while
// one is in flight, stall until it is done; latencies as PCSX2's microVU
// has them); the MAC, status and clip flags
// that FMxxx/FSxxx/FCxxx read are those of FMAC ops (and CLIPs) issued at
// least 4 cycles before. WaterScreenSpaceRendererCode reads Q a cycle after
// issuing the next DIV and P one ELENG behind (its triangles were garbage
// with immediate results), StripLitCode's guard-band test reads the clip
// flags of the CLIPs 4 to 6 back. ILW/ILWR results take 4 cycles (stall).
// A program starts with everything landed (the VU idles between programs).
//
// Speed: each instruction word is decoded once (Dec, per address, redone
// when micro memory changes): the stall's read sets, the upper op's class
// (ADD/SUB/MUL/MADD/MSUB in vector, bc, Q and I forms run in SSE with the
// MAC flags from movemask, UpperFast) and the common lower ops (NOP, LQ, SQ,
// LQI, SQI, IADDIU, ISUBIU inline); the rest run as before (Upper, Lower).
// Flag updates are queued and landed only when an instruction reads them (or
// the queue fills); Q and P land when read or replaced.
//
// This interpreter is the reference: the recompiler (vu1_rec.cpp) runs the
// programs by default; PORT_VU1_INTERP=1 selects this one, PORT_VU1_DIFF=1
// runs both and compares (DiffRun).
#include "vu1_internal.h"

#include "gs/gs.h"
#include "port.h"
#include "settings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

Vu1 g_vu1;

namespace vu1i {

Regs R;
Timing Pipe;

// PORT_VU1_NOTIMING=1: results land at once (the interpreter before the
// timing model, for comparisons)
const bool g_noTiming = getenv("PORT_VU1_NOTIMING") != nullptr;

void CommitFlags(uint64_t now);

// Flag updates are queued and landed lazily: by the instructions that read
// the flags (Dec::flagRead), when the queue fills, and at the next program's
// start; in order, so the result is as if each landed at its cycle.
inline void PushFlag(uint8_t kind, uint32_t v)
{
    if (__builtin_expect(Pipe.tail - Pipe.head == 64, 0))
        CommitFlags(Pipe.cyc);  // at most the last 4 cycles' stay
    uint64_t ready = Pipe.cyc + (g_noTiming ? 0 : 4);
    Pipe.flags[Pipe.tail++ & 63] = {ready, kind, v};
}

// Lands what has finished by `now`.
// Lands the flag updates that have finished by `now`.
void CommitFlags(uint64_t now)
{
    while (Pipe.head != Pipe.tail && Pipe.flags[Pipe.head & 63].ready <= now) {
        const Timing::Flag &f = Pipe.flags[Pipe.head++ & 63];
        if (f.kind == 0)
            LandMac(f.v);
        else
            LandClip(f.v);
    }
}

// Lands Q and P if they have finished by `now` (every instruction).
inline void CommitQP(uint64_t now)
{
    if (Pipe.qPend && Pipe.qDone <= now) {
        R.q = Pipe.qNext;
        Pipe.qPend = false;
    }
    if (Pipe.pPend && Pipe.pDone <= now) {
        R.p = Pipe.pNext;
        Pipe.pPend = false;
    }
}

// Lands what has finished by `now`.
void Commit(uint64_t now)
{
    CommitQP(now);
    CommitFlags(now);
}

void CommitAll()
{
    Commit(~0ull);
    Pipe.cyc = 0;
    memset(Pipe.vf, 0, sizeof Pipe.vf);
    memset(Pipe.vfMax, 0, sizeof Pipe.vfMax);
    memset(Pipe.vi, 0, sizeof Pipe.vi);
}

// An FMAC op's MAC flags (and through them status Z/S), 4 cycles later.
uint32_t SetMac(uint32_t dest, const Vec &v)
{
    uint32_t mac = 0;
    for (int k = 0; k < 4; k++) {
        if (!Has(dest, k))
            continue;
        int bit = 3 - k;
        if (v.f[k] == 0)
            mac |= 1u << bit;
        if (std::signbit(v.f[k]))
            mac |= 1u << (bit + 4);
    }
    return mac;
}

// MAX/MINI leave the flags alone
bool g_noFlags;

void Upper(uint32_t w, Pending &out)
{
    out.reg = -1;
    out.flagKind = -1;
    uint32_t dest = (w >> 21) & 15;
    int ft = (w >> 16) & 31, fs = (w >> 11) & 31, fd = (w >> 6) & 31;
    uint32_t op = w & 63;
    const Vec &S = R.vf[fs], &T = R.vf[ft];
    Vec res = R.vf[fd];
    auto bc = [&](int k) { return T.f[k]; };
    auto each = [&](auto fn, int dreg) {
        Vec r = dreg == 32 ? R.acc : R.vf[dreg];
        for (int k = 0; k < 4; k++)
            if (Has(dest, k))
                r.f[k] = Fix(fn(k));
        out.reg = dreg;
        out.dest = dest;
        out.v = r;
        if (!g_noFlags) {
            out.flagKind = 0;
            out.flagV = SetMac(dest, r);
        }
        g_noFlags = false;
    };
    (void)res;
    if (op < 0x1C) {
        float b = bc(op & 3);
        switch (op >> 2) {
        case 0: each([&](int k) { return S.f[k] + b; }, fd); break;
        case 1: each([&](int k) { return S.f[k] - b; }, fd); break;
        case 2: each([&](int k) { return R.acc.f[k] + S.f[k] * b; }, fd); break;
        case 3: each([&](int k) { return R.acc.f[k] - S.f[k] * b; }, fd); break;
        case 4: g_noFlags = true; each([&](int k) { return std::fmax(S.f[k], b); }, fd); break;
        case 5: g_noFlags = true; each([&](int k) { return std::fmin(S.f[k], b); }, fd); break;
        case 6: each([&](int k) { return S.f[k] * b; }, fd); break;
        }
        return;
    }
    switch (op) {
    case 0x1C: each([&](int k) { return S.f[k] * R.q; }, fd); return;
    case 0x1D: g_noFlags = true; each([&](int k) { return std::fmax(S.f[k], R.i); }, fd); return;
    case 0x1E: each([&](int k) { return S.f[k] * R.i; }, fd); return;
    case 0x1F: g_noFlags = true; each([&](int k) { return std::fmin(S.f[k], R.i); }, fd); return;
    case 0x20: each([&](int k) { return S.f[k] + R.q; }, fd); return;
    case 0x21: each([&](int k) { return R.acc.f[k] + S.f[k] * R.q; }, fd); return;
    case 0x22: each([&](int k) { return S.f[k] + R.i; }, fd); return;
    case 0x23: each([&](int k) { return R.acc.f[k] + S.f[k] * R.i; }, fd); return;
    case 0x24: each([&](int k) { return S.f[k] - R.q; }, fd); return;
    case 0x25: each([&](int k) { return R.acc.f[k] - S.f[k] * R.q; }, fd); return;
    case 0x26: each([&](int k) { return S.f[k] - R.i; }, fd); return;
    case 0x27: each([&](int k) { return R.acc.f[k] - S.f[k] * R.i; }, fd); return;
    case 0x28: each([&](int k) { return S.f[k] + T.f[k]; }, fd); return;
    case 0x29: each([&](int k) { return R.acc.f[k] + S.f[k] * T.f[k]; }, fd); return;
    case 0x2A: each([&](int k) { return S.f[k] * T.f[k]; }, fd); return;
    case 0x2B: g_noFlags = true; each([&](int k) { return std::fmax(S.f[k], T.f[k]); }, fd); return;
    case 0x2C: each([&](int k) { return S.f[k] - T.f[k]; }, fd); return;
    case 0x2D: each([&](int k) { return R.acc.f[k] - S.f[k] * T.f[k]; }, fd); return;
    case 0x2E: {
        float c[3] = {S.f[1] * T.f[2], S.f[2] * T.f[0], S.f[0] * T.f[1]};
        each([&](int k) { return k < 3 ? R.acc.f[k] - c[k] : 0.0f; }, fd);
        return;
    }
    case 0x2F: g_noFlags = true; each([&](int k) { return std::fmin(S.f[k], T.f[k]); }, fd); return;
    }
    if (op < 0x3C) {
        port_log("VU1: upper op 0x%08x", w);
        return;
    }
    uint32_t s = (((w >> 6) & 31) << 2) | (w & 3);
    if (s < 0x10) {
        float b = bc(s & 3);
        switch (s >> 2) {
        case 0: each([&](int k) { return S.f[k] + b; }, 32); break;
        case 1: each([&](int k) { return S.f[k] - b; }, 32); break;
        case 2: each([&](int k) { return R.acc.f[k] + S.f[k] * b; }, 32); break;
        case 3: each([&](int k) { return R.acc.f[k] - S.f[k] * b; }, 32); break;
        }
        return;
    }
    if (s < 0x18) {
        // ITOF/FTOI write ft, set no flags
        static const float kScale[4] = {1.0f, 16.0f, 4096.0f, 32768.0f};
        float sc = kScale[s & 3];
        Vec r = R.vf[ft];
        for (int k = 0; k < 4; k++) {
            if (!Has(dest, k))
                continue;
            uint32_t raw = AsU(S.f[k]);
            if (s < 0x14) {
                r.f[k] = (float)(int32_t)raw / sc;
            } else {
                double v = (double)S.f[k] * sc;
                int32_t iv = v >= 2147483647.0 ? 0x7FFFFFFF : v <= -2147483648.0 ? (int32_t)0x80000000 : (int32_t)v;
                r.f[k] = AsF((uint32_t)iv);
            }
        }
        out.reg = ft;
        out.dest = dest;
        out.v = r;
        return;
    }
    if (s < 0x1C) {
        float b = bc(s & 3);
        each([&](int k) { return S.f[k] * b; }, 32);
        return;
    }
    switch (s) {
    case 0x1C: each([&](int k) { return S.f[k] * R.q; }, 32); return;
    case 0x1D: {
        Vec r = R.vf[ft];
        for (int k = 0; k < 4; k++)
            if (Has(dest, k))
                r.f[k] = std::fabs(S.f[k]);
        out.reg = ft;
        out.dest = dest;
        out.v = r;
        return;
    }
    case 0x1E: each([&](int k) { return S.f[k] * R.i; }, 32); return;
    case 0x1F: {
        float wv = std::fabs(T.f[3]);
        uint32_t c = 0;
        if (S.f[0] > wv) c |= 1;
        if (S.f[0] < -wv) c |= 2;
        if (S.f[1] > wv) c |= 4;
        if (S.f[1] < -wv) c |= 8;
        if (S.f[2] > wv) c |= 16;
        if (S.f[2] < -wv) c |= 32;
        out.flagKind = 1;
        out.flagV = c;
        return;
    }
    case 0x20: each([&](int k) { return S.f[k] + R.q; }, 32); return;
    case 0x21: each([&](int k) { return R.acc.f[k] + S.f[k] * R.q; }, 32); return;
    case 0x22: each([&](int k) { return S.f[k] + R.i; }, 32); return;
    case 0x23: each([&](int k) { return R.acc.f[k] + S.f[k] * R.i; }, 32); return;
    case 0x24: each([&](int k) { return S.f[k] - R.q; }, 32); return;
    case 0x25: each([&](int k) { return R.acc.f[k] - S.f[k] * R.q; }, 32); return;
    case 0x26: each([&](int k) { return S.f[k] - R.i; }, 32); return;
    case 0x27: each([&](int k) { return R.acc.f[k] - S.f[k] * R.i; }, 32); return;
    case 0x28: each([&](int k) { return S.f[k] + T.f[k]; }, 32); return;
    case 0x29: each([&](int k) { return R.acc.f[k] + S.f[k] * T.f[k]; }, 32); return;
    case 0x2A: each([&](int k) { return S.f[k] * T.f[k]; }, 32); return;
    case 0x2C: each([&](int k) { return S.f[k] - T.f[k]; }, 32); return;
    case 0x2D: each([&](int k) { return R.acc.f[k] - S.f[k] * T.f[k]; }, 32); return;
    case 0x2E: {
        float c[3] = {S.f[1] * T.f[2], S.f[2] * T.f[0], S.f[0] * T.f[1]};
        each([&](int k) { return k < 3 ? c[k] : 0.0f; }, 32);
        return;
    }
    case 0x2F: return;  // NOP
    }
    port_log("VU1: upper op 0x%08x", w);
}

void SetVf(int r, uint32_t dest, const Vec &v)
{
    if (!r)
        return;
    for (int k = 0; k < 4; k++)
        if (Has(dest, k))
            R.vf[r].f[k] = v.f[k];
}

Vec Load(uint32_t qaddr)
{
    Vec v;
    memcpy(&v, Mem(qaddr), 16);
    return v;
}

void Store(uint32_t qaddr, uint32_t dest, const Vec &v)
{
    float *m = (float *)Mem(qaddr);
    for (int k = 0; k < 4; k++)
        if (Has(dest, k))
            m[k] = v.f[k];
}

int FirstField(uint32_t dest)
{
    for (int k = 0; k < 4; k++)
        if (Has(dest, k))
            return k;
    return 0;
}

// DIV/SQRT/RSQRT: Q changes `lat` cycles after issue (Stall waited for the
// previous one).
void SetQ(float v, int lat)
{
    if (Pipe.qPend)
        R.q = Pipe.qNext;  // landed: DIV/SQRT/RSQRT stall until it has
    Pipe.qPend = true;
    Pipe.qNext = v;
    Pipe.qDone = Pipe.cyc + (g_noTiming ? 0 : lat);
}

// The EFU ops: P changes `lat` cycles after issue. The latencies are the
// manual's minus one, as PCSX2's microVU has them: retail's hand-scheduled
// loops issue ELENG 17 cycles after the previous one (no stall).
void SetP(float v, int lat)
{
    if (Pipe.pPend)
        R.p = Pipe.pNext;  // landed: the EFU ops stall until it has
    Pipe.pPend = true;
    Pipe.pNext = v;
    Pipe.pDone = Pipe.cyc + (g_noTiming ? 0 : lat);
}

Dec g_dec[2048];

__attribute__((always_inline)) inline uint64_t Stall(const Dec &d)
{
    if (g_noTiming)
        return Pipe.cyc;
    return StallOn(Pipe, d);
}

int LowerVfWrite(uint32_t lo);

void DecodeLower(uint32_t lo, bool lower, Dec &d)
{
    d.lk = LK_OTHER;
    d.flagRead = false;
    if (!lower) {
        d.lk = LK_NONE;
        return;
    }
    uint32_t op = lo >> 25;
    d.ldest = (lo >> 21) & 15;
    d.lit = (lo >> 16) & 31;
    d.lis = (lo >> 11) & 31;
    if (op >= 0x10 && op <= 0x1C)
        d.flagRead = true;  // FC*, FS*, FM*, FCGET (FCSET lands everything itself)
    switch (op) {
    case 0x00: d.lk = LK_LQ; d.limm = Sx(lo & 0x7FF, 11); return;
    case 0x01: d.lk = LK_SQ; d.limm = Sx(lo & 0x7FF, 11); return;
    case 0x08: d.lk = LK_IADDIU; d.limm = (((lo >> 21) & 15) << 11) | (lo & 0x7FF); return;
    case 0x09: d.lk = LK_ISUBIU; d.limm = (((lo >> 21) & 15) << 11) | (lo & 0x7FF); return;
    case 0x40: break;
    default: return;
    }
    if ((lo & 63) < 0x3C)
        return;
    uint32_t sx = (((lo >> 6) & 31) << 2) | (lo & 3);
    if (sx == 0x64)
        d.qpRead = true;  // MFP
    if (sx == 0x30 && (d.ldest == 0 || d.lit == 0))
        d.lk = LK_NOP;  // MOVE to nothing (the assembler's NOP)
    else if (sx == 0x34)
        d.lk = LK_LQI;
    else if (sx == 0x35)
        d.lk = LK_SQI;
}

void DecodeUpper(uint32_t hi, Dec &d)
{
    d.ucls = UC_SLOW;
    d.udest = (hi >> 21) & 15;
    d.uft = (hi >> 16) & 31;
    d.ufs = (hi >> 11) & 31;
    int fd = (hi >> 6) & 31;
    uint32_t op = hi & 63;
    auto set = [&](int cls, int src, int dst) {
        d.ucls = cls;
        d.usrc = src;
        d.udst = dst;
    };
    if (op < 0x1C) {
        d.ubc = op & 3;
        static const int kCls[7] = {UC_ADD, UC_SUB, UC_MADD, UC_MSUB, UC_SLOW, UC_SLOW, UC_MUL};
        if (kCls[op >> 2] != UC_SLOW)
            set(kCls[op >> 2], US_BC, fd);
        return;
    }
    if (op < 0x30) {
        switch (op) {
        case 0x1C: set(UC_MUL, US_Q, fd); break;
        case 0x1E: set(UC_MUL, US_I, fd); break;
        case 0x20: set(UC_ADD, US_Q, fd); break;
        case 0x21: set(UC_MADD, US_Q, fd); break;
        case 0x22: set(UC_ADD, US_I, fd); break;
        case 0x23: set(UC_MADD, US_I, fd); break;
        case 0x24: set(UC_SUB, US_Q, fd); break;
        case 0x25: set(UC_MSUB, US_Q, fd); break;
        case 0x26: set(UC_SUB, US_I, fd); break;
        case 0x27: set(UC_MSUB, US_I, fd); break;
        case 0x28: set(UC_ADD, US_VEC, fd); break;
        case 0x29: set(UC_MADD, US_VEC, fd); break;
        case 0x2A: set(UC_MUL, US_VEC, fd); break;
        case 0x2C: set(UC_SUB, US_VEC, fd); break;
        case 0x2D: set(UC_MSUB, US_VEC, fd); break;
        }
        return;
    }
    if (op < 0x3C)
        return;
    uint32_t sx = (((hi >> 6) & 31) << 2) | (hi & 3);
    if (sx < 0x10) {
        d.ubc = sx & 3;
        static const int kCls[4] = {UC_ADD, UC_SUB, UC_MADD, UC_MSUB};
        set(kCls[sx >> 2], US_BC, 32);
        return;
    }
    if (sx >= 0x18 && sx < 0x1C) {
        d.ubc = sx & 3;
        set(UC_MUL, US_BC, 32);
        return;
    }
    switch (sx) {
    case 0x1C: set(UC_MUL, US_Q, 32); break;
    case 0x1E: set(UC_MUL, US_I, 32); break;
    case 0x20: set(UC_ADD, US_Q, 32); break;
    case 0x21: set(UC_MADD, US_Q, 32); break;
    case 0x22: set(UC_ADD, US_I, 32); break;
    case 0x23: set(UC_MADD, US_I, 32); break;
    case 0x24: set(UC_SUB, US_Q, 32); break;
    case 0x25: set(UC_MSUB, US_Q, 32); break;
    case 0x26: set(UC_SUB, US_I, 32); break;
    case 0x27: set(UC_MSUB, US_I, 32); break;
    case 0x28: set(UC_ADD, US_VEC, 32); break;
    case 0x29: set(UC_MADD, US_VEC, 32); break;
    case 0x2A: set(UC_MUL, US_VEC, 32); break;
    case 0x2C: set(UC_SUB, US_VEC, 32); break;
    case 0x2D: set(UC_MSUB, US_VEC, 32); break;
    case 0x2F: d.ucls = UC_NOP; break;
    }
}

// Fills d's read sets as the hardware's interlocks see them.
void DecodeStall(uint32_t hi, uint32_t lo, bool lower, Dec &d)
{
    d.nvf = d.nvi = 0;
    d.waitQ = d.waitP = false;
    d.loVf = lower ? (int8_t)LowerVfWrite(lo) : -1;
    auto vf = [&](int r, uint32_t fields) {
        if (!r || !(fields & 15))
            return;
        d.vfReg[d.nvf] = (uint8_t)r;
        d.vfMask[d.nvf++] = (uint8_t)(fields & 15);
    };
    auto vi = [&](int r) {
        if (r)
            d.viReg[d.nvi++] = (uint8_t)r;
    };

    {
        uint32_t dest = (hi >> 21) & 15;
        int ft = (hi >> 16) & 31, fs = (hi >> 11) & 31;
        uint32_t op = hi & 63;
        uint32_t bc = 8 >> (op & 3);
        if (op < 0x1C) {
            vf(fs, dest);
            vf(ft, bc);
        } else if (op < 0x28) {
            vf(fs, dest);
        } else if (op < 0x30) {
            uint32_t f = op == 0x2E ? 14 : dest;
            vf(fs, f);
            vf(ft, f);
        } else if (op >= 0x3C) {
            uint32_t s = (((hi >> 6) & 31) << 2) | (hi & 3);
            bc = 8 >> (s & 3);
            if (s < 0x10 || (s >= 0x18 && s < 0x1C)) {
                vf(fs, dest);
                vf(ft, bc);
            } else if (s < 0x28 && s != 0x1F) {
                vf(fs, dest);
            } else if (s == 0x1F) {
                vf(fs, 14);
                vf(ft, 1);
            } else if (s < 0x2F) {
                uint32_t f = s == 0x2E ? 14 : dest;
                vf(fs, f);
                vf(ft, f);
            }
        }
    }
    if (!lower)
        return;
    uint32_t op = lo >> 25;
    uint32_t dest = (lo >> 21) & 15;
    int it = (lo >> 16) & 31, is = (lo >> 11) & 31;
    uint32_t fsf = 8 >> ((lo >> 21) & 3), ftf = 8 >> ((lo >> 23) & 3);
    if (op == 0x40) {
        uint32_t f = lo & 63;
        if (f >= 0x30 && f <= 0x35) {
            vi(is & 15);
            if (f != 0x32)
                vi(it & 15);
            return;
        }
        if (f < 0x3C)
            return;
        uint32_t s = (((lo >> 6) & 31) << 2) | (lo & 3);
        switch (s) {
        case 0x30: vf(is, dest); break;
        case 0x31: vf(is, (dest >> 1) | ((dest & 1) << 3)); break;  // MR32: x<-y ... w<-x
        case 0x34: case 0x36: case 0x3D: case 0x3E: case 0x6C: vi(is & 15); break;
        case 0x35: case 0x37: vf(is, dest); vi(it & 15); break;
        case 0x3F: vi(is & 15); vi(it & 15); break;
        case 0x38: case 0x3A:
            vf(is, fsf);
            vf(it, ftf);
            d.waitQ = true;
            break;
        case 0x39:
            vf(it, ftf);
            d.waitQ = true;
            break;
        case 0x3B:
            d.waitQ = true;
            break;
        case 0x3C: case 0x42: case 0x43: vf(is, fsf); break;
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76:
        case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E:
            if (s <= 0x73)
                vf(is, 14);
            else if (s == 0x74)
                vf(is, 12);
            else if (s == 0x75)
                vf(is, 10);
            else if (s == 0x76)
                vf(is, 15);
            else if (s != 0x7B)
                vf(is, fsf);
            d.waitP = true;
            break;
        }
        return;
    }
    switch (op) {
    case 0x00: case 0x04: case 0x08: case 0x09: case 0x18: case 0x1A: case 0x1B: case 0x24: case 0x25:
        vi(is & 15);
        break;
    case 0x01: vf(is, dest); vi(it & 15); break;
    case 0x05: vi(is & 15); vi(it & 15); break;
    case 0x28: case 0x29: vi(is & 15); vi(it & 15); break;
    case 0x2C: case 0x2D: case 0x2E: case 0x2F: vi(is & 15); break;
    }
    return;
}

// The VF fields an instruction writes land 4 cycles after issue.
inline void Lands(int reg, uint32_t dest)
{
    if (reg <= 0 || reg > 31)
        return;
    uint64_t t = Pipe.cyc + 4;
    uint64_t *l = Pipe.vf[reg];
    l[0] = (dest & 8) ? t : l[0];
    l[1] = (dest & 4) ? t : l[1];
    l[2] = (dest & 2) ? t : l[2];
    l[3] = (dest & 1) ? t : l[3];
    Pipe.vfMax[reg] = (dest & 15) ? t : Pipe.vfMax[reg];
}

// The VF register a lower instruction writes (loads, MOVE, MR32, MFIR,
// MFP, RNEXT/RGET), for Lands.
int LowerVfWrite(uint32_t lo)
{
    uint32_t op = lo >> 25;
    int it = (lo >> 16) & 31;
    if (op == 0x00)
        return it;
    if (op != 0x40 || (lo & 63) < 0x3C)
        return -1;
    uint32_t s = (((lo >> 6) & 31) << 2) | (lo & 3);
    switch (s) {
    case 0x30: case 0x31: case 0x34: case 0x36: case 0x3D: case 0x40: case 0x41: case 0x64: return it;
    }
    return -1;
}

float QValue(uint32_t w)
{
    int it = (w >> 16) & 31, is = (w >> 11) & 31;
    int fsf = (w >> 21) & 3, ftf = (w >> 23) & 3;
    uint32_t s = (((w >> 6) & 31) << 2) | (w & 3);
    if (s == 0x38) {
        float a = R.vf[is].f[fsf], b = R.vf[it].f[ftf];
        return b == 0 ? (std::signbit(a) != std::signbit(b) ? -3.4028235e38f : 3.4028235e38f) : Fix(a / b);
    }
    if (s == 0x39)
        return Fix(std::sqrt(std::fabs(R.vf[it].f[ftf])));
    float a = R.vf[is].f[fsf], b = std::sqrt(std::fabs(R.vf[it].f[ftf]));
    return b == 0 ? 3.4028235e38f : Fix(a / b);
}

int QLatency(uint32_t w)
{
    uint32_t s = (((w >> 6) & 31) << 2) | (w & 3);
    return s == 0x3A ? 13 : 7;
}

float PValue(uint32_t w)
{
    int is = (w >> 11) & 31;
    int fsf = (w >> 21) & 3;
    uint32_t s = (((w >> 6) & 31) << 2) | (w & 3);
    switch (s) {
    case 0x70: case 0x71: case 0x72: case 0x73: {
        const Vec &v = R.vf[is];
        float sq = v.f[0] * v.f[0] + v.f[1] * v.f[1] + v.f[2] * v.f[2];
        float r = s == 0x70 ? sq : s == 0x71 ? 1.0f / sq : s == 0x72 ? std::sqrt(sq) : 1.0f / std::sqrt(sq);
        return Fix(r);
    }
    case 0x74: return Fix(std::atan2(R.vf[is].f[1], R.vf[is].f[0]));
    case 0x75: return Fix(std::atan2(R.vf[is].f[2], R.vf[is].f[0]));
    case 0x76: return Fix(R.vf[is].f[0] + R.vf[is].f[1] + R.vf[is].f[2] + R.vf[is].f[3]);
    case 0x78: return Fix(std::sqrt(std::fabs(R.vf[is].f[fsf])));
    case 0x79: return Fix(1.0f / std::sqrt(std::fabs(R.vf[is].f[fsf])));
    case 0x7A: return Fix(1.0f / R.vf[is].f[fsf]);
    case 0x7C: return Fix(std::sin(R.vf[is].f[fsf]));
    case 0x7D: return Fix(std::atan(R.vf[is].f[fsf]));
    default: return Fix(std::exp(-R.vf[is].f[fsf]));  // 0x7E
    }
}

// The latencies are the manual's minus one, as PCSX2's microVU has them
// (SetP).
int PLatency(uint32_t w)
{
    uint32_t s = (((w >> 6) & 31) << 2) | (w & 3);
    switch (s) {
    case 0x70: return 10;  // ESADD
    case 0x71: case 0x72: return 17;  // ERSADD ELENG
    case 0x73: return 23;  // ERLENG
    case 0x74: case 0x75: case 0x7D: return 53;  // EATANxy EATANxz EATAN
    case 0x76: case 0x78: case 0x7A: return 11;  // ESUM ESQRT ERCPR
    case 0x79: return 17;  // ERSQRT
    case 0x7C: return 28;  // ESIN
    default: return 43;  // EEXP
    }
}

// Runs the lower half. Returns the branch target (instruction index) or -1.
int Lower(uint32_t w, uint32_t pc, bool &stop)
{
    uint32_t op = w >> 25;
    uint32_t dest = (w >> 21) & 15;
    int it = (w >> 16) & 31, is = (w >> 11) & 31, id = (w >> 6) & 31;
    int imm11 = Sx(w & 0x7FF, 11);
    int target = ((int)pc + 1 + imm11) & 2047; // wraps: TerrainScissorCode calls 0x797 as -0x69
    switch (op) {
    case 0x40: {
        uint32_t f = w & 63;
        switch (f) {
        case 0x30: SetVi(id, Vi(is) + Vi(it)); return -1;
        case 0x31: SetVi(id, Vi(is) - Vi(it)); return -1;
        case 0x32: SetVi(it, Vi(is) + Sx(id, 5)); return -1;
        case 0x34: SetVi(id, R.vi[is] & R.vi[it]); return -1;
        case 0x35: SetVi(id, R.vi[is] | R.vi[it]); return -1;
        }
        if (f < 0x3C)
            break;
        uint32_t s = (((w >> 6) & 31) << 2) | (w & 3);
        int fsf = (w >> 21) & 3, ftf = (w >> 23) & 3;
        switch (s) {
        case 0x30: SetVf(it, dest, R.vf[is]); return -1;  // MOVE
        case 0x31: {
            Vec v = R.vf[is], r = {{v.f[1], v.f[2], v.f[3], v.f[0]}};
            SetVf(it, dest, r);
            return -1;
        }
        case 0x34: SetVf(it, dest, Load(R.vi[is])); SetVi(is, R.vi[is] + 1); return -1;
        case 0x35: Store(R.vi[it], dest, R.vf[is]); SetVi(it, R.vi[it] + 1); return -1;
        case 0x36: SetVi(is, R.vi[is] - 1); SetVf(it, dest, Load(R.vi[is])); return -1;
        case 0x37: SetVi(it, R.vi[it] - 1); Store(R.vi[it], dest, R.vf[is]); return -1;
        case 0x38: case 0x39: case 0x3A: SetQ(QValue(w), QLatency(w)); return -1;
        case 0x3B: return -1;  // WAITQ (the stall is in Stall)
        case 0x3C: SetVi(it, AsU(R.vf[is].f[fsf])); return -1;
        case 0x3D: {
            Vec v;
            for (int k = 0; k < 4; k++)
                v.f[k] = AsF((uint32_t)(int32_t)Vi(is));
            SetVf(it, dest, v);
            return -1;
        }
        case 0x3E:
            SetVi(it, *(uint32_t *)(Mem(R.vi[is]) + FirstField(dest) * 4));
            Pipe.vi[it] = Pipe.cyc + 4;
            return -1;
        case 0x3F:
            for (int k = 0; k < 4; k++)
                if (Has(dest, k))
                    *(uint32_t *)(Mem(R.vi[is]) + k * 4) = R.vi[it];
            return -1;
        case 0x40: case 0x41: case 0x42: case 0x43: {
            if (s == 0x42)
                R.r = 0x3F800000 | (AsU(R.vf[is].f[fsf]) & 0x7FFFFF);
            else if (s == 0x43)
                R.r = 0x3F800000 | ((R.r ^ AsU(R.vf[is].f[fsf])) & 0x7FFFFF);
            else {
                if (s == 0x40) {
                    uint32_t x = R.r;
                    int b = ((x >> 4) ^ (x >> 22)) & 1;
                    R.r = 0x3F800000 | (((x << 1) | b) & 0x7FFFFF);
                }
                Vec v;
                for (int k = 0; k < 4; k++)
                    v.f[k] = AsF(R.r);
                SetVf(it, dest, v);
            }
            return -1;
        }
        case 0x64: {
            Vec v = {{R.p, R.p, R.p, R.p}};
            SetVf(it, dest, v);
            return -1;
        }
        case 0x68: SetVi(it, g_vu1.top); return -1;
        case 0x69: SetVi(it, g_vu1.itop); return -1;
        case 0x6C: Kick(R.vi[is]); return -1;
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x78: case 0x79:
        case 0x7A: case 0x7C: case 0x7D: case 0x7E:
            SetP(PValue(w), PLatency(w));
            return -1;
        case 0x7B: return -1;  // WAITP (the stall is in Stall)
        }
        break;
    }
    case 0x00: SetVf(it, dest, Load(Vi(is) + imm11)); return -1;
    case 0x01: Store(Vi(it) + imm11, dest, R.vf[is]); return -1;
    case 0x04:
        SetVi(it, *(uint32_t *)(Mem(Vi(is) + imm11) + FirstField(dest) * 4));
        Pipe.vi[it] = Pipe.cyc + 4;
        return -1;
    case 0x05:
        for (int k = 0; k < 4; k++)
            if (Has(dest, k))
                *(uint32_t *)(Mem(Vi(is) + imm11) + k * 4) = R.vi[it];
        return -1;
    case 0x08: SetVi(it, R.vi[is] + ((((w >> 21) & 15) << 11) | (w & 0x7FF))); return -1;
    case 0x09: SetVi(it, R.vi[is] - ((((w >> 21) & 15) << 11) | (w & 0x7FF))); return -1;
    case 0x10: SetVi(1, (R.clip & 0xFFFFFF) == (w & 0xFFFFFF)); return -1;
    case 0x11: Commit(~0ull); R.clip = w & 0xFFFFFF; return -1;
    case 0x12: SetVi(1, (R.clip & w & 0xFFFFFF) != 0); return -1;
    case 0x13: SetVi(1, ((R.clip | w) & 0xFFFFFF) == 0xFFFFFF); return -1;
    case 0x14: case 0x15: case 0x16: case 0x17: {
        uint32_t imm12 = (((w >> 21) & 1) << 11) | (w & 0x7FF);
        if (op == 0x14) SetVi(it, (R.status & 0xFFF) == imm12);
        else if (op == 0x15) R.status = (R.status & 0x3F) | (imm12 & 0xFC0);
        else if (op == 0x16) SetVi(it, R.status & imm12);
        else SetVi(it, R.status | imm12);
        return -1;
    }
    case 0x18: SetVi(it, R.mac == R.vi[is]); return -1;
    case 0x1A: SetVi(it, R.mac & R.vi[is]); return -1;
    case 0x1B: SetVi(it, R.mac | R.vi[is]); return -1;
    case 0x1C: SetVi(it, R.clip & 0xFFF); return -1;
    case 0x20: return target;
    case 0x21: SetVi(it, pc + 2); return target;
    case 0x24: return R.vi[is];
    case 0x25: { int t = R.vi[is]; SetVi(it, pc + 2); return t; }
    case 0x28: return Vi(it) == Vi(is) ? target : -1;
    case 0x29: return Vi(it) != Vi(is) ? target : -1;
    case 0x2C: return Vi(is) < 0 ? target : -1;
    case 0x2D: return Vi(is) > 0 ? target : -1;
    case 0x2E: return Vi(is) <= 0 ? target : -1;
    case 0x2F: return Vi(is) >= 0 ? target : -1;
    }
    static bool logged;
    if (!logged) {
        logged = true;
        port_log("VU1: lower op 0x%08x at 0x%x (%s)", w, pc, vu1_program_at(pc));
    }
    stop = false;
    return -1;
}


const __m128i kLaneMask[16] = {
#define L(d) _mm_set_epi32((d) & 1 ? -1 : 0, (d) & 2 ? -1 : 0, (d) & 4 ? -1 : 0, (d) & 8 ? -1 : 0)
    L(0), L(1), L(2), L(3), L(4), L(5), L(6), L(7), L(8), L(9), L(10), L(11), L(12), L(13), L(14), L(15),
#undef L
};
const uint8_t kRev4[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

// LQ/LQI's write: the dest fields of VF[it] from data memory.
inline void LoadVf(int it, uint32_t dest, uint32_t qaddr)
{
    if (!it)
        return;
    __m128 m = _mm_castsi128_ps(kLaneMask[dest]);
    __m128 v = _mm_loadu_ps((const float *)Mem(qaddr));
    float *r = R.vf[it].f;
    _mm_storeu_ps(r, _mm_or_ps(_mm_and_ps(m, v), _mm_andnot_ps(m, _mm_loadu_ps(r))));
}

// SQ/SQI: the dest fields of VF[is] to data memory.
inline void StoreVf(uint32_t qaddr, uint32_t dest, int is)
{
    __m128 m = _mm_castsi128_ps(kLaneMask[dest]);
    float *p = (float *)Mem(qaddr);
    _mm_storeu_ps(p, _mm_or_ps(_mm_and_ps(m, _mm_loadu_ps(R.vf[is].f)), _mm_andnot_ps(m, _mm_loadu_ps(p))));
}

// The upper half's common FMAC ops in SSE (as Upper computes them: one
// rounding per operation, Fix on the result, MAC flags of the dest fields).
// false: not one of them (Upper runs it).
inline bool UpperFast(const Dec &d, Pending &out)
{
    if (d.ucls == UC_SLOW)
        return false;
    if (d.ucls == UC_NOP) {
        out.reg = -1;
        out.flagKind = -1;
        return true;
    }
    __m128 S = _mm_loadu_ps(R.vf[d.ufs].f), T;
    if (d.usrc == US_BC)
        T = _mm_set1_ps(R.vf[d.uft].f[d.ubc]);
    else if (d.usrc == US_VEC)
        T = _mm_loadu_ps(R.vf[d.uft].f);
    else
        T = _mm_set1_ps(d.usrc == US_Q ? R.q : R.i);
    __m128 r;
    switch (d.ucls) {
    case UC_ADD: r = _mm_add_ps(S, T); break;
    case UC_SUB: r = _mm_sub_ps(S, T); break;
    case UC_MUL: r = _mm_mul_ps(S, T); break;
    case UC_MADD: r = _mm_add_ps(_mm_loadu_ps(R.acc.f), _mm_mul_ps(S, T)); break;
    default: r = _mm_sub_ps(_mm_loadu_ps(R.acc.f), _mm_mul_ps(S, T)); break;
    }
    // NaN/inf (exponent 255) or denormal (exponent 0, mantissa set) in a lane?
    {
        const __m128i expMask = _mm_set1_epi32(0x7F800000);
        __m128i u = _mm_castps_si128(r);
        __m128i e = _mm_and_si128(u, expMask);
        __m128i bad = _mm_or_si128(_mm_cmpeq_epi32(e, expMask),
                                   _mm_andnot_si128(_mm_cmpeq_epi32(_mm_slli_epi32(u, 1), _mm_setzero_si128()),
                                                    _mm_cmpeq_epi32(e, _mm_setzero_si128())));
        if (__builtin_expect(_mm_movemask_epi8(bad), 0))
            r = FixV(r);
    }
    // the lanes outside dest are blended away when it is applied
    _mm_storeu_ps(out.v.f, r);
    out.reg = d.udst;
    out.dest = d.udest;
    // MAC: Z and S of the dest fields
    out.flagKind = 0;
    out.flagV = MacOf(r, d.udest);
    return true;
}


void Decode(uint64_t ins, Dec &d)
{
    uint32_t lo = (uint32_t)ins, hi = (uint32_t)(ins >> 32);
    bool hasLower = !(hi & 0x80000000u);
    d = Dec{};
    DecodeStall(hi, lo, hasLower, d);
    DecodeUpper(hi, d);
    DecodeLower(lo, hasLower, d);
    if (d.ucls >= UC_ADD && d.usrc == US_Q)
        d.qpRead = true;
    d.word = ins;
    d.valid = true;
}

// As Upper sets out.reg/out.dest/out.flagKind.
void UpperWrites(uint32_t hi, int &reg, uint32_t &dest, int &flagKind)
{
    dest = (hi >> 21) & 15;
    int ft = (hi >> 16) & 31, fd = (hi >> 6) & 31;
    uint32_t op = hi & 63;
    reg = -1;
    flagKind = -1;
    if (op < 0x1C) {
        reg = fd;
        flagKind = (op >> 2) == 4 || (op >> 2) == 5 ? -1 : 0;  // MAX/MINI: no flags
        return;
    }
    if (op < 0x30) {
        reg = fd;
        flagKind = op == 0x1D || op == 0x1F || op == 0x2B || op == 0x2F ? -1 : 0;
        return;
    }
    if (op < 0x3C)
        return;
    uint32_t s = (((hi >> 6) & 31) << 2) | (hi & 3);
    if (s < 0x10 || (s >= 0x18 && s < 0x1D) || s == 0x1E || (s >= 0x20 && s < 0x2B) ||
        s == 0x2C || s == 0x2D || s == 0x2E) {
        reg = 32;
        flagKind = 0;
    } else if ((s >= 0x10 && s < 0x18) || s == 0x1D) {
        reg = ft;  // ITOF/FTOI/ABS
    } else if (s == 0x1F) {
        flagKind = 1;  // CLIP
    }
}

// The differential mode (PORT_VU1_DIFF): XGKICKs recorded (the packet's
// address and a hash of data memory), sent only by the run that counts.
struct KickRec {
    uint32_t qaddr;
    uint64_t hash;
    bool operator==(const KickRec &o) const { return qaddr == o.qaddr && hash == o.hash; }
};
std::vector<KickRec> *g_kickLog;
bool g_kickSend = true;

uint64_t HashData()
{
    const uint64_t *w = (const uint64_t *)g_vu1.data;
    uint64_t h = 0x9E3779B97F4A7C15ull;
    for (int k = 0; k < 2048; k++)
        h = (h ^ w[k]) * 0x100000001B3ull + (h >> 29);
    return h;
}

// Widescreen: the running program draws in screen coordinates (the menus,
// the HUD), so its packets are squeezed to their own proportions
// (gs_gif_kick). Set by vu1_run for the 2D renderers.
bool g_run2d;
// The GS frame a terrain or water program last ran in: while that is the
// current frame a race is on screen, and the 2D is its HUD.
uint64_t g_sceneFrame = ~0ull;

void Kick(uint32_t qaddr)
{
    if (g_kickLog)
        g_kickLog->push_back({qaddr, HashData()});
    if (g_kickSend)
        gs_gif_kick(g_vu1.data, qaddr, g_run2d ? port_widescreen_scale() : 1.0f,
                    gs_frame_seq() - g_sceneFrame <= 2);
}

const char *g_names[2048];

// A program that runs this long has lost its way (a retail program runs
// for at most a frame's worth of VU1 cycles, ~2.5M at 150 MHz).
const int kMaxSteps = getenv("PORT_VU1_MAX") ? atoi(getenv("PORT_VU1_MAX")) : 4000000;

// PORT_VU1_STATS=1: the longest run of each program.
bool g_vu1Stats = getenv("PORT_VU1_STATS") != nullptr;
// PORT_VU1_WATCH=pc[,n]: log the VI registers the first n times pc runs
// (PORT_VU1_WATCH_PROG=name: only in that program).
const char *g_watchProg = getenv("PORT_VU1_WATCH_PROG");
int g_watchPc = getenv("PORT_VU1_WATCH") ? (int)strtol(getenv("PORT_VU1_WATCH"), nullptr, 16) : -1;
int g_watchLeft = getenv("PORT_VU1_WATCH") && strchr(getenv("PORT_VU1_WATCH"), ',')
                      ? atoi(strchr(getenv("PORT_VU1_WATCH"), ',') + 1) : 200;

uint64_t g_vu1Steps;  // instructions run (the status line)

// Which engine runs the programs: the recompiler (default), the
// interpreter (PORT_VU1_INTERP=1; also with PORT_VU1_NOTIMING or
// PORT_VU1_WATCH, which only it implements), or both, compared
// (PORT_VU1_DIFF=1).
enum { kRec, kInterp, kDiff };
const int g_engine = getenv("PORT_VU1_DIFF") ? kDiff
                     : getenv("PORT_VU1_INTERP") || getenv("PORT_VU1_NOTIMING") || getenv("PORT_VU1_WATCH") ? kInterp
                                                                                                           : kRec;

bool Interp(uint32_t &pc, bool untilClean, int &steps)
{
    int branch = -1;       // a taken branch's target, after its delay slot
    bool ending = false;   // [E] seen: one more instruction
    static uint16_t trace[64];
    static uint32_t hist[2048];
    const int histFrom = kMaxSteps - (1 << 20);
    const bool watch = g_watchPc >= 0;
    for (; steps < kMaxSteps; steps++) {
        pc &= 2047;
        if (__builtin_expect(steps >= histFrom, 0)) {
            // a runaway: its last PCs and the hottest ones
            trace[steps & 63] = (uint16_t)pc;
            hist[pc]++;
        }
        if (__builtin_expect(watch, 0) && (int)pc == g_watchPc && g_watchLeft > 0 &&
            (!g_watchProg || strstr(vu1_program_at(pc), g_watchProg))) {
            g_watchLeft--;
            port_log("VU1 watch %x step %d: vi1-15 %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x", pc, steps, R.vi[1],
                     R.vi[2], R.vi[3], R.vi[4], R.vi[5], R.vi[6], R.vi[7], R.vi[8], R.vi[9], R.vi[10], R.vi[11],
                     R.vi[12], R.vi[13], R.vi[14], R.vi[15]);
        }
        uint64_t ins = g_vu1.micro[pc];
        uint32_t lo = (uint32_t)ins, hi = (uint32_t)(ins >> 32);
        Dec &d = g_dec[pc];
        if (__builtin_expect(!d.valid || d.word != ins, 0))
            Decode(ins, d);
        // stall until the sources have landed, then land what has finished
        Pipe.cyc = Stall(d);
        if (d.qpRead)
            CommitQP(Pipe.cyc);
        if (d.flagRead)
            CommitFlags(Pipe.cyc);
        Pending up;
        if (!UpperFast(d, up))
            Upper(hi, up);
        if (up.flagKind >= 0)
            PushFlag((uint8_t)up.flagKind, up.flagV);
        int target = -1;
        bool unused = false;
        // the lower half ([I]: none, the lower word is the I register's new
        // value, after the upper ran)
        switch (d.lk) {
        case LK_NONE: case LK_NOP: break;
        case LK_LQ: LoadVf(d.lit, d.ldest, (uint32_t)(Vi(d.lis) + d.limm)); break;
        case LK_SQ: StoreVf((uint32_t)(Vi(d.lit) + d.limm), d.ldest, d.lis); break;
        case LK_LQI: LoadVf(d.lit, d.ldest, R.vi[d.lis]); SetVi(d.lis, R.vi[d.lis] + 1); break;
        case LK_SQI: StoreVf(R.vi[d.lit], d.ldest, d.lis); SetVi(d.lit, R.vi[d.lit] + 1); break;
        case LK_IADDIU: SetVi(d.lit, R.vi[d.lis] + d.limm); break;
        case LK_ISUBIU: SetVi(d.lit, R.vi[d.lis] - d.limm); break;
        default: target = Lower(lo, pc, unused); break;
        }
        if (d.loVf > 0)
            Lands(d.loVf, d.ldest);
        if (up.reg >= 0 && up.reg != 0) {
            float *dst = up.reg == 32 ? R.acc.f : R.vf[up.reg].f;
            __m128 m = _mm_castsi128_ps(kLaneMask[up.dest & 15]);
            _mm_storeu_ps(dst, _mm_or_ps(_mm_and_ps(m, _mm_loadu_ps(up.v.f)), _mm_andnot_ps(m, _mm_loadu_ps(dst))));
            if (up.reg != 32)
                Lands(up.reg, up.dest);
        }
        Pipe.cyc++;
        if (hi & 0x80000000u)
            R.i = AsF(lo);  // (VI0 stays 0: SetVi ignores it)
        bool endNow = ending;
        if (hi & 0x40000000u)
            ending = true;
        uint32_t next = pc + 1;
        if (branch >= 0) {
            next = (uint32_t)branch;
            branch = -1;
        }
        if (target >= 0)
            branch = target;
        pc = next;
        if (endNow) {
            g_vu1.pc = pc & 2047;
            steps++;
            return true;
        }
        if (untilClean && branch < 0 && !ending) {
            pc &= 2047;
            steps++;
            return false;
        }
    }
    port_log("VU1: %s ran %d instructions without [E]", vu1_program_at(pc), kMaxSteps);
    static int dumps;
    if (dumps++ < 2) {
        char buf[1024];
        int n = 0;
        for (int k = 0; k < 64; k++)
            n += snprintf(buf + n, sizeof buf - n, "%x ", trace[(steps + k) & 63]);
        port_log("VU1: last pcs: %s", buf);
        n = 0;
        for (int k = 0; k < 16; k++)
            n += snprintf(buf + n, sizeof buf - n, "vi%d=%x ", k, R.vi[k]);
        port_log("VU1: %s", buf);
        // the hottest instructions of the last 1M
        n = 0;
        for (int k = 0; k < 2048; k++)
            if (hist[k] > 4096)
                n += snprintf(buf + n, sizeof buf - n, "%x:%u ", k, hist[k]);
        port_log("VU1: hot: %s", buf);
    }
    memset(hist, 0, sizeof hist);
    return true;
}

// PORT_VU1_DIFF: runs the program with the recompiler, then from the same
// state with the interpreter (whose results stand), and compares the
// registers (everything landed), data memory, the end PC and the XGKICKs.
uint64_t g_diffRuns, g_diffBad;

void DiffRun(uint32_t pc, int &steps)
{
    static Regs r0, rA;
    static Timing p0;
    alignas(16) static uint8_t d0[16384], dA[16384];
    static std::vector<KickRec> kA, kB;
    r0 = R;
    p0 = Pipe;
    memcpy(d0, g_vu1.data, sizeof d0);
    uint32_t pc0 = g_vu1.pc;
    kA.clear();
    kB.clear();
    g_kickLog = &kA;
    g_kickSend = false;
    int sA = 0;
    RecRun(pc, sA);
    uint32_t endA = g_vu1.pc;
    rA = R;
    memcpy(dA, g_vu1.data, sizeof dA);
    R = r0;
    Pipe = p0;
    memcpy(g_vu1.data, d0, sizeof d0);
    g_vu1.pc = pc0;
    g_kickLog = &kB;
    g_kickSend = true;
    uint32_t pcB = pc;
    Interp(pcB, false, steps);
    Commit(~0ull);
    g_kickLog = nullptr;
    g_diffRuns++;
    char what[512];
    int n = 0;
    what[0] = 0;
    auto add = [&](const char *fmt, auto... a) {
        if (n < 400)
            n += snprintf(what + n, sizeof what - n, fmt, a...);
    };
    for (int r = 1; r < 32; r++)
        for (int k = 0; k < 4; k++)
            if (AsU(rA.vf[r].f[k]) != AsU(R.vf[r].f[k]))
                add(" vf%d.%c %08x/%08x", r, "xyzw"[k], AsU(rA.vf[r].f[k]), AsU(R.vf[r].f[k]));
    for (int r = 1; r < 16; r++)
        if (rA.vi[r] != R.vi[r])
            add(" vi%d %x/%x", r, rA.vi[r], R.vi[r]);
    for (int k = 0; k < 4; k++)
        if (AsU(rA.acc.f[k]) != AsU(R.acc.f[k]))
            add(" acc.%c %08x/%08x", "xyzw"[k], AsU(rA.acc.f[k]), AsU(R.acc.f[k]));
    if (AsU(rA.q) != AsU(R.q)) add(" q %08x/%08x", AsU(rA.q), AsU(R.q));
    if (AsU(rA.p) != AsU(R.p)) add(" p %08x/%08x", AsU(rA.p), AsU(R.p));
    if (AsU(rA.i) != AsU(R.i)) add(" i %08x/%08x", AsU(rA.i), AsU(R.i));
    if (rA.r != R.r) add(" r %x/%x", rA.r, R.r);
    if (rA.mac != R.mac) add(" mac %x/%x", rA.mac, R.mac);
    if (rA.status != R.status) add(" status %x/%x", rA.status, R.status);
    if (rA.clip != R.clip) add(" clip %x/%x", rA.clip, R.clip);
    if (endA != g_vu1.pc) add(" end pc %x/%x", endA, g_vu1.pc);
    if (sA != steps) add(" steps %d/%d", sA, steps);
    if (memcmp(dA, g_vu1.data, sizeof dA)) {
        int q = 0;
        while (memcmp(dA + q * 16, g_vu1.data + q * 16, 16))
            q++;
        add(" data (first qword 0x%x)", q);
    }
    if (kA.size() != kB.size()) {
        add(" kicks %zu/%zu", kA.size(), kB.size());
    } else {
        for (size_t k = 0; k < kA.size(); k++)
            if (!(kA[k] == kB[k])) {
                add(" kick %zu (at 0x%x/0x%x)", k, kA[k].qaddr, kB[k].qaddr);
                break;
            }
    }
    if (n) {
        g_diffBad++;
        if (g_diffBad <= 50)
            port_log("VU1 diff: %s at 0x%x (run %llu):%s", vu1_program_at(pc), pc, (unsigned long long)g_diffRuns, what);
    }
    if ((g_diffRuns & 0xFFFF) == 0)
        port_log("VU1 diff: %llu runs, %llu differ", (unsigned long long)g_diffRuns, (unsigned long long)g_diffBad);
}

} // namespace vu1i

using namespace vu1i;

void vu1_note_upload(uint32_t addr, uint32_t n, const void *src)
{
    const char *name = port_microprogram_containing(src);
    for (uint32_t k = 0; k < n; k++)
        g_names[(addr + k) & 2047] = name;
}

const char *vu1_program_at(uint32_t pc)
{
    const char *n = g_names[pc & 2047];
    return n ? n : "?";
}

uint64_t vu1_instructions()
{
    return __atomic_load_n(&g_vu1Steps, __ATOMIC_RELAXED);
}

// The microprograms that draw in screen coordinates.
static bool Is2D(const char *name)
{
    return name && (!strcmp(name, "Quad2DRendererCode") || !strcmp(name, "Rectangle2DRendererCode") ||
                    !strcmp(name, "Triangle2DRendererCode") || !strcmp(name, "Polyline2DRendererCode"));
}

void vu1_run(uint32_t pc)
{
    uint32_t entry = pc;
    const char *program = g_names[pc & 2047];
    g_run2d = Is2D(program);
    if (program && (!strncmp(program, "Terrain", 7) || !strncmp(program, "Water", 5)))
        g_sceneFrame = gs_frame_seq();
    CommitAll();
    R.vf[0].f[0] = R.vf[0].f[1] = R.vf[0].f[2] = 0;
    R.vf[0].f[3] = 1.0f;
    R.vi[0] = 0;
    int steps = 0;
    if (g_engine == kRec)
        RecRun(pc, steps);
    else if (g_engine == kInterp)
        Interp(pc, false, steps);
    else
        DiffRun(pc, steps);
    __atomic_add_fetch(&g_vu1Steps, (uint64_t)steps, __ATOMIC_RELAXED);
    if (g_vu1Stats) {
        static int longest[2048];
        if (steps > longest[entry & 2047] && steps > 100000) {
            longest[entry & 2047] = steps;
            port_log("VU1: %s at 0x%x: %d instructions", vu1_program_at(entry), entry, steps);
        }
        static uint64_t runs;
        if ((++runs & 0x3FFFF) == 0 && g_engine != kInterp)
            RecStats();
    }
}
