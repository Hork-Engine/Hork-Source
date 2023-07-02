#include "Resource_Mesh.h"
#include "Resource_Skeleton.h"
#include "ResourceManager.h"

#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

MeshResource::MeshResource(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    Read(stream, resManager);
}

MeshResource::~MeshResource()
{
    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Deallocate(m_VertexHandle);
    vertexMemory->Deallocate(m_WeightsHandle);
    vertexMemory->Deallocate(m_LightmapUVsGPU);
    vertexMemory->Deallocate(m_IndexHandle);
}

bool MeshResource::Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    String resourcePath;

    stream.ReadArray(m_Vertices);
    stream.ReadArray(m_Weights);
    stream.ReadArray(m_LightmapUVs);
    stream.ReadArray(m_Indices);
    stream.ReadArray(m_Subparts);
    stream.ReadArray(m_Sockets);
    #if 0
    uint32_t numMaterials = stream.ReadUInt32();

    m_Materials.Clear();
    m_Materials.Reserve(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i)
    {
        resourcePath = stream.ReadString();

        m_Materials.Add(resManager->GetResource<MaterialInstance>(resourcePath));
    }
    #endif
    stream.ReadArray(m_Skin.JointIndices);
    stream.ReadArray(m_Skin.OffsetMatrices);
    stream.ReadObject(m_BoundingBox);

    resourcePath = stream.ReadString();
    if (!resourcePath.IsEmpty())
        m_Skeleton = resManager->GetResource<SkeletonResource>(resourcePath);
    else
        m_Skeleton = {};

    m_IsSkinned = stream.ReadBool();
    m_BvhPrimitivesPerLeaf = stream.ReadUInt16();    

    return true;
}

void MeshResource::Write(IBinaryStreamWriteInterface& stream, ResourceManager* resManager) const
{
    StringView resourcePath;

    stream.WriteUInt32(MakeResourceMagic(Type, Version));
    stream.WriteArray(m_Vertices);
    stream.WriteArray(m_Weights);
    stream.WriteArray(m_LightmapUVs);
    stream.WriteArray(m_Indices);
    stream.WriteArray(m_Subparts);
    stream.WriteArray(m_Sockets);
    #if 0
    uint32_t numMaterials = m_Materials.Size();
    stream.WriteUInt32(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i)
    {
        resourcePath = resManager->GetProxy(m_Materials[i]).GetName();
        stream.WriteString(resourcePath);
    }
    #endif
    stream.WriteArray(m_Skin.JointIndices);
    stream.WriteArray(m_Skin.OffsetMatrices);
    stream.WriteObject(m_BoundingBox);

    if (m_Skeleton)
        resourcePath = resManager->GetProxy(m_Skeleton).GetName();
    else
        resourcePath = "";
    stream.WriteString(resourcePath);

    stream.WriteBool(m_IsSkinned);
    stream.WriteUInt16(m_BvhPrimitivesPerLeaf);
}

void* MeshResource::GetVertexMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_Vertices.ToPtr();
}

void* MeshResource::GetWeightMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_Weights.ToPtr();
}

void* MeshResource::GetLightmapUVMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_LightmapUVs.ToPtr();
}

void* MeshResource::GetIndexMemory(void* _This)
{
    return static_cast<MeshResource*>(_This)->m_Indices.ToPtr();
}

void MeshResource::GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_VertexHandle)
    {
        VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_VertexHandle, ppBuffer, pOffset);
    }
}

void MeshResource::GetWeightsBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_WeightsHandle)
    {
        VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_WeightsHandle, ppBuffer, pOffset);
    }
}

void MeshResource::GetLightmapUVsGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_LightmapUVsGPU)
    {
        VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_LightmapUVsGPU, ppBuffer, pOffset);
    }
}

void MeshResource::GetIndexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_IndexHandle)
    {
        VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_IndexHandle, ppBuffer, pOffset);
    }
}

