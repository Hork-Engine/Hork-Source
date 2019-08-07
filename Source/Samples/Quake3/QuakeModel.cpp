/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "QuakeModel.h"

#include <Engine/Core/Public/Logger.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/World/Public/Level.h>

AN_BEGIN_CLASS_META( FQuakeBSP )
AN_END_CLASS_META()

static constexpr float FromQuakeScale = 1.0f/32.0f;

AN_FORCEINLINE void ConvertFromQuakeCoord( Float3 & _Coord ) {
    _Coord *= FromQuakeScale;
    FCore::SwapArgs( _Coord.Y, _Coord.Z );
    _Coord.X = -_Coord.X;
}

AN_FORCEINLINE void ConvertFromQuakeNormal( Float3 & _Normal ) {
    FCore::SwapArgs( _Normal.Y, _Normal.Z );
    _Normal.X = -_Normal.X;
}

AN_FORCEINLINE void FixupBoundingBox( Float3 & _Mins, Float3 & _Maxs ) {
    if ( _Mins.X > _Maxs.X ) {
        FCore::SwapArgs( _Mins.X, _Maxs.X );
    }
    if ( _Mins.Y > _Maxs.Y ) {
        FCore::SwapArgs( _Mins.Y, _Maxs.Y );
    }
    if ( _Mins.Z > _Maxs.Z ) {
        FCore::SwapArgs( _Mins.Z, _Maxs.Z );
    }
}

#define	BLOCK_WIDTH     128
#define	BLOCK_HEIGHT    128

#define LIGHTMAP_BYTES  3

FQuakeBSP::~FQuakeBSP() {
    Purge();
}

bool FQuakeBSP::FromData( FLevel * _Level, const byte * _Data ) {
    typedef struct {
        int32_t id;
        int32_t version;
        QBSPEntry  entities;
        QBSPEntry  shaders;
        QBSPEntry  planes;
        QBSPEntry  nodes;
        QBSPEntry  leafs;
        QBSPEntry  lface;
        QBSPEntry  lbrush;
        QBSPEntry  models;
        QBSPEntry  brush;
        QBSPEntry  brush_sides;
        QBSPEntry  vertices;
        QBSPEntry  indices;
        QBSPEntry  fog;
        QBSPEntry  faces;
        QBSPEntry  lightmaps;
        QBSPEntry  lightgrid;
        QBSPEntry  visilist;
    } QHeader;

    QHeader const * header;

    header = ( QHeader const * )_Data;

    if ( header->id != (*(int32_t *)"IBSP") || ( header->version != 46 && header->version != 47 ) ) {
        return false;
    }

    Purge();

    BSP.NumVisClusters = *( int * )( _Data + header->visilist.offset );
    int visRowSize = *( int * )( _Data + header->visilist.offset + sizeof( int ) );
    BSP.bCompressedVisData = false;

    if ( BSP.Visdata ) {
        DeallocateBufferData( BSP.Visdata );
    }
    BSP.Visdata = ( byte * )AllocateBufferData( header->visilist.size - sizeof( int )*2 );
    memcpy( BSP.Visdata, _Data + header->visilist.offset + sizeof( int )*2, header->visilist.size - sizeof( int )*2 );

    //ReadEntities
    //ReadShaders
    ReadLightmaps( _Level, _Data, header->lightmaps );
    ReadPlanes( _Level, _Data, header->planes );
    ReadFaces( _Level, _Data, header->vertices, header->indices, header->shaders, header->faces );
    ReadLFaces( _Level, _Data, header->lface );
    ReadLeafs( _Level, _Data, header->leafs, visRowSize );
    ReadNodes( _Level, _Data, header->nodes );
    //ReadLBrushes
    //ReadBrushSides
    //ReadBrushes
    //ReadModels


#if 0


    SrcVertices = (Float3 const * )( _Data + header->vertices.offset );
    Edges = (QEdge const *)( _Data + header->edges.offset );
    ledges = (int const *)( _Data + header->ledges.offset );

    _Level->SetLightData( _Data + header->lightmaps.offset, header->lightmaps.size );

    if ( BSP.Visdata ) {
        DeallocateBufferData( BSP.Visdata );
    }
    BSP.Visdata = (byte *)AllocateBufferData( header->visilist.size );
    memcpy( BSP.Visdata, _Data + header->visilist.offset, header->visilist.size );
    BSP.bCompressedVisData = true;

    ReadTextures( _Data, _Palette, header->miptex );
    ReadPlanes( _Level, _Data, header->planes );
    ReadTexInfos( _Data, header->texinfo );
    ReadFaces( _Level, _Data, header->faces );
    ReadLFaces( _Level, _Data, header->lface );
    ReadLeafs( _Level, _Data, header->leafs );
    ReadNodes( _Level, _Data, header->nodes );
    ReadClipnodes( _Data, header->clipnodes );
    ReadEntities( _Data, header->entities );
    ReadModels( _Data, header->models );

    TexInfos.Free();
#endif

    GLogger.Printf( "texcount %d lightmaps %d leafs %d leafscount %d\n",
                    Textures.Size(),
                    _Level->Lightmaps.Size(),
                    BSP.Leafs.Size(),
                    LeafsCount );

    return true;
}

void FQuakeBSP::Purge() {
    for ( FTexture * texture : Textures ) {
        if ( texture ) {
            texture->RemoveRef();
        }
    }

    Textures.Free();
    LightmapGroups.Free();
//    Entities.Free();
//    EntitiesString.Free();
}

void FQuakeBSP::ReadLightmaps( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    int numLightBytes = _Entry.size;

    #define	MAX_MAP_LIGHTING 0x800000

    if ( numLightBytes < 1 || numLightBytes >= MAX_MAP_LIGHTING ) {
        GLogger.Print( "FQuakeBSP::ReadLightmaps: invalid lightmap\n" );
        return;
    }

    const int BANK_SIZE = BLOCK_WIDTH*BLOCK_HEIGHT*LIGHTMAP_BYTES;
    int numLightmaps = numLightBytes / BANK_SIZE;

    _Level->SetLightData( _Data + _Entry.offset, _Entry.size );

    // Swap to bgr
    //for ( byte * pLightData = _Level->GetLightData() ; pLightData < &_Level->GetLightData()[ numLightBytes ] ; pLightData += LIGHTMAP_BYTES ) {
    //    FCore::SwapArgs( *pLightData, *( pLightData + 2 ) );
    //}

    // Create lightmaps
    _Level->ClearLightmaps();
    _Level->Lightmaps.ResizeInvalidate( numLightmaps );
    for ( int i = 0 ; i < _Level->Lightmaps.Size() ; i++ ) {
        _Level->Lightmaps[i] = NewObject< FTexture >();
        _Level->Lightmaps[i]->AddRef();
        //_Level->Lightmaps[i]->Initialize2D( TEXTURE_PF_R32F, 1, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 1 );
        _Level->Lightmaps[i]->Initialize2D( TEXTURE_PF_BGR16F, 1, BLOCK_WIDTH, BLOCK_HEIGHT, 1 );

        UShort * pPixels = (UShort*)_Level->Lightmaps[i]->WriteTextureData( 0, 0, 0, BLOCK_WIDTH, BLOCK_HEIGHT, 0 );
        if ( pPixels ) {
            const byte * src = _Level->GetLightData() + i*BANK_SIZE;

            const float scale = 1.0f / 255.0f;
            const float brightness = 16.0f;
            float t[3];
            
            for ( int p = 0 ; p < BANK_SIZE ; p += LIGHTMAP_BYTES, pPixels += 3, src += LIGHTMAP_BYTES ) {
                t[0] = ConvertToRGB( src[2] * scale ) * brightness;
                t[1] = ConvertToRGB( src[1] * scale ) * brightness;
                t[2] = ConvertToRGB( src[0] * scale ) * brightness;
                pPixels[0] = Float::FloatToHalf( *reinterpret_cast< const uint32_t * >( &t[0] ) );
                pPixels[1] = Float::FloatToHalf( *reinterpret_cast< const uint32_t * >( &t[1] ) );
                pPixels[2] = Float::FloatToHalf( *reinterpret_cast< const uint32_t * >( &t[2] ) );
            }
        }
    }
}

