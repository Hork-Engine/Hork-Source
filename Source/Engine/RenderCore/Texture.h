/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "DeviceObject.h"
#include "Buffer.h"

#include <memory.h>

namespace RenderCore {

class ITexture;

enum TEXTURE_FORMAT : uint8_t
{
    //
    // Normalized signed/unsigned formats
    //

    TEXTURE_FORMAT_R8,             /// RED   8
    TEXTURE_FORMAT_R8_SNORM,       /// RED   s8
    TEXTURE_FORMAT_R16,            /// RED   16
    TEXTURE_FORMAT_R16_SNORM,      /// RED   s16
    TEXTURE_FORMAT_RG8,            /// RG    8   8
    TEXTURE_FORMAT_RG8_SNORM,      /// RG    s8  s8
    TEXTURE_FORMAT_RG16,           /// RG    16  16
    TEXTURE_FORMAT_RG16_SNORM,     /// RG    s16 s16
    TEXTURE_FORMAT_R3_G3_B2,       /// RGB   3  3  2
    TEXTURE_FORMAT_RGB4,           /// RGB   4  4  4
    TEXTURE_FORMAT_RGB5,           /// RGB   5  5  5
    TEXTURE_FORMAT_RGB8,           /// RGB   8  8  8
    TEXTURE_FORMAT_RGB8_SNORM,     /// RGB   s8  s8  s8
    TEXTURE_FORMAT_RGB10,          /// RGB   10  10  10
    TEXTURE_FORMAT_RGB12,          /// RGB   12  12  12
    TEXTURE_FORMAT_RGB16,          /// RGB   16  16  16
    TEXTURE_FORMAT_RGB16_SNORM,    /// RGB   s16  s16  s16
    TEXTURE_FORMAT_RGBA2,          /// RGB   2  2  2  2
    TEXTURE_FORMAT_RGBA4,          /// RGB   4  4  4  4
    TEXTURE_FORMAT_RGB5_A1,        /// RGBA  5  5  5  1
    TEXTURE_FORMAT_RGBA8,          /// RGBA  8  8  8  8
    TEXTURE_FORMAT_RGBA8_SNORM,    /// RGBA  s8  s8  s8  s8
    TEXTURE_FORMAT_RGB10_A2,       /// RGBA  10  10  10  2
    TEXTURE_FORMAT_RGB10_A2UI,     /// RGBA  ui10  ui10  ui10  ui2
    TEXTURE_FORMAT_RGBA12,         /// RGBA  12  12  12  12
    TEXTURE_FORMAT_RGBA16,         /// RGBA  16  16  16  16
    TEXTURE_FORMAT_RGBA16_SNORM,   /// RGBA  s16  s16  s16  s16
    TEXTURE_FORMAT_SRGB8,          /// RGB   8  8  8
    TEXTURE_FORMAT_SRGB8_ALPHA8,   /// RGBA  8  8  8  8

    //
    // Half-float
    //

    TEXTURE_FORMAT_R16F,           /// RED   f16
    TEXTURE_FORMAT_RG16F,          /// RG    f16  f16
    TEXTURE_FORMAT_RGB16F,         /// RGB   f16  f16  f16
    TEXTURE_FORMAT_RGBA16F,        /// RGBA  f16  f16  f16  f16

    //
    // Float
    //

    TEXTURE_FORMAT_R32F,           /// RED   f32
    TEXTURE_FORMAT_RG32F,          /// RG    f32  f32
    TEXTURE_FORMAT_RGB32F,         /// RGB   f32  f32  f32
    TEXTURE_FORMAT_RGBA32F,        /// RGBA  f32  f32  f32  f32
    TEXTURE_FORMAT_R11F_G11F_B10F, /// RGB   f11  f11  f10

    // Shared exponent
    TEXTURE_FORMAT_RGB9_E5,        /// RGB   9  9  9     5

    //
    // Integer formats
    //

