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
#include "Canvas.h"
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

//#define IMGUI_CONTEXT

static AConsoleVar com_ShowStat(_CTS("com_ShowStat"), _CTS("0"));
static AConsoleVar com_ShowFPS(_CTS("com_ShowFPS"), _CTS("0"));
static AConsoleVar com_SimulateCursorBallistics(_CTS("com_SimulateCursorBallistics"), _CTS("1"));

AConsoleVar rt_VidWidth(_CTS("rt_VidWidth"), _CTS("0"));
AConsoleVar rt_VidHeight(_CTS("rt_VidHeight"), _CTS("0"));
#ifdef HK_DEBUG
AConsoleVar rt_VidFullscreen(_CTS("rt_VidFullscreen"), _CTS("0"));
#else
AConsoleVar rt_VidFullscreen(_CTS("rt_VidFullscreen"), _CTS("1"));
#endif

AConsoleVar rt_SwapInterval(_CTS("rt_SwapInterval"), _CTS("0"), 0, _CTS("1 - enable vsync, 0 - disable vsync, -1 - tearing"));

static int TotalAllocatedRenderCore = 0;

AEngine* GEngine;

AEngine::AEngine() :
    Rand(Core::RandomSeed())
{
    GEngine     = this;
    RetinaScale = Float2(1.0f);
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
    HK_ASSERT(_Alignment <= 16);
    return GZoneMemory.Alloc(_BytesCount);
}

static void* PhysModuleAlloc(size_t _BytesCount)
{
    return GZoneMemory.Alloc(_BytesCount);
}

static void PhysModuleFree(void* _Bytes)
{
    GZoneMemory.Free(_Bytes);
}

static void* NavModuleAlloc(size_t _BytesCount, dtAllocHint _Hint)
{
    return GHeapMemory.Alloc(_BytesCount);
    //return GZoneMemory.Alloc( _BytesCount );
}

static void NavModuleFree(void* _Bytes)
{
    GHeapMemory.Free(_Bytes);
    //GZoneMemory.Free( _Bytes );
}

#if 0
static void *ImguiModuleAlloc( size_t _BytesCount, void * )
{
    return GZoneMemory.Alloc( _BytesCount );
}

static void ImguiModuleFree( void * _Bytes, void * )
{
    GZoneMemory.Free( _Bytes );
}
#endif

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

    AFileStream f;
    if (f.OpenRead(configFile))
    {
        AString data;
        data.FromFile(f);

        CommandProcessor.Add(data.CStr());

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

        CommandProcessor.Execute(context);
    }
}

void AEngine::InitializeDirectories()
{
    SProcessInfo const& processInfo = Platform::GetProcessInfo();

    WorkingDir = processInfo.Executable;
    WorkingDir.ClipFilename();

#if defined HK_OS_WIN32
    SetCurrentDirectoryA(WorkingDir.CStr());
#elif defined HK_OS_LINUX
    int r = chdir(WorkingDir.CStr());
    if (r != 0)
    {
        LOG("Cannot set working directory\n");
    }
#else
#    error "InitializeDirectories not implemented under current platform"
#endif

    RootPath = pModuleDecl->RootPath;
    if (RootPath.IsEmpty())
    {
        RootPath = "Data/";
    }
    else
    {
        RootPath.FixSeparator();
        if (RootPath[RootPath.Length() - 1] != '/')
        {
            RootPath += '/';
        }
    }

    LOG("Working directory: {}\n", GetWorkingDir());
    LOG("Root path: {}\n", GetRootPath());
    LOG("Executable: {}\n", GetExecutableName());
}

