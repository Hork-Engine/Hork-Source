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
#if 0
void LoadScreenState::Begin()
{
    // Create UI with fullscreen image

    LOG("LoadScreenState\n");
}

void LoadScreenState::End()
{
    // Destroy UI
}

void LoadScreenState::Tick(float timeStep)
{
    // Update progress bar

    if (m_LoadingArea && GameApplication::GetResourceManager().IsAreaReady(m_LoadingArea))
    {
        m_Machine->SetCurrent(NextState);
    }
}

void IngameState::Begin()
{
    LOG("IngameState\n");
}

void IngameState::End()
{
}

void IngameState::Tick(float timeStep)
{
}
#endif
void StateMachine::DestroyState(StateBase* state)
{
    HK_ASSERT(state);
    HK_ASSERT(!state->IsActive());

    for (auto it = m_States.Begin(); it != m_States.End(); it++)
    {
        if (it->get() == state)
        {
            m_States.Erase(it);
            break;
        }
    }

    if (m_CurrentState == state)
        m_CurrentState = nullptr;
    if (m_PendingState == state)
        m_PendingState = nullptr;
}

void StateMachine::SetCurrent(StateBase* state)
{
    HK_ASSERT(HasState(state));

    m_PendingState = state;
}

bool StateMachine::HasState(StateBase* state) const
{
    for (auto& it : m_States)
        if (it.get() == state)
            return true;
    return false;
}

void StateMachine::Tick(float timeStep)
{
    UpdateStateChange();

    if (!m_CurrentState)
        return;

    m_CurrentState->Tick(timeStep);
}

void StateMachine::UpdateStateChange()
{
    if (m_CurrentState == m_PendingState)
        return;

    if (m_CurrentState)
    {
        m_CurrentState->End();
        m_CurrentState->m_bIsActive = false;
    }

    m_CurrentState = m_PendingState;
    if (m_CurrentState)
    {
        m_CurrentState->m_bIsActive = true;
        m_CurrentState->Begin();
    }
}

HK_NAMESPACE_END
