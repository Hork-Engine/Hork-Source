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

#include "FrameLoop.h"
#include <Engine/World/Modules/Render/WorldRenderView.h>
#include <Engine/World/Resources/Resource_Font.h>

#include <Engine/Core/Platform.h>
#include <Engine/Core/Profiler.h>
#include <Engine/Core/ConsoleVar.h>

#include <Engine/RenderCore/GPUSync.h>

#include <SDL/SDL.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_SyncGPU("com_SyncGPU"s, "0"s);
ConsoleVar com_MaxFPS("com_MaxFPS"s, "120"s);
ConsoleVar com_FrameSleep("com_FrameSleep"s, "0"s);

FrameLoop::FrameLoop(RenderCore::IDevice* RenderDevice) :
    m_FrameMemory(Allocators::FrameMemoryAllocator::GetAllocator()),
    m_RenderDevice(RenderDevice)
{
    m_GPUSync = MakeRef<GPUSync>(m_RenderDevice->GetImmediateContext());
    m_StreamedMemoryGPU = MakeRef<StreamedMemoryGPU>(m_RenderDevice);

    m_FrameTimeStamp = Core::SysStartMicroseconds();
    m_FrameDuration = 1000000.0 / 60;
    m_FrameNumber = 0;

    Core::ZeroMem(m_PressedKeys.ToPtr(), sizeof(m_PressedKeys));
    Core::ZeroMem(m_PressedMouseButtons.ToPtr(), sizeof(m_PressedMouseButtons));
    Core::ZeroMem(m_JoystickButtonState.ToPtr(), sizeof(m_JoystickButtonState));
    Core::ZeroMem(m_JoystickAxisState.ToPtr(), sizeof(m_JoystickAxisState));
    Core::ZeroMem(m_JoystickAdded.ToPtr(), sizeof(m_JoystickAdded));

    m_FontStash = GetSharedInstance<FontStash>();
}

FrameLoop::~FrameLoop()
{
    ClearViews();
}

void* FrameLoop::AllocFrameMem(size_t _SizeInBytes)
{
    return m_FrameMemory.Allocate(_SizeInBytes);
}

size_t FrameLoop::GetFrameMemorySize() const
{
    return m_FrameMemory.GetBlockMemoryUsage();
}

size_t FrameLoop::GetFrameMemoryUsed() const
{
    return m_FrameMemory.GetTotalMemoryUsage();
}

size_t FrameLoop::GetFrameMemoryUsedPrev() const
{
    return m_FrameMemoryUsedPrev;
}

size_t FrameLoop::GetMaxFrameMemoryUsage() const
{
    return m_MaxFrameMemoryUsage;
}

int64_t FrameLoop::SysFrameTimeStamp()
{
    return m_FrameTimeStamp;
}

int64_t FrameLoop::SysFrameDuration()
{
    return m_FrameDuration;
}

int FrameLoop::SysFrameNumber() const
{
    return m_FrameNumber;
}

void FrameLoop::SetGenerateInputEvents(bool bShouldGenerateInputEvents)
{
    m_bShouldGenerateInputEvents = bShouldGenerateInputEvents;
}

void FrameLoop::NewFrame(ArrayView<RenderCore::ISwapChain*> SwapChains, int SwapInterval, ResourceManager* resourceManager)
{
    HK_PROFILER_EVENT("Setup new frame");

    MemoryHeap::MemoryNewFrame();

    m_GPUSync->SetEvent();

    // Swap buffers for streamed memory
    m_StreamedMemoryGPU->Swap();

    // Swap window
    for (auto* swapChain : SwapChains)
    {
        swapChain->Present(SwapInterval);
    }

    // Wait for free streamed buffer
    m_StreamedMemoryGPU->Wait();

    if (com_FrameSleep.GetInteger() > 0)
    {
        Thread::WaitMilliseconds(com_FrameSleep.GetInteger());
    }

    int64_t prevTimeStamp = m_FrameTimeStamp;

    int64_t maxFrameRate = (SwapInterval == 0 && com_MaxFPS.GetInteger() > 0) ? 1000000.0 / com_MaxFPS.GetInteger() : 0;

    m_FrameTimeStamp = Core::SysMicroseconds();

    if (prevTimeStamp == Core::SysStartMicroseconds())
    {
        // First frame
        m_FrameDuration = 1000000.0 / 60;

        resourceManager->MainThread_Update(m_FrameDuration * 0.000001);
    }
    else
    {        
        m_FrameDuration = m_FrameTimeStamp - prevTimeStamp;

        if (m_FrameDuration < maxFrameRate)
        {
            resourceManager->MainThread_Update((maxFrameRate - m_FrameDuration) * 0.000001);

            m_FrameTimeStamp = Core::SysMicroseconds();
            m_FrameDuration = m_FrameTimeStamp - prevTimeStamp;

            if (m_FrameDuration < maxFrameRate)
            {
                Thread::WaitMicroseconds(maxFrameRate - m_FrameDuration);

                m_FrameTimeStamp = Core::SysMicroseconds();
                m_FrameDuration = m_FrameTimeStamp - prevTimeStamp;
            }
        }
        else
            resourceManager->MainThread_Update(0.001f);
    }

    m_FrameNumber++;

    // Keep memory statistics
    m_MaxFrameMemoryUsage = Math::Max(m_MaxFrameMemoryUsage, m_FrameMemory.GetTotalMemoryUsage());
    m_FrameMemoryUsedPrev = m_FrameMemory.GetTotalMemoryUsage();

    // Free frame memory for new frame
    m_FrameMemory.ResetAndMerge();

    ClearViews();

    m_FontStash->Cleanup();
}

