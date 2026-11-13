// PSXAudio.cpp  @ 0x0026B1B0 (text 0x26B1B0-0x26EB00)
// MATCHED (mwccps2 2.4.1.01, -O4,p -Cpp_exceptions off -sdatathreshold 1 -str readonly, mode: cpp) — byte-identical to retail.
//
// Retail's PSXAudio.cpp as one translation unit: the sound group ids,
// PSXSoundInterface, PSXSound, PSXMusic, PSXSoundGroup and Listener, then its
// ContainerList<PSXSound *> and ContainerList<int> members (instantiated at the
// end, CONTAINERLIST_MEMBERS_ALL) and the static initializer (only the axis
// vectors). Its .rodata is @844-@2130 (labels restart after
// PhysicsRigidBody.cpp's @1095; RTTI Listener ... SoundInterface); its .bss
// starts with gGroupIds (0x473F60).


#include "SoundInterface/SoundInterface.h"
#include "PSXSoundInterface/PSXSoundInterface.h"
#include "StringTable/StringTable.h"
#include "RainbowPS2Snd/RainbowPS2Snd.h"
#include "PSXSound/PSXSound.h"
#include "PSXMusic/PSXMusic.h"            // before PSXSoundGroup.h: ContainerList<int> is
#include "PSXSoundGroup/PSXSoundGroup.h"  // instantiated first, its members written last
#include "d3dtypes.h"
#include "Sound/Sound.h"
#include "ContainerList/ContainerList.h"
#include "SoundGroup/SoundGroup.h"
#include "soultree_assert_block.h"
#define CONTAINERLIST_MEMBERS_ALL
#include "ContainerList/ContainerListMembers.h"
#include "Game/Game.h"
#include "GameObject/GameObject.h"
#include "sinit/AxisVectors.h"
#include "Listener/Listener.h"

// ---- src/global/InitializeGroupIds.cpp
int gGroupIds[25];  // .bss 0x473F60: 1 = sound group id in use

void InitializeGroupIds()
{
    for (int i = 0; i < 25; i++) {
        if (i < 2)
            gGroupIds[i] = 1;
        else
            gGroupIds[i] = 0;
    }
}


// ---- src/global/GetGroupId.cpp

int GetGroupId()
{
    for (int i = 0; i < 25; i++) {
        if (gGroupIds[i] == 0) {
            gGroupIds[i] = 1;
            return i;
        }
    }
    return -1;
}


// ---- src/global/ReleaseGroupId.cpp

void ReleaseGroupId(int id)
{
    gGroupIds[id] = 0;
}


// ---- src/PSXSoundInterface/ctor.cpp


PSXSoundInterface::PSXSoundInterface()
{
    m_flag = 0;
    m_surround = 0;
    m_soundStringTable = 0;
}


// ---- src/PSXSoundInterface/dtor.cpp


PSXSoundInterface::~PSXSoundInterface()
{
    m_snd.StopAllVoices();
    m_flag = 0;
    if (m_soundStringTable)
        m_soundStringTable->Release();
}


// ---- src/PSXSoundInterface/Create.cpp
void PSXprintf(const char *fmt, ...);
void InitializeGroupIds();



int PSXSoundInterface::Create(unsigned int flags)
{
    if (!m_flag) {
        m_snd.Init(flags);
        PSXprintf("Started Sound System...\n");
        SetSFXVolume(100);
        SetStreamVolume(100);
        SetSpeakerConfiguration(1);
    }
    m_flag = 1;
    InitializeGroupIds();
    return 0;
}


// ---- src/PSXSoundInterface/SetSpeakerConfiguration.cpp
void PSXprintf(const char *fmt, ...);


int PSXSoundInterface::SetSpeakerConfiguration(unsigned int config)
{
    if (config < 2) {
        m_surround = 0;
    } else if (config == 2) {
        m_surround = 1;
    } else {
        PSXprintf("Invalid parameter passed to SetSpeakerConfiguration\n");
        return 0;
    }
    m_speakerConfig = config;
    return 1;
}


// ---- src/PSXSoundInterface/SetSFXVolume.cpp
void PSXprintf(const char *fmt, ...);


int PSXSoundInterface::SetSFXVolume(long volume)
{
    if (volume < 0 || volume > 100) {
        PSXprintf("Invalid volume for SetSFXVolume\n");
        return 0;
    }
    m_sfxVolume = volume;
    m_snd.VolumeGroup(0, volume);
    return 1;
}


// ---- src/PSXSoundInterface/SetStreamVolume.cpp
void PSXprintf(const char *fmt, ...);


