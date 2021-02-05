/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/EngineInterface.h>
#include <Runtime/Public/InputDefs.h>

#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/HashFunc.h>
#include <Core/Public/WindowsDefs.h>
#include <Core/Public/Document.h>

#include "CPUInfo.h"
#include "GPUSync.h"

#include <SDL.h>

#include <chrono>

#ifdef AN_OS_LINUX
#include <unistd.h>
#include <sys/file.h>
#include <signal.h>
#include <dlfcn.h>
#endif

#ifdef AN_OS_ANDROID
#include <android/log.h>
#endif

#include <Runtime/Public/EntryDecl.h>

#define PROCESS_COULDNT_CHECK_UNIQUE 1
#define PROCESS_ALREADY_EXISTS       2
#define PROCESS_UNIQUE               3

ARuntime & GRuntime = ARuntime::Inst();

AAsyncJobManager GAsyncJobManager;
AAsyncJobList * GRenderFrontendJobList;
AAsyncJobList * GRenderBackendJobList;

ARuntimeVariable rt_VidWidth( _CTS( "rt_VidWidth" ), _CTS( "1600" ) );
ARuntimeVariable rt_VidHeight( _CTS( "rt_VidHeight" ), _CTS( "900" ) );
#ifdef AN_DEBUG
ARuntimeVariable rt_VidFullscreen( _CTS( "rt_VidFullscreen" ), _CTS( "0" ) );
#else
ARuntimeVariable rt_VidFullscreen( _CTS( "rt_VidFullscreen" ), _CTS( "1" ) );
#endif
ARuntimeVariable rt_SyncGPU( _CTS( "rt_SyncGPU" ), _CTS( "0" ) );

ARuntimeVariable rt_SwapInterval( _CTS( "rt_SwapInterval" ), _CTS( "0" ), 0, _CTS( "1 - enable vsync, 0 - disable vsync, -1 - tearing" ) );

static int TotalAllocatedRenderCore = 0;
static SDL_Window * WindowHandle;
static TRef< AGPUSync > GPUSync;

static int PressedKeys[KEY_LAST+1];
static bool PressedMouseButtons[MOUSE_BUTTON_8+1];
static unsigned char JoystickButtonState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];
static short JoystickAxisState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];
static bool JoystickAdded[ MAX_JOYSTICKS_COUNT ];

static void InitKeyMappingsSDL();
static void LoggerMessageCallback( int _Level, const char * _Message );

IEngineInterface * ARuntime::Engine = nullptr;

ARuntime::ARuntime()
    : Rand( Core::RandomSeed() )
{
    NumArguments = 0;
    Arguments = nullptr;
    Executable = nullptr;
    FrameMemoryAddress = nullptr;
    FrameMemorySize = 0;
    FrameMemoryUsed = 0;
    FrameMemoryUsedPrev = 0;
    MaxFrameMemoryUsage = 0;
    bPostTerminateEvent = false;
    bPostChangeVideoMode = false;
    Clipboard = nullptr;
    ProcessAttribute = 0;
}

void ARuntime::LoadConfigFile()
{
    AString configFile = GetRootPath() + "config.cfg";
    AFileStream f;
    if ( f.OpenRead( configFile ) ) {
        AString data;

        data.FromFile( f );

        ARuntimeCommandProcessor cmdProcessor;

        cmdProcessor.Add( data.CStr() );

        class CommandContext : public IRuntimeCommandContext {
        public:
            void ExecuteCommand( ARuntimeCommandProcessor const & _Proc ) override {
                AN_ASSERT( _Proc.GetArgsCount() > 0 );

                const char * name = _Proc.GetArg( 0 );
                ARuntimeVariable * var;
                if ( nullptr != (var = ARuntimeVariable::FindVariable( name )) ) {
                    if ( _Proc.GetArgsCount() < 2 ) {
                        var->Print();
                    } else {
                        var->SetString( _Proc.GetArg( 1 ) );
                    }
                 }
            }
        };

        CommandContext context;

        cmdProcessor.Execute( context );
    }
}

static SDL_Window * CreateGenericWindow( SVideoMode const & _VideoMode )
{
    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;

    bool bOpenGL = true;

    SDL_InitSubSystem( SDL_INIT_VIDEO );

    if ( bOpenGL ) {
        flags |= SDL_WINDOW_OPENGL;

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 5 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_EGL, 0 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, 0 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS,
                             SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
#ifdef AN_DEBUG
                             | SDL_GL_CONTEXT_DEBUG_FLAG
#endif
        );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0 );
        SDL_GL_SetAttribute( SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1 );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_RELEASE_BEHAVIOR,  );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_RESET_NOTIFICATION,  );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_NO_ERROR,  );
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_STEREO, 0 );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );
        //SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,  );
        //SDL_GL_SetAttribute( SDL_GL_RETAINED_BACKING,  );
    }

    int x, y;

    if ( _VideoMode.bFullscreen ) {
        flags |= SDL_WINDOW_FULLSCREEN;// | SDL_WINDOW_BORDERLESS;
        x = y = 0;
    }
    else {
        if ( _VideoMode.bCentrized ) {
            x = SDL_WINDOWPOS_CENTERED;
            y = SDL_WINDOWPOS_CENTERED;
        }
        else {
            x = _VideoMode.WindowedX;
            y = _VideoMode.WindowedY;
        }
    }

    return SDL_CreateWindow( _VideoMode.Title, x, y, _VideoMode.Width, _VideoMode.Height, flags );
}

static void DestroyGenericWindow( SDL_Window * Window )
{
    if ( Window ) {
        SDL_DestroyWindow( Window );
        SDL_QuitSubSystem( SDL_INIT_VIDEO );
    }
}

void ARuntime::PrintCPUFeatures()
{
    GLogger.Printf( "CPU: %s\n", CPUInfo.Intel ? "Intel" : "AMD" );
    GLogger.Print( "CPU Features:" );
    if ( CPUInfo.MMX ) GLogger.Print( " MMX" );
    if ( CPUInfo.x64 ) GLogger.Print( " x64" );
    if ( CPUInfo.ABM ) GLogger.Print( " ABM" );
    if ( CPUInfo.RDRAND ) GLogger.Print( " RDRAND" );
    if ( CPUInfo.BMI1 ) GLogger.Print( " BMI1" );
    if ( CPUInfo.BMI2 ) GLogger.Print( " BMI2" );
    if ( CPUInfo.ADX ) GLogger.Print( " ADX" );
    if ( CPUInfo.MPX ) GLogger.Print( " MPX" );
    if ( CPUInfo.PREFETCHWT1 ) GLogger.Print( " PREFETCHWT1" );
    GLogger.Print( "\n" );
    GLogger.Print( "Simd 128 bit:" );
    if ( CPUInfo.SSE ) GLogger.Print( " SSE" );
    if ( CPUInfo.SSE2 ) GLogger.Print( " SSE2" );
    if ( CPUInfo.SSE3 ) GLogger.Print( " SSE3" );
    if ( CPUInfo.SSSE3 ) GLogger.Print( " SSSE3" );
    if ( CPUInfo.SSE4a ) GLogger.Print( " SSE4a" );
    if ( CPUInfo.SSE41 ) GLogger.Print( " SSE4.1" );
    if ( CPUInfo.SSE42 ) GLogger.Print( " SSE4.2" );
    if ( CPUInfo.AES ) GLogger.Print( " AES-NI" );
    if ( CPUInfo.SHA ) GLogger.Print( " SHA" );
    GLogger.Print( "\n" );
    GLogger.Print( "Simd 256 bit:" );
    if ( CPUInfo.AVX ) GLogger.Print( " AVX" );
    if ( CPUInfo.XOP ) GLogger.Print( " XOP" );
    if ( CPUInfo.FMA3 ) GLogger.Print( " FMA3" );
    if ( CPUInfo.FMA4 ) GLogger.Print( " FMA4" );
    if ( CPUInfo.AVX2 ) GLogger.Print( " AVX2" );
    GLogger.Print( "\n" );
    GLogger.Print( "Simd 512 bit:" );
    if ( CPUInfo.AVX512_F ) GLogger.Print( " AVX512-F" );
    if ( CPUInfo.AVX512_CD ) GLogger.Print( " AVX512-CD" );
    if ( CPUInfo.AVX512_PF ) GLogger.Print( " AVX512-PF" );
    if ( CPUInfo.AVX512_ER ) GLogger.Print( " AVX512-ER" );
    if ( CPUInfo.AVX512_VL ) GLogger.Print( " AVX512-VL" );
    if ( CPUInfo.AVX512_BW ) GLogger.Print( " AVX512-BW" );
    if ( CPUInfo.AVX512_DQ ) GLogger.Print( " AVX512-DQ" );
    if ( CPUInfo.AVX512_IFMA ) GLogger.Print( " AVX512-IFMA" );
    if ( CPUInfo.AVX512_VBMI ) GLogger.Print( " AVX512-VBMI" );
    GLogger.Print( "\n" );
    GLogger.Print( "OS: " AN_OS_STRING "\n" );
    GLogger.Print( "OS Features:" );
    if ( CPUInfo.OS_64bit ) GLogger.Print( " 64bit" );
    if ( CPUInfo.OS_AVX ) GLogger.Print( " AVX" );
    if ( CPUInfo.OS_AVX512 ) GLogger.Print( " AVX512" );
    GLogger.Print( "\n" );
    GLogger.Print( "Endian: " AN_ENDIAN_STRING "\n" );
    #ifdef AN_DEBUG
    GLogger.Print( "Compiler: " AN_COMPILER_STRING "\n" );
    #endif
}

static TUniqueRef< AArchive > GEmbeddedResourcesArch;

extern "C" const size_t EmbeddedResources_Size;
extern "C" const uint64_t EmbeddedResources_Data[];

AArchive const & GetEmbeddedResources()
{
    if ( !GEmbeddedResourcesArch ) {
        GEmbeddedResourcesArch = MakeUnique< AArchive >();
        if ( !GEmbeddedResourcesArch->OpenFromMemory( EmbeddedResources_Data, EmbeddedResources_Size ) ) {
            GLogger.Printf( "Failed to open embedded resources\n" );
        }
    }
    return *GEmbeddedResourcesArch.GetObject();
}

