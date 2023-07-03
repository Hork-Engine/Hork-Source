#include "PhysicsSystem.h"
#include "../GameFrame.h"
#include "../Utils.h"
#include "../CollisionModel_ECS.h"
#include "../World.h"

#include "../Components/TransformComponent.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/FinalTransformComponent.h"
#include "../Components/NodeComponent.h"
#include "../Components/MovableTag.h"
#include "../Components/ExperimentalComponents.h"
#include "../Components/SpringArmComponent.h"

//#include <Jolt/Physics/Collision/ShapeCast.h>

// TODO: remove unused includes:
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/AABoxCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/NarrowPhaseStats.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/PulleyConstraint.h>

#include <Engine/Core/ConsoleVar.h>
#include <Engine/Runtime/PhysicsModule.h>
#include <Engine/Runtime/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCollisionModel("com_DrawCollisionModel"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawTriggers("com_DrawTriggers"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCenterOfMass("com_DrawCenterOfMass"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawWaterVolume("com_DrawWaterVolume"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawStaticBodies("com_DrawStaticBodies"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawDynamicBodies("com_DrawDynamicBodies"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawKinematicBodies("com_DrawKinematicBodies"s, "0"s, CVAR_CHEAT);

PhysicsSystem_ECS::PhysicsSystem_ECS(World_ECS* world, GameEvents* gameEvents) :
    m_World(world),
    m_PhysicsInterface(world->GetPhysicsInterface()),
    m_GameEvents(gameEvents)
{
    // A body activation listener gets notified when bodies activate and go to sleep
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.

    m_PhysicsInterface.GetImpl().SetBodyActivationListener(this);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.

    m_PhysicsInterface.GetImpl().SetContactListener(this);

    // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
    // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
    // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
    m_PhysicsInterface.GetImpl().OptimizeBroadPhase();

    world->AddEventHandler<ECS::Event::OnComponentAdded<StaticBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentAdded<DynamicBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentAdded<KinematicBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<StaticBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<DynamicBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<KinematicBodyComponent>>(this);
}

PhysicsSystem_ECS::~PhysicsSystem_ECS()
{
    auto& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    if (!m_PendingDestroyBodies.IsEmpty())
    {
        body_interface.RemoveBodies(m_PendingDestroyBodies.ToPtr(), m_PendingDestroyBodies.Size());
        body_interface.DestroyBodies(m_PendingDestroyBodies.ToPtr(), m_PendingDestroyBodies.Size());

        m_PendingDestroyBodies.Clear();
    }

    m_World->RemoveHandler(this);
}

void PhysicsSystem_ECS::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<StaticBodyComponent> const& event)
{
    m_PendingAddBodies.Add(event.GetEntity());
}

void PhysicsSystem_ECS::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<DynamicBodyComponent> const& event)
{
    m_PendingAddBodies.Add(event.GetEntity());
}

void PhysicsSystem_ECS::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<KinematicBodyComponent> const& event)
{
    m_PendingAddBodies.Add(event.GetEntity());
}

void PhysicsSystem_ECS::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<StaticBodyComponent> const& event)
{
    PendingDestroyBody(event.GetEntity(), event.Component().m_BodyId);
}

void PhysicsSystem_ECS::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<DynamicBodyComponent> const& event)
{
    PendingDestroyBody(event.GetEntity(), event.Component().m_BodyId);
}

void PhysicsSystem_ECS::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<KinematicBodyComponent> const& event)
{
    PendingDestroyBody(event.GetEntity(), event.Component().m_BodyId);
}

void PhysicsSystem_ECS::PendingDestroyBody(ECS::EntityHandle handle, JPH::BodyID bodyID)
{
    auto i = m_PendingAddBodies.IndexOf(handle);
    if (i != Core::NPOS)
        m_PendingAddBodies.Remove(i);

    if (!bodyID.IsInvalid())
    {
        m_PendingDestroyBodies.Add(bodyID);        
    }
}

JPH::ValidateResult PhysicsSystem_ECS::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
    //LOG("Contact validate callback\n");

    // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

