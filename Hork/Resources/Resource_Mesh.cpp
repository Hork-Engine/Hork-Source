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

#include "Resource_Mesh.h"
#include "Implementation/OzzIO.h"

#include <Hork/Core/ReadWriteBuffer.h>
#include <Hork/Geometry/BV/BvIntersect.h>
#include <Hork/Geometry/TangentSpace.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

HK_NAMESPACE_BEGIN

VertexMemoryGPU* MeshResource::s_VertexMemory = nullptr;

void MeshResource::SetVertexMemoryGPU(VertexMemoryGPU* vertexMemory)
{
    s_VertexMemory = vertexMemory;
}

MeshResource::~MeshResource()
{
    if (m_VertexHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");

        s_VertexMemory->Deallocate(m_VertexHandle);
        s_VertexMemory->Deallocate(m_SkinBufferHandle);
        s_VertexMemory->Deallocate(m_LightmapUVsGPU);
        s_VertexMemory->Deallocate(m_IndexHandle);
    }
}

int16_t MeshResource::FindJoint(StringView name) const
{
    if (!m_Skeleton)
        return -1;

    int16_t jointIndex = 0;
    for (const char* joint : m_Skeleton->joint_names())
    {
        if (!name.Cmp(joint))
            return jointIndex;
        ++jointIndex;
    }
    return -1;
}

uint16_t MeshResource::GetJointCount() const
{
    return m_Skeleton ? m_Skeleton->num_joints() : 0;
}

const char* MeshResource::GetJointName(uint16_t jointIndex) const
{
    return m_Skeleton ? m_Skeleton->joint_names()[jointIndex] : "";
}

int16_t MeshResource::GetJointParent(uint16_t jointIndex) const
{
    return m_Skeleton ? m_Skeleton->joint_parents()[jointIndex] : -1;
}

void MeshResource::Clear()
{
    m_Surfaces.Clear();
    m_Skins.Clear();
    m_JointRemaps.Clear();
    m_InverseBindPoses.Clear();
    m_Skeleton.Reset();
    m_Vertices.Clear();
    m_SkinBuffer.Clear();
    m_LightmapUVs.Clear();
    m_Indices.Clear();
    m_BoundingBox.Clear();
}

UniqueRef<MeshResource> MeshResource::sLoad(IBinaryStreamReadInterface& stream)
{
    StringView extension = PathUtils::sGetExt(stream.GetName());

    if (!extension.Icmp(".gltf") || !extension.Icmp(".glb") || !extension.Icmp(".fbx") || !extension.Icmp(".obj"))
    {
        RawMesh mesh;
        RawMeshLoadFlags flags = RawMeshLoadFlags::Surfaces | RawMeshLoadFlags::Skins | RawMeshLoadFlags::Skeleton;

        bool result;
        if (!extension.Icmp(".fbx"))
            result = mesh.LoadFBX(stream, flags);
        else if (!extension.Icmp(".obj"))
            result = mesh.LoadOBJ(stream, flags);
        else
            result = mesh.LoadGLTF(stream, flags);

        if (!result)
            return {};

        return MeshResourceBuilder().Build(mesh);
    }

    UniqueRef<MeshResource> resource = MakeUnique<MeshResource>();
    if (!resource->Read(stream))
        return {};
    return resource;
}

namespace
{

    void ReadInverseBindPoses(IBinaryStreamReadInterface& stream, Vector<SimdFloat4x4>& v)
    {
        uint32_t arraySize = stream.ReadUInt32();

        v.Clear();
        v.Reserve(arraySize);

        Float3x4 inverseBindPose;
        for (uint32_t n = 0; n < arraySize; ++n)
        {
            stream.ReadObject(inverseBindPose);
            alignas(16) Float4x4 source(inverseBindPose);
            source.TransposeSelf();
            Simd::LoadFloat4x4(source, v.EmplaceBack().cols);
        }
    }

    void WriteInverseBindPoses(IBinaryStreamWriteInterface& stream, Vector<SimdFloat4x4> const& v)
    {
        stream.WriteUInt32(v.Size());

        alignas(16) Float4x4 inverseBindPose;
        for (uint32_t n = 0, arraySize = v.Size(); n < arraySize; ++n)
        {
            Simd::StoreFloat4x4(v[n].cols, inverseBindPose);
            stream.WriteObject(Float3x4(inverseBindPose.Transposed()));
        }
    }
}

