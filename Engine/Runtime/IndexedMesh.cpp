/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "IndexedMesh.h"
#include "Animation.h"
#include "Level.h"
#include "ResourceManager.h"
#include "Engine.h"

#include <Engine/Assets/Asset.h>
#include <Engine/Core/Platform/Logger.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Core/ScopedTimer.h>
#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/Geometry/TangentSpace.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(IndexedMesh)
HK_CLASS_META(ProceduralMesh)

///////////////////////////////////////////////////////////////////////////////////////////////////////

IndexedMesh::IndexedMesh()
{
    static TStaticResourceFinder<Skeleton> SkeletonResource("/Default/Skeleton/Default"s);
    m_Skeleton = SkeletonResource.GetObject();

    m_BoundingBox.Clear();
}

IndexedMesh::~IndexedMesh()
{
    Purge();

    HK_ASSERT(m_LightmapUVs.IsEmpty());
}

IndexedMesh* IndexedMesh::Create(int NumVertices, int NumIndices, int NumSubparts, bool bSkinnedMesh)
{
    IndexedMesh* mesh = NewObj<IndexedMesh>();
    mesh->Initialize(NumVertices, NumIndices, NumSubparts, bSkinnedMesh);
    return mesh;
}

void IndexedMesh::Initialize(int NumVertices, int NumIndices, int NumSubparts, bool bSkinnedMesh)
{
    Purge();

    m_bSkinnedMesh      = bSkinnedMesh;
    m_bBoundingBoxDirty = true;
    m_BoundingBox.Clear();

    m_Vertices.ResizeInvalidate(NumVertices);
    m_Indices.ResizeInvalidate(NumIndices);

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    m_VertexHandle = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertex), nullptr, GetVertexMemory, this);
    m_IndexHandle  = vertexMemory->AllocateIndex(m_Indices.Size() * sizeof(unsigned int), nullptr, GetIndexMemory, this);

    if (m_bSkinnedMesh)
    {
        m_Weights.ResizeInvalidate(NumVertices);
        m_WeightsHandle = vertexMemory->AllocateVertex(m_Weights.Size() * sizeof(MeshVertexSkin), nullptr, GetWeightMemory, this);
    }

    NumSubparts = NumSubparts < 1 ? 1 : NumSubparts;

    m_Subparts.ResizeInvalidate(NumSubparts);
    for (int i = 0; i < NumSubparts; i++)
    {
        m_Subparts[i]              = new IndexedMeshSubpart;
        m_Subparts[i]->m_OwnerMesh = this;
    }

    if (NumSubparts == 1)
    {
        IndexedMeshSubpart* subpart = m_Subparts[0];
        subpart->m_BaseVertex        = 0;
        subpart->m_FirstIndex        = 0;
        subpart->m_VertexCount       = m_Vertices.Size();
        subpart->m_IndexCount        = m_Indices.Size();
    }
}

void IndexedMesh::AddLightmapUVs()
{
    if (m_LightmapUVs.Size() == m_Vertices.Size())
        return;

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    if (m_LightmapUVsGPU)
        vertexMemory->Deallocate(m_LightmapUVsGPU);

    m_LightmapUVsGPU = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertexUV), nullptr, GetLightmapUVMemory, this);
    m_LightmapUVs.Resize(m_Vertices.Size());
}

void IndexedMesh::Purge()
{
    for (IndexedMeshSubpart* subpart : m_Subparts)
    {
        subpart->m_OwnerMesh = nullptr;
        subpart->RemoveRef();
    }

    m_Subparts.Clear();

    for (SocketDef* socket : m_Sockets)
    {
        socket->RemoveRef();
    }

    m_Sockets.Clear();

    m_Skin.JointIndices.Clear();
    m_Skin.OffsetMatrices.Clear();

    m_CollisionModel.Reset();

    m_Vertices.Free();
    m_Weights.Free();
    m_Indices.Free();
    m_LightmapUVs.Free();

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Deallocate(m_VertexHandle);
    m_VertexHandle = nullptr;

    vertexMemory->Deallocate(m_IndexHandle);
    m_IndexHandle = nullptr;

    vertexMemory->Deallocate(m_WeightsHandle);
    m_WeightsHandle = nullptr;

    vertexMemory->Deallocate(m_LightmapUVsGPU);
    m_LightmapUVsGPU = nullptr;
}

bool IndexedMesh::LoadResource(IBinaryStreamReadInterface& Stream)
{
    //ScopedTimer ScopedTime(Stream.GetFileName());

    Purge();

    String text = Stream.AsString();

    DocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = text.CStr();
    deserializeInfo.bInsitu       = true;

    Document doc;
    doc.DeserializeFromString(deserializeInfo);

    DocumentMember* member;

    member = doc.FindMember("Mesh");
    if (!member)
    {
        LOG("IndexedMesh::LoadResource: invalid mesh\n");

        NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);
        return false;
    }

    auto meshFile = member->GetStringView();
    if (meshFile.IsEmpty())
    {
        LOG("IndexedMesh::LoadResource: invalid mesh\n");

        NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);
        return false;
    }

    BinaryResource* meshBinary = Resource::CreateFromFile<BinaryResource>(meshFile);

    if (!meshBinary->GetSizeInBytes())
    {
        LOG("IndexedMesh::LoadResource: invalid mesh\n");

        NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);
        return false;
    }

    File meshData = File::OpenRead(meshFile, meshBinary->GetBinaryData(), meshBinary->GetSizeInBytes());
    if (!meshData)
    {
        LOG("IndexedMesh::LoadResource: invalid mesh\n");

        NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);
        return false;
    }

    uint32_t fileFormat = meshData.ReadUInt32();

    if (fileFormat != ASSET_MESH)
    {
        LOG("Expected file format {}\n", ASSET_MESH);

        NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);
        return false;
    }

    uint32_t fileVersion = meshData.ReadUInt32();

    if (fileVersion != ASSET_VERSION_MESH)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_MESH);

        NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);
        return false;
    }

    bool bRaycastBVH;

    String guidStr = meshData.ReadString();

    m_bSkinnedMesh = meshData.ReadBool();
    meshData.ReadObject(m_BoundingBox);
    meshData.ReadArray(m_Indices);
    meshData.ReadArray(m_Vertices);
    meshData.ReadArray(m_Weights);
    bRaycastBVH              = meshData.ReadBool();
    m_RaycastPrimitivesPerLeaf = meshData.ReadUInt16();

    uint32_t subpartsCount = meshData.ReadUInt32();
    m_Subparts.ResizeInvalidate(subpartsCount);
    for (int i = 0; i < m_Subparts.Size(); i++)
    {
        m_Subparts[i] = new IndexedMeshSubpart;
        m_Subparts[i]->Read(meshData);
    }

    member = doc.FindMember("Subparts");
    if (member)
    {
        DocumentValue* values       = member->GetArrayValues();
        int        subpartIndex = 0;
        for (DocumentValue* v = values; v && subpartIndex < m_Subparts.Size(); v = v->GetNext())
        {
            if (v->GetStringView())
                m_Subparts[subpartIndex]->SetMaterialInstance(GetOrCreateResource<MaterialInstance>(v->GetStringView()));
            subpartIndex++;
        }
    }

    if (bRaycastBVH)
    {
        for (IndexedMeshSubpart* subpart : m_Subparts)
        {
            std::unique_ptr<BvhTree> bvh = std::make_unique<BvhTree>();

            meshData.ReadObject(*bvh);

            subpart->SetBVH(std::move(bvh));
        }
    }

    uint32_t socketsCount = meshData.ReadUInt32();
    m_Sockets.ResizeInvalidate(socketsCount);
    for (int i = 0; i < m_Sockets.Size(); i++)
    {
        SocketDef* socket = new SocketDef;
        socket->Read(meshData);
        m_Sockets[i] = socket;
    }

    if (m_bSkinnedMesh)
    {
        meshData.ReadArray(m_Skin.JointIndices);
        meshData.ReadArray(m_Skin.OffsetMatrices);
    }

    for (IndexedMeshSubpart* subpart : m_Subparts)
    {
        subpart->m_OwnerMesh = this;
    }

    member = doc.FindMember("Skeleton");
    SetSkeleton(GetOrCreateResource<Skeleton>(member ? member->GetStringView() : "/Default/Skeleton/Default"));

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    m_VertexHandle = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertex), nullptr, GetVertexMemory, this);
    m_IndexHandle  = vertexMemory->AllocateIndex(m_Indices.Size() * sizeof(unsigned int), nullptr, GetIndexMemory, this);

    if (m_bSkinnedMesh)
    {
        m_WeightsHandle = vertexMemory->AllocateVertex(m_Weights.Size() * sizeof(MeshVertexSkin), nullptr, GetWeightMemory, this);
    }

    SendVertexDataToGPU(m_Vertices.Size(), 0);
    SendIndexDataToGPU(m_Indices.Size(), 0);
    if (m_bSkinnedMesh)
    {
        SendJointWeightsToGPU(m_Weights.Size(), 0);
    }

    // TODO: Load lightmapUVs

    m_bBoundingBoxDirty = false;

    if (!m_bSkinnedMesh)
    {
        GenerateRigidbodyCollisions(); // TODO: load collision from file
    }

    NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_ALL);

    return true;
}

