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

#include <Engine/Core/CoreApplication.h>
#include <Engine/Core/CommandProcessor.h>
#include <Engine/Core/IO.h>
#include <Engine/Core/Random.h>

#include <Engine/ECSRuntime/Resources/MaterialManager.h>
#include <Engine/ECSRuntime/StateMachine.h>
#include <Engine/Runtime/UI/UIManager.h>

#include "FrameLoop.h"
#include "Event.h"
#include "InputSystem.h"
#include "RenderFrontend.h"
#include <Engine/Renderer/RenderBackend.h>

HK_NAMESPACE_BEGIN

class World_ECS;
class AsyncJobManager;
class AsyncJobList;

class GameApplication : public CoreApplication, public IEventListener
{
public:
    GameApplication(ArgumentPack const& args, StringView title);
    ~GameApplication();

    World_ECS* CreateWorld();

    void DestroyWorld(World_ECS* world);

    /** Set main window video mode. */
    void PostChangeVideoMode(DisplayVideoMode const& mode);

    /** Terminate the application */
    void PostTerminateEvent();

    void TakeScreenshot(StringView filename);

    /** Add global console command */
    void AddCommand(GlobalStringView name, Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment = ""s);

    /** Remove global console command */
    void RemoveCommand(StringView name);

    void RunMainLoop();

    /** Read main window back buffer pixels. */
    void ReadBackbufferPixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, size_t sizeInBytes, void* sysMem);

    /** Current video mode */
    static DisplayVideoMode const& GetVideoMode()
    {
        return static_cast<GameApplication*>(Instance())->m_Window->GetVideoMode();
    }

    static String const& GetApplicationLocalData()
    {
        return static_cast<GameApplication*>(Instance())->m_ApplicationLocalData;
    }

    static Archive& GetEmbeddedArchive()
    {
        return static_cast<GameApplication*>(Instance())->m_EmbeddedArchive;
    }

    static RenderCore::IDevice* GetRenderDevice()
    {
        return static_cast<GameApplication*>(Instance())->m_RenderDevice;
    }

    static ResourceManager& GetResourceManager()
    {
        return *static_cast<GameApplication*>(Instance())->m_ResourceManager;
    }

    static MaterialManager& GetMaterialManager()
    {
        return *static_cast<GameApplication*>(Instance())->m_MaterialManager;
    }

    static FrameLoop& GetFrameLoop()
    {
        return *static_cast<GameApplication*>(Instance())->m_FrameLoop.GetObject();
    }

    static UIManager& GetUIManager()
    {
        return *static_cast<GameApplication*>(Instance())->m_UIManager.GetObject();
    }

    static MersenneTwisterRand& GetRandom()
    {
        return static_cast<GameApplication*>(Instance())->m_Random;
    }

    static StateMachine& GetStateMachine()
    {
        return static_cast<GameApplication*>(Instance())->m_StateMachine;
    }

    static CommandProcessor& GetCommandProcessor()
    {
        return static_cast<GameApplication*>(Instance())->m_CommandProcessor;
    }

    static InputSystem& GetInputSystem()
    {
        return static_cast<GameApplication*>(Instance())->m_InputSystem;
    }

    static VertexMemoryGPU* GetVertexMemoryGPU()
    {
        return static_cast<GameApplication*>(Instance())->m_VertexMemoryGPU;
    }

    static RenderBackend& GetRenderBackend()
    {
        return *static_cast<GameApplication*>(Instance())->m_RenderBackend.GetObject();
    }

    static AsyncJobList* GetRenderFrontendJobList()
    {
        return static_cast<GameApplication*>(Instance())->m_RenderFrontendJobList;
    }

protected:
    bool bToggleFullscreenAltEnter{true};

private:
    void DrawCanvas();
    void ShowStats();

    void LoadConfigFile(StringView configFile);
    void CreateMainWindowAndSwapChain();

    void TakeScreenshot();

    void Cmd_Quit(CommandProcessor const&);

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

private:
    Archive                          m_EmbeddedArchive;
    TRef<AsyncJobManager>            m_AsyncJobManager;
    AsyncJobList*                    m_RenderFrontendJobList{};
    TRef<RenderCore::IDevice>        m_RenderDevice;
    ResourceManager*                 m_ResourceManager{};
    MaterialManager*                 m_MaterialManager{};
    String                           m_Title;
    String                           m_ApplicationLocalData;
    TRef<FrameLoop>                  m_FrameLoop;
    TRef<RenderCore::IGenericWindow> m_Window;
    TRef<RenderCore::ISwapChain>     m_pSwapChain;
    TRef<VertexMemoryGPU>            m_VertexMemoryGPU;
    TUniqueRef<Canvas>               m_Canvas;
    TUniqueRef<UIManager>            m_UIManager;
    TRef<RenderFrontend>             m_Renderer;
    TRef<RenderBackend>              m_RenderBackend;
    InputSystem                      m_InputSystem;
    CommandProcessor                 m_CommandProcessor;
    CommandContext                   m_CommandContext;
    StateMachine                     m_StateMachine;
    TVector<World_ECS*>              m_Worlds;
    DisplayVideoMode                 m_DesiredMode;
    MersenneTwisterRand              m_Random;
    String                           m_Screenshot;
    float                            m_FrameDurationInSeconds{};
    bool                             m_bIsWindowVisible{};
    bool                             m_bPostChangeVideoMode{};
    bool                             m_bPostTerminateEvent{};
    bool                             m_bPostTakeScreenshot{};
};

namespace Global
{
extern FontHandle GDefaultFontHandle;
extern FontResource* GDefaultFont;
extern Float2 GRetinaScale;
} // namespace Global

HK_NAMESPACE_END
