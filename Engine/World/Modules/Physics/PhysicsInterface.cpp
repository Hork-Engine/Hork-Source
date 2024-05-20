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
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Geometry/OrientedBox.h>

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
    const JPH::uint cMaxBodies = 65536;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const JPH::uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    const JPH::uint cMaxBodyPairs = 65536;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    const JPH::uint cMaxContactConstraints = 10240;

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

class BroadphaseBodyCollector : public JPH::CollideShapeBodyCollector
{
public:
    BroadphaseBodyCollector(TVector<PhysBodyID>& outResult) :
        m_Hits(outResult)
    {
        m_Hits.Clear();
    }

    void AddHit(const JPH::BodyID& inBodyID) override
    {
        m_Hits.Add(inBodyID);
    }

    TVector<PhysBodyID>& m_Hits;
};

JPH::RVec3 CalcBaseOffset(JPH::Vec3 const& pos, JPH::Vec3 const& direction)
{
    // Define a base offset that is halfway the probe to test getting the collision results relative to some offset.
    // Note that this is not necessarily the best choice for a base offset, but we want something that's not zero
    // and not the start of the collision test either to ensure that we'll see errors in the algorithm.
    return pos + 0.5f * direction;
}

void CopyShapeCastResult(ShapeCastResult& out, JPH::ShapeCastResult const& in)
{
    out.BodyID = in.mBodyID2;
    out.ContactPointOn1 = ConvertVector(in.mContactPointOn1);
    out.ContactPointOn2 = ConvertVector(in.mContactPointOn2);
    out.PenetrationAxis = ConvertVector(in.mPenetrationAxis);
    out.PenetrationDepth = in.mPenetrationDepth;
    out.Fraction = in.mFraction;
    out.IsBackFaceHit = in.mIsBackFaceHit;
}

void CopyShapeCastResult(TVector<ShapeCastResult>& out, JPH::Array<JPH::ShapeCastResult> const& in)
{
    out.Resize(in.size());
    for (int i = 0; i < in.size(); i++)
        CopyShapeCastResult(out[i], in[i]);
}

void CopyShapeCollideResult(ShapeCollideResult& out, JPH::CollideShapeResult const& in)
{
    out.BodyID = in.mBodyID2;
    out.ContactPointOn1 = ConvertVector(in.mContactPointOn1);
    out.ContactPointOn2 = ConvertVector(in.mContactPointOn2);
    out.PenetrationAxis = ConvertVector(in.mPenetrationAxis);
    out.PenetrationDepth = in.mPenetrationDepth;
}

void CopyShapeCollideResult(TVector<ShapeCollideResult>& out, JPH::Array<JPH::CollideShapeResult> const& in)
{
    out.Resize(in.size());
    for (int i = 0; i < in.size(); i++)
        CopyShapeCollideResult(out[i], in[i]);
}

} // namespace

bool PhysicsInterface::CastRayClosest(Float3 const& inRayStart, Float3 const& inRayDir, RayCastResult& outResult, RayCastFilter const& inFilter)
{
    JPH::RRayCast raycast;
    raycast.mOrigin = ConvertVector(inRayStart);
    raycast.mDirection = ConvertVector(inRayDir);

    JPH::RayCastResult hit;
    if (inFilter.bIgonreBackFaces)
    {
        JPH::RayCastSettings settings;

        // How backfacing triangles should be treated
        //settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
        settings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;

        // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
        settings.mTreatConvexAsSolid = true;

        JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
        m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(raycast, settings, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

        if (!collector.HadHit())
            return false;

        hit = collector.mHit;
    }
    else
    {
        if (!m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(raycast, hit, BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())))
            return false;
    }

    outResult.BodyID = hit.mBodyID;
    outResult.Fraction = hit.mFraction;

    if (inFilter.bCalcSurfcaceNormal)
    {
        JPH::BodyLockRead lock(m_PhysicsSystem.GetBodyLockInterface(), outResult.BodyID);
        JPH::Body const& body = lock.GetBody();

        auto normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, raycast.GetPointOnRay(outResult.Fraction));
        outResult.Normal = ConvertVector(normal);
    }

    return true;
}

