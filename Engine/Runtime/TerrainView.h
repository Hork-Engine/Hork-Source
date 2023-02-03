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

#pragma once

#include "Terrain.h"
#include "TerrainMesh.h"

#include <Engine/Renderer/RenderDefs.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

struct TerrainLodInfo
{
    /** Grid offset in global grid space */
    Int2 Offset;
    /** Texture offset in global grid space */
    Int2 TextureOffset;
    /** Previous texture offset in global grid space */
    Int2 PrevTextureOffset;
    /** Grid step */
    int GridScale;
    /** Interior trim type */
    int InteriorTrim;
    /** Current lod index */
    int LodIndex;
    /** Fource update flag */
    bool bForceUpdateTexture : 1;
    /** Elevation minimum height */
    float MinH;
    /** Elevation maximum height */
    float MaxH;
    /** Lod elevation data */
    Float2* HeightMap;
    /** Lod normal map data */
    byte* NormalMap;
};

enum
{
    MAX_TERRAIN_LODS = 10
};

class TerrainView : public RefCounted
{
public:
    TerrainView(int TextureSize);
    virtual ~TerrainView();

    void SetTerrain(Terrain* Mesh);

    void Update(class StreamedMemoryGPU* StreamedMemory, class TerrainMesh* TerrainMesh, Float3 const& ViewPosition, BvFrustum const& ViewFrustum);

    int GetTextureSize() const
    {
        return m_TextureSize;
    }

    size_t GetInstanceBufferStreamHandle() const
    {
        return m_InstanceBufferStreamHandle;
    }

    size_t GetIndirectBufferStreamHandle() const
    {
        return m_IndirectBufferStreamHandle;
    }

    int GetIndirectBufferDrawCount() const
    {
        return m_IndirectBuffer.Size();
    }

    RenderCore::ITexture* GetClipmapArray() const
    {
        return m_ClipmapArray;
    }

    RenderCore::ITexture* GetNormalMapArray() const
    {
        return m_NormalMapArray;
    }

    float GetViewHeight() const
    {
        return m_ViewHeight;
    }

    void DrawDebug(DebugRenderer* InRenderer, TerrainMesh* TerrainMesh);

private:
    void MakeView(TerrainMesh* TerrainMesh, Float3 const& ViewPosition, BvFrustum const& ViewFrustum);
    void AddPatches(TerrainMesh* TerrainMesh, BvFrustum const& ViewFrustum);
    void AddBlock(TerrainLodInfo const& Lod, Int2 const& Offset);
    void AddGapV(TerrainLodInfo const& Lod, Int2 const& Offset);
    void AddGapH(TerrainLodInfo const& Lod, Int2 const& Offset);
    void AddInteriorTopLeft(TerrainLodInfo const& Lod);
    void AddInteriorTopRight(TerrainLodInfo const& Lod);
    void AddInteriorBottomLeft(TerrainLodInfo const& Lod);
    void AddInteriorBottomRight(TerrainLodInfo const& Lod);
    void AddCrackLines(TerrainLodInfo const& Lod);
    bool CullBlock(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod, Int2 const& Offset);
    bool CullGapV(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod, Int2 const& Offset);
    bool CullGapH(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod, Int2 const& Offset);
    bool CullInteriorTrim(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod);

    void UpdateTextures();
    void UpdateRect(TerrainLodInfo const& Lod, TerrainLodInfo const& CoarserLod, int MinX, int MaxX, int MinY, int MaxY);

    TerrainPatchInstance& AddInstance()
    {
        return m_InstanceBuffer.Add();
    }

    void AddPatchInstances(TerrainPatch const& Patch, int InstanceCount)
    {
        if (InstanceCount > 0)
        {
            RenderCore::DrawIndexedIndirectCmd& blocks = m_IndirectBuffer.Add();
            blocks.IndexCountPerInstance = Patch.IndexCount;
            blocks.InstanceCount = InstanceCount;
            blocks.StartIndexLocation = Patch.StartIndex;
            blocks.BaseVertexLocation = Patch.BaseVertex;
            blocks.StartInstanceLocation = m_StartInstanceLocation;

            m_StartInstanceLocation += InstanceCount;
        }
    }

    enum
    {
        INTERIOR_TOP_LEFT,
        INTERIOR_TOP_RIGHT,
        INTERIOR_BOTTOM_LEFT,
        INTERIOR_BOTTOM_RIGHT
    };

    const int m_TextureSize;
    const int m_TextureWrapMask;
    const int m_GapWidth;
    const int m_BlockWidth;
    const int m_LodGridSize;
    const int m_HalfGridSize;

    TRef<Terrain> m_Terrain;

    /** Current lod state */
    TerrainLodInfo m_LodInfo[MAX_TERRAIN_LODS];

    /** Min viewable lod */
    int m_MinViewLod;
    /** Max viewable lod */
    int m_MaxViewLod;
    /** Height above the terrain */
    float m_ViewHeight;

    TPodVector<TerrainPatchInstance> m_InstanceBuffer;
    TPodVector<RenderCore::DrawIndexedIndirectCmd> m_IndirectBuffer;

    TRef<RenderCore::ITexture> m_ClipmapArray;
    TRef<RenderCore::ITexture> m_NormalMapArray;

    size_t m_InstanceBufferStreamHandle;
    size_t m_IndirectBufferStreamHandle;

    int m_StartInstanceLocation;

    // Debug draw
    void DrawIndexedTriStrip(TerrainVertex const* Vertices, unsigned short const* Indices, int IndexCount);
    void DrawTerrainTriangle(TerrainVertex const& a, TerrainVertex const& b, TerrainVertex const& c);
    Float3 VertexShader(TerrainVertex const& v);
    DebugRenderer* m_TerrainRenderer;
    TerrainPatchInstance const* m_pDrawCallUniformData;
    TPodVector<BvAxisAlignedBox> m_BoundingBoxes;
};

HK_NAMESPACE_END