void MeshSurface::Read(IBinaryStreamReadInterface& stream)
{
    BaseVertex = stream.ReadUInt32();
    FirstIndex = stream.ReadUInt32();
    VertexCount = stream.ReadUInt32();
    IndexCount = stream.ReadUInt32();
    SkinIndex = stream.ReadInt16();
    JointIndex = stream.ReadUInt16();

    alignas(16) Float4x4 m;
    stream.ReadObject(m);
    Simd::LoadFloat4x4(m, InverseTransform.cols);

    stream.ReadObject(BoundingBox);
    stream.ReadObject(Bvh);
}

void MeshSurface::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt32(BaseVertex);
    stream.WriteUInt32(FirstIndex);
    stream.WriteUInt32(VertexCount);
    stream.WriteUInt32(IndexCount);
    stream.WriteInt16(SkinIndex);
    stream.WriteUInt16(JointIndex);

    alignas(16) Float4x4 m;
    Simd::StoreFloat4x4(InverseTransform.cols, m);
    stream.WriteObject(m);

    stream.WriteObject(BoundingBox);
    stream.WriteObject(Bvh);
}

void MeshSkin::Read(IBinaryStreamReadInterface& stream)
{
    FirstMatrix = stream.ReadUInt16();
    MatrixCount = stream.ReadUInt16();
}

void MeshSkin::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt16(FirstMatrix);
    stream.WriteUInt16(MatrixCount);
}

bool MeshResource::Read(IBinaryStreamReadInterface& stream)
{   
    Clear();

    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    stream.ReadArray(m_Surfaces);
    stream.ReadArray(m_Skins);
    stream.ReadArray(m_JointRemaps);
    ReadInverseBindPoses(stream, m_InverseBindPoses);

    m_Skeleton = OzzReadSkeleton(stream);

    stream.ReadArray(m_Vertices);
    stream.ReadArray(m_SkinBuffer);
    stream.ReadArray(m_LightmapUVs);
    stream.ReadArray(m_Indices);
    stream.ReadObject(m_BoundingBox);

    stream.ReadArray(m_Vertices);
    stream.ReadArray(m_SkinBuffer);
    stream.ReadArray(m_LightmapUVs);
    stream.ReadArray(m_Indices);
    stream.ReadArray(m_Surfaces);
      
    return true;
}

void MeshResource::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt32(MakeResourceMagic(Type, Version));

    stream.WriteArray(m_Surfaces);
    stream.WriteArray(m_Skins);
    stream.WriteArray(m_JointRemaps);
    WriteInverseBindPoses(stream, m_InverseBindPoses);
    
    OzzWriteSkeleton(stream, m_Skeleton.RawPtr());

    stream.WriteArray(m_Vertices);
    stream.WriteArray(m_SkinBuffer);
    stream.WriteArray(m_LightmapUVs);
    stream.WriteArray(m_Indices);
    stream.WriteObject(m_BoundingBox);

    stream.WriteArray(m_Vertices);
    stream.WriteArray(m_SkinBuffer);
    stream.WriteArray(m_LightmapUVs);
    stream.WriteArray(m_Indices);
    stream.WriteArray(m_Surfaces);
}

void* MeshResource::sGetVertexMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_Vertices.ToPtr();
}

void* MeshResource::sGetSkinMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_SkinBuffer.ToPtr();
}

void* MeshResource::sGetLightmapUVMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_LightmapUVs.ToPtr();
}

void* MeshResource::sGetIndexMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_Indices.ToPtr();
}

void MeshResource::GetVertexBufferGPU(RHI::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_VertexHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->GetPhysicalBufferAndOffset(m_VertexHandle, ppBuffer, pOffset);
    }
    else
    {
        *ppBuffer = nullptr;
        *pOffset = 0;
    }
}

void MeshResource::GetSkinBufferBufferGPU(RHI::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_SkinBufferHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->GetPhysicalBufferAndOffset(m_SkinBufferHandle, ppBuffer, pOffset);
    }
    else
    {
        *ppBuffer = nullptr;
        *pOffset = 0;
    }
}

void MeshResource::GetLightmapUVsGPU(RHI::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_LightmapUVsGPU)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->GetPhysicalBufferAndOffset(m_LightmapUVsGPU, ppBuffer, pOffset);
    }
    else
    {
        *ppBuffer = nullptr;
        *pOffset = 0;
    }
}

