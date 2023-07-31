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

#if 0

#include "Engine.h"
#include "RenderFrontend.h"
#include "Canvas/Canvas.h"

#include "World/PlayerController.h"
#include "World/World.h"

#include "PhysicsModule.h"
#include "AINavigationModule.h"
#include "AudioModule.h"

#include <Engine/Core/Application/Logger.h>
#include <Engine/Core/Platform/Platform.h>
#include <Engine/Core/Platform/Profiler.h>
#include <Engine/Core/Display.h>

#include <Engine/Audio/AudioMixer.h>

#ifdef HK_OS_WIN32
#include <Engine/Core/Platform/WindowsDefs.h>
#endif

#ifdef HK_OS_LINUX
#    include <unistd.h> // chdir
#endif

#include <SDL/SDL.h>

extern "C" const size_t   EmbeddedResources_Size;
extern "C" const uint64_t EmbeddedResources_Data[];

HK_NAMESPACE_BEGIN

namespace Global {

const char* GGameTitle = "Hork";

Archive                       GEmbeddedResourcesArch; // TODO: move to resource manager?
ResourceManager* GResourceManager{};
MaterialManager* GMaterialManager{};
FontHandle       GDefaultFontHandle;
FontResource*    GDefaultFont{};
StateMachine*                 GStateMachine{};
RenderCore::IDevice*          GRenderDevice{};
VertexMemoryGPU*              GVertexMemoryGPU{};
Float2                        GRetinaScale;
FrameLoop*                    GFrameLoop{};
InputSystem*                  GInputSystem{};
MersenneTwisterRand           GRand; // TODO: For each thread

extern const ClassMeta* GGameClass;

}

ConsoleVar com_ShowStat("com_ShowStat"s, "0"s);
ConsoleVar com_ShowFPS("com_ShowFPS"s, "0"s);

ConsoleVar rt_VidWidth("rt_VidWidth"s, "0"s);
ConsoleVar rt_VidHeight("rt_VidHeight"s, "0"s);
#ifdef HK_DEBUG
ConsoleVar rt_VidFullscreen("rt_VidFullscreen"s, "0"s);
#else
ConsoleVar rt_VidFullscreen("rt_VidFullscreen"s, "1"s);
#endif
ConsoleVar rt_SwapInterval("rt_SwapInterval"s, "0"s, 0, "1 - enable vsync, 0 - disable vsync, -1 - tearing"s);

Engine* GEngine;

namespace
{

GameModule* CreateGameModule(ClassMeta const* pClassMeta)
{
    if (!pClassMeta->IsSubclassOf<GameModule>())
    {
        CoreApplication::TerminateWithError("CreateGameModule: game module is not subclass of GameModule\n");
    }
    return static_cast<GameModule*>(pClassMeta->CreateInstance());
}

} // namespace

