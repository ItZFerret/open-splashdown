// The pads the game reads through libpad (ports/pc/sdk/pad.cpp), from SDL3
// gamepads, the keyboard and a script.
//
// Pad 0 is the keyboard, the first gamepad and PORT_INPUT merged; pad N is
// the (N+1)th gamepad. Every pad is a DualShock 2 in analog, pressure-
// sensitive mode (digital presses read as full pressure).
//
// Keyboard (window focused):
//   arrows d-pad, WASD left stick, IJKL right stick,
//   X / Space Cross, C / Escape Circle, Z Square, V Triangle,
//   Enter Start, Tab / Backspace Select, Q L1, E R1, 1 L2, 3 R2, F L3, G R3.
// Gamepads use SDL's layout (South Cross, East Circle, West Square, North
// Triangle, Back Select, shoulders L1/R1, triggers L2/R2 with pressure).
//
// PORT_INPUT="120:cross,180:down,200:cross/10,300:l1+r1" (headless or not):
// at finished frame F (the numbering of PORT_SCREENSHOT_FRAME) press the
// buttons for H frames (/H, default 3) on pad 0; "F:pN:buttons" on pad N-1
// (p1 = pad 0, p2 = pad 1 ...; a script that addresses pad N-1 connects N
// pads even without gamepads). Names: up down left right
// cross circle square triangle start select l1 r1 l2 r2 l3 r3, and the
// sticks at full deflection: lup ldown lleft lright rup rdown rleft rright.
// PORT_INPUT_LOG=1 logs each scripted press as it starts.
#include "port.h"

#include "gs/gs.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace {

// libpad's button bits, active high (data[2] << 8 | data[3], inverted)
enum : uint16_t {
    kSelect = 1 << 8, kL3 = 1 << 9, kR3 = 1 << 10, kStart = 1 << 11,
    kUp = 1 << 12, kRight = 1 << 13, kDown = 1 << 14, kLeft = 1 << 15,
    kL2 = 1 << 0, kR2 = 1 << 1, kL1 = 1 << 2, kR1 = 1 << 3,
    kTriangle = 1 << 4, kCircle = 1 << 5, kCross = 1 << 6, kSquare = 1 << 7,
};

// Stick pseudo-buttons in the script (bits above 16)
enum : uint32_t {
    kLUp = 1u << 16, kLDown = 1u << 17, kLLeft = 1u << 18, kLRight = 1u << 19,
    kRUp = 1u << 20, kRDown = 1u << 21, kRLeft = 1u << 22, kRRight = 1u << 23,
};

struct Name {
    const char *name;
    uint32_t bits;
};
const Name kNames[] = {
    {"up", kUp}, {"down", kDown}, {"left", kLeft}, {"right", kRight},
    {"cross", kCross}, {"x", kCross}, {"circle", kCircle}, {"o", kCircle},
    {"square", kSquare}, {"triangle", kTriangle}, {"start", kStart},
    {"select", kSelect}, {"l1", kL1}, {"r1", kR1}, {"l2", kL2}, {"r2", kR2},
    {"l3", kL3}, {"r3", kR3}, {"lup", kLUp}, {"ldown", kLDown},
    {"lleft", kLLeft}, {"lright", kLRight}, {"rup", kRUp}, {"rdown", kRDown},
    {"rleft", kRLeft}, {"rright", kRRight},
};

// The pressure bytes' order in the pad data (data[8..19])
const uint16_t kPressureOrder[12] = {
    kRight, kLeft, kUp, kDown, kTriangle, kCircle, kCross, kSquare, kL1, kR1, kL2, kR2,
};

struct Step {
    uint64_t frame, hold;
    uint32_t bits;
    int pad;
    std::string text;
    bool logged;
};

std::vector<Step> g_script;
bool g_scriptLog;
int g_scriptPads;  // pads the script addresses (highest + 1)
std::atomic<uint64_t> g_autopilotFrame[4];  // last frame each player was driven (+1)

