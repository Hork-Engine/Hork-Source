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

#include "SponzaModel.h"
#include "StaticMesh.h"
#include "Player.h"

#include <World/Public/World.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Canvas.h>
#include <World/Public/Actors/DirectionalLight.h>
#include <World/Public/Actors/PointLight.h>
#include <World/Public/Resource/Asset.h>
#include <World/Public/Resource/AssetImporter.h>
#include <World/Public/Base/ResourceManager.h>

AN_CLASS_META( ASponzaModel )

const char * IGameModule::RootPath = "Samples/Sponza";

ASponzaModel * GModule;

class WMyDesktop : public WDesktop {
    AN_CLASS( WMyDesktop, WDesktop )

public:
    TRef< APlayerController > PlayerController;

protected:
    WMyDesktop() {
        SetDrawBackground( true );
    }

    void OnDrawBackground( ACanvas & _Canvas ) override {
        _Canvas.DrawViewport( PlayerController, 0, 0, _Canvas.Width, _Canvas.Height );
    }
};

AN_CLASS_META( WMyDesktop )

void ASponzaModel::OnGameStart() {

    GModule = this;

    GEngine.bAllowConsole = true;
    //GEngine.MouseSensitivity = 0.15f;
    GEngine.MouseSensitivity = 0.3f;
    GEngine.SetWindowDefs( 1, true, false, false, "AngieEngine: Sponza" );
    //GEngine.SetVideoMode( 640,480,0,60,false,"OpenGL 4.5");
    GEngine.SetVideoMode( 1920,1080,0,60,false,"OpenGL 4.5");
    GEngine.SetCursorEnabled( false );

    SetInputMappings();

    World = AWorld::CreateWorld();

    // Spawn HUD
    //AHUD * hud = World->SpawnActor< AMyHUD >();

    RenderingParams = NewObject< ARenderingParameters >();
    RenderingParams->BackgroundColor = AColor4(0.5f);
    RenderingParams->bWireframe = false;
    RenderingParams->bDrawDebug = true;

    // Skybox texture
    {
#if 0
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.OutputPath = "Skybox";
        importSettings.ExplicitSkyboxFaces[0] = "ClearSky/rt.bmp";
        importSettings.ExplicitSkyboxFaces[1] = "ClearSky/lt.bmp";
        importSettings.ExplicitSkyboxFaces[2] = "ClearSky/up.bmp";
        importSettings.ExplicitSkyboxFaces[3] = "ClearSky/dn.bmp";
        importSettings.ExplicitSkyboxFaces[4] = "ClearSky/bk.bmp";
        importSettings.ExplicitSkyboxFaces[5] = "ClearSky/ft.bmp";
        importSettings.bImportSkyboxExplicit = true;
        importSettings.bSkyboxHDRI = true;
        importSettings.SkyboxHDRIScale = 4.0f;
        importSettings.SkyboxHDRIPow = 1.1f;
        importSettings.bCreateSkyboxMaterialInstance = true;
        importer.ImportSkybox( importSettings );
#endif

        //ATexture* SkyboxTexture = GetOrCreateResource< ATexture >( "/Root/Skybox/Skybox_Texture.asset" );
        //RegisterResource( SkyboxTexture, "SkyboxTexture" );

#if 0
        const char * Cubemap[6] = {
            "ClearSky/rt.bmp",
            "ClearSky/lt.bmp",
            "ClearSky/up.bmp",
            "ClearSky/dn.bmp",
            "ClearSky/bk.bmp",
            "ClearSky/ft.bmp"
        };
        AImage rt, lt, up, dn, bk, ft;
        AImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
        rt.LoadHDRI( Cubemap[0], false, false, 3 );
        lt.LoadHDRI( Cubemap[1], false, false, 3 );
        up.LoadHDRI( Cubemap[2], false, false, 3 );
        dn.LoadHDRI( Cubemap[3], false, false, 3 );
        bk.LoadHDRI( Cubemap[4], false, false, 3 );
        ft.LoadHDRI( Cubemap[5], false, false, 3 );
        const float HDRI_Scale = 4.0f;
        const float HDRI_Pow = 1.1f;
        for ( int i = 0 ; i < 6 ; i++ ) {
            float * HDRI = (float*)cubeFaces[i]->pRawData;
            int count = cubeFaces[i]->Width*cubeFaces[i]->Height*3;
            for ( int j = 0; j < count ; j += 3 ) {
                HDRI[j] = pow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 1] = pow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 2] = pow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
            }
        }
        ATexture* SkyboxTexture = NewObject< ATexture >();
        SkyboxTexture->InitializeCubemapFromImages( cubeFaces );
        RegisterResource( SkyboxTexture, "SkyboxTexture" );
