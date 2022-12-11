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

#include "TangentSpace.h"
#include <Containers/Vector.h>

namespace Geometry
{

void CalcTangentSpace(MeshVertex* VertexArray, unsigned int NumVerts, unsigned int const* IndexArray, unsigned int NumIndices)
{
    Float3 binormal, tangent;

    TVector<Float3> binormals(NumVerts);
    TVector<Float3> tangents(NumVerts);

    for (unsigned int i = 0; i < NumIndices; i += 3)
    {
        const unsigned int a = IndexArray[i];
        const unsigned int b = IndexArray[i + 1];
        const unsigned int c = IndexArray[i + 2];

        Float3 e1  = VertexArray[b].Position - VertexArray[a].Position;
        Float3 e2  = VertexArray[c].Position - VertexArray[a].Position;
        Float2 et1 = VertexArray[b].GetTexCoord() - VertexArray[a].GetTexCoord();
        Float2 et2 = VertexArray[c].GetTexCoord() - VertexArray[a].GetTexCoord();

        const float denom = et1.X * et2.Y - et1.Y * et2.X;
        const float scale = (Math::Abs(denom) < 0.0001f) ? 1.0f : (1.0 / denom);
        tangent           = (e1 * et2.Y - e2 * et1.Y) * scale;
        binormal          = (e2 * et1.X - e1 * et2.X) * scale;

        tangents[a] += tangent;
        tangents[b] += tangent;
        tangents[c] += tangent;

        binormals[a] += binormal;
        binormals[b] += binormal;
        binormals[c] += binormal;
    }

    for (int i = 0; i < NumVerts; i++)
    {
        const Float3  n = VertexArray[i].GetNormal();
        Float3 const& t = tangents[i];
        VertexArray[i].SetTangent((t - n * Math::Dot(n, t)).Normalized());
        VertexArray[i].Handedness = (int8_t)CalcHandedness(t, binormals[i].Normalized(), n);
    }
}

} // namespace Geometry
