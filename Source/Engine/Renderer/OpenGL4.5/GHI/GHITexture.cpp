/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#include "GHITexture.h"
#include "GHIBuffer.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "LUT.h"

#include <algorithm>
#include <memory.h>
#include <assert.h>
#include "GL/glew.h"

namespace GHI {

template< typename T > T max3( T const & a, T const & b, T const & c ) {
    return std::max( std::max( a, b ), c );
}

Texture::Texture() {
    pDevice = nullptr;
    Handle = nullptr;
}

Texture::~Texture() {
    Deinitialize();
}

static size_t CalcTextureRequiredMemory( TextureCreateInfo const & _CreateInfo ) {
    // TODO: calculate!
    return 0;
}

static size_t CalcTextureRequiredMemory( TextureStorageCreateInfo const & _CreateInfo ) {
    // TODO: calculate!
    return 0;
}

static void SetSwizzleParams( GLuint _Id, TextureSwizzle const & _Swizzle ) {
    if ( _Swizzle.R != SAMPLER_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_R, SwizzleLUT[_Swizzle.R] );
    }
    if ( _Swizzle.G != SAMPLER_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_G, SwizzleLUT[_Swizzle.G] );
    }
    if ( _Swizzle.B != SAMPLER_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_B, SwizzleLUT[_Swizzle.B] );
    }
    if ( _Swizzle.A != SAMPLER_SWIZZLE_IDENTITY ) {
        glTextureParameteri( _Id, GL_TEXTURE_SWIZZLE_A, SwizzleLUT[_Swizzle.A] );
    }
}

void Texture::Initialize( TextureCreateInfo const & _CreateInfo, TextureInitialData const * _InitialData ) {
    State * state = GetCurrentState();
    GLuint id;
    GLint currentBinding;

    Deinitialize();

    GLenum target = TextureTargetLUT[ _CreateInfo.Type ].Target;

    glGetIntegerv( TextureTargetLUT[ _CreateInfo.Type ].Binding, &currentBinding );

    glCreateTextures( target, 1, &id ); // 4.5

    SetSwizzleParams( id, _CreateInfo.Swizzle );

    glBindTexture( target, id );

    CreateTextureLod( _CreateInfo, 0, _InitialData );

    glBindTexture( target, currentBinding );

    pDevice = state->GetDevice();
    pDevice->TotalTextures++;
    pDevice->TextureMemoryAllocated += CalcTextureRequiredMemory( _CreateInfo );

    Handle = ( void * )( size_t )id;
    UID = pDevice->GenerateUID();
    CreateInfo = _CreateInfo;
    bImmutableStorage = false;
    bTextureBuffer = false;
    bTextureView = false;
}

void Texture::InitializeStorage( TextureStorageCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();
    GLuint id;
    GLenum target = TextureTargetLUT[ _CreateInfo.Type ].Target;
    GLenum internalFormat = InternalFormatLUT[ _CreateInfo.InternalFormat ].InternalFormat;

    Deinitialize();

    memset( &CreateInfo, 0, sizeof( CreateInfo ) );
    CreateInfo.Type = _CreateInfo.Type;
    CreateInfo.InternalFormat = _CreateInfo.InternalFormat;
    CreateInfo.Resolution = _CreateInfo.Resolution;

    glCreateTextures( target, 1, &id );

    SetSwizzleParams( id, _CreateInfo.Swizzle );

    switch ( _CreateInfo.Type ) {
    case TEXTURE_1D:
        glTextureStorage1D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.Tex1D.Width );
        break;
    case TEXTURE_1D_ARRAY:
        glTextureStorage2D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.Tex1DArray.Width, _CreateInfo.Resolution.Tex1DArray.NumLayers );
        break;
    case TEXTURE_2D:
        glTextureStorage2D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.Tex2D.Width, _CreateInfo.Resolution.Tex2D.Height );
        break;
    case TEXTURE_2D_MULTISAMPLE:
        glTextureStorage2DMultisample( id, _CreateInfo.Multisample.NumSamples, internalFormat, _CreateInfo.Resolution.Tex2D.Width, _CreateInfo.Resolution.Tex2D.Height, _CreateInfo.Multisample.bFixedSampleLocations );
        CreateInfo.Multisample.NumSamples = _CreateInfo.Multisample.NumSamples;
        CreateInfo.Multisample.bFixedSampleLocations = _CreateInfo.Multisample.bFixedSampleLocations;
        break;
    case TEXTURE_2D_ARRAY:
        glTextureStorage3D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.Tex2DArray.Width, _CreateInfo.Resolution.Tex2DArray.Height, _CreateInfo.Resolution.Tex2DArray.NumLayers );
        break;
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        glTextureStorage3DMultisample( id, _CreateInfo.Multisample.NumSamples, internalFormat, _CreateInfo.Resolution.Tex2DArray.Width, _CreateInfo.Resolution.Tex2DArray.Height, _CreateInfo.Resolution.Tex2DArray.NumLayers, _CreateInfo.Multisample.bFixedSampleLocations );
        CreateInfo.Multisample.NumSamples = _CreateInfo.Multisample.NumSamples;
        CreateInfo.Multisample.bFixedSampleLocations = _CreateInfo.Multisample.bFixedSampleLocations;
        break;
    case TEXTURE_3D:
        glTextureStorage3D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.Tex3D.Width, _CreateInfo.Resolution.Tex3D.Height, _CreateInfo.Resolution.Tex3D.Depth );
        break;
    case TEXTURE_CUBE_MAP:
        glTextureStorage2D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.TexCubemap.Width, _CreateInfo.Resolution.TexCubemap.Width );
        break;
    case TEXTURE_CUBE_MAP_ARRAY:
        glTextureStorage3D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.TexCubemapArray.Width, _CreateInfo.Resolution.TexCubemapArray.Width, _CreateInfo.Resolution.TexCubemapArray.NumLayers * 6 );
        break;
    case TEXTURE_RECT:
        glTextureStorage2D( id, _CreateInfo.NumLods, internalFormat, _CreateInfo.Resolution.TexRect.Width, _CreateInfo.Resolution.TexRect.Height );
        break;
    }

    pDevice = state->GetDevice();
    pDevice->TotalTextures++;
    pDevice->TextureMemoryAllocated += CalcTextureRequiredMemory( _CreateInfo );

    //int param;
    //glGetTextureParameteriv( id, GL_TEXTURE_IMMUTABLE_FORMAT, &param );
    bImmutableStorage = true;//param != 0;

    bTextureBuffer = false;
    bTextureView = false;

    Handle = ( void * )( size_t )id;
    UID = pDevice->GenerateUID();
}

