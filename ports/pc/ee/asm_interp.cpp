// EE inline-asm interpreter for the PC port.
//
// The game's inline asm (VU0 macro-mode vector math, COP1 helpers, a few MMI
// halfword loops) is kept verbatim in src/. tools/port/rewrite.py turns every
// asm statement into a port_asm() call with its operands; this file parses the
// asm text once per site and runs it against an emulated EE register file.
// VU0 state is global, as on the hardware: several call sites read registers
// an earlier statement left (soultree.cpp's `sqc2 vf20`).
//
// Host-side differences from the EE, on purpose:
// - GPRs hold host pointers, so addi/addiu are 64-bit adds (no sign
//   extension of a 32-bit result), and lq/sq/lqc2/sqc2 don't mask the
//   address to 16 bytes.
// - bc0f (COP0 condition = DMA done) asks ports/pc/dma.cpp whether the channels
//   D_PCR selects have finished (STR clear).
#include "port_asm.h"
#include "port.h"
#include "port_ptr32.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

union Q128 {
    uint8_t b[16];
    int16_t h[8];
    uint16_t uh[8];
    int32_t w[4];
    uint32_t uw[4];
    int64_t d[2];
    uint64_t ud[2];
    float f[4];
};

const int kSlots = 32; // %0 .. %31 live in registers 32.. of each file

struct EEState {
    Q128 gpr[32 + kSlots];
    uint32_t fpr[32 + kSlots];
    uint32_t fcr0, fcr31;
    float vf[32][4];
    float acc[4];
    float q, i;
    uint32_t clip;
    Q128 hi, lo;
    unsigned sa; // in bytes
};

// One per thread: the timer thread runs the game's interrupt handlers
// (ports/pc/hw.cpp), which have asm of their own.
thread_local EEState g_ee;

enum RegKind { R_NONE, R_GPR, R_FPR, R_SLOT, R_VF, R_ACC, R_Q, R_VI, R_IMM, R_LABEL };

struct Arg {
    RegKind kind;
    int idx;       // register number, slot number, label pc
    int field;     // vfNx: broadcast field 0-3, else -1
    int64_t imm;   // immediate, or memory offset
    bool mem;      // off(base)
};

enum Op {
    OP_NOP, OP_LQC2, OP_SQC2, OP_QMFC2, OP_QMTC2, OP_CFC2,
    OP_VARITH, OP_VOPMULA, OP_VOPMSUB, OP_VRSQRT, OP_VDIV, OP_VSQRT,
    OP_VMOVE, OP_VCLIPW,
    OP_MINS, OP_MAXS, OP_ABSS, OP_SQRTS, OP_RSQRTS, OP_ADDS, OP_SUBS,
    OP_MULS, OP_DIVS, OP_NEGS, OP_MOVS, OP_MTC1, OP_MFC1, OP_LWC1, OP_SWC1,
    OP_CVTWS, OP_CVTSW, OP_CFC1, OP_CTC1,
    OP_ADDI, OP_LUI, OP_ORI, OP_ANDI, OP_AND, OP_OR, OP_ADDU, OP_SUBU,
    OP_LW, OP_SW, OP_LQ, OP_SQ,
    OP_B, OP_BNE, OP_BEQ, OP_BLEZ, OP_BGTZ, OP_BC0F,
    OP_PADDSH, OP_PSUBH, OP_PMAXH, OP_PMINH, OP_PCGTH, OP_PSRAH, OP_PMULTH,
    OP_PMADDH, OP_PMFLO, OP_PMFHI, OP_PMFHLSH, OP_MTSAH, OP_QFSRV, OP_PADDUB,
    OP_PEXTLH, OP_PEXTUH,
};

// VARITH: v{add,sub,mul,madd,msub}[a][x|y|z|w|q|i]
enum { VA_ADD, VA_SUB, VA_MUL, VA_MADD, VA_MSUB };

struct Insn {
    Op op;
    int dest;      // VU dest mask, bit 0 = x
    int varith;    // VA_*
    bool toAcc;    // vmula and friends
    int bc;        // -1 vector, 0-3 broadcast field, 4 Q, 5 I
    bool delaySlot; // branch assembled under .set noreorder
    std::vector<Arg> args;
    std::string text;
};

struct Prog {
    std::vector<Insn> insns;
};

[[noreturn]] void Fail(const char *what, const std::string &text)
{
    fprintf(stderr, "port_asm: %s: `%s`\n", what, text.c_str());
    abort();
}

