﻿/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Platform/Logger.h>

//#include <bullet3/Extras/ConvexDecomposition/ConvexDecomposition.h>
#include <ThirdParty/bullet3/Extras/HACD/hacdHACD.h>
#include <VHACD.h>

AN_FORCEINLINE bool IsPointInsideConvexHull(Float3 const& _Point, PlaneF const* _Planes, int _NumPlanes, float _Margin)
{
    for (int i = 0; i < _NumPlanes; i++)
    {
        if (Math::Dot(_Planes[i].Normal, _Point) + _Planes[i].D - _Margin > 0)
        {
            return false;
        }
    }
    return true;
}

static int FindPlane(PlaneF const& _Plane, PlaneF const* _Planes, int _NumPlanes)
{
    for (int i = 0; i < _NumPlanes; i++)
    {
        if (Math::Dot(_Plane.Normal, _Planes[i].Normal) > 0.999f)
        {
            return i;
        }
    }
    return -1;
}

static bool AreVerticesBehindPlane(PlaneF const& _Plane, Float3 const* _Vertices, int _NumVertices, float _Margin)
{
    for (int i = 0; i < _NumVertices; i++)
    {
        float dist = Math::Dot(_Plane.Normal, _Vertices[i]) + _Plane.D - _Margin;
        if (dist > 0.0f)
        {
            return false;
        }
    }
    return true;
}

void ConvexHullPlanesFromVertices(Float3 const* _Vertices, int _NumVertices, TPodVector<PlaneF>& _Planes)
{
    PlaneF plane;
    Float3 edge0, edge1;

    const float margin = 0.01f;

    _Planes.Clear();

    for (int i = 0; i < _NumVertices; i++)
    {
        Float3 const& normal1 = _Vertices[i];

        for (int j = i + 1; j < _NumVertices; j++)
        {
            Float3 const& normal2 = _Vertices[j];

            edge0 = normal2 - normal1;

            for (int k = j + 1; k < _NumVertices; k++)
            {
                Float3 const& normal3 = _Vertices[k];

                edge1 = normal3 - normal1;

                float normalSign = 1;

                for (int ww = 0; ww < 2; ww++)
                {
                    plane.Normal = normalSign * Math::Cross(edge0, edge1);
                    if (plane.Normal.LengthSqr() > 0.0001f)
                    {
                        plane.Normal.NormalizeSelf();

                        if (FindPlane(plane, _Planes.ToPtr(), _Planes.Size()) == -1)
                        {
                            plane.D = -Math::Dot(plane.Normal, normal1);

                            if (AreVerticesBehindPlane(plane, _Vertices, _NumVertices, margin))
                            {
                                _Planes.Append(plane);
                            }
                        }
                    }
                    normalSign = -1;
                }
            }
        }
    }
}

void ConvexHullVerticesFromPlanes(PlaneF const* _Planes, int _NumPlanes, TPodVector<Float3>& _Vertices)
{
    constexpr float tolerance         = 0.0001f;
    constexpr float quotientTolerance = 0.000001f;

    _Vertices.Clear();

    for (int i = 0; i < _NumPlanes; i++)
    {
        Float3 const& normal1 = _Planes[i].Normal;

        for (int j = i + 1; j < _NumPlanes; j++)
        {
            Float3 const& normal2 = _Planes[j].Normal;

            Float3 n1n2 = Math::Cross(normal1, normal2);

            if (n1n2.LengthSqr() > tolerance)
            {
                for (int k = j + 1; k < _NumPlanes; k++)
                {
                    Float3 const& normal3 = _Planes[k].Normal;

                    Float3 n2n3 = Math::Cross(normal2, normal3);
                    Float3 n3n1 = Math::Cross(normal3, normal1);

                    if ((n2n3.LengthSqr() > tolerance) && (n3n1.LengthSqr() > tolerance))
                    {
                        float quotient = Math::Dot(normal1, n2n3);
                        if (fabs(quotient) > quotientTolerance)
                        {
                            quotient = -1 / quotient;

                            Float3 potentialVertex = n2n3 * _Planes[i].D + n3n1 * _Planes[j].D + n1n2 * _Planes[k].D;
                            potentialVertex *= quotient;

                            if (IsPointInsideConvexHull(potentialVertex, _Planes, _NumPlanes, 0.01f))
                            {
                                _Vertices.Append(potentialVertex);
                            }
                        }
                    }
                }
            }
        }
    }
}

//struct SConvexDecompositionDesc {
//    // options
//    unsigned int  Depth;    // depth to split, a maximum of 10, generally not over 7.
//    float         ConcavityThreshold; // the concavity threshold percentage.  0=20 is reasonable.
//    float         VolumeConservationThreshold; // the percentage volume conservation threshold to collapse hulls. 0-30 is reasonable.

