/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#pragma once

#include "GHIBasic.h"
#include "GHIBuffer.h"

namespace GHI {

class Device;
class Buffer;
class Texture;

/// Input pixel format for texture
enum TEXTURE_PIXEL_FORMAT : uint8_t
{
    PIXEL_FORMAT_BYTE_R,
    PIXEL_FORMAT_BYTE_RG,
    PIXEL_FORMAT_BYTE_RGB,
    PIXEL_FORMAT_BYTE_RGBA,

    PIXEL_FORMAT_BYTE_BGR,
    PIXEL_FORMAT_BYTE_BGRA,

    PIXEL_FORMAT_UBYTE_R,
    PIXEL_FORMAT_UBYTE_RG,
    PIXEL_FORMAT_UBYTE_RGB,
    PIXEL_FORMAT_UBYTE_RGBA,

    PIXEL_FORMAT_UBYTE_BGR,
    PIXEL_FORMAT_UBYTE_BGRA,

    PIXEL_FORMAT_SHORT_R,
    PIXEL_FORMAT_SHORT_RG,
    PIXEL_FORMAT_SHORT_RGB,
    PIXEL_FORMAT_SHORT_RGBA,

    PIXEL_FORMAT_SHORT_BGR,
    PIXEL_FORMAT_SHORT_BGRA,

    PIXEL_FORMAT_USHORT_R,
    PIXEL_FORMAT_USHORT_RG,
    PIXEL_FORMAT_USHORT_RGB,
    PIXEL_FORMAT_USHORT_RGBA,

    PIXEL_FORMAT_USHORT_BGR,
    PIXEL_FORMAT_USHORT_BGRA,

    PIXEL_FORMAT_INT_R,
    PIXEL_FORMAT_INT_RG,
    PIXEL_FORMAT_INT_RGB,
    PIXEL_FORMAT_INT_RGBA,

    PIXEL_FORMAT_INT_BGR,
    PIXEL_FORMAT_INT_BGRA,

    PIXEL_FORMAT_UINT_R,
    PIXEL_FORMAT_UINT_RG,
    PIXEL_FORMAT_UINT_RGB,
    PIXEL_FORMAT_UINT_RGBA,

    PIXEL_FORMAT_UINT_BGR,
    PIXEL_FORMAT_UINT_BGRA,

    PIXEL_FORMAT_HALF_R,          /// only with IsHalfFloatPixelSupported
    PIXEL_FORMAT_HALF_RG,         /// only with IsHalfFloatPixelSupported
    PIXEL_FORMAT_HALF_RGB,        /// only with IsHalfFloatPixelSupported
    PIXEL_FORMAT_HALF_RGBA,       /// only with IsHalfFloatPixelSupported

    PIXEL_FORMAT_HALF_BGR,        /// only with IsHalfFloatPixelSupported
    PIXEL_FORMAT_HALF_BGRA,       /// only with IsHalfFloatPixelSupported

    PIXEL_FORMAT_FLOAT_R,
    PIXEL_FORMAT_FLOAT_RG,
    PIXEL_FORMAT_FLOAT_RGB,
    PIXEL_FORMAT_FLOAT_RGBA,

    PIXEL_FORMAT_FLOAT_BGR,
    PIXEL_FORMAT_FLOAT_BGRA,

    PIXEL_FORMAT_COMPRESSED_RGB_DXT1,
    PIXEL_FORMAT_COMPRESSED_RGBA_DXT1,
    PIXEL_FORMAT_COMPRESSED_RGBA_DXT3,
    PIXEL_FORMAT_COMPRESSED_RGBA_DXT5,

    PIXEL_FORMAT_COMPRESSED_SRGB_DXT1,                /// only with IsTextureCompressionS3tcSupported()
    PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_DXT1,          /// only with IsTextureCompressionS3tcSupported()
    PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_DXT3,          /// only with IsTextureCompressionS3tcSupported()
    PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_DXT5,          /// only with IsTextureCompressionS3tcSupported()

    PIXEL_FORMAT_COMPRESSED_RED_RGTC1,
    PIXEL_FORMAT_COMPRESSED_RG_RGTC2,

    PIXEL_FORMAT_COMPRESSED_RGBA_BPTC_UNORM,
    PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
    PIXEL_FORMAT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
    PIXEL_FORMAT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,

