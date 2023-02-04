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

#pragma once

#include "BaseObject.h"
#include "DebugRenderer.h"
#include "Level.h"
#include "Material.h"
#include "CollisionModel.h"
#include "Skeleton.h"

#include <Engine/Geometry/BV/BvhTree.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

HK_NAMESPACE_BEGIN

class IndexedMesh;
struct VertexHandle;

template <typename VertexType>
using TVertexBufferCPU = TVector<VertexType, Allocators::HeapMemoryAllocator<HEAP_CPU_VERTEX_BUFFER>>;

template <typename IndexType>
using TIndexBufferCPU = TVector<IndexType, Allocators::HeapMemoryAllocator<HEAP_CPU_INDEX_BUFFER>>;

/**

SocketDef

Socket for attaching

*/
class SocketDef : public RefCounted
{
public:
    String Name;
    Float3  Position;
    Float3  Scale = Float3(1);
    Quat    Rotation;
    int     JointIndex{-1};

    void Read(IBinaryStreamReadInterface& stream)
    {
        Name       = stream.ReadString();
        JointIndex = stream.ReadUInt32();
        stream.ReadObject(Position);
        stream.ReadObject(Scale);
        stream.ReadObject(Rotation);
    }
};

/**

IndexedMeshSubpart

Part of indexed mesh (submesh / element)

*/
class IndexedMeshSubpart : public RefCounted
{
    friend class IndexedMesh;

public:
    IndexedMeshSubpart();
    ~IndexedMeshSubpart();

    void SetName(StringView Name)
    {
        m_Name = Name;
    }

    String const& GetName() const
    {
        return m_Name;
    }

    void SetBaseVertex(int BaseVertex);
    void SetFirstIndex(int FirstIndex);
    void SetVertexCount(int VertexCount);
    void SetIndexCount(int IndexCount);
    void SetMaterialInstance(MaterialInstance* pMaterialInstance);
    void SetBoundingBox(BvAxisAlignedBox const& BoundingBox);

    int                     GetBaseVertex() const { return m_BaseVertex; }
    int                     GetFirstIndex() const { return m_FirstIndex; }
    int                     GetVertexCount() const { return m_VertexCount; }
    int                     GetIndexCount() const { return m_IndexCount; }
    MaterialInstance*       GetMaterialInstance() { return m_MaterialInstance; }
    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    IndexedMesh* GetOwner() { return m_OwnerMesh; }

    void GenerateBVH(unsigned int PrimitivesPerLeaf = 16);

    void SetBVH(std::unique_ptr<BvhTree> BVH);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, Float3 const& InvRayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, Float3 const& InvRayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const;

    void DrawBVH(DebugRenderer* pRenderer, Float3x4 const& TransformMatrix);

    void Read(IBinaryStreamReadInterface& stream);

private:
    IndexedMesh*             m_OwnerMesh = nullptr;
    BvAxisAlignedBox         m_BoundingBox;
    int                      m_BaseVertex  = 0;
    int                      m_FirstIndex  = 0;
    int                      m_VertexCount = 0;
    int                      m_IndexCount  = 0;
    TRef<MaterialInstance>   m_MaterialInstance;
    std::unique_ptr<BvhTree> m_bvhTree;
    bool                     m_bAABBTreeDirty = false;
    String                   m_Name;
};

/**

VertexLight

Vertex light channel

*/
class VertexLight : public RefCounted
{
public:
    VertexLight(IndexedMesh* pSourceMesh);
    ~VertexLight();

    MeshVertexLight*       GetVertices() { return m_Vertices.ToPtr(); }
    MeshVertexLight const* GetVertices() const { return m_Vertices.ToPtr(); }
    int                    GetVertexCount() const { return m_Vertices.Size(); }

    bool SendVertexDataToGPU(int VerticesCount, int StartVertexLocation);
    bool WriteVertexData(MeshVertexLight const* Vertices, int VerticesCount, int StartVertexLocation);

    void GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);

private:
    static void* GetVertexMemory(void* _This);

    VertexHandle*                     m_VertexBufferGPU = nullptr;
    TVertexBufferCPU<MeshVertexLight> m_Vertices;
};

