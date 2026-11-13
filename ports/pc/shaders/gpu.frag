// The GPU renderer's fragment shader for Vulkan (ports/pc/gpu.cpp has the
// Metal one; keep the two alike). tools/port/shaders.py compiles it into
// ports/pc/gpu_spirv.h twice: as it is, and with EXACTZ, which computes the
// depth itself.
#version 450
layout(location = 0) in vec4 col;
layout(location = 1) in vec4 stq;
layout(location = 2) flat in float zbase;
layout(location = 3) in float dz;
layout(location = 0) out vec4 outCol;
// SDL's places for a fragment shader's resources: textures in set 2,
// uniforms in set 3
layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 3, binding = 0, std140) uniform FU {
    vec2 texSize;
    vec2 origin;
    vec2 copySize;
    ivec4 wrap;
    ivec4 region;
    ivec4 tfx;
    ivec4 alpha;
    ivec4 blend;
    vec4 fog;
} u;
// The GS's wrap modes: repeat, clamp, clamp to a region, repeat a region.
int wrap1(int c, int size, int mode, int mn, int mx)
{
    if (mode == 0)
        return c & (size - 1);
    if (mode == 1)
        return clamp(c, 0, size - 1);
    if (mode == 2)
        return clamp(c, mn, mx);
    return (c & mn) | mx;
}
vec4 texel(ivec2 p)
{
    int x = wrap1(p.x, int(u.texSize.x), u.wrap.x, u.region.x, u.region.y) - int(u.origin.x);
    int y = wrap1(p.y, int(u.texSize.y), u.wrap.y, u.region.z, u.region.w) - int(u.origin.y);
    x = clamp(x, 0, int(u.copySize.x) - 1);
    y = clamp(y, 0, int(u.copySize.y) - 1);
    return texelFetch(tex, ivec2(x, y), 0);
}
void main()
{
    vec4 c = col;
    if (u.wrap.w != 0) {
        float q = stq.z;
        if (abs(q) < 1e-20)
            q = 1e-20;
        vec2 uv = stq.xy / q * u.texSize;
        vec4 t;
        if (u.wrap.z != 0) {
            // bilinear, as the GS: between the four texels around uv - 0.5
            vec2 p = uv - 0.5;
            vec2 f = floor(p);
            vec2 w = p - f;
            ivec2 i = ivec2(f);
            vec4 top = mix(texel(i), texel(i + ivec2(1, 0)), w.x);
            vec4 bot = mix(texel(i + ivec2(0, 1)), texel(i + ivec2(1, 1)), w.x);
            t = mix(top, bot, w.y);
        } else {
            t = texel(ivec2(floor(uv)));
        }
        // the texture functions: 0x80 is 1.0 in a product
        const float k = 255.0 / 128.0;
        vec3 mod = t.rgb * c.rgb * k;
        bool tcc = u.tfx.y != 0;
        if (u.tfx.x == 0)
            c = vec4(mod, tcc ? t.a * c.a * k : c.a);
        else if (u.tfx.x == 1)
            c = vec4(t.rgb, tcc ? t.a : c.a);
        else if (u.tfx.x == 2)
            c = vec4(mod + c.a, tcc ? t.a + c.a : c.a);
        else
            c = vec4(mod + c.a, tcc ? t.a : c.a);
        c = min(c, vec4(1.0));
    }
    // fog: the vertex's coefficient between the colour and FOGCOL
    if (u.blend.w != 0)
        c.rgb = mix(u.fog.rgb, c.rgb, stq.w);
    // the alpha test. What AFAIL keeps of a failed pixel is drawn in a
    // second pass that takes only those
    if (u.tfx.z != 0) {
        int a = int(c.a * 255.0 + 0.5), ref = u.alpha.x, op = u.tfx.w;
        bool pass = op == 2 ? a < ref : op == 3 ? a <= ref : op == 4 ? a == ref : op == 5 ? a >= ref : op == 6 ? a > ref : a != ref;
        if (pass != (u.alpha.y == 0))
            discard;
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
        c.rgb = vec3(coef);
    else if (u.blend.x == 5)
        c.rgb = vec3(1.0);
    c.a = u.blend.z != 0 ? 1.0 : min(a, 1.0);
    outCol = c;
#ifdef EXACTZ
    // Depth as the GS computes it: the buffer's integer, cut off from what
    // is interpolated (a float depth rounds to the nearest, and at the top
    // of 24 bits has no fraction to tell; the game's HUD is layers one step
    // apart)
    gl_FragDepth = (zbase + floor(dz)) * (1.0 / 4294967296.0);
#endif
}
