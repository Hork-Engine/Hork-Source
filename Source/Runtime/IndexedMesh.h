/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Geometry/BV/BvhTree.h>
#include <Core/IntrusiveLinkedListMacro.h>

class AIndexedMesh;
struct SVertexHandle;

template <typename VertexType>
using TVertexBufferCPU = TVector<VertexType, Allocators::HeapMemoryAllocator<HEAP_CPU_VERTEX_BUFFER>>;

template <typename IndexType>
using TIndexBufferCPU = TVector<IndexType, Allocators::HeapMemoryAllocator<HEAP_CPU_INDEX_BUFFER>>;

/**

ASocketDef

Socket for attaching

*/
class ASocketDef : public ARefCounted
{
public:
    AString Name;
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

AIndexedMeshSubpart

Part of indexed mesh (submesh / element)

*/
class AIndexedMeshSubpart : public ARefCounted
{
    friend class AIndexedMesh;

public:
    AIndexedMeshSubpart();
    ~AIndexedMeshSubpart();

    void SetName(AStringView Name)
    {
        m_Name = Name;
    }

    AString const& GetName() const
    {
        return m_Name;
    }

    void SetBaseVertex(int BaseVertex);
    void SetFirstIndex(int FirstIndex);
    void SetVertexCount(int VertexCount);
    void SetIndexCount(int IndexCount);
    void SetMaterialInstance(AMaterialInstance* pMaterialInstance);
    void SetBoundingBox(BvAxisAlignedBox const& BoundingBox);

    int                     GetBaseVertex() const { return m_BaseVertex; }
    int                     GetFirstIndex() const { return m_FirstIndex; }
    int                     GetVertexCount() const { return m_VertexCount; }
    int                     GetIndexCount() const { return m_IndexCount; }
    AMaterialInstance*      GetMaterialInstance() { return m_MaterialInstance; }
    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    AIndexedMesh* GetOwner() { return m_OwnerMesh; }

    void GenerateBVH(unsigned int PrimitivesPerLeaf = 16);

    void SetBVH(std::unique_ptr<BvhTree> BVH);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, Float3 const& InvRayDir, float Distance, bool bCullBackFace, TPodVector<STriangleHitResult>& HitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, Float3 const& InvRayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const;

    void DrawBVH(ADebugRenderer* pRenderer, Float3x4 const& TransformMatrix);

    void Read(IBinaryStreamReadInterface& stream);

private:
    AIndexedMesh*            m_OwnerMesh = nullptr;
    BvAxisAlignedBox         m_BoundingBox;
    int                      m_BaseVertex  = 0;
    int                      m_FirstIndex  = 0;
    int                      m_VertexCount = 0;
    int                      m_IndexCount  = 0;
    TRef<AMaterialInstance>  m_MaterialInstance;
    std::unique_ptr<BvhTree> m_bvhTree;
    bool                     m_bAABBTreeDirty = false;
    AString                  m_Name;
};

/**

AVertexLight

Vertex light channel

*/
class AVertexLight : public ARefCounted
{
public:
    AVertexLight(AIndexedMesh* pSourceMesh);
    ~AVertexLight();

    SMeshVertexLight*       GetVertices() { return m_Vertices.ToPtr(); }
    SMeshVertexLight const* GetVertices() const { return m_Vertices.ToPtr(); }
    int                     GetVertexCount() const { return m_Vertices.Size(); }

    bool SendVertexDataToGPU(int VerticesCount, int StartVertexLocation);
    bool WriteVertexData(SMeshVertexLight const* Vertices, int VerticesCount, int StartVertexLocation);

    void GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);

private:
    static void* GetVertexMemory(void* _This);

    SVertexHandle*                     m_VertexBufferGPU = nullptr;
    TVertexBufferCPU<SMeshVertexLight> m_Vertices;
};

using AIndexedMeshSubpartArray = TVector<AIndexedMeshSubpart*>;

struct SSoftbodyLink
{
    unsigned int Indices[2];
};

struct SSoftbodyFace
{
    unsigned int Indices[3];
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

class AIndexedMeshListener
{
public:
    TLink<AIndexedMeshListener> Link;

    virtual void OnMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag) = 0;
};


/**

AIndexedMesh

Triangulated 3d surfaces with indexed vertices
Note that if you modify the mesh, you must call the NotifyMeshResourceUpdate method to let the listeners know that the mesh has been updated.

*/
class AIndexedMesh : public AResource
{
    HK_CLASS(AIndexedMesh, AResource)

    friend class AIndexedMeshSubpart;

public:
    TList<AIndexedMeshListener> Listeners;

