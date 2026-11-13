// The GPU renderer: see gpu.h. SDL's GPU API (Metal, Vulkan, Direct3D 12
// behind one interface); the shaders here are Metal's, so for now it runs
// on macOS only and port_gpu_start fails elsewhere.
#include "gpu.h"

#include "port.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <iterator>
#include <mutex>
#include <vector>

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

// What a fragment needs besides its interpolated colour and coordinates.
// (The same layout as the shader's FU: three float2, then int4s at 32.)
struct FragUniforms {
    float texSize[2];    // the texture's size in texels
    float origin[2];     // the copy's first texel in it
    float copySize[2];   // the copy's size
    float pad[2];
    int32_t wrap[4];     // wms, wmt, bilinear, textured
    int32_t region[4];   // minu, maxu, minv, maxv
    int32_t tfx[4];      // texture function, TCC, alpha test on, ATST
    int32_t alpha[4];    // AREF, which pixels are kept (0 those that pass, 1 those that fail)
    int32_t blend[4];    // what the colour is multiplied by (Premul), C is FIX, FBA, fog
    float fog[4];        // the fog colour, and FIX (1.0 is 0x80)
};

// What the shader does to the colour so the blender's factors can do the
// rest: nothing, times C, times 1 - C, times 1 + C, C itself, 1.
enum Premul { kPmNone, kPmCoef, kPmInvCoef, kPmOnePlusCoef, kPmIsCoef, kPmIsOne };