void MeshResource::Allocate(int vertexCount, int indexCount, int subpartCount, bool bSkinned, bool bWithLightmapUVs)
{
    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Deallocate(m_VertexHandle);
    vertexMemory->Deallocate(m_WeightsHandle);
    vertexMemory->Deallocate(m_LightmapUVsGPU);
    vertexMemory->Deallocate(m_IndexHandle);

    m_Vertices.Resize(vertexCount);

    if (bSkinned)
        m_Weights.Resize(vertexCount);
    else
        m_Weights.Clear();

    if (bWithLightmapUVs)
        m_LightmapUVs.Resize(vertexCount);
    else
        m_LightmapUVs.Clear();

    m_IsSkinned = bSkinned;

    m_VertexHandle = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertex), nullptr, GetVertexMemory, this);
    m_IndexHandle  = vertexMemory->AllocateIndex(m_Indices.Size() * sizeof(unsigned int), nullptr, GetIndexMemory, this);

    if (bSkinned)
        m_WeightsHandle = vertexMemory->AllocateVertex(m_Weights.Size() * sizeof(MeshVertexSkin), nullptr, GetWeightMemory, this);
    else
        m_WeightsHandle = nullptr;

    if (bWithLightmapUVs)
        m_LightmapUVsGPU = vertexMemory->AllocateVertex(m_LightmapUVs.Size() * sizeof(MeshVertexUV), nullptr, GetLightmapUVMemory, this);
    else
        m_LightmapUVsGPU = nullptr;

    subpartCount = subpartCount < 1 ? 1 : subpartCount;
    m_Subparts.Resize(subpartCount);
    if (subpartCount == 1)
    {
        MeshSubpart& subpart = m_Subparts[0];
        subpart.BaseVertex = 0;
        subpart.FirstIndex = 0;
        subpart.VertexCount = m_Vertices.Size();
        subpart.IndexCount = m_Indices.Size();
    }

    m_Vertices.ShrinkToFit();
    m_Weights.ShrinkToFit();
    m_LightmapUVs.ShrinkToFit();
    m_Subparts.ShrinkToFit();
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

    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Update(m_VertexHandle, startVertexLocation * sizeof(MeshVertex), vertexCount * sizeof(MeshVertex), m_Vertices.ToPtr() + startVertexLocation);

    return true;
}

bool MeshResource::WriteJointWeights(MeshVertexSkin const* vertices, int vertexCount, int startVertexLocation)
{
    if (!m_IsSkinned)
    {
        LOG("MeshResource::WriteJointWeights: Cannot write joint weights for static mesh\n");
        return false;
    }

    if (!vertexCount)
    {
        return true;
    }

    if (startVertexLocation + vertexCount > m_Weights.Size())
    {
        LOG("MeshResource::WriteJointWeights: Referencing outside of buffer\n");
        return false;
    }

    Core::Memcpy(m_Weights.ToPtr() + startVertexLocation, vertices, vertexCount * sizeof(MeshVertexSkin));

    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Update(m_WeightsHandle, startVertexLocation * sizeof(MeshVertexSkin), vertexCount * sizeof(MeshVertexSkin), m_Weights.ToPtr() + startVertexLocation);

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

    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Update(m_LightmapUVsGPU, startVertexLocation * sizeof(MeshVertexUV), vertexCount * sizeof(MeshVertexUV), m_LightmapUVs.ToPtr() + startVertexLocation);

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

    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Update(m_IndexHandle, startIndexLocation * sizeof(unsigned int), indexCount * sizeof(unsigned int), m_Indices.ToPtr() + startIndexLocation);

    return true;
}

void MeshResource::Upload()
{
    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    vertexMemory->Deallocate(m_VertexHandle);
    vertexMemory->Deallocate(m_WeightsHandle);
    vertexMemory->Deallocate(m_LightmapUVsGPU);
    vertexMemory->Deallocate(m_IndexHandle);

    m_VertexHandle = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertex), nullptr, GetVertexMemory, this);
    m_IndexHandle = vertexMemory->AllocateIndex(m_Indices.Size() * sizeof(unsigned int), nullptr, GetIndexMemory, this);

    if (m_IsSkinned)
    {
        m_WeightsHandle = vertexMemory->AllocateVertex(m_Weights.Size() * sizeof(MeshVertexSkin), nullptr, GetWeightMemory, this);
    }
    else
        m_WeightsHandle = nullptr;

    if (!m_LightmapUVs.IsEmpty())
    {
        m_LightmapUVsGPU = vertexMemory->AllocateVertex(m_LightmapUVs.Size() * sizeof(MeshVertexUV), nullptr, GetLightmapUVMemory, this);
    }
    else
        m_LightmapUVsGPU = nullptr;

    vertexMemory->Update(m_VertexHandle, 0, m_Vertices.Size() * sizeof(MeshVertex), m_Vertices.ToPtr());

    vertexMemory->Update(m_IndexHandle, 0, m_Indices.Size() * sizeof(unsigned int), m_Indices.ToPtr());

    if (m_IsSkinned)
    {
        vertexMemory->Update(m_WeightsHandle, 0, m_Weights.Size() * sizeof(MeshVertexSkin), m_Weights.ToPtr());
    }
}

