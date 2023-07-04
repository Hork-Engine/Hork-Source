#include "PhysicsInterface.h"

#include "Components/RigidBodyComponent.h"
#include "Components/CharacterControllerComponent.h"

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>

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

namespace
{

JPH::RVec3 CalcBaseOffset(JPH::Vec3 const& pos, JPH::Vec3 const& direction)
{
    // Define a base offset that is halfway the probe to test getting the collision results relative to some offset.
    // Note that this is not necessarily the best choice for a base offset, but we want something that's not zero
    // and not the start of the collision test either to ensure that we'll see errors in the algorithm.
    return pos + 0.5f * direction;
}

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

ShapeCastFilter ShapeCastFilterDefault;

} // namespace

bool PhysicsInterface::CastRay(Float3 const& start, Float3 const& dir, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;
    
    JPH::RRayCast rayCast;
    rayCast.mOrigin = ConvertVector(start);
    rayCast.mDirection = ConvertVector(dir);

    JPH::RayCastResult hit;

    bool bHadHit = m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(rayCast, hit, BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    if (bHadHit)
        result.HitFraction = hit.mFraction;

    return bHadHit;
}

bool PhysicsInterface::CastRayAll(Float3 const& start, Float3 const& dir, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::RRayCast rayCast;
    rayCast.mOrigin = ConvertVector(start);
    rayCast.mDirection = ConvertVector(dir);

    JPH::RayCastSettings settings;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
    settings.mTreatConvexAsSolid = true;

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(rayCast, settings, collector, BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto& hit : collector.mHits)
        {
            ShapeCastResult& r = result.Add();
            r.HitFraction = hit.mFraction;
        }
    }

    return collector.HadHit();
}

