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
#include "StaticMesh.h"
#include "Ground.h"
#include "Player.h"

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/MeshAsset.h>
#include <Engine/World/Public/ResourceManager.h>

#include <Engine/Runtime/Public/EntryDecl.h>

AN_ENTRY_DECL( FModule )
AN_CLASS_META_NO_ATTRIBS( FModule )

FModule * GModule;

void FModule::OnGameStart() {

    GModule = this;

    GGameMaster.bAllowConsole = true;
    //GGameMaster.MouseSensitivity = 0.15f;
    GGameMaster.MouseSensitivity = 0.3f;
    GGameMaster.SetRenderFeatures( VSync_Disabled );
    GGameMaster.SetWindowDefs( 1, true, false, false, "AngieEngine: Physics" );
    GGameMaster.SetVideoMode( 640,480,0,60,false,"OpenGL 4.5");
    //GGameMaster.SetVideoMode( 1920,1080,0,60,false,"OpenGL 4.5");
    GGameMaster.SetCursorEnabled( false );

    SetInputMappings();

    //
    // Example, how to create mesh resource and register it
    //
    {
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    mesh->InitializeShape< FPlaneShape >( 256, 256, 256 );
    mesh->SetName( "DefaultShapePlane256x256x256" );
    mesh->BodyComposition.NewCollisionBody< FCollisionPlane >();
    RegisterResource( mesh );
    }

    {
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    mesh->InitializeInternalMesh( "*box*" );
    RegisterResource( mesh );
    }

    {
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    mesh->InitializeShape< FSphereShape >( 0.5f, 2, 32, 32 );
    mesh->SetName( "ShapeSphereMesh" );
    FCollisionSphere * collisionBody = mesh->BodyComposition.NewCollisionBody< FCollisionSphere >();
    collisionBody->Radius = 0.5f;
    RegisterResource( mesh );
    }

    {
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    mesh->InitializeShape< FCylinderShape >( 0.5f, 1, 1, 32 );
    mesh->SetName( "ShapeCylinderMesh" );
    FCollisionCylinder * collisionBody = mesh->BodyComposition.NewCollisionBody< FCollisionCylinder >();
    collisionBody->HalfExtents = Float3( 0.5f );
    RegisterResource( mesh );
    }

    //
    // Example, how to create texture resource from file
    //
    //CreateUniqueTexture( "mipmapchecker.png" );

    //
    // Example, how to create texture resource from file with alias
    //
    CreateResource< FTexture >( "mipmapchecker.png", "MipmapChecker" );


    // Spawn world
    TWorldSpawnParameters< FWorld > WorldSpawnParameters;
    World = GGameMaster.SpawnWorld< FWorld >( WorldSpawnParameters );

    // Spawn HUD
    //FHUD * hud = World->SpawnActor< FMyHUD >();

    RenderingParams = NewObject< FRenderingParameters >();
    RenderingParams->BackgroundColor = Float3(0.5f);
    RenderingParams->bWireframe = false;
    RenderingParams->bDrawDebug = false;

    CreateResources();

    //FPlayer * player = World->SpawnActor< FPlayer >( Float3(2.5f, 3.5f, 2.5f), Angl(-30,45,0).ToQuat() );
    FPlayer * player = World->SpawnActor< FPlayer >( Float3(2.5f, 0.0f, 2.5f), Angl(-30,45,0).ToQuat() );


    //LoadStaticMeshes();

    World->SpawnActor< FGround >();

    FBoxTrigger * trigger = World->SpawnActor< FBoxTrigger >( Float3(0,2,3), Quat::Identity() );
    trigger->RootComponent->SetScale( Float3(1,4,1) );

    FBoxTrigger * trigger2 = World->SpawnActor< FBoxTrigger >( Float3(0.1f,2,3.1f), Quat::Identity() );
    trigger2->RootComponent->SetScale( Float3(1,4,1) );

    FStaticBoxActor * staticBox = World->SpawnActor< FStaticBoxActor >( Float3(0,0,3), Quat::Identity() );
    staticBox->RootComponent->SetScale( Float3(2,1,2) );

    FStaticBoxActor * staticBox2 = World->SpawnActor< FStaticBoxActor >( Float3(0.5,0,3.5), Quat::Identity() );
    staticBox2->RootComponent->SetScale( Float3(2,1,2) );

    // Stair
    for ( int i = 0 ; i <16 ; i++ ) {
        FStaticBoxActor * box = World->SpawnActor< FStaticBoxActor >( Float3(i*0.5f,(i+0.5)*0.25f,0) + Float3(5,0,3), Quat::Identity() );
        box->RootComponent->SetScale( Float3(2,0.25f,2) );
    }

//    FTransform spawnTransform;
//    spawnTransform.Position = Float3(0,0,0);
//    spawnTransform.Rotation = Angl(0,45,0).ToQuat();
//    spawnTransform.Scale = Float3(10.0f,1.0f,1.0f);
//    World->SpawnActor< FSphereActor >( spawnTransform );

    

//    mesh = NewObject< FIndexedMesh >();
//    mesh->InitializeInternalMesh( "*box*" );
//    mesh->GetSubpart(0)->MaterialInstance = NewObject< FMaterialInstance >();
//    mesh->GetSubpart(0)->MaterialInstance->Material = Material;
//    mesh->GetSubpart(0)->MaterialInstance->SetTexture( 0, LoadCachedTexture( "blank512.png" ) );
//    actor = World->SpawnActor< FStaticMesh >();
//    actor->SetMesh( mesh );

    // Spawn player controller
    PlayerController = World->SpawnActor< FMyPlayerController >();
    PlayerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
    PlayerController->SetInputMappings( InputMappings );
    PlayerController->SetRenderingParameters( RenderingParams );
    //PlayerController->SetHUD( hud );

    PlayerController->SetPawn( player );
    PlayerController->SetViewCamera( player->Camera );
}

