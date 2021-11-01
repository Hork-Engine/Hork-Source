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
#include <Core/Public/HashFunc.h>

namespace RenderCore
{

class ITexture;

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

enum TEXTURE_FORMAT : uint8_t
{
    //
    // Normalized signed/unsigned formats
    //

    TEXTURE_FORMAT_R8,           /// RED   8
    TEXTURE_FORMAT_R8_SNORM,     /// RED   s8
    TEXTURE_FORMAT_R16,          /// RED   16
    TEXTURE_FORMAT_R16_SNORM,    /// RED   s16
    TEXTURE_FORMAT_RG8,          /// RG    8   8
    TEXTURE_FORMAT_RG8_SNORM,    /// RG    s8  s8
    TEXTURE_FORMAT_RG16,         /// RG    16  16
    TEXTURE_FORMAT_RG16_SNORM,   /// RG    s16 s16
    TEXTURE_FORMAT_R3_G3_B2,     /// RGB   3  3  2
    TEXTURE_FORMAT_RGB4,         /// RGB   4  4  4
    TEXTURE_FORMAT_RGB5,         /// RGB   5  5  5
    TEXTURE_FORMAT_RGB8,         /// RGB   8  8  8
    TEXTURE_FORMAT_RGB8_SNORM,   /// RGB   s8  s8  s8
    TEXTURE_FORMAT_RGB10,        /// RGB   10  10  10
    TEXTURE_FORMAT_RGB12,        /// RGB   12  12  12
    TEXTURE_FORMAT_RGB16,        /// RGB   16  16  16
    TEXTURE_FORMAT_RGB16_SNORM,  /// RGB   s16  s16  s16
    TEXTURE_FORMAT_RGBA2,        /// RGB   2  2  2  2
    TEXTURE_FORMAT_RGBA4,        /// RGB   4  4  4  4
    TEXTURE_FORMAT_RGB5_A1,      /// RGBA  5  5  5  1
    TEXTURE_FORMAT_RGBA8,        /// RGBA  8  8  8  8
    TEXTURE_FORMAT_RGBA8_SNORM,  /// RGBA  s8  s8  s8  s8
    TEXTURE_FORMAT_RGB10_A2,     /// RGBA  10  10  10  2
    TEXTURE_FORMAT_RGB10_A2UI,   /// RGBA  ui10  ui10  ui10  ui2
    TEXTURE_FORMAT_RGBA12,       /// RGBA  12  12  12  12
    TEXTURE_FORMAT_RGBA16,       /// RGBA  16  16  16  16
    TEXTURE_FORMAT_RGBA16_SNORM, /// RGBA  s16  s16  s16  s16
    TEXTURE_FORMAT_SRGB8,        /// RGB   8  8  8
    TEXTURE_FORMAT_SRGB8_ALPHA8, /// RGBA  8  8  8  8

    //
    // Half-float
    //

    TEXTURE_FORMAT_R16F,    /// RED   f16
    TEXTURE_FORMAT_RG16F,   /// RG    f16  f16
    TEXTURE_FORMAT_RGB16F,  /// RGB   f16  f16  f16
    TEXTURE_FORMAT_RGBA16F, /// RGBA  f16  f16  f16  f16

    //
    // Float
    //

    TEXTURE_FORMAT_R32F,           /// RED   f32
    TEXTURE_FORMAT_RG32F,          /// RG    f32  f32
    TEXTURE_FORMAT_RGB32F,         /// RGB   f32  f32  f32
    TEXTURE_FORMAT_RGBA32F,        /// RGBA  f32  f32  f32  f32
    TEXTURE_FORMAT_R11F_G11F_B10F, /// RGB   f11  f11  f10

    // Shared exponent
    TEXTURE_FORMAT_RGB9_E5, /// RGB   9  9  9     5

    //
    // Integer formats
    //

