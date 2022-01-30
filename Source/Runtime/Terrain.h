/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
#include "HitTest.h"
#include "BaseObject.h"

class ADebugRenderer;

struct STerrainLodInfo
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
    Float2 * HeightMap;
    /** Lod normal map data */
    byte * NormalMap;
};

struct STerrainPatch
{
    int IndexCount;
    int BaseVertex;
    int StartIndex;
};

enum { MAX_TERRAIN_LODS = 10 };

class ATerrainMesh : public ARefCounted
{
public:
    ATerrainMesh( int TextureSize );

    int GetTextureSize() const
    {
        return TextureSize;
    }

    /** Get vertex buffer in GPU */
    RenderCore::IBuffer * GetVertexBufferGPU() const { return VertexBufferGPU; }
    /** Get index buffer in GPU */
    RenderCore::IBuffer * GetIndexBufferGPU() const { return IndexBufferGPU; }

    /** Get vertex buffer in CPU. We keep it only for debug draw */
    STerrainVertex const * GetVertexBufferCPU() const { return VertexBuffer.ToPtr(); }
    /** Get index buffer in CPU. We keep it only for debug draw */
    unsigned short const * GetIndexBufferCPU() const { return IndexBuffer.ToPtr(); }

    int GetVertexCount() const { return VertexBuffer.Size(); }
    int GetIndexCount() const { return IndexBuffer.Size(); }

    STerrainPatch const & GetBlockPatch() const { return BlockPatch; }
    STerrainPatch const & GetHorGapPatch() const { return HorGapPatch; }
    STerrainPatch const & GetVertGapPatch() const { return VertGapPatch; }
    STerrainPatch const & GetInteriorTLPatch() const { return InteriorTLPatch; }
    STerrainPatch const & GetInteriorTRPatch() const { return InteriorTRPatch; }
    STerrainPatch const & GetInteriorBLPatch() const { return InteriorBLPatch; }
    STerrainPatch const & GetInteriorBRPatch() const { return InteriorBRPatch; }
    STerrainPatch const & GetInteriorFinestPatch() const { return InteriorFinestPatch; }
    STerrainPatch const & GetCrackPatch() const { return CrackPatch; }

private:
    /** Initial texture size */
    int TextureSize;

    STerrainPatch BlockPatch;
    STerrainPatch HorGapPatch;
    STerrainPatch VertGapPatch;
    STerrainPatch InteriorTLPatch;
    STerrainPatch InteriorTRPatch;
    STerrainPatch InteriorBLPatch;
    STerrainPatch InteriorBRPatch;
    STerrainPatch InteriorFinestPatch;
    STerrainPatch CrackPatch;

    /** Vertex buffer in GPU */
    TRef< RenderCore::IBuffer > VertexBufferGPU;
    /** Index buffer in GPU */
    TRef< RenderCore::IBuffer > IndexBufferGPU;

    /** Vertex buffer in CPU. We keep it only for debug draw */
    TPodVector< STerrainVertex > VertexBuffer;
    /** Index buffer in CPU. We keep it only for debug draw */
    TPodVector< unsigned short > IndexBuffer;
};

struct STerrainTriangle
{
    Float3 Vertices[3];
    Float3 Normal;
    Float2 Texcoord;
};

class ATerrain : public ABaseObject
{
    AN_CLASS( ATerrain, ABaseObject )

public:
    ATerrain();
    ~ATerrain();

    float GetMinHeight() const { return MinHeight; }

    float GetMaxHeight() const { return MaxHeight; }

    float Height( int X, int Z, int Lod ) const;

    Int2 const & GetClipMin() const { return ClipMin; }
    Int2 const & GetClipMax() const { return ClipMax; }

    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast( Float3 const & RayStart, Float3 const & RayDir, float Distance, bool bCullBackFace, TPodVector< STriangleHitResult > & HitResult ) const;
    /** Check ray intersection */
    bool RaycastClosest( Float3 const & RayStart, Float3 const & RayDir, float Distance, bool bCullBackFace, STriangleHitResult & HitResult ) const;

    bool GetTriangleVertices( float X, float Z, Float3 & V0, Float3 & V1, Float3 & V2 ) const;

    bool GetNormal( float X, float Z, Float3 & Normal ) const;

    bool GetTexcoord( float X, float Z, Float2 & Texcoord ) const;

    bool GetTerrainTriangle( float X, float Z, STerrainTriangle & Triangle ) const;

    class btHeightfieldTerrainShape * GetHeightfieldShape() const { return HeightfieldShape.GetObject(); }

private:
    int HeightmapResolution;
    int HeightmapLods;
    TPodVector< float * > Heightmap;
    float MinHeight;
    float MaxHeight;
    TUniqueRef< btHeightfieldTerrainShape > HeightfieldShape;
    Int2 ClipMin;
    Int2 ClipMax;
    BvAxisAlignedBox BoundingBox;
};

