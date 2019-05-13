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

#include "rt_main.h"
#include "rt_joystick.h"
#include "rt_monitor.h"
#include "rt_display.h"
#include "rt_event.h"

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/CriticalError.h>
#include <Engine/Core/Public/HashFunc.h>
#include <Engine/Core/Public/WindowsDefs.h>

#include <Engine/Runtime/Public/ImportExport.h>

#include <chrono>

#ifdef AN_OS_LINUX
#include <unistd.h>
#include <sys/file.h>
#define GLFW_EXPOSE_NATIVE_X11
#endif

#ifdef AN_OS_ANDROID
#include <android/log.h>
#endif

#include <GLFW/glfw3.h>

#define MAX_COMMAND_LINE_LENGTH 1024
static char CmdLineBuffer[ MAX_COMMAND_LINE_LENGTH ];
static FThread GameThread;
static bool bApplicationRun = false;

int rt_NumArguments = 0;
char **rt_Arguments = nullptr;
FString rt_WorkingDir;
char * rt_Executable = nullptr;
int64_t rt_SysStartSeconds;
int64_t rt_SysStartMilliseconds;
int64_t rt_SysStartMicroseconds;
FAtomicBool rt_Terminate( false );
FRenderFrame rt_FrameData[ 2 ];
FSyncEvent rt_SimulationIsDone;
FSyncEvent rt_GameUpdateEvent;
int64_t rt_StalledTime = 0;
void * rt_FrameMemoryAddress = nullptr;
size_t rt_FrameMemorySize = 0;

FCreateGameModuleCallback CreateGameModuleCallback;
//void (*RuntimeUpdateCallback)( FEventQueue & _RuntimeEvents, FEventQueue & _GameEvents );

int rt_CheckArg( const char * _Arg ) {
    for ( int i = 0 ; i < rt_NumArguments ; i++ ) {
        if ( !FString::Icmp( rt_Arguments[ i ], _Arg ) ) {
            return i;
        }
    }
    return -1;
}

struct FMemoryInfo {
    int TotalAvailableMegabytes;
    int CurrentAvailableMegabytes;
};

static FMemoryInfo GetPhysMemoryInfo() {
    FMemoryInfo info;

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

static void InitializeMemory() {
    const size_t ZoneSizeInMegabytes = 256;//8;
    const size_t HunkSizeInMegabytes = 16;
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

    FMemoryInfo physMemoryInfo = GetPhysMemoryInfo();
    if ( physMemoryInfo.TotalAvailableMegabytes > 0 && physMemoryInfo.CurrentAvailableMegabytes > 0 ) {
        GLogger.Printf( "Total available phys memory: %d Megs\n", physMemoryInfo.TotalAvailableMegabytes );
        GLogger.Printf( "Current available phys memory: %d Megs\n", physMemoryInfo.CurrentAvailableMegabytes );
    }

    GLogger.Printf( "Zone memory size: %d Megs\n"
                    "Hunk memory size: %d Megs\n"
                    "Frame memory size: %d Megs\n",
                    ZoneSizeInMegabytes, HunkSizeInMegabytes, FrameMemorySizeInMegabytes );

    GMainHeapMemory.Initialize();

    MemoryHeap = GMainHeapMemory.HeapAllocCleared( TotalMemorySizeInBytes, 16 );

    //TouchMemoryPages( MemoryHeap, TotalMemorySizeInBytes );

    void * ZoneMemory = MemoryHeap;
    GMainMemoryZone.Initialize( ZoneMemory, ZoneSizeInMegabytes );

    void * HunkMemory = ( byte * )MemoryHeap + ( ZoneSizeInMegabytes << 20 );
    GMainHunkMemory.Initialize( HunkMemory, HunkSizeInMegabytes );

    rt_FrameMemoryAddress = ( byte * )MemoryHeap + ( ( ZoneSizeInMegabytes + HunkSizeInMegabytes ) << 20 );
    rt_FrameMemorySize = FrameMemorySizeInMegabytes << 20;
}

static void DeinitializeMemory() {
    GMainMemoryZone.Deinitialize();
    GMainHunkMemory.Deinitialize();
    GMainHeapMemory.HeapFree( MemoryHeap );
    GMainHeapMemory.Deinitialize();
}

static void InitWorkingDirectory() {
    rt_WorkingDir = rt_Executable;
    rt_WorkingDir.StripFilename();

#if defined AN_OS_WIN32
    SetCurrentDirectoryA( rt_WorkingDir.ToConstChar() );
#elif defined AN_OS_LINUX
    chdir( rt_WorkingDir.ToConstChar() );
#else
    #warning "InitWorkingDirectory not implemented under current platform"
#endif
}

struct processLog_t {
    FILE * file = nullptr;
};

static processLog_t Log;
static FThreadSync loggerSync;

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

    GamePrintCallback( _Message );

    if ( Log.file ) {
        FSyncGuard syncGuard( loggerSync );
        fprintf( Log.file, "%s", _Message );
        fflush( Log.file );
    }
}