PhysicsSystem_ECS::Trigger* PhysicsSystem_ECS::GetTriggerBody(const JPH::BodyID& inBody1, const JPH::BodyID& inBody2)
{
    auto it = m_Triggers.Find(inBody1);
    if (it != m_Triggers.End())
        return &it->second;

    it = m_Triggers.Find(inBody2);
    if (it != m_Triggers.End())
        return &it->second;

    return nullptr;
}

void PhysicsSystem_ECS::AddBodyReference(JPH::BodyID const& bodyId)
{
    // Add to list and make sure that the list remains sorted for determinism (contacts can be added from multiple threads)
    BodyReference bodyRef{bodyId, 1};
    auto b = std::lower_bound(m_BodiesInSensors.begin(), m_BodiesInSensors.end(), bodyRef);
    if (b != m_BodiesInSensors.end() && b->mBodyID == bodyId)
    {
        // This is the right body, increment reference
        b->mCount++;
        return;
    }
    m_BodiesInSensors.Insert(b, bodyRef);
}

void PhysicsSystem_ECS::RemoveBodyReference(JPH::BodyID const& bodyId)
{
    BodyReference bodyRef{bodyId, 1};
    auto b = std::lower_bound(m_BodiesInSensors.begin(), m_BodiesInSensors.end(), bodyRef);
    if (b != m_BodiesInSensors.end() && b->mBodyID == bodyId)
    {
        // This is the right body, increment reference
        HK_ASSERT(b->mCount > 0);
        b->mCount--;

        // When last reference remove from the list
        if (b->mCount == 0)
        {
            m_BodiesInSensors.Erase(b);
        }
        return;
    }
}

void PhysicsSystem_ECS::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
    Trigger* trigger = GetTriggerBody(inBody1.GetID(), inBody2.GetID());
    if (!trigger)
        return;

    JPH::BodyID body_id = trigger->mBodyID == inBody1.GetID() ? inBody2.GetID() : inBody1.GetID();

    ECS::EntityHandle bodyEntity = trigger->mBodyID == inBody1.GetID() ? ECS::EntityHandle(inBody2.GetUserData()) : ECS::EntityHandle(inBody1.GetUserData());

    std::lock_guard lock(mMutex);

    // Make list of unique IDs
    AddBodyReference(body_id);

    // Add to list and make sure that the list remains sorted for determinism (contacts can be added from multiple threads)
    BodyAndCount body_and_count{body_id, bodyEntity, 1};
    BodiesInSensor& bodies_in_sensor = trigger->mBodiesInSensor;
    BodiesInSensor::iterator b = lower_bound(bodies_in_sensor.begin(), bodies_in_sensor.end(), body_and_count);
    if (b != bodies_in_sensor.end() && b->mBodyID == body_id)
    {
        // This is the right body, increment reference
        b->mCount++;
        return;
    }
    bodies_in_sensor.insert(b, body_and_count);

    TriggerEvent& event = m_GameEvents->AddEvent<TriggerEvent>();
    event.Type = TRIGGER_EVENT_BEGIN_OVERLAP;
    event.TriggerClass = trigger->TriggerClass;
    event.TriggerId = trigger->mEntity;
    event.BodyId = bodyEntity;
    event.NumEntitiesInTrigger = bodies_in_sensor.size();
}

void PhysicsSystem_ECS::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
    //LOG("A contact was persisted\n");
}

