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

/*

NOTE: The terrain is still in the early development stage.


TODO:

Textures update
Streaming
Frustum culling:
 Calc ZMin, ZMax for each block. The calc can be made approximately,
 based on the height of the center:
   zMin = centerH - blockSize * f;
   zMax = centerH + blockSize * f;
   f - some value that gives a margin

FIXME: move normal texture fetching to fragment shader?

Future:
 Precalculate occluders inside mountains so that invisible objects can be cut off.

Modify NavMesh

*/

#include "TerrainMesh.h"
#include "Engine.h"

HK_NAMESPACE_BEGIN

static const unsigned short RESET_INDEX = 0xffff;

static void CreateTriangleStripPatch(int NumQuadsX, int NumQuadsY, TVector<TerrainVertex>& Vertices, TVector<unsigned short>& Indices)
{
    int numVertsX = NumQuadsX + 1;
    int numVertsY = NumQuadsY + 1;

    Vertices.Resize(numVertsX * numVertsY);

    for (int y = 0; y < NumQuadsY; y++)
    {
        for (int x = 0; x < NumQuadsX + 1; x++)
        {
            Indices.Add(x + (y)*numVertsX);
            Indices.Add(x + (y + 1) * numVertsX);
        }
        Indices.Add(RESET_INDEX);
    }

    TerrainVertex* vert = Vertices.ToPtr();

    for (int i = 0; i < numVertsY; i++)
    {
        for (int j = 0; j < numVertsX; j++)
        {
            vert[i * numVertsX + j].X = j;
            vert[i * numVertsX + j].Y = i;
        }
    }
}

HK_FORCEINLINE TerrainVertex MakeVertex(short X, short Y)
{
    TerrainVertex v;
    v.X = X;
    v.Y = Y;
    return v;
}

