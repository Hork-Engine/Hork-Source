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

#include "Terrain.h"

#include <Hork/Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

namespace
{
    void DownsampleHeightMap(int inSourceResolution, const float* inSourceMap, float* outDestMap)
    {
        HK_ASSERT((inSourceResolution & 1) == 0);

        float h1, h2, h3, h4;
        int x, y;

        int lod_resolution = inSourceResolution >> 1;

        for (y = 0; y < lod_resolution; y++)
        {
            int src_y = y << 1;
            for (x = 0; x < lod_resolution; x++)
            {
                int src_x = x << 1;
                h1 = inSourceMap[src_y * inSourceResolution + src_x];
                h2 = inSourceMap[src_y * inSourceResolution + src_x + 1];
                h3 = inSourceMap[(src_y + 1) * inSourceResolution + src_x];
                h4 = inSourceMap[(src_y + 1) * inSourceResolution + src_x + 1];

                float result = 0;
                float count = 0;
                if (h1 != FLT_MAX)
                    result += h1, count++;
                if (h2 != FLT_MAX)
                    result += h2, count++;
                if (h3 != FLT_MAX)
                    result += h3, count++;
                if (h4 != FLT_MAX)
                    result += h4, count++;

                if (count > 0)
                    result /= count;
                else
                    result = FLT_MAX;

                outDestMap[y * lod_resolution + x] = result;
            }
        }
    }
}

void TerrainData::GenerateLods(/* TODO: Add region */)
{
    for (int i = 1; i < m_NumLods; i++)
        DownsampleHeightMap(1 << (m_NumLods - i), (const float*)m_Lods[i - 1].GetData(), (float*)m_Lods[i].GetData());
}

UniqueRef<TerrainData> Terrain::BeginAsyncLoad(IBinaryStreamReadInterface& stream)
{
    //uint32_t fileMagic = stream.ReadUInt32();

    //if (fileMagic != MakeResourceMagic(Type, Version))
    //{
    //    LOG("Unexpected file format\n");
    //    return false;
    //}

    // TODO: read heightmap

#if 0
    Allocate(1024, stream.AsBlob().GetData());
#endif

    UniqueRef<TerrainData> data = MakeUnique<TerrainData>();
    data->Allocate(1024);

    return data;
}

void Terrain::InitFromData(UniqueRef<TerrainData> data)
{
    if (!data)
        return;

    m_Data = std::move(*data);

    m_IsPurged = false;
}

void Terrain::Load(IBinaryStreamReadInterface& stream)
{
    auto tempData = BeginAsyncLoad(stream);
    if (tempData)
        InitFromData(std::move(tempData));
}

void Terrain::Write(IBinaryStreamWriteInterface& stream)
{
    // TODO
}

void Terrain::Purge()
{
    m_Data.m_Lods.Free();
   
    m_IsPurged = true;
}

void Terrain::Allocate(uint32_t resolution, float const* data)
{
    m_Data.Allocate(resolution, data);

    m_IsPurged = false;
}

bool Terrain::WriteData(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, const void* pData)
{
    // TODO
    return true;
}

float Terrain::Sample(float x, float z) const
{
    float minX = Math::Floor(x);
    float minZ = Math::Floor(z);

    int quadX = minX + (m_Data.m_Resolution >> 1);
    int quadZ = minZ + (m_Data.m_Resolution >> 1);

    if (quadX < 0 || quadX >= m_Data.m_Resolution - 1)
        return 0.0f;
    if (quadZ < 0 || quadZ >= m_Data.m_Resolution - 1)
        return 0.0f;

    /*

    h0       h1
    +--------+
    |        |
    |        |
    |        |
    +--------+
    h3       h2

    */

    const float* data = (const float*)m_Data.m_Lods[0].GetData();

    float h1 = data[quadZ * m_Data.m_Resolution + quadX + 1];
    float h3 = data[(quadZ + 1) * m_Data.m_Resolution + quadX];

    if (h1 == FLT_MAX || h3 == FLT_MAX)
        return 0;

    float fx = x - minX;
    float fz = z - minZ;

    fz = 1.0f - fz;
    if (fx >= fz)
    {
        float h2 = data[(quadZ + 1) * m_Data.m_Resolution + quadX + 1];
        if (h2 == FLT_MAX)
            return 0;
        float u = fz;
        float v = fx - fz;
        float w = 1.0f - fx;
        return h1 * u + h2 * v + h3 * w;
    }
    else
    {
        float h0 = data[quadZ * m_Data.m_Resolution + quadX];
        if (h0 == FLT_MAX)
            return 0;
        float u = fz - fx;
        float v = fx;
        float w = 1.0f - fz;
        return h0 * u + h1 * v + h3 * w;
    }
}