void PhysicsSystem_ECS::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
    Trigger* trigger = GetTriggerBody(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());
    if (!trigger)
        return;

    JPH::BodyID body_id = trigger->mBodyID == inSubShapePair.GetBody1ID() ? inSubShapePair.GetBody2ID() : inSubShapePair.GetBody1ID();

    std::lock_guard lock(mMutex);

    // Remove list of unique IDs
    RemoveBodyReference(body_id);

    // Remove from list
    BodyAndCount body_and_count{body_id, 0, 1};
    BodiesInSensor& bodies_in_sensor = trigger->mBodiesInSensor;
    BodiesInSensor::iterator b = lower_bound(bodies_in_sensor.begin(), bodies_in_sensor.end(), body_and_count);
    if (b != bodies_in_sensor.end() && b->mBodyID == body_id)
    {
        // This is the right body, increment reference
        HK_ASSERT(b->mCount > 0);
        b->mCount--;

        // When last reference remove from the list
        if (b->mCount == 0)
        {
            TriggerEvent& event = m_GameEvents->AddEvent<TriggerEvent>();
            event.Type = TRIGGER_EVENT_END_OVERLAP;
            event.TriggerClass = trigger->TriggerClass;
            event.TriggerId = trigger->mEntity;
            event.BodyId = b->mEntity;
            event.NumEntitiesInTrigger = bodies_in_sensor.size() - 1;

            ECS::EntityView triggerView = m_World->GetEntityView(event.TriggerId);

            TriggerComponent* triggerComponent = triggerView.GetComponent<TriggerComponent>();
            event.TriggerClass = triggerComponent ? triggerComponent->TriggerClass : ECS::ComponentTypeId(-1);

            bodies_in_sensor.erase(b);
        }
        return;
    }
    HK_ASSERT_(false, "Body pair not found");
}

void PhysicsSystem_ECS::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    //LOG("A body got activated\n");
}

void PhysicsSystem_ECS::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    //LOG("A body went to sleep\n");
}

void PhysicsSystem_ECS::AddAndRemoveBodies(GameFrame const& frame)
{
    auto& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    if (!m_PendingDestroyBodies.IsEmpty())
    {
        for (auto& body : m_PendingDestroyBodies)
        {
            auto it = m_Triggers.Find(body);
            if (it != m_Triggers.End())
                m_Triggers.Erase(it);
        }

        body_interface.RemoveBodies(m_PendingDestroyBodies.ToPtr(), m_PendingDestroyBodies.Size());
        body_interface.DestroyBodies(m_PendingDestroyBodies.ToPtr(), m_PendingDestroyBodies.Size());

        m_PendingDestroyBodies.Clear();
    }

    if (!m_PendingAddBodies.IsEmpty())
    {
        for (auto entity : m_PendingAddBodies)
        {
            ECS::EntityView entityView = m_World->GetEntityView(entity);

            WorldTransformComponent* worldTransform = entityView.GetComponent<WorldTransformComponent>();
            TriggerComponent* trigger = entityView.GetComponent<TriggerComponent>();
            StaticBodyComponent* staticBody = entityView.GetComponent<StaticBodyComponent>();
            DynamicBodyComponent* dynamicBody = entityView.GetComponent<DynamicBodyComponent>();
            KinematicBodyComponent* kinematicBody = entityView.GetComponent<KinematicBodyComponent>();

            JPH::BodyID* pBody{};
            JPH::uint8 group{};
            JPH::uint8 broadphase{};
            JPH::EActivation activation{JPH::EActivation::DontActivate};
            JPH::EMotionType motionType{JPH::EMotionType::Static};
            int checksum = 0;

            CollisionModel* collisionModel{};

            if (staticBody)
            {
                pBody = &staticBody->m_BodyId;
                checksum++;

                activation = JPH::EActivation::DontActivate;
                motionType = JPH::EMotionType::Static;

                collisionModel = staticBody->m_Model;
                group = staticBody->m_CollisionGroup;
            }
            if (dynamicBody)
            {
                pBody = &dynamicBody->m_BodyId;
                checksum++;

                activation = JPH::EActivation::Activate;
                motionType = JPH::EMotionType::Dynamic;

                collisionModel = dynamicBody->m_Model;
                group = dynamicBody->m_CollisionGroup;
            }
            if (kinematicBody)
            {
                pBody = &kinematicBody->m_BodyId;
                checksum++;

                activation = JPH::EActivation::Activate;
                motionType = JPH::EMotionType::Kinematic;

                collisionModel = kinematicBody->m_Model;
                group = kinematicBody->m_CollisionGroup;
            }
            HK_ASSERT(checksum == 1);

            if (checksum != 1)
                continue;

            if (trigger)
            {
                broadphase = BroadphaseLayer::SENSOR;

                if (dynamicBody)
                {
                    LOG("WARNING: Triggers can only have STATIC or KINEMATIC motion behavior but set to SIMULATED.\n");

                    activation = JPH::EActivation::DontActivate;
                    motionType = JPH::EMotionType::Static;
                }
            }
            else
            {
                broadphase = staticBody ? BroadphaseLayer::NON_MOVING : BroadphaseLayer::MOVING;
            }

            JPH::ObjectLayer layer = MakeObjectLayer(group, broadphase);

            Float3 scale = worldTransform ? worldTransform->Scale[frame.StateIndex] : Float3(1.0f);

            JPH::BodyCreationSettings settings(collisionModel->Instatiate(scale),
                                               worldTransform ? ConvertVector(worldTransform->Position[frame.StateIndex]) : JPH::Vec3::sZero(),
                                               worldTransform ? ConvertQuaternion(worldTransform->Rotation[frame.StateIndex]) : JPH::Quat::sIdentity(),
                                               motionType,
                                               layer);

            if (motionType == JPH::EMotionType::Dynamic)
            {
                settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                settings.mMassPropertiesOverride.mMass = 10.0f;
            }

            if (trigger)
            {
                settings.mIsSensor = true;
            }

            settings.mUserData = entity;

            *pBody = body_interface.CreateAndAddBody(settings, activation);

            if (trigger)
            {
                Trigger& t = m_Triggers[*pBody];
                t.mBodyID = *pBody;
                t.mEntity = entity;
                t.TriggerClass = trigger->TriggerClass;
                HK_ASSERT(t.mBodiesInSensor.empty());
            }
        }
        m_PendingAddBodies.Clear();
    }
}

