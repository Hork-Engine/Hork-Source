/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "RuntimeMain.h"
#include "RuntimeEvents.h"
#include "JoystickManager.h"
#include "MonitorManager.h"
#include "WindowManager.h"
#include "CPUInfo.h"

#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/HashFunc.h>
#include <Core/Public/WindowsDefs.h>

#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <Runtime/Public/RuntimeVariable.h>

#include <chrono>

#ifdef AN_OS_LINUX
#include <unistd.h>
#include <sys/file.h>
#include <signal.h>
#define GLFW_EXPOSE_NATIVE_X11
#endif

#ifdef AN_OS_ANDROID
#include <android/log.h>
#endif

#include <GLFW/glfw3.h>

#define PROCESS_COULDNT_CHECK_UNIQUE 1
#define PROCESS_ALREADY_EXISTS       2
#define PROCESS_UNIQUE               3

static ARuntimeVariable RVSyncGPU( _CTS( "SyncGPU" ), _CTS( "1" ) );
static ARuntimeVariable RVTestInput( _CTS( "TestInput" ),  _CTS( "0" ) );

ARuntimeMain & GRuntimeMain = ARuntimeMain::Inst();

static void LoggerMessageCallback( int _Level, const char * _Message );
static void TestInput();

ARuntimeMain::ARuntimeMain() {
    NumArguments = 0;
    Arguments = nullptr;
    Executable = nullptr;
    FrameMemoryAddress = nullptr;
    FrameMemorySize = 0;
    FrameMemoryUsed = 0;
    FrameMemoryUsedPrev = 0;
    MaxFrameMemoryUsage = 0;
    bTerminate = false;
}

void ARuntimeMain::Run( ACreateGameModuleCallback _CreateGameModule ) {
    SysStartMicroseconds = StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count();
    SysStartMilliseconds = SysStartMicroseconds * 0.001;
    SysStartSeconds = SysStartMicroseconds * 0.000001;
    SysFrameTimeStamp = SysStartMicroseconds;

    CreateGameModuleCallback = _CreateGameModule;

    Engine = GetEngineInstance();

    //SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        EmergencyExit();
    }

    GetCPUInfo( CPUInfo );

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

    glfwSetErrorCallback( []( int _ErrorCode, const char * _UnicodeMessage ) {
        GLogger.Printf( "Error: %d : %s\n", _ErrorCode, _UnicodeMessage );
    } );

    //glfwInitHint(int hint, int value); GLFW_JOYSTICK_HAT_BUTTONS / GLFW_COCOA_CHDIR_RESOURCES / GLFW_COCOA_MENUBAR

    if ( !glfwInit() ) {
        CriticalError( "Failed to initialize runtime\n" );
    }

    if ( AThread::NumHardwareThreads ) {
        GLogger.Printf( "Num hardware threads: %d\n", AThread::NumHardwareThreads );
    }

    int jobManagerThreadCount = AThread::NumHardwareThreads ? Math::Min( AThread::NumHardwareThreads, GAsyncJobManager.MAX_WORKER_THREADS )
        : GAsyncJobManager.MAX_WORKER_THREADS;
    GAsyncJobManager.Initialize( jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS );

    GRenderFrontendJobList = GAsyncJobManager.GetAsyncJobList( RENDER_FRONTEND_JOB_LIST );
    GRenderBackendJobList = GAsyncJobManager.GetAsyncJobList( RENDER_BACKEND_JOB_LIST );

    GJoystickManager.Initialize();

    GMonitorManager.Initialize();

    GWindowManager.Initialize();

    RuntimeMainLoop();

    ARuntimeVariable::FreeVariables();

    GAsyncJobManager.Deinitialize();

    if ( IsCriticalError() ) {
        EmergencyExit();
    }

    GWindowManager.Deinitialize();

    GMonitorManager.Deinitialize();

    GJoystickManager.Deinitialize();

    glfwTerminate();

    WorkingDir.Free();

    DeinitializeMemory();

    DeinitializeProcess();
}