#ifdef HK_OS_WIN32
Engine::Engine()
#else
Engine::Engine(int argc, char** argv)
#endif
{
    HK_ASSERT(!GEngine);
    GEngine = this;

    Global::GEmbeddedResourcesArch = Archive::OpenFromMemory(EmbeddedResources_Data, EmbeddedResources_Size);
    if (!Global::GEmbeddedResourcesArch)
    {
        LOG("Failed to open embedded resources\n");
    }

    Core::InitializeProfiler();

    int jobManagerThreadCount = Thread::NumHardwareThreads ? Math::Min(Thread::NumHardwareThreads, AsyncJobManager::MAX_WORKER_THREADS) : AsyncJobManager::MAX_WORKER_THREADS;
    pAsyncJobManager = MakeRef<AsyncJobManager>(jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS);
    pRenderFrontendJobList = pAsyncJobManager->GetAsyncJobList(RENDER_FRONTEND_JOB_LIST);
    //pRenderBackendJobList  = pAsyncJobManager->GetAsyncJobList(RENDER_BACKEND_JOB_LIST);

    LoadConfigFile();

    CreateLogicalDevice("OpenGL 4.5", &m_RenderDevice);

    Global::GRenderDevice = m_RenderDevice.GetObject();

    if (rt_VidWidth.GetInteger() <= 0 || rt_VidHeight.GetInteger() <= 0)
    {
        DisplayMode mode;

        TVector<DisplayInfo> displays;
        Core::GetDisplays(displays);
        if (!displays.IsEmpty())
        {
            Core::GetDesktopDisplayMode(displays[0], mode);

            rt_VidWidth.ForceInteger(mode.Width);
            rt_VidHeight.ForceInteger(mode.Height);
        }
        else
        {
            rt_VidWidth.ForceInteger(1024);
            rt_VidHeight.ForceInteger(768);
        }
    }

    DisplayVideoMode desiredMode  = {};
    desiredMode.Width       = rt_VidWidth.GetInteger();
    desiredMode.Height      = rt_VidHeight.GetInteger();
    desiredMode.Opacity     = 1;
    desiredMode.bFullscreen = rt_VidFullscreen;
    desiredMode.bCentrized  = true;
    Core::Strcpy(desiredMode.Title, sizeof(desiredMode.Title), Global::GGameTitle);

    m_RenderDevice->GetOrCreateMainWindow(desiredMode, &m_Window);
    m_RenderDevice->CreateSwapChain(m_Window, &m_pSwapChain);

    // Swap buffers to prevent flickering
    m_pSwapChain->Present(rt_SwapInterval.GetInteger());

    Global::GRetinaScale = Float2(1.0f);

    m_VertexMemoryGPU = MakeRef<VertexMemoryGPU>(m_RenderDevice);
    Global::GVertexMemoryGPU = m_VertexMemoryGPU.GetObject();

    AINavigationModule::Initialize();
    PhysicsModule::Initialize();
    AudioModule::Initialize();

    Global::GResourceManager = new ResourceManager;
    Global::GMaterialManager = new MaterialManager;

    // Q: Move RobotoMono-Regular.ttf to embedded files?
    Global::GDefaultFontHandle = Global::GResourceManager->CreateResourceFromFile<FontResource>("/Root/fonts/RobotoMono/RobotoMono-Regular.ttf");
    Global::GDefaultFont = Global::GResourceManager->TryGet(Global::GDefaultFontHandle);
    HK_ASSERT(Global::GDefaultFont);
    Global::GDefaultFont->Upload();
    HK_ASSERT(Global::GDefaultFont->IsValid());

    Global::GStateMachine = &m_StateMachine;

    Global::GRand.Seed(Core::RandomSeed());

    m_Renderer = MakeRef<RenderFrontend>();

    m_RenderBackend = MakeRef<RenderBackend>(m_RenderDevice);

    Global::GInputSystem = &m_InputSystem;

    m_FrameLoop = MakeRef<FrameLoop>(m_RenderDevice);
    Global::GFrameLoop = m_FrameLoop.GetObject();

    // Process initial events
    m_FrameLoop->PollEvents(this);

    m_Canvas = MakeUnique<Canvas>();

    m_UIManager = MakeUnique<UIManager>(m_Window);

    m_GameModule = CreateGameModule(Global::GGameClass);
    m_GameModule->AddRef();

    LOG("Created game module: {}\n", m_GameModule->FinalClassName());

    m_bAllowInputEvents = true;
}

Engine::~Engine()
{
    m_bAllowInputEvents = false;

    m_GameModule->RemoveRef();
    m_GameModule = nullptr;

    m_UIManager.Reset();

    World::DestroyWorlds();
    World::KillWorlds();

    SoundEmitter::ClearOneShotSounds();

    GarbageCollector::DeallocateObjects();

    HK_ASSERT(m_WorldsECS.IsEmpty());   

    m_Canvas.Reset();

    Global::GResourceManager->UnloadResource(Global::GDefaultFontHandle);

    // Process resource unload
    Global::GResourceManager->MainThread_Update(1);

    m_RenderBackend.Reset();

    m_Renderer.Reset();

    delete Global::GMaterialManager;
    delete Global::GResourceManager;

    AudioModule::Deinitialize();
    AINavigationModule::Deinitialize();
    PhysicsModule::Deinitialize();

    Hk::ECS::Shutdown();

    m_FrameLoop.Reset();

    GarbageCollector::Shutdown();

    VisibilitySystem::PrimitivePool.Free();
    VisibilitySystem::PrimitiveLinkPool.Free();

    Core::ShutdownProfiler();

    Global::GEmbeddedResourcesArch.Close();
}