#endif
    }

    //// Skybox material instance
    //{
    //    static TStaticResourceFinder< AMaterial > SkyboxMaterial( _CTS( "/Default/Materials/Skybox" ) );
    //    static TStaticResourceFinder< ATexture > SkyboxTexture( _CTS( "SkyboxTexture" ) );
    //    AMaterialInstance * SkyboxMaterialInstance = NewObject< AMaterialInstance >();
    //    SkyboxMaterialInstance->SetMaterial( SkyboxMaterial.GetObject() );
    //    SkyboxMaterialInstance->SetTexture( 0, SkyboxTexture.GetObject() );
    //    RegisterResource( SkyboxMaterialInstance, "SkyboxMaterialInstance" );
    //}

    // create texture resource from file with alias
    GetOrCreateResource< ATexture >( "MipmapChecker", "/Common/mipmapchecker.png" );

    Quat r;
    //r.FromAngles( 0, Math::_HALF_PI, 0 );
    r.SetIdentity();
    APlayer * player = World->SpawnActor< APlayer >( Float3(0,1.6f,-0.36f), r );

    LoadStaticMeshes();

    APointLight * pointLight = World->SpawnActor< APointLight >( Float3(0,2,0), Quat::Identity() );
    pointLight->LightComponent->SetOuterRadius( 3.0f );
    pointLight->LightComponent->SetInnerRadius( 2.5f );
    pointLight->LightComponent->SetColor( 1, 0.5f, 0.5f );
    
    // Spawn directional light
    ADirectionalLight * dirlight = World->SpawnActor< ADirectionalLight >();
    dirlight->LightComponent->bCastShadow = true;
    dirlight->LightComponent->SetDirection( Float3( -0.5f, -2, -0.5f ) );

    // Spawn player controller
    PlayerController = World->SpawnActor< AMyPlayerController >();
    PlayerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
    PlayerController->SetInputMappings( InputMappings );
    PlayerController->SetRenderingParameters( RenderingParams );
    //PlayerController->SetHUD( hud );

    PlayerController->SetPawn( player );

    WMyDesktop * desktop = NewObject< WMyDesktop >();
    desktop->PlayerController = PlayerController;
    GEngine.SetDesktop( desktop );
}

void ASponzaModel::OnGameEnd() {
}










#include <World/Public/AnimationController.h>
#include <World/Public/Components/SkinnedComponent.h>

class ABrainStem : public APawn {
    AN_ACTOR( ABrainStem, APawn )

protected:
    ABrainStem();
    void Tick( float _TimeStep ) override;

private:
    TRef< AIndexedMesh > Mesh;
    ASkinnedComponent * SkinnedComponent;
};

AN_CLASS_META( ABrainStem )

ABrainStem::ABrainStem() {
    SkinnedComponent = CreateComponent< ASkinnedComponent >( "Skin" );

    AAnimationController * controller = NewObject< AAnimationController >();
    controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/BrainStem/BrainStem_Animation.asset" ) );
    controller->SetPlayMode( ANIMATION_PLAY_WRAP );
    SkinnedComponent->AddAnimationController( controller );

    SkinnedComponent->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/BrainStem/BrainStem_Mesh.asset" ) );
    SkinnedComponent->CopyMaterialsFromMeshResource();

    RootComponent = SkinnedComponent;
    bCanEverTick = true;
}

