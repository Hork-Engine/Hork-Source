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

#include <Core/Public/CriticalError.h>
#include <Core/Public/WindowsDefs.h>
#include <Core/Public/CoreMath.h>
#include <Core/Public/Logger.h>

#include <Runtime/Public/RuntimeVariable.h>

#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "BufferGLImpl.h"
#include "BufferViewGLImpl.h"
#include "TextureGLImpl.h"
#include "SamplerGLImpl.h"
#include "TransformFeedbackGLImpl.h"
#include "QueryGLImpl.h"
#include "RenderPassGLImpl.h"
#include "PipelineGLImpl.h"
#include "ShaderModuleGLImpl.h"

#include "LUT.h"

#include "GL/glew.h"
#ifdef AN_OS_WIN32
#include "GL/wglew.h"
#endif
#ifdef AN_OS_LINUX
#include "GL/glxew.h"
#endif

#include <SDL.h>

ARuntimeVariable RVSwapInterval( _CTS( "SwapInterval" ), _CTS( "0" ), 0, _CTS( "1 - enable vsync, 0 - disable vsync, -1 - tearing" ) );

namespace RenderCore {

struct SamplerInfo {
    SSamplerCreateInfo CreateInfo;
    void * Handle;
};

template< typename T > T clamp( T const & _a, T const & _min, T const & _max ) {
    return std::max( std::min( _a, _max ), _min );
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

static constexpr SAllocatorCallback DefaultAllocator =
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

static void DebugMessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * userParam ) {
    const char * sourceStr;
    switch ( source ) {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "WINDOW SYSTEM";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "SHADER COMPILER";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "THIRD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "APPLICATION";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "OTHER";
        break;
    default:
        sourceStr = "UNKNOWN";
        break;
    }

    const char * typeStr;
    switch ( type ) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEFINED BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "MISC";
        break;
    case GL_DEBUG_TYPE_MARKER:
        typeStr = "MARKER";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        typeStr = "PUSH GROUP";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        typeStr = "POP GROUP";
        break;
    default:
        typeStr = "UNKNOWN";
        break;
    }

    const char * severityStr;
    switch ( severity ) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityStr = "NOTIFICATION";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;
    default:
        severityStr = "UNKNOWN";
        break;
    }

    GLogger.Printf( "-----------------------------------\n"
                    "%s %s\n"
                    "%s: %s (Id %d)\n"
                    "-----------------------------------\n",
                    sourceStr, typeStr, severityStr, message, id );
}

void CreateLogicalDevice( SImmediateContextCreateInfo const & _CreateInfo,
                          SAllocatorCallback const * _Allocator,
                          HashCallback _Hash,
                          TRef< IDevice > * ppDevice )
{
    *ppDevice = MakeRef< ADeviceGLImpl >( _CreateInfo, _Allocator, _Hash );
}