const char kShaderMSL[] = R"(
#include <metal_stdlib>
using namespace metal;
struct VIn {
    float3 pos [[attribute(0)]];
    float4 col [[attribute(1)]];
    float4 stq [[attribute(2)]];
    float2 zz [[attribute(3)]];
};
struct VOut {
    float4 pos [[position]];
    float4 col;
    float4 stq;
    float zbase [[flat]];
    float dz;
};
struct FOutZ {
    float4 col [[color(0)]];
    float depth [[depth(any)]];
};
struct FU {
    float2 texSize;
    float2 origin;
    float2 copySize;
    int4 wrap;
    int4 region;
    int4 tfx;
    int4 alpha;
    int4 blend;
    float4 fog;
};
vertex VOut vmain(VIn in [[stage_in]])
{
    VOut o;
    // w is 1, so everything is interpolated linearly on the screen, as the
    // GS does (it divides S and T by Q per pixel itself)
    o.pos = float4(in.pos, 1.0);
    o.col = in.col;
    o.stq = in.stq;
    o.zbase = in.zz.x;
    o.dz = in.zz.y;
    return o;
}
// The GS's wrap modes: repeat, clamp, clamp to a region, repeat a region.
static int wrap1(int c, int size, int mode, int mn, int mx)
{
    if (mode == 0)
        return c & (size - 1);
    if (mode == 1)
        return clamp(c, 0, size - 1);
    if (mode == 2)
        return clamp(c, mn, mx);
    return (c & mn) | mx;
}
static float4 texel(texture2d<float> tex, constant FU &u, int2 p)
{
    int x = wrap1(p.x, int(u.texSize.x), u.wrap.x, u.region.x, u.region.y) - int(u.origin.x);
    int y = wrap1(p.y, int(u.texSize.y), u.wrap.y, u.region.z, u.region.w) - int(u.origin.y);
    x = clamp(x, 0, int(u.copySize.x) - 1);
    y = clamp(y, 0, int(u.copySize.y) - 1);
    return tex.read(uint2(uint(x), uint(y)));
}
static float4 shade(VOut in, constant FU &u, texture2d<float> tex)
{
    float4 c = in.col;
    if (u.wrap.w != 0) {
        float q = in.stq.z;
        if (abs(q) < 1e-20)
            q = 1e-20;
        float2 uv = in.stq.xy / q * u.texSize;
        float4 t;
        if (u.wrap.z != 0) {
            // bilinear, as the GS: between the four texels around uv - 0.5
            float2 p = uv - 0.5;
            float2 f = floor(p);
            float2 w = p - f;
            int2 i = int2(f);
            float4 top = mix(texel(tex, u, i), texel(tex, u, i + int2(1, 0)), w.x);
            float4 bot = mix(texel(tex, u, i + int2(0, 1)), texel(tex, u, i + int2(1, 1)), w.x);
            t = mix(top, bot, w.y);
        } else {
            t = texel(tex, u, int2(floor(uv)));
        }
        // the texture functions: 0x80 is 1.0 in a product
        const float k = 255.0 / 128.0;
        float3 mod = t.rgb * c.rgb * k;
        bool tcc = u.tfx.y != 0;
        if (u.tfx.x == 0)
            c = float4(mod, tcc ? t.a * c.a * k : c.a);
        else if (u.tfx.x == 1)
            c = float4(t.rgb, tcc ? t.a : c.a);
        else if (u.tfx.x == 2)
            c = float4(mod + c.a, tcc ? t.a + c.a : c.a);
        else
            c = float4(mod + c.a, tcc ? t.a : c.a);
        c = min(c, float4(1.0));
    }
    // fog: the vertex's coefficient between the colour and FOGCOL
    if (u.blend.w != 0)
        c.rgb = mix(u.fog.rgb, c.rgb, in.stq.w);
    // the alpha test. What AFAIL keeps of a failed pixel is drawn in a
    // second pass that takes only those
    if (u.tfx.z != 0) {
        int a = int(c.a * 255.0 + 0.5), ref = u.alpha.x, op = u.tfx.w;
        bool pass = op == 2 ? a < ref : op == 3 ? a <= ref : op == 4 ? a == ref : op == 5 ? a >= ref : op == 6 ? a > ref : a != ref;
        if (pass != (u.alpha.y == 0))
            discard_fragment();
    }
    // blending: the part of the GS's equation the blender's factors can't
    // do. Alpha goes out with 0x80 as 1.0, which is what the factors want
    float a = c.a * (255.0 / 128.0);
    float coef = u.blend.y != 0 ? u.fog.w : a;
    if (u.blend.x == 1)
        c.rgb *= coef;
    else if (u.blend.x == 2)
        c.rgb *= max(1.0 - coef, 0.0);
    else if (u.blend.x == 3)
        c.rgb *= 1.0 + coef;
    else if (u.blend.x == 4)
        c.rgb = float3(coef);
    else if (u.blend.x == 5)
        c.rgb = float3(1.0);
    c.a = u.blend.z != 0 ? 1.0 : min(a, 1.0);
    return c;
}
fragment float4 fmain(VOut in [[stage_in]], constant FU &u [[buffer(0)]], texture2d<float> tex [[texture(0)]],
                      sampler smp [[sampler(0)]])
{
    return shade(in, u, tex);
}
// Depth as the GS computes it: the buffer's integer, cut off from what is
// interpolated (a float depth rounds to the nearest, and at the top of 24
// bits has no fraction to tell; the game's HUD is layers one step apart)
fragment FOutZ fmainz(VOut in [[stage_in]], constant FU &u [[buffer(0)]], texture2d<float> tex [[texture(0)]],
                      sampler smp [[sampler(0)]])
{
    FOutZ o;
    o.col = shade(in, u, tex);
    o.depth = (in.zbase + floor(in.dz)) * (1.0 / 4294967296.0);
    return o;
}
)";

struct ClipVertex {
    float x, y, z;
    uint32_t rgba;
    float s, t, q, f;
    float zbase, dz;
};

// What a pipeline is made for.
struct PipeState {
    uint8_t ztest, zwrite, cmask;
    bool exactz;
    bool blend;
    SDL_GPUBlendOp op;
    SDL_GPUBlendFactor src, dst;
};

