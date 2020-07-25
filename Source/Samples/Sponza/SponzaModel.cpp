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
#include <World/Public/Actors/SpotLight.h>
#include <World/Public/Resource/Asset.h>
#include <World/Public/Resource/AssetImporter.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/Widgets/WViewport.h>
#include <Runtime/Public/Runtime.h>

AN_CLASS_META( ASponzaModel )

ASponzaModel * GModule;

void ASponzaModel::OnGameStart() {

    GModule = this;

    GEngine.bAllowConsole = true;

    //SVideoMode videoMode = GRuntime.GetVideoMode();
    //videoMode.Width = 1280;
    //videoMode.Height = 720;
    //videoMode.RefreshRate = 60;
    //videoMode.Opacity = 1.0f;
    //videoMode.bFullscreen = false;
    //Core::Strcpy( videoMode.Backend, sizeof( videoMode.Backend ), "OpenGL 4.5" );
    //Core::Strcpy( videoMode.Title, sizeof( videoMode.Title ), "AngieEngine: Sponza" );
    //GRuntime.PostChangeVideoMode( videoMode );

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
#if 1
        //AAssetImporter importer;
        //SAssetImportSettings importSettings;
        //importSettings.OutputPath = "Skybox";
        //importSettings.ExplicitSkyboxFaces[0] = "ClearSky/rt.bmp";
        //importSettings.ExplicitSkyboxFaces[1] = "ClearSky/lt.bmp";
        //importSettings.ExplicitSkyboxFaces[2] = "ClearSky/up.bmp";
        //importSettings.ExplicitSkyboxFaces[3] = "ClearSky/dn.bmp";
        //importSettings.ExplicitSkyboxFaces[4] = "ClearSky/bk.bmp";
        //importSettings.ExplicitSkyboxFaces[5] = "ClearSky/ft.bmp";
        //importSettings.bImportSkyboxExplicit = true;
        //importSettings.bSkyboxHDRI = true;
        //importSettings.SkyboxHDRIScale = 4.0f;
        //importSettings.SkyboxHDRIPow = 1.1f;
        //importSettings.bCreateSkyboxMaterialInstance = true;
        //importer.ImportSkybox( importSettings );
        //AAssetImporter importer;
        //SAssetImportSettings importSettings;
        //importSettings.OutputPath = "Skybox2";
        //importSettings.ExplicitSkyboxFaces[0] = "DarkSky/rt.tga";
        //importSettings.ExplicitSkyboxFaces[1] = "DarkSky/lt.tga";
        //importSettings.ExplicitSkyboxFaces[2] = "DarkSky/up.tga";
        //importSettings.ExplicitSkyboxFaces[3] = "DarkSky/dn.tga";
        //importSettings.ExplicitSkyboxFaces[4] = "DarkSky/bk.tga";
        //importSettings.ExplicitSkyboxFaces[5] = "DarkSky/ft.tga";
        //importSettings.bImportSkyboxExplicit = true;
        //importSettings.bSkyboxHDRI = true;
        //importSettings.SkyboxHDRIScale = 4.0f;
        //importSettings.SkyboxHDRIPow = 1.1f;
        //importSettings.bCreateSkyboxMaterialInstance = true;
        //importer.ImportSkybox( importSettings );

        //AAssetImporter importer;
        //SAssetImportSettings importSettings;
        //importSettings.OutputPath = "Skybox3";
        //importSettings.ExplicitSkyboxFaces[0] = "ClearSky/rt.bmp";
        //importSettings.ExplicitSkyboxFaces[1] = "ClearSky/lt.bmp";
        //importSettings.ExplicitSkyboxFaces[2] = "ClearSky/up.bmp";
        //importSettings.ExplicitSkyboxFaces[3] = "ClearSky/dn.bmp";
        //importSettings.ExplicitSkyboxFaces[4] = "ClearSky/bk.bmp";
        //importSettings.ExplicitSkyboxFaces[5] = "ClearSky/ft.bmp";
        //importSettings.bImportSkyboxExplicit = true;
        //importSettings.bSkyboxHDRI = true;
        //importSettings.SkyboxHDRIScale = 1.0f;
        //importSettings.SkyboxHDRIPow = 1.0f;
        //importSettings.bCreateSkyboxMaterialInstance = true;
        //importer.ImportSkybox( importSettings );


        //AAssetImporter importer;
        //SAssetImportSettings importSettings;
        //importSettings.OutputPath = "Skybox6";
        //importSettings.ExplicitSkyboxFaces[0] = "cubemap_0.hdr";
        //importSettings.ExplicitSkyboxFaces[1] = "cubemap_1.hdr";
        //importSettings.ExplicitSkyboxFaces[2] = "cubemap_2.hdr";
        //importSettings.ExplicitSkyboxFaces[3] = "cubemap_3.hdr";
        //importSettings.ExplicitSkyboxFaces[4] = "cubemap_4.hdr";
        //importSettings.ExplicitSkyboxFaces[5] = "cubemap_5.hdr";
        //importSettings.bImportSkyboxExplicit = true;
        //importSettings.bSkyboxHDRI = true;
        //importSettings.SkyboxHDRIScale = 1.0f;
        //importSettings.SkyboxHDRIPow = 1.0f;
        //importSettings.bCreateSkyboxMaterialInstance = true;
        //importer.ImportSkybox( importSettings );

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
        rt.Load( Cubemap[0], nullptr, IMAGE_PF_BGR32F );
        lt.Load( Cubemap[1], nullptr, IMAGE_PF_BGR32F );
        up.Load( Cubemap[2], nullptr, IMAGE_PF_BGR32F );
        dn.Load( Cubemap[3], nullptr, IMAGE_PF_BGR32F );
        bk.Load( Cubemap[4], nullptr, IMAGE_PF_BGR32F );
        ft.Load( Cubemap[5], nullptr, IMAGE_PF_BGR32F );
        const float HDRI_Scale = 4.0f;
        const float HDRI_Pow = 1.1f;
        for ( int i = 0 ; i < 6 ; i++ ) {
            float * HDRI = (float*)cubeFaces[i]->pRawData;
            int count = cubeFaces[i]->Width*cubeFaces[i]->Height*3;
            for ( int j = 0; j < count ; j += 3 ) {
                HDRI[j] = Math::Pow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 1] = Math::Pow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 2] = Math::Pow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
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

    Quat r;
    //r.FromAngles( 0, Math::_HALF_PI, 0 );
    r.SetIdentity();
    APlayer * player = World->SpawnActor< APlayer >( Float3(0,1.6f,-0.36f), r );

    LoadStaticMeshes();

    
#if 1
    // Spawn directional light
    ADirectionalLight * dirlight = World->SpawnActor< ADirectionalLight >();
    dirlight->LightComponent->bCastShadow = true;
    //dirlight->LightComponent->SetDirection( Float3( -0.5f, -2, -0.5f ) );

    dirlight->LightComponent->SetDirection( Float3( -0.75f, -2, -0.2f ) );

    //dirlight->LightComponent->SetDirection( Float3( -0.2f, -2, 2.0f ) );

    //dirlight->LightComponent->SetColor( 1,0.3f,0.2f );
    dirlight->LightComponent->SetTemperature( 6500 );
#endif

    //AImageBasedLight * ibl = World->SpawnActor< AImageBasedLight >();
    //ibl->IBLComponent->SetPosition( 4,0,0 );
    //ibl->IBLComponent->SetRadius( 5.0f );
    //ibl->IBLComponent->SetIrradianceMap( 0 );
    //ibl->IBLComponent->SetReflectionMap( 0 );

    //AImageBasedLight * ibl2 = World->SpawnActor< AImageBasedLight >();
    //ibl2->IBLComponent->SetPosition( 0, 0, 0 );
    //ibl2->IBLComponent->SetRadius( 5.0f );
    //ibl2->IBLComponent->SetIrradianceMap( 0 );
    //ibl2->IBLComponent->SetReflectionMap( 0 );

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

    //{
    //    AAssetImporter importer;
    //    SAssetImportSettings importSettings;
    //    importSettings.ImportFile = "Import/space_smugglers_club_house_-_dark_version/scene.gltf";
    //    importSettings.OutputPath = "Indoor2";
    //    importSettings.bImportMeshes = true;
    //    importSettings.bImportMaterials = true;
    //    importSettings.bImportSkinning = true;
    //    importSettings.bImportSkeleton = true;
    //    importSettings.bImportAnimations = true;
    //    importSettings.bImportTextures = true;
    //    importSettings.bSingleModel = true;
    //    importSettings.bMergePrimitives = true;
    //    importSettings.Scale = 1;//0.01f;
    //    importer.ImportGLTF( importSettings );
    //}
    
    //{
    //    AAssetImporter importer;
    //    SAssetImportSettings importSettings;
    //    importSettings.ImportFile = "Import/xyz_-_bake/scene.gltf";
    //    importSettings.OutputPath = "Indoor1";
    //    importSettings.bImportMeshes = true;
    //    importSettings.bImportMaterials = true;
    //    importSettings.bImportSkinning = true;
    //    importSettings.bImportSkeleton = true;
    //    importSettings.bImportAnimations = true;
    //    importSettings.bImportTextures = true;
    //    importSettings.bSingleModel = true;
    //    importSettings.bMergePrimitives = true;
    //    importSettings.Scale = 0.01f;
    //    importer.ImportGLTF( importSettings );
    //}

#if 0
    {
        AAssetImporter importer;
        SAssetImportSettings importSettings;
        importSettings.ImportFile = "Import/space_station_scene_hd/scene.gltf";
        importSettings.OutputPath = "SpaceStation";
        importSettings.bImportMeshes = true;
        importSettings.bImportMaterials = true;
        importSettings.bImportSkinning = true;
        importSettings.bImportSkeleton = true;
        importSettings.bImportAnimations = true;
        importSettings.bImportTextures = true;
        importSettings.bSingleModel = false;
        importSettings.bMergePrimitives = true;
        importSettings.Scale = 0.1f;
        importer.ImportGLTF( importSettings );
    }


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
    q.FromAngles( 0, Math::_HALF_PI, 0 );
    AGargoyle * gargoyle = World->SpawnActor< AGargoyle >( Float3( -8, 0, 0 ), q );
    //gargoyle->RootComponent->SetScale(0.01f);
    AN_UNUSED( gargoyle );


#if 1
#if 0

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

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( -2, 2, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/cacodemon/scene_Mesh.asset" ) );
    //}



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

    {
        STransform transform;
        transform.Clear();
        transform.SetScale( 0.15f );
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( transform );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/knight_artorias/scene_Mesh.asset" ) );
    }
    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/artorias/artorias_Mesh.asset" ) );
    //}

    {
        STransform transform;
        transform.Clear();
        transform.Position.X = -1.5f;
        transform.Position.Y = -0.1f;
        transform.SetScale( 0.15f );
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( transform );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Lantern/Lantern_Mesh.asset" ) );
    }
    

//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/cupid/scene_Mesh.asset" ) );
//    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, 0 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/fallen_titan/scene_Mesh.asset" ) );
    //}
    
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


    {
        STransform transform;
        transform.Clear();
        transform.Position.X = 13;
        transform.Position.Y = 0;
        transform.SetScale( 0.5f );
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( transform );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Corset/Corset_Mesh.asset" ) );
    }
    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 16, 0, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/DamagedHelmet/DamagedHelmet_mesh_helmet_LP_13930damagedHelmet.asset" ) );
    }
    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 19, 0, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SciFiHelmet/SciFiHelmet_SciFiHelmet.asset" ) );
    }
    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 22, 0, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/FlightHelmet/FlightHelmet_Mesh.asset" ) );
    }    
    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 26, 0, 0 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/BoomBox/BoomBox_Mesh.asset" ) );
    }

    {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, -5 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/MetalRoughSpheres/MetalRoughSpheres_Mesh.asset" ) );
    }
        {
        AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 0, -15 ), Quat::Identity() );
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/EnvironmentTest/EnvironmentTest_Mesh.asset" ) );
    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, -15 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_e1m1/scene_Mesh.asset" ) );
    //    actor->GetComponent< AMeshComponent >()->SetCastShadow( false );
    //}

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0, 3, -15 ), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/doom_2/scene_Mesh.asset" ) );
    //    actor->GetComponent< AMeshComponent >()->SetCastShadow( false );
    //}



