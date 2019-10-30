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

    if ( !image.LoadLDRI( _Path, false/*true*/, true ) ) {

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

    if ( image.LoadLDRI( _Path, true, false, 3 ) ) {
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

    mult.SetTemperature( FMath::Clamp( p.ColorTemperature, 1000.0f, 40000.0f ) );

    FColor4 c =_Color.GetRGB().Lerp( _Color.GetRGB() * mult.GetRGB(), p.ColorTemperatureStrength );

    float newLum = c.GetLuminance();

    c *= FMath::Lerp( 1.0, ( newLum > 1e-6 ) ? ( lum / newLum ) : 1.0, p.ColorTemperatureBrightnessNormalization );

    c = Float3( c.GetLuminance() ).Lerp( c.GetRGB(), p.Presaturation );

    Float3 t = ( p.Gain * 2.0f ) * ( c.GetRGB() + ( ( p.Lift * 2.0f - 1.0 ) * ( Float3( 1.0 ) - c.GetRGB() ) ) );

    t.X = std::pow( t.X, 0.5f / p.Gamma.X );
    t.Y = std::pow( t.Y, 0.5f / p.Gamma.Y );
    t.Z = std::pow( t.Z, 0.5f / p.Gamma.Z );

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