int PSXSoundInterface::SetStreamVolume(long volume)
{
    if (volume < 0 || volume > 100) {
        PSXprintf("Invalid volume for SetStreamVolume\n");
        return 0;
    }
    m_streamVolume = volume;
    m_snd.VolumeGroup(1, volume);
    return 1;
}


// ---- src/PSXSoundInterface/SetDolby.cpp

void PSXSoundInterface::SetDolby(int dolby)
{
    m_snd.m_surround = dolby;
}


// ---- src/PSXSoundInterface/SetMono.cpp

void PSXSoundInterface::SetMono(int mono)
{
    m_snd.m_mono = mono;
}


// ---- src/PSXSoundInterface/GetAvailable3DHardwareVoices.cpp

int PSXSoundInterface::GetAvailable3DHardwareVoices()
{
    return 48;
}


// ---- src/PSXSoundInterface/CreateListener.cpp


Listener *PSXSoundInterface::CreateListener()
{
    if (!m_flag)
        return 0;
    Listener *listener = new Listener;
    if (listener)
        return listener;
    return 0;
}


// ---- src/PSXSoundInterface/GetSoundFromResourceName.cpp
extern "C" char *strncpy(char *dst, const char *src, unsigned int n);

class PSXSoundInterface;
class PSXSoundGroup;




PSXSound *PSXSoundInterface::GetSoundFromResourceName(PSXSoundGroup *group, char *name, unsigned int a, unsigned int b)
{
    PSXSound *sound = new PSXSound(this, group, 1);
    if (!sound)
        return 0;
    if (!sound->CreateFromFile(name, a, b)) {
        group->RemoveSound(sound);
        return 0;
    }
    strncpy(sound->m_name, name, sizeof(sound->m_name) - 1);
    sound->m_name[sizeof(sound->m_name) - 1] = 0;
    return sound;
}


// ---- src/PSXSoundInterface/CreateMusic.cpp


int PSXSoundInterface::CreateMusic(int a, int track, int c)
{
    PSXMusic *music = new PSXMusic(this, c, a);
    if (!music)
        return 0;
    return (int)music->Create(track);
}


// ---- src/PSXSoundInterface/LoadSoundStringTable.cpp


int PSXSoundInterface::LoadSoundStringTable(char *filename)
{
    if (m_soundStringTable)
        delete m_soundStringTable;
    m_soundStringTable = new StringTable;
    return m_soundStringTable->Load(filename);
}


// ---- src/PSXSoundInterface/ClearSoundStringTable.cpp


void PSXSoundInterface::ClearSoundStringTable()
{
    if (m_soundStringTable) {
        for (int i = 0; i < m_soundStringTable->m_totalEntries; i++) {
            m_soundStringTable->ClearRequested(i);
            m_soundStringTable->SetPointer(i, 0);
        }
    }
}


// ---- src/PSXSoundInterface/LoadRequestedSounds.cpp



void PSXSoundInterface::LoadRequestedSounds()
{
    for (int i = 0; i < m_soundStringTable->m_totalEntries; i++) {
        if (m_soundStringTable->GetRequested(i) > 0) {
            char *name = m_soundStringTable->GetString(i);
            PSXSound *sound = (PSXSound *)m_soundStringTable->GetPointer(i);
            sound->LoadPreviouslyCreatedSound(name);
            m_soundStringTable->SetPointer(i, (void *)PS2_ADDR(sound->m_soundHandle));
            m_soundStringTable->ClearRequested(i);
        }
    }
}


// ---- src/PSXSound/ctor.cpp
extern "C" char *strcpy(char *dst, const char *src);



class PSXSound;
class PSXSoundInterface;




PSXSound::PSXSound(PSXSoundInterface *soundInterface, PSXSoundGroup *group, short)
{
    m_soundHandle = -1;
    m_handle = -1;
    m_interface = soundInterface;
    m_group = group;
    m_group->AddSound(this);
    m_baseVolume = 100;
    m_basePan = 0;
    m_pitch = 0x1000;
    m_volume = 100;
    m_pan = 0;
    m_playPitch = 0x1000;
    m_basePitch = 0x1000;
    m_frequencyModifier = 1.0f;
    m_x70 = 0;
    m_unk74 = 0;
    m_paused = 0;
    m_velocity = ZeroVector;
    m_x198 = 0;
    m_voice = 0;
    m_streamed = 0;
    m_elapsed = 0;
    m_duration = 0;
    m_stream = -1;
    m_nameIndex = -1;
    strcpy(m_name, "");
}


