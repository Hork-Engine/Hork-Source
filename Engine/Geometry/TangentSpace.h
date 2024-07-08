/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Engine/Geometry/VertexFormat.h>

HK_NAMESPACE_BEGIN

namespace Geometry
{

void CalcTangentSpaceLegacy(MeshVertex* VertexArray, unsigned int NumVerts, unsigned int const* IndexArray, unsigned int NumIndices);

bool CalcTangentSpaceMikkTSpace(MeshVertex* VertexArray, unsigned int const* IndexArray, unsigned int NumIndices);
bool CalcTangentSpaceMikkTSpace(Float3 const* Positions, Float2 const* TexCoords, Float3 const* Normals, Float4* Tangents, unsigned int const* IndexArray, unsigned int NumIndices);

HK_FORCEINLINE void CalcTangentSpace(MeshVertex* VertexArray, unsigned int NumVerts, unsigned int const* IndexArray, unsigned int NumIndices)
{
    #if 1
    HK_UNUSED(NumVerts);
    CalcTangentSpaceMikkTSpace(VertexArray, IndexArray, NumIndices);
    #else
    CalcTangentSpaceLegacy(VertexArray, NumVerts, IndexArray, NumIndices);
    #endif
}

HK_FORCEINLINE void CalcTangentSpace(Float3 const* Positions, Float2 const* TexCoords, Float3 const* Normals, Float4* Tangents, unsigned int const* IndexArray, unsigned int NumIndices)
{
    CalcTangentSpaceMikkTSpace(Positions, TexCoords, Normals, Tangents, IndexArray, NumIndices);
}

/** binormal = cross( normal, tangent ) * handedness */
HK_FORCEINLINE float CalcHandedness(Float3 const& Tangent, Float3 const& Binormal, Float3 const& Normal)
{
    return (Math::Dot(Math::Cross(Normal, Tangent), Binormal) < 0.0f) ? -1.0f : 1.0f;
}

HK_FORCEINLINE Float3 CalcBinormal(Float3 const& Tangent, Float3 const& Normal, float Handedness)
{
    return Math::Cross(Normal, Tangent).Normalized() * Handedness;
}

} // namespace Geometry

HK_NAMESPACE_END
