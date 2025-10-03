/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "HeightFieldComponent.h"
#include <Hork/Runtime/World/Modules/Physics/PhysicsInterfaceImpl.h>

#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

HK_NAMESPACE_BEGIN

void HeightFieldComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    GameObject* owner = GetOwner();

    m_UserData = physics->CreateUserData();
    m_UserData->Initialize(this);

    MeshCollisionDataInternal const* data = Data ? Data->GetData() : nullptr;    
    if (data && data->m_Shape)
    {
        JPH::BodyCreationSettings settings;
        settings.SetShape(data->m_Shape);
        settings.mPosition = ConvertVector(owner->GetWorldPosition());
        settings.mRotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());
        settings.mUserData = (size_t)m_UserData;
        settings.mObjectLayer = MakeObjectLayer(CollisionLayer, BroadphaseLayer::Static);
        settings.mMotionType = JPH::EMotionType::Static;
        settings.mAllowDynamicOrKinematic = false;
        settings.mIsSensor = false;
        //settings.mFriction = Material.Friction;
        //settings.mRestitution = Material.Restitution;

        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        JPH::Body* body = bodyInterface.CreateBody(settings);
        m_BodyID = PhysBodyID(body->GetID().GetIndexAndSequenceNumber());

        physics->QueueToAdd(body, true);
    }
}

void HeightFieldComponent::EndPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();

    JPH::BodyID bodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        if (bodyInterface.IsAdded(bodyID))
            bodyInterface.RemoveBody(bodyID);

        bodyInterface.DestroyBody(bodyID);
        m_BodyID.ID = JPH::BodyID::cInvalidBodyID;
    }

    physics->DeleteUserData(m_UserData);
    m_UserData = nullptr;
}

void HeightFieldComponent::GatherGeometry(BvAxisAlignedBox const& cropBox, Vector<Float3>& vertices, Vector<uint32_t>& indices)
{
    if (!Data)
        return;

    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    JPH::Vec3 position;
    JPH::Quat rotation;
    bodyInterface.GetPositionAndRotation(JPH::BodyID(m_BodyID.ID), position, rotation);

    Float3x4 transformMatrix;
    transformMatrix.Compose(ConvertVector(position), ConvertQuaternion(rotation).ToMatrix3x3());

    Float3x4 transformMatrixInv = transformMatrix.Inversed();

    BvAxisAlignedBox localClip = cropBox.Transform(transformMatrixInv);

    auto firstVert = vertices.Size();
    Data->GatherGeometry(localClip, vertices, indices);

    if (firstVert != vertices.Size())
        TransformVertices(&vertices[firstVert], vertices.Size() - firstVert, transformMatrix);
}

TerrainCollisionData::TerrainCollisionData() :
    m_Data(MakeUnique<MeshCollisionDataInternal>())
{}

void TerrainCollisionData::Create(const float* inSamples, uint32_t inSampleCount/*, const uint8_t* inMaterialIndices, const JPH::PhysicsMaterialList& inMaterialList*/)
{
    const int BLOCK_SIZE_SHIFT = 2;
    const int BITS_PER_SAMPLE = 8;

    HK_ASSERT(IsPowerOfTwo(inSampleCount) && (inSampleCount % (1 << BLOCK_SIZE_SHIFT)) == 0);

    const float CELL_SIZE = 1;

    JPH::Vec3 terrainOffset = JPH::Vec3(-0.5f * CELL_SIZE * inSampleCount, 0, -0.5f * CELL_SIZE * inSampleCount);
    JPH::Vec3 terrainScale = JPH::Vec3(CELL_SIZE, 1.0f, CELL_SIZE);

    JPH::HeightFieldShapeSettings settings(inSamples, terrainOffset, terrainScale, inSampleCount, /*inMaterialIndices*/nullptr, /*inMaterialList*/{});
    settings.mBlockSize = 1 << BLOCK_SIZE_SHIFT;
    settings.mBitsPerSample = BITS_PER_SAMPLE;

    m_Data->m_Shape = settings.Create().Get().GetPtr();

    LOG("TerrainCollisionData memory usage {} bytes\n", GetMemoryUsage());
}

