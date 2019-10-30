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

#include "Module.h"
#include "Player.h"
#include "Checker.h"

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/Resource/Public/ResourceManager.h>

AN_CLASS_META( FModule )

class WMyDesktop : public WDesktop {
    AN_CLASS( WMyDesktop, WDesktop )

public:
    TRef< FPlayerController > PlayerController;

protected:
    WMyDesktop() {
        SetDrawBackground( true );
    }

    void OnDrawBackground( FCanvas & _Canvas ) override {
        _Canvas.DrawViewport( PlayerController, 0, 0, _Canvas.Width, _Canvas.Height );
    }
};

AN_CLASS_META( WMyDesktop )

FModule * GModule;

void FModule::OnGameStart() {

    GModule = this;

    //GEngine.MouseSensitivity = 0.15f;
    GEngine.MouseSensitivity = 0.4f;
    //GEngine.SetVideoMode( 640,480,0,60,false,"OpenGL 4.5");
    GEngine.SetVideoMode( 1920,1080,0,60,false,"OpenGL 4.5");
    GEngine.SetCursorEnabled( false );
    GEngine.SetWindowDefs(1,true,false,false,"AngieEngine: Portals");

    SetInputMappings();
    CreateResources();

    World = FWorld::CreateWorld();

    FLevel * level = World->GetPersistentLevel();
    FLevelArea * area1 = level->AddArea( Float3(-1,0,0), Float3(2.0f), Float3(-1,0,0) );
    FLevelArea * area2 = level->AddArea( Float3(-3,0,0), Float3(2.0f), Float3(-3,0,0) );
    FLevelArea * area3 = level->AddArea( Float3(1,0,0), Float3(2.0f), Float3(1,0,0) );
    FLevelArea * area4 = level->AddArea( Float3(1,0,3), Float3(2.0f,2.0f,4.0f), Float3(1,0,3) );

    Float3 points[4] = {
        Float3( 0, 0.2f, -0.2f ),
        Float3( 0, 0.8f, -0.2f ),
        Float3( 0, 0.8f, 0.2f ),
        Float3( 0, 0.2f, 0.2f )
    };
    FLevelPortal * p0 = level->AddPortal( points, 4, area1, area3 );
    for ( int i = 0 ; i < 4 ; i++ ) {
        points[i].X -= 2;
    }
    FLevelPortal * p1 = level->AddPortal( points, 4, area1, area2 );
    Float3 points2[4] = {
        Float3( 0.2f, 0.2f, 1 ),
        Float3( 0.4f, 0.2f, 1 ),
        Float3( 0.4f, 0.8f, 1 ),
        Float3( 0.2f, 0.8f, 1 )
    };
    FLevelPortal * p2 = level->AddPortal( points2, 4, area3, area4 );

    for ( int i = 0 ; i < 4 ; i++ ) {
        points[i].X -= 2;
    }
    FLevelPortal * p3 = level->AddPortal( points, 4, NULL, area2 );

    for ( int i = 0 ; i < 4 ; i++ ) {
        points2[i].Z += 4;
    }
    FLevelPortal * p4 = level->AddPortal( points2, 4, area4, NULL );

    AN_UNUSED(p0);
    AN_UNUSED(p1);
    AN_UNUSED(p2);
    AN_UNUSED(p3);
    AN_UNUSED(p4);
    level->BuildPortals();

    // Spawn HUD
    //FHUD * hud = World->SpawnActor< FMyHUD >();

    RenderingParams = NewObject< FRenderingParameters >();
    RenderingParams->BackgroundColor = FColor4(0.5f);
    RenderingParams->bWireframe = false;
    RenderingParams->bDrawDebug = true;

    FTransform t;
    t.Rotation = Quat::Identity();
    t.Scale = Float3(0.1f);
    //int n = 0;
    for ( int i = 0 ; i < 28 ; i++ )
        for ( int j = 0 ; j < 14 ; j++ )
            for ( int k = 0 ; k < 28 ; k++ ) {

                Float3 pos( i,j,k );

                pos += Float3(-8, -4, -2 ) * 2.0f;

                t.Position = pos*0.25;//*0.2f;

                World->SpawnActor< FChecker >( t );
                //GLogger.Printf("n %d\n",++n);
            }

    //GLogger.Printf( "sizeof FChecker %u sizeof FActor %u\n", sizeof(FChecker), sizeof(FActor) );

    Float3 center(0);
    for ( int i = 0 ; i < 4 ; i++ ) {
        center += points2[i];
    }
    t.Position = center/4.0f;
    t.Scale = Float3(0.1f,0.1f,3);
    World->SpawnActor< FChecker >( t );

    FPlayer * player = World->SpawnActor< FPlayer >( Float3(0,0.2f,1), Quat::Identity() );

    //World->SpawnActor< FAtmosphere >();


    // Spawn player controller
    PlayerController = World->SpawnActor< FMyPlayerController >();
    PlayerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
    PlayerController->SetInputMappings( InputMappings );
    PlayerController->SetRenderingParameters( RenderingParams );
    //PlayerController->SetHUD( hud );

    PlayerController->SetPawn( player );
    PlayerController->SetViewCamera( player->Camera );

    WMyDesktop * desktop = NewObject< WMyDesktop >();
    desktop->PlayerController = PlayerController;
    GEngine.SetDesktop( desktop );
}