ADeviceGLImpl::ADeviceGLImpl( SImmediateContextCreateInfo const & _CreateInfo,
                SAllocatorCallback const * _Allocator,
                HashCallback _Hash ) {
    TotalContexts = 0;
    TotalBuffers = 0;
    TotalTextures = 0;
    TotalShaderModules = 0;
    TotalPipelines = 0;
    TotalRenderPasses = 0;
    TotalFramebuffers = 0;
    TotalTransformFeedbacks = 0;
    TotalQueryPools = 0;
    BufferMemoryAllocated = 0;
    TextureMemoryAllocated = 0;

    SDL_GLContext windowCtx = SDL_GL_CreateContext( _CreateInfo.Window );
    if ( !windowCtx ) {
        CriticalError( "Failed to initialize OpenGL context\n" );
    }

    SDL_GL_MakeCurrent( _CreateInfo.Window, windowCtx );

    glewExperimental = true;
    GLenum result = glewInit();
    if ( result != GLEW_OK ) {
        CriticalError( "Failed to load OpenGL functions\n" );
    }

    // GLEW has a long-existing bug where calling glewInit() always sets the GL_INVALID_ENUM error flag and
    // thus the first glGetError will always return an error code which can throw you completely off guard.
    // To fix this it's advised to simply call glGetError after glewInit to clear the flag.
    (void)glGetError();

#ifdef AN_DEBUG
    GLint contextFlags;
    glGetIntegerv( GL_CONTEXT_FLAGS, &contextFlags );
    if ( contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT ) {
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageCallback( DebugMessageCallback, NULL );
    }
#endif

    const char * vendorString = (const char *)glGetString( GL_VENDOR );
    vendorString = vendorString ? vendorString : "Unknown";

    const char * adapterString = (const char *)glGetString( GL_RENDERER );
    adapterString = adapterString ? adapterString : "Unknown";

    const char * driverVersion = (const char *)glGetString( GL_VERSION );
    driverVersion = driverVersion ? driverVersion : "Unknown";

    GLogger.Printf( "Graphics vendor: %s\n", vendorString );
    GLogger.Printf( "Graphics adapter: %s\n", adapterString );
    GLogger.Printf( "Driver version: %s\n", driverVersion );

#if 0
    SMemoryInfo gpuMemoryInfo = GetGPUMemoryInfo();
    if ( gpuMemoryInfo.TotalAvailableMegabytes > 0 && gpuMemoryInfo.CurrentAvailableMegabytes > 0 ) {
        GLogger.Printf( "Total available GPU memory: %d Megs\n", gpuMemoryInfo.TotalAvailableMegabytes );
        GLogger.Printf( "Current available GPU memory: %d Megs\n", gpuMemoryInfo.CurrentAvailableMegabytes );
    }
#endif

#if defined( AN_OS_WIN32 )
    FeatureSupport[FEATURE_SWAP_CONTROL] = !!WGLEW_EXT_swap_control;
    FeatureSupport[FEATURE_SWAP_CONTROL_TEAR] = !!WGLEW_EXT_swap_control_tear;
#elif defined( AN_OS_LINUX )
    FeatureSupport[FEATURE_SWAP_CONTROL] = !!GLXEW_EXT_swap_control || !!GLXEW_MESA_swap_control || !!GLXEW_SGI_swap_control;
    FeatureSupport[FEATURE_SWAP_CONTROL_TEAR] = !!GLXEW_EXT_swap_control_tear;
#else
#error "Swap control tear checking not implemented on current platform"
#endif

    bool NV_half_float = FindExtension( "GL_NV_half_float" );

    FeatureSupport[FEATURE_HALF_FLOAT_VERTEX] = FindExtension( "GL_ARB_half_float_vertex" ) || NV_half_float;
    FeatureSupport[FEATURE_HALF_FLOAT_PIXEL] = FindExtension( "GL_ARB_half_float_pixel" ) || NV_half_float;
    FeatureSupport[FEATURE_TEXTURE_COMPRESSION_S3TC] = FindExtension( "GL_EXT_texture_compression_s3tc" );
    FeatureSupport[FEATURE_TEXTURE_ANISOTROPY] = FindExtension( "GL_EXT_texture_filter_anisotropic" );

    MaxVertexBufferSlots = GL_GetInteger( GL_MAX_VERTEX_ATTRIB_BINDINGS ); // TODO: check if 0 ???

    // GL_MAX_VERTEX_ATTRIB_STRIDE sinse GL v4.4
    MaxVertexAttribStride = GL_GetInteger( GL_MAX_VERTEX_ATTRIB_STRIDE );
    //if ( GL vertsion < 4.4 ) {
    //    storage->MaxVertexAttribStride = 2048;
    //}
    MaxVertexAttribRelativeOffset = GL_GetInteger( GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET );

    MaxCombinedTextureImageUnits = GL_GetInteger( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS );

    //MaxVertexTextureImageUnits = GL_GetInteger( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS );

    MaxImageUnits = GL_GetInteger( GL_MAX_IMAGE_UNITS );

    DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE] = GL_GetInteger( GL_MAX_TEXTURE_BUFFER_SIZE );

    DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT] = GL_GetInteger( GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT );
    if ( !DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT] ) {
        GLogger.Printf( "Warning: TextureBufferOffsetAlignment == 0, using default alignment (256)\n" );
        DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT] = 256;
    }

    DeviceCaps[DEVICE_CAPS_UNIFORM_BUFFER_OFFSET_ALIGNMENT] = GL_GetInteger( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT );
    if ( !DeviceCaps[DEVICE_CAPS_UNIFORM_BUFFER_OFFSET_ALIGNMENT] ) {
        GLogger.Printf( "Warning: UniformBufferOffsetAlignment == 0, using default alignment (256)\n" );
        DeviceCaps[DEVICE_CAPS_UNIFORM_BUFFER_OFFSET_ALIGNMENT] = 256;
    }

    DeviceCaps[DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT] = GL_GetInteger( GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT );
    if ( !DeviceCaps[DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT] ) {
        GLogger.Printf( "Warning: ShaderStorageBufferOffsetAlignment == 0, using default alignment (256)\n" );
        DeviceCaps[DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT] = 256;
    }

    MaxBufferBindings[UNIFORM_BUFFER] = GL_GetInteger( GL_MAX_UNIFORM_BUFFER_BINDINGS );
    MaxBufferBindings[SHADER_STORAGE_BUFFER] = GL_GetInteger( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS );
    MaxBufferBindings[TRANSFORM_FEEDBACK_BUFFER] = GL_GetInteger( GL_MAX_TRANSFORM_FEEDBACK_BUFFERS );
    MaxBufferBindings[ATOMIC_COUNTER_BUFFER] = GL_GetInteger( GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS );

    if ( FeatureSupport[FEATURE_TEXTURE_ANISOTROPY] ) {
        MaxTextureAnisotropy = (unsigned int)GL_GetFloat( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT );
    } else {
        MaxTextureAnisotropy = 0;
    }

    DeviceCaps[DEVICE_CAPS_MAX_TEXTURE_SIZE] = GL_GetInteger( GL_MAX_TEXTURE_SIZE );

    Allocator = _Allocator ? *_Allocator : DefaultAllocator;
    Hash = _Hash ? _Hash : SDBM_Hash;

    TotalContexts++;
    pMainContext = new AImmediateContextGLImpl( this, _CreateInfo, windowCtx );

    // Mark swap interval modified to set initial swap interval.
    RVSwapInterval.MarkModified();

    // Clear garbage on screen, set initial swap interval and swap the buffers.
    glClearColor( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT );
    SwapBuffers( _CreateInfo.Window );
}

