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

#include <Runtime/MeshComponent.h>
#include <Runtime/InputComponent.h>
#include <Runtime/TerrainComponent.h>

class ASpectator : public AActor
{
    AN_ACTOR(ASpectator, AActor)

protected:
    ACameraComponent* Camera{};
    Angl              Angles;
    Float3            MoveVector;
    bool              bSpeed{};
    bool              bTrace{};

    TPodVector<SCollisionTraceResult> TraceResult;
    TPodVector<STriangleHitResult>    HitResult;
    STerrainTriangle                  HitTriangle;

    ASpectator()
    {}

    void Initialize(SActorInitializer& Initializer) override
    {
        Camera        = CreateComponent<ACameraComponent>("Camera");
        RootComponent = Camera;
        PawnCamera    = Camera;

        Initializer.bCanEverTick        = true;
        Initializer.bTickEvenWhenPaused = true;
    }

    void BeginPlay() override
    {
        Super::BeginPlay();

        Float3 vec = RootComponent->GetBackVector();
        Float2 projected(vec.X, vec.Z);
        float  lenSqr = projected.LengthSqr();
        if (lenSqr < 0.0001f)
        {
            vec         = RootComponent->GetRightVector();
            projected.X = vec.X;
            projected.Y = vec.Z;
            projected.NormalizeSelf();
            Angles.Yaw = Math::Degrees(Math::Atan2(projected.X, projected.Y)) + 90;
        }
        else
        {
            projected.NormalizeSelf();
            Angles.Yaw = Math::Degrees(Math::Atan2(projected.X, projected.Y));
        }

        Angles.Pitch = Angles.Roll = 0;

        RootComponent->SetAngles(Angles);
    }

    void SetupInputComponent(AInputComponent* Input) override
    {
        bool bExecuteBindingsWhenPaused = true;

        Input->BindAxis("MoveForward", this, &ASpectator::MoveForward, bExecuteBindingsWhenPaused);
        Input->BindAxis("MoveRight", this, &ASpectator::MoveRight, bExecuteBindingsWhenPaused);
        Input->BindAxis("MoveUp", this, &ASpectator::MoveUp, bExecuteBindingsWhenPaused);
        Input->BindAxis("MoveDown", this, &ASpectator::MoveDown, bExecuteBindingsWhenPaused);
        Input->BindAxis("TurnRight", this, &ASpectator::TurnRight, bExecuteBindingsWhenPaused);
        Input->BindAxis("TurnUp", this, &ASpectator::TurnUp, bExecuteBindingsWhenPaused);
        Input->BindAction("Speed", IA_PRESS, this, &ASpectator::SpeedPress, bExecuteBindingsWhenPaused);
        Input->BindAction("Speed", IA_RELEASE, this, &ASpectator::SpeedRelease, bExecuteBindingsWhenPaused);
        Input->BindAction("Trace", IA_PRESS, this, &ASpectator::TracePress, bExecuteBindingsWhenPaused);
        Input->BindAction("Trace", IA_RELEASE, this, &ASpectator::TraceRelease, bExecuteBindingsWhenPaused);
    }

    void Tick(float TimeStep) override
    {
        Super::Tick(TimeStep);

        constexpr float MOVE_SPEED      = 40; // Meters per second
        constexpr float MOVE_HIGH_SPEED = 80;

        float lenSqr = MoveVector.LengthSqr();
        if (lenSqr > 0)
        {
            MoveVector.NormalizeSelf();

            const float moveSpeed = TimeStep * (bSpeed ? MOVE_HIGH_SPEED : MOVE_SPEED);
            Float3      dir       = MoveVector * moveSpeed;

            RootComponent->Step(dir);

            MoveVector.Clear();
        }

        TraceResult.Clear();
        HitResult.Clear();

        if (bTrace)
        {
            GetWorld()->Trace(TraceResult, Camera->GetWorldPosition(), Camera->GetWorldPosition() + Camera->GetWorldForwardVector() * 1000.0f);
        }
        else
        {
            SWorldRaycastClosestResult result;
            SWorldRaycastFilter        filter;
            filter.VisibilityMask = VISIBILITY_GROUP_TERRAIN;
            if (GetWorld()->RaycastClosest(result, Camera->GetWorldPosition(), Camera->GetWorldForwardVector() * 10000, &filter))
            {
                HitResult.Append(result.TriangleHit);

                ATerrainComponent* terrainComponent = result.Object->GetOwnerActor()->GetComponent<ATerrainComponent>();
                if (terrainComponent)
                    terrainComponent->GetTerrainTriangle(result.TriangleHit.Location, HitTriangle);
            }
        }
    }

    void MoveForward(float Value)
    {
        MoveVector += RootComponent->GetForwardVector() * Math::Sign(Value);
    }

    void MoveRight(float Value)
    {
        MoveVector += RootComponent->GetRightVector() * Math::Sign(Value);
    }

    void MoveUp(float Value)
    {
        if (Value)
            MoveVector.Y += 1;
    }

    void MoveDown(float Value)
    {
        if (Value)
            MoveVector.Y -= 1;
    }

    void TurnRight(float Value)
    {
        Angles.Yaw -= Value;
        Angles.Yaw = Angl::Normalize180(Angles.Yaw);
        RootComponent->SetAngles(Angles);
    }

    void TurnUp(float Value)
    {
        Angles.Pitch += Value;
        Angles.Pitch = Math::Clamp(Angles.Pitch, -90.0f, 90.0f);
        RootComponent->SetAngles(Angles);
    }

    void SpeedPress()
    {
        bSpeed = true;
    }

    void SpeedRelease()
    {
        bSpeed = false;
    }

    void TracePress()
    {
        bTrace = true;
    }

    void TraceRelease()
    {
        bTrace = false;
    }

    void DrawDebug(ADebugRenderer* InRenderer)
    {
        Super::DrawDebug(InRenderer);

        if (bTrace)
        {
            for (auto& tr : TraceResult)
            {
                InRenderer->DrawBoxFilled(tr.Position, Float3(0.1f));
            }
        }
        else
        {
            for (STriangleHitResult& hit : HitResult)
            {
                InRenderer->SetColor(Color4(1, 1, 1, 1));
                InRenderer->DrawBoxFilled(hit.Location, Float3(0.1f));
                InRenderer->DrawLine(hit.Location, hit.Location + hit.Normal);

                InRenderer->SetColor(Color4(0, 1, 0, 1));
                InRenderer->DrawTriangle(HitTriangle.Vertices[0], HitTriangle.Vertices[1], HitTriangle.Vertices[2]);
                InRenderer->DrawLine(hit.Location, hit.Location + HitTriangle.Normal * 0.5f);
            }
        }
    }
};
