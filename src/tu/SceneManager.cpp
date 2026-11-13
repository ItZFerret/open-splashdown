// SceneManager.cpp  @ 0x00285AC0 (text 0x285AC0-0x290BB0)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's SceneManager.cpp as one translation unit: SceneManager's methods,
// Scene's (the scene file reader), RetrieveColorFromString, the two trigger
// callbacks, LightData's ctor and the static initializer. `make link` places
// the object's data as well as its code (tools/link_c.py). The
// ContainerList<ObjectSoundData *> members at the end of the tu_map row are
// instantiated here (see the end). Scene's m_info is the SceneManager
// (SceneInfo was a view of it); the local types are on the shared headers
// (LightData, SceneManagerData, SceneFog, ObjectSoundData, Smoker,
// Scene/SceneAnimation.h); the C-mode stubs (AddObjectSound,
// RemoveAllObjectSounds, KeyUpHandler) are members again. ReadSceneInfo and
// LoadStaticModels each need a heap-image session shim recalibrated for this
// TU (see each).



// inline_depth is TU-wide (the last value counts): Tick and ReadLightData (4 in their
// per-function files) also match at ReadAnimationData's 5. fast_fptosi is Tick's.
#pragma inline_depth(5)
#pragma fast_fptosi on

#include "SceneManager/SceneManager.h"
#include "DataAccess/DataAccess.h"
#include "ParameterBlock/ParameterBlock.h"
#include "d3dtypes.h"
#include "LightEmitter/_D3DCOLORVALUE.h"
#include "GameObject/GameObject.h"
#include "ContainerList/ContainerList.h"
#include "LightManager/LightManager.h"
#include "SearchPlacement/SearchPlacement.h"
#include "Portal/Portal.h"
#include "Scene/Scene.h"
#include "PSXSoundGroup/PSXSoundGroup.h"
#include "AuralScape/AuralScape.h"
#include "Game/Game.h"
#include "PSXSound/PSXSound.h"
#include "SoultreeObject/SoultreeObject.h"
#include "CollisionObject/CollisionObject.h"
#include "Motion/Motion.h"
#include "Water/Water.h"
#include "Camera/Camera.h"
#include "Character/Character.h"
#include "CollisionCharacter/CollisionCharacter.h"
#include "Scene/SceneAnimation.h"
#include "JetskiGame/JetskiGame.h"
#include "JetskiCompetition/JetskiCompetition.h"
#include "PSXWater/PSXWater.h"
#include "CarProcedural/CarProcedural.h"
#include "BoatProcedural/BoatProcedural.h"
#include "SceneManagerData/SceneManagerData.h"
#include "SceneFog/SceneFog.h"
#include "ObjectSoundData/ObjectSoundData.h"
#include "LightData/LightData.h"
#include "Smoker/Smoker.h"
#include "SearchList/SearchList.h"
#include "VisibilityQuadTree/VisibilityQuadTree.h"
#include "AmbientListSoundEmitter/AmbientListSoundEmitter.h"
#include "CollisionTypeRegistry/CollisionTypeRegistry.h"
#include "PSXSoundInterface/PSXSoundInterface.h"
#include "ControlInterface/ControlTypes.h"
#include "soultree_assert_block.h"
#include "SearchNode/SearchNode.h"
#include "_DS3DBUFFER/_DS3DBUFFER.h"
#include "PSXRenderableObject/PSXRenderableObject.h"
#include "PhysicsObject/PhysicsObject.h"
#include "PhysicsRigidBody/PhysicsRigidBody.h"
#include "BuoyObject/BuoyObject.h"
#include "TriggerObject/Vu0Orient.h"
#include "TriggerObject/TriggerObject.h"
#include "D3DIMSoultreeCharacter/D3DIMSoultreeCharacter.h"
#include "MotionLibrary/MotionLibrary.h"
#include "ParticleManager/ParticleManager.h"
#include "LightEmitter/LightEmitter.h"
#include "sinit/AxisVectors.h"

extern JetskiGame *LpGame;

extern "C" void *memset(void *dst, int c, unsigned int n);
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" char *strncpy(char *dst, const char *src, unsigned int n);
extern "C" int strcasecmp(const char *a, const char *b);
void PSXprintf(const char *fmt, ...);
extern char SceneContentDirectory[];
extern DataAccess *LpDataAccess;
extern "C" unsigned int strlen(const char *s);
extern "C" char *strcpy(char *dst, const char *src);
extern "C" void free(void *p);
class KeyInfo;
class ProcessNode;
struct FILE_HANDLE_STRUCT;
int FCLOSE(FILE_HANDLE_STRUCT *file);
extern LightManager *LpLightManager;
extern SearchPlacement *LpGlobalSearchPlacement;
extern VisibilityQuadTree *LpVisibilityQuadTree;
class PSXSoundInterface;
extern "C" int rand();
extern "C" char *strtok(char *s, const char *delim);
extern "C" double atof(const char *s);
extern "C" int strcmp(const char *a, const char *b);
extern "C" int atoi(const char *s);
struct ObjectEnumerator {
    char *m_name;
    int m_value;
};
// Light types by name (GetEnumerator's table for the light "type" key).
ObjectEnumerator LightEmitterEnumerator[] = {
    {"Undefined", 0},
    {"ObjectPoint", 1},
    {"Directional", 2},
    {"Ambient", 3},
    {"", 0},
};
int GetEnumerator(int *value, char *name, ObjectEnumerator *table);
extern "C" char *strstr(const char *s, const char *find);
extern "C" char *strcat(char *dst, const char *src);
_D3DCOLORVALUE RetrieveColorFromString(char *string);
class PSXSoundGroup;
extern char AudioContentDirectory[];
class Camera;
class ParticleManager;
class ParameterBlock;
class GameObject;
extern "C" unsigned int strcspn(const char *s, const char *reject);
extern char SltGeometryContentDirectory[];
extern const unsigned int WORLDOBJECT_STATICOBJECT;
int GetOrientationVectors(_D3DVECTOR *forward, _D3DVECTOR *up, float yaw, float pitch, float roll);
class SearchNode;
class SoultreeSoundEmitter;
void PhysicsObjectTriggerCallBack(CollisionObject *, void *, void *userData, int entering);
void AnimationTriggerCallBack(CollisionObject *, void *, void *userData, int entering);
void EnableTriggerCallBack(CollisionObject *object, void *, void *userData, int active);
extern "C" int strncasecmp(const char *a, const char *b, unsigned int n);
extern MotionLibrary *LpMotionLibrary;
extern char CollisionContentDirectory[];
extern char AnimContentDirectory[];
extern "C" void *malloc(unsigned int size);
FILE_HANDLE_STRUCT *FOPEN(char *name, const char *mode);

// ---- src/SceneManager/ctor.cpp


SceneManager::SceneManager()
{
    memset(m_sceneName, 0, sizeof(m_sceneName));
    memset(m_sceneFile, 0, sizeof(m_sceneFile));
    memset(m_estFile, 0, sizeof(m_estFile));
    memset(m_terrainFile, 0, sizeof(m_terrainFile));
    memset(m_cubeFile, 0, sizeof(m_cubeFile));
    memset(m_waypointFile, 0, sizeof(m_waypointFile));
    memset(m_wttFile, 0, sizeof(m_wttFile));
    m_scenePath[0] = 0;
    m_sceneValid = 0;
    m_numWaypointRaces = 0;
}


// ---- src/SceneManager/dtor.cpp

SceneManager::~SceneManager()
{
}


// ---- src/SceneManager/SetCurrentSceneFile.cpp


void SceneManager::SetCurrentSceneFile(char *name, char *extension, int waypointSet)
{
    char fileName[0x110];
    char path[0x110];
    char descriptor[0x40];
    sprintf(fileName, "%s.%s", name, extension);
    strncpy(m_sceneFile, fileName, 0x3F);
    SetCurrentTerrainFile(name);
    sprintf(path, "%s%s", SceneContentDirectory, m_sceneFile);
    if (LpDataAccess->FileExists(path)) {
        GetSceneDescriptor(descriptor, m_sceneFile, waypointSet);
        if (waypointSet) {
            if (!strcasecmp(descriptor, "NO NAME")) {
                m_sceneValid = 0;
                PSXprintf("Waypoint file %s%s%02d.wpt does not exist\n", SceneContentDirectory, name, waypointSet);
                return;
            }
            m_sceneValid = 1;
        } else {
            m_sceneValid = 1;
        }
    } else {
        m_sceneValid = 0;
        PSXprintf("Scene file %s does not exist\n", path);
    }
}


// ---- src/SceneManager/SetCurrentTerrainFile.cpp


void SceneManager::SetCurrentTerrainFile(char *name)
{
#line 175 "SceneManager.cpp"
    ASSERT(strlen(name) < 28, "Terrain filename too long!");
    sprintf(m_terrainFile, "%s.trn", name);
    strcpy(m_cubeFile, "");
    sprintf(m_waypointFile, "%s01.wpt", name);
    sprintf(m_estFile, "%s.est", name);
    sprintf(m_wttFile, "%s.wtt", name);
}


// ---- src/SceneManager/GetSceneDescriptor.cpp


// An inlined strrchr.
static inline char *FindLast(char *s, char c)
{
    char *start = s;
    while (*s++)
        ;
    do {
        s--;
    } while (s != start && *s != c);
    if (*s == c)
        return s;
    return 0;
}


void SceneManager::GetSceneDescriptor(char *descriptor, char *sceneFile, int waypointSet)
{
    char path[0x110];
    char name[0x110];
    sprintf(path, "%s%s", SceneContentDirectory, sceneFile);
    ParameterBlock *params = new ParameterBlock;
    params->OpenFile(path, 0);
    params->ReadParameterBlock("SceneInfo");
    params->GetParameter("NumberOfWaypointRaces", 0, &m_numWaypointRaces);
    if (waypointSet) {
        if (m_numWaypointRaces) {
            strcpy(name, sceneFile);
            strcpy(FindLast(name, '.'), "");
            sprintf(path, "%s%02d.wpt", name, waypointSet);
            strncpy(m_waypointFile, path, 0x3F);
            sprintf(path, "%s%s", SceneContentDirectory, m_waypointFile);
            if (LpDataAccess->FileExists(path)) {
                sprintf(m_scenePath, "%s%s", SceneContentDirectory, m_waypointFile);
                sprintf(path, "WaypointRaceName%d", waypointSet);
                if (!params->GetParameter(path, "", descriptor, 0x3F))
                    strncpy(descriptor, "NO NAME", 0x3F);
            } else {
                m_scenePath[0] = 0;
                strncpy(descriptor, "NO NAME", 0x3F);
            }
        } else {
            m_scenePath[0] = 0;
            strncpy(descriptor, "NO NAME", 0x3F);
        }
    } else {
        if (!params->GetParameter("SceneName", "", descriptor, 0x3F))
            strncpy(descriptor, "NO NAME", 0x3F);
    }
    if (params)
        delete params;
}


// Retail creates "." (@1650) between GetSceneDescriptor's literals and
// ReadD3DVector's, long before LoadStaticModels uses it: a function here that
// nothing called, which retail's linker stripped. link_c leaves its section
// out.
static void StrippedDot()
{
    PSXprintf(".");
}


// ---- src/Scene/ctor.cpp


Scene::Scene(int enabled) : GameObject(enabled)
{
    int i;
    m_numLights = 0;
    m_lights = 0;
    m_fog = 0;
    m_manager = 0;
    m_flashEnabled = 0;
    m_flashTime = 0;
    m_soundDelay = 0;
    m_alternateSoundDelay = 0;
    m_idleTime = 0;
    m_minIdleTime = 0;
    m_x114 = 0;
    m_duration = 0;
    m_minDuration = 0;
    m_x120 = 0;
    m_volume = 0;
    m_minVolume = 0;
    m_x12C = 0;
    m_soundVolume = 0;
    m_alternateSoundVolume = 0;
    m_sound = 0;
    m_alternateSound = 0;
    m_flashSound = 0;
    m_color.r = 127.5f;
    m_color.g = 127.5f;
    m_color.b = 127.5f;
    m_color.a = 64.0f;
    m_minColor.r = 127.5f;
    m_minColor.g = 127.5f;
    m_minColor.b = 127.5f;
    m_minColor.a = 64.0f;
    m_currentColor.r = 127.5f;
    m_currentColor.g = 127.5f;
    m_currentColor.b = 127.5f;
    m_currentColor.a = 64.0f;
    m_soundMinDelay = 0;
    m_soundMaxDelay = 0;
    m_numBuoys = 0;
    for (i = 0; i < 20; i++) {
        m_buoyOrient[i] = 0;
        m_buoyWeight[i] = 0;
        m_buoyInertia[i] = 0;
        m_buoySpring[i] = 0;
        m_buoyDamping[i] = 0;
        strcpy(m_buoyNames[i], "");
    }
    m_animations = 0;
    m_zoneModels = 0;
    m_scoringTriggers = 0;
    m_numScoringTriggers = 0;
    m_animationTriggers = 0;
    m_numAnimationTriggers = 0;
    m_enableTriggers = 0;
    m_numEnableTriggers = 0;
    m_sceneName[0] = 0;
    m_fileName[0] = 0;
    m_startPosition.x = 0;
    m_startPosition.y = 0;
    m_startPosition.z = 0;
    m_startPosition.w = 1.0f;
    m_startDirection.x = 0;
    m_startDirection.y = 0;
    m_startDirection.z = 0;
    m_startDirection.w = 1.0f;
    m_crowdPresent = 0;
    m_ambientSounds.Create(4, 4);
    m_soundGroup = 0;
    m_xA88[0] = 0;
    m_stadiumFileName[0] = 0;
    m_ghostOpacity = 0;
    m_ghostReset = 0;
    m_numCameras = 0;
    m_file = 0;
    m_visible = 1;
    m_xA44 = 0;
    m_zones = 0;
    m_staticLists = 0;
    m_dynamicLists = 0;
    m_staticNodes = 0;
    m_dynamicNodes = 0;
    m_portals = 0;
    m_params = 0;
    m_cameraZones = 0;
    for (i = 0; i < 6; i++)
        m_impactSoundModels[i].Create(1, 1);
    m_haveImpactSounds = 0;
    m_objectSounds.Create(1, 1);
    m_haveObjectSounds = 0;
}


// ---- src/Scene/AddObjectSound.cpp

void Scene::AddObjectSound(ObjectSoundData *data)
{
    data->m_objects.Create(1, 1);
    m_objectSounds.AddItem(data);
}


// ---- src/Scene/RemoveAllObjectSoundModels.cpp


void Scene::RemoveAllObjectSoundModels(ObjectSoundData *data)
{
    while (data->m_objects.m_numItems) {
        char *model = data->m_objects.GetItem(0);
        data->m_objects.RemoveItemByIndex(0);
        free(model);
    }
    data->m_objects.Clear();
}


// ---- src/Scene/RemoveAllObjectSounds.cpp

void Scene::RemoveAllObjectSounds()
{
    while (m_objectSounds.m_numItems) {
        ObjectSoundData *data = m_objectSounds.GetItem(0);
        RemoveAllObjectSoundModels(data);
        m_objectSounds.RemoveItemByIndex(0);
        delete data;
    }
    m_objectSounds.Clear();
}


// ---- src/Scene/RemoveAllImpactSoundModels.cpp


void Scene::RemoveAllImpactSoundModels()
{
    for (int i = 0; i < 6; i++) {
        while (m_impactSoundModels[i].m_numItems) {
            char *model = m_impactSoundModels[i].GetItem(0);
            m_impactSoundModels[i].RemoveItemByIndex(0);
            free(model);
        }
        m_impactSoundModels[i].Clear();
    }
}


// ---- src/Scene/dtor.cpp