void FQuakeBSP::ReadPlanes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    struct QPlane {
        Float3 normal;
        float dist;
    };

    QPlane const * planes = (QPlane const *)( _Data + _Entry.offset );
    int numPlanes = _Entry.size / sizeof( QPlane );

    BSP.Planes.ResizeInvalidate( numPlanes );
    BSP.Planes.ZeroMem();

    QPlane const * in = planes;
    FBinarySpacePlane * out = BSP.Planes.ToPtr();

    for ( int i = 0 ; i < numPlanes ; i++, in++, out++ ) {
        out->Normal = in->normal;
        ConvertFromQuakeNormal( out->Normal );
        out->D = -in->dist * FromQuakeScale;
        out->Type = out->Normal.NormalAxialType();
    }
}

#if 0
void FQuakeBSP::ReadFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    struct QFace {
        short       planenum;
        short       side;
        int         firstedge;
        short       numedges;
        short       texinfo;
        byte        styles[MAX_SURFACE_LIGHTMAPS];
        int         lightofs;
    };

    QFace const * faces = (QFace const *)( _Data + _Entry.offset );
    int NumFaces = _Entry.size / sizeof( QFace );

    BSP.Surfaces.ResizeInvalidate( NumFaces );

    QFace const * in = faces;
    FSurfaceDef * out = BSP.Surfaces.ToPtr();

    int numWorldVertices = 0;
    int numWorldIndices = 0;
    int numLightmaps = 0;

    int firstVertex = 0;
    int firstIndex = 0;

    for ( int surfnum = 0 ; surfnum < NumFaces ; surfnum++, in++ ) {
        const int numTriangles = in->numedges - 2;
        numWorldIndices += numTriangles * 3;
        numWorldVertices += in->numedges;
    }

    in = faces;

    BSP.Vertices.ResizeInvalidate( numWorldVertices );
    BSP.LightmapVerts.ResizeInvalidate( numWorldVertices );
    BSP.Indices.ResizeInvalidate( numWorldIndices );

    FMeshVertex * pVertex = BSP.Vertices.ToPtr();
    FMeshLightmapUV * pLightmapUV = BSP.LightmapVerts.ToPtr();
    unsigned int * pIndex = BSP.Indices.ToPtr();

    LightmapBlockAllocator.Clear();

    for ( int surfnum = 0 ; surfnum < NumFaces ; surfnum++, in++, out++ ) {
        out->Bounds.Clear();

        const int numTriangles = in->numedges - 2;
        const int numIndices = numTriangles * 3;

        for ( int k = 0 ; k < numTriangles ; k++ ) {
            *pIndex++ = /*firstVertex + */0;
            *pIndex++ = /*firstVertex + */k + 1;
            *pIndex++ = /*firstVertex + */k + 2;
        }

        //out->dlightframe = 0;
        //out->dlightbits = 0;
        int lightmapBlock;

        //out->Flags = 0;

        //if (in->side)
        // out->Flags |= SURF_PLANEBACK;

        QTexinfoExt * tex = TexInfos.ToPtr() +  (in->texinfo);

        short texturemins[2];
        short extents[2];
        float mins[ 2 ], maxs[ 2 ], val;
        Float3 const * v;

        // calc texture extents and texturemins
        mins[ 0 ] = mins[ 1 ] = 999999;
        maxs[ 0 ] = maxs[ 1 ] = -99999;
        for ( int i = 0 ; i < in->numedges ; i++ ) {
            int e = ledges[ in->firstedge + i ];
            if ( e >= 0 )
                v = &SrcVertices[ Edges[ e ].vertex0 ];
            else
                v = &SrcVertices[ Edges[ -e ].vertex1 ];
            for ( int j = 0 ; j < 2 ; j++ ) {
                val = v->X * tex->vecs[ j ][ 0 ] + v->Y * tex->vecs[ j ][ 1 ] + v->Z * tex->vecs[ j ][ 2 ] + tex->vecs[ j ][ 3 ];
                if ( val < mins[ j ] )
                    mins[ j ] = val;
                if ( val > maxs[ j ] )
                    maxs[ j ] = val;
            }
        }
        for ( int i = 0 ; i < 2 ; i++ ) {
            int bmins = floor( mins[ i ] / 16 );
            int bmaxs = ceil( maxs[ i ] / 16 );
            texturemins[ i ] = bmins * 16;
            extents[ i ] = ( bmaxs - bmins ) * 16;
        }

        for ( int i = 0 ; i < MAX_SURFACE_LIGHTMAPS ; i++ ) {
            out->LightStyles[ i ] = in->styles[ i ];
        }

//        if ( out->styles[ 0 ] != 0 && out->styles[ 0 ] != 255&& out->styles[ 0 ] != 2 ) {
//            GLogger("Breakpoint\n");
//        }

        out->LightDataOffset = in->lightofs;

        //bool bSky = false;
        //bool bWater = false;
        bool bHasLightmap = true;
        int texWidth = 32;
        int texHeight = 32;

        FTexture * texture = Textures[ tex->textureIndex ].Object;
        if ( texture ) {
            const char * texName = texture->GetResourcePath();

            texWidth = texture->GetWidth();
            texHeight = texture->GetHeight();

            if ( texture ) {
                if ( !FString::CmpN( texName, "sky", 3 ) ) {
                    bHasLightmap = false;
                    //bSky = true;
                } else if ( texName[ 0 ] == '*' ) {
                    bHasLightmap = false;
                    //bWater = true;
                    for ( int i = 0 ; i < 2 ; i++ ) {
                        extents[ i ] = 16384;
                        texturemins[ i ] = -8192;
                    }
                }
            }
        }

        out->LightmapOffsetX = 0;
        out->LightmapOffsetY = 0;

        out->LightmapWidth = ( extents[ 0 ] >> 4 ) + 1;
        out->LightmapHeight = ( extents[ 1 ] >> 4 ) + 1;

        if ( bHasLightmap ) {
            bHasLightmap = LightmapBlockAllocator.Alloc( out->LightmapWidth, out->LightmapHeight, &out->LightmapOffsetX, &out->LightmapOffsetY, &lightmapBlock );
            if ( bHasLightmap ) {
                numLightmaps = FMath::Max( numLightmaps, lightmapBlock + 1 );
            }
        }

        Float3 center(0);
        for ( int edgeIndex = 0 ; edgeIndex < in->numedges ; edgeIndex++, pVertex++, pLightmapUV++ ) {
            int e = ledges[ in->firstedge + in->numedges - edgeIndex - 1 ];
            if ( e >= 0 )
                pVertex->Position = SrcVertices[ Edges[ e ].vertex0 ];
            else
                pVertex->Position = SrcVertices[ Edges[ -e ].vertex1 ];

            pVertex->TexCoord.X = FMath::Dot( pVertex->Position, *( Float3 * )&tex->vecs[ 0 ][ 0 ] ) + tex->vecs[ 0 ][ 3 ];
            pVertex->TexCoord.Y = FMath::Dot( pVertex->Position, *( Float3 * )&tex->vecs[ 1 ][ 0 ] ) + tex->vecs[ 1 ][ 3 ];

            if ( texture ) {
                pVertex->TexCoord.X /= texWidth;
                pVertex->TexCoord.Y /= texHeight;
            }

            // lightmap texture coordinates
            float s,t;
            s = FMath::Dot( pVertex->Position, *( Float3 * )&tex->vecs[ 0 ][ 0 ] ) + tex->vecs[ 0 ][ 3 ];
            s -= texturemins[ 0 ];
            s += out->LightmapOffsetX * 16;
            s += 8;
            s /= LightmapBlockAllocator.BLOCK_WIDTH * 16;

            t = FMath::Dot( pVertex->Position, *( Float3 * )&tex->vecs[ 1 ][ 0 ] ) + tex->vecs[ 1 ][ 3 ];
            t -= texturemins[ 1 ];
            t += out->LightmapOffsetY * 16;
            t += 8;
            t /= LightmapBlockAllocator.BLOCK_HEIGHT * 16;

            pLightmapUV->TexCoord.X = s;
            pLightmapUV->TexCoord.Y = t;

            ConvertFromQuakeCoord( pVertex->Position );

            center += pVertex->Position;

            out->Bounds.AddPoint( pVertex->Position );
        }

        center *= ( 1.0 / in->numedges );

        out->Plane.Normal = FMath::Cross( pVertex[-2].Position - center, pVertex[-1].Position - center ).NormalizeFix();
        out->Plane.D = -FMath::Dot( pVertex[-2].Position, out->Plane.Normal );

        for ( int i = 0 ; i < in->numedges ; i++ ) {
            ( BSP.Vertices.ToPtr() + firstVertex + i )->Normal = -out->Plane.Normal;
        }

        out->FirstVertex = firstVertex;
        out->NumVertices = in->numedges;
        out->FirstIndex = firstIndex;
        out->NumIndices = numIndices;
        out->Type = SURF_PLANAR;

        numWorldIndices -= numIndices;

        firstVertex += in->numedges;
        firstIndex += numIndices;

        if ( bHasLightmap ) {
            const byte * samples = out->LightDataOffset != -1 ? _Level->GetLightData() + out->LightDataOffset : nullptr;

            AccumulateLight( AccumulatedLight, samples, out->LightStyles, out->LightmapWidth, out->LightmapHeight, _Level->GetLightData() == nullptr );

            LightmapBlockAllocator.CopySamples( out->LightmapOffsetX, out->LightmapOffsetY, lightmapBlock, out->LightmapWidth, out->LightmapHeight, AccumulatedLight );
        }

        out->LightmapGroup = GetLightmapGroup( tex->textureIndex, lightmapBlock );
    }

    AN_Assert( numWorldIndices == 0 );

    CalcTangentSpace( BSP.Vertices.ToPtr(), BSP.Vertices.Length(), BSP.Indices.ToPtr(), BSP.Indices.Length() );

    // Create lightmaps
    _Level->ClearLightmaps();
    _Level->Lightmaps.ResizeInvalidate( numLightmaps );
    for ( int i = 0 ; i < _Level->Lightmaps.Length() ; i++ ) {
        _Level->Lightmaps[i] = NewObject< FTexture >();
        _Level->Lightmaps[i]->AddRef();
        _Level->Lightmaps[i]->Initialize2D( TEXTURE_PF_R32F, 1, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 1 );

        void * pPixels = _Level->Lightmaps[i]->WriteTextureData( 0, 0, 0, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 0 );
        if ( pPixels ) {
            memcpy( pPixels, LightmapBlockAllocator.GetLightmapBlock( i ), LightmapBlockAllocator.GetLightmapBlockLength() );
        }
    }
}
#endif

