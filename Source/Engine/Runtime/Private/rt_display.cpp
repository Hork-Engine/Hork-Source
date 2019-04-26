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

#include "rt_display.h"
#include "rt_event.h"
#include "rt_monitor.h"
#include "rt_main.h"

#include <Engine/Core/Public/CriticalError.h>
#include <Engine/Runtime/Public/ImportExport.h>

#include <GLFW/glfw3.h>

#define MAX_KEY_STALLED_TIME    3000000     // microseconds
#define MAX_MOUSE_STALLED_TIME  3000000     // microseconds

static const int GLFWCursorMode[2] = { GLFW_CURSOR_NORMAL, GLFW_CURSOR_DISABLED };

#define MOUSE_LOST (-999999999999.0)

static double MousePositionX = MOUSE_LOST;
static double MousePositionY = MOUSE_LOST;

static unsigned short rt_Width;
static unsigned short rt_Height;
static unsigned short rt_PhysicalMonitor;
static byte rt_RefreshRate;
static bool rt_Fullscreen;
static char rt_Backend[32];
static byte rt_Opacity;
static bool rt_Decorated;
static bool rt_AutoIconify;
static bool rt_Floating;
static char rt_Title[32];
static int rt_PositionX;
static int rt_PositionY;
static int rt_VSyncMode;
static bool rt_DisabledCursor;

static bool bSetRenderBackend = false;
static bool bSetVideoMode = false;
static bool bSetWindowDefs = false;
static bool bSetWindowPos = false;
static bool bSetFocus = false;

static bool bIsWindowFocused = false;
static bool bIsWindowIconified = false;
static bool bIsWindowVisible = false;

static GLFWwindow * Wnd = nullptr;

FRenderBackendFeatures rt_RenderFeatures;
int rt_InputEventCount = 0;

static void SendChangedVideoModeEvent();

static int PressedKeys[GLFW_KEY_LAST+1];
static bool PressedMouseButtons[GLFW_MOUSE_BUTTON_LAST+1];

static FString ClipboardPaste;
static FString ClipboardCopy;