void ARuntime::Run( SEntryDecl const & _EntryDecl )
{
    // Synchronize SDL ticks with our start time
    (void)SDL_GetTicks();

    StartMicroseconds = StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count();
    StartMilliseconds = StartMicroseconds * 0.001;
    StartSeconds = StartMicroseconds * 0.000001;
    FrameTimeStamp = StartMicroseconds;
    FrameDuration = 1000000.0 / 60;
    FrameNumber = 0;

    pModuleDecl = &_EntryDecl;

    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        EmergencyExit();
    }

    ::GetCPUInfo( CPUInfo );

    InitializeProcess();

    Engine = CreateEngineInstance();

    GLogger.SetMessageCallback( LoggerMessageCallback );

    PrintCPUFeatures();

    switch ( ProcessAttribute ) {
        case PROCESS_COULDNT_CHECK_UNIQUE:
            CriticalError( "Couldn't check unique instance\n" );
            break;
        case PROCESS_ALREADY_EXISTS:
            CriticalError( "Process already exists\n" );
            break;
        case PROCESS_UNIQUE:
            break;
    }

    InitializeMemory();

    InitializeWorkingDirectory();

    ARuntimeVariable::AllocateVariables();

    GLogger.Printf( "Working directory: %s\n", WorkingDir.CStr() );
    GLogger.Printf( "Root path: %s\n", RootPath.CStr() );
    GLogger.Printf( "Executable: %s\n", Executable );

    SDL_LogSetOutputFunction(
                [](void *userdata, int category, SDL_LogPriority priority, const char *message) {
                    GLogger.Printf( "SDL: %d : %s\n", category, message );
                },
                NULL );

    SDL_InitSubSystem( SDL_INIT_SENSOR | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS );

    if ( AThread::NumHardwareThreads ) {
        GLogger.Printf( "Num hardware threads: %d\n", AThread::NumHardwareThreads );
    }

    int jobManagerThreadCount = AThread::NumHardwareThreads ? Math::Min( AThread::NumHardwareThreads, GAsyncJobManager.MAX_WORKER_THREADS )
        : GAsyncJobManager.MAX_WORKER_THREADS;
    GAsyncJobManager.Initialize( jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS );

    GRenderFrontendJobList = GAsyncJobManager.GetAsyncJobList( RENDER_FRONTEND_JOB_LIST );
    GRenderBackendJobList = GAsyncJobManager.GetAsyncJobList( RENDER_BACKEND_JOB_LIST );

    InitKeyMappingsSDL();

    Core::ZeroMem( PressedKeys, sizeof( PressedKeys ) );
    Core::ZeroMem( PressedMouseButtons, sizeof( PressedMouseButtons ) );
    Core::ZeroMem( JoystickButtonState, sizeof( JoystickButtonState ) );
    Core::ZeroMem( JoystickAxisState, sizeof( JoystickAxisState ) );
    Core::ZeroMem( JoystickAdded, sizeof( JoystickAdded ) );

    LoadConfigFile();

    SVideoMode desiredMode = {};
    desiredMode.Width = rt_VidWidth.GetInteger();
    desiredMode.Height = rt_VidHeight.GetInteger();
    desiredMode.Opacity = 1;
    desiredMode.bFullscreen = rt_VidFullscreen;
    desiredMode.bCentrized = true;
    Core::Strcpy( desiredMode.Backend, sizeof( desiredMode.Backend ), "OpenGL 4.5" );
    Core::Strcpy( desiredMode.Title, sizeof( desiredMode.Title ), _EntryDecl.GameTitle );

    WindowHandle = CreateGenericWindow( desiredMode );
    if ( !WindowHandle ) {
        CriticalError( "Failed to create main window\n" );
    }

    RenderCore::SImmediateContextCreateInfo stateCreateInfo = {};
    stateCreateInfo.ClipControl = RenderCore::CLIP_CONTROL_DIRECTX;
    stateCreateInfo.ViewportOrigin = RenderCore::VIEWPORT_ORIGIN_TOP_LEFT;
    stateCreateInfo.SwapInterval = rt_SwapInterval.GetInteger();
    stateCreateInfo.Window = WindowHandle;

    RenderCore::SAllocatorCallback allocator;

    allocator.Allocate =
        []( size_t _BytesCount )
        {
            TotalAllocatedRenderCore++;
            return GZoneMemory.Alloc( _BytesCount );
        };

    allocator.Deallocate =
        []( void * _Bytes )
        {
            TotalAllocatedRenderCore--;
            GZoneMemory.Free( _Bytes );
        };

    CreateLogicalDevice( stateCreateInfo,
                         &allocator,
                         []( const unsigned char * _Data, int _Size )
                         {
                             return Core::Hash( ( const char * )_Data, _Size );
                         },
                         &RenderDevice );

    VertexMemoryGPU = MakeRef< AVertexMemoryGPU >( RenderDevice );
    StreamedMemoryGPU = MakeRef< AStreamedMemoryGPU >( RenderDevice );

    RenderDevice->GetImmediateContext( &pImmediateContext );

    GPUSync = MakeRef< AGPUSync >( pImmediateContext );

    SetVideoMode( desiredMode );

    // Process initial events
    GRuntime.PollEvents();

    Engine->Run( _EntryDecl );

    DestroyEngineInstance();
    Engine = nullptr;

    ARuntimeVariable::FreeVariables();

    GAsyncJobManager.Deinitialize();

    if ( IsCriticalError() ) {
        EmergencyExit();
    }

    GPUSync.Reset();

    VertexMemoryGPU.Reset();
    StreamedMemoryGPU.Reset();

    RenderDevice.Reset();

    DestroyGenericWindow( WindowHandle );
    WindowHandle = nullptr;

    GLogger.Printf( "TotalAllocatedRenderCore: %d\n", TotalAllocatedRenderCore );

    SDocumentAllocator< ADocValue >::FreeMemoryPool();
    SDocumentAllocator< ADocMember >::FreeMemoryPool();

    GEmbeddedResourcesArch.Reset();

    WorkingDir.Free();
    RootPath.Free();

    if ( Clipboard ) {
        SDL_free( Clipboard );
    }

    SDL_Quit();

    DeinitializeMemory();

    DeinitializeProcess();
}

struct SProcessLog {
    FILE * File;
};

static SProcessLog ProcessLog = {};
static AMutex LoggerSync;

void ARuntime::LoggerMessageCallback( int _Level, const char * _Message )
{
    #if defined AN_DEBUG
        #if defined AN_COMPILER_MSVC
        {
            int n = MultiByteToWideChar( CP_UTF8, 0, _Message, -1, NULL, 0 );
            if ( 0 != n ) {
                // Try to alloc on stack
                if ( n < 4096 ) {
                    wchar_t * chars = (wchar_t *)StackAlloc( n * sizeof( wchar_t ) );

                    MultiByteToWideChar( CP_UTF8, 0, _Message, -1, chars, n );

                    OutputDebugString( chars );
                }
                else {
                    wchar_t * chars = (wchar_t *)malloc( n * sizeof( wchar_t ) );

                    MultiByteToWideChar( CP_UTF8, 0, _Message, -1, chars, n );

                    OutputDebugString( chars );

                    free( chars );
                }
            }
        }
        #else
            #ifdef AN_OS_ANDROID
                __android_log_print( ANDROID_LOG_INFO, "Angie Engine", _Message );
            #else
                fprintf( stdout, "%s", _Message );
                fflush( stdout );
            #endif
        #endif
    #endif

    if ( Engine ) {
        Engine->Print( _Message );
    }

    if ( ProcessLog.File ) {
        AMutexGurad syncGuard( LoggerSync );
        fprintf( ProcessLog.File, "%s", _Message );
        fflush( ProcessLog.File );
    }
}

#ifdef AN_ALLOW_ASSERTS

// Define global assert function
void AssertFunction( const char * _File, int _Line, const char * _Function, const char * _Assertion, const char * _Comment )
{
    static thread_local bool bNestedFunctionCall = false;

    if ( bNestedFunctionCall ) {
        // Assertion occurs in logger's print function
        return;
    }

    bNestedFunctionCall = true;

    // Printf is thread-safe function so we don't need to wrap it by a critical section
    GLogger.Printf( "===== Assertion failed =====\n"
                    "At file %s, line %d\n"
                    "Function: %s\n"
                    "Assertion: %s\n"
                    "%s%s"
                    "============================\n",
                    _File, _Line, _Function, _Assertion, _Comment ? _Comment : "", _Comment ? "\n" : "" );

    SDL_SetRelativeMouseMode( SDL_FALSE ); // FIXME: Is it threadsafe?

#ifdef AN_OS_WIN32
    DebugBreak();
#else
    //__asm__( "int $3" );
    raise( SIGTRAP );
#endif

    bNestedFunctionCall = false;
}

#endif

#ifdef AN_OS_WIN32
static HANDLE ProcessMutex = NULL;
#endif

void ARuntime::InitializeProcess()
{
    setlocale( LC_ALL, "C" );
    srand( ( unsigned )time( NULL ) );

#if defined( AN_OS_WIN32 )
    SetErrorMode( SEM_FAILCRITICALERRORS );
#endif

#ifdef AN_OS_WIN32
    int curLen = 1024;
    int len = 0;

    Executable = nullptr;
    while ( 1 ) {
        Executable = ( char * )realloc( Executable, curLen + 1 );
        len = GetModuleFileNameA( NULL, Executable, curLen );
        if ( len < curLen && len != 0 ) {
            break;
        }
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
            curLen <<= 1;
        } else {
            CriticalError( "InitializeProcess: Failed on GetModuleFileName\n" );
            len = 0;
            break;
        }
    }
    Executable[ len ] = 0;

    Core::FixSeparator( Executable );

    uint32_t appHash = Core::SDBMHash( Executable, len );

    ProcessMutex = CreateMutexA( NULL, FALSE, Core::Fmt( "angie_%u", appHash ) );
    if ( !ProcessMutex ) {
        ProcessAttribute = PROCESS_COULDNT_CHECK_UNIQUE;
    } else if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        ProcessAttribute = PROCESS_ALREADY_EXISTS;
    } else {
        ProcessAttribute = PROCESS_UNIQUE;
    }
#elif defined AN_OS_LINUX
    int curLen = 1024;
    int len = 0;

    Executable = nullptr;
    while ( 1 ) {
        Executable = ( char * )realloc( Executable, curLen + 1 );
        len = readlink( "/proc/self/exe", Executable, curLen );
        if ( len == -1 ) {
            CriticalError( "InitializeProcess: Failed on readlink\n" );
            len = 0;
            break;
        }
        if ( len < curLen ) {
            break;
        }
        curLen <<= 1;
    }
    Executable[ len ] = 0;

    uint32_t appHash = Core::SDBMHash( Executable, len );
    int f = open( Core::Fmt( "/tmp/angie_%u.pid", appHash ), O_RDWR | O_CREAT, 0666 );
    int locked = flock( f, LOCK_EX | LOCK_NB );
    if ( locked ) {
        if ( errno == EWOULDBLOCK ) {
            ProcessAttribute = PROCESS_ALREADY_EXISTS;
        } else {
            ProcessAttribute = PROCESS_COULDNT_CHECK_UNIQUE;
        }
    } else {
        ProcessAttribute = PROCESS_UNIQUE;
    }
#else
#   error "Not implemented under current platform"
#endif

    ProcessLog.File = nullptr;
    if ( CheckArg( "-enableLog" ) != -1 ) {
        // TODO: Set correct path for log file
        ProcessLog.File = fopen( "log.txt", "ab" );
    }
}

void ARuntime::DeinitializeProcess()
{
    if ( ProcessLog.File ) {
        fclose( ProcessLog.File );
        ProcessLog.File = nullptr;
    }

    if ( Executable ) {
        free( Executable );
        Executable = nullptr;
    }

#ifdef AN_OS_WIN32
    if ( ProcessMutex ) {
        ReleaseMutex( ProcessMutex );
        CloseHandle( ProcessMutex );
        ProcessMutex = NULL;
    }
#endif
}

struct SMemoryInfo
{
    int TotalAvailableMegabytes;
    int CurrentAvailableMegabytes;
};

static SMemoryInfo GetPhysMemoryInfo()
{
    SMemoryInfo info;

    Core::ZeroMem( &info, sizeof( info ) );

#if defined AN_OS_WIN32
    MEMORYSTATUS memstat;
    GlobalMemoryStatus( &memstat );
    info.TotalAvailableMegabytes = memstat.dwTotalPhys >> 20;
    info.CurrentAvailableMegabytes = memstat.dwAvailPhys >> 20;
#elif defined AN_OS_LINUX
    long long TotalPages = sysconf( _SC_PHYS_PAGES );
    long long AvailPages = sysconf( _SC_AVPHYS_PAGES );
    long long PageSize = sysconf( _SC_PAGE_SIZE );
    info.TotalAvailableMegabytes = ( TotalPages * PageSize ) >> 20;
    info.CurrentAvailableMegabytes = ( AvailPages * PageSize ) >> 20;
#else
#   error "GetPhysMemoryInfo not implemented under current platform"
#endif
    return info;
}

