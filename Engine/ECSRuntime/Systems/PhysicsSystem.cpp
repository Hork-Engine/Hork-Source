#include "PhysicsSystem.h"
#include "../GameFrame.h"
#include "../Utils.h"
#include "../CollisionModel.h"
#include "../World.h"

#include "../Components/TransformComponent.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/MovableTag.h"

#include <Jolt/Core/JobSystemThreadPool.h>

// TODO: remove unused includes:
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/AABoxCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseStats.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/PulleyConstraint.h>

#include <Engine/Core/ConsoleVar.h>
#include <Engine/Runtime/PhysicsModule.h>
#include <Engine/Runtime/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCollisionModel("com_DrawCollisionModel"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCollisionShape("com_DrawCollisionShape"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawTriggers("com_DrawTriggers"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCenterOfMass("com_DrawCenterOfMass"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawWaterVolume("com_DrawWaterVolume"s, "0"s, CVAR_CHEAT);

PhysicsSystem::PhysicsSystem(World* world, GameEvents* gameEvents) :
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

    world->AddEventHandler<ECS::Event::OnComponentAdded<RigidBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<RigidBodyComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentAdded<HeightFieldComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<HeightFieldComponent>>(this);
}

PhysicsSystem::~PhysicsSystem()
{
    auto& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    if (!m_PendingDestroyBodies.IsEmpty())
    {
        body_interface.RemoveBodies(m_PendingDestroyBodies.ToPtr(), m_PendingDestroyBodies.Size());
        body_interface.DestroyBodies(m_PendingDestroyBodies.ToPtr(), m_PendingDestroyBodies.Size());

        m_PendingDestroyBodies.Clear();
    }

    {
        for (auto it : m_PhysicsInterface.m_PendingBodies)
            body_interface.DestroyBody(it.second);
        m_PhysicsInterface.m_PendingBodies.Clear();
    }
    
    m_World->RemoveHandler(this);
}

void PhysicsSystem::AddBody(ECS::EntityHandle entity)
{
    m_PendingAddBodies.Add(entity);
}

void PhysicsSystem::RemoveBody(ECS::EntityHandle entity, PhysBodyID bodyID)
{
    auto i = m_PendingAddBodies.IndexOf(entity);
    if (i != Core::NPOS)
    {
        {
            //SpinLockGuard lock(m_PhysicsInterface.m_PendingBodiesMutex);
            auto it = m_PhysicsInterface.m_PendingBodies.Find(entity);
            if (it != m_PhysicsInterface.m_PendingBodies.end())
            {
                auto& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();
                body_interface.DestroyBody(it->second);
                m_PhysicsInterface.m_PendingBodies.Erase(it);
            }
        }

        m_PendingAddBodies.Remove(i);
    }
    else
    {
        if (!bodyID.IsInvalid())
        {
            m_PendingDestroyBodies.Add(bodyID);
        }
    }
}

void PhysicsSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<RigidBodyComponent> const& event)
{
    AddBody(event.GetEntity());
}

void PhysicsSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<RigidBodyComponent> const& event)
{
    RemoveBody(event.GetEntity(), event.Component().GetBodyId());
}

void PhysicsSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<HeightFieldComponent> const& event)
{
    AddBody(event.GetEntity());
}

void PhysicsSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<HeightFieldComponent> const& event)
{
    RemoveBody(event.GetEntity(), event.Component().GetBodyId());
}

JPH::ValidateResult PhysicsSystem::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
    //LOG("Contact validate callback\n");

    // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

PhysicsSystem::Trigger* PhysicsSystem::GetTriggerBody(const JPH::BodyID& inBody1, const JPH::BodyID& inBody2)
{
    auto it = m_Triggers.Find(inBody1);
    if (it != m_Triggers.End())
        return &it->second;

    it = m_Triggers.Find(inBody2);
    if (it != m_Triggers.End())
        return &it->second;

    return nullptr;
}

void PhysicsSystem::AddBodyReference(JPH::BodyID const& bodyId)
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

void PhysicsSystem::RemoveBodyReference(JPH::BodyID const& bodyId)
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

void PhysicsSystem::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
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

void PhysicsSystem::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
    //LOG("A contact was persisted\n");
}

void PhysicsSystem::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
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

void PhysicsSystem::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    //LOG("A body got activated\n");
}

void PhysicsSystem::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    //LOG("A body went to sleep\n");
}

