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

#if 0

/*

NOTE: The terrain is still in the early development stage.


TODO:

Textures update
Streaming
Frustum culling:
 Calc ZMin, ZMax for each block. The calc can be made approximately,
 based on the height of the center:
   zMin = centerH - blockSize * f;
   zMax = centerH + blockSize * f;
   f - some value that gives a margin

FIXME: move normal texture fetching to fragment shader?

Future:
 Precalculate occluders inside mountains so that invisible objects can be cut off.

*/

#include "Terrain.h"

#include <Engine/Geometry/BV/BvIntersect.h>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "BulletCompatibility.h"

HK_NAMESPACE_BEGIN

HK_CLASS_META(Terrain)

Terrain::~Terrain()
{
    Purge();
}

Terrain::Terrain(int Resolution, const float* pData)
{
    if (!IsPowerOfTwo(Resolution - 1))
    {
        LOG("Terrain::ctor: invalid resolution\n");
        return;
    }

    m_HeightmapResolution = Resolution;

    // Allocate memory for terrain lods
    m_HeightmapLods = Math::Log2((uint32_t)(m_HeightmapResolution - 1)) + 1;
    m_Heightmap.Resize(m_HeightmapLods);
    for (int i = 0; i < m_HeightmapLods; i++)
    {
        int sz = 1 << (m_HeightmapLods - i - 1);
        m_Heightmap[i] = (float*)Core::GetHeapAllocator<HEAP_MISC>().Alloc((sz + 1) * (sz + 1) * sizeof(float));
    }

    // Read most detailed lod
    Core::Memcpy(m_Heightmap[0], pData, m_HeightmapResolution * m_HeightmapResolution * sizeof(float));

    GenerateLods();
    UpdateTerrainBounds();
    UpdateTerrainShape();
    NotifyTerrainResourceUpdate(TERRAIN_UPDATE_ALL);
}

bool Terrain::LoadResource(IBinaryStreamReadInterface& Stream)
{
    Purge();

    // TODO: Heightmap resolution should be specified in asset file
    m_HeightmapResolution = 4097;

    // Allocate memory for terrain lods
    m_HeightmapLods = Math::Log2((uint32_t)(m_HeightmapResolution - 1)) + 1;
    m_Heightmap.Resize(m_HeightmapLods);
    for (int i = 0; i < m_HeightmapLods; i++)
    {
        int sz = 1 << (m_HeightmapLods - i - 1);
        m_Heightmap[i] = (float*)Core::GetHeapAllocator<HEAP_MISC>().Alloc((sz + 1) * (sz + 1) * sizeof(float));
    }

    // Read most detailed lod
    Stream.Read(m_Heightmap[0], m_HeightmapResolution * m_HeightmapResolution * sizeof(float));

    GenerateLods();
    UpdateTerrainBounds();
    UpdateTerrainShape();
    NotifyTerrainResourceUpdate(TERRAIN_UPDATE_ALL);

    return true;
}

void Terrain::LoadInternalResource(StringView Path)
{
    Purge();

    // Create some dummy terrain

    m_HeightmapResolution = 33;

    // Allocate memory for terrain lods
    m_HeightmapLods = Math::Log2((uint32_t)(m_HeightmapResolution - 1)) + 1;
    m_Heightmap.Resize(m_HeightmapLods);
    for (int i = 0; i < m_HeightmapLods; i++)
    {
        int sz = 1 << (m_HeightmapLods - i - 1);
        m_Heightmap[i] = (float*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(float) * (sz + 1) * (sz + 1), 0, MALLOC_ZERO);
    }

    m_MinHeight = -0.1f;
    m_MaxHeight = 0.1f;

    // Calc clipping region
    int halfResolution = m_HeightmapResolution >> 1;
    m_ClipMin.X = halfResolution;
    m_ClipMin.Y = halfResolution;
    m_ClipMax.X = halfResolution;
    m_ClipMax.Y = halfResolution;

    // Calc bounding box
    m_BoundingBox.Mins.X = -m_ClipMin.X;
    m_BoundingBox.Mins.Y = m_MinHeight;
    m_BoundingBox.Mins.Z = -m_ClipMin.Y;
    m_BoundingBox.Maxs.X = m_ClipMax.X;
    m_BoundingBox.Maxs.Y = m_MaxHeight;
    m_BoundingBox.Maxs.Z = m_ClipMax.Y;

    UpdateTerrainShape();
    NotifyTerrainResourceUpdate(TERRAIN_UPDATE_ALL);
}