Scene::~Scene()
{
    int i;
    if (m_file)
        FCLOSE(m_file);
    if (m_lights)
        delete[] m_lights;
    if (m_fog)
        delete m_fog;
    if (m_manager)
        delete m_manager;
    if (m_animations) {
        if (m_animations->m_count > 0) {
            for (i = 0; i < m_animations->m_count; i++) {
                if (m_animations->m_animations[i].m_isCharacter && m_animations->m_animations[i].m_numMotions > 0) {
                    if (m_animations->m_animations[i].m_motions)
                        delete[] m_animations->m_animations[i].m_motions;
                    if (m_animations->m_animations[i].m_sequences)
                        delete[] m_animations->m_animations[i].m_sequences;
                }
                if (m_animations->m_animations[i].m_numWakePoints > 0)
                    delete[] m_animations->m_animations[i].m_wakePoints;
            }
            delete[] m_animations->m_animations;
        }
        if (m_animations->m_numRandomSets > 0) {
            for (int j = 0; j < m_animations->m_numRandomSets; j++) {
                if (m_animations->m_randomSets[j].m_count > 0) {
                    if (m_animations->m_randomSets[j].m_anims)
                        delete[] m_animations->m_randomSets[j].m_anims;
                    if (m_animations->m_randomSets[j].m_motions)
                        delete[] m_animations->m_randomSets[j].m_motions;
                }
            }
            delete[] m_animations->m_randomSets;
        }
        delete m_animations;
    }
    if (m_scoringTriggers)
        delete m_scoringTriggers;
    if (m_params) {
        delete m_params;
        m_params = 0;
    }
    if (m_cameraZones)
        delete[] m_cameraZones;
    LpLightManager->ReleaseLightGraph();
    delete LpGlobalSearchPlacement;
    LpGlobalSearchPlacement = 0;
    CleanUpScene();
    if (m_zones)
        delete[] m_zones;
    if (m_staticNodes)
        delete[] m_staticNodes;
    if (m_portals)
        delete[] m_portals;
    if (m_staticLists) {
        for (i = 0; i < m_numZones; i++)
            delete m_staticLists[i];
        delete[] m_staticLists;
    }
    if (m_dynamicNodes)
        delete[] m_dynamicNodes;
    if (m_dynamicLists) {
        for (i = 0; i < m_numZones; i++)
            delete m_dynamicLists[i];
        delete[] m_dynamicLists;
    }
}


// ---- src/Scene/CleanUpScene.cpp

void Scene::CleanUpScene()
{
    if (m_zoneModels) {
        for (int i = 0; i < m_numZones; i++) {
            delete[] m_zoneModels[i]->m_models;
            delete m_zoneModels[i];
        }
        delete[] m_zoneModels;
        m_zoneModels = 0;
    }
}


// ---- src/Scene/Create.cpp


Scene *Scene::Create(SceneManager *manager, AuralScape *, unsigned char flag)
{
    m_info = manager;
    m_xA88[0] = flag;
    if (LpGame->m_auralScape) {
        m_soundGroup = new PSXSoundGroup(LpGame->m_auralScape->m_soundInterface, m_enabled);
        m_soundGroup = (PSXSoundGroup *)m_soundGroup->Create();
        AddChild(m_soundGroup, 2);
        m_soundGroup->SetVolume(100);
    }
    m_params = new ParameterBlock;
    return this;
}


// ---- src/Scene/Enable.cpp


void Scene::Enable()
{
    for (int i = 0; i < m_ambientSounds.m_numItems; i++) {
        PSXSound *sound = m_ambientSounds.GetItem(i);
        if (sound)
            sound->Play(0, 0);
    }
    GameObject::Enable();
}


// ---- src/Scene/AncestorsEnabled.cpp


void Scene::AncestorsEnabled()
{
    for (int i = 0; i < m_ambientSounds.m_numItems; i++) {
        PSXSound *sound = m_ambientSounds.GetItem(i);
        if (sound)
            sound->Play(0, 0);
    }
    GameObject::AncestorsEnabled();
}


// ---- src/Scene/Tick.cpp
// Tick__5SceneFf  @ 0x00286D40
// Ticks the scene's animated characters (the sequenced ones, then the random sets) and the
// ambient colour flash. Max/Min (Camera.h) are asm helpers, so the global optimizer is off.


static inline float RandomUnit() { return 4.656613e-10f * rand(); }

static inline int FloatToInt(float f)
{
    int i;
    asm("cvt.w.s $f5, %1; mfc1 %0, $f5" : "=r"(i) : "f"(f) : "$f5");
    return i;
}

// The libvu0-style transform (out = m * v, w = 1) wrapped in a by-value inline: only this
// nesting gives retail's address set-up order and the copy through the return slot.
static inline void Apply(_D3DVECTOR *out, _D3DMATRIX *m, _D3DVECTOR *v)
{
    asm("lqc2 vf4, 0x0(%2); lqc2 vf5, 0x10(%2); lqc2 vf6, 0x20(%2); lqc2 vf7, 0x30(%2); lqc2 vf8, 0x0(%1); vmulw.w vf9, vf0, vf0w; vmulax.xyz ACC, vf4, vf8x; vmadday.xyz ACC, vf5, vf8y; vmaddz.xyz vf9, vf6, vf8z; vadd.xyz vf9, vf9, vf7; sqc2 vf9, 0x0(%0)"
        : : "r"(out), "r"(v), "r"(m) : "memory");
}

static inline _D3DVECTOR TransformPoint(_D3DMATRIX *m, _D3DVECTOR *v)
{
    _D3DVECTOR r;
    Apply(&r, m, v);
    return r;
}

int Scene::Tick(float dt)
{
    int i;
    float scaledDt;
    float t;

    if (LpGame->m_paused)
        return 1;
    scaledDt = LpGame->m_x240 * dt;
    if (m_animations) {
        for (i = 0; i < m_animations->m_count; i++) {
            CollisionCharacter *character;
            Motion *motion;
            int j;
            if (!m_animations->m_animations[i].m_isCharacter)
                continue;
            if (m_xA44 && !m_animations->m_animations[i].m_x45_bit7)
                continue;
            character = m_animations->m_animations[i].m_character;
            if (m_animations->m_animations[i].m_inRandomSet)
                continue;
            if (!m_animations->m_animations[i].m_automaticSequence)
                continue;
            character->m_root->Unhide();
            m_animations->m_animations[i].m_character->m_collision->Enable();
            if (m_animations->m_animations[i].m_numSequences) {
                if ((motion = character->m_motion)) {
                    if (m_animations->m_animations[i].m_numWakePoints > 0) {
                        for (j = 0; j < m_animations->m_animations[i].m_numWakePoints; j++) {
                            if (m_animations->m_animations[i].m_motion == m_animations->m_animations[i].m_wakePoints[j].m_motion) {
                                _D3DVECTOR position = m_animations->m_animations[i].m_wakePoints[j].m_offset;
                                _D3DMATRIX matrix;
                                character->m_root->GetMatrix(&matrix);
                                position = TransformPoint(&matrix, &position);
                                if (m_animations->m_animations[i].m_wakePoints[j].m_frame == (int)m_animations->m_animations[i].m_time * m_animations->m_animations[i].m_motions[m_animations->m_animations[i].m_motion]->m_fps)
                                    LpGame->m_competition->m_water->ApplyExternalForce(m_animations->m_animations[i].m_wakePoints[j].m_force, position);
                            }
                        }
                    }
                    float remaining = motion->m_duration - m_animations->m_animations[i].m_time;
                    if (!m_animations->m_animations[i].m_loopingSequence && m_animations->m_animations[i].m_motion + 1 >= m_animations->m_animations[i].m_numSequences) {
                        if (remaining <= 0.0f)
                            m_animations->m_animations[i].m_automaticSequence = 0;
                    } else if (remaining < motion->m_transitionTime / 2.0f) {
                        if (++m_animations->m_animations[i].m_sequence >= m_animations->m_animations[i].m_numSequences) {
                            m_animations->m_animations[i].m_sequence = 0;
                            if (!m_animations->m_animations[i].m_loopingSequence)
                                m_animations->m_animations[i].m_automaticSequence = 0;
                        }
                        if (m_animations->m_animations[i].m_automaticSequence) {
                            m_animations->m_animations[i].m_time = 0.0f;
                            m_animations->m_animations[i].m_motion = m_animations->m_animations[i].m_sequences[m_animations->m_animations[i].m_sequence] - 1;
                            character->TransitionToMotion(m_animations->m_animations[i].m_motions[m_animations->m_animations[i].m_motion], 2.0f * remaining, 0);
                        }
                    }
                } else if (m_animations->m_animations[i].m_numSequences == 1 && !m_animations->m_animations[i].m_motions[m_animations->m_animations[i].m_motion]->m_looping) {
                    m_animations->m_animations[i].m_automaticSequence = 0;
                } else {
                    if (++m_animations->m_animations[i].m_sequence >= m_animations->m_animations[i].m_numSequences)
                        m_animations->m_animations[i].m_sequence = 0;
                    m_animations->m_animations[i].m_time = 0.0f;
                    m_animations->m_animations[i].m_motion = m_animations->m_animations[i].m_sequences[m_animations->m_animations[i].m_sequence] - 1;
                    RewindAnimation(i, 0.0f, 0, 0);
                }
            } else if (m_animations->m_animations[i].m_character->m_x10) {
                character->PerformActionByMotion(m_animations->m_animations[i].m_motions[m_animations->m_animations[i].m_motion]);
            }
            if (!character->m_motion->m_looping && character->m_x10) {
                RewindAnimation(i, 0.0f, 0, 0);
                m_animations->m_animations[i].m_time = 0.0f;
                character->m_frame = 0.0f;
                character->PerformActionByMotion(m_animations->m_animations[i].m_motions[0]);
                character->TickBlendedFrame(0.0f, 0, 0);
                character->m_root->UpdateSearchPlacement();
                character->m_collision->UpdateSearchPlacement();
                m_animations->m_animations[i].m_automaticSequence = 0;
            } else {
                m_animations->m_animations[i].m_time += scaledDt;
                character->TickBlendedFrame(scaledDt, 0, 0);
                character->m_root->UpdateSearchPlacement();
                character->m_collision->UpdateSearchPlacement();
            }
        }
        if (m_xA44)
            return 1;
        for (i = 0; i < m_animations->m_numRandomSets; i++) {
            int count;
            CollisionCharacter *character;
            SceneAnimation *animation;
            count = m_animations->m_randomSets[i].m_count;
            if (!count)
                continue;
            animation = m_animations->m_randomSets[i].m_animations;
            if (!m_animations->m_animations[i].m_isCharacter)
                continue;
            character = animation->m_character;
            if (!character->m_enabled || character->m_x10) {
                character->Disable();
                int k = FloatToInt(count * RandomUnit());
                animation = &m_animations->m_animations[m_animations->m_randomSets[i].m_anims[k]];
                animation->m_motion = m_animations->m_randomSets[i].m_motions[k];
                character = animation->m_character;
                character->Enable();
                m_animations->m_randomSets[i].m_animations = animation;
                character->PerformActionByMotion(animation->m_motions[animation->m_motion]);
            }
            m_animations->m_animations[i].m_time += scaledDt;
            character->TickBlendedFrame(scaledDt, 0, 0);
            character->m_root->UpdateSearchPlacement();
            character->m_collision->UpdateSearchPlacement();
        }
    }
    if (m_flashEnabled) {
        if (m_flashTime > m_x114)
            LpLightManager->ChangeAmbientColor(*(QWORD *)&m_currentColor);
        else
            LpLightManager->RestoreAmbientColor();
        if (m_soundDelay > 0.0f) {
            m_soundDelay -= dt;
            if (m_soundDelay <= 0.0f) {
                m_sound->Play(0, 0);
                m_sound->SetVolume(m_soundVolume);
            }
        }
        if (m_alternateSoundDelay > 0.0f) {
            m_alternateSoundDelay -= dt;
            if (m_alternateSoundDelay <= 0.0f) {
                m_alternateSound->Play(0, 0);
                m_alternateSound->SetVolume(m_alternateSoundVolume);
            }
        }
        if (m_flashTime > m_x120) {
            m_flashTime = 0.0f;
            t = m_idleTime * RandomUnit();
            m_x114 = Max(t, m_minIdleTime);
            t = Min(m_minDuration, m_duration * RandomUnit());
            m_x120 = m_x114 + t;
            t = RandomUnit();
            float lo;                   // one variable for the three channels: retail keeps it in $f3
            lo = m_minColor.r;
            m_currentColor.r = lo + t * (m_color.r - lo);
            lo = m_minColor.g;
            m_currentColor.g = lo + t * (m_color.g - lo);
            lo = m_minColor.b;
            m_currentColor.b = lo + t * (m_color.b - lo);
            if (m_sound) {
                if (m_alternateSound) {
                    if (RandomUnit() < 0.5f) {
                        if (m_sound->IsPlaying())
                            m_flashSound = m_alternateSound;
                        else
                            m_flashSound = m_sound;
                    } else {
                        if (m_alternateSound->IsPlaying())
                            m_flashSound = m_sound;
                        else
                            m_flashSound = m_alternateSound;
                    }
                } else {
                    m_flashSound = m_sound;
                }
                if (m_flashSound == m_sound) {
                    m_soundVolume = m_minVolume + t * (m_x12C - m_minVolume);
                    m_soundDelay = m_soundMaxDelay - t * (m_soundMaxDelay - m_soundMinDelay);
                    if (m_soundDelay <= 0.0f) {
                        m_sound->Play(0, 0);
                        m_sound->SetVolume(m_soundVolume);
                    }
                } else if (m_flashSound == m_alternateSound) {
                    m_alternateSoundVolume = m_minVolume + t * (m_x12C - m_minVolume);
                    m_alternateSoundDelay = m_soundMaxDelay - t * (m_soundMaxDelay - m_soundMinDelay);
                    if (m_alternateSoundDelay <= 0.0f) {
                        m_alternateSound->Play(0, 0);
                        m_alternateSound->SetVolume(m_alternateSoundVolume);
                    }
                }
            }
        }
        m_flashTime += dt;
    }
    return GameObject::Tick(scaledDt);
}


// ---- src/Scene/RewindAnimation.cpp

int Scene::RewindAnimation(int index, float time, int force, int sequence)
{
    if (!m_animations)
        return 0;
    int reset = 1;
    if (time < 0.0001f) {
        time = 0.0001f;
        reset = 0;
    }
    if (m_animations->m_animations[index].m_isCharacter) {
        Character *character = m_animations->m_animations[index].m_character;
        if (character->m_x10 || force) {
            if (sequence >= m_animations->m_animations[index].m_numSequences)
                m_animations->m_animations[index].m_sequence = 0;
            else
                m_animations->m_animations[index].m_sequence = sequence;
            m_animations->m_animations[index].m_motion = m_animations->m_animations[index].m_sequences[m_animations->m_animations[index].m_sequence] - 1;
            character->PerformActionByMotion(m_animations->m_animations[index].m_motions[m_animations->m_animations[index].m_motion]);
            character->m_frame = m_animations->m_animations[index].m_time;
            m_animations->m_animations[index].m_time = time;
            character->TickBlendedFrame(time, 0, 0);
        }
    } else if (m_animations->m_animations[index].m_isCar) {
        m_animations->m_animations[index].m_car->ResetTime(time, reset);
        if (reset)
            m_animations->m_animations[index].m_car->Tick(0.0001f);
    } else {
        m_animations->m_animations[index].m_boat->ResetTime(time, reset);
        if (reset)
            m_animations->m_animations[index].m_boat->Tick(0.0001f);
    }
    return 0;
}


// ---- src/Scene/ReportError.cpp


void Scene::ReportError(char *, char *, char *) {}


// ---- src/Scene/ReportError_00287C50.cpp


void Scene::ReportError(char *) {}


// ---- src/Scene/ReportSuccess_00287C60.cpp


void Scene::ReportSuccess(char *) {}


// ---- src/Scene/ReportSuccess_00287C70.cpp


void Scene::ReportSuccess(char *, char *, char *) {}


// ---- src/Scene/ReportSuccess.cpp

void Scene::ReportSuccess(char *, char *, float) {}


// ---- src/Scene/ReportSuccess_00287C90.cpp


void Scene::ReportSuccess(char *, char *, int) {}


// ---- src/Scene/ReportSuccessBOOL.cpp


void Scene::ReportSuccessBOOL(char *, char *, int) {}


// ---- src/Scene/ReportSuccess_00287CB0.cpp


void Scene::ReportSuccess(char *, char *, _D3DVECTOR *) {}


// ---- src/Scene/ReadD3DVector.cpp


// Retail repeats the parse in each branch; a macro reproduces that.
#define PARSE_VECTOR()                                                  \
    do {                                                                \
        ReportSuccess(block, name, buffer);                             \
        v->x = atof(strtok(buffer, ","));                               \
        v->y = atof(strtok(0, ","));                                    \
        v->z = atof(strtok(0, "\n"));                                   \
    } while (0)

// Reads "x,y,z". A missing parameter falls back to defaultValue (when not
// empty), but still reports failure.
int Scene::ReadD3DVector(_D3DVECTOR *v, char *block, char *name, char *defaultValue, int arg, ParameterBlock *params)
{
    char buffer[400];
    if (!params)
        params = m_params;
    params->ReadParameterBlock(block);
    if (!m_info->m_sceneValid) {
        params->GetParameter(name, defaultValue, buffer, 0x80);
        PARSE_VECTOR();
        return 1;
    }
    if (params->GetParameter(name, "", buffer, 0x80)) {
        PARSE_VECTOR();
        return 1;
    }
    if (*defaultValue) {
        strcpy(buffer, defaultValue);
        PARSE_VECTOR();
    }
    return 0;
}


// ---- src/global/RetrieveColorFromString.cpp


