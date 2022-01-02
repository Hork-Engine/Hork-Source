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

#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/EngineInterface.h>
#include <Runtime/Public/InputDefs.h>

#include <Platform/Public/Platform.h>
#include <Platform/Public/Logger.h>
#include <Platform/Public/WindowsDefs.h>
#include <Core/Public/HashFunc.h>
#include <Core/Public/Document.h>
#include <Core/Public/Image.h>

#include "GPUSync.h"

#include <SDL.h>

#include <chrono>

#ifdef AN_OS_LINUX
#    include <unistd.h> // chdir
#endif

#include <Runtime/Public/EntryDecl.h>

ARuntime* GRuntime;

ARuntimeVariable rt_VidWidth(_CTS("rt_VidWidth"), _CTS("0"));
ARuntimeVariable rt_VidHeight(_CTS("rt_VidHeight"), _CTS("0"));
#ifdef AN_DEBUG
ARuntimeVariable rt_VidFullscreen(_CTS("rt_VidFullscreen"), _CTS("0"));
#else
ARuntimeVariable rt_VidFullscreen(_CTS("rt_VidFullscreen"), _CTS("1"));
#endif
ARuntimeVariable rt_SyncGPU(_CTS("rt_SyncGPU"), _CTS("0"));

ARuntimeVariable rt_SwapInterval(_CTS("rt_SwapInterval"), _CTS("0"), 0, _CTS("1 - enable vsync, 0 - disable vsync, -1 - tearing"));

static SDL_Window* CreateGenericWindow(SVideoMode const& VideoMode);
static void        DestroyGenericWindow(SDL_Window* Window);

ARuntime::ARuntime(struct SEntryDecl const& EntryDecl) :
    Rand(Core::RandomSeed())
{
    GRuntime = this;

    ARuntimeVariable::AllocateVariables();

    FrameTimeStamp = Platform::SysStartMicroseconds();
    FrameDuration  = 1000000.0 / 60;
    FrameNumber    = 0;

    pModuleDecl = &EntryDecl;

    Engine = CreateEngineInstance();

    GLogger.SetMessageCallback([](int Level, const char* Message, void* UserData)
                               {
                                   Platform::WriteDebugString(Message);
                                   Platform::WriteLog(Message);

                                   IEngineInterface* engine = ((ARuntime*)UserData)->Engine;

                                   if (engine)
                                   {
                                       std::string& messageBuffer = Platform::GetMessageBuffer();
                                       if (!messageBuffer.empty())
                                       {
                                           engine->Print(messageBuffer.c_str());
                                           messageBuffer.clear();
                                       }
                                       engine->Print(Message);
                                   }
                               },
                               this);

    InitializeWorkingDirectory();

    GLogger.Printf("Working directory: %s\n", WorkingDir.CStr());
    GLogger.Printf("Root path: %s\n", RootPath.CStr());
    GLogger.Printf("Executable: %s\n", Platform::GetProcessInfo().Executable);

    SDL_LogSetOutputFunction(
        [](void* userdata, int category, SDL_LogPriority priority, const char* message)
        {
            GLogger.Printf("SDL: %d : %s\n", category, message);
        },
        NULL);

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_SENSOR | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);

    if (AThread::NumHardwareThreads)
    {
        GLogger.Printf("Num hardware threads: %d\n", AThread::NumHardwareThreads);
    }

    int jobManagerThreadCount = AThread::NumHardwareThreads ? Math::Min(AThread::NumHardwareThreads, AAsyncJobManager::MAX_WORKER_THREADS) : AAsyncJobManager::MAX_WORKER_THREADS;

    AsyncJobManager = MakeRef<AAsyncJobManager>(jobManagerThreadCount, MAX_RUNTIME_JOB_LISTS);

    RenderFrontendJobList = AsyncJobManager->GetAsyncJobList(RENDER_FRONTEND_JOB_LIST);
    RenderBackendJobList  = AsyncJobManager->GetAsyncJobList(RENDER_BACKEND_JOB_LIST);

    Platform::ZeroMem(PressedKeys.ToPtr(), sizeof(PressedKeys));
    Platform::ZeroMem(PressedMouseButtons.ToPtr(), sizeof(PressedMouseButtons));
    Platform::ZeroMem(JoystickButtonState.ToPtr(), sizeof(JoystickButtonState));
    Platform::ZeroMem(JoystickAxisState.ToPtr(), sizeof(JoystickAxisState));
    Platform::ZeroMem(JoystickAdded.ToPtr(), sizeof(JoystickAdded));

    LoadConfigFile();

    if (rt_VidWidth.GetInteger() <= 0 || rt_VidHeight.GetInteger() <= 0)
    {
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(0, &mode);

        rt_VidWidth.ForceInteger(mode.w);
        rt_VidHeight.ForceInteger(mode.h);
    }

    SVideoMode desiredMode  = {};
    desiredMode.Width       = rt_VidWidth.GetInteger();
    desiredMode.Height      = rt_VidHeight.GetInteger();
    desiredMode.Opacity     = 1;
    desiredMode.bFullscreen = rt_VidFullscreen;
    desiredMode.bCentrized  = true;
    Platform::Strcpy(desiredMode.Backend, sizeof(desiredMode.Backend), "OpenGL 4.5");
    Platform::Strcpy(desiredMode.Title, sizeof(desiredMode.Title), EntryDecl.GameTitle);

    WindowHandle = CreateGenericWindow(desiredMode);
    if (!WindowHandle)
    {
        CriticalError("Failed to create main window\n");
    }

    RenderCore::SImmediateContextDesc stateCreateInfo = {};
    stateCreateInfo.Window                            = WindowHandle;

    RenderCore::SAllocatorCallback allocator;

    allocator.Allocate =
        [](size_t _BytesCount)
    {
        GRuntime->TotalAllocatedRenderCore++;
        return GZoneMemory.Alloc(_BytesCount);
    };

    allocator.Deallocate =
        [](void* _Bytes)
    {
        GRuntime->TotalAllocatedRenderCore--;
        GZoneMemory.Free(_Bytes);
    };

    CreateLogicalDevice(stateCreateInfo,
                        &allocator,
                        &RenderDevice,
                        &pImmediateContext);

    VertexMemoryGPU   = MakeRef<AVertexMemoryGPU>(RenderDevice);
    StreamedMemoryGPU = MakeRef<AStreamedMemoryGPU>(RenderDevice, pImmediateContext);

    RenderDevice->CreateSwapChain(WindowHandle, &pSwapChain);

    GPUSync = MakeRef<AGPUSync>(pImmediateContext);

    SetVideoMode(desiredMode);

    // Process initial events
    PollEvents();
}

