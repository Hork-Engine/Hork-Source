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

#include <Engine/GameThread/Public/EngineInstance.h>
#include <Engine/GameThread/Public/RenderFrontend.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/Audio/Public/AudioSystem.h>
#include <Engine/Audio/Public/AudioCodec/OggVorbisDecoder.h>
#include <Engine/Audio/Public/AudioCodec/Mp3Decoder.h>
#include <Engine/Audio/Public/AudioCodec/WavDecoder.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Actors/Actor.h>
#include <Engine/World/Public/Actors/PlayerController.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/Canvas.h>

#include <Engine/Runtime/Public/Runtime.h>

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/CriticalError.h>

#include <Bullet3Common/b3Logging.h>
#include <Bullet3Common/b3AlignedAllocator.h>

#include <DetourAlloc.h>

#include "Console.h"
#include "ImguiContext.h"

//#define IMGUI_CONTEXT

FRuntimeVariable RVShowStat( _CTS( "ShowStat" ), _CTS( "0" ) );

FEngineInstance & GEngine = FEngineInstance::Inst();

FCanvas GCanvas;

static float FractAvg = 1;
static float AxesFract = 1;

FEngineInstance::FEngineInstance() {
    bInputFocus = false;
    bIsWindowVisible = false;
    WindowPosX = 0;
    WindowPosY = 0;
}

static void PhysModulePrintFunction( const char * _Message ) {
    GLogger.Printf( "PhysModule: %s", _Message );
}

static void PhysModuleWarningFunction( const char * _Message ) {
    GLogger.Warning( "PhysModule: %s", _Message );
}

static void PhysModuleErrorFunction( const char * _Message ) {
    GLogger.Error( "PhysModule: %s", _Message );
}

static void *PhysModuleAlignedAlloc( size_t _BytesCount, int _Alignement ) {
    return GZoneMemory.Alloc( _BytesCount, _Alignement );
}

static void *PhysModuleAlloc( size_t _BytesCount ) {
    return GZoneMemory.Alloc( _BytesCount, 1 );
}

static void PhysModuleDealloc( void * _Bytes ) {
    GZoneMemory.Dealloc( _Bytes );
}

static void * NavModuleAlloc( size_t _BytesCount, dtAllocHint _Hint ) {
    return GZoneMemory.Alloc( _BytesCount, 1 );
}

static void NavModuleFree( void * _Bytes ) {
    GZoneMemory.Dealloc( _Bytes );
}

static void *ImguiModuleAlloc( size_t _BytesCount, void * ) {
    return GZoneMemory.Alloc( _BytesCount, 1 );
}

static void ImguiModuleFree( void * _Bytes, void * ) {
    GZoneMemory.Dealloc( _Bytes );
}

void FEngineInstance::Initialize( FCreateGameModuleCallback _CreateGameModuleCallback ) {
    GConsole.ReadStoryLines();

    InitializeFactories();

    FGarbageCollector::Initialize();

    // Init physics module
    b3SetCustomPrintfFunc( PhysModulePrintFunction );
    b3SetCustomWarningMessageFunc( PhysModuleWarningFunction );
    b3SetCustomErrorMessageFunc( PhysModuleErrorFunction );
    b3AlignedAllocSetCustom( PhysModuleAlloc, PhysModuleDealloc );
    b3AlignedAllocSetCustomAligned( PhysModuleAlignedAlloc, PhysModuleDealloc );

    // Init recast navigation module
    dtAllocSetCustom( NavModuleAlloc, NavModuleFree );

    // Init Imgui allocators
    ImGui::SetAllocatorFunctions( ImguiModuleAlloc, ImguiModuleFree, NULL );

    GRenderFrontend.Initialize();
    GResourceManager.Initialize();

    GAudioSystem.Initialize();
    GAudioSystem.RegisterDecoder( "ogg", CreateInstanceOf< FOggVorbisDecoder >() );
    GAudioSystem.RegisterDecoder( "mp3", CreateInstanceOf< FMp3Decoder >() );
    GAudioSystem.RegisterDecoder( "wav", CreateInstanceOf< FWavDecoder >() );

    FFont::SetGlyphRanges( FFont::GetGlyphRangesCyrillic() );

    GCanvas.Initialize();

    GameModule = _CreateGameModuleCallback();

    GLogger.Printf( "Created game module: %s\n", GameModule->FinalClassName() );

    ProcessEvents();

    AxesFract = 1;

    GameModule->AddRef();
    GameModule->OnGameStart();

#ifdef IMGUI_CONTEXT
    ImguiContext = CreateInstanceOf< FImguiContext >();
    ImguiContext->SetFont( FCanvas::GetDefaultFont() );
    ImguiContext->AddRef();
#endif

    FrameDuration = 1000000.0 / 60;
}

