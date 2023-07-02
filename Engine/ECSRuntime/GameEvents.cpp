#include "GameEvents.h"

HK_NAMESPACE_BEGIN

GameEvents::~GameEvents()
{
    SwapReadWrite();
    SwapReadWrite();
}

void GameEvents::Clear()
{
    SpinLockGuard lock(m_SpinLock[m_WriteFrameIndex]);

    for (EventBase* event : m_Events[m_WriteFrameIndex])
        event->~EventBase();

    m_Allocator[m_WriteFrameIndex].Reset();
    m_Events[m_WriteFrameIndex].Clear();
}

void GameEvents::SwapReadWrite()
{
    m_ReadFrameIndex = m_WriteFrameIndex;
    m_WriteFrameIndex = (m_WriteFrameIndex + 1) & 1;

    Clear();
}

TVector<EventBase*> const& GameEvents::GetEventsUnlocked() const
{
    return m_Events[m_ReadFrameIndex];
}

HK_NAMESPACE_END