//    // hull output limits.
//    unsigned int  MaxHullVertices; // maximum number of vertices in the output hull. Recommended 32 or less.
//    float         HullSkinWidth;   // a skin width to apply to the output hulls.
//};

//class MyConvexDecompInterface : public ConvexDecomposition::ConvexDecompInterface {
//public:
//    MyConvexDecompInterface() {

//    }

//    void ConvexDecompResult( ConvexDecomposition::ConvexResult & _Result ) override {

//#if 0
//        // the convex hull.
//        unsigned int	    mHullVcount;
//        float *			    mHullVertices;
//        unsigned  int       mHullTcount;
//        unsigned int	*   mHullIndices;

//        float               mHullVolume;		    // the volume of the convex hull.

//        float               mOBBSides[3];			  // the width, height and breadth of the best fit OBB
//        float               mOBBCenter[3];      // the center of the OBB
//        float               mOBBOrientation[4]; // the quaternion rotation of the OBB.
//        float               mOBBTransform[16];  // the 4x4 transform of the OBB.
//        float               mOBBVolume;         // the volume of the OBB

//        float               mSphereRadius;      // radius and center of best fit sphere
//        float               mSphereCenter[3];
//        float               mSphereVolume;      // volume of the best fit sphere
//#endif

//        SConvexHullDesc & hull = Hulls.Append();

//        hull.FirstVertex = Vertices.Length();
//        hull.VertexCount = _Result.mHullVcount;
//        hull.FirstIndex = Indices.Length();
//        hull.IndexCount = _Result.mHullTcount;

//        Vertices.Resize( Vertices.Length() + hull.VertexCount );
//        Indices.Resize( Indices.Length() + hull.IndexCount );

//        Platform::Memcpy( &Vertices[ hull.FirstVertex ], _Result.mHullVertices, hull.VertexCount * (sizeof( float ) * 3) );
//        Platform::Memcpy( &Indices[ hull.FirstIndex ], _Result.mHullIndices, hull.IndexCount * sizeof( unsigned int ) );



//    }

//    struct SConvexHullDesc {
//        int FirstVertex;
//        int VertexCount;
//        int FirstIndex;
//        int IndexCount;

//#if 0
//        float   HullVolume;           // the volume of the convex hull.

//        float   OBBSides[3];          // the width, height and breadth of the best fit OBB
//        Float3  OBBCenter;         // the center of the OBB
//        float   OBBOrientation[4];    // the quaternion rotation of the OBB.
//        Float4x4 OBBTransform;     // the 4x4 transform of the OBB.
//        float   OBBVolume;            // the volume of the OBB

//        float   SphereRadius;         // radius and center of best fit sphere
//        Float3  SphereCenter;
//        float   SphereVolume;         // volume of the best fit sphere
//#endif
//    };

//    TPodVector< Float3 > Vertices;
//    TPodVector< unsigned int > Indices;
//    TPodVector< SConvexHullDesc > Hulls;
//};
// TODO: try ConvexBuilder

void BakeCollisionMarginConvexHull(Float3 const* _InVertices, int _VertexCount, TPodVector<Float3>& _OutVertices, float _Margin)
{
    TPodVector<PlaneF> planes;

    ConvexHullPlanesFromVertices(_InVertices, _VertexCount, planes);

    for (int i = 0; i < planes.Size(); i++)
    {
        PlaneF& plane = planes[i];

        plane.D += _Margin;
    }

    ConvexHullVerticesFromPlanes(planes.ToPtr(), planes.Size(), _OutVertices);
}

