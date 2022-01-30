/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#pragma once

#include <Runtime/InputComponent.h>
#include <Runtime/MeshComponent.h>

constexpr float CHARACTER_CAPSULE_RADIUS = 0.35f;
constexpr float CHARACTER_CAPSULE_HEIGHT = 1.0f;

class ACharacter : public AActor
{
    AN_ACTOR(ACharacter, AActor)

public:
    void SetFirstPersonCamera(bool FirstPersonCamera)
    {
        bFirstPersonCamera = FirstPersonCamera;

        if (bFirstPersonCamera)
        {
            float eyeOffset = CHARACTER_CAPSULE_HEIGHT * 0.5f;
            Camera->SetPosition(0, eyeOffset, 0);
            Camera->SetAngles(Math::Degrees(FirstPersonCameraPitch), 0, 0);
        }
        else
        {
            Camera->SetPosition(0, 4, std::sqrt(8.0f));
            Camera->SetAngles(-60, 0, 0);
        }
    }

    bool IsFirstPersonCamera() const
    {
        return bFirstPersonCamera;
    }

protected:
    AMeshComponent*           CharacterMesh{};
    APhysicalBody*            CharacterPhysics{};
    ACameraComponent*         Camera{};
    float                     ForwardMove{};
    float                     SideMove{};
    bool                      bWantJump{};
    Float3                    TotalVelocity{};
    AProceduralMeshComponent* ProcMesh;
    TRef<AProceduralMesh>     ProcMeshResource;
    float                     NextJumpTime{};
    bool                      bFirstPersonCamera{};
    float                     FirstPersonCameraPitch{};

    ACharacter()
    {}

    void Initialize(SActorInitializer& Initializer) override
    {
        static TStaticResourceFinder<AIndexedMesh>      CapsuleMesh(_CTS("CharacterCapsule"));
        static TStaticResourceFinder<AMaterialInstance> CharacterMaterialInstance(_CTS("CharacterMaterialInstance"));

        // Create capsule collision model
        SCollisionCapsuleDef capsule;
        capsule.Radius = CHARACTER_CAPSULE_RADIUS;
        capsule.Height = CHARACTER_CAPSULE_HEIGHT;

        ACollisionModel* model = CreateInstanceOf<ACollisionModel>();
        model->Initialize(&capsule);

        // Create simulated physics body
        CharacterPhysics = CreateComponent<APhysicalBody>("CharacterPhysics");
        CharacterPhysics->SetMotionBehavior(MB_SIMULATED);
        CharacterPhysics->SetAngularFactor({0, 0, 0});
        CharacterPhysics->SetCollisionModel(model);
        CharacterPhysics->SetCollisionGroup(CM_PAWN);

        // Create character model and attach it to physics body
        CharacterMesh = CreateComponent<AMeshComponent>("CharacterMesh");
        CharacterMesh->SetMesh(CapsuleMesh.GetObject());
        CharacterMesh->SetMaterialInstance(CharacterMaterialInstance.GetObject());
        CharacterMesh->SetMotionBehavior(MB_KINEMATIC);
        CharacterMesh->AttachTo(CharacterPhysics);

        // Create camera and attach it to character mesh
        Camera = CreateComponent<ACameraComponent>("Camera");
        Camera->SetPosition(0, 4, std::sqrt(8.0f));
        Camera->SetAngles(-60, 0, 0);
        Camera->AttachTo(CharacterMesh);

        static TStaticResourceFinder<AIndexedMesh>      UnitBox(_CTS("/Default/Meshes/Skybox"));
        static TStaticResourceFinder<AMaterialInstance> SkyboxMaterialInst(_CTS("/Root/Skybox/skybox_matinst.minst"));
        AMeshComponent*                                 SkyboxComponent;
        SkyboxComponent = CreateComponent<AMeshComponent>("Skybox");
        SkyboxComponent->SetMotionBehavior(MB_KINEMATIC);
        SkyboxComponent->SetMesh(UnitBox.GetObject());
        SkyboxComponent->SetMaterialInstance(SkyboxMaterialInst.GetObject());
        SkyboxComponent->AttachTo(Camera);
        SkyboxComponent->SetAbsoluteRotation(true);

        // Set root
        RootComponent = CharacterPhysics;
        // Set pawn camera
        PawnCamera = Camera;

        // Call TickPrePhysics() events
        Initializer.bTickPrePhysics = true;
    }

