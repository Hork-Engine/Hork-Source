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

#include "GameApplication.h"

#include <Hork/Core/Logger.h>
#include <Hork/Core/Profiler.h>
#include <Hork/Core/Display.h>
#include <Hork/Core/AsyncJobManager.h>
#include <Hork/Core/Platform.h>
#include <Hork/Runtime/Audio/AudioMixer.h>
#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/Modules/Physics/PhysicsModule.h>
#include <Hork/ShaderUtils/ShaderCompiler.h>
#include <Hork/RHI/CreateDevice.h>

#if defined HK_OS_WIN32
#include <ShlObj.h>
#include <Hork/Core/WindowsDefs.h>
#endif

#include <SDL3/SDL.h>
#include <ozz/base/memory/allocator.h>
#include <Recast/RecastAlloc.h>
#include <Detour/DetourAlloc.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_ShowStat("com_ShowStat"_s, "0"_s);
ConsoleVar com_ShowFPS("com_ShowFPS"_s, "0"_s);
ConsoleVar com_AppDataPath("com_AppDataPath"_s, ""_s, CVAR_NOSAVE);

ConsoleVar rt_VidWidth("rt_VidWidth"_s, "0"_s);
ConsoleVar rt_VidHeight("rt_VidHeight"_s, "0"_s);
//ConsoleVar rt_VidWidth("rt_VidWidth"_s, "1024"_s);
//ConsoleVar rt_VidHeight("rt_VidHeight"_s, "768"_s);
ConsoleVar rt_VidHz("rt_VidHz"_s, "0"_s);
#ifdef HK_DEBUG
ConsoleVar rt_VidMode("rt_VidMode"_s, "exclusive"_s);
#else
ConsoleVar rt_VidMode("rt_VidMode"_s, "exclusive"_s, 0, "windowed/borderless/exclusive"_s);
#endif
ConsoleVar rt_SwapInterval("rt_SwapInterval"_s, "0"_s, 0, "1 - enable vsync, 0 - disable vsync, -1 - tearing"_s);

enum
{
    RENDER_FRONTEND_JOB_LIST,
    //RENDER_BACKEND_JOB_LIST,
    MAX_RUNTIME_JOB_LISTS
};

namespace
{

MemoryStat MemoryHeapStat[HEAP_MAX];
MemoryStat MemoryGlobalStat;

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

void SaveMemoryStats()
{
#define SHOW_HEAP_STAT(heap)                                          \
    {                                                                 \
        MemoryHeapStat[heap] = Core::GetHeapAllocator<heap>().GetStat(); \
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

    MemoryGlobalStat = {};

    for (int n = 0; n < HEAP_MAX; n++)
    {
        MemoryGlobalStat.FrameAllocs += MemoryHeapStat[n].FrameAllocs;
        MemoryGlobalStat.FrameFrees += MemoryHeapStat[n].FrameFrees;
        MemoryGlobalStat.MemoryAllocated += MemoryHeapStat[n].MemoryAllocated;
        MemoryGlobalStat.MemoryAllocs += MemoryHeapStat[n].MemoryAllocs;
        MemoryGlobalStat.MemoryPeakAlloc += MemoryHeapStat[n].MemoryPeakAlloc;
    }
}

#ifdef HK_OS_WIN32
int ConvertWCharToUtf8(char* buffer, size_t bufferlen, const wchar_t* input)
{
    return WideCharToMultiByte(65001 /* UTF8 */, 0, input, -1, buffer, (int)bufferlen, NULL, NULL);
}
#endif

String GetApplicationUserPath()
{
    #if defined HK_OS_WIN32
    TCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPath(NULL,
                                  CSIDL_LOCAL_APPDATA,
                                  NULL,
                                  0,
                                  szPath)))
    {
        char buffer[MAX_PATH * 4];
        ConvertWCharToUtf8(buffer, sizeof(buffer), szPath);
        Core::FixSeparatorInplace(buffer);
        return String(buffer);
    }
    return "C:/";
    #elif defined HK_OS_LINUX
    const char* p = getenv("HOME");
    return String(p);
    #else
    #error "GetApplicationUserPath not implemented"
    #endif
}