volatile int MemoryChecksum;
static void * MemoryHeap;

static void TouchMemoryPages( void * _MemoryPointer, int _MemorySize )
{
    GLogger.Printf( "Touching memory pages...\n" );

    byte * p = ( byte * )_MemoryPointer;
    for ( int n = 0 ; n < 4 ; n++ ) {
        for ( int m = 0 ; m < ( _MemorySize - 16 * 0x1000 ) ; m += 4 ) {
            MemoryChecksum += *( int32_t * )&p[ m ];
            MemoryChecksum += *( int32_t * )&p[ m + 16 * 0x1000 ];
        }
    }

    //int j = _MemorySize >> 2;
    //for ( int i = 0 ; i < j ; i += 64 ) {
    //    MemoryChecksum += ( ( int32_t * )_MemoryPointer )[ i ];
    //}
}

void ARuntime::InitializeMemory()
{
    const size_t ZoneSizeInMegabytes = 256;
    const size_t HunkSizeInMegabytes = 32;
    const size_t FrameMemorySizeInMegabytes = 16;

    const size_t TotalMemorySizeInBytes = ( ZoneSizeInMegabytes + HunkSizeInMegabytes + FrameMemorySizeInMegabytes ) << 20;

#ifdef AN_OS_WIN32
    if ( !SetProcessWorkingSetSize( GetCurrentProcess(), TotalMemorySizeInBytes, 1024 << 20 ) ) {
        GLogger.Printf( "Failed on SetProcessWorkingSetSize\n" );
    }
#endif

    int pageSize;
#ifdef AN_OS_WIN32
    SYSTEM_INFO systemInfo;
    GetSystemInfo( &systemInfo );
    pageSize = systemInfo.dwPageSize;
#elif defined AN_OS_LINUX
    pageSize = sysconf( _SC_PAGE_SIZE );
#else
#   error "GetPageSize not implemented under current platform"
#endif

    GLogger.Printf( "Memory page size: %d bytes\n", pageSize );

    SMemoryInfo physMemoryInfo = GetPhysMemoryInfo();
    if ( physMemoryInfo.TotalAvailableMegabytes > 0 && physMemoryInfo.CurrentAvailableMegabytes > 0 ) {
        GLogger.Printf( "Total available phys memory: %d Megs\n", physMemoryInfo.TotalAvailableMegabytes );
        GLogger.Printf( "Current available phys memory: %d Megs\n", physMemoryInfo.CurrentAvailableMegabytes );
    }

    GLogger.Printf( "Zone memory size: %d Megs\n"
                    "Hunk memory size: %d Megs\n"
                    "Frame memory size: %d Megs\n",
                    ZoneSizeInMegabytes, HunkSizeInMegabytes, FrameMemorySizeInMegabytes );

    GHeapMemory.Initialize();

    MemoryHeap = GHeapMemory.Alloc( TotalMemorySizeInBytes, 16 );
    Core::ZeroMemSSE( MemoryHeap, TotalMemorySizeInBytes );

    //TouchMemoryPages( MemoryHeap, TotalMemorySizeInBytes );

    void * ZoneMemory = MemoryHeap;
    GZoneMemory.Initialize( ZoneMemory, ZoneSizeInMegabytes );

    void * HunkMemory = ( byte * )MemoryHeap + ( ZoneSizeInMegabytes << 20 );
    GHunkMemory.Initialize( HunkMemory, HunkSizeInMegabytes );

    FrameMemoryAddress = ( byte * )MemoryHeap + ( ( ZoneSizeInMegabytes + HunkSizeInMegabytes ) << 20 );
    FrameMemorySize = FrameMemorySizeInMegabytes << 20;
}

void ARuntime::DeinitializeMemory()
{
    GZoneMemory.Deinitialize();
    GHunkMemory.Deinitialize();
    GHeapMemory.Free( MemoryHeap );
    GHeapMemory.Deinitialize();
}

void ARuntime::InitializeWorkingDirectory()
{
    WorkingDir = Executable;
    WorkingDir.StripFilename();

#if defined AN_OS_WIN32
    SetCurrentDirectoryA( WorkingDir.CStr() );
#elif defined AN_OS_LINUX
    chdir( WorkingDir.CStr() );
#else
#   error "InitializeWorkingDirectory not implemented under current platform"
#endif

    RootPath = pModuleDecl->RootPath;
    if ( RootPath.IsEmpty() ) {
        RootPath = "Data/";
    } else {
        RootPath.FixSeparator();
        if ( RootPath[RootPath.Length()-1] != '/' ) {
            RootPath += '/';
        }
    }
}

void ARuntime::DisplayCriticalMessage( const char * _Message )
{
#if defined AN_OS_WIN32
    wchar_t wstr[1024];
    MultiByteToWideChar( CP_UTF8, 0, _Message, -1, wstr, AN_ARRAY_SIZE( wstr ) );
    MessageBox( NULL, wstr, L"Critical Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST );
#else
    SDL_MessageBoxData data = {};
    SDL_MessageBoxButtonData button = {};
    const SDL_MessageBoxColorScheme scheme =
    {
        {
            { 56,  54,  53  }, /* SDL_MESSAGEBOX_COLOR_BACKGROUND, */
            { 209, 207, 205 }, /* SDL_MESSAGEBOX_COLOR_TEXT, */
            { 140, 135, 129 }, /* SDL_MESSAGEBOX_COLOR_BUTTON_BORDER, */
            { 105, 102, 99  }, /* SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND, */
            { 205, 202, 53  }, /* SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED, */
        }
    };

    data.flags = SDL_MESSAGEBOX_ERROR;
    data.title = "Critical Error";
    data.message = _Message;
    data.numbuttons = 1;
    data.buttons = &button;
    data.window = NULL;
    data.colorScheme = &scheme;

    button.flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    button.flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    button.text = "OK";

    SDL_ShowMessageBox( &data, NULL );
#endif
}

void ARuntime::EmergencyExit()
{
    const char * msg = MapCriticalErrorMessage();
    DisplayCriticalMessage( msg );
    UnmapCriticalErrorMessage();

    SDL_Quit();

    GHeapMemory.Clear();

    DeinitializeProcess();

    std::quick_exit( 0 );
    //std::abort();
}

int ARuntime::CheckArg( const char * _Arg )
{
    for ( int i = 0 ; i < NumArguments ; i++ ) {
        if ( !Core::Stricmp( Arguments[ i ], _Arg ) ) {
            return i;
        }
    }
    return -1;
}

static void AllocCommandLineArgs( char * _Buffer, int * _NumArguments, char *** _Arguments )
{
    char ** args = nullptr;
    int count;
    char * s;

    // Parse command line
    s = _Buffer;
    count = 0;

    // get argc
    while ( *s ) {
        while ( *s && ( ( *s <= 32 && *s > 0 ) || *s > 126 ) ) {
            s++;
        }
        if ( *s ) {
            bool q = false;
            while ( *s ) {
                if ( *s == '\"' ) {
                    q = !q;
                    if ( !q ) {
                        s++;
                        break;
                    } else {
                        s++;
                        continue;
                    }
                }
                if ( !q && ( ( *s <= 32 && *s > 0 ) || *s > 126 ) ) {
                    break;
                }
                s++;
            }
            count++;
        }
    }

    if ( count > 0 ) {
        args = ( char ** )calloc( count, sizeof( char * ) );

        s = _Buffer;
        count = 0;

        while ( *s ) {
            while ( *s && ( ( *s <= 32 && *s > 0 ) || *s > 126 ) ) {
                s++;
            }
            if ( *s ) {
                bool q = false;
                args[ count ] = s;
                while ( *s ) {
                    if ( *s == '\"' ) {
                        q = !q;
                        if ( !q ) {
                            s++;
                            break;
                        } else {
                            args[ count ] = ++s;
                            continue;
                        }
                    }
                    if ( !q && ( ( *s <= 32 && *s > 0 ) || *s > 126 ) ) {
                        break;
                    }
                    s++;
                }
                if ( *s ) {
                    *s = 0;
                    s++;
                }
                count++;
            }
        }
    }

    *_NumArguments = count;
    *_Arguments = args;
}

static void FreeCommandLineArgs( char ** _Arguments )
{
    free( _Arguments );
}

#define MAX_COMMAND_LINE_LENGTH 1024
static char CmdLineBuffer[ MAX_COMMAND_LINE_LENGTH ];
static bool bApplicationRun = false;

void Runtime( const char * _CommandLine, SEntryDecl const & _EntryDecl )
{
    if ( bApplicationRun ) {
        AN_ASSERT( 0 );
        return;
    }
    bApplicationRun = true;
    Core::Strcpy( CmdLineBuffer, sizeof( CmdLineBuffer ), _CommandLine );
    AllocCommandLineArgs( CmdLineBuffer, &GRuntime.NumArguments, &GRuntime.Arguments );
    if ( GRuntime.NumArguments < 1 ) {
        AN_ASSERT( 0 );
        return;
    }
    // Fix executable path separator
    Core::FixSeparator( GRuntime.Arguments[ 0 ] );
    GRuntime.Run( _EntryDecl );
    FreeCommandLineArgs( GRuntime.Arguments );
}

void Runtime( int _Argc, char ** _Argv, SEntryDecl const & _EntryDecl )
{
    if ( bApplicationRun ) {
        AN_ASSERT( 0 );
        return;
    }
    bApplicationRun = true;
    GRuntime.NumArguments = _Argc;
    GRuntime.Arguments = _Argv;
    if ( GRuntime.NumArguments < 1 ) {
        AN_ASSERT( 0 );
        return;
    }
    // Fix executable path separator
    Core::FixSeparator( GRuntime.Arguments[ 0 ] );
    GRuntime.Run( _EntryDecl );
}

#if defined( AN_DEBUG ) && defined( AN_COMPILER_MSVC )
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved )
{
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

    return TRUE;
}
#endif

int ARuntime::GetArgc()
{
    return NumArguments;
}

const char * const *ARuntime::GetArgv()
{
    return Arguments;
}

AString const & ARuntime::GetWorkingDir()
{
    return WorkingDir;
}

AString const & ARuntime::GetRootPath()
{
    return RootPath;
}

const char * ARuntime::GetExecutableName()
{
    return Executable ? Executable : "";
}

void * ARuntime::AllocFrameMem( size_t _SizeInBytes )
{
    if ( FrameMemoryUsed + _SizeInBytes > FrameMemorySize ) {
        // TODO: Allocate new block
        CriticalError( "AllocFrameMem: failed on allocation of %u bytes (available %u, total %u)\n", _SizeInBytes, FrameMemorySize - FrameMemoryUsed, FrameMemorySize );
        return nullptr;
    }

    void * pMemory = (byte *)FrameMemoryAddress + FrameMemoryUsed;

    FrameMemoryUsed += _SizeInBytes;
    FrameMemoryUsed = Align( FrameMemoryUsed, 16 );

    AN_ASSERT( IsAlignedPtr( pMemory, 16 ) );

    //GLogger.Printf( "Allocated %u, Used %u\n", _SizeInBytes, FrameMemoryUsed );

    return pMemory;
}

