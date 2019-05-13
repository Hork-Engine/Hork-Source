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
#include <Engine/World/Public/ResourceManager.h>
#include <Engine/World/Public/Shape.h>

AN_BEGIN_CLASS_META( FQuakeModel )
AN_END_CLASS_META()

AN_BEGIN_CLASS_META( FQuakeBSP )
AN_END_CLASS_META()

struct QPakEntry {
    unsigned char filename[ 0x38 ];
    int32_t offset;
    int32_t size;
};

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

bool FQuakePack::LoadPalette( unsigned * _Palette ) {
    int32_t offset;
    int32_t size;

    if ( !FindEntry( "gfx/palette.lmp", &offset, &size ) ) {
        return false;
    }

    byte * palette = ( byte * )GMainHunkMemory.HunkMemory( size, 1 );

    Read( offset, size, palette );

    unsigned r, g, b;
    unsigned *table = _Palette;
    const byte * pPalette = palette;
    for ( int n = 0 ; n < 256 ; n++ ) {
        r = pPalette[ 0 ];
        g = pPalette[ 1 ];
        b = pPalette[ 2 ];
        pPalette += 3;
        *table++ = ( 255 << 24 ) + ( r << 0 ) + ( g << 8 ) + ( b << 16 );
    }
    _Palette[ 255 ] &= 0xffffff; // 255 is transparent

    GMainHunkMemory.ClearLastHunk();

    return true;
}

bool FQuakePack::FindEntry( const char * _Name, int32_t * _Offset, int32_t * _Size ) {
    QPakEntry entry;

    if ( !File.IsOpened() ) {
        return false;
    }

    File.SeekSet( PackHeader.diroffset );

    for ( int i = 0 ; i < NumEntries ; i++ ) {
        File.Read( &entry, sizeof( QPakEntry ) );

        if ( !FString::Icmp( (const char *)entry.filename, _Name ) ) {
            *_Offset = entry.offset;
            *_Size = entry.size;
            return true;
        }
    }

    return false;
}

void FQuakePack::Read( int32_t _Offset, int32_t _Size, byte * _Data ) {
    File.SeekSet( _Offset );
    File.Read( _Data, _Size );
}

FQuakeModel::~FQuakeModel() {
    Purge();
}

