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

struct FCPUInfo {
    bool OS_AVX : 1;
    bool OS_AVX512 : 1;
    bool OS_64bit : 1;

    bool Intel : 1;
    bool AMD : 1;

    // Simd 128 bit
    bool SSE : 1;
    bool SSE2 : 1;
    bool SSE3 : 1;
    bool SSSE3 : 1;
    bool SSE41 : 1;
    bool SSE42 : 1;
    bool SSE4a : 1;
    bool AES : 1;
    bool SHA : 1;

    // Simd 256 bit
    bool AVX : 1;
    bool XOP : 1;
    bool FMA3 : 1;
    bool FMA4 : 1;
    bool AVX2 : 1;

    // Simd 512 bit
    bool AVX512_F : 1;
    bool AVX512_CD : 1;
    bool AVX512_PF : 1;
    bool AVX512_ER : 1;
    bool AVX512_VL : 1;
    bool AVX512_BW : 1;
    bool AVX512_DQ : 1;
    bool AVX512_IFMA : 1;
    bool AVX512_VBMI : 1;

    // Features
    bool x64 : 1;
    bool ABM : 1;
    bool MMX : 1;
    bool RDRAND : 1;
    bool BMI1 : 1;
    bool BMI2 : 1;
    bool ADX : 1;
    bool MPX : 1;
    bool PREFETCHWT1 : 1;
};

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
int64_t rt_SysFrameTimeStamp;
FRenderFrame rt_FrameData;
FEventQueue rt_Events;
FEventQueue rt_GameEvents;
static void * rt_FrameMemoryAddress = nullptr;
static size_t rt_FrameMemorySize = 0;
static FSyncEvent rt_SimulationIsDone;
static FSyncEvent rt_GameUpdateEvent;
static bool rt_Terminate = false;
static IGameEngine * rt_GameEngine;
static FCreateGameModuleCallback rt_CreateGameModuleCallback;
static FCPUInfo rt_CPUInfo;

int rt_CheckArg( const char * _Arg ) {
    for ( int i = 0 ; i < rt_NumArguments ; i++ ) {
        if ( !FString::Icmp( rt_Arguments[ i ], _Arg ) ) {
            return i;
        }
    }
    return -1;
}

#ifdef AN_OS_LINUX

#include <cpuid.h>

static void CPUID( int32_t out[4], int32_t x ) {
    __cpuid_count( x, 0, out[0], out[1], out[2], out[3] );
}

static uint64_t xgetbv( unsigned int index ) {
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
    return ((uint64_t)edx << 32) | eax;
}

#define _XCR_XFEATURE_ENABLED_MASK 0

#endif

#ifdef AN_OS_WIN32
//#include <intrin.h>

static void CPUID( int32_t out[4], int32_t x ) {
    __cpuidex(out, x, 0);
}

static __int64 xgetbv( unsigned int index ) {
    return _xgetbv(index);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            printf("Error Detecting Operating System.\n");
            printf("Defaulting to 32-bit OS.\n\n");
            bIsWow64 = FALSE;
        }
    }
    return bIsWow64;
}

#endif