void InitializeThirdPartyLibraries()
{
    {
        class OzzAllocator : public ozz::memory::Allocator
        {
        public:
            void* Allocate(size_t _size, size_t _alignment) override
            {
                return Core::GetHeapAllocator<HEAP_MISC>().Alloc(_size, _alignment);
            }

            void Deallocate(void* _block) override
            {
                return Core::GetHeapAllocator<HEAP_MISC>().Free(_block);
            }
        };

        static OzzAllocator s_OzzAllocator;
        ozz::memory::SetDefaulAllocator(&s_OzzAllocator);
    }

    {
        auto detourAlloc = [](size_t sizeInBytes, dtAllocHint hint)
        {
            return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(sizeInBytes);
        };

        auto recastAlloc = [](size_t sizeInBytes, rcAllocHint hint)
        {
            if (sizeInBytes == 0)
                sizeInBytes = 1;
            return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(sizeInBytes);
        };

        auto dealloc = [](void* bytes)
        {
            Core::GetHeapAllocator<HEAP_NAVIGATION>().Free(bytes);
        };

        dtAllocSetCustom(detourAlloc, dealloc);
        rcAllocSetCustom(recastAlloc, dealloc);
    }
}

GlobalStringView GetWindowModeString(WindowMode mode)
{
    switch (mode)
    {
    case WindowMode::Windowed:
        return "windowed"_s;
    case WindowMode::BorderlessFullscreen:
        return "borderless"_s;
    case WindowMode::ExclusiveFullscreen:
        return "exclusive"_s;
    }
    return "windowed"_s;
}

WindowMode GetWindowModeFromString(StringView str)
{
    if (!str.Icmp("windowed"))
        return WindowMode::Windowed;
    if (!str.Icmp("borderless"))
        return WindowMode::BorderlessFullscreen;
    if (!str.Icmp("exclusive"))
        return WindowMode::ExclusiveFullscreen;

    return WindowMode::Windowed;
}

} // namespace

GameApplication::GameApplication(ArgumentPack const& args, StringView title) :
    GameApplication(args,
                    ApplicationDesc{}
                        .SetTitle(title)
                        .SetCompany("Hork Games"))
{
}

GameApplication::GameApplication(ArgumentPack const& args, ApplicationDesc const& appDesc) :
    CoreApplication(args),
    m_Title(appDesc.Title),
    m_Random(Core::RandomSeed())
{
    LoadConfigFile(sGetRootPath() / "default.cfg");

    if (com_AppDataPath.GetString().IsEmpty())
        com_AppDataPath = GetApplicationUserPath() / appDesc.Company / m_Title;

    m_ApplicationLocalData = com_AppDataPath.GetString();

    LOG("AppData: {}\n", m_ApplicationLocalData);

    LoadConfigFile(m_ApplicationLocalData / "config.cfg");

    int jobManagerThreadCount = Thread::NumHardwareThreads ? Math::Min(Thread::NumHardwareThreads, AsyncJobManager::MAX_WORKER_THREADS) : AsyncJobManager::MAX_WORKER_THREADS;
    m_AsyncJobManager = MakeUnique<AsyncJobManager>(jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS);
    m_RenderFrontendJobList = m_AsyncJobManager->GetAsyncJobList(RENDER_FRONTEND_JOB_LIST);
    //pRenderBackendJobList  = m_AsyncJobManager->GetAsyncJobList(RENDER_BACKEND_JOB_LIST);

    ShaderCompiler::sInitialize();

    CreateLogicalDevice("OpenGL 4.5", &m_RenderDevice);

    CreateMainWindowAndSwapChain();

    // FIXME: Move to RenderModule?
    m_RetinaScale = Float2(1.0f);
    m_VertexMemoryGPU = MakeUnique<VertexMemoryGPU>(m_RenderDevice);

    InitializeThirdPartyLibraries();

    PhysicsModule::sInitialize();

    m_AudioDevice = MakeRef<AudioDevice>();
    m_AudioMixer = MakeUnique<AudioMixer>(m_AudioDevice);
    m_AudioMixer->StartAsync();

    m_RenderBackend = MakeUnique<RenderBackend>(m_RenderDevice);

    m_Renderer = MakeUnique<RenderFrontend>();

    m_ResourceManager = MakeUnique<ResourceManager>();
    m_MaterialManager = MakeUnique<MaterialManager>();

    // Q: Move RobotoMono-Regular.ttf to embedded files?
    m_DefaultFontHandle = m_ResourceManager->CreateResourceFromFile<FontResource>("/Root/fonts/RobotoMono/RobotoMono-Regular.ttf");
    m_DefaultFont = m_ResourceManager->TryGet(m_DefaultFontHandle);
    HK_ASSERT(m_DefaultFont);
    m_DefaultFont->Upload();
    HK_ASSERT(m_DefaultFont->IsValid());

    m_FrameLoop = MakeUnique<FrameLoop>(m_RenderDevice);

    // Process initial events
    m_FrameLoop->SetGenerateInputEvents(false);
    m_FrameLoop->PollEvents(this);

    m_Canvas = MakeUnique<Canvas>();

    m_UIManager = MakeUnique<UIManager>(m_Window);

    m_FrameLoop->SetGenerateInputEvents(true);

    AddCommand("quit"_s, {this, &GameApplication::Cmd_Quit}, "Quit the game"_s);
}