bool PhysicsInterface::CastRay(Float3 const& inRayStart, Float3 const& inRayDir, TVector<RayCastResult>& outResult, RayCastFilter const& inFilter)
{
    JPH::RRayCast raycast;
    raycast.mOrigin = ConvertVector(inRayStart);
    raycast.mDirection = ConvertVector(inRayDir);

    JPH::RayCastSettings settings;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
    settings.mTreatConvexAsSolid = true;

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(raycast, settings, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (inFilter.bSortByDistance)
            collector.Sort();

        outResult.Reserve(collector.mHits.size());
        for (auto& hit : collector.mHits)
        {
            RayCastResult& r = outResult.Add();
            r.BodyID = hit.mBodyID;
            r.Fraction = hit.mFraction;

            if (inFilter.bCalcSurfcaceNormal)
            {
                JPH::BodyLockRead lock(m_PhysicsSystem.GetBodyLockInterface(), hit.mBodyID);
                JPH::Body const& body = lock.GetBody();

                auto normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, raycast.GetPointOnRay(hit.mFraction));
                r.Normal = ConvertVector(normal);
            }
        }
    }

    return collector.HadHit();
}

bool PhysicsInterface::CastBoxClosest(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastBox(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastBoxMinMaxClosest(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastSphereClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastSphere(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCapsuleClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCapsule(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCylinderClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCylinder(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastShapeClosest(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = true;

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CastShape(inShapeCast, settings, inBaseOffset, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    if (collector.HadHit())
        CopyShapeCastResult(outResult, collector.mHit);

    return collector.HadHit();
}

bool PhysicsInterface::CastShape(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = false;

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CastShape(inShapeCast, settings, inBaseOffset, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (inFilter.bSortByDistance)
            collector.Sort();

        CopyShapeCastResult(outResult, collector.mHits);
    }

    return collector.HadHit();
}

void PhysicsInterface::OverlapBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    if (inRotation == Quat::Identity())
    {
        m_PhysicsSystem.GetBroadPhaseQuery().CollideAABox(JPH::AABox(ConvertVector(inPosition - inHalfExtent), ConvertVector(inPosition + inHalfExtent)),
                                                          collector,
                                                          BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));
    }
    else
    {
        JPH::OrientedBox oriented_box;
        oriented_box.mOrientation.SetTranslation(ConvertVector(inPosition));
        oriented_box.mOrientation.SetRotation(ConvertMatrix(inRotation.ToMatrix4x4()));
        oriented_box.mHalfExtents = ConvertVector(inHalfExtent);

        m_PhysicsSystem.GetBroadPhaseQuery().CollideOrientedBox(oriented_box, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));
    }
}

void PhysicsInterface::OverlapBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_PhysicsSystem.GetBroadPhaseQuery().CollideAABox(JPH::AABox(ConvertVector(inMins), ConvertVector(inMaxs)),
                                                      collector,
                                                      BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));
}

void PhysicsInterface::OverlapSphere(Float3 const& inPosition, float inRadius, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_PhysicsSystem.GetBroadPhaseQuery().CollideSphere(ConvertVector(inPosition),
                                                       inRadius,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));
}

void PhysicsInterface::OverlapPoint(Float3 const& inPosition, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_PhysicsSystem.GetBroadPhaseQuery().CollidePoint(ConvertVector(inPosition),
                                                      collector,
                                                      BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())
                                                      /* TODO: objectLayerFilter */);
}