// The GS's blend, (A - B) * C + D, as the blender does it: an operation
// and two factors, with the shader multiplying the colour first (Premul)
// where a factor can't. C is taken as at most 1.0 where it is a factor.
// The few forms with a destination factor of 1 + C are drawn without the C.
void BlendFor(const GpuDraw &d, PipeState &ps, int &premul)
{
    premul = kPmNone;
    ps.blend = false;
    ps.op = SDL_GPU_BLENDOP_ADD;
    ps.src = SDL_GPU_BLENDFACTOR_ONE;
    ps.dst = SDL_GPU_BLENDFACTOR_ZERO;
    if (!d.abe)
        return;
    ps.blend = true;
    enum { Z, O, C, IC, P1, DC };   // zero, one, C, 1 - C, one with the colour times 1 + C, destination colour
    enum { ADD, SUB, RSUB };
    struct Form {
        uint8_t op, src, dst;
    };
    static const Form only[3] = {{ADD, O, Z}, {ADD, Z, O}, {ADD, Z, Z}};   // A == B: D alone
    static const Form forms[3][3][3] = {
        // A source
        {{},
         {{SUB, P1, C}, {ADD, C, IC}, {SUB, C, C}},     // B destination
         {{ADD, P1, Z}, {ADD, C, O}, {ADD, C, Z}}},     // B zero
        // A destination
        {{{ADD, IC, C}, {RSUB, C, O}, {RSUB, C, C}},    // B source
         {},
         {{ADD, O, C}, {ADD, DC, O}, {ADD, Z, C}}},     // B zero
        // A zero
        {{{ADD, IC, Z}, {RSUB, C, O}, {ADD, Z, Z}},     // B source
         {{SUB, O, C}, {ADD, Z, IC}, {ADD, Z, Z}},      // B destination
         {}},
    };
    const Form f = d.ba == d.bb ? only[d.bd] : forms[d.ba][d.bb][d.bd];
    ps.op = f.op == ADD ? SDL_GPU_BLENDOP_ADD : f.op == SUB ? SDL_GPU_BLENDOP_SUBTRACT : SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
    const bool dstAlpha = d.bc == 1;
    switch (f.src) {
    case Z: ps.src = SDL_GPU_BLENDFACTOR_ZERO; break;
    case O: ps.src = SDL_GPU_BLENDFACTOR_ONE; break;
    case C:
        ps.src = dstAlpha ? SDL_GPU_BLENDFACTOR_DST_ALPHA : SDL_GPU_BLENDFACTOR_ONE;
        premul = dstAlpha ? kPmNone : kPmCoef;
        break;
    case IC:
        ps.src = dstAlpha ? SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA : SDL_GPU_BLENDFACTOR_ONE;
        premul = dstAlpha ? kPmNone : kPmInvCoef;
        break;
    case P1:
        ps.src = SDL_GPU_BLENDFACTOR_ONE;
        premul = dstAlpha ? kPmNone : kPmOnePlusCoef;
        break;
    case DC:
        ps.src = SDL_GPU_BLENDFACTOR_DST_COLOR;
        premul = dstAlpha ? kPmIsOne : kPmIsCoef;
        break;
    }
    switch (f.dst) {
    case Z: ps.dst = SDL_GPU_BLENDFACTOR_ZERO; break;
    case O: ps.dst = SDL_GPU_BLENDFACTOR_ONE; break;
    case C:
        ps.dst = d.bc == 0   ? SDL_GPU_BLENDFACTOR_SRC_ALPHA
                 : d.bc == 1 ? SDL_GPU_BLENDFACTOR_DST_ALPHA
                             : SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        break;
    case IC:
        ps.dst = d.bc == 0   ? SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
                 : d.bc == 1 ? SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA
                             : SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
        break;
    }
}

struct Target {
    SDL_GPUTexture *gpu = nullptr;
    SDL_Texture *sdl = nullptr;   // colour targets: the same texture for the SDL renderer
};

