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

#include <Engine/Resource/Public/Texture.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META( FTexture )
AN_CLASS_META( FTexture1D )
AN_CLASS_META( FTexture1DArray )
AN_CLASS_META( FTexture2D )
AN_CLASS_META( FTexture2DArray )
AN_CLASS_META( FTexture3D )
AN_CLASS_META( FTextureCubemap )
AN_CLASS_META( FTextureCubemapArray )
AN_CLASS_META( FTexture2DNPOT )

FTexture::FTexture() {
    TextureGPU = GRenderBackend->CreateTexture( this );
}

FTexture::~FTexture() {
    GRenderBackend->DestroyTexture( TextureGPU );
}

void FTexture::Purge() {

}

static bool GetAppropriatePixelFormat( FImage const & _Image, FTexturePixelFormat & _PixelFormat ) {
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
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
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
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
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
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
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
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
                return false;
            }
        }
    }

    return true;
}

bool FTexture2D::InitializeFromImage( FImage const & _Image ) {
    if ( !_Image.pRawData ) {
        GLogger.Printf( "FTexture2D::InitializeFromImage: empty image data\n" );
        return false;
    }

    FTexturePixelFormat pixelFormat;

    if ( !GetAppropriatePixelFormat( _Image, pixelFormat ) ) {
        return false;
    }

    Initialize( pixelFormat, _Image.NumLods, _Image.Width, _Image.Height );

    byte * pSrc = ( byte * )_Image.pRawData;
    int w, h, stride;
    int pixelByteLength = pixelFormat.SizeInBytesUncompressed();

    for ( int lod = 0 ; lod < _Image.NumLods ; lod++ ) {
        w = FMath::Max( 1, _Image.Width >> lod );
        h = FMath::Max( 1, _Image.Height >> lod );

        stride = w * h * pixelByteLength;

        WriteTextureData( 0, 0, w, h, lod, pSrc );

        pSrc += stride;
    }

    return true;
}

bool FTextureCubemap::InitializeCubemapFromImages( FImage const * _Faces[6] ) {
    const void * faces[6];

    int width = _Faces[0]->Width;

    for ( int i = 0 ; i < 6 ; i++ ) {

        if ( !_Faces[i]->pRawData ) {
            GLogger.Printf( "FTextureCubemap::InitializeCubemapFromImages: empty image data\n" );
            return false;
        }

        if ( _Faces[i]->Width != width
             || _Faces[i]->Height != width ) {
            GLogger.Printf( "FTextureCubemap::InitializeCubemapFromImages: faces with different sizes\n" );
            return false;
        }

        faces[i] = _Faces[i]->pRawData;
    }

    FTexturePixelFormat pixelFormat;

    if ( !GetAppropriatePixelFormat( *_Faces[0], pixelFormat ) ) {
        return false;
    }

    for ( int i = 1 ; i < 6 ; i++ ) {
        FTexturePixelFormat facePF;

        if ( !GetAppropriatePixelFormat( *_Faces[i], facePF ) ) {
            return false;
        }

        if ( pixelFormat != facePF ) {
            GLogger.Printf( "FTexture::InitializeCubemapFromImages: faces with different pixel formats\n" );
            return false;
        }
    }

    Initialize( pixelFormat, 1, width );

    int w;
    //int pixelByteLength = ::UncompressedPixelByteLength( pixelFormat );

    // TODO: Write lods?

    int lod = 0;

    for ( int face = 0 ; face < 6 ; face++ ) {
        byte * pSrc = ( byte * )faces[face];

        w = FMath::Max( 1, width >> lod );

        WriteTextureData( 0, 0, w, w, face, lod, pSrc );
    }

    return true;
}

//void FTexture::Initialize1D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
//    Purge();
//
//    TextureType = _ArraySize > 1 ? TEXTURE_1D_ARRAY : TEXTURE_1D;
//    PixelFormat = _PixelFormat;
//    Width = _Width;
//    Height = _ArraySize;
//    Depth = 1;
//    NumLods = _NumLods;
//
//    GRenderBackend->InitializeTexture1D( TextureGPU, _PixelFormat, _NumLods, _Width, _ArraySize );
//}

//void FTexture::Initialize2D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
//    Purge();

