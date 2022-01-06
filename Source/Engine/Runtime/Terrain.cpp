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

/*
TODO:

Textures update
Streaming
Frustum culling:
 Рассчитать ZMin, ZMax для каждого блока. Расчет можно сделать приблизительный,
 взяв за основу высоту центра:
   zMin = centerH - blockSize * f;
   zMax = centerH + blockSize * f;
   f - некоторая величина, дающая запас

FIXME: move normal texture fetching to fragment shader?

Future:
 Предрасчитать оклюдеры внутри гор, чтобы можно было отсекать невидимые объекты


Построение NavMesh с учетом ландшафта

*/

#include "Terrain.h"
#include "World.h"
#include "DebugRenderer.h"
#include "Engine.h"
#include "RuntimeVariable.h"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "BulletCompatibility.h"

#include <Geometry/BV/BvIntersect.h>
#include <RenderCore/VertexMemoryGPU.h>

static const unsigned short RESET_INDEX = 0xffff;

ARuntimeVariable com_TerrainMinLod(_CTS("com_TerrainMinLod"),_CTS("0"));
ARuntimeVariable com_TerrainMaxLod(_CTS("com_TerrainMaxLod"),_CTS("5"));
ARuntimeVariable com_ShowTerrainMemoryUsage(_CTS("com_ShowTerrainMemoryUsage"),_CTS("0"));

void CreateTriangleStripPatch( int NumQuadsX, int NumQuadsY, TPodVector< STerrainVertex > & Vertices, TPodVector< unsigned short > & Indices )
{
    int numVertsX = NumQuadsX + 1;
    int numVertsY = NumQuadsY + 1;

    Vertices.Resize( numVertsX * numVertsY );

    for ( int y = 0; y < NumQuadsY ; y++ ) {
        for ( int x = 0 ; x < NumQuadsX + 1 ; x++ ) {
            Indices.Append( x + (y)*numVertsX );
            Indices.Append( x + (y+1)*numVertsX );
        }
        Indices.Append( RESET_INDEX );
    }

    STerrainVertex * vert = Vertices.ToPtr();

    for ( int i = 0 ; i < numVertsY ; i++ ) {
        for ( int j = 0 ; j < numVertsX ; j++ ) {
            vert[i * numVertsX + j].X = j;
            vert[i * numVertsX + j].Y = i;
        }
    }
}

AN_FORCEINLINE STerrainVertex MakeVertex( short X, short Y )
{
    STerrainVertex v;
    v.X = X;
    v.Y = Y;
    return v;
}