ARuntime::~ARuntime()
{
    DestroyEngineInstance();
    Engine = nullptr;

    AsyncJobManager.Reset();

    GPUSync.Reset();

    VertexMemoryGPU.Reset();
    StreamedMemoryGPU.Reset();

    pSwapChain.Reset();

    pImmediateContext.Reset();

    TArray<const char*, RenderCore::DEVICE_OBJECT_TYPE_MAX> name =
        {
            "DEVICE_OBJECT_TYPE_UNKNOWN",

            "DEVICE_OBJECT_TYPE_IMMEDIATE_CONTEXT",

            "DEVICE_OBJECT_TYPE_BUFFER",
            "DEVICE_OBJECT_TYPE_BUFFER_VIEW",

            "DEVICE_OBJECT_TYPE_TEXTURE",
            "DEVICE_OBJECT_TYPE_TEXTURE_VIEW",

            "DEVICE_OBJECT_TYPE_SPARSE_TEXTURE",

            "DEVICE_OBJECT_TYPE_PIPELINE",
            "DEVICE_OBJECT_TYPE_SHADER_MODULE",
            "DEVICE_OBJECT_TYPE_TRANSFORM_FEEDBACK",
            "DEVICE_OBJECT_TYPE_QUERY_POOL",
            "DEVICE_OBJECT_TYPE_RESOURCE_TABLE",

            "DEVICE_OBJECT_TYPE_SWAP_CHAIN"};

    for (int i = 0; i < RenderCore::DEVICE_OBJECT_TYPE_MAX; i++)
    {
        GLogger.Printf("Object count %s: %d\n", name[i], RenderDevice->GetObjectCount((RenderCore::DEVICE_OBJECT_PROXY_TYPE)i));
    }
    RenderDevice.Reset();

    DestroyGenericWindow(WindowHandle);
    WindowHandle = nullptr;

    GLogger.Printf("TotalAllocatedRenderCore: %d\n", TotalAllocatedRenderCore);

    ARuntimeVariable::FreeVariables();
}

void ARuntime::LoadConfigFile()
{
    AString configFile = GetRootPath() + "config.cfg";

    AFileStream f;
    if (f.OpenRead(configFile))
    {
        AString data;

        data.FromFile(f);

        std::unique_ptr<ARuntimeCommandProcessor> cmdProcessor = std::make_unique<ARuntimeCommandProcessor>();

        cmdProcessor->Add(data.CStr());

        class CommandContext : public IRuntimeCommandContext
        {
        public:
            void ExecuteCommand(ARuntimeCommandProcessor const& _Proc) override
            {
                AN_ASSERT(_Proc.GetArgsCount() > 0);

                const char*       name = _Proc.GetArg(0);
                ARuntimeVariable* var;
                if (nullptr != (var = ARuntimeVariable::FindVariable(name)))
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

        cmdProcessor->Execute(context);
    }
}

static SDL_Window* CreateGenericWindow(SVideoMode const& VideoMode)
{
    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;

    bool bOpenGL = true;

    if (bOpenGL)
    {
        flags |= SDL_WINDOW_OPENGL;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                            SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
#ifdef AN_DEBUG
                                | SDL_GL_CONTEXT_DEBUG_FLAG
#endif
        );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_RELEASE_BEHAVIOR,  );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_RESET_NOTIFICATION,  );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_NO_ERROR,  );
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        //SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,  );
        //SDL_GL_SetAttribute( SDL_GL_RETAINED_BACKING,  );
    }

    int x, y;

    if (VideoMode.bFullscreen)
    {
        flags |= SDL_WINDOW_FULLSCREEN; // | SDL_WINDOW_BORDERLESS;
        x = y = 0;
    }
    else
    {
        if (VideoMode.bCentrized)
        {
            x = SDL_WINDOWPOS_CENTERED;
            y = SDL_WINDOWPOS_CENTERED;
        }
        else
        {
            x = VideoMode.WindowedX;
            y = VideoMode.WindowedY;
        }
    }

    return SDL_CreateWindow(VideoMode.Title, x, y, VideoMode.Width, VideoMode.Height, flags);
}

static void DestroyGenericWindow(SDL_Window* Window)
{
    if (Window)
    {
        SDL_DestroyWindow(Window);
    }
}

extern "C" const size_t   EmbeddedResources_Size;
extern "C" const uint64_t EmbeddedResources_Data[];

AArchive const& ARuntime::GetEmbeddedResources()
{
    if (!EmbeddedResourcesArch)
    {
        EmbeddedResourcesArch = MakeUnique<AArchive>();
        if (!EmbeddedResourcesArch->OpenFromMemory(EmbeddedResources_Data, EmbeddedResources_Size))
        {
            GLogger.Printf("Failed to open embedded resources\n");
        }
    }
    return *EmbeddedResourcesArch.GetObject();
}

void ARuntime::Run()
{
    Engine->Run(*pModuleDecl);
}

void ARuntime::InitializeWorkingDirectory()
{
    SProcessInfo const& processInfo = Platform::GetProcessInfo();

    WorkingDir = processInfo.Executable;
    WorkingDir.ClipFilename();

#if defined AN_OS_WIN32
    SetCurrentDirectoryA(WorkingDir.CStr());
#elif defined AN_OS_LINUX
    chdir(WorkingDir.CStr());
#else
#    error "InitializeWorkingDirectory not implemented under current platform"
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
}

#ifdef AN_OS_WIN32
void RunEngine(SEntryDecl const& EntryDecl)
#else
void RunEngine(int _Argc, char** _Argv, SEntryDecl const& EntryDecl)
#endif
{
    static bool bApplicationRun = false;

    if (bApplicationRun)
    {
        AN_ASSERT(0);
        return;
    }

    bApplicationRun = true;

    SCoreInitialize init;
#ifdef AN_OS_WIN32
    init.pCommandLine = ::GetCommandLineA();
#else
    init.Argc = _Argc;
    init.Argv = _Argv;
#endif
    init.bAllowMultipleInstances = false;
    init.ZoneSizeInMegabytes     = 256;
    init.HunkSizeInMegabytes     = 32;
    Platform::Initialize(init);
    {
        ARuntime runtime(EntryDecl);
        runtime.Run();
    }
    Platform::Deinitialize();
}

#if defined(AN_DEBUG) && defined(AN_COMPILER_MSVC)
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    return TRUE;
}
#endif

AString const& ARuntime::GetWorkingDir()
{
    return WorkingDir;
}

