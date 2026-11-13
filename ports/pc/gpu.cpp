// The GPU renderer: see gpu.h. SDL's GPU API (Metal, Vulkan, Direct3D 12
// behind one interface); the shaders here are Metal's, so for now it runs
// on macOS only and port_gpu_start fails elsewhere.
#include "gpu.h"

#include "port.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>

// SDL_CreateGPURenderer, which lets the window's SDL renderer and the F1
// overlay share the GPU device, is SDL 3.4's: with an older SDL the GPU
// renderer is left out and the setting falls back to the software picture.
#if !SDL_VERSION_ATLEAST(3, 4, 0)

bool port_gpu_active(void)
{
    return false;
}

void port_gpu_submit(GpuFrame &&)
{
}

SDL_Renderer *port_gpu_start(SDL_Window *, int)
{
    port_log("GPU: built with an SDL older than 3.4: the software picture");
    return nullptr;
}

bool port_gpu_pump(SDL_Renderer *, SDL_Texture **, SDL_FRect *)
{
    return false;
}

#else

namespace {

const int kW = 640, kH = 512;   // a PS2 buffer's pixels at 1x

const char kShaderMSL[] = R"(
#include <metal_stdlib>
using namespace metal;
struct VIn {
    float3 pos [[attribute(0)]];
    float4 col [[attribute(1)]];
};
struct VOut {
    float4 pos [[position]];
    float4 col;
};
vertex VOut vmain(VIn in [[stage_in]])
{
    VOut o;
    o.pos = float4(in.pos, 1.0);
    o.col = in.col;
    return o;
}
fragment float4 fmain(VOut in [[stage_in]])
{
    return in.col;
}
)";

struct ClipVertex {
    float x, y, z;
    uint32_t rgba;
};

struct Target {
    SDL_GPUTexture *gpu = nullptr;
    SDL_Texture *sdl = nullptr;   // colour targets: the same texture for the SDL renderer
};

struct {
    SDL_GPUDevice *device;
    SDL_GPUShader *vs, *fs;
    int scale = 1;
    std::map<uint32_t, SDL_GPUGraphicsPipeline *> pipelines;   // by depth test, depth write, colour write, has depth
    std::map<uint32_t, Target> color, depth;                   // by PS2 block address
    SDL_GPUBuffer *vbuf;
    SDL_GPUTransferBuffer *xfer;
    uint32_t vcap;   // vertices the buffers hold
    // what is shown
    SDL_Texture *shown;
    SDL_FRect src;
    // from the GS thread
    std::mutex mutex;
    std::condition_variable room;
    std::deque<GpuFrame> queue;
    std::atomic<bool> active;
    uint64_t dropped;
} G;

SDL_GPUGraphicsPipeline *Pipeline(const GpuDraw &d)
{
    uint32_t key = d.ztest | d.zwrite << 4 | d.cwrite << 5;
    auto it = G.pipelines.find(key);
    if (it != G.pipelines.end())
        return it->second;
    SDL_GPUVertexBufferDescription vb = {};
    vb.slot = 0;
    vb.pitch = sizeof(ClipVertex);
    vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexAttribute attrs[2] = {};
    attrs[0].location = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    attrs[1].offset = 12;
    SDL_GPUColorTargetDescription ct = {};
    ct.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ct.blend_state.enable_color_write_mask = true;
    ct.blend_state.color_write_mask =
        d.cwrite ? SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A : 0;
    SDL_GPUGraphicsPipelineCreateInfo info = {};
    info.vertex_shader = G.vs;
    info.fragment_shader = G.fs;
    info.vertex_input_state.vertex_buffer_descriptions = &vb;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attrs;
    info.vertex_input_state.num_vertex_attributes = 2;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.target_info.color_target_descriptions = &ct;
    info.target_info.num_color_targets = 1;
    if (d.ztest) {
        // the PS2's depth: larger is nearer
        info.target_info.has_depth_stencil_target = true;
        info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.depth_stencil_state.enable_depth_test = true;
        info.depth_stencil_state.enable_depth_write = d.zwrite != 0;
        info.depth_stencil_state.compare_op = d.ztest == 1   ? SDL_GPU_COMPAREOP_ALWAYS
                                              : d.ztest == 2 ? SDL_GPU_COMPAREOP_GREATER_OR_EQUAL
                                                             : SDL_GPU_COMPAREOP_GREATER;
    }
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(G.device, &info);
    if (!p)
        port_fatal("GPU: no pipeline for state %x (%s)", key, SDL_GetError());
    G.pipelines[key] = p;
    return p;
}

