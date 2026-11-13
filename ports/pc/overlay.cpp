// The settings overlay: a panel over the game's picture with the port's
// display settings (ports/pc/settings.h). Opened with F1, the gamepad's
// guide button or both sticks pressed together. Up/Down pick a row,
// Left/Right change it, Esc or F1 closes; the mouse works too (click a row
// to step it, right-click to step back). The game keeps running underneath
// with its pads untouched (port_input_block).
//
// Drawn with SDL's renderer alone: filled rectangles and its built-in 8x8
// debug font, scaled to the window.
#include "overlay.h"

#include "port.h"
#include "settings.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

struct Color {
    uint8_t r, g, b, a;
};
const Color kBackdrop = {6, 10, 18, 150};
const Color kPanel = {20, 26, 38, 240};
const Color kPanelEdge = {58, 70, 92, 255};
const Color kAccent = {0, 168, 255, 255};
const Color kRowHot = {34, 46, 68, 255};
const Color kText = {236, 240, 246, 255};
const Color kDim = {138, 150, 170, 255};
const Color kOff = {84, 94, 112, 255};

enum RowId { kRowAspect, kRowResolution, kRowDisplay, kRowWindow, kRowScaling, kRowVsync, kRowFps, kRowSkipIntro, kRowReset, kRowClose, kRows };

struct Row {
    const char *label;
    const char *hint;
};
const Row kRowInfo[kRows] = {
    {"Aspect ratio", "Wider shapes show more to the sides. The HUD and menus stretch."},
    {"Resolution", "Draws with more pixels: sharper. 3x can drop below 30 FPS in races."},
    {"Display", "Fullscreen is borderless, at the desktop's resolution."},
    {"Window size", "The picture's height, in steps of the game's 448 lines."},
    {"Scaling", "Smooth blends pixels. Sharp keeps them. Integer uses whole multiples."},
    {"VSync", "Waits for the display's refresh. The game itself runs at 30 FPS."},
    {"FPS counter", "Game frames per second, in the corner."},
    {"Skip intro", "Starts at the menu: no publisher logos or intro movie."},
    {"Reset to defaults", "Puts every setting back."},
    {"Close", "Back to the game (Esc or F1)."},
};

// PORT_OVERLAY_OPEN=1 (testing): the menu is open from the start.
bool g_open = getenv("PORT_OVERLAY_OPEN") != nullptr;
bool g_changed;
int g_selected;
SDL_FRect g_rowRect[kRows];   // where each row was drawn, in pixels (mouse)
float g_density = 1;
const auto g_start = std::chrono::steady_clock::now();

int *Value(PortSettings &s, int row, int *count)
{
    switch (row) {
    case kRowAspect: *count = kAspectCount; return &s.aspect;
    case kRowResolution: *count = 3; return &s.resolution;   // 1..3
    case kRowDisplay: *count = 2; return &s.fullscreen;
    case kRowWindow: *count = 4; return &s.windowScale;   // 1..4
    case kRowScaling: *count = kScaleCount; return &s.scaling;
    case kRowVsync: *count = 2; return &s.vsync;
    case kRowFps: *count = 2; return &s.showFps;
    case kRowSkipIntro: *count = 2; return &s.skipIntro;
    }
    *count = 0;
    return nullptr;
}

std::string ValueText(const PortSettings &s, int row)
{
    static const char *const kAspects[] = {"4:3 Original", "16:9 Widescreen", "16:10", "21:9 Ultrawide", "Fill window"};
    static const char *const kScalings[] = {"Smooth", "Sharp", "Integer"};
    switch (row) {
    case kRowAspect: return kAspects[s.aspect];
    case kRowResolution: {
        char buf[32];
        snprintf(buf, sizeof buf, "%dx  (%dx%d)", s.resolution, 640 * s.resolution, 448 * s.resolution);
        return buf;
    }
    case kRowDisplay: return s.fullscreen ? "Fullscreen" : "Windowed";
    case kRowWindow: {
        char buf[32];
        snprintf(buf, sizeof buf, "%dx  (%d lines)", s.windowScale, s.windowScale * 448);
        return buf;
    }
    case kRowScaling: return kScalings[s.scaling];
    case kRowVsync: return s.vsync ? "On" : "Off";
    case kRowFps: return s.showFps ? "On" : "Off";
    case kRowSkipIntro: return s.skipIntro ? "On" : "Off";
    }
    return "";
}

bool Enabled(const PortSettings &s, int row)
{
    return row != kRowWindow || !s.fullscreen;
}

void Changed()
{
    g_changed = true;
    port_settings_save();
}

void Step(int row, int dir)
{
    PortSettings &s = port_settings();
    if (!Enabled(s, row))
        return;
    if (row == kRowReset) {
        s = PortSettings();
        Changed();
        return;
    }
    if (row == kRowClose) {
        g_open = false;
        port_input_block(false);
        return;
    }
    int count;
    int *v = Value(s, row, &count);
    int base = row == kRowWindow || row == kRowResolution ? 1 : 0;
    *v = base + ((*v - base + dir) % count + count) % count;
    Changed();
}