static void GetCPUInfo( FCPUInfo & _Info ){
    int32_t cpuInfo[4];
    char vendor[13];

    memset( &_Info, 0, sizeof( _Info ) );

#ifdef AN_OS_WIN32
#ifdef _M_X64
    _Info.OS_64bit = true;
#else
    _Info.OS_64bit = IsWow64() != 0;
#endif
#else
    _Info.OS_64bit = true; // FIXME
#endif

    CPUID( cpuInfo, 1 );

    bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

    if ( osUsesXSAVE_XRSTORE && cpuAVXSuport ) {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        _Info.OS_AVX = (xcrFeatureMask & 0x6) == 0x6;
    }

    if ( _Info.OS_AVX ) {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        _Info.OS_AVX512 = (xcrFeatureMask & 0xe6) == 0xe6;
    }

    CPUID( cpuInfo, 0 );
    memcpy( vendor + 0, &cpuInfo[1], 4 );
    memcpy( vendor + 4, &cpuInfo[3], 4 );
    memcpy( vendor + 8, &cpuInfo[2], 4 );
    vendor[12] = '\0';

    if ( !FString::Cmp( vendor, "GenuineIntel" ) ){
        _Info.Intel = true;
    } else if ( !FString::Cmp( vendor, "AuthenticAMD" ) ){
        _Info.AMD = true;
    }

    int nIds = cpuInfo[0];

    CPUID( cpuInfo, 0x80000000 );
    uint32_t nExIds = cpuInfo[0];

    if ( nIds >= 0x00000001 ) {
        CPUID( cpuInfo, 0x00000001 );

        _Info.MMX    = (cpuInfo[3] & ((int)1 << 23)) != 0;
        _Info.SSE    = (cpuInfo[3] & ((int)1 << 25)) != 0;
        _Info.SSE2   = (cpuInfo[3] & ((int)1 << 26)) != 0;
        _Info.SSE3   = (cpuInfo[2] & ((int)1 <<  0)) != 0;

        _Info.SSSE3  = (cpuInfo[2] & ((int)1 <<  9)) != 0;
        _Info.SSE41  = (cpuInfo[2] & ((int)1 << 19)) != 0;
        _Info.SSE42  = (cpuInfo[2] & ((int)1 << 20)) != 0;
        _Info.AES    = (cpuInfo[2] & ((int)1 << 25)) != 0;

        _Info.AVX    = (cpuInfo[2] & ((int)1 << 28)) != 0;
        _Info.FMA3   = (cpuInfo[2] & ((int)1 << 12)) != 0;

        _Info.RDRAND = (cpuInfo[2] & ((int)1 << 30)) != 0;
    }

    if ( nIds >= 0x00000007 ) {
        CPUID( cpuInfo, 0x00000007 );

        _Info.AVX2         = (cpuInfo[1] & ((int)1 <<  5)) != 0;

        _Info.BMI1         = (cpuInfo[1] & ((int)1 <<  3)) != 0;
        _Info.BMI2         = (cpuInfo[1] & ((int)1 <<  8)) != 0;
        _Info.ADX          = (cpuInfo[1] & ((int)1 << 19)) != 0;
        _Info.MPX          = (cpuInfo[1] & ((int)1 << 14)) != 0;
        _Info.SHA          = (cpuInfo[1] & ((int)1 << 29)) != 0;
        _Info.PREFETCHWT1  = (cpuInfo[2] & ((int)1 <<  0)) != 0;

        _Info.AVX512_F     = (cpuInfo[1] & ((int)1 << 16)) != 0;
        _Info.AVX512_CD    = (cpuInfo[1] & ((int)1 << 28)) != 0;
        _Info.AVX512_PF    = (cpuInfo[1] & ((int)1 << 26)) != 0;
        _Info.AVX512_ER    = (cpuInfo[1] & ((int)1 << 27)) != 0;
        _Info.AVX512_VL    = (cpuInfo[1] & ((int)1 << 31)) != 0;
        _Info.AVX512_BW    = (cpuInfo[1] & ((int)1 << 30)) != 0;
        _Info.AVX512_DQ    = (cpuInfo[1] & ((int)1 << 17)) != 0;
        _Info.AVX512_IFMA  = (cpuInfo[1] & ((int)1 << 21)) != 0;
        _Info.AVX512_VBMI  = (cpuInfo[2] & ((int)1 <<  1)) != 0;
    }

    if ( nExIds >= 0x80000001 ) {
        CPUID( cpuInfo, 0x80000001 );
        _Info.x64   = (cpuInfo[3] & ((int)1 << 29)) != 0;
        _Info.ABM   = (cpuInfo[2] & ((int)1 <<  5)) != 0;
        _Info.SSE4a = (cpuInfo[2] & ((int)1 <<  6)) != 0;
        _Info.FMA4  = (cpuInfo[2] & ((int)1 << 16)) != 0;
        _Info.XOP   = (cpuInfo[2] & ((int)1 << 11)) != 0;
    }
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

    rt_GameEngine->Print( _Message );

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

static void RuntimeUpdate() {

    FEvent * event = rt_Events.Push();
    event->Type = ET_RuntimeUpdateEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    rt_InputEventCount = 0;

    rt_UpdatePhysicalMonitors();

    rt_UpdateDisplays( rt_GameEvents );

    // Pump joystick events before any input
    rt_PollJoystickEvents();

    glfwPollEvents();

    if ( testInput.Load() ) {
        testInput.Store( false );

        FEvent * testEvent;

        testEvent = rt_Events.Push();
        testEvent->Type = ET_MouseMoveEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseMoveEvent.X = 10;
        testEvent->Data.MouseMoveEvent.Y = 0;
        rt_InputEventCount++;

        testEvent = rt_Events.Push();
        testEvent->Type = ET_MouseButtonEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseButtonEvent.Action = IE_Press;
        testEvent->Data.MouseButtonEvent.Button = 0;
        testEvent->Data.MouseButtonEvent.ModMask = 0;
        rt_InputEventCount++;

        testEvent = rt_Events.Push();
        testEvent->Type = ET_MouseButtonEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseButtonEvent.Action = IE_Release;
        testEvent->Data.MouseButtonEvent.Button = 0;
        testEvent->Data.MouseButtonEvent.ModMask = 0;
        rt_InputEventCount++;

        testEvent = rt_Events.Push();
        testEvent->Type = ET_MouseMoveEvent;
        testEvent->TimeStamp = GRuntime.SysSeconds_d();
        testEvent->Data.MouseMoveEvent.X = 10;
        testEvent->Data.MouseMoveEvent.Y = 0;
        rt_InputEventCount++;
    }

    // It may happen if game thread is too busy
    if ( event->Type == ET_RuntimeUpdateEvent && rt_Events.Size() != rt_Events.MaxSize() ) {
        event->Data.RuntimeUpdateEvent.InputEventCount = rt_InputEventCount;
    } else {
        GLogger.Printf( "Warning: Runtime queue was overflowed\n" );

        rt_Events.Clear();
    }
}

static void SubmitGameUpdate() {
    rt_FrameData.WriteIndex ^= 1;
    rt_FrameData.ReadIndex = rt_FrameData.WriteIndex ^ 1;
    rt_FrameData.FrameMemorySize = rt_FrameMemorySize - rt_FrameData.FrameMemoryUsed;
    if ( rt_FrameData.WriteIndex & 1 ) {
        rt_FrameData.pFrameMemory = (byte *)rt_FrameMemoryAddress + rt_FrameMemorySize;
    } else {
        rt_FrameData.pFrameMemory = rt_FrameMemoryAddress;
    }
    rt_FrameData.FrameMemoryUsed = 0;
    rt_GameUpdateEvent.Signal();
}

static void WaitGameUpdate() {
    rt_GameUpdateEvent.Wait();
}

static void SignalSimulationIsDone() {
    rt_SimulationIsDone.Signal();
}

static void WaitSimulationIsDone() {
    rt_SimulationIsDone.Wait();
}

static void GameThreadMain( void * ) {
    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        rt_Terminate = true;
        return;
    }

    while ( 1 ) {
        // Ожидаем пока MainThread не пробудит поток вызовом SubmitGameUpdate
        //do {
        //    //  Пока ждем, можно сделать что-то полезное, например копить сетевые пакеты
        //    ProcessNetworkPackets();
        //} while ( WaitGameUpdate( timeOut ) );

        WaitGameUpdate();

        if ( IsCriticalError() ) {
            // Critical error in other thread was occured
            rt_Terminate = true;
            return;
        }

        if ( rt_GameEngine->IsStopped() ) {
            break;
        }

        rt_GameEngine->UpdateFrame();

        SignalSimulationIsDone();
    }

    rt_Terminate = true;
    SignalSimulationIsDone();
}