//    TextureType = _ArraySize > 1 ? TEXTURE_2D_ARRAY : TEXTURE_2D;
//    PixelFormat = _PixelFormat;
//    Width = _Width;
//    Height = _Height;
//    Depth = _ArraySize;
//    NumLods = _NumLods;

//    GRenderBackend->InitializeTexture2D( TextureGPU, _PixelFormat, _NumLods, _Width, _Height, _ArraySize );
//}

//void FTexture::Initialize3D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
//    Purge();

//    TextureType = TEXTURE_3D;
//    PixelFormat = _PixelFormat;
//    Width = _Width;
//    Height = _Height;
//    Depth = _Depth;
//    NumLods = _NumLods;

//    GRenderBackend->InitializeTexture3D( TextureGPU, _PixelFormat, _NumLods, _Width, _Height, _Depth );
//}

//void FTexture::InitializeCubemap( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
//    Purge();

//    TextureType = _ArraySize > 1 ? TEXTURE_CUBEMAP_ARRAY : TEXTURE_CUBEMAP;
//    PixelFormat = _PixelFormat;
//    Width = _Width;
//    Height = _Width;
//    Depth = _ArraySize;
//    NumLods = _NumLods;

//    GRenderBackend->InitializeTextureCubemap( TextureGPU, _PixelFormat, _NumLods, _Width, _ArraySize );
//}

//void FTexture::InitializeRect( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
//    Purge();
//
//    TextureType = TEXTURE_2DNPOT;
//    PixelFormat = _PixelFormat;
//    Width = _Width;
//    Height = _Height;
//    Depth = 1;
//    NumLods = _NumLods;
//
//    GRenderBackend->InitializeTexture2DNPOT( TextureGPU, _PixelFormat, _NumLods, _Width, _Height );
//}

void FTexture2D::InitializeInternalResource( const char * _InternalResourceName ) {
    if ( !FString::Icmp( _InternalResourceName, "FTexture2D.White" )
        || !FString::Icmp( _InternalResourceName, "FTexture2D.Default" ) ) {

        // White texture

        byte data[ 1 * 1 * 3 ];
        memset( data, 0xff, sizeof( data ) );

        Initialize( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FTexture2D.Black" ) ) {
        // Black texture

        byte data[ 1 * 1 * 3 ];
        memset( data, 0x0, sizeof( data ) );

        Initialize( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FTexture2D.Gray" ) ) {
        // Black texture

        byte data[ 1 * 1 * 3 ];
        memset( data, 127, sizeof( data ) );

        Initialize( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FTexture2D.Normal" ) ) {
        // Normal texture

        byte data[ 1 * 1 * 3 ];
        data[ 0 ] = 255; // z
        data[ 1 ] = 127; // y
        data[ 2 ] = 127; // x

        Initialize( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData( 0, 0, 1, 1, 0, data );

        return;
    }

    GLogger.Printf( "Unknown internal texture %s\n", _InternalResourceName );
}

void FTextureCubemap::InitializeInternalResource( const char * _InternalResourceName ) {
    if ( !FString::Icmp( _InternalResourceName, "FTextureCubemap.Default" ) ) {
        // Cubemap texture

        const Float3 dirs[6] = {
            Float3( 1,0,0 ),
            Float3( -1,0,0 ),
            Float3( 0,1,0 ),
            Float3( 0,-1,0 ),
            Float3( 0,0,1 ),
            Float3( 0,0,-1 )
        };

        byte data[6][3];
        for ( int i = 0 ; i < 6 ; i++ ) {
            data[i][0] = (dirs[i].Z + 1.0f) * 127.5f;
            data[i][1] = (dirs[i].Y + 1.0f) * 127.5f;
            data[i][2] = (dirs[i].X + 1.0f) * 127.5f;
        }

        Initialize( TEXTURE_PF_BGR8, 1, 1 );

        for ( int face = 0 ; face < 6 ; face++ ) {
            WriteTextureData( 0, 0, 1, 1, face, 0, data[face] );
        }
        return;
    }

    GLogger.Printf( "Unknown internal texture %s\n", _InternalResourceName );
}


bool FTexture2D::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
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

void FTexture::SendTextureDataToGPU( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod, const void * _SysMem ) {
    if ( !Width ) {
        GLogger.Printf( "FTexture::SendTextureDataToGPU: texture is not initialized\n" );
        return;
    }

    size_t sizeInBytes = _Width * _Height;

    if ( IsCompressed() ) {
        // TODO
        AN_Assert( 0 );
        return;
    } else {
        sizeInBytes *= SizeInBytesUncompressed();
    }

    FTextureRect rect;

    rect.Offset.X = _LocationX;
    rect.Offset.Y = _LocationY;
    rect.Offset.Z = _LocationZ;
    rect.Offset.Lod = _Lod;
    rect.Dimension.X = _Width;
    rect.Dimension.Y = _Height;
    rect.Dimension.Z = 1;

    GRenderBackend->WriteTexture( TextureGPU, rect, PixelFormat.Data, sizeInBytes, 1, _SysMem );
}

bool FTexture::IsCubemap() const {
    return TextureType == TEXTURE_CUBEMAP || TextureType == TEXTURE_CUBEMAP_ARRAY;
}

size_t FTexture::TextureByteLength1D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width );
            _Width >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * FMath::Max( _ArraySize, 1 );
    }
}

size_t FTexture::TextureByteLength2D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
    if ( _PixelFormat.IsCompressed() ) {
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
        return _PixelFormat.SizeInBytesUncompressed() * sum * FMath::Max( _ArraySize, 1 );
    }
}