bool FQuakeModel::FromData( const byte * _Data, const unsigned * _Palette ) {
    struct QMdlHeader {
        unsigned char Magic[4];
        int32_t Version;
        Float3 Scale;
        Float3 Translate;
        float Radius;
        Float3 EyePosition;
        int32_t SkinsCount;
        int32_t TexWidth;
        int32_t TexHeight;
        int32_t VerticesCount;
        int32_t TrianglesCount;
        int32_t FramesCount;
        int32_t SyncType;     // 0 = synchron, 1 = random
        int32_t Flags;
        float Size;
    };

    struct QTexcoord {
        int32_t onseam;
        int32_t s;
        int32_t t;
    };

    struct QTriangle {
        int32_t CullFace;       // 0 = backface, 1 = frontface
        int32_t Indices[ 3 ];
    };

    struct QModelGroup {
        int     NumPoses;
        QCompressedVertex BboxMin;
        QCompressedVertex BboxMax;
    };

    QMdlHeader const * header = ( QMdlHeader *)_Data;

    _Data += sizeof( QMdlHeader );

    if ( header->Magic[ 0 ] != 'I'
        || header->Magic[ 1 ] != 'D'
        || header->Magic[ 2 ] != 'P'
        || header->Magic[ 3 ] != 'O'
        || header->Version != 6 ) {
        GLogger.Printf( "FQuakeModel::FromData: invalid file\n" );
        return false;
    }

    Purge();

    Scale = header->Scale * FromQuakeScale;
    Translate = header->Translate * FromQuakeScale;
    FCore::SwapArgs( Scale.Y, Scale.Z );
    FCore::SwapArgs( Scale.X, Scale.Z );
    FCore::SwapArgs( Translate.Y, Translate.Z );
    FCore::SwapArgs( Translate.X, Translate.Z );

    Skins.ResizeInvalidate( header->SkinsCount );
    int stride = header->TexWidth * header->TexHeight;
    for ( int i = 0 ; i < header->SkinsCount ; i++ ) {
        QSkin & Skin = Skins[ i ];
        Skin.Group = *( int32_t * )( _Data ); _Data += sizeof( int32_t );
        Skin.Texture = LoadSkin( _Data, header->TexWidth, header->TexHeight, _Palette ); _Data += stride;
        Skin.Texture->AddRef();
    }

    QTexcoord const * texcoords;
    texcoords = (QTexcoord const *)_Data; _Data += sizeof( QTexcoord ) * header->VerticesCount;

    QTriangle const * triangles;
    triangles = (QTriangle const *)_Data; _Data += sizeof( QTriangle ) * header->TrianglesCount;

    int numExtraVerts = 0;
    for ( int i = 0 ; i < header->TrianglesCount ; i++ ) {
        for ( int j = 0 ; j < 3 ; j++ ) {
            if ( triangles[ i ].CullFace == 0 && texcoords[ triangles[ i ].Indices[ j ] ].onseam ) {
                numExtraVerts++;
            }
        }
    }

    Frames.ResizeInvalidate( header->FramesCount );

    VerticesCount = header->VerticesCount + numExtraVerts;

    Texcoords.ResizeInvalidate( VerticesCount );

    Indices.ResizeInvalidate( header->TrianglesCount * 3 );

    this->Texcoords.ZeroMem();

    Float2 texcoord;
    const Float2 texcoordScale( 1.0f / header->TexWidth, 1.0f / header->TexHeight );
    int firstVert = 0;
    unsigned int * pIndices = Indices.ToPtr();
    for ( int i = 0 ; i < header->TrianglesCount ; i++, pIndices += 3 ) {
        pIndices[ 0 ] = triangles[ i ].Indices[ 2 ];
        pIndices[ 1 ] = triangles[ i ].Indices[ 1 ];
        pIndices[ 2 ] = triangles[ i ].Indices[ 0 ];
        for ( int j = 0 ; j < 3 ; j++ ) {
            texcoord.X = texcoords[ triangles[ i ].Indices[ j ] ].s;
            texcoord.Y = texcoords[ triangles[ i ].Indices[ j ] ].t;
            if ( triangles[ i ].CullFace == 0 && texcoords[ triangles[ i ].Indices[ j ] ].onseam ) {
                texcoord.X += header->TexWidth>>1;
                pIndices[ 2 - j ] = header->VerticesCount + firstVert;
                firstVert++;
            }
            this->Texcoords[ pIndices[ 2 - j ] ] = texcoord * texcoordScale;
        }
    }

    enum { MAX_MODEL_POSES = 256 };
    int posenum = 0;
    QCompressedVertex *poseverts[ MAX_MODEL_POSES ];

    for ( int i = 0; i < header->FramesCount; i++ ) {
        QFrame & frame = Frames[ i ];

        int32_t type = *( int32_t * )_Data; _Data += sizeof( int32_t );

        if ( type == 0 ) {

            frame.FirstPose = posenum;
            frame.NumPoses = 1;

            memcpy( &frame.Mins, _Data, sizeof( frame.Mins ) ); _Data += sizeof( frame.Mins );
            memcpy( &frame.Maxs, _Data, sizeof( frame.Maxs ) ); _Data += sizeof( frame.Maxs );
            memcpy( frame.Name, _Data, sizeof( frame.Name ) ); _Data += sizeof( frame.Name );

            FCore::SwapArgs( frame.Mins.Position[1], frame.Mins.Position[2] );
            FCore::SwapArgs( frame.Mins.Position[0], frame.Mins.Position[2] );

            FCore::SwapArgs( frame.Maxs.Position[1], frame.Maxs.Position[2] );
            FCore::SwapArgs( frame.Maxs.Position[0], frame.Maxs.Position[2] );

            AN_Assert( posenum < MAX_MODEL_POSES );
            poseverts[ posenum ] = ( QCompressedVertex * )_Data; _Data += sizeof( QCompressedVertex ) * header->VerticesCount;
            posenum++;

        } else {
            QModelGroup * group = ( QModelGroup * )_Data; _Data += sizeof( QModelGroup );

            frame.FirstPose = posenum;
            frame.NumPoses = group->NumPoses;

            AN_Assert( frame.NumPoses > 0 );

            memcpy( &frame.Mins, &group->BboxMin, sizeof( frame.Mins ) );
            memcpy( &frame.Maxs, &group->BboxMax, sizeof( frame.Maxs ) );

            FCore::SwapArgs( frame.Mins.Position[ 1 ], frame.Mins.Position[ 2 ] );
            FCore::SwapArgs( frame.Mins.Position[ 0 ], frame.Mins.Position[ 2 ] );

            FCore::SwapArgs( frame.Maxs.Position[ 1 ], frame.Maxs.Position[ 2 ] );
            FCore::SwapArgs( frame.Maxs.Position[ 0 ], frame.Maxs.Position[ 2 ] );

            /*frame.interval = *( float * )_Data; */_Data += sizeof( float ) * group->NumPoses;

            for ( int pose = 0; pose < group->NumPoses; pose++ )
            {
                /*memcpy( &frame.Mins, _Data, sizeof( frame.Mins ) );*/ _Data += sizeof( frame.Mins );
                /*memcpy( &frame.Maxs, _Data, sizeof( frame.Maxs ) );*/ _Data += sizeof( frame.Maxs );
                /*memcpy( frame.Name, _Data, sizeof( frame.Name ) );*/ _Data += sizeof( frame.Name );

                AN_Assert( posenum < MAX_MODEL_POSES );
                poseverts[ posenum ] = ( QCompressedVertex * )_Data; _Data += sizeof( QCompressedVertex ) * header->VerticesCount;
                posenum++;
            }
        }
    }

    int poseVerticesCount = VerticesCount * posenum;

    CompressedVertices.ResizeInvalidate( poseVerticesCount );
    QCompressedVertex * pFirstVertex = CompressedVertices.ToPtr();
    for ( int pose = 0; pose < posenum; pose++ ) {
        QCompressedVertex * compressedVerts = ( QCompressedVertex * )poseverts[ pose ];
        memcpy( pFirstVertex, compressedVerts, sizeof( QCompressedVertex ) * header->VerticesCount );

        firstVert = 0;
        for ( int t = 0; t < header->TrianglesCount; t++ ) {
            for ( int j = 0; j < 3; j++ ) {
                if ( triangles[ t ].CullFace == 0 && texcoords[ triangles[ t ].Indices[ j ] ].onseam ) {
                    pFirstVertex[ header->VerticesCount + firstVert ] = compressedVerts[ triangles[ t ].Indices[ j ] ];
                    firstVert++;
                }
            }
        }
        pFirstVertex += VerticesCount;
    }

    for ( QCompressedVertex & vertex : CompressedVertices ) {
        FCore::SwapArgs( vertex.Position[1], vertex.Position[2] );
        FCore::SwapArgs( vertex.Position[0], vertex.Position[2] );
    }

    for ( int i = 0; i < header->FramesCount; i++ ) {
        QFrame & frame = Frames[ i ];
        frame.Vertices = CompressedVertices.ToPtr() + frame.FirstPose * VerticesCount;
    }

    return true;
}

