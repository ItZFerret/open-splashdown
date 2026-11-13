// PSXUIWaterEffect.cpp  @ 0x00314570 (text 0x314570-0x315010)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's PSXUIWaterEffect.cpp as one translation unit: its functions in
// retail order, then its static initializer. `make link` places the object's
// data as well as its code (tools/link_c.py).



#include "d3dtypes.h"
#include "GameObject/GameObject.h"
#include "PSXUIWaterEffect/PSXUIWaterEffect.h"
#include "StringTable/StringTable.h"
#include "PSXMesh/PSXMesh.h"
#include "Mesh/Mesh.h"
#include "UIWaterPatch/UIWaterPatch.h"
#include "TextureMap/TextureMap.h"
#include "SurfaceMaterial/SurfaceMaterial.h"
#include "ShaderDescriptor/ShaderDescriptor.h"
#include "soultree_assert_block.h"
// The axis vectors every Soultree TU defines (and __sinit constructs).
#include "sinit/AxisVectors.h"

extern "C" char *strcpy(char *dst, const char *src);
extern "C" char *strcat(char *dst, const char *src);
extern char SltTextureContentDirectory[0x104];
extern char UITextureContentDirectory[0x104];
extern StringTable *LpMaterialStringTable;
extern StringTable *LpTextureMapStringTable;

// ---- src/PSXUIWaterEffect/ctor.cpp


PSXUIWaterEffect::PSXUIWaterEffect(int enabled) : UIWaterEffect(enabled)
{
    m_meshFlags = 0;
    m_numStrips = 0;
    m_stripLengths = 0;
    m_shares = 0;
    m_mesh = 0;
    m_material = -1;
    m_lightManager = 0;
    SetUpLights();
}


// ---- src/PSXUIWaterEffect/Initialize.cpp

// Builds one triangle strip per row of the water grid.
void PSXUIWaterEffect::Initialize(char *name)
{
    unsigned int color = 0xFF787878;
    char path[0x104];
    int i;
    int j;
    int index;
    int vertex;
    m_meshFlags = 7;
    m_mesh = new PSXMesh(1);
    m_numStrips = m_height - 1;
    m_stripLengths = new int[m_numStrips];
    m_shares = new PSXUIWaterEffectMeshVertexInfo[m_height * m_width];
    for (int k = 0; k < m_height - 1; k++)
        m_stripLengths[k] = m_width * 2;
    strcpy(SltTextureContentDirectory, "UI\\NTSC\\");
    m_mesh->Create(name, m_meshFlags, m_numStrips, m_stripLengths, (Mesh::MeshLightingType)0, color);
    strcpy(SltTextureContentDirectory, "Scenes\\Geometry\\Textures\\");
    strcpy(path, UITextureContentDirectory);
    strcat(path, name);
    m_material = LpMaterialStringTable->GetIndex(path);
#line 71 "PSXUIWaterEffect.cpp"
    ASSERT(m_material != -1, "WE NEED A MATERIAL FOR THE WATER MESH");
    AddChild(m_mesh, -1);
    for (i = 0; i < m_height - 1; i++) {
        vertex = 0;
        for (j = 0; j < m_width; j++) {
            index = j + i * m_width;
            m_mesh->SetXYZ(i, vertex, &m_vertices[index]);
            m_mesh->SetNormal(i, vertex, &m_normals[index]);
            m_mesh->SetUV(i, vertex, m_uvs[index].u, m_uvs[index].v);
            m_shares[index].m_strips[m_shares[index].m_count] = i;
            m_shares[index].m_vertices[m_shares[index].m_count] = vertex;
            m_shares[index].m_count++;
            vertex++;
            index = j + (i + 1) * m_width;
            m_mesh->SetXYZ(i, vertex, &m_vertices[index]);
            m_mesh->SetNormal(i, vertex, &m_normals[index]);
            m_mesh->SetUV(i, vertex, m_uvs[index].u, m_uvs[index].v);
            m_shares[index].m_strips[m_shares[index].m_count] = i;
            m_shares[index].m_vertices[m_shares[index].m_count] = vertex;
            m_shares[index].m_count++;
            vertex++;
        }
    }
}


