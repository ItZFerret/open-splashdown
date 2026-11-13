// PSXWaterDisplacement.cpp  @ 0x002E6630 (text 0x2E6630-0x2ED780)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's PSXWaterDisplacement.cpp as one translation unit: its functions in
// retail order, then its static initializer. `make link` places the object's
// data as well as its code (tools/link_c.py). It also holds the free functions
// getFrameBuffer, textureTransferFromVideoMemoryCompleteHandler and
// getTextureFromVideoMemory. The old files' views map onto the shared headers:
// the water's "views" are Water::m_wakePatches (WaterWakePatch: m_scrollScale,
// m_position, m_texture), its strip count and height m_numTiles/m_gridSize,
// the camera's viewport fields Camera::m_viewport, the "froth" records the
// WakePoints, +0x6F0 m_viewIndex, +0x708/+0x70C the per-view strip/tile
// packets.



#pragma inline_depth(6)

#include "sceGifTag/sceGifTag.h"
#include "d3dtypes.h"
#include "QWORD/QWORD.h"
#include "DmaRenderItem/DmaRenderItem.h"
#include "DmaPacket/DmaPacket.h"
#include "GameObject/GameObject.h"
#include "PSXTextureMap/PSXTextureMap.h"
#include "PSXVideoCard/PSXVideoCard.h"
#include "Water/Water.h"
#include "WaterWakePatch/WaterWakePatch.h"
#include "PSXWater/PSXWater.h"
#include "PSXWaterDisplacement/PSXWaterDisplacement.h"
#include "_sceDmaTag/_sceDmaTag.h"
#include "soultree_assert_block.h"
#include "Camera/Camera.h"
#include "PSXCamera/PSXCamera.h"
#include "RenderTarget/RenderTarget.h"
#include "PSXRenderTarget/PSXRenderTarget.h"
#include "Game/Game.h"
#include "_RECT/_RECT.h"
#include "PSXTextureMapManager/PSXTextureMapManager.h"
#include "PSXSignatureWave/PSXSignatureWave.h"
#include "SoultreeObject/SoultreeObject.h"
#include "WaterObject/WaterObject.h"
#include "PSXRenderStateBlock/PSXRenderStateBlock.h"
// The axis vectors every Soultree TU defines (and __sinit constructs).
#include "sinit/AxisVectors.h"
#include "sinit/D3DIntVector.h"
#include "HiResMeter/HiResMeter.h"

typedef unsigned long u_long;   // 64-bit under mwccps2
typedef unsigned int u_int;
typedef unsigned char u_char;

#define UNCACHED(p) (((unsigned int)(p) & 0x0FFFFFFF) | EE_UNCACHED)
#define UNCACHED_ACCEL(p) ((unsigned int)(p) | EE_UNCACHED_ACCEL)
#define SCE_GS_SET_XYZ(x, y, z) ((u_long)(x) | ((u_long)(y) << 16) | ((u_long)(z) << 32))
#define SCE_GS_SET_XYOFFSET(ofx, ofy) ((u_long)(ofx) | ((u_long)(ofy) << 32))
#define SCE_GS_SET_SCISSOR(scax0, scax1, scay0, scay1) \
    ((u_long)(scax0) | ((u_long)(scax1) << 16) | ((u_long)(scay0) << 32) | ((u_long)(scay1) << 48))
#define SCE_GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
    ((u_long)(sbp) | ((u_long)(sbw) << 16) | ((u_long)(spsm) << 24) | \
     ((u_long)(dbp) << 32) | ((u_long)(dbw) << 48) | ((u_long)(dpsm) << 56))
#define SCE_GS_SET_TRXPOS(ssax, ssay, dsax, dsay, dir) \
    ((u_long)(ssax) | ((u_long)(ssay) << 16) | ((u_long)(dsax) << 32) | \
     ((u_long)(dsay) << 48) | ((u_long)(dir) << 59))
#define SCE_GS_SET_TRXREG(rrw, rrh) ((u_long)(rrw) | ((u_long)(rrh) << 32))
#define SCE_GS_SET_ZBUF(zbp, psm, zmsk) ((u_long)(zbp) | ((u_long)(psm) << 24) | ((u_long)(zmsk) << 32))

#define SCE_GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    ((u_long)(tbp) | ((u_long)(tbw) << 14) | ((u_long)(psm) << 20) | ((u_long)(tw) << 26) | \
     ((u_long)(th) << 30) | ((u_long)(tcc) << 34) | ((u_long)(tfx) << 35) | ((u_long)(cbp) << 37) | \
     ((u_long)(cpsm) << 51) | ((u_long)(csm) << 55) | ((u_long)(csa) << 56) | ((u_long)(cld) << 61))
#define GS_BUSDIR (*(volatile u_long *)0x12001040)
#define VIF1_STAT (*(volatile unsigned int *)0x10003C00)
#define SCE_GS_BITBLTBUF 0x50
#define SCE_GS_TRXPOS 0x51
#define SCE_GS_TRXREG 0x52
#define SCE_GS_TRXDIR 0x53
#define SCE_GS_FINISH 0x61


#define GS_CSR (*(volatile u_long *)0x12001000)
#define D1_CHCR (*(volatile unsigned int *)0x10009000)
#define D1_MADR (*(volatile unsigned int *)0x10009010)
#define D1_QWC (*(volatile unsigned int *)0x10009020)


extern "C" void free(void *p);
extern "C" void *memalign(unsigned int alignment, unsigned int size);
extern "C" void *memset(void *dst, int c, unsigned int n);
extern "C" int sprintf(char *dst, const char *fmt, ...);
extern "C" double fmod(double x, double y);
void PSXprintf(const char *fmt, ...);
float FastTan(float);

class Palette8;
BaseObject *GetTextureMapFromResourceName(char *name, int format, Palette8 *palette, unsigned int flags,
                                          _D3DBLEND srcBlend, _D3DBLEND destBlend, int colorKeyMode,
                                          unsigned int colorKey, int addRef);
int BytesPerPixel(int format);
int SyncDmaPath(int a, int b, int c);
int textureTransferFromVideoMemoryCompleteHandler(int cause);
void getTextureFromVideoMemory(unsigned char *dst, int address, int width, int x, int y, int w, int h, int a, int b);
int getFrameBuffer();

extern Game *LpGame;
extern RenderTarget *LpRenderTarget;
extern PSXVideoCard *LpVideoCard;
extern PSXTextureMapManager *LpTextureMapManager;
extern DmaPacket *LpGifPacket;
extern PSXWater *GameWater;
extern Camera *gpCamera;
extern sceGifTag gGifTag;
extern _D3DVECTOR gVectorZero;
extern _D3DVECTOR gLastVehiclePosition[];
extern _D3DVECTOR gwp;
extern _D3DVECTOR gwn;
extern _D3DVECTOR gNearClipTopLeft;
extern _D3DINTVECTOR colorRed;
extern _D3DINTVECTOR colorBlueGreen;
extern _D3DINTVECTOR colorWhite;

// Four z values, also read as one quadword.
union ZQuad {
    int z[4];
    int128_t q;
};

// The static initializer's data, in retail's .bss order (ahead of the
// functions' statics): the displacement vectors and the debug colours; the
// "getFbTotal" meter ID is defined before getFrameBuffer.
_D3DVECTOR gNearClipTopLeft;
_D3DVECTOR gwp(0.0f, 0.0f, 0.0f);
_D3DVECTOR gwn(0.0f, 1.0f, 0.0f);
_D3DVECTOR gVectorOne(1.0f, 1.0f, 1.0f, 1.0f);
_D3DVECTOR gVectorZero(0.0f, 0.0f, 0.0f, 0.0f);

// Defined here for its retail .bss slot.
_D3DVIEWPORT8 gViewport[4];

_D3DVECTOR gLastVehiclePosition[2];

// Defined here for its retail .bss slot.
sceGifTag gGifTag;
int gCameraUnderWater;
int giZbufAddress;
int giViewportWidth;
unsigned char *gpWakeFrothMaskMap;
unsigned char *gWaterDmaData;
int gReformattedViewportsXOffset;
ZQuad gZValueAtBottomOfScreen;


_D3DINTVECTOR colorBlack(0, 0, 0, 0x40);
_D3DINTVECTOR colorRed(0x7F, 0, 0, 0x40);
_D3DINTVECTOR colorGreen(0, 0x7F, 0, 0x40);
_D3DINTVECTOR colorBlue(0, 0, 0x7F, 0x40);
_D3DINTVECTOR colorGrey(0x7F, 0x7F, 0x7F, 0x40);
_D3DINTVECTOR colorWhite(0x7F, 0x7F, 0x7F, 0x40);  // same as colorGrey in retail
_D3DINTVECTOR colorBlueGreen(0, 0x7F, 0x7F, 0x40);

// Defined here for its retail .bss slot.
Camera *gpCamera;

