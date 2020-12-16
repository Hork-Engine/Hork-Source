/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <World/Public/Audio/AudioSystem.h>
#include "AudioSystemLocal.h"

#include <Runtime/Public/Runtime.h>

#include <World/Public/Audio/AudioClip.h>
#include <World/Public/World.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Components/PhysicalBody.h>

#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>

//#define USE_MOJOAL

#ifndef USE_MOJOAL
#define AL_NO_PROTOTYPES
#else
#define AL_API //__declspec(dllexport)
#define ALC_API //__declspec(dllexport)
#define ALC_APIENTRY __cdecl
#endif

#include "AL/al.h"
#include "AL/alc.h"

//#ifdef USE_MOJOAL
//#undef AL_API
//#undef ALC_API
//#include "mojoal.h"
//#endif

#ifndef AN_NO_OPENAL
#define AL_SAFE( Instruction ) {Instruction;AL_CheckError( AN_STRINGIFY( Instruction ) );}
#define ALC_SAFE( Instruction ) {Instruction;ALC_CheckError( AN_STRINGIFY( Instruction ) );}
#else
#define AL_SAFE( Instruction )
#define ALC_SAFE( Instruction )
#endif

AN_CLASS_META( AAudioControlCallback )
AN_CLASS_META( AAudioGroup )

static void * LibOpenAL = NULL;
static ALCdevice * ALCDevice = NULL;
static ALCcontext * ALCContext = NULL;

#ifndef USE_MOJOAL
static LPALENABLE alEnable = NULL;
static LPALDISABLE alDisable = NULL;
static LPALISENABLED alIsEnabled = NULL;
static LPALGETSTRING alGetString = NULL;
static LPALGETBOOLEANV alGetBooleanv = NULL;
static LPALGETINTEGERV alGetIntegerv = NULL;
static LPALGETFLOATV alGetFloatv = NULL;
static LPALGETDOUBLEV alGetDoublev = NULL;
static LPALGETBOOLEAN alGetBoolean = NULL;
static LPALGETINTEGER alGetInteger = NULL;
static LPALGETFLOAT alGetFloat = NULL;
static LPALGETDOUBLE alGetDouble = NULL;
static LPALGETERROR alGetError = NULL;
static LPALISEXTENSIONPRESENT alIsExtensionPresent = NULL;
static LPALGETPROCADDRESS alGetProcAddress = NULL;
static LPALGETENUMVALUE alGetEnumValue = NULL;
static LPALLISTENERF alListenerf = NULL;
static LPALLISTENER3F alListener3f = NULL;
static LPALLISTENERFV alListenerfv = NULL;
static LPALLISTENERI alListeneri = NULL;
static LPALLISTENER3I alListener3i = NULL;
static LPALLISTENERIV alListeneriv = NULL;
static LPALGETLISTENERF alGetListenerf = NULL;
static LPALGETLISTENER3F alGetListener3f = NULL;
static LPALGETLISTENERFV alGetListenerfv = NULL;
static LPALGETLISTENERI alGetListeneri = NULL;
static LPALGETLISTENER3I alGetListener3i = NULL;
static LPALGETLISTENERIV alGetListeneriv = NULL;
static LPALGENSOURCES alGenSources = NULL;
static LPALDELETESOURCES alDeleteSources = NULL;
static LPALISSOURCE alIsSource = NULL;
static LPALSOURCEF alSourcef = NULL;
static LPALSOURCE3F alSource3f = NULL;
static LPALSOURCEFV alSourcefv = NULL;
static LPALSOURCEI alSourcei = NULL;
static LPALSOURCE3I alSource3i = NULL;
static LPALSOURCEIV alSourceiv = NULL;
static LPALGETSOURCEF alGetSourcef = NULL;
static LPALGETSOURCE3F alGetSource3f = NULL;
static LPALGETSOURCEFV alGetSourcefv = NULL;
static LPALGETSOURCEI alGetSourcei = NULL;
static LPALGETSOURCE3I alGetSource3i = NULL;
static LPALGETSOURCEIV alGetSourceiv = NULL;
static LPALSOURCEPLAYV alSourcePlayv = NULL;
static LPALSOURCESTOPV alSourceStopv = NULL;
static LPALSOURCEREWINDV alSourceRewindv = NULL;
static LPALSOURCEPAUSEV alSourcePausev = NULL;
static LPALSOURCEPLAY alSourcePlay = NULL;
static LPALSOURCESTOP alSourceStop = NULL;
static LPALSOURCEREWIND alSourceRewind = NULL;
static LPALSOURCEPAUSE alSourcePause = NULL;
static LPALSOURCEQUEUEBUFFERS alSourceQueueBuffers = NULL;
static LPALSOURCEUNQUEUEBUFFERS alSourceUnqueueBuffers = NULL;
static LPALGENBUFFERS alGenBuffers = NULL;
static LPALDELETEBUFFERS alDeleteBuffers = NULL;
static LPALISBUFFER alIsBuffer = NULL;
static LPALBUFFERDATA alBufferData = NULL;
static LPALBUFFERF alBufferf = NULL;
static LPALBUFFER3F alBuffer3f = NULL;
static LPALBUFFERFV alBufferfv = NULL;
static LPALBUFFERI alBufferi = NULL;
static LPALBUFFER3I alBuffer3i = NULL;
static LPALBUFFERIV alBufferiv = NULL;
static LPALGETBUFFERF alGetBufferf = NULL;
static LPALGETBUFFER3F alGetBuffer3f = NULL;
static LPALGETBUFFERFV alGetBufferfv = NULL;
static LPALGETBUFFERI alGetBufferi = NULL;
static LPALGETBUFFER3I alGetBuffer3i = NULL;
static LPALGETBUFFERIV alGetBufferiv = NULL;
static LPALDOPPLERFACTOR alDopplerFactor = NULL;
static LPALDOPPLERVELOCITY alDopplerVelocity = NULL;
static LPALSPEEDOFSOUND alSpeedOfSound = NULL;
static LPALDISTANCEMODEL alDistanceModel = NULL;
#endif

static LPALCCREATECONTEXT malcCreateContext = NULL;
static LPALCMAKECONTEXTCURRENT malcMakeContextCurrent = NULL;
static LPALCPROCESSCONTEXT malcProcessContext = NULL;
static LPALCSUSPENDCONTEXT malcSuspendContext = NULL;
static LPALCDESTROYCONTEXT malcDestroyContext = NULL;
//static LPALCGETCURRENTCONTEXT malcGetCurrentContext = NULL;
//static LPALCGETCONTEXTSDEVICE malcGetContextsDevice = NULL;
static LPALCOPENDEVICE malcOpenDevice = NULL;
static LPALCCLOSEDEVICE malcCloseDevice = NULL;
static LPALCGETERROR malcGetError = NULL;
static LPALCISEXTENSIONPRESENT malcIsExtensionPresent = NULL;
//static LPALCGETPROCADDRESS malcGetProcAddress = NULL;
//static LPALCGETENUMVALUE malcGetEnumValue = NULL;
static LPALCGETSTRING malcGetString = NULL;
static LPALCGETINTEGERV malcGetIntegerv = NULL;
static LPALCCAPTUREOPENDEVICE malcCaptureOpenDevice = NULL;
static LPALCCAPTURECLOSEDEVICE malcCaptureCloseDevice = NULL;
static LPALCCAPTURESTART malcCaptureStart = NULL;
static LPALCCAPTURESTOP malcCaptureStop = NULL;
static LPALCCAPTURESAMPLES malcCaptureSamples = NULL;

#ifndef ALC_SOFT_HRTF
#define ALC_SOFT_HRTF 1
#define ALC_HRTF_SOFT                            0x1992
#define ALC_DONT_CARE_SOFT                       0x0002
#define ALC_HRTF_STATUS_SOFT                     0x1993
#define ALC_HRTF_DISABLED_SOFT                   0x0000
#define ALC_HRTF_ENABLED_SOFT                    0x0001
#define ALC_HRTF_DENIED_SOFT                     0x0002
#define ALC_HRTF_REQUIRED_SOFT                   0x0003
#define ALC_HRTF_HEADPHONES_DETECTED_SOFT        0x0004
#define ALC_HRTF_UNSUPPORTED_FORMAT_SOFT         0x0005
#define ALC_NUM_HRTF_SPECIFIERS_SOFT             0x1994
#define ALC_HRTF_SPECIFIER_SOFT                  0x1995
#define ALC_HRTF_ID_SOFT                         0x1996
typedef const ALCchar* ( ALC_APIENTRY*LPALCGETSTRINGISOFT )( ALCdevice *device, ALCenum paramName, ALCsizei index );
typedef ALCboolean ( ALC_APIENTRY*LPALCRESETDEVICESOFT )( ALCdevice *device, const ALCint *attribs );
#ifdef AL_ALEXT_PROTOTYPES
ALC_API const ALCchar* ALC_APIENTRY alcGetStringiSOFT( ALCdevice *device, ALCenum paramName, ALCsizei index );
ALC_API ALCboolean ALC_APIENTRY alcResetDeviceSOFT( ALCdevice *device, const ALCint *attribs );
#endif
#endif

