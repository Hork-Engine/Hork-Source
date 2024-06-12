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
    m_GPUSync = MakeUnique<GPUSync>(m_RenderDevice->GetImmediateContext());
    m_StreamedMemoryGPU = MakeUnique<StreamedMemoryGPU>(m_RenderDevice);

    m_FrameTimeStamp = Core::SysStartMicroseconds();
    m_FrameDuration = 1000000.0 / 60;
    m_FrameNumber = 0;

    m_PressedKeys.ZeroMem();
    //Core::ZeroMem(m_JoystickAxisState.ToPtr(), sizeof(m_JoystickAxisState));
    //Core::ZeroMem(m_JoystickAdded.ToPtr(), sizeof(m_JoystickAdded));

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

static const VirtualKey InvalidKey = VirtualKey(0xffff);

struct KeyMappingsSDL : public Array<VirtualKey, SDL_NUM_SCANCODES>
{
    KeyMappingsSDL()
    {
        KeyMappingsSDL& self = *this;

        for (size_t n = 0; n < SDL_NUM_SCANCODES; ++n)
            self[n] = InvalidKey;

        self[SDL_SCANCODE_A]            = VirtualKey::A;
        self[SDL_SCANCODE_B]            = VirtualKey::B;
        self[SDL_SCANCODE_C]            = VirtualKey::C;
        self[SDL_SCANCODE_D]            = VirtualKey::D;
        self[SDL_SCANCODE_E]            = VirtualKey::E;
        self[SDL_SCANCODE_F]            = VirtualKey::F;
        self[SDL_SCANCODE_G]            = VirtualKey::G;
        self[SDL_SCANCODE_H]            = VirtualKey::H;
        self[SDL_SCANCODE_I]            = VirtualKey::I;
        self[SDL_SCANCODE_J]            = VirtualKey::J;
        self[SDL_SCANCODE_K]            = VirtualKey::K;
        self[SDL_SCANCODE_L]            = VirtualKey::L;
        self[SDL_SCANCODE_M]            = VirtualKey::M;
        self[SDL_SCANCODE_N]            = VirtualKey::N;
        self[SDL_SCANCODE_O]            = VirtualKey::O;
        self[SDL_SCANCODE_P]            = VirtualKey::P;
        self[SDL_SCANCODE_Q]            = VirtualKey::Q;
        self[SDL_SCANCODE_R]            = VirtualKey::R;
        self[SDL_SCANCODE_S]            = VirtualKey::S;
        self[SDL_SCANCODE_T]            = VirtualKey::T;
        self[SDL_SCANCODE_U]            = VirtualKey::U;
        self[SDL_SCANCODE_V]            = VirtualKey::V;
        self[SDL_SCANCODE_W]            = VirtualKey::W;
        self[SDL_SCANCODE_X]            = VirtualKey::X;
        self[SDL_SCANCODE_Y]            = VirtualKey::Y;
        self[SDL_SCANCODE_Z]            = VirtualKey::Z;
        self[SDL_SCANCODE_1]            = VirtualKey::_1;
        self[SDL_SCANCODE_2]            = VirtualKey::_2;
        self[SDL_SCANCODE_3]            = VirtualKey::_3;
        self[SDL_SCANCODE_4]            = VirtualKey::_4;
        self[SDL_SCANCODE_5]            = VirtualKey::_5;
        self[SDL_SCANCODE_6]            = VirtualKey::_6;
        self[SDL_SCANCODE_7]            = VirtualKey::_7;
        self[SDL_SCANCODE_8]            = VirtualKey::_8;
        self[SDL_SCANCODE_9]            = VirtualKey::_9;
        self[SDL_SCANCODE_0]            = VirtualKey::_0;
        self[SDL_SCANCODE_RETURN]       = VirtualKey::Enter;
        self[SDL_SCANCODE_ESCAPE]       = VirtualKey::Escape;
        self[SDL_SCANCODE_BACKSPACE]    = VirtualKey::Backspace;
        self[SDL_SCANCODE_TAB]          = VirtualKey::Tab;
        self[SDL_SCANCODE_SPACE]        = VirtualKey::Space;
        self[SDL_SCANCODE_MINUS]        = VirtualKey::Minus;
        self[SDL_SCANCODE_EQUALS]       = VirtualKey::Equal;
        self[SDL_SCANCODE_LEFTBRACKET]  = VirtualKey::LeftBracket;
        self[SDL_SCANCODE_RIGHTBRACKET] = VirtualKey::RightBracket;
        self[SDL_SCANCODE_BACKSLASH]    = VirtualKey::Backslash;
        self[SDL_SCANCODE_SEMICOLON]    = VirtualKey::Semicolon;
        self[SDL_SCANCODE_APOSTROPHE]   = VirtualKey::Apostrophe;
        self[SDL_SCANCODE_GRAVE]        = VirtualKey::GraveAccent;
        self[SDL_SCANCODE_COMMA]        = VirtualKey::Comma;
        self[SDL_SCANCODE_PERIOD]       = VirtualKey::Period;
        self[SDL_SCANCODE_SLASH]        = VirtualKey::Slash;
        self[SDL_SCANCODE_CAPSLOCK]     = VirtualKey::CapsLock;
        self[SDL_SCANCODE_F1]           = VirtualKey::F1;
        self[SDL_SCANCODE_F2]           = VirtualKey::F2;
        self[SDL_SCANCODE_F3]           = VirtualKey::F3;
        self[SDL_SCANCODE_F4]           = VirtualKey::F4;
        self[SDL_SCANCODE_F5]           = VirtualKey::F5;
        self[SDL_SCANCODE_F6]           = VirtualKey::F6;
        self[SDL_SCANCODE_F7]           = VirtualKey::F7;
        self[SDL_SCANCODE_F8]           = VirtualKey::F8;
        self[SDL_SCANCODE_F9]           = VirtualKey::F9;
        self[SDL_SCANCODE_F10]          = VirtualKey::F10;
        self[SDL_SCANCODE_F11]          = VirtualKey::F11;
        self[SDL_SCANCODE_F12]          = VirtualKey::F12;
        self[SDL_SCANCODE_PRINTSCREEN]  = VirtualKey::PrintScreen;
        self[SDL_SCANCODE_SCROLLLOCK]   = VirtualKey::ScrollLock;
        self[SDL_SCANCODE_PAUSE]        = VirtualKey::Pause;
        self[SDL_SCANCODE_INSERT]       = VirtualKey::Insert;
        self[SDL_SCANCODE_HOME]         = VirtualKey::Home;
        self[SDL_SCANCODE_PAGEUP]       = VirtualKey::PageUp;
        self[SDL_SCANCODE_DELETE]       = VirtualKey::Delete;
        self[SDL_SCANCODE_END]          = VirtualKey::End;
        self[SDL_SCANCODE_PAGEDOWN]     = VirtualKey::PageDown;
        self[SDL_SCANCODE_RIGHT]        = VirtualKey::Right;
        self[SDL_SCANCODE_LEFT]         = VirtualKey::Left;
        self[SDL_SCANCODE_DOWN]         = VirtualKey::Down;
        self[SDL_SCANCODE_UP]           = VirtualKey::Up;
        self[SDL_SCANCODE_NUMLOCKCLEAR] = VirtualKey::NumLock;
        self[SDL_SCANCODE_KP_DIVIDE]    = VirtualKey::KP_Divide;
        self[SDL_SCANCODE_KP_MULTIPLY]  = VirtualKey::KP_Multiply;
        self[SDL_SCANCODE_KP_MINUS]     = VirtualKey::KP_Subtract;
        self[SDL_SCANCODE_KP_PLUS]      = VirtualKey::KP_Add;
        self[SDL_SCANCODE_KP_ENTER]     = VirtualKey::KP_Enter;
        self[SDL_SCANCODE_KP_1]         = VirtualKey::KP_1;
        self[SDL_SCANCODE_KP_2]         = VirtualKey::KP_2;
        self[SDL_SCANCODE_KP_3]         = VirtualKey::KP_3;
        self[SDL_SCANCODE_KP_4]         = VirtualKey::KP_4;
        self[SDL_SCANCODE_KP_5]         = VirtualKey::KP_5;
        self[SDL_SCANCODE_KP_6]         = VirtualKey::KP_6;
        self[SDL_SCANCODE_KP_7]         = VirtualKey::KP_7;
        self[SDL_SCANCODE_KP_8]         = VirtualKey::KP_8;
        self[SDL_SCANCODE_KP_9]         = VirtualKey::KP_9;
        self[SDL_SCANCODE_KP_0]         = VirtualKey::KP_0;
        self[SDL_SCANCODE_KP_PERIOD]    = VirtualKey::KP_Decimal;
        self[SDL_SCANCODE_KP_EQUALS]    = VirtualKey::KP_Equal;
        self[SDL_SCANCODE_F13]          = VirtualKey::F13;
        self[SDL_SCANCODE_F14]          = VirtualKey::F14;
        self[SDL_SCANCODE_F15]          = VirtualKey::F15;
        self[SDL_SCANCODE_F16]          = VirtualKey::F16;
        self[SDL_SCANCODE_F17]          = VirtualKey::F17;
        self[SDL_SCANCODE_F18]          = VirtualKey::F18;
        self[SDL_SCANCODE_F19]          = VirtualKey::F19;
        self[SDL_SCANCODE_F20]          = VirtualKey::F20;
        self[SDL_SCANCODE_F21]          = VirtualKey::F21;
        self[SDL_SCANCODE_F22]          = VirtualKey::F22;
        self[SDL_SCANCODE_F23]          = VirtualKey::F23;
        self[SDL_SCANCODE_F24]          = VirtualKey::F24;
        self[SDL_SCANCODE_MENU]         = VirtualKey::Menu;
        self[SDL_SCANCODE_LCTRL]        = VirtualKey::LeftControl;
        self[SDL_SCANCODE_LSHIFT]       = VirtualKey::LeftShift;
        self[SDL_SCANCODE_LALT]         = VirtualKey::LeftAlt;
        self[SDL_SCANCODE_LGUI]         = VirtualKey::LeftSuper;
        self[SDL_SCANCODE_RCTRL]        = VirtualKey::RightControl;
        self[SDL_SCANCODE_RSHIFT]       = VirtualKey::RightShift;
        self[SDL_SCANCODE_RALT]         = VirtualKey::RightAlt;
        self[SDL_SCANCODE_RGUI]         = VirtualKey::RightSuper;
    }
};