std::string Trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

int GprByName(const std::string &n)
{
    static const char *names[32] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
    };
    for (int r = 0; r < 32; r++)
        if (n == names[r])
            return r;
    return -1;
}

int FieldOf(char c)
{
    switch (c) {
    case 'x': return 0;
    case 'y': return 1;
    case 'z': return 2;
    case 'w': return 3;
    }
    return -1;
}

Arg ParseReg(const std::string &tok, const std::string &text)
{
    Arg a = {R_NONE, 0, -1, 0, false};
    if (tok.empty())
        Fail("empty operand", text);
    if (tok[0] == '%') {
        a.kind = R_SLOT;
        a.idx = atoi(tok.c_str() + 1);
        if (a.idx >= kSlots)
            Fail("operand number too large", text);
        return a;
    }
    if (tok[0] == '$') {
        std::string n = tok.substr(1);
        if (n.size() > 1 && n[0] == 'f' && isdigit((unsigned char)n[1])) {
            a.kind = R_FPR;
            a.idx = atoi(n.c_str() + 1);
            return a;
        }
        a.kind = R_GPR;
        a.idx = isdigit((unsigned char)n[0]) ? atoi(n.c_str()) : GprByName(n);
        if (a.idx < 0 || a.idx > 31)
            Fail("bad register", text);
        return a;
    }
    if (tok == "ACC") {
        a.kind = R_ACC;
        return a;
    }
    if (tok == "Q") {
        a.kind = R_Q;
        return a;
    }
    if (tok.compare(0, 2, "vf") == 0) {
        a.kind = R_VF;
        size_t p = 2;
        a.idx = 0;
        while (p < tok.size() && isdigit((unsigned char)tok[p]))
            a.idx = a.idx * 10 + (tok[p++] - '0');
        if (p < tok.size())
            a.field = FieldOf(tok[p]);
        return a;
    }
    if (tok.compare(0, 2, "vi") == 0) {
        a.kind = R_VI;
        a.idx = atoi(tok.c_str() + 2);
        return a;
    }
    if (isdigit((unsigned char)tok[0]) || tok[0] == '-') {
        a.kind = R_IMM;
        a.imm = strtoll(tok.c_str(), NULL, 0);
        return a;
    }
    a.kind = R_LABEL; // resolved after parsing
    return a;
}

Arg ParseArg(const std::string &tok, const std::string &text)
{
    size_t lp = tok.find('(');
    if (lp == std::string::npos)
        return ParseReg(tok, text);
    Arg a = ParseReg(Trim(tok.substr(lp + 1, tok.find(')') - lp - 1)), text);
    std::string off = Trim(tok.substr(0, lp));
    a.imm = off.empty() ? 0 : strtoll(off.c_str(), NULL, 0);
    a.mem = true;
    return a;
}

int DestMask(const std::string &suffix)
{
    int m = 0;
    for (size_t k = 0; k < suffix.size(); k++)
        m |= 1 << FieldOf(suffix[k]);
    return m;
}

bool ParseVArith(const std::string &mn, Insn &in)
{
    static const struct { const char *name; int va; } bases[] = {
        {"madd", VA_MADD}, {"msub", VA_MSUB}, {"add", VA_ADD}, {"sub", VA_SUB}, {"mul", VA_MUL},
    };
    if (mn.empty() || mn[0] != 'v')
        return false;
    for (size_t b = 0; b < sizeof bases / sizeof bases[0]; b++) {
        size_t n = strlen(bases[b].name);
        if (mn.compare(1, n, bases[b].name) != 0)
            continue;
        std::string rest = mn.substr(1 + n);
        in.toAcc = false;
        in.bc = -1;
        if (!rest.empty() && rest[0] == 'a') {
            in.toAcc = true;
            rest = rest.substr(1);
        }
        if (rest.size() > 1)
            return false;
        if (rest.size() == 1) {
            if (rest[0] == 'q')
                in.bc = 4;
            else if (rest[0] == 'i')
                in.bc = 5;
            else if ((in.bc = FieldOf(rest[0])) < 0)
                return false;
        }
        in.op = OP_VARITH;
        in.varith = bases[b].va;
        return true;
    }
    return false;
}

struct LabelRef {
    size_t insn, arg;
    std::string name;
};

