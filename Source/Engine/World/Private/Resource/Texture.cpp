/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Resource/Texture.h>
#include <World/Public/Resource/Asset.h>
#include <Runtime/Public/ScopedTimeCheck.h>
#include <Core/Public/Logger.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

static const char * TextureTypeName[] =
{
    "TEXTURE_1D",
    "TEXTURE_1D_ARRAY",
    "TEXTURE_2D",
    "TEXTURE_2D_ARRAY",
    "TEXTURE_3D",
    "TEXTURE_CUBEMAP",
    "TEXTURE_CUBEMAP_ARRAY",
    "TEXTURE_2DNPOT"
};

AN_CLASS_META( ATexture )

ATexture::ATexture() {
    TextureGPU = GRenderBackend->CreateTexture( this );
}

ATexture::~ATexture() {
    GRenderBackend->DestroyTexture( TextureGPU );
}

void ATexture::Purge() {

}

bool ATexture::InitializeFromImage( AImage const & _Image ) {
    if ( !_Image.pRawData ) {
        GLogger.Printf( "ATexture::InitializeFromImage: empty image data\n" );
        return false;
    }

    STexturePixelFormat pixelFormat;

    if ( !STexturePixelFormat::GetAppropriatePixelFormat( _Image, pixelFormat ) ) {
        return false;
    }

    Initialize2D( pixelFormat, _Image.NumLods, _Image.Width, _Image.Height );

    byte * pSrc = ( byte * )_Image.pRawData;
    int w, h, stride;
    int pixelByteLength = pixelFormat.SizeInBytesUncompressed();

    for ( int lod = 0 ; lod < _Image.NumLods ; lod++ ) {
        w = Math::Max( 1, _Image.Width >> lod );
        h = Math::Max( 1, _Image.Height >> lod );

        stride = w * h * pixelByteLength;

        WriteTextureData2D( 0, 0, w, h, lod, pSrc );

        pSrc += stride;
    }

    return true;
}

bool ATexture::InitializeCubemapFromImages( AImage const * _Faces[6] ) {
    const void * faces[6];

    int width = _Faces[0]->Width;

    for ( int i = 0 ; i < 6 ; i++ ) {

        if ( !_Faces[i]->pRawData ) {
            GLogger.Printf( "ATexture::InitializeCubemapFromImages: empty image data\n" );
            return false;
        }

        if ( _Faces[i]->Width != width
             || _Faces[i]->Height != width ) {
            GLogger.Printf( "ATexture::InitializeCubemapFromImages: faces with different sizes\n" );
            return false;
        }

        faces[i] = _Faces[i]->pRawData;
    }

    STexturePixelFormat pixelFormat;

    if ( !STexturePixelFormat::GetAppropriatePixelFormat( *_Faces[0], pixelFormat ) ) {
        return false;
    }

    for ( int i = 1 ; i < 6 ; i++ ) {
        STexturePixelFormat facePF;

        if ( !STexturePixelFormat::GetAppropriatePixelFormat( *_Faces[i], facePF ) ) {
            return false;
        }

        if ( pixelFormat != facePF ) {
            GLogger.Printf( "ATexture::InitializeCubemapFromImages: faces with different pixel formats\n" );
            return false;
        }
    }

    InitializeCubemap( pixelFormat, 1, width );

    for ( int face = 0 ; face < 6 ; face++ ) {
        WriteTextureDataCubemap( 0, 0, width, width, face, 0, (byte *)faces[face] );
    }

    return true;
}

