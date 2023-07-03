#include "PhysicsInterface.h"

#include "Components/RigidBodyComponent.h"
#include "Components/CharacterControllerComponent.h"

#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

HK_NAMESPACE_BEGIN

CollisionFilter::CollisionFilter()
{
    Core::ZeroMem(m_CollisionMask.ToPtr(), m_CollisionMask.Size() * sizeof(m_CollisionMask[0]));
    //Core::ZeroMem(m_GroupName.ToPtr(), m_GroupName.Size() & sizeof(m_GroupName[0]));
}

void CollisionFilter::Clear()
{
    Core::ZeroMem(m_CollisionMask.ToPtr(), m_CollisionMask.Size() * sizeof(m_CollisionMask[0]));
}

//void CollisionFilter::SetGroupName(uint32_t group, StringView name)
//{
//    Core::StrcpyN(m_GroupName[group], sizeof(m_GroupName[group]), name.ToPtr(), name.Size());
//}
//
//uint32_t CollisionFilter::FindGroup(StringView name) const
//{
//    for (uint32_t i = 0; i < m_GroupName.Size(); i++)
//    {
//        if (name.Icompare(m_GroupName[i]))
//            return i;
//    }
//    return uint32_t(-1);
//}

void CollisionFilter::SetShouldCollide(uint32_t group1, uint32_t group2, bool shouldCollide)
{
    if (shouldCollide)
    {
        m_CollisionMask[group1] |= HK_BIT(group2);
        m_CollisionMask[group2] |= HK_BIT(group1);
    }
    else
    {
        m_CollisionMask[group1] &= ~HK_BIT(group2);
        m_CollisionMask[group2] &= ~HK_BIT(group1);
    }
}

bool CollisionFilter::ShouldCollide(uint32_t group1, uint32_t group2) const
{
    return (m_CollisionMask[group1] & HK_BIT(group2)) != 0;
}


PhysicsInterface::PhysicsInterface(ECS::World* world) :
    m_World(world),
    m_ObjectVsObjectLayerFilter(m_CollisionFilter)
{
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint cMaxBodies = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const JPH::uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint cMaxBodyPairs = 1024;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
    const JPH::uint cMaxContactConstraints = 1024;

    // TODO: Move to game setup/config/resource
    m_CollisionFilter.SetShouldCollide(CollisionGroup::CHARACTER, CollisionGroup::CHARACTER, true);
    m_CollisionFilter.SetShouldCollide(CollisionGroup::CHARACTER, CollisionGroup::DEFAULT, true);
    m_CollisionFilter.SetShouldCollide(CollisionGroup::CHARACTER, CollisionGroup::PLATFORM, true);
    m_CollisionFilter.SetShouldCollide(CollisionGroup::CHARACTER, CollisionGroup::TRIGGER_CHARACTER, true);
    m_CollisionFilter.SetShouldCollide(CollisionGroup::DEFAULT, CollisionGroup::DEFAULT, true);
    m_CollisionFilter.SetShouldCollide(CollisionGroup::PLATFORM, CollisionGroup::DEFAULT, true);
    m_CollisionFilter.SetShouldCollide(CollisionGroup::WATER, CollisionGroup::DEFAULT, true);

    // Now we can create the actual physics system.
    m_PhysicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_BroadPhaseLayerInterface, m_ObjectVsBroadPhaseLayerFilter, m_ObjectVsObjectLayerFilter);
}

bool PhysicsInterface::CastSphere(Float3 const& start, Float3 const& dir, float sphereRadius, SphereCastResult& result)
{
    JPH::SphereShape sphereShape(sphereRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    JPH::ShapeCastSettings settings;
    //settings.mUseShrunkenShapeAndConvexRadius = mUseShrunkenShapeAndConvexRadius;
    //settings.mActiveEdgeMode = mActiveEdgeMode;
    settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mBackFaceModeConvex = JPH::EBackFaceMode::CollideWithBackFaces;
    //settings.mReturnDeepestPoint = mReturnDeepestPoint;
    //settings.mCollectFacesMode = mCollectFacesMode;
    settings.mReturnDeepestPoint = true;

    // Define a base offset that is halfway the probe to test getting the collision results relative to some offset.
    // Note that this is not necessarily the best choice for a base offset, but we want something that's not zero
    // and not the start of the collision test either to ensure that we'll see errors in the algorithm.
    JPH::RVec3 baseOffset = pos + 0.5f * direction;

    class BroadphaseLayerFilter final : public JPH::BroadPhaseLayerFilter
    {
    public:
        BroadphaseLayerFilter(uint32_t collisionMask) :
            m_CollisionMask(collisionMask)
        {}

        uint32_t m_CollisionMask;

        bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
        {
            return (HK_BIT(static_cast<uint8_t>(inLayer)) & m_CollisionMask) != 0;
        }
    };
    BroadphaseLayerFilter broadphaseFilter(HK_BIT(BroadphaseLayer::MOVING) | HK_BIT(BroadphaseLayer::NON_MOVING) /* | HK_BIT(BroadphaseLayer::CHARACTER_PROXY)*/);

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;

    m_PhysicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, baseOffset, collector, broadphaseFilter);

    if (collector.HadHit())
        result.HitFraction = collector.mHit.mFraction;

    return collector.HadHit();
}

void PhysicsInterface::SetLinearVelocity(ECS::EntityHandle handle, Float3 const& velocity)
{
    ECS::EntityView entityView = m_World->GetEntityView(handle);

    if (auto dynamicBody = entityView.GetComponent<DynamicBodyComponent>())
    {
        m_PhysicsSystem.GetBodyInterface().SetLinearVelocity(dynamicBody->GetBodyId(), ConvertVector(velocity));
    }
    else if (auto character = entityView.GetComponent<CharacterControllerComponent>())
    {
        character->m_pCharacter->SetLinearVelocity(ConvertVector(velocity));
    }
}

HK_NAMESPACE_END