size_t ARuntime::GetFrameMemorySize() const
{
    return FrameMemorySize;
}

size_t ARuntime::GetFrameMemoryUsed() const
{
    return FrameMemoryUsed;
}

size_t ARuntime::GetFrameMemoryUsedPrev() const
{
    return FrameMemoryUsedPrev;
}

size_t ARuntime::GetMaxFrameMemoryUsage() const
{
    return MaxFrameMemoryUsage;
}

SCPUInfo const & ARuntime::GetCPUInfo() const
{
    return CPUInfo;
}

#ifdef AN_OS_WIN32

struct SWaitableTimer {
    HANDLE Handle = nullptr;
    ~SWaitableTimer() {
        if ( Handle ) {
            CloseHandle( Handle );
        }
    }
};

static thread_local SWaitableTimer WaitableTimer;

//#include <thread>

static void WaitMicrosecondsWIN32( int _Microseconds )
{
#if 0
    std::this_thread::sleep_for( StdChrono::microseconds( _Microseconds ) );
#else
    LARGE_INTEGER WaitTime;

    WaitTime.QuadPart = -10 * _Microseconds;

    if ( !WaitableTimer.Handle ) {
        WaitableTimer.Handle = CreateWaitableTimer( NULL, TRUE, NULL );
    }

    SetWaitableTimer( WaitableTimer.Handle, &WaitTime, 0, NULL, NULL, FALSE );
    WaitForSingleObject( WaitableTimer.Handle, INFINITE );
#endif
}

#endif

void ARuntime::WaitSeconds( int _Seconds )
{
#ifdef AN_OS_WIN32
    //std::this_thread::sleep_for( StdChrono::seconds( _Seconds ) );
    WaitMicrosecondsWIN32( _Seconds * 1000000 );
#else
    struct timespec ts = { _Seconds, 0 };
    while ( ::nanosleep( &ts, &ts ) == -1 && errno == EINTR ) {}
#endif
}

void ARuntime::WaitMilliseconds( int _Milliseconds )
{
#ifdef AN_OS_WIN32
    //std::this_thread::sleep_for( StdChrono::milliseconds( _Milliseconds ) );
    WaitMicrosecondsWIN32( _Milliseconds * 1000 );
#else
    int64_t seconds = _Milliseconds / 1000;
    int64_t nanoseconds = ( _Milliseconds - seconds * 1000 ) * 1000000;
    struct timespec ts = {
        seconds,
        nanoseconds
    };
    while ( ::nanosleep( &ts, &ts ) == -1 && errno == EINTR ) {}
#endif
}

void ARuntime::WaitMicroseconds( int _Microseconds )
{
#ifdef AN_OS_WIN32
    //std::this_thread::sleep_for( StdChrono::microseconds( _Microseconds ) );
    WaitMicrosecondsWIN32( _Microseconds );
#else
    int64_t seconds = _Microseconds / 1000000;
    int64_t nanoseconds = ( _Microseconds - seconds * 1000000 ) * 1000;
    struct timespec ts = {
        seconds,
        nanoseconds
    };
    while ( ::nanosleep( &ts, &ts ) == -1 && errno == EINTR ) {}
#endif
}

int64_t ARuntime::SysSeconds()
{
    return StdChrono::duration_cast< StdChrono::seconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - StartSeconds;
}

double ARuntime::SysSeconds_d()
{
    return SysMicroseconds() * 0.000001;
}

int64_t ARuntime::SysMilliseconds()
{
    return StdChrono::duration_cast< StdChrono::milliseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - StartMilliseconds;
}

double ARuntime::SysMilliseconds_d()
{
    return SysMicroseconds() * 0.001;
}

int64_t ARuntime::SysMicroseconds()
{
    return StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - StartMicroseconds;
}

double ARuntime::SysMicroseconds_d()
{
    return static_cast< double >( SysMicroseconds() );
}

int64_t ARuntime::SysFrameTimeStamp()
{
    return FrameTimeStamp;
}

int64_t ARuntime::SysFrameDuration()
{
    return FrameDuration;
}

int ARuntime::SysFrameNumber() const
{
    return FrameNumber;
}

void * ARuntime::LoadDynamicLib( const char * _LibraryName )
{
    AString name( _LibraryName );
#if defined AN_OS_WIN32
    name.ReplaceExt( ".dll" );
#else
    name.ReplaceExt( ".so" );
#endif
    return SDL_LoadObject( name.CStr() );
}

void ARuntime::UnloadDynamicLib( void * _Handle )
{
    SDL_UnloadObject( _Handle );
}

void * ARuntime::GetProcAddress( void * _Handle, const char * _ProcName )
{
    return _Handle ? SDL_LoadFunction( _Handle, _ProcName ) : nullptr;
}

void ARuntime::SetClipboard( const char * _Utf8String )
{
    SDL_SetClipboardText( _Utf8String );
}

const char * ARuntime::GetClipboard()
{
    if ( Clipboard ) {
        SDL_free( Clipboard );
    }
    Clipboard = SDL_GetClipboardText();
    return Clipboard;
}

SVideoMode const & ARuntime::GetVideoMode() const
{
    return VideoMode;
}

void ARuntime::PostChangeVideoMode( SVideoMode const & _DesiredMode )
{
    DesiredMode = _DesiredMode;
    bPostChangeVideoMode = true;
}

void ARuntime::PostTerminateEvent()
{
    bPostTerminateEvent = true;
}

bool ARuntime::IsPendingTerminate()
{
    return bPostTerminateEvent;
}

#define FROM_SDL_TIMESTAMP(event) ( (event).timestamp * 0.001 )

struct SKeyMappingsSDL {
    unsigned short & operator[]( const int i ) {
        AN_ASSERT( i >= 0 && i < SDL_NUM_SCANCODES );
        return Table[i];
    }

    unsigned short Table[SDL_NUM_SCANCODES];
};

static SKeyMappingsSDL SDLKeyMappings;