void Texture::InitializeTextureBuffer( BUFFER_DATA_TYPE _DataType, Buffer const & _Buffer ) {
    State * state = GetCurrentState();
    GLuint bufferId = GL_HANDLE( _Buffer.GetHandle() );
    GLuint textureId;

    Deinitialize();

    if ( !bufferId ) {
        LogPrintf( "Texture::InitializeTextureBuffer: buffer must have been created before\n" );
    }

    if ( _Buffer.GetSizeInBytes() > state->pDevice->MaxTextureBufferSize ) {
        LogPrintf( "Texture::InitializeTextureBuffer: Warning: buffer size > MaxTextureBufferSize\n" );
    }

    // TODO: Check errors

    glCreateTextures( GL_TEXTURE_BUFFER, 1, &textureId );
    // FIXME: UnpackAlignment???

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    glTextureBuffer( textureId, type->InternalFormat, bufferId );

    pDevice = state->GetDevice();
    pDevice->TotalTextures++;

    memset( &CreateInfo, 0, sizeof( CreateInfo ) );
    CreateInfo.InternalFormat = type->IPF;

    bImmutableStorage = false;
    bTextureBuffer = true;
    bTextureView = false;

    Handle = ( void * )( size_t )textureId;
    UID = pDevice->GenerateUID();
}

void Texture::InitializeTextureBuffer( BUFFER_DATA_TYPE _DataType, Buffer const & _Buffer,
                                       size_t _Offset, size_t _SizeInBytes ) {
    State * state = GetCurrentState();
    GLuint bufferId = GL_HANDLE( _Buffer.GetHandle() );
    GLuint textureId;

    Deinitialize();

    if ( !bufferId ) {
        LogPrintf( "Texture::InitializeTextureBuffer: buffer must have been created before\n" );
    }

    if ( ( _Offset % state->pDevice->TextureBufferOffsetAlignment ) == 0 ) {
        LogPrintf( "Texture::InitializeTextureBuffer: Warning: buffer offset is not aligned\n" );
    }

    if ( _SizeInBytes > state->pDevice->MaxTextureBufferSize ) {
        LogPrintf( "Texture::InitializeTextureBuffer: Warning: buffer size > MaxTextureBufferSize\n" );
    }

    // TODO: Check errors

    glCreateTextures( GL_TEXTURE_BUFFER, 1, &textureId );
    // FIXME: UnpackAlignment???

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    glTextureBufferRange( textureId, type->InternalFormat, bufferId, _Offset, _SizeInBytes );

    pDevice = state->GetDevice();
    pDevice->TotalTextures++;

    memset( &CreateInfo, 0, sizeof( CreateInfo ) );
    CreateInfo.InternalFormat = type->IPF;

    bImmutableStorage = false;
    bTextureBuffer = true;
    bTextureView = false;

    Handle = ( void * )( size_t )textureId;
    UID = pDevice->GenerateUID();
}