// Parses "r,g,b[,a]" (0-255); missing components default to 255.
_D3DCOLORVALUE RetrieveColorFromString(char *string)
{
    _D3DCOLORVALUE color;
    char *token = strtok(string, ",");
    if (!token || !strcmp(string, "undef")) {
        PSXprintf("Light color red is invalid or not defined.\n");
        color.r = color.g = color.b = color.a = 255.0f;
        return color;
    }
    color.r = atoi(token);
    token = strtok(0, ",");
    if (!token) {
        PSXprintf("Light color green is invalid or not defined.\n");
        color.g = color.b = color.a = 255.0f;
        return color;
    }
    color.g = atoi(token);
    token = strtok(0, ",");
    if (!token) {
        PSXprintf("Light color blue is invalid or not defined.");
        color.b = color.a = 255.0f;
        return color;
    }
    color.b = atoi(token);
    token = strtok(0, ",");
    if (!token) {
        color.a = 255.0f;
        return color;
    }
    color.a = atoi(token);
    return color;
}


// ---- src/Scene/ReadColorRGB.cpp


// Retail repeats the parse in both branches; a macro reproduces that.
#define PARSE_RGB()                                                     \
    do {                                                                \
        ReportSuccess(block, name, buffer);                             \
        int r = atoi(strtok(buffer, ","));                              \
        int g = atoi(strtok(0, ","));                                   \
        int b = atoi(strtok(0, "\n"));                                  \
        *color = (r << 16) | (g << 8) | b;                              \
        return 1;                                                       \
    } while (0)

// Reads "r,g,b" into a packed 0xRRGGBB color.
int Scene::ReadColorRGB(int *color, char *block, char *name, char *defaultValue, int arg, ParameterBlock *params)
{
    char buffer[400];
    if (!params)
        params = m_params;
    params->ReadParameterBlock(block);
    if (!m_info->m_sceneValid) {
        params->GetParameter(name, defaultValue, buffer, 0x80);
        PARSE_RGB();
    }
    if (params->GetParameter(name, defaultValue, buffer, 0x80)) {
        PARSE_RGB();
    }
    *color = 0;
    return 0;
}


// ---- src/Scene/ReadSeriesOfBOOLs.cpp


// Reads a comma-separated list of booleans (Y/y/T/t/1 are true) into values[];
// all false if the parameter is missing.
int Scene::ReadSeriesOfBOOLs(unsigned char *values, int count, char *block, char *name, char *defaultValue, int arg, ParameterBlock *params)
{
    char buffer[400];
    if (!params)
        params = m_params;
    params->ReadParameterBlock(block);
    if (params->GetParameter(name, defaultValue, buffer, 0x80)) {
        char *token = strtok(buffer, ",");
        for (int i = 0; i < count; i++) {
            values[i] = 0;
            if (token) {
                char c = *token;
                if (c == 'Y' || c == 'y' || c == 'T' || c == 't' || c == '1')
                    values[i] = 1;
            }
            token = strtok(0, ",");
        }
        return 1;
    }
    for (int i = 0; i < count; i++)
        values[i] = 0;
    return 0;
}


// ---- src/Scene/ReadLightData.cpp
// ReadLightData__5SceneFi  @ 0x002884C0
// Reads the scene file's [LightN] block into m_lights[index], or sets up the default lights
// (an ambient one, then point shadowcasters) when there is no scene file or no such block.
// RSqrt and Normalize are asm helpers, so the global optimizer is off.


static inline float RSqrt(float x)
{
    float r;
    asm("rsqrt.s $f8, %0, %1; swc1 $f8, 0(%2)" : : "f"(1.0f), "f"(x), "r"(&r) : "$f8", "memory");
    return r;
}

static inline float Magnitude(const _D3DVECTOR &v)
{
    float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len2 == 0.0f)
        return 0.0f;
    return 1.0f / RSqrt(len2);
}

// libvu0 sceVu0Normalize of a temporary.
static inline void Normalize(_D3DVECTOR *out, const _D3DVECTOR &in)
{
    asm("lqc2 vf4, 0x0(%1); vmul.xyz vf5, vf4, vf4; vaddy.x vf5, vf5, vf5y; vaddz.x vf5, vf5, vf5z; vrsqrt Q, vf0w, vf5x; vsub.xyzw vf6, vf0, vf0; vaddw.xyzw vf6, vf6, vf4w; vwaitq; vmulq.xyz vf6, vf4, Q; sqc2 vf6, 0x0(%0)"
        : : "r"(out), "r"(&in) : "memory");
}

// An inlined strrchr(name, '.').
static inline char *FindExtension(char *s)
{
    char *start = s;
    while (*s++)
        ;
    do {
        s--;
    } while (s != start && *s != '.');
    s = *s == '.' ? s : 0;
    return s;
}

int Scene::ReadLightData(int index)
{
    char block[0x10];
    char value[0x210];
    int atInfinity;
    int flag;

    sprintf(block, "Light%d", index + 1);
    int found = m_params->ReadParameterBlock(block);
    if (!m_info->m_sceneValid || !found) {
        LightData *light = &m_lights[index];
        if (index == 0) {
            light->m_type = 3;
            light->m_color.r = 0.35294119f;
            light->m_color.g = 0.35294119f;
            light->m_color.b = 0.35294119f;
            light->m_color.a = 0.0f;
            light->m_pPosition = 0;
            light->m_atInfinity = 0;
            light->m_direction = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            light->m_initialZone = 0;
            light->m_pDirection = 0;
            light->m_range = 0.0f;
            light->m_showDebugSphere = 0;
            light->m_emitsLight = 1;
            light->m_castsShadows = 0;
            light->m_hasLensFlare = 0;
            light->m_hasLightFlare = 0;
        } else {
            light->m_type = 1;
            light->m_color.r = 0.78431374f;
            light->m_color.g = 0.78431374f;
            light->m_color.b = 0.78431374f;
            light->m_color.a = 0.0f;
            light->m_position = _D3DVECTOR(0.0f, 4.0f, 1.252f);
            light->m_initialZone = 0;
            light->m_atInfinity = 0;
            light->m_infinitePosition = (80000.0f / Magnitude(light->m_position)) * light->m_position;
            light->m_pPosition = &light->m_position;
            light->m_direction = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            light->m_pDirection = 0;
            light->m_range = 0.0f;
            light->m_showDebugSphere = 0;
            light->m_emitsLight = 1;
            light->m_castsShadows = 1;
            light->m_hasLensFlare = 0;
            light->m_hasLightFlare = 0;
        }
        ReportError("No Scene File or no light data:  Use DEFAULT Lights (1 Ambient, 1 ObjectPoint shadowcaster\n");
        return 1;
    }
    if (found) {
        LightData *light = &m_lights[index];
        m_params->GetParameter("Type", "BLANK", value, 0x100);
        if (GetEnumerator(&light->m_type, value, LightEmitterEnumerator)) {
            ReportSuccess(block, "Type", value);
        } else {
            ReportError(block, "Type", value);
            return 0;
        }
        if (!ReadD3DVector((_D3DVECTOR *)&light->m_color, block, "ColorRGB", "", 1, 0))
            return 0;
        light->m_color.a = 0.0f;
        light->m_pPosition = &light->m_position;
        if (!ReadD3DVector(&light->m_position, block, "Position", "", 0, 0)) {
            light->m_position = _D3DVECTOR(0.0f, 4.0f, 1.252f);
            if (light->m_type != 3)
                ReportSuccess(block, "Position(Default)", light->m_pPosition);
            else
                light->m_pPosition = 0;
        }
        m_params->GetParameter("InitialZone", 1, &light->m_initialZone);
        m_params->GetParameter("PlaceLightAtInfinity", 0, &atInfinity);
        light->m_atInfinity = atInfinity;
        ReportSuccessBOOL(block, "PlaceLightAtInfinity", light->m_atInfinity);
        if (light->m_atInfinity) {
            light->m_infinitePosition = (80000.0f / Magnitude(light->m_position)) * light->m_position;
            light->m_pPosition = &light->m_infinitePosition;
        }
        _D3DVECTOR look;
        light->m_pDirection = &light->m_direction;
        if (ReadD3DVector(&look, block, "LookVector", "", 0, 0)) {
            light->m_direction = look;
        } else if (ReadD3DVector(&look, block, "TargetPosition", "", 0, 0)) {
            Normalize(&light->m_direction, look - light->m_position);
        } else {
            light->m_direction = _D3DVECTOR(0.0f, 0.0f, 0.0f);
            if (light->m_type != 2)
                light->m_pDirection = 0;
        }
        m_params->GetParameter("Range", -1.0f, &light->m_range);
        if (light->m_range == -1.0f) {
            light->m_range = 0.0f;
            if (light->m_type != 3)
                ReportSuccess(block, "Range(Default)", light->m_range);
        } else {
            ReportSuccess(block, "Range", light->m_range);
        }
        m_params->GetParameter("ShowDebugSphere", 0, &flag);
        light->m_showDebugSphere = flag;
        ReportSuccessBOOL(block, "ShowDebugSphere", flag);
        m_params->GetParameter("CastsShadows", 0, &flag);
        light->m_castsShadows = flag;
        ReportSuccessBOOL(block, "CastsShadows", flag);
        m_params->GetParameter("EmitsLight", 1, &flag);
        light->m_emitsLight = flag;
        ReportSuccessBOOL(block, "EmitsLight", flag);
        m_params->GetParameter("HasLensFlare", 0, &flag);
        light->m_hasLensFlare = flag;
        ReportSuccessBOOL(block, "HasLensFlare", flag);
        if (light->m_hasLensFlare) {
            ReadColorRGB(&light->m_lensFlareColor, block, "LensFlareColorRGB", "", 0, 0);
            m_params->GetParameter("LensFlareBrightness", -1.0f, &light->m_lensFlareBrightness);
            if (light->m_lensFlareBrightness == -1.0f) {
                light->m_lensFlareBrightness = 1.0f;
                ReportSuccess(block, "LensFlareBrightness(Default)", light->m_lensFlareBrightness);
            } else {
                ReportSuccess(block, "LensFlareBrightness", light->m_lensFlareBrightness);
            }
            if (ReadSeriesOfBOOLs(light->m_lensFlareLayers, 5, block, "LensFlareLayersVisible", "T,F,F,F,F", 0, 0)) {
                sprintf(value, "[%s].LensFlareLayersVisible=%s,%s,%s,%s,%s", block,
                        light->m_lensFlareLayers[0] ? "T" : "F", light->m_lensFlareLayers[1] ? "T" : "F",
                        light->m_lensFlareLayers[2] ? "T" : "F", light->m_lensFlareLayers[3] ? "T" : "F",
                        light->m_lensFlareLayers[4] ? "T" : "F");
                ReportSuccess(value);
            }
            if (m_params->GetParameter("LensFlareTextureMapName", "", light->m_lensFlareTexture, 0x3F))
                ReportSuccess(block, "LensFlareTextureMapName", light->m_lensFlareTexture);
            else
                strncpy(light->m_lensFlareTexture, "overlay\\lensflare.tga", 0x3F);
        }
        m_params->GetParameter("HasLightFlare", 0, &flag);
        light->m_hasLightFlare = flag;
        ReportSuccessBOOL(block, "HasLightFlare", flag);
        if (light->m_hasLightFlare) {
            ReadColorRGB(&light->m_lightFlareColor, block, "LightFlareColorRGB", "", 0, 0);
            m_params->GetParameter("LightFlareSize", 10.0f, &light->m_lightFlareSize);
            if (m_params->GetParameter("LightFlareTextureMapName", "", light->m_lightFlareTexture, 0x3F)) {
                ReportSuccess(block, "LightFlareTextureMapName", light->m_lightFlareTexture);
                char *ext = FindExtension(light->m_lightFlareTexture);
                if (ext)
                    memset(ext, 0, strlen(ext));
            } else {
                strncpy(light->m_lightFlareTexture, "flare1", 0x3F);
            }
        }
        ReportError("");
    } else {
        sprintf(value, "\nCannot find data for [%s] in %s.\n\n", block, m_fileName);
        ReportError(value);
        return 0;
    }
    return 1;
}


// ---- src/Scene/ReadFogData.cpp


// One shared `return 1` at the end: mwcc copies a branch target's first
// instruction into only the first `b` that jumps there, so the later branch
// after ReportError("") keeps retail's empty delay slot.
int Scene::ReadFogData()
{
    char message[0x208];
    float visibility;
    float haziness;
    m_fog = new SceneFog;
    m_fog->m_color = 0x8080C0;
    m_fog->m_visibility = 1.0f;
    m_fog->m_haziness = 0.5f;
    int found = m_params->ReadParameterBlock("Fog");
    if (!m_info->m_sceneValid || !found) {
        ReportError("No Scene File or no fog data:  Using DEFAULT Fog\n");
    } else if (found) {
        visibility = 1.0f;
        if (!m_params->GetParameter("Visibility", 1.0f, &visibility)) {
            ReportError("Fog", "Visibility", "NOT FOUND using default 1.0");
        } else if (visibility < 0.0f || visibility > 1.0f) {
            ReportError("Fog", "Visibility", "Invalid value, must be between 0.0 and 1.0, using default 1.0");
            visibility = 1.0f;
        }
        m_fog->m_visibility = visibility;
        ReportSuccess("Fog", "Visibility", m_fog->m_visibility);
        // Retail range-checks the visibility again here instead of the haziness.
        if (!m_params->GetParameter("Haziness", 0.5f, &haziness)) {
            ReportError("Fog", "Haziness", "NOT FOUND using default 0.5");
        } else if (visibility < 0.0f || visibility > 1.0f) {
            ReportError("Fog", "Haziness", "Invalid value, must be between 0.0 and 1.0, using default 0.5");
            haziness = 0.5f;
        }
        m_fog->m_haziness = haziness;
        ReportSuccess("Fog", "Haziness", m_fog->m_haziness);
        if (!ReadColorRGB((int *)&m_fog->m_color, "Fog", "ColorRGB", "", 1, 0))
            return 0;
        ReportError("");
    } else {
        sprintf(message, "\nCannot find data for [%s] in %s.\n\n", "Fog", m_fileName);
        ReportError(message);
    }
    return 1;
}


// ---- src/Scene/ReadAmbientColorFlashData.cpp
// ReadAmbientColorFlashData__5SceneFv  @ 0x00289490


void Scene::ReadAmbientColorFlashData()
{
    char soundFile[0x110];
    char colorString[0x40];
    if (!m_params->GetParameter("AmbientColorFlashMinIdleTime", 0.0f, &m_minIdleTime))
        return;
    m_flashEnabled = 1;
    m_params->GetParameter("AmbientColorFlashIdleTime", 0.0f, &m_idleTime);
    m_x114 = m_idleTime;
    m_params->GetParameter("AmbientColorFlashMinDuration", 0.0f, &m_minDuration);
    m_params->GetParameter("AmbientColorFlashDuration", 0.0f, &m_duration);
    m_x120 = m_duration;
    m_params->GetParameter("AmbientColorFlashMinVolume", 0.0f, &m_minVolume);
    m_params->GetParameter("AmbientColorFlashVolume", 100.0f, &m_volume);
    m_x12C = m_volume;
    m_params->GetParameter("AmbientColorFlashSoundMinDelay", 0.0f, &m_soundMinDelay);
    m_params->GetParameter("AmbientColorFlashSoundMaxDelay", 0.0f, &m_soundMaxDelay);
    if (m_params->GetParameter("AmbientColorFlashSoundFile", "", soundFile, 0x103)) {
        if (!strstr(soundFile, ".wav"))
            strcat(soundFile, ".wav");
        PSXSound *sound = LpGame->m_auralScape->m_soundInterface->GetSoundFromResourceName(m_soundGroup, soundFile, 1, 1);
        if (sound) {
            m_sound = sound;
            m_sound->SetBaseVolume(100.0f);
        }
    }
    if (m_params->GetParameter("AmbientColorFlashAlternateSoundFile", "", soundFile, 0x103)) {
        if (!strstr(soundFile, ".wav"))
            strcat(soundFile, ".wav");
        PSXSound *sound = LpGame->m_auralScape->m_soundInterface->GetSoundFromResourceName(m_soundGroup, soundFile, 1, 1);
        if (sound) {
            m_alternateSound = sound;
            m_alternateSound->SetBaseVolume(100.0f);
        }
    }
    m_params->GetParameter("AmbientColorFlashMinColorRGB", "undef", colorString, -1);
    _D3DCOLORVALUE color = RetrieveColorFromString(colorString);
    m_minColor.r = 0.5f * color.r;
    m_minColor.g = 0.5f * color.g;
    m_minColor.b = 0.5f * color.b;
    m_minColor.a = 64.0f;
    m_params->GetParameter("AmbientColorFlashColorRGB", "undef", colorString, -1);
    color = RetrieveColorFromString(colorString);
    float r = 0.5f * color.r;
    m_color.r = r;
    float g = 0.5f * color.g;
    m_color.g = g;
    float b = 0.5f * color.b;
    m_color.b = b;
    m_color.a = 64.0f;
    m_currentColor.r = r;
    m_currentColor.g = g;
    m_currentColor.b = b;
    m_currentColor.a = 64.0f;
    m_flashTime = 0;
    m_soundDelay = 0;
    m_alternateSoundDelay = 0;
}