static void RenderBackend() {
    GRenderBackend->RenderFrame( &rt_FrameData );
}

static void WaitGPU() {
    GRenderBackend->WaitGPU();
}

static void RuntimeMainLoop() {
    for ( int j = 0 ; j < 2 ; j++ ) {
        rt_FrameData.RenderProxyUploadHead[j] = nullptr;
        rt_FrameData.RenderProxyUploadTail[j] = nullptr;
        rt_FrameData.RenderProxyFree[j] = nullptr;
    }
    rt_FrameData.DrawListHead = nullptr;
    rt_FrameData.DrawListTail = nullptr;
    rt_FrameData.ReadIndex = 1;
    rt_FrameData.WriteIndex = 0;
    rt_FrameData.FrameMemoryUsed = 0;
    rt_FrameData.FrameMemorySize = rt_FrameMemorySize;
    rt_FrameData.pFrameMemory = rt_FrameMemoryAddress;

    // Pump initial events
    RuntimeUpdate();

    rt_GameEngine->Initialize( rt_CreateGameModuleCallback );

    GameThread.Routine = GameThreadMain;
    GameThread.Data = nullptr;
    GameThread.Start();

    if ( SetCriticalMark() ) {
        return;
    }

    while ( 1 ) {
        rt_SysFrameTimeStamp = GRuntime.SysMicroseconds();

        rt_FrameData.DrawListHead = nullptr;
        rt_FrameData.DrawListTail = nullptr;

        if ( IsCriticalError() ) {
            // Critical error in other thread was occured
            return;
        }

        if ( rt_Terminate ) {
            break;
        }

        // Получить свежие данные ввода и другие события
        RuntimeUpdate();

        // Обновить данные кадра (камера, курсор), подготовить данные для render backend
        rt_GameEngine->BuildFrame();

        // Разбудить игровой поток, начать подготовку следующего кадра
        SubmitGameUpdate();

        // Сгенерировать команды для GPU, SwapBuffers
        RenderBackend();

        // Ожидаем окончание симуляции в игровом потоке (тем временем, GPU выполняет команды бэкенда)
        WaitSimulationIsDone();

        // Ожидаем выполнение команд GPU, чтобы исключить "input lag"
        WaitGPU();
    }

    GameThread.Join();

    rt_GameEngine->Deinitialize();

    GRenderBackend->CleanupFrame( &rt_FrameData );
    rt_FrameData.ReadIndex^=1;
    GRenderBackend->CleanupFrame( &rt_FrameData );
    rt_FrameData.Instances.Free();
    rt_FrameData.DbgVertices.Free();
    rt_FrameData.DbgIndices.Free();
    rt_FrameData.DbgCmds.Free();
}

