/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "WindowManager.h"
#include "MonitorManager.h"
#include "RuntimeEvents.h"

#include <GLFW/glfw3.h>

#define MAX_INPUT_EVENTS  200

#define MOUSE_LOST (-999999999999.0)

AWindowManager & GWindowManager = AWindowManager::Inst();

static const int GLFWCursorMode[2] = { GLFW_CURSOR_NORMAL, GLFW_CURSOR_DISABLED };

static double MousePositionX = MOUSE_LOST;
static double MousePositionY = MOUSE_LOST;

static unsigned short VidWidth;
static unsigned short VidHeight;
static unsigned short VidPhysicalMonitor;
static byte VidRefreshRate;
static bool VidFullscreen;
static char VidRenderBackend[32];
static byte WinOpacity;
static bool WinDecorated;
static bool WinAutoIconify;
static bool WinFloating;
static char WinTitle[32];
static int WinPositionX;
static int WinPositionY;
static bool WinDisabledCursor;

static bool bSetRenderBackend = false;
static bool bSetVideoMode = false;
static bool bSetWindowDefs = false;
static bool bSetWindowPos = false;
static bool bSetFocus = false;

static bool bIsWindowFocused = false;
static bool bIsWindowIconified = false;
static bool bIsWindowVisible = false;

static GLFWwindow * Wnd = nullptr;

static int PressedKeys[GLFW_KEY_LAST+1];
static bool PressedMouseButtons[GLFW_MOUSE_BUTTON_LAST+1];

static void SendChangedVideoModeEvent();

static void KeyCallback( GLFWwindow * _Window, int _Key, int _Scancode, int _Action, int _Mods ) {
    if ( _Key < 0 || _Key > GLFW_KEY_LAST ) {
        return;
    }

    if ( GInputEventsCount >= MAX_INPUT_EVENTS ) {
        GLogger.Printf( "Ignoring stalled keys\n" );
        return;
    }

    //if ( _Action == GLFW_REPEAT ) {
    //    return;
    //}

    if ( _Action == GLFW_RELEASE && !PressedKeys[_Key] ) {
        return;
    }

    if ( _Action == GLFW_PRESS && PressedKeys[_Key] ) {
        return;
    }

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_KeyEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    SKeyEvent & keyEvent = event->Data.KeyEvent;
    keyEvent.Key = _Key;
    keyEvent.Scancode = _Scancode;
    keyEvent.ModMask = _Mods;
    keyEvent.Action = _Action;
    PressedKeys[_Key] = ( _Action == GLFW_RELEASE ) ? 0 : _Scancode + 1;
    GInputEventsCount++;
}

static void MouseButtonCallback( GLFWwindow * _Window, int _Button, int _Action, int _Mods ) {
    AN_ASSERT( _Action != GLFW_REPEAT ); // Is GLFW produce GLFW_REPEAT event for mouse buttons?

    if ( GInputEventsCount >= MAX_INPUT_EVENTS ) {
        GLogger.Printf( "Ignoring stalled buttons\n" );
        return;
    }

    if ( _Action == (int)PressedMouseButtons[_Button] ) {
        return;
    }

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_MouseButtonEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    SMouseButtonEvent & mouseEvent = event->Data.MouseButtonEvent;
    mouseEvent.Button = _Button;
    mouseEvent.ModMask = _Mods;
    mouseEvent.Action = _Action;
    PressedMouseButtons[_Button] = _Action;
    GInputEventsCount++;
}

static void CursorPosCallback( GLFWwindow * _Window, double _MouseX, double _MouseY ) {
    if ( !WinDisabledCursor || !bIsWindowFocused ) {
        return;
    }

    if ( GInputEventsCount >= MAX_INPUT_EVENTS ) {
        GLogger.Printf( "Ignoring stalled mouse move\n" );
        return;
    }

    if ( MousePositionX <= MOUSE_LOST ) {
        MousePositionX = _MouseX;
        MousePositionY = _MouseY;
        return;
    }

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_MouseMoveEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    SMouseMoveEvent & mouseEvent = event->Data.MouseMoveEvent;
    mouseEvent.X = static_cast< float >( _MouseX - MousePositionX );
    mouseEvent.Y = static_cast< float >( MousePositionY - _MouseY );
    MousePositionX = _MouseX;
    MousePositionY = _MouseY;
    GInputEventsCount++;
}