ATerrainMesh::ATerrainMesh( int InTextureSize )
{
    TPodVector< STerrainVertex > blockVerts;
    TPodVector< unsigned short > blockIndices;
    TPodVector< STerrainVertex > horGapVertices;
    TPodVector< unsigned short > horGapIndices;
    TPodVector< STerrainVertex > vertGapVertices;
    TPodVector< unsigned short > vertGapIndices;
    TPodVector< STerrainVertex > interiorTLVertices;
    TPodVector< STerrainVertex > interiorTRVertices;
    TPodVector< STerrainVertex > interiorBLVertices;
    TPodVector< STerrainVertex > interiorBRVertices;
    TPodVector< STerrainVertex > interiorFinestVertices;
    TPodVector< unsigned short > interiorTLIndices;
    TPodVector< unsigned short > interiorTRIndices;
    TPodVector< unsigned short > interiorBLIndices;
    TPodVector< unsigned short > interiorBRIndices;
    TPodVector< unsigned short > interiorFinestIndices;
    TPodVector< STerrainVertex > crackVertices;
    TPodVector< unsigned short > crackIndices;

    AN_ASSERT( IsPowerOfTwo( InTextureSize ) );

    const int BlockWidth = InTextureSize / 4 - 1;
    const int GapWidth = 2;
    const int CrackTrianglesCount = (BlockWidth * 4 + GapWidth) / 2;

    TextureSize = InTextureSize;

    // Blocks
    CreateTriangleStripPatch( BlockWidth, BlockWidth, blockVerts, blockIndices );

    // Gaps
    CreateTriangleStripPatch( BlockWidth, GapWidth, horGapVertices, horGapIndices );
    CreateTriangleStripPatch( GapWidth, BlockWidth, vertGapVertices, vertGapIndices );

    // Interior L-shapes
    int i = 0;
    for ( int q = 0 ; q < (BlockWidth*2 + GapWidth) + 1 ; q++ ) {
        interiorTLVertices.Append( MakeVertex( q, 0 ) );
        interiorTLVertices.Append( MakeVertex( q, 1 ) );

        interiorTRVertices.Append( MakeVertex( q, 0 ) );
        interiorTRVertices.Append( MakeVertex( q, 1 ) );

        interiorBLVertices.Append( MakeVertex( q, BlockWidth*2 + GapWidth - 1 ) );
        interiorBLVertices.Append( MakeVertex( q, BlockWidth*2 + GapWidth ) );

        interiorBRVertices.Append( MakeVertex( q, BlockWidth*2 + GapWidth - 1 ) );
        interiorBRVertices.Append( MakeVertex( q, BlockWidth*2 + GapWidth ) );

        interiorTLIndices.Append( i );
        interiorTLIndices.Append( i + 1 );

        interiorTRIndices.Append( i );
        interiorTRIndices.Append( i + 1 );

        interiorBLIndices.Append( i );
        interiorBLIndices.Append( i + 1 );

        interiorBRIndices.Append( i );
        interiorBRIndices.Append( i + 1 );

        i += 2;
    }

    interiorTLIndices.Append( RESET_INDEX );
    interiorTRIndices.Append( RESET_INDEX );
    interiorBLIndices.Append( RESET_INDEX );
    interiorBRIndices.Append( RESET_INDEX );

    int prev_a_tl = 1;
    int prev_b_tl = prev_a_tl + 2;

    int prev_a_tr = ((BlockWidth*2 + GapWidth) + 1) * 2 - 3;
    int prev_b_tr = prev_a_tr + 2;

    int q;
    for ( q = 0 ; q < (BlockWidth*2 + GapWidth) - 1 ; q++ ) {
        interiorTLIndices.Append( prev_a_tl );
        interiorTLIndices.Append( i );
        interiorTLIndices.Append( prev_b_tl );
        interiorTLIndices.Append( i + 1 );
        prev_a_tl = i;
        prev_b_tl = i + 1;

        interiorTRIndices.Append( prev_a_tr );
        interiorTRIndices.Append( i );
        interiorTRIndices.Append( prev_b_tr );
        interiorTRIndices.Append( i + 1 );
        prev_a_tr = i;
        prev_b_tr = i + 1;

        if ( q < (BlockWidth*2 + GapWidth) - 2 ) {
            interiorTLIndices.Append( RESET_INDEX );
            interiorTRIndices.Append( RESET_INDEX );

            interiorBLIndices.Append( i );
            interiorBLIndices.Append( i + 2 );
            interiorBLIndices.Append( i + 1 );
            interiorBLIndices.Append( i + 3 );
            interiorBLIndices.Append( RESET_INDEX );

            interiorBRIndices.Append( i );
            interiorBRIndices.Append( i + 2 );
            interiorBRIndices.Append( i + 1 );
            interiorBRIndices.Append( i + 3 );
            interiorBRIndices.Append( RESET_INDEX );

            i += 2;
        }

        interiorTLVertices.Append( MakeVertex( 0, q + 2 ) );
        interiorTLVertices.Append( MakeVertex( 1, q + 2 ) );

        interiorTRVertices.Append( MakeVertex( (BlockWidth*2 + GapWidth) - 1, q + 2 ) );
        interiorTRVertices.Append( MakeVertex( (BlockWidth*2 + GapWidth), q + 2 ) );

        interiorBLVertices.Append( MakeVertex( 0, q ) );
        interiorBLVertices.Append( MakeVertex( 1, q ) );

        interiorBRVertices.Append( MakeVertex( (BlockWidth*2 + GapWidth) - 1, q ) );
        interiorBRVertices.Append( MakeVertex( (BlockWidth*2 + GapWidth), q ) );
    }

    interiorBLIndices.Append( i );
    interiorBLIndices.Append( 0 );
    interiorBLIndices.Append( i + 1 );
    interiorBLIndices.Append( 2 );

    interiorBRIndices.Append( i );
    interiorBRIndices.Append( ((BlockWidth*2 + GapWidth) + 1) * 2 - 4 );
    interiorBRIndices.Append( i + 1 );
    interiorBRIndices.Append( ((BlockWidth*2 + GapWidth) + 1) * 2 - 2 );

    for ( STerrainVertex & v : interiorTLVertices ) {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for ( STerrainVertex & v : interiorTRVertices ) {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for ( STerrainVertex & v : interiorBLVertices ) {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }
    for ( STerrainVertex & v : interiorBRVertices ) {
        v.X += BlockWidth;
        v.Y += BlockWidth;
    }

    // Finest lod interior L-shape

    i = 0;
    int x, y;

    y = BlockWidth*2;
    for ( x = 0 ; x < BlockWidth*2 + 2 ; x++ ) {
        interiorFinestIndices.Append( i++ );
        interiorFinestIndices.Append( i++ );

        interiorFinestVertices.Append( MakeVertex( x, y ) );
        interiorFinestVertices.Append( MakeVertex( x, y + 1 ) );
    }
    interiorFinestIndices.Append( RESET_INDEX );

    x = BlockWidth*2;
    for ( y = 0 ; y < BlockWidth*2 ; y++ ) {
        interiorFinestIndices.Append( i );
        interiorFinestIndices.Append( i + 2 );
        interiorFinestIndices.Append( i + 1 );
        interiorFinestIndices.Append( i + 3 );
        interiorFinestIndices.Append( RESET_INDEX );

        interiorFinestVertices.Append( MakeVertex( x, y ) );
        interiorFinestVertices.Append( MakeVertex( x + 1, y ) );

        i += 2;
    }

    interiorFinestVertices.Append( MakeVertex( x, y ) );
    interiorFinestVertices.Append( MakeVertex( x + 1, y ) );

    int debugOffset = 0;//-1;

    // top line
    int j = 0;
    for ( i = 0 ; i < CrackTrianglesCount ; i++ ) {
        crackIndices.Append( i*2 );
        crackIndices.Append( i*2 );
        crackIndices.Append( i*2+1 );
        crackIndices.Append( i*2+2 );

        crackVertices.Append( MakeVertex( i * 2, j ) );
        crackVertices.Append( MakeVertex( i * 2 + 1, j-debugOffset ) );
    }
    // right line
    j = CrackTrianglesCount * 2;
    int vertOfs = crackVertices.Size();
    for ( i = 0 ; i < CrackTrianglesCount ; i++ ) {
        crackIndices.Append( vertOfs + i*2 );
        crackIndices.Append( vertOfs + i*2 );
        crackIndices.Append( vertOfs + i*2+1 );
        crackIndices.Append( vertOfs + i*2+2 );

        crackVertices.Append( MakeVertex( j, i * 2 ) );
        crackVertices.Append( MakeVertex( j+debugOffset, i * 2 + 1 ) );
    }
    // bottom line
    j = CrackTrianglesCount * 2;
    vertOfs = crackVertices.Size();
    for ( i = 0 ; i < CrackTrianglesCount ; i++ ) {
        crackIndices.Append( vertOfs + i*2 );
        crackIndices.Append( vertOfs + i*2 );
        crackIndices.Append( vertOfs + i*2+1 );
        crackIndices.Append( vertOfs + i*2+2 );

        crackVertices.Append( MakeVertex( j - i * 2, j ) );
        crackVertices.Append( MakeVertex( j - i * 2 - 1, j+debugOffset ) );
    }
    // left line
    j = CrackTrianglesCount * 2;
    vertOfs = crackVertices.Size();
    for ( i = 0 ; i < CrackTrianglesCount ; i++ ) {
        crackIndices.Append( vertOfs + i*2 );
        crackIndices.Append( vertOfs + i*2 );
        crackIndices.Append( vertOfs + i*2+1 );
        crackIndices.Append( vertOfs + i*2+2 );

        crackVertices.Append( MakeVertex( 0, j - i * 2 ) );
        crackVertices.Append( MakeVertex( -debugOffset, j - i * 2 - 1 ) );
    }
    crackVertices.Append( MakeVertex( 0, 0 ) );

    // Reverse face culling
    crackVertices.Reverse();
    crackIndices.Reverse();

    // Create single vertex and index buffer

    VertexBuffer.Resize( blockVerts.Size() +
                         horGapVertices.Size() +
                         vertGapVertices.Size() +
                         interiorTLVertices.Size() +
                         interiorTRVertices.Size() +
                         interiorBLVertices.Size() +
                         interiorBRVertices.Size() +
                         interiorFinestVertices.Size() +
                         crackVertices.Size() );

    IndexBuffer.Resize( blockIndices.Size() +
                        horGapIndices.Size() +
                        vertGapIndices.Size() +
                        interiorTLIndices.Size() +
                        interiorTRIndices.Size() +
                        interiorBLIndices.Size() +
                        interiorBRIndices.Size() +
                        interiorFinestIndices.Size() +
                        crackIndices.Size() );

    int firstVert = 0;
    int firstIndex = 0;

    auto AddPatch = [&]( STerrainPatch & Patch, TPodVector< STerrainVertex > const & VB, TPodVector< unsigned short > const & IB )
    {
        Patch.BaseVertex = firstVert;
        Patch.StartIndex = firstIndex;
        Patch.IndexCount = IB.Size();
        Platform::Memcpy( VertexBuffer.ToPtr() + firstVert, VB.ToPtr(), VB.Size() * sizeof( STerrainVertex ) );
        firstVert += VB.Size();
        Platform::Memcpy( IndexBuffer.ToPtr() + firstIndex, IB.ToPtr(), IB.Size() * sizeof( unsigned short ) );
        firstIndex += IB.Size();
    };

    AddPatch( BlockPatch, blockVerts, blockIndices );
    AddPatch( HorGapPatch, horGapVertices, horGapIndices );
    AddPatch( VertGapPatch, vertGapVertices, vertGapIndices );
    AddPatch( InteriorTLPatch, interiorTLVertices, interiorTLIndices );
    AddPatch( InteriorTRPatch, interiorTRVertices, interiorTRIndices );
    AddPatch( InteriorBLPatch, interiorBLVertices, interiorBLIndices );
    AddPatch( InteriorBRPatch, interiorBRVertices, interiorBRIndices );
    AddPatch( InteriorFinestPatch, interiorFinestVertices, interiorFinestIndices );
    AddPatch( CrackPatch, crackVertices, crackIndices );

    AN_ASSERT( firstVert == VertexBuffer.Size() );
    AN_ASSERT( firstIndex == IndexBuffer.Size() );

    RenderCore::IDevice* device = GEngine->GetRenderDevice();

    RenderCore::SBufferDesc ci = {};
    ci.bImmutableStorage = true;

    ci.SizeInBytes = VertexBuffer.Size() * sizeof( STerrainVertex );
    device->CreateBuffer( ci, VertexBuffer.ToPtr(), &VertexBufferGPU );

    ci.SizeInBytes = IndexBuffer.Size() * sizeof( unsigned short );
    device->CreateBuffer( ci, IndexBuffer.ToPtr(), &IndexBufferGPU );

    GLogger.Printf( "Terrain Mesh: Total vertices %d, Total indices %d\n", VertexBuffer.Size(), IndexBuffer.Size() );
}


ATerrainView::ATerrainView( int InTextureSize )
    : TextureSize( InTextureSize )
    , TextureWrapMask( InTextureSize - 1 )
    , GapWidth( 2 )
    , BlockWidth( TextureSize / 4 - 1 )
    , LodGridSize( TextureSize - 2 )
    , HalfGridSize( LodGridSize >> 1 )
    , ViewHeight( 0.0f )
{
    for ( int i = 0 ; i < MAX_TERRAIN_LODS ; i++ ) {
        LodInfo[i].HeightMap = (Float2 *)GHeapMemory.Alloc( TextureSize * TextureSize * sizeof( Float2 ) );
        LodInfo[i].NormalMap = (byte *)GHeapMemory.Alloc( TextureSize * TextureSize * 4 );
        LodInfo[i].LodIndex = i;

        LodInfo[i].TextureOffset.X = 0;
        LodInfo[i].TextureOffset.Y = 0;

        LodInfo[i].PrevTextureOffset.X = 0;
        LodInfo[i].PrevTextureOffset.Y = 0;

        LodInfo[i].bForceUpdateTexture = true;
    }

    MinViewLod = MaxViewLod = 0;

    auto textureFormat = RenderCore::STextureDesc()
                             .SetFormat(RenderCore::TEXTURE_FORMAT_RG32F)
                             .SetResolution(RenderCore::STextureResolution2DArray(TextureSize,
                                                                                  TextureSize,
                                                                                  MAX_TERRAIN_LODS))
                             .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);
    GEngine->GetRenderDevice()->CreateTexture(textureFormat, &ClipmapArray);

    auto normalMapFormat = RenderCore::STextureDesc()
                               .SetFormat(RenderCore::TEXTURE_FORMAT_RGBA8)
                               .SetResolution(RenderCore::STextureResolution2DArray(TextureSize,
                                                                                    TextureSize,
                                                                                    MAX_TERRAIN_LODS))
                               .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);
    GEngine->GetRenderDevice()->CreateTexture(normalMapFormat, &NormalMapArray);
}

