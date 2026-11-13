// The VU microprograms on the PC.
//
// Each of these names a DMA chain that uploads one VU0/VU1 microprogram by
// VIF MPG codes. The PC build carries retail's upload packets byte for byte
// (build/port/gen/vupackets.s, from tools/port/vu.py gen), so the chains the
// game sends or copies into its render queue (AddMPGToRenderQueue copies the
// REF tag) are the real ones, and VU1 runs the real microcode
// (ports/pc/vu/vu1.cpp). Here: the programs' names, for logs.
#include "port.h"
#include "port_ptr32.h"

#include <cstring>

extern "C" {
extern char AnimShadowCastCode[];
extern char CubeSubdivCode[];
extern char EcoSystemCode2D[];
extern char Envmap2Code[];
extern char Envmap2CompressedCode[];
extern char EnvmapCode[];
extern char KickGSCode[];
extern char NopLoop[];
extern char StripOptFogCodeNoKick[];
extern char ParticleCode[];
extern char ParticlePhysicsCode[];
extern char Polyline2DRendererCode[];
extern char ProjectedShadowCode[];
extern char ProjectedStripRendererCode[];
extern char ProjtexLitCode[];
extern char ProjtexPrelitCode[];
extern char Quad2DRendererCode[];
extern char Rectangle2DRendererCode[];
extern char ScissorCoreCode[];
extern char ShadowReceiverCode[];
extern char SpecularMap2Code[];
extern char SpecularMapCode[];
extern char SpecularMapCompressedCode[];
extern char StripDiffuseAndPrelitCode[];
extern char StripFogCullCode[];
extern char StripLitCode[];
extern char StripLitCompressedCode[];
extern char StripOptFogEmbossShiftedCode[];
extern char StripOptTexXformCode[];
extern char StripPreambleCode[];
extern char StripPrelitCode[];
extern char StripPrelitCompressedCode[];
extern char StripRendererCode[];
extern char StripScisRendererCode[];
extern char StripTexTransCompressedCode[];
extern char StripUnlitCode[];
extern char StripWireframeCode[];
extern char TerrainRendererClipCode[];
extern char TerrainRendererHalfResCode[];
extern char TerrainScisHalfResCode[];
extern char TerrainScisPreambleCode[];
extern char TerrainScissorCode[];
extern char TerrainShadowHalfResCode[];
extern char TerrainShadowScisCode[];
extern char Triangle2DRendererCode[];
extern char Vu0MiscCode[];
extern char Vu0TransformNoLightNoClipCode[];
extern char WaterObjectCode[];
extern char WaterScreenSpaceRendererCode[];
extern char port_vupackets_start[];
extern char port_vupackets_end[];
extern const uint32_t port_vurelocs[];
extern const uint32_t port_vurelocs_end[];
}

// When the packets come straight from the executable (tools/port/vu.py
// gen_elf), their REF tags still hold retail's addresses: point each at its
// data in the block. After ports/pc/hw.cpp's HwInit, before any game code.
__attribute__((constructor(102))) static void RelocatePackets()
{
    for (const uint32_t *r = port_vurelocs; r < port_vurelocs_end; r += 2) {
        uintptr_t target = (uintptr_t)port_vupackets_start + r[1];
        // as the game's 32-bit address (fatal if the executable isn't in
        // the game's window: include/port_ptr32.h)
        uint32_t t = port_ptr32((const void *)target);
        memcpy(port_vupackets_start + r[0], &t, 4);
    }
}

static const struct {
    const char *name;
    const char *code;
} kPrograms[] = {
    {"AnimShadowCastCode", AnimShadowCastCode},
    {"CubeSubdivCode", CubeSubdivCode},
    {"EcoSystemCode2D", EcoSystemCode2D},
    {"Envmap2Code", Envmap2Code},
    {"Envmap2CompressedCode", Envmap2CompressedCode},
    {"EnvmapCode", EnvmapCode},
    {"KickGSCode", KickGSCode},
    {"NopLoop", NopLoop},
    {"StripOptFogCodeNoKick", StripOptFogCodeNoKick},
    {"ParticleCode", ParticleCode},
    {"ParticlePhysicsCode", ParticlePhysicsCode},
    {"Polyline2DRendererCode", Polyline2DRendererCode},
    {"ProjectedShadowCode", ProjectedShadowCode},
    {"ProjectedStripRendererCode", ProjectedStripRendererCode},
    {"ProjtexLitCode", ProjtexLitCode},
    {"ProjtexPrelitCode", ProjtexPrelitCode},
    {"Quad2DRendererCode", Quad2DRendererCode},
    {"Rectangle2DRendererCode", Rectangle2DRendererCode},
    {"ScissorCoreCode", ScissorCoreCode},
    {"ShadowReceiverCode", ShadowReceiverCode},
    {"SpecularMap2Code", SpecularMap2Code},
    {"SpecularMapCode", SpecularMapCode},
    {"SpecularMapCompressedCode", SpecularMapCompressedCode},
    {"StripDiffuseAndPrelitCode", StripDiffuseAndPrelitCode},
    {"StripFogCullCode", StripFogCullCode},
    {"StripLitCode", StripLitCode},
    {"StripLitCompressedCode", StripLitCompressedCode},
    {"StripOptFogEmbossShiftedCode", StripOptFogEmbossShiftedCode},
    {"StripOptTexXformCode", StripOptTexXformCode},
    {"StripPreambleCode", StripPreambleCode},
    {"StripPrelitCode", StripPrelitCode},
    {"StripPrelitCompressedCode", StripPrelitCompressedCode},
    {"StripRendererCode", StripRendererCode},
    {"StripScisRendererCode", StripScisRendererCode},
    {"StripTexTransCompressedCode", StripTexTransCompressedCode},
    {"StripUnlitCode", StripUnlitCode},
    {"StripWireframeCode", StripWireframeCode},
    {"TerrainRendererClipCode", TerrainRendererClipCode},
    {"TerrainRendererHalfResCode", TerrainRendererHalfResCode},
    {"TerrainScisHalfResCode", TerrainScisHalfResCode},
    {"TerrainScisPreambleCode", TerrainScisPreambleCode},
    {"TerrainScissorCode", TerrainScissorCode},
    {"TerrainShadowHalfResCode", TerrainShadowHalfResCode},
    {"TerrainShadowScisCode", TerrainShadowScisCode},
    {"Triangle2DRendererCode", Triangle2DRendererCode},
    {"Vu0MiscCode", Vu0MiscCode},
    {"Vu0TransformNoLightNoClipCode", Vu0TransformNoLightNoClipCode},
    {"WaterObjectCode", WaterObjectCode},
    {"WaterScreenSpaceRendererCode", WaterScreenSpaceRendererCode},
};

// The microprogram whose upload chain starts at p, or nullptr.
const char *port_microprogram_name(const void *p)
{
    for (const auto &prog : kPrograms)
        if (prog.code == p)
            return prog.name;
    return nullptr;
}

// The microprogram whose upload data contains p (the nearest symbol at or
// below it in the packet region), or nullptr.
const char *port_microprogram_containing(const void *p)
{
    if (p < port_vupackets_start || p >= port_vupackets_end)
        return nullptr;
    const char *best = nullptr;
    const char *bestAddr = nullptr;
    for (const auto &prog : kPrograms)
        if (prog.code <= p && prog.code > bestAddr) {
            best = prog.name;
            bestAddr = prog.code;
        }
    return best;
}
