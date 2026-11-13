// The GPU renderer's vertex shader for Vulkan (ports/pc/gpu.cpp has the
// Metal one; keep the two alike). tools/port/shaders.py compiles it into
// ports/pc/gpu_spirv.h.
#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec4 inStq;
layout(location = 3) in vec2 inZz;
layout(location = 0) out vec4 col;
layout(location = 1) out vec4 stq;
layout(location = 2) flat out float zbase;
layout(location = 3) out float dz;
void main()
{
    // w is 1, so everything is interpolated linearly on the screen, as the
    // GS does (it divides S and T by Q per pixel itself)
    gl_Position = vec4(inPos, 1.0);
    col = inCol;
    stq = inStq;
    zbase = inZz.x;
    dz = inZz.y;
}