// PSXRenderStateBlock::SetTextureMap's TEX0 setup, written into a packet.
// An inline with external linkage (retail's statics are
// allowThisAssertion$localstatic3$SetTextureMap__FP13PSXTextureMapPUl ...),
// defined between gpCamera and LpGifPacket: its statics sit there in retail.
inline void SetTextureMap(PSXTextureMap *texture, u_long *tex0)
{
    PSXMipSurface *mip;
    u_char psm;
    short cbp;
    u_char cpsm;
    u_char csm;
    u_char csa;
    u_char cld;
#line 277 "PSXWaterDisplacement.cpp"
    ASSERT(texture, "SetTextureMap: lpTextureMap == NULL");
    LpTextureMapManager->AddTextureToGifList(texture);
    LpTextureMapManager->AddTextureInterruptToRenderQueue();
    mip = texture->GetReqdMipLevel();
    csm = 0;
    csa = 0;
    switch (texture->m_format) {
    case 0x22B8:
    case 0x115C:
        psm = 0;
        cbp = 0;
        cpsm = cld = 0;
        break;
    case 0x613:
    case 0x22B:
    case 0x235:
        psm = 2;
        cbp = 0;
        cpsm = cld = 0;
        break;
    case 8:
        psm = 0x13;
        cbp = *(short *)&texture->m_xF0;
        cpsm = 0;
        cld = 1;
        break;
    case 4:
        psm = 0x14;
        cbp = *(short *)&texture->m_xF0;
        cpsm = 0;
        cld = 1;
        break;
    default:
        PSXprintf("Invalid pixel format (%d) in PSXRenderStateBlock::SetTextureFunction()\n", texture->m_format);
#line 327 "PSXWaterDisplacement.cpp"
        ASSERTF(0, ("Invalid pixel format (%d) in PSXRenderStateBlock::SetTextureFunction()", texture->m_format));
    }
    *tex0 = SCE_GS_SET_TEX0(texture->m_xEC, mip->m_blocks, psm, mip->m_widthLog2, mip->m_heightLog2, 1, 0, cbp, cpsm, csm, csa, cld);
}

DmaPacket *LpGifPacket;


extern char WaterContentDirectory[];
extern int giZbufAddress;
extern int giViewportWidth;
extern _D3DVIEWPORT8 gViewport[];
extern int gReformattedViewportsXOffset;
extern int gComputeWake;
extern unsigned char *gpWakeFrothMaskMap;
extern unsigned char *gWaterDmaData;
extern int giReducedScissor;
extern volatile int LockRenderQueueState;
extern int giTemp;
extern int gClearFrameBuffer;
extern const int gZValue;
extern int gRenderWater;
extern int TimeGPU;
extern float gGuardBandAdditionalFov;
extern int gbRenderInfinitePlane;
extern int gRenderWaterObjects;
extern int gRenderFroth;
extern int gCameraUnderWater;
extern float gfMidDistance;
extern float gfBias;
extern float gfMinForce;
extern float gfBaseLineScale;
extern float gfStartScale;
extern float gfTotalTime;

// A wake froth point (0x14 bytes; m_wakePoints, 50 of them).
struct WakePoint {
    WakePoint();
    float m_x;                          // +0x00
    float m_z;                          // +0x04
    float m_scale;                      // +0x08
    float m_x0C;                        // +0x0C
    float m_time;                       // +0x10
};


// BuildRenderItem's screen position quadword (12.4 fixed point).
struct QWORDI {
    int x, y, z, w;
};

static inline void SetDmaTag(_sceDmaTag *tag, int qwc, int id, _sceDmaTag *next)
{
    tag->qwc = qwc;
    tag->mark = 0;
    tag->id = id;
    tag->next = next;
    tag->p[0] = 0;
    tag->p[1] = 0;
}


extern _D3DVECTOR gwp;
extern char WaterContentDirectory[];
extern int giZbufAddress;
extern int giViewportWidth;
extern _D3DVIEWPORT8 gViewport[];
extern int gReformattedViewportsXOffset;
extern int gComputeWake;
extern unsigned char *gpWakeFrothMaskMap;
extern ZQuad gZValueAtBottomOfScreen;

// One A+D register write: the data, then the GS register address.
struct GsAD {
    u_long data;
    u_long addr;
};

// m_packets: the per-pass wake render state (0xB0 bytes).
struct WakeStatePacket {
    u_int vif[4];                       // +0x00 FLUSH, BASE, OFFSET, UNPACK
    sceGifTag tag;                      // +0x10
    GsAD tex0;                          // +0x20 (data set per pass)
    GsAD texflush;                      // +0x30
    GsAD tex1;                          // +0x40
    GsAD texa;                          // +0x50
    GsAD test;                          // +0x60
    GsAD zbuf;                          // +0x70
    GsAD clamp;                         // +0x80
    GsAD alpha;                         // +0x90
    u_int vifEnd[4];                    // +0xA0 MSCAL, FLUSHE?
};

// m_viewportVifs: XYOFFSET/SCISSOR packets (0x50 bytes).
struct ViewportVifPacket {
    u_int vif[4];                       // +0x00
    sceGifTag tag;                      // +0x10
    GsAD xyoffset;                      // +0x20
    GsAD scissor;                       // +0x30
    u_int vifEnd[4];                    // +0x40
};

// m_x700: strip packets (0xF0 bytes).
struct StripPacket {
    u_int vif[4];                       // +0x00
    sceGifTag tag;                      // +0x10
    char pad20[0xE0 - 0x20];
    u_int vifEnd[4];                    // +0xE0
};

// m_vifCodes: the z clear and fill sprites (0x150 bytes).
struct ClearPacket {
    u_int vif[4];                       // +0x00
    sceGifTag tag;                      // +0x10
    GsAD test;                          // +0x20
    GsAD zbuf;                          // +0x30
    GsAD rgbaq;                         // +0x40
    GsAD xyz0;                          // +0x50
    GsAD xyz1;                          // +0x60
    sceGifTag spriteTag;                // +0x70
    struct {
        GsAD rgbaq;
        GsAD xyz0;
        GsAD xyz1;
    } sprites[4];                       // +0x80
    u_int vifEnd[4];                    // +0x140
};

// m_tilePackets[player]: per tile, local-to-local copies of the z buffer (0x180 bytes).
struct TileCopyPacket {
    u_int vif[4];                       // +0x00
    sceGifTag tag1;                     // +0x10
    GsAD bitbltbuf1;                    // +0x20
    GsAD trxreg1;                       // +0x30
    GsAD trxpos1;                       // +0x40
    GsAD trxdir1;                       // +0x50
    GsAD trxpos2;                       // +0x60
    GsAD trxdir2;                       // +0x70
    GsAD trxpos3;                       // +0x80
    GsAD trxdir3;                       // +0x90
    sceGifTag tag2;                     // +0xA0
    GsAD trxreg2;                       // +0xB0
    GsAD trxpos4;                       // +0xC0
    GsAD trxdir4;                       // +0xD0
    GsAD trxpos5;                       // +0xE0
    GsAD trxdir5;                       // +0xF0
    GsAD trxpos6;                       // +0x100
    GsAD trxdir6;                       // +0x110
    sceGifTag tag3;                     // +0x120
    GsAD bitbltbuf2;                    // +0x130
    GsAD trxreg3;                       // +0x140
    GsAD trxpos7;                       // +0x150
    GsAD trxdir7;                       // +0x160
    u_int vifEnd[4];                    // +0x170
};

// A GS register packet (5 quadwords) setting XYOFFSET_1 and SCISSOR_1.
struct SetViewportVif {
    char pad0[0x20];
    u_long m_xyoffset;                  // +0x20
    u_long m_xyoffsetReg;               // +0x28
    u_long m_scissor;                   // +0x30
    u_long m_scissorReg;                // +0x38
    char pad40[0x50 - 0x40];
};

PSXWaterDisplacement::PSXWaterDisplacement()
{
    if (!LpGifPacket)
        LpGifPacket = new DmaPacket(0x40, 1);
    m_time = 0;
    *(int128_t *)&gGifTag = 0;
    gGifTag.EOP = 1;
    gGifTag.PRE = 1;
    gGifTag.FLG = 0;
    gGifTag.NREG = 3;
    gGifTag.REGS0 = 2;
    gGifTag.REGS1 = 1;
    gGifTag.REGS2 = 4;
    gGifTag.NLOOP = 4;
    gGifTag.PRIM = 0x5C;
    for (int i = 0; i < 4; i++)
        m_x338[i] = 0;
    m_signatureWaves = 0;
    m_x320 = 0;
    m_texture = 0;
    m_waterObjects = 0;
    m_vifCodes = 0;
    m_frac[0] = 0.0f;
    m_frac[1] = 0.0f;
    m_frac[2] = 0.0f;
    m_frac[3] = 0.0f;
    m_invFrac[0] = 1.0f;
    m_invFrac[1] = 1.0f;
    m_invFrac[2] = 1.0f;
    m_invFrac[3] = 1.0f;
    m_numFroth = 0;
    m_wakePoints = new WakePoint[50];
    m_wakeItems = new DmaRenderItem[50];
    m_x6D4 = 0;
    m_wakeTextures[0] = 0;
    m_wakeTextures[1] = 0;
    gLastVehiclePosition[0] = gLastVehiclePosition[1] = gVectorZero;
    m_tilePackets[1] = 0;
    m_tilePackets[0] = 0;
    m_viewIndex = -1;
    m_lastPlayer = 0;
}

WakePoint::WakePoint()
{
    m_x = m_z = m_scale = m_x0C = m_time = 0;
}

