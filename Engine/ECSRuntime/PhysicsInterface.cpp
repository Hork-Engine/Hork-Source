#include "PhysicsInterface.h"

#include "Components/RigidBodyComponent.h"
#include "Components/CharacterControllerComponent.h"

#include <Engine/Core/Logger.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

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

auto PhysicsInterface::GetEntity(PhysBodyID const& bodyID) -> ECS::EntityHandle
{
    return ECS::EntityHandle(m_PhysicsSystem.GetBodyInterface().GetUserData(bodyID));
}

auto PhysicsInterface::GetPhysBodyID(ECS::EntityHandle entityHandle) -> PhysBodyID
{
    ECS::EntityView entityView = m_World->GetEntityView(entityHandle);

    if (auto body = entityView.GetComponent<PhysBodyComponent>())
    {
        return body->GetBodyId();
    }
    //if (auto character = entityView.GetComponent<CharacterControllerComponent>())
    //{
    //    return character->GetBodyId();
    //}
    return {};
}

ECS::EntityHandle PhysicsInterface::CreateBody(ECS::CommandBuffer& commandBuffer, RigidBodyDesc const& desc)
{
    JPH::EMotionType motionType{JPH::EMotionType::Static};
    switch (desc.MotionBehavior)
    {
        case MB_STATIC:
            motionType = JPH::EMotionType::Static;
            break;
        case MB_DYNAMIC:
            motionType = JPH::EMotionType::Dynamic;
            break;
        case MB_KINEMATIC:
            motionType = JPH::EMotionType::Kinematic;
            break;
    }

    if (desc.bIsTrigger && motionType == JPH::EMotionType::Dynamic)
    {
        LOG("WARNING: Triggers can only have STATIC or KINEMATIC motion behavior but set to DYNAMIC.\n");
        motionType = JPH::EMotionType::Static;
    }

    JPH::uint8 broadphase;
    if (desc.bIsTrigger)
        broadphase = BroadphaseLayer::SENSOR;
    else if (motionType == JPH::EMotionType::Static)
        broadphase = BroadphaseLayer::NON_MOVING;
    else
        broadphase = BroadphaseLayer::MOVING;    

    SceneNodeDesc nodeDesc;

    nodeDesc.Parent = desc.Parent;
    nodeDesc.Position = desc.Position;
    nodeDesc.Rotation = desc.Rotation;
    nodeDesc.Scale = desc.Scale;
    nodeDesc.NodeFlags = motionType != JPH::EMotionType::Dynamic ? desc.NodeFlags : SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE;
    nodeDesc.bMovable = motionType != JPH::EMotionType::Static;
    nodeDesc.bTransformInterpolation = desc.bTransformInterpolation;

    ECS::EntityHandle entityHandle = CreateSceneNode(commandBuffer, nodeDesc);

    CollisionModel* collisionModel = desc.Model;

    JPH::BodyCreationSettings settings(collisionModel->Instatiate(Float3(1)), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), motionType, MakeObjectLayer(desc.CollisionGroup, broadphase));

    settings.mLinearVelocity = ConvertVector(desc.Dynamic.LinearVelocity);
    settings.mAngularVelocity = ConvertVector(desc.Dynamic.AngularVelocity);
    settings.mUserData = entityHandle;
    settings.mIsSensor = desc.bIsTrigger;
    settings.mMotionQuality = desc.MotionQuality == MQ_DISCRETE ? JPH::EMotionQuality::Discrete : JPH::EMotionQuality::LinearCast;
    settings.mAllowSleeping = desc.bAllowSleeping;
    settings.mFriction = desc.Friction;
    settings.mRestitution = desc.Restitution;
    settings.mLinearDamping = desc.Dynamic.LinearDamping;
    settings.mAngularDamping = desc.Dynamic.AngularDamping;
    settings.mMaxLinearVelocity = desc.Dynamic.MaxLinearVelocity;
    settings.mMaxAngularVelocity = desc.Dynamic.MaxAngularVelocity;
    settings.mGravityFactor = desc.Dynamic.GravityFactor;

    if (desc.Dynamic.bCalculateInertia)
    {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mInertiaMultiplier = desc.Dynamic.InertiaMultiplier;
    }
    else
    {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        settings.mMassPropertiesOverride.mInertia = ConvertMatrix(desc.Dynamic.Inertia);
    }
    settings.mMassPropertiesOverride.mMass = desc.Dynamic.Mass;

    JPH::Body* body = m_PhysicsSystem.GetBodyInterface().CreateBody(settings);
    if (body)
    {
        {
            SpinLockGuard lock(m_PendingBodiesMutex);

            HK_ASSERT(m_PendingBodies.Count(entityHandle) == 0);
            m_PendingBodies[entityHandle] = body->GetID();
        }

        switch (motionType)
        {
            case JPH::EMotionType::Static:
                commandBuffer.AddComponent<StaticBodyComponent>(entityHandle);
                break;
            case JPH::EMotionType::Dynamic:
                commandBuffer.AddComponent<DynamicBodyComponent>(entityHandle);
                break;
            case JPH::EMotionType::Kinematic:
                commandBuffer.AddComponent<KinematicBodyComponent>(entityHandle);
                break;
        }

        if (desc.bIsTrigger)
            commandBuffer.AddComponent<TriggerComponent>(entityHandle, desc.TriggerClass);

        commandBuffer.AddComponent<PhysBodyComponent>(entityHandle, desc.Model, body->GetID());

        if (desc.bAllowRigidBodyScaling)
        {
            auto& scaling = commandBuffer.AddComponent<RigidBodyDynamicScaling>(entityHandle);
            scaling.CachedScale = desc.Scale;
        }
    }
    else
    {
        LOG("Couldn't create rigid body for the entity\n");
    }

    return entityHandle;
}