void ParseScript()
{
    const char *env = getenv("PORT_INPUT");
    g_scriptLog = getenv("PORT_INPUT_LOG") && atoi(getenv("PORT_INPUT_LOG"));
    if (!env)
        return;
    std::string s = env;
    for (size_t at = 0; at < s.size();) {
        size_t end = s.find(',', at);
        if (end == std::string::npos)
            end = s.size();
        std::string item = s.substr(at, end - at);
        at = end + 1;
        if (item.empty())
            continue;
        size_t colon = item.find(':');
        if (colon == std::string::npos)
            port_fatal("PORT_INPUT: '%s' has no frame:", item.c_str());
        Step st = {strtoull(item.c_str(), nullptr, 0), 3, 0, 0, item, false};
        std::string keys = item.substr(colon + 1);
        if (keys.size() > 2 && keys[0] == 'p' && isdigit((unsigned char)keys[1])) {
            size_t c2 = keys.find(':');
            if (c2 == std::string::npos)
                port_fatal("PORT_INPUT: '%s': pN needs a ':'", item.c_str());
            st.pad = atoi(keys.c_str() + 1) - 1;
            if (st.pad < 0 || st.pad >= 4)
                port_fatal("PORT_INPUT: '%s': pads are p1-p4", item.c_str());
            keys = keys.substr(c2 + 1);
        }
        g_scriptPads = std::max(g_scriptPads, st.pad + 1);
        size_t slash = keys.find('/');
        if (slash != std::string::npos) {
            st.hold = strtoull(keys.c_str() + slash + 1, nullptr, 0);
            keys.resize(slash);
        }
        for (size_t k = 0; k <= keys.size();) {
            size_t plus = keys.find('+', k);
            if (plus == std::string::npos)
                plus = keys.size();
            std::string key = keys.substr(k, plus - k);
            k = plus + 1;
            bool found = false;
            for (const Name &n : kNames)
                if (key == n.name) {
                    st.bits |= n.bits;
                    found = true;
                }
            if (!found)
                port_fatal("PORT_INPUT: unknown button '%s'", key.c_str());
        }
        g_script.push_back(st);
    }
    port_log("input: %zu scripted presses", g_script.size());
}

// What the main thread saw last (keyboard, gamepads), per pad
const int kMaxPads = 4;
std::mutex g_mutex;
PortPad g_live[kMaxPads];
std::atomic<int> g_gamepads;  // open SDL gamepads
uint8_t g_motorSmall[kMaxPads], g_motorLarge[kMaxPads];
std::vector<SDL_Gamepad *> g_open;

PortPad Neutral()
{
    PortPad p = {};
    p.lx = p.ly = p.rx = p.ry = 0x80;
    return p;
}

uint8_t Axis(int v)  // SDL -32768..32767 to 0..255
{
    return (uint8_t)((v + 32768) >> 8);
}

// Apply stick pseudo-buttons and set full pressure for the digital buttons
void Finish(PortPad &p, uint32_t bits)
{
    p.buttons |= (uint16_t)bits;
    if (bits & kLLeft) p.lx = 0;
    if (bits & kLRight) p.lx = 0xFF;
    if (bits & kLUp) p.ly = 0;
    if (bits & kLDown) p.ly = 0xFF;
    if (bits & kRLeft) p.rx = 0;
    if (bits & kRRight) p.rx = 0xFF;
    if (bits & kRUp) p.ry = 0;
    if (bits & kRDown) p.ry = 0xFF;
    for (int i = 0; i < 12; i++)
        if ((p.buttons & kPressureOrder[i]) && !p.pressure[i])
            p.pressure[i] = 0xFF;
}

