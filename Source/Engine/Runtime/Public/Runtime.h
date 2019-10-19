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

#pragma once

#include <Engine/Core/Public/Atomic.h>
#include <Engine/Core/Public/String.h>
#include <Engine/Core/Public/PodQueue.h>
#include <Engine/Core/Public/Utf8.h>
#include "AsyncJobManager.h"

//enum EVerticalSyncMode {
//    VSync_Disabled,
//    VSync_Adaptive,
//    VSync_Fixed,
//    VSync_Half
//};

struct FMonitorVideoMode {
    int Width;
    int Height;
    int RedBits;
    int GreenBits;
    int BlueBits;
    int RefreshRate;
};

#define GAMMA_RAMP_SIZE 1024

struct FPhysicalMonitorInternal {
    void * Pointer;
    unsigned short InitialGammaRamp[ GAMMA_RAMP_SIZE * 3 ];
    unsigned short GammaRamp[ GAMMA_RAMP_SIZE * 3 ];
    bool bGammaRampDirty;
};

struct FPhysicalMonitor {
    char MonitorName[128];
    int PhysicalWidthMM;
    int PhysicalHeightMM;
    float DPI_X;
    float DPI_Y;
    int PositionX;
    int PositionY;
    int GammaRampSize;
    FPhysicalMonitorInternal Internal; // for internal use
    int VideoModesCount;
    FMonitorVideoMode VideoModes[1];
};

struct FJoystick {
    int NumAxes;
    int NumButtons;
    bool bGamePad;
    bool bConnected;
    int Id;
};

enum EEventType {
    ET_Unknown,
    ET_RuntimeUpdateEvent,
    ET_KeyEvent,
    ET_MouseButtonEvent,
    ET_MouseWheelEvent,
    ET_MouseMoveEvent,
    ET_JoystickAxisEvent,
    ET_JoystickButtonEvent,
    ET_JoystickStateEvent,
    ET_CharEvent,
    ET_MonitorConnectionEvent,
    ET_CloseEvent,
    ET_FocusEvent,
    ET_VisibleEvent,
    ET_WindowPosEvent,
    ET_ChangedVideoModeEvent,

    ET_SetVideoModeEvent,
    ET_SetWindowDefsEvent,
    ET_SetWindowPosEvent,
    ET_SetInputFocusEvent,
    //ET_SetRenderFeaturesEvent,
    ET_SetCursorModeEvent
};

enum EInputEvent {
    IE_Release,
    IE_Press,
    IE_Repeat
};

struct FRuntimeUpdateEvent {
    int InputEventCount;
};

struct FKeyEvent {
    int Key;
    int Scancode;       // Not used, reserved for future
    int ModMask;
    int Action;         // EInputEvent
};

struct FMouseButtonEvent {
    int Button;
    int ModMask;
    int Action;         // EInputEvent
};

struct FMouseWheelEvent {
    double WheelX;
    double WheelY;
};

struct FMouseMoveEvent {
    float X;
    float Y;
};

struct FJoystickAxisEvent {
    int Joystick;
    int Axis;
    float Value;
};

struct FJoystickButtonEvent {
    int Joystick;
    int Button;
    int Action;         // EInputEvent
};

struct FJoystickStateEvent {
    int Joystick;
    int NumAxes;
    int NumButtons;
    bool bGamePad;
    bool bConnected;
};

struct FCharEvent {
    FWideChar UnicodeCharacter;
    int ModMask;
};

struct FMonitorConnectionEvent {
    int Handle;
    bool bConnected;
};

struct FCloseEvent {
};

struct FFocusEvent {
    bool bFocused;
};

struct FVisibleEvent {
    bool bVisible;
};

struct FWindowPosEvent {
    int PositionX;
    int PositionY;
};

struct FChangedVideoModeEvent {
    unsigned short Width;
    unsigned short Height;
    unsigned short PhysicalMonitor;
    byte RefreshRate;
    bool bFullscreen;
    char Backend[32];
};

struct FSetVideoModeEvent {
    unsigned short Width;
    unsigned short Height;
    unsigned short PhysicalMonitor;
    byte RefreshRate;
    bool bFullscreen;
    char Backend[32];
};

struct FSetWindowDefsEvent {
    byte Opacity;
    bool bDecorated;
    bool bAutoIconify;
    bool bFloating;
    char Title[32];
};

struct FSetWindowPosEvent {
    int PositionX;
    int PositionY;
};

struct FSetInputFocusEvent {

};

//struct FSetRenderFeaturesEvent {
//    int VSyncMode;
//};

struct FSetCursorModeEvent {
    bool bDisabledCursor;
};

struct FEvent {
    int Type;
    double TimeStamp;   // in seconds

