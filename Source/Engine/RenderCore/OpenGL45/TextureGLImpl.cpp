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

#include "TextureGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/CoreMath.h>
#include <Core/Public/Logger.h>

namespace RenderCore {

static size_t CalcTextureRequiredMemory() {
    // TODO: calculate!
    return 0;
}

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

static void FixSamplesCount( TEXTURE_TYPE InType, STextureMultisampleInfo const & InMultisampleInfo, uint8_t & NumSamples, bool & bFixedSampleLocations )
{
    NumSamples = 1;
    bFixedSampleLocations = false;

    if ( InMultisampleInfo.NumSamples > 1 ) {
        switch ( InType ) {
        case TEXTURE_2D:
        case TEXTURE_2D_ARRAY:
            NumSamples = InMultisampleInfo.NumSamples;
            bFixedSampleLocations = InMultisampleInfo.bFixedSampleLocations;
            break;
        default:
            GLogger.Printf( "Multisample allowed only for 2D and 2DArray textures\n" );
            break;
        }
    }
}

static void GetTextureTypeGL( TEXTURE_TYPE InType, uint8_t InNumSamples, GLenum & TypeGL, GLenum & BindingGL )
{
    TypeGL = TextureTargetLUT[ InType ].Target;
    BindingGL = TextureTargetLUT[ InType ].Binding;

    if ( InNumSamples > 1 ) {
        switch ( TypeGL ) {
        case GL_TEXTURE_2D:
            TypeGL = GL_TEXTURE_2D_MULTISAMPLE;
            BindingGL = GL_TEXTURE_BINDING_2D_MULTISAMPLE;
            break;
        case GL_TEXTURE_2D_ARRAY:
            TypeGL = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
            BindingGL = GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
            break;
        }
    }
}

static void GetTextureTypeGL( TEXTURE_TYPE InType, uint8_t InNumSamples, GLenum & TypeGL )
{
    TypeGL = TextureTargetLUT[ InType ].Target;

    if ( InNumSamples > 1 ) {
        switch ( TypeGL ) {
        case GL_TEXTURE_2D:
            TypeGL = GL_TEXTURE_2D_MULTISAMPLE;
            break;
        case GL_TEXTURE_2D_ARRAY:
            TypeGL = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
            break;
        }
    }
}

#if 0
ATextureGLImpl::ATextureGLImpl( ADeviceGLImpl * _Device, STextureCreateInfo const & _CreateInfo, STextureInitialData const * _InitialData )
    : pDevice( _Device )
{
    GLuint id;
    GLenum target;

    Type = _CreateInfo.Type;
    Format = _CreateInfo.InternalFormat;
    Resolution = _CreateInfo.Resolution;
    Swizzle = _CreateInfo.Swizzle;
    StorageNumLods = 0;

    FixSamplesCount( Type, _CreateInfo.Multisample, NumSamples, bFixedSampleLocations );

    GetTextureTypeGL( Type, NumSamples, target );

    glCreateTextures( target, 1, &id ); // 4.5

    pDevice->TotalTextures++;
    pDevice->TextureMemoryAllocated += CalcTextureRequiredMemory();
    Handle = ( void * )( size_t )id;
    bImmutableStorage = false;
    bTextureView = false;
    bCompressed = IsCompressedFormat( Format );

    SetSwizzleParams( id, Swizzle );

    ATextureGLImpl::CreateLod( 0, _InitialData );
}
#endif

ATextureGLImpl::ATextureGLImpl( ADeviceGLImpl * _Device, STextureCreateInfo const & _CreateInfo )
    : ITexture( _Device ), pDevice( _Device )
{
    GLuint id;
    GLenum target;
    GLenum internalFormat = InternalFormatLUT[ _CreateInfo.Format ].InternalFormat;

    AN_ASSERT( _CreateInfo.NumLods > 0 );

    Type = _CreateInfo.Type;
    Format = _CreateInfo.Format;
    Resolution = _CreateInfo.Resolution;
    Swizzle = _CreateInfo.Swizzle;
    NumLods = _CreateInfo.NumLods;

    FixSamplesCount( Type, _CreateInfo.Multisample, NumSamples, bFixedSampleLocations );

    GetTextureTypeGL( Type, NumSamples, target );

    glCreateTextures( target, 1, &id );

    SetSwizzleParams( id, Swizzle );

    //glTextureParameterf( id, GL_TEXTURE_BASE_LEVEL, 0 );
    //glTextureParameterf( id, GL_TEXTURE_MAX_LEVEL, StorageNumLods - 1 );

    switch ( Type ) {
    case TEXTURE_1D:
        glTextureStorage1D( id, NumLods, internalFormat, Resolution.Tex1D.Width );
        break;
    case TEXTURE_1D_ARRAY:
        glTextureStorage2D( id, NumLods, internalFormat, Resolution.Tex1DArray.Width, Resolution.Tex1DArray.NumLayers );
        break;
    case TEXTURE_2D:
        if ( NumSamples > 1 ) {
            glTextureStorage2DMultisample( id, NumSamples, internalFormat, Resolution.Tex2D.Width, Resolution.Tex2D.Height, bFixedSampleLocations );
        } else {
            glTextureStorage2D( id, NumLods, internalFormat, Resolution.Tex2D.Width, Resolution.Tex2D.Height );
        }
        break;
    case TEXTURE_2D_ARRAY:
        if ( NumSamples > 1 ) {
            glTextureStorage3DMultisample( id, NumSamples, internalFormat, Resolution.Tex2DArray.Width, Resolution.Tex2DArray.Height, Resolution.Tex2DArray.NumLayers, bFixedSampleLocations );
        } else {
            glTextureStorage3D( id, NumLods, internalFormat, Resolution.Tex2DArray.Width, Resolution.Tex2DArray.Height, Resolution.Tex2DArray.NumLayers );
        }
        break;
    case TEXTURE_3D:
        glTextureStorage3D( id, NumLods, internalFormat, Resolution.Tex3D.Width, Resolution.Tex3D.Height, Resolution.Tex3D.Depth );
        break;
    case TEXTURE_CUBE_MAP:
        glTextureStorage2D( id, NumLods, internalFormat, Resolution.TexCubemap.Width, Resolution.TexCubemap.Width );
        break;
    case TEXTURE_CUBE_MAP_ARRAY:
        glTextureStorage3D( id, NumLods, internalFormat, Resolution.TexCubemapArray.Width, Resolution.TexCubemapArray.Width, Resolution.TexCubemapArray.NumLayers * 6 );
        break;
    case TEXTURE_RECT_GL:
        glTextureStorage2D( id, NumLods, internalFormat, Resolution.TexRect.Width, Resolution.TexRect.Height );
        break;
    }

    //glTextureParameteri( id, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    //glTextureParameteri( id, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    //glTextureParameteri( id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    //glTextureParameteri( id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    //glTextureParameteri( id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    //glTextureParameterf( id, GL_TEXTURE_LOD_BIAS, 0.0f );
    //glTextureParameterf( id, GL_TEXTURE_MIN_LOD, 0 );
    //glTextureParameterf( id, GL_TEXTURE_MAX_LOD, StorageNumLods - 1 );
    //glTextureParameteri( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.0f );
    //glTextureParameteri( id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    //glTextureParameteri( id, GL_TEXTURE_COMPARE_FUNC, ComparisonFunc );
    //glTextureParameterfv( id, GL_TEXTURE_BORDER_COLOR, BorderColor );
    //glTextureParameteri( id, GL_TEXTURE_CUBE_MAP_SEAMLESS, bCubemapSeamless );

    pDevice->TotalTextures++;
    pDevice->TextureMemoryAllocated += CalcTextureRequiredMemory();

    bTextureView = false;
    bCompressed = IsCompressedFormat( Format );

    SetHandleNativeGL( id );
}

ATextureGLImpl::ATextureGLImpl( ADeviceGLImpl * _Device, STextureViewCreateInfo const & _CreateInfo )
    : ITexture( _Device ), pDevice( _Device )
{
    GLuint id;
    GLenum target = TextureTargetLUT[ _CreateInfo.Type ].Target;
    GLint internalFormat = InternalFormatLUT[ _CreateInfo.Format ].InternalFormat;

    ITexture * originalTex = _CreateInfo.pOriginalTexture;

    // From OpenGL specification:
    bool bCompatible = false;
    switch ( originalTex->GetType() ) {
    case TEXTURE_1D:
    case TEXTURE_1D_ARRAY:
        bCompatible = ( target == GL_TEXTURE_1D || target == GL_TEXTURE_1D_ARRAY );
        break;
    case TEXTURE_2D:
    case TEXTURE_2D_ARRAY:
        bCompatible = ( target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_ARRAY )
                && originalTex->IsMultisample() == _CreateInfo.bMultisample;
        break;
    case TEXTURE_3D:
        bCompatible = ( target == GL_TEXTURE_3D );
        break;
    case TEXTURE_CUBE_MAP:
    case TEXTURE_CUBE_MAP_ARRAY:
        bCompatible = ( target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY );
        break;
    case TEXTURE_RECT_GL:
        bCompatible = ( target == GL_TEXTURE_RECTANGLE );
        break;
    }

    if ( !bCompatible ) {
        GLogger.Printf( "ATextureGLImpl::ctor: failed to initialize texture view, incompatible texture types\n" );
        return;
    }

    if ( _CreateInfo.bMultisample ) {
        if ( target == GL_TEXTURE_2D )
            target = GL_TEXTURE_2D_MULTISAMPLE;
        if ( target == GL_TEXTURE_2D_ARRAY )
            target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    }

    // TODO: Check internal formats compatibility
    // TODO: Check texture resolution compatibility
    // TODO: Lods/Layers compatibility

    // Avoid previous errors if any
    (void)glGetError();

    glGenTextures( 1, &id ); // 4.5

    // 4.3
    glTextureView( id, target, originalTex->GetHandleNativeGL(),
                   internalFormat, _CreateInfo.MinLod, _CreateInfo.NumLods, _CreateInfo.MinLayer, _CreateInfo.NumLayers );

    if ( glGetError() != GL_NO_ERROR ) {
        // Incompatible texture formats (See OpenGL specification for details)
        if ( glIsTexture( id ) ) {
            glDeleteTextures( 1, &id );
        }
        GLogger.Printf( "ATextureGLImpl::ctor: failed to initialize texture view, incompatible texture formats\n" );
        return;
    }

    pDevice->TotalTextures++;

    SetHandleNativeGL( id );

    Type = _CreateInfo.Type;
    Format = _CreateInfo.Format;
    Resolution = originalTex->GetResolution();
    Swizzle = originalTex->GetSwizzle();
    NumSamples = originalTex->GetNumSamples();
    bFixedSampleLocations = originalTex->FixedSampleLocations();
    NumLods = _CreateInfo.NumLods;
    bTextureView = true;
    bCompressed = IsCompressedFormat( _CreateInfo.Format );
    pOriginalTex = originalTex;
}

ATextureGLImpl::~ATextureGLImpl() {
    GLuint id = GetHandleNativeGL();

    if ( !id ) {
        return;
    }

    glDeleteTextures( 1, &id );
    pDevice->TotalTextures--;

    if ( !bTextureView ) {
        pDevice->TextureMemoryAllocated -= CalcTextureRequiredMemory();
    }
}

#if 0
bool ATextureGLImpl::CreateLod( uint16_t InLod, STextureInitialData const * InInitialData )
{
    struct TableMagicTextureFormat {
        GLint   InternalFormat;
        GLenum  Format;
        GLenum  PixelType;
        int     SizeOf;
    };

    constexpr TableMagicTextureFormat MagicTextureFormatLUT[] = {
        { GL_STENCIL_INDEX1,    GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
        { GL_STENCIL_INDEX4,    GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
        { GL_STENCIL_INDEX8,    GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
        { GL_STENCIL_INDEX16,   GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
        { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
        { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
        { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
        { GL_DEPTH24_STENCIL8,  GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8, 4 },
        { GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8 } // FIXME: SizeOf 8 or 4? // 3.0 or GL_ARB_depth_buffer_float
    };

    GLenum target, binding, format, pixelType;
    GLint bindTex, internalFormat;
    bool bCompressedData;
    unsigned int alignment;
    const void * sysMem;
    GLsizei compressedDataSizeInBytes;
    uint16_t arrayLength;
    uint32_t lodWidth, lodHeight, lodDepth;

    if ( bImmutableStorage ) {
        GLogger.Printf( "ATextureGLImpl::CreateLod: not allowed for immutable storage\n" );
        return false;
    }

    if ( bTextureView ) {
        GLogger.Printf( "ATextureGLImpl::CreateLod: not allowed for texture views\n" );
        return false;
    }

    GetTextureTypeGL( Type, NumSamples, target, binding );

    glGetIntegerv( binding, &bindTex );
    glBindTexture( target, GetHandleNativeGL() );

    // TODO: must be: arrayLength < GL_MAX_ARRAY_TEXTURE_LAYERS for texture arrays
    if ( Type == TEXTURE_1D_ARRAY ) {
        arrayLength = ( uint16_t )std::max( 1u, Resolution.Tex1DArray.NumLayers );
    } else if ( Type == TEXTURE_2D_ARRAY ) {
        arrayLength = ( uint16_t )std::max( 1u, Resolution.Tex2DArray.NumLayers );
    } else if ( Type == TEXTURE_CUBE_MAP_ARRAY ) {
        arrayLength = ( uint16_t )std::max( 1u, Resolution.TexCubemapArray.NumLayers );
    } else {
        arrayLength = 1;
    }

    internalFormat = InternalFormatLUT[ InternalFormat ].InternalFormat;

    // TODO: NumSamples must be <= GL_MAX_DEPTH_TEXTURE_SAMPLES for depth/stencil integer internal format
    //                          <= GL_MAX_COLOR_TEXTURE_SAMPLES for color integer internal format
    //                          <= GL_MAX_INTEGER_SAMPLES for signed/unsigned integer internal format
    //                          <= GL_MAX_SAMPLES-1 for all formats

    lodWidth = std::max( 1u, Resolution.Tex3D.Width >> InLod );
    lodHeight = std::max( 1u, Resolution.Tex3D.Height >> InLod );
    lodDepth = std::max( 1u, Resolution.Tex3D.Depth >> InLod );

    // TODO: lodWidth and lodHeight​ must be <= GL_MAX_TEXTURE_SIZE

    if ( InInitialData ) {
        format = TexturePixelFormatLUT[ InInitialData->PixelFormat ].Format;
        pixelType = TexturePixelFormatLUT[ InInitialData->PixelFormat ].PixelType;
        bCompressedData = ( pixelType == 0 ); // Pixel type is 0 for compressed input data
        alignment = std::max( 1u, InInitialData->Alignment );
        sysMem = InInitialData->SysMem;
        compressedDataSizeInBytes = (GLsizei)InInitialData->SizeInBytes;
    } else {
        format = InternalFormatLUT[ InternalFormat ].Format;
        int i = InternalFormat - INTERNAL_PIXEL_FORMAT_STENCIL1;
        if ( i >= 0 ) {
            pixelType = MagicTextureFormatLUT[ i ].PixelType;
        } else {
            pixelType = GL_UNSIGNED_BYTE;
        }
        bCompressedData = false;
        alignment = 1;
        sysMem = NULL;
        compressedDataSizeInBytes = 0;
    }

    if ( sysMem ) {
        AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

        ctx->UnpackAlignment( alignment );
    }

    switch ( Type ) {
    case TEXTURE_1D:
        if ( bCompressedData ) {
            glCompressedTexImage1D( target, 0, format, lodWidth, 0, compressedDataSizeInBytes, sysMem );
        } else {
            glTexImage1D( target, 0, internalFormat, lodWidth, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_1D_ARRAY:
        if ( bCompressedData ) {
            glCompressedTexImage2D( target, 0, format, lodWidth, arrayLength, 0, compressedDataSizeInBytes, sysMem );
        } else {
            glTexImage2D( target, 0, internalFormat, lodWidth, arrayLength, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_2D:
        if ( NumSamples > 1 ) {
            if ( InLod == 0 ) {
                glTexImage2DMultisample( target, NumSamples, internalFormat, lodWidth, lodHeight, bFixedSampleLocations ); // 3.2
            }
        } else {
            if ( bCompressedData ) {
                glCompressedTexImage2D( target, 0, format, lodWidth, lodHeight, 0, compressedDataSizeInBytes, sysMem );
            } else {
                glTexImage2D( target, 0, internalFormat, lodWidth, lodHeight, 0, format, pixelType, sysMem );
            }
        }
        break;
    case TEXTURE_3D:
        // Avoid previous errors if any
        (void)glGetError();
        if ( bCompressedData ) {
            glCompressedTexImage3D( target, 0, format, lodWidth, lodHeight, lodDepth, 0, compressedDataSizeInBytes, sysMem );
        } else {
            glTexImage3D( target, 0, internalFormat, lodWidth, lodHeight, lodDepth, 0, format, pixelType, sysMem );
        }
        // FIXME: wtf this error?
        {
            int error = glGetError();
            GLogger.Printf( "Error %d\n", error );
        }
        break;
    case TEXTURE_2D_ARRAY:
        if ( NumSamples > 1 ) {
            if ( InLod == 0 ) {
                glTexImage3DMultisample( target, NumSamples, internalFormat, lodWidth, lodHeight, arrayLength, bFixedSampleLocations ); // 3.2
            }
        } else {
            if ( bCompressedData ) {
                glCompressedTexImage3D( target, 0, format, lodWidth, lodHeight, arrayLength, 0, compressedDataSizeInBytes, sysMem );
            } else {
                glTexImage3D( target, 0, internalFormat, lodWidth, lodHeight, arrayLength, 0, format, pixelType, sysMem );
            }
        }
        break;
    case TEXTURE_CUBE_MAP:
        if ( bCompressedData ) {
            // TODO: check this:
            for ( int face = 0; face < 6; face++ ) {
                if ( sysMem ) {
                    glCompressedTexImage3D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, format,
                        lodWidth, lodWidth, 1, 0, compressedDataSizeInBytes, ( const uint8_t * )( sysMem ) + face * InInitialData->SizeInBytes );

                } else {
                    glCompressedTexImage3D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, format,
                        lodWidth, lodWidth, 1, 0, compressedDataSizeInBytes, NULL );
                }
            }
        } else {
            // TODO: check this:
            int sizeOfPixel = InInitialData ? TexturePixelFormatLUT[ InInitialData->PixelFormat ].SizeOf : 0;
            int sizeOfFace = lodWidth * lodWidth * sizeOfPixel;
            for ( int face = 0; face < 6; face++ ) {
                if ( sysMem ) {
                    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, internalFormat,
                        lodWidth, lodWidth, 0, format, pixelType, ( const uint8_t * )( sysMem ) + face * sizeOfFace );
                } else {
                    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, internalFormat,
                        lodWidth, lodWidth, 0, format, pixelType, NULL );
                }
            }
        }
        break;
    case TEXTURE_CUBE_MAP_ARRAY:
        // TODO: check this:
        // FIXME: mul 6
        if ( bCompressedData ) {
            glCompressedTexImage3D( target, 0, format, lodWidth, lodWidth, arrayLength * 6, 0, compressedDataSizeInBytes, sysMem );
        } else {
            glTexImage3D( target, 0, internalFormat, lodWidth, lodWidth, arrayLength * 6, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_RECT_GL:
        glTexImage2D( target, 0, internalFormat, lodWidth, lodHeight, 0, format, pixelType, sysMem );
        break;
    }

    glBindTexture( target, bindTex );

    return true;
}
#endif

void ATextureGLImpl::GenerateLods() {
    GLuint id = GetHandleNativeGL();

    if ( !id ) {
        return;
    }

    glGenerateTextureMipmap( id );
}

void ATextureGLImpl::GetLodInfo( uint16_t _Lod, STextureLodInfo * _Info ) const {
    GLuint id = GetHandleNativeGL();
    int width, height, depth, tmp;

    memset( _Info, 0, sizeof( STextureLodInfo ) );

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_WIDTH, &width );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_HEIGHT, &height );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_DEPTH, &depth );

    switch ( Type ) {
    case TEXTURE_1D:
        _Info->Resoultion.Tex1D.Width = width;
        break;
    case TEXTURE_1D_ARRAY:
        _Info->Resoultion.Tex1DArray.Width = width;
        _Info->Resoultion.Tex1DArray.NumLayers = height;
        break;
    case TEXTURE_2D:
        _Info->Resoultion.Tex2D.Width = width;
        _Info->Resoultion.Tex2D.Height = height;
        break;
    case TEXTURE_2D_ARRAY:
        _Info->Resoultion.Tex2DArray.Width = width;
        _Info->Resoultion.Tex2DArray.Height = height;
        _Info->Resoultion.Tex2DArray.NumLayers = depth;
        break;
    case TEXTURE_3D:
        _Info->Resoultion.Tex3D.Width = width;
        _Info->Resoultion.Tex3D.Height = height;
        _Info->Resoultion.Tex3D.Depth = depth;
        break;
    case TEXTURE_CUBE_MAP:
        _Info->Resoultion.TexCubemap.Width = width;
        break;
    case TEXTURE_CUBE_MAP_ARRAY:
        _Info->Resoultion.TexCubemapArray.Width = width;
        _Info->Resoultion.TexCubemapArray.NumLayers = depth / 6; // FIXME: is this right? maybe depth ? Check this
        break;
    case TEXTURE_RECT_GL:
        _Info->Resoultion.TexRect.Width = width;
        _Info->Resoultion.TexRect.Height = height;
        break;
    }

    _Info->bCompressed = bCompressed;

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &tmp );
    _Info->CompressedDataSizeInBytes = tmp;

    //GL_TEXTURE_INTERNAL_FORMAT
    //params returns a single value, the internal format of the texture image.

    //GL_TEXTURE_RED_TYPE, GL_TEXTURE_GREEN_TYPE, GL_TEXTURE_BLUE_TYPE, GL_TEXTURE_ALPHA_TYPE, GL_TEXTURE_DEPTH_TYPE
    //The data type used to store the component. The types GL_NONE, GL_SIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_FLOAT, GL_INT, and GL_UNSIGNED_INT may be returned to indicate signed normalized fixed-point, unsigned normalized fixed-point, floating-point, integer unnormalized, and unsigned integer unnormalized components, respectively.

    //GL_TEXTURE_RED_SIZE, GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE, GL_TEXTURE_ALPHA_SIZE, GL_TEXTURE_DEPTH_SIZE
    //The internal storage resolution of an individual component. The resolution chosen by the GL will be a close match for the resolution requested by the user with the component argument of glTexImage1D, glTexImage2D, glTexImage3D, glCopyTexImage1D, and glCopyTexImage2D. The initial value is 0.
}

void ATextureGLImpl::Read( uint16_t _Lod,
                           DATA_FORMAT _Format, // Specifies a pixel format for the returned data
                           size_t _SizeInBytes,
                           unsigned int _Alignment,               // Specifies alignment of destination data
                           void * _SysMem ) {
    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    GLuint id = GetHandleNativeGL();

    ctx->PackAlignment( _Alignment );

    if ( bCompressed ) {
        glGetCompressedTextureImage( id, _Lod, (GLsizei)_SizeInBytes, _SysMem );
    } else {
        glGetTextureImage( id,
                           _Lod,
                           TypeLUT[_Format].FormatBGR,
                           TypeLUT[_Format].Type,
                           (GLsizei)_SizeInBytes,
                           _SysMem );
    }
}

void ATextureGLImpl::ReadRect( STextureRect const & _Rectangle,
                               DATA_FORMAT _Format, // Specifies a pixel format for the returned data
                               size_t _SizeInBytes,
                               unsigned int _Alignment,               // Specifies alignment of destination data
                               void * _SysMem ) {
    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    GLuint id = GetHandleNativeGL();

    ctx->PackAlignment( _Alignment );

    if ( bCompressed ) {
        glGetCompressedTextureSubImage( id,
                                        _Rectangle.Offset.Lod,
                                        _Rectangle.Offset.X,
                                        _Rectangle.Offset.Y,
                                        _Rectangle.Offset.Z,
                                        _Rectangle.Dimension.X,
                                        _Rectangle.Dimension.Y,
                                        _Rectangle.Dimension.Z,
                                        (GLsizei)_SizeInBytes,
                                        _SysMem );
    } else {
        glGetTextureSubImage( id,
                              _Rectangle.Offset.Lod,
                              _Rectangle.Offset.X,
                              _Rectangle.Offset.Y,
                              _Rectangle.Offset.Z,
                              _Rectangle.Dimension.X,
                              _Rectangle.Dimension.Y,
                              _Rectangle.Dimension.Z,
                              TypeLUT[_Format].FormatBGR,
                              TypeLUT[_Format].Type,
                              (GLsizei)_SizeInBytes,
                              _SysMem );
    }
}

bool ATextureGLImpl::Write( uint16_t _Lod,
                            DATA_FORMAT _Type, // Specifies a pixel format for the input data
                            size_t _SizeInBytes,
                            unsigned int _Alignment,               // Specifies alignment of source data
                            const void * _SysMem ) {
    GLuint id = GetHandleNativeGL();

    int dimensionX, dimensionY, dimensionZ;

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_WIDTH, &dimensionX );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_HEIGHT, &dimensionY );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_DEPTH, &dimensionZ );

    STextureRect rect;
    rect.Offset.Lod = _Lod;
    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Dimension.X = (uint16_t)dimensionX;
    rect.Dimension.Y = (uint16_t)dimensionY;
    rect.Dimension.Z = (uint16_t)dimensionZ;

    return WriteRect( rect,
                      _Type,
                      _SizeInBytes,
                      _Alignment,
                      _SysMem );
}

bool ATextureGLImpl::WriteRect( STextureRect const & _Rectangle,
                                DATA_FORMAT _Format, // Specifies a pixel format for the input data
                                size_t _SizeInBytes,
                                unsigned int _Alignment,               // Specifies alignment of source data
                                const void * _SysMem ) {
    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    GLuint id = GetHandleNativeGL();
    GLenum compressedFormat = InternalFormatLUT[Format].InternalFormat;
    GLenum format = TypeLUT[_Format].FormatBGR;
    GLenum type = TypeLUT[_Format].Type;

    ctx->UnpackAlignment( _Alignment );

    switch ( Type ) {
    case TEXTURE_1D:
        if ( bCompressed ) {
            glCompressedTextureSubImage1D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Dimension.X,
                                           compressedFormat,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            glTextureSubImage1D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Dimension.X,
                                 format,
                                 type,
                                 _SysMem );
        }
        break;
    case TEXTURE_1D_ARRAY:
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
    case TEXTURE_2D:
        if ( IsMultisample() ) {
            return false;
        }
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
    case TEXTURE_2D_ARRAY:
        if ( IsMultisample() ) {
            return false;
        }
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
    case TEXTURE_3D:
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
    case TEXTURE_CUBE_MAP:
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
    case TEXTURE_CUBE_MAP_ARRAY:
        // FIXME: В спецификации ничего не сказано о возможности записи в данный тип текстурного таргета
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
    case TEXTURE_RECT_GL:
        // FIXME: В спецификации ничего не сказано о возможности записи в данный тип текстурного таргета
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

    return true;
}

void ATextureGLImpl::Invalidate( uint16_t _Lod ) {
    glInvalidateTexImage( GetHandleNativeGL(), _Lod );
}

void ATextureGLImpl::InvalidateRect( uint32_t _NumRectangles, STextureRect const * _Rectangles ) {
    GLuint id = GetHandleNativeGL();

    for ( STextureRect const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
        glInvalidateTexSubImage( id,
                                 rect->Offset.Lod,
                                 rect->Offset.X,
                                 rect->Offset.Y,
                                 rect->Offset.Z,
                                 rect->Dimension.X,
                                 rect->Dimension.Y,
                                 rect->Dimension.Z );
    }
}

int ITexture::CalcMaxLods( TEXTURE_TYPE _Type, STextureResolution const & _Resolution ) {
    int maxLods = 0;
    uint32_t maxDimension = 0;

    switch ( _Type ) {
    case TEXTURE_1D:
        maxLods = _Resolution.Tex1D.Width > 0 ? Math::Log2( _Resolution.Tex1D.Width ) + 1 : 0;
        break;
    case TEXTURE_1D_ARRAY:
        maxLods = _Resolution.Tex1DArray.Width > 0 ? Math::Log2( _Resolution.Tex1DArray.Width ) + 1 : 0;
        break;

    case TEXTURE_2D:
        maxDimension = std::max( _Resolution.Tex2D.Width, _Resolution.Tex2D.Height );
        maxLods = maxDimension > 0 ? Math::Log2( maxDimension ) + 1 : 0;
        break;

    case TEXTURE_2D_ARRAY:
        maxDimension = std::max( _Resolution.Tex2DArray.Width, _Resolution.Tex2DArray.Height );
        maxLods = maxDimension > 0 ? Math::Log2( maxDimension ) + 1 : 0;
        break;

    case TEXTURE_3D:
        maxDimension = Math::Max3( _Resolution.Tex3D.Width, _Resolution.Tex3D.Height, _Resolution.Tex3D.Depth );
        maxLods = maxDimension > 0 ? Math::Log2( maxDimension ) + 1 : 0;
        break;

    case TEXTURE_CUBE_MAP:
        maxLods = _Resolution.TexCubemap.Width > 0 ? Math::Log2( _Resolution.TexCubemap.Width ) + 1 : 0;
        break;

    case TEXTURE_CUBE_MAP_ARRAY:
        maxLods = _Resolution.TexCubemapArray.Width > 0 ? Math::Log2( _Resolution.TexCubemapArray.Width ) + 1 : 0;
        break;

    case TEXTURE_RECT_GL:
        maxLods = 1; // texture rect does not support mipmapping
        break;
    }

    return maxLods;
}

#if 0
size_t ATextureGLImpl::GetPixelFormatSize( int _Width, int _Height, TEXTURE_PIXEL_FORMAT _PixelFormat ) {
    // FIXME: What about compressed texture? Create function QueryPixelFormatSize( int Width, int Height, TEXTURE_PIXEL_FORMAT PixelFormat ) ?

    bool bCompressed = TexturePixelFormatLUT[_PixelFormat].SizeOf == 0;

    if ( bCompressed ) {
        // TODO:
        //int blockSize = TexturePixelFormatLUT[_PixelFormat].BlockSize;
        //int blocksX = Align( _Width, blockSize ) / blockSize;
        //int blocksY = Align( _Height, blockSize ) / blockSize;
        //return blocksX * blocksY * TexturePixelFormatLUT[_PixelFormat].BlockSizeOf;
        AN_ASSERT(0);
    }

    return _Width * _Height * TexturePixelFormatLUT[_PixelFormat].SizeOf;
}
#endif

}