ATerrainView::~ATerrainView()
{
    for ( int i = 0 ; i < MAX_TERRAIN_LODS ; i++ ) {
        GHeapMemory.Free( LodInfo[i].HeightMap );
        GHeapMemory.Free( LodInfo[i].NormalMap );
    }
}

void ATerrainView::SetTerrain( ATerrain * InTerrain )
{
    if ( Terrain == InTerrain ) {
        return;
    }

    Terrain = InTerrain;
    for ( int i = 0 ; i < MAX_TERRAIN_LODS ; i++ ) {
        LodInfo[i].bForceUpdateTexture = true;
    }
}

void ATerrainView::Update( AStreamedMemoryGPU* StreamedMemory, ATerrainMesh * TerrainMesh, Float3 const & ViewPosition, BvFrustum const & ViewFrustum )
{
    AN_ASSERT( TerrainMesh->GetTextureSize() == TextureSize );

    BoundingBoxes.Clear();

    IndirectBuffer.Clear();
    InstanceBuffer.Clear();

    StartInstanceLocation = 0;

    if ( !ViewFrustum.IsBoxVisible( Terrain->GetBoundingBox() ) ) {
        return;
    }

    MakeView( TerrainMesh, ViewPosition, ViewFrustum );

    InstanceBufferStreamHandle = StreamedMemory->AllocateVertex(InstanceBuffer.Size() * sizeof(STerrainPatchInstance),
                                                                  InstanceBuffer.ToPtr() );

    IndirectBufferStreamHandle = StreamedMemory->AllocateWithCustomAlignment(IndirectBuffer.Size() * sizeof(RenderCore::SDrawIndexedIndirectCmd),
                                                                               16, // FIXME: is this alignment correct for DrawIndirect commands?
                                                                               IndirectBuffer.ToPtr() );

    if ( com_ShowTerrainMemoryUsage ) {
        GLogger.Printf( "Instance buffer size in bytes %d\n", InstanceBuffer.Size() * sizeof( STerrainPatchInstance ) );
        GLogger.Printf( "Indirect buffer size in bytes %d\n", IndirectBuffer.Size() * sizeof( RenderCore::SDrawIndexedIndirectCmd ) );
    }
}

static AN_FORCEINLINE Int2 GetTexcoordOffset( STerrainLodInfo const & Lod )
{
    Int2 TexcoordOffset;
    TexcoordOffset.X = Lod.TextureOffset.X * Lod.GridScale - Lod.Offset.X;
    TexcoordOffset.Y = Lod.TextureOffset.Y * Lod.GridScale - Lod.Offset.Y;
    return TexcoordOffset;
}

bool ATerrainView::CullBlock( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod, Int2 const & Offset )
{
    int blockSize = BlockWidth * Lod.GridScale;
    int minX = Offset.X * Lod.GridScale + Lod.Offset.X;
    int minZ = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    int maxX = minX + blockSize;
    int maxZ = minZ + blockSize;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if ( !BvBoxOverlapBox( Terrain->GetBoundingBox(), box ) ) {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible( box );
}

bool ATerrainView::CullGapV( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod, Int2 const & Offset )
{
    int blockSize = BlockWidth * Lod.GridScale;
    int minX = Offset.X * Lod.GridScale + Lod.Offset.X;
    int minZ = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    int maxX = minX + 2 * Lod.GridScale;
    int maxZ = minZ + blockSize;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if ( !BvBoxOverlapBox( Terrain->GetBoundingBox(), box ) ) {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible( box );
}

bool ATerrainView::CullGapH( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod, Int2 const & Offset )
{
    int blockSize = BlockWidth * Lod.GridScale;
    int minX = Offset.X * Lod.GridScale + Lod.Offset.X;
    int minZ = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    int maxX = minX + blockSize;
    int maxZ = minZ + 2 * Lod.GridScale;

    BvAxisAlignedBox box;
    box.Mins.X = minX;
    box.Mins.Y = Lod.MinH;
    box.Mins.Z = minZ;
    box.Maxs.X = maxX;
    box.Maxs.Y = Lod.MaxH;
    box.Maxs.Z = maxZ;

    if ( !BvBoxOverlapBox( Terrain->GetBoundingBox(), box ) ) {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible( box );
}

bool ATerrainView::CullInteriorTrim( BvFrustum const & ViewFrustum, STerrainLodInfo const & Lod )
{
    int blockSize = BlockWidth * Lod.GridScale;
    int interiorSize = (BlockWidth*2 + GapWidth) * Lod.GridScale;

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

    if ( !BvBoxOverlapBox( Terrain->GetBoundingBox(), box ) ) {
        //BoundingBoxes.Append( box );
        return true;
    }

    //BoundingBoxes.Append( box );

    return !ViewFrustum.IsBoxVisible( box );
}

void ATerrainView::AddBlock( STerrainLodInfo const & Lod, Int2 const & Offset )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate.X = Offset.X * Lod.GridScale + Lod.Offset.X;
    instance.VertexTranslate.Y = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.5f, 0.5f, 0.5f, 1.0f );
}

void ATerrainView::AddGapV( STerrainLodInfo const & Lod, Int2 const & Offset )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate.X = Offset.X * Lod.GridScale + Lod.Offset.X;
    instance.VertexTranslate.Y = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.2f, 0.7f, 0.2f, 1.0f );
}

