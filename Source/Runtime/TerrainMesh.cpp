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

static const unsigned short RESET_INDEX = 0xffff;

static void CreateTriangleStripPatch(int NumQuadsX, int NumQuadsY, TPodVector<STerrainVertex>& Vertices, TPodVector<unsigned short>& Indices)
{
    int numVertsX = NumQuadsX + 1;
    int numVertsY = NumQuadsY + 1;

    Vertices.Resize(numVertsX * numVertsY);

    for (int y = 0; y < NumQuadsY; y++)
    {
        for (int x = 0; x < NumQuadsX + 1; x++)
        {
            Indices.Append(x + (y)*numVertsX);
            Indices.Append(x + (y + 1) * numVertsX);
        }
        Indices.Append(RESET_INDEX);
    }

    STerrainVertex* vert = Vertices.ToPtr();

    for (int i = 0; i < numVertsY; i++)
    {
        for (int j = 0; j < numVertsX; j++)
        {
            vert[i * numVertsX + j].X = j;
            vert[i * numVertsX + j].Y = i;
        }
    }
}

HK_FORCEINLINE STerrainVertex MakeVertex(short X, short Y)
{
    STerrainVertex v;
    v.X = X;
    v.Y = Y;
    return v;
}

ATerrainMesh::ATerrainMesh(int InTextureSize)
{
    TPodVector<STerrainVertex> blockVerts;
    TPodVector<unsigned short> blockIndices;
    TPodVector<STerrainVertex> horGapVertices;
    TPodVector<unsigned short> horGapIndices;
    TPodVector<STerrainVertex> vertGapVertices;
    TPodVector<unsigned short> vertGapIndices;
    TPodVector<STerrainVertex> interiorTLVertices;
    TPodVector<STerrainVertex> interiorTRVertices;
    TPodVector<STerrainVertex> interiorBLVertices;
    TPodVector<STerrainVertex> interiorBRVertices;
    TPodVector<STerrainVertex> interiorFinestVertices;
    TPodVector<unsigned short> interiorTLIndices;
    TPodVector<unsigned short> interiorTRIndices;
    TPodVector<unsigned short> interiorBLIndices;
    TPodVector<unsigned short> interiorBRIndices;
    TPodVector<unsigned short> interiorFinestIndices;
    TPodVector<STerrainVertex> crackVertices;
    TPodVector<unsigned short> crackIndices;

    HK_ASSERT(IsPowerOfTwo(InTextureSize));

    const int BlockWidth          = InTextureSize / 4 - 1;
    const int GapWidth            = 2;
    const int CrackTrianglesCount = (BlockWidth * 4 + GapWidth) / 2;

    TextureSize = InTextureSize;

    // Blocks
    CreateTriangleStripPatch(BlockWidth, BlockWidth, blockVerts, blockIndices);

    // Gaps
    CreateTriangleStripPatch(BlockWidth, GapWidth, horGapVertices, horGapIndices);
    CreateTriangleStripPatch(GapWidth, BlockWidth, vertGapVertices, vertGapIndices);

    // Interior L-shapes
    int i = 0;
    for (int q = 0; q < (BlockWidth * 2 + GapWidth) + 1; q++)
    {
        interiorTLVertices.Append(MakeVertex(q, 0));
        interiorTLVertices.Append(MakeVertex(q, 1));

        interiorTRVertices.Append(MakeVertex(q, 0));
        interiorTRVertices.Append(MakeVertex(q, 1));

        interiorBLVertices.Append(MakeVertex(q, BlockWidth * 2 + GapWidth - 1));
        interiorBLVertices.Append(MakeVertex(q, BlockWidth * 2 + GapWidth));

        interiorBRVertices.Append(MakeVertex(q, BlockWidth * 2 + GapWidth - 1));
        interiorBRVertices.Append(MakeVertex(q, BlockWidth * 2 + GapWidth));

        interiorTLIndices.Append(i);
        interiorTLIndices.Append(i + 1);

        interiorTRIndices.Append(i);
        interiorTRIndices.Append(i + 1);

        interiorBLIndices.Append(i);
        interiorBLIndices.Append(i + 1);

        interiorBRIndices.Append(i);
        interiorBRIndices.Append(i + 1);

        i += 2;
    }

    interiorTLIndices.Append(RESET_INDEX);
    interiorTRIndices.Append(RESET_INDEX);
    interiorBLIndices.Append(RESET_INDEX);
    interiorBRIndices.Append(RESET_INDEX);

    int prev_a_tl = 1;
    int prev_b_tl = prev_a_tl + 2;

    int prev_a_tr = ((BlockWidth * 2 + GapWidth) + 1) * 2 - 3;
    int prev_b_tr = prev_a_tr + 2;

    int q;
    for (q = 0; q < (BlockWidth * 2 + GapWidth) - 1; q++)
    {
        interiorTLIndices.Append(prev_a_tl);
        interiorTLIndices.Append(i);
        interiorTLIndices.Append(prev_b_tl);
        interiorTLIndices.Append(i + 1);
        prev_a_tl = i;
        prev_b_tl = i + 1;

        interiorTRIndices.Append(prev_a_tr);
        interiorTRIndices.Append(i);
        interiorTRIndices.Append(prev_b_tr);
        interiorTRIndices.Append(i + 1);
        prev_a_tr = i;
        prev_b_tr = i + 1;

        if (q < (BlockWidth * 2 + GapWidth) - 2)
        {
            interiorTLIndices.Append(RESET_INDEX);
            interiorTRIndices.Append(RESET_INDEX);

            interiorBLIndices.Append(i);
            interiorBLIndices.Append(i + 2);
            interiorBLIndices.Append(i + 1);
            interiorBLIndices.Append(i + 3);
            interiorBLIndices.Append(RESET_INDEX);

            interiorBRIndices.Append(i);
            interiorBRIndices.Append(i + 2);
            interiorBRIndices.Append(i + 1);
            interiorBRIndices.Append(i + 3);
            interiorBRIndices.Append(RESET_INDEX);

            i += 2;
        }

        interiorTLVertices.Append(MakeVertex(0, q + 2));
        interiorTLVertices.Append(MakeVertex(1, q + 2));

        interiorTRVertices.Append(MakeVertex((BlockWidth * 2 + GapWidth) - 1, q + 2));
        interiorTRVertices.Append(MakeVertex((BlockWidth * 2 + GapWidth), q + 2));

        interiorBLVertices.Append(MakeVertex(0, q));
        interiorBLVertices.Append(MakeVertex(1, q));

        interiorBRVertices.Append(MakeVertex((BlockWidth * 2 + GapWidth) - 1, q));
        interiorBRVertices.Append(MakeVertex((BlockWidth * 2 + GapWidth), q));
    }

    interiorBLIndices.Append(i);
    interiorBLIndices.Append(0);
    interiorBLIndices.Append(i + 1);
    interiorBLIndices.Append(2);

    interiorBRIndices.Append(i);
    interiorBRIndices.Append(((BlockWidth * 2 + GapWidth) + 1) * 2 - 4);
    interiorBRIndices.Append(i + 1);
    interiorBRIndices.Append(((BlockWidth * 2 + GapWidth) + 1) * 2 - 2);

    for (STerrainVertex& v : interiorTLVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for (STerrainVertex& v : interiorTRVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for (STerrainVertex& v : interiorBLVertices)
    {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for (STerrainVertex& v : interiorBRVertices)
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
        interiorFinestIndices.Append(i++);
        interiorFinestIndices.Append(i++);

        interiorFinestVertices.Append(MakeVertex(x, y));
        interiorFinestVertices.Append(MakeVertex(x, y + 1));
    }
    interiorFinestIndices.Append(RESET_INDEX);

    x = BlockWidth * 2;
    for (y = 0; y < BlockWidth * 2; y++)
    {
        interiorFinestIndices.Append(i);
        interiorFinestIndices.Append(i + 2);
        interiorFinestIndices.Append(i + 1);
        interiorFinestIndices.Append(i + 3);
        interiorFinestIndices.Append(RESET_INDEX);

        interiorFinestVertices.Append(MakeVertex(x, y));
        interiorFinestVertices.Append(MakeVertex(x + 1, y));

        i += 2;
    }

    interiorFinestVertices.Append(MakeVertex(x, y));
    interiorFinestVertices.Append(MakeVertex(x + 1, y));

    int debugOffset = 0; //-1;

    // top line
    int j = 0;
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Append(i * 2);
        crackIndices.Append(i * 2);
        crackIndices.Append(i * 2 + 1);
        crackIndices.Append(i * 2 + 2);

        crackVertices.Append(MakeVertex(i * 2, j));
        crackVertices.Append(MakeVertex(i * 2 + 1, j - debugOffset));
    }
    // right line
    j           = CrackTrianglesCount * 2;
    int vertOfs = crackVertices.Size();
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Append(vertOfs + i * 2);
        crackIndices.Append(vertOfs + i * 2);
        crackIndices.Append(vertOfs + i * 2 + 1);
        crackIndices.Append(vertOfs + i * 2 + 2);

        crackVertices.Append(MakeVertex(j, i * 2));
        crackVertices.Append(MakeVertex(j + debugOffset, i * 2 + 1));
    }
    // bottom line
    j       = CrackTrianglesCount * 2;
    vertOfs = crackVertices.Size();
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Append(vertOfs + i * 2);
        crackIndices.Append(vertOfs + i * 2);
        crackIndices.Append(vertOfs + i * 2 + 1);
        crackIndices.Append(vertOfs + i * 2 + 2);

        crackVertices.Append(MakeVertex(j - i * 2, j));
        crackVertices.Append(MakeVertex(j - i * 2 - 1, j + debugOffset));
    }
    // left line
    j       = CrackTrianglesCount * 2;
    vertOfs = crackVertices.Size();
    for (i = 0; i < CrackTrianglesCount; i++)
    {
        crackIndices.Append(vertOfs + i * 2);
        crackIndices.Append(vertOfs + i * 2);
        crackIndices.Append(vertOfs + i * 2 + 1);
        crackIndices.Append(vertOfs + i * 2 + 2);

        crackVertices.Append(MakeVertex(0, j - i * 2));
        crackVertices.Append(MakeVertex(-debugOffset, j - i * 2 - 1));
    }
    crackVertices.Append(MakeVertex(0, 0));

    // Reverse face culling
    crackVertices.Reverse();
    crackIndices.Reverse();

    // Create single vertex and index buffer

    VertexBuffer.Resize(blockVerts.Size() +
                        horGapVertices.Size() +
                        vertGapVertices.Size() +
                        interiorTLVertices.Size() +
                        interiorTRVertices.Size() +
                        interiorBLVertices.Size() +
                        interiorBRVertices.Size() +
                        interiorFinestVertices.Size() +
                        crackVertices.Size());

    IndexBuffer.Resize(blockIndices.Size() +
                       horGapIndices.Size() +
                       vertGapIndices.Size() +
                       interiorTLIndices.Size() +
                       interiorTRIndices.Size() +
                       interiorBLIndices.Size() +
                       interiorBRIndices.Size() +
                       interiorFinestIndices.Size() +
                       crackIndices.Size());

    int firstVert  = 0;
    int firstIndex = 0;

    auto AddPatch = [&](STerrainPatch& Patch, TPodVector<STerrainVertex> const& VB, TPodVector<unsigned short> const& IB)
    {
        Patch.BaseVertex = firstVert;
        Patch.StartIndex = firstIndex;
        Patch.IndexCount = IB.Size();
        Platform::Memcpy(VertexBuffer.ToPtr() + firstVert, VB.ToPtr(), VB.Size() * sizeof(STerrainVertex));
        firstVert += VB.Size();
        Platform::Memcpy(IndexBuffer.ToPtr() + firstIndex, IB.ToPtr(), IB.Size() * sizeof(unsigned short));
        firstIndex += IB.Size();
    };

    AddPatch(BlockPatch, blockVerts, blockIndices);
    AddPatch(HorGapPatch, horGapVertices, horGapIndices);
    AddPatch(VertGapPatch, vertGapVertices, vertGapIndices);
    AddPatch(InteriorTLPatch, interiorTLVertices, interiorTLIndices);
    AddPatch(InteriorTRPatch, interiorTRVertices, interiorTRIndices);
    AddPatch(InteriorBLPatch, interiorBLVertices, interiorBLIndices);
    AddPatch(InteriorBRPatch, interiorBRVertices, interiorBRIndices);
    AddPatch(InteriorFinestPatch, interiorFinestVertices, interiorFinestIndices);
    AddPatch(CrackPatch, crackVertices, crackIndices);

    HK_ASSERT(firstVert == VertexBuffer.Size());
    HK_ASSERT(firstIndex == IndexBuffer.Size());

    RenderCore::IDevice* device = GEngine->GetRenderDevice();

    RenderCore::SBufferDesc ci = {};
    ci.bImmutableStorage       = true;

    ci.SizeInBytes = VertexBuffer.Size() * sizeof(STerrainVertex);
    device->CreateBuffer(ci, VertexBuffer.ToPtr(), &VertexBufferGPU);

    ci.SizeInBytes = IndexBuffer.Size() * sizeof(unsigned short);
    device->CreateBuffer(ci, IndexBuffer.ToPtr(), &IndexBufferGPU);

    LOG("Terrain Mesh: Total vertices {}, Total indices {}\n", VertexBuffer.Size(), IndexBuffer.Size());
}
