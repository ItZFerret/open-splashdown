// Presenting the GS's output on the main thread (ports/pc/main.cpp).
//
// Each frame the game finishes, the picture it displays (PMODE/DISPFB/DISPLAY at
// 0x12000000, read from the GS's local memory: ports/pc/gs/gs.cpp) is shown in
// an SDL3 window, scaled to fit. Without a display (no WAYLAND_DISPLAY or
// DISPLAY, or PORT_HEADLESS=1) nothing opens, for headless verification:
//   PORT_SCREENSHOT=out.png PORT_SCREENSHOT_FRAME=N   save the game's Nth
//                                     finished frame as PNG (default 60)
//   PORT_SCREENSHOT_EVERY=N           ... or every Nth frame, to out.png
//                                     with _<frame> before ".png"
//   PORT_EXIT_FRAME=N                 exit after frame N
// A frame is finished when the game's next Flip sends FINISH, before DISPFB
// moves (ports/pc/gs/gs.cpp: the game draws its 2D UI into the displayed buffer
// after the flip); the GS reads it out then, only if it is wanted
// (port_video_want: a screenshot, or the window has shown the last one).
// A status line goes to stderr every 5 seconds (with game frames per second,
// the average and the slowest second; PORT_FPS_LOG=1 logs every second).
#include "port.h"
#include "port_os.h"

#include "gs/gs.h"
#include "overlay.h"
#include "settings.h"
#include "vu/vu.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <zlib.h>

namespace {

void Put32(std::vector<uint8_t> &v, uint32_t x)
{
    v.push_back(x >> 24);
    v.push_back(x >> 16);
    v.push_back(x >> 8);
    v.push_back(x);
}

void Chunk(FILE *f, const char *type, const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> c;
    Put32(c, (uint32_t)data.size());
    c.insert(c.end(), type, type + 4);
    c.insert(c.end(), data.begin(), data.end());
    uint32_t crc = crc32(0, c.data() + 4, (uInt)(c.size() - 4));
    Put32(c, crc);
    fwrite(c.data(), 1, c.size(), f);
}

bool WritePng(const std::string &path, const uint32_t *rgba, int w, int h)
{
    FILE *f = fopen(path.c_str(), "wb");
    if (!f)
        return false;
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ihdr;
    Put32(ihdr, w);
    Put32(ihdr, h);
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});  // 8-bit RGB
    Chunk(f, "IHDR", ihdr);
    std::vector<uint8_t> raw;
    raw.reserve((size_t)h * (w * 3 + 1));
    for (int y = 0; y < h; y++) {
        raw.push_back(0);
        for (int x = 0; x < w; x++) {
            uint32_t c = rgba[(size_t)y * w + x];
            raw.push_back(c);
            raw.push_back(c >> 8);
            raw.push_back(c >> 16);
        }
    }
    uLongf n = compressBound(raw.size());
    std::vector<uint8_t> z(n);
    compress2(z.data(), &n, raw.data(), raw.size(), 6);
    z.resize(n);
    Chunk(f, "IDAT", z);
    Chunk(f, "IEND", {});
    fclose(f);
    return true;
}

long EnvInt(const char *name, long def)
{
    const char *v = getenv(name);
    return v && *v ? strtol(v, nullptr, 0) : def;
}

struct Window {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    int tw = 0, th = 0;
    PortSettings applied;   // what the window was last set to
    double fps = 0;

    // The window for a picture windowScale * 448 lines high in the
    // setting's shape (Fill window: 16:9 to start with).
    static void WindowSize(const PortSettings &s, int *w, int *h)
    {
        *h = s.windowScale * 448;
        float aspect = s.aspect == kAspectFill ? 16.0f / 9 : port_aspect_value(s.aspect, 0, 0);
        *w = (int)(*h * aspect + 0.5f);
    }

