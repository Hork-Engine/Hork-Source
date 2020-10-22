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

#pragma once

#include <Core/Public/Atomic.h>
#include <Core/Public/String.h>
#include <Core/Public/PodQueue.h>
#include <Core/Public/Utf8.h>
#include <Core/Public/Random.h>
#include "AsyncJobManager.h"
#include "GameModuleCallback.h"
#include "VertexMemoryGPU.h"

struct SVideoMode
{
    /** Horizontal position on display (read only) */
    int X;
    /** Vertical position on display (read only) */
    int Y;
    /** Horizontal position on display in windowed mode. */
    int WindowedX;
    /** Vertical position on display in windowed mode. */
    int WindowedY;
    /** Horizontal display resolution */
    int Width;
    /** Vertical display resolution */
    int Height;
    /** Video mode framebuffer width (for Retina displays, read only) */
    int FramebufferWidth;
    /** Video mode framebuffer width (for Retina displays, read only) */
    int FramebufferHeight;
    /** Physical monitor (read only) */
    int DisplayIndex;
    /** Display refresh rate (read only) */
    int RefreshRate;
    /** Display dots per inch (read only) */
    float DPI_X;
    /** Display dots per inch (read only) */
    float DPI_Y;
    /** Viewport aspect ratio scale (read only) */
    float AspectScale;
    /** Window opacity */
    float Opacity;
    /** Fullscreen or Windowed mode */
    bool bFullscreen;
    /** Move window to center of the screen. WindowedX and WindowedY will be ignored. */
    bool bCentrized;
    /** Render backend name */
    char Backend[32];
    /** Window title */
    char Title[128];
};

//struct SJoystick {
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//    int Id;
//};

/** CPU features */
struct SCPUInfo {
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

enum EInputAction {
    IA_RELEASE,
    IA_PRESS,
    IA_REPEAT
};

struct SKeyEvent {
    int Key;
    int Scancode;       // Not used, reserved for future
    int ModMask;
    int Action;         // EInputAction
};

struct SMouseButtonEvent {
    int Button;
    int ModMask;
    int Action;         // EInputAction
};

struct SMouseWheelEvent {
    double WheelX;
    double WheelY;
};

struct SMouseMoveEvent {
    float X;
    float Y;
};

struct SJoystickAxisEvent {
    int Joystick;
    int Axis;
    float Value;
};

struct SJoystickButtonEvent {
    int Joystick;
    int Button;
    int Action;         // EInputAction
};

//struct SJoystickStateEvent {
//    int Joystick;
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//};

struct SCharEvent {
    SWideChar UnicodeCharacter;
    int ModMask;
};

enum
{
    RENDER_FRONTEND_JOB_LIST,
    RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

struct SRenderFrame;

/** Runtime public class */
class ANGIE_API ARuntime
{
    AN_SINGLETON( ARuntime )

public:
    /** Is cheats allowed for the game. This allow to change runtime variables with flag VAR_CHEAT */
    bool bCheatsAllowed = true;

    /** Is game server. This allow to change runtime variables with flag VAR_SERVERONLY */
    bool bServerActive = false;

    /** Is in game. This blocks changing runtime variables with flag VAR_NOINGAME */
    bool bInGameStatus = false;

    /** Global random number generator */
    AMersenneTwisterRand Rand;

    /** Application command line args count */
    int GetArgc();

    /** Application command line args */
    const char * const *GetArgv();

    /** Check is argument exists in application command line. Return argument index or -1 if argument was not found. */
    int CheckArg( const char * _Arg );

    /** Return application working directory */
    AString const & GetWorkingDir();

    /** Return game module root directory */
    AString const & GetRootPath();

    /** Return application exacutable name */
    const char * GetExecutableName();

    /** Allocate frame memory */
    void * AllocFrameMem( size_t _SizeInBytes );

    /** Return frame memory size in bytes */
    size_t GetFrameMemorySize() const;

    /** Return used frame memory in bytes */
    size_t GetFrameMemoryUsed() const;

    /** Return used frame memory on previous frame, in bytes */
    size_t GetFrameMemoryUsedPrev() const;

    /** Return max frame memory usage since application start */
    size_t GetMaxFrameMemoryUsage() const;

    /** Get CPU info */
    SCPUInfo const & GetCPUInfo() const;

    /** Sleep current thread */
    void WaitSeconds( int _Seconds );

    /** Sleep current thread */
    void WaitMilliseconds( int _Milliseconds );

    /** Sleep current thread */
    void WaitMicroseconds( int _Microseconds );

