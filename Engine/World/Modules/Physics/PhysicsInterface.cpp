/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "PhysicsInterfaceImpl.h"
#include "PhysicsModule.h"
#include "Components/StaticBodyComponent.h"
#include "Components/DynamicBodyComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/HeightFieldComponent.h"
#include "Components/TriggerComponent.h"
#include "Components/WaterVolumeComponent.h"

#include <Engine/World/DebugRenderer.h>

#include <Engine/Core/Logger.h>
#include <Engine/Core/ConsoleVar.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Geometry/OrientedBox.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/EstimateCollisionResponse.h>
#include <Jolt/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h>
#include <Jolt/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCollisionModel("com_DrawCollisionModel"_s, "0"_s, CVAR_CHEAT);
ConsoleVar com_DrawCollisionShape("com_DrawCollisionShape"_s, "0"_s, CVAR_CHEAT);
ConsoleVar com_DrawTriggers("com_DrawTriggers"_s, "0"_s, CVAR_CHEAT);
ConsoleVar com_DrawCenterOfMass("com_DrawCenterOfMass"_s, "0"_s, CVAR_CHEAT);
ConsoleVar com_DrawWaterVolume("com_DrawWaterVolume"_s, "0"_s, CVAR_CHEAT);
ConsoleVar com_DrawCharacterController("com_DrawCharacterController"_s, "0"_s);

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
    BroadphaseBodyCollector(Vector<PhysBodyID>& outResult) :
        m_Hits(outResult)
    {
        m_Hits.Clear();
    }

    void AddHit(const JPH::BodyID& inBodyID) override
    {
        m_Hits.Add(PhysBodyID(inBodyID.GetIndexAndSequenceNumber()));
    }

    Vector<PhysBodyID>& m_Hits;
};

class GroupFilter : public JPH::GroupFilter
{
public:
    virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
    {
        const uint64_t id = static_cast<uint64_t>(group1.GetGroupID()) << 32 | group2.GetGroupID();
        return !m_IgnoreCollisions.Contains(id);
    }

    HashSet<uint64_t> m_IgnoreCollisions;
};

class CastObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
    CastObjectLayerFilter(uint32_t collisionMask) :
        m_CollisionMask(collisionMask)
    {}

    uint32_t m_CollisionMask;

    bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return (HK_BIT(static_cast<uint8_t>(inLayer)) & m_CollisionMask) != 0;
    }
};

PhysicsInterfaceImpl::PhysicsInterfaceImpl() :
    m_ObjectVsObjectLayerFilter(m_CollisionFilter)
{}

BodyUserData* PhysicsInterfaceImpl::CreateUserData()
{
    return new (m_UserDataAllocator.Allocate()) BodyUserData;
}

void PhysicsInterfaceImpl::DeleteUserData(BodyUserData* userData)
{
    if (userData)
    {
        userData->~BodyUserData();
        m_UserDataAllocator.Deallocate(userData);
    }
}

void PhysicsInterfaceImpl::QueueToAdd(JPH::Body* body, bool startAsSleeping)
{
    if (!startAsSleeping)
        m_QueueToAdd[0].Add(body->GetID());
    else
        m_QueueToAdd[1].Add(body->GetID());
}

bool PhysicsInterfaceImpl::CreateCollision(CreateCollisionSettings const& settings, JPH::Shape*& outShape,  ScalingMode& outScalingMode)
{
    if (!settings.Object)
        return false;

    m_TempShapes.Clear();
    m_TempShapeTransform.Clear();

    outScalingMode = ScalingMode::NonUniform;

    for (auto component : settings.Object->GetComponents())
    {
        if (auto* collider = Component::Upcast<SphereCollider>(component))
        {
            // add sphere
            m_TempShapes.Add(new JPH::SphereShape(collider->Radius));
            m_TempShapeTransform.EmplaceBack(ConvertVector(collider->OffsetPosition), JPH::Quat::sIdentity());
            outScalingMode = ScalingMode::Uniform;
            continue;
        }

        if (auto* collider = Component::Upcast<BoxCollider>(component))
        {
            // add box
            m_TempShapes.Add(new JPH::BoxShape(ConvertVector(collider->HalfExtents)));
            m_TempShapeTransform.EmplaceBack(ConvertVector(collider->OffsetPosition), ConvertQuaternion(collider->OffsetRotation));            
            continue;
        }

        if (auto* collider = Component::Upcast<CylinderCollider>(component))
        {
            // add cylinder
            m_TempShapes.Add(new JPH::CylinderShape(collider->Height * 0.5f, collider->Radius));
            m_TempShapeTransform.EmplaceBack(ConvertVector(collider->OffsetPosition), ConvertQuaternion(collider->OffsetRotation));

            if (outScalingMode != ScalingMode::Uniform)
                outScalingMode = collider->OffsetRotation != Quat::Identity() ? ScalingMode::Uniform : ScalingMode::UniformXZ;
            continue;
        }

        if (auto* collider = Component::Upcast<CapsuleCollider>(component))
        {
            // add capsule
            m_TempShapes.Add(new JPH::CapsuleShape(collider->Height * 0.5f, collider->Radius));
            m_TempShapeTransform.EmplaceBack(ConvertVector(collider->OffsetPosition), ConvertQuaternion(collider->OffsetRotation));
            outScalingMode = ScalingMode::Uniform;
            continue;
        }

        if (auto* collider = Component::Upcast<MeshCollider>(component))
        {
            // add mesh
            MeshCollisionData* data = collider->Data;
            if (data)
            {
                if (data->IsConvex() || !settings.ConvexOnly)
                {
                    m_TempShapes.Add(data->m_Data->m_Shape);
                    m_TempShapeTransform.EmplaceBack(ConvertVector(collider->OffsetPosition), ConvertQuaternion(collider->OffsetRotation));
                }
            }
            continue;
        }
    }

    if (m_TempShapes.IsEmpty())
        return false;

    if (m_TempShapes.Size() > 1)
    {
        m_TempCompoundShapeSettings.mSubShapes.reserve(m_TempShapes.Size());
        for (int i = 0, count = m_TempShapes.Size(); i < count; ++i)
            m_TempCompoundShapeSettings.AddShape(m_TempShapeTransform[i].Position, m_TempShapeTransform[i].Rotation.Normalized(), m_TempShapes[i]);

        JPH::ShapeSettings::ShapeResult result;
        outShape = new JPH::StaticCompoundShape(m_TempCompoundShapeSettings, *PhysicsModule::Get().GetTempAllocator(), result);
        outShape->AddRef();

        m_TempCompoundShapeSettings.mSubShapes.clear();
    }
    else if (m_TempShapeTransform[0].Position.LengthSq() > 0.001f || m_TempShapeTransform[0].Rotation != JPH::Quat::sIdentity())
    {
        outShape = new JPH::RotatedTranslatedShape(m_TempShapeTransform[0].Position, m_TempShapeTransform[0].Rotation.Normalized(), m_TempShapes[0]);
        outShape->AddRef();
    }
    else
    {
        outShape = m_TempShapes[0];
        outShape->AddRef();
    }

    if (settings.CenterOfMassOffset != Float3(0.0f))
    {
        // TODO: Use OffsetCenterOfMassShape
    }

    return true;
}

JPH::Shape* PhysicsInterfaceImpl::CreateScaledShape(ScalingMode scalingMode, JPH::Shape* sourceShape, Float3 const& scale)
{
    if (!sourceShape)
        return nullptr;

    if (scale.X != 1 || scale.Y != 1 || scale.Z != 1)
    {
        bool isUniformXZ = scale.X == scale.Z;
        bool isUniformScaling = isUniformXZ && scale.X == scale.Y; // FIXME: allow some epsilon?

        if (scalingMode == ScalingMode::NonUniform || isUniformScaling)
            return new JPH::ScaledShape(sourceShape, ConvertVector(scale));

        if (scalingMode == ScalingMode::UniformXZ)
        {
            if (!isUniformXZ)
                LOG("WARNING: Non-uniform XZ scaling is not allowed for this collision model\n");

            float scaleXZ = Math::Max(scale.X, scale.Z);
            return new JPH::ScaledShape(sourceShape, JPH::Vec3(scaleXZ, scale.Y, scaleXZ));
        }

        LOG("WARNING: Non-uniform scaling is not allowed for this collision model\n");
        return new JPH::ScaledShape(sourceShape, JPH::Vec3::sReplicate(Math::Max3(scale.X, scale.Y, scale.Z)));
    }
    return sourceShape;
}

namespace
{

    void GatherGeometry(JPH::SphereShape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        float sinTheta, cosTheta, sinPhi, cosPhi;

        float radius = shape->GetRadius();

        float detail = Math::Floor(Math::Max(1.0f, radius) + 0.5f);

        int numStacks = 8 * detail;
        int numSlices = 12 * detail;

        int vertexCount = (numStacks + 1) * numSlices;
        int indexCount = numStacks * numSlices * 6;

        int firstVertex = vertices.Size();
        int firstIndex = indices.Size();

        vertices.Resize(firstVertex + vertexCount);
        indices.Resize(firstIndex + indexCount);

        Float3* pVertices = vertices.ToPtr() + firstVertex;
        unsigned int* pIndices = indices.ToPtr() + firstIndex;

        for (int stack = 0; stack <= numStacks; ++stack)
        {
            float theta = stack * Math::_PI / numStacks;
            Math::SinCos(theta, sinTheta, cosTheta);

            for (int slice = 0; slice < numSlices; ++slice)
            {
                float phi = slice * Math::_2PI / numSlices;
                Math::SinCos(phi, sinPhi, cosPhi);

                *pVertices++ = Float3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta) * radius;
            }
        }