Prog *Parse(const char *src)
{
    Prog *p = new Prog;
    std::map<std::string, int> labels;           // named labels
    std::vector<std::pair<std::string, int> > numbered; // "1:" in order
    std::vector<LabelRef> refs;
    bool noreorder = false;

    std::string all(src);
    size_t pos = 0;
    while (pos <= all.size()) {
        size_t end = all.find_first_of(";\n", pos);
        if (end == std::string::npos)
            end = all.size();
        std::string stmt = Trim(all.substr(pos, end - pos));
        pos = end + 1;
        // Leading labels: "name:" or "1:"
        for (;;) {
            size_t colon = stmt.find(':');
            if (colon == std::string::npos)
                break;
            std::string lab = Trim(stmt.substr(0, colon));
            if (lab.empty() || lab.find_first_of(" \t,(") != std::string::npos)
                break;
            if (isdigit((unsigned char)lab[0]))
                numbered.push_back(std::make_pair(lab, (int)p->insns.size()));
            else
                labels[lab] = (int)p->insns.size();
            stmt = Trim(stmt.substr(colon + 1));
        }
        if (stmt.empty())
            continue;
        size_t sp = stmt.find_first_of(" \t");
        std::string mn = stmt.substr(0, sp);
        std::string ops = sp == std::string::npos ? "" : Trim(stmt.substr(sp));
        if (mn == ".set") {
            if (ops == "noreorder")
                noreorder = true;
            else if (ops == "reorder")
                noreorder = false;
            continue;
        }
        if (mn[0] == '.')
            continue; // .p2align and the like

        Insn in;
        in.text = stmt;
        in.dest = 0;
        in.varith = 0;
        in.toAcc = false;
        in.bc = -1;
        in.delaySlot = noreorder;
        std::string base = mn, suffix;
        size_t dot = mn.find('.');
        if (dot != std::string::npos) {
            base = mn.substr(0, dot);
            suffix = mn.substr(dot + 1);
        }
        if (base[0] == 'v' && !suffix.empty())
            in.dest = DestMask(suffix);

        static const std::map<std::string, Op> table = [] {
            std::map<std::string, Op> table;
            table["nop"] = OP_NOP; table["sync"] = OP_NOP; table["sync.p"] = OP_NOP;
            table["ei"] = OP_NOP; table["di"] = OP_NOP; table["vnop"] = OP_NOP;
            table["vwaitq"] = OP_NOP;
            table["lqc2"] = OP_LQC2; table["sqc2"] = OP_SQC2;
            table["qmfc2"] = OP_QMFC2; table["qmfc2.ni"] = OP_QMFC2; table["qmfc2.i"] = OP_QMFC2;
            table["qmtc2"] = OP_QMTC2; table["qmtc2.ni"] = OP_QMTC2; table["qmtc2.i"] = OP_QMTC2;
            table["cfc2"] = OP_CFC2; table["cfc2.ni"] = OP_CFC2;
            table["vopmula"] = OP_VOPMULA; table["vopmsub"] = OP_VOPMSUB;
            table["vrsqrt"] = OP_VRSQRT; table["vdiv"] = OP_VDIV; table["vsqrt"] = OP_VSQRT;
            table["vmove"] = OP_VMOVE; table["vclipw"] = OP_VCLIPW;
            table["min.s"] = OP_MINS; table["max.s"] = OP_MAXS; table["abs.s"] = OP_ABSS;
            table["sqrt.s"] = OP_SQRTS; table["rsqrt.s"] = OP_RSQRTS; table["add.s"] = OP_ADDS;
            table["sub.s"] = OP_SUBS; table["mul.s"] = OP_MULS; table["div.s"] = OP_DIVS;
            table["neg.s"] = OP_NEGS; table["mov.s"] = OP_MOVS;
            table["mtc1"] = OP_MTC1; table["mfc1"] = OP_MFC1; table["lwc1"] = OP_LWC1;
            table["swc1"] = OP_SWC1; table["cvt.w.s"] = OP_CVTWS; table["cvt.s.w"] = OP_CVTSW;
            table["cfc1"] = OP_CFC1; table["ctc1"] = OP_CTC1;
            table["addi"] = OP_ADDI; table["addiu"] = OP_ADDI; table["daddiu"] = OP_ADDI;
            table["lui"] = OP_LUI; table["ori"] = OP_ORI; table["andi"] = OP_ANDI;
            table["and"] = OP_AND; table["or"] = OP_OR; table["addu"] = OP_ADDU;
            table["daddu"] = OP_ADDU; table["subu"] = OP_SUBU; table["dsubu"] = OP_SUBU;
            table["lw"] = OP_LW; table["sw"] = OP_SW; table["lq"] = OP_LQ; table["sq"] = OP_SQ;
            table["b"] = OP_B; table["bne"] = OP_BNE; table["beq"] = OP_BEQ;
            table["blez"] = OP_BLEZ; table["bgtz"] = OP_BGTZ; table["bc0f"] = OP_BC0F;
            table["paddsh"] = OP_PADDSH; table["psubh"] = OP_PSUBH; table["pmaxh"] = OP_PMAXH;
            table["pminh"] = OP_PMINH; table["pcgth"] = OP_PCGTH; table["psrah"] = OP_PSRAH;
            table["pmulth"] = OP_PMULTH; table["pmaddh"] = OP_PMADDH; table["pmflo"] = OP_PMFLO;
            table["pmfhi"] = OP_PMFHI; table["pmfhl.sh"] = OP_PMFHLSH; table["mtsah"] = OP_MTSAH;
            table["qfsrv"] = OP_QFSRV; table["paddub"] = OP_PADDUB; table["pextlh"] = OP_PEXTLH;
            table["pextuh"] = OP_PEXTUH;
            return table;
        }();
        std::map<std::string, Op>::const_iterator it = table.find(mn);
        if (it == table.end())
            it = table.find(base);
        if (it != table.end())
            in.op = it->second;
        else if (!ParseVArith(base, in))
            Fail("unknown instruction", stmt);

        size_t q = 0;
        while (!ops.empty() && q <= ops.size()) {
            size_t comma = ops.find(',', q);
            if (comma == std::string::npos)
                comma = ops.size();
            std::string tok = Trim(ops.substr(q, comma - q));
            Arg a = ParseArg(tok, stmt);
            if (a.kind == R_LABEL) {
                LabelRef r = {p->insns.size(), in.args.size(), tok};
                refs.push_back(r);
            }
            in.args.push_back(a);
            q = comma + 1;
        }
        p->insns.push_back(in);
    }

    for (size_t r = 0; r < refs.size(); r++) {
        const std::string &n = refs[r].name;
        int target = -1;
        char last = n[n.size() - 1];
        if (isdigit((unsigned char)n[0]) && (last == 'b' || last == 'f')) {
            std::string num = n.substr(0, n.size() - 1);
            int at = (int)refs[r].insn;
            if (last == 'b') {
                for (size_t k = 0; k < numbered.size(); k++)
                    if (numbered[k].first == num && numbered[k].second <= at)
                        target = numbered[k].second;
            } else {
                for (size_t k = numbered.size(); k-- > 0;)
                    if (numbered[k].first == num && numbered[k].second > at)
                        target = numbered[k].second;
            }
        } else if (labels.count(n)) {
            target = labels[n];
        }
        if (target < 0)
            Fail("unknown label", n);
        p->insns[refs[r].insn].args[refs[r].arg].idx = target;
    }
    return p;
}