FTexture * FQuakeModel::LoadSkin( const byte * _Data, int _Width, int _Height, const unsigned * _Palette ) {
    static byte TexData[ 1024*1024*3 ];
    AN_Assert( _Width <= 1024 && _Height <= 1024 );

    int Sz = _Width * _Height;
    for ( int t = 0 ; t < Sz ; t++ ) {
        TexData[ t*3 + 2 ] =   _Palette[ _Data[ t ] ] & 0x000000ff;
        TexData[ t*3 + 1 ] = ( _Palette[ _Data[ t ] ] & 0x0000ff00 ) >> 8;
        TexData[ t*3 + 0 ] = ( _Palette[ _Data[ t ] ] & 0x00ff0000 ) >> 16;
    }

    FSoftwareMipmapGenerator mipmapGen;

    mipmapGen.SourceImage = TexData;
    mipmapGen.Width = _Width;
    mipmapGen.Height = _Height;
    mipmapGen.NumChannels = 3;
    mipmapGen.bLinearSpace = false;
    mipmapGen.bHDRI = false;

    int requiredMemorySize, numLods;
    mipmapGen.ComputeRequiredMemorySize( requiredMemorySize, numLods );

    void * mipmappedData = GMainHunkMemory.HunkMemory( requiredMemorySize, 1 );

    mipmapGen.GenerateMipmaps( mipmappedData );

    FTexture * texture = NewObject< FTexture >();
    texture->Initialize2D( TEXTURE_PF_BGR8_SRGB, numLods, _Width, _Height, 1 );

    byte * pSrc = ( byte * )mipmappedData;
    int w, h, stride;
    int pixelByteLength = texture->UncompressedPixelByteLength();

    for ( int lod = 0 ; lod < numLods ; lod++ ) {
        w = FMath::Max( 1, _Width >> lod );
        h = FMath::Max( 1, _Height >> lod );

        stride = w * h * pixelByteLength;

        void * pPixels = texture->WriteTextureData( 0, 0, 0, w, h, lod );
        if ( pPixels ) {
            memcpy( pPixels, pSrc, stride );
        }

        pSrc += stride;
    }

    GMainHunkMemory.ClearLastHunk();

    return texture;
}

FQuakePack::FQuakePack() {
    NumEntries = 0;
}

bool FQuakePack::Load( const char * _PackFile ) {
    if ( !File.OpenRead( _PackFile ) ) {
        return false;
    }

    File.Read( &PackHeader, sizeof( PackHeader ) );

    if ( !( PackHeader.magic[ 0 ] == 'P'
            && PackHeader.magic[ 1 ] == 'A'
            && PackHeader.magic[ 2 ] == 'C'
            && PackHeader.magic[ 3 ] == 'K' ) ) {
        GLogger.Printf( "LoadQuakeModel: invalid PAK file\n" );
        return false;
    }

    NumEntries = PackHeader.dirsize / sizeof(QPakEntry);
    if ( !NumEntries ) {
        GLogger.Printf( "LoadQuakeModel: empty PAK file\n" );
        return false;
    }

    return true;
}

bool FQuakeModel::LoadFromPack( FQuakePack * _Pack, const unsigned * _Palette, const char * _ModelFile ) {
    int32_t offset;
    int32_t size;

    if ( !_Pack->FindEntry( _ModelFile, &offset, &size ) ) {
        return false;
    }

    byte * data = ( byte * )GMainHunkMemory.HunkMemory( size, 1 );

    _Pack->Read( offset, size, data );

    FromData( data, _Palette );

    GMainHunkMemory.ClearLastHunk();

    return true;
}

void FQuakeModel::Purge() {
    for ( QSkin & skin : Skins ) {
        skin.Texture->RemoveRef();
    }
    Skins.Free();
    Frames.Free();
    CompressedVertices.Free();
    Texcoords.Free();
    Indices.Free();
    Indices.Free();
}



struct FLightmapBlockAllocator {

    enum {
        BLOCK_WIDTH     = 256,//128
        BLOCK_HEIGHT    = 256,//128
        MAX_BLOCKS      = 16, //64
        NUM_CHANNELS    = 1
    };

    void Clear();

    bool Alloc( int _Width, int _Height, int *x, int *y, int *z );

    void CopySamples( int _X, int _Y, int _Z, int _Width, int _Height, const float * _Samples );