static const KeyMappingsSDL SDLKeyMappings;

static HK_FORCEINLINE KeyModifierMask FromKeymodSDL(Uint16 Mod)
{
    KeyModifierMask modMask;

    if (Mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        modMask.Shift = true;
    }

    if (Mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        modMask.Control = true;
    }

    if (Mod & (KMOD_LALT | KMOD_RALT))
    {
        modMask.Alt = true;
    }

    if (Mod & (KMOD_LGUI | KMOD_RGUI))
    {
        modMask.Super = true;
    }

    return modMask;
}

static HK_FORCEINLINE KeyModifierMask FromKeymodSDL_Char(Uint16 Mod)
{
    KeyModifierMask modMask;

    if (Mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        modMask.Shift = true;
    }

    if (Mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        modMask.Control = true;
    }

    if (Mod & (KMOD_LALT | KMOD_RALT))
    {
        modMask.Alt = true;
    }

    if (Mod & (KMOD_LGUI | KMOD_RGUI))
    {
        modMask.Super = true;
    }

    if (Mod & KMOD_CAPS)
    {
        modMask.CapsLock = true;
    }

    if (Mod & KMOD_NUM)
    {
        modMask.NumLock = true;
    }

    return modMask;
}

void FrameLoop::UnpressJoystickButtons(IEventListener* Listener, int JoystickNum)
{
#if 0
    JoystickButtonEvent joystickEvent;
    joystickEvent.Joystick = JoystickNum;
    joystickEvent.Action   = InputAction::Released;
    for (VirtualKey virtualKey = VirtualKey::JoyBtn1; virtualKey <= VirtualKey::JoyBtn32; virtualKey = VirtualKey(ToUnderlying(virtualKey) + 1))
    {
        uint32_t index = ToUnderlying(virtualKey);
        if (m_PressedKeys[index]) // TODO: Keep pressed keys per joystick device!!!
        {
            m_PressedKeys[index] = 0;

            if (m_bShouldGenerateInputEvents)
            {
                joystickEvent.Button = virtualKey;
                Listener->OnJoystickButtonEvent(joystickEvent);
            }
        }
    }
#endif
}