#ifndef AL_SOFT_source_spatialize
#define AL_SOFT_source_spatialize
#define AL_SOURCE_SPATIALIZE_SOFT                0x1214
#define AL_AUTO_SOFT                             0x0002
#endif

static LPALCGETSTRINGISOFT malcGetStringiSOFT = NULL;
static LPALCRESETDEVICESOFT malcResetDeviceSOFT = NULL;

static float MasterVolume = 1.0f;
static Float3 ListenerPosition(0);
static bool bSourceSpatialize = false;
static int NumHRTFs = 0;
static uint64_t AudioListenerId = 0;

static void InitializeChannels();

static void UnloadOpenAL() {
    GRuntime.UnloadDynamicLib( LibOpenAL );
}

template< class type > AN_FORCEINLINE bool AL_GetProcAddress( type ** _ProcPtr, const char * _ProcName ) {
    return ( NULL != ( ( *_ProcPtr ) = ( type * )alGetProcAddress( _ProcName ) ) );
}

static bool LoadOpenAL() {
    UnloadOpenAL();

#ifndef USE_MOJOAL
#ifdef AN_OS_LINUX
    LibOpenAL = GRuntime.LoadDynamicLib( "libopenal" );
#else
    LibOpenAL = GRuntime.LoadDynamicLib( "OpenAL32" );
#endif
    if( !LibOpenAL ) {
        GLogger.Printf( "Failed to load OpenAL library\n" );
        return false;
    }
#endif

    bool bError = false;

#ifdef USE_MOJOAL
    #define GET_PROC_ADDRESS( Proc )
#else
    #define GET_PROC_ADDRESS( Proc ) { \
        if ( !GRuntime.GetProcAddress( LibOpenAL, &Proc, AN_STRINGIFY( Proc ) ) ) { \
            GLogger.Printf( "Failed to load %s\n", AN_STRINGIFY(Proc) ); \
            bError = true; \
        } \
    }
#endif

    GET_PROC_ADDRESS( alEnable );
    GET_PROC_ADDRESS( alDisable );
    GET_PROC_ADDRESS( alIsEnabled );
    GET_PROC_ADDRESS( alGetString );
    GET_PROC_ADDRESS( alGetBooleanv );
    GET_PROC_ADDRESS( alGetIntegerv );
    GET_PROC_ADDRESS( alGetFloatv );
    GET_PROC_ADDRESS( alGetDoublev );
    GET_PROC_ADDRESS( alGetBoolean );
    GET_PROC_ADDRESS( alGetInteger );
    GET_PROC_ADDRESS( alGetFloat );
    GET_PROC_ADDRESS( alGetDouble );
    GET_PROC_ADDRESS( alGetError );
    GET_PROC_ADDRESS( alIsExtensionPresent );
    GET_PROC_ADDRESS( alGetProcAddress );
    GET_PROC_ADDRESS( alGetEnumValue );
    GET_PROC_ADDRESS( alListenerf );
    GET_PROC_ADDRESS( alListener3f );
    GET_PROC_ADDRESS( alListenerfv );
    GET_PROC_ADDRESS( alListeneri );
    GET_PROC_ADDRESS( alListener3i );
    GET_PROC_ADDRESS( alListeneriv );
    GET_PROC_ADDRESS( alGetListenerf );
    GET_PROC_ADDRESS( alGetListener3f );
    GET_PROC_ADDRESS( alGetListenerfv );
    GET_PROC_ADDRESS( alGetListeneri );
    GET_PROC_ADDRESS( alGetListener3i );
    GET_PROC_ADDRESS( alGetListeneriv );
    GET_PROC_ADDRESS( alGenSources );
    GET_PROC_ADDRESS( alDeleteSources );
    GET_PROC_ADDRESS( alIsSource );
    GET_PROC_ADDRESS( alSourcef );
    GET_PROC_ADDRESS( alSource3f );
    GET_PROC_ADDRESS( alSourcefv );
    GET_PROC_ADDRESS( alSourcei );
    GET_PROC_ADDRESS( alSource3i );
    GET_PROC_ADDRESS( alSourceiv );
    GET_PROC_ADDRESS( alGetSourcef );
    GET_PROC_ADDRESS( alGetSource3f );
    GET_PROC_ADDRESS( alGetSourcefv );
    GET_PROC_ADDRESS( alGetSourcei );
    GET_PROC_ADDRESS( alGetSource3i );
    GET_PROC_ADDRESS( alGetSourceiv );
    GET_PROC_ADDRESS( alSourcePlayv );
    GET_PROC_ADDRESS( alSourceStopv );
    GET_PROC_ADDRESS( alSourceRewindv );
    GET_PROC_ADDRESS( alSourcePausev );
    GET_PROC_ADDRESS( alSourcePlay );
    GET_PROC_ADDRESS( alSourceStop );
    GET_PROC_ADDRESS( alSourceRewind );
    GET_PROC_ADDRESS( alSourcePause );
    GET_PROC_ADDRESS( alSourceQueueBuffers );
    GET_PROC_ADDRESS( alSourceUnqueueBuffers );
    GET_PROC_ADDRESS( alGenBuffers );
    GET_PROC_ADDRESS( alDeleteBuffers );
    GET_PROC_ADDRESS( alIsBuffer );
    GET_PROC_ADDRESS( alBufferData );
    GET_PROC_ADDRESS( alBufferf );
    GET_PROC_ADDRESS( alBuffer3f );
    GET_PROC_ADDRESS( alBufferfv );
    GET_PROC_ADDRESS( alBufferi );
    GET_PROC_ADDRESS( alBuffer3i );
    GET_PROC_ADDRESS( alBufferiv );
    GET_PROC_ADDRESS( alGetBufferf );
    GET_PROC_ADDRESS( alGetBuffer3f );
    GET_PROC_ADDRESS( alGetBufferfv );
    GET_PROC_ADDRESS( alGetBufferi );
    GET_PROC_ADDRESS( alGetBuffer3i );
    GET_PROC_ADDRESS( alGetBufferiv );
    GET_PROC_ADDRESS( alDopplerFactor );
    GET_PROC_ADDRESS( alDopplerVelocity );
    GET_PROC_ADDRESS( alSpeedOfSound );
    GET_PROC_ADDRESS( alDistanceModel );

    #undef GET_PROC_ADDRESS

#ifdef USE_MOJOAL
#define GET_PROC_ADDRESS( Proc ) { \
        m##Proc = Proc; \
    }
#else
    #define GET_PROC_ADDRESS( Proc ) { \
        if ( !GRuntime.GetProcAddress( LibOpenAL, &m##Proc, AN_STRINGIFY( Proc ) ) ) { \
            GLogger.Printf( "Failed to load %s\n", AN_STRINGIFY( Proc ) ); \
            bError = true; \
        } \
    }
#endif

    GET_PROC_ADDRESS( alcCreateContext );
    GET_PROC_ADDRESS( alcMakeContextCurrent );
    GET_PROC_ADDRESS( alcProcessContext );
    GET_PROC_ADDRESS( alcSuspendContext );
    GET_PROC_ADDRESS( alcDestroyContext );
    //GET_PROC_ADDRESS( alcGetCurrentContext );
    //GET_PROC_ADDRESS( alcGetContextsDevice );
    GET_PROC_ADDRESS( alcOpenDevice );
    GET_PROC_ADDRESS( alcCloseDevice );
    GET_PROC_ADDRESS( alcGetError );
    GET_PROC_ADDRESS( alcIsExtensionPresent );
    //GET_PROC_ADDRESS( alcGetProcAddress );
    //GET_PROC_ADDRESS( alcGetEnumValue );
    GET_PROC_ADDRESS( alcGetString );
#ifndef USE_MOJOAL
    GET_PROC_ADDRESS( alcGetIntegerv );
#endif
    GET_PROC_ADDRESS( alcCaptureOpenDevice );
    GET_PROC_ADDRESS( alcCaptureCloseDevice );
    GET_PROC_ADDRESS( alcCaptureStart );
    GET_PROC_ADDRESS( alcCaptureStop );
    GET_PROC_ADDRESS( alcCaptureSamples );
#ifndef USE_MOJOAL
    GET_PROC_ADDRESS( alcGetStringiSOFT );
    GET_PROC_ADDRESS( alcResetDeviceSOFT );
#endif

    if ( bError ) {
        UnloadOpenAL();
        return false;
    }

    return true;
}

void AL_CheckError( const char * _Text ) {
#ifndef AN_NO_OPENAL
    int32_t Error = alGetError();
    if ( Error != AL_NO_ERROR ) {
        GLogger.Printf( "AL ERROR: %s %s\n", _Text, Math::ToHexString( Error ).CStr() );
    }
#endif
}