void FEngineInstance::Deinitialize() {
    GameModule->OnGameEnd();

    Desktop = nullptr;

    FWorld::DestroyWorlds();
    FWorld::KickoffPendingKillWorlds();

    GameModule->RemoveRef();
    GameModule = nullptr;

#ifdef IMGUI_CONTEXT
    ImguiContext->RemoveRef();
    ImguiContext = nullptr;
#endif

    GCanvas.Deinitialize();

    GResourceManager.Deinitialize();
    GRenderFrontend.Deinitialize();
    GAudioSystem.PurgeChannels();
    GAudioSystem.UnregisterDecoders();

    FGarbageCollector::Deinitialize();

    GAudioSystem.Deinitialize();

    DeinitializeFactories();

    GConsole.WriteStoryLines();
}

void FEngineInstance::PrepareFrame() {
    ProcessEvents();

    UpdateInputAxes( AxesFract );

    GRenderFrontend.Render();
}

void FEngineInstance::UpdateFrame() {
    // Take current frame duration
    FrameDurationInSeconds = FrameDuration * 0.000001;

    // Set current frame number
    FrameNumber++;

    // Garbage collect from previuous frames
    FGarbageCollector::DeallocateObjects();

    FCommandContext * commandContext = FPlayerController::GetCurrentCommandContext();
    if ( commandContext ) {
        CommandProcessor.Execute( *commandContext );
    }

    // Tick worlds
    FWorld::UpdateWorlds( GameModule, FrameDurationInSeconds );

    // Update audio system
    GAudioSystem.Update( FPlayerController::GetCurrentAudioListener(), FrameDurationInSeconds );

    // Build draw lists for canvas
    DrawCanvas();

#ifdef IMGUI_CONTEXT
    // Imgui test
    UpdateImgui();
#endif

    // Set next frame duration
    FrameDuration = GRuntime.SysMicroseconds() - FrameTimeStamp;

    // Set next frame time stamp
    FrameTimeStamp = GRuntime.SysMicroseconds();
}

