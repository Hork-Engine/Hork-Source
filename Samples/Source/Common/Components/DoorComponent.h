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

class DoorComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    Hk::Float3 Direction;

    float m_MaxOpenDist{};
    float m_OpenSpeed{1};
    float m_CloseSpeed{1};

    enum DOOR_STATE
    {
        STATE_CLOSED,
        STATE_OPENED,
        STATE_OPENING,
        STATE_CLOSING
    };
    DOOR_STATE m_DoorState{STATE_CLOSED};
    float m_NextThinkTime{};
    float m_OpenDist{};
    bool m_bIsActive = false;
    Hk::Float3 m_StartPosition;

    void BeginPlay()
    {
        m_StartPosition = GetOwner()->GetPosition();
    }

    void FixedUpdate()
    {
        float timeStep = GetWorld()->GetTick().FixedTimeStep;

        if (m_bIsActive)
        {
            if (m_DoorState == DoorComponent::STATE_CLOSED)
                m_DoorState = DoorComponent::STATE_OPENING;
            else if (m_DoorState == DoorComponent::STATE_OPENED)
                m_NextThinkTime = 2;
        }

        switch (m_DoorState)
        {
            case DoorComponent::STATE_CLOSED:
                break;
            case DoorComponent::STATE_OPENED: {
                m_NextThinkTime -= timeStep;
                if (m_NextThinkTime <= 0)
                {
                    m_DoorState = DoorComponent::STATE_CLOSING;
                }
                break;
            }
            case DoorComponent::STATE_OPENING: {
                m_OpenDist += timeStep * m_OpenSpeed;
                if (m_OpenDist >= m_MaxOpenDist)
                {
                    m_OpenDist = m_MaxOpenDist;
                    m_DoorState = DoorComponent::STATE_OPENED;
                    m_NextThinkTime = 2;
                }
                GetOwner()->SetPosition(m_StartPosition + Direction * m_OpenDist);
                break;
            }
            case DoorComponent::STATE_CLOSING: {
                m_OpenDist -= timeStep * m_CloseSpeed;
                if (m_OpenDist <= 0)
                {
                    m_OpenDist = 0;
                    m_DoorState = DoorComponent::STATE_CLOSED;
                }
                GetOwner()->SetPosition(m_StartPosition + Direction * m_OpenDist);
                break;
            }
        }
    }
};