size_t FTexture::TextureByteLength3D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    if ( _PixelFormat.IsCompressed() ) {
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
        return _PixelFormat.SizeInBytesUncompressed() * sum;
    }
}

size_t FTexture::TextureByteLengthCubemap( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_Assert(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += FMath::Max( 1, _Width ) * FMath::Max( 1, _Width );
            _Width >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * 6 * FMath::Max( _ArraySize, 1 );
    }
}

size_t FTexture::TextureByteLength2DNPOT( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    if ( _PixelFormat.IsCompressed() ) {
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
        return _PixelFormat.SizeInBytesUncompressed() * sum;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Image
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#define STBI_MALLOC(sz)                     HugeAlloc( sz )
#define STBI_FREE(p)                        HugeFree( p )
#define STBI_REALLOC_SIZED(p,oldsz,newsz)   stb_realloc_impl(p,newsz,oldsz)

static void * stb_realloc_impl( void * p, size_t newsz, size_t oldsz = 0 ) {
    void * newp = HugeAlloc( newsz );
    if ( oldsz ) {
        memcpy( newp, p, FMath::Min( oldsz, newsz ) );
    }
    HugeFree( p );
    return newp;
}

#define STBI_REALLOC(p,newsz) stb_realloc_impl(p, newsz)
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
            StdSwap( data[i], data[i+2] );
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

        _Image.pRawData = HugeAlloc( requiredMemorySize );

        mipmapGen.GenerateMipmaps( _Image.pRawData );

        HugeFree( data );
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
            StdSwap( data[i], data[i+2] );
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

        void * tmp = HugeAlloc( requiredMemorySize );

        mipmapGen.GenerateMipmaps( tmp );

        HugeFree( data );
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

        uint16_t * tmp = ( uint16_t * )HugeAlloc( imageSize * sizeof( uint16_t ) );
        FMath::FloatToHalf( data, tmp, imageSize );
        HugeFree( data );
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
        HugeFree( pRawData );
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
    return FMath::Clamp( _Value, 0.0f, 255.0f );
}

AN_FORCEINLINE float ByteToFloat( const byte & _Color ) {
    return _Color / 255.0f;
}

AN_FORCEINLINE byte FloatToByte( const float & _Color ) {
    //return ClampByte( _Color * 255.0f );  // round to small
    return ClampByte( FMath::Floor( _Color * 255.0f + 0.5f ) ); // round to nearest
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

                    _DstData[ idx * Bpp + ch ] = ClampByte( FMath::Floor( avg + 0.5f ) );
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






















void FTexture1D::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    Purge();

    TextureType = TEXTURE_1D;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = 1;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture1D( TextureGPU, _PixelFormat.Data, _NumLods, _Width );
}

void FTexture1D::WriteTextureData( int _LocationX, int _Width, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, 0, 0, _Width, 1, _Lod, _SysMem );
}

void FTexture1DArray::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    Purge();

    TextureType = TEXTURE_1D_ARRAY;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _ArraySize;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture1DArray( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _ArraySize );
}

void FTexture1DArray::WriteTextureData( int _LocationX, int _Width, int _ArrayLayer, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _ArrayLayer, 0, _Width, 1, _Lod, _SysMem );
}