void Terrain::Purge()
{
    for (int i = 0; i < m_HeightmapLods; i++)
    {
        Core::GetHeapAllocator<HEAP_MISC>().Free(m_Heightmap[i]);
    }

    m_HeightmapLods = 0;
    m_Heightmap.Clear();

    delete m_HeightfieldShape;
    m_HeightfieldShape = nullptr;
}

void Terrain::GenerateLods()
{
    float h1, h2, h3, h4;
    int x, y;
    for (int i = 1; i < m_HeightmapLods; i++)
    {
        float* lod = m_Heightmap[i];
        float* srcLod = m_Heightmap[i - 1];

        int sz = 1 << (m_HeightmapLods - i - 1);
        int sz2 = 1 << (m_HeightmapLods - i);

        sz++;
        sz2++;

        for (y = 0; y < sz - 1; y++)
        {
            int src_y = y << 1;
            for (x = 0; x < sz - 1; x++)
            {
                int src_x = x << 1;
                h1 = srcLod[src_y * sz2 + src_x];
                h2 = srcLod[src_y * sz2 + src_x + 1];
                h3 = srcLod[(src_y + 1) * sz2 + src_x];
                h4 = srcLod[(src_y + 1) * sz2 + src_x + 1];

                lod[y * sz + x] = (h1 + h2 + h3 + h4) * 0.25f;
            }

            int src_x = x << 1;
            h1 = srcLod[src_y * sz2 + src_x];
            h2 = srcLod[(src_y + 1) * sz2 + src_x];
            lod[y * sz + x] = (h1 + h2) * 0.5f;
        }

        int src_y = y << 1;
        for (x = 0; x < sz - 1; x++)
        {
            int src_x = x << 1;
            h1 = srcLod[src_y * sz2 + src_x];
            h2 = srcLod[src_y * sz2 + src_x + 1];

            lod[y * sz + x] = (h1 + h2) * 0.5f;
        }

        int src_x = x << 1;
        lod[y * sz + x] = srcLod[src_y * sz2 + src_x];
    }
}

void Terrain::UpdateTerrainBounds()
{
    // Calc Min/Max height. TODO: should be specified in asset file
    m_MinHeight = std::numeric_limits<float>::max();
    m_MaxHeight = -std::numeric_limits<float>::max();
    for (int y = 0; y < m_HeightmapResolution; y++)
    {
        for (int x = 0; x < m_HeightmapResolution; x++)
        {
            float h = m_Heightmap[0][y * m_HeightmapResolution + x];
            m_MinHeight = Math::Min(m_MinHeight, h);
            m_MaxHeight = Math::Max(m_MaxHeight, h);
        }
    }

    // Calc clipping region
    int halfResolution = m_HeightmapResolution >> 1;
    m_ClipMin.X = halfResolution;
    m_ClipMin.Y = halfResolution;
    m_ClipMax.X = halfResolution;
    m_ClipMax.Y = halfResolution;

    // Calc bounding box
    m_BoundingBox.Mins.X = -m_ClipMin.X;
    m_BoundingBox.Mins.Y = m_MinHeight;
    m_BoundingBox.Mins.Z = -m_ClipMin.Y;
    m_BoundingBox.Maxs.X = m_ClipMax.X;
    m_BoundingBox.Maxs.Y = m_MaxHeight;
    m_BoundingBox.Maxs.Z = m_ClipMax.Y;
}

void Terrain::UpdateTerrainShape()
{
    /*
    NOTE about btHeightfieldTerrainShape:
      The caller is responsible for maintaining the heightfield array; this
      class does not make a copy.

      The heightfield can be dynamic so long as the min/max height values
      capture the extremes (heights must always be in that range).
    */

    // Generate accelerated terrain collision shape
    delete m_HeightfieldShape;
    m_HeightfieldShape = new btHeightfieldTerrainShape(m_HeightmapResolution, m_HeightmapResolution, m_Heightmap[0], 1, m_MinHeight, m_MaxHeight, 1, PHY_FLOAT, false /* bFlipQuadEdges */);
    m_HeightfieldShape->buildAccelerator();
}

