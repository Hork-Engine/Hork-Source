/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "Controller.h"
#include <Engine/Core/Platform/Logger.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(Actor_Controller)

Actor_Controller::Actor_Controller()
{}

void Actor_Controller::Initialize(ActorInitializer& Initializer)
{
    Super::Initialize(Initializer);
}

void Actor_Controller::SetPawn(Actor* pawn)
{
    if (m_Pawn == pawn)
    {
        return;
    }

    if (pawn && pawn->GetController())
    {
        LOG("The pawn is already controlled by another controller.\n");
        return;
    }

    if (m_Pawn)
    {
        m_Pawn->m_Controller = nullptr;
        if (!m_Pawn->IsPendingKill())
            m_Pawn->OnInputLost();
    }

    m_Pawn = pawn;

    if (m_Pawn)
    {
        m_Pawn->m_Controller = this;
    }

    OnPawnChanged();
}

HK_NAMESPACE_END