// ---- execution ----

Q128 &G(const Arg &a)
{
    static Q128 sink;
    if (a.kind == R_SLOT)
        return g_ee.gpr[32 + a.idx];
    if (a.kind == R_GPR) {
        if (a.idx == 0) {
            memset(&sink, 0, sizeof sink);
            return sink;
        }
        return g_ee.gpr[a.idx];
    }
    Fail("expected a GPR", "");
}

Q128 GR(const Arg &a)
{
    if (a.kind == R_GPR && a.idx == 0) {
        Q128 z;
        memset(&z, 0, sizeof z);
        return z;
    }
    return G(a);
}

uint32_t &F(const Arg &a)
{
    if (a.kind == R_SLOT)
        return g_ee.fpr[32 + a.idx];
    if (a.kind == R_FPR)
        return g_ee.fpr[a.idx];
    Fail("expected an FPR", "");
}

float FF(const Arg &a)
{
    float f;
    memcpy(&f, &F(a), 4);
    return f;
}

void SetF(const Arg &a, float f)
{
    memcpy(&F(a), &f, 4);
}

void *Addr(const Arg &a)
{
    // (a register may hold a 32-bit game address: a literal the asm built)
    return (void *)port_host_addr(GR(a).ud[0] + a.imm);
}

float *VF(const Arg &a)
{
    static float scratch[4];
    if (a.kind == R_ACC)
        return g_ee.acc;
    if (a.kind != R_VF)
        Fail("expected a VF register", "");
    if (a.idx == 0) { // vf0 is (0, 0, 0, 1)
        scratch[0] = scratch[1] = scratch[2] = 0.0f;
        scratch[3] = 1.0f;
        return scratch;
    }
    return g_ee.vf[a.idx];
}