    AIndexedMesh();
    ~AIndexedMesh();

    /** Allocate mesh */
    static AIndexedMesh* Create(int NumVertices, int NumIndices, int NumSubparts, bool bSkinnedMesh = false);

    /** Helper. Create box mesh */
    static AIndexedMesh* CreateBox(Float3 const& Extents, float TexCoordScale);

    /** Helper. Create sphere mesh */
    static AIndexedMesh* CreateSphere(float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32);

    /** Helper. Create plane mesh */
    static AIndexedMesh* CreatePlaneXZ(float Width, float Height, float TexCoordScale);

    /** Helper. Create plane mesh */
    static AIndexedMesh* CreatePlaneXY(float Width, float Height, float TexCoordScale);

    /** Helper. Create patch mesh */
    static AIndexedMesh* CreatePatch(Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs);

    /** Helper. Create cylinder mesh */
    static AIndexedMesh* CreateCylinder(float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

    /** Helper. Create cone mesh */
    static AIndexedMesh* CreateCone(float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

    /** Helper. Create capsule mesh */
    static AIndexedMesh* CreateCapsule(float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs = 6, int NumHorizontalSubdivs = 8);

    /** Helper. Create skybox mesh */
    static AIndexedMesh* CreateSkybox(Float3 const& Extents, float TexCoordScale);

    /** Helper. Create skydome mesh */
    static AIndexedMesh* CreateSkydome(float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32, bool bHemisphere = true);

    /** Purge model data */
    void Purge();

    /** Skinned mesh have 4 weights for each vertex */
    bool IsSkinned() const { return m_bSkinnedMesh; }

    /** Get mesh part */
    AIndexedMeshSubpart* GetSubpart(int SubpartIndex);

    /** Add the socket */
    void AddSocket(ASocketDef* Socket);
    // TODO: RemoveSocket?

    /** Find socket by name */
    ASocketDef* FindSocket(AStringView Name);

    /** Get array of sockets */
    TVector<ASocketDef*> const& GetSockets() const { return m_Sockets; }

    /** Set skeleton for the mesh. */
    void SetSkeleton(ASkeleton* pSkeleton);

    /** Skeleton for the mesh. Never return null. */
    ASkeleton* GetSkeleton() const { return m_Skeleton; }

    /** Set mesh skin */
    void SetSkin(int32_t const* pJointIndices, Float3x4 const* pOffsetMatrices, int JointsCount);

    /** Get mesh skin */
    ASkin const& GetSkin() const { return m_Skin; }

    /** Collision model for the mesh. */
    void SetCollisionModel(ACollisionModel* pCollisionModel);

    /** Collision model for the mesh. */
    ACollisionModel* GetCollisionModel() const { return m_CollisionModel; }

    /** Soft body collision model */
    TPodVector<SSoftbodyLink> /*const*/& GetSoftbodyLinks() /*const*/ { return m_SoftbodyLinks; }
    /** Soft body collision model */
    TPodVector<SSoftbodyFace> /*const*/& GetSoftbodyFaces() /*const*/ { return m_SoftbodyFaces; }

    /** Set subpart material */
    void SetMaterialInstance(int SubpartIndex, AMaterialInstance* pMaterialInstance);

    /** Set subpart bounding box */
    void SetBoundingBox(int SubpartIndex, BvAxisAlignedBox const& BoundingBox);

    /** Get mesh vertices */
    SMeshVertex* GetVertices() { return m_Vertices.ToPtr(); }

    /** Get mesh vertices */
    SMeshVertex const* GetVertices() const { return m_Vertices.ToPtr(); }

    /** Get weights for vertex skinning */
    SMeshVertexSkin* GetWeights() { return m_Weights.ToPtr(); }

    /** Get weights for vertex skinning */
    SMeshVertexSkin const* GetWeights() const { return m_Weights.ToPtr(); }

    SMeshVertexUV* GetLigtmapUVs() { return m_LightmapUVs.ToPtr(); }
    SMeshVertexUV const* GetLigtmapUVs() const { return m_LightmapUVs.ToPtr(); }

    /** Get mesh indices */
    unsigned int* GetIndices() { return m_Indices.ToPtr(); }

    /** Get mesh indices */
    unsigned int const* GetIndices() const { return m_Indices.ToPtr(); }

    /** Get total vertex count */
    int GetVertexCount() const { return m_Vertices.Size(); }

    /** Get total index count */
    int GetIndexCount() const { return m_Indices.Size(); }

    /** Get all mesh subparts */
    AIndexedMeshSubpartArray const& GetSubparts() const { return m_Subparts; }

    /** Max primitives per leaf. For raycasting */
    unsigned int GetRaycastPrimitivesPerLeaf() const { return m_RaycastPrimitivesPerLeaf; }

    /** Write vertices at location and send them to GPU */
    bool SendVertexDataToGPU(int VerticesCount, int StartVertexLocation);

    /** Write vertices at location and send them to GPU */
    bool WriteVertexData(SMeshVertex const* Vertices, int VerticesCount, int StartVertexLocation);

    /** Write joint weights at location and send them to GPU */
    bool SendJointWeightsToGPU(int VerticesCount, int StartVertexLocation);

    /** Write joint weights at location and send them to GPU */
    bool WriteJointWeights(SMeshVertexSkin const* Vertices, int VerticesCount, int StartVertexLocation);

    bool SendLightmapUVsToGPU(int VerticesCount, int StartVertexLocation);
    bool WriteLightmapUVsData(SMeshVertexUV const* UVs, int VerticesCount, int StartVertexLocation);

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
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TPodVector<STriangleHitResult>& HitResult) const;

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

    void DrawBVH(ADebugRenderer* pRenderer, Float3x4 const& TransformMatrix);

    void NotifyMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag);