void ABrainStem::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    float time = GetWorld()->GetGameplayTimeMicro()*0.000001;

    SkinnedComponent->SetTimeBroadcast( time );
}



class AMonster : public APawn {
    AN_ACTOR( AMonster, APawn )

protected:
    AMonster();
    void Tick( float _TimeStep ) override;

private:
    TRef< AIndexedMesh > Mesh;
    ASkinnedComponent * SkinnedComponent;
};

AN_CLASS_META( AMonster )

AMonster::AMonster() {
    SkinnedComponent = CreateComponent< ASkinnedComponent >( "Skin" );

//    AAnimationController * controller = NewObject< AAnimationController >();
//    controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/Monster/Monster_Animation.asset" ) );
//    controller->SetPlayMode( ANIMATION_PLAY_WRAP );
//    SkinnedComponent->AddAnimationController( controller );

    

    //SkinnedComponent->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/cute_house_in_an_island/scene_Mesh.asset" ) );
    SkinnedComponent->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Monster/Monster_Mesh.asset" ) );
    SkinnedComponent->CopyMaterialsFromMeshResource();

    RootComponent = SkinnedComponent;
    bCanEverTick = true;
}

void AMonster::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    float time = GetWorld()->GetGameplayTimeMicro()*0.000001;

    SkinnedComponent->SetTimeBroadcast( time );
}








class AGargoyle : public APawn {
    AN_ACTOR( AGargoyle, APawn )

protected:
    AGargoyle();
    void Tick( float _TimeStep ) override;

private:
    TRef< AIndexedMesh > Mesh;
    ASkinnedComponent * SkinnedComponent;
};

AN_CLASS_META( AGargoyle )

AGargoyle::AGargoyle() {
    SkinnedComponent = CreateComponent< ASkinnedComponent >( "Skin" );

    AAnimationController * controller = NewObject< AAnimationController >();
    controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/doom_hell_knight/scene_CINEMA_4D_Main.asset" ) );
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation.asset" ) ); // jump
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation_1.asset" ) ); // attack
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation_2.asset" ) ); // attack
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation_3.asset" ) ); // growl
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation_4.asset" ) ); // idle
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation_5.asset" ) ); // fall
    //controller->SetAnimation( GetOrCreateResource< ASkeletalAnimation >( "/Root/gargoyle/scene_Animation_6.asset" ) ); // walk
    controller->SetPlayMode( ANIMATION_PLAY_WRAP );
    SkinnedComponent->AddAnimationController( controller );

    //SkinnedComponent->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/gargoyle/scene_Mesh.asset" ) );
    //SkinnedComponent->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/sea_keep_lonely_watcher/scene_Mesh.asset" ) );
    SkinnedComponent->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_hell_knight/scene_Mesh.asset" ) );
    SkinnedComponent->CopyMaterialsFromMeshResource();

    RootComponent = SkinnedComponent;
    bCanEverTick = true;
}

void AGargoyle::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    float time = GetWorld()->GetGameplayTimeMicro()*0.000001;

    SkinnedComponent->SetTimeBroadcast( time );
}

#include <Runtime/Public/ScopedTimeCheck.h>