AString const& ARuntime::GetRootPath()
{
    return RootPath;
}

const char* ARuntime::GetExecutableName()
{
    SProcessInfo const& processInfo = Platform::GetProcessInfo();
    return processInfo.Executable ? processInfo.Executable : "";
}

void* ARuntime::AllocFrameMem(size_t _SizeInBytes)
{
    return FrameMemory.Allocate(_SizeInBytes);
}

size_t ARuntime::GetFrameMemorySize() const
{
    return FrameMemory.GetBlockMemoryUsage();
}

size_t ARuntime::GetFrameMemoryUsed() const
{
    return FrameMemory.GetTotalMemoryUsage();
}

size_t ARuntime::GetFrameMemoryUsedPrev() const
{
    return FrameMemoryUsedPrev;
}

size_t ARuntime::GetMaxFrameMemoryUsage() const
{
    return MaxFrameMemoryUsage;
}

int64_t ARuntime::SysFrameTimeStamp()
{
    return FrameTimeStamp;
}

int64_t ARuntime::SysFrameDuration()
{
    return FrameDuration;
}

int ARuntime::SysFrameNumber() const
{
    return FrameNumber;
}

SVideoMode const& ARuntime::GetVideoMode() const
{
    return VideoMode;
}

void ARuntime::PostChangeVideoMode(SVideoMode const& _DesiredMode)
{
    DesiredMode          = _DesiredMode;
    bPostChangeVideoMode = true;
}

void ARuntime::PostTerminateEvent()
{
    bPostTerminateEvent = true;
}

bool ARuntime::IsPendingTerminate()
{
    return bPostTerminateEvent;
}

#define FROM_SDL_TIMESTAMP(event) ((event).timestamp * 0.001)

struct SKeyMappingsSDL : public TArray<unsigned short, SDL_NUM_SCANCODES>
{
    SKeyMappingsSDL()
    {
        SKeyMappingsSDL& self = *this;

        Platform::ZeroMem(ToPtr(), sizeof(*this));

        self[SDL_SCANCODE_A]            = KEY_A;
        self[SDL_SCANCODE_B]            = KEY_B;
        self[SDL_SCANCODE_C]            = KEY_C;
        self[SDL_SCANCODE_D]            = KEY_D;
        self[SDL_SCANCODE_E]            = KEY_E;
        self[SDL_SCANCODE_F]            = KEY_F;
        self[SDL_SCANCODE_G]            = KEY_G;
        self[SDL_SCANCODE_H]            = KEY_H;
        self[SDL_SCANCODE_I]            = KEY_I;
        self[SDL_SCANCODE_J]            = KEY_J;
        self[SDL_SCANCODE_K]            = KEY_K;
        self[SDL_SCANCODE_L]            = KEY_L;
        self[SDL_SCANCODE_M]            = KEY_M;
        self[SDL_SCANCODE_N]            = KEY_N;
        self[SDL_SCANCODE_O]            = KEY_O;
        self[SDL_SCANCODE_P]            = KEY_P;
        self[SDL_SCANCODE_Q]            = KEY_Q;
        self[SDL_SCANCODE_R]            = KEY_R;
        self[SDL_SCANCODE_S]            = KEY_S;
        self[SDL_SCANCODE_T]            = KEY_T;
        self[SDL_SCANCODE_U]            = KEY_U;
        self[SDL_SCANCODE_V]            = KEY_V;
        self[SDL_SCANCODE_W]            = KEY_W;
        self[SDL_SCANCODE_X]            = KEY_X;
        self[SDL_SCANCODE_Y]            = KEY_Y;
        self[SDL_SCANCODE_Z]            = KEY_Z;
        self[SDL_SCANCODE_1]            = KEY_1;
        self[SDL_SCANCODE_2]            = KEY_2;
        self[SDL_SCANCODE_3]            = KEY_3;
        self[SDL_SCANCODE_4]            = KEY_4;
        self[SDL_SCANCODE_5]            = KEY_5;
        self[SDL_SCANCODE_6]            = KEY_6;
        self[SDL_SCANCODE_7]            = KEY_7;
        self[SDL_SCANCODE_8]            = KEY_8;
        self[SDL_SCANCODE_9]            = KEY_9;
        self[SDL_SCANCODE_0]            = KEY_0;
        self[SDL_SCANCODE_RETURN]       = KEY_ENTER;
        self[SDL_SCANCODE_ESCAPE]       = KEY_ESCAPE;
        self[SDL_SCANCODE_BACKSPACE]    = KEY_BACKSPACE;
        self[SDL_SCANCODE_TAB]          = KEY_TAB;
        self[SDL_SCANCODE_SPACE]        = KEY_SPACE;
        self[SDL_SCANCODE_MINUS]        = KEY_MINUS;
        self[SDL_SCANCODE_EQUALS]       = KEY_EQUAL;
        self[SDL_SCANCODE_LEFTBRACKET]  = KEY_LEFT_BRACKET;
        self[SDL_SCANCODE_RIGHTBRACKET] = KEY_RIGHT_BRACKET;
        self[SDL_SCANCODE_BACKSLASH]    = KEY_BACKSLASH;
        self[SDL_SCANCODE_SEMICOLON]    = KEY_SEMICOLON;
        self[SDL_SCANCODE_APOSTROPHE]   = KEY_APOSTROPHE;
        self[SDL_SCANCODE_GRAVE]        = KEY_GRAVE_ACCENT;
        self[SDL_SCANCODE_COMMA]        = KEY_COMMA;
        self[SDL_SCANCODE_PERIOD]       = KEY_PERIOD;
        self[SDL_SCANCODE_SLASH]        = KEY_SLASH;
        self[SDL_SCANCODE_CAPSLOCK]     = KEY_CAPS_LOCK;
        self[SDL_SCANCODE_F1]           = KEY_F1;
        self[SDL_SCANCODE_F2]           = KEY_F2;
        self[SDL_SCANCODE_F3]           = KEY_F3;
        self[SDL_SCANCODE_F4]           = KEY_F4;
        self[SDL_SCANCODE_F5]           = KEY_F5;
        self[SDL_SCANCODE_F6]           = KEY_F6;
        self[SDL_SCANCODE_F7]           = KEY_F7;
        self[SDL_SCANCODE_F8]           = KEY_F8;
        self[SDL_SCANCODE_F9]           = KEY_F9;
        self[SDL_SCANCODE_F10]          = KEY_F10;
        self[SDL_SCANCODE_F11]          = KEY_F11;
        self[SDL_SCANCODE_F12]          = KEY_F12;
        self[SDL_SCANCODE_PRINTSCREEN]  = KEY_PRINT_SCREEN;
        self[SDL_SCANCODE_SCROLLLOCK]   = KEY_SCROLL_LOCK;
        self[SDL_SCANCODE_PAUSE]        = KEY_PAUSE;
        self[SDL_SCANCODE_INSERT]       = KEY_INSERT;
        self[SDL_SCANCODE_HOME]         = KEY_HOME;
        self[SDL_SCANCODE_PAGEUP]       = KEY_PAGE_UP;
        self[SDL_SCANCODE_DELETE]       = KEY_DELETE;
        self[SDL_SCANCODE_END]          = KEY_END;
        self[SDL_SCANCODE_PAGEDOWN]     = KEY_PAGE_DOWN;
        self[SDL_SCANCODE_RIGHT]        = KEY_RIGHT;
        self[SDL_SCANCODE_LEFT]         = KEY_LEFT;
        self[SDL_SCANCODE_DOWN]         = KEY_DOWN;
        self[SDL_SCANCODE_UP]           = KEY_UP;
        self[SDL_SCANCODE_NUMLOCKCLEAR] = KEY_NUM_LOCK;
        self[SDL_SCANCODE_KP_DIVIDE]    = KEY_KP_DIVIDE;
        self[SDL_SCANCODE_KP_MULTIPLY]  = KEY_KP_MULTIPLY;
        self[SDL_SCANCODE_KP_MINUS]     = KEY_KP_SUBTRACT;
        self[SDL_SCANCODE_KP_PLUS]      = KEY_KP_ADD;
        self[SDL_SCANCODE_KP_ENTER]     = KEY_KP_ENTER;
        self[SDL_SCANCODE_KP_1]         = KEY_KP_1;
        self[SDL_SCANCODE_KP_2]         = KEY_KP_2;
        self[SDL_SCANCODE_KP_3]         = KEY_KP_3;
        self[SDL_SCANCODE_KP_4]         = KEY_KP_4;
        self[SDL_SCANCODE_KP_5]         = KEY_KP_5;
        self[SDL_SCANCODE_KP_6]         = KEY_KP_6;
        self[SDL_SCANCODE_KP_7]         = KEY_KP_7;
        self[SDL_SCANCODE_KP_8]         = KEY_KP_8;
        self[SDL_SCANCODE_KP_9]         = KEY_KP_9;
        self[SDL_SCANCODE_KP_0]         = KEY_KP_0;
        self[SDL_SCANCODE_KP_PERIOD]    = KEY_KP_DECIMAL;
        self[SDL_SCANCODE_KP_EQUALS]    = KEY_KP_EQUAL;
        self[SDL_SCANCODE_F13]          = KEY_F13;
        self[SDL_SCANCODE_F14]          = KEY_F14;
        self[SDL_SCANCODE_F15]          = KEY_F15;
        self[SDL_SCANCODE_F16]          = KEY_F16;
        self[SDL_SCANCODE_F17]          = KEY_F17;
        self[SDL_SCANCODE_F18]          = KEY_F18;
        self[SDL_SCANCODE_F19]          = KEY_F19;
        self[SDL_SCANCODE_F20]          = KEY_F20;
        self[SDL_SCANCODE_F21]          = KEY_F21;
        self[SDL_SCANCODE_F22]          = KEY_F22;
        self[SDL_SCANCODE_F23]          = KEY_F23;
        self[SDL_SCANCODE_F24]          = KEY_F24;
        self[SDL_SCANCODE_MENU]         = KEY_MENU;
        self[SDL_SCANCODE_LCTRL]        = KEY_LEFT_CONTROL;
        self[SDL_SCANCODE_LSHIFT]       = KEY_LEFT_SHIFT;
        self[SDL_SCANCODE_LALT]         = KEY_LEFT_ALT;
        self[SDL_SCANCODE_LGUI]         = KEY_LEFT_SUPER;
        self[SDL_SCANCODE_RCTRL]        = KEY_RIGHT_CONTROL;
        self[SDL_SCANCODE_RSHIFT]       = KEY_RIGHT_SHIFT;
        self[SDL_SCANCODE_RALT]         = KEY_RIGHT_ALT;
        self[SDL_SCANCODE_RGUI]         = KEY_RIGHT_SUPER;
    }
};

