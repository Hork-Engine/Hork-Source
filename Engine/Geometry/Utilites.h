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

#include <Engine/Core/Containers/Vector.h>
#include "BV/BvAxisAlignedBox.h"
#include "VertexFormat.h"

HK_NAMESPACE_BEGIN

template <typename VertexType>
using VertexBufferCPU = Vector<VertexType, Allocators::HeapMemoryAllocator<HEAP_CPU_VERTEX_BUFFER>>;

template <typename IndexType>
using IndexBufferCPU = Vector<IndexType, Allocators::HeapMemoryAllocator<HEAP_CPU_INDEX_BUFFER>>;

void CreateBoxMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale);

void CreateSphereMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32);

void CreatePlaneMeshXZ(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale);

void CreatePlaneMeshXY(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale);

void CreatePatchMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs);

void CreateCylinderMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

void CreateConeMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs = 32);

void CreateCapsuleMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs = 6, int NumHorizontalSubdivs = 8);

void CreateSkyboxMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale);

void CreateSkydomeMesh(VertexBufferCPU<MeshVertex>& Vertices, IndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs = 32, int NumHorizontalSubdivs = 32, bool bHemisphere = true);

HK_NAMESPACE_END
