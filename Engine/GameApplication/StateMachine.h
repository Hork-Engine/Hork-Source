#pragma once

//#include <Engine/World/Resources/ResourceManager.h>

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