static bool IsTextureViewCompatible( TEXTURE_TYPE _OriginalType, TEXTURE_TYPE _ViewType ) {
    // From OpenGL specification:
    switch ( _OriginalType ) {
    case TEXTURE_1D:
    case TEXTURE_1D_ARRAY:
        return ( _ViewType == TEXTURE_1D || _ViewType == TEXTURE_1D_ARRAY );
    case TEXTURE_2D:
    case TEXTURE_2D_ARRAY:
        return ( _ViewType == TEXTURE_2D || _ViewType == TEXTURE_2D_ARRAY );
    case TEXTURE_3D:
        return ( _ViewType == TEXTURE_3D );
    case TEXTURE_CUBE_MAP:
    case TEXTURE_CUBE_MAP_ARRAY:
        return ( _ViewType == TEXTURE_CUBE_MAP || _ViewType == TEXTURE_2D || _ViewType == TEXTURE_2D_ARRAY || _ViewType == TEXTURE_CUBE_MAP_ARRAY );
    case TEXTURE_RECT:
        return ( _ViewType == TEXTURE_RECT );
    case TEXTURE_2D_MULTISAMPLE:
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        return ( _ViewType == TEXTURE_2D_MULTISAMPLE || _ViewType == TEXTURE_2D_ARRAY_MULTISAMPLE );
    }
    return false;
}

bool Texture::InitializeView( TextureViewCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();
    GLuint id;
    GLenum target = TextureTargetLUT[ _CreateInfo.Type ].Target;
    GLint internalFormat = InternalFormatLUT[ _CreateInfo.InternalFormat ].InternalFormat;

    Deinitialize();

    if ( _CreateInfo.pOriginalTexture->IsTextureBuffer() ) {
        LogPrintf( "Texture::InitializeView: original texture is texture buffer\n" );
        return false;
    }

    if ( !_CreateInfo.pOriginalTexture->IsImmutableStorage() ) {
        LogPrintf( "Texture::InitializeView: original texture must be immutable storage\n" );
        return false;
    }

    if ( !IsTextureViewCompatible( _CreateInfo.pOriginalTexture->GetType(), _CreateInfo.Type ) ) {
        LogPrintf( "Texture::InitializeView: incompatible texture types\n" );
        return false;
    }

    // TODO: Check internal formats compatibility
    // TODO: Check texture resolution compatibility
    // TODO: Lods/Layers compatibility

    // Avoid previous errors if any
    (void)glGetError();

    glCreateTextures( target, 1, &id ); // 4.5

    //GLint currentBinding;
    //glGetIntegerv( TextureTargetLUT[ _CreateInfo.Type ].Binding, &currentBinding );
    //glBindTexture( target, id );

    // 4.3
    glTextureView( id, target, GL_HANDLE( _CreateInfo.pOriginalTexture->GetHandle() ), internalFormat, _CreateInfo.MinLod, _CreateInfo.NumLods, _CreateInfo.MinLayer, _CreateInfo.NumLayers );

    if ( glGetError() != GL_NO_ERROR ) {
        // Incompatible texture formats (See OpenGL specification for details)
        if ( glIsTexture( id ) ) {
            glDeleteTextures( 1, &id );
        }
        LogPrintf( "Texture::InitializeView: incompatible texture formats\n" );
        return false;
    }

    //glBindTexture( target, currentBinding );

    pDevice = state->GetDevice();
    pDevice->TotalTextures++;

    Handle = ( void * )( size_t )id;
    UID = pDevice->GenerateUID();
    CreateInfo = _CreateInfo.pOriginalTexture->CreateInfo;
    CreateInfo.Type = _CreateInfo.Type;
    CreateInfo.InternalFormat = _CreateInfo.InternalFormat;

    bImmutableStorage = true;
    bTextureBuffer = false;
    bTextureView = true;

    return true;
}

bool Texture::Realloc( TextureCreateInfo const & _CreateInfo, TextureInitialData const * _InitialData ) {
    if ( !Handle ) {
        return false;
    }

    if ( bImmutableStorage ) {
        LogPrintf( "Texture::Realloc: immutable texture cannot be reallocated\n" );
        return false;
    }

    if ( bTextureBuffer ) {
        LogPrintf( "Texture::Realloc: texture buffer cannot be reallocated\n" );
        return false;
    }

    if ( bTextureView ) {
        LogPrintf( "Texture::Realloc: texture view cannot be reallocated\n" );
        return false;
    }

    GLint currentBinding;
    GLint id = GL_HANDLE( Handle );
    GLenum target = TextureTargetLUT[ _CreateInfo.Type ].Target;

    glGetIntegerv( TextureTargetLUT[ _CreateInfo.Type ].Binding, &currentBinding );

    if ( currentBinding != id ) {
        glBindTexture( target, id );
    }

    CreateTextureLod( _CreateInfo, 0, _InitialData );

    if ( currentBinding != id ) {
        glBindTexture( target, currentBinding );
    }

    CreateInfo = _CreateInfo;

    return true;
}