void ATexture::LoadInternalResource( const char * _Path ) {
    if ( !Core::Stricmp( _Path, "/Default/Textures/White" ) ) {
        byte data[ 1 * 1 * 3 ];
        Core::Memset( data, 0xff, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData2D( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/Black" ) ) {
        byte data[ 1 * 1 * 3 ];
        Core::ZeroMem( data, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData2D( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/Gray" ) ) {
        byte data[ 1 * 1 * 3 ];
        Core::Memset( data, 127, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData2D( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/BaseColorWhite" )
         || !Core::Stricmp( _Path, "/Default/Textures/Default2D" ) ) {
        byte data[ 1 * 1 * 3 ];
        Core::Memset( data, 240, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData2D( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/BaseColorBlack" ) ) {
        byte data[ 1 * 1 * 3 ];
        Core::Memset( data, 30, sizeof( data ) );

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData2D( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/Normal" ) ) {
        byte data[ 1 * 1 * 3 ];
        data[ 0 ] = 255; // z
        data[ 1 ] = 127; // y
        data[ 2 ] = 127; // x

        Initialize2D( TEXTURE_PF_BGR8, 1, 1, 1 );
        WriteTextureData2D( 0, 0, 1, 1, 0, data );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/DefaultCubemap" ) ) {
        constexpr Float3 dirs[6] = {
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

        InitializeCubemap( TEXTURE_PF_BGR8, 1, 1 );

        for ( int face = 0 ; face < 6 ; face++ ) {
            WriteTextureDataCubemap( 0, 0, 1, 1, face, 0, data[face] );
        }
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/LUT1" )
        || !Core::Stricmp( _Path, "/Default/Textures/Default3D" ) ) {

        constexpr SColorGradingPreset ColorGradingPreset1 = {
            Float3( 0.5f ),   // Gain
            Float3( 0.5f ),   // Gamma
            Float3( 0.5f ),   // Lift
            Float3( 1.0f ),   // Presaturation
            Float3( 0.0f ),   // Color temperature strength
            6500.0f,          // Color temperature (in K)
            0.0f              // Color temperature brightness normalization factor
        };

        InitializeColorGradingLUT( ColorGradingPreset1 );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/LUT2" ) ) {
        constexpr SColorGradingPreset ColorGradingPreset2 = {
            Float3( 0.5f ),   // Gain
            Float3( 0.5f ),   // Gamma
            Float3( 0.5f ),   // Lift
            Float3( 1.0f ),   // Presaturation
            Float3( 1.0f ),   // Color temperature strength
            3500.0f,          // Color temperature (in K)
            1.0f              // Color temperature brightness normalization factor
        };

        InitializeColorGradingLUT( ColorGradingPreset2 );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/LUT3" ) ) {
        constexpr SColorGradingPreset ColorGradingPreset3 = {
            Float3( 0.51f, 0.55f, 0.53f ), // Gain
            Float3( 0.45f, 0.57f, 0.55f ), // Gamma
            Float3( 0.5f,  0.4f,  0.6f ),  // Lift
            Float3( 1.0f,  0.9f,  0.8f ),  // Presaturation
            Float3( 1.0f,  1.0f,  1.0f ),  // Color temperature strength
            6500.0,                        // Color temperature (in K)
            0.0                            // Color temperature brightness normalization factor
        };

        InitializeColorGradingLUT( ColorGradingPreset3 );

        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Textures/LUT_Luminance" ) ) {
        byte data[ 16 ][ 16 ][ 16 ][ 3 ];
        for ( int z = 0 ; z < 16 ; z++ ) {
            for ( int y = 0 ; y < 16 ; y++ ) {
                for ( int x = 0 ; x < 16 ; x++ ) {
                    data[ z ][ y ][ x ][ 0 ] =
                    data[ z ][ y ][ x ][ 1 ] =
                    data[ z ][ y ][ x ][ 2 ] = Math::Clamp( x * ( 0.2126f / 15.0f * 255.0f ) + y * ( 0.7152f / 15.0f * 255.0f ) + z * ( 0.0722f / 15.0f * 255.0f ), 0.0f, 255.0f );
                }
            }
        }
        Initialize3D( TEXTURE_PF_BGR8_SRGB, 1, 16, 16, 16 );
        WriteArbitraryData( 0, 0, 0, 16, 16, 16, 0, data );

        return;
    }

    GLogger.Printf( "Unknown internal texture %s\n", _Path );

    LoadInternalResource( "/Default/Textures/Default2D" );
}

bool ATexture::LoadResource( AString const & _Path ) {
    AScopedTimeCheck ScopedTime( _Path.CStr() );

    AImage image;

    int i = _Path.FindExt();
    if ( !Core::Stricmp( &_Path[i], ".jpg" )
         || !Core::Stricmp( &_Path[i], ".jpeg" )
         || !Core::Stricmp( &_Path[i], ".png" )
         || !Core::Stricmp( &_Path[i], ".tga" )
         || !Core::Stricmp( &_Path[i], ".psd" )
         || !Core::Stricmp( &_Path[i], ".gif" )
         || !Core::Stricmp( &_Path[i], ".hdr" )
         || !Core::Stricmp( &_Path[i], ".pic" )
         || !Core::Stricmp( &_Path[i], ".pnm" )
         || !Core::Stricmp( &_Path[i], ".ppm" )
         || !Core::Stricmp( &_Path[i], ".pgm" ) ) {

        if ( !Core::Stricmp( &_Path[i], ".hdr" ) ) {
            if ( !image.LoadHDRI( _Path.CStr(), true, true ) ) {
                return false;
            }
        } else {
            if ( !image.LoadLDRI( _Path.CStr(), true, true ) ) {
                return false;
            }
        }

        if ( !InitializeFromImage( image ) ) {
            return false;
        }
    } else {

        AFileStream f;

        if ( !f.OpenRead( _Path ) ) {
            return false;
        }

        uint32_t fileFormat;
        uint32_t fileVersion;

        fileFormat = f.ReadUInt32();

        if ( fileFormat != FMT_FILE_TYPE_TEXTURE ) {
            GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_TEXTURE );
            return false;
        }

        fileVersion = f.ReadUInt32();

        if ( fileVersion != FMT_VERSION_TEXTURE ) {
            GLogger.Printf( "Expected file version %d\n", FMT_VERSION_TEXTURE );
            return false;
        }

        AString guid;
        uint32_t textureType;
        STexturePixelFormat texturePixelFormat;
        uint32_t w, h, d, numLods;

        f.ReadString( guid );
        textureType = f.ReadUInt32();
        f.ReadObject( texturePixelFormat );
        w = f.ReadUInt32();
        h = f.ReadUInt32();
        d = f.ReadUInt32();
        numLods = f.ReadUInt32();

        switch ( textureType ) {
        case TEXTURE_1D:
            Initialize1D( texturePixelFormat, numLods, w );
            break;
        case TEXTURE_1D_ARRAY:
            Initialize1DArray( texturePixelFormat, numLods, w, h );
            break;
        case TEXTURE_2D:
            Initialize2D( texturePixelFormat, numLods, w, h );
            break;
        case TEXTURE_2D_ARRAY:
            Initialize2DArray( texturePixelFormat, numLods, w, h, d );
            break;
        case TEXTURE_3D:
            Initialize3D( texturePixelFormat, numLods, w, h, d );
            break;
        case TEXTURE_CUBEMAP:
            InitializeCubemap( texturePixelFormat, numLods, w );
            break;
        case TEXTURE_CUBEMAP_ARRAY:
            InitializeCubemapArray( texturePixelFormat, numLods, w, d );
            break;
        case TEXTURE_2DNPOT:
            Initialize2DNPOT( texturePixelFormat, numLods, w, h );
            break;
        default:
            GLogger.Printf( "ATexture::LoadResource: Unknown texture type %d\n", textureType );
            return false;
        }

        uint32_t lodWidth, lodHeight, lodDepth;
        size_t pixelSize = texturePixelFormat.SizeInBytesUncompressed();
        size_t maxSize = w * h * d * pixelSize;
        //byte * lodData = (byte *)GHunkMemory.HunkMemory( maxSize, 1 );
        byte * lodData = (byte *)GHeapMemory.Alloc( maxSize, 1 );

        //int numLayers = 1;

        //if ( textureType == TEXTURE_CUBEMAP ) {
        //    numLayers = 6;
        //} else if ( textureType == TEXTURE_CUBEMAP_ARRAY ) {
        //    numLayers = d * 6;
        //}
        //
        //for ( int layerNum = 0 ; layerNum < numLayers ; layerNum++ ) {
            for ( int n = 0 ; n < numLods ; n++ ) {
                lodWidth = f.ReadUInt32();
                lodHeight = f.ReadUInt32();
                lodDepth = f.ReadUInt32();

                size_t size = lodWidth * lodHeight * lodDepth * pixelSize;

                if ( size > maxSize ) {
                    GLogger.Printf( "ATexture: invalid image %s\n", _Path.CStr() );
                    break;
                }

                f.ReadBuffer( lodData, size );

                WriteArbitraryData( 0, 0, /*layerNum*/0, lodWidth, lodHeight, lodDepth, n, lodData );
            }
        //}

        //GHunkMemory.ClearLastHunk();
        GHeapMemory.Free( lodData );

#if 0
        byte * buf = (byte *)GHeapMemory.HeapAlloc( size, 1 );

        f.Read( buf, size );

        AMemoryStream ms;

        if ( !ms.OpenRead( _Path, buf, size ) ) {
            GHeapMemory.HeapFree( buf );
            return false;
        }

        if ( !image.LoadLDRI( ms, bSRGB, true ) ) {
            GHeapMemory.HeapFree( buf );
            return false;
        }

        GHeapMemory.HeapFree( buf );

        if ( !InitializeFromImage( image ) ) {
            return false;
        }
#endif
    }

    return true;
}

bool ATexture::IsCubemap() const {
    return TextureType == TEXTURE_CUBEMAP || TextureType == TEXTURE_CUBEMAP_ARRAY;
}

size_t ATexture::TextureByteLength1D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_ASSERT(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += Math::Max( 1, _Width );
            _Width >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * Math::Max( _ArraySize, 1 );
    }
}

size_t ATexture::TextureByteLength2D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_ASSERT(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += Math::Max( 1, _Width ) * Math::Max( 1, _Height );
            _Width >>= 1;
            _Height >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * Math::Max( _ArraySize, 1 );
    }
}

size_t ATexture::TextureByteLength3D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_ASSERT(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += Math::Max( 1, _Width ) * Math::Max( 1, _Height ) * Math::Max( 1, _Depth );
            _Width >>= 1;
            _Height >>= 1;
            _Depth >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum;
    }
}

size_t ATexture::TextureByteLengthCubemap( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_ASSERT(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += Math::Max( 1, _Width ) * Math::Max( 1, _Width );
            _Width >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * 6 * Math::Max( _ArraySize, 1 );
    }
}

size_t ATexture::TextureByteLength2DNPOT( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    if ( _PixelFormat.IsCompressed() ) {
        // TODO
        AN_ASSERT(0);
        return 0;
    } else {
        size_t sum = 0;
        for ( int i = 0 ; i < _NumLods ; i++ ) {
            sum += Math::Max( 1, _Width ) * Math::Max( 1, _Height );
            _Width >>= 1;
            _Height >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum;
    }
}

void ATexture::Initialize1D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    Purge();

    TextureType = TEXTURE_1D;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = 1;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture1D( TextureGPU, _PixelFormat.Data, _NumLods, _Width );
}

void ATexture::Initialize1DArray( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    Purge();

    TextureType = TEXTURE_1D_ARRAY;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _ArraySize;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture1DArray( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _ArraySize );
}

void ATexture::Initialize2D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    Purge();

    TextureType = TEXTURE_2D;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture2D( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height );
}

void ATexture::Initialize2DArray( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
    Purge();

    TextureType = TEXTURE_2D_ARRAY;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = _ArraySize;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture2DArray( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height, _ArraySize );
}

void ATexture::Initialize3D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    Purge();

    TextureType = TEXTURE_3D;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = _Depth;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture3D( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height, _Depth );
}

void ATexture::InitializeColorGradingLUT( const char * _Path ) {
    AImage image;

    if ( image.LoadLDRI( _Path, true, false, 3 ) ) {
        byte data[ 16 ][ 16 ][ 16 ][ 3 ];

        for ( int z = 0 ; z < 16 ; z++ ) {
            for ( int y = 0 ; y < 16 ; y++ ) {
                Core::Memcpy( &data[ z ][ y ][ 0 ][ 0 ], static_cast< byte * >(image.pRawData) + ( z * 16 * 16 * 3 + y * 16 * 3 ), 16 * 3 );
            }
        }

        Initialize3D( TEXTURE_PF_BGR8_SRGB, 1, 16, 16, 16 );
        WriteArbitraryData( 0, 0, 0, 16, 16, 16, 0, data );

        return;
    }

    LoadInternalResource( "/Default/Textures/LUT_Luminance" );
}

static Float3 ApplyColorGrading( SColorGradingPreset const & p, AColor4 const & _Color ) {
    float lum = _Color.GetLuminance();

    AColor4 mult;

    mult.SetTemperature( Math::Clamp( p.ColorTemperature, 1000.0f, 40000.0f ) );

    AColor4 c = Math::Lerp( _Color.GetRGB(), _Color.GetRGB() * mult.GetRGB(), p.ColorTemperatureStrength );

    float newLum = c.GetLuminance();

    c *= Math::Lerp( 1.0f, ( newLum > 1e-6 ) ? ( lum / newLum ) : 1.0f, p.ColorTemperatureBrightnessNormalization );

    c = Math::Lerp( Float3( c.GetLuminance() ), c.GetRGB(), p.Presaturation );

    Float3 t = ( p.Gain * 2.0f ) * ( c.GetRGB() + ( ( p.Lift * 2.0f - 1.0 ) * ( Float3( 1.0 ) - c.GetRGB() ) ) );

    t.X = StdPow( t.X, 0.5f / p.Gamma.X );
    t.Y = StdPow( t.Y, 0.5f / p.Gamma.Y );
    t.Z = StdPow( t.Z, 0.5f / p.Gamma.Z );

    return t;
}

void ATexture::InitializeColorGradingLUT( SColorGradingPreset const & _Preset ) {
    byte data[ 16 ][ 16 ][ 16 ][ 3 ];
    AColor4 color;
    Float3 result;

    Initialize3D( TEXTURE_PF_BGR8_SRGB, 1, 16, 16, 16 );

    const float scale = 1.0f / 15.0f;

    for ( int z = 0 ; z < 16 ; z++ ) {
        color.Z = scale * z;

        for ( int y = 0 ; y < 16 ; y++ ) {
            color.Y = scale * y;

            for ( int x = 0 ; x < 16 ; x++ ) {
                color.X = scale * x;

                result = ApplyColorGrading( _Preset, color ) * 255.0f;

                data[ z ][ y ][ x ][ 0 ] = Math::Clamp( result.Z, 0.0f, 255.0f );
                data[ z ][ y ][ x ][ 1 ] = Math::Clamp( result.Y, 0.0f, 255.0f );
                data[ z ][ y ][ x ][ 2 ] = Math::Clamp( result.X, 0.0f, 255.0f );
            }
        }
    }

    WriteArbitraryData( 0, 0, 0, 16, 16, 16, 0, data );
}

void ATexture::InitializeCubemap( STexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    Purge();

    TextureType = TEXTURE_CUBEMAP;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Width;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTextureCubemap( TextureGPU, _PixelFormat.Data, _NumLods, _Width );
}

void ATexture::InitializeCubemapArray( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    Purge();

    TextureType = TEXTURE_CUBEMAP_ARRAY;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Width;
    Depth = _ArraySize;
    NumLods = _NumLods;

    GRenderBackend->InitializeTextureCubemapArray( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _ArraySize );
}

void ATexture::Initialize2DNPOT( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    Purge();

    TextureType = TEXTURE_2DNPOT;
    PixelFormat = _PixelFormat;
    Width = _Width;
    Height = _Height;
    Depth = 1;
    NumLods = _NumLods;

    GRenderBackend->InitializeTexture2DNPOT( TextureGPU, _PixelFormat.Data, _NumLods, _Width, _Height );
}

size_t ATexture::GetSizeInBytes() const {
    switch ( TextureType ) {
    case TEXTURE_1D:
        return ATexture::TextureByteLength1D( PixelFormat, NumLods, Width, 1 );
    case TEXTURE_1D_ARRAY:
        return ATexture::TextureByteLength1D( PixelFormat, NumLods, Width, GetArraySize() );
    case TEXTURE_2D:
        return ATexture::TextureByteLength2D( PixelFormat, NumLods, Width, Height, 1 );
    case TEXTURE_2D_ARRAY:
        return ATexture::TextureByteLength2D( PixelFormat, NumLods, Width, Height, GetArraySize() );
    case TEXTURE_3D:
        return ATexture::TextureByteLength3D( PixelFormat, NumLods, Width, Height, Depth );
    case TEXTURE_CUBEMAP:
        return ATexture::TextureByteLengthCubemap( PixelFormat, NumLods, Width, 1 );
    case TEXTURE_CUBEMAP_ARRAY:
        return ATexture::TextureByteLengthCubemap( PixelFormat, NumLods, Width, GetArraySize() );
    case TEXTURE_2DNPOT:
        return ATexture::TextureByteLength2DNPOT( PixelFormat, NumLods, Width, Height );
    }
    return 0;
}

int ATexture::GetArraySize() const {
    switch ( TextureType ) {
    case TEXTURE_1D_ARRAY:
        return Height;
    case TEXTURE_2D_ARRAY:
    case TEXTURE_CUBEMAP_ARRAY:
        return Depth;
    }
    return 1;
}

bool ATexture::WriteTextureData1D( int _LocationX, int _Width, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_1D && TextureType != TEXTURE_1D_ARRAY ) {
        GLogger.Printf( "ATexture::WriteTextureData1D: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, 0, 0, _Width, 1, 1, _Lod, _SysMem );
}

bool ATexture::WriteTextureData1DArray( int _LocationX, int _Width, int _ArrayLayer, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_1D_ARRAY ) {
        GLogger.Printf( "ATexture::WriteTextureData1DArray: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, _ArrayLayer, 0, _Width, 1, 1, _Lod, _SysMem );
}

bool ATexture::WriteTextureData2D( int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_2D && TextureType != TEXTURE_2D_ARRAY && TextureType != TEXTURE_2DNPOT ) {
        GLogger.Printf( "ATexture::WriteTextureData2D: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, _LocationY, 0, _Width, _Height, 1, _Lod, _SysMem );
}

bool ATexture::WriteTextureData2DArray( int _LocationX, int _LocationY, int _Width, int _Height, int _ArrayLayer, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_2D_ARRAY ) {
        GLogger.Printf( "ATexture::WriteTextureData2DArray: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, _LocationY, _ArrayLayer, _Width, _Height, 1, _Lod, _SysMem );
}

bool ATexture::WriteTextureData3D( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Depth, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_3D ) {
        GLogger.Printf( "ATexture::WriteTextureData3D: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, _LocationY, _LocationZ, _Width, _Height, _Depth, _Lod, _SysMem );
}

bool ATexture::WriteTextureDataCubemap( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_CUBEMAP && TextureType != TEXTURE_CUBEMAP_ARRAY ) {
        GLogger.Printf( "ATexture::WriteTextureDataCubemap: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, _LocationY, _FaceIndex, _Width, _Height, 1, _Lod, _SysMem );
}

bool ATexture::WriteTextureDataCubemapArray( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _ArrayLayer, int _Lod, const void * _SysMem ) {
    if ( TextureType != TEXTURE_CUBEMAP_ARRAY ) {
        GLogger.Printf( "ATexture::WriteTextureDataCubemapArray: called for %s\n", TextureTypeName[TextureType] );
        return false;
    }
    return WriteArbitraryData( _LocationX, _LocationY, _ArrayLayer*6 + _FaceIndex, _Width, _Height, 1, _Lod, _SysMem );
}

bool ATexture::WriteArbitraryData( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Depth, int _Lod, const void * _SysMem ) {
    if ( !Width ) {
        GLogger.Printf( "ATexture::WriteArbitraryData: texture is not initialized\n" );
        return false;
    }

    size_t sizeInBytes = _Width * _Height * _Depth;

    if ( IsCompressed() ) {
        // TODO
        AN_ASSERT( 0 );
        return false;
    } else {
        sizeInBytes *= SizeInBytesUncompressed();
    }

    STextureRect rect;

    rect.Offset.X = _LocationX;
    rect.Offset.Y = _LocationY;
    rect.Offset.Z = _LocationZ;
    rect.Offset.Lod = _Lod;
    rect.Dimension.X = _Width;
    rect.Dimension.Y = _Height;
    rect.Dimension.Z = _Depth;

    // TODO: bounds check?

    GRenderBackend->WriteTexture( TextureGPU, rect, PixelFormat.Data, sizeInBytes, 1, _SysMem );

    return true;
}

void ATexture::UploadResourcesGPU() {
    GLogger.Printf( "ATexture::UploadResourcesGPU\n" );
}
