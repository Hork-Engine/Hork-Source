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

#include <Core/Public/Image.h>
#include <Core/Public/Color.h>
#include <Core/Public/Logger.h>

#define STBI_MALLOC(sz)                     GHeapMemory.Alloc( sz )
#define STBI_FREE(p)                        GHeapMemory.Free( p )
#define STBI_REALLOC(p,newsz)               GHeapMemory.Realloc( p, newsz, true )
#define STBI_REALLOC_SIZED(p,oldsz,newsz)   STBI_REALLOC( p, newsz )
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_STDIO
#define STBI_NO_GIF  // Maybe in future gif will be used, but not now
#include "stb/stb_image.h"

#define STBIW_MALLOC(sz)                    GHeapMemory.Alloc( sz )
#define STBIW_FREE(p)                       GHeapMemory.Free( p )
#define STBIW_REALLOC(p,newsz)              GHeapMemory.Realloc( p, newsz, true )
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBI_WRITE_NO_STDIO
#include "stb/stb_image_write.h"

#define STBIR_MALLOC(sz,context)            GHunkMemory.Alloc( sz )
#define STBIR_FREE(p,context)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#define STBIR_MAX_CHANNELS                  4
#include "stb/stb_image_resize.h"

AImage::AImage() {
    pRawData = nullptr;
    Width = 0;
    Height = 0;
    NumChannels = 0;
    bHDRI = false;
    bLinearSpace = false;
    bHalf = false;
    NumLods = 0;
}

AImage::~AImage() {
    Free();
}

static int Stbi_Read( void *user, char *data, int size ) {
    IBinaryStream * stream = (IBinaryStream * )user;
    stream->ReadBuffer( data, size );
    return stream->GetReadBytesCount();
}

static void Stbi_Skip( void *user, int n ) {
    IBinaryStream * stream = (IBinaryStream * )user;
    stream->SeekCur( n );
}

static int Stbi_Eof( void *user ) {
    IBinaryStream * stream = (IBinaryStream * )user;
    return stream->Eof();
}

static void Stbi_Write( void *context, void *data, int size ) {
    IBinaryStream * stream = (IBinaryStream * )context;
    stream->WriteBuffer( data, size );
}

bool AImage::LoadLDRI( const char * _Path, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    AFileStream stream;

    Free();

    if ( !stream.OpenRead( _Path ) ) {
        return false;
    }

    return LoadLDRI( stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

static bool LoadRawImage( const char * _Name, AImage & _Image, const stbi_io_callbacks * _Callbacks, void * _User, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    AN_ASSERT( _NumDesiredChannels >= 0 && _NumDesiredChannels <= 4 );

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
        GLogger.Printf( "AImage::LoadRawImage: couldn't load %s\n", _Name );
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
        SSoftwareMipmapGenerator mipmapGen;

        mipmapGen.SourceImage = data;
        mipmapGen.Width = _Image.Width;
        mipmapGen.Height = _Image.Height;
        mipmapGen.NumChannels = _Image.NumChannels;
        mipmapGen.bLinearSpace = _Image.bLinearSpace;
        mipmapGen.bPremultipliedAlpha = false;
        mipmapGen.bHDRI = false;

        int requiredMemorySize;
        ComputeRequiredMemorySize( mipmapGen, requiredMemorySize, _Image.NumLods );

        _Image.pRawData = GHeapMemory.Alloc( requiredMemorySize );

        GenerateMipmaps( mipmapGen, _Image.pRawData );

        GHeapMemory.Free( data );
    } else {
        _Image.pRawData = data;
    }

    return true;
}

static bool LoadRawImageHDRI( const char * _Name, AImage & _Image, const stbi_io_callbacks * _Callbacks, void * _User, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    AN_ASSERT( _NumDesiredChannels >= 0 && _NumDesiredChannels <= 4 );

    _Image.Free();

    float * data = stbi_loadf_from_callbacks( _Callbacks, _User, &_Image.Width, &_Image.Height, &_Image.NumChannels, _NumDesiredChannels );
    if ( !data ) {
        GLogger.Printf( "AImage::LoadRawImageHDRI: couldn't load %s\n", _Name );
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
        SSoftwareMipmapGenerator mipmapGen;

        mipmapGen.SourceImage = data;
        mipmapGen.Width = _Image.Width;
        mipmapGen.Height = _Image.Height;
        mipmapGen.NumChannels = _Image.NumChannels;
        mipmapGen.bLinearSpace = _Image.bLinearSpace;
        mipmapGen.bPremultipliedAlpha = false;
        mipmapGen.bHDRI = true;

        int requiredMemorySize;
        ComputeRequiredMemorySize( mipmapGen, requiredMemorySize, _Image.NumLods );

        void * tmp = GHeapMemory.Alloc( requiredMemorySize );

        GenerateMipmaps( mipmapGen, tmp );

        GHeapMemory.Free( data );
        data = ( float * )tmp;
    }

    if ( _HalfFloat ) {
        int imageSize = 0;
        for ( int i = 0 ; ; i++ ) {
            int w = Math::Max( 1, _Image.Width >> i );
            int h = Math::Max( 1, _Image.Height >> i );
            imageSize += w * h;
            if ( w == 1 && h == 1 ) {
                break;
            }
        }
        imageSize *= _Image.NumChannels;

        uint16_t * tmp = ( uint16_t * )GHeapMemory.Alloc( imageSize * sizeof( uint16_t ) );
        Math::FloatToHalf( data, tmp, imageSize );
        GHeapMemory.Free( data );
        data = ( float * )tmp;
    }

    _Image.pRawData = data;

    return true;
}

bool AImage::LoadLDRI( IBinaryStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read,
        Stbi_Skip,
        Stbi_Eof
    };

    return ::LoadRawImage( _Stream.GetFileName(), *this, &callbacks, &_Stream, _SRGB, _GenerateMipmaps, _NumDesiredChannels );
}

