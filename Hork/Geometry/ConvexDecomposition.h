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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Math/Plane.h>

HK_NAMESPACE_BEGIN

struct ConvexHullDesc
{
    int    FirstVertex;
    int    VertexCount;
    int    FirstIndex;
    int    IndexCount;
    Float3 Centroid;
};

struct VHACDParameters
{
    enum class FillMode
    {
        /// This is the default behavior, after the voxelization step it uses a flood fill to determine 'inside'
        /// from 'outside'. However, meshes with holes can fail and create hollow results.
        FLOOD_FILL,

        /// Only consider the 'surface', will create 'skins' with hollow centers.
        SURFACE_ONLY,

        /// Uses raycasting to determine inside from outside.
        RAYCAST_FILL,
    };

    /// The maximum number of convex hulls to produce
    uint32_t            MaxConvexHulls{64};

    /// The voxel resolution to use
    uint32_t            VoxelResolution{400000};

    /// if the voxels are within 1% of the volume of the hull, we consider this a close enough approximation
    double              MinimumVolumePercentErrorAllowed{1};

    /// The maximum recursion depth
    uint32_t            MaxRecursionDepth{10};

    /// Whether or not to shrinkwrap the voxel positions to the source mesh on output
    bool                ShrinkWrap{true};

    /// How to fill the interior of the voxelized mesh
    VHACDParameters::FillMode FillMode{FillMode::FLOOD_FILL};

    /// The maximum number of vertices allowed in any output convex hull
    uint32_t            MaxNumVerticesPerCH{64};

    /// Once a voxel patch has an edge length of less than 2 on all 3 sides, we don't keep recursing
    uint32_t            MinEdgeLength{2};

    /// Whether or not to attempt to split planes along the best location
    bool                FindBestPlane{false};
};

namespace Geometry
{

void BakeCollisionMarginConvexHull(Float3 const* vertices, int vertexCount, Vector<Float3>& outVertices, float margin = 0.01f);

bool PerformConvexDecompositionVHACD(VHACDParameters const& params,
                                     Float3 const* vertices,
                                     int vertexCount,
                                     int vertexStride,
                                     unsigned int const* indices,
                                     int indexCount,
                                     Vector<Float3>& outVertices,
                                     Vector<unsigned int>& outIndices,
                                     Vector<ConvexHullDesc>& outHulls,
                                     Float3& centerOfMass);

void ConvexHullPlanesFromVertices(Float3 const* vertices, int vertexCount, Vector<PlaneF>& planes);

void ConvexHullVerticesFromPlanes(PlaneF const* planes, int planeCount, Vector<Float3>& vertices);

} // namespace Geometry

HK_NAMESPACE_END
