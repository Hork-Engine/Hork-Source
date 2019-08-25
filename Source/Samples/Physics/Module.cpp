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
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/Resource/Public/Asset.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/Resource/Public/Skeleton.h>

#include <Engine/Runtime/Public/EntryDecl.h>

AN_ENTRY_DECL( FModule )
AN_CLASS_META( FModule )

FModule * GModule;

void FModule::OnGameStart() {

    GModule = this;

    GGameEngine.bAllowConsole = true;
    //GGameEngine.MouseSensitivity = 0.15f;
    GGameEngine.MouseSensitivity = 0.3f;
    GGameEngine.SetRenderFeatures( VSync_Disabled );
    GGameEngine.SetWindowDefs( 1, true, false, false, "AngieEngine: Physics" );
    GGameEngine.SetVideoMode( 640,480,0,60,false,"OpenGL 4.5");
    //GGameEngine.SetVideoMode( 1920,1080,0,60,false,"OpenGL 4.5");
    GGameEngine.SetCursorEnabled( false );

    SetInputMappings();

    CreateResources();

    // Spawn world
    TWorldSpawnParameters< FWorld > WorldSpawnParameters;
    World = GGameEngine.SpawnWorld< FWorld >( WorldSpawnParameters );

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

    World->GetPersistentLevel()->BuildNavMesh();
}

void FModule::OnGameEnd() {
}

