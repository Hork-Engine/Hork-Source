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

#include <Hork/Geometry/VertexFormat.h>

HK_NAMESPACE_BEGIN

namespace Geometry
{

bool CalcTangentSpace(MeshVertex* vertices, unsigned int const* indices, unsigned int indexCount);
bool CalcTangentSpace(Float3 const* positions, Float2 const* texCoords, Float3 const* normals, Float4* tangents, unsigned int const* indices, unsigned int indexCount);

/// binormal = cross( normal, tangent ) * handedness
HK_FORCEINLINE float CalcHandedness(Float3 const& tangent, Float3 const& binormal, Float3 const& normal)
{
    return (Math::Dot(Math::Cross(normal, tangent), binormal) < 0.0f) ? -1.0f : 1.0f;
}

HK_FORCEINLINE Float3 CalcBinormal(Float3 const& tangent, Float3 const& normal, float handedness)
{
    return Math::Cross(normal, tangent).Normalized() * handedness;
}

void CalcNormals(Float3 const* positions, Float3* normals, unsigned int vertexCount, unsigned int const* indices, unsigned int indexCount);

} // namespace Geometry

HK_NAMESPACE_END