GameApplication::~GameApplication()
{
    m_UIManager.Reset();

    GarbageCollector::sDeallocateObjects();

    HK_ASSERT(m_Worlds.IsEmpty());

    m_Canvas.Reset();

    m_FrameLoop.Reset();

    m_ResourceManager->UnloadResource(m_DefaultFontHandle);

    // Process resource unload
    m_ResourceManager->MainThread_Update(1);

    m_Renderer.Reset();

    m_MaterialManager.Reset();
    m_ResourceManager.Reset();

    m_RenderBackend.Reset();

    m_AudioMixer.Reset();
    m_AudioDevice.Reset();
    
    PhysicsModule::sDeinitialize();

    ShaderCompiler::sDeinitialize();

    //Hk::ECS::Shutdown();

    GarbageCollector::sShutdown();

    Core::ShutdownProfiler();
}

void GameApplication::RunMainLoop()
{
    RHI::ISwapChain* swapChains[1] = {m_SwapChain};

    do
    {
        _HK_PROFILER_FRAME("EngineFrame");

        // Garbage collect from previuous frames
        GarbageCollector::sDeallocateObjects();

        // Set new frame, process game events
        m_FrameLoop->NewFrame(swapChains, rt_SwapInterval.GetInteger(), m_ResourceManager.RawPtr());

        m_InputSystem.NewFrame();

        m_Random.Get();

        if (m_bPostTakeScreenshot)
        {
            m_bPostTakeScreenshot = false;

            TakeScreenshot();
        }

        if (m_bPostChangeWindowSettings)
        {
            m_bPostChangeWindowSettings = false;

            m_Window->ChangeWindowSettings(m_WindowSettings);

            // Swap buffers to prevent flickering
            m_SwapChain->Present(rt_SwapInterval.GetInteger());
        }

        // Take current frame duration
        m_FrameDurationInSeconds = m_FrameLoop->SysFrameDuration() * 0.000001;

        const float minFps = 10;
        if (m_FrameDurationInSeconds > 1.0f / minFps)
            m_FrameDurationInSeconds = 1.0f / minFps;
        else if (m_FrameDurationInSeconds < 0.001f)
            m_FrameDurationInSeconds = 0.001f;

        // Execute console commands
        m_CommandProcessor.Execute(m_CommandContext);

        // Poll runtime events
        m_FrameLoop->PollEvents(this);

        // Update input
        m_InputSystem.Tick(m_FrameDurationInSeconds);

        // Tick state
        m_StateMachine.Update(m_FrameDurationInSeconds);

        // Tick worlds
        for (auto* world : m_Worlds)
            world->Tick(m_FrameDurationInSeconds);

        // Update audio
        if (!m_AudioMixer->IsAsync())
            m_AudioMixer->Update();

        m_UIManager->Tick(m_FrameDurationInSeconds);

        // Draw widgets, HUD, etc
        DrawCanvas();

        // Build frame data for rendering
        m_Renderer->Render(m_FrameLoop.RawPtr(), m_Canvas.RawPtr());

        // Generate GPU commands
        m_RenderBackend->RenderFrame(m_FrameLoop->GetStreamedMemoryGPU(), m_SwapChain->GetBackBuffer(), m_Renderer->GetFrameData());

        SaveMemoryStats();

    } while (!m_bPostTerminateEvent);
}

