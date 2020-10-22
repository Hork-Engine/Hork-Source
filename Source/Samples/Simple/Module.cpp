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

#include <World/Public/Components/InputComponent.h>
#include <World/Public/Components/MeshComponent.h>
#include <World/Public/Actors/DirectionalLight.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/MaterialGraph/MaterialGraph.h>
#include <World/Public/Widgets/WDesktop.h>
#include <GameThread/Public/EngineInstance.h>

class APlayer : public APawn
{
    AN_ACTOR( APlayer, APawn )

protected:
    AMeshComponent * Movable;
    ACameraComponent * Camera;
    ASceneComponent * Spin;

    APlayer()
    {
        Spin = CreateComponent< ASceneComponent >( "Spin" );

        static TStaticResourceFinder< AIndexedMesh > BoxMesh( _CTS( "/Default/Meshes/Box" ) );
        static TStaticResourceFinder< AMaterialInstance > BoxMaterialInst( _CTS( "BoxMaterialInstance" ) );
        Movable = CreateComponent< AMeshComponent >( "Movable" );
        Movable->SetMesh( BoxMesh.GetObject() );
        Movable->SetMaterialInstance( BoxMaterialInst.GetObject() );
        Movable->SetMotionBehavior( MB_KINEMATIC );
        Movable->AttachTo( Spin );

        Camera = CreateComponent< ACameraComponent >( "Camera" );
        Camera->SetPosition( 2, 4, 2 );
        Camera->SetAngles( -60, 45, 0 );
        Camera->AttachTo( Spin );

        RootComponent = Spin;
        PawnCamera = Camera;
    }

    void SetupPlayerInputComponent( AInputComponent * Input ) override
    {
        Input->BindAxis( "MoveForward", this, &APlayer::MoveForward );
        Input->BindAxis( "MoveRight", this, &APlayer::MoveRight );
        Input->BindAxis( "MoveUp", this, &APlayer::MoveUp );
        Input->BindAxis( "MoveDown", this, &APlayer::MoveDown );
        Input->BindAxis( "TurnRight", this, &APlayer::TurnRight );
    }

    void MoveForward( float Value )
    {
        Float3 pos = RootComponent->GetPosition();
        pos += Movable->GetForwardVector() * Value;
        RootComponent->SetPosition( pos );
    }

    void MoveRight( float Value )
    {
        Float3 pos = RootComponent->GetPosition();
        pos += Movable->GetRightVector() * Value;
        RootComponent->SetPosition( pos );
    }

    void MoveUp( float Value )
    {
        Float3 pos = Movable->GetWorldPosition();
        pos.Y += Value;
        Movable->SetWorldPosition( pos );
    }

    void MoveDown( float Value )
    {
        Float3 pos = Movable->GetWorldPosition();
        pos.Y -= Value;
        if ( pos.Y < 0.5f ) {
            pos.Y = 0.5f;
        }
        Movable->SetWorldPosition( pos );
    }

    void TurnRight( float Value )
    {
        const float RotationSpeed = 0.01f;
        Movable->TurnRightFPS( Value * RotationSpeed );
    }

    void DrawDebug( ADebugRenderer * Renderer )
    {
        Float3 pos = Movable->GetWorldPosition();
        Float3 dir = Movable->GetWorldForwardVector();
        Float3 p1 = pos + dir * 0.5f;
        Float3 p2 = pos + dir * 2.0f;
        Renderer->SetColor( AColor4::Blue() );
        Renderer->DrawLine( p1, p2 );
        Renderer->DrawCone( p2, Movable->GetWorldRotation().ToMatrix() * Float3x3::RotationAroundNormal( Math::_PI, Float3(1,0,0) ), 0.4f, Math::_PI/6 );
    }
};

class AGround : public AActor
{
    AN_ACTOR( AGround, AActor )

protected:
    AMeshComponent * MeshComponent;

    AGround()
    {
        static TStaticResourceFinder< AMaterialInstance > BoxMaterialInstance( _CTS( "BoxMaterialInstance" ) );
        static TStaticResourceFinder< AIndexedMesh > DefaultShapePlane256x256x256( _CTS( "DefaultShapePlane256x256x256" ) );

        // Create mesh component
        MeshComponent = CreateComponent< AMeshComponent >( "Ground" );

        // Setup mesh and material
        MeshComponent->SetMesh( DefaultShapePlane256x256x256.GetObject() );
        MeshComponent->SetMaterialInstance( 0, BoxMaterialInstance.GetObject() );
        MeshComponent->SetCastShadow( false );

        // Set root component
        RootComponent = MeshComponent;
    }
};

class AModule final : public IGameModule
{
    AN_CLASS( AModule, IGameModule )

private:
    AModule()
    {
    }