static void InitKeyMappingsSDL()
{
    Core::ZeroMem( SDLKeyMappings.Table, sizeof( SDLKeyMappings.Table ) );

    SDLKeyMappings[ SDL_SCANCODE_A ] = KEY_A;
    SDLKeyMappings[ SDL_SCANCODE_B ] = KEY_B;
    SDLKeyMappings[ SDL_SCANCODE_C ] = KEY_C;
    SDLKeyMappings[ SDL_SCANCODE_D ] = KEY_D;
    SDLKeyMappings[ SDL_SCANCODE_E ] = KEY_E;
    SDLKeyMappings[ SDL_SCANCODE_F ] = KEY_F;
    SDLKeyMappings[ SDL_SCANCODE_G ] = KEY_G;
    SDLKeyMappings[ SDL_SCANCODE_H ] = KEY_H;
    SDLKeyMappings[ SDL_SCANCODE_I ] = KEY_I;
    SDLKeyMappings[ SDL_SCANCODE_J ] = KEY_J;
    SDLKeyMappings[ SDL_SCANCODE_K ] = KEY_K;
    SDLKeyMappings[ SDL_SCANCODE_L ] = KEY_L;
    SDLKeyMappings[ SDL_SCANCODE_M ] = KEY_M;
    SDLKeyMappings[ SDL_SCANCODE_N ] = KEY_N;
    SDLKeyMappings[ SDL_SCANCODE_O ] = KEY_O;
    SDLKeyMappings[ SDL_SCANCODE_P ] = KEY_P;
    SDLKeyMappings[ SDL_SCANCODE_Q ] = KEY_Q;
    SDLKeyMappings[ SDL_SCANCODE_R ] = KEY_R;
    SDLKeyMappings[ SDL_SCANCODE_S ] = KEY_S;
    SDLKeyMappings[ SDL_SCANCODE_T ] = KEY_T;
    SDLKeyMappings[ SDL_SCANCODE_U ] = KEY_U;
    SDLKeyMappings[ SDL_SCANCODE_V ] = KEY_V;
    SDLKeyMappings[ SDL_SCANCODE_W ] = KEY_W;
    SDLKeyMappings[ SDL_SCANCODE_X ] = KEY_X;
    SDLKeyMappings[ SDL_SCANCODE_Y ] = KEY_Y;
    SDLKeyMappings[ SDL_SCANCODE_Z ] = KEY_Z;
    SDLKeyMappings[ SDL_SCANCODE_1 ] = KEY_1;
    SDLKeyMappings[ SDL_SCANCODE_2 ] = KEY_2;
    SDLKeyMappings[ SDL_SCANCODE_3 ] = KEY_3;
    SDLKeyMappings[ SDL_SCANCODE_4 ] = KEY_4;
    SDLKeyMappings[ SDL_SCANCODE_5 ] = KEY_5;
    SDLKeyMappings[ SDL_SCANCODE_6 ] = KEY_6;
    SDLKeyMappings[ SDL_SCANCODE_7 ] = KEY_7;
    SDLKeyMappings[ SDL_SCANCODE_8 ] = KEY_8;
    SDLKeyMappings[ SDL_SCANCODE_9 ] = KEY_9;
    SDLKeyMappings[ SDL_SCANCODE_0 ] = KEY_0;
    SDLKeyMappings[ SDL_SCANCODE_RETURN ] = KEY_ENTER;
    SDLKeyMappings[ SDL_SCANCODE_ESCAPE ] = KEY_ESCAPE;
    SDLKeyMappings[ SDL_SCANCODE_BACKSPACE ] = KEY_BACKSPACE;
    SDLKeyMappings[ SDL_SCANCODE_TAB ] = KEY_TAB;
    SDLKeyMappings[ SDL_SCANCODE_SPACE ] = KEY_SPACE;
    SDLKeyMappings[ SDL_SCANCODE_MINUS ] = KEY_MINUS;
    SDLKeyMappings[ SDL_SCANCODE_EQUALS ] = KEY_EQUAL;
    SDLKeyMappings[ SDL_SCANCODE_LEFTBRACKET ] = KEY_LEFT_BRACKET;
    SDLKeyMappings[ SDL_SCANCODE_RIGHTBRACKET ] = KEY_RIGHT_BRACKET;
    SDLKeyMappings[ SDL_SCANCODE_BACKSLASH ] = KEY_BACKSLASH;
    //SDLKeyMappings[ SDL_SCANCODE_NONUSHASH ] = 50;
    SDLKeyMappings[ SDL_SCANCODE_SEMICOLON ] = KEY_SEMICOLON;
    SDLKeyMappings[ SDL_SCANCODE_APOSTROPHE ] = KEY_APOSTROPHE;
    SDLKeyMappings[ SDL_SCANCODE_GRAVE ] = KEY_GRAVE_ACCENT;
    SDLKeyMappings[ SDL_SCANCODE_COMMA ] = KEY_COMMA;
    SDLKeyMappings[ SDL_SCANCODE_PERIOD ] = KEY_PERIOD;
    SDLKeyMappings[ SDL_SCANCODE_SLASH ] = KEY_SLASH;
    SDLKeyMappings[ SDL_SCANCODE_CAPSLOCK ] = KEY_CAPS_LOCK;
    SDLKeyMappings[ SDL_SCANCODE_F1 ] = KEY_F1;
    SDLKeyMappings[ SDL_SCANCODE_F2 ] = KEY_F2;
    SDLKeyMappings[ SDL_SCANCODE_F3 ] = KEY_F3;
    SDLKeyMappings[ SDL_SCANCODE_F4 ] = KEY_F4;
    SDLKeyMappings[ SDL_SCANCODE_F5 ] = KEY_F5;
    SDLKeyMappings[ SDL_SCANCODE_F6 ] = KEY_F6;
    SDLKeyMappings[ SDL_SCANCODE_F7 ] = KEY_F7;
    SDLKeyMappings[ SDL_SCANCODE_F8 ] = KEY_F8;
    SDLKeyMappings[ SDL_SCANCODE_F9 ] = KEY_F9;
    SDLKeyMappings[ SDL_SCANCODE_F10 ] = KEY_F10;
    SDLKeyMappings[ SDL_SCANCODE_F11 ] = KEY_F11;
    SDLKeyMappings[ SDL_SCANCODE_F12 ] = KEY_F12;
    SDLKeyMappings[ SDL_SCANCODE_PRINTSCREEN ] = KEY_PRINT_SCREEN;
    SDLKeyMappings[ SDL_SCANCODE_SCROLLLOCK ] = KEY_SCROLL_LOCK;
    SDLKeyMappings[ SDL_SCANCODE_PAUSE ] = KEY_PAUSE;
    SDLKeyMappings[ SDL_SCANCODE_INSERT ] = KEY_INSERT;
    SDLKeyMappings[ SDL_SCANCODE_HOME ] = KEY_HOME;
    SDLKeyMappings[ SDL_SCANCODE_PAGEUP ] = KEY_PAGE_UP;
    SDLKeyMappings[ SDL_SCANCODE_DELETE ] = KEY_DELETE;
    SDLKeyMappings[ SDL_SCANCODE_END ] = KEY_END;
    SDLKeyMappings[ SDL_SCANCODE_PAGEDOWN ] = KEY_PAGE_DOWN;
    SDLKeyMappings[ SDL_SCANCODE_RIGHT ] = KEY_RIGHT;
    SDLKeyMappings[ SDL_SCANCODE_LEFT ] = KEY_LEFT;
    SDLKeyMappings[ SDL_SCANCODE_DOWN ] = KEY_DOWN;
    SDLKeyMappings[ SDL_SCANCODE_UP ] = KEY_UP;
    SDLKeyMappings[ SDL_SCANCODE_NUMLOCKCLEAR ] = KEY_NUM_LOCK;
    SDLKeyMappings[ SDL_SCANCODE_KP_DIVIDE ] = KEY_KP_DIVIDE;
    SDLKeyMappings[ SDL_SCANCODE_KP_MULTIPLY ] = KEY_KP_MULTIPLY;
    SDLKeyMappings[ SDL_SCANCODE_KP_MINUS ] = KEY_KP_SUBTRACT;
    SDLKeyMappings[ SDL_SCANCODE_KP_PLUS ] = KEY_KP_ADD;
    SDLKeyMappings[ SDL_SCANCODE_KP_ENTER ] = KEY_KP_ENTER;
    SDLKeyMappings[ SDL_SCANCODE_KP_1 ] = KEY_KP_1;
    SDLKeyMappings[ SDL_SCANCODE_KP_2 ] = KEY_KP_2;
    SDLKeyMappings[ SDL_SCANCODE_KP_3 ] = KEY_KP_3;
    SDLKeyMappings[ SDL_SCANCODE_KP_4 ] = KEY_KP_4;
    SDLKeyMappings[ SDL_SCANCODE_KP_5 ] = KEY_KP_5;
    SDLKeyMappings[ SDL_SCANCODE_KP_6 ] = KEY_KP_6;
    SDLKeyMappings[ SDL_SCANCODE_KP_7 ] = KEY_KP_7;
    SDLKeyMappings[ SDL_SCANCODE_KP_8 ] = KEY_KP_8;
    SDLKeyMappings[ SDL_SCANCODE_KP_9 ] = KEY_KP_9;
    SDLKeyMappings[ SDL_SCANCODE_KP_0 ] = KEY_KP_0;
    SDLKeyMappings[ SDL_SCANCODE_KP_PERIOD ] = KEY_KP_DECIMAL;
    //SDLKeyMappings[ SDL_SCANCODE_NONUSBACKSLASH ] = 100;
    //SDLKeyMappings[ SDL_SCANCODE_APPLICATION ] = 101;
    //SDLKeyMappings[ SDL_SCANCODE_POWER ] = 102;
    SDLKeyMappings[ SDL_SCANCODE_KP_EQUALS ] = KEY_KP_EQUAL;
    SDLKeyMappings[ SDL_SCANCODE_F13 ] = KEY_F13;
    SDLKeyMappings[ SDL_SCANCODE_F14 ] = KEY_F14;
    SDLKeyMappings[ SDL_SCANCODE_F15 ] = KEY_F15;
    SDLKeyMappings[ SDL_SCANCODE_F16 ] = KEY_F16;
    SDLKeyMappings[ SDL_SCANCODE_F17 ] = KEY_F17;
    SDLKeyMappings[ SDL_SCANCODE_F18 ] = KEY_F18;
    SDLKeyMappings[ SDL_SCANCODE_F19 ] = KEY_F19;
    SDLKeyMappings[ SDL_SCANCODE_F20 ] = KEY_F20;
    SDLKeyMappings[ SDL_SCANCODE_F21 ] = KEY_F21;
    SDLKeyMappings[ SDL_SCANCODE_F22 ] = KEY_F22;
    SDLKeyMappings[ SDL_SCANCODE_F23 ] = KEY_F23;
    SDLKeyMappings[ SDL_SCANCODE_F24 ] = KEY_F24;
    //SDLKeyMappings[ SDL_SCANCODE_EXECUTE ] = 116;
    //SDLKeyMappings[ SDL_SCANCODE_HELP ] = 117;
    SDLKeyMappings[ SDL_SCANCODE_MENU ] = KEY_MENU;
    //SDLKeyMappings[ SDL_SCANCODE_SELECT ] = 119;
    //SDLKeyMappings[ SDL_SCANCODE_STOP ] = 120;
    //SDLKeyMappings[ SDL_SCANCODE_AGAIN ] = 121;   /**< redo */
    //SDLKeyMappings[ SDL_SCANCODE_UNDO ] = 122;
    //SDLKeyMappings[ SDL_SCANCODE_CUT ] = 123;
    //SDLKeyMappings[ SDL_SCANCODE_COPY ] = 124;
    //SDLKeyMappings[ SDL_SCANCODE_PASTE ] = 125;
    //SDLKeyMappings[ SDL_SCANCODE_FIND ] = 126;
    //SDLKeyMappings[ SDL_SCANCODE_MUTE ] = 127;
    //SDLKeyMappings[ SDL_SCANCODE_VOLUMEUP ] = 128;
    //SDLKeyMappings[ SDL_SCANCODE_VOLUMEDOWN ] = 129;
    //SDLKeyMappings[ SDL_SCANCODE_KP_COMMA ] = 133;
    //SDLKeyMappings[ SDL_SCANCODE_KP_EQUALSAS400 ] = 134;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL1 ] = 135;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL2 ] = 136;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL3 ] = 137; /**< Yen */
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL4 ] = 138;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL5 ] = 139;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL6 ] = 140;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL7 ] = 141;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL8 ] = 142;
    //SDLKeyMappings[ SDL_SCANCODE_INTERNATIONAL9 ] = 143;
    //SDLKeyMappings[ SDL_SCANCODE_LANG1 ] = 144; /**< Hangul/English toggle */
    //SDLKeyMappings[ SDL_SCANCODE_LANG2 ] = 145; /**< Hanja conversion */
    //SDLKeyMappings[ SDL_SCANCODE_LANG3 ] = 146; /**< Katakana */
    //SDLKeyMappings[ SDL_SCANCODE_LANG4 ] = 147; /**< Hiragana */
    //SDLKeyMappings[ SDL_SCANCODE_LANG5 ] = 148; /**< Zenkaku/Hankaku */
    //SDLKeyMappings[ SDL_SCANCODE_LANG6 ] = 149; /**< reserved */
    //SDLKeyMappings[ SDL_SCANCODE_LANG7 ] = 150; /**< reserved */
    //SDLKeyMappings[ SDL_SCANCODE_LANG8 ] = 151; /**< reserved */
    //SDLKeyMappings[ SDL_SCANCODE_LANG9 ] = 152; /**< reserved */
    //SDLKeyMappings[ SDL_SCANCODE_ALTERASE ] = 153; /**< Erase-Eaze */
    //SDLKeyMappings[ SDL_SCANCODE_SYSREQ ] = 154;
    //SDLKeyMappings[ SDL_SCANCODE_CANCEL ] = 155;
    //SDLKeyMappings[ SDL_SCANCODE_CLEAR ] = 156;
    //SDLKeyMappings[ SDL_SCANCODE_PRIOR ] = 157;
    //SDLKeyMappings[ SDL_SCANCODE_RETURN2 ] = 158;
    //SDLKeyMappings[ SDL_SCANCODE_SEPARATOR ] = 159;
    //SDLKeyMappings[ SDL_SCANCODE_OUT ] = 160;
    //SDLKeyMappings[ SDL_SCANCODE_OPER ] = 161;
    //SDLKeyMappings[ SDL_SCANCODE_CLEARAGAIN ] = 162;
    //SDLKeyMappings[ SDL_SCANCODE_CRSEL ] = 163;
    //SDLKeyMappings[ SDL_SCANCODE_EXSEL ] = 164;
    //SDLKeyMappings[ SDL_SCANCODE_KP_00 ] = 176;
    //SDLKeyMappings[ SDL_SCANCODE_KP_000 ] = 177;
    //SDLKeyMappings[ SDL_SCANCODE_THOUSANDSSEPARATOR ] = 178;
    //SDLKeyMappings[ SDL_SCANCODE_DECIMALSEPARATOR ] = 179;
    //SDLKeyMappings[ SDL_SCANCODE_CURRENCYUNIT ] = 180;
    //SDLKeyMappings[ SDL_SCANCODE_CURRENCYSUBUNIT ] = 181;
    //SDLKeyMappings[ SDL_SCANCODE_KP_LEFTPAREN ] = 182;
    //SDLKeyMappings[ SDL_SCANCODE_KP_RIGHTPAREN ] = 183;
    //SDLKeyMappings[ SDL_SCANCODE_KP_LEFTBRACE ] = 184;
    //SDLKeyMappings[ SDL_SCANCODE_KP_RIGHTBRACE ] = 185;
    //SDLKeyMappings[ SDL_SCANCODE_KP_TAB ] = 186;
    //SDLKeyMappings[ SDL_SCANCODE_KP_BACKSPACE ] = 187;
    //SDLKeyMappings[ SDL_SCANCODE_KP_A ] = 188;
    //SDLKeyMappings[ SDL_SCANCODE_KP_B ] = 189;
    //SDLKeyMappings[ SDL_SCANCODE_KP_C ] = 190;
    //SDLKeyMappings[ SDL_SCANCODE_KP_D ] = 191;
    //SDLKeyMappings[ SDL_SCANCODE_KP_E ] = 192;
    //SDLKeyMappings[ SDL_SCANCODE_KP_F ] = 193;
    //SDLKeyMappings[ SDL_SCANCODE_KP_XOR ] = 194;
    //SDLKeyMappings[ SDL_SCANCODE_KP_POWER ] = 195;
    //SDLKeyMappings[ SDL_SCANCODE_KP_PERCENT ] = 196;
    //SDLKeyMappings[ SDL_SCANCODE_KP_LESS ] = 197;
    //SDLKeyMappings[ SDL_SCANCODE_KP_GREATER ] = 198;
    //SDLKeyMappings[ SDL_SCANCODE_KP_AMPERSAND ] = 199;
    //SDLKeyMappings[ SDL_SCANCODE_KP_DBLAMPERSAND ] = 200;
    //SDLKeyMappings[ SDL_SCANCODE_KP_VERTICALBAR ] = 201;
    //SDLKeyMappings[ SDL_SCANCODE_KP_DBLVERTICALBAR ] = 202;
    //SDLKeyMappings[ SDL_SCANCODE_KP_COLON ] = 203;
    //SDLKeyMappings[ SDL_SCANCODE_KP_HASH ] = 204;
    //SDLKeyMappings[ SDL_SCANCODE_KP_SPACE ] = 205;
    //SDLKeyMappings[ SDL_SCANCODE_KP_AT ] = 206;
    //SDLKeyMappings[ SDL_SCANCODE_KP_EXCLAM ] = 207;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMSTORE ] = 208;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMRECALL ] = 209;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMCLEAR ] = 210;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMADD ] = 211;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMSUBTRACT ] = 212;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMMULTIPLY ] = 213;
    //SDLKeyMappings[ SDL_SCANCODE_KP_MEMDIVIDE ] = 214;
    //SDLKeyMappings[ SDL_SCANCODE_KP_PLUSMINUS ] = 215;
    //SDLKeyMappings[ SDL_SCANCODE_KP_CLEAR ] = 216;
    //SDLKeyMappings[ SDL_SCANCODE_KP_CLEARENTRY ] = 217;
    //SDLKeyMappings[ SDL_SCANCODE_KP_BINARY ] = 218;
    //SDLKeyMappings[ SDL_SCANCODE_KP_OCTAL ] = 219;
    //SDLKeyMappings[ SDL_SCANCODE_KP_DECIMAL ] = 220;
    //SDLKeyMappings[ SDL_SCANCODE_KP_HEXADECIMAL ] = 221;
    SDLKeyMappings[ SDL_SCANCODE_LCTRL ] = KEY_LEFT_CONTROL;
    SDLKeyMappings[ SDL_SCANCODE_LSHIFT ] = KEY_LEFT_SHIFT;
    SDLKeyMappings[ SDL_SCANCODE_LALT ] = KEY_LEFT_ALT;
    SDLKeyMappings[ SDL_SCANCODE_LGUI ] = KEY_LEFT_SUPER;
    SDLKeyMappings[ SDL_SCANCODE_RCTRL ] = KEY_RIGHT_CONTROL;
    SDLKeyMappings[ SDL_SCANCODE_RSHIFT ] = KEY_RIGHT_SHIFT;
    SDLKeyMappings[ SDL_SCANCODE_RALT ] = KEY_RIGHT_ALT;
    SDLKeyMappings[ SDL_SCANCODE_RGUI ] = KEY_RIGHT_SUPER;
    //SDLKeyMappings[ SDL_SCANCODE_MODE ] = 257;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIONEXT ] = 258;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIOPREV ] = 259;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIOSTOP ] = 260;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIOPLAY ] = 261;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIOMUTE ] = 262;
    //SDLKeyMappings[ SDL_SCANCODE_MEDIASELECT ] = 263;
    //SDLKeyMappings[ SDL_SCANCODE_WWW ] = 264;
    //SDLKeyMappings[ SDL_SCANCODE_MAIL ] = 265;
    //SDLKeyMappings[ SDL_SCANCODE_CALCULATOR ] = 266;
    //SDLKeyMappings[ SDL_SCANCODE_COMPUTER ] = 267;
    //SDLKeyMappings[ SDL_SCANCODE_AC_SEARCH ] = 268;
    //SDLKeyMappings[ SDL_SCANCODE_AC_HOME ] = 269;
    //SDLKeyMappings[ SDL_SCANCODE_AC_BACK ] = 270;
    //SDLKeyMappings[ SDL_SCANCODE_AC_FORWARD ] = 271;
    //SDLKeyMappings[ SDL_SCANCODE_AC_STOP ] = 272;
    //SDLKeyMappings[ SDL_SCANCODE_AC_REFRESH ] = 273;
    //SDLKeyMappings[ SDL_SCANCODE_AC_BOOKMARKS ] = 274;
    //SDLKeyMappings[ SDL_SCANCODE_BRIGHTNESSDOWN ] = 275;
    //SDLKeyMappings[ SDL_SCANCODE_BRIGHTNESSUP ] = 276;
    //SDLKeyMappings[ SDL_SCANCODE_DISPLAYSWITCH ] = 277; 
    //SDLKeyMappings[ SDL_SCANCODE_KBDILLUMTOGGLE ] = 278;
    //SDLKeyMappings[ SDL_SCANCODE_KBDILLUMDOWN ] = 279;
    //SDLKeyMappings[ SDL_SCANCODE_KBDILLUMUP ] = 280;
    //SDLKeyMappings[ SDL_SCANCODE_EJECT ] = 281;
    //SDLKeyMappings[ SDL_SCANCODE_SLEEP ] = 282;
    //SDLKeyMappings[ SDL_SCANCODE_APP1 ] = 283;
    //SDLKeyMappings[ SDL_SCANCODE_APP2 ] = 284;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIOREWIND ] = 285;
    //SDLKeyMappings[ SDL_SCANCODE_AUDIOFASTFORWARD ] = 286;
}

