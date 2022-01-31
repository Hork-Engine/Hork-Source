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

Modify NavMesh

*/

#include "Terrain.h"
#include "TerrainComponent.h"

#include <Core/IntrusiveLinkedListMacro.h>
#include <Geometry/BV/BvIntersect.h>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "BulletCompatibility.h"

AN_CLASS_META(ATerrain)

ATerrain::ATerrain()
{}

ATerrain::~ATerrain()
{
    Purge();
}

bool ATerrain::LoadResource(IBinaryStream& Stream)
{
    Purge();

    // TODO: Heightmap resolution should be specified in asset file
    HeightmapResolution = 4097;

    // Allocate memory for terrain lods
    HeightmapLods = Math::Log2((uint32_t)(HeightmapResolution - 1)) + 1;
    Heightmap.Resize(HeightmapLods);
    for (int i = 0; i < HeightmapLods; i++)
    {
        int sz       = 1 << (HeightmapLods - i - 1);
        Heightmap[i] = (float*)GHeapMemory.Alloc((sz + 1) * (sz + 1) * sizeof(float));
    }

    // Read most detailed lod
    Stream.ReadBuffer(Heightmap[0], HeightmapResolution * HeightmapResolution * sizeof(float));

    // Generate lods
    float h1, h2, h3, h4;
    int   x, y;
    for (int i = 1; i < HeightmapLods; i++)
    {
        float* lod    = Heightmap[i];
        float* srcLod = Heightmap[i - 1];

        int sz  = 1 << (HeightmapLods - i - 1);
        int sz2 = 1 << (HeightmapLods - i);

        sz++;
        sz2++;

        for (y = 0; y < sz - 1; y++)
        {
            int src_y = y << 1;
            for (x = 0; x < sz - 1; x++)
            {
                int src_x = x << 1;
                h1        = srcLod[src_y * sz2 + src_x];
                h2        = srcLod[src_y * sz2 + src_x + 1];
                h3        = srcLod[(src_y + 1) * sz2 + src_x];
                h4        = srcLod[(src_y + 1) * sz2 + src_x + 1];

                lod[y * sz + x] = (h1 + h2 + h3 + h4) * 0.25f;
            }

            int src_x       = x << 1;
            h1              = srcLod[src_y * sz2 + src_x];
            h2              = srcLod[(src_y + 1) * sz2 + src_x];
            lod[y * sz + x] = (h1 + h2) * 0.5f;
        }

        int src_y = y << 1;
        for (x = 0; x < sz - 1; x++)
        {
            int src_x = x << 1;
            h1        = srcLod[src_y * sz2 + src_x];
            h2        = srcLod[src_y * sz2 + src_x + 1];

            lod[y * sz + x] = (h1 + h2) * 0.5f;
        }

        int src_x       = x << 1;
        lod[y * sz + x] = srcLod[src_y * sz2 + src_x];
    }

    // Calc Min/Max height. TODO: should be specified in asset file
    MinHeight = 99999;
    MaxHeight = -99999;
    for (y = 0; y < HeightmapResolution; y++)
    {
        for (x = 0; x < HeightmapResolution; x++)
        {
            float h   = Heightmap[0][y * HeightmapResolution + x];
            MinHeight = Math::Min(MinHeight, h);
            MaxHeight = Math::Max(MaxHeight, h);
        }
    }

    // Calc clipping region
    int halfResolution = HeightmapResolution >> 1;
    ClipMin.X          = halfResolution;
    ClipMin.Y          = halfResolution;
    ClipMax.X          = halfResolution;
    ClipMax.Y          = halfResolution;

    // Calc bounding box
    BoundingBox.Mins.X = -ClipMin.X;
    BoundingBox.Mins.Y = MinHeight;
    BoundingBox.Mins.Z = -ClipMin.Y;
    BoundingBox.Maxs.X = ClipMax.X;
    BoundingBox.Maxs.Y = MaxHeight;
    BoundingBox.Maxs.Z = ClipMax.Y;

    // Generate accelerated terrain collision shape
    HeightfieldShape = MakeUnique<btHeightfieldTerrainShape>(HeightmapResolution, HeightmapResolution, Heightmap[0], 1, MinHeight, MaxHeight, 1, PHY_FLOAT, false /* bFlipQuadEdges */);
    HeightfieldShape->buildAccelerator();

    /*
    NOTE about btHeightfieldTerrainShape:
      The caller is responsible for maintaining the heightfield array; this
      class does not make a copy.

      The heightfield can be dynamic so long as the min/max height values
      capture the extremes (heights must always be in that range).
    */

    NotifyTerrainModified();

    return true;
}