static const SKeyMappingsSDL SDLKeyMappings;

static AN_FORCEINLINE int FromKeymodSDL(Uint16 Mod)
{
    int modMask = 0;

    if (Mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        modMask |= KMOD_MASK_SHIFT;
    }

    if (Mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        modMask |= KMOD_MASK_CONTROL;
    }

    if (Mod & (KMOD_LALT | KMOD_RALT))
    {
        modMask |= KMOD_MASK_ALT;
    }

    if (Mod & (KMOD_LGUI | KMOD_RGUI))
    {
        modMask |= KMOD_MASK_SUPER;
    }

    return modMask;
}

static AN_FORCEINLINE int FromKeymodSDL_Char(Uint16 Mod)
{
    int modMask = 0;

    if (Mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        modMask |= KMOD_MASK_SHIFT;
    }

    if (Mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        modMask |= KMOD_MASK_CONTROL;
    }

    if (Mod & (KMOD_LALT | KMOD_RALT))
    {
        modMask |= KMOD_MASK_ALT;
    }

    if (Mod & (KMOD_LGUI | KMOD_RGUI))
    {
        modMask |= KMOD_MASK_SUPER;
    }

    if (Mod & KMOD_CAPS)
    {
        modMask |= KMOD_MASK_CAPS_LOCK;
    }

    if (Mod & KMOD_NUM)
    {
        modMask |= KMOD_MASK_NUM_LOCK;
    }

    return modMask;
}

void ARuntime::UnpressJoystickButtons(int _JoystickNum, double _TimeStamp)
{
    SJoystickButtonEvent buttonEvent;
    buttonEvent.Joystick = _JoystickNum;
    buttonEvent.Action   = IA_RELEASE;
    for (int i = 0; i < MAX_JOYSTICK_BUTTONS; i++)
    {
        if (JoystickButtonState[_JoystickNum][i])
        {
            JoystickButtonState[_JoystickNum][i] = SDL_RELEASED;
            buttonEvent.Button                   = JOY_BUTTON_1 + i;
            Engine->OnJoystickButtonEvent(buttonEvent, _TimeStamp);
        }
    }
}