TerrainMesh::TerrainMesh(int InTextureSize)
{
    TVector<TerrainVertex> blockVerts;
    TVector<unsigned short> blockIndices;
    TVector<TerrainVertex> horGapVertices;
    TVector<unsigned short> horGapIndices;
    TVector<TerrainVertex> vertGapVertices;
    TVector<unsigned short> vertGapIndices;
    TVector<TerrainVertex> interiorTLVertices;
    TVector<TerrainVertex> interiorTRVertices;
    TVector<TerrainVertex> interiorBLVertices;
    TVector<TerrainVertex> interiorBRVertices;
    TVector<TerrainVertex> interiorFinestVertices;
    TVector<unsigned short> interiorTLIndices;
    TVector<unsigned short> interiorTRIndices;
    TVector<unsigned short> interiorBLIndices;
    TVector<unsigned short> interiorBRIndices;
    TVector<unsigned short> interiorFinestIndices;
    TVector<TerrainVertex> crackVertices;
    TVector<unsigned short> crackIndices;

    HK_ASSERT(IsPowerOfTwo(InTextureSize));

    const int BlockWidth = InTextureSize / 4 - 1;
    const int GapWidth = 2;
    const int CrackTrianglesCount = (BlockWidth * 4 + GapWidth) / 2;

    m_TextureSize = InTextureSize;

    // Blocks
    CreateTriangleStripPatch(BlockWidth, BlockWidth, blockVerts, blockIndices);

    // Gaps
    CreateTriangleStripPatch(BlockWidth, GapWidth, horGapVertices, horGapIndices);
    CreateTriangleStripPatch(GapWidth, BlockWidth, vertGapVertices, vertGapIndices);

    // Interior L-shapes
    int i = 0;
    for (int q = 0; q < (BlockWidth * 2 + GapWidth) + 1; q++)
    {
        interiorTLVertices.Add(MakeVertex(q, 0));
        interiorTLVertices.Add(MakeVertex(q, 1));

        interiorTRVertices.Add(MakeVertex(q, 0));
        interiorTRVertices.Add(MakeVertex(q, 1));

        interiorBLVertices.Add(MakeVertex(q, BlockWidth * 2 + GapWidth - 1));
        interiorBLVertices.Add(MakeVertex(q, BlockWidth * 2 + GapWidth));

        interiorBRVertices.Add(MakeVertex(q, BlockWidth * 2 + GapWidth - 1));
        interiorBRVertices.Add(MakeVertex(q, BlockWidth * 2 + GapWidth));

        interiorTLIndices.Add(i);
        interiorTLIndices.Add(i + 1);

        interiorTRIndices.Add(i);
        interiorTRIndices.Add(i + 1);

        interiorBLIndices.Add(i);
        interiorBLIndices.Add(i + 1);

        interiorBRIndices.Add(i);
        interiorBRIndices.Add(i + 1);

        i += 2;
    }

    interiorTLIndices.Add(RESET_INDEX);
    interiorTRIndices.Add(RESET_INDEX);
    interiorBLIndices.Add(RESET_INDEX);
    interiorBRIndices.Add(RESET_INDEX);

    int prev_a_tl = 1;
    int prev_b_tl = prev_a_tl + 2;

    int prev_a_tr = ((BlockWidth * 2 + GapWidth) + 1) * 2 - 3;
    int prev_b_tr = prev_a_tr + 2;

    int q;
    for (q = 0; q < (BlockWidth * 2 + GapWidth) - 1; q++)
    {
        interiorTLIndices.Add(prev_a_tl);
        interiorTLIndices.Add(i);
        interiorTLIndices.Add(prev_b_tl);
        interiorTLIndices.Add(i + 1);
        prev_a_tl = i;
        prev_b_tl = i + 1;

        interiorTRIndices.Add(prev_a_tr);
        interiorTRIndices.Add(i);
        interiorTRIndices.Add(prev_b_tr);
        interiorTRIndices.Add(i + 1);
        prev_a_tr = i;
        prev_b_tr = i + 1;

        if (q < (BlockWidth * 2 + GapWidth) - 2)
        {
            interiorTLIndices.Add(RESET_INDEX);
            interiorTRIndices.Add(RESET_INDEX);

            interiorBLIndices.Add(i);
            interiorBLIndices.Add(i + 2);
            interiorBLIndices.Add(i + 1);
            interiorBLIndices.Add(i + 3);
            interiorBLIndices.Add(RESET_INDEX);

            interiorBRIndices.Add(i);
            interiorBRIndices.Add(i + 2);
            interiorBRIndices.Add(i + 1);
            interiorBRIndices.Add(i + 3);
            interiorBRIndices.Add(RESET_INDEX);

            i += 2;
        }

        interiorTLVertices.Add(MakeVertex(0, q + 2));
        interiorTLVertices.Add(MakeVertex(1, q + 2));

        interiorTRVertices.Add(MakeVertex((BlockWidth * 2 + GapWidth) - 1, q + 2));
        interiorTRVertices.Add(MakeVertex((BlockWidth * 2 + GapWidth), q + 2));

        interiorBLVertices.Add(MakeVertex(0, q));
        interiorBLVertices.Add(MakeVertex(1, q));

        interiorBRVertices.Add(MakeVertex((BlockWidth * 2 + GapWidth) - 1, q));
        interiorBRVertices.Add(MakeVertex((BlockWidth * 2 + GapWidth), q));
    }

    interiorBLIndices.Add(i);
    interiorBLIndices.Add(0);
    interiorBLIndices.Add(i + 1);
    interiorBLIndices.Add(2);

    interiorBRIndices.Add(i);
    interiorBRIndices.Add(((BlockWidth * 2 + GapWidth) + 1) * 2 - 4);
    interiorBRIndices.Add(i + 1);
    interiorBRIndices.Add(((BlockWidth * 2 + GapWidth) + 1) * 2 - 2);

    for (TerrainVertex& v : interiorTLVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for (TerrainVertex& v : interiorTRVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for (TerrainVertex& v : interiorBLVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for (TerrainVertex& v : interiorBRVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }

    // Finest lod interior L-shape

    i = 0;
    int x, y;

    y = BlockWidth * 2;
    for (x = 0; x < BlockWidth * 2 + 2; x++)
    {
        interiorFinestIndices.Add(i++);
        interiorFinestIndices.Add(i++);

        interiorFinestVertices.Add(MakeVertex(x, y));
        interiorFinestVertices.Add(MakeVertex(x, y + 1));
    }
    interiorFinestIndices.Add(RESET_INDEX);

    x = BlockWidth * 2;
    for (y = 0; y < BlockWidth * 2; y++)
    {
        interiorFinestIndices.Add(i);
        interiorFinestIndices.Add(i + 2);
        interiorFinestIndices.Add(i + 1);
        interiorFinestIndices.Add(i + 3);
        interiorFinestIndices.Add(RESET_INDEX);

        interiorFinestVertices.Add(MakeVertex(x, y));
        interiorFinestVertices.Add(MakeVertex(x + 1, y));

        i += 2;
    }

    interiorFinestVertices.Add(MakeVertex(x, y));
    interiorFinestVertices.Add(MakeVertex(x + 1, y));

    int debugOffset = 0; //-1;

    // top line
    int j = 0;
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Add(i * 2);
        crackIndices.Add(i * 2);
        crackIndices.Add(i * 2 + 1);
        crackIndices.Add(i * 2 + 2);

        crackVertices.Add(MakeVertex(i * 2, j));
        crackVertices.Add(MakeVertex(i * 2 + 1, j - debugOffset));
    }
    // right line
    j = CrackTrianglesCount * 2;
    int vertOfs = crackVertices.Size();
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Add(vertOfs + i * 2);
        crackIndices.Add(vertOfs + i * 2);
        crackIndices.Add(vertOfs + i * 2 + 1);
        crackIndices.Add(vertOfs + i * 2 + 2);

        crackVertices.Add(MakeVertex(j, i * 2));
        crackVertices.Add(MakeVertex(j + debugOffset, i * 2 + 1));
    }
    // bottom line
    j = CrackTrianglesCount * 2;
    vertOfs = crackVertices.Size();
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Add(vertOfs + i * 2);
        crackIndices.Add(vertOfs + i * 2);
        crackIndices.Add(vertOfs + i * 2 + 1);
        crackIndices.Add(vertOfs + i * 2 + 2);

        crackVertices.Add(MakeVertex(j - i * 2, j));
        crackVertices.Add(MakeVertex(j - i * 2 - 1, j + debugOffset));
    }
    // left line
    j = CrackTrianglesCount * 2;
    vertOfs = crackVertices.Size();
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Add(vertOfs + i * 2);
        crackIndices.Add(vertOfs + i * 2);
        crackIndices.Add(vertOfs + i * 2 + 1);
        crackIndices.Add(vertOfs + i * 2 + 2);

        crackVertices.Add(MakeVertex(0, j - i * 2));
        crackVertices.Add(MakeVertex(-debugOffset, j - i * 2 - 1));
    }
    crackVertices.Add(MakeVertex(0, 0));

    // Reverse face culling
    crackVertices.Reverse();
    crackIndices.Reverse();

    // Create single vertex and index buffer

    m_VertexBuffer.Resize(blockVerts.Size() +
                          horGapVertices.Size() +
                          vertGapVertices.Size() +
                          interiorTLVertices.Size() +
                          interiorTRVertices.Size() +
                          interiorBLVertices.Size() +
                          interiorBRVertices.Size() +
                          interiorFinestVertices.Size() +
                          crackVertices.Size());

    m_IndexBuffer.Resize(blockIndices.Size() +
                         horGapIndices.Size() +
                         vertGapIndices.Size() +
                         interiorTLIndices.Size() +
                         interiorTRIndices.Size() +
                         interiorBLIndices.Size() +
                         interiorBRIndices.Size() +
                         interiorFinestIndices.Size() +
                         crackIndices.Size());

    int firstVert = 0;
    int firstIndex = 0;

    auto AddPatch = [&](TerrainPatch& Patch, TVector<TerrainVertex> const& VB, TVector<unsigned short> const& IB)
    {
        Patch.BaseVertex = firstVert;
        Patch.StartIndex = firstIndex;
        Patch.IndexCount = IB.Size();
        Platform::Memcpy(m_VertexBuffer.ToPtr() + firstVert, VB.ToPtr(), VB.Size() * sizeof(TerrainVertex));
        firstVert += VB.Size();
        Platform::Memcpy(m_IndexBuffer.ToPtr() + firstIndex, IB.ToPtr(), IB.Size() * sizeof(unsigned short));
        firstIndex += IB.Size();
    };

    AddPatch(m_BlockPatch, blockVerts, blockIndices);
    AddPatch(m_HorGapPatch, horGapVertices, horGapIndices);
    AddPatch(m_VertGapPatch, vertGapVertices, vertGapIndices);
    AddPatch(m_InteriorTLPatch, interiorTLVertices, interiorTLIndices);
    AddPatch(m_InteriorTRPatch, interiorTRVertices, interiorTRIndices);
    AddPatch(m_InteriorBLPatch, interiorBLVertices, interiorBLIndices);
    AddPatch(m_InteriorBRPatch, interiorBRVertices, interiorBRIndices);
    AddPatch(m_InteriorFinestPatch, interiorFinestVertices, interiorFinestIndices);
    AddPatch(m_CrackPatch, crackVertices, crackIndices);

    HK_ASSERT(firstVert == m_VertexBuffer.Size());
    HK_ASSERT(firstIndex == m_IndexBuffer.Size());

    RenderCore::IDevice* device = GEngine->GetRenderDevice();

    RenderCore::BufferDesc ci = {};
    ci.bImmutableStorage = true;

    ci.SizeInBytes = m_VertexBuffer.Size() * sizeof(TerrainVertex);
    device->CreateBuffer(ci, m_VertexBuffer.ToPtr(), &m_VertexBufferGPU);

    ci.SizeInBytes = m_IndexBuffer.Size() * sizeof(unsigned short);
    device->CreateBuffer(ci, m_IndexBuffer.ToPtr(), &m_IndexBufferGPU);

    LOG("Terrain Mesh: Total vertices {}, Total indices {}\n", m_VertexBuffer.Size(), m_IndexBuffer.Size());
}

HK_NAMESPACE_END