static void ALC_CheckError( const char * _Text ) {
#ifndef AN_NO_OPENAL
    int32_t Error = malcGetError( ALCDevice );
    if ( Error != ALC_NO_ERROR ) {
        GLogger.Printf( "ALC ERROR: %s %s\n", _Text, Math::ToHexString( Error ).CStr() );
    }
#endif
}

AAudioSystem & GAudioSystem = AAudioSystem::Inst();

AAudioSystem::AAudioSystem() {
    //bEAX = false;
    ALCDevice = NULL;
    ALCContext = NULL;
}

AAudioSystem::~AAudioSystem() {
    AN_ASSERT( bInitialized == false );
}

void AAudioSystem::Initialize() {
    GLogger.Printf( "Initializing audio system...\n" );

    if ( !LoadOpenAL() ) {
        CriticalError( "Failed to load OpenAL library\n" );
    }

    ALC_SAFE( ALCDevice = malcOpenDevice( NULL ) );

    if ( !ALCDevice ) {
        CriticalError( "AAudioSystem::Initialize: Failed to open device\n" );
    }

    ALC_SAFE( ALCContext = malcCreateContext( ALCDevice, NULL ) );

    if ( !ALCContext ) {
#ifndef AN_NO_OPENAL
        malcCloseDevice( ALCDevice );
#endif
        ALCDevice = NULL;

        CriticalError( "AAudioSystem::Initialize: Failed to create context\n" );
    }

    //ALCint freq,r,s;
    //malcGetIntegerv( ALCDevice, ALC_FREQUENCY, 1, &freq );
    //malcGetIntegerv( ALCDevice, ALC_REFRESH, 1, &r );
    //malcGetIntegerv( ALCDevice, ALC_SYNC, 1, &s );

    //GLogger.Printf( "Freq %d refr %d sync %d\n", freq,r,s);


    ALCboolean Result = ALC_FALSE;
    ALC_SAFE( Result = malcMakeContextCurrent( ALCContext ) );

    if ( !Result ) {
        ALC_SAFE( malcDestroyContext( ALCContext ) );
#ifndef AN_NO_OPENAL
        malcCloseDevice( ALCDevice );
#endif

        ALCDevice = NULL;
        ALCContext = NULL;

        CriticalError( "AAudioSystem::Initialize: Failed to make current context\n" );
    }

    // Check for EAX 2.0 support
//    AL_SAFE( bEAX = !!alIsExtensionPresent("EAX2.0") );

//    if ( bEAX ) {
//        GLogger.Printf( "EAX supported\n" );
//    } else {
//        GLogger.Printf( "EAX not supported\n" );
//    }

#if 1
    const ALCchar * pDevices = NULL;

    ALC_SAFE( pDevices = malcGetString( NULL, ALC_DEVICE_SPECIFIER ) );

    const ALCchar *deviceName = pDevices, *next = pDevices + 1;
    size_t len = 0;
    GLogger.Printf( "Devices list:" );
    while ( deviceName && *deviceName != '\0' && next && *next != '\0' ) {
        GLogger.Printf( " '%s'", deviceName );
        len = strlen( deviceName );
        deviceName += ( len + 1 );
        next += ( len + 2 );
    }
    GLogger.Printf(  "\n" );
#endif

#if 1
    const ALchar * pVendor = "", *pVersion = "", *pRenderer = "", *pExtensions = "";
    AL_SAFE( pVendor = alGetString( AL_VENDOR ) );
    AL_SAFE( pVersion = alGetString( AL_VERSION ) );
    AL_SAFE( pRenderer = alGetString( AL_RENDERER ) );
    AL_SAFE( pExtensions = alGetString( AL_EXTENSIONS ) );
    GLogger.Printf( "Audio vendor: %s/%s (version %s)\n", pVendor, pRenderer, pVersion );
    AString tmp = pExtensions;
    char * s = tmp.ToPtr();
    char * word = tmp.ToPtr();
    while ( *s ) {
        if ( *s++ == ' ' ) {
            *(s - 1) = 0;
            GLogger.Printf( "\t%s\n", word );
            word = s;
            continue;
        }
    }
    if ( *word ) {
        GLogger.Printf( "\t%s\n", word );
    }

    //ALCint MaxMonoSources = 0;
    //ALCint MaxStereoSources = 0;
    //ALC_SAFE( malcGetIntegerv( ALCDevice, ALC_MONO_SOURCES, 1, &MaxMonoSources ) );
    //ALC_SAFE( malcGetIntegerv( ALCDevice, ALC_STEREO_SOURCES, 1, &MaxStereoSources ) );
    //GLogger.Printf( "ALC_MONO_SOURCES: %d\n", MaxMonoSources );
    //GLogger.Printf( "ALC_STEREO_SOURCES: %d\n", MaxStereoSources );
#endif

    NumHRTFs = 0;

    if ( malcIsExtensionPresent( ALCDevice, "ALC_SOFT_HRTF" ) ) {
        GLogger.Printf( "HRTF supported\n" );

        // Print available HRTFs
        ALC_SAFE( malcGetIntegerv( ALCDevice, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &NumHRTFs ) );
        if ( NumHRTFs > 0 ) {
            GLogger.Printf( "Available HRTFs:\n" );
            for ( int i = 0; i < NumHRTFs; i++ ) {
                const ALCchar *name;
                ALC_SAFE( name = malcGetStringiSOFT( ALCDevice, ALC_HRTF_SPECIFIER_SOFT, i ) );
                if ( name && *name ) {
                    GLogger.Printf( "    %d: %s\n", i, name );
                }
            }
        } else {
            GLogger.Printf( "No HRTFs found\n" );
        }

    } else {
        GLogger.Printf( "HRTF not supported\n" );
    }

    //EnableDefaultHRTF();

    if ( !alIsExtensionPresent( "AL_EXT_STEREO_ANGLES" ) ) {
        GLogger.Printf( "Rotated stereo not supported\n" );
    } else {
        GLogger.Printf( "Rotated stereo supported\n" );
    }

    bSourceSpatialize = !!alIsExtensionPresent( "AL_SOFT_source_spatialize" );
    if ( !bSourceSpatialize ) {
        GLogger.Printf( "Source spatialize not supported\n" );
    } else {
        GLogger.Printf( "Source spatialize supported\n" );
    }

    AL_SAFE( alListenerf( AL_GAIN, 1.0f ) );

    InitializeChannels();

    bInitialized = true;
}

void AAudioSystem::Deinitialize() {
    GLogger.Printf( "Deinitializing audio system...\n" );

    RemoveAudioDecoders();

    ALC_SAFE( malcMakeContextCurrent( NULL ) );
    ALC_SAFE( malcDestroyContext( ALCContext ) );
#ifndef AN_NO_OPENAL
    malcCloseDevice( ALCDevice );
#endif

    ALCDevice = NULL;
    ALCContext = NULL;

    UnloadOpenAL();

    bInitialized = false;
}

static void CheckHRTFState() {
    ALCint hrtfState;

    ALC_SAFE( malcGetIntegerv( ALCDevice, ALC_HRTF_SOFT, 1, &hrtfState ) );

    if ( !hrtfState ) {
        GLogger.Printf( "HRTF not enabled\n" );
        return;
    }

    const ALchar *name;
    ALC_SAFE( name = malcGetString( ALCDevice, ALC_HRTF_SPECIFIER_SOFT ) );

    if ( name && *name ) {
        GLogger.Printf( "HRTF enabled, using %s\n", name );
    }
}

void AAudioSystem::EnableHRTF( int _Index ) {
    if ( _Index < 0 || _Index >= NumHRTFs ) {
        return;
    }

    if ( !malcResetDeviceSOFT ) {
        return;
    }

    GLogger.Printf( "Selecting HRTF %d...\n", _Index );

    const ALCint attr[] = { ALC_HRTF_SOFT, ALC_TRUE,
                            ALC_HRTF_ID_SOFT, _Index,
                            0 };

    ALCboolean result;
    ALC_SAFE( result = malcResetDeviceSOFT( ALCDevice, attr ) );
    if ( !result ) {
        GLogger.Printf( "Failed to reset device: %s\n", malcGetString( ALCDevice, malcGetError( ALCDevice ) ) );
    }

    CheckHRTFState();
}

void AAudioSystem::EnableDefaultHRTF() {
    if ( !NumHRTFs ) {
        return;
    }

    if ( !malcResetDeviceSOFT ) {
        return;
    }

    GLogger.Printf( "Using default HRTF...\n" );

    constexpr ALCint attr[] = { ALC_HRTF_SOFT, ALC_TRUE, 0 };

    ALCboolean result;
    ALC_SAFE( result = malcResetDeviceSOFT( ALCDevice, attr ) );
    if ( !result ) {
        GLogger.Printf( "Failed to reset device: %s\n", malcGetString( ALCDevice, malcGetError( ALCDevice ) ) );
    }

    CheckHRTFState();
}

