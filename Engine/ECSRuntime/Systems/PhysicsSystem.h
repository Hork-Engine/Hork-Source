#pragma once

#include <Engine/Core/BaseTypes.h>
#include "../JoltPhysics.h"
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

HK_NAMESPACE_BEGIN

namespace HashTraits
{

HK_FORCEINLINE uint32_t Hash(JPH::BodyID Key)
{
    return std::hash<uint32_t>()(Key.GetIndexAndSequenceNumber());
}

} // namespace HashTraits

HK_NAMESPACE_END


#include "EngineSystem.h"

#include "../PhysicsInterface.h"
#include "../GameEvents.h"
#include "../Events/TriggerEvent.h"
#include "../Components/RigidBodyComponent.h"

HK_NAMESPACE_BEGIN

class World;

class PhysicsSystem : public EngineSystemECS, public JPH::ContactListener, public JPH::BodyActivationListener
{
public:
    PhysicsSystem(World* world, GameEvents* gameEvents);
    ~PhysicsSystem();

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<PhysBodyComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<PhysBodyComponent> const& event);
    
    void Update(struct GameFrame const& frame);

    void DrawDebug(DebugRenderer& renderer) override;

private:
    JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
    void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;
    void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
    void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;

    void AddAndRemoveBodies(GameFrame const& frame);

    void UpdateScaling(GameFrame const& frame);
    void UpdateKinematicBodies(GameFrame const& frame);
    void UpdateWaterBodies(GameFrame const& frame);

    void StoreDynamicBodiesSnapshot();

    void DrawCollisionGeometry(DebugRenderer& renderer, CollisionModel* collisionModel, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale);

    World* m_World;
    PhysicsInterface& m_PhysicsInterface;
    GameEvents* m_GameEvents;
    int m_FrameIndex{};
    TVector<ECS::EntityHandle> m_PendingAddBodies;
    TVector<JPH::BodyID> m_PendingDestroyBodies;
    TVector<JPH::BodyID> m_BodyAddList[2];

    // Structure that keeps track of how many contact point each body has with the sensor
    struct BodyReference
    {
        JPH::BodyID mBodyID;
        int mCount;

        bool operator<(const BodyReference& inRHS) const { return mBodyID < inRHS.mBodyID; }
    };

    struct BodyAndCount
    {
        JPH::BodyID mBodyID;
        ECS::EntityHandle mEntity;
        int mCount;

        bool operator<(const BodyAndCount& inRHS) const { return mBodyID < inRHS.mBodyID; }
    };

    using BodiesInSensor = JPH::Array<BodyAndCount>;

    struct Trigger
    {
        JPH::BodyID mBodyID;
        ECS::EntityHandle mEntity;
        ECS::ComponentTypeId TriggerClass;

        BodiesInSensor mBodiesInSensor;
    };

    THashMap<JPH::BodyID, Trigger> m_Triggers;

    TVector<BodyReference> m_BodiesInSensors;
    TVector<JPH::BodyID> m_IdBodiesInSensors;

    void AddBodyReference(JPH::BodyID const& bodyId);
    void RemoveBodyReference(JPH::BodyID const& bodyId);

    Trigger* GetTriggerBody(const JPH::BodyID& inBody1, const JPH::BodyID& inBody2);

    TVector<Float3> m_DebugDrawVertices;
    TVector<unsigned int> m_DebugDrawIndices;

    JPH::Mutex mMutex; // Mutex that protects mBodiesInSensor
};

HK_NAMESPACE_END