// ---- src/Scene/ReadEnvironmentData.cpp


#define READ_FLOAT(name, def, field)                                            \
    if (!m_params->GetParameter(name, def, &m_manager->field))                 \
        ReportSuccess("Environment", name "(Default)", m_manager->field);       \
    else                                                                        \
        ReportSuccess("Environment", name, m_manager->field);

// Reads the [Environment] block (terrain, ecosystem, water and cube files,
// terrain metrics), the per-zone surface properties and the reverb zones.
int Scene::ReadEnvironmentData(char *dir, char *cubeDir)
{
    char buf[0x210];
    char zone[0x40];
    char reverb[0x40];
    int lowRes;
    int placeEco;
    int dust;
    int dirt;
    int zoneDust;
    int zoneDirt;
    int found = m_params->ReadParameterBlock("Environment");
    if (!m_info->m_sceneValid || found) {
        m_manager = new SceneManagerData;
        sprintf(m_manager->m_waterFile, "%s%s", dir, m_info->m_wttFile);
        sprintf(m_manager->m_terrainFile, "%s%s", dir, m_info->m_terrainFile);
        sprintf(m_manager->m_ecosystemFile, "%s%s", dir, m_info->m_estFile);
        if (m_info->m_cubeFile[0])
            sprintf(m_manager->m_cubeFile, "%s%s", cubeDir, m_info->m_cubeFile);
        else
            m_manager->m_cubeFile[0] = 0;
        strncpy(m_manager->m_particleTexture, "ltBROWN.TGA", 0x3F);
        strncpy(m_manager->m_detailTexture, "GrayNois.tga", 0x3F);
        m_manager->m_startGateScale = 1.0f;
        if (m_params->GetParameter("TerrainFile", "", buf, 0x100)) {
            if (strcasecmp(buf, "NONE")) {
                sprintf(m_manager->m_terrainFile, "%s%s", dir, buf);
                strncpy(m_info->m_terrainFile, buf, 0x3F);
                // Retail does not check for a missing extension here.
                strcpy(FindExtension(buf), ".est");
                sprintf(m_manager->m_ecosystemFile, "%s%s", dir, buf);
                strncpy(m_info->m_estFile, buf, 0x3F);
                ReportSuccess("Environment", "TerrainFile", m_manager->m_terrainFile);
                // Retail reports the terrain file name for the ecosystem file.
                ReportSuccess("Environment", "EcosystemFile", m_manager->m_terrainFile);
            } else {
                sprintf(m_manager->m_terrainFile, "NONE");
                strncpy(m_info->m_terrainFile, "NONE", 0x3F);
                sprintf(m_manager->m_ecosystemFile, "NONE");
                strncpy(m_info->m_estFile, "NONE", 0x3F);
            }
        } else {
            ReportSuccess("Environment", "TerrainFile(Default)", m_manager->m_terrainFile);
            ReportSuccess("Environment", "EcosystemFile(Default)", m_manager->m_ecosystemFile);
            ReportSuccess("Environment", "WaterMapFile(Default)", m_manager->m_waterFile);
        }
        if (m_params->GetParameter("EcosystemFile", "", buf, 0x100)) {
            if (strcasecmp(buf, "NONE")) {
                sprintf(m_manager->m_ecosystemFile, "%s%s", dir, buf);
                strncpy(m_info->m_estFile, buf, 0x3F);
                ReportSuccess("Environment", "EcosystemFile", m_manager->m_terrainFile);
            } else {
                sprintf(m_manager->m_ecosystemFile, "NONE");
                strncpy(m_info->m_estFile, "NONE", 0x3F);
            }
        }
        if (m_params->GetParameter("WaterFile", "", buf, 0x100)) {
            if (strcasecmp(buf, "NONE")) {
                sprintf(m_manager->m_waterFile, "%s%s", dir, buf);
                strncpy(m_info->m_wttFile, buf, 0x3F);
                ReportSuccess("Environment", "WaterFileName", m_manager->m_waterFile);
            } else {
                sprintf(m_manager->m_waterFile, "NONE");
                strncpy(m_info->m_wttFile, "NONE", 0x3F);
            }
        }
        if (m_params->GetParameter("CubeFile", "", buf, 0x104)) {
            sprintf(m_manager->m_cubeFile, "%s%s", cubeDir, buf);
            strncpy(m_info->m_cubeFile, buf, 0x3F);
            ReportSuccess("Environment", "CubeFile", m_manager->m_cubeFile);
        }
        if (m_params->GetParameter("ParticleTextureMapName", "", buf, 0x100)) {
            strncpy(m_manager->m_particleTexture, buf, 0x3F);
            m_manager->m_particleTexture[0x3F] = 0;
            ReportSuccess("Environment", "ParticleTextureMapName", m_manager->m_particleTexture);
        } else if (m_info->m_sceneValid) {
            ReportError("Environment", "ParticleTextureMapName", "dirtpart.tga");
        }
        if (m_params->GetParameter("DetailTextureMapName", "", buf, 0x100)) {
            strncpy(m_manager->m_detailTexture, buf, 0x3F);
            m_manager->m_detailTexture[0x3F] = 0;
            // Retail reports the particle texture name here.
            ReportSuccess("Environment", "DetailTextureMapName", m_manager->m_particleTexture);
        } else if (m_info->m_sceneValid) {
            ReportError("Environment", "DetailTextureMapName", "grynoise.tga");
        }
        READ_FLOAT("TerrainWidthScale", 3.0f, m_terrainWidthScale)
        READ_FLOAT("TerrainBreadthScale", 3.0f, m_terrainBreadthScale)
        READ_FLOAT("TerrainWidth", 5.0f, m_terrainWidth)
        READ_FLOAT("TerrainBreadth", 5.0f, m_terrainBreadth)
        READ_FLOAT("TerrainBorderWidth", 0.0f, m_terrainBorderWidth)
        READ_FLOAT("PosXEdgeDistFromTerrain", 0.0f, m_posXEdgeDist)
        READ_FLOAT("NegXEdgeDistFromTerrain", 0.0f, m_negXEdgeDist)
        READ_FLOAT("PosZEdgeDistFromTerrain", 0.0f, m_posZEdgeDist)
        READ_FLOAT("NegZEdgeDistFromTerrain", 0.0f, m_negZEdgeDist)
        if (!m_params->GetParameter("RenderTerrainAtLowRes", 1, &lowRes)) {
            m_manager->m_lowRes = lowRes;
            ReportSuccess("Environment", "RenderTerrainAtLowRes(Default)", m_manager->m_lowRes);
        } else {
            m_manager->m_lowRes = lowRes;
            ReportSuccess("Environment", "RenderTerrainAtLowRes", m_manager->m_lowRes);
        }
        if (!m_params->GetParameter("PlaceEcoOnTerrain", 1, &placeEco)) {
            m_manager->m_placeEco = placeEco;
            ReportSuccess("Environment", "PlaceEcoOnTerrain(Default)", m_manager->m_placeEco);
        } else {
            m_manager->m_placeEco = placeEco;
            ReportSuccess("Environment", "PlaceEcoOnTerrain", m_manager->m_placeEco);
        }
        READ_FLOAT("StartGateScale", 1.0f, m_startGateScale)
        found = m_params->ReadParameterBlock("TerrainZone1");
        m_manager->m_friction[0] = 1.0f;
        m_manager->m_drag[0] = 1.0f;
        m_manager->m_traction[0] = 1.0f;
        if (!found) {
            sprintf(buf, "\nCannot find Dust/Dirt data for [%s] in %s.\n\n", "TerrainZone1", m_fileName);
            ReportError(buf);
        } else {
            m_params->GetParameter("GenerateDust", 1, &dust);
            m_manager->m_dust[0] = dust;
            m_params->GetParameter("GenerateDirt", 1, &dirt);
            m_manager->m_dirt[0] = dirt;
        }
        int i;
        for (i = 2; i < 9; i++) {
            sprintf(zone, "TerrainZone%d", i);
            if (!m_params->ReadParameterBlock(zone)) {
                sprintf(buf, "\nCannot find data for [%s] in %s.\n\n", zone, m_fileName);
                ReportError(buf);
                m_manager->m_friction[i - 1] = 1.0f;
                m_manager->m_drag[i - 1] = 1.0f;
                m_manager->m_traction[i - 1] = 1.0f;
                m_manager->m_dust[i - 1] = i != 8;
                m_manager->m_dirt[i - 1] = i != 8;
                continue;
            }
            if (!m_params->GetParameter("SurfaceFriction", 1.0f, &m_manager->m_friction[i - 1])) {
                ReportError(zone, "SurfaceFriction", "NOT FOUND");
            } else if (m_manager->m_friction[i - 1] > 2.0f) {
                m_manager->m_friction[i - 1] = 2.0f;
            } else if (m_manager->m_friction[i - 1] < 0.05f) {
                m_manager->m_friction[i - 1] = 0.05f;
            }
            ReportSuccess(zone, "SurfaceFriction", m_manager->m_friction[i - 1]);
            if (!m_params->GetParameter("SurfaceDrag", 1.0f, &m_manager->m_drag[i - 1])) {
                ReportError(zone, "SurfaceDrag", "NOT FOUND");
            } else if (m_manager->m_drag[i - 1] > 2.0f) {
                m_manager->m_drag[i - 1] = 2.0f;
            } else if (m_manager->m_drag[i - 1] < 0.05f) {
                m_manager->m_drag[i - 1] = 0.05f;
            }
            ReportSuccess(zone, "SurfaceDrag", m_manager->m_drag[i - 1]);
            if (!m_params->GetParameter("SurfaceTraction", 1.0f, &m_manager->m_traction[i - 1])) {
                ReportError(zone, "SurfaceTraction", "NOT FOUND");
            } else if (m_manager->m_traction[i - 1] > 2.0f) {
                m_manager->m_traction[i - 1] = 2.0f;
            } else if (m_manager->m_traction[i - 1] < 0.05f) {
                m_manager->m_traction[i - 1] = 0.05f;
            }
            ReportSuccess(zone, "SurfaceTraction", m_manager->m_traction[i - 1]);
            m_params->GetParameter("GenerateDust", 1, &zoneDust);
            m_manager->m_dust[i - 1] = zoneDust;
            ReportSuccess(zone, "GenerateDust", m_manager->m_dust[i - 1]);
            m_params->GetParameter("GenerateDirt", 1, &zoneDirt);
            m_manager->m_dirt[i - 1] = zoneDirt;
            ReportSuccess(zone, "GenerateDirt", m_manager->m_dirt[i - 1]);
        }
        ReportError("");
        for (i = 0; i < 4; i++)
            m_manager->m_reverb[i] = 0;
        for (i = 0; i < 4; i++) {
            sprintf(reverb, "ReverbZone%d", i + 1);
            if (!m_params->ReadParameterBlock(reverb)) {
                sprintf(buf, "\nCannot find ReverbZone data for [%s] in %s.\n\n", reverb, m_fileName);
                ReportError(buf);
            } else {
                m_params->GetParameter("EnviroID", 0, &m_manager->m_reverb[i]);
            }
        }
        ReadAmbientColorFlashData();
    } else {
        sprintf(buf, "\nCannot find data for [%s] in %s.\n\n", "Environment", m_fileName);
        ReportError(buf);
        return 0;
    }
    return 1;
}


// ---- src/Scene/ReadResourceFiles.cpp


int Scene::ReadResourceFiles()
{
    char message[0x210];
    char key[0x40];
    char value[0x80];
    char path[0x110];
    char file[0x110];
    int found = m_params->ReadParameterBlock("ResourceFiles");
    if (!m_info->m_sceneValid || !found) {
        sprintf(message, "\nCannot find data for [%s] in %s.\n\n", "ResourceFiles", m_fileName);
        ReportError(message);
        return 0;
    }
    int n = 1;
    sprintf(key, "ResourceFile%d", n);
    while (m_params->GetParameter(key, "", value, 0x7F)) {
        sprintf(file, "%s%s", AudioContentDirectory, value);
        if (!LpGame->GetFullPathToContentFile(file, path))
            return 0;
        LpDataAccess->LoadResourceFile(path, 0, 0, 0, 0, 0, 0, 0);
        n++;
        sprintf(key, "ResourceFile%d", n);
    }
    return 1;
}


// ---- src/Scene/ReadStadiumData.cpp


void Scene::ReadStadiumData()
{
    m_hasStadium = m_params->ReadParameterBlock("Stadium");
    if (m_hasStadium) {
        m_params->GetParameter("FileName", "", m_stadiumFileName, 0xFF);
        m_params->GetParameter("Top", 0.0f, &m_stadiumTop);
        m_params->GetParameter("Bottom", 0.0f, &m_stadiumBottom);
        m_params->GetParameter("Scale", 0.0f, &m_stadiumScale);
        ReadD3DVector(&m_stadiumPosition, "Stadium", "Position", "0.0,0.0,0.0", 0, 0);
    }
}


// Retail creates "%s" (@2435) here, between ReadStadiumData's and
// ReadBuoyNames' literals, long before ReadAnimationData uses it: a stripped
// function again.
static void StrippedPercentS(char *s)
{
    PSXprintf("%s", s);
}


// ---- src/Scene/ReadBuoyNames.cpp


#define MAX_BUOYS 20
#define MIN(a, b) ((a) < (b) ? (a) : (b))


void Scene::ReadBuoyNames()
{
    char name[0x104];
    m_params->ReadParameterBlock("Buoys");
    m_numBuoys = 0;
    m_params->GetParameter("NumberOfBuoys", &m_numBuoys);
    m_numBuoys = MIN(m_numBuoys, MAX_BUOYS);
    for (int i = 1; i <= m_numBuoys; i++) {
        sprintf(name, "Buoy%d", i);
        m_params->GetParameter(name, m_buoyNames[i - 1], 0x3F);
        sprintf(name, "Orient%d", i);
        m_params->GetParameter(name, 0, &m_buoyOrient[i - 1]);
        sprintf(name, "Weight%d", i);
        m_params->GetParameter(name, 200.0f, &m_buoyWeight[i - 1]);
        sprintf(name, "Inertia%d", i);
        m_params->GetParameter(name, 1.0f, &m_buoyInertia[i - 1]);
        sprintf(name, "Spring%d", i);
        m_params->GetParameter(name, 0.2f, &m_buoySpring[i - 1]);
        sprintf(name, "Damping%d", i);
        m_params->GetParameter(name, 0.5f, &m_buoyDamping[i - 1]);
    }
}


// ---- src/Scene/ReadStaticModelData.cpp


int Scene::ReadStaticModelData(ParticleManager *particles, char *path, void (*progress)(int *), int a, int b)
{
    PSXprintf("Begin ReadStaticModelData . . .\n");
    for (int i = 0; i < m_numZones; i++) {
        ReadSoundData(m_zones[i]);
        LoadStaticModels(particles, path, progress, a, b, m_params, i, m_zones[i]);
    }
    PSXprintf("End ReadStaticModelData . . .\n");
    return 1;
}


// ---- src/Scene/ReadSceneInfo.cpp