    float * GetLightmapBlock( int _BlockIndex );

    int GetLightmapBlockLength() const;

private:
    int Allocated[ MAX_BLOCKS ][ BLOCK_WIDTH ];
    float LightmapData[ MAX_BLOCKS * BLOCK_WIDTH * BLOCK_HEIGHT * NUM_CHANNELS ];
};

void FLightmapBlockAllocator::Clear() {
    memset( Allocated, 0, sizeof( Allocated ) );
}

bool FLightmapBlockAllocator::Alloc( int _Width, int _Height, int *x, int *y, int *z ) {
    int i, j, bestHeight, tentativeHeight;

    *x = *y = *z = 0;

    AN_Assert( _Width <= BLOCK_WIDTH && _Height <= BLOCK_WIDTH );

    for ( int blockIndex = 0 ; blockIndex < MAX_BLOCKS ; blockIndex++ ) {
        bestHeight = BLOCK_HEIGHT;

        for ( i = 0 ; i < BLOCK_WIDTH - _Width ; i++ ) {
            tentativeHeight = 0;

            for ( j = 0 ; j < _Width ; j++ ) {                
                if ( Allocated[ blockIndex ][ i + j ] >= bestHeight ) {
                    break;
                }
                if ( Allocated[ blockIndex ][ i + j ] > tentativeHeight ) {
                    tentativeHeight = Allocated[ blockIndex ][ i + j ];
                }
            }
            if ( j == _Width ) {
                *x = i;
                *y = bestHeight = tentativeHeight;
            }
        }

        if ( bestHeight + _Height > BLOCK_HEIGHT ) {
            continue;
        }

        for ( i = 0 ; i < _Width ; i++ ) {
            Allocated[ blockIndex ][ *x + i ] = bestHeight + _Height;
        }

        *z = blockIndex;
        return true;
    }

    GLogger.Printf( "FLightmapBlockAllocator::Alloc: couldn't allocate lightmap %d x %d\n", _Width, _Height );
    return false;
}

void FLightmapBlockAllocator::CopySamples( int _X, int _Y, int _Z, int _Width, int _Height, const float * _Samples ) {
    AN_Assert( _X + _Width <= BLOCK_WIDTH );
    AN_Assert( _Y + _Height <= BLOCK_HEIGHT );

    float * base = GetLightmapBlock( _Z );

    base += ( _Y * BLOCK_WIDTH + _X ) * NUM_CHANNELS;

    int stride = BLOCK_WIDTH * NUM_CHANNELS;
    for ( int i = 0 ; i < _Height ; i++ ) {
        memcpy( base, _Samples, sizeof( *base ) * _Width * NUM_CHANNELS );
        _Samples += _Width * NUM_CHANNELS;
        base += stride;
    }
}

float * FLightmapBlockAllocator::GetLightmapBlock( int _BlockIndex ) {
    AN_Assert( _BlockIndex < MAX_BLOCKS );

    return LightmapData + _BlockIndex * ( BLOCK_WIDTH * BLOCK_HEIGHT * NUM_CHANNELS );
}

int FLightmapBlockAllocator::GetLightmapBlockLength() const {
    return BLOCK_WIDTH * BLOCK_HEIGHT * NUM_CHANNELS * sizeof(*LightmapData);
}

#if 0
#define MAX_LIGHTSTYLES 64
struct QLightStyle {
    int     length;
    char    map[64];
};
QLightStyle cl_lightstyle[ MAX_LIGHTSTYLES ];
#endif

static int          lightstylevalue[256]; // 8.8 fraction of base light value

//static unsigned     AccumulatedLight[18*18];
//static unsigned     AccumulatedLight[32*32];
//static float        AccumulatedLight[64*64];
static float        AccumulatedLight[128*128];

static void AccumulateLight( float * _AccumulatedLight, byte const * _Samples, byte const * _Styles, int _Width, int _Height, bool _Fullbright ) {
    int size = _Width * _Height;

    AN_Assert( size < AN_ARRAY_LENGTH( AccumulatedLight ) );

    if ( _Fullbright ) {
        for ( int i = 0 ; i < size ; i++ ) {
            _AccumulatedLight[ i ] = 1;
        }
        return;
    }

    // Clear to no light
    for ( int i = 0 ; i < size ; i++ ) {
        _AccumulatedLight[ i ] = 0;
    }

    // Add all the lightmaps
    if ( _Samples ) {
        for ( int maps = 0 ; maps < MAX_SURFACE_LIGHTMAPS && _Styles[ maps ] != 255 ; maps++ ) {

            unsigned scale = lightstylevalue[ _Styles[ maps ] ];

            //surf->cached_light[maps] = scale; // 8.8 fraction

            for ( int i = 0 ; i < size ; i++ ) {
                _AccumulatedLight[ i ] += _Samples[ i ] / 255.0f * scale;
            }

            _Samples += size; // skip to next lightmap
        }
    }

    // add all the dynamic lights
    //if (surf->dlightframe == r_framecount)
    // R_AddDynamicLights (surf);

    // bound, invert, and shift
    for ( int i = 0 ; i < size ; i++ ) {

        //_AccumulatedLight[ i ] >>= 7;
        //if ( _AccumulatedLight[ i ] > 255 ) {
        //    _AccumulatedLight[ i ] = 255;
        //}

        _AccumulatedLight[ i ] = pow( _AccumulatedLight[ i ] / 64, 2.2f );//128.0f;
    }
}

