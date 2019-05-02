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

#include <Engine/World/Public/Texture.h>
#include <Engine/World/Public/RenderFrontend.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META_NO_ATTRIBS( FTexture )

FTexture::FTexture() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_Texture >();
}

FTexture::~FTexture() {
    RenderProxy->KillProxy();
}

void FTexture::Purge() {

}

static bool GetAppropriatePixelFormat( FImage const & _Image, ETexturePixelFormat & _PixelFormat ) {
    if ( _Image.bHDRI ) {

        if ( _Image.bHalf ) {

            switch ( _Image.NumChannels ) {
            case 1:
                _PixelFormat = TEXTURE_PF_R16F;
                break;
            case 2:
                _PixelFormat = TEXTURE_PF_RG16F;
                break;
            case 3:
                _PixelFormat = TEXTURE_PF_BGR16F;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA16F;
                break;
            default:
                GLogger.Printf( "FTexture::FromImage: invalid image\n" );
                return false;
            }

        } else {

            switch ( _Image.NumChannels ) {
            case 1:
                _PixelFormat = TEXTURE_PF_R32F;
                break;
            case 2:
                _PixelFormat = TEXTURE_PF_RG32F;
                break;
            case 3:
                _PixelFormat = TEXTURE_PF_BGR32F;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA32F;
                break;
            default:
                GLogger.Printf( "FTexture::FromImage: invalid image\n" );
                return false;
            }

        }
    } else {

        if ( _Image.bLinearSpace ) {

            switch ( _Image.NumChannels ) {
            case 1:
                _PixelFormat = TEXTURE_PF_R8;
                break;
            case 2:
                _PixelFormat = TEXTURE_PF_RG8;
                break;
            case 3:
                _PixelFormat = TEXTURE_PF_BGR8;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA8;
                break;
            default:
                GLogger.Printf( "FTexture::FromImage: invalid image\n" );
                return false;
            }

        } else {

            switch ( _Image.NumChannels ) {
            case 3:
                _PixelFormat = TEXTURE_PF_BGR8_SRGB;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA8_SRGB;
                break;
            case 1:
            case 2:
            default:
                GLogger.Printf( "FTexture::FromImage: invalid image\n" );
                return false;
            }
        }
    }

    return true;
}

bool FTexture::InitializeFromImage( FImage const & _Image ) {
    if ( !_Image.pRawData ) {
        GLogger.Printf( "FTexture::InitializeFromImage: empty image data\n" );
        return false;
    }

    ETexturePixelFormat pixelFormat;

    if ( !GetAppropriatePixelFormat( _Image, pixelFormat ) ) {
        return false;
    }

    Initialize2D( pixelFormat, _Image.NumLods, _Image.Width, _Image.Height, 1 );

    byte * pSrc = ( byte * )_Image.pRawData;
    int w, h, stride;
    int pixelByteLength = ::UncompressedPixelByteLength( pixelFormat );

    for ( int lod = 0 ; lod < _Image.NumLods ; lod++ ) {
        w = FMath::Max( 1, _Image.Width >> lod );
        h = FMath::Max( 1, _Image.Height >> lod );

        stride = w * h * pixelByteLength;

        void * pPixels = WriteTextureData( 0, 0, 0, w, h, lod );
        if ( pPixels ) {
            memcpy( pPixels, pSrc, stride );
        }

        pSrc += stride;
    }

    return true;
}