static void Runtime( FCreateGameModuleCallback _CreateGameModule ) {

    rt_SysStartMicroseconds = StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count();
    rt_SysStartMilliseconds = rt_SysStartMicroseconds * 0.001;
    rt_SysStartSeconds = rt_SysStartMicroseconds * 0.000001;
    rt_SysFrameTimeStamp = rt_SysStartMicroseconds;

    rt_CreateGameModuleCallback = _CreateGameModule;

    rt_GameEngine = GetGameEngine();

    //SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        EmergencyExit();
    }

    GetCPUInfo( rt_CPUInfo );

    InitializeProcess();

    GLogger.SetMessageCallback( LoggerMessageCallback );

    GLogger.Printf( "CPU: %s\n", rt_CPUInfo.Intel ? "Intel" : "AMD" );
    GLogger.Print( "CPU Features:" );
    if ( rt_CPUInfo.MMX ) GLogger.Print( " MMX" );
    if ( rt_CPUInfo.x64 ) GLogger.Print( " x64" );
    if ( rt_CPUInfo.ABM ) GLogger.Print( " ABM" );
    if ( rt_CPUInfo.RDRAND ) GLogger.Print( " RDRAND" );
    if ( rt_CPUInfo.BMI1 ) GLogger.Print( " BMI1" );
    if ( rt_CPUInfo.BMI2 ) GLogger.Print( " BMI2" );
    if ( rt_CPUInfo.ADX ) GLogger.Print( " ADX" );
    if ( rt_CPUInfo.MPX ) GLogger.Print( " MPX" );
    if ( rt_CPUInfo.PREFETCHWT1 ) GLogger.Print( " PREFETCHWT1" );
    GLogger.Print( "\n" );
    GLogger.Print( "Simd 128 bit:" );
    if ( rt_CPUInfo.SSE ) GLogger.Print( " SSE" );
    if ( rt_CPUInfo.SSE2 ) GLogger.Print( " SSE2" );
    if ( rt_CPUInfo.SSE3 ) GLogger.Print( " SSE3" );
    if ( rt_CPUInfo.SSSE3 ) GLogger.Print( " SSSE3" );
    if ( rt_CPUInfo.SSE4a ) GLogger.Print( " SSE4a" );
    if ( rt_CPUInfo.SSE41 ) GLogger.Print( " SSE4.1" );
    if ( rt_CPUInfo.SSE42 ) GLogger.Print( " SSE4.2" );
    if ( rt_CPUInfo.AES ) GLogger.Print( " AES-NI" );
    if ( rt_CPUInfo.SHA ) GLogger.Print( " SHA" );
    GLogger.Print( "\n" );
    GLogger.Print( "Simd 256 bit:" );
    if ( rt_CPUInfo.AVX ) GLogger.Print( " AVX" );
    if ( rt_CPUInfo.XOP ) GLogger.Print( " XOP" );
    if ( rt_CPUInfo.FMA3 ) GLogger.Print( " FMA3" );
    if ( rt_CPUInfo.FMA4 ) GLogger.Print( " FMA4" );
    if ( rt_CPUInfo.AVX2 ) GLogger.Print( " AVX2" );
    GLogger.Print( "\n" );
    GLogger.Print( "Simd 512 bit:" );
    if ( rt_CPUInfo.AVX512_F ) GLogger.Print( " AVX512-F" );
    if ( rt_CPUInfo.AVX512_CD ) GLogger.Print( " AVX512-CD" );
    if ( rt_CPUInfo.AVX512_PF ) GLogger.Print( " AVX512-PF" );
    if ( rt_CPUInfo.AVX512_ER ) GLogger.Print( " AVX512-ER" );
    if ( rt_CPUInfo.AVX512_VL ) GLogger.Print( " AVX512-VL" );
    if ( rt_CPUInfo.AVX512_BW ) GLogger.Print( " AVX512-BW" );
    if ( rt_CPUInfo.AVX512_DQ ) GLogger.Print( " AVX512-DQ" );
    if ( rt_CPUInfo.AVX512_IFMA ) GLogger.Print( " AVX512-IFMA" );
    if ( rt_CPUInfo.AVX512_VBMI ) GLogger.Print( " AVX512-VBMI" );
    GLogger.Print( "\n" );
    GLogger.Print( "OS: " AN_OS_STRING "\n" );
    GLogger.Print( "OS Features:" );
    if ( rt_CPUInfo.OS_64bit ) GLogger.Print( " 64bit" );
    if ( rt_CPUInfo.OS_AVX ) GLogger.Print( " AVX" );
    if ( rt_CPUInfo.OS_AVX512 ) GLogger.Print( " AVX512" );
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
