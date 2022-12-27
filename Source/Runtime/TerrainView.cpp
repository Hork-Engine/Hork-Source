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

#include "TerrainView.h"
#include "DebugRenderer.h"
#include "Engine.h"

#include <Core/ConsoleVar.h>
#include <Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

static const unsigned short RESET_INDEX = 0xffff;

ConsoleVar com_TerrainMinLod("com_TerrainMinLod"s, "0"s);
ConsoleVar com_TerrainMaxLod("com_TerrainMaxLod"s, "5"s);
ConsoleVar com_ShowTerrainMemoryUsage("com_ShowTerrainMemoryUsage"s, "0"s);

TerrainView::TerrainView(int InTextureSize) :
    TextureSize(InTextureSize), TextureWrapMask(InTextureSize - 1), GapWidth(2), BlockWidth(TextureSize / 4 - 1), LodGridSize(TextureSize - 2), HalfGridSize(LodGridSize >> 1), ViewHeight(0.0f)
{
    for (int i = 0; i < MAX_TERRAIN_LODS; i++)
    {
        LodInfo[i].HeightMap = (Float2*)Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(TextureSize * TextureSize * sizeof(Float2));
        LodInfo[i].NormalMap = (byte*)Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(TextureSize * TextureSize * 4);
        LodInfo[i].LodIndex  = i;

        LodInfo[i].TextureOffset.X = 0;
        LodInfo[i].TextureOffset.Y = 0;

        LodInfo[i].PrevTextureOffset.X = 0;
        LodInfo[i].PrevTextureOffset.Y = 0;

        LodInfo[i].bForceUpdateTexture = true;
    }

    MinViewLod = MaxViewLod = 0;

    auto textureFormat = RenderCore::TextureDesc()
                             .SetFormat(TEXTURE_FORMAT_RG32_FLOAT)
                             .SetResolution(RenderCore::TextureResolution2DArray(TextureSize,
                                                                                  TextureSize,
                                                                                  MAX_TERRAIN_LODS))
                             .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);
    GEngine->GetRenderDevice()->CreateTexture(textureFormat, &ClipmapArray);
    ClipmapArray->SetDebugName("Terrain Clipmap Array");

    auto normalMapFormat = RenderCore::TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_BGRA8_UNORM)
                               .SetResolution(RenderCore::TextureResolution2DArray(TextureSize,
                                                                                    TextureSize,
                                                                                    MAX_TERRAIN_LODS))
                               .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);
    GEngine->GetRenderDevice()->CreateTexture(normalMapFormat, &NormalMapArray);
    NormalMapArray->SetDebugName("Terrain Normal Map Array");
}

TerrainView::~TerrainView()
{
    for (int i = 0; i < MAX_TERRAIN_LODS; i++)
    {
        Platform::GetHeapAllocator<HEAP_IMAGE>().Free(LodInfo[i].HeightMap);
        Platform::GetHeapAllocator<HEAP_IMAGE>().Free(LodInfo[i].NormalMap);
    }
}

void TerrainView::SetTerrain(Terrain* terrain)
{
    if (m_Terrain == terrain)
    {
        return;
    }

    m_Terrain = terrain;
    for (int i = 0; i < MAX_TERRAIN_LODS; i++)
    {
        LodInfo[i].bForceUpdateTexture = true;
    }
}

void TerrainView::Update(StreamedMemoryGPU* StreamedMemory, TerrainMesh* TerrainMesh, Float3 const& ViewPosition, BvFrustum const& ViewFrustum)
{
    HK_ASSERT(TerrainMesh->GetTextureSize() == TextureSize);

    BoundingBoxes.Clear();

    IndirectBuffer.Clear();
    InstanceBuffer.Clear();

    StartInstanceLocation = 0;

    if (!ViewFrustum.IsBoxVisible(m_Terrain->GetBoundingBox()))
    {
        return;
    }

    MakeView(TerrainMesh, ViewPosition, ViewFrustum);

    InstanceBufferStreamHandle = StreamedMemory->AllocateVertex(InstanceBuffer.Size() * sizeof(TerrainPatchInstance),
                                                                InstanceBuffer.ToPtr());

    IndirectBufferStreamHandle = StreamedMemory->AllocateWithCustomAlignment(IndirectBuffer.Size() * sizeof(RenderCore::DrawIndexedIndirectCmd),
                                                                             16, // FIXME: is this alignment correct for DrawIndirect commands?
                                                                             IndirectBuffer.ToPtr());

    if (com_ShowTerrainMemoryUsage)
    {
        LOG("Instance buffer size in bytes {}\n", InstanceBuffer.Size() * sizeof(TerrainPatchInstance));
        LOG("Indirect buffer size in bytes {}\n", IndirectBuffer.Size() * sizeof(RenderCore::DrawIndexedIndirectCmd));
    }
}