bool FTexture::InitializeCubemapFromImages( FImage const * _Faces[6] ) {
    const void * faces[6];

    int width = _Faces[0]->Width;

    for ( int i = 0 ; i < 6 ; i++ ) {

        if ( !_Faces[i]->pRawData ) {
            GLogger.Printf( "FTexture::InitializeCubemapFromImages: empty image data\n" );
            return false;
        }

        if ( _Faces[i]->Width != width
             || _Faces[i]->Height != width ) {
            GLogger.Printf( "FTexture::InitializeCubemapFromImages: faces with different sizes\n" );
            return false;
        }

        faces[i] = _Faces[i]->pRawData;
    }

    ETexturePixelFormat pixelFormat;

    if ( !GetAppropriatePixelFormat( *_Faces[0], pixelFormat ) ) {
        return false;
    }

    for ( int i = 1 ; i < 6 ; i++ ) {
        ETexturePixelFormat facePF;

        if ( !GetAppropriatePixelFormat( *_Faces[i], facePF ) ) {
            return false;
        }

        if ( pixelFormat != facePF ) {
            GLogger.Printf( "FTexture::InitializeCubemapFromImages: faces with different pixel formats\n" );
            return false;
        }
    }

    InitializeCubemap( pixelFormat, 1, width );

    int w, stride;
    int pixelByteLength = ::UncompressedPixelByteLength( pixelFormat );

    // TODO: Write lods?

    int lod = 0;

    for ( int face = 0 ; face < 6 ; face++ ) {
        byte * pSrc = ( byte * )faces[face];

        w = FMath::Max( 1, width >> lod );

        void * pPixels = WriteTextureData( 0, 0, face, w, w, lod );
        if ( pPixels ) {
            stride = w * w * pixelByteLength;

            memcpy( pPixels, pSrc, stride );
        }
    }

    return true;
}

void FTexture::Initialize1D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_Texture::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    Purge();

    data.TextureType = _ArrayLength > 1 ? TEXTURE_1D_ARRAY : TEXTURE_1D;
    data.PixelFormat = _PixelFormat;
    data.NumLods = _NumLods;
    data.Width = _Width;
    data.Height = _ArrayLength;
    data.Depth = 1;
    data.Chunks = nullptr;
    data.ChunksTail  = nullptr;
    data.bReallocated = true;

    TextureType = data.TextureType;
    PixelFormat = data.PixelFormat;
    Width = data.Width;
    Height = data.Height;
    Depth = data.Depth;
    NumLods = data.NumLods;

    RenderProxy->MarkUpdated();
}

void FTexture::Initialize2D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArrayLength ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_Texture::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    Purge();

    data.TextureType = _ArrayLength > 1 ? TEXTURE_2D_ARRAY : TEXTURE_2D;
    data.PixelFormat = _PixelFormat;
    data.NumLods = _NumLods;
    data.Width = _Width;
    data.Height = _Height;
    data.Depth = _ArrayLength;
    data.Chunks = nullptr;
    data.ChunksTail  = nullptr;
    data.bReallocated = true;

    TextureType = data.TextureType;
    PixelFormat = data.PixelFormat;
    Width = data.Width;
    Height = data.Height;
    Depth = data.Depth;
    NumLods = data.NumLods;

    RenderProxy->MarkUpdated();
}

void FTexture::Initialize3D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_Texture::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    Purge();

    data.TextureType = TEXTURE_3D;
    data.PixelFormat = _PixelFormat;
    data.NumLods = _NumLods;
    data.Width = _Width;
    data.Height = _Height;
    data.Depth = _Depth;
    data.Chunks = nullptr;
    data.ChunksTail  = nullptr;
    data.bReallocated = true;

    TextureType = data.TextureType;
    PixelFormat = data.PixelFormat;
    Width = data.Width;
    Height = data.Height;
    Depth = data.Depth;
    NumLods = data.NumLods;

    RenderProxy->MarkUpdated();
}

void FTexture::InitializeCubemap( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_Texture::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    Purge();

    data.TextureType = _ArrayLength > 1 ? TEXTURE_CUBEMAP_ARRAY : TEXTURE_CUBEMAP;
    data.PixelFormat = _PixelFormat;
    data.NumLods = _NumLods;
    data.Width = _Width;
    data.Height = _Width;
    data.Depth = _ArrayLength;
    data.Chunks = nullptr;
    data.ChunksTail  = nullptr;
    data.bReallocated = true;

    TextureType = data.TextureType;
    PixelFormat = data.PixelFormat;
    Width = data.Width;
    Height = data.Height;
    Depth = data.Depth;
    NumLods = data.NumLods;

    RenderProxy->MarkUpdated();
}