bool AImage::LoadHDRI( const char * _Path, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    AFileStream stream;

    Free();

    if ( !stream.OpenRead( _Path ) ) {
        return false;
    }

    return LoadHDRI( stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

bool AImage::LoadHDRI( IBinaryStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels ) {
    const stbi_io_callbacks callbacks = {
        Stbi_Read,
        Stbi_Skip,
        Stbi_Eof
    };

    return ::LoadRawImageHDRI( _Stream.GetFileName(), *this, &callbacks, &_Stream, _HalfFloat, _GenerateMipmaps, _NumDesiredChannels );
}

void AImage::FromRawDataLDRI( const byte * _Data, int _Width, int _Height, int _NumChannels, bool _SRGB, bool _GenerateMipmaps ) {
    Free();

    Width = _Width;
    Height = _Height;
    NumChannels = _NumChannels;
    bLinearSpace = !_SRGB;
    bHDRI = false;
    bHalf = false;
    NumLods = 1;

    if ( _GenerateMipmaps ) {
        SSoftwareMipmapGenerator mipmapGen;

        mipmapGen.SourceImage = (void *)_Data;
        mipmapGen.Width = Width;
        mipmapGen.Height = Height;
        mipmapGen.NumChannels = NumChannels;
        mipmapGen.bLinearSpace = bLinearSpace;
        mipmapGen.bPremultipliedAlpha = false;
        mipmapGen.bHDRI = bHDRI;

        int requiredMemorySize;
        ComputeRequiredMemorySize( mipmapGen, requiredMemorySize, NumLods );

        pRawData = GHeapMemory.Alloc( requiredMemorySize );

        GenerateMipmaps( mipmapGen, pRawData );
    } else {
        size_t sizeInBytes = _Width * _Height * NumChannels;

        pRawData = GHeapMemory.Alloc( sizeInBytes );
        Core::Memcpy( pRawData, _Data, sizeInBytes );
    }
}

void AImage::Free() {
    if ( pRawData ) {
        GHeapMemory.Free( pRawData );
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
    return Math::Clamp( _Value, 0.0f, 255.0f );
}

AN_FORCEINLINE float ByteToFloat( const byte & _Color ) {
    return _Color / 255.0f;
}

AN_FORCEINLINE byte FloatToByte( const float & _Color ) {
    //return ClampByte( _Color * 255.0f );  // round to small
    return ClampByte( Math::Floor( _Color * 255.0f + 0.5f ) ); // round to nearest
}

AN_FORCEINLINE byte LinearToSRGB_Byte( const float & _lRGB ) {
    return FloatToByte( LinearToSRGB( _lRGB ) );
}

// TODO: Add other mipmap filters
static void DownscaleSimpleAverage( int _CurWidth, int _CurHeight, int _NewWidth, int _NewHeight, int _NumChannels, int _AlphaChannel, bool _LinearSpace, const byte * _SrcData, byte * _DstData ) {
    int Bpp = _NumChannels;

    if ( _CurWidth == _NewWidth && _CurHeight == _NewHeight ) {
        Core::Memcpy( _DstData, _SrcData, _NewWidth*_NewHeight*Bpp );
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

                    _DstData[ idx * Bpp + ch ] = ClampByte( Math::Floor( avg + 0.5f ) );
                } else {
                    if ( _NewWidth == _CurWidth ) {
                        x = i;
                        y = j << 1;

                        a = LinearFromSRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x)*Bpp + ch ] ) );
                        c = LinearFromSRGB( ByteToFloat( _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ] ) );

                        avg = ( a + c ) * 0.5f;

                    } else if ( _NewHeight == _CurHeight ) {
                        x = i << 1;
                        y = j;

                        a = LinearFromSRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x)*Bpp + ch ] ) );
                        b = LinearFromSRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ] ) );

                        avg = ( a + b ) * 0.5f;

                    } else {
                        x = i << 1;
                        y = j << 1;

                        a = LinearFromSRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x)*Bpp + ch ] ) );
                        b = LinearFromSRGB( ByteToFloat( _SrcData[ (y * _CurWidth + x + 1)*Bpp + ch ] ) );
                        c = LinearFromSRGB( ByteToFloat( _SrcData[ ((y + 1) * _CurWidth + x)*Bpp + ch ] ) );
                        d = LinearFromSRGB( ByteToFloat( _SrcData[ ((y + 1) * _CurWidth + x + 1)*Bpp + ch ] ) );

                        avg = ( a + b + c + d ) * 0.25f;
                    }

                    _DstData[ idx * Bpp + ch ] = LinearToSRGB_Byte( avg );
                }
            }
        }
    }
}

