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

struct TerrainTriangle
{
    Float3 Vertices[3];
    Float3 Normal;
    Float2 Texcoord;
};

class Terrain : public Resource
{
public:
    /** Navigation areas are used to gather navigation geometry.
    
    NOTE: In the future, we can create a bit mask for each terrain quad to decide which triangles should be used for navigation.
    e.g. TBitMask<> WalkableMask
    */
    TVector<BvAxisAlignedBox> NavigationAreas;

    /** Find ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const;
    /** Find ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TriangleHitResult& HitResult) const;

    bool GetTriangle(float X, float Z, TerrainTriangle& Triangle) const;
};

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

HK_NAMESPACE_END

#endif