void AAudioSystem::DisableHRTF() {
    if ( !NumHRTFs ) {
        return;
    }

    if ( !malcResetDeviceSOFT ) {
        return;
    }

    GLogger.Printf( "Disabling HRTF...\n" );

    constexpr ALCint attr[] = { ALC_HRTF_SOFT, ALC_FALSE, 0 };

    ALCboolean result;
    ALC_SAFE( result = malcResetDeviceSOFT( ALCDevice, attr ) );
    if ( !result ) {
        GLogger.Printf( "Failed to reset device: %s\n", malcGetString( ALCDevice, malcGetError( ALCDevice ) ) );
    }

    CheckHRTFState();
}

int AAudioSystem::GetNumHRTFs() const {
    return NumHRTFs;
}

const char * AAudioSystem::GetHRTF( int _Index ) const {
    if ( !malcGetStringiSOFT ) {
        return AString::NullCString();
    }

    if ( _Index >= 0 && _Index < NumHRTFs ) {
        const ALCchar * name;
        ALC_SAFE( name = malcGetStringiSOFT( ALCDevice, ALC_HRTF_SPECIFIER_SOFT, _Index ) );
        return name ? name : AString::NullCString();
    }
    return AString::NullCString();
}

void AAudioSystem::AddAudioDecoder( const char * _Extension, IAudioDecoderInterface * _Interface ) {
    for ( SAudioDecoderDef & def : Decoders ) {
        if ( !Core::Stricmp( _Extension, def.Extension ) ) {
            def.Interface->RemoveRef();
            def.Interface = _Interface;
            _Interface->AddRef();
            return;
        }
    }
    SAudioDecoderDef def;
    Core::Strcpy( def.Extension, sizeof( def.Extension ), _Extension );
    def.Interface = _Interface;
    _Interface->AddRef();
    Decoders.Append( def );
}

void AAudioSystem::RemoveAudioDecoder( const char * _Extension ) {
    for ( int i = 0 ; i < Decoders.Size() ; i++ ) {
        SAudioDecoderDef & def =  Decoders[i];
        if ( !Core::Stricmp( _Extension, def.Extension ) ) {
            def.Interface->RemoveRef();
            Decoders.Remove( i );
            return;
        }
    }
}

void AAudioSystem::RemoveAudioDecoders() {
    for ( int i = 0; i < Decoders.Size(); i++ ) {
        Decoders[ i ].Interface->RemoveRef();
    }
    Decoders.Free();
}

IAudioDecoderInterface * AAudioSystem::FindAudioDecoder( const char * _FileName ) {
    int i = Core::FindExtWithoutDot( _FileName );
    for ( SAudioDecoderDef & def : Decoders ) {
        if ( !Core::Stricmp( _FileName + i, def.Extension ) ) {
            return def.Interface;
        }
    }
    return nullptr;
}

// How to use:
//AAudioClip * S_Player_Pain1 = GetOrCreateResource< AAudioClip >( "Sounds/Player/Pain1.ogg" );
//GAudioSystem.PlaySound( S_Player_Pain1, this );
//GAudioSystem.PlaySoundAt( S_Player_Pain1, Position, this );

static SSoundSpawnParameters DefaultSpawnParameters;

#define MAX_AUDIO_CHANNELS 64

struct SAudioChannel {
    ALuint      SourceId;
    int         ChannelIndex;
    int         Priority;   // EAudioChannelPriority
    EAudioSourceType SourceType;
    uint64_t    AudioClient;
    int64_t     PlayTimeStamp;
    AWorld *    World;
    IAudioStreamInterface * StreamInterface; // Audio streaming interface
    AAudioControlCallback * ControlCallback;
    AAudioGroup * Group;
    ASceneComponent * Instigator;
    APhysicalBody * PhysicalBody;
    AAudioClip* Clip;
    int         ClipSerialId;
    int         NumStreamBuffers;
    unsigned int StreamBuffers[2];
    int         PlaybackPosition; // Playback position in samples for streamed audio
    Float3      SpawnPosition;
    Float3      SoundPosition; // current position
    Float3      PrevSoundPosition; // position on previous frame
    Float3      Velocity;
    Float3      Direction;
    float       Pitch;
    float       Volume;
    float       CurVolume;
    float       ReferenceDistance;
    float       MaxDistance;
    float       RolloffFactor;
    float       ConeInnerAngle;
    float       ConeOuterAngle;
    float       LifeSpan;
    float       VirtualTime;
    bool        bLooping : 1;
    bool        bStopWhenInstigatorDead : 1;
    bool        bStreamed : 1;
    bool        bPlayEvenWhenPaused : 1;
    bool        bFree : 1;
    bool        bPausedByGame : 1;
    bool        bLocked : 1;
    bool        bVirtualizeWhenSilent : 1;
    bool        bIsVirtual : 1;
    bool        bUseVelocity : 1;
    bool        bUsePhysicalVelocity : 1;
    bool        bDirectional : 1;
};

static SAudioChannel AudioChannels[ MAX_AUDIO_CHANNELS ];
static int NumAudioChannels = 0;

static SAudioChannel * FreeAudioChannels[ MAX_AUDIO_CHANNELS ];
static int NumFreeAudioChannels = 0;

static TPodArray< SAudioChannel > VirtualChannels;

static short PCM[AUDIO_MAX_PCM_BUFFER_SIZE];

static SAudioChannel * AllocateChannel( int _Priority );
static void FreeChannel( SAudioChannel * _Channel );
static void PlayChannel( SAudioChannel * channel, float _PlayOffset );
static float CalcAudioVolume( SAudioChannel * _Channel );

static void InitializeChannels() {
    Core::ZeroMem( AudioChannels, sizeof( AudioChannels ) );
}

void AAudioSystem::PurgeChannels() {
    GLogger.Printf( "Purging audio channels\n" );

    for ( int i = 0; i < NumAudioChannels; i++ ) {
        SAudioChannel * channel = &AudioChannels[ i ];

        FreeChannel( channel );

        AL_SAFE( alDeleteSources( 1, &channel->SourceId ) );

        // Remove data used for streaming
        if ( channel->StreamBuffers[0] ) {
            AL_SAFE( alDeleteBuffers( 2, channel->StreamBuffers ) );
            Core::ZeroMem( channel->StreamBuffers, sizeof( channel->StreamBuffers ) );
        }
    }

    NumAudioChannels = 0;

    for ( SAudioChannel & channel : VirtualChannels ) {
        FreeChannel( &channel );
    }

    VirtualChannels.Free();
}

static void FreeChannel( SAudioChannel * channel ) {

    if ( channel->bFree ) {
        return;
    }

    AN_ASSERT( NumFreeAudioChannels < MAX_AUDIO_CHANNELS );

    if ( !channel->bIsVirtual ) {

        AL_SAFE( alSourceStop( channel->SourceId ) );

        //AL_SAFE( alSourceRewind( channel->SourceId ) );
        //AL_SAFE( alSourceUnqueueBuffers( channel->SourceId, channel->NumStreamBuffers, channel->StreamBuffers ) );

        AL_SAFE( alSourcei( channel->SourceId, AL_BUFFER, 0 ) );

        FreeAudioChannels[ NumFreeAudioChannels++ ] = channel;
    }

    channel->bFree = true;

    if ( channel->Clip ) {
        channel->Clip->RemoveRef();
        channel->Clip = nullptr;
    }

    channel->ClipSerialId = -1;

    if ( channel->ControlCallback ) {
        channel->ControlCallback->RemoveRef();
        channel->ControlCallback = nullptr;
    }

    channel->AudioClient = 0;

    if ( channel->Group ) {
        channel->Group->RemoveRef();
        channel->Group = nullptr;
    }

    if ( channel->StreamInterface ) {
        channel->StreamInterface->RemoveRef();
        channel->StreamInterface = nullptr;
    }

    if ( channel->Instigator ) {
        channel->Instigator->RemoveRef();
        channel->Instigator = nullptr;
    }

    if ( channel->World ) {
        channel->World->RemoveRef();
        channel->World = nullptr;
    }
}

static void VirtualizeChannel( SAudioChannel * _Channel ) {
    if ( _Channel->bFree ) {
        return;
    }

    //GLogger.Printf( "Virtualize channel\n" );

    AN_ASSERT( !_Channel->bIsVirtual );

    VirtualChannels.Append( *_Channel );

    SAudioChannel * virtualChannel = &VirtualChannels.Last();

    virtualChannel->SourceId = 0;
    virtualChannel->ChannelIndex = VirtualChannels.Size() - 1;
    virtualChannel->NumStreamBuffers = 0;
    virtualChannel->bIsVirtual = true;

    if ( _Channel->bStreamed ) {
        // FIXME: PlaybackPosition на самом деле находится немного в будущем, т.к.
        // инкрементируется сразу после отправки буфера на воспроизведение.
        // TODO: Нужно определить реальный playback position!
        virtualChannel->VirtualTime = (float)virtualChannel->PlaybackPosition / virtualChannel->Clip->GetFrequency();
    } else {
        AL_SAFE( alGetSourcef( _Channel->SourceId, AL_SEC_OFFSET, &virtualChannel->VirtualTime ) );
    }

    AL_SAFE( alSourceStop( _Channel->SourceId ) );
    AN_ASSERT( NumFreeAudioChannels < MAX_AUDIO_CHANNELS );
    AL_SAFE( alSourcei( _Channel->SourceId, AL_BUFFER, 0 ) );
    _Channel->bFree = true;
    FreeAudioChannels[ NumFreeAudioChannels++ ] = _Channel;
    _Channel->ClipSerialId = -1;
}

