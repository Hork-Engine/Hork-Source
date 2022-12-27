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

#pragma once

#include <Containers/Vector.h>
#include <Geometry/Plane.h>

HK_NAMESPACE_BEGIN

struct ConvexHullDesc
{
    int    FirstVertex;
    int    VertexCount;
    int    FirstIndex;
    int    IndexCount;
    Float3 Centroid;
};

namespace Geometry
{

void BakeCollisionMarginConvexHull(Float3 const* _InVertices, int _VertexCount, TPodVector<Float3>& _OutVertices, float _Margin = 0.01f);

bool PerformConvexDecomposition(Float3 const*                _Vertices,
                                int                          _VerticesCount,
                                int                          _VertexStride,
                                unsigned int const*          _Indices,
                                int                          _IndicesCount,
                                TPodVector<Float3>&          _OutVertices,
                                TPodVector<unsigned int>&    _OutIndices,
                                TPodVector<ConvexHullDesc>& _OutHulls);

bool PerformConvexDecompositionVHACD(Float3 const*                _Vertices,
                                     int                          _VerticesCount,
                                     int                          _VertexStride,
                                     unsigned int const*          _Indices,
                                     int                          _IndicesCount,
                                     TPodVector<Float3>&          _OutVertices,
                                     TPodVector<unsigned int>&    _OutIndices,
                                     TPodVector<ConvexHullDesc>& _OutHulls,
                                     Float3&                      _CenterOfMass);

void ConvexHullPlanesFromVertices(Float3 const* _Vertices, int _NumVertices, TPodVector<PlaneF>& _Planes);

void ConvexHullVerticesFromPlanes(PlaneF const* _Planes, int _NumPlanes, TPodVector<Float3>& _Vertices);

} // namespace Geometry

HK_NAMESPACE_END