float VField(const Arg &a)
{
    return VF(a)[a.field < 0 ? 0 : a.field];
}

// The EE's FPU and the VUs have no infinities or NaNs: an overflow is
// +-max, and so is a division by zero (sign: the operands' signs XORed).
float Fix(float f)
{
    if (std::isinf(f))
        return std::copysign(3.4028235e38f, f);
    if (std::isnan(f))
        return std::signbit(f) ? -3.4028235e38f : 3.4028235e38f;
    return f;
}

float Div(float a, float b)
{
    if (b == 0.0f)
        return std::signbit(a) != std::signbit(b) ? -3.4028235e38f : 3.4028235e38f;
    return Fix(a / b);
}

void VStore(float *d, const float *v, int mask)
{
    for (int k = 0; k < 4; k++)
        if (mask & (1 << k))
            d[k] = Fix(v[k]);
}

int16_t SatH(int32_t v)
{
    return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v;
}

void Run(const Prog *p)
{
    size_t pc = 0;
    long budget = 4000000000L;   // ~a minute: waits on a slow software frame are legit
    while (pc < p->insns.size()) {
        if (--budget == 0) {
            // which channels a bc0f wait is stuck on
            for (int ch = 0; ch < 10; ch++) {
                static const uintptr_t kBase[10] = {0x10008000, 0x10009000, 0x1000A000, 0x1000B000, 0x1000B400,
                                                    0x1000C000, 0x1000C400, 0x1000C800, 0x1000D000, 0x1000D400};
                uint32_t chcr = *(volatile uint32_t *)PORT_HW(kBase[ch]);
                if (chcr & 0x100)
                    port_log("DMA %s still running: CHCR %08x MADR %08x TADR %08x", port_dma_channel_name(ch), chcr,
                             *(volatile uint32_t *)PORT_HW(kBase[ch] + 0x10), *(volatile uint32_t *)PORT_HW(kBase[ch] + 0x30));
            }
            port_log("D_PCR %08x", *(volatile uint32_t *)PORT_HW(0x1000E020));
            Fail("runaway loop", p->insns[pc].text);
        }
        const Insn &in = p->insns[pc];
        const std::vector<Arg> &A = in.args;
        size_t next = pc + 1;
        int branchTo = -1;
        switch (in.op) {
        case OP_NOP:
            break;
        case OP_LQC2:
            memcpy(VF(A[0]), Addr(A[1]), 16);
            break;
        case OP_SQC2:
            memcpy(Addr(A[1]), VF(A[0]), 16);
            break;
        case OP_QMFC2:
            memcpy(G(A[0]).b, VF(A[1]), 16);
            break;
        case OP_QMTC2:
            if (A[1].idx != 0)
                memcpy(VF(A[1]), GR(A[0]).b, 16);
            break;
        case OP_CFC2: {
            Q128 &d = G(A[0]);
            int64_t v = A[1].idx == 18 ? (int64_t)g_ee.clip : 0;
            d.d[0] = v;
            break;
        }
        case OP_VARITH: {
            const float *s = VF(A[1]);
            float t[4], acc[4], out[4];
            for (int k = 0; k < 4; k++) {
                if (in.bc == 4)
                    t[k] = g_ee.q;
                else if (in.bc == 5)
                    t[k] = g_ee.i;
                else if (in.bc >= 0)
                    t[k] = VF(A[2])[in.bc];
                else
                    t[k] = VF(A[2])[k];
                acc[k] = g_ee.acc[k];
            }
            for (int k = 0; k < 4; k++) {
                switch (in.varith) {
                case VA_ADD: out[k] = s[k] + t[k]; break;
                case VA_SUB: out[k] = s[k] - t[k]; break;
                case VA_MUL: out[k] = s[k] * t[k]; break;
                case VA_MADD: out[k] = acc[k] + s[k] * t[k]; break;
                case VA_MSUB: out[k] = acc[k] - s[k] * t[k]; break;
                }
            }
            if (in.toAcc)
                VStore(g_ee.acc, out, in.dest);
            else if (A[0].idx != 0)
                VStore(VF(A[0]), out, in.dest);
            break;
        }
        case OP_VOPMULA: {
            const float *s = VF(A[1]), *t = VF(A[2]);
            float out[4] = {s[1] * t[2], s[2] * t[0], s[0] * t[1], 0.0f};
            VStore(g_ee.acc, out, 7);
            break;
        }
        case OP_VOPMSUB: {
            const float *s = VF(A[1]), *t = VF(A[2]);
            float out[4] = {g_ee.acc[0] - s[1] * t[2], g_ee.acc[1] - s[2] * t[0],
                            g_ee.acc[2] - s[0] * t[1], 0.0f};
            if (A[0].idx != 0)
                VStore(VF(A[0]), out, 7);
            break;
        }
        case OP_VRSQRT:
            g_ee.q = Div(VField(A[1]), sqrtf(fabsf(VField(A[2]))));
            break;
        case OP_VDIV:
            g_ee.q = Div(VField(A[1]), VField(A[2]));
            break;
        case OP_VSQRT:
            g_ee.q = sqrtf(fabsf(VField(A[1])));
            break;
        case OP_VMOVE:
            if (A[0].idx != 0)
                VStore(VF(A[0]), VF(A[1]), in.dest);
            break;
        case OP_VCLIPW: {
            const float *s = VF(A[0]);
            float w = fabsf(VField(A[1]));
            uint32_t flags = 0;
            for (int k = 0; k < 3; k++) {
                if (s[k] > w)
                    flags |= 1u << (2 * k);
                if (s[k] < -w)
                    flags |= 2u << (2 * k);
            }
            g_ee.clip = ((g_ee.clip << 6) | flags) & 0xFFFFFF;
            break;
        }
        case OP_MINS: SetF(A[0], fminf(FF(A[1]), FF(A[2]))); break;
        case OP_MAXS: SetF(A[0], fmaxf(FF(A[1]), FF(A[2]))); break;
        case OP_ABSS: SetF(A[0], fabsf(FF(A[1]))); break;
        case OP_SQRTS: SetF(A[0], sqrtf(fabsf(FF(A[1])))); break;
        case OP_RSQRTS: SetF(A[0], Div(FF(A[1]), sqrtf(fabsf(FF(A[2]))))); break;
        case OP_ADDS: SetF(A[0], FF(A[1]) + FF(A[2])); break;
        case OP_SUBS: SetF(A[0], FF(A[1]) - FF(A[2])); break;
        case OP_MULS: SetF(A[0], FF(A[1]) * FF(A[2])); break;
        case OP_DIVS: SetF(A[0], Div(FF(A[1]), FF(A[2]))); break;
        case OP_NEGS: SetF(A[0], -FF(A[1])); break;
        case OP_MOVS: F(A[0]) = F(A[1]); break;
        case OP_MTC1: F(A[1]) = GR(A[0]).uw[0]; break;
        case OP_MFC1: G(A[0]).d[0] = (int32_t)F(A[1]); break;
        case OP_LWC1: memcpy(&F(A[0]), Addr(A[1]), 4); break;
        case OP_SWC1: memcpy(Addr(A[1]), &F(A[0]), 4); break;
        case OP_CVTWS: { // the EE always truncates
            float f = FF(A[1]);
            int32_t w = f >= 2147483647.0f ? 0x7FFFFFFF : f <= -2147483648.0f ? (int32_t)0x80000000 : (int32_t)f;
            F(A[0]) = (uint32_t)w;
            break;
        }
        case OP_CVTSW: SetF(A[0], (float)(int32_t)F(A[1])); break;
        case OP_CFC1:
            G(A[0]).d[0] = (int32_t)(A[1].idx == 0 ? g_ee.fcr0 : g_ee.fcr31);
            break;
        case OP_CTC1:
            if (A[1].idx == 31)
                g_ee.fcr31 = GR(A[0]).uw[0];
            break;
        case OP_ADDI: G(A[0]).d[0] = GR(A[1]).d[0] + A[2].imm; break;
        case OP_ADDU: G(A[0]).d[0] = GR(A[1]).d[0] + GR(A[2]).d[0]; break;
        case OP_SUBU: G(A[0]).d[0] = GR(A[1]).d[0] - GR(A[2]).d[0]; break;
        case OP_LUI: G(A[0]).d[0] = (int32_t)((uint32_t)A[1].imm << 16); break;
        case OP_ORI: G(A[0]).ud[0] = GR(A[1]).ud[0] | (uint16_t)A[2].imm; break;
        case OP_ANDI: G(A[0]).ud[0] = GR(A[1]).ud[0] & (uint16_t)A[2].imm; break;
        case OP_AND: G(A[0]).ud[0] = GR(A[1]).ud[0] & GR(A[2]).ud[0]; break;
        case OP_OR: G(A[0]).ud[0] = GR(A[1]).ud[0] | GR(A[2]).ud[0]; break;
        case OP_LW: {
            int32_t w;
            memcpy(&w, Addr(A[1]), 4);
            G(A[0]).d[0] = w;
            break;
        }
        case OP_SW: {
            Q128 v = GR(A[0]);
            memcpy(Addr(A[1]), &v.uw[0], 4);
            break;
        }
        case OP_LQ: memcpy(G(A[0]).b, Addr(A[1]), 16); break;
        case OP_SQ: {
            Q128 v = GR(A[0]);
            memcpy(Addr(A[1]), v.b, 16);
            break;
        }
        case OP_B: branchTo = A[0].idx; break;
        case OP_BNE: if (GR(A[0]).ud[0] != GR(A[1]).ud[0]) branchTo = A[2].idx; break;
        case OP_BEQ: if (GR(A[0]).ud[0] == GR(A[1]).ud[0]) branchTo = A[2].idx; break;
        case OP_BLEZ: if (GR(A[0]).d[0] <= 0) branchTo = A[1].idx; break;
        case OP_BGTZ: if (GR(A[0]).d[0] > 0) branchTo = A[1].idx; break;
        case OP_BC0F:
            // COP0 condition: the DMA channels D_PCR selects are done
            if (!port_dma_cpcond0()) {
                // a wait on the render queue, which in software can take
                // longer than the runaway budget: not counted
                budget++;
                port_dma_wait_cpcond0();
                branchTo = A[0].idx;
            }
            break;
        case OP_PADDSH: {
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            for (int k = 0; k < 8; k++)
                d.h[k] = SatH((int32_t)s.h[k] + t.h[k]);
            G(A[0]) = d;
            break;
        }
        case OP_PSUBH: {
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            for (int k = 0; k < 8; k++)
                d.h[k] = (int16_t)(s.h[k] - t.h[k]);
            G(A[0]) = d;
            break;
        }
        case OP_PMAXH:
        case OP_PMINH: {
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            for (int k = 0; k < 8; k++)
                d.h[k] = (in.op == OP_PMAXH) == (s.h[k] > t.h[k]) ? s.h[k] : t.h[k];
            G(A[0]) = d;
            break;
        }
        case OP_PCGTH: {
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            for (int k = 0; k < 8; k++)
                d.h[k] = s.h[k] > t.h[k] ? -1 : 0;
            G(A[0]) = d;
            break;
        }
        case OP_PSRAH: {
            Q128 t = GR(A[1]), d;
            for (int k = 0; k < 8; k++)
                d.h[k] = (int16_t)(t.h[k] >> (A[2].imm & 15));
            G(A[0]) = d;
            break;
        }
        case OP_PMULTH:
        case OP_PMADDH: {
            // Products 0,1 -> LO w0,w1; 2,3 -> HI w0,w1; 4,5 -> LO w2,w3;
            // 6,7 -> HI w2,w3. rd gets LO w0, HI w0, LO w2, HI w2.
            Q128 s = GR(A[1]), t = GR(A[2]);
            static const int toLo[8] = {1, 1, 0, 0, 1, 1, 0, 0};
            static const int word[8] = {0, 1, 0, 1, 2, 3, 2, 3};
            for (int k = 0; k < 8; k++) {
                int32_t prod = (int32_t)s.h[k] * t.h[k];
                Q128 &r = toLo[k] ? g_ee.lo : g_ee.hi;
                r.w[word[k]] = in.op == OP_PMADDH ? r.w[word[k]] + prod : prod;
            }
            if (!(A[0].kind == R_GPR && A[0].idx == 0)) {
                Q128 d;
                d.w[0] = g_ee.lo.w[0];
                d.w[1] = g_ee.hi.w[0];
                d.w[2] = g_ee.lo.w[2];
                d.w[3] = g_ee.hi.w[2];
                G(A[0]) = d;
            }
            break;
        }
        case OP_PMFLO: G(A[0]) = g_ee.lo; break;
        case OP_PMFHI: G(A[0]) = g_ee.hi; break;
        case OP_PMFHLSH: {
            Q128 d;
            const int32_t src[8] = {g_ee.lo.w[0], g_ee.lo.w[1], g_ee.hi.w[0], g_ee.hi.w[1],
                                    g_ee.lo.w[2], g_ee.lo.w[3], g_ee.hi.w[2], g_ee.hi.w[3]};
            for (int k = 0; k < 8; k++)
                d.h[k] = SatH(src[k]);
            G(A[0]) = d;
            break;
        }
        case OP_MTSAH:
            g_ee.sa = (unsigned)(((GR(A[0]).ud[0] ^ (uint64_t)A[1].imm) & 7) * 2);
            break;
        case OP_QFSRV: {
            uint8_t cat[32];
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            memcpy(cat, t.b, 16);
            memcpy(cat + 16, s.b, 16);
            memcpy(d.b, cat + (g_ee.sa & 31), 16);
            G(A[0]) = d;
            break;
        }
        case OP_PADDUB: {
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            for (int k = 0; k < 16; k++) {
                int v = s.b[k] + t.b[k];
                d.b[k] = v > 255 ? 255 : (uint8_t)v;
            }
            G(A[0]) = d;
            break;
        }
        case OP_PEXTLH:
        case OP_PEXTUH: {
            Q128 s = GR(A[1]), t = GR(A[2]), d;
            int base = in.op == OP_PEXTUH ? 4 : 0;
            for (int k = 0; k < 4; k++) {
                d.h[2 * k] = t.h[base + k];
                d.h[2 * k + 1] = s.h[base + k];
            }
            G(A[0]) = d;
            break;
        }
        }
        if (in.op == OP_B || in.op == OP_BNE || in.op == OP_BEQ || in.op == OP_BLEZ ||
            in.op == OP_BGTZ || in.op == OP_BC0F) {
            if (in.delaySlot) {
                // Run the delay slot as its own step, then take the branch.
                // (No delay slot here holds another branch.)
                Prog one;
                one.insns.push_back(p->insns[pc + 1]);
                one.insns.back().delaySlot = false;
                Run(&one);
                next = pc + 2;
            }
            if (branchTo >= 0)
                next = (size_t)branchTo;
        }
        pc = next;
    }
}

} // namespace