void GameApplication::DrawCanvas()
{
    HK_PROFILER_EVENT("Draw Canvas");

    m_Canvas->NewFrame();

    if (m_bIsWindowVisible)
    {
        m_UIManager->Draw(*m_Canvas);

        ShowStats();
    }
}

void GameApplication::ShowStats()
{
    TSprintfBuffer<1024> sb;

    m_Canvas->ResetScissor();

    if (com_ShowStat)
    {
        RenderFrameData* frameData = m_Renderer->GetFrameData();

        RenderFrontendStat const& stat = m_Renderer->GetStat();

        StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

        const float y_step = 14;
        const int numLines = 13;

        Float2 pos(8, 8);

        FontStyle fontStyle;
        fontStyle.FontSize = 12;

        m_Canvas->FontFace(FontHandle{});

        pos.Y = 100;
        for (int n = 0; n < HEAP_MAX; n++)
        {
            MemoryStat& memstat = MemoryHeapStat[n];

            m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("%s\t\tHeap memory usage: %f KB / peak %f MB Allocs %d", HeapName[n], memstat.MemoryAllocated / 1024.0f, memstat.MemoryPeakAlloc / 1024.0f / 1024.0f, memstat.MemoryAllocs), true);
            pos.Y += y_step;
        }

        int h = m_SwapChain->GetHeight();

        pos.Y = h - numLines * y_step;

        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("SDL Allocs (HEAP_MISC) %d", SDL_GetNumAllocations()), true);
        pos.Y += y_step;

        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Heap memory usage: %f KB / peak %f MB Allocs %d", MemoryGlobalStat.MemoryAllocated / 1024.0f, MemoryGlobalStat.MemoryPeakAlloc / 1024.0f / 1024.0f, MemoryGlobalStat.MemoryAllocs), true);
        pos.Y += y_step;

        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Frame allocs %d Frame frees %d", MemoryGlobalStat.FrameAllocs, MemoryGlobalStat.FrameFrees), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Frame memory usage: %f KB / %d MB (Peak %f KB)", m_FrameLoop->GetFrameMemoryUsedPrev() / 1024.0f, m_FrameLoop->GetFrameMemorySize() >> 20, m_FrameLoop->GetMaxFrameMemoryUsage() / 1024.0f), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Frame memory usage (GPU): %f KB / %d MB (Peak %f KB)", streamedMemory->GetUsedMemoryPrev() / 1024.0f, streamedMemory->GetAllocatedMemory() >> 20, streamedMemory->GetMaxMemoryUsage() / 1024.0f), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Vertex cache memory usage (GPU): %f KB / %d MB", m_VertexMemoryGPU->GetUsedMemory() / 1024.0f, m_VertexMemoryGPU->GetAllocatedMemory() >> 20), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Visible instances: %d", frameData->Instances.Size() + frameData->TranslucentInstances.Size()), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Visible shadow instances: %d", frameData->ShadowInstances.Size()), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Visible dir lights: %d", frameData->DirectionalLights.Size()), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Polycount: %d", stat.PolyCount), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("ShadowMapPolyCount: %d", stat.ShadowMapPolyCount), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Frontend time: %d msec", stat.FrontendTime), true);
        pos.Y += y_step;
        m_Canvas->DrawText(fontStyle, pos, Color4::sWhite(), sb.Sprintf("Audio channels: %d active, %d virtual", m_AudioMixer->GetNumActiveTracks(), m_AudioMixer->GetNumVirtualTracks()), true);
    }

    if (com_ShowFPS)
    {
        enum
        {
            FPS_BUF = 16
        };
        static float fpsavg[FPS_BUF];
        static int n = 0;
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
        m_Canvas->DrawText(fontStyle, Float2(10, 30), Color4::sWhite(), sb.Sprintf("Frame time %.1f ms (FPS: %d, AVG %d)", m_FrameDurationInSeconds * 1000.0f, int(1.0f / m_FrameDurationInSeconds), int(fps + 0.5f)), true);
    }
}