static bool DevirtualizeChannel( SAudioChannel * _VirtualChannel ) {
    AN_ASSERT( _VirtualChannel->bIsVirtual );

    //GLogger.Printf( "Devirtualize channel\n" );

    SAudioChannel * channel = AllocateChannel( _VirtualChannel->Priority );
    if ( !channel ) {
        return false;
    }

    // Turn off virtual status
    channel->bIsVirtual = false;

    // Update time stamp
    channel->PlayTimeStamp = GRuntime.SysFrameTimeStamp();

    // Copy parameters
    channel->SpawnPosition = _VirtualChannel->SpawnPosition;
    channel->Pitch = _VirtualChannel->Pitch;
    channel->Volume = _VirtualChannel->Volume;
    channel->ReferenceDistance = _VirtualChannel->ReferenceDistance;
    channel->MaxDistance = _VirtualChannel->MaxDistance;
    channel->RolloffFactor = _VirtualChannel->RolloffFactor;
    channel->bLooping = _VirtualChannel->bLooping;
    channel->bStopWhenInstigatorDead = _VirtualChannel->bStopWhenInstigatorDead;
    channel->SourceType = _VirtualChannel->SourceType;
    channel->bStreamed = _VirtualChannel->bStreamed;
    channel->Clip = _VirtualChannel->Clip;
    channel->ClipSerialId = _VirtualChannel->ClipSerialId;
    channel->StreamInterface = _VirtualChannel->StreamInterface;
    channel->Priority = _VirtualChannel->Priority;
    channel->bPlayEvenWhenPaused = _VirtualChannel->bPlayEvenWhenPaused;
    channel->bDirectional = _VirtualChannel->bDirectional;
    channel->ConeInnerAngle = _VirtualChannel->ConeInnerAngle;
    channel->ConeOuterAngle = _VirtualChannel->ConeOuterAngle;
    channel->Direction = _VirtualChannel->Direction;
    channel->ControlCallback = _VirtualChannel->ControlCallback;
    channel->AudioClient = _VirtualChannel->AudioClient;
    channel->Group = _VirtualChannel->Group;
    channel->Instigator = _VirtualChannel->Instigator;
    channel->PhysicalBody = _VirtualChannel->PhysicalBody;
    channel->World = _VirtualChannel->World;
    channel->bPausedByGame = _VirtualChannel->bPausedByGame;
    channel->LifeSpan = _VirtualChannel->LifeSpan;
    channel->SoundPosition = _VirtualChannel->SoundPosition;
    channel->PrevSoundPosition = _VirtualChannel->PrevSoundPosition;
    channel->Velocity = _VirtualChannel->Velocity;
    channel->bUseVelocity = _VirtualChannel->bUseVelocity;
    channel->bUsePhysicalVelocity = _VirtualChannel->bUsePhysicalVelocity;
    channel->CurVolume = CalcAudioVolume( channel );
    channel->bVirtualizeWhenSilent = _VirtualChannel->bVirtualizeWhenSilent;
    channel->bLocked = _VirtualChannel->bLocked;

    PlayChannel( channel, _VirtualChannel->VirtualTime );

    int i = _VirtualChannel->ChannelIndex;
    VirtualChannels.RemoveSwap( i );
    if ( i < VirtualChannels.Size() ) {
        VirtualChannels[ i ].ChannelIndex = i;
    }

    return true;
}

static void FreeOrVirtualizeChannel( SAudioChannel * _Channel ) {
    if ( _Channel->bVirtualizeWhenSilent ) {
        VirtualizeChannel( _Channel );
    } else {
        FreeChannel( _Channel );
    }
}

static int FindCandidateToUse( int _Priority ) {

    int candidate = -1;
    float minVolume = 99999;
    int minPriority = 99999;
    float minTimeStamp = GRuntime.SysFrameTimeStamp();
    bool paused = false;

    for ( int i = 0 ; i < NumAudioChannels ; i++ ) {
        SAudioChannel * channel = &AudioChannels[ i ];

        if ( channel->bLocked ) {
            // don't touch locked channels
            continue;
        }

        if ( channel->bFree ) {
            // free channel is best candidate to use
            return i;
        }

        if ( channel->bPausedByGame ) {

            paused = true;

            if ( candidate < 0 || channel->Priority < minPriority || channel->PlayTimeStamp < minTimeStamp ) {

                minPriority = channel->Priority;
                minTimeStamp = channel->PlayTimeStamp;
                candidate = i;
            }

        } else {

            if ( !paused && channel->Priority < _Priority
                 && ( channel->Priority < minPriority
                      || ( !channel->bLooping && (channel->CurVolume < minVolume || channel->PlayTimeStamp < minTimeStamp ) ) ) ) {

                minPriority = channel->Priority;
                minTimeStamp = channel->PlayTimeStamp;
                minVolume = channel->CurVolume;
                candidate = i;
            }

        }
    }

    return candidate;
}

static SAudioChannel * AllocateChannel( int _Priority ) {
    if ( NumFreeAudioChannels > 0 ) {
        SAudioChannel * channel = FreeAudioChannels[ --NumFreeAudioChannels ];
        channel->bFree = false;
        return channel;
    }

    if ( NumAudioChannels < MAX_AUDIO_CHANNELS ) {
        // Allocate new channel
        SAudioChannel * channel = &AudioChannels[ NumAudioChannels++ ];

        alGenSources( 1, &channel->SourceId );

        channel->ChannelIndex = NumAudioChannels - 1;

        channel->bFree = false;
        return channel;
    }

    int freeChannelIndex = FindCandidateToUse( _Priority );
    if ( freeChannelIndex < 0 ) {
        return nullptr;
    }

    FreeOrVirtualizeChannel( &AudioChannels[freeChannelIndex] );

    SAudioChannel * channel = FreeAudioChannels[ --NumFreeAudioChannels ];
    channel->bFree = false;

    return channel;
}

static bool StreamToBuffer( SAudioChannel * channel, int _BufferId ) {
    AAudioClip * Clip = channel->Clip;

    const int requiredBufferSize = Clip->GetBufferSize();

    AN_ASSERT( requiredBufferSize <= AUDIO_MAX_PCM_BUFFER_SIZE );

    int totalSamples = channel->StreamInterface->StreamDecodePCM( PCM, requiredBufferSize );
    if ( totalSamples == 0 ) {
        return false;
    }

    SAudioBufferUpload upload = {};
    upload.SamplesCount = totalSamples / Clip->GetChannels();
    upload.BitsPerSample = Clip->GetBitsPerSample();
    upload.Frequency = Clip->GetFrequency();
    upload.PCM = PCM;
    upload.bStereo = Clip->GetChannels() == 2;
    UploadAudioBuffer( _BufferId, &upload );

    channel->PlaybackPosition += upload.SamplesCount;

    return true;
}

AN_FORCEINLINE float GetGraceDistance( float _MaxDistance ) {
    return _MaxDistance * 1.3f;
}

static float CalcAudioVolume( SAudioChannel * _Channel ) {

    if ( _Channel->AudioClient ) {
        if ( AudioListenerId != _Channel->AudioClient ) {
            return 0.0f;
        }
    }

    float volume = MasterVolume * _Channel->Volume * ( _Channel->Group ? _Channel->Group->Volume : 1.0f );

    if ( _Channel->World ) {
        volume *= _Channel->World->AudioVolume;
    }

    if ( _Channel->ControlCallback ) {
        volume *= _Channel->ControlCallback->VolumeScale;
    }

    if ( _Channel->SourceType == AUDIO_SOURCE_BACKGROUND ) {
        return volume;
    }

    if ( volume < 0.0001f ) {
        return 0.0f;
    }

    float distance = ListenerPosition.Dist( _Channel->SoundPosition );

    distance -= _Channel->MaxDistance;
    if ( distance <= 0 ) {
        return volume;
    }

    const float GraceDistance = GetGraceDistance( _Channel->MaxDistance );

    if ( distance >= GraceDistance ) {
        return 0;
    }

    return volume * ( 1.0f - distance / GraceDistance );
}