void ARuntime::ClearJoystickAxes(int _JoystickNum, double _TimeStamp)
{
    SJoystickAxisEvent axisEvent;
    axisEvent.Joystick = _JoystickNum;
    axisEvent.Value    = 0;
    for (int i = 0; i < MAX_JOYSTICK_AXES; i++)
    {
        if (JoystickAxisState[_JoystickNum][i] != 0)
        {
            JoystickAxisState[_JoystickNum][i] = 0;
            axisEvent.Axis                     = JOY_AXIS_1 + i;
            Engine->OnJoystickAxisEvent(axisEvent, _TimeStamp);
        }
    }
}

void ARuntime::UnpressKeysAndButtons()
{
    SKeyEvent         keyEvent;
    SMouseButtonEvent mouseEvent;

    keyEvent.Action  = IA_RELEASE;
    keyEvent.ModMask = 0;

    mouseEvent.Action  = IA_RELEASE;
    mouseEvent.ModMask = 0;

    double timeStamp = Platform::SysSeconds_d();

    for (int i = 0; i <= KEY_LAST; i++)
    {
        if (PressedKeys[i])
        {
            keyEvent.Key      = i;
            keyEvent.Scancode = PressedKeys[i] - 1;

            PressedKeys[i] = 0;

            Engine->OnKeyEvent(keyEvent, timeStamp);
        }
    }
    for (int i = MOUSE_BUTTON_1; i <= MOUSE_BUTTON_8; i++)
    {
        if (PressedMouseButtons[i])
        {
            mouseEvent.Button = i;

            PressedMouseButtons[i] = 0;

            Engine->OnMouseButtonEvent(mouseEvent, timeStamp);
        }
    }

    for (int i = 0; i < MAX_JOYSTICKS_COUNT; i++)
    {
        UnpressJoystickButtons(i, timeStamp);
        ClearJoystickAxes(i, timeStamp);
    }
}

void ARuntime::NewFrame()
{
    GPUSync->SetEvent();

    // Swap buffers for streamed memory
    StreamedMemoryGPU->Swap();

    // Swap window
    pSwapChain->Present(rt_SwapInterval.GetInteger());

    // Wait for free streamed buffer
    StreamedMemoryGPU->Wait();

    int64_t prevTimeStamp = FrameTimeStamp;
    FrameTimeStamp        = Platform::SysMicroseconds();
    if (prevTimeStamp == Platform::SysStartMicroseconds())
    {
        // First frame
        FrameDuration = 1000000.0 / 60;
    }
    else
    {
        FrameDuration = FrameTimeStamp - prevTimeStamp;
    }

    FrameNumber++;

    // Keep memory statistics
    MaxFrameMemoryUsage = Math::Max(MaxFrameMemoryUsage, FrameMemory.GetTotalMemoryUsage());
    FrameMemoryUsedPrev = FrameMemory.GetTotalMemoryUsage();

    // Free frame memory for new frame
    FrameMemory.Reset();

    if (bPostChangeVideoMode)
    {
        bPostChangeVideoMode = false;
        SetVideoMode(DesiredMode);
    }
}