// Session shim for ReadSceneInfo (recalibrated for this TU with r18 image.py: the
// per-function file's flags, every other constant node of the epoch cleared).
struct MwccSessionImageRSI {
    char pad0[0x79D];
    char flag0;
    char pad1[0x837];
    char flag1;
    char tail[0xF02A];
};
#pragma section mwccshim begin
inline void MwccSessionShimRSI() { const MwccSessionImageRSI image = { {0}, 1, {0}, 1 }; }
#pragma section mwccshim end
// Builds the zone graph: a search node pair per zone, the portals linking
// them (and the light graph), and the visibility search methods.
int Scene::ReadSceneInfo()
{
    char block[0x110];
    int connect1;
    int connect2;
    int i;
    m_params->ReadParameterBlock("SceneInfo");
    m_params->GetParameter("NumPortals", 0, &m_numPortals);
    m_params->GetParameter("NumZones", 1, &m_numZones);
    m_zones = new GameObject *[m_numZones];
    m_staticNodes = new SearchNode *[m_numZones];
    m_dynamicNodes = new SearchNode *[m_numZones];
    m_zoneModels = new ZoneModels *[m_numZones];
    for (i = 0; i < m_numZones; i++) {
        m_zones[i] = new GameObject(1);
        AddChild(m_zones[i], 0);
        m_staticNodes[i] = new SearchNode(1);
        m_dynamicNodes[i] = new SearchNode(1);
        m_zoneModels[i] = 0;
    }
    LpLightManager->CreateLightGraph(m_numZones);
    _D3DVECTOR vertices[4];
    if (m_numPortals > 0) {
        m_portals = new Portal[m_numPortals];
        for (i = 0; i < m_numPortals; i++) {
            sprintf(block, "Portal%d", i + 1);
            m_params->ReadParameterBlock(block);
            ReadD3DVector(&vertices[0], block, "Vertex1", "0.0,0.0,0.0", 0, 0);
            ReadD3DVector(&vertices[1], block, "Vertex2", "0.0,0.0,0.0", 0, 0);
            ReadD3DVector(&vertices[2], block, "Vertex3", "0.0,0.0,0.0", 0, 0);
            ReadD3DVector(&vertices[3], block, "Vertex4", "0.0,0.0,0.0", 0, 0);
            m_portals[i].Create(vertices);
            m_params->GetParameter("Connect1", 0, &connect1);
            m_params->GetParameter("Connect2", 0, &connect2);
            m_staticNodes[connect1 - 1]->AddNeighbor(m_staticNodes[connect2 - 1], &m_portals[i]);
            m_staticNodes[connect2 - 1]->AddNeighbor(m_staticNodes[connect1 - 1], &m_portals[i]);
            m_dynamicNodes[connect1 - 1]->AddNeighbor(m_dynamicNodes[connect2 - 1], &m_portals[i]);
            m_dynamicNodes[connect2 - 1]->AddNeighbor(m_dynamicNodes[connect1 - 1], &m_portals[i]);
            LpLightManager->m_nodes[connect1 - 1]->AddNeighbor(LpLightManager->m_nodes[connect2 - 1], &m_portals[i]);
            LpLightManager->m_nodes[connect2 - 1]->AddNeighbor(LpLightManager->m_nodes[connect1 - 1], &m_portals[i]);
        }
    }
    GameObject *nodes = new GameObject(1);
    AddChild(nodes, 0);
    if (m_numZones == 1) {
        LpVisibilityQuadTree = new VisibilityQuadTree;
        SceneManagerData *m = m_manager;
        if (!LpVisibilityQuadTree->Create(-m->m_terrainBorderWidth, -3.4028235e38f, -m->m_terrainBorderWidth,
                                          m->m_terrainBorderWidth + 256.0f * m->m_terrainWidthScale * m->m_terrainWidth, 3.4028235e38f,
                                          m->m_terrainBorderWidth + 256.0f * m->m_terrainBreadthScale * m->m_terrainBreadth, 96.0f)) {
#line 2168 "SceneManager.cpp"
            ASSERT(0, "ReadSceneInfo: Unable to create VisibilityQuadTree!");
        }
        LpVisibilityQuadTree->SetMaxSizeOfSearchList(5000);
        nodes->AddChild(m_staticNodes[0], 0);
        nodes->AddChild(m_dynamicNodes[0], 0);
        m_staticNodes[0]->m_x48 = 0;
        m_dynamicNodes[0]->m_x48 = 0;
        SearchList *list = new SearchList(0x400);
        m_staticNodes[0]->SetStaticSearchMethod(LpVisibilityQuadTree);
        m_staticNodes[0]->SetDynamicSearchMethod(list);
        m_dynamicNodes[0]->SetStaticSearchMethod(LpVisibilityQuadTree);
        m_dynamicNodes[0]->SetDynamicSearchMethod(list);
        LpVisibilityQuadTree->Release();
        list->Release();
        LpGlobalSearchPlacement = new SearchPlacement;
        LpGlobalSearchPlacement->FirstPlacementItem.LpSearchNode = m_dynamicNodes[0];
        LpGlobalSearchPlacement->FirstPlacementItem.LpSearchMethod = LpVisibilityQuadTree;
    } else {
        m_staticLists = new SearchList *[m_numZones];
        m_dynamicLists = new SearchList *[m_numZones];
        for (i = 0; i < m_numZones; i++) {
            m_staticLists[i] = new SearchList(0x400);
            m_dynamicLists[i] = new SearchList(0x400);
            m_staticNodes[i]->m_x48 = i;
            m_dynamicNodes[i]->m_x48 = i;
            nodes->AddChild(m_staticNodes[i], 0x400);
            nodes->AddChild(m_dynamicNodes[i], 0);
            m_staticNodes[i]->SetStaticSearchMethod(m_staticLists[i]);
            m_staticNodes[i]->SetDynamicSearchMethod(m_staticLists[i]);
            m_dynamicNodes[i]->SetStaticSearchMethod(m_dynamicLists[i]);
            m_dynamicNodes[i]->SetDynamicSearchMethod(m_dynamicLists[i]);
        }
        LpGlobalSearchPlacement = new SearchPlacement;
        LpGlobalSearchPlacement->FirstPlacementItem.LpSearchNode = m_dynamicNodes[0];
        LpGlobalSearchPlacement->FirstPlacementItem.LpSearchMethod = m_staticLists[0];
    }
    return 1;
}


// ---- src/Scene/LoadStaticModels.cpp
// Loads one zone's static models: for each [ZoneN-ModelM] (or [ModelM] under a single
// [StaticModels] block) a buoy, a physics object or a renderable model with its collision, an
// optional smoker and sound emitter; then attached bodies, impact-sound materials and object
// sounds. The VU0 orientation block makes this an optimizer-off function.
// Codegen notes (spellings chosen to reproduce retail):
//  - Heading/Pitch: DegToRad with an inner Deg2RadConst() inline and `c * degrees` give retail's
//    constant numbering and mul operand order.
//  - SetPhysicalProperties' 0.1: a heap-image shim (MwccHeapShim) sets the constant node's
//    has-call flag, as retail's uninitialised heap did.
//  - The block-optimizer event is the `switch (isSmoker) { case 0: break; default: }` form; every
//    plain if/else tried gave none.
//  - `(SoultreeObject *)MODEL.m_object` (a redundant cast) gives retail's offset-first `addu` for
//    the model address and loads m_object before m_collision.
//  - The two `asm(".p2align 0")` lines in the object-sound tail emit nothing; each puts an
//    alignment pcode at the head of the preceding if's join block, so the `bnez emitter` delay
//    slot is filled from the fall-through (not with a clone of `found = 1`) and `beqz onBuoy` is
//    not threaded past the break's `b`, as in retail.


static inline float Half()
{
    float half = 0.5f;
    return half;
}

static inline float Deg2RadConst() { float c = 0.01745329f; return c; }
static inline float DegToRad(float degrees)
{
    float c = Deg2RadConst(); return c * degrees;
}

static inline int Min(int a, int b)
{
    return a < b ? a : b;
}

// Every access goes back through m_zoneModels (retail reloads it each time).
#define MODEL (m_zoneModels[index]->m_models[i])

// Session shim for LoadStaticModels, recalibrated for this TU with r18 image.py: the
// per-function file's three flags plus its 0.0 at +0x16C40, and a 0x30000-byte image so that
// mwcc puts it in a new first heap block, which the function's parse then uses from its start.
struct MwccHeapImage {
    char pad0[0xDAA5];
    char flag0;
    char pad1[0x2777];
    char flag1;
    char pad2[0x69D7];
    char flag2;
    char pad3[0x3F];
    char flag3;
    char tail[0x193CA];
};
#pragma section mwccshim begin
inline void MwccHeapShim() { const MwccHeapImage image = { {0}, 1, {0}, 1, {0}, 1, {0}, 1 }; }
#pragma section mwccshim end
int Scene::LoadStaticModels(ParticleManager *particles, char *path, void (*progress)(int *), int a, int b,
                            ParameterBlock *params, int index, GameObject *object)
{
    static unsigned int WORLDOBJECT_MATERIAL_ID1 = LpCollisionTypeRegistry->GetType("MATERIALID1");
    static unsigned int WORLDOBJECT_MATERIAL_ID2 = LpCollisionTypeRegistry->GetType("MATERIALID2");
    static unsigned int WORLDOBJECT_MATERIAL_ID3 = LpCollisionTypeRegistry->GetType("MATERIALID3");
    static unsigned int WORLDOBJECT_MATERIAL_ID4 = LpCollisionTypeRegistry->GetType("MATERIALID4");
    static unsigned int WORLDOBJECT_MATERIAL_ID5 = LpCollisionTypeRegistry->GetType("MATERIALID5");
    static unsigned int WORLDOBJECT_MATERIAL_ID6 = LpCollisionTypeRegistry->GetType("MATERIALID6");
    int singleZone;
    char text[0x210];
    char zoneBlock[0x110];
    char block[0x50];
    char sltName[0x110];
    char soundFile[0x110];
    _DS3DBUFFER buffer;
    char message[0x210];
    char fileName[0x110];
    char colName[0x110];
    char colPath[0x110];
    char segName[0x110];
    int numModels;
    unsigned int flags;
    float oneShotDistance;
    float randomTriggerPercent;
    int oneShot;
    int force2D;
    int i;
    int j;
    int k;
    int n;
    int found;
    int ok;

    numModels = 0;
    singleZone = 0;
    sprintf(zoneBlock, "Zone%d", index + 1);
    ok = params->ReadParameterBlock(zoneBlock);
    if (!ok && m_numZones == 1) {
        ok = params->ReadParameterBlock("StaticModels");
        singleZone = 1;
    }
    if (!m_info->m_sceneValid || !ok) {
        sprintf(text, "\nCannot find data for [%s] in %s.\n\n", "StaticModels", m_fileName);
        ReportError(text);
        return 0;
    }
    if (!params->GetParameter("NumberOfStaticModels", -1, &numModels)) {
        sprintf(text, "\nNumberOfStaticModels cannot be found under [%s] in %s.\n\n", "StaticModels", m_fileName);
        ReportError(text);
        return 0;
    }
    if (numModels > 0) {
        m_zoneModels[index] = new ZoneModels;
        m_zoneModels[index]->m_numModels = numModels;
        m_zoneModels[index]->m_models = new StaticModel[m_zoneModels[index]->m_numModels];
        for (i = 0; i < m_zoneModels[index]->m_numModels; i++) {
            if (i % a == a - 1 && progress)
                progress(0);
            if (singleZone)
                sprintf(block, "Model%d", i + 1);
            else
                sprintf(block, "Zone%d-Model%d", index + 1, i + 1);
            params->ReadParameterBlock(block);
            if (!params->GetParameter("SLT", "", sltName, 0x104)) {
                // (retail passes the empty name, not "SLT")
                sprintf(message, "\nCannot find %s for [%s] in %s.\n\n", sltName, block, m_fileName);
                ReportError(message);
                return 0;
            }
            _D3DVECTOR position;
            _D3DVECTOR look;
            _D3DVECTOR up;
            _D3DVECTOR unused;
            float heading;
            float pitch;
            int isPhysics;
            int isBuoy;
            int buoy;
            strcpy(segName, sltName);
            strcpy(FindExtension(segName), ".seg");
            int isStadium = 0;
            if (!strcasecmp(m_stadiumFileName, segName))
                isStadium = 1;
            sprintf(fileName, "%s%s", path, sltName);
            strcpy(colName, sltName);
            strcpy(FindExtension(colName), ".col");
            sprintf(colPath, "%s%s", SltGeometryContentDirectory, colName);
            ReadD3DVector(&position, block, "Position", "0.0,0.0,0.0", 0, 0);
            if (params->GetParameter("Heading", -999.0f, &heading)) {
                params->GetParameter("Pitch", 0.0f, &pitch);
                GetOrientationVectors(&look, &up, heading * 0.01745329f, DegToRad(pitch), 0.0f);
            } else {
                ReadD3DVector(&look, block, "LookVector", "0.0,0.0,1.0", 0, 0);
                ReadD3DVector(&up, block, "UpVector", "0.0,1.0,0.0", 0, 0);
            }
            MODEL.m_object = 0;
            MODEL.m_physics = 0;
            MODEL.m_buoy = 0;
            MODEL.m_collision = 0;
            params->GetParameter("PhysicsObject", 0, &isPhysics);
            isBuoy = 0;
            buoy = 0;
            while (!isBuoy && buoy < m_numBuoys) {
                if (!strcasecmp(sltName, m_buoyNames[buoy]))
                    isBuoy = 1;
                else
                    buoy++;
            }
            if (isBuoy) {
                MODEL.m_buoy = new BuoyObject(1);
                MODEL.m_buoy->SetPhysicalProperties(m_buoyOrient[buoy], m_buoyWeight[buoy], m_buoyInertia[buoy],
                                                    m_buoySpring[buoy], m_buoyDamping[buoy], position, look, up);
                strcpy(MODEL.m_name, fileName);
            } else if (isPhysics) {
                float weight;
                float inertiaScale;
                PhysicsObject *physics = new PhysicsObject(1);
                physics->Create(position, look, up, 0, fileName, m_staticNodes[0], m_dynamicNodes[0]);
                AddChild(physics, 2);
                MODEL.m_physics = physics;
                MODEL.m_object = physics->m_object;
                params->GetParameter("Weight", 1.0f, &weight);
                params->GetParameter("InertiaScale", 1.0f, &inertiaScale);
                physics->SetPhysicalProperties(weight, inertiaScale, 1.0f, 0.0f, 0.0f, Half(), 0.5f, 0.1f, 0.0f, 0.0f, 0.0f);
                ReadD3DVector(&physics->m_body->m_centerOfMass, block, "CG", "0.0, 0.0, 0.0", 0, 0);
            } else {
                MODEL.m_object = (SoultreeObject *)(new PSXRenderableObject(1))->Create(fileName, 0, 0);
                if (MODEL.m_object) {
                    object->AddChild(MODEL.m_object, 0);
                    int length = Min(strcspn(fileName, "."), 50);
                    strncpy(MODEL.m_name, fileName, length);
                    MODEL.m_name[length] = 0;
                    MODEL.m_object->SetPosition(position);
                    MODEL.m_object->SetOrientation(&look, &up, 1, 0, 0);
                    MODEL.m_object->AddSearchPlacement(m_staticNodes[index], 1);
                }
                MODEL.m_collision = new CollisionObject(1);
                MODEL.m_collision->Create(13);
                MODEL.m_collision->m_ownerType = WORLDOBJECT_STATICOBJECT;
                MODEL.m_collision->m_owner = MODEL.m_object;
                if (MODEL.m_collision->SetSoultreeCollision((SoultreeObject *)MODEL.m_object, colPath)) {
                    if (MODEL.m_object) {
                        MODEL.m_object->AddChild(MODEL.m_collision, 0);
                        MODEL.m_collision->InitializeTransform();
                    } else {
                        _D3DMATRIX matrix;
                        AddChild(MODEL.m_collision, 0);
                        Vu0OrientMatrix(&matrix, &position, look, up, 0, 1);
                        MODEL.m_collision->InitializeTransform(&matrix);
                    }
                    MODEL.m_collision->AddSearchPlacement(m_dynamicNodes[index], 1);
                } else {
                    MODEL.m_collision->Release();
                    MODEL.m_collision = 0;
                }
            }
            if (b) {
                _D3DVECTOR smokeOffset;
                _D3DVECTOR smokeVelocity;
                int isSmoker;
                float size;
                float growth;
                float variance;
                params->GetParameter("IsSmoker", 0, &isSmoker);
                switch (isSmoker) {
                case 0:
                    break;
                default:
                    ReadD3DVector(&smokeOffset, block, "SmokeOffset", "0.0,0.0,0.0", 0, 0);
                    ReadD3DVector(&smokeVelocity, block, "SmokeVelocity", "0.0,0.0,0.0", 0, 0);
                    params->GetParameter("SmokeSize", 1.0f, &size);
                    params->GetParameter("SmokeGrowth", 1.0f, &growth);
                    params->GetParameter("SmokeVariance", 1.0f, &variance);
                    Smoker *smoker = new Smoker(1);
                    smoker->Create(MODEL.m_object, smokeOffset, smokeVelocity, size, growth, variance);
                    AddChild(smoker, -1);
                    smoker->SetParticleManager(particles);
                    break;
                }
            }
            if (LpGame->m_auralScape) {
                params->GetParameter("SoundResourceFile", "", soundFile, 0x103);
                if (strcmp(soundFile, "") && strcasecmp(soundFile, "NONE")) {
                    ReadSoundEmitterParams(block, soundFile, 1, &buffer, &flags, &oneShotDistance,
                                           &randomTriggerPercent, &oneShot, &force2D);
                    if (!LpGame->m_auralScape->CreateSoultreeEmitter("", MODEL.m_object, m_soundGroup, soundFile,
                                                                     buffer, 1, 10, flags, oneShotDistance,
                                                                     randomTriggerPercent, 0, force2D)) {
                        sprintf(text, "\nScene::SoundEmitter(%s) not created\n", soundFile);
                        ReportError(text);
                    }
                }
            } else {
                sprintf(text, "\nScene:  No AuralScape, so NO Static Model SOUNDS!\n");
                ReportError(text);
            }
        }
        for (i = 0; i < m_zoneModels[index]->m_numModels; i++) {
            char attachedName[0x40];
            int isPhysics;
            int numAttached;
            int attached;
            if (singleZone)
                sprintf(block, "Model%d", i + 1);
            else
                sprintf(block, "Zone%d-Model%d", index + 1, i + 1);
            params->ReadParameterBlock(block);
            params->GetParameter("PhysicsObject", 0, &isPhysics);
            if (isPhysics) {
                params->GetParameter("NumberOfAttachedModels", 0, &numAttached);
                MODEL.m_physics->m_body->CreateAttachedBodyList(numAttached);
                for (j = 0; j < numAttached; j++) {
                    sprintf(attachedName, "AttachedModel%d", j + 1);
                    params->GetParameter(attachedName, 0, &attached);
                    if (attached <= m_zoneModels[index]->m_numModels)
                        MODEL.m_physics->m_body->AddAttachedBody(
                            m_zoneModels[index]->m_models[attached - 1].m_physics->m_body);
                }
            }
            sltName[0] = 0;
            if (m_haveImpactSounds && params->GetParameter("SLT", "", sltName, 0x104)) {
                unsigned int *type;
                found = 0;
                type = 0;
                for (k = 0; k < 6; k++) {
                    for (n = 0; n < m_impactSoundModels[k].m_numItems; n++) {
                        if (!strcasecmp(sltName, m_impactSoundModels[k].GetItem(n))) {
                            if (MODEL.m_collision)
                                type = &MODEL.m_collision->m_ownerType;
                            else if (MODEL.m_buoy)
                                type = &MODEL.m_buoy->m_collisionType;
                            if (type) {
                                switch (k) {
                                case 0:
                                    *type |= WORLDOBJECT_MATERIAL_ID1;
                                    break;
                                case 1:
                                    *type |= WORLDOBJECT_MATERIAL_ID2;
                                    break;
                                case 2:
                                    *type |= WORLDOBJECT_MATERIAL_ID3;
                                    break;
                                case 3:
                                    *type |= WORLDOBJECT_MATERIAL_ID4;
                                    break;
                                case 4:
                                    *type |= WORLDOBJECT_MATERIAL_ID5;
                                    break;
                                case 5:
                                    *type |= WORLDOBJECT_MATERIAL_ID6;
                                    break;
                                }
                                found = 1;
                                break;
                            }
                        }
                    }
                    if (found)
                        break;
                }
            }
            if (m_haveObjectSounds) {
                if (!sltName[0])
                    params->GetParameter("SLT", "", sltName, 0x104);
                if (LpGame->m_auralScape) {
                    found = 0;
                    for (k = 0; k < m_objectSounds.m_numItems; k++) {
                        ObjectSoundData *data = m_objectSounds.GetItem(k);
                        for (n = 0; n < data->m_objects.m_numItems; n++) {
                            if (!strcasecmp(sltName, data->m_objects.GetItem(n))) {
                                SoultreeObject *target = 0;
                                int onBuoy = 0;
                                if (MODEL.m_object)
                                    target = MODEL.m_object;
                                else if (MODEL.m_buoy)
                                    onBuoy = 1;
                                else if (MODEL.m_physics)
                                    target = MODEL.m_physics->m_object;
                                if (target || onBuoy) {
                                    SoultreeSoundEmitter *emitter = LpGame->m_auralScape->CreateSoultreeEmitter(
                                        "", target, m_soundGroup, data->m_name, data->m_buffer, 1, 10, data->m_flags,
                                        data->m_oneShotDistance, data->m_randomTriggerPercent, 0, data->m_force2D);
                                    if (!emitter) {
                                        sprintf(text, "\nScene::SoultreeSoundEmitter(%s) not created\n", data->m_name);
                                        ReportError(text);
                                    }
                                    asm(".p2align 0"); // (no code: see the note at the top)
                                    found = 1;
                                    if (onBuoy)
                                        *(SoultreeSoundEmitter **)&MODEL.m_buoy->m_group = emitter;
                                    asm(".p2align 0");
                                    break;
                                }
                            }
                        }
                        if (found)
                            break;
                    }
                }
            }
        }
        RemoveAllImpactSoundModels();
        RemoveAllObjectSounds();
        ReportError("");
        return 1;
    }
    // (the format has one %s for two arguments)
    sprintf(text, "\nNumber of models<1 in %s.\n\n", "StaticModels", m_fileName);
    ReportError(text);
    return 0;
}