static FLightmapBlockAllocator LightmapBlockAllocator;


FQuakeBSP::~FQuakeBSP() {
    Purge();
}

bool FQuakeBSP::FromData( FLevel * _Level, const byte * _Data, const unsigned * _Palette ) {
    struct QHeader {
        int32_t    version;
        QBSPEntry  entities;
        QBSPEntry  planes;
        QBSPEntry  miptex;
        QBSPEntry  vertices;
        QBSPEntry  visilist;
        QBSPEntry  nodes;
        QBSPEntry  texinfo;
        QBSPEntry  faces;
        QBSPEntry  lightmaps;
        QBSPEntry  clipnodes;
        QBSPEntry  leafs;
        QBSPEntry  lface;
        QBSPEntry  edges;
        QBSPEntry  ledges;
        QBSPEntry  models;
    };

    QHeader const * header;

    header = ( QHeader const * )_Data;

    if ( header->version != 29 ) {
        return false;
    }

    for ( int i = 0 ; i < 256 ; i++ ) {
        lightstylevalue[ i ] = 264;  // normal light value
    }
#if 0
    memset ( cl_lightstyle, 0, sizeof( cl_lightstyle ) );
    cl_lightstyle[ 1 ].length = 64;
    for ( int j = 1; j < 64; j++ ) {
        for ( int i = 0 ; i < 64 ; i++ ) {
            cl_lightstyle[ j ].map[ i ] = 'a' + ( ( i + j * 32 ) % 26 );
        }
    }
#endif

    Purge();

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

    GLogger.Printf( "texcount %d lightmaps %d leafs %d leafscount %d\n",
                    Textures.Length(),
                    _Level->Lightmaps.Length(),
                    BSP.Leafs.Length(),
                    LeafsCount );

    return true;
}

bool FQuakeBSP::LoadFromPack( FLevel * _Level, FQuakePack * _Pack, const unsigned * _Palette, const char * _MapFile ) {
    int32_t offset;
    int32_t size;

    if ( !_Pack->FindEntry( _MapFile, &offset, &size ) ) {
        return false;
    }

    byte * data = ( byte * )GMainHunkMemory.HunkMemory( size, 1 );

    _Pack->Read( offset, size, data );

    FromData( _Level, data, _Palette );

    GMainHunkMemory.ClearLastHunk();

    return true;
}

void FQuakeBSP::Purge() {
    for ( QTexture & texture : Textures ) {
        texture.Object->RemoveRef();
    }

    Textures.Free();
    LightmapGroups.Free();
    Entities.Free();
    EntitiesString.Free();
}