void PhysicsSystem_ECS::PrePhysicsUpdate(GameFrame const& frame)
{
    int frameNum = frame.StateIndex;

    JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    // Update kinematic bodies
    {
        using Query = ECS::Query<>
            ::ReadOnly<WorldTransformComponent>
            ::Required<KinematicBodyComponent>
            ::ReadOnly<MovableTag>;

        for (Query::Iterator q(*m_World); q; q++)
        {
            WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
            KinematicBodyComponent* bodies = q.Get<KinematicBodyComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                JPH::Vec3 position = ConvertVector(t[i].Position[frameNum]);
                JPH::Quat rotation = ConvertQuaternion(t[i].Rotation[frameNum]);

                body_interface.MoveKinematic(bodies[i].m_BodyId, position, rotation, frame.FixedTimeStep);
            }
        }
    }

    // Update water
    {
        using Query = ECS::Query<>
            ::ReadOnly<WaterVolumeComponent>;

        JPH::BroadPhaseQuery const& broadPhaseQuery = m_PhysicsInterface.GetImpl().GetBroadPhaseQuery();

        // Broadphase results, will apply buoyancy to any body that intersects with the water volume
        class Collector : public JPH::CollideShapeBodyCollector
        {
        public:
            Collector(JPH::PhysicsSystem& inSystem, JPH::Vec3Arg inSurfaceNormal, float inDeltaTime) :
                mSystem(inSystem), mSurfacePosition(JPH::RVec3::sZero()), mSurfaceNormal(inSurfaceNormal), mDeltaTime(inDeltaTime) {}

            void SetSurfacePosition(Float3 const& surfacePosition)
            {
                mSurfacePosition = ConvertVector(surfacePosition);
            }

            virtual void AddHit(const JPH::BodyID& inBodyID) override
            {
                JPH::BodyLockWrite lock(mSystem.GetBodyLockInterface(), inBodyID);
                JPH::Body& body = lock.GetBody();
                if (body.IsActive() && body.GetMotionType() == JPH::EMotionType::Dynamic)
                    body.ApplyBuoyancyImpulse(mSurfacePosition, mSurfaceNormal, 1.1f, 0.3f, 0.05f, JPH::Vec3::sZero(), mSystem.GetGravity(), mDeltaTime);

                if (body.GetMotionType() != JPH::EMotionType::Dynamic)
                    LOG("Motion type {}\n", body.GetMotionType() == JPH::EMotionType::Static ? "Static" : "Kinematic");
            }

        private:
            JPH::PhysicsSystem& mSystem;
            JPH::RVec3 mSurfacePosition;
            JPH::Vec3 mSurfaceNormal;
            float mDeltaTime;
        };

        Collector collector(m_PhysicsInterface.GetImpl(), JPH::Vec3::sAxisY(), frame.FixedTimeStep);

        for (Query::Iterator q(*m_World); q; q++)
        {
            WaterVolumeComponent const* waterVolume = q.Get<WaterVolumeComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                JPH::AABox water_box(ConvertVector(waterVolume[i].BoundingBox.Mins), ConvertVector(waterVolume[i].BoundingBox.Maxs));

                Float3 surfacePosition = waterVolume[i].BoundingBox.Center();
                surfacePosition.Y = waterVolume[i].BoundingBox.Maxs.Y;

                collector.SetSurfacePosition(surfacePosition);

                ObjectLayerFilter layerFilter(m_PhysicsInterface.GetCollisionFilter(), waterVolume[i].CollisionGroup);

                broadPhaseQuery.CollideAABox(water_box, collector, JPH::SpecifiedBroadPhaseLayerFilter(JPH::BroadPhaseLayer(BroadphaseLayer::MOVING)), layerFilter);
            }
        }
    }
}