static void WindowPosCallback( GLFWwindow * _Window, int _X, int _Y ) {
    if ( !VidFullscreen ) {
        WinPositionX = _X;
        WinPositionY = _Y;

        SEvent * event = GRuntimeEvents.Push();
        event->Type = ET_WindowPosEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        SWindowPosEvent & windowPosEvent = event->Data.WindowPosEvent;
        windowPosEvent.PositionX = _X;
        windowPosEvent.PositionY = _Y;
    }
}

static void WindowSizeCallback( GLFWwindow * _Window, int _Width, int _Height ) {
    VidWidth = _Width;
    VidHeight = _Height;
}

static void WindowCloseCallback( GLFWwindow * ) {
    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_CloseEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
}

static void WindowRefreshCallback( GLFWwindow * ) {
}

static void WindowFocusCallback( GLFWwindow * _Window, int _Focused ) {
    bIsWindowFocused = !!_Focused;

    if ( bIsWindowFocused && WinDisabledCursor ) {
        //glfwGetCursorPos( Wnd, &MousePositionX, &MousePositionY );
        MousePositionX = MOUSE_LOST;
    }

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_FocusEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    event->Data.FocusEvent.bFocused = bIsWindowFocused;
}

static void WindowIconifyCallback( GLFWwindow * _Window, int _Iconified ) {
    bIsWindowIconified = !!_Iconified;
}

static void FramebufferSizeCallback( GLFWwindow *, int, int ) {
}

static void CharCallback( GLFWwindow *, unsigned int ) {
}

static void CharModsCallback( GLFWwindow * _Window, unsigned int _UnicodeCharacter, int _Mods ) {
    if ( _UnicodeCharacter > 0xffff ) {
        return;
    }

    if ( GInputEventsCount >= MAX_INPUT_EVENTS ) {
        GLogger.Printf( "Ignoring stalled chars\n" );
        return;
    }

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_CharEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    SCharEvent & charEvent = event->Data.CharEvent;
    charEvent.UnicodeCharacter = _UnicodeCharacter;
    charEvent.ModMask = _Mods;
}

static void CursorEnterCallback( GLFWwindow * _Window, int _Entered ) {
//    if ( _Entered && WinDisabledCursor ) {
//        glfwGetCursorPos( Wnd, &display->MousePositionX, &display->MousePositionY );
//    }
}

static void ScrollCallback( GLFWwindow * _Window, double _WheelX, double _WheelY ) {
    if ( GInputEventsCount >= MAX_INPUT_EVENTS ) {
        //GLogger.Printf( "Ignoring stalled wheel\n" );
        return;
    }

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_MouseWheelEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    SMouseWheelEvent & mouseWheelEvent = event->Data.MouseWheelEvent;
    mouseWheelEvent.WheelX = _WheelX;
    mouseWheelEvent.WheelY = _WheelY;
    GInputEventsCount++;
}

static void DropCallback( GLFWwindow *, int, const char** ) {
}