    /** Get current time in seconds since application start */
    int64_t SysSeconds();

    /** Get current time in seconds since application start */
    double SysSeconds_d();

    /** Get current time in milliseconds since application start */
    int64_t SysMilliseconds();

    /** Get current time in milliseconds since application start */
    double SysMilliseconds_d();

    /** Get current time in microseconds since application start */
    int64_t SysMicroseconds();

    /** Get current time in microseconds since application start */
    double SysMicroseconds_d();

    /** Get time stamp at beggining of the frame */
    int64_t SysFrameTimeStamp();

    /** Get frame duration in microseconds */
    int64_t SysFrameDuration();

    /** Get current frame update number */
    int SysFrameNumber() const;

    /** Load dynamic library (.dll or .so) */
    void * LoadDynamicLib( const char * _LibraryName );

    /** Unload dynamic library (.dll or .so) */
    void UnloadDynamicLib( void * _Handle );

    /** Get address of procedure in dynamic library */
    void * GetProcAddress( void * _Handle, const char * _ProcName );

    /** Helper. Get address of procedure in dynamic library */
    template< typename T >
    bool GetProcAddress( void * _Handle, T ** _ProcPtr, const char * _ProcName ) {
        return nullptr != ( (*_ProcPtr) = (T *)GetProcAddress( _Handle, _ProcName ) );
    }

    /** Set clipboard text */
    void SetClipboard( const char * _Utf8String );

    /** Set clipboard text */
    void SetClipboard( AString const & _Clipboard ) { SetClipboard( _Clipboard.CStr() ); }

    /** Get clipboard text */
    const char * GetClipboard();

    /** Current video mode */
    SVideoMode const & GetVideoMode() const;

    /** Change a video mode */
    void PostChangeVideoMode( SVideoMode const & _DesiredMode );

    /** Terminate the application */
    void PostTerminateEvent();

    bool IsPendingTerminate();

    /** Begin a new frame */
    void NewFrame();

    /** Poll runtime events */
    void PollEvents();

    void SetCursorEnabled( bool _Enabled );

    bool IsCursorEnabled();

    void GetCursorPosition( int * _X, int * _Y );

    RenderCore::IDevice * GetRenderDevice();

    AVertexMemoryGPU * GetVertexMemoryGPU() { return VertexMemoryGPU; }
    AStreamedMemoryGPU * GetStreamedMemoryGPU() { return StreamedMemoryGPU; }

    void ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem );

private:
    int             NumArguments;
    char **         Arguments;

    AString         WorkingDir;
    AString         RootPath;
    char *          Executable;

    int64_t         StartSeconds;
    int64_t         StartMilliseconds;
    int64_t         StartMicroseconds;
    int64_t         FrameTimeStamp;
    int64_t         FrameDuration;
    int             FrameNumber;

    void *          FrameMemoryAddress;
    size_t          FrameMemorySize;
    size_t          FrameMemoryUsed;
    size_t          FrameMemoryUsedPrev;
    size_t          MaxFrameMemoryUsage;

    char *          Clipboard;

    struct SEntryDecl const * pModuleDecl;

    class IEngineInterface * Engine;

    SCPUInfo        CPUInfo;

    SVideoMode      VideoMode;
    SVideoMode      DesiredMode;
    bool            bResetVideoMode;

    TRef< RenderCore::IDevice > RenderDevice;
    RenderCore::IImmediateContext * pImmediateContext;

    TRef< AVertexMemoryGPU > VertexMemoryGPU;
    TRef< AStreamedMemoryGPU > StreamedMemoryGPU;

    bool            bTerminate;

    int             ProcessAttribute;

    void Run( struct SEntryDecl const & _EntryDecl );

    void EmergencyExit();

    void DisplayCriticalMessage( const char * _Message );

    void InitializeProcess();

    void DeinitializeProcess();

    void InitializeMemory();

    void DeinitializeMemory();

    void InitializeWorkingDirectory();

    void LoadConfigFile();

    void SetVideoMode( SVideoMode const & _DesiredMode );

    friend void Runtime( const char * _CommandLine, SEntryDecl const & _EntryDecl );
    friend void Runtime( int _Argc, char ** _Argv, SEntryDecl const & _EntryDecl );
};

extern ANGIE_API ARuntime & GRuntime;
extern ANGIE_API AAsyncJobManager GAsyncJobManager;
extern ANGIE_API AAsyncJobList * GRenderFrontendJobList;
extern ANGIE_API AAsyncJobList * GRenderBackendJobList;
