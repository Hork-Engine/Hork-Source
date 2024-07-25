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
    HashMap<GameObjectHandle, Vector<DelegateWithComponent>> m_Delegates;
};

struct Event_OnBeginOverlap
{
    using Holder = EventHolderTempl<Event_OnBeginOverlap, class BodyComponent*>;
};

struct Event_OnEndOverlap
{
    using Holder = EventHolderTempl<Event_OnEndOverlap, class BodyComponent*>;
};

using WorldEventTypeID = uint32_t;

namespace WorldEventRTTR
{

    /// Static time ID generation. Do not use.
    HK_INLINE WorldEventTypeID __StaticTimeTypeIDGenerator = 0;

    // NOTE: ID is used for runtime. For static time use __StaticTimeTypeIDGenerator
    template <typename T>
    HK_INLINE const WorldEventTypeID TypeID = __StaticTimeTypeIDGenerator++;

    /// Total types count
    HK_FORCEINLINE size_t GetTypesCount()
    {
        return __StaticTimeTypeIDGenerator;
    }

} // namespace WorldEventRTTR

HK_NAMESPACE_END
