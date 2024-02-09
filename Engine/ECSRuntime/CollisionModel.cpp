#include "CollisionModel.h"

#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>

#include <Jolt/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h>
#include <Jolt/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat.h>

#include <Engine/Runtime/DebugRenderer.h>

#include <Engine/Runtime/PhysicsModule.h>
#include <Engine/Geometry/ConvexDecomposition.h>

HK_NAMESPACE_BEGIN

CollisionModel* CollisionModel::Create(CollisionModelCreateInfo const& createInfo)
{
    int shapeCount = 0;

    shapeCount += createInfo.SphereCount;
    shapeCount += createInfo.BoxCount;
    shapeCount += createInfo.CylinderCount;
    shapeCount += createInfo.CapsuleCount;
    shapeCount += createInfo.ConvexHullCount;
    shapeCount += createInfo.TriangleMeshCount;

    if (!shapeCount)
        return nullptr;

    bool bUseCompound = shapeCount > 1;

    JPH::StaticCompoundShapeSettings compoundSettings;
    if (bUseCompound)
    {
        compoundSettings.mSubShapes.reserve(shapeCount);
    }

    CollisionModel* model = new CollisionModel;

    for (CollisionSphereDef* shape = createInfo.pSpheres ; shape < &createInfo.pSpheres[createInfo.SphereCount] ; shape++)
    {
        JPH::SphereShape* sphere = new JPH::SphereShape(shape->Radius);

        if (bUseCompound)
        {
            compoundSettings.AddShape(ConvertVector(shape->Position), JPH::Quat::sIdentity(), sphere);
        }
        else if (shape->Position.LengthSqr() > 0.001f)
        {
            model->m_Shape = new JPH::RotatedTranslatedShape(ConvertVector(shape->Position), JPH::Quat::sIdentity(), sphere);
        }
        else
        {
            model->m_Shape = sphere;
        }

        model->m_AllowedScalingMode = CollisionModel::UNIFORM;
    }

    for (CollisionBoxDef* shape = createInfo.pBoxes ; shape < &createInfo.pBoxes[createInfo.BoxCount] ; shape++)
    {
        JPH::BoxShape* box = new JPH::BoxShape(ConvertVector(shape->HalfExtents));

        if (bUseCompound)
        {
            compoundSettings.AddShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), box);
        }
        else if (shape->Position.LengthSqr() > 0.001f || shape->Rotation != Quat::Identity())
        {
            model->m_Shape = new JPH::RotatedTranslatedShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), box);
        }
        else
        {
            model->m_Shape = box;
        }
    }

    for (CollisionCylinderDef* shape = createInfo.pCylinders ; shape < &createInfo.pCylinders[createInfo.CylinderCount] ; shape++)
    {
        JPH::CylinderShape* cylinder = new JPH::CylinderShape(shape->Height * 0.5f, shape->Radius);

        if (bUseCompound)
        {
            compoundSettings.AddShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), cylinder);
        }
        else if (shape->Position.LengthSqr() > 0.001f || shape->Rotation != Quat::Identity())
        {
            model->m_Shape = new JPH::RotatedTranslatedShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), cylinder);
        }
        else
        {
            model->m_Shape = cylinder;
        }

        if (model->m_AllowedScalingMode != CollisionModel::UNIFORM)
        {
            model->m_AllowedScalingMode = shape->Rotation != Quat::Identity() ? CollisionModel::UNIFORM : CollisionModel::UNIFORM_XZ;
        }
    }

    for (CollisionCapsuleDef* shape = createInfo.pCapsules ; shape < &createInfo.pCapsules[createInfo.CapsuleCount] ; shape++)
    {
        JPH::CapsuleShape* capsule = new JPH::CapsuleShape(shape->Height * 0.5f, shape->Radius);

        if (bUseCompound)
        {
            compoundSettings.AddShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), capsule);
        }
        else if (shape->Position.LengthSqr() > 0.001f || shape->Rotation != Quat::Identity())
        {
            model->m_Shape = new JPH::RotatedTranslatedShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), capsule);
        }
        else
        {
            model->m_Shape = capsule;
        }

        model->m_AllowedScalingMode = CollisionModel::UNIFORM;
    }

    JPH::ConvexHullShapeSettings convexHullSettings;
    convexHullSettings.mMaxConvexRadius = JPH::cDefaultConvexRadius;
    for (CollisionConvexHullDef* shape = createInfo.pConvexHulls ; shape < &createInfo.pConvexHulls[createInfo.ConvexHullCount] ; shape++)
    {
        convexHullSettings.mPoints.resize(shape->VertexCount);

        for (int i = 0 ; i < shape->VertexCount ; i++)
            convexHullSettings.mPoints[i] = ConvertVector(shape->pVertices[i]);

        JPH::ShapeSettings::ShapeResult result;

        JPH::ConvexHullShape* convexHull = new JPH::ConvexHullShape(convexHullSettings, result);

        if (bUseCompound)
        {
            compoundSettings.AddShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), convexHull);
        }
        else if (shape->Position.LengthSqr() > 0.001f || shape->Rotation != Quat::Identity())
        {
            model->m_Shape = new JPH::RotatedTranslatedShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), convexHull);
        }
        else
        {
            model->m_Shape = convexHull;
        }
    }

    JPH::MeshShapeSettings meshSettings;

    for (CollisionTriangleSoupDef* shape = createInfo.pTriangleMeshes ; shape < &createInfo.pTriangleMeshes[createInfo.TriangleMeshCount] ; shape++)
    {
        uint32_t triangleCount = shape->IndexCount / 3;

        meshSettings.mTriangleVertices.resize(shape->VertexCount);

        static_assert(sizeof(JPH::Float3) == sizeof(Float3), "Type sizeof mismatch");

        if (shape->VertexStride == sizeof(Float3))
        {
            Core::Memcpy(meshSettings.mTriangleVertices.data(), shape->pVertices, shape->VertexCount * sizeof(JPH::Float3));
        }
        else
        {
            for (int i = 0; i < shape->VertexCount; i++)
                *(Float3*)(&meshSettings.mTriangleVertices[i]) = *(Float3 const*)((uint8_t const*)shape->pVertices + i * shape->VertexStride);
        }

        meshSettings.mIndexedTriangles.resize(triangleCount);
        for (int i = 0 ; i < triangleCount ; i++)
        {
            meshSettings.mIndexedTriangles[i].mIdx[0] = shape->pIndices[i*3 + 0];
            meshSettings.mIndexedTriangles[i].mIdx[1] = shape->pIndices[i*3 + 1];
            meshSettings.mIndexedTriangles[i].mIdx[2] = shape->pIndices[i*3 + 2];
        }

        meshSettings.Sanitize();

        JPH::ShapeSettings::ShapeResult result;
        JPH::MeshShape* mesh = new JPH::MeshShape(meshSettings, result);

        if (bUseCompound)
        {
            compoundSettings.AddShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), mesh);
        }
        else if (shape->Position.LengthSqr() > 0.001f || shape->Rotation != Quat::Identity())
        {
            model->m_Shape = new JPH::RotatedTranslatedShape(ConvertVector(shape->Position), ConvertQuaternion(shape->Rotation), mesh);
        }
        else
        {
            model->m_Shape = mesh;
        }
    }

    if (bUseCompound)
    {
        JPH::ShapeSettings::ShapeResult result;
        JPH::StaticCompoundShape* compoundShape = new JPH::StaticCompoundShape(compoundSettings, *PhysicsModule::Get().GetTempAllocator(), result);

        model->m_Shape = compoundShape;
    }

    model->m_CenterOfMass = ConvertVector(model->m_Shape->GetCenterOfMass());

    //if (createInfo.CenterOfMassOffset.LengthSqr() > 0.001f)
    //{
    //    JPH::ShapeSettings::ShapeResult result;
    //    JPH::OffsetCenterOfMassShape* offsetShape = new JPH::OffsetCenterOfMassShape(JPH::OffsetCenterOfMassShapeSettings(ConvertVector(createInfo.CenterOfMassOffset), model->m_Shape), result);

    //    model->m_Shape = offsetShape;
    //}

    //model->m_CenterOfMassWithOffset = ConvertVector(model->m_Shape->GetCenterOfMass());

    return model;
}