void AEngine::Run(SEntryDecl const& _EntryDecl)
{
    pModuleDecl = &_EntryDecl;

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
        return GZoneMemory.Alloc(_BytesCount);
    };

    allocator.Deallocate =
        [](void* _Bytes)
    {
        TotalAllocatedRenderCore--;
        GZoneMemory.Free(_Bytes);
    };

    CreateLogicalDevice("OpenGL 4.5", &allocator, &RenderDevice);

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
    Platform::Strcpy(desiredMode.Title, sizeof(desiredMode.Title), _EntryDecl.GameTitle);

    RenderDevice->GetOrCreateMainWindow(desiredMode, &Window);
    RenderDevice->CreateSwapChain(Window, &pSwapChain);

    // Swap buffers to prevent flickering
    pSwapChain->Present(rt_SwapInterval.GetInteger());

    VertexMemoryGPU = MakeRef<AVertexMemoryGPU>(RenderDevice);

    Console.ReadStoryLines();

    InitializeFactories();

    // Init physics module
    b3SetCustomPrintfFunc(PhysModulePrintFunction);
    b3SetCustomWarningMessageFunc(PhysModuleWarningFunction);
    b3SetCustomErrorMessageFunc(PhysModuleErrorFunction);
    b3AlignedAllocSetCustom(PhysModuleAlloc, PhysModuleFree);
    b3AlignedAllocSetCustomAligned(PhysModuleAlignedAlloc, PhysModuleFree);
    btAlignedAllocSetCustom(PhysModuleAlloc, PhysModuleFree);
    btAlignedAllocSetCustomAligned(PhysModuleAlignedAlloc, PhysModuleFree);

    // Init recast navigation module
    dtAllocSetCustom(NavModuleAlloc, NavModuleFree);
#if 0
    // Init Imgui allocators
    ImGui::SetAllocatorFunctions( ImguiModuleAlloc, ImguiModuleFree, NULL );
#endif
    ResourceManager = MakeUnique<AResourceManager>();

    Renderer = CreateInstanceOf<ARenderFrontend>();

    RenderBackend = MakeRef<ARenderBackend>(RenderDevice);

    AFont::SetGlyphRanges(GLYPH_RANGE_CYRILLIC);

    FrameLoop = MakeRef<AFrameLoop>(RenderDevice);

    // Process initial events
    FrameLoop->PollEvents(this);



    GameModule = CreateGameModule(_EntryDecl.ModuleClass);
    GameModule->AddRef();

    LOG("Created game module: {}\n", GameModule->FinalClassName());

#ifdef IMGUI_CONTEXT
    ImguiContext = CreateInstanceOf<AImguiContext>();
    ImguiContext->SetFont(ACanvas::GetDefaultFont());
    ImguiContext->AddRef();
#endif

    bAllowInputEvents = true;

    do
    {
        // Set new frame, process game events
        FrameLoop->NewFrame({pSwapChain}, rt_SwapInterval.GetInteger());

        if (bPostChangeVideoMode)
        {
            bPostChangeVideoMode = false;

            Window->SetVideoMode(DesiredMode);

            // Swap buffers to prevent flickering
            pSwapChain->Present(rt_SwapInterval.GetInteger());
        }

        // Take current frame duration
        FrameDurationInSeconds = FrameLoop->SysFrameDuration() * 0.000001;

        // Don't allow very slow frames
        if (FrameDurationInSeconds > 0.5f)
        {
            FrameDurationInSeconds = 0.5f;
        }

        // Garbage collect from previuous frames
        AGarbageCollector::DeallocateObjects();

        // Execute console commands
        CommandProcessor.Execute(GameModule->CommandContext);

        // Tick worlds
        AWorld::UpdateWorlds(FrameDurationInSeconds);

        // Update audio system
        AudioSystem.Update(APlayerController::GetCurrentAudioListener(), FrameDurationInSeconds);

        // Poll runtime events
        FrameLoop->PollEvents(this);

        // Update input
        UpdateInput();

#ifdef IMGUI_CONTEXT
        // Imgui test
        UpdateImgui();
#endif
        // Draw widgets, HUD, etc
        DrawCanvas();

        // Build frame data for rendering
        Renderer->Render(FrameLoop, &Canvas);

        // Generate GPU commands
        RenderBackend->RenderFrame(FrameLoop->GetStreamedMemoryGPU(), pSwapChain->GetBackBuffer(), Renderer->GetFrameData());

    } while (!IsPendingTerminate());

    bAllowInputEvents = false;

    GameModule->RemoveRef();
    GameModule = nullptr;

    Desktop.Reset();

    AWorld::DestroyWorlds();
    AWorld::KillWorlds();

    ASoundEmitter::ClearOneShotSounds();

