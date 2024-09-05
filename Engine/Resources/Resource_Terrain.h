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
    static const uint8_t        Type = RESOURCE_TERRAIN;
    static const uint8_t        Version = 1;

                                TerrainResource() = default;
                                ~TerrainResource();

    static UniqueRef<TerrainResource> sLoad(IBinaryStreamReadInterface& stream);

    bool                        Read(IBinaryStreamReadInterface& stream);

    void                        Upload() override;

    /// Allocate empty height map
    void                        Allocate(uint32_t resolution);

    /// Fill height map data.
    bool                        WriteData(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, const void* pData);

    float                       Sample(float x, float z) const;

    float                       Fetch(int x, int z, int lod) const;

    bool                        GetTriangleVertices(float x, float z, Float3& outV0, Float3& outV1, Float3& outV2) const;
    bool                        GetNormal(float x, float z, Float3& outNormal) const;
    bool                        GetTexcoord(float x, float z, Float2& outTexcoord) const;
    void                        GatherGeometry(BvAxisAlignedBox const& inLocalBounds, Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const;

    Int2 const&                 GetClipMin() const { return m_ClipMin; }
    Int2 const&                 GetClipMax() const { return m_ClipMax; }

    BvAxisAlignedBox const&     GetBoundingBox() const { return m_BoundingBox; }

private:
    void                        GenerateLods();

    uint32_t                    m_Resolution = 0;
    int                         m_NumLods{};
    Int2                        m_ClipMin{};
    Int2                        m_ClipMax{};
    BvAxisAlignedBox            m_BoundingBox;

    // TODO: store heightmap in blocks m*m for cache friendly sampling
    // TODO: проверить, можем ли мы обойтись без лодов без потери в качетсве отображения.
    Vector<HeapBlob>            m_Lods;
};

using TerrainHandle = ResourceHandle<TerrainResource>;

HK_NAMESPACE_END
