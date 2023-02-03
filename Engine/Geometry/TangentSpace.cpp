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

#include "TangentSpace.h"
#include <Engine/Core/Containers/Vector.h>

#include <MikkTSpace/mikktspace.h>

HK_NAMESPACE_BEGIN

namespace Geometry
{

void CalcTangentSpaceLegacy(MeshVertex* VertexArray, unsigned int NumVerts, unsigned int const* IndexArray, unsigned int NumIndices)
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

bool CalcTangentSpaceMikkTSpace(MeshVertex* VertexArray, unsigned int const* IndexArray, unsigned int NumIndices)
{
    struct GeometryData
    {
        MeshVertex* VertexArray;
        unsigned int const* IndexArray;
        unsigned int NumFaces;
    };

    GeometryData data;
    data.VertexArray = VertexArray;
    data.IndexArray = IndexArray;
    data.NumFaces = NumIndices / 3;

    SMikkTSpaceInterface iface = {};
    iface.m_getNumFaces = [](SMikkTSpaceContext const* context) -> int
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        return data->NumFaces;
    };
    iface.m_getNumVerticesOfFace = [](SMikkTSpaceContext const* context, const int faceNum) -> int
    {
        return 3;
    };
    iface.m_getPosition = [](SMikkTSpaceContext const* context, float posOut[], const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        MeshVertex const& vertex = data->VertexArray[data->IndexArray[faceNum * 3 + vertNum]];
        *((Float3*)&posOut[0]) = vertex.Position;
    };
    iface.m_getNormal = [](SMikkTSpaceContext const* context, float normOut[], const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        MeshVertex const& vertex = data->VertexArray[data->IndexArray[faceNum * 3 + vertNum]];
        *((Float3*)&normOut[0]) = vertex.GetNormal();
    };
    iface.m_getTexCoord = [](SMikkTSpaceContext const* context, float texCoordOut[], const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        MeshVertex const& vertex = data->VertexArray[data->IndexArray[faceNum * 3 + vertNum]];
        *((Float2*)&texCoordOut[0]) = vertex.GetTexCoord();
    };
    iface.m_setTSpaceBasic = [](SMikkTSpaceContext const* context, const float tangent[], const float fSign, const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        MeshVertex& vertex = data->VertexArray[data->IndexArray[faceNum * 3 + vertNum]];
        vertex.SetTangent(tangent[0], tangent[1], tangent[2]);
        vertex.Handedness = (int8_t)fSign;
    };

    SMikkTSpaceContext ctx{&iface, &data};
    if (!genTangSpaceDefault(&ctx))
    {
        LOG("Failed on tangent space calculation\n");
        return false;
    }
    return true;
}

} // namespace Geometry

HK_NAMESPACE_END
