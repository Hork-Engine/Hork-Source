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

#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/Console.h>
#include <Engine/World/Public/Canvas.h>
//#include <Engine/World/Public/ImguiContext.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Actor.h>
#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/RenderFrontend.h>
#include <Engine/World/Public/ResourceManager.h>
#include <Engine/World/Public/MaterialAssembly.h>

#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Runtime/Public/ImportExport.h>

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/CriticalError.h>

#include "FactoryLocal.h"

AN_CLASS_META_NO_ATTRIBS( IGameModule )

FGameMaster & GGameMaster = FGameMaster::Inst();

ImFont * GAngieFont;

static FCanvas GCanvas;

static size_t FrameMemoryUsed = 0;
static size_t FrameMemorySize = 0;

static float FractAvg = 1;
static float AxesFract = 1;

void FWorldSpawnParameters::SetTemplate( FWorld const * _Template ) {
    AN_Assert( &_Template->FinalClassMeta() == WorldTypeClassMeta );
    Template = _Template;
}

FGameMaster::FGameMaster() {
    bStopRequest = false;
    bInputFocus = false;
    bIsWindowVisible = false;
    WindowPosX = 0;
    WindowPosY = 0;
}

static void PrecacheResources( FClassMeta const & _ClassMeta ) {
    for ( FPrecacheMeta const * precache = _ClassMeta.GetPrecacheList() ; precache ; precache = precache->Next() ) {
        GLogger.Printf( "---------- Precache -----------\n"
                        "Resource Class: \"%s\"\n"
                        "Resource Path: \"%s\"\n",
                        precache->GetResourceClassMeta().GetName(),
                        precache->GetResourcePath() );

        GResourceManager.LoadResource( precache->GetResourceClassMeta(), precache->GetResourcePath() );
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

FWorld * FGameMaster::SpawnWorld( FWorldSpawnParameters const & _SpawnParameters ) {

    GLogger.Printf( "==== Spawn World ====\n" );

    FClassMeta const * classMeta = _SpawnParameters.WorldClassMeta();

    if ( !classMeta ) {
        GLogger.Printf( "FGameMaster::SpawnWorld: invalid world class\n" );
        return nullptr;
    }

    if ( classMeta->Factory() != &FWorld::Factory() ) {
        GLogger.Printf( "FGameMaster::SpawnWorld: not an world class\n" );
        return nullptr;
    }

    FWorld const * templateWorld = _SpawnParameters.GetTemplate();

    if ( templateWorld && classMeta != &templateWorld->ClassMeta() ) {
        GLogger.Printf( "FGameMaster::SpawnWorld: FWorldSpawnParameters::Template class doesn't match meta data\n" );
        return nullptr;
    }

    FWorld * world = static_cast< FWorld * >( classMeta->CreateInstance() );

    world->AddRef();

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Length() - 1;

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

FWorld * FGameMaster::LoadWorld( FDocument const & _Document, int _FieldsHead ) {

    GLogger.Printf( "==== Load World ====\n" );

    FDocumentField * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "FGameMaster::LoadWorld: invalid world class\n" );
        return nullptr;
    }

    FDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    FClassMeta const * classMeta = FWorld::Factory().LookupClass( classNameValue->Token.ToString().ToConstChar() );
    if ( !classMeta ) {
        GLogger.Printf( "FGameMaster::LoadWorld: invalid world class \"%s\"\n", classNameValue->Token.ToString().ToConstChar() );
        return nullptr;
    }

    FWorld * world = static_cast< FWorld * >( classMeta->CreateInstance() );

    world->AddRef();
    //world->bDuringConstruction = false;

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Length() - 1;

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

void FGameMaster::DeveloperKeys( FKeyEvent const & _Event ) {
    if ( _Event.Action == IE_Press ) {
        if ( _Event.Key == KEY_F1 ) {
            GLogger.Printf( "OpenGL Backend Test\n" );
            FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "OpenGL 4.5" );
            ResetVideoMode();
        } else if ( _Event.Key == KEY_F2 ) {
            GLogger.Printf( "Vulkan Backend Test\n" );
            FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "Vulkan" );
            ResetVideoMode();
        } else if ( _Event.Key == KEY_F3 ) {
            GLogger.Printf( "Null Backend Test\n" );
            FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), "Null" );
            ResetVideoMode();
        }
    }

    //if ( _Event.Action == IE_Press && _Event.Key == KEY_F12 ) {
    //    CriticalError( "This is a test of critical error\n" );
    //}

