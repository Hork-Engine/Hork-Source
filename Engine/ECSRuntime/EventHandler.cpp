#include "EventHandler.h"
#include "Utils.h"

#include "Components/TransformComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/ExperimentalComponents.h"

HK_NAMESPACE_BEGIN

EventHandler::EventHandler(ECS::World* world, PhysicsInterface& physicsInterface) :
    m_World(world),
    m_PhysicsInterface(physicsInterface)
{}

void EventHandler::ProcessEvents(TVector<EventBase*> const& events)
{
    for (auto* event : events)
    {
        switch (event->Type)
        {
            case EVENT_TYPE_TRIGGER:
                HandleTriggerEvent(*static_cast<TriggerEvent*>(event));
                break;
        }
    }
}

void EventHandler::HandleTriggerEvent(TriggerEvent& event)
{
    if (event.TriggerClass == ECS::Component<JumpadComponent>::Id)
        HandleJumpadOverlap(event.Type, event.TriggerId, event.BodyId);

    else if (event.TriggerClass == ECS::Component<TeleportComponent>::Id)
        HandleTeleportOverlap(event.Type, event.TriggerId, event.BodyId);

    else if (event.TriggerClass == ECS::Component<ActivatorComponent>::Id)
    {
        LOG("Num entities in trigger {}\n", event.NumEntitiesInTrigger);
        HandleActivatorOverlap(event.Type, event.TriggerId, event.BodyId);
    }

    else if (event.TriggerClass == ECS::Component<DoorActivatorComponent>::Id)
        HandleDoorActivatorOverlap(event.Type, event.TriggerId, event.BodyId);
}

void PlaySound(StringView sound)
{}

void EventHandler::ThrowEntity(ECS::World* world, ECS::EntityHandle handle, Float3 const& velocity)
{
    m_PhysicsInterface.SetLinearVelocity(handle, velocity);
    PlaySound("throw.wav");
}

void EventHandler::HandleJumpadOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other)
{
    if (eventType == TRIGGER_EVENT_BEGIN_OVERLAP)
    {
        //LOG("HandleJumpadOverlap: OnBeginOverlap TRIGGER_{} ENTITY_{}\n", trigger, other);

        auto triggerView = m_World->GetEntityView(trigger);

        if (auto jumpad = triggerView.GetComponent<JumpadComponent>())
        {
            ThrowEntity(m_World, other, jumpad->ThrowVelocity);
        }
    }
    else if (eventType == TRIGGER_EVENT_END_OVERLAP)
    {
        //LOG("HandleJumpadOverlap: OnEndOverlap TRIGGER_{} ENTITY_{}\n", trigger, other);
    }
}

void EventHandler::HandleTeleportOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other)
{
    if (eventType == TRIGGER_EVENT_BEGIN_OVERLAP)
    {
        auto triggerView = m_World->GetEntityView(trigger);

        if (auto* teleport = triggerView.GetComponent<TeleportComponent>())
        {
            auto& commandBuffer = m_World->GetCommandBuffer(0);

            commandBuffer.AddComponent<TeleportationComponent>(other, teleport->DestPosition, teleport->DestRotation);
        }
    }
}

void EventHandler::HandleActivatorOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other)
{
    auto triggerView = m_World->GetEntityView(trigger);

    if (auto* activator = triggerView.GetComponent<ActivatorComponent>())
    {
        auto target = activator->Target;

        auto targetView = m_World->GetEntityView(target);

        ActiveComponent* component = targetView.GetComponent<ActiveComponent>();
        if (component)
        {
            auto mode = (eventType == TRIGGER_EVENT_BEGIN_OVERLAP) ? activator->TriggerEvent.OnBeginOverlap : activator->TriggerEvent.OnEndOverlap;
            switch (mode)
            {
                case ActivatorComponent::ACTIVATE:
                    component->bIsActive = true;
                    break;
                case ActivatorComponent::DEACTVATE:
                    component->bIsActive = false;
                    break;
                case ActivatorComponent::TOGGLE:
                    component->bIsActive = !component->bIsActive;
                    break;
                case ActivatorComponent::KEEP:
                    break;
            }
        }
    }
}

void EventHandler::HandleDoorActivatorOverlap(TRIGGER_EVENT eventType, ECS::EntityHandle trigger, ECS::EntityHandle other)
{
    auto triggerView = m_World->GetEntityView(trigger);

    if (auto* activator = triggerView.GetComponent<DoorActivatorComponent>())
    {
        if (eventType == TRIGGER_EVENT_BEGIN_OVERLAP)
        {
            for (ECS::EntityHandle part : activator->Parts)
            {
                auto targetView = m_World->GetEntityView(part);

                if (auto* door = targetView.GetComponent<DoorComponent>())
                {
                    if (door->m_DoorState == DoorComponent::STATE_CLOSED)
                        door->m_DoorState = DoorComponent::STATE_OPENING;

                    door->m_bIsActive = true;
                }
            }
        }
        else if (eventType == TRIGGER_EVENT_END_OVERLAP)
        {
            for (ECS::EntityHandle part : activator->Parts)
            {
                auto targetView = m_World->GetEntityView(part);

                if (auto* door = targetView.GetComponent<DoorComponent>())
                {
                    door->m_bIsActive = false;
                }
            }
        }
    }
}

HK_NAMESPACE_END