    TEXTURE_FORMAT_R8I,            /// RED   i8
    TEXTURE_FORMAT_R8UI,           /// RED   ui8
    TEXTURE_FORMAT_R16I,           /// RED   i16
    TEXTURE_FORMAT_R16UI,          /// RED   ui16
    TEXTURE_FORMAT_R32I,           /// RED   i32
    TEXTURE_FORMAT_R32UI,          /// RED   ui32
    TEXTURE_FORMAT_RG8I,           /// RG    i8   i8
    TEXTURE_FORMAT_RG8UI,          /// RG    ui8   ui8
    TEXTURE_FORMAT_RG16I,          /// RG    i16   i16
    TEXTURE_FORMAT_RG16UI,         /// RG    ui16  ui16
    TEXTURE_FORMAT_RG32I,          /// RG    i32   i32
    TEXTURE_FORMAT_RG32UI,         /// RG    ui32  ui32
    TEXTURE_FORMAT_RGB8I,          /// RGB   i8   i8   i8
    TEXTURE_FORMAT_RGB8UI,         /// RGB   ui8  ui8  ui8
    TEXTURE_FORMAT_RGB16I,         /// RGB   i16  i16  i16
    TEXTURE_FORMAT_RGB16UI,        /// RGB   ui16 ui16 ui16
    TEXTURE_FORMAT_RGB32I,         /// RGB   i32  i32  i32
    TEXTURE_FORMAT_RGB32UI,        /// RGB   ui32 ui32 ui32
    TEXTURE_FORMAT_RGBA8I,         /// RGBA  i8   i8   i8   i8
    TEXTURE_FORMAT_RGBA8UI,        /// RGBA  ui8  ui8  ui8  ui8
    TEXTURE_FORMAT_RGBA16I,        /// RGBA  i16  i16  i16  i16
    TEXTURE_FORMAT_RGBA16UI,       /// RGBA  ui16 ui16 ui16 ui16
    TEXTURE_FORMAT_RGBA32I,        /// RGBA  i32  i32  i32  i32
    TEXTURE_FORMAT_RGBA32UI,       /// RGBA  ui32 ui32 ui32 ui32

    //
    // Compressed formats
    //

    // RGB
    TEXTURE_FORMAT_COMPRESSED_BC1_RGB,
    TEXTURE_FORMAT_COMPRESSED_BC1_SRGB,

    // RGB A-4bit / RGB (not the best quality, it is better to use BC3)
    TEXTURE_FORMAT_COMPRESSED_BC2_RGBA,
    TEXTURE_FORMAT_COMPRESSED_BC2_SRGB_ALPHA,

    // RGB A-8bit
    TEXTURE_FORMAT_COMPRESSED_BC3_RGBA,
    TEXTURE_FORMAT_COMPRESSED_BC3_SRGB_ALPHA,

    // R single channel texture (use for metalmap, glossmap, etc)
    TEXTURE_FORMAT_COMPRESSED_BC4_R,
    TEXTURE_FORMAT_COMPRESSED_BC4_R_SIGNED,

    // RG two channel texture (use for normal map or two grayscale maps)
    TEXTURE_FORMAT_COMPRESSED_BC5_RG,
    TEXTURE_FORMAT_COMPRESSED_BC5_RG_SIGNED,

    // RGB half float HDR
    TEXTURE_FORMAT_COMPRESSED_BC6H,
    TEXTURE_FORMAT_COMPRESSED_BC6H_SIGNED,

    // RGB[A], best quality, every block is compressed different
    TEXTURE_FORMAT_COMPRESSED_BC7_RGBA,
    TEXTURE_FORMAT_COMPRESSED_BC7_SRGB_ALPHA,

    //
    // Depth and stencil formats
    //