        for (int stack = 0; stack < numStacks; ++stack)
        {
            int stackOffset = firstVertex + stack * numSlices;
            int nextStackOffset = firstVertex + (stack + 1) * numSlices;

            for (int slice = 0; slice < numSlices; ++slice)
            {
                int nextSlice = (slice + 1) % numSlices;
                *pIndices++ = stackOffset + slice;
                *pIndices++ = stackOffset + nextSlice;
                *pIndices++ = nextStackOffset + nextSlice;
                *pIndices++ = nextStackOffset + nextSlice;
                *pIndices++ = nextStackOffset + slice;
                *pIndices++ = stackOffset + slice;
            }
        }
    }

    void GatherGeometry(JPH::BoxShape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        unsigned int const faceIndices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

        int firstVertex = vertices.Size();
        int firstIndex = indices.Size();

        vertices.Resize(firstVertex + 8);
        indices.Resize(firstIndex + 36);

        Float3* pVertices = vertices.ToPtr() + firstVertex;
        unsigned int* pIndices = indices.ToPtr() + firstIndex;

        Float3 halfExtents = ConvertVector(shape->GetHalfExtent());

        *pVertices++ = Float3(-halfExtents.X, halfExtents.Y, -halfExtents.Z);
        *pVertices++ = Float3(halfExtents.X, halfExtents.Y, -halfExtents.Z);
        *pVertices++ = Float3(halfExtents.X, halfExtents.Y, halfExtents.Z);
        *pVertices++ = Float3(-halfExtents.X, halfExtents.Y, halfExtents.Z);
        *pVertices++ = Float3(-halfExtents.X, -halfExtents.Y, -halfExtents.Z);
        *pVertices++ = Float3(halfExtents.X, -halfExtents.Y, -halfExtents.Z);
        *pVertices++ = Float3(halfExtents.X, -halfExtents.Y, halfExtents.Z);
        *pVertices++ = Float3(-halfExtents.X, -halfExtents.Y, halfExtents.Z);

        for (int i = 0; i < 36; i++)
        {
            *pIndices++ = firstVertex + faceIndices[i];
        }
    }

    void GatherGeometry(JPH::CylinderShape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        float sinPhi, cosPhi;

        float halfHeight = shape->GetHalfHeight();
        float radius = shape->GetRadius();

        float detail = Math::Floor(Math::Max(1.0f, radius) + 0.5f);

        int numSlices = 8 * detail;
        int faceTriangles = numSlices - 2;

        int vertexCount = numSlices * 2;
        int indexCount = faceTriangles * 3 * 2 + numSlices * 6;

        int firstVertex = vertices.Size();
        int firstIndex = indices.Size();

        vertices.Resize(firstVertex + vertexCount);
        indices.Resize(firstIndex + indexCount);

        Float3* pVertices = vertices.ToPtr() + firstVertex;
        unsigned int* pIndices = indices.ToPtr() + firstIndex;

        Float3 vert;

        for (int slice = 0; slice < numSlices; slice++, pVertices++)
        {
            Math::SinCos(slice * Math::_2PI / numSlices, sinPhi, cosPhi);

            vert[0] = cosPhi * radius;
            vert[2] = sinPhi * radius;
            vert[1] = halfHeight;

            *pVertices = vert;

            vert[1] = -vert[1];

            *(pVertices + numSlices) = vert;
        }

        int offset = firstVertex;
        int nextOffset = firstVertex + numSlices;

        // top face
        for (int i = 0; i < faceTriangles; i++)
        {
            *pIndices++ = offset + i + 2;
            *pIndices++ = offset + i + 1;
            *pIndices++ = offset + 0;
        }

        // bottom face
        for (int i = 0; i < faceTriangles; i++)
        {
            *pIndices++ = nextOffset + i + 1;
            *pIndices++ = nextOffset + i + 2;
            *pIndices++ = nextOffset + 0;
        }

        for (int slice = 0; slice < numSlices; ++slice)
        {
            int nextSlice = (slice + 1) % numSlices;
            *pIndices++ = offset + slice;
            *pIndices++ = offset + nextSlice;
            *pIndices++ = nextOffset + nextSlice;
            *pIndices++ = nextOffset + nextSlice;
            *pIndices++ = nextOffset + slice;
            *pIndices++ = offset + slice;
        }
    }

    void GatherGeometry(JPH::CapsuleShape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        float radius = shape->GetRadius();

        int x, y;
        float verticalAngle, horizontalAngle;

        unsigned int quad[4];

        float detail = Math::Floor(Math::Max(1.0f, radius) + 0.5f);

        int numVerticalSubdivs = 6 * detail;
        int numHorizontalSubdivs = 8 * detail;

        int halfVerticalSubdivs = numVerticalSubdivs >> 1;

        int vertexCount = (numHorizontalSubdivs + 1) * (numVerticalSubdivs + 1) * 2;
        int indexCount = numHorizontalSubdivs * (numVerticalSubdivs + 1) * 6;

        int firstVertex = vertices.Size();
        int firstIndex = indices.Size();

        vertices.Resize(firstVertex + vertexCount);
        indices.Resize(firstIndex + indexCount);

        Float3* pVertices = vertices.ToPtr() + firstVertex;
        unsigned int* pIndices = indices.ToPtr() + firstIndex;

        float verticalStep = Math::_PI / numVerticalSubdivs;
        float horizontalStep = Math::_2PI / numHorizontalSubdivs;

        float halfHeight = shape->GetHalfHeightOfCylinder();

        for (y = 0, verticalAngle = -Math::_HALF_PI; y <= halfVerticalSubdivs; y++)
        {
            float h, r;
            Math::SinCos(verticalAngle, h, r);
            h = h * radius - halfHeight;
            r *= radius;
            for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
            {
                float s, c;
                Float3& v = *pVertices++;
                Math::SinCos(horizontalAngle, s, c);
                v[0] = r * c;
                v[2] = r * s;
                v[1] = h;
                horizontalAngle += horizontalStep;
            }
            verticalAngle += verticalStep;
        }

        for (y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++)
        {
            float h, r;
            Math::SinCos(verticalAngle, h, r);
            h = h * radius + halfHeight;
            r *= radius;
            for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
            {
                float s, c;
                Float3& v = *pVertices++;
                Math::SinCos(horizontalAngle, s, c);
                v[0] = r * c;
                v[2] = r * s;
                v[1] = h;
                horizontalAngle += horizontalStep;
            }
            verticalAngle += verticalStep;
        }

        for (y = 0; y <= numVerticalSubdivs; y++)
        {
            int y2 = y + 1;
            for (x = 0; x < numHorizontalSubdivs; x++)
            {
                int x2 = x + 1;
                quad[0] = firstVertex + y * (numHorizontalSubdivs + 1) + x;
                quad[1] = firstVertex + y2 * (numHorizontalSubdivs + 1) + x;
                quad[2] = firstVertex + y2 * (numHorizontalSubdivs + 1) + x2;
                quad[3] = firstVertex + y * (numHorizontalSubdivs + 1) + x2;
                *pIndices++ = quad[0];
                *pIndices++ = quad[1];
                *pIndices++ = quad[2];
                *pIndices++ = quad[2];
                *pIndices++ = quad[3];
                *pIndices++ = quad[0];
            }
        }
    }

    void GatherGeometry(JPH::ConvexHullShape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        int vertexCount = shape->GetNumPoints();

        int indexCount = 0;

        uint32_t faceCount = shape->GetNumFaces();
        for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            indexCount += (shape->GetNumVerticesInFace(faceIndex) - 2) * 3;
        }

        int firstVertex = vertices.Size();
        int firstIndex = indices.Size();

        vertices.Resize(firstVertex + vertexCount);
        indices.Resize(firstIndex + indexCount);

        Float3* pVertices = vertices.ToPtr() + firstVertex;
        unsigned int* pIndices = indices.ToPtr() + firstIndex;

        for (int i = 0; i < vertexCount; i++)
        {
            pVertices[i] = ConvertVector(shape->GetPoint(i));
        }

        for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            const JPH::uint8* indexData = shape->GetFaceVertices(faceIndex);
            int triangleCount = shape->GetNumVerticesInFace(faceIndex) - 2;

            for (int i = 0; i < triangleCount; i++)
            {
                pIndices[0] = firstVertex + indexData[0];
                pIndices[1] = firstVertex + indexData[i + 1];
                pIndices[2] = firstVertex + indexData[i + 2];
                pIndices += 3;
            }
        }
    }

    void GatherGeometry(JPH::MeshShape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        using TriangleCodec = JPH::TriangleCodecIndexed8BitPackSOA4Flags;
        using NodeCodec = JPH::NodeCodecQuadTreeHalfFloat<1>;

        struct Visitor
        {
            JPH_INLINE bool ShouldAbort() const
            {
                return false;
            }

            JPH_INLINE bool ShouldVisitNode(int inStackTop) const
            {
                return true;
            }

            JPH_INLINE int VisitNodes(JPH::Vec4Arg inBoundsMinX, JPH::Vec4Arg inBoundsMinY, JPH::Vec4Arg inBoundsMinZ, JPH::Vec4Arg inBoundsMaxX, JPH::Vec4Arg inBoundsMaxY, JPH::Vec4Arg inBoundsMaxZ, JPH::UVec4& ioProperties, int inStackTop)
            {
                JPH::UVec4 valid = JPH::UVec4::sOr(JPH::UVec4::sOr(JPH::Vec4::sLess(inBoundsMinX, inBoundsMaxX), JPH::Vec4::sLess(inBoundsMinY, inBoundsMaxY)), JPH::Vec4::sLess(inBoundsMinZ, inBoundsMaxZ));
                return CountAndSortTrues(valid, ioProperties);
            }

            JPH_INLINE void VisitTriangles(const TriangleCodec::DecodingContext& ioContext, const void* inTriangles, int inNumTriangles, [[maybe_unused]] JPH::uint32 inTriangleBlockID)
            {
                HK_ASSERT(inNumTriangles <= JPH::MeshShape::MaxTrianglesPerLeaf);

                JPH::Vec3 vertices[JPH::MeshShape::MaxTrianglesPerLeaf * 3];
                ioContext.Unpack(inTriangles, inNumTriangles, vertices);

                auto firstVertex = m_Vertices.Size();

                for (const JPH::Vec3 *v = vertices, *v_end = vertices + inNumTriangles * 3; v < v_end; v += 3)
                {
                    m_Vertices.Add(ConvertVector(v[0]));
                    m_Vertices.Add(ConvertVector(v[1]));
                    m_Vertices.Add(ConvertVector(v[2]));

                    m_Indices.Add(firstVertex + 0);
                    m_Indices.Add(firstVertex + 1);
                    m_Indices.Add(firstVertex + 2);

                    firstVertex += 3;
                }
            }

            Vector<Float3>& m_Vertices;
            Vector<unsigned int>& m_Indices;
        };

        Visitor visitor{vertices, indices};

        const NodeCodec::Header* header = shape->mTree.Get<NodeCodec::Header>(0);
        NodeCodec::DecodingContext node_ctx(header);

        const TriangleCodec::DecodingContext triangle_ctx(shape->mTree.Get<TriangleCodec::TriangleHeader>(NodeCodec::HeaderSize));
        const JPH::uint8* buffer_start = &shape->mTree[0];
        node_ctx.WalkTree(buffer_start, triangle_ctx, visitor);
    }

    void GatherGeometrySimpleShape(JPH::Shape const* shape, Vector<Float3>& vertices, Vector<unsigned int>& indices)
    {
        switch (shape->GetSubType())
        {
        case JPH::EShapeSubType::Sphere:
            GatherGeometry(static_cast<JPH::SphereShape const*>(shape), vertices, indices);
            break;
        case JPH::EShapeSubType::Box:
            GatherGeometry(static_cast<JPH::BoxShape const*>(shape), vertices, indices);
            break;
        case JPH::EShapeSubType::Cylinder:
            GatherGeometry(static_cast<JPH::CylinderShape const*>(shape), vertices, indices);
            break;
        case JPH::EShapeSubType::Capsule:
            GatherGeometry(static_cast<JPH::CapsuleShape const*>(shape), vertices, indices);
            break;
        case JPH::EShapeSubType::ConvexHull:
            GatherGeometry(static_cast<JPH::ConvexHullShape const*>(shape), vertices, indices);
            break;
        case JPH::EShapeSubType::Mesh:
            GatherGeometry(static_cast<JPH::MeshShape const*>(shape), vertices, indices);
            break;
        case JPH::EShapeSubType::Triangle:
            HK_ASSERT_(0, "Unsupported shape type Triangle\n");
            break;
        case JPH::EShapeSubType::TaperedCapsule:
            HK_ASSERT_(0, "Unsupported shape type TaperedCapsule\n");
            break;
        case JPH::EShapeSubType::HeightField:
            HK_ASSERT_(0, "Use TerrainCollider to gather geometry\n");
            break;
        case JPH::EShapeSubType::SoftBody:
            HK_ASSERT_(0, "Unsupported shape type SoftBody\n");
        default:
            HK_ASSERT_(0, "Unknown shape type\n");
            break;
        }
    }

    void GatherShapeGeometry(JPH::Shape const* shape, Vector<Float3>& outVertices, Vector<uint32_t>& outIndices)
    {
        if (!shape)
            return;

        switch (shape->GetSubType())
        {
            case JPH::EShapeSubType::Sphere:
            case JPH::EShapeSubType::Box:
            case JPH::EShapeSubType::Triangle:
            case JPH::EShapeSubType::Capsule:
            case JPH::EShapeSubType::TaperedCapsule:
            case JPH::EShapeSubType::Cylinder:
            case JPH::EShapeSubType::ConvexHull:
            case JPH::EShapeSubType::Mesh:
            case JPH::EShapeSubType::HeightField:
            case JPH::EShapeSubType::SoftBody: {
                auto centerOfMass = ConvertVector(shape->GetCenterOfMass());
                Float3x4 centerOfMassOffsetMatrix = Float3x4::Translation(centerOfMass);

                auto firstVert = outVertices.Size();
                GatherGeometrySimpleShape(shape, outVertices, outIndices);
                TransformVertices(outVertices.ToPtr() + firstVert, outVertices.Size() - firstVert, centerOfMassOffsetMatrix);
                break;
            }

            case JPH::EShapeSubType::StaticCompound: {
                JPH::StaticCompoundShape const* compoundShape = static_cast<JPH::StaticCompoundShape const*>(shape);

                auto centerOfMass = ConvertVector(shape->GetCenterOfMass());
                Float3x4 centerOfMassOffsetMatrix = Float3x4::Translation(centerOfMass);

                Float3x4 localTransform;

                JPH::StaticCompoundShape::SubShapes const& subShapes = compoundShape->GetSubShapes();
                for (JPH::StaticCompoundShape::SubShape const& subShape : subShapes)
                {
                    auto position = ConvertVector(subShape.GetPositionCOM());
                    auto rotation = ConvertQuaternion(subShape.GetRotation());

                    localTransform.Compose(position, rotation.ToMatrix3x3());

                    auto firstVert = outVertices.Size();
                    GatherShapeGeometry(subShape.mShape, outVertices, outIndices);
                    TransformVertices(outVertices.ToPtr() + firstVert, outVertices.Size() - firstVert, centerOfMassOffsetMatrix * localTransform);
                }
                break;
            }

            case JPH::EShapeSubType::MutableCompound:
                HK_ASSERT_(0, "MutableCompound shape is not supported\n");
                break;

            case JPH::EShapeSubType::RotatedTranslated: {
                JPH::RotatedTranslatedShape const* transformedShape = static_cast<JPH::RotatedTranslatedShape const*>(shape);

                Float3x4 localTransform;
                localTransform.Compose(ConvertVector(transformedShape->GetPosition()), ConvertQuaternion(transformedShape->GetRotation()).ToMatrix3x3());

                auto firstVert = outVertices.Size();
                GatherShapeGeometry(transformedShape->GetInnerShape(), outVertices, outIndices);
                TransformVertices(outVertices.ToPtr() + firstVert, outVertices.Size() - firstVert, localTransform);
                break;
            }
            case JPH::EShapeSubType::Scaled: {
                JPH::ScaledShape const* scaledShape = static_cast<JPH::ScaledShape const*>(shape);

                auto firstVert = outVertices.Size();
                GatherShapeGeometry(scaledShape->GetInnerShape(), outVertices, outIndices);
                TransformVertices(outVertices.ToPtr() + firstVert, outVertices.Size() - firstVert, Float3x4::Scale(ConvertVector(scaledShape->GetScale())));
                break;
            }
            case JPH::EShapeSubType::OffsetCenterOfMass:
                HK_ASSERT_(0, "TODO: Add OffsetCenterOfMass\n");
                break;
        }
    }

} // namespace

