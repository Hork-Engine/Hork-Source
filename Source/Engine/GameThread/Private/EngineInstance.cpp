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

#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>

#include <Bullet3Common/b3Logging.h>
#include <Bullet3Common/b3AlignedAllocator.h>

#include <DetourAlloc.h>

#include "Console.h"
#include "ImguiContext.h"

#include <Runtime/Public/EntryDecl.h>

//#define IMGUI_CONTEXT

static ARuntimeVariable com_ShowStat( _CTS( "com_ShowStat" ), _CTS( "0" ) );
static ARuntimeVariable com_ShowFPS( _CTS( "com_ShowFPS" ), _CTS( "0" ) );

static AConsole Console;

AN_CLASS_META( AEngineCommands )

AEngineInstance & GEngine = AEngineInstance::Inst();

AEngineInstance::AEngineInstance() {
    RetinaScale = Float2( 1.0f );
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

#if 0
static void *ImguiModuleAlloc( size_t _BytesCount, void * ) {
    return GZoneMemory.Alloc( _BytesCount );
}

static void ImguiModuleFree( void * _Bytes, void * ) {
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

AEngineCommands::AEngineCommands()
{
    CommandContext.AddCommand( "quit", { this, &AEngineCommands::Quit }, "Quit from application" );
    CommandContext.AddCommand( "RebuildMaterials", { this, &AEngineCommands::RebuildMaterials }, "Rebuild materials" );
}

void AEngineCommands::Quit( ARuntimeCommandProcessor const & _Proc )
{
    GRuntime.PostTerminateEvent();
}

void AEngineCommands::RebuildMaterials( ARuntimeCommandProcessor const & _Proc )
{
    AMaterial::RebuildMaterials();
}

void AEngineInstance::AddCommand( const char * _Name, TCallback< void( ARuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment )
{
    EngineCmd->CommandContext.AddCommand( _Name, _Callback, _Comment );
}

void AEngineInstance::RemoveCommand( const char * _Name )
{
    EngineCmd->CommandContext.RemoveCommand( _Name );
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

    // Init recast navigation module
    dtAllocSetCustom( NavModuleAlloc, NavModuleFree );
#if 0
    // Init Imgui allocators
    ImGui::SetAllocatorFunctions( ImguiModuleAlloc, ImguiModuleFree, NULL );
#endif
    GResourceManager.Initialize();

    Renderer = CreateInstanceOf< ARenderFrontend >();

    RenderBackend = MakeRef< ARenderBackend >();

    GAudioSystem.Initialize();
    GAudioSystem.AddAudioDecoder( "ogg", CreateInstanceOf< AOggVorbisDecoder >() );
    GAudioSystem.AddAudioDecoder( "mp3", CreateInstanceOf< AMp3Decoder >() );
    GAudioSystem.AddAudioDecoder( "wav", CreateInstanceOf< AWavDecoder >() );

    AFont::SetGlyphRanges( GLYPH_RANGE_CYRILLIC );

    Canvas.Initialize();

    EngineCmd = CreateInstanceOf< AEngineCommands >();

    GameModule = CreateGameModule( _EntryDecl.ModuleClass );
    GameModule->AddRef();

    GLogger.Printf( "Created game module: %s\n", GameModule->FinalClassName() );

#ifdef IMGUI_CONTEXT
    ImguiContext = CreateInstanceOf< AImguiContext >();
    ImguiContext->SetFont( ACanvas::GetDefaultFont() );
    ImguiContext->AddRef();
#endif

    bAllowInputEvents = true;

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
        CommandProcessor.Execute( EngineCmd->CommandContext );

        // Tick worlds
        AWorld::UpdateWorlds( FrameDurationInSeconds );

        // Update audio system
        GAudioSystem.Update( APlayerController::GetCurrentAudioListener(), FrameDurationInSeconds );

        // Poll runtime events
        GRuntime.PollEvents();

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
        RenderBackend->RenderFrame( Renderer->GetFrameData() );

    } while ( !GRuntime.IsPendingTerminate() );

    bAllowInputEvents = false;

    GameModule->RemoveRef();
    GameModule = nullptr;

    Desktop.Reset();

    AWorld::DestroyWorlds();
    AWorld::KickoffPendingKillWorlds();

#ifdef IMGUI_CONTEXT
    ImguiContext->RemoveRef();
    ImguiContext = nullptr;
#endif

    EngineCmd.Reset();

    Canvas.Deinitialize();

    RenderBackend.Reset();

    Renderer.Reset();

    GResourceManager.Deinitialize();

    GAudioSystem.PurgeChannels();
    GAudioSystem.RemoveAudioDecoders();

    AGarbageCollector::Deinitialize();

    GPrimitiveLinkPool.Free();

    GAudioSystem.Deinitialize();

    DeinitializeFactories();

    Console.WriteStoryLines();
}

void AEngineInstance::DrawCanvas()
{
    SVideoMode const & videoMode = GRuntime.GetVideoMode();

    Canvas.Begin( videoMode.FramebufferWidth, videoMode.FramebufferHeight );

    if ( IsWindowVisible() ) {
        if ( Desktop ) {
            // Draw desktop
            Desktop->GenerateWindowHoverEvents();
            Desktop->GenerateDrawEvents( Canvas );
            if ( Desktop->IsCursorVisible() && !GRuntime.IsCursorEnabled() ) {
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
    if ( com_ShowStat ) {
        SRenderFrame * frameData = Renderer->GetFrameData();

        const size_t TotalMemorySizeInBytes = ( (GZoneMemory.GetZoneMemorySizeInMegabytes()<<20)
                                                + (GHunkMemory.GetHunkMemorySizeInMegabytes()<<20)
                                                + GRuntime.GetFrameMemorySize() );

        SRenderFrontendStat const & stat = Renderer->GetStat();

        AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();
        AStreamedMemoryGPU * streamedMemory = GRuntime.GetStreamedMemoryGPU();

        const float y_step = 22;
        const int numLines = 13;

        Float2 pos( 8, 8 );
        pos.Y = Canvas.Height - numLines * y_step;

        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Zone memory usage: %f KB / %d MB", GZoneMemory.GetTotalMemoryUsage()/1024.0f, GZoneMemory.GetZoneMemorySizeInMegabytes() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Hunk memory usage: %f KB / %d MB", GHunkMemory.GetTotalMemoryUsage()/1024.0f, GHunkMemory.GetHunkMemorySizeInMegabytes() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Frame memory usage: %f KB / %d MB (Max %f KB)", GRuntime.GetFrameMemoryUsedPrev()/1024.0f, GRuntime.GetFrameMemorySize()>>20, GRuntime.GetMaxFrameMemoryUsage()/1024.0f ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Frame memory usage (GPU): %f KB / %d MB (Max %f KB)", streamedMemory->GetUsedMemoryPrev()/1024.0f, streamedMemory->GetAllocatedMemory()>>20, streamedMemory->GetMaxMemoryUsage()/1024.0f ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Vertex cache memory usage (GPU): %f KB / %d MB", vertexMemory->GetUsedMemory()/1024.0f, vertexMemory->GetAllocatedMemory()>>20 ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Heap memory usage: %f KB", (GHeapMemory.GetTotalMemoryUsage()-TotalMemorySizeInBytes)/1024.0f ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Visible instances: %d", frameData->Instances.Size() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Visible shadow instances: %d", frameData->ShadowInstances.Size() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Visible dir lights: %d", frameData->DirectionalLights.Size() ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Polycount: %d", stat.PolyCount ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("ShadowMapPolyCount: %d", stat.ShadowMapPolyCount ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Frontend time: %d msec", stat.FrontendTime ) ); pos.Y += y_step;
        Canvas.DrawTextUTF8( pos, AColor4::White(), Core::Fmt("Active audio channels: %d", GAudioSystem.GetNumActiveChannels() ) ); pos.Y += y_step;
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
        Canvas.DrawTextUTF8( Float2( 10, 10 ), AColor4::White(), Core::Fmt( "Frame time %.1f ms (FPS: %d, AVG %d)", FrameDurationInSeconds*1000.0f, int( 1.0f / FrameDurationInSeconds ), int( fps+0.5f ) ) );
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

    if ( bQuitOnEscape && _Event.Action == IA_PRESS && _Event.Key == KEY_ESCAPE ) {
        GameModule->OnGameClose();
    }

    // Check Alt+Enter to toggle fullscreen/windowed mode
    if ( bToggleFullscreenAltEnter ) {
        if ( _Event.Action == IA_PRESS && _Event.Key == KEY_ENTER && ( HAS_MODIFIER( _Event.ModMask, KMOD_ALT ) ) ) {
            SVideoMode videoMode = GRuntime.GetVideoMode();
            videoMode.bFullscreen = !videoMode.bFullscreen;
            GRuntime.PostChangeVideoMode( videoMode );
        }
    }

#ifdef IMGUI_CONTEXT
    ImguiContext->OnKeyEvent( _Event );
#endif

    DeveloperKeys( _Event );

    if ( Console.IsActive() || bAllowConsole ) {
        Console.KeyEvent( _Event, EngineCmd->CommandContext, CommandProcessor );
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
        SVideoMode const & videoMode = GRuntime.GetVideoMode();

        if ( GRuntime.IsCursorEnabled() ) {
            Float2 cursorPosition;
            int x, y;

            GRuntime.GetCursorPosition( &x, &y );

            cursorPosition.X = Math::Clamp( x, 0, videoMode.FramebufferWidth-1 );
            cursorPosition.Y = Math::Clamp( y, 0, videoMode.FramebufferHeight-1 );

            Desktop->SetCursorPosition( cursorPosition );
        }
        else {
            Float2 cursorPosition = Desktop->GetCursorPosition();

            // Simulate ballistics
            const bool bSimulateCursorBallistics = true;
            if ( bSimulateCursorBallistics ) {
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
    SVideoMode const & videoMode = GRuntime.GetVideoMode();

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
    SVideoMode const & videoMode = GRuntime.GetVideoMode();

    switch ( CursorMode ) {
    case CURSOR_MODE_AUTO:
        if ( !videoMode.bFullscreen && Console.IsActive() ) {
            GRuntime.SetCursorEnabled( true );
        }
        else {
            GRuntime.SetCursorEnabled( false );
        }
        break;
    case CURSOR_MODE_FORCE_ENABLED:
        GRuntime.SetCursorEnabled( true );
        break;
    case CURSOR_MODE_FORCE_DISABLED:
        GRuntime.SetCursorEnabled( false );
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
    SVideoMode const & videoMode = GRuntime.GetVideoMode();
    InOutX += videoMode.X;
    InOutY += videoMode.Y;
}

void AEngineInstance::UnmapWindowCoordinate( float & InOutX, float & InOutY ) const
{
    SVideoMode const & videoMode = GRuntime.GetVideoMode();
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
        SVideoMode const & videoMode = GRuntime.GetVideoMode();
        Desktop->SetSize( videoMode.FramebufferWidth, videoMode.FramebufferHeight );
    }
}

IEngineInterface * GetEngineInstance()
{
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