void FrameLoop::ClearViews()
{
    for (auto* view : m_Views)
        view->RemoveRef();
    m_Views.Clear();
}

void FrameLoop::RegisterView(WorldRenderView* pView)
{
    m_Views.Add(pView);
    pView->AddRef();
}

struct KeyMappingsSDL : public Array<unsigned short, SDL_NUM_SCANCODES>
{
    KeyMappingsSDL()
    {
        KeyMappingsSDL& self = *this;

        Core::ZeroMem(ToPtr(), sizeof(*this));

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

static const KeyMappingsSDL SDLKeyMappings;

static HK_FORCEINLINE int FromKeymodSDL(Uint16 Mod)
{
    int modMask = 0;

    if (Mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        modMask |= MOD_MASK_SHIFT;
    }

    if (Mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        modMask |= MOD_MASK_CONTROL;
    }

    if (Mod & (KMOD_LALT | KMOD_RALT))
    {
        modMask |= MOD_MASK_ALT;
    }

    if (Mod & (KMOD_LGUI | KMOD_RGUI))
    {
        modMask |= MOD_MASK_SUPER;
    }

    return modMask;
}

static HK_FORCEINLINE int FromKeymodSDL_Char(Uint16 Mod)
{
    int modMask = 0;

    if (Mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        modMask |= MOD_MASK_SHIFT;
    }

    if (Mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        modMask |= MOD_MASK_CONTROL;
    }

    if (Mod & (KMOD_LALT | KMOD_RALT))
    {
        modMask |= MOD_MASK_ALT;
    }

    if (Mod & (KMOD_LGUI | KMOD_RGUI))
    {
        modMask |= MOD_MASK_SUPER;
    }

    if (Mod & KMOD_CAPS)
    {
        modMask |= MOD_MASK_CAPS_LOCK;
    }

    if (Mod & KMOD_NUM)
    {
        modMask |= MOD_MASK_NUM_LOCK;
    }

    return modMask;
}

void FrameLoop::UnpressJoystickButtons(IEventListener* Listener, int JoystickNum)
{
    JoystickButtonEvent buttonEvent;
    buttonEvent.Joystick = JoystickNum;
    buttonEvent.Action   = IA_RELEASE;
    for (int i = 0; i < MAX_JOYSTICK_BUTTONS; i++)
    {
        if (m_JoystickButtonState[JoystickNum][i])
        {
            m_JoystickButtonState[JoystickNum][i] = SDL_RELEASED;

            if (m_bShouldGenerateInputEvents)
            {
                buttonEvent.Button = JOY_BUTTON_1 + i;
                Listener->OnJoystickButtonEvent(buttonEvent);
            }
        }
    }
}

void FrameLoop::ClearJoystickAxes(IEventListener* Listener, int JoystickNum)
{
    JoystickAxisEvent axisEvent;
    axisEvent.Joystick = JoystickNum;
    axisEvent.Value    = 0;
    for (int i = 0; i < MAX_JOYSTICK_AXES; i++)
    {
        if (m_JoystickAxisState[JoystickNum][i] != 0)
        {
            m_JoystickAxisState[JoystickNum][i] = 0;

            if (m_bShouldGenerateInputEvents)
            {
                axisEvent.Axis = JOY_AXIS_1 + i;
                Listener->OnJoystickAxisEvent(axisEvent);
            }
        }
    }
}

void FrameLoop::UnpressKeysAndButtons(IEventListener* Listener)
{
    KeyEvent         keyEvent;
    MouseButtonEvent mouseEvent;

    keyEvent.Action  = IA_RELEASE;
    keyEvent.ModMask = 0;

    mouseEvent.Action  = IA_RELEASE;
    mouseEvent.ModMask = 0;

    for (int i = 0; i <= KEY_LAST; i++)
    {
        if (m_PressedKeys[i])
        {
            keyEvent.Key      = i;
            keyEvent.Scancode = m_PressedKeys[i] - 1;

            m_PressedKeys[i] = 0;

            if (m_bShouldGenerateInputEvents)
                Listener->OnKeyEvent(keyEvent);
        }
    }
    for (int i = MOUSE_BUTTON_1; i <= MOUSE_BUTTON_8; i++)
    {
        if (m_PressedMouseButtons[i])
        {
            mouseEvent.Button = i;

            m_PressedMouseButtons[i] = 0;

            if (m_bShouldGenerateInputEvents)
                Listener->OnMouseButtonEvent(mouseEvent);
        }
    }

    for (int i = 0; i < MAX_JOYSTICKS_COUNT; i++)
    {
        UnpressJoystickButtons(Listener, i);
        ClearJoystickAxes(Listener, i);
    }
}

void FrameLoop::PollEvents(IEventListener* Listener)
{
    // NOTE: Workaround of SDL bug with false mouse motion when a window gain keyboard focus.
    static bool bIgnoreFalseMouseMotionHack = false;

    HK_PROFILER_EVENT("Frame Poll Events");

    // Sync with GPU to prevent "input lag"
    if (com_SyncGPU)
    {
        m_GPUSync->Wait();
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // User-requested quit
            case SDL_QUIT:
                Listener->OnCloseEvent();
                break;

            // The application is being terminated by the OS
            // Called on iOS in applicationWillTerminate()
            // Called on Android in onDestroy()
            case SDL_APP_TERMINATING:
                LOG("PollEvent: Terminating\n");
                break;

            // The application is low on memory, free memory if possible.
            // Called on iOS in applicationDidReceiveMemoryWarning()
            // Called on Android in onLowMemory()
            case SDL_APP_LOWMEMORY:
                LOG("PollEvent: Low memory\n");
                break;

            // The application is about to enter the background
            // Called on iOS in applicationWillResignActive()
            // Called on Android in onPause()
            case SDL_APP_WILLENTERBACKGROUND:
                LOG("PollEvent: Will enter background\n");
                break;

            // The application did enter the background and may not get CPU for some time
            // Called on iOS in applicationDidEnterBackground()
            // Called on Android in onPause()
            case SDL_APP_DIDENTERBACKGROUND:
                LOG("PollEvent: Did enter background\n");
                break;

            // The application is about to enter the foreground
            // Called on iOS in applicationWillEnterForeground()
            // Called on Android in onResume()
            case SDL_APP_WILLENTERFOREGROUND:
                LOG("PollEvent: Will enter foreground\n");
                break;

            // The application is now interactive
            // Called on iOS in applicationDidBecomeActive()
            // Called on Android in onResume()
            case SDL_APP_DIDENTERFOREGROUND:
                LOG("PollEvent: Did enter foreground\n");
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
                                LOG("PollEvent: Display orientation has changed to landscape mode\n");
                                break;
                            // The display is in landscape mode, with the left side up, relative to portrait mode
                            case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
                                LOG("PollEvent: Display orientation has changed to flipped landscape mode\n");
                                break;
                            // The display is in portrait mode
                            case SDL_ORIENTATION_PORTRAIT:
                                LOG("PollEvent: Display orientation has changed to portrait mode\n");
                                break;
                            // The display is in portrait mode, upside down
                            case SDL_ORIENTATION_PORTRAIT_FLIPPED:
                                LOG("PollEvent: Display orientation has changed to flipped portrait mode\n");
                                break;
                            // The display orientation can't be determined
                            case SDL_ORIENTATION_UNKNOWN:
                            default:
                                LOG("PollEvent: The display orientation can't be determined\n");
                                break;
                        }
                    default:
                        LOG("PollEvent: Unknown display event type\n");
                        break;
                }
                break;

            // Window state change
            case SDL_WINDOWEVENT: {
                RenderCore::IGenericWindow* window = RenderCore::IGenericWindow::GetWindowFromNativeHandle(SDL_GetWindowFromID(event.window.windowID));
                if (window)
                {
                    window->ParseEvent(event);

                    switch (event.window.event)
                    {
                        // Window has been shown
                        case SDL_WINDOWEVENT_SHOWN:
                            Listener->OnWindowVisible(true);
                            break;
                        // Window has been hidden
                        case SDL_WINDOWEVENT_HIDDEN:
                            Listener->OnWindowVisible(false);
                            break;
                        // Window has been exposed and should be redrawn
                        case SDL_WINDOWEVENT_EXPOSED:
                            break;
                        // Window has been moved to data1, data2
                        case SDL_WINDOWEVENT_MOVED:
                            break;
                        // Window has been resized to data1xdata2
                        case SDL_WINDOWEVENT_RESIZED:
                        // The window size has changed, either as
                        // a result of an API call or through the
                        // system or user changing the window size.
                        case SDL_WINDOWEVENT_SIZE_CHANGED: {
                            auto& videoMode = window->GetVideoMode();
                            CoreApplication::GetConsoleBuffer().Resize(videoMode.FramebufferWidth);
                            Listener->OnResize();
                            break;
                        }
                        // Window has been minimized
                        case SDL_WINDOWEVENT_MINIMIZED:
                            Listener->OnWindowVisible(false);
                            break;
                        // Window has been maximized
                        case SDL_WINDOWEVENT_MAXIMIZED:
                            break;
                        // Window has been restored to normal size and position
                        case SDL_WINDOWEVENT_RESTORED:
                            Listener->OnWindowVisible(true);
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
                            UnpressKeysAndButtons(Listener);
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
                }
                break;
            }

            // System specific event
            case SDL_SYSWMEVENT:
                break;

            // Key pressed/released
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                KeyEvent keyEvent;
                keyEvent.Key      = SDLKeyMappings[event.key.keysym.scancode];
                keyEvent.Scancode = event.key.keysym.scancode;
                keyEvent.Action = (event.type == SDL_KEYDOWN) ? (m_PressedKeys[keyEvent.Key] ? IA_REPEAT : IA_PRESS) : IA_RELEASE;
                keyEvent.ModMask  = FromKeymodSDL(event.key.keysym.mod);
                if (keyEvent.Key)
                {
                    if ((keyEvent.Action == IA_RELEASE && !m_PressedKeys[keyEvent.Key]) || (keyEvent.Action == IA_PRESS && m_PressedKeys[keyEvent.Key]))
                    {
                        // State does not changed
                    }
                    else
                    {
                        m_PressedKeys[keyEvent.Key] = (keyEvent.Action == IA_RELEASE) ? 0 : keyEvent.Scancode + 1;

                        if (m_bShouldGenerateInputEvents)
                            Listener->OnKeyEvent(keyEvent);
                    }
                }
                break;
            }

            // Keyboard text editing (composition)
            case SDL_TEXTEDITING:
                break;

            // Keyboard text input
            case SDL_TEXTINPUT: {
                if (m_bShouldGenerateInputEvents)
                {
                    CharEvent charEvent;
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
                        Listener->OnCharEvent(charEvent);
                    }
                }
                break;
            }