void PhysicsSystem_ECS::StoreDynamicBodiesSnapshot()
{
    // Copy dynamic bodies snapshot to TransformComponent
    {
        using Query = ECS::Query<>
            ::Required<TransformComponent>
            ::ReadOnly<DynamicBodyComponent>;

        JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

        JPH::Vec3 position;
        JPH::Quat rotation;

        for (Query::Iterator q(*m_World); q; q++)
        {
            TransformComponent* t = q.Get<TransformComponent>();
            DynamicBodyComponent const* bodies = q.Get<DynamicBodyComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                body_interface.GetPositionAndRotation(bodies[i].m_BodyId, position, rotation);

                t[i].Position = ConvertVector(position);
                t[i].Rotation = ConvertQuaternion(rotation);
            }
        }
    }
}

void PhysicsSystem_ECS::Update(GameFrame const& frame)
{
    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int collisionSteps = 1;

    // If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
    const int integrationSubSteps = 1;

    auto& physicsModule = PhysicsModule::Get();

    AddAndRemoveBodies(frame);

    PrePhysicsUpdate(frame);

    m_PhysicsInterface.GetImpl().Update(frame.FixedTimeStep, collisionSteps, integrationSubSteps, physicsModule.GetTempAllocator(), physicsModule.GetJobSystemThreadPool());

    // Keep bodies in trigger active
    m_IdBodiesInSensors.Clear();
    for (auto& bodyRef : m_BodiesInSensors)
        m_IdBodiesInSensors.Add(bodyRef.mBodyID);

    if (!m_IdBodiesInSensors.IsEmpty())
    {
        m_PhysicsInterface.GetImpl().GetBodyInterface().ActivateBodies(m_IdBodiesInSensors.ToPtr(), m_IdBodiesInSensors.Size());
    }

    StoreDynamicBodiesSnapshot();

    // Update spring arms
    {
        using Query = ECS::Query<>
            ::Required<TransformComponent>
            ::ReadOnly<WorldTransformComponent>
            ::Required<SpringArmComponent>;

        int frameNum = frame.StateIndex;

        SphereCastResult result;

        for (Query::Iterator q(*m_World); q; q++)
        {
            TransformComponent* transform = q.Get<TransformComponent>();
            WorldTransformComponent const* worldTransform = q.Get<WorldTransformComponent>();
            SpringArmComponent* springArm = q.Get<SpringArmComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                Float3 dir = worldTransform[i].Rotation[frameNum].ZAxis();
                Float3 worldPos = worldTransform[i].Position[frameNum] - dir * springArm[i].ActualDistance;
                
                if (m_PhysicsInterface.CastSphere(worldPos, dir * springArm[i].DesiredDistance, SpringArmComponent::SPRING_ARM_SPHERE_CAST_RADIUS, result))
                {
                    float distance = springArm[i].DesiredDistance * result.HitFraction;

                    //if (springArm[i].ActualDistance > distance)
                    //    springArm[i].ActualDistance = distance;
                    //else
                        //springArm[i].ActualDistance = Math::Lerp(springArm[i].ActualDistance, distance, springArm[i].Speed * frame.FixedTimeStep);
                        springArm[i].ActualDistance = Math::Lerp(springArm[i].ActualDistance, distance, 0.5f);

                    if (springArm[i].ActualDistance < springArm[i].MinDistance)
                        springArm[i].ActualDistance = springArm[i].MinDistance;
                }
                else
                {
                    springArm[i].ActualDistance = Math::Lerp(springArm[i].ActualDistance, springArm[i].DesiredDistance, springArm[i].Speed * frame.FixedTimeStep);
                }

                transform[i].Position.Z = springArm[i].ActualDistance;
            }
        }
    }
}