static HK_FORCEINLINE Int2 GetTexcoordOffset(TerrainLodInfo const& Lod)
{
    Int2 TexcoordOffset;
    TexcoordOffset.X = Lod.TextureOffset.X * Lod.GridScale - Lod.Offset.X;
    TexcoordOffset.Y = Lod.TextureOffset.Y * Lod.GridScale - Lod.Offset.Y;
    return TexcoordOffset;
}

bool TerrainView::CullBlock(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod, Int2 const& Offset)
{
    int blockSize = BlockWidth * Lod.GridScale;
    int minX      = Offset.X * Lod.GridScale + Lod.Offset.X;
    int minZ      = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    int maxX      = minX + blockSize;
    int maxZ      = minZ + blockSize;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if (!BvBoxOverlapBox(m_Terrain->GetBoundingBox(), box))
    {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible(box);
}

bool TerrainView::CullGapV(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod, Int2 const& Offset)
{
    int blockSize = BlockWidth * Lod.GridScale;
    int minX      = Offset.X * Lod.GridScale + Lod.Offset.X;
    int minZ      = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    int maxX      = minX + 2 * Lod.GridScale;
    int maxZ      = minZ + blockSize;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if (!BvBoxOverlapBox(m_Terrain->GetBoundingBox(), box))
    {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible(box);
}

bool TerrainView::CullGapH(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod, Int2 const& Offset)
{
    int blockSize = BlockWidth * Lod.GridScale;
    int minX      = Offset.X * Lod.GridScale + Lod.Offset.X;
    int minZ      = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    int maxX      = minX + blockSize;
    int maxZ      = minZ + 2 * Lod.GridScale;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if (!BvBoxOverlapBox(m_Terrain->GetBoundingBox(), box))
    {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible(box);
}

bool TerrainView::CullInteriorTrim(BvFrustum const& ViewFrustum, TerrainLodInfo const& Lod)
{
    int blockSize    = BlockWidth * Lod.GridScale;
    int interiorSize = (BlockWidth * 2 + GapWidth) * Lod.GridScale;

    int minX = blockSize + Lod.Offset.X;
    int minZ = blockSize + Lod.Offset.Y;
    int maxX = minX + interiorSize;
    int maxZ = minZ + interiorSize;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if (!BvBoxOverlapBox(m_Terrain->GetBoundingBox(), box))
    {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible(box);
}

void TerrainView::AddBlock(TerrainLodInfo const& Lod, Int2 const& Offset)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale       = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate.X = Offset.X * Lod.GridScale + Lod.Offset.X;
    instance.VertexTranslate.Y = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    instance.TexcoordOffset    = GetTexcoordOffset(Lod);
    instance.QuadColor         = Color4(0.5f, 0.5f, 0.5f, 1.0f);
}

void TerrainView::AddGapV(TerrainLodInfo const& Lod, Int2 const& Offset)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale       = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate.X = Offset.X * Lod.GridScale + Lod.Offset.X;
    instance.VertexTranslate.Y = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    instance.TexcoordOffset    = GetTexcoordOffset(Lod);
    instance.QuadColor         = Color4(0.2f, 0.7f, 0.2f, 1.0f);
}

void TerrainView::AddGapH(TerrainLodInfo const& Lod, Int2 const& Offset)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale       = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate.X = Offset.X * Lod.GridScale + Lod.Offset.X;
    instance.VertexTranslate.Y = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    instance.TexcoordOffset    = GetTexcoordOffset(Lod);
    instance.QuadColor         = Color4(0.2f, 0.7f, 0.2f, 1.0f);
}

void TerrainView::AddInteriorTopLeft(TerrainLodInfo const& Lod)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale     = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset  = GetTexcoordOffset(Lod);
    instance.QuadColor       = Color4(0.5f, 0.5f, 1.0f, 1.0f);
}

void TerrainView::AddInteriorTopRight(TerrainLodInfo const& Lod)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale     = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset  = GetTexcoordOffset(Lod);
    instance.QuadColor       = Color4(0.5f, 0.5f, 1.0f, 1.0f);
}