PSXWaterDisplacement::~PSXWaterDisplacement()
{
    if (m_x320) {
        delete[] m_x320;
        m_x320 = 0;
    }
    if (m_signatureWaves) {
        delete[] m_signatureWaves;
        m_signatureWaves = 0;
    }
    if (m_texture) {
        m_texture->Release();
        m_texture = 0;
    }
    if (m_frothTexture) {
        m_frothTexture->Release();
        m_frothTexture = 0;
    }
    delete[] m_wakePoints;
    m_wakePoints = 0;
    delete[] m_wakeItems;
    m_wakeItems = 0;
    for (unsigned int i = 0; i < 2; i++) {
        if (m_wakeTextures[i]) {
            m_wakeTextures[i]->Release();
            m_wakeTextures[i] = 0;
        }
    }
    m_waterObjects = 0;
    free(m_packets);
    free(m_viewportVifs);
    free(m_vifCodes);
    free(m_x700);
    if (m_tilePackets[0]) {
        free(m_tilePackets[0]);
        m_tilePackets[0] = 0;
    }
    if (m_tilePackets[1]) {
        free(m_tilePackets[1]);
        m_tilePackets[1] = 0;
    }
    if (LpGifPacket) {
        delete LpGifPacket;
        LpGifPacket = 0;
    }
}

PSXWaterDisplacement *PSXWaterDisplacement::Create(PSXWater *water, char *name, LightManager *lights, char *dir,
                                                   int players, int mode)
{
    // Declaration order matters for register colouring (found by search).
    unsigned int i;
    ClearPacket *c;
    sceGifTag *tag;
    u_int dst;
    TileCopyPacket *t;
    u_int xoff;
    u_int yoff;
    u_int player;
    u_int x3;
    u_int x;
    u_int tile;
    u_long zbp;
    *(LightManager **)pad5A4 = lights;  // +0x5A4 (the shared header has no member there yet)
    gwp.y = water->m_baseHeight;
    m_signatureWaves = new PSXSignatureWave *[water->m_numWaves + 1];
    m_x320 = new DmaRenderItem[water->m_numWaves * 4];
    m_waves = water->m_waves;
    m_waterObjects = (WaterObject **)water->m_objects;
    if (name && *name && water->m_enableBaseDisplacement)
        m_texture = (PSXTextureMap *)GetTextureMapFromResourceName(name, 0x22B8, 0, 0, (_D3DBLEND)5, (_D3DBLEND)6,
                                                                   0x80, 0xFF00FF, 1);
    sprintf(pad5CC, "%swakeAdditive.tga", WaterContentDirectory);    // +0x5CC: the froth texture's name
    m_frothTexture = (PSXTextureMap *)GetTextureMapFromResourceName(pad5CC, 0x22B8, 0, 0, (_D3DBLEND)5,
                                                                    (_D3DBLEND)6, 0x80, 0xFF00FF, 1);
    m_frothTexture->LockSys(0, 0, 0);              // results unused
    BytesPerPixel(m_frothTexture->m_format);
    giZbufAddress = LpVideoCard->GetZBufferAddress() << 5;
    giViewportWidth = GameWater->m_xE0;
    gViewport[0].X = 0;
    gViewport[1].X = giViewportWidth;
    gViewport[2].X = giViewportWidth * 2;
    gViewport[3].X = giViewportWidth * 3 + 0x10;
    gViewport[0].Y = gViewport[1].Y = gViewport[2].Y = 0;
    gViewport[3].Y = 1;
    gViewport[0].Width = gViewport[1].Width = gViewport[2].Width = (GameWater->m_numTiles << 4) + 1;
    gViewport[3].Width = 0x7E;
    gViewport[0].Height = gViewport[1].Height = gViewport[2].Height = GameWater->m_gridSize;
    gViewport[3].Height = 0x7E;
    gReformattedViewportsXOffset = GameWater->m_numTiles * 20 + 0x10;
    m_x6D4 = players;
    if (gComputeWake) {
        for (i = 0; i < m_x6D4; i++) {
            m_wakeTextures[i] = new PSXTextureMap(1);
            m_wakeTextures[i]->Load(0, 0x80, 0x80, 0x80, 0x80, 0x22B8, 0x22B8, 0, 0, 0, 1, 0, (_D3DBLEND)5,
                                    (_D3DBLEND)6, 0, 0xFF00FF);
            gpWakeFrothMaskMap = (unsigned char *)m_wakeTextures[i]->LockSys(0, 0, 0);
            PSXTextureMap *w = m_wakeTextures[i];
            if (9.0f > w->m_usage)
                w->m_usage = 9.0f;
        }
    }

    m_packets = (WakePacket *)memalign(0x80, 4 * sizeof(WakeStatePacket));
    for (u_int k = 0; k < 4; k++) {
        WakeStatePacket *p = &((WakeStatePacket *)m_packets)[k];
        p->vif[0] = 0x11000000;
        p->vif[1] = 0x03000000;
        p->vif[2] = 0x02000000;
        p->vif[3] = 0x6C09C320;
        *(int128_t *)&p->tag = 0;
        p->tag.FLG = 0;
        p->tag.EOP = 1;
        p->tag.NREG = 8;
        p->tag.NLOOP = 1;
        p->tag.REGS0 = p->tag.REGS1 = p->tag.REGS2 = p->tag.REGS3 = p->tag.REGS4 = p->tag.REGS5 =
            p->tag.REGS6 = p->tag.REGS7 = 0xE;
        p->tex0.addr = 0x06;            // TEX0_1
        p->zbuf.addr = 0x4E;            // ZBUF_1
        p->zbuf.data = SCE_GS_SET_ZBUF(LpVideoCard->GetZBufferAddress(), 0x31, 0);
        p->texflush.addr = 0x3F;        // TEXFLUSH
        p->texflush.data = 0;
        p->clamp.addr = 0x08;           // CLAMP_1
        p->clamp.data = 5;
        p->test.addr = 0x47;            // TEST_1
        p->test.data = 0x3000C;
        p->tex1.addr = 0x14;            // TEX1_1
        p->tex1.data = 0xFFFFFFB500000060;
        p->texa.addr = 0x3B;            // TEXA
        p->texa.data = 0x7F8000;
        p->alpha.addr = 0x42;           // ALPHA_1
        p->alpha.data = 0x80000000A8;
        p->vifEnd[0] = 0x14000000;
        p->vifEnd[1] = 0x10000000;
        p->vifEnd[2] = p->vifEnd[3] = 0;
    }

    m_viewportVifs = (SetViewportVif *)memalign(0x80, 20 * sizeof(ViewportVifPacket));
    for (i = 0; i < 20; i++) {
        ViewportVifPacket *v = &((ViewportVifPacket *)m_viewportVifs)[i];
        v->vif[0] = 0x11000000;
        v->vif[1] = 0x03000000;
        v->vif[2] = 0x02000000;
        v->vif[3] = 0x6C03C320;
        *(int128_t *)&v->tag = 0;
        v->tag.FLG = 0;
        v->tag.EOP = 1;
        v->tag.NREG = 2;
        v->tag.NLOOP = 1;
        v->tag.REGS0 = v->tag.REGS1 = 0xE;
        v->xyoffset.addr = 0x18;        // XYOFFSET_1
        v->scissor.addr = 0x40;         // SCISSOR_1
        v->vifEnd[0] = 0x14000000;
        v->vifEnd[1] = 0x10000000;
        v->vifEnd[2] = v->vifEnd[3] = 0;
    }

    m_x700 = memalign(0x80, 2 * sizeof(StripPacket));
    for (i = 0; i < 2; i++) {
        StripPacket *s = &((StripPacket *)m_x700)[i];
        s->vif[0] = 0x11000000;
        s->vif[1] = 0x03000000;
        s->vif[2] = 0x02000000;
        s->vif[3] = 0x6C0DC320;
        *(int128_t *)&s->tag = 0;
        s->tag.EOP = 1;
        s->tag.PRE = 1;
        s->tag.FLG = 0;
        s->tag.NREG = 3;
        s->tag.REGS0 = 2;
        s->tag.REGS1 = 1;
        s->tag.REGS2 = 4;
        s->tag.NLOOP = 4;
        s->tag.PRIM = 0x5C;
        s->vifEnd[0] = 0x14000000;
        s->vifEnd[1] = 0x10000000;
        s->vifEnd[2] = s->vifEnd[3] = 0;
    }

    m_vifCodes = (_sceDmaTag *)memalign(0x80, sizeof(ClearPacket));
    c = (ClearPacket *)m_vifCodes;
    c->vif[0] = 0x11000000;
    c->vif[1] = 0x03000000;
    c->vif[2] = 0x02000000;
    c->vif[3] = 0x6C13C320;
    tag = &((ClearPacket *)m_vifCodes)->tag;
    memset(tag, 0, sizeof(sceGifTag));
    tag->EOP = 0;
    tag->PRE = 1;
    tag->FLG = 0;
    tag->NREG = 5;
    tag->REGS0 = tag->REGS1 = tag->REGS2 = tag->REGS3 = tag->REGS4 = 0xE;
    tag->NLOOP = 1;
    tag->PRIM = 6;
    c->test.addr = 0x47;                // TEST_1
    c->test.data = 0x30002;
    c->zbuf.addr = 0x4E;                // ZBUF_1
    c->zbuf.data = SCE_GS_SET_ZBUF(LpVideoCard->GetZBufferAddress(), 0x31, 0);
    c->rgbaq.addr = 1;                  // RGBAQ
    c->rgbaq.data = 0x3F80000000000000;
    c->xyz0.addr = c->xyz1.addr = 4;    // XYZ2
    tag = &((ClearPacket *)m_vifCodes)->spriteTag;
    memset(tag, 0, sizeof(sceGifTag));
    tag->EOP = 1;
    tag->PRE = 1;
    tag->FLG = 0;
    tag->NREG = 12;
    tag->REGS0 = tag->REGS1 = tag->REGS2 = tag->REGS3 = tag->REGS4 = tag->REGS5 = tag->REGS6 = tag->REGS7 =
        tag->REGS8 = tag->REGS9 = tag->REGS10 = tag->REGS11 = 0xE;
    tag->NLOOP = 1;
    tag->PRIM = 6;
    c->sprites[0].rgbaq.addr = 1;
    c->sprites[0].rgbaq.data = 0x3F8000006E3CC020;
    c->sprites[1].rgbaq.addr = 1;
    c->sprites[1].rgbaq.data = 0x3F8000006C05C05C;
    c->sprites[2].rgbaq.addr = 1;
    c->sprites[2].rgbaq.data = 0x3F80000017000000;
    c->sprites[3].rgbaq.addr = 1;
    c->sprites[3].rgbaq.data = 0x3F80000010000000;
    c->sprites[0].xyz0.addr = c->sprites[0].xyz1.addr = c->sprites[1].xyz0.addr = c->sprites[1].xyz1.addr =
        c->sprites[2].xyz0.addr = c->sprites[2].xyz1.addr = c->sprites[3].xyz0.addr = c->sprites[3].xyz1.addr = 4;
    c->vifEnd[0] = 0x14000000;
    c->vifEnd[1] = 0x10000000;
    c->vifEnd[2] = c->vifEnd[3] = 0;

    LpVideoCard->GetCurrentDisplayBufferAddress();  // results unused
    LpVideoCard->GetCurrentFrameBufferAddress();
    zbp = LpVideoCard->GetZBufferAddress() << 5;
    xoff = 0;
    yoff = 0;
    for (player = 0, x3 = xoff + giViewportWidth * 3; player < players;
         player++, x3 = xoff + giViewportWidth * 3) {
        m_tilePackets[player] = memalign(0x80, GameWater->m_numTiles * sizeof(TileCopyPacket));
        x = xoff;

        for (tile = 0; tile < GameWater->m_numTiles; tile++) {
            t = &((TileCopyPacket *)m_tilePackets[player])[tile];
            t->vif[0] = 0x11000000;
            t->vif[1] = 0x03000000;
            t->vif[2] = 0x02000000;
            t->vif[3] = 0x6C16C320;
            tag = &t->tag1;
            memset(tag, 0, sizeof(sceGifTag));
            tag->EOP = 0;
            tag->PRE = 0;
            tag->FLG = 0;
            tag->NREG = 8;
            tag->REGS0 = tag->REGS1 = tag->REGS2 = tag->REGS3 = tag->REGS4 = tag->REGS5 = tag->REGS6 =
                tag->REGS7 = 0xE;
            tag->NLOOP = 1;
            tag = &t->tag2;
            memset(tag, 0, sizeof(sceGifTag));
            tag->EOP = 0;
            tag->PRE = 0;
            tag->FLG = 0;
            tag->NREG = 7;
            tag->REGS0 = tag->REGS1 = tag->REGS2 = tag->REGS3 = tag->REGS4 = tag->REGS5 = tag->REGS6 = 0xE;
            tag->NLOOP = 1;
            tag = &t->tag3;
            memset(tag, 0, sizeof(sceGifTag));
            tag->EOP = 1;
            tag->PRE = 0;
            tag->FLG = 0;
            tag->NREG = 4;
            tag->REGS0 = tag->REGS1 = tag->REGS2 = tag->REGS3 = 0xE;
            tag->NLOOP = 1;
            t->bitbltbuf1.addr = t->bitbltbuf2.addr = 0x50;     // BITBLTBUF
            t->bitbltbuf2.data = SCE_GS_SET_BITBLTBUF(zbp, LpVideoCard->m_width >> 6, 0x30, zbp,
                                                      LpVideoCard->m_width >> 6, 0x30);
            t->trxpos1.addr = t->trxpos2.addr = t->trxpos3.addr = t->trxpos4.addr = t->trxpos5.addr =
                t->trxpos6.addr = t->trxpos7.addr = 0x51;       // TRXPOS
            t->trxreg1.addr = t->trxreg2.addr = t->trxreg3.addr = 0x52;     // TRXREG
            t->trxdir1.addr = t->trxdir2.addr = t->trxdir3.addr = t->trxdir4.addr = t->trxdir5.addr =
                t->trxdir6.addr = t->trxdir7.addr = 0x53;       // TRXDIR
            t->trxdir1.data = t->trxdir2.data = t->trxdir3.data = t->trxdir4.data = t->trxdir5.data =
                t->trxdir6.data = t->trxdir7.data = 2;          // local to local

            dst = xoff + gReformattedViewportsXOffset + tile * 0x5C;   // strength-reduced to a stepped register
            t->trxpos1.data = SCE_GS_SET_TRXPOS(x, yoff, dst + 4, yoff, 0);
            // This one TRXPOS is built in two steps in retail (its source half is
            // ORed together first, which decides the temporaries' registers).
            u_long src2 = (u_long)(x + giViewportWidth) | ((u_long)yoff << 16);
            t->trxpos2.data = src2 | ((u_long)(dst + 0x18) << 32) | ((u_long)yoff << 48);
            t->trxpos3.data = SCE_GS_SET_TRXPOS(x + giViewportWidth * 2, yoff, dst + 0x2C, yoff, 0);
            t->trxpos4.data = SCE_GS_SET_TRXPOS(x3, yoff, dst, yoff, 0);
            t->trxpos5.data = SCE_GS_SET_TRXPOS(x3 + 4, yoff, dst + 0x40, yoff, 0);
            t->trxpos6.data = SCE_GS_SET_TRXPOS(x3 + 8, yoff, dst + 0x58, yoff, 0);
            t->trxpos7.data = SCE_GS_SET_TRXPOS(x, yoff, dst + 0x44, yoff, 0);
            t->trxreg1.data = t->trxreg3.data = SCE_GS_SET_TRXREG(0x14, GameWater->m_gridSize);
            t->trxreg2.data = SCE_GS_SET_TRXREG(4, GameWater->m_gridSize);
            t->vifEnd[0] = 0x14000000;
            t->vifEnd[1] = 0x10000000;
            t->vifEnd[2] = t->vifEnd[3] = 0;
            x += 0x10;

        }
        switch (mode) {
        case 0:
            yoff = 0;
            xoff = 0;
            break;
        case 2:
            xoff = 0x140;
            yoff = 0;
            break;
        case 1:
            xoff = 0;
            yoff = 0xE0;
            break;
        }
    }

    memset(&m_gifTag, 0, sizeof(sceGifTag));
    m_gifTag.EOP = 1;
    m_gifTag.PRE = 1;
    m_gifTag.FLG = 0;
    m_gifTag.NREG = 3;
    m_gifTag.REGS0 = 2;
    m_gifTag.REGS1 = 1;
    m_gifTag.REGS2 = 4;
    m_gifTag.NLOOP = 2;
    m_gifTag.PRIM = 0x5E;
    gZValueAtBottomOfScreen.z[0] = gZValueAtBottomOfScreen.z[1] = gZValueAtBottomOfScreen.z[2] =
        gZValueAtBottomOfScreen.z[3] = 0xFFFFF;
    return this;
}

