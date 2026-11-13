// The VIF1 command decoder (ports/pc/vu/vu.h).
//
// The DMA channel hands over its words as they come (under TTE the tag's
// upper 64 bits first); a command and its data may span several tags, so
// words collect in a buffer and each command runs once all of it is there.
#include "vu.h"

#include "gs/gs.h"
#include "port.h"
#include "port_simd.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include "port_os.h"
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

extern uintptr_t g_vif1_src;
void port_describe_address(const void *p);  // ports/pc/dma.cpp

namespace {

// The request word: 0 idle, 1 raised (a futex: the handler thread sleeps
// on it, VIF1 wakes it).
std::atomic<uint32_t> g_intcRequest{0};
std::atomic<uint64_t> g_intcRaised, g_intcDone;
std::atomic<int> g_stalled;  // VIF1 waits for the handler (watchdog dumps)

void *IntcThread(void *)
{
    port_thread_name("vif1-intc");
    for (;;) {
        for (int idle = 0; !g_intcRequest.load(std::memory_order_acquire); idle++) {
            if (idle > 2000)
                port_futex_wait(&g_intcRequest, 0, 0);
            else
                port_cpu_pause();
        }
        port_raise_intc(5);
        g_intcDone.fetch_add(1, std::memory_order_relaxed);
        g_intcRequest.store(0, std::memory_order_release);
    }
    return nullptr;
}

struct IntcThreadStart {
    IntcThreadStart()
    {
        const size_t size = 1 << 20;
        port_watch_futex(&g_intcRequest, "vif1.intcRequest");
        if (!port_thread_start_low(IntcThread, nullptr, size))
            port_fatal("can't start the VIF1 interrupt thread");
    }
};

struct Vif {
    std::vector<uint32_t> buf;
    size_t pos;
    uint32_t cl = 1, wl = 1, mask, mode;
    uint32_t row[4], col[4];
    uint32_t base, ofst, tops, itops, dbf, mark;
} V;

void LogOnce(uint32_t cmd)
{
    static bool seen[128];
    if (!seen[cmd & 127]) {
        seen[cmd & 127] = true;
        port_log("VIF1: command 0x%02x ignored", cmd);
    }
}

void StartVu(uint32_t pc)
{
    g_vu1.top = V.tops;
    g_vu1.itop = V.itops;
    V.dbf ^= 1;
    V.tops = V.dbf ? V.base + V.ofst : V.base;
    // each program at each entry point once (programs share load addresses)
    static std::set<std::pair<const char *, uint32_t>> seen;
    if (seen.insert({vu1_program_at(pc), pc}).second)
    {
        port_log("VU1: first run of %s at 0x%x (VIF data from %p)", vu1_program_at(pc), pc, (void *)g_vif1_src);
        port_describe_address((const void *)g_vif1_src);
    }
    vu1_run(pc);
}

// One UNPACK. d points at its data, which holds `words` words.
void Unpack(uint32_t code, const uint32_t *d)
{
    uint32_t cmd = (code >> 24) & 0x7F;
    uint32_t vn = (cmd >> 2) & 3, vl = cmd & 3;
    bool m = cmd & 0x10;
    uint32_t num = (code >> 16) & 0xFF;
    if (!num)
        num = 256;
    bool usn = code & 0x4000;
    uint32_t addr = (code & 0x3FF) + ((code & 0x8000) ? V.tops : 0);
    const uint8_t *bytes = (const uint8_t *)d;
    uint32_t bits = vl == 0 ? 32 : vl == 1 ? 16 : vl == 2 ? 8 : 5;
    uint32_t comps = vn + 1;
    size_t bitpos = 0;
    uint32_t cycle = 0;
    uint32_t qw = addr;
    for (uint32_t n = 0; n < num; n++) {
        // write cycle position within CL/WL
        bool fill = V.wl > V.cl && cycle >= V.cl;
        uint32_t data[4] = {0, 0, 0, 0};
        if (!fill) {
            if (vl == 3) {
                uint16_t c;
                memcpy(&c, bytes + bitpos / 8, 2);
                bitpos += 16;
                data[0] = (c & 31) << 3;
                data[1] = ((c >> 5) & 31) << 3;
                data[2] = ((c >> 10) & 31) << 3;
                data[3] = (c >> 15) << 7;
            } else {
                for (uint32_t k = 0; k < comps; k++) {
                    uint32_t v;
                    if (bits == 32) {
                        memcpy(&v, bytes + bitpos / 8, 4);
                    } else if (bits == 16) {
                        uint16_t h;
                        memcpy(&h, bytes + bitpos / 8, 2);
                        v = usn ? h : (uint32_t)(int32_t)(int16_t)h;
                    } else {
                        uint8_t b = bytes[bitpos / 8];
                        v = usn ? b : (uint32_t)(int32_t)(int8_t)b;
                    }
                    bitpos += bits;
                    data[k] = v;
                }
                if (vn == 0)
                    data[1] = data[2] = data[3] = data[0];
            }
        }
        uint32_t *dst = (uint32_t *)(g_vu1.data + (qw & 1023) * 16);
        uint32_t r = cycle < 3 ? cycle : 3;
        for (int k = 0; k < 4; k++) {
            bool have = fill ? false : (vl == 3 || (uint32_t)k < comps || vn == 0);
            uint32_t mk = m ? (V.mask >> ((r * 4 + k) * 2)) & 3 : 0;
            if (fill && !m)
                mk = 1;  // filling writes ROW without a mask
            uint32_t v;
            switch (mk) {
            case 0:
                if (!have)
                    continue;  // V2/V3: the other fields are left as they were
                v = data[k];
                if (V.mode == 1) {
                    v += V.row[k];
                } else if (V.mode == 2) {
                    v += V.row[k];
                    V.row[k] = v;
                }
                break;
            case 1: v = V.row[k]; break;
            case 2: v = V.col[r]; break;
            default: continue;
            }
            dst[k] = v;
        }
        // advance the destination address
        cycle++;
        if (V.wl <= V.cl) {
            qw++;
            if (cycle >= V.wl) {
                qw += V.cl - V.wl;
                cycle = 0;
            }
        } else {
            qw++;
            if (cycle >= V.wl)
                cycle = 0;
        }
    }
}

// Words of data an UNPACK takes.
uint32_t UnpackWords(uint32_t code)
{
    uint32_t cmd = (code >> 24) & 0x7F;
    uint32_t vn = (cmd >> 2) & 3, vl = cmd & 3;
    uint32_t num = (code >> 16) & 0xFF;
    if (!num)
        num = 256;
    uint32_t n = num;
    if (V.wl > V.cl)
        n = num / V.wl * V.cl + std::min(num % V.wl, V.cl);
    uint32_t bitsPer = vl == 3 ? 16 : (vn + 1) * (vl == 0 ? 32 : vl == 1 ? 16 : 8);
    return (n * bitsPer + 31) / 32;
}

// Runs the command at buf[pos] if its data is all there.
bool Step()
{
    size_t avail = V.buf.size() - V.pos;
    if (!avail)
        return false;
    const uint32_t *p = V.buf.data() + V.pos;
    uint32_t code = p[0];
    uint32_t cmd = (code >> 24) & 0x7F;
    uint32_t imm = code & 0xFFFF;
    size_t need = 1;
    if (cmd >= 0x60)
        need = 1 + UnpackWords(code);
    else if (cmd == 0x20)
        need = 2;
    else if (cmd == 0x30 || cmd == 0x31)
        need = 5;
    else if (cmd == 0x4A)
        need = 1 + 2 * (((code >> 16) & 0xFF) ? ((code >> 16) & 0xFF) : 256);
    else if (cmd == 0x50 || cmd == 0x51) {
        // DIRECT data starts on a qword boundary of the stream
        need = 1 + 4 * (imm ? imm : 65536);
    }
    if (avail < need)
        return false;
    switch (cmd) {
    case 0x00: break;
    case 0x01: V.cl = code & 0xFF; V.wl = (code >> 8) & 0xFF; break;
    case 0x02: V.ofst = code & 0x3FF; V.dbf = 0; V.tops = V.base; break;
    case 0x03: V.base = code & 0x3FF; break;
    case 0x04: V.itops = code & 0x3FF; break;
    case 0x05: V.mode = code & 3; break;
    case 0x06: break;  // MSKPATH3
    case 0x07: V.mark = imm; break;
    case 0x10: case 0x11: case 0x13: break;  // FLUSHE/FLUSH/FLUSHA: VU1 runs synchronously
    case 0x14: case 0x15:
        if (!strcmp(vu1_program_at(imm), "?") || getenv("PORT_VIF_DUMP_MSCAL")) {
            static int dumps;
            if (dumps++ < 3) {
                // the VIF words before this MSCAL (a misparsed stream?)
                char buf[2048];
                int n = 0;
                for (size_t k = V.pos >= 48 ? V.pos - 48 : 0; k <= V.pos && n < 1900; k++)
                    n += snprintf(buf + n, sizeof buf - n, "%08x ", V.buf[k]);
                port_log("VIF1: MSCAL 0x%x after: %s", imm, buf);
            }
        }
        StartVu(imm);
        break;
    case 0x17: StartVu(g_vu1.pc); break;
    case 0x20: V.mask = p[1]; break;
    case 0x30: memcpy(V.row, p + 1, 16); break;
    case 0x31: memcpy(V.col, p + 1, 16); break;
    case 0x4A: {
        uint32_t n = (need - 1) / 2;
        bool changed = false;
        for (uint32_t k = 0; k < n; k++) {
            uint64_t &m = g_vu1.micro[(imm + k) & 2047];
            if (memcmp(&m, p + 1 + 2 * k, 8)) {
                memcpy(&m, p + 1 + 2 * k, 8);
                changed = true;
            }
        }
        if (changed)
            vu1_micro_changed();
        vu1_note_upload(imm, n, (const void *)g_vif1_src);
        break;
    }
    case 0x50: case 0x51: gs_gif_write(GS_PATH2, p + 1, (need - 1) / 4); break;
    default:
        if (cmd >= 0x60)
            Unpack(code, p + 1);
        else
            LogOnce(cmd);
        break;
    }
    V.pos += need;
    if (code & 0x80000000u) {
        // The I bit: VIF1 stalls and raises INTC 5. The game's handler
        // (ManualTextureSynchronizationInterrupt) starts the next texture's
        // GIF transfer once the channel is free. It runs on its own thread
        // while this one keeps the other channels going (the game thread may
        // be waiting on the GIF too); VIF1 resumes after the handler's
        // transfer.
        // (One persistent handler thread, its stack below 4 GB like the
        // game's: a thread per interrupt cost ~15% and made gdb crawl.)
        static IntcThreadStart start;
        g_intcRaised.fetch_add(1, std::memory_order_relaxed);
        g_stalled.store(1, std::memory_order_relaxed);
        g_intcRequest.store(1, std::memory_order_release);
        port_futex_wake(&g_intcRequest, false);
        while (g_intcRequest.load(std::memory_order_acquire)) {
            for (int ch = 0; ch < 10; ch++)
                if (ch != 1)
                    port_dma_service(ch);
            port_cpu_pause();
        }
        g_stalled.store(0, std::memory_order_relaxed);
        port_dma_service(2);
    }
    return true;
}

} // namespace