void FModule::SetInputMappings() {
    InputMappings = NewObject< FInputMappings >();

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

void FModule::CreateResources() {
    // Texture Blank512
    {
        GetOrCreateResource< FTexture2D >( "blank512.png", "Blank512" );
    }

    // CheckerMaterialInstance
    {
        static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.Default" ) );
        static TStaticResourceFinder< FTexture2D > TextureResource( _CTS( "Blank512" ) );
        FMaterialInstance * CheckerMaterialInstance = NewObject< FMaterialInstance >();
        CheckerMaterialInstance->SetMaterial( MaterialResource.GetObject() );
        CheckerMaterialInstance->SetTexture( 0, TextureResource.GetObject() );
        CheckerMaterialInstance->SetName( "CheckerMaterialInstance" );
        RegisterResource( CheckerMaterialInstance );
    }

    // Checker mesh
    {
        static TStaticResourceFinder< FMaterialInstance > MaterialInst( _CTS( "CheckerMaterialInstance" ) );
        FIndexedMesh * CheckerMesh = NewObject< FIndexedMesh >();
        CheckerMesh->InitializeInternalResource( "FIndexedMesh.Sphere" );
        CheckerMesh->SetName( "CheckerMesh" );
        CheckerMesh->SetMaterialInstance( 0, MaterialInst.GetObject() );
        RegisterResource( CheckerMesh );
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
        FImage rt, lt, up, dn, bk, ft;
        FImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
        rt.LoadHDRI( Cubemap[0], false, false, 3 );
        lt.LoadHDRI( Cubemap[1], false, false, 3 );
        up.LoadHDRI( Cubemap[2], false, false, 3 );
        dn.LoadHDRI( Cubemap[3], false, false, 3 );
        bk.LoadHDRI( Cubemap[4], false, false, 3 );
        ft.LoadHDRI( Cubemap[5], false, false, 3 );
        //const float HDRI_Scale = 4.0f;
        //const float HDRI_Pow = 1.1f;
        //for ( int i = 0 ; i < 6 ; i++ ) {
        //    float * HDRI = (float*)cubeFaces[i]->pRawData;
        //    int count = cubeFaces[i]->Width*cubeFaces[i]->Height*3;
        //    for ( int j = 0; j < count ; j += 3 ) {
        //        HDRI[j] = pow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
        //        HDRI[j + 1] = pow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
        //        HDRI[j + 2] = pow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
        //    }
        //}
        FTextureCubemap * SkyboxTexture = NewObject< FTextureCubemap >();
        SkyboxTexture->InitializeCubemapFromImages( cubeFaces );
        SkyboxTexture->SetName( "SkyboxTexture" );
        RegisterResource( SkyboxTexture );
    }

    // Skybox material instance
    {
        static TStaticInternalResourceFinder< FMaterial > SkyboxMaterial( _CTS( "FMaterial.Skybox" ) );
        static TStaticResourceFinder< FTextureCubemap > SkyboxTexture( _CTS( "SkyboxTexture" ) );
        FMaterialInstance * SkyboxMaterialInstance = NewObject< FMaterialInstance >();
        SkyboxMaterialInstance->SetName( "SkyboxMaterialInstance" );
        SkyboxMaterialInstance->SetMaterial( SkyboxMaterial.GetObject() );
        SkyboxMaterialInstance->SetTexture( 0, SkyboxTexture.GetObject() );
        RegisterResource( SkyboxMaterialInstance );
    }
}

#include <Engine/Runtime/Public/EntryDecl.h>

AN_ENTRY_DECL( FModule )
