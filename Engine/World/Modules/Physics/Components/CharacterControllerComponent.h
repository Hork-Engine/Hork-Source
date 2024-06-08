/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include <Engine/World/Modules/Physics/PhysicsInterface.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

class CharacterControllerComponent : public BodyComponent
{
    friend class PhysicsInterface;

public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t     m_CollisionLayer = CollisionLayer::Character;

    float      HeightStanding = 1.35f;
    float      RadiusStanding = 0.3f;
    float      HeightCrouching = 0.8f;
    float      RadiusCrouching = 0.3f;
    float      MaxSlopeAngle = Math::Radians(45.0f);
    float      MaxStrength = 100.0f;
    float      CharacterPadding = 0.02f;
    float      PenetrationRecoverySpeed = 1.0f;
    float      PredictiveContactDistance = 0.1f;




    Float3 MovementDirection;
    Float3 DesiredVelocity;
    bool Jump = false;

    float MoveSpeed = 2;
    float JumpSpeed = 4.0f;

    bool EnableWalkStairs = true;
    bool EnableStickToFloor = true;

    /// Teleport character to specified position / rotation
    void                    SetWorldPosition(Float3 const& position);
    void                    SetWorldRotation(Quat const& rotation);
    void                    SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation);

    Float3                  GetWorldPosition() const;
    Quat                    GetWorldRotation() const;

    /// Set the linear velocity of the character (m / s)
    void                    SetLinearVelocity(Float3 const& velocity);
    /// Get the linear velocity of the character (m / s)
    Float3                  GetLinearVelocity();

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;
    class CharacterControllerImpl* m_pImpl;

    class BodyUserData*     m_UserData = nullptr;
};

HK_NAMESPACE_END
