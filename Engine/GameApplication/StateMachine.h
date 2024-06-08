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

#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

class StateBase
{
    friend class StateMachine;

public:
    virtual ~StateBase() = default;

    bool IsActive() const
    {
        return m_bIsActive;
    }

protected:
    virtual void Begin()
    {}

    virtual void End()
    {}

    virtual void Tick(float timeStep)
    {}

private:
    bool m_bIsActive{};

protected:
    StateMachine* m_Machine;
};
#if 0
class LoadScreenState : public StateBase
{
public:
    ResourceAreaID m_LoadingArea{};
    StateBase* NextState{};

protected:
    void Begin() override;

    void End() override;

    void Tick(float timeStep) override;
};

class IngameState : public StateBase
{
public:

protected:
    void Begin() override;

    void End() override;

    void Tick(float timeStep) override;
};
#endif
class StateMachine
{
public:
    virtual ~StateMachine() = default;

    template <typename T, typename... Args>
    T* CreateState(Args&&... args)
    {
        std::unique_ptr<StateBase> state(new T(std::forward<Args>(args)...));
        state->m_Machine = this;
        m_States.Add(std::move(state));
        return static_cast<T*>(m_States.Last().get());
    }

    void DestroyState(StateBase* state);

    void SetCurrent(StateBase* state);

    bool HasState(StateBase* state) const;

    void Tick(float timeStep);

private:
    void UpdateStateChange();

    TVector<std::unique_ptr<StateBase>> m_States;
    StateBase* m_CurrentState{};
    StateBase* m_PendingState{};
};

HK_NAMESPACE_END