void Engine::RunMainLoop()
{
    do
    {
        _HK_PROFILER_FRAME("EngineFrame");

        // Garbage collect from previuous frames
        GarbageCollector::DeallocateObjects();

        // Set new frame, process game events
        m_FrameLoop->NewFrame({m_pSwapChain}, rt_SwapInterval.GetInteger());

        m_InputSystem.NewFrame();

        Global::GRand.Get();

        if (m_bPostChangeVideoMode)
        {
            m_bPostChangeVideoMode = false;

            m_Window->SetVideoMode(m_DesiredMode);

            // Swap buffers to prevent flickering
            m_pSwapChain->Present(rt_SwapInterval.GetInteger());
        }

        // Take current frame duration
        m_FrameDurationInSeconds = m_FrameLoop->SysFrameDuration() * 0.000001;

        // Don't allow very slow frames
        if (m_FrameDurationInSeconds > 0.5f)
        {
            m_FrameDurationInSeconds = 0.5f;
        }

        // Execute console commands
        m_CommandProcessor.Execute(m_GameModule->CmdContext);

        // Poll runtime events
        m_FrameLoop->PollEvents(this);

        // Update input
        m_InputSystem.Update(m_FrameDurationInSeconds);

        // Tick worlds
        World::UpdateWorlds(m_FrameDurationInSeconds);

        m_StateMachine.Tick(m_FrameDurationInSeconds);

        Global::GResourceManager->MainThread_Update(0.0001f);

        for (auto* world : m_WorldsECS)
            world->Tick(m_FrameDurationInSeconds);

        // Update audio system
        AudioModule::Get().Update(Actor_PlayerController::GetCurrentAudioListener(), m_FrameDurationInSeconds);

        m_UIManager->Update(m_FrameDurationInSeconds);

        // Draw widgets, HUD, etc
        DrawCanvas();

        // Build frame data for rendering
        m_Renderer->Render(m_FrameLoop, m_Canvas.GetObject());

        // Generate GPU commands
        m_RenderBackend->RenderFrame(m_FrameLoop->GetStreamedMemoryGPU(), m_pSwapChain->GetBackBuffer(), m_Renderer->GetFrameData());

        SaveMemoryStats();

    } while (!IsPendingTerminate());
}

int Engine::ExitCode() const
{
    return 0;
}

void Engine::LoadConfigFile()
{
    String configFile = GetRootPath() + "config.cfg";

    File f = File::OpenRead(configFile);
    if (f)
    {
        m_CommandProcessor.Add(f.AsString());

        class CommandContext : public ICommandContext
        {
        public:
            void ExecuteCommand(CommandProcessor const& _Proc) override
            {
                HK_ASSERT(_Proc.GetArgsCount() > 0);

                const char*  name = _Proc.GetArg(0);
                ConsoleVar* var;
                if (nullptr != (var = ConsoleVar::FindVariable(name)))
                {
                    if (_Proc.GetArgsCount() < 2)
                    {
                        var->Print();
                    }
                    else
                    {
                        var->SetString(_Proc.GetArg(1));
                    }
                }
            }
        };

        CommandContext context;

        m_CommandProcessor.Execute(context);
    }
}

World* Engine::CreateWorldECS()
{
    ECS::WorldCreateInfo worldInfo;
    worldInfo.NumThreads = 1;

    World* world = new World(worldInfo);
    m_WorldsECS.Add(world);
    return world;
}

void Engine::DestroyWorldECS(World* world)
{
    auto index = m_WorldsECS.IndexOf(world);
    if (index != Core::NPOS)
    {
        m_WorldsECS.Remove(index);
        delete world;
    }
}

void Engine::DrawCanvas()
{
    HK_PROFILER_EVENT("Draw Canvas");

    m_Canvas->NewFrame();

    if (IsWindowVisible())
    {
        m_UIManager->Draw(*m_Canvas);

        ShowStats();
    }
}

MemoryStat GMemoryStat[HEAP_MAX];
MemoryStat GMemoryStatGlobal;

const char* HeapName[HEAP_MAX] =
{
        "HEAP_STRING",
        "HEAP_VECTOR",
        "HEAP_HASH_SET",
        "HEAP_HASH_MAP",
        "HEAP_CPU_VERTEX_BUFFER",
        "HEAP_CPU_INDEX_BUFFER",
        "HEAP_IMAGE",
        "HEAP_AUDIO_DATA",
        "HEAP_RHI",
        "HEAP_PHYSICS",
        "HEAP_NAVIGATION",
        "HEAP_TEMP",
        "HEAP_MISC",
        "HEAP_WORLD_OBJECTS",
};

