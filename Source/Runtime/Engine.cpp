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

#include "Engine.h"
#include "Display.h"
#include "EntryDecl.h"
#include "ResourceManager.h"
#include "RenderFrontend.h"
#include "AudioSystem.h"
#include "Canvas/Canvas.h"

#include "World/PlayerController.h"
#include "World/InputComponent.h"
#include "World/World.h"

#include <Platform/Logger.h>
#include <Platform/Platform.h>
#include <Platform/Profiler.h>

#include <Audio/AudioMixer.h>

#include <Bullet3Common/b3Logging.h>
#include <Bullet3Common/b3AlignedAllocator.h>
#include <LinearMath/btAlignedAllocator.h>

#include <DetourAlloc.h>

#ifdef HK_OS_LINUX
#    include <unistd.h> // chdir
#endif

#include <SDL.h>

HK_NAMESPACE_BEGIN

static ConsoleVar com_ShowStat("com_ShowStat"s, "0"s);
static ConsoleVar com_ShowFPS("com_ShowFPS"s, "0"s);

ConsoleVar rt_VidWidth("rt_VidWidth"s, "0"s);
ConsoleVar rt_VidHeight("rt_VidHeight"s, "0"s);
#ifdef HK_DEBUG
ConsoleVar rt_VidFullscreen("rt_VidFullscreen"s, "0"s);
#else
ConsoleVar rt_VidFullscreen("rt_VidFullscreen"s, "1"s);
#endif

ConsoleVar rt_SwapInterval("rt_SwapInterval"s, "0"s, 0, "1 - enable vsync, 0 - disable vsync, -1 - tearing"s);

static int TotalAllocatedRenderCore = 0;

Engine* GEngine;

Engine::Engine() :
    Rand(Core::RandomSeed())
{
    GEngine     = this;
    m_RetinaScale = Float2(1.0f);
}

static void PhysModulePrintFunction(const char* _Message)
{
    LOG("PhysModule: {}", _Message);
}

static void PhysModuleWarningFunction(const char* _Message)
{
    WARNING("PhysModule: {}", _Message);
}

static void PhysModuleErrorFunction(const char* _Message)
{
    ERROR("PhysModule: {}", _Message);
}

static void* PhysModuleAlignedAlloc(size_t _BytesCount, int _Alignment)
{
    return Platform::GetHeapAllocator<HEAP_PHYSICS>().Alloc(_BytesCount, _Alignment);
}

static void* PhysModuleAlloc(size_t _BytesCount)
{
    return Platform::GetHeapAllocator<HEAP_PHYSICS>().Alloc(_BytesCount);
}

static void PhysModuleFree(void* _Bytes)
{
    Platform::GetHeapAllocator<HEAP_PHYSICS>().Free(_Bytes);
}

static void* NavModuleAlloc(size_t _BytesCount, dtAllocHint _Hint)
{
    return Platform::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_BytesCount);
}

static void NavModuleFree(void* _Bytes)
{
    Platform::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Bytes);
}

