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

#include <Hork/RenderDefs/RenderDefs.h>

HK_NAMESPACE_BEGIN

struct TerrainPatch
{
    int IndexCount;
    int BaseVertex;
    int StartIndex;
};

class TerrainMesh final : public Noncopyable
{
public:
    TerrainMesh();

    /// Get vertex buffer in GPU
    RHI::IBuffer* GetVertexBufferGPU() const { return m_VertexBufferGPU; }
    /// Get index buffer in GPU
    RHI::IBuffer* GetIndexBufferGPU() const { return m_IndexBufferGPU; }

    /// Get vertex buffer in CPU. We keep it only for debug draw
    TerrainVertex const* GetVertexBufferCPU() const { return m_VertexBuffer.ToPtr(); }
    /// Get index buffer in CPU. We keep it only for debug draw
    unsigned short const* GetIndexBufferCPU() const { return m_IndexBuffer.ToPtr(); }

    int GetVertexCount() const { return m_VertexBuffer.Size(); }
    int GetIndexCount() const { return m_IndexBuffer.Size(); }

    TerrainPatch const& GetBlockPatch() const { return m_BlockPatch; }
    TerrainPatch const& GetHorGapPatch() const { return m_HorGapPatch; }
    TerrainPatch const& GetVertGapPatch() const { return m_VertGapPatch; }
    TerrainPatch const& GetInteriorTLPatch() const { return m_InteriorTLPatch; }
    TerrainPatch const& GetInteriorTRPatch() const { return m_InteriorTRPatch; }
    TerrainPatch const& GetInteriorBLPatch() const { return m_InteriorBLPatch; }
    TerrainPatch const& GetInteriorBRPatch() const { return m_InteriorBRPatch; }
    TerrainPatch const& GetInteriorFinestPatch() const { return m_InteriorFinestPatch; }
    TerrainPatch const& GetCrackPatch() const { return m_CrackPatch; }

private:
    TerrainPatch m_BlockPatch;
    TerrainPatch m_HorGapPatch;
    TerrainPatch m_VertGapPatch;
    TerrainPatch m_InteriorTLPatch;
    TerrainPatch m_InteriorTRPatch;
    TerrainPatch m_InteriorBLPatch;
    TerrainPatch m_InteriorBRPatch;
    TerrainPatch m_InteriorFinestPatch;
    TerrainPatch m_CrackPatch;

    /// Vertex buffer in GPU
    Ref<RHI::IBuffer> m_VertexBufferGPU;
    /// Index buffer in GPU
    Ref<RHI::IBuffer> m_IndexBufferGPU;

    /// Vertex buffer in CPU. We keep it only for debug draw
    Vector<TerrainVertex> m_VertexBuffer;
    /// Index buffer in CPU. We keep it only for debug draw
    Vector<unsigned short> m_IndexBuffer;
};

HK_NAMESPACE_END