using IndexedMeshSubpartArray = TVector<IndexedMeshSubpart*>;

struct SoftbodyLink
{
    unsigned int Indices[2];
};

struct SoftbodyFace
{
    unsigned int Indices[3];
};

class MeshRenderView : public GCObject
{
public:
    ~MeshRenderView();

    /** Unset materials */
    void ClearMaterials();

    /** Set material instance for subpart of the mesh */
    void SetMaterial(MaterialInstance* instance) { SetMaterial(0, instance); }

    /** Set material instance for subpart of the mesh */
    void SetMaterial(int subpartIndex, MaterialInstance* instance);

    /** Set materials from mesh resource */
    void SetMaterials(IndexedMesh const* indexedMesh);

    /** Get material instance of subpart of the mesh. Never return null. */
    MaterialInstance* GetMaterial() const { return GetMaterial(0); }

    /** Get material instance of subpart of the mesh. Never return null. */
    MaterialInstance* GetMaterial(int subpartIndex) const;

    void SetEnabled(bool bEnabled);

    bool IsEnabled() const { return m_bEnabled; }

private:
    MaterialInstance* GetMaterialUnsafe(int subpartIndex) const;

    bool m_bEnabled{true};
    TVector<MaterialInstance*> m_Materials;
};

enum INDEXED_MESH_UPDATE_FLAG : uint8_t
{
    INDEXED_MESH_UPDATE_GEOMETRY = HK_BIT(0),
    INDEXED_MESH_UPDATE_MATERIAL = HK_BIT(1),
    INDEXED_MESH_UPDATE_COLLISION = HK_BIT(2),
    INDEXED_MESH_UPDATE_BOUNDING_BOX = HK_BIT(3),
    INDEXED_MESH_UPDATE_ALL = INDEXED_MESH_UPDATE_FLAG(~0),
};

HK_FLAG_ENUM_OPERATORS(INDEXED_MESH_UPDATE_FLAG)

class IndexedMeshListener
{
public:
    TLink<IndexedMeshListener> Link;

    virtual void OnMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag) = 0;
};


/**

IndexedMesh

Triangulated 3d surfaces with indexed vertices
Note that if you modify the mesh, you must call the NotifyMeshResourceUpdate method to let the listeners know that the mesh has been updated.

*/
class IndexedMesh : public Resource
{
    HK_CLASS(IndexedMesh, Resource)

    friend class IndexedMeshSubpart;

public:
    TList<IndexedMeshListener> Listeners;

    IndexedMesh();
    ~IndexedMesh();

    /** Allocate mesh */
    static IndexedMesh* Create(int NumVertices, int NumIndices, int NumSubparts, bool bSkinnedMesh = false);

    /** Helper. Create box mesh */
    static IndexedMesh* CreateBox(Float3 const& Extents, float TexCoordScale);

    /** Helper. Create sphere mesh */
    static IndexedMesh* CreateSphere(float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32);

    /** Helper. Create plane mesh */
    static IndexedMesh* CreatePlaneXZ(float Width, float Height, Float2 const& TexCoordScale);

    /** Helper. Create plane mesh */
    static IndexedMesh* CreatePlaneXY(float Width, float Height, Float2 const& TexCoordScale);

    /** Helper. Create patch mesh */
    static IndexedMesh* CreatePatch(Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs);