void FTexture::InitializeRect( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_Texture::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    Purge();

    data.TextureType = TEXTURE_RECT;
    data.PixelFormat = _PixelFormat;
    data.NumLods = _NumLods;
    data.Width = _Width;
    data.Height = _Height;
    data.Depth = 1;
    data.Chunks = nullptr;
    data.ChunksTail  = nullptr;
    data.bReallocated = true;

    TextureType = data.TextureType;
    PixelFormat = data.PixelFormat;
    Width = data.Width;
    Height = data.Height;
    Depth = data.Depth;
    NumLods = data.NumLods;

    RenderProxy->MarkUpdated();
}

void FTexture::InitializeInternalTexture( const char * _Name ) {
    if ( !FString::Cmp( _Name, "*white*" ) ) {
        // White texture

        byte data[ 1 * 1 * 3 ];
        memset( data, 0xff, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        void * pixels = WriteTextureData( 0, 0, 0, 1, 1, 0 );
        if ( pixels ) {
            memcpy( pixels, data, 3 );
        }
        SetName( _Name );
        return;
    }

    if ( !FString::Cmp( _Name, "*black*" ) ) {
        // Black texture

        byte data[ 1 * 1 * 3 ];
        memset( data, 0x0, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        void * pixels = WriteTextureData( 0, 0, 0, 1, 1, 0 );
        if ( pixels ) {
            memcpy( pixels, data, 3 );
        }
        SetName( _Name );
        return;
    }

    if ( !FString::Cmp( _Name, "*gray*" ) ) {
        // Black texture

        byte data[ 1 * 1 * 3 ];
        memset( data, 127, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        void * pixels = WriteTextureData( 0, 0, 0, 1, 1, 0 );
        if ( pixels ) {
            memcpy( pixels, data, 3 );
        }
        SetName( _Name );
        return;
    }

    if ( !FString::Cmp( _Name, "*normal*" ) ) {
        // Normal texture

        byte data[ 1 * 1 * 3 ];
        data[ 0 ] = 255; // z
        data[ 1 ] = 127; // y
        data[ 2 ] = 127; // x

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        void * pixels = WriteTextureData( 0, 0, 0, 1, 1, 0 );
        if ( pixels ) {
            memcpy( pixels, data, 3 );
        }
        SetName( _Name );
        return;
    }

    if ( !FString::Cmp( _Name, "*cubemap*" ) ) {
        // Cubemap texture

        const Float3 dirs[6] = {
            Float3(1,0,0),
            Float3(-1,0,0),
            Float3(0,1,0),
            Float3(0,-1,0),
            Float3(0,0,1),
            Float3(0,0,-1)
        };

        byte data[ 6 ][ 3 ];
        for ( int i = 0 ; i < 6 ; i++ ) {
            data[i][0] = ( dirs[i].Z + 1.0f ) * 127.5f;
            data[i][1] = ( dirs[i].Y + 1.0f ) * 127.5f;
            data[i][2] = ( dirs[i].X + 1.0f ) * 127.5f;
        }

        InitializeCubemap( TEXTURE_PF_BGR8, 1, 1 );

        for ( int face = 0 ; face < 6 ; face++ ) {
            void * pixels = WriteTextureData( 0, 0, face, 1, 1, 0 );
            if ( pixels ) {
                memcpy( pixels, data[face], 3 );
            }
        }
        SetName( _Name );
        return;
    }

    GLogger.Printf( "Unknown internal texture %s\n", _Name );
}

void FTexture::InitializeDefaultObject() {
    InitializeInternalTexture( "*white*" );
}

bool FTexture::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    FImage image;

    if ( !image.LoadRawImage( _Path, true, true ) ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    if ( !InitializeFromImage( image ) ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    return true;
}

void * FTexture::WriteTextureData( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod ) {
    if ( !Width ) {
        GLogger.Printf( "FTexture::WriteTextureData: texture is not initialized\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_Texture::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    data.TextureType = TextureType;
    data.PixelFormat = PixelFormat;
    data.Width = Width;
    data.Height = Height;
    data.Depth = Depth;
    data.NumLods = NumLods;

    int bytesToAllocate = _Width * _Height;

    if ( IsCompressed() ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        bytesToAllocate *= UncompressedPixelByteLength();
    }

    FTextureChunk * chunk = ( FTextureChunk * )frameData->AllocFrameData( sizeof( FTextureChunk ) - sizeof( int ) + bytesToAllocate );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->LocationX = _LocationX;
    chunk->LocationY = _LocationY;
    chunk->LocationZ = _LocationZ;
    chunk->Width = _Width;
    chunk->Height = _Height;
    chunk->LodNum = _Lod;

    IntrusiveAddToList( chunk, Next, Prev, data.Chunks, data.ChunksTail );

    RenderProxy->MarkUpdated();

    return &chunk->Pixels[0];
}

int FTexture::GetDimensionCount() const {
    if ( !Width ) {
        return 0;
    }

    switch ( TextureType ) {
    case TEXTURE_1D:
    case TEXTURE_1D_ARRAY:
        return 1;
    case TEXTURE_2D:
    //case TEXTURE_2D_MULTISAMPLE:
    case TEXTURE_2D_ARRAY:
    //case TEXTURE_2D_ARRAY_MULTISAMPLE:
    case TEXTURE_CUBEMAP:
    case TEXTURE_CUBEMAP_ARRAY:
    case TEXTURE_RECT:
        return 2;
    case TEXTURE_3D:
        return 3;
    default:
        AN_Assert( 0 );
        break;
    }

    return 0;
}

bool FTexture::IsCubemap() const {
    return TextureType == TEXTURE_CUBEMAP
            || TextureType == TEXTURE_CUBEMAP_ARRAY;
}

size_t FTexture::TextureByteLength1D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength ) {
    bool bCompressed = IsTextureCompressed( _PixelFormat );

    if ( bCompressed ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width );
            _Width >>= 1;
        }
        return ::UncompressedPixelByteLength( _PixelFormat ) * sum * FMath::Max( _ArrayLength, 1 );
    }
}

size_t FTexture::TextureByteLength2D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArrayLength ) {
    bool bCompressed = IsTextureCompressed( _PixelFormat );

    if ( bCompressed ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width ) * FMath::Max( 1, _Height );
            _Width >>= 1;
            _Height >>= 1;
        }
        return ::UncompressedPixelByteLength( _PixelFormat ) * sum * FMath::Max( _ArrayLength, 1 );
    }
}

