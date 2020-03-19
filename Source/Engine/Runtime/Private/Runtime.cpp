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
#include <Core/Public/WindowsDefs.h>

#include "JoystickManager.h"
#include "MonitorManager.h"
#include "WindowManager.h"
#include "RuntimeMain.h"
#include "RuntimeEvents.h"

#ifdef AN_OS_LINUX
#include <dlfcn.h>
#endif

#include <chrono>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

ARuntime & GRuntime = ARuntime::Inst();
AAsyncJobManager GAsyncJobManager;
AAsyncJobList * GRenderFrontendJobList;
AAsyncJobList * GRenderBackendJobList;

ARuntime::ARuntime() {
}

SPhysicalMonitor const * ARuntime::GetPrimaryMonitor() {
    return GMonitorManager.GetPrimaryMonitor();
}

SPhysicalMonitor const * ARuntime::GetMonitor( int _Handle ) {
    APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
    return ( (unsigned)_Handle < physicalMonitors.Size() ) ? physicalMonitors[ _Handle ] : nullptr;
}

SPhysicalMonitor const * ARuntime::GetMonitor( const char * _MonitorName ) {
    return GMonitorManager.FindMonitor( _MonitorName );
}

bool ARuntime::IsMonitorConnected( int _Handle ) {
    bool bConnected = false;
    APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
    if ( (unsigned)_Handle < physicalMonitors.Size() ) {
        bConnected = physicalMonitors[ _Handle ]->Internal.Pointer != nullptr;
    }
    return bConnected;
}

void ARuntime::SetMonitorGammaCurve( int _Handle, float _Gamma ) {
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
}

void ARuntime::SetMonitorGamma( int _Handle, float _Gamma ) {
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
}

void ARuntime::SetMonitorGammaRamp( int _Handle, const unsigned short * _GammaRamp ) {
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        return;
    }

    Core::Memcpy( physMonitor->Internal.GammaRamp, _GammaRamp, sizeof( unsigned short ) * physMonitor->GammaRampSize * 3 );

    GMonitorManager.UpdateMonitorGamma( physMonitor );
}

void ARuntime::GetMonitorGammaRamp( int _Handle, unsigned short * _GammaRamp, int & _GammaRampSize ) {
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        Core::ZeroMem( _GammaRamp, sizeof( SPhysicalMonitorInternal::GammaRamp ) );
        return;
    }

    _GammaRampSize = physMonitor->GammaRampSize;

    Core::Memcpy( _GammaRamp, physMonitor->Internal.GammaRamp, sizeof( unsigned short ) * _GammaRampSize * 3 );
}

void ARuntime::RestoreMonitorGamma( int _Handle ) {
    SPhysicalMonitor * physMonitor = const_cast< SPhysicalMonitor * >( GetMonitor( _Handle ) );
    if ( !physMonitor ) {
        return;
    }

    Core::Memcpy( physMonitor->Internal.GammaRamp, physMonitor->Internal.InitialGammaRamp, sizeof( unsigned short ) * physMonitor->GammaRampSize * 3 );

    GMonitorManager.UpdateMonitorGamma( physMonitor );
}

SCPUInfo const & ARuntime::GetCPUInfo() const {
    return GRuntimeMain.CPUInfo;
}

