#include "WorldInterface.h"
#include "World.h"

HK_NAMESPACE_BEGIN

void WorldInterfaceBase::RegisterTickFunction(TickFunction const& f)
{
    m_World->RegisterTickFunction(f);
}

void WorldInterfaceBase::RegisterDebugDrawFunction(Delegate<void(DebugRenderer&)> const& function)
{
    m_World->RegisterDebugDrawFunction(function);
}

HK_NAMESPACE_END