// ---- src/global/AnimationTriggerCallBack.cpp

// A trigger's action 3: start the scene animation (userData).
void AnimationTriggerCallBack(CollisionObject *, void *, void *userData, int entering)
{
    if (entering) {
        SceneAnimation *animation = (SceneAnimation *)userData;
        animation->m_automaticSequence = 1;
        animation->m_character->m_x10 = 0;
    }
}


// ---- src/global/EnableTriggerCallBack.cpp

// A trigger's action 4: let this trigger's owner set off (or stop it setting
// off) another trigger (userData is the EnableTrigger).
void EnableTriggerCallBack(CollisionObject *object, void *, void *userData, int active)
{
    if (!active)
        return;
    EnableTrigger *data = (EnableTrigger *)userData;
    int i = 0;
    if (data->m_enable) {
        // Find a free slot or this trigger's own (retail tests the index last).
        while (data->m_target->m_filter[i] && data->m_target->m_filter[i] != object->m_owner && i < 9)
            i++;
#line 2782 "SceneManager.cpp"
        ASSERT(i < 9, "");
        data->m_target->m_filter[i] = object->m_owner;
    } else {
        while (data->m_target->m_filter[i] != object->m_owner && i < 9)
            i++;
        if (i < 9)
            data->m_target->m_filter[i] = 0;
    }
}


// ---- src/Scene/ReadTriggerData.cpp


// Reads the [Triggers] block: per trigger a box (TriggerObject) attached to
// a static model or an animation, and its actions (1 wake a physics object,
// 2 scoring, 3 start an animation, 4 enable/disable another trigger).
int Scene::ReadTriggerData()
{
    char msg[0x100];
    char name[0x40];
    int numTriggers;
    int numPhysicsTriggers;
    int numScoring;
    int numAnimation;
    int numEnable;
    int modelNumber;
    int animationNumber;
    int numActions;
    int initialZone;
    float radius;
    float yaw;
    float pitch;
    float roll;
    int actionType;
    int model;
    int animation;
    int animationTrigger;
    int enable;
    int excludeAI;
    int scoringIndex = 0;
    int animationIndex = 0;
    int enableIndex = 0;

    int found = m_params->ReadParameterBlock("Triggers");
    if (!m_info->m_sceneValid || !found) {
        sprintf(msg, "\nCannot find data for [%s] in %s.\n\n", "Triggers", m_fileName);
        ReportError(msg);
        return 0;
    }
    m_params->GetParameter("NumberOfTriggers", 0, &numTriggers);
    m_params->GetParameter("NumberOfPhysicsObjectTriggers", 0, &numPhysicsTriggers);
    m_params->GetParameter("NumberOfScoringTriggers", 0, &numScoring);
    m_params->GetParameter("NumberOfAnimationTriggers", 0, &numAnimation);
    m_params->GetParameter("NumberOfEnableTriggers", 0, &numEnable);
    m_numScoringTriggers = numScoring;
    if (m_numScoringTriggers > 0)
        m_scoringTriggers = new ScoringTrigger[numScoring];
    m_numAnimationTriggers = numAnimation;
    if (m_numAnimationTriggers > 0)
        m_animationTriggers = new AnimationTrigger[numAnimation];
    m_numEnableTriggers = numEnable;
    if (m_numEnableTriggers > 0)
        m_enableTriggers = new EnableTrigger[numEnable];
    for (int i = 0; i < numTriggers; i++) {
        sprintf(name, "Trigger%d", i + 1);
        if (!m_params->ReadParameterBlock(name))
            continue;
        _D3DVECTOR position;
        _D3DVECTOR box;
        _D3DVECTOR forward;
        _D3DVECTOR up;
        m_params->GetParameter("InitialZone", 1, &initialZone);
        ReadD3DVector(&position, name, "Position", "0.0, 0.0, 0.0", 0, 0);
        ReadD3DVector(&box, name, "Box", "0.0, 0.0, 0.0", 0, 0);
        m_params->GetParameter("Yaw", 0.0f, &yaw);
        m_params->GetParameter("Pitch", 0.0f, &pitch);
        m_params->GetParameter("Roll", 0.0f, &roll);
        GetOrientationVectors(&forward, &up, 0.01745329f * yaw, 0.01745329f * pitch, 0.01745329f * roll);
        m_params->GetParameter("Radius", 0.0f, &radius);
        m_params->GetParameter("AttachedModelNumber", 0, &modelNumber);
        m_params->GetParameter("AttachedAnimationNumber", 0, &animationNumber);
        m_params->GetParameter("NumberOfActions", 0, &numActions);
        TriggerObject *trigger = new TriggerObject(1);
        trigger->Create(position, box, forward, up, radius, numActions);
        AddChild(trigger, 10);
        if (modelNumber) {
            if (m_zoneModels[initialZone]->m_models[modelNumber - 1].m_physics)
                trigger->m_parent = m_zoneModels[initialZone]->m_models[modelNumber - 1].m_physics->m_object;
            else if (m_zoneModels[initialZone]->m_models[modelNumber - 1].m_object)
                trigger->m_parent = m_zoneModels[initialZone]->m_models[modelNumber - 1].m_object;
        } else if (m_animations && animationNumber) {
            if (m_animations->m_animations[animationNumber - 1].m_character)
                trigger->m_parent = m_animations->m_animations[animationNumber - 1].m_character->m_root->m_child;
            else if (m_animations->m_animations[animationNumber - 1].m_car)
                trigger->m_parent = m_animations->m_animations[animationNumber - 1].m_car->m_model;
            else if (m_animations->m_animations[animationNumber - 1].m_boat)
                trigger->m_parent = m_animations->m_animations[animationNumber - 1].m_boat->m_model;
        }
        for (int j = 0; j < numActions; j++) {
            char actionName[0x44];      // 0x44: the frame has no other gap there
            sprintf(actionName, "%sAction%d", name, j + 1);
            if (m_params->ReadParameterBlock(actionName)) {
                m_params->GetParameter("ActionType", 0, &actionType);
                switch (actionType) {
                case 1:
                    m_params->GetParameter("ModelNumber", 0, &model);
                    if (model > 0) {
                        ZoneModels *zone = m_zoneModels[initialZone];
                        if (model <= zone->m_numModels && zone->m_models[model - 1].m_physics)
                            trigger->AddCallback(zone->m_models[model - 1].m_physics, PhysicsObjectTriggerCallBack);
                    }
                    break;
                case 2:
                    if (scoringIndex < numScoring) {
                        m_params->GetParameter("Points", 0.0f, &m_scoringTriggers[scoringIndex].m_points);
                        m_params->GetParameter("BonusMultiplier", 0.0f,
                                               &m_scoringTriggers[scoringIndex].m_bonusMultiplier);
                        m_scoringTriggers[scoringIndex].m_trigger = trigger;
                        scoringIndex++;
                    }
                    break;
                case 3:
                    if (animationIndex < numAnimation) {
                        m_params->GetParameter("TimeTillReset", 0.0f, &m_animationTriggers[animationIndex].m_timeTillReset);
                        m_params->GetParameter("OneTimeOnly", 0, &m_animationTriggers[animationIndex].m_oneTimeOnly);
                        m_animationTriggers[animationIndex].m_trigger = trigger;
                        animationIndex++;
                        m_params->GetParameter("AnimationNumber", 0, &animation);
                        if (animation > 0 && animation <= m_animations->m_count &&
                            m_animations->m_animations[animation - 1].m_character) {
                            trigger->AddCallback(&m_animations->m_animations[animation - 1], AnimationTriggerCallBack);
                            trigger->AddSearchPlacement(m_dynamicNodes[initialZone - 1]);
                            m_animations->m_animations[animation - 1].m_automaticSequence = 0;
                        }
                    }
                    break;
                case 4:
                    if (enableIndex < numEnable) {
                        m_params->GetParameter("TimeTillReset", 0.0f, &m_enableTriggers[enableIndex].m_timeTillReset);
                        m_params->GetParameter("OneTimeOnly", 0, &m_enableTriggers[enableIndex].m_oneTimeOnly);
                        m_params->GetParameter("AnimationTriggerNumber", 0, &animationTrigger);
                        m_params->GetParameter("EnableTrigger", 1, &enable);
                        m_params->GetParameter("ExcludeAI", 0, &excludeAI);
                        m_enableTriggers[enableIndex].m_trigger = trigger;
                        if (animationTrigger > 0 && animationTrigger <= numAnimation) {
                            m_enableTriggers[enableIndex].m_target = m_animationTriggers[animationTrigger - 1].m_trigger;
                            m_animationTriggers[animationTrigger - 1].m_trigger->m_filtered = 1;
                            m_enableTriggers[enableIndex].m_enable = enable;
                            m_enableTriggers[enableIndex].m_excludeAI = excludeAI;
                            trigger->AddCallback(&m_enableTriggers[enableIndex], EnableTriggerCallBack);
                            trigger->AddSearchPlacement(m_dynamicNodes[initialZone - 1]);
                            // Retail indexes by trigger number, not enableIndex.
                            m_enableTriggers[i].m_trigger->m_collision->SetTestListType((CollisionObjectTestListType)1);
                        }
                        enableIndex++;
                    }
                    break;
                default:
                    sprintf(msg, "\nInvalid action type %d in %s.\n\n", actionType, m_fileName);
                    ReportError(msg);
                    break;
                }
            } else {
                PSXprintf("ERROR: ReadTriggerData could not find %s\n", actionName);
            }
        }
    }
    return 1;
}


// ---- src/Scene/ReadAnimationData.cpp
// What retail needs: the 12th argument's register must take the 0.0f materialisation itself,
// and the values for arguments 13/14 must be copies made before FunctionCalls emits the
// parameter moves (they are emitted strictly in parameter order). The source that does this:
// `y1 = y2 = 0.0f, y1--, y2--` for the three zero arguments. The chained assignment passes the
// mtc1's own register to argument 12 (and copies it into y1/y2); each postfix `--` makes a
// fresh copy of y1/y2 when the argument is generated, which is the early copy. The decrements
// are dead and disappear. Plain `0.0f` (or y1, y2, named zeros, inline wrappers, casts...) is
// CSE'd into one shared temp and gives 16 words. Arguments are unsequenced here, but mwcc
// evaluates the assignment first, so all three are 0. The 1.0f argument still goes through One
// (see below).





// The 1.0f argument as a call: an argument containing a call is evaluated first.
static inline float One()
{
    float one = 1.0f;
    return one;
}


static inline void SetExtension(char *name, char *extension)
{
    char *ext;
    if ((ext = FindExtension(name)))
        strcpy(ext, extension);
    else
        strcat(name, extension);
}

// Every access goes back through m_animations (retail reloads it each time).
#define ANIM (m_animations->m_animations[i])