void FrameLoop::ClearJoystickAxes(IEventListener* Listener, int JoystickNum)
{
#if 0
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
#endif
}

void FrameLoop::UnpressKeysAndButtons(IEventListener* Listener)
{
    KeyEvent         keyEvent;
    MouseButtonEvent mouseEvent;
    //JoystickButtonEvent joystickEvent;

    keyEvent.Action  = InputAction::Released;

    mouseEvent.Action  = InputAction::Released;

    //joystickEvent.Action = InputAction::Released;

    for (uint32_t i = 0; i <= VirtualKeyTableSize; ++i)
    {
        if (m_PressedKeys[i])
        {
            m_PressedKeys[i] = 0;

            if (m_bShouldGenerateInputEvents)
            {
                VirtualKey virtualKey = VirtualKey(i);
                if (virtualKey >= VirtualKey::MouseLeftBtn && virtualKey <= VirtualKey::Mouse8/*MouseWheelRight*/)
                {
                    mouseEvent.Button = virtualKey;

                    Listener->OnMouseButtonEvent(mouseEvent);
                }
                else if (virtualKey >= VirtualKey::JoyBtn1 && virtualKey >= VirtualKey::JoyBtn32)
                {
                    //joystickEvent.Button = virtualKey;

                    //for (int joystickNum = 0; joystickNum < MAX_JOYSTICKS_COUNT; ++joystickNum)
                    //{
                    //    joystickEvent.Joystick = joystickNum;
                    //    Listener->OnJoystickButtonEvent(joystickEvent);
                    //}
                }
                else
                {
                    keyEvent.Key      = virtualKey;
                    keyEvent.Scancode = m_PressedKeys[i] - 1;

                    Listener->OnKeyEvent(keyEvent);
                }
            }
        }
    }

    //for (int i = 0; i < MAX_JOYSTICKS_COUNT; i++)
    //{
    //    ClearJoystickAxes(Listener, i);
    //}
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
                keyEvent.Action = (event.type == SDL_KEYDOWN) ? (m_PressedKeys[ToUnderlying(keyEvent.Key)] ? InputAction::Repeat : InputAction::Pressed) : InputAction::Released;
                keyEvent.ModMask  = FromKeymodSDL(event.key.keysym.mod);
                if (keyEvent.Key != InvalidKey)
                {
                    if ((keyEvent.Action == InputAction::Released && !m_PressedKeys[ToUnderlying(keyEvent.Key)]) || (keyEvent.Action == InputAction::Pressed && m_PressedKeys[ToUnderlying(keyEvent.Key)]))
                    {
                        // State does not changed
                    }
                    else
                    {
                        m_PressedKeys[ToUnderlying(keyEvent.Key)] = (keyEvent.Action == InputAction::Released) ? 0 : keyEvent.Scancode + 1;

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
                        mouseEvent.Button = VirtualKey::MouseMidBtn;
                        break;
                    case 3:
                        mouseEvent.Button = VirtualKey::MouseRightBtn;
                        break;
                    default:
                        mouseEvent.Button = VirtualKey(ToUnderlying(VirtualKey::MouseLeftBtn) + event.button.button - 1);
                        break;
                }
                mouseEvent.Action  = event.type == SDL_MOUSEBUTTONDOWN ? InputAction::Pressed : InputAction::Released;
                mouseEvent.ModMask = FromKeymodSDL(SDL_GetModState());

                if (mouseEvent.Button >= VirtualKey::MouseLeftBtn && mouseEvent.Button <= VirtualKey::Mouse8)
                {
                    if (mouseEvent.Action == (int)m_PressedKeys[ToUnderlying(mouseEvent.Button)])
                    {
                        // State does not changed
                    }
                    else
                    {
                        m_PressedKeys[ToUnderlying(mouseEvent.Button)] = mouseEvent.Action != InputAction::Released;

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
                        mouseEvent.Button = VirtualKey::MouseWheelLeft;

                        mouseEvent.Action = InputAction::Pressed;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                    else if (wheelEvent.WheelX > 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelRight;

                        mouseEvent.Action = InputAction::Pressed;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                    if (wheelEvent.WheelY < 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelDown;

                        mouseEvent.Action = InputAction::Pressed;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                    else if (wheelEvent.WheelY > 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelUp;

                        mouseEvent.Action = InputAction::Pressed;
                        Listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        Listener->OnMouseButtonEvent(mouseEvent);
                    }
                }
                break;
            }

            // Joystick axis motion
            case SDL_JOYAXISMOTION: {
            //    if (m_bShouldGenerateInputEvents)
            //    {
            //        if (event.jaxis.which >= 0 && event.jaxis.which < MAX_JOYSTICKS_COUNT)
            //        {
            //            HK_ASSERT(m_JoystickAdded[event.jaxis.which]);
            //            if (event.jaxis.axis >= 0 && event.jaxis.axis < MAX_JOYSTICK_AXES)
            //            {
            //                if (m_JoystickAxisState[event.jaxis.which][event.jaxis.axis] != event.jaxis.value)
            //                {
            //                    JoystickAxisEvent axisEvent;
            //                    axisEvent.Joystick = event.jaxis.which;
            //                    axisEvent.Axis     = JOY_AXIS_1 + event.jaxis.axis;
            //                    axisEvent.Value    = ((float)event.jaxis.value + 32768.0f) / 0xffff * 2.0f - 1.0f; // scale to -1.0f ... 1.0f

            //                    Listener->OnJoystickAxisEvent(axisEvent);
            //                }
            //            }
            //            else
            //            {
            //                HK_ASSERT_(0, "Invalid joystick axis num");
            //            }
            //        }
            //        else
            //        {
            //            HK_ASSERT_(0, "Invalid joystick id");
            //        }
            //    }
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
                //if (event.jbutton.which >= 0 && event.jbutton.which < MAX_JOYSTICKS_COUNT)
                //{
                //    HK_ASSERT(m_JoystickAdded[event.jbutton.which]);
                //    if (event.jbutton.button >= 0 && event.jbutton.button < MAX_JOYSTICK_BUTTONS)
                //    {
                //        if (m_JoystickButtonState[event.jbutton.which][event.jbutton.button] != event.jbutton.state)
                //        {
                //            m_JoystickButtonState[event.jbutton.which][event.jbutton.button] = event.jbutton.state;

                //            if (m_bShouldGenerateInputEvents)
                //            {
                //                JoystickButtonEvent buttonEvent;
                //                buttonEvent.Joystick = event.jbutton.which;
                //                buttonEvent.Button   = JOY_BUTTON_1 + event.jbutton.button;
                //                buttonEvent.Action   = event.jbutton.state == SDL_PRESSED ? InputAction::Pressed : InputAction::Released;
                //                Listener->OnJoystickButtonEvent(buttonEvent);
                //            }
                //        }
                //    }
                //    else
                //    {
                //        HK_ASSERT_(0, "Invalid joystick button num");
                //    }
                //}
                //else
                //{
                //    HK_ASSERT_(0, "Invalid joystick id");
                //}
                break;
            }

            // A new joystick has been inserted into the system
            case SDL_JOYDEVICEADDED:
                //if (event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT)
                //{
                //    HK_ASSERT(!m_JoystickAdded[event.jdevice.which]);
                //    m_JoystickAdded[event.jdevice.which] = true;

                //    Core::ZeroMem(m_JoystickButtonState[event.jdevice.which].ToPtr(), sizeof(m_JoystickButtonState[0]));
                //    Core::ZeroMem(m_JoystickAxisState[event.jdevice.which].ToPtr(), sizeof(m_JoystickAxisState[0]));
                //}
                //else
                //{
                //    HK_ASSERT_(0, "Invalid joystick id");
                //}
                //LOG("PollEvent: Joystick added\n");
                break;

            // An opened joystick has been removed
            case SDL_JOYDEVICEREMOVED: {
                //if (event.jdevice.which >= 0 && event.jdevice.which < MAX_JOYSTICKS_COUNT)
                //{
                //    UnpressJoystickButtons(Listener, event.jdevice.which);
                //    ClearJoystickAxes(Listener, event.jdevice.which);

                //    HK_ASSERT(m_JoystickAdded[event.jdevice.which]);
                //    m_JoystickAdded[event.jdevice.which] = false;
                //}
                //else
                //{
                //    HK_ASSERT_(0, "Invalid joystick id");
                //}

                //LOG("PollEvent: Joystick removed\n");
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