// ---- src/PSXSound/dtor.cpp
// Local copy: the shared PSXSound.h derives from BaseObject, not Sound
// (see there), which changes this destructor's base call.
struct _D3DVECTOR;






PSXSound::~PSXSound()
{
    if (m_state == 2 && m_handle >= 0)
        m_interface->m_snd.StopVoice(m_handle, m_voice, 0);
}


// ---- src/PSXSound/CreateFromFile.cpp
extern "C" int strcmp(const char *a, const char *b);
extern "C" char *strcat(char *dst, const char *src);
extern "C" char *strcpy(char *dst, const char *src);
extern "C" int sprintf(char *buf, const char *fmt, ...);
void PSXprintf(const char *fmt, ...);

extern char AudioContentDirectory[];




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


int PSXSound::CreateFromFile(char *name, unsigned int flags, unsigned int arg)
{
    m_state = flags;
    if (!strcmp(name, ""))
        return 0;
    if (flags & 0x80)
        m_streamed = 1;
    if (flags & 0x40)
        m_voice = 1;
    if (flags & 0x2)
        m_x198 = 1;
    strcat(m_name, name);
    if (!m_voice) {
        char *ext = FindLast(m_name, '.');
        if (ext)
            strcpy(ext, ".vag");
        else
            strcat(m_name, (const char *)".vag");  // the cast loads the literal first, as in retail
    }
    if (m_x198) {
        if (!m_interface->m_snd.PreLoadStream(m_name, &m_stream)) {
            PSXprintf("Failed to PreLoadStream %s in PSXSound::CreateFromFile\n", name);
            return 0;
        }
        m_duration = m_interface->m_snd.GetStreamDuration(m_stream) / 1000;
        m_basePitch = m_interface->m_snd.GetStreamPitch(m_stream);
        m_pitch = m_basePitch;
    } else {
        char path[0x80];
        sprintf(path, "%s%s", AudioContentDirectory, m_name);
        if (m_voice)
            strcat(path, "l.vag");
        m_nameIndex = m_interface->m_soundStringTable->GetIndex(path);
        m_interface->m_soundStringTable->Request(m_nameIndex);
        m_interface->m_soundStringTable->SetPointer(m_nameIndex, this);
    }
    return 1;
}


// ---- src/PSXSound/LoadPreviouslyCreatedSound.cpp
extern "C" char *strcpy(char *dst, const char *src);






// An inlined strrchr.

static inline int IsValidHandle(int handle)
{
    return handle >= 0;
}

int PSXSound::LoadPreviouslyCreatedSound(char *name)
{
    char buf[0x80];
    strcpy(buf, name);
    // Cut the name one character before its extension. Retail doesn't check
    // for a missing '.', which would write through (char *)0 - 1.
    if (m_voice)
        strcpy(FindLast(buf, '.') - 1, "");
    m_soundHandle = m_interface->m_snd.LoadSound(buf, m_group->m_groupID, m_streamed, m_voice, m_x198);
    if (IsValidHandle(m_soundHandle)) {
        m_duration = m_interface->m_snd.GetSoundDuration(m_soundHandle) / 1000;
        m_basePitch = m_interface->m_snd.GetSoundPitch(m_soundHandle);
        m_pitch = m_basePitch;
    }
    return 1;
}



// ---- src/PSXSound/Play.cpp





int PSXSound::Play(int restart, int loop)
{
    if (m_soundHandle < 0 && !m_x198) {
        m_soundHandle = (int)m_interface->m_soundStringTable->GetPointer(m_nameIndex);
        if (m_soundHandle >= 0) {
            m_duration = m_interface->m_snd.GetSoundDuration(m_soundHandle) / 1000;
            m_basePitch = m_interface->m_snd.GetSoundPitch(m_soundHandle);
            m_pitch = m_basePitch;
        }
    }
    if (m_soundHandle < 0 && !m_x198)
        return 0;
    if (restart && IsPlaying())
        Stop(0);
    if (m_x198) {
        unsigned int voice;
        m_handle = m_interface->m_snd.LoadAndPlayStream(m_name, &voice, m_group->m_groupID, m_voice, -1, -1, -1,
                                                        m_stream);
        if (m_handle < 0)
            return 0;
    } else {
        m_handle = m_interface->m_snd.PlaySound(m_soundHandle, m_voice, m_x198, m_streamed, m_baseVolume, m_basePan, m_pitch, loop);
    }
    m_playPitch = m_pitch;
    m_volume = m_baseVolume;
    m_pan = m_basePan;
    if (m_handle < 0)
        return 0;
    m_elapsed = 0;
    m_unk74 = 1;
    return 1;
}


// ---- src/PSXSound/Pause.cpp