size_t FTexture::TextureByteLength3D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    bool bCompressed = IsTextureCompressed( _PixelFormat );

    if ( bCompressed ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width ) * FMath::Max( 1, _Height ) * FMath::Max( 1, _Depth );
            _Width >>= 1;
            _Height >>= 1;
            _Depth >>= 1;
        }
        return ::UncompressedPixelByteLength( _PixelFormat ) * sum;
    }
}

size_t FTexture::TextureByteLengthCubemap( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength ) {
    bool bCompressed = IsTextureCompressed( _PixelFormat );

    if ( bCompressed ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width ) * FMath::Max( 1, _Width );
            _Width >>= 1;
        }
        return ::UncompressedPixelByteLength( _PixelFormat ) * sum * 6 * FMath::Max( _ArrayLength, 1 );
    }
}

size_t FTexture::TextureByteLengthRect( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    bool bCompressed = IsTextureCompressed( _PixelFormat );

    if ( bCompressed ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width ) * FMath::Max( 1, _Height );
            _Width >>= 1;
            _Height >>= 1;
        }
        return ::UncompressedPixelByteLength( _PixelFormat ) * sum;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Image
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#define STBI_MALLOC(sz)                     AllocateBufferData( sz )
#define STBI_FREE(p)                        DeallocateBufferData( p )
#define STBI_REALLOC_SIZED(p,oldsz,newsz)   ExtendBufferData(p,oldsz,newsz,false)
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
//#define STBI_NO_STDIO

#ifdef AN_COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4456 )
#endif
#include <stb_image.h>
#ifdef AN_COMPILER_MSVC
#pragma warning( pop )
#endif

FImage::FImage() {
    pRawData = nullptr;
    Width = 0;
    Height = 0;
    NumChannels = 0;
    bHDRI = false;
    bLinearSpace = false;
    bHalf = false;
    NumLods = 0;
}