    /** Helper. Create cylinder mesh */
    static IndexedMesh* CreateCylinder(float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

    /** Helper. Create cone mesh */
    static IndexedMesh* CreateCone(float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

    /** Helper. Create capsule mesh */
    static IndexedMesh* CreateCapsule(float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs = 6, int NumHorizontalSubdivs = 8);

    /** Helper. Create skybox mesh */
    static IndexedMesh* CreateSkybox(Float3 const& Extents, float TexCoordScale);

    /** Helper. Create skydome mesh */
    static IndexedMesh* CreateSkydome(float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32, bool bHemisphere = true);

    /** Purge model data */
    void Purge();

    /** Skinned mesh have 4 weights for each vertex */
    bool IsSkinned() const { return m_bSkinnedMesh; }

    /** Get mesh part */
    IndexedMeshSubpart* GetSubpart(int SubpartIndex);

    /** Add the socket */
    void AddSocket(SocketDef* Socket);
    // TODO: RemoveSocket?

    /** Find socket by name */
    SocketDef* FindSocket(StringView Name);

    /** Get array of sockets */
    TVector<SocketDef*> const& GetSockets() const { return m_Sockets; }

    /** Set skeleton for the mesh. */
    void SetSkeleton(Skeleton* pSkeleton);

    /** Skeleton for the mesh. Never return null. */
    Skeleton* GetSkeleton() const { return m_Skeleton; }

    /** Set mesh skin */
    void SetSkin(int32_t const* pJointIndices, Float3x4 const* pOffsetMatrices, int JointsCount);

    /** Get mesh skin */
    MeshSkin const& GetSkin() const { return m_Skin; }

    /** Collision model for the mesh. */
    void SetCollisionModel(CollisionModel* pCollisionModel);

    /** Collision model for the mesh. */
    CollisionModel* GetCollisionModel() const { return m_CollisionModel; }

    MeshRenderView* GetDefaultRenderView() const;

    /** Soft body collision model */
    TVector<SoftbodyLink> /*const*/& GetSoftbodyLinks() /*const*/ { return m_SoftbodyLinks; }
    /** Soft body collision model */
    TVector<SoftbodyFace> /*const*/& GetSoftbodyFaces() /*const*/ { return m_SoftbodyFaces; }

    /** Set subpart material */
    void SetMaterialInstance(int SubpartIndex, MaterialInstance* pMaterialInstance);

    /** Set subpart bounding box */
    void SetBoundingBox(int SubpartIndex, BvAxisAlignedBox const& BoundingBox);

    /** Get mesh vertices */
    MeshVertex* GetVertices() { return m_Vertices.ToPtr(); }

    /** Get mesh vertices */
    MeshVertex const* GetVertices() const { return m_Vertices.ToPtr(); }

    /** Get weights for vertex skinning */
    MeshVertexSkin* GetWeights() { return m_Weights.ToPtr(); }

    /** Get weights for vertex skinning */
    MeshVertexSkin const* GetWeights() const { return m_Weights.ToPtr(); }

    MeshVertexUV* GetLigtmapUVs() { return m_LightmapUVs.ToPtr(); }
    MeshVertexUV const* GetLigtmapUVs() const { return m_LightmapUVs.ToPtr(); }

    /** Get mesh indices */
    unsigned int* GetIndices() { return m_Indices.ToPtr(); }

    /** Get mesh indices */
    unsigned int const* GetIndices() const { return m_Indices.ToPtr(); }

    /** Get total vertex count */
    int GetVertexCount() const { return m_Vertices.Size(); }

    /** Get total index count */
    int GetIndexCount() const { return m_Indices.Size(); }

    /** Get all mesh subparts */
    IndexedMeshSubpartArray const& GetSubparts() const { return m_Subparts; }

    /** Max primitives per leaf. For raycasting */
    unsigned int GetRaycastPrimitivesPerLeaf() const { return m_RaycastPrimitivesPerLeaf; }

    /** Write vertices at location and send them to GPU */
    bool SendVertexDataToGPU(int VerticesCount, int StartVertexLocation);

    /** Write vertices at location and send them to GPU */
    bool WriteVertexData(MeshVertex const* Vertices, int VerticesCount, int StartVertexLocation);

    /** Write joint weights at location and send them to GPU */
    bool SendJointWeightsToGPU(int VerticesCount, int StartVertexLocation);

    /** Write joint weights at location and send them to GPU */
    bool WriteJointWeights(MeshVertexSkin const* Vertices, int VerticesCount, int StartVertexLocation);

    bool SendLightmapUVsToGPU(int VerticesCount, int StartVertexLocation);
    bool WriteLightmapUVsData(MeshVertexUV const* UVs, int VerticesCount, int StartVertexLocation);

    /** Write indices at location and send them to GPU */
    bool SendIndexDataToGPU(int IndexCount, int StartIndexLocation);

    /** Write indices at location and send them to GPU */
    bool WriteIndexData(unsigned int const* Indices, int IndexCount, int StartIndexLocation);

    void UpdateBoundingBox();

    BvAxisAlignedBox const& GetBoundingBox() const;

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);
    void GetIndexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);
    void GetWeightsBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);
    void GetLightmapUVsGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3], int& SubpartIndex) const;

    /** Create BVH for raycast optimization */
    void GenerateBVH(unsigned int PrimitivesPerLeaf = 16);

    /** Generate static collisions */
    void GenerateRigidbodyCollisions();

    /** Generate soft body collisions */
    void GenerateSoftbodyFacesFromMeshIndices();

    /** Generate soft body collisions */
    void GenerateSoftbodyLinksFromFaces();

    void DrawBVH(DebugRenderer* pRenderer, Float3x4 const& TransformMatrix);

    void NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag);

    bool HasLightmapUVs() const { return m_LightmapUVsGPU != nullptr; }