struct {
    SDL_GPUDevice *device;
    SDL_GPUShader *vs, *fs, *fsz;   // fsz: computes the depth itself (exactz)
    int scale = 1;
    std::map<uint32_t, SDL_GPUGraphicsPipeline *> pipelines;   // by depth test, depth write, colour write, has depth
    std::map<uint32_t, Target> color, depth;                   // by PS2 block address
    SDL_GPUBuffer *vbuf;
    SDL_GPUTransferBuffer *xfer;
    uint32_t vcap;   // vertices the buffers hold
    // the GS's textures by serial (GpuTexture), and what an untextured draw binds
    struct Tex {
        SDL_GPUTexture *gpu;
        uint16_t w, h;
        bool oneShot;
    };
    std::map<uint64_t, Tex> textures;
    SDL_GPUTexture *white;
    SDL_GPUSampler *sampler;
    SDL_GPUTransferBuffer *texXfer;
    uint32_t texXferBytes;
    std::vector<GpuTexture> carried;    // of frames dropped: still to be sent
    std::vector<uint64_t> carriedReleased;
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

SDL_GPUGraphicsPipeline *Pipeline(const PipeState &d)
{
    uint32_t key = d.ztest | d.zwrite << 2 | d.cmask << 3 | d.blend << 7 | (uint32_t)d.op << 8 | (uint32_t)d.src << 12 |
                   (uint32_t)d.dst << 16 | (uint32_t)d.exactz << 20;
    auto it = G.pipelines.find(key);
    if (it != G.pipelines.end())
        return it->second;
    SDL_GPUVertexBufferDescription vb = {};
    vb.slot = 0;
    vb.pitch = sizeof(ClipVertex);
    vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexAttribute attrs[4] = {};
    attrs[3].location = 3;
    attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[3].offset = 32;
    attrs[0].location = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    attrs[1].offset = 12;
    attrs[2].location = 2;
    attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attrs[2].offset = 16;
    SDL_GPUColorTargetDescription ct = {};
    ct.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ct.blend_state.enable_color_write_mask = true;
    ct.blend_state.color_write_mask = d.cmask;   // (the same bits: R, G, B, A)
    if (d.blend) {
        // alpha isn't blended: the source's is written
        ct.blend_state.enable_blend = true;
        ct.blend_state.color_blend_op = d.op;
        ct.blend_state.src_color_blendfactor = d.src;
        ct.blend_state.dst_color_blendfactor = d.dst;
        ct.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        ct.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        ct.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    }
    SDL_GPUGraphicsPipelineCreateInfo info = {};
    info.vertex_shader = G.vs;
    info.fragment_shader = d.exactz ? G.fsz : G.fs;
    info.vertex_input_state.vertex_buffer_descriptions = &vb;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attrs;
    info.vertex_input_state.num_vertex_attributes = 4;
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
        out[i] = {v.x * (2.0f / kW) - 1.0f, 1.0f - v.y * (2.0f / kH), v.z, v.rgba, v.s, v.t, v.q, v.f, v.zbase, v.dz};
    }
    SDL_UnmapGPUTransferBuffer(G.device, G.xfer);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation from = {G.xfer, 0};
    SDL_GPUBufferRegion to = {G.vbuf, 0, n * (uint32_t)sizeof(ClipVertex)};
    SDL_UploadToGPUBuffer(copy, &from, &to, true);
    SDL_EndGPUCopyPass(copy);
}

SDL_GPUTexture *NewTexture(uint32_t w, uint32_t h)
{
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = w;
    info.height = h;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    SDL_GPUTexture *t = SDL_CreateGPUTexture(G.device, &info);
    if (!t)
        port_fatal("GPU: no %ux%u texture (%s)", w, h, SDL_GetError());
    return t;
}

