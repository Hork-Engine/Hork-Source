#include "Component.h"
#include "ComponentManager.h"
#include "World.h"

HK_NAMESPACE_BEGIN

World* Component::GetWorld()
{
    return m_Manager->GetWorld();
}

World const* Component::GetWorld() const
{
    return m_Manager->GetWorld();
}

HK_NAMESPACE_END
