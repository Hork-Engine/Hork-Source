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
#include <Hork/Core/UniqueRef.h>

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

struct TerrainData
{
    uint32_t                    m_Resolution = 0;
    int                         m_NumLods{};
    Int2                        m_ClipMin{};
    Int2                        m_ClipMax{};
    BvAxisAlignedBox            m_BoundingBox;

    // TODO: store heightmap in blocks m*m for cache friendly sampling
    // TODO: проверить, можем ли мы обойтись без лодов без потери в качетсве отображения.
    Vector<HeapBlob>            m_Lods;

    void Allocate(uint32_t resolution, float const* data = nullptr)
    {
        HK_ASSERT(IsPowerOfTwo(resolution));

        m_Resolution = resolution;

        // Calc clipping region
        int halfResolutionX = m_Resolution >> 1;
        int halfResolutionY = m_Resolution >> 1;
        m_ClipMin.X = halfResolutionX;
        m_ClipMin.Y = halfResolutionY;
        m_ClipMax.X = halfResolutionX - 1;
        m_ClipMax.Y = halfResolutionY - 1;

        // Calc bounding box
        m_BoundingBox.Mins.X = -m_ClipMin.X;
        m_BoundingBox.Mins.Y = 0;
        m_BoundingBox.Mins.Z = -m_ClipMin.Y;
        m_BoundingBox.Maxs.X = m_ClipMax.X;
        m_BoundingBox.Maxs.Y = 0;
        m_BoundingBox.Maxs.Z = m_ClipMax.Y;

        // Allocate memory for terrain lods
        size_t totalMemoryAllocated{};
        m_NumLods = Math::Log2(m_Resolution) + 1;
        m_Lods.Resize(m_NumLods);
        m_Lods.ShrinkToFit();
        for (int i = 0; i < m_NumLods; i++)
        {
            int sz = 1 << (m_NumLods - i - 1);
            size_t size = sz * sz * sizeof(float);
            if (i == 0)
                m_Lods[i].Reset(size, data);
            else
                m_Lods[i].Reset(size);
            if (data == nullptr)
                m_Lods[i].ZeroMem();
            totalMemoryAllocated += size;
        }

        if (data)
        {
            GenerateLods();

            float minHeight = std::numeric_limits<float>::max();
            float maxHeight = -std::numeric_limits<float>::max();
            for (int y = 0; y < m_Resolution; y++)
                for (int x = 0; x < m_Resolution; x++)
                {
                    float h = data[y * m_Resolution + x];
                    if (h != FLT_MAX)
                    {
                        minHeight = Math::Min(h, minHeight);
                        maxHeight = Math::Max(h, maxHeight);
                    }
                }

            // Update vertical bounds
            m_BoundingBox.Mins.Y = minHeight;
            m_BoundingBox.Maxs.Y = maxHeight;
        }

        LOG("Terrain height field memory usage: {} KB\n", totalMemoryAllocated >> 10);
    }

    void GenerateLods(/* TODO: Add region */);
};

class Terrain : public Resource
{
public:
    using DataType = TerrainData;

    static const uint8_t        Type = 8;
    static const uint8_t        Version = 1;

                                Terrain() = default;

    static UniqueRef<TerrainData> BeginAsyncLoad(IBinaryStreamReadInterface& stream);

    void                        InitFromData(UniqueRef<TerrainData> data);

    void                        Load(IBinaryStreamReadInterface& stream) override;

    void                        Write(IBinaryStreamWriteInterface& stream);

    void                        Purge();

    /// Allocate empty height map
    void                        Allocate(uint32_t resolution, float const* data = nullptr);

    /// Fill height map data.
    bool                        WriteData(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, const void* pData);

    float                       Sample(float x, float z) const;

    float                       Fetch(int x, int z, int lod) const;

    bool                        GetTriangleVertices(float x, float z, Float3& outV0, Float3& outV1, Float3& outV2) const;
    bool                        GetNormal(float x, float z, Float3& outNormal) const;
    bool                        GetTexcoord(float x, float z, Float2& outTexcoord) const;
    void                        GatherGeometry(BvAxisAlignedBox const& inLocalBounds, Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const;

    Int2 const&                 GetClipMin() const { return m_Data.m_ClipMin; }
    Int2 const&                 GetClipMax() const { return m_Data.m_ClipMax; }

    BvAxisAlignedBox const&     GetBoundingBox() const { return m_Data.m_BoundingBox; }

private:
    TerrainData                 m_Data;
};

using TerrainHandle = IntrusiveRef<Terrain>;

HK_NAMESPACE_END