uint32_t KeyboardBits()
{
    const bool *k = SDL_GetKeyboardState(nullptr);
    struct Key {
        SDL_Scancode sc;
        uint32_t bits;
    };
    static const Key kKeys[] = {
        {SDL_SCANCODE_UP, kUp}, {SDL_SCANCODE_DOWN, kDown}, {SDL_SCANCODE_LEFT, kLeft},
        {SDL_SCANCODE_RIGHT, kRight}, {SDL_SCANCODE_W, kLUp}, {SDL_SCANCODE_S, kLDown},
        {SDL_SCANCODE_A, kLLeft}, {SDL_SCANCODE_D, kLRight}, {SDL_SCANCODE_I, kRUp},
        {SDL_SCANCODE_K, kRDown}, {SDL_SCANCODE_J, kRLeft}, {SDL_SCANCODE_L, kRRight},
        {SDL_SCANCODE_X, kCross}, {SDL_SCANCODE_SPACE, kCross}, {SDL_SCANCODE_C, kCircle},
        {SDL_SCANCODE_ESCAPE, kCircle}, {SDL_SCANCODE_Z, kSquare}, {SDL_SCANCODE_V, kTriangle},
        {SDL_SCANCODE_RETURN, kStart}, {SDL_SCANCODE_KP_ENTER, kStart},
        {SDL_SCANCODE_TAB, kSelect}, {SDL_SCANCODE_BACKSPACE, kSelect},
        {SDL_SCANCODE_Q, kL1}, {SDL_SCANCODE_E, kR1}, {SDL_SCANCODE_1, kL2},
        {SDL_SCANCODE_3, kR2}, {SDL_SCANCODE_F, kL3}, {SDL_SCANCODE_G, kR3},
    };
    uint32_t bits = 0;
    for (const Key &key : kKeys)
        if (k[key.sc])
            bits |= key.bits;
    return bits;
}

PortPad GamepadState(SDL_Gamepad *g)
{
    PortPad p = Neutral();
    struct Button {
        SDL_GamepadButton b;
        uint16_t bits;
    };
    static const Button kButtons[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, kUp}, {SDL_GAMEPAD_BUTTON_DPAD_DOWN, kDown},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, kLeft}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, kRight},
        {SDL_GAMEPAD_BUTTON_SOUTH, kCross}, {SDL_GAMEPAD_BUTTON_EAST, kCircle},
        {SDL_GAMEPAD_BUTTON_WEST, kSquare}, {SDL_GAMEPAD_BUTTON_NORTH, kTriangle},
        {SDL_GAMEPAD_BUTTON_START, kStart}, {SDL_GAMEPAD_BUTTON_BACK, kSelect},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, kL1}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, kR1},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, kL3}, {SDL_GAMEPAD_BUTTON_RIGHT_STICK, kR3},
    };
    for (const Button &b : kButtons)
        if (SDL_GetGamepadButton(g, b.b))
            p.buttons |= b.bits;
    p.lx = Axis(SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX));
    p.ly = Axis(SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY));
    p.rx = Axis(SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHTX));
    p.ry = Axis(SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHTY));
    // triggers 0..32767: analog L2/R2 (pressure index 10, 11)
    int l2 = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >> 7;
    int r2 = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) >> 7;
    if (l2 > 25) {
        p.buttons |= kL2;
        p.pressure[10] = (uint8_t)std::min(l2, 255);
    }
    if (r2 > 25) {
        p.buttons |= kR2;
        p.pressure[11] = (uint8_t)std::min(r2, 255);
    }
    Finish(p, 0);  // the digital buttons at full pressure: the game reads pressure
    return p;
}

// Merge b into a (pad 0: keyboard over the first gamepad)
void Merge(PortPad &a, const PortPad &b)
{
    a.buttons |= b.buttons;
    for (int i = 0; i < 12; i++)
        a.pressure[i] = std::max(a.pressure[i], b.pressure[i]);
    auto stick = [](uint8_t &x, uint8_t y) {
        if (abs(y - 0x80) > abs(x - 0x80))
            x = y;
    };
    stick(a.lx, b.lx);
    stick(a.ly, b.ly);
    stick(a.rx, b.rx);
    stick(a.ry, b.ry);
}

struct Init {
    Init()
    {
        for (PortPad &p : g_live)
            p = Neutral();
        ParseScript();
    }
} g_init;

} // namespace

void port_input_event(const void *event)
{
    const SDL_Event &e = *(const SDL_Event *)event;
    if (e.type == SDL_EVENT_GAMEPAD_ADDED) {
        SDL_Gamepad *g = SDL_OpenGamepad(e.gdevice.which);
        if (g) {
            g_open.push_back(g);
            port_log("input: gamepad %zu: %s", g_open.size() - 1, SDL_GetGamepadName(g));
        }
    } else if (e.type == SDL_EVENT_GAMEPAD_REMOVED) {
        for (size_t i = 0; i < g_open.size(); i++)
            if (SDL_GetGamepadID(g_open[i]) == e.gdevice.which) {
                port_log("input: gamepad %zu removed", i);
                SDL_CloseGamepad(g_open[i]);
                g_open.erase(g_open.begin() + i);
                break;
            }
    }
    g_gamepads = (int)g_open.size();
}