void FModule::OnGameEnd() {
}

void FModule::CreateResources() {
    // Default material
    {
        FMaterialProject * proj = NewObject< FMaterialProject >();
        FMaterialInTexCoordBlock * inTexCoordBlock = proj->NewBlock< FMaterialInTexCoordBlock >();
        FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
        FAssemblyNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );
        FMaterialTextureSlotBlock * diffuseTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
        diffuseTexture->AddressU = diffuseTexture->AddressV = diffuseTexture->AddressW = TEXTURE_ADDRESS_WRAP;
        FMaterialSamplerBlock * diffuseSampler = proj->NewBlock< FMaterialSamplerBlock >();
        diffuseSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        FMaterialUniformAddress * uniformAddress = proj->NewBlock< FMaterialUniformAddress >();
        uniformAddress->Address = 0;
        uniformAddress->Type = AT_Float4;

        FMaterialMulBlock * mul = proj->NewBlock< FMaterialMulBlock >();
        mul->ValueA->Connect( diffuseSampler, "RGBA" );
        mul->ValueB->Connect( uniformAddress, "Value" );

        FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
        materialFragmentStage->Color->Connect( mul, "Result" );
        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_UNLIT;
        builder->RegisterTextureSlot( diffuseTexture );

        FMaterial * material = builder->Build();
        material->SetName( "DefaultMaterial" );

        RegisterResource( material );
    }

    // Unit box
    {
        FIndexedMesh * unitBox = NewObject< FIndexedMesh >();
        unitBox->InitializeInternalMesh( "*box*" );
        unitBox->SetName( "UnitBox" );
        RegisterResource( unitBox );
    }

    // Skybox material
    {
        FMaterialProject * proj = NewObject< FMaterialProject >();

        //
        // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
        //
        FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
        FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();

        //
        // VS_Dir = InPosition - ViewPostion.xyz;
        //
        FMaterialInViewPositionBlock * inViewPosition = proj->NewBlock< FMaterialInViewPositionBlock >();
        FMaterialSubBlock * positionMinusViewPosition = proj->NewBlock< FMaterialSubBlock >();
        positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
        positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
        materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
        FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
        NSV_Dir->Connect( /*positionMinusViewPosition*/inPositionBlock, "Value" );

        FMaterialAtmosphereBlock * atmo = proj->NewBlock< FMaterialAtmosphereBlock >();
        atmo->Dir->Connect( materialVertexStage, "Dir" );

        FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
        materialFragmentStage->Color->Connect( atmo, "Result" );

        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_UNLIT;
        builder->MaterialFacing = MATERIAL_FACE_BACK;

        FMaterial * material = builder->Build();
        material->SetName( "SkyboxMaterial" );

        RegisterResource( material );
    }
}

void FModule::LoadStaticMeshes() {
#if 0
#if 1
    // Load as separate meshes
    FString fileName;

    for ( int i = 0 ; i < 25 ; i++ ) {
        fileName = "SponzaProject/Meshes/sponza_" + Int(i).ToString() + ".angie_mesh";

        FIndexedMesh * mesh = LoadCachedMesh( fileName.ToConstChar() );

        FStaticMesh * actor = World->SpawnActor< FStaticMesh >();

        actor->SetMesh( mesh );
    }
#else
    // Load as single mesh with subparts
    FIndexedMesh * mesh = LoadCachedMesh( "SponzaProject/Meshes/sponza.angie_mesh" );

    FStaticMesh * actor = World->SpawnActor< FStaticMesh >();

    actor->SetMesh( mesh );
#endif
#endif
}

void FModule::SetInputMappings() {
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
    InputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_LEFT, -90.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_RIGHT, 90.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Attack", ID_MOUSE, MOUSE_BUTTON_LEFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Speed", ID_KEYBOARD, KEY_LEFT_SHIFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F12, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
}

void FModule::DrawCanvas( FCanvas * _Canvas ) {
    _Canvas->DrawViewport( PlayerController, 0, 0, _Canvas->Width, _Canvas->Height );
}