// The frame's new textures, onto the GPU (one transfer buffer, one copy pass).
void UploadTextures(SDL_GPUCommandBuffer *cmd, const std::vector<GpuTexture> &list)
{
    uint32_t bytes = 0;
    for (const GpuTexture &t : list)
        bytes += (uint32_t)t.rgba.size() * 4;
    if (!bytes)
        return;
    if (bytes > G.texXferBytes) {
        if (G.texXfer)
            SDL_ReleaseGPUTransferBuffer(G.device, G.texXfer);
        G.texXferBytes = bytes + bytes / 2;
        SDL_GPUTransferBufferCreateInfo ti = {};
        ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        ti.size = G.texXferBytes;
        G.texXfer = SDL_CreateGPUTransferBuffer(G.device, &ti);
        if (!G.texXfer)
            port_fatal("GPU: no transfer buffer for %u bytes of textures (%s)", bytes, SDL_GetError());
    }
    uint8_t *out = (uint8_t *)SDL_MapGPUTransferBuffer(G.device, G.texXfer, true);
    uint32_t at = 0;
    for (const GpuTexture &t : list) {
        memcpy(out + at, t.rgba.data(), t.rgba.size() * 4);
        at += (uint32_t)t.rgba.size() * 4;
    }
    SDL_UnmapGPUTransferBuffer(G.device, G.texXfer);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    at = 0;
    for (const GpuTexture &t : list) {
        if (t.rgba.empty() || !t.w || !t.h)
            continue;
        auto &slot = G.textures[t.serial];
        if (slot.gpu)
            SDL_ReleaseGPUTexture(G.device, slot.gpu);
        slot = {NewTexture(t.w, t.h), t.w, t.h, t.oneShot};
        SDL_GPUTextureTransferInfo from = {};
        from.transfer_buffer = G.texXfer;
        from.offset = at;
        from.pixels_per_row = t.w;
        from.rows_per_layer = t.h;
        SDL_GPUTextureRegion to = {};
        to.texture = slot.gpu;
        to.w = t.w;
        to.h = t.h;
        to.d = 1;
        SDL_UploadToGPUTexture(copy, &from, &to, false);
        at += (uint32_t)t.rgba.size() * 4;
    }
    SDL_EndGPUCopyPass(copy);
}

void ReleaseTextures(const GpuFrame &f)
{
    for (uint64_t serial : f.released) {
        auto it = G.textures.find(serial);
        if (it != G.textures.end()) {
            SDL_ReleaseGPUTexture(G.device, it->second.gpu);
            G.textures.erase(it);
        }
    }
    for (const GpuTexture &t : f.textures)
        if (t.oneShot) {
            auto it = G.textures.find(t.serial);
            if (it != G.textures.end()) {
                SDL_ReleaseGPUTexture(G.device, it->second.gpu);
                G.textures.erase(it);
            }
        }
}