class ATerrainView : public ARefCounted
{
public:
    ATerrainView( int TextureSize );
    virtual ~ATerrainView();

    void SetTerrain( ATerrain * Mesh );

    void Update(AStreamedMemoryGPU* StreamedMemory, ATerrainMesh* TerrainMesh, Float3 const& ViewPosition, BvFrustum const& ViewFrustum);

    int GetTextureSize() const
    {
        return TextureSize;
    }

    size_t GetInstanceBufferStreamHandle() const
    {
        return InstanceBufferStreamHandle;
    }

    size_t GetIndirectBufferStreamHandle() const
    {
        return IndirectBufferStreamHandle;
    }

    int GetIndirectBufferDrawCount() const
    {
        return IndirectBuffer.Size();
    }

    RenderCore::ITexture * GetClipmapArray() const
    {
        return ClipmapArray;
    }

    RenderCore::ITexture * GetNormalMapArray() const
    {
        return NormalMapArray;
    }

    float GetViewHeight() const
    {
        return ViewHeight;
    }

    void DrawDebug( ADebugRenderer * InRenderer, ATerrainMesh * TerrainMesh );

private:
    void MakeView( ATerrainMesh * TerrainMesh, Float3 const & ViewPosition, BvFrustum const & ViewFrustum );
    void AddPatches( ATerrainMesh * TerrainMesh, BvFrustum const & ViewFrustum );
    void AddBlock( STerrainLodInfo const & Lod, Int2 const & Offset );
    void AddGapV( STerrainLodInfo const & Lod, Int2 const & Offset );
    void AddGapH( STerrainLodInfo const & Lod, Int2 const & Offset );
    void AddInteriorTopLeft( STerrainLodInfo const & Lod );
    void AddInteriorTopRight( STerrainLodInfo const & Lod );
    void AddInteriorBottomLeft( STerrainLodInfo const & Lod );
    void AddInteriorBottomRight( STerrainLodInfo const & Lod );
    void AddCrackLines( STerrainLodInfo const & Lod );
    bool CullBlock( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod, Int2 const & Offset );
    bool CullGapV( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod, Int2 const & Offset );
    bool CullGapH( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod, Int2 const & Offset );
    bool CullInteriorTrim( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod );

    void UpdateTextures();
    void UpdateRect( STerrainLodInfo const & Lod, STerrainLodInfo const & CoarserLod, int MinX, int MaxX, int MinY, int MaxY );

    STerrainPatchInstance & AddInstance()
    {
        return InstanceBuffer.Append();
    }

    void AddPatchInstances( STerrainPatch const & Patch, int InstanceCount )
    {
        if ( InstanceCount > 0 ) {
            RenderCore::SDrawIndexedIndirectCmd & blocks = IndirectBuffer.Append();
            blocks.IndexCountPerInstance = Patch.IndexCount;
            blocks.InstanceCount = InstanceCount;
            blocks.StartIndexLocation = Patch.StartIndex;
            blocks.BaseVertexLocation = Patch.BaseVertex;
            blocks.StartInstanceLocation = StartInstanceLocation;

            StartInstanceLocation += InstanceCount;
        }
    }

    enum
    {
        INTERIOR_TOP_LEFT,
        INTERIOR_TOP_RIGHT,
        INTERIOR_BOTTOM_LEFT,
        INTERIOR_BOTTOM_RIGHT
    };

    const int TextureSize;
    const int TextureWrapMask;
    const int GapWidth;
    const int BlockWidth;
    const int LodGridSize;
    const int HalfGridSize;

    TRef< ATerrain > Terrain;

    /** Current lod state */
    STerrainLodInfo LodInfo[MAX_TERRAIN_LODS];

    /** Min viewable lod */
    int MinViewLod;
    /** Max viewable lod */
    int MaxViewLod;
    /** Height above the terrain */
    float ViewHeight;

    TPodVector< STerrainPatchInstance > InstanceBuffer;
    TPodVector< RenderCore::SDrawIndexedIndirectCmd > IndirectBuffer;

    TRef< RenderCore::ITexture > ClipmapArray;
    TRef< RenderCore::ITexture > NormalMapArray;

    size_t InstanceBufferStreamHandle;
    size_t IndirectBufferStreamHandle;

    int StartInstanceLocation;

    // Debug draw
    void DrawIndexedTriStrip( STerrainVertex const * Vertices, unsigned short const * Indices, int IndexCount );
    void DrawTerrainTriangle( STerrainVertex const & a, STerrainVertex const & b, STerrainVertex const & c );
    Float3 VertexShader( STerrainVertex const & v );
    ADebugRenderer * TerrainRenderer;
    STerrainPatchInstance const * pDrawCallUniformData;
    TPodVector< BvAxisAlignedBox > BoundingBoxes;
};