void ATerrainView::AddGapH( STerrainLodInfo const & Lod, Int2 const & Offset )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate.X = Offset.X * Lod.GridScale + Lod.Offset.X;
    instance.VertexTranslate.Y = Offset.Y * Lod.GridScale + Lod.Offset.Y;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.2f, 0.7f, 0.2f, 1.0f );
}

void ATerrainView::AddInteriorTopLeft( STerrainLodInfo const & Lod )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.5f, 0.5f, 1.0f, 1.0f );
}

void ATerrainView::AddInteriorTopRight( STerrainLodInfo const & Lod )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.5f, 0.5f, 1.0f, 1.0f );
}

void ATerrainView::AddInteriorBottomLeft( STerrainLodInfo const & Lod )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.5f, 0.5f, 1.0f, 1.0f );
}

void ATerrainView::AddInteriorBottomRight( STerrainLodInfo const & Lod )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0.5f, 0.5f, 1.0f, 1.0f );
}

void ATerrainView::AddCrackLines( STerrainLodInfo const & Lod )
{
    STerrainPatchInstance & instance = AddInstance();

    instance.VertexScale = Int2( Lod.GridScale, Lod.LodIndex );
    instance.VertexTranslate = Lod.Offset;
    instance.TexcoordOffset = GetTexcoordOffset( Lod );
    instance.QuadColor = Color4( 0, 1, 0, 1.0f );
}

void ATerrainView::MakeView( ATerrainMesh * TerrainMesh, Float3 const & ViewPosition, BvFrustum const & ViewFrustum )
{
    int minLod = Math::Max( com_TerrainMinLod.GetInteger(), 0 );
    int maxLod = Math::Min( com_TerrainMaxLod.GetInteger(), MAX_TERRAIN_LODS-1 );

    float terrainH = Terrain->Height( ViewPosition.X, ViewPosition.Z, 0 );

    // height above the terrain
    ViewHeight = Math::Max( ViewPosition.Y - terrainH, 0.0f );

    for ( int lod = minLod ; lod <= maxLod ; lod++ ) {
        STerrainLodInfo & lodInfo = LodInfo[lod];

        int gridScale = 1 << lod;
        int snapSize = gridScale * 2;
        int gridExtent = gridScale * LodGridSize;

        Int2 snapPos;
        snapPos.X = (Math::Floor( ViewPosition.X / snapSize ) + 0.5f) * snapSize;
        snapPos.Y = (Math::Floor( ViewPosition.Z / snapSize ) + 0.5f) * snapSize;

        Float2 snapOffset;
        snapOffset.X = ViewPosition.X - snapPos.X;
        snapOffset.Y = ViewPosition.Z - snapPos.Y;

        lodInfo.Offset.X = snapPos.X - HalfGridSize * gridScale;
        lodInfo.Offset.Y = snapPos.Y - HalfGridSize * gridScale;
        lodInfo.TextureOffset.X = snapPos.X / gridScale;
        lodInfo.TextureOffset.Y = snapPos.Y / gridScale;
        lodInfo.GridScale = gridScale;

        lodInfo.InteriorTrim = snapOffset.X > 0.0f ?
            (snapOffset.Y > 0.0f ? INTERIOR_TOP_LEFT : INTERIOR_BOTTOM_LEFT)
            : (snapOffset.Y > 0.0f ? INTERIOR_TOP_RIGHT : INTERIOR_BOTTOM_RIGHT);

        if ( minLod < maxLod && gridExtent < ViewHeight * 2.5f ) {
            minLod++;
            //lodInfo.bForceUpdateTexture = true;
            continue;
        }

        if ( maxLod - minLod > 5 ) {
            maxLod = 5;
        }
    }

    MinViewLod = minLod;
    MaxViewLod = maxLod;

    UpdateTextures();
    AddPatches( TerrainMesh, ViewFrustum );
}