static void PlayChannel( SAudioChannel * channel, float _PlayOffset ) {

    float playOffsetMod = Math::FMod( _PlayOffset, channel->Clip->GetDurationInSecounds() );

    if ( channel->bIsVirtual ) {
        channel->VirtualTime = _PlayOffset > 0 ? playOffsetMod : 0;
        return;
    }

    AL_SAFE( alSourcef( channel->SourceId, AL_PITCH, channel->Pitch ) );
    AL_SAFE( alSourcef( channel->SourceId, AL_GAIN, channel->CurVolume ) );
    AL_SAFE( alSourcefv( channel->SourceId, AL_VELOCITY, (float *)channel->Velocity.ToPtr() ) );

    if ( channel->SourceType == AUDIO_SOURCE_BACKGROUND ) {
        AL_SAFE( alSourcei( channel->SourceId, AL_SOURCE_RELATIVE, AL_TRUE ) );

        AL_SAFE( alSourcef( channel->SourceId, AL_REFERENCE_DISTANCE, channel->ReferenceDistance ) );
        AL_SAFE( alSourcef( channel->SourceId, AL_MAX_DISTANCE, channel->MaxDistance ) );
        AL_SAFE( alSourcef( channel->SourceId, AL_ROLLOFF_FACTOR, 0.0f ) );

        AL_SAFE( alSourcefv( channel->SourceId, AL_DIRECTION, (float *)Float3::Zero().ToPtr() ) );

        if ( bSourceSpatialize ) {
            AL_SAFE( alSourcei( channel->SourceId, AL_SOURCE_SPATIALIZE_SOFT, AL_FALSE ) );
        }
    } else {
        AL_SAFE( alSourcei( channel->SourceId, AL_SOURCE_RELATIVE, AL_FALSE ) );

        AL_SAFE( alSourcef( channel->SourceId, AL_REFERENCE_DISTANCE, channel->ReferenceDistance ) );
        AL_SAFE( alSourcef( channel->SourceId, AL_MAX_DISTANCE, channel->MaxDistance ) );
        AL_SAFE( alSourcef( channel->SourceId, AL_ROLLOFF_FACTOR, channel->RolloffFactor ) );

        if ( channel->bDirectional ) {
            AL_SAFE( alSourcefv( channel->SourceId, AL_DIRECTION, (float *)channel->Direction.ToPtr() ) );
            AL_SAFE( alSourcef( channel->SourceId, AL_CONE_INNER_ANGLE, channel->ConeInnerAngle ) );
            AL_SAFE( alSourcef( channel->SourceId, AL_CONE_OUTER_ANGLE, channel->ConeOuterAngle ) );
        } else {
            AL_SAFE( alSourcefv( channel->SourceId, AL_DIRECTION, (float *)Float3::Zero().ToPtr() ) );
        }

        if ( bSourceSpatialize ) {
            AL_SAFE( alSourcei( channel->SourceId, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE ) );
        }
    }

    AL_SAFE( alSourcefv( channel->SourceId, AL_POSITION, (float *)channel->SoundPosition.ToPtr() ) );

    if ( channel->bStreamed ) {

        IAudioStreamInterface * streamInterface = channel->StreamInterface;

        if ( !channel->StreamBuffers[0] ) {
            AL_SAFE( alGenBuffers( 2, channel->StreamBuffers ) );
        }

        AL_SAFE( alSourcei( channel->SourceId, AL_LOOPING, AL_FALSE ) );

        if ( _PlayOffset > 0 ) {
            // TODO: check this
            int PositionInSamples = playOffsetMod * channel->Clip->GetFrequency();
            streamInterface->StreamSeek( PositionInSamples );
            channel->PlaybackPosition = PositionInSamples;
        } else {
            streamInterface->StreamRewind();
            channel->PlaybackPosition = 0;
        }

        // Play
        channel->NumStreamBuffers = 0;
        if ( StreamToBuffer( channel, channel->StreamBuffers[0] ) ) {
            channel->NumStreamBuffers++;

            if ( StreamToBuffer( channel, channel->StreamBuffers[1] ) ) {
                channel->NumStreamBuffers++;
            }

            AL_SAFE( alSourceQueueBuffers( channel->SourceId, channel->NumStreamBuffers, channel->StreamBuffers ) );

            if ( !channel->bPausedByGame ) {
                AL_SAFE( alSourcePlay( channel->SourceId ) );
            }
        } else {
            // Can't play
            FreeChannel( channel );
            return;
        }

    } else {
        AL_SAFE( alSourcei( channel->SourceId, AL_LOOPING, channel->bLooping ) );
        AL_SAFE( alSourcei( channel->SourceId, AL_BUFFER, channel->Clip->GetBufferHandle() ) );

        // Remove data used for streaming
        if ( channel->StreamBuffers[0] ) {
            AL_SAFE( alDeleteBuffers( 2, channel->StreamBuffers ) );
            Core::ZeroMem( channel->StreamBuffers, sizeof( channel->StreamBuffers ) );
        }

        if ( _PlayOffset > 0 ) {
            AL_SAFE( alSourcef( channel->SourceId, AL_SEC_OFFSET, playOffsetMod ) );
        } else {
            //AL_SAFE( alSourcei( Id, AL_SAMPLE_OFFSET, 0 ) );
        }

        // Play
        if ( !channel->bPausedByGame ) {
            AL_SAFE( alSourcePlay( channel->SourceId ) );
        }
    }
}

static void CreateSound( AAudioClip * _AudioClip, Float3 const & _SpawnPosition, EAudioSourceType _Location, ASceneComponent * _Instigator, SSoundSpawnParameters const * _SpawnParameters ) {
    if ( !_AudioClip ) {
        return;
    }

    if ( _SpawnParameters->AudioClient && _SpawnParameters->AudioClient->IsPendingKill() ) {
        return;
    }

    SSoundAttenuationParameters const & atten = _SpawnParameters->Attenuation;

    float refDist = Math::Clamp( atten.ReferenceDistance, AUDIO_MIN_REF_DISTANCE, AUDIO_MAX_DISTANCE );
    float maxDist = Math::Clamp( atten.MaxDistance, refDist, AUDIO_MAX_DISTANCE );
    float graceDist = GetGraceDistance( maxDist );

    //int numActiveChannels = NumAudioChannels - NumFreeAudioChannels;

    bool bVirtualizeWhenSilent = _SpawnParameters->bVirtualizeWhenSilent || _SpawnParameters->bLooping;

    bool bSilent = _Location != AUDIO_SOURCE_BACKGROUND && /*numActiveChannels > 8 &&*/
            ListenerPosition.DistSqr( _SpawnPosition ) >= (maxDist+graceDist)*(maxDist+graceDist);

    if ( bSilent && !bVirtualizeWhenSilent ) {
        // Sound is too far from listener
        return;
    }

    IAudioStreamInterface * streamInterface = nullptr;

    // Initialize audio stream instance
    if ( _AudioClip->GetStreamType() != SOUND_STREAM_DISABLED ) {
        streamInterface = _AudioClip->CreateAudioStreamInstance();
        if ( !streamInterface ) {
            GLogger.Printf( "Couldn't create audio stream instance\n" );
            return;
        }
    }

    SAudioChannel * channel;

    if ( bSilent ) {
        channel = &VirtualChannels.Append();
        channel->bIsVirtual = true;
        channel->bFree = false;
        channel->ChannelIndex = VirtualChannels.Size() - 1;
        //GLogger.Printf( "ChannelIndex %d\n", channel->ChannelIndex );
    } else {
        channel = AllocateChannel( _SpawnParameters->Priority );
        if ( !channel ) {

            if ( bVirtualizeWhenSilent ) {
                channel = &VirtualChannels.Append();
                channel->bIsVirtual = true;
                channel->bFree = false;
                channel->ChannelIndex = VirtualChannels.Size() - 1;
            } else {
                return;
            }

        } else {
            channel->bIsVirtual = false;
        }
    }

    channel->PlayTimeStamp = GRuntime.SysFrameTimeStamp();
    channel->SpawnPosition = _SpawnPosition;
    channel->Pitch = _SpawnParameters->Pitch;
    channel->Volume = _SpawnParameters->Volume;
    channel->ReferenceDistance = refDist;
    channel->MaxDistance = maxDist;
    channel->RolloffFactor = atten.RolloffRate;
    channel->bLooping = _SpawnParameters->bLooping;
    channel->bStopWhenInstigatorDead = _SpawnParameters->bStopWhenInstigatorDead;
    channel->SourceType = _Location;
    channel->bStreamed = _AudioClip->GetStreamType() != SOUND_STREAM_DISABLED;
    channel->Clip = _AudioClip;
    channel->ClipSerialId = _AudioClip->GetSerialId();
    channel->StreamInterface = streamInterface;
    channel->Priority = _SpawnParameters->Priority;
    channel->bPlayEvenWhenPaused = _SpawnParameters->bPlayEvenWhenPaused;

    if ( _Location == AUDIO_SOURCE_BACKGROUND ) {
        channel->bDirectional = false;
        channel->Direction.Clear();
    } else {
        channel->bDirectional = _SpawnParameters->bDirectional;
        channel->ConeInnerAngle = Math::Clamp( _SpawnParameters->ConeInnerAngle, 0.0f, 360.0f );
        channel->ConeOuterAngle = Math::Clamp( _SpawnParameters->ConeOuterAngle, _SpawnParameters->ConeInnerAngle, 360.0f );

        if ( _Location == AUDIO_SOURCE_STATIC ) {
            channel->Direction = _SpawnParameters->Direction;
        } else if ( _Location == AUDIO_SOURCE_FOLLOW_INSIGATOR ) {
            channel->Direction = _Instigator ? _Instigator->GetWorldForwardVector() : _SpawnParameters->Direction;
        }
    }

    channel->ControlCallback = _SpawnParameters->ControlCallback;
    channel->AudioClient = _SpawnParameters->AudioClient ? _SpawnParameters->AudioClient->Id : 0;
    channel->Group = _SpawnParameters->Group;
    channel->Instigator = _Instigator;
    channel->PhysicalBody = nullptr;
    channel->World = _Instigator ? _Instigator->GetWorld() : nullptr;
    channel->bPausedByGame = false;
    channel->LifeSpan = _SpawnParameters->LifeSpan;
    channel->SoundPosition = channel->PrevSoundPosition = channel->SpawnPosition;
    channel->Velocity.Clear();
    channel->bUseVelocity = _SpawnParameters->bUseVelocity;
    channel->bUsePhysicalVelocity = _SpawnParameters->bUsePhysicalVelocity;
    channel->bVirtualizeWhenSilent = bVirtualizeWhenSilent;
    channel->bLocked = false;
    channel->CurVolume = CalcAudioVolume( channel );

    if ( channel->ControlCallback ) {
        channel->ControlCallback->AddRef();
    }

    if ( channel->Group ) {
        channel->Group->AddRef();
    }

    if ( channel->Instigator ) {
        channel->Instigator->AddRef();

        if ( channel->bUsePhysicalVelocity ) {
            channel->PhysicalBody = Upcast< APhysicalBody >( channel->Instigator );
        }
    }

    if ( channel->World ) {
        channel->World->AddRef();
    }

    if ( channel->StreamInterface ) {
        channel->StreamInterface->AddRef();
    }

    _AudioClip->AddRef();

    PlayChannel( channel, _SpawnParameters->PlayOffset );
}