void MeshResource::GetIndexBufferGPU(RHI::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_IndexHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->GetPhysicalBufferAndOffset(m_IndexHandle, ppBuffer, pOffset);
    }
    else
    {
        *ppBuffer = nullptr;
        *pOffset = 0;
    }
}

void MeshResource::Allocate(MeshAllocateDesc const& desc)
{
    Clear();

    m_Vertices.Resize(desc.VertexCount);
    m_Indices.Resize(desc.IndexCount);

    if (desc.SkinsCount)
        m_SkinBuffer.Resize(desc.VertexCount);
    else
        m_SkinBuffer.Clear();

    if (desc.HasLightmapChannel)
        m_LightmapUVs.Resize(desc.VertexCount);
    else
        m_LightmapUVs.Clear();

    uint32_t surfaceCount = desc.SurfaceCount < 1 ? 1 : desc.SurfaceCount;
    m_Surfaces.Resize(surfaceCount);
    if (surfaceCount == 1)
    {
        MeshSurface& surface = m_Surfaces[0];
        surface.BaseVertex = 0;
        surface.FirstIndex = 0;
        surface.VertexCount = m_Vertices.Size();
        surface.IndexCount = m_Indices.Size();
    }

    m_Skins.Resize(desc.SkinsCount);
    m_JointRemaps.Resize(desc.JointReampSize);
    m_InverseBindPoses.Resize(desc.JointReampSize);
    //m_Skeleton = .... TODO: allocate with desc.JointCount

    m_Surfaces.ShrinkToFit();
    m_Skins.ShrinkToFit();
    m_JointRemaps.ShrinkToFit();
    m_InverseBindPoses.ShrinkToFit();
    m_Vertices.ShrinkToFit();
    m_SkinBuffer.ShrinkToFit();
    m_LightmapUVs.ShrinkToFit();

    HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
    s_VertexMemory->Deallocate(m_VertexHandle);
    s_VertexMemory->Deallocate(m_SkinBufferHandle);
    s_VertexMemory->Deallocate(m_LightmapUVsGPU);
    s_VertexMemory->Deallocate(m_IndexHandle);

    m_VertexHandle = s_VertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertex), nullptr, sGetVertexMemory, this);
    m_IndexHandle  = s_VertexMemory->AllocateIndex(m_Indices.Size() * sizeof(unsigned int), nullptr, sGetIndexMemory, this);
    if (desc.SkinsCount)
        m_SkinBufferHandle = s_VertexMemory->AllocateVertex(m_SkinBuffer.Size() * sizeof(SkinVertex), nullptr, sGetSkinMemory, this);
    else
        m_SkinBufferHandle = nullptr;
    if (desc.HasLightmapChannel)
        m_LightmapUVsGPU = s_VertexMemory->AllocateVertex(m_LightmapUVs.Size() * sizeof(MeshVertexUV), nullptr, sGetLightmapUVMemory, this);
    else
        m_LightmapUVsGPU = nullptr;
}

bool MeshResource::WriteVertexData(MeshVertex const* vertices, int vertexCount, int startVertexLocation)
{
    if (!vertexCount)
    {
        return true;
    }

    if (startVertexLocation + vertexCount > m_Vertices.Size())
    {
        LOG("MeshResource::WriteVertexData: Referencing outside of buffer\n");
        return false;
    }

    Core::Memcpy(m_Vertices.ToPtr() + startVertexLocation, vertices, vertexCount * sizeof(MeshVertex));

    if (m_VertexHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->Update(m_VertexHandle, startVertexLocation * sizeof(MeshVertex), vertexCount * sizeof(MeshVertex), m_Vertices.ToPtr() + startVertexLocation);
    }

    return true;
}

bool MeshResource::WriteSkinningData(SkinVertex const* vertices, int vertexCount, int startVertexLocation)
{
    if (!m_SkinBufferHandle)
    {
        LOG("MeshResource::WriteSkinningData: Cannot write skinning data for static mesh\n");
        return false;
    }

    if (!vertexCount)
    {
        return true;
    }

    if (startVertexLocation + vertexCount > m_SkinBuffer.Size())
    {
        LOG("MeshResource::WriteSkinningData: Referencing outside of buffer\n");
        return false;
    }

    Core::Memcpy(m_SkinBuffer.ToPtr() + startVertexLocation, vertices, vertexCount * sizeof(SkinVertex));

    if (m_SkinBufferHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->Update(m_SkinBufferHandle, startVertexLocation * sizeof(SkinVertex), vertexCount * sizeof(SkinVertex), m_SkinBuffer.ToPtr() + startVertexLocation);
    }

    return true;
}

