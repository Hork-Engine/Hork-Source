/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Platform/Public/Atomic.h>
#include <Platform/Public/Memory/LinearAllocator.h>
#include <Core/Public/String.h>
#include <Core/Public/Utf8.h>
#include <Core/Public/Random.h>
#include <Containers/Public/PodQueue.h>
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
    int DisplayId;
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

//struct SJoystick
//{
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//    int Id;
//};

enum EInputAction
{
    IA_RELEASE,
    IA_PRESS,
    IA_REPEAT
};

struct SKeyEvent
{
    int Key;
    int Scancode; // Not used, reserved for future
    int ModMask;
    int Action; // EInputAction
};

struct SMouseButtonEvent
{
    int Button;
    int ModMask;
    int Action; // EInputAction
};

struct SMouseWheelEvent
{
    double WheelX;
    double WheelY;
};

struct SMouseMoveEvent
{
    float X;
    float Y;
};

struct SJoystickAxisEvent
{
    int   Joystick;
    int   Axis;
    float Value;
};

struct SJoystickButtonEvent
{
    int Joystick;
    int Button;
    int Action; // EInputAction
};

//struct SJoystickStateEvent
//{
//    int Joystick;
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//};

struct SCharEvent
{
    SWideChar UnicodeCharacter;
    int       ModMask;
};

enum
{
    RENDER_FRONTEND_JOB_LIST,
    RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

struct SRenderFrame;

/** Runtime public class */
class ARuntime
{
public:
    /** Is cheats allowed for the game. This allow to change runtime variables with flag VAR_CHEAT */
    bool bCheatsAllowed = true;

    /** Is game server. This allow to change runtime variables with flag VAR_SERVERONLY */
    bool bServerActive = false;

    /** Is in game. This blocks changing runtime variables with flag VAR_NOINGAME */
    bool bInGameStatus = false;

    /** Global random number generator */
    AMersenneTwisterRand Rand;

    TRef<AAsyncJobManager> AsyncJobManager;
    AAsyncJobList*         RenderFrontendJobList;
    AAsyncJobList*         RenderBackendJobList;

    ARuntime(struct SEntryDecl const& _EntryDecl);

    ~ARuntime();

    /** Return application working directory */
    AString const& GetWorkingDir();

    /** Return game module root directory */
    AString const& GetRootPath();

    /** Return application exacutable name */
    const char* GetExecutableName();

    /** Allocate frame memory */
    void* AllocFrameMem(size_t _SizeInBytes);

    /** Return frame memory size in bytes */
    size_t GetFrameMemorySize() const;

    /** Return used frame memory in bytes */
    size_t GetFrameMemoryUsed() const;

    /** Return used frame memory on previous frame, in bytes */
    size_t GetFrameMemoryUsedPrev() const;

    /** Return max frame memory usage since application start */
    size_t GetMaxFrameMemoryUsage() const;

    /** Get time stamp at beggining of the frame */
    int64_t SysFrameTimeStamp();

    /** Get frame duration in microseconds */
    int64_t SysFrameDuration();

    /** Get current frame update number */
    int SysFrameNumber() const;

    /** Current video mode */
    SVideoMode const& GetVideoMode() const;

    /** Change a video mode */
    void PostChangeVideoMode(SVideoMode const& _DesiredMode);

    /** Terminate the application */
    void PostTerminateEvent();

    bool IsPendingTerminate();

    /** Begin a new frame */
    void NewFrame();

    /** Poll runtime events */
    void PollEvents();

    void SetCursorEnabled(bool _Enabled);

    bool IsCursorEnabled();

    void GetCursorPosition(int* _X, int* _Y);

    RenderCore::IDevice*           GetRenderDevice();
    RenderCore::IImmediateContext* GetImmediateContext() { return pImmediateContext; }
    RenderCore::ISwapChain*        GetSwapChain() { return pSwapChain; }

    AVertexMemoryGPU*   GetVertexMemoryGPU() { return VertexMemoryGPU; }
    AStreamedMemoryGPU* GetStreamedMemoryGPU() { return StreamedMemoryGPU; }

    void ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem);

    /** Zip archive of embedded content */
    AArchive const& GetEmbeddedResources();

private:
    AString WorkingDir;
    AString RootPath;
    char*   Executable;

    int64_t FrameTimeStamp;
    int64_t FrameDuration;
    int     FrameNumber;

    TLinearAllocator<> FrameMemory;
    size_t             FrameMemoryUsedPrev;
    size_t             MaxFrameMemoryUsage;

    struct SEntryDecl const* pModuleDecl;

    class IEngineInterface* Engine;

    SVideoMode VideoMode;
    SVideoMode DesiredMode;
    bool       bPostChangeVideoMode;

    TRef<RenderCore::IDevice>           RenderDevice;
    TRef<RenderCore::IImmediateContext> pImmediateContext;
    TRef<RenderCore::ISwapChain>        pSwapChain;

    TRef<AVertexMemoryGPU>   VertexMemoryGPU;
    TRef<AStreamedMemoryGPU> StreamedMemoryGPU;

    bool bPostTerminateEvent;

    TUniqueRef<AArchive> EmbeddedResourcesArch;

    void Run();

    void InitializeWorkingDirectory();

    void LoadConfigFile();

    void SetVideoMode(SVideoMode const& _DesiredMode);

    void ClearJoystickAxes(int _JoystickNum, double _TimeStamp);
    void UnpressKeysAndButtons();
    void UnpressJoystickButtons(int _JoystickNum, double _TimeStamp);

#ifdef AN_OS_WIN32
    friend void RunEngine(SEntryDecl const& _EntryDecl);
#else
    friend void RunEngine(int _Argc, char** _Argv, SEntryDecl const& _EntryDecl);
#endif
};

extern ARuntime* GRuntime;