int Scene::ReadAnimationData(void (*progress)(int *), int a)
{
    char text[0x210];
    char name[0x110];
    char message[0x210];
    char savedSltDir[0x110];
    char savedColDir[0x110];
    char block[0x40];
    char vueName[0x40];
    char fileName[0x110];
    char colName[0x110];
    int numAnims;
    int i;
    int j;
    numAnims = 0;
    int found = m_params->ReadParameterBlock("Animations");
    if (!m_info->m_sceneValid || !found) {
        sprintf(text, "\nCannot find data for [%s] in %s.\n\n", "Animations", m_fileName);
        ReportError(text);
        return 0;
    }
    if (!m_params->GetParameter("NumberOfAnimations", -1, &numAnims)) {
        sprintf(text, "\nNumberOfAnimations cannot be found under [%s] in %s.\n\n", "Animations", m_fileName);
        ReportError(text);
        return 0;
    }
    if (numAnims > 0) {
        m_animations = new SceneAnimations;
        m_animations->m_count = numAnims;
        ReportSuccess("Animations", "NumberOfAnimations", numAnims);
        m_animations->m_animations = (SceneAnimation *)new char[numAnims * sizeof(SceneAnimation)];
        m_animations->m_numRandomSets = 0;
        m_animations->m_randomSets = 0;
        for (i = 0; i < m_animations->m_count; i++) {
            float fps;
            float lagDistance;
            int frontWheelsTurn;
            int pathIsLevel;
            int driveOnSoultrees;
            int numberOfTires;
            int zone;
            int numWakePoints;
            *&ANIM.m_position = ZeroVector;
            ANIM.m_character = 0;
            ANIM.m_car = 0;
            ANIM.m_boat = 0;
            *(int *)&ANIM.m_time = 0;
            ANIM.m_numMotions = 0;
            ANIM.m_motions = 0;
            ANIM.m_motion = 0;
            ANIM.m_numSequences = 0;
            ANIM.m_sequence = 0;
            ANIM.m_inRandomSet = 0;
            ANIM.m_sequences = 0;
            ANIM.m_name[0] = 0;
            ANIM.m_automaticSequence = 0;
            ANIM.m_loopingSequence = 0;
            ANIM.m_isCharacter = 0;
            ANIM.m_isCar = 0;
            ANIM.m_isBoat = 0;
            ANIM.m_excludeFromCamera = 0;
            ANIM.m_x45_bit7 = 0;
            if (i % a == a - 1 && progress)
                progress(0);
            driveOnSoultrees = 1;
            sprintf(block, "Animation%d", i + 1);
            m_params->ReadParameterBlock(block);
            if (!m_params->GetParameter("AnimationType", "", text, 0x40)) {
                sprintf(message, "\nScene: AnimationType not specified for anim#%d.  Using KeyFramed.\n", i);
                ReportError(message);
                ANIM.m_isCharacter = 1;
            } else {
                if (!strncasecmp(text, "Procedural", 3)) {
                    ANIM.m_isCharacter = 0;
                    if (!m_params->GetParameter("VUE", "", vueName, 0x3F)) {
                        sprintf(message, "\nCannot find %s for [%s] in %s.\n\n", "VUE", block, m_fileName);
                        ReportError(message);
                        return 0;
                    }
                    ReportSuccess(block, "VUE", vueName);
                    if (!m_params->GetParameter("ProceduralType", "Car", text, 0x40))
                        ReportSuccess(block, "ProceduralType (default)", "Car");
                    else
                        ReportSuccess(block, "ProceduralType", "Boat");     // sic
                    if (!strncasecmp(text, "Car", 3)) {
                        ANIM.m_isCar = 1;
                        ANIM.m_isBoat = 0;
                    } else if (!strncasecmp(text, "Boat", 3)) {
                        ANIM.m_isBoat = 1;
                        ANIM.m_isCar = 0;
                    }
                    if (!m_params->GetParameter("FPS", 30.0f, &fps))
                        ReportSuccess(block, "FPS(Default)", fps);
                    else
                        ReportSuccess(block, "FPS", fps);
                    if (!m_params->GetParameter("LagDistance", 0.0f, &lagDistance))
                        ReportSuccess(block, "LagDistance(Default)", lagDistance);
                    else
                        ReportSuccess(block, "LagDistance", lagDistance);
            if (ANIM.m_isCar) {
                        if (!m_params->GetParameter("FrontWheelsTurn", 1, &frontWheelsTurn)) {
                            ReportSuccess(block, "FrontWheelsTurn(Default)", frontWheelsTurn);
                            frontWheelsTurn = lagDistance == 0.0f;
                        } else {
                            ReportSuccess(block, "FrontWheelsTurn", frontWheelsTurn);
                        }
                        if (!m_params->GetParameter("PathIsLevel", 0, &pathIsLevel))
                            ReportSuccess(block, "PathIsLevel(Default)", pathIsLevel);
                        else
                            ReportSuccess(block, "PathIsLevel", pathIsLevel);
                        if (!m_params->GetParameter("NumberOfTires", 4, &numberOfTires))
                            ReportSuccess(block, "NumberOfTires(Default)", numberOfTires);
                        else
                            ReportSuccess(block, "NumberOfTires", numberOfTires);
                        if (!m_params->GetParameter("DriveOnSoultrees", 1, &driveOnSoultrees))
                            ReportSuccess(block, "DriveOnSoultrees(Default)", driveOnSoultrees);
                        else
                            ReportSuccess(block, "DriveOnSoultrees", driveOnSoultrees);
                    } else {
                        if (!m_params->GetParameter("FrontWheelsTurn", 0, &frontWheelsTurn)) {
                            ReportSuccess(block, "FrontWheelsTurn(Default)", frontWheelsTurn);
                            frontWheelsTurn = lagDistance == 0.0f;
                        } else {
                            ReportSuccess(block, "FrontWheelsTurn", frontWheelsTurn);
                        }
                        if (!m_params->GetParameter("PathIsLevel", 1, &pathIsLevel))
                            ReportSuccess(block, "PathIsLevel(Default)", pathIsLevel);
                        else
                            ReportSuccess(block, "PathIsLevel", pathIsLevel);
                        if (!m_params->GetParameter("NumberOfTires", 0, &numberOfTires))
                            ReportSuccess(block, "NumberOfTires(Default)", numberOfTires);
                        else
                            ReportSuccess(block, "NumberOfTires", numberOfTires);
                        if (!m_params->GetParameter("DriveOnSoultrees", 0, &driveOnSoultrees))
                            ReportSuccess(block, "DriveOnSoultrees(Default)", driveOnSoultrees);
                        else
                            ReportSuccess(block, "DriveOnSoultrees", driveOnSoultrees);
                    }
                } else {
                    ANIM.m_isCharacter = 1;
                }
                numWakePoints = 0;
                m_params->GetParameter("NumberOfWakePoints", 0, &numWakePoints);
                if (numWakePoints > 0) {
                    // A plain allocation: SceneWakePoint's vector would make `new[]` construct them.
                    ANIM.m_wakePoints = (SceneWakePoint *)new char[numWakePoints * sizeof(SceneWakePoint)];
                    for (j = 0; j < numWakePoints; j++) {
                        sprintf(text, "WakePoint%dMotion", j);
                        m_params->GetParameter(text, 0, &ANIM.m_wakePoints[j].m_motion);
                        sprintf(text, "WakePoint%dFrame", j);
                        m_params->GetParameter(text, 0, &ANIM.m_wakePoints[j].m_frame);
                        sprintf(text, "WakePoint%dForce", j);
                        m_params->GetParameter(text, 50.0f, &ANIM.m_wakePoints[j].m_force);
                        ReadD3DVector(&ANIM.m_wakePoints[j].m_offset, block, text, "0.0,0.0,0.0", 0, 0);
                    }
                }
                ANIM.m_numWakePoints = numWakePoints;
            }
            ANIM.m_x45_bit7 = 0;
            if (ANIM.m_isCharacter) {
                if (!m_params->GetParameter("MCF", "", fileName, 0x104)) {
                    sprintf(message, "\nCannot find %s for [%s] in %s.\n\n", "MCF", block, m_fileName);
                    ReportError(message);
                    return 0;
                }
            }
            if (!ANIM.m_isCharacter) {
                if (!m_params->GetParameter("SLT", "", fileName, 0x104)) {
                    sprintf(message, "\nCannot find %s for [%s] in %s.\n\n", "SLT", block, m_fileName);
                    ReportError(message);
                    return 0;
                }
            }
            sprintf(name, "%s", fileName);
            strcpy(fileName, name);
            int length = Min(strcspn(fileName, "."), 20);
            strncpy(ANIM.m_name, fileName, length);
            ANIM.m_name[length] = 0;
            strcpy(colName, name);
            SetExtension(colName, ".col");
            if (!ReadD3DVector(&ANIM.m_position, block, "Position", "0.0,0.0,0.0", 0, 0))
                ReadD3DVector(&ANIM.m_position, block, "Offset", "0.0,0.0,0.0", 0, 0);
            if (!ANIM.m_isCharacter) {
                strcpy(savedSltDir, SltGeometryContentDirectory);
                strcpy(savedColDir, CollisionContentDirectory);
                strcpy(SltGeometryContentDirectory, AnimContentDirectory);
                strcpy(CollisionContentDirectory, AnimContentDirectory);
            }
            if (ANIM.m_isCharacter) {
                if (!AddChild((ANIM.m_character = new CollisionCharacter(1))->Create(name, colName, 1, 1, 1), 2)) {
                    sprintf(message, "\nScene: Cannot create character from %s.\n\n", fileName);
                    ReportError(message);
                    return 0;
                }
            }
            if (ANIM.m_isCar) {
                float y1, y2; // only to reproduce retail's zero-argument copies (see the header)
                if (!AddChild((GameObject *)PS2_ADDR((ANIM.m_car = new CarProcedural(1))
                                  ->Create(name, colName, 0, vueName, &ANIM.m_position, fps, pathIsLevel,
                                           frontWheelsTurn, lagDistance, One(), numberOfTires, y1 = y2 = 0.0f, y1--, y2--,
                                           driveOnSoultrees, 0)), 2)) {
                    sprintf(message, "\nScene: Cannot create procedural model from %s.\n\n", fileName);
                    ReportError(message);
                    return 0;
                }
            }
            if (ANIM.m_isBoat) {
                float y1, y2; // as above
                if (!AddChild((GameObject *)PS2_ADDR((ANIM.m_boat = new BoatProcedural(1))
                                  ->Create(name, colName, 0, vueName, &ANIM.m_position, fps, pathIsLevel,
                                           frontWheelsTurn, lagDistance, One(), numberOfTires, y1 = y2 = 0.0f, y1--, y2--,
                                           driveOnSoultrees, 0)), 2)) {
                    sprintf(message, "\nScene: Cannot create procedural model from %s.\n\n", fileName);
                    ReportError(message);
                    return 0;
                }
            }
            _D3DVECTOR look;
            _D3DVECTOR up;
            int useLighting;
            float heading;
            float pitch;
            int oriented;
            int exclude;
            if (!ANIM.m_isCharacter) {
                strcpy(SltGeometryContentDirectory, savedSltDir);
                strcpy(CollisionContentDirectory, savedColDir);
            }
            m_params->GetParameter("UseLighting", 1, &useLighting);
            oriented = 0;
            if (m_params->GetParameter("Heading", -999.0f, &heading)) {
                m_params->GetParameter("Pitch", 0.0f, &pitch);
                GetOrientationVectors(&look, &up, heading * 0.01745329f, pitch * 0.01745329f, 0.0f);
                oriented = 1;
            } else if (ReadD3DVector(&look, block, "LookVector", "0.0,0.0,1.0", 0, 0)) {
                ReadD3DVector(&up, block, "UpVector", "0.0,1.0,0.0", 0, 0);
                oriented = 1;
            }
            m_params->GetParameter("ExcludeFromCameraIteration", 0, &exclude);
            m_params->GetParameter("InitialZone", 1, &zone);
            ANIM.m_excludeFromCamera = exclude;
            if (ANIM.m_isCharacter) {
                char motionKey[0x50];
                char motionName[0x20];
                char sequence[0x80];
                char number[0x20];
                int flag;
                ANIM.m_character->m_root->m_x190_bit3 = 1;
                ANIM.m_character->m_root->m_x190_bit4 = 1;
                ReportSuccess(block, "MCF", fileName);
                ANIM.m_character->m_root->SetPosition(&ANIM.m_position);
                if (oriented)
                    ANIM.m_character->m_root->SetOrientation(&look, &up, 1, 0, 0);
                ANIM.m_character->m_collision->InitializeTransform();
                m_params->GetParameter("NumberOfMotions", 0, &ANIM.m_numMotions);
                ANIM.m_motions = 0;
                ANIM.m_sequences = 0;
                if (ANIM.m_numMotions > 0) {
                    ANIM.m_motions = new Motion *[ANIM.m_numMotions];
                    for (int m = 0; m < ANIM.m_numMotions; m++) {
                        sprintf(motionKey, "MotionToPlay%d", m + 1);
                        if (!m_params->GetParameter(motionKey, "", motionName, 0x10)) {
                            sprintf(message, "\nCannot find %s for [%s] in %s.\n\n", motionKey, block, m_fileName);
                            ReportError(message);
                            return 0;
                        }
                        ReportSuccess(block, motionKey, motionName);
                        ANIM.m_motions[m] = LpMotionLibrary->MotionOfName(motionName, 1);
                    }
                    if (m_params->GetParameter("MotionSequence", "", sequence, 0x40)) {
                        char *token = strtok(sequence, ",");
                        j = 0;
                        m_params->GetParameter("NumberInSequence", "", number, 0x10);
                        ANIM.m_numSequences = atoi(number);
                        if (ANIM.m_numSequences) {
                            ANIM.m_sequences = new signed char[ANIM.m_numSequences];
                            while (token && j < ANIM.m_numSequences) {
                                ANIM.m_sequences[j] = atoi(token);
                                token = strtok(0, ",");
                                j++;
                            }
                            m_params->GetParameter("AutomaticSequence", 1, &flag);
                            ANIM.m_automaticSequence = flag;
                            m_params->GetParameter("LoopingSequence", 0, &flag);
                            ANIM.m_loopingSequence = flag;
                            ANIM.m_sequence = ANIM.m_sequences[0];
                            if (ANIM.m_sequence >= ANIM.m_numSequences)
                                ANIM.m_sequence = 0;
                        } else {
                            ANIM.m_sequence = 0;
                            sprintf(message, "\n%s for [%s] in %s is ZERO!\n\n", "NumberInSequence", block, m_fileName);
                            ReportError(message);
                            return 0;
                        }
                        ANIM.m_motion = ANIM.m_sequence;
                    } else {
                        ANIM.m_motion = 0;
                        ANIM.m_numSequences = 0;
                        sprintf(message, "\n%s for [%s] in %s is ZERO!\n\n", "MotionSequence", block, m_fileName);
                        ReportError(message);
                        return 0;
                    }
                    RewindAnimation(i, 0.0f, 0, 0);
                }
            } else {
                ReportSuccess(block, "SLT", name);
                ANIM.m_numMotions = 1;
                if (oriented) {
                    if (ANIM.m_isCar) {
                        ANIM.m_car->m_model->SetOrientation(&look, &up, 1, 0, 0);
                        ANIM.m_car->m_collision->InitializeTransform();
                    } else {
                        ANIM.m_boat->m_model->SetOrientation(&look, &up, 1, 0, 0);
                        ANIM.m_boat->m_collision->InitializeTransform();
                    }
                }
            }
            if (ANIM.m_isCharacter) {
                ANIM.m_character->ApplyDefaultFrame();
                ANIM.m_character->m_collision->AddSearchPlacement(m_dynamicNodes[zone - 1], 2);
                ANIM.m_character->m_root->AddSearchPlacement(m_staticNodes[zone - 1], 2);
                ANIM.m_character->m_root->Hide();
                ANIM.m_character->m_collision->Disable();
            }
            if (ANIM.m_numMotions > 0) {
                char soundFile[0x110];
                _DS3DBUFFER buffer;
                char attachName[0x90];
                unsigned int flags;
                float oneShotDistance;
                float randomTriggerPercent;
                int oneShot;
                int force2D;
                if (LpGame->m_auralScape) {
                    m_params->GetParameter("SoundResourceFile", "", soundFile, 0x103);
                    if (strcmp(soundFile, "") && strcasecmp(soundFile, "NONE")) {
                        ReadSoundEmitterParams(block, soundFile, 0, &buffer, &flags, &oneShotDistance,
                                               &randomTriggerPercent, &oneShot, &force2D);
                        m_params->GetParameter("SoundAttachToObject", "", attachName, 0x40);
                        if (!LpGame->m_auralScape->CreateSoultreeEmitter(
                                attachName,
                                ANIM.m_isCharacter ? ANIM.m_character->m_root
                                                   : (ANIM.m_isCar ? ANIM.m_car->m_model
                                                                  : ANIM.m_boat->m_model),
                                m_soundGroup, soundFile, buffer, 1, 10, flags, oneShotDistance, randomTriggerPercent, 0,
                                force2D)) {
                            sprintf(text, "\nScene::SoundEmitter(%s) not created\n", soundFile);
                            ReportError(text);
                        }
                    }
                } else {
                    sprintf(text, "\nScene:  No AuralScape, so NO Animation SOUNDS!\n");
                    ReportError(text);
                }
            }
        }
        m_params->ReadParameterBlock("Animations");
        m_params->GetParameter("NumberOfRandomSets", 0, &m_animations->m_numRandomSets);
        if (m_animations->m_numRandomSets > 0) {
            char setBlock[0x50];
            char setString[0x80];
            //PB
            int k;
            char *token;
            int dot;
            int n;
            //PE
            m_animations->m_randomSets = new RandomSet[m_animations->m_numRandomSets];
            for (k = 0; k < m_animations->m_numRandomSets; k++) {
                m_animations->m_randomSets[k].m_count = 0;
                m_animations->m_randomSets[k].m_animations = 0;
                m_animations->m_randomSets[k].m_anims = 0;
                m_animations->m_randomSets[k].m_motions = 0;
                sprintf(setBlock, "RandomSet%d", k + 1);
                if (!m_params->ReadParameterBlock(setBlock)) {
                    sprintf(message, "\nCannot find [%s] in %s.\n\n", setBlock, m_fileName);
                    ReportError(message);
                    return 0;
                }
                if (!m_params->GetParameter("RandomSet", "", setString, 0x80)) {
                    sprintf(message, "\nCannot find %s for [%s] in %s.\n\n", "RandomSet", setBlock, m_fileName);
                    ReportError(message);
                    return 0;
                }
                token = strtok(setString, ",");
                n = 0;
                m_animations->m_randomSets[k].m_animations = 0;
                // Reads the count as a string into the count byte (sic).
                m_params->GetParameter("NumberInSequence", 0, (char *)&m_animations->m_randomSets[k].m_count, -1);
                if (!m_animations->m_randomSets[k].m_count)
                    continue;
                m_animations->m_randomSets[k].m_anims = new signed char[m_animations->m_randomSets[k].m_count];
                m_animations->m_randomSets[k].m_motions = new signed char[m_animations->m_randomSets[k].m_count];
                while (token && n < m_animations->m_randomSets[k].m_count) {
                    dot = strcspn(token, ".");
                    if (dot && dot != strlen(token)) {
                        token[dot] = 0;
                        m_animations->m_randomSets[k].m_anims[n] = atoi(token) - 1;
                        m_animations->m_animations[m_animations->m_randomSets[k].m_anims[n]].m_inRandomSet = 1;
                        // Tests animation k, the set index, not the set's animation (sic).
                        if (m_animations->m_animations[k].m_isCharacter)
                            m_animations->m_animations[m_animations->m_randomSets[k].m_anims[n]].m_character->Disable();
                        else if (m_animations->m_animations[k].m_isCar)
                            m_animations->m_animations[m_animations->m_randomSets[k].m_anims[n]].m_car->Disable();
                        else
                            m_animations->m_animations[m_animations->m_randomSets[k].m_anims[n]].m_boat->Disable();
                        m_animations->m_randomSets[k].m_motions[n] = atoi(token + dot + 1) - 1;
                        token = strtok(0, ",");
                        n++;
                    } else {
                        sprintf(message, "\nInvalid Animation or Motion #%d in [%s] in %s.\n\n", n, setBlock, m_fileName);
                        ReportError(message);
                        return 0;
                    }
                }
                m_animations->m_randomSets[k].m_animations = m_animations->m_animations;
            }
        } else {
            m_animations->m_randomSets = 0;
        }
        ReportError("");
    } else {
        sprintf(text, "\nNumber of animations<1 in %s.\n\n", "Animations", m_fileName);
        ReportError(text);
        return 0;
    }
    return 1;
}