void FEngineInstance::DrawCanvas() {
    GCanvas.Begin( VideoMode.Width, VideoMode.Height );

    if ( Desktop ) {
        Desktop->SetSize( VideoMode.Width, VideoMode.Height );
        Desktop->GenerateWindowHoverEvents();
        Desktop->GenerateDrawEvents( GCanvas );

        // Draw console
        GConsole.SetFullscreen( false );
        GConsole.Draw( &GCanvas, FrameDurationInSeconds );

    } else {

        // Draw fullscreen console
        GConsole.SetFullscreen( true );
        GConsole.Draw( &GCanvas, FrameDurationInSeconds );
    }

    // Show stats
    if ( RVShowStat ) {
        FRenderFrame * frameData = GRuntime.GetFrameData();

        Float2 pos( 8, 8 );
        const float y_step = 22;
        const int numLines = 12;

        pos.Y = GCanvas.Height - numLines * y_step;

        const size_t TotalMemorySizeInBytes = ( (GZoneMemory.GetZoneMemorySizeInMegabytes()<<20)
                                                + (GHunkMemory.GetHunkMemorySizeInMegabytes()<<20)
                                                + GRuntime.GetFrameMemorySize() );

        static size_t MaxFrameMemoryUsage = 0;

        MaxFrameMemoryUsage = FMath::Max( MaxFrameMemoryUsage, GRuntime.GetFrameMemoryUsedPrev() );

        FRenderFrontendStat const & stat = GRenderFrontend.GetStat();

        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("FPS: %d", int(1.0f / FrameDurationInSeconds) ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Zone memory usage: %f KB / %d MB", GZoneMemory.GetTotalMemoryUsage()/1024.0f, GZoneMemory.GetZoneMemorySizeInMegabytes() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Hunk memory usage: %f KB / %d MB", GHunkMemory.GetTotalMemoryUsage()/1024.0f, GHunkMemory.GetHunkMemorySizeInMegabytes() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Frame memory usage: %f KB / %d MB (Max %f KB)", GRuntime.GetFrameMemoryUsedPrev()/1024.0f, GRuntime.GetFrameMemorySize()>>20, MaxFrameMemoryUsage/1024.0f ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Heap memory usage: %f KB", (GHeapMemory.GetTotalMemoryUsage()-TotalMemorySizeInBytes)/1024.0f
        /*- GZoneMemory.GetZoneMemorySizeInMegabytes()*1024 - GMainHunkMemory.GetHunkMemorySizeInMegabytes()*1024 - 256*1024.0f*/ ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Visible instances: %d", frameData->Instances.Size() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Visible shadow instances: %d", frameData->ShadowInstances.Size() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Visible dir lights: %d", frameData->DirectionalLights.Size() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Polycount: %d", stat.PolyCount ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("ShadowMapPolyCount: %d", stat.ShadowMapPolyCount ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Frontend time: %d msec", stat.FrontendTime ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, FColor4::White(), FString::Fmt("Active audio channels: %d", GAudioSystem.GetNumActiveChannels() ) );

    }

    GCanvas.End();
}

void FEngineInstance::Print( const char * _Message ) {
    GConsole.Print( _Message );
}

void FEngineInstance::DeveloperKeys( FKeyEvent const & _Event ) {
//    if ( _Event.Action == IE_Press ) {
//        if ( _Event.Key == KEY_F1 ) {
//            GLogger.Printf( "OpenGL Backend Test\n" );
//            FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "OpenGL 4.5" );
//            ResetVideoMode();
//        } else if ( _Event.Key == KEY_F2 ) {
//            GLogger.Printf( "Vulkan Backend Test\n" );
//            FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "Vulkan" );
//            ResetVideoMode();
//        } else if ( _Event.Key == KEY_F3 ) {
//            GLogger.Printf( "Null Backend Test\n" );
//            FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "Null" );
//            ResetVideoMode();
//        }
//    }
}

void FEngineInstance::OnKeyEvent( FKeyEvent const & _Event, double _TimeStamp ) {
    if ( bQuitOnEscape && _Event.Action == IE_Press && _Event.Key == KEY_ESCAPE ) {
        GameModule->OnGameClose();
    }

    // Check Alt+Enter to toggle fullscreen/windowed mode
    if ( bToggleFullscreenAltEnter ) {
        if ( _Event.Action == IE_Press && _Event.Key == KEY_ENTER && ( HAS_MODIFIER( _Event.ModMask, MOD_ALT ) ) ) {
            VideoMode.bFullscreen = !VideoMode.bFullscreen;
            VideoMode.PhysicalMonitor = 0;
            ResetVideoMode();
        }
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnKeyEvent( _Event );
#endif

    DeveloperKeys( _Event );

    if ( GConsole.IsActive() || bAllowConsole ) {
        FCommandContext * commandContext = FPlayerController::GetCurrentCommandContext();
        if ( commandContext ) {
            GConsole.KeyEvent( _Event, *commandContext, CommandProcessor );
        }
    }
    if ( GConsole.IsActive() && _Event.Action != IE_Release ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateKeyEvents( _Event, _TimeStamp );
    }

    UpdateInputAxes( FractAvg );

    for ( FInputComponent * component = FInputComponent::GetInputComponents()
          ; component ; component = component->GetNext() ) {

        if ( !component->bActive ) {
            continue;
        }

        if ( !component->bIgnoreKeyboardEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->SetButtonState( ID_KEYBOARD, _Event.Key, _Event.Action, _Event.ModMask, _TimeStamp );
        }
    }
}

void FEngineInstance::OnMouseButtonEvent( FMouseButtonEvent const & _Event, double _TimeStamp ) {
#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseButtonEvent( _Event );
#endif

    if ( GConsole.IsActive() ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateMouseButtonEvents( _Event, _TimeStamp );
    }

    UpdateInputAxes( FractAvg );

    for ( FInputComponent * component = FInputComponent::GetInputComponents()
          ; component ; component = component->GetNext() ) {

        if ( !component->bActive ) {
            continue;
        }

        if ( !component->bIgnoreJoystickEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->SetButtonState( ID_MOUSE, _Event.Button, _Event.Action, _Event.ModMask, _TimeStamp );
        }
    }
}

void FEngineInstance::OnMouseWheelEvent( FMouseWheelEvent const & _Event, double _TimeStamp ) {
#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseWheelEvent( _Event );
#endif

    GConsole.MouseWheelEvent( _Event );
    if ( GConsole.IsActive() ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateMouseWheelEvents( _Event, _TimeStamp );
    }

    UpdateInputAxes( FractAvg );

    for ( FInputComponent * component = FInputComponent::GetInputComponents()
          ; component ; component = component->GetNext() ) {

        if ( !component->bActive ) {
            continue;
        }

        if ( !component->bIgnoreMouseEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            if ( _Event.WheelX < 0.0 ) {
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_LEFT, IE_Press, 0, _TimeStamp );
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_LEFT, IE_Release, 0, _TimeStamp );
            } else if ( _Event.WheelX > 0.0 ) {
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_RIGHT, IE_Press, 0, _TimeStamp );
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_RIGHT, IE_Release, 0, _TimeStamp );
            }
            if ( _Event.WheelY < 0.0 ) {
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_DOWN, IE_Press, 0, _TimeStamp );
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_DOWN, IE_Release, 0, _TimeStamp );
            } else if ( _Event.WheelY > 0.0 ) {
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_UP, IE_Press, 0, _TimeStamp );
                component->SetButtonState( ID_MOUSE, MOUSE_WHEEL_UP, IE_Release, 0, _TimeStamp );
            }
        }
    }
}