FTexture * FQuakeBSP::LoadTexture( FArchive & _Pack, const char * _FileName ) {
    FMemoryStream stream;

    if ( !stream.OpenRead( _FileName, _Pack ) ) {
        return nullptr;
    }

    FImage img;

    if ( !img.LoadRawImage( stream, true, true ) ) {
        return nullptr;
    }

    FTexture * tx = NewObject< FTexture >();
    tx->SetName( _FileName );
    tx->InitializeFromImage( img );
    return tx;
}

FTexture * FQuakeBSP::LoadSky() {
    FImage img1, img2;

    if ( !img1.LoadRawImage( "textures/skies/killsky_2.jpg", true, true, 3 ) ) {
        return nullptr;
    }

    if ( !img2.LoadRawImage( "textures/skies/killsky_1.jpg", true, true, 3 ) ) {
        return nullptr;
    }

    if ( img1.Width != 256
        || img1.Height != 256
        || img2.Width != 256
        || img2.Height != 256 ) {
        return nullptr;
    }

    FTexture * tx = NewObject< FTexture >();
    tx->SetName( "sky" );

    tx->Initialize2D( TEXTURE_PF_BGR8_SRGB, 1, 256, 256, 2 );
    void *pLayer1 = tx->WriteTextureData( 0,0,0,256,256,0);
    if ( pLayer1 ) {
        memcpy( pLayer1, img1.pRawData, 256*256*3 );
    }
    void *pLayer2 = tx->WriteTextureData( 0,0,1,256,256,0);
    if ( pLayer2 ) {
        memcpy( pLayer2, img2.pRawData, 256*256*3 );
    }

    return tx;
}

