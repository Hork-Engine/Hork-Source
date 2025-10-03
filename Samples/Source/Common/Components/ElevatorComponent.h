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

#pragma once

#include <Hork/Runtime/World/World.h>

using namespace Hk;

class ElevatorComponent : public Component
{
    enum State
    {
        Idle, MoveUp, Stay, MoveDown
    };
    State m_State = Idle;
    float m_StartY = 0;
    float m_Height = 0;
    float m_StayTime = 0;

    void UpdatePosition()
    {
        Float3 position = GetOwner()->GetWorldPosition();
        position.Y = m_StartY + m_Height;

        GetOwner()->SetWorldPosition(position);
    }

public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    bool IsTriggered = false;

    float MaxHeight = 0;

    void BeginPlay()
    {
        m_StartY = GetOwner()->GetWorldPosition().Y;
        m_Height = 0;
    }

    void FixedUpdate()
    {
        const float MAX_STAY_TIME = 3;
        const float MOVE_SPEED = 3;

        float timeStep = GetWorld()->GetTick().FixedTimeStep;

        if (IsTriggered)
        {
            if (m_State == Idle)
            {
                m_State = MoveUp;
                IsTriggered = false;
            }
        }

        switch (m_State)
        {
        case MoveUp:
            m_Height += MOVE_SPEED * timeStep;
            if (m_Height > MaxHeight)
            {
                m_Height = MaxHeight;
                m_State = Stay;
            }
            UpdatePosition();
            break;
        case MoveDown:
            m_Height -= MOVE_SPEED * timeStep;
            if (m_Height < 0)
            {
                m_Height = 0;
                m_State = Idle;
            }
            UpdatePosition();
            break;
        case Stay:
            m_StayTime += timeStep;
            if (m_StayTime > MAX_STAY_TIME)
            {
                m_StayTime = 0;
                m_State = MoveDown;
            }
            break;
        case Idle:
            break;
        }
    }
};

class ElevatorActivatorComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    Handle32<ElevatorComponent> Elevator;

    void OnBeginOverlap(BodyComponent* body)
    {
        if (auto elevatorComp = GetWorld()->GetComponent(Elevator))
        {
            elevatorComp->IsTriggered = true;
        }        
    }
};