    void OnGameStart() override
    {
        AInputMappings * inputMappings = NewObject< AInputMappings >();
        inputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_W, 1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_S, -1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_A, -1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_D, 1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "MoveUp", ID_KEYBOARD, KEY_SPACE, 1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "MoveDown", ID_KEYBOARD, KEY_C, 1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "TurnRight", ID_MOUSE, MOUSE_AXIS_X, 1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "TurnUp", ID_MOUSE, MOUSE_AXIS_Y, 1.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_LEFT, -90.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAxis( "TurnRight", ID_KEYBOARD, KEY_RIGHT, 90.0f, CONTROLLER_PLAYER_1 );
        inputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
        inputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );

        CreateResources();

        ARenderingParameters * renderingParams = NewObject< ARenderingParameters >();
        renderingParams->bDrawDebug = true;

        AWorld * world = AWorld::CreateWorld();

        // Spawn player
        APlayer * player = world->SpawnActor< APlayer >( Float3( 0, 0.5f, 0 ), Quat::Identity() );

        // Spawn player controller
        APlayerController * playerController = world->SpawnActor< APlayerController >();
        playerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
        playerController->SetInputMappings( inputMappings );
        playerController->SetRenderingParameters( renderingParams );
        playerController->GetInputComponent()->MouseSensitivity = 0.3f;
        playerController->SetPawn( player );

        // Spawn directional light
        ADirectionalLight * dirlight = world->SpawnActor< ADirectionalLight >();
        dirlight->LightComponent->SetCastShadow( true );
        dirlight->LightComponent->SetDirection( Float3( 1, -1, -1 ) );

        // Spawn ground
        STransform spawnTransform;
        spawnTransform.Position = Float3( 0 );
        spawnTransform.Rotation = Quat::Identity();
        spawnTransform.Scale = Float3( 2,1,2 );
        world->SpawnActor< AGround >( spawnTransform );

        WDesktop * desktop = NewObject< WDesktop >();
        GEngine.SetDesktop( desktop );

        desktop->SetCursorVisible( false );

        desktop->AddWidget(
            &WWidget::New< WViewport >()
            .SetPlayerController( playerController )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
            .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
            .SetFocus()
        );
    }

    void OnGameEnd() override
    {
    }

    void CreateResources()
    {
        // Create mesh for ground
        {
            AIndexedMesh * mesh = NewObject< AIndexedMesh >();
            mesh->InitializePlaneMeshXZ( 256, 256, 256 );
            RegisterResource( mesh, "DefaultShapePlane256x256x256" );
        }

        // Create material for box
        {
            MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

            graph->MaterialType = MATERIAL_TYPE_PBR;
            graph->bAllowScreenSpaceReflections = false;

            MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
            diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
            graph->RegisterTextureSlot( diffuseTexture );

            MGInTexCoord * texCoord = graph->AddNode< MGInTexCoord >();
        
            MGSampler * diffuseSampler = graph->AddNode< MGSampler >();
            diffuseSampler->TexCoord->Connect( texCoord, "Value" );
            diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );
        
            MGFloatNode * metallic = graph->AddNode< MGFloatNode >();
            metallic->Value = 0.0f;

            MGFloatNode * roughness = graph->AddNode< MGFloatNode >();
            roughness->Value = 1;

            graph->Color->Connect( diffuseSampler->RGBA );
            graph->Metallic->Connect( metallic->OutValue );
            graph->Roughness->Connect( roughness->OutValue );

            AMaterial * material = CreateMaterial( graph );
            RegisterResource( material, "BoxMaterial" );
        }

        // Create texture for box
        GetOrCreateResource< ATexture >( "TexGrid8", "/Common/grid8.png" );

        // Create material instance for box
        {
            static TStaticResourceFinder< AMaterial > BoxMaterial( _CTS( "BoxMaterial" ) );
            static TStaticResourceFinder< ATexture > GroundTexture( _CTS( "TexGrid8" ) );
            AMaterialInstance * BoxMaterialInstance = NewObject< AMaterialInstance >();
            BoxMaterialInstance->SetMaterial( BoxMaterial.GetObject() );
            BoxMaterialInstance->SetTexture( 0, GroundTexture.GetObject() );
            RegisterResource( BoxMaterialInstance, "BoxMaterialInstance" );
        }
    }
};

#include <Runtime/Public/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "AngieEngine: Simple",
    // Root path
    "Samples/Simple",
    // Module class
    &AModule::ClassMeta()
};

AN_ENTRY_DECL( ModuleDecl )
AN_CLASS_META( APlayer )
AN_CLASS_META( AGround )
AN_CLASS_META( AModule )
