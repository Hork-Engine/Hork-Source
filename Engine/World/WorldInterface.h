#pragma once

#include <Engine/Core/Delegate.h>
#include "InterfaceTypeRegistry.h"
#include "TickFunction.h"

HK_NAMESPACE_BEGIN

class DebugRenderer;

class WorldInterfaceBase : public Noncopyable
{
    friend class World;

public:
    InterfaceTypeID         GetInterfaceTypeID() const;

    World*                  GetWorld();
    World const*            GetWorld() const;

protected:
                            WorldInterfaceBase() = default;
    virtual                 ~WorldInterfaceBase() {}

    virtual void            Initialize() {}
    virtual void            Deinitialize() {}

    virtual void            Purge() {}

    void                    RegisterTickFunction(TickFunction const& f);
    void                    RegisterDebugDrawFunction(Delegate<void(DebugRenderer&)> const& function);

private:
    World*                  m_World{};
    InterfaceTypeID         m_InterfaceTypeID{};
};

HK_FORCEINLINE InterfaceTypeID WorldInterfaceBase::GetInterfaceTypeID() const
{
    return m_InterfaceTypeID;
}

HK_FORCEINLINE World* WorldInterfaceBase::GetWorld()
{
    return m_World;
}

HK_FORCEINLINE World const* WorldInterfaceBase::GetWorld() const
{
    return m_World;
}

HK_NAMESPACE_END
