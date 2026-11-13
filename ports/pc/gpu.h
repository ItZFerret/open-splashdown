// The GPU renderer (ports/pc/gpu.cpp): the picture the window shows, drawn
// by the graphics card instead of the software GS's high-resolution twins.
//
// The software GS still draws everything into its local memory at 1x: that
// is what the game reads back and textures from. In GPU mode it also
// records each primitive aimed at a full-width frame buffer (gs/gs.cpp,
// GpuRecord), and at every FINISH hands the list over; the presenter's
// thread replays the lists on the GPU into a colour and a depth texture per
// PS2 buffer, at any resolution, and shows the displayed one.
//
// Stage 2: geometry with the depth test, textures (the software GS's decoded
// copies, sampled with the PS2's wrap modes, filtering and texture
// functions) and the alpha test. Blending and fog are to come; until then
// everything is opaque and the setting is for development.
#ifndef PORT_GPU_H
#define PORT_GPU_H

#include <cstdint>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_FRect;

// A vertex as recorded: x, y in the buffer's pixels at 1x, z in 0..1
// (nearer is larger, as the PS2's), colour as R, G, B, A bytes, and the
// texture coordinates as the GS interpolates them: s and t in texture
// widths and heights, times q; the texel is (s / q, t / q) * size.
struct GpuVertex {
    float x, y, z;
    uint32_t rgba;
    float s, t, q;
};

// A run of triangles (three vertices each) drawn with one state. Everything
// before `first` is the state: zero the struct before filling it in, so two
// draws in the same state compare equal byte for byte.
struct GpuDraw {
    uint32_t fbp, zbp;              // the PS2 frame and depth buffers (block addresses)
    uint16_t sx0, sy0, sx1, sy1;    // scissor, inclusive, 1x pixels
    uint8_t ztest;                  // 0 no depth buffer, 1 always, 2 >=, 3 >
    uint8_t zwrite, cwrite;         // writes depth, writes colour
    // texturing: tex 0 for none, else a GpuTexture's serial
    uint8_t linear, tfx, tcc;       // bilinear; TEX0's texture function and TCC
    uint8_t wms, wmt;               // CLAMP's wrap modes
    uint64_t tex;
    uint16_t tw, th;                // the texture's size in texels
    uint16_t ox, oy;                // where the copy's first texel is in it
    uint16_t minu, maxu, minv, maxv;
    // the alpha test (one that never passes is in zwrite/cwrite instead)
    uint8_t ate, atst, afail, aref;
    uint32_t first, count;          // vertices
};

// A texture the draws name: RGBA8 texels as the GS decodes them (alpha
// 0x80 is opaque). Sent once; oneShot ones are dropped after the frame.
struct GpuTexture {
    uint64_t serial;
    uint16_t w, h;
    bool oneShot;
    std::vector<uint32_t> rgba;
};

// What the GS drew between two FINISHes.
struct GpuFrame {
    std::vector<GpuVertex> verts;
    std::vector<GpuDraw> draws;
    std::vector<GpuTexture> textures;     // new ones, before the draws
    std::vector<uint64_t> released;       // serials no longer used
    bool present = false;           // a picture is complete: show dispFbp's
    uint32_t dispFbp = 0;
    int dx = 0, dy = 0, dw = 0, dh = 0;   // the displayed rectangle, 1x pixels
};

// The GS thread: is the GPU drawing (record primitives)? Hand a frame over
// (waits a little if the presenter is behind).
bool port_gpu_active(void);
void port_gpu_submit(GpuFrame &&frame);

// The presenter's thread. Start: a GPU device and an SDL renderer on it
// for the window (null if the GPU or its shader format isn't available:
// the caller falls back to the software picture). Pump: replay what the GS
// handed over; true when there is a (new) picture, with the texture to
// show and the part of it that is the display.
SDL_Renderer *port_gpu_start(SDL_Window *window, int scale);
bool port_gpu_pump(SDL_Renderer *renderer, SDL_Texture **texture, SDL_FRect *src);

#endif
