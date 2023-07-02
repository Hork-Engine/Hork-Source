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

#include "ConvexDecomposition.h"

#include <Engine/Core/Logger.h>

#include <HACD/hacdHACD.h>

#include <VHACD/VHACD.h>

HK_NAMESPACE_BEGIN

namespace
{

HK_FORCEINLINE bool IsPointInsideConvexHull(Float3 const& p, PlaneF const* planes, int planeCount, float margin)
{
    for (int i = 0; i < planeCount; i++)
        if (Math::Dot(planes[i].Normal, p) + planes[i].D - margin > 0)
            return false;
    return true;
}

static int FindPlane(PlaneF const& plane, PlaneF const* planes, int planeCount)
{
    for (int i = 0; i < planeCount; i++)
        if (Math::Dot(plane.Normal, planes[i].Normal) > 0.999f)
            return i;
    return -1;
}

static bool AreVerticesBehindPlane(PlaneF const& plane, Float3 const* vertices, int vertexCount, float margin)
{
    for (int i = 0; i < vertexCount; i++)
    {
        float dist = Math::Dot(plane.Normal, vertices[i]) + plane.D - margin;
        if (dist > 0.0f)
            return false;
    }
    return true;
}

} // namespace

namespace Geometry
{

void ConvexHullPlanesFromVertices(Float3 const* vertices, int vertexCount, TVector<PlaneF>& planes)
{
    PlaneF plane;
    Float3 edge0, edge1;

    const float margin = 0.01f;

    planes.Clear();

    for (int i = 0; i < vertexCount; i++)
    {
        Float3 const& normal1 = vertices[i];

        for (int j = i + 1; j < vertexCount; j++)
        {
            Float3 const& normal2 = vertices[j];

            edge0 = normal2 - normal1;

            for (int k = j + 1; k < vertexCount; k++)
            {
                Float3 const& normal3 = vertices[k];

                edge1 = normal3 - normal1;

                float normalSign = 1;

                for (int ww = 0; ww < 2; ww++)
                {
                    plane.Normal = normalSign * Math::Cross(edge0, edge1);
                    if (plane.Normal.LengthSqr() > 0.0001f)
                    {
                        plane.Normal.NormalizeSelf();

                        if (FindPlane(plane, planes.ToPtr(), planes.Size()) == -1)
                        {
                            plane.D = -Math::Dot(plane.Normal, normal1);

                            if (AreVerticesBehindPlane(plane, vertices, vertexCount, margin))
                            {
                                planes.Add(plane);
                            }
                        }
                    }
                    normalSign = -1;
                }
            }
        }
    }
}

void ConvexHullVerticesFromPlanes(PlaneF const* planes, int planeCount, TVector<Float3>& vertices)
{
    constexpr float tolerance = 0.0001f;
    constexpr float quotientTolerance = 0.000001f;

    vertices.Clear();

    for (int i = 0; i < planeCount; i++)
    {
        Float3 const& normal1 = planes[i].Normal;

        for (int j = i + 1; j < planeCount; j++)
        {
            Float3 const& normal2 = planes[j].Normal;

            Float3 n1n2 = Math::Cross(normal1, normal2);

            if (n1n2.LengthSqr() > tolerance)
            {
                for (int k = j + 1; k < planeCount; k++)
                {
                    Float3 const& normal3 = planes[k].Normal;

                    Float3 n2n3 = Math::Cross(normal2, normal3);
                    Float3 n3n1 = Math::Cross(normal3, normal1);

                    if ((n2n3.LengthSqr() > tolerance) && (n3n1.LengthSqr() > tolerance))
                    {
                        float quotient = Math::Dot(normal1, n2n3);
                        if (fabs(quotient) > quotientTolerance)
                        {
                            quotient = -1 / quotient;

                            Float3 potentialVertex = n2n3 * planes[i].D + n3n1 * planes[j].D + n1n2 * planes[k].D;
                            potentialVertex *= quotient;

                            if (IsPointInsideConvexHull(potentialVertex, planes, planeCount, 0.01f))
                            {
                                vertices.Add(potentialVertex);
                            }
                        }
                    }
                }
            }
        }
    }
}

void BakeCollisionMarginConvexHull(Float3 const* vertices, int vertexCount, TVector<Float3>& outVertices, float margin)
{
    TVector<PlaneF> planes;

    ConvexHullPlanesFromVertices(vertices, vertexCount, planes);

    for (int i = 0; i < planes.Size(); i++)
    {
        PlaneF& plane = planes[i];

        plane.D += margin;
    }

    ConvexHullVerticesFromPlanes(planes.ToPtr(), planes.Size(), outVertices);
}

bool PerformConvexDecomposition(Float3 const* vertices,
                                int vertexCount,
                                int vertexStride,
                                unsigned int const* indices,
                                int indexCount,
                                TVector<Float3>& outVertices,
                                TVector<unsigned int>& outIndices,
                                TVector<ConvexHullDesc>& outHulls)
{
    outVertices.Clear();
    outIndices.Clear();
    outHulls.Clear();

    HK_VERIFY_R(indexCount % 3 == 0, "PerformConvexDecomposition: The number of indices must be a multiple of 3");

    TVector<HACD::Vec3<HACD::Real>> points(vertexCount);
    TVector<HACD::Vec3<long>> triangles(indexCount / 3);

    byte const* srcVertices = (byte const*)vertices;
    for (int i = 0; i < vertexCount; i++)
    {
        Float3 const* vertex = (Float3 const*)srcVertices;

        points[i] = HACD::Vec3<HACD::Real>(vertex->X, vertex->Y, vertex->Z);

        srcVertices += vertexStride;
    }

    int triangleNum = 0;
    for (int i = 0; i < indexCount; i += 3, triangleNum++)
    {
        triangles[triangleNum] = HACD::Vec3<long>(indices[i], indices[i + 1], indices[i + 2]);
    }

    HACD::HACD hacd;
    hacd.SetPoints(points.ToPtr());
    hacd.SetNPoints(vertexCount);
    hacd.SetTriangles(triangles.ToPtr());
    hacd.SetNTriangles(indexCount / 3);
    //    hacd.SetCompacityWeight( 0.1 );
    //    hacd.SetVolumeWeight( 0.0 );
    //    hacd.SetNClusters( 2 );                     // recommended 2
    //    hacd.SetNVerticesPerCH( 100 );
    //    hacd.SetConcavity( 100 );                   // recommended 100
    //    hacd.SetAddExtraDistPoints( false );        // recommended false
    //    hacd.SetAddNeighboursDistPoints( false );   // recommended false
    //    hacd.SetAddFacesPoints( false );            // recommended false

    hacd.SetCompacityWeight(0.1);
    hacd.SetVolumeWeight(0.0);
    hacd.SetNClusters(2); // recommended 2
    hacd.SetNVerticesPerCH(100);
    hacd.SetConcavity(0.01);               // recommended 100
    hacd.SetAddExtraDistPoints(true);      // recommended false
    hacd.SetAddNeighboursDistPoints(true); // recommended false
    hacd.SetAddFacesPoints(true);          // recommended false

    hacd.Compute();

    int maxPointsPerCluster = 0;
    int maxTrianglesPerCluster = 0;
    int totalPoints = 0;
    int totalTriangles = 0;

    int numClusters = hacd.GetNClusters();
    for (int cluster = 0; cluster < numClusters; cluster++)
    {
        int numPoints = hacd.GetNPointsCH(cluster);
        int numTriangles = hacd.GetNTrianglesCH(cluster);

        totalPoints += numPoints;
        totalTriangles += numTriangles;

        maxPointsPerCluster = Math::Max(maxPointsPerCluster, numPoints);
        maxTrianglesPerCluster = Math::Max(maxTrianglesPerCluster, numTriangles);
    }

    TVector<HACD::Vec3<HACD::Real>> hullPoints(maxPointsPerCluster);
    TVector<HACD::Vec3<long>> hullTriangles(maxTrianglesPerCluster);

    outHulls.Resize(numClusters);
    outVertices.Resize(totalPoints);
    outIndices.Resize(totalTriangles * 3);

    totalPoints = 0;
    totalTriangles = 0;

    for (int cluster = 0; cluster < numClusters; cluster++)
    {
        int numPoints = hacd.GetNPointsCH(cluster);
        int numTriangles = hacd.GetNTrianglesCH(cluster);

        hacd.GetCH(cluster, hullPoints.ToPtr(), hullTriangles.ToPtr());

        ConvexHullDesc& hull = outHulls[cluster];
        hull.FirstVertex = totalPoints;
        hull.VertexCount = numPoints;
        hull.FirstIndex = totalTriangles * 3;
        hull.IndexCount = numTriangles * 3;
        hull.Centroid.Clear();

        totalPoints += numPoints;
        totalTriangles += numTriangles;

        Float3* pVertices = outVertices.ToPtr() + hull.FirstVertex;
        for (int i = 0; i < numPoints; i++, pVertices++)
        {
            pVertices->X = hullPoints[i].X();
            pVertices->Y = hullPoints[i].Y();
            pVertices->Z = hullPoints[i].Z();

            hull.Centroid += *pVertices;
        }

        hull.Centroid /= (float)numPoints;

        // Adjust vertices
        pVertices = outVertices.ToPtr() + hull.FirstVertex;
        for (int i = 0; i < numPoints; i++, pVertices++)
        {
            *pVertices -= hull.Centroid;
        }

        unsigned int* pIndices = outIndices.ToPtr() + hull.FirstIndex;
        int n = 0;
        for (int i = 0; i < hull.IndexCount; i += 3, n++)
        {
            *pIndices++ = hullTriangles[n].X();
            *pIndices++ = hullTriangles[n].Y();
            *pIndices++ = hullTriangles[n].Z();
        }
    }

    return !outHulls.IsEmpty();
}

bool PerformConvexDecompositionVHACD(Float3 const* vertices,
                                     int vertexCount,
                                     int vertexStride,
                                     unsigned int const* indices,
                                     int indexCount,
                                     TVector<Float3>& outVertices,
                                     TVector<unsigned int>& outIndices,
                                     TVector<ConvexHullDesc>& outHulls,
                                     Float3& centerOfMass)
{
    class Callback : public VHACD::IVHACD::IUserCallback
    {
    public:
        void Update(const double overallProgress,
                    const double stageProgress,
                    const char* const stage,
                    const char* operation) override
        {
            LOG("Overall progress {}, {} progress {}, operation: {}\n", overallProgress, stage, stageProgress, operation);
        }
    };
    class Logger : public VHACD::IVHACD::IUserLogger
    {
    public:
        void Log(const char* const msg) override
        {
            LOG(msg);
        }
    };

    Callback callback;
    Logger logger;

    VHACD::IVHACD* vhacd = VHACD::CreateVHACD();

    VHACD::IVHACD::Parameters params;
    params.m_callback = &callback;                   // Optional user provided callback interface for progress
    params.m_logger = &logger;                       // Optional user provided callback interface for log messages
    params.m_taskRunner = nullptr;                   // Optional user provided interface for creating tasks
    params.m_maxConvexHulls = 64;                    // The maximum number of convex hulls to produce
    params.m_resolution = 400000;                    // The voxel resolution to use
    params.m_minimumVolumePercentErrorAllowed = 1;   // if the voxels are within 1% of the volume of the hull, we consider this a close enough approximation
    params.m_maxRecursionDepth = 14;                 // The maximum recursion depth
    params.m_shrinkWrap = true;                      // Whether or not to shrinkwrap the voxel positions to the source mesh on output
    params.m_fillMode = VHACD::FillMode::FLOOD_FILL; // How to fill the interior of the voxelized mesh //FLOOD_FILL SURFACE_ONLY RAYCAST_FILL
    params.m_maxNumVerticesPerCH = 64;               // The maximum number of vertices allowed in any output convex hull
    params.m_asyncACD = true;                        // Whether or not to run asynchronously, taking advantage of additonal cores
    params.m_minEdgeLength = 2;                      // Once a voxel patch has an edge length of less than 4 on all 3 sides, we don't keep recursing
    params.m_findBestPlane = false;                  // Whether or not to attempt to split planes along the best location. Experimental feature. False by default.

    outVertices.Clear();
    outIndices.Clear();
    outHulls.Clear();

    HK_VERIFY_R(indexCount % 3 == 0, "PerformConvexDecompositionVHACD: The number of indices must be a multiple of 3");

    TVector<Double3> tempVertices(vertexCount);
    byte const* srcVertices = (byte const*)vertices;
    for (int i = 0; i < vertexCount; i++)
    {
        tempVertices[i] = Double3(*(Float3 const*)srcVertices);
        srcVertices += vertexStride;
    }
    bool bResult = vhacd->Compute(&tempVertices[0][0], vertexCount, indices, indexCount / 3, params);

    if (bResult)
    {
        double dcenterOfMass[3];
        if (!vhacd->ComputeCenterOfMass(dcenterOfMass))
        {
            dcenterOfMass[0] = dcenterOfMass[1] = dcenterOfMass[2] = 0;
        }

        centerOfMass[0] = dcenterOfMass[0];
        centerOfMass[1] = dcenterOfMass[1];
        centerOfMass[2] = dcenterOfMass[2];

        VHACD::IVHACD::ConvexHull ch;
        outHulls.Resize(vhacd->GetNConvexHulls());
        int totalVertices = 0;
        int totalIndices = 0;
        for (int i = 0; i < outHulls.Size(); i++)
        {
            ConvexHullDesc& hull = outHulls[i];

            vhacd->GetConvexHull(i, ch);

            hull.FirstVertex = totalVertices;
            hull.VertexCount = ch.m_nPoints;
            hull.FirstIndex = totalIndices;
            hull.IndexCount = ch.m_nTriangles * 3;
            hull.Centroid[0] = ch.m_center[0];
            hull.Centroid[1] = ch.m_center[1];
            hull.Centroid[2] = ch.m_center[2];

            totalVertices += hull.VertexCount;
            totalIndices += hull.IndexCount;
        }

        outVertices.Resize(totalVertices);
        outIndices.Resize(totalIndices);

        for (int i = 0; i < outHulls.Size(); i++)
        {
            ConvexHullDesc& hull = outHulls[i];

            vhacd->GetConvexHull(i, ch);

            Float3* pVertices = outVertices.ToPtr() + hull.FirstVertex;
            for (int v = 0; v < hull.VertexCount; v++, pVertices++)
            {
                pVertices->X = ch.m_points[v * 3 + 0] - ch.m_center[0];
                pVertices->Y = ch.m_points[v * 3 + 1] - ch.m_center[1];
                pVertices->Z = ch.m_points[v * 3 + 2] - ch.m_center[2];
            }

            unsigned int* pIndices = outIndices.ToPtr() + hull.FirstIndex;
            for (int v = 0; v < hull.IndexCount; v += 3, pIndices += 3)
            {
                pIndices[0] = ch.m_triangles[v + 0];
                pIndices[1] = ch.m_triangles[v + 1];
                pIndices[2] = ch.m_triangles[v + 2];
            }
        }
    }
    else
    {
        LOG("PerformConvexDecompositionVHACD: convex decomposition error\n");
    }

    vhacd->Clean();
    vhacd->Release();

    return !outHulls.IsEmpty();
}

} // namespace Geometry

HK_NAMESPACE_END