ADeviceGLImpl::~ADeviceGLImpl()
{
    for ( SamplerInfo * sampler : SamplerCache ) {

        GLuint id = GL_HANDLE( sampler->Handle );
        glDeleteSamplers( 1, &id );

        Allocator.Deallocate( sampler );
    }

    for ( SBlendingStateInfo * state : BlendingStateCache ) {
        Allocator.Deallocate( state );
    }

    for ( SRasterizerStateInfo * state : RasterizerStateCache ) {
        Allocator.Deallocate( state );
    }

    for ( SDepthStencilStateInfo * state : DepthStencilStateCache ) {
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

    ReleaseImmediateContext( pMainContext );
    pMainContext = nullptr;

    AN_ASSERT( TotalContexts == 0 );
    AN_ASSERT( TotalBuffers == 0 );
    AN_ASSERT( TotalTextures == 0 );
    AN_ASSERT( TotalShaderModules == 0 );
}

void ADeviceGLImpl::SwapBuffers( SDL_Window * WindowHandle ) {
    if ( RVSwapInterval.IsModified() ) {
        int i = Math::Clamp( RVSwapInterval.GetInteger(), -1, 1 );
        if ( i == -1 && !FeatureSupport[FEATURE_SWAP_CONTROL_TEAR] ) {
            // Tearing not supported
            i = 0;
        }

        GLogger.Printf( "Changing swap interval to %d\n", i );

        SDL_GL_SetSwapInterval( i );

        RVSwapInterval.ForceInteger( i );
        RVSwapInterval.UnmarkModified();
    }

    SDL_GL_SwapWindow( WindowHandle );
}

void ADeviceGLImpl::GetImmediateContext( IImmediateContext ** ppImmediateContext )
{
    *ppImmediateContext = pMainContext;
}

void ADeviceGLImpl::CreateImmediateContext( SImmediateContextCreateInfo const & _CreateInfo, IImmediateContext ** ppImmediateContext )
{
    TotalContexts++;
    *ppImmediateContext = new AImmediateContextGLImpl( this, _CreateInfo, nullptr );
}

void ADeviceGLImpl::ReleaseImmediateContext( IImmediateContext * pImmediateContext )
{
    if ( pImmediateContext ) {
        TotalContexts--;
        delete pImmediateContext;
    }
}

void ADeviceGLImpl::CreateFramebuffer( SFramebufferCreateInfo const & _CreateInfo, TRef< IFramebuffer > * ppFramebuffer )
{
    *ppFramebuffer = MakeRef< AFramebufferGLImpl >( this, _CreateInfo, false );
}

void ADeviceGLImpl::CreateRenderPass( SRenderPassCreateInfo const & _CreateInfo, TRef< IRenderPass > * ppRenderPass )
{
    *ppRenderPass = MakeRef< ARenderPassGLImpl >( this, _CreateInfo );
}

void ADeviceGLImpl::CreatePipeline( SPipelineCreateInfo const & _CreateInfo, TRef< IPipeline > * ppPipeline )
{
    *ppPipeline = MakeRef< APipelineGLImpl >( this, _CreateInfo );
}

void ADeviceGLImpl::CreateShaderFromBinary( SShaderBinaryData const * _BinaryData, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule )
{
    *ppShaderModule = MakeRef< AShaderModuleGLImpl >( this, _BinaryData, _InfoLog );
}

void ADeviceGLImpl::CreateShaderFromCode( SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule )
{
    *ppShaderModule = MakeRef< AShaderModuleGLImpl >( this, _ShaderType, _NumSources, _Sources, _InfoLog );
}

void ADeviceGLImpl::CreateShaderFromCode( SHADER_TYPE _ShaderType, const char * _Source, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule )
{
    *ppShaderModule = MakeRef< AShaderModuleGLImpl >( this, _ShaderType, _Source, _InfoLog );
}

void ADeviceGLImpl::CreateBuffer( SBufferCreateInfo const & _CreateInfo, const void * _SysMem, TRef< IBuffer > * ppBuffer )
{
    *ppBuffer = MakeRef< ABufferGLImpl >( this, _CreateInfo, _SysMem );
}

void ADeviceGLImpl::CreateBufferView( SBufferViewCreateInfo const & CreateInfo, TRef< IBuffer > pBuffer, TRef< IBufferView > * ppBufferView )
{
    *ppBufferView = MakeRef< ABufferViewGLImpl >( this, CreateInfo, static_cast< ABufferGLImpl * >( pBuffer.GetObject() ) );
}

void ADeviceGLImpl::CreateTexture( STextureCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture )
{
    *ppTexture = MakeRef< ATextureGLImpl >( this, _CreateInfo );
}

void ADeviceGLImpl::CreateTextureView( STextureViewCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture )
{
    *ppTexture = MakeRef< ATextureGLImpl >( this, _CreateInfo );
}

void ADeviceGLImpl::CreateTransformFeedback( STransformFeedbackCreateInfo const & _CreateInfo, TRef< ITransformFeedback > * ppTransformFeedback )
{
    *ppTransformFeedback = MakeRef< ATransformFeedbackGLImpl >( this, _CreateInfo );
}

void ADeviceGLImpl::CreateQueryPool( SQueryPoolCreateInfo const & _CreateInfo, TRef< IQueryPool > * ppQueryPool )
{
    *ppQueryPool = MakeRef< AQueryPoolGLImpl >( this, _CreateInfo );
}

void ADeviceGLImpl::CreateBindlessSampler( ITexture * pTexture, Sampler Sampler, TRef< IBindlessSampler > * ppBindlessSampler )
{
    *ppBindlessSampler = MakeRef< ABindlessSamplerGLImpl >( pTexture, Sampler );
}

void ADeviceGLImpl::GetOrCreateSampler( struct SSamplerCreateInfo const & _CreateInfo, Sampler * ppSampler )
{
    int hash = Hash( (unsigned char *)&_CreateInfo, sizeof( _CreateInfo ) );

    int i = SamplerHash.First( hash );
    for ( ; i != -1 ; i = SamplerHash.Next( i ) ) {

        SamplerInfo const * sampler = SamplerCache[i];

        if ( !memcmp( &sampler->CreateInfo, &_CreateInfo, sizeof( sampler->CreateInfo ) ) ) {
            //GLogger.Printf( "Caching sampler\n" );
            *ppSampler = sampler->Handle;
            return;
        }
    }

    SamplerInfo * sampler = static_cast< SamplerInfo * >( Allocator.Allocate( sizeof( SamplerInfo ) ) );
    memcpy( &sampler->CreateInfo, &_CreateInfo, sizeof( sampler->CreateInfo ) );

    i = SamplerCache.Size();

    SamplerHash.Insert( hash, i );
    SamplerCache.Append( sampler );

    //GLogger.Printf( "Total samplers %d\n", i+1 );

    // 3.3 or GL_ARB_sampler_objects

    GLuint id;

    glCreateSamplers( 1, &id ); // 4.5

    glSamplerParameteri( id, GL_TEXTURE_MIN_FILTER, SamplerFilterModeLUT[_CreateInfo.Filter].Min );
    glSamplerParameteri( id, GL_TEXTURE_MAG_FILTER, SamplerFilterModeLUT[_CreateInfo.Filter].Mag );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_S, SamplerAddressModeLUT[_CreateInfo.AddressU] );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_T, SamplerAddressModeLUT[_CreateInfo.AddressV] );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_R, SamplerAddressModeLUT[_CreateInfo.AddressW] );
    glSamplerParameterf( id, GL_TEXTURE_LOD_BIAS, _CreateInfo.MipLODBias );
    if ( FeatureSupport[FEATURE_TEXTURE_ANISOTROPY] && _CreateInfo.MaxAnisotropy > 0 ) {
        glSamplerParameteri( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp< unsigned int >( _CreateInfo.MaxAnisotropy, 1u, MaxTextureAnisotropy ) );
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

    *ppSampler = sampler->Handle;
}

