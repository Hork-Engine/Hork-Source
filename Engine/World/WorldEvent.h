#pragma once

#include "GameObject.h"

HK_NAMESPACE_BEGIN

class EventHolderBase
{
public:
    virtual ~EventHolderBase() {}

    virtual void Clear() = 0;
};

template <typename _EventType, typename... _DelegateArgs>
class EventHolderTempl final : public EventHolderBase
{
public:
    using EventType = _EventType;
    using DelegateType = Delegate<void(ComponentHandle, _DelegateArgs...)>;

    struct DelegateWithComponent
    {
        ComponentHandle Receiver;
        DelegateType    Delegate;
    };

    virtual void Clear() override
    {
        m_Delegates.Clear();
    }

    void Add(GameObject* object, Component* receiver, DelegateType delegate)
    {
        DelegateWithComponent delegateWithComponent;
        delegateWithComponent.Receiver = receiver->GetHandle();
        delegateWithComponent.Delegate = delegate;

        m_Delegates[object->GetHandle()].Add(delegateWithComponent);
    }

    void Remove(GameObject* object, Component* receiver)
    {
        auto object_it = m_Delegates.Find(object->GetHandle());
        if (object_it == m_Delegates.end())
            return;

        auto& objectDelegates = object_it->second;

        for (auto it = objectDelegates.begin() ; it != objectDelegates.end() ;)
        {
            if (it->Receiver == receiver->GetHandle())
                it = objectDelegates.Erase(it);
            else
                ++it;
        }

        if (objectDelegates.IsEmpty())
            m_Delegates.Erase(object_it);
    }

    void Dispatch(GameObject* object, _DelegateArgs... args)
    {
        auto object_it = m_Delegates.Find(object->GetHandle());
        if (object_it == m_Delegates.end())
            return;

        auto& objectDelegates = object_it->second;

        for (DelegateWithComponent& delegateWithComponent : objectDelegates)
        {
            delegateWithComponent.Delegate.Invoke(delegateWithComponent.Receiver, std::forward<_DelegateArgs>(args)...);
        }
    }

private:
    THashMap<GameObjectHandle, TVector<DelegateWithComponent>> m_Delegates;
};

struct Event_OnBeginOverlap
{
    using Holder = typename EventHolderTempl<Event_OnBeginOverlap, class BodyComponent*>;
};

struct Event_OnEndOverlap
{
    using Holder = typename EventHolderTempl<Event_OnEndOverlap, class BodyComponent*>;
};

namespace WorldEvent
{

using EventTypeID = uint32_t;

HK_INLINE EventTypeID __StaticTimeTypeIDGenerator = 0;
template <typename T>
HK_INLINE const EventTypeID TypeID = __StaticTimeTypeIDGenerator++;

HK_FORCEINLINE uint32_t GetTypesCount()
{
    return __StaticTimeTypeIDGenerator;
}

}

HK_NAMESPACE_END