protected:
    void Initialize(int NumVertices, int NumIndices, int NumSubparts, bool bSkinnedMesh = false);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Meshes/Box"; }

private:
    static void* GetVertexMemory(void* _This);
    static void* GetIndexMemory(void* _This);
    static void* GetWeightMemory(void* _This);
    static void* GetLightmapUVMemory(void* _This);

    void AddLightmapUVs();

    VertexHandle*            m_VertexHandle  = nullptr;
    VertexHandle*            m_IndexHandle   = nullptr;
    VertexHandle*            m_WeightsHandle = nullptr;
    IndexedMeshSubpartArray  m_Subparts;
    mutable TWeakRef<MeshRenderView> m_RenderView;

    VertexHandle*                  m_LightmapUVsGPU = nullptr;
    TVertexBufferCPU<MeshVertexUV> m_LightmapUVs;

    TVertexBufferCPU<MeshVertex>     m_Vertices;
    TVertexBufferCPU<MeshVertexSkin> m_Weights;
    TIndexBufferCPU<unsigned int>    m_Indices;

    TVector<SocketDef*>      m_Sockets;
    TRef<Skeleton>           m_Skeleton;
    TRef<CollisionModel>     m_CollisionModel;
    TVector<SoftbodyLink>    m_SoftbodyLinks;
    TVector<SoftbodyFace>    m_SoftbodyFaces;
    MeshSkin                 m_Skin;
    BvAxisAlignedBox         m_BoundingBox;
    uint16_t                 m_RaycastPrimitivesPerLeaf = 16;
    bool                     m_bSkinnedMesh             = false;
    mutable bool             m_bBoundingBoxDirty        = false;
};

class StreamedMemoryGPU;
struct RenderFrontendDef;

/**

ProceduralMesh

Runtime-generated procedural mesh.

*/
class ProceduralMesh : public BaseObject
{
    HK_CLASS(ProceduralMesh, BaseObject)

public:
    /** Update vertex cache occasionally or every frame */
    TVertexBufferCPU<MeshVertex> VertexCache;

    /** Update index cache occasionally or every frame */
    TIndexBufferCPU<unsigned int> IndexCache;

    /** Bounding box is used for raycast early exit and VSD culling */
    BvAxisAlignedBox BoundingBox;

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Get mesh GPU buffers */
    void GetIndexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const;

    /** Called before rendering. Don't call directly. */
    void PreRenderUpdate(RenderFrontendDef const* pDef);

    // TODO: Add methods like AddTriangle, AddQuad, etc.

    ProceduralMesh();
    ~ProceduralMesh();

private:
    size_t m_VertexStream = 0;
    size_t m_IndexSteam   = 0;

    int m_VisFrame = -1;
};



/*

Utilites


*/

void CreateBoxMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale);

void CreateSphereMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32);

void CreatePlaneMeshXZ(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale);

void CreatePlaneMeshXY(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale);

void CreatePatchMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs);

void CreateCylinderMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

void CreateConeMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

void CreateCapsuleMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs = 6, int NumHorizontalSubdivs = 8);

void CreateSkyboxMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale);

void CreateSkydomeMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32, bool bHemisphere = true);

HK_NAMESPACE_END
