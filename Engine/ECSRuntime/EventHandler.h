#pragma once

#include <Engine/ECS/ECS.h>

#include "GameEvents.h"
#include "PhysicsInterface.h"

#include "Events/TriggerEvent.h"

HK_NAMESPACE_BEGIN

class EventHandler
{
public:
    EventHandler(ECS::World* world, PhysicsInterface& physicsInterface);

    void ProcessEvents(TVector<EventBase*> const& events);

private:
    void HandleTriggerEvent(TriggerEvent& event);
    void HandleJumpadOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other);
    void HandleTeleportOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other);
    void HandleActivatorOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other);
    void HandleDoorActivatorOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other);

    void ThrowEntity(ECS::World* world, ECS::EntityHandle handle, Float3 const& velocity);

    ECS::World* m_World;
    PhysicsInterface& m_PhysicsInterface;
};

HK_NAMESPACE_END
