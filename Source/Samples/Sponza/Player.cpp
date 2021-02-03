/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "Player.h"

#include <Runtime/Public/Runtime.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Resource/SoundResource.h>

AN_BEGIN_CLASS_META( APlayer )
AN_END_CLASS_META()

APlayer::APlayer() {
    Camera = CreateComponent< ACameraComponent >( "Camera" );
    RootComponent = Camera;

    Camera->SetFovY(70);

    PawnCamera = Camera;

    bCanEverTick = true;

    // Create skybox
//    static TStaticResourceFinder< AIndexedMesh > UnitBox( _CTS( "/Default/Meshes/Skybox" ) );
//    //static TStaticResourceFinder< AMaterialInstance > SkyboxMaterialInst( _CTS( "/Root/Skybox3/Skybox_MaterialInstance.asset" ) );
//    //static TStaticResourceFinder< AMaterialInstance > SkyboxMaterialInst( _CTS( "/Root/Skybox2/Skybox_MaterialInstance.asset" ) );
//    static TStaticResourceFinder< AMaterialInstance > SkyboxMaterialInst( _CTS( "/Root/Skybox6/Skybox_MaterialInstance.asset" ) );
//    SkyboxComponent = CreateComponent< AMeshComponent >( "Skybox" );
//    SkyboxComponent->SetMesh( UnitBox.GetObject() );
//    SkyboxComponent->SetMaterialInstance( SkyboxMaterialInst.GetObject() );
//    SkyboxComponent->AttachTo( Camera );
//    SkyboxComponent->SetAbsoluteRotation( true );
//    SkyboxComponent->RenderingOrder = RENDER_ORDER_SKYBOX;

    Weapon = CreateComponent< AMeshComponent >( "Weapon" );
    Weapon->SetMesh( _CTS( "/Root/doom_plasma_rifle/scene_Mesh.asset" ) );
    Weapon->CopyMaterialsFromMeshResource();
    Weapon->AttachTo( Camera );
    Weapon->SetPosition( 0.15f,-0.5f,-0.4f );
    Weapon->SetCollisionGroup( CM_NOCOLLISION );
    Weapon->SetMotionBehavior( MB_KINEMATIC );
}

void APlayer::BeginPlay() {
    Super::BeginPlay();

    Float3 vec = RootComponent->GetBackVector();
    Float2 projected( vec.X, vec.Z );
    float lenSqr = projected.LengthSqr();
    if ( lenSqr < 0.0001 ) {
        vec = RootComponent->GetRightVector();
        projected.X = vec.X;
        projected.Y = vec.Z;
//        float lenSqr = projected.LengthSqr();
//        if ( lenSqr < 0.0001 ) {

//        } else {
            projected.NormalizeSelf();
            Angles.Yaw = Math::Degrees( atan2( projected.X, projected.Y ) ) + 90;
//        }
    } else {
        projected.NormalizeSelf();
        Angles.Yaw = Math::Degrees( atan2( projected.X, projected.Y ) );
    }

    Angles.Pitch = Angles.Roll = 0;

    RootComponent->SetAngles( Angles );
}

void APlayer::EndPlay() {
    Super::EndPlay();
}

void APlayer::SetupPlayerInputComponent( AInputComponent * _Input ) {
    _Input->BindAxis( "MoveForward", this, &APlayer::MoveForward );
    _Input->BindAxis( "MoveRight", this, &APlayer::MoveRight );
    _Input->BindAxis( "MoveUp", this, &APlayer::MoveUp );
    _Input->BindAxis( "MoveDown", this, &APlayer::MoveDown );
    _Input->BindAxis( "TurnRight", this, &APlayer::TurnRight );
    _Input->BindAxis( "TurnUp", this, &APlayer::TurnUp );
    _Input->BindAction( "Speed", IA_PRESS, this, &APlayer::SpeedPress );
    _Input->BindAction( "Speed", IA_RELEASE, this, &APlayer::SpeedRelease );
    _Input->BindAction( "Attack", IA_PRESS, this, &APlayer::AttackPress );
}

void APlayer::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    constexpr float PLAYER_MOVE_SPEED = 4; // Meters per second
    constexpr float PLAYER_MOVE_HIGH_SPEED = 8;

    const float MoveSpeed = _TimeStep * ( bSpeed ? PLAYER_MOVE_HIGH_SPEED : PLAYER_MOVE_SPEED );
    float lenSqr = MoveVector.LengthSqr();
    if ( lenSqr > 0 ) {

        //if ( lenSqr > 1 ) {
            MoveVector.NormalizeSelf();
        //}

        Float3 dir = MoveVector * MoveSpeed;

        RootComponent->Step( dir );

        MoveVector.Clear();
    }

    //unitBoxComponent->SetPosition(RootComponent->GetPosition());
}

void APlayer::MoveForward( float _Value ) {
    MoveVector += RootComponent->GetForwardVector() * Math::Sign(_Value);
}

void APlayer::MoveRight( float _Value ) {
    MoveVector += RootComponent->GetRightVector() * Math::Sign(_Value);
}

void APlayer::MoveUp( float _Value ) {
    MoveVector.Y += 1;//_Value;
}

void APlayer::MoveDown( float _Value ) {
    MoveVector.Y -= 1;//_Value;
}

void APlayer::TurnRight( float _Value ) {
    Angles.Yaw -= _Value;
    Angles.Yaw = Angl::Normalize180( Angles.Yaw );
    RootComponent->SetAngles( Angles );
}