void FTexture2D::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    Purge();

    TextureType = TEXTURE_2D;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture2D( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height );
}

void FTexture2D::WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _LocationY, 0, _Width, _Height, _Lod, _SysMem );
}

void FTexture2DArray::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
    Purge();

    TextureType = TEXTURE_2D_ARRAY;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = _ArraySize;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture2DArray( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height, _ArraySize );
}

void FTexture2DArray::WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _ArrayLayer, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _LocationY, _ArrayLayer, _Width, _Height, _Lod, _SysMem );
}

void FTexture3D::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    Purge();

    TextureType = TEXTURE_3D;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = _Depth;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture3D( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height, _Depth );
}

void FTexture3D::InitializeInternalResource( const char * _InternalResourceName ) {
    if ( !FString::Icmp( _InternalResourceName, "FTexture3D.LUT1" )
        || !FString::Icmp( _InternalResourceName, "FTexture3D.Default" ) ) {

        constexpr FColorGradingPreset ColorGradingPreset1 = {
              Float3(0.5f),   // Gain
              Float3(0.5f),   // Gamma
              Float3(0.5f),   // Lift
              Float3(1.0f),   // Presaturation
              Float3(0.0f),   // Color temperature strength
              6500.0f,      // Color temperature (in K)
              0.0f          // Color temperature brightness normalization factor
            };

        InitializeColorGradingLUT( ColorGradingPreset1 );

        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FTexture3D.LUT2" ) ) {
        constexpr FColorGradingPreset ColorGradingPreset2 = {
              Float3(0.5f),   // Gain
              Float3(0.5f),   // Gamma
              Float3(0.5f),   // Lift
              Float3(1.0f),   // Presaturation
              Float3(1.0f),   // Color temperature strength
              3500.0f,      // Color temperature (in K)
              1.0f          // Color temperature brightness normalization factor
            };

        InitializeColorGradingLUT( ColorGradingPreset2 );

        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FTexture3D.LUT3" ) ) {
        constexpr FColorGradingPreset ColorGradingPreset3 = {
            Float3(0.51f, 0.55f, 0.53f), // Gain
            Float3(0.45f, 0.57f, 0.55f), // Gamma
            Float3(0.5f,  0.4f,  0.6f),  // Lift
            Float3(1.0f,  0.9f,  0.8f),  // Presaturation
            Float3(1.0f,  1.0f,  1.0f),  // Color temperature strength
            6500.0,                 // Color temperature (in K)
            0.0                     // Color temperature brightness normalization factor
          };

        InitializeColorGradingLUT( ColorGradingPreset3 );

        return;
    }

    GLogger.Printf( "Unknown internal texture %s\n", _InternalResourceName );
}

void FTexture3D::InitializeColorGradingLUT( const char * _Path ) {
    FImage image;
    byte data[ 16 ][ 16 ][ 16 ][ 3 ];

    Initialize( TEXTURE_PF_BGR8_SRGB, 1, 16, 16, 16 );

    if ( image.LoadRawImage( _Path, true, false, 3 ) ) {
        for ( int z = 0 ; z < 16 ; z++ ) {
            for ( int y = 0 ; y < 16 ; y++ ) {
                memcpy( &data[ z ][ y ][ 0 ][ 0 ], static_cast< byte * >(image.pRawData) + ( z * 16 * 16 * 3 + y * 16 * 3 ), 16 * 3 );
            }
        }
    } else {
        // Initialize default color grading
        for ( int z = 0 ; z < 16 ; z++ ) {
            for ( int y = 0 ; y < 16 ; y++ ) {
                for ( int x = 0 ; x < 16 ; x++ ) {
                    //data[ z ][ y ][ x ][ 0 ] = z * (255.0f / 15.0f);
                    //data[ z ][ y ][ x ][ 1 ] = y * (255.0f / 15.0f);
                    //data[ z ][ y ][ x ][ 2 ] = x * (255.0f / 15.0f);

                    // Luminance
                    data[ z ][ y ][ x ][ 0 ] =
                    data[ z ][ y ][ x ][ 1 ] =
                    data[ z ][ y ][ x ][ 2 ] = FMath::Clamp( x * ( 0.2126f / 15.0f * 255.0f ) + y * ( 0.7152f / 15.0f * 255.0f ) + z * ( 0.0722f / 15.0f * 255.0f ), 0.0f, 255.0f );
                }
            }
        }
    }

    FTextureRect rect;

    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Offset.Lod = 0;
    rect.Dimension.X = 16;
    rect.Dimension.Y = 16;
    rect.Dimension.Z = 16;

    GRenderBackend->WriteTexture( TextureGPU, rect, PixelFormat.Data, 16*16*16*3, 1, data );
}

