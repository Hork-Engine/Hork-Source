/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Core/CoreApplication.h>
#include <Hork/Core/CommandProcessor.h>
#include <Hork/Core/Random.h>

#include <Hork/Runtime/World/Modules/Render/RenderFrontend.h>

#include <Hork/Runtime/Materials/MaterialManager.h>
#include <Hork/Runtime/UI/UIManager.h>
#include <Hork/Runtime/Renderer/RenderBackend.h>

#include "FrameLoop.h"
#include "InputSystem.h"
#include "StateMachine.h"

HK_NAMESPACE_BEGIN

class World;
class AsyncJobManager;
class AsyncJobList;
class AudioDevice;
class AudioMixer;

class ApplicationDesc
{
public:
    StringView Title;
    StringView Company;

    ApplicationDesc& SetTitle(StringView title)
    {
        Title = title;
        return *this;
    }

    ApplicationDesc& SetCompany(StringView company)
    {
        Company = company;
        return *this;
    }
};

class GameApplication : public CoreApplication, public IEventListener
{
public:
    GameApplication(ArgumentPack const& args, StringView title);
    GameApplication(ArgumentPack const& args, ApplicationDesc const& appDesc);
    ~GameApplication();

    World* CreateWorld();

    void DestroyWorld(World* world);

    /// Set main window settings.
    void ChangeMainWindowSettings(WindowSettings const& windowSettings);

    /// Terminate the application
    void PostTerminateEvent();

    void TakeScreenshot(StringView filename);

    /// Add global console command
    void AddCommand(GlobalStringView name, Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment = ""_s);

    /// Remove global console command
    void RemoveCommand(StringView name);

    void RunMainLoop();

    /// Read main window back buffer pixels.
    void ReadBackbufferPixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, size_t sizeInBytes, void* sysMem);

    static GameApplication* sInstance()
    {
        return static_cast<GameApplication*>(CoreApplication::sInstance());
    }

    static String const& sGetApplicationLocalData()
    {
        return static_cast<GameApplication*>(sInstance())->m_ApplicationLocalData;
    }

    static RHI::IDevice* sGetRenderDevice()
    {
        return static_cast<GameApplication*>(sInstance())->m_RenderDevice;
    }

    static ResourceManager& sGetResourceManager()
    {
        return *static_cast<GameApplication*>(sInstance())->m_ResourceManager.RawPtr();
    }

    static MaterialManager& sGetMaterialManager()
    {
        return *static_cast<GameApplication*>(sInstance())->m_MaterialManager.RawPtr();
    }

    static FrameLoop& sGetFrameLoop()
    {
        return *static_cast<GameApplication*>(sInstance())->m_FrameLoop.RawPtr();
    }

    static UIManager& sGetUIManager()
    {
        return *static_cast<GameApplication*>(sInstance())->m_UIManager.RawPtr();
    }

    static MersenneTwisterRand& sGetRandom()
    {
        return static_cast<GameApplication*>(sInstance())->m_Random;
    }

    static StateMachine& sGetStateMachine()
    {
        return static_cast<GameApplication*>(sInstance())->m_StateMachine;
    }

    static CommandProcessor& sGetCommandProcessor()
    {
        return static_cast<GameApplication*>(sInstance())->m_CommandProcessor;
    }

    static InputSystem& sGetInputSystem()
    {
        return static_cast<GameApplication*>(sInstance())->m_InputSystem;
    }

    static VertexMemoryGPU* sGetVertexMemoryGPU()
    {
        return static_cast<GameApplication*>(sInstance())->m_VertexMemoryGPU.RawPtr();
    }

    static RenderBackend& sGetRenderBackend()
    {
        return *static_cast<GameApplication*>(sInstance())->m_RenderBackend.RawPtr();
    }

    static AsyncJobList* sGetRenderFrontendJobList()
    {
        return static_cast<GameApplication*>(sInstance())->m_RenderFrontendJobList;
    }

    static AudioDevice* sGetAudioDevice()
    {
        return static_cast<GameApplication*>(sInstance())->m_AudioDevice;
    }

    static AudioMixer* sGetAudioMixer()
    {
        return static_cast<GameApplication*>(sInstance())->m_AudioMixer.RawPtr();
    }

    static FontHandle sGetDefaultFontHandle()
    {
        return static_cast<GameApplication*>(sInstance())->m_DefaultFontHandle;
    }

    static FontResource* sGetDefaultFont()
    {
        return static_cast<GameApplication*>(sInstance())->m_DefaultFont;
    }

    static Float2 const& sGetRetinaScale()
    {
        return static_cast<GameApplication*>(sInstance())->m_RetinaScale;
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

    /// IEventListener interface.
    void OnKeyEvent(KeyEvent const& event) override final;

    /// IEventListener interface.
    void OnMouseButtonEvent(MouseButtonEvent const& event) override final;

    /// IEventListener interface.
    void OnMouseWheelEvent(MouseWheelEvent const& event) override final;

    /// IEventListener interface.
    void OnMouseMoveEvent(MouseMoveEvent const& event) override final;

    /// IEventListener interface.
    void OnGamepadButtonEvent(struct GamepadKeyEvent const& event) override final;

    /// IEventListener interface.
    void OnGamepadAxisMotionEvent(struct GamepadAxisMotionEvent const& event)  override final;

    /// IEventListener interface.
    void OnCharEvent(CharEvent const& event) override final;

    /// IEventListener interface.
    void OnWindowVisible(bool bVisible) override final;

    /// IEventListener interface.
    void OnCloseEvent() override final;

    /// IEventListener interface.
    void OnResize() override final;

private:
    UniqueRef<AsyncJobManager>      m_AsyncJobManager;
    AsyncJobList*                   m_RenderFrontendJobList{};
    UniqueRef<ResourceManager>      m_ResourceManager;
    UniqueRef<MaterialManager>      m_MaterialManager;
    String                          m_Title;
    String                          m_ApplicationLocalData;
    UniqueRef<FrameLoop>            m_FrameLoop;
    Ref<RHI::IDevice>        m_RenderDevice;
    Ref<RHI::IGenericWindow> m_Window;
    Ref<RHI::ISwapChain>     m_SwapChain;
    UniqueRef<VertexMemoryGPU>      m_VertexMemoryGPU;
    UniqueRef<Canvas>               m_Canvas;
    UniqueRef<UIManager>            m_UIManager;
    UniqueRef<RenderFrontend>       m_Renderer;
    UniqueRef<RenderBackend>        m_RenderBackend;
    Ref<AudioDevice>                m_AudioDevice;
    UniqueRef<AudioMixer>           m_AudioMixer;
    InputSystem                     m_InputSystem;
    CommandProcessor                m_CommandProcessor;
    CommandContext                  m_CommandContext;
    StateMachine                    m_StateMachine;
    Vector<World*>                  m_Worlds;
    WindowSettings                  m_WindowSettings;
    MersenneTwisterRand             m_Random;
    String                          m_Screenshot;
    float                           m_FrameDurationInSeconds{};
    bool                            m_bIsWindowVisible{};
    bool                            m_bPostChangeWindowSettings{};
    bool                            m_bPostTerminateEvent{};
    bool                            m_bPostTakeScreenshot{};
    FontHandle                      m_DefaultFontHandle;
    FontResource*                   m_DefaultFont;
    Float2                          m_RetinaScale;
};

HK_NAMESPACE_END