void FModule::CreateResources() {
    //
    // Example, how to create mesh resource and register it
    //
    {
        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->InitializePlaneMesh( 256, 256, 256 );
        mesh->SetName( "DefaultShapePlane256x256x256" );
        FCollisionBox * box = mesh->BodyComposition.AddCollisionBody< FCollisionBox >();
        box->HalfExtents.X = 128;
        box->HalfExtents.Y = 0.1f;
        box->HalfExtents.Z = 128;
        box->Position.Y -= box->HalfExtents.Y;
        RegisterResource( mesh );
    }

    CreateSofbodyPatchAndSkeleton();

    {
        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->InitializeInternalMesh( "*box*" );
        mesh->SetName( "ShapeBoxMesh" );
        RegisterResource( mesh );
    }

    {
        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->InitializeSphereMesh( 0.5f, 2, 32, 32 );
        mesh->SetName( "ShapeSphereMesh" );
        FCollisionSphere * collisionBody = mesh->BodyComposition.AddCollisionBody< FCollisionSphere >();
        collisionBody->Radius = 0.5f;
        RegisterResource( mesh );
    }

    {
        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->InitializeCylinderMesh( 0.5f, 1, 1, 32 );
        mesh->SetName( "ShapeCylinderMesh" );
//        FCollisionCylinder * collisionBody = mesh->BodyComposition.AddCollisionBody< FCollisionCylinder >();
//        collisionBody->HalfExtents = Float3( 0.5f );
//        collisionBody->Axial = FCollisionBody::AXIAL_X;
        FCollisionCapsule * collisionBody = mesh->BodyComposition.AddCollisionBody< FCollisionCapsule >();
        collisionBody->Radius = 0.5f;
        collisionBody->Height = 1;
        collisionBody->Axial = FCollisionBody::AXIAL_Z;

        RegisterResource( mesh );
    }

    //
    // Example, how to create texture resource from file
    //
    //CreateUniqueTexture( "mipmapchecker.png" );

    //
    // Example, how to create texture resource from file with alias
    //
    GetOrCreateResource< FTexture >( "mipmapchecker.png", "MipmapChecker" );

    // Default material
    {
        FMaterialProject * proj = NewObject< FMaterialProject >();
        FMaterialInTexCoordBlock * inTexCoordBlock = proj->AddBlock< FMaterialInTexCoordBlock >();
        FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
        FAssemblyNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );
        FMaterialTextureSlotBlock * diffuseTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
        diffuseTexture->AddressU = diffuseTexture->AddressV = diffuseTexture->AddressW = TEXTURE_ADDRESS_WRAP;
        FMaterialSamplerBlock * diffuseSampler = proj->AddBlock< FMaterialSamplerBlock >();
        diffuseSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        FMaterialUniformAddress * uniformAddress = proj->AddBlock< FMaterialUniformAddress >();
        uniformAddress->Address = 0;
        uniformAddress->Type = AT_Float4;

        FMaterialMulBlock * mul = proj->AddBlock< FMaterialMulBlock >();
        mul->ValueA->Connect( diffuseSampler, "RGBA" );
        mul->ValueB->Connect( uniformAddress, "Value" );

        FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
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
        FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();
        FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();

        //
        // VS_Dir = InPosition - ViewPostion.xyz;
        //
        FMaterialInViewPositionBlock * inViewPosition = proj->AddBlock< FMaterialInViewPositionBlock >();
        FMaterialSubBlock * positionMinusViewPosition = proj->AddBlock< FMaterialSubBlock >();
        positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
        positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
        materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
        FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
        NSV_Dir->Connect( /*positionMinusViewPosition*/inPositionBlock, "Value" );

        FMaterialAtmosphereBlock * atmo = proj->AddBlock< FMaterialAtmosphereBlock >();
        atmo->Dir->Connect( materialVertexStage, "Dir" );

        FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
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

void FModule::CreateSofbodyPatchAndSkeleton() {
    FIndexedMesh * mesh = NewObject< FIndexedMesh >();
    float s = 4;

#if 0
    mesh->InitializePatchMesh(
        Float3( -s, 0, -s ),
        Float3( s, 0, -s ),
        Float3( -s, 0, s ),
        Float3( s, 0, s ),
        17, 17,
        2,
        false );
#endif

    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    TPodArray< FMeshVertexJoint > weights;
    TPodArray< FJoint > joints;
    BvAxisAlignedBox bounds;

    // Generate path vertices, indices and bounds
    CreatePatchMesh( vertices,
                     indices,
                     bounds,
                     Float3( -s, 0, -s ),
                     Float3( s, 0, -s ),
                     Float3( -s, 0, s ),
                     Float3( s, 0, s ),
                     8, 8,
                     2,
                     true );

    // Patch is two sided, so joints count is only half of vertices count
    int numJoints = vertices.Size() >> 1;

    joints.Resize( numJoints );

    // Generate vertex weights for skinning
    weights.Resize( vertices.Size() );
    for ( int i = 0 ; i < weights.Size() ; i++ ) {
        FMeshVertexJoint & w = weights[i];
        w.JointIndices[0] = w.JointIndices[1] = w.JointIndices[2] = w.JointIndices[3] = i%numJoints;
        w.JointWeights[0] = 255;
        w.JointWeights[1] = 0;
        w.JointWeights[2] = 0;
        w.JointWeights[3] = 0;
    }

    // Generate skeleton joints
    for ( int i = 0 ; i < numJoints ; i++ ) {
        FString::CopySafe( joints[i].Name, sizeof( joints[i].Name ), FString::Fmt( "joint_%d", i ) );
        //joints[i].JointOffsetMatrix.SetIdentity();

        joints[i].OffsetMatrix.Compose( -vertices[i].Position, Float3x3::Identity() );
        //joints[i].JointOffsetMatrix.Compose( vertices[i].Position, Float3x3::Identity() );

        //vertices[i].Position.Clear();
        //vertices[i+numJoints].Position.Clear();

        joints[i].Parent = -1;
    }

    // Initialize indexed mesh with skinning
    mesh->Initialize( vertices.Size(), indices.Size(), 1, true );
    mesh->WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    mesh->WriteIndexData( indices.ToPtr(), indices.Size(), 0 );
    mesh->WriteJointWeights( weights.ToPtr(), weights.Size(), 0 );
    mesh->GetSubpart( 0 )->SetBoundingBox( bounds );
    mesh->SetName( "SoftmeshPatch" );

    // Generate softbody faces
    int totalIndices = indices.Size() >> 1; // ignore back faces
    mesh->SoftbodyFaces.ResizeInvalidate( totalIndices / 3 );
    int faceIndex = 0;
    unsigned int const * pIndices = indices.ToPtr();
    for ( int i = 0; i < totalIndices; i += 3 ) {
        FSoftbodyFace & face = mesh->SoftbodyFaces[ faceIndex++ ];
        face.Indices[ 0 ] = pIndices[ i ];
        face.Indices[ 1 ] = pIndices[ i + 1 ];
        face.Indices[ 2 ] = pIndices[ i + 2 ];
    }

    // Generate softbody links
    mesh->GenerateSoftbodyLinksFromFaces();

    // Create skeleton
    FSkeleton * skel = NewObject< FSkeleton >();
    skel->Initialize( joints.ToPtr(), joints.Size(), bounds );
    skel->SetName( "SoftmeshSkeleton" );

    // Register mesh and skeleton
    RegisterResource( mesh );
    RegisterResource( skel );
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