void GameApplication::LoadConfigFile(StringView configFile)
{
    File f = File::sOpenRead(configFile);
    if (f)
    {
        m_CommandProcessor.Add(f.AsString());

        class CommandContext : public ICommandContext
        {
        public:
            void ExecuteCommand(CommandProcessor const& proc) override
            {
                HK_ASSERT(proc.GetArgsCount() > 0);

                const char*  name = proc.GetArg(0);
                ConsoleVar* var;
                if (nullptr != (var = ConsoleVar::sFindVariable(name)))
                {
                    if (proc.GetArgsCount() < 2)
                    {
                        var->Print();
                    }
                    else
                    {
                        var->SetString(proc.GetArg(1));
                    }
                }
            }
        };

        CommandContext context;

        m_CommandProcessor.Execute(context);
    }
}

void GameApplication::CreateMainWindowAndSwapChain()
{
    if (rt_VidWidth.GetInteger() <= 0 || rt_VidHeight.GetInteger() <= 0)
    {
        DisplayMode mode;

        Vector<DisplayInfo> displays;
        Core::GetDisplays(displays);
        if (!displays.IsEmpty())
        {
            if (GetWindowModeFromString(rt_VidMode.GetString()) == WindowMode::Windowed)
            {
                rt_VidWidth.ForceInteger(displays[0].DisplayUsableW);
                rt_VidHeight.ForceInteger(displays[0].DisplayUsableH);
            }
            else
            {
                Core::GetDesktopDisplayMode(displays[0], mode);
                rt_VidWidth.ForceInteger(mode.Width);
                rt_VidHeight.ForceInteger(mode.Height);
            }
        }
        else
        {
            rt_VidWidth.ForceInteger(1920);
            rt_VidHeight.ForceInteger(1080);
        }
    }

    WindowSettings windowSettings  = {};
    windowSettings.Width       = rt_VidWidth.GetInteger();
    windowSettings.Height      = rt_VidHeight.GetInteger();
    windowSettings.Mode        = GetWindowModeFromString(rt_VidMode.GetString());
    windowSettings.RefreshRate = rt_VidHz.GetFloat();
    windowSettings.bCentrized  = true;

    m_RenderDevice->GetOrCreateMainWindow(windowSettings, &m_Window);
    m_RenderDevice->CreateSwapChain(m_Window, &m_SwapChain);

    m_Window->SetTitle(m_Title);

    // Swap buffers to prevent flickering
    m_SwapChain->Present(rt_SwapInterval.GetInteger());
}

void GameApplication::OnKeyEvent(KeyEvent const& event)
{
    // Check Alt+Enter to toggle fullscreen/windowed mode
    if (bToggleFullscreenAltEnter)
    {
        if (event.Action == InputAction::Pressed && event.Key == VirtualKey::Enter && event.ModMask.Alt)
        {
            WindowSettings windowSettings = {};

            windowSettings.Width = rt_VidWidth.GetInteger();
            windowSettings.Height = rt_VidHeight.GetInteger();
            windowSettings.Mode = m_Window->IsFullscreenMode() ? WindowMode::Windowed : WindowMode::ExclusiveFullscreen;
            windowSettings.RefreshRate = rt_VidHz.GetFloat();
            windowSettings.bCentrized = true;

            ChangeMainWindowSettings(windowSettings);
        }
    }

    m_UIManager->GenerateKeyEvents(event, m_CommandContext, m_CommandProcessor);
}

void GameApplication::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    m_UIManager->GenerateMouseButtonEvents(event);
}

void GameApplication::OnMouseWheelEvent(MouseWheelEvent const& event)
{
    m_UIManager->GenerateMouseWheelEvents(event);
}

