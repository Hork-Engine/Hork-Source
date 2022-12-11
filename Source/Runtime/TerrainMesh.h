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

#pragma once

#include <Renderer/RenderDefs.h>

struct TerrainPatch
{
    int IndexCount;
    int BaseVertex;
    int StartIndex;
};

class TerrainMesh : public RefCounted
{
public:
    TerrainMesh(int TextureSize);

    int GetTextureSize() const
    {
        return TextureSize;
    }

    /** Get vertex buffer in GPU */
    RenderCore::IBuffer* GetVertexBufferGPU() const { return VertexBufferGPU; }
    /** Get index buffer in GPU */
    RenderCore::IBuffer* GetIndexBufferGPU() const { return IndexBufferGPU; }

    /** Get vertex buffer in CPU. We keep it only for debug draw */
    TerrainVertex const* GetVertexBufferCPU() const { return VertexBuffer.ToPtr(); }
    /** Get index buffer in CPU. We keep it only for debug draw */
    unsigned short const* GetIndexBufferCPU() const { return IndexBuffer.ToPtr(); }

    int GetVertexCount() const { return VertexBuffer.Size(); }
    int GetIndexCount() const { return IndexBuffer.Size(); }

    TerrainPatch const& GetBlockPatch() const { return BlockPatch; }
    TerrainPatch const& GetHorGapPatch() const { return HorGapPatch; }
    TerrainPatch const& GetVertGapPatch() const { return VertGapPatch; }
    TerrainPatch const& GetInteriorTLPatch() const { return InteriorTLPatch; }
    TerrainPatch const& GetInteriorTRPatch() const { return InteriorTRPatch; }
    TerrainPatch const& GetInteriorBLPatch() const { return InteriorBLPatch; }
    TerrainPatch const& GetInteriorBRPatch() const { return InteriorBRPatch; }
    TerrainPatch const& GetInteriorFinestPatch() const { return InteriorFinestPatch; }
    TerrainPatch const& GetCrackPatch() const { return CrackPatch; }

private:
    /** Initial texture size */
    int TextureSize;

    TerrainPatch BlockPatch;
    TerrainPatch HorGapPatch;
    TerrainPatch VertGapPatch;
    TerrainPatch InteriorTLPatch;
    TerrainPatch InteriorTRPatch;
    TerrainPatch InteriorBLPatch;
    TerrainPatch InteriorBRPatch;
    TerrainPatch InteriorFinestPatch;
    TerrainPatch CrackPatch;

    /** Vertex buffer in GPU */
    TRef<RenderCore::IBuffer> VertexBufferGPU;
    /** Index buffer in GPU */
    TRef<RenderCore::IBuffer> IndexBufferGPU;

    /** Vertex buffer in CPU. We keep it only for debug draw */
    TVector<TerrainVertex> VertexBuffer;
    /** Index buffer in CPU. We keep it only for debug draw */
    TVector<unsigned short> IndexBuffer;
};
