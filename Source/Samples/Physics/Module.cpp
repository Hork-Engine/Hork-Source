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
#include "Trigger.h"
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
    //GGameMaster.SetVideoMode( 640,480,0,60,false,"OpenGL 4.5");
    GGameMaster.SetVideoMode( 1920,1080,0,60,false,"OpenGL 4.5");
    GGameMaster.SetCursorEnabled( false );

    SetInputMappings();

    CreateResources();

    // Spawn world
    TWorldSpawnParameters< FWorld > WorldSpawnParameters;
    World = GGameMaster.SpawnWorld< FWorld >( WorldSpawnParameters );

    // Spawn HUD
    //FHUD * hud = World->SpawnActor< FMyHUD >();

    RenderingParams = NewObject< FRenderingParameters >();
    RenderingParams->BackgroundColor = Float3(0.5f);
    RenderingParams->bWireframe = false;
    RenderingParams->bDrawDebug = false;

    FPlayer * player = World->SpawnActor< FPlayer >( Float3( 0, 0.0f, 15.0f ), Quat::Identity() );

    FTransform spawnTransform;

    spawnTransform.Position = Float3( 0 );
    spawnTransform.Rotation = Quat::Identity();
    spawnTransform.Scale = Float3( 1 );
    World->SpawnActor< FGround >( spawnTransform );

    spawnTransform.Position = Float3( 4, 2, 0 );    
    spawnTransform.Scale = Float3( 2, 4, 2 );
    World->SpawnActor< FBoxTrigger >( spawnTransform );

    spawnTransform.Position = Float3( 10, 2, 0 );
    spawnTransform.Scale = Float3( 2, 4, 2 );
    World->SpawnActor< FBoxTrigger >( spawnTransform );

    spawnTransform.Position = Float3( 7, 0, 0 );
    spawnTransform.Scale = Float3( 8, 1, 8 );
    World->SpawnActor< FStaticBoxActor >( spawnTransform );

    // Stair
    for ( int i = 0 ; i <16 ; i++ ) {
        spawnTransform.Position = Float3( 0, ( i + 0.5 )*0.25f, -i*0.5f ) + Float3( -10, 0, 0 );
        spawnTransform.Scale = Float3( 2, 0.25f, 2 );
        World->SpawnActor< FStaticBoxActor >( spawnTransform );
    }

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
        float s = 4;
        mesh->InitializeShape< FPatchShape >(
            Float3( -s, 0, -s ),
            Float3( s, 0, -s ),
            Float3( -s, 0, s ),
            Float3( s, 0, s ),
            17, 17,
            2 );
        mesh->SetName( "SoftmeshPatch" );
        RegisterResource( mesh );
    }

    {
        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->InitializeInternalMesh( "*box*" );
        mesh->SetName( "ShapeBoxMesh" );
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
    InputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_LEFT, -90.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_RIGHT, 90.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "SpawnRandomShape", ID_MOUSE, MOUSE_BUTTON_LEFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "SpawnSoftBody", ID_MOUSE, MOUSE_BUTTON_RIGHT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "SpawnComposedActor", ID_MOUSE, MOUSE_BUTTON_MIDDLE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Speed", ID_KEYBOARD, KEY_LEFT_SHIFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F12, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveObjectForward", ID_KEYBOARD, KEY_UP, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveObjectForward", ID_KEYBOARD, KEY_DOWN, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveObjectRight", ID_KEYBOARD, KEY_RIGHT, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveObjectRight", ID_KEYBOARD, KEY_LEFT, -1.0f, CONTROLLER_PLAYER_1 );
}

void FModule::DrawCanvas( FCanvas * _Canvas ) {
    _Canvas->DrawViewport( PlayerController, 0, 0, _Canvas->Width, _Canvas->Height );
}