void Engine::SaveMemoryStats()
{
#define SHOW_HEAP_STAT(heap)                                              \
    {                                                                     \
        GMemoryStat[heap] = Core::GetHeapAllocator<heap>().GetStat(); \
    }

    SHOW_HEAP_STAT(HEAP_STRING);
    SHOW_HEAP_STAT(HEAP_VECTOR);
    SHOW_HEAP_STAT(HEAP_HASH_SET);
    SHOW_HEAP_STAT(HEAP_HASH_MAP);
    SHOW_HEAP_STAT(HEAP_CPU_VERTEX_BUFFER);
    SHOW_HEAP_STAT(HEAP_CPU_INDEX_BUFFER);
    SHOW_HEAP_STAT(HEAP_IMAGE);
    SHOW_HEAP_STAT(HEAP_AUDIO_DATA);
    SHOW_HEAP_STAT(HEAP_RHI);
    SHOW_HEAP_STAT(HEAP_PHYSICS);
    SHOW_HEAP_STAT(HEAP_NAVIGATION);
    SHOW_HEAP_STAT(HEAP_TEMP);
    SHOW_HEAP_STAT(HEAP_MISC);
    SHOW_HEAP_STAT(HEAP_WORLD_OBJECTS);

    GMemoryStatGlobal = {};

    for (int n = 0; n < HEAP_MAX; n++)
    {
        GMemoryStatGlobal.FrameAllocs += GMemoryStat[n].FrameAllocs;
        GMemoryStatGlobal.FrameFrees += GMemoryStat[n].FrameFrees;
        GMemoryStatGlobal.MemoryAllocated += GMemoryStat[n].MemoryAllocated;
        GMemoryStatGlobal.MemoryAllocs += GMemoryStat[n].MemoryAllocs;
        GMemoryStatGlobal.MemoryPeakAlloc += GMemoryStat[n].MemoryPeakAlloc;
    }
}

void Engine::ShowStats()
{
    #if 0
    Formatter fmt;

    m_Canvas->ResetScissor();

    if (com_ShowStat)
    {
        RenderFrameData* frameData = m_Renderer->GetFrameData();

        RenderFrontendStat const& stat = m_Renderer->GetStat();

        StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

        const float y_step = 14;
        const int   numLines = 13;

        Float2 pos(8, 8);

        FontStyle fontStyle;
        fontStyle.FontSize = 12;
        
        m_Canvas->FontFace(FontHandle{});

        pos.Y = 100;
        for (int n = 0; n < HEAP_MAX; n++)
        {
            MemoryStat& memstat = GMemoryStat[n];

            m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("{}\t\tHeap memory usage: {} KB / peak {} MB Allocs {}", HeapName[n], memstat.MemoryAllocated / 1024.0f, memstat.MemoryPeakAlloc / 1024.0f / 1024.0f, memstat.MemoryAllocs), true);
            pos.Y += y_step;
        }

        pos.Y = m_Canvas->GetHeight() - numLines * y_step;

        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("SDL Allocs (HEAP_MISC) {}", SDL_GetNumAllocations()), true);
        pos.Y += y_step;

        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Heap memory usage: {} KB / peak {} MB Allocs {}", GMemoryStatGlobal.MemoryAllocated / 1024.0f, GMemoryStatGlobal.MemoryPeakAlloc / 1024.0f / 1024.0f, GMemoryStatGlobal.MemoryAllocs), true);
        pos.Y += y_step;

        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Frame allocs {} Frame frees {}", GMemoryStatGlobal.FrameAllocs, GMemoryStatGlobal.FrameFrees), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Frame memory usage: {} KB / {} MB (Peak {} KB)", m_FrameLoop->GetFrameMemoryUsedPrev() / 1024.0f, m_FrameLoop->GetFrameMemorySize() >> 20, m_FrameLoop->GetMaxFrameMemoryUsage() / 1024.0f), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Frame memory usage (GPU): {} KB / {} MB (Peak {} KB)", streamedMemory->GetUsedMemoryPrev() / 1024.0f, streamedMemory->GetAllocatedMemory() >> 20, streamedMemory->GetMaxMemoryUsage() / 1024.0f), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Vertex cache memory usage (GPU): {} KB / {} MB", m_VertexMemoryGPU->GetUsedMemory() / 1024.0f, m_VertexMemoryGPU->GetAllocatedMemory() >> 20), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Visible instances: {}", frameData->Instances.Size() + frameData->TranslucentInstances.Size()), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Visible shadow instances: {}", frameData->ShadowInstances.Size()), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Visible dir lights: {}", frameData->DirectionalLights.Size()), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Polycount: {}", stat.PolyCount), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("ShadowMapPolyCount: {}", stat.ShadowMapPolyCount), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Frontend time: {} msec", stat.FrontendTime), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Audio channels: {} active, {} virtual", AudioModule::Get().GetMixer()->GetNumActiveChannels(), AudioModule::Get().GetMixer()->GetNumVirtualChannels()), true);
    }

    if (com_ShowFPS)
    {
        enum
        {
            FPS_BUF = 16
        };
        static float fpsavg[FPS_BUF];
        static int   n            = 0;
        fpsavg[n & (FPS_BUF - 1)] = m_FrameDurationInSeconds;
        n++;
        float fps = 0;
        for (int i = 0; i < FPS_BUF; i++)
            fps += fpsavg[i];
        fps *= (1.0f / FPS_BUF);
        fps = 1.0f / (fps > 0.0f ? fps : 1.0f);
        FontStyle fontStyle;
        fontStyle.FontSize = 14;
        m_Canvas->FontFace(FontHandle{});
        m_Canvas->DrawText(fontStyle, Float2(10, 30), Color4::White(), fmt("Frame time {:.1f} ms (FPS: {}, AVG {})", m_FrameDurationInSeconds * 1000.0f, int(1.0f / m_FrameDurationInSeconds), int(fps + 0.5f)), true);
    }
    #endif
}