static void SubdivideCurve_r( FMeshVertex * _Verts, FMeshLightmapUV * _VertsLM, int _VertexNum, int _Step, int _Level ) {
    int		next = _VertexNum + _Step;
    int		halfStep = _Step >> 1;

    if ( --_Level == 0 ) {
        {
        FMeshVertex a = FMeshVertex::Lerp( _Verts[ _VertexNum     ], _Verts[ next ] );
        FMeshVertex b = FMeshVertex::Lerp( _Verts[ next + _Step ], _Verts[ next ] );
        _Verts[ next ] = FMeshVertex::Lerp( a, b );
        }
        {
        FMeshLightmapUV a = FMeshLightmapUV::Lerp( _VertsLM[ _VertexNum     ], _VertsLM[ next ] );
        FMeshLightmapUV b = FMeshLightmapUV::Lerp( _VertsLM[ next + _Step ], _VertsLM[ next ] );
        _VertsLM[ next ] = FMeshLightmapUV::Lerp( a, b );
        }
        return;
    }

    _Verts[ _VertexNum + halfStep ] = FMeshVertex::Lerp( _Verts[ _VertexNum            ], _Verts[ next            ] );
    _Verts[ next + halfStep       ] = FMeshVertex::Lerp( _Verts[ next + _Step          ], _Verts[ next            ] );
    _Verts[ next                  ] = FMeshVertex::Lerp( _Verts[ _VertexNum + halfStep ], _Verts[ next + halfStep ] );

    _VertsLM[ _VertexNum + halfStep ] = FMeshLightmapUV::Lerp( _VertsLM[ _VertexNum            ], _VertsLM[ next            ] );
    _VertsLM[ next + halfStep       ] = FMeshLightmapUV::Lerp( _VertsLM[ next + _Step          ], _VertsLM[ next            ] );
    _VertsLM[ next                  ] = FMeshLightmapUV::Lerp( _VertsLM[ _VertexNum + halfStep ], _VertsLM[ next + halfStep ] );

    SubdivideCurve_r( _Verts, _VertsLM, _VertexNum, halfStep, _Level );
    SubdivideCurve_r( _Verts, _VertsLM, next, halfStep, _Level );
}

static bool IsColinear( Float3 const & v1, Float3 const & v2, Float3 const & v3 ) {
    if ( v1[0] == v2[0] && v1[0] == v3[0]
        && v1[1] == v2[1] && v1[1] == v3[1]
        && v1[2] == v2[2] && v1[2] == v3[2] ) {
        return true;
    }
    if ( ( v1[0] == v2[0] && v1[1] == v2[1] && v1[2]==v2[2] )
        || ( v2[0] == v3[0] && v2[1] == v3[1] && v2[2]==v3[2] )
        || ( v1[0] == v3[0] && v1[1] == v3[1] && v1[2]==v3[2] ) ) {
        return true;
    }
    if ( fabs((v2[1]-v1[1])*(v3[0]-v1[0])-(v3[1]-v1[1])*(v2[0]-v1[0])) < 0.0002
        && fabs((v2[2]-v1[2])*(v3[0]-v1[0])-(v3[2]-v1[2])*(v2[0]-v1[0])) < 0.0002
        && fabs((v2[2]-v1[2])*(v3[1]-v1[1])-(v3[2]-v1[2])*(v2[1]-v1[1])) < 0.0002 ) {
        return true;
    }
    return false;
}

static bool IsPlanarU( int _PatchWidth, int _PatchHeight, const FMeshVertex * _Verts ) {
    for ( int v = 0 ; v < _PatchHeight ; v++ ) {
        for ( int u = 0 ; u < _PatchWidth-2 ; u++ ) {
            if ( !IsColinear( _Verts[_PatchWidth*v+u].Position,
                              _Verts[_PatchWidth*v+(u+1)].Position,
                              _Verts[_PatchWidth*v+(u+2)].Position ) ) {
                return false;
            }
        }
    }
    return true;
}

static bool IsPlanarV( int _PatchWidth, int _PatchHeight, const FMeshVertex * _Verts ) {
    for ( int u = 0 ; u < _PatchWidth ; u++ ) {
        for ( int v = 0 ; v < _PatchHeight-2 ; v++ ) {
            if ( !IsColinear( _Verts[_PatchWidth*v+u].Position,
                              _Verts[_PatchWidth*(v+1)+u].Position,
                              _Verts[_PatchWidth*(v+2)+u].Position ) ) {
                return false;
            }
        }
    }
    return true;
}

static int CalcUSize( int _PatchWidth, int _PatchHeight, float _SubdivFactor, const FMeshVertex * _Verts ) {
    int		u, v;
    float	size = 0;
    float	inc;

    for ( v=0 ; v<_PatchHeight ; v++ ) {
        inc = 0;
        for ( u=0 ; u<_PatchWidth-1 ; u++ ) {
            const Float3 & v1 = _Verts[_PatchWidth*v+u+1].Position;
            const Float3 & v2 = _Verts[_PatchWidth*v+u].Position;

            inc += v1.Dist( v2 );
        }
        size = FMath::Max( size, inc );
    }

    return FMath::Log2((int)(size * _SubdivFactor/(_PatchWidth-1)));
}

static int CalcVSize( int _PatchWidth, int _PatchHeight, float _SubdivFactor, const FMeshVertex * _Verts ) {
    int		u, v;
    float	size = 0;
    float	inc;

    for ( u=0 ; u<_PatchWidth ; u++ ) {
        inc = 0;
        for ( v=0 ; v<_PatchHeight-1 ; v++ ) {
            const Float3 & v1 = _Verts[_PatchWidth*(v+1)+u].Position;
            const Float3 & v2 = _Verts[_PatchWidth*v+u].Position;

            inc += v1.Dist( v2 );
        }
        size = FMath::Max( size, inc );
    }

    return FMath::Log2((int)(size * _SubdivFactor/(_PatchHeight-1)));
}