void FEngineInstance::OnMouseMoveEvent( FMouseMoveEvent const & _Event, double _TimeStamp ) {
    if ( !GConsole.IsActive() ) {
        float x = _Event.X * MouseSensitivity;
        float y = _Event.Y * MouseSensitivity;

        AxesFract -= FractAvg;

        for ( FInputComponent * component = FInputComponent::GetInputComponents()
              ; component ; component = component->GetNext() ) {

            if ( !component->bActive ) {
                continue;
            }

            if ( !component->bIgnoreMouseEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
                component->SetMouseAxisState( x, y );
            }

            component->UpdateAxes( FractAvg, FrameDurationInSeconds );

            if ( !component->bIgnoreMouseEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
                component->SetMouseAxisState( 0, 0 );
            }
        }
    }

    // Simulate ballistics
    const bool bSimulateCursorBallistics = true;
    if ( bSimulateCursorBallistics ) {
        CursorPosition.X += _Event.X / VideoMode.RefreshRate * DPI_X;
        CursorPosition.Y -= _Event.Y / VideoMode.RefreshRate * DPI_Y;
    } else {
        CursorPosition.X += _Event.X;
        CursorPosition.Y -= _Event.Y;
    }
    CursorPosition = CursorPosition.Clamp( Float2(0.0f), Float2( FramebufferWidth, FramebufferHeight ) );

    if ( Desktop ) {
        Desktop->SetCursorPosition( CursorPosition );
        Desktop->GenerateMouseMoveEvents( _Event, _TimeStamp );
    }
}

void FEngineInstance::OnCharEvent( FCharEvent const & _Event, double _TimeStamp ) {
#ifdef IMGUI_CONTEXT
    ImguiContext->OnCharEvent( _Event );
#endif

    GConsole.CharEvent( _Event );
    if ( GConsole.IsActive() ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateCharEvents( _Event, _TimeStamp );
    }

    for ( FInputComponent * component = FInputComponent::GetInputComponents()
          ; component ; component = component->GetNext() ) {

        if ( !component->bActive ) {
            continue;
        }

        if ( !component->bIgnoreCharEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->NotifyUnicodeCharacter( _Event.UnicodeCharacter, _Event.ModMask, _TimeStamp );
        }
    }
}

void FEngineInstance::OnChangedVideoModeEvent( FChangedVideoModeEvent const & _Event ) {
    VideoMode.Width = _Event.Width;
    VideoMode.Height = _Event.Height;
    VideoMode.PhysicalMonitor = _Event.PhysicalMonitor;
    VideoMode.RefreshRate = _Event.RefreshRate;
    VideoMode.bFullscreen = _Event.bFullscreen;
    FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), _Event.Backend );

    FramebufferWidth = VideoMode.Width; // TODO
    FramebufferHeight = VideoMode.Height; // TODO
    RetinaScale = Float2( FramebufferWidth / VideoMode.Width, FramebufferHeight / VideoMode.Height );

    if ( _Event.bFullscreen ) {
        FPhysicalMonitor const * monitor = GRuntime.GetMonitor( _Event.PhysicalMonitor );
        VideoAspectRatio = ( float )monitor->PhysicalWidthMM / monitor->PhysicalHeightMM;

        const float MM_To_Inch = 0.0393701f;
        DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);
    } else {
        FPhysicalMonitor const * monitor = GRuntime.GetPrimaryMonitor();//GRuntime.GetMonitor( _Event.PhysicalMonitor );

        VideoAspectRatio = ( float )_Event.Width / _Event.Height;

        DPI_X = monitor->DPI_X;
        DPI_Y = monitor->DPI_Y;
    }

    GConsole.Resize( VideoMode.Width );
}