FImage::~FImage() {
    Free();
}

static int Stbi_Read( void *user, char *data, int size ) {
    FFileStream & stream = *( FFileStream * )user;

    stream.Read( data, size );
    return stream.GetReadBytesCount();
}

static void Stbi_Skip( void *user, int n ) {
    FFileStream & stream = *( FFileStream * )user;

    stream.SeekCur( n );
}

static int Stbi_Eof( void *user ) {
    FFileStream & stream = *( FFileStream * )user;

    return stream.Eof();
}

static int Stbi_Read_Mem( void *user, char *data, int size ) {
    FMemoryStream & stream = *( FMemoryStream * )user;

    stream.Read( data, size );
    return stream.GetReadBytesCount();
}

static void Stbi_Skip_Mem( void *user, int n ) {
    FMemoryStream & stream = *( FMemoryStream * )user;

    stream.SeekCur( n );
}

static int Stbi_Eof_Mem( void *user ) {
    FMemoryStream & stream = *( FMemoryStream * )user;

    return stream.Eof();
}

bool FImage::LoadRawImage( const char * _Path, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    FFileStream stream;

    Free();

    if ( !stream.OpenRead( _Path ) ) {
        return false;
    }

    return LoadRawImage( stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

static bool LoadRawImage( const char * _Name, FImage & _Image, const stbi_io_callbacks * _Callbacks, void * _User, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    AN_Assert( _NumDesiredChannels >= 0 && _NumDesiredChannels <= 4 );

    _Image.Free();

    if ( _SRGB ) {
        if ( _NumDesiredChannels == 1 ) {
            _NumDesiredChannels = 3;
        } else if ( _NumDesiredChannels == 2 || _NumDesiredChannels == 0 ) {
            _NumDesiredChannels = 4;
        }
    }

    stbi_uc * data = stbi_load_from_callbacks( _Callbacks, _User, &_Image.Width, &_Image.Height, &_Image.NumChannels, _NumDesiredChannels );
    if ( !data ) {
        GLogger.Printf( "FImage::LoadRawImage: couldn't load %s\n", _Name );
        return false;
    }
    _Image.bHDRI = false;
    _Image.bLinearSpace = !_SRGB;
    _Image.bHalf = false;
    _Image.NumLods = 1;
    if ( _NumDesiredChannels > 0 ) {
        _Image.NumChannels = _NumDesiredChannels;
    }

    if ( _Image.NumChannels > 2 ) {
        // swap r & b channels to store image as BGR
        int count = _Image.Width * _Image.Height * _Image.NumChannels;
        for ( int i = 0 ; i < count ; i += _Image.NumChannels ) {
            std::swap( data[i], data[i+2] );
        }
    }

    if ( _GenerateMipmaps ) {
        FSoftwareMipmapGenerator mipmapGen;

        mipmapGen.SourceImage = data;
        mipmapGen.Width = _Image.Width;
        mipmapGen.Height = _Image.Height;
        mipmapGen.NumChannels = _Image.NumChannels;
        mipmapGen.bLinearSpace = _Image.bLinearSpace;
        mipmapGen.bHDRI = false;

        int requiredMemorySize;
        mipmapGen.ComputeRequiredMemorySize( requiredMemorySize, _Image.NumLods );

        _Image.pRawData = AllocateBufferData( requiredMemorySize );

        mipmapGen.GenerateMipmaps( _Image.pRawData );

        DeallocateBufferData( data );
    } else {
        _Image.pRawData = data;
    }

    return true;
}

static bool LoadRawImageHDRI( const char * _Name, FImage & _Image, const stbi_io_callbacks * _Callbacks, void * _User, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    AN_Assert( _NumDesiredChannels >= 0 && _NumDesiredChannels <= 4 );

    _Image.Free();

    float * data = stbi_loadf_from_callbacks( _Callbacks, _User, &_Image.Width, &_Image.Height, &_Image.NumChannels, _NumDesiredChannels );
    if ( !data ) {
        GLogger.Printf( "FImage::LoadRawImageHDRI: couldn't load %s\n", _Name );
        return false;
    }
    _Image.bHDRI = true;
    _Image.bLinearSpace = true;
    _Image.bHalf = _HalfFloat;
    _Image.NumLods = 1;

    if ( _NumDesiredChannels > 0 ) {
        _Image.NumChannels = _NumDesiredChannels;
    }

    if ( _Image.NumChannels > 2 ) {
        // swap r & b channels to store image as BGR
        int count = _Image.Width * _Image.Height * _Image.NumChannels;
        for ( int i = 0 ; i < count ; i += _Image.NumChannels ) {
            std::swap( data[i], data[i+2] );
        }
    }

    if ( _GenerateMipmaps ) {
        FSoftwareMipmapGenerator mipmapGen;

        mipmapGen.SourceImage = data;
        mipmapGen.Width = _Image.Width;
        mipmapGen.Height = _Image.Height;
        mipmapGen.NumChannels = _Image.NumChannels;
        mipmapGen.bLinearSpace = _Image.bLinearSpace;
        mipmapGen.bHDRI = true;

        int requiredMemorySize;
        mipmapGen.ComputeRequiredMemorySize( requiredMemorySize, _Image.NumLods );

        void * tmp = AllocateBufferData( requiredMemorySize );

        mipmapGen.GenerateMipmaps( tmp );

        DeallocateBufferData( data );
        data = ( float * )tmp;
    }

    if ( _HalfFloat ) {
        int imageSize = 0;
        for ( int i = 0 ; ; i++ ) {
            int w = FMath::Max( 1, _Image.Width >> i );
            int h = FMath::Max( 1, _Image.Height >> i );
            imageSize += w * h;
            if ( w == 1 && h == 1 ) {
                break;
            }
        }
        imageSize *= _Image.NumChannels;

        uint16_t * tmp = ( uint16_t * )AllocateBufferData( imageSize * sizeof( uint16_t ) );
        Float::FloatToHalf( data, tmp, imageSize );
        DeallocateBufferData( data );
        data = ( float * )tmp;
    }

    _Image.pRawData = data;

    return true;
}

bool FImage::LoadRawImage( FFileStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read,
        Stbi_Skip,
        Stbi_Eof
    };

    return ::LoadRawImage( _Stream.GetFileName(), *this, &callbacks, &_Stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadRawImage( FMemoryStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read_Mem,
        Stbi_Skip_Mem,
        Stbi_Eof_Mem
    };

    return ::LoadRawImage( _Stream.GetFileName(), *this, &callbacks, &_Stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadRawImageHDRI( const char * _Path, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    FFileStream stream;

    Free();

    if ( !stream.OpenRead( _Path ) ) {
        return false;
    }

    return LoadRawImageHDRI( stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadRawImageHDRI( FFileStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read,
        Stbi_Skip,
        Stbi_Eof
    };

    return ::LoadRawImageHDRI( _Stream.GetFileName(), *this, &callbacks, &_Stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadRawImageHDRI( FMemoryStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read_Mem,
        Stbi_Skip_Mem,
        Stbi_Eof_Mem
    };

    return ::LoadRawImageHDRI( _Stream.GetFileName(), *this, &callbacks, &_Stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

void FImage::Free() {
    if ( pRawData ) {
        DeallocateBufferData( pRawData );
        pRawData = nullptr;
    }
    Width = 0;
    Height = 0;
    NumChannels = 0;
    bHDRI = false;
    bLinearSpace = false;
    bHalf = false;
    NumLods = 0;
}

AN_FORCEINLINE float ClampByte( const float & _Value ) {
    return Float( _Value ).Clamp( 0.0f, 255.0f );
}

AN_FORCEINLINE float ByteToFloat( const byte & _Color ) {
    return _Color / 255.0f;
}

AN_FORCEINLINE byte FloatToByte( const float & _Color ) {
    //return ClampByte( _Color * 255.0f );  // round to small
    return ClampByte( floorf( _Color * 255.0f + 0.5f ) ); // round to nearest
}

AN_FORCEINLINE byte ConvertToSRGB_UB( const float & _lRGB ) {
    return FloatToByte( ConvertToSRGB( _lRGB ) );
}

// TODO: Add other mipmap filters
static void DownscaleSimpleAverage( int _CurWidth, int _CurHeight, int _NewWidth, int _NewHeight, int _NumChannels, int _AlphaChannel, bool _LinearSpace, const byte * _SrcData, byte * _DstData ) {
    int Bpp = _NumChannels;

    if ( _CurWidth == _NewWidth && _CurHeight == _NewHeight ) {
        memcpy( _DstData, _SrcData, _NewWidth*_NewHeight*Bpp );
        return;
    }

    float a, b, c, d, avg;

    int i, j, x, y, ch, idx;

    for ( i = 0 ; i < _NewWidth ; i++ ) {
        for ( j = 0 ; j < _NewHeight ; j++ ) {

            idx = j * _NewWidth + i;

            for ( ch = 0 ; ch < _NumChannels ; ch++ ) {
                if ( _LinearSpace || ch == _AlphaChannel ) {
                    if ( _NewWidth == _CurWidth ) {
                        x = i;
                        y = j << 1;

                        a = _SrcData[ (y * _CurWidth + x)*Bpp + ch ];
                        c = _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ];

                        avg = ( a + c ) * 0.5f;

                    } else if ( _NewHeight == _CurHeight ) {
                        x = i << 1;
                        y = j;

                        a = _SrcData[ (y * _CurWidth + x)*Bpp + ch ];
                        b = _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ];

                        avg = ( a + b ) * 0.5f;

                    } else {
                        x = i << 1;
                        y = j << 1;

                        a = _SrcData[ (y * _CurWidth + x)*Bpp + ch ];
                        b = _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ];
                        c = _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ];
                        d = _SrcData[ ((y + 1) * _CurWidth + x + 1)*Bpp + ch ];

                        avg = ( a + b + c + d ) * 0.25f;
                    }

                    _DstData[ idx * Bpp + ch ] = ClampByte( floor( avg + 0.5f ) );
                } else {
                    if ( _NewWidth == _CurWidth ) {
                        x = i;
                        y = j << 1;

                        a = ConvertToRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x)*Bpp + ch ] ) );
                        c = ConvertToRGB( ByteToFloat( _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ] ) );

                        avg = ( a + c ) * 0.5f;

                    } else if ( _NewHeight == _CurHeight ) {
                        x = i << 1;
                        y = j;

                        a = ConvertToRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x)*Bpp + ch ] ) );
                        b = ConvertToRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ] ) );

                        avg = ( a + b ) * 0.5f;

                    } else {
                        x = i << 1;
                        y = j << 1;

                        a = ConvertToRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x)*Bpp + ch ] ) );
                        b = ConvertToRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ] ) );
                        c = ConvertToRGB( ByteToFloat( _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ] ) );
                        d = ConvertToRGB( ByteToFloat( _SrcData[ ((y + 1) * _CurWidth + x + 1)*Bpp + ch ] ) );

                        avg = ( a + b + c + d ) * 0.25f;
                    }

                    _DstData[ idx * Bpp + ch ] = ConvertToSRGB_UB( avg );
                }
            }
        }
    }
}

