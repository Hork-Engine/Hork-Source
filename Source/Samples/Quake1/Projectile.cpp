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

#include "Projectile.h"
#include "Explosion.h"
#include "Game.h"
#include "QuakeModelFrame.h"

#include <Engine/Resource/Public/MaterialAssembly.h>
#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/Resource/Public/ResourceManager.h>

#include <Engine/Audio/Public/AudioSystem.h>
#include <Engine/Audio/Public/AudioClip.h>

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/MeshComponent.h>


AN_CLASS_META( FProjectileActor )

FProjectileActor::FProjectileActor() {
    // Create material instance for mesh component
    FMaterialInstance * matInst = NewObject< FMaterialInstance >();
    matInst->Material = GetResource< FMaterial >( "SkinMaterial" );

    FQuakeModel * model = GGameModule->LoadQuakeResource< FQuakeModel >( "progs/missile.mdl");

    if ( model && !model->Skins.IsEmpty() ) {
        // Set random skin (just for fun)
        matInst->SetTexture( 0, model->Skins[rand()%model->Skins.Size()].Texture );
    }

    // Create mesh component and set it as root component
    MeshComponent = AddComponent< FQuakeModelFrame >( "Missile" );
    RootComponent = MeshComponent;
    MeshComponent->PhysicsBehavior = PB_DYNAMIC;
    MeshComponent->bUseDefaultBodyComposition = false;
    MeshComponent->bDispatchContactEvents = true;
    MeshComponent->bDisableGravity = true;
    MeshComponent->Mass = 1.0f;
    FCollisionCapsule * capsule = MeshComponent->BodyComposition.AddCollisionBody< FCollisionCapsule >();
    capsule->Radius = 0.1f;
    capsule->Height = 0.35f;
    capsule->Axial = FCollisionCapsule::AXIAL_Z;
    MeshComponent->SetCcdRadius( 10 );
    MeshComponent->CollisionGroup = CM_PROJECTILE;
    MeshComponent->CollisionMask = CM_WORLD | CM_PAWN | CM_PROJECTILE;
    //MeshComponent->SetAngularFactor( Float3( 0 ) );

    // Set mesh and material resources for mesh component
    MeshComponent->SetModel( model );
    MeshComponent->SetMaterialInstance( 0, matInst );
}

void FProjectileActor::BeginPlay() {
    E_OnBeginContact.Add( this, &FProjectileActor::OnDamage );

    SpawnPosition = RootComponent->GetPosition();

    MeshComponent->AddCollisionIgnoreActor( GetInstigator() );

    FSoundSpawnParameters spawnParameters;
    spawnParameters.Location = AUDIO_FOLLOW_INSIGATOR;
    spawnParameters.bStopWhenInstigatorDead = true;
    spawnParameters.Volume = 0.5f;

    //desc.AttenuationDistance = std::numeric_limits<float>::infinity();
    //desc.ReferenceDistance = 5;
    //desc.RolloffRate = 0.5f;

    FAudioClip * Clip = GGameModule->LoadQuakeResource< FQuakeAudio >( "sound/weapons/sgun1.wav" );

    GAudioSystem.PlaySound( Clip, this, &spawnParameters );
}

void FProjectileActor::SpawnExplosion( Float3 const & _Position ) {
    GetWorld()->SpawnActor< FExplosionActor >( _Position, Quat::Identity() );
}

void FProjectileActor::OnDamage( FContactEvent const & _Event ) {

    if ( IsPendingKill() ) {
        return;
    }
    
    SpawnExplosion( RootComponent->GetPosition() );

//    FActor * damageReceiver = _Event.OtherActor;
//
//#if 1
//    damageReceiver->ApplyDamage( 100, RootComponent->GetPosition(), this );
//#else
//    damageReceiver->Destroy();
//#endif

    GetWorld()->ApplyRadialDamage( 100, RootComponent->GetPosition(), 1.0f );

    Destroy();
}

void FProjectileActor::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    _DebugDraw->SetColor( FColor4(1,0,1,1) );
    _DebugDraw->DrawLine( SpawnPosition, RootComponent->GetWorldPosition() );
}