static AN_FORCEINLINE int FromKeymodSDL( Uint16 Mod )
{
    int modMask = 0;

    if ( Mod & (KMOD_LSHIFT|KMOD_RSHIFT) ) {
        modMask |= KMOD_MASK_SHIFT;
    }

    if ( Mod & (KMOD_LCTRL|KMOD_RCTRL) ) {
        modMask |= KMOD_MASK_CONTROL;
    }

    if ( Mod & (KMOD_LALT|KMOD_RALT) ) {
        modMask |= KMOD_MASK_ALT;
    }

    if ( Mod & (KMOD_LGUI|KMOD_RGUI) ) {
        modMask |= KMOD_MASK_SUPER;
    }

    return modMask;
}

static AN_FORCEINLINE int FromKeymodSDL_Char( Uint16 Mod )
{
    int modMask = 0;

    if ( Mod & (KMOD_LSHIFT|KMOD_RSHIFT) ) {
        modMask |= KMOD_MASK_SHIFT;
    }

    if ( Mod & (KMOD_LCTRL|KMOD_RCTRL) ) {
        modMask |= KMOD_MASK_CONTROL;
    }

    if ( Mod & (KMOD_LALT|KMOD_RALT) ) {
        modMask |= KMOD_MASK_ALT;
    }

    if ( Mod & (KMOD_LGUI|KMOD_RGUI) ) {
        modMask |= KMOD_MASK_SUPER;
    }

    if ( Mod & KMOD_CAPS ) {
        modMask |= KMOD_MASK_CAPS_LOCK;
    }

    if ( Mod & KMOD_NUM ) {
        modMask |= KMOD_MASK_NUM_LOCK;
    }

    return modMask;
}

void ARuntime::UnpressJoystickButtons( int _JoystickNum, double _TimeStamp )
{
    SJoystickButtonEvent buttonEvent;
    buttonEvent.Joystick = _JoystickNum;
    buttonEvent.Action = IA_RELEASE;
    for ( int i = 0 ; i < MAX_JOYSTICK_BUTTONS ; i++ ) {
        if ( JoystickButtonState[_JoystickNum][i] ) {
            JoystickButtonState[_JoystickNum][i] = SDL_RELEASED;
            buttonEvent.Button = JOY_BUTTON_1 + i;
            Engine->OnJoystickButtonEvent( buttonEvent, _TimeStamp );
        }
    }
}

void ARuntime::ClearJoystickAxes( int _JoystickNum, double _TimeStamp )
{
    SJoystickAxisEvent axisEvent;
    axisEvent.Joystick = _JoystickNum;
    axisEvent.Value = 0;
    for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
        if ( JoystickAxisState[_JoystickNum][i] != 0 ) {
            JoystickAxisState[_JoystickNum][i] = 0;
            axisEvent.Axis = JOY_AXIS_1 + i;
            Engine->OnJoystickAxisEvent( axisEvent, _TimeStamp );
        }
    }
}

void ARuntime::UnpressKeysAndButtons()
{
    SKeyEvent keyEvent;
    SMouseButtonEvent mouseEvent;

    keyEvent.Action = IA_RELEASE;
    keyEvent.ModMask = 0;

    mouseEvent.Action = IA_RELEASE;
    mouseEvent.ModMask = 0;

    double timeStamp = GRuntime.SysSeconds_d();

    for ( int i = 0 ; i <= KEY_LAST ; i++ ) {
        if ( PressedKeys[i] ) {
            keyEvent.Key = i;
            keyEvent.Scancode = PressedKeys[i] - 1;

            PressedKeys[i] = 0;

            Engine->OnKeyEvent( keyEvent, timeStamp );
        }
    }
    for ( int i = MOUSE_BUTTON_1 ; i <= MOUSE_BUTTON_8 ; i++ ) {
        if ( PressedMouseButtons[i] ) {
            mouseEvent.Button = i;

            PressedMouseButtons[i] = 0;

            Engine->OnMouseButtonEvent( mouseEvent, timeStamp );
        }
    }

    for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        UnpressJoystickButtons( i, timeStamp );
        ClearJoystickAxes( i, timeStamp );
    }
}

void ARuntime::NewFrame()
{
    GPUSync->SetEvent();

    // Swap buffers for streamed memory
    StreamedMemoryGPU->Swap();

    // Swap window
    RenderDevice->SwapBuffers( WindowHandle, rt_SwapInterval.GetInteger() );

    // Wait for free streamed buffer
    StreamedMemoryGPU->Wait();

    int64_t prevTimeStamp = FrameTimeStamp;
    FrameTimeStamp = SysMicroseconds();
    if ( prevTimeStamp == StartMicroseconds ) {
        // First frame
        FrameDuration = 1000000.0 / 60;
    } else {
        FrameDuration = FrameTimeStamp - prevTimeStamp;
    }

    FrameNumber++;

    // Keep memory statistics
    MaxFrameMemoryUsage = Math::Max( MaxFrameMemoryUsage, FrameMemoryUsed );
    FrameMemoryUsedPrev = FrameMemoryUsed;

    // Free frame memory for new frame
    FrameMemoryUsed = 0;

    if ( bPostChangeVideoMode ) {
        bPostChangeVideoMode = false;
        SetVideoMode( DesiredMode );
    }
}

