#pragma once

#include "../GameEvents.h"

HK_NAMESPACE_BEGIN

enum EVENT_TYPE
{
    EVENT_TYPE_TRIGGER
};

enum TRIGGER_EVENT
{
    TRIGGER_EVENT_BEGIN_OVERLAP,
    TRIGGER_EVENT_END_OVERLAP,
};

struct TriggerEvent : EventBase
{
    TriggerEvent() :
        EventBase(EVENT_TYPE_TRIGGER)
    {}

    TRIGGER_EVENT Type;
    ECS::ComponentTypeId TriggerClass;
    ECS::EntityHandle Trigger;
    ECS::EntityHandle Other;
    uint32_t NumEntitiesInTrigger;
};

HK_NAMESPACE_END