// The source address of the words now arriving: MPG names its program by
// it (ports/pc/dma.cpp sets it).
uintptr_t g_vif1_src;

void vif1_write(const uint32_t *words, size_t n)
{
    V.buf.insert(V.buf.end(), words, words + n);
    while (Step()) {
    }
    if (V.pos == V.buf.size()) {
        V.buf.clear();
        V.pos = 0;
    } else if (V.pos > 4096) {
        V.buf.erase(V.buf.begin(), V.buf.begin() + V.pos);
        V.pos = 0;
    }
}

void vif1_debug(FILE *f)
{
    // the DMA thread's data, read racily: a snapshot for a stuck thread
    size_t size = V.buf.size(), pos = V.pos;
    fprintf(f, "VIF1: buffer %zu words, at %zu (next code %08x); stalled for INTC 5: %d, request %u, raised %llu, "
               "handled %llu; CL %u WL %u BASE %x OFST %x TOPS %x ITOPS %x MARK %x\n",
            size, pos, pos < size ? V.buf[pos] : 0, g_stalled.load(), g_intcRequest.load(),
            (unsigned long long)g_intcRaised.load(), (unsigned long long)g_intcDone.load(), V.cl, V.wl, V.base, V.ofst,
            V.tops, V.itops, V.mark);
}