float Terrain::ReadHeight(int X, int Z, int Lod) const
{
    if (Lod < 0 || Lod >= m_HeightmapLods)
        return 0.0f;

    int sampleX = X >> Lod;
    int sampleY = Z >> Lod;

    int lodResoultion = (1 << (m_HeightmapLods - Lod - 1)) + 1;

    sampleX = Math::Clamp(sampleX + (lodResoultion >> 1), 0, (lodResoultion - 1));
    sampleY = Math::Clamp(sampleY + (lodResoultion >> 1), 0, (lodResoultion - 1));
    return m_Heightmap[Lod][sampleY * lodResoultion + sampleX];
}

bool Terrain::Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const
{
    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        bool bCullBackFace;
        int IntersectionCount = 0;

        TVector<TriangleHitResult>* Result;

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) override
        {
            Float3 v0 = btVectorToFloat3(triangle[0]);
            Float3 v1 = btVectorToFloat3(triangle[1]);
            Float3 v2 = btVectorToFloat3(triangle[2]);
            float d, u, v;
            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                TriangleHitResult& hit = Result->Add();
                hit.Location = RayStart + RayDir * d;
                hit.Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                hit.UV.X = u;
                hit.UV.Y = v;
                hit.Distance = d;
                hit.Indices[0] = 0;
                hit.Indices[1] = 0;
                hit.Indices[2] = 0;
                //hit.Material = nullptr;
                IntersectionCount++;
            }
        }
    };

    if (!m_HeightfieldShape)
        return false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, m_BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    Float3 ShapeOffset = Float3(0.0f, (m_MinHeight + m_MaxHeight) * 0.5f, 0.0f);
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart = RayStartLocal;
    triangleRaycastCallback.RayDir = RayDir;
    triangleRaycastCallback.Result = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    m_HeightfieldShape->performRaycast(&triangleRaycastCallback, btVectorToFloat3(RayStartLocal), btVectorToFloat3(RayStartLocal + RayDir * Distance));

    for (int i = HitResult.Size() - triangleRaycastCallback.IntersectionCount; i < HitResult.Size(); i++)
    {
        HitResult[i].Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

bool Terrain::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TriangleHitResult& HitResult) const
{
#define FIRST_INTERSECTION_IS_CLOSEST

    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        float Distance;
        bool bCullBackFace;
        int IntersectionCount = 0;

        TriangleHitResult* Result;

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) override
        {
#ifdef FIRST_INTERSECTION_IS_CLOSEST
            if (IntersectionCount == 0)
            {
                Float3 v0 = btVectorToFloat3(triangle[0]);
                Float3 v1 = btVectorToFloat3(triangle[1]);
                Float3 v2 = btVectorToFloat3(triangle[2]);
                float d, u, v;
                if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
                {
                    Result->Location = RayStart + RayDir * d;
                    Result->Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    Result->UV.X = u;
                    Result->UV.Y = v;
                    Result->Distance = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    //Result->Material = nullptr;
                    IntersectionCount++;
                }
            }
#else
            Float3 v0 = btVectorToFloat3(triangle[0]);
            Float3 v1 = btVectorToFloat3(triangle[1]);
            Float3 v2 = btVectorToFloat3(triangle[2]);
            float d, u, v;
            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (d < Distance)
                {
                    Result->Location = RayStart + RayDir * d;
                    Result->Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    Result->UV.X = u;
                    Result->UV.Y = v;
                    Result->Distance = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    Result->Material = nullptr;
                    Distance = d;
                    IntersectionCount++;
                }
            }
