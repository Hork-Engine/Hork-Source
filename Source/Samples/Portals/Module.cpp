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

#include "Module.h"
#include "Player.h"
#include "Checker.h"

#include <World/Public/World.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Canvas.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/Widgets/WViewport.h>

#include <Core/Public/Image.h>
#include <Runtime/Public/Runtime.h>

AN_CLASS_META( AModule )

AModule * GModule;

void AModule::OnGameStart() {

    GModule = this;

    SetInputMappings();
    CreateResources();

    World = AWorld::CreateWorld();

    TPodArray< SPortalDef > portals;
    TPodArray< Float3 > hullVerts;

    portals.Resize( 5 );

    ALevel * level = NewObject< ALevel >();
    World->AddLevel( level );
    //ALevel * level = World->GetPersistentLevel();

    Float3 position;
    Float3 halfExtents;

    level->Areas.Resize( 4 );
    level->Areas.ZeroMem();

    position = Float3(-1,0,0);
    halfExtents = Float3(2.0f) * 0.5f;
    level->Areas[0].Bounds.Mins = position - halfExtents;
    level->Areas[0].Bounds.Maxs = position + halfExtents;

    position = Float3(-3,0,0);
    halfExtents = Float3(2.0f) * 0.5f;
    level->Areas[1].Bounds.Mins = position - halfExtents;
    level->Areas[1].Bounds.Maxs = position + halfExtents;

    position = Float3(1,0,0);
    halfExtents = Float3(2.0f) * 0.5f;
    level->Areas[2].Bounds.Mins = position - halfExtents;
    level->Areas[2].Bounds.Maxs = position + halfExtents;

    position = Float3(1,0,3);
    halfExtents = Float3(2,2,4) * 0.5f;
    level->Areas[3].Bounds.Mins = position - halfExtents;
    level->Areas[3].Bounds.Maxs = position + halfExtents;


    Float3 points[4] = {
        Float3( 0, 0.2f, -0.2f ),
        Float3( 0, 0.8f, -0.2f ),
        Float3( 0, 0.8f, 0.2f ),
        Float3( 0, 0.2f, 0.2f )
    };
    portals[0].FirstVert = hullVerts.Size();
    portals[0].NumVerts = 4;
    portals[0].Areas[0] = 0;
    portals[0].Areas[1] = 2;
    hullVerts.Append( points, 4 );

    for ( int i = 0 ; i < 4 ; i++ ) {
        points[i].X -= 2;
    }
    portals[1].FirstVert = hullVerts.Size();
    portals[1].NumVerts = 4;
    portals[1].Areas[0] = 0;
    portals[1].Areas[1] = 1;
    hullVerts.Append( points, 4 );

    Float3 points2[4] = {
        Float3( 0.2f, 0.2f, 1 ),
        Float3( 0.4f, 0.2f, 1 ),
        Float3( 0.4f, 0.8f, 1 ),
        Float3( 0.2f, 0.8f, 1 )
    };
    portals[2].FirstVert = hullVerts.Size();
    portals[2].NumVerts = 4;
    portals[2].Areas[0] = 2;
    portals[2].Areas[1] = 3;
    hullVerts.Append( points2, 4 );

    for ( int i = 0 ; i < 4 ; i++ ) {
        points[i].X -= 2;
    }
    portals[3].FirstVert = hullVerts.Size();
    portals[3].NumVerts = 4;
    portals[3].Areas[0] = -1;
    portals[3].Areas[1] = 1;
    hullVerts.Append( points, 4 );

    for ( int i = 0 ; i < 4 ; i++ ) {
        points2[i].Z += 4;
    }
    portals[4].FirstVert = hullVerts.Size();
    portals[4].NumVerts = 4;
    portals[4].Areas[0] = 3;
    portals[4].Areas[1] = -1;
    hullVerts.Append( points2, 4 );

    level->CreatePortals( portals.ToPtr(), portals.Size(), hullVerts.ToPtr() );
    level->Initialize();

    // Spawn HUD
    //AHUD * hud = World->SpawnActor< AMyHUD >();

    RenderingParams = NewObject< ARenderingParameters >();
    RenderingParams->BackgroundColor = AColor4::Black();
    RenderingParams->bClearBackground = true;
    RenderingParams->bWireframe = false;
    RenderingParams->bDrawDebug = true;

    STransform t;
    t.Rotation = Quat::Identity();
    t.Scale = Float3(0.1f);
    //int n = 0;
    for ( int i = 0 ; i < 30 ; i++ )
        for ( int j = 0 ; j < 14 ; j++ )
            for ( int k = 0 ; k < 30 ; k++ ) {

                Float3 pos( i,j,k );

                pos += Float3(-8, -4, -2 ) * 2.0f;

                t.Position = pos*0.25;//*0.2f;

                World->SpawnActor< AChecker >( t, level );
                //GLogger.Printf("n %d\n",++n);
            }

    //GLogger.Printf( "sizeof AChecker %u sizeof AActor %u\n", sizeof(AChecker), sizeof(AActor) );

    Float3 center(0);
    for ( int i = 0 ; i < 4 ; i++ ) {
        center += points2[i];
    }
    t.Position = center/4.0f;
    t.Scale = Float3(0.1f,0.1f,3);
    World->SpawnActor< AChecker >( t, level );

    APlayer * player = World->SpawnActor< APlayer >( Float3(0,0.2f,1), Quat::Identity(), level );

    //World->SpawnActor< FAtmosphere >();


    // Spawn player controller
    PlayerController = World->SpawnActor< APlayerController >();
    PlayerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
    PlayerController->SetInputMappings( InputMappings );
    PlayerController->SetRenderingParameters( RenderingParams );
    //PlayerController->SetHUD( hud );
    PlayerController->GetInputComponent()->MouseSensitivity = 0.3f;

    PlayerController->SetPawn( player );

    WDesktop * desktop = NewObject< WDesktop >();
    GEngine.SetDesktop( desktop );

    desktop->AddWidget(
        &WWidget::New< WViewport >()
        .SetPlayerController( PlayerController )
        .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
        .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
        .SetFocus()
    );
}

