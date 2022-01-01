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

#include <World/Public/EngineInstance.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/Render/RenderFrontend.h>
#include <World/Public/AudioSystem.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Canvas.h>
#include <World/Public/World.h>

#include <Audio/Public/AudioMixer.h>

#include <Runtime/Public/Runtime.h>

#include <Platform/Public/Logger.h>
#include <Platform/Public/Platform.h>

#include <Bullet3Common/b3Logging.h>
#include <Bullet3Common/b3AlignedAllocator.h>
#include <LinearMath/btAlignedAllocator.h>

#include <DetourAlloc.h>

#include "Console.h"
#include "ImguiContext.h"

#include <Runtime/Public/EntryDecl.h>

//#define IMGUI_CONTEXT

static ARuntimeVariable com_ShowStat( _CTS( "com_ShowStat" ), _CTS( "0" ) );
static ARuntimeVariable com_ShowFPS( _CTS( "com_ShowFPS" ), _CTS( "0" ) );
static ARuntimeVariable com_SimulateCursorBallistics( _CTS( "com_SimulateCursorBallistics" ), _CTS( "1" ) );

static AConsole Console;

AEngineInstance * GEngine = nullptr;

IEngineInterface * CreateEngineInstance()
{
    AN_ASSERT( GEngine == nullptr );
    GEngine = new AEngineInstance;
    return GEngine;
}

void DestroyEngineInstance()
{
    delete GEngine;
    GEngine = nullptr;
}

AEngineInstance::AEngineInstance()
{
    RetinaScale = Float2( 1.0f );
}

static void PhysModulePrintFunction( const char * _Message )
{
    GLogger.Printf( "PhysModule: %s", _Message );
}

static void PhysModuleWarningFunction( const char * _Message )
{
    GLogger.Warning( "PhysModule: %s", _Message );
}

static void PhysModuleErrorFunction( const char * _Message )
{
    GLogger.Error( "PhysModule: %s", _Message );
}

static void *PhysModuleAlignedAlloc( size_t _BytesCount, int _Alignment )
{
    AN_ASSERT( _Alignment <= 16 );
    return GZoneMemory.Alloc( _BytesCount );
}

static void *PhysModuleAlloc( size_t _BytesCount )
{
    return GZoneMemory.Alloc( _BytesCount );
}

static void PhysModuleFree( void * _Bytes )
{
    GZoneMemory.Free( _Bytes );
}

static void * NavModuleAlloc( size_t _BytesCount, dtAllocHint _Hint )
{
    return GHeapMemory.Alloc( _BytesCount );
    //return GZoneMemory.Alloc( _BytesCount );
}