void MeshResource::AddLightmapUVs()
{
    if (m_LightmapUVsGPU && m_LightmapUVs.Size() == m_Vertices.Size())
        return;

    VertexMemoryGPU* vertexMemory = GameApplication::GetVertexMemoryGPU();

    if (m_LightmapUVsGPU)
        vertexMemory->Deallocate(m_LightmapUVsGPU);

    m_LightmapUVsGPU = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertexUV), nullptr, GetLightmapUVMemory, this);
    m_LightmapUVs.Resize(m_Vertices.Size());
}

void MeshResource::SetBoundingBox(BvAxisAlignedBox const& boundingBox)
{
    m_BoundingBox = boundingBox;
}
#if 0
void MeshResource::SetMaterials(TArrayView<MaterialInstanceHandle> materials)
{
    m_Materials.Clear();
    m_Materials.Reserve(materials.Size());
    for (MaterialInstanceHandle handle : materials)
        m_Materials.Add(handle);
}

void MeshResource::SetMaterial(int subpartIndex, MaterialInstanceHandle handle)
{
    if (m_Materials.Size() <= subpartIndex)
        m_Materials.Resize(subpartIndex + 1);
    m_Materials[subpartIndex] = handle;
}
#endif
void MeshResource::SetSockets(TArrayView<MeshSocket> sockets)
{
    m_Sockets.Clear();
    m_Sockets.Reserve(sockets.Size());
    for (MeshSocket const& socket : sockets)
        m_Sockets.Add(socket);
}

void MeshResource::SetSkin(MeshSkin skin)
{
    m_Skin = std::move(skin);
}

void MeshResource::GenerateBVH(uint16_t primitivesPerLeaf)
{
    if (m_IsSkinned)
    {
        LOG("MeshResource::GenerateBVH: called for skinned mesh\n");
        return;
    }

    const uint16_t MaxPrimitivesPerLeaf = 1024;

    // Don't allow to generate large leafs
    if (primitivesPerLeaf > MaxPrimitivesPerLeaf)
    {
        primitivesPerLeaf = MaxPrimitivesPerLeaf;
    }

    for (MeshSubpart& subpart : m_Subparts)
    {
        subpart.Bvh = BvhTree(TArrayView<MeshVertex>(m_Vertices), TArrayView<unsigned int>(m_Indices.ToPtr() + subpart.FirstIndex, (size_t)subpart.IndexCount), subpart.BaseVertex, primitivesPerLeaf);
    }

    m_BvhPrimitivesPerLeaf = primitivesPerLeaf;
}