void TerrainView::AddInteriorBottomLeft(TerrainLodInfo const& Lod)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale     = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset  = GetTexcoordOffset(Lod);
    instance.QuadColor       = Color4(0.5f, 0.5f, 1.0f, 1.0f);
}

void TerrainView::AddInteriorBottomRight(TerrainLodInfo const& Lod)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale     = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset  = GetTexcoordOffset(Lod);
    instance.QuadColor       = Color4(0.5f, 0.5f, 1.0f, 1.0f);
}

void TerrainView::AddCrackLines(TerrainLodInfo const& Lod)
{
    TerrainPatchInstance& instance = AddInstance();

    instance.VertexScale     = Int2(Lod.GridScale, Lod.LodIndex);
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset  = GetTexcoordOffset(Lod);
    instance.QuadColor       = Color4(0, 1, 0, 1.0f);
}

void TerrainView::MakeView(TerrainMesh* TerrainMesh, Float3 const& ViewPosition, BvFrustum const& ViewFrustum)
{
    int minLod = Math::Max(com_TerrainMinLod.GetInteger(), 0);
    int maxLod = Math::Min(com_TerrainMaxLod.GetInteger(), MAX_TERRAIN_LODS - 1);

    float terrainH = m_Terrain->ReadHeight(ViewPosition.X, ViewPosition.Z, 0);

    // height above the terrain
    ViewHeight = Math::Max(ViewPosition.Y - terrainH, 0.0f);

    for (int lod = minLod; lod <= maxLod; lod++)
    {
        TerrainLodInfo& lodInfo = LodInfo[lod];

        int gridScale  = 1 << lod;
        int snapSize   = gridScale * 2;
        int gridExtent = gridScale * LodGridSize;

        Int2 snapPos;
        snapPos.X = (Math::Floor(ViewPosition.X / snapSize) + 0.5f) * snapSize;
        snapPos.Y = (Math::Floor(ViewPosition.Z / snapSize) + 0.5f) * snapSize;

        Float2 snapOffset;
        snapOffset.X = ViewPosition.X - snapPos.X;
        snapOffset.Y = ViewPosition.Z - snapPos.Y;

        lodInfo.Offset.X        = snapPos.X - HalfGridSize * gridScale;
        lodInfo.Offset.Y        = snapPos.Y - HalfGridSize * gridScale;
        lodInfo.TextureOffset.X = snapPos.X / gridScale;
        lodInfo.TextureOffset.Y = snapPos.Y / gridScale;
        lodInfo.GridScale       = gridScale;

        lodInfo.InteriorTrim = snapOffset.X > 0.0f ?
            (snapOffset.Y > 0.0f ? INTERIOR_TOP_LEFT : INTERIOR_BOTTOM_LEFT) :
            (snapOffset.Y > 0.0f ? INTERIOR_TOP_RIGHT : INTERIOR_BOTTOM_RIGHT);

        if (minLod < maxLod && gridExtent < ViewHeight * 2.5f)
        {
            minLod++;
            //lodInfo.bForceUpdateTexture = true;
            continue;
        }

        if (maxLod - minLod > 5)
        {
            maxLod = 5;
        }
    }

    MinViewLod = minLod;
    MaxViewLod = maxLod;

    UpdateTextures();
    AddPatches(TerrainMesh, ViewFrustum);
}