    bool Open()
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            port_log("video: SDL_Init failed (%s): running headless", SDL_GetError());
            return false;
        }
        const PortSettings &s = port_settings();
        int w, h;
        WindowSize(s, &w, &h);
        window = SDL_CreateWindow("Splashdown", w, h, SDL_WINDOW_RESIZABLE | (s.fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
        if (!window) {
            port_log("video: no window (%s): running headless", SDL_GetError());
            return false;
        }
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            port_log("video: no renderer (%s): running headless", SDL_GetError());
            return false;
        }
        SDL_SetRenderVSync(renderer, s.vsync ? 1 : 0);
        applied = s;
        port_log("video: %s renderer; F1 opens the settings", SDL_GetRendererName(renderer));
        return true;
    }

    // The settings changed (the overlay): the window follows.
    void Apply()
    {
        const PortSettings &s = port_settings();
        if (s.fullscreen != applied.fullscreen)
            SDL_SetWindowFullscreen(window, s.fullscreen != 0);
        if (!s.fullscreen && (s.windowScale != applied.windowScale || s.aspect != applied.aspect ||
                              s.fullscreen != applied.fullscreen)) {
            int w, h;
            WindowSize(s, &w, &h);
            SDL_SetWindowSize(window, w, h);
        }
        if (s.vsync != applied.vsync)
            SDL_SetRenderVSync(renderer, s.vsync ? 1 : 0);
        if (texture)
            SDL_SetTextureScaleMode(texture, s.scaling == kScaleSmooth ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
        applied = s;
    }

    void Upload(const uint32_t *rgba, int w, int h)
    {
        if (!texture || w != tw || h != th) {
            if (texture)
                SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h);
            SDL_SetTextureScaleMode(texture, port_settings().scaling == kScaleSmooth ? SDL_SCALEMODE_LINEAR
                                                                                   : SDL_SCALEMODE_NEAREST);
            tw = w;
            th = h;
        }
        SDL_UpdateTexture(texture, nullptr, rgba, w * 4);
    }

    // The picture in the setting's shape, as large as the window allows
    // (Integer: at a whole multiple of its 448 lines), then the overlay.
    void Draw()
    {
        const PortSettings &s = port_settings();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        int ww, wh;
        SDL_GetWindowSizeInPixels(window, &ww, &wh);
        float aspect = port_aspect_value(s.aspect, ww, wh);
        // the game's projection widens to the same shape (ports/pc/settings.h)
        port_widescreen_set(aspect);
        float aw = (float)ww, ah = aw / aspect;
        if (ah > wh) {
            ah = (float)wh;
            aw = ah * aspect;
        }
        if (s.scaling == kScaleInteger && th > 0 && ah >= th) {
            ah = (float)((int)ah / th * th);
            aw = ah * aspect;
        }
        SDL_FRect dst = {(ww - aw) / 2, (wh - ah) / 2, aw, ah};
        if (texture)
            SDL_RenderTexture(renderer, texture, nullptr, &dst);
        port_overlay_draw(renderer, ww, wh, fps);
        // PORT_WINDOW_SHOT=out.png (testing the overlay): save what the
        // window shows at its PORT_WINDOW_SHOT_DRAW'th draw (default 120)
        static const char *shot = getenv("PORT_WINDOW_SHOT");
        static long shotAt = EnvInt("PORT_WINDOW_SHOT_DRAW", 120), draws;
        if (shot && ++draws == shotAt) {
            if (SDL_Surface *raw = SDL_RenderReadPixels(renderer, nullptr)) {
                if (SDL_Surface *rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ABGR8888)) {
                    std::vector<uint32_t> px((size_t)rgba->w * rgba->h);
                    for (int y = 0; y < rgba->h; y++)
                        memcpy(&px[(size_t)y * rgba->w], (const uint8_t *)rgba->pixels + (size_t)y * rgba->pitch,
                               (size_t)rgba->w * 4);
                    if (WritePng(shot, px.data(), rgba->w, rgba->h))
                        port_log("video: window (%dx%d) saved to %s", rgba->w, rgba->h, shot);
                    SDL_DestroySurface(rgba);
                }
                SDL_DestroySurface(raw);
            }
        }
        SDL_RenderPresent(renderer);
    }

    void Present(const uint32_t *rgba, int w, int h)
    {
        Upload(rgba, w, h);
        Draw();
    }
};

// Frames the GS hands over (port_video_want/port_video_frame, on the GS's
// thread), for the presenter.
struct Shot {
    uint64_t seq;
    PortFrame rgba;
    int w, h;
};

