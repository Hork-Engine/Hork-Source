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
#include "Actor.h"
#include "PlayerController.h"
#include "InputComponent.h"
#include "Canvas/Canvas.h"
#include "World.h"

#include <Platform/Logger.h>
#include <Platform/Platform.h>

#include <Audio/AudioMixer.h>

#include <Bullet3Common/b3Logging.h>
#include <Bullet3Common/b3AlignedAllocator.h>
#include <LinearMath/btAlignedAllocator.h>

#include <DetourAlloc.h>

#ifdef HK_OS_LINUX
#    include <unistd.h> // chdir
#endif

static AConsoleVar com_ShowStat("com_ShowStat"s, "0"s);
static AConsoleVar com_ShowFPS("com_ShowFPS"s, "0"s);

AConsoleVar rt_VidWidth("rt_VidWidth"s, "0"s);
AConsoleVar rt_VidHeight("rt_VidHeight"s, "0"s);
#ifdef HK_DEBUG
AConsoleVar rt_VidFullscreen("rt_VidFullscreen"s, "0"s);
#else
AConsoleVar rt_VidFullscreen("rt_VidFullscreen"s, "1"s);
#endif

AConsoleVar rt_SwapInterval("rt_SwapInterval"s, "0"s, 0, "1 - enable vsync, 0 - disable vsync, -1 - tearing"s);

static int TotalAllocatedRenderCore = 0;

AEngine* GEngine;

AEngine::AEngine() :
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

static AGameModule* CreateGameModule(AClassMeta const* pClassMeta)
{
    if (!pClassMeta->IsSubclassOf<AGameModule>())
    {
        CriticalError("CreateGameModule: game module is not subclass of AGameModule\n");
    }
    return static_cast<AGameModule*>(pClassMeta->CreateInstance());
}

