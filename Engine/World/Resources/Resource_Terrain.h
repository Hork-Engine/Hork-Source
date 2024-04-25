#pragma once

#include "ResourceHandle.h"
#include "ResourceBase.h"

#include <Engine/Core/BinaryStream.h>
#include <Engine/Core/Containers/Vector.h>
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

class TerrainResource : public ResourceBase
{
public:
    static const uint8_t Type = RESOURCE_TERRAIN;
    static const uint8_t Version = 1;

    TerrainResource() = default;
    TerrainResource(IBinaryStreamReadInterface& stream, class ResourceManager* resManager);
    ~TerrainResource();

    bool Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager);

    void Upload() override;

    /** Allocate empty height map */
    void Allocate(uint32_t resolution);

    /** Fill height map data. */
    bool WriteData(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, const void* pData);

    float Sample(float x, float z) const;

    float Fetch(int x, int z, int lod) const;

    bool GetTriangleVertices(float x, float z, Float3& outV0, Float3& outV1, Float3& outV2) const;
    bool GetNormal(float x, float z, Float3& outNormal) const;
    bool GetTexcoord(float x, float z, Float2& outTexcoord) const;
    void GatherGeometry(BvAxisAlignedBox const& inLocalBounds, TVector<Float3>& outVertices, TVector<unsigned int>& outIndices) const;

    Int2 const& GetClipMin() const { return m_ClipMin; }
    Int2 const& GetClipMax() const { return m_ClipMax; }

    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

private:
    void GenerateLods();

    uint32_t m_Resolution = 0;
    int m_NumLods{};
    Int2 m_ClipMin{};
    Int2 m_ClipMax{};
    BvAxisAlignedBox m_BoundingBox;

    // TODO: store heightmap in blocks m*m for cache friendly sampling
    // TODO: проверить, можем ли мы обойтись без лодов без потери в качетсве отображения.
    TVector<HeapBlob> m_Lods;
};

using TerrainHandle = ResourceHandle<TerrainResource>;

HK_NAMESPACE_END