static void CreateDisplays() {
    //GRenderBackend = FindRenderBackend( VidRenderBackend );
    //if ( !GRenderBackend ) {
    //    CriticalError( "Unknown rendering backend \"%s\"\n", VidRenderBackend );
    //}

    GRenderBackend->PreInit();

    glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
    glfwWindowHint( GLFW_FOCUS_ON_SHOW, GLFW_TRUE );
    glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE );
    glfwWindowHint( GLFW_CENTER_CURSOR, GLFW_TRUE );
    glfwWindowHint( GLFW_FOCUSED, GLFW_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    glfwWindowHint( GLFW_DECORATED, WinDecorated );
    glfwWindowHint( GLFW_AUTO_ICONIFY, WinAutoIconify );
    glfwWindowHint( GLFW_FLOATING, WinFloating );
    glfwWindowHint( GLFW_MAXIMIZED, GLFW_FALSE );
    glfwWindowHint( GLFW_REFRESH_RATE, VidRefreshRate );

    GLFWmonitor * monitor = nullptr;
    if ( VidFullscreen ) {
        APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
        SPhysicalMonitor * physMonitor = physicalMonitors[ VidPhysicalMonitor ];
        monitor = ( GLFWmonitor * )physMonitor->Internal.Pointer;
    }

    Wnd = glfwCreateWindow( VidWidth, VidHeight, WinTitle, monitor, nullptr );
    if ( !Wnd ) {
        CriticalError( "Failed to initialize game display\n" );
    }

    if ( monitor ) {
        const GLFWvidmode * videoMode = glfwGetVideoMode( monitor );

        // Store real refresh rate
        VidRefreshRate = videoMode->refreshRate;
    }

    VidFullscreen = glfwGetWindowMonitor( Wnd ) != nullptr;

    // Disable cursor to discard any mouse moving
    bool bDisabledCursor = WinDisabledCursor;
    WinDisabledCursor = false;

    //glfwSetWindowUserPointer( Wnd, display );
    //glfwSetWindowSizeLimits( Wnd, MinWidth, MinHeight, MaxWidth, MaxHeight );
    glfwSetWindowPos( Wnd, WinPositionX, WinPositionY );
    glfwSetWindowOpacity( Wnd, WinOpacity / 255.0f );
    glfwSetInputMode( Wnd, GLFW_STICKY_KEYS, GLFW_FALSE );
    glfwSetInputMode( Wnd, GLFW_STICKY_MOUSE_BUTTONS, GLFW_FALSE );
    glfwSetInputMode( Wnd, GLFW_LOCK_KEY_MODS, GLFW_TRUE );
    glfwSetKeyCallback( Wnd, KeyCallback );
    glfwSetMouseButtonCallback( Wnd, MouseButtonCallback );
    glfwSetCursorPosCallback( Wnd, CursorPosCallback );
    glfwSetWindowPosCallback( Wnd, WindowPosCallback );
    glfwSetWindowSizeCallback( Wnd, WindowSizeCallback );
    glfwSetWindowCloseCallback( Wnd, WindowCloseCallback );
    glfwSetWindowRefreshCallback( Wnd, WindowRefreshCallback );
    glfwSetWindowFocusCallback( Wnd, WindowFocusCallback );
    glfwSetWindowIconifyCallback( Wnd, WindowIconifyCallback );
    glfwSetFramebufferSizeCallback( Wnd, FramebufferSizeCallback );
    glfwSetCharCallback( Wnd, CharCallback );
    glfwSetCharModsCallback( Wnd, CharModsCallback );
    glfwSetCursorEnterCallback( Wnd, CursorEnterCallback );
    glfwSetScrollCallback( Wnd, ScrollCallback );
    glfwSetDropCallback( Wnd, DropCallback );

    glfwShowWindow( Wnd );

    // Set actual cursor state
    glfwSetInputMode( Wnd, GLFW_CURSOR, GLFWCursorMode[ bDisabledCursor ] );
    if ( bDisabledCursor ) {
        //glfwGetCursorPos( Wnd, &MousePositionX, &MousePositionY );
        //glfwSetCursorPos( Wnd, MousePositionX, MousePositionY );
        MousePositionX = MOUSE_LOST;
    }
    WinDisabledCursor = bDisabledCursor;

    GRenderBackend->Initialize( Wnd );

    bSetRenderBackend = false;
    bSetVideoMode = false;
    bSetWindowDefs = false;
    bSetWindowPos = false;
    bSetFocus = false;

    SendChangedVideoModeEvent();
}

static void DestroyDisplays() {
    GRenderBackend->Deinitialize();

    glfwDestroyWindow( Wnd );

    // Unpress all pressed keys
    for ( int i = 0 ; i <= GLFW_KEY_LAST ; i++ ) {
        if ( PressedKeys[i] ) {
            KeyCallback( nullptr, i, PressedKeys[i] - 1, IE_Release, 0 );
        }
    }
    for ( int i = 0 ; i <= GLFW_MOUSE_BUTTON_LAST ; i++ ) {
        if ( PressedMouseButtons[i] ) {
            MouseButtonCallback( nullptr, i, IE_Release, 0 );
        }
    }
}

AWindowManager::AWindowManager() {

}