#ifdef IMGUI_CONTEXT
    ImguiContext->RemoveRef();
    ImguiContext = nullptr;
#endif

    RenderBackend.Reset();

    Renderer.Reset();

    ResourceManager.Reset();

    AGarbageCollector::DeallocateObjects();

    AVisibilitySystem::PrimitivePool.Free();
    AVisibilitySystem::PrimitiveLinkPool.Free();

    DeinitializeFactories();

    Console.WriteStoryLines();
}

void AEngine::DrawCanvas()
{
    SVideoMode const& videoMode = Window->GetVideoMode();

    Canvas.Begin(videoMode.FramebufferWidth, videoMode.FramebufferHeight);

    if (IsWindowVisible())
    {
        if (Desktop)
        {
            // Draw desktop
            Desktop->GenerateWindowHoverEvents();
            Desktop->GenerateDrawEvents(Canvas);
            if (Desktop->IsCursorVisible() && !Platform::IsCursorEnabled())
            {
                Desktop->DrawCursor(Canvas);
            }

            // Draw halfscreen console
            Console.SetFullscreen(false);
            Console.Draw(&Canvas, FrameDurationInSeconds);
        }
        else
        {
            // Draw fullscreen console
            Console.SetFullscreen(true);
            Console.Draw(&Canvas, FrameDurationInSeconds);
        }

        ShowStats();
    }

    Canvas.End();
}