void Texture::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    GLuint id = GL_HANDLE( Handle );
    glDeleteTextures( 1, &id );
    pDevice->TotalTextures--;

    if ( !bTextureBuffer && !bTextureView ) {
        pDevice->TextureMemoryAllocated -= CalcTextureRequiredMemory( CreateInfo );
    }

    pDevice = nullptr;
    Handle = nullptr;
}

uint32_t Texture::GetWidth() const {
    switch ( CreateInfo.Type ) {
    case TEXTURE_1D:
        return CreateInfo.Resolution.Tex1D.Width;
    case TEXTURE_1D_ARRAY:
        return CreateInfo.Resolution.Tex1DArray.Width;
    case TEXTURE_2D:
    case TEXTURE_2D_MULTISAMPLE:
        return CreateInfo.Resolution.Tex2D.Width;
    case TEXTURE_2D_ARRAY:
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        return CreateInfo.Resolution.Tex2DArray.Width;
    case TEXTURE_3D:
        return CreateInfo.Resolution.Tex3D.Width;
    case TEXTURE_CUBE_MAP:
        return CreateInfo.Resolution.TexCubemap.Width;
    case TEXTURE_CUBE_MAP_ARRAY:
        return CreateInfo.Resolution.TexCubemapArray.Width;
    case TEXTURE_RECT:
        return CreateInfo.Resolution.TexRect.Width;
    }
    assert( 0 );
    return 0;
}

uint32_t Texture::GetHeight() const {
    switch ( CreateInfo.Type ) {
    case TEXTURE_1D:
    case TEXTURE_1D_ARRAY:
        return 1;
    case TEXTURE_2D:
    case TEXTURE_2D_MULTISAMPLE:
        return CreateInfo.Resolution.Tex2D.Height;
    case TEXTURE_2D_ARRAY:
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        return CreateInfo.Resolution.Tex2DArray.Height;
    case TEXTURE_3D:
        return CreateInfo.Resolution.Tex3D.Height;
    case TEXTURE_CUBE_MAP:
        return CreateInfo.Resolution.TexCubemap.Width;
    case TEXTURE_CUBE_MAP_ARRAY:
        return CreateInfo.Resolution.TexCubemapArray.Width;
    case TEXTURE_RECT:
        return CreateInfo.Resolution.TexRect.Height;
    }
    assert( 0 );
    return 0;
}

