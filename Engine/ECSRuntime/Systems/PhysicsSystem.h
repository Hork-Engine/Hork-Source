#pragma once

#include <Engine/Core/BaseTypes.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

HK_NAMESPACE_BEGIN

namespace HashTraits
{

HK_FORCEINLINE uint32_t Hash(JPH::BodyID Key)
{
    return std::hash<uint32_t>()(Key.GetIndexAndSequenceNumber());
}

} // namespace HashTraits

HK_NAMESPACE_END


#include <Engine/ECS/ECS.h>
#include <Engine/Runtime/DebugRenderer.h>




#include "../PhysicsInterface.h"
#include "../GameEvents.h"
#include "../Events/TriggerEvent.h"
#include "../Components/RigidBodyComponent.h"

HK_NAMESPACE_BEGIN

class PhysicsSystem_ECS : public JPH::ContactListener, public JPH::BodyActivationListener
{
public:
    PhysicsSystem_ECS(ECS::World* world, PhysicsInterface& physicsInterface, GameEvents* gameEvents);
    ~PhysicsSystem_ECS();

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<StaticBodyComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<DynamicBodyComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<KinematicBodyComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<StaticBodyComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<DynamicBodyComponent> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<KinematicBodyComponent> const& event);
    
    void Update(struct GameFrame const& frame);

    void DrawDebug(DebugRenderer& renderer);

private:
    JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
    void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;
    void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
    void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;

    void PendingDestroyBody(ECS::EntityHandle handle, JPH::BodyID bodyID);

    void AddAndRemoveBodies(GameFrame const& frame);

    void PrePhysicsUpdate(GameFrame const& frame);

    void StoreDynamicBodiesSnapshot();

    struct SphereCastResult
    {
        float HitFraction;
    };
    bool CastSphere(Float3 const& start, Float3 const& dir, float sphereRadius, SphereCastResult& result);

    void DrawCollisionGeometry(DebugRenderer& renderer, CollisionModel* collisionModel, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale);

    ECS::World* m_World;
    PhysicsInterface& m_PhysicsInterface;
    GameEvents* m_GameEvents;
    TVector<ECS::EntityHandle> m_PendingAddBodies;
    TVector<JPH::BodyID> m_PendingDestroyBodies;

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