void ASponzaModel::LoadStaticMeshes() {
    AScopedTimeCheck ScopedTime( "LoadStaticMeshes" );

#if 0
    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/DamagedHelmet/glTF/DamagedHelmet.gltf";
        importSettings.OutputPath = "DamagedHelmet";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = false;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/Sponza2/glTF/Sponza.gltf";
        importSettings.OutputPath = "Sponza2";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = false;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/Sponza2/glTF/Sponza.gltf";
        importSettings.OutputPath = "SponzaSingleMesh";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/SciFiHelmet/glTF/SciFiHelmet.gltf";
        importSettings.OutputPath = "SciFiHelmet";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = false;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/FlightHelmet/glTF/FlightHelmet.gltf";
        importSettings.OutputPath = "FlightHelmet";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 10.0f;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/BoomBox/glTF/BoomBox.gltf";
        importSettings.OutputPath = "BoomBox";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 100.0f;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/Lantern/glTF/Lantern.gltf";
        importSettings.OutputPath = "Lantern";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/EnvironmentTest/glTF/EnvironmentTest.gltf";
        importSettings.OutputPath = "EnvironmentTest";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/Corset/glTF/Corset.gltf";
        importSettings.OutputPath = "Corset";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 100.0f;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf";
        importSettings.OutputPath = "MetalRoughSpheres";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }



    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/BrainStem/glTF/BrainStem.gltf";
        importSettings.OutputPath = "BrainStem";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/Monster/glTF/Monster.gltf";
        importSettings.OutputPath = "Monster";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 0.001f;
        importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

     {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/cute_house_in_an_island/scene.gltf";
        importSettings.OutputPath = "cute_house_in_an_island";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/pirate_island/scene.gltf";
        importSettings.OutputPath = "pirate_island";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 0.01f;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/the_mill/scene.gltf";
        importSettings.OutputPath = "the_mill";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 0.01f;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/fallen_titan/scene.gltf";
        importSettings.OutputPath = "fallen_titan";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = false;//true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.01f;
        importer.ImportGLTF( importSettings );
    }
    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/cupid/scene.gltf";
        importSettings.OutputPath = "cupid";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.1f;
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/gargoyle/scene.gltf";
        importSettings.OutputPath = "gargoyle";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.01f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/gargoyle_illidan/scene.gltf";
        importSettings.OutputPath = "gargoyle_illidan";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.01f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/knight_artorias/scene.gltf";
        importSettings.OutputPath = "knight_artorias";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.005f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/gargoyle_of_the_milandes_castle/scene.gltf";
        importSettings.OutputPath = "gargoyle_of_the_milandes_castle";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        //importSettings.Scale = 0.01f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/sea_keep_lonely_watcher/scene.gltf";
        importSettings.OutputPath = "sea_keep_lonely_watcher";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.1f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }



        {
            AAssetImporter importer;
            SAssetImportSettings importSettings;
            importSettings.ImportFile = "Import/doom_-_hell_knight_-/scene.gltf";
            importSettings.OutputPath = "doom_hell_knight";
            importSettings.bImportMeshes = true;
            importSettings.bImportMaterials = true;
            importSettings.bImportSkinning = true;
            importSettings.bImportSkeleton = true;
            importSettings.bImportAnimations = true;
            importSettings.bImportTextures = true;
            importSettings.bSingleModel = true;
            importSettings.bMergePrimitives = true;
            //importSettings.bAllowUnlitMaterials = false;
            importSettings.Scale = 10.0f/32.0f;
            //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
            importer.ImportGLTF( importSettings );
        }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/doom_2016_-_plasma_gun/scene.gltf";
        importSettings.OutputPath = "doom_plasma_gun";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.1f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/doom_3_plazmatron/scene.gltf";
        importSettings.OutputPath = "doom_3_plazmatron";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.1f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/doom_plasma_rifle/scene.gltf";
        importSettings.OutputPath = "doom_plasma_rifle";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        //importSettings.bAllowUnlitMaterials = false;
        importSettings.Scale = 0.01f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }

    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/cacodemon/scene.gltf";
        importSettings.OutputPath = "cacodemon";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 10.0f/32.0f;
        importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }


    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/doom_e1m1_hangar_-_map/scene.gltf";
        importSettings.OutputPath = "doom_e1m1";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 1.0f/32.0f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }
    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/doom_2/scene.gltf";
        importSettings.OutputPath = "doom_2";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = true;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 1.0f/32.0f;
        //importSettings.Rotation.FromAngles(-Math::_HALF_PI,0,0);
        importer.ImportGLTF( importSettings );
    }