bool PhysicsInterface::CastBox(Float3 const& start, Float3 const& dir, Float3 const& halfExtent, Quat const& boxRotation, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    JPH::BoxShape boxShape(ConvertVector(halfExtent));

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);
    JPH::Quat rotation = ConvertQuaternion(boxRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShape(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastBoxAll(Float3 const& start, Float3 const& dir, Float3 const& halfExtent, Quat const& boxRotation, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    JPH::BoxShape boxShape(ConvertVector(halfExtent));

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);
    JPH::Quat rotation = ConvertQuaternion(boxRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShapeAll(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastBoxMinMax(Float3 const& mins, Float3 const& maxs, Float3 const& dir, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    JPH::BoxShape boxShape(ConvertVector((maxs - mins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((mins + maxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(dir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return CastShape(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastBoxMinMaxAll(Float3 const& mins, Float3 const& maxs, Float3 const& dir, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    JPH::BoxShape boxShape(ConvertVector((maxs - mins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((mins + maxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(dir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return CastShapeAll(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastSphere(Float3 const& start, Float3 const& dir, float sphereRadius, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    JPH::SphereShape sphereShape(sphereRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return CastShape(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastSphereAll(Float3 const& start, Float3 const& dir, float sphereRadius, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    JPH::SphereShape sphereShape(sphereRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return CastShapeAll(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastCapsule(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    JPH::CapsuleShape capsuleShape(halfHeightOfCylinder, capsuleRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);
    JPH::Quat rotation = ConvertQuaternion(capsuleRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&capsuleShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShape(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastCapsuleAll(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    JPH::CapsuleShape capsuleShape(halfHeightOfCylinder, capsuleRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);
    JPH::Quat rotation = ConvertQuaternion(capsuleRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&capsuleShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShapeAll(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastCylinder(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    JPH::CylinderShape cylinderShape(halfHeightOfCylinder, cylinderRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);
    JPH::Quat rotation = ConvertQuaternion(cylinderRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&cylinderShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShape(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastCylinderAll(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    JPH::CylinderShape cylinderShape(halfHeightOfCylinder, cylinderRadius);

    JPH::Vec3 pos = ConvertVector(start);
    JPH::Vec3 direction = ConvertVector(dir);
    JPH::Quat rotation = ConvertQuaternion(cylinderRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&cylinderShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShapeAll(shapeCast, CalcBaseOffset(pos, direction), result, filter);
}

bool PhysicsInterface::CastShape(JPH::RShapeCast const& shapeCast, JPH::RVec3Arg baseOffset, ShapeCastResult& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = true;

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, baseOffset, collector, BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    if (collector.HadHit())
        result.HitFraction = collector.mHit.mFraction;

    return collector.HadHit();
}

bool PhysicsInterface::CastShapeAll(JPH::RShapeCast const& shapeCast, JPH::RVec3Arg baseOffset, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = false;

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, baseOffset, collector, BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto& hit : collector.mHits)
        {
            ShapeCastResult& r = result.Add();
            r.HitFraction = hit.mFraction;
        }
    }

    return collector.HadHit();
}

bool PhysicsInterface::CollidePoint(Float3 const& point, BroadphaseLayer::Mask broadphaseLayrs)
{
    JPH::AnyHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(point), collector, BroadphaseLayerFilter(broadphaseLayrs.Get()));
    return collector.HadHit();
}

bool PhysicsInterface::CollidePointAll(Float3 const& point, TVector<JPH::BodyID>& bodies, BroadphaseLayer::Mask broadphaseLayrs)
{
    JPH::AllHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(point), collector, BroadphaseLayerFilter(broadphaseLayrs.Get()));

    bodies.Clear();
    if (collector.HadHit())
    {
        bodies.Reserve(collector.mHits.size());
        for (JPH::CollidePointResult const& hit : collector.mHits)
        {
            auto& b = bodies.Add();
            b = hit.mBodyID;
        }
        return true;
    }
    return false;
}

bool PhysicsInterface::CheckBox(Float3 const& position, Float3 const& halfExtent, Quat const& boxRotation, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::BoxShape boxShape(ConvertVector(halfExtent));

    JPH::Vec3 pos = ConvertVector(position);
    JPH::Quat rotation = ConvertQuaternion(boxRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&boxShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    return collector.HadHit();
}

bool PhysicsInterface::CheckBoxMinMax(Float3 const& mins, Float3 const& maxs, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::BoxShape boxShape(ConvertVector((maxs - mins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((mins + maxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&boxShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    return collector.HadHit();
}

bool PhysicsInterface::CheckSphere(Float3 const& position, float sphereRadius, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::SphereShape sphereShape(sphereRadius);

    JPH::Vec3 pos = ConvertVector(position);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&sphereShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    return collector.HadHit();
}

bool PhysicsInterface::CheckCapsule(Float3 const& position, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::CapsuleShape capsuleShape(halfHeightOfCylinder, capsuleRadius);

    JPH::Vec3 pos = ConvertVector(position);
    JPH::Quat rotation = ConvertQuaternion(capsuleRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&capsuleShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    return collector.HadHit();
}

bool PhysicsInterface::CheckCylinder(Float3 const& position, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::CylinderShape cylinderShape(halfHeightOfCylinder, cylinderRadius);

    JPH::Vec3 pos = ConvertVector(position);
    JPH::Quat rotation = ConvertQuaternion(cylinderRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&cylinderShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    return collector.HadHit();
}

void PhysicsInterface::OverlapBox(Float3 const& position, Float3 const& halfExtent, Quat const& boxRotation, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::BoxShape boxShape(ConvertVector(halfExtent));

    JPH::Vec3 pos = ConvertVector(position);
    JPH::Quat rotation = ConvertQuaternion(boxRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&boxShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto const& hit : collector.mHits)
        {
            result.Add(hit.mBodyID2);
        }
    }
}

void PhysicsInterface::OverlapBoxMinMax(Float3 const& mins, Float3 const& maxs, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::BoxShape boxShape(ConvertVector((maxs - mins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((mins + maxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&boxShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto const& hit : collector.mHits)
        {
            result.Add(hit.mBodyID2);
        }
    }
}

void PhysicsInterface::OverlapSphere(Float3 const& position, float sphereRadius, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::SphereShape sphereShape(sphereRadius);

    JPH::Vec3 pos = ConvertVector(position);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&sphereShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto const& hit : collector.mHits)
        {
            result.Add(hit.mBodyID2);
        }
    }
}

void PhysicsInterface::OverlapCapsule(Float3 const& position, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::CapsuleShape capsuleShape(halfHeightOfCylinder, capsuleRadius);

    JPH::Vec3 pos = ConvertVector(position);
    JPH::Quat rotation = ConvertQuaternion(capsuleRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&capsuleShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto const& hit : collector.mHits)
        {
            result.Add(hit.mBodyID2);
        }
    }
}

void PhysicsInterface::OverlapCylinder(Float3 const& position, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter)
{
    if (!filter)
        filter = &ShapeCastFilterDefault;

    JPH::CylinderShape cylinderShape(halfHeightOfCylinder, cylinderRadius);

    JPH::Vec3 pos = ConvertVector(position);
    JPH::Quat rotation = ConvertQuaternion(cylinderRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = filter->bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // inBaseOffset All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&cylinderShape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       baseOffset,
                                                       collector,
                                                       BroadphaseLayerFilter(filter->BroadphaseLayerMask.Get()));

    result.Clear();
    if (collector.HadHit())
    {
        if (filter->bSortByDistance)
            collector.Sort();

        result.Reserve(collector.mHits.size());
        for (auto const& hit : collector.mHits)
        {
            result.Add(hit.mBodyID2);
        }
    }
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