void FQuakeBSP::ReadPlanes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    struct QPlane {
        Float3 normal;
        float dist;
        int32_t type;
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

void FQuakeBSP::ReadTexInfos( const byte * _Data, QBSPEntry const & _Entry ) {
    struct QTexinfo {
        float       vecs[2][4];   // [s/t][xyz offset]
        int         textureIndex;
        int         flags;
    };

    QTexinfo const * texInfo = (QTexinfo const *)( _Data + _Entry.offset );
    int numTexInfo = _Entry.size / sizeof( QTexinfo );
    QTexinfo const * in = texInfo;
    QTexinfoExt *out;

    TexInfos.ResizeInvalidate( numTexInfo );
    TexInfos.ZeroMem();

    out = TexInfos.ToPtr();

    for ( int i = 0 ; i < numTexInfo ; i++, in++, out++ ) {
        for ( int j = 0 ; j < 4 ; j++ ){
            out->vecs[ 0 ][ j ] = in->vecs[ 0 ][ j ];
            out->vecs[ 1 ][ j ] = in->vecs[ 1 ][ j ];
        }

        out->textureIndex = in->textureIndex;

        if ( in->textureIndex >= Textures.Length() ) {
            GLogger.Printf( "FQuakeBSP::ReadTexInfos: textureIndex >= numtextures\n" );
        }
    }
}

void FQuakeBSP::ReadTextures( const byte * _Data, const unsigned * _Palette, QBSPEntry const & _Entry ) {
    struct QMipHeader {
        int32_t numMipTextures;
        int32_t texOffset[1];
    };

    struct QMipTex {
        char   name[16];
        uint32_t width;
        uint32_t height;
        uint32_t miplevels[4];
    };

    QMipHeader const * miptex = (QMipHeader *)( _Data + _Entry.offset );

    Textures.ResizeInvalidate( miptex->numMipTextures );
    Textures.ZeroMem();

    FTexture * defaultTexture = NewObject< FTexture >();
    defaultTexture->SetName( "default" );
    defaultTexture->Initialize2D( TEXTURE_PF_BGR8_SRGB, 1, 1, 1 );
    void * pPixels = defaultTexture->WriteTextureData( 0, 0, 0, 1, 1, 0 );
    if ( pPixels ) {
        memset( pPixels, 0xff, 1*1*3 );
    }

    for ( int i = 0 ; i < miptex->numMipTextures ; i++ ) {
        QTexture * texture = &Textures[ i ];

        if ( miptex->texOffset[ i ] == -1 ) {
            GLogger.Printf( "miptex->texOffset[ %d ] == -1\n", i );

            texture->Object = defaultTexture;
            defaultTexture->AddRef();
            continue;
        }

        QMipTex * mt = (QMipTex *)((byte *)miptex + miptex->texOffset[i]);

        FTexture * tx = NewObject< FTexture >();
        tx->AddRef();
        tx->SetName( mt->name );
        texture->Object = tx;

        if ( !FString::CmpN( mt->name, "sky", 3 ) ) {
            const byte * pix = ( byte * )( mt + 1 );

            int layerWidth = mt->width >> 1;

            tx->Initialize2D( TEXTURE_PF_BGRA8_SRGB, 1, layerWidth, mt->height, 2 );
            byte * layer0 = ( byte * )tx->WriteTextureData( 0, 0, 0, layerWidth, mt->height, 0 );
            byte * layer1 = ( byte * )tx->WriteTextureData( 0, 0, 1, layerWidth, mt->height, 0 );

            if ( layer0 && layer1 ) {
                byte * pLayer0 = layer0;
                byte * pLayer1 = layer1;

                for ( int y = 0 ; y < mt->height ; y++ ) {
                    for ( int x = 0 ; x < layerWidth ; x++ ) {
                        *pLayer0++ = ( _Palette[ pix[ x + layerWidth ] ] & 0x00ff0000 ) >> 16;
                        *pLayer0++ = ( _Palette[ pix[ x + layerWidth ] ] & 0x0000ff00 ) >> 8;
                        *pLayer0++ =   _Palette[ pix[ x + layerWidth ] ] & 0x000000ff;
                        *pLayer0++ = 255;
                        *pLayer1++ = ( _Palette[ pix[ x ] ] & 0x00ff0000 ) >> 16;
                        *pLayer1++ = ( _Palette[ pix[ x ] ] & 0x0000ff00 ) >> 8;
                        *pLayer1++ =   _Palette[ pix[ x ] ] & 0x000000ff;
                        *pLayer1++ = pix[ x ] == 0 ? 0 : 255;
                    }
                    pix += mt->width;
                }
            }

        } else {
            static byte TexData[ 1024 * 1024 * 3 ];

            AN_Assert( mt->width < 1024 && mt->height < 1024 );

            const byte * pix = ( byte * )( mt + 1 );

            int sz = mt->width * mt->height;
            for ( int t = 0 ; t < sz ; t++ ) {
                TexData[ t*3 + 2 ] =   _Palette[ pix[ t ] ] & 0x000000ff;
                TexData[ t*3 + 1 ] = ( _Palette[ pix[ t ] ] & 0x0000ff00 ) >> 8;
                TexData[ t*3 + 0 ] = ( _Palette[ pix[ t ] ] & 0x00ff0000 ) >> 16;
            }

            FSoftwareMipmapGenerator mipmapGen;

            mipmapGen.SourceImage = TexData;
            mipmapGen.Width = mt->width;
            mipmapGen.Height = mt->height;
            mipmapGen.NumChannels = 3;
            mipmapGen.bLinearSpace = false;
            mipmapGen.bHDRI = false;

            int requiredMemorySize, numLods;
            mipmapGen.ComputeRequiredMemorySize( requiredMemorySize, numLods );

            void * mipmappedData = GMainHunkMemory.HunkMemory( requiredMemorySize, 1 );

            mipmapGen.GenerateMipmaps( mipmappedData );

            tx->Initialize2D( TEXTURE_PF_BGR8_SRGB, numLods, mt->width, mt->height, 1 );

            byte * pSrc = ( byte * )mipmappedData;
            int w, h, stride;
            int pixelByteLength = tx->UncompressedPixelByteLength();

            for ( int lod = 0 ; lod < numLods ; lod++ ) {
                w = FMath::Max< int >( 1, mt->width >> lod );
                h = FMath::Max< int >( 1, mt->height >> lod );

                stride = w * h * pixelByteLength;

                pPixels = tx->WriteTextureData( 0, 0, 0, w, h, lod );
                if ( pPixels ) {
                    memcpy( pPixels, pSrc, stride );
                }

                pSrc += stride;
            }

            GMainHunkMemory.ClearLastHunk();
        }
    }

    enum { MAX_ANIM_FRAMES = 10 };
    QTexture *anims[ MAX_ANIM_FRAMES ];
    QTexture *altanims[ MAX_ANIM_FRAMES ];

    for ( int i = 0 ; i < Textures.Length() ; i++ ) {
        QTexture * tx = &Textures[ i ];

        const char * name = tx->Object->GetName().ToConstChar();

        if ( name[ 0 ] != '+' ) {
            continue;
        }

        if ( tx->Next ) {
            continue;
        }

        memset( anims, 0, sizeof( anims ) );
        memset( altanims, 0, sizeof( altanims ) );

        int numFrames = name[ 1 ];
        int numFramesAlt = 0;
        if ( numFrames >= 'a' && numFrames <= 'z' ) {
            numFrames -= 'a' - 'A';
        }
        if ( numFrames >= '0' && numFrames <= '9' ) {
            numFrames -= '0';
            numFramesAlt = 0;
            anims[ numFrames ] = tx;
            numFrames++;
        } else if ( numFrames >= 'A' && numFrames <= 'J' ) {
            numFramesAlt = numFrames - 'A';
            numFrames = 0;
            altanims[ numFramesAlt ] = tx;
            numFramesAlt++;
        } else {
            CriticalError( "Invalid texture animation %s", name );
        }

        for ( int j = i + 1 ; j < Textures.Length() ; j++ ) {
            QTexture * tx2 = &Textures[ j ];

            const char * name2 = tx2->Object->GetName().ToConstChar();

            if ( name2[ 0 ] != '+' ) {
                continue;
            }

            if ( strcmp ( name2 + 2, name + 2 ) ) {
                continue;
            }

            int num = tx2->Object->GetName()[ 1 ];
            if ( num >= 'a' && num <= 'z' ) {
                num -= 'a' - 'A';
            }
            if ( num >= '0' && num <= '9' ) {
                num -= '0';
                anims[ num ] = tx2;
                if ( num + 1 > numFrames ) {
                    numFrames = num + 1;
                }
            } else if ( num >= 'A' && num <= 'J' ) {
                num = num - 'A';
                altanims[ num ] = tx2;
                if ( num + 1 > numFramesAlt ) {
                    numFramesAlt = num + 1;
                }
            } else {
                CriticalError( "Invalid texture animation %s", name );
            }
        }

        #define	ANIM_CYCLE 2

        for ( int j = 0; j<numFrames; j++ ) {
            QTexture * tx2 = anims[ j ];
            if ( !tx2 ) {
                CriticalError( "Missing frame %i of %s", j, tx->Object->GetName().ToConstChar() );
            }

            tx2->NumFrames = numFrames * ANIM_CYCLE;
            tx2->FrameTimeMin = j * ANIM_CYCLE;
            tx2->FrameTimeMax = ( j + 1 ) * ANIM_CYCLE;
            tx2->Next = anims[ ( j + 1 ) % numFrames ];
            if ( numFramesAlt ) {
                tx2->AltNext = altanims[ 0 ];
            }
        }

        for ( int j = 0; j<numFramesAlt; j++ ) {
            QTexture * tx2 = altanims[ j ];
            if ( !tx2 ) {
                CriticalError( "Missing frame %i of %s", j, tx2->Object->GetName().ToConstChar() );
            }
            tx2->NumFrames = numFramesAlt * ANIM_CYCLE;
            tx2->FrameTimeMin = j * ANIM_CYCLE;
            tx2->FrameTimeMax = ( j + 1 ) * ANIM_CYCLE;
            tx2->Next = altanims[ ( j + 1 ) % numFramesAlt ];
            if ( numFrames ) {
                tx2->AltNext = anims[ 0 ];
            }
        }
    }
}

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

    Bounds.Clear();

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
        int lightmapBlock = 0;

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

        Bounds.AddAABB( out->Bounds );

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
    //_Level->ClearLightmaps();
    //_Level->Lightmaps.ResizeInvalidate( numLightmaps );
    //for ( int i = 0 ; i < _Level->Lightmaps.Length() ; i++ ) {
    //    _Level->Lightmaps[i] = NewObject< FTexture >();
    //    _Level->Lightmaps[i]->AddRef();
    //    _Level->Lightmaps[i]->Initialize2D( TEXTURE_PF_R32F, 1, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 1 );

    //    void * pPixels = _Level->Lightmaps[i]->WriteTextureData( 0, 0, 0, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 0 );
    //    if ( pPixels ) {
    //        memcpy( pPixels, LightmapBlockAllocator.GetLightmapBlock( i ), LightmapBlockAllocator.GetLightmapBlockLength() );
    //    }
    //}

    // Create lightmaps
    _Level->ClearLightmaps();
    _Level->Lightmaps.ResizeInvalidate( numLightmaps );
    for ( int i = 0 ; i < _Level->Lightmaps.Length() ; i++ ) {
        _Level->Lightmaps[i] = NewObject< FTexture >();
        _Level->Lightmaps[i]->AddRef();
        _Level->Lightmaps[i]->Initialize2D( TEXTURE_PF_BGR16F, 1, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 1 );

        UShort * pPixels = (UShort*)_Level->Lightmaps[i]->WriteTextureData( 0, 0, 0, LightmapBlockAllocator.BLOCK_WIDTH, LightmapBlockAllocator.BLOCK_HEIGHT, 0 );
        if ( pPixels ) {
            const int sz = LightmapBlockAllocator.BLOCK_WIDTH*LightmapBlockAllocator.BLOCK_HEIGHT*LightmapBlockAllocator.NUM_CHANNELS;
            const float * src = LightmapBlockAllocator.GetLightmapBlock( i );

            for ( int p = 0 ; p < sz ; p += LightmapBlockAllocator.NUM_CHANNELS, pPixels += 3, src += LightmapBlockAllocator.NUM_CHANNELS ) {
                pPixels[0] = pPixels[1] = pPixels[2] = Float::FloatToHalf( *reinterpret_cast< const uint32_t * >( src ) );
            }
        }
    }
}