void GameApplication::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    m_UIManager->GenerateMouseMoveEvents(event);
}

void GameApplication::OnGamepadButtonEvent(struct GamepadKeyEvent const& event)
{
    m_UIManager->GeneratenGamepadButtonEvents(event);
}

void GameApplication::OnGamepadAxisMotionEvent(struct GamepadAxisMotionEvent const& event)
{
    m_UIManager->GenerateGamepadAxisMotionEvents(event);
}

void GameApplication::OnCharEvent(CharEvent const& event)
{
    m_UIManager->GenerateCharEvents(event);
}

void GameApplication::OnWindowVisible(bool bVisible)
{
    m_bIsWindowVisible = bVisible;
}

void GameApplication::OnCloseEvent()
{
    PostTerminateEvent();
}

void GameApplication::OnResize()
{
    m_RetinaScale = Float2(static_cast<float>(m_Window->GetFramebufferWidth()) / m_Window->GetWidth(),
                           static_cast<float>(m_Window->GetFramebufferHeight()) / m_Window->GetHeight());
}

void GameApplication::ChangeMainWindowSettings(WindowSettings const& windowSettings)
{
    m_WindowSettings = windowSettings;
    m_bPostChangeWindowSettings = true;

    rt_VidWidth.ForceInteger(windowSettings.Width);
    rt_VidHeight.ForceInteger(windowSettings.Height);
    rt_VidHz.ForceFloat(windowSettings.RefreshRate);
    rt_VidMode.ForceString(GetWindowModeString(windowSettings.Mode));
}

void GameApplication::PostTerminateEvent()
{
    m_bPostTerminateEvent = true;
}

void GameApplication::TakeScreenshot(StringView filename)
{
    m_Screenshot = filename;
    m_bPostTakeScreenshot = true;
}

void GameApplication::ReadBackbufferPixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, size_t sizeInBytes, void* sysMem)
{
    RHI::ITexture* pBackBuffer = m_SwapChain->GetBackBuffer();

    RHI::TextureRect rect;
    rect.Offset.X    = x;
    rect.Offset.Y    = y;
    rect.Dimension.X = width;
    rect.Dimension.Y = height;
    rect.Dimension.Z = 1;

    pBackBuffer->ReadRect(rect, sizeInBytes, 4, sysMem);
}

void GameApplication::TakeScreenshot()
{
    RHI::ITexture* pBackBuffer = m_SwapChain->GetBackBuffer();

    uint32_t w = pBackBuffer->GetWidth();
    uint32_t h = pBackBuffer->GetHeight();

    RHI::TextureRect rect;
    rect.Dimension.X = w;
    rect.Dimension.Y = h;
    rect.Dimension.Z = 1;

    size_t sizeInBytes = (size_t)w * h * 4;

    // TODO: Use temp memory?
    HeapBlob dataBlob(sizeInBytes);

    pBackBuffer->ReadRect(rect, dataBlob.Size(), 4, dataBlob.GetData());

    FlipImageY(dataBlob.GetData(), w, h, 4, (size_t)w * 4);

    // TODO: Add to async tasks?

    ImageWriteConfig cfg;
    cfg.Width = w;
    cfg.Height = h;
    cfg.NumChannels = 4;
    cfg.pData = dataBlob.GetData();
    cfg.Quality = 1.0f;
    cfg.bLossless = false;

    WriteImage(m_Screenshot, cfg);
}

World* GameApplication::CreateWorld()
{
    auto* world = new World;
    m_Worlds.Add(world);
    return world;
}

void GameApplication::DestroyWorld(World* world)
{
    auto index = m_Worlds.IndexOf(world);
    if (index != Core::NPOS)
    {
        m_Worlds.Remove(index);
        delete world;
    }
}

void GameApplication::AddCommand(GlobalStringView name, Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment)
{
    m_CommandContext.AddCommand(name, callback, comment);
}

void GameApplication::RemoveCommand(StringView name)
{
    m_CommandContext.RemoveCommand(name);
}

void GameApplication::Cmd_Quit(CommandProcessor const&)
{
    PostTerminateEvent();
}

HK_NAMESPACE_END
