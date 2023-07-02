#pragma once

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>

#include "../CollisionModel_ECS.h"

HK_NAMESPACE_BEGIN

class CollisionModel;

struct StaticBodyComponent
{
    friend class PhysicsSystem_ECS;

    //StaticBodyComponent() = default;
    StaticBodyComponent(CollisionModel* cmodel, uint8_t collisionGroup) :
        m_Model(cmodel),
        m_CollisionGroup(collisionGroup)
    {}

    JPH::BodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

private:
    JPH::BodyID m_BodyId;
    TRef<CollisionModel> m_Model; // TODO: refcounted
    uint8_t m_CollisionGroup;
};

struct DynamicBodyComponent
{
    friend class PhysicsSystem_ECS;

    //DynamicBodyComponent() = default;
    DynamicBodyComponent(CollisionModel* cmodel, uint8_t collisionGroup) :
        m_Model(cmodel),
        m_CollisionGroup(collisionGroup)
    {}

    JPH::BodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

private:
    JPH::BodyID m_BodyId;
    TRef<CollisionModel> m_Model; // TODO: refcounted
    uint8_t m_CollisionGroup;
};

struct KinematicBodyComponent
{
    friend class PhysicsSystem_ECS;

    //KinematicBodyComponent() = default;
    KinematicBodyComponent(CollisionModel* cmodel, uint8_t collisionGroup) :
        m_Model(cmodel),
        m_CollisionGroup(collisionGroup)
    {}

    JPH::BodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

private:
    JPH::BodyID m_BodyId;
    TRef<CollisionModel> m_Model; // TODO: refcounted
    uint8_t m_CollisionGroup;
};

struct TriggerComponent
{
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