void TerrainView::AddPatches(TerrainMesh* TerrainMesh, BvFrustum const& ViewFrustum)
{
    TerrainLodInfo const& finestLod = LodInfo[MinViewLod];

    Int2 trimOffset;

    switch (finestLod.InteriorTrim)
    {
        case INTERIOR_TOP_LEFT:
            trimOffset = Int2(1, 1);
            break;
        case INTERIOR_TOP_RIGHT:
            trimOffset = Int2(0, 1);
            break;
        case INTERIOR_BOTTOM_LEFT:
            trimOffset = Int2(1, 0);
            break;
        case INTERIOR_BOTTOM_RIGHT:
        default:
            trimOffset = Int2(0, 0);
            break;
    }

    trimOffset.X += BlockWidth;
    trimOffset.Y += BlockWidth;

    //
    // Draw interior L-shape for finest lod
    //

    TerrainPatchInstance& instance = AddInstance();
    instance.VertexScale            = Int2(finestLod.GridScale, finestLod.LodIndex);
    instance.VertexTranslate.X      = finestLod.Offset.X + trimOffset.X * finestLod.GridScale;
    instance.VertexTranslate.Y      = finestLod.Offset.Y + trimOffset.Y * finestLod.GridScale;
    instance.TexcoordOffset         = GetTexcoordOffset(finestLod);
    instance.QuadColor              = Color4(0.3f, 0.5f, 0.4f, 1.0f);
    AddPatchInstances(TerrainMesh->GetInteriorFinestPatch(), 1);

    //
    // Draw blocks
    //

    int numBlocks       = 0;
    int numCulledBlocks = 0;

    Int2 offset(trimOffset);

    if (!CullBlock(ViewFrustum, finestLod, offset))
    {
        AddBlock(finestLod, offset);
        numBlocks++;
    }
    else
    {
        numCulledBlocks++;
    }

    offset.X += BlockWidth;
    if (!CullBlock(ViewFrustum, finestLod, offset))
    {
        AddBlock(finestLod, offset);
        numBlocks++;
    }
    else
    {
        numCulledBlocks++;
    }

    offset.X = trimOffset.X;
    offset.Y += BlockWidth;
    if (!CullBlock(ViewFrustum, finestLod, offset))
    {
        AddBlock(finestLod, offset);
        numBlocks++;
    }
    else
    {
        numCulledBlocks++;
    }

    offset.X += BlockWidth;
    if (!CullBlock(ViewFrustum, finestLod, offset))
    {
        AddBlock(finestLod, offset);
        numBlocks++;
    }
    else
    {
        numCulledBlocks++;
    }

    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        offset.X = 0;
        offset.Y = 0;

        // 1
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 2
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += GapWidth;
        // 3
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 4
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X = 0;
        offset.Y += BlockWidth;

        // 5
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += BlockWidth;
        offset.X += GapWidth;
        offset.X += BlockWidth;
        // 6
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X = 0;
        offset.Y += BlockWidth;
        offset.Y += GapWidth;

        // 7
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += BlockWidth;
        offset.X += GapWidth;
        offset.X += BlockWidth;
        // 8
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X = 0;
        offset.Y += BlockWidth;

        // 9
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 10
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += GapWidth;
        // 11
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 12
        if (!CullBlock(ViewFrustum, lodInfo, offset))
        {
            AddBlock(lodInfo, offset);
            numBlocks++;
        }
        else
        {
            numCulledBlocks++;
        }
    }

    AddPatchInstances(TerrainMesh->GetBlockPatch(), numBlocks);

    //
    // Draw interior trims
    //

    int numTrims       = 0;
    int totalTrims     = 0;
    int numCulledTrims = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        if (lodInfo.InteriorTrim == INTERIOR_TOP_LEFT)
        {
            if (!CullInteriorTrim(ViewFrustum, lodInfo))
            {
                AddInteriorTopLeft(lodInfo);
                numTrims++;
            }
            else
            {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances(TerrainMesh->GetInteriorTLPatch(), numTrims);
    totalTrims += numTrims;

    numTrims = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        if (lodInfo.InteriorTrim == INTERIOR_TOP_RIGHT)
        {
            if (!CullInteriorTrim(ViewFrustum, lodInfo))
            {
                AddInteriorTopRight(lodInfo);
                numTrims++;
            }
            else
            {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances(TerrainMesh->GetInteriorTRPatch(), numTrims);
    totalTrims += numTrims;

    numTrims = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        if (lodInfo.InteriorTrim == INTERIOR_BOTTOM_LEFT)
        {
            if (!CullInteriorTrim(ViewFrustum, lodInfo))
            {
                AddInteriorBottomLeft(lodInfo);
                numTrims++;
            }
            else
            {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances(TerrainMesh->GetInteriorBLPatch(), numTrims);
    totalTrims += numTrims;

    numTrims = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        if (lodInfo.InteriorTrim == INTERIOR_BOTTOM_RIGHT)
        {
            if (!CullInteriorTrim(ViewFrustum, lodInfo))
            {
                AddInteriorBottomRight(lodInfo);
                numTrims++;
            }
            else
            {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances(TerrainMesh->GetInteriorBRPatch(), numTrims);
    totalTrims += numTrims;

    //
    // Draw vertical gaps
    //

    int numVertGaps   = 0;
    int numCulledGaps = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        offset.X = BlockWidth * 2;
        offset.Y = 0;
        if (!CullGapV(ViewFrustum, lodInfo, offset))
        {
            AddGapV(lodInfo, offset);
            numVertGaps++;
        }
        else
        {
            numCulledGaps++;
        }
        offset.Y += (BlockWidth * 3 + GapWidth);
        if (!CullGapV(ViewFrustum, lodInfo, offset))
        {
            AddGapV(lodInfo, offset);
            numVertGaps++;
        }
        else
        {
            numCulledGaps++;
        }
    }
    AddPatchInstances(TerrainMesh->GetVertGapPatch(), numVertGaps);

    //
    // Draw horizontal gaps
    //

    int numHorGaps = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        offset.X = 0;
        offset.Y = BlockWidth * 2;
        if (!CullGapH(ViewFrustum, lodInfo, offset))
        {
            AddGapH(lodInfo, offset);
            numHorGaps++;
        }
        else
        {
            numCulledGaps++;
        }
        offset.X += (BlockWidth * 3 + GapWidth);
        if (!CullGapH(ViewFrustum, lodInfo, offset))
        {
            AddGapH(lodInfo, offset);
            numHorGaps++;
        }
        else
        {
            numCulledGaps++;
        }
    }
    AddPatchInstances(TerrainMesh->GetHorGapPatch(), numHorGaps);

    //
    // Draw crack strips
    //

    int numCrackStrips = 0;
    for (int lod = MinViewLod; lod <= MaxViewLod - 1; lod++)
    {
        TerrainLodInfo const& lodInfo = LodInfo[lod];

        AddCrackLines(lodInfo);
        numCrackStrips++;
    }
    AddPatchInstances(TerrainMesh->GetCrackPatch(), numCrackStrips);

#if 0
    LOG( "Blocks {}/{}, gaps {}/{}, trims {}/{}, cracks {}/{}, total culled {}, total instances {}({})\n",
                    numCulledBlocks, numCulledBlocks+numBlocks,
                    numCulledGaps, numCulledGaps+numVertGaps+numHorGaps,
                    numCulledTrims, numCulledTrims+totalTrims,
                    0, numCrackStrips,
                    numCulledBlocks+numCulledGaps+numCulledTrims,
                    StartInstanceLocation,
                    numBlocks+numVertGaps+numHorGaps+totalTrims+numCrackStrips
                    + 1 // L-shape for finest lod
    );
#endif
}

void TerrainView::UpdateRect(TerrainLodInfo const& Lod, TerrainLodInfo const& CoarserLod, int MinX, int MaxX, int MinY, int MaxY)
{
    Int2   texelWorldPos;
    float  h[4];
    Float3 n;

    const float InvGridSizeCoarse = 1.0f / CoarserLod.GridScale;

    // TODO: Move this to GPU
    for (int y = MinY; y < MaxY; y++)
    {
        for (int x = MinX; x < MaxX; x++)
        {
            int wrapX = x & TextureWrapMask;
            int wrapY = y & TextureWrapMask;

            // from texture space to world space
            texelWorldPos.X = (x - Lod.TextureOffset.X) * Lod.GridScale + Lod.Offset.X;
            texelWorldPos.Y = (y - Lod.TextureOffset.Y) * Lod.GridScale + Lod.Offset.Y;

            HK_ASSERT(wrapX >= 0 && wrapY >= 0);
            HK_ASSERT(wrapX <= TextureSize - 1 && wrapY <= TextureSize - 1);

            int sampleLod = Lod.LodIndex;

            Float2& heightMap = Lod.HeightMap[wrapY * TextureSize + wrapX];

            heightMap.X = m_Terrain->ReadHeight(texelWorldPos.X, texelWorldPos.Y, sampleLod);

            const int texelStep = Lod.GridScale;
            h[0] = m_Terrain->ReadHeight(texelWorldPos.X, texelWorldPos.Y - texelStep, sampleLod);
            h[1] = m_Terrain->ReadHeight(texelWorldPos.X - texelStep, texelWorldPos.Y, sampleLod);
            h[2] = m_Terrain->ReadHeight(texelWorldPos.X + texelStep, texelWorldPos.Y, sampleLod);
            h[3] = m_Terrain->ReadHeight(texelWorldPos.X, texelWorldPos.Y + texelStep, sampleLod);

            // normal = tangent ^ binormal
            // correct tangent t = cross( binormal, normal );
            //Float3 n = Math::Cross( Float3( 0.0, h[3] - h[0], 2.0f * texelStep ),
            //                        Float3( 2.0f * texelStep, h[2] - h[1], 0.0 ) );

            n.X = h[1] - h[2];
            n.Y = 2 * texelStep;
            n.Z = h[0] - h[3];
            //n.NormalizeSelf();
            float invLength = Math::RSqrt(n.X * n.X + n.Y * n.Y + n.Z * n.Z);
            n.X *= invLength;
            n.Z *= invLength;

            byte* normal = &Lod.NormalMap[(wrapY * TextureSize + wrapX) * 4];
            normal[0]    = n.X * 127.5f + 127.5f;
            normal[1]    = n.Z * 127.5f + 127.5f;

            int ofsX = texelWorldPos.X - CoarserLod.Offset.X;
            int ofsY = texelWorldPos.Y - CoarserLod.Offset.Y;

            // from world space to texture space of coarser level
            wrapX = ofsX / CoarserLod.GridScale + CoarserLod.TextureOffset.X;
            wrapY = ofsY / CoarserLod.GridScale + CoarserLod.TextureOffset.Y;

            // wrap coordinates
            wrapX &= TextureWrapMask;
            wrapY &= TextureWrapMask;
            int wrapX2 = (wrapX + 1) & TextureWrapMask;
            int wrapY2 = (wrapY + 1) & TextureWrapMask;

            float fx = Math::Fract(float(ofsX) * InvGridSizeCoarse);
            float fy = Math::Fract(float(ofsY) * InvGridSizeCoarse);

            h[0] = CoarserLod.HeightMap[wrapY * TextureSize + wrapX].X;
            h[1] = CoarserLod.HeightMap[wrapY * TextureSize + wrapX2].X;
            h[2] = CoarserLod.HeightMap[wrapY2 * TextureSize + wrapX2].X;
            h[3] = CoarserLod.HeightMap[wrapY2 * TextureSize + wrapX].X;

            heightMap.Y = Math::Bilerp(h[0], h[1], h[3], h[2], Float2(fx, fy));

            //float zf = floor( zf_zd );
            //float zd = zf_zd / 512 + 256;

            byte* n0 = &CoarserLod.NormalMap[(wrapY * TextureSize + wrapX) * 4];
            byte* n1 = &CoarserLod.NormalMap[(wrapY * TextureSize + wrapX2) * 4];
            byte* n2 = &CoarserLod.NormalMap[(wrapY2 * TextureSize + wrapX2) * 4];
            byte* n3 = &CoarserLod.NormalMap[(wrapY2 * TextureSize + wrapX) * 4];

            normal[2] = Math::Clamp(Math::Bilerp(float(n0[0]), float(n1[0]), float(n3[0]), float(n2[0]), Float2(fx, fy)), 0.0f, 255.0f);
            normal[3] = Math::Clamp(Math::Bilerp(float(n0[1]), float(n1[1]), float(n3[1]), float(n2[1]), Float2(fx, fy)), 0.0f, 255.0f);

            //heightMap.Y = CoarserLod.HeightMap[wrapY * TextureSize + wrapX].X;
        }
    }
}

void TerrainView::UpdateTextures()
{
    const int count = TextureSize * TextureSize;

    for (int lod = MaxViewLod; lod >= MinViewLod; lod--)
    {
        TerrainLodInfo& lodInfo        = LodInfo[lod];
        TerrainLodInfo& coarserLodInfo = LodInfo[lod < MaxViewLod ? lod + 1 : lod];

        Int2 DeltaMove;
        DeltaMove.X = lodInfo.TextureOffset.X - lodInfo.PrevTextureOffset.X;
        DeltaMove.Y = lodInfo.TextureOffset.Y - lodInfo.PrevTextureOffset.Y;

        lodInfo.PrevTextureOffset = lodInfo.TextureOffset;

        int Min[2] = {0, 0};
        int Max[2] = {0, 0};

        for (int i = 0; i < 2; i++)
        {
            if (DeltaMove[i] < 0)
            {
                Max[i] = lodInfo.TextureOffset[i] - DeltaMove[i];
                Min[i] = lodInfo.TextureOffset[i];
            }
            else if (DeltaMove[i] > 0)
            {
                Min[i] = lodInfo.TextureOffset[i] + TextureSize - DeltaMove[i];
                Max[i] = lodInfo.TextureOffset[i] + TextureSize;
            }
        }

        int MinX = Min[0];
        int MinY = Min[1];
        int MaxX = Max[0];
        int MaxY = Max[1];

        bool bUpdateToGPU = false;

        if (Math::Abs(DeltaMove.X) >= TextureSize || Math::Abs(DeltaMove.Y) >= TextureSize || lodInfo.bForceUpdateTexture)
        {

            lodInfo.bForceUpdateTexture = false;

            // Update whole texture
            UpdateRect(lodInfo,
                       coarserLodInfo,
                       lodInfo.TextureOffset.X, lodInfo.TextureOffset.X + TextureSize,
                       lodInfo.TextureOffset.Y, lodInfo.TextureOffset.Y + TextureSize);

            bUpdateToGPU = true;
        }
        else
        {
            if (MinY != MaxY)
            {
                UpdateRect(lodInfo, coarserLodInfo, lodInfo.TextureOffset.X, lodInfo.TextureOffset.X + TextureSize, MinY, MaxY);

                bUpdateToGPU = true;
            }
            if (MinX != MaxX)
            {
                UpdateRect(lodInfo, coarserLodInfo, MinX, MaxX, lodInfo.TextureOffset.Y, lodInfo.TextureOffset.Y + TextureSize);

                bUpdateToGPU = true;
            }
        }

        if (bUpdateToGPU)
        {
            RenderCore::TextureRect rect;

            rect.Offset.MipLevel = 0;
            rect.Offset.X        = 0;
            rect.Offset.Y        = 0;
            rect.Offset.Z        = lod;
            rect.Dimension.X     = TextureSize;
            rect.Dimension.Y     = TextureSize;
            rect.Dimension.Z     = 1;

            lodInfo.MinH = 99999;
            lodInfo.MaxH = -99999;

            // TODO: Optimize this: precompute low resolution grid of heightmap pages with
            // minimum and maximum height
            for (int i = 0; i < count; i += 3)
            {
                lodInfo.MinH = Math::Min(lodInfo.MinH, lodInfo.HeightMap[i].X);
                lodInfo.MaxH = Math::Max(lodInfo.MaxH, lodInfo.HeightMap[i].X);
            }
            const int Margin = 2;
            lodInfo.MinH -= Margin;
            lodInfo.MaxH += Margin;

            // TODO: Update only dirty regions
            ClipmapArray->WriteRect(rect, count * sizeof(Float2), 4, lodInfo.HeightMap);

            NormalMapArray->WriteRect(rect, count * 4, 4, lodInfo.NormalMap);
        }
    }
}

void TerrainView::DrawDebug(DebugRenderer* InRenderer, TerrainMesh* TerrainMesh)
{
    HK_ASSERT(TerrainMesh->GetTextureSize() == TextureSize);

    TerrainRenderer = InRenderer;

    TerrainRenderer->SetColor(Color4(1, 1, 1, 1));
    for (BvAxisAlignedBox& box : BoundingBoxes)
    {
        TerrainRenderer->DrawAABB(box);
    }

#if 0

    TerrainVertex const * pVertices = TerrainMesh->GetVertexBufferCPU();
    unsigned short const * pIndices = TerrainMesh->GetIndexBufferCPU();

#    if 0
    static int64_t currentDrawCall = 0;

    static int64_t prev = 0;
    int64_t cur = Platform::SysMilliseconds();
    int64_t delta = prev ? cur - prev : 0;
    prev = cur;

    int totalDrawcalls = 0;
    for ( RenderCore::DrawIndexedIndirectCmd const & cmd : IndirectBuffer ) {
        totalDrawcalls += cmd.InstanceCount;
    }

    int cmdIndex = 0;
#    endif

    int drawCall = 0;
    for ( RenderCore::DrawIndexedIndirectCmd const & cmd : IndirectBuffer ) {

        //if ( cmdIndex < ( int(currentDrawCall / 10000.0) % IndirectBuffer.Size()) ) {

        for ( int i = 0 ; i < cmd.InstanceCount ; i++ ) {

            pDrawCallUniformData = &InstanceBuffer[drawCall++];

            //if ( drawCall < ( int(currentDrawCall / 40000.0) % totalDrawcalls) ) {

            DrawIndexedTriStrip( pVertices + cmd.BaseVertexLocation,
                                 pIndices + cmd.StartIndexLocation,
                                 cmd.IndexCountPerInstance );

            //}

            //currentDrawCall += delta;
        }

        //}

        //cmdIndex++;
        //currentDrawCall += delta;
    }

    //LOG( "Drawcalls {}\n", IndirectBuffer.Size() );



#endif
}

void TerrainView::DrawIndexedTriStrip(TerrainVertex const* Vertices, unsigned short const* Indices, int IndexCount)
{
    if (IndexCount < 3)
    {
        return;
    }

    TerrainVertex v[3];

    int i = 0;

    int t  = 0;
    v[t++] = Vertices[Indices[i++]];
    v[t++] = Vertices[Indices[i++]];

    for (; i < IndexCount; i++)
    {
        if (Indices[i] == RESET_INDEX)
        { // reset index
            t = 0;
            i++;
            if (i + 2 >= IndexCount)
            {
                return;
            }
            v[t++] = Vertices[Indices[i++]];
            v[t++] = Vertices[Indices[i]];
            continue;
        }

        v[t % 3] = Vertices[Indices[i]];

        TerrainVertex const& a = v[(t - 2) % 3];
        TerrainVertex const& b = v[(t - 1) % 3];
        TerrainVertex const& c = v[t % 3];

        if (t & 1)
        {
            DrawTerrainTriangle(c, b, a);
        }
        else
        {
            DrawTerrainTriangle(a, b, c);
        }

        t++;
    }
}

void TerrainView::DrawTerrainTriangle(TerrainVertex const& a, TerrainVertex const& b, TerrainVertex const& c)
{
    Float3 v0 = VertexShader(a);
    Float3 v1 = VertexShader(b);
    Float3 v2 = VertexShader(c);

    const Float3 lightVec = Float3(0.5f, 0.5f, -0.5f).Normalized();
    Float3       n        = Math::Cross((v1 - v0), (v2 - v0)).Normalized();
    float        dp       = Math::Max(Math::Dot(n, lightVec), 0.1f);

    TerrainRenderer->SetDepthTest(true);
    TerrainRenderer->SetColor(Color4(pDrawCallUniformData->QuadColor.R * dp,
                                     pDrawCallUniformData->QuadColor.G * dp,
                                     pDrawCallUniformData->QuadColor.B * dp,
                                     1));
    TerrainRenderer->DrawTriangle(v0, v1, v2, false);

    // Add little offset for zfighting
    v0.Y += 0.01f;
    v1.Y += 0.01f;
    v2.Y += 0.01f;
    TerrainRenderer->SetColor(Color4::White());
    //TerrainRenderer->SetColor( pDrawCallUniformData->QuadColor );
    TerrainRenderer->DrawLine(v0, v1);
    TerrainRenderer->DrawLine(v1, v2);
    TerrainRenderer->DrawLine(v2, v0);
}

Float3 TerrainView::VertexShader(TerrainVertex const& v)
{
    Float3 result;
    Int2   texelWorldPos;

    texelWorldPos.X = v.X * pDrawCallUniformData->VertexScale.X + pDrawCallUniformData->VertexTranslate.X;
    texelWorldPos.Y = v.Y * pDrawCallUniformData->VertexScale.X + pDrawCallUniformData->VertexTranslate.Y;

    int     lodIndex = (int)pDrawCallUniformData->VertexScale.Y;
    Float2* texture  = LodInfo[lodIndex].HeightMap;

#if 0
    const int MIN_TERRAIN_BOUNDS = -40;
    const int MAX_TERRAIN_BOUNDS = 40;

    texelWorldPos.X = Math::Clamp( texelWorldPos.X, MIN_TERRAIN_BOUNDS, MAX_TERRAIN_BOUNDS );
    texelWorldPos.Y = Math::Clamp( texelWorldPos.Y, MIN_TERRAIN_BOUNDS, MAX_TERRAIN_BOUNDS );
#endif

    result.X = texelWorldPos.X;
    result.Z = texelWorldPos.Y;

    Int2 texCoord;

    // from world space to texture space
    texCoord.X = (texelWorldPos.X + pDrawCallUniformData->TexcoordOffset.X) / pDrawCallUniformData->VertexScale.X;
    texCoord.Y = (texelWorldPos.Y + pDrawCallUniformData->TexcoordOffset.Y) / pDrawCallUniformData->VertexScale.X;

    // wrap coordinates
    texCoord.X &= TextureWrapMask;
    texCoord.Y &= TextureWrapMask;

    HK_ASSERT(texCoord.X >= 0 && texCoord.Y >= 0);
    HK_ASSERT(texCoord.X <= TextureSize - 1 && texCoord.Y <= TextureSize - 1);

    result.Y = texture[texCoord.Y * TextureSize + texCoord.X].X;
    return result;
}

HK_NAMESPACE_END