Float3 const& CollisionModel::GetCenterOfMass() const
{
    return m_CenterOfMass;//m_CenterOfMassWithOffset;
}

Float3 CollisionModel::GetCenterOfMassWorldPosition(Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale) const
{
    return worldPosition + worldRotation * (GetCenterOfMass() * GetValidScale(worldScale));
}

Float3 CollisionModel::GetValidScale(Float3 const& scale) const
{
    if (scale.X != 1 || scale.Y != 1 || scale.Z != 1)
    {
        bool bIsUniformXZ = scale.X == scale.Z;
        bool bIsUniformScaling = bIsUniformXZ && scale.X == scale.Y; // FIXME: allow some epsilon?

        if (m_AllowedScalingMode == CollisionModel::NON_UNIFORM || bIsUniformScaling)
        {
            return scale;
        }
        else if (m_AllowedScalingMode == CollisionModel::UNIFORM_XZ)
        {
            float scaleXZ = Math::Max(scale.X, scale.Z);

            return Float3(scaleXZ, scale.Y, scaleXZ);
        }
        else
        {
            return Float3(Math::Max3(scale.X, scale.Y, scale.Z));
        }
    }
    return scale;
}

CollisionInstanceRef CollisionModel::Instatiate(Float3 const& scale)
{
    if (scale.X != 1 || scale.Y != 1 || scale.Z != 1)
    {
        bool bIsUniformXZ = scale.X == scale.Z;
        bool bIsUniformScaling = bIsUniformXZ && scale.X == scale.Y; // FIXME: allow some epsilon?
        
        if (m_AllowedScalingMode == CollisionModel::NON_UNIFORM || bIsUniformScaling)
        {
            return new JPH::ScaledShape(m_Shape, ConvertVector(scale));
        }
        else if (m_AllowedScalingMode == CollisionModel::UNIFORM_XZ)
        {
            if (!bIsUniformXZ)
            {
                LOG("WARNING: Non-uniform XZ scaling is not allowed for this collision model\n");
            }

            float scaleXZ = Math::Max(scale.X, scale.Z);

            return new JPH::ScaledShape(m_Shape, JPH::Vec3(scaleXZ, scale.Y, scaleXZ));
        }
        else
        {
            LOG("WARNING: Non-uniform scaling is not allowed for this collision model\n");

            return new JPH::ScaledShape(m_Shape, JPH::Vec3::sReplicate(Math::Max3(scale.X, scale.Y, scale.Z)));
        }
    }
    return m_Shape;
}