void Texture::CreateTextureLod( TextureCreateInfo const & _CreateInfo, uint16_t _Lod, TextureInitialData const * _InitialData ) {
    GLenum format;
    GLenum pixelType;
    bool bCompressed;
    unsigned int alignment;
    const void * sysMem;
    GLsizei compressedDataByteLength;
    uint16_t arrayLength;

    // TODO: must be: arrayLength < GL_MAX_ARRAY_TEXTURE_LAYERS for texture arrays
    if ( _CreateInfo.Type == TEXTURE_1D_ARRAY ) {
        arrayLength = ( uint16_t )std::max( 1u, _CreateInfo.Resolution.Tex1DArray.NumLayers );
    } else if ( _CreateInfo.Type == TEXTURE_2D_ARRAY || _CreateInfo.Type == TEXTURE_2D_ARRAY_MULTISAMPLE ) {
        arrayLength = ( uint16_t )std::max( 1u, _CreateInfo.Resolution.Tex2DArray.NumLayers );
    } else if ( _CreateInfo.Type == TEXTURE_CUBE_MAP_ARRAY ) {
        arrayLength = ( uint16_t )std::max( 1u, _CreateInfo.Resolution.TexCubemapArray.NumLayers );
    } else {
        arrayLength = 1;
    }

    GLint internalFormat = InternalFormatLUT[ _CreateInfo.InternalFormat ].InternalFormat;
    GLenum target = TextureTargetLUT[ _CreateInfo.Type ].Target;

    // TODO: NumSamples must be <= GL_MAX_DEPTH_TEXTURE_SAMPLES for depth/stencil integer internal format
    //                          <= GL_MAX_COLOR_TEXTURE_SAMPLES for color integer internal format
    //                          <= GL_MAX_INTEGER_SAMPLES for signed/unsigned integer internal format
    //                          <= GL_MAX_SAMPLES-1 for all formats
    int numSamples = _CreateInfo.Multisample.NumSamples;

    uint32_t lodWidth = std::max( 1u, _CreateInfo.Resolution.Tex3D.Width >> _Lod );
    uint32_t lodHeight = std::max( 1u, _CreateInfo.Resolution.Tex3D.Height >> _Lod );
    uint32_t lodDepth = std::max( 1u, _CreateInfo.Resolution.Tex3D.Depth >> _Lod );

    // TODO: lodWidth and lodHeight​ must be <= GL_MAX_TEXTURE_SIZE

    if ( _InitialData ) {
        format = TexturePixelFormatLUT[ _InitialData->PixelFormat ].Format;
        pixelType = TexturePixelFormatLUT[ _InitialData->PixelFormat ].PixelType;
        bCompressed = ( pixelType == 0 ); // Pixel type is 0 for compressed input data
        alignment = std::max( 1u, _InitialData->Alignment );
        sysMem = _InitialData->SysMem;
        compressedDataByteLength = (GLsizei)_InitialData->SizeInBytes;
    } else {
        format = InternalFormatLUT[ _CreateInfo.InternalFormat ].Format;
        int i = _CreateInfo.InternalFormat - INTERNAL_PIXEL_FORMAT_STENCIL1;
        if ( i >= 0 ) {
            pixelType = MagicTextureFormatLUT[ i ].PixelType;
        } else {
            pixelType = GL_UNSIGNED_BYTE;
        }
        bCompressed = false;
        alignment = 1;
        sysMem = NULL;
        compressedDataByteLength = 0;
    }

    if ( sysMem ) {
        State * state = GetCurrentState();

        state->UnpackAlignment( alignment );
    }

    switch ( _CreateInfo.Type ) {
    case TEXTURE_1D:
        if ( bCompressed ) {
            glCompressedTexImage1D( target, 0, format, lodWidth, 0, compressedDataByteLength, sysMem );
        } else {
            glTexImage1D( target, 0, internalFormat, lodWidth, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_1D_ARRAY:
        if ( bCompressed ) {
            glCompressedTexImage2D( target, 0, format, lodWidth, arrayLength, 0, compressedDataByteLength, sysMem );
        } else {
            glTexImage2D( target, 0, internalFormat, lodWidth, arrayLength, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_2D:
        if ( bCompressed ) {
            glCompressedTexImage2D( target, 0, format, lodWidth, lodHeight, 0, compressedDataByteLength, sysMem );
        } else {
            glTexImage2D( target, 0, internalFormat, lodWidth, lodHeight, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_2D_MULTISAMPLE:
        if ( _Lod == 0 ) {
            glTexImage2DMultisample( target, numSamples, internalFormat, lodWidth, lodHeight, _CreateInfo.Multisample.bFixedSampleLocations ); // 3.2
        }
        break;
    case TEXTURE_3D:
        // Avoid previous errors if any
        (void)glGetError();
        if ( bCompressed ) {
            glCompressedTexImage3D( target, 0, format, lodWidth, lodHeight, lodDepth, 0, compressedDataByteLength, sysMem );
        } else {
            glTexImage3D( target, 0, internalFormat, lodWidth, lodHeight, lodDepth, 0, format, pixelType, sysMem );
        }
        // FIXME: wtf this error?
        {
            int error = glGetError();
            LogPrintf( "Error %d\n", error );
        }
        break;
    case TEXTURE_2D_ARRAY:
        if ( bCompressed ) {
            glCompressedTexImage3D( target, 0, format, lodWidth, lodHeight, arrayLength, 0, compressedDataByteLength, sysMem );
        } else {
            glTexImage3D( target, 0, internalFormat, lodWidth, lodHeight, arrayLength, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        if ( _Lod == 0 ) {
            glTexImage3DMultisample( target, numSamples, internalFormat, lodWidth, lodHeight, arrayLength, _CreateInfo.Multisample.bFixedSampleLocations ); // 3.2
        }
        break;
    case TEXTURE_CUBE_MAP:
        if ( bCompressed ) {
            // TODO: check this:
            for ( int face = 0; face < 6; face++ ) {
                if ( sysMem ) {
                    glCompressedTexImage3D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, format,
                        lodWidth, lodWidth, 1, 0, compressedDataByteLength, ( const uint8_t * )( sysMem ) + face * _InitialData->SizeInBytes );

                } else {
                    glCompressedTexImage3D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, format,
                        lodWidth, lodWidth, 1, 0, compressedDataByteLength, NULL );
                }
            }
        } else {
            // TODO: check this:
            int sizeOfPixel = _InitialData ? TexturePixelFormatLUT[ _InitialData->PixelFormat ].SizeOf : 0;
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
        if ( bCompressed ) {
            glCompressedTexImage3D( target, 0, format, lodWidth, lodWidth, arrayLength * 6, 0, compressedDataByteLength, sysMem );
        } else {
            glTexImage3D( target, 0, internalFormat, lodWidth, lodWidth, arrayLength * 6, 0, format, pixelType, sysMem );
        }
        break;
    case TEXTURE_RECT:
        glTexImage2D( target, 0, internalFormat, lodWidth, lodHeight, 0, format, pixelType, sysMem );
        break;
    }
}

bool Texture::CreateLod( uint16_t _Lod, TextureInitialData const * _InitialData ) {

    if ( bImmutableStorage ) {
        LogPrintf( "Texture::CreateLod: this function is allowed only for mutable textures\n" );
        return false;
    }

    if ( bTextureBuffer ) {
        LogPrintf( "Texture::CreateLod: this function is not allowed for texture buffers\n" );
        return false;
    }

    if ( bTextureView ) {
        LogPrintf( "Texture::CreateLod: this function is not allowed for texture views\n" );
        return false;
    }

    GLenum target = TextureTargetLUT[ CreateInfo.Type ].Target;
    GLint currentBinding;

    glGetIntegerv( TextureTargetLUT[ CreateInfo.Type ].Binding, &currentBinding );
    glBindTexture( target, GL_HANDLE( Handle ) );

    CreateTextureLod( CreateInfo, _Lod, _InitialData );

    glBindTexture( target, currentBinding );

    return true;
}

void Texture::GenerateLods() {
    if ( !Handle ) {
        return;
    }

    if ( bTextureBuffer ) {
        LogPrintf( "Texture::GenerateLods: this function is not allowed for texture buffers\n" );
        return;
    }

    glGenerateTextureMipmap( GL_HANDLE( Handle ) );
}

void Texture::GetLodInfo( uint16_t _Lod, TextureLodInfo * _Info ) const {
    GLuint id = GL_HANDLE( Handle );
    int width, height, depth, tmp;
    TEXTURE_TYPE type = CreateInfo.Type;

    memset( _Info, 0, sizeof( TextureLodInfo ) );

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_WIDTH, &width );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_HEIGHT, &height );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_DEPTH, &depth );

    switch ( type ) {
    case TEXTURE_1D:
        _Info->Resoultion.Tex1D.Width = width;
        break;
    case TEXTURE_1D_ARRAY:
        _Info->Resoultion.Tex1DArray.Width = width;
        _Info->Resoultion.Tex1DArray.NumLayers = height;
        break;
    case TEXTURE_2D:
    case TEXTURE_2D_MULTISAMPLE:
        _Info->Resoultion.Tex2D.Width = width;
        _Info->Resoultion.Tex2D.Height = height;
        break;
    case TEXTURE_2D_ARRAY:
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
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
    case TEXTURE_RECT:
        _Info->Resoultion.TexRect.Width = width;
        _Info->Resoultion.TexRect.Height = height;
        break;
    }

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_COMPRESSED, &tmp );
    _Info->bCompressed = tmp != 0;

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &tmp );
    _Info->CompressedDataByteLength = tmp;

    //GL_TEXTURE_INTERNAL_FORMAT
    //params returns a single value, the internal format of the texture image.

    //GL_TEXTURE_RED_TYPE, GL_TEXTURE_GREEN_TYPE, GL_TEXTURE_BLUE_TYPE, GL_TEXTURE_ALPHA_TYPE, GL_TEXTURE_DEPTH_TYPE
    //The data type used to store the component. The types GL_NONE, GL_SIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_FLOAT, GL_INT, and GL_UNSIGNED_INT may be returned to indicate signed normalized fixed-point, unsigned normalized fixed-point, floating-point, integer unnormalized, and unsigned integer unnormalized components, respectively.

    //GL_TEXTURE_RED_SIZE, GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE, GL_TEXTURE_ALPHA_SIZE, GL_TEXTURE_DEPTH_SIZE
    //The internal storage resolution of an individual component. The resolution chosen by the GL will be a close match for the resolution requested by the user with the component argument of glTexImage1D, glTexImage2D, glTexImage3D, glCopyTexImage1D, and glCopyTexImage2D. The initial value is 0.
}

void Texture::Read( uint16_t _Lod,
                    TEXTURE_PIXEL_FORMAT _PixelFormat, // Specifies a pixel format for the returned data
                    size_t _SizeInBytes,
                    unsigned int _Alignment,               // Specifies alignment of destination data
                    void * _SysMem ) {
    State * state = GetCurrentState();

    GLuint id = GL_HANDLE( Handle );
    int bCompressed;

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_COMPRESSED, &bCompressed );

    state->PackAlignment( _Alignment );

    if ( bCompressed ) {
        glGetCompressedTextureImage( id, _Lod, (GLsizei)_SizeInBytes, _SysMem );
    } else {
        glGetTextureImage( id,
                           _Lod,
                           TexturePixelFormatLUT[ _PixelFormat ].Format,
                           TexturePixelFormatLUT[ _PixelFormat ].PixelType,
                           (GLsizei)_SizeInBytes,
                           _SysMem );
    }
}

void Texture::ReadRect( TextureRect const & _Rectangle,
                        TEXTURE_PIXEL_FORMAT _PixelFormat, // Specifies a pixel format for the returned data
                        size_t _SizeInBytes,
                        unsigned int _Alignment,               // Specifies alignment of destination data
                        void * _SysMem ) {
    State * state = GetCurrentState();

    GLuint id = GL_HANDLE( Handle );
    int bCompressed;

    glGetTextureLevelParameteriv( id, _Rectangle.Offset.Lod, GL_TEXTURE_COMPRESSED, &bCompressed );

    state->PackAlignment( _Alignment );

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
                              TexturePixelFormatLUT[ _PixelFormat ].Format,
                              TexturePixelFormatLUT[ _PixelFormat ].PixelType,
                              (GLsizei)_SizeInBytes,
                              _SysMem );
    }
}