void ARuntimeMain::RuntimeMainLoop() {

    // Pump initial events
    RuntimeUpdate();

    Engine->Initialize( CreateGameModuleCallback );

    if ( SetCriticalMark() ) {
        return;
    }

    do
    {
        SysFrameTimeStamp = GRuntime.SysMicroseconds();

        if ( IsCriticalError() ) {
            // Critical error in other thread was occured
            return;
        }

        // Process game events, pump runtime events
        RuntimeUpdate();

        // Refresh frame data (camera, cursor), prepare frame data for render backend
        Engine->PrepareFrame();

        // Generate GPU commands, SwapBuffers
        GRenderBackend->RenderFrame( &FrameData );

        // Keep memory statistics
        MaxFrameMemoryUsage = Math::Max( MaxFrameMemoryUsage, FrameMemoryUsed );
        FrameMemoryUsedPrev = FrameMemoryUsed;

        // Free frame memory for the next frame
        FrameMemoryUsed = 0;

        // Run game logic for next frame (GPU process current frame now)
        Engine->UpdateFrame();

        if ( RVSyncGPU ) {
            // Wait GPU to prevent "input lag"
            GRenderBackend->WaitGPU();
        }

    } while ( !bTerminate );

    Engine->Deinitialize();

    FrameData.Instances.Free();
    FrameData.ShadowInstances.Free();
    FrameData.DirectionalLights.Free();
    FrameData.Lights.Free();
    FrameData.DbgVertices.Free();
    FrameData.DbgIndices.Free();
    FrameData.DbgCmds.Free();
}

void ARuntimeMain::RuntimeUpdate() {
    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_RuntimeUpdateEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    GInputEventsCount = 0;

    GMonitorManager.UpdateMonitors();

    GWindowManager.Update( GGameEvents );

    // Pump joystick events before any input
    GJoystickManager.PollEvents();

    glfwPollEvents();

    TestInput();

    // It may happen if game thread is too busy
    if ( event->Type == ET_RuntimeUpdateEvent && GRuntimeEvents.Size() != GRuntimeEvents.MaxSize() ) {
        event->Data.RuntimeUpdateEvent.InputEventCount = GInputEventsCount;
    } else {
        GLogger.Printf( "Warning: Runtime queue was overflowed\n" );

        GRuntimeEvents.Clear();
    }
}

struct SProcessLog {
    FILE * File = nullptr;
};

static SProcessLog ProcessLog;
static AThreadSync LoggerSync;

static void LoggerMessageCallback( int _Level, const char * _Message ) {
#if defined AN_COMPILER_MSVC// && defined AN_DEBUG
    OutputDebugStringA( _Message );
#else
#ifdef AN_OS_ANDROID
    __android_log_print( ANDROID_LOG_INFO, "Angie Engine", _Message );
#else
    fprintf( stdout, "%s", _Message );
    fflush( stdout );
#endif
#endif

    GRuntimeMain.Engine->Print( _Message );

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

void ARuntimeMain::InitializeProcess() {
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

    AString::FixSeparator( Executable );

    uint32_t appHash = Core::SDBMHash( Executable, len );

    ProcessMutex = CreateMutexA( NULL, FALSE, AString::Fmt( "angie_%u", appHash ) );
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
    int f = open( AString::Fmt( "/tmp/angie_%u.pid", appHash ), O_RDWR | O_CREAT, 0666 );
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
        ProcessLog.File = fopen( "log.txt", "ab" );
    }
}