void PhysicsInterfaceImpl::GatherShapeGeometry(JPH::Shape const* shape, Vector<Float3>& outVertices, Vector<uint32_t>& outIndices)
{
    Hk::GatherShapeGeometry(shape, outVertices, outIndices);
}

namespace
{

    void DrawSphere(DebugRenderer& renderer, JPH::SphereShape const* shape)
    {
        renderer.DrawSphere(Float3(0), shape->GetRadius());
    }

    void DrawBox(DebugRenderer& renderer, JPH::BoxShape const* shape)
    {
        renderer.DrawBox(Float3(0), ConvertVector(shape->GetHalfExtent()));
    }

    void DrawCylinder(DebugRenderer& renderer, JPH::CylinderShape const* shape)
    {
        renderer.DrawCylinder(Float3(0), Float3x3::Identity(), shape->GetRadius(), shape->GetHalfHeight() * 2);
    }

    void DrawCapsule(DebugRenderer& renderer, JPH::CapsuleShape const* shape)
    {
        renderer.DrawCapsule(Float3(0), Float3x3::Identity(), shape->GetRadius(), shape->GetHalfHeightOfCylinder() * 2, 1);
    }

    void DrawConvexHull(DebugRenderer& renderer, JPH::ConvexHullShape const* shape)
    {
        SmallVector<Float3, 32> verts;

        uint32_t faceCount = shape->GetNumFaces();
        for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            verts.Clear();

            uint32_t vertexCount = shape->GetNumVerticesInFace(faceIndex);
            const JPH::uint8* indexData = shape->GetFaceVertices(faceIndex);
            for (uint32_t v = 0; v < vertexCount; ++v)
                verts.Add(ConvertVector(shape->GetPoint(indexData[v])));

            renderer.DrawLine(verts, true);
        }
    }

    void DrawMesh(DebugRenderer& renderer, JPH::MeshShape const* shape)
    {
        // TODO
    }

    void DrawSimpleShape(DebugRenderer& renderer, JPH::Shape const* shape, Float3x4 const& transform)
    {
        renderer.PushTransform(transform);

        switch (shape->GetSubType())
        {
        case JPH::EShapeSubType::Sphere:
            DrawSphere(renderer, static_cast<JPH::SphereShape const*>(shape));
            break;
        case JPH::EShapeSubType::Box:
            DrawBox(renderer, static_cast<JPH::BoxShape const*>(shape));
            break;
        case JPH::EShapeSubType::Cylinder:
            DrawCylinder(renderer, static_cast<JPH::CylinderShape const*>(shape));
            break;
        case JPH::EShapeSubType::Capsule:
            DrawCapsule(renderer, static_cast<JPH::CapsuleShape const*>(shape));
            break;
        case JPH::EShapeSubType::ConvexHull:
            DrawConvexHull(renderer, static_cast<JPH::ConvexHullShape const*>(shape));
            break;
        case JPH::EShapeSubType::Mesh:
            DrawMesh(renderer, static_cast<JPH::MeshShape const*>(shape));
            break;
        case JPH::EShapeSubType::Triangle:
            HK_ASSERT_(0, "Unsupported shape type Triangle\n");
            break;
        case JPH::EShapeSubType::TaperedCapsule:
            HK_ASSERT_(0, "Unsupported shape type TaperedCapsule\n");
            break;
        case JPH::EShapeSubType::HeightField:
            HK_ASSERT_(0, "Use TerrainCollider to draw shape\n");
            break;
        case JPH::EShapeSubType::SoftBody:
            HK_ASSERT_(0, "Unsupported shape type SoftBody\n");
        default:
            HK_ASSERT_(0, "Unknown shape type\n");
            break;
        }

        renderer.PopTransform();
    }

    void DrawShape(DebugRenderer& renderer, JPH::Shape const* shape, Float3x4 const& transform)
    {
        if (!shape)
            return;

        switch (shape->GetSubType())
        {
            case JPH::EShapeSubType::Sphere:
            case JPH::EShapeSubType::Box:
            case JPH::EShapeSubType::Triangle:
            case JPH::EShapeSubType::Capsule:
            case JPH::EShapeSubType::TaperedCapsule:
            case JPH::EShapeSubType::Cylinder:
            case JPH::EShapeSubType::ConvexHull:
            case JPH::EShapeSubType::Mesh:
            case JPH::EShapeSubType::HeightField:
            case JPH::EShapeSubType::SoftBody: {

                auto centerOfMass = ConvertVector(shape->GetCenterOfMass());
                Float3x4 centerOfMassOffsetMatrix = transform * Float3x4::Translation(centerOfMass);

                DrawSimpleShape(renderer, shape, centerOfMassOffsetMatrix);
                break;
            }

            case JPH::EShapeSubType::StaticCompound: {
                JPH::StaticCompoundShape const* compoundShape = static_cast<JPH::StaticCompoundShape const*>(shape);

                auto centerOfMass = ConvertVector(shape->GetCenterOfMass());
                Float3x4 centerOfMassOffsetMatrix = transform * Float3x4::Translation(centerOfMass);

                Float3x4 localTransform;

                JPH::StaticCompoundShape::SubShapes const& subShapes = compoundShape->GetSubShapes();
                for (JPH::StaticCompoundShape::SubShape const& subShape : subShapes)
                {
                    auto position = ConvertVector(subShape.GetPositionCOM());
                    auto rotation = ConvertQuaternion(subShape.GetRotation());

                    localTransform.Compose(position, rotation.ToMatrix3x3());

                    DrawShape(renderer, subShape.mShape, centerOfMassOffsetMatrix * localTransform);
                }
                break;
            }

            case JPH::EShapeSubType::MutableCompound:
                HK_ASSERT_(0, "MutableCompound shape is not supported\n");
                break;

            case JPH::EShapeSubType::RotatedTranslated: {
                JPH::RotatedTranslatedShape const* transformedShape = static_cast<JPH::RotatedTranslatedShape const*>(shape);

                Float3x4 localTransform;
                localTransform.Compose(ConvertVector(transformedShape->GetPosition()), ConvertQuaternion(transformedShape->GetRotation()).ToMatrix3x3());

                DrawShape(renderer, transformedShape->GetInnerShape(), transform * localTransform);
                break;
            }
            case JPH::EShapeSubType::Scaled: {
                JPH::ScaledShape const* scaledShape = static_cast<JPH::ScaledShape const*>(shape);

                DrawShape(renderer, scaledShape->GetInnerShape(), transform * Float3x4::Scale(ConvertVector(scaledShape->GetScale())));
                break;
            }
            case JPH::EShapeSubType::OffsetCenterOfMass:
                HK_ASSERT_(0, "TODO: Add OffsetCenterOfMass\n");
                break;
        }
    }

    void CopyShapeCastResult(ShapeCastResult& out, JPH::ShapeCastResult const& in)
    {
        out.BodyID = PhysBodyID(in.mBodyID2.GetIndexAndSequenceNumber());
        out.ContactPointOn1 = ConvertVector(in.mContactPointOn1);
        out.ContactPointOn2 = ConvertVector(in.mContactPointOn2);
        out.PenetrationAxis = ConvertVector(in.mPenetrationAxis);
        out.PenetrationDepth = in.mPenetrationDepth;
        out.Fraction = in.mFraction;
        out.IsBackFaceHit = in.mIsBackFaceHit;
    }

    void CopyShapeCastResult(Vector<ShapeCastResult>& out, JPH::Array<JPH::ShapeCastResult> const& in)
    {
        out.Resize(in.size());
        for (int i = 0; i < in.size(); i++)
            CopyShapeCastResult(out[i], in[i]);
    }

    void CopyShapeCollideResult(ShapeCollideResult& out, JPH::CollideShapeResult const& in)
    {
        out.BodyID = PhysBodyID(in.mBodyID2.GetIndexAndSequenceNumber());
        out.ContactPointOn1 = ConvertVector(in.mContactPointOn1);
        out.ContactPointOn2 = ConvertVector(in.mContactPointOn2);
        out.PenetrationAxis = ConvertVector(in.mPenetrationAxis);
        out.PenetrationDepth = in.mPenetrationDepth;
    }

    void CopyShapeCollideResult(Vector<ShapeCollideResult>& out, JPH::Array<JPH::CollideShapeResult> const& in)
    {
        out.Resize(in.size());
        for (int i = 0; i < in.size(); i++)
            CopyShapeCollideResult(out[i], in[i]);
    }

}

