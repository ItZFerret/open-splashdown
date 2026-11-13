// JetskiCompetition.cpp  @ 0x00244D70 (text 0x244D70-0x252C00)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp)
//
// Retail's JetskiCompetition.cpp as one translation unit: JetskiCompetition, the inline
// RaceStatusEventInterface and RaceStatusVehicleInterface default handlers and
// JetskiGhostVehicle's RSGetVehicleStatus (written with the vtables), the free callbacks
// (SetCurrentTunedLevel, checkSecretSkinObjects, TimeElimCallback, SecretSkinCallback) and
// the ContainerList<char *> members at the end (CONTAINERLIST_MEMBERS_ALL).
// 0x244C60-0x244D70 (EndOfRaceRecordKeeping, RaceStatusEventInterface's handlers, the @48@
// thunks) is JetskiCircuitCompetition.cpp's vtable tail. Defines .data gWaterLOD,
// gJetskiLevelTuning, SecretSkinObjects and .bss EnableResourceBuilding, WORLDOBJECT_*,
// gCurrentTunedLevel. `make link` places its data as well as its code.





#include "RaceStatusEventInterface/RaceStatusEventInterface.h"
#include "d3dtypes.h"
#include "JetskiGameOptions/JetskiGameOptions.h"
#include "JetskiGame/JetskiGame.h"
#include "PSXVideoCard/PSXVideoCard.h"
#include "GameObject/GameObject.h"
#include "JetskiCompetition/JetskiCompetition.h"
#include "MaterialStringTable/MaterialStringTable.h"
#include "TextureMapStringTable/TextureMapStringTable.h"
#include "DataAccess/DataAccess.h"
#include "SurfaceShaderList/SurfaceShaderList.h"
#include "RainbowPS2Snd/RainbowPS2Snd.h"
#include "PSXSoundInterface/PSXSoundInterface.h"
#include "AuralScape/AuralScape.h"
#include "Game/Game.h"
#include "JetskiNameMenu/JetskiNameMenu.h"
#include "JetskiPromptMenu/JetskiPromptMenu.h"
#include "ParameterBlock/ParameterBlock.h"
#include "PSXSound/PSXSound.h"
#include "PSXSoundGroup/PSXSoundGroup.h"
#include "PSXMusic/PSXMusic.h"
#include "ResponseEmitter/ResponseEmitter.h"
#include "JetskiLoadManager/JetskiLoadManager.h"
#include "RenderTarget/RenderTarget.h"
#include "PSXSurfaceShaderList/PSXSurfaceShaderList.h"
#include "RaceStatusClass/RaceStatusClass.h"
#include "TrackType/TrackType.h"
#include "GateClass/GateClass.h"
#include "Scene/Scene.h"
#include "Scene/SceneAnimation.h"
#include "CarProcedural/CarProcedural.h"
#include "BoatProcedural/BoatProcedural.h"
#include "SplitScreenSeparator/SplitScreenSeparator.h"
#include "SceneManagerData/SceneManagerData.h"
#include "BuoyObject/BuoyObject.h"
#include "CollisionObject/CollisionObject.h"
#include "SoultreeObject/SoultreeObject.h"
#include "JetskiVCRController/JetskiVCRController.h"
#include "ScreenPrimitiveManager/ScreenPrimitiveManager.h"
#include "ScreenIconManager/ScreenIconManager.h"
#include "ScreenTextManager/ScreenTextManager.h"
#include "VideoFXManager/VideoFXManager.h"
#include "JetskiCrowdSoundEmitter/JetskiCrowdSoundEmitter.h"
#include "JetskiCameraController/JetskiCameraController.h"
#include "JetskiFollowCamera/JetskiFollowCamera.h"
#include "CinematographerCam/CinematographerCam.h"
#include "SceneManager/SceneManager.h"
#include "ControlInterface/ControlInterface.h"
#include "JoystickDevice/JoystickDevice.h"
#include "JetskiMenu/JetskiMenu.h"
#include "JetskiInGameMenu/JetskiInGameMenu.h"
#include "JetskiRaceOverMenuCountdown/JetskiRaceOverMenuCountdown.h"
#include "JetskiRaceOverMenuCareer/JetskiRaceOverMenuCareer.h"
#include "JetskiRaceStartMenuCareer/JetskiRaceStartMenuCareer.h"
#include "JetskiTrainingInGameMenu/JetskiTrainingInGameMenu.h"
#include "JetskiTraining/JetskiTraining.h"
#include "JetskiCountDown/JetskiCountDown.h"
#include "JetskiRadar/JetskiRadar.h"
#include "JetskiInGameStats/JetskiInGameStats.h"
#include "JetskiPerformanceMeter/JetskiPerformanceMeter.h"
#include "PSXWaterDisplacement/PSXWaterDisplacement.h"
#include "Water/Water.h"
#include "JetskiVehicle/JetskiVehicle.h"
#include "Terrain/Terrain.h"
#include "PSXWater/PSXWater.h"
#include "CollisionCharacter/CollisionCharacter.h"
#include "Fog/Fog.h"
#include "DrawableCube/DrawableCube.h"
#include "SkyCube/SkyCube.h"
#include "soultree_assert_block.h"
#include "RenderableObject/RenderableObject.h"
#include "PSXRenderableObject/PSXRenderableObject.h"
#include "JetskiPlayerVehicle/JetskiPlayerVehicle.h"
#include "JetskiAIVehicle/AIVehicle.h"
#include "VehicleAIClass/VehicleAIClass.h"
#include "JetskiEngineModel/JetskiEngineModel.h"
#include "JetskiRider/JetskiRider.h"
#include "PhysicsRigidBody/PhysicsRigidBody.h"
#include "ViewPortCode/ViewPortCode.h"
#include "PSXCamera/PSXCamera.h"
#include "Camera/Camera.h"
#include "ParticleManager/ParticleManager.h"
#include "EcoSystem/EcoSystem.h"
#include "JetskiHeadingArrows/JetskiHeadingArrows.h"
#include "JetskiInfoText/JetskiInfoText.h"
#include "JetskiTVCamera/JetskiTVCamera.h"
#include "PhysicsBody/PhysicsBody.h"
#include "JetskiRiderSoundEmitter/JetskiRiderSoundEmitter.h"
#include "SegmentList_Tag/SegmentList_Tag.h"
#include "ProcessNode/ProcessNode.h"
#include "PSXTextureMapManager/PSXTextureMapManager.h"
#include "RaceStatusStruct_Tag/RaceStatusStruct_Tag.h"
#include "WaterWrecker/WaterWrecker.h"
#include "Character/Character.h"
#include "GameText/GameText.h"
#include "ContainerList/ContainerList.h"
#define CONTAINERLIST_MEMBERS_ALL
#include "ContainerList/ContainerListMembers.h"
#include "JetskiEngineSoundEmitter/JetskiEngineSoundEmitter.h"
#include "RaceStatusVehicleInterface/RaceStatusVehicleInterface.h"
#include "sinit/AxisVectors.h"

#pragma inline_depth(6)
#pragma fast_fptosi on

extern JetskiGame *LpGame;

// The time-trial ghost: a saucer or jet model that follows the best lap
// (0x20 bytes, `new`ed by LoadGhost). With m_x54 == 2 the "ghost" is a
// recorded JetskiVehicle instead.
class JetskiGhostVehicle : public RaceStatusVehicleInterface {
public:
    JetskiGhostVehicle(PSXRenderableObject *object) { m_object = object; }
    int RSGetVehicleStatus(_D3DVECTOR *position, _D3DVECTOR *velocity, float *speed, int *status)
    {
        if (position) {
            position->x = m_object->m_worldMatrix._41;
            position->y = m_object->m_worldMatrix._42;
            position->z = m_object->m_worldMatrix._43;
        }
        return 1;
    }

    PSXRenderableObject *m_object;      // +0x10
    char pad14[0x20 - 0x14];
};

// ---- src/global/SetCurrentTunedLevel.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" int strcmp(const char *a, const char *b);

struct JetskiLevelTuning {
    char name[0x100];           // +0x000
    int mode;                   // +0x100
    float m_fogFar;             // +0x104
    float m_fogVisibility;      // +0x108
    float m_fogHaziness;        // +0x10C
    int m_waterLevel;           // +0x110, index into gWaterLOD
    int m_renderFroth;          // +0x114
    float m_textureBiasMin;     // +0x118
    float m_textureBiasMax;     // +0x11C
    float m_geometryBiasMin;    // +0x120
    float m_geometryBiasMax;    // +0x124
};

// The water grid resolution (a) and tile count (b) per split-screen mode
// and level (LoadWater).
struct WaterLOD {
    int a;
    int b;
};

WaterLOD gWaterLOD[2][4] = {
    { { 45, 4 }, { 37, 3 }, { 37, 2 }, { 29, 2 } },
    { { 77, 2 }, { 69, 2 }, { 61, 2 }, { 53, 2 } },
};