float Terrain::Fetch(int x, int z, int lod) const
{
    if (lod < 0 || lod >= m_Data.m_NumLods)
        return 0.0f;

    int sampleX = x >> lod;
    int sampleY = z >> lod;

    int lodResoultion = 1 << (m_Data.m_NumLods - lod - 1);

    sampleX = Math::Clamp(sampleX + (lodResoultion >> 1), 0, (lodResoultion - 1));
    sampleY = Math::Clamp(sampleY + (lodResoultion >> 1), 0, (lodResoultion - 1));
    return ((const float*)m_Data.m_Lods[lod].GetData())[sampleY * lodResoultion + sampleX];
}

bool Terrain::GetTriangleVertices(float x, float z, Float3& outV0, Float3& outV1, Float3& outV2) const
{
    float minX = Math::Floor(x);
    float minZ = Math::Floor(z);

    int quadX = minX + (m_Data.m_Resolution >> 1);
    int quadZ = minZ + (m_Data.m_Resolution >> 1);

    if (quadX < 0 || quadX >= m_Data.m_Resolution - 1)
        return false;

    if (quadZ < 0 || quadZ >= m_Data.m_Resolution - 1)
        return false;

    /*

    h0       h1
    +--------+
    |        |
    |        |
    |        |
    +--------+
    h3       h2

    */

    const float* data = (const float*)m_Data.m_Lods[0].GetData();

    float h0 = data[quadZ * m_Data.m_Resolution + quadX];
    float h1 = data[quadZ * m_Data.m_Resolution + quadX + 1];
    float h2 = data[(quadZ + 1) * m_Data.m_Resolution + quadX + 1];
    float h3 = data[(quadZ + 1) * m_Data.m_Resolution + quadX];

    float maxX = minX + 1.0f;
    float maxZ = minZ + 1.0f;

    float fractX = x - minX;
    float fractZ = z - minZ;

    if (fractZ < 1.0f - fractX)
    {
        outV0.X = minX;
        outV0.Y = h0;
        outV0.Z = minZ;

        outV1.X = minX;
        outV1.Y = h3;
        outV1.Z = maxZ;

        outV2.X = maxX;
        outV2.Y = h1;
        outV2.Z = minZ;
    }
    else
    {
        outV0.X = minX;
        outV0.Y = h3;
        outV0.Z = maxZ;

        outV1.X = maxX;
        outV1.Y = h2;
        outV1.Z = maxZ;

        outV2.X = maxX;
        outV2.Y = h1;
        outV2.Z = minZ;
    }

    return true;
}

bool Terrain::GetNormal(float x, float z, Float3& outNormal) const
{
    Float3 v0, v1, v2;
    if (!GetTriangleVertices(x, z, v0, v1, v2))
        return false;
    outNormal = Math::Cross(v1 - v0, v2 - v0).Normalized();
    return true;
}

bool Terrain::GetTexcoord(float x, float z, Float2& outTexcoord) const
{
    const float invResolution = 1.0f / m_Data.m_Resolution;

    outTexcoord.X = Math::Clamp(x * invResolution + 0.5f, 0.0f, 1.0f);
    outTexcoord.Y = Math::Clamp(z * invResolution + 0.5f, 0.0f, 1.0f);

    return true;
}