void* IndexedMesh::GetVertexMemory(void* _This)
{
    return static_cast<IndexedMesh*>(_This)->GetVertices();
}

void* IndexedMesh::GetIndexMemory(void* _This)
{
    return static_cast<IndexedMesh*>(_This)->GetIndices();
}

void* IndexedMesh::GetWeightMemory(void* _This)
{
    return static_cast<IndexedMesh*>(_This)->GetWeights();
}

void* IndexedMesh::GetLightmapUVMemory(void* _This)
{
    return static_cast<IndexedMesh*>(_This)->GetLigtmapUVs();
}

void IndexedMesh::GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_VertexHandle)
    {
        VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_VertexHandle, ppBuffer, pOffset);
    }
}

void IndexedMesh::GetIndexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_IndexHandle)
    {
        VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_IndexHandle, ppBuffer, pOffset);
    }
}

void IndexedMesh::GetWeightsBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_WeightsHandle)
    {
        VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_WeightsHandle, ppBuffer, pOffset);
    }
}

void IndexedMesh::GetLightmapUVsGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_LightmapUVsGPU)
    {
        VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_LightmapUVsGPU, ppBuffer, pOffset);
    }
}

void IndexedMesh::AddSocket(SocketDef* pSocket)
{
    if (!pSocket)
    {
        LOG("IndexedMesh::AddSocket: nullptr\n");
        return;
    }
    pSocket->AddRef();
    m_Sockets.Add(pSocket);
}

SocketDef* IndexedMesh::FindSocket(StringView Name)
{
    for (SocketDef* socket : m_Sockets)
    {
        if (!socket->Name.Icmp(Name))
        {
            return socket;
        }
    }
    return nullptr;
}

void IndexedMesh::GenerateBVH(unsigned int PrimitivesPerLeaf)
{
    ScopedTimer ScopedTime("GenerateBVH");

    if (m_bSkinnedMesh)
    {
        LOG("IndexedMesh::GenerateBVH: called for skinned mesh\n");
        return;
    }

    const unsigned int MaxPrimitivesPerLeaf = 1024;

    // Don't allow to generate large leafs
    if (PrimitivesPerLeaf > MaxPrimitivesPerLeaf)
    {
        PrimitivesPerLeaf = MaxPrimitivesPerLeaf;
    }

    for (IndexedMeshSubpart* subpart : m_Subparts)
    {
        subpart->GenerateBVH(PrimitivesPerLeaf);
    }

    m_RaycastPrimitivesPerLeaf = PrimitivesPerLeaf;
}

void IndexedMesh::SetSkeleton(Skeleton* pSkeleton)
{
    m_Skeleton = pSkeleton;

    if (!m_Skeleton)
    {
        static TStaticResourceFinder<Skeleton> SkeletonResource("/Default/Skeleton/Default"s);
        m_Skeleton = SkeletonResource.GetObject();
    }
}

void IndexedMesh::SetSkin(int32_t const* pJointIndices, Float3x4 const* pOffsetMatrices, int JointsCount)
{
    m_Skin.JointIndices.ResizeInvalidate(JointsCount);
    m_Skin.OffsetMatrices.ResizeInvalidate(JointsCount);

    Platform::Memcpy(m_Skin.JointIndices.ToPtr(), pJointIndices, JointsCount * sizeof(*pJointIndices));
    Platform::Memcpy(m_Skin.OffsetMatrices.ToPtr(), pOffsetMatrices, JointsCount * sizeof(*pOffsetMatrices));
}

void IndexedMesh::SetCollisionModel(CollisionModel* pCollisionModel)
{
    m_CollisionModel = pCollisionModel;

    // TODO: Notify users that the collision model has been changed?
}

void IndexedMesh::SetMaterialInstance(int SubpartIndex, MaterialInstance* pMaterialInstance)
{
    if (SubpartIndex < 0 || SubpartIndex >= m_Subparts.Size())
    {
        return;
    }
    m_Subparts[SubpartIndex]->SetMaterialInstance(pMaterialInstance);

    // TODO: Notify users that the material instance has been changed?
}

void IndexedMesh::SetBoundingBox(int SubpartIndex, BvAxisAlignedBox const& _BoundingBox)
{
    if (SubpartIndex < 0 || SubpartIndex >= m_Subparts.Size())
    {
        return;
    }
    m_Subparts[SubpartIndex]->SetBoundingBox(_BoundingBox);

    // TODO: Notify users that the bounding box has been changed?
}

IndexedMeshSubpart* IndexedMesh::GetSubpart(int SubpartIndex)
{
    if (SubpartIndex < 0 || SubpartIndex >= m_Subparts.Size())
    {
        return nullptr;
    }
    return m_Subparts[SubpartIndex];
}