void FQuakeBSP::ReadLFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    int i, j;

    short const * lface = (short const *)( _Data + _Entry.offset );
    int numMarksurfaces = _Entry.size / sizeof( short );

    BSP.Marksurfaces.ResizeInvalidate( numMarksurfaces );
    BSP.Marksurfaces.ZeroMem();

    int * out = BSP.Marksurfaces.ToPtr();
    short const * in = lface;

    for ( i = 0 ; i < numMarksurfaces ; i++ ) {
        j = in[i];
        if ( j >= BSP.Surfaces.Length() ) {
            GLogger.Printf("FQuakeBSP::ReadLFaces: bad surface number\n");
            return;
        }
        out[i] = j;
    }
}

void FQuakeBSP::ReadLeafs( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry ) {
    #define AMBIENT_WATER   0
    #define AMBIENT_SKY     1
    #define AMBIENT_SLIME   2
    #define AMBIENT_LAVA    3
    #define NUM_AMBIENTS    4       // automatic ambient sounds

    struct QLeaf {
        int         contents;
        int         visofs;
        short       mins[3];
        short       maxs[3];
        unsigned short  firstmarksurface;
        unsigned short  nummarksurfaces;
        byte        ambient_level[NUM_AMBIENTS];
    };

    int i, j, visOffset;

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

        //out->Contents = in->contents;

        out->Cluster = i - 1; // Quake1 has no clusters

        out->FirstSurface = /*this->Marksurfaces.ToPtr() + */in->firstmarksurface;
        out->NumSurfaces = in->nummarksurfaces;

        AN_Assert( in->firstmarksurface + in->nummarksurfaces <= BSP.Marksurfaces.Length() );

        visOffset = in->visofs;
        if ( visOffset == -1 ) {
            out->Visdata = NULL;
        } else {
            out->Visdata = BSP.Visdata + visOffset;
        }

        //for (j=0 ; j<4 ; j++)
        // out->AmbientSoundLevel[j] = in->ambient_level[j];
    }

    BSP.NumVisClusters = numLeafs;
}