    TEXTURE_FORMAT_STENCIL1,
    TEXTURE_FORMAT_STENCIL4,
    TEXTURE_FORMAT_STENCIL8,
    TEXTURE_FORMAT_STENCIL16,
    TEXTURE_FORMAT_DEPTH16,
    TEXTURE_FORMAT_DEPTH24,
    TEXTURE_FORMAT_DEPTH32,
    TEXTURE_FORMAT_DEPTH24_STENCIL8,
    TEXTURE_FORMAT_DEPTH32F_STENCIL8
};

AN_INLINE bool IsCompressedFormat( TEXTURE_FORMAT Format )
{
    switch ( Format ) {
    case TEXTURE_FORMAT_COMPRESSED_BC1_RGB:
    case TEXTURE_FORMAT_COMPRESSED_BC1_SRGB:
    case TEXTURE_FORMAT_COMPRESSED_BC2_RGBA:
    case TEXTURE_FORMAT_COMPRESSED_BC2_SRGB_ALPHA:
    case TEXTURE_FORMAT_COMPRESSED_BC3_RGBA:
    case TEXTURE_FORMAT_COMPRESSED_BC3_SRGB_ALPHA:
    case TEXTURE_FORMAT_COMPRESSED_BC4_R:
    case TEXTURE_FORMAT_COMPRESSED_BC4_R_SIGNED:
    case TEXTURE_FORMAT_COMPRESSED_BC5_RG:
    case TEXTURE_FORMAT_COMPRESSED_BC5_RG_SIGNED:
    case TEXTURE_FORMAT_COMPRESSED_BC6H:
    case TEXTURE_FORMAT_COMPRESSED_BC6H_SIGNED:
    case TEXTURE_FORMAT_COMPRESSED_BC7_RGBA:
    case TEXTURE_FORMAT_COMPRESSED_BC7_SRGB_ALPHA:
        return true;
    default:;
    }
    return false;
}

/// Texture types
enum TEXTURE_TYPE : uint8_t
{
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
    TEXTURE_CUBE_MAP_ARRAY,

    TEXTURE_RECT_GL // Can be used only with OpenGL backend
};

enum TEXTURE_SWIZZLE : uint8_t
{
    TEXTURE_SWIZZLE_IDENTITY = 0,
    TEXTURE_SWIZZLE_ZERO = 1,
    TEXTURE_SWIZZLE_ONE = 2,
    TEXTURE_SWIZZLE_R = 3,
    TEXTURE_SWIZZLE_G = 4,
    TEXTURE_SWIZZLE_B = 5,
    TEXTURE_SWIZZLE_A = 6
};

struct STextureResolution1D
{
    uint32_t Width;

    STextureResolution1D() = default;
    STextureResolution1D( uint32_t InWidth )
        : Width( InWidth )
    {
    }
};

struct STextureResolution1DArray
{
    uint32_t Width;
    uint32_t NumLayers;

    STextureResolution1DArray() = default;
    STextureResolution1DArray( uint32_t InWidth, uint32_t InNumLayers )
        : Width( InWidth ), NumLayers( InNumLayers )
    {
    }
};

struct STextureResolution2D
{
    uint32_t Width;
    uint32_t Height;

    STextureResolution2D() = default;
    STextureResolution2D( uint32_t InWidth, uint32_t InHeight )
        : Width( InWidth ), Height( InHeight )
    {
    }
};

struct STextureResolution2DArray
{
    uint32_t Width;
    uint32_t Height;
    uint32_t NumLayers;

    STextureResolution2DArray() = default;
    STextureResolution2DArray( uint32_t InWidth, uint32_t InHeight, uint32_t InNumLayers )
        : Width( InWidth ), Height( InHeight ), NumLayers( InNumLayers )
    {
    }
};

struct STextureResolution3D
{
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;

    STextureResolution3D() = default;
    STextureResolution3D( uint32_t InWidth, uint32_t InHeight, uint32_t InDepth )
        : Width( InWidth ), Height( InHeight ), Depth( InDepth )
    {
    }
};

struct STextureResolutionCubemap
{
    uint32_t Width;