void PerformConvexDecomposition(Float3 const*                _Vertices,
                                int                          _VerticesCount,
                                int                          _VertexStride,
                                unsigned int const*          _Indices,
                                int                          _IndicesCount,
                                TPodVector<Float3>&          _OutVertices,
                                TPodVector<unsigned int>&    _OutIndices,
                                TPodVector<SConvexHullDesc>& _OutHulls)
{
    int hunkMark = GHunkMemory.SetHunkMark();

    HACD::Vec3<HACD::Real>* points =
        (HACD::Vec3<HACD::Real>*)GHunkMemory.Alloc(_VerticesCount * sizeof(HACD::Vec3<HACD::Real>));

    HACD::Vec3<long>* triangles =
        (HACD::Vec3<long>*)GHunkMemory.Alloc((_IndicesCount / 3) * sizeof(HACD::Vec3<long>));

    byte const* srcVertices = (byte const*)_Vertices;
    for (int i = 0; i < _VerticesCount; i++)
    {
        Float3 const* vertex = (Float3 const*)srcVertices;

        points[i] = HACD::Vec3<HACD::Real>(vertex->X, vertex->Y, vertex->Z);

        srcVertices += _VertexStride;
    }

    int triangleNum = 0;
    for (int i = 0; i < _IndicesCount; i += 3, triangleNum++)
    {
        triangles[triangleNum] = HACD::Vec3<long>(_Indices[i], _Indices[i + 1], _Indices[i + 2]);
    }

    HACD::HACD hacd;
    hacd.SetPoints(points);
    hacd.SetNPoints(_VerticesCount);
    hacd.SetTriangles(triangles);
    hacd.SetNTriangles(_IndicesCount / 3);
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

    int maxPointsPerCluster    = 0;
    int maxTrianglesPerCluster = 0;
    int totalPoints            = 0;
    int totalTriangles         = 0;

    int numClusters = hacd.GetNClusters();
    for (int cluster = 0; cluster < numClusters; cluster++)
    {
        int numPoints    = hacd.GetNPointsCH(cluster);
        int numTriangles = hacd.GetNTrianglesCH(cluster);

        totalPoints += numPoints;
        totalTriangles += numTriangles;

        maxPointsPerCluster    = Math::Max(maxPointsPerCluster, numPoints);
        maxTrianglesPerCluster = Math::Max(maxTrianglesPerCluster, numTriangles);
    }

    HACD::Vec3<HACD::Real>* hullPoints =
        (HACD::Vec3<HACD::Real>*)GHunkMemory.Alloc(maxPointsPerCluster * sizeof(HACD::Vec3<HACD::Real>));

    HACD::Vec3<long>* hullTriangles =
        (HACD::Vec3<long>*)GHunkMemory.Alloc(maxTrianglesPerCluster * sizeof(HACD::Vec3<long>));


    _OutHulls.ResizeInvalidate(numClusters);
    _OutVertices.ResizeInvalidate(totalPoints);
    _OutIndices.ResizeInvalidate(totalTriangles * 3);

    totalPoints    = 0;
    totalTriangles = 0;

    for (int cluster = 0; cluster < numClusters; cluster++)
    {
        int numPoints    = hacd.GetNPointsCH(cluster);
        int numTriangles = hacd.GetNTrianglesCH(cluster);

        hacd.GetCH(cluster, hullPoints, hullTriangles);

        SConvexHullDesc& hull = _OutHulls[cluster];
        hull.FirstVertex      = totalPoints;
        hull.VertexCount      = numPoints;
        hull.FirstIndex       = totalTriangles * 3;
        hull.IndexCount       = numTriangles * 3;
        hull.Centroid.Clear();

        totalPoints += numPoints;
        totalTriangles += numTriangles;

        Float3* pVertices = _OutVertices.ToPtr() + hull.FirstVertex;
        for (int i = 0; i < numPoints; i++, pVertices++)
        {
            pVertices->X = hullPoints[i].X();
            pVertices->Y = hullPoints[i].Y();
            pVertices->Z = hullPoints[i].Z();

            hull.Centroid += *pVertices;
        }

        hull.Centroid /= (float)numPoints;

        // Adjust vertices
        pVertices = _OutVertices.ToPtr() + hull.FirstVertex;
        for (int i = 0; i < numPoints; i++, pVertices++)
        {
            *pVertices -= hull.Centroid;
        }

        unsigned int* pIndices = _OutIndices.ToPtr() + hull.FirstIndex;
        int           n        = 0;
        for (int i = 0; i < hull.IndexCount; i += 3, n++)
        {
            *pIndices++ = hullTriangles[n].X();
            *pIndices++ = hullTriangles[n].Y();
            *pIndices++ = hullTriangles[n].Z();
        }
    }

    GHunkMemory.ClearToMark(hunkMark);
}

enum EVHACDMode
{
    VHACD_MODE_VOXEL       = 0, // recommended
    VHACD_MODE_TETRAHEDRON = 1
};