void AModule::SetInputMappings() {
    InputMappings = NewObject< AInputMappings >();

    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_W, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_S, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_A, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_D, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveUp", ID_KEYBOARD, KEY_SPACE, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveDown", ID_KEYBOARD, KEY_C, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_MOUSE, MOUSE_AXIS_X, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnUp", ID_MOUSE, MOUSE_AXIS_Y, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Speed", ID_KEYBOARD, KEY_LEFT_SHIFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F12, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
}

void AModule::CreateResources() {
    // Texture Blank512
    {
        GetOrCreateResource< ATexture >( "Blank512", "/Common/blank512.png" );
    }

    // CheckerMaterialInstance
    {
        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/Unlit" ) );
        static TStaticResourceFinder< ATexture > TextureResource( _CTS( "Blank512" ) );
        AMaterialInstance * CheckerMaterialInstance = NewObject< AMaterialInstance >();
        CheckerMaterialInstance->SetMaterial( MaterialResource.GetObject() );
        CheckerMaterialInstance->SetTexture( 0, TextureResource.GetObject() );
        RegisterResource( CheckerMaterialInstance, "CheckerMaterialInstance" );
    }

    // Checker mesh
    {
        static TStaticResourceFinder< AMaterialInstance > MaterialInst( _CTS( "CheckerMaterialInstance" ) );
        AIndexedMesh * CheckerMesh = NewObject< AIndexedMesh >();
        CheckerMesh->InitializeFromFile( "/Default/Meshes/Sphere" );
        CheckerMesh->SetMaterialInstance( 0, MaterialInst.GetObject() );
        RegisterResource( CheckerMesh, "CheckerMesh" );
    }

    // Skybox texture
    {
        const char * Cubemap[6] = {
            "DarkSky/rt.tga",
            "DarkSky/lt.tga",
            "DarkSky/up.tga",
            "DarkSky/dn.tga",
            "DarkSky/bk.tga",
            "DarkSky/ft.tga"
        };
        AImage rt, lt, up, dn, bk, ft;
        AImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
        rt.Load( Cubemap[0], nullptr, IMAGE_PF_BGR16F );
        lt.Load( Cubemap[1], nullptr, IMAGE_PF_BGR16F );
        up.Load( Cubemap[2], nullptr, IMAGE_PF_BGR16F );
        dn.Load( Cubemap[3], nullptr, IMAGE_PF_BGR16F );
        bk.Load( Cubemap[4], nullptr, IMAGE_PF_BGR16F );
        ft.Load( Cubemap[5], nullptr, IMAGE_PF_BGR16F );
        //TODO: convert to 16F
        //const float HDRI_Scale = 4.0f;
        //const float HDRI_Pow = 1.1f;
        //for ( int i = 0 ; i < 6 ; i++ ) {
        //    float * HDRI = (float*)cubeFaces[i]->pRawData;
        //    int count = cubeFaces[i]->Width*cubeFaces[i]->Height*3;
        //    for ( int j = 0; j < count ; j += 3 ) {
        //        HDRI[j] = StdPow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
        //        HDRI[j + 1] = StdPow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
        //        HDRI[j + 2] = StdPow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
        //    }
        //}
        ATexture * SkyboxTexture = NewObject< ATexture >();
        SkyboxTexture->InitializeCubemapFromImages( cubeFaces );
        RegisterResource( SkyboxTexture, "SkyboxTexture" );
    }

    // Skybox material instance
    {
        static TStaticResourceFinder< AMaterial > SkyboxMaterial( _CTS( "/Default/Materials/Skybox" ) );
        static TStaticResourceFinder< ATexture > SkyboxTexture( _CTS( "SkyboxTexture" ) );
        AMaterialInstance * SkyboxMaterialInstance = NewObject< AMaterialInstance >();
        SkyboxMaterialInstance->SetMaterial( SkyboxMaterial.GetObject() );
        SkyboxMaterialInstance->SetTexture( 0, SkyboxTexture.GetObject() );
        RegisterResource( SkyboxMaterialInstance, "SkyboxMaterialInstance" );
    }
}

#include <Runtime/Public/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "AngieEngine: Portals",
    // Root path
    "Samples/Portals",
    // Module class
    &AModule::ClassMeta()
};

AN_ENTRY_DECL( ModuleDecl )

