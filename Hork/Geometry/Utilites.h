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
#include "BV/BvAxisAlignedBox.h"

HK_NAMESPACE_BEGIN

namespace Geometry
{

void CreateBoxMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, Float3 const& extents, float texCoordScale);
void CreateSphereMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs);
void CreatePlaneMeshXZ(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float width, float height, Float2 const& texCoordScale);
void CreatePlaneMeshXY(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float width, float height, Float2 const& texCoordScale);
void CreatePatchMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, Float3 const& corner00, Float3 const& corner10, Float3 const& corner01, Float3 const& corner11, float texCoordScale, bool isTwoSided, int numVerticalSubdivs, int numHorizontalSubdivs);
void CreateCylinderMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float height, float texCoordScale, int numSubdivs);
void CreateConeMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float height, float texCoordScale, int numSubdivs);
void CreateCapsuleMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float height, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs);
void CreateSkyboxMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, Float3 const& extents, float texCoordScale);
void CreateSkydomeMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs, bool isHemisphere);

}

HK_NAMESPACE_END