    PIXEL_FORMAT_STENCIL,
    PIXEL_FORMAT_DEPTH16,
    PIXEL_FORMAT_DEPTH24,
    PIXEL_FORMAT_DEPTH32,
    PIXEL_FORMAT_DEPTH24_STENCIL8,
    PIXEL_FORMAT_DEPTH32F_STENCIL8
};

/// Internal pixel format for texture
enum INTERNAL_PIXEL_FORMAT : uint8_t
{
    INTERNAL_PIXEL_FORMAT_R8,             /// RED   8
    INTERNAL_PIXEL_FORMAT_R8_SNORM,       /// RED   s8
    INTERNAL_PIXEL_FORMAT_R16,            /// RED   16
    INTERNAL_PIXEL_FORMAT_R16_SNORM,      /// RED   s16
    INTERNAL_PIXEL_FORMAT_RG8,            /// RG    8   8
    INTERNAL_PIXEL_FORMAT_RG8_SNORM,      /// RG    s8  s8
    INTERNAL_PIXEL_FORMAT_RG16,           /// RG    16  16
    INTERNAL_PIXEL_FORMAT_RG16_SNORM,     /// RG    s16 s16
    INTERNAL_PIXEL_FORMAT_R3_G3_B2,       /// RGB   3  3  2
    INTERNAL_PIXEL_FORMAT_RGB4,           /// RGB   4  4  4
    INTERNAL_PIXEL_FORMAT_RGB5,           /// RGB   5  5  5
    INTERNAL_PIXEL_FORMAT_RGB8,           /// RGB   8  8  8
    INTERNAL_PIXEL_FORMAT_RGB8_SNORM,     /// RGB   s8  s8  s8
    INTERNAL_PIXEL_FORMAT_RGB10,          /// RGB   10  10  10
    INTERNAL_PIXEL_FORMAT_RGB12,          /// RGB   12  12  12
    INTERNAL_PIXEL_FORMAT_RGB16,          /// RGB   16  16  16
    INTERNAL_PIXEL_FORMAT_RGB16_SNORM,    /// RGB   s16  s16  s16
    INTERNAL_PIXEL_FORMAT_RGBA2,          /// RGB   2  2  2  2
    INTERNAL_PIXEL_FORMAT_RGBA4,          /// RGB   4  4  4  4
    INTERNAL_PIXEL_FORMAT_RGB5_A1,        /// RGBA  5  5  5  1
    INTERNAL_PIXEL_FORMAT_RGBA8,          /// RGBA  8  8  8  8
    INTERNAL_PIXEL_FORMAT_RGBA8_SNORM,    /// RGBA  s8  s8  s8  s8
    INTERNAL_PIXEL_FORMAT_RGB10_A2,       /// RGBA  10  10  10  2
    INTERNAL_PIXEL_FORMAT_RGB10_A2UI,     /// RGBA  ui10  ui10  ui10  ui2
    INTERNAL_PIXEL_FORMAT_RGBA12,         /// RGBA  12  12  12  12
    INTERNAL_PIXEL_FORMAT_RGBA16,         /// RGBA  16  16  16  16
    INTERNAL_PIXEL_FORMAT_RGBA16_SNORM,   /// RGBA  s16  s16  s16  s16
    INTERNAL_PIXEL_FORMAT_SRGB8,          /// RGB   8  8  8
    INTERNAL_PIXEL_FORMAT_SRGB8_ALPHA8,   /// RGBA  8  8  8  8
    INTERNAL_PIXEL_FORMAT_R16F,           /// RED   f16
    INTERNAL_PIXEL_FORMAT_RG16F,          /// RG    f16  f16
    INTERNAL_PIXEL_FORMAT_RGB16F,         /// RGB   f16  f16  f16
    INTERNAL_PIXEL_FORMAT_RGBA16F,        /// RGBA  f16  f16  f16  f16
    INTERNAL_PIXEL_FORMAT_R32F,           /// RED   f32
    INTERNAL_PIXEL_FORMAT_RG32F,          /// RG    f32  f32
    INTERNAL_PIXEL_FORMAT_RGB32F,         /// RGB   f32  f32  f32
    INTERNAL_PIXEL_FORMAT_RGBA32F,        /// RGBA  f32  f32  f32  f32
    INTERNAL_PIXEL_FORMAT_R11F_G11F_B10F, /// RGB   f11  f11  f10
    INTERNAL_PIXEL_FORMAT_RGB9_E5,        /// RGB   9  9  9     5
    INTERNAL_PIXEL_FORMAT_R8I,            /// RED   i8
    INTERNAL_PIXEL_FORMAT_R8UI,           /// RED   ui8
    INTERNAL_PIXEL_FORMAT_R16I,           /// RED   i16
    INTERNAL_PIXEL_FORMAT_R16UI,          /// RED   ui16
    INTERNAL_PIXEL_FORMAT_R32I,           /// RED   i32
    INTERNAL_PIXEL_FORMAT_R32UI,          /// RED   ui32
    INTERNAL_PIXEL_FORMAT_RG8I,           /// RG    i8   i8
    INTERNAL_PIXEL_FORMAT_RG8UI,          /// RG    ui8   ui8
    INTERNAL_PIXEL_FORMAT_RG16I,          /// RG    i16   i16
    INTERNAL_PIXEL_FORMAT_RG16UI,         /// RG    ui16  ui16
    INTERNAL_PIXEL_FORMAT_RG32I,          /// RG    i32   i32
    INTERNAL_PIXEL_FORMAT_RG32UI,         /// RG    ui32  ui32
    INTERNAL_PIXEL_FORMAT_RGB8I,          /// RGB   i8   i8   i8
    INTERNAL_PIXEL_FORMAT_RGB8UI,         /// RGB   ui8  ui8  ui8
    INTERNAL_PIXEL_FORMAT_RGB16I,         /// RGB   i16  i16  i16
    INTERNAL_PIXEL_FORMAT_RGB16UI,        /// RGB   ui16 ui16 ui16
    INTERNAL_PIXEL_FORMAT_RGB32I,         /// RGB   i32  i32  i32
    INTERNAL_PIXEL_FORMAT_RGB32UI,        /// RGB   ui32 ui32 ui32
    INTERNAL_PIXEL_FORMAT_RGBA8I,         /// RGBA  i8   i8   i8   i8
    INTERNAL_PIXEL_FORMAT_RGBA8UI,        /// RGBA  ui8  ui8  ui8  ui8
    INTERNAL_PIXEL_FORMAT_RGBA16I,        /// RGBA  i16  i16  i16  i16
    INTERNAL_PIXEL_FORMAT_RGBA16UI,       /// RGBA  ui16 ui16 ui16 ui16
    INTERNAL_PIXEL_FORMAT_RGBA32I,        /// RGBA  i32  i32  i32  i32
    INTERNAL_PIXEL_FORMAT_RGBA32UI,       /// RGBA  ui32 ui32 ui32 ui32