void AWindowManager::Initialize() {

    //REGISTER_RENDER_BACKEND( OpenGLBackend );
    //REGISTER_RENDER_BACKEND( VulkanBackend );
    //REGISTER_RENDER_BACKEND( NullBackend );

    // TODO: load this from config:
    VidWidth = 640;
    VidHeight = 480;
    VidPhysicalMonitor = 0;
    VidRefreshRate = 120;
    VidFullscreen = false;
    AString::CopySafe( VidRenderBackend, sizeof( VidRenderBackend ), "OpenGL 4.5" );
    WinOpacity = 255;
    WinDecorated = true;
    WinAutoIconify = false;
    WinFloating = false;
    AString::CopySafe( WinTitle, sizeof( WinTitle ), "Game" );
    WinPositionX = 100;
    WinPositionY = 100;
    WinDisabledCursor = false;

    //for ( ARenderBackend const * backend = GetRenderBackends() ; backend ; backend = backend->Next ) {
    //    GLogger.Printf( "Found renderer backend: %s\n", backend->Name );
    //}

    memset( PressedKeys, 0, sizeof( PressedKeys ) );
    memset( PressedMouseButtons, 0, sizeof( PressedMouseButtons ) );

    CreateDisplays();
}

void AWindowManager::Deinitialize() {
    DestroyDisplays();
}

static void ProcessEvent( SEvent const & _Event ) {
    switch ( _Event.Type ) {
    case ET_SetVideoModeEvent:
        VidWidth = Math::Max< unsigned short >( 1, _Event.Data.SetVideoModeEvent.Width );
        VidHeight = Math::Max< unsigned short >( 1, _Event.Data.SetVideoModeEvent.Height );
        VidPhysicalMonitor = Int( _Event.Data.SetVideoModeEvent.PhysicalMonitor ).Clamp( 0, GMonitorManager.GetMonitors().Size() - 1 );
        VidRefreshRate = _Event.Data.SetVideoModeEvent.RefreshRate;
        VidFullscreen = _Event.Data.SetVideoModeEvent.bFullscreen;

        if ( AString::Icmp( VidRenderBackend, _Event.Data.SetVideoModeEvent.Backend ) ) {
            AString::CopySafe( VidRenderBackend, sizeof( VidRenderBackend ), _Event.Data.SetVideoModeEvent.Backend );
            bSetRenderBackend = true;
        }

        bSetVideoMode = true;
        break;
    case ET_SetWindowDefsEvent:
        WinOpacity = _Event.Data.SetWindowDefsEvent.Opacity;
        WinDecorated = _Event.Data.SetWindowDefsEvent.bDecorated;
        WinAutoIconify = _Event.Data.SetWindowDefsEvent.bAutoIconify;
        WinFloating = _Event.Data.SetWindowDefsEvent.bFloating;
        AString::CopySafe( WinTitle, sizeof( WinTitle ), _Event.Data.SetWindowDefsEvent.Title );
        bSetWindowDefs = true;
        break;
    case ET_SetWindowPosEvent:
        WinPositionX = _Event.Data.SetWindowPosEvent.PositionX;
        WinPositionY = _Event.Data.SetWindowPosEvent.PositionY;
        bSetWindowPos = true;
        break;
    case ET_SetInputFocusEvent:
        bSetFocus = true;
        break;
//    case ET_SetRenderFeaturesEvent:
//        VSyncMode = Int( _Event.Data.SetRenderFeaturesEvent.VSyncMode ).Clamp( VSync_Disabled, VSync_Half );
//        {
//            FRenderFeatures features;
//            features.VSyncMode = VSyncMode;
//            GRenderBackend->SetRenderFeatures( features );
//        }
//        break;
    case ET_SetCursorModeEvent:
        if ( WinDisabledCursor != _Event.Data.SetCursorModeEvent.bDisabledCursor ) {
            WinDisabledCursor = _Event.Data.SetCursorModeEvent.bDisabledCursor;
            glfwSetInputMode( Wnd, GLFW_CURSOR, GLFWCursorMode[ WinDisabledCursor ] );
            if ( WinDisabledCursor ) {
                //glfwGetCursorPos( Wnd, &MousePositionX, &MousePositionY );
                MousePositionX = MOUSE_LOST;
            }
        }

        break;
    }
}

