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

#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/EngineInterface.h>
#include <Runtime/Public/InputDefs.h>
#include <Runtime/Public/VertexMemoryGPU.h>

#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/HashFunc.h>
#include <Core/Public/WindowsDefs.h>

#include "CPUInfo.h"

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

#define PROCESS_COULDNT_CHECK_UNIQUE 1
#define PROCESS_ALREADY_EXISTS       2
#define PROCESS_UNIQUE               3

ARuntime & GRuntime = ARuntime::Inst();

AAsyncJobManager GAsyncJobManager;
AAsyncJobList * GRenderFrontendJobList;
AAsyncJobList * GRenderBackendJobList;

static int PressedKeys[KEY_LAST+1];
static bool PressedMouseButtons[MOUSE_BUTTON_8+1];
static unsigned char JoystickButtonState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];
static short JoystickAxisState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];
static bool JoystickAdded[ MAX_JOYSTICKS_COUNT ];

static void InitKeyMappingsSDL();
static void LoggerMessageCallback( int _Level, const char * _Message );

ARuntime::ARuntime() {
    NumArguments = 0;
    Arguments = nullptr;
    Executable = nullptr;
    FrameMemoryAddress = nullptr;
    FrameMemorySize = 0;
    FrameMemoryUsed = 0;
    FrameMemoryUsedPrev = 0;
    MaxFrameMemoryUsage = 0;
    bTerminate = false;
    Clipboard = nullptr;
    ProcessAttribute = 0;
}

void ARuntime::Run( ACreateGameModuleCallback _CreateGameModule ) {

    // Synchronize SDL ticks with our start time
    (void)SDL_GetTicks();

    StartMicroseconds = StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count();
    StartMilliseconds = StartMicroseconds * 0.001;
    StartSeconds = StartMicroseconds * 0.000001;
    FrameTimeStamp = StartMicroseconds;
    FrameDuration = 1000000.0 / 60;
    FrameNumber = 0;

    CreateGameModuleCallback = _CreateGameModule;

    Engine = GetEngineInstance();

    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        EmergencyExit();
    }

    ::GetCPUInfo( CPUInfo );

    InitializeProcess();

    GLogger.SetMessageCallback( LoggerMessageCallback );

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
    GLogger.Printf( "Executable: %s\n", Executable );

    GLogger.Printf( "Привет, Вася\n" );

    SDL_LogSetOutputFunction(
                [](void *userdata, int category, SDL_LogPriority priority, const char *message) {
                    GLogger.Printf( "SDL: %d : %s\n", category, message );
                },
                NULL );

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

#if 0
    GMonitorManager.Initialize();
#endif

    // TODO: load this from config:
    SVideoMode desiredMode = {};
    desiredMode.Width = 640;
    desiredMode.Height = 480;
    desiredMode.RefreshRate = 120;
    desiredMode.Opacity = 1;
    desiredMode.bFullscreen = false;
    desiredMode.bCentrized = true;
    Core::Strcpy( desiredMode.Backend, sizeof( desiredMode.Backend ), "OpenGL 4.5" );
    Core::Strcpy( desiredMode.Title, sizeof( desiredMode.Title ), "Game" );

    InitializeRenderer( desiredMode );

    Engine->Run( CreateGameModuleCallback );

    ARuntimeVariable::FreeVariables();

    GAsyncJobManager.Deinitialize();

    if ( IsCriticalError() ) {
        EmergencyExit();
    }

    DeinitializeRenderer();

#if 0
    GMonitorManager.Deinitialize();
#endif

    WorkingDir.Free();

    if ( Clipboard ) {
        SDL_free( Clipboard );
    }

    SDL_Quit();

    DeinitializeMemory();

    DeinitializeProcess();
}

struct SProcessLog {
    FILE * File = nullptr;
};

static SProcessLog ProcessLog;
static AThreadSync LoggerSync;