static void NavModuleFree( void * _Bytes )
{
    GHeapMemory.Free( _Bytes );
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

static IGameModule * CreateGameModule( AClassMeta const * _Meta )
{
    if ( !_Meta->IsSubclassOf< IGameModule >() ) {
        CriticalError( "CreateGameModule: game module is not subclass of IGameModule\n" );
    }
    return static_cast< IGameModule * >(_Meta->CreateInstance());
}

void AEngineInstance::Run( SEntryDecl const & _EntryDecl )
{
    Console.ReadStoryLines();

    InitializeFactories();

    AGarbageCollector::Initialize();

    // Init physics module
    b3SetCustomPrintfFunc( PhysModulePrintFunction );
    b3SetCustomWarningMessageFunc( PhysModuleWarningFunction );
    b3SetCustomErrorMessageFunc( PhysModuleErrorFunction );
    b3AlignedAllocSetCustom( PhysModuleAlloc, PhysModuleFree );
    b3AlignedAllocSetCustomAligned( PhysModuleAlignedAlloc, PhysModuleFree );
    btAlignedAllocSetCustom( PhysModuleAlloc, PhysModuleFree );
    btAlignedAllocSetCustomAligned( PhysModuleAlignedAlloc, PhysModuleFree );

    // Init recast navigation module
    dtAllocSetCustom( NavModuleAlloc, NavModuleFree );
#if 0
    // Init Imgui allocators
    ImGui::SetAllocatorFunctions( ImguiModuleAlloc, ImguiModuleFree, NULL );
#endif
    ResourceManager = MakeUnique< AResourceManager >();
    GResourceManager = ResourceManager.GetObject();

    Renderer = CreateInstanceOf< ARenderFrontend >();

    RenderBackend = MakeRef< ARenderBackend >();

    GAudioSystem.Initialize();

    AFont::SetGlyphRanges( GLYPH_RANGE_CYRILLIC );

    GameModule = CreateGameModule( _EntryDecl.ModuleClass );
    GameModule->AddRef();

    GLogger.Printf( "Created game module: %s\n", GameModule->FinalClassName() );

#ifdef IMGUI_CONTEXT
    ImguiContext = CreateInstanceOf< AImguiContext >();
    ImguiContext->SetFont( ACanvas::GetDefaultFont() );
    ImguiContext->AddRef();
#endif

    bAllowInputEvents = true;

    do
    {
        // Set new frame, process game events
        GRuntime->NewFrame();

        // Take current frame duration
        FrameDurationInSeconds = GRuntime->SysFrameDuration() * 0.000001;

        // Don't allow very slow frames
        if ( FrameDurationInSeconds > 0.5f ) {
            FrameDurationInSeconds = 0.5f;
        }

        // Garbage collect from previuous frames
        AGarbageCollector::DeallocateObjects();

        // Execute console commands
        CommandProcessor.Execute( GameModule->CommandContext );

        // Tick worlds
        AWorld::UpdateWorlds( FrameDurationInSeconds );

        // Update audio system
        GAudioSystem.Update( APlayerController::GetCurrentAudioListener(), FrameDurationInSeconds );

        // Poll runtime events
        GRuntime->PollEvents();

        // Update input
        UpdateInput();

#ifdef IMGUI_CONTEXT
        // Imgui test
        UpdateImgui();
#endif
        // Draw widgets, HUD, etc
        DrawCanvas();

        // Build frame data for rendering
        Renderer->Render( &Canvas );

        // Generate GPU commands
        RenderBackend->RenderFrame( GRuntime->GetSwapChain()->GetBackBuffer(), Renderer->GetFrameData() );

    } while ( !GRuntime->IsPendingTerminate() );

    bAllowInputEvents = false;

    GameModule->RemoveRef();
    GameModule = nullptr;

    Desktop.Reset();

    AWorld::DestroyWorlds();
    AWorld::KickoffPendingKillWorlds();

    ASoundEmitter::ClearOneShotSounds();

#ifdef IMGUI_CONTEXT
    ImguiContext->RemoveRef();
    ImguiContext = nullptr;
#endif

    RenderBackend.Reset();

    Renderer.Reset();

    ResourceManager.Reset();
    GResourceManager = nullptr;

    AGarbageCollector::Deinitialize();

    ALevel::PrimitiveLinkPool.Free();

    GAudioSystem.Deinitialize();

    DeinitializeFactories();

    Console.WriteStoryLines();
}

void AEngineInstance::DrawCanvas()
{
    SVideoMode const & videoMode = GRuntime->GetVideoMode();

    Canvas.Begin( videoMode.FramebufferWidth, videoMode.FramebufferHeight );

    if ( IsWindowVisible() ) {
        if ( Desktop ) {
            // Draw desktop
            Desktop->GenerateWindowHoverEvents();
            Desktop->GenerateDrawEvents( Canvas );
            if ( Desktop->IsCursorVisible() && !GRuntime->IsCursorEnabled() ) {
                Desktop->DrawCursor( Canvas );
            }

            // Draw halfscreen console
            Console.SetFullscreen( false );
            Console.Draw( &Canvas, FrameDurationInSeconds );
        }
        else {
            // Draw fullscreen console
            Console.SetFullscreen( true );
            Console.Draw( &Canvas, FrameDurationInSeconds );
        }

        ShowStats();
    }

    Canvas.End();
}

void AEngineInstance::ShowStats()
{
    static TStaticResourceFinder< AFont > Impact18( _CTS( "/Root/impact18.font" ) );
    AFont * font = Impact18.GetObject();

    if ( com_ShowStat ) {
        SRenderFrame * frameData = Renderer->GetFrameData();

        const size_t TotalMemorySizeInBytes = ( (GZoneMemory.GetZoneMemorySizeInMegabytes()<<20)
                                                + (GHunkMemory.GetHunkMemorySizeInMegabytes()<<20)
                                                + GRuntime->GetFrameMemorySize() );

        SRenderFrontendStat const & stat = Renderer->GetStat();

        AVertexMemoryGPU * vertexMemory = GRuntime->GetVertexMemoryGPU();
        AStreamedMemoryGPU * streamedMemory = GRuntime->GetStreamedMemoryGPU();

        const float y_step = 22;
        const int numLines = 13;

        Float2 pos( 8, 8 );
        pos.Y = Canvas.GetHeight() - numLines * y_step;

        Canvas.PushFont( font );
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Zone memory usage: %f KB / %d MB", GZoneMemory.GetTotalMemoryUsage()/1024.0f, GZoneMemory.GetZoneMemorySizeInMegabytes() ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Hunk memory usage: %f KB / %d MB", GHunkMemory.GetTotalMemoryUsage()/1024.0f, GHunkMemory.GetHunkMemorySizeInMegabytes() ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Frame memory usage: %f KB / %d MB (Max %f KB)", GRuntime->GetFrameMemoryUsedPrev()/1024.0f, GRuntime->GetFrameMemorySize()>>20, GRuntime->GetMaxFrameMemoryUsage()/1024.0f ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Frame memory usage (GPU): %f KB / %d MB (Max %f KB)", streamedMemory->GetUsedMemoryPrev()/1024.0f, streamedMemory->GetAllocatedMemory()>>20, streamedMemory->GetMaxMemoryUsage()/1024.0f ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Vertex cache memory usage (GPU): %f KB / %d MB", vertexMemory->GetUsedMemory()/1024.0f, vertexMemory->GetAllocatedMemory()>>20 ), nullptr, true ); pos.Y += y_step;
        if ( GHeapMemory.GetTotalMemoryUsage() > 0 ) {
            Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Heap memory usage: %f KB", (GHeapMemory.GetTotalMemoryUsage()-TotalMemorySizeInBytes)/1024.0f ), nullptr, true ); pos.Y += y_step;
        }
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Visible instances: %d", frameData->Instances.Size() ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Visible shadow instances: %d", frameData->ShadowInstances.Size() ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Visible dir lights: %d", frameData->DirectionalLights.Size() ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Polycount: %d", stat.PolyCount ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("ShadowMapPolyCount: %d", stat.ShadowMapPolyCount ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Frontend time: %d msec", stat.FrontendTime ), nullptr, true ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, Color4::White(), Core::Fmt("Audio channels: %d active, %d virtual", GAudioSystem.GetMixer()->GetNumActiveChannels(), GAudioSystem.GetMixer()->GetNumVirtualChannels() ), nullptr, true ); pos.Y += y_step;
        Canvas.PopFont();
    }

    if ( com_ShowFPS ) {
        enum { FPS_BUF = 16 };
        static float fpsavg[FPS_BUF];
        static int n = 0;
        fpsavg[n & (FPS_BUF-1)] = FrameDurationInSeconds;
        n++;
        float fps = 0;
        for ( int i = 0 ; i < FPS_BUF ; i++ )
            fps += fpsavg[i];
        fps *= (1.0f/FPS_BUF);
        fps = 1.0f / (fps > 0.0f ? fps : 1.0f);
        Canvas.PushFont( font );
        Canvas.DrawTextUTF8( Float2( 10, 10 ), Color4::White(), Core::Fmt( "Frame time %.1f ms (FPS: %d, AVG %d)", FrameDurationInSeconds*1000.0f, int( 1.0f / FrameDurationInSeconds ), int( fps+0.5f ) ), nullptr, true );
        Canvas.PopFont();
    }
}

void AEngineInstance::Print( const char * _Message )
{
    Console.Print( _Message );
}

void AEngineInstance::DeveloperKeys( SKeyEvent const & _Event )
{
}

void AEngineInstance::OnKeyEvent( SKeyEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

    if ( GameModule->bQuitOnEscape && _Event.Action == IA_PRESS && _Event.Key == KEY_ESCAPE ) {
        GameModule->OnGameClose();
    }

    // Check Alt+Enter to toggle fullscreen/windowed mode
    if ( GameModule->bToggleFullscreenAltEnter ) {
        if ( _Event.Action == IA_PRESS && _Event.Key == KEY_ENTER && ( HAS_MODIFIER( _Event.ModMask, KMOD_ALT ) ) ) {
            SVideoMode videoMode = GRuntime->GetVideoMode();
            videoMode.bFullscreen = !videoMode.bFullscreen;
            GRuntime->PostChangeVideoMode( videoMode );
        }
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnKeyEvent( _Event );
#endif

    DeveloperKeys( _Event );

    if ( Console.IsActive() || GameModule->bAllowConsole ) {
        Console.KeyEvent( _Event, GameModule->CommandContext, CommandProcessor );

        if ( !Console.IsActive() && _Event.Key == KEY_GRAVE_ACCENT ) {
            // Console just closed
            return;
        }
    }

    if ( Console.IsActive() && _Event.Action != IA_RELEASE ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateKeyEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnMouseButtonEvent( SMouseButtonEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseButtonEvent( _Event );
#endif

    if ( Console.IsActive() && _Event.Action != IA_RELEASE ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateMouseButtonEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnMouseWheelEvent( SMouseWheelEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnMouseWheelEvent( _Event );
#endif

    Console.MouseWheelEvent( _Event );
    if ( Console.IsActive() ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateMouseWheelEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnMouseMoveEvent( SMouseMoveEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

    if ( Desktop ) {
        SVideoMode const & videoMode = GRuntime->GetVideoMode();

        if ( GRuntime->IsCursorEnabled() ) {
            Float2 cursorPosition;
            int x, y;

            GRuntime->GetCursorPosition( &x, &y );

            cursorPosition.X = Math::Clamp( x, 0, videoMode.FramebufferWidth-1 );
            cursorPosition.Y = Math::Clamp( y, 0, videoMode.FramebufferHeight-1 );

            Desktop->SetCursorPosition( cursorPosition );
        }
        else {
            Float2 cursorPosition = Desktop->GetCursorPosition();

            // Simulate ballistics
            if ( com_SimulateCursorBallistics ) {
                cursorPosition.X += _Event.X / videoMode.RefreshRate * videoMode.DPI_X;
                cursorPosition.Y -= _Event.Y / videoMode.RefreshRate * videoMode.DPI_Y;
            } else {
                cursorPosition.X += _Event.X;
                cursorPosition.Y -= _Event.Y;
            }
            cursorPosition = Math::Clamp( cursorPosition, Float2(0.0f), Float2( videoMode.FramebufferWidth-1, videoMode.FramebufferHeight-1 ) );

            Desktop->SetCursorPosition( cursorPosition );
        }

        if ( !Console.IsActive() ) {
            Desktop->GenerateMouseMoveEvents( _Event, _TimeStamp );
        }
    }
}

void AEngineInstance::OnJoystickButtonEvent( SJoystickButtonEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

    if ( Console.IsActive() && _Event.Action != IA_RELEASE ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateJoystickButtonEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnJoystickAxisEvent( struct SJoystickAxisEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateJoystickAxisEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnCharEvent( SCharEvent const & _Event, double _TimeStamp )
{
    if ( !bAllowInputEvents ) {
        return;
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnCharEvent( _Event );
#endif

    Console.CharEvent( _Event );
    if ( Console.IsActive() ) {
        return;
    }

    if ( Desktop ) {
        Desktop->GenerateCharEvents( _Event, _TimeStamp );
    }
}

void AEngineInstance::OnWindowVisible( bool _Visible )
{
    bIsWindowVisible = _Visible;
}

void AEngineInstance::OnCloseEvent()
{
    GameModule->OnGameClose();
}

void AEngineInstance::OnResize()
{
    SVideoMode const & videoMode = GRuntime->GetVideoMode();

    RetinaScale = Float2( (float)videoMode.FramebufferWidth / videoMode.Width,
                          (float)videoMode.FramebufferHeight / videoMode.Height );

    Console.Resize( videoMode.FramebufferWidth );

    if ( Desktop ) {
        // Force update transform
        Desktop->MarkTransformDirty();

        // Set size
        Desktop->SetSize( videoMode.FramebufferWidth, videoMode.FramebufferHeight );
    }
}

void AEngineInstance::UpdateInput()
{
    SVideoMode const & videoMode = GRuntime->GetVideoMode();

    switch ( GameModule->CursorMode ) {
    case CURSOR_MODE_AUTO:
        if ( !videoMode.bFullscreen && Console.IsActive() ) {
            GRuntime->SetCursorEnabled( true );
        }
        else {
            GRuntime->SetCursorEnabled( false );
        }
        break;
    case CURSOR_MODE_FORCE_ENABLED:
        GRuntime->SetCursorEnabled( true );
        break;
    case CURSOR_MODE_FORCE_DISABLED:
        GRuntime->SetCursorEnabled( false );
        break;
    default:
        AN_ASSERT( 0 );
    }

    for ( AInputComponent * component = AInputComponent::GetInputComponents() ; component ; component = component->GetNext() ) {
        component->UpdateAxes( FrameDurationInSeconds );
    }
}

void AEngineInstance::MapWindowCoordinate( float & InOutX, float & InOutY ) const
{
    SVideoMode const & videoMode = GRuntime->GetVideoMode();
    InOutX += videoMode.X;
    InOutY += videoMode.Y;
}

void AEngineInstance::UnmapWindowCoordinate( float & InOutX, float & InOutY ) const
{
    SVideoMode const & videoMode = GRuntime->GetVideoMode();
    InOutX -= videoMode.X;
    InOutY -= videoMode.Y;
}

void AEngineInstance::SetDesktop( WDesktop * _Desktop )
{
    if ( IsSame( Desktop, _Desktop ) ) {
        return;
    }

    //if ( Desktop ) {
    //    Desktop->SetFocusWidget( nullptr );
    //}

    Desktop = _Desktop;

    if ( Desktop ) {
        // Force update transform
        Desktop->MarkTransformDirty();

        // Set size
        SVideoMode const & videoMode = GRuntime->GetVideoMode();
        Desktop->SetSize( videoMode.FramebufferWidth, videoMode.FramebufferHeight );
    }
}
