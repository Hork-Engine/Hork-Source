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

#include "SparseTextureGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/CoreMath.h>
#include <Core/Public/Logger.h>

namespace RenderCore {

static void SetSwizzleParams( GLuint _Id, STextureSwizzle const & _Swizzle ) {
    if ( _Swizzle.R != TEXTURE_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_R, SwizzleLUT[_Swizzle.R] );
    }
    if ( _Swizzle.G != TEXTURE_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_G, SwizzleLUT[_Swizzle.G] );
    }
    if ( _Swizzle.B != TEXTURE_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_B, SwizzleLUT[_Swizzle.B] );
    }
    if ( _Swizzle.A != TEXTURE_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_A, SwizzleLUT[_Swizzle.A] );
    }
}

ASparseTextureGLImpl::ASparseTextureGLImpl( ADeviceGLImpl * _Device, SSparseTextureCreateInfo const & _CreateInfo )
    : ISparseTexture( _Device ), pDevice( _Device )
{
    GLuint id;
    GLenum target = SparseTextureTargetLUT[_CreateInfo.Type].Target;
    GLenum internalFormat = InternalFormatLUT[_CreateInfo.Format].InternalFormat;

    AN_ASSERT( _CreateInfo.NumLods > 0 );

    Type = _CreateInfo.Type;
    Format = _CreateInfo.Format;
    Resolution = _CreateInfo.Resolution;
    Swizzle = _CreateInfo.Swizzle;
    NumLods = _CreateInfo.NumLods;
    bCompressed = IsCompressedFormat( Format );

    int pageSizeIndex = 0;
    bool r = false;

    switch ( Type ) {
    case SPARSE_TEXTURE_2D:
        r = pDevice->ChooseAppropriateSparseTexturePageSize( Type, Format, Resolution.Tex2D.Width, Resolution.Tex2D.Height, 1, &pageSizeIndex, &PageSizeX, &PageSizeY, &PageSizeZ );
        break;
    case SPARSE_TEXTURE_2D_ARRAY:
        r = pDevice->ChooseAppropriateSparseTexturePageSize( Type, Format, Resolution.Tex2DArray.Width, Resolution.Tex2DArray.Height, 1, &pageSizeIndex, &PageSizeX, &PageSizeY, &PageSizeZ );
        break;
    case SPARSE_TEXTURE_3D:
        r = pDevice->ChooseAppropriateSparseTexturePageSize( Type, Format, Resolution.Tex3D.Width, Resolution.Tex3D.Height, Resolution.Tex3D.Depth, &pageSizeIndex, &PageSizeX, &PageSizeY, &PageSizeZ );
        break;
    case SPARSE_TEXTURE_CUBE_MAP:
        r = pDevice->ChooseAppropriateSparseTexturePageSize( Type, Format, Resolution.TexCubemap.Width, Resolution.TexCubemap.Width, 1, &pageSizeIndex, &PageSizeX, &PageSizeY, &PageSizeZ );
        break;
    case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
        r = pDevice->ChooseAppropriateSparseTexturePageSize( Type, Format, Resolution.TexCubemapArray.Width, Resolution.TexCubemapArray.Width, 1, &pageSizeIndex, &PageSizeX, &PageSizeY, &PageSizeZ );
        break;
    case SPARSE_TEXTURE_RECT_GL:
        r = pDevice->ChooseAppropriateSparseTexturePageSize( Type, Format, Resolution.TexRect.Width, Resolution.TexRect.Height, 1, &pageSizeIndex, &PageSizeX, &PageSizeY, &PageSizeZ );
        break;
    default:
        AN_ASSERT( 0 );
    }

    if ( !r ) {
        GLogger.Printf( "ASparseTextureGLImpl::ctor: failed to find appropriate sparse texture page size\n" );
        return;
    }

    glCreateTextures( target, 1, &id );
    glTextureParameteri( id, GL_TEXTURE_SPARSE_ARB, GL_TRUE );
    glTextureParameteri( id, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, pageSizeIndex );

    SetSwizzleParams( id, Swizzle );

    switch ( Type ) {
    case SPARSE_TEXTURE_2D:
        glTextureStorage2D( id, NumLods, internalFormat, Resolution.Tex2D.Width, Resolution.Tex2D.Height );
        break;
    case SPARSE_TEXTURE_2D_ARRAY:
        glTextureStorage3D( id, NumLods, internalFormat, Resolution.Tex2DArray.Width, Resolution.Tex2DArray.Height, Resolution.Tex2DArray.NumLayers );
        break;
    case SPARSE_TEXTURE_3D:
        glTextureStorage3D( id, NumLods, internalFormat, Resolution.Tex3D.Width, Resolution.Tex3D.Height, Resolution.Tex3D.Depth );
        break;
    case SPARSE_TEXTURE_CUBE_MAP:
        glTextureStorage2D( id, NumLods, internalFormat, Resolution.TexCubemap.Width, Resolution.TexCubemap.Width );
        break;
    case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
        glTextureStorage3D( id, NumLods, internalFormat, Resolution.TexCubemapArray.Width, Resolution.TexCubemapArray.Width, Resolution.TexCubemapArray.NumLayers * 6 );
        break;
    case SPARSE_TEXTURE_RECT_GL:
        glTextureStorage2D( id, NumLods, internalFormat, Resolution.TexRect.Width, Resolution.TexRect.Height );
        break;
    }

    pDevice->TotalTextures++;

    SetHandleNativeGL( id );
}