void AEngine::ShowStats()
{
    static TStaticResourceFinder<AFont> Impact18(_CTS("/Root/impact18.font"));
    AFont*                              font = Impact18.GetObject();

    if (com_ShowStat)
    {
        SRenderFrame* frameData = Renderer->GetFrameData();

        const size_t TotalMemorySizeInBytes = ((GZoneMemory.GetZoneMemorySizeInMegabytes() << 20) + (GHunkMemory.GetHunkMemorySizeInMegabytes() << 20) + FrameLoop->GetFrameMemorySize());

        SRenderFrontendStat const& stat = Renderer->GetStat();

        AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

        const float y_step   = 22;
        const int   numLines = 13;

        Float2 pos(8, 8);
        pos.Y = Canvas.GetHeight() - numLines * y_step;

        Canvas.PushFont(font);
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Zone memory usage: %f KB / %d MB", GZoneMemory.GetTotalMemoryUsage() / 1024.0f, GZoneMemory.GetZoneMemorySizeInMegabytes()), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Hunk memory usage: %f KB / %d MB", GHunkMemory.GetTotalMemoryUsage() / 1024.0f, GHunkMemory.GetHunkMemorySizeInMegabytes()), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Frame memory usage: %f KB / %d MB (Max %f KB)", FrameLoop->GetFrameMemoryUsedPrev() / 1024.0f, FrameLoop->GetFrameMemorySize() >> 20, FrameLoop->GetMaxFrameMemoryUsage() / 1024.0f), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Frame memory usage (GPU): %f KB / %d MB (Max %f KB)", streamedMemory->GetUsedMemoryPrev() / 1024.0f, streamedMemory->GetAllocatedMemory() >> 20, streamedMemory->GetMaxMemoryUsage() / 1024.0f), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Vertex cache memory usage (GPU): %f KB / %d MB", VertexMemoryGPU->GetUsedMemory() / 1024.0f, VertexMemoryGPU->GetAllocatedMemory() >> 20), true);
        pos.Y += y_step;
        if (GHeapMemory.GetTotalMemoryUsage() > 0)
        {
            Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Heap memory usage: %f KB", (GHeapMemory.GetTotalMemoryUsage() - TotalMemorySizeInBytes) / 1024.0f), true);
            pos.Y += y_step;
        }
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Visible instances: %d", frameData->Instances.Size()), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Visible shadow instances: %d", frameData->ShadowInstances.Size()), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Visible dir lights: %d", frameData->DirectionalLights.Size()), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Polycount: %d", stat.PolyCount), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("ShadowMapPolyCount: %d", stat.ShadowMapPolyCount), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Frontend time: %d msec", stat.FrontendTime), true);
        pos.Y += y_step;
        Canvas.DrawTextUTF8(pos, Color4::White(), Platform::Fmt("Audio channels: %d active, %d virtual", AudioSystem.GetMixer()->GetNumActiveChannels(), AudioSystem.GetMixer()->GetNumVirtualChannels()), true);

        Canvas.PopFont();
    }

    if (com_ShowFPS)
    {
        enum
        {
            FPS_BUF = 16
        };
        static float fpsavg[FPS_BUF];
        static int   n            = 0;
        fpsavg[n & (FPS_BUF - 1)] = FrameDurationInSeconds;
        n++;
        float fps = 0;
        for (int i = 0; i < FPS_BUF; i++)
            fps += fpsavg[i];
        fps *= (1.0f / FPS_BUF);
        fps = 1.0f / (fps > 0.0f ? fps : 1.0f);
        Canvas.PushFont(font);
        Canvas.DrawTextUTF8(Float2(10, 10), Color4::White(), Platform::Fmt("Frame time %.1f ms (FPS: %d, AVG %d)", FrameDurationInSeconds * 1000.0f, int(1.0f / FrameDurationInSeconds), int(fps + 0.5f)), true);
        Canvas.PopFont();
    }
}

void AEngine::DeveloperKeys(SKeyEvent const& _Event)
{
}