bool MeshResource::WriteLightmapUVsData(MeshVertexUV const* UVs, int vertexCount, int startVertexLocation)
{
    if (!vertexCount)
    {
        return true;
    }

    if (startVertexLocation + vertexCount > m_Vertices.Size())
    {
        LOG("MeshResource::WriteLightmapUVsData: Referencing outside of buffer\n");
        return false;
    }

    AddLightmapUVs();

    Core::Memcpy(m_LightmapUVs.ToPtr() + startVertexLocation, UVs, vertexCount * sizeof(MeshVertexUV));

    if (m_LightmapUVsGPU)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->Update(m_LightmapUVsGPU, startVertexLocation * sizeof(MeshVertexUV), vertexCount * sizeof(MeshVertexUV), m_LightmapUVs.ToPtr() + startVertexLocation);
    }

    return true;
}

bool MeshResource::WriteIndexData(unsigned int const* indices, int indexCount, int startIndexLocation)
{
    if (!indexCount)
    {
        return true;
    }

    if (startIndexLocation + indexCount > m_Indices.Size())
    {
        LOG("MeshResource::WriteIndexData: Referencing outside of buffer\n");
        return false;
    }

    Core::Memcpy(m_Indices.ToPtr() + startIndexLocation, indices, indexCount * sizeof(unsigned int));

    if (m_IndexHandle)
    {
        HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
        s_VertexMemory->Update(m_IndexHandle, startIndexLocation * sizeof(unsigned int), indexCount * sizeof(unsigned int), m_Indices.ToPtr() + startIndexLocation);
    }

    return true;
}

void MeshResource::Upload(RHI::IDevice* device)
{
    HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");
    s_VertexMemory->Deallocate(m_VertexHandle);
    s_VertexMemory->Deallocate(m_SkinBufferHandle);
    s_VertexMemory->Deallocate(m_LightmapUVsGPU);
    s_VertexMemory->Deallocate(m_IndexHandle);

    m_VertexHandle = s_VertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertex), m_Vertices.ToPtr(), sGetVertexMemory, this);
    m_IndexHandle = s_VertexMemory->AllocateIndex(m_Indices.Size() * sizeof(unsigned int), m_Indices.ToPtr(), sGetIndexMemory, this);

    if (!m_SkinBuffer.IsEmpty())
        m_SkinBufferHandle = s_VertexMemory->AllocateVertex(m_SkinBuffer.Size() * sizeof(SkinVertex), m_SkinBuffer.ToPtr(), sGetSkinMemory, this);
    else
        m_SkinBufferHandle = nullptr;

    if (!m_LightmapUVs.IsEmpty())
        m_LightmapUVsGPU = s_VertexMemory->AllocateVertex(m_LightmapUVs.Size() * sizeof(MeshVertexUV), m_LightmapUVs.ToPtr(), sGetLightmapUVMemory, this);
    else
        m_LightmapUVsGPU = nullptr;
}

void MeshResource::AddLightmapUVs()
{
    if (m_LightmapUVsGPU && m_LightmapUVs.Size() == m_Vertices.Size())
        return;

    HK_ASSERT_(s_VertexMemory, "The vertex memory allocator must be set! Use MeshResource::SetVertexMemoryGPU");

    if (m_LightmapUVsGPU)
        s_VertexMemory->Deallocate(m_LightmapUVsGPU);

    m_LightmapUVsGPU = s_VertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertexUV), nullptr, sGetLightmapUVMemory, this);
    m_LightmapUVs.Resize(m_Vertices.Size());
}

void MeshResource::SetBoundingBox(BvAxisAlignedBox const& boundingBox)
{
    m_BoundingBox = boundingBox;
}

void MeshResource::GenerateBVH(uint16_t trianglesPerLeaf)
{
    const uint16_t MaxTrianglesPerLeaf = 1024;
    if (trianglesPerLeaf > MaxTrianglesPerLeaf)
        trianglesPerLeaf = MaxTrianglesPerLeaf;

    for (MeshSurface& surface : m_Surfaces)
        surface.Bvh = BvhTree(ArrayView<MeshVertex>(m_Vertices), ArrayView<unsigned int>(m_Indices.ToPtr() + surface.FirstIndex, (size_t)surface.IndexCount), surface.BaseVertex, trianglesPerLeaf);
}