void Engine::DeveloperKeys(KeyEvent const& event)
{
}

void Engine::OnKeyEvent(KeyEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    // Check Alt+Enter to toggle fullscreen/windowed mode
    if (m_GameModule->bToggleFullscreenAltEnter)
    {
        if (event.Action == IA_PRESS && event.Key == KEY_ENTER && (HAS_MODIFIER(event.ModMask, KEY_MOD_ALT)))
        {
            DisplayVideoMode videoMode  = m_Window->GetVideoMode();
            videoMode.bFullscreen = !videoMode.bFullscreen;
            PostChangeVideoMode(videoMode);
        }
    }

    DeveloperKeys(event);

    m_UIManager->GenerateKeyEvents(event, m_GameModule->CmdContext, m_CommandProcessor);
}

void Engine::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseButtonEvents(event);
}

void Engine::OnMouseWheelEvent(MouseWheelEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseWheelEvents(event);
}

void Engine::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseMoveEvents(event);
}

void Engine::OnJoystickButtonEvent(JoystickButtonEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateJoystickButtonEvents(event);
}

void Engine::OnJoystickAxisEvent(JoystickAxisEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateJoystickAxisEvents(event);
}

void Engine::OnCharEvent(CharEvent const& event)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateCharEvents(event);
}

void Engine::OnWindowVisible(bool bVisible)
{
    m_bIsWindowVisible = bVisible;
}

void Engine::OnCloseEvent()
{
    m_GameModule->OnGameClose();
}

void Engine::OnResize()
{
    DisplayVideoMode const& videoMode = m_Window->GetVideoMode();

    Global::GRetinaScale = Float2((float)videoMode.FramebufferWidth / videoMode.Width,
                                  (float)videoMode.FramebufferHeight / videoMode.Height);
}

void Engine::MapWindowCoordinate(float& InOutX, float& InOutY) const
{
    DisplayVideoMode const& videoMode = m_Window->GetVideoMode();
    InOutX += videoMode.X;
    InOutY += videoMode.Y;
}

void Engine::UnmapWindowCoordinate(float& InOutX, float& InOutY) const
{
    DisplayVideoMode const& videoMode = m_Window->GetVideoMode();
    InOutX -= videoMode.X;
    InOutY -= videoMode.Y;
}

void Engine::PostChangeVideoMode(DisplayVideoMode const& _DesiredMode)
{
    m_DesiredMode          = _DesiredMode;
    m_bPostChangeVideoMode = true;
}

void Engine::PostTerminateEvent()
{
    m_bPostTerminateEvent = true;
}

bool Engine::IsPendingTerminate()
{
    return m_bPostTerminateEvent;
}

void Engine::ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem)
{
    RenderCore::ITexture* pBackBuffer = m_pSwapChain->GetBackBuffer();

    RenderCore::TextureRect rect;
    rect.Offset.X    = _X;
    rect.Offset.Y    = _Y;
    rect.Dimension.X = _Width;
    rect.Dimension.Y = _Height;
    rect.Dimension.Z = 1;

    pBackBuffer->ReadRect(rect, _SizeInBytes, 4, _SysMem);
}

String const& Engine::GetWorkingDir()
{
    return m_WorkingDir;
}

String const& Engine::GetRootPath()
{
    return m_RootPath;
}

HK_NAMESPACE_END
#endif
