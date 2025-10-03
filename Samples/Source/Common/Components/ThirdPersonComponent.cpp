/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "ThirdPersonComponent.h"
#include "ProjectileComponent.h"
#include "../CollisionLayer.h"

#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>

HK_NAMESPACE_BEGIN

void ThirdPersonComponent::BindInput(InputBindings& input)
{
    input.BindAxis("MoveForward", this, &ThirdPersonComponent::MoveForward);
    input.BindAxis("MoveRight", this, &ThirdPersonComponent::MoveRight);

    input.BindAction("Attack", this, &ThirdPersonComponent::Attack, InputEvent::OnPress);

    input.BindAxis("TurnRight", this, &ThirdPersonComponent::TurnRight);
    input.BindAxis("TurnUp", this, &ThirdPersonComponent::TurnUp);

    input.BindAxis("FreelookHorizontal", this, &ThirdPersonComponent::FreelookHorizontal);
    input.BindAxis("FreelookVertical", this, &ThirdPersonComponent::FreelookVertical);

    input.BindAxis("MoveUp", this, &ThirdPersonComponent::MoveUp);
}

void ThirdPersonComponent::MoveForward(float amount)
{
    m_MoveForward = amount;
}

void ThirdPersonComponent::MoveRight(float amount)
{
    m_MoveRight = amount;
}

void ThirdPersonComponent::TurnRight(float amount)
{
    if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
        viewPoint->Rotate(-amount * GetWorld()->GetTick().FrameTimeStep, Float3::sAxisY());
}

void ThirdPersonComponent::TurnUp(float amount)
{
    if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
        viewPoint->Rotate(amount * GetWorld()->GetTick().FrameTimeStep, viewPoint->GetRightVector());
}

void ThirdPersonComponent::FreelookHorizontal(float amount)
{
    if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
        viewPoint->Rotate(-amount, Float3::sAxisY());
}

void ThirdPersonComponent::FreelookVertical(float amount)
{
    if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
        viewPoint->Rotate(amount, viewPoint->GetRightVector());
}

void ThirdPersonComponent::Attack()
{
    if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
    {
        Float3 p = GetOwner()->GetWorldPosition();
        Float3 dir = viewPoint->GetWorldDirection();
        const float EyeHeight = 1.7f;
        const float Impulse = 100;
        p.Y += EyeHeight;
        p += dir;
        SpawnProjectile(GetWorld(), p, dir * Impulse, PlayerTeam::Blue);
    }
}

void ThirdPersonComponent::MoveUp(float amount)
{
    m_Jump = amount != 0.0f;
}

void ThirdPersonComponent::FixedUpdate()
{
    auto controller = GetOwner()->GetComponent<CharacterControllerComponent>();
    if (!controller)
        return;

    auto viewPoint = GetWorld()->GetObject(ViewPoint);
    if (!viewPoint)
        return;

    Float3 rightVec = viewPoint->GetWorldRightVector();
    rightVec.Y = 0;
    rightVec.NormalizeSelf();

    Float3 forwardVec;
    forwardVec.X = rightVec.Z;
    forwardVec.Z = -rightVec.X;

    Float3 moveDir = forwardVec * m_MoveForward + rightVec * m_MoveRight;

    if (moveDir.LengthSqr() > 1)
        moveDir.NormalizeSelf();

    // Smooth the player input
    m_DesiredVelocity = 0.25f * moveDir * MoveSpeed + 0.75f * m_DesiredVelocity;

    Float3 gravity = GetWorld()->GetInterface<PhysicsInterface>().GetGravity();

    // Determine new basic velocity
    Float3 currentVerticalVelocity = Float3(0, controller->GetLinearVelocity().Y, 0);
    Float3 groundVelocity = controller->GetGroundVelocity();
    Float3 newVelocity;
    if (controller->IsOnGround())
    {
        // Assume velocity of ground when on ground
        newVelocity = groundVelocity;

        if (m_Jump)
            newVelocity.Y = Math::Max(JumpSpeed + newVelocity.Y, JumpSpeed);
    }
    else
    {
        newVelocity = currentVerticalVelocity;

        if (controller->IsOnGround() && newVelocity.Y < 0)
            newVelocity.Y = 0;
    }

    if (!controller->IsOnGround())
    {
        // Gravity
        newVelocity += gravity * GetWorld()->GetTick().FixedTimeStep;
    }

    // Player input
    newVelocity += m_DesiredVelocity;

    // Update character velocity
    controller->SetLinearVelocity(newVelocity);
}

HK_NAMESPACE_END
