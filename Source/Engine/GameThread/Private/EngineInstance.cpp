/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <GameThread/Public/EngineInstance.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/Render/RenderFrontend.h>
#include <World/Public/Audio/AudioSystem.h>
#include <World/Public/Audio/AudioCodec/OggVorbisDecoder.h>
#include <World/Public/Audio/AudioCodec/Mp3Decoder.h>
#include <World/Public/Audio/AudioCodec/WavDecoder.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Canvas.h>
#include <World/Public/World.h>

#include <World/Private/PrimitiveLinkPool.h>

#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/VertexMemoryGPU.h>

#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>

#include <Bullet3Common/b3Logging.h>
#include <Bullet3Common/b3AlignedAllocator.h>

#include <DetourAlloc.h>

#include "Console.h"
#include "ImguiContext.h"

//#define IMGUI_CONTEXT

static ARuntimeVariable RVSyncGPU( _CTS( "SyncGPU" ), _CTS( "0" ) );
static ARuntimeVariable RVShowStat( _CTS( "ShowStat" ), _CTS( "1" ) );

AEngineInstance & GEngine = AEngineInstance::Inst();

AEngineInstance::AEngineInstance() {
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

static void *PhysModuleAlignedAlloc( size_t _BytesCount, int _Alignment ) {
    AN_ASSERT( _Alignment <= 16 );
    return GZoneMemory.Alloc( _BytesCount );
}

static void *PhysModuleAlloc( size_t _BytesCount ) {
    return GZoneMemory.Alloc( _BytesCount );
}

static void PhysModuleFree( void * _Bytes ) {
    GZoneMemory.Free( _Bytes );
}

static void * NavModuleAlloc( size_t _BytesCount, dtAllocHint _Hint ) {
    return GZoneMemory.Alloc( _BytesCount );
}

static void NavModuleFree( void * _Bytes ) {
    GZoneMemory.Free( _Bytes );
}

static void *ImguiModuleAlloc( size_t _BytesCount, void * ) {
    return GZoneMemory.Alloc( _BytesCount );
}

static void ImguiModuleFree( void * _Bytes, void * ) {
    GZoneMemory.Free( _Bytes );
}

void AEngineInstance::Run( ACreateGameModuleCallback _CreateGameModuleCallback ) {
    // Pump initial events
    GRuntime.PumpEvents();

    GConsole.ReadStoryLines();

    InitializeFactories();

    AGarbageCollector::Initialize();

    // Init physics module
    b3SetCustomPrintfFunc( PhysModulePrintFunction );
    b3SetCustomWarningMessageFunc( PhysModuleWarningFunction );
    b3SetCustomErrorMessageFunc( PhysModuleErrorFunction );
    b3AlignedAllocSetCustom( PhysModuleAlloc, PhysModuleFree );
    b3AlignedAllocSetCustomAligned( PhysModuleAlignedAlloc, PhysModuleFree );

    // Init recast navigation module
    dtAllocSetCustom( NavModuleAlloc, NavModuleFree );

    // Init Imgui allocators
    ImGui::SetAllocatorFunctions( ImguiModuleAlloc, ImguiModuleFree, NULL );

    GResourceManager.Initialize();

    GVertexMemoryGPU.Initialize();
    GStreamedMemoryGPU.Initialize();

    GRenderFrontend.Initialize();

    GAudioSystem.Initialize();
    GAudioSystem.RegisterDecoder( "ogg", CreateInstanceOf< AOggVorbisDecoder >() );
    GAudioSystem.RegisterDecoder( "mp3", CreateInstanceOf< AMp3Decoder >() );
    GAudioSystem.RegisterDecoder( "wav", CreateInstanceOf< AWavDecoder >() );

    AFont::SetGlyphRanges( AFont::GetGlyphRangesCyrillic() );

    Canvas.Initialize();

    GameModule = _CreateGameModuleCallback();
    GameModule->AddRef();

    GLogger.Printf( "Created game module: %s\n", GameModule->FinalClassName() );

    ProcessEvents();

    GameModule->OnGameStart();

#ifdef IMGUI_CONTEXT
    ImguiContext = CreateInstanceOf< AImguiContext >();
    ImguiContext->SetFont( ACanvas::GetDefaultFont() );
    ImguiContext->AddRef();
#endif

    if ( SetCriticalMark() ) {
        return;
    }

    do
    {
        if ( IsCriticalError() ) {
            // Critical error in other thread was occured
            return;
        }

        // Set new frame, process game events
        GRuntime.NewFrame();

        // Take current frame duration
        FrameDurationInSeconds = GRuntime.SysFrameDuration() * 0.000001;

        // Don't allow very slow frames
        if ( FrameDurationInSeconds > 0.5f ) {
            FrameDurationInSeconds = 0.5f;
        }

        // Garbage collect from previuous frames
        AGarbageCollector::DeallocateObjects();

        // Execute console commands
        ACommandContext * commandContext = APlayerController::GetCurrentCommandContext();
        if ( commandContext ) {
            CommandProcessor.Execute( *commandContext );
        }

        // Tick worlds
        AWorld::UpdateWorlds( GameModule, FrameDurationInSeconds );

        // Update audio system
        GAudioSystem.Update( APlayerController::GetCurrentAudioListener(), FrameDurationInSeconds );

        // Sync with GPU to prevent "input lag"
        if ( RVSyncGPU ) {
            GRenderBackend->WaitGPU();
        }

        // Pump runtime events
        GRuntime.PumpEvents();

        // Process runtime events
        ProcessEvents();

        // Update input
        UpdateInputAxes();

#ifdef IMGUI_CONTEXT
        // Imgui test
        UpdateImgui();
#endif
        // Draw widgets, HUD, etc
        DrawCanvas();

        // Build frame data for rendering
        GRenderFrontend.Render( &Canvas );

        // Generate GPU commands
        GRenderBackend->RenderFrame( GRenderFrontend.GetFrameData() );

        // Swap buffers for streamed memory
        GStreamedMemoryGPU.SwapFrames();

        // Swap screen buffers
        GRenderBackend->SwapBuffers();

        // Wait for free streamed buffer
        GStreamedMemoryGPU.WaitBuffer();

    } while ( !GRuntime.IsPendingTerminate() );

    GameModule->OnGameEnd();

    Desktop.Reset();

    AWorld::DestroyWorlds();
    AWorld::KickoffPendingKillWorlds();

    GameModule->RemoveRef();
    GameModule = nullptr;

#ifdef IMGUI_CONTEXT
    ImguiContext->RemoveRef();
    ImguiContext = nullptr;
#endif

    Canvas.Deinitialize();

    GRenderFrontend.Deinitialize();

    GResourceManager.Deinitialize();

    GAudioSystem.PurgeChannels();
    GAudioSystem.UnregisterDecoders();

    AGarbageCollector::Deinitialize();

    GVertexMemoryGPU.Deinitialize();
    GStreamedMemoryGPU.Deinitialize();
    GPrimitiveLinkPool.Free();

    GAudioSystem.Deinitialize();

    DeinitializeFactories();

    GConsole.WriteStoryLines();
}

void AEngineInstance::DrawCanvas() {
    Canvas.Begin( FramebufferWidth, FramebufferHeight );

    if ( IsWindowVisible() )
    {
        if ( Desktop )
        {
            // Draw desktop
            Desktop->GenerateWindowHoverEvents();
            Desktop->GenerateDrawEvents( Canvas );
            if ( Desktop->IsCursorVisible() )
            {
                Desktop->DrawCursor( Canvas );
            }

            // Draw halfscreen console
            GConsole.SetFullscreen( false );
            GConsole.Draw( &Canvas, FrameDurationInSeconds );
        }
        else
        {
            // Draw fullscreen console
            GConsole.SetFullscreen( true );
            GConsole.Draw( &Canvas, FrameDurationInSeconds );
        }

        ShowStats();
    }

    Canvas.End();
}

void AEngineInstance::ShowStats() {
    if ( RVShowStat )
    {
        SRenderFrame * frameData = GRenderFrontend.GetFrameData();

        Float2 pos( 8, 8 );
        const float y_step = 22;
        const int numLines = 14;

        pos.Y = Canvas.Height - numLines * y_step;

        const size_t TotalMemorySizeInBytes = ( (GZoneMemory.GetZoneMemorySizeInMegabytes()<<20)
                                                + (GHunkMemory.GetHunkMemorySizeInMegabytes()<<20)
                                                + GRuntime.GetFrameMemorySize() );

        SRenderFrontendStat const & stat = GRenderFrontend.GetStat();

        enum { FPS_BUF = 16 };
        static float fpsavg[FPS_BUF];
        static int n = 0;
        fpsavg[n & (FPS_BUF-1)] = FrameDurationInSeconds;
        n++;
        float fps = 0;
        for ( int i = 0 ; i < FPS_BUF ; i++ )
            fps += fpsavg[i];
        fps /= FPS_BUF;
        fps = 1.0f / ( fps > 0.0f ? fps : 1.0f );

        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Zone memory usage: %f KB / %d MB", GZoneMemory.GetTotalMemoryUsage()/1024.0f, GZoneMemory.GetZoneMemorySizeInMegabytes() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Hunk memory usage: %f KB / %d MB", GHunkMemory.GetTotalMemoryUsage()/1024.0f, GHunkMemory.GetHunkMemorySizeInMegabytes() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Frame memory usage: %f KB / %d MB (Max %f KB)", GRuntime.GetFrameMemoryUsedPrev()/1024.0f, GRuntime.GetFrameMemorySize()>>20, GRuntime.GetMaxFrameMemoryUsage()/1024.0f ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Frame memory usage (GPU): %f KB / %d MB (Max %f KB)", GStreamedMemoryGPU.GetUsedMemoryPrev()/1024.0f, GStreamedMemoryGPU.GetAllocatedMemory()>>20, GStreamedMemoryGPU.GetMaxMemoryUsage()/1024.0f ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Vertex cache memory usage (GPU): %f KB / %d MB", GVertexMemoryGPU.GetUsedMemory()/1024.0f, GVertexMemoryGPU.GetAllocatedMemory()>>20 ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Heap memory usage: %f KB", (GHeapMemory.GetTotalMemoryUsage()-TotalMemorySizeInBytes)/1024.0f
        /*- GZoneMemory.GetZoneMemorySizeInMegabytes()*1024 - GMainHunkMemory.GetHunkMemorySizeInMegabytes()*1024 - 256*1024.0f*/ ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Visible instances: %d", frameData->Instances.Size() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Visible shadow instances: %d", frameData->ShadowInstances.Size() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Visible dir lights: %d", frameData->DirectionalLights.Size() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Polycount: %d", stat.PolyCount ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("ShadowMapPolyCount: %d", stat.ShadowMapPolyCount ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Frontend time: %d msec", stat.FrontendTime ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Active audio channels: %d", GAudioSystem.GetNumActiveChannels() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos+Float2(1.0f), AColor4::White(), Core::Fmt("Frame time %.1f ms (FPS: %d, AVG %d)", FrameDurationInSeconds*1000.0f, int(1.0f / FrameDurationInSeconds), int(fps+0.5f) ) );
        Canvas.DrawTextUTF8( pos, AColor4::Blue(), Core::Fmt("Frame time %.1f ms (FPS: %d, AVG %d)", FrameDurationInSeconds*1000.0f, int(1.0f / FrameDurationInSeconds), int(fps+0.5f) ) );
    }
}

void AEngineInstance::Print( const char * _Message ) {
    GConsole.Print( _Message );
}

void AEngineInstance::DeveloperKeys( SKeyEvent const & _Event ) {
//    if ( _Event.Action == IE_Press ) {
//        if ( _Event.Key == KEY_F1 ) {
//            GLogger.Printf( "OpenGL Backend Test\n" );
//            Core::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "OpenGL 4.5" );
//            ResetVideoMode();
//        } else if ( _Event.Key == KEY_F2 ) {
//            GLogger.Printf( "Vulkan Backend Test\n" );
//            Core::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "Vulkan" );
//            ResetVideoMode();
//        } else if ( _Event.Key == KEY_F3 ) {
//            GLogger.Printf( "Null Backend Test\n" );
//            Core::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "Null" );
//            ResetVideoMode();
//        }
//    }
}

void AEngineInstance::OnKeyEvent( SKeyEvent const & _Event, double _TimeStamp ) {
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
        ACommandContext * commandContext = APlayerController::GetCurrentCommandContext();
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
}

void AEngineInstance::OnMouseButtonEvent( SMouseButtonEvent const & _Event, double _TimeStamp ) {
#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseButtonEvent( _Event );
#endif

    if ( GConsole.IsActive() && _Event.Action != IE_Release ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateMouseButtonEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnMouseWheelEvent( SMouseWheelEvent const & _Event, double _TimeStamp ) {
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
}

void AEngineInstance::OnMouseMoveEvent( SMouseMoveEvent const & _Event, double _TimeStamp ) {
    if ( Desktop ) {
        Float2 cursorPosition = Desktop->GetCursorPosition();

        // Simulate ballistics
        const bool bSimulateCursorBallistics = true;
        if ( bSimulateCursorBallistics ) {
            cursorPosition.X += _Event.X / VideoMode.RefreshRate * DPI_X;
            cursorPosition.Y -= _Event.Y / VideoMode.RefreshRate * DPI_Y;
        } else {
            cursorPosition.X += _Event.X;
            cursorPosition.Y -= _Event.Y;
        }
        cursorPosition = cursorPosition.Clamp( Float2(0.0f), Float2( FramebufferWidth-1, FramebufferHeight-1 ) );

        Desktop->SetCursorPosition( cursorPosition );

        if ( !GConsole.IsActive() ) {
            Desktop->GenerateMouseMoveEvents( _Event, _TimeStamp );
        }
    }
}

void AEngineInstance::OnJoystickButtonEvent( SJoystickButtonEvent const & _Event, double _TimeStamp ) {
    if ( GConsole.IsActive() && _Event.Action != IE_Release ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateJoystickButtonEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnJoystickAxisEvent( struct SJoystickAxisEvent const & _Event, double _TimeStamp ) {
    if ( Desktop ) {
        Desktop->GenerateJoystickAxisEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnCharEvent( SCharEvent const & _Event, double _TimeStamp ) {
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
}

void AEngineInstance::OnChangedVideoModeEvent( SChangedVideoModeEvent const & _Event ) {
    VideoMode.Width = _Event.Width;
    VideoMode.Height = _Event.Height;
    VideoMode.PhysicalMonitor = _Event.PhysicalMonitor;
    VideoMode.RefreshRate = _Event.RefreshRate;
    VideoMode.bFullscreen = _Event.bFullscreen;
    Core::Strcpy( VideoMode.Backend, sizeof( VideoMode.Backend ), _Event.Backend );

    FramebufferWidth = _Event.FramebufferWidth;
    FramebufferHeight = _Event.FramebufferHeight;
    RetinaScale = Float2( (float)FramebufferWidth / VideoMode.Width, (float)FramebufferHeight / VideoMode.Height );

    GLogger.Printf( "OnChangedVideoModeEvent: %d %d %d %d\n",FramebufferWidth,FramebufferHeight,_Event.Width,_Event.Height);

    if ( _Event.bFullscreen ) {
        SPhysicalMonitor const * monitor = GRuntime.GetMonitor( _Event.PhysicalMonitor );

        const float MM_To_Inch = 0.0393701f;
        DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);
    } else {
        SPhysicalMonitor const * monitor = GRuntime.GetPrimaryMonitor();//GRuntime.GetMonitor( _Event.PhysicalMonitor );

        DPI_X = monitor->DPI_X;
        DPI_Y = monitor->DPI_Y;
    }

    GLogger.Printf( "Console resize %d\n",FramebufferWidth);
    GConsole.Resize( FramebufferWidth );

    if ( Desktop ) {
        // Force update transform
        Desktop->MarkTransformDirty();

        // Set size
        Desktop->SetSize( FramebufferWidth, FramebufferHeight );
    }
}

void AEngineInstance::ProcessEvent( SEvent const & _Event ) {
    switch ( _Event.Type ) {
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
        AInputComponent::SetJoystickState( _Event.Data.JoystickStateEvent.Joystick, _Event.Data.JoystickStateEvent.NumAxes, _Event.Data.JoystickStateEvent.NumButtons, _Event.Data.JoystickStateEvent.bGamePad, _Event.Data.JoystickStateEvent.bConnected );
        break;
    case ET_JoystickButtonEvent:
        OnJoystickButtonEvent( _Event.Data.JoystickButtonEvent, _Event.TimeStamp );
        break;
    case ET_JoystickAxisEvent:
        OnJoystickAxisEvent( _Event.Data.JoystickAxisEvent, _Event.TimeStamp );
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

void AEngineInstance::ProcessEvents() {
    AEventQueue * eventQueue = GRuntime.ReadEvents_GameThread();

    SEvent const * event;
    while ( nullptr != ( event = eventQueue->Pop() ) ) {
        ProcessEvent( *event );
    }

    if ( !VideoMode.bFullscreen && GConsole.IsActive() ) {
        GRuntime.SetCursorEnabled( true );
    } else {
        GRuntime.SetCursorEnabled( false );
    }
}

void AEngineInstance::UpdateInputAxes() {
    for ( AInputComponent * component = AInputComponent::GetInputComponents() ; component ; component = component->GetNext() ) {
        component->UpdateAxes( FrameDurationInSeconds );
    }
}

void AEngineInstance::SetVideoMode( unsigned short _Width, unsigned short _Height, unsigned short _PhysicalMonitor, uint8_t _RefreshRate, bool _Fullscreen, const char * _Backend ) {
    SEvent & event = SendEvent();
    event.Type = ET_SetVideoModeEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    SSetVideoModeEvent & data = event.Data.SetVideoModeEvent;
    data.Width = _Width;
    data.Height = _Height;
    data.PhysicalMonitor = _PhysicalMonitor;
    data.RefreshRate = _RefreshRate;
    data.bFullscreen = _Fullscreen;
    Core::Strcpy( data.Backend, sizeof( data.Backend ), _Backend );

    VideoMode.Width = _Width;
    VideoMode.Height = _Height;
    VideoMode.PhysicalMonitor = _PhysicalMonitor;
    VideoMode.RefreshRate = _RefreshRate;
    VideoMode.bFullscreen = _Fullscreen;
    Core::Strcpy( VideoMode.Backend, sizeof( VideoMode.Backend ), _Backend );
}

void AEngineInstance::SetVideoMode( SVideoMode const & _VideoMode ) {
    SetVideoMode( _VideoMode.Width, _VideoMode.Height, _VideoMode.PhysicalMonitor, _VideoMode.RefreshRate, _VideoMode.bFullscreen, _VideoMode.Backend );
}

void AEngineInstance::ResetVideoMode() {
    SetVideoMode( VideoMode );
}

SVideoMode const & AEngineInstance::GetVideoMode() const {
    return VideoMode;
}

void AEngineInstance::SetWindowDefs( float _Opacity, bool _Decorated, bool _AutoIconify, bool _Floating, const char * _Title ) {
    SEvent & event = SendEvent();
    event.Type = ET_SetWindowDefsEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    SSetWindowDefsEvent & data = event.Data.SetWindowDefsEvent;
    data.Opacity = Math::Clamp( _Opacity, 0.0f, 1.0f ) * 255.0f;
    data.bDecorated = _Decorated;
    data.bAutoIconify = _AutoIconify;
    data.bFloating = _Floating;
    Core::Strcpy( data.Title, sizeof( data.Title ), _Title );
}

void AEngineInstance::SetWindowPos( int _X, int _Y ) {
    SEvent & event = SendEvent();
    event.Type = ET_SetWindowPosEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    SSetWindowPosEvent & data = event.Data.SetWindowPosEvent;
    data.PositionX = WindowPosX = _X;
    data.PositionY = WindowPosY = _Y;
}

void AEngineInstance::GetWindowPos( int & _X, int & _Y ) {
    _X = WindowPosX;
    _Y = WindowPosY;
}

void AEngineInstance::SetInputFocus() {
    SEvent & event = SendEvent();
    event.Type = ET_SetInputFocusEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
}

SEvent & AEngineInstance::SendEvent() {
    AEventQueue * queue = GRuntime.WriteEvents_GameThread();
    return *queue->Push();
}

void AEngineInstance::MapWindowCoordinate( float & InOutX, float & InOutY ) const {
    InOutX += WindowPosX;
    InOutY += WindowPosY;
}

void AEngineInstance::UnmapWindowCoordinate( float & InOutX, float & InOutY ) const {
    InOutX -= WindowPosX;
    InOutY -= WindowPosY;
}

void AEngineInstance::SetDesktop( WDesktop * _Desktop ) {
    Desktop = _Desktop;
    if ( Desktop ) {
        // Force update transform
        Desktop->MarkTransformDirty();

        // Set size
        Desktop->SetSize( FramebufferWidth, FramebufferHeight );
    }
}

IEngineInterface * GetEngineInstance() {
    return &GEngine;
}


#ifdef IMGUI_CONTEXT

///////////////////////////////////////////////////////////////////////////////////////////
// IMGUI test
///////////////////////////////////////////////////////////////////////////////////////////

static AActor * SelectedActor = nullptr;
static ASceneComponent * SelectedComponent = nullptr;

static void ShowAttribute( ADummy * a, AAttributeMeta const * attr ) {
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

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().CStr() );

            break;
        }

        case EAttributeType::T_Float3: {
            Float3 v = attr->GetFloat3Value( a );

            AString s;
            attr->GetValue( a, s );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().CStr() );

            break;
        }

        case EAttributeType::T_Float4: {
            Float4 v = attr->GetFloat4Value( a );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().CStr() );

            break;
        }

        case EAttributeType::T_Quat: {
            Quat v = attr->GetQuatValue( a );

            ImGui::Text( "%s (%s) : %s", attr->GetName(), attr->GetTypeName(), v.ToString().CStr() );

            break;
        }

        case EAttributeType::T_String: {
            AString v;
            attr->GetValue( a, v );

            ImGui::InputText( attr->GetName(), (char*)v.CStr(), v.Length(), ImGuiInputTextFlags_ReadOnly );

            break;
        }

        default:
            break;
    }
}

static void ShowComponentHierarchy( ASceneComponent * component ) {
    if ( ImGui::TreeNodeEx( component, component == SelectedComponent ? ImGuiTreeNodeFlags_Selected : 0, "%s (%s)", component->GetNameConstChar(), component->FinalClassName() ) ) {

        if ( ImGui::IsItemClicked() ) {
            SelectedComponent = component;
            SelectedActor = component->GetParentActor();
        }

        AArrayOfChildComponents const & childs = component->GetChilds();
        for ( ASceneComponent * child : childs ) {
            ShowComponentHierarchy( child );
        }

        ImGui::TreePop();
    }
}

void AEngineInstance::UpdateImgui() {
    ImguiContext->BeginFrame( FrameDurationInSeconds );

    //ImGui::ShowDemoWindow();

    if ( ImGui::Begin( "Test" ) ) {
        TPodArray< AAttributeMeta const * > attributes;

        static char buf[ 1024 ];
        ImGui::InputTextMultiline( "textedit", buf, sizeof( buf ) );

        TPodArray< AWorld * > const & Worlds = AWorld::GetWorlds();

        for ( int i = 0 ; i < Worlds.Size() ; i++ ) {
            if ( ImGui::CollapsingHeader( "World" ) ) {

                static uint8_t childFrameId;
                ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                contentRegion.y *= 0.5f;
                if ( ImGui::BeginChildFrame( ( ImGuiID )( size_t )&childFrameId, contentRegion ) ) {

                    AWorld * w = Worlds[ i ];

                    TPodArray< AActor * > const & actors = w->GetActors();

                    ImGui::Text( "Actors" );
                    for ( int j = 0 ; j < actors.Size() ; j++ ) {
                        AActor * a = actors[ j ];

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
                static uint8_t childFrameId2;
                contentRegion = ImGui::GetContentRegionAvail();
                if ( ImGui::BeginChildFrame( ( ImGuiID )( size_t )&childFrameId2, contentRegion ) ) {

                    if ( SelectedActor ) {
                        AActor * a = SelectedActor;

                        AClassMeta const & meta = a->FinalClassMeta();

                        attributes.Clear();
                        meta.GetAttributes( attributes );

                        for ( AAttributeMeta const * attr : attributes ) {
                            ShowAttribute( a, attr );
                        }

                        for ( int k = 0 ; k < a->GetComponents().Size() ; k++ ) {

                            AActorComponent * component = a->GetComponents()[ k ];

                            if ( ImGui::CollapsingHeader( Core::Fmt( "%s (%s)", component->GetNameConstChar(), component->FinalClassName() ) ) ) {

                                AClassMeta const & componentMeta = component->FinalClassMeta();

                                attributes.Clear();
                                componentMeta.GetAttributes( attributes );

                                for ( AAttributeMeta const * attr : attributes ) {
                                    ShowAttribute( component, attr );
                                }
                            }
                        }
                    }
                }
                ImGui::EndChildFrame();


                //ImGui::Text( "Levels" );
                //for( int j = 0 ; j < w->ArrayOfLevels.Length() ; j++ ) {
                //    ALevel * lev = w->ArrayOfLevels[j];

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
static void PrecacheResources( AClassMeta const & _ClassMeta ) {
    for ( APrecacheMeta const * precache = _ClassMeta.GetPrecacheList() ; precache ; precache = precache->Next() ) {
        GLogger.Printf( "---------- Precache -----------\n"
                        "Resource Class: \"%s\"\n"
                        "Resource Path: \"%s\"\n",
                        precache->GetResourceClassMeta().GetName(),
                        precache->GetResourcePath() );

        //GResourceManager.LoadResource( precache->GetResourceClassMeta(), precache->GetResourcePath() );
    }
}

static AClassMeta const * GetActorClassMeta( ADocument const & _Document, int _Object ) {
    SDocumentField * classNameField = _Document.FindField( _Object, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "AWorld::LoadActor: invalid actor class\n" );
        return nullptr;
    }

    SDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    AClassMeta const * classMeta = AActor::Factory().LookupClass( classNameValue->Token.ToString().CStr() );
    if ( !classMeta ) {
        GLogger.Printf( "AWorld::LoadActor: invalid actor class \"%s\"\n", classNameValue->Token.ToString().CStr() );
        return nullptr;
    }

    return classMeta;
}

AWorld * AEngineInstance::SpawnWorld( FWorldSpawnParameters const & _SpawnParameters ) {

    GLogger.Printf( "==== Spawn World ====\n" );

    AClassMeta const * classMeta = _SpawnParameters.WorldClassMeta();

    if ( !classMeta ) {
        GLogger.Printf( "AEngineInstance::SpawnWorld: invalid world class\n" );
        return nullptr;
    }

    if ( classMeta->Factory() != &AWorld::Factory() ) {
        GLogger.Printf( "AEngineInstance::SpawnWorld: not an world class\n" );
        return nullptr;
    }

    AWorld const * templateWorld = _SpawnParameters.GetTemplate();

    if ( templateWorld && classMeta != &templateWorld->ClassMeta() ) {
        GLogger.Printf( "AEngineInstance::SpawnWorld: FWorldSpawnParameters::Template class doesn't match meta data\n" );
        return nullptr;
    }

    AWorld * world = static_cast< AWorld * >( classMeta->CreateInstance() );

    world->AddRef();

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Size() - 1;

    if ( templateWorld ) {
        // Clone attributes
        AClassMeta::CloneAttributes( templateWorld, world );

        // Precache world resources
        for ( AActor const * templateActor : templateWorld->GetActors() ) {
            PrecacheResources( templateActor->FinalClassMeta() );
        }

        // Clone actors
        for ( AActor const * templateActor : templateWorld->GetActors() ) {
            if ( templateActor->IsPendingKill() ) {
                continue;
            }

            SActorSpawnInfo spawnParameters( &templateActor->FinalClassMeta() );
            spawnParameters.SetTemplate( templateActor );
            //spawnParameters->SpawnTransform = templateActor->GetTransform();
            world->SpawnActor( spawnParameters );
        }
    }

    world->BeginPlay();

    GLogger.Printf( "=====================\n" );
    return world;
}

AWorld * AEngineInstance::LoadWorld( ADocument const & _Document, int _FieldsHead ) {

    GLogger.Printf( "==== Load World ====\n" );

    SDocumentField * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "AEngineInstance::LoadWorld: invalid world class\n" );
        return nullptr;
    }

    SDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    AClassMeta const * classMeta = AWorld::Factory().LookupClass( classNameValue->Token.ToString().CStr() );
    if ( !classMeta ) {
        GLogger.Printf( "AEngineInstance::LoadWorld: invalid world class \"%s\"\n", classNameValue->Token.ToString().CStr() );
        return nullptr;
    }

    AWorld * world = static_cast< AWorld * >( classMeta->CreateInstance() );

    world->AddRef();
    //world->bDuringConstruction = false;

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Size() - 1;

    // Load world attributes
    world->LoadAttributes( _Document, _FieldsHead );

    // Load actors
    SDocumentField * actorsField = _Document.FindField( _FieldsHead, "Actors" );
    if ( actorsField ) {

        // First pass. Precache resources
        for ( int i = actorsField->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
            if ( _Document.Values[ i ].Type != SDocumentValue::T_Object ) {
                continue;
            }

            int actorObject = _Document.Values[ i ].FieldsHead;

            AClassMeta const * actorClassMeta = GetActorClassMeta( _Document, actorObject );
            if ( !actorClassMeta ) {
                continue;
            }

            PrecacheResources( *actorClassMeta );
        }

        // Second pass. Load actors
        for ( int i = actorsField->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
            if ( _Document.Values[ i ].Type != SDocumentValue::T_Object ) {
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