void AEngine::LoadConfigFile()
{
    AString configFile = GetRootPath() + "config.cfg";

    AFile f = AFile::OpenRead(configFile);
    if (f)
    {
        m_CommandProcessor.Add(f.AsString());

        class CommandContext : public ICommandContext
        {
        public:
            void ExecuteCommand(ACommandProcessor const& _Proc) override
            {
                HK_ASSERT(_Proc.GetArgsCount() > 0);

                const char*  name = _Proc.GetArg(0);
                AConsoleVar* var;
                if (nullptr != (var = AConsoleVar::FindVariable(name)))
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

void AEngine::InitializeDirectories()
{
    SProcessInfo const& processInfo = Platform::GetProcessInfo();

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

void AEngine::Run(SEntryDecl const& entryDecl)
{
    m_pModuleDecl = &entryDecl;

    InitializeDirectories();

    if (AThread::NumHardwareThreads)
    {
        LOG("Num hardware threads: {}\n", AThread::NumHardwareThreads);
    }

    int jobManagerThreadCount = AThread::NumHardwareThreads ? Math::Min(AThread::NumHardwareThreads, AAsyncJobManager::MAX_WORKER_THREADS) : AAsyncJobManager::MAX_WORKER_THREADS;

    AsyncJobManager = MakeRef<AAsyncJobManager>(jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS);

    RenderFrontendJobList = AsyncJobManager->GetAsyncJobList(RENDER_FRONTEND_JOB_LIST);
    RenderBackendJobList  = AsyncJobManager->GetAsyncJobList(RENDER_BACKEND_JOB_LIST);

    LoadConfigFile();

    RenderCore::SAllocatorCallback allocator;

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
        SDisplayMode mode;

        TPodVector<SDisplayInfo> displays;
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

    SVideoMode desiredMode  = {};
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

    m_VertexMemoryGPU = MakeRef<AVertexMemoryGPU>(m_RenderDevice);

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

    m_ResourceManager = MakeUnique<AResourceManager>();

    m_Renderer = NewObj<ARenderFrontend>();

    m_RenderBackend = MakeRef<ARenderBackend>(m_RenderDevice);

    m_FrameLoop = MakeRef<AFrameLoop>(m_RenderDevice);

    // Process initial events
    m_FrameLoop->PollEvents(this);

    m_Canvas = MakeUnique<ACanvas>();

    m_UIManager = MakeUnique<UIManager>(m_Window);

    m_GameModule = CreateGameModule(entryDecl.ModuleClass);
    m_GameModule->AddRef();

    LOG("Created game module: {}\n", m_GameModule->FinalClassName());

    m_bAllowInputEvents = true;

    do
    {
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
        AWorld::UpdateWorlds(m_FrameDurationInSeconds);

        // Update audio system
        m_AudioSystem.Update(APlayerController::GetCurrentAudioListener(), m_FrameDurationInSeconds);

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

    AWorld::DestroyWorlds();
    AWorld::KillWorlds();

    ASoundEmitter::ClearOneShotSounds();

    m_Canvas.Reset();

    m_RenderBackend.Reset();

    m_Renderer.Reset();

    m_ResourceManager.Reset();

    m_FrameLoop.Reset();

    GarbageCollector::Shutdown();

    AVisibilitySystem::PrimitivePool.Free();
    AVisibilitySystem::PrimitiveLinkPool.Free();
}

void AEngine::DrawCanvas()
{
    SVideoMode const& videoMode = m_Window->GetVideoMode();

    m_Canvas->NewFrame(videoMode.FramebufferWidth, videoMode.FramebufferHeight);

    if (IsWindowVisible())
    {
        m_UIManager->Draw(*m_Canvas);

        ShowStats();
    }
}

SMemoryStat GMemoryStat[HEAP_MAX];
SMemoryStat GMemoryStatGlobal;

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

void AEngine::SaveMemoryStats()
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
#include <SDL.h>

void AEngine::ShowStats()
{
    static TStaticResourceFinder<AFont> font("/Root/fonts/RobotoMono/RobotoMono-Regular.ttf"s);

    AFormatter fmt;

    m_Canvas->ResetScissor();

    if (com_ShowStat)
    {
        SRenderFrame* frameData = m_Renderer->GetFrameData();

        SRenderFrontendStat const& stat = m_Renderer->GetStat();

        AStreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

        const float y_step = 40;
        const int   numLines = 13;

        Float2 pos(8, 8);

        FontStyle fontStyle;
        fontStyle.FontSize = 24;
        
        m_Canvas->FontFace(font);

        pos.Y = 100;
        for (int n = 0; n < HEAP_MAX; n++)
        {
            SMemoryStat& memstat = GMemoryStat[n];

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

void AEngine::DeveloperKeys(SKeyEvent const& event)
{
}

void AEngine::OnKeyEvent(SKeyEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    if (m_GameModule->bQuitOnEscape && event.Action == IA_PRESS && event.Key == KEY_ESCAPE)
    {
        m_GameModule->OnGameClose();
    }

    // Check Alt+Enter to toggle fullscreen/windowed mode
    if (m_GameModule->bToggleFullscreenAltEnter)
    {
        if (event.Action == IA_PRESS && event.Key == KEY_ENTER && (HAS_MODIFIER(event.ModMask, KEY_MOD_ALT)))
        {
            SVideoMode videoMode  = m_Window->GetVideoMode();
            videoMode.bFullscreen = !videoMode.bFullscreen;
            PostChangeVideoMode(videoMode);
        }
    }

    DeveloperKeys(event);

    m_UIManager->GenerateKeyEvents(event, timeStamp, m_GameModule->CommandContext, m_CommandProcessor);
}

void AEngine::OnMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseButtonEvents(event, timeStamp);
}

void AEngine::OnMouseWheelEvent(SMouseWheelEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseWheelEvents(event, timeStamp);
}

void AEngine::OnMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateMouseMoveEvents(event, timeStamp);
}

void AEngine::OnJoystickButtonEvent(SJoystickButtonEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateJoystickButtonEvents(event, timeStamp);
}

void AEngine::OnJoystickAxisEvent(SJoystickAxisEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateJoystickAxisEvents(event, timeStamp);
}

void AEngine::OnCharEvent(SCharEvent const& event, double timeStamp)
{
    if (!m_bAllowInputEvents)
    {
        return;
    }

    m_UIManager->GenerateCharEvents(event, timeStamp);
}

void AEngine::OnWindowVisible(bool bVisible)
{
    m_bIsWindowVisible = bVisible;
}

void AEngine::OnCloseEvent()
{
    m_GameModule->OnGameClose();
}

void AEngine::OnResize()
{
    SVideoMode const& videoMode = m_Window->GetVideoMode();

    m_RetinaScale = Float2((float)videoMode.FramebufferWidth / videoMode.Width,
                           (float)videoMode.FramebufferHeight / videoMode.Height);
}

void AEngine::UpdateInput()
{
    for (AInputComponent* component = AInputComponent::GetInputComponents(); component; component = component->GetNext())
    {
        component->UpdateAxes(m_FrameDurationInSeconds);
    }
}

void AEngine::MapWindowCoordinate(float& InOutX, float& InOutY) const
{
    SVideoMode const& videoMode = m_Window->GetVideoMode();
    InOutX += videoMode.X;
    InOutY += videoMode.Y;
}

void AEngine::UnmapWindowCoordinate(float& InOutX, float& InOutY) const
{
    SVideoMode const& videoMode = m_Window->GetVideoMode();
    InOutX -= videoMode.X;
    InOutY -= videoMode.Y;
}

void AEngine::PostChangeVideoMode(SVideoMode const& _DesiredMode)
{
    m_DesiredMode          = _DesiredMode;
    m_bPostChangeVideoMode = true;
}

void AEngine::PostTerminateEvent()
{
    m_bPostTerminateEvent = true;
}

bool AEngine::IsPendingTerminate()
{
    return m_bPostTerminateEvent;
}

void AEngine::ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem)
{
    RenderCore::ITexture* pBackBuffer = m_pSwapChain->GetBackBuffer();

    RenderCore::STextureRect rect;
    rect.Offset.X    = _X;
    rect.Offset.Y    = _Y;
    rect.Dimension.X = _Width;
    rect.Dimension.Y = _Height;
    rect.Dimension.Z = 1;

    pBackBuffer->ReadRect(rect, _SizeInBytes, 4, _SysMem);
}

AString const& AEngine::GetWorkingDir()
{
    return m_WorkingDir;
}

AString const& AEngine::GetRootPath()
{
    return m_RootPath;
}

const char* AEngine::GetExecutableName()
{
    SProcessInfo const& processInfo = Platform::GetProcessInfo();
    return processInfo.Executable ? processInfo.Executable : "";
}

RenderCore::IDevice* AEngine::GetRenderDevice()
{
    return m_RenderDevice;
}


extern "C" const size_t   EmbeddedResources_Size;
extern "C" const uint64_t EmbeddedResources_Data[];

static AArchive EmbeddedResourcesArch;

namespace Runtime
{
AArchive const& GetEmbeddedResources()
{
    if (!EmbeddedResourcesArch)
    {
        EmbeddedResourcesArch = AArchive::OpenFromMemory(EmbeddedResources_Data, EmbeddedResources_Size);
        if (!EmbeddedResourcesArch)
        {
            LOG("Failed to open embedded resources\n");
        }
    }
    return EmbeddedResourcesArch;
}
} // namespace Runtime

#ifdef HK_OS_WIN32
void RunEngine(SEntryDecl const& EntryDecl)
#else
void RunEngine(int _Argc, char** _Argv, SEntryDecl const& EntryDecl)
#endif
{
    static bool bApplicationRun = false;

    HK_ASSERT(!bApplicationRun);
    if (bApplicationRun)
    {
        return;
    }

    bApplicationRun = true;

    SPlatformInitialize init;
#ifdef HK_OS_WIN32
    init.pCommandLine = ::GetCommandLineA();
#else
    init.Argc = _Argc;
    init.Argv = _Argv;
#endif
    Platform::Initialize(init);

    AConsoleVar::AllocateVariables();

    MakeUnique<AEngine>()->Run(EntryDecl);

    EmbeddedResourcesArch.Close();

    AConsoleVar::FreeVariables();

    Platform::Deinitialize();
}

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