//    if ( _Event.Action == IE_Press && _Event.Key == KEY_C && ( HAS_MODIFIER( _Event.ModMask, MOD_CONTROL ) ) ) {
//        GRuntime.SetClipboard( "Some text" );
//    }
//    if ( _Event.Action == IE_Press && _Event.Key == KEY_V && ( HAS_MODIFIER( _Event.ModMask, MOD_CONTROL ) ) ) {
//        FString clipboard;
//        GRuntime.GetClipboard( clipboard );
//        GLogger.Print( clipboard.ToConstChar() );
//    }
}

void FGameMaster::OnKeyEvent( FKeyEvent const & _Event, double _TimeStamp ) {
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

    //ImguiContext->OnKeyEvent( _Event );

    if ( GConsole.IsActive() || bAllowConsole ) {
        GConsole.KeyEvent( _Event );
    }
    if ( GConsole.IsActive() && _Event.Action != IE_Release ) {
        return;
    }

    DeveloperKeys( _Event );

    UpdateInputAxes( FractAvg );

    for ( FInputComponent * component : FInputComponent::GetInputComponents() ) {
        if ( !component->bIgnoreKeyboardEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->SetButtonState( ID_KEYBOARD, _Event.Key, _Event.Action, _Event.ModMask, _TimeStamp );
        }
    }
}

void FGameMaster::OnMouseButtonEvent( FMouseButtonEvent const & _Event, double _TimeStamp ) {
    if ( GConsole.IsActive() ) {
        return;
    }

    //ImguiContext->OnMouseButtonEvent( _Event );

    UpdateInputAxes( FractAvg );

    for ( FInputComponent * component : FInputComponent::GetInputComponents() ) {
        if ( !component->bIgnoreJoystickEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->SetButtonState( ID_MOUSE, _Event.Button, _Event.Action, _Event.ModMask, _TimeStamp );
        }
    }
}