void PhysicsSystem::AddAndRemoveBodies(GameFrame const& frame)
{
    auto& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    if (!m_PendingDestroyBodies.IsEmpty())
    {
        for (auto& body : m_PendingDestroyBodies)
        {
            auto it = m_Triggers.Find(body);
            if (it != m_Triggers.End())
                m_Triggers.Erase(it);

            // TODO: find if body in sensors, remove it from m_BodiesInSensors
            //m_BodiesInSensors.Find()
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

            auto scale = worldTransform ? worldTransform->Scale[frame.StateIndex] : Float3(1.0f);
            auto position = worldTransform ? ConvertVector(worldTransform->Position[frame.StateIndex]) : JPH::Vec3::sZero();
            auto rotation = worldTransform ? ConvertQuaternion(worldTransform->Rotation[frame.StateIndex]) : JPH::Quat::sIdentity();

            if (RigidBodyComponent* physBody = entityView.GetComponent<RigidBodyComponent>())
            {
                if (scale != Float3(1.0f))
                {
                    auto scaledModel = physBody->GetModel()->Instatiate(scale);

                    bool bUpdateMassProperties = false;
                    body_interface.SetShape(physBody->GetBodyId(), scaledModel, bUpdateMassProperties, JPH::EActivation::DontActivate);
                }

                body_interface.SetPositionAndRotation(physBody->GetBodyId(), position, rotation, JPH::EActivation::DontActivate);

                auto motionType = body_interface.GetMotionType(physBody->GetBodyId());
                auto activation = (motionType == JPH::EMotionType::Static) ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;

                m_BodyAddList[int(activation)].Add(physBody->GetBodyId());

                if (TriggerComponent* trigger = entityView.GetComponent<TriggerComponent>())
                {
                    Trigger& t = m_Triggers[physBody->GetBodyId()];
                    t.mBodyID = physBody->GetBodyId();
                    t.mEntity = entity;
                    t.TriggerClass = trigger->TriggerClass;
                    HK_ASSERT(t.mBodiesInSensor.empty());
                }
            }
            else if (HeightFieldComponent* heightfield = entityView.GetComponent<HeightFieldComponent>())
            {
                // TODO: scaling?
                //if (scale != Float3(1.0f))
                //{
                //    auto scaledModel = physBody->GetModel()->Instatiate(scale);
                //    bool bUpdateMassProperties = false;
                //    body_interface.SetShape(physBody->GetBodyId(), scaledModel, bUpdateMassProperties, JPH::EActivation::DontActivate);
                //}

                body_interface.SetPositionAndRotation(heightfield->GetBodyId(), position, rotation, JPH::EActivation::DontActivate);

                auto activation = JPH::EActivation::DontActivate;

                m_BodyAddList[int(activation)].Add(heightfield->GetBodyId());
            }
            else
                HK_ASSERT(0);

            {
                //SpinLockGuard lock(m_PhysicsInterface.m_PendingBodiesMutex);
                m_PhysicsInterface.m_PendingBodies.Erase(entity);
            }
        }
        for (int i = 0; i < 2; i++)
        {
            if (!m_BodyAddList[i].IsEmpty())
            {
                auto addState = body_interface.AddBodiesPrepare(m_BodyAddList[i].ToPtr(), m_BodyAddList[i].Size());
                body_interface.AddBodiesFinalize(m_BodyAddList[i].ToPtr(), m_BodyAddList[i].Size(), addState, JPH::EActivation(i));

                m_BodyAddList[i].Clear();
            }
        }

        HK_ASSERT(m_PhysicsInterface.m_PendingBodies.IsEmpty());

        // TODO: optimize broadphase when too many entities are added?
        //m_PhysicsInterface.GetImpl().OptimizeBroadPhase();

        m_PendingAddBodies.Clear();
    }
}

void PhysicsSystem::UpdateScaling(GameFrame const& frame)
{
    int frameNum = frame.StateIndex;

    JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    using Query = ECS::Query<>
        ::Required<RigidBodyComponent>
        ::Required<RigidBodyDynamicScaling>
        ::ReadOnly<WorldTransformComponent>;

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
        RigidBodyComponent* bodies = q.Get<RigidBodyComponent>();
        RigidBodyDynamicScaling* cache = q.Get<RigidBodyDynamicScaling>();

        for (int i = 0; i < q.Count(); i++)
        {
            Float3 const& scale = t[i].Scale[frameNum];
            if (scale != cache[i].CachedScale)
            {
                cache[i].CachedScale = scale;

                const bool bUpdateMassProperties = false;
                body_interface.SetShape(bodies[i].GetBodyId(),
                                        bodies[i].GetModel()->Instatiate(scale),
                                        bUpdateMassProperties,
                                        JPH::EActivation::Activate);
            }
        }
    }
}