void FEngineInstance::ProcessEvent( FEvent const & _Event ) {
    switch ( _Event.Type ) {
    case ET_RuntimeUpdateEvent:

        //GLogger.Printf( "AxesFract %f\n", AxesFract );
        //AN_Assert( AxesFract >= 1.0f );

        FractAvg = 1.0f / (_Event.Data.RuntimeUpdateEvent.InputEventCount + 1);
        AxesFract = 1.0f;

        //GLogger.Printf( "AxesFract %f FractAvg %f index %d\n", AxesFract, FractAvg, _Event.Data.InputTimeStampEvent.Index );
        break;
    case ET_KeyEvent:
        OnKeyEvent( _Event.Data.KeyEvent, _Event.TimeStamp );
        break;
    case ET_MouseButtonEvent:
        OnMouseButtonEvent( _Event.Data.MouseButtonEvent, _Event.TimeStamp );
        break;
    case ET_MouseWheelEvent:
        OnMouseWheelEvent( _Event.Data.MouseWheelEvent, _Event.TimeStamp );
        break;
    case ET_MouseMoveEvent:
        OnMouseMoveEvent( _Event.Data.MouseMoveEvent, _Event.TimeStamp );
        break;
    case ET_JoystickStateEvent:
        FInputComponent::SetJoystickState( _Event.Data.JoystickStateEvent.Joystick, _Event.Data.JoystickStateEvent.NumAxes, _Event.Data.JoystickStateEvent.NumButtons, _Event.Data.JoystickStateEvent.bGamePad, _Event.Data.JoystickStateEvent.bConnected );
        break;
    case ET_JoystickButtonEvent:
        FInputComponent::SetJoystickButtonState( _Event.Data.JoystickButtonEvent.Joystick, _Event.Data.JoystickButtonEvent.Button, _Event.Data.JoystickButtonEvent.Action, _Event.TimeStamp );
        break;
    case ET_JoystickAxisEvent:
        FInputComponent::SetJoystickAxisState( _Event.Data.JoystickAxisEvent.Joystick, _Event.Data.JoystickAxisEvent.Axis, _Event.Data.JoystickAxisEvent.Value );
        break;
    case ET_CharEvent:
        OnCharEvent( _Event.Data.CharEvent, _Event.TimeStamp );
        break;
    case ET_MonitorConnectionEvent:
        break;
    case ET_CloseEvent:
        GameModule->OnGameClose();
        break;
    case ET_FocusEvent:
        bInputFocus = _Event.Data.FocusEvent.bFocused;
        break;
    case ET_VisibleEvent:
        bIsWindowVisible = _Event.Data.VisibleEvent.bVisible;
        break;
    case ET_WindowPosEvent:
        WindowPosX = _Event.Data.WindowPosEvent.PositionX;
        WindowPosY = _Event.Data.WindowPosEvent.PositionY;
        break;
    case ET_ChangedVideoModeEvent:
        OnChangedVideoModeEvent( _Event.Data.ChangedVideoModeEvent );
        break;
    default:
        GLogger.Printf( "Warning: unhandled runtime event %d\n", _Event.Type );
        break;
    }
}

void FEngineInstance::ProcessEvents() {
    FEventQueue * eventQueue = GRuntime.ReadEvents_GameThread();

    FEvent const * event;
    while ( nullptr != ( event = eventQueue->Pop() ) ) {
        ProcessEvent( *event );
    }

    AN_Assert( eventQueue->IsEmpty() );
}

void FEngineInstance::SetVideoMode( unsigned short _Width, unsigned short _Height, unsigned short _PhysicalMonitor, byte _RefreshRate, bool _Fullscreen, const char * _Backend ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetVideoModeEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetVideoModeEvent & data = event.Data.SetVideoModeEvent;
    data.Width = _Width;
    data.Height = _Height;
    data.PhysicalMonitor = _PhysicalMonitor;
    data.RefreshRate = _RefreshRate;
    data.bFullscreen = _Fullscreen;
    FString::CopySafe( data.Backend, sizeof( data.Backend ), _Backend );

    VideoMode.Width = _Width;
    VideoMode.Height = _Height;
    VideoMode.PhysicalMonitor = _PhysicalMonitor;
    VideoMode.RefreshRate = _RefreshRate;
    VideoMode.bFullscreen = _Fullscreen;
    FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), _Backend );
}

void FEngineInstance::SetVideoMode( FVideoMode const & _VideoMode ) {
    SetVideoMode( _VideoMode.Width, _VideoMode.Height, _VideoMode.PhysicalMonitor, _VideoMode.RefreshRate, _VideoMode.bFullscreen, _VideoMode.Backend );
}

void FEngineInstance::ResetVideoMode() {
    SetVideoMode( VideoMode );
}

FVideoMode const & FEngineInstance::GetVideoMode() const {
    return VideoMode;
}