extern "C" void port_asm(PortAsmProg *site, const char *text, PortAsmArg *args, int nargs)
{
    // A site is parsed once (the game thread and the interrupt handlers'
    // threads share sites); two threads parsing the same site at once only
    // leak one copy.
    void *prog = __atomic_load_n(&site->prog, __ATOMIC_ACQUIRE);
    if (!prog) {
        prog = Parse(text);
        __atomic_store_n(&site->prog, prog, __ATOMIC_RELEASE);
    }
    for (int k = 0; k < nargs; k++) {
        const PortAsmArg &a = args[k];
        if (a.kind == PORT_ASM_IN_R || a.kind == PORT_ASM_INOUT_R) {
            memset(&g_ee.gpr[32 + k], 0, sizeof(Q128));
            g_ee.gpr[32 + k].d[0] = a.kind == PORT_ASM_IN_R ? a.value : port_asm_load(a);
        } else if (a.kind == PORT_ASM_IN_F) {
            memcpy(&g_ee.fpr[32 + k], &a.fvalue, 4);
        }
    }
    const char *outer = port_tls_asm;
    port_tls_asm = text;
    Run((const Prog *)prog);
    port_tls_asm = outer;
    for (int k = 0; k < nargs; k++) {
        const PortAsmArg &a = args[k];
        if (a.kind == PORT_ASM_OUT_R || a.kind == PORT_ASM_INOUT_R)
            port_asm_store(a, g_ee.gpr[32 + k].d[0]);
        else if (a.kind == PORT_ASM_OUT_F)
            memcpy(a.ptr, &g_ee.fpr[32 + k], 4);
    }
}

extern "C" int64_t port_asm_load(PortAsmArg a)
{
    switch (a.size) {
    case 1: return *(int8_t *)a.ptr;
    case 2: return *(int16_t *)a.ptr;
    case 4: return *(int32_t *)a.ptr;
    default: return *(int64_t *)a.ptr;
    }
}

extern "C" void port_asm_store(PortAsmArg a, int64_t v)
{
    switch (a.size) {
    case 1: *(int8_t *)a.ptr = (int8_t)v; break;
    case 2: *(int16_t *)a.ptr = (int16_t)v; break;
    case 4: *(int32_t *)a.ptr = (int32_t)v; break;
    default: *(int64_t *)a.ptr = v; break;
    }
}