void PSXWaterDisplacement::RenderVifCodes()
{
    u_long *codes = (u_long *)UNCACHED_ACCEL(m_vifCodes);
    Camera *camera = LpRenderTarget->m_camera;
    int top = (2048 - camera->m_viewport.Height / 2) * 16;
    int bottom = top + GameWater->m_gridSize * 16;
    int x = (2048 - camera->m_viewport.Width / 2 + giViewportWidth * 3) * 16;
    codes[10] = SCE_GS_SET_XYZ(x, top, 0xFFFFFFF);
    codes[12] = SCE_GS_SET_XYZ(x + 0xD0, bottom, 0xFFFFFFF);
    codes[18] = SCE_GS_SET_XYZ(x + 0x30, top, 0xFFFFFFF);
    codes[20] = SCE_GS_SET_XYZ(x + 0x40, bottom, 0xFFFFFFF);
    codes[24] = SCE_GS_SET_XYZ(x + 0x70, top, 0xFFFFFFF);
    codes[26] = SCE_GS_SET_XYZ(x + 0x80, bottom, 0xFFFFFFF);
    codes[30] = SCE_GS_SET_XYZ(x + 0x80, top, 0xFFFFFFF);
    codes[32] = SCE_GS_SET_XYZ(x + 0x90, bottom, 0xFFFFFFF);
    codes[36] = SCE_GS_SET_XYZ(x + 0x90, top, 0xFFFFFFF);
    codes[38] = SCE_GS_SET_XYZ(x + 0xA0, bottom, 0xFFFFFFF);

    _sceDmaTag tag;
    SetDmaTag(&tag, 0x15, 0x30, m_vifCodes);
    ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&tag, 1, "");

    int frame = LpVideoCard->GetCurrentFrameBufferAddress();
    int zbuf = LpVideoCard->GetZBufferAddress();
    u_long bitbltbuf = SCE_GS_SET_BITBLTBUF(frame << 5, LpVideoCard->m_width >> 6, LpVideoCard->m_psm,
                                           zbuf << 5, LpVideoCard->m_width >> 6, 0x30);
    for (unsigned int i = 0; i < (unsigned int)GameWater->m_numTiles; i++) {
        _sceDmaTag stripTag;
        unsigned char *strip = (unsigned char *)m_tilePackets[m_viewIndex] + i * 0x180;
        *(u_long *)(strip + 0x20) = bitbltbuf;
        SetDmaTag(&stripTag, 0x18, 0x30, (_sceDmaTag *)strip);
        ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&stripTag, 1, "");
    }
}