void FEngineInstance::SetWindowDefs( float _Opacity, bool _Decorated, bool _AutoIconify, bool _Floating, const char * _Title ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetWindowDefsEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetWindowDefsEvent & data = event.Data.SetWindowDefsEvent;
    data.Opacity = FMath::Clamp( _Opacity, 0.0f, 1.0f ) * 255.0f;
    data.bDecorated = _Decorated;
    data.bAutoIconify = _AutoIconify;
    data.bFloating = _Floating;
    FString::CopySafe( data.Title, sizeof( data.Title ), _Title );
}

void FEngineInstance::SetWindowPos( int _X, int _Y ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetWindowPosEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetWindowPosEvent & data = event.Data.SetWindowPosEvent;
    data.PositionX = WindowPosX = _X;
    data.PositionY = WindowPosY = _Y;
}

void FEngineInstance::GetWindowPos( int & _X, int & _Y ) {
    _X = WindowPosX;
    _Y = WindowPosY;
}

void FEngineInstance::SetInputFocus() {
    FEvent & event = SendEvent();
    event.Type = ET_SetInputFocusEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
}

//void FEngineInstance::SetRenderFeatures( int _VSyncMode ) {
//    FEvent & event = SendEvent();
//    event.Type = ET_SetRenderFeaturesEvent;
//    event.TimeStamp = GRuntime.SysSeconds_d();
//    FSetRenderFeaturesEvent & data = event.Data.SetRenderFeaturesEvent;
//    data.VSyncMode = _VSyncMode;
//}

void FEngineInstance::SetCursorEnabled( bool _Enabled ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetCursorModeEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetCursorModeEvent & data = event.Data.SetCursorModeEvent;
    data.bDisabledCursor = !_Enabled;
}

FEvent & FEngineInstance::SendEvent() {
    FEventQueue * queue = GRuntime.WriteEvents_GameThread();
    return *queue->Push();
}

void FEngineInstance::MapWindowCoordinate( float & InOutX, float & InOutY ) const {
    InOutX += WindowPosX;
    InOutY += WindowPosY;
}

void FEngineInstance::UnmapWindowCoordinate( float & InOutX, float & InOutY ) const {
    InOutX -= WindowPosX;
    InOutY -= WindowPosY;
}

void FEngineInstance::UpdateInputAxes( float _Fract ) {
    if ( _Fract <= 0 ) {
        return;
    }

    AxesFract -= _Fract;

    for ( FInputComponent * component = FInputComponent::GetInputComponents()
          ; component ; component = component->GetNext() ) {

        if ( !component->bActive ) {
            continue;
        }

        component->UpdateAxes( _Fract, FrameDurationInSeconds );
    }
}

void FEngineInstance::SetDesktop( WDesktop * _Desktop ) {
    Desktop = _Desktop;
}

IEngineInterface * GetEngineInstance() {
    return &GEngine;
}


#ifdef IMGUI_CONTEXT

///////////////////////////////////////////////////////////////////////////////////////////
// IMGUI test
///////////////////////////////////////////////////////////////////////////////////////////

static FActor * SelectedActor = nullptr;
static FSceneComponent * SelectedComponent = nullptr;

static void ShowAttribute( FDummy * a, FAttributeMeta const * attr ) {
    switch ( attr->GetType() ) {
        case EAttributeType::T_Byte: {
            byte v = attr->GetBoolValue( a );

            ImGui::Text( "%s (%s) : %d", attr->GetName(), attr->GetTypeName(), v );

            break;
        }

        case EAttributeType::T_Bool: {
            bool v = attr->GetBoolValue( a );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v ? "true" : "false" );

            break;
        }

        case EAttributeType::T_Int: {
            int v = attr->GetIntValue( a );

            ImGui::Text( "%s (%s) : %d", attr->GetName(), attr->GetTypeName(), v );

            break;
        }

        case EAttributeType::T_Float: {
            float v = attr->GetFloatValue( a );

            ImGui::Text( "%s (%s) : %f", attr->GetName(), attr->GetTypeName(), v );

            break;
        }

        case EAttributeType::T_Float2: {
            Float2 v = attr->GetFloat2Value( a );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().ToConstChar() );

            break;
        }

        case EAttributeType::T_Float3: {
            Float3 v = attr->GetFloat3Value( a );

            FString s;
            attr->GetValue( a, s );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().ToConstChar() );

            break;
        }

        case EAttributeType::T_Float4: {
            Float4 v = attr->GetFloat4Value( a );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().ToConstChar() );

            break;
        }

        case EAttributeType::T_Quat: {
            Quat v = attr->GetQuatValue( a );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().ToConstChar() );

            break;
        }

        case EAttributeType::T_String: {
            FString v;
            attr->GetValue( a, v );

            ImGui::InputText( attr->GetName(), (char*)v.ToConstChar(), v.Length(), ImGuiInputTextFlags_ReadOnly );

            break;
        }

        default:
            break;
    }
}

