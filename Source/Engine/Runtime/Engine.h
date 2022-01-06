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

#include "RuntimeCommandProcessor.h"
#include "FrameLoop.h"
#include "Console.h"
#include "GameModuleInterface.h"
#include "FontAtlas.h"
#include "WDesktop.h"
#include "World.h"
#include "RenderFrontend.h"

#include <Renderer/RenderBackend.h>
#include <Core/Random.h>

enum
{
    RENDER_FRONTEND_JOB_LIST,
    RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

struct SEntryDecl;

class AEngineInstance : public IEventListener
{
    AN_FORBID_COPY(AEngineInstance)

public:
    ACanvas Canvas;

    /** Global random number generator */
    AMersenneTwisterRand Rand;

    TRef<AAsyncJobManager> AsyncJobManager;
    AAsyncJobList*         RenderFrontendJobList;
    AAsyncJobList*         RenderBackendJobList;

    AEngineInstance();

    /** Run the engine */
    void Run(SEntryDecl const& _EntryDecl);

    /** Helper. Create a new world */
    AWorld* CreateWorld() { return AWorld::CreateWorld(); }

    /** Helper. Destroy all existing worlds */
    void DestroyWorlds() { return AWorld::DestroyWorlds(); }

    /** Helper. Get all existing worlds */
    TPodVector<AWorld*> const& GetWorld() { return AWorld::GetWorlds(); }

    /** Get scale for Retina displays */
    Float2 const& GetRetinaScale() const { return RetinaScale; }

    /** Get window visible status */
    bool IsWindowVisible() const { return bIsWindowVisible; }

    /** Map coordinate from window space to monitor space */
    void MapWindowCoordinate(float& InOutX, float& InOutY) const;

    /** Map coordinate from monitor space to window space */
    void UnmapWindowCoordinate(float& InOutX, float& InOutY) const;

    /** Set hud desktop */
    void SetDesktop(WDesktop* _Desktop);

    /** Get hud desktop */
    WDesktop* GetDesktop() { return Desktop; }

    ARuntimeCommandProcessor& GetCommandProcessor() { return CommandProcessor; }

    ARenderBackend* GetRenderBackend() { return RenderBackend; }

    AVertexMemoryGPU* GetVertexMemoryGPU() { return VertexMemoryGPU; }

    /** Current video mode */
    SVideoMode const& GetVideoMode() const
    {
        return Window->GetVideoMode();
    }

    /** Change a video mode */
    void PostChangeVideoMode(SVideoMode const& _DesiredMode);

    /** Terminate the application */
    void PostTerminateEvent();

    bool IsPendingTerminate();

    void ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem);

    AFrameLoop* GetFrameLoop()
    {
        return FrameLoop;
    }

    /** Return application working directory */
    AString const& GetWorkingDir();

    /** Return game module root directory */
    AString const& GetRootPath();

    /** Return application exacutable name */
    const char* GetExecutableName();

    RenderCore::IDevice* GetRenderDevice();

private:
    /** IEventListener interface. */
    void OnKeyEvent(struct SKeyEvent const& _Event, double _TimeStamp) override;

    /** IEventListener interface. */
    void OnMouseButtonEvent(struct SMouseButtonEvent const& _Event, double _TimeStamp) override;

    /** IEventListener interface. */
    void OnMouseWheelEvent(struct SMouseWheelEvent const& _Event, double _TimeStamp) override;

    /** IEventListener interface. */
    void OnMouseMoveEvent(struct SMouseMoveEvent const& _Event, double _TimeStamp) override;

    /** IEventListener interface. */
    void OnJoystickAxisEvent(struct SJoystickAxisEvent const& _Event, double _TimeStamp) override;

    /** IEventListener interface. */
    void OnJoystickButtonEvent(struct SJoystickButtonEvent const& _Event, double _TimeStamp);

    /** IEventListener interface. */
    void OnCharEvent(struct SCharEvent const& _Event, double _TimeStamp) override;

    /** IEventListener interface. */
    void OnWindowVisible(bool _Visible) override;

    /** IEventListener interface. */
    void OnCloseEvent() override;

    /** IEventListener interface. */
    void OnResize() override;

    /** Update input */
    void UpdateInput();

    /** Used to debug some features. Must be removed from release. */
    void DeveloperKeys(struct SKeyEvent const& _Event);

    void DrawCanvas();

    //void UpdateImgui();

    void ShowStats();

    void InitializeDirectories();

    void LoadConfigFile();

    AString                   WorkingDir;
    AString                   RootPath;
    struct SEntryDecl const*  pModuleDecl;
    TRef<RenderCore::IDevice> RenderDevice;

    /** scale coordinates for Retina displays */
    Float2 RetinaScale;

    bool bIsWindowVisible = false;

    //class AImguiContext * ImguiContext;

    /** Frame update duration */
    float FrameDurationInSeconds = 0;

    IGameModule* GameModule;

    TRef<WDesktop> Desktop;

    ARuntimeCommandProcessor CommandProcessor;

    TRef<ARenderFrontend> Renderer;
    TRef<ARenderBackend>  RenderBackend;

    TUniqueRef<AResourceManager> ResourceManager;

    TRef<AFrameLoop>                 FrameLoop;
    TRef<RenderCore::IGenericWindow> Window;
    TRef<RenderCore::ISwapChain>     pSwapChain;
    TRef<AVertexMemoryGPU>           VertexMemoryGPU;

    AConsole Console;

    SVideoMode DesiredMode;
    bool       bPostChangeVideoMode = false;
    bool       bPostTerminateEvent  = false;

    bool bAllowInputEvents = false;
};

extern AEngineInstance* GEngine;
