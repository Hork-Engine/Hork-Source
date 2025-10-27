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

#pragma once

#include "Resource.h"

#include <Hork/Core/IntrusiveRef.h>

#include <Hork/RHI/Common/VertexMemoryGPU.h>

#include <Hork/Geometry/BV/BvhTree.h>
#include <Hork/Geometry/Utilites.h>
#include <Hork/Geometry/RawMesh.h>
#include <Hork/Math/Simd/Simd.h>

#include <ozz/animation/runtime/skeleton.h> // TODO: move to cpp

HK_NAMESPACE_BEGIN

struct MeshSurface
{
    uint32_t            BaseVertex = 0;
    uint32_t            FirstIndex = 0;
    uint32_t            VertexCount = 0;
    uint32_t            IndexCount = 0;
    int16_t             SkinIndex = -1;
    uint16_t            JointIndex = 0;
    SimdFloat4x4        InverseTransform = SimdFloat4x4::identity();
    BvAxisAlignedBox    BoundingBox = BvAxisAlignedBox::sEmpty();
    BvhTree             Bvh;

    void                Read(IBinaryStreamReadInterface& stream);
    void                Write(IBinaryStreamWriteInterface& stream) const;
};

struct MeshSkin
{
    uint16_t            FirstMatrix = 0;
    uint16_t            MatrixCount = 0;

    void                Read(IBinaryStreamReadInterface& stream);
    void                Write(IBinaryStreamWriteInterface& stream) const;
};

struct MeshAllocateDesc
{
    uint32_t            SurfaceCount = 0;
    uint32_t            SkinsCount = 0;
    uint32_t            JointReampSize = 0;
    uint32_t            JointCount = 0;
    uint32_t            VertexCount = 0;
    uint32_t            IndexCount = 0;
    bool                HasLightmapChannel = false;
};

template <typename VertexType>
using VertexBufferCPU = Vector<VertexType, Allocators::HeapMemoryAllocator<HEAP_CPU_VERTEX_BUFFER>>;

template <typename IndexType>
using IndexBufferCPU = Vector<IndexType, Allocators::HeapMemoryAllocator<HEAP_CPU_INDEX_BUFFER>>;

using OzzSkeleton = ozz::animation::Skeleton;

struct TriangleHitResult
{
    Float3 Location;
    Float3 Normal;
    Float2 UV;
    float Distance;
    unsigned int Indices[3];
    //Material* MaterialInstance;
};

using VertexBuffer =        VertexBufferCPU<MeshVertex>;
using UvBuffer =            VertexBufferCPU<MeshVertexUV>;
using SkinBuffer =          VertexBufferCPU<SkinVertex>;
using IndexBuffer =         IndexBufferCPU<unsigned int>;

enum class MeshLoadFlags : uint32_t
{
    Default         = 0,
    DontUploadToGpu = HK_BIT(0)
};
HK_FLAG_ENUM_OPERATORS(MeshLoadFlags)

struct MeshData
{
    Vector<MeshSurface>         m_Surfaces;
    Vector<MeshSkin>            m_Skins;
    Vector<uint16_t>            m_JointRemaps;
    Vector<SimdFloat4x4>        m_InverseBindPoses;
    UniqueRef<OzzSkeleton>      m_Skeleton;
    VertexBuffer                m_Vertices;
    SkinBuffer                  m_SkinBuffer;
    UvBuffer                    m_LightmapUVs;
    IndexBuffer                 m_Indices; // TODO: unsigned short, split large meshes to surfaces
    BvAxisAlignedBox            m_BoundingBox;

    void FromRawMesh(RawMesh const& rawMesh);
};

class Mesh : public Resource
{
public:
    using DataType = MeshData;

    static const uint8_t        Type = 1;
    static const uint8_t        Version = 2;

                                Mesh() = default;
                                ~Mesh();

    // Асинхронная загрузка - запускается в фоновом потоке
    static UniqueRef<MeshData>  BeginAsyncLoad(IBinaryStreamReadInterface& stream);

    // Копирование данных из временного хранилища - вызывается только в основном потоке
    void                        InitFromData(UniqueRef<MeshData> meshData, MeshLoadFlags flags = MeshLoadFlags::Default);

    // Синхронная загрузка
    void                        Load(IBinaryStreamReadInterface& stream) override;

    void                        Write(IBinaryStreamWriteInterface& stream) const;

    void                        Purge();

    bool                        HasLightmapUVs() const { return m_LightmapUVsGPU != nullptr; }
    bool                        HasSkinning() const { return !m_SkinBuffer.IsEmpty(); }

    void                        Allocate(MeshAllocateDesc const& desc);