static void ShowComponentHierarchy( FSceneComponent * component ) {
    if ( ImGui::TreeNodeEx( component, component == SelectedComponent ? ImGuiTreeNodeFlags_Selected : 0, "%s (%s)", component->GetNameConstChar(), component->FinalClassName() ) ) {

        if ( ImGui::IsItemClicked() ) {
            SelectedComponent = component;
            SelectedActor = component->GetParentActor();
        }

        FArrayOfChildComponents const & childs = component->GetChilds();
        for ( FSceneComponent * child : childs ) {
            ShowComponentHierarchy( child );
        }

        ImGui::TreePop();
    }
}

void FEngineInstance::UpdateImgui() {
    ImguiContext->BeginFrame( FrameDurationInSeconds );

    //ImGui::ShowDemoWindow();

    if ( ImGui::Begin( "Test" ) ) {
        TPodArray< FAttributeMeta const * > attributes;

        static char buf[ 1024 ];
        ImGui::InputTextMultiline( "textedit", buf, sizeof( buf ) );

        TPodArray< FWorld * > const & Worlds = FWorld::GetWorlds();

        for ( int i = 0 ; i < Worlds.Size() ; i++ ) {
            if ( ImGui::CollapsingHeader( "World" ) ) {

                static byte childFrameId;
                ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                contentRegion.y *= 0.5f;
                if ( ImGui::BeginChildFrame( ( ImGuiID )( size_t )&childFrameId, contentRegion ) ) {

                    FWorld * w = Worlds[ i ];

                    TPodArray< FActor * > const & actors = w->GetActors();

                    ImGui::Text( "Actors" );
                    for ( int j = 0 ; j < actors.Size() ; j++ ) {
                        FActor * a = actors[ j ];

                        if ( ImGui::TreeNodeEx( a, a == SelectedActor ? ImGuiTreeNodeFlags_Selected : 0, "%s (%s)", a->GetNameConstChar(), a->FinalClassName() ) ) {

                            if ( ImGui::IsItemClicked() ) {
                                SelectedActor = a;
                            }

                            if ( a->RootComponent ) {
                                ShowComponentHierarchy( a->RootComponent );
                            }

                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::EndChildFrame();

                ImGui::Text( "Inspector" );
                static byte childFrameId2;
                contentRegion = ImGui::GetContentRegionAvail();
                if ( ImGui::BeginChildFrame( ( ImGuiID )( size_t )&childFrameId2, contentRegion ) ) {

                    if ( SelectedActor ) {
                        FActor * a = SelectedActor;

                        FClassMeta const & meta = a->FinalClassMeta();

                        attributes.Clear();
                        meta.GetAttributes( attributes );

                        for ( FAttributeMeta const * attr : attributes ) {
                            ShowAttribute( a, attr );
                        }

                        for ( int k = 0 ; k < a->GetComponents().Size() ; k++ ) {

                            FActorComponent * component = a->GetComponents()[ k ];

                            if ( ImGui::CollapsingHeader( FString::Fmt( "%s (%s)", component->GetNameConstChar(), component->FinalClassName() ) ) ) {

                                FClassMeta const & componentMeta = component->FinalClassMeta();

                                attributes.Clear();
                                componentMeta.GetAttributes( attributes );

                                for ( FAttributeMeta const * attr : attributes ) {
                                    ShowAttribute( component, attr );
                                }
                            }
                        }
                    }
                }
                ImGui::EndChildFrame();


                //ImGui::Text( "Levels" );
                //for( int j = 0 ; j < w->ArrayOfLevels.Length() ; j++ ) {
                //    FLevel * lev = w->ArrayOfLevels[j];

                //    if ( ImGui::TreeNode( lev, "%s (%s)", lev->GetNameConstChar(), lev->FinalClassName() ) ) {

                //        ImGui::TreePop();
                //    }
                //}


            }
        }
    }
    ImGui::End();


    ImguiContext->EndFrame();
}
#endif



#if 0
static void PrecacheResources( FClassMeta const & _ClassMeta ) {
    for ( FPrecacheMeta const * precache = _ClassMeta.GetPrecacheList() ; precache ; precache = precache->Next() ) {
        GLogger.Printf( "---------- Precache -----------\n"
                        "Resource Class: \"%s\"\n"
                        "Resource Path: \"%s\"\n",
                        precache->GetResourceClassMeta().GetName(),
                        precache->GetResourcePath() );

        //GResourceManager.LoadResource( precache->GetResourceClassMeta(), precache->GetResourcePath() );
    }
}

static FClassMeta const * GetActorClassMeta( FDocument const & _Document, int _Object ) {
    FDocumentField * classNameField = _Document.FindField( _Object, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "FWorld::LoadActor: invalid actor class\n" );
        return nullptr;
    }

    FDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    FClassMeta const * classMeta = FActor::Factory().LookupClass( classNameValue->Token.ToString().ToConstChar() );
    if ( !classMeta ) {
        GLogger.Printf( "FWorld::LoadActor: invalid actor class \"%s\"\n", classNameValue->Token.ToString().ToConstChar() );
        return nullptr;
    }

    return classMeta;
}

FWorld * FEngineInstance::SpawnWorld( FWorldSpawnParameters const & _SpawnParameters ) {

    GLogger.Printf( "==== Spawn World ====\n" );

    FClassMeta const * classMeta = _SpawnParameters.WorldClassMeta();

    if ( !classMeta ) {
        GLogger.Printf( "FEngineInstance::SpawnWorld: invalid world class\n" );
        return nullptr;
    }

    if ( classMeta->Factory() != &FWorld::Factory() ) {
        GLogger.Printf( "FEngineInstance::SpawnWorld: not an world class\n" );
        return nullptr;
    }

    FWorld const * templateWorld = _SpawnParameters.GetTemplate();

    if ( templateWorld && classMeta != &templateWorld->ClassMeta() ) {
        GLogger.Printf( "FEngineInstance::SpawnWorld: FWorldSpawnParameters::Template class doesn't match meta data\n" );
        return nullptr;
    }

    FWorld * world = static_cast< FWorld * >( classMeta->CreateInstance() );

    world->AddRef();

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Size() - 1;

    if ( templateWorld ) {
        // Clone attributes
        FClassMeta::CloneAttributes( templateWorld, world );

        // Precache world resources
        for ( FActor const * templateActor : templateWorld->GetActors() ) {
            PrecacheResources( templateActor->FinalClassMeta() );
        }

        // Clone actors
        for ( FActor const * templateActor : templateWorld->GetActors() ) {
            if ( templateActor->IsPendingKill() ) {
                continue;
            }

            FActorSpawnParameters spawnParameters( &templateActor->FinalClassMeta() );
            spawnParameters.SetTemplate( templateActor );
            //spawnParameters->SpawnTransform = templateActor->GetTransform();
            world->SpawnActor( spawnParameters );
        }
    }

    world->BeginPlay();

    GLogger.Printf( "=====================\n" );
    return world;
}

FWorld * FEngineInstance::LoadWorld( FDocument const & _Document, int _FieldsHead ) {

    GLogger.Printf( "==== Load World ====\n" );

    FDocumentField * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "FEngineInstance::LoadWorld: invalid world class\n" );
        return nullptr;
    }

    FDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    FClassMeta const * classMeta = FWorld::Factory().LookupClass( classNameValue->Token.ToString().ToConstChar() );
    if ( !classMeta ) {
        GLogger.Printf( "FEngineInstance::LoadWorld: invalid world class \"%s\"\n", classNameValue->Token.ToString().ToConstChar() );
        return nullptr;
    }

    FWorld * world = static_cast< FWorld * >( classMeta->CreateInstance() );

    world->AddRef();
    //world->bDuringConstruction = false;

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Size() - 1;

    // Load world attributes
    world->LoadAttributes( _Document, _FieldsHead );

    // Load actors
    FDocumentField * actorsField = _Document.FindField( _FieldsHead, "Actors" );
    if ( actorsField ) {

        // First pass. Precache resources
        for ( int i = actorsField->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
            if ( _Document.Values[ i ].Type != FDocumentValue::T_Object ) {
                continue;
            }

            int actorObject = _Document.Values[ i ].FieldsHead;

            FClassMeta const * actorClassMeta = GetActorClassMeta( _Document, actorObject );
            if ( !actorClassMeta ) {
                continue;
            }

            PrecacheResources( *actorClassMeta );
        }

        // Second pass. Load actors
        for ( int i = actorsField->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
            if ( _Document.Values[ i ].Type != FDocumentValue::T_Object ) {
                continue;
            }

            int actorObject = _Document.Values[ i ].FieldsHead;

            world->LoadActor( _Document, actorObject );
        }
    }

    world->BeginPlay();

    GLogger.Printf( "=====================\n" );
    return world;
}
#endif