bool PhysicsInterface::CheckBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckSphere(Float3 const& inPosition, float inRadius, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckPoint(Float3 const& inPosition, BroadphaseLayer::Mask inBroadphaseLayrs)
{
    JPH::AnyHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(inPosition), collector, BroadphaseLayerFilter(inBroadphaseLayrs.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);
    return collector.HadHit();
}

void PhysicsInterface::CollideBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.bSortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.bSortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideSphere(Float3 const& inPosition, float inRadius, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sTranslation(pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.bSortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.bSortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.bIgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_PhysicsSystem.GetNarrowPhaseQuery().CollideShape(&shape,
                                                       JPH::Vec3::sReplicate(1.0f),
                                                       JPH::RMat44::sRotationTranslation(rotation, pos),
                                                       settings,
                                                       base_offset,
                                                       collector,
                                                       BroadphaseLayerFilter(inFilter.BroadphaseLayerMask.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.bSortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollidePoint(Float3 const& inPosition, TVector<PhysBodyID>& outResult, BroadphaseLayer::Mask inBroadphaseLayrs)
{
    JPH::AllHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_PhysicsSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(inPosition), collector, BroadphaseLayerFilter(inBroadphaseLayrs.Get())
                                                       /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    outResult.Clear();
    if (collector.HadHit())
    {
        outResult.Reserve(collector.mHits.size());
        for (JPH::CollidePointResult const& hit : collector.mHits)
            outResult.Add(hit.mBodyID);
    }
}

auto PhysicsInterface::GetEntity(PhysBodyID const& inBodyID) -> ECS::EntityHandle
{
    return ECS::EntityHandle(m_PhysicsSystem.GetBodyInterface().GetUserData(inBodyID));
}

auto PhysicsInterface::GetPhysBodyID(ECS::EntityHandle inEntityHandle) -> PhysBodyID
{
    ECS::EntityView entity_view = m_World->GetEntityView(inEntityHandle);

    if (auto body = entity_view.GetComponent<RigidBodyComponent>())
    {
        return body->GetBodyId();
    }
    if (auto heightfield = entity_view.GetComponent<HeightFieldComponent>())
    {
        return heightfield->GetBodyId();
    }
    //if (auto character = entity_view.GetComponent<CharacterControllerComponent>())
    //{
    //    return character->GetBodyId();
    //}
    return {};
}

ECS::EntityHandle PhysicsInterface::CreateBody(ECS::CommandBuffer& inCB, RigidBodyDesc const& inDesc)
{
    JPH::EMotionType motion_type{JPH::EMotionType::Static};
    switch (inDesc.MotionBehavior)
    {
        case MB_STATIC:
            motion_type = JPH::EMotionType::Static;
            break;
        case MB_DYNAMIC:
            motion_type = JPH::EMotionType::Dynamic;
            break;
        case MB_KINEMATIC:
            motion_type = JPH::EMotionType::Kinematic;
            break;
    }

    if (inDesc.bIsTrigger && motion_type == JPH::EMotionType::Dynamic)
    {
        LOG("WARNING: Triggers can only have STATIC or KINEMATIC motion behavior but set to DYNAMIC.\n");
        motion_type = JPH::EMotionType::Static;
    }

    uint8_t broadphase;
    if (inDesc.bIsTrigger)
        broadphase = BroadphaseLayer::SENSOR;
    else if (motion_type == JPH::EMotionType::Static)
        broadphase = BroadphaseLayer::NON_MOVING;
    else
        broadphase = BroadphaseLayer::MOVING;    

    SceneNodeDesc nd;
    nd.Parent = inDesc.Parent;
    nd.Position = inDesc.Position;
    nd.Rotation = inDesc.Rotation;
    nd.Scale = inDesc.Scale;
    nd.NodeFlags = motion_type != JPH::EMotionType::Dynamic ? inDesc.NodeFlags : SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE;
    nd.bMovable = motion_type != JPH::EMotionType::Static;
    nd.bTransformInterpolation = inDesc.bTransformInterpolation;

    ECS::EntityHandle entity = CreateSceneNode(inCB, nd);

    CollisionModel* model = inDesc.Model;

    JPH::BodyCreationSettings settings(model->Instatiate(Float3(1)), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), motion_type, MakeObjectLayer(inDesc.CollisionGroup, broadphase));
    settings.mLinearVelocity = ConvertVector(inDesc.Dynamic.LinearVelocity);
    settings.mAngularVelocity = ConvertVector(inDesc.Dynamic.AngularVelocity);
    settings.mUserData = entity;
    settings.mIsSensor = inDesc.bIsTrigger;
    settings.mMotionQuality = inDesc.MotionQuality == MQ_DISCRETE ? JPH::EMotionQuality::Discrete : JPH::EMotionQuality::LinearCast;
    settings.mAllowSleeping = inDesc.bAllowSleeping;
    settings.mFriction = inDesc.Friction;
    settings.mRestitution = inDesc.Restitution;
    settings.mLinearDamping = inDesc.Dynamic.LinearDamping;
    settings.mAngularDamping = inDesc.Dynamic.AngularDamping;
    settings.mMaxLinearVelocity = inDesc.Dynamic.MaxLinearVelocity;
    settings.mMaxAngularVelocity = inDesc.Dynamic.MaxAngularVelocity;
    settings.mGravityFactor = inDesc.Dynamic.GravityFactor;

    if (inDesc.Dynamic.bCalculateInertia)
    {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mInertiaMultiplier = inDesc.Dynamic.InertiaMultiplier;
    }
    else
    {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        settings.mMassPropertiesOverride.mInertia = ConvertMatrix(inDesc.Dynamic.Inertia);
    }
    settings.mMassPropertiesOverride.mMass = inDesc.Dynamic.Mass;

    JPH::Body* body = m_PhysicsSystem.GetBodyInterface().CreateBody(settings);
    if (body)
    {
        {
            SpinLockGuard lock(m_PendingBodiesMutex);

            HK_ASSERT(m_PendingBodies.Count(entity) == 0);
            m_PendingBodies[entity] = body->GetID();
        }

        switch (motion_type)
        {
            case JPH::EMotionType::Static:
                inCB.AddComponent<StaticBodyComponent>(entity);
                break;
            case JPH::EMotionType::Dynamic:
                inCB.AddComponent<DynamicBodyComponent>(entity);
                break;
            case JPH::EMotionType::Kinematic:
                inCB.AddComponent<KinematicBodyComponent>(entity);
                break;
        }

        if (inDesc.bIsTrigger)
            inCB.AddComponent<TriggerComponent>(entity, inDesc.TriggerClass);

        inCB.AddComponent<RigidBodyComponent>(entity, model, body->GetID());

        if (inDesc.bAllowRigidBodyScaling)
        {
            auto& scaling = inCB.AddComponent<RigidBodyDynamicScaling>(entity);
            scaling.CachedScale = inDesc.Scale;
        }
    }
    else
    {
        LOG("Couldn't create body for the entity\n");
    }

    return entity;
}

ECS::EntityHandle PhysicsInterface::CreateHeightField(ECS::CommandBuffer& inCB, HeightFieldDesc const& inDesc)
{
    auto& body_interface = m_PhysicsSystem.GetBodyInterface();

    SceneNodeDesc nd;
    nd.Parent = inDesc.Parent;
    nd.Position = inDesc.Position;
    nd.Rotation = inDesc.Rotation;
    nd.bMovable = false;
    nd.bTransformInterpolation = false;

    ECS::EntityHandle entity = CreateSceneNode(inCB, nd);

    TerrainCollision* model = inDesc.Model;

    auto settings = JPH::BodyCreationSettings(model->Instatiate(),
                                              JPH::Vec3::sZero(),
                                              JPH::Quat::sIdentity(),
                                              JPH::EMotionType::Static,
                                              MakeObjectLayer(inDesc.CollisionGroup, BroadphaseLayer::NON_MOVING));
    settings.mUserData = entity;
    
    JPH::Body* body = body_interface.CreateBody(settings);
    if (body)
    {
        {
            SpinLockGuard lock(m_PendingBodiesMutex);

            HK_ASSERT(m_PendingBodies.Count(entity) == 0);
            m_PendingBodies[entity] = body->GetID();
        }

        inCB.AddComponent<HeightFieldComponent>(entity, inDesc.Model, body->GetID());
    }
    else
    {
        LOG("Couldn't create body for the entity\n");
    }

    return entity;
}

class CharacterControllerImpl : public JPH::CharacterVirtual, public JPH::CharacterContactListener
{
public:
    JPH_OVERRIDE_NEW_DELETE

    CharacterControllerImpl(ECS::World* inWorld, ECS::EntityHandle inEntity, const JPH::CharacterVirtualSettings* inSettings, JPH::Vec3Arg inPosition, JPH::QuatArg inRotation, JPH::PhysicsSystem* inSystem) :
        JPH::CharacterVirtual(inSettings, inPosition, inRotation, inSystem),
        m_World(inWorld),
        m_Entity(inEntity)
    {
        SetListener(this);
    }

    // Called whenever the character collides with a body. Returns true if the contact can push the character.
    void OnContactAdded(const JPH::CharacterVirtual*, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
    {
        //LOG("CharacterControllerSystem::OnContactAdded {}\n", inBodyID2.GetIndexAndSequenceNumber());

        ECS::EntityView view = m_World->GetEntityView(m_Entity);

        CharacterControllerComponent* component = view.GetComponent<CharacterControllerComponent>();
        if (component)
        {
#if 0
        // Dynamic boxes on the ramp go through all permutations
        JPH::Array<JPH::BodyID>::const_iterator i = std::find(mRampBlocks.begin(), mRampBlocks.end(), inBodyID2);
        if (i != mRampBlocks.end())
        {
            size_t index = i - mRampBlocks.begin();
            ioSettings.mCanPushCharacter = (index & 1) != 0;
            ioSettings.mCanReceiveImpulses = (index & 2) != 0;
        }
#endif
            // If we encounter an object that can push us, enable sliding
            if (ioSettings.mCanPushCharacter && mSystem->GetBodyInterface().GetMotionType(inBodyID2) != JPH::EMotionType::Static)
                component->m_bAllowSliding = true;
        }
    }

    // Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
    void OnContactSolve(const JPH::CharacterVirtual*, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
    {
        ECS::EntityView view = m_World->GetEntityView(m_Entity);

        CharacterControllerComponent* component = view.GetComponent<CharacterControllerComponent>();
        if (component)
        {
            // Don't allow the player to slide down static not-too-steep surfaces when not actively moving and when not on a moving platform
            if (!component->m_bAllowSliding && inContactVelocity.IsNearZero() && !IsSlopeTooSteep(inContactNormal))
                ioNewCharacterVelocity = JPH::Vec3::sZero();
        }
    }

private:
    ECS::World* m_World;
    ECS::EntityHandle m_Entity;
};

ECS::EntityHandle PhysicsInterface::CreateCharacterController(ECS::CommandBuffer& inCB, CharacterControllerDesc const& inDesc)
{
    auto& body_interface = m_PhysicsSystem.GetBodyInterface();

    SceneNodeDesc nd;
    nd.Position = inDesc.Position;
    nd.Rotation = inDesc.Rotation;
    nd.NodeFlags = SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE;
    nd.bMovable = true;
    nd.bTransformInterpolation = inDesc.bTransformInterpolation;

    ECS::EntityHandle entity = CreateSceneNode(inCB, nd);

    // Create capsule shapes for all stances
    //switch (inDesc.ShapeType)
    //{
    //case CHARACTER_SHAPE_CAPSULE:
    //auto standing_shape= JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * inDesc.HeightStanding + inDesc.RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * inDesc.HeightStanding, inDesc.RadiusStanding)).Create().Get();
    //auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * inDesc.HeightCrouching + inDesc.RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * inDesc.HeightCrouching, inDesc.RadiusCrouching)).Create().Get();
    //        break;

    //case CHARACTER_SHAPE_CYLINDER:
    auto standing_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * inDesc.HeightStanding + inDesc.RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * inDesc.HeightStanding + inDesc.RadiusStanding, inDesc.RadiusStanding)).Create().Get();
    auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * inDesc.HeightCrouching + inDesc.RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * inDesc.HeightCrouching + inDesc.RadiusCrouching, inDesc.RadiusCrouching)).Create().Get();
    //    break;

    //case CHARACTER_SHAPE_BOX:
    //auto standing_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * inDesc.HeightStanding + inDesc.RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(inDesc.RadiusStanding, 0.5f * inDesc.HeightStanding + inDesc.RadiusStanding, inDesc.RadiusStanding))).Create().Get();
    //auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * inDesc.HeightCrouching + inDesc.RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(inDesc.RadiusCrouching, 0.5f * inDesc.HeightCrouching + inDesc.RadiusCrouching, inDesc.RadiusCrouching))).Create().Get();
    //    break;
    //}

    auto position = ConvertVector(inDesc.Position);
    auto rotation = ConvertQuaternion(inDesc.Rotation);

    auto body_settings = JPH::BodyCreationSettings(standing_shape,
                                                   position,
                                                   rotation,
                                                   JPH::EMotionType::Kinematic,
                                                   MakeObjectLayer(inDesc.CollisionGroup, BroadphaseLayer::CHARACTER_PROXY));    
    body_settings.mUserData = entity;
    //body_settings.mCollisionGroup.SetGroupID(groupId);
    //body_settings.mCollisionGroup.SetGroupFilter(GetGroupFilter());

    JPH::Body* body = body_interface.CreateBody(body_settings);
    if (body)
    {
        {
            SpinLockGuard lock(m_PendingBodiesMutex);

            HK_ASSERT(m_PendingBodies.Count(entity) == 0);
            m_PendingBodies[entity] = body->GetID();
        }

        auto& character = inCB.AddComponent<CharacterControllerComponent>(entity, body->GetID(), inDesc.CollisionGroup);

        JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
        settings->mMaxSlopeAngle = inDesc.MaxSlopeAngle;
        settings->mMaxStrength = inDesc.MaxStrength;
        settings->mShape = standing_shape;
        settings->mCharacterPadding = inDesc.CharacterPadding;
        settings->mPenetrationRecoverySpeed = inDesc.PenetrationRecoverySpeed;
        settings->mPredictiveContactDistance = inDesc.PredictiveContactDistance;
        settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -inDesc.RadiusStanding); // Accept contacts that touch the lower sphere of the capsule

        character.m_pCharacter = new CharacterControllerImpl(m_World, entity, settings, position, rotation, &m_PhysicsSystem);
        character.m_StandingShape = standing_shape;
        character.m_CrouchingShape = crouching_shape;
    }
    else
    {
        LOG("Couldn't create body for the entity\n");
    }

    return entity;
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
    JPH::Vec3 linear_vel;
    JPH::Vec3 angular_vel;
    m_PhysicsSystem.GetBodyInterface().GetLinearAndAngularVelocity(inBodyID, linear_vel, angular_vel);
    outLinearVelocity = ConvertVector(linear_vel);
    outAngularVelocity = ConvertVector(angular_vel);
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
void PhysicsInterface::SetGravity(Float3 const inGravity)
{
    return m_PhysicsSystem.SetGravity(ConvertVector(inGravity));
}
Float3 PhysicsInterface::GetGravity() const
{
    return ConvertVector(m_PhysicsSystem.GetGravity());
}

HK_NAMESPACE_END