void AEngine::OnKeyEvent(SKeyEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

    if (GameModule->bQuitOnEscape && _Event.Action == IA_PRESS && _Event.Key == KEY_ESCAPE)
    {
        GameModule->OnGameClose();
    }

    // Check Alt+Enter to toggle fullscreen/windowed mode
    if (GameModule->bToggleFullscreenAltEnter)
    {
        if (_Event.Action == IA_PRESS && _Event.Key == KEY_ENTER && (HAS_MODIFIER(_Event.ModMask, KMOD_ALT)))
        {
            SVideoMode videoMode  = Window->GetVideoMode();
            videoMode.bFullscreen = !videoMode.bFullscreen;
            PostChangeVideoMode(videoMode);
        }
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnKeyEvent(_Event);
#endif

    DeveloperKeys(_Event);

    if (Console.IsActive() || GameModule->bAllowConsole)
    {
        Console.KeyEvent(_Event, GameModule->CommandContext, CommandProcessor);

        if (!Console.IsActive() && _Event.Key == KEY_GRAVE_ACCENT)
        {
            // Console just closed
            return;
        }
    }

    if (Console.IsActive() && _Event.Action != IA_RELEASE)
    {
        return;
    }

    if (Desktop)
    {
        Desktop->GenerateKeyEvents(_Event, _TimeStamp);
    }
}

void AEngine::OnMouseButtonEvent(SMouseButtonEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseButtonEvent(_Event);
#endif

    if (Console.IsActive() && _Event.Action != IA_RELEASE)
    {
        return;
    }

    if (Desktop)
    {
        Desktop->GenerateMouseButtonEvents(_Event, _TimeStamp);
    }
}

void AEngine::OnMouseWheelEvent(SMouseWheelEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseWheelEvent(_Event);
#endif

    Console.MouseWheelEvent(_Event);
    if (Console.IsActive())
    {
        return;
    }

    if (Desktop)
    {
        Desktop->GenerateMouseWheelEvents(_Event, _TimeStamp);
    }
}

void AEngine::OnMouseMoveEvent(SMouseMoveEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

    if (Desktop)
    {
        SVideoMode const& videoMode = Window->GetVideoMode();

        if (Platform::IsCursorEnabled())
        {
            Float2 cursorPosition;
            int    x, y;

            Platform::GetCursorPosition(x, y);

            cursorPosition.X = Math::Clamp(x, 0, videoMode.FramebufferWidth - 1);
            cursorPosition.Y = Math::Clamp(y, 0, videoMode.FramebufferHeight - 1);

            Desktop->SetCursorPosition(cursorPosition);
        }
        else
        {
            Float2 cursorPosition = Desktop->GetCursorPosition();

            // Simulate ballistics
            if (com_SimulateCursorBallistics)
            {
                cursorPosition.X += _Event.X / videoMode.RefreshRate * videoMode.DPI_X;
                cursorPosition.Y -= _Event.Y / videoMode.RefreshRate * videoMode.DPI_Y;
            }
            else
            {
                cursorPosition.X += _Event.X;
                cursorPosition.Y -= _Event.Y;
            }
            cursorPosition = Math::Clamp(cursorPosition, Float2(0.0f), Float2(videoMode.FramebufferWidth - 1, videoMode.FramebufferHeight - 1));

            Desktop->SetCursorPosition(cursorPosition);
        }

        if (!Console.IsActive())
        {
            Desktop->GenerateMouseMoveEvents(_Event, _TimeStamp);
        }
    }
}

void AEngine::OnJoystickButtonEvent(SJoystickButtonEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

    if (Console.IsActive() && _Event.Action != IA_RELEASE)
    {
        return;
    }

    if (Desktop)
    {
        Desktop->GenerateJoystickButtonEvents(_Event, _TimeStamp);
    }
}

void AEngine::OnJoystickAxisEvent(SJoystickAxisEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

    if (Desktop)
    {
        Desktop->GenerateJoystickAxisEvents(_Event, _TimeStamp);
    }
}

void AEngine::OnCharEvent(SCharEvent const& _Event, double _TimeStamp)
{
    if (!bAllowInputEvents)
    {
        return;
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnCharEvent(_Event);
#endif

    Console.CharEvent(_Event);
    if (Console.IsActive())
    {
        return;
    }

    if (Desktop)
    {
        Desktop->GenerateCharEvents(_Event, _TimeStamp);
    }
}

void AEngine::OnWindowVisible(bool _Visible)
{
    bIsWindowVisible = _Visible;
}

void AEngine::OnCloseEvent()
{
    GameModule->OnGameClose();
}

void AEngine::OnResize()
{
    SVideoMode const& videoMode = Window->GetVideoMode();

    RetinaScale = Float2((float)videoMode.FramebufferWidth / videoMode.Width,
                         (float)videoMode.FramebufferHeight / videoMode.Height);

    if (Desktop)
    {
        // Force update transform
        Desktop->MarkTransformDirty();

        // Set size
        Desktop->SetSize(videoMode.FramebufferWidth, videoMode.FramebufferHeight);
    }
}

void AEngine::UpdateInput()
{
    SVideoMode const& videoMode = Window->GetVideoMode();

    switch (GameModule->CursorMode)
    {
        case CURSOR_MODE_AUTO:
            if (!videoMode.bFullscreen && Console.IsActive())
            {
                Platform::SetCursorEnabled(true);
            }
            else
            {
                Platform::SetCursorEnabled(false);
            }
            break;
        case CURSOR_MODE_FORCE_ENABLED:
            Platform::SetCursorEnabled(true);
            break;
        case CURSOR_MODE_FORCE_DISABLED:
            Platform::SetCursorEnabled(false);
            break;
        default:
            HK_ASSERT(0);
    }

    for (AInputComponent* component = AInputComponent::GetInputComponents(); component; component = component->GetNext())
    {
        component->UpdateAxes(FrameDurationInSeconds);
    }
}

void AEngine::MapWindowCoordinate(float& InOutX, float& InOutY) const
{
    SVideoMode const& videoMode = Window->GetVideoMode();
    InOutX += videoMode.X;
    InOutY += videoMode.Y;
}

void AEngine::UnmapWindowCoordinate(float& InOutX, float& InOutY) const
{
    SVideoMode const& videoMode = Window->GetVideoMode();
    InOutX -= videoMode.X;
    InOutY -= videoMode.Y;
}

void AEngine::SetDesktop(WDesktop* _Desktop)
{
    if (IsSame(Desktop, _Desktop))
    {
        return;
    }

    //if ( Desktop ) {
    //    Desktop->SetFocusWidget( nullptr );
    //}

    Desktop = _Desktop;

    if (Desktop)
    {
        // Force update transform
        Desktop->MarkTransformDirty();

        // Set size
        SVideoMode const& videoMode = Window->GetVideoMode();
        Desktop->SetSize(videoMode.FramebufferWidth, videoMode.FramebufferHeight);
    }
}

void AEngine::PostChangeVideoMode(SVideoMode const& _DesiredMode)
{
    DesiredMode          = _DesiredMode;
    bPostChangeVideoMode = true;
}

void AEngine::PostTerminateEvent()
{
    bPostTerminateEvent = true;
}

bool AEngine::IsPendingTerminate()
{
    return bPostTerminateEvent;
}

void AEngine::ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem)
{
    RenderCore::ITexture* pBackBuffer = pSwapChain->GetBackBuffer();

    RenderCore::STextureRect rect;
    rect.Offset.X    = _X;
    rect.Offset.Y    = _Y;
    rect.Dimension.X = _Width;
    rect.Dimension.Y = _Height;
    rect.Dimension.Z = 1;

    pBackBuffer->ReadRect(rect, RenderCore::FORMAT_UBYTE4, _SizeInBytes, 4, _SysMem);

    // Swap to rgba
    int count = _Width * _Height * 4;
    for (int i = 0; i < count; i += 4)
    {
        std::swap(((uint8_t*)_SysMem)[i], ((uint8_t*)_SysMem)[i + 2]);
    }
}

AString const& AEngine::GetWorkingDir()
{
    return WorkingDir;
}

AString const& AEngine::GetRootPath()
{
    return RootPath;
}

const char* AEngine::GetExecutableName()
{
    SProcessInfo const& processInfo = Platform::GetProcessInfo();
    return processInfo.Executable ? processInfo.Executable : "";
}

RenderCore::IDevice* AEngine::GetRenderDevice()
{
    return RenderDevice;
}


extern "C" const size_t   EmbeddedResources_Size;
extern "C" const uint64_t EmbeddedResources_Data[];

static TUniqueRef<AArchive> EmbeddedResourcesArch;

namespace Runtime
{
AArchive const& GetEmbeddedResources()
{
    if (!EmbeddedResourcesArch)
    {
        EmbeddedResourcesArch = MakeUnique<AArchive>();
        if (!EmbeddedResourcesArch->OpenFromMemory(EmbeddedResources_Data, EmbeddedResources_Size))
        {
            LOG("Failed to open embedded resources\n");
        }
    }
    return *EmbeddedResourcesArch.GetObject();
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
    init.bAllowMultipleInstances = false;
    init.ZoneSizeInMegabytes     = 256;
    init.HunkSizeInMegabytes     = 32;
    Platform::Initialize(init);

    AConsoleVar::AllocateVariables();

    MakeUnique<AEngine>()->Run(EntryDecl);

    EmbeddedResourcesArch.Reset();

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
