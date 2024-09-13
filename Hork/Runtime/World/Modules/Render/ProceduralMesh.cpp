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

#include "ProceduralMesh.h"
#include "RenderContext.h"

#include <Hork/RHI/Common/VertexMemoryGPU.h>
#include <Hork/Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

void ProceduralMesh::GetVertexBufferGPU(StreamedMemoryGPU* streamedMemory, RHI::IBuffer** ppBuffer, size_t* pOffset)
{
    streamedMemory->GetPhysicalBufferAndOffset(m_VertexStream, ppBuffer, pOffset);
}

void ProceduralMesh::GetIndexBufferGPU(StreamedMemoryGPU* streamedMemory, RHI::IBuffer** ppBuffer, size_t* pOffset)
{
    streamedMemory->GetPhysicalBufferAndOffset(m_IndexSteam, ppBuffer, pOffset);
}

void ProceduralMesh::PrepareStreams(RenderContext const& context)
{
    if (m_VisFrame == context.FrameNumber)
    {
        return;
    }

    m_VisFrame = context.FrameNumber;

    if (!VertexCache.IsEmpty() && !IndexCache.IsEmpty())
    {
        StreamedMemoryGPU* streamedMemory = context.StreamedMemory;

        m_VertexStream = streamedMemory->AllocateVertex(sizeof(MeshVertex) * VertexCache.Size(), VertexCache.ToPtr());
        m_IndexSteam = streamedMemory->AllocateIndex(sizeof(unsigned int) * IndexCache.Size(), IndexCache.ToPtr());
    }
}

bool ProceduralMesh::Raycast(Float3 const& rayStart, Float3 const& rayDir, float distance, bool cullBackFace, Vector<TriangleHitResult>& hitResult) const
{
    if (distance < 0.0001f)
    {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    if (!BvRayIntersectBox(rayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= distance)
    {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool ret = false;
    float d, u, v;
    unsigned int const* indices = IndexCache.ToPtr() + FirstIndex;
    MeshVertex const* vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for (int tri = 0; tri < primCount; tri++, indices += 3)
    {
        const unsigned int i0 = BaseVertex + indices[0];
        const unsigned int i1 = BaseVertex + indices[1];
        const unsigned int i2 = BaseVertex + indices[2];

        Float3 const& v0 = vertices[i0].Position;
        Float3 const& v1 = vertices[i1].Position;
        Float3 const& v2 = vertices[i2].Position;

        if (BvRayIntersectTriangle(rayStart, rayDir, v0, v1, v2, d, u, v, cullBackFace))
        {
            if (distance > d)
            {
                TriangleHitResult& hit = hitResult.Add();
                hit.Location = rayStart + rayDir * d;
                hit.Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                hit.Distance = d;
                hit.UV.X = u;
                hit.UV.Y = v;
                hit.Indices[0] = i0;
                hit.Indices[1] = i1;
                hit.Indices[2] = i2;
                //hit.Material = nullptr;
                ret = true;
            }
        }
    }

    return ret;
}

bool ProceduralMesh::RaycastClosest(Float3 const& rayStart, Float3 const& rayDir, float distance, bool cullBackFace, Float3& hitLocation, Float2& hitUV, float& hitDistance, unsigned int triangleIndices[3]) const
{
    if (distance < 0.0001f)
    {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    if (!BvRayIntersectBox(rayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= distance)
    {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool ret = false;
    float d, u, v;
    unsigned int const* indices = IndexCache.ToPtr() + FirstIndex;
    MeshVertex const* vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for (int tri = 0; tri < primCount; tri++, indices += 3)
    {
        const unsigned int i0 = BaseVertex + indices[0];
        const unsigned int i1 = BaseVertex + indices[1];
        const unsigned int i2 = BaseVertex + indices[2];

        Float3 const& v0 = vertices[i0].Position;
        Float3 const& v1 = vertices[i1].Position;
        Float3 const& v2 = vertices[i2].Position;

        if (BvRayIntersectTriangle(rayStart, rayDir, v0, v1, v2, d, u, v, cullBackFace))
        {
            if (distance > d)
            {
                distance = d;
                hitLocation = rayStart + rayDir * d;
                hitDistance = d;
                hitUV.X = u;
                hitUV.Y = v;
                triangleIndices[0] = i0;
                triangleIndices[1] = i1;
                triangleIndices[2] = i2;
                ret = true;
            }
        }
    }

    return ret;
}

HK_NAMESPACE_END