void APlayer::TurnUp( float _Value ) {
    Angles.Pitch += _Value;
    Angles.Pitch = Math::Clamp( Angles.Pitch, -90.0f, 90.0f );
    RootComponent->SetAngles( Angles );
}

void APlayer::SpeedPress() {
    bSpeed = true;
}

void APlayer::SpeedRelease() {
    bSpeed = false;
}




#include"SponzaModel.h"
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/World.h>

class ASphereActor : public AActor {
    AN_ACTOR( ASphereActor, AActor )

protected:
    ASphereActor();

    void BeginPlay();

private:
    void OnContact( SContactEvent const & Contact );

    AMeshComponent * MeshComponent;
};

#include <World/Public/MaterialGraph/MaterialGraph.h>
AN_CLASS_META( ASphereActor )

static AMaterial * GetOrCreateSphereMaterial() {
    static AMaterial * material = nullptr;
    if ( !material )
    {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();
        MGInTexCoord * texCoord = graph->AddNode< MGInTexCoord >();
        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
        MGSampler * diffuseSampler = graph->AddNode< MGSampler >();
        diffuseSampler->TexCoord->Connect( texCoord, "Value" );
        diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );
        MGUniformAddress * uniformAddress = graph->AddNode< MGUniformAddress >();
        uniformAddress->Address = 0;
        uniformAddress->Type = AT_Float4;
        MGMulNode * mul = graph->AddNode< MGMulNode >();
        mul->ValueA->Connect( diffuseSampler, "RGBA" );
        mul->ValueB->Connect( uniformAddress, "Value" );
        graph->Color->Connect( mul, "Result" );

        //MGFloat3Node * normal = graph->AddNode< MGFloat3Node >();
        //normal->Value = Float3(0,0,1);
        MGFloatNode * metallic = graph->AddNode< MGFloatNode >();
        metallic->Value = 0.0f;
        MGFloatNode * roughness = graph->AddNode< MGFloatNode >();
        roughness->Value = 0.1f;

        //graph->Normal->Connect( normal, "Value" );
        graph->Metallic->Connect( metallic, "Value" );
        graph->Roughness->Connect( roughness, "Value" );

        //graph->Ambient->Connect( ambientSampler, "R" );
        //graph->Emissive->Connect( emissiveSampler, "RGBA" );

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );

        material = CreateMaterial( graph );
        RegisterResource( material, "SphereMaterial" );
    }
    return material;
}

ASphereActor::ASphereActor() {
    static TStaticResourceFinder< AIndexedMesh > MeshResource( _CTS( "/Default/Meshes/Sphere" ) );
    static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/mipmapchecker.png" ) );

    AMersenneTwisterRand & rng = GRuntime.Rand;

    // Create material instance for mesh component
    AMaterialInstance * matInst = NewObject< AMaterialInstance >();
    matInst->SetMaterial( GetOrCreateSphereMaterial() );
    matInst->SetTexture( 0, TextureResource.GetObject() );
    matInst->UniformVectors[0] = Float4( rng.GetFloat(), rng.GetFloat(), rng.GetFloat(), 1.0f );

    // Create mesh component and set it as root component
    MeshComponent = CreateComponent< AMeshComponent >( "StaticMesh" );
    RootComponent = MeshComponent;
    MeshComponent->SetMotionBehavior( MB_SIMULATED );
    MeshComponent->SetDispatchContactEvents( true );
    MeshComponent->SetGenerateContactPoints( true );
    //MeshComponent->Mass = 0.3f;
    //MeshComponent->SetFriction( 1.0f );
    //MeshComponent->SetRestitution( 10 );
    //MeshComponent->SetCcdRadius( 0.2f );
    //MeshComponent->SetCcdMotionThreshold( 1e-7f );
    //MeshComponent->SetAngularFactor( Float3( 0.0f ) );

    // Set mesh and material resources for mesh component
    MeshComponent->SetMesh( MeshResource.GetObject() );
    MeshComponent->SetMaterialInstance( 0, matInst );

    //LifeSpan = 3;
}

void ASphereActor::BeginPlay() {
    E_OnBeginContact.Add( this, &ASphereActor::OnContact );

    MeshComponent->AddCollisionIgnoreActor( GetInstigator() );
}

void ASphereActor::OnContact( SContactEvent const & Contact ) {
    //static TStaticResourceFinder< ASoundResource > Sound( _CTS( "/Root/Audio/qubodup.wav" ) );
    static TStaticResourceFinder< ASoundResource > Sound( _CTS( "/Root/Audio/bounce.wav" ) );

    Float3 pos = Contact.Points[0].Position/*RootComponent->GetPosition()*/;
    ASoundEmitter::PlaySoundAt( GetWorld(), Sound.GetObject(), nullptr, pos );
}

void APlayer::AttackPress() {
    AActor * actor;

    STransform transform;

    transform.Position = Camera->GetWorldPosition();// + Camera->GetWorldForwardVector() * 1.5f;
    transform.Rotation = Angl( 45.0f, 45.0f, 45.0f ).ToQuat();
    transform.SetScale( 1 );

    actor = GetWorld()->SpawnActor< ASphereActor >( transform );

    AMeshComponent * mesh = actor->GetComponent< AMeshComponent >();
    if ( mesh ) {
        mesh->ApplyCentralImpulse( Camera->GetWorldForwardVector() * 20.0f );
    }
}
