#pragma once

#include "ResourceHandle.h"
#include "Resource_Skeleton.h"

#include <Engine/RenderCore/VertexMemoryGPU.h>

#include <Engine/Math/VectorMath.h>
#include <Engine/Geometry/VertexFormat.h>
#include <Engine/Geometry/Skinning.h>
#include <Engine/Geometry/BV/BvhTree.h>
#include <Engine/Geometry/Utilites.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;
struct TriangleHitResult;

class ResourceManager;

struct MeshSubpart
{
    uint32_t BaseVertex;
    uint32_t FirstIndex;
    uint32_t VertexCount;
    uint32_t IndexCount;

    BvAxisAlignedBox BoundingBox;
    BvhTree  Bvh;

    void Read(IBinaryStreamReadInterface& stream)
    {
        BaseVertex = stream.ReadUInt32();
        FirstIndex = stream.ReadUInt32();
        VertexCount = stream.ReadUInt32();
        IndexCount = stream.ReadUInt32();
        stream.ReadObject(BoundingBox);
        stream.ReadObject(Bvh);
    }

    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteUInt32(BaseVertex);
        stream.WriteUInt32(FirstIndex);
        stream.WriteUInt32(VertexCount);
        stream.WriteUInt32(IndexCount);
        stream.WriteObject(BoundingBox);
        stream.WriteObject(Bvh);
    }
};

struct MeshSocket
{
    Float3  Position;
    Quat    Rotation;
    Float3  Scale = Float3(1);
    int32_t JointIndex{-1};

    void Read(IBinaryStreamReadInterface& stream)
    {
        stream.ReadObject(Position);
        stream.ReadObject(Rotation);
        stream.ReadObject(Scale);
        JointIndex = stream.ReadInt32();
    }

    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteObject(Position);
        stream.WriteObject(Rotation);
        stream.WriteObject(Scale);
        stream.WriteInt32(JointIndex);
    }
};

//class MeshAsset
//{
//public:
//    TVector<String>           SupbartMaterials;
//    String                    DefaultMaterialInstance;
//
//    // TODO
//    //TRef<CollisionModel> Collision;
//    
//};

class MeshResource : public ResourceBase
{
public:
    static const uint8_t Type = RESOURCE_MESH;
    static const uint8_t Version = 1;

    MeshResource() = default;
    MeshResource(IBinaryStreamReadInterface& stream, ResourceManager* resManager);
    ~MeshResource();