    bool HasLightmapUVs() const { return m_LightmapUVsGPU != nullptr; }

protected:
    void Initialize(int NumVertices, int NumIndices, int NumSubparts, bool bSkinnedMesh = false);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(AStringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Meshes/Box"; }

private:
    static void* GetVertexMemory(void* _This);
    static void* GetIndexMemory(void* _This);
    static void* GetWeightMemory(void* _This);
    static void* GetLightmapUVMemory(void* _This);

    void AddLightmapUVs();

    SVertexHandle*            m_VertexHandle  = nullptr;
    SVertexHandle*            m_IndexHandle   = nullptr;
    SVertexHandle*            m_WeightsHandle = nullptr;
    AIndexedMeshSubpartArray  m_Subparts;

    SVertexHandle*                  m_LightmapUVsGPU = nullptr;
    TVertexBufferCPU<SMeshVertexUV> m_LightmapUVs;

    TVertexBufferCPU<SMeshVertex>     m_Vertices;
    TVertexBufferCPU<SMeshVertexSkin> m_Weights;
    TIndexBufferCPU<unsigned int>     m_Indices;

    TVector<ASocketDef*>      m_Sockets;
    TRef<ASkeleton>           m_Skeleton;
    TRef<ACollisionModel>     m_CollisionModel;
    TPodVector<SSoftbodyLink> m_SoftbodyLinks;
    TPodVector<SSoftbodyFace> m_SoftbodyFaces;
    ASkin                     m_Skin;
    BvAxisAlignedBox          m_BoundingBox;
    uint16_t                  m_RaycastPrimitivesPerLeaf = 16;
    bool                      m_bSkinnedMesh             = false;
    mutable bool              m_bBoundingBoxDirty        = false;
};

class AStreamedMemoryGPU;
struct SRenderFrontendDef;

/**

AProceduralMesh

Runtime-generated procedural mesh.

*/
class AProceduralMesh : public ABaseObject
{
    HK_CLASS(AProceduralMesh, ABaseObject)

public:
    /** Update vertex cache occasionally or every frame */
    TVertexBufferCPU<SMeshVertex> VertexCache;

    /** Update index cache occasionally or every frame */
    TIndexBufferCPU<unsigned int> IndexCache;

    /** Bounding box is used for raycast early exit and VSD culling */
    BvAxisAlignedBox BoundingBox;

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU(AStreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Get mesh GPU buffers */
    void GetIndexBufferGPU(AStreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TPodVector<STriangleHitResult>& HitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const;

    /** Called before rendering. Don't call directly. */
    void PreRenderUpdate(SRenderFrontendDef const* pDef);

    // TODO: Add methods like AddTriangle, AddQuad, etc.

    AProceduralMesh();
    ~AProceduralMesh();

private:
    size_t m_VertexStream = 0;
    size_t m_IndexSteam   = 0;

    int m_VisFrame = -1;
};



/*

Utilites


*/

void CreateBoxMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale);

void CreateSphereMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32);

void CreatePlaneMeshXZ(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, float TexCoordScale);

void CreatePlaneMeshXY(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, float TexCoordScale);

void CreatePatchMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs);

void CreateCylinderMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

void CreateConeMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

void CreateCapsuleMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs = 6, int NumHorizontalSubdivs = 8);

void CreateSkyboxMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale);

void CreateSkydomeMesh(TVertexBufferCPU<SMeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32, bool bHemisphere = true);