Float3 TerrainCollisionData::GetPosition(uint32_t inX, uint32_t inY) const
{
    if (!m_Data->m_Shape)
        return Float3(0);

    return ConvertVector(static_cast<JPH::HeightFieldShape*>(m_Data->m_Shape.GetPtr())->GetPosition(inX, inY));
}

bool TerrainCollisionData::IsNoCollision(uint32_t inX, uint32_t inY) const
{
    if (!m_Data->m_Shape)
        return true;

    return static_cast<JPH::HeightFieldShape*>(m_Data->m_Shape.GetPtr())->IsNoCollision(inX, inY);
}

bool TerrainCollisionData::ProjectOntoSurface(Float3 const& inLocalPosition, Float3& outSurfacePosition, Float3& outSurfaceNormal) const
{
    JPH::Vec3 surface_position;
    JPH::SubShapeID sub_shape_id;

    if (!m_Data->m_Shape)
        return false;

    bool result = static_cast<JPH::HeightFieldShape*>(m_Data->m_Shape.GetPtr())->ProjectOntoSurface(ConvertVector(inLocalPosition), surface_position, sub_shape_id);
    if (!result)
        return false;

    outSurfacePosition = ConvertVector(surface_position);
    outSurfaceNormal = ConvertVector(static_cast<JPH::HeightFieldShape*>(m_Data->m_Shape.GetPtr())->GetSurfaceNormal(sub_shape_id, surface_position));

    return true;
}

size_t TerrainCollisionData::GetMemoryUsage() const
{
    if (!m_Data->m_Shape)
        return 0;
    return static_cast<JPH::HeightFieldShape*>(m_Data->m_Shape.GetPtr())->GetStats().mSizeBytes;
}

void TerrainCollisionData::GatherGeometry(BvAxisAlignedBox const& inLocalBounds, Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const
{
    if (!m_Data->m_Shape)
        return;

    int first_vertex = outVertices.Size();

    JPH::AABox box;
    box.mMin = ConvertVector(inLocalBounds.Mins);
    box.mMax = ConvertVector(inLocalBounds.Maxs);

    static JPH::Shape::GetTrianglesContext context;

    constexpr int cMaxTriangles = 1000;
    static JPH::Float3 vertices[3 * cMaxTriangles];

    // Start iterating all triangles of the shape
    JPH::HeightFieldShape* heightField = static_cast<JPH::HeightFieldShape*>(m_Data->m_Shape.GetPtr());
    heightField->GetTrianglesStart(context, box, JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sReplicate(1.0f));
    for (;;)
    {
        // Get the next batch of vertices
        int triangle_count = heightField->GetTrianglesNext(context, cMaxTriangles, vertices);
        if (triangle_count == 0)
            break;

        // Allocate space for triangles
        size_t output_index = outIndices.Size();
        outIndices.Resize(output_index + triangle_count * 3);
        unsigned int* indices = &outIndices[output_index];

        for (int vertex = 0, vertex_max = 3 * triangle_count; vertex < vertex_max; vertex += 3, indices += 3, first_vertex += 3)
        {
            outVertices.EmplaceBack(vertices[vertex    ].x, vertices[vertex    ].y, vertices[vertex    ].z);
            outVertices.EmplaceBack(vertices[vertex + 1].x, vertices[vertex + 1].y, vertices[vertex + 1].z);
            outVertices.EmplaceBack(vertices[vertex + 2].x, vertices[vertex + 2].y, vertices[vertex + 2].z);

            indices[0] = first_vertex;
            indices[1] = first_vertex + 1;
            indices[2] = first_vertex + 2;
        }
    }
}

HK_NAMESPACE_END