static void KeyCallback( GLFWwindow * _Window, int _Key, int _Scancode, int _Action, int _Mods ) {
    AN_Assert( _Key >= 0 || _Key <= GLFW_KEY_LAST );

    if ( rt_StalledTime >= MAX_KEY_STALLED_TIME && rt_InputEventCount > 200 ) {
        //GLogger.Printf( "Ignoring stalled keys\n" );
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

    if ( _Key == GLFW_KEY_V && ( _Mods & GLFW_MOD_CONTROL ) ) {
        ClipboardPaste = glfwGetClipboardString( NULL );
    }

    FEvent * event = rt_SendEvent();
    event->Type = ET_KeyEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    FKeyEvent & keyEvent = event->Data.KeyEvent;
    keyEvent.Key = _Key;
    keyEvent.Scancode = _Scancode;
    keyEvent.ModMask = _Mods;
    keyEvent.Action = _Action;
    PressedKeys[_Key] = ( _Action == GLFW_RELEASE ) ? 0 : _Scancode + 1;
    ++rt_InputEventCount;
}

static void MouseButtonCallback( GLFWwindow * _Window, int _Button, int _Action, int _Mods ) {
    AN_Assert( _Action != GLFW_REPEAT ); // Is GLFW produce GLFW_REPEAT event for mouse buttons?

    if ( rt_StalledTime >= MAX_KEY_STALLED_TIME && rt_InputEventCount > 200 ) {
        //GLogger.Printf( "Ignoring stalled buttons\n" );
        return;
    }

    if ( _Action == (int)PressedMouseButtons[_Button] ) {
        return;
    }

    FEvent * event = rt_SendEvent();
    event->Type = ET_MouseButtonEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    FMouseButtonEvent & mouseEvent = event->Data.MouseButtonEvent;
    mouseEvent.Button = _Button;
    mouseEvent.ModMask = _Mods;
    mouseEvent.Action = _Action;
    PressedMouseButtons[_Button] = _Action;
    ++rt_InputEventCount;
}

static void CursorPosCallback( GLFWwindow * _Window, double _MouseX, double _MouseY ) {
    if ( !rt_DisabledCursor || !bIsWindowFocused ) {
        return;
    }

    if ( rt_StalledTime >= MAX_MOUSE_STALLED_TIME && rt_InputEventCount > 200 ) {
        //GLogger.Printf( "Ignoring stalled mouse move\n" );
        return;
    }

    if ( MousePositionX <= MOUSE_LOST ) {
        MousePositionX = _MouseX;
        MousePositionY = _MouseY;
        return;
    }

    FEvent * event = rt_SendEvent();
    event->Type = ET_MouseMoveEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    FMouseMoveEvent & mouseEvent = event->Data.MouseMoveEvent;
    mouseEvent.X = static_cast< float >( _MouseX - MousePositionX );
    mouseEvent.Y = static_cast< float >( MousePositionY - _MouseY );
    MousePositionX = _MouseX;
    MousePositionY = _MouseY;
    ++rt_InputEventCount;
}

static void WindowPosCallback( GLFWwindow * _Window, int _X, int _Y ) {
    if ( !rt_Fullscreen ) {
        rt_PositionX = _X;
        rt_PositionY = _Y;

        FEvent * event = rt_SendEvent();
        event->Type = ET_WindowPosEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        FWindowPosEvent & windowPosEvent = event->Data.WindowPosEvent;
        windowPosEvent.PositionX = _X;
        windowPosEvent.PositionY = _Y;
    }
}

static void WindowSizeCallback( GLFWwindow * _Window, int _Width, int _Height ) {
    rt_Width = _Width;
    rt_Height = _Height;
}

static void WindowCloseCallback( GLFWwindow * ) {
    FEvent * event = rt_SendEvent();
    event->Type = ET_CloseEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
}

static void WindowRefreshCallback( GLFWwindow * ) {
}

static void WindowFocusCallback( GLFWwindow * _Window, int _Focused ) {
    bIsWindowFocused = !!_Focused;

    if ( bIsWindowFocused && rt_DisabledCursor ) {
        //glfwGetCursorPos( Wnd, &MousePositionX, &MousePositionY );
        MousePositionX = MOUSE_LOST;
    }

    FEvent * event = rt_SendEvent();
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

    if ( rt_StalledTime >= MAX_KEY_STALLED_TIME && rt_InputEventCount > 200 ) {
        //GLogger.Printf( "Ignoring stalled chars\n" );
        return;
    }

    FEvent * event = rt_SendEvent();
    event->Type = ET_CharEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    FCharEvent & charEvent = event->Data.CharEvent;
    charEvent.UnicodeCharacter = _UnicodeCharacter;
    charEvent.ModMask = _Mods;
}

static void CursorEnterCallback( GLFWwindow * _Window, int _Entered ) {
//    if ( _Entered && rt_DisabledCursor ) {
//        glfwGetCursorPos( Wnd, &display->MousePositionX, &display->MousePositionY );
//    }
}

static void ScrollCallback( GLFWwindow * _Window, double _WheelX, double _WheelY ) {
    if ( rt_StalledTime >= MAX_KEY_STALLED_TIME && rt_InputEventCount > 200 ) {
        //GLogger.Printf( "Ignoring stalled wheel\n" );
        return;
    }

    FEvent * event = rt_SendEvent();
    event->Type = ET_MouseWheelEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    FMouseWheelEvent & mouseWheelEvent = event->Data.MouseWheelEvent;
    mouseWheelEvent.WheelX = _WheelX;
    mouseWheelEvent.WheelY = _WheelY;
    ++rt_InputEventCount;
}

static void DropCallback( GLFWwindow *, int, const char** ) {
}

static void CreateDisplays() {
    GRenderBackend = FindRenderBackend( rt_Backend );
    if ( !GRenderBackend ) {
        CriticalError( "Unknown rendering backend \"%s\"\n", rt_Backend );
    }

    GRenderBackend->PreInit();

    glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
    glfwWindowHint( GLFW_FOCUS_ON_SHOW, GLFW_TRUE );
    glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE );
    glfwWindowHint( GLFW_CENTER_CURSOR, GLFW_TRUE );
    glfwWindowHint( GLFW_FOCUSED, GLFW_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    glfwWindowHint( GLFW_DECORATED, rt_Decorated );
    glfwWindowHint( GLFW_AUTO_ICONIFY, rt_AutoIconify );
    glfwWindowHint( GLFW_FLOATING, rt_Floating );
    glfwWindowHint( GLFW_MAXIMIZED, GLFW_FALSE );
    glfwWindowHint( GLFW_REFRESH_RATE, rt_RefreshRate );

    GLFWmonitor * monitor = nullptr;
    if ( rt_Fullscreen ) {
        FPhysicalMonitorArray const & physicalMonitors = rt_GetPhysicalMonitors();
        FPhysicalMonitor * physMonitor = physicalMonitors[ rt_PhysicalMonitor ];
        monitor = ( GLFWmonitor * )physMonitor->Internal.Pointer;
    }

    Wnd = glfwCreateWindow( rt_Width, rt_Height, rt_Title, monitor, nullptr );
    if ( !Wnd ) {
        CriticalError( "Failed to initialize game display\n" );
    }

    if ( monitor ) {
        const GLFWvidmode * videoMode = glfwGetVideoMode( monitor );

        // Store real refresh rate
        rt_RefreshRate = videoMode->refreshRate;
    }

    rt_Fullscreen = glfwGetWindowMonitor( Wnd ) != nullptr;

    // Disable cursor to discard any mouse moving
    bool bDisabledCursor = rt_DisabledCursor;
    rt_DisabledCursor = false;

    //glfwSetWindowUserPointer( Wnd, display );
    //glfwSetWindowSizeLimits( Wnd, MinWidth, MinHeight, MaxWidth, MaxHeight );
    glfwSetWindowPos( Wnd, rt_PositionX, rt_PositionY );
    glfwSetWindowOpacity( Wnd, rt_Opacity / 255.0f );
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
    rt_DisabledCursor = bDisabledCursor;

    GRenderBackend->Initialize( ( void ** )&Wnd, 1, &rt_RenderFeatures );

    FRenderFeatures features;
    features.VSyncMode = rt_VSyncMode;
    GRenderBackend->SetRenderFeatures( features );

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
    int64_t stalledTime = rt_StalledTime;
    rt_StalledTime = 0;
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
    rt_StalledTime = stalledTime;
}

void rt_InitializeDisplays() {

    REGISTER_RENDER_BACKEND( OpenGLBackend );
    REGISTER_RENDER_BACKEND( VulkanBackend );
    REGISTER_RENDER_BACKEND( NullBackend );

    // TODO: load this from config:
    rt_Width = 640;
    rt_Height = 480;
    rt_PhysicalMonitor = 0;
    rt_RefreshRate = 120;
    rt_Fullscreen = false;
    FString::CopySafe( rt_Backend, sizeof( rt_Backend ), "OpenGL 4.5" );
    rt_Opacity = 255;
    rt_Decorated = true;
    rt_AutoIconify = false;
    rt_Floating = false;
    FString::CopySafe( rt_Title, sizeof( rt_Title ), "Game" );
    rt_PositionX = 100;
    rt_PositionY = 100;
    rt_VSyncMode = VSync_Disabled;
    rt_DisabledCursor = false;

    for ( FRenderBackend const * backend = GetRenderBackends() ; backend ; backend = backend->Next ) {
        GLogger.Printf( "Found renderer backend: %s\n", backend->Name );
    }

    memset( PressedKeys, 0, sizeof( PressedKeys ) );
    memset( PressedMouseButtons, 0, sizeof( PressedMouseButtons ) );

    CreateDisplays();
}

void rt_DeinitializeDisplays() {
    DestroyDisplays();

    ClipboardPaste.Free();
    ClipboardCopy.Free();
}

static void ProcessEvent( FEvent const & _Event ) {
    switch ( _Event.Type ) {
    case ET_SetVideoModeEvent:
        rt_Width = FMath::Max< unsigned short >( MIN_DISPLAY_WIDTH, _Event.Data.SetVideoModeEvent.Width );
        rt_Height = FMath::Max< unsigned short >( MIN_DISPLAY_HEIGHT, _Event.Data.SetVideoModeEvent.Height );
        rt_PhysicalMonitor = Int( _Event.Data.SetVideoModeEvent.PhysicalMonitor ).Clamp( 0, rt_GetPhysicalMonitors().Length() - 1 );
        rt_RefreshRate = _Event.Data.SetVideoModeEvent.RefreshRate;
        rt_Fullscreen = _Event.Data.SetVideoModeEvent.bFullscreen;

        if ( FString::Icmp( rt_Backend, _Event.Data.SetVideoModeEvent.Backend ) ) {
            FString::CopySafe( rt_Backend, sizeof( rt_Backend ), _Event.Data.SetVideoModeEvent.Backend );
            bSetRenderBackend = true;
        }

        bSetVideoMode = true;
        break;
    case ET_SetWindowDefsEvent:
        rt_Opacity = _Event.Data.SetWindowDefsEvent.Opacity;
        rt_Decorated = _Event.Data.SetWindowDefsEvent.bDecorated;
        rt_AutoIconify = _Event.Data.SetWindowDefsEvent.bAutoIconify;
        rt_Floating = _Event.Data.SetWindowDefsEvent.bFloating;
        FString::CopySafe( rt_Title, sizeof( rt_Title ), _Event.Data.SetWindowDefsEvent.Title );
        bSetWindowDefs = true;
        break;
    case ET_SetWindowPosEvent:
        rt_PositionX = _Event.Data.SetWindowPosEvent.PositionX;
        rt_PositionY = _Event.Data.SetWindowPosEvent.PositionY;
        bSetWindowPos = true;
        break;
    case ET_SetInputFocusEvent:
        bSetFocus = true;
        break;
    case ET_SetRenderFeaturesEvent:
        rt_VSyncMode = Int( _Event.Data.SetRenderFeaturesEvent.VSyncMode ).Clamp( VSync_Disabled, VSync_Half );
        {
            FRenderFeatures features;
            features.VSyncMode = rt_VSyncMode;
            GRenderBackend->SetRenderFeatures( features );
        }
        break;
    case ET_SetCursorModeEvent:
        if ( rt_DisabledCursor != _Event.Data.SetCursorModeEvent.bDisabledCursor ) {
            rt_DisabledCursor = _Event.Data.SetCursorModeEvent.bDisabledCursor;
            glfwSetInputMode( Wnd, GLFW_CURSOR, GLFWCursorMode[ rt_DisabledCursor ] );
            if ( rt_DisabledCursor ) {
                //glfwGetCursorPos( Wnd, &MousePositionX, &MousePositionY );
                MousePositionX = MOUSE_LOST;
            }
        }

        break;
    }
}

static void SendChangedVideoModeEvent() {
    FEvent * event = rt_SendEvent();
    event->Type = ET_ChangedVideoModeEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();
    FChangedVideoModeEvent & data = event->Data.ChangedVideoModeEvent;
    data.Width = rt_Width;
    data.Height = rt_Height;
    data.PhysicalMonitor = rt_PhysicalMonitor;
    data.RefreshRate = rt_RefreshRate;
    data.bFullscreen = rt_Fullscreen;
    FString::CopySafe( data.Backend, sizeof( data.Backend ), GRenderBackend->Name );
}

void rt_UpdateDisplays( FEventQueue & _EventQueue ) {
    FEvent * event;
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

        if ( rt_Fullscreen ) {
            FPhysicalMonitorArray const & physicalMonitors = rt_GetPhysicalMonitors();
            FPhysicalMonitor * physMonitor = physicalMonitors[ rt_PhysicalMonitor ];
            GLFWmonitor * monitor = ( GLFWmonitor * )physMonitor->Internal.Pointer;
            bool bIsMonitorConnected = monitor != nullptr;

            glfwSetWindowMonitor( Wnd,
                                  monitor,                      // switch to fullscreen mode
                                  rt_PositionX, rt_PositionY,   // position is ignored
                                  rt_Width,
                                  rt_Height,
                                  rt_RefreshRate );

            bSetWindowPos = false;

            if ( bIsMonitorConnected ) {
                // Store real refresh rate
                const GLFWvidmode * videoMode = glfwGetVideoMode( monitor );
                rt_RefreshRate = videoMode->refreshRate;

                glfwFocusWindow( Wnd );
                bSetFocus = false;
            }

            if ( !glfwGetWindowMonitor( Wnd ) ) {
                rt_Fullscreen = false;
            }

        } else {
            glfwSetWindowMonitor( Wnd,
                nullptr,  // switch to windowed mode
                rt_PositionX, rt_PositionY,
                rt_Width,
                rt_Height,
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

        glfwSetWindowOpacity( Wnd, rt_Opacity / 255.0f );
        glfwSetWindowAttrib( Wnd, GLFW_DECORATED, rt_Decorated );
        glfwSetWindowAttrib( Wnd, GLFW_AUTO_ICONIFY, rt_AutoIconify );
        glfwSetWindowAttrib( Wnd, GLFW_FLOATING, rt_Floating );
        glfwSetWindowTitle( Wnd, rt_Title );
    }

    if ( bSetWindowPos ) {
        bSetWindowPos = false;

        if ( !rt_Fullscreen ) {
            glfwSetWindowPos( Wnd, rt_PositionX, rt_PositionY );
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
        event = rt_SendEvent();
        event->Type = ET_VisibleEvent;
        event->TimeStamp = GRuntime.SysSeconds_d();
        FVisibleEvent & data = event->Data.VisibleEvent;
        data.bVisible = bIsWindowVisible;
    }

    //bIsWindowMaximized = !!glfwGetWindowAttrib( Wnd, GLFW_MAXIMIZED );
    //bIsWindowHovered = glfwGetWindowAttrib( Wnd, GLFW_HOVERED );

    if ( !ClipboardCopy.IsEmpty() ) {
        glfwSetClipboardString( NULL, ClipboardCopy.ToConstChar() );
        ClipboardCopy.Clear();
    }
}

void rt_SetClipboard_GameThread( const char * _Utf8String ) {
    ClipboardCopy = _Utf8String;
    ClipboardPaste = _Utf8String;
}

FString const & rt_GetClipboard_GameThread() {
    return ClipboardPaste;
}