void Move(int dir)
{
    const PortSettings &s = port_settings();
    for (int i = 0; i < kRows; i++) {
        g_selected = ((g_selected + dir) % kRows + kRows) % kRows;
        if (Enabled(s, g_selected))
            return;
    }
}

void Toggle()
{
    g_open = !g_open;
    port_input_block(g_open);
}

int RowAt(float x, float y)
{
    for (int i = 0; i < kRows; i++) {
        const SDL_FRect &r = g_rowRect[i];
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            return i;
    }
    return -1;
}

// ---- drawing ----------------------------------------------------------------

void Fill(SDL_Renderer *r, float x, float y, float w, float h, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

// A rectangle with its corners cut by `cut` pixels in two steps: reads as
// rounded at the sizes the panel is drawn at.
void Rounded(SDL_Renderer *r, float x, float y, float w, float h, float cut, Color c)
{
    Fill(r, x + cut, y, w - 2 * cut, h, c);
    Fill(r, x, y + cut, w, h - 2 * cut, c);
    Fill(r, x + cut / 3, y + cut / 3, w - 2 * cut / 3, h - 2 * cut / 3, c);
}

// The 8x8 debug font at `scale` pixels per font pixel, top left at (x, y).
void Text(SDL_Renderer *r, float x, float y, float scale, Color c, const std::string &s)
{
    SDL_SetRenderScale(r, scale, scale);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDebugText(r, x / scale, y / scale, s.c_str());
    SDL_SetRenderScale(r, 1, 1);
}

float TextWidth(const std::string &s, float scale)
{
    return (float)s.size() * 8 * scale;
}

void DrawMenu(SDL_Renderer *r, int ww, int wh)
{
    const PortSettings &s = port_settings();
    // sizes follow the window: the font's pixels are whole multiples
    float ts = (float)std::max(1, std::min(wh / 300, ww / 420));   // body text scale
    float cw = 8 * ts, lh = 8 * ts;
    float rowH = lh * 2.6f;
    float panelW = std::min((float)ww - 2 * cw, cw * 50);
    float pad = cw * 2;
    float headH = lh * 5.2f, footH = lh * 6.6f;
    float panelH = headH + rowH * kRows + footH;
    float px = ((float)ww - panelW) / 2, py = std::max(0.0f, ((float)wh - panelH) / 2);

    Fill(r, 0, 0, (float)ww, (float)wh, kBackdrop);
    Rounded(r, px - 2, py - 2, panelW + 4, panelH + 4, cw, kPanelEdge);
    Rounded(r, px, py, panelW, panelH, cw, kPanel);

    // header: a title in larger type over an accent rule
    Text(r, px + pad, py + lh * 1.3f, ts * 2, kText, "SETTINGS");
    Text(r, px + panelW - pad - TextWidth("DISPLAY", ts), py + lh * 2.2f, ts, kDim, "DISPLAY");
    Fill(r, px + pad, py + headH - lh * 0.9f, panelW - 2 * pad, std::max(1.0f, ts), kPanelEdge);
    Fill(r, px + pad, py + headH - lh * 0.9f, cw * 6, std::max(2.0f, ts * 2), kAccent);

    float y = py + headH;
    for (int i = 0; i < kRows; i++, y += rowH) {
        bool hot = i == g_selected, on = Enabled(s, i);
        bool action = i == kRowReset || i == kRowClose;
        SDL_FRect rect = {px + pad / 2, y, panelW - pad, rowH - ts * 2};
        g_rowRect[i] = rect;
        if (hot) {
            Rounded(r, rect.x, rect.y, rect.w, rect.h, cw / 2, kRowHot);
            Fill(r, rect.x, rect.y + rect.h * 0.2f, std::max(2.0f, ts * 2), rect.h * 0.6f, kAccent);
        }
        float ty = y + (rect.h - lh) / 2;
        Text(r, px + pad + cw, ty, ts, !on ? kOff : hot ? kText : action ? kDim : kText, kRowInfo[i].label);
        if (action)
            continue;
        // the value between its arrows, right-aligned in a fixed column
        std::string value = ValueText(s, i);
        float right = px + panelW - pad - cw;
        float colW = cw * 20;
        float vx = right - colW + (colW - TextWidth(value, ts)) / 2;
        Text(r, vx, ty, ts, !on ? kOff : hot ? kAccent : kText, value);
        if (on) {
            Text(r, right - colW, ty, ts, hot ? kText : kOff, "<");
            Text(r, right - cw, ty, ts, hot ? kText : kOff, ">");
        }
    }

    // footer: what the selected row does, then the keys
    y = py + headH + rowH * kRows + lh * 0.6f;
    Fill(r, px + pad, y, panelW - 2 * pad, std::max(1.0f, ts), kPanelEdge);
    std::string hint = kRowInfo[g_selected].hint;
    size_t fit = (size_t)((panelW - 2 * pad) / cw);
    // two lines at most, broken at a space
    std::string first = hint, second;
    if (hint.size() > fit) {
        size_t cut = hint.rfind(' ', fit);
        if (cut == std::string::npos)
            cut = fit;
        first = hint.substr(0, cut);
        second = hint.substr(std::min(hint.size(), cut + 1), fit);
    }
    Text(r, px + pad, y + lh * 0.9f, ts, kDim, first);
    Text(r, px + pad, y + lh * 2.1f, ts, kDim, second);
    const char *keys = "Up/Down select   Left/Right change   Esc close";
    Text(r, px + (panelW - TextWidth(keys, ts)) / 2, y + lh * 4.4f, ts, kOff, keys);
}

// A small label on a dark plate (the frame counter, the opening note).
void Plate(SDL_Renderer *r, float x, float y, float scale, Color c, const std::string &s, uint8_t alpha)
{
    float w = TextWidth(s, scale) + 12 * scale, h = 16 * scale;
    Rounded(r, x, y, w, h, 3 * scale, {8, 12, 20, (uint8_t)(alpha * 190 / 255)});
    c.a = alpha;
    Text(r, x + 6 * scale, y + 4 * scale, scale, c, s);
}

double Seconds()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - g_start).count();
}

