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

#include "FirstPersonComponent.h"
#include "ProjectileComponent.h"
#include "../CollisionLayer.h"

#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>

HK_NAMESPACE_BEGIN

const float EyeHeight = 1.6f;

void FirstPersonComponent::BindInput(InputBindings& input)
{
    input.BindAxis("MoveForward", this, &FirstPersonComponent::MoveForward);
    input.BindAxis("MoveRight", this, &FirstPersonComponent::MoveRight);

    input.BindAction("Attack", this, &FirstPersonComponent::Attack, InputEvent::OnPress);

    input.BindAxis("TurnRight", this, &FirstPersonComponent::TurnRight);
    input.BindAxis("TurnUp", this, &FirstPersonComponent::TurnUp);

    input.BindAxis("FreelookHorizontal", this, &FirstPersonComponent::FreelookHorizontal);
    input.BindAxis("FreelookVertical", this, &FirstPersonComponent::FreelookVertical);

    input.BindAxis("MoveUp", this, &FirstPersonComponent::MoveUp);
}

void FirstPersonComponent::MoveForward(float amount)
{
    m_MoveForward = amount;
}

void FirstPersonComponent::MoveRight(float amount)
{
    m_MoveRight = amount;
}

void FirstPersonComponent::TurnRight(float amount)
{
    if (auto viewPoint = GetViewPoint())
        viewPoint->Rotate(-amount * GetWorld()->GetTick().FrameTimeStep, Float3::sAxisY());
}

void FirstPersonComponent::TurnUp(float amount)
{
    if (auto viewPoint = GetViewPoint())
        viewPoint->Rotate(amount * GetWorld()->GetTick().FrameTimeStep, viewPoint->GetRightVector());
}

void FirstPersonComponent::FreelookHorizontal(float amount)
{
    if (auto viewPoint = GetViewPoint())
        viewPoint->Rotate(-amount, Float3::sAxisY());
}

void FirstPersonComponent::FreelookVertical(float amount)
{
    if (auto viewPoint = GetViewPoint())
        viewPoint->Rotate(amount, viewPoint->GetRightVector());
}

void FirstPersonComponent::Attack()
{
    if (auto viewPoint = GetViewPoint())
    {
        Float3 p = GetOwner()->GetWorldPosition();
        Float3 dir = viewPoint->GetWorldDirection();
        const float Impulse = 100;
        p.Y += EyeHeight;
        p += dir;
        SpawnProjectile(GetWorld(), p, dir * Impulse, Team);
    }
}

void FirstPersonComponent::MoveUp(float amount)
{
    m_Jump = amount != 0.0f;
}

GameObject* FirstPersonComponent::GetViewPoint()
{
    return GetWorld()->GetObject(ViewPoint);
}

void FirstPersonComponent::FixedUpdate()
{
    auto controller = GetOwner()->GetComponent<CharacterControllerComponent>();
    if (!controller)
        return;

    auto viewPoint = GetViewPoint();
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

void FirstPersonComponent::ApplyDamage(Float3 const& damageVector)
{
    m_DesiredVelocity += damageVector * 2;
}

void FirstPersonComponent::PhysicsUpdate()
{
    auto controller = GetOwner()->GetComponent<CharacterControllerComponent>();
    if (!controller)
        return;

    auto viewPoint = GetViewPoint();

    if (controller->IsOnGround())
    {
        const float StepHeight = 0.5f;

        float curY = controller->GetWorldPosition().Y;
        float d = Math::Abs(m_ViewY - curY);
        if (d > 0.001f && d <= StepHeight)
            m_ViewY = Math::Lerp(m_ViewY, curY, 0.4f);
        else
            m_ViewY = curY;

        float delta = m_ViewY - curY;
        viewPoint->SetPosition(Float3(0,EyeHeight + delta,0));
    }
    else
        viewPoint->SetPosition(Float3(0,EyeHeight,0));
}

HK_NAMESPACE_END