    void TickPrePhysics(float TimeStep) override
    {
        SCollisionTraceResult result;
        SCollisionQueryFilter filter;

        AActor* ignoreList[] = {this};
        filter.IgnoreActors  = ignoreList;
        filter.ActorsCount   = 1;
        filter.CollisionMask = CM_SOLID;

        Float3 traceStart = CharacterPhysics->GetWorldPosition();
        Float3 traceEnd   = traceStart - Float3(0, 0.1f, 0);

        bool bOnGround{};

        NextJumpTime = Math::Max(0.0f, NextJumpTime - TimeStep);

        if (GetWorld()->TraceCapsule(result, CHARACTER_CAPSULE_HEIGHT + 0.1f, CHARACTER_CAPSULE_RADIUS - 0.1f, traceStart, traceEnd, &filter))
        {
            constexpr float JUMP_IMPULSE = 4.5f;

            bOnGround = true;

            if (bWantJump && NextJumpTime <= 0.0f)
            {
                CharacterPhysics->ApplyCentralImpulse(Float3(0, 1, 0) * JUMP_IMPULSE);

                NextJumpTime = 0.05f;
            }
        }

        bWantJump = false;

        constexpr float WALK_IMPULSE     = 0.4f;
        constexpr float FLY_IMPULSE      = 0.2f;
        constexpr float STOP_IMPULSE     = 0.08f;
        constexpr float STOP_IMPULSE_AIR = 0.05f;

        Float3 wishDir = CharacterMesh->GetForwardVector() * ForwardMove + CharacterMesh->GetRightVector() * SideMove;

        Float2 horizontalDir = {wishDir.X, wishDir.Z};
        horizontalDir.NormalizeSelf();

        Float3 overallImpulse{};
        overallImpulse += Float3(horizontalDir.X, 0, horizontalDir.Y) * (bOnGround ? WALK_IMPULSE : FLY_IMPULSE);

        Float3 horizontalVelocity = CharacterPhysics->GetLinearVelocity();
        horizontalVelocity.Y      = 0;

        Float3 stopImpulse = -horizontalVelocity * (bOnGround ? STOP_IMPULSE : STOP_IMPULSE_AIR);

        overallImpulse += stopImpulse;

        CharacterPhysics->ApplyCentralImpulse(overallImpulse);
    }

    void SetupInputComponent(AInputComponent* Input) override
    {
        Input->BindAxis("MoveForward", this, &ACharacter::MoveForward);
        Input->BindAxis("MoveRight", this, &ACharacter::MoveRight);
        Input->BindAxis("MoveUp", this, &ACharacter::MoveUp);
        Input->BindAxis("TurnRight", this, &ACharacter::TurnRight);
        Input->BindAxis("TurnUp", this, &ACharacter::TurnUp);
    }

    void MoveForward(float Value)
    {
        ForwardMove = Value;
    }

    void MoveRight(float Value)
    {
        SideMove = Value;
    }

    void MoveUp(float Value)
    {
        if (Value > 0)
        {
            bWantJump = true;
        }
    }

    void TurnRight(float Value)
    {
        const float RotationSpeed = 0.01f;
        CharacterMesh->TurnRightFPS(Value * RotationSpeed);
    }

    void TurnUp(float Value)
    {
        if (bFirstPersonCamera && Value)
        {
            const float RotationSpeed = 0.01f;
            float delta = Value * RotationSpeed;
            if (FirstPersonCameraPitch + delta > Math::_PI/2)
            {
                delta                  = FirstPersonCameraPitch - Math::_PI / 2;
                FirstPersonCameraPitch = Math::_PI / 2;
            }
            else if (FirstPersonCameraPitch + delta < -Math::_PI / 2)
            {
                delta                  = -Math::_PI / 2 - FirstPersonCameraPitch;
                FirstPersonCameraPitch = -Math::_PI / 2;
            }
            else
                FirstPersonCameraPitch += delta;
            Camera->TurnUpFPS(delta);
        }
    }
};