bool Texture::Write( uint16_t _Lod,
                     TEXTURE_PIXEL_FORMAT _PixelFormat, // Specifies a pixel format for the input data
                     size_t _SizeInBytes,
                     unsigned int _Alignment,               // Specifies alignment of source data
                     const void * _SysMem ) {
    GLuint id = GL_HANDLE( Handle );

    int dimensionX, dimensionY, dimensionZ;

    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_WIDTH, &dimensionX );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_HEIGHT, &dimensionY );
    glGetTextureLevelParameteriv( id, _Lod, GL_TEXTURE_DEPTH, &dimensionZ );

    TextureRect rect;
    rect.Offset.Lod = _Lod;
    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Dimension.X = (uint16_t)dimensionX;
    rect.Dimension.Y = (uint16_t)dimensionY;
    rect.Dimension.Z = (uint16_t)dimensionZ;

    return WriteRect( rect,
                      _PixelFormat,
                      _SizeInBytes,
                      _Alignment,
                      _SysMem );
}

bool Texture::WriteRect( TextureRect const & _Rectangle,
                         TEXTURE_PIXEL_FORMAT _PixelFormat, // Specifies a pixel format for the input data
                         size_t _SizeInBytes,
                         unsigned int _Alignment,               // Specifies alignment of source data
                         const void * _SysMem ) {
    State * state = GetCurrentState();

    GLuint id = GL_HANDLE( Handle );
    GLenum format = TexturePixelFormatLUT[ _PixelFormat ].Format;
    GLenum pixelType = TexturePixelFormatLUT[ _PixelFormat ].PixelType;
    bool bCompressed = ( pixelType == 0 );

    state->UnpackAlignment( _Alignment );

    switch ( CreateInfo.Type ) {
    case TEXTURE_1D:
        if ( bCompressed ) {
            glCompressedTextureSubImage1D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Dimension.X,
                                           format,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            glTextureSubImage1D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Dimension.X,
                                 format,
                                 pixelType,
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
                                           format,
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
                                 pixelType,
                                 _SysMem );
        }
        break;
    case TEXTURE_2D:
        if ( bCompressed ) {
            glCompressedTextureSubImage2D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           format,
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
                                 pixelType,
                                 _SysMem );
        }
        break;
    case TEXTURE_2D_MULTISAMPLE:
        return false;
    case TEXTURE_2D_ARRAY:
        if ( bCompressed ) {
            glCompressedTextureSubImage3D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Offset.Z,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           _Rectangle.Dimension.Z,
                                           format,
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
                                 pixelType,
                                 _SysMem );
        }
        break;
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        return false;
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
                                           format,
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
                                 pixelType,
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
                                           format,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            //glTextureSubImage2D( id, _Rectangle.Offset.Lod, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, pixelType, _SysMem );

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
                                 pixelType,
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
                                           format,
                                           (GLsizei)_SizeInBytes,
                                           _SysMem );
        } else {
            //glTextureSubImage2D( id, _Rectangle.Offset.Lod, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, pixelType, _SysMem );

            glTextureSubImage3D( id,
                                 _Rectangle.Offset.Lod,
                                 _Rectangle.Offset.X,
                                 _Rectangle.Offset.Y,
                                 _Rectangle.Offset.Z,
                                 _Rectangle.Dimension.X,
                                 _Rectangle.Dimension.Y,
                                 _Rectangle.Dimension.Z,
                                 format,
                                 pixelType,
                                 _SysMem );
        }
        break;
    case TEXTURE_RECT:
        // FIXME: В спецификации ничего не сказано о возможности записи в данный тип текстурного таргета
        if ( bCompressed ) {
            glCompressedTextureSubImage2D( id,
                                           _Rectangle.Offset.Lod,
                                           _Rectangle.Offset.X,
                                           _Rectangle.Offset.Y,
                                           _Rectangle.Dimension.X,
                                           _Rectangle.Dimension.Y,
                                           format,
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
                                 pixelType,
                                 _SysMem );
        }
        break;
    }

    return true;
}