void PhysicsSystem::UpdateKinematicBodies(GameFrame const& frame)
{
    int frameNum = frame.StateIndex;

    JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    using Query = ECS::Query<>
        ::Required<RigidBodyComponent>
        ::ReadOnly<KinematicBodyComponent>
        ::ReadOnly<WorldTransformComponent>        
        ::ReadOnly<MovableTag>;

    for (Query::Iterator q(*m_World); q; q++)
    {
        WorldTransformComponent const* t = q.Get<WorldTransformComponent>();
        RigidBodyComponent* bodies = q.Get<RigidBodyComponent>();

        for (int i = 0; i < q.Count(); i++)
        {
            JPH::Vec3 position = ConvertVector(t[i].Position[frameNum]);
            JPH::Quat rotation = ConvertQuaternion(t[i].Rotation[frameNum]);

            body_interface.MoveKinematic(bodies[i].GetBodyId(), position, rotation, frame.FixedTimeStep);
        }
    }
}

void PhysicsSystem::UpdateWaterBodies(GameFrame const& frame)
{
    JPH::BroadPhaseQuery const& broadphase_query = m_PhysicsInterface.GetImpl().GetBroadPhaseQuery();

    // Broadphase results, will apply buoyancy to any body that intersects with the water volume
    class Collector : public JPH::CollideShapeBodyCollector
    {
    public:
        Collector(JPH::PhysicsSystem& inSystem, JPH::Vec3Arg inSurfaceNormal, float inDeltaTime) :
            mSystem(inSystem), mSurfacePosition(JPH::RVec3::sZero()), mSurfaceNormal(inSurfaceNormal), mDeltaTime(inDeltaTime) {}

        void SetSurfacePosition(Float3 const& inSurfacePosition)
        {
            mSurfacePosition = ConvertVector(inSurfacePosition);
        }

        void AddHit(const JPH::BodyID& inBodyID) override
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

    using Query = ECS::Query<>
        ::ReadOnly<WaterVolumeComponent>;

    for (Query::Iterator q(*m_World); q; q++)
    {
        WaterVolumeComponent const* volume = q.Get<WaterVolumeComponent>();

        for (int i = 0; i < q.Count(); i++)
        {
            JPH::AABox water_box(ConvertVector(volume[i].BoundingBox.Mins), ConvertVector(volume[i].BoundingBox.Maxs));

            Float3 surface_position = volume[i].BoundingBox.Center();
            surface_position.Y = volume[i].BoundingBox.Maxs.Y;

            collector.SetSurfacePosition(surface_position);

            ObjectLayerFilter layer_filter(m_PhysicsInterface.GetCollisionFilter(), volume[i].CollisionGroup);

            broadphase_query.CollideAABox(water_box, collector, JPH::SpecifiedBroadPhaseLayerFilter(JPH::BroadPhaseLayer(BroadphaseLayer::MOVING)), layer_filter);
        }
    }
}

void PhysicsSystem::StoreDynamicBodiesSnapshot()
{
    // Copy dynamic bodies snapshot to TransformComponent
    {
        using Query = ECS::Query<>
            ::Required<TransformComponent>
            ::ReadOnly<RigidBodyComponent>
            ::ReadOnly<DynamicBodyComponent>;

        JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

        JPH::Vec3 position;
        JPH::Quat rotation;

        for (Query::Iterator q(*m_World); q; q++)
        {
            TransformComponent* t = q.Get<TransformComponent>();
            RigidBodyComponent const* bodies = q.Get<RigidBodyComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                body_interface.GetPositionAndRotation(bodies[i].GetBodyId(), position, rotation);

                t[i].Position = ConvertVector(position);
                t[i].Rotation = ConvertQuaternion(rotation);
            }
        }
    }
}

void PhysicsSystem::Update(GameFrame const& frame)
{
    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int collisionSteps = 1;

    // If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
    const int integrationSubSteps = 1;

    auto& physicsModule = PhysicsModule::Get();

    m_FrameIndex = frame.StateIndex;

    AddAndRemoveBodies(frame);

    // NOTE: We can update scale at lower framerate to save performance
    UpdateScaling(frame);

    UpdateKinematicBodies(frame);
    UpdateWaterBodies(frame);

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
}