bool PhysicsInterfaceImpl::CastShapeClosest(JPH::RShapeCast const& inShapeCast, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = true;

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysSystem.GetNarrowPhaseQuery().CastShape(inShapeCast, settings, JPH::RVec3::sZero(), collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()), CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    if (collector.HadHit())
        CopyShapeCastResult(outResult, collector.mHit);

    return collector.HadHit();
}

bool PhysicsInterfaceImpl::CastShape(JPH::RShapeCast const& inShapeCast, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = false;

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysSystem.GetNarrowPhaseQuery().CastShape(inShapeCast, settings, JPH::RVec3::sZero(), collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()), CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCastResult(outResult, collector.mHits);
    }

    return collector.HadHit();
}

void BodyActivationListener::OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
{
    BodyUserData* userdata = reinterpret_cast<BodyUserData*>(inBodyUserData);
    if (userdata->TypeID == ComponentRTTR::TypeID<DynamicBodyComponent>)
    {
        MutexGuard lockGuard(m_Mutex);
        m_ActiveBodies.SortedInsert(userdata->Component);
    }
}

void BodyActivationListener::OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
{
    BodyUserData* userdata = reinterpret_cast<BodyUserData*>(inBodyUserData);
    if (userdata->TypeID == ComponentRTTR::TypeID<DynamicBodyComponent>)
    {
        MutexGuard lockGuard(m_Mutex);
        m_ActiveBodies.SortedErase(userdata->Component);
        m_JustDeactivated.SortedInsert(userdata->Component);
    }
}

