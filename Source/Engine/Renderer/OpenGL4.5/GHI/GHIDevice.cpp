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

#include "GHIDevice.h"
#include "GHIBuffer.h"
#include "GHISampler.h"
#include "LUT.h"

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <GL/glew.h>

namespace GHI {

struct SamplerInfo {
    SamplerCreateInfo CreateInfo;
    void * Handle;
};

template< typename T > T clamp( T const & _a, T const & _min, T const & _max ) {
    return std::max( std::min( _a, _max ), _min );
}

Device::Device() {
    TotalStates = 0;
    TotalBuffers = 0;
    TotalTextures = 0;
    TotalShaderModules = 0;
    BufferMemoryAllocated = 0;
    TextureMemoryAllocated = 0;
}

Device::~Device() {

}

static int GL_GetInteger( GLenum pname ) {
    GLint i;
    glGetIntegerv( pname, &i );
    return i;
}

static float GL_GetFloat( GLenum pname ) {
    float f;
    glGetFloatv( pname, &f );
    return f;
}

static bool FindExtension( const char * _Extension ) {
    GLint NumExtensions = 0;

    glGetIntegerv( GL_NUM_EXTENSIONS, &NumExtensions );
    for ( int i = 0 ; i < NumExtensions ; i++ ) {
        const char * ExtI = ( const char * )glGetStringi( GL_EXTENSIONS, i );
        if ( ExtI && strcmp( ExtI, _Extension ) == 0 ) {
            return true;
        }
    }
    return false;
}

static void * Allocate( size_t _BytesCount ) {
    return malloc( _BytesCount );
}

static void Deallocate( void * _Bytes ) {
    free( _Bytes );
}

static constexpr AllocatorCallback DefaultAllocator =
{
    Allocate, Deallocate
};

static int SDBM_Hash( const unsigned char * _Data, int _Size ) {
    int hash = 0;
    while ( _Size-- > 0 ) {
        hash = *_Data++ + ( hash << 6 ) + ( hash << 16 ) - hash;
    }
    return hash;
}

void Device::Initialize( AllocatorCallback const * _Allocator, HashCallback _Hash ) {

    bool NV_half_float = FindExtension( "GL_NV_half_float" );

    bHalfFloatVertexSupported = FindExtension( "GL_ARB_half_float_vertex" ) || NV_half_float;
    bHalfFloatPixelSupported = FindExtension( "GL_ARB_half_float_pixel" ) || NV_half_float;
    bTextureCompressionS3tcSupported = FindExtension( "GL_EXT_texture_compression_s3tc" );
    bTextureAnisotropySupported = FindExtension( "GL_EXT_texture_filter_anisotropic" );

    MaxVertexBufferSlots = GL_GetInteger( GL_MAX_VERTEX_ATTRIB_BINDINGS ); // TODO: check if 0 ???

    // GL_MAX_VERTEX_ATTRIB_STRIDE sinse GL v4.4
    MaxVertexAttribStride = GL_GetInteger( GL_MAX_VERTEX_ATTRIB_STRIDE );
    //if ( GL vertsion < 4.4 ) {
    //    storage->MaxVertexAttribStride = 2048;
    //}
    MaxVertexAttribRelativeOffset = GL_GetInteger( GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET );

    MaxCombinedTextureImageUnits = GL_GetInteger( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS );

    MaxImageUnits = GL_GetInteger( GL_MAX_IMAGE_UNITS );

    MaxTextureBufferSize = GL_GetInteger( GL_MAX_TEXTURE_BUFFER_SIZE );

    TextureBufferOffsetAlignment = GL_GetInteger( GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT );
    if ( !TextureBufferOffsetAlignment ) {
        LogPrintf( "Warning: TextureBufferOffsetAlignment == 0, using default alignment (256)\n" );
        TextureBufferOffsetAlignment = 256;
    }

    UniformBufferOffsetAlignment = GL_GetInteger( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT );    
    if ( !UniformBufferOffsetAlignment ) {
        LogPrintf( "Warning: UniformBufferOffsetAlignment == 0, using default alignment (256)\n" );
        UniformBufferOffsetAlignment = 256;
    }

    ShaderStorageBufferOffsetAlignment = GL_GetInteger( GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT );
    if ( !ShaderStorageBufferOffsetAlignment ) {
        LogPrintf( "Warning: ShaderStorageBufferOffsetAlignment == 0, using default alignment (256)\n" );
        ShaderStorageBufferOffsetAlignment = 256;
    }

    MaxBufferBindings[UNIFORM_BUFFER] = GL_GetInteger( GL_MAX_UNIFORM_BUFFER_BINDINGS );
    MaxBufferBindings[SHADER_STORAGE_BUFFER] = GL_GetInteger( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS );
    MaxBufferBindings[TRANSFORM_FEEDBACK_BUFFER] = GL_GetInteger( GL_MAX_TRANSFORM_FEEDBACK_BUFFERS );
    MaxBufferBindings[ATOMIC_COUNTER_BUFFER] = GL_GetInteger( GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS );

    if ( bTextureAnisotropySupported ) {
        MaxTextureAnisotropy = (unsigned int)GL_GetFloat( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT );
    } else {
        MaxTextureAnisotropy = 0;
    }

    UIDGen = 0;

    Allocator = _Allocator ? *_Allocator : DefaultAllocator;
    Hash = _Hash ? _Hash : SDBM_Hash;
}

void Device::Deinitialize() {

    for ( SamplerInfo * sampler : SamplerCache ) {

        GLuint id = GL_HANDLE( sampler->Handle );
        glDeleteSamplers( 1, &id );

        Allocator.Deallocate( sampler );
    }

    for ( BlendingStateInfo * state : BlendingStateCache ) {
        Allocator.Deallocate( state );
    }

    for ( RasterizerStateInfo * state : RasterizerStateCache ) {
        Allocator.Deallocate( state );
    }

    for ( DepthStencilStateInfo * state : DepthStencilStateCache ) {
        Allocator.Deallocate( state );
    }

    SamplerCache.Free();
    BlendingStateCache.Free();
    RasterizerStateCache.Free();
    DepthStencilStateCache.Free();

    SamplerHash.Free();
    BlendingHash.Free();
    RasterizerHash.Free();
    DepthStencilHash.Free();

    assert( TotalStates == 0 );
    assert( TotalBuffers == 0 );
    assert( TotalTextures == 0 );
    assert( TotalShaderModules == 0 );
}

Sampler const Device::GetOrCreateSampler( SamplerCreateInfo const & _CreateInfo ) {
    int hash = Hash( (unsigned char *)&_CreateInfo, sizeof( _CreateInfo ) );

    int i = SamplerHash.First( hash );
    for ( ; i != -1 ; i = SamplerHash.Next( i ) ) {

        SamplerInfo const * sampler = SamplerCache[i];

        if ( !memcmp( &sampler->CreateInfo, &_CreateInfo, sizeof( sampler->CreateInfo ) ) ) {
            //LogPrintf( "Caching sampler\n" );
            return sampler->Handle;
        }
    }

    SamplerInfo * sampler = static_cast< SamplerInfo * >( Allocator.Allocate( sizeof( SamplerInfo ) ) );
    memcpy( &sampler->CreateInfo, &_CreateInfo, sizeof( sampler->CreateInfo ) );

    i = SamplerCache.Size();

    SamplerHash.Insert( hash, i );
    SamplerCache.Append( sampler );

    //LogPrintf( "Total samplers %d\n", i+1 );

    // 3.3 or GL_ARB_sampler_objects

    GLuint id;

    glCreateSamplers( 1, &id ); // 4.5

    glSamplerParameteri( id, GL_TEXTURE_MIN_FILTER, SamplerFilterModeLUT[_CreateInfo.Filter].Min );
    glSamplerParameteri( id, GL_TEXTURE_MAG_FILTER, SamplerFilterModeLUT[_CreateInfo.Filter].Mag );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_S, SamplerAddressModeLUT[_CreateInfo.AddressU] );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_T, SamplerAddressModeLUT[_CreateInfo.AddressV] );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_R, SamplerAddressModeLUT[_CreateInfo.AddressW] );
    glSamplerParameterf( id, GL_TEXTURE_LOD_BIAS, _CreateInfo.MipLODBias );
    if ( IsTextureAnisotropySupported() ) {
        glSamplerParameteri( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp< unsigned int >( _CreateInfo.MaxAnisotropy, 0u, MaxTextureAnisotropy ) );
    }
    if ( _CreateInfo.bCompareRefToTexture ) {
        glSamplerParameteri( id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    }
    glSamplerParameteri( id, GL_TEXTURE_COMPARE_FUNC, ComparisonFuncLUT[_CreateInfo.ComparisonFunc] );
    glSamplerParameterfv( id, GL_TEXTURE_BORDER_COLOR, _CreateInfo.BorderColor );
    glSamplerParameterf( id, GL_TEXTURE_MIN_LOD, _CreateInfo.MinLOD );
    glSamplerParameterf( id, GL_TEXTURE_MAX_LOD, _CreateInfo.MaxLOD );
    glSamplerParameteri( id, GL_TEXTURE_CUBE_MAP_SEAMLESS, _CreateInfo.bCubemapSeamless );

    // We can use also these functions to retrieve sampler parameters:
    //glGetSamplerParameterfv
    //glGetSamplerParameterIiv
    //glGetSamplerParameterIuiv
    //glGetSamplerParameteriv

    sampler->Handle = (void *)(size_t)id;

    return sampler->Handle;
}