ASparseTextureGLImpl::~ASparseTextureGLImpl()
{
    GLuint id = GetHandleNativeGL();

    if ( !id ) {
        return;
    }

    glDeleteTextures( 1, &id );
    pDevice->TotalTextures--;
}

void ASparseTextureGLImpl::CommitPage( int InLod, int InPageX, int InPageY, int InPageZ,
                                       DATA_FORMAT _Format, // Specifies a pixel format for the input data
                                       size_t _SizeInBytes,
                                       unsigned int _Alignment,               // Specifies alignment of source data
                                       const void * _SysMem )
{
    STextureRect rect;

    rect.Offset.Lod = InLod;
    rect.Offset.X = InPageX * PageSizeX;
    rect.Offset.Y = InPageY * PageSizeY;
    rect.Offset.Z = InPageZ * PageSizeZ;
    rect.Dimension.X = PageSizeX;
    rect.Dimension.Y = PageSizeY;
    rect.Dimension.Z = PageSizeZ;

    CommitRect( rect, _Format, _SizeInBytes, _Alignment, _SysMem );
}

void ASparseTextureGLImpl::CommitRect( STextureRect const & _Rectangle,
                                       DATA_FORMAT _Format, // Specifies a pixel format for the input data
                                       size_t _SizeInBytes,
                                       unsigned int _Alignment,               // Specifies alignment of source data
                                       const void * _SysMem )
{
    GLuint id = GetHandleNativeGL();

    if ( !id ) {
        GLogger.Printf( "ASparseTextureGLImpl::CommitRect: null handle\n" );
        return;
    }

    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    GLenum compressedFormat = InternalFormatLUT[Format].InternalFormat;
    GLenum format = TypeLUT[_Format].FormatBGR;
    GLenum type = TypeLUT[_Format].Type;

    glTexturePageCommitmentEXT( id,
                                _Rectangle.Offset.Lod,
                                _Rectangle.Offset.X,
                                _Rectangle.Offset.Y,
                                _Rectangle.Offset.Z,
                                _Rectangle.Dimension.X,
                                _Rectangle.Dimension.Y,
                                _Rectangle.Dimension.Z,
                                GL_TRUE );

    ctx->UnpackAlignment( _Alignment );

    switch ( Type ) {
    case SPARSE_TEXTURE_2D:
        if ( bCompressed ) {
            glCompressedTextureSubImage2D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            glTextureSubImage2D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    case SPARSE_TEXTURE_2D_ARRAY:
        if ( bCompressed ) {
            glCompressedTextureSubImage3D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Offset.Z,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           _Rectangle.Dimension.Z,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            glTextureSubImage3D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Offset.Z,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 _Rectangle.Dimension.Z,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    case SPARSE_TEXTURE_3D:
        if ( bCompressed ) {
            glCompressedTextureSubImage3D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Offset.Z,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           _Rectangle.Dimension.Z,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            glTextureSubImage3D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Offset.Z,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 _Rectangle.Dimension.Z,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    case SPARSE_TEXTURE_CUBE_MAP:
        if ( bCompressed ) {
            //glCompressedTextureSubImage2D( id, _Rectangle.Offset.Lod, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, _SizeInBytes, _SysMem );

            // Tested on NVidia
            glCompressedTextureSubImage3D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Offset.Z,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           _Rectangle.Dimension.Z,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            //glTextureSubImage2D( id, _Rectangle.Offset.Lod, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, type, _SysMem );

            // Tested on NVidia
            glTextureSubImage3D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Offset.Z,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 _Rectangle.Dimension.Z,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
        // FIXME: specs
        if ( bCompressed ) {
            //glCompressedTextureSubImage2D( id, _Rectangle.Offset.Lod, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, _SizeInBytes, _SysMem );

            glCompressedTextureSubImage3D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Offset.Z,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           _Rectangle.Dimension.Z,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            //glTextureSubImage2D( id, _Rectangle.Offset.Lod, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, type, _SysMem );

            glTextureSubImage3D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Offset.Z,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 _Rectangle.Dimension.Z,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    case SPARSE_TEXTURE_RECT_GL:
        // FIXME: specs
        if ( bCompressed ) {
            glCompressedTextureSubImage2D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            glTextureSubImage2D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    }
}

void ASparseTextureGLImpl::UncommitPage( int InLod, int InPageX, int InPageY, int InPageZ )
{
    STextureRect rect;

    rect.Offset.Lod = InLod;
    rect.Offset.X = InPageX * PageSizeX;
    rect.Offset.Y = InPageY * PageSizeY;
    rect.Offset.Z = InPageZ * PageSizeZ;
    rect.Dimension.X = PageSizeX;
    rect.Dimension.Y = PageSizeY;
    rect.Dimension.Z = PageSizeZ;

    UncommitRect( rect );
}

void ASparseTextureGLImpl::UncommitRect( STextureRect const & _Rectangle )
{
    GLuint id = GetHandleNativeGL();

    if ( !id ) {
        GLogger.Printf( "ASparseTextureGLImpl::UncommitRect: null handle\n" );
        return;
    }

    glTexturePageCommitmentEXT( id,
                                _Rectangle.Offset.Lod,
                                _Rectangle.Offset.X,
                                _Rectangle.Offset.Y,
                                _Rectangle.Offset.Z,
                                _Rectangle.Dimension.X,
                                _Rectangle.Dimension.Y,
                                _Rectangle.Dimension.Z,
                                GL_FALSE );
}

}