    STextureResolutionCubemap() = default;
    STextureResolutionCubemap( uint32_t InWidth )
        : Width( InWidth )
    {
    }
};

struct STextureResolutionCubemapArray
{
    uint32_t Width;
    uint32_t NumLayers;

    STextureResolutionCubemapArray() = default;
    STextureResolutionCubemapArray( uint32_t InWidth, uint32_t InNumLayers )
        : Width( InWidth ), NumLayers( InNumLayers )
    {
    }
};

struct STextureResolutionRectGL
{
    uint32_t Width;
    uint32_t Height;

    STextureResolutionRectGL() = default;
    STextureResolutionRectGL( uint32_t InWidth, uint32_t InHeight )
        : Width( InWidth ), Height( InHeight )
    {
    }
};

struct STextureResolution
{
    union {
        STextureResolution1D         Tex1D;

        STextureResolution1DArray    Tex1DArray;

        STextureResolution2D         Tex2D;

        STextureResolution2DArray    Tex2DArray;

        STextureResolution3D         Tex3D;

        STextureResolutionCubemap    TexCubemap;

        STextureResolutionCubemapArray TexCubemapArray;

        STextureResolutionRectGL     TexRect;
    };

    bool operator==( STextureResolution const & Rhs ) const {
        return memcmp( this, &Rhs, sizeof( *this ) ) == 0;
    }
};

struct STextureOffset
{
    uint16_t Lod;
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct STextureDimension
{
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct STextureRect
{
    STextureOffset Offset;
    STextureDimension Dimension;
};

struct TextureCopy
{
    STextureRect SrcRect;
    STextureOffset DstOffset;
};

struct STextureMultisampleInfo
{
    uint8_t NumSamples;             /// The number of samples in the multisample texture's image
    bool    bFixedSampleLocations;  /// Specifies whether the image will use identical sample locations
                                    /// and the same number of samples for all texels in the image,
                                    /// and the sample locations will not depend on the internal format or size of the image

    STextureMultisampleInfo() {
        NumSamples = 1;
        bFixedSampleLocations = false;
    }

    STextureMultisampleInfo & SetSamples( int InNumSamples ) {
        NumSamples = InNumSamples;
        return *this;
    }

    STextureMultisampleInfo & SetFixedSampleLocations( bool InbFixedSampleLocations ) {
        bFixedSampleLocations = InbFixedSampleLocations;
        return *this;
    }
};

struct STextureSwizzle
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;

    STextureSwizzle()
        : R(TEXTURE_SWIZZLE_IDENTITY),
          G(TEXTURE_SWIZZLE_IDENTITY),
          B(TEXTURE_SWIZZLE_IDENTITY),
          A(TEXTURE_SWIZZLE_IDENTITY)
    {
    }

    STextureSwizzle( uint8_t InR,
                    uint8_t InG,
                    uint8_t InB,
                    uint8_t InA )
        : R(InR), G(InG), B(InB), A(InA)
    {
    }

    bool operator==( STextureSwizzle const & Rhs ) const {
        return memcmp( this, &Rhs, sizeof( *this ) ) == 0;
    }
};

struct STextureCreateInfo
{
    TEXTURE_TYPE            Type;
    TEXTURE_FORMAT          Format;
    STextureResolution      Resolution;
    STextureMultisampleInfo Multisample;
    STextureSwizzle         Swizzle;
    uint16_t                NumLods;

