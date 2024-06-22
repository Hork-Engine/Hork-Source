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

#include "StateMachine.h"

HK_NAMESPACE_BEGIN

void StateMachine::State::Begin()
{
    m_OnBegin.Invoke();
}

void StateMachine::State::End()
{
    m_OnEnd.Invoke();
}

void StateMachine::State::Update(float timeStep)
{
    m_OnUpdate.Invoke(timeStep);
}

void StateMachine::MakeCurrent(StringView name)
{
    m_PendingState = name;
}

void StateMachine::Update(float timeStep)
{
    UpdateStateChange();

    if (m_CurrentState.IsEmpty())
        return;

    auto it = m_States.Find(m_CurrentState);
    if (it == m_States.End())
        return;

    it->second->Update(timeStep);
}

void StateMachine::UpdateStateChange()
{
    if (m_CurrentState == m_PendingState)
        return;

    if (!m_CurrentState.IsEmpty())
    {
        auto it = m_States.Find(m_CurrentState);
        if (it != m_States.End())
        {
            it->second->End();
            it->second->m_IsActive = false;
        }
    }

    m_CurrentState = m_PendingState;

    if (!m_CurrentState.IsEmpty())
    {
        auto it = m_States.Find(m_CurrentState);
        if (it != m_States.End())
        {
            it->second->Begin();
            it->second->m_IsActive = true;
        }
    }
}

void StateMachine::Unbind(StringView name)
{
    m_States.Erase(name);
    if (m_CurrentState == name)
        m_CurrentState.Clear();
    if (m_PendingState == name)
        m_PendingState.Clear();
}

HK_NAMESPACE_END