            // Keymap changed due to a system event such as an
            // input language or keyboard layout change.
            case SDL_KEYMAPCHANGED:
                break;

            // Mouse moved
            case SDL_MOUSEMOTION: {
                if (!bIgnoreFalseMouseMotionHack && m_bShouldGenerateInputEvents)
                {
                    MouseMoveEvent moveEvent;
                    moveEvent.X = event.motion.xrel;
                    moveEvent.Y = -event.motion.yrel;
                    Listener->OnMouseMoveEvent(moveEvent);
                }
                bIgnoreFalseMouseMotionHack = false;
                break;
            }

            // Mouse button pressed
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                MouseButtonEvent mouseEvent;
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
                    if (mouseEvent.Action == (int)m_PressedMouseButtons[mouseEvent.Button])
                    {

                        // State does not changed
                    }
                    else
                    {
                        m_PressedMouseButtons[mouseEvent.Button] = mouseEvent.Action != IA_RELEASE;

                        if (m_bShouldGenerateInputEvents)
                            Listener->OnMouseButtonEvent(mouseEvent);
                    }
                }
                break;
            }

            // Mouse wheel motion
            case SDL_MOUSEWHEEL: {
                if (m_bShouldGenerateInputEvents)
                {
                    MouseWheelEvent wheelEvent;
                    wheelEvent.WheelX = event.wheel.x;
                    wheelEvent.WheelY = event.wheel.y;
                    Listener->OnMouseWheelEvent(wheelEvent);

                    MouseButtonEvent mouseEvent;
                    mouseEvent.ModMask = FromKeymodSDL(SDL_GetModState());

                    if (wheelEvent.WheelX < 0.0)
                    {
                        mouseEvent.Button = MOUSE_WHEEL_LEFT;

                        mouseEvent.Action = IA_PRESS;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = IA_RELEASE;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                    else if (wheelEvent.WheelX > 0.0)
                    {
                        mouseEvent.Button = MOUSE_WHEEL_RIGHT;

                        mouseEvent.Action = IA_PRESS;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = IA_RELEASE;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                    if (wheelEvent.WheelY < 0.0)
                    {
                        mouseEvent.Button = MOUSE_WHEEL_DOWN;

                        mouseEvent.Action = IA_PRESS;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = IA_RELEASE;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                    else if (wheelEvent.WheelY > 0.0)
                    {
                        mouseEvent.Button = MOUSE_WHEEL_UP;

                        mouseEvent.Action = IA_PRESS;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = IA_RELEASE;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                }
                break;
            }

            // Joystick axis motion
            case SDL_JOYAXISMOTION: {
                if (m_bShouldGenerateInputEvents)
                {
                    if (event.jaxis.which >= 0 && event.jaxis.which < MAX_JOYSTICKS_COUNT)
                    {
                        HK_ASSERT(m_JoystickAdded[event.jaxis.which]);
                        if (event.jaxis.axis >= 0 && event.jaxis.axis < MAX_JOYSTICK_AXES)
                        {
                            if (m_JoystickAxisState[event.jaxis.which][event.jaxis.axis] != event.jaxis.value)
                            {
                                JoystickAxisEvent axisEvent;
                                axisEvent.Joystick = event.jaxis.which;
                                axisEvent.Axis     = JOY_AXIS_1 + event.jaxis.axis;
                                axisEvent.Value    = ((float)event.jaxis.value + 32768.0f) / 0xffff * 2.0f - 1.0f; // scale to -1.0f ... 1.0f

                                Listener->OnJoystickAxisEvent(axisEvent);
                            }
                        }
                        else
                        {
                            HK_ASSERT_(0, "Invalid joystick axis num");
                        }
                    }
                    else
                    {
                        HK_ASSERT_(0, "Invalid joystick id");
                    }
                }
                break;
            }

            // Joystick trackball motion
            case SDL_JOYBALLMOTION:
                LOG("PollEvent: Joystick ball move\n");
                break;

            // Joystick hat position change
            case SDL_JOYHATMOTION:
                LOG("PollEvent: Joystick hat move\n");
                break;

            // Joystick button pressed
            case SDL_JOYBUTTONDOWN:
            // Joystick button released
            case SDL_JOYBUTTONUP: {
                if (event.jbutton.which >= 0 && event.jbutton.which < MAX_JOYSTICKS_COUNT)
                {
                    HK_ASSERT(m_JoystickAdded[event.jbutton.which]);
                    if (event.jbutton.button >= 0 && event.jbutton.button < MAX_JOYSTICK_BUTTONS)
                    {
                        if (m_JoystickButtonState[event.jbutton.which][event.jbutton.button] != event.jbutton.state)
                        {
                            m_JoystickButtonState[event.jbutton.which][event.jbutton.button] = event.jbutton.state;

                            if (m_bShouldGenerateInputEvents)
                            {
                                JoystickButtonEvent buttonEvent;
                                buttonEvent.Joystick = event.jbutton.which;
                                buttonEvent.Button   = JOY_BUTTON_1 + event.jbutton.button;
                                buttonEvent.Action   = event.jbutton.state == SDL_PRESSED ? IA_PRESS : IA_RELEASE;
                                Listener->OnJoystickButtonEvent(buttonEvent);
                            }
                        }
                    }
                    else
                    {
                        HK_ASSERT_(0, "Invalid joystick button num");
                    }
                }
                else
                {
                    HK_ASSERT_(0, "Invalid joystick id");
                }
                break;
            }

            // A new joystick has been inserted into the system
            case SDL_JOYDEVICEADDED:
                if (event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT)
                {
                    HK_ASSERT(!m_JoystickAdded[event.jdevice.which]);
                    m_JoystickAdded[event.jdevice.which] = true;

                    Core::ZeroMem(m_JoystickButtonState[event.jdevice.which].ToPtr(), sizeof(m_JoystickButtonState[0]));
                    Core::ZeroMem(m_JoystickAxisState[event.jdevice.which].ToPtr(), sizeof(m_JoystickAxisState[0]));
                }
                else
                {
                    HK_ASSERT_(0, "Invalid joystick id");
                }
                LOG("PollEvent: Joystick added\n");
                break;

            // An opened joystick has been removed
            case SDL_JOYDEVICEREMOVED: {
                if (event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT)
                {
                    UnpressJoystickButtons(Listener, event.jdevice.which);
                    ClearJoystickAxes(Listener, event.jdevice.which);

                    HK_ASSERT(m_JoystickAdded[event.jdevice.which]);
                    m_JoystickAdded[event.jdevice.which] = false;
                }
                else
                {
                    HK_ASSERT_(0, "Invalid joystick id");
                }

                LOG("PollEvent: Joystick removed\n");
                break;
            }

            // Game controller axis motion
            case SDL_CONTROLLERAXISMOTION:
                LOG("PollEvent: Gamepad axis move\n");
                break;
            // Game controller button pressed
            case SDL_CONTROLLERBUTTONDOWN:
                LOG("PollEvent: Gamepad button press\n");
                break;
            // Game controller button released
            case SDL_CONTROLLERBUTTONUP:
                LOG("PollEvent: Gamepad button release\n");
                break;
            // A new Game controller has been inserted into the system
            case SDL_CONTROLLERDEVICEADDED:
                LOG("PollEvent: Gamepad added\n");
                break;
            // An opened Game controller has been removed
            case SDL_CONTROLLERDEVICEREMOVED:
                LOG("PollEvent: Gamepad removed\n");
                break;
            // The controller mapping was updated
            case SDL_CONTROLLERDEVICEREMAPPED:
                LOG("PollEvent: Gamepad device mapped\n");
                break;

            // Touch events
            case SDL_FINGERDOWN:
                LOG("PollEvent: Touch press\n");
                break;
            case SDL_FINGERUP:
                LOG("PollEvent: Touch release\n");
                break;
            case SDL_FINGERMOTION:
                LOG("PollEvent: Touch move\n");
                break;

            // Gesture events
            case SDL_DOLLARGESTURE:
                LOG("PollEvent: Dollar gesture\n");
                break;
            case SDL_DOLLARRECORD:
                LOG("PollEvent: Dollar record\n");
                break;
            case SDL_MULTIGESTURE:
                LOG("PollEvent: Multigesture\n");
                break;

            // The clipboard changed
            case SDL_CLIPBOARDUPDATE:
                LOG("PollEvent: Clipboard update\n");
                break;

            // The system requests a file open
            case SDL_DROPFILE:
                LOG("PollEvent: Drop file\n");
                break;
            // text/plain drag-and-drop event
            case SDL_DROPTEXT:
                LOG("PollEvent: Drop text\n");
                break;
            // A new set of drops is beginning (NULL filename)
            case SDL_DROPBEGIN:
                LOG("PollEvent: Drop begin\n");
                break;
            // Current set of drops is now complete (NULL filename)
            case SDL_DROPCOMPLETE:
                LOG("PollEvent: Drop complete\n");
                break;

            // A new audio device is available
            case SDL_AUDIODEVICEADDED:
                LOG("Audio {} device added: {}\n", event.adevice.iscapture ? "capture" : "playback", SDL_GetAudioDeviceName(event.adevice.which, event.adevice.iscapture));
                break;
            // An audio device has been removed
            case SDL_AUDIODEVICEREMOVED:
                LOG("Audio {} device removed: {}\n", event.adevice.iscapture ? "capture" : "playback", SDL_GetAudioDeviceName(event.adevice.which, event.adevice.iscapture));
                break;

            // A sensor was updated
            case SDL_SENSORUPDATE:
                LOG("PollEvent: Sensor update\n");
                break;

            // The render targets have been reset and their contents need to be updated
            case SDL_RENDER_TARGETS_RESET:
                LOG("PollEvent: Render targets reset\n");
                break;
            case SDL_RENDER_DEVICE_RESET:
                LOG("PollEvent: Render device reset\n");
                break;
        }
    }
}

HK_NAMESPACE_END