// ---- src/Scene/ReadSoundData.cpp
// ReadSoundData__5SceneFP10GameObject  @ 0x0028F9A0
// The Object%d name loops are two inline members: ReadObjectNames (dynamic object sounds)
// and ReadImpactNames, which takes the impact index and declares `list = &m_impactSounds[index]`
// after its counter. That makes list an inline @temp numbered between the counter and the
// malloc result, which gives retail's colouring (list in $s2 with k, counter in $s3); a named
// list or a list parameter (CSE/LICM temp) put them the other way round.


// Reads the Object1, Object2, ... names of one sound into a list.
inline void Scene::ReadObjectNames(char *key, char *objectName, ContainerList<char *> *list, int *flag)
{
    int n = 1;
    for (;;) {
        sprintf(key, "Object%d", n);
        if (!m_params->GetParameter(key, "", objectName, 0x3F))
            break;
        char *name = (char *)malloc(0x40);
        strcpy(name, objectName);
        list->AddItem(name);
        *flag = 1;
        n++;
    }
}

inline void Scene::ReadImpactNames(char *key, char *objectName, int index)
{
    int n = 1;
    ContainerList<char *> *list = &m_impactSoundModels[index];
    for (;;) {
        sprintf(key, "Object%d", n);
        if (!m_params->GetParameter(key, "", objectName, 0x3F))
            break;
        char *name = (char *)malloc(0x40);
        strcpy(name, objectName);
        list->AddItem(name);
        m_haveImpactSounds = 1;
        n++;
    }
}

int Scene::ReadSoundData(GameObject *)
{
    char message[0x210];
    char block[0x210];
    char position[0x80];
    char fileName[0x110];
    char soundType[0x110];
    unsigned int flags;
    float oneShotDistance;
    float randomTriggerPercent;
    int oneShot;
    int force2D;
    int volume;
    int numStatic;
    int numDynamic;
    AmbientListSoundEmitter *ambient;
    int i;
    PSXSound *sound;
    ObjectSoundData *data;
    int k;
    int d;
    if (!m_info->m_sceneValid)
        return 0;
    m_params->ReadParameterBlock("Sounds");
    m_params->GetParameter("CrowdPresent", 0, &m_crowdPresent);
    ReportSuccessBOOL("Sounds", "CrowdPresent", m_crowdPresent);
    ReportError("");
    _DS3DBUFFER buffer;
    m_params->GetParameter("NumStaticSounds", 0, &numStatic);
    m_params->GetParameter("NumDynamicSounds", 0, &numDynamic);
    if (numStatic > 0) {
        ambient = 0;
        sprintf(position, "%f %f %f", 3.40282347e38f, 3.40282347e38f, 3.40282347e38f);
        for (i = 1; i <= numStatic; i++) {
            sprintf(block, "StaticSound%d", i);
            m_params->ReadParameterBlock(block);
            if (LpGame->m_auralScape) {
            if (!m_params->GetParameter("SoundResourceFile", "", fileName, 0x103))
                continue;
            if (!strcmp(fileName, "") || !strcasecmp(fileName, "NONE"))
                continue;
            m_params->GetParameter("SoundType", "Emitter", soundType, 0x103);
            if (!strcasecmp(soundType, "AmbientList")) {
                ReadSoundEmitterParams(block, fileName, 1, &buffer, &flags, &oneShotDistance, &randomTriggerPercent,
                                       &oneShot, &force2D);
                if (!ambient)
                    ambient = (new AmbientListSoundEmitter(LpGame->m_auralScape, m_soundGroup, 1, m_enabled))
                                  ->Create(buffer, flags, oneShotDistance, randomTriggerPercent, 0, force2D);
                ambient->LoadSoundIntoList(fileName);
            } else if (!strcasecmp(soundType, "Emitter")) {
                ReadSoundEmitterParams(block, fileName, 1, &buffer, &flags, &oneShotDistance, &randomTriggerPercent,
                                       &oneShot, &force2D);
                if (!ReadD3DVector(&buffer.vPosition, block, "Position", position, 1, 0)) {
                    sprintf(message, "Position not available for StaticSound%d\n", i);
                    ReportError(message);
                }
                if (!LpGame->m_auralScape->CreateSoundEmitter(m_soundGroup, fileName, buffer, 1, 10, flags,
                                                              oneShotDistance, randomTriggerPercent, 0, force2D)) {
                    sprintf(message, "\nScene::SoundEmitter(%s) not created\n", fileName);
                    ReportError(message);
                }
            } else if (!strcasecmp(soundType, "Sound")) {
                if (!strstr(fileName, ".wav"))
                    strcat(fileName, ".wav");
                sound = LpGame->m_auralScape->m_soundInterface->GetSoundFromResourceName(m_soundGroup,
                                                                                                   fileName, 1, 1);
                m_params->GetParameter("SoundVolume", 100, &volume);
                sound->SetVolume(volume);
                m_ambientSounds.AddItem(sound);
            }
            } else {
                sprintf(message, "\nScene:  No AuralScape, so NO Static Model SOUNDS!\n");
                ReportError(message);
            }
        }
        ReportError("");
        if (ambient)
            AddChild(ambient, 2);
    } else {
        sprintf(message, "\nNumber of StaticSounds<1 in %s.\n\n", m_fileName);
        ReportError(message);
    }
    if (numDynamic > 0) {
        char objectName[0x40];
        char key[0x10];
        sprintf(position, "%f %f %f", 3.40282347e38f, 3.40282347e38f, 3.40282347e38f);
        for (d = 1; d <= numDynamic; d++) {
            sprintf(block, "DynamicSound%d", d);
            m_params->ReadParameterBlock(block);
            if (!m_params->GetParameter("SoundType", "ObjectSound", soundType, 0x103))
                continue;
            if (!strcasecmp(soundType, "ObjectSound")) {
                m_params->GetParameter("SoundResourceFile", "", fileName, 0x103);
                if (!strcmp(fileName, "") || !strcasecmp(fileName, "NONE"))
                    continue;
                data = new ObjectSoundData;
                AddObjectSound(data);
                ReadSoundEmitterParams(block, fileName, 1, &data->m_buffer, &data->m_flags, &data->m_oneShotDistance,
                                       &data->m_randomTriggerPercent, &oneShot, &data->m_force2D);
                strcpy(data->m_name, fileName);
                ReadObjectNames(key, objectName, &data->m_objects, &m_haveObjectSounds);
            } else {
                for (k = 0; k < 6; k++) {
                    sprintf(message, "ImpactSound%d", k + 1);
                    if (!strcasecmp(soundType, message)) {
                        ReadImpactNames(key, objectName, k);
                        break;
                    }
                }
            }
        }
        ReportError("");
    } else {
        sprintf(message, "\nNumber of DynamicSounds<1 in %s.\n\n", m_fileName);
        ReportError(message);
    }
    if (!numStatic || !numDynamic)
        return 0;
    return 1;
}


// ---- src/Scene/ReadSoundEmitterParams.cpp


// Reads a sound emitter's settings from its parameter block (adding ".wav" to the
// file name if missing).
void Scene::ReadSoundEmitterParams(char *block, char *fileName, int is3D, _DS3DBUFFER *buffer, unsigned int *flags,
                                   float *oneShotDistance, float *randomTriggerPercent, int *oneShot, int *force2D)
{
    int looping;
    memset(buffer, 0, sizeof(_DS3DBUFFER));
    if (!strstr(fileName, ".wav"))
        strcat(fileName, ".wav");
    m_params->ReadParameterBlock(block);
    *flags = is3D ? 2 : 0;
    m_params->GetParameter("SoundLooping", 0, &looping);
    if (looping)
        *flags |= 8;
    m_params->GetParameter("SoundOneShot", 0, oneShot);
    if (*oneShot)
        *flags |= 1;
    m_params->GetParameter("SoundForce2D", 0, force2D);
    m_params->GetParameter("SoundOneShotDistance", 20.0f, oneShotDistance);
    m_params->GetParameter("SoundRandomTriggerPercent", 100.0f, randomTriggerPercent);
    m_params->GetParameter("SoundMaxDistance", 0.0f, &buffer->flMaxDistance);
    m_params->GetParameter("SoundMinDistance", 0.0f, &buffer->flMinDistance);
    buffer->vPosition = _D3DVECTOR(3.40282347e38f, 3.40282347e38f, 3.40282347e38f);
}


// ---- src/Scene/ReadSceneData.cpp
// ReadSceneData__5SceneFPP15ParticleManagerPFPi_viiP14ParameterBlock  @ 0x002903F0
// The light name is a ?: written inline in the Create call: as a separate local it
// is not turned into movz, and without that block-optimizer event retail's
// delay-slot fills are lost.


int Scene::ReadSceneData(ParticleManager **particles, void (*progress)(int *), int a, int b, ParameterBlock *params)
{
    char logName[0x110];
    int logProgress;
    sprintf(m_fileName, "%s%s", SceneContentDirectory, m_info->m_sceneFile);
    if (params) {
        if (m_params) {
            delete m_params;
            m_params = 0;
        }
        m_params = params;
    } else if (!m_params->OpenFile(m_fileName, 0)) {
        return 0;
    }
    m_params->ReadParameterBlock("SceneInfo");
    if (m_file)
        FCLOSE(m_file);
    logProgress = 0;
    m_params->GetParameter("LogProgress", 0, &logProgress);
    if (logProgress) {
        sprintf(logName, "%s%s", SceneContentDirectory, "scnmgr.log");
        if (!(m_file = FOPEN(logName, "w")))
            PSXprintf("Error opening Scene Log File");
    } else {
        m_file = 0;
    }
    if (!m_params->GetParameter("SceneName", "", m_sceneName, 0x3F))
        strncpy(m_sceneName, "NO NAME", 0x3F);
    ReportSuccess("SceneInfo", "SceneName", m_sceneName);
    ReadD3DVector(&m_startPosition, "SceneInfo", "DefaultStartPosition", "1152.0,100.0,1152.0", 1, 0);
    ReadD3DVector(&m_startDirection, "SceneInfo", "DefaultStartDirection", "0.0,0.0,1.0", 1, 0);
    m_params->GetParameter("PreRaceVue", "", m_preRaceVue, 0x7F);
    ReportError("");
    ReadD3DVector(&m_podiumPosition, "SceneInfo", "PodiumPosition", "0.0,0.0,0.0", 1, 0);
    ReadD3DVector(&m_podiumDirection, "SceneInfo", "PodiumDirection", "0.0,0.0,0.0", 1, 0);
    if (!ReadEnvironmentData(SceneContentDirectory, ""))
        return 0;
    if (b && !*particles) {
        if (!(*particles = new ParticleManager(1))->Create(m_manager->m_particleTexture, 2)) {
            PSXprintf("Failed to create the LpParticleManager in Scene::ReadSceneData\n");
            return 0;
        }
    }
    if (!ReadSceneInfo())
        return 0;
    m_params->ReadParameterBlock("Lights");
    m_params->GetParameter("NumberOfLights", -1, &m_numLights);
    ReportSuccess("Lights", "NumberOfLights", m_numLights);
    ReportError("");
    if (!m_info->m_sceneValid || m_numLights == -1)
        m_numLights = 2;
    _D3DVECTOR attenuation(0.0f, 0.0002f, 0.0f);
    m_lights = new LightData[m_numLights];
    for (int i = 0; i < m_numLights; i++) {
        m_lights[i].m_emitter = 0;
        if (!ReadLightData(i))
            return 0;
        if (m_lights[i].m_emitsLight) {
            m_lights[i].m_emitter = (new LightEmitter(1))->Create((LightEmitterType)m_lights[i].m_type, m_lights[i].m_color,
                                                                  &m_lights[i].m_position, m_lights[i].m_pDirection,
                                                                  &attenuation, m_lights[i].m_range,
                                                                  m_lights[i].m_showDebugSphere, 0, m_lights[i].m_hasLensFlare ? m_lights[i].m_lensFlareTexture : 0, i);
            if (!m_lights[i].m_emitter)
                return 0;
            LpLightManager->AddLight(m_lights[i].m_emitter, 0);
            AddChild(m_lights[i].m_emitter, 0);
        }
    }
    m_params->ReadParameterBlock("GhostOpacity");
    m_params->GetParameter("Ghost", 0.5f, &m_ghostOpacity);
    m_params->GetParameter("Reset", 0.5f, &m_ghostReset);
    m_params->ReadParameterBlock("CSCameras");
    m_params->GetParameter("NumberOfCameras", 0, &m_numCameras);
    if (m_numCameras > 0) {
        char cameraName[0x40];
        m_cameraZones = new int[m_numCameras];
        for (int i = 0; i < m_numCameras; i++) {
            sprintf(cameraName, "CSCamera%d", i + 1);
            m_params->ReadParameterBlock(cameraName);
            m_params->GetParameter("InitialZone", 1, &m_cameraZones[i]);
        }
    }
    ReadFogData();
    ReadResourceFiles();
    ReadStadiumData();
    ReadBuoyNames();
    ReadStaticModelData(particles ? *particles : 0, "", progress, a, b);
    ReadAnimationData(progress, a);
    ReadTriggerData();
    if (m_params) {
        delete m_params;
        m_params = 0;
    }
    return 1;
}


// ---- src/LightData/ctor.cpp

LightData::LightData()
{
}


// ---- src/Scene/Render3D.cpp


int Scene::Render3D()
{
    if (m_visible)
        return GameObject::Render3D();
    return 1;
}


// ---- src/Scene/KeyUpHandler.cpp
int KEYDOWN_HANDLER_TEST(unsigned int key, DeviceType device, ProcessNode *node, unsigned int mods);

int Scene::KeyUpHandler(ProcessNode *node, KeyInfo *key)
{
    if (KEYDOWN_HANDLER_TEST(0x21, DEVICE_KEYBOARD, node, 0x80)) {
        m_visible = 1 - m_visible;
        return 1;
    }
    return GameObject::KeyUpHandler(node, key) != 0;
}


// ---- src/Scene/KeyDownHandler.cpp


int Scene::KeyDownHandler(ProcessNode *node, KeyInfo *key)
{
    return GameObject::KeyDownHandler(node, key) != 0;
}


// ---- src/sinit/SceneManager.cpp
// Static initializer of SceneManager.cpp: only the axis vectors from the shared header.



// ---- ContainerList<ObjectSoundData *>: mwcc instantiates the members this
// TU calls at its end, where retail has them (0x290BB0-0x291020).
#define CONTAINERLIST_MEMBERS_ALL
#include "ContainerList/ContainerListMembers.h"