bool MeshResource::Raycast(int surfaceIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool bCullBackFace, Vector<TriangleHitResult>& hitResult) const
{
    bool ret = false;
    float d, u, v;
    MeshSurface const& surface = m_Surfaces[surfaceIndex];
    unsigned int const* indices = m_Indices.ToPtr() + surface.FirstIndex;
    MeshVertex const* vertices = m_Vertices.ToPtr();

    if (distance < 0.0001f)
    {
        return false;
    }

    if (!surface.Bvh.GetNodes().IsEmpty())
    {
        Vector<BvhNode> const& nodes = surface.Bvh.GetNodes();
        unsigned int const* indirection = surface.Bvh.GetIndirection();

        float hitMin, hitMax;

        for (int nodeIndex = 0, numNodes = nodes.Size(); nodeIndex < numNodes;)
        {
            BvhNode const* node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox(rayStart, invRayDir, node->Bounds, hitMin, hitMax) && hitMin <= distance;
            const bool bLeaf = node->IsLeaf();

            if (bLeaf && bOverlap)
            {
                for (int t = 0; t < node->PrimitiveCount; t++)
                {
                    const int triangleNum = node->Index + t;
                    const unsigned int baseInd = indirection[triangleNum];
                    const unsigned int i0 = surface.BaseVertex + indices[baseInd + 0];
                    const unsigned int i1 = surface.BaseVertex + indices[baseInd + 1];
                    const unsigned int i2 = surface.BaseVertex + indices[baseInd + 2];
                    Float3 const& v0 = vertices[i0].Position;
                    Float3 const& v1 = vertices[i1].Position;
                    Float3 const& v2 = vertices[i2].Position;
                    if (BvRayIntersectTriangle(rayStart, rayDir, v0, v1, v2, d, u, v, bCullBackFace))
                    {
                        if (distance > d)
                        {
                            TriangleHitResult& hit = hitResult.Add();
                            hit.Location = rayStart + rayDir * d;
                            hit.Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                            hit.Distance = d;
                            hit.UV.X = u;
                            hit.UV.Y = v;
                            hit.Indices[0] = i0;
                            hit.Indices[1] = i1;
                            hit.Indices[2] = i2;
                            //hit.Material = nullptr; //m_MaterialInstance; // FIXME
                            ret = true;
                        }
                    }
                }
            }

            nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
        }
    }
    else
    {
        float hitMin, hitMax;

        if (!BvRayIntersectBox(rayStart, invRayDir, surface.BoundingBox, hitMin, hitMax) || hitMin >= distance)
        {
            return false;
        }

        const int primCount = surface.IndexCount / 3;

        for (int tri = 0; tri < primCount; tri++, indices += 3)
        {
            const unsigned int i0 = surface.BaseVertex + indices[0];
            const unsigned int i1 = surface.BaseVertex + indices[1];
            const unsigned int i2 = surface.BaseVertex + indices[2];

            Float3 const& v0 = vertices[i0].Position;
            Float3 const& v1 = vertices[i1].Position;
            Float3 const& v2 = vertices[i2].Position;

            if (BvRayIntersectTriangle(rayStart, rayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (distance > d)
                {
                    TriangleHitResult& hit = hitResult.Add();
                    hit.Location = rayStart + rayDir * d;
                    hit.Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    hit.Distance = d;
                    hit.UV.X = u;
                    hit.UV.Y = v;
                    hit.Indices[0] = i0;
                    hit.Indices[1] = i1;
                    hit.Indices[2] = i2;
                    //hit.Material = nullptr; //m_MaterialInstance; // FIXME
                    ret = true;
                }
            }
        }
    }
    return ret;
}

bool MeshResource::RaycastClosest(int surfaceIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool bCullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3]) const
{
    bool ret = false;
    float d, u, v;
    MeshSurface const& surface = m_Surfaces[surfaceIndex];
    unsigned int const* indices = m_Indices.ToPtr() + surface.FirstIndex;
    MeshVertex const* vertices = m_Vertices.ToPtr();

    if (distance < 0.0001f)
    {
        return false;
    }

    if (!surface.Bvh.GetNodes().IsEmpty())
    {
        Vector<BvhNode> const& nodes = surface.Bvh.GetNodes();
        unsigned int const* indirection = surface.Bvh.GetIndirection();

        float hitMin, hitMax;

        for (int nodeIndex = 0, numNodes = nodes.Size(); nodeIndex < numNodes;)
        {
            BvhNode const* node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox(rayStart, invRayDir, node->Bounds, hitMin, hitMax) && hitMin <= distance;
            const bool bLeaf = node->IsLeaf();

            if (bLeaf && bOverlap)
            {
                for (int t = 0; t < node->PrimitiveCount; t++)
                {
                    const int triangleNum = node->Index + t;
                    const unsigned int baseInd = indirection[triangleNum];
                    const unsigned int i0 = surface.BaseVertex + indices[baseInd + 0];
                    const unsigned int i1 = surface.BaseVertex + indices[baseInd + 1];
                    const unsigned int i2 = surface.BaseVertex + indices[baseInd + 2];
                    Float3 const& v0 = vertices[i0].Position;
                    Float3 const& v1 = vertices[i1].Position;
                    Float3 const& v2 = vertices[i2].Position;
                    if (BvRayIntersectTriangle(rayStart, rayDir, v0, v1, v2, d, u, v, bCullBackFace))
                    {
                        if (distance > d)
                        {
                            distance = d;
                            hitDistance = d;
                            hitLocation = rayStart + rayDir * d;
                            hitUV.X = u;
                            hitUV.Y = v;
                            triangle[0] = i0;
                            triangle[1] = i1;
                            triangle[2] = i2;
                            ret = true;
                        }
                    }
                }
            }

            nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
        }
    }
    else
    {
        float hitMin, hitMax;

        if (!BvRayIntersectBox(rayStart, invRayDir, surface.BoundingBox, hitMin, hitMax) || hitMin >= distance)
        {
            return false;
        }

        const int primCount = surface.IndexCount / 3;

        for (int tri = 0; tri < primCount; tri++, indices += 3)
        {
            const unsigned int i0 = surface.BaseVertex + indices[0];
            const unsigned int i1 = surface.BaseVertex + indices[1];
            const unsigned int i2 = surface.BaseVertex + indices[2];

            Float3 const& v0 = vertices[i0].Position;
            Float3 const& v1 = vertices[i1].Position;
            Float3 const& v2 = vertices[i2].Position;

            if (BvRayIntersectTriangle(rayStart, rayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (distance > d)
                {
                    distance = d;
                    hitDistance = d;
                    hitLocation = rayStart + rayDir * d;
                    hitUV.X = u;
                    hitUV.Y = v;
                    triangle[0] = i0;
                    triangle[1] = i1;
                    triangle[2] = i2;
                    ret = true;
                }
            }
        }
    }
    return ret;
}

bool MeshResource::Raycast(Float3 const& rayStart, Float3 const& rayDir, float distance, bool bCullBackFace, Vector<TriangleHitResult>& hitResult) const
{
    bool ret = false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    if (!BvRayIntersectBox(rayStart, invRayDir, m_BoundingBox, boxMin, boxMax) || boxMin >= distance)
    {
        return false;
    }

    for (int i = 0; i < m_Surfaces.Size(); i++)
    {
        ret |= Raycast(i, rayStart, rayDir, invRayDir, distance, bCullBackFace, hitResult);
    }
    return ret;
}

bool MeshResource::RaycastClosest(Float3 const& rayStart, Float3 const& rayDir, float distance, bool bCullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3], int& surfaceIndex) const
{
    bool ret = false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    if (!BvRayIntersectBox(rayStart, invRayDir, m_BoundingBox, boxMin, boxMax) || boxMin >= distance)
    {
        return false;
    }

    for (int i = 0; i < m_Surfaces.Size(); i++)
    {
        if (RaycastClosest(i, rayStart, rayDir, invRayDir, distance, bCullBackFace, hitLocation, hitUV, hitDistance, triangle))
        {
            surfaceIndex = i;
            distance = hitDistance;
            ret = true;
        }
    }

    return ret;
}