void FQuakeBSP::ReadFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _VertexEntry, QBSPEntry const & _IndexEntry, QBSPEntry const & _ShaderEntry, QBSPEntry const & _FaceEntry ) {
    typedef enum {
        SURFTYPE_BAD,
        SURFTYPE_PLANAR,
        SURFTYPE_CURVE,
        SURFTYPE_MESH,
        SURFTYPE_FLARE
    } QSurfaceType;

    typedef struct {
        int			shaderNum;
        int			fogNum;
        int			surfaceType;

        int			firstVert;
        int			numVerts;

        int			firstIndex;
        int			numIndexes;

        int			lightmapNum;
        int			lightmapX;
        int         lightmapY;
        int			lightmapWidth;
        int         lightmapHeight;

        Float3		lightmapOrigin;
        Float3		lightmapVecs[3]; // for patches, [0] and [1] are lodbounds

        int			patchWidth;
        int			patchHeight;
    } QFace;

    typedef struct {
        Float3       Position;
        Float2       TexCoord;
        Float2       LightmapTexCoord;
        Float3       Normal;
        int          Color;
    } QVertex;

    typedef struct {
        char		shader[ 64 ];
        int			surfaceFlags;
        int			contentFlags;
    } QShader;

    QVertex * Vertices = (QVertex *)( _Data + _VertexEntry.offset );
    int NumVertices = _VertexEntry.size / sizeof( QVertex );

    unsigned int * Indices = (unsigned int *)( _Data + _IndexEntry.offset );
    int NumIndices = _IndexEntry.size / sizeof( unsigned int );

    QFace * Faces = (QFace *)( _Data + _FaceEntry.offset );
    int NumFaces = _FaceEntry.size / sizeof( QFace );

    QShader * Shaders = (QShader *)( _Data + _ShaderEntry.offset );
    int NumShaders = _ShaderEntry.size / sizeof( QShader );

    FArchive pack;

    pack.Open( PackName.ToConstChar() );

    FTexture * defaultTexture = NewObject< FTexture >();
    defaultTexture->SetName( "default" );
#if 0
    defaultTexture->Initialize2D( TEXTURE_PF_BGR8_SRGB, 1, 4, 4 );
    void * pPixels = defaultTexture->WriteTextureData( 0, 0, 0, 4, 4, 0 );
    if ( pPixels ) {
        memset( pPixels, 0xff, 4*4*3 );

        for ( int i = 0 ; i < 3 ; i++ ) {
            *((byte *)pPixels + 5*3+i) = 0;
            *((byte *)pPixels + 5*3+i) = 0;
            *((byte *)pPixels + 6*3+i) = 0;
            *((byte *)pPixels + 9*3+i) = 0;
            *((byte *)pPixels + 10*3+i) = 0;
        }
    }
#else
    defaultTexture->Initialize2D( TEXTURE_PF_BGR8_SRGB, 1, 1, 1 );
    void * pPixels = defaultTexture->WriteTextureData( 0, 0, 0, 1, 1, 0 );
    if ( pPixels ) {
        memset( pPixels, 0xff, 1*1*3 );
    }
#endif

    FTexture * skyTexture = LoadSky();
    if ( !skyTexture ) {
        skyTexture = defaultTexture;
    }

    Textures.ResizeInvalidate( NumShaders );
    for ( int i = 0 ; i < NumShaders ; i++ ) {

        if ( strstr( Shaders[i].shader, "cloud" )
            || strstr( Shaders[i].shader, "skie" )
            || strstr( Shaders[i].shader, "sky" ) ) {
            Textures[i] = skyTexture;
        } else {
            Textures[i] = LoadTexture( pack, ( FString( (const char*)Shaders[i].shader ) + ".jpg" ).ToConstChar() );
            if ( !Textures[i] ) {
                Textures[i] = LoadTexture( pack, ( FString( (const char*)Shaders[i].shader ) + ".tga" ).ToConstChar() );
                if ( !Textures[i] ) {
                    Textures[i] = LoadTexture( pack, ( FString( (const char*)Shaders[i].shader ) + ".png" ).ToConstChar() );
                    if ( !Textures[i] ) {
                        Textures[i] = defaultTexture;
                    }
                }
            }
        }
        Textures[i]->AddRef();
    }

    QVertex * pInputVert = Vertices;

    BSP.Vertices.ResizeInvalidate( NumVertices );
    BSP.LightmapVerts.ResizeInvalidate( NumVertices );
    BSP.VertexLight.ResizeInvalidate( NumVertices );

    FMeshVertex *pOutputVert = BSP.Vertices.ToPtr();
    FMeshLightmapUV *pOutputLM = BSP.LightmapVerts.ToPtr();
    FMeshVertexLight * pOutputVL = BSP.VertexLight.ToPtr();

    for ( int i = 0 ; i < NumVertices ; i++, pInputVert++, pOutputVert++, pOutputLM++, pOutputVL++ ) {
        pOutputVert->Position = pInputVert->Position;
        pOutputVert->TexCoord = pInputVert->TexCoord;
        pOutputVert->Normal = pInputVert->Normal;

        pOutputLM->TexCoord = pInputVert->LightmapTexCoord;
        pOutputVL->VertexLight = pInputVert->Color | 0xff000000;

        ConvertFromQuakeCoord( pOutputVert->Position );
        ConvertFromQuakeNormal( pOutputVert->Normal );
    }

    QFace * pInputFace = Faces;

    //BSP.Indices.ResizeInvalidate( NumIndices );
    //memcpy( BSP.Indices.ToPtr(), Indices, NumIndices*sizeof(unsigned int) );
    BSP.Indices.Reserve( NumIndices );
    for ( int SurfNum = 0 ; SurfNum < NumFaces ; SurfNum++, pInputFace++ ) {
        int FirstIndex = BSP.Indices.Size();

        if ( pInputFace->numIndexes > 0 ) {
            for ( int i = 0 ; i < pInputFace->numIndexes ; i++ ) {
                BSP.Indices.Append( Indices[ pInputFace->firstIndex + pInputFace->numIndexes - i - 1 ] );
            }
        }
        pInputFace->firstIndex = FirstIndex;
    }

    BSP.Surfaces.Resize( NumFaces );

    pInputFace = Faces;
    FSurfaceDef * pOutFace = BSP.Surfaces.ToPtr();

    BvAxisAlignedBox Bounds;
    ESurfaceType SurfaceType = SURF_TRISOUP;
    PlaneF SurfacePlane;
    for ( int SurfNum = 0 ; SurfNum < NumFaces ; SurfNum++, pInputFace++, pOutFace++ ) {
        //int LightmapTextureIndex = pInputFace->lightmapNum;

        int SurfFirstVertex = 0;
        int SurfVerticesCount = 0;
        int SurfFirstIndex = 0;
        int SurfIndicesCount = 0;
                SurfFirstVertex = pInputFace->firstVert;
                SurfVerticesCount = pInputFace->numVerts;
                //SurfFirstIndex = pInputFace->patchWidth;
                SurfFirstIndex = pInputFace->firstIndex;
                SurfIndicesCount = pInputFace->numIndexes;
        //QShader * pShader = Shaders + pInputFace->shaderNum;

        if ( ( unsigned )pInputFace->shaderNum >= NumShaders ) {
            GLogger.Print( "FQuakeBSP::ReadFaces: invalid shader num\n" );
        }

        switch ( pInputFace->surfaceType ) {
            case SURFTYPE_PLANAR:
            {
                SurfaceType = SURF_PLANAR;

                SurfFirstVertex = pInputFace->firstVert;
                SurfVerticesCount = pInputFace->numVerts;
                //SurfFirstIndex = pInputFace->patchWidth;
                SurfFirstIndex = pInputFace->firstIndex;
                SurfIndicesCount = pInputFace->numIndexes;

                SurfacePlane.Normal = pInputFace->lightmapVecs[ 2 ];
                ConvertFromQuakeNormal( SurfacePlane.Normal );
                SurfacePlane.D = -FMath::Dot( BSP.Vertices[ SurfFirstVertex + BSP.Indices[ SurfFirstIndex ] ].Position, SurfacePlane.Normal );

                break;
            }
            case SURFTYPE_CURVE:
            {
                SurfaceType = SURF_TRISOUP;

                //pOutFace->PatchWidth = pInputFace->patchWidth;
                //pOutFace->PatchHeight = pInputFace->patchHeight;

                const float SubdivFactor = 4;
                const int MaxSubdivs = 4;
                int u, v;

                int subdivX = CalcUSize( pInputFace->patchWidth, pInputFace->patchHeight, SubdivFactor, BSP.Vertices.ToPtr() + pInputFace->firstVert );
                int subdivY = CalcVSize( pInputFace->patchWidth, pInputFace->patchHeight, SubdivFactor, BSP.Vertices.ToPtr() + pInputFace->firstVert );
                subdivX = Int( subdivX ).Clamp( 1, MaxSubdivs );
                subdivY = Int( subdivY ).Clamp( 1, MaxSubdivs );
                //int subdivX = 4;
                //int subdivY = 4;
                int stepX = 1 << ( subdivX - 1 );
                int stepY = 1 << ( subdivY - 1 );
                int _StepX = FMath::Min( 1, 1 << subdivX );
                int _StepY = FMath::Min( 1, 1 << subdivY );
                int sizeX = ( pInputFace->patchWidth - 1 ) * stepX + 1;
                int sizeY = ( pInputFace->patchHeight - 1 ) * stepY + 1;

                SurfFirstVertex = BSP.Vertices.Size();
                SurfFirstIndex = BSP.Indices.Size();

                enum EPlanarType {
                    NO_PLANAR,
                    PLANAR_U,
                    PLANAR_V
                };
                EPlanarType planarType;
                if ( IsPlanarU( pInputFace->patchWidth, pInputFace->patchHeight, BSP.Vertices.ToPtr() + pInputFace->firstVert ) ) {
                    SurfVerticesCount = 2 * sizeY;
                    SurfIndicesCount = ( ( sizeY - 1 ) / _StepY ) * 6;

                    sizeX = 2;

                    planarType = PLANAR_U;
                } else if ( IsPlanarV( pInputFace->patchWidth, pInputFace->patchHeight, BSP.Vertices.ToPtr() + pInputFace->firstVert ) ) {
                    SurfVerticesCount = 2 * sizeX;
                    SurfIndicesCount = ( ( sizeX - 1 ) / _StepX ) * 6;

                    sizeY = 2;

                    planarType = PLANAR_V;
                } else {
                    SurfVerticesCount = sizeX*sizeY;
                    SurfIndicesCount = ( ( sizeY - 1 ) / _StepY ) * ( ( sizeX - 1 ) / _StepX ) * 6;

                    planarType = NO_PLANAR;
                }

                BSP.Vertices.Resize( BSP.Vertices.Size() + SurfVerticesCount );
                BSP.LightmapVerts.Resize( BSP.LightmapVerts.Size() + SurfVerticesCount );
                BSP.Indices.Resize( BSP.Indices.Size() + SurfIndicesCount );

                FMeshVertex const * srcVerts = BSP.Vertices.ToPtr() + pInputFace->firstVert;
                FMeshLightmapUV const * srcLM = BSP.LightmapVerts.ToPtr() + pInputFace->firstVert;
                FMeshVertex * dstVerts = BSP.Vertices.ToPtr() + SurfFirstVertex;
                FMeshLightmapUV * dstLM = BSP.LightmapVerts.ToPtr() + SurfFirstVertex;
                unsigned int * dstIndices = BSP.Indices.ToPtr() + SurfFirstIndex;

                int baseVertex = 0;

                if ( planarType == PLANAR_U ) {
                    int i;
                    for ( v = 0, i = 0 ; v < sizeY ; v += stepY, i++ ) {
                        memcpy( &dstVerts[ v * 2 ], &srcVerts[ i*pInputFace->patchWidth ], sizeof( dstVerts[ 0 ] ) );
                        memcpy( &dstVerts[ v * 2 + 1 ], &srcVerts[ i*pInputFace->patchWidth + pInputFace->patchWidth - 1 ], sizeof( dstVerts[ 0 ] ) );
                        memcpy( &dstLM[ v * 2 ], &srcLM[ i*pInputFace->patchWidth ], sizeof( dstLM[ 0 ] ) );
                        memcpy( &dstLM[ v * 2 + 1 ], &srcLM[ i*pInputFace->patchWidth + pInputFace->patchWidth - 1 ], sizeof( dstLM[ 0 ] ) );
                    }
                    for ( v = 0 ; v < sizeY - 1 ; v += stepY << 1 ) {     // for each patch
                        SubdivideCurve_r( dstVerts, dstLM, v*sizeX, 2 * stepY, subdivY );
                        SubdivideCurve_r( dstVerts, dstLM, v*sizeX + 1, 2 * stepY, subdivY );
                    }
                    i = 0;
                    for ( v=0 ; v<sizeY-1 ; v+=_StepY ) {
                        dstIndices[ i++ ] = baseVertex + v * sizeX + 1;
                        dstIndices[ i++ ] = baseVertex + ( v + _StepY ) * sizeX;
                        dstIndices[ i++ ] = baseVertex + v * sizeX;
                        dstIndices[ i++ ] = baseVertex + ( v + _StepY ) * sizeX + 1;
                        dstIndices[ i++ ] = baseVertex + ( v + _StepY ) * sizeX;
                        dstIndices[ i++ ] = baseVertex + v * sizeX + 1;
                    }
                    AN_Assert( i == SurfIndicesCount );
                } else if ( planarType == PLANAR_V ) {
                    int i;
                    for ( u = 0, i = 0 ; u < sizeX ; u += stepX, i++ ) {
                        memcpy( &dstVerts[ u ], &srcVerts[ i ], sizeof( dstVerts[ 0 ] ) );
                        memcpy( &dstVerts[ sizeX + u ], &srcVerts[ i + pInputFace->patchWidth*( pInputFace->patchHeight - 1 ) ], sizeof( dstVerts[ 0 ] ) );
                        memcpy( &dstLM[ u ], &srcLM[ i ], sizeof( dstLM[ 0 ] ) );
                        memcpy( &dstLM[ sizeX + u ], &srcLM[ i + pInputFace->patchWidth*( pInputFace->patchHeight - 1 ) ], sizeof( dstLM[ 0 ] ) );
                    }
                    for ( u = 0 ; u < sizeX - 1 ; u += stepX << 1 ) {      // for each patch
                        SubdivideCurve_r( dstVerts, dstLM, u, stepX, subdivX );
                        SubdivideCurve_r( dstVerts, dstLM, sizeX + u, stepX, subdivX );
                    }
                    i = 0;
                    for ( u = 0 ; u < sizeX - 1 ; u += _StepX ) {
                        dstIndices[ i++ ] = baseVertex + u + _StepX;
                        dstIndices[ i++ ] = baseVertex + 1 * sizeX + u;
                        dstIndices[ i++ ] = baseVertex + u;
                        dstIndices[ i++ ] = baseVertex + 1 * sizeX + u + _StepX;
                        dstIndices[ i++ ] = baseVertex + 1 * sizeX + u;
                        dstIndices[ i++ ] = baseVertex + u + _StepX;
                    }
                    AN_Assert( i == SurfIndicesCount );
                } else {
                    for ( v = 0 ; v < sizeY ; v += stepY ) {
                        for ( u = 0 ; u < sizeX ; u += stepX ) {
                            memcpy( &dstVerts[ v*sizeX + u ], srcVerts, sizeof( dstVerts[ 0 ] ) );
                            memcpy( &dstLM[ v*sizeX + u ], srcLM, sizeof( dstLM[ 0 ] ) );
                            srcVerts++;
                            srcLM++;
                        }
                    }
                    for ( u = 0 ; u < sizeX ; u += stepX ) {
                        for ( v = 0 ; v < sizeY - 1 ; v += stepY << 1 ) {
                            SubdivideCurve_r( dstVerts, dstLM, v*sizeX + u, sizeX*stepY, subdivY );
                        }
                    }
                    for ( v = 0 ; v < sizeY ; v++ ) {
                        for ( u = 0 ; u < sizeX - 1 ; u += stepX << 1 ) {
                            SubdivideCurve_r( dstVerts, dstLM, v*sizeX + u, stepX, subdivX );
                        }
                    }
                    int i = 0;
                    for ( v = 0 ; v < sizeY - 1 ; v += _StepY ) {
                        for ( u = 0 ; u < sizeX - 1 ; u += _StepX ) {
                            dstIndices[ i++ ] = baseVertex + v * sizeX + u + _StepX;
                            dstIndices[ i++ ] = baseVertex + ( v + _StepY ) * sizeX + u;
                            dstIndices[ i++ ] = baseVertex + v * sizeX + u;
                            dstIndices[ i++ ] = baseVertex + ( v + _StepY ) * sizeX + u + _StepX;
                            dstIndices[ i++ ] = baseVertex + ( v + _StepY ) * sizeX + u;
                            dstIndices[ i++ ] = baseVertex + v * sizeX + u + _StepX;
                        }
                    }
                    AN_Assert( i == SurfIndicesCount );
                }
                break;
            }

            case SURFTYPE_MESH:
            {
                SurfaceType = SURF_TRISOUP;
#if 0
                SurfFirstVertex = 0;//pInputFace->firstVert;
                SurfVerticesCount = pInputFace->numVerts;
                SurfFirstIndex = pInputFace->patchWidth;//pInputFace->firstIndex;
                SurfIndicesCount = pInputFace->numIndexes;

                //if ( LightmapTextureIndex < 0 ) {
                //    GQuakeViewer->LightmapTextures.Append( GResourceManager->CreateUnnamedResource< FTextureResource >() );
                //    LightmapTextureIndex = GQuakeViewer->LightmapTextures.Length() - 1;

                //    FTextureResource * Lightmap = GQuakeViewer->LightmapTextures[ LightmapTextureIndex ];

                //    TPodArray< FVertexLight > LightVertices;

                //    LightVertices.Resize( pInputFace->numVerts );
                //    FVertexLight * pLightVert = LightVertices.ToPtr();
                //    for ( int i = 0 ; i < pInputFace->numVerts ; i++, pLightVert++ ) {
                //        pLightVert->TexCoord = Vertices[ pInputFace->firstVert + i ].TexCoord;
                //        pLightVert->Color = Vertices[ pInputFace->firstVert +  i ].Color;

                //        OutVertices[ pInputFace->firstVert + i ].LightmapTexCoord = Vertices[ pInputFace->firstVert +  i ].TexCoord;
                //    }

                //    int OriginalFirstIndex = pInputFace->patchWidth;
                //    Lightmap->BakeVertexLighting( 32, 32, LightVertices.ToPtr(), LightVertices.Length(), Indices + OriginalFirstIndex, pInputFace->numIndexes );
                //}
#endif
                break;
            }
            case SURFTYPE_FLARE:
            {
                SurfaceType = SURF_PLANAR;
                SurfVerticesCount=SurfIndicesCount=0;

                // TODO
                break;
            }
        }

        //Bounds.Clear();
        //for ( int i = 0 ; i < SurfIndicesCount ; i++ ) {
        //    Bounds.AddPoint( BSP.Vertices[ SurfFirstVertex + BSP.Indices[ i + SurfFirstIndex ] ].Position );
        //}


        pOutFace->Type = SurfaceType;
        if ( SurfaceType == SURF_PLANAR ) {
            pOutFace->Plane = SurfacePlane;
        }

        //if ( pInputFace->surfaceType == SURFTYPE_PLANAR
        //    || pInputFace->surfaceType == SURFTYPE_MESH
        //    || pInputFace->surfaceType == SURFTYPE_CURVE ) {
            //pOutFace->SetUseCustomBounds( true );
            pOutFace->Bounds = Bounds;
            pOutFace->FirstVertex = SurfFirstVertex;
            pOutFace->NumVertices = SurfVerticesCount;
            pOutFace->FirstIndex = SurfFirstIndex;
            pOutFace->NumIndices = SurfIndicesCount;
            pOutFace->LightmapGroup = GetLightmapGroup( pInputFace->shaderNum, pInputFace->lightmapNum );
            pOutFace->LightmapOffsetX = pInputFace->lightmapX;
            pOutFace->LightmapOffsetY = pInputFace->lightmapY;
            pOutFace->LightmapWidth = pInputFace->lightmapWidth;
            pOutFace->LightmapHeight = pInputFace->lightmapHeight;
    }

    CalcTangentSpace( BSP.Vertices.ToPtr(), BSP.Vertices.Size(), BSP.Indices.ToPtr(), BSP.Indices.Size() );
}