#pragma fast_fptosi on

// Retail's fmin/fmax: min.s/max.s into $f8, stored through a stack temporary.
static inline float Fmin(float a, float b)
{
    float r;
    asm("min.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}

static inline float Fmax(float a, float b)
{
    float r;
    asm("max.s $f8, %1, %2; swc1 $f8, 0(%0)" : : "r"(&r), "f"(a), "f"(b) : "$f8", "memory");
    return r;
}

// libvu0 sceVu0ApplyMatrix: out = m * v (xyzw).  This copy has a nop before
// the store, and its operands listed (out, v, m) for retail's registers.
static inline void Vu0ApplyMatrix(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR *v)
{
    asm("lqc2 vf4, 0x0(%2); lqc2 vf5, 0x10(%2); lqc2 vf6, 0x20(%2); lqc2 vf7, 0x30(%2); lqc2 vf8, 0x0(%1); vmulax.xyzw ACC, vf4, vf8x; vmadday.xyzw ACC, vf5, vf8y; vmaddaz.xyzw ACC, vf6, vf8z; vmaddw.xyzw vf9, vf7, vf8w; nop; sqc2 vf9, 0x0(%0)"
        : : "r"(out), "r"(v), "r"(m) : "memory");
}

void PSXWaterDisplacement::BuildRenderItem(DmaRenderItem *item, float scale, _D3DINTVECTOR &color, float originX, float originZ)
{
    static int WaterDisplacementBuildRenderItemID = -1;
    PSXCamera *camera = (PSXCamera *)LpRenderTarget->m_camera;
    float invScale = 1.0f / scale;
    item->Start((DmaRenderDestination)1, 0x80);
    item->DmaAddVifCode(0x11000000);
    item->DmaAddVifCode(0x03000000);
    item->DmaAddVifCode(0x02000000);
    item->DmaBeginUnpack(0x320, 12);
    item->DmaAddGifTagToUnpack(&gGifTag);
    _D3DVECTOR screen;
    QWORDI xyz;
    _D3DVECTOR st(0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; i++) {
        _D3DVECTOR *corner = &m_corners[i];
        Vu0ApplyMatrix(&screen, &camera->m_x250, corner);
        screen.z = Fmax(Fmin(screen.z, 16776960.0f), 0.0f);
        screen.w = Fmax(screen.w, 1.0f);
        float q = 1.0f / screen.w;
        float s = 16.0f * q;
        xyz.x = (int)(screen.x * s) - 16;
        xyz.y = (int)(screen.y * s);
        xyz.z = (int)(screen.z * s);
        xyz.w = 0;
        float t = q * invScale;
        st.x = t * (corner->x - originX);
        st.y = t * (corner->z - originZ);
        st.z = q;
        item->DmaAddVectorToUnpack(&st);
        item->DmaAddIntVectorToUnpack(&color);
        item->DmaAddQwordToUnpack((QWORD *)&xyz);
    }
    item->DmaEndUnpack();
    item->DmaAddVifCode(0x14000000);
    item->DmaAddVifCode(0x10000000);
    item->End();
    ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(item, "");
}

#pragma fast_fptosi reset

void PSXWaterDisplacement::SetViewport(int index)
{
    if (index == m_currentViewport)
        return;
    // Profiler ID, initialized but never read.
    static int SetViewportID = -1;
    m_currentViewport = index;
#line 1223 "PSXWaterDisplacement.cpp"
    ASSERTF(miViewportIdx < 20, ("SetViewport: miViewportIdx (%d) >= MAX_VIEWPORT_RENDER_ITEMS", miViewportIdx));
    SetViewport(&gViewport[index], &m_viewportVifs[miViewportIdx++]);
}

// Points the camera at the viewport (offset by the saved viewport), then
// queues a GS packet with the matching XYOFFSET and SCISSOR.
void PSXWaterDisplacement::SetViewport(_D3DVIEWPORT8 *viewport, SetViewportVif *vif)
{
    PSXCamera *camera = (PSXCamera *)LpRenderTarget->m_camera;
    float yScale = viewport->Height / (camera->m_bottom - camera->m_top);
    float xScale = viewport->Width / (camera->m_right - camera->m_left);
    camera->m_x338 = xScale;
    camera->m_x33C = yScale;
    camera->m_viewport.X = viewport->X + m_savedViewport.X;
    camera->m_viewport.Y = viewport->Y + m_savedViewport.Y;
    camera->m_viewport.Width = viewport->Width;
    camera->m_viewport.Height = viewport->Height;
    camera->UpdateMatrices();
    int ofx = (0x800 - (viewport->Width >> 1) - (viewport->X + m_savedViewport.X)) << 4;
    int ofy = (0x800 - (viewport->Height >> 1) - (viewport->Y + m_savedViewport.Y)) << 4;
    vif->m_xyoffset = SCE_GS_SET_XYOFFSET(ofx, ofy);
    int x0 = viewport->X + m_savedViewport.X;
    int y0 = viewport->Y + m_savedViewport.Y;
    int x1 = x0 + viewport->Width - 1;
    int y1 = y0 + viewport->Height - 1 - giReducedScissor;
    vif->m_scissor = SCE_GS_SET_SCISSOR(x0, x1, y0, y1);
    _sceDmaTag tag;
    tag.qwc = 5;
    tag.mark = 0;
    tag.id = 0x30;                      // REF
    tag.next = (_sceDmaTag *)vif;
    tag.p[0] = 0;
    tag.p[1] = 0;
    ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&tag, 1, "");
}


void PSXWaterDisplacement::RestoreViewport()
{
    // Profiler ID, initialized but never read.
    static int RestoreViewportID = -1;
    _D3DVIEWPORT8 viewport;
    // (Retail's message names SetViewport here too.)
#line 1285 "PSXWaterDisplacement.cpp"
    ASSERTF(miViewportIdx < 20, ("SetViewport: miViewportIdx (%d) >= MAX_VIEWPORT_RENDER_ITEMS", miViewportIdx));
    LpRenderTarget->m_camera->m_fov = m_savedFov;
    viewport.X = m_savedViewport.X;
    viewport.Y = m_savedViewport.Y;
    viewport.Width = m_savedViewport.Width;
    viewport.Height = m_savedViewport.Height;
    m_savedViewport.X = m_savedViewport.Y = 0;
    LpRenderTarget->m_camera->ResetClipPlanes();
    int reducedScissor = giReducedScissor;
    giReducedScissor = 0;
    SetViewport(&viewport, &m_viewportVifs[miViewportIdx++]);
    giReducedScissor = reducedScissor;
}


// Defined here for its retail .bss slot.
volatile int LockRenderQueueState;

// Set by the static initializer (after the vectors and colours); retail's
// .bss has it between RestoreViewport's statics and getFrameBuffer's.
static int getFrameBufferTotalID = LpHiResMeter->GetMeterID("getFbTotal");

// Starts a readback of the frame buffer (into the wake froth mask, or the
// water's z data); while one is in flight, polls for its completion.
int getFrameBuffer()
{
    static int getFrameBufferID = -1;   // unused profiler ID
    short address = LpVideoCard->GetCurrentFrameBufferAddress() << 5;
    if (LockRenderQueueState) {
        if (SyncDmaPath(1, 0, 0))
            return LockRenderQueueState;
        // Through a local: a direct `return handler(0);` chains the branch
        // above through this path's `b epilogue`.
        int r = textureTransferFromVideoMemoryCompleteHandler(0);
        return r;
    }
    LockRenderQueueState = 1;
    if (gComputeWake && !LpGame->m_paused) {
        getTextureFromVideoMemory((unsigned char *)UNCACHED(gpWakeFrothMaskMap), address, 0x280, 0x80, 0x80,
                                  (int)gpCamera->m_viewport.X + (int)gViewport[3].X - 1, gpCamera->m_viewport.Y, 0, 4);
        return LockRenderQueueState;
    }
    getTextureFromVideoMemory((unsigned char *)UNCACHED(gWaterDmaData), giZbufAddress, 0x280, 0x5C,
                              (GameWater->m_gridSize + 3) / 4 * 4, (int)gpCamera->m_viewport.X + gReformattedViewportsXOffset,
                              gpCamera->m_viewport.Y, 0x30, 4);
    LockRenderQueueState = 2;
    return LockRenderQueueState;
}


// Each water row is 0x170 bytes (23 qwords) of DMA data.
#define WATER_ROW_SIZE 0x170
struct WaterRow {
    unsigned char data[WATER_ROW_SIZE];
};

// Z-buffer readback completion: start the next strip, or, after the last
// one, patch the bottom rows of every strip and resume rendering.
int textureTransferFromVideoMemoryCompleteHandler(int)
{
    unsigned int i;
    int j;
    unsigned char *row;

    VIF1_STAT = 0;
    GS_BUSDIR = 0;
    LpVideoCard->GetCurrentFrameBufferAddress();
    int strip = LockRenderQueueState - 1;
    unsigned int qwords = (GameWater->m_gridSize * sizeof(WaterRow) / 4 + 3) / 4;
    if (strip >= (unsigned int)GameWater->m_numTiles) {
        LockRenderQueueState = 0;
        gZValueAtBottomOfScreen.z[0] = gZValueAtBottomOfScreen.z[1] = gZValueAtBottomOfScreen.z[2] =
            gZValueAtBottomOfScreen.z[3] = gZValue;
        for (i = 0; i < GameWater->m_numTiles; i++) {
            for (j = 1; j <= giTemp; j++) {
                row = (unsigned char *)UNCACHED_ACCEL(gWaterDmaData + ((i + 1) * GameWater->m_gridSize - j) * WATER_ROW_SIZE);
                memset(row + 0x10, 0, 0xF0);
                *(int128_t *)(row + 0x110) = *(int128_t *)(row + 0x120) = *(int128_t *)(row + 0x130) =
                    *(int128_t *)(row + 0x140) = *(int128_t *)(row + 0x150) = gZValueAtBottomOfScreen.q;
            }
        }
        if (gClearFrameBuffer) {
            _RECT rect;
            rect.top = gpCamera->m_viewport.Y;
            rect.bottom = gpCamera->m_viewport.Y + gpCamera->m_viewport.Height;
            rect.left = gpCamera->m_viewport.X;
            rect.right = gpCamera->m_viewport.X + gpCamera->m_viewport.Width;
            LpVideoCard->ClearFrameBuffer(&rect, (PSXCamera *)gpCamera, LpRenderTarget->m_clearColor, 3);
        }
        ((PSXRenderTarget *)LpRenderTarget)->LockTrySendRenderQueue(0);
    } else {
        getTextureFromVideoMemory((unsigned char *)UNCACHED(gWaterDmaData + strip * qwords * 16), giZbufAddress, 0x280,
                                  0x5C, (GameWater->m_gridSize + 3) / 4 * 4,
                                  (int)gpCamera->m_viewport.X + gReformattedViewportsXOffset + strip * 0x5C, gpCamera->m_viewport.Y,
                                  0x30, 4);
        LockRenderQueueState++;     // (volatile: retail reads it twice)
    }
    return LockRenderQueueState;
}


void PSXWaterDisplacement::RenderWaterObjects()
{
    // Profiler ID, initialized but never read.
    static int WaterDisplacementRenderWaterObjectsID = -1;
    WaterObject **object;
    _D3DVECTOR scale(1.0f, 1.0f, 1.0f, 1.0f);
    _D3DVECTOR offset(0.0f, 0.0f, 0.0f, 0.0f);
    SetViewport(0);
    WaterWakePatch *view = &GameWater->m_wakePatches[m_viewIndex];
    float invSize = 1.0f / (128.0f * view->m_scrollScale);
    scale.x = 256.0f * invSize;
    scale.y = -scale.x;
    offset.x = -view->m_position.x;
    offset.y = -view->m_position.z;
    object = m_waterObjects;
    while (*object)
        (*object++)->Render3D(5, &colorRed, view->m_texture, &scale, &offset, invSize, invSize, 0, 0, 1.0f,
                            &m_corners[1]);
    if (gComputeWake) {
        SetViewport(2);
        scale.x = 2.0f;
        scale.y = -scale.x;
        offset.x = -(gLastVehiclePosition[m_viewIndex].x - 65.0f);
        offset.y = -(gLastVehiclePosition[m_viewIndex].z - 65.0f);
        object = m_waterObjects;
        while (*object)
            (*object++)->Render3D(5, &colorBlueGreen, m_wakeTextures[m_viewIndex], &scale, &offset, 1.0f / 128.0f,
                                1.0f / 128.0f, 0, 0, 1.0f, &m_corners[1]);
    }
    m_x374 = 0;
    m_x378 = 1;
    m_x37C = 0;
    int i = 0;
    while (m_signatureWaves[i])
        m_signatureWaves[i++]->RenderWaterObjects(this);
}

#pragma fast_fptosi on


int PSXWaterDisplacement::Render3D()
{
    int visible;
    int i;
    PSXTextureMap *texture;
    gpCamera = LpRenderTarget->m_camera;
    if (LpGame->m_currentPlayer <= m_lastPlayer)
        m_viewIndex = 0;
    else
        m_viewIndex++;
    m_lastPlayer = LpGame->m_currentPlayer;
    LockRenderQueueState = 0;
    if (!gRenderWater)
        return GameObject::Render3D();
    if (TimeGPU) {
        LpRenderTarget->TrySendRenderQueue(1);
        LpRenderTarget->WaitForRenderingToComplete();
    }
    static int CPUTimeID = -1;                      // unused profiler IDs
    static int WaterDisplacementOverheadID = -1;
    m_currentViewport = -1;
    miViewportIdx = 0;
    Camera *camera = LpRenderTarget->m_camera;
    m_savedMatrix = camera->m_x140;
    m_savedFov = camera->m_fov;
    m_savedViewport.X = camera->m_viewport.X;
    m_savedViewport.Y = camera->m_viewport.Y;
    m_savedViewport.Width = camera->m_viewport.Width;
    m_savedViewport.Height = camera->m_viewport.Height;
    camera->SetClipPlanes(1.0f, m_params->m_farClip);
    camera->m_fov = m_savedFov + gGuardBandAdditionalFov;
    camera->UpdateMatrices();
    gwp.y = GameWater->m_baseHeight;
    if (FrustumIntersectPlane(&gwp, &gwn)) {
        visible = 1;
        m_corners[0] = m_x430[m_crossings[0]];
        m_corners[1] = m_x430[m_crossings[3]];
        m_corners[2] = m_x430[m_crossings[1]];
        m_corners[3] = m_x430[m_crossings[2]];
    } else {
        visible = 0;
    }
    ClearViewport();
    m_x6D8 = 0;
    m_packetIndex = 0;
    if (gComputeWake) {
        texture = m_wakeTextures[m_viewIndex];
        gpWakeFrothMaskMap = (unsigned char *)texture->LockSys(0, 0, 0);
        RenderWake();
    }
    if (visible && gbRenderInfinitePlane) {
        SetViewport(0);
        WaterWakePatch *view = &GameWater->m_wakePatches[m_viewIndex];
        texture = view->m_texture;
        SetTextureMap(texture, &m_packets[m_packetIndex].m_tex0);
        _sceDmaTag tag;
        SetDmaTag(&tag, 0xB, 0x30, (_sceDmaTag *)&m_packets[m_packetIndex]);
        ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&tag, 1, "");
        m_packetIndex++;
        BuildRenderItem(&m_x80, 128.0f * view->m_scrollScale, colorRed, view->m_position.x, view->m_position.z);
        if (gComputeWake) {
            SetViewport(2);
            texture = m_wakeTextures[m_viewIndex];
            SetTextureMap(texture, &m_packets[m_packetIndex].m_tex0);
            _sceDmaTag tag2;
            SetDmaTag(&tag2, 0xB, 0x30, (_sceDmaTag *)&m_packets[m_packetIndex]);
            ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&tag2, 1, "");
            float originX = gLastVehiclePosition[m_viewIndex].x - 64.0f;
            BuildRenderItem(&m_xB0, 128.0f, colorBlueGreen, originX, gLastVehiclePosition[m_viewIndex].z - 64.0f);
        }
        PSXSignatureWave *wave;
        for (i = 0; (wave = m_signatureWaves[i]) != 0; i++)
            wave->BuildRenderItem(this);
    }
    if (gRenderWaterObjects && m_waterObjects)
        RenderWaterObjects();
    RestoreViewport();
    RenderVifCodes();
    if (TimeGPU) {
        static int GPUTimeID = -1;
        LpRenderTarget->TrySendRenderQueue(1);
        LpRenderTarget->WaitForRenderingToComplete();
        ((PSXRenderTarget *)LpRenderTarget)->LockTrySendRenderQueue(getFrameBuffer);
        LpRenderTarget->TrySendRenderQueue(0);
    } else {
        static int StallTimeID = -1;
        LpRenderTarget->TrySendRenderQueue(1);
        ((PSXRenderTarget *)LpRenderTarget)->LockTrySendRenderQueue(getFrameBuffer);
        LpRenderTarget->TrySendRenderQueue(0);
    }
    return GameObject::Render3D();
}

