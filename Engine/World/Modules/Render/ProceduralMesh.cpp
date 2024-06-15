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
#include "RenderFrontend.h"

#include <Engine/RenderCore/VertexMemoryGPU.h>
#include <Engine/Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

ProceduralMesh_ECS::ProceduralMesh_ECS()
{
    BoundingBox.Clear();
}

ProceduralMesh_ECS::~ProceduralMesh_ECS()
{
}

void ProceduralMesh_ECS::GetVertexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    StreamedMemory->GetPhysicalBufferAndOffset(m_VertexStream, ppBuffer, pOffset);
}

void ProceduralMesh_ECS::GetIndexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset)
{
    StreamedMemory->GetPhysicalBufferAndOffset(m_IndexSteam, ppBuffer, pOffset);
}

void ProceduralMesh_ECS::PrepareStreams(RenderFrontendDef const* pDef)
{
    if (m_VisFrame == pDef->FrameNumber)
    {
        return;
    }

    m_VisFrame = pDef->FrameNumber;

    if (!VertexCache.IsEmpty() && !IndexCache.IsEmpty())
    {
        StreamedMemoryGPU* streamedMemory = pDef->StreamedMemory;

        m_VertexStream = streamedMemory->AllocateVertex(sizeof(MeshVertex) * VertexCache.Size(), VertexCache.ToPtr());
        m_IndexSteam   = streamedMemory->AllocateIndex(sizeof(unsigned int) * IndexCache.Size(), IndexCache.ToPtr());
    }
}

bool ProceduralMesh_ECS::Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Vector<TriangleHitResult>& HitResult) const
{
    if (Distance < 0.0001f)
    {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool                ret = false;
    float               d, u, v;
    unsigned int const* indices  = IndexCache.ToPtr() + FirstIndex;
    MeshVertex const*  vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for (int tri = 0; tri < primCount; tri++, indices += 3)
    {
        const unsigned int i0 = BaseVertex + indices[0];
        const unsigned int i1 = BaseVertex + indices[1];
        const unsigned int i2 = BaseVertex + indices[2];

        Float3 const& v0 = vertices[i0].Position;
        Float3 const& v1 = vertices[i1].Position;
        Float3 const& v2 = vertices[i2].Position;

        if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
        {
            if (Distance > d)
            {
                TriangleHitResult& hitResult = HitResult.Add();
                hitResult.Location            = RayStart + RayDir * d;
                hitResult.Normal              = Math::Cross(v1 - v0, v2 - v0).Normalized();
                hitResult.Distance            = d;
                hitResult.UV.X                = u;
                hitResult.UV.Y                = v;
                hitResult.Indices[0]          = i0;
                hitResult.Indices[1]          = i1;
                hitResult.Indices[2]          = i2;
                //hitResult.Material            = nullptr;
                ret                           = true;
            }
        }
    }

    return ret;
}

bool ProceduralMesh_ECS::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const
{
    if (Distance < 0.0001f)
    {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool                ret = false;
    float               d, u, v;
    unsigned int const* indices  = IndexCache.ToPtr() + FirstIndex;
    MeshVertex const*  vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for (int tri = 0; tri < primCount; tri++, indices += 3)
    {
        const unsigned int i0 = BaseVertex + indices[0];
        const unsigned int i1 = BaseVertex + indices[1];
        const unsigned int i2 = BaseVertex + indices[2];

        Float3 const& v0 = vertices[i0].Position;
        Float3 const& v1 = vertices[i1].Position;
        Float3 const& v2 = vertices[i2].Position;

        if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
        {
            if (Distance > d)
            {
                Distance     = d;
                HitLocation  = RayStart + RayDir * d;
                HitDistance  = d;
                HitUV.X      = u;
                HitUV.Y      = v;
                Indices[0]   = i0;
                Indices[1]   = i1;
                Indices[2]   = i2;
                ret          = true;
            }
        }
    }

    return ret;
}

HK_NAMESPACE_END