int PSXSound::Pause(int pause)
{
    if (m_handle == -1)
        return 0;
    if (pause) {
        if (m_paused)
            return 1;
        m_interface->m_snd.PauseVoice(m_handle);
        m_paused = 1;
        return 1;
    } else if (m_paused) {
        m_interface->m_snd.PauseVoice(m_handle);
        m_paused = 0;
        return 1;
    }
    return 1;
}


// ---- src/PSXSound/Stop.cpp

int PSXSound::Stop(int fade)
{
    if (m_handle == -1)
        return 1;
    if (!IsPlaying()) {
        m_handle = -1;
        return 1;
    }
    m_interface->m_snd.StopVoice(m_handle, m_voice, fade);
    m_elapsed = 0;
    m_handle = -1;
    m_unk74 = 0;
    return 1;
}


// ---- src/PSXSound/IsPlaying.cpp
int PSXSound::IsPlaying()
{
    if (m_handle == -1)
        return 0;
    return m_interface->m_snd.IsSoundPlaying(m_handle, 0) == 1;
}


// ---- src/PSXSound/SetVolume.cpp
#pragma fast_fptosi on

void PSXprintf(const char *fmt, ...);
struct _D3DVECTOR;





int PSXSound::SetVolume(float volume)
{
    if (volume < 0.0f || volume > 100.0f) {
        PSXprintf("Invalid volume value for Sound\n");
        return 0;
    }
    m_volume = volume;
    if (IsPlaying() && m_handle >= 0)
        m_interface->m_snd.VolumeVoice(m_handle, m_volume, 0, 0);
    return 1;
}
#pragma fast_fptosi reset


// ---- src/PSXSound/SetBaseVolume.cpp
#pragma fast_fptosi on

void PSXprintf(const char *fmt, ...);


int PSXSound::SetBaseVolume(float volume)
{
    if (volume < 0.0f || volume > 100.0f) {
        PSXprintf("Invalid volume value for Sound\n");
        return 0;
    }
    m_baseVolume = volume;
    return 1;
}
#pragma fast_fptosi reset


// ---- src/PSXSound/SetPan.cpp

int PSXSound::SetPan(long pan)
{
    if (m_interface->m_surround == 1)
        m_pan = -pan;
    else
        m_pan = pan;
    if (m_pan < 0)
        m_pan += 0x167;
    if (IsPlaying() && m_handle >= 0)
        m_interface->m_snd.PanVoice(m_handle, m_pan);
    return 1;
}


// ---- src/PSXSound/SetBasePan.cpp
int PSXSound::SetBasePan(long pan)
{
    if (m_interface->m_surround == 1)
        m_basePan = -pan;
    else
        m_basePan = pan;
    if (m_basePan < 0)
        m_basePan += 0x167;
    return 1;
}


// ---- src/PSXSound/SetFrequency.cpp

int PSXSound::SetFrequency(unsigned int frequency)
{
    if (frequency > 0x3FFF)
        return 0;
    if (IsPlaying() && m_handle >= 0)
        m_interface->m_snd.PitchVoice(m_handle, frequency, 0, 0);
    return 1;
}


// ---- src/PSXSound/SetFrequencyModifier.cpp

int PSXSound::SetFrequencyModifier(float modifier)
{
    if (modifier < 0.0f || modifier > 16383.0f)
        return 0;
    m_frequencyModifier = modifier;
    return 1;
}


// ---- src/PSXSound/UpdateSettings.cpp
#pragma fast_fptosi on

void PSXprintf(const char *fmt, ...);


// Retail's reciprocal square root is an inline-asm helper: rsqrt.s into $f8,
// stored through a pointer to a stack temporary.
static inline float RSqrt(float x)
{
    float r;
    asm("rsqrt.s $f8, %0, %1; swc1 $f8, 0(%2)" : : "f"(1.0f), "f"(x), "r"(&r) : "$f8", "memory");
    return r;
}

static inline float Length(const _D3DVECTOR &v)
{
    float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len2 == 0.0f)
        return 0.0f;
    return 1.0f / RSqrt(len2);
}




