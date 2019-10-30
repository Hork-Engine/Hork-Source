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

#include "SponzaModel.h"
#include "StaticMesh.h"
#include "Player.h"

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/Actors/DirectionalLight.h>
#include <Engine/World/Public/Actors/PointLight.h>
#include <Engine/Resource/Public/Asset.h>
#include <Engine/Resource/Public/ResourceManager.h>

AN_CLASS_META( FSponzaModel )

FSponzaModel * GModule;

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

void FSponzaModel::OnGameStart() {

    GModule = this;

    GEngine.bAllowConsole = true;
    //GEngine.MouseSensitivity = 0.15f;
    GEngine.MouseSensitivity = 0.3f;
    GEngine.SetWindowDefs( 1, true, false, false, "AngieEngine: Sponza" );
    //GEngine.SetVideoMode( 640,480,0,60,false,"OpenGL 4.5");
    GEngine.SetVideoMode( 1920,1080,0,60,false,"OpenGL 4.5");
    GEngine.SetCursorEnabled( false );

    SetInputMappings();

    World = FWorld::CreateWorld();

    // Spawn HUD
    //FHUD * hud = World->SpawnActor< FMyHUD >();

    RenderingParams = NewObject< FRenderingParameters >();
    RenderingParams->BackgroundColor = FColor4(0.5f);
    RenderingParams->bWireframe = false;
    RenderingParams->bDrawDebug = true;

    // Skybox texture
    {
        const char * Cubemap[6] = {
            "ClearSky/rt.bmp",
            "ClearSky/lt.bmp",
            "ClearSky/up.bmp",
            "ClearSky/dn.bmp",
            "ClearSky/bk.bmp",
            "ClearSky/ft.bmp"
        };
        FImage rt, lt, up, dn, bk, ft;
        FImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
        rt.LoadHDRI( Cubemap[0], false, false, 3 );
        lt.LoadHDRI( Cubemap[1], false, false, 3 );
        up.LoadHDRI( Cubemap[2], false, false, 3 );
        dn.LoadHDRI( Cubemap[3], false, false, 3 );
        bk.LoadHDRI( Cubemap[4], false, false, 3 );
        ft.LoadHDRI( Cubemap[5], false, false, 3 );
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

    // create texture resource from file with alias
    GetOrCreateResource< FTexture2D >( "mipmapchecker.png", "MipmapChecker" );

    Quat r;
    r.FromAngles( 0, FMath::_HALF_PI, 0 );
    FPlayer * player = World->SpawnActor< FPlayer >( Float3(0,1.6f,-0.36f), r );

    LoadStaticMeshes();

    FPointLight * pointLight = World->SpawnActor< FPointLight >( Float3(0,2,0), Quat::Identity() );
    pointLight->LightComponent->SetOuterRadius( 3.0f );
    pointLight->LightComponent->SetInnerRadius( 2.5f );
    pointLight->LightComponent->SetColor( 1, 0.5f, 0.5f );
    
    // Spawn directional light
    FDirectionalLight * dirlight = World->SpawnActor< FDirectionalLight >();
    dirlight->LightComponent->bCastShadow = true;
    dirlight->LightComponent->SetDirection( Float3( -0.5f, -2, -0.5f ) );

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

void FSponzaModel::OnGameEnd() {
}

void FSponzaModel::LoadStaticMeshes() {
#if 0
    static TStaticResourceFinder< FIndexedMesh > Mesh( _CTS( "glTF/SciFiHelmet.gltf" ) );
    static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.PBRMetallicRoughness" ) );
    static TStaticResourceFinder< FTexture2D > Diffuse( _CTS( "glTF/SciFiHelmet_BaseColor.png" ) );
    //static TStaticInternalResourceFinder< FTexture2D > Metallic( _CTS( "FTexture2D.Black" ) );
    //static TStaticResourceFinder< FTexture2D > Normal( _CTS( "glTF/SciFiHelmet_Normal.png" ) );
    //static TStaticResourceFinder< FTexture2D > Roughness( _CTS( "glTF/SciFiHelmet_MetallicRoughness.png" ) );


    FImage image;
    //image.LoadRawImage( "glTF/SciFiHelmet_BaseColor.png", true, true, 3 );
    //FTexture2D * Albedo = NewObject< FTexture2D >();
    //Albedo->InitializeFromImage( image );

    image.LoadRawImage( "glTF/SciFiHelmet_Normal.png", false, true, 3 );
    FTexture2D * Normal = NewObject< FTexture2D >();
    Normal->InitializeFromImage( image );

    image.LoadRawImage( "glTF/SciFiHelmet_MetallicRoughness.png", false, true, 3 );
    FTexture2D * MetallicRoughness = NewObject< FTexture2D >();
    MetallicRoughness->InitializeFromImage( image );
#else
    static TStaticResourceFinder< FIndexedMesh > Mesh( _CTS( "DamagedHelmet/glTF/DamagedHelmet.gltf" ) );
    static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.PBRMetallicRoughness" ) );
    static TStaticResourceFinder< FTexture2D > Diffuse( _CTS( "DamagedHelmet/glTF/Default_albedo.jpg" ) );
    //static TStaticInternalResourceFinder< FTexture2D > Metallic( _CTS( "FTexture2D.Black" ) );
    //static TStaticResourceFinder< FTexture2D > Normal( _CTS( "glTF/SciFiHelmet_Normal.png" ) );
    //static TStaticResourceFinder< FTexture2D > Roughness( _CTS( "glTF/SciFiHelmet_MetallicRoughness.png" ) );


    FImage image;
    //image.LoadRawImage( "glTF/SciFiHelmet_BaseColor.png", true, true, 3 );
    //FTexture2D * Albedo = NewObject< FTexture2D >();
    //Albedo->InitializeFromImage( image );

    image.LoadLDRI( "DamagedHelmet/glTF/Default_normal.jpg", false, true, 3 );
    FTexture2D * Normal = NewObject< FTexture2D >();
    Normal->InitializeFromImage( image );

    image.LoadLDRI( "DamagedHelmet/glTF/Default_metalRoughness.jpg", false, true, 3 );
    FTexture2D * MetallicRoughness = NewObject< FTexture2D >();
    MetallicRoughness->InitializeFromImage( image );

    image.LoadLDRI( "DamagedHelmet/glTF/Default_AO.jpg", false, true, 3 );
    FTexture2D * Ambient = NewObject< FTexture2D >();
    Ambient->InitializeFromImage( image );

    image.LoadLDRI( "DamagedHelmet/glTF/Default_emissive.jpg", false, true, 3 );
    FTexture2D * Emissive = NewObject< FTexture2D >();
    Emissive->InitializeFromImage( image );

#endif

    FMaterialInstance * materialInst = NewObject< FMaterialInstance >();
    materialInst->SetMaterial( MaterialResource.GetObject() );
    materialInst->SetTexture( 0, Diffuse.GetObject() );
    materialInst->SetTexture( 1, MetallicRoughness );
    materialInst->SetTexture( 2, Normal );
    materialInst->SetTexture( 3, Ambient );
    materialInst->SetTexture( 4, Emissive );

    Mesh.GetObject()->SetMaterialInstance( 0, materialInst );

    FStaticMesh * actor = World->SpawnActor< FStaticMesh >( Float3(0,3,0), Quat::RotationY( FMath::Radians(90.0f) ) );
    actor->SetMesh( Mesh.GetObject() );

    

#if 1
    //// Load as separate meshes
    FString fileName;

    for ( int i = 0 ; i < 25 ; i++ ) {
        fileName = "SponzaProject/Meshes/sponza_" + Int(i).ToString() + ".angie_mesh";

        actor = World->SpawnActor< FStaticMesh >();

        FIndexedMesh * mesh = GetOrCreateResource< FIndexedMesh >( fileName.ToConstChar() );
        //mesh->SetMaterialInstance( 0, materialInst );

        actor->SetMesh( mesh );
    }
#else
    // Load as single mesh with subparts
    FStaticMesh * actor = World->SpawnActor< FStaticMesh >();
    actor->SetMesh( GetOrCreateResource< FIndexedMesh >( "SponzaProject/Meshes/sponza.angie_mesh" ) );
#endif
}

void FSponzaModel::SetInputMappings() {
    InputMappings = NewObject< FInputMappings >();

    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_W, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_S, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_UP, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_DOWN, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_A, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_D, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveUp", ID_KEYBOARD, KEY_SPACE, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveDown", ID_KEYBOARD, KEY_C, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_MOUSE, MOUSE_AXIS_X, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnUp", ID_MOUSE, MOUSE_AXIS_Y, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_LEFT, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_RIGHT, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Speed", ID_KEYBOARD, KEY_LEFT_SHIFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Attack", ID_MOUSE, MOUSE_BUTTON_LEFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F12, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
}

#include <Engine/Runtime/Public/EntryDecl.h>

AN_ENTRY_DECL( FSponzaModel )