ECS::EntityHandle PhysicsInterface::CreateCharacterController(ECS::CommandBuffer& commandBuffer, CharacterControllerDesc const& desc)
{
    SceneNodeDesc nodeDesc;

    nodeDesc.Position = desc.Position;
    nodeDesc.Rotation = desc.Rotation;
    nodeDesc.NodeFlags = SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE;
    nodeDesc.bMovable = true;
    nodeDesc.bTransformInterpolation = desc.bTransformInterpolation;

    ECS::EntityHandle handle = CreateSceneNode(commandBuffer, nodeDesc);

    commandBuffer.AddComponent<CharacterControllerComponent>(handle);

    return handle;
}

void PhysicsInterface::ActivateBody(PhysBodyID const& inBodyID)
{
    m_PhysicsSystem.GetBodyInterface().ActivateBody(inBodyID);
}
void PhysicsInterface::ActivateBodies(TArrayView<PhysBodyID> inBodyIDs)
{
    m_PhysicsSystem.GetBodyInterface().ActivateBodies(inBodyIDs.ToPtr(), inBodyIDs.Size());
}
void PhysicsInterface::DeactivateBody(PhysBodyID const& inBodyID)
{
    m_PhysicsSystem.GetBodyInterface().DeactivateBody(inBodyID);
}
void PhysicsInterface::DeactivateBodies(TArrayView<PhysBodyID> inBodyIDs)
{
    m_PhysicsSystem.GetBodyInterface().DeactivateBodies(inBodyIDs.ToPtr(), inBodyIDs.Size());
}
bool PhysicsInterface::IsActive(PhysBodyID const& inBodyID) const
{
    return m_PhysicsSystem.GetBodyInterface().IsActive(inBodyID);
}
//void PhysicsInterface::SetPositionAndRotation(PhysBodyID const& inBodyID, Float3 const& inPosition, Quat const& inRotation, JPH::EActivation inActivationMode)
//{
//    m_PhysicsSystem.GetBodyInterface().SetPositionAndRotation(inBodyID, ConvertVector(inPosition), ConvertQuaternion(inRotation), inActivationMode);
//}
//void PhysicsInterface::SetPositionAndRotationWhenChanged(PhysBodyID const& inBodyID, Float3 const& inPosition, Quat const& inRotation, JPH::EActivation inActivationMode)
//{
//    m_PhysicsSystem.GetBodyInterface().SetPositionAndRotationWhenChanged(inBodyID, ConvertVector(inPosition), ConvertQuaternion(inRotation), inActivationMode);
//}
//void PhysicsInterface::GetPositionAndRotation(PhysBodyID const& inBodyID, Float3& outPosition, Quat& outRotation) const
//{
//    JPH::RVec3 position;
//    JPH::Quat rotation;
//    m_PhysicsSystem.GetBodyInterface().GetPositionAndRotation(inBodyID, position, rotation);
//    outPosition = ConvertVector(position);
//    outRotation = ConvertQuaternion(rotation);
//}
//void PhysicsInterface::SetPosition(PhysBodyID const& inBodyID, Float3 const& inPosition, JPH::EActivation inActivationMode)
//{
//    m_PhysicsSystem.GetBodyInterface().SetPosition(inBodyID, ConvertVector(inPosition), inActivationMode);
//}
//auto PhysicsInterface::GetPosition(PhysBodyID const& inBodyID) const -> Float3
//{
//    return ConvertVector(m_PhysicsSystem.GetBodyInterface().GetPosition(inBodyID));
//}
auto PhysicsInterface::GetCenterOfMassPosition(PhysBodyID const& inBodyID) const -> Float3
{
    return ConvertVector(m_PhysicsSystem.GetBodyInterface().GetCenterOfMassPosition(inBodyID));
}
//void PhysicsInterface::SetRotation(PhysBodyID const& inBodyID, Quat const& inRotation, JPH::EActivation inActivationMode)
//{
//    m_PhysicsSystem.GetBodyInterface().SetRotation(inBodyID, ConvertQuaternion(inRotation), inActivationMode);
//}
//auto PhysicsInterface::GetRotation(PhysBodyID const& inBodyID) const -> Quat
//{
//    return ConvertQuaternion(m_PhysicsSystem.GetBodyInterface().GetRotation(inBodyID));
//}
//auto PhysicsInterface::GetWorldTransform(PhysBodyID const& inBodyID) const -> Float4x4
//{
//    return ConvertMatrix(m_PhysicsSystem.GetBodyInterface().GetWorldTransform(inBodyID));
//}
auto PhysicsInterface::GetCenterOfMassTransform(PhysBodyID const& inBodyID) const -> Float4x4
{
    return ConvertMatrix(m_PhysicsSystem.GetBodyInterface().GetCenterOfMassTransform(inBodyID));
}
//void PhysicsInterface::MoveKinematic(PhysBodyID const& inBodyID, Float3 const& inTargetPosition, Quat const& inTargetRotation, float inDeltaTime)
//{
//    m_PhysicsSystem.GetBodyInterface().MoveKinematic(inBodyID, ConvertVector(inTargetPosition), ConvertQuaternion(inTargetRotation), inDeltaTime);
//}
void PhysicsInterface::SetLinearAndAngularVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity, Float3 const& inAngularVelocity)
{
    m_PhysicsSystem.GetBodyInterface().SetLinearAndAngularVelocity(inBodyID, ConvertVector(inLinearVelocity), ConvertVector(inAngularVelocity));
}
void PhysicsInterface::GetLinearAndAngularVelocity(PhysBodyID const& inBodyID, Float3& outLinearVelocity, Float3& outAngularVelocity) const
{
    JPH::Vec3 linearVel;
    JPH::Vec3 angularVel;
    m_PhysicsSystem.GetBodyInterface().GetLinearAndAngularVelocity(inBodyID, linearVel, angularVel);
    outLinearVelocity = ConvertVector(linearVel);
    outAngularVelocity = ConvertVector(angularVel);
}
void PhysicsInterface::SetLinearVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity)
{
    m_PhysicsSystem.GetBodyInterface().SetLinearVelocity(inBodyID, ConvertVector(inLinearVelocity));
}
auto PhysicsInterface::GetLinearVelocity(PhysBodyID const& inBodyID) const -> Float3
{
    return ConvertVector(m_PhysicsSystem.GetBodyInterface().GetLinearVelocity(inBodyID));
}
void PhysicsInterface::AddLinearVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity)
{
    m_PhysicsSystem.GetBodyInterface().AddLinearVelocity(inBodyID, ConvertVector(inLinearVelocity));
}
void PhysicsInterface::AddLinearAndAngularVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity, Float3 const& inAngularVelocity)
{
    m_PhysicsSystem.GetBodyInterface().AddLinearAndAngularVelocity(inBodyID, ConvertVector(inLinearVelocity), ConvertVector(inAngularVelocity));
}
void PhysicsInterface::SetAngularVelocity(PhysBodyID const& inBodyID, Float3 const& inAngularVelocity)
{
    m_PhysicsSystem.GetBodyInterface().SetAngularVelocity(inBodyID, ConvertVector(inAngularVelocity));
}
auto PhysicsInterface::GetAngularVelocity(PhysBodyID const& inBodyID) const -> Float3
{
    return ConvertVector(m_PhysicsSystem.GetBodyInterface().GetAngularVelocity(inBodyID));
}
auto PhysicsInterface::GetPointVelocity(PhysBodyID const& inBodyID, Float3 const& inPoint) const -> Float3
{
    return ConvertVector(m_PhysicsSystem.GetBodyInterface().GetPointVelocity(inBodyID, ConvertVector(inPoint)));
}
//void PhysicsInterface::SetPositionRotationAndVelocity(PhysBodyID const& inBodyID, Float3 const& inPosition, Quat const& inRotation, Float3 const& inLinearVelocity, Float3 const& inAngularVelocity)
//{
//    m_PhysicsSystem.GetBodyInterface().SetPositionRotationAndVelocity(inBodyID, ConvertVector(inPosition), ConvertQuaternion(inRotation), ConvertVector(inLinearVelocity), ConvertVector(inAngularVelocity));
//}
void PhysicsInterface::AddForce(PhysBodyID const& inBodyID, Float3 const& inForce)
{
    m_PhysicsSystem.GetBodyInterface().AddForce(inBodyID, ConvertVector(inForce));
}
void PhysicsInterface::AddForce(PhysBodyID const& inBodyID, Float3 const& inForce, Float3 const& inPoint)
{
    m_PhysicsSystem.GetBodyInterface().AddForce(inBodyID, ConvertVector(inForce), ConvertVector(inPoint));
}
void PhysicsInterface::AddTorque(PhysBodyID const& inBodyID, Float3 const& inTorque)
{
    m_PhysicsSystem.GetBodyInterface().AddTorque(inBodyID, ConvertVector(inTorque));
}
void PhysicsInterface::AddForceAndTorque(PhysBodyID const& inBodyID, Float3 const& inForce, Float3 const& inTorque)
{
    m_PhysicsSystem.GetBodyInterface().AddForceAndTorque(inBodyID, ConvertVector(inForce), ConvertVector(inTorque));
}
void PhysicsInterface::AddImpulse(PhysBodyID const& inBodyID, Float3 const& inImpulse)
{
    m_PhysicsSystem.GetBodyInterface().AddImpulse(inBodyID, ConvertVector(inImpulse));
}
void PhysicsInterface::AddImpulse(PhysBodyID const& inBodyID, Float3 const& inImpulse, Float3 const& inPoint)
{
    m_PhysicsSystem.GetBodyInterface().AddImpulse(inBodyID, ConvertVector(inImpulse), ConvertVector(inPoint));
}
void PhysicsInterface::AddAngularImpulse(PhysBodyID const& inBodyID, Float3 const& inAngularImpulse)
{
    m_PhysicsSystem.GetBodyInterface().AddAngularImpulse(inBodyID, ConvertVector(inAngularImpulse));
}
auto PhysicsInterface::GetMotionBehavior(PhysBodyID const& inBodyID) const -> MOTION_BEHAVIOR
{
    switch (m_PhysicsSystem.GetBodyInterface().GetMotionType(inBodyID))
    {
        case JPH::EMotionType::Static:
            return MB_STATIC;
        case JPH::EMotionType::Kinematic:
            return MB_KINEMATIC;
        case JPH::EMotionType::Dynamic:
        default:
            return MB_DYNAMIC;
    }
}
void PhysicsInterface::SetMotionQuality(PhysBodyID const& inBodyID, MOTION_QUALITY inMotionQuality)
{
    m_PhysicsSystem.GetBodyInterface().SetMotionQuality(inBodyID, JPH::EMotionQuality(inMotionQuality));
}
auto PhysicsInterface::GetMotionQuality(PhysBodyID const& inBodyID) const -> MOTION_QUALITY
{
    return MOTION_QUALITY(m_PhysicsSystem.GetBodyInterface().GetMotionQuality(inBodyID));
}
auto PhysicsInterface::GetInverseInertia(PhysBodyID const& inBodyID) const -> Float4x4
{
    return ConvertMatrix(m_PhysicsSystem.GetBodyInterface().GetInverseInertia(inBodyID));
}
void PhysicsInterface::SetRestitution(PhysBodyID const& inBodyID, float inRestitution)
{
    m_PhysicsSystem.GetBodyInterface().SetRestitution(inBodyID, inRestitution);
}
auto PhysicsInterface::GetRestitution(PhysBodyID const& inBodyID) const -> float
{
    return m_PhysicsSystem.GetBodyInterface().GetRestitution(inBodyID);
}
void PhysicsInterface::SetFriction(PhysBodyID const& inBodyID, float inFriction)
{
    m_PhysicsSystem.GetBodyInterface().SetFriction(inBodyID, inFriction);
}
auto PhysicsInterface::GetFriction(PhysBodyID const& inBodyID) const -> float
{
    return m_PhysicsSystem.GetBodyInterface().GetFriction(inBodyID);
}
void PhysicsInterface::SetGravityFactor(PhysBodyID const& inBodyID, float inGravityFactor)
{
    m_PhysicsSystem.GetBodyInterface().SetGravityFactor(inBodyID, inGravityFactor);
}
auto PhysicsInterface::GetGravityFactor(PhysBodyID const& inBodyID) const -> float
{
    return m_PhysicsSystem.GetBodyInterface().GetGravityFactor(inBodyID);
}

HK_NAMESPACE_END
