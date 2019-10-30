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

#include <Engine/Core/Public/Image.h>
#include <Engine/Core/Public/Color.h>
#include <Engine/Core/Public/Logger.h>

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

bool FImage::LoadLDRI( const char * _Path, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    FFileStream stream;

    Free();

    if ( !stream.OpenRead( _Path ) ) {
        return false;
    }

    return LoadLDRI( stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
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

bool FImage::LoadLDRI( FFileStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read,
        Stbi_Skip,
        Stbi_Eof
    };

    return ::LoadRawImage( _Stream.GetFileName(), *this, &callbacks, &_Stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadLDRI( FMemoryStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read_Mem,
        Stbi_Skip_Mem,
        Stbi_Eof_Mem
    };

    return ::LoadRawImage( _Stream.GetFileName(), *this, &callbacks, &_Stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadHDRI( const char * _Path, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    FFileStream stream;

    Free();

    if ( !stream.OpenRead( _Path ) ) {
        return false;
    }

    return LoadHDRI( stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadHDRI( FFileStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read,
        Stbi_Skip,
        Stbi_Eof
    };

    return ::LoadRawImageHDRI( _Stream.GetFileName(), *this, &callbacks, &_Stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

bool FImage::LoadHDRI( FMemoryStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
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