JPH::ValidateResult	ContactListener::OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult)
{
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

namespace
{
    bool IsBodyDispatchEvent(BodyComponent* body)
    {
        auto typeID = body->GetManager()->GetComponentTypeID();
        if (typeID == ComponentRTTR::TypeID<StaticBodyComponent>)
            return static_cast<StaticBodyComponent*>(body)->DispatchContactEvents;
        if (typeID == ComponentRTTR::TypeID<DynamicBodyComponent>)
            return static_cast<DynamicBodyComponent*>(body)->DispatchContactEvents;
        if (typeID == ComponentRTTR::TypeID<HeightFieldComponent>)
            return static_cast<HeightFieldComponent*>(body)->DispatchContactEvents;
        return false;
    }
}


void ContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
{
    if (inBody1.IsSensor() || inBody2.IsSensor())
    {
        TriggerComponent* trigger = nullptr;
        ComponentExtendedHandle target;

        if (inBody1.IsSensor())
        {
            trigger = reinterpret_cast<BodyUserData*>(inBody1.GetUserData())->TryGetComponent<TriggerComponent>(m_World);
            target  = reinterpret_cast<BodyUserData*>(inBody2.GetUserData())->GetExtendedHandle();
        }
        else if (inBody2.IsSensor())
        {
            trigger = reinterpret_cast<BodyUserData*>(inBody2.GetUserData())->TryGetComponent<TriggerComponent>(m_World);
            target  = reinterpret_cast<BodyUserData*>(inBody1.GetUserData())->GetExtendedHandle();
        }

        if (trigger && target)
        {
            uint32_t body1ID = inBody1.GetID().GetIndexAndSequenceNumber();
            uint32_t body2ID = inBody2.GetID().GetIndexAndSequenceNumber();
            uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

            std::lock_guard lock(m_Mutex);
            TriggerContact& contact = m_Triggers[contactID];
            contact.Trigger = Handle32<TriggerComponent>(trigger->GetHandle());
            contact.Target  = target;
            ++contact.Count;
            if (contact.Count == 1)
            {
                auto& event = m_pTriggerEvents->EmplaceBack();
                event.Type = TriggerEvent::OnBeginOverlap;
                event.Trigger = contact.Trigger;
                event.Target = contact.Target;
            }
        }

        return;
    }

    AddContactEvents(inBody1, inBody2, inManifold, ioSettings, false);
}

void ContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
{
#if 0
    uint32_t body1ID = inBody1.GetID().GetIndexAndSequenceNumber();
    uint32_t body2ID = inBody2.GetID().GetIndexAndSequenceNumber();
    uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

    std::lock_guard lock(m_Mutex);

    auto it = m_Triggers.Find(contactID);
    if (it != m_Triggers.End())
    {
        TriggerContact& contact = it->second;

        auto& event = m_pTriggerEvents->EmplaceBack();
        event.Type = TriggerEvent::OnUpdateOverlap;
        event.Trigger = contact.Trigger;
        event.Target = contact.Target;

        m_Triggers.Erase(it);

        return;
    }
#endif

    AddContactEvents(inBody1, inBody2, inManifold, ioSettings, true);
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair)
{
    uint32_t body1ID = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
    uint32_t body2ID = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();
    uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

    std::lock_guard lock(m_Mutex);

    {
        auto it = m_Triggers.Find(contactID);
        if (it != m_Triggers.End())
        {
            TriggerContact& contact = it->second;
            HK_ASSERT(contact.Count > 0);
            contact.Count--;
            if (contact.Count == 0)
            {
                auto& event = m_pTriggerEvents->EmplaceBack();
                event.Type = TriggerEvent::OnEndOverlap;
                event.Trigger = contact.Trigger;
                event.Target = contact.Target;

                m_Triggers.Erase(it);
            }

            return;
        }
    }

    {
        auto it = m_BodyContacts.Find(contactID);
        if (it != m_BodyContacts.End())
        {
            BodyContact& contact = it->second;

            if (contact.Body1DispatchEvent)
            {
                auto& event = m_pContactEvents->EmplaceBack();
                event.Type = ContactEvent::OnEndContact;
                event.Self = contact.Body1;
                event.Other = contact.Body2;
            }

            if (contact.Body2DispatchEvent)
            {
                auto& event = m_pContactEvents->EmplaceBack();
                event.Type = ContactEvent::OnEndContact;
                event.Self = contact.Body2;
                event.Other = contact.Body1;
            }

            m_BodyContacts.Erase(it);
        }
    }
    //LOG("m_BodyContacts {}\n", m_BodyContacts.Size());
}

void ContactListener::AddContactEvents(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings, bool isPersisted)
{
    BodyUserData* userData1 = reinterpret_cast<BodyUserData*>(inBody1.GetUserData());
    BodyUserData* userData2 = reinterpret_cast<BodyUserData*>(inBody2.GetUserData());

    ComponentExtendedHandle handle1  = userData1->GetExtendedHandle();
    ComponentExtendedHandle handle2  = userData2->GetExtendedHandle();

    // We assume that BodyUserData only contains a component that derives from BodyComponent
    BodyComponent* body1 = static_cast<BodyComponent*>(userData1->TryGetComponent(m_World));
    BodyComponent* body2 = static_cast<BodyComponent*>(userData2->TryGetComponent(m_World));

    if (!body1 || !body2)
        return;

    bool body1DispatchEvent = IsBodyDispatchEvent(body1);
    bool body2DispatchEvent = IsBodyDispatchEvent(body2);

    if (body1DispatchEvent || body2DispatchEvent)
    {
        uint32_t body1ID = inBody1.GetID().GetIndexAndSequenceNumber();
        uint32_t body2ID = inBody2.GetID().GetIndexAndSequenceNumber();
        uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

        const float MinVelocityForRestitution = 1.0f; // TODO: Move to config
        const uint32_t NumIterations = 5;

        JPH::CollisionEstimationResult estimationResult;
        JPH::EstimateCollisionResponse(inBody1, inBody2, inManifold, estimationResult, ioSettings.mCombinedFriction, ioSettings.mCombinedRestitution, MinVelocityForRestitution, NumIterations);

        std::lock_guard lock(m_Mutex);

        BodyContact& contact = m_BodyContacts[contactID];
        contact.Body1 = handle1;
        contact.Body2 = handle2;
        contact.Body1DispatchEvent = body1DispatchEvent;
        contact.Body2DispatchEvent = body2DispatchEvent;

        uint32_t numContactPoints = inManifold.mRelativeContactPointsOn1.size();

        uint32_t firstPoint = m_pContactPoints->Size();
        m_pContactPoints->Resize(firstPoint + ((body1DispatchEvent && body2DispatchEvent) ? numContactPoints * 2 : numContactPoints));

        uint32_t offset = body1DispatchEvent ? numContactPoints : 0;

        for (uint32_t index = 0; index < numContactPoints; ++index)
        {
            auto contactPosition1 = inManifold.GetWorldSpaceContactPointOn1(index);
            auto contactPosition2 = inManifold.GetWorldSpaceContactPointOn2(index);

            auto velocity1 = ConvertVector(inBody1.GetPointVelocity(contactPosition1));
            auto velocity2 = ConvertVector(inBody2.GetPointVelocity(contactPosition2));

            auto& impulse = estimationResult.mImpulses[index];

            auto frictionImpulse1 = estimationResult.mTangent1 * impulse.mFrictionImpulse1;
            auto frictionImpulse2 = estimationResult.mTangent2 * impulse.mFrictionImpulse2;
            auto combinedImpulse = ConvertVector(inManifold.mWorldSpaceNormal * impulse.mContactImpulse + frictionImpulse1 + frictionImpulse2);

            if (body1DispatchEvent)
            {
                auto& contactPoint = (*m_pContactPoints)[firstPoint + index];

                contactPoint.PositionSelf = ConvertVector(contactPosition1);
                contactPoint.PositionOther = ConvertVector(contactPosition2);
                contactPoint.VelocitySelf = velocity1;
                contactPoint.VelocityOther = velocity2;
                contactPoint.Impulse = -combinedImpulse;
            }

            if (body2DispatchEvent)
            {
                auto& contactPoint = (*m_pContactPoints)[firstPoint + index + offset];

                contactPoint.PositionSelf = ConvertVector(contactPosition2);
                contactPoint.PositionOther = ConvertVector(contactPosition1);
                contactPoint.VelocitySelf = velocity2;
                contactPoint.VelocityOther = velocity1;
                contactPoint.Impulse = combinedImpulse;
            }
        }

        if (body1DispatchEvent)
        {
            auto& event = m_pContactEvents->EmplaceBack();
            event.Type = isPersisted ? ContactEvent::OnUpdateContact : ContactEvent::OnBeginContact;
            event.Self = handle1;
            event.Other = handle2;
            event.Normal = -ConvertVector(inManifold.mWorldSpaceNormal);
            event.Depth = inManifold.mPenetrationDepth;
            event.FirstPoint = firstPoint;
            event.NumPoints = numContactPoints;
        }

        if (body2DispatchEvent)
        {
            auto& event = m_pContactEvents->EmplaceBack();
            event.Type = isPersisted ? ContactEvent::OnUpdateContact : ContactEvent::OnBeginContact;
            event.Self = handle2;
            event.Other = handle1;
            event.Normal = ConvertVector(inManifold.mWorldSpaceNormal);
            event.Depth = inManifold.mPenetrationDepth;
            event.FirstPoint = firstPoint + offset;
            event.NumPoints = numContactPoints;
        }        
    }
}

void CharacterContactListener::OnContactAdded(const JPH::CharacterVirtual* character, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
{
    CharacterControllerImpl const* characterImpl = static_cast<CharacterControllerImpl const*>(character);
    BodyUserData* userData = nullptr;
    bool isSensor = false;
    Float3 contactVelocity;

    {
        JPH::BodyLockRead lock(m_PhysSystem->GetBodyLockInterface(), inBodyID2);
        if (lock.Succeeded())
        {
            auto& body = lock.GetBody();

            userData = reinterpret_cast<BodyUserData*>(body.GetUserData());
            isSensor = body.IsSensor();

            if (!isSensor)
                contactVelocity = ConvertVector(body.GetPointVelocity(inContactPosition));
        }
    }

    if (isSensor)
    {
        if (auto trigger = userData->TryGetComponent<TriggerComponent>(m_World))
        {
            ContactID contactID = inBodyID2.GetIndexAndSequenceNumber() | (static_cast<uint64_t>(characterImpl->m_Component.ToUInt32()) << 32);

            TriggerContact& contact = m_Triggers[contactID];
            contact.Trigger = Handle32<TriggerComponent>(trigger->GetHandle());
            if (contact.FrameIndex == 0)
            {
                auto& event = m_pTriggerEvents->EmplaceBack();
                event.Type = TriggerEvent::OnBeginOverlap;
                event.Trigger = contact.Trigger;
                event.Target.Handle = ComponentHandle(characterImpl->m_Component);
                event.Target.TypeID = ComponentRTTR::TypeID<CharacterControllerComponent>;
                m_UpdateOverlap.Add(contactID);
            }
            else
            {
                // TODO: Generate OnUpdateOverlap?
            }
            contact.FrameIndex = m_World->GetTick().FixedFrameNum;
        }
        return;
    }

    ioSettings.mCanPushCharacter = false;

    if (userData)
    {
        if (BodyComponent* body2 = static_cast<BodyComponent*>(userData->TryGetComponent(m_World)))
        {
            bool body2DispatchEvent = IsBodyDispatchEvent(body2);
            if (body2DispatchEvent)
            {
                ComponentExtendedHandle handle2 = userData->GetExtendedHandle();
                ContactID contactID = inBodyID2.GetIndexAndSequenceNumber() | (static_cast<uint64_t>(characterImpl->m_Component.ToUInt32()) << 32);

                BodyContact& contact = m_BodyContacts[contactID];
                bool isPersisted = contact.FrameIndex != 0;

                contact.Body = handle2;
                contact.FrameIndex = m_World->GetTick().FixedFrameNum;

                auto& event = m_pContactEvents->EmplaceBack();
                event.Type = isPersisted ? ContactEvent::OnUpdateContact : ContactEvent::OnBeginContact;
                event.Self = handle2;
                event.Other.Handle = ComponentHandle(characterImpl->m_Component);
                event.Other.TypeID = ComponentRTTR::TypeID<CharacterControllerComponent>;
                event.Normal = ConvertVector(inContactNormal);
                event.Depth = 0;
                event.FirstPoint = m_pContactPoints->Size();
                event.NumPoints = 1;

                auto& contactPoint = m_pContactPoints->EmplaceBack();
                contactPoint.PositionSelf = contactPoint.PositionOther = ConvertVector(inContactPosition);
                contactPoint.VelocitySelf = contactVelocity;

                // TODO:
                //contactPoint.VelocityOther;
                //contactPoint.Impulse;

                if (!isPersisted)
                    m_UpdateContact.Add(contactID);
            }
        }

        if (auto dynamicBody = userData->TryGetComponent<DynamicBodyComponent>(m_World))
        {
            ioSettings.mCanPushCharacter = dynamicBody->CanPushCharacter;
        }
    }
}

void CharacterContactListener::OnContactSolve(const JPH::CharacterVirtual* character, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
{
}

CharacterControllerImpl::CharacterControllerImpl(const JPH::CharacterVirtualSettings* inSettings, JPH::Vec3Arg inPosition, JPH::QuatArg inRotation, JPH::PhysicsSystem* inSystem) :
    JPH::CharacterVirtual(inSettings, inPosition, inRotation, inSystem)
{
}

PhysicsInterface::PhysicsInterface() :
    m_pImpl(new PhysicsInterfaceImpl)
{}

void PhysicsInterface::Initialize()
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

    m_pImpl->m_CollisionFilter.SetShouldCollide(0, 0, true);

    // Now we can create the actual physics system.
    m_pImpl->m_PhysSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_pImpl->m_BroadPhaseLayerInterface, m_pImpl->m_ObjectVsBroadPhaseLayerFilter, m_pImpl->m_ObjectVsObjectLayerFilter);

    m_pImpl->m_GroupFilter = new GroupFilter;
    m_pImpl->m_GroupFilter->AddRef();

    m_pImpl->m_PhysSystem.SetBodyActivationListener(&m_pImpl->m_BodyActivationListener);
    m_pImpl->m_PhysSystem.SetContactListener(&m_pImpl->m_ContactListener);

    m_pImpl->m_ContactListener.m_World = GetWorld();
    m_pImpl->m_ContactListener.m_pTriggerEvents = &m_pImpl->m_TriggerEvents;
    m_pImpl->m_ContactListener.m_pContactEvents = &m_pImpl->m_ContactEvents;
    m_pImpl->m_ContactListener.m_pContactPoints = &m_pImpl->m_ContactPoints;

    m_pImpl->m_CharacterContactListener.m_World = GetWorld();
    m_pImpl->m_CharacterContactListener.m_PhysSystem = &m_pImpl->m_PhysSystem;
    m_pImpl->m_CharacterContactListener.m_pTriggerEvents = &m_pImpl->m_TriggerEvents;
    m_pImpl->m_CharacterContactListener.m_pContactEvents = &m_pImpl->m_ContactEvents;
    m_pImpl->m_CharacterContactListener.m_pContactPoints = &m_pImpl->m_ContactPoints;

    {
        TickFunction tickFunc;
        tickFunc.Desc.Name.FromString("Update Physics");
        tickFunc.Desc.TickEvenWhenPaused = true;
        tickFunc.Group = TickGroup::PhysicsUpdate;
        tickFunc.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
        tickFunc.Delegate.Bind(this, &PhysicsInterface::Update);
        RegisterTickFunction(tickFunc);
    }

    {
        TickFunction tickFunc;
        tickFunc.Desc.Name.FromString("Update Physics Post Transform");
        tickFunc.Desc.TickEvenWhenPaused = true;
        tickFunc.Group = TickGroup::PostTransform;
        tickFunc.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
        tickFunc.Delegate.Bind(this, &PhysicsInterface::PostTransform);
        RegisterTickFunction(tickFunc);
    }

    RegisterDebugDrawFunction({this, &PhysicsInterface::DrawDebug});
}

void PhysicsInterface::Deinitialize()
{
    m_pImpl->m_GroupFilter->Release();
    m_pImpl->m_GroupFilter = nullptr;
}

void PhysicsInterface::Purge()
{
    m_pImpl->m_QueueToAdd[0].Clear();
    m_pImpl->m_QueueToAdd[1].Clear();
}

void PhysicsInterface::UpdateCharacterControllers()
{
    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();
    auto& tick = GetWorld()->GetTick();
    float timeStep = tick.FixedTimeStep;

    auto& characterControllerManager = GetWorld()->GetComponentManager<CharacterControllerComponent>();

    struct Visitor
    {
        JPH::BodyInterface&     m_BodyInterface;
        CollisionFilter const&  m_CollisionFilter;
        JPH::Vec3               m_Gravity;
        float                   m_TimeStep;

        Visitor(float timeStep, JPH::Vec3 const& gravity, CollisionFilter const& collisionFilter, JPH::BodyInterface& bodyInterface) :
            m_BodyInterface(bodyInterface),
            m_CollisionFilter(collisionFilter),
            m_Gravity(gravity),
            m_TimeStep(timeStep)
        {}

        HK_FORCEINLINE void Visit(CharacterControllerComponent& character)
        {
            GameObject* owner = character.GetOwner();

            auto* physCharacter = character.m_pImpl;

            JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
            if (character.EnableStickToFloor)
                updateSettings.mStickToFloorStepDown = JPH::Vec3(0,character.StickToFloorStepDown,0);
            else
                updateSettings.mStickToFloorStepDown = JPH::Vec3::sZero();

            if (character.EnableWalkStairs)
                updateSettings.mWalkStairsStepUp = JPH::Vec3(0,character.StairsStepUp,0);
            else
                updateSettings.mWalkStairsStepUp = JPH::Vec3::sZero();

            updateSettings.mWalkStairsMinStepForward = 0.2f;

            // TODO: Move these settings to CharacterControllerComponent
            //mWalkStairsMinStepForward { 0.02f };									// See WalkStairs inStepForward parameter. Note that the parameter only indicates a magnitude, direction is taken from current velocity.
            //mWalkStairsStepForwardTest { 0.15f };									// See WalkStairs inStepForwardTest parameter. Note that the parameter only indicates a magnitude, direction is taken from current velocity.
            //mWalkStairsCosAngleForwardContact { Cos(DegreesToRadians(75.0f)) };	// Cos(angle) where angle is the maximum angle between the ground normal in the horizontal plane and the character forward vector where we're willing to adjust the step forward test towards the contact normal.
            //mWalkStairsStepDownExtra { Vec3::sZero() };							// See WalkStairs inStepDownExtra

            CharacterControllerImpl::BroadphaseLayerFilter broadphaseFilter(
                HK_BIT(uint32_t(BroadphaseLayer::Static)) |
                HK_BIT(uint32_t(BroadphaseLayer::Dynamic)) |
                HK_BIT(uint32_t(BroadphaseLayer::Trigger)) |
                HK_BIT(uint32_t(BroadphaseLayer::Character)));

            ObjectLayerFilter layerFilter(m_CollisionFilter, character.CollisionLayer);
            CharacterControllerImpl::BodyFilter bodyFilter;
            CharacterControllerImpl::ShapeFilter shapeFilter;

            // Update the character position
            physCharacter->ExtendedUpdate(m_TimeStep, m_Gravity, updateSettings, broadphaseFilter, layerFilter, bodyFilter, shapeFilter, *PhysicsModule::Get().GetTempAllocator());

            owner->SetWorldPositionAndRotation(ConvertVector(physCharacter->GetPosition()), ConvertQuaternion(physCharacter->GetRotation()));

            if (physCharacter->GetGroundState() != JPH::CharacterVirtual::EGroundState::OnGround &&
                physCharacter->GetGroundState() != JPH::CharacterVirtual::EGroundState::OnSteepGround)
            {
                const float Overbounce = 1;
                auto velocity = physCharacter->GetLinearVelocity();
                for (JPH::CharacterVirtual::Contact const &c : physCharacter->GetActiveContacts())
                    if (c.mHadCollision)
                        velocity = velocity - c.mContactNormal * (velocity.Dot(c.mContactNormal) * Overbounce);
                physCharacter->SetLinearVelocity(velocity);
            }
        }
    };

    Visitor visitor(timeStep, m_pImpl->m_PhysSystem.GetGravity(), m_pImpl->m_CollisionFilter, bodyInterface);
    characterControllerManager.IterateComponents(visitor);

    for (auto contactIt = m_pImpl->m_CharacterContactListener.m_UpdateOverlap.begin(); contactIt != m_pImpl->m_CharacterContactListener.m_UpdateOverlap.end();)
    {
        auto contactID = *contactIt;
        bool removeContact = false;

        auto it = m_pImpl->m_CharacterContactListener.m_Triggers.Find(contactID);
        if (it != m_pImpl->m_CharacterContactListener.m_Triggers.End())
        {
            auto& contact = it->second;
            if (contact.FrameIndex != tick.FixedFrameNum)
            {
                auto& event = m_pImpl->m_TriggerEvents.EmplaceBack();
                event.Type = TriggerEvent::OnEndOverlap;
                event.Trigger = contact.Trigger;
                event.Target.Handle = ComponentHandle(contactID >> 32);
                event.Target.TypeID = ComponentRTTR::TypeID<CharacterControllerComponent>;

                m_pImpl->m_CharacterContactListener.m_Triggers.Erase(it);
                removeContact = true;
            }
        }

        if (removeContact)
            contactIt = m_pImpl->m_CharacterContactListener.m_UpdateOverlap.Erase(contactIt);
        else
            ++contactIt;
    }

    for (auto contactIt = m_pImpl->m_CharacterContactListener.m_UpdateContact.begin(); contactIt != m_pImpl->m_CharacterContactListener.m_UpdateContact.end();)
    {
        auto contactID = *contactIt;
        bool removeContact = false;

        auto it = m_pImpl->m_CharacterContactListener.m_BodyContacts.Find(contactID);
        if (it != m_pImpl->m_CharacterContactListener.m_BodyContacts.End())
        {
            auto& contact = it->second;
            if (contact.FrameIndex != tick.FixedFrameNum)
            {
                auto& event = m_pImpl->m_ContactEvents.EmplaceBack();
                event.Type = ContactEvent::OnEndContact;
                event.Self = contact.Body;
                event.Other.Handle = ComponentHandle(contactID >> 32);
                event.Other.TypeID = ComponentRTTR::TypeID<CharacterControllerComponent>;
                m_pImpl->m_CharacterContactListener.m_BodyContacts.Erase(it);
                removeContact = true;
            }
        }

        if (removeContact)
            contactIt = m_pImpl->m_CharacterContactListener.m_UpdateContact.Erase(contactIt);
        else
            ++contactIt;
    }
}

void PhysicsInterface::Update()
{
    auto& tick = GetWorld()->GetTick();
    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();
    auto& dynamicBodyManager = GetWorld()->GetComponentManager<DynamicBodyComponent>();
    auto& triggerManager = GetWorld()->GetComponentManager<TriggerComponent>();

    for (int i = 0; i < 2; ++i)
    {
        if (!m_pImpl->m_QueueToAdd[i].IsEmpty())
        {
            auto addState = bodyInterface.AddBodiesPrepare(m_pImpl->m_QueueToAdd[i].ToPtr(), m_pImpl->m_QueueToAdd[i].Size());
            bodyInterface.AddBodiesFinalize(m_pImpl->m_QueueToAdd[i].ToPtr(), m_pImpl->m_QueueToAdd[i].Size(), addState, JPH::EActivation(i));

            m_pImpl->m_QueueToAdd[i].Clear();
        }
    }

    if (tick.IsPaused)
        return;

    // Update character controllers
    UpdateCharacterControllers();

    // Update dynmaic scaling
    // TODO: Update scale with lower framerate to save performance
    {
        for (auto& dynamicBody : m_pImpl->m_DynamicScaling)
        {
            DynamicBodyComponent* component = dynamicBodyManager.GetComponent(dynamicBody);

            auto* owner = component->GetOwner();

            Float3 const& scale = owner->GetWorldScale();
            if (scale != component->m_CachedScale)
            {
                component->m_CachedScale = scale;

                if (JPH::Shape* shape = m_pImpl->CreateScaledShape(component->m_ScalingMode, component->m_Shape, scale))
                {
                    const bool bUpdateMassProperties = false;
                    bodyInterface.SetShape(JPH::BodyID(component->m_BodyID.ID), shape, bUpdateMassProperties, JPH::EActivation::Activate);
                }
            }
        }
    }

    // Move triggers bodies
    {
        for (auto& trigger : m_pImpl->m_MovableTriggers)
        {
            TriggerComponent* component = triggerManager.GetComponent(trigger);

            auto* owner = component->GetOwner();

            owner->UpdateWorldTransform();

            JPH::Vec3 position = ConvertVector(owner->GetWorldPosition());
            JPH::Quat rotation = ConvertQuaternion(owner->GetWorldRotation()).Normalized();

            bodyInterface.SetPositionAndRotation(JPH::BodyID(component->m_BodyID.ID), position, rotation, JPH::EActivation::Activate);
        }
    }

    // Move kinematic bodies
    {
        float timeStep = tick.FixedTimeStep;

        for (auto& kinematicBody : m_pImpl->m_KinematicBodies)
        {
            DynamicBodyComponent* component = dynamicBodyManager.GetComponent(kinematicBody);

            auto* owner = component->GetOwner();

            owner->UpdateWorldTransform();

            JPH::Vec3 position = ConvertVector(owner->GetWorldPosition());
            JPH::Quat rotation = ConvertQuaternion(owner->GetWorldRotation()).Normalized();

            bodyInterface.MoveKinematic(JPH::BodyID(component->m_BodyID.ID), position, rotation, timeStep);
        }
    }

    // Update dynamic bodies
    {
        for (auto& msg : m_pImpl->m_DynamicBodyMessageQueue)
        {
            DynamicBodyComponent* component = dynamicBodyManager.GetComponent(msg.m_Component);
            if (!component)
                continue;

            if (component->IsKinematic())
                continue;

            auto bodyID = JPH::BodyID(component->m_BodyID.ID);
            if (bodyID.IsInvalid())
                continue;

            switch (msg.m_MsgType)
            {
            case DynamicBodyMessage::AddForce:
                bodyInterface.AddForce(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            case DynamicBodyMessage::AddForceAtPosition:
                bodyInterface.AddForce(bodyID, ConvertVector(msg.m_Data[0]), ConvertVector(msg.m_Data[1]));
                break;
            case DynamicBodyMessage::AddTorque:
                bodyInterface.AddTorque(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            case DynamicBodyMessage::AddForceAndTorque:
                bodyInterface.AddForceAndTorque(bodyID, ConvertVector(msg.m_Data[0]), ConvertVector(msg.m_Data[1]));
                break;
            case DynamicBodyMessage::AddImpulse:
                bodyInterface.AddImpulse(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            case DynamicBodyMessage::AddImpulseAtPosition:
                bodyInterface.AddImpulse(bodyID, ConvertVector(msg.m_Data[0]), ConvertVector(msg.m_Data[1]));
                break;
            case DynamicBodyMessage::AddAngularImpulse:
                bodyInterface.AddAngularImpulse(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            }
        }
        m_pImpl->m_DynamicBodyMessageQueue.Clear();
    }

    // Update water bodies
    {
        float timeStep = tick.FixedTimeStep;

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

                //if (body.GetMotionType() != JPH::EMotionType::Dynamic)
                //    LOG("Motion type {}\n", body.GetMotionType() == JPH::EMotionType::Static ? "Static" : "Kinematic");
            }

        private:
            JPH::PhysicsSystem& mSystem;
            JPH::RVec3 mSurfacePosition;
            JPH::Vec3 mSurfaceNormal;
            float mDeltaTime;
        };

        Collector collector(m_pImpl->m_PhysSystem, JPH::Vec3::sAxisY(), timeStep);

        auto& waterVolumeManager = GetWorld()->GetComponentManager<WaterVolumeComponent>();

        struct Visitor
        {
            JPH::BroadPhaseQuery const& m_BroadPhaseQuery;
            CollisionFilter const& m_CollisionFilter;
            Collector& m_Collector;

            Visitor(JPH::BroadPhaseQuery const& broadPhaseQuery, CollisionFilter const& collisionFilter, Collector& collector) :
                m_BroadPhaseQuery(broadPhaseQuery),
                m_CollisionFilter(collisionFilter),
                m_Collector(collector)
            {}

            HK_FORCEINLINE void Visit(WaterVolumeComponent& waterVolume)
            {
                if (waterVolume.HalfExtents.X <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Y <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Z <= std::numeric_limits<float>::epsilon())
                    return;

                GameObject* owner = waterVolume.GetOwner();

                Float3 worldPosition = owner->GetWorldPosition();
                Float3 scaledExtents = waterVolume.HalfExtents * owner->GetWorldScale();

                Float3 mins = worldPosition - scaledExtents;
                Float3 maxs = worldPosition + scaledExtents;

                JPH::AABox waterBox(ConvertVector(mins), ConvertVector(maxs));

                Float3 surfacePos = worldPosition;
                surfacePos.Y = maxs.Y;

                m_Collector.SetSurfacePosition(surfacePos);

                ObjectLayerFilter layerFilter(m_CollisionFilter, waterVolume.CollisionLayer);

                m_BroadPhaseQuery.CollideAABox(waterBox, m_Collector, JPH::SpecifiedBroadPhaseLayerFilter(JPH::BroadPhaseLayer(ToUnderlying(BroadphaseLayer::Dynamic))), layerFilter);
            }
        };

        JPH::BroadPhaseQuery const& broadphaseQuery = m_pImpl->m_PhysSystem.GetBroadPhaseQuery();

        Visitor visitor(broadphaseQuery, m_pImpl->m_CollisionFilter, collector);
        waterVolumeManager.IterateComponents(visitor);
    }

    // Simulation step
    {
        const int numCollisionSteps = 1;
        auto& physicsModule = PhysicsModule::Get();

        m_pImpl->m_PhysSystem.Update(tick.FixedTimeStep, numCollisionSteps, physicsModule.GetTempAllocator(), physicsModule.GetJobSystemThreadPool());
    }

    // Capture active bodies transform
    {
        for (uint32_t handle : m_pImpl->m_BodyActivationListener.m_ActiveBodies)
        {
            DynamicBodyComponent* body = dynamicBodyManager.GetComponent(Handle32<DynamicBodyComponent>(handle));

            if (body && !body->IsKinematic())
            {
                JPH::Vec3 position;
                JPH::Quat rotation;

                bodyInterface.GetPositionAndRotation(JPH::BodyID(body->m_BodyID.ID), position, rotation);

                body->GetOwner()->SetWorldPositionAndRotation(ConvertVector(position), ConvertQuaternion(rotation));
            }
        }

        for (uint32_t handle : m_pImpl->m_BodyActivationListener.m_JustDeactivated)
        {
            DynamicBodyComponent* body = dynamicBodyManager.GetComponent(Handle32<DynamicBodyComponent>(handle));

            if (body && !body->IsKinematic())
            {
                JPH::Vec3 position;
                JPH::Quat rotation;

                bodyInterface.GetPositionAndRotation(JPH::BodyID(body->m_BodyID.ID), position, rotation);

                body->GetOwner()->SetWorldPositionAndRotation(ConvertVector(position), ConvertQuaternion(rotation));
            }
        }

        //LOG("Active {} from {}, just deactivated {}, kinematic {}\n", m_pImpl->m_BodyActivationListener.m_ActiveBodies.Size(), dynamicBodyManager.GetComponentCount(), m_pImpl->m_BodyActivationListener.m_JustDeactivated.Size(), m_pImpl->m_KinematicBodies.Size());
        m_pImpl->m_BodyActivationListener.m_JustDeactivated.Clear();
    }
}

void PhysicsInterface::PostTransform()
{
    for (auto& event : m_pImpl->m_TriggerEvents)
    {
        TriggerComponent* trigger = GetWorld()->GetComponent(event.Trigger);

        switch (event.Type)
        {
            case TriggerEvent::OnBeginOverlap:
            {
                if (auto componentManager = GetWorld()->TryGetComponentManager(event.Target.TypeID))
                {
                    if (auto component = componentManager->GetComponent(event.Target.Handle))
                    {
                        BodyComponent* body = static_cast<BodyComponent*>(component);
                        World::DispatchEvent<Event_OnBeginOverlap>(trigger->GetOwner(), body);
                    }
                }
                break;
            }
            case TriggerEvent::OnEndOverlap:
            {
                if (auto componentManager = GetWorld()->TryGetComponentManager(event.Target.TypeID))
                {
                    if (auto component = componentManager->GetComponent(event.Target.Handle))
                    {
                        BodyComponent* body = static_cast<BodyComponent*>(component);
                        World::DispatchEvent<Event_OnEndOverlap>(trigger->GetOwner(), body);
                    }
                }
                break;
            }
        }
    }
    m_pImpl->m_TriggerEvents.Clear();

    for (auto& event : m_pImpl->m_ContactEvents)
    {
        BodyComponent* self = nullptr;
        BodyComponent* other = nullptr;

        if (auto body1ComponentManager = GetWorld()->TryGetComponentManager(event.Self.TypeID))
            if (auto body1Component = body1ComponentManager->GetComponent(event.Self.Handle))
                self = static_cast<BodyComponent*>(body1Component);

        if (auto body2ComponentManager = GetWorld()->TryGetComponentManager(event.Other.TypeID))
            if (auto body2Component = body2ComponentManager->GetComponent(event.Other.Handle))
                other = static_cast<BodyComponent*>(body2Component);

        if (!self || !other)
            continue;

        switch (event.Type)
        {
            case ContactEvent::OnBeginContact:
            {
                Collision collision;
                collision.Body = other;
                collision.Normal = event.Normal;
                collision.Depth = event.Depth;
                collision.Contacts = ArrayView<ContactPoint>(&m_pImpl->m_ContactPoints[event.FirstPoint], event.NumPoints);
                World::DispatchEvent<Event_OnBeginContact>(self->GetOwner(), collision);
                break;
            }
            case ContactEvent::OnUpdateContact:
            {
                Collision collision;
                collision.Body = other;
                collision.Normal = event.Normal;
                collision.Depth = event.Depth;
                collision.Contacts = ArrayView<ContactPoint>(&m_pImpl->m_ContactPoints[event.FirstPoint], event.NumPoints);
                World::DispatchEvent<Event_OnUpdateContact>(self->GetOwner(), collision);
                break;
            }
            case ContactEvent::OnEndContact:
            {
                World::DispatchEvent<Event_OnEndContact>(self->GetOwner(), other);
                break;
            }
        }
    }
    m_pImpl->m_ContactEvents.Clear();
    m_pImpl->m_ContactPoints.Clear();
}

template <typename T>
void PhysicsInterface::DrawRigidBody(DebugRenderer& renderer, T* rigidBody)
{
    m_DebugDrawVertices.Clear();
    m_DebugDrawIndices.Clear();

    rigidBody->GatherGeometry(m_DebugDrawVertices, m_DebugDrawIndices);

    renderer.DrawTriangleSoupWireframe(m_DebugDrawVertices, m_DebugDrawIndices);
}

void PhysicsInterface::DrawHeightField(DebugRenderer& renderer, PhysBodyID bodyID, HeightFieldComponent* heightfield)
{
    if (!heightfield->Data)
        return;

    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

    m_DebugDrawVertices.Clear();
    m_DebugDrawIndices.Clear();

    JPH::Vec3 position;
    JPH::Quat rotation;
    bodyInterface.GetPositionAndRotation(JPH::BodyID(bodyID.ID), position, rotation);

    Float3x4 transformMatrix;
    transformMatrix.Compose(ConvertVector(position), ConvertQuaternion(rotation).ToMatrix3x3());

    Float3x4 transformMatrixInv = transformMatrix.Inversed();
    Float3 localViewPosition = transformMatrixInv * renderer.GetRenderView()->ViewPosition;

    BvAxisAlignedBox localBounds(localViewPosition - 5, localViewPosition + 5);

    localBounds.Mins.Y = -FLT_MAX;
    localBounds.Maxs.Y = FLT_MAX;

    heightfield->Data->GatherGeometry(localBounds, m_DebugDrawVertices, m_DebugDrawIndices);

    renderer.PushTransform(transformMatrix);
    renderer.DrawTriangleSoupWireframe(m_DebugDrawVertices, m_DebugDrawIndices);
    renderer.PopTransform();
}

void PhysicsInterface::DrawDebug(DebugRenderer& renderer)
{
    const float visibilityHalfSize = 5;

    if (com_DrawCollisionModel)
    {
        ShapeOverlapFilter filter;
        filter.BroadphaseLayers
            .AddLayer(BroadphaseLayer::Static)
            .AddLayer(BroadphaseLayer::Dynamic)
            .AddLayer(BroadphaseLayer::Trigger);

        OverlapBox(renderer.GetRenderView()->ViewPosition, Float3(visibilityHalfSize), Quat::Identity(), m_BodyQueryResult, filter);

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        renderer.SetColor(Color4::White());
        renderer.SetDepthTest(false);
        for (PhysBodyID bodyID : m_BodyQueryResult)
        {
            BodyUserData* userData = reinterpret_cast<BodyUserData*>(bodyInterface.GetUserData(JPH::BodyID(bodyID.ID)));

            if (auto staticBody = userData->TryGetComponent<StaticBodyComponent>(GetWorld()))
            {
                DrawRigidBody(renderer, staticBody);
            }
            else if (auto dynamicBody = userData->TryGetComponent<DynamicBodyComponent>(GetWorld()))
            {
                DrawRigidBody(renderer, dynamicBody);
            }
            else if (auto heightfield = userData->TryGetComponent<HeightFieldComponent>(GetWorld()))
            {
                DrawHeightField(renderer, bodyID, heightfield);
            }
        }
    }

    if (com_DrawCollisionShape)
    {
        ShapeOverlapFilter filter;
        filter.BroadphaseLayers
            .AddLayer(BroadphaseLayer::Static)
            .AddLayer(BroadphaseLayer::Dynamic);

        OverlapBox(renderer.GetRenderView()->ViewPosition, Float3(visibilityHalfSize), Quat::Identity(), m_BodyQueryResult, filter);

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        renderer.SetDepthTest(false);

        for (PhysBodyID bodyID : m_BodyQueryResult)
        {
            JPH::BodyID joltBodyID(bodyID.ID);

            auto motionType = bodyInterface.GetMotionType(joltBodyID);
            switch (motionType)
            {
            case JPH::EMotionType::Static:
                renderer.SetColor({0.6f, 0.6f, 0.6f, 1});
                break;

            case JPH::EMotionType::Kinematic:
                renderer.SetColor({0, 1, 1, 1});
                break;

            case JPH::EMotionType::Dynamic:
                renderer.SetColor(bodyInterface.IsActive(joltBodyID) ? Color4(1, 0, 1, 1) : Color4(1, 1, 1, 1));
                break;
            }

            JPH::Vec3 position;
            JPH::Quat rotation;
            bodyInterface.GetPositionAndRotation(joltBodyID, position, rotation);

            Float3x3 rotationMatrix = ConvertQuaternion(rotation).ToMatrix3x3();

            Float3x4 transformMatrix;
            transformMatrix.Compose(ConvertVector(position), rotationMatrix);

            DrawShape(renderer, bodyInterface.GetShape(joltBodyID), transformMatrix);

            if (motionType == JPH::EMotionType::Dynamic)
            {
                renderer.SetColor({1, 1, 1, 1});
                renderer.DrawAxis(ConvertVector(position), rotationMatrix[0], rotationMatrix[1], rotationMatrix[2], Float3(0.25f));
            }
        }
    }

    if (com_DrawWaterVolume)
    {
        auto& waterVolumeManager = GetWorld()->GetComponentManager<WaterVolumeComponent>();

        renderer.SetDepthTest(true);
        renderer.SetColor({0, 0, 1, 0.5f});

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;

            Visitor(DebugRenderer& debugRenderer) :
                m_DebugRenderer(debugRenderer)
            {}

            HK_FORCEINLINE void Visit(WaterVolumeComponent& waterVolume)
            {
                if (waterVolume.HalfExtents.X <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Y <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Z <= std::numeric_limits<float>::epsilon())
                    return;

                GameObject* owner = waterVolume.GetOwner();

                Float3 worldPosition = owner->GetWorldPosition();
                Float3 scaledExtents = waterVolume.HalfExtents * owner->GetWorldScale();

                m_DebugRenderer.DrawBoxFilled(worldPosition, scaledExtents, true);
            }
        };

        Visitor visitor(renderer);
        waterVolumeManager.IterateComponents(visitor);
    }

    if (com_DrawTriggers)
    {
        auto& triggerManager = GetWorld()->GetComponentManager<TriggerComponent>();

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;
            Vector<Float3>& m_DebugDrawVertices;
            Vector<unsigned int>& m_DebugDrawIndices;
            JPH::BodyInterface& m_BodyInterface;

            Visitor(DebugRenderer& debugRenderer, Vector<Float3>& debugDrawVertices, Vector<unsigned int>& debugDrawIndices, JPH::BodyInterface& bodyInterface) :
                m_DebugRenderer(debugRenderer), m_DebugDrawVertices(debugDrawVertices), m_DebugDrawIndices(debugDrawIndices), m_BodyInterface(bodyInterface)
            {}

            HK_FORCEINLINE void Visit(TriggerComponent& body)
            {
                m_DebugDrawVertices.Clear();
                m_DebugDrawIndices.Clear();

                PhysicsInterfaceImpl::GatherShapeGeometry(m_BodyInterface.GetShape((JPH::BodyID(body.m_BodyID.ID))), m_DebugDrawVertices, m_DebugDrawIndices);

                JPH::Vec3 position;
                JPH::Quat rotation;
                m_BodyInterface.GetPositionAndRotation(JPH::BodyID(body.m_BodyID.ID), position, rotation);

                Float3x4 transform;
                transform.Compose(ConvertVector(position), ConvertQuaternion(rotation).ToMatrix3x3());

                m_DebugRenderer.PushTransform(transform);
                m_DebugRenderer.DrawTriangleSoup(m_DebugDrawVertices, m_DebugDrawIndices);
                m_DebugRenderer.PopTransform();
            }
        };

        renderer.SetDepthTest(true);
        renderer.SetColor({0, 1, 0, 0.5f});

        Visitor visitor(renderer, m_DebugDrawVertices, m_DebugDrawIndices, bodyInterface);
        triggerManager.IterateComponents(visitor);
    }

    if (com_DrawCenterOfMass)
    {
        auto& dynamicBodyManager = GetWorld()->GetComponentManager<DynamicBodyComponent>();

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;
            JPH::BodyInterface& m_BodyInterface;

            Visitor(DebugRenderer& debugRenderer, JPH::BodyInterface& bodyInterface) :
                m_DebugRenderer(debugRenderer), m_BodyInterface(bodyInterface)
            {}

            HK_FORCEINLINE void Visit(DynamicBodyComponent& body)
            {
                if (!body.IsKinematic())
                {
                    //auto* owner = body.GetOwner();
                    //auto* model = body.m_CollisionModel.RawPtr();

                    //Float3x4 transform;
                    //transform.Compose(owner->GetWorldPosition(),
                    //    owner->GetWorldRotation().ToMatrix3x3(),
                    //    model->GetValidScale(body.m_CachedScale));

                    //Float3 centerOfMassPos = transform * model->GetCenterOfMass();
                    Float3 centerOfMassPos2 = ConvertVector(m_BodyInterface.GetCenterOfMassPosition(JPH::BodyID(body.m_BodyID.ID)));

                    //m_DebugRenderer.SetColor({1, 1, 1, 1});
                    //m_DebugRenderer.DrawBoxFilled(centerOfMassPos, Float3(0.05f));

                    m_DebugRenderer.SetColor({1, 0, 0, 1});
                    m_DebugRenderer.DrawBoxFilled(centerOfMassPos2, Float3(0.05f));
                }
            }
        };

        renderer.SetDepthTest(false);

        Visitor visitor(renderer, bodyInterface);
        dynamicBodyManager.IterateComponents(visitor);
    }

    if (com_DrawCharacterController)
    {
        auto& characterControllerManager = GetWorld()->GetComponentManager<CharacterControllerComponent>();

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;

            Visitor(DebugRenderer& debugRenderer) :
                m_DebugRenderer(debugRenderer)
            {}

            HK_FORCEINLINE void Visit(CharacterControllerComponent& character)
            {
                if (character.IsInitialized())
                {
                    Float3x4 transformMatrix;
                    transformMatrix.Compose(ConvertVector(character.m_pImpl->GetPosition()), ConvertQuaternion(character.m_pImpl->GetRotation()).ToMatrix3x3());

                    DrawShape(m_DebugRenderer, character.m_pImpl->GetShape(), transformMatrix);
                }
            }
        };

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 1, 0, 1));

        Visitor visitor(renderer);
        characterControllerManager.IterateComponents(visitor);
    }
}

