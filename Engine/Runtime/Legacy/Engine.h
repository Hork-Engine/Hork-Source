#if 0
/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include "UI/UIManager.h"
#include "RenderFrontend.h"
#include "AsyncJobManager.h"
#include "InputSystem.h"

#include <Engine/Renderer/RenderBackend.h>
#include <Engine/Core/Random.h>
#include <Engine/Core/CommandProcessor.h>
#include <Engine/ECSRuntime/Resources/MaterialManager.h>
#include <Engine/ECSRuntime/World.h>
#include <Engine/ECSRuntime/StateMachine.h>

#include "World/World.h"

namespace JPH
{
class TempAllocator;
}

HK_NAMESPACE_BEGIN

enum
{
    RENDER_FRONTEND_JOB_LIST,
    //RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

class Engine : public IEventListener
{
    HK_FORBID_COPY(Engine)

public:
    TRef<AsyncJobManager> pAsyncJobManager;
    AsyncJobList*         pRenderFrontendJobList;
    //AsyncJobList*         pRenderBackendJobList;

#ifdef HK_OS_WIN32
    Engine();
#else
    Engine(int argc, char** argv);
#endif
    virtual ~Engine();

    /** Run the engine */
    void RunMainLoop();

    int ExitCode() const;

    /** Helper. Create a new world */
    World* CreateWorld() { return World::CreateWorld(); }

    /** Helper. Destroy all existing worlds */
    void DestroyWorlds() { return World::DestroyWorlds(); }

    World_ECS* CreateWorldECS();

    void DestroyWorldECS(World_ECS* world);

    /** Helper. Get all existing worlds */
    TVector<World*> const& GetWorlds() { return World::GetWorlds(); }

    /** Get window visible status */
    bool IsWindowVisible() const { return m_bIsWindowVisible; }

    /** Map coordinate from window space to monitor space */
    void MapWindowCoordinate(float& InOutX, float& InOutY) const;

    /** Map coordinate from monitor space to window space */
    void UnmapWindowCoordinate(float& InOutX, float& InOutY) const;

    CommandProcessor& GetCommandProcessor() { return m_CommandProcessor; }

    RenderBackend* GetRenderBackend() { return m_RenderBackend; }

    VertexMemoryGPU* GetVertexMemoryGPU() { return m_VertexMemoryGPU; }

    /** Current video mode */
    DisplayVideoMode const& GetVideoMode() const
    {
        return m_Window->GetVideoMode();
    }

    /** Change a video mode */
    void PostChangeVideoMode(DisplayVideoMode const& _DesiredMode);

    /** Terminate the application */
    void PostTerminateEvent();

    bool IsPendingTerminate();

    void ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem);

    /** Return application working directory */
    String const& GetWorkingDir();

    /** Return game module root directory */
    String const& GetRootPath();

private:
    /** IEventListener interface. */
    void OnKeyEvent(KeyEvent const& event) override;

    /** IEventListener interface. */
    void OnMouseButtonEvent(MouseButtonEvent const& event) override;

    /** IEventListener interface. */
    void OnMouseWheelEvent(MouseWheelEvent const& event) override;

    /** IEventListener interface. */
    void OnMouseMoveEvent(MouseMoveEvent const& event) override;

    /** IEventListener interface. */
    void OnJoystickAxisEvent(JoystickAxisEvent const& event) override;

    /** IEventListener interface. */
    void OnJoystickButtonEvent(JoystickButtonEvent const& event);

    /** IEventListener interface. */
    void OnCharEvent(CharEvent const& event) override;

    /** IEventListener interface. */
    void OnWindowVisible(bool bVisible) override;

    /** IEventListener interface. */
    void OnCloseEvent() override;

    /** IEventListener interface. */
    void OnResize() override;

    /** Used to debug some features. Must be removed from release. */
    void DeveloperKeys(KeyEvent const& event);

    void DrawCanvas();

    void ShowStats();

    void InitializeDirectories();

    void LoadConfigFile();

    void SaveMemoryStats();

    String                   m_WorkingDir;
    String                   m_RootPath;
    TRef<RenderCore::IDevice> m_RenderDevice;

    InputSystem m_InputSystem;

    TUniqueRef<Canvas> m_Canvas;

    bool m_bIsWindowVisible = false;

    /** Frame update duration */
    float m_FrameDurationInSeconds = 0;

    GameModule* m_GameModule;

    TUniqueRef<UIManager> m_UIManager;

    CommandProcessor m_CommandProcessor;

    TRef<RenderFrontend> m_Renderer;
    TRef<RenderBackend>  m_RenderBackend;

    TRef<FrameLoop>                 m_FrameLoop;
    TRef<RenderCore::IGenericWindow> m_Window;
    TRef<RenderCore::ISwapChain>     m_pSwapChain;
    TRef<VertexMemoryGPU>           m_VertexMemoryGPU;

    DisplayVideoMode m_DesiredMode;
    bool       m_bPostChangeVideoMode = false;
    bool       m_bPostTerminateEvent  = false;

    bool m_bAllowInputEvents = false;


    TVector<World_ECS*> m_WorldsECS;

    StateMachine m_StateMachine;
};

extern Engine* GEngine;

namespace Global
{

extern Archive GEmbeddedResourcesArch;
extern ResourceManager* GResourceManager;
extern MaterialManager* GMaterialManager;
extern FontHandle GDefaultFontHandle;
extern FontResource* GDefaultFont;
extern StateMachine* GStateMachine;
extern RenderCore::IDevice* GRenderDevice;
extern VertexMemoryGPU* GVertexMemoryGPU;
extern Float2 GRetinaScale;
extern FrameLoop* GFrameLoop;
extern InputSystem* GInputSystem;
extern MersenneTwisterRand GRand;

}

HK_NAMESPACE_END
#endif