void PerformConvexDecompositionVHACD(Float3 const*                _Vertices,
                                     int                          _VerticesCount,
                                     int                          _VertexStride,
                                     unsigned int const*          _Indices,
                                     int                          _IndicesCount,
                                     TPodVector<Float3>&          _OutVertices,
                                     TPodVector<unsigned int>&    _OutIndices,
                                     TPodVector<SConvexHullDesc>& _OutHulls,
                                     Float3&                      _CenterOfMass)
{


    class Callback : public VHACD::IVHACD::IUserCallback
    {
    public:
        void Update(const double      overallProgress,
                    const double      stageProgress,
                    const double      operationProgress,
                    const char* const stage,
                    const char* const operation) override
        {

            GLogger.Printf("Overall progress %f, %s progress %f, %s progress %f\n",
                           overallProgress, stage, stageProgress, operation, operationProgress);
        }
    };
    class Logger : public VHACD::IVHACD::IUserLogger
    {
    public:
        void Log(const char* const msg) override
        {
            GLogger.Printf("%s", msg);
        }
    };

    Callback callback;
    Logger   logger;

    VHACD::IVHACD* vhacd = VHACD::CreateVHACD();

    VHACD::IVHACD::Parameters params;

    //    params.m_resolution = 80000000;//100000;
    //    params.m_concavity = 0.00001;
    //    params.m_planeDownsampling = 4;
    //    params.m_convexhullDownsampling = 4;
    //    params.m_alpha = 0.005;
    //    params.m_beta = 0.005;
    //    params.m_pca = 0;
    //    params.m_mode = VHACD_MODE_VOXEL;
    //    params.m_maxNumVerticesPerCH = 64;
    //    params.m_minVolumePerCH = 0.0001;
    params.m_callback = &callback;
    params.m_logger   = &logger;
    //    params.m_convexhullApproximation = true;
    //    params.m_oclAcceleration = false;//true;
    //    params.m_maxConvexHulls = 1024*10;
    //    params.m_projectHullVertices = true; // This will project the output convex hull vertices onto the original source mesh to increase the floating point accuracy of the results

    params.m_resolution              = 100000;
    params.m_planeDownsampling       = 1;
    params.m_convexhullDownsampling  = 1;
    params.m_alpha                   = 0.0001;
    params.m_beta                    = 0.0001;
    params.m_pca                     = 0;
    params.m_convexhullApproximation = false;
    params.m_concavity               = 0.00000001;
    params.m_mode                    = VHACD_MODE_VOXEL; //VHACD_MODE_TETRAHEDRON;
    params.m_oclAcceleration         = false;
    params.m_projectHullVertices     = false;

    int hunkMark = GHunkMemory.SetHunkMark();

    Double3* tempVertices = (Double3*)GHunkMemory.Alloc(_VerticesCount * sizeof(Double3));

    byte const* srcVertices = (byte const*)_Vertices;
    for (int i = 0; i < _VerticesCount; i++)
    {
        tempVertices[i] = Double3(*(Float3 const*)srcVertices);

        srcVertices += _VertexStride;
    }

    bool bResult = vhacd->Compute((const double*)tempVertices, _VerticesCount, _Indices, _IndicesCount / 3, params);

    if (bResult)
    {
        double centerOfMass[3];
        if (!vhacd->ComputeCenterOfMass(centerOfMass))
        {
            centerOfMass[0] = centerOfMass[1] = centerOfMass[2] = 0;
        }

        _CenterOfMass[0] = centerOfMass[0];
        _CenterOfMass[1] = centerOfMass[1];
        _CenterOfMass[2] = centerOfMass[2];

        VHACD::IVHACD::ConvexHull ch;
        _OutHulls.ResizeInvalidate(vhacd->GetNConvexHulls());
        int totalVertices = 0;
        int totalIndices  = 0;
        for (int i = 0; i < _OutHulls.Size(); i++)
        {
            SConvexHullDesc& hull = _OutHulls[i];

            vhacd->GetConvexHull(i, ch);

            hull.FirstVertex = totalVertices;
            hull.VertexCount = ch.m_nPoints;
            hull.FirstIndex  = totalIndices;
            hull.IndexCount  = ch.m_nTriangles * 3;
            hull.Centroid[0] = ch.m_center[0];
            hull.Centroid[1] = ch.m_center[1];
            hull.Centroid[2] = ch.m_center[2];

            totalVertices += hull.VertexCount;
            totalIndices += hull.IndexCount;
        }

        _OutVertices.ResizeInvalidate(totalVertices);
        _OutIndices.ResizeInvalidate(totalIndices);

        for (int i = 0; i < _OutHulls.Size(); i++)
        {
            SConvexHullDesc& hull = _OutHulls[i];

            vhacd->GetConvexHull(i, ch);

            Float3* pVertices = _OutVertices.ToPtr() + hull.FirstVertex;
            for (int v = 0; v < hull.VertexCount; v++, pVertices++)
            {
                pVertices->X = ch.m_points[v * 3 + 0] - ch.m_center[0];
                pVertices->Y = ch.m_points[v * 3 + 1] - ch.m_center[1];
                pVertices->Z = ch.m_points[v * 3 + 2] - ch.m_center[2];
            }

            unsigned int* pIndices = _OutIndices.ToPtr() + hull.FirstIndex;
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
        GLogger.Printf("PerformConvexDecompositionVHACD: convex decomposition error\n");

        _OutVertices.Clear();
        _OutIndices.Clear();
        _OutHulls.Clear();
    }

    vhacd->Clean();
    vhacd->Release();

    GHunkMemory.ClearToMark(hunkMark);
}