    STextureCreateInfo()
        : Type( TEXTURE_2D )
        , Format( TEXTURE_FORMAT_RGBA8 )
        , NumLods( 1 )
    {
    }
};

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution1D const &     Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_1D;
    createInfo.Format = Format;
    createInfo.Resolution.Tex1D = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution1DArray const &Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_1D_ARRAY;
    createInfo.Format = Format;
    createInfo.Resolution.Tex1DArray = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution2D const &     Resolution,
    STextureMultisampleInfo const &  Multisample = STextureMultisampleInfo(),
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_2D;
    createInfo.Format = Format;
    createInfo.Resolution.Tex2D = Resolution;
    createInfo.Multisample = Multisample;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution2DArray const &Resolution,
    STextureMultisampleInfo const &  Multisample = STextureMultisampleInfo(),
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_2D_ARRAY;
    createInfo.Format = Format;
    createInfo.Resolution.Tex2DArray = Resolution;
    createInfo.Multisample = Multisample;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution3D const &     Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_3D;
    createInfo.Format = Format;
    createInfo.Resolution.Tex3D = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolutionCubemap const &Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_CUBE_MAP;
    createInfo.Format = Format;
    createInfo.Resolution.TexCubemap = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolutionCubemapArray const &Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_CUBE_MAP_ARRAY;
    createInfo.Format = Format;
    createInfo.Resolution.TexCubemapArray = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline STextureCreateInfo MakeTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolutionRectGL const & Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    STextureCreateInfo createInfo;
    createInfo.Type = TEXTURE_RECT_GL;
    createInfo.Format = Format;
    createInfo.Resolution.TexRect = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

struct STextureLodInfo
{
    STextureResolution Resoultion;
    bool               bCompressed;
    size_t             CompressedDataSizeInBytes;
};

struct STextureViewCreateInfo
{
    TEXTURE_TYPE    Type;
    TEXTURE_FORMAT  Format;
    ITexture *      pOriginalTexture;
    uint16_t        MinLod;
    uint16_t        NumLods;
    uint16_t        MinLayer;
    uint16_t        NumLayers;
    bool            bMultisample;
};

enum DATA_FORMAT : uint8_t
{
    FORMAT_BYTE1,
    FORMAT_BYTE2,
    FORMAT_BYTE3,
    FORMAT_BYTE4,

    FORMAT_UBYTE1,
    FORMAT_UBYTE2,
    FORMAT_UBYTE3,
    FORMAT_UBYTE4,

    FORMAT_SHORT1,
    FORMAT_SHORT2,
    FORMAT_SHORT3,
    FORMAT_SHORT4,

    FORMAT_USHORT1,
    FORMAT_USHORT2,
    FORMAT_USHORT3,
    FORMAT_USHORT4,

    FORMAT_INT1,
    FORMAT_INT2,
    FORMAT_INT3,
    FORMAT_INT4,

    FORMAT_UINT1,
    FORMAT_UINT2,
    FORMAT_UINT3,
    FORMAT_UINT4,

    FORMAT_HALF1,
    FORMAT_HALF2,
    FORMAT_HALF3,
    FORMAT_HALF4,

    FORMAT_FLOAT1,
    FORMAT_FLOAT2,
    FORMAT_FLOAT3,
    FORMAT_FLOAT4
};

class ITextureBase : public IDeviceObject
{
public:
    ITextureBase( IDevice * Device ) : IDeviceObject( Device ) {}
};

class ITexture : public ITextureBase
{
public:
    ITexture( IDevice * Device ) : ITextureBase( Device ) {}

    TEXTURE_TYPE GetType() const { return Type; }

    uint32_t GetWidth() const;

    uint32_t GetHeight() const;

    uint32_t GetNumLods() const { return NumLods; }

    TEXTURE_FORMAT GetFormat() const { return Format; }

    bool IsCompressed() const { return bCompressed; }

    STextureResolution const & GetResolution() const { return Resolution; }

    STextureSwizzle const & GetSwizzle() const { return Swizzle; }

    bool IsMultisample() const { return NumSamples > 1; }

    uint8_t GetNumSamples() const { return NumSamples; }

    bool FixedSampleLocations() const { return bFixedSampleLocations; }

    //bool IsTextureView() const { return bTextureView; }

    /// Only for TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP, or TEXTURE_CUBE_MAP_ARRAY.
    virtual void GenerateLods() = 0;

