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

#include <Engine/Geometry/Utilites.h>
#include <Engine/RenderCore/Buffer.h>
#include <Engine/World/Resources/Resource_Mesh.h>

HK_NAMESPACE_BEGIN

class StreamedMemoryGPU;
struct RenderFrontendDef;
struct TriangleHitResult;

class ProceduralMesh : public RefCounted
{
public:
    ProceduralMesh();
    ~ProceduralMesh();

    /** Update vertex cache occasionally or every frame */
    VertexBufferCPU<MeshVertex> VertexCache;

    /** Update index cache occasionally or every frame */
    IndexBufferCPU<unsigned int> IndexCache;

    /** Bounding box is used for raycast early exit and VSD culling */
    BvAxisAlignedBox BoundingBox;

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Get mesh GPU buffers */
    void GetIndexBufferGPU(StreamedMemoryGPU* StreamedMemory, RenderCore::IBuffer** ppBuffer, size_t* pOffset);

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Vector<TriangleHitResult>& HitResult) const;

    /** Check ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, Float3& HitLocation, Float2& HitUV, float& HitDistance, unsigned int Indices[3]) const;

    /** Called before rendering. Don't call directly. */
    void PrepareStreams(RenderFrontendDef const* pDef);

    // TODO: Add methods like AddTriangle, AddQuad, etc.
private:
    size_t m_VertexStream = 0;
    size_t m_IndexSteam   = 0;

    int m_VisFrame = -1;
};


HK_NAMESPACE_END