// Applies volume, pan and a doppler-shifted pitch to the playing voice. The
// pitch is raised when the relative speed is falling (approaching) and lowered
// when it is rising, by up to half an octave per 1125 ft/s (speed of sound).
int PSXSound::UpdateSettings(float volume, long pan, _D3DVECTOR velocity, int doppler)
{
    static int lastPitch = 0;
    _D3DVECTOR relative = velocity - m_position;
    if (m_handle == -1)
        return 0;
    if (volume < 0.0f || volume > 100.0f) {
        PSXprintf("Invalid volume value for Sound\n");
        return 0;
    }
    m_volume = (int)(m_baseVolume * volume) / 100;
    if (!m_interface->m_surround)
        m_pan = -pan;
    else
        m_pan = pan;
    if (m_pan < 0)
        m_pan += 0x167;
    if (pan < -360 || pan > 360) {
        PSXprintf("Invalid Pan value\n");
        return 0;
    }
    if (Length(m_velocity) < Length(relative))
        doppler = -doppler;
    float shift = 1.0f + 0.5f * (doppler / 1125.0f);
    int pitch = m_frequencyModifier * (m_pitch * shift);

    // Slew-limit the pitch by 5 per update. Retail stores the difference
    // instead of the target for small steps, and never uses the result.
    if (lastPitch < pitch) {
        int delta = pitch - lastPitch;
        if (delta > 5)
            lastPitch += 5;
        else
            lastPitch = delta;
    } else if (pitch < lastPitch) {
        int delta = pitch - lastPitch;
        if (delta < -5)
            lastPitch -= 5;
        else
            lastPitch = delta;
    } else {
        lastPitch = pitch;
    }
    m_velocity = relative;
    if (pitch < -1 || pitch > 0x3FFF) {
        PSXprintf("Invalid Pitch for sound\n");
        return 0;
    }
    return m_interface->m_snd.SettingsVoice(m_handle, m_volume, m_pan, pitch, 0, 0, 0, 0, 0);
}
#pragma fast_fptosi reset


// ---- src/PSXSound/SetMinMaxDistance.cpp

int PSXSound::SetMinMaxDistance(float, float)
{
    return 0;
}


// ---- src/PSXSound/SetPosition.cpp


int PSXSound::SetPosition(_D3DVECTOR position)
{
    m_position.x = position.x;
    m_position.y = position.y;
    m_position.z = position.z;
    return 1;
}


// ---- src/PSXSound/SetVelocity.cpp

int PSXSound::SetVelocity(_D3DVECTOR)
{
    return 0;
}


// ---- src/PSXSound/Tick.cpp

int PSXSound::Tick(float dt)
{
    if (m_handle >= 0)
        m_elapsed += dt;
    return 1;
}


// ---- src/PSXMusic/ctor.cpp
#pragma fast_fptosi on


// mwcc session-state shim: retail was
// built in one compiler run, so an earlier 64-bit divide had raised the
// helper-argument high-water mark; this unused function reproduces it.
static void MWCC_SHIM_TEXT MwccSessionState(long *a) { a[0] = a[0] / a[1]; }

extern int TimerOverflowCounterT0;