    // Compressed formats:
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RED,                     /// RED  Generic
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RG,                      /// RG  Generic
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB,                     /// RGB  Generic
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA,                    /// RGBA  Generic
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB,                    /// RGB  Generic
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA,              /// RGBA  Generic
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RED_RGTC1,               /// RED  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SIGNED_RED_RGTC1,        /// RED  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RG_RGTC2,                /// RG  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SIGNED_RG_RGTC2,         /// RG  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_BPTC_UNORM,         /// RGBA  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,   /// RGBA  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,   /// RGB  Specific
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, /// RGB  Specific

    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB_S3TC_DXT1,           /// only with IsTextureCompressionS3tcSupported()
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT1,          /// only with IsTextureCompressionS3tcSupported()
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT3,          /// only with IsTextureCompressionS3tcSupported()
    INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT5,          /// only with IsTextureCompressionS3tcSupported()

    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_S3TC_DXT1,          /// only with IsTextureCompressionS3tcSupported()
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT1,    /// only with IsTextureCompressionS3tcSupported()
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT3,    /// only with IsTextureCompressionS3tcSupported()
    INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT5,    /// only with IsTextureCompressionS3tcSupported()

    // Depth and stencil formats:
    INTERNAL_PIXEL_FORMAT_STENCIL1,
    INTERNAL_PIXEL_FORMAT_STENCIL4,
    INTERNAL_PIXEL_FORMAT_STENCIL8,
    INTERNAL_PIXEL_FORMAT_STENCIL16,
    INTERNAL_PIXEL_FORMAT_DEPTH16,
    INTERNAL_PIXEL_FORMAT_DEPTH24,
    INTERNAL_PIXEL_FORMAT_DEPTH32,
    INTERNAL_PIXEL_FORMAT_DEPTH24_STENCIL8,
    INTERNAL_PIXEL_FORMAT_DEPTH32F_STENCIL8
};

/// Texture types
enum TEXTURE_TYPE : uint8_t
{
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY,
    TEXTURE_2D_ARRAY_MULTISAMPLE,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
    TEXTURE_CUBE_MAP_ARRAY,
    TEXTURE_RECT