//    {
//        AStaticMesh * actor = World->SpawnActor< AStaticMesh >();
//        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SponzaSingleMesh/Sponza_Mesh.asset" ) );
//    }

    //{
    //    AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3(2,1,-1.5f), Quat::Identity() );
    //    actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Monster/Monster_Mesh.asset" ) );
    //}
#else
    //World->SpawnActor< ABrainStem >( Float3( -11, 11.55f, -1.5f ), Quat::Identity() );
    World->SpawnActor< ABrainStem >( Float3(3,0,-1.5f), Quat::Identity() );
    World->SpawnActor< ABrainStem >( Float3(0,0,-1.5f), Quat::Identity() );
    //World->SpawnActor< AMonster >( Float3(-2,1,-1.5f), Quat::Identity() );

    {
        AStaticMesh * actor;
        actor = World->SpawnActor< AStaticMesh >();
        actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Sponza2/Sponza_Mesh.asset" ) );
        for ( int i = 1; i <= 24; i++ ) {
            actor = World->SpawnActor< AStaticMesh >();
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( ( AString("/Root/Sponza2/Sponza_Mesh_") + Math::ToString(i) + ".asset" ).CStr() ) );
        }
    }

    float lightTemperature = 5000;

    //status = IE_ReadFile( "E:/IES/leomoon-dot-com_ies-lights-pack/ies-lights-pack/area-light.ies", &PhotoData );
    //status = IE_ReadFile( "E:/IES/leomoon-dot-com_ies-lights-pack/ies-lights-pack/bollard.ies", &PhotoData );
    //status = IE_ReadFile( "E:/IES/leomoon-dot-com_ies-lights-pack/ies-lights-pack/display.ies", &PhotoData );
    //status = IE_ReadFile( "E:/IES/leomoon-dot-com_ies-lights-pack/ies-lights-pack/three-lobe-vee.ies", &PhotoData );

    APhotometricProfile * profile = NewObject< APhotometricProfile >();
    profile->InitializeFromFile( "/FS/E:/IES/leomoon-dot-com_ies-lights-pack/ies-lights-pack/three-lobe-vee.ies" );

    APhotometricProfile * profile2 = NewObject< APhotometricProfile >();
    profile2->InitializeFromFile( "/FS/E:/IES/leomoon-dot-com_ies-lights-pack/ies-lights-pack/bollard.ies" );

    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( 3.9f, 1, 1.15f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 4.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3( 0, 1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile );
        pointLight->LightComponent->SetPhotometricAsMask( false );
        pointLight->LightComponent->SetLuminousIntensityScale( 0.5f );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( -5.0f+0.05f, 1, 1.15f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 4.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3( 0, 1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile );
        pointLight->LightComponent->SetPhotometricAsMask( false );
        pointLight->LightComponent->SetLuminousIntensityScale( 0.5f );
    }

    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( 3.9f, 1, 1.15f-3 ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 4.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3( 0, 1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile );
        pointLight->LightComponent->SetPhotometricAsMask( false );
        pointLight->LightComponent->SetLuminousIntensityScale( 0.5f );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( -5.0f+0.05f, 1, 1.15f-3 ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 4.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3( 0, 1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile );
        pointLight->LightComponent->SetPhotometricAsMask( false );
        pointLight->LightComponent->SetLuminousIntensityScale( 0.5f );
    }

    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( 0, 2.3f, 4.2f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 2.5f );
        //pointLight->LightComponent->SetColor( 1.0f, 0.5f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3(0,-1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile );
        pointLight->LightComponent->SetPhotometricAsMask( false );
        pointLight->LightComponent->SetLuminousIntensityScale( 0.5f );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( 0, 2.3f, -4.8f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 2.5f );
        //pointLight->LightComponent->SetColor( 1.0f, 0.5f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3( 0, -1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile );
        pointLight->LightComponent->SetPhotometricAsMask( false );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( 4, 2.3f, 4.2f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 1.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
        pointLight->LightComponent->SetDirection( Float3( 0, -1, 0 ) );
        pointLight->LightComponent->SetPhotometricProfile( profile2 );
        pointLight->LightComponent->SetPhotometricAsMask( false );
        pointLight->LightComponent->SetLuminousIntensityScale( 0.5f );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( 4, 1.3f, -3.8f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 2.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 1.0f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( -4, 1.3f, 3.2f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 2.5f );
        //pointLight->LightComponent->SetColor( 1.0f, 1.0f, 0.5f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( -4, 1.3f, -3.8f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 2.5f );
        //pointLight->LightComponent->SetColor( 1.0f, 1.0f, 0.5f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
    }
    {
        APointLight * pointLight = World->SpawnActor< APointLight >( Float3( -10.0f, 1.3f, -0.5f ), Quat::Identity() );
        pointLight->LightComponent->SetRadius( 2.5f );
        //pointLight->LightComponent->SetColor( 0.5f, 1.0f, 0.5f );
        pointLight->LightComponent->SetTemperature( lightTemperature );
        pointLight->LightComponent->SetLumens( 1700 );
    }
#endif

#else
    {
        for ( int i = 1 ; i <= 32 ; i++ ) {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( Core::Fmt( "/Root/SpaceStation/scene_Circle.0%02d_0.asset", i ) ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Circle_0.asset" ) );
        }
        for ( int i = 0 ; i <= 41 ; i++ ) {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( Core::Fmt( "/Root/SpaceStation/scene_Cube.0%02d_0.asset", i ) ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Cube_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Cube_1.asset" ) );
        }
        for ( int i = 1 ; i <= 10 ; i++ ) {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( Core::Fmt( "/Root/SpaceStation/scene_SF.0%02d_0.asset", i ) ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_SF_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Sphere.001_0.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Station_0.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Text.003_0.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.000_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.000_1.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.001_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.001_1.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.002_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.002_1.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.003_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.003_1.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.004_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.004_1.asset" ) );
        }

        for ( int i = 5 ; i <= 21 ; i++ ) {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( Core::Fmt( "/Root/SpaceStation/scene_Torus.0%02d_0.asset", i ) ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.021_1.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.022_0.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Torus.022_1.asset" ) );
        }
        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Under_Attack_0.asset" ) );
        }

        {
            AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
            actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/SpaceStation/scene_Warning_0.asset" ) );
        }
    }
#endif

    //{
    //AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
    //actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Indoor1/scene_Mesh.asset" ) );
    //    }

        //{
        //AStaticMesh * actor = World->SpawnActor< AStaticMesh >( Float3( 0 ), Quat::Identity() );
        //actor->SetMesh( GetOrCreateResource< AIndexedMesh >( "/Root/Indoor2/scene_Mesh.asset" ) );
        //}
    
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
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F11, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
}

#include <Runtime/Public/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "AngieEngine: Sponza",
    // Root path
    "Samples/Sponza",
    // Module class
    &ASponzaModel::ClassMeta()
};

AN_ENTRY_DECL( ModuleDecl )

//EXPORT COLLISIONS
//EXPORT RAYCAST BVH
//TEXTURE COMPRESSION