    union {
        // Runtime output events
        FRuntimeUpdateEvent RuntimeUpdateEvent;
        FKeyEvent KeyEvent;
        FMouseButtonEvent MouseButtonEvent;
        FMouseWheelEvent MouseWheelEvent;
        FMouseMoveEvent MouseMoveEvent;
        FCharEvent CharEvent;
        FJoystickAxisEvent JoystickAxisEvent;
        FJoystickButtonEvent JoystickButtonEvent;
        FJoystickStateEvent JoystickStateEvent;
        FMonitorConnectionEvent MonitorConnectionEvent;
        FCloseEvent CloseEvent;
        FFocusEvent FocusEvent;
        FVisibleEvent VisibleEvent;
        FWindowPosEvent WindowPosEvent;
        FChangedVideoModeEvent ChangedVideoModeEvent;

        // Runtime input events
        FSetVideoModeEvent SetVideoModeEvent;
        FSetWindowDefsEvent SetWindowDefsEvent;
        FSetWindowPosEvent SetWindowPosEvent;
        FSetInputFocusEvent SetInputFocusEvent;
        //FSetRenderFeaturesEvent SetRenderFeaturesEvent;
        FSetCursorModeEvent SetCursorModeEvent;
    } Data;
};

using FEventQueue = TPodQueue< FEvent >;

enum {
    RENDER_FRONTEND_JOB_LIST,
    RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

#define MIN_DISPLAY_WIDTH 1
#define MIN_DISPLAY_HEIGHT 1

struct FRenderFrame;

class ANGIE_API FRuntime {
    AN_SINGLETON( FRuntime )

public:
    bool bCheatsAllowed = true;

    bool bServerActive = false;

    bool bInGameStatus = false;

    int GetArgc();

    const char * const *GetArgv();

    int CheckArg( const char * _Arg );

    FString const & GetWorkingDir();

    const char * GetExecutableName();

    FEventQueue * ReadEvents_GameThread();

    FEventQueue * WriteEvents_GameThread();

    FRenderFrame * GetFrameData();

    void * AllocFrameMem( size_t _SizeInBytes );

    size_t GetFrameMemorySize() const;

    size_t GetFrameMemoryUsed() const;

    size_t GetFrameMemoryUsedPrev() const;

    // Is vertical synchronization control supported
    //bool IsVSyncSupported();

    // Is vertical synchronization tearing supported
    //bool IsAdaptiveVSyncSupported();

    int GetPhysicalMonitorsCount();

    FPhysicalMonitor const * GetPrimaryMonitor();

    FPhysicalMonitor const * GetMonitor( int _Handle );

    FPhysicalMonitor const * GetMonitor( const char * _MonitorName );

    bool IsMonitorConnected( int _Handle );

    void SetMonitorGammaCurve( int _Handle, float _Gamma );

    void SetMonitorGamma( int _Handle, float _Gamma );

    void SetMonitorGammaRamp( int _Handle, const unsigned short * _GammaRamp );

    void GetMonitorGammaRamp( int _Handle, unsigned short * _GammaRamp, int & _GammaRampSize );

    void RestoreMonitorGamma( int _Handle );

    FString GetJoystickName( int _Joystick );

    FString GetJoystickName( const FJoystick * _Joystick ) { return GetJoystickName( _Joystick->Id ); }

    void WaitSeconds( int _Seconds );

    void WaitMilliseconds( int _Milliseconds );

    void WaitMicroseconds( int _Microseconds );

    int64_t SysSeconds();
    double SysSeconds_d();

    int64_t SysMilliseconds();
    double SysMilliseconds_d();

    int64_t SysMicroseconds();
    double SysMicroseconds_d();

    int64_t SysFrameTimeStamp();

    void * LoadDynamicLib( const char * _LibraryName );

    void UnloadDynamicLib( void * _Handle );

    void * GetProcAddress( void * _Handle, const char * _ProcName );

    template< typename T >
    bool GetProcAddress( void * _Handle, T ** _ProcPtr, const char * _ProcName ) {
        return nullptr != ( (*_ProcPtr) = (T *)GetProcAddress( _Handle, _ProcName ) );
    }

    void SetClipboard( const char * _Utf8String );
    void SetClipboard( FString const & _Clipboard ) { SetClipboard( _Clipboard.ToConstChar() ); }

    const char * GetClipboard();
};

extern ANGIE_API FRuntime & GRuntime;
extern ANGIE_API FAsyncJobManager GAsyncJobManager;
extern ANGIE_API FAsyncJobList * GRenderFrontendJobList;
extern ANGIE_API FAsyncJobList * GRenderBackendJobList;