bool PhysicsInterface::CastRayClosest(Float3 const& inRayStart, Float3 const& inRayDir, RayCastResult& outResult, RayCastFilter const& inFilter)
{
    JPH::RRayCast raycast;
    raycast.mOrigin = ConvertVector(inRayStart);
    raycast.mDirection = ConvertVector(inRayDir);

    JPH::RayCastResult hit;
    if (inFilter.IgonreBackFaces)
    {
        JPH::RayCastSettings settings;

        // How backfacing triangles should be treated
        //settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
        settings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;

        // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
        settings.mTreatConvexAsSolid = true;

        JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
        m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CastRay(raycast, settings, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()), CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

        if (!collector.HadHit())
            return false;

        hit = collector.mHit;
    }
    else
    {
        if (!m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CastRay(raycast, hit, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()), CastObjectLayerFilter(inFilter.ObjectLayers.Get())))
            return false;
    }

    outResult.BodyID = PhysBodyID(hit.mBodyID.GetIndexAndSequenceNumber());
    outResult.Fraction = hit.mFraction;

    if (inFilter.CalcSurfcaceNormal)
    {
        JPH::BodyLockRead lock(m_pImpl->m_PhysSystem.GetBodyLockInterface(), hit.mBodyID);
        JPH::Body const& body = lock.GetBody();

        auto normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, raycast.GetPointOnRay(outResult.Fraction));
        outResult.Normal = ConvertVector(normal);
    }

    return true;
}