unsigned int ADeviceGLImpl::CreateShaderProgram( unsigned int _Type,
                                          int _NumStrings,
                                          const char * const * _Strings,
                                          char ** _InfoLog ) {
    static char ErrorLog[ MAX_ERROR_LOG_LENGTH ];
    GLuint program;

    ErrorLog[0] = 0;

    if ( _InfoLog ) {
        *_InfoLog = ErrorLog;
    }

#if 1
    program = glCreateShaderProgramv( _Type, _NumStrings, _Strings );  // v 4.1
    if ( program ) {
        glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
    }

#else
    // Other path
    GLuint shader = glCreateShader( _Type );
    if ( shader ) {
        glShaderSource( shader, _NumStrings, _Strings, NULL );
        glCompileShader( shader );
        program = glCreateProgram();
        if ( program ) {
            GLint compiled = GL_FALSE;
            glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
            glProgramParameteri( program, GL_PROGRAM_SEPARABLE, GL_TRUE );
            glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
            if ( compiled ) {
                glAttachShader( program, shader );
                glLinkProgram( program );
                glDetachShader( program, shader );
            }
            /* append-shader-info-log-to-program-info-log */
        }
        glDeleteShader( shader );
    } else {
        program = 0;
    }
#endif

    if ( !program ) {

        if ( _InfoLog ) {
            Core::Strcpy( ErrorLog, sizeof( ErrorLog ), "Failed to create shader program" );
        }

        return 0;
    }

    GLint linkStatus = 0;
    glGetProgramiv( program, GL_LINK_STATUS, &linkStatus );
    if ( !linkStatus ) {

        if ( _InfoLog ) {
            GLint infoLogLength = 0;
            glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infoLogLength );
            glGetProgramInfoLog( program, sizeof( ErrorLog ), NULL, ErrorLog );

            if ( (GLint)sizeof( ErrorLog ) < infoLogLength ) {
                ErrorLog[ sizeof( ErrorLog ) - 4 ] = '.';
                ErrorLog[ sizeof( ErrorLog ) - 3 ] = '.';
                ErrorLog[ sizeof( ErrorLog ) - 2 ] = '.';
            }
        }

        glDeleteProgram( program );

        return 0;
    }

    return program;
}