static void DownscaleSimpleAverageHDRI( int _CurWidth, int _CurHeight, int _NewWidth, int _NewHeight, int _NumChannels, const float * _SrcData, float * _DstData ) {
    int Bpp = _NumChannels;

    if ( _CurWidth == _NewWidth && _CurHeight == _NewHeight ) {
        Core::Memcpy( _DstData, _SrcData, _NewWidth*_NewHeight*Bpp*sizeof( float ) );
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

static void GenerateMipmaps( const byte * ImageData, int ImageWidth, int ImageHeight, int NumChannels, EMipmapEdgeMode EdgeMode, EMipmapFilter Filter, bool bLinearSpace, bool bPremultipliedAlpha, byte * Dest ) {
    Core::Memcpy( Dest, ImageData, ImageWidth * ImageHeight * NumChannels );

    int MemoryOffset = ImageWidth * ImageHeight * NumChannels;

    int CurWidth = ImageWidth;
    int CurHeight = ImageHeight;

    int AlphaChannel = NumChannels == 4 ? 3 : -1;

    for ( int i = 1 ; ; i++ ) {
        int LodWidth = Math::Max( 1, ImageWidth >> i );
        int LodHeight = Math::Max( 1, ImageHeight >> i );

        byte * LodData = Dest + MemoryOffset;
        MemoryOffset += LodWidth * LodHeight * NumChannels;

#if 1
        int hunkMark = GHunkMemory.SetHunkMark();

        stbir_resize( ImageData, CurWidth, CurHeight, NumChannels * CurWidth,
                      LodData, LodWidth, LodHeight, NumChannels * LodWidth,
                      STBIR_TYPE_UINT8,
                      NumChannels,
                      AlphaChannel,
                      bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                      (stbir_edge)EdgeMode, (stbir_edge)EdgeMode,
                      (stbir_filter)Filter, (stbir_filter)Filter,
                      bLinearSpace ? STBIR_COLORSPACE_LINEAR : STBIR_COLORSPACE_SRGB,
                      NULL );

        GHunkMemory.ClearToMark( hunkMark );
#else
        DownscaleSimpleAverage( CurWidth, CurHeight, LodWidth, LodHeight, NumChannels, AlphaChannel, bLinearSpace, ImageData, LodData );
#endif

        ImageData = LodData;

        CurWidth = LodWidth;
        CurHeight = LodHeight;

        if ( LodWidth == 1 && LodHeight == 1 ) {
            break;
        }
    }
}

static void GenerateMipmapsHDRI( const float * ImageData, int ImageWidth, int ImageHeight, int NumChannels, EMipmapEdgeMode EdgeMode, EMipmapFilter Filter, bool bPremultipliedAlpha, float * _Dest ) {
    Core::Memcpy( _Dest, ImageData, ImageWidth * ImageHeight * NumChannels * sizeof( float ) );

    int MemoryOffset = ImageWidth * ImageHeight * NumChannels;

    int CurWidth = ImageWidth;
    int CurHeight = ImageHeight;

    for ( int i = 1 ; ; i++ ) {
        int LodWidth = Math::Max( 1, ImageWidth >> i );
        int LodHeight = Math::Max( 1, ImageHeight >> i );

        float * LodData = _Dest + MemoryOffset;
        MemoryOffset += LodWidth * LodHeight * NumChannels;

#if 1
        int hunkMark = GHunkMemory.SetHunkMark();

        stbir_resize( ImageData, CurWidth, CurHeight, NumChannels * CurWidth * sizeof( float ),
                      LodData, LodWidth, LodHeight, NumChannels * LodWidth * sizeof( float ),
                      STBIR_TYPE_FLOAT,
                      NumChannels,
                      -1,
                      bPremultipliedAlpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                      (stbir_edge)EdgeMode, (stbir_edge)EdgeMode,
                      (stbir_filter)Filter, (stbir_filter)Filter,
                      STBIR_COLORSPACE_LINEAR,
                      NULL );

        GHunkMemory.ClearToMark( hunkMark );
#else
        DownscaleSimpleAverageHDRI( CurWidth, CurHeight, LodWidth, LodHeight, NumChannels, ImageData, LodData );
#endif

        ImageData = LodData;

        CurWidth = LodWidth;
        CurHeight = LodHeight;

        if ( LodWidth == 1 && LodHeight == 1 ) {
            break;
        }
    }
}

void ComputeRequiredMemorySize( SSoftwareMipmapGenerator const & _Config, int & _RequiredMemory, int & _NumLods ) {
    _RequiredMemory = 0;
    _NumLods = 0;

    for ( int i = 0 ; ; i++ ) {
        int LodWidth = Math::Max( 1, _Config.Width >> i );
        int LodHeight = Math::Max( 1, _Config.Height >> i );
        _RequiredMemory += LodWidth * LodHeight;
        _NumLods++;
        if ( LodWidth == 1 && LodHeight == 1 ) {
            break;
        }
    }

    _RequiredMemory *= _Config.NumChannels;

    if ( _Config.bHDRI ) {
        _RequiredMemory *= sizeof( float );
    }
}

void GenerateMipmaps( SSoftwareMipmapGenerator const & _Config, void * _Data ) {
    if ( _Config.bHDRI ) {
        ::GenerateMipmapsHDRI( (const float *)_Config.SourceImage, _Config.Width, _Config.Height, _Config.NumChannels, _Config.EdgeMode, _Config.Filter, _Config.bPremultipliedAlpha, (float *)_Data );
    } else {
        ::GenerateMipmaps( (const byte *)_Config.SourceImage, _Config.Width, _Config.Height, _Config.NumChannels, _Config.EdgeMode, _Config.Filter, _Config.bLinearSpace, _Config.bPremultipliedAlpha, (byte *)_Data );
    }
}

static void MemSwap( byte * Block, const size_t BlockSz, byte * _Ptr1, byte * _Ptr2, const size_t _Size ) {
    const size_t blockCount = _Size / BlockSz;
    size_t i;
    for ( i = 0; i < blockCount; i++ ) {
        Core::Memcpy( Block, _Ptr1, BlockSz );
        Core::Memcpy( _Ptr1, _Ptr2, BlockSz );
        Core::Memcpy( _Ptr2, Block, BlockSz );
        _Ptr2 += BlockSz;
        _Ptr1 += BlockSz;
    }
    i = _Size - i*BlockSz;
    if ( i > 0 ) {
        Core::Memcpy( Block, _Ptr1, i );
        Core::Memcpy( _Ptr1, _Ptr2, i );
        Core::Memcpy( _Ptr2, Block, i );
    }
}

void FlipImageX( void * _ImageData, int _Width, int _Height, int _BytesPerPixel, int _BytesPerLine ) {
    int lineWidth = _Width * _BytesPerPixel;
    int halfWidth = _Width >> 1;
    byte * temp = (byte *)StackAlloc( _BytesPerPixel );
    byte * image = (byte *)_ImageData;
    for ( int y = 0 ; y < _Height ; y++ ) {
        byte * s = image;
        byte * e = image + lineWidth;
        for ( int x = 0; x < halfWidth; x++ ) {
            e -= _BytesPerPixel;
            Core::Memcpy( temp, s, _BytesPerPixel );
            Core::Memcpy( s, e, _BytesPerPixel );
            Core::Memcpy( e, temp, _BytesPerPixel );
            s += _BytesPerPixel;
        }
        image += _BytesPerLine;
    }
}

void FlipImageY( void * _ImageData, int _Width, int _Height, int _BytesPerPixel, int _BytesPerLine ) {
    const size_t blockSizeInBytes = 4096;
    byte block[blockSizeInBytes];
    int lineWidth = _Width * _BytesPerPixel;
    int halfHeight = _Height >> 1;
    byte * image = (byte *)_ImageData;
    byte * e = image + _Height * _BytesPerLine;
    for ( int y = 0; y < halfHeight; y++ ) {
        e -= _BytesPerLine;
        MemSwap( block, blockSizeInBytes, image, e, lineWidth );
        image += _BytesPerLine;
    }
}

void LinearToPremultipliedAlphaSRGB( const float * SourceImage,
                                    int Width,
                                    int Height,
                                    bool bOverbright,
                                    float fOverbright,
                                    bool bReplaceAlpha,
                                    float fReplaceAlpha,
                                    byte * sRGB ) {

    const float * src = SourceImage;
    byte * dst = sRGB;
    float r, g, b;

    int pixCount = Width * Height;

    byte replaceAlpha = FloatToByte( fReplaceAlpha );

    for ( int i = 0 ; i < pixCount ; i++, src += 4, dst += 4 ) {
        r = src[ 0 ] * src[3];
        g = src[ 1 ] * src[3];
        b = src[ 2 ] * src[3];

        if ( bOverbright ) {
            r *= fOverbright;
            g *= fOverbright;
            b *= fOverbright;
#if 1
            float m = Math::Max( r, g, b );
            if ( m > 1.0f ) {
                m = 1.0f / m;
                r *= m;
                g *= m;
                b *= m;
            }
#else
            if ( r > 1.0f ) r = 1.0f;
            if ( g > 1.0f ) g = 1.0f;
            if ( b > 1.0f ) b = 1.0f;
#endif
        }

        dst[0] = LinearToSRGB_Byte( r );
        dst[1] = LinearToSRGB_Byte( g );
        dst[2] = LinearToSRGB_Byte( b );
        dst[3] = bReplaceAlpha ? replaceAlpha : FloatToByte( src[3] );
    }
}

bool WritePNG( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData, int _BytesPerLine ) {
    return !!stbi_write_png_to_func( Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData, _BytesPerLine );
}

bool WriteBMP( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData ) {
    return !!stbi_write_bmp_to_func( Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData );
}

bool WriteTGA( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData ) {
    return !!stbi_write_tga_to_func( Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData );
}

bool WriteJPG( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData, int _Quality ) {
    return !!stbi_write_jpg_to_func( Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData, _Quality );
}

bool WriteHDR( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const float * _ImageData ) {
    return !!stbi_write_hdr_to_func( Stbi_Write, &_Stream, _Width, _Height, _NumChannels, _ImageData );
}