namespace
{
    UniqueRef<OzzSkeleton> ConvertSkeletonToOzz(RawSkeleton const* rawSkeleton)
    {
        UniqueRef<ozz::animation::Skeleton> ozzSkeleton = MakeUnique<ozz::animation::Skeleton>();
        const int numJoints = rawSkeleton->Joints.Size();

        int nameBufferSize = 0;
        for (int i = 0; i < numJoints; ++i)
            nameBufferSize += rawSkeleton->Joints[i].Name.Size() + 1;

        char* nameBuffer = ozzSkeleton->Allocate(nameBufferSize, numJoints);
        for (int i = 0; i < numJoints; ++i)
        {
            auto& joint = rawSkeleton->Joints[i];
            ozzSkeleton->joint_names_[i] = nameBuffer;
            strcpy(nameBuffer, joint.Name.GetRawString());
            nameBuffer += joint.Name.Size() + 1;
        }

        for (int i = 0; i < numJoints; ++i)
            ozzSkeleton->joint_parents_[i] = rawSkeleton->Joints[i].Parent;

        const SimdFloat4 w_axis = Simd::AxisW();
        const SimdFloat4 zero = Simd::Zero();
        const SimdFloat4 one = Simd::One();
        for (int i = 0; i < ozzSkeleton->num_soa_joints(); ++i)
        {
            SimdFloat4 translations[4];
            SimdFloat4 scales[4];
            SimdFloat4 rotations[4];
            for (int j = 0; j < 4; ++j)
            {
                if (i * 4 + j < numJoints)
                {
                    auto& rawJoint = rawSkeleton->Joints[i * 4 + j];

                    translations[j] = Simd::Load3PtrU(&rawJoint.Position.X);
                    rotations[j] = Simd::NormalizeSafe4(Simd::LoadPtrU(&rawJoint.Rotation.X), w_axis);
                    scales[j] = Simd::Load3PtrU(&rawJoint.Scale.X);
                }
                else
                {
                    translations[j] = zero;
                    rotations[j] = w_axis;
                    scales[j] = one;
                }
            }

            // Fills the SoaTransform structure.
            Simd::Transpose4x3(translations, &ozzSkeleton->joint_rest_poses_[i].translation.x);
            Simd::Transpose4x4(rotations, &ozzSkeleton->joint_rest_poses_[i].rotation.x);
            Simd::Transpose4x3(scales, &ozzSkeleton->joint_rest_poses_[i].scale.x);
        }

        return ozzSkeleton;
    }
}