void Texture::Invalidate( uint16_t _Lod ) {
    glInvalidateTexImage( GL_HANDLE( Handle ), _Lod );
}

void Texture::InvalidateRect( uint32_t _NumRectangles, TextureRect const * _Rectangles ) {
    GLuint id = GL_HANDLE( Handle );

    for ( TextureRect const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
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

size_t Texture::GetTextureBufferOffset( uint16_t _Lod ) {
    int offset;
    glGetTextureLevelParameteriv( GL_HANDLE( Handle ), _Lod, GL_TEXTURE_BUFFER_OFFSET, &offset );
    return offset;
}

size_t Texture::GetTextureBufferByteLength( uint16_t _Lod ) {
    int byteLength;
    glGetTextureLevelParameteriv( GL_HANDLE( Handle ), _Lod, GL_TEXTURE_BUFFER_SIZE, &byteLength );
    return byteLength;
}

static int Log2( int64_t value ) {
    int log2 = 0;
    while ( value >>= 1 )
        log2++;
    return log2;
}

int Texture::CalcMaxLods( TEXTURE_TYPE _Type, TextureResolution const & _Resolution ) {
    int maxLods = 0;
    int maxDimension = 0;

    switch ( _Type ) {
    case TEXTURE_1D:
        maxLods = _Resolution.Tex1D.Width > 0 ? Log2( _Resolution.Tex1D.Width ) + 1 : 0;
        break;
    case TEXTURE_1D_ARRAY:
        maxLods = _Resolution.Tex1DArray.Width > 0 ? Log2( _Resolution.Tex1DArray.Width ) + 1 : 0;
        break;

    case TEXTURE_2D:
    case TEXTURE_2D_MULTISAMPLE:
        maxDimension = std::max( _Resolution.Tex2D.Width, _Resolution.Tex2D.Height );
        maxLods = maxDimension > 0 ? Log2( maxDimension ) + 1 : 0;
        break;

    case TEXTURE_2D_ARRAY:
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
        maxDimension = std::max( _Resolution.Tex2DArray.Width, _Resolution.Tex2DArray.Height );
        maxLods = maxDimension > 0 ? Log2( maxDimension ) + 1 : 0;
        break;

    case TEXTURE_3D:
        maxDimension = max3( _Resolution.Tex3D.Width, _Resolution.Tex3D.Height, _Resolution.Tex3D.Depth );
        maxLods = maxDimension > 0 ? Log2( maxDimension ) + 1 : 0;
        break;

    case TEXTURE_CUBE_MAP:
        maxLods = _Resolution.TexCubemap.Width > 0 ? Log2( _Resolution.TexCubemap.Width ) + 1 : 0;
        break;

    case TEXTURE_CUBE_MAP_ARRAY:
        maxLods = _Resolution.TexCubemapArray.Width > 0 ? Log2( _Resolution.TexCubemapArray.Width ) + 1 : 0;
        break;

    case TEXTURE_RECT:
        maxDimension = std::max( _Resolution.TexRect.Width, _Resolution.TexRect.Height );
        maxLods = maxDimension > 0 ? Log2( maxDimension ) + 1 : 0;
        break;
    }

    return maxLods;
}

bool Texture::LookupImageFormat( const char * _FormatQualifier, INTERNAL_PIXEL_FORMAT * _InternalPixelFormat ) {
    int numFormats = sizeof( InternalFormatLUT ) / sizeof( InternalFormatLUT[0] );
    for ( int i = 0 ; i < numFormats ; i++ ) {
        if ( !strcmp( InternalFormatLUT[ i ].ShaderImageFormatQualifier, _FormatQualifier ) ) {
            *_InternalPixelFormat = (INTERNAL_PIXEL_FORMAT)i;
            return true;
        }
    }
    return false;
}

const char * Texture::LookupImageFormatQualifier( INTERNAL_PIXEL_FORMAT _InternalPixelFormat ) {
    return InternalFormatLUT[ _InternalPixelFormat ].ShaderImageFormatQualifier;
}

}