void FQuakeBSP::ReadLFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    int i, j;

    int const * lface = (int const *)( _Data + _Entry.offset );
    int numMarksurfaces = _Entry.size / sizeof( short );

    BSP.Marksurfaces.ResizeInvalidate( numMarksurfaces );
    BSP.Marksurfaces.ZeroMem();

    int * out = BSP.Marksurfaces.ToPtr();
    int const * in = lface;

    for ( i = 0 ; i < numMarksurfaces ; i++ ) {
        j = in[i];
        if ( j >= BSP.Surfaces.Size() ) {
            GLogger.Printf("FQuakeBSP::ReadLFaces: bad surface number\n");
            return;
        }
        out[i] = j;
    }
}

void FQuakeBSP::ReadLeafs( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry, int _VisRowSize ) {
    int i, j;

    QLeaf const * leafs = (QLeaf const *)( _Data + _Entry.offset );
    int numLeafs = _Entry.size / sizeof( QLeaf );

    BSP.Leafs.ResizeInvalidate( numLeafs );
    BSP.Leafs.ZeroMem();

    FBinarySpaceLeaf * out = BSP.Leafs.ToPtr();
    QLeaf const * in = leafs;

    for ( i = 0 ; i < numLeafs ; i++, in++, out++ ) {
        for ( j = 0 ; j < 3 ; j++ ) {
            out->Bounds.Mins[j] = in->mins[j];
            out->Bounds.Maxs[j] = in->maxs[j];
        }

        ConvertFromQuakeCoord( out->Bounds.Mins );
        ConvertFromQuakeCoord( out->Bounds.Maxs );
        //FixupBoundingBox( out->Bounds.Mins, out->Bounds.Maxs );

        //out->Contents = -1;//in->contents;

        out->FirstSurface = /*BSP.Marksurfaces.ToPtr() + */in->firstmarksurface;
        out->NumSurfaces = in->nummarksurfaces;

        //out->FirstBrush = leafBrushes + in->firstBrush;
        //out->NumBrushes = in->numBrushes;

        out->Cluster = in->cluster;
        if ( out->Cluster < 0 || out->Cluster >= BSP.NumVisClusters ) {
            out->Visdata = NULL;
        } else {
            out->Visdata = BSP.Visdata + out->Cluster * _VisRowSize;
        }
    }
}