JetskiLevelTuning gJetskiLevelTuning[30] = {
    { "default_multi", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.4f, 0.6f, 0.5f, 0.5f },
    { "default_single", 1, -1.0f, -1.0f, -1.0f, 0, 1, 1.0f, 1.0f, 1.0f, 1.0f },
    { "MissionBay_easy", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.4f, 0.5f, 0.4f, 0.6f },
    { "MissionBay_hard", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.4f, 0.5f, 0.4f, 0.6f },
    { "NileRiver_easy", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.2f, 0.6f, 0.3f, 0.7f },
    { "NileRiver_hard", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.2f, 0.6f, 0.3f, 0.7f },
    { "ThreeRivers_easy", 2, 4.1e+02f, -1.0f, -1.0f, 2, 0, 0.2f, 0.6f, 0.3f, 0.7f },
    { "ThreeRivers_hard", 2, 4.1e+02f, -1.0f, -1.0f, 2, 0, 0.2f, 0.6f, 0.3f, 0.7f },
    { "Amazon_easy", 2, 4.2e+02f, -1.0f, -1.0f, 2, 0, 0.2f, 0.5f, 0.3f, 0.7f },
    { "Amazon_hard", 2, -1.0f, -1.0f, -1.0f, 2, 0, 0.2f, 0.5f, 0.3f, 0.7f },
    { "Bali_easy", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.5f, 0.5f, 0.4f, 0.6f },
    { "Bali_hard", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.5f, 0.5f, 0.4f, 0.6f },
    { "BeringStrait_easy", 2, 4.3e+02f, -1.0f, -1.0f, 2, 0, 0.2f, 0.5f, 0.3f, 0.7f },
    { "BeringStrait_hard", 2, 4.3e+02f, -1.0f, -1.0f, 2, 0, 0.2f, 0.5f, 0.3f, 0.7f },
    { "Nice_easy", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.5f, 0.5f, 0.5f, 0.5f },
    { "Nice_hard", 2, -1.0f, -1.0f, -1.0f, 2, 1, 0.5f, 0.5f, 0.5f, 0.5f },
    { "Venice_easy", 2, 4.1e+02f, -1.0f, 0.1f, 3, 0, 0.2f, 0.3f, 0.3f, 0.6f },
    { "Venice_hard", 2, 4.1e+02f, -1.0f, 0.1f, 3, 0, 0.2f, 0.3f, 0.3f, 0.6f },
    { "Tokyo_easy", 2, 4.3e+02f, -1.0f, 0.2f, 2, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Tokyo_hard", 2, 4.3e+02f, -1.0f, 0.2f, 2, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "LochNess_easy", 2, 4.2e+02f, -1.0f, -1.0f, 2, 1, 0.2f, 0.5f, 0.3f, 0.6f },
    { "LochNess_hard", 2, 4.2e+02f, -1.0f, -1.0f, 2, 1, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Madrid_easy", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Madrid_hard", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Madrid2_easy", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Madrid2_hard", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Seoul_easy", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Seoul_hard", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Seoul2_easy", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
    { "Seoul2_hard", 2, -1.0f, -1.0f, -1.0f, 3, 0, 0.2f, 0.5f, 0.3f, 0.6f },
};

// The secret-skin pickups' object names (checkSecretSkinObjects).
char SecretSkinObjects[9][10] = {
    "Derrick", "Haily", "Andreas", "Kyoko", "Juana", "Sebastian", "Jeremy", "AmanRa", "Steve",
};

int EnableResourceBuilding;
// Collision owner types of the course objects (the ctor looks them up).
unsigned int WORLDOBJECT_PINKBALLOON;
unsigned int WORLDOBJECT_BLUEBALLOON;
unsigned int WORLDOBJECT_GREENBALLOON;
unsigned int WORLDOBJECT_RAMP;
unsigned int WORLDOBJECT_BUOY;
int gCurrentTunedLevel;

void SetCurrentTunedLevel(int mode, char *sceneFile)
{
    char buf[0x130];

    gCurrentTunedLevel = 0;
    if (mode < 2)
        gCurrentTunedLevel = 1;
    for (int i = 0; i < 30; i++) {
        sprintf(buf, "%s.scn", gJetskiLevelTuning[i].name);
        if (mode == gJetskiLevelTuning[i].mode && strcmp(sceneFile, buf) == 0) {
            gCurrentTunedLevel = i;
            return;
        }
    }
}


// ---- src/JetskiCompetition/ctor.cpp
class KeyInfo;
class ProcessNode;

extern "C" void *memset(void *dst, int c, unsigned int n);




extern PSXVideoCard *LpVideoCard;

class CollisionTypeRegistry {
public:
    unsigned int GetType(char *name);
};
extern CollisionTypeRegistry *LpCollisionTypeRegistry;

extern unsigned int WORLDOBJECT_PINKBALLOON;
extern unsigned int WORLDOBJECT_BLUEBALLOON;
extern unsigned int WORLDOBJECT_GREENBALLOON;
extern unsigned int WORLDOBJECT_RAMP;
extern unsigned int WORLDOBJECT_BUOY;

class VisibilityQuadTree;
extern VisibilityQuadTree *LpVisibilityQuadTree;




JetskiCompetition::JetskiCompetition(int enabled) : GameObject(enabled)
{
    int i;
    JetskiGameOptions *options = LpGame->m_gameOptions;
    m_x78 = 0;
    m_lightManager = 0;
    m_particles = 0;
    m_xA4 = 0;
    m_fog = 0;
    m_skyCube = 0;
    m_terrain = 0;
    m_ecoSystem = 0;
    m_water = 0;
    m_waterDisplacement = 0;
    m_scene = 0;
    m_inGameStats = 0;
    m_infoText = 0;
    m_radar = 0;
    m_headingArrows = 0;
    m_countDown = 0;
    m_separator = 0;
    m_performanceMeter = 0;
    if (LpGame->m_gameOptions->m_splitScreen)
        m_x134 = LpGame->m_gameOptions->m_x50;
    else
        m_x134 = 0;
    m_xA0 = 0;
    for (i = 0; i < 5; i++) {
        m_xA8[i] = 0;
        m_xBC[i] = 0;
    }
    m_xA4 = 0;
    LpVisibilityQuadTree = 0;
    m_textManager = 0;
    for (i = 0; i < 2; i++) {
        m_cameras[i] = 0;
        m_cameraControllers[i] = 0;
        m_vehicles[5 + i] = 0;
        m_fadeOutTime[i] = 0;
        m_fadeInTime[i] = 0;
    }
    m_videoFX = 0;
    m_numVehicles = 0;
    m_x118 = 0;
    m_numPlayers = 0;
    m_track = 0;
    m_raceStatus = 0;
    m_x1EC = 0;
    m_x1F0 = 0;
    m_crowd = 0;
    m_readySound = 0;
    m_balloonSounds[5] = 0;
    m_balloonSounds[4] = 0;
    m_balloonSounds[3] = 0;
    m_balloonSounds[2] = 0;
    m_balloonSounds[1] = 0;
    m_balloonSounds[0] = 0;
    m_skinSound = 0;
    m_rainSound = 0;
    m_goSound = 0;
    m_responses = 0;
    m_x1E8 = 0;
    m_music = 0;
    m_raceTime = 0;
    m_x204 = 0;
    m_x200 = 0;
    m_x1FC = 0;
    m_x208 = 0;
    m_xD8 = 0;
    m_x144 = 0;
    m_x148 = 0;
    m_iconManager = 0;
    m_inGameMenu = 0;
    m_raceStartMenu = 0;
    m_raceOverMenu = 0;
    m_nameMenu = 0;
    m_promptMenu = 0;
    m_x138 = 4.5f;
    m_joinTimer = 0;
    m_status = 0;
    m_x24C = 1;
    m_vcr = 0;
    m_x290 = 0;
    m_timeElimTime = 0;
    m_ghostEnabled = 0;
    m_ghost = 0;
    m_ghostObject = 0;
    m_ghostVisible = 0;
    m_ghostHeight = 0;
    m_playbackType = 1;
    m_cycleIndex = 0;
    m_cameraType = 0;
    m_x2C0 = 0;
    m_x2C4 = 0;
    m_x2C8 = 0;
    m_xD4 = -1;
    m_x34 = 0;
    m_sceneFolder[0] = 0;
    m_joinTime = 0;
    m_joinState = 0;
    m_options = 0;
    m_timeElimStartTime = 0;
    m_x294 = 0;
    m_bestTime = 0;
    m_x298 = 0;
    m_waterTime = 0;
    m_baseTime = 0;
    m_x258 = 100;
    m_x254 = 100;
    m_faded[1] = 0;
    m_faded[0] = 0;
    m_x2D8 = 0;
    m_x2DC = 0;
    m_x2E0 = 0;
    for (i = 0; i < 12; i++) {
        m_trackNames2[i] = 0;
        m_trackNames[i] = 0;
    }
    LpVideoCard->m_x44 = 1;
    WORLDOBJECT_PINKBALLOON = LpCollisionTypeRegistry->GetType("PINKBALLOON");
    WORLDOBJECT_BLUEBALLOON = LpCollisionTypeRegistry->GetType("BLUEBALLOON");
    WORLDOBJECT_GREENBALLOON = LpCollisionTypeRegistry->GetType("GREENBALLOON");
    WORLDOBJECT_RAMP = LpCollisionTypeRegistry->GetType("RAMP");
    WORLDOBJECT_BUOY = LpCollisionTypeRegistry->GetType("BUOY");
    m_x330 = 0;
    if (options->m_numHumans == 1) {
        if (options->m_raceType == 3) {
            m_x334 = 1;
            m_x338 = 2;
            m_x33C = 3;
            m_x340 = 4;
            m_x344 = 5;
        } else {
            m_x334 = 5;
            m_x338 = 1;
            m_x33C = 2;
            m_x340 = 3;
            m_x344 = 4;
        }
    } else {
        m_x334 = 4;
        m_x338 = 5;
        m_x33C = 1;
        m_x340 = 2;
        m_x344 = 3;
    }
    m_x348 = 0;
    m_x34C = 0;
    m_x350 = 0;
    m_currentStatus = 0;
    m_x358 = 1.0f;
    memset(m_copyCatCameras, 0, sizeof(m_copyCatCameras));
}


// ---- src/JetskiCompetition/dtor.cpp
extern "C" void free(void *p);

class KeyInfo;
class ProcessNode;



extern MaterialStringTable *LpMaterialStringTable;

extern TextureMapStringTable *LpTextureMapStringTable;

extern DataAccess *LpDataAccess;

extern SurfaceShaderList *LpSurfaceShaderList;





extern PSXVideoCard *LpVideoCard;


#define RELEASE_OBJECT(p)       \
    if (p) {                    \
        (p)->RemoveFromChain(); \
        (p)->Release();         \
        (p) = 0;                \
    }

JetskiCompetition::~JetskiCompetition()
{
    LpVideoCard->m_x44 = 0;
    LpMaterialStringTable->ReleaseMaterials();
    LpTextureMapStringTable->ReleaseTextureMaps();
    if (m_options->m_x54 != 2 && m_ghost) {
        delete m_ghost;
        m_ghost = 0;
    }
    if (m_ghostHeight) {
        delete m_ghostHeight;
        m_ghostHeight = 0;
    }
    RELEASE_OBJECT(m_inGameMenu)
    RELEASE_OBJECT(m_raceStartMenu)
    RELEASE_OBJECT(m_raceOverMenu)
    RELEASE_OBJECT(m_nameMenu)
    RELEASE_OBJECT(m_promptMenu)
    DestroyListeners(m_numPlayers);
    LpGame->m_auralScape->DestroyAllSoundEmitters();
    LpGame->m_auralScape->Disable();
    LpGame->m_auralScape->m_soundInterface->m_snd.StopAllVoices();
    LpGame->m_auralScape->m_soundInterface->m_snd.SendBatch();
    LpGame->m_auralScape->m_soundInterface->m_snd.ClearMemory();
    if (LpSurfaceShaderList) {
        delete LpSurfaceShaderList;
        LpSurfaceShaderList = 0;
    }
    for (int i = 0; i < 12; i++) {
        if (m_trackNames2[i])
            free(m_trackNames2[i]);
        if (m_trackNames[i])
            free(m_trackNames[i]);
    }
    LpDataAccess->DropAllDevices(1);
}


// ---- src/JetskiCompetition/Create.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" char *strstr(const char *s, const char *find);
extern "C" unsigned int strlen(const char *s);
extern "C" char *strcpy(char *dst, const char *src);
extern "C" void *malloc(unsigned int size);
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);
void PSXprintf(const char *fmt, ...);





// The ghost's height filter (20 bytes, built inline).
struct HeightFilter {
    float m_value;
    float m_rate;
    float m_scale;
    float m_min;
    float m_max;
    HeightFilter()
    {
        m_value = 0.0f;
        m_rate = 0.15f;
        m_min = 3.4028235e38f;
        m_max = -3.4028235e38f;
        m_scale = 1.0f;
    }
};

class StaticObject;


extern RenderTarget *LpRenderTarget;
extern DataAccess *LpDataAccess;
extern MaterialStringTable *LpMaterialStringTable;
extern TextureMapStringTable *LpTextureMapStringTable;
extern int EnableResourceBuilding;
extern char SceneContentDirectory[];
extern char AudioContentDirectory[];
extern float gPoolDepth;
extern unsigned int WORLDOBJECT_RAMP;
extern unsigned int WORLDOBJECT_TRIGGER;
extern unsigned int WORLDOBJECT_PINKBALLOON;
extern unsigned int WORLDOBJECT_BLUEBALLOON;
extern unsigned int WORLDOBJECT_GREENBALLOON;
void TimeElimCallback(CollisionObject *balloon, CollisionObject *other);
void SetCurrentTunedLevel(int mode, char *sceneFile);


// Unary minus (Direct3D's D3D_OVERLOADS form; d3dtypes.h lacks it).
inline _D3DVECTOR operator-(const _D3DVECTOR &v)
{
    return _D3DVECTOR(-v.x, -v.y, -v.z);
}

// VU0 cross product, out = a x b.
static inline void CrossProduct(_D3DVECTOR *out, _D3DVECTOR *a, _D3DVECTOR *b)
{
    asm("lqc2 vf4, 0x0(%1); lqc2 vf5, 0x0(%2); vopmula.xyz ACC, vf4, vf5; vopmsub.xyz vf6, vf5, vf4; vsub.w vf6, vf6, vf6; sqc2 vf6, 0x0(%0)"
        : : "r"(out), "r"(a), "r"(b) : "memory");
}

// Loading-screen progress.
#define LOAD_PROGRESS(f) LpGame->m_loadManager->AddProgress(f)

// The scene's resource file name: <folder><scene>T/C/R/E/H<n>.res by race type and skill.
#define RESOURCE_NAME(buf, fmtT, fmtC, fmtR, fmtE, fmtH, ...)                   \
    if (m_options->m_raceType == 4)                                             \
        sprintf(buf, fmtT, __VA_ARGS__);                                        \
    else if (m_options->m_raceType == 5)                                        \
        sprintf(buf, fmtC, __VA_ARGS__);                                        \
    else if (m_options->m_raceType == 8)                                        \
        sprintf(buf, fmtR, __VA_ARGS__);                                        \
    else {                                                                      \
        switch (m_options->m_difficulty) {                                      \
        case 1:                                                                 \
        case 2:                                                                 \
            sprintf(buf, fmtE, __VA_ARGS__);                                    \
            break;                                                              \
        case 3:                                                                 \
            sprintf(buf, fmtH, __VA_ARGS__);                                    \
            break;                                                              \
        }                                                                       \
    }

#define LOAD_SOUND(member, name, flags)                                         \
    member = new PSXSound(LpGame->m_auralScape->m_soundInterface, m_scene->m_soundGroup, 1); \
    if (!member->CreateFromFile(name, flags, 3)) {                              \
        if (member) {                                                           \
            delete member;                                                      \
            member = 0;                                                         \
        }                                                                       \
    }

#define LOAD_SOUND_VOL(member, name)                                            \
    member = new PSXSound(LpGame->m_auralScape->m_soundInterface, m_scene->m_soundGroup, 1); \
    if (!member->CreateFromFile(name, 1, 3)) {                                  \
        if (member) {                                                           \
            delete member;                                                      \
            member = 0;                                                         \
        }                                                                       \
    } else {                                                                    \
        member->SetVolume(100.0f);                                              \
    }


// Session shim: a heap image for mwcc's uninitialised "has call" bytes. It flags three AddProgress
// constants (the 2nd and 5th 0.05f, the 4th 0.04f) so they load before LpGame, as in retail, and
// clears every other flag; the third flag lands in the parser heap's second block, hence the size.
// Image object in .mwccshim.

struct MwccHeapImage {
    char pad0[0x3EC5];
    char flag0;
    char pad1[0x5347];
    char flag1;
    char pad2[0x1CACF];
    char flag2;
    char tail[0x19322];
};
#pragma section mwccshim begin
inline void MwccHeapShim() { const MwccHeapImage image = { {0}, 1, {0}, 1, {0}, 1 }; }
#pragma section mwccshim end
// Load and build the whole race: scene, water, fog, terrain, track and race status, the static
// models' gates and balloons, resource files, VCR, sounds, overlays, vehicles and music.
JetskiCompetition *JetskiCompetition::Create()
{
    char resourceFile[0x110];
    int i;
    LpGame->m_auralScape->m_soundInterface->ClearSoundStringTable();
    static int event1 = -1;
    static int event4 = -1;
    // Underwater levels (the char casts give retail's andi).
    int underwater = (unsigned char)(LpGame->m_gameOptions->m_x2C == 3 || LpGame->m_gameOptions->m_x2C == 11
                               || LpGame->m_gameOptions->m_x2C == 7 || LpGame->m_gameOptions->m_x2C == 15);
    int noSky = (unsigned char)(LpGame->m_gameOptions->m_x2C == 7 || LpGame->m_gameOptions->m_x2C == 15);
    LpRenderTarget->SetTextureStageState(0, (_D3DTEXTURESTAGESTATETYPE)13, 0);
    LpRenderTarget->SetTextureStageState(0, (_D3DTEXTURESTAGESTATETYPE)14, 0);
    GameObject::Create();
    LOAD_PROGRESS(0.05f);
    new PSXSurfaceShaderList;
    m_options = LpGame->m_gameOptions;
    ModifyLocalGameOptions();
    m_numVehicles = m_options->m_numRacers;
    m_x118 = m_options->m_numHumans;
    m_x134 = LpGame->m_gameOptions->m_x50;
    SetSceneFolder();
    LOAD_PROGRESS(0.05f);
    static int event2 = -1;
    if (EnableResourceBuilding) {
        LpDataAccess->Initialize(1, 10, 0x100000);
        LpDataAccess->TagForSave(1);
    } else {
        RESOURCE_NAME(resourceFile, "%s%s\\%.6sT1.res", "%s%s\\%.6sC1.res", "%s%s\\%.6sR1.res",
                      "%s%s\\%.6sE1.res", "%s%s\\%.6sH1.res", SceneContentDirectory, m_sceneFolder, m_sceneFolder)
        LpDataAccess->LoadResourceFile(resourceFile, 0, 0, 0, 0, 0, 0, 0);
    }
    LOAD_PROGRESS(0.05f);
    if (!LoadLightManager())
        return 0;
    LOAD_PROGRESS(0.05f);
    static int event3 = -1;
    if (!LoadScene())
        return 0;
    static int event5 = -1;
    LOAD_PROGRESS(0.05f);
    SetCurrentTunedLevel(m_x118, LpGame->m_sceneManager->m_sceneFile);
    LoadCameras(m_numPlayers, underwater);
    LOAD_PROGRESS(0.05f);
    static int event6 = -1;
    if (!LoadWater())
        return 0;
    if (!LoadWaterDisplacement())
        return 0;
    static int event7 = -1;
    if (!LoadFog(underwater))
        return 0;
    if (!noSky && !LoadSkyCube())
        return 0;
    LOAD_PROGRESS(0.05f);
    if (!LoadTerrain())
        return 0;
    LOAD_PROGRESS(0.05f);
    switch (m_options->m_raceType) {
    case 6:
    case 7:
    case 1:
    case 2:
    case 3:
        if (LoadTrackDefinition())
            AddChild(m_track, -1);
        else
            m_track = 0;
        break;
    }
    LOAD_PROGRESS(0.04f);
    if (!(m_raceStatus = (new RaceStatusClass(1))->Create(this))) {
        Release();
        return 0;
    }
    m_raceStatus->m_x4C = m_track;
    m_raceStatus->m_x64 = m_options->m_x30;
    m_raceStatus->m_x84 = gPoolDepth;
    switch (m_options->m_raceType) {
    case 1:
    case 6:
    case 7:
        m_raceStatus->m_x44 = 1;
        m_raceStatus->m_raceType = 2;
        m_raceStatus->m_x6C = 30.0f;
        break;
    case 2:
        m_raceStatus->m_x44 = 2;
        m_raceStatus->m_raceType = 2;
        m_raceStatus->m_x6C = 30.0f;
        break;
    case 5:
        m_raceStatus->m_x44 = 1;
        m_raceStatus->m_raceType = 5;
        m_raceStatus->m_x6C = -1.0f;
        m_raceStatus->m_x70 = 30.99f;
        m_raceStatus->m_x7C = 6.0f;
        m_raceStatus->m_x80 = 0.5f;
        break;
    case 4:
        m_raceStatus->m_x44 = 1;
        m_raceStatus->m_raceType = 4;
        m_raceStatus->m_x6C = -1.0f;
        m_raceStatus->m_x70 = 60.99f;
        break;
    case 3:
        m_raceStatus->m_x44 = 2;
        m_raceStatus->m_raceType = 6;
        m_raceStatus->m_x6C = -1.0f;
        break;
    case 8:
        m_raceStatus->m_x44 = 1;
        m_raceStatus->m_raceType = 3;
        m_raceStatus->m_x6C = 30.0f;
        break;
    }
    LOAD_PROGRESS(0.04f);
    if (m_scene->m_zoneModels && m_scene->m_zoneModels[0]) {
        _D3DVECTOR left;
        _D3DVECTOR right;
        _D3DVECTOR normal;
        for (i = 0; i < m_scene->m_zoneModels[0]->m_numModels; i++) {
            StaticModel *model = &((ZoneModels *)m_scene->m_zoneModels[0])->m_models[i];  // the redundant cast keeps retail's codegen (8 words without it)
            if (strstr(model->m_name, "ramp") || strstr(model->m_name, "bankleft") || strstr(model->m_name, "bankright")) {
                if (model->m_collision)
                    model->m_collision->m_ownerType |= WORLDOBJECT_RAMP;
            } else if (strstr(model->m_name, "startgate")) {
                if (m_track) {
                    _D3DMATRIX *m = &model->m_object->m_worldMatrix;
                    GateClass *gate = m_track->m_endGate;
                    gate->m_center.x = m->_41;
                    gate->m_center.y = m->_42;
                    gate->m_center.z = m->_43;
                    gate->m_halfSize.x = 27.0f * m->_11;
                    gate->m_halfSize.y = 27.0f * m->_12;
                    gate->m_halfSize.z = 27.0f * m->_13;
                    gate->m_height = 50.0f;
                    gate->m_x70 = -1.0f;
                    m_track->FindClosest(gate->m_center, &m_track->m_finish, 0);
                    if (m_track->DistanceBetween(m_track->m_startTrackPosition, m_track->m_finish) < m_track->m_behind) {
                        m_track->m_startGate->m_center = gate->m_center + 3.0f * gate->m_normal;
                        m_track->FindClosest(m_track->m_startGate->m_center, &m_track->m_startTrackPosition, 0);
                    }
                }
            } else if (strstr(model->m_name, "rightturnbuoy")) {
                BuoyObject *buoy = model->m_buoy;
                if (buoy) {
                    left = buoy->m_position;
                    right = -buoy->m_forward;
                    CrossProduct(&normal, &buoy->m_xC0, &buoy->m_forward);
                    normal *= 100.0f;
                    m_raceStatus->m_flags |= 0x40;
                    buoy->m_marker = (BuoyMarker *)m_raceStatus->AddGate(left, right, normal, 100.0f, -1.0f, 0.0f, 11);
                }
            } else if (strstr(model->m_name, "leftturnbuoy")) {
                BuoyObject *buoy = model->m_buoy;
                if (buoy) {
                    left = buoy->m_position;
                    right = buoy->m_forward;
                    CrossProduct(&normal, &buoy->m_xC0, &buoy->m_forward);
                    normal *= 100.0f;
                    m_raceStatus->m_flags |= 0x40;
                    buoy->m_marker = (BuoyMarker *)m_raceStatus->AddGate(left, right, normal, 100.0f, -1.0f, 0.0f, 11);
                }
            } else if (strstr(model->m_name, "cbalfp")) {
                if (model->m_collision) {
                    model->m_collision->m_ownerType = WORLDOBJECT_TRIGGER | WORLDOBJECT_PINKBALLOON;
                    model->m_collision->m_otherCallback = TimeElimCallback;
                    m_timeElimTime++;
                }
            } else if (strstr(model->m_name, "cbalfb")) {
                if (model->m_collision) {
                    model->m_collision->m_ownerType = WORLDOBJECT_TRIGGER | WORLDOBJECT_BLUEBALLOON;
                    model->m_collision->m_otherCallback = TimeElimCallback;
                    m_timeElimTime++;
                }
            } else if (strstr(model->m_name, "cbalfg")) {
                if (model->m_collision) {
                    model->m_collision->m_ownerType = WORLDOBJECT_TRIGGER | WORLDOBJECT_GREENBALLOON;
                    model->m_collision->m_otherCallback = TimeElimCallback;
                    m_timeElimTime++;
                }
            } else {
                checkSecretSkinObjectsName(model->m_name, (StaticObject *)model);
            }
        }
    }
    m_timeElimStartTime = m_timeElimTime;
    LOAD_PROGRESS(0.04f);
    if (m_options->m_raceType == 3)
        LoadGhost();
    static int event8 = -1;
    LpMaterialStringTable->LoadRequested();
    LpTextureMapStringTable->LoadRequested();
    static int event9 = -1;
    if (EnableResourceBuilding) {
        LpDataAccess->TagForSave(0);
        RESOURCE_NAME(resourceFile, "%s%.6sT1.res", "%s%.6sC1.res", "%s%.6sR1.res", "%s%.6sE1.res",
                      "%s%.6sH1.res", SceneContentDirectory, m_sceneFolder)
        LpDataAccess->SaveResourceFile(resourceFile, 1, 1, 0, 0, 0, 0, 1);
    } else {
        LpDataAccess->DropDevice(resourceFile, 1);
    }
    LOAD_PROGRESS(0.04f);
    if (!m_x134) {
        m_ghostEnabled = m_options->m_raceType == 3;
        if (m_ghostEnabled) {
            m_vcr = (JetskiVCRController *)(new JetskiVCRController(1))->Create(27, 0x2CFB8, 1, 0);
            if (!m_ghostHeight)
                m_ghostHeight = new HeightFilter;
        } else if (m_options->m_raceType == 8) {
            m_vcr = (JetskiVCRController *)(new JetskiVCRController(1))->Create(84, 0xC800, 1, 0);
        } else {
            m_vcr = (JetskiVCRController *)(new JetskiVCRController(1))->Create(175, 0x70738, 0, 0);
        }
    }
    LOAD_PROGRESS(0.04f);
    if (LpGame->m_auralScape) {
        LpGame->m_auralScape->m_soundInterface->SetStreamVolume(100);
        m_x1EC = new PSXSoundGroup(LpGame->m_auralScape->m_soundInterface, m_enabled);
        m_x1EC = (PSXSoundGroup *)m_x1EC->Create();
        m_x1EC->SetVolume(LpGame->m_gameOptions->m_x82C);
        AddChild(m_x1EC, 2);
        m_x1F0 = new PSXSoundGroup(LpGame->m_auralScape->m_soundInterface, m_enabled);
        m_x1F0 = (PSXSoundGroup *)m_x1F0->Create();
        m_x1F0->SetVolume(LpGame->m_gameOptions->m_musicVolume);
        AddChild(m_x1F0, 2);
        CreateListeners(m_numPlayers);
        LpGame->m_auralScape->Enable();
        LpGame->m_auralScape->SetDolby(m_options->m_x830 == 3);
        m_scene->m_soundGroup->SetVolume(LpGame->m_gameOptions->m_x828);
        if (m_scene->m_crowdPresent) {
            char crowdFile[0x110];
            sprintf(crowdFile, "%s%s", AudioContentDirectory, "\\Crowd\\crowd.aud");
            m_crowd = new JetskiCrowdSoundEmitter(LpGame->m_auralScape, m_scene->m_soundGroup, 3, 1);
            m_crowd = m_crowd->Create(crowdFile);
            if (m_crowd)
                AddChild(m_crowd, -1);
        }
        LOAD_SOUND(m_readySound, "Waypoint\\count.wav", 1)
        LOAD_SOUND(m_goSound, "Waypoint\\start.wav", 1)
        if (m_options->m_raceType == 4) {
            LOAD_SOUND_VOL(m_balloonSounds[0], "ball001.wav")
            LOAD_SOUND_VOL(m_balloonSounds[1], "ball002.wav")
            LOAD_SOUND_VOL(m_balloonSounds[2], "ball003.wav")
            if (m_x134) {
                LOAD_SOUND_VOL(m_balloonSounds[3], "ball004.wav")
                LOAD_SOUND_VOL(m_balloonSounds[4], "ball005.wav")
                LOAD_SOUND_VOL(m_balloonSounds[5], "ball006.wav")
            }
        }
        LOAD_SOUND_VOL(m_skinSound, "skin001.wav")
        if ((LpGame->m_gameOptions->m_x2C == 1 && LpGame->m_gameOptions->m_difficulty != 3)
            || (LpGame->m_gameOptions->m_x2C == 10 && LpGame->m_gameOptions->m_difficulty != 3)) {
            m_rainSound = new PSXSound(LpGame->m_auralScape->m_soundInterface, m_scene->m_soundGroup, 1);
            if (!m_rainSound->CreateFromFile("Soultree\\Everg\\rain.vag", 0x81, 3)) {
                if (m_rainSound) {
                    delete m_rainSound;
                    m_rainSound = 0;
                }
            } else {
                m_rainSound->SetVolume(100.0f);
            }
        }
    }
    LOAD_PROGRESS(0.04f);
    static int event11 = -1;
    if (EnableResourceBuilding) {
        LpDataAccess->Initialize(1, 10, 0x100000);
        LpDataAccess->TagForSave(1);
    } else {
        LpDataAccess->DropDevice(resourceFile, 1);
        RESOURCE_NAME(resourceFile, "%s%.6sT2.res", "%s%.6sC2.res", "%s%.6sR2.res", "%s%.6sE2.res",
                      "%s%.6sH2.res", SceneContentDirectory, m_sceneFolder)
        LpDataAccess->LoadResourceFile(resourceFile, 0, 0, 0, 0, 0, 0, 0);
    }
    LOAD_PROGRESS(0.04f);
    static int event12 = -1;
    LOAD_PROGRESS(0.04f);
    if (!LoadEcosystem())
        return 0;
    if (m_scene->m_numBuoys > 0 && !CreateBuoys())
        return 0;
    LOAD_PROGRESS(0.04f);
    SetUpAnimations();
    if (underwater || LpGame->m_gameOptions->m_raceType == 8)
        m_xD4 = -2;
    m_x34 = (new ScreenPrimitiveManager(1));
    m_x34 = ((ScreenPrimitiveManager *)m_x34)->Create(128);
    if (!(m_iconManager = (new ScreenIconManager(1))->Create(0x80))) {
        PSXprintf("ScreenIconManager not created.\n");
        return 0;
    }
    LOAD_PROGRESS(0.04f);
    if (!(m_textManager = (new ScreenTextManager(0, 1))->Create(0x80))) {
        PSXprintf("ScreenTextManager not created.\n");
        return 0;
    }
    LoadOverlays();
    LOAD_PROGRESS(0.04f);
    if (!(m_videoFX = (VideoFXManager *)(new VideoFXManager(1))->Create()))
        PSXprintf("Video FX Manager not created...\n");
    LOAD_PROGRESS(0.04f);
    static int event13 = -1;
    LpMaterialStringTable->LoadRequested();
    LpTextureMapStringTable->LoadRequested();
    if (EnableResourceBuilding) {
        LpDataAccess->TagForSave(0);
        RESOURCE_NAME(resourceFile, "%s%.6sT2.res", "%s%.6sC2.res", "%s%.6sR2.res", "%s%.6sE2.res",
                      "%s%.6sH2.res", SceneContentDirectory, m_sceneFolder)
        LpDataAccess->SaveResourceFile(resourceFile, 1, 1, 0, 0, 0, 0, 1);
    } else {
        LpDataAccess->DropDevice(resourceFile, 1);
    }
    static int event14 = -1;
    m_status = 1;
    static int event10 = -1;
    if (!LoadRaceVehicles(underwater))
        return 0;
    static int event20 = -1;
    if (!InitializeRace())
        return 0;
    LOAD_PROGRESS(0.05f);
    if (LpGame->m_auralScape) {
        static int event21 = -1;
        if (m_options->m_raceType != 8) {
            ParameterBlock block;
            char key[0x20];
            char value[0x30];
            if (block.OpenFile("Audio\\songs.pb", 0)) {
                if (block.ReadParameterBlock("SongTitles")) {
                    for (i = 1; i <= 12; i++) {
                        sprintf(key, "Band_%d", i);
                        if (block.GetParameter(key, value, 40)) {
                            m_trackNames[i - 1] = (char *)malloc(strlen(value) + 1);
                            strcpy(m_trackNames[i - 1], value);
                        }
                        sprintf(key, "Song_%d", i);
                        if (block.GetParameter(key, value, 40)) {
                            m_trackNames2[i - 1] = (char *)malloc(strlen(value) + 3);
                            sprintf(m_trackNames2[i - 1], "\"%s\"", value);
                        }
                    }
                }
            } else {
                PSXprintf("PB PROB:Can't load %s\n", "Audio\\songs.pb");
            }
            m_music = (PSXMusic *)PS2_ADDR(LpGame->m_auralScape->CreateMusic(1, 12, m_x1F0->m_groupID));
            if (m_music) {
                m_x1F0->SetVolume(LpGame->m_gameOptions->m_musicVolume);
                m_music->Shuffle(1);
                AddChild(m_music, 2);
            }
        }
    }
    LOAD_PROGRESS(0.02f);
    if (LpGame->m_auralScape) {
        _DS3DBUFFER buffer;
        m_responses = new ResponseEmitter(LpGame->m_auralScape, m_scene->m_soundGroup, 1, 1);
        if (m_responses) {
            AddChild(m_responses, -1);
            m_responses->Create(buffer, 2, 20.0f, 100.0f, 0, 0);
            m_responses->LoadResponseList("UI\\InGame.aud");
        }
        m_x1E8 = new PSXSound(LpGame->m_auralScape->m_soundInterface, m_x1EC, 1);
        if (!m_x1E8->CreateFromFile("Seb\\Negative\\ooff01.vag", 1, 1)) {
            if (m_x1E8) {
                delete m_x1E8;
                m_x1E8 = 0;
            }
        }
    }
    LpGame->m_auralScape->m_soundInterface->LoadRequestedSounds();
    if (m_rainSound)
        m_rainSound->Play(0, 0);
    SetupCamerasAndViewports();
    if (LpGame->m_gameOptions->m_raceType == 5) {
        CopyCatCamera *cam = &m_copyCatCameras[0];
        cam->m_x04 = m_cameraControllers[0]->m_followCamera->m_settings[3];
        cam->m_x08 = m_cameraControllers[0]->m_followCamera->m_settings[0];
        cam->m_x0C = m_cameraControllers[0]->m_followCamera->m_settings[2];
        cam->m_x10 = m_cameraControllers[0]->m_followCamera->m_settings[1];
        memcpy(cam->m_presets, &m_cameraControllers[0]->m_cinematographer->m_input, sizeof(cam->m_presets));
        memcpy(&m_copyCatCameras[1], cam, sizeof(CopyCatCamera));
    }
    return this;
}


// ---- src/JetskiCompetition/Tick.cpp

void PSXprintf(const char *fmt, ...);







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

// a + t * (b - a), with the parameters in the order that evaluates the
// geometry bias as retail does (the texture bias uses Lerp2 on locals).
static inline float Lerp(float t, float b, float a)
{
    return a + t * (b - a);
}

static inline float Lerp2(float t, float a, float b)
{
    return a + t * (b - a);
}

// Scales the LOD biases with the frame time (full detail at 28 ms or less)
// and returns the scale.
static inline float LODScale(float dt)
{
    float scale = 1.0f - (dt - 0.028f) / dt;
    scale = Fmax(0.0f, scale);
    scale = Fmin(1.0f, scale);
    float geometry = Lerp(scale, gJetskiLevelTuning[gCurrentTunedLevel].m_geometryBiasMax,
                          gJetskiLevelTuning[gCurrentTunedLevel].m_geometryBiasMin);
    float textureMin = gJetskiLevelTuning[gCurrentTunedLevel].m_textureBiasMin;
    float textureMax = gJetskiLevelTuning[gCurrentTunedLevel].m_textureBiasMax;
    float texture = Lerp2(scale, textureMin, textureMax);
    RenderableObject::SetRenderableLODBias(geometry, texture);
    return scale;
}

int JetskiCompetition::Tick(float dt)
{
    static float counter = 0;
    int i;

    counter += dt;
    m_raceTime += dt;
    if (m_music && m_x258 != m_x254) {
        switch (m_x254) {
        case 100:
            if (m_music->m_playing) {
                m_x1F0->SetVolume(LpGame->m_gameOptions->m_musicVolume);
            } else {
                m_x1F0->SetVolume(LpGame->m_gameOptions->m_musicVolume);
                m_music->Shuffle(1);
            }
            break;
        case 0:
            PSXprintf("Music Stopped\n");
            m_music->Stop(0.0f);
            break;
        default:
            m_x1F0->SetVolume(LpGame->m_gameOptions->m_musicVolume * (float)m_x254 / 100.0f);
            break;
        }
    }
    m_x258 = m_x254;
    switch (m_status) {
    case 1:
        m_status = 2;
        m_x138 = 4.5f;
        m_joinTimer = 0;
        break;
    case 2:
        if (m_raceStartMenu) {
            Pause(1);
            ((JetskiMenu *)m_raceStartMenu)->EnableMenu();
        } else if (m_options->m_raceType == 5) {
            FadeInCameras();
            m_promptMenu->SetCopyCatPrompt(0, 1);
            m_promptMenu->EnableMenu();
            m_status = 3;
        } else {
            m_status = 3;
        }
        break;
    case 3:
        if ((m_options->m_x48 != 2 && m_x138 == 4.5f) || (m_options->m_x48 == 2 && m_x24C)) {
            if (m_countDown)
                m_countDown->EnableAll();
            m_x138 = m_options->m_raceType == 5 ? 0.0f : 3.5f;
            m_x24C = 0;
            ResetCameraControllers();
            if (m_radar && LpGame->m_gameOptions->m_x858)
                m_radar->SetShowing(1);
            m_inGameStats->SetShowing(-1, -1);
            m_performanceMeter->SetShowing(-1, -1);
            if (m_options->m_raceType == 5 || m_options->m_raceType == 4)
                m_performanceMeter->SetPerformance(0);
            if (m_options->m_raceType == 6)
                m_inGameStats->SetRaceTypeText(1);
            m_inGameStats->SetReplayText(0);
        } else if (m_x138 > 0.0f) {
            if (m_options->m_x48 != 2) {
                int seconds = m_x138;
                m_x138 -= dt;
                if ((int)m_x138 < seconds) {
                    if (m_readySound)
                        m_readySound->Play(1, 0);
                }
            }
        } else {
            if (m_countDown) {
                m_countDown->Hide();
                m_countDown->DisableAll();
            }
            if (m_options->m_raceType == 6)
                m_inGameStats->SetRaceTypeText(0);
            if (m_goSound)
                m_goSound->Play(1, 0);
            m_status = 5;
            m_raceStatus->StartRace();
            if (m_raceStatus->m_raceType != 6 && !m_x290 && m_vcr) {
                m_vcr->REC();
                m_waterTime = m_waterDisplacement->m_time;
                m_baseTime = ((Water *)m_water)->GetBaseTime();
            }
        }
        break;
    case 5:
        if ((m_raceStatus->m_flags & 2) && m_status < 7) {
            m_status = 7;
            for (i = 0; i < m_numPlayers; i++) {
                if (m_cameraControllers[i]->m_cameraType != 2) {
                    m_cameraControllers[i]->SwitchCamera((CameraType)0);
                    m_cameraControllers[i]->SwitchFollowCameraPresets((FollowCameraPresetType)2);
                    m_cameraControllers[i]->m_followCamera->m_resetFilters = 1;
                    m_fadeOutTime[m_cameraControllers[i]->m_viewport] = 0;
                    m_fadeInTime[m_cameraControllers[i]->m_viewport] = 0;
                    m_faded[m_cameraControllers[i]->m_viewport] = 0;
                    if (m_videoFX) {
                        m_videoFX->m_state[m_cameraControllers[i]->m_viewport] = 2;
                        m_videoFX->m_duration[m_cameraControllers[i]->m_viewport] = 100;
                    }
                    m_cameraControllers[i]->StartRotate();
                }
            }
        }
        if (m_vcr)
            UpdateGhost(dt);
        break;
    case 7:
        m_joinTimer += dt;
        if (m_joinTimer > 3.5f || m_vehicles[5]->m_x530 || (m_x134 && m_vehicles[6]->m_x530)) {
            if (m_options->m_raceType == 7 && m_vehicles[0]->m_ownStatus->m_place < 2) {
                m_status = 8;
                m_joinState = 1;
            } else {
                m_status = 9;
            }
        }
        break;
    case 8:
        if (!DoingJoinAnimation())
            m_status = 9;
        break;
    case 9:
        Pause(1);
        if (m_radar)
            m_radar->SetShowing(0);
        m_inGameStats->SetShowing(0, 0);
        m_performanceMeter->SetShowing(0, 0);
        if (m_raceOverMenu)
            ((JetskiMenu *)m_raceOverMenu)->EnableMenu();
        else
            EventComplete();
        for (i = 0; i < m_numPlayers; i++)
            m_cameraControllers[i]->StopRotate();
        return 1;
    case 10:
        if (m_vcr->TestEOT()) {
            m_status = 9;
            Pause(1);
            m_raceOverMenu->EnableAll();
        }
        if (m_options->m_raceType != 8 && m_playbackType == 1)
            PickACamera(dt);
        else if (m_playbackType == 2)
            UpdateGhost(dt);
        break;
    }
    float scale = LODScale(dt);
    if (m_x118 > 1)
        PSXRenderableObject::SetMultipassCullDistance(5.0f + 10.0f * scale);
    else
        PSXRenderableObject::SetMultipassCullDistance(20.0f + 5.0f * scale);
    UpdateListeners();
    GameObject::Tick(dt);
    if (m_xD8 && (m_raceStatus->m_flags & 1)) {
        m_xD8 = 0;
        m_status = 2;
        ResetVCR();
    }
    if (m_options->m_raceType == 5) {
        if (m_x348) {
            FadeInCameras();
            m_promptMenu->EnableMenu();
            m_x348 = 0;
        }
        if (m_x34C) {
            if (m_x350 > 0.0f) {
                m_x350 -= dt;
            } else {
                CopyCatSwitchTurn(m_currentStatus);
                m_x34C = 0;
                m_raceStatus->m_flags &= ~8;
            }
        }
    }
    if (counter > 1.0f) {
        switch (m_x118) {
        case 2:
            if (!LpGame->m_controls->m_joysticks[1]->IsControllerPluggedIn())
                InGamePause(1, 1);
        case 1:
            if (!LpGame->m_controls->m_joysticks[0]->IsControllerPluggedIn()) {
                if (m_status == 10 && m_options->m_raceType != 8) {
                    m_status = 9;
                    LpGame->m_paused = 0;
                    Pause(1);
                    m_raceOverMenu->EnableAll();
                    LpGame->m_auralScape->StopAllAuralScapeSounds();
                    LpGame->m_auralScape->Disable();
                } else {
                    InGamePause(1, 0);
                }
            }
            break;
        }
        counter = 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/LoadWaterDisplacement.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern char SceneContentDirectory[];


class PSXWater;
class LightManager;




int JetskiCompetition::LoadWaterDisplacement()
{
    char name[0x110];
    int players = 1;
    switch (m_x134) {
    case 0:
        // Redundant store: it compiles to nothing, which leaves an empty case
        // block that mwcc's block optimizer threads past; that rebuilds the
        // predecessor lists, so the `new` null check gets retail's
        // `move a0,v0` delay slot.
        players = 1;
        break;
    case 2:
    case 1:
        players = 2;
        break;
    case 3:
        break;
    case 4:
        break;
    }
    sprintf(name, "%sbaseDisplacement.tga", SceneContentDirectory);
    m_waterDisplacement = (new PSXWaterDisplacement)->Create(m_water, name, m_lightManager, SceneContentDirectory, players, m_x134);
    ((Water *)m_water)->setWaterDisplacement(m_waterDisplacement);
    return 1;
}


// ---- src/JetskiCompetition/LoadLightManager.cpp

int JetskiCompetition::LoadLightManager()
{
    m_xA0 = 0;
    m_xA4 = 0;
    return 1;
}


// ---- src/JetskiCompetition/LoadTerrain.cpp

extern "C" char *strcpy(char *dst, const char *src);
extern "C" char *strcat(char *dst, const char *src);
extern "C" char *strstr(const char *s, const char *find);
extern "C" int strcasecmp(const char *a, const char *b);
void PSXprintf(const char *fmt, ...);

extern DataAccess *LpDataAccess;





// An inlined strrchr(name, '.').
static inline char *FindExtension(char *s)
{
    char *start = s;
    while (*s++)
        ;
    do {
        s--;
    } while (s != start && *s != '.');
    if (*s == '.')
        return s;
    return 0;
}

int JetskiCompetition::LoadTerrain()
{
    char path[0x110];

    if (!strcasecmp(m_scene->m_manager->m_terrainFile, "NONE")) {
        m_terrain = 0;
        PSXprintf("Terrain intentionally omitted in scene file.\n");
        return 1;
    }
    strcpy(path, m_scene->m_manager->m_terrainFile);
    char *extension = FindExtension(path);
    if (extension)
        strcpy(extension, ".tr2");
    else
        strcat(path, (char *)".tr2");    // redundant cast: retail loads the literal first
    // Prefer the IPU-compressed version (.tri) when it's there.
    strcpy(strstr(path, ".tr2"), ".tri");
    if (!LpDataAccess->FileExists(path)) {
        PSXprintf("IPU Compressed terrain '%s' wasn't in current folder\n", path);
        strcpy(strstr(path, ".tri"), ".tr2");
        if (!LpDataAccess->FileExists(path)) {
            PSXprintf("No Terrain found.  Terrain not created.\n");
            return 0;
        }
    } else {
        PSXprintf("IPU Compressed terrain '%s' FOUND in current folder\n", path);
    }
    int detail = m_x134 == 0 ? m_scene->m_manager->m_lowRes : 1;
    if (!(m_terrain = new Terrain(1))->Create(path, 1, 8, m_scene->m_manager->m_detailTexture, 0, detail, detail)) {
        PSXprintf("Failed to create the Terrain\n");
        return 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/LoadWater.cpp

extern "C" char *strcpy(char *dst, const char *src);
extern "C" int strcasecmp(const char *a, const char *b);
void PSXprintf(const char *fmt, ...);

class LightManager;



extern int gRenderFroth;




// An inlined strrchr(name, '_').
static inline char *FindLastUnderscore(char *s)
{
    char *start = s;
    while (*s++)
        ;
    do {
        s--;
    } while (s != start && *s != '_');
    if (*s == '_')
        return s;
    return 0;
}

int JetskiCompetition::LoadWater()
{
    char path[0x110];

    if (!strcasecmp(m_scene->m_manager->m_waterFile, "NONE")) {
        m_water = 0;
        PSXprintf("Water intentionally omitted in scene file.\n");
        return 0;
    }
    int level = gJetskiLevelTuning[gCurrentTunedLevel].m_waterLevel;
    int lodA;
    int lodB = 4;
    int c;
    float scale;
    switch (m_x134) {
    case 0:
        lodA = 0x55;
        scale = 2.0f;
        c = 0x78;
        gRenderFroth = 1;
        break;
    case 1:
        lodA = gWaterLOD[0][level].a;
        c = 0x5A;
        lodB = gWaterLOD[0][level].b;
        scale = 2.0f;
        gRenderFroth = gJetskiLevelTuning[gCurrentTunedLevel].m_renderFroth;
        break;
    case 2:
        lodA = gWaterLOD[1][level].a;
        c = 0x5A;
        lodB = gWaterLOD[1][level].b;
        scale = 2.0f;
        gRenderFroth = gJetskiLevelTuning[gCurrentTunedLevel].m_renderFroth;
        break;
    case 3:
    case 4:
    default:
        lodA = 0x1D;
        scale = 2.0f;
        lodB = 2;
        c = 0x5A;
        gRenderFroth = 0;
        break;
    }
    strcpy(path, m_scene->m_manager->m_waterFile);
    if (m_options->m_raceType == 4 || m_options->m_raceType == 5)
        strcpy(FindLastUnderscore(path), "_easy.wtt");
    m_water = (new PSXWater(1, lodA, lodB))->Create(path, m_lightManager, m_numPlayers, c, scale);
    if (!m_water) {
        PSXprintf("Water create failed in JetskiCompetition::LoadWater.\n");
        return 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/LoadTrackDefinition.cpp

extern "C" int sprintf(char *buf, const char *fmt, ...);
void PSXprintf(const char *fmt, ...);

extern char SceneContentDirectory[];








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

int JetskiCompetition::LoadTrackDefinition()
{
    char path[0x110];
    int skill;  // (uninitialized on the default path in retail too)

    sprintf(path, "%s%s", SceneContentDirectory, LpGame->m_sceneManager->m_sceneFile);
    // Replace the "_xxx" suffix with the skill's track file.
    switch (m_options->m_difficulty) {
    case 1:
        skill = 1;
        sprintf(FindLast(path, '_'), "_easy.tdf");
        break;
    case 2:
        skill = 2;
        sprintf(FindLast(path, '_'), "_easy.tdf");
        break;
    case 3:
        skill = 3;
        sprintf(FindLast(path, '_'), "_hard.tdf");
        break;
    default:
        PSXprintf("Invalid skill level setting in JetskiCompetition::LoadScene");
        break;
    }
    if (!m_track)
        m_track = (new TrackType(1))->Create(path, skill);
    else
        m_track->Reload(path);
    if (!m_track) {
        PSXprintf("Error Reading Track Data in JetskiCompetition::LoadTrackDefinition.\n");
        return 0;
    }
    if (!m_track->m_startGate) {
        PSXprintf("No start gate defined in JetskiCompetition::LoadTrackDefinition.\n");
        m_track->Release();
        m_track = 0;
        return 0;
    }
    if (!m_track->m_endGate) {
        PSXprintf("No end gate defined in JetskiCompetition::LoadTrackDefinition.\n");
        m_track->Release();
        m_track = 0;
        return 0;
    }
    if (m_water) {
        m_water->GetGridPointIntersection(&m_track->m_endGate->m_center, 0);
        m_water->GetGridPointIntersection(&m_track->m_startGate->m_center, 0);
        if (m_track->m_x58)
            m_water->GetGridPointIntersection(&m_track->m_x58->m_center, 0);
    }
    SetTrackHeights();
    return 1;
}


// ---- src/JetskiCompetition/LoadEcosystem.cpp
extern "C" int strcasecmp(const char *a, const char *b);
void PSXprintf(const char *fmt, ...);

class Terrain;





int JetskiCompetition::LoadEcosystem()
{
    if (strcasecmp(m_scene->m_manager->m_ecosystemFile, "NONE") == 0) {
        m_ecoSystem = 0;
        PSXprintf("Ecosystem intentionally omitted in scene file.\n");
        return 1;
    }
    m_ecoSystem = (new EcoSystem(1))->Create(m_terrain, m_scene->m_manager->m_ecosystemFile, 0x115C, 1, 9, 180.0f, 120.0f,
                                            m_scene->m_manager->m_placeEco);
    if (!m_ecoSystem)
        PSXprintf("No Ecosystem file found.  Ecosystem not created.\n");
    return 1;
}


// ---- src/JetskiCompetition/CreateBuoys.cpp
class SearchNode;
class LightManager;
class PSXWater;








int JetskiCompetition::CreateBuoys()
{
    if (m_scene->m_zoneModels && m_scene->m_zoneModels[0]) {
        for (int i = 0; i < m_scene->m_zoneModels[0]->m_numModels; i++) {
            StaticModel *entry = &m_scene->m_zoneModels[0]->m_models[i];
            if (entry->m_buoy)
                entry->m_buoy->Create(m_scene->m_staticNodes[0], m_scene->m_dynamicNodes[0], m_lightManager, m_water, entry->m_name);
        }
    }
    return 1;
}


// ---- src/JetskiCompetition/SetUpAnimations.cpp

extern "C" char *strstr(const char *s, const char *find);

extern const unsigned int WORLDOBJECT_PHYSICSBODY;
extern const unsigned int COLLISION_PROCEDURAL;

class SearchNode;
class Terrain;
class PSXWater;









// Hooks the scene's animated objects into collision detection.
void JetskiCompetition::SetUpAnimations()
{
    int i, j;
    if (!m_scene->m_animations)
        return;
    for (i = 0; i < m_scene->m_animations->m_count; i++) {
        if (m_scene->m_animations->m_animations[i].m_isCharacter) {
            CollisionCharacter *character = m_scene->m_animations->m_animations[i].m_character;
            character->m_collision->InitializeTransform();
            character->m_collision->SetTestListType((CollisionObjectTestListType)1);
            if (strstr(m_scene->m_animations->m_animations[i].m_name, "SH_SeaMonster")) {
                m_xD4 = i;
                m_scene->m_animations->m_animations[i].m_excludeFromCamera = 1;
            }
        } else if (m_scene->m_animations->m_animations[i].m_isCar) {
            CarProcedural *car = m_scene->m_animations->m_animations[i].m_car;
            car->m_terrain = m_terrain;
            car->m_testCollision->SetTestListType((CollisionObjectTestListType)1);
            for (j = 0; j < m_scene->m_animations->m_count; j++) {
                if (m_scene->m_animations->m_animations[j].m_isCharacter)
                    car->m_testCollision->AddToTestList(m_scene->m_animations->m_animations[j].m_character->m_collision);
                else if (m_scene->m_animations->m_animations[j].m_isCar)
                    car->m_testCollision->AddToTestList(m_scene->m_animations->m_animations[j].m_car->m_collision);
                else
                    car->m_testCollision->AddToTestList(m_scene->m_animations->m_animations[j].m_boat->m_collision);
            }
            car->m_collision->InitializeTransform();
            car->m_collision->SetOwner(car, WORLDOBJECT_PHYSICSBODY | COLLISION_PROCEDURAL);
            car->m_collision->m_ownerData = car;
            car->m_collision->SetTestListType((CollisionObjectTestListType)1);
        } else if (m_scene->m_animations->m_animations[i].m_isBoat) {
            BoatProcedural *boat = m_scene->m_animations->m_animations[i].m_boat;
            boat->m_terrain = m_terrain;
            boat->m_testCollision->SetTestListType((CollisionObjectTestListType)1);
            for (j = 0; j < m_scene->m_animations->m_count; j++) {
                if (m_scene->m_animations->m_animations[j].m_isCharacter)
                    boat->m_testCollision->AddToTestList(m_scene->m_animations->m_animations[j].m_character->m_collision);
                else if (m_scene->m_animations->m_animations[j].m_isCar)
                    boat->m_testCollision->AddToTestList(m_scene->m_animations->m_animations[j].m_car->m_collision);
                else
                    boat->m_testCollision->AddToTestList(m_scene->m_animations->m_animations[j].m_boat->m_collision);
            }
            boat->m_collision->InitializeTransform();
            boat->m_collision->SetOwner(boat, WORLDOBJECT_PHYSICSBODY | COLLISION_PROCEDURAL);
            boat->m_collision->m_ownerData = boat;
            boat->m_collision->SetTestListType((CollisionObjectTestListType)1);
            boat->m_water = m_water;
            boat->AddSearchPlacement(*m_scene->m_staticNodes, *m_scene->m_dynamicNodes);
        }
    }
}


// ---- src/JetskiCompetition/LoadFog.cpp
void PSXprintf(const char *fmt, ...);



struct SceneFog {
    unsigned int m_color;       // +0x0
    float m_visibility;         // +0x4
    float m_haziness;           // +0x8
};



int JetskiCompetition::LoadFog(int underwater)
{
    float maxFar;   // retail leaves it unset for an unknown mode
    float visibility = m_scene->m_fog->m_visibility;
    float haziness = m_scene->m_fog->m_haziness;

    if (underwater) {
        maxFar = 2600.0f;
    } else {
        switch (m_x134) {
        case 0:
            maxFar = 600.0f;
            break;
        case 1:
        case 2:
            maxFar = gJetskiLevelTuning[gCurrentTunedLevel].m_fogFar;
            if (maxFar < 0.0f)
                maxFar = 470.0f;
            if (gJetskiLevelTuning[gCurrentTunedLevel].m_fogVisibility > 0.0f)
                visibility = gJetskiLevelTuning[gCurrentTunedLevel].m_fogVisibility;
            if (gJetskiLevelTuning[gCurrentTunedLevel].m_fogHaziness > 0.0f)
                haziness = gJetskiLevelTuning[gCurrentTunedLevel].m_fogHaziness;
            break;
        case 3:
        case 4:
            maxFar = 430.0f;
            break;
        }
    }
    if (!(m_fog = (new Fog(1))->Create(m_scene->m_fog->m_color, visibility, haziness, maxFar, 64.0f, 64.0f))) {
        PSXprintf("Failed to create LpFog\n");
        return 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/LoadSkyCube.cpp

extern "C" int sprintf(char *buf, const char *fmt, ...);
void PSXprintf(const char *fmt, ...);

extern char SceneContentDirectory[];






int JetskiCompetition::LoadSkyCube()
{
    char file[0x110];
    char *sky = m_scene->m_manager->m_cubeFile;

    if (sky[0] == 0) {
        m_skyCube = 0;
        PSXprintf("Sky intentionally omitted from scene file. Sky not created.\n");
        return 1;
    }
    sprintf(file, "%s%s", SceneContentDirectory, sky);
    if (m_x118 > 1)
        DrawableCube::EnableVUSubdivision(0);
    else
        DrawableCube::EnableVUSubdivision(1);
    DrawableCube::EnableCubeZWrite(1);
    if (!(m_skyCube = (new SkyCube(m_enabled))->Create(file, 1024.0f))) {
        PSXprintf("Failed to create the Sky Cube\n");
        return 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/SetSceneFolder.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" int strcmp(const char *a, const char *b);
extern "C" char *strcpy(char *dst, const char *src);
void PSXprintf(const char *fmt, ...);

extern char SceneContentDirectory[];



// Reads this scene's folder from scenes.dat; it defaults to the scene name.
void JetskiCompetition::SetSceneFolder()
{
    char sceneName[0x110];
    char file[0x110];
    char block[0x80];
    ParameterBlock params;

    sprintf(file, "%sscenes.dat", SceneContentDirectory);
    if (!params.OpenFile(file, 0))
        return;
    sprintf(block, "Scene %d", m_options->m_x2C + 1);
    params.ReadParameterBlock(block);
    params.GetParameter("SceneName", sceneName, 0x104);
    params.GetParameter("SceneFolder", "NONE", m_sceneFolder, 0x40);
    if (strcmp(m_sceneFolder, "NONE") == 0) {
        PSXprintf("SceneName and ScneFolder must be the same.");  // (sic)
        strcpy(m_sceneFolder, sceneName);
    }
}


// ---- src/JetskiCompetition/LoadScene.cpp

extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" char *strcat(char *dst, const char *src);
void PSXprintf(const char *fmt, ...);

extern char SceneContentDirectory[];


class ParticleManager;
class AuralScape;





int JetskiCompetition::LoadScene()
{
    char sceneName[0x110];
    char listFile[0x110];
    char section[0x80];
    ParameterBlock block;

    sprintf(listFile, "%sscenes.dat", SceneContentDirectory);
    if (!block.OpenFile(listFile, 0)) {
        PSXprintf("Failed to load scene list in JetskiCompetition::LoadScene\n");
        return 0;
    }
    sprintf(section, "Scene %d", m_options->m_x2C + 1);
    block.ReadParameterBlock(section);
    block.GetParameter("SceneName", sceneName, 0x104);
    strcat(SceneContentDirectory, m_sceneFolder);
    strcat(SceneContentDirectory, "\\");
    switch (m_options->m_raceType) {
    case 1:
    case 2:
    case 3:
    case 6:
    case 7:
        switch (m_options->m_difficulty) {
        case 1:
        case 2:
            strcat(sceneName, "_easy");
            break;
        case 3:
            strcat(sceneName, "_hard");
            break;
        default:
            PSXprintf("Invalid skill level setting in JetskiCompetition::LoadScene");
            break;
        }
        break;
    case 5:
        strcat(sceneName, "_cc");
        break;
    case 4:
        strcat(sceneName, "_elim");
        break;
    case 8:
        strcat(sceneName, "_easy");
        break;
    case 0:
        break;
    }
    if (!(m_scene = (new Scene(m_enabled))->Create(LpGame->m_sceneManager, 0, 0)))
        return 0;
    LpGame->m_sceneManager->SetCurrentSceneFile(sceneName, "bpb", 0);
    if (!LpGame->m_sceneManager->m_sceneValid)
        LpGame->m_sceneManager->SetCurrentSceneFile(sceneName, "scn", 0);
    if (!m_scene->ReadSceneData(&m_particles, 0, 1, 1, 0)) {
        PSXprintf("Failed to load scene in JetskiCompetition::LoadScene\n");
        return 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/LoadRaceVehicles.cpp

extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" char *strcpy(char *dst, const char *src);

extern DataAccess *LpDataAccess;
extern MaterialStringTable *LpMaterialStringTable;
extern TextureMapStringTable *LpTextureMapStringTable;



#ifdef SPLASHDOWN_PC
// x86-64: Scene.h's EnableTrigger and Scene::m_enableTriggers, and the
// trigger's collision object (TriggerObject::m_collision)
#include "TriggerObject/TriggerObject.h"
struct SceneCollider {
    char *m_object;
    char *m_x04;
    int m_x08;
    int m_active;
    int m_x10;
    int m_x14;
};

struct SceneColliders {
    char pad[__builtin_offsetof(Scene, m_enableTriggers)];
    SceneCollider *m_colliders;
};
#define TRIGGER_COLLISION_OFFSET __builtin_offsetof(TriggerObject, m_collision)
#else
// m_scene->m_x85C: 0x18-byte entries of collidable scene objects.
struct SceneCollider {
    char *m_object;                     // +0x00, collision object at +0xC4
    int m_x04;
    int m_x08;
    int m_active;                       // +0x0C
    int m_x10;
    int m_x14;
};


struct SceneColliders {
    char pad[0x85C];
    SceneCollider *m_colliders;
};
#define TRIGGER_COLLISION_OFFSET 0xC4
#endif


// Creates the race's vehicles (player, AI and ghost) in racer-vehicle
// order, lined up in rows of four from the scene's start, with their skins
// (or ghost materials). With Steve (character 8) in the race the racers are
// split into teams first.
int JetskiCompetition::LoadRaceVehicles(int join)
{
    // Declared apart from their initialisation in this order (a statement-order search)
    // for retail's register allocation.
    int i;
    JetskiGameOptions *options;
    JetskiVehicle *player;
    int character;
    JetskiGhostVehicle *ghost;
    JetskiAIVehicle *av;
    int steve;
    int last;
    JetskiPlayerVehicle *pv;
    int ok;
    int skin;
    JetskiVehicle *gv;
    int cls;
    options = LpGame->m_gameOptions;
    ParameterBlock block1;
    ParameterBlock block2;
    _D3DVECTOR start;
    _D3DVECTOR direction;
    char resource[0x80];
    char riderPhysics[0x104];
    char key[0x104];
    char audio[0x104];
    char physics[0x104];
    int order[5] = {99, 99, 99, 99, 99};
    int index[5];
    int type[5];
    char sounds[3][0x104];
    char ghostMaterial1[0x80];
    char ghostMaterial2[0x80];
    char ghostBody[0x80];
    char ghostFace[0x80];
    ok = 1;

    strcpy(sounds[0], "vehicles\\SeaDooRX.aud");
    strcpy(sounds[1], "vehicles\\SeaDooXP.aud");
    strcpy(sounds[2], "vehicles\\SteveSWC.aud");

    steve = 0;
    for (i = 0; i < m_numVehicles; i++) {
        if (LpGame->m_gameOptions->m_racerCharacter[i] == 8) {
            steve = 1;
            break;
        }
    }
    for (i = 0; i < m_numVehicles; i++) {
        switch (LpGame->m_gameOptions->m_racerCharacter[i]) {
        case 0:
        case 1:
        case 4:
        case 5:
            type[i] = 0;
            cls = 2;
            break;
        case 2:
        case 3:
        case 6:
        case 7:
            type[i] = cls = 1;
            break;
        case 8:
            type[i] = 2;
            cls = 3;
            break;
        }
        if (steve) {
            if (m_x118 > i || i == 0) {
                (&m_x1FC)[i] = cls;
                if (i > 0) {
                    if (m_x1FC != cls)
                        m_x208++;
                } else {
                    m_x208++;
                }
                if (steve && cls != 3)
                    last = type[i];
            } else if (i == 1) {
                if (m_x1FC == 3) {
                    (&m_x1FC)[i] = cls;
                    if (cls != 3) {
                        last = type[i];
                        m_x208++;
                    }
                } else if (cls == 3) {
                    m_x200 = 3;
                    m_x208++;
                } else {
                    type[i] = last;
                }
            } else if (m_x208 > 1) {
                type[i] = last;
            } else if (cls == 3) {
                if (m_x1FC != cls)
                    m_x208++;
            } else if (m_x1FC == 3) {
                m_x200 = cls;
                m_x208++;
                last = type[i];
            } else {
                type[i] = last;
            }
        }
    }
    PSXprintf("DEBUG");

    // Racers in vehicle order.
    for (i = 0; i < m_numVehicles; i++) {
        int j = 0;
        while (order[j] <= m_options->m_racerVehicle[i] && j < m_numVehicles)
            j++;
        for (int m = m_numVehicles - 1; m > j; m--) {
            order[m] = order[m - 1];
            index[m] = index[m - 1];
        }
        order[j] = m_options->m_racerVehicle[i];
        index[j] = i;
    }

    for (i = 0; i < m_numVehicles; i++) {
        switch (m_options->m_racerCharacter[index[i]]) {
        case 0:
            strcpy(physics, "Vehicles\\SeaDooRX_Derrick.phy");
            strcpy(riderPhysics, "Riders\\DerrickPhy.phy");
            strcpy(audio, "Riders\\Derrick.aud");
            strcpy(key, "Riders\\DerrickKey.key");
            sprintf(resource, "CharRes\\Derrick.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 4:
            strcpy(physics, "Vehicles\\SeaDooGTX_Juana.phy");
            strcpy(riderPhysics, "Riders\\JuanaPhy.phy");
            strcpy(audio, "Riders\\Juana.aud");
            strcpy(key, "Riders\\JuanaKey.key");
            sprintf(resource, "CharRes\\Juana.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 2:
            strcpy(physics, "Vehicles\\SeaDooXP_Andreas.phy");
            strcpy(riderPhysics, "Riders\\AndreasPhy.phy");
            strcpy(audio, "Riders\\Andrea.aud");
            strcpy(key, "Riders\\AndreasKey.key");
            sprintf(resource, "CharRes\\Andreas.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 3:
            strcpy(physics, "Vehicles\\SeaDooSPX_Umi.phy");
            strcpy(riderPhysics, "Riders\\UmiPhy.phy");
            strcpy(audio, "Riders\\Umi.aud");
            strcpy(key, "Riders\\UmiKey.key");
            sprintf(resource, "CharRes\\Umi.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 1:
            strcpy(physics, "Vehicles\\SeaDooRX_Britt.phy");
            strcpy(riderPhysics, "Riders\\BrittPhy.phy");
            strcpy(audio, "Riders\\Brit.aud");
            strcpy(key, "Riders\\BrittKey.key");
            sprintf(resource, "CharRes\\Britt.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 5:
            strcpy(physics, "Vehicles\\SeaDooGTX_Sebas.phy");
            strcpy(riderPhysics, "Riders\\SebastianPhy.phy");
            strcpy(audio, "Riders\\Seb.aud");
            strcpy(key, "Riders\\SebastianKey.key");
            sprintf(resource, "CharRes\\Sebas.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 6:
            strcpy(physics, "Vehicles\\SeaDooSPX_Jeremy.phy");
            strcpy(riderPhysics, "Riders\\JeremyPhy.phy");
            strcpy(audio, "Riders\\Jeremy.aud");
            strcpy(key, "Riders\\JeremyKey.key");
            sprintf(resource, "CharRes\\Jeremy.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 7:
            strcpy(physics, "Vehicles\\SeaDooXP_AmmanRa.phy");
            strcpy(riderPhysics, "Riders\\AmmanRaPhy.phy");
            strcpy(audio, "Riders\\Amman.aud");
            strcpy(key, "Riders\\AmmanRaKey.key");
            sprintf(resource, "CharRes\\AmmanRa.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 8:
            strcpy(physics, "Vehicles\\SteveDooSWC_Steve.phy");
            strcpy(riderPhysics, "Riders\\StevePhy.phy");
            strcpy(audio, "Riders\\Steve.aud");
            strcpy(key, "Riders\\SteveKey.key");
            sprintf(resource, "CharRes\\Steve.res");
            LpDataAccess->LoadResourceFile(resource, 1, 0, 0, 0, 0, 0, 0);
            break;
        }
        // Rows of four: each row starts back at the start line.
        // The doubled assignments reproduce retail's doubled stores.
        if (index[i] % 4 == 0) {
            start.x = start.x = m_scene->m_startPosition.x;
            start.z = start.z = m_scene->m_startPosition.z;
            start.x = start.x = m_scene->m_startPosition.x - m_scene->m_startDirection.x * (12.0f * (index[i] / 4));
            start.z = start.z = m_scene->m_startPosition.z - m_scene->m_startDirection.z * (12.0f * (index[i] / 4));
        }
        switch (m_options->m_racerActive[index[i]]) {
        case 0: {
            static int event10 = -1;
            if (!index[i])
                last = 0;
            else
                last = 1;
            if ((pv = new JetskiPlayerVehicle(1))->Create(this, physics, riderPhysics, key, audio, sounds[type[index[i]]], m_lightManager,
                                m_particles, start, direction, m_terrain, m_water, last, index[i], m_scene)) {
                if (m_raceStatus)
                    pv->m_ownStatus = m_raceStatus->AddVehicle(pv, 4, index[i]);
                m_vehicles[index[i] + 5] = pv;
                m_vehicles[index[i]] = pv;
                m_vehicles[index[i]]->m_x1E0 = join;
                pv->SetCompetition(this);
                if (m_options->m_raceType == 3)
                    player = pv;
            } else {
                ok = 0;
            }
            break;
        }
        case 1: {
            static int event10 = -1;
            av = new JetskiAIVehicle(1);
            if (!av) {
                Release();
                return 0;
            }
            if ((av = av->Create(this, physics, riderPhysics, key, audio, sounds[type[index[i]]],
                                           m_lightManager, m_particles, start, direction, m_terrain, m_water,
                                           index[i], m_scene, m_raceStatus, m_track)) != 0) {
                if (m_raceStatus) {
                    av->m_ownStatus = m_raceStatus->AddVehicle(av, 8, index[i]);
                    av->m_ai->m_status = av->m_ownStatus;
                }
                m_vehicles[index[i]] = av;
                m_vehicles[index[i]]->m_x1E0 = join;
                if (m_options->m_difficulty == 1) {
                    if (m_track)
                        av->m_engine->m_maxRPM = av->m_engine->m_maxRPM * m_track->m_tuning.z;
                }
                for (last = 0; last < m_scene->m_numEnableTriggers; last++) {
                    if (((SceneColliders *)m_scene)->m_colliders[last].m_active)
                        av->m_xA4->AddToTestList(*(CollisionObject **)(((SceneColliders *)m_scene)->m_colliders[last].m_object + TRIGGER_COLLISION_OFFSET));
                }
            } else {
                ok = 0;
            }
            break;
        }
        case 2: {
            if ((gv = new JetskiVehicle(1))->Create(this, physics, riderPhysics, key, audio, sounds[type[index[i]]], start, direction,
                                ZeroVector, ZeroVector, m_lightManager, m_particles, m_terrain, m_water, index[i],
                                m_scene)) {
                gv->m_x584 = 2;
                gv->m_body->m_enabled = 0;
                gv->m_x1CC = 1;
                m_vehicles[index[i]] = gv;
                m_vehicles[index[i]]->m_x1E0 = join;
                m_ghost = (JetskiGhostVehicle *)gv;
            } else {
                ok = 0;
            }
            break;
        }
        default:
            PSXprintf("Invalid racer type specified in JetskiCompetition::LoadRaceVehicles\n");
            return 0;
        }
        if (m_options->m_x48 == 2) {
            start.x = start.x = start.x + 6.0f * m_scene->m_startDirection.z;
            start.z = start.z = start.z + 6.0f * m_scene->m_startDirection.x;
        } else {
            start.x = start.x = start.x - 6.0f * m_scene->m_startDirection.z;
            start.z = start.z = start.z - 6.0f * m_scene->m_startDirection.x;
        }
        if (ok) {
        if (m_options->m_racerActive[index[i]] == 2) {
            LpDataAccess->DropDevice(resource, 1);
            LpDataAccess->LoadResourceFile("CharRes\\bs_saucr.res", 1, 0, 0, 0, 0, 0, 0);
            switch (m_options->m_racerCharacter[index[i]]) {
            case 0:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooRX_Derr_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooRX_Derr_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\DerrBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\DerrFace_Ghost");
                break;
            case 4:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooGTX_Juana_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooGTX_Juana_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\JuanaBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\JuanaFace_Ghost");
                break;
            case 2:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooXP_Andreas_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooXP_Andreas_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\AndreasBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\AndreasFace_Ghost");
                break;
            case 3:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooSPX_Umi_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooSPX_Umi_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\UmiBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\UmiFace_Ghost");
                break;
            case 1:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooRX_Britt_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooRX_Britt_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\BrittBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\BrittFace_Ghost");
                break;
            case 5:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooGTX_Sebas_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooGTX_Sebas_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\SebastianBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\SebastianFace_Ghost");
                break;
            case 6:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooSPX_Jeremy_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooSPX_Jeremy_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\JeremyBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\JeremyFace_Ghost");
                break;
            case 7:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SeaDooXP_Ammanra_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SeaDooXP_Ammanra_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\AmmanraBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\AmmanraFace_Ghost");
                break;
            case 8:
                strcpy(ghostMaterial1, "Scenes\\Geometry\\Textures\\SWC_1_Ghost");
                strcpy(ghostMaterial2, "Scenes\\Geometry\\Textures\\SWC_2_Ghost");
                strcpy(ghostBody, "Scenes\\Geometry\\Textures\\SteveBody_Ghost");
                strcpy(ghostFace, "Scenes\\Geometry\\Textures\\SteveFace_Ghost");
                break;
            }
            // (retail reads `character` here though only the other branch sets it)
            ((RenderableObject *)m_vehicles[index[i]]->m_rider->m_root)->ChangeMaterial(options->m_riderSkins[character][0][0],
                                                                        LpMaterialStringTable->GetIndex(ghostBody));
            ((RenderableObject *)m_vehicles[index[i]]->m_rider->m_root)->ChangeMaterial(options->m_riderSkins[character][1][0],
                                                                        LpMaterialStringTable->GetIndex(ghostFace));
            m_vehicles[index[i]]->m_renderable->ChangeMaterial(options->m_jetskiSkins[character][0][0],
                                                               LpMaterialStringTable->GetIndex(ghostMaterial1));
            m_vehicles[index[i]]->m_renderable->ChangeMaterial(options->m_jetskiSkins[character][1][0],
                                                               LpMaterialStringTable->GetIndex(ghostMaterial2));
            LpMaterialStringTable->LoadRequested();
            LpTextureMapStringTable->LoadRequested();
            LpDataAccess->DropDevice("CharRes\\bs_saucr.res", 1);
        } else {
            character = options->m_racerCharacter[index[i]];
            skin = options->m_racerSkin[index[i]];
            if (skin) {
                ((RenderableObject *)m_vehicles[index[i]]->m_rider->m_root)->ChangeMaterial(options->m_riderSkins[character][0][0],
                                                                            options->m_riderSkins[character][0][skin]);
                ((RenderableObject *)m_vehicles[index[i]]->m_rider->m_root)->ChangeMaterial(options->m_riderSkins[character][1][0],
                                                                            options->m_riderSkins[character][1][skin]);
                m_vehicles[index[i]]->m_renderable->ChangeMaterial(options->m_jetskiSkins[character][0][0],
                                                                   options->m_jetskiSkins[character][0][skin]);
                m_vehicles[index[i]]->m_renderable->ChangeMaterial(options->m_jetskiSkins[character][1][0],
                                                                   options->m_jetskiSkins[character][1][skin]);
            }
            LpMaterialStringTable->LoadRequested();
            LpTextureMapStringTable->LoadRequested();
            LpDataAccess->DropDevice(resource, 1);
        }
        } else {
            LpDataAccess->DropDevice(resource, 1);
        }
    }
    if (m_raceStatus && (ghost = m_ghost) != 0 && m_options->m_raceType == 3) {
        if (m_options->m_x54 == 2)
            ((JetskiVehicle *)ghost)->m_ownStatus = m_raceStatus->AddGhostVehicle(
                player->m_ownStatus, (JetskiVehicle *)ghost, 512);
        else
            *(int *)ghost = (int)m_raceStatus->AddGhostVehicle(player->m_ownStatus,
                                                              (RaceStatusVehicleInterface *)ghost, 512);
    }
    if (!LoadGenericVehicleSounds("vehicles\\genvehsd.aud")) {
#line 3399 "JetskiCompetition.cpp"
        ASSERT(0, "Error loading generic jetski vehicle sounds\n");
    }
    if (m_x118 > 1) {
        float bias2 = 0.4f;     // a local: retail loads it first
        RenderableObject::SetRenderableLODBias(0.6f, bias2);
        PSXRenderableObject::SetMultipassCullDistance(15.0f);
    } else {
        RenderableObject::SetRenderableLODBias(1.0f, 1.0f);
        PSXRenderableObject::SetMultipassCullDistance(25.0f);
    }
    return ok;
}


// ---- src/JetskiCompetition/LoadCameras.cpp


class Scene;







int JetskiCompetition::LoadCameras(int numPlayers, int player)
{
    switch (numPlayers) {
    case 1:
        LpGame->m_paused = 0;
        break;
    case 2:
        LpGame->m_flag0 = 0;
        LpGame->m_paused = 0;
        break;
    case 3:
        LpGame->m_flag0 = 0;
        LpGame->m_paused = 0;
        break;
    case 4:
        LpGame->m_flag0 = 0;
        LpGame->m_paused = 0;
        break;
    default:
#line 3443 "JetskiCompetition.cpp"
        ASSERT(0, "Invalid number of players passed into LoadCameras.");
        break;
    }
    if (!(m_cameras[0] = new PSXCamera(1))->Create()) {
        PSXprintf("Failed to create LpCameras[0]\n");
        return 0;
    }
    if (numPlayers > 1) {
        if (!(m_cameras[1] = new PSXCamera(1))->Create()) {
            PSXprintf("Failed to create LpCamera[1]\n");
            return 0;
        }
    }
    if (!(m_cameraControllers[0] = new JetskiCameraController(1))->Create((Camera *)m_cameras[0], (ViewPortCode)m_x134, m_scene, player)) {
        PSXprintf("Failed to create JetskiCameraControllers[0]\n");
        return 0;
    }
    if (numPlayers > 1) {
        Camera *camera = m_cameras[1] ? m_cameras[1] : m_cameras[0];
        if (!(m_cameraControllers[1] = new JetskiCameraController(1))->Create(camera, (ViewPortCode)m_x134, m_scene, player)) {
            PSXprintf("Failed to create JetskiCameraControllers[1]\n");
            return 0;
        }
    }
    return 1;
}


// ---- src/JetskiCompetition/CreateListeners.cpp




void JetskiCompetition::CreateListeners(int count)
{
    // The local game pointer keeps retail's schedule with the current Game.h;
    // this function reschedules with almost any change to the headers it sees
    // (with SetPointersForVCR declared in JetskiCompetition.h, `i` must be
    // declared in the for statement).
    for (int i = 0; i < count; i++) {
        Game *g = LpGame;
        g->m_auralScape->AddListener(i, 2.5f, 1.0f);
    }
}


// ---- src/JetskiCompetition/UpdateListeners.cpp







void JetskiCompetition::UpdateListeners()
{
    if (LpGame->m_auralScape) {
        for (int i = 0; i < m_numPlayers; i++) {
            if (m_cameras[i])
                LpGame->m_auralScape->UpdateListener(i, &m_cameras[i]->m_position, &m_cameras[i]->m_lookAt,
                                                     &m_cameras[i]->m_up, &ZeroVector);
        }
    }
}


// ---- src/JetskiCompetition/DestroyListeners.cpp




void JetskiCompetition::DestroyListeners(int count)
{
    for (int i = 0; i < count; i++)
        LpGame->m_auralScape->RemoveListener(i);
}


// ---- src/JetskiCompetition/RenderPre3D.cpp
int JetskiCompetition::RenderPre3D()
{
    if (m_x134)
        SetupViewport(LpGame->m_currentPlayer);
    return GameObject::RenderPre3D();
}

// ---- src/JetskiCompetition/SetupCamerasAndViewports.cpp





// Hand the cameras to the game and split the screen between the players.
void JetskiCompetition::SetupCamerasAndViewports()
{
    switch (m_x134) {
    case 0:
        LpGame->SetCameras(m_cameras[0], 0, 0, 0);
        m_cameras[0]->ResizeViewport(0, 0, m_cameras[0]->m_surfaceWidth, m_cameras[0]->m_surfaceHeight);
        break;
    case 1:
        LpGame->SetCameras(m_cameras[0], m_cameras[1], 0, 0);
        m_cameras[0]->ResizeViewport(0, 0, m_cameras[0]->m_surfaceWidth, m_cameras[0]->m_surfaceHeight / 2);
        m_cameras[1]->ResizeViewport(0, m_cameras[1]->m_surfaceHeight / 2, m_cameras[1]->m_surfaceWidth, m_cameras[1]->m_surfaceHeight / 2);
        break;
    case 2:
        LpGame->SetCameras(m_cameras[0], m_cameras[1], 0, 0);
        m_cameras[0]->ResizeViewport(0, 0, m_cameras[0]->m_surfaceWidth / 2, m_cameras[0]->m_surfaceHeight);
        m_cameras[1]->ResizeViewport(m_cameras[1]->m_surfaceWidth / 2, 0, m_cameras[1]->m_surfaceWidth / 2, m_cameras[1]->m_surfaceHeight);
        break;
    default:
#line 3551 "JetskiCompetition.cpp"
        ASSERT(0, "Invalid PlayerViewportCode passed into SetupCamerasAndViewports");
        break;
    }
}


// ---- src/JetskiCompetition/SetupViewport.cpp




// Split the screen between the players.
void JetskiCompetition::SetupViewport(int player)
{
    switch (m_x134) {
    case 0:
        m_cameras[0]->ResizeViewport(0, 0, m_cameras[0]->m_surfaceWidth, m_cameras[0]->m_surfaceHeight);
        break;
    case 1:
        if (player == 0)
            m_cameras[0]->ResizeViewport(0, 0, m_cameras[0]->m_surfaceWidth, m_cameras[0]->m_surfaceHeight / 2);
        else
            m_cameras[1]->ResizeViewport(0, m_cameras[1]->m_surfaceHeight / 2, m_cameras[1]->m_surfaceWidth, m_cameras[1]->m_surfaceHeight / 2);
        break;
    case 2:
        if (player == 0)
            m_cameras[0]->ResizeViewport(0, 0, m_cameras[0]->m_surfaceWidth / 2, m_cameras[0]->m_surfaceHeight);
        else
            m_cameras[1]->ResizeViewport(m_cameras[1]->m_surfaceWidth / 2, 0, m_cameras[1]->m_surfaceWidth / 2, m_cameras[1]->m_surfaceHeight);
        break;
    default:
#line 3588 "JetskiCompetition.cpp"
        ASSERT(0, "Invalid PlayerViewportCode passed into SetupViewport");
        break;
    }
}


// ---- src/JetskiCompetition/AttachScene.cpp
void PSXprintf(const char *fmt, ...);



class JetskiInGameOverlayContainer : public GameObject {
public:
    virtual ~JetskiInGameOverlayContainer();
    JetskiInGameOverlayContainer(int enabled);
};








int JetskiCompetition::AttachScene()
{
    int i;
    if (m_waterDisplacement) {
        if (!AddChild(m_waterDisplacement, -1)) {
            PSXprintf("Failed to attach LpWaterDisplacement in JetskiCompetition::AttachScene\n");
            return 0;
        }
        m_waterDisplacement->m_params = (WaterDisplacementParams *)m_fog;
    } else {
        PSXprintf("Cannot attach LpWaterDisplacement, it was never created.\n");
    }
    if (m_skyCube) {
        if (!AddChild(m_skyCube, -1)) {
            PSXprintf("Failed to attach LpSkyCube in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpSkyCube, it was never created.\n");
    }
    if (m_fog) {
        if (!AddChild(m_fog, -1)) {
            PSXprintf("Failed to attach LpFog in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpFog, it was never created.\n");
    }
    if (m_terrain) {
        if (!AddChild(m_terrain, -1)) {
            PSXprintf("Failed to attach LpTerrain in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpTerrain, it was never created.\n");
    }
    if (m_scene) {
        if (!AddChild(m_scene, -1)) {
            PSXprintf("Failed to attach LpScene in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpScene, it was never created.\n");
    }
    if (m_cameras[0]) {
        if (!AddChild(m_cameras[0], -1))
            PSXprintf("Failed to attach LpCamera[0]\n");
    } else {
        PSXprintf("Cannot attach LpCamera[0], it was never created.\n");
    }
    if (m_numPlayers > 1) {
        if (m_cameras[1]) {
            if (!AddChild(m_cameras[1], -1))
                PSXprintf("Failed to attach LpCamera[1]\n");
        } else {
            PSXprintf("Cannot attach LpCamera[1], it was never created.\n");
        }
    }
    for (i = 0; i < m_numVehicles; i++) {
        if (!AddChild(m_vehicles[i], -1)) {
            PSXprintf("Failed to attach LpJetskiVehicles in JetskiCompetition::AttachScene\n");
            return 0;
        }
    }
    if (m_vcr) {
        if (!AddChild(m_vcr, -1)) {
            PSXprintf("Failed to attach LpJetskiVCRController in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpJetskiVCRController, it was never created.\n");
    }
    if (m_raceStatus)
        AddChild(m_raceStatus, -1);
    if (m_water) {
        if (!AddChild(m_water, 0x402)) {
            PSXprintf("Failed to attach LpWater in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpWater, it was never created.\n");
    }
    if (m_scene->m_zoneModels && m_scene->m_zoneModels[0] && m_scene->m_numBuoys > 0) {
        for (i = 0; i < m_scene->m_zoneModels[0]->m_numModels; i++) {
            if (m_scene->m_zoneModels[0]->m_models[i].m_buoy) {
                if (!AddChild(m_scene->m_zoneModels[0]->m_models[i].m_buoy, 0)) {
                    PSXprintf("Failed to attach BuoyObject in JetskiCompetition::AttachScene\n");
                    return 0;
                }
            }
        }
    }
    if (m_ecoSystem) {
        if (!AddChild(m_ecoSystem, 0)) {
            PSXprintf("Failed to attach LpEcoSystem in JetskiCompetition::AttachScene\n");
            return 0;
        }
    } else {
        PSXprintf("Cannot attach LpEcoSystem, it was never created.\n");
    }
    if (!AddChild(m_particles, -1)) {
        PSXprintf("Failed to attach LpParticleManager in JetskiCompetition::AttachScene\n");
        return 0;
    }
    if (m_cameraControllers[0]) {
        if (!AddChild(m_cameraControllers[0], 2))
            PSXprintf("Failed to attach LpJetskiCameraControllers[0]\n");
    } else {
        PSXprintf("Cannot attach LpJetskiCameraControllers[0], it was never created.\n");
    }
    if (m_numPlayers > 1) {
        if (m_cameraControllers[1]) {
            if (!AddChild(m_cameraControllers[1], 2))
                PSXprintf("Failed to attach LpJetskiCameraControllers[1]\n");
        } else {
            PSXprintf("Cannot attach LpJetskiCameraControllers[1], it was never created.\n");
        }
    }
    if (m_videoFX)
        AddChild(m_videoFX, 0xC);
    GameObject *overlays = (new JetskiInGameOverlayContainer(1))->Create();
    AddChild(overlays, -1);
    if (m_inGameStats)
        overlays->AddChild(m_inGameStats, -1);
    if (m_infoText)
        overlays->AddChild(m_infoText, -1);
    if (m_headingArrows)
        overlays->AddChild(m_headingArrows, -1);
    if (m_countDown)
        overlays->AddChild(m_countDown, -1);
    if (m_separator)
        overlays->AddChild(m_separator, -1);
    if (m_performanceMeter)
        overlays->AddChild(m_performanceMeter, -1);
    overlays->AddChild(m_iconManager, -1);
    overlays->AddChild(m_x34, -1);
    overlays->AddChild(m_textManager, -1);
    if (m_inGameMenu)
        LpGame->m_root->AddChild(m_inGameMenu, -1);
    if (m_raceStartMenu)
        LpGame->m_root->AddChild(m_raceStartMenu, -1);
    if (m_raceOverMenu)
        LpGame->m_root->AddChild(m_raceOverMenu, -1);
    if (m_nameMenu)
        LpGame->m_root->AddChild(m_nameMenu, -1);
    if (m_promptMenu)
        LpGame->m_root->AddChild(m_promptMenu, -1);
    if (m_radar)
        overlays->AddChild(m_radar, -1);
    return 1;
}


// ---- src/JetskiCompetition/EventComplete.cpp

void JetskiCompetition::EventComplete()
{
}


// ---- src/JetskiCompetition/SetPlayback.cpp
// SetPlayback__17JetskiCompetitionF12PlaybackType  @ 0x0024CA10

class SoultreeObject;















// Heap shim: flags
// the "has call" bytes of 3.0f, 70.0f and two int constants as in the
// per-function file (so the constants load before the camera pointer) and
// clears the rest of the epoch. Offsets from image.py on the truncated TU.
struct MwccHeapImage3 {
    char pad0[0x155];
    char flag0;
    char pad1[0x13F];
    char flag1;
    char pad2[0x437];
    char flag2;
    char pad3[0x30F];
    char flag3;
    char tail[0xBB22];
};
#pragma section mwccshim begin
inline void MwccHeapShim3() { const MwccHeapImage3 image = { {0}, 1, {0}, 1, {0}, 1, {0}, 1 }; }
#pragma section mwccshim end

void JetskiCompetition::SetPlayback(PlaybackType type)
{
    // SetSoultreeObject's constants as function-scope locals: retail loads
    // them before the camera and player pointers.
    float a = 3.0f;
    float b = 70.0f;
    if (m_vcr) {
        m_x290 = 1;
        m_playbackType = type;
        if (type == 1) {
            for (int i = 0; i < m_options->m_numRacers; i++) {
                m_vehicles[i]->m_x1CC = 1;
                m_vehicles[i]->StopWreck(0);
                m_vehicles[i]->ResetPosition(1);
            }
            if (m_cameraControllers[0]->m_tvCamera) {
                m_cameraControllers[0]->m_tvCamera->ResetTrace();
                m_cameraControllers[0]->SwitchCamera((CameraType)4);
                m_cameraType = 4;
            }
            m_x298 = m_cameraControllers[0]->m_tvCamera ? m_cameraControllers[0]->m_tvCamera->m_switchEvery : 5.0f;
        } else if (type == 2 && m_options->m_x54 != 2) {
            m_cameraControllers[0]->m_followCamera->SetSoultreeObject(m_ghost->m_object, a, b);
        }
        m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
        m_cameraControllers[0]->m_followCamera->SwitchFilterValueForPlayback(1);
        m_vcr->REWforPLAY();
        m_status = 10;
        m_x2C0 = 0;
        m_x2C4 = 0;
        m_x2C8 = 0;
        m_water->ResetWakes();
        m_waterDisplacement->m_time = m_waterTime;
        m_water->SetBaseTime(m_baseTime);
        m_inGameStats->SetTotalRaceTime(0);
        m_inGameStats->SetRecordLapTime(0);
        m_inGameStats->SetBestLap(0);
        m_inGameStats->SetLapTime(0);
        m_inGameStats->SetTimeBehindLeader(0);
        m_inGameStats->SetPosition(0);
        m_inGameStats->SetLap(0);
        m_inGameStats->SetCountdownText(0);
        m_inGameStats->SetScoreText(0);
        m_inGameStats->SetNewRecordText(0);
        m_inGameStats->SetWrongWayText(0);
        m_inGameStats->SetReplayText(1);
        m_inGameStats->SetWings(0, 0);
        m_inGameStats->SetShowing(1, 0);
    }
}


// ---- src/JetskiCompetition/ResetCameraControllers.cpp




void JetskiCompetition::ResetCameraControllers()
{
    for (int i = 0; i < m_numPlayers; i++) {
        if (m_cameraControllers[i]->m_cameraType != 2) {
            m_cameraControllers[i]->SwitchCamera((CameraType)0);
            m_cameraControllers[i]->SwitchFollowCameraPresets((FollowCameraPresetType)m_cameraControllers[i]->m_presetID);
            m_cameraControllers[i]->m_followCamera->m_resetFilters = 1;
        }
        m_cameraControllers[i]->StopRotate();
    }
}


// ---- src/JetskiCompetition/InitializeRace.cpp
// Notes: the start-height checks in the track cases borrow z and x (function-scope
// floats) for the ground height; those earlier live ranges turn the grid x/z into
// split temporaries, which gives retail's FP registers for side/x/z.





// _D3DVECTOR has += but no -= (include/d3dtypes.h).
static inline void operator-=(_D3DVECTOR &a, const _D3DVECTOR &b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
}

int JetskiCompetition::InitializeRace()
{
    _D3DVECTOR position = m_scene->m_startPosition;
    _D3DVECTOR direction = m_scene->m_startDirection;
    float back;
    float side;
    int i;
    float height;
    float x;                            // also the ground height in cases 1/2/6/7
    float z;                            // also the ground height in case 3

    back = side = 0.0f;
    m_x24C = 1;
    m_x138 = 4.5f;
    m_joinTimer = 0;
    m_joinState = 0;
    for (i = 0; i < m_x118; i++)
        m_vehicles[i]->m_player = i;
    for (i = 0; i < m_numPlayers; i++) {
        if (m_options->m_raceType == 5 && i != 0)
            continue;
        m_cameraControllers[i]->SetJetski(m_vehicles[i], i);
        if (m_cameraControllers[i]->m_player < 0) {
            m_cameraControllers[i]->Initialize(m_terrain, m_water, i, i);
            m_cameras[i]->AddSearchPlacement(*m_scene->m_staticNodes, 2);
        }
    }
    if (m_waterDisplacement) {
        for (int i = 0; i < m_numPlayers; i++)
            m_waterDisplacement->m_vehicles[i] = m_vehicles[m_cameraControllers[i]->m_x64]->m_body->m_object;
    }
    if (m_water)
        ((Water *)m_water)->ResetWakes();
    if (m_particles)
        m_particles->ClearParticles();
    switch (m_options->m_raceType) {
    case 3:
        if (m_ghost) {
            m_ghostVisible = 0;
            if (m_options->m_x54 == 2)
                ((JetskiVehicle *)m_ghost)->DisableAll();
            else
                m_ghost->m_object->Disable();
        }
        if (m_track) {
            if (m_raceStatus && m_track->m_x58) {
                position = m_track->m_x58->m_center;
                m_track->GetTrackDirection(m_track->m_x74.m_data, &direction);
            } else {
                position = m_track->m_startGate->m_center;
                m_track->GetTrackDirection(m_track->m_startTrackPosition.m_data, &direction);
                position -= 20.0f * direction;
            }
            if (m_terrain)
                m_terrain->GetGridPointIntersection(&position, 0, 0, 0);
            z = position.y;
            if (m_water)
                ((Water *)m_water)->GetGridPointIntersection(&position, 0);
            if (z > position.y)
                position.y = z;
        }
        break;
    case 1:
    case 2:
    case 6:
    case 7:
        if (m_track) {
            position = m_track->m_startGate->m_center;
            m_track->GetTrackDirection(m_track->m_startTrackPosition.m_data, &direction);
            if (m_terrain)
                m_terrain->GetGridPointIntersection(&position, 0, 0, 0);
            x = position.y;
            if (m_water)
                ((Water *)m_water)->GetGridPointIntersection(&position, 0);
            if (x > position.y)
                position.y = x;
        }
        break;
    case 5:
        m_vehicles[1]->DisableAll();
        m_raceStatus->SortVehiclesByID();
    case 4:
    case 8:
        position = m_scene->m_startPosition;
        direction = m_scene->m_startDirection;
        break;
    }
    height = position.y;
    for (i = 0; i < m_numVehicles; i++) {
        JetskiVehicle *vehicle = m_vehicles[i];
        if (m_options->m_raceType != 5) {
            switch ((&m_x334)[i]) {
            case 1:
                side = 0.0f;
                back = side;
                break;
            case 2:
                side = 6.0f;
                back = 20.0f;
                break;
            case 3:
                side = -6.0f;
                back = 40.0f;
                break;
            case 4:
                side = 12.0f;
                back = 60.0f;
                break;
            case 5:
                side = -12.0f;
                back = 80.0f;
                break;
            }
        }
        if (m_options->m_x48 == 2)
            side *= -1.0f;
        x = position.x + side * direction.z - back * direction.x;
        z = position.z - side * direction.x - back * direction.z;
        if (vehicle->m_x530 || vehicle->m_rider->m_wrecker->m_wreckType != 0.0f)
            vehicle->StopWreck(1);
        vehicle->m_resetPosition = _D3DVECTOR(x, height, z);
        vehicle->m_resetOrientation = direction;
        vehicle->m_resetUp = YVector;
        vehicle->m_x220 = _D3DVECTOR(x, height, z);
        vehicle->m_x230 = direction;
        vehicle->ResetPosition(1);
        if (vehicle->m_ownStatus->m_flags & 8)
            ((VehicleAIClass *)((JetskiAIVehicle *)vehicle)->m_ai)->ResetAI();
    }
    ResetCameraControllers();
    LpGame->m_paused = 0;
    if (m_xD4 >= 0 && m_xD4 < *(int *)m_scene->m_animations)
        m_scene->RewindAnimation(m_xD4, 0.0001f, 1, 0);
    m_raceStatus->ResetRaceStatus();
    if (m_options->m_raceType != 5 && m_options->m_raceType != 3) {
        for (i = 0; i < m_numVehicles; i++)
            m_vehicles[i]->m_ownStatus->m_place = (&m_x334)[i];
    }
    return 1;
}


// ---- src/JetskiCompetition/RestartRace.cpp

extern "C" void *memset(void *dst, int c, unsigned int n);
void PSXprintf(const char *fmt, ...);











int JetskiCompetition::RestartRace(GamePlayStatus status)
{
    int i;

    if (m_options->m_raceType != 3)
        ResetVCR();
    m_status = status;
    LpGame->m_auralScape->StopAllAuralScapeSounds();
    m_cameraControllers[0]->m_followCamera->SwitchFilterValueForPlayback(0);
    if (!InitializeRace()) {
        PSXprintf("JetskiCompetition::RestartRace:  InitializeRace FAILED!\n");
        return 0;
    }
    for (i = 0; i < m_x118; i++) {
        if (m_vehicles[i]) {
            m_vehicles[i]->m_ownStatus->m_onTrack = 1;
            m_vehicles[i]->m_wrongWay = 0;
        }
    }
    if (m_options->m_raceType == 4)
        ResetTimeElimBuoys(1);
    for (i = 0; i < m_numVehicles; i++) {
        memset(m_vehicles[i]->m_rider->m_x404, 0, sizeof(m_vehicles[i]->m_rider->m_x404));
        m_vehicles[i]->m_x7A4 = 0;
    }
    return 1;
}


// ---- src/JetskiCompetition/DoingJoinAnimation.cpp
extern int TimerOverflowCounterT0;

extern "C" char *strncpy(char *dst, const char *src, unsigned int n);
extern "C" char *strcat(char *dst, const char *src);

// Win32-style timeGetTime: EE timer 0 (T0_COUNT at 0x10000000) ticks plus the
// overflows counted by TimerHandlerT0, accumulated and scaled to milliseconds.
inline unsigned int timeGetTime()
{
    static long Time = 0;
    static int lastCount = 0;
    int count;
    int elapsed;
    int overflows;
    count = *(volatile int *)0x10000000;
    overflows = TimerOverflowCounterT0;
    TimerOverflowCounterT0 = 0;
    if (lastCount <= count)
        elapsed = count - lastCount;
    else
        elapsed = 0x10000 - lastCount + count;
    if (lastCount >= count && overflows > 0)
        overflows--;
    lastCount = count;
    Time += elapsed + (overflows << 16);
    return 0.0017066667f * Time;
}















int JetskiCompetition::DoingJoinAnimation()
{
    static _D3DVECTOR SavedPosition;
    switch (m_joinState) {
    case 1: {
        _D3DVECTOR velocity = ZeroVector;
        char action[0x20];
        m_fadeOutTime[0] = 180;
        m_joinTime = timeGetTime();
        m_vehicles[1]->m_rider->ResetVariables();
        m_vehicles[1]->ResetPosition(1);
        m_vehicles[1]->m_body->m_velocity = velocity;
        m_cameraControllers[0]->SetJetski(m_vehicles[1], 1);
        strncpy(action, m_vehicles[1]->m_rider->m_riderName, 16);
        strcat(action, "_Join");
        m_vehicles[1]->m_rider->PerformActionByName(action);
        m_joinState = 3;
        break;
    }
    case 3:
        if (m_joinTime + 220 < timeGetTime()) {
            _D3DVECTOR position = ZeroVector;
            m_cameraControllers[0]->ToggleFreezeRotate();
            m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
            m_vehicles[0]->m_body->m_object->GetPosition(&SavedPosition);
            m_vehicles[0]->m_body->m_object->SetPosition(0, &position);
            m_fadeInTime[0] = 180;
            m_joinState = 2;
            m_joinTime = timeGetTime();
        }
        break;
    case 2:
        if (m_joinTime + 220 < timeGetTime()) {
            m_joinState = 5;
            m_vehicles[1]->m_rider->m_sounds->PlayJoinSounds();
        }
        break;
    case 5:
        if (m_vehicles[1]->m_rider->m_x10) {
            m_vehicles[0]->m_body->m_object->SetPosition(0, &SavedPosition);
            m_cameraControllers[0]->SetJetski(m_vehicles[0], 0);
            m_cameraControllers[0]->ToggleFreezeRotate();
            return 0;
        }
        break;
    }
    return 1;
}


// ---- src/JetskiCompetition/LoadOverlays.cpp
void PSXprintf(const char *fmt, ...);

class ScreenIconManager;
class ScreenTextManager;
class LightManager;
class JetskiCompetition;















// Their headers are view headers that don't fit JetskiMenu.h
// (JetskiRaceOverMenuCircuit.h) or don't exist (JetskiRaceOverMenuChallenge).
#ifdef SPLASHDOWN_PC
#include "JetskiRaceOverMenuCircuit/JetskiRaceOverMenuCircuit.h"
#include "JetskiRaceOverMenuChallenge/JetskiRaceOverMenuChallenge.h"
#else
class JetskiRaceOverMenuCircuit : public JetskiMenu {
public:
    JetskiRaceOverMenuCircuit(int enabled);
    JetskiRaceOverMenuCircuit *Create(ScreenIconManager *icons, ScreenTextManager *texts, ViewPortCode viewport);
    char padF8[0x128 - sizeof(JetskiMenu)];
};

class JetskiRaceOverMenuChallenge : public JetskiMenu {
public:
    JetskiRaceOverMenuChallenge(int enabled);
    JetskiRaceOverMenuChallenge *Create(ScreenIconManager *icons, ScreenTextManager *texts, ViewPortCode viewport);
    char padF8[0x120 - sizeof(JetskiMenu)];
};
#endif

// Creates the menus for this race type and the in-race overlays.
void JetskiCompetition::LoadOverlays()
{
    switch (m_options->m_raceType) {
    case 0:
        break;
    case 8:
        m_inGameMenu = (new JetskiTrainingInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        break;
    case 6:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_raceStartMenu = (new JetskiRaceStartMenuCareer(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_raceOverMenu = (new JetskiRaceOverMenuCareer(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_promptMenu = (new JetskiPromptMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134, 1);
        break;
    case 1:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_raceOverMenu = (new JetskiRaceOverMenuCircuit(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        break;
    case 7:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_raceOverMenu = (new JetskiRaceOverMenuChallenge(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        break;
    case 3:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        break;
    case 2:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        break;
    case 4:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_raceOverMenu = (new JetskiRaceOverMenuCountdown(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        break;
    case 5:
        m_inGameMenu = (new JetskiInGameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
        m_promptMenu = (new JetskiPromptMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134, 0);
        break;
    }
    if (m_inGameMenu)
        m_inGameMenu->DisableAll();
    if (m_raceStartMenu)
        m_raceStartMenu->DisableAll();
    if (m_raceOverMenu)
        m_raceOverMenu->DisableAll();
    if (m_promptMenu)
        m_promptMenu->DisableAll();
    m_nameMenu = (new JetskiNameMenu(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134);
    m_nameMenu->DisableAll();
    if (m_options->m_raceType != 5 && m_options->m_raceType != 4 && m_options->m_raceType != 8) {
        if (!(m_radar = (new JetskiRadar(1))->Create((ScreenTextManager *)m_textManager, (ViewPortCode)m_x134, this)))
            PSXprintf("Radar not created. Failed in JetskiCompetition::CreateOverlays()\n");
        if (!(m_headingArrows = (new JetskiHeadingArrows(1))->Create()))
            PSXprintf("Off Track Arrows not created. Failed in JetskiCompetition::CreateOverlays()\n");
    }
    int showLaps = m_options->m_raceType != 4;
    int showPosition = m_options->m_raceType != 3;
    if (!(m_inGameStats = (new JetskiInGameStats(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134, 0,
                                                        showPosition, showLaps)))
        PSXprintf("In Game Overlays not created. Failed in JetskiCompetition::CreateOverlays()\n");
    if (!(m_infoText = (new JetskiInfoText(1))->Create((ScreenTextManager *)m_textManager, (ViewPortCode)m_x134)))
        PSXprintf("In Game Overlays not created. Failed in JetskiCompetition::CreateOverlays()\n");
    if (!(m_performanceMeter = (new JetskiPerformanceMeter(1))->Create((ScreenIconManager *)m_iconManager, (ScreenTextManager *)m_textManager, (ViewPortCode)m_x134)))
        PSXprintf("Performance Meter not created. Failed in JetskiCompetition::CreateOverlays()\n");
    if (m_options->m_raceType != 5) {
        if (!(m_countDown = (new JetskiCountDown(1))->Create((LightManager *)m_lightManager, (ViewPortCode)m_x134)))
            PSXprintf("Count down numbers not created. Failed in JetskiCompetition::CreateOverlays()\n");
        if (m_x134) {
            if (!(m_separator = (new SplitScreenSeparator(1))->Create((ViewPortCode)m_x134)))
                PSXprintf("Split Screen Separator not created...\n");
            else
                PSXprintf("Split Screen Separator created...\n");
        }
    }
}


// ---- src/JetskiCompetition/ModifyLocalGameOptions.cpp

void JetskiCompetition::ModifyLocalGameOptions()
{
}


// ---- src/JetskiCompetition/PrepareGeometry.cpp




int JetskiCompetition::PrepareGeometry()
{
    if (LpGame->m_currentPlayer == 3 && m_x118 == 3)
        return 1;
    static int timerID = -1;   // profiler id, unused in retail
    if (m_fadeOutTime[LpGame->m_currentPlayer]) {
        m_faded[LpGame->m_currentPlayer] = 1;
        if (m_videoFX)
            m_videoFX->FadeOut(m_fadeOutTime[LpGame->m_currentPlayer], LpGame->m_currentPlayer);
        m_fadeOutTime[LpGame->m_currentPlayer] = 0;
    } else if (m_fadeInTime[LpGame->m_currentPlayer]) {
        m_faded[LpGame->m_currentPlayer] = 0;
        if (m_videoFX)
            m_videoFX->FadeIn(m_fadeInTime[LpGame->m_currentPlayer], LpGame->m_currentPlayer);
        m_fadeInTime[LpGame->m_currentPlayer] = 0;
    }
    GameObject::PrepareGeometry();
    return 1;
}


// ---- src/JetskiCompetition/SetTrackHeights.cpp

extern "C" void *calloc(unsigned int n, unsigned int size);
extern "C" void free(void *p);




#ifdef SPLASHDOWN_PC
// x86-64: the same views with the real structs' offsets
// (TrackData_Tag.h, TrackSegment_Tag.h, here in a namespace of their own).
namespace pc_track {
#include "TrackData_Tag/TrackData_Tag.h"
#include "TrackSegment_Tag/TrackSegment_Tag.h"
}
#define PC_TRACK_OFF(s, m) __builtin_offsetof(pc_track::s, m)
struct TrackPoint_Tag {
    char pad0[PC_TRACK_OFF(TrackData_Tag, m_position)];
    _D3DVECTOR m_left;
    _D3DVECTOR m_center;
    _D3DVECTOR m_right;
    char pad40[PC_TRACK_OFF(TrackData_Tag, m_next) - PC_TRACK_OFF(TrackData_Tag, m_position) - 0x30];
    TrackPoint_Tag *m_next;
};

struct TrackSegment_Tag {
    signed char m_flags;
    char pad1[PC_TRACK_OFF(TrackSegment_Tag, m_nodes) - 1];
    TrackPoint_Tag *m_points;
    char pad10[PC_TRACK_OFF(TrackSegment_Tag, m_numLinks) - PC_TRACK_OFF(TrackSegment_Tag, m_nodes) - sizeof(void *)];
    int m_numLinks;
    char pad18[PC_TRACK_OFF(TrackSegment_Tag, m_links) - PC_TRACK_OFF(TrackSegment_Tag, m_numLinks) - 4];
    TrackSegment_Tag **m_links;
};
#else
// A cross-section of the track: left, centre and right points.
struct TrackPoint_Tag {
    char pad0[0x10];
    _D3DVECTOR m_left;                  // +0x10
    _D3DVECTOR m_center;                // +0x20
    _D3DVECTOR m_right;                 // +0x30
    char pad40[0x50 - 0x40];
    TrackPoint_Tag *m_next;             // +0x50
};

struct TrackSegment_Tag {
    signed char m_flags;                // +0x00, bit 2: visited
    char pad1[0xC - 0x1];
    TrackPoint_Tag *m_points;           // +0x0C
    char pad10[0x14 - 0x10];
    int m_numLinks;                     // +0x14
    char pad18[0x20 - 0x18];
    TrackSegment_Tag **m_links;         // +0x20
};
#endif




// Walks every track segment and drops its points onto the water and terrain
// (whichever is higher).
int JetskiCompetition::SetTrackHeights()
{
    SegmentList_Tag *list = 0;
    if (!m_track)
        return 0;
    SegmentList_Tag *node = (SegmentList_Tag *)calloc(1, sizeof(SegmentList_Tag));
    if (!node)
        return 0;
    node->m_segment = m_track->m_start;
    node->m_next = list;
    list = node;
    while (list) {
        node = list;
        if (node->m_segment->m_flags & 4) {
            list = node->m_next;
            node->m_segment->m_flags &= ~4;
            free(node);
            continue;
        }
        float left = 0.0f;
        float center = left;
        float right = left;
        for (TrackPoint_Tag *p = node->m_segment->m_points; p; p = p->m_next) {
            p->m_left.y = 1000.0f;
            p->m_center.y = 1000.0f;
            p->m_right.y = 1000.0f;
            if (m_water) {
                m_water->GetGridPointIntersection(&p->m_left, 0);
                left = p->m_left.y;
                m_water->GetGridPointIntersection(&p->m_center, 0);
                center = p->m_center.y;
                m_water->GetGridPointIntersection(&p->m_right, 0);
                right = p->m_right.y;
            }
            if (m_terrain) {
                m_terrain->GetGridPointIntersection(&p->m_left, 0, 0, 0);
                if (left > p->m_left.y)
                    p->m_left.y = left;
                m_terrain->GetGridPointIntersection(&p->m_center, 0, 0, 0);
                if (center > p->m_center.y)
                    p->m_center.y = center;
                m_terrain->GetGridPointIntersection(&p->m_right, 0, 0, 0);
                if (right > p->m_right.y)
                    p->m_right.y = right;
            }
        }
        SegmentList_Tag *current = list;
        current->m_segment->m_flags |= 4;
        for (int i = 0; i < current->m_segment->m_numLinks; i++) {
            if (current->m_segment->m_links[i]->m_flags & 4)
                continue;
            node = (SegmentList_Tag *)calloc(1, sizeof(SegmentList_Tag));
            if (!node) {
                m_track->FreeSegmentList(&list, 0);
                return 0;
            }
            node->m_segment = current->m_segment->m_links[i];
            node->m_next = list;
            list = node;
        }
    }
    return 1;
}


// ---- src/JetskiCompetition/KeyDownHandler.cpp



int KEYDOWN_HANDLER_TEST(AssignableActionIndex action, DeviceType device, ProcessNode *node, unsigned int mods);







int JetskiCompetition::KeyDownHandler(ProcessNode *node, KeyInfo *key)
{
    if (m_paused || !(node->m_controller < m_x118 || m_x118 || !node->m_controller))
        return 0;
    if ((m_raceStatus->m_flags & 1) || m_status == 3) {
        if (m_status != 10 || m_options->m_raceType == 8) {
            if (KEYDOWN_HANDLER_TEST((AssignableActionIndex)8, (DeviceType)2, node, 0x80000000) && !m_faded[0] && !m_faded[1]) {
                if (m_options->m_raceType == 5 && !m_cameraControllers[0]->m_x64 == node->m_controller)
                    return 0;
                return InGamePause(1, node->m_controller);
            }
        }
    }
    if (m_status == 10 && m_options->m_raceType != 8) {
        if (KEYDOWN_HANDLER_TEST((AssignableActionIndex)4, (DeviceType)2, node, 0x80000000) ||
            KEYDOWN_HANDLER_TEST((AssignableActionIndex)8, (DeviceType)2, node, 0x80000000)) {
            m_status = 9;
            LpGame->m_paused = 0;
            Pause(1);
            m_raceOverMenu->EnableAll();
            LpGame->m_auralScape->StopAllAuralScapeSounds();
            LpGame->m_auralScape->Disable();
            return 1;
        }
        if (KEYDOWN_HANDLER_TEST((AssignableActionIndex)0x12, (DeviceType)2, node, 0x80000000)) {
            m_x2C0 = 1;
            FadeInCameras();
        }
    }
    return GameObject::KeyDownHandler(node, key);
}


// ---- src/JetskiCompetition/AllocateIpuCache.cpp
extern PSXTextureMapManager *LpTextureMapManager;


void JetskiCompetition::AllocateIpuCache()
{
    LpTextureMapManager->AllocateIpuCache(0x380000);
}


// ---- src/JetskiCompetition/FreeIpuCache.cpp
extern PSXTextureMapManager *LpTextureMapManager;


void JetskiCompetition::FreeIpuCache()
{
    LpTextureMapManager->FreeIpuCache();
}


// ---- src/JetskiCompetition/CheckForNewHighScores.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" char *strcpy(char *dst, const char *src);


// One hall-of-fame list: six names and their times, best first (0x50 bytes).
struct FameTimes {
    char m_names[6][9];                 // +0x00
    float m_times[6];                   // +0x38
};

// Stunt list: six names and their scores, best first (0x50 bytes).
struct FameScores {
    char m_names[6][9];                 // +0x00
    int m_scores[6];                    // +0x38
};

struct FameRace {                       // 0xA0 bytes
    FameTimes m_race;                   // +0x00
    FameTimes m_lap;                    // +0x50
};

struct FameStunt {                      // 0xA0 bytes
    FameTimes m_time;                   // +0x00
    FameScores m_score;                 // +0x50
};

// JetskiGameOptions::m_hallOfFame, per difficulty (hard or not) and track.
struct HallOfFame {
    FameRace m_circuit[2][21];          // +0x0000
    FameTimes m_timeTrial[2][21];       // +0x1A40
    FameStunt m_stunt[2][21];           // +0x2760
};

// The vehicles' race-status record (vehicle +0x30).
struct RacerStatusView {
#ifdef SPLASHDOWN_PC
    char pad0[__builtin_offsetof(JetskiVehicle, m_ownStatus)];
#else
    char pad0[0x30];
#endif
    RaceStatusStruct_Tag *m_status;     // +0x30
};

struct OptionsFame {
    char pad0[0x8A8];
    HallOfFame m_fame;                  // +0x8A8 (JetskiGameOptions::m_hallOfFame)
};

#ifdef SPLASHDOWN_PC
#include "JetskiCareerCompetition/JetskiCareerCompetition.h"
#endif
struct CompetitionX40C {
#ifdef SPLASHDOWN_PC
    char pad0[__builtin_offsetof(JetskiCareerCompetition, m_raceType)];
#else
    char pad0[0x40C];
#endif
    int m_x40C;                         // +0x40C
};

// Enters the player's results into the hall of fame; returns 1 if any made it.
int JetskiCompetition::CheckForNewHighScores(int player)
{
    int newRecord;
    int p;
    int i;
    int j;
    float time;
    RaceStatusStruct_Tag *st;
    char name[0x10];
    newRecord = 0;
    p = player - 1;
    sprintf(name, "%^&*%d", p + 1);
    switch (m_options->m_raceType) {
    case 6:
        if (((CompetitionX40C *)this)->m_x40C == 1) {
            FameTimes *t = &((OptionsFame *)m_options)->m_fame.m_timeTrial[m_options->m_difficulty == 3 ? 1 : 0][m_options->m_x2C];
            for (i = 0; i < 6; i++) {
                time = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
                if (time != 0.0f && time <= t->m_times[i]) {
                    for (j = 5; j > i; j--) {
                        t->m_times[j] = t->m_times[j - 1];
                        strcpy(t->m_names[j], t->m_names[j - 1]);
                    }
                    t->m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
                    strcpy(t->m_names[i], name);
                    newRecord = 1;
                    i = 6;
                }
            }
        } else {
        FameRace *r = &((OptionsFame *)m_options)->m_fame.m_circuit[m_options->m_difficulty == 3 ? 1 : 0][m_options->m_x2C];
        for (i = 0; i < 6; i++) {
            time = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
            if (time != 0.0f && time <= r->m_race.m_times[i]) {
                for (j = 5; j > i; j--) {
                    r->m_race.m_times[j] = r->m_race.m_times[j - 1];
                    strcpy(r->m_race.m_names[j], r->m_race.m_names[j - 1]);
                }
                r->m_race.m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
                strcpy(r->m_race.m_names[i], name);
                newRecord = 1;
                i = 6;
            }
        }
        if (m_options->m_x30 == 3) {
            for (i = 0; i < 6; i++) {
                st = ((RacerStatusView *)m_vehicles[p + 5])->m_status;
                if ((st->m_flags & 1) && st->m_x6D4 <= r->m_lap.m_times[i]) {
                    for (j = 5; j > i; j--) {
                        r->m_lap.m_times[j] = r->m_lap.m_times[j - 1];
                        strcpy(r->m_lap.m_names[j], r->m_lap.m_names[j - 1]);
                    }
                    r->m_lap.m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_x6D4;
                    strcpy(r->m_lap.m_names[i], name);
                    newRecord = 1;
                    i = 6;
                }
            }
        }
                }
        break;
    case 1:
    case 2:
    case 7: {
        FameRace *r = &((OptionsFame *)m_options)->m_fame.m_circuit[m_options->m_difficulty == 3 ? 1 : 0][m_options->m_x2C];
        for (i = 0; i < 6; i++) {
            time = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
            if (time != 0.0f && time <= r->m_race.m_times[i]) {
                for (j = 5; j > i; j--) {
                    r->m_race.m_times[j] = r->m_race.m_times[j - 1];
                    strcpy(r->m_race.m_names[j], r->m_race.m_names[j - 1]);
                }
                r->m_race.m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
                strcpy(r->m_race.m_names[i], name);
                newRecord = 1;
                i = 6;
            }
        }
        if (m_options->m_x30 == 3) {
            for (i = 0; i < 6; i++) {
                st = ((RacerStatusView *)m_vehicles[p + 5])->m_status;
                if ((st->m_flags & 1) && st->m_x6D4 <= r->m_lap.m_times[i]) {
                    for (j = 5; j > i; j--) {
                        r->m_lap.m_times[j] = r->m_lap.m_times[j - 1];
                        strcpy(r->m_lap.m_names[j], r->m_lap.m_names[j - 1]);
                    }
                    r->m_lap.m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_x6D4;
                    strcpy(r->m_lap.m_names[i], name);
                    newRecord = 1;
                    i = 6;
                }
            }
        }
        break;
    }
    case 3: {
        FameTimes *t = &((OptionsFame *)m_options)->m_fame.m_timeTrial[m_options->m_difficulty == 3 ? 1 : 0][m_options->m_x2C];
        for (i = 0; i < 6; i++) {
            time = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
            if (time != 0.0f && time <= t->m_times[i]) {
                for (j = 5; j > i; j--) {
                    t->m_times[j] = t->m_times[j - 1];
                    strcpy(t->m_names[j], t->m_names[j - 1]);
                }
                t->m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_bestLap;
                strcpy(t->m_names[i], name);
                newRecord = 1;
                i = 6;
            }
        }
        break;
    }
    case 4: {
        FameStunt *f = &((OptionsFame *)m_options)->m_fame.m_stunt[m_options->m_difficulty == 3 ? 1 : 0][m_options->m_x2C];
        for (i = 0; i < 6; i++) {
            time = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_x6D4;
            if (time != 0.0f && time >= f->m_time.m_times[i]) {
                for (j = 5; j > i; j--) {
                    f->m_time.m_times[j] = f->m_time.m_times[j - 1];
                    strcpy(f->m_time.m_names[j], f->m_time.m_names[j - 1]);
                }
                f->m_time.m_times[i] = ((RacerStatusView *)m_vehicles[p + 5])->m_status->m_x6D4;
                strcpy(f->m_time.m_names[i], name);
                newRecord = 1;
                i = 6;
            }
        }
        for (i = 0; i < 6; i++) {
            st = ((RacerStatusView *)m_vehicles[p + 5])->m_status;
            if ((st->m_flags & 1) && *(int *)((char *)m_vehicles[p + 5] + 0x618) >= f->m_score.m_scores[i]) {
                for (j = 5; j > i; j--) {
                    f->m_score.m_scores[j] = f->m_score.m_scores[j - 1];
                    strcpy(f->m_score.m_names[j], f->m_score.m_names[j - 1]);
                }
                f->m_score.m_scores[i] = *(int *)((char *)m_vehicles[p + 5] + 0x618);
                strcpy(f->m_score.m_names[i], name);
                newRecord = 1;
                i = 6;
            }
        }
        break;
    }
    }
    return newRecord;
}


// ---- src/JetskiCompetition/PickACamera.cpp

extern "C" int rand();









// Uniform in [0, 1).
#define RAND_SCALE 4.656613e-10f

void JetskiCompetition::PickACamera(float dt)
{
    // Retail's layout: case 2 falls into default's return, case 1 jumps to it,
    // and case 0's body follows the switch.
    switch (m_x2C8) {
    case 1:
        m_timer -= dt;
        if (m_timer < 0.0f) {
            m_cameraControllers[0]->SwitchCamera((CameraType)0);
            m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)6);
            m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
            m_cameraType = 6;
            m_x2C8 = 2;
            m_fadeInTime[0] = 380;
            m_timer = 0.5f;
        }
        goto done;
    case 2:
        m_timer -= dt;
        if (m_timer < 0.0f)
            m_x2C8 = 0;
    default:
    done:
        return;
    case 0:
        break;
    }
    if (m_x2C0) {
        int c = ++m_cycleIndex;
        c %= 4;
        m_cycleIndex = c;
        switch (c) {
        case 0:
            m_cameraControllers[0]->SwitchCamera((CameraType)4);
            m_cameraType = 4;
            break;
        case 1:
            m_cameraControllers[0]->SwitchCamera((CameraType)0);
            m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)6);
            m_cameraType = 6;
            break;
        case 2:
            m_cameraControllers[0]->SwitchCamera((CameraType)0);
            m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)7);
            m_cameraType = 7;
            break;
        case 3:
            m_cameraControllers[0]->SwitchCamera((CameraType)0);
            m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)5);
            m_cameraType = 5;
            break;
        }
        m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
        m_x2C0 = 0;
        return;
    }
    int probabilities[4];
    if (m_cameraControllers[0]->m_tvCamera) {
        m_cameraControllers[0]->m_tvCamera->GetProbabilities(probabilities);
    } else {
        probabilities[0] = 50;
        probabilities[1] = 40;
        probabilities[2] = 20;
        probabilities[3] = 0;
    }
    float wreck = m_vehicles[0]->m_rider->m_wrecker->m_wreckType;
    if (wreck > 1.0f) {
        m_cameraControllers[0]->SwitchCamera((CameraType)4);
        m_x2C4 = 1;
        m_cameraType = 4;
    } else if (m_x2C4 && wreck == 0.0f) {
        m_x2C8 = 1;
        m_fadeOutTime[0] = 100;
        m_timer = 0.3f;
        m_x2C4 = 0;
    } else if (!m_cameraControllers[0]->m_cameraType && m_cameraControllers[0]->m_followCamera->m_x114) {
        m_cameraControllers[0]->SwitchCamera((CameraType)4);
        m_cameraType = 4;
    } else if (m_cameraControllers[0]->m_pickCamera) {
        m_cameraControllers[0]->m_pickCamera = 0;
        int roll = (int)(100.0f * (RAND_SCALE * rand()));
        if (roll >= probabilities[0]) {
            m_cameraControllers[0]->SwitchCamera((CameraType)4);
            m_cameraType = 4;
        } else {
            m_cameraControllers[0]->SwitchCamera((CameraType)0);
            if (roll >= probabilities[1]) {
                if (m_cameraType != 6) {
                    m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)6);
                    m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
                    m_cameraType = 6;
                }
            } else if (roll >= probabilities[2]) {
                if (m_cameraType != 7) {
                    m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)7);
                    m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
                    m_cameraType = 7;
                }
            } else if (roll >= probabilities[3]) {
                if (m_cameraType != 5) {
                    m_cameraControllers[0]->SwitchFollowCameraPresets((FollowCameraPresetType)5);
                    m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
                    m_cameraType = 5;
                }
            }
        }
    }
}


// ---- src/JetskiCompetition/SetPointersForVCR.cpp
void JetskiCompetition::SetPointersForVCR(JetskiVehicle *vehicle)
{
    if (m_vcr)
        m_vcr->RegisterVariables(vehicle);
}

// ---- src/JetskiCompetition/LoadVCRAndRun.cpp
void PSXprintf(const char *fmt, ...);




void JetskiCompetition::LoadVCRAndRun(char *filename)
{
    if (m_vcr) {
        if (m_vcr->LoadFromDisk(filename)) {
            m_vcr->REWforPLAY();
            for (int i = 0; i < m_numVehicles; i++)
                m_vehicles[i]->m_x1CC = 1;
            m_status = 10;
        } else {
            PSXprintf("Failed to load VCR file %s!\n", filename);
        }
    }
}


// ---- src/JetskiCompetition/ResetVCR.cpp



void JetskiCompetition::ResetVCR()
{
    if (m_vcr) {
        m_vcr->RESET();
        m_x290 = 0;
        for (int i = 0; i < m_options->m_numRacers; i++)
            m_vehicles[i]->m_x1CC = 0;
    }
}


// ---- src/JetskiCompetition/LoadGhost.cpp

extern "C" char *strncpy(char *dst, const char *src, unsigned int n);
void PSXprintf(const char *fmt, ...);

extern DataAccess *LpDataAccess;

extern MaterialStringTable *LpMaterialStringTable;

extern TextureMapStringTable *LpTextureMapStringTable;

class SearchNode;









int JetskiCompetition::LoadGhost()
{
    char name[0x20];

    if (m_options->m_x54 == 2)
        return 1;
    LpDataAccess->LoadResourceFile("CharRes\\bs_saucr.res", 0, 0, 0, 0, 0, 0, 0);
    switch (m_options->m_x54) {
    case 0:
        strncpy(name, "bs_saucer12ft.slt", 0x20);
        break;
    case 1:
        strncpy(name, "MB_F18.slt", 0x20);
        break;
    default:
        strncpy(name, "bs_saucer12ft.slt", 0x20);
        break;
    }
    m_ghostObject = new PSXRenderableObject(1);
    if (!m_ghostObject->Create(name, 0, 0)) {
        PSXprintf("Ghost not created (%s).\n", name);
        delete m_ghostObject;
        m_ghostObject = 0;
        LpDataAccess->DropDevice("CharRes\\bs_saucr.res", 1);
        return 0;
    }
    AddChild(m_ghostObject, 0);
    m_ghostObject->AddSearchPlacement(*m_scene->m_staticNodes, 1);
    m_ghost = new JetskiGhostVehicle(m_ghostObject);
    LpMaterialStringTable->LoadRequested();
    LpTextureMapStringTable->LoadRequested();
    LpDataAccess->DropDevice("CharRes\\bs_saucr.res", 1);
    return 1;
}


// ---- src/JetskiCompetition/UpdateGhost.cpp


static inline float Sqrt(float x)
{
    float r;
    asm("sqrt.s $f8, %1; swc1 $f8, 0(%0)" : : "r"(&r), "f"(x) : "$f8", "memory");
    return r;
}

static inline float Length(const _D3DVECTOR &v)
{
    return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}















void JetskiCompetition::UpdateGhost(float dt)
{
    if (!m_ghostEnabled)
        return;
    if (!m_vcr->m_loaded)
        return;
    if (m_options->m_x54 == 2) {
        JetskiVehicle *ghost = (JetskiVehicle *)m_ghost;
        if (ghost && !m_vcr->TestEOT()) {
            if (!m_ghostVisible) {
                if (m_vehicles[5]->m_ownStatus->m_flags & 2)
                    m_ghostVisible = 1;
            } else {
                ghost->EnableAll();
                m_ghostVisible = 0;
            }
            ghost->m_x290 = m_vehicles[5]->m_x290;
            ghost->m_x2A0 = m_vehicles[5]->m_x2A0;
            ghost->m_x2B0 = m_vehicles[5]->m_x2B0;
            ghost->m_body->m_object->UpdateSearchPlacement();
            if (m_vcr->m_ghostDirection < 2) {
                ghost->m_x2C0 = m_vehicles[5]->m_x290;
                ghost->m_x2E0 = m_vehicles[5]->m_x2A0;
                ghost->m_x2F0 = m_vehicles[5]->m_x2B0;
            } else {
                ghost->m_x60 = m_vehicles[5]->m_x300;
                ghost->m_x64 = m_vehicles[5]->m_x304;
                ghost->m_body->m_velocity = m_vehicles[5]->m_x320;
                ghost->m_body->m_speed = m_vehicles[5]->m_x314;
                ghost->m_x2C0 = m_vehicles[5]->m_x2C0;
                ghost->m_x2E0 = m_vehicles[5]->m_x2E0;
                ghost->m_x2F0 = m_vehicles[5]->m_x2F0;
                ghost->m_rider->m_stuntF = m_vehicles[5]->m_x308;
                ghost->m_rider->m_x898 = m_vehicles[5]->m_x30C;
                ghost->m_rider->m_wrecker->m_wreckType = m_vehicles[5]->m_x310;
            }
        } else {
            ghost->m_rider->RestartAction();
            ghost->m_rider->ResetVariables();
            ghost->m_rider->StopSpecialAnimation();
            ghost->m_rider->m_wrecker->StopWreck();
            ghost->m_x530 = 0;
            ghost->m_body->m_velocity = ZeroVector;
            ghost->m_body->m_speed = 0;
        }
    } else {
        JetskiGhostVehicle *ghost = m_ghost;
        // Two returns, not `!ghost || TestEOT()`: the || form branches to a
        // lone `b epilogue` block, mwcc's block optimizer threads it and
        // rebuilds the predecessor lists, and the delay-slot filler then fills
        // the `flags & 2` beqz slots that retail leaves empty.
        if (!ghost)
            return;
        if (m_vcr->TestEOT())
            return;
        if (!m_ghostVisible) {
            if (m_vehicles[5]->m_ownStatus->m_flags & 2)
                m_ghostVisible = 1;
        } else {
            ghost->m_object->Enable();
            m_ghostVisible = 0;
        }
        _D3DVECTOR position = m_vehicles[5]->m_x290;
        float distance = Length(m_cameraControllers[0]->m_camera->m_position - position);
        if (m_status != 10) {
            position.y += 5.5f;
            if (distance < 80.0f)
                position.y += 15.0f + 0.1785f * -distance;
        } else {
            position.y += 4.5f;
        }
        HeightFilter *filter = m_ghostHeight;
        float y = position.y;           // loaded before the min.s asm, as retail
        filter->m_scale = Fmin(dt, filter->m_rate) / filter->m_rate;
        position.y = filter->m_value = filter->m_value + filter->m_scale * (y - filter->m_value);
        ghost->m_object->SetPosition(0, &position);
        ghost->m_object->SetOrientation(0, &m_vehicles[5]->m_x2A0, (_D3DVECTOR *)&m_vehicles[5]->m_x2B0 /* cast: retail computes this argument first */, 0, 1, 0);
        if (m_options->m_x54 && m_options->m_x54 == 1)
            ghost->m_object->SetScale(0.25f, 0.25f, 0.25f);
        ghost->m_object->UpdateSearchPlacement();
    }
}


// ---- src/JetskiCompetition/RSSetCurrentVehicle.cpp

class VehicleBase {
public:
    char pad0[0x30];
};







int JetskiCompetition::RSSetCurrentVehicle(RaceStatusStruct_Tag *status)
{
    bool previousIdle;  // retail reads this uninitialized when there was no previous vehicle
    if (m_currentStatus) {
        JetskiVehicle *vehicle = static_cast<JetskiVehicle *>(m_currentStatus->m_vehicle);
        previousIdle = !vehicle->m_rider->m_x8A4;
    }
    m_currentStatus = status;
    if ((m_raceStatus->m_flags & 1) && previousIdle) {
        m_raceStatus->m_flags |= 8;
        m_x34C = 1;
        m_x350 = 1.0f;
    } else {
        m_x34C = 0;
        m_x350 = 0;
        m_raceStatus->m_flags &= ~8;
        CopyCatSwitchTurn(status);
    }
    return 1;
}


// ---- src/JetskiCompetition/CopyCatSwitchTurn.cpp
extern "C" void *memcpy(void *dst, const void *src, unsigned int n);

















// Ends any wreck/special animation and puts the jetski back on its reset spot.
#define RESET_JETSKI(j)                                         \
    if ((j)->m_x530 || (j)->m_rider->m_x484) {                  \
        (j)->m_rider->StopSpecialAnimation();                   \
        (j)->m_rider->m_wrecker->StopWreck();                   \
        (j)->m_x530 = 0;                                        \
        (j)->m_x550 = 0;                                        \
        (j)->m_rider->m_wrecker->m_x160 = 1;                    \
        (j)->m_x554 = 0;                                        \
        (j)->m_body->m_x27C = 18225.0f;                      \
        (j)->m_rider->m_wrecker->m_body->m_x27C = 18225.0f;    \
    }                                                           \
    (j)->m_resetTimer = 0;                                            \
    (j)->ResetPosition(1);                                      \
    (j)->m_rider->AnimateRider(0.0167f);                        \
    {                                                           \
        float *src = &(j)->m_body->m_x60.x;                     \
        (j)->m_x2C0[0] = src[0];                                \
        (j)->m_x2C0[1] = src[1];                                \
        (j)->m_x2C0[2] = src[2];                                \
        (j)->m_x2C0[3] = src[3];                                \
    }

int JetskiCompetition::CopyCatSwitchTurn(RaceStatusStruct_Tag *status)
{
    static int lastPlayerIndex = 0;
    JetskiVehicle *other = 0;
    if (status && status->m_vehicle && !(status->m_flags & 1) && m_cameraControllers[0]) {
        int player = status->m_id;
        if (m_options->m_raceType == 5) {
            JetskiVehicle *jetski = (JetskiVehicle *)status->m_vehicle;
            if (status->m_next)
                other = (JetskiVehicle *)status->m_next->m_vehicle;
            else if (m_raceStatus->m_vehicles)
                other = (JetskiVehicle *)m_raceStatus->m_vehicles->m_vehicle;
            RESET_JETSKI(jetski);
            if (other) {
                RESET_JETSKI(other);
                other->DisableAll();
            }
            jetski->EnableAll();
            if (m_cameraControllers[0]->m_player >= 0) {
                CopyCatCamera *saved = &m_copyCatCameras[lastPlayerIndex];
                saved->m_x04 = m_cameraControllers[0]->m_followCamera->m_settings[3];
                saved->m_x08 = m_cameraControllers[0]->m_followCamera->m_settings[0];
                saved->m_x0C = m_cameraControllers[0]->m_followCamera->m_settings[2];
                saved->m_x10 = m_cameraControllers[0]->m_followCamera->m_settings[1];
                memcpy(saved->m_presets, &m_cameraControllers[0]->m_cinematographer->m_input, sizeof(saved->m_presets));
                m_cameraControllers[0]->SetJetski(jetski, player);
                m_cameraControllers[0]->m_player = status->m_id;
                m_cameraControllers[0]->m_followCamera->m_resetFilters = 1;
                CopyCatCamera *restored = &m_copyCatCameras[player];
                m_cameraControllers[0]->m_followCamera->m_settings[3] = restored->m_x04;
                m_cameraControllers[0]->m_followCamera->m_settings[0] = restored->m_x08;
                m_cameraControllers[0]->m_followCamera->m_settings[2] = restored->m_x0C;
                m_cameraControllers[0]->m_followCamera->m_settings[1] = restored->m_x10;
                memcpy(&m_cameraControllers[0]->m_cinematographer->m_input, restored->m_presets, sizeof(restored->m_presets));
            }
            jetski->m_rider->m_x8A4 = 0;
            if (m_promptMenu && LpGame->m_competition) {
                if (!(status->m_flags & 0x100)) {
                    m_promptMenu->SetCopyCatPrompt(1, player + 1);
                    m_x348 = 1;
                } else {
                    m_promptMenu->SetCopyCatPrompt(0, player + 1);
                    m_x348 = 1;
                }
            }
        }
        lastPlayerIndex = player;
    }
    return 1;
}


// ---- src/JetskiCompetition/checkSecretSkinObjectsName.cpp

int checkSecretSkinObjects(char *name);


void SecretSkinCallback(CollisionObject *a, CollisionObject *b);

// A scene's static model as JetskiCompetition sees it (Scene.h's StaticModel).
class StaticObject {
public:
    SoultreeObject *m_object;       // +0x00
#ifdef SPLASHDOWN_PC
    char pad04[__builtin_offsetof(StaticModel, m_collision) - sizeof(void *)];
#else
    char pad04[0xC - 0x4];
#endif
    CollisionObject *m_collision;   // +0x0C
};




// Secret-skin pickups: once a skin is unlocked its pickup is removed from the
// course; otherwise the pickup's collision object becomes a trigger.
void JetskiCompetition::checkSecretSkinObjectsName(char *name, StaticObject *object)
{
    int skin = checkSecretSkinObjects(name);
    if (skin == -1)
        return;
    if (LpGame->m_gameOptions->m_secretSkinUnlocked[skin / 8][skin % 8] == 0) {
        if (object->m_collision) {
            object->m_collision->m_ownerType = WORLDOBJECT_TRIGGER;
            object->m_collision->m_otherCallback = SecretSkinCallback;
        }
    } else {
        object->m_object->Disable();
        object->m_object->RemoveFromSearch();
        object->m_collision->m_collidable = 0;
        object->m_collision->RemoveFromSearch();
    }
}


// ---- src/JetskiCompetition/Pause.cpp
int JetskiCompetition::Pause(int pause)
{
    FadeInCameras();
    LpGame->m_paused = pause;
    return GameObject::Pause(pause);
}

// ---- src/JetskiCompetition/InGamePause.cpp









int JetskiCompetition::InGamePause(int pause, int player)
{
    if (m_options->m_raceType != 5) {
        if (!m_cameraControllers[player] || (!player && m_cameraControllers[0]->m_player != player))
            return 1;
    }
    static int prevCompetitionStatus = 0;
    FadeInCameras();
    if (pause) {
        if (m_status != 6)
            prevCompetitionStatus = m_status;
        if (LpGame->m_controls->m_joysticks[0])
            LpGame->m_controls->m_joysticks[0]->StopEffect(2);
        if (LpGame->m_controls->m_joysticks[1])
            LpGame->m_controls->m_joysticks[1]->StopEffect(2);
        if (m_vehicles[5 + player]) {
            if (m_vehicles[5 + player]->m_resetTimer > 0.0f) {
                m_vehicles[5 + player]->m_resetTimer = 0;
                m_vehicles[5 + player]->ResetPosition(1);
                m_vehicles[5 + player]->Tick(0.0f);
                m_cameraControllers[player]->Tick(0.0f);
                m_fadeInTime[m_cameraControllers[player]->m_viewport] = 50;
            }
            m_vehicles[5 + player]->m_resetTimer = 0;
        }
        Pause(1);
        m_status = 6;
        LpGame->m_paused = 1;
        ((JetskiMenu *)m_inGameMenu)->m_xB8 = player;
        ((JetskiMenu *)m_inGameMenu)->EnableMenu();
    } else {
        m_status = prevCompetitionStatus;
        LpGame->m_paused = 0;
    }
    return 1;
}


// ---- src/global/checkSecretSkinObjects.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" int strcasecmp(const char *a, const char *b);


// Returns the secret-skin slot for an object named "<object>Skin<n>"
// (object i, skin n = 1..5), or -1.
int checkSecretSkinObjects(char *name)
{
    int j, i;
    char buf[32];

    for (i = 0; i < 9; i++) {
        for (j = 0; j < 5; j++) {
            sprintf(buf, "%sSkin%d", SecretSkinObjects[i], j + 1);
            if (strcasecmp(name, buf) == 0)
                return j + 2 + i * 8;
        }
    }
    return -1;
}


// ---- src/JetskiCompetition/ResetTimeElimBuoys.cpp

extern "C" char *strstr(const char *s, const char *find);









// Re-enable the time-elimination buoys ("cbalf*" objects).
void JetskiCompetition::ResetTimeElimBuoys(int resetVehicles)
{
    int i;

    if (m_scene->m_zoneModels && m_scene->m_zoneModels[0]) {
        for (i = 0; i < m_scene->m_zoneModels[0]->m_numModels; i++) {
            StaticModel *entry = &m_scene->m_zoneModels[0]->m_models[i];
            if (strstr(entry->m_name, "cbalfp") || strstr(entry->m_name, "cbalfb") || strstr(entry->m_name, "cbalfg")) {
                entry->m_object->Enable();
                entry->m_object->AddSearchPlacement(m_scene->m_staticNodes[0], 1);
                if (entry->m_collision) {
                    entry->m_collision->m_collidable = 1;
                    entry->m_collision->AddSearchPlacement(m_scene->m_dynamicNodes[0], 1);
                }
            }
        }
    }
    m_timeElimTime = m_timeElimStartTime;
    if (resetVehicles) {
        for (i = 0; i < m_numVehicles; i++)
            m_vehicles[i]->m_timeElimBuoy = 0;
        m_x358 = 1.0f;
    }
}


// ---- src/global/TimeElimCallback.cpp
// TimeElimCallback__FP15CollisionObjectP15CollisionObject  @ 0x00250950

extern unsigned int WORLDOBJECT_PINKBALLOON;
extern unsigned int WORLDOBJECT_BLUEBALLOON;
extern unsigned int WORLDOBJECT_GREENBALLOON;

// JetskiVehicle's +0xA0 (inside JetskiVehicle.h's pad9C): an object that
// records the last balloon hit at +0x120.
#ifdef SPLASHDOWN_PC
#include "JetskiCollisionModel/JetskiCollisionModel.h"
#endif
struct BalloonRecord {
#ifdef SPLASHDOWN_PC
    char pad0[__builtin_offsetof(JetskiCollisionModel, m_hitObject)];
#else
    char pad0[0x120];
#endif
    CollisionObject *m_lastBalloon;     // +0x120
};

struct VehicleBalloonView {
#ifdef SPLASHDOWN_PC
    char pad0[__builtin_offsetof(JetskiVehicle, m_collisionModel)];
#else
    char pad0[0xA0];
#endif
    BalloonRecord *m_xA0;               // +0xA0
};

// A time-elimination balloon was hit. If a vehicle hit it, add the
// balloon's time bonus (pink 2 s, blue 5 s, green 10 s, scaled by +0x358),
// play its sound, remove the balloon, and once all balloons are gone,
// put them back with half the bonus.
void TimeElimCallback(CollisionObject *balloon, CollisionObject *other)
{
    int numVehicles;
    int i;
    float bonus;
    RaceStatusClass *raceStatus;
    JetskiVehicle **vehicles;
    JetskiCompetition *comp;
    SoultreeObject *object;
    int color;
    RaceStatusStruct_Tag *status;
    comp = LpGame->m_competition;
    // Retail re-reads LpGame->m_competition here; the shared load makes comp
    // an optimizer temp, which gives retail's register for it.
    numVehicles = LpGame->m_competition->m_numVehicles;
    raceStatus = comp->m_raceStatus;
    vehicles = comp->m_vehicles;
    color = -1;

    if (balloon->m_ownerType & WORLDOBJECT_PINKBALLOON) {
        bonus = 2.0f;
        color = 0;
    } else if (balloon->m_ownerType & WORLDOBJECT_BLUEBALLOON) {
        bonus = 5.0f;
        color = 1;
    } else if (balloon->m_ownerType & WORLDOBJECT_GREENBALLOON) {
        bonus = 10.0f;
        color = 2;
    }

    for (i = 0; i < numVehicles; i++) {
        if (vehicles[i]->m_xA4 != other)
            continue;
        ((VehicleBalloonView *)vehicles[i])->m_xA0->m_lastBalloon = balloon;
        status = vehicles[i]->m_ownStatus;
        if (raceStatus && !(status->m_flags & 1)) {
            // Player 2's balloons use the second set of sounds.
            if (vehicles[i]->m_player == 1)
                color += 3;
            if (color != -1 && comp->m_balloonSounds[color])
                comp->m_balloonSounds[color]->Play(0, 0);
            raceStatus->UpdateTimeRemaining(vehicles[i]->m_ownStatus,
                                            bonus * comp->m_x358, 0);
            vehicles[i]->m_timeElimBuoy++;
            LpGame->m_competition->m_timeElimTime--;
            object = (SoultreeObject *)balloon->m_owner;
            object->Disable();
            object->RemoveFromSearch();
            balloon->m_collidable = 0;
            balloon->RemoveFromSearch();
        }
        i = numVehicles;    // retail's spelling of break
    }

    if (comp->m_timeElimTime == 0) {
        comp->ResetTimeElimBuoys(0);
        comp->m_x358 *= 0.5f;
    }
}


// ---- src/global/SecretSkinCallback.cpp

extern "C" char *strstr(const char *s, const char *find);
extern "C" char *strcpy(char *dst, const char *src);
extern "C" int sprintf(char *buf, const char *fmt, ...);


int checkSecretSkinObjects(char *name);


// A secret-skin pickup was hit. If a player's vehicle hit it, unlock the skin
// (and the character's bonus skin once skins 2-6 are all unlocked), remove
// the pickup and set up the "<character> (<skin>)" message.
void SecretSkinCallback(CollisionObject *pickup, CollisionObject *other)
{
    char name[0x50];
    char character[0x20];
    int found;
    int i;
    int index;
    SoultreeObject *object;
    JetskiCompetition *comp;
    JetskiGameOptions *options;
    object = (SoultreeObject *)pickup->m_owner;
    comp = LpGame->m_competition;
    options = LpGame->m_gameOptions;
    i = 0;
    found = 0;
    while (i < comp->m_x118) {
        if (comp->m_vehicles[i + 5]->m_xA4 == other) {
            found = 1;
            break;
        }
        i++;
    }

    // Object names carry the riders' early names.
    if (strstr(object->m_name, "Raphael")) {
        strcpy(name, object->m_name);
        sprintf(name, "Andreas%s", object->m_name + 7);
    } else if (strstr(object->m_name, "Andy")) {
        strcpy(name, object->m_name);
        sprintf(name, "Derrick%s", object->m_name + 4);
    } else {
        strcpy(name, object->m_name);
    }

    if (!found)
        return;
    if ((index = checkSecretSkinObjects(name)) < 0)
        return;

    // +0x1E0 is the pickup sound.
    PSXSound *sound = comp->m_skinSound;
    if (sound)
        sound->Play(0, 0);
    object->Disable();
    object->RemoveFromSearch();
    pickup->m_collidable = 0;
    pickup->RemoveFromSearch();
    comp->m_x2D8 = 1;
    comp->m_x2DC = i;
    strcpy(&comp->m_x2E0, object->m_name);

    int rider = index / 8;
    int skin = index % 8;
    options->m_secretSkinUnlocked[rider][skin] = 1;
    int j;
    int all = 1;
    for (j = 2; j < 7; j++) {
        if (!options->m_secretSkinUnlocked[rider][j]) {
            j = 8;  // ends the loop (retail's spelling of break)
            all = 0;
        }
    }
    if (all)
        options->m_secretSkinUnlocked[rider][7] = 1;

    switch (rider) {
    case 0:
        strcpy(character, "Andy");
        break;
    case 4:
        strcpy(character, "Juana");
        break;
    case 2:
        strcpy(character, "Rafael");
        break;
    case 3:
        strcpy(character, "Kyoko");
        break;
    case 1:
        strcpy(character, "Haily");
        break;
    case 5:
        strcpy(character, "Sebastian");
        break;
    case 6:
        strcpy(character, "Jeremy");
        break;
    case 7:
        strcpy(character, "Amman Ra");
        break;
    case 8:
        strcpy(character, "Steve");
        break;
    default:
        strcpy(character, "???");
        break;
    }

    switch (skin) {
    case 2:
        sprintf(&comp->m_x2E0, "%s (%s)", character, LpGame->m_text->GetString(0xFB));
        break;
    case 3:
        sprintf(&comp->m_x2E0, "%s (%s)", character, LpGame->m_text->GetString(0xFC));
        break;
    case 4:
        sprintf(&comp->m_x2E0, "%s (%s)", character, LpGame->m_text->GetString(0xFD));
        break;
    case 5:
        sprintf(&comp->m_x2E0, "%s (%s)", character, LpGame->m_text->GetString(0xFE));
        break;
    case 6:
        sprintf(&comp->m_x2E0, "%s (%s)", character, LpGame->m_text->GetString(0xFF));
        break;
    default:
        sprintf(&comp->m_x2E0, "%s (%s)", character, "???");
        break;
    }
}


// ---- src/JetskiCompetition/RSStartGhostVehicle.cpp
class RaceStatusClass;
struct RaceStatusStruct_Tag;



int JetskiCompetition::RSStartGhostVehicle(RaceStatusClass *, RaceStatusStruct_Tag *)
{
    if (!m_x290 && m_vcr) {
        if (m_vcr->m_loaded)
            m_vcr->REPLAYREC();
        else
            m_vcr->REC();
    }
    return 1;
}


// ---- src/JetskiCompetition/RSStopGhostVehicle_00251350.cpp
class RaceStatusClass;
struct RaceStatusStruct_Tag;


int JetskiCompetition::RSStopGhostVehicle(RaceStatusClass *, RaceStatusStruct_Tag *)
{
    return 0;
}


// ---- src/JetskiCompetition/RSUpdateBestGhostLap.cpp


class RaceStatusClass;
struct RaceStatusStruct_Tag;








int JetskiCompetition::RSUpdateBestGhostLap(RaceStatusClass *, RaceStatusStruct_Tag *, RaceStatusStruct_Tag *)
{
    if (m_vcr) {
        m_vcr->STOP();
        m_vcr->RESET();
        if (m_options->m_x54 == 2) {
            JetskiVehicle *vehicle = (JetskiVehicle *)m_ghost;
            vehicle->m_rider->RestartAction();
            vehicle->m_rider->ResetVariables();
            vehicle->m_rider->StopSpecialAnimation();
            vehicle->m_rider->m_wrecker->StopWreck();
            vehicle->m_x530 = 0;
            vehicle->m_body->m_velocity = ZeroVector;
            vehicle->m_body->m_speed = 0;
        }
    }
    return 0;
}


// ---- src/JetskiCompetition/LoadGenericVehicleSounds.cpp
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" unsigned int strlen(const char *s);
extern "C" char *strcpy(char *dst, const char *src);
extern "C" void *malloc(unsigned int size);
extern "C" void free(void *p);





// The generic (per-model) sound lists every vehicle loads, one per block of
// the vehicle sound file.
struct VehicleAudType {
    ContainerList<char *> m_spray;          // +0x00
    ContainerList<char *> m_edgeMonster;    // +0x14
    ContainerList<char *> m_splashIdle;     // +0x28
    ContainerList<char *> m_splashSmall;    // +0x3C
    ContainerList<char *> m_splashMid;      // +0x50
    ContainerList<char *> m_splashLarge;    // +0x64
    ContainerList<char *> m_splashSmack;    // +0x78
    ContainerList<char *> m_slowScrape;     // +0x8C
    ContainerList<char *> m_midScrape;      // +0xA0
    ContainerList<char *> m_fastScrape;     // +0xB4
    ContainerList<char *> m_groundImpact;   // +0xC8
    ContainerList<char *> m_runAground;     // +0xDC
    ContainerList<char *> m_boatImpact;     // +0xF0
};


#define LOAD_SOUND_LIST(block, list)                            \
    if (params.ReadParameterBlock(block)) {                     \
        params.GetParameter("NumSounds", 0, &numSounds);        \
        for (i = 1; i <= numSounds; i++) {                      \
            sprintf(key, "Sound_%d", i);                        \
            params.GetParameter(key, value, -1);                \
            name = (char *)malloc(strlen(value) + 1);           \
            strcpy(name, value);                                \
            audType.list.AddItem(name);                         \
        }                                                       \
    }

#define FREE_SOUND_LIST(list)                                   \
    while (audType.list.m_numItems) {                           \
        name = audType.list.GetItem(0);                         \
        audType.list.RemoveItemByIndex(0);                      \
        free(name);                                             \
    }

int JetskiCompetition::LoadGenericVehicleSounds(char *fileName)
{
    ParameterBlock params;
    char value[0x110];
    char key[0x40];
    VehicleAudType audType;
    int numSounds;
    char *name;     // declared before i: gives the first list loop its i=$s1/name=$s0
    int i;
    if (!params.OpenFile(fileName, 0))
        return 0;
    audType.m_spray.Create(1, 16);
    audType.m_edgeMonster.Create(1, 16);
    audType.m_splashIdle.Create(1, 16);
    audType.m_splashSmall.Create(1, 16);
    audType.m_splashMid.Create(1, 16);
    audType.m_splashLarge.Create(1, 16);
    audType.m_splashSmack.Create(1, 16);
    audType.m_slowScrape.Create(1, 16);
    audType.m_midScrape.Create(1, 16);
    audType.m_fastScrape.Create(1, 16);
    audType.m_groundImpact.Create(1, 16);
    audType.m_runAground.Create(1, 16);
    audType.m_boatImpact.Create(1, 16);
    LOAD_SOUND_LIST("Spray", m_spray)
    LOAD_SOUND_LIST("EdgeMonster", m_edgeMonster)
    LOAD_SOUND_LIST("SplashIdle", m_splashIdle)
    LOAD_SOUND_LIST("SplashSmall", m_splashSmall)
    LOAD_SOUND_LIST("SplashMid", m_splashMid)
    LOAD_SOUND_LIST("SplashLarge", m_splashLarge)
    LOAD_SOUND_LIST("SplashSmack", m_splashSmack)
    LOAD_SOUND_LIST("SlowScrape", m_slowScrape)
    LOAD_SOUND_LIST("MidScrape", m_midScrape)
    LOAD_SOUND_LIST("FastScrape", m_fastScrape)
    LOAD_SOUND_LIST("GroundImpact", m_groundImpact)
    LOAD_SOUND_LIST("RunAground", m_runAground)
    LOAD_SOUND_LIST("BoatImpact", m_boatImpact)
    for (i = 0; i < m_numVehicles; i++) {
        JetskiVehicle *vehicle = m_vehicles[i];
        vehicle->LoadGenericSounds(&audType);
        vehicle->m_responses = new ResponseEmitter(LpGame->m_auralScape, m_scene->m_soundGroup, 6, 1);
        if (vehicle->m_responses) {
            _DS3DBUFFER buffer;
            buffer.flMinDistance = 0.0f;
            buffer.flMaxDistance = 100.0f;
            ResponseEmitter *emitter = vehicle->m_responses->Create(buffer, 2, 20.0f, 100.0f, 0, 0);
            if (emitter) {
                emitter->LoadResponseList(&params);
                AddChild(vehicle->m_responses, -1);
            } else if (vehicle->m_responses && !vehicle->m_responses->Release()) {
                vehicle->m_responses = 0;
            }
        }
    }
    FREE_SOUND_LIST(m_spray)
    FREE_SOUND_LIST(m_edgeMonster)
    FREE_SOUND_LIST(m_splashIdle)
    FREE_SOUND_LIST(m_splashSmall)
    FREE_SOUND_LIST(m_splashMid)
    FREE_SOUND_LIST(m_splashLarge)
    FREE_SOUND_LIST(m_splashSmack)
    FREE_SOUND_LIST(m_slowScrape)
    FREE_SOUND_LIST(m_midScrape)
    FREE_SOUND_LIST(m_fastScrape)
    FREE_SOUND_LIST(m_groundImpact)
    FREE_SOUND_LIST(m_runAground)
    FREE_SOUND_LIST(m_boatImpact)
    audType.m_spray.Clear();
    audType.m_edgeMonster.Clear();
    audType.m_splashIdle.Clear();
    audType.m_splashSmall.Clear();
    audType.m_splashMid.Clear();
    audType.m_splashLarge.Clear();
    audType.m_splashSmack.Clear();
    audType.m_slowScrape.Clear();
    audType.m_midScrape.Clear();
    audType.m_fastScrape.Clear();
    audType.m_groundImpact.Clear();
    audType.m_runAground.Clear();
    audType.m_boatImpact.Clear();
    return 1;
}


// ---- src/JetskiCompetition/FadeInCameras.cpp



class JetskiCompetition;




int JetskiCompetition::FadeInCameras()
{
    for (int i = 0; i < m_numPlayers; i++) {
        m_fadeOutTime[m_cameraControllers[i]->m_viewport] = 0;
        m_fadeInTime[m_cameraControllers[i]->m_viewport] = 0;
        m_faded[m_cameraControllers[i]->m_viewport] = 0;
        if (m_videoFX) {
            m_videoFX->m_state[m_cameraControllers[i]->m_viewport] = 2;
            m_videoFX->m_duration[m_cameraControllers[i]->m_viewport] = 100;
        }
    }
    if (m_options->m_raceType == 8)
        ((JetskiTraining *)LpGame->m_competition)->m_x474 = 1;
    return 1;
}


// ---- src/JetskiCompetition/ResetAllEngineSounds.cpp


void JetskiCompetition::ResetAllEngineSounds()
{
    for (int i = 0; i < m_numVehicles; i++) {
        if (m_vehicles[i]->m_engineSound)
            m_vehicles[i]->m_engineSound->ResetState();
    }
}


// The RaceStatusVehicleInterface/RaceStatusEventInterface handlers and
// JetskiGhostVehicle::RSGetVehicleStatus are inline; mwcc writes them here,
// with the vtables (retail 0x252620-0x252790).