static GameModule* CreateGameModule(ClassMeta const* pClassMeta)
{
    if (!pClassMeta->IsSubclassOf<GameModule>())
    {
        CriticalError("CreateGameModule: game module is not subclass of GameModule\n");
    }
    return static_cast<GameModule*>(pClassMeta->CreateInstance());
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

void Engine::InitializeDirectories()
{
    ProcessInfo const& processInfo = Platform::GetProcessInfo();

    m_WorkingDir = PathUtils::GetFilePath(processInfo.Executable);

#if defined HK_OS_WIN32
    SetCurrentDirectoryA(m_WorkingDir.CStr());
#elif defined HK_OS_LINUX
    int r = chdir(m_WorkingDir.CStr());
    if (r != 0)
    {
        LOG("Cannot set working directory\n");
    }
#else
#    error "InitializeDirectories not implemented under current platform"
#endif

    m_RootPath = m_pModuleDecl->RootPath;
    if (m_RootPath.IsEmpty())
    {
        m_RootPath = "Data/";
    }
    else
    {
        PathUtils::FixSeparatorInplace(m_RootPath);
        if (m_RootPath[m_RootPath.Length() - 1] != '/')
        {
            m_RootPath += '/';
        }
    }

    LOG("Working directory: {}\n", GetWorkingDir());
    LOG("Root path: {}\n", GetRootPath());
    LOG("Executable: {}\n", GetExecutableName());
}

void Engine::Run(EntryDecl const& entryDecl)
{
    m_pModuleDecl = &entryDecl;

    InitializeDirectories();

    Platform::InitializeProfiler();

    if (Thread::NumHardwareThreads)
    {
        LOG("Num hardware threads: {}\n", Thread::NumHardwareThreads);
    }

    int jobManagerThreadCount = Thread::NumHardwareThreads ? Math::Min(Thread::NumHardwareThreads, AsyncJobManager::MAX_WORKER_THREADS) : AsyncJobManager::MAX_WORKER_THREADS;

    pAsyncJobManager = MakeRef<AsyncJobManager>(jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS);

    pRenderFrontendJobList = pAsyncJobManager->GetAsyncJobList(RENDER_FRONTEND_JOB_LIST);
    pRenderBackendJobList  = pAsyncJobManager->GetAsyncJobList(RENDER_BACKEND_JOB_LIST);

    LoadConfigFile();

    RenderCore::AllocatorCallback allocator;

    allocator.Allocate =
        [](size_t _BytesCount)
    {
        TotalAllocatedRenderCore++;
        return Platform::GetHeapAllocator<HEAP_RHI>().Alloc(_BytesCount);
    };

    allocator.Deallocate =
        [](void* _Bytes)
    {
        TotalAllocatedRenderCore--;
        Platform::GetHeapAllocator<HEAP_RHI>().Free(_Bytes);
    };

    CreateLogicalDevice("OpenGL 4.5", &allocator, &m_RenderDevice);

    if (rt_VidWidth.GetInteger() <= 0 || rt_VidHeight.GetInteger() <= 0)
    {
        DisplayMode mode;

        TPodVector<DisplayInfo> displays;
        Runtime::GetDisplays(displays);
        if (!displays.IsEmpty())
        {
            Runtime::GetDesktopDisplayMode(displays[0], mode);

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
    Platform::Strcpy(desiredMode.Backend, sizeof(desiredMode.Backend), "OpenGL 4.5");
    Platform::Strcpy(desiredMode.Title, sizeof(desiredMode.Title), entryDecl.GameTitle);

    m_RenderDevice->GetOrCreateMainWindow(desiredMode, &m_Window);
    m_RenderDevice->CreateSwapChain(m_Window, &m_pSwapChain);

    // Swap buffers to prevent flickering
    m_pSwapChain->Present(rt_SwapInterval.GetInteger());

    m_VertexMemoryGPU = MakeRef<VertexMemoryGPU>(m_RenderDevice);

    // Init physics module
    //b3SetCustomPrintfFunc(PhysModulePrintFunction);
    //b3SetCustomWarningMessageFunc(PhysModuleWarningFunction);
    //b3SetCustomErrorMessageFunc(PhysModuleErrorFunction);
    //b3AlignedAllocSetCustom(PhysModuleAlloc, PhysModuleFree);
    //b3AlignedAllocSetCustomAligned(PhysModuleAlignedAlloc, PhysModuleFree);
    btAlignedAllocSetCustom(PhysModuleAlloc, PhysModuleFree);
    btAlignedAllocSetCustomAligned(PhysModuleAlignedAlloc, PhysModuleFree);

    // Init recast navigation module
    dtAllocSetCustom(NavModuleAlloc, NavModuleFree);

    m_ResourceManager = MakeUnique<ResourceManager>();

    m_Renderer = MakeRef<RenderFrontend>();

    m_RenderBackend = MakeRef<RenderBackend>(m_RenderDevice);

    m_FrameLoop = MakeRef<FrameLoop>(m_RenderDevice);

    // Process initial events
    m_FrameLoop->PollEvents(this);

    m_Canvas = MakeUnique<Canvas>();

    m_UIManager = MakeUnique<UIManager>(m_Window);

    m_GameModule = CreateGameModule(entryDecl.ModuleClass);
    m_GameModule->AddRef();

    LOG("Created game module: {}\n", m_GameModule->FinalClassName());

    m_bAllowInputEvents = true;

    do
    {
        _HK_PROFILER_FRAME("EngineFrame");

        // Garbage collect from previuous frames
        GarbageCollector::DeallocateObjects();

        // Set new frame, process game events
        m_FrameLoop->NewFrame({m_pSwapChain}, rt_SwapInterval.GetInteger());

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
        m_CommandProcessor.Execute(m_GameModule->CommandContext);

        // Tick worlds
        World::UpdateWorlds(m_FrameDurationInSeconds);

        // Update audio system
        m_AudioSystem.Update(Actor_PlayerController::GetCurrentAudioListener(), m_FrameDurationInSeconds);

        // Poll runtime events
        m_FrameLoop->PollEvents(this);

        // Update input
        UpdateInput();

        m_UIManager->Update(m_FrameDurationInSeconds);

        // Draw widgets, HUD, etc
        DrawCanvas();

        // Build frame data for rendering
        m_Renderer->Render(m_FrameLoop, m_Canvas.GetObject());

        // Generate GPU commands
        m_RenderBackend->RenderFrame(m_FrameLoop->GetStreamedMemoryGPU(), m_pSwapChain->GetBackBuffer(), m_Renderer->GetFrameData());

        SaveMemoryStats();

    } while (!IsPendingTerminate());

    m_bAllowInputEvents = false;

    m_GameModule->RemoveRef();
    m_GameModule = nullptr;

    m_UIManager.Reset();

    World::DestroyWorlds();
    World::KillWorlds();

    SoundEmitter::ClearOneShotSounds();

    m_Canvas.Reset();

    m_RenderBackend.Reset();

    m_Renderer.Reset();

    m_ResourceManager.Reset();

    m_FrameLoop.Reset();

    GarbageCollector::Shutdown();

    VisibilitySystem::PrimitivePool.Free();
    VisibilitySystem::PrimitiveLinkPool.Free();

    Platform::ShutdownProfiler();
}

void Engine::DrawCanvas()
{
    HK_PROFILER_EVENT("Draw Canvas");

    DisplayVideoMode const& videoMode = m_Window->GetVideoMode();

    m_Canvas->NewFrame(videoMode.FramebufferWidth, videoMode.FramebufferHeight);

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
        GMemoryStat[heap] = Platform::GetHeapAllocator<heap>().GetStat(); \
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
    static TStaticResourceFinder<Font> font("/Root/fonts/RobotoMono/RobotoMono-Regular.ttf"s);

    Formatter fmt;

    m_Canvas->ResetScissor();

    if (com_ShowStat)
    {
        RenderFrameData* frameData = m_Renderer->GetFrameData();

        RenderFrontendStat const& stat = m_Renderer->GetStat();

        StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

        const float y_step = 40;
        const int   numLines = 13;

        Float2 pos(8, 8);

        FontStyle fontStyle;
        fontStyle.FontSize = 24;
        
        m_Canvas->FontFace(font);

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
        m_Canvas->DrawText(fontStyle, pos, Color4::White(), fmt("Audio channels: {} active, {} virtual", m_AudioSystem.GetMixer()->GetNumActiveChannels(), m_AudioSystem.GetMixer()->GetNumVirtualChannels()), true);
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
        m_Canvas->FontFace(font);
        m_Canvas->DrawText(fontStyle, Float2(10, 30), Color4::White(), fmt("Frame time {:.1f} ms (FPS: {}, AVG {})", m_FrameDurationInSeconds * 1000.0f, int(1.0f / m_FrameDurationInSeconds), int(fps + 0.5f)), true);
    }
}

void Engine::DeveloperKeys(KeyEvent const& event)
{
}

void Engine::OnKeyEvent(KeyEvent const& event, double timeStamp)
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

    m_UIManager->GenerateKeyEvents(event, timeStamp, m_GameModule->CommandContext, m_CommandProcessor);
}

void Engine::OnMouseButtonEvent(MouseButtonEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseButtonEvents(event, timeStamp);
}

void Engine::OnMouseWheelEvent(MouseWheelEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseWheelEvents(event, timeStamp);
}

void Engine::OnMouseMoveEvent(MouseMoveEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseMoveEvents(event, timeStamp);
}

void Engine::OnJoystickButtonEvent(JoystickButtonEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateJoystickButtonEvents(event, timeStamp);
}

void Engine::OnJoystickAxisEvent(JoystickAxisEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateJoystickAxisEvents(event, timeStamp);
}

void Engine::OnCharEvent(CharEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateCharEvents(event, timeStamp);
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

    m_RetinaScale = Float2((float)videoMode.FramebufferWidth / videoMode.Width,
                           (float)videoMode.FramebufferHeight / videoMode.Height);
}

void Engine::UpdateInput()
{
    HK_PROFILER_EVENT("Update Input");

    for (InputComponent* component = InputComponent::GetInputComponents(); component; component = component->GetNext())
    {
        component->UpdateAxes(m_FrameDurationInSeconds);
    }
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

const char* Engine::GetExecutableName()
{
    ProcessInfo const& processInfo = Platform::GetProcessInfo();
    return processInfo.Executable ? processInfo.Executable : "";
}

RenderCore::IDevice* Engine::GetRenderDevice()
{
    return m_RenderDevice;
}

HK_NAMESPACE_END

extern "C" const size_t   EmbeddedResources_Size;
extern "C" const uint64_t EmbeddedResources_Data[];

namespace
{
Hk::Archive EmbeddedResourcesArch;
}

HK_NAMESPACE_BEGIN

namespace Runtime
{
Archive const& GetEmbeddedResources()
{
    if (!EmbeddedResourcesArch)
    {
        EmbeddedResourcesArch = Archive::OpenFromMemory(EmbeddedResources_Data, EmbeddedResources_Size);
        if (!EmbeddedResourcesArch)
        {
            LOG("Failed to open embedded resources\n");
        }
    }
    return EmbeddedResourcesArch;
}
} // namespace Runtime

#ifdef HK_OS_WIN32
void RunEngine(EntryDecl const& EntryDecl)
#else
void RunEngine(int _Argc, char** _Argv, EntryDecl const& EntryDecl)
#endif
{
    static bool bApplicationRun = false;

    HK_ASSERT(!bApplicationRun);
    if (bApplicationRun)
    {
        return;
    }

    bApplicationRun = true;

    PlatformInitialize init;
#ifdef HK_OS_WIN32
    init.pCommandLine = GetCommandLineA();
#else
    init.Argc = _Argc;
    init.Argv = _Argv;
#endif
    Platform::Initialize(init);

    ConsoleVar::AllocateVariables();

    MakeUnique<Engine>()->Run(EntryDecl);

    EmbeddedResourcesArch.Close();

    ConsoleVar::FreeVariables();

    Platform::Deinitialize();
}
HK_NAMESPACE_END

#if defined(HK_DEBUG) && defined(HK_COMPILER_MSVC)
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    return TRUE;
}
#endif