void ADeviceGLImpl::DeleteShaderProgram( unsigned int _Program ) {
    glDeleteProgram( _Program );
}

bool ADeviceGLImpl::CreateShaderBinaryData( SHADER_TYPE _ShaderType,
                                     unsigned int _NumSources,
                                     const char * const * _Sources,
                                     /* optional */ char ** _InfoLog,
                                     SShaderBinaryData * _BinaryData )
{
    GLuint id;
    GLsizei binaryLength;
    GLsizei length;
    GLenum format;
    uint8_t * binary;

    Core::ZeroMem( _BinaryData, sizeof( *_BinaryData ) );

    id = CreateShaderProgram( ShaderTypeLUT[ _ShaderType ], _NumSources, _Sources, _InfoLog );
    if ( !id ) {
        GLogger.Printf( "Device::CreateShaderBinaryData: couldn't create shader program\n" );
        return false;
    }

    glGetProgramiv( id, GL_PROGRAM_BINARY_LENGTH, &binaryLength );
    if ( binaryLength ) {
        binary = ( uint8_t * )Allocator.Allocate( binaryLength );

        glGetProgramBinary( id, binaryLength, &length, &format, binary );

        _BinaryData->BinaryCode = binary;
        _BinaryData->BinaryLength = length;
        _BinaryData->BinaryFormat = format;
        _BinaryData->ShaderType = _ShaderType;
    } else {
        if ( _InfoLog ) {
            Core::Strcpy( *_InfoLog, MAX_ERROR_LOG_LENGTH, "Failed to retrieve shader program binary length" );
        }

        DeleteShaderProgram( id );
        return false;
    }

    DeleteShaderProgram( id );
    return true;
}