static void DownscaleSimpleAverageHDRI( int _CurWidth, int _CurHeight, int _NewWidth, int _NewHeight, int _NumChannels, const float * _SrcData, float * _DstData ) {
    int Bpp = _NumChannels;

    if ( _CurWidth == _NewWidth && _CurHeight == _NewHeight ) {
        memcpy( _DstData, _SrcData, _NewWidth*_NewHeight*Bpp*sizeof( float ) );
        return;
    }

    float a, b, c, d, avg;

    int i, j, x, y, ch, idx;

    for ( i = 0 ; i < _NewWidth ; i++ ) {
        for ( j = 0 ; j < _NewHeight ; j++ ) {

            idx = j * _NewWidth + i;

            for ( ch = 0 ; ch < _NumChannels ; ch++ ) {

                if ( _NewWidth == _CurWidth ) {
                    x = i;
                    y = j << 1;

                    a = _SrcData[ (y * _CurWidth + x)*Bpp + ch ];
                    c = _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ];

                    avg = ( a + c ) * 0.5f;

                } else if ( _NewHeight == _CurHeight ) {
                    x = i << 1;
                    y = j;

                    a = _SrcData[ (y * _CurWidth + x)*Bpp + ch ];
                    b = _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ];

                    avg = ( a + b ) * 0.5f;

                } else {
                    x = i << 1;
                    y = j << 1;

                    a = _SrcData[ (y * _CurWidth + x)*Bpp + ch ];
                    b = _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ];
                    c = _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ];
                    d = _SrcData[ ((y + 1) * _CurWidth + x + 1)*Bpp + ch ];

                    avg = ( a + b + c + d ) * 0.25f;
                }

                _DstData[ idx * Bpp + ch ] = avg;
            }
        }
    }
}