#pragma fast_fptosi reset

// libvu0 sceVu0OuterProduct: a x b.
static inline _D3DVECTOR CrossProduct(_D3DVECTOR &a, _D3DVECTOR &b)
{
    _D3DVECTOR out;
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x0(%2); vopmula.xyz ACC, vf4, vf5; vopmsub.xyz vf6, vf5, vf4; vsub.w vf6, vf6, vf6; sqc2 vf6, 0x0(%0)"
        : : "r"(&out), "r"(&a), "r"(&b) : "memory");
    return out;
}

static inline _D3DVECTOR Normalize(const _D3DVECTOR &v)
{
    _D3DVECTOR out;
    asm("lqc2 vf4, 0x0(%1); vmul.xyz vf5, vf4, vf4; vaddy.x vf5, vf5, vf5y; vaddz.x vf5, vf5, vf5z; vrsqrt Q, vf0w, vf5x; vsub.xyzw vf6, vf0, vf0; vaddw.xyzw vf6, vf6, vf4w; vwaitq; vmulq.xyz vf6, vf4, Q; sqc2 vf6, 0x0(%0)"
        : : "r"(&out), "r"(&v) : "memory");
    return out;
}

static inline float DotProduct(const _D3DVECTOR &a, const _D3DVECTOR &b)
{
    float r;
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x0(%2); vmul.xyz vf5, vf4, vf5; vaddy.x vf5, vf5, vf5y; vaddz.x vf5, vf5, vf5z; qmfc2.ni $2, vf5; mtc1 $2, %0"
        : "=f"(r) : "r"(&a), "r"(&b) : "$2");
    return r;
}