bool IndexedMesh::SendVertexDataToGPU(int VerticesCount, int StartVertexLocation)
{
    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Vertices.Size())
    {
        LOG("IndexedMesh::SendVertexDataToGPU: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Update(m_VertexHandle, StartVertexLocation * sizeof(MeshVertex), VerticesCount * sizeof(MeshVertex), m_Vertices.ToPtr() + StartVertexLocation);

    return true;
}

bool IndexedMesh::WriteVertexData(MeshVertex const* Vertices, int VerticesCount, int StartVertexLocation)
{
    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Vertices.Size())
    {
        LOG("IndexedMesh::WriteVertexData: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    Platform::Memcpy(m_Vertices.ToPtr() + StartVertexLocation, Vertices, VerticesCount * sizeof(MeshVertex));

    for (IndexedMeshSubpart* subpart : m_Subparts)
    {
        subpart->m_bAABBTreeDirty = true;
    }

    return SendVertexDataToGPU(VerticesCount, StartVertexLocation);
}

bool IndexedMesh::SendJointWeightsToGPU(int VerticesCount, int StartVertexLocation)
{
    if (!m_bSkinnedMesh)
    {
        LOG("IndexedMesh::SendJointWeightsToGPU: Cannot write joint weights for static mesh\n");
        return false;
    }

    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Weights.Size())
    {
        LOG("IndexedMesh::SendJointWeightsToGPU: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Update(m_WeightsHandle, StartVertexLocation * sizeof(MeshVertexSkin), VerticesCount * sizeof(MeshVertexSkin), m_Weights.ToPtr() + StartVertexLocation);

    return true;
}

bool IndexedMesh::WriteJointWeights(MeshVertexSkin const* Vertices, int VerticesCount, int StartVertexLocation)
{
    if (!m_bSkinnedMesh)
    {
        LOG("IndexedMesh::WriteJointWeights: Cannot write joint weights for static mesh\n");
        return false;
    }

    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Weights.Size())
    {
        LOG("IndexedMesh::WriteJointWeights: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    Platform::Memcpy(m_Weights.ToPtr() + StartVertexLocation, Vertices, VerticesCount * sizeof(MeshVertexSkin));

    return SendJointWeightsToGPU(VerticesCount, StartVertexLocation);
}

bool IndexedMesh::SendLightmapUVsToGPU(int VerticesCount, int StartVertexLocation)
{
    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Vertices.Size())
    {
        LOG("IndexedMesh::SendLightmapUVsToGPU: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    AddLightmapUVs();

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Update(m_LightmapUVsGPU, StartVertexLocation * sizeof(MeshVertexUV), VerticesCount * sizeof(MeshVertexUV), m_LightmapUVs.ToPtr() + StartVertexLocation);

    return true;
}

bool IndexedMesh::WriteLightmapUVsData(MeshVertexUV const* UVs, int VerticesCount, int StartVertexLocation)
{
    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Vertices.Size())
    {
        LOG("IndexedMesh::WriteLightmapUVsData: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    AddLightmapUVs();

    Platform::Memcpy(m_LightmapUVs.ToPtr() + StartVertexLocation, UVs, VerticesCount * sizeof(MeshVertexUV));

    return SendLightmapUVsToGPU(VerticesCount, StartVertexLocation);
}

bool IndexedMesh::SendIndexDataToGPU(int _IndexCount, int _StartIndexLocation)
{
    if (!_IndexCount)
    {
        return true;
    }

    if (_StartIndexLocation + _IndexCount > m_Indices.Size())
    {
        LOG("IndexedMesh::SendIndexDataToGPU: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Update(m_IndexHandle, _StartIndexLocation * sizeof(unsigned int), _IndexCount * sizeof(unsigned int), m_Indices.ToPtr() + _StartIndexLocation);

    return true;
}

bool IndexedMesh::WriteIndexData(unsigned int const* Indices, int _IndexCount, int _StartIndexLocation)
{
    if (!_IndexCount)
    {
        return true;
    }

    if (_StartIndexLocation + _IndexCount > m_Indices.Size())
    {
        LOG("IndexedMesh::WriteIndexData: Referencing outside of buffer ({})\n", GetResourcePath());
        return false;
    }

    Platform::Memcpy(m_Indices.ToPtr() + _StartIndexLocation, Indices, _IndexCount * sizeof(unsigned int));

    for (IndexedMeshSubpart* subpart : m_Subparts)
    {
        if (_StartIndexLocation >= subpart->m_FirstIndex && _StartIndexLocation + _IndexCount <= subpart->m_FirstIndex + subpart->m_IndexCount)
        {
            subpart->m_bAABBTreeDirty = true;
        }
    }

    return SendIndexDataToGPU(_IndexCount, _StartIndexLocation);
}

void IndexedMesh::UpdateBoundingBox()
{
    m_BoundingBox.Clear();
    for (IndexedMeshSubpart const* subpart : m_Subparts)
    {
        m_BoundingBox.AddAABB(subpart->GetBoundingBox());
    }
    m_bBoundingBoxDirty = false;
}

BvAxisAlignedBox const& IndexedMesh::GetBoundingBox() const
{
    if (m_bBoundingBoxDirty)
    {
        const_cast<ThisClass*>(this)->UpdateBoundingBox();
    }

    return m_BoundingBox;
}

IndexedMesh* IndexedMesh::CreateBox(Float3 const& Extents, float TexCoordScale)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateBoxMesh(vertices, indices, bounds, Extents, TexCoordScale);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreateSphere(float Radius, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateSphereMesh(vertices, indices, bounds, Radius, TexCoordScale, NumVerticalSubdivs, NumHorizontalSubdivs);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreatePlaneXZ(float Width, float Height, Float2 const& TexCoordScale)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreatePlaneMeshXZ(vertices, indices, bounds, Width, Height, TexCoordScale);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreatePlaneXY(float Width, float Height, Float2 const& TexCoordScale)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreatePlaneMeshXY(vertices, indices, bounds, Width, Height, TexCoordScale);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreatePatch(Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreatePatchMesh(vertices, indices, bounds,
                    Corner00, Corner10, Corner01, Corner11, TexCoordScale, bTwoSided, NumVerticalSubdivs, NumHorizontalSubdivs);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreateCylinder(float Radius, float Height, float TexCoordScale, int NumSubdivs)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateCylinderMesh(vertices, indices, bounds, Radius, Height, TexCoordScale, NumSubdivs);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreateCone(float Radius, float Height, float TexCoordScale, int NumSubdivs)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateConeMesh(vertices, indices, bounds, Radius, Height, TexCoordScale, NumSubdivs);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreateCapsule(float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateCapsuleMesh(vertices, indices, bounds, Radius, Height, TexCoordScale, NumVerticalSubdivs, NumHorizontalSubdivs);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreateSkybox(Float3 const& Extents, float TexCoordScale)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateSkyboxMesh(vertices, indices, bounds, Extents, TexCoordScale);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

IndexedMesh* IndexedMesh::CreateSkydome(float Radius, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs, bool bHemisphere)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox      bounds;

    CreateSkydomeMesh(vertices, indices, bounds, Radius, TexCoordScale, NumVerticalSubdivs, NumHorizontalSubdivs, bHemisphere);

    IndexedMesh* mesh = Create(vertices.Size(), indices.Size(), 1);
    mesh->WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    mesh->WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    mesh->SetBoundingBox(0, bounds);

    return mesh;
}

void IndexedMesh::LoadInternalResource(StringView _Path)
{
    TVertexBufferCPU<MeshVertex> vertices;
    TIndexBufferCPU<unsigned int> indices;
    BvAxisAlignedBox              bounds;
    CollisionModel*              collisionModel = nullptr;

    if (!_Path.Icmp("/Default/Meshes/Box"))
    {
        CreateBoxMesh(vertices, indices, bounds, Float3(1), 1);

        CollisionBoxDef box;

        collisionModel = NewObj<CollisionModel>(&box);
    }
    else if (!_Path.Icmp("/Default/Meshes/Sphere"))
    {
        CreateSphereMesh(vertices, indices, bounds, 0.5f, 1);

        CollisionSphereDef sphere;

        collisionModel = NewObj<CollisionModel>(&sphere);
    }
    else if (!_Path.Icmp("/Default/Meshes/Cylinder"))
    {
        CreateCylinderMesh(vertices, indices, bounds, 0.5f, 1, 1);

        CollisionCylinderDef cylinder;
        cylinder.Radius = 0.5f;
        cylinder.Height = 0.5f;

        collisionModel = NewObj<CollisionModel>(&cylinder);
    }
    else if (!_Path.Icmp("/Default/Meshes/Cone"))
    {
        CreateConeMesh(vertices, indices, bounds, 0.5f, 1, 1);

        CollisionConeDef cone;
        cone.Radius = 0.5f;

        collisionModel = NewObj<CollisionModel>(&cone);
    }
    else if (!_Path.Icmp("/Default/Meshes/Capsule"))
    {
        CreateCapsuleMesh(vertices, indices, bounds, 0.5f, 1.0f, 1);

        CollisionCapsuleDef capsule;
        capsule.Radius = 0.5f;

        collisionModel = NewObj<CollisionModel>(&capsule);
    }
    else if (!_Path.Icmp("/Default/Meshes/PlaneXZ"))
    {
        CreatePlaneMeshXZ(vertices, indices, bounds, 256, 256, Float2(256));

        CollisionBoxDef box;
        box.HalfExtents.X = 128;
        box.HalfExtents.Y = 0.1f;
        box.HalfExtents.Z = 128;
        box.Position.Y -= box.HalfExtents.Y;

        collisionModel = NewObj<CollisionModel>(&box);
    }
    else if (!_Path.Icmp("/Default/Meshes/PlaneXY"))
    {
        CreatePlaneMeshXY(vertices, indices, bounds, 256, 256, Float2(256));

        CollisionBoxDef box;
        box.HalfExtents.X = 128;
        box.HalfExtents.Y = 128;
        box.HalfExtents.Z = 0.1f;
        box.Position.Z -= box.HalfExtents.Z;

        collisionModel = NewObj<CollisionModel>(&box);
    }
    else if (!_Path.Icmp("/Default/Meshes/QuadXZ"))
    {
        CreatePlaneMeshXZ(vertices, indices, bounds, 1, 1, Float2(1));

        CollisionBoxDef box;
        box.HalfExtents.X = 0.5f;
        box.HalfExtents.Y = 0.1f;
        box.HalfExtents.Z = 0.5f;
        box.Position.Y -= box.HalfExtents.Y;

        collisionModel = NewObj<CollisionModel>(&box);
    }
    else if (!_Path.Icmp("/Default/Meshes/QuadXY"))
    {
        CreatePlaneMeshXY(vertices, indices, bounds, 1, 1, Float2(1));

        CollisionBoxDef box;
        box.HalfExtents.X = 0.5f;
        box.HalfExtents.Y = 0.5f;
        box.HalfExtents.Z = 0.1f;
        box.Position.Z -= box.HalfExtents.Z;

        collisionModel = NewObj<CollisionModel>(&box);
    }
    else if (!_Path.Icmp("/Default/Meshes/Skybox"))
    {
        CreateSkyboxMesh(vertices, indices, bounds, Float3(1), 1);
    }
    else if (!_Path.Icmp("/Default/Meshes/Skydome"))
    {
        CreateSkydomeMesh(vertices, indices, bounds, 0.5f, 1, 32, 32, false);
    }
    else if (!_Path.Icmp("/Default/Meshes/SkydomeHemisphere"))
    {
        CreateSkydomeMesh(vertices, indices, bounds, 0.5f, 1, 16, 32, true);
    }
    else
    {
        LOG("Unknown internal mesh {}\n", _Path);

        LoadInternalResource("/Default/Meshes/Box");
        return;
    }

    Initialize(vertices.Size(), indices.Size(), 1);
    WriteVertexData(vertices.ToPtr(), vertices.Size(), 0);
    WriteIndexData(indices.ToPtr(), indices.Size(), 0);
    SetBoundingBox(0, bounds);
    SetCollisionModel(collisionModel);
}

void IndexedMesh::GenerateRigidbodyCollisions()
{
    ScopedTimer ScopedTime("GenerateRigidbodyCollisions");

    CollisionTriangleSoupBVHDef bvh;
    bvh.pVertices            = &m_Vertices[0].Position;
    bvh.VertexStride         = sizeof(m_Vertices[0]);
    bvh.VertexCount          = m_Vertices.Size();
    bvh.pIndices             = m_Indices.ToPtr();
    bvh.IndexCount           = m_Indices.Size();
    bvh.pIndexedMeshSubparts = m_Subparts.ToPtr();
    bvh.SubpartCount         = m_Subparts.Size();

    SetCollisionModel(NewObj<CollisionModel>(&bvh));
}

void IndexedMesh::GenerateSoftbodyFacesFromMeshIndices()
{
    ScopedTimer ScopedTime("GenerateSoftbodyFacesFromMeshIndices");

    int totalIndices = 0;

    for (IndexedMeshSubpart const* subpart : m_Subparts)
    {
        totalIndices += subpart->m_IndexCount;
    }

    m_SoftbodyFaces.ResizeInvalidate(totalIndices / 3);

    int faceIndex = 0;

    unsigned int const* indices = m_Indices.ToPtr();

    for (IndexedMeshSubpart const* subpart : m_Subparts)
    {
        for (int i = 0; i < subpart->m_IndexCount; i += 3)
        {
            SoftbodyFace& face = m_SoftbodyFaces[faceIndex++];

            face.Indices[0] = subpart->m_BaseVertex + indices[subpart->m_FirstIndex + i];
            face.Indices[1] = subpart->m_BaseVertex + indices[subpart->m_FirstIndex + i + 1];
            face.Indices[2] = subpart->m_BaseVertex + indices[subpart->m_FirstIndex + i + 2];
        }
    }
}

void IndexedMesh::GenerateSoftbodyLinksFromFaces()
{
    ScopedTimer ScopedTime("GenerateSoftbodyLinksFromFaces");

    TVector<bool> checks;
    unsigned int* indices;

    checks.Resize(m_Vertices.Size() * m_Vertices.Size());
    checks.ZeroMem();

    m_SoftbodyLinks.Clear();

    for (SoftbodyFace& face : m_SoftbodyFaces)
    {
        indices = face.Indices;

        for (int j = 2, k = 0; k < 3; j = k++)
        {

            unsigned int index_j_k = indices[j] + indices[k] * m_Vertices.Size();

            // Check if link not exists
            if (!checks[index_j_k])
            {

                unsigned int index_k_j = indices[k] + indices[j] * m_Vertices.Size();

                // Mark link exits
                checks[index_j_k] = checks[index_k_j] = true;

                // Add link
                SoftbodyLink& link = m_SoftbodyLinks.Add();
                link.Indices[0]     = indices[j];
                link.Indices[1]     = indices[k];
            }
        }
    }
}

bool IndexedMesh::Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const
{
    bool ret = false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, GetBoundingBox(), boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    for (int i = 0; i < m_Subparts.Size(); i++)
    {
        IndexedMeshSubpart* subpart = m_Subparts[i];
        ret |= subpart->Raycast(RayStart, RayDir, invRayDir, Distance, bCullBackFace, HitResult);
    }
    return ret;
}

bool IndexedMesh::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3], int& SubpartIndex) const
{
    bool ret = false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, GetBoundingBox(), boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    for (int i = 0; i < m_Subparts.Size(); i++)
    {
        IndexedMeshSubpart* subpart = m_Subparts[i];
        if (subpart->RaycastClosest(RayStart, RayDir, invRayDir, Distance, bCullBackFace, HitLocation, HitUV, HitDistance, Indices))
        {
            SubpartIndex  = i;
            Distance      = HitDistance;
            ret           = true;
        }
    }

    return ret;
}

void IndexedMesh::DrawBVH(DebugRenderer* pRenderer, Float3x4 const& TransformMatrix)
{
    for (IndexedMeshSubpart* subpart : m_Subparts)
    {
        subpart->DrawBVH(pRenderer, TransformMatrix);
    }
}

void IndexedMesh::NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag)
{
    for (TListIterator<IndexedMeshListener> it(Listeners) ; it ; it++)
    {
        it->OnMeshResourceUpdate(UpdateFlag);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

IndexedMeshSubpart::IndexedMeshSubpart()
{
    m_BoundingBox.Clear();

    static TStaticResourceFinder<MaterialInstance> DefaultMaterialInstance("/Default/MaterialInstance/Default"s);
    m_MaterialInstance = DefaultMaterialInstance.GetObject();
}

IndexedMeshSubpart::~IndexedMeshSubpart()
{
}

void IndexedMeshSubpart::SetBaseVertex(int BaseVertex)
{
    m_BaseVertex     = BaseVertex;
    m_bAABBTreeDirty = true;
}

void IndexedMeshSubpart::SetFirstIndex(int FirstIndex)
{
    m_FirstIndex     = FirstIndex;
    m_bAABBTreeDirty = true;
}

void IndexedMeshSubpart::SetVertexCount(int VertexCount)
{
    m_VertexCount = VertexCount;
}

void IndexedMeshSubpart::SetIndexCount(int IndexCount)
{
    m_IndexCount     = IndexCount;
    m_bAABBTreeDirty = true;
}

void IndexedMeshSubpart::SetMaterialInstance(MaterialInstance* pMaterialInstance)
{
    m_MaterialInstance = pMaterialInstance;

    if (!m_MaterialInstance)
    {
        static TStaticResourceFinder<MaterialInstance> DefaultMaterialInstance("/Default/MaterialInstance/Default"s);
        m_MaterialInstance = DefaultMaterialInstance.GetObject();
    }
}

void IndexedMeshSubpart::SetBoundingBox(BvAxisAlignedBox const& _BoundingBox)
{
    m_BoundingBox = _BoundingBox;

    if (m_OwnerMesh)
    {
        m_OwnerMesh->m_bBoundingBoxDirty = true;
    }
}

void IndexedMeshSubpart::GenerateBVH(unsigned int PrimitivesPerLeaf)
{
    // TODO: Try KD-tree

    if (m_OwnerMesh)
    {
        m_bvhTree        = std::make_unique<BvhTree>(TArrayView<MeshVertex>(m_OwnerMesh->m_Vertices), TArrayView<unsigned int>(m_OwnerMesh->m_Indices.ToPtr() + m_FirstIndex, (size_t)m_IndexCount), m_BaseVertex, PrimitivesPerLeaf);
        m_bAABBTreeDirty = false;
    }
}

void IndexedMeshSubpart::SetBVH(std::unique_ptr<BvhTree> BVH)
{
    m_bvhTree        = std::move(BVH);
    m_bAABBTreeDirty = false;
}

bool IndexedMeshSubpart::Raycast(Float3 const& RayStart, Float3 const& RayDir, Float3 const& InvRayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const
{
    bool                ret = false;
    float               d, u, v;
    unsigned int const* indices  = m_OwnerMesh->GetIndices() + m_FirstIndex;
    MeshVertex const*   vertices = m_OwnerMesh->GetVertices();

    if (Distance < 0.0001f)
    {
        return false;
    }

    if (m_bvhTree)
    {
        if (m_bAABBTreeDirty)
        {
            LOG("IndexedMeshSubpart::Raycast: bvh is outdated\n");
            return false;
        }

        TVector<BvhNode> const& nodes       = m_bvhTree->GetNodes();
        unsigned int const*     indirection = m_bvhTree->GetIndirection();

        float hitMin, hitMax;

        for (int nodeIndex = 0, numNodes = nodes.Size(); nodeIndex < numNodes;)
        {
            BvhNode const* node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox(RayStart, InvRayDir, node->Bounds, hitMin, hitMax) && hitMin <= Distance;
            const bool bLeaf    = node->IsLeaf();

            if (bLeaf && bOverlap)
            {
                for (int t = 0; t < node->PrimitiveCount; t++)
                {
                    const int          triangleNum = node->Index + t;
                    const unsigned int baseInd     = indirection[triangleNum];
                    const unsigned int i0          = m_BaseVertex + indices[baseInd + 0];
                    const unsigned int i1          = m_BaseVertex + indices[baseInd + 1];
                    const unsigned int i2          = m_BaseVertex + indices[baseInd + 2];
                    Float3 const&      v0          = vertices[i0].Position;
                    Float3 const&      v1          = vertices[i1].Position;
                    Float3 const&      v2          = vertices[i2].Position;
                    if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
                    {
                        if (Distance > d)
                        {
                            TriangleHitResult& hitResult = HitResult.Add();
                            hitResult.Location            = RayStart + RayDir * d;
                            hitResult.Normal              = Math::Cross(v1 - v0, v2 - v0).Normalized();
                            hitResult.Distance            = d;
                            hitResult.UV.X                = u;
                            hitResult.UV.Y                = v;
                            hitResult.Indices[0]          = i0;
                            hitResult.Indices[1]          = i1;
                            hitResult.Indices[2]          = i2;
                            hitResult.Material            = m_MaterialInstance;
                            ret                           = true;
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

        if (!BvRayIntersectBox(RayStart, InvRayDir, m_BoundingBox, hitMin, hitMax) || hitMin >= Distance)
        {
            return false;
        }

        const int primCount = m_IndexCount / 3;

        for (int tri = 0; tri < primCount; tri++, indices += 3)
        {
            const unsigned int i0 = m_BaseVertex + indices[0];
            const unsigned int i1 = m_BaseVertex + indices[1];
            const unsigned int i2 = m_BaseVertex + indices[2];

            Float3 const& v0 = vertices[i0].Position;
            Float3 const& v1 = vertices[i1].Position;
            Float3 const& v2 = vertices[i2].Position;

            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (Distance > d)
                {
                    TriangleHitResult& hitResult = HitResult.Add();
                    hitResult.Location            = RayStart + RayDir * d;
                    hitResult.Normal              = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    hitResult.Distance            = d;
                    hitResult.UV.X                = u;
                    hitResult.UV.Y                = v;
                    hitResult.Indices[0]          = i0;
                    hitResult.Indices[1]          = i1;
                    hitResult.Indices[2]          = i2;
                    hitResult.Material            = m_MaterialInstance;
                    ret                           = true;
                }
            }
        }
    }
    return ret;
}

bool IndexedMeshSubpart::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, Float3 const& InvRayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const
{
    bool                ret = false;
    float               d, u, v;
    unsigned int const* indices  = m_OwnerMesh->GetIndices() + m_FirstIndex;
    MeshVertex const*  vertices = m_OwnerMesh->GetVertices();

    if (Distance < 0.0001f)
    {
        return false;
    }

    if (m_bvhTree)
    {
        if (m_bAABBTreeDirty)
        {
            LOG("IndexedMeshSubpart::RaycastClosest: bvh is outdated\n");
            return false;
        }

        TVector<BvhNode> const& nodes       = m_bvhTree->GetNodes();
        unsigned int const*     indirection = m_bvhTree->GetIndirection();

        float hitMin, hitMax;

        for (int nodeIndex = 0, numNodes = nodes.Size(); nodeIndex < numNodes;)
        {
            BvhNode const* node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox(RayStart, InvRayDir, node->Bounds, hitMin, hitMax) && hitMin <= Distance;
            const bool bLeaf    = node->IsLeaf();

            if (bLeaf && bOverlap)
            {
                for (int t = 0; t < node->PrimitiveCount; t++)
                {
                    const int          triangleNum = node->Index + t;
                    const unsigned int baseInd     = indirection[triangleNum];
                    const unsigned int i0          = m_BaseVertex + indices[baseInd + 0];
                    const unsigned int i1          = m_BaseVertex + indices[baseInd + 1];
                    const unsigned int i2          = m_BaseVertex + indices[baseInd + 2];
                    Float3 const&      v0          = vertices[i0].Position;
                    Float3 const&      v1          = vertices[i1].Position;
                    Float3 const&      v2          = vertices[i2].Position;
                    if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
                    {
                        if (Distance > d)
                        {
                            Distance     = d;
                            HitDistance  = d;
                            HitLocation  = RayStart + RayDir * d;
                            HitUV.X      = u;
                            HitUV.Y      = v;
                            Indices[0]   = i0;
                            Indices[1]   = i1;
                            Indices[2]   = i2;
                            ret          = true;
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

        if (!BvRayIntersectBox(RayStart, InvRayDir, m_BoundingBox, hitMin, hitMax) || hitMin >= Distance)
        {
            return false;
        }

        const int primCount = m_IndexCount / 3;

        for (int tri = 0; tri < primCount; tri++, indices += 3)
        {
            const unsigned int i0 = m_BaseVertex + indices[0];
            const unsigned int i1 = m_BaseVertex + indices[1];
            const unsigned int i2 = m_BaseVertex + indices[2];

            Float3 const& v0 = vertices[i0].Position;
            Float3 const& v1 = vertices[i1].Position;
            Float3 const& v2 = vertices[i2].Position;

            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (Distance > d)
                {
                    Distance     = d;
                    HitDistance  = d;
                    HitLocation  = RayStart + RayDir * d;
                    HitUV.X      = u;
                    HitUV.Y      = v;
                    Indices[0]   = i0;
                    Indices[1]   = i1;
                    Indices[2]   = i2;
                    ret          = true;
                }
            }
        }
    }
    return ret;
}

void IndexedMeshSubpart::DrawBVH(DebugRenderer* pRenderer, Float3x4 const& TransformMatrix)
{
    if (!m_bvhTree)
    {
        return;
    }

    pRenderer->SetDepthTest(false);
    pRenderer->SetColor(Color4::White());

    BvOrientedBox orientedBox;

    for (BvhNode const& n : m_bvhTree->GetNodes())
    {
        if (n.IsLeaf())
        {
            orientedBox.FromAxisAlignedBox(n.Bounds, TransformMatrix);
            pRenderer->DrawOBB(orientedBox);
        }
    }
}

void IndexedMeshSubpart::Read(IBinaryStreamReadInterface& stream)
{
    m_Name        = stream.ReadString();
    m_BaseVertex  = stream.ReadInt32();
    m_FirstIndex  = stream.ReadUInt32();
    m_VertexCount = stream.ReadUInt32();
    m_IndexCount  = stream.ReadUInt32();

    stream.ReadObject(m_BoundingBox);

    m_bAABBTreeDirty = true;

    if (m_OwnerMesh)
    {
        m_OwnerMesh->m_bBoundingBoxDirty = true;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

VertexLight::VertexLight(IndexedMesh* pSourceMesh)
{
    m_Vertices.ResizeInvalidate(pSourceMesh->GetVertexCount());

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    m_VertexBufferGPU = vertexMemory->AllocateVertex(m_Vertices.Size() * sizeof(MeshVertexLight), nullptr, GetVertexMemory, this);
}

VertexLight::~VertexLight()
{
    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Deallocate(m_VertexBufferGPU);
    m_VertexBufferGPU = nullptr;
}

bool VertexLight::SendVertexDataToGPU(int VerticesCount, int StartVertexLocation)
{
    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Vertices.Size())
    {
        LOG("VertexLight::SendVertexDataToGPU: Referencing outside of buffer\n");
        return false;
    }

    VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();

    vertexMemory->Update(m_VertexBufferGPU, StartVertexLocation * sizeof(MeshVertexLight), VerticesCount * sizeof(MeshVertexLight), m_Vertices.ToPtr() + StartVertexLocation);

    return true;
}

bool VertexLight::WriteVertexData(MeshVertexLight const* Vertices, int VerticesCount, int StartVertexLocation)
{
    if (!VerticesCount)
    {
        return true;
    }

    if (StartVertexLocation + VerticesCount > m_Vertices.Size())
    {
        LOG("VertexLight::WriteVertexData: Referencing outside of buffer\n");
        return false;
    }

    Platform::Memcpy(m_Vertices.ToPtr() + StartVertexLocation, Vertices, VerticesCount * sizeof(MeshVertexLight));

    return SendVertexDataToGPU(VerticesCount, StartVertexLocation);
}

void VertexLight::GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    if (m_VertexBufferGPU)
    {
        VertexMemoryGPU* vertexMemory = GEngine->GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset(m_VertexBufferGPU, ppBuffer, pOffset);
    }
}

void* VertexLight::GetVertexMemory(void* _This)
{
    return static_cast<VertexLight*>(_This)->GetVertices();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

ProceduralMesh::ProceduralMesh()
{
    BoundingBox.Clear();
}

ProceduralMesh::~ProceduralMesh()
{
}

void ProceduralMesh::GetVertexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    StreamedMemory->GetPhysicalBufferAndOffset(m_VertexStream, ppBuffer, pOffset);
}

void ProceduralMesh::GetIndexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    StreamedMemory->GetPhysicalBufferAndOffset(m_IndexSteam, ppBuffer, pOffset);
}

void ProceduralMesh::PreRenderUpdate(RenderFrontendDef const* pDef)
{
    if (m_VisFrame == pDef->FrameNumber)
    {
        return;
    }

    m_VisFrame = pDef->FrameNumber;

    if (!VertexCache.IsEmpty() && !IndexCache.IsEmpty())
    {
        StreamedMemoryGPU* streamedMemory = pDef->StreamedMemory;

        m_VertexStream = streamedMemory->AllocateVertex(sizeof(MeshVertex) * VertexCache.Size(), VertexCache.ToPtr());
        m_IndexSteam   = streamedMemory->AllocateIndex(sizeof(unsigned int) * IndexCache.Size(), IndexCache.ToPtr());
    }
}

bool ProceduralMesh::Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const
{
    if (Distance < 0.0001f)
    {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool                ret = false;
    float               d, u, v;
    unsigned int const* indices  = IndexCache.ToPtr() + FirstIndex;
    MeshVertex const*  vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for (int tri = 0; tri < primCount; tri++, indices += 3)
    {
        const unsigned int i0 = BaseVertex + indices[0];
        const unsigned int i1 = BaseVertex + indices[1];
        const unsigned int i2 = BaseVertex + indices[2];

        Float3 const& v0 = vertices[i0].Position;
        Float3 const& v1 = vertices[i1].Position;
        Float3 const& v2 = vertices[i2].Position;

        if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
        {
            if (Distance > d)
            {
                TriangleHitResult& hitResult = HitResult.Add();
                hitResult.Location            = RayStart + RayDir * d;
                hitResult.Normal              = Math::Cross(v1 - v0, v2 - v0).Normalized();
                hitResult.Distance            = d;
                hitResult.UV.X                = u;
                hitResult.UV.Y                = v;
                hitResult.Indices[0]          = i0;
                hitResult.Indices[1]          = i1;
                hitResult.Indices[2]          = i2;
                hitResult.Material            = nullptr;
                ret                           = true;
            }
        }
    }

    return ret;
}

bool ProceduralMesh::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const
{
    if (Distance < 0.0001f)
    {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool                ret = false;
    float               d, u, v;
    unsigned int const* indices  = IndexCache.ToPtr() + FirstIndex;
    MeshVertex const*  vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for (int tri = 0; tri < primCount; tri++, indices += 3)
    {
        const unsigned int i0 = BaseVertex + indices[0];
        const unsigned int i1 = BaseVertex + indices[1];
        const unsigned int i2 = BaseVertex + indices[2];

        Float3 const& v0 = vertices[i0].Position;
        Float3 const& v1 = vertices[i1].Position;
        Float3 const& v2 = vertices[i2].Position;

        if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
        {
            if (Distance > d)
            {
                Distance     = d;
                HitLocation  = RayStart + RayDir * d;
                HitDistance  = d;
                HitUV.X      = u;
                HitUV.Y      = v;
                Indices[0]   = i0;
                Indices[1]   = i1;
                Indices[2]   = i2;
                ret          = true;
            }
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void CreateBoxMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale)
{
    constexpr unsigned int indices[6 * 6] =
        {
            0, 1, 2, 2, 3, 0,                                                 // front face
            4, 5, 6, 6, 7, 4,                                                 // back face
            5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face
            1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
            3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
            1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
        };

    Vertices.ResizeInvalidate(24);
    Indices.ResizeInvalidate(36);

    Platform::Memcpy(Indices.ToPtr(), indices, sizeof(indices));

    const Float3 halfSize = Extents * 0.5f;

    Bounds.Mins  = -halfSize;
    Bounds.Maxs = halfSize;

    Float3 const& mins = Bounds.Mins;
    Float3 const& maxs = Bounds.Maxs;

    MeshVertex* pVerts = Vertices.ToPtr();

    Half zero = 0.0f;
    Half pos  = 1.0f;
    Half neg  = -1.0f;

    pVerts[0 + 8 * 0].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[0 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[1 + 8 * 0].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[1 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[2 + 8 * 0].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[2 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[3 + 8 * 0].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[3 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[4 + 8 * 0].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[4 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[5 + 8 * 0].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[5 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[6 + 8 * 0].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[6 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[7 + 8 * 0].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[7 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[0 + 8 * 1].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[0 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[1 + 8 * 1].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[1 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 1].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[2 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[3 + 8 * 1].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[3 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[4 + 8 * 1].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[4 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[5 + 8 * 1].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[5 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[6 + 8 * 1].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[6 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[7 + 8 * 1].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[7 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[1 + 8 * 2].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[1 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[0 + 8 * 2].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[0 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[5 + 8 * 2].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[5 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[4 + 8 * 2].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[4 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);


    pVerts[3 + 8 * 2].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[3 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 2].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[2 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[7 + 8 * 2].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[7 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[6 + 8 * 2].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[6 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateSphereMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    int          x, y;
    float        verticalAngle, horizontalAngle;
    unsigned int quad[4];

    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 4);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 4);

    Vertices.ResizeInvalidate((NumHorizontalSubdivs + 1) * (NumVerticalSubdivs + 1));
    Indices.ResizeInvalidate(NumHorizontalSubdivs * NumVerticalSubdivs * 6);

    Bounds.Mins.X = Bounds.Mins.Y = Bounds.Mins.Z = -Radius;
    Bounds.Maxs.X = Bounds.Maxs.Y = Bounds.Maxs.Z = Radius;

    MeshVertex* pVert = Vertices.ToPtr();

    const float verticalStep    = Math::_PI / NumVerticalSubdivs;
    const float horizontalStep  = Math::_2PI / NumHorizontalSubdivs;
    const float verticalScale   = 1.0f / NumVerticalSubdivs;
    const float horizontalScale = 1.0f / NumHorizontalSubdivs;

    for (y = 0, verticalAngle = -Math::_HALF_PI; y <= NumVerticalSubdivs; y++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position = Float3(scaledR * c, scaledH, scaledR * s);
            pVert->SetTexCoord(Float2(1.0f - static_cast<float>(x) * horizontalScale, 1.0f - static_cast<float>(y) * verticalScale) * TexCoordScale);
            pVert->SetNormal(r * c, h, r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = Indices.ToPtr();
    for (y = 0; y < NumVerticalSubdivs; y++)
    {
        const int y2 = y + 1;
        for (x = 0; x < NumHorizontalSubdivs; x++)
        {
            const int x2 = x + 1;

            quad[0] = y * (NumHorizontalSubdivs + 1) + x;
            quad[1] = y2 * (NumHorizontalSubdivs + 1) + x;
            quad[2] = y2 * (NumHorizontalSubdivs + 1) + x2;
            quad[3] = y * (NumHorizontalSubdivs + 1) + x2;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreatePlaneMeshXZ(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale)
{
    Vertices.ResizeInvalidate(4);
    Indices.ResizeInvalidate(6);

    const float halfWidth  = Width * 0.5f;
    const float halfHeight = Height * 0.5f;

    const MeshVertex Verts[4] = {
        MakeMeshVertex(Float3(-halfWidth, 0, -halfHeight), Float2(0, 0), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0)),
        MakeMeshVertex(Float3(-halfWidth, 0, halfHeight), Float2(0, TexCoordScale.Y), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0)),
        MakeMeshVertex(Float3(halfWidth, 0, halfHeight), Float2(TexCoordScale.X, TexCoordScale.Y), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0)),
        MakeMeshVertex(Float3(halfWidth, 0, -halfHeight), Float2(TexCoordScale.X, 0), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0))};

    Platform::Memcpy(Vertices.ToPtr(), &Verts, 4 * sizeof(MeshVertex));

    constexpr unsigned int indices[6] = {0, 1, 2, 2, 3, 0};
    Platform::Memcpy(Indices.ToPtr(), &indices, sizeof(indices));

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());

    Bounds.Mins.X  = -halfWidth;
    Bounds.Mins.Y  = -0.001f;
    Bounds.Mins.Z  = -halfHeight;
    Bounds.Maxs.X  = halfWidth;
    Bounds.Maxs.Y  = 0.001f;
    Bounds.Maxs.Z  = halfHeight;
}

void CreatePlaneMeshXY(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale)
{
    Vertices.ResizeInvalidate(4);
    Indices.ResizeInvalidate(6);

    const float halfWidth  = Width * 0.5f;
    const float halfHeight = Height * 0.5f;

    const MeshVertex Verts[4] = {
        MakeMeshVertex(Float3(-halfWidth, -halfHeight, 0), Float2(0, TexCoordScale.Y), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1)),
        MakeMeshVertex(Float3(halfWidth, -halfHeight, 0), Float2(TexCoordScale.X, TexCoordScale.Y), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1)),
        MakeMeshVertex(Float3(halfWidth, halfHeight, 0), Float2(TexCoordScale.X, 0), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1)),
        MakeMeshVertex(Float3(-halfWidth, halfHeight, 0), Float2(0, 0), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1))};

    Platform::Memcpy(Vertices.ToPtr(), &Verts, 4 * sizeof(MeshVertex));

    constexpr unsigned int indices[6] = {0, 1, 2, 2, 3, 0};
    Platform::Memcpy(Indices.ToPtr(), &indices, sizeof(indices));

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());

    Bounds.Mins.X  = -halfWidth;
    Bounds.Mins.Y  = -halfHeight;
    Bounds.Mins.Z  = -0.001f;
    Bounds.Maxs.X  = halfWidth;
    Bounds.Maxs.Y  = halfHeight;
    Bounds.Maxs.Z  = 0.001f;
}

void CreatePatchMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 2);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 2);

    const float scaleX = 1.0f / (float)(NumHorizontalSubdivs - 1);
    const float scaleY = 1.0f / (float)(NumVerticalSubdivs - 1);

    const int vertexCount = NumHorizontalSubdivs * NumVerticalSubdivs;
    const int indexCount  = (NumHorizontalSubdivs - 1) * (NumVerticalSubdivs - 1) * 6;

    Float3 normal = Math::Cross(Corner10 - Corner00, Corner01 - Corner00).Normalized();

    Half normalNative[3];
    normalNative[0] = normal.X;
    normalNative[1] = normal.Y;
    normalNative[2] = normal.Z;

    Vertices.ResizeInvalidate(bTwoSided ? vertexCount << 1 : vertexCount);
    Indices.ResizeInvalidate(bTwoSided ? indexCount << 1 : indexCount);

    MeshVertex*  pVert    = Vertices.ToPtr();
    unsigned int* pIndices = Indices.ToPtr();

    for (int y = 0; y < NumVerticalSubdivs; ++y)
    {
        const float  lerpY = y * scaleY;
        const Float3 py0   = Math::Lerp(Corner00, Corner01, lerpY);
        const Float3 py1   = Math::Lerp(Corner10, Corner11, lerpY);
        const float  ty    = lerpY * TexCoordScale;

        for (int x = 0; x < NumHorizontalSubdivs; ++x)
        {
            const float lerpX = x * scaleX;

            pVert->Position = Math::Lerp(py0, py1, lerpX);
            pVert->SetTexCoord(lerpX * TexCoordScale, ty);
            pVert->SetNormal(normalNative[0], normalNative[1], normalNative[2]);

            ++pVert;
        }
    }

    if (bTwoSided)
    {
        normal = -normal;

        normalNative[0] = normal.X;
        normalNative[1] = normal.Y;
        normalNative[2] = normal.Z;

        for (int y = 0; y < NumVerticalSubdivs; ++y)
        {
            const float  lerpY = y * scaleY;
            const Float3 py0   = Math::Lerp(Corner00, Corner01, lerpY);
            const Float3 py1   = Math::Lerp(Corner10, Corner11, lerpY);
            const float  ty    = lerpY * TexCoordScale;

            for (int x = 0; x < NumHorizontalSubdivs; ++x)
            {
                const float lerpX = x * scaleX;

                pVert->Position = Math::Lerp(py0, py1, lerpX);
                pVert->SetTexCoord(lerpX * TexCoordScale, ty);
                pVert->SetNormal(normalNative[0], normalNative[1], normalNative[2]);

                ++pVert;
            }
        }
    }

    for (int y = 0; y < NumVerticalSubdivs; ++y)
    {

        const int index0 = y * NumHorizontalSubdivs;
        const int index1 = (y + 1) * NumHorizontalSubdivs;

        for (int x = 0; x < NumHorizontalSubdivs; ++x)
        {
            const int quad00 = index0 + x;
            const int quad01 = index0 + x + 1;
            const int quad10 = index1 + x;
            const int quad11 = index1 + x + 1;

            if ((x + 1) < NumHorizontalSubdivs && (y + 1) < NumVerticalSubdivs)
            {
                *pIndices++ = quad00;
                *pIndices++ = quad10;
                *pIndices++ = quad11;
                *pIndices++ = quad11;
                *pIndices++ = quad01;
                *pIndices++ = quad00;
            }
        }
    }

    if (bTwoSided)
    {
        for (int y = 0; y < NumVerticalSubdivs; ++y)
        {

            const int index0 = vertexCount + y * NumHorizontalSubdivs;
            const int index1 = vertexCount + (y + 1) * NumHorizontalSubdivs;

            for (int x = 0; x < NumHorizontalSubdivs; ++x)
            {
                const int quad00 = index0 + x;
                const int quad01 = index0 + x + 1;
                const int quad10 = index1 + x;
                const int quad11 = index1 + x + 1;

                if ((x + 1) < NumHorizontalSubdivs && (y + 1) < NumVerticalSubdivs)
                {
                    *pIndices++ = quad00;
                    *pIndices++ = quad01;
                    *pIndices++ = quad11;
                    *pIndices++ = quad11;
                    *pIndices++ = quad10;
                    *pIndices++ = quad00;
                }
            }
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());

    Bounds.Clear();
    Bounds.AddPoint(Corner00);
    Bounds.AddPoint(Corner01);
    Bounds.AddPoint(Corner10);
    Bounds.AddPoint(Corner11);
}

void CreateCylinderMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs)
{
    int          i, j;
    float        angle;
    unsigned int quad[4];

    NumSubdivs = Math::Max(NumSubdivs, 4);

    const float invSubdivs = 1.0f / NumSubdivs;
    const float angleStep  = Math::_2PI * invSubdivs;
    const float halfHeight = Height * 0.5f;

    Vertices.ResizeInvalidate(6 * (NumSubdivs + 1));
    Indices.ResizeInvalidate(3 * NumSubdivs * 6);

    Bounds.Mins.X  = -Radius;
    Bounds.Mins.Z = -Radius;
    Bounds.Mins.Y  = -halfHeight;

    Bounds.Maxs.X  = Radius;
    Bounds.Maxs.Z = Radius;
    Bounds.Maxs.Y  = halfHeight;

    MeshVertex* pVerts = Vertices.ToPtr();

    int firstVertex = 0;

    Half pos  = 1.0f;
    Half neg  = -1.0f;
    Half zero = 0.0f;

    for (j = 0; j <= NumSubdivs; j++)
    {
        pVerts[firstVertex + j].Position = Float3(0.0f, -halfHeight, 0.0f);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, pos, zero);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0; j <= NumSubdivs; j++)
    {
        pVerts[firstVertex + j].Position = Float3(0.0f, halfHeight, 0.0f);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, pos, zero);
    }
    firstVertex += NumSubdivs + 1;

    // generate indices

    unsigned int* pIndices = Indices.ToPtr();

    firstVertex = 0;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < NumSubdivs; j++)
        {
            quad[3] = firstVertex + j;
            quad[2] = firstVertex + j + 1;
            quad[1] = firstVertex + j + 1 + (NumSubdivs + 1);
            quad[0] = firstVertex + j + (NumSubdivs + 1);

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
        firstVertex += (NumSubdivs + 1) * 2;
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateConeMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs)
{
    int          i, j;
    float        angle;
    unsigned int quad[4];

    NumSubdivs = Math::Max(NumSubdivs, 4);

    const float invSubdivs = 1.0f / NumSubdivs;
    const float angleStep  = Math::_2PI * invSubdivs;
    const float halfHeight = Height * 0.5f;

    Vertices.ResizeInvalidate(4 * (NumSubdivs + 1));
    Indices.ResizeInvalidate(2 * NumSubdivs * 6);

    Bounds.Mins.X  = -Radius;
    Bounds.Mins.Z = -Radius;
    Bounds.Mins.Y  = -halfHeight;

    Bounds.Maxs.X  = Radius;
    Bounds.Maxs.Z = Radius;
    Bounds.Maxs.Y  = halfHeight;

    Half neg = -1.0f;
    Half zero = 0.0f;

    MeshVertex* pVerts = Vertices.ToPtr();

    int firstVertex = 0;

    for (j = 0; j <= NumSubdivs; j++)
    {
        pVerts[firstVertex + j].Position = Float3(0, -halfHeight, 0);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
        ;
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    Float3 vx;
    Float3 vy(0, halfHeight, 0);
    Float3 v;
    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(0, halfHeight, 0);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 0.0f) * TexCoordScale);

        vx = Float3(c, 0.0f, s);
        v  = vy - vx;
        pVerts[firstVertex + j].SetNormal(Math::Cross(Math::Cross(v, vx), v).Normalized());

        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    HK_ASSERT(firstVertex == Vertices.Size());

    // generate indices

    unsigned int* pIndices = Indices.ToPtr();

    firstVertex = 0;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < NumSubdivs; j++)
        {
            quad[3] = firstVertex + j;
            quad[2] = firstVertex + j + 1;
            quad[1] = firstVertex + j + 1 + (NumSubdivs + 1);
            quad[0] = firstVertex + j + (NumSubdivs + 1);

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
        firstVertex += (NumSubdivs + 1) * 2;
    }

    HK_ASSERT(pIndices == Indices.ToPtr() + Indices.Size());

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateCapsuleMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    int          x, y, tcY;
    float        verticalAngle, horizontalAngle;
    const float  halfHeight = Height * 0.5f;
    unsigned int quad[4];

    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 4);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 4);

    const int halfVerticalSubdivs = NumVerticalSubdivs >> 1;

    Vertices.ResizeInvalidate((NumHorizontalSubdivs + 1) * (NumVerticalSubdivs + 1) * 2);
    Indices.ResizeInvalidate(NumHorizontalSubdivs * (NumVerticalSubdivs + 1) * 6);

    Bounds.Mins.X = Bounds.Mins.Z = -Radius;
    Bounds.Mins.Y                 = -Radius - halfHeight;
    Bounds.Maxs.X = Bounds.Maxs.Z = Radius;
    Bounds.Maxs.Y                 = Radius + halfHeight;

    MeshVertex* pVert = Vertices.ToPtr();

    const float verticalStep    = Math::_PI / NumVerticalSubdivs;
    const float horizontalStep  = Math::_2PI / NumHorizontalSubdivs;
    const float verticalScale   = 1.0f / (NumVerticalSubdivs + 1);
    const float horizontalScale = 1.0f / NumHorizontalSubdivs;

    tcY = 0;

    for (y = 0, verticalAngle = -Math::_HALF_PI; y <= halfVerticalSubdivs; y++, tcY++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        const float posY    = scaledH - halfHeight;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->SetTexCoord((1.0f - static_cast<float>(x) * horizontalScale) * TexCoordScale,
                               (1.0f - static_cast<float>(tcY) * verticalScale) * TexCoordScale);
            pVert->SetNormal(r * c, h, r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for (y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++, tcY++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        const float posY    = scaledH + halfHeight;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->SetTexCoord((1.0f - static_cast<float>(x) * horizontalScale) * TexCoordScale,
                               (1.0f - static_cast<float>(tcY) * verticalScale) * TexCoordScale);
            pVert->SetNormal(r * c, h, r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = Indices.ToPtr();
    for (y = 0; y <= NumVerticalSubdivs; y++)
    {
        const int y2 = y + 1;
        for (x = 0; x < NumHorizontalSubdivs; x++)
        {
            const int x2 = x + 1;

            quad[0] = y * (NumHorizontalSubdivs + 1) + x;
            quad[1] = y2 * (NumHorizontalSubdivs + 1) + x;
            quad[2] = y2 * (NumHorizontalSubdivs + 1) + x2;
            quad[3] = y * (NumHorizontalSubdivs + 1) + x2;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateSkyboxMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale)
{
    constexpr unsigned int indices[6 * 6] =
        {
            0, 1, 2, 2, 3, 0,                                                 // front face
            4, 5, 6, 6, 7, 4,                                                 // back face
            5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face

            1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
            3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
            1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
        };

    Vertices.ResizeInvalidate(24);
    Indices.ResizeInvalidate(36);

    for (int i = 0; i < 36; i += 3)
    {
        Indices[i]      = indices[i + 2];
        Indices[i + 1]  = indices[i + 1];
        Indices[i + 2]  = indices[i];
    }

    const Float3 halfSize = Extents * 0.5f;

    Bounds.Mins  = -halfSize;
    Bounds.Maxs = halfSize;

    Float3 const& mins = Bounds.Mins;
    Float3 const& maxs = Bounds.Maxs;

    MeshVertex* pVerts = Vertices.ToPtr();

    Half     zero = 0.0f;
    Half     pos  = 1.0f;
    Half     neg  = -1.0f;

    pVerts[0 + 8 * 0].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[0 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[1 + 8 * 0].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[1 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[2 + 8 * 0].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[2 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[3 + 8 * 0].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[3 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[4 + 8 * 0].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[4 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[5 + 8 * 0].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[5 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[6 + 8 * 0].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[6 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[7 + 8 * 0].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[7 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[0 + 8 * 1].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[0 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[1 + 8 * 1].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[1 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 1].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[2 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[3 + 8 * 1].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[3 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[4 + 8 * 1].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[4 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[5 + 8 * 1].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[5 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[6 + 8 * 1].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[6 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[7 + 8 * 1].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[7 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[1 + 8 * 2].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[1 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[0 + 8 * 2].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[0 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[5 + 8 * 2].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[5 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[4 + 8 * 2].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[4 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);


    pVerts[3 + 8 * 2].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[3 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 2].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[2 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[7 + 8 * 2].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[7 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[6 + 8 * 2].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[6 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateSkydomeMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs, bool bHemisphere)
{
    int          x, y;
    float        verticalAngle, horizontalAngle;
    unsigned int quad[4];

    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 4);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 4);

    Vertices.ResizeInvalidate((NumHorizontalSubdivs + 1) * (NumVerticalSubdivs + 1));
    Indices.ResizeInvalidate(NumHorizontalSubdivs * NumVerticalSubdivs * 6);

    Bounds.Mins.X = Bounds.Mins.Y = Bounds.Mins.Z = -Radius;
    Bounds.Maxs.X = Bounds.Maxs.Y = Bounds.Maxs.Z = Radius;

    MeshVertex* pVert = Vertices.ToPtr();

    const float verticalRange   = bHemisphere ? Math::_HALF_PI : Math::_PI;
    const float verticalStep    = verticalRange / NumVerticalSubdivs;
    const float horizontalStep  = Math::_2PI / NumHorizontalSubdivs;
    const float verticalScale   = 1.0f / NumVerticalSubdivs;
    const float horizontalScale = 1.0f / NumHorizontalSubdivs;

    for (y = 0, verticalAngle = bHemisphere ? 0 : -Math::_HALF_PI; y <= NumVerticalSubdivs; y++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position = Float3(scaledR * c, scaledH, scaledR * s);
            pVert->SetTexCoord(Float2(1.0f - static_cast<float>(x) * horizontalScale, 1.0f - static_cast<float>(y) * verticalScale) * TexCoordScale);
            pVert->SetNormal(-r * c, -h, -r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = Indices.ToPtr();
    for (y = 0; y < NumVerticalSubdivs; y++)
    {
        const int y2 = y + 1;
        for (x = 0; x < NumHorizontalSubdivs; x++)
        {
            const int x2 = x + 1;

            quad[0] = y * (NumHorizontalSubdivs + 1) + x;
            quad[1] = y * (NumHorizontalSubdivs + 1) + x2;
            quad[2] = y2 * (NumHorizontalSubdivs + 1) + x2;
            quad[3] = y2 * (NumHorizontalSubdivs + 1) + x;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

MeshRenderView* IndexedMesh::GetDefaultRenderView() const
{
    if (m_RenderView.IsExpired())
    {
        m_RenderView = NewObj<MeshRenderView>();

        m_RenderView->SetMaterials(this);
    }

    return m_RenderView;
}

MeshRenderView::~MeshRenderView()
{
    ClearMaterials();
}

void MeshRenderView::ClearMaterials()
{
    for (auto& material : m_Materials)
    {
        if (material)
            material->RemoveRef();
    }

    m_Materials.Clear();
}

void MeshRenderView::SetMaterial(int subpartIndex, MaterialInstance* instance)
{
    HK_ASSERT(subpartIndex >= 0);

    if (subpartIndex >= m_Materials.Size())
    {
        auto n = m_Materials.Size();
        m_Materials.Resize(n + subpartIndex + 1);
        for (auto i = n, count = m_Materials.Size(); i < count; ++i)
            m_Materials[i] = nullptr;
    }

    MaterialInstance* material = m_Materials[subpartIndex];
    if (material)
        material->RemoveRef();

    m_Materials[subpartIndex] = instance;
    if (instance)
        instance->AddRef();
}

void MeshRenderView::SetMaterials(IndexedMesh const* indexedMesh)
{
    ClearMaterials();

    auto& subparts = indexedMesh->GetSubparts();

    m_Materials.Resize(subparts.Size());
    for (int i = 0, count = subparts.Size(); i < count; ++i)
    {
        m_Materials[i] = subparts[i]->GetMaterialInstance();
        m_Materials[i]->AddRef();
    }
}

MaterialInstance* MeshRenderView::GetMaterialUnsafe(int subpartIndex) const
{
    if (subpartIndex < 0 || subpartIndex >= m_Materials.Size())
        return nullptr;
    return m_Materials[subpartIndex];
}

MaterialInstance* MeshRenderView::GetMaterial(int subpartIndex) const
{
    MaterialInstance* pInstance = GetMaterialUnsafe(subpartIndex);
    if (!pInstance)
    {
        static TStaticResourceFinder<MaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
        pInstance = DefaultInstance.GetObject();
    }
    return pInstance;
}

HK_NAMESPACE_END