bool PhysicsInterface::CastRay(Float3 const& inRayStart, Float3 const& inRayDir, Vector<RayCastResult>& outResult, RayCastFilter const& inFilter)
{
    JPH::RRayCast raycast;
    raycast.mOrigin = ConvertVector(inRayStart);
    raycast.mDirection = ConvertVector(inRayDir);

    JPH::RayCastSettings settings;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
    settings.mTreatConvexAsSolid = true;

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CastRay(raycast, settings, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()), CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (inFilter.SortByDistance)
            collector.Sort();

        outResult.Reserve(collector.mHits.size());
        for (auto& hit : collector.mHits)
        {
            RayCastResult& r = outResult.Add();
            r.BodyID = PhysBodyID(hit.mBodyID.GetIndexAndSequenceNumber());
            r.Fraction = hit.mFraction;

            if (inFilter.CalcSurfcaceNormal)
            {
                JPH::BodyLockRead lock(m_pImpl->m_PhysSystem.GetBodyLockInterface(), hit.mBodyID);
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

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShapeClosest(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastBox(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShape(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastBoxMinMaxClosest(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return m_pImpl->CastShapeClosest(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return m_pImpl->CastShape(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastSphereClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return m_pImpl->CastShapeClosest(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastSphere(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return m_pImpl->CastShape(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastCapsuleClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShapeClosest(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastCapsule(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShape(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastCylinderClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShapeClosest(shapeCast, outResult, inFilter);
}

bool PhysicsInterface::CastCylinder(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShape(shapeCast, outResult, inFilter);
}

void PhysicsInterface::OverlapBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    if (inRotation == Quat::Identity())
    {
        m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideAABox(JPH::AABox(ConvertVector(inPosition - inHalfExtent), ConvertVector(inPosition + inHalfExtent)),
            collector,
            BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
    }
    else
    {
        JPH::OrientedBox oriented_box;
        oriented_box.mOrientation.SetTranslation(ConvertVector(inPosition));
        oriented_box.mOrientation.SetRotation(ConvertMatrix(inRotation.ToMatrix4x4()));
        oriented_box.mHalfExtents = ConvertVector(inHalfExtent);

        m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideOrientedBox(oriented_box, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
    }
}

void PhysicsInterface::OverlapBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideAABox(JPH::AABox(ConvertVector(inMins), ConvertVector(inMaxs)),
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
}

void PhysicsInterface::OverlapSphere(Float3 const& inPosition, float inRadius, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideSphere(ConvertVector(inPosition),
        inRadius,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
}

void PhysicsInterface::OverlapPoint(Float3 const& inPosition, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollidePoint(ConvertVector(inPosition),
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
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
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);

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
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);

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
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);

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
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);

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
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckPoint(Float3 const& inPosition, BroadphaseLayerMask inBroadphaseLayers, ObjectLayerMask inObjectLayers)
{
    JPH::AnyHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(inPosition), collector, BroadphaseLayerFilter(inBroadphaseLayers.Get()), CastObjectLayerFilter(inObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);
    return collector.HadHit();
}

void PhysicsInterface::CollideBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideSphere(Float3 const& inPosition, float inRadius, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 baseOffset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        baseOffset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()),
        CastObjectLayerFilter(inFilter.ObjectLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollidePoint(Float3 const& inPosition, Vector<PhysBodyID>& outResult, BroadphaseLayerMask inBroadphaseLayers, ObjectLayerMask inObjectLayers)
{
    JPH::AllHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(inPosition), collector, BroadphaseLayerFilter(inBroadphaseLayers.Get()), CastObjectLayerFilter(inObjectLayers.Get())
    /* TODO: bodyFilter, shapeFilter*/);

    outResult.Clear();
    if (collector.HadHit())
    {
        outResult.Reserve(collector.mHits.size());
        for (JPH::CollidePointResult const& hit : collector.mHits)
            outResult.Add(PhysBodyID(hit.mBodyID.GetIndexAndSequenceNumber()));
    }
}

void PhysicsInterface::SetGravity(Float3 const inGravity)
{
    return m_pImpl->m_PhysSystem.SetGravity(ConvertVector(inGravity));
}

Float3 PhysicsInterface::GetGravity() const
{
    return ConvertVector(m_pImpl->m_PhysSystem.GetGravity());
}

void PhysicsInterface::SetCollisionFilter(CollisionFilter const& inCollisionFilter)
{
    m_pImpl->m_CollisionFilter = inCollisionFilter;
}

CollisionFilter const& PhysicsInterface::GetCollisionFilter() const
{
    return m_pImpl->m_CollisionFilter;
}

Component* PhysicsInterface::TryGetComponent(PhysBodyID inBodyID)
{
    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

    BodyUserData* userData = reinterpret_cast<BodyUserData*>(bodyInterface.GetUserData(JPH::BodyID(inBodyID.ID)));
    if (!userData)
        return nullptr;

    return userData->TryGetComponent(GetWorld());
}

HK_NAMESPACE_END
