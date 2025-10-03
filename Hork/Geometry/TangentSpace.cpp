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

#include "TangentSpace.h"

#include <Hork/Core/Logger.h>
#include <MikkTSpace/mikktspace.h>

HK_NAMESPACE_BEGIN

namespace Geometry
{

bool CalcTangentSpace(MeshVertex* vertices, unsigned int const* indices, unsigned int indexCount)
{
    struct GeometryData
    {
        MeshVertex* VertexArray;
        unsigned int const* IndexArray;
        unsigned int NumFaces;
    };

    GeometryData data;
    data.VertexArray = vertices;
    data.IndexArray = indices;
    data.NumFaces = indexCount / 3;

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

bool CalcTangentSpace(Float3 const* positions, Float2 const* texCoords, Float3 const* normals, Float4* tangents, unsigned int const* indices, unsigned int indexCount)
{
    struct GeometryData
    {
        Float3 const* Positions;
        Float2 const* TexCoords;
        Float3 const* Normals;
        Float4* Tangents;

        unsigned int const* IndexArray;
        unsigned int NumFaces;
    };

    GeometryData data;
    data.Positions = positions;
    data.TexCoords = texCoords;
    data.Normals = normals;
    data.Tangents = tangents;
    data.IndexArray = indices;
    data.NumFaces = indexCount / 3;

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
        *((Float3*)&posOut[0]) = data->Positions[data->IndexArray[faceNum * 3 + vertNum]];
    };
    iface.m_getNormal = [](SMikkTSpaceContext const* context, float normOut[], const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        *((Float3*)&normOut[0]) = data->Normals[data->IndexArray[faceNum * 3 + vertNum]];
    };
    iface.m_getTexCoord = [](SMikkTSpaceContext const* context, float texCoordOut[], const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        *((Float2*)&texCoordOut[0]) = data->TexCoords[data->IndexArray[faceNum * 3 + vertNum]];
    };
    iface.m_setTSpaceBasic = [](SMikkTSpaceContext const* context, const float tangent[], const float fSign, const int faceNum, const int vertNum)
    {
        GeometryData* data = (GeometryData*)context->m_pUserData;
        Float4& outTangent = data->Tangents[data->IndexArray[faceNum * 3 + vertNum]];
        outTangent[0] = tangent[0];
        outTangent[1] = tangent[1];
        outTangent[2] = tangent[2];
        outTangent[3] = fSign;
    };

    SMikkTSpaceContext ctx{&iface, &data};
    if (!genTangSpaceDefault(&ctx))
    {
        LOG("Failed on tangent space calculation\n");
        return false;
    }
    return true;
}

void CalcNormals(Float3 const* positions, Float3* normals, unsigned int vertexCount, unsigned int const* indices, unsigned int indexCount)
{
    int triangleCount = indexCount / 3;

    Core::ZeroMem(normals, vertexCount * sizeof(Float3));

    for (int n = 0; n < triangleCount; ++n)
    {
        auto& v0 = positions[indices[n * 3    ]];
        auto& v1 = positions[indices[n * 3 + 1]];
        auto& v2 = positions[indices[n * 3 + 2]];

        auto e0 = v1 - v0;
        auto e1 = v2 - v0;

        auto normal = Math::Cross(e0, e1).Normalized();

        normals[indices[n * 3    ]] += normal;
        normals[indices[n * 3 + 1]] += normal;
        normals[indices[n * 3 + 2]] += normal;
    }

    for (int n = 0; n < vertexCount; ++n)
        normals[n].NormalizeSelf();
}

} // namespace Geometry

HK_NAMESPACE_END