bool MeshResource::SubpartRaycast(int subpartIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool bCullBackFace, TVector<TriangleHitResult>& hitResult) const
{
    bool ret = false;
    float d, u, v;
    MeshSubpart const& subpart = m_Subparts[subpartIndex];
    unsigned int const* indices = m_Indices.ToPtr() + subpart.FirstIndex;
    MeshVertex const* vertices = m_Vertices.ToPtr();

    if (distance < 0.0001f)
    {
        return false;
    }

    if (!subpart.Bvh.GetNodes().IsEmpty())
    {
        TVector<BvhNode> const& nodes = subpart.Bvh.GetNodes();
        unsigned int const* indirection = subpart.Bvh.GetIndirection();

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
                    const unsigned int i0 = subpart.BaseVertex + indices[baseInd + 0];
                    const unsigned int i1 = subpart.BaseVertex + indices[baseInd + 1];
                    const unsigned int i2 = subpart.BaseVertex + indices[baseInd + 2];
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

        if (!BvRayIntersectBox(rayStart, invRayDir, subpart.BoundingBox, hitMin, hitMax) || hitMin >= distance)
        {
            return false;
        }

        const int primCount = subpart.IndexCount / 3;

        for (int tri = 0; tri < primCount; tri++, indices += 3)
        {
            const unsigned int i0 = subpart.BaseVertex + indices[0];
            const unsigned int i1 = subpart.BaseVertex + indices[1];
            const unsigned int i2 = subpart.BaseVertex + indices[2];

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

bool MeshResource::SubpartRaycastClosest(int subpartIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool bCullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3]) const
{
    bool ret = false;
    float d, u, v;
    MeshSubpart const& subpart = m_Subparts[subpartIndex];
    unsigned int const* indices = m_Indices.ToPtr() + subpart.FirstIndex;
    MeshVertex const* vertices = m_Vertices.ToPtr();

    if (distance < 0.0001f)
    {
        return false;
    }

    if (!subpart.Bvh.GetNodes().IsEmpty())
    {
        TVector<BvhNode> const& nodes = subpart.Bvh.GetNodes();
        unsigned int const* indirection = subpart.Bvh.GetIndirection();

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
                    const unsigned int i0 = subpart.BaseVertex + indices[baseInd + 0];
                    const unsigned int i1 = subpart.BaseVertex + indices[baseInd + 1];
                    const unsigned int i2 = subpart.BaseVertex + indices[baseInd + 2];
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

        if (!BvRayIntersectBox(rayStart, invRayDir, subpart.BoundingBox, hitMin, hitMax) || hitMin >= distance)
        {
            return false;
        }

        const int primCount = subpart.IndexCount / 3;

        for (int tri = 0; tri < primCount; tri++, indices += 3)
        {
            const unsigned int i0 = subpart.BaseVertex + indices[0];
            const unsigned int i1 = subpart.BaseVertex + indices[1];
            const unsigned int i2 = subpart.BaseVertex + indices[2];

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

bool MeshResource::Raycast(Float3 const& rayStart, Float3 const& rayDir, float distance, bool bCullBackFace, TVector<TriangleHitResult>& hitResult) const
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

    for (int i = 0; i < m_Subparts.Size(); i++)
    {
        ret |= SubpartRaycast(i, rayStart, rayDir, invRayDir, distance, bCullBackFace, hitResult);
    }
    return ret;
}

bool MeshResource::RaycastClosest(Float3 const& rayStart, Float3 const& rayDir, float distance, bool bCullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3], int& subpartIndex) const
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

    for (int i = 0; i < m_Subparts.Size(); i++)
    {
        if (SubpartRaycastClosest(i, rayStart, rayDir, invRayDir, distance, bCullBackFace, hitLocation, hitUV, hitDistance, triangle))
        {
            subpartIndex = i;
            distance = hitDistance;
            ret = true;
        }
    }

    return ret;
}

void MeshResource::DrawDebug(DebugRenderer& renderer) const
{
    renderer.SetDepthTest(false);
    renderer.SetColor(Color4::White());

    renderer.DrawAABB(m_BoundingBox);

    for (MeshSubpart const& subpart : m_Subparts)
    {
        renderer.DrawAABB(subpart.BoundingBox);

        if (!subpart.Bvh.GetNodes().IsEmpty())
        {
            BvOrientedBox orientedBox;

            for (BvhNode const& node : subpart.Bvh.GetNodes())
            {
                if (node.IsLeaf())
                {
                    renderer.DrawAABB(node.Bounds);
                }
            }
        }
    }

    // TODO: Draw sockets
}

//void MeshResourceInterface::Raycast(ResourceID resource, Float3 const& rayStart, Float3 const& rayDir)
//{
//    //if (!resource.Is<MeshResource>())
//    //{
//    //    // error
//    //}
//
//    //MeshResource& meshResource = m_Manager->m_Resources.Get<MeshResource>(resource);
//
//    //HK_UNUSED(meshResource);
//}

HK_NAMESPACE_END