#endif
        }
    };

    if (!m_HeightfieldShape)
        return false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, m_BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    Float3 ShapeOffset = Float3(0.0f, (m_MinHeight + m_MaxHeight) * 0.5f, 0.0f);
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart = RayStartLocal;
    triangleRaycastCallback.RayDir = RayDir;
    triangleRaycastCallback.Distance = Distance;
    triangleRaycastCallback.Result = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    m_HeightfieldShape->performRaycast(&triangleRaycastCallback, btVectorToFloat3(RayStartLocal), btVectorToFloat3(RayStartLocal + RayDir * Distance));

    //LOG( "triangleRaycastCallback.IntersectionCount {}\n", triangleRaycastCallback.IntersectionCount );

    if (triangleRaycastCallback.IntersectionCount > 0)
    {
        HitResult.Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

float Terrain::SampleHeight(float X, float Z) const
{
    float minX = Math::Floor(X);
    float minZ = Math::Floor(Z);

    int quadX = minX + (m_HeightmapResolution >> 1);
    int quadZ = minZ + (m_HeightmapResolution >> 1);

    if (quadX < 0 || quadX >= m_HeightmapResolution - 1)
    {
        return 0.0f;
    }
    if (quadZ < 0 || quadZ >= m_HeightmapResolution - 1)
    {
        return 0.0f;
    }

    /*

    h0       h1
    +--------+
    |        |
    |        |
    |        |
    +--------+
    h3       h2

    */

    float h1 = m_Heightmap[0][quadZ * m_HeightmapResolution + quadX + 1];
    float h3 = m_Heightmap[0][(quadZ + 1) * m_HeightmapResolution + quadX];

    float fx = X - minX;
    float fz = Z - minZ;

    fz = 1.0f - fz;
    if (fx >= fz)
    {
        float h2 = m_Heightmap[0][(quadZ + 1) * m_HeightmapResolution + quadX + 1];
        float u = fz;
        float v = fx - fz;
        float w = 1.0f - fx;
        return h1 * u + h2 * v + h3 * w;
    }
    else
    {
        float h0 = m_Heightmap[0][quadZ * m_HeightmapResolution + quadX];
        float u = fz - fx;
        float v = fx;
        float w = 1.0f - fz;
        return h0 * u + h1 * v + h3 * w;
    }
}

bool Terrain::GetTriangleVertices(float X, float Z, Float3& V0, Float3& V1, Float3& V2) const
{
    float minX = Math::Floor(X);
    float minZ = Math::Floor(Z);

    int quadX = minX + (m_HeightmapResolution >> 1);
    int quadZ = minZ + (m_HeightmapResolution >> 1);

    if (quadX < 0 || quadX >= m_HeightmapResolution - 1)
    {
        return false;
    }
    if (quadZ < 0 || quadZ >= m_HeightmapResolution - 1)
    {
        return false;
    }

    /*

    h0       h1
    +--------+
    |        |
    |        |
    |        |
    +--------+
    h3       h2

    */
    float h0 = m_Heightmap[0][quadZ * m_HeightmapResolution + quadX];
    float h1 = m_Heightmap[0][quadZ * m_HeightmapResolution + quadX + 1];
    float h2 = m_Heightmap[0][(quadZ + 1) * m_HeightmapResolution + quadX + 1];
    float h3 = m_Heightmap[0][(quadZ + 1) * m_HeightmapResolution + quadX];

    float maxX = minX + 1.0f;
    float maxZ = minZ + 1.0f;

    float fractX = X - minX;
    float fractZ = Z - minZ;

    if (fractZ < 1.0f - fractX)
    {
        V0.X = minX;
        V0.Y = h0;
        V0.Z = minZ;

        V1.X = minX;
        V1.Y = h3;
        V1.Z = maxZ;

        V2.X = maxX;
        V2.Y = h1;
        V2.Z = minZ;
    }
    else
    {
        V0.X = minX;
        V0.Y = h3;
        V0.Z = maxZ;

        V1.X = maxX;
        V1.Y = h2;
        V1.Z = maxZ;

        V2.X = maxX;
        V2.Y = h1;
        V2.Z = minZ;
    }

    return true;
}

bool Terrain::GetNormal(float X, float Z, Float3& Normal) const
{
    Float3 v0, v1, v2;
    if (!GetTriangleVertices(X, Z, v0, v1, v2))
    {
        return false;
    }

    Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
    return true;
}

bool Terrain::GetTexcoord(float X, float Z, Float2& Texcoord) const
{
    const float invResolution = 1.0f / (m_HeightmapResolution - 1);

    Texcoord.X = Math::Clamp(X * invResolution + 0.5f, 0.0f, 1.0f);
    Texcoord.Y = Math::Clamp(Z * invResolution + 0.5f, 0.0f, 1.0f);

    return true;
}

bool Terrain::GetTriangle(float X, float Z, TerrainTriangle& Triangle) const
{
    if (!GetTriangleVertices(X, Z, Triangle.Vertices[0], Triangle.Vertices[1], Triangle.Vertices[2]))
    {
        return false;
    }

    Triangle.Normal = Math::Cross(Triangle.Vertices[1] - Triangle.Vertices[0],
                                  Triangle.Vertices[2] - Triangle.Vertices[0]);
    Triangle.Normal.NormalizeSelf();

    GetTexcoord(X, Z, Triangle.Texcoord);

    return true;
}

void Terrain::NotifyTerrainResourceUpdate(TERRAIN_UPDATE_FLAG UpdateFlag)
{
    for (TListIterator<TerrainResourceListener> it(Listeners); it; it++)
    {
        it->OnTerrainResourceUpdate(UpdateFlag);
    }

    // TODO: Update terrain views
}

void Terrain::GatherGeometry(BvAxisAlignedBox const& LocalBounds, TVector<Float3>& Vertices, TVector<unsigned int>& Indices) const
{
    if (!BvBoxOverlapBox(m_BoundingBox, LocalBounds))
    {
        return;
    }

    float minX = Math::Floor(LocalBounds.Mins.X);
    float minZ = Math::Floor(LocalBounds.Mins.Z);
    float maxX = Math::Ceil(LocalBounds.Maxs.X);
    float maxZ = Math::Ceil(LocalBounds.Maxs.Z);
    float minY = LocalBounds.Mins.Y;
    float maxY = LocalBounds.Maxs.Y;

    int halfResolution = m_HeightmapResolution >> 1;

    int minQuadX = minX + halfResolution;
    int minQuadZ = minZ + halfResolution;
    int maxQuadX = maxX + halfResolution;
    int maxQuadZ = maxZ + halfResolution;

    minQuadX = Math::Max(minQuadX, 0);
    minQuadZ = Math::Max(minQuadZ, 0);
    maxQuadX = Math::Min(maxQuadX, m_HeightmapResolution - 1);
    maxQuadZ = Math::Min(maxQuadZ, m_HeightmapResolution - 1);

    int n = Vertices.Size();

    for (int qz = minQuadZ; qz < maxQuadZ; qz++)
    {
        float z = qz - halfResolution;

        float h0 = m_Heightmap[0][qz * m_HeightmapResolution + minQuadX];
        float h3 = m_Heightmap[0][(qz + 1) * m_HeightmapResolution + minQuadX];

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

            float h1 = m_Heightmap[0][qz * m_HeightmapResolution + qx + 1];
            float h2 = m_Heightmap[0][(qz + 1) * m_HeightmapResolution + qx + 1];

            bool firstTriangleCut = false;

            // Triangle h0 h3 h1
            if ((h0 >= minY && h0 <= maxY) ||
                (h3 >= minY && h3 <= maxY) ||
                (h1 >= minY && h1 <= maxY))
            {
                // emit triangle

                Vertices.Add({x, h0, z});
                Vertices.Add({x, h3, z + 1});
                Vertices.Add({x + 1, h1, z});

                Indices.Add(n);
                Indices.Add(n + 1);
                Indices.Add(n + 2);
                n += 3;
            }
            else
                firstTriangleCut = true;

            // Triangle h1 h3 h2
            if ((h1 >= minY && h1 <= maxY) ||
                (h3 >= minY && h3 <= maxY) ||
                (h2 >= minY && h2 <= maxY))
            {
                // emit triangle
                if (firstTriangleCut)
                {
                    Vertices.Add({x + 1, h1, z});
                    Vertices.Add({x, h3, z + 1});
                    Vertices.Add({x + 1, h2, z + 1});

                    Indices.Add(n);
                    Indices.Add(n + 1);
                    Indices.Add(n + 2);
                    n += 3;
                }
                else
                {
                    Vertices.Add({x + 1, h2, z + 1});

                    Indices.Add(n - 1);
                    Indices.Add(n - 2);
                    Indices.Add(n);
                    n++;
                }
            }

            h0 = h1;
            h3 = h2;
        }
    }
}

HK_NAMESPACE_END

#endif