// ---- src/PSXUIWaterEffectMeshVertexInfo/ctor.cpp

PSXUIWaterEffectMeshVertexInfo::PSXUIWaterEffectMeshVertexInfo()
{
    m_count = 0;
}


// ---- src/PSXUIWaterEffect/Create.cpp

PSXUIWaterEffect *PSXUIWaterEffect::Create(char *name, int size)
{
    UIWaterEffect::Create(size);
    Initialize(name);
    return this;
}


// ---- src/PSXUIWaterEffect/dtor.cpp

PSXUIWaterEffect::~PSXUIWaterEffect()
{
    if (m_stripLengths)
        delete[] m_stripLengths;
    if (m_shares)
        delete[] m_shares;
}


// ---- src/PSXUIWaterEffect/Tick.cpp

int PSXUIWaterEffect::Tick(float dt)
{
    GameObject::Tick(dt);
    m_patch->Tick(dt);
    static int PSXUIWaterEffect_Tick = -1;  // profiling timer id (unused)
    ModifyMesh();
    UpdateNormals();
    return 1;
}


// ---- src/PSXUIWaterEffect/UpdateNormals.cpp

// Copies each changed grid normal to every mesh vertex at that grid point.
void PSXUIWaterEffect::UpdateNormals()
{
    int i;
    PSXUIWaterEffectMeshVertexInfo *share;
    int *dirty;
    _D3DVECTOR *normal;
    int *strip;
    int *vertex;
    int *last;
    int count;

    static int PSXUIWaterEffect_UpdateNormals = -1;  // profiling timer id (unused)
    if (EnableUIWaterEffectLighting) {
        UIWaterEffect::UpdateNormals();
        count = m_width * m_height;
        share = m_shares;
        dirty = m_normalDirty;
        normal = m_normals;
        for (i = 0; i < count; i++, share++, dirty++, normal++) {
            if (*dirty) {
                strip = share->m_strips;
                last = &share->m_strips[share->m_count - 1];
                vertex = share->m_vertices;
                for (; strip < last; strip++, vertex++)
                    m_mesh->SetNormal(*strip, *vertex, normal);
                m_mesh->SetNormal(*strip, *vertex, normal);
                *dirty = 0;
            }
        }
    }
}


// ---- src/PSXUIWaterEffect/ModifyMesh.cpp

// Moves every mesh vertex at a grid point to the water patch's height.
void PSXUIWaterEffect::ModifyMesh()
{
    int z;
    int x;
    int k;
    PSXUIWaterEffectMeshVertexInfo *share;
    int *strip;
    int *v;
    int count;
    _D3DVECTOR *vertex;
    int *changed;
    float *height;
    float heights[256];

    static int PSXUIWaterEffect_ModifyMesh = -1;  // profiling timer id (unused)
#ifdef SPLASHDOWN_PC
    // Widescreen (ports/pc/settings.h): the grid is sized to fill a 4:3
    // view, and the projection is wider by 1 / port_widescreen_scale(). So
    // the grid is widened about its middle by the same ratio whenever the
    // ratio changes, and every vertex sent again. (One effect exists at a
    // time; a new one starts unwidened.)
    if (m_mesh) {
        static PSXUIWaterEffect *pcWho;
        static float pcApplied = 1.0f;
        static _D3DVECTOR *pcVertices;
        if (pcWho != this || pcVertices != m_vertices) {
            pcWho = this;
            pcVertices = m_vertices;
            pcApplied = 1.0f;
        }
        float pcWant = 1.0f / port_widescreen_scale();
        if (pcWant != pcApplied) {
            int pcTotal = m_width * m_height;
            float pcLo = m_vertices[0].x, pcHi = m_vertices[0].x;
            for (k = 1; k < pcTotal; k++) {
                if (m_vertices[k].x < pcLo)
                    pcLo = m_vertices[k].x;
                if (m_vertices[k].x > pcHi)
                    pcHi = m_vertices[k].x;
            }
            float pcMid = 0.5f * (pcLo + pcHi), pcRatio = pcWant / pcApplied;
            for (z = 0; z < pcTotal; z++) {
                m_vertices[z].x = pcMid + (m_vertices[z].x - pcMid) * pcRatio;
                for (k = 0; k < m_shares[z].m_count; k++)
                    m_mesh->SetXYZ(m_shares[z].m_strips[k], m_shares[z].m_vertices[k], &m_vertices[z]);
            }
            pcApplied = pcWant;
        }
    }
#endif
    if (m_mesh) {
        for (z = 0; z < m_height; z++) {
            int index = z * m_width;
            share = &m_shares[index];
            vertex = &m_vertices[index];
            changed = &m_heightChanged[index];
            m_patch->GetHeight(z, heights);
            height = heights;
            for (x = 0; x < m_width; x++, share++, vertex++, changed++, height++) {
                float y = 975.0f + *height;
                if (vertex->y != y) {
                    vertex->y = y;
                    count = share->m_count;
                    strip = share->m_strips;
                    v = share->m_vertices;
                    for (k = 0; k < count; k++, strip++, v++)
                        m_mesh->SetXYZ(*strip, *v, vertex);
                    *changed = 1;
                } else {
                    *changed = 0;
                }
            }
        }
    }
}