void ATerrain::LoadInternalResource(const char* Path)
{
    Purge();

    // Create some dummy terrain

    HeightmapResolution = 33;

    // Allocate memory for terrain lods
    HeightmapLods = Math::Log2((uint32_t)(HeightmapResolution - 1)) + 1;
    Heightmap.Resize(HeightmapLods);
    for (int i = 0; i < HeightmapLods; i++)
    {
        int sz       = 1 << (HeightmapLods - i - 1);
        Heightmap[i] = (float*)GHeapMemory.ClearedAlloc((sz + 1) * (sz + 1) * sizeof(float));
    }

    MinHeight = 0.1f;
    MaxHeight = -0.1f;

    // Calc clipping region
    int halfResolution = HeightmapResolution >> 1;
    ClipMin.X          = halfResolution;
    ClipMin.Y          = halfResolution;
    ClipMax.X          = halfResolution;
    ClipMax.Y          = halfResolution;

    // Calc bounding box
    BoundingBox.Mins.X = -ClipMin.X;
    BoundingBox.Mins.Y = MinHeight;
    BoundingBox.Mins.Z = -ClipMin.Y;
    BoundingBox.Maxs.X = ClipMax.X;
    BoundingBox.Maxs.Y = MaxHeight;
    BoundingBox.Maxs.Z = ClipMax.Y;

    // Generate accelerated terrain collision shape
    HeightfieldShape = MakeUnique<btHeightfieldTerrainShape>(HeightmapResolution, HeightmapResolution, Heightmap[0], 1, MinHeight, MaxHeight, 1, PHY_FLOAT, false /* bFlipQuadEdges */);
    HeightfieldShape->buildAccelerator();

    NotifyTerrainModified();
}

void ATerrain::Purge()
{
    for (int i = 0; i < HeightmapLods; i++)
    {
        GHeapMemory.Free(Heightmap[i]);
    }
}

float ATerrain::ReadHeight(int X, int Z, int Lod) const
{
    AN_ASSERT(Lod >= 0 && Lod < HeightmapLods);

    int sampleX = X >> Lod;
    int sampleY = Z >> Lod;

    int lodResoultion = (1 << (HeightmapLods - Lod - 1)) + 1;

    sampleX = Math::Clamp(sampleX + (lodResoultion >> 1), 0, (lodResoultion - 1));
    sampleY = Math::Clamp(sampleY + (lodResoultion >> 1), 0, (lodResoultion - 1));
    return Heightmap[Lod][sampleY * lodResoultion + sampleX];
}

bool ATerrain::Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TPodVector<STriangleHitResult>& HitResult) const
{
    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        bool   bCullBackFace;
        int    IntersectionCount = 0;

        TPodVector<STriangleHitResult>* Result;

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) override
        {
            Float3 v0 = btVectorToFloat3(triangle[0]);
            Float3 v1 = btVectorToFloat3(triangle[1]);
            Float3 v2 = btVectorToFloat3(triangle[2]);
            float  d, u, v;
            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                STriangleHitResult& hit = Result->Append();
                hit.Location            = RayStart + RayDir * d;
                hit.Normal              = Math::Cross(v1 - v0, v2 - v0).Normalized();
                hit.UV.X                = u;
                hit.UV.Y                = v;
                hit.Distance            = d;
                hit.Indices[0]          = 0;
                hit.Indices[1]          = 0;
                hit.Indices[2]          = 0;
                hit.Material            = nullptr;
                IntersectionCount++;
            }
        }
    };

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    Float3 ShapeOffset   = Float3(0.0f, (MinHeight + MaxHeight) * 0.5f, 0.0f);
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart      = RayStartLocal;
    triangleRaycastCallback.RayDir        = RayDir;
    triangleRaycastCallback.Result        = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    HeightfieldShape->performRaycast(&triangleRaycastCallback, btVectorToFloat3(RayStartLocal), btVectorToFloat3(RayStartLocal + RayDir * Distance));

    for (int i = HitResult.Size() - triangleRaycastCallback.IntersectionCount; i < HitResult.Size(); i++)
    {
        HitResult[i].Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

bool ATerrain::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, STriangleHitResult& HitResult) const
{
#define FIRST_INTERSECTION_IS_CLOSEST

    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        float  Distance;
        bool   bCullBackFace;
        int    IntersectionCount = 0;

        STriangleHitResult* Result;

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) override
        {
#ifdef FIRST_INTERSECTION_IS_CLOSEST
            if (IntersectionCount == 0)
            {
                Float3 v0 = btVectorToFloat3(triangle[0]);
                Float3 v1 = btVectorToFloat3(triangle[1]);
                Float3 v2 = btVectorToFloat3(triangle[2]);
                float  d, u, v;
                if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
                {
                    Result->Location   = RayStart + RayDir * d;
                    Result->Normal     = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    Result->UV.X       = u;
                    Result->UV.Y       = v;
                    Result->Distance   = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    Result->Material   = nullptr;
                    IntersectionCount++;
                }
            }
#else
            Float3 v0 = btVectorToFloat3(triangle[0]);
            Float3 v1 = btVectorToFloat3(triangle[1]);
            Float3 v2 = btVectorToFloat3(triangle[2]);
            float  d, u, v;
            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (d < Distance)
                {
                    Result->Location   = RayStart + RayDir * d;
                    Result->Normal     = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    Result->UV.X       = u;
                    Result->UV.Y       = v;
                    Result->Distance   = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    Result->Material   = nullptr;
                    Distance           = d;
                    IntersectionCount++;
                }
            }