    // TODO: Add proxy???
};

struct TextureResolution1D {
    uint32_t Width;
};

struct TextureResolution1DArray {
    uint32_t Width;
    uint32_t NumLayers;
};

struct TextureResolution2D {
    uint32_t Width;
    uint32_t Height;
};

struct TextureResolution2DArray {
    uint32_t Width;
    uint32_t Height;
    uint32_t NumLayers;
};

struct TextureResolution3D {
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
};

struct TextureResolutionCubemap {
    uint32_t Width;
};

struct TextureResolutionCubemapArray {
    uint32_t Width;
    uint32_t NumLayers;
};

struct TextureResolutionRect {
    uint32_t Width;
    uint32_t Height;
};

struct TextureResolution {
    union {
        TextureResolution1D         Tex1D;

        TextureResolution1DArray    Tex1DArray;

        TextureResolution2D         Tex2D;

        TextureResolution2DArray    Tex2DArray;

        TextureResolution3D         Tex3D;

        TextureResolutionCubemap    TexCubemap;

        TextureResolutionCubemapArray TexCubemapArray;

        TextureResolutionRect       TexRect;
    };
};

struct TextureOffset {
    uint16_t Lod;
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct TextureDimension {
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
};

struct TextureRect {
    TextureOffset Offset;
    TextureDimension Dimension;
};

struct TextureCopy {
    TextureRect SrcRect;
    TextureOffset DstOffset;
};

struct TextureMultisampleInfo {
    uint8_t NumSamples;             /// The number of samples in the multisample texture's image
    bool    bFixedSampleLocations;  /// Specifies whether the image will use identical sample locations
                                    /// and the same number of samples for all texels in the image,
                                    /// and the sample locations will not depend on the internal format or size of the image
};

struct TextureCreateInfo {
    TEXTURE_TYPE           Type;
    INTERNAL_PIXEL_FORMAT  InternalFormat;  /// Internal data format, ignored for compressed input data.
    TextureResolution      Resolution;
    TextureMultisampleInfo Multisample;     /// for TEXTURE_2D_MULTISAMPLE and TEXTURE_2D_ARRAY_MULTISAMPLE
};

struct TextureStorageCreateInfo {
    TEXTURE_TYPE            Type;
    INTERNAL_PIXEL_FORMAT   InternalFormat;
    TextureResolution       Resolution;
    TextureMultisampleInfo  Multisample;
    uint16_t                NumLods;
};

struct TextureLodInfo {
    TextureResolution  Resoultion;
    bool               bCompressed;
    size_t             CompressedDataByteLength;
};

struct TextureInitialData {
    TEXTURE_PIXEL_FORMAT PixelFormat;  /// Input data format. NOTE: TextureRect cannot be compressed.
    const void *         SysMem;       /// Optional. Set it to NULL if you want to create empty texture.
    unsigned int         Alignment;    /// 1 byte by default.
    size_t               SizeInBytes;   /// Byte length of input data (only for compressed input data).
                                       /// Ignored for uncompressed input data.

    /// Texture input data ignored for multisample textures
};

struct TextureViewCreateInfo {
    TEXTURE_TYPE            Type;
    INTERNAL_PIXEL_FORMAT   InternalFormat;
    Texture *               pOriginalTexture;
    uint16_t                MinLod;
    uint16_t                NumLods;
    uint16_t                MinLayer;
    uint16_t                NumLayers;
};

class Texture final : public NonCopyable, IObjectInterface {
public:
    Texture();
    ~Texture();

    void Initialize( TextureCreateInfo const & _CreateInfo, TextureInitialData const * _InitialData = nullptr );

    void InitializeStorage( TextureStorageCreateInfo const & _CreateInfo );