void AAudioSystem::PlaySound( AAudioClip * _AudioClip, AActor * _Instigator, SSoundSpawnParameters const * _SpawnParameters ) {
    PlaySound( _AudioClip, _Instigator ? _Instigator->RootComponent : nullptr, _SpawnParameters );
}

void AAudioSystem::PlaySoundAt( AAudioClip * _AudioClip, Float3 const & _SpawnPosition, AActor * _Instigator, SSoundSpawnParameters const * _SpawnParameters ) {
    PlaySoundAt( _AudioClip, _SpawnPosition, _Instigator ? _Instigator->RootComponent : nullptr, _SpawnParameters );
}

void AAudioSystem::PlaySound( AAudioClip * _AudioClip, ASceneComponent * _Instigator, SSoundSpawnParameters const * _SpawnParameters ) {

    if ( !_SpawnParameters ) {
        _SpawnParameters = &DefaultSpawnParameters;
    }

    if ( _SpawnParameters->bStopWhenInstigatorDead && !_Instigator ) {
        GLogger.Printf( "AAudioSystem::PlaySound: bStopWhenInstigatorDead with no instigator specified\n" );
        return;
    }

    if ( _SpawnParameters->SourceType == AUDIO_SOURCE_STATIC ) {

        if ( _Instigator ) {
            CreateSound( _AudioClip, _Instigator->GetWorldPosition(), AUDIO_SOURCE_STATIC, _Instigator, _SpawnParameters );
        } else {
            GLogger.Printf( "AAudioSystem::PlaySound: no spawn location specified with flag AUDIO_STAY_AT_SPAWN_LOCATION\n" );
        }

    } else if ( _SpawnParameters->SourceType == AUDIO_SOURCE_FOLLOW_INSIGATOR ) {

        if ( _Instigator ) {
            CreateSound( _AudioClip, _Instigator->GetWorldPosition(), AUDIO_SOURCE_FOLLOW_INSIGATOR, _Instigator, _SpawnParameters );
        } else {
            GLogger.Printf( "AAudioSystem::PlaySound: no instigator specified with flag AUDIO_FOLLOW_INSIGATOR\n" );
        }

    } else if ( _SpawnParameters->SourceType == AUDIO_SOURCE_BACKGROUND ) {

        CreateSound( _AudioClip, Float3( 0 ), AUDIO_SOURCE_BACKGROUND, _Instigator, _SpawnParameters );

    } else {
        GLogger.Printf( "AAudioSystem::PlaySound: unknown spawn location\n" );
    }
}

void AAudioSystem::PlaySoundAt( AAudioClip * _AudioClip, Float3 const & _SpawnPosition, ASceneComponent * _Instigator, SSoundSpawnParameters const * _SpawnParameters ) {

    if ( !_SpawnParameters ) {
        _SpawnParameters = &DefaultSpawnParameters;
    }

    if ( _SpawnParameters->bStopWhenInstigatorDead && !_Instigator ) {
        GLogger.Printf( "AAudioSystem::PlaySoundAt: bStopWhenInstigatorDead with no instigator specified\n" );
        return;
    }

    CreateSound( _AudioClip, _SpawnPosition, AUDIO_SOURCE_STATIC, _Instigator, _SpawnParameters );
}

static void UpdateChannelStreaming( SAudioChannel * _Channel ) {
    if ( !_Channel->bStreamed || _Channel->bIsVirtual ) {
        return;
    }

    ALint processed = 0;

    AL_SAFE( alGetSourcei( _Channel->SourceId, AL_BUFFERS_PROCESSED, &processed ) );

    bool bPlay = ( processed == _Channel->NumStreamBuffers );

    while ( processed-- ) {
        ALuint buffer = 0;

        AL_SAFE( alSourceUnqueueBuffers( _Channel->SourceId, 1, &buffer ) );

        if ( !StreamToBuffer( _Channel, buffer ) ) {
            bool bShouldExit = true;

            if ( _Channel->bLooping ) {
                _Channel->StreamInterface->StreamRewind();
                _Channel->PlaybackPosition = 0;
                bShouldExit = !StreamToBuffer( _Channel, buffer );
            }

            if ( bShouldExit ) {
                return;
            }
        }

        AL_SAFE( alSourceQueueBuffers( _Channel->SourceId, 1, &buffer ) );
    }

    if ( bPlay ) {
        AL_SAFE( alSourcePlay( _Channel->SourceId ) );
    }
}