// Where the segment a..b crosses the plane through point with normal; 1 if it does.
static inline int IntersectSegment(_D3DVECTOR a, _D3DVECTOR b, _D3DVECTOR *point, _D3DVECTOR *normal, _D3DVECTOR *out)
{
    _D3DVECTOR d = b - a;
    float denom = DotProduct(*normal, d);
    if (denom != 0.0f) {
        float t = DotProduct(*normal, *point - a) / denom;
        if (t >= 0.0f && t <= 1.0f) {
            *out = a + t * d;
            return 1;
        }
    }
    return 0;
}

int PSXWaterDisplacement::FrustumIntersectPlane(_D3DVECTOR *point, _D3DVECTOR *normal)
{
    static int gFrustumIntersectID = -1;    // an unused profiler ID
    Camera *camera = LpRenderTarget->m_camera;
    _D3DVECTOR far[4], mid[4], near[4];
    int hits[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    gCameraUnderWater = 0;
    float farHeight, midHeight, nearHeight, farWidth, midWidth, nearWidth, farDistance;
    float tanHalf = FastTan(0.01745329f * (0.5f * camera->m_fov));
    midHeight = gfMidDistance * tanHalf;
    farDistance = camera->m_farClip;    // (this statement order gives retail's multiply order)
    farHeight = farDistance * tanHalf;
    nearHeight = camera->m_nearClip * tanHalf;
    float aspect = camera->m_pixelAspect;
    farWidth = farHeight * aspect;
    midWidth = midHeight * aspect;
    nearWidth = nearHeight * aspect;
    FastTan(0.5f * 0.01745329f * (m_savedFov + gGuardBandAdditionalFov));   // result unused (retail)
    float tiles = GameWater->m_numTiles << 4;
    float scale = tiles / (2.0f + tiles);
    farHeight *= scale;
    midHeight *= scale;
    nearHeight *= scale;
#ifdef SPLASHDOWN_PC
    // Widescreen (ports/pc/settings.h): these are the frustum's half extents
    // along `right`, from the 4:3 field of view; the projection is wider by
    // 1 / port_widescreen_scale(), and the water patch must reach as far.
    {
        float wide = 1.0f / port_widescreen_scale();
        farHeight *= wide;
        midHeight *= wide;
        nearHeight *= wide;
    }
#endif
    _D3DVECTOR right = Normalize(CrossProduct(camera->m_up, camera->m_lookAt));
    m_x3A0 = farWidth * camera->m_up;
    m_x3B0 = midWidth * camera->m_up;
    m_x3C0 = nearWidth * camera->m_up;
    m_x3D0 = farHeight * right;
    m_x3E0 = midHeight * right;
    m_x3F0 = nearHeight * right;
    m_x400 = farDistance * camera->m_lookAt + camera->m_position;
    far[0] = m_x400 + m_x3A0 - m_x3D0;
    far[1] = m_x400 + m_x3A0 + m_x3D0;
    far[2] = m_x400 - m_x3A0 + m_x3D0;
    far[3] = m_x400 - m_x3A0 - m_x3D0;
    m_x410 = gfMidDistance * camera->m_lookAt + camera->m_position;
    mid[0] = m_x410 + m_x3B0 - m_x3E0;
    mid[1] = m_x410 + m_x3B0 + m_x3E0;
    mid[2] = m_x410 - m_x3B0 + m_x3E0;
    mid[3] = m_x410 - m_x3B0 - m_x3E0;
    m_x420 = camera->m_nearClip * camera->m_lookAt + camera->m_position;
    near[0] = m_x420 + m_x3C0 - m_x3F0;
    near[1] = m_x420 + m_x3C0 + m_x3F0;
    near[2] = m_x420 - m_x3C0 + m_x3F0;
    near[3] = m_x420 - m_x3C0 - m_x3F0;
    hits[0] = IntersectSegment(near[0], far[0], point, normal, &m_x430[0]);
    hits[1] = IntersectSegment(near[1], far[1], point, normal, &m_x430[1]);
    hits[2] = IntersectSegment(near[2], far[2], point, normal, &m_x430[2]);
    hits[3] = IntersectSegment(near[3], far[3], point, normal, &m_x430[3]);
    hits[5] = IntersectSegment(far[1], far[2], point, normal, &m_x430[5]);
    hits[7] = IntersectSegment(far[0], far[3], point, normal, &m_x430[7]);
    hits[9] = IntersectSegment(near[1], near[2], point, normal, &m_x430[9]);
    hits[11] = IntersectSegment(near[0], near[3], point, normal, &m_x430[11]);
    if (IntersectSegment(mid[1], mid[2], point, normal, &m_x430[6])) {
        m_corners[4] = m_x430[6];
        m_corners[4].y += gfBias;
        m_x5B4 = 1;
    } else {
        m_x5B4 = 0;
    }
    if (IntersectSegment(mid[3], mid[0], point, normal, &m_x430[8])) {
        m_corners[5] = m_x430[8];
        m_corners[5].y += gfBias;
        m_x5B8 = 1;
    } else {
        m_x5B8 = 0;
    }
    for (int i = 0; i < 4; i++) {
        m_crossings[i] = -1;
        if (hits[i])
            m_crossings[i] = i;
    }
    gNearClipTopLeft = m_x420 + m_x3C0 - m_x3F0;
    if (camera->m_position.y <= gwp.y) {     // the global, not *point (retail)
        gCameraUnderWater = 1;
        if (hits[5]) {
            m_crossings[2] = 5;
            m_crossings[3] = 7;
        }
        if (hits[9]) {
            m_crossings[0] = 11;
            m_crossings[1] = 9;
        }
    } else {
        if (hits[5]) {
            m_crossings[0] = 7;
            m_crossings[1] = 5;
        }
        if (hits[9]) {
            m_crossings[2] = 9;
            m_crossings[3] = 11;
        }
    }
    return m_crossings[0] != -1;
}

// Compiler session state: an mwcc session
// that has generated a two-argument runtime helper call models every later
// helper call (__floatdisf, fptoui, fptodp, ...) as reading $a0-$a1. This unused
// function recreates that state; it is not part of the retail file. The whole
// TU still needs it for Tick (2 words off without it): nothing earlier in the
// TU makes such a call, so the state came from before the TU.
static void MWCC_SHIM_TEXT MwccSessionState(long *a) { a[0] = a[0] / a[1]; }

int PSXWaterDisplacement::Tick(float dt)
{
    unsigned int offsets[5];
    if (LpGame->m_paused)
        return 1;
    m_time += dt;
    float t = m_time * GameWater->m_scrollRate[0];
    offsets[0] = t;
    m_frac[0] = fmod(t, 1.0);
    m_invFrac[0] = 1.0f - m_frac[0];
    t = m_time * GameWater->m_scrollRate[1];
    offsets[1] = t;
    m_frac[1] = fmod(t, 1.0);
    m_invFrac[1] = 1.0f - m_frac[1];
    offsets[2] = m_time * GameWater->m_scrollRate[2];
    offsets[3] = m_time * GameWater->m_scrollRate[3];
    m_frac[2] = m_frac[3] = 0.0f;
    m_invFrac[2] = m_invFrac[3] = 1.0f;
    t = m_time * GameWater->m_scrollRate[4];
    offsets[4] = t;
    m_frac[4] = fmod(t, 1.0);
    m_invFrac[4] = 1.0f - m_frac[4];
    PSXSignatureWave *wave;
    int i = 0;
    while ((wave = m_waves[i++]) != 0)
        wave->Tick(dt, offsets);
    return GameObject::Tick(dt);
}

int PSXWaterDisplacement::PrepareGeometry()
{
    Camera *camera = LpRenderTarget->m_camera;
    camera->SetClipPlanes(camera->m_nearClip, m_params->m_farClip);
    m_savedFov = camera->m_fov;
    camera->m_fov = m_savedFov + gGuardBandAdditionalFov;
    camera->UpdateMatrices();
    WaterObject **objects = m_waterObjects;
    if (objects) {
        while (*objects)
            (*objects++)->PrepareGeometry();
    }
    int i = 0;
    int n = 0;
    PSXSignatureWave *item;
    while ((item = m_waves[i++]) != 0)
        m_signatureWaves[n++] = item;
    m_signatureWaves[n] = 0;
    camera->ResetClipPlanes();
    camera->m_fov = m_savedFov;
    camera->UpdateMatrices();
    return GameObject::PrepareGeometry();
}

void PSXWaterDisplacement::ClearViewport()
{
    static int ClearViewportID = -1;    // profiling timer id (unused)
    Camera *surface = LpRenderTarget->m_camera;
    _RECT rect;
    rect.left = surface->m_viewport.X;
    rect.right = surface->m_viewport.X + surface->m_viewport.Width;
    rect.top = surface->m_viewport.Y;
    rect.bottom = surface->m_viewport.Y + surface->m_viewport.Height;
    LpVideoCard->ClearFrameBuffer(&rect, 0, 1);
}

#pragma fast_fptosi on

static inline int Wrap(int v, unsigned int n)
{
    int r = v % n;
    if (r < 0)
        r += n;
    return r;
}

static inline unsigned char InBounds(PSXSignatureWave *wave, float dx, float dz)
{
    return dx >= 0.0f && dx <= wave->m_width && dz >= 0.0f && dz <= wave->m_length;
}

float PSXWaterDisplacement::GetSignatureWaveHeight(float x, float z, int *flag)
{
    float h0, h1, h2, h3, h4, h5;
    h0 = h1 = h2 = h3 = h4 = h5 = 0.0f;
    if (flag)
        *flag = 0;
    PSXSignatureWave *wave;
    for (int i = 0; wave = m_waves[i]; i++) {
        if (wave->m_mode == 4)
            continue;
        int inside;
        if (!wave->m_xD4) {
            float dx = x - wave->m_xC0;
            float dz = z - wave->m_xC4;
            inside = InBounds(wave, dx, dz);
        } else {
            inside = 1;
        }
        unsigned int size = wave->m_numSegments;
        unsigned char *frame0 = (unsigned char *)wave->m_surface;
        unsigned char *frame1 = (unsigned char *)wave->m_nextSurface;
        if (inside) {
            float fsize = size;
            float fu = wave->m_oneOverWidth * (fsize * (x - wave->m_xC0));
            float fv = wave->m_oneOverLength * (fsize * (z - wave->m_xC4));
            int u = Wrap((int)fu, size);
            int v = Wrap((int)fv, size);
            int index = v * wave->m_pitch + (int)wave->m_bytesPerPixel * u;
            float height = wave->m_oneOverNumWaves * (frame0[index] - 127);
            switch (wave->m_mode) {
            case 0:
                h0 += height;
                h4 += wave->m_oneOverNumWaves * (frame1[index] - 127);
                break;
            case 1:
                h1 += height;
                h5 += wave->m_oneOverNumWaves * (frame1[index] - 127);
                break;
            case 2:
                h2 += height;
                break;
            case 3:
                h3 += height;
                break;
            }
            if (flag && wave->m_x110)
                *flag = wave->m_x110;
        }
    }
    float a = h0 * m_invFrac[0] + h4 * m_frac[0];
    float b = h1 * m_invFrac[1] + h5 * m_frac[1];
    return GameWater->m_weights[0] * a + GameWater->m_weights[1] * b + GameWater->m_weights[2] * h2
         + GameWater->m_weights[3] * h3;
}

void PSXWaterDisplacement::RenderWake()
{
    static int WakeFrothID = -1;    // unused profiler ID
    unsigned int i;
    SetViewport(3);
    _D3DVECTOR uv0(0.0f, 0.0f, 0.5f);
    _D3DVECTOR uv1(0.5f, 0.5f, 0.5f);
    _D3DINTVECTOR xyz(0, 0, 0x7FC7180, 0x20);
    PSXRenderStateBlock state;
    m_packets[1].m_x90 = m_packets[2].m_x90 = 0x8200000068;
    DmaRenderItem *item = &m_x2C0[0];
    int view = m_viewIndex;
    _D3DVECTOR pos;
    m_vehicles[view]->GetPosition(0, &pos);
    _D3DVECTOR delta = gLastVehiclePosition[view] - pos;
    gLastVehiclePosition[view] = pos;

    PSXTextureMap *texture = m_wakeTextures[view];
    SetTextureMap(texture, &m_packets[m_packetIndex].m_tex0);
    _sceDmaTag tag;
    SetDmaTag(&tag, 0xB, 0x30, (_sceDmaTag *)&m_packets[m_packetIndex]);
    ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&tag, 1, "");
    m_packetIndex++;
    item->Start((DmaRenderDestination)1, 0x80);
    item->DmaAddVifCode(0x11000000);
    item->DmaAddVifCode(0x3000000);
    item->DmaAddVifCode(0x2000000);
    item->DmaBeginUnpack(0x320, 12);
    item->DmaAddGifTagToUnpack(&m_gifTag);
    unsigned int offset = 0x7C0;
    xyz.x = (unsigned int)(16.0f * (offset + delta.x));
    xyz.y = (unsigned int)(16.0f * (offset + delta.z));
    item->DmaAddVectorToUnpack(&uv0);
    item->DmaAddIntVectorToUnpack(&colorWhite);
    item->DmaAddIntVectorToUnpack(&xyz);
    offset = 0x7F0;
    xyz.x += offset;
    xyz.y += offset;
    item->DmaAddVectorToUnpack(&uv1);
    item->DmaAddIntVectorToUnpack(&colorWhite);
    item->DmaAddIntVectorToUnpack(&xyz);
    item->DmaEndUnpack();
    item->DmaAddVifCode(0x14000000);
    item->End();
    ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(item, "");

    SetTextureMap(m_frothTexture, &m_packets[m_packetIndex].m_tex0);
    _sceDmaTag tag2;
    SetDmaTag(&tag2, 0xB, 0x30, (_sceDmaTag *)&m_packets[m_packetIndex]);
    ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(&tag2, 1, "");
    m_packetIndex++;
    for (i = 0; i < m_numFroth; i++) {
        WakePoint *point = &m_wakePoints[i];
        float limit = 64.0f;
        float dx = point->m_x - pos.x;
        float dz = point->m_z - pos.z;
        if (dx <= limit && dx >= -limit && dz <= limit && dz >= -limit) {
            item = &m_wakeItems[i + 1];
            item->Start((DmaRenderDestination)1, 0x80);
            item->DmaAddVifCode(0x11000000);
            item->DmaAddVifCode(0x3000000);
            item->DmaAddVifCode(0x2000000);
            item->DmaBeginUnpack(0x320, 12);
            item->DmaAddGifTagToUnpack(&m_gifTag);
            int size = 16.0f * point->m_scale;
            float half = 8.0f * point->m_scale;
            xyz.x = 16.0f * (2048.0f + dx) - half;
            xyz.y = 16.0f * (2048.0f + dz) - half;
            item->DmaAddVectorToUnpack(&uv0);
            item->DmaAddIntVectorToUnpack(&colorWhite);
            item->DmaAddIntVectorToUnpack(&xyz);
            xyz.x += size;
            xyz.y += size;
            item->DmaAddVectorToUnpack(&uv1);
            item->DmaAddIntVectorToUnpack(&colorWhite);
            item->DmaAddIntVectorToUnpack(&xyz);
            item->DmaEndUnpack();
            item->DmaAddVifCode(0x14000000);
            item->End();
            ((PSXRenderTarget *)LpRenderTarget)->AddToRenderQueue(item, "");
        }
    }
    if (m_viewIndex == m_x6D4 - 1)
        m_numFroth = 0;
}

