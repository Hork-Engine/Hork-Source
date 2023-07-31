#pragma once

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>

#include "../CollisionModel.h"
#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class CollisionModel;

struct PhysBodyComponent
{
    PhysBodyComponent(CollisionModel* cmodel, PhysBodyID id) :
        m_BodyId(id),
        m_Model(cmodel)
    {}

    PhysBodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

    PhysBodyID m_BodyId;
    TRef<CollisionModel> m_Model;
    //uint8_t m_CollisionGroup;
};

struct StaticBodyComponent
{
};

struct DynamicBodyComponent
{
};

struct KinematicBodyComponent
{
};

struct RigidBodyDynamicScaling
{
    Float3 CachedScale;
};

struct TriggerComponent
{
    TriggerComponent() = default;
    TriggerComponent(ECS::ComponentTypeId triggerClass) :
        TriggerClass(triggerClass)
    {}

    ECS::ComponentTypeId TriggerClass = ECS::ComponentTypeId(-1);
};

struct WaterVolumeComponent
{
    WaterVolumeComponent(Float3 const& mins, Float3 const& maxs) :
        BoundingBox(mins, maxs)
    {}

    BvAxisAlignedBox BoundingBox;
    uint32_t CollisionGroup = CollisionGroup::DEFAULT;
};

HK_NAMESPACE_END