struct {
    bool init, headless;
    std::string shot;
    long shotFrame = -1, shotEvery = 0, exitFrame = -1;
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<Shot> queue;
    bool presentPending;  // a window's frame not shown yet
} V;

bool Screenshot(uint64_t f)
{
    return !V.shot.empty() && ((long)f == V.shotFrame || (V.shotEvery && f % V.shotEvery == 0));
}

} // namespace

bool port_video_want(uint64_t seq)
{
    if (!V.init)
        return false;
    if (Screenshot(seq))
        return true;
    if (V.headless)
        return false;
    std::lock_guard lock(V.mutex);
    return !V.presentPending;
}

void port_video_frame(uint64_t seq, PortFrame &&rgba, int w, int h)
{
    {
        std::lock_guard lock(V.mutex);
        if (!V.headless)
            V.presentPending = true;
        V.queue.push_back({seq, std::move(rgba), w, h});
    }
    V.cv.notify_one();
}

void port_video_run(volatile bool *done)
{
    port_heap_this_thread();  // the game runs now: keep out of its heap
    // PORT_HEADLESS=0 tries a window even without WAYLAND_DISPLAY/DISPLAY
    // (e.g. SDL_VIDEO_DRIVER=offscreen)
    long headlessEnv = EnvInt("PORT_HEADLESS", -1);
#ifdef __linux__
    bool headless = headlessEnv > 0 || (headlessEnv < 0 && !getenv("WAYLAND_DISPLAY") && !getenv("DISPLAY"));
#else
    bool headless = headlessEnv > 0;   // macOS, Windows: there is always a display to try
#endif
    Window win;
    if (headless && headlessEnv < 0)
        port_log("video: no WAYLAND_DISPLAY or DISPLAY: running headless (set one, or PORT_HEADLESS=0)");
    if (!headless && !win.Open())
        headless = true;
    V.headless = headless;
    // PORT_ASPECT=16:9 (or 1.778): the game's field of view for that shape
    // without a window, for headless tests; the 640x448 screenshots are
    // then the picture before it is stretched to the shape.
    if (const char *a = getenv("PORT_ASPECT"); a && *a) {
        double w = atof(a), h = 1;
        if (const char *colon = strchr(a, ':'))
            h = atof(colon + 1);
        if (w > 0 && h > 0 && headless)
            port_widescreen_set((float)(w / h));
    }
    V.shot = getenv("PORT_SCREENSHOT") ? getenv("PORT_SCREENSHOT") : "";
    V.shotFrame = EnvInt("PORT_SCREENSHOT_FRAME", -1);
    V.shotEvery = EnvInt("PORT_SCREENSHOT_EVERY", 0);
    V.exitFrame = EnvInt("PORT_EXIT_FRAME", -1);
    if (!V.shot.empty() && V.shotFrame < 0 && !V.shotEvery)
        V.shotFrame = 60;
    __atomic_store_n(&V.init, true, __ATOMIC_RELEASE);
    auto lastStatus = std::chrono::steady_clock::now(), lastSecond = lastStatus;
    uint64_t flipsAtSecond = 0;
    std::vector<double> perSecond;  // flips in each second since the last status line
    static const bool fpsLog = getenv("PORT_FPS_LOG");
    while (!*done) {
        // the published frame count first: every frame up to it is queued
        uint64_t seq = gs_frame_seq();
        std::deque<Shot> got;
        {
            std::unique_lock lock(V.mutex);
            if (V.queue.empty())
                V.cv.wait_for(lock, std::chrono::milliseconds(headless ? 50 : 4));
            got.swap(V.queue);
        }
        if (!headless) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) {
                    port_log("video: window closed");
                    SDL_Quit();
                    port_exit(0);
                }
                if (!port_overlay_event(e, win.window))
                    port_input_event(&e);
                else if (e.type == SDL_EVENT_GAMEPAD_ADDED || e.type == SDL_EVENT_GAMEPAD_REMOVED)
                    port_input_event(&e);
            }
            if (port_overlay_take_changed())
                win.Apply();
            port_input_poll();
        }
        bool presented = false;
        for (size_t i = 0; i < got.size(); i++) {
            Shot &f = got[i];
            if (!headless && i + 1 == got.size()) {
                win.Present(f.rgba.data(), f.w, f.h);
                presented = true;
                std::lock_guard lock(V.mutex);
                V.presentPending = false;
            }
            if (Screenshot(f.seq)) {
                std::string path = V.shot;
                if (V.shotEvery) {
                    size_t dot = path.rfind(".png");
                    path.insert(dot == std::string::npos ? path.size() : dot, "_" + std::to_string(f.seq));
                }
                if (WritePng(path, f.rgba.data(), f.w, f.h))
                    port_log("video: frame %llu (%dx%d) saved to %s", (unsigned long long)f.seq, f.w, f.h, path.c_str());
            }
        }
        if (V.exitFrame >= 0 && (long)seq >= V.exitFrame) {
            if (getenv("PORT_DUMP_TEX")) {
                // PORT_DUMP_TEX=<tex0 hex>[,...]: write each texture to
                // tex_<tex0>.png, its alpha to tex_<tex0>_a.png
                std::string list = getenv("PORT_DUMP_TEX");
                for (size_t at = 0; at < list.size();) {
                    size_t comma = list.find(',', at);
                    std::string one = list.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                    std::vector<uint32_t> tex;
                    int tw, th;
                    gs_read_texture(strtoull(one.c_str(), nullptr, 16), tex, tw, th);
                    WritePng("tex_" + one + ".png", tex.data(), tw, th);
                    for (uint32_t &c : tex)
                        c = (c >> 24) * 0x010101;
                    WritePng("tex_" + one + "_a.png", tex.data(), tw, th);
                    at = comma == std::string::npos ? list.size() : comma + 1;
                }
            }
            port_log("video: exit at frame %llu", (unsigned long long)seq);
            port_exit(0);
        }
        auto now = std::chrono::steady_clock::now();
        // the overlay moves without the game: redraw over the last picture
        // (the game loading, or between its 30 frames a second)
        static auto lastDraw = now;
        if (presented)
            lastDraw = now;
        else if (!headless && port_overlay_animating() && now - lastDraw > std::chrono::milliseconds(16)) {
            win.Draw();
            lastDraw = now;
        }
        static long trapAt = EnvInt("PORT_TRAP_SECONDS", 0);  // debugging: stop in gdb after N s
        static auto start = now;
        if (trapAt && now - start > std::chrono::seconds(trapAt)) {
            trapAt = 0;
            port_debug_break();
        }
        if (now - lastSecond >= std::chrono::seconds(1)) {
            // game frames per second (Flips), each second
            double secs = std::chrono::duration<double>(now - lastSecond).count();
            uint64_t flips = gs_flip_count();
            perSecond.push_back((flips - flipsAtSecond) / secs);
            win.fps = perSecond.back();
            if (fpsLog)
                port_log("fps %.1f", perSecond.back());
            flipsAtSecond = flips;
            lastSecond = now;
        }
        if (now - lastStatus > std::chrono::seconds(5)) {
            lastStatus = now;
            double sum = 0, mn = 1e9;
            for (double f : perSecond) {
                sum += f;
                mn = std::min(mn, f);
            }
            char fps[64] = "";
            if (!perSecond.empty())
                snprintf(fps, sizeof fps, ", fps %.1f (min %.1f)", sum / perSecond.size(), mn);
            perSecond.clear();
            std::string line = "status: vblank " + std::to_string(port_vblanks()) + ", frames " +
                               std::to_string(gs_frame_seq()) + ", flips " + std::to_string(gs_flip_count()) + fps +
                               ", prims " + std::to_string(gs_prim_count()) + ", GS waits " +
                               std::to_string(gs_barrier_count()) + ", VU1 " +
                               std::to_string(vu1_instructions() / 1000000) + "M, DMA";
            for (int ch = 0; ch < 10; ch++)
                if (uint64_t n = port_dma_kicks(ch))
                    line += std::string(" ") + port_dma_channel_name(ch) + " " + std::to_string(n);
            port_log("%s", line.c_str());
        }
    }
}