// ---- src/PSXUIWaterEffect/PrepareGeometry.cpp

int PSXUIWaterEffect::PrepareGeometry()
{
    SurfaceMaterial *material = (SurfaceMaterial *)LpMaterialStringTable->GetPointer(m_material);
    TextureMap *texture = (TextureMap *)LpTextureMapStringTable->GetPointer(material->m_shaders[0]->m_textures[0]);
    if (texture && 10.0f > texture->m_usage)
        texture->m_usage = 10.0f;
    return 1;
}


// ---- src/PSXUIWaterEffect/Render3D.cpp

int PSXUIWaterEffect::Render3D()
{
    if (m_lightManager && m_mesh)
        ((PSXMesh *)m_mesh)->SetLightInfo(m_lightDirections, m_lightColors, m_lightAmbient);
    return GameObject::Render3D();
}


// ---- src/PSXUIWaterEffect/SetLightManager_00314F70.cpp

void PSXUIWaterEffect::SetLightManager(LightManager *lightManager)
{
    m_lightManager = lightManager;
}


// ---- src/PSXUIWaterEffect/SetUpLights.cpp
void PSXUIWaterEffect::SetUpLights()
{
    m_lightDirections[0].m_value = 0;
    m_lightDirections[1].m_value = 0;
    m_lightDirections[2].m_value = 0;
    m_lightDirections[3].m_value = 0;
    m_lightColors[0].m_value = 0;
    m_lightColors[1].m_value = 0;
    m_lightColors[2].m_value = 0;
    m_lightColors[3].m_value = 0;
    m_lightAmbient[0].m_value = 0;
    m_lightAmbient[1].m_value = 0;
    m_lightAmbient[2].m_value = 0;
    m_lightAmbient[3].m_value = 0;
    m_lightDirections[0].f[0] = -0.577f;
    m_lightDirections[0].f[1] = -0.577f;
    m_lightDirections[0].f[2] = -0.577f;
    m_lightColors[0].f[0] = m_lightAmbient[0].f[0] = 250.0f;
    m_lightColors[0].f[1] = m_lightAmbient[0].f[1] = 250.0f;
    m_lightColors[0].f[2] = m_lightAmbient[0].f[2] = 250.0f;
    // Retail clears w and then stores the integer 64 into it.
    m_lightColors[0].ui[3] = m_lightAmbient[0].ui[3] = 0;
    m_lightColors[0].ui[3] = m_lightAmbient[0].ui[3] = 64;
    m_lightColors[3].f[0] = m_lightColors[3].f[1] = m_lightColors[3].f[2] = 70.0f;
    m_lightColors[3].f[3] = 64.0f;
}


// ---- src/sinit/PSXUIWaterEffect.cpp
// Static initializer of PSXUIWaterEffect.cpp: only the axis vectors from the shared header.