void ATerrainView::AddPatches( ATerrainMesh * TerrainMesh, BvFrustum const & ViewFrustum )
{
    STerrainLodInfo const & finestLod = LodInfo[MinViewLod];

    Int2 trimOffset;

    switch ( finestLod.InteriorTrim )
    {
    case INTERIOR_TOP_LEFT:
        trimOffset = Int2( 1, 1 );
        break;
    case INTERIOR_TOP_RIGHT:
        trimOffset = Int2( 0, 1 );
        break;
    case INTERIOR_BOTTOM_LEFT:
        trimOffset = Int2( 1, 0 );
        break;
    case INTERIOR_BOTTOM_RIGHT:
    default:
        trimOffset = Int2( 0, 0 );
        break;
    }

    trimOffset.X += BlockWidth;
    trimOffset.Y += BlockWidth;

    //
    // Draw interior L-shape for finest lod
    //

    STerrainPatchInstance & instance = AddInstance();
    instance.VertexScale = Int2( finestLod.GridScale, finestLod.LodIndex );
    instance.VertexTranslate.X = finestLod.Offset.X + trimOffset.X * finestLod.GridScale;
    instance.VertexTranslate.Y = finestLod.Offset.Y + trimOffset.Y * finestLod.GridScale;
    instance.TexcoordOffset = GetTexcoordOffset( finestLod );
    instance.QuadColor = Color4( 0.3f, 0.5f, 0.4f, 1.0f );
    AddPatchInstances( TerrainMesh->GetInteriorFinestPatch(), 1 );

    //
    // Draw blocks
    //

    int numBlocks = 0;
    int numCulledBlocks = 0;

    Int2 offset( trimOffset );

    if ( !CullBlock( ViewFrustum, finestLod, offset ) ) {
        AddBlock( finestLod, offset );
        numBlocks++;
    }
    else {
        numCulledBlocks++;
    }

    offset.X += BlockWidth;
    if ( !CullBlock( ViewFrustum, finestLod, offset ) ) {
        AddBlock( finestLod, offset );
        numBlocks++;
    }
    else {
        numCulledBlocks++;
    }

    offset.X = trimOffset.X;
    offset.Y += BlockWidth;
    if ( !CullBlock( ViewFrustum, finestLod, offset ) ) {
        AddBlock( finestLod, offset );
        numBlocks++;
    }
    else {
        numCulledBlocks++;
    }

    offset.X += BlockWidth;
    if ( !CullBlock( ViewFrustum, finestLod, offset ) ) {
        AddBlock( finestLod, offset );
        numBlocks++;
    }
    else {
        numCulledBlocks++;
    }

    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        offset.X = 0;
        offset.Y = 0;

        // 1
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 2
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += GapWidth;
        // 3
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 4
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X = 0;
        offset.Y += BlockWidth;

        // 5
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += BlockWidth;
        offset.X += GapWidth;
        offset.X += BlockWidth;
        // 6
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X = 0;
        offset.Y += BlockWidth;
        offset.Y += GapWidth;

        // 7
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += BlockWidth;
        offset.X += GapWidth;
        offset.X += BlockWidth;
        // 8
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X = 0;
        offset.Y += BlockWidth;

        // 9
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 10
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        offset.X += GapWidth;
        // 11
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
        offset.X += BlockWidth;
        // 12
        if ( !CullBlock( ViewFrustum, lodInfo, offset ) ) {
            AddBlock( lodInfo, offset );
            numBlocks++;
        }
        else {
            numCulledBlocks++;
        }
    }

    AddPatchInstances( TerrainMesh->GetBlockPatch(), numBlocks );

    //
    // Draw interior trims
    //

    int numTrims = 0;
    int totalTrims = 0;
    int numCulledTrims = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        if ( lodInfo.InteriorTrim == INTERIOR_TOP_LEFT ) {
            if ( !CullInteriorTrim( ViewFrustum, lodInfo ) ) {
                AddInteriorTopLeft( lodInfo );
                numTrims++;
            }
            else {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances( TerrainMesh->GetInteriorTLPatch(), numTrims );
    totalTrims += numTrims;

    numTrims = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        if ( lodInfo.InteriorTrim == INTERIOR_TOP_RIGHT ) {
            if ( !CullInteriorTrim( ViewFrustum, lodInfo ) ) {
                AddInteriorTopRight( lodInfo );
                numTrims++;
            }
            else {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances( TerrainMesh->GetInteriorTRPatch(), numTrims );
    totalTrims += numTrims;

    numTrims = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        if ( lodInfo.InteriorTrim == INTERIOR_BOTTOM_LEFT ) {
            if ( !CullInteriorTrim( ViewFrustum, lodInfo ) ) {
                AddInteriorBottomLeft( lodInfo );
                numTrims++;
            }
            else {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances( TerrainMesh->GetInteriorBLPatch(), numTrims );
    totalTrims += numTrims;

    numTrims = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        if ( lodInfo.InteriorTrim == INTERIOR_BOTTOM_RIGHT ) {
            if ( !CullInteriorTrim( ViewFrustum, lodInfo ) ) {
                AddInteriorBottomRight( lodInfo );
                numTrims++;
            }
            else {
                numCulledTrims++;
            }
        }
    }
    AddPatchInstances( TerrainMesh->GetInteriorBRPatch(), numTrims );
    totalTrims += numTrims;

    //
    // Draw vertical gaps
    //

    int numVertGaps = 0;
    int numCulledGaps = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        offset.X = BlockWidth * 2;
        offset.Y = 0;
        if ( !CullGapV( ViewFrustum, lodInfo, offset ) ) {
            AddGapV( lodInfo, offset );
            numVertGaps++;
        }
        else {
            numCulledGaps++;
        }
        offset.Y += (BlockWidth * 3 + GapWidth);
        if ( !CullGapV( ViewFrustum, lodInfo, offset ) ) {
            AddGapV( lodInfo, offset );
            numVertGaps++;
        }
        else {
            numCulledGaps++;
        }
    }
    AddPatchInstances( TerrainMesh->GetVertGapPatch(), numVertGaps );

    //
    // Draw horizontal gaps
    //

    int numHorGaps = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        offset.X = 0;
        offset.Y = BlockWidth * 2;
        if ( !CullGapH( ViewFrustum, lodInfo, offset ) ) {
            AddGapH( lodInfo, offset );
            numHorGaps++;
        }
        else {
            numCulledGaps++;
        }
        offset.X += (BlockWidth * 3 + GapWidth);
        if ( !CullGapH( ViewFrustum, lodInfo, offset ) ) {
            AddGapH( lodInfo, offset );
            numHorGaps++;
        }
        else {
            numCulledGaps++;
        }
    }
    AddPatchInstances( TerrainMesh->GetHorGapPatch(), numHorGaps );

    //
    // Draw crack strips
    //

    int numCrackStrips = 0;
    for ( int lod = MinViewLod ; lod <= MaxViewLod - 1 ; lod++ ) {
        STerrainLodInfo const & lodInfo = LodInfo[lod];

        AddCrackLines( lodInfo ); numCrackStrips++;
    }
    AddPatchInstances( TerrainMesh->GetCrackPatch(), numCrackStrips );

#if 0
    GLogger.Printf( "Blocks %d/%d, gaps %d/%d, trims %d/%d, cracks %d/%d, total culled %d, total instances %d(%d)\n",
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

void ATerrainView::UpdateRect( STerrainLodInfo const & Lod, STerrainLodInfo const & CoarserLod, int MinX, int MaxX, int MinY, int MaxY )
{
    Int2 texelWorldPos;
    float h[4];
    Float3 n;

    const float InvGridSizeCoarse = 1.0f / CoarserLod.GridScale;

    // TODO: Move this to GPU
    for ( int y = MinY ; y < MaxY ; y++ ) {
        for ( int x = MinX ; x < MaxX ; x++ ) {
            int wrapX = x & TextureWrapMask;
            int wrapY = y & TextureWrapMask;

            // from texture space to world space
            texelWorldPos.X = ( x - Lod.TextureOffset.X ) * Lod.GridScale + Lod.Offset.X;
            texelWorldPos.Y = ( y - Lod.TextureOffset.Y ) * Lod.GridScale + Lod.Offset.Y;

            AN_ASSERT( wrapX >= 0 && wrapY >= 0 );
            AN_ASSERT( wrapX <= TextureSize-1 && wrapY <= TextureSize-1 );

            int sampleLod = Lod.LodIndex;

            Float2 & heightMap = Lod.HeightMap[ wrapY * TextureSize + wrapX ];
            
            heightMap.X = Terrain->Height( texelWorldPos.X, texelWorldPos.Y, sampleLod );

            const int texelStep = Lod.GridScale;
            h[0] = Terrain->Height(texelWorldPos.X,           texelWorldPos.Y-texelStep, sampleLod );
            h[1] = Terrain->Height(texelWorldPos.X-texelStep, texelWorldPos.Y, sampleLod );
            h[2] = Terrain->Height(texelWorldPos.X+texelStep, texelWorldPos.Y, sampleLod );
            h[3] = Terrain->Height(texelWorldPos.X,           texelWorldPos.Y+texelStep, sampleLod );

            // normal = tangent ^ binormal
            // correct tangent t = cross( binormal, normal );
            //Float3 n = Math::Cross( Float3( 0.0, h[3] - h[0], 2.0f * texelStep ),
            //                        Float3( 2.0f * texelStep, h[2] - h[1], 0.0 ) );

            n.X = h[1] - h[2];
            n.Y = 2 * texelStep;
            n.Z = h[0] - h[3];
            //n.NormalizeSelf();
            float invLength = Math::RSqrt( n.X*n.X + n.Y*n.Y + n.Z*n.Z );
            n.X *= invLength;
            n.Z *= invLength;

            byte * normal = &Lod.NormalMap[ (wrapY * TextureSize + wrapX)*4 ];
            normal[0] = n.X * 127.5f + 127.5f;
            normal[1] = n.Z * 127.5f + 127.5f;

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

            float fx = Math::Fract( float( ofsX ) * InvGridSizeCoarse );
            float fy = Math::Fract( float( ofsY ) * InvGridSizeCoarse );

            h[0] = CoarserLod.HeightMap[wrapY * TextureSize + wrapX].X;
            h[1] = CoarserLod.HeightMap[wrapY * TextureSize + wrapX2].X;
            h[2] = CoarserLod.HeightMap[wrapY2 * TextureSize + wrapX2].X;
            h[3] = CoarserLod.HeightMap[wrapY2 * TextureSize + wrapX].X;

            heightMap.Y = Math::Bilerp(h[0],h[1],h[3],h[2],Float2(fx,fy));

            //float zf = floor( zf_zd );
            //float zd = zf_zd / 512 + 256;

            byte * n0 = &CoarserLod.NormalMap[(wrapY * TextureSize + wrapX)*4];
            byte * n1 = &CoarserLod.NormalMap[(wrapY * TextureSize + wrapX2)*4];
            byte * n2 = &CoarserLod.NormalMap[(wrapY2 * TextureSize + wrapX2)*4];
            byte * n3 = &CoarserLod.NormalMap[(wrapY2 * TextureSize + wrapX)*4];

            normal[2] = Math::Clamp( Math::Bilerp( float( n0[0] ), float( n1[0] ), float( n3[0] ), float( n2[0] ), Float2( fx, fy ) ), 0.0f, 255.0f );
            normal[3] = Math::Clamp( Math::Bilerp( float( n0[1] ), float( n1[1] ), float( n3[1] ), float( n2[1] ), Float2( fx, fy ) ), 0.0f, 255.0f );

            //heightMap.Y = CoarserLod.HeightMap[wrapY * TextureSize + wrapX].X;
        }
    }
}

void ATerrainView::UpdateTextures()
{
    const int count = TextureSize * TextureSize;

    for ( int lod = MaxViewLod ; lod >= MinViewLod ; lod-- ) {
        STerrainLodInfo & lodInfo = LodInfo[lod];
        STerrainLodInfo & coarserLodInfo = LodInfo[lod < MaxViewLod ? lod + 1 : lod];

        Int2 DeltaMove;
        DeltaMove.X = lodInfo.TextureOffset.X - lodInfo.PrevTextureOffset.X;
        DeltaMove.Y = lodInfo.TextureOffset.Y - lodInfo.PrevTextureOffset.Y;

        lodInfo.PrevTextureOffset = lodInfo.TextureOffset;

        int Min[2] = {0,0};
        int Max[2] = {0,0};

        for ( int i = 0 ; i < 2 ; i++ ) {
            if ( DeltaMove[i] < 0 )
            {
                Max[i] = lodInfo.TextureOffset[i] - DeltaMove[i];
                Min[i] = lodInfo.TextureOffset[i];
            }
            else if ( DeltaMove[i] > 0 )
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

        if ( Math::Abs( DeltaMove.X ) >= TextureSize || Math::Abs( DeltaMove.Y ) >= TextureSize || lodInfo.bForceUpdateTexture ) {

            lodInfo.bForceUpdateTexture = false;

            // Update whole texture
            UpdateRect( lodInfo,
                        coarserLodInfo,
                        lodInfo.TextureOffset.X, lodInfo.TextureOffset.X + TextureSize,
                        lodInfo.TextureOffset.Y, lodInfo.TextureOffset.Y + TextureSize );

            bUpdateToGPU = true;
        }
        else {
            if ( MinY != MaxY ) {
                UpdateRect( lodInfo, coarserLodInfo, lodInfo.TextureOffset.X, lodInfo.TextureOffset.X + TextureSize, MinY, MaxY );

                bUpdateToGPU = true;
            }
            if ( MinX != MaxX ) {
                UpdateRect( lodInfo, coarserLodInfo, MinX, MaxX, lodInfo.TextureOffset.Y, lodInfo.TextureOffset.Y + TextureSize );

                bUpdateToGPU = true;
            }
        }

        if ( bUpdateToGPU ) {
            RenderCore::STextureRect rect;

            rect.Offset.MipLevel = 0;
            rect.Offset.X = 0;
            rect.Offset.Y = 0;
            rect.Offset.Z = lod;
            rect.Dimension.X = TextureSize;
            rect.Dimension.Y = TextureSize;
            rect.Dimension.Z = 1;

            lodInfo.MinH = 99999;
            lodInfo.MaxH = -99999;

            // TODO: Optimize this: precompute low resolution grid of heightmap pages with
            // minimum and maximum height
            for ( int i = 0 ; i < count ; i+=3 ) {
                lodInfo.MinH = Math::Min( lodInfo.MinH, lodInfo.HeightMap[i].X );
                lodInfo.MaxH = Math::Max( lodInfo.MaxH, lodInfo.HeightMap[i].X );
            }
            const int Margin = 2;
            lodInfo.MinH -= Margin;
            lodInfo.MaxH += Margin;

            // TODO: Update only dirty regions
            ClipmapArray->WriteRect(rect, RenderCore::FORMAT_FLOAT2, count * sizeof(Float2), 4, lodInfo.HeightMap);

            NormalMapArray->WriteRect(rect, RenderCore::FORMAT_UBYTE4, count * 4, 4, lodInfo.NormalMap);
        }
    }
}

void ATerrainView::DrawDebug( ADebugRenderer * InRenderer, ATerrainMesh * TerrainMesh )
{
    AN_ASSERT( TerrainMesh->GetTextureSize() == TextureSize );

    TerrainRenderer = InRenderer;

    TerrainRenderer->SetColor( Color4( 1, 1, 1, 1 ) );
    for ( BvAxisAlignedBox & box : BoundingBoxes ) {
        TerrainRenderer->DrawAABB( box );
    }

#if 0

    STerrainVertex const * pVertices = TerrainMesh->GetVertexBufferCPU();
    unsigned short const * pIndices = TerrainMesh->GetIndexBufferCPU();

#if 0
    static int64_t currentDrawCall = 0;

    static int64_t prev = 0;
    int64_t cur = Platform::SysMilliseconds();
    int64_t delta = prev ? cur - prev : 0;
    prev = cur;

    int totalDrawcalls = 0;
    for ( RenderCore::SDrawIndexedIndirectCmd const & cmd : IndirectBuffer ) {
        totalDrawcalls += cmd.InstanceCount;
    }

    int cmdIndex = 0;
#endif

    int drawCall = 0;
    for ( RenderCore::SDrawIndexedIndirectCmd const & cmd : IndirectBuffer ) {

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

    //GLogger.Printf( "Drawcalls %d\n", IndirectBuffer.Size() );

    

#endif
}

void ATerrainView::DrawIndexedTriStrip( STerrainVertex const * Vertices, unsigned short const * Indices, int IndexCount )
{
    if ( IndexCount < 3 ) {
        return;
    }

    STerrainVertex v[3];

    int i = 0;

    int t = 0;
    v[t++] = Vertices[Indices[i++]];
    v[t++] = Vertices[Indices[i++]];

    for ( ; i < IndexCount ; i++ )
    {
        if ( Indices[i] == RESET_INDEX ) { // reset index
            t = 0;
            i++;
            if ( i + 2 >= IndexCount ) {
                return;
            }
            v[t++] = Vertices[Indices[i++]];
            v[t++] = Vertices[Indices[i]];
            continue;
        }

        v[t%3] = Vertices[Indices[i]];

        STerrainVertex const & a = v[(t-2)%3];
        STerrainVertex const & b = v[(t-1)%3];
        STerrainVertex const & c = v[t%3];

        if ( t & 1 ) {
            DrawTerrainTriangle( c, b, a );
        }
        else {
            DrawTerrainTriangle( a, b, c );
        }

        t++;
    }
}

void ATerrainView::DrawTerrainTriangle( STerrainVertex const & a, STerrainVertex const & b, STerrainVertex const & c )
{
    Float3 v0 = VertexShader( a );
    Float3 v1 = VertexShader( b );
    Float3 v2 = VertexShader( c );

    const Float3 lightVec = Float3( 0.5f, 0.5f, -0.5f ).Normalized();
    Float3 n = Math::Cross( (v1 - v0), (v2 - v0 ) ).Normalized();
    float dp = Math::Max( Math::Dot( n, lightVec ), 0.1f );

    TerrainRenderer->SetDepthTest( true );
    TerrainRenderer->SetColor( Color4( pDrawCallUniformData->QuadColor.R*dp,
                                        pDrawCallUniformData->QuadColor.G*dp,
                                        pDrawCallUniformData->QuadColor.B*dp,
                                        1 ) );
    TerrainRenderer->DrawTriangle( v0, v1, v2, false );

    // Add little offset for zfighting
    v0.Y += 0.01f;
    v1.Y += 0.01f;
    v2.Y += 0.01f;
    TerrainRenderer->SetColor( Color4::White() );
    //TerrainRenderer->SetColor( pDrawCallUniformData->QuadColor );
    TerrainRenderer->DrawLine( v0, v1 );
    TerrainRenderer->DrawLine( v1, v2 );
    TerrainRenderer->DrawLine( v2, v0 );
}

Float3 ATerrainView::VertexShader( STerrainVertex const & v )
{
    Float3 result;
    Int2 texelWorldPos;

    texelWorldPos.X = v.X * pDrawCallUniformData->VertexScale.X + pDrawCallUniformData->VertexTranslate.X;
    texelWorldPos.Y = v.Y * pDrawCallUniformData->VertexScale.X + pDrawCallUniformData->VertexTranslate.Y;

    int lodIndex = (int)pDrawCallUniformData->VertexScale.Y;
    Float2 * texture = LodInfo[lodIndex].HeightMap;

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
    texCoord.X = ( texelWorldPos.X + pDrawCallUniformData->TexcoordOffset.X ) / pDrawCallUniformData->VertexScale.X;
    texCoord.Y = ( texelWorldPos.Y + pDrawCallUniformData->TexcoordOffset.Y ) / pDrawCallUniformData->VertexScale.X;

    // wrap coordinates
    texCoord.X &= TextureWrapMask;
    texCoord.Y &= TextureWrapMask;

    AN_ASSERT( texCoord.X >= 0 && texCoord.Y >= 0 );
    AN_ASSERT( texCoord.X <= TextureSize-1 && texCoord.Y <= TextureSize-1 );

    result.Y = texture[texCoord.Y * TextureSize + texCoord.X].X;
    return result;
}

AN_CLASS_META( ATerrain )

ATerrain::ATerrain()
{
    HeightmapResolution = 4097;

    //Heightmap = (float *)GHeapMemory.Alloc( TERRAIN_SIZE*TERRAIN_SIZE*sizeof( float ) );
    HeightmapLods = Math::Log2( (uint32_t)(HeightmapResolution-1) ) + 1;

    Heightmap.Resize( HeightmapLods );

    for ( int i = 0 ; i < HeightmapLods ; i++ ) {
        int sz = 1 << (HeightmapLods - i - 1);

        sz++;

        Heightmap[i] = (float *)GHeapMemory.Alloc( sz*sz*sizeof( float ) );
    }

#if 0
    for ( int y = 0 ; y < HeightmapResolution ; y++ ) {
        for ( int x = 0 ; x < HeightmapResolution ; x++ ) {
            Heightmap[0][y*HeightmapResolution + x] = Terrain( Float2( x, y ) * 0.5f );
        }
    }

    AFileStream f;
    f.OpenWrite( "heightmap.dat" );
    f.WriteBuffer( Heightmap[0], HeightmapResolution*HeightmapResolution*sizeof( float ) );
#endif
#if 1
    AFileStream f;
    f.OpenRead( "heightmap.dat" );
    f.ReadBuffer( Heightmap[0], HeightmapResolution*HeightmapResolution*sizeof( float ) );
#endif
#if 0
    Platform::ZeroMem( Heightmap[0], HeightmapResolution*HeightmapResolution*sizeof( float ) );
    AImage image;
    if ( !image.Load( "swiss2min.png", nullptr, IMAGE_PF_R32F ) ) {
        GEngine->PostTerminateEvent();
    }
    for ( int y = 0 ; y < HeightmapResolution ; y++ ) {
        for ( int x = 0 ; x < HeightmapResolution ; x++ ) {
            int sx = (float)x / HeightmapResolution * image.GetWidth();
            int sy = (float)y / HeightmapResolution * image.GetHeight();
            Heightmap[0][y*HeightmapResolution + x] = ((float*)image.GetData())[sy * image.GetWidth() + sx] * 500;
        }
    }
#endif
    float h1,h2,h3,h4;
    int x, y;
    for ( int i = 1 ; i < HeightmapLods ; i++ ) {
        float * lod = Heightmap[i];
        float * srcLod = Heightmap[i - 1];

        int sz = 1 << (HeightmapLods - i - 1);
        int sz2 = 1 << (HeightmapLods - i);

        sz++;
        sz2++;

        for ( y = 0 ; y < sz - 1 ; y++ ) {
            int src_y = y << 1;
            for ( x = 0 ; x < sz - 1 ; x++ ) {
                int src_x = x << 1;
                h1 = srcLod[src_y * sz2 + src_x];
                h2 = srcLod[src_y * sz2 + src_x + 1];
                h3 = srcLod[(src_y + 1) * sz2 + src_x];
                h4 = srcLod[(src_y + 1) * sz2 + src_x + 1];

                lod[ y * sz + x ] = (h1+h2+h3+h4)*0.25f;
            }

            int src_x = x << 1;
            h1 = srcLod[src_y * sz2 + src_x];
            h2 = srcLod[(src_y + 1) * sz2 + src_x];
            lod[y * sz + x] = (h1+h2)*0.5f;
        }

        int src_y = y << 1;
        for ( x = 0 ; x < sz - 1 ; x++ ) {
            int src_x = x << 1;
            h1 = srcLod[src_y * sz2 + src_x];
            h2 = srcLod[src_y * sz2 + src_x + 1];

            lod[y * sz + x] = (h1+h2)*0.5f;
        }

        int src_x = x << 1;
        lod[y * sz + x] = srcLod[src_y * sz2 + src_x];
    }

    MinHeight = 99999;
    MaxHeight = -99999;

    for ( y = 0 ; y < HeightmapResolution ; y++ ) {
        for ( x = 0 ; x < HeightmapResolution ; x++ ) {
            float h = Heightmap[0][y*HeightmapResolution + x];
            MinHeight = Math::Min( MinHeight, h );
            MaxHeight = Math::Max( MaxHeight, h );
        }
    }

    HeightfieldShape = MakeUnique< btHeightfieldTerrainShape >( HeightmapResolution, HeightmapResolution, Heightmap[0], 1, MinHeight, MaxHeight, 1, PHY_FLOAT, false /* bFlipQuadEdges */ );
    HeightfieldShape->buildAccelerator();

    int halfResolution = HeightmapResolution >> 1;
    ClipMin.X = halfResolution;
    ClipMin.Y = halfResolution;
    ClipMax.X = halfResolution;
    ClipMax.Y = halfResolution;

    BoundingBox.Mins.X = -ClipMin.X;
    BoundingBox.Mins.Y = MinHeight;
    BoundingBox.Mins.Z = -ClipMin.Y;
    BoundingBox.Maxs.X = ClipMax.X;
    BoundingBox.Maxs.Y = MaxHeight;
    BoundingBox.Maxs.Z = ClipMax.Y;
}

ATerrain::~ATerrain()
{
    for ( int i = 0 ; i < HeightmapLods ; i++ ) {
        GHeapMemory.Free( Heightmap[i] );
    }
}

float ATerrain::Height( int X, int Z, int Lod ) const
{
    AN_ASSERT(Lod>=0&&Lod<HeightmapLods );
    int sampleX = X >> Lod;
    int sampleY = Z >> Lod;

    int lodResoultion = ( 1 << (HeightmapLods - Lod - 1) ) + 1;

    sampleX = Math::Clamp( sampleX + (lodResoultion>>1), 0, (lodResoultion-1) );
    sampleY = Math::Clamp( sampleY + (lodResoultion>>1), 0, (lodResoultion-1) );
    return Heightmap[Lod][sampleY*lodResoultion + sampleX];
}

bool ATerrain::Raycast( Float3 const & RayStart, Float3 const & RayDir, float Distance, bool bCullBackFace, TPodVector< STriangleHitResult > & HitResult ) const
{
    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        bool bCullBackFace;
        int IntersectionCount = 0;

        TPodVector< STriangleHitResult > * Result;

        void processTriangle( btVector3 * triangle, int partId, int triangleIndex ) override
        {
            Float3 v0 = btVectorToFloat3( triangle[0] );
            Float3 v1 = btVectorToFloat3( triangle[1] );
            Float3 v2 = btVectorToFloat3( triangle[2] );
            float d, u, v;
            if ( BvRayIntersectTriangle( RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                STriangleHitResult & hit = Result->Append();
                hit.Location = RayStart + RayDir * d;
                hit.Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
                hit.UV.X = u;
                hit.UV.Y = v;
                hit.Distance = d;
                hit.Indices[0] = 0;
                hit.Indices[1] = 0;
                hit.Indices[2] = 0;
                hit.Material = nullptr;
                IntersectionCount++;
            }
        }
    };

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if ( !BvRayIntersectBox( RayStart, invRayDir, BoundingBox, boxMin, boxMax ) || boxMin >= Distance ) {
        return false;
    }

    Float3 ShapeOffset = Float3( 0.0f, (MinHeight+MaxHeight)*0.5f, 0.0f );
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart = RayStartLocal;
    triangleRaycastCallback.RayDir = RayDir;
    triangleRaycastCallback.Result = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    HeightfieldShape->performRaycast( &triangleRaycastCallback, btVectorToFloat3( RayStartLocal ), btVectorToFloat3( RayStartLocal + RayDir * Distance ) );

    //GLogger.Printf( "triangleRaycastCallback.IntersectionCount %d\n", triangleRaycastCallback.IntersectionCount );

    for ( int i = HitResult.Size() - triangleRaycastCallback.IntersectionCount ; i < HitResult.Size() ; i++ ) {
        HitResult[i].Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

bool ATerrain::RaycastClosest( Float3 const & RayStart, Float3 const & RayDir, float Distance, bool bCullBackFace, STriangleHitResult & HitResult ) const
{
    #define FIRST_INTERSECTION_IS_CLOSEST

    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        float Distance;
        bool bCullBackFace;
        int IntersectionCount = 0;

        STriangleHitResult * Result;

        void processTriangle( btVector3 * triangle, int partId, int triangleIndex ) override
        {
#ifdef FIRST_INTERSECTION_IS_CLOSEST
            if ( IntersectionCount == 0 )
            {
                Float3 v0 = btVectorToFloat3( triangle[0] );
                Float3 v1 = btVectorToFloat3( triangle[1] );
                Float3 v2 = btVectorToFloat3( triangle[2] );
                float d, u, v;
                if ( BvRayIntersectTriangle( RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                    Result->Location = RayStart + RayDir * d;
                    Result->Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
                    Result->UV.X = u;
                    Result->UV.Y = v;
                    Result->Distance = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    Result->Material = nullptr;
                    IntersectionCount++;
                }
            }
#else
            Float3 v0 = btVectorToFloat3( triangle[0] );
            Float3 v1 = btVectorToFloat3( triangle[1] );
            Float3 v2 = btVectorToFloat3( triangle[2] );
            float d, u, v;
            if ( BvRayIntersectTriangle( RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                if ( d < Distance ) {
                    Result->Location = RayStart + RayDir * d;
                    Result->Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
                    Result->UV.X = u;
                    Result->UV.Y = v;
                    Result->Distance = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    Result->Material = nullptr;
                    Distance = d;
                    IntersectionCount++;
                }
            }
#endif
        }
    };

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if ( !BvRayIntersectBox( RayStart, invRayDir, BoundingBox, boxMin, boxMax ) || boxMin >= Distance ) {
        return false;
    }

    Float3 ShapeOffset = Float3( 0.0f, (MinHeight+MaxHeight)*0.5f, 0.0f );
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart = RayStartLocal;
    triangleRaycastCallback.RayDir = RayDir;
    triangleRaycastCallback.Distance = Distance;
    triangleRaycastCallback.Result = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    HeightfieldShape->performRaycast( &triangleRaycastCallback, btVectorToFloat3( RayStartLocal ), btVectorToFloat3( RayStartLocal + RayDir * Distance ) );

    //GLogger.Printf( "triangleRaycastCallback.IntersectionCount %d\n", triangleRaycastCallback.IntersectionCount );

    if ( triangleRaycastCallback.IntersectionCount > 0 ) {
        HitResult.Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

bool ATerrain::GetTriangleVertices( float X, float Z, Float3 & V0, Float3 & V1, Float3 & V2 ) const
{
    float minX = Math::Floor( X );
    float minZ = Math::Floor( Z );

    int quadX = minX + (HeightmapResolution>>1);
    int quadZ = minZ + (HeightmapResolution>>1);

    if ( quadX < 0 || quadX >= HeightmapResolution-1 ) {
        return false;
    }
    if ( quadZ < 0 || quadZ >= HeightmapResolution-1 ) {
        return false;
    }

    /*

    h0       h1
    +--------+
    |        |
    |        |
    |        |
    +--------+
    h3       h2

    */
    float h0 = Heightmap[0][quadZ * HeightmapResolution + quadX];
    float h1 = Heightmap[0][quadZ * HeightmapResolution + quadX + 1];
    float h2 = Heightmap[0][(quadZ + 1) * HeightmapResolution + quadX + 1];
    float h3 = Heightmap[0][(quadZ + 1) * HeightmapResolution + quadX];

    float maxX = minX + 1.0f;
    float maxZ = minZ + 1.0f;

    float fractX = X - minX;
    float fractZ = Z - minZ;

    if ( fractZ < 1.0f - fractX ) {
        V0.X = minX;
        V0.Y = h0;
        V0.Z = minZ;

        V1.X = minX;
        V1.Y = h3;
        V1.Z = maxZ;

        V2.X = maxX;
        V2.Y = h1;
        V2.Z = minZ;
    }
    else {
        V0.X = minX;
        V0.Y = h3;
        V0.Z = maxZ;

        V1.X = maxX;
        V1.Y = h2;
        V1.Z = maxZ;

        V2.X = maxX;
        V2.Y = h1;
        V2.Z = minZ;
    }

    return true;
}

bool ATerrain::GetNormal( float X, float Z, Float3 & Normal ) const
{
    Float3 v0, v1, v2;
    if ( !GetTriangleVertices( X, Z, v0, v1, v2 ) ) {
        return false;
    }

    Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
    return true;
}

bool ATerrain::GetTexcoord( float X, float Z, Float2 & Texcoord ) const
{
    const float invResolution = 1.0f / (HeightmapResolution - 1);

    Texcoord.X = Math::Clamp( X * invResolution + 0.5f, 0.0f, 1.0f );
    Texcoord.Y = Math::Clamp( Z * invResolution + 0.5f, 0.0f, 1.0f );

    return true;
}

bool ATerrain::GetTerrainTriangle( float X, float Z, STerrainTriangle & Triangle ) const
{
    if ( !GetTriangleVertices( X, Z, Triangle.Vertices[0], Triangle.Vertices[1], Triangle.Vertices[2] ) ) {
        return false;
    }

    Triangle.Normal = Math::Cross( Triangle.Vertices[1] - Triangle.Vertices[0],
                                   Triangle.Vertices[2] - Triangle.Vertices[0] );
    Triangle.Normal.NormalizeSelf();

    GetTexcoord( X, Z, Triangle.Texcoord );

    return true;
}