#ifdef AN_OS_WIN32
static HANDLE ProcessMutex = NULL;
#endif

#define PROCESS_COULDNT_CHECK_UNIQUE 1
#define PROCESS_ALREADY_EXISTS       2
#define PROCESS_UNIQUE               3

static int ProcessAttribute = 0;

static void InitializeProcess() {
    setlocale( LC_ALL, "C" );
    srand( ( unsigned )time( NULL ) );

#if defined( AN_OS_WIN32 )
    SetErrorMode( SEM_FAILCRITICALERRORS );
#endif

#ifdef AN_OS_WIN32
    int curLen = 1024;
    int len = 0;

    rt_Executable = nullptr;
    while ( 1 ) {
        rt_Executable = ( char * )realloc( rt_Executable, curLen + 1 );
        len = GetModuleFileNameA( NULL, rt_Executable, curLen );
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
    rt_Executable[ len ] = 0;

    FString::UpdateSeparator( rt_Executable );

    uint32_t appHash = FCore::SDBMHash( rt_Executable, len );

    ProcessMutex = CreateMutexA( NULL, FALSE, FString::Fmt( "angie_%u", appHash ) );
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

    rt_Executable = nullptr;
    while ( 1 ) {
        rt_Executable = ( char * )realloc( rt_Executable, curLen + 1 );
        len = readlink( "/proc/self/exe", rt_Executable, curLen );
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
    rt_Executable[ len ] = 0;

    uint32_t appHash = FCore::SDBMHash( rt_Executable, len );
    int f = open( FString::Fmt( "/tmp/angie_%u.pid", appHash ), O_RDWR | O_CREAT, 0666 );
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

    Log.file = nullptr;
    if ( rt_CheckArg( "-enableLog" ) != -1 ) {
        Log.file = fopen( "log.txt", "ab" );
    }
}

static void DeinitializeProcess() {
    if ( Log.file ) {
        fclose( Log.file );
        Log.file = nullptr;
    }

    if ( rt_Executable ) {
        free( rt_Executable );
        rt_Executable = nullptr;
    }

#ifdef AN_OS_WIN32
    if ( ProcessMutex ) {
        ReleaseMutex( ProcessMutex );
        CloseHandle( ProcessMutex );
        ProcessMutex = NULL;
    }
#endif
}

static void DisplayCriticalMessage( const char * _Message ) {
#if defined AN_OS_WIN32
    MessageBoxA( NULL, _Message, "Critical Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST );
#elif defined AN_OS_ANDROID
    __android_log_print( ANDROID_LOG_INFO, "Critical Error", _Message );
#else
    fprintf( stdout, "Critical Error: %s", _Message );
    fflush( stdout );
#endif
}

static void EmergencyExit() {
    GameThread.Join();

    glfwTerminate();

    GMainHeapMemory.Clear();

    const char * msg = MapCriticalErrorMessage();
    DisplayCriticalMessage( msg );
    UnmapCriticalErrorMessage();

    DeinitializeProcess();

    std::quick_exit( 0 );
    //std::abort();
}

FAtomicBool testInput;

static void RuntimeUpdate( FEventQueue & _RuntimeEvents, FEventQueue & _GameEvents ) {
    rt_InitEventQueue( &_RuntimeEvents );

    FEvent * event = rt_SendEvent();
    event->Type = ET_RuntimeUpdateEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    rt_InputEventCount = 0;

    rt_UpdatePhysicalMonitors();

    rt_UpdateDisplays( _GameEvents );

    // Pump joystick events before any input
    rt_PollJoystickEvents();

    glfwPollEvents();

    if ( testInput.Load() ) {
        testInput.Store( false );

        FEvent * testEvent;

        testEvent = _RuntimeEvents.Push();
        testEvent->Type = ET_MouseMoveEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseMoveEvent.X = 10;
        testEvent->Data.MouseMoveEvent.Y = 0;
        rt_InputEventCount++;

        testEvent = _RuntimeEvents.Push();
        testEvent->Type = ET_MouseButtonEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseButtonEvent.Action = IE_Press;
        testEvent->Data.MouseButtonEvent.Button = 0;
        testEvent->Data.MouseButtonEvent.ModMask = 0;
        rt_InputEventCount++;

        testEvent = _RuntimeEvents.Push();
        testEvent->Type = ET_MouseButtonEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseButtonEvent.Action = IE_Release;
        testEvent->Data.MouseButtonEvent.Button = 0;
        testEvent->Data.MouseButtonEvent.ModMask = 0;
        rt_InputEventCount++;

        testEvent = _RuntimeEvents.Push();
        testEvent->Type = ET_MouseMoveEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseMoveEvent.X = 10;
        testEvent->Data.MouseMoveEvent.Y = 0;
        rt_InputEventCount++;
    }

    // It may happen if game thread is too busy
    if ( event->Type == ET_RuntimeUpdateEvent && _RuntimeEvents.Length() != _RuntimeEvents.MaxLength() ) {
        event->Data.RuntimeUpdateEvent.InputEventCount = rt_InputEventCount;
    } else {
        GLogger.Printf( "Warning: Runtime queue was overflowed\n" );

        _RuntimeEvents.Clear();
    }
}

static void _GameThreadMain( void * ) {
    GameThreadMain();
}

#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/RenderFrontend.h>
extern float AxesFract;
extern FCanvas GCanvas;

FRenderFrame * frameData;

void ReadInput() {
    rt_StalledTime = 0;
    RuntimeUpdate( frameData->RuntimeEvents, frameData->GameEvents );
}

void UpdateCamera() {
    GGameMaster.ProcessEvents();

    GGameMaster.UpdateInputAxes( AxesFract );
}

void RenderFrontend() {
    GRenderFrontend.BuildFrameData( &GCanvas, ImGui::GetDrawData() );
}

void SubmitGameUpdate() {

    frameData->WriteIndex ^= 1;
    frameData->FrameMemorySize = rt_FrameMemorySize - frameData->FrameMemoryUsed;
    if ( frameData->WriteIndex & 1 ) {
        frameData->pFrameMemory = (byte *)rt_FrameMemoryAddress + rt_FrameMemorySize;
    } else {
        frameData->pFrameMemory = rt_FrameMemoryAddress;
    }
    frameData->FrameMemoryUsed = 0;

    frameData->ReadIndex = frameData->WriteIndex ^ 1;

    GLogger.Printf( "Write %d, Read %d\n", frameData->WriteIndex, frameData->ReadIndex );

    rt_GameUpdateEvent.Signal();
}

void RenderBackend() {
    GRenderBackend->CleanupFrame( frameData );
    GRenderBackend->RenderFrame( frameData );
    // Установить GPU эвент, чтобы в конце кадра синхронизировать CPU-GPU
    GRenderBackend->SetGPUEvent();
    GRenderBackend->SwapBuffers();
}

void WaitSimulationIsDone() {
    rt_SimulationIsDone.Wait();
}

void WaitGPU() {
    GRenderBackend->WaitGPU();
}

static void RuntimeMainLoop() {
//    const int RenderHertz = 120;
//    const int64_t frameTimeMicro = 1000000.0 / RenderHertz;
//    int64_t frameTimeStamp;
//    bool timedOut = true;
//    int64_t syncWaitTimeout = 0;

    for ( int i = 0 ; i < 1 ; i++ ) {
        frameData = &rt_FrameData[ i ];

        for ( int j = 0 ; j < 2 ; j++ ) {
            frameData->RenderProxyUploadHead[j] = nullptr;
            frameData->RenderProxyUploadTail[j] = nullptr;
            frameData->RenderProxyFree[j] = nullptr;
        }
        frameData->DrawListHead = nullptr;
        frameData->DrawListTail = nullptr;
    }

    frameData->ReadIndex = 0;
    frameData->WriteIndex = 1;
    frameData->FrameMemoryUsed = 0;
    frameData->FrameMemorySize = rt_FrameMemorySize;
    frameData->pFrameMemory = rt_FrameMemoryAddress;

    // Pump initial events
    frameData = &rt_FrameData[ 0 ];
    RuntimeUpdate( frameData->RuntimeEvents, frameData->GameEvents );

    GGameMaster.InitializeGame();

    GameThread.Routine = _GameThreadMain;
    GameThread.Data = nullptr;
    GameThread.Start();

    if ( SetCriticalMark() ) {
        return;
    }

    while ( 1 ) {
//        frameTimeStamp = GRuntime.SysMicroseconds();

        frameData->DrawListHead = nullptr;
        frameData->DrawListTail = nullptr;

        if ( IsCriticalError() ) {
            // Critical error in other thread was occured
            return;
        }

        if ( rt_Terminate.Load() ) {
            break;
        }

        // Получить свежие данные ввода
        ReadInput();

        // Обновить камеру/курсор
        UpdateCamera();

        // Подготовить кадр на отрисовку (VSD culling, sort, batch, etc). Может быть разделен на параллельные джобы
        RenderFrontend();

        // Разбудить игровой поток
        SubmitGameUpdate();

        // Сгенерировать команды для GPU, SwapBuffers
        RenderBackend();

        // Ожидаем окончание симуляции в игровом потоке (тем временем, GPU выполняет команды бэкенда)
        WaitSimulationIsDone();

        // Ожидаем выполнение команд GPU, чтобы исключить "input lag"
        WaitGPU();

        //FrameNum++;
    }

    GameThread.Join();

    GGameMaster.DeinitializeGame();

    // Game thread is stopped at this moment

    //for ( int i = 0 ; i < 2 ; i++ ) {
    //    frameData = &rt_FrameData[ 0 ];

        GRenderBackend->CleanupFrame( frameData );
        frameData->ReadIndex^=1;
        GRenderBackend->CleanupFrame( frameData );

        frameData->Instances.Free();
        frameData->DbgVertices.Free();
        frameData->DbgIndices.Free();
        frameData->DbgCmds.Free();
    //}
}

static void Runtime( FCreateGameModuleCallback _CreateGameModule ) {

    rt_SysStartMicroseconds = StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count();
    rt_SysStartMilliseconds = rt_SysStartMicroseconds * 0.001;
    rt_SysStartSeconds = rt_SysStartMicroseconds * 0.000001;

    //SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        EmergencyExit();
    }

    InitializeProcess();

    GLogger.SetMessageCallback( LoggerMessageCallback );

    GLogger.Print( "OS: " AN_OS_STRING "\n" );
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

    InitWorkingDirectory();

    GLogger.Printf( "Working directory: %s\n", rt_WorkingDir.ToConstChar() );
    GLogger.Printf( "Executable: %s\n", rt_Executable );

    glfwSetErrorCallback( []( int _ErrorCode, const char * _UnicodeMessage ) {
        GLogger.Printf( "Error: %d : %s\n", _ErrorCode, _UnicodeMessage );
    } );

    //glfwInitHint(int hint, int value); GLFW_JOYSTICK_HAT_BUTTONS / GLFW_COCOA_CHDIR_RESOURCES / GLFW_COCOA_MENUBAR

    if ( !glfwInit() ) {
        CriticalError( "Failed to initialize runtime\n" );
    }

    if ( FThread::NumHardwareThreads ) {
        GLogger.Printf( "Num hardware threads: %d\n", FThread::NumHardwareThreads );
    }

    int jobManagerThreadCount = FThread::NumHardwareThreads ? FMath::Min( FThread::NumHardwareThreads, GAsyncJobManager.MAX_WORKER_THREADS )
        : GAsyncJobManager.MAX_WORKER_THREADS;
    GAsyncJobManager.Initialize( jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS );

    GRenderFrontendJobList = GAsyncJobManager.GetAsyncJobList( RENDER_FRONTEND_JOB_LIST );
    GRenderBackendJobList = GAsyncJobManager.GetAsyncJobList( RENDER_BACKEND_JOB_LIST );

    rt_InitializeJoysticks();
    rt_InitializePhysicalMonitors();

    rt_InitializeDisplays();

    CreateGameModuleCallback = _CreateGameModule;
    //RuntimeUpdateCallback = RuntimeUpdate;

    RuntimeMainLoop();

    GAsyncJobManager.Deinitialize();

    if ( IsCriticalError() ) {
        EmergencyExit();
    }

    rt_DeinitializeDisplays();
    rt_DeinitializePhysicalMonitors();
    rt_DeinitializeJoysticks();

    glfwTerminate();

    rt_WorkingDir.Free();

    DeinitializeMemory();

    DeinitializeProcess();
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

ANGIE_API void Runtime( const char * _CommandLine, FCreateGameModuleCallback _CreateGameModule ) {
    if ( bApplicationRun ) {
        AN_Assert( 0 );
        return;
    }
    bApplicationRun = true;
    FString::CopySafe( CmdLineBuffer, sizeof( CmdLineBuffer ), _CommandLine );
    AllocCommandLineArgs( CmdLineBuffer, &rt_NumArguments, &rt_Arguments );
    if ( rt_NumArguments < 1 ) {
        AN_Assert( 0 );
        return;
    }
    // Fix executable path separator
    FString::UpdateSeparator( rt_Arguments[ 0 ] );
    Runtime( _CreateGameModule );
    FreeCommandLineArgs( rt_Arguments );
}

ANGIE_API void Runtime( int _Argc, char ** _Argv, FCreateGameModuleCallback _CreateGameModule ) {
    if ( bApplicationRun ) {
        AN_Assert( 0 );
        return;
    }
    bApplicationRun = true;
    rt_NumArguments = _Argc;
    rt_Arguments = _Argv;
    if ( rt_NumArguments < 1 ) {
        AN_Assert( 0 );
        return;
    }
    // Fix executable path separator
    FString::UpdateSeparator( rt_Arguments[ 0 ] );
    Runtime( _CreateGameModule );
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