void FQuakeBSP::SetParent_r( FLevel * _Level, FBinarySpaceNode * _Node, FBinarySpaceNode * _Parent ) {
    _Node->Parent = _Parent;

    if ( _Node->ChildrenIdx[ 0 ] == 0 ) {
        // Solid
    } else if ( _Node->ChildrenIdx[ 0 ] < 0 ) {
        ( BSP.Leafs.ToPtr() + (-1 - _Node->ChildrenIdx[ 0 ]) )->Parent = _Node;
        LeafsCount++;
    } else {
        SetParent_r( _Level, BSP.Nodes.ToPtr() + _Node->ChildrenIdx[ 0 ], _Node );
    }

    if ( _Node->ChildrenIdx[ 1 ] == 0 ) {
        // Solid
    } else if ( _Node->ChildrenIdx[ 1 ] < 0 ) {
        ( BSP.Leafs.ToPtr() + (-1 - _Node->ChildrenIdx[ 1 ]) )->Parent = _Node;
        LeafsCount++;
    } else {
        SetParent_r( _Level, BSP.Nodes.ToPtr() + _Node->ChildrenIdx[ 1 ], _Node );
    }
}

void FQuakeBSP::ReadNodes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    int i, j;

    QNode const * nodes = (QNode const *)( _Data + _Entry.offset );
    int numNodes = _Entry.size / sizeof( QNode );

    BSP.Nodes.ResizeInvalidate( numNodes );
    BSP.Nodes.ZeroMem();

    FBinarySpaceNode * out = BSP.Nodes.ToPtr();
    QNode const * in = nodes;

    for ( i = 0 ; i < numNodes ; i++, in++, out++ ) {
        for ( j = 0 ; j < 3 ; j++ ) {
            out->Bounds.Mins[j] = in->mins[j];
            out->Bounds.Maxs[j] = in->maxs[j];
        }

        ConvertFromQuakeCoord( out->Bounds.Mins );
        ConvertFromQuakeCoord( out->Bounds.Maxs );
        //FixupBoundingBox( out->Bounds.Mins, out->Bounds.Maxs );

        out->Plane = BSP.Planes.ToPtr() + in->planenum;

//        out->NodeId = i;

        for ( j = 0 ; j < 2 ; j++ ) {
            out->ChildrenIdx[ j ] = in->children[ j ];
        }
    }