void ADeviceGLImpl::DestroyShaderBinaryData( SShaderBinaryData * _BinaryData )
{
    if ( !_BinaryData->BinaryCode ) {
        return;
    }

    uint8_t * binary = ( uint8_t * )_BinaryData->BinaryCode;
    Allocator.Deallocate( binary );

    Core::ZeroMem( _BinaryData, sizeof( *_BinaryData ) );
}

SAllocatorCallback const & ADeviceGLImpl::GetAllocator() const
{
    return Allocator;
}

bool ADeviceGLImpl::IsFeatureSupported( FEATURE_TYPE InFeatureType )
{
    return FeatureSupport[InFeatureType];
}

unsigned int ADeviceGLImpl::GetDeviceCaps( DEVICE_CAPS InDeviceCaps )
{
    return DeviceCaps[InDeviceCaps];
}


SBlendingStateInfo const * ADeviceGLImpl::CachedBlendingState( SBlendingStateInfo const & _BlendingState ) {
    int hash = Hash( ( unsigned char * )&_BlendingState, sizeof( _BlendingState ) );

    int i = BlendingHash.First( hash );
    for ( ; i != -1 ; i = BlendingHash.Next( i ) ) {

        SBlendingStateInfo const * state = BlendingStateCache[ i ];

        if ( !memcmp( state, &_BlendingState, sizeof( *state ) ) ) {
            //GLogger.Printf( "Caching blending state\n" );
            return state;
        }
    }

    SBlendingStateInfo * state = static_cast< SBlendingStateInfo * >( Allocator.Allocate( sizeof( SBlendingStateInfo ) ) );
    memcpy( state, &_BlendingState, sizeof( *state ) );

    i = BlendingStateCache.Size();

    BlendingHash.Insert( hash, i );
    BlendingStateCache.Append( state );

    //GLogger.Printf( "Total blending states %d\n", i+1 );

    return state;
}

SRasterizerStateInfo const * ADeviceGLImpl::CachedRasterizerState( SRasterizerStateInfo const & _RasterizerState ) {
    int hash = Hash( ( unsigned char * )&_RasterizerState, sizeof( _RasterizerState ) );

    int i = RasterizerHash.First( hash );
    for ( ; i != -1 ; i = RasterizerHash.Next( i ) ) {

        SRasterizerStateInfo const * state = RasterizerStateCache[ i ];

        if ( !memcmp( state, &_RasterizerState, sizeof( *state ) ) ) {
            //GLogger.Printf( "Caching rasterizer state\n" );
            return state;
        }
    }

    SRasterizerStateInfo * state = static_cast< SRasterizerStateInfo * >( Allocator.Allocate( sizeof( SRasterizerStateInfo ) ) );
    memcpy( state, &_RasterizerState, sizeof( *state ) );

    i = RasterizerStateCache.Size();

    RasterizerHash.Insert( hash, i );
    RasterizerStateCache.Append( state );

    //GLogger.Printf( "Total rasterizer states %d\n", i+1 );

    return state;
}

SDepthStencilStateInfo const * ADeviceGLImpl::CachedDepthStencilState( SDepthStencilStateInfo const & _DepthStencilState ) {
    int hash = Hash( ( unsigned char * )&_DepthStencilState, sizeof( _DepthStencilState ) );

    int i = DepthStencilHash.First( hash );
    for ( ; i != -1 ; i = DepthStencilHash.Next( i ) ) {

        SDepthStencilStateInfo const * state = DepthStencilStateCache[ i ];

        if ( !memcmp( state, &_DepthStencilState, sizeof( *state ) ) ) {
            //GLogger.Printf( "Caching depth stencil state\n" );
            return state;
        }
    }

    SDepthStencilStateInfo * state = static_cast< SDepthStencilStateInfo * >( Allocator.Allocate( sizeof( SDepthStencilStateInfo ) ) );
    memcpy( state, &_DepthStencilState, sizeof( *state ) );

    i = DepthStencilStateCache.Size();

    DepthStencilHash.Insert( hash, i );
    DepthStencilStateCache.Append( state );

    //GLogger.Printf( "Total depth stencil states %d\n", i+1 );

    return state;
}

}
