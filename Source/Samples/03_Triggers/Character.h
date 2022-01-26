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

constexpr float CHARACTER_CAPSULE_RADIUS = 0.5f;
constexpr float CHARACTER_CAPSULE_HEIGHT = 1.0f;

class ACharacter : public AActor
{
    AN_ACTOR(ACharacter, AActor)

protected:
    AMeshComponent*   CharacterMesh{};
    APhysicalBody*    CharacterPhysics{};
    ACameraComponent* Camera{};
    Float3            WishDir{};
    bool              bWantJump{};
    Float3            TotalVelocity{};

    ACharacter()
    {}

    void Initialize(SActorInitializer& Initializer) override
    {
        static TStaticResourceFinder<AIndexedMesh>      CapsuleMesh(_CTS("CharacterCapsule"));
        static TStaticResourceFinder<AMaterialInstance> CharacterMaterialInstance(_CTS("CharacterMaterialInstance"));

        // Create capsule collision model
        ACollisionModel*   model   = CreateInstanceOf<ACollisionModel>();
        ACollisionCapsule* capsule = model->CreateBody<ACollisionCapsule>();
        capsule->Radius            = CHARACTER_CAPSULE_RADIUS;
        capsule->Height            = CHARACTER_CAPSULE_HEIGHT;

        // Create simulated physics body
        CharacterPhysics = CreateComponent<APhysicalBody>("CharacterPhysics");
        CharacterPhysics->SetMotionBehavior(MB_SIMULATED);
        CharacterPhysics->SetAngularFactor({0, 0, 0});
        CharacterPhysics->SetCollisionModel(model);
        CharacterPhysics->SetOverrideWorldGravity(true);
        CharacterPhysics->SetSelfGravity({0, 0, 0});
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

        if (GetWorld()->TraceCapsule(result, CHARACTER_CAPSULE_HEIGHT, CHARACTER_CAPSULE_RADIUS - 0.1f, traceStart, traceEnd, &filter))
        {
            constexpr float JUMP_VELOCITY = 8;

            bOnGround = true;

            TotalVelocity.Y = 0;
            if (bWantJump)
                TotalVelocity.Y = JUMP_VELOCITY;
        }

        bWantJump = false;

        constexpr float WALK_VELOCITY = 4;
        constexpr float FLY_VELOCITY  = 2;

        float velocityScale = bOnGround ? WALK_VELOCITY : FLY_VELOCITY;

        Float2 horizontalDir = {WishDir.X, WishDir.Z};
        horizontalDir.NormalizeSelf();

        TotalVelocity.X = horizontalDir.X * velocityScale;
        TotalVelocity.Z = horizontalDir.Y * velocityScale;

        if (!bOnGround)
        {
            constexpr float GRAVITY = 20.0f;
            TotalVelocity.Y -= TimeStep * GRAVITY;
        }

        CharacterPhysics->SetLinearVelocity(TotalVelocity);

        WishDir.Clear();
    }

    void SetupInputComponent(AInputComponent* Input) override
    {
        Input->BindAxis("MoveForward", this, &ACharacter::MoveForward);
        Input->BindAxis("MoveRight", this, &ACharacter::MoveRight);
        Input->BindAxis("MoveUp", this, &ACharacter::MoveUp);
        Input->BindAxis("TurnRight", this, &ACharacter::TurnRight);
    }

    void MoveForward(float Value)
    {
        WishDir += CharacterMesh->GetForwardVector() * Value;
    }

    void MoveRight(float Value)
    {
        WishDir += CharacterMesh->GetRightVector() * Value;
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

    void DrawDebug(ADebugRenderer* Renderer) override
    {
        Super::DrawDebug(Renderer);

        Float3 pos = CharacterMesh->GetWorldPosition();
        Float3 dir = CharacterMesh->GetWorldForwardVector();
        Float3 p1  = pos + dir * CHARACTER_CAPSULE_RADIUS;
        Float3 p2  = pos + dir * (CHARACTER_CAPSULE_RADIUS + 1.5f);
        Renderer->SetColor(Color4::Blue());
        Renderer->DrawLine(p1, p2);
        Renderer->DrawCone(p2, CharacterMesh->GetWorldRotation().ToMatrix3x3() * Float3x3::RotationAroundNormal(Math::_PI, Float3(1, 0, 0)), 0.4f, Math::_PI / 6);
    }
};