    bool Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager);

    void Write(IBinaryStreamWriteInterface& stream, ResourceManager* resManager) const;

    void Upload() override;

    bool IsSkinned() const { return m_IsSkinned; }

    bool HasLightmapUVs() const { return m_LightmapUVsGPU != nullptr; }

    void Allocate(int vertexCount, int indexCount, int subpartCount, bool bSkinned, bool bWithLightmapUVs);

    /** Write vertices at location and send them to GPU */
    bool WriteVertexData(MeshVertex const* vertices, int vertexCount, int startVertexLocation);
    bool SendVertexDataToGPU(int vertexCount, int startVertexLocation);

    /** Write joint weights at location and send them to GPU */
    bool WriteJointWeights(MeshVertexSkin const* vertices, int vertexCount, int startVertexLocation);

    /** Write lightmap UVs at location and send them to GPU */
    bool WriteLightmapUVsData(MeshVertexUV const* UVs, int vertexCount, int startVertexLocation);

    /** Write indices at location and send them to GPU */
    bool WriteIndexData(unsigned int const* indices, int IndexCount, int startIndexLocation);

    /** Get mesh vertices */
    MeshVertex* GetVertices() { return m_Vertices.ToPtr(); }

    /** Get mesh vertices */
    MeshVertex const* GetVertices() const { return m_Vertices.ToPtr(); }

    /** Get weights for vertex skinning */
    MeshVertexSkin* GetWeights() { return m_Weights.ToPtr(); }

    /** Get weights for vertex skinning */
    MeshVertexSkin const* GetWeights() const { return m_Weights.ToPtr(); }

    MeshVertexUV* GetLightmapUVs() { return m_LightmapUVs.ToPtr(); }
    MeshVertexUV const* GetLightmapUVs() const { return m_LightmapUVs.ToPtr(); }

    /** Get mesh indices */
    unsigned int* GetIndices() { return m_Indices.ToPtr(); }

    /** Get mesh indices */
    unsigned int const* GetIndices() const { return m_Indices.ToPtr(); }

    /** Mesh vertex count */
    int GetVertexCount() const { return m_Vertices.Size(); }

    /** Mesh index count */
    int GetIndexCount() const { return m_Indices.Size(); }

    void SetBoundingBox(BvAxisAlignedBox const& boundingBox);
    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    TVector<MeshSubpart>& GetSubparts() { return m_Subparts; }
    TVector<MeshSubpart> const& GetSubparts() const { return m_Subparts; }
    #if 0
    void SetMaterials(TArrayView<MaterialInstanceHandle> materials);
    void SetMaterial(int subpartIndex, MaterialInstanceHandle handle);

    TVector<MaterialInstanceHandle>& GetMaterials() { return m_Materials; }
    TVector<MaterialInstanceHandle> const& GetMaterials() const { return m_Materials; }
    #endif

    /** Attach skeleton. */
    void SetSkeleton(SkeletonHandle skeleton) { m_Skeleton = skeleton; }

    /** Get attached skeleton. */
    SkeletonHandle GetSkeleton() const { return m_Skeleton; }

    void SetSockets(TArrayView<MeshSocket> sockets);
    TVector<MeshSocket>& GetSockets() { return m_Sockets; }
    TVector<MeshSocket> const& GetSockets() const { return m_Sockets; }

    /** Set mesh skin */
    void SetSkin(MeshSkin skin);

    /** Get mesh skin */
    MeshSkin const& GetSkin() const { return m_Skin; }

    /** Create BVH for raycast optimization. */
    void GenerateBVH(uint16_t primitivesPerLeaf = 16);

    /** Max primitives per leaf used for BVH generation. */
    uint16_t GetBvhPrimitivesPerLeaf() const { return m_BvhPrimitivesPerLeaf; }

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);
    void GetWeightsBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);
    void GetLightmapUVsGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);
    void GetIndexBufferGPU(RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& rayStart, Float3 const& rayDir, float distance, bool bCullBackFace, TVector<TriangleHitResult>& hitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& rayStart, Float3 const& rayDir, float distance, bool bCullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3], int& subpartIndex) const;

    void DrawDebug(DebugRenderer& renderer) const;
    void DrawDebugSubpart(DebugRenderer& renderer, int subpartIndex) const;

//private:
    static void* GetVertexMemory(void* _This);
    static void* GetWeightMemory(void* _This);
    static void* GetLightmapUVMemory(void* _This);
    static void* GetIndexMemory(void* _This);

    void AddLightmapUVs();

    bool SubpartRaycast(int subpartIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool bCullBackFace, TVector<TriangleHitResult>& hitResult) const;
    bool SubpartRaycastClosest(int subpartIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool bCullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3]) const;

    VertexHandle*                    m_VertexHandle{};
    VertexHandle*                    m_WeightsHandle{};
    VertexHandle*                    m_LightmapUVsGPU{};
    VertexHandle*                    m_IndexHandle{};
    TVertexBufferCPU<MeshVertex>     m_Vertices;
    TVertexBufferCPU<MeshVertexSkin> m_Weights;
    TVertexBufferCPU<MeshVertexUV>   m_LightmapUVs;
    TIndexBufferCPU<unsigned int>    m_Indices; // TODO: unsigned short, split large meshes to subparts
    TVector<MeshSubpart>             m_Subparts;
    #if 0
    TVector<MaterialInstanceHandle>  m_Materials;
    #endif
    TVector<MeshSocket>              m_Sockets;
    SkeletonHandle                   m_Skeleton;
    MeshSkin                         m_Skin;
    BvAxisAlignedBox                 m_BoundingBox;
    uint16_t                         m_BvhPrimitivesPerLeaf{16};
    bool                             m_IsSkinned{};
};

using MeshHandle = ResourceHandle<MeshResource>;

HK_NAMESPACE_END