void ARuntime::PollEvents()
{
    // NOTE: Workaround of SDL bug with false mouse motion when a window gain keyboard focus.
    static bool bIgnoreFalseMouseMotionHack = false;

    // Sync with GPU to prevent "input lag"
    if ( rt_SyncGPU ) {
        GPUSync->Wait();
    }

    SDL_Event event;
    while ( SDL_PollEvent( &event ) ) {

        switch ( event.type ) {

        // User-requested quit
        case SDL_QUIT:
            Engine->OnCloseEvent();
            break;

        // The application is being terminated by the OS
        // Called on iOS in applicationWillTerminate()
        // Called on Android in onDestroy()
        case SDL_APP_TERMINATING:
            GLogger.Printf( "PollEvent: Terminating\n" );
            break;

        // The application is low on memory, free memory if possible.
        // Called on iOS in applicationDidReceiveMemoryWarning()
        // Called on Android in onLowMemory()
        case SDL_APP_LOWMEMORY:
            GLogger.Printf( "PollEvent: Low memory\n" );
            break;

        // The application is about to enter the background
        // Called on iOS in applicationWillResignActive()
        // Called on Android in onPause()
        case SDL_APP_WILLENTERBACKGROUND:
            GLogger.Printf( "PollEvent: Will enter background\n" );
            break;

        // The application did enter the background and may not get CPU for some time
        // Called on iOS in applicationDidEnterBackground()
        // Called on Android in onPause()
        case SDL_APP_DIDENTERBACKGROUND:
            GLogger.Printf( "PollEvent: Did enter background\n" );
            break;

        // The application is about to enter the foreground
        // Called on iOS in applicationWillEnterForeground()
        // Called on Android in onResume()
        case SDL_APP_WILLENTERFOREGROUND:
            GLogger.Printf( "PollEvent: Will enter foreground\n" );
            break;

        // The application is now interactive
        // Called on iOS in applicationDidBecomeActive()
        // Called on Android in onResume()
        case SDL_APP_DIDENTERFOREGROUND:
            GLogger.Printf( "PollEvent: Did enter foreground\n" );
            break;

        // Display state change
        case SDL_DISPLAYEVENT:
            switch ( event.display.event ) {
            // Display orientation has changed to data1
            case SDL_DISPLAYEVENT_ORIENTATION:
                switch ( event.display.data1 ) {
                // The display is in landscape mode, with the right side up, relative to portrait mode
                case SDL_ORIENTATION_LANDSCAPE:
                    GLogger.Printf( "PollEvent: Display orientation has changed to landscape mode\n" );
                    break;
                // The display is in landscape mode, with the left side up, relative to portrait mode
                case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
                    GLogger.Printf( "PollEvent: Display orientation has changed to flipped landscape mode\n" );
                    break;
                // The display is in portrait mode
                case SDL_ORIENTATION_PORTRAIT:
                    GLogger.Printf( "PollEvent: Display orientation has changed to portrait mode\n" );
                    break;
                // The display is in portrait mode, upside down
                case SDL_ORIENTATION_PORTRAIT_FLIPPED:
                    GLogger.Printf( "PollEvent: Display orientation has changed to flipped portrait mode\n" );
                    break;
                // The display orientation can't be determined
                case SDL_ORIENTATION_UNKNOWN:
                default:
                    GLogger.Printf( "PollEvent: The display orientation can't be determined\n" );
                    break;
                }
            default:
                GLogger.Printf( "PollEvent: Unknown display event type\n" );
                break;
            }
            break;

        // Window state change
        case SDL_WINDOWEVENT: {
            switch ( event.window.event ) {
            // Window has been shown
            case SDL_WINDOWEVENT_SHOWN:
                Engine->OnWindowVisible( true );
                break;
            // Window has been hidden
            case SDL_WINDOWEVENT_HIDDEN:
                Engine->OnWindowVisible( false );
                break;
            // Window has been exposed and should be redrawn
            case SDL_WINDOWEVENT_EXPOSED:
                break;
            // Window has been moved to data1, data2
            case SDL_WINDOWEVENT_MOVED:
                VideoMode.DisplayIndex = SDL_GetWindowDisplayIndex( SDL_GetWindowFromID( event.window.windowID ) );
                VideoMode.X = event.window.data1;
                VideoMode.Y = event.window.data2;
                if ( !VideoMode.bFullscreen ) {
                    VideoMode.WindowedX = event.window.data1;
                    VideoMode.WindowedY = event.window.data2;
                }
                break;
            // Window has been resized to data1xdata2
            case SDL_WINDOWEVENT_RESIZED:
            // The window size has changed, either as
            // a result of an API call or through the
            // system or user changing the window size.
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                SDL_Window * wnd = SDL_GetWindowFromID( event.window.windowID );
                VideoMode.Width = event.window.data1;
                VideoMode.Height = event.window.data2;
                VideoMode.DisplayIndex = SDL_GetWindowDisplayIndex( wnd );
                SDL_GL_GetDrawableSize( wnd, &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight );
                if ( VideoMode.bFullscreen ) {
                    SDL_DisplayMode mode;
                    SDL_GetDesktopDisplayMode( VideoMode.DisplayIndex, &mode );
                    float sx = (float)mode.w / VideoMode.FramebufferWidth;
                    float sy = (float)mode.h / VideoMode.FramebufferHeight;
                    VideoMode.AspectScale = (sx / sy);
                } else {
                    VideoMode.AspectScale = 1;
                }
                Engine->OnResize();
                break;
            }
            // Window has been minimized
            case SDL_WINDOWEVENT_MINIMIZED:
                Engine->OnWindowVisible( false );
                break;
            // Window has been maximized
            case SDL_WINDOWEVENT_MAXIMIZED:
                break;
            // Window has been restored to normal size and position
            case SDL_WINDOWEVENT_RESTORED:
                Engine->OnWindowVisible( true );
                break;
            // Window has gained mouse focus
            case SDL_WINDOWEVENT_ENTER:
                break;
            // Window has lost mouse focus
            case SDL_WINDOWEVENT_LEAVE:
                break;
            // Window has gained keyboard focus
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                bIgnoreFalseMouseMotionHack = true;
                break;
            // Window has lost keyboard focus
            case SDL_WINDOWEVENT_FOCUS_LOST:
                UnpressKeysAndButtons();
                break;
            // The window manager requests that the window be closed
            case SDL_WINDOWEVENT_CLOSE:
                break;
            // Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore)
            case SDL_WINDOWEVENT_TAKE_FOCUS:
                break;
            // Window had a hit test that wasn't SDL_HITTEST_NORMAL.
            case SDL_WINDOWEVENT_HIT_TEST:
                break;
            }
            break;
        }

        // System specific event
        case SDL_SYSWMEVENT:
            break;

        // Key pressed/released
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            SKeyEvent keyEvent;
            keyEvent.Key = SDLKeyMappings[ event.key.keysym.scancode ];
            keyEvent.Scancode = event.key.keysym.scancode;
            keyEvent.Action = ( event.type == SDL_KEYDOWN ) ? ( PressedKeys[keyEvent.Key] ? IA_REPEAT : IA_PRESS ) : IA_RELEASE;
            keyEvent.ModMask = FromKeymodSDL( event.key.keysym.mod );
            if ( keyEvent.Key ) {
                if ( ( keyEvent.Action == IA_RELEASE && !PressedKeys[keyEvent.Key] )
                     || ( keyEvent.Action == IA_PRESS && PressedKeys[keyEvent.Key] ) ) {

                    // State does not changed
                } else {
                    PressedKeys[keyEvent.Key] = ( keyEvent.Action == IA_RELEASE ) ? 0 : keyEvent.Scancode + 1;

                    Engine->OnKeyEvent( keyEvent, FROM_SDL_TIMESTAMP(event.key) );
                }
            }
            break;
        }

        // Keyboard text editing (composition)
        case SDL_TEXTEDITING:
            break;

        // Keyboard text input
        case SDL_TEXTINPUT: {
            SCharEvent charEvent;
            charEvent.ModMask = FromKeymodSDL_Char( SDL_GetModState() );

            const char * unicode = event.text.text;
            while ( *unicode ) {
                const int byteLen = Core::WideCharDecodeUTF8( unicode, charEvent.UnicodeCharacter );
                if ( !byteLen ) {
                    break;
                }
                unicode += byteLen;
                Engine->OnCharEvent( charEvent, FROM_SDL_TIMESTAMP(event.text) );
            }
            break;
        }

        // Keymap changed due to a system event such as an
        // input language or keyboard layout change.
        case SDL_KEYMAPCHANGED:
            break;

        // Mouse moved
        case SDL_MOUSEMOTION: {
            if ( !bIgnoreFalseMouseMotionHack ) {
                SMouseMoveEvent moveEvent;
                moveEvent.X = event.motion.xrel;
                moveEvent.Y = -event.motion.yrel;
                Engine->OnMouseMoveEvent( moveEvent, FROM_SDL_TIMESTAMP(event.motion) );
            }
            bIgnoreFalseMouseMotionHack = false;
            break;
        }

        // Mouse button pressed
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            SMouseButtonEvent mouseEvent;
            switch ( event.button.button ) {
            case 2:
                mouseEvent.Button = MOUSE_BUTTON_3;
                break;
            case 3:
                mouseEvent.Button = MOUSE_BUTTON_2;
                break;
            default:
                mouseEvent.Button = MOUSE_BUTTON_1 + event.button.button - 1;
                break;
            }
            mouseEvent.Action = event.type == SDL_MOUSEBUTTONDOWN ? IA_PRESS : IA_RELEASE;
            mouseEvent.ModMask = FromKeymodSDL( SDL_GetModState() );

            if ( mouseEvent.Button >= MOUSE_BUTTON_1 && mouseEvent.Button <= MOUSE_BUTTON_8 ) {
                if ( mouseEvent.Action == (int)PressedMouseButtons[mouseEvent.Button] ) {

                    // State does not changed
                } else {
                    PressedMouseButtons[mouseEvent.Button] = mouseEvent.Action != IA_RELEASE;

                    Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.button) );
                }
            }
            break;
        }

        // Mouse wheel motion
        case SDL_MOUSEWHEEL: {
            SMouseWheelEvent wheelEvent;
            wheelEvent.WheelX = event.wheel.x;
            wheelEvent.WheelY = event.wheel.y;
            Engine->OnMouseWheelEvent( wheelEvent, FROM_SDL_TIMESTAMP(event.wheel) );

            SMouseButtonEvent mouseEvent;
            mouseEvent.ModMask = FromKeymodSDL( SDL_GetModState() );

            if ( wheelEvent.WheelX < 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_LEFT;

                mouseEvent.Action = IA_PRESS;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_RELEASE;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

            } else if ( wheelEvent.WheelX > 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_RIGHT;

                mouseEvent.Action = IA_PRESS;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_RELEASE;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );
            }
            if ( wheelEvent.WheelY < 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_DOWN;

                mouseEvent.Action = IA_PRESS;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_RELEASE;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );
            } else if ( wheelEvent.WheelY > 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_UP;

                mouseEvent.Action = IA_PRESS;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_RELEASE;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );
            }
            break;
        }

        // Joystick axis motion
        case SDL_JOYAXISMOTION: {
            if ( event.jaxis.which >= 0 && event.jaxis.which < MAX_JOYSTICKS_COUNT ) {
                AN_ASSERT( JoystickAdded[event.jaxis.which] );
                if ( event.jaxis.axis >= 0 && event.jaxis.axis < MAX_JOYSTICK_AXES ) {
                    if ( JoystickAxisState[event.jaxis.which][event.jaxis.axis] != event.jaxis.value ) {
                        SJoystickAxisEvent axisEvent;
                        axisEvent.Joystick = event.jaxis.which;
                        axisEvent.Axis = JOY_AXIS_1 + event.jaxis.axis;
                        axisEvent.Value = ( (float)event.jaxis.value + 32768.0f ) / 0xffff * 2.0f - 1.0f; // scale to -1.0f ... 1.0f

                        Engine->OnJoystickAxisEvent( axisEvent, FROM_SDL_TIMESTAMP(event.jaxis) );
                    }
                } else {
                    AN_ASSERT_( 0, "Invalid joystick axis num" );
                }
            } else {
                AN_ASSERT_( 0, "Invalid joystick id" );
            }
            break;
        }

        // Joystick trackball motion
        case SDL_JOYBALLMOTION:
            GLogger.Printf( "PollEvent: Joystick ball move\n" );
            break;

        // Joystick hat position change
        case SDL_JOYHATMOTION:
            GLogger.Printf( "PollEvent: Joystick hat move\n" );
            break;

        // Joystick button pressed
        case SDL_JOYBUTTONDOWN:
        // Joystick button released
        case SDL_JOYBUTTONUP: {
            if ( event.jbutton.which >= 0 && event.jbutton.which < MAX_JOYSTICKS_COUNT ) {
                AN_ASSERT( JoystickAdded[event.jbutton.which] );
                if ( event.jbutton.button >= 0 && event.jbutton.button < MAX_JOYSTICK_BUTTONS ) {
                    if ( JoystickButtonState[event.jbutton.which][event.jbutton.button] != event.jbutton.state ) {
                        JoystickButtonState[event.jbutton.which][event.jbutton.button] = event.jbutton.state;
                        SJoystickButtonEvent buttonEvent;
                        buttonEvent.Joystick = event.jbutton.which;
                        buttonEvent.Button = JOY_BUTTON_1 + event.jbutton.button;
                        buttonEvent.Action = event.jbutton.state == SDL_PRESSED ? IA_PRESS : IA_RELEASE;
                        Engine->OnJoystickButtonEvent( buttonEvent, FROM_SDL_TIMESTAMP(event.jbutton) );
                    }
                } else {
                    AN_ASSERT_( 0, "Invalid joystick button num" );
                }
            } else {
                AN_ASSERT_( 0, "Invalid joystick id" );
            }
            break;
        }

        // A new joystick has been inserted into the system
        case SDL_JOYDEVICEADDED:
            if ( event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT ) {
                AN_ASSERT( !JoystickAdded[event.jdevice.which] );
                JoystickAdded[event.jdevice.which] = true;

                Core::ZeroMem( JoystickButtonState[event.jdevice.which], sizeof( JoystickButtonState[0] ) );
                Core::ZeroMem( JoystickAxisState[event.jdevice.which], sizeof( JoystickAxisState[0] ) );
            } else {
                AN_ASSERT_( 0, "Invalid joystick id" );
            }
            GLogger.Printf( "PollEvent: Joystick added\n" );
            break;

        // An opened joystick has been removed
        case SDL_JOYDEVICEREMOVED: {
            if ( event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT ) {
                double timeStamp = FROM_SDL_TIMESTAMP(event.jdevice);

                UnpressJoystickButtons( event.jdevice.which, timeStamp );
                ClearJoystickAxes( event.jdevice.which, timeStamp );

                AN_ASSERT( JoystickAdded[event.jdevice.which] );
                JoystickAdded[event.jdevice.which] = false;
            } else {
                AN_ASSERT_( 0, "Invalid joystick id" );
            }

            GLogger.Printf( "PollEvent: Joystick removed\n" );
            break;
        }

        // Game controller axis motion
        case SDL_CONTROLLERAXISMOTION:
            GLogger.Printf( "PollEvent: Gamepad axis move\n" );
            break;
        // Game controller button pressed
        case SDL_CONTROLLERBUTTONDOWN:
            GLogger.Printf( "PollEvent: Gamepad button press\n" );
            break;
        // Game controller button released
        case SDL_CONTROLLERBUTTONUP:
            GLogger.Printf( "PollEvent: Gamepad button release\n" );
            break;
        // A new Game controller has been inserted into the system
        case SDL_CONTROLLERDEVICEADDED:
            GLogger.Printf( "PollEvent: Gamepad added\n" );
            break;
        // An opened Game controller has been removed
        case SDL_CONTROLLERDEVICEREMOVED:
            GLogger.Printf( "PollEvent: Gamepad removed\n" );
            break;
        // The controller mapping was updated
        case SDL_CONTROLLERDEVICEREMAPPED:
            GLogger.Printf( "PollEvent: Gamepad device mapped\n" );
            break;

        // Touch events
        case SDL_FINGERDOWN:
            GLogger.Printf( "PollEvent: Touch press\n" );
            break;
        case SDL_FINGERUP:
            GLogger.Printf( "PollEvent: Touch release\n" );
            break;
        case SDL_FINGERMOTION:
            GLogger.Printf( "PollEvent: Touch move\n" );
            break;

        // Gesture events
        case SDL_DOLLARGESTURE:
            GLogger.Printf( "PollEvent: Dollar gesture\n" );
            break;
        case SDL_DOLLARRECORD:
            GLogger.Printf( "PollEvent: Dollar record\n" );
            break;
        case SDL_MULTIGESTURE:
            GLogger.Printf( "PollEvent: Multigesture\n" );
            break;

        // The clipboard changed
        case SDL_CLIPBOARDUPDATE:
            GLogger.Printf( "PollEvent: Clipboard update\n" );
            break;

        // The system requests a file open
        case SDL_DROPFILE:
            GLogger.Printf( "PollEvent: Drop file\n" );
            break;
        // text/plain drag-and-drop event
        case SDL_DROPTEXT:
            GLogger.Printf( "PollEvent: Drop text\n" );
            break;
        // A new set of drops is beginning (NULL filename)
        case SDL_DROPBEGIN:
            GLogger.Printf( "PollEvent: Drop begin\n" );
            break;
        // Current set of drops is now complete (NULL filename)
        case SDL_DROPCOMPLETE:
            GLogger.Printf( "PollEvent: Drop complete\n" );
            break;

        // A new audio device is available
        case SDL_AUDIODEVICEADDED:
            GLogger.Printf( "PollEvent: Audio device added\n" );
            break;
        // An audio device has been removed
        case SDL_AUDIODEVICEREMOVED:
            GLogger.Printf( "PollEvent: Audio device removed\n" );
            break;

        // A sensor was updated
        case SDL_SENSORUPDATE:
            GLogger.Printf( "PollEvent: Sensor update\n" );
            break;

        // The render targets have been reset and their contents need to be updated
        case SDL_RENDER_TARGETS_RESET:
            GLogger.Printf( "PollEvent: Render targets reset\n" );
            break;
        case SDL_RENDER_DEVICE_RESET:
            GLogger.Printf( "PollEvent: Render device reset\n" );
            break;
        }
    }
}