Target &ColorTarget(SDL_Renderer *renderer, uint32_t fbp)
{
    Target &t = G.color[fbp];
    if (t.gpu)
        return t;
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = kW * G.scale;
    info.height = kH * G.scale;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    t.gpu = SDL_CreateGPUTexture(G.device, &info);
    if (!t.gpu)
        port_fatal("GPU: no %dx%d colour target (%s)", info.width, info.height, SDL_GetError());
    // the same texture for the SDL renderer, to show it
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER, t.gpu);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_ABGR8888);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STATIC);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, info.width);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, info.height);
    t.sdl = SDL_CreateTextureWithProperties(renderer, props);
    SDL_DestroyProperties(props);
    if (!t.sdl)
        port_fatal("GPU: the SDL renderer can't use the colour target (%s)", SDL_GetError());
    SDL_SetTextureBlendMode(t.sdl, SDL_BLENDMODE_NONE);
    port_log("GPU: colour target for frame buffer %u (%dx%d)", fbp, info.width, info.height);
    return t;
}

Target &DepthTarget(uint32_t zbp)
{
    Target &t = G.depth[zbp];
    if (t.gpu)
        return t;
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = kW * G.scale;
    info.height = kH * G.scale;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    t.gpu = SDL_CreateGPUTexture(G.device, &info);
    if (!t.gpu)
        port_fatal("GPU: no depth target (%s)", SDL_GetError());
    return t;
}

// The frame's vertices, in clip space, into the vertex buffer.
void Upload(SDL_GPUCommandBuffer *cmd, const GpuFrame &f)
{
    uint32_t n = (uint32_t)f.verts.size();
    if (n > G.vcap) {
        if (G.vbuf)
            SDL_ReleaseGPUBuffer(G.device, G.vbuf);
        if (G.xfer)
            SDL_ReleaseGPUTransferBuffer(G.device, G.xfer);
        G.vcap = n + n / 2 + 4096;
        SDL_GPUBufferCreateInfo bi = {};
        bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bi.size = G.vcap * sizeof(ClipVertex);
        G.vbuf = SDL_CreateGPUBuffer(G.device, &bi);
        SDL_GPUTransferBufferCreateInfo ti = {};
        ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        ti.size = bi.size;
        G.xfer = SDL_CreateGPUTransferBuffer(G.device, &ti);
        if (!G.vbuf || !G.xfer)
            port_fatal("GPU: no vertex buffer for %u vertices (%s)", G.vcap, SDL_GetError());
    }
    ClipVertex *out = (ClipVertex *)SDL_MapGPUTransferBuffer(G.device, G.xfer, true);
    for (uint32_t i = 0; i < n; i++) {
        const GpuVertex &v = f.verts[i];
        out[i] = {v.x * (2.0f / kW) - 1.0f, 1.0f - v.y * (2.0f / kH), v.z, v.rgba};
    }
    SDL_UnmapGPUTransferBuffer(G.device, G.xfer);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation from = {G.xfer, 0};
    SDL_GPUBufferRegion to = {G.vbuf, 0, n * (uint32_t)sizeof(ClipVertex)};
    SDL_UploadToGPUBuffer(copy, &from, &to, true);
    SDL_EndGPUCopyPass(copy);
}