void Terrain::GatherGeometry(BvAxisAlignedBox const& inLocalBounds, Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const
{
    if (!BvBoxOverlapBox(m_Data.m_BoundingBox, inLocalBounds))
        return;

    float minX = Math::Floor(inLocalBounds.Mins.X);
    float minZ = Math::Floor(inLocalBounds.Mins.Z);
    float maxX = Math::Ceil(inLocalBounds.Maxs.X);
    float maxZ = Math::Ceil(inLocalBounds.Maxs.Z);
    float minY = inLocalBounds.Mins.Y;
    float maxY = inLocalBounds.Maxs.Y;

    int halfResolution = m_Data.m_Resolution >> 1;

    int minQuadX = minX + halfResolution;
    int minQuadZ = minZ + halfResolution;
    int maxQuadX = maxX + halfResolution;
    int maxQuadZ = maxZ + halfResolution;

    minQuadX = Math::Max(minQuadX, 0);
    minQuadZ = Math::Max(minQuadZ, 0);
    maxQuadX = Math::Min(maxQuadX, (int)m_Data.m_Resolution - 1);
    maxQuadZ = Math::Min(maxQuadZ, (int)m_Data.m_Resolution - 1);

    int n = outVertices.Size();

    const float* data = (const float*)m_Data.m_Lods[0].GetData();

    for (int qz = minQuadZ; qz < maxQuadZ; qz++)
    {
        float z = qz - halfResolution;

        float h0 = data[qz * m_Data.m_Resolution + minQuadX];
        float h3 = data[(qz + 1) * m_Data.m_Resolution + minQuadX];

        for (int qx = minQuadX; qx < maxQuadX; qx++)
        {
            float x = qx - halfResolution;

            /*

            h0       h1
            +--------+
            |     /  |
            |   /    |
            | /      |
            +--------+
            h3       h2

            */

            float h1 = data[qz * m_Data.m_Resolution + qx + 1];
            float h2 = data[(qz + 1) * m_Data.m_Resolution + qx + 1];

            // Check shared vertices
            if (h1 != FLT_MAX && h3 != FLT_MAX)
            {
                bool firstTriangleCut = false;

                // Triangle h0 h3 h1
                if (h0 != FLT_MAX &&
                    ((h0 >= minY && h0 <= maxY) ||
                        (h3 >= minY && h3 <= maxY) ||
                        (h1 >= minY && h1 <= maxY)))
                {
                    // emit triangle

                    outVertices.EmplaceBack(x, h0, z);
                    outVertices.EmplaceBack(x, h3, z + 1);
                    outVertices.EmplaceBack(x + 1, h1, z);

                    outIndices.Add(n);
                    outIndices.Add(n + 1);
                    outIndices.Add(n + 2);
                    n += 3;
                }
                else
                    firstTriangleCut = true;

                // Triangle h1 h3 h2
                if (h2 != FLT_MAX &&
                    ((h1 >= minY && h1 <= maxY) ||
                        (h3 >= minY && h3 <= maxY) ||
                        (h2 >= minY && h2 <= maxY)))
                {
                    // emit triangle
                    if (firstTriangleCut)
                    {
                        outVertices.EmplaceBack(x + 1, h1, z);
                        outVertices.EmplaceBack(x, h3, z + 1);
                        outVertices.EmplaceBack(x + 1, h2, z + 1);

                        outIndices.Add(n);
                        outIndices.Add(n + 1);
                        outIndices.Add(n + 2);
                        n += 3;
                    }
                    else
                    {
                        outVertices.EmplaceBack(x + 1, h2, z + 1);

                        outIndices.Add(n - 1);
                        outIndices.Add(n - 2);
                        outIndices.Add(n);
                        n++;
                    }
                }
            }

            h0 = h1;
            h3 = h2;
        }
    }
}

HK_NAMESPACE_END
