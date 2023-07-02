#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Core/Ref.h>

HK_NAMESPACE_BEGIN

struct EventBase
{
    EventBase(uint32_t type) :
        Type(type)
    {}

    virtual ~EventBase() = default;

    const uint32_t Type;
};

class GameEvents
{
public:
    ~GameEvents();

    void Clear();

    template <typename T, typename... Args>
    T& AddEvent(Args&&... args)
    {
        SpinLockGuard lock(m_SpinLock[m_WriteFrameIndex]);
        T* event = m_Allocator[m_WriteFrameIndex].New<T>(std::forward<Args>(args)...);
        m_Events[m_WriteFrameIndex].Add(event);
        return *event;
    }

    void SwapReadWrite();

    TVector<EventBase*> const& GetEventsUnlocked() const;

private:
    TLinearAllocator<> m_Allocator[2];
    TVector<EventBase*> m_Events[2];
    SpinLock m_SpinLock[2];
    int m_ReadFrameIndex{0};
    int m_WriteFrameIndex{1};
};

class IEventHandler : public RefCounted
{
public:
    virtual ~IEventHandler() = default;
    virtual void ProcessEvents(TVector<EventBase*> const& events) = 0;
};

HK_NAMESPACE_END