void ARuntime::PollEvents()
{
    // NOTE: Workaround of SDL bug with false mouse motion when a window gain keyboard focus.
    static bool bIgnoreFalseMouseMotionHack = false;

    // Sync with GPU to prevent "input lag"
    if (rt_SyncGPU)
    {
        GPUSync->Wait();
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {

            // User-requested quit
            case SDL_QUIT:
                Engine->OnCloseEvent();
                break;

            // The application is being terminated by the OS
            // Called on iOS in applicationWillTerminate()
            // Called on Android in onDestroy()
            case SDL_APP_TERMINATING:
                GLogger.Printf("PollEvent: Terminating\n");
                break;

            // The application is low on memory, free memory if possible.
            // Called on iOS in applicationDidReceiveMemoryWarning()
            // Called on Android in onLowMemory()
            case SDL_APP_LOWMEMORY:
                GLogger.Printf("PollEvent: Low memory\n");
                break;

            // The application is about to enter the background
            // Called on iOS in applicationWillResignActive()
            // Called on Android in onPause()
            case SDL_APP_WILLENTERBACKGROUND:
                GLogger.Printf("PollEvent: Will enter background\n");
                break;

            // The application did enter the background and may not get CPU for some time
            // Called on iOS in applicationDidEnterBackground()
            // Called on Android in onPause()
            case SDL_APP_DIDENTERBACKGROUND:
                GLogger.Printf("PollEvent: Did enter background\n");
                break;

            // The application is about to enter the foreground
            // Called on iOS in applicationWillEnterForeground()
            // Called on Android in onResume()
            case SDL_APP_WILLENTERFOREGROUND:
                GLogger.Printf("PollEvent: Will enter foreground\n");
                break;

            // The application is now interactive
            // Called on iOS in applicationDidBecomeActive()
            // Called on Android in onResume()
            case SDL_APP_DIDENTERFOREGROUND:
                GLogger.Printf("PollEvent: Did enter foreground\n");
                break;

            // Display state change
            case SDL_DISPLAYEVENT:
                switch (event.display.event)
                {
                    // Display orientation has changed to data1
                    case SDL_DISPLAYEVENT_ORIENTATION:
                        switch (event.display.data1)
                        {
                            // The display is in landscape mode, with the right side up, relative to portrait mode
                            case SDL_ORIENTATION_LANDSCAPE:
                                GLogger.Printf("PollEvent: Display orientation has changed to landscape mode\n");
                                break;
                            // The display is in landscape mode, with the left side up, relative to portrait mode
                            case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
                                GLogger.Printf("PollEvent: Display orientation has changed to flipped landscape mode\n");
                                break;
                            // The display is in portrait mode
                            case SDL_ORIENTATION_PORTRAIT:
                                GLogger.Printf("PollEvent: Display orientation has changed to portrait mode\n");
                                break;
                            // The display is in portrait mode, upside down
                            case SDL_ORIENTATION_PORTRAIT_FLIPPED:
                                GLogger.Printf("PollEvent: Display orientation has changed to flipped portrait mode\n");
                                break;
                            // The display orientation can't be determined
                            case SDL_ORIENTATION_UNKNOWN:
                            default:
                                GLogger.Printf("PollEvent: The display orientation can't be determined\n");
                                break;
                        }
                    default:
                        GLogger.Printf("PollEvent: Unknown display event type\n");
                        break;
                }
                break;

            // Window state change
            case SDL_WINDOWEVENT: {
                switch (event.window.event)
                {
                    // Window has been shown
                    case SDL_WINDOWEVENT_SHOWN:
                        Engine->OnWindowVisible(true);
                        break;
                    // Window has been hidden
                    case SDL_WINDOWEVENT_HIDDEN:
                        Engine->OnWindowVisible(false);
                        break;
                    // Window has been exposed and should be redrawn
                    case SDL_WINDOWEVENT_EXPOSED:
                        break;
                    // Window has been moved to data1, data2
                    case SDL_WINDOWEVENT_MOVED:
                        VideoMode.DisplayId = SDL_GetWindowDisplayIndex(SDL_GetWindowFromID(event.window.windowID));
                        VideoMode.X         = event.window.data1;
                        VideoMode.Y         = event.window.data2;
                        if (!VideoMode.bFullscreen)
                        {
                            VideoMode.WindowedX = event.window.data1;
                            VideoMode.WindowedY = event.window.data2;
                        }
                        break;
                    // Window has been resized to data1xdata2
                    case SDL_WINDOWEVENT_RESIZED:
                    // The window size has changed, either as
                    // a result of an API call or through the
                    // system or user changing the window size.
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        SDL_Window* wnd     = SDL_GetWindowFromID(event.window.windowID);
                        VideoMode.Width     = event.window.data1;
                        VideoMode.Height    = event.window.data2;
                        VideoMode.DisplayId = SDL_GetWindowDisplayIndex(wnd);
                        SDL_GL_GetDrawableSize(wnd, &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight);
                        if (VideoMode.bFullscreen)
                        {
                            SDL_DisplayMode mode;
                            SDL_GetDesktopDisplayMode(VideoMode.DisplayId, &mode);
                            float sx              = (float)mode.w / VideoMode.FramebufferWidth;
                            float sy              = (float)mode.h / VideoMode.FramebufferHeight;
                            VideoMode.AspectScale = (sx / sy);
                        }
                        else
                        {
                            VideoMode.AspectScale = 1;
                        }
                        pSwapChain->Resize(VideoMode.FramebufferWidth, VideoMode.FramebufferHeight);
                        Engine->OnResize();
                        break;
                    }
                    // Window has been minimized
                    case SDL_WINDOWEVENT_MINIMIZED:
                        Engine->OnWindowVisible(false);
                        break;
                    // Window has been maximized
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        break;
                    // Window has been restored to normal size and position
                    case SDL_WINDOWEVENT_RESTORED:
                        Engine->OnWindowVisible(true);
                        break;
                    // Window has gained mouse focus
                    case SDL_WINDOWEVENT_ENTER:
                        break;
                    // Window has lost mouse focus
                    case SDL_WINDOWEVENT_LEAVE:
                        break;
                    // Window has gained keyboard focus
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        bIgnoreFalseMouseMotionHack = true;
                        break;
                    // Window has lost keyboard focus
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        UnpressKeysAndButtons();
                        break;
                    // The window manager requests that the window be closed
                    case SDL_WINDOWEVENT_CLOSE:
                        break;
                    // Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore)
                    case SDL_WINDOWEVENT_TAKE_FOCUS:
                        break;
                    // Window had a hit test that wasn't SDL_HITTEST_NORMAL.
                    case SDL_WINDOWEVENT_HIT_TEST:
                        break;
                }
                break;
            }

            // System specific event
            case SDL_SYSWMEVENT:
                break;

            // Key pressed/released
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                SKeyEvent keyEvent;
                keyEvent.Key      = SDLKeyMappings[event.key.keysym.scancode];
                keyEvent.Scancode = event.key.keysym.scancode;
                keyEvent.Action   = (event.type == SDL_KEYDOWN) ? (PressedKeys[keyEvent.Key] ? IA_REPEAT : IA_PRESS) : IA_RELEASE;
                keyEvent.ModMask  = FromKeymodSDL(event.key.keysym.mod);
                if (keyEvent.Key)
                {
                    if ((keyEvent.Action == IA_RELEASE && !PressedKeys[keyEvent.Key]) || (keyEvent.Action == IA_PRESS && PressedKeys[keyEvent.Key]))
                    {

                        // State does not changed
                    }
                    else
                    {
                        PressedKeys[keyEvent.Key] = (keyEvent.Action == IA_RELEASE) ? 0 : keyEvent.Scancode + 1;

                        Engine->OnKeyEvent(keyEvent, FROM_SDL_TIMESTAMP(event.key));
                    }
                }
                break;
            }

            // Keyboard text editing (composition)
            case SDL_TEXTEDITING:
                break;

            // Keyboard text input
            case SDL_TEXTINPUT: {
                SCharEvent charEvent;
                charEvent.ModMask = FromKeymodSDL_Char(SDL_GetModState());

                const char* unicode = event.text.text;
                while (*unicode)
                {
                    const int byteLen = Core::WideCharDecodeUTF8(unicode, charEvent.UnicodeCharacter);
                    if (!byteLen)
                    {
                        break;
                    }
                    unicode += byteLen;
                    Engine->OnCharEvent(charEvent, FROM_SDL_TIMESTAMP(event.text));
                }
                break;
            }

            // Keymap changed due to a system event such as an
            // input language or keyboard layout change.
            case SDL_KEYMAPCHANGED:
                break;

            // Mouse moved
            case SDL_MOUSEMOTION: {
                if (!bIgnoreFalseMouseMotionHack)
                {
                    SMouseMoveEvent moveEvent;
                    moveEvent.X = event.motion.xrel;
                    moveEvent.Y = -event.motion.yrel;
                    Engine->OnMouseMoveEvent(moveEvent, FROM_SDL_TIMESTAMP(event.motion));
                }
                bIgnoreFalseMouseMotionHack = false;
                break;
            }

            // Mouse button pressed
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                SMouseButtonEvent mouseEvent;
                switch (event.button.button)
                {
                    case 2:
                        mouseEvent.Button = MOUSE_BUTTON_3;
                        break;
                    case 3:
                        mouseEvent.Button = MOUSE_BUTTON_2;
                        break;
                    default:
                        mouseEvent.Button = MOUSE_BUTTON_1 + event.button.button - 1;
                        break;
                }
                mouseEvent.Action  = event.type == SDL_MOUSEBUTTONDOWN ? IA_PRESS : IA_RELEASE;
                mouseEvent.ModMask = FromKeymodSDL(SDL_GetModState());

                if (mouseEvent.Button >= MOUSE_BUTTON_1 && mouseEvent.Button <= MOUSE_BUTTON_8)
                {
                    if (mouseEvent.Action == (int)PressedMouseButtons[mouseEvent.Button])
                    {

                        // State does not changed
                    }
                    else
                    {
                        PressedMouseButtons[mouseEvent.Button] = mouseEvent.Action != IA_RELEASE;

                        Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.button));
                    }
                }
                break;
            }

            // Mouse wheel motion
            case SDL_MOUSEWHEEL: {
                SMouseWheelEvent wheelEvent;
                wheelEvent.WheelX = event.wheel.x;
                wheelEvent.WheelY = event.wheel.y;
                Engine->OnMouseWheelEvent(wheelEvent, FROM_SDL_TIMESTAMP(event.wheel));

                SMouseButtonEvent mouseEvent;
                mouseEvent.ModMask = FromKeymodSDL(SDL_GetModState());

                if (wheelEvent.WheelX < 0.0)
                {
                    mouseEvent.Button = MOUSE_WHEEL_LEFT;

                    mouseEvent.Action = IA_PRESS;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));

                    mouseEvent.Action = IA_RELEASE;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));
                }
                else if (wheelEvent.WheelX > 0.0)
                {
                    mouseEvent.Button = MOUSE_WHEEL_RIGHT;

                    mouseEvent.Action = IA_PRESS;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));

                    mouseEvent.Action = IA_RELEASE;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));
                }
                if (wheelEvent.WheelY < 0.0)
                {
                    mouseEvent.Button = MOUSE_WHEEL_DOWN;

                    mouseEvent.Action = IA_PRESS;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));

                    mouseEvent.Action = IA_RELEASE;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));
                }
                else if (wheelEvent.WheelY > 0.0)
                {
                    mouseEvent.Button = MOUSE_WHEEL_UP;

                    mouseEvent.Action = IA_PRESS;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));

                    mouseEvent.Action = IA_RELEASE;
                    Engine->OnMouseButtonEvent(mouseEvent, FROM_SDL_TIMESTAMP(event.wheel));
                }
                break;
            }

            // Joystick axis motion
            case SDL_JOYAXISMOTION: {
                if (event.jaxis.which >= 0 && event.jaxis.which < MAX_JOYSTICKS_COUNT)
                {
                    AN_ASSERT(JoystickAdded[event.jaxis.which]);
                    if (event.jaxis.axis >= 0 && event.jaxis.axis < MAX_JOYSTICK_AXES)
                    {
                        if (JoystickAxisState[event.jaxis.which][event.jaxis.axis] != event.jaxis.value)
                        {
                            SJoystickAxisEvent axisEvent;
                            axisEvent.Joystick = event.jaxis.which;
                            axisEvent.Axis     = JOY_AXIS_1 + event.jaxis.axis;
                            axisEvent.Value    = ((float)event.jaxis.value + 32768.0f) / 0xffff * 2.0f - 1.0f; // scale to -1.0f ... 1.0f

                            Engine->OnJoystickAxisEvent(axisEvent, FROM_SDL_TIMESTAMP(event.jaxis));
                        }
                    }
                    else
                    {
                        AN_ASSERT_(0, "Invalid joystick axis num");
                    }
                }
                else
                {
                    AN_ASSERT_(0, "Invalid joystick id");
                }
                break;
            }

            // Joystick trackball motion
            case SDL_JOYBALLMOTION:
                GLogger.Printf("PollEvent: Joystick ball move\n");
                break;

            // Joystick hat position change
            case SDL_JOYHATMOTION:
                GLogger.Printf("PollEvent: Joystick hat move\n");
                break;

            // Joystick button pressed
            case SDL_JOYBUTTONDOWN:
            // Joystick button released
            case SDL_JOYBUTTONUP: {
                if (event.jbutton.which >= 0 && event.jbutton.which < MAX_JOYSTICKS_COUNT)
                {
                    AN_ASSERT(JoystickAdded[event.jbutton.which]);
                    if (event.jbutton.button >= 0 && event.jbutton.button < MAX_JOYSTICK_BUTTONS)
                    {
                        if (JoystickButtonState[event.jbutton.which][event.jbutton.button] != event.jbutton.state)
                        {
                            JoystickButtonState[event.jbutton.which][event.jbutton.button] = event.jbutton.state;
                            SJoystickButtonEvent buttonEvent;
                            buttonEvent.Joystick = event.jbutton.which;
                            buttonEvent.Button   = JOY_BUTTON_1 + event.jbutton.button;
                            buttonEvent.Action   = event.jbutton.state == SDL_PRESSED ? IA_PRESS : IA_RELEASE;
                            Engine->OnJoystickButtonEvent(buttonEvent, FROM_SDL_TIMESTAMP(event.jbutton));
                        }
                    }
                    else
                    {
                        AN_ASSERT_(0, "Invalid joystick button num");
                    }
                }
                else
                {
                    AN_ASSERT_(0, "Invalid joystick id");
                }
                break;
            }

            // A new joystick has been inserted into the system
            case SDL_JOYDEVICEADDED:
                if (event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT)
                {
                    AN_ASSERT(!JoystickAdded[event.jdevice.which]);
                    JoystickAdded[event.jdevice.which] = true;

                    Platform::ZeroMem(JoystickButtonState[event.jdevice.which].ToPtr(), sizeof(JoystickButtonState[0]));
                    Platform::ZeroMem(JoystickAxisState[event.jdevice.which].ToPtr(), sizeof(JoystickAxisState[0]));
                }
                else
                {
                    AN_ASSERT_(0, "Invalid joystick id");
                }
                GLogger.Printf("PollEvent: Joystick added\n");
                break;

            // An opened joystick has been removed
            case SDL_JOYDEVICEREMOVED: {
                if (event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT)
                {
                    double timeStamp = FROM_SDL_TIMESTAMP(event.jdevice);

                    UnpressJoystickButtons(event.jdevice.which, timeStamp);
                    ClearJoystickAxes(event.jdevice.which, timeStamp);

                    AN_ASSERT(JoystickAdded[event.jdevice.which]);
                    JoystickAdded[event.jdevice.which] = false;
                }
                else
                {
                    AN_ASSERT_(0, "Invalid joystick id");
                }

                GLogger.Printf("PollEvent: Joystick removed\n");
                break;
            }

            // Game controller axis motion
            case SDL_CONTROLLERAXISMOTION:
                GLogger.Printf("PollEvent: Gamepad axis move\n");
                break;
            // Game controller button pressed
            case SDL_CONTROLLERBUTTONDOWN:
                GLogger.Printf("PollEvent: Gamepad button press\n");
                break;
            // Game controller button released
            case SDL_CONTROLLERBUTTONUP:
                GLogger.Printf("PollEvent: Gamepad button release\n");
                break;
            // A new Game controller has been inserted into the system
            case SDL_CONTROLLERDEVICEADDED:
                GLogger.Printf("PollEvent: Gamepad added\n");
                break;
            // An opened Game controller has been removed
            case SDL_CONTROLLERDEVICEREMOVED:
                GLogger.Printf("PollEvent: Gamepad removed\n");
                break;
            // The controller mapping was updated
            case SDL_CONTROLLERDEVICEREMAPPED:
                GLogger.Printf("PollEvent: Gamepad device mapped\n");
                break;

            // Touch events
            case SDL_FINGERDOWN:
                GLogger.Printf("PollEvent: Touch press\n");
                break;
            case SDL_FINGERUP:
                GLogger.Printf("PollEvent: Touch release\n");
                break;
            case SDL_FINGERMOTION:
                GLogger.Printf("PollEvent: Touch move\n");
                break;

            // Gesture events
            case SDL_DOLLARGESTURE:
                GLogger.Printf("PollEvent: Dollar gesture\n");
                break;
            case SDL_DOLLARRECORD:
                GLogger.Printf("PollEvent: Dollar record\n");
                break;
            case SDL_MULTIGESTURE:
                GLogger.Printf("PollEvent: Multigesture\n");
                break;

            // The clipboard changed
            case SDL_CLIPBOARDUPDATE:
                GLogger.Printf("PollEvent: Clipboard update\n");
                break;

            // The system requests a file open
            case SDL_DROPFILE:
                GLogger.Printf("PollEvent: Drop file\n");
                break;
            // text/plain drag-and-drop event
            case SDL_DROPTEXT:
                GLogger.Printf("PollEvent: Drop text\n");
                break;
            // A new set of drops is beginning (NULL filename)
            case SDL_DROPBEGIN:
                GLogger.Printf("PollEvent: Drop begin\n");
                break;
            // Current set of drops is now complete (NULL filename)
            case SDL_DROPCOMPLETE:
                GLogger.Printf("PollEvent: Drop complete\n");
                break;

            // A new audio device is available
            case SDL_AUDIODEVICEADDED:
                GLogger.Printf("Audio %s device added: %s\n", event.adevice.iscapture ? "capture" : "playback", SDL_GetAudioDeviceName(event.adevice.which, event.adevice.iscapture));
                break;
            // An audio device has been removed
            case SDL_AUDIODEVICEREMOVED:
                GLogger.Printf("Audio %s device removed: %s\n", event.adevice.iscapture ? "capture" : "playback", SDL_GetAudioDeviceName(event.adevice.which, event.adevice.iscapture));
                break;

            // A sensor was updated
            case SDL_SENSORUPDATE:
                GLogger.Printf("PollEvent: Sensor update\n");
                break;

            // The render targets have been reset and their contents need to be updated
            case SDL_RENDER_TARGETS_RESET:
                GLogger.Printf("PollEvent: Render targets reset\n");
                break;
            case SDL_RENDER_DEVICE_RESET:
                GLogger.Printf("PollEvent: Render device reset\n");
                break;
        }
    }
}