void FGameMaster::OnMouseWheelEvent( FMouseWheelEvent const & _Event, double _TimeStamp ) {
    //ImguiContext->OnMouseWheelEvent( _Event );

    GConsole.MouseWheelEvent( _Event );
    if ( GConsole.IsActive() ) {
        return;
    }

    UpdateInputAxes( FractAvg );

    for ( FInputComponent * component : FInputComponent::GetInputComponents() ) {
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

void FGameMaster::OnMouseMoveEvent( FMouseMoveEvent const & _Event, double _TimeStamp ) {
    if ( GConsole.IsActive() ) {
        return;
    }

    float x = _Event.X * MouseSensitivity;
    float y = _Event.Y * MouseSensitivity;

    AxesFract -= FractAvg;

    for ( FInputComponent * component : FInputComponent::GetInputComponents() ) {
        if ( !component->bIgnoreMouseEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->SetMouseAxisState( x, y );
        }

        if ( !bGamePaused ) {
            component->UpdateAxes( FractAvg, TimeScale );
        }

        if ( !component->bIgnoreMouseEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->SetMouseAxisState( 0, 0 );
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
}

void FGameMaster::OnCharEvent( FCharEvent const & _Event, double _TimeStamp ) {
    //ImguiContext->OnCharEvent( _Event );

    GConsole.CharEvent( _Event );
    if ( GConsole.IsActive() ) {
        return;
    }

    for ( FInputComponent * component : FInputComponent::GetInputComponents() ) {
        if ( !component->bIgnoreCharEvents /*&& ( component->ReceiveInputMask & RI_Mask )*/ ) {
            component->NotifyUnicodeCharacter( _Event.UnicodeCharacter, _Event.ModMask, _TimeStamp );
        }
    }
}

void FGameMaster::OnChangedVideoModeEvent( FChangedVideoModeEvent const & _Event ) {
    VideoMode.Width = _Event.Width;
    VideoMode.Height = _Event.Height;
    VideoMode.PhysicalMonitor = _Event.PhysicalMonitor;
    VideoMode.RefreshRate = _Event.RefreshRate;
    VideoMode.bFullscreen = _Event.bFullscreen;
    FString::CopySafe( VideoMode.Backend, sizeof( VideoMode.Backend ), _Event.Backend );

    FramebufferWidth = VideoMode.Width; // TODO
    FramebufferHeight = VideoMode.Height; // TODO
    RetinaScale = Float2( FramebufferWidth / VideoMode.Width, FramebufferHeight / VideoMode.Height );

    const float MM_To_Inch = 0.0393701f;

    if ( _Event.bFullscreen ) {
        FPhysicalMonitor const * monitor = GRuntime.GetMonitor( _Event.PhysicalMonitor );
        VideoAspectRatio = ( float )monitor->PhysicalWidthMM / monitor->PhysicalHeightMM;

        DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);
    } else {
        VideoAspectRatio = ( float )_Event.Width / _Event.Height;

        DPI_X = 1.0f / MM_To_Inch;
        DPI_Y = 1.0f / MM_To_Inch;
    }

    GConsole.Resize( VideoMode.Width );

    FBaseObject::ReloadAll();
}

void FGameMaster::ProcessEvent( FEvent const & _Event ) {
    switch ( _Event.Type ) {
    case ET_RuntimeUpdateEvent:

        //GLogger.Printf( "AxesFract %f\n", AxesFract );
        AN_Assert( AxesFract >= 1.0f );

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

void FGameMaster::ProcessEvents() {
    FEventQueue * eventQueue = GRuntime.ReadEvents();

    FEvent const * event;
    while ( nullptr != ( event = eventQueue->Pop() ) ) {
        ProcessEvent( *event );
    }

    AN_Assert( eventQueue->IsEmpty() );
}

void FGameMaster::SetVideoMode( unsigned short _Width, unsigned short _Height, unsigned short _PhysicalMonitor, byte _RefreshRate, bool _Fullscreen, const char * _Backend ) {
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

void FGameMaster::SetVideoMode( FVideoMode const & _VideoMode ) {
    SetVideoMode( _VideoMode.Width, _VideoMode.Height, _VideoMode.PhysicalMonitor, _VideoMode.RefreshRate, _VideoMode.bFullscreen, _VideoMode.Backend );
}

void FGameMaster::ResetVideoMode() {
    SetVideoMode( VideoMode );
}

FVideoMode const & FGameMaster::GetVideoMode() const {
    return VideoMode;
}

void FGameMaster::SetWindowDefs( float _Opacity, bool _Decorated, bool _AutoIconify, bool _Floating, const char * _Title ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetWindowDefsEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetWindowDefsEvent & data = event.Data.SetWindowDefsEvent;
    data.Opacity = Float(_Opacity).Clamp( 0.0f, 1.0f ) * 255.0f;
    data.bDecorated = _Decorated;
    data.bAutoIconify = _AutoIconify;
    data.bFloating = _Floating;
    FString::CopySafe( data.Title, sizeof( data.Title ), _Title );
}

void FGameMaster::SetWindowPos( int _X, int _Y ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetWindowPosEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetWindowPosEvent & data = event.Data.SetWindowPosEvent;
    data.PositionX = WindowPosX = _X;
    data.PositionY = WindowPosY = _Y;
}

void FGameMaster::GetWindowPos( int & _X, int & _Y ) {
    _X = WindowPosX;
    _Y = WindowPosY;
}

void FGameMaster::SetInputFocus() {
    FEvent & event = SendEvent();
    event.Type = ET_SetInputFocusEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
}

void FGameMaster::SetRenderFeatures( int _VSyncMode ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetRenderFeaturesEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetRenderFeaturesEvent & data = event.Data.SetRenderFeaturesEvent;
    data.VSyncMode = _VSyncMode;
}

void FGameMaster::SetCursorEnabled( bool _Enabled ) {
    FEvent & event = SendEvent();
    event.Type = ET_SetCursorModeEvent;
    event.TimeStamp = GRuntime.SysSeconds_d();
    FSetCursorModeEvent & data = event.Data.SetCursorModeEvent;
    data.bDisabledCursor = !_Enabled;
}

FEvent & FGameMaster::SendEvent() {
    FEventQueue * queue = GRuntime.WriteEvents();
    return *queue->Push();
}

void FGameMaster::MapWindowCoordinate( float & InOutX, float & InOutY ) {
    InOutX += WindowPosX;
    InOutY += WindowPosY;
}

void FGameMaster::UnmapWindowCoordinate( float & InOutX, float & InOutY ) {
    InOutX -= WindowPosX;
    InOutY -= WindowPosY;
}

void FGameMaster::DestroyWorlds() {
    GLogger.Printf( "FGameMaster::DestroyWorlds()\n" );

    for ( FWorld * world : Worlds ) {
        world->Destroy();
    }
}

void FGameMaster::Tick( float _TimeStep ) {
    //GLogger.Printf( "FGameMaster::Tick()\n" );

    //GRuntime.WaitMilliseconds(1000);

    if ( bGamePauseRequest ) {
        bGamePauseRequest = false;
        bGamePaused = true;
        GLogger.Printf( "Game paused\n" );
    } else if ( bGameUnpauseRequest ) {
        bGameUnpauseRequest = false;
        bGamePaused = false;
        GLogger.Printf( "Game unpaused\n" );
    }

    GameModule->OnPreGameTick( _TimeStep );
    for ( FWorld * world : Worlds ) {
        world->Tick( _TimeStep );
    }
    GameModule->OnPostGameTick( _TimeStep );

    KickoffPendingKillWorlds();
}

void FGameMaster::KickoffPendingKillWorlds() {
    while ( PendingKillWorlds ) {
        FWorld * world = PendingKillWorlds;
        FWorld * nextWorld;

        PendingKillWorlds = nullptr;

        while ( world ) {
            nextWorld = world->NextPendingKillWorld;

            // FIXME: Call world->EndPlay here?

            // Remove world from game array of worlds
            Worlds[ world->IndexInGameArrayOfWorlds ] = Worlds[ Worlds.Length() - 1 ];
            Worlds[ world->IndexInGameArrayOfWorlds ]->IndexInGameArrayOfWorlds = world->IndexInGameArrayOfWorlds;
            Worlds.RemoveLast();
            world->IndexInGameArrayOfWorlds = -1;
            world->RemoveRef();

            world = nextWorld;
        }
    }
}

//class FPhysicsWorld : public btCollisionDispatcher, public btSoftRigidDynamicsWorld {
//public:
//    btDbvtBroadphase Broadphase;
//    btDefaultCollisionConfiguration CollisionConfiguration;
//    btSequentialImpulseConstraintSolver ConstraintSolver;
//    FPhysicsWorld()
//        : btCollisionDispatcher( &CollisionConfiguration )
//        , btSoftRigidDynamicsWorld( this,
//                                    &Broadphase,
//                                    &ConstraintSolver,
//                                    &CollisionConfiguration )
//    {
//    }
//};

//byte PhysWorld[sizeof(FPhysicsWorld)];

//FPhysicsWorld * FWorld::GetPhysWorld() {
//    return reinterpret_cast< FPhysicsWorld * >( &PhysWorld[0] );
//}

//FBulletPhysicsIntegration

void FGameMaster::UpdateInputAxes( float _Fract ) {
    if ( _Fract <= 0 ) {
        return;
    }

    AxesFract -= _Fract;

    if ( bGamePaused ) {
        return;
    }

    for ( FInputComponent * component : FInputComponent::GetInputComponents() ) {
        component->UpdateAxes( _Fract, TimeScale );
    }
}


static FTexture * FontAtlasTexture;
static ImFontAtlas FontAtlas;

static void CreateAngieFont() {
    unsigned char * pixels;
    int atlasWidth, atlasHeight;

    // Create font atlas
    //Font = FontAtlas.AddFontFromFileTTF( "DejaVuSansMono.ttf", 16, NULL, FontAtlas.GetGlyphRangesCyrillic() );
    GAngieFont = FontAtlas.AddFontFromFileTTF( "DroidSansMono.ttf", 16, NULL, FontAtlas.GetGlyphRangesCyrillic() );

    // Get atlas raw data
    FontAtlas.GetTexDataAsAlpha8( &pixels, &atlasWidth, &atlasHeight );

    // Create atlas texture
    FontAtlasTexture = static_cast< FTexture * >( FTexture::ClassMeta().CreateInstance() );
    FontAtlasTexture->AddRef();
    FontAtlasTexture->Initialize2D( TEXTURE_PF_R8, 1, atlasWidth, atlasHeight );
    void * pPixels = FontAtlasTexture->WriteTextureData( 0,0,0, atlasWidth, atlasHeight, 0 );
    if ( pPixels ) {
        memcpy( pPixels, pixels, atlasWidth*atlasHeight );
    }

    FontAtlas.TexID = FontAtlasTexture->GetRenderProxy();
}

static void DestroyAngieFont() {
    FontAtlas.Clear();
    FontAtlasTexture->RemoveRef();
}

static void *imgui_alloc(size_t sz, void*) {
    return GMainMemoryZone.Alloc( sz, 1 );
}

static void imgui_free(void *ptr, void*) {
    GMainMemoryZone.Dealloc(ptr);
}

void FGameMaster::Run() {

    if ( SetCriticalMark() ) {
        // Critical error was emitted by this thread
        GRuntime.Terminate();
        return;
    }

    GConsole.ReadStoryLines();

    InitializeFactories();

    FGarbageCollector::Initialize();

    GRenderFrontend.Initialize();
    GResourceManager.Initialize();

    GameRunningTimeMicro = 0;
    GameRunningTimeMicroAfterTick = 0;
    GameplayTimeMicro = 0;
    GameplayTimeMicroAfterTick = 0;

    GameModule = CreateGameModuleCallback();

    GLogger.Printf( "Created game module: %s\n", GameModule->FinalClassName() );

    GRuntime.SwapFrameData();
    ProcessEvents();

    AxesFract = 1;

    GameModule->AddRef();
    GameModule->OnGameStart();

    ImGui::SetAllocatorFunctions( imgui_alloc, imgui_free, NULL );

    CreateAngieFont();

    GCanvas.Initialize();

    //ImguiContext = static_cast< FImguiContext * >( FImguiContext::ClassMeta().CreateInstance() );
    //ImguiContext->SetFontAtlas( &FontAtlas );
    //ImguiContext->AddRef();

    const int64_t tickTimeMicro = 1000000.0 / GameHertz;
    const float tickTimeSeconds = tickTimeMicro * 0.000001;

    int64_t residualTime = tickTimeMicro;
    int64_t frameDuration = tickTimeMicro;

    while ( 1 ) {
        FrameTimeStamp = GRuntime.SysMicroseconds();

        FrameDurationInSeconds = frameDuration * 0.000001;

        TimeScale = FrameDurationInSeconds * GameHertz;

        //GLogger.Printf( "---- game frame ---- (%f ms)\n", TimeScale );

        if ( IsCriticalError() ) {
            // Critical error in other thread was occured
            GRuntime.Terminate();
            return;
        }

        if ( bStopRequest ) {
            break;
        }

        ProcessEvents();

        UpdateInputAxes( AxesFract );

        int num = 0;
        while ( residualTime >= tickTimeMicro ) {

            //GLogger.Printf( "tick\n" );

            TickTimeStamp = GRuntime.SysMicroseconds();
            GameRunningTimeMicro = GameRunningTimeMicroAfterTick;
            GameplayTimeMicro = GameplayTimeMicroAfterTick;

            FGarbageCollector::DeallocateObjects();

            UpdateInputAxes( AxesFract );

            AxesFract = 1;
            TimeScale = 1;

            Tick( tickTimeSeconds );

            GameRunningTimeMicroAfterTick += tickTimeMicro;
            residualTime -= tickTimeMicro;

            UpdateGameplayTimer( tickTimeMicro );

            TickNumber++;

            int64_t tickDuration = GRuntime.SysMicroseconds() - FrameTimeStamp;

            num++;

            if ( tickDuration > tickTimeMicro ) {
                // Game tick is too long. Slowdown gameplay.
                GLogger.Printf( "Game tick is too long. Slowdown gameplay. (%d vs %d, num %d )\n", tickDuration, tickTimeMicro, num );
                residualTime = 0;
                break;
            }
        }

        AxesFract = 1;

        //ImguiContext->BeginFrame( FrameDurationInSeconds );

        DrawCanvas();

        //int64_t t = GRuntime.SysMilliseconds();
        GRenderFrontend.BuildFrameData( &GCanvas );
        //GLogger.Printf( "time BuildFrameData %d msec\n", GRuntime.SysMilliseconds() - t );

        //ImguiContext->EndFrame();

#if 1
        FrameMemoryUsed = GRuntime.GetFrameData()->FrameMemoryUsed;
        FrameMemorySize = GRuntime.GetFrameData()->FrameMemorySize;
        GRuntime.SwapFrameData();
#else
        const float timeOut = 1; // milliseconds
        while ( !GRuntime.SwapFrameDataTimeout( timeOut ) ) {
            // Do something...
            GLogger.Printf( "Wait...\n" );
        }
#endif

        // FIXME: here?
        //BuildNetworkPacket();

        frameDuration = GRuntime.SysMicroseconds() - FrameTimeStamp;
        residualTime += frameDuration;

        FrameNumber++;

        // Limit frame rate to game hertz
//        if ( residualTime < tickTimeMicro ) {
////            GLogger.Printf( "Still waiting for %d microseconds\n", tickTimeMicro - residualTime );
//            GRuntime.WaitMicroseconds( tickTimeMicro - residualTime );
//            residualTime = tickTimeMicro;
//            frameDuration = GRuntime.SysMicroseconds() - FrameTimeStamp;
//        }
    }

    GameModule->OnGameEnd();

    DestroyWorlds();
    KickoffPendingKillWorlds();

    FInputComponent::InputComponents.Free();

    GameModule->RemoveRef();
    GameModule = nullptr;

    //ImguiContext->RemoveRef();
    //ImguiContext = nullptr;

    GCanvas.Deinitialize();

    DestroyAngieFont();

    GResourceManager.Deinitialize();
    GRenderFrontend.Deinitialize();

    FGarbageCollector::Deinitialize();

    DeinitializeFactories();

    GConsole.WriteStoryLines();

    GRuntime.Terminate();
}

void FGameMaster::Stop() {
    bStopRequest = true;
}

void FGameMaster::SetGamePaused( bool _Paused ) {
    bGamePauseRequest = _Paused;
    bGameUnpauseRequest = !_Paused;
}

bool FGameMaster::IsGamePaused() const {
    return bGamePaused;
}

void FGameMaster::ResetGameplayTimer() {
    bResetGameplayTimer = true;
}

void FGameMaster::UpdateGameplayTimer( int64_t _TimeStep ) {
    if ( bResetGameplayTimer ) {
        bResetGameplayTimer = false;
        GameplayTimeMicroAfterTick = 0;
        return;
    }

    if ( bGamePaused ) {
        return;
    }

    GameplayTimeMicroAfterTick += _TimeStep;
}

void FGameMaster::DrawCanvas() {
    GCanvas.Begin( GAngieFont, VideoMode.Width, VideoMode.Height );

    // Draw game
    GameModule->DrawCanvas( &GCanvas );

    // Draw console
    GConsole.Draw( &GCanvas, FrameDurationInSeconds );

    // Draw debug
    if ( !GConsole.IsActive() ) {
        FRenderFrame * frameData = GRuntime.GetFrameData();

        Float2 pos( 8, 8 );
        const float y_step = 22;
        const int numLines = 8;

        pos.Y = GCanvas.Height - numLines * y_step;

        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("FPS: %d", int(1.0f / FrameDurationInSeconds) ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Zone memory usage: %f KB / %d MB", GMainMemoryZone.GetTotalMemoryUsage()/1024.0f, GMainMemoryZone.GetZoneMemorySizeInMegabytes() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Hunk memory usage: %f KB / %d MB", GMainHunkMemory.GetTotalMemoryUsage()/1024.0f, GMainHunkMemory.GetHunkMemorySizeInMegabytes() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Frame memory usage: %f KB / %d MB", FrameMemoryUsed/1024.0f, FrameMemorySize>>20 ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Heap memory usage: %f KB", GMainHeapMemory.GetTotalMemoryUsage()/1024.0f
        /*- GMainMemoryZone.GetZoneMemorySizeInMegabytes()*1024 - GMainHunkMemory.GetHunkMemorySizeInMegabytes()*1024 - 256*1024.0f*/ ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Visible instances: %d", frameData->Instances.Length() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Polycount: %d", GRenderFrontend.GetPolyCount() ) ); pos.Y += y_step;
        GCanvas.DrawTextUTF8( pos, 0xffffffff, FString::Fmt("Frontend time: %d msec", GRenderFrontend.GetFrontendTime() ) );
        
    }

    GCanvas.End();
}

void IGameModule::OnGameClose() {
    GGameMaster.Stop();
}

void _GameThreadMain() {
    GGameMaster.Run();
}

static void Con_Print( const char * _Message ) {
    GConsole.Print( _Message );
}

void ( *GameThreadMain )() = _GameThreadMain;
void (*GamePrintCallback)( const char * _Message ) = Con_Print;