template <typename T, typename U>
T CheckedStaticCast(U u)
{
    return static_cast<T>(u);
//    static_assert(!std::is_same<T, U>::value, "Redundant CheckedStaticCast");
//#ifdef HK_DEBUG
//    if (!u) return nullptr;
//    T t = dynamic_cast<T>(u);
//    if (!t) HK_ASSERT(!"Invalid type cast"); // NOLINT(clang-diagnostic-string-conversion)
//    return t;
//#else
//    return static_cast<T>(u);
//#endif
}

namespace
{

void GatherGeometry(JPH::SphereShape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Float3x4 const& transform)
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

            *pVertices++ = transform * (Float3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta) * radius + position);
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

void GatherGeometry(JPH::BoxShape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
{
    unsigned int const faceIndices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

    int firstVertex = vertices.Size();
    int firstIndex = indices.Size();

    vertices.Resize(firstVertex + 8);
    indices.Resize(firstIndex + 36);

    Float3* pVertices = vertices.ToPtr() + firstVertex;
    unsigned int* pIndices = indices.ToPtr() + firstIndex;

    Float3 halfExtents = ConvertVector(shape->GetHalfExtent());

    *pVertices++ = transform * (rotation * Float3(-halfExtents.X, halfExtents.Y, -halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(halfExtents.X, halfExtents.Y, -halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(halfExtents.X, halfExtents.Y, halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(-halfExtents.X, halfExtents.Y, halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(-halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(halfExtents.X, -halfExtents.Y, halfExtents.Z) + position);
    *pVertices++ = transform * (rotation * Float3(-halfExtents.X, -halfExtents.Y, halfExtents.Z) + position);

    for (int i = 0; i < 36; i++)
    {
        *pIndices++ = firstVertex + faceIndices[i];
    }
}
    
void GatherGeometry(JPH::CylinderShape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
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

        *pVertices = transform * (rotation * vert + position);

        vert[1] = -vert[1];

        *(pVertices + numSlices) = transform * (rotation * vert + position);
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
    
void GatherGeometry(JPH::CapsuleShape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
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
            v = transform * (rotation * v + position);
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
            v = transform * (rotation * v + position);
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

void GatherGeometry(JPH::ConvexHullShape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
{
    int vertexCount = shape->mPoints.size();

    int indexCount = 0;
    for (const auto& f : shape->mFaces)
    {
        indexCount += (f.mNumVertices - 2) * 3;
    }

    int firstVertex = vertices.Size();
    int firstIndex = indices.Size();

    vertices.Resize(firstVertex + vertexCount);
    indices.Resize(firstIndex + indexCount);

    Float3* pVertices = vertices.ToPtr() + firstVertex;
    unsigned int* pIndices = indices.ToPtr() + firstIndex;

    for (int i = 0; i < vertexCount; i++)
    {
        pVertices[i] = transform * (rotation * ConvertVector(shape->mPoints[i].mPosition) + position);
    }

    for (const auto& f : shape->mFaces)
    {
        const JPH::uint8* indexData = shape->mVertexIdx.data() + f.mFirstVertex;

        for (int i = 0; i < f.mNumVertices - 2; i++)
        {
            pIndices[0] = firstVertex + indexData[0];
            pIndices[1] = firstVertex + indexData[i + 1];
            pIndices[2] = firstVertex + indexData[i + 2];
            pIndices += 3;
        }
    }
}

void GatherGeometry(JPH::MeshShape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
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
                m_Vertices.Add(m_Transform * (m_Rotation * ConvertVector(v[0]) + m_Position));
                m_Vertices.Add(m_Transform * (m_Rotation * ConvertVector(v[1]) + m_Position));
                m_Vertices.Add(m_Transform * (m_Rotation * ConvertVector(v[2]) + m_Position));

                m_Indices.Add(firstVertex + 0);
                m_Indices.Add(firstVertex + 1);
                m_Indices.Add(firstVertex + 2);

                firstVertex += 3;
            }
        }

        TVector<Float3>& m_Vertices;
        TVector<unsigned int>& m_Indices;
        Float3 const& m_Position;
        Quat const& m_Rotation;
        Float3x4 const& m_Transform;
    };

    Visitor visitor{vertices, indices, position, rotation, transform};

    const NodeCodec::Header* header = shape->mTree.Get<NodeCodec::Header>(0);
    NodeCodec::DecodingContext node_ctx(header);

    const TriangleCodec::DecodingContext triangle_ctx(shape->mTree.Get<TriangleCodec::TriangleHeader>(NodeCodec::HeaderSize));
    const JPH::uint8* buffer_start = &shape->mTree[0];
    node_ctx.WalkTree(buffer_start, triangle_ctx, visitor);
}

void GatherGeometrySimpleShape(JPH::Shape const* shape, TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
{
    switch (shape->GetSubType())
    {
        case JPH::EShapeSubType::Sphere:
            GatherGeometry(CheckedStaticCast<JPH::SphereShape const*>(shape), vertices, indices, position, transform);
            break;
        case JPH::EShapeSubType::Box:
            GatherGeometry(CheckedStaticCast<JPH::BoxShape const*>(shape), vertices, indices, position, rotation, transform);
            break;
        case JPH::EShapeSubType::Cylinder:
            GatherGeometry(CheckedStaticCast<JPH::CylinderShape const*>(shape), vertices, indices, position, rotation, transform);
            break;
        case JPH::EShapeSubType::Capsule:
            GatherGeometry(CheckedStaticCast<JPH::CapsuleShape const*>(shape), vertices, indices, position, rotation, transform);
            break;
        case JPH::EShapeSubType::ConvexHull:
            GatherGeometry(CheckedStaticCast<JPH::ConvexHullShape const*>(shape), vertices, indices, position, rotation, transform);
            break;
        case JPH::EShapeSubType::Mesh:
            GatherGeometry(CheckedStaticCast<JPH::MeshShape const*>(shape), vertices, indices, position, rotation, transform);
            break;
        default:
            HK_ASSERT(0);
            break;
    }
}

} // namespace
 //mPosition - mRotation * mShape->GetCenterOfMass()
//worldPosition + worldRotation*(m_CenterOfMassWithOffset* GetValidScale(worldScale));
void CollisionModel::GatherGeometry(TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale) const
{
    Float3 validScale = GetValidScale(worldScale);
    Float3x4 transform;

    JPH::Shape const* shape = m_Shape.GetPtr();

    auto centerOfMass = ConvertVector(shape->GetCenterOfMass());

    //if (shape->GetSubType() == JPH::EShapeSubType::OffsetCenterOfMass)
    //{
    //    JPH::OffsetCenterOfMassShape const* offsetShape = CheckedStaticCast<JPH::OffsetCenterOfMassShape const*>(shape);

    //    shape = offsetShape->GetInnerShape();
    //}

    
    //HK_UNUSED(centerOfMass);

    Float3 centerOfMassWorldPos = worldPosition + worldRotation * (centerOfMass * validScale);

    //Float3 centerOfMassWorldPos = worldPosition + worldRotation * (m_CenterOfMassWithOffset * validScale);

    transform.Compose(centerOfMassWorldPos, worldRotation.ToMatrix3x3(), validScale);

    if (shape->GetSubType() == JPH::EShapeSubType::StaticCompound)
    {
        JPH::StaticCompoundShape const* compoundShape = CheckedStaticCast<JPH::StaticCompoundShape const*>(shape);

        JPH::StaticCompoundShape::SubShapes const& subShapes = compoundShape->GetSubShapes();
        for (JPH::StaticCompoundShape::SubShape const& subShape : subShapes)
        {
            auto position = ConvertVector(subShape.GetPositionCOM()); // FIXME: Is it correct?
            auto rotation = ConvertQuaternion(subShape.GetRotation());

            JPH::Shape const* innerShape = subShape.mShape;

            GatherGeometrySimpleShape(innerShape, vertices, indices, position, rotation, transform);
        }
    }
    else if (shape->GetSubType() == JPH::EShapeSubType::RotatedTranslated)
    {
        JPH::RotatedTranslatedShape const* transformedShape = CheckedStaticCast<JPH::RotatedTranslatedShape const*>(shape);

        JPH::Shape const* innerShape = transformedShape->GetInnerShape();

        GatherGeometrySimpleShape(innerShape, vertices, indices, ConvertVector(transformedShape->GetPosition()), ConvertQuaternion(transformedShape->GetRotation()), transform);
    }
    else
    {
        GatherGeometrySimpleShape(shape, vertices, indices, Float3::Zero(), Quat::Identity(), transform);
    }
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
    // TODO: check

    TSmallVector<Float3, 32> verts;

    for (auto& face : shape->mFaces)
    {
        verts.Clear();
        for (uint16_t v = 0; v < face.mNumVertices; v++)
        {
            uint16_t index = shape->mFaces[0].mFirstVertex + v;

            verts.Add(ConvertVector(shape->mPoints[index].mPosition));
        }

        renderer.DrawLine(verts, true);
    }
}

void DrawMesh(DebugRenderer& renderer, JPH::MeshShape const* shape)
{
    // TODO
}

void DrawShape(DebugRenderer& renderer, JPH::Shape const* shape, Float3 const& position, Quat const& rotation, Float3x4 const& transform)
{
    Float3x4 t;
    t.Compose(position, rotation.ToMatrix3x3());

    t = transform * t;

    renderer.PushTransform(t);

    switch (shape->GetSubType())
    {
        case JPH::EShapeSubType::Sphere:
            DrawSphere(renderer, CheckedStaticCast<JPH::SphereShape const*>(shape));
            break;
        case JPH::EShapeSubType::Box:
            DrawBox(renderer, CheckedStaticCast<JPH::BoxShape const*>(shape));
            break;
        case JPH::EShapeSubType::Cylinder:
            DrawCylinder(renderer, CheckedStaticCast<JPH::CylinderShape const*>(shape));
            break;
        case JPH::EShapeSubType::Capsule:
            DrawCapsule(renderer, CheckedStaticCast<JPH::CapsuleShape const*>(shape));
            break;
        case JPH::EShapeSubType::ConvexHull:
            DrawConvexHull(renderer, CheckedStaticCast<JPH::ConvexHullShape const*>(shape));
            break;
        case JPH::EShapeSubType::Mesh:
            DrawMesh(renderer, CheckedStaticCast<JPH::MeshShape const*>(shape));
            break;
        default:
            HK_ASSERT(0);
            break;
    }

    renderer.PopTransform();
}

} // namespace

void CollisionModel::DrawDebug(DebugRenderer& renderer, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale) const
{
    Float3 validScale = GetValidScale(worldScale);
    Float3x4 transform;

    JPH::Shape const* shape = m_Shape.GetPtr();

    auto centerOfMass = ConvertVector(shape->GetCenterOfMass());

    Float3 centerOfMassWorldPos = worldPosition + worldRotation * (centerOfMass * validScale);

    transform.Compose(centerOfMassWorldPos, worldRotation.ToMatrix3x3(), validScale);

    if (shape->GetSubType() == JPH::EShapeSubType::StaticCompound)
    {
        JPH::StaticCompoundShape const* compoundShape = CheckedStaticCast<JPH::StaticCompoundShape const*>(shape);

        JPH::StaticCompoundShape::SubShapes const& subShapes = compoundShape->GetSubShapes();
        for (JPH::StaticCompoundShape::SubShape const& subShape : subShapes)
        {
            auto position = ConvertVector(subShape.GetPositionCOM()); // FIXME: Is it correct?
            auto rotation = ConvertQuaternion(subShape.GetRotation());

            JPH::Shape const* innerShape = subShape.mShape;

            DrawShape(renderer, innerShape, position, rotation, transform);
        }
    }
    else if (shape->GetSubType() == JPH::EShapeSubType::RotatedTranslated)
    {
        JPH::RotatedTranslatedShape const* transformedShape = CheckedStaticCast<JPH::RotatedTranslatedShape const*>(shape);

        JPH::Shape const* innerShape = transformedShape->GetInnerShape();

        DrawShape(renderer, innerShape, ConvertVector(transformedShape->GetPosition()), ConvertQuaternion(transformedShape->GetRotation()), transform);
    }
    else
    {
        DrawShape(renderer, shape, Float3::Zero(), Quat::Identity(), transform);
    }
}

CollisionModel* CreateConvexDecomposition(Float3 const* vertices,
                                          int vertexCount,
                                          int vertexStride,
                                          unsigned int const* indices,
                                          int indexCount)
{
    TVector<Float3> hullVertices;
    TVector<unsigned int> hullIndices;
    TVector<ConvexHullDesc> hulls;

    if (vertexStride <= 0)
    {
        LOG("CreateConvexDecomposition: invalid VertexStride\n");
        return {};
    }

    Geometry::PerformConvexDecomposition(vertices,
                                         vertexCount,
                                         vertexStride,
                                         indices,
                                         indexCount,
                                         hullVertices,
                                         hullIndices,
                                         hulls);

    if (hulls.IsEmpty())
    {
        LOG("CreateConvexDecomposition: failed on convex decomposition\n");
        return {};
    }

    TVector<CollisionConvexHullDef> hullDefs;
    hullDefs.Reserve(hulls.Size());

    for (ConvexHullDesc const& hull : hulls)
    {
        CollisionConvexHullDef& hulldef = hullDefs.Add();

        hulldef.Position = hull.Centroid;
        hulldef.pVertices = hullVertices.ToPtr() + hull.FirstVertex;
        hulldef.VertexCount = hull.VertexCount;
        //hulldef.pIndices = hullIndices.ToPtr() + hull.FirstIndex;
        //hulldef.IndexCount = hull.IndexCount;
    }

    CollisionModelCreateInfo createInfo;
    createInfo.pConvexHulls = hullDefs.ToPtr();
    createInfo.ConvexHullCount = hullDefs.Size();

    return CollisionModel::Create(createInfo);
}

CollisionModel* CreateConvexDecompositionVHACD(Float3 const* vertices,
                                               int vertexCount,
                                               int vertexStride,
                                               unsigned int const* indices,
                                               int indexCount)
{
    TVector<Float3> hullVertices;
    TVector<unsigned int> hullIndices;
    TVector<ConvexHullDesc> hulls;
    Float3 decompositionCenterOfMass;

    if (vertexStride <= 0)
    {
        LOG("CreateConvexDecompositionVHACD: invalid VertexStride\n");
        return {};
    }

    Geometry::PerformConvexDecompositionVHACD(vertices,
                                              vertexCount,
                                              vertexStride,
                                              indices,
                                              indexCount,
                                              hullVertices,
                                              hullIndices,
                                              hulls,
                                              decompositionCenterOfMass);

    if (hulls.IsEmpty())
    {
        LOG("CreateConvexDecompositionVHACD: failed on convex decomposition\n");
        return {};
    }

    TVector<CollisionConvexHullDef> hullDefs;
    hullDefs.Reserve(hulls.Size());

    for (ConvexHullDesc const& hull : hulls)
    {
        CollisionConvexHullDef& hulldef = hullDefs.Add();

        hulldef.Position = hull.Centroid;
        hulldef.pVertices = hullVertices.ToPtr() + hull.FirstVertex;
        hulldef.VertexCount = hull.VertexCount;
        //hulldef.pIndices = hullIndices.ToPtr() + hull.FirstIndex;
        //hulldef.IndexCount = hull.IndexCount;
    }

    CollisionModelCreateInfo createInfo;
    createInfo.pConvexHulls = hullDefs.ToPtr();
    createInfo.ConvexHullCount = hullDefs.Size();

    return CollisionModel::Create(createInfo);
}

HK_NAMESPACE_END