static void UpdateChannel( SAudioChannel * InChannel, float InTimeStep ) {
    if ( InChannel->bFree ) {
        return;
    }

    if ( InChannel->ClipSerialId != InChannel->Clip->GetSerialId() ) {
        // audio clip was modified
        FreeChannel( InChannel );
        //DevirtualizeOneChannel();
        // TODO: ReloadChannelAudio( i )
        return;
    }

    if ( InChannel->bStopWhenInstigatorDead ) {
        if ( InChannel->Instigator && InChannel->Instigator->IsPendingKill() ) {
            FreeChannel( InChannel );
            //DevirtualizeOneChannel();
            return;
        }
    }

    bool bUpdateSoundPosition = false;
    bool bUpdateSoundVelocity = false;
    bool bUpdateSoundDirection = false;

    if ( InChannel->SourceType == AUDIO_SOURCE_FOLLOW_INSIGATOR ) {

        if ( InChannel->Instigator && !InChannel->Instigator->IsPendingKill() ) {
            InChannel->PrevSoundPosition = InChannel->SoundPosition;
            InChannel->SoundPosition = InChannel->Instigator->GetWorldPosition();

            bUpdateSoundPosition = true;

            if ( InChannel->bUsePhysicalVelocity ) {
                if ( InChannel->PhysicalBody ) {
                    InChannel->Velocity = InChannel->PhysicalBody->GetLinearVelocity();
                    bUpdateSoundVelocity = true;
                }
            } else if ( InChannel->bUseVelocity ) {
                InChannel->Velocity = ( InChannel->SoundPosition - InChannel->PrevSoundPosition ) / InTimeStep;
                bUpdateSoundVelocity = true;
            }

            if ( InChannel->bDirectional ) {
                InChannel->Direction = InChannel->Instigator->GetWorldForwardVector();
                bUpdateSoundDirection = true;
            }
        }
    }

    if ( !InChannel->bIsVirtual ) {
        if ( !InChannel->bStreamed ) {
            int State;
            alGetSourcei( InChannel->SourceId, AL_SOURCE_STATE, &State );
            if ( State == AL_STOPPED ) {
                FreeChannel( InChannel );
                return;
            }
        } else {
            if ( !InChannel->bLooping ) {
                // FIXME: PlaybackPosition на самом деле находится немного в будущем, т.к.
                // инкрементируется сразу после отправки буфера на воспроизведение.
                // TODO: Нужно определить реальный playback position!
                if ( InChannel->PlaybackPosition >= InChannel->Clip->GetSamplesCount() ) {
                    FreeChannel( InChannel );
                    return;
                }
            }
        }
    }

    AWorld * world = InChannel->World;
    if ( world && !InChannel->bPlayEvenWhenPaused ) {
        if ( world->IsPaused() ) {
            if ( !InChannel->bPausedByGame ) {
                InChannel->bPausedByGame = true;

                if ( !InChannel->bIsVirtual ) {
                    alSourcePause( InChannel->SourceId );
                }
            }
        } else {
            if ( InChannel->bPausedByGame ) {
                InChannel->bPausedByGame = false;

                if ( !InChannel->bIsVirtual ) {
                    alSourcePlay( InChannel->SourceId );
                }
            }
        }
    }

    if ( InChannel->bPausedByGame ) {
        return;
    }

    if ( InChannel->LifeSpan > 0 ) {
        InChannel->LifeSpan -= InTimeStep;

        if ( InChannel->LifeSpan <= 0 ) {
            FreeChannel( InChannel );
            //DevirtualizeOneChannel();
            return;
        }
    }

    if ( InChannel->bIsVirtual ) {
        // Update playback position

        InChannel->VirtualTime += InTimeStep;

        if ( InChannel->VirtualTime >= InChannel->Clip->GetDurationInSecounds() ) {
            if ( InChannel->bLooping ) {
                InChannel->VirtualTime = Math::FMod( InChannel->VirtualTime, InChannel->Clip->GetDurationInSecounds() );
            } else {
                // Stopped
                FreeChannel( InChannel );
                //DevirtualizeOneChannel();
                return;
            }
        }

    } else {

        // Update world position
        if ( bUpdateSoundPosition ) {
            AL_SAFE( alSourcefv( InChannel->SourceId, AL_POSITION, ( float * )InChannel->SoundPosition.ToPtr() ) );
        }

        // Update velocity
        if ( bUpdateSoundVelocity ) {
            AL_SAFE( alSourcefv( InChannel->SourceId, AL_VELOCITY, ( float * )InChannel->Velocity.ToPtr() ) );
        }

        // Update direction
        if ( bUpdateSoundDirection ) {
            AL_SAFE( alSourcefv( InChannel->SourceId, AL_DIRECTION, ( float * )InChannel->Direction.ToPtr() ) );
        }

        // Update volume
        float volume = CalcAudioVolume( InChannel );
        if ( InChannel->CurVolume != volume ) {
            InChannel->CurVolume = volume;

            if ( InChannel->CurVolume == 0.0f ) {
                FreeOrVirtualizeChannel( InChannel );
            } else {
                AL_SAFE( alSourcef( InChannel->SourceId, AL_GAIN, volume ) );
            }
        }
    }

    UpdateChannelStreaming( InChannel );
}

int AAudioSystem::GetNumActiveChannels() const {
    return NumAudioChannels - NumFreeAudioChannels;
}

Float3 const & AAudioSystem::GetListenerPosition() const {
    return ListenerPosition;
}

void AAudioSystem::Update( APlayerController * _Controller, float _TimeStep ) {

    ASceneComponent * audioListener = _Controller ? _Controller->GetAudioListener() : nullptr;
    AAudioParameters * audioParameters = _Controller ? _Controller->GetAudioParameters() : nullptr;

    if ( audioListener ) {

        Float3x4 const & TransformMatrix = audioListener->GetWorldTransformMatrix();

        ListenerPosition = TransformMatrix.DecomposeTranslation();

        const Float3x3 ListenerRotation = TransformMatrix.DecomposeRotation();

        const ALfloat Orient[] = { -ListenerRotation[2].X, -ListenerRotation[2].Y, -ListenerRotation[2].Z, // Look at
                                   ListenerRotation[1].X,  ListenerRotation[1].Y,  ListenerRotation[1].Z  // up
                                 };

        AL_SAFE( alListenerfv( AL_ORIENTATION, Orient ) );

        AudioListenerId = audioListener->GetParentActor()->Id;

    } else {
        ListenerPosition.Clear();

        const ALfloat Orient[] = {  0, 0, -1, // Look at
                                    0, 1,  0  // up
                                 };

        AL_SAFE( alListenerfv( AL_ORIENTATION, Orient ) );

        AudioListenerId = 0;
    }

    AL_SAFE( alListenerfv( AL_POSITION, (float *)ListenerPosition.ToPtr() ) );

    if ( audioParameters ) {
        AL_SAFE( alListenerfv( AL_VELOCITY, &audioParameters->Velocity.X ) );
        AL_SAFE( alDopplerFactor( audioParameters->DopplerFactor ) );
        AL_SAFE( alDopplerVelocity( audioParameters->DopplerVelocity ) );
        AL_SAFE( alSpeedOfSound( audioParameters->SpeedOfSound ) );
        AL_SAFE( alDistanceModel( 0xD001 + audioParameters->DistanceModel ) );
        MasterVolume = audioParameters->Volume;
    } else {
        // set defaults
        AL_SAFE( alListenerfv( AL_VELOCITY, (float *)Float3::Zero().ToPtr() ) );
        AL_SAFE( alDopplerFactor( 1 ) );
        AL_SAFE( alDopplerVelocity( 1 ) );
        AL_SAFE( alSpeedOfSound( 343.3f ) );
        AL_SAFE( alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED ) );
        MasterVolume = 1;
    }

    //GLogger.Printf( "Total active audio channels %d (total %d free %d)\n", GetNumActiveChannels(), NumAudioChannels, NumFreeAudioChannels );

    int numFreeChannels = MAX_AUDIO_CHANNELS - GetNumActiveChannels();
    if ( numFreeChannels > 0 ) {
        int canRestore = Math::Min( numFreeChannels, VirtualChannels.Size() );

        // TODO: sort channels by priority, gain, etc?

            for ( int i = 0 ; i < VirtualChannels.Size() && canRestore > 0 ;  ) {
                SAudioChannel * channel = &VirtualChannels[i];

                float graceDist = GetGraceDistance( channel->MaxDistance );

                bool bSilent = channel->SourceType != AUDIO_SOURCE_BACKGROUND
                        && ListenerPosition.DistSqr( channel->SoundPosition ) >= (channel->MaxDistance+graceDist)*(channel->MaxDistance+graceDist);

                if ( bSilent ) {
                    i++;
                    continue;
                }

                if ( !DevirtualizeChannel( &VirtualChannels[i] ) ) {
                    break;
                }

                canRestore--;

                //if ( maxPriority < channel->Priority ) {
                //    maxPriority = channel->Priority;
                //    candidate = i;
                //}
            }

//            if ( candidate != -1 ) {
//                if ( !DevirtualizeChannel( &VirtualChannels[candidate] ) ) {
//                    break;
//                }
//            }
//            canRestore--;

    }

    // Update active channels
    for ( int i = 0 ; i < NumAudioChannels ; i++ ) {
        SAudioChannel * channel = &AudioChannels[i];

        UpdateChannel( channel, _TimeStep );
    }

    // Update virtual channels
    for ( int i = 0 ; i < VirtualChannels.Size() ; ) {
        SAudioChannel * channel = &VirtualChannels[i];

        UpdateChannel( channel, _TimeStep );

        if ( channel->bFree ) {
            // Was freed during update
            VirtualChannels.RemoveSwap( i );
            if ( i < VirtualChannels.Size() ) {
                VirtualChannels[ i ].ChannelIndex = i;
            }
        } else {
            i++;
        }
    }
}

SAudioBufferHandle CreateAudioBuffer( SAudioBufferUpload const * Upload ) {
    SAudioBufferHandle id;
    AL_SAFE( alGenBuffers( 1, &id ) );
    UploadAudioBuffer( id, Upload );
    return id;
}

void DeleteAudioBuffer( SAudioBufferHandle BufferId ) {
    AL_SAFE( alDeleteBuffers( 1, &BufferId ) );
}

void UploadAudioBuffer( SAudioBufferHandle BufferId, SAudioBufferUpload const * Upload ) {
    ALenum format;
    size_t size;

    switch ( Upload->BitsPerSample ) {
    case 16:
        format = Upload->bStereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        break;
    case 8:
        format = Upload->bStereo ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
        break;
    default:
        CriticalError( "UploadAudioBuffer: Unsupported bits per sample: %d\n", Upload->BitsPerSample );
        return;
    }

    size = Upload->SamplesCount * (Upload->BitsPerSample >> 3);

    if ( Upload->bStereo ) {
        size <<= 1;
    }

    AL_SAFE( alBufferData( BufferId, format, Upload->PCM, size, Upload->Frequency ) );
}
