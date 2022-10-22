/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "FrameLoop.h"
#include "GameModule.h"
#include "Font.h"
#include "UI/UIManager.h"
#include "World.h"
#include "RenderFrontend.h"
#include "AudioSystem.h"
#include "AsyncJobManager.h"

#include <Renderer/RenderBackend.h>
#include <Core/Random.h>
#include <Core/CommandProcessor.h>

enum
{
    RENDER_FRONTEND_JOB_LIST,
    RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

struct SEntryDecl;

class AResourceManager;

class AEngine : public IEventListener
{
    HK_FORBID_COPY(AEngine)

public:
    /** Global random number generator */
    AMersenneTwisterRand Rand;

    TRef<AAsyncJobManager> AsyncJobManager;
    AAsyncJobList*         RenderFrontendJobList;
    AAsyncJobList*         RenderBackendJobList;

    AEngine();

    /** Run the engine */
    void Run(SEntryDecl const& entryDecl);

    /** Helper. Create a new world */
    AWorld* CreateWorld() { return AWorld::CreateWorld(); }

    /** Helper. Destroy all existing worlds */
    void DestroyWorlds() { return AWorld::DestroyWorlds(); }

    /** Helper. Get all existing worlds */
    TVector<AWorld*> const& GetWorlds() { return AWorld::GetWorlds(); }

    /** Get scale for Retina displays */
    Float2 const& GetRetinaScale() const { return m_RetinaScale; }

    /** Get window visible status */
    bool IsWindowVisible() const { return m_bIsWindowVisible; }

    /** Map coordinate from window space to monitor space */
    void MapWindowCoordinate(float& InOutX, float& InOutY) const;

    /** Map coordinate from monitor space to window space */
    void UnmapWindowCoordinate(float& InOutX, float& InOutY) const;

    ACommandProcessor& GetCommandProcessor() { return m_CommandProcessor; }

    ARenderBackend* GetRenderBackend() { return m_RenderBackend; }

    AVertexMemoryGPU* GetVertexMemoryGPU() { return m_VertexMemoryGPU; }

    /** Current video mode */
    SVideoMode const& GetVideoMode() const
    {
        return m_Window->GetVideoMode();
    }

    /** Change a video mode */
    void PostChangeVideoMode(SVideoMode const& _DesiredMode);

    /** Terminate the application */
    void PostTerminateEvent();

    bool IsPendingTerminate();

    void ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem);

    AFrameLoop* GetFrameLoop()
    {
        return m_FrameLoop;
    }

    /** Return application working directory */
    AString const& GetWorkingDir();

    /** Return game module root directory */
    AString const& GetRootPath();

    /** Return application exacutable name */
    const char* GetExecutableName();

    RenderCore::IDevice* GetRenderDevice();

    AAudioSystem* GetAudioSystem()
    {
        return &m_AudioSystem;
    }

    AResourceManager* GetResourceManager()
    {
        return m_ResourceManager.GetObject();
    }

private:
    /** IEventListener interface. */
    void OnKeyEvent(SKeyEvent const& event, double timeStamp) override;

    /** IEventListener interface. */
    void OnMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp) override;

    /** IEventListener interface. */
    void OnMouseWheelEvent(SMouseWheelEvent const& event, double timeStamp) override;

    /** IEventListener interface. */
    void OnMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp) override;

    /** IEventListener interface. */
    void OnJoystickAxisEvent(SJoystickAxisEvent const& event, double timeStamp) override;

    /** IEventListener interface. */
    void OnJoystickButtonEvent(SJoystickButtonEvent const& event, double timeStamp);

    /** IEventListener interface. */
    void OnCharEvent(SCharEvent const& event, double timeStamp) override;

    /** IEventListener interface. */
    void OnWindowVisible(bool bVisible) override;

    /** IEventListener interface. */
    void OnCloseEvent() override;

    /** IEventListener interface. */
    void OnResize() override;

    /** Update input */
    void UpdateInput();

    /** Used to debug some features. Must be removed from release. */
    void DeveloperKeys(SKeyEvent const& event);

    void DrawCanvas();

    void ShowStats();

    void InitializeDirectories();

    void LoadConfigFile();

    void SaveMemoryStats();

    AString                   m_WorkingDir;
    AString                   m_RootPath;
    SEntryDecl const*         m_pModuleDecl;
    TRef<RenderCore::IDevice> m_RenderDevice;

    TUniqueRef<ACanvas> m_Canvas;

    /** scale coordinates for Retina displays */
    Float2 m_RetinaScale;

    bool m_bIsWindowVisible = false;

    /** Frame update duration */
    float m_FrameDurationInSeconds = 0;

    AGameModule* m_GameModule;

    TUniqueRef<UIManager> m_UIManager;

    ACommandProcessor m_CommandProcessor;

    TRef<ARenderFrontend> m_Renderer;
    TRef<ARenderBackend>  m_RenderBackend;

    TUniqueRef<AResourceManager> m_ResourceManager;

    TRef<AFrameLoop>                 m_FrameLoop;
    TRef<RenderCore::IGenericWindow> m_Window;
    TRef<RenderCore::ISwapChain>     m_pSwapChain;
    TRef<AVertexMemoryGPU>           m_VertexMemoryGPU;

    AAudioSystem m_AudioSystem;

    SVideoMode m_DesiredMode;
    bool       m_bPostChangeVideoMode = false;
    bool       m_bPostTerminateEvent  = false;

    bool m_bAllowInputEvents = false;
};

extern AEngine* GEngine;