void PhysicsSystem::DrawCollisionGeometry(DebugRenderer& renderer, CollisionModel* collisionModel, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale)
{
    m_DebugDrawVertices.Clear();
    m_DebugDrawIndices.Clear();

    collisionModel->GatherGeometry(m_DebugDrawVertices, m_DebugDrawIndices);
    
    Float3x4 transform;
    transform.Compose(worldPosition, worldRotation.ToMatrix3x3(), collisionModel->GetValidScale(worldScale));
    
    renderer.PushTransform(transform);
    renderer.DrawTriangleSoup(m_DebugDrawVertices, m_DebugDrawIndices);
    renderer.PopTransform();
}

void PhysicsSystem::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawCollisionModel)
    {
        ShapeOverlapFilter filter;
        filter.BroadphaseLayerMask
            .AddLayer(BroadphaseLayer::NON_MOVING)
            .AddLayer(BroadphaseLayer::MOVING)
            .AddLayer(BroadphaseLayer::SENSOR);

        TVector<PhysBodyID> result;
        m_PhysicsInterface.OverlapBox(renderer.GetRenderView()->ViewPosition, Float3(5), Quat::Identity(), result, filter);

        renderer.SetDepthTest(false);
        for (PhysBodyID body_id : result)
        {
            ECS::EntityHandle entity = m_PhysicsInterface.GetEntity(body_id);
            ECS::EntityView entity_view = m_World->GetEntityView(entity);

            if (WorldTransformComponent* transform = entity_view.GetComponent<WorldTransformComponent>())
            {
                if (RigidBodyComponent* rigid_body = entity_view.GetComponent<RigidBodyComponent>())
                {
                    m_DebugDrawVertices.Clear();
                    m_DebugDrawIndices.Clear();

                    rigid_body->GetModel()->GatherGeometry(m_DebugDrawVertices, m_DebugDrawIndices);

                    Float3x4 transform_matrix;
                    transform_matrix.Compose(transform->Position[m_FrameIndex],
                                             transform->Rotation[m_FrameIndex].ToMatrix3x3(),
                                             rigid_body->GetModel()->GetValidScale(transform->Scale[m_FrameIndex]));

                    renderer.PushTransform(transform_matrix);
                    renderer.DrawTriangleSoupWireframe(m_DebugDrawVertices, m_DebugDrawIndices);
                    renderer.PopTransform();
                }
                else if (HeightFieldComponent* heightfield = entity_view.GetComponent<HeightFieldComponent>())
                {
                    m_DebugDrawVertices.Clear();
                    m_DebugDrawIndices.Clear();

                    Float3x4 transform_matrix;
                    transform_matrix.Compose(transform->Position[m_FrameIndex], transform->Rotation[m_FrameIndex].ToMatrix3x3());

                    Float3x4 transform_matrix_inv = transform_matrix.Inversed();
                    Float3 local_view_position = transform_matrix_inv * renderer.GetRenderView()->ViewPosition;

                    BvAxisAlignedBox local_bounds(local_view_position - 5, local_view_position + 5);

                    local_bounds.Mins.Y = -FLT_MAX;
                    local_bounds.Maxs.Y = FLT_MAX;

                    heightfield->GetModel()->GatherGeometry(local_bounds, m_DebugDrawVertices, m_DebugDrawIndices);

                    renderer.PushTransform(transform_matrix);
                    renderer.DrawTriangleSoupWireframe(m_DebugDrawVertices, m_DebugDrawIndices);
                    renderer.PopTransform();
                }
            }
        }
    }

    if (com_DrawCollisionShape)
    {
        ShapeOverlapFilter filter;
        filter.BroadphaseLayerMask
            .AddLayer(BroadphaseLayer::NON_MOVING)
            .AddLayer(BroadphaseLayer::MOVING)
            .AddLayer(BroadphaseLayer::SENSOR);

        TVector<PhysBodyID> result;
        m_PhysicsInterface.OverlapBox(renderer.GetRenderView()->ViewPosition, Float3(5), Quat::Identity(), result, filter);

        renderer.SetDepthTest(false);
        for (PhysBodyID body_id : result)
        {
            ECS::EntityHandle entity = m_PhysicsInterface.GetEntity(body_id);
            ECS::EntityView entity_view = m_World->GetEntityView(entity);

            if (WorldTransformComponent* transform = entity_view.GetComponent<WorldTransformComponent>())
            {
                if (RigidBodyComponent* rigid_body = entity_view.GetComponent<RigidBodyComponent>())
                {
                    auto motion_behavior = m_PhysicsInterface.GetMotionBehavior(body_id);
                    switch (motion_behavior)
                    {
                        case MB_STATIC:
                            renderer.SetColor({0.6f, 0.6f, 0.6f, 1});
                            break;

                        case MB_KINEMATIC:
                            renderer.SetColor({0, 1, 1, 1});
                            break;

                        case MB_DYNAMIC:
                            renderer.SetColor(m_PhysicsInterface.IsActive(body_id) ? Color4(1, 0, 1, 1) : Color4(1, 1, 1, 1));
                            break;
                    }

                    Float3x3 rotation_matrix = transform->Rotation[m_FrameIndex].ToMatrix3x3();

                    Float3x4 transform_matrix;
                    transform_matrix.Compose(transform->Position[m_FrameIndex],
                                             rotation_matrix,
                                             rigid_body->GetModel()->GetValidScale(transform->Scale[m_FrameIndex]));

                    rigid_body->GetModel()->DrawDebug(renderer, transform_matrix);

                    if (motion_behavior == MB_DYNAMIC)
                    {
                        renderer.SetColor({1, 1, 1, 1});
                        renderer.DrawAxis(transform->Position[m_FrameIndex], rotation_matrix[0], rotation_matrix[1], rotation_matrix[2], Float3(0.25f));
                    }
                }
            }
        }
    }

    if (com_DrawWaterVolume)
    {
        using Query = ECS::Query<>
            ::ReadOnly<WaterVolumeComponent>;

        renderer.SetDepthTest(true);
        renderer.SetColor({0, 0, 1, 0.5f});

        for (Query::Iterator q(*m_World); q; q++)
        {
            WaterVolumeComponent const* volume = q.Get<WaterVolumeComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                renderer.DrawBoxFilled(volume[i].BoundingBox.Center(), volume[i].BoundingBox.HalfSize(), true);
            }
        }
    }

    if (com_DrawTriggers)
    {
        using Query = ECS::Query<>
            ::ReadOnly<RigidBodyComponent>
            ::ReadOnly<WorldTransformComponent>
            ::ReadOnly<TriggerComponent>;

        renderer.SetDepthTest(true);
        renderer.SetColor({0, 1, 0, 0.5f});

        for (Query::Iterator q(*m_World); q; q++)
        {
            RigidBodyComponent const* bodies = q.Get<RigidBodyComponent>();
            WorldTransformComponent const* transforms = q.Get<WorldTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                DrawCollisionGeometry(renderer, bodies[i].GetModel(), transforms[i].Position[m_FrameIndex], transforms[i].Rotation[m_FrameIndex], transforms[i].Scale[m_FrameIndex]);
            }
        }
    }

    if (com_DrawCenterOfMass)
    {
        using Query = ECS::Query<>
            ::ReadOnly<RigidBodyComponent>
            ::ReadOnly<DynamicBodyComponent>
            ::ReadOnly<WorldTransformComponent>;

        JPH::BodyInterface& body_interface = m_PhysicsInterface.GetImpl().GetBodyInterface();

        renderer.SetDepthTest(false);

        for (Query::Iterator q(*m_World); q; q++)
        {
            RigidBodyComponent const* bodies = q.Get<RigidBodyComponent>();
            WorldTransformComponent const* transforms = q.Get<WorldTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                auto* model = bodies[i].GetModel();

                Float3x4 transform;
                transform.Compose(transforms[i].Position[m_FrameIndex],
                                  transforms[i].Rotation[m_FrameIndex].ToMatrix3x3(),
                                  model->GetValidScale(transforms[i].Scale[m_FrameIndex]));

                Float3 center_of_mass_pos = transform * model->GetCenterOfMass();
                Float3 center_of_mass_pos2 = ConvertVector(body_interface.GetCenterOfMassPosition(bodies[i].GetBodyId()));

                renderer.SetColor({1, 1, 1, 1});
                renderer.DrawBoxFilled(center_of_mass_pos, Float3(0.05f));

                renderer.SetColor({1, 0, 0, 1});
                renderer.DrawBoxFilled(center_of_mass_pos2, Float3(0.05f));
            }
        }
    }
}

HK_NAMESPACE_END
