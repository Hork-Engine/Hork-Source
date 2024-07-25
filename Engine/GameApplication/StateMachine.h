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

#include <Engine/Core/Containers/Hash.h>
#include <Engine/Core/Ref.h>
#include <Engine/Core/Delegate.h>

HK_NAMESPACE_BEGIN

class StateBase : public Noncopyable
{
    friend class StateMachine;

public:
    virtual             ~StateBase() = default;

    bool                IsActive() const { return m_IsActive; }

    StateMachine&       GetOwner() { return *m_Machine; }

protected:
    virtual void        Begin() {}

    virtual void        End() {}

    virtual void        Update(float timeStep) {}

    StateMachine*       m_Machine;

private:
    bool                m_IsActive{};
};

class StateMachine : public Noncopyable
{
public:
    virtual             ~StateMachine() = default;

    /// Bind state object
    template <typename T, typename... Args>
    void                BindObject(StringView name, Args&&... args);

    /// Bind functions for specified state
    template <typename T>
    void                Bind(StringView name, T* object, void (T::*begin)(), void (T::*end)(), void (T::*update)(float));

    /// Unbind/destroy state
    void                Unbind(StringView name);

    /// Set current state
    void                MakeCurrent(StringView name);

    /// Update state
    void                Update(float timeStep);

private:
    void                UpdateStateChange();

    class State : public StateBase
    {
    public:
        Delegate<void()>        m_OnBegin;
        Delegate<void()>        m_OnEnd;
        Delegate<void(float)>   m_OnUpdate;

                        State(StateMachine* machine);

        void            Begin() override;
        void            End() override;
        void            Update(float timeStep) override;
    };

    StringHashMap<UniqueRef<StateBase>> m_States;

    String              m_CurrentState{};
    String              m_PendingState{};
};

template <typename T, typename... Args>
HK_INLINE void StateMachine::BindObject(StringView name, Args&&... args)
{
    auto& state = m_States[name];
    state = MakeUnique<T>(std::forward<Args>(args)...);
    state->m_Machine = this;
}

template <typename T>
HK_FORCEINLINE void StateMachine::Bind(StringView name, T* object, void (T::*begin)(), void (T::*end)(), void (T::*update)(float))
{
    auto state = MakeUnique<State>(this);
    if (begin)
        state->m_OnBegin.Bind(object, begin);
    if (end)
        state->m_OnEnd.Bind(object, end);
    if (update)
        state->m_OnUpdate.Bind(object, update);
    m_States[name] = std::move(state);
}

HK_NAMESPACE_END