const double kNoteSeconds = 6;

} // namespace

bool port_overlay_is_open(void)
{
    return g_open;
}

bool port_overlay_take_changed(void)
{
    bool c = g_changed;
    g_changed = false;
    return c;
}

bool port_overlay_animating(void)
{
    return g_open || Seconds() < kNoteSeconds;
}

bool port_overlay_event(const SDL_Event &e, SDL_Window *window)
{
    g_density = SDL_GetWindowPixelDensity(window);
    if (g_density <= 0)
        g_density = 1;
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN:
        if (e.key.scancode == SDL_SCANCODE_F1 && !e.key.repeat) {
            Toggle();
            return true;
        }
        if (!g_open)
            return false;
        switch (e.key.scancode) {
        case SDL_SCANCODE_ESCAPE: Toggle(); break;
        case SDL_SCANCODE_UP: case SDL_SCANCODE_W: Move(-1); break;
        case SDL_SCANCODE_DOWN: case SDL_SCANCODE_S: Move(1); break;
        case SDL_SCANCODE_LEFT: case SDL_SCANCODE_A: Step(g_selected, -1); break;
        case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D:
        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: case SDL_SCANCODE_SPACE:
            Step(g_selected, 1);
            break;
        default: break;
        }
        return true;
    case SDL_EVENT_KEY_UP:
        return g_open;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
        int b = e.gbutton.button;
        bool chord = false;
        if (b == SDL_GAMEPAD_BUTTON_LEFT_STICK || b == SDL_GAMEPAD_BUTTON_RIGHT_STICK)
            if (SDL_Gamepad *g = SDL_GetGamepadFromID(e.gbutton.which))
                chord = SDL_GetGamepadButton(g, b == SDL_GAMEPAD_BUTTON_LEFT_STICK ? SDL_GAMEPAD_BUTTON_RIGHT_STICK
                                                                                     : SDL_GAMEPAD_BUTTON_LEFT_STICK);
        if (b == SDL_GAMEPAD_BUTTON_GUIDE || chord) {
            Toggle();
            return true;
        }
        if (!g_open)
            return false;
        switch (b) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP: Move(-1); break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: Move(1); break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: Step(g_selected, -1); break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: case SDL_GAMEPAD_BUTTON_SOUTH: Step(g_selected, 1); break;
        case SDL_GAMEPAD_BUTTON_EAST: case SDL_GAMEPAD_BUTTON_START: Toggle(); break;
        default: break;
        }
        return true;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        return g_open;
    case SDL_EVENT_MOUSE_MOTION:
        if (g_open) {
            int row = RowAt(e.motion.x * g_density, e.motion.y * g_density);
            if (row >= 0 && Enabled(port_settings(), row))
                g_selected = row;
        }
        return g_open;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (g_open) {
            int row = RowAt(e.button.x * g_density, e.button.y * g_density);
            if (row >= 0 && Enabled(port_settings(), row)) {
                g_selected = row;
                Step(row, e.button.button == SDL_BUTTON_RIGHT ? -1 : 1);
            }
        }
        return g_open;
    default:
        return false;
    }
}

void port_overlay_draw(SDL_Renderer *r, int ww, int wh, double fps)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    float scale = (float)std::max(1, wh / 448);
    if (port_settings().showFps) {
        char buf[32];
        snprintf(buf, sizeof buf, "%.0f FPS", fps);
        Plate(r, 8 * scale, 8 * scale, scale, kText, buf, 255);
    }
    double t = Seconds();
    if (!g_open && t < kNoteSeconds) {
        // fades out over its last second
        double a = std::min(1.0, kNoteSeconds - t);
        const char *note = "F1  Settings";
        Plate(r, (float)ww - TextWidth(note, scale) - 20 * scale, 8 * scale, scale, kText, note, (uint8_t)(a * 255));
    }
    if (g_open)
        DrawMenu(r, ww, wh);
}