UniqueRef<MeshResource> MeshResourceBuilder::Build(RawMesh const& rawMesh)
{
    UniqueRef<MeshResource> resource = MakeUnique<MeshResource>();

    auto& m_Surfaces = resource->m_Surfaces;
    auto& m_Skins = resource->m_Skins;
    auto& m_JointRemaps = resource->m_JointRemaps;
    auto& m_InverseBindPoses = resource->m_InverseBindPoses;
    auto& m_Skeleton = resource->m_Skeleton;
    auto& m_Vertices = resource->m_Vertices;
    auto& m_SkinBuffer = resource->m_SkinBuffer;
    //auto& m_LightmapUVs = resource->m_LightmapUVs; // TODO
    auto& m_Indices = resource->m_Indices;
    auto& m_BoundingBox = resource->m_BoundingBox;

    m_Surfaces.Reserve(rawMesh.Surfaces.Size());
    m_Skeleton = ConvertSkeletonToOzz(&rawMesh.Skeleton);

    m_Skins.Resize(rawMesh.Skins.Size());
    uint32_t matrixCount = 0;
    for (size_t skinIndex = 0; skinIndex < m_Skins.Size(); ++skinIndex)
    {
        m_Skins[skinIndex].FirstMatrix = matrixCount;
        m_Skins[skinIndex].MatrixCount = rawMesh.Skins[skinIndex]->GetJointCount();
        matrixCount += m_Skins[skinIndex].MatrixCount;
    }

    m_JointRemaps.Resize(matrixCount);
    m_InverseBindPoses.Resize(matrixCount);
    for (size_t skinIndex = 0; skinIndex < m_Skins.Size(); ++skinIndex)
    {
        Core::Memcpy(&m_JointRemaps[m_Skins[skinIndex].FirstMatrix], &rawMesh.Skins[skinIndex]->JointRemaps[0], sizeof(m_JointRemaps[0]) * m_Skins[skinIndex].MatrixCount);

        for (uint32_t n = 0; n < m_Skins[skinIndex].MatrixCount; ++n)
        {
            auto source = Float4x4(rawMesh.Skins[skinIndex]->InverseBindPoses[n]).Transposed();
            auto& matrix = m_InverseBindPoses[m_Skins[skinIndex].FirstMatrix + n];

            matrix.cols[0] = Simd::LoadPtr(&source.Col0[0]);
            matrix.cols[1] = Simd::LoadPtr(&source.Col1[0]);
            matrix.cols[2] = Simd::LoadPtr(&source.Col2[0]);
            matrix.cols[3] = Simd::LoadPtr(&source.Col3[0]);
        }
    }

    Vector<Float3> tempNormals;

    size_t vertexCount = 0;
    size_t indexCount = 0;
    bool hasSkinning = false;
    for (auto& surface : rawMesh.Surfaces)
    {
        auto firstVertex = vertexCount;
        auto firstIndex = indexCount;

        vertexCount += surface->Positions.Size();
        indexCount += surface->Indices.Size();

        auto& dst = m_Surfaces.EmplaceBack();
        dst.BaseVertex = firstVertex;
        dst.VertexCount = surface->Positions.Size();
        dst.FirstIndex = firstIndex;
        dst.IndexCount = surface->Indices.Size();
        dst.SkinIndex = rawMesh.Skins.IndexOf(surface->Skin, [](auto& a, auto& b)
            {
                return a.RawPtr() == b;
            });
        dst.JointIndex = surface->JointIndex;

        Float4x4 inverseTransform = Float4x4(surface->InverseTransform).Transposed();
        dst.InverseTransform.cols[0] = Simd::LoadPtr(&inverseTransform.Col0[0]);
        dst.InverseTransform.cols[1] = Simd::LoadPtr(&inverseTransform.Col1[0]);
        dst.InverseTransform.cols[2] = Simd::LoadPtr(&inverseTransform.Col2[0]);
        dst.InverseTransform.cols[3] = Simd::LoadPtr(&inverseTransform.Col3[0]);

        dst.BoundingBox = surface->BoundingBox;

        hasSkinning |= surface->SkinVerts.Size() > 0;
    }

    m_Vertices.Reserve(vertexCount);
    m_Indices.Reserve(indexCount);

    if (hasSkinning)
        m_SkinBuffer.Resize(vertexCount);

    for (auto& surface : rawMesh.Surfaces)
    {
        auto firstVertex = m_Vertices.Size();

        // Fill positions
        {
            size_t count = surface->Positions.Size();
            m_Vertices.Resize(firstVertex + count);
            for (size_t n = 0; n < count; ++n)
                m_Vertices[firstVertex + n].Position = surface->Positions[n];
        }

        // Fill texcoords
        {
            size_t count = Math::Min(surface->TexCoords.Size(), surface->Positions.Size());
            size_t n;
            for (n = 0; n < count; ++n)
                m_Vertices[firstVertex + n].SetTexCoord(surface->TexCoords[n]);
            for (; n < surface->Positions.Size(); ++n)
                m_Vertices[firstVertex + n].SetTexCoord(0, 0);
        }

        // Fill normals
        {
            size_t count = surface->Positions.Size();
            if (surface->Normals.Size() != count)
            {
                if (tempNormals.Size() < count)
                    tempNormals.Resize(count);

                Geometry::CalcNormals(surface->Positions.ToPtr(), tempNormals.ToPtr(), count, surface->Indices.ToPtr(), surface->Indices.Size());

                for (size_t n = 0; n < count; ++n)
                    m_Vertices[firstVertex + n].SetNormal(tempNormals[n]);
            }
            else
            {
                for (size_t n = 0; n < count; ++n)
                    m_Vertices[firstVertex + n].SetNormal(surface->Normals[n]);
            }
        }

        // Fill tangents
        {
            size_t count = surface->Positions.Size();
            if (surface->Tangents.Size() != count)
            {
                Geometry::CalcTangentSpace(&m_Vertices[firstVertex], surface->Indices.ToPtr(), surface->Indices.Size());
            }
            else
            {
                for (size_t n = 0; n < count; ++n)
                {
                    m_Vertices[firstVertex + n].SetTangent(surface->Tangents[n].X, surface->Tangents[n].Y, surface->Tangents[n].Z);
                    m_Vertices[firstVertex + n].Handedness = surface->Tangents[n].W;
                }
            }
        }

        // Fill skinning
        if (hasSkinning)
        {
            size_t count = Math::Min(surface->SkinVerts.Size(), surface->Positions.Size());
            size_t n;
            for (n = 0; n < count; ++n)
                m_SkinBuffer[firstVertex + n] = surface->SkinVerts[n];
            count = surface->Positions.Size() - n;
            if (count)
                Core::ZeroMem(&m_SkinBuffer[firstVertex + n], count * sizeof(SkinVertex));
        }

        // Fill indices
        {
            m_Indices.Add(surface->Indices);
        }
    }

    m_BoundingBox = rawMesh.CalcBoundingBox();

    return resource;
}

HK_NAMESPACE_END
