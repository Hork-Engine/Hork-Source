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

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_SyncGPU("com_SyncGPU"_s, "0"_s);
ConsoleVar com_MaxFPS("com_MaxFPS"_s, "120"_s);
ConsoleVar com_FrameSleep("com_FrameSleep"_s, "0"_s);
ConsoleVar in_StickDeadZone("in_StickDeadZone"_s, "0.23"_s);

FrameLoop::FrameLoop(RenderCore::IDevice* renderDevice) :
    m_FrameMemory(Allocators::FrameMemoryAllocator::GetAllocator()),
    m_RenderDevice(renderDevice)
{
    m_GPUSync = MakeUnique<GPUSync>(m_RenderDevice->GetImmediateContext());
    m_StreamedMemoryGPU = MakeUnique<StreamedMemoryGPU>(m_RenderDevice);

    m_FrameTimeStamp = Core::SysStartMicroseconds();
    m_FrameDuration = 1000000.0 / 60;
    m_FrameNumber = 0;

    m_FontStash = GetSharedInstance<FontStash>();
}

FrameLoop::~FrameLoop()
{
    ClearViews();
}

void* FrameLoop::AllocFrameMem(size_t sizeInBytes)
{
    return m_FrameMemory.Allocate(sizeInBytes);
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

void FrameLoop::SetGenerateInputEvents(bool shouldGenerateInputEvents)
{
    m_ShouldGenerateInputEvents = shouldGenerateInputEvents;
}

void FrameLoop::NewFrame(ArrayView<RenderCore::ISwapChain*> swapChains, int swapInterval, ResourceManager* resourceManager)
{
    HK_PROFILER_EVENT("Setup new frame");

    MemoryHeap::MemoryNewFrame();

    m_GPUSync->SetEvent();

    // Swap buffers for streamed memory
    m_StreamedMemoryGPU->Swap();

    // Swap window
    for (auto* swapChain : swapChains)
    {
        HK_PROFILER_EVENT("Swap chain present");
        swapChain->Present(swapInterval);
    }

    // Wait for free streamed buffer
    m_StreamedMemoryGPU->Wait();

    if (com_FrameSleep.GetInteger() > 0)
    {
        Thread::WaitMilliseconds(com_FrameSleep.GetInteger());
    }

    int64_t prevTimeStamp = m_FrameTimeStamp;

    int64_t maxFrameRate = (swapInterval == 0 && com_MaxFPS.GetInteger() > 0) ? 1000000.0 / com_MaxFPS.GetInteger() : 0;

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

void FrameLoop::RegisterView(WorldRenderView* view)
{
    m_Views.Add(view);
    view->AddRef();
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

static KeyModifierMask FromKeymodSDL(Uint16 mod)
{
    KeyModifierMask modMask;

    if (mod & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT))
    {
        modMask.Shift = true;
    }

    if (mod & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL))
    {
        modMask.Control = true;
    }

    if (mod & (SDL_KMOD_LALT | SDL_KMOD_RALT))
    {
        modMask.Alt = true;
    }

    if (mod & (SDL_KMOD_LGUI | SDL_KMOD_RGUI))
    {
        modMask.Super = true;
    }

    return modMask;
}

static KeyModifierMask FromKeymodSDL_Char(Uint16 mod)
{
    KeyModifierMask modMask;

    if (mod & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT))
    {
        modMask.Shift = true;
    }

    if (mod & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL))
    {
        modMask.Control = true;
    }

    if (mod & (SDL_KMOD_LALT | SDL_KMOD_RALT))
    {
        modMask.Alt = true;
    }

    if (mod & (SDL_KMOD_LGUI | SDL_KMOD_RGUI))
    {
        modMask.Super = true;
    }

    if (mod & SDL_KMOD_CAPS)
    {
        modMask.CapsLock = true;
    }

    if (mod & SDL_KMOD_NUM)
    {
        modMask.NumLock = true;
    }

    return modMask;
}