#endif
        }
    };

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    Float3 ShapeOffset   = Float3(0.0f, (MinHeight + MaxHeight) * 0.5f, 0.0f);
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart      = RayStartLocal;
    triangleRaycastCallback.RayDir        = RayDir;
    triangleRaycastCallback.Distance      = Distance;
    triangleRaycastCallback.Result        = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    HeightfieldShape->performRaycast(&triangleRaycastCallback, btVectorToFloat3(RayStartLocal), btVectorToFloat3(RayStartLocal + RayDir * Distance));

    //GLogger.Printf( "triangleRaycastCallback.IntersectionCount %d\n", triangleRaycastCallback.IntersectionCount );

    if (triangleRaycastCallback.IntersectionCount > 0)
    {
        HitResult.Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

float ATerrain::SampleHeight(float X, float Z) const
{
    float minX = Math::Floor(X);
    float minZ = Math::Floor(Z);

    int quadX = minX + (HeightmapResolution >> 1);
    int quadZ = minZ + (HeightmapResolution >> 1);

    if (quadX < 0 || quadX >= HeightmapResolution - 1)
    {
        return 0.0f;
    }
    if (quadZ < 0 || quadZ >= HeightmapResolution - 1)
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

    float h1 = Heightmap[0][quadZ * HeightmapResolution + quadX + 1];
    float h3 = Heightmap[0][(quadZ + 1) * HeightmapResolution + quadX];

    float fx = X - minX;
    float fz = Z - minZ;

    fz = 1.0f - fz;
    if (fx >= fz)
    {
        float h2 = Heightmap[0][(quadZ + 1) * HeightmapResolution + quadX + 1];
        float u  = fz;
        float v  = fx - fz;
        float w  = 1.0f - fx;
        return h1 * u + h2 * v + h3 * w;
    }
    else
    {
        float h0 = Heightmap[0][quadZ * HeightmapResolution + quadX];
        float u  = fz - fx;
        float v  = fx;
        float w  = 1.0f - fz;
        return h0 * u + h1 * v + h3 * w;
    }
}

bool ATerrain::GetTriangleVertices(float X, float Z, Float3& V0, Float3& V1, Float3& V2) const
{
    float minX = Math::Floor(X);
    float minZ = Math::Floor(Z);

    int quadX = minX + (HeightmapResolution >> 1);
    int quadZ = minZ + (HeightmapResolution >> 1);

    if (quadX < 0 || quadX >= HeightmapResolution - 1)
    {
        return false;
    }
    if (quadZ < 0 || quadZ >= HeightmapResolution - 1)
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
    float h0 = Heightmap[0][quadZ * HeightmapResolution + quadX];
    float h1 = Heightmap[0][quadZ * HeightmapResolution + quadX + 1];
    float h2 = Heightmap[0][(quadZ + 1) * HeightmapResolution + quadX + 1];
    float h3 = Heightmap[0][(quadZ + 1) * HeightmapResolution + quadX];

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

bool ATerrain::GetNormal(float X, float Z, Float3& Normal) const
{
    Float3 v0, v1, v2;
    if (!GetTriangleVertices(X, Z, v0, v1, v2))
    {
        return false;
    }

    Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
    return true;
}

bool ATerrain::GetTexcoord(float X, float Z, Float2& Texcoord) const
{
    const float invResolution = 1.0f / (HeightmapResolution - 1);

    Texcoord.X = Math::Clamp(X * invResolution + 0.5f, 0.0f, 1.0f);
    Texcoord.Y = Math::Clamp(Z * invResolution + 0.5f, 0.0f, 1.0f);

    return true;
}

bool ATerrain::GetTriangle(float X, float Z, STerrainTriangle& Triangle) const
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

void ATerrain::AddListener(ATerrainComponent* Listener)
{
    INTRUSIVE_ADD_UNIQUE(Listener, pNext, pPrev, Listeners, ListenersTail);
}

void ATerrain::RemoveListener(ATerrainComponent* Listener)
{
    INTRUSIVE_REMOVE(Listener, pNext, pPrev, Listeners, ListenersTail);
}

void ATerrain::NotifyTerrainModified()
{
    for (ATerrainComponent* component = Listeners; component; component = component->pNext)
    {
        component->OnTerrainModified();
    }

    // TODO: Update terrain views
}