void Replay(SDL_Renderer *renderer, const GpuFrame &f)
{
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(G.device);
    UploadTextures(cmd, f.textures);
    if (f.draws.empty() || f.verts.empty()) {
        SDL_SubmitGPUCommandBuffer(cmd);
        ReleaseTextures(f);
        return;
    }
    Upload(cmd, f);
    uint64_t boundTex = ~0ull;
    FragUniforms boundU;
    bool haveU = false;
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
            boundTex = ~0ull;
        }
        // the texture, and what the fragment shader does with it
        auto tex = d.tex ? G.textures.find(d.tex) : G.textures.end();
        bool textured = tex != G.textures.end();
        uint64_t want = textured ? d.tex : 0;
        if (want != boundTex) {
            SDL_GPUTextureSamplerBinding tb = {textured ? tex->second.gpu : G.white, G.sampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &tb, 1);
            boundTex = want;
        }
        FragUniforms u = {};
        if (textured) {
            u.texSize[0] = d.tw;
            u.texSize[1] = d.th;
            u.origin[0] = d.ox;
            u.origin[1] = d.oy;
            u.copySize[0] = tex->second.w;
            u.copySize[1] = tex->second.h;
            u.wrap[0] = d.wms;
            u.wrap[1] = d.wmt;
            u.wrap[2] = d.linear;
            u.wrap[3] = 1;
            u.region[0] = d.minu;
            u.region[1] = d.maxu;
            u.region[2] = d.minv;
            u.region[3] = d.maxv;
            u.tfx[0] = d.tfx;
            u.tfx[1] = d.tcc;
        }
        u.tfx[2] = d.ate;
        u.tfx[3] = d.atst;
        u.alpha[0] = d.aref;
        PipeState ps = {};
        ps.ztest = d.ztest;
        ps.zwrite = d.zwrite;
        ps.exactz = d.exactz && d.ztest;
        ps.cmask = d.cwrite ? d.cmask : 0;
        int premul;
        BlendFor(d, ps, premul);
        u.blend[0] = premul;
        u.blend[1] = d.bc == 2;
        // (FBA with source alpha as a factor: the factor matters more)
        u.blend[2] = d.fba && !(ps.blend && d.bc == 0);
        u.blend[3] = d.fge;
        u.fog[0] = d.fogr / 255.0f;
        u.fog[1] = d.fogg / 255.0f;
        u.fog[2] = d.fogb / 255.0f;
        u.fog[3] = d.fix / 128.0f;
        if (ps.blend && d.bc == 2) {
            float c = std::min(d.fix / 128.0f, 1.0f);
            SDL_SetGPUBlendConstants(pass, SDL_FColor{c, c, c, c});
        }
        SDL_Rect sc = {d.sx0 * N, d.sy0 * N, (d.sx1 - d.sx0 + 1) * N, (d.sy1 - d.sy0 + 1) * N};
        SDL_SetGPUScissor(pass, &sc);
        // the pixels that pass the alpha test; then, where AFAIL keeps
        // part of those that fail, them with only that part written
        for (int failed = 0; failed < 2; failed++) {
            if (failed) {
                if (!d.ate || d.afail == 0)
                    break;
                if (d.afail == 1) {
                    ps.zwrite = 0;
                } else if (d.afail == 2) {
                    ps.cmask = 0;
                } else {
                    ps.zwrite = 0;
                    ps.cmask &= 7;
                }
                if (!ps.cmask && !ps.zwrite)
                    break;
            }
            u.alpha[1] = failed;
            if (!haveU || memcmp(&u, &boundU, sizeof u)) {
                SDL_PushGPUFragmentUniformData(cmd, 0, &u, sizeof u);
                boundU = u;
                haveU = true;
            }
            SDL_GPUGraphicsPipeline *p = Pipeline(ps);
            if (p != bound) {
                SDL_BindGPUGraphicsPipeline(pass, p);
                bound = p;
            }
            SDL_DrawGPUPrimitives(pass, d.count, 1, d.first, 0);
        }
    }
    if (pass)
        SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    ReleaseTextures(f);
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
        // its drawing is lost; the textures it brought are still needed
        GpuFrame &old = G.queue.front();
        for (GpuTexture &t : old.textures)
            if (!t.oneShot)
                G.carried.push_back(std::move(t));
        G.carriedReleased.insert(G.carriedReleased.end(), old.released.begin(), old.released.end());
        G.queue.pop_front();
    }
    if (!G.carried.empty() || !G.carriedReleased.empty()) {
        frame.textures.insert(frame.textures.begin(), std::make_move_iterator(G.carried.begin()),
                              std::make_move_iterator(G.carried.end()));
        frame.released.insert(frame.released.begin(), G.carriedReleased.begin(), G.carriedReleased.end());
        G.carried.clear();
        G.carriedReleased.clear();
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
    si.num_samplers = 1;
    si.num_uniform_buffers = 1;
    G.fs = SDL_CreateGPUShader(G.device, &si);
    si.entrypoint = "fmainz";
    G.fsz = SDL_CreateGPUShader(G.device, &si);
    if (!G.vs || !G.fs || !G.fsz)
        port_log("GPU: the shaders didn't compile (%s)", SDL_GetError());
    SDL_Renderer *renderer = G.vs && G.fs && G.fsz ? SDL_CreateGPURenderer(G.device, window) : nullptr;
    if (!renderer) {
        port_log("GPU: can't start (vertex shader %p, fragment shader %p, renderer: %s): the software picture", (void *)G.vs,
                 (void *)G.fs, SDL_GetError());
        SDL_DestroyGPUDevice(G.device);
        G.device = nullptr;
        return nullptr;
    }
    // what an untextured draw binds (the shader doesn't read it), and the
    // sampler every binding needs (texels are read by coordinate)
    G.white = NewTexture(1, 1);
    SDL_GPUSamplerCreateInfo sm = {};
    sm.min_filter = sm.mag_filter = SDL_GPU_FILTER_NEAREST;
    sm.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sm.address_mode_u = sm.address_mode_v = sm.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    G.sampler = SDL_CreateGPUSampler(G.device, &sm);
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