// While the settings overlay is open the game sees untouched pads.
static bool g_blocked;

void port_input_block(bool blocked)
{
    g_blocked = blocked;
}

void port_input_poll(void)
{
    PortPad live[kMaxPads];
    for (int i = 0; i < kMaxPads; i++) {
        live[i] = i < (int)g_open.size() && !g_blocked ? GamepadState(g_open[i]) : Neutral();
    }
    PortPad kb = Neutral();
    Finish(kb, g_blocked ? 0 : KeyboardBits());
    Merge(live[0], kb);
    uint8_t small[kMaxPads], large[kMaxPads];
    {
        std::lock_guard lock(g_mutex);
        memcpy(g_live, live, sizeof live);
        memcpy(small, g_motorSmall, sizeof small);
        memcpy(large, g_motorLarge, sizeof large);
    }
    static uint16_t last[kMaxPads];
    for (int i = 0; i < kMaxPads && i < (int)g_open.size(); i++) {
        uint16_t now = (uint16_t)(large[i] << 8 | small[i]);
        if (now != last[i])  // low frequency = the large motor
            SDL_RumbleGamepad(g_open[i], large[i] * 257, small[i] * 257, now ? 10000 : 0);
        last[i] = now;
    }
}

int port_input_pads(void)
{
    return std::max({1, g_scriptPads, std::min((int)g_gamepads, kMaxPads)});
}

void port_input_read(int pad, PortPad *out)
{
    if (pad < 0 || pad >= kMaxPads) {
        *out = Neutral();
        return;
    }
    {
        std::lock_guard lock(g_mutex);
        *out = g_live[pad];
    }
    if (pad < g_scriptPads) {
        uint64_t frame = gs_frame_seq();
        uint32_t bits = 0;
        for (Step &st : g_script)
            if (st.pad == pad && frame >= st.frame && frame < st.frame + st.hold) {
                bits |= st.bits;
                if (g_scriptLog && !st.logged) {
                    st.logged = true;
                    port_log("input: frame %llu: %s", (unsigned long long)frame, st.text.c_str());
                }
            }
        Finish(*out, bits);
    }
    // PORT_AUTOPILOT holds Cross (throttle) while its player is being driven:
    // the vehicle reads the key as well as m_throttle (pulsed presses left
    // it idling), and in-race menus, where nothing drives, still see edges.
    if (uint64_t f = g_autopilotFrame[pad].load(std::memory_order_relaxed))
        if (gs_frame_seq() < f - 1 + 30)
            Finish(*out, kCross);
}

void port_input_rumble(int pad, uint8_t small, uint8_t large)
{
    if (pad < 0 || pad >= kMaxPads)
        return;
    std::lock_guard lock(g_mutex);
    g_motorSmall[pad] = small;
    g_motorLarge[pad] = large;
}

// PORT_AUTOPILOT=1 (or "1,2", "all"): JetskiPlayerVehicle::Tick steers
// those players (1-based) along the track's centre line with the throttle
// held (an #ifdef SPLASHDOWN_PC hook in JetskiPlayerVehicle.cpp).
extern "C" int port_autopilot(int player)
{
    static int mask = -1;
    if (mask < 0) {
        mask = 0;
        if (const char *e = getenv("PORT_AUTOPILOT")) {
            if (!strcmp(e, "all"))
                mask = 0xF;
            else
                for (const char *p = e; *p; p++)
                    if (*p >= '1' && *p <= '4')
                        mask |= 1 << (*p - '1');
            if (mask)
                port_log("input: autopilot for players 0x%x", mask);
        }
    }
    if (player < 0 || player >= 4 || !(mask >> player & 1))
        return 0;
    g_autopilotFrame[player].store(gs_frame_seq() + 1, std::memory_order_relaxed);
    return 1;
}