#endif

    Quat q;
    q.SetIdentity();
    //q.FromAngles( -Math::_HALF_PI, 0, 0 );
    AGargoyle * gargoyle = World->SpawnActor< AGargoyle >( Float3( 0, 0, 0 ), q );
    //gargoyle->RootComponent->SetScale(0.01f);
    AN_UNUSED(gargoyle);

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_plasma_gun/scene_Mesh.asset" ) );
//    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_3_plazmatron/scene_Mesh.asset" ) );
//    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_plasma_rifle/scene_Mesh.asset" ) );
//    }

    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( -2, 2, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/cacodemon/scene_Mesh.asset" ) );
    }



    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/sea_keep_lonely_watcher/scene_Mesh.asset" ) );
    //}

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/gargoyle_of_the_milandes_castle/scene_Mesh.asset" ) );
//    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/gargoyle_illidan/scene_Mesh.asset" ) );
    //}

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/knight_artorias/scene_Mesh.asset" ) );
//    }

    

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/cupid/scene_Mesh.asset" ) );
//    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/fallen_titan/scene_Mesh.asset" ) );
//    }
    
    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/the_mill/scene_Mesh.asset" ) );
    //}

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/pirate_island/scene_Mesh.asset" ) );
    //}

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/AirplaneProject/Level_Mesh.asset" ) );
//    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/artorias/artorias_Mesh.asset" ) );
    //}

#if 0
    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3(0,3,0), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/BoomBox/BoomBox_Mesh.asset" ) );
    }

    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 3, 3, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Corset/Corset_Mesh.asset" ) );
    }



    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 9, 3, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SciFiHelmet/SciFiHelmet_SciFiHelmet.asset" ) );
    }



    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 15, 3, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Lantern/Lantern_Mesh.asset" ) );
    }
#endif

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 12, 3, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/FlightHelmet/FlightHelmet_Mesh.asset" ) );
//    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 6, 3, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/DamagedHelmet/DamagedHelmet_mesh_helmet_LP_13930damagedHelmet.asset" ) );
//    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, -5 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/MetalRoughSpheres/MetalRoughSpheres_Mesh.asset" ) );
//    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, -15 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/EnvironmentTest/EnvironmentTest_Mesh.asset" ) );
//    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, -15 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_e1m1/scene_Mesh.asset" ) );
    //    actor->GetComponent< AMeshComponent >()->SetCastShadow( false );
    //}

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, -15 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_2/scene_Mesh.asset" ) );
//        actor->GetComponent< AMeshComponent >()->SetCastShadow( false );
//    }

    {
        AStaticMesh * actor;
        actor = World->SpawnActor< AStaticMesh >();
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Sponza2/Sponza_Mesh.asset" ) );
        for ( int i = 1; i <= 24; i++ ) {
            actor = World->SpawnActor< AStaticMesh >();
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( ( AString("/Root/Sponza2/Sponza_Mesh_") + Int(i).ToString() + ".asset" ).CStr() ) );
        }
    }

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >();
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SponzaSingleMesh/Sponza_Mesh.asset" ) );
//    }


//    {
//        ASkinnedMesh * actor = World->SpawnActor< ASkinnedMesh >( Float3(0,0,0), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SponzaSingleMesh/Sponza_Mesh.asset" ) );
//    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3(2,1,-1.5f), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Monster/Monster_Mesh.asset" ) );
    //}

    //World->SpawnActor< ABrainStem >( Float3( -11, 11.55f, -1.5f ), Quat::Identity() );
    World->SpawnActor< ABrainStem >( Float3(0,0,-1.5f), Quat::Identity() );
    World->SpawnActor< ABrainStem >( Float3(0,0,1.5f), Quat::Identity() );
    //World->SpawnActor< AMonster >( Float3(-2,1,-1.5f), Quat::Identity() );
}

void ASponzaModel::SetInputMappings() {
    InputMappings = NewObject< AInputMappings >();

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

#include <Runtime/Public/EntryDecl.h>

AN_ENTRY_DECL( ASponzaModel )


//EXPORT COLLISIONS
//EXPORT RAYCAST BVH
//TEXTURE COMPRESSION