void FrameLoop::PollEvents(IEventListener* listener)
{
    // NOTE: Workaround of SDL bug with false mouse motion when a window gain keyboard focus.
    static bool IgnoreFalseMouseMotionHack = false;

    HK_PROFILER_EVENT("Frame Poll Events");

    // Sync with GPU to prevent "input lag"
    if (com_SyncGPU)
    {
        m_GPUSync->Wait();
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // Handle window events
        if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
        {
            RenderCore::IGenericWindow* window = RenderCore::IGenericWindow::GetWindowFromNativeHandle(SDL_GetWindowFromID(event.window.windowID));
            if (window)
            {
                window->ParseEvent(event.window);

                switch (event.type)
                {
                    // Window has been shown
                    case SDL_EVENT_WINDOW_SHOWN:
                        listener->OnWindowVisible(true);
                        break;
                    // Window has been hidden
                    case SDL_EVENT_WINDOW_HIDDEN:
                        listener->OnWindowVisible(false);
                        break;
                    // Window has been exposed and should be redrawn
                    case SDL_EVENT_WINDOW_EXPOSED:
                        break;
                    // Window has been moved to data1, data2
                    case SDL_EVENT_WINDOW_MOVED:
                        break;
                    // Window has been resized to data1xdata2
                    case SDL_EVENT_WINDOW_RESIZED:
                        break;
                    // The window size has changed, either as
                    // a result of an API call or through the
                    // system or user changing the window size.
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                        CoreApplication::GetConsoleBuffer().Resize(window->GetFramebufferWidth());
                        listener->OnResize();
                        break;
                    // Window has been minimized
                    case SDL_EVENT_WINDOW_MINIMIZED:
                        listener->OnWindowVisible(false);
                        break;
                    // Window has been maximized
                    case SDL_EVENT_WINDOW_MAXIMIZED:
                        break;
                    // Window has been restored to normal size and position
                    case SDL_EVENT_WINDOW_RESTORED:
                        listener->OnWindowVisible(true);
                        break;
                    // Window has gained mouse focus
                    case SDL_EVENT_WINDOW_MOUSE_ENTER:
                        break;
                    // Window has lost mouse focus
                    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                        break;
                    // Window has gained keyboard focus
                    case SDL_EVENT_WINDOW_FOCUS_GAINED:
                        IgnoreFalseMouseMotionHack = true;
                        break;
                    // Window has lost keyboard focus
                    case SDL_EVENT_WINDOW_FOCUS_LOST:
                        break;
                    // The window manager requests that the window be closed
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        break;
                    // Window had a hit test that wasn't SDL_HITTEST_NORMAL.
                    case SDL_EVENT_WINDOW_HIT_TEST:
                        break;
                }
            }
            continue;
        }

        // Handle general events
        switch (event.type)
        {
            // User-requested quit
            case SDL_EVENT_QUIT:
                listener->OnCloseEvent();
                break;

            // The application is being terminated by the OS
            // Called on iOS in applicationWillTerminate()
            // Called on Android in onDestroy()
            case SDL_EVENT_TERMINATING:
                LOG("PollEvent: Terminating\n");
                break;

            // The application is low on memory, free memory if possible.
            // Called on iOS in applicationDidReceiveMemoryWarning()
            // Called on Android in onLowMemory()
            case SDL_EVENT_LOW_MEMORY:
                LOG("PollEvent: Low memory\n");
                break;

            // The application is about to enter the background
            // Called on iOS in applicationWillResignActive()
            // Called on Android in onPause()
            case SDL_EVENT_WILL_ENTER_BACKGROUND:
                LOG("PollEvent: Will enter background\n");
                break;

            // The application did enter the background and may not get CPU for some time
            // Called on iOS in applicationDidEnterBackground()
            // Called on Android in onPause()
            case SDL_EVENT_DID_ENTER_BACKGROUND:
                LOG("PollEvent: Did enter background\n");
                break;

            // The application is about to enter the foreground
            // Called on iOS in applicationWillEnterForeground()
            // Called on Android in onResume()
            case SDL_EVENT_WILL_ENTER_FOREGROUND:
                LOG("PollEvent: Will enter foreground\n");
                break;

            // The application is now interactive
            // Called on iOS in applicationDidBecomeActive()
            // Called on Android in onResume()
            case SDL_EVENT_DID_ENTER_FOREGROUND:
                LOG("PollEvent: Did enter foreground\n");
                break;

            // Display orientation has changed to data1
            case SDL_EVENT_DISPLAY_ORIENTATION:
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
                break;
            // Display has been added to the system
            case SDL_EVENT_DISPLAY_ADDED:
                break;
            // Display has been removed from the system
            case SDL_EVENT_DISPLAY_REMOVED:
                break;
            // Display has changed position
            case SDL_EVENT_DISPLAY_MOVED:
                break;
            // Display has changed desktop mode
            case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
                break;
            // Display has changed current mode
            case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
                break;
            // Display has changed content scale
            case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
                break;

            // System specific event (Removed in SDL3, migration note: "You can use SDL_SetWindowsMessageHook() and SDL_SetX11EventHook() to watch and modify system events before SDL sees them.")
            //case SDL_SYSWMEVENT:
            //    break;

            // Key pressed/released
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                if (m_ShouldGenerateInputEvents)
                {
                    KeyEvent keyEvent;
                    keyEvent.Key      = SDLKeyMappings[event.key.scancode];
                    keyEvent.Scancode = event.key.scancode;
                    keyEvent.Action   = (event.key.state == SDL_PRESSED) ? (event.key.repeat ? InputAction::Repeat : InputAction::Pressed) : InputAction::Released;
                    keyEvent.ModMask  = FromKeymodSDL(event.key.mod);
                    if (keyEvent.Key != InvalidKey)
                        listener->OnKeyEvent(keyEvent);
                }
                break;
            }

            // Keyboard text editing (composition)
            case SDL_EVENT_TEXT_EDITING:
                break;

            // Keyboard text input
            case SDL_EVENT_TEXT_INPUT: {
                if (m_ShouldGenerateInputEvents)
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
                        listener->OnCharEvent(charEvent);
                    }
                }
                break;
            }

            // Keymap changed due to a system event such as an
            // input language or keyboard layout change.
            case SDL_EVENT_KEYMAP_CHANGED:
                break;

            // Mouse moved
            case SDL_EVENT_MOUSE_MOTION: {
                if (!IgnoreFalseMouseMotionHack && m_ShouldGenerateInputEvents)
                {
                    MouseMoveEvent moveEvent;
                    moveEvent.X = event.motion.xrel;
                    moveEvent.Y = -event.motion.yrel;
                    listener->OnMouseMoveEvent(moveEvent);
                }
                IgnoreFalseMouseMotionHack = false;
                break;
            }

            // Mouse button pressed/released
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (m_ShouldGenerateInputEvents)
                {
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
                    mouseEvent.Action  = event.button.state == SDL_PRESSED ? InputAction::Pressed : InputAction::Released;
                    mouseEvent.ModMask = FromKeymodSDL(SDL_GetModState());

                    if (mouseEvent.Button >= VirtualKey::MouseLeftBtn && mouseEvent.Button <= VirtualKey::Mouse8)
                    {
                        listener->OnMouseButtonEvent(mouseEvent);
                    }
                }
                break;
            }

            // Mouse wheel motion
            case SDL_EVENT_MOUSE_WHEEL: {
                if (m_ShouldGenerateInputEvents)
                {
                    MouseWheelEvent wheelEvent;
                    wheelEvent.WheelX = event.wheel.x;
                    wheelEvent.WheelY = event.wheel.y;
                    listener->OnMouseWheelEvent(wheelEvent);

                    MouseButtonEvent mouseEvent;
                    mouseEvent.ModMask = FromKeymodSDL(SDL_GetModState());

                    if (wheelEvent.WheelX < 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelLeft;

                        mouseEvent.Action = InputAction::Pressed;
                        listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        listener->OnMouseButtonEvent(mouseEvent);
                    }
                    else if (wheelEvent.WheelX > 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelRight;

                        mouseEvent.Action = InputAction::Pressed;
                        listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        listener->OnMouseButtonEvent(mouseEvent);
                    }
                    if (wheelEvent.WheelY < 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelDown;

                        mouseEvent.Action = InputAction::Pressed;
                        listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        listener->OnMouseButtonEvent(mouseEvent);
                    }
                    else if (wheelEvent.WheelY > 0.0)
                    {
                        mouseEvent.Button = VirtualKey::MouseWheelUp;

                        mouseEvent.Action = InputAction::Pressed;
                        listener->OnMouseButtonEvent(mouseEvent);

                        mouseEvent.Action = InputAction::Released;
                        listener->OnMouseButtonEvent(mouseEvent);
                    }
                }
                break;
            }

            // Joystick axis motion
            case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                break;

            // Joystick trackball motion
            case SDL_EVENT_JOYSTICK_BALL_MOTION:
                break;

            // Joystick hat position change
            case SDL_EVENT_JOYSTICK_HAT_MOTION:
                break;

            // Joystick button pressed/released
            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
            case SDL_EVENT_JOYSTICK_BUTTON_UP:
                break;

            // A new joystick has been inserted into the system
            case SDL_EVENT_JOYSTICK_ADDED:
                break;

            // An opened joystick has been removed
            case SDL_EVENT_JOYSTICK_REMOVED:
                break;

            // Game controller axis motion
            case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
                if (m_ShouldGenerateInputEvents)
                {
                    float deadzone = Math::Clamp(in_StickDeadZone.GetFloat(), 0.0f, 0.999f);
                    float value = static_cast<float>(event.gaxis.value) / 32767;

                    value = value > 0.0f ? Math::Max(0.0f, value - deadzone)
                                         : Math::Min(0.0f, value + deadzone);

                    GamepadAxisMotionEvent gamepadEvent;
                    gamepadEvent.GamepadID = event.gaxis.which;
                    gamepadEvent.AssignedPlayerIndex = SDL_GetGamepadPlayerIndex(SDL_GetGamepadFromID(event.gaxis.which));
                    gamepadEvent.Axis = GamepadAxis(event.gaxis.axis);
                    gamepadEvent.Value = Math::Clamp(value / (1.0f - deadzone), -1.0f, 1.0f);

                    // Invert Y axis
                    if (gamepadEvent.Value != 0.0f && (gamepadEvent.Axis == GamepadAxis::LeftY || gamepadEvent.Axis == GamepadAxis::RightY))
                    {
                        gamepadEvent.Value = -gamepadEvent.Value;
                    }

                    // Fix player index
                    if (gamepadEvent.AssignedPlayerIndex == -1)
                    {
                        auto it = m_GamepadIDToPlayerIndex.Find(gamepadEvent.GamepadID);
                        if (it != m_GamepadIDToPlayerIndex.End())
                        {
                            gamepadEvent.AssignedPlayerIndex = it->second;
                        }
                    }
                    else
                    {
                        m_GamepadIDToPlayerIndex[gamepadEvent.GamepadID] = gamepadEvent.AssignedPlayerIndex;
                    }

                    listener->OnGamepadAxisMotionEvent(gamepadEvent);
                }
                break;
            }

            // Game controller button pressed / released
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP: {
                if (m_ShouldGenerateInputEvents)
                {
                    GamepadKeyEvent gamepadEvent;
                    gamepadEvent.GamepadID = event.gbutton.which;
                    gamepadEvent.AssignedPlayerIndex = SDL_GetGamepadPlayerIndex(SDL_GetGamepadFromID(event.gbutton.which));
                    gamepadEvent.Key = GamepadKey(event.gbutton.button);
                    gamepadEvent.Action = (event.gbutton.state == SDL_PRESSED) ? InputAction::Pressed : InputAction::Released;

                    // Fix player index
                    if (gamepadEvent.AssignedPlayerIndex == -1)
                    {
                        auto it = m_GamepadIDToPlayerIndex.Find(gamepadEvent.GamepadID);
                        if (it != m_GamepadIDToPlayerIndex.End())
                        {
                            gamepadEvent.AssignedPlayerIndex = it->second;
                        }
                    }
                    else
                    {
                        m_GamepadIDToPlayerIndex[gamepadEvent.GamepadID] = gamepadEvent.AssignedPlayerIndex;
                    }

                    listener->OnGamepadButtonEvent(gamepadEvent);
                }
                break;
            }

            // A new Game controller has been inserted into the system
            case SDL_EVENT_GAMEPAD_ADDED: {
                SDL_Gamepad* gamepad = SDL_OpenGamepad(event.gdevice.which);
                LOG("Input device added: {}\n", SDL_GetGamepadName(gamepad));
                break;
            }

            // An opened Game controller has been removed
            case SDL_EVENT_GAMEPAD_REMOVED: {
                SDL_Gamepad* gamepad = SDL_GetGamepadFromID(event.gdevice.which);

                if (m_ShouldGenerateInputEvents)
                {
                    int assignedPlayerIndex = SDL_GetGamepadPlayerIndex(gamepad);

                    // Try to restore player index from last event
                    auto it = m_GamepadIDToPlayerIndex.Find(event.gdevice.which);
                    if (it != m_GamepadIDToPlayerIndex.End())
                    {
                        if (assignedPlayerIndex == -1)
                            assignedPlayerIndex = it->second;
                        m_GamepadIDToPlayerIndex.Erase(it);
                    }

                    if (assignedPlayerIndex != -1)
                    {
                        {
                            GamepadKeyEvent gamepadEvent;
                            gamepadEvent.GamepadID = event.gdevice.which;
                            gamepadEvent.AssignedPlayerIndex = assignedPlayerIndex;
                            gamepadEvent.Action = InputAction::Released;

                            for (int button = 0; button < SDL_GAMEPAD_BUTTON_MAX; ++button)
                            {                   
                                if (SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)button) == SDL_PRESSED)
                                {
                                    gamepadEvent.Key = GamepadKey(button);                            
                                    listener->OnGamepadButtonEvent(gamepadEvent);
                                }
                            }
                        }

                        {
                            GamepadAxisMotionEvent gamepadEvent;
                            gamepadEvent.GamepadID = event.gdevice.which;
                            gamepadEvent.AssignedPlayerIndex = assignedPlayerIndex;
                            gamepadEvent.Value = 0;
                            for (int axis = 0; axis < SDL_GAMEPAD_AXIS_MAX; ++axis)
                            {                   
                                if (SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)axis))
                                {
                                    gamepadEvent.Axis = GamepadAxis(axis);
                                    listener->OnGamepadAxisMotionEvent(gamepadEvent);
                                }
                            }
                        }
                    }
                }
                LOG("Input device removed: {}\n", SDL_GetGamepadName(gamepad));
                SDL_CloseGamepad(gamepad);                
                break;
            }

            // The controller mapping was updated
            case SDL_EVENT_GAMEPAD_REMAPPED:
                LOG("PollEvent: Gamepad device mapped\n");
                break;

            // Touch events
            case SDL_EVENT_FINGER_DOWN:
                LOG("PollEvent: Touch press\n");
                break;
            case SDL_EVENT_FINGER_UP:
                LOG("PollEvent: Touch release\n");
                break;
            case SDL_EVENT_FINGER_MOTION:
                LOG("PollEvent: Touch move\n");
                break;

            // The clipboard changed
            case SDL_EVENT_CLIPBOARD_UPDATE:
                LOG("PollEvent: Clipboard update\n");
                break;

            // The system requests a file open
            case SDL_EVENT_DROP_FILE:
                LOG("PollEvent: Drop file\n");
                break;
            // text/plain drag-and-drop event
            case SDL_EVENT_DROP_TEXT:
                LOG("PollEvent: Drop text\n");
                break;
            // A new set of drops is beginning (NULL filename)
            case SDL_EVENT_DROP_BEGIN:
                LOG("PollEvent: Drop begin\n");
                break;
            // Current set of drops is now complete (NULL filename)
            case SDL_EVENT_DROP_COMPLETE:
                LOG("PollEvent: Drop complete\n");
                break;

            // A new audio device is available
            case SDL_EVENT_AUDIO_DEVICE_ADDED:
                LOG("Audio {} device added: {}\n", event.adevice.recording ? "recording" : "playback", SDL_GetAudioDeviceName(event.adevice.which));
                break;
            // An audio device has been removed
            case SDL_EVENT_AUDIO_DEVICE_REMOVED:
                LOG("Audio {} device removed: {}\n", event.adevice.recording ? "recording" : "playback", SDL_GetAudioDeviceName(event.adevice.which));
                break;

            // A sensor was updated
            case SDL_EVENT_SENSOR_UPDATE:
                LOG("PollEvent: Sensor update\n");
                break;

            // The render targets have been reset and their contents need to be updated
            case SDL_EVENT_RENDER_TARGETS_RESET:
                LOG("PollEvent: Render targets reset\n");
                break;
            case SDL_EVENT_RENDER_DEVICE_RESET:
                LOG("PollEvent: Render device reset\n");
                break;
        }
    }
}

HK_NAMESPACE_END