#pragma fast_fptosi reset


// Defined here for its retail .bss slot.
float gfStartScale;
float gfBaseLineScale;

void PSXWaterDisplacement::ApplyWakeFroth(_D3DVECTOR &position, float scale, _D3DVECTOR &velocity)
{
    if (gComputeWake && m_numFroth < 50) {
        float force = scale * (velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
        if (force >= gfMinForce) {
            WakePoint *froth = &m_wakePoints[m_numFroth++];
            froth->m_x = position.x;
            froth->m_z = position.z;
            froth->m_scale = gfBaseLineScale + gfStartScale * force;
            froth->m_time = gfTotalTime;
        }
    }
}

// Reads a rectangle of GS local memory back into main memory over PATH1/
// VIF1 (a host-to-local transfer turned around with BUSDIR).
void getTextureFromVideoMemory(unsigned char *dst, int address, int bufferWidth, int width, int height, int x,
                               int y, int psm, int bytesPerPixel)
{
    unsigned int sbw = bufferWidth >> 6;
    unsigned int spsm = psm;
    int qwc = bytesPerPixel * (width * height) / 16;
#line 2302 "PSXWaterDisplacement.cpp"
    ASSERT((qwc & 7) == 0, "Image transmission must be multiple of 8 qwords");
    LpGifPacket->ResetPacket();
    LpGifPacket->AddSourceDmaTag();
    LpGifPacket->AddGifTag(1);
    LpGifPacket->AddGifData(SCE_GS_BITBLTBUF, SCE_GS_SET_BITBLTBUF(address, sbw, spsm, 0, 0, 0));
    LpGifPacket->AddGifData(SCE_GS_TRXREG, SCE_GS_SET_TRXREG(width, height));
    LpGifPacket->AddGifData(SCE_GS_TRXPOS, SCE_GS_SET_TRXPOS(x, y, 0, 0, 0));
    LpGifPacket->AddGifData(SCE_GS_FINISH, 0);
    LpGifPacket->AddGifData(SCE_GS_TRXDIR, 1);
    LpGifPacket->AddDmaEndTag();
    GS_CSR = 2;     // clear FINISH
    LpGifPacket->SendGifPacket(1, 0);
    while (!(GS_CSR & 2))
        ;
    VIF1_STAT = 0x800000;   // FDR: VIF1 to memory
    GS_BUSDIR = 1;
    D1_QWC = qwc;
    D1_MADR = (unsigned int)dst & 0x0FFFFFFF;
    D1_CHCR = 0x100;
}


int PSXWaterDisplacement::KeyDownHandler(ProcessNode *node, KeyInfo *key)
{
    return GameObject::KeyDownHandler(node, key);
}

// The TU's .data, in retail's order. gZValue is const where it is read (a
// non-const int changes RenderWaterObjects' schedule) but lives in .data.
int gbRenderInfinitePlane = 1;
int gRenderWaterObjects = 1;
int gRenderFroth = 1;
int gClearFrameBuffer = 1;
float gfMinForce = 0.005f;
float gGuardBandAdditionalFov = 4.0f;
int giReducedScissor = 2;
int giTemp = 3;
extern const int gZValue __attribute__((section(".data"))) = 0xFFFFF;
float gfMidDistance = 125.0f;
float gfBias = 0.5f;