static void GenerateMipmaps( const byte * ImageData, int ImageWidth, int ImageHeight, int _NumChannels, bool _LinearSpace, byte * _Dest ) {
    memcpy( _Dest, ImageData, ImageWidth * ImageHeight * _NumChannels );

    int MemoryOffset = ImageWidth * ImageHeight * _NumChannels;

    int CurWidth = ImageWidth;
    int CurHeight = ImageHeight;

    int AlphaChannel = _NumChannels == 4 ? 3 : -1;

    for ( int i = 1 ; ; i++ ) {
        int LodWidth = FMath::Max( 1, ImageWidth >> i );
        int LodHeight = FMath::Max( 1, ImageHeight >> i );

        byte * LodData = _Dest + MemoryOffset;
        MemoryOffset += LodWidth * LodHeight * _NumChannels;

        DownscaleSimpleAverage( CurWidth, CurHeight, LodWidth, LodHeight, _NumChannels, AlphaChannel, _LinearSpace, ImageData, LodData );

        ImageData = LodData;

        CurWidth = LodWidth;
        CurHeight = LodHeight;

        if ( LodWidth == 1 && LodHeight == 1 ) {
            break;
        }
    }
}

static void GenerateMipmapsHDRI( const float * ImageData, int ImageWidth, int ImageHeight, int _NumChannels, float * _Dest ) {
    memcpy( _Dest, ImageData, ImageWidth * ImageHeight * _NumChannels * sizeof( float ) );

    int MemoryOffset = ImageWidth * ImageHeight * _NumChannels;

    int CurWidth = ImageWidth;
    int CurHeight = ImageHeight;

    for ( int i = 1 ; ; i++ ) {
        int LodWidth = FMath::Max( 1, ImageWidth >> i );
        int LodHeight = FMath::Max( 1, ImageHeight >> i );

        float * LodData = _Dest + MemoryOffset;
        MemoryOffset += LodWidth * LodHeight * _NumChannels;

        DownscaleSimpleAverageHDRI( CurWidth, CurHeight, LodWidth, LodHeight, _NumChannels, ImageData, LodData );

        ImageData = LodData;

        CurWidth = LodWidth;
        CurHeight = LodHeight;

        if ( LodWidth == 1 && LodHeight == 1 ) {
            break;
        }
    }
}

void FSoftwareMipmapGenerator::ComputeRequiredMemorySize( int & _RequiredMemory, int & _NumLods ) const {
    _RequiredMemory = 0;
    _NumLods = 0;

    for ( int i = 0 ; ; i++ ) {
        int LodWidth = FMath::Max( 1, Width >> i );
        int LodHeight = FMath::Max( 1, Height >> i );
        _RequiredMemory += LodWidth * LodHeight;
        _NumLods++;
        if ( LodWidth == 1 && LodHeight == 1 ) {
            break;
        }
    }

    _RequiredMemory *= NumChannels;

    if ( bHDRI ) {
        _RequiredMemory *= sizeof( float );
    }
}

void FSoftwareMipmapGenerator::GenerateMipmaps( void * _Data ) {
    if ( bHDRI ) {
        ::GenerateMipmapsHDRI( (const float *)SourceImage, Width, Height, NumChannels, (float *)_Data );
    } else {
        ::GenerateMipmaps( (const byte *)SourceImage, Width, Height, NumChannels, bLinearSpace, (byte *)_Data );
    }
}