inline unsigned int timeGetTime()
{
    static long Time = 0;
    static int lastCount = 0;
    int count;
    int elapsed;
    int overflows;
    count = *(volatile int *)0x10000000;    // T0_COUNT
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

PSXMusic::PSXMusic(PSXSoundInterface *soundInterface, int volume, int enabled)
    : Music(enabled)
{
    m_volume = volume;
    m_track = 0;
    m_streamHandle = 0;
    m_voice = -1;
    m_numTracks = 1;
    m_tracks = 0;
    m_created = 0;
    m_x54 = 0;
    m_x5C = 10000;
    m_position = 0;
    m_x60 = 0;
    m_x88 = 0;
    m_x8C = 0;
    m_x94 = 0;
    m_x98 = 0;
    m_startTime = timeGetTime();
    m_x68 = 0;
    m_system = soundInterface;
    m_playing = 0;
    m_paused = 0;
    m_mode = 4;
    m_playList.Create(1, 16);
}
#pragma fast_fptosi reset


// ---- src/PSXMusic/dtor.cpp
extern "C" void free(void *p);


// Growable array with an inline destructor (layout as ContainerList<T>).
PSXMusic::~PSXMusic()
{
    if (m_voice >= 0)
        m_system->m_snd.StopVoice(m_voice, 1, 0);
    if (m_tracks)
        free(m_tracks);
}


// ---- src/PSXMusic/Create.cpp
extern "C" void *malloc(unsigned int size);
extern "C" int sprintf(char *buf, const char *fmt, ...);



struct MusicTrack {
    char m_name[0x104];   // +0x000
    int m_stream;         // +0x104
};


// Tracks are numbered from 1: song01.vag, song02.vag, ...
PSXMusic *PSXMusic::Create(int numTracks)
{
    m_numTracks = numTracks;
    m_tracks = (MusicTrack *)malloc(m_numTracks * sizeof(MusicTrack));
    for (int i = 1; i <= m_numTracks; i++) {
        sprintf(m_tracks[i - 1].m_name, "song%02d.vag", i);
        m_system->m_snd.PreLoadStream(m_tracks[i - 1].m_name, &m_tracks[i - 1].m_stream);
    }
    m_created = 1;
    return this;
}


// ---- src/PSXMusic/Play.cpp
void PSXprintf(const char *fmt, ...);





// Play a track (-1 = the current one, repeating). Returns 1 if it's playing.
int PSXMusic::Play(int track, int restart)
{
    if (!m_created)
        return 0;
    m_x90 = -5.0f;
    m_mode = 1;
    if (track == -1) {
        track = m_track;
        m_mode = 3;
    }
    if (m_voice >= 0 && restart)
        Stop(0.0f);
    else if (m_voice >= 0)
        return 1;
    if (m_track == track && !restart)
        return 0;
    if (track <= m_numTracks) {
        if (m_voice < 0)
            m_voice = m_system->m_snd.LoadAndPlayStream(m_tracks[track - 1].m_name, &m_streamHandle, m_volume, 1,
                                                         -1, -1, -1, m_tracks[track - 1].m_stream);
        if (m_voice < 0) {
            m_x54 = 0;
            m_playing = 0;
            return 0;
        }
        PSXprintf("Playing music track %d.\n", track);
        m_x54 = 1;
        m_playing = 1;
    } else {
        PSXprintf("Cannot play music track %d.\n", track);
        return 0;
    }
    m_track = track;
    return 1;
}


// ---- src/PSXMusic/Stop.cpp



int PSXMusic::Stop(float fadeTime)
{
    if (!m_created)
        return 0;
    if (m_voice != -1) {
        if (fadeTime == 0.0f) {
            m_system->m_snd.StopVoice(m_voice, 1, 0);
            m_voice = -1;
            m_playing = 0;
            m_x54 = 0;
            m_x98 = 0;
            m_x94 = 0;
            m_x88 = 0;
            m_x8C = 0;
            m_mode = 4;
            return 1;
        }
        float remaining = m_streamHandle - m_position;
        if (remaining < fadeTime) {
            if (m_mode == 3) {
                if (m_streamHandle < fadeTime) {
                    m_x94 = 1;
                    m_x88 = 0.9f * m_streamHandle;
                    m_x8C = 0.8f * m_streamHandle;
                    return 1;
                } else if (remaining < 0.66f * fadeTime) {
                    m_x94 = 1;
                    m_x88 = fadeTime + 0.25;  // double constant: retail goes through dpadd
                    m_x8C = fadeTime;
                    return 1;
                } else {
                    m_x98 = 1;
                    m_x88 = 0.9f * remaining;
                    m_x8C = 0.8f * remaining;
                    RampVolume(0.0f, m_x8C, 0);
                    return 1;
                }
            } else {
                m_x98 = 1;
                m_x88 = fadeTime + 0.25;  // double constant: retail goes through dpadd
                m_x8C = fadeTime;
                RampVolume(0.0f, fadeTime, 0);
                return 1;
            }
        } else {
            m_x98 = 1;
            m_x88 = 0.9f * remaining;
            m_x8C = 0.8f * remaining;
            RampVolume(0.0f, m_x8C, 0);
            return 1;
        }
    }
    return 0;
}


// ---- src/PSXMusic/Pause.cpp

int PSXMusic::Pause(int pause)
{
    if (!m_created)
        return 0;
    if (m_voice == -1)
        return 0;
    if (pause) {
        if (m_paused)
            return 1;
        m_system->m_snd.PauseVoice(m_voice);
        m_paused = 1;
        return 1;
    }
    if (!m_paused)
        return 1;
    m_system->m_snd.PauseVoice(m_voice);
    m_paused = 0;
    return 1;
}


// ---- src/PSXMusic/Repeat.cpp

int PSXMusic::Repeat(int track, int flags)
{
    if (!Play(track, flags))
        return 0;
    m_mode = 3;
    return 1;
}


// ---- src/PSXMusic/Shuffle.cpp

#pragma fast_fptosi on
#pragma inline_depth(4)

extern "C" int rand(void);
extern "C" void srand(unsigned int seed);
extern int TimerOverflowCounterT0;

inline int GetRandomNumber(int lo, int hi)
{
    // volatile: retail reloads seeder for srand() right after storing it
    static volatile unsigned int seeder = 0;
    if (!seeder) {
        seeder = timeGetTime();
        srand(seeder);
    }
    int r = (int)((hi - lo + 1) * (rand() * 4.656613e-10f)) + lo;
    r = r > hi ? hi : r;
    if (r < lo)
        r = lo;
    return r;
}


int PSXMusic::Shuffle(int play)
{
    if (!m_created)
        return 0;
    int track = GetRandomNumber(1, m_numTracks);
    if (m_numTracks > 1) {
        if (m_playList.m_numItems == m_numTracks)
            m_playList.Clear();
        while (track == m_track || !m_playList.AddItemUnique(track))
            track = track % m_numTracks + 1;
    }
    if (play) {
        if (!Play(track, 1))
            m_playList.RemoveItem(track);
    }
    m_mode = 2;
    return track;
}
#pragma fast_fptosi reset


// ---- src/PSXMusic/ShuffleStartingAt.cpp


int PSXMusic::ShuffleStartingAt(int track, int play)
{
    if (!m_created)
        return 0;
    if (m_playList.m_numItems > 0)
        m_playList.Clear();
    m_playList.AddItemUnique(track);
    if (play) {
        if (!Play(track, 1)) {
            m_playList.RemoveItem(track);
            m_track = track;
            m_mode = 5;
        } else {
            m_mode = 2;
        }
    } else {
        m_track = track;
        m_mode = 5;
    }
    return 1;
}


// ---- src/PSXMusic/Tick.cpp
extern int TimerOverflowCounterT0;

// Win32-style timeGetTime: EE timer 0 (T0_COUNT at 0x10000000) ticks plus the
// overflows counted by TimerHandlerT0, accumulated and scaled to milliseconds.




int PSXMusic::Tick(float dt)
{
    int update = 0;
    timeGetTime();
    m_position += dt;
    if (m_playing)
        m_x90 += dt;
    if (m_x90 >= 1.0f) {
        update = 1;
        m_x90 = 0;
    }
    if (m_x98) {
        m_x88 -= dt;
        if (m_x88 <= 0.0f)
            Stop(0.0f);
    }
    if (m_x94 && m_x54) {
        m_x98 = 1;
        m_x94 = 0;
        RampVolume(0.0f, m_x8C, 0);
    }
    m_x54 = 0;
    if (m_playing && m_voice >= 0 && !m_system->m_snd.IsSoundPlaying(m_voice, update)) {
        m_position = 0;
        m_playing = 0;
        m_voice = -1;
        m_x54 = 0;
        if (m_mode == 2)
            Shuffle(1);
        if (m_mode == 3)
            Repeat(m_track, 1);
        if (m_mode == 1)
            m_mode = 4;
    }
    if (!m_playing && m_mode == 2)
        Shuffle(1);
    if (!m_playing && m_mode == 5 && Play(m_track, 1)) {
        m_playList.AddItemUnique(m_track);
        m_mode = 2;
    }
    if (!m_playing && m_mode == 3 && m_track >= 0)
        Repeat(m_track, 1);
    return 1;
}


// ---- src/PSXMusic/RampVolume.cpp
#pragma fast_fptosi on




int PSXMusic::RampVolume(float volume, float time, int a)
{
    if (m_voice == -1)
        return 0;
    return m_system->m_snd.VolumeVoice(m_voice, (int)volume, a, (int)(1000.0f * time)) == 0;
}
#pragma fast_fptosi reset


// ---- src/PSXSoundGroup/ctor.cpp


class PSXSoundInterface;


PSXSoundGroup::PSXSoundGroup(PSXSoundInterface *soundInterface, int id)
    : SoundGroup(id)
{
    m_sounds.Create(8, 16);
    m_interface = soundInterface;
    m_groupID = 2;
}


// ---- src/PSXSoundGroup/dtor.cpp

void ReleaseGroupId(int id);


class PSXSound;


PSXSoundGroup::~PSXSoundGroup()
{
    ReleaseGroupId(m_groupID);
    ReleaseAllSounds();
}


// ---- src/PSXSoundGroup/Create.cpp

int GetGroupId();

GameObject *PSXSoundGroup::Create()
{
    m_groupID = GetGroupId();
    m_38 = 100;
    if (m_groupID == -1)
        return 0;
    return this;
}


// ---- src/PSXSoundGroup/AddSound.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off



int PSXSoundGroup::AddSound(PSXSound *sound)
{
#line 1552 "PSXAudio.cpp"
    ASSERT(sound, "AddSound: Sound==NULL");
    return m_sounds.AddItem(sound);
}
#pragma alignlabel reset


// ---- src/PSXSoundGroup/RemoveSound.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off




void PSXSoundGroup::RemoveSound(PSXSound *sound)
{
#line 1558 "PSXAudio.cpp"
    ASSERT(sound, "RemoveSound: Sound==NULL");
    sound->Stop(0);
    sound->Release();
    m_sounds.RemoveItem(sound);
}
#pragma alignlabel reset


// ---- src/PSXSoundGroup/ReleaseAllSounds.cpp



void PSXSoundGroup::ReleaseAllSounds()
{
    if (m_sounds.m_numItems > 0) {
        // Iterate over a copy of the list.
        ContainerList<PSXSound *> sounds;
        sounds.Create(m_sounds.m_numItems, 16);
        int i;
        for (i = 0; i < m_sounds.m_numItems; i++)
            sounds.AddItem(m_sounds.GetItem(i));
        PSXSound *sound;
        for (int j = 0; j < sounds.m_numItems; j++) {
            sound = sounds.GetItem(j);
            sound->Stop(0);
            sound->Release();
        }
    }
    m_sounds.Clear();
}


// ---- src/PSXSoundGroup/StopAllSounds.cpp

int PSXSoundGroup::StopAllSounds()
{
    m_interface->m_snd.StopGroup(m_groupID);
    return 1;
}


// ---- src/PSXSoundGroup/DisplayStats.cpp

void PSXSoundGroup::DisplayStats() {}


// ---- src/PSXSoundGroup/SetVolume.cpp

void PSXSoundGroup::SetVolume(long volume)
{
    m_38 = volume;
    m_interface->m_snd.VolumeGroup(m_groupID, m_38);
}


// ---- src/PSXSoundGroup/Disable.cpp

void PSXSoundGroup::Disable()
{
    GameObject::Disable();
    Pause(1);
}


// ---- src/PSXSoundGroup/Enable.cpp

void PSXSoundGroup::Enable()
{
    GameObject::Enable();
    Pause(0);
}


// ---- src/PSXSoundGroup/AncestorsDisabled.cpp


extern Game *LpGame;



void PSXSoundGroup::AncestorsDisabled()
{
    GameObject::AncestorsDisabled();
    if (m_parent && (!m_parent->m_enabled || LpGame->m_paused))
        m_interface->m_snd.PauseGroup(m_groupID);
}


// ---- src/PSXSoundGroup/AncestorsEnabled.cpp


extern Game *LpGame;



void PSXSoundGroup::AncestorsEnabled()
{
    GameObject::AncestorsEnabled();
    if (m_parent && (m_parent->m_enabled || LpGame->m_paused))
        m_interface->m_snd.PauseGroup(m_groupID);
}


// ---- src/PSXSoundGroup/Pause.cpp

int PSXSoundGroup::Pause(int pause)
{
    GameObject::Pause(pause);
    m_interface->m_snd.PauseGroup(m_groupID);
    return 1;
}


// ---- src/PSXSoundGroup/Tick.cpp



int PSXSoundGroup::Tick(float dt)
{
    for (int i = 0; i < m_sounds.m_numItems; i++)
        m_sounds.GetItem(i)->Tick(dt);
    return GameObject::Tick(dt);
}



// ---- src/Listener/ctor.cpp




Listener::Listener()
{
    m_position = ZeroVector;
    m_front = ZeroVector;
    m_top = ZeroVector;
    m_velocity = ZeroVector;
}


// ---- src/Listener/dtor.cpp


Listener::~Listener()
{
}


// ---- src/Listener/CommitDeferredSettings.cpp

int Listener::CommitDeferredSettings()
{
    return 1;
}


// ---- src/Listener/SetDistanceFactor.cpp

int Listener::SetDistanceFactor(float factor, int deferred)
{
    return 1;
}


// ---- src/Listener/SetDopplerFactor.cpp

int Listener::SetDopplerFactor(float factor, int deferred)
{
    return 1;
}


// ---- src/Listener/SetRolloffFactor.cpp

int Listener::SetRolloffFactor(float factor, int deferred)
{
    return 1;
}


// ---- src/ContainerList_LT/AddItemUnique.cpp
// ASSERT contains inline asm; match retail's label alignment for asm functions.
#pragma alignlabel off



// Add item unless it is already in the list (returns 0 then).
template <class T>
int ContainerList<T>::AddItemUnique(T item)
{
#line 107 "ContainerList.h"
    ASSERT(m_created, "AddItemUnique: Item not created");
    ASSERT(item, "AddItemUnique: item==0");
    for (int i = 0; i < m_numItems; i++) {
        if (m_items[i] == item)
            return 0;
    }
    if (m_numItems >= m_maxItems && !Grow(m_numItems + m_growBy))
        return 0;
    m_items[m_numItems] = item;
    m_numItems++;
    return 1;
}
#pragma alignlabel reset