void ARuntimeMain::DeinitializeProcess() {
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

    memset( &info, 0, sizeof( info ) );

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

void ARuntimeMain::InitializeMemory() {
    const size_t ZoneSizeInMegabytes = 256;//8;
    const size_t HunkSizeInMegabytes = 32;
    const size_t FrameMemorySizeInMegabytes = 256;//128;

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

    MemoryHeap = GHeapMemory.HeapAllocCleared( TotalMemorySizeInBytes, 16 );

    //TouchMemoryPages( MemoryHeap, TotalMemorySizeInBytes );

    void * ZoneMemory = MemoryHeap;
    GZoneMemory.Initialize( ZoneMemory, ZoneSizeInMegabytes );

    void * HunkMemory = ( byte * )MemoryHeap + ( ZoneSizeInMegabytes << 20 );
    GHunkMemory.Initialize( HunkMemory, HunkSizeInMegabytes );

    FrameMemoryAddress = ( byte * )MemoryHeap + ( ( ZoneSizeInMegabytes + HunkSizeInMegabytes ) << 20 );
    FrameMemorySize = FrameMemorySizeInMegabytes << 20;
}

void ARuntimeMain::DeinitializeMemory() {
    GZoneMemory.Deinitialize();
    GHunkMemory.Deinitialize();
    GHeapMemory.HeapFree( MemoryHeap );
    GHeapMemory.Deinitialize();
}

void ARuntimeMain::InitializeWorkingDirectory() {
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

void ARuntimeMain::DisplayCriticalMessage( const char * _Message ) {
#if defined AN_OS_WIN32
    MessageBoxA( NULL, _Message, "Critical Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST );
#elif defined AN_OS_ANDROID
    __android_log_print( ANDROID_LOG_INFO, "Critical Error", _Message );
#else
    fprintf( stdout, "Critical Error: %s", _Message );
    fflush( stdout );
#endif
}

void ARuntimeMain::EmergencyExit() {
    glfwTerminate();

    GHeapMemory.Clear();

    const char * msg = MapCriticalErrorMessage();
    DisplayCriticalMessage( msg );
    UnmapCriticalErrorMessage();

    DeinitializeProcess();

    std::quick_exit( 0 );
    //std::abort();
}

static void TestInput() {
    if ( RVTestInput ) {
        RVTestInput = false;

        SEvent * event;

        event = GRuntimeEvents.Push();
        event->Type = ET_MouseMoveEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        event->Data.MouseMoveEvent.X = 10;
        event->Data.MouseMoveEvent.Y = 0;
        GInputEventsCount++;

        event = GRuntimeEvents.Push();
        event->Type = ET_MouseButtonEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        event->Data.MouseButtonEvent.Action = IE_Press;
        event->Data.MouseButtonEvent.Button = 0;
        event->Data.MouseButtonEvent.ModMask = 0;
        GInputEventsCount++;

        event = GRuntimeEvents.Push();
        event->Type = ET_MouseButtonEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        event->Data.MouseButtonEvent.Action = IE_Release;
        event->Data.MouseButtonEvent.Button = 0;
        event->Data.MouseButtonEvent.ModMask = 0;
        GInputEventsCount++;

        event = GRuntimeEvents.Push();
        event->Type = ET_MouseMoveEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        event->Data.MouseMoveEvent.X = 10;
        event->Data.MouseMoveEvent.Y = 0;
        GInputEventsCount++;
    }
}

int ARuntimeMain::CheckArg( const char * _Arg ) {
    for ( int i = 0 ; i < NumArguments ; i++ ) {
        if ( !AString::Icmp( Arguments[ i ], _Arg ) ) {
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
    AString::CopySafe( CmdLineBuffer, sizeof( CmdLineBuffer ), _CommandLine );
    AllocCommandLineArgs( CmdLineBuffer, &GRuntimeMain.NumArguments, &GRuntimeMain.Arguments );
    if ( GRuntimeMain.NumArguments < 1 ) {
        AN_ASSERT( 0 );
        return;
    }
    // Fix executable path separator
    AString::FixSeparator( GRuntimeMain.Arguments[ 0 ] );
    GRuntimeMain.Run( _CreateGameModule );
    FreeCommandLineArgs( GRuntimeMain.Arguments );
}

ANGIE_API void Runtime( int _Argc, char ** _Argv, ACreateGameModuleCallback _CreateGameModule ) {
    if ( bApplicationRun ) {
        AN_ASSERT( 0 );
        return;
    }
    bApplicationRun = true;
    GRuntimeMain.NumArguments = _Argc;
    GRuntimeMain.Arguments = _Argv;
    if ( GRuntimeMain.NumArguments < 1 ) {
        AN_ASSERT( 0 );
        return;
    }
    // Fix executable path separator
    AString::FixSeparator( GRuntimeMain.Arguments[ 0 ] );
    GRuntimeMain.Run( _CreateGameModule );
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