    bool                        WriteVertexData(MeshVertex const* vertices, int vertexCount, int startVertexLocation);
    bool                        WriteSkinningData(SkinVertex const* vertices, int vertexCount, int startVertexLocation);
    bool                        WriteLightmapUVsData(MeshVertexUV const* UVs, int vertexCount, int startVertexLocation);
    bool                        WriteIndexData(unsigned int const* indices, int indexCount, int startIndexLocation);

    MeshVertex const*           GetVertices() const { return m_Vertices.ToPtr(); }
    SkinVertex const*           GetSkinBuffer() const { return m_SkinBuffer.ToPtr(); }
    MeshVertexUV const*         GetLightmapUVs() const { return m_LightmapUVs.ToPtr(); }
    unsigned int const*         GetIndices() const { return m_Indices.ToPtr(); }

    int                         GetVertexCount() const { return m_Vertices.Size(); }
    int                         GetIndexCount() const { return m_Indices.Size(); }

    void                        SetBoundingBox(BvAxisAlignedBox const& boundingBox);
    BvAxisAlignedBox const&     GetBoundingBox() const { return m_BoundingBox; }

    MeshSurface&                LockSurface(uint32_t surfaceIndex) { return m_Surfaces[surfaceIndex]; }

    MeshSurface const*          GetSurfaces() const { return m_Surfaces.ToPtr(); }
    int                         GetSurfaceCount() const { return m_Surfaces.Size(); }

    OzzSkeleton const*          GetSkeleton() const { return m_Skeleton.RawPtr(); }

    uint16_t                    GetJointCount() const;
    const char*                 GetJointName(uint16_t jointIndex) const;
    int16_t                     GetJointParent(uint16_t jointIndex) const;

    int16_t                     FindJoint(StringView name) const;

    Vector<MeshSkin> const&     GetSkins() const { return m_Skins; }
    Vector<uint16_t> const&     GetJointRemaps() const { return m_JointRemaps; }
    Vector<SimdFloat4x4> const& GetInverseBindPoses() const { return m_InverseBindPoses; }

    /// Geometry BVH
    void                        GenerateBVH(uint16_t trianglesPerLeaf = 16);

    /// Get mesh GPU buffers
    void                        GetVertexBufferGPU(RHI::IBuffer** ppBuffer, size_t* pOffset);
    void                        GetSkinBufferBufferGPU(RHI::IBuffer** ppBuffer, size_t* pOffset);
    void                        GetLightmapUVsGPU(RHI::IBuffer** ppBuffer, size_t* pOffset);
    void                        GetIndexBufferGPU(RHI::IBuffer** ppBuffer, size_t* pOffset);

    /// Check ray intersection. Result is unordered by distance to save performance
    bool                        Raycast(Float3 const& rayStart, Float3 const& rayDir, float distance, bool cullBackFace, Vector<TriangleHitResult>& hitResult) const;

    /// Check ray intersection
    bool                        RaycastClosest(Float3 const& rayStart, Float3 const& rayDir, float distance, bool cullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3], int& surfaceIndex) const;

    static void                 SetVertexMemoryGPU(VertexMemoryGPU* vertexMemory);

private:
    static void*                sGetVertexMemory(void* _This);
    static void*                sGetSkinMemory(void* _This);
    static void*                sGetLightmapUVMemory(void* _This);
    static void*                sGetIndexMemory(void* _This);

    void                        CreateGpuBuffers(bool copyFromCPU);
    void                        ReleaseGpuBuffers();
    void                        AddLightmapUVs();

    bool                        Raycast(int surfaceIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool cullBackFace, Vector<TriangleHitResult>& hitResult) const;
    bool                        RaycastClosest(int surfaceIndex, Float3 const& rayStart, Float3 const& rayDir, Float3 const& invRayDir, float distance, bool cullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangle[3]) const;

    Vector<MeshSurface>         m_Surfaces;
    Vector<MeshSkin>            m_Skins;
    Vector<uint16_t>            m_JointRemaps;
    Vector<SimdFloat4x4>        m_InverseBindPoses;
    UniqueRef<OzzSkeleton>      m_Skeleton;
    VertexBuffer                m_Vertices;
    SkinBuffer                  m_SkinBuffer;
    UvBuffer                    m_LightmapUVs;
    IndexBuffer                 m_Indices;
    BvAxisAlignedBox            m_BoundingBox;

    VertexHandle*               m_VertexHandle{};
    VertexHandle*               m_SkinBufferHandle{};
    VertexHandle*               m_LightmapUVsGPU{};
    VertexHandle*               m_IndexHandle{};

    static VertexMemoryGPU*     s_VertexMemory;
};

using MeshRef = IntrusiveRef<Mesh>;

HK_NAMESPACE_END
