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

#include "ConvexDecomposition.h"

#include <Hork/Core/Logger.h>

#include <VHACD/VHACD.h>

HK_NAMESPACE_BEGIN

namespace
{

bool IsPointInsideConvexHull(Float3 const& p, PlaneF const* planes, int planeCount, float margin)
{
    for (int i = 0; i < planeCount; i++)
        if (Math::Dot(planes[i].Normal, p) + planes[i].D - margin > 0)
            return false;
    return true;
}

int FindPlane(PlaneF const& plane, PlaneF const* planes, int planeCount)
{
    for (int i = 0; i < planeCount; i++)
        if (Math::Dot(plane.Normal, planes[i].Normal) > 0.999f)
            return i;
    return -1;
}

bool AreVerticesBehindPlane(PlaneF const& plane, Float3 const* vertices, int vertexCount, float margin)
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

void ConvexHullPlanesFromVertices(Float3 const* vertices, int vertexCount, Vector<PlaneF>& planes)
{
    PlaneF plane;
    Float3 edge0, edge1;

    const float margin = 0.01f;

    auto firstPlane = planes.Size();

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

                        if (FindPlane(plane, planes.ToPtr() + firstPlane, planes.Size() - firstPlane) == -1)
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

void ConvexHullVerticesFromPlanes(PlaneF const* planes, int planeCount, Vector<Float3>& vertices)
{
    constexpr float tolerance = 0.0001f;
    constexpr float quotientTolerance = 0.000001f;

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

void BakeCollisionMarginConvexHull(Float3 const* vertices, int vertexCount, Vector<Float3>& outVertices, float margin)
{
    Vector<PlaneF> planes;

    ConvexHullPlanesFromVertices(vertices, vertexCount, planes);

    for (int i = 0; i < planes.Size(); i++)
    {
        PlaneF& plane = planes[i];

        plane.D += margin;
    }

    ConvexHullVerticesFromPlanes(planes.ToPtr(), planes.Size(), outVertices);
}

bool PerformConvexDecompositionVHACD(VHACDParameters const& inParams,
                                     Float3 const* vertices,
                                     int vertexCount,
                                     int vertexStride,
                                     unsigned int const* indices,
                                     int indexCount,
                                     Vector<Float3>& outVertices,
                                     Vector<unsigned int>& outIndices,
                                     Vector<ConvexHullDesc>& outHulls,
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
    params.m_callback = &callback;
    params.m_logger = &logger;
    params.m_taskRunner = nullptr; // TODO
    params.m_maxConvexHulls = inParams.MaxConvexHulls;
    params.m_resolution = inParams.VoxelResolution;
    params.m_minimumVolumePercentErrorAllowed = inParams.MinimumVolumePercentErrorAllowed;
    params.m_maxRecursionDepth = inParams.MaxRecursionDepth;
    params.m_shrinkWrap = inParams.ShrinkWrap;
    switch (inParams.FillMode)
    {
    case VHACDParameters::FillMode::FLOOD_FILL: params.m_fillMode = VHACD::FillMode::FLOOD_FILL; break;
    case VHACDParameters::FillMode::SURFACE_ONLY: params.m_fillMode = VHACD::FillMode::SURFACE_ONLY; break;
    case VHACDParameters::FillMode::RAYCAST_FILL: params.m_fillMode = VHACD::FillMode::RAYCAST_FILL; break;
    default: params.m_fillMode = VHACD::FillMode::FLOOD_FILL; break;
    }
    params.m_maxNumVerticesPerCH = inParams.MaxNumVerticesPerCH;
    params.m_asyncACD = true;
    params.m_minEdgeLength = inParams.MinEdgeLength;
    params.m_findBestPlane = inParams.FindBestPlane;

    outVertices.Clear();
    outIndices.Clear();
    outHulls.Clear();

    HK_VERIFY_R(indexCount % 3 == 0, "PerformConvexDecompositionVHACD: The number of indices must be a multiple of 3");

    Vector<Double3> tempVertices(vertexCount);
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
            hull.VertexCount = ch.m_points.size();
            hull.FirstIndex = totalIndices;
            hull.IndexCount = ch.m_triangles.size() * 3;
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
                pVertices->X = ch.m_points[v * 3 + 0].mX - ch.m_center[0];
                pVertices->Y = ch.m_points[v * 3 + 1].mY - ch.m_center[1];
                pVertices->Z = ch.m_points[v * 3 + 2].mZ - ch.m_center[2];
            }

            unsigned int* pIndices = outIndices.ToPtr() + hull.FirstIndex;
            for (int v = 0; v < ch.m_triangles.size(); ++v, pIndices += 3)
            {
                pIndices[0] = ch.m_triangles[v].mI0;
                pIndices[1] = ch.m_triangles[v].mI1;
                pIndices[2] = ch.m_triangles[v].mI2;
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