    virtual void GetLodInfo( uint16_t _Lod, STextureLodInfo * _Info ) const = 0;

    /// Client-side call function. Read data to client memory.
    virtual void Read( uint16_t _Lod,
                       DATA_FORMAT _Format, /// Specifies a pixel format for the returned data
                       size_t _SizeInBytes,
                       unsigned int _Alignment,           /// Specifies alignment of destination data
                       void * _SysMem ) = 0;

    /// Client-side call function. Read data to client memory.
    virtual void ReadRect( STextureRect const & _Rectangle,
                           DATA_FORMAT _Format, /// Specifies a pixel format for the returned data
                           size_t _SizeInBytes,
                           unsigned int _Alignment,           /// Specifies alignment of destination data
                           void * _SysMem ) = 0;

    /// Client-side call function. Write data from client memory.
    virtual bool Write( uint16_t _Lod,
                        DATA_FORMAT _Format, /// Specifies a pixel format for the input data
                        size_t _SizeInBytes,
                        unsigned int _Alignment,           /// Specifies alignment of source data
                        const void * _SysMem ) = 0;

    /// Only for TEXTURE_1D TEXTURE_1D_ARRAY TEXTURE_2D TEXTURE_2D_ARRAY TEXTURE_3D
    /// Client-side call function. Write data from client memory.
    virtual bool WriteRect( STextureRect const & _Rectangle,
                            DATA_FORMAT _Format, /// Specifies a pixel format for the input data
                            size_t _SizeInBytes,
                            unsigned int _Alignment,               /// Specifies alignment of source data
                            const void * _SysMem ) = 0;

    virtual void Invalidate( uint16_t _Lod ) = 0;

    virtual void InvalidateRect( uint32_t _NumRectangles, STextureRect const * _Rectangles ) = 0;

    //
    // Utilites
    //

    static int CalcMaxLods( TEXTURE_TYPE _Type, STextureResolution const & _Resolution );

protected:
    TEXTURE_TYPE            Type;
    TEXTURE_FORMAT          Format;
    STextureResolution      Resolution;
    STextureSwizzle         Swizzle;
    uint8_t                 NumSamples;
    bool                    bFixedSampleLocations;
    uint16_t                NumLods;
    bool                    bTextureView;
    bool                    bCompressed;
};

AN_INLINE uint32_t ITexture::GetWidth() const {
    switch ( Type ) {
    case TEXTURE_1D:
        return Resolution.Tex1D.Width;
    case TEXTURE_1D_ARRAY:
        return Resolution.Tex1DArray.Width;
    case TEXTURE_2D:
        return Resolution.Tex2D.Width;
    case TEXTURE_2D_ARRAY:
        return Resolution.Tex2DArray.Width;
    case TEXTURE_3D:
        return Resolution.Tex3D.Width;
    case TEXTURE_CUBE_MAP:
        return Resolution.TexCubemap.Width;
    case TEXTURE_CUBE_MAP_ARRAY:
        return Resolution.TexCubemapArray.Width;
    case TEXTURE_RECT_GL:
        return Resolution.TexRect.Width;
    }
    AN_ASSERT( 0 );
    return 0;
}

AN_INLINE uint32_t ITexture::GetHeight() const {
    switch ( Type ) {
    case TEXTURE_1D:
    case TEXTURE_1D_ARRAY:
        return 1;
    case TEXTURE_2D:
        return Resolution.Tex2D.Height;
    case TEXTURE_2D_ARRAY:
        return Resolution.Tex2DArray.Height;
    case TEXTURE_3D:
        return Resolution.Tex3D.Height;
    case TEXTURE_CUBE_MAP:
        return Resolution.TexCubemap.Width;
    case TEXTURE_CUBE_MAP_ARRAY:
        return Resolution.TexCubemapArray.Width;
    case TEXTURE_RECT_GL:
        return Resolution.TexRect.Height;
    }
    AN_ASSERT( 0 );
    return 0;
}

}
