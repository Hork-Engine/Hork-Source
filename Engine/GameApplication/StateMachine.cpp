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
