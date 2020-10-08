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

#pragma once

#include "Texture.h"

namespace RenderCore {

/// Sparse texture types
enum SPARSE_TEXTURE_TYPE : uint8_t
{
    SPARSE_TEXTURE_2D,
    SPARSE_TEXTURE_2D_ARRAY,
    SPARSE_TEXTURE_3D,
    SPARSE_TEXTURE_CUBE_MAP,
    SPARSE_TEXTURE_CUBE_MAP_ARRAY,

    SPARSE_TEXTURE_RECT_GL // Can be used only with OpenGL backend
};

struct SSparseTextureResolution
{
    union {
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

struct SSparseTextureCreateInfo
{
    SPARSE_TEXTURE_TYPE     Type;
    TEXTURE_FORMAT          Format;
    SSparseTextureResolution Resolution;
    STextureSwizzle         Swizzle;
    uint16_t                NumLods;

    SSparseTextureCreateInfo()
        : Type( SPARSE_TEXTURE_2D )
        , Format( TEXTURE_FORMAT_RGBA8 )
        , NumLods( 1 )
    {
    }
};

inline SSparseTextureCreateInfo MakeSparseTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution2D const &     Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    SSparseTextureCreateInfo createInfo;
    createInfo.Type = SPARSE_TEXTURE_2D;
    createInfo.Format = Format;
    createInfo.Resolution.Tex2D = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline SSparseTextureCreateInfo MakeSparseTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution2DArray const &Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    SSparseTextureCreateInfo createInfo;
    createInfo.Type = SPARSE_TEXTURE_2D_ARRAY;
    createInfo.Format = Format;
    createInfo.Resolution.Tex2DArray = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline SSparseTextureCreateInfo MakeSparseTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolution3D const &     Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    SSparseTextureCreateInfo createInfo;
    createInfo.Type = SPARSE_TEXTURE_3D;
    createInfo.Format = Format;
    createInfo.Resolution.Tex3D = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline SSparseTextureCreateInfo MakeSparseTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolutionCubemap const &Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    SSparseTextureCreateInfo createInfo;
    createInfo.Type = SPARSE_TEXTURE_CUBE_MAP;
    createInfo.Format = Format;
    createInfo.Resolution.TexCubemap = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline SSparseTextureCreateInfo MakeSparseTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolutionCubemapArray const &Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    SSparseTextureCreateInfo createInfo;
    createInfo.Type = SPARSE_TEXTURE_CUBE_MAP_ARRAY;
    createInfo.Format = Format;
    createInfo.Resolution.TexCubemapArray = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

inline SSparseTextureCreateInfo MakeSparseTexture(
    TEXTURE_FORMAT                   Format,
    STextureResolutionRectGL const & Resolution,
    STextureSwizzle const &          Swizzle = STextureSwizzle(),
    uint16_t                         NumLods = 1 )
{
    SSparseTextureCreateInfo createInfo;
    createInfo.Type = SPARSE_TEXTURE_RECT_GL;
    createInfo.Format = Format;
    createInfo.Resolution.TexRect = Resolution;
    createInfo.Swizzle = Swizzle;
    createInfo.NumLods = NumLods;
    return createInfo;
}

class ISparseTexture : public ITextureBase
{
public:
    ISparseTexture( IDevice * Device ) : ITextureBase( Device ) {}

    SPARSE_TEXTURE_TYPE GetType() const { return Type; }

    uint32_t GetWidth() const;

    uint32_t GetHeight() const;

    uint32_t GetNumLods() const { return NumLods; }

    TEXTURE_FORMAT GetFormat() const { return Format; }

    bool IsCompressed() const { return bCompressed; }

    SSparseTextureResolution const & GetResolution() const { return Resolution; }

    STextureSwizzle const & GetSwizzle() const { return Swizzle; }

    int GetPageSizeX() const { return PageSizeX; }
    int GetPageSizeY() const { return PageSizeY; }
    int GetPageSizeZ() const { return PageSizeZ; }

    virtual void CommitPage( int InLod, int InPageX, int InPageY, int InPageZ,
                             DATA_FORMAT _Format, // Specifies a pixel format for the input data
                             size_t _SizeInBytes,
                             unsigned int _Alignment,               // Specifies alignment of source data
                             const void * _SysMem ) = 0;

    virtual void CommitRect( STextureRect const & _Rectangle,
                             DATA_FORMAT _Format, // Specifies a pixel format for the input data
                             size_t _SizeInBytes,
                             unsigned int _Alignment,               // Specifies alignment of source data
                             const void * _SysMem ) = 0;

    virtual void UncommitPage( int InLod, int InPageX, int InPageY, int InPageZ ) = 0;

    virtual void UncommitRect( STextureRect const & _Rectangle ) = 0;

protected:
    int PageSizeX;
    int PageSizeY;
    int PageSizeZ;
    SPARSE_TEXTURE_TYPE Type;
    TEXTURE_FORMAT Format;
    SSparseTextureResolution Resolution;
    STextureSwizzle Swizzle;
    int NumLods;
    bool bCompressed;
};

AN_INLINE uint32_t ISparseTexture::GetWidth() const {
    switch ( Type ) {
    case SPARSE_TEXTURE_2D:
        return Resolution.Tex2D.Width;
    case SPARSE_TEXTURE_2D_ARRAY:
        return Resolution.Tex2DArray.Width;
    case SPARSE_TEXTURE_3D:
        return Resolution.Tex3D.Width;
    case SPARSE_TEXTURE_CUBE_MAP:
        return Resolution.TexCubemap.Width;
    case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
        return Resolution.TexCubemapArray.Width;
    case SPARSE_TEXTURE_RECT_GL:
        return Resolution.TexRect.Width;
    }
    AN_ASSERT( 0 );
    return 0;
}

AN_INLINE uint32_t ISparseTexture::GetHeight() const {
    switch ( Type ) {
    case SPARSE_TEXTURE_2D:
        return Resolution.Tex2D.Height;
    case SPARSE_TEXTURE_2D_ARRAY:
        return Resolution.Tex2DArray.Height;
    case SPARSE_TEXTURE_3D:
        return Resolution.Tex3D.Height;
    case SPARSE_TEXTURE_CUBE_MAP:
        return Resolution.TexCubemap.Width;
    case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
        return Resolution.TexCubemapArray.Width;
    case SPARSE_TEXTURE_RECT_GL:
        return Resolution.TexRect.Height;
    }
    AN_ASSERT( 0 );
    return 0;
}

}