int ARuntime::GetPhysicalMonitorsCount() {
    int count;
    APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
    count = physicalMonitors.Size();
    return count;
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

#define CHRONO_TIMER

int64_t ARuntime::SysSeconds() {
#ifdef CHRONO_TIMER
    return StdChrono::duration_cast< StdChrono::seconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - GRuntimeMain.SysStartSeconds;
#else
    return glfwGetTime();
#endif
}

double ARuntime::SysSeconds_d() {
#ifdef CHRONO_TIMER
    return SysMicroseconds() * 0.000001;
#else
    return glfwGetTime();
#endif
}

int64_t ARuntime::SysMilliseconds() {
#ifdef CHRONO_TIMER
    return StdChrono::duration_cast< StdChrono::milliseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - GRuntimeMain.SysStartMilliseconds;
#else
    return glfwGetTime() * 1000.0;
#endif
}

double ARuntime::SysMilliseconds_d() {
#ifdef CHRONO_TIMER
    return SysMicroseconds() * 0.001;
#else
    return glfwGetTime() * 1000.0;
#endif
}

int64_t ARuntime::SysMicroseconds() {
#ifdef CHRONO_TIMER
    return StdChrono::duration_cast< StdChrono::microseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count() - GRuntimeMain.SysStartMicroseconds;
#else
    return glfwGetTime() * 1000000.0;
#endif
}

double ARuntime::SysMicroseconds_d() {
#ifdef CHRONO_TIMER
    return static_cast< double >( SysMicroseconds() );
#else
    return glfwGetTime() * 1000000.0;
#endif
}

int64_t ARuntime::SysFrameTimeStamp() {
    return GRuntimeMain.SysFrameTimeStamp;
}

int64_t ARuntime::SysFrameDuration() {
    return GRuntimeMain.SysFrameDuration;
}

int ARuntime::SysFrameNumber() const {
    return GRuntimeMain.SysFrameNumber;
}

void * ARuntime::LoadDynamicLib( const char * _LibraryName ) {
    AString name( _LibraryName );
#if defined AN_OS_WIN32
    name.ReplaceExt( ".dll" );
    return ( void * )LoadLibraryA( name.CStr() );
#elif defined AN_OS_LINUX
    name.ReplaceExt( ".so" );
    return dlopen( name.CStr(), RTLD_LAZY | RTLD_GLOBAL );
#else
#error LoadDynamicLib not implemented under current platform
#endif
}

void ARuntime::UnloadDynamicLib( void * _Handle ) {
    if ( !_Handle ) {
        return;
    }
#if defined AN_OS_WIN32
    FreeLibrary( (HMODULE)_Handle );
#elif defined AN_OS_LINUX
    dlclose( _Handle );
#else
#error UnloadDynamicLib not implemented under current platform
#endif
}

void * ARuntime::GetProcAddress( void * _Handle, const char * _ProcName ) {
    if ( !_Handle ) {
        return nullptr;
    }
#if defined AN_OS_WIN32
    return ::GetProcAddress( (HMODULE)_Handle, _ProcName );
#elif defined AN_OS_LINUX
    return dlsym( _Handle, _ProcName );
#else
#error GetProcAddress not implemented under current platform
#endif
}

AString ARuntime::GetJoystickName( int _Joystick ) {
    return GJoystickManager.GetJoystickName( _Joystick );
}

int ARuntime::CheckArg( const char * _Arg ) {
    return GRuntimeMain.CheckArg( _Arg );
}

int ARuntime::GetArgc() {
    return GRuntimeMain.NumArguments;
}

const char * const *ARuntime::GetArgv() {
    return GRuntimeMain.Arguments;
}

AString const & ARuntime::GetWorkingDir() {
    return GRuntimeMain.WorkingDir;
}

const char * ARuntime::GetExecutableName() {
    return GRuntimeMain.Executable ? GRuntimeMain.Executable : "";
}

void ARuntime::SetClipboard( const char * _Utf8String ) {
    glfwSetClipboardString( NULL, _Utf8String );
}

const char * ARuntime::GetClipboard() {
    return glfwGetClipboardString( NULL );
}

AEventQueue * ARuntime::ReadEvents_GameThread() {
    return &GRuntimeEvents;
}

AEventQueue * ARuntime::WriteEvents_GameThread() {
    return &GGameEvents;
}

void * ARuntime::AllocFrameMem( size_t _SizeInBytes ) {
    if ( GRuntimeMain.FrameMemoryUsed + _SizeInBytes > GRuntimeMain.FrameMemorySize ) {
        GLogger.Printf( "AllocFrameMem: failed on allocation of %u bytes (available %u, total %u)\n", _SizeInBytes, GRuntimeMain.FrameMemorySize - GRuntimeMain.FrameMemoryUsed, GRuntimeMain.FrameMemorySize );
        return nullptr;
    }

    void * pMemory = (byte *)GRuntimeMain.FrameMemoryAddress + GRuntimeMain.FrameMemoryUsed;

    GRuntimeMain.FrameMemoryUsed += _SizeInBytes;
    GRuntimeMain.FrameMemoryUsed = Align( GRuntimeMain.FrameMemoryUsed, 16 );

    AN_ASSERT( IsAlignedPtr( pMemory, 16 ) );

    //GLogger.Printf( "Allocated %u, Used %u\n", _SizeInBytes, GRuntimeMain.FrameMemoryUsed );

    return pMemory;
}

size_t ARuntime::GetFrameMemorySize() const {
    return GRuntimeMain.FrameMemorySize;
}

size_t ARuntime::GetFrameMemoryUsed() const {
    return GRuntimeMain.FrameMemoryUsed;
}

size_t ARuntime::GetFrameMemoryUsedPrev() const {
    return GRuntimeMain.FrameMemoryUsedPrev;
}

size_t ARuntime::GetMaxFrameMemoryUsage() const {
    return GRuntimeMain.MaxFrameMemoryUsage;
}

void ARuntime::PostTerminateEvent() {
    GRuntimeMain.bTerminate = true;
}

bool ARuntime::IsPendingTerminate() {
    return GRuntimeMain.bTerminate;
}

void ARuntime::PumpEvents() {
    GInputEventsCount = 0;

    // Pump joystick events before any input
    GJoystickManager.PollEvents();

    // Pump input events and other
    glfwPollEvents();
}

void ARuntime::NewFrame() {
    int64_t prevTimeStamp = GRuntimeMain.SysFrameTimeStamp;
    GRuntimeMain.SysFrameTimeStamp = SysMicroseconds();
    if ( prevTimeStamp == GRuntimeMain.SysStartMicroseconds ) {
        GRuntimeMain.SysFrameDuration = 1000000.0 / 60;
    } else {
        GRuntimeMain.SysFrameDuration = GRuntimeMain.SysFrameTimeStamp - prevTimeStamp;
    }

    GRuntimeMain.SysFrameNumber++;

    // Keep memory statistics
    GRuntimeMain.MaxFrameMemoryUsage = Math::Max( GRuntimeMain.MaxFrameMemoryUsage, GRuntimeMain.FrameMemoryUsed );
    GRuntimeMain.FrameMemoryUsedPrev = GRuntimeMain.FrameMemoryUsed;

    // Free frame memory for new frame
    GRuntimeMain.FrameMemoryUsed = 0;

    GWindowManager.ProcessEvents( GGameEvents );
}

void ARuntime::SetCursorEnabled( bool _Enabled ) {
    GWindowManager.SetCursorEnabled( _Enabled );
}

#if 0

#define GLFW_HAT_CENTERED           0
#define GLFW_HAT_UP                 1
#define GLFW_HAT_RIGHT              2
#define GLFW_HAT_DOWN               4
#define GLFW_HAT_LEFT               8
#define GLFW_HAT_RIGHT_UP           (GLFW_HAT_RIGHT | GLFW_HAT_UP)
#define GLFW_HAT_RIGHT_DOWN         (GLFW_HAT_RIGHT | GLFW_HAT_DOWN)
#define GLFW_HAT_LEFT_UP            (GLFW_HAT_LEFT  | GLFW_HAT_UP)
#define GLFW_HAT_LEFT_DOWN          (GLFW_HAT_LEFT  | GLFW_HAT_DOWN)

#define GLFW_GAMEPAD_BUTTON_A               0
#define GLFW_GAMEPAD_BUTTON_B               1
#define GLFW_GAMEPAD_BUTTON_X               2
#define GLFW_GAMEPAD_BUTTON_Y               3
#define GLFW_GAMEPAD_BUTTON_LEFT_BUMPER     4
#define GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER    5
#define GLFW_GAMEPAD_BUTTON_BACK            6
#define GLFW_GAMEPAD_BUTTON_START           7
#define GLFW_GAMEPAD_BUTTON_GUIDE           8
#define GLFW_GAMEPAD_BUTTON_LEFT_THUMB      9
#define GLFW_GAMEPAD_BUTTON_RIGHT_THUMB     10
#define GLFW_GAMEPAD_BUTTON_DPAD_UP         11
#define GLFW_GAMEPAD_BUTTON_DPAD_RIGHT      12
#define GLFW_GAMEPAD_BUTTON_DPAD_DOWN       13
#define GLFW_GAMEPAD_BUTTON_DPAD_LEFT       14
#define GLFW_GAMEPAD_BUTTON_LAST            GLFW_GAMEPAD_BUTTON_DPAD_LEFT

#define GLFW_GAMEPAD_AXIS_LEFT_X        0
#define GLFW_GAMEPAD_AXIS_LEFT_Y        1
#define GLFW_GAMEPAD_AXIS_RIGHT_X       2
#define GLFW_GAMEPAD_AXIS_RIGHT_Y       3
#define GLFW_GAMEPAD_AXIS_LEFT_TRIGGER  4
#define GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER 5
#define GLFW_GAMEPAD_AXIS_LAST          GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER

#define GLFW_NO_ERROR               0
#define GLFW_NOT_INITIALIZED        0x00010001
#define GLFW_NO_CURRENT_CONTEXT     0x00010002
#define GLFW_INVALID_ENUM           0x00010003
#define GLFW_INVALID_VALUE          0x00010004
#define GLFW_OUT_OF_MEMORY          0x00010005
#define GLFW_API_UNAVAILABLE        0x00010006
#define GLFW_VERSION_UNAVAILABLE    0x00010007
#define GLFW_PLATFORM_ERROR         0x00010008
#define GLFW_FORMAT_UNAVAILABLE     0x00010009
#define GLFW_NO_WINDOW_CONTEXT      0x0001000A

#define GLFW_SCALE_TO_MONITOR       0x0002200C

#define GLFW_COCOA_RETINA_FRAMEBUFFER 0x00023001
#define GLFW_COCOA_GRAPHICS_SWITCHING 0x00023003

#define GLFW_NATIVE_CONTEXT_API     0x00036001
#define GLFW_EGL_CONTEXT_API        0x00036002
#define GLFW_OSMESA_CONTEXT_API     0x00036003

#define GLFW_JOYSTICK_HAT_BUTTONS   0x00050001
#define GLFW_COCOA_CHDIR_RESOURCES  0x00051001
#define GLFW_COCOA_MENUBAR          0x00051002


typedef struct GLFWgamepadstate
{
    /*! The states of each [gamepad button](@ref gamepad_buttons), `GLFW_PRESS`
     *  or `GLFW_RELEASE`.
     */
    unsigned char buttons[15];
    /*! The states of each [gamepad axis](@ref gamepad_axes), in the range -1.0
     *  to 1.0 inclusive.
     */
    float axes[6];
} GLFWgamepadstate;

GLFWAPI void glfwGetMonitorContentScale(GLFWmonitor* monitor, float* xscale, float* yscale);
GLFWAPI void glfwWindowHintString(int hint, const char* value); GLFW_COCOA_FRAME_NAME/GLFW_X11_CLASS_NAME/GLFW_X11_INSTANCE_NAME
GLFWAPI void glfwGetWindowContentScale(GLFWwindow* window, float* xscale, float* yscale);
GLFWAPI void glfwRequestWindowAttention(GLFWwindow* window);
GLFWAPI GLFWwindowcontentscalefun glfwSetWindowContentScaleCallback(GLFWwindow* window, GLFWwindowcontentscalefun cbfun);

GLFWAPI const unsigned char* glfwGetJoystickHats(int jid, int* count);
GLFWAPI const char* glfwGetJoystickGUID(int jid);
GLFWAPI void glfwSetJoystickUserPointer(int jid, void* pointer);
GLFWAPI void* glfwGetJoystickUserPointer(int jid);
GLFWAPI int glfwUpdateGamepadMappings(const char* string);
GLFWAPI const char* glfwGetGamepadName(int jid);
GLFWAPI int glfwGetGamepadState(int jid, GLFWgamepadstate* state);

// Cursor state
#define GLFW_CURSOR_HIDDEN          0x00034002

// WINDOW
GLFWAPI int glfwWindowShouldClose(GLFWwindow* window);
GLFWAPI void glfwSetWindowShouldClose(GLFWwindow* window, int value);
GLFWAPI void glfwSetWindowIcon(GLFWwindow* window, int count, const GLFWimage* images);
GLFWAPI void glfwGetWindowPos(GLFWwindow* window, int* xpos, int* ypos);
GLFWAPI void glfwGetWindowSize(GLFWwindow* window, int* width, int* height);
GLFWAPI void glfwGetWindowFrameSize(GLFWwindow* window, int* left, int* top, int* right, int* bottom);

// EVENTS
GLFWAPI void glfwWaitEvents(void);
GLFWAPI void glfwWaitEventsTimeout(double timeout);
GLFWAPI void glfwPostEmptyEvent(void);

// INPUTS
GLFWAPI void glfwSetCursorPos(GLFWwindow* window, double xpos, double ypos);

// CURSOR
GLFWAPI GLFWcursor* glfwCreateCursor(const GLFWimage* image, int xhot, int yhot);
GLFWAPI GLFWcursor* glfwCreateStandardCursor(int shape); GLFW_ARROW_CURSOR/GLFW_IBEAM_CURSOR/GLFW_CROSSHAIR_CURSOR/GLFW_HAND_CURSOR/GLFW_HRESIZE_CURSOR/GLFW_VRESIZE_CURSOR
GLFWAPI void glfwDestroyCursor(GLFWcursor* cursor);
GLFWAPI void glfwSetCursor(GLFWwindow* window, GLFWcursor* cursor);

// JOYSTICK
GLFWAPI int glfwJoystickPresent(int joy);

// TIMER
GLFWAPI uint64_t glfwGetTimerValue(void);
GLFWAPI uint64_t glfwGetTimerFrequency(void);

// VULKAN
GLFWAPI const char** glfwGetRequiredInstanceExtensions(uint32_t* count);
GLFWAPI GLFWvkproc glfwGetInstanceProcAddress(VkInstance instance, const char* procname);
GLFWAPI int glfwGetPhysicalDevicePresentationSupport(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);

static Int2 GetCursorPos() {
#if defined( AN_OS_WIN32 )
    POINT point;
    ::GetCursorPos( &point );
    return Int2( point.x, point.y );
#elif defined( AN_OS_LINUX )
    // TODO: check this
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int mask;
    XQueryPointer( glfwGetX11Display(), RootWindow( glfwGetX11Display(), DefaultScreen( glfwGetX11Display() ) ),
                   &root, &child,
                   &rootX, &rootY, &childX, &childY,
                   &mask );
    return Int2( rootX, rootY );
#else
    #warning "GetCursorPos not implemented under current platform"
#endif
}

#endif