    TEXTURE_FORMAT_R8I,      /// RED   i8
    TEXTURE_FORMAT_R8UI,     /// RED   ui8
    TEXTURE_FORMAT_R16I,     /// RED   i16
    TEXTURE_FORMAT_R16UI,    /// RED   ui16
    TEXTURE_FORMAT_R32I,     /// RED   i32
    TEXTURE_FORMAT_R32UI,    /// RED   ui32
    TEXTURE_FORMAT_RG8I,     /// RG    i8   i8
    TEXTURE_FORMAT_RG8UI,    /// RG    ui8   ui8
    TEXTURE_FORMAT_RG16I,    /// RG    i16   i16
    TEXTURE_FORMAT_RG16UI,   /// RG    ui16  ui16
    TEXTURE_FORMAT_RG32I,    /// RG    i32   i32
    TEXTURE_FORMAT_RG32UI,   /// RG    ui32  ui32
    TEXTURE_FORMAT_RGB8I,    /// RGB   i8   i8   i8
    TEXTURE_FORMAT_RGB8UI,   /// RGB   ui8  ui8  ui8
    TEXTURE_FORMAT_RGB16I,   /// RGB   i16  i16  i16
    TEXTURE_FORMAT_RGB16UI,  /// RGB   ui16 ui16 ui16
    TEXTURE_FORMAT_RGB32I,   /// RGB   i32  i32  i32
    TEXTURE_FORMAT_RGB32UI,  /// RGB   ui32 ui32 ui32
    TEXTURE_FORMAT_RGBA8I,   /// RGBA  i8   i8   i8   i8
    TEXTURE_FORMAT_RGBA8UI,  /// RGBA  ui8  ui8  ui8  ui8
    TEXTURE_FORMAT_RGBA16I,  /// RGBA  i16  i16  i16  i16
    TEXTURE_FORMAT_RGBA16UI, /// RGBA  ui16 ui16 ui16 ui16
    TEXTURE_FORMAT_RGBA32I,  /// RGBA  i32  i32  i32  i32
    TEXTURE_FORMAT_RGBA32UI, /// RGBA  ui32 ui32 ui32 ui32

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

AN_INLINE bool IsCompressedFormat(TEXTURE_FORMAT Format)
{
    // TODO: lookup table
    switch (Format)
    {
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

AN_INLINE bool IsDepthStencilFormat(TEXTURE_FORMAT Format)
{
    // TODO: lookup table
    switch (Format)
    {
        case TEXTURE_FORMAT_STENCIL1:
        case TEXTURE_FORMAT_STENCIL4:
        case TEXTURE_FORMAT_STENCIL8:
        case TEXTURE_FORMAT_STENCIL16:
        case TEXTURE_FORMAT_DEPTH16:
        case TEXTURE_FORMAT_DEPTH24:
        case TEXTURE_FORMAT_DEPTH32:
        case TEXTURE_FORMAT_DEPTH24_STENCIL8:
        case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
            return true;
        default:;
    }
    return false;
}

enum TEXTURE_VIEW : uint8_t
{
    TEXTURE_VIEW_UNDEFINED = 0,
    TEXTURE_VIEW_SHADER_RESOURCE,
    TEXTURE_VIEW_RENDER_TARGET,
    TEXTURE_VIEW_DEPTH_STENCIL,
    TEXTURE_VIEW_UNORDERED_ACCESS
};

struct STextureViewDesc
{
    TEXTURE_VIEW   ViewType = TEXTURE_VIEW_UNDEFINED;
    TEXTURE_TYPE   Type     = TEXTURE_2D;
    TEXTURE_FORMAT Format   = TEXTURE_FORMAT_RGBA8;

    uint16_t FirstMipLevel = 0;
    uint16_t NumMipLevels  = 0;

    /** Slice is an array layer or depth for a 3D texture. Cubemap has 6 slices, cubemap array has num_layers * 6 slices. */
    uint16_t FirstSlice = 0;
    uint16_t NumSlices  = 0;

    bool operator==(STextureViewDesc const& Rhs) const
    {
        // clang-format off
        return ViewType      == Rhs.ViewType &&
               Type          == Rhs.Type &&
               Format        == Rhs.Format &&
               FirstMipLevel == Rhs.FirstMipLevel &&
               NumMipLevels  == Rhs.NumMipLevels &&
               FirstSlice    == Rhs.FirstSlice &&
               NumSlices     == Rhs.NumSlices;
        // clang-format on
    }

    bool operator!=(STextureViewDesc const& Rhs) const
    {
        return !(operator==(Rhs));
    }
};

class ITextureView : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_TEXTURE_VIEW;

    ITextureView(STextureViewDesc const& TextureViewDesc, ITexture* pTexture);

    STextureViewDesc const& GetDesc() const { return Desc; }

    ITexture* GetTexture() { return pTexture; }

    uint32_t GetWidth() const;

    uint32_t GetHeight() const;

private:
    STextureViewDesc Desc;
    ITexture*        pTexture;
};

} // namespace RenderCore

namespace HashTraits
{

AN_FORCEINLINE std::size_t Hash(RenderCore::STextureViewDesc const& Desc)
{
    return Core::SDBMHash(reinterpret_cast<const char*>(&Desc), sizeof(Desc));
}

} // namespace HashTraits