static void LoggerMessageCallback( int _Level, const char * _Message ) {
    #if defined AN_DEBUG
        #if defined AN_COMPILER_MSVC
        {
            int n = MultiByteToWideChar( CP_UTF8, 0, _Message, -1, NULL, 0 );
            if ( 0 != n ) {
                wchar_t * chars = (wchar_t *)StackAlloc( n * sizeof( wchar_t ) );

                MultiByteToWideChar( CP_UTF8, 0, _Message, -1, chars, n );

                OutputDebugString( chars );
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

    GetEngineInstance()->Print( _Message );

    if ( ProcessLog.File ) {
        ASyncGuard syncGuard( LoggerSync );
        fprintf( ProcessLog.File, "%s", _Message );
        fflush( ProcessLog.File );
    }
}

#ifdef AN_ALLOW_ASSERTS

// Define global assert function
void AssertFunction( const char * _File, int _Line, const char * _Function, const char * _Assertion, const char * _Comment ) {
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

#ifdef AN_OS_WIN32
    DebugBreak();
#else
    raise( SIGTRAP );
#endif

    bNestedFunctionCall = false;
}

#endif

#ifdef AN_OS_WIN32
static HANDLE ProcessMutex = NULL;
#endif

void ARuntime::InitializeProcess() {
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
#error "Not implemented under current platform"
#endif

    ProcessLog.File = nullptr;
    if ( CheckArg( "-enableLog" ) != -1 ) {
        // TODO: Set correct path for log file
        ProcessLog.File = fopen( "log.txt", "ab" );
    }
}

void ARuntime::DeinitializeProcess() {
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

struct SMemoryInfo {
    int TotalAvailableMegabytes;
    int CurrentAvailableMegabytes;
};

static SMemoryInfo GetPhysMemoryInfo() {
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
    #warning "GetPhysMemoryInfo not implemented under current platform"
#endif
    return info;
}

volatile int MemoryChecksum;
static void * MemoryHeap;

static void TouchMemoryPages( void * _MemoryPointer, int _MemorySize ) {
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

void ARuntime::InitializeMemory() {
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
    #warning "GetPageSize not implemented under current platform"
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

void ARuntime::DeinitializeMemory() {
    GZoneMemory.Deinitialize();
    GHunkMemory.Deinitialize();
    GHeapMemory.Free( MemoryHeap );
    GHeapMemory.Deinitialize();
}

void ARuntime::InitializeWorkingDirectory() {
    WorkingDir = Executable;
    WorkingDir.StripFilename();

#if defined AN_OS_WIN32
    SetCurrentDirectoryA( WorkingDir.CStr() );
#elif defined AN_OS_LINUX
    chdir( WorkingDir.CStr() );
#else
    #warning "InitializeWorkingDirectory not implemented under current platform"
#endif
}

void ARuntime::DisplayCriticalMessage( const char * _Message ) {
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

void ARuntime::EmergencyExit() {
    const char * msg = MapCriticalErrorMessage();
    DisplayCriticalMessage( msg );
    UnmapCriticalErrorMessage();

    SDL_Quit();

    GHeapMemory.Clear();

    DeinitializeProcess();

    std::quick_exit( 0 );
    //std::abort();
}

int ARuntime::CheckArg( const char * _Arg ) {
    for ( int i = 0 ; i < NumArguments ; i++ ) {
        if ( !Core::Stricmp( Arguments[ i ], _Arg ) ) {
            return i;
        }
    }
    return -1;
}

static void AllocCommandLineArgs( char * _Buffer, int * _NumArguments, char *** _Arguments ) {
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

static void FreeCommandLineArgs( char ** _Arguments ) {
    free( _Arguments );
}

#define MAX_COMMAND_LINE_LENGTH 1024
static char CmdLineBuffer[ MAX_COMMAND_LINE_LENGTH ];
static bool bApplicationRun = false;

ANGIE_API void Runtime( const char * _CommandLine, ACreateGameModuleCallback _CreateGameModule ) {
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
    GRuntime.Run( _CreateGameModule );
    FreeCommandLineArgs( GRuntime.Arguments );
}

ANGIE_API void Runtime( int _Argc, char ** _Argv, ACreateGameModuleCallback _CreateGameModule ) {
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
    GRuntime.Run( _CreateGameModule );
}

#if defined( AN_DEBUG ) && defined( AN_COMPILER_MSVC )
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved
) {
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

    return TRUE;
}
#endif

int ARuntime::GetArgc() {
    return NumArguments;
}

const char * const *ARuntime::GetArgv() {
    return Arguments;
}

AString const & ARuntime::GetWorkingDir() {
    return WorkingDir;
}

const char * ARuntime::GetExecutableName() {
    return Executable ? Executable : "";
}

void * ARuntime::AllocFrameMem( size_t _SizeInBytes ) {
    if ( FrameMemoryUsed + _SizeInBytes > FrameMemorySize ) {
        GLogger.Printf( "AllocFrameMem: failed on allocation of %u bytes (available %u, total %u)\n", _SizeInBytes, FrameMemorySize - FrameMemoryUsed, FrameMemorySize );
        return nullptr;
    }

    void * pMemory = (byte *)FrameMemoryAddress + FrameMemoryUsed;

    FrameMemoryUsed += _SizeInBytes;
    FrameMemoryUsed = Align( FrameMemoryUsed, 16 );

    AN_ASSERT( IsAlignedPtr( pMemory, 16 ) );

    //GLogger.Printf( "Allocated %u, Used %u\n", _SizeInBytes, FrameMemoryUsed );

    return pMemory;
}

size_t ARuntime::GetFrameMemorySize() const {
    return FrameMemorySize;
}

size_t ARuntime::GetFrameMemoryUsed() const {
    return FrameMemoryUsed;
}

size_t ARuntime::GetFrameMemoryUsedPrev() const {
    return FrameMemoryUsedPrev;
}

size_t ARuntime::GetMaxFrameMemoryUsage() const {
    return MaxFrameMemoryUsage;
}

SCPUInfo const & ARuntime::GetCPUInfo() const {
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

static SWaitableTimer WaitableTimer;
static LARGE_INTEGER WaitTime;

//#include <thread>

static void WaitMicrosecondsWIN32( int _Microseconds ) {
#if 0
    std::this_thread::sleep_for( StdChrono::microseconds( _Microseconds ) );
#else
    WaitTime.QuadPart = -10 * _Microseconds;

    if ( !WaitableTimer.Handle ) {
        WaitableTimer.Handle = CreateWaitableTimer( NULL, TRUE, NULL );
    }

    SetWaitableTimer( WaitableTimer.Handle, &WaitTime, 0, NULL, NULL, FALSE );
    WaitForSingleObject( WaitableTimer.Handle, INFINITE );
    //CloseHandle( WaitableTimer );
#endif
}

#endif

void ARuntime::WaitSeconds( int _Seconds ) {
#ifdef AN_OS_WIN32
    //std::this_thread::sleep_for( StdChrono::seconds( _Seconds ) );
    WaitMicrosecondsWIN32( _Seconds * 1000000 );
#else
    struct timespec ts = { _Seconds, 0 };
    while ( ::nanosleep( &ts, &ts ) == -1 && errno == EINTR ) {}
#endif
}

void ARuntime::WaitMilliseconds( int _Milliseconds ) {
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

void ARuntime::WaitMicroseconds( int _Microseconds ) {
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

int64_t ARuntime::SysSeconds() {
    return StdChrono::duration_cast< StdChrono::seconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - StartSeconds;
}

double ARuntime::SysSeconds_d() {
    return SysMicroseconds() * 0.000001;
}

int64_t ARuntime::SysMilliseconds() {
    return StdChrono::duration_cast< StdChrono::milliseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - StartMilliseconds;
}

double ARuntime::SysMilliseconds_d() {
    return SysMicroseconds() * 0.001;
}

int64_t ARuntime::SysMicroseconds() {
    return StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - StartMicroseconds;
}

double ARuntime::SysMicroseconds_d() {
    return static_cast< double >( SysMicroseconds() );
}

int64_t ARuntime::SysFrameTimeStamp() {
    return FrameTimeStamp;
}

int64_t ARuntime::SysFrameDuration() {
    return FrameDuration;
}

int ARuntime::SysFrameNumber() const {
    return FrameNumber;
}

void * ARuntime::LoadDynamicLib( const char * _LibraryName ) {
    AString name( _LibraryName );
#if defined AN_OS_WIN32
    name.ReplaceExt( ".dll" );
#else
    name.ReplaceExt( ".so" );
#endif
    return SDL_LoadObject( name.CStr() );
}

void ARuntime::UnloadDynamicLib( void * _Handle ) {
    SDL_UnloadObject( _Handle );
}

void * ARuntime::GetProcAddress( void * _Handle, const char * _ProcName ) {
    return _Handle ? SDL_LoadFunction( _Handle, _ProcName ) : nullptr;
}

void ARuntime::SetClipboard( const char * _Utf8String ) {
    SDL_SetClipboardText( _Utf8String );
}

const char * ARuntime::GetClipboard() {
    if ( Clipboard ) {
        SDL_free( Clipboard );
    }
    Clipboard = SDL_GetClipboardText();
    return Clipboard;
}

SVideoMode const & ARuntime::GetVideoMode() const {
    return VideoMode;
}

void ARuntime::PostChangeVideoMode( SVideoMode const & _DesiredMode ) {
    DesiredMode = _DesiredMode;
    bResetVideoMode = true;
}

void ARuntime::PostTerminateEvent() {
    bTerminate = true;
}

bool ARuntime::IsPendingTerminate() {
    return bTerminate;
}

#define FROM_SDL_TIMESTAMP(event) ( (event).timestamp * 0.001 )

struct SKeyMappingsSDL {
    unsigned short & operator[]( const int i ) {
        AN_ASSERT( i >= 0 );
        AN_ASSERT( ( !(i & SDLK_SCANCODE_MASK) && i < AN_ARRAY_SIZE( Table1 ) )
                   || ( (i & SDLK_SCANCODE_MASK) && (i & ~SDLK_SCANCODE_MASK) < AN_ARRAY_SIZE( Table2 ) ) );

        return ( i & SDLK_SCANCODE_MASK ) ? Table2[i & ~SDLK_SCANCODE_MASK] : Table1[i];
    }

    unsigned short Table1[128];
    unsigned short Table2[256];
};

static SKeyMappingsSDL SDLKeyMappings;

static void InitKeyMappingsSDL() {
    Core::ZeroMem( SDLKeyMappings.Table1, sizeof( SDLKeyMappings.Table1 ) );
    Core::ZeroMem( SDLKeyMappings.Table2, sizeof( SDLKeyMappings.Table2 ) );

    SDLKeyMappings[ SDLK_RETURN ] = KEY_ENTER;
    SDLKeyMappings[ SDLK_ESCAPE ] = KEY_ESCAPE;
    SDLKeyMappings[ SDLK_BACKSPACE ] = KEY_BACKSPACE;
    SDLKeyMappings[ SDLK_TAB ] = KEY_TAB;
    SDLKeyMappings[ SDLK_SPACE ] = KEY_SPACE;
//    SDLKeyMappings[ SDLK_EXCLAIM ] = '!';
//    SDLKeyMappings[ SDLK_QUOTEDBL ] = '"';
//    SDLKeyMappings[ SDLK_HASH ] = '#';
//    SDLKeyMappings[ SDLK_PERCENT ] = '%';
//    SDLKeyMappings[ SDLK_DOLLAR ] = '$';
//    SDLKeyMappings[ SDLK_AMPERSAND ] = '&';
    SDLKeyMappings[ SDLK_QUOTE ] = KEY_APOSTROPHE;
//    SDLKeyMappings[ SDLK_LEFTPAREN ] = '(';
//    SDLKeyMappings[ SDLK_RIGHTPAREN ] = ')';
//    SDLKeyMappings[ SDLK_ASTERISK ] = '*';
//    SDLKeyMappings[ SDLK_PLUS ] = '+';
    SDLKeyMappings[ SDLK_COMMA ] = KEY_COMMA;
    SDLKeyMappings[ SDLK_MINUS ] = KEY_MINUS;
    SDLKeyMappings[ SDLK_PERIOD ] = KEY_PERIOD;
    SDLKeyMappings[ SDLK_SLASH ] = KEY_SLASH;
    SDLKeyMappings[ SDLK_0 ] = KEY_0;
    SDLKeyMappings[ SDLK_1 ] = KEY_1;
    SDLKeyMappings[ SDLK_2 ] = KEY_2;
    SDLKeyMappings[ SDLK_3 ] = KEY_3;
    SDLKeyMappings[ SDLK_4 ] = KEY_4;
    SDLKeyMappings[ SDLK_5 ] = KEY_5;
    SDLKeyMappings[ SDLK_6 ] = KEY_6;
    SDLKeyMappings[ SDLK_7 ] = KEY_7;
    SDLKeyMappings[ SDLK_8 ] = KEY_8;
    SDLKeyMappings[ SDLK_9 ] = KEY_9;
//    SDLKeyMappings[ SDLK_COLON ] = ':';
    SDLKeyMappings[ SDLK_SEMICOLON ] = KEY_SEMICOLON;
//    SDLKeyMappings[ SDLK_LESS ] = '<';
    SDLKeyMappings[ SDLK_EQUALS ] = KEY_EQUAL;
//    SDLKeyMappings[ SDLK_GREATER ] = '>';
//    SDLKeyMappings[ SDLK_QUESTION ] = '?';
//    SDLKeyMappings[ SDLK_AT ] = '@';

    SDLKeyMappings[ SDLK_LEFTBRACKET ] = KEY_LEFT_BRACKET;
    SDLKeyMappings[ SDLK_BACKSLASH ] = KEY_BACKSLASH;
    SDLKeyMappings[ SDLK_RIGHTBRACKET ] = KEY_RIGHT_BRACKET;
//    SDLKeyMappings[ SDLK_CARET ] = '^';
//    SDLKeyMappings[ SDLK_UNDERSCORE ] = '_';
    SDLKeyMappings[ SDLK_BACKQUOTE ] = KEY_GRAVE_ACCENT;
    SDLKeyMappings[ SDLK_a ] = KEY_A;
    SDLKeyMappings[ SDLK_b ] = KEY_B;
    SDLKeyMappings[ SDLK_c ] = KEY_C;
    SDLKeyMappings[ SDLK_d ] = KEY_D;
    SDLKeyMappings[ SDLK_e ] = KEY_E;
    SDLKeyMappings[ SDLK_f ] = KEY_F;
    SDLKeyMappings[ SDLK_g ] = KEY_G;
    SDLKeyMappings[ SDLK_h ] = KEY_H;
    SDLKeyMappings[ SDLK_i ] = KEY_I;
    SDLKeyMappings[ SDLK_j ] = KEY_J;
    SDLKeyMappings[ SDLK_k ] = KEY_K;
    SDLKeyMappings[ SDLK_l ] = KEY_L;
    SDLKeyMappings[ SDLK_m ] = KEY_M;
    SDLKeyMappings[ SDLK_n ] = KEY_N;
    SDLKeyMappings[ SDLK_o ] = KEY_O;
    SDLKeyMappings[ SDLK_p ] = KEY_P;
    SDLKeyMappings[ SDLK_q ] = KEY_Q;
    SDLKeyMappings[ SDLK_r ] = KEY_R;
    SDLKeyMappings[ SDLK_s ] = KEY_S;
    SDLKeyMappings[ SDLK_t ] = KEY_T;
    SDLKeyMappings[ SDLK_u ] = KEY_U;
    SDLKeyMappings[ SDLK_v ] = KEY_V;
    SDLKeyMappings[ SDLK_w ] = KEY_W;
    SDLKeyMappings[ SDLK_x ] = KEY_X;
    SDLKeyMappings[ SDLK_y ] = KEY_Y;
    SDLKeyMappings[ SDLK_z ] = KEY_Z;

    SDLKeyMappings[ SDLK_CAPSLOCK ] = KEY_CAPS_LOCK;

    SDLKeyMappings[ SDLK_F1 ] = KEY_F1;
    SDLKeyMappings[ SDLK_F2 ] = KEY_F2;
    SDLKeyMappings[ SDLK_F3 ] = KEY_F3;
    SDLKeyMappings[ SDLK_F4 ] = KEY_F4;
    SDLKeyMappings[ SDLK_F5 ] = KEY_F5;
    SDLKeyMappings[ SDLK_F6 ] = KEY_F6;
    SDLKeyMappings[ SDLK_F7 ] = KEY_F7;
    SDLKeyMappings[ SDLK_F8 ] = KEY_F8;
    SDLKeyMappings[ SDLK_F9 ] = KEY_F9;
    SDLKeyMappings[ SDLK_F10 ] = KEY_F10;
    SDLKeyMappings[ SDLK_F11 ] = KEY_F11;
    SDLKeyMappings[ SDLK_F12 ] = KEY_F12;

    SDLKeyMappings[ SDLK_PRINTSCREEN ] = KEY_PRINT_SCREEN;
    SDLKeyMappings[ SDLK_SCROLLLOCK ] = KEY_SCROLL_LOCK;
    SDLKeyMappings[ SDLK_PAUSE ] = KEY_PAUSE;
    SDLKeyMappings[ SDLK_INSERT ] = KEY_INSERT;
    SDLKeyMappings[ SDLK_HOME ] = KEY_HOME;
    SDLKeyMappings[ SDLK_PAGEUP ] = KEY_PAGE_UP;
    SDLKeyMappings[ SDLK_DELETE ] = KEY_DELETE;
    SDLKeyMappings[ SDLK_END ] = KEY_END;
    SDLKeyMappings[ SDLK_PAGEDOWN ] = KEY_PAGE_DOWN;
    SDLKeyMappings[ SDLK_RIGHT ] = KEY_RIGHT;
    SDLKeyMappings[ SDLK_LEFT ] = KEY_LEFT;
    SDLKeyMappings[ SDLK_DOWN ] = KEY_DOWN;
    SDLKeyMappings[ SDLK_UP ] = KEY_UP;

    SDLKeyMappings[ SDLK_NUMLOCKCLEAR ] = KEY_NUM_LOCK;
    SDLKeyMappings[ SDLK_KP_DIVIDE ] = KEY_KP_DIVIDE;
    SDLKeyMappings[ SDLK_KP_MULTIPLY ] = KEY_KP_MULTIPLY;
    SDLKeyMappings[ SDLK_KP_MINUS ] = KEY_KP_SUBTRACT;
    SDLKeyMappings[ SDLK_KP_PLUS ] = KEY_KP_ADD;
    SDLKeyMappings[ SDLK_KP_ENTER ] = KEY_KP_ENTER;
    SDLKeyMappings[ SDLK_KP_1 ] = KEY_KP_1;
    SDLKeyMappings[ SDLK_KP_2 ] = KEY_KP_2;
    SDLKeyMappings[ SDLK_KP_3 ] = KEY_KP_3;
    SDLKeyMappings[ SDLK_KP_4 ] = KEY_KP_4;
    SDLKeyMappings[ SDLK_KP_5 ] = KEY_KP_5;
    SDLKeyMappings[ SDLK_KP_6 ] = KEY_KP_6;
    SDLKeyMappings[ SDLK_KP_7 ] = KEY_KP_7;
    SDLKeyMappings[ SDLK_KP_8 ] = KEY_KP_8;
    SDLKeyMappings[ SDLK_KP_9 ] = KEY_KP_9;
    SDLKeyMappings[ SDLK_KP_0 ] = KEY_KP_0;
    SDLKeyMappings[ SDLK_KP_PERIOD ] = KEY_KP_DECIMAL;

//    SDLKeyMappings[ SDLK_APPLICATION ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APPLICATION);
//    SDLKeyMappings[ SDLK_POWER ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_POWER);
    SDLKeyMappings[ SDLK_KP_EQUALS ] = KEY_KP_EQUAL;
    SDLKeyMappings[ SDLK_F13 ] = KEY_F13;
    SDLKeyMappings[ SDLK_F14 ] = KEY_F14;
    SDLKeyMappings[ SDLK_F15 ] = KEY_F15;
    SDLKeyMappings[ SDLK_F16 ] = KEY_F16;
    SDLKeyMappings[ SDLK_F17 ] = KEY_F17;
    SDLKeyMappings[ SDLK_F18 ] = KEY_F18;
    SDLKeyMappings[ SDLK_F19 ] = KEY_F19;
    SDLKeyMappings[ SDLK_F20 ] = KEY_F20;
    SDLKeyMappings[ SDLK_F21 ] = KEY_F21;
    SDLKeyMappings[ SDLK_F22 ] = KEY_F22;
    SDLKeyMappings[ SDLK_F23 ] = KEY_F23;
    SDLKeyMappings[ SDLK_F24 ] = KEY_F24;
//    SDLKeyMappings[ SDLK_EXECUTE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXECUTE);
//    SDLKeyMappings[ SDLK_HELP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HELP);
    SDLKeyMappings[ SDLK_MENU ] = KEY_MENU;
//    SDLKeyMappings[ SDLK_SELECT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SELECT);
//    SDLKeyMappings[ SDLK_STOP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_STOP);
//    SDLKeyMappings[ SDLK_AGAIN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AGAIN);
//    SDLKeyMappings[ SDLK_UNDO ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UNDO);
//    SDLKeyMappings[ SDLK_CUT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CUT);
//    SDLKeyMappings[ SDLK_COPY ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COPY);
//    SDLKeyMappings[ SDLK_PASTE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PASTE);
//    SDLKeyMappings[ SDLK_FIND ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_FIND);
//    SDLKeyMappings[ SDLK_MUTE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MUTE);
//    SDLKeyMappings[ SDLK_VOLUMEUP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEUP);
//    SDLKeyMappings[ SDLK_VOLUMEDOWN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEDOWN);
//    SDLKeyMappings[ SDLK_KP_COMMA ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COMMA);
//    SDLKeyMappings[ SDLK_KP_EQUALSAS400] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALSAS400);

//    SDLKeyMappings[ SDLK_ALTERASE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ALTERASE);
//    SDLKeyMappings[ SDLK_SYSREQ ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SYSREQ);
//    SDLKeyMappings[ SDLK_CANCEL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CANCEL);
//    SDLKeyMappings[ SDLK_CLEAR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEAR);
//    SDLKeyMappings[ SDLK_PRIOR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRIOR);
//    SDLKeyMappings[ SDLK_RETURN2 ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RETURN2);
//    SDLKeyMappings[ SDLK_SEPARATOR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SEPARATOR);
//    SDLKeyMappings[ SDLK_OUT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OUT);
//    SDLKeyMappings[ SDLK_OPER ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OPER);
//    SDLKeyMappings[ SDLK_CLEARAGAIN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEARAGAIN);
//    SDLKeyMappings[ SDLK_CRSEL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CRSEL);
//    SDLKeyMappings[ SDLK_EXSEL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXSEL);

//    SDLKeyMappings[ SDLK_KP_00 ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_00);
//    SDLKeyMappings[ SDLK_KP_000 ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_000);
//    SDLKeyMappings[ SDLK_THOUSANDSSEPARATOR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_THOUSANDSSEPARATOR);
//    SDLKeyMappings[ SDLK_DECIMALSEPARATOR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DECIMALSEPARATOR);
//    SDLKeyMappings[ SDLK_CURRENCYUNIT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYUNIT);
//    SDLKeyMappings[ SDLK_CURRENCYSUBUNIT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYSUBUNIT);
//    SDLKeyMappings[ SDLK_KP_LEFTPAREN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTPAREN);
//    SDLKeyMappings[ SDLK_KP_RIGHTPAREN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTPAREN);
//    SDLKeyMappings[ SDLK_KP_LEFTBRACE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTBRACE);
//    SDLKeyMappings[ SDLK_KP_RIGHTBRACE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTBRACE);
//    SDLKeyMappings[ SDLK_KP_TAB ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_TAB);
//    SDLKeyMappings[ SDLK_KP_BACKSPACE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BACKSPACE);
//    SDLKeyMappings[ SDLK_KP_A ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_A);
//    SDLKeyMappings[ SDLK_KP_B ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_B);
//    SDLKeyMappings[ SDLK_KP_C ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_C);
//    SDLKeyMappings[ SDLK_KP_D ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_D);
//    SDLKeyMappings[ SDLK_KP_E ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_E);
//    SDLKeyMappings[ SDLK_KP_F ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_F);
//    SDLKeyMappings[ SDLK_KP_XOR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_XOR);
//    SDLKeyMappings[ SDLK_KP_POWER ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_POWER);
//    SDLKeyMappings[ SDLK_KP_PERCENT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERCENT);
//    SDLKeyMappings[ SDLK_KP_LESS ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LESS);
//    SDLKeyMappings[ SDLK_KP_GREATER ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_GREATER);
//    SDLKeyMappings[ SDLK_KP_AMPERSAND ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AMPERSAND);
//    SDLKeyMappings[ SDLK_KP_DBLAMPERSAND ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLAMPERSAND);
//    SDLKeyMappings[ SDLK_KP_VERTICALBAR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_VERTICALBAR);
//    SDLKeyMappings[ SDLK_KP_DBLVERTICALBAR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLVERTICALBAR);
//    SDLKeyMappings[ SDLK_KP_COLON ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COLON);
//    SDLKeyMappings[ SDLK_KP_HASH ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HASH);
//    SDLKeyMappings[ SDLK_KP_SPACE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_SPACE);
//    SDLKeyMappings[ SDLK_KP_AT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AT);
//    SDLKeyMappings[ SDLK_KP_EXCLAM ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EXCLAM);
//    SDLKeyMappings[ SDLK_KP_MEMSTORE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSTORE);
//    SDLKeyMappings[ SDLK_KP_MEMRECALL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMRECALL);
//    SDLKeyMappings[ SDLK_KP_MEMCLEAR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMCLEAR);
//    SDLKeyMappings[ SDLK_KP_MEMADD ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMADD);
//    SDLKeyMappings[ SDLK_KP_MEMSUBTRACT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSUBTRACT);
//    SDLKeyMappings[ SDLK_KP_MEMMULTIPLY ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMMULTIPLY);
//    SDLKeyMappings[ SDLK_KP_MEMDIVIDE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMDIVIDE);
//    SDLKeyMappings[ SDLK_KP_PLUSMINUS ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUSMINUS);
//    SDLKeyMappings[ SDLK_KP_CLEAR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEAR);
//    SDLKeyMappings[ SDLK_KP_CLEARENTRY ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEARENTRY);
//    SDLKeyMappings[ SDLK_KP_BINARY ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BINARY);
//    SDLKeyMappings[ SDLK_KP_OCTAL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_OCTAL);
//    SDLKeyMappings[ SDLK_KP_DECIMAL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DECIMAL);
//    SDLKeyMappings[ SDLK_KP_HEXADECIMAL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HEXADECIMAL);

    SDLKeyMappings[ SDLK_LCTRL ] = KEY_LEFT_CONTROL;
    SDLKeyMappings[ SDLK_LSHIFT ] = KEY_LEFT_SHIFT;
    SDLKeyMappings[ SDLK_LALT ] = KEY_LEFT_ALT;
    SDLKeyMappings[ SDLK_LGUI ] = KEY_LEFT_SUPER;
    SDLKeyMappings[ SDLK_RCTRL ] = KEY_RIGHT_CONTROL;
    SDLKeyMappings[ SDLK_RSHIFT ] = KEY_RIGHT_SHIFT;
    SDLKeyMappings[ SDLK_RALT ] = KEY_RIGHT_ALT;
    SDLKeyMappings[ SDLK_RGUI ] = KEY_RIGHT_SUPER;

//    SDLKeyMappings[ SDLK_MODE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MODE);

//    SDLKeyMappings[ SDLK_AUDIONEXT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIONEXT);
//    SDLKeyMappings[ SDLK_AUDIOPREV ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOPREV);
//    SDLKeyMappings[ SDLK_AUDIOSTOP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOSTOP);
//    SDLKeyMappings[ SDLK_AUDIOPLAY ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOPLAY);
//    SDLKeyMappings[ SDLK_AUDIOMUTE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOMUTE);
//    SDLKeyMappings[ SDLK_MEDIASELECT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIASELECT);
//    SDLKeyMappings[ SDLK_WWW ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_WWW);
//    SDLKeyMappings[ SDLK_MAIL ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MAIL);
//    SDLKeyMappings[ SDLK_CALCULATOR ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CALCULATOR);
//    SDLKeyMappings[ SDLK_COMPUTER ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COMPUTER);
//    SDLKeyMappings[ SDLK_AC_SEARCH ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SEARCH);
//    SDLKeyMappings[ SDLK_AC_HOME ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_HOME);
//    SDLKeyMappings[ SDLK_AC_BACK ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BACK);
//    SDLKeyMappings[ SDLK_AC_FORWARD ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_FORWARD);
//    SDLKeyMappings[ SDLK_AC_STOP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_STOP);
//    SDLKeyMappings[ SDLK_AC_REFRESH ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_REFRESH);
//    SDLKeyMappings[ SDLK_AC_BOOKMARKS ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BOOKMARKS);

//    SDLKeyMappings[ SDLK_BRIGHTNESSDOWN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_BRIGHTNESSDOWN);
//    SDLKeyMappings[ SDLK_BRIGHTNESSUP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_BRIGHTNESSUP);
//    SDLKeyMappings[ SDLK_DISPLAYSWITCH ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DISPLAYSWITCH);
//    SDLKeyMappings[ SDLK_KBDILLUMTOGGLE ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMTOGGLE);
//    SDLKeyMappings[ SDLK_KBDILLUMDOWN ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMDOWN);
//    SDLKeyMappings[ SDLK_KBDILLUMUP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMUP);
//    SDLKeyMappings[ SDLK_EJECT ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EJECT);
//    SDLKeyMappings[ SDLK_SLEEP ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SLEEP);
//    SDLKeyMappings[ SDLK_APP1 ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APP1);
//    SDLKeyMappings[ SDLK_APP2 ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APP2);

//    SDLKeyMappings[ SDLK_AUDIOREWIND ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOREWIND);
//    SDLKeyMappings[ SDLK_AUDIOFASTFORWARD ] = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOFASTFORWARD);
}

static AN_FORCEINLINE int FromKeymodSDL( Uint16 Mod ) {
    int modMask = 0;

    if ( Mod & (KMOD_LSHIFT|KMOD_RSHIFT) ) {
        modMask |= MOD_MASK_SHIFT;
    }

    if ( Mod & (KMOD_LCTRL|KMOD_RCTRL) ) {
        modMask |= MOD_MASK_CONTROL;
    }

    if ( Mod & (KMOD_LALT|KMOD_RALT) ) {
        modMask |= MOD_MASK_ALT;
    }

    if ( Mod & (KMOD_LGUI|KMOD_RGUI) ) {
        modMask |= MOD_MASK_SUPER;
    }

    if ( Mod & KMOD_CAPS ) {
        modMask |= MOD_MASK_CAPS_LOCK;
    }

    if ( Mod & KMOD_NUM ) {
        modMask |= MOD_MASK_NUM_LOCK;
    }

    return modMask;
}

static void UnpressJoystickButtons( int _JoystickNum, double _TimeStamp ) {
    SJoystickButtonEvent buttonEvent;
    buttonEvent.Joystick = _JoystickNum;
    buttonEvent.Action = IA_Release;
    for ( int i = 0 ; i < MAX_JOYSTICK_BUTTONS ; i++ ) {
        if ( JoystickButtonState[_JoystickNum][i] ) {
            JoystickButtonState[_JoystickNum][i] = SDL_RELEASED;
            buttonEvent.Button = JOY_BUTTON_1 + i;
            GetEngineInstance()->OnJoystickButtonEvent( buttonEvent, _TimeStamp );
        }
    }
}

static void ClearJoystickAxes( int _JoystickNum, double _TimeStamp ) {
    SJoystickAxisEvent axisEvent;
    axisEvent.Joystick = _JoystickNum;
    axisEvent.Value = 0;
    for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
        if ( JoystickAxisState[_JoystickNum][i] != 0 ) {
            JoystickAxisState[_JoystickNum][i] = 0;
            axisEvent.Axis = JOY_AXIS_1 + i;
            GetEngineInstance()->OnJoystickAxisEvent( axisEvent, _TimeStamp );
        }
    }
}

static void UnpressKeysAndButtons() {
    SKeyEvent keyEvent;
    SMouseButtonEvent mouseEvent;

    keyEvent.Action = IA_Release;
    keyEvent.ModMask = 0;

    mouseEvent.Action = IA_Release;
    mouseEvent.ModMask = 0;

    double timeStamp = GRuntime.SysSeconds_d();

    for ( int i = 0 ; i <= KEY_LAST ; i++ ) {
        if ( PressedKeys[i] ) {
            keyEvent.Key = i;
            keyEvent.Scancode = PressedKeys[i] - 1;

            PressedKeys[i] = 0;

            GetEngineInstance()->OnKeyEvent( keyEvent, timeStamp );
        }
    }
    for ( int i = MOUSE_BUTTON_1 ; i <= MOUSE_BUTTON_8 ; i++ ) {
        if ( PressedMouseButtons[i] ) {
            mouseEvent.Button = i;

            PressedMouseButtons[i] = 0;

            GetEngineInstance()->OnMouseButtonEvent( mouseEvent, timeStamp );
        }
    }

    for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        UnpressJoystickButtons( i, timeStamp );
        ClearJoystickAxes( i, timeStamp );
    }
}

void ARuntime::NewFrame() {
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

    if ( bResetVideoMode ) {
        bResetVideoMode = false;
        SetVideoMode( DesiredMode );
    }
}

void ARuntime::PollEvents() {
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
                VideoMode.DisplayIndex = SDL_GetWindowDisplayIndex( (SDL_Window *)GRenderBackend->GetMainWindow() );
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
                SDL_Window * wnd = (SDL_Window *)GRenderBackend->GetMainWindow();
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
            keyEvent.Key = SDLKeyMappings[ event.key.keysym.sym ];
            keyEvent.Scancode = event.key.keysym.scancode;
            keyEvent.Action = ( event.type == SDL_KEYDOWN ) ? ( PressedKeys[keyEvent.Key] ? IA_Repeat : IA_Press ) : IA_Release;
            keyEvent.ModMask = FromKeymodSDL( event.key.keysym.mod );
            if ( keyEvent.Key ) {
                if ( ( keyEvent.Action == IA_Release && !PressedKeys[keyEvent.Key] )
                     || ( keyEvent.Action == IA_Press && PressedKeys[keyEvent.Key] ) ) {

                    // State does not changed
                } else {
                    PressedKeys[keyEvent.Key] = ( keyEvent.Action == IA_Release ) ? 0 : keyEvent.Scancode + 1;

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
            charEvent.ModMask = FromKeymodSDL( SDL_GetModState() );

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
            SMouseMoveEvent moveEvent;
            moveEvent.X = event.motion.xrel;
            moveEvent.Y = -event.motion.yrel;
            Engine->OnMouseMoveEvent( moveEvent, FROM_SDL_TIMESTAMP(event.motion) );
            break;
        }

        // Mouse button pressed
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            SMouseButtonEvent mouseEvent;
            mouseEvent.Button = event.button.button - 1;
            mouseEvent.Action = event.type == SDL_MOUSEBUTTONDOWN ? IA_Press : IA_Release;
            mouseEvent.ModMask = FromKeymodSDL( SDL_GetModState() );

            if ( mouseEvent.Button >= MOUSE_BUTTON_1 && mouseEvent.Button <= MOUSE_BUTTON_8 ) {
                if ( mouseEvent.Action == (int)PressedMouseButtons[mouseEvent.Button] ) {

                    // State does not changed
                } else {
                    PressedMouseButtons[mouseEvent.Button] = mouseEvent.Action != IA_Release;

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

                mouseEvent.Action = IA_Press;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_Release;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

            } else if ( wheelEvent.WheelX > 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_RIGHT;

                mouseEvent.Action = IA_Press;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_Release;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );
            }
            if ( wheelEvent.WheelY < 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_DOWN;

                mouseEvent.Action = IA_Press;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_Release;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );
            } else if ( wheelEvent.WheelY > 0.0 ) {
                mouseEvent.Button = MOUSE_WHEEL_UP;

                mouseEvent.Action = IA_Press;
                Engine->OnMouseButtonEvent( mouseEvent, FROM_SDL_TIMESTAMP(event.wheel) );

                mouseEvent.Action = IA_Release;
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
                        buttonEvent.Action = event.jbutton.state == SDL_PRESSED ? IA_Press : IA_Release;
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

static void TestDisplays() {
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
            } else {
                GLogger.Printf( "Mode %d: %d %d %d hz (uncompatible pixel format)\n", mode.w, mode.h, mode.refresh_rate );
            }
        }
    }

    //SDL_GetDesktopDisplayMode
    //SDL_GetCurrentDisplayMode
    //SDL_GetClosestDisplayMode
}

void ARuntime::InitializeRenderer( SVideoMode const & _DesiredMode ) {
    TestDisplays();

    Core::Memcpy( &VideoMode, &_DesiredMode, sizeof( VideoMode ) );

    GRenderBackend->Initialize( _DesiredMode );

    SDL_Window * wnd = (SDL_Window *)GRenderBackend->GetMainWindow();

    VideoMode.Opacity = Math::Clamp( VideoMode.Opacity, 0.0f, 1.0f );

    if ( VideoMode.Opacity < 1.0f ) {
        SDL_SetWindowOpacity( wnd, VideoMode.Opacity );
    }

    // Store real video mode
    SDL_GetWindowSize( wnd, &VideoMode.Width, &VideoMode.Height );
    SDL_GL_GetDrawableSize( wnd, &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight );
    VideoMode.bFullscreen = !!(SDL_GetWindowFlags( wnd ) & SDL_WINDOW_FULLSCREEN);

    VideoMode.DisplayIndex = SDL_GetWindowDisplayIndex( wnd );
    /*if ( VideoMode.bFullscreen ) {
        const float MM_To_Inch = 0.0393701f;
        VideoMode.DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        VideoMode.DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);

    } else */{
        SDL_GetDisplayDPI( VideoMode.DisplayIndex, NULL, &VideoMode.DPI_X, &VideoMode.DPI_Y );
    }

    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode( wnd, &mode );
    VideoMode.RefreshRate = mode.refresh_rate;
}

void ARuntime::DeinitializeRenderer() {
    GRenderBackend->Deinitialize();

    UnpressKeysAndButtons();
}

void ARuntime::SetVideoMode( SVideoMode const & _DesiredMode ) {
    if ( Core::Stricmp( VideoMode.Backend, _DesiredMode.Backend ) )
    {
        // Backend changed, so we need to restart renderer
        DeinitializeRenderer();
        InitializeRenderer( _DesiredMode );
    }
    else
    {
        // Backend does not changed, so we just change video mode

//        SDL_DisplayMode mode = {};
//        mode.w = VidWidth;
//        mode.h = VidHeight;
//        mode.refresh_rate = VidRefreshRate;

        Core::Memcpy( &VideoMode, &_DesiredMode, sizeof( VideoMode ) );

        SDL_Window * wnd = (SDL_Window *)GRenderBackend->GetMainWindow();


        // Set refresh rate
        SDL_DisplayMode mode = {};
        mode.format = SDL_PIXELFORMAT_RGB888;
        mode.w = _DesiredMode.Width;
        mode.h = _DesiredMode.Height;
        mode.refresh_rate = _DesiredMode.RefreshRate;
        SDL_SetWindowDisplayMode( wnd, &mode );

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

        } else */{
            SDL_GetDisplayDPI( VideoMode.DisplayIndex, NULL, &VideoMode.DPI_X, &VideoMode.DPI_Y );
        }

//        SDL_DisplayMode mode;
        SDL_GetWindowDisplayMode( wnd, &mode );
        VideoMode.RefreshRate = mode.refresh_rate;
    }
}

void ARuntime::SetCursorEnabled( bool _Enabled ) {
    //SDL_SetWindowGrab( Wnd, (SDL_bool)!_Enabled ); // FIXME: Always grab in fullscreen?
    SDL_SetRelativeMouseMode( (SDL_bool)!_Enabled );
}

bool ARuntime::IsCursorEnabled() {
    return !SDL_GetRelativeMouseMode();
}

void ARuntime::GetCursorPosition( int * _X, int * _Y ) {
    (void)SDL_GetMouseState( _X, _Y );
}










#if 0
SPhysicalMonitor const * ARuntime::GetPrimaryMonitor() {
#if 0
    return GMonitorManager.GetPrimaryMonitor();
#else
    return nullptr;
#endif
}

SPhysicalMonitor const * ARuntime::GetMonitor( int _Handle ) {
#if 0
    APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
    return ( (unsigned)_Handle < physicalMonitors.Size() ) ? physicalMonitors[ _Handle ] : nullptr;
#else
    return nullptr;
#endif
}

SPhysicalMonitor const * ARuntime::GetMonitor( const char * _MonitorName ) {
#if 0
    return GMonitorManager.FindMonitor( _MonitorName );
#else
    return nullptr;
#endif
}

bool ARuntime::IsMonitorConnected( int _Handle ) {
#if 0
    bool bConnected = false;
    APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
    if ( (unsigned)_Handle < physicalMonitors.Size() ) {
        bConnected = physicalMonitors[ _Handle ]->Internal.Pointer != nullptr;
    }
    return bConnected;
#else
    return false;
#endif
}

void ARuntime::SetMonitorGammaCurve( int _Handle, float _Gamma ) {
#if 0
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        return;
    }

    if ( _Gamma <= 0.0f ) {
        return;
    }

    const double scale = 1.0 / (physMonitor->GammaRampSize - 1);
    const double InvGamma = 1.0 / _Gamma;

    for( int i = 0 ; i < physMonitor->GammaRampSize ; i++ ) {
        double val = StdPow( i * scale, InvGamma ) * 65535.0 + 0.5;
        if ( val > 65535.0 ) val = 65535.0;
        physMonitor->Internal.GammaRamp[i] =
        physMonitor->Internal.GammaRamp[i + physMonitor->GammaRampSize] =
        physMonitor->Internal.GammaRamp[i + physMonitor->GammaRampSize * 2] = (unsigned short)val;
    }

    GMonitorManager.UpdateMonitorGamma( physMonitor );
#endif
}

void ARuntime::SetMonitorGamma( int _Handle, float _Gamma ) {
#if 0
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        return;
    }

    int brightness = _Gamma > 0.0f ? _Gamma * 255.0f : 0.0f;
    for( int i = 0 ; i < physMonitor->GammaRampSize ; i++ ) {
        int val = i * (brightness + 1);
        if ( val > 0xffff ) val = 0xffff;
        physMonitor->Internal.GammaRamp[i] = val;
        physMonitor->Internal.GammaRamp[i + physMonitor->GammaRampSize] = val;
        physMonitor->Internal.GammaRamp[i + physMonitor->GammaRampSize * 2] = val;
    }

    GMonitorManager.UpdateMonitorGamma( physMonitor );
#endif
}

void ARuntime::SetMonitorGammaRamp( int _Handle, const unsigned short * _GammaRamp ) {
#if 0
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        return;
    }

    Core::Memcpy( physMonitor->Internal.GammaRamp, _GammaRamp, sizeof( unsigned short ) * physMonitor->GammaRampSize * 3 );

    GMonitorManager.UpdateMonitorGamma( physMonitor );
#endif
}

void ARuntime::GetMonitorGammaRamp( int _Handle, unsigned short * _GammaRamp, int & _GammaRampSize ) {
#if 0
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        Core::ZeroMem( _GammaRamp, sizeof( SPhysicalMonitorInternal::GammaRamp ) );
        return;
    }

    _GammaRampSize = physMonitor->GammaRampSize;

    Core::Memcpy( _GammaRamp, physMonitor->Internal.GammaRamp, sizeof( unsigned short ) * _GammaRampSize * 3 );
#endif
}

void ARuntime::RestoreMonitorGamma( int _Handle ) {
#if 0
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        return;
    }

    Core::Memcpy( physMonitor->Internal.GammaRamp, physMonitor->Internal.InitialGammaRamp, sizeof( unsigned short ) * physMonitor->GammaRampSize * 3 );

    GMonitorManager.UpdateMonitorGamma( physMonitor );
#endif
}

int ARuntime::GetPhysicalMonitorsCount() {
#if 0
    int count;
    APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
    count = physicalMonitors.Size();
    return count;
#else
    return 1;
#endif
}

#endif