//    for ( i = 0 ; i < Leafs.Length() ; i++ ) {
//        Leafs[i].NodeId = NumNodes + i;
//    }

    LeafsCount = 0;
    SetParent_r( _Level, BSP.Nodes.ToPtr(), NULL );
}

#if 0
static char * SkipWhiteSpaces( char * s ) {
    while ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ) {
        s++;
    }
    return s;
}

void FQuakeBSP::ReadEntities( const byte * _Data, QBSPEntry const & _Entry ) {
    EntitiesString = (const char *)(_Data + _Entry.offset);
    //GLogger.Print( EntitiesString.ToConstChar() );
    char * s = EntitiesString.ToPtr();
    int brackets = 0;
    int entityNum = 0;
    while ( *s ) {
        s = SkipWhiteSpaces(s);
        if ( *s == '{' ) {
            s++;
            brackets++;
            entityNum++;
            continue;
        }
        if ( *s == '}' ) {
            s++;
            brackets--;
            continue;
        }
        if ( brackets != 1 ) {
            s++;
            continue;
        }

        Entities.Resize( entityNum );
        QEntity * ent = &Entities[entityNum - 1];

        const char * token = "";
        if ( *s == '\"' ) {
            token = ++s;
            while ( *s ) {
                if ( *s == '\"' ) {
                    *s = 0;
                    s++;
                    break;
                }
                s++;
            }
        }

        if ( !*token ) {
            break;
        }

        s = SkipWhiteSpaces(s);
        const char * value = "";
        if ( *s == '\"' ) {
            value = ++s;
            while ( *s ) {
                if ( *s == '\"' ) {
                    *s = 0;
                    s++;
                    break;
                }
                s++;
            }
        }

        if ( !*value ) {
            break;
        }

        if ( !FString::Icmp( token, "classname" ) ) {

            ent->ClassName = value;

            GLogger.Printf("Classname %s\n",value);

        } else if ( !FString::Icmp( token, "origin" ) ) {

            sscanf( value, "%f %f %f", &ent->Origin.X.Value, &ent->Origin.Y.Value, &ent->Origin.Z.Value );

            ConvertFromQuakeCoord( ent->Origin );

        } else if ( !FString::Icmp( token, "angle" ) ) {

            ent->Angle = Float().FromString( value ) - 90.0f;

        } else {

        }
    }
}

void FQuakeBSP::ReadModels( const byte * _Data, QBSPEntry const & _Entry ) {
    // TODO..
}
#endif


int FQuakeBSP::GetLightmapGroup( int _TextureIndex, int _LightmapBlock ) {
    for ( int i = 0 ; i < LightmapGroups.Size() ; i++ ) {
        if ( LightmapGroups[i].TextureIndex == _TextureIndex
             && LightmapGroups[i].LightmapBlock == _LightmapBlock ) {
            return i;
        }
    }
    QLightmapGroup & newIndex = LightmapGroups.Append();
    newIndex.TextureIndex = _TextureIndex;
    newIndex.LightmapBlock = _LightmapBlock;
    return LightmapGroups.Size()-1;
}

#if 0
#include <Engine/GameThread/Public/GameEngine.h>
void FQuakeBSP::UpdateSurfaceLight( FLevel * _Level, FSurfaceDef * _Surf ) {
    if ( _Surf->LightDataOffset < 0 || _Surf->LightmapGroup < 0 ) {
        return;
    }

//    for ( int i = 0 ; i < 256 ; i++ ) {
//        lightstylevalue[ 0 ] = 264 * (sin( float( (GGameEngine.GetGameplayTimeMicro()>>13) + i * 30 ) / 180.0 * 3.14 )*0.5+0.5);
//    }

    lightstylevalue[ 2 ] = lightstylevalue[ 5 ] = lightstylevalue[ 32 ] = 264 * (sin( float( (GGameEngine.GetGameplayTimeMicro()>>13) ) / 180.0 * 3.14 )*0.5+0.5);

    void * data = _Level->Lightmaps[ LightmapGroups[ _Surf->LightmapGroup ].LightmapBlock ]->WriteTextureData(

                _Surf->LightmapOffsetX,
                _Surf->LightmapOffsetY,
                0,
                _Surf->LightmapWidth,
                _Surf->LightmapHeight,
                0
                );

    const byte * samples = _Level->GetLightData() + _Surf->LightDataOffset;

    AccumulateLight( (float*)data, samples, _Surf->LightStyles, _Surf->LightmapWidth, _Surf->LightmapHeight, _Level->GetLightData() == nullptr );
}
#endif
