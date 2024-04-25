#pragma once

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

#include "../CollisionModel.h"
#include "../PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class CollisionModel;

struct RigidBodyComponent
{
    RigidBodyComponent(CollisionModel* model, PhysBodyID id) :
        m_BodyId(id),
        m_Model(model)
    {}

    PhysBodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

    CollisionModel* GetModel() const
    {
        return m_Model;
    }

private:
    PhysBodyID m_BodyId;
    TRef<CollisionModel> m_Model;
};

struct HeightFieldComponent
{
    HeightFieldComponent(TerrainCollision* model, PhysBodyID id) :
        m_BodyId(id),
        m_Model(model)
    {}

    PhysBodyID const& GetBodyId() const
    {
        return m_BodyId;
    }

    TerrainCollision* GetModel() const
    {
        return m_Model;
    }
   
private:
    PhysBodyID m_BodyId;
    TRef<TerrainCollision> m_Model;
};

struct StaticBodyComponent{};

struct DynamicBodyComponent{};

struct KinematicBodyComponent{};

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