static void SendChangedVideoModeEvent() {
    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_ChangedVideoModeEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    SChangedVideoModeEvent & data = event->Data.ChangedVideoModeEvent;
    data.Width = VidWidth;
    data.Height = VidHeight;
    data.PhysicalMonitor = VidPhysicalMonitor;
    data.RefreshRate = VidRefreshRate;
    data.bFullscreen = VidFullscreen;
    AString::CopySafe( data.Backend, sizeof( data.Backend ), GRenderBackend->GetName() );
}

void AWindowManager::Update( AEventQueue & _EventQueue ) {
    SEvent * event;
    while ( nullptr != ( event = _EventQueue.Pop() ) ) {
        ProcessEvent( *event );
    }

    if ( bSetRenderBackend ) {
        bSetRenderBackend = false;

        DestroyDisplays();
        CreateDisplays();
    }

    if ( bSetVideoMode ) {
        bSetVideoMode = false;

        if ( VidFullscreen ) {
            APhysicalMonitorArray const & physicalMonitors = GMonitorManager.GetMonitors();
            SPhysicalMonitor * physMonitor = physicalMonitors[ VidPhysicalMonitor ];
            GLFWmonitor * monitor = ( GLFWmonitor * )physMonitor->Internal.Pointer;
            bool bIsMonitorConnected = monitor != nullptr;

            glfwSetWindowMonitor( Wnd,
                                  monitor,                      // switch to fullscreen mode
                                  WinPositionX, WinPositionY,   // position is ignored
                                  VidWidth,
                                  VidHeight,
                                  VidRefreshRate );

            bSetWindowPos = false;

            if ( bIsMonitorConnected ) {
                // Store real refresh rate
                const GLFWvidmode * videoMode = glfwGetVideoMode( monitor );
                VidRefreshRate = videoMode->refreshRate;

                glfwFocusWindow( Wnd );
                bSetFocus = false;
            }

            if ( !glfwGetWindowMonitor( Wnd ) ) {
                VidFullscreen = false;
            }

        } else {
            glfwSetWindowMonitor( Wnd,
                nullptr,  // switch to windowed mode
                WinPositionX, WinPositionY,
                VidWidth,
                VidHeight,
                0         // refresh rate is ignored
            );

            bSetWindowPos = false;

            glfwFocusWindow( Wnd );
            bSetFocus = false;

            if ( !glfwGetWindowAttrib( Wnd, GLFW_VISIBLE ) ) {
                glfwShowWindow( Wnd );
            }
        }

        MousePositionX = MOUSE_LOST;

        SendChangedVideoModeEvent();
    }

    if ( bSetWindowDefs ) {
        bSetWindowDefs = false;

        glfwSetWindowOpacity( Wnd, WinOpacity / 255.0f );
        glfwSetWindowAttrib( Wnd, GLFW_DECORATED, WinDecorated );
        glfwSetWindowAttrib( Wnd, GLFW_AUTO_ICONIFY, WinAutoIconify );
        glfwSetWindowAttrib( Wnd, GLFW_FLOATING, WinFloating );
        glfwSetWindowTitle( Wnd, WinTitle );
    }

    if ( bSetWindowPos ) {
        bSetWindowPos = false;

        if ( !VidFullscreen ) {
            glfwSetWindowPos( Wnd, WinPositionX, WinPositionY );
        }
    }

    if ( bSetFocus ) {
        bSetFocus = false;

        glfwFocusWindow( Wnd );
    }

    bool bCheck;

    bCheck = bIsWindowVisible;
    bIsWindowVisible = glfwGetWindowAttrib( Wnd, GLFW_VISIBLE );
    if ( bIsWindowVisible != bCheck ) {
        event = GRuntimeEvents.Push();
        event->Type = ET_VisibleEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        SVisibleEvent & data = event->Data.VisibleEvent;
        data.bVisible = bIsWindowVisible;
    }

    //bIsWindowMaximized = !!glfwGetWindowAttrib( Wnd, GLFW_MAXIMIZED );
    //bIsWindowHovered = glfwGetWindowAttrib( Wnd, GLFW_HOVERED );
}