void PhysicsSystem_ECS::DrawCollisionGeometry(DebugRenderer& renderer, CollisionModel* collisionModel, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale)
{
    m_DebugDrawVertices.Clear();
    m_DebugDrawIndices.Clear();
    collisionModel->GatherGeometry(m_DebugDrawVertices, m_DebugDrawIndices, worldPosition, worldRotation, worldScale);
    renderer.DrawTriangleSoup(m_DebugDrawVertices, m_DebugDrawIndices);
}

void PhysicsSystem_ECS::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawCollisionModel)
    {
        renderer.SetDepthTest(true);
        renderer.SetRandomColors(true);

        {
            using Query = ECS::Query<>::ReadOnly<StaticBodyComponent>::ReadOnly<FinalTransformComponent>;

            for (Query::Iterator q(*m_World); q; q++)
            {
                StaticBodyComponent const* bodies = q.Get<StaticBodyComponent>();
                FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

                // Exclude triggers
                if (q.HasComponent<TriggerComponent>())
                    continue;

                for (int i = 0; i < q.Count(); i++)
                {
                    DrawCollisionGeometry(renderer, bodies[i].m_Model, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
                }
            }
        }
        {
            using Query = ECS::Query<>::ReadOnly<KinematicBodyComponent>::ReadOnly<FinalTransformComponent>;

            for (Query::Iterator q(*m_World); q; q++)
            {
                KinematicBodyComponent const* bodies = q.Get<KinematicBodyComponent>();
                FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

                // Exclude triggers
                if (q.HasComponent<TriggerComponent>())
                    continue;

                for (int i = 0; i < q.Count(); i++)
                {
                    DrawCollisionGeometry(renderer, bodies[i].m_Model, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
                }
            }
        }
        {
            using Query = ECS::Query<>::ReadOnly<DynamicBodyComponent>::ReadOnly<FinalTransformComponent>;

            for (Query::Iterator q(*m_World); q; q++)
            {
                DynamicBodyComponent const* bodies = q.Get<DynamicBodyComponent>();
                FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

                // Exclude triggers
                if (q.HasComponent<TriggerComponent>())
                    continue;

                for (int i = 0; i < q.Count(); i++)
                {
                    DrawCollisionGeometry(renderer, bodies[i].m_Model, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
                }
            }
        }

        renderer.SetRandomColors(false);
    }

    // Draw water
    if (com_DrawWaterVolume)
    {
        using Query = ECS::Query<>::ReadOnly<WaterVolumeComponent>;

        renderer.SetDepthTest(true);
        renderer.SetColor(Color4(0, 0, 1, 0.5f));

        for (Query::Iterator q(*m_World); q; q++)
        {
            WaterVolumeComponent const* waterVolume = q.Get<WaterVolumeComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                renderer.DrawBoxFilled(waterVolume[i].BoundingBox.Center(), waterVolume[i].BoundingBox.HalfSize(), true);
            }
        }
    }

    if (com_DrawTriggers)
    {
        using Query = ECS::Query<>::ReadOnly<StaticBodyComponent>::ReadOnly<FinalTransformComponent>::ReadOnly<TriggerComponent>;

        renderer.SetDepthTest(true);
        renderer.SetColor(Color4(0, 1, 0, 0.5f));

        for (Query::Iterator q(*m_World); q; q++)
        {
            StaticBodyComponent const* bodies = q.Get<StaticBodyComponent>();
            FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                DrawCollisionGeometry(renderer, bodies[i].m_Model, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
            }
        }
    }

    // Draw static bodies
    if (com_DrawStaticBodies)
    {
        using Query = ECS::Query<>::ReadOnly<StaticBodyComponent>::ReadOnly<FinalTransformComponent>;

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(0.6f, 0.6f, 0.6f, 1));

        for (Query::Iterator q(*m_World); q; q++)
        {
            StaticBodyComponent const* bodies = q.Get<StaticBodyComponent>();
            FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                bodies[i].m_Model->DrawDebug(renderer, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
            }
        }
    }

    if (com_DrawKinematicBodies)
    {
        using Query = ECS::Query<>::ReadOnly<KinematicBodyComponent>::ReadOnly<FinalTransformComponent>;

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(0, 1, 1, 1));

        for (Query::Iterator q(*m_World); q; q++)
        {
            KinematicBodyComponent const* bodies = q.Get<KinematicBodyComponent>();
            FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                bodies[i].m_Model->DrawDebug(renderer, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
            }
        }
    }

    // Draw dynamic bodies
    if (com_DrawDynamicBodies)
    {
        using Query = ECS::Query<>::ReadOnly<DynamicBodyComponent>::ReadOnly<FinalTransformComponent>;

        JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

        renderer.SetDepthTest(false);

        for (Query::Iterator q(*m_World); q; q++)
        {
            DynamicBodyComponent const* bodies = q.Get<DynamicBodyComponent>();
            FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                auto* collisionModel = bodies[i].m_Model.GetObject();

                if (body_interface.IsActive(bodies[i].m_BodyId))
                {
                    renderer.SetColor(Color4(1, 0, 1, 1));
                }
                else
                {
                    renderer.SetColor(Color4(1, 1, 1, 1));
                }

                collisionModel->DrawDebug(renderer, transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);

                Float3x3 r = transforms[i].Rotation.ToMatrix3x3();

                renderer.SetColor(Color4(1, 1, 1, 1));
                renderer.DrawAxis(transforms[i].Position, r[0], r[1], r[2], Float3(0.25f));
            }
        }
    }

    if (com_DrawCenterOfMass)
    {
        using Query = ECS::Query<>::ReadOnly<DynamicBodyComponent>::ReadOnly<FinalTransformComponent>;

        JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

        renderer.SetDepthTest(false);

        for (Query::Iterator q(*m_World); q; q++)
        {
            DynamicBodyComponent const* bodies = q.Get<DynamicBodyComponent>();
            FinalTransformComponent const* transforms = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                auto* collisionModel = bodies[i].m_Model.GetObject();

                Float3 centerOfMassPos = collisionModel->GetCenterOfMassWorldPosition(transforms[i].Position, transforms[i].Rotation, transforms[i].Scale);
                Float3 centerOfMassPos2 = ConvertVector(body_interface.GetCenterOfMassPosition(bodies[i].m_BodyId));

                renderer.SetColor(Color4(1, 1, 1, 1));
                renderer.DrawBoxFilled(centerOfMassPos, Float3(0.05f));

                renderer.SetColor(Color4(1, 0, 0, 1));
                renderer.DrawBoxFilled(centerOfMassPos2, Float3(0.05f));
            }
        }
    }
}

HK_NAMESPACE_END