//static void SetParent_r( FNode *node, FNode *parent ) {
//    node->Parent = parent;
//    if ( node->Contents < 0 )
//        return;
//    SetParent_r( node->Children[ 0 ], node );
//    SetParent_r( node->Children[ 1 ], node );
//}

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
    struct QNode {
        int         planenum;
        short       children[2];
        short       mins[3];
        short       maxs[3];
        unsigned short  firstface;
        unsigned short  numfaces; // both sides
    };

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

void FQuakeBSP::ReadClipnodes( const byte * _Data, QBSPEntry const & _Entry ) {
    //struct QClipNode {
    //    int         planenum;
    //    short       children[2];
    //};
    //QClipNode * ClipNodes = (QClipNode *)( Data + _Entry.offset );
    //int NumClipnodes = _Entry.size / sizeof( QClipNode );

    //hull = &MapHulls[1];
    //hull->clipnodes = ClipNodes;
    //hull->firstclipnode = 0;
    //hull->lastclipnode = NumClipnodes-1;
    //hull->planes = Planes.ToPtr();
    //hull->clip_mins[0] = -16;
    //hull->clip_mins[1] = -16;
    //hull->clip_mins[2] = -24;
    //hull->clip_maxs[0] = 16;
    //hull->clip_maxs[1] = 16;
    //hull->clip_maxs[2] = 32;

    //hull = &MapHulls[2];
    //hull->clipnodes = ClipNodes;
    //hull->firstclipnode = 0;
    //hull->lastclipnode = NumClipnodes-1;
    //hull->planes = Planes.ToPtr();
    //hull->clip_mins[0] = -32;
    //hull->clip_mins[1] = -32;
    //hull->clip_mins[2] = -24;
    //hull->clip_maxs[0] = 32;
    //hull->clip_maxs[1] = 32;
    //hull->clip_maxs[2] = 64;
}

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



int FQuakeBSP::GetLightmapGroup( int _TextureIndex, int _LightmapBlock ) {
    for ( int i = 0 ; i < LightmapGroups.Length() ; i++ ) {
        if ( LightmapGroups[i].TextureIndex == _TextureIndex
             && LightmapGroups[i].LightmapBlock == _LightmapBlock ) {
            return i;
        }
    }
    QLightmapGroup & newIndex = LightmapGroups.Append();
    newIndex.TextureIndex = _TextureIndex;
    newIndex.LightmapBlock = _LightmapBlock;
    return LightmapGroups.Length()-1;
}


#include <Engine/World/Public/GameMaster.h>
void FQuakeBSP::UpdateSurfaceLight( FLevel * _Level, FSurfaceDef * _Surf ) {
    if ( _Surf->LightDataOffset < 0 || _Surf->LightmapGroup < 0 ) {
        return;
    }

//    for ( int i = 0 ; i < 256 ; i++ ) {
//        lightstylevalue[ 0 ] = 264 * (sin( float( (GGameMaster.GetGameplayTimeMicro()>>13) + i * 30 ) / 180.0 * 3.14 )*0.5+0.5);
//    }

    lightstylevalue[ 2 ] = lightstylevalue[ 5 ] = lightstylevalue[ 32 ] = 264 * (sin( float( (GGameMaster.GetGameplayTimeMicro()>>13) ) / 180.0 * 3.14 )*0.5+0.5);

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