void ARuntime::SetVideoMode(SVideoMode const& _DesiredMode)
{
    Platform::Memcpy(&VideoMode, &_DesiredMode, sizeof(VideoMode));

    SDL_Window* wnd = WindowHandle;

    SDL_SetWindowFullscreen(wnd, _DesiredMode.bFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    SDL_SetWindowSize(wnd, _DesiredMode.Width, _DesiredMode.Height);
    if (!_DesiredMode.bFullscreen)
    {
        if (_DesiredMode.bCentrized)
        {
            SDL_SetWindowPosition(wnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
        else
        {
            SDL_SetWindowPosition(wnd, _DesiredMode.WindowedX, _DesiredMode.WindowedY);
        }
    }
    //SDL_SetWindowDisplayMode( wnd, &mode );
    SDL_GL_GetDrawableSize(wnd, &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight);

    VideoMode.bFullscreen = !!(SDL_GetWindowFlags(wnd) & SDL_WINDOW_FULLSCREEN);

    VideoMode.Opacity = Math::Clamp(VideoMode.Opacity, 0.0f, 1.0f);

    float opacity = 1.0f;
    SDL_GetWindowOpacity(wnd, &opacity);
    if (Math::Abs(VideoMode.Opacity - opacity) > 1.0f / 255.0f)
    {
        SDL_SetWindowOpacity(wnd, VideoMode.Opacity);
    }

    VideoMode.DisplayId = SDL_GetWindowDisplayIndex(wnd);
    /*if ( VideoMode.bFullscreen ) {
        const float MM_To_Inch = 0.0393701f;
        VideoMode.DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        VideoMode.DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);

    } else */
    {
        SDL_GetDisplayDPI(VideoMode.DisplayId, NULL, &VideoMode.DPI_X, &VideoMode.DPI_Y);
    }

    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(wnd, &mode);
    VideoMode.RefreshRate = mode.refresh_rate;

    // Swap buffers to prevent flickering
    pSwapChain->Present(rt_SwapInterval.GetInteger());
}

void ARuntime::SetCursorEnabled(bool _Enabled)
{
    //SDL_SetWindowGrab( Wnd, (SDL_bool)!_Enabled ); // FIXME: Always grab in fullscreen?
    SDL_SetRelativeMouseMode((SDL_bool)!_Enabled);
}

bool ARuntime::IsCursorEnabled()
{
    return !SDL_GetRelativeMouseMode();
}

void ARuntime::GetCursorPosition(int* _X, int* _Y)
{
    (void)SDL_GetMouseState(_X, _Y);
}

RenderCore::IDevice* ARuntime::GetRenderDevice()
{
    return RenderDevice;
}

void ARuntime::ReadScreenPixels(uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, void* _SysMem)
{
    RenderCore::ITexture* pBackBuffer = pSwapChain->GetBackBuffer();

    RenderCore::STextureRect rect;
    rect.Offset.X    = _X;
    rect.Offset.Y    = _Y;
    rect.Dimension.X = _Width;
    rect.Dimension.Y = _Height;
    rect.Dimension.Z = 1;

    pImmediateContext->ReadTextureRect(pBackBuffer, rect, RenderCore::FORMAT_UBYTE4, _SizeInBytes, 4, _SysMem);

    // Swap to rgba
    int count = _Width * _Height * 4;
    for (int i = 0; i < count; i += 4)
    {
        std::swap(((uint8_t*)_SysMem)[i], ((uint8_t*)_SysMem)[i + 2]);
    }
}