static Float3 ApplyColorGrading( FColorGradingPreset const & p, FColor4 const & _Color ) {
    float lum = _Color.GetLuminance();

    FColor4 mult;

    mult.SetTemperature( FMath::Clamp( p.colorTemperature, 1000.0f, 40000.0f ) );

    FColor4 c =_Color.GetRGB().Lerp( _Color.GetRGB() * mult.GetRGB(), p.colorTemperatureStrength );

    float newLum = c.GetLuminance();

    c *= FMath::Lerp( 1.0, ( newLum > 1e-6 ) ? ( lum / newLum ) : 1.0, p.colorTemperatureBrightnessNormalization );

    c = Float3( c.GetLuminance() ).Lerp( c.GetRGB(), p.presaturation );

    Float3 t = ( p.gain * 2.0f ) * ( c.GetRGB() + ( ( p.lift * 2.0f - 1.0 ) * ( Float3( 1.0 ) - c.GetRGB() ) ) );

    t.X = std::pow( t.X, 0.5f / p.gamma.X );
    t.Y = std::pow( t.Y, 0.5f / p.gamma.Y );
    t.Z = std::pow( t.Z, 0.5f / p.gamma.Z );

    return t;
}

void FTexture3D::InitializeColorGradingLUT( FColorGradingPreset const & _Preset ) {
    byte data[ 16 ][ 16 ][ 16 ][ 3 ];
    FColor4 color;
    Float3 result;

    Initialize( TEXTURE_PF_BGR8_SRGB, 1, 16, 16, 16 );

    for ( int z = 0 ; z < 16 ; z++ ) {
        color.Z = ( 1.0f / 15.0f ) * z;

        for ( int y = 0 ; y < 16 ; y++ ) {
            color.Y = ( 1.0f / 15.0f ) * y;

            for ( int x = 0 ; x < 16 ; x++ ) {
                color.X = ( 1.0f / 15.0f ) * x;

                result = ApplyColorGrading( _Preset, color ) * 255.0f;

                data[ z ][ y ][ x ][ 0 ] = FMath::Clamp( result.Z, 0.0f, 255.0f );
                data[ z ][ y ][ x ][ 1 ] = FMath::Clamp( result.Y, 0.0f, 255.0f );
                data[ z ][ y ][ x ][ 2 ] = FMath::Clamp( result.X, 0.0f, 255.0f );
            }
        }
    }

    FTextureRect rect;

    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Offset.Lod = 0;
    rect.Dimension.X = 16;
    rect.Dimension.Y = 16;
    rect.Dimension.Z = 16;

    GRenderBackend->WriteTexture( TextureGPU, rect, PixelFormat.Data, 16*16*16*3, 1, data );
}

void FTexture3D::WriteTextureData( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _LocationY, _LocationZ, _Width, _Height, _Lod, _SysMem );
}

void FTextureCubemap::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    Purge();

    TextureType = TEXTURE_CUBEMAP;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Width;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTextureCubemap( TextureGPU, _PixelFormat.Data, _NumLods, _Width );
}

void FTextureCubemap::WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _LocationY, _FaceIndex, _Width, _Height, _Lod, _SysMem );
}

void FTextureCubemapArray::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    Purge();

    TextureType = TEXTURE_CUBEMAP_ARRAY;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Width;
    Depth = _ArraySize;
    NumLods = _NumLods;

    GRenderBackend->InitializeTextureCubemapArray( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _ArraySize );
}

void FTextureCubemapArray::WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _ArrayLayer, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _LocationY, _ArrayLayer*6 + _FaceIndex, _Width, _Height, _Lod, _SysMem );
}

void FTexture2DNPOT::Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    Purge();

    TextureType = TEXTURE_2DNPOT;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture2DNPOT( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height );
}

void FTexture2DNPOT::WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void * _SysMem ) {
    SendTextureDataToGPU( _LocationX, _LocationY, 0, _Width, _Height, _Lod, _SysMem );
}