    void InitializeTextureBuffer( BUFFER_DATA_TYPE _DataType, Buffer const & _Buffer );

    void InitializeTextureBuffer( BUFFER_DATA_TYPE _DataType, Buffer const & _Buffer, size_t _Offset, size_t _SizeInBytes );

    bool InitializeView( TextureViewCreateInfo const & _CreateInfo );

    void Deinitialize();

    TEXTURE_TYPE GetType() const { return CreateInfo.Type; }

    uint32_t GetWidth() const;

    uint32_t GetHeight() const;

    INTERNAL_PIXEL_FORMAT GetInternalPixelFormat() const { return CreateInfo.InternalFormat; }

    TextureResolution const & GetResoulution() const { return CreateInfo.Resolution; }

    uint8_t GetSamplesCount() const { return CreateInfo.Multisample.NumSamples; }

    bool FixedSampleLocations() const { return CreateInfo.Multisample.bFixedSampleLocations; }

    bool IsImmutableStorage() const { return bImmutableStorage; }

    bool IsTextureBuffer() const { return bTextureBuffer; }

    bool IsTextureView() const { return bTextureView; }

    /// Only for mutable textures
    bool Realloc( TextureCreateInfo const & _CreateInfo, TextureInitialData const * _InitialData = nullptr );

    /// Only for mutable TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP, or TEXTURE_CUBE_MAP_ARRAY.
    bool CreateLod( uint16_t _Lod, TextureInitialData const * _InitialData );

    // FIXME: move to CommandBuffer?
    /// Only for TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP, or TEXTURE_CUBE_MAP_ARRAY.
    void GenerateLods();

    void GetLodInfo( uint16_t _Lod, TextureLodInfo * _Info ) const;

    /// Client-side call function. Read data to client memory.
    void Read( uint16_t _Lod,
               TEXTURE_PIXEL_FORMAT _PixelFormat, /// Specifies a pixel format for the returned data
               size_t _SizeInBytes,
               unsigned int _Alignment,           /// Specifies alignment of destination data
               void * _SysMem );

    /// Client-side call function. Read data to client memory.
    void ReadRect( TextureRect const & _Rectangle,
                   TEXTURE_PIXEL_FORMAT _PixelFormat, /// Specifies a pixel format for the returned data
                   size_t _SizeInBytes,
                   unsigned int _Alignment,           /// Specifies alignment of destination data
                   void * _SysMem );

    /// Client-side call function. Write data from client memory.
    bool Write( uint16_t _Lod,
                TEXTURE_PIXEL_FORMAT _PixelFormat, /// Specifies a pixel format for the input data
                size_t _SizeInBytes,
                unsigned int _Alignment,           /// Specifies alignment of source data
                const void * _SysMem );

    /// Only for TEXTURE_1D TEXTURE_1D_ARRAY TEXTURE_2D TEXTURE_2D_ARRAY TEXTURE_3D
    /// Client-side call function. Write data from client memory.
    bool WriteRect( TextureRect const & _Rectangle,
                    TEXTURE_PIXEL_FORMAT _PixelFormat, // Specifies a pixel format for the input data
                    size_t _SizeInBytes,
                    unsigned int _Alignment,               // Specifies alignment of source data
                    const void * _SysMem );

    void Invalidate( uint16_t _Lod );

    void InvalidateRect( uint32_t _NumRectangles, TextureRect const * _Rectangles );

    size_t GetTextureBufferOffset( uint16_t _Lod );
    size_t GetTextureBufferByteLength( uint16_t _Lod );

    void * GetHandle() const { return Handle; }

    //
    // Utilites
    //

    static int CalcMaxLods( TEXTURE_TYPE _Type, TextureResolution const & _Resolution );

    static bool LookupImageFormat( const char * _FormatQualifier, INTERNAL_PIXEL_FORMAT * _InternalPixelFormat );

    static const char * LookupImageFormatQualifier( INTERNAL_PIXEL_FORMAT _InternalPixelFormat );

private:
    static void CreateTextureLod( TextureCreateInfo const & _CreateInfo, uint16_t _Lod, TextureInitialData const * _InitialData );

    Device * pDevice;
    void * Handle;
    TextureCreateInfo CreateInfo;
    bool bImmutableStorage;
    bool bTextureBuffer;
    bool bTextureView;
};

}