static void TestDisplays()
{
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    int displayCount = SDL_GetNumVideoDisplays();
    SDL_Rect rect;
    SDL_Rect usableRect;
    for ( int i = 0 ; i < displayCount ; i++ ) {
        const char * name = SDL_GetDisplayName( i );

        SDL_GetDisplayBounds( i, &rect );
        SDL_GetDisplayUsableBounds( i, &usableRect );
        SDL_DisplayOrientation orient = SDL_GetDisplayOrientation( i );

        const char * orientName;
        switch ( orient ) {
        case SDL_ORIENTATION_LANDSCAPE:
            orientName = "Landscape";
            break;
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            orientName = "Landscape (Flipped)";
            break;
        case SDL_ORIENTATION_PORTRAIT:
            orientName = "Portrait";
            break;
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            orientName = "Portrait (Flipped)";
            break;
        case SDL_ORIENTATION_UNKNOWN:
        default:
            orientName = "Undetermined";
            break;
        }

        GLogger.Printf( "Found display %s (%d %d %d %d, usable %d %d %d %d) %s\n", name, rect.x, rect.y, rect.w, rect.h, usableRect.x, usableRect.y, usableRect.w, usableRect.h, orientName );

        int numModes = SDL_GetNumDisplayModes( i );
        SDL_DisplayMode mode;
        for ( int m = 0 ; m < numModes ; m++ ) {
            SDL_GetDisplayMode( i, m, &mode );

            if ( mode.format == SDL_PIXELFORMAT_RGB888 ) {
                GLogger.Printf( "Mode %d: %d %d %d hz\n", m, mode.w, mode.h, mode.refresh_rate );
            }
        }
    }

    //SDL_GetDesktopDisplayMode
    //SDL_GetCurrentDisplayMode
    //SDL_GetClosestDisplayMode
}

void ARuntime::SetVideoMode( SVideoMode const & _DesiredMode )
{
    Core::Memcpy( &VideoMode, &_DesiredMode, sizeof( VideoMode ) );

    SDL_Window * wnd = WindowHandle;

    // Set refresh rate
    //SDL_DisplayMode mode = {};
    //mode.format = SDL_PIXELFORMAT_RGB888;
    //mode.w = _DesiredMode.Width;
    //mode.h = _DesiredMode.Height;
    //mode.refresh_rate = _DesiredMode.RefreshRate;
    //SDL_SetWindowDisplayMode( wnd, &mode );

    SDL_SetWindowFullscreen( wnd, _DesiredMode.bFullscreen ? SDL_WINDOW_FULLSCREEN : 0 );
    SDL_SetWindowSize( wnd, _DesiredMode.Width, _DesiredMode.Height );
    if ( !_DesiredMode.bFullscreen ) {
        if ( _DesiredMode.bCentrized ) {
            SDL_SetWindowPosition( wnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );
        } else {
            SDL_SetWindowPosition( wnd, _DesiredMode.WindowedX, _DesiredMode.WindowedY );
        }
    }
    //SDL_SetWindowDisplayMode( wnd, &mode );
    SDL_GL_GetDrawableSize( wnd, &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight );

    VideoMode.bFullscreen = !!(SDL_GetWindowFlags( wnd ) & SDL_WINDOW_FULLSCREEN);

    VideoMode.Opacity = Math::Clamp( VideoMode.Opacity, 0.0f, 1.0f );

    float opacity = 1.0f;
    SDL_GetWindowOpacity( wnd, &opacity );
    if ( Math::Abs( VideoMode.Opacity - opacity ) > 1.0f/255.0f ) {
        SDL_SetWindowOpacity( wnd, VideoMode.Opacity );
    }

    VideoMode.DisplayIndex = SDL_GetWindowDisplayIndex( wnd );
    /*if ( VideoMode.bFullscreen ) {
        const float MM_To_Inch = 0.0393701f;
        VideoMode.DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        VideoMode.DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);

    } else */ {
        SDL_GetDisplayDPI( VideoMode.DisplayIndex, NULL, &VideoMode.DPI_X, &VideoMode.DPI_Y );
    }

    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode( wnd, &mode );
    VideoMode.RefreshRate = mode.refresh_rate;

    // Swap buffers to prevent flickering
    RenderDevice->SwapBuffers( WindowHandle, rt_SwapInterval.GetInteger() );
}

void ARuntime::SetCursorEnabled( bool _Enabled )
{
    //SDL_SetWindowGrab( Wnd, (SDL_bool)!_Enabled ); // FIXME: Always grab in fullscreen?
    SDL_SetRelativeMouseMode( (SDL_bool)!_Enabled );
}

bool ARuntime::IsCursorEnabled()
{
    return !SDL_GetRelativeMouseMode();
}

void ARuntime::GetCursorPosition( int * _X, int * _Y )
{
    (void)SDL_GetMouseState( _X, _Y );
}

RenderCore::IDevice * ARuntime::GetRenderDevice()
{
    return RenderDevice;
}

void ARuntime::ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem )
{
    RenderCore::SRect2D rect;
    rect.X = _X;
    rect.Y = _Y;
    rect.Width = _Width;
    rect.Height = _Height;

    RenderCore::IFramebuffer * framebuffer = pImmediateContext->GetDefaultFramebuffer();

    framebuffer->Read( RenderCore::FB_BACK_DEFAULT, rect, RenderCore::FB_CHANNEL_RGBA, RenderCore::FB_UBYTE, RenderCore::COLOR_CLAMP_ON, _SizeInBytes, _Alignment, _SysMem );
}