BlendingStateInfo const * Device::CachedBlendingState( BlendingStateInfo const & _BlendingState ) {
    int hash = Hash( ( unsigned char * )&_BlendingState, sizeof( _BlendingState ) );

    int i = BlendingHash.First( hash );
    for ( ; i != -1 ; i = BlendingHash.Next( i ) ) {

        BlendingStateInfo const * state = BlendingStateCache[ i ];

        if ( !memcmp( state, &_BlendingState, sizeof( *state ) ) ) {
            //LogPrintf( "Caching blending state\n" );
            return state;
        }
    }

    BlendingStateInfo * state = static_cast< BlendingStateInfo * >( Allocator.Allocate( sizeof( BlendingStateInfo ) ) );
    memcpy( state, &_BlendingState, sizeof( *state ) );

    i = BlendingStateCache.Size();

    BlendingHash.Insert( hash, i );
    BlendingStateCache.Append( state );

    //LogPrintf( "Total blending states %d\n", i+1 );

    return state;
}

RasterizerStateInfo const * Device::CachedRasterizerState( RasterizerStateInfo const & _RasterizerState ) {
    int hash = Hash( ( unsigned char * )&_RasterizerState, sizeof( _RasterizerState ) );

    int i = RasterizerHash.First( hash );
    for ( ; i != -1 ; i = RasterizerHash.Next( i ) ) {

        RasterizerStateInfo const * state = RasterizerStateCache[ i ];

        if ( !memcmp( state, &_RasterizerState, sizeof( *state ) ) ) {
            //LogPrintf( "Caching rasterizer state\n" );
            return state;
        }
    }

    RasterizerStateInfo * state = static_cast< RasterizerStateInfo * >( Allocator.Allocate( sizeof( RasterizerStateInfo ) ) );
    memcpy( state, &_RasterizerState, sizeof( *state ) );

    i = RasterizerStateCache.Size();

    RasterizerHash.Insert( hash, i );
    RasterizerStateCache.Append( state );

    //LogPrintf( "Total rasterizer states %d\n", i+1 );

    return state;
}

DepthStencilStateInfo const * Device::CachedDepthStencilState( DepthStencilStateInfo const & _DepthStencilState ) {
    int hash = Hash( ( unsigned char * )&_DepthStencilState, sizeof( _DepthStencilState ) );

    int i = DepthStencilHash.First( hash );
    for ( ; i != -1 ; i = DepthStencilHash.Next( i ) ) {

        DepthStencilStateInfo const * state = DepthStencilStateCache[ i ];

        if ( !memcmp( state, &_DepthStencilState, sizeof( *state ) ) ) {
            //LogPrintf( "Caching depth stencil state\n" );
            return state;
        }
    }

    DepthStencilStateInfo * state = static_cast< DepthStencilStateInfo * >( Allocator.Allocate( sizeof( DepthStencilStateInfo ) ) );
    memcpy( state, &_DepthStencilState, sizeof( *state ) );

    i = DepthStencilStateCache.Size();

    DepthStencilHash.Insert( hash, i );
    DepthStencilStateCache.Append( state );

    //LogPrintf( "Total depth stencil states %d\n", i+1 );

    return state;
}

}