void Replay(SDL_Renderer *renderer, const GpuFrame &f)
{
    if (f.draws.empty() || f.verts.empty())
        return;
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(G.device);
    Upload(cmd, f);
    SDL_GPURenderPass *pass = nullptr;
    uint32_t passFbp = ~0u, passZbp = ~0u;
    bool passDepth = false;
    SDL_GPUGraphicsPipeline *bound = nullptr;
    const int N = G.scale;
    for (const GpuDraw &d : f.draws) {
        bool depth = d.ztest != 0;
        // a pass per pair of targets; their contents carry over
        if (!pass || d.fbp != passFbp || depth != passDepth || (depth && d.zbp != passZbp)) {
            if (pass)
                SDL_EndGPURenderPass(pass);
            SDL_GPUColorTargetInfo ct = {};
            ct.texture = ColorTarget(renderer, d.fbp).gpu;
            ct.load_op = SDL_GPU_LOADOP_LOAD;
            ct.store_op = SDL_GPU_STOREOP_STORE;
            SDL_GPUDepthStencilTargetInfo dt = {};
            if (depth) {
                dt.texture = DepthTarget(d.zbp).gpu;
                dt.load_op = SDL_GPU_LOADOP_LOAD;
                dt.store_op = SDL_GPU_STOREOP_STORE;
                dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
                dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
            }
            pass = SDL_BeginGPURenderPass(cmd, &ct, 1, depth ? &dt : nullptr);
            SDL_GPUBufferBinding vb = {G.vbuf, 0};
            SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
            passFbp = d.fbp;
            passZbp = d.zbp;
            passDepth = depth;
            bound = nullptr;
        }
        SDL_GPUGraphicsPipeline *p = Pipeline(d);
        if (p != bound) {
            SDL_BindGPUGraphicsPipeline(pass, p);
            bound = p;
        }
        SDL_Rect sc = {d.sx0 * N, d.sy0 * N, (d.sx1 - d.sx0 + 1) * N, (d.sy1 - d.sy0 + 1) * N};
        SDL_SetGPUScissor(pass, &sc);
        SDL_DrawGPUPrimitives(pass, d.count, 1, d.first, 0);
    }
    if (pass)
        SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

} // namespace

bool port_gpu_active(void)
{
    return G.active.load(std::memory_order_relaxed);
}

void port_gpu_submit(GpuFrame &&frame)
{
    std::unique_lock lock(G.mutex);
    // the presenter replays at its own pace: wait for room, but not for
    // ever (a window being dragged stops it)
    if (!G.room.wait_for(lock, std::chrono::milliseconds(200), [] { return G.queue.size() < 4; })) {
        if (G.dropped++ == 0)
            port_log("GPU: the presenter is behind: frames are being dropped");
        G.queue.pop_front();
    }
    G.queue.push_back(std::move(frame));
}

SDL_Renderer *port_gpu_start(SDL_Window *window, int scale)
{
    G.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
    if (!G.device) {
        port_log("GPU: no device that takes Metal shaders (%s): the software picture", SDL_GetError());
        return nullptr;
    }
    SDL_GPUShaderCreateInfo si = {};
    si.code = (const Uint8 *)kShaderMSL;
    si.code_size = sizeof kShaderMSL - 1;   // the text, without its terminating zero
    si.format = SDL_GPU_SHADERFORMAT_MSL;
    si.entrypoint = "vmain";
    si.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    G.vs = SDL_CreateGPUShader(G.device, &si);
    si.entrypoint = "fmain";
    si.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    G.fs = SDL_CreateGPUShader(G.device, &si);
    if (!G.vs || !G.fs)
        port_log("GPU: the shaders didn't compile (%s)", SDL_GetError());
    SDL_Renderer *renderer = G.vs && G.fs ? SDL_CreateGPURenderer(G.device, window) : nullptr;
    if (!renderer) {
        port_log("GPU: can't start (vertex shader %p, fragment shader %p, renderer: %s): the software picture", (void *)G.vs,
                 (void *)G.fs, SDL_GetError());
        SDL_DestroyGPUDevice(G.device);
        G.device = nullptr;
        return nullptr;
    }
    G.scale = scale < 1 ? 1 : scale;
    G.active = true;
    port_log("GPU: %s, %dx%d", SDL_GetGPUDeviceDriver(G.device), 640 * G.scale, 448 * G.scale);
    return renderer;
}

bool port_gpu_pump(SDL_Renderer *renderer, SDL_Texture **texture, SDL_FRect *src)
{
    std::deque<GpuFrame> got;
    {
        std::lock_guard lock(G.mutex);
        got.swap(G.queue);
    }
    G.room.notify_all();
    bool fresh = false;
    for (const GpuFrame &f : got) {
        Replay(renderer, f);
        if (f.present && G.color.count(f.dispFbp)) {
            G.shown = G.color[f.dispFbp].sdl;
            G.src = {(float)f.dx * G.scale, (float)f.dy * G.scale, (float)f.dw * G.scale, (float)f.dh * G.scale};
            fresh = true;
        }
    }
    *texture = G.shown;
    *src = G.src;
    return fresh;
}

#endif // SDL 3.4
