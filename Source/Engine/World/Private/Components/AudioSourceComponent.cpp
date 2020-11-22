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

#include <World/Public/Components/AudioSourceComponent.h>
#include <World/Public/Components/MeshComponent.h>

AN_BEGIN_CLASS_META( AAudioSourceComponent )
AN_END_CLASS_META()

AAudioSourceComponent::AAudioSourceComponent()
{
    AudioControl = NewObject< AAudioControlCallback >();
}

void AAudioSourceComponent::OnCreateAvatar()
{
    Super::OnCreateAvatar();

    // TODO: Create mesh or sprite for avatar
    static TStaticResourceFinder< AIndexedMesh > Mesh( _CTS( "/Default/Meshes/Sphere" ) );
    static TStaticResourceFinder< AMaterialInstance > MaterialInstance( _CTS( "AvatarMaterialInstance" ) );
    AMeshComponent * meshComponent = GetParentActor()->CreateComponent< AMeshComponent >( "AudioSourceAvatar" );
    meshComponent->SetMotionBehavior( MB_KINEMATIC );
    meshComponent->SetCollisionGroup( CM_NOCOLLISION );
    meshComponent->SetMesh( Mesh.GetObject() );
    meshComponent->SetMaterialInstance( MaterialInstance.GetObject() );
    meshComponent->SetCastShadow( false );
    meshComponent->SetAbsoluteScale( true );
    meshComponent->SetAbsoluteRotation( true );
    meshComponent->SetScale( 0.1f );
    meshComponent->AttachTo( this );
    meshComponent->SetHideInEditor( true );
}

void AAudioSourceComponent::BeginPlay()
{
    SSoundSpawnParameters spawnParameters;

    spawnParameters.SourceType = SourceType;
    spawnParameters.Priority = Priority;
    spawnParameters.bPlayEvenWhenPaused = bPlayEvenWhenPaused;
    spawnParameters.bVirtualizeWhenSilent = bVirtualizeWhenSilent;
    spawnParameters.bUseVelocity = bUseVelocity;
    spawnParameters.bUsePhysicalVelocity = false;
    spawnParameters.AudioClient = AudioClient;
    spawnParameters.Group = AudioGroup;
    spawnParameters.Attenuation.ReferenceDistance = ReferenceDistance;
    spawnParameters.Attenuation.MaxDistance = MaxDistance;
    spawnParameters.Attenuation.RolloffRate = RolloffRate;
    spawnParameters.Volume = 1.0f;
    spawnParameters.Pitch = Pitch;
    spawnParameters.PlayOffset = PlayOffset;
    spawnParameters.bLooping = bLooping;
    spawnParameters.bStopWhenInstigatorDead = true;
    spawnParameters.bDirectional = bDirectional;
    spawnParameters.ConeInnerAngle = ConeInnerAngle;
    spawnParameters.ConeOuterAngle = ConeOuterAngle;
    spawnParameters.Direction = Direction;
    spawnParameters.LifeSpan = AudioLifeSpan;
    spawnParameters.ControlCallback = AudioControl;

    GAudioSystem.PlaySound( AudioClip, this, &spawnParameters );
}
