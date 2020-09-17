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

#include "ImmediateContextGLImpl.h"
#include "DeviceGLImpl.h"
#include "BufferGLImpl.h"
#include "TextureGLImpl.h"
#include "PipelineGLImpl.h"
#include "TransformFeedbackGLImpl.h"
#include "QueryGLImpl.h"
#include "RenderPassGLImpl.h"
#include "VertexArrayObjectGL.h"
#include "LUT.h"

#include <Core/Public/CriticalError.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Core/Public/CoreMath.h>
#include <Core/Public/Logger.h>

#include "GL/glew.h"

#include <SDL.h>

#define DEFAULT_STENCIL_REF 0

#define VerifyContext() AN_ASSERT( AImmediateContextGLImpl::Current == this );

namespace RenderCore {

AImmediateContextGLImpl * AImmediateContextGLImpl::StateHead = nullptr, *AImmediateContextGLImpl::StateTail = nullptr;

AResourceTableGLImpl::AResourceTableGLImpl( ADeviceGLImpl * _Device )
    : pDevice( _Device )
{
    memset( TextureBindings, 0, sizeof( TextureBindings ) );
    memset( TextureBindingUIDs, 0, sizeof( TextureBindingUIDs ) );
    memset( ImageBindings, 0, sizeof( ImageBindings ) );
    memset( ImageBindingUIDs, 0, sizeof( ImageBindingUIDs ) );
    memset( ImageLod, 0, sizeof( ImageLod ) );
    memset( ImageLayerIndex, 0, sizeof( ImageLayerIndex ) );
    memset( ImageLayered, 0, sizeof( ImageLayered ) );
    memset( BufferBindings, 0, sizeof( BufferBindings ) );
    memset( BufferBindingUIDs, 0, sizeof( BufferBindingUIDs ) );
    memset( BufferBindingOffsets, 0, sizeof( BufferBindingOffsets ) );
    memset( BufferBindingSizes, 0, sizeof( BufferBindingSizes ) );
}

AResourceTableGLImpl::~AResourceTableGLImpl()
{
}

void AResourceTableGLImpl::BindTexture( unsigned int Slot, ITextureBase const * Texture )
{
    AN_ASSERT( Slot < MAX_SAMPLER_SLOTS );

    // Slot must be < pDevice->MaxCombinedTextureImageUnits

    if ( Texture ) {
        TextureBindings[Slot] = Texture->GetHandleNativeGL();
        TextureBindingUIDs[Slot] = Texture->GetUID();
    }
    else {
        TextureBindings[Slot] = 0;
        TextureBindingUIDs[Slot] = 0;
    }
}

void AResourceTableGLImpl::BindImage( unsigned int Slot, ITextureBase const * Texture, uint16_t Lod, bool bLayered, uint16_t LayerIndex )
{
    AN_ASSERT( Slot < MAX_IMAGE_SLOTS );

    // _Slot must be < pDevice->MaxCombinedTextureImageUnits

    if ( Texture ) {
        ImageBindings[Slot] = Texture->GetHandleNativeGL();
        ImageBindingUIDs[Slot] = Texture->GetUID();
        ImageLod[Slot] = Lod;
        ImageLayerIndex[Slot] = LayerIndex;
        ImageLayered[Slot] = bLayered;
    }
    else {
        ImageBindings[Slot] = 0;
        ImageBindingUIDs[Slot] = 0;
        ImageLod[Slot] = 0;
        ImageLayerIndex[Slot] = 0;
        ImageLayered[Slot] = false;
    }
}

void AResourceTableGLImpl::BindBuffer( int Slot, IBuffer const * Buffer, size_t Offset, size_t Size )
{
    AN_ASSERT( Slot < MAX_BUFFER_SLOTS );

    if ( Buffer ) {
        BufferBindings[Slot] = Buffer->GetHandleNativeGL();
        BufferBindingUIDs[Slot] = Buffer->GetUID();
        BufferBindingOffsets[Slot] = Offset;
        BufferBindingSizes[Slot] = Size;
    }
    else {
        BufferBindings[Slot] = 0;
        BufferBindingUIDs[Slot] = 0;
        BufferBindingOffsets[Slot] = 0;
        BufferBindingSizes[Slot] = 0;
    }
}

AImmediateContextGLImpl::AImmediateContextGLImpl( ADeviceGLImpl * _Device, SImmediateContextCreateInfo const & _CreateInfo, void * _Context )
    : pDevice( _Device )
{
    pWindow = _CreateInfo.Window;
    pContextGL = _Context;

    if ( !pContextGL ) {
        pContextGL = SDL_GL_CreateContext( pWindow );
        if ( !pContextGL ) {
            CriticalError( "Failed to initialize OpenGL context\n" );
        }

        SDL_GL_MakeCurrent( pWindow, pContextGL );

        glewExperimental = true;
        GLenum result = glewInit();
        if ( result != GLEW_OK ) {
            CriticalError( "Failed to load OpenGL functions\n" );
        }

        // GLEW has a long-existing bug where calling glewInit() always sets the GL_INVALID_ENUM error flag and
        // thus the first glGetError will always return an error code which can throw you completely off guard.
        // To fix this it's advised to simply call glGetError after glewInit to clear the flag.
        (void)glGetError();
    }

    Current = this;

    memset( BufferBindingUIDs, 0, sizeof( BufferBindingUIDs ) );
    memset( BufferBindingOffsets, 0, sizeof( BufferBindingOffsets ) );
    memset( BufferBindingSizes, 0, sizeof( BufferBindingSizes ) );

    CurrentPipeline = nullptr;
    CurrentVAO = nullptr;
    NumPatchVertices = 0;
    IndexBufferType = 0;
    IndexBufferTypeSizeOf = 0;
    IndexBufferOffset = 0;
    IndexBufferUID = 0;
    IndexBufferHandle = 0;
    memset( VertexBufferUIDs, 0, sizeof( VertexBufferUIDs ) );
    memset( VertexBufferHandles, 0, sizeof( VertexBufferHandles ) );    
    memset( VertexBufferOffsets, 0, sizeof( VertexBufferOffsets ) );

    //CurrentQueryTarget = 0;
    //CurrentQueryObject = 0;
    memset( CurrentQueryUID, 0, sizeof( CurrentQueryUID ) );

    // GL_NICEST, GL_FASTEST and GL_DONT_CARE

    // Sampling quality of antialiased lines during rasterization stage
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

    // Sampling quality of antialiased polygons during rasterization stage
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    // Quality and performance of the compressing texture images
    glHint( GL_TEXTURE_COMPRESSION_HINT, GL_NICEST );

    // Accuracy of the derivative calculation for the GLSL fragment processing built-in functions: dFdx, dFdy, and fwidth.
    glHint( GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST );

    // If enabled, cubemap textures are sampled such that when linearly sampling from the border between two adjacent faces,
    // texels from both faces are used to generate the final sample value.
    // When disabled, texels from only a single face are used to construct the final sample value.
    glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );

    PixelStore.PackAlignment = 4;
    glPixelStorei( GL_PACK_ALIGNMENT, PixelStore.PackAlignment );
    PixelStore.UnpackAlignment = 4;
    glPixelStorei( GL_UNPACK_ALIGNMENT, PixelStore.UnpackAlignment );

    memset( &Binding, 0, sizeof( Binding ) );

    // Init default blending state
    bLogicOpEnabled = false;
    glColorMask( 1,1,1,1 );
    glDisable( GL_BLEND );
    glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
    glBlendFunc( GL_ONE, GL_ZERO );
    glBlendEquation( GL_FUNC_ADD );
    glBlendColor( 0, 0, 0, 0 );
    glDisable( GL_COLOR_LOGIC_OP );
    glLogicOp( GL_COPY );
    memset( BlendColor, 0, sizeof( BlendColor ) );

    GLint maxSampleMaskWords = 0;
    glGetIntegerv( GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords );
    if ( maxSampleMaskWords > 4 ) {
        maxSampleMaskWords = 4;
    }    
    SampleMask[ 0 ] = 0xffffffff;
    SampleMask[ 1 ] = 0;
    SampleMask[ 2 ] = 0;
    SampleMask[ 3 ] = 0;
    for ( int i = 0 ; i < maxSampleMaskWords ; i++ ) {
        glSampleMaski( i, SampleMask[ i ] );
    }
    bSampleMaskEnabled = false;
    glDisable( GL_SAMPLE_MASK );

    // Init default rasterizer state
    glDisable( GL_POLYGON_OFFSET_FILL );
    PolygonOffsetClampSafe( 0, 0, 0 );
    glDisable( GL_DEPTH_CLAMP );
    glDisable( GL_LINE_SMOOTH );
    glDisable( GL_RASTERIZER_DISCARD );
    glDisable( GL_MULTISAMPLE );
    glDisable( GL_SCISSOR_TEST );
    glEnable( GL_CULL_FACE );
    CullFace = GL_BACK;
    glCullFace( CullFace );
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glFrontFace( GL_CCW );
    // GL_POLYGON_SMOOTH
    // If enabled, draw polygons with proper filtering. Otherwise, draw aliased polygons.
    // For correct antialiased polygons, an alpha buffer is needed and the polygons must be sorted front to back.
    glDisable( GL_POLYGON_SMOOTH ); // Smooth polygons have some artifacts
    bPolygonOffsetEnabled = false;

    // Init default depth-stencil state
    StencilRef = DEFAULT_STENCIL_REF;
    glEnable( GL_DEPTH_TEST );
    glDepthMask( 1 );
    glDepthFunc( GL_LESS );
    glDisable( GL_STENCIL_TEST );
    glStencilMask( DEFAULT_STENCIL_WRITE_MASK );
    glStencilOpSeparate( GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP );
    glStencilFuncSeparate( GL_FRONT_AND_BACK, GL_ALWAYS, StencilRef, DEFAULT_STENCIL_READ_MASK );

    ColorClamp = COLOR_CLAMP_OFF;
    glClampColor( GL_CLAMP_READ_COLOR, GL_FALSE );

    glEnable( GL_FRAMEBUFFER_SRGB );

    bPrimitiveRestartEnabled = false;

    CurrentRenderPass = nullptr;

    SwapChainWidth = 512;
    SwapChainHeight = 512;

    CurrentViewport[0] = Math::MaxValue< float >();
    CurrentViewport[1] = Math::MaxValue< float >();
    CurrentViewport[2] = Math::MaxValue< float >();
    CurrentViewport[3] = Math::MaxValue< float >();

    CurrentDepthRange[0] = 0;
    CurrentDepthRange[1] = 1;
    glDepthRangef( CurrentDepthRange[0], CurrentDepthRange[1] ); // Since GL v4.1

    CurrentScissor.X = 0;
    CurrentScissor.Y = 0;
    CurrentScissor.Width = 0;
    CurrentScissor.Height = 0;

    if ( _CreateInfo.ClipControl == CLIP_CONTROL_OPENGL ) {
        // OpenGL Classic ndc_z -1,-1, lower-left corner
        glClipControl( GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE ); // Zw = ( ( f - n ) / 2 ) * Zd + ( n + f ) / 2
    } else {
        // DirectX ndc_z 0,1, upper-left corner
        glClipControl( GL_UPPER_LEFT, GL_ZERO_TO_ONE );         // Zw = ( f - n ) * Zd + n
    }

    ClipControl = _CreateInfo.ClipControl;
    ViewportOrigin = _CreateInfo.ViewportOrigin;

    INTRUSIVE_ADD( this, Next, Prev, StateHead, StateTail );

    SFramebufferCreateInfo framebufferCI = {};
    DefaultFramebuffer = new AFramebufferGLImpl( pDevice, framebufferCI, true );

    Binding.ReadFramebufferUID = DefaultFramebuffer->GetUID();
    Binding.DrawFramebufferUID = DefaultFramebuffer->GetUID();

    pDevice->CreateResourceTable( &RootResourceTable );

    CurrentResourceTable = static_cast< AResourceTableGLImpl *>( RootResourceTable.GetObject() );
}

void AImmediateContextGLImpl::MakeCurrent()
{
    SDL_GL_MakeCurrent( pWindow, pContextGL );
    Current = this;
}

AImmediateContextGLImpl::~AImmediateContextGLImpl()
{
    VerifyContext();

    CurrentResourceTable.Reset();
    RootResourceTable.Reset();

    DefaultFramebuffer.Reset();

    SAllocatorCallback const & allocator = pDevice->GetAllocator();

    glBindVertexArray( 0 );
    for ( SVertexArrayObject * vao : VAOCache ) {
        glDeleteVertexArrays( 1, &vao->Handle );
        allocator.Deallocate( vao );
    }
    VAOCache.Free();
    VAOHash.Free();

    INTRUSIVE_REMOVE( this, Next, Prev, StateHead, StateTail );

    SDL_GL_DeleteContext( pContextGL );
    if ( Current == this ) {
        Current = nullptr;
    }
}

void AImmediateContextGLImpl::SetSwapChainResolution( int _Width, int _Height )
{
    SwapChainWidth = _Width;
    SwapChainHeight = _Height;

    if ( Binding.DrawFramebufferUID == DefaultFramebuffer->GetUID() ) {
        Binding.DrawFramebufferWidth = ( unsigned short )SwapChainWidth;
        Binding.DrawFramebufferHeight = ( unsigned short )SwapChainHeight;
    }
}

void AImmediateContextGLImpl::PolygonOffsetClampSafe( float _Slope, int _Bias, float _Clamp )
{
    VerifyContext();

    const float DepthBiasTolerance = 0.00001f;

    if ( std::fabs( _Slope ) < DepthBiasTolerance
        && std::fabs( _Clamp ) < DepthBiasTolerance
        && _Bias == 0 ) {

        // FIXME: GL_POLYGON_OFFSET_LINE,GL_POLYGON_OFFSET_POINT тоже enable/disable?

        if ( bPolygonOffsetEnabled ) {
            glDisable( GL_POLYGON_OFFSET_FILL );
            bPolygonOffsetEnabled = false;
        }
    } else {
        if ( !bPolygonOffsetEnabled ) {
            glEnable( GL_POLYGON_OFFSET_FILL );
            bPolygonOffsetEnabled = true;
        }
    }

    if ( glPolygonOffsetClampEXT ) {
        glPolygonOffsetClampEXT( _Slope, static_cast< float >( _Bias ), _Clamp );
    } else {
        glPolygonOffset( _Slope, static_cast< float >( _Bias ) );
    }
}

void AImmediateContextGLImpl::PackAlignment( unsigned int _Alignment )
{
    VerifyContext();

    if ( PixelStore.PackAlignment != _Alignment ) {
        glPixelStorei( GL_PACK_ALIGNMENT, _Alignment );
        PixelStore.PackAlignment = _Alignment;
    }
}

void AImmediateContextGLImpl::UnpackAlignment( unsigned int _Alignment )
{
    VerifyContext();

    if ( PixelStore.UnpackAlignment != _Alignment ) {
        glPixelStorei( GL_UNPACK_ALIGNMENT, _Alignment );
        PixelStore.UnpackAlignment = _Alignment;
    }
}

void AImmediateContextGLImpl::ClampReadColor( COLOR_CLAMP _ColorClamp )
{
    VerifyContext();

    if ( ColorClamp != _ColorClamp ) {
        glClampColor( GL_CLAMP_READ_COLOR, ColorClampLUT[ _ColorClamp ] );
        ColorClamp = _ColorClamp;
    }
}

SVertexArrayObject * AImmediateContextGLImpl::CachedVAO( SVertexBindingInfo const * pVertexBindings,
                                                         uint32_t NumVertexBindings,
                                                         SVertexAttribInfo const * pVertexAttribs,
                                                         uint32_t NumVertexAttribs )
{
    VerifyContext();

    SVertexArrayObjectHashedData hashed;

    memset( &hashed, 0, sizeof( hashed ) );

    hashed.NumVertexBindings = NumVertexBindings;
    if ( hashed.NumVertexBindings > MAX_VERTEX_BINDINGS ) {
        hashed.NumVertexBindings = MAX_VERTEX_BINDINGS;

        GLogger.Printf( "AImmediateContextGLImpl::CachedVAO: NumVertexBindings > MAX_VERTEX_BINDINGS\n" );
    }
    memcpy( hashed.VertexBindings, pVertexBindings, sizeof( hashed.VertexBindings[0] ) * hashed.NumVertexBindings );

    hashed.NumVertexAttribs = NumVertexAttribs;
    if ( hashed.NumVertexAttribs > MAX_VERTEX_ATTRIBS ) {
        hashed.NumVertexAttribs = MAX_VERTEX_ATTRIBS;

        GLogger.Printf( "AImmediateContextGLImpl::CachedVAO: NumVertexAttribs > MAX_VERTEX_ATTRIBS\n" );
    }
    memcpy( hashed.VertexAttribs, pVertexAttribs, sizeof( hashed.VertexAttribs[0] ) * hashed.NumVertexAttribs );

    // Clear semantic name to have proper hash key
    for ( int i = 0 ; i < hashed.NumVertexAttribs ; i++ ) {
        hashed.VertexAttribs[i].SemanticName = nullptr;
    }

    int hash = pDevice->Hash( ( unsigned char * )&hashed, sizeof( hashed ) );

    int i = VAOHash.First( hash );
    for ( ; i != -1 ; i = VAOHash.Next( i ) ) {
        SVertexArrayObject * vao = VAOCache[ i ];

        if ( memcmp( &vao->Hashed, &hashed, sizeof( vao->Hashed ) ) == 0 ) {
            //GLogger.Printf( "Caching VAO\n" );
            return vao;
        }
    }

    SAllocatorCallback const & allocator = pDevice->GetAllocator();

    SVertexArrayObject * vao = static_cast< SVertexArrayObject * >( allocator.Allocate( sizeof( SVertexArrayObject ) ) );

    memcpy( &vao->Hashed, &hashed, sizeof( vao->Hashed ) );

    vao->IndexBufferUID = 0;

    memset( vao->VertexBufferUIDs, 0, sizeof( vao->VertexBufferUIDs ) );
    memset( vao->VertexBufferOffsets, 0, sizeof( vao->VertexBufferOffsets ) );

    i = VAOCache.Size();

    VAOHash.Insert( hash, i );
    VAOCache.Append( vao );

    //GLogger.Printf( "Total VAOs %d\n", i+1 );

    // TODO: For each context create VAO
    glCreateVertexArrays( 1, &vao->Handle );
    if ( !vao->Handle ) {
        GLogger.Printf( "AImmediateContextGLImpl::CachedVAO: couldn't create vertex array object\n" );
        //return nullptr;
    }

    memset( vao->VertexBindingsStrides, 0, sizeof( vao->VertexBindingsStrides ) );
    for ( SVertexBindingInfo const * binding = hashed.VertexBindings ; binding < &hashed.VertexBindings[hashed.NumVertexBindings] ; binding++ ) {
        AN_ASSERT( binding->InputSlot < MAX_VERTEX_BUFFER_SLOTS );

        if ( binding->InputSlot >= pDevice->GetDeviceCaps( DEVICE_CAPS_MAX_VERTEX_BUFFER_SLOTS ) ) {
            GLogger.Printf( "AImmediateContextGLImpl::CachedVAO: binding->InputSlot >= MaxVertexBufferSlots\n" );
        }

        if ( binding->Stride > pDevice->GetDeviceCaps( DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE ) ) {
            GLogger.Printf( "AImmediateContextGLImpl::CachedVAO: binding->Stride > MaxVertexAttribStride\n" );
        }

        vao->VertexBindingsStrides[ binding->InputSlot ] = binding->Stride;
    }

    for ( SVertexAttribInfo const * attrib = hashed.VertexAttribs ; attrib < &hashed.VertexAttribs[hashed.NumVertexAttribs] ; attrib++ ) {

        // glVertexAttribFormat, glVertexAttribBinding, glVertexBindingDivisor - v4.3 or GL_ARB_vertex_attrib_binding

        if ( attrib->Offset > pDevice->GetDeviceCaps( DEVICE_CAPS_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET ) ) {
            GLogger.Printf( "AImmediateContextGLImpl::CachedVAO: attrib offset > MaxVertexAttribRelativeOffset\n" );
        }

        switch ( attrib->Mode ) {
        case VAM_FLOAT:
            glVertexArrayAttribFormat( vao->Handle,
                                       attrib->Location,
                                       attrib->NumComponents(),
                                       VertexAttribTypeLUT[ attrib->TypeOfComponent() ],
                                       attrib->IsNormalized(),
                                       attrib->Offset );
            break;
        case VAM_DOUBLE:
            glVertexArrayAttribLFormat( vao->Handle,
                                        attrib->Location,
                                        attrib->NumComponents(),
                                        VertexAttribTypeLUT[ attrib->TypeOfComponent() ],
                                        attrib->Offset );
            break;
        case VAM_INTEGER:
            glVertexArrayAttribIFormat( vao->Handle,
                                        attrib->Location,
                                        attrib->NumComponents(),
                                        VertexAttribTypeLUT[ attrib->TypeOfComponent() ],
                                        attrib->Offset );
            break;
        }

        glVertexArrayAttribBinding( vao->Handle, attrib->Location, attrib->InputSlot );

        for ( SVertexBindingInfo const * binding = hashed.VertexBindings ; binding < &hashed.VertexBindings[hashed.NumVertexBindings] ; binding++ ) {
            if ( binding->InputSlot == attrib->InputSlot ) {
                if ( binding->InputRate == INPUT_RATE_PER_INSTANCE ) {
                    //glVertexAttribDivisor( ) // тоже самое, что и glVertexBindingDivisor если attrib->Location==InputSlot
                    glVertexArrayBindingDivisor( vao->Handle, attrib->InputSlot, attrib->InstanceDataStepRate );   // Since GL v4.3
                } else {
                    glVertexArrayBindingDivisor( vao->Handle, attrib->InputSlot, 0 );   // Since GL v4.3
                }
                break;
            }
        }

        glEnableVertexArrayAttrib( vao->Handle, attrib->Location );
    }

    return vao;
}


static GLenum Attachments[ MAX_COLOR_ATTACHMENTS ];

static bool BlendCompareEquation( SRenderTargetBlendingInfo::Operation const & _Mode1,
                                  SRenderTargetBlendingInfo::Operation const & _Mode2 )
{
    return _Mode1.ColorRGB == _Mode2.ColorRGB && _Mode1.Alpha == _Mode2.Alpha;
}

static bool BlendCompareFunction( SRenderTargetBlendingInfo::Function const & _Func1,
                                  SRenderTargetBlendingInfo::Function const & _Func2 )
{
    return _Func1.SrcFactorRGB == _Func2.SrcFactorRGB
            && _Func1.DstFactorRGB == _Func2.DstFactorRGB
            && _Func1.SrcFactorAlpha == _Func2.SrcFactorAlpha
            && _Func1.DstFactorAlpha == _Func2.DstFactorAlpha;
}

static bool BlendCompareColor( const float _Color1[4], const float _Color2[4] )
{
    return     std::fabs( _Color1[0] - _Color2[0] ) < 0.000001f
            && std::fabs( _Color1[1] - _Color2[1] ) < 0.000001f
            && std::fabs( _Color1[2] - _Color2[2] ) < 0.000001f
            && std::fabs( _Color1[3] - _Color2[3] ) < 0.000001f;
}

// Compare render target blending at specified slot and change if different
static void SetRenderTargetSlotBlending( int _Slot,
                                         SRenderTargetBlendingInfo const & _CurrentState,
                                         SRenderTargetBlendingInfo const & _RequiredState )
{

    bool isEquationChanged = !BlendCompareEquation( _RequiredState.Op, _CurrentState.Op );
    bool isFunctionChanged = !BlendCompareFunction( _RequiredState.Func, _CurrentState.Func );

    // Change only modified blending states

    if ( _CurrentState.bBlendEnable != _RequiredState.bBlendEnable ) {
        if ( _RequiredState.bBlendEnable ) {
            glEnablei( GL_BLEND, _Slot );
        } else {
            glDisablei( GL_BLEND, _Slot );
        }
    }

    if ( _CurrentState.ColorWriteMask != _RequiredState.ColorWriteMask ) {
        if ( _RequiredState.ColorWriteMask == COLOR_WRITE_RGBA ) {
            glColorMaski( _Slot, 1, 1, 1, 1 );
        } else if ( _RequiredState.ColorWriteMask == COLOR_WRITE_DISABLED ) {
            glColorMaski( _Slot, 0, 0, 0, 0 );
        } else {
            glColorMaski( _Slot,
                          !!(_RequiredState.ColorWriteMask & COLOR_WRITE_R_BIT),
                          !!(_RequiredState.ColorWriteMask & COLOR_WRITE_G_BIT),
                          !!(_RequiredState.ColorWriteMask & COLOR_WRITE_B_BIT),
                          !!(_RequiredState.ColorWriteMask & COLOR_WRITE_A_BIT) );
        }
    }

    if ( isEquationChanged ) {

        bool equationSeparate = _RequiredState.Op.ColorRGB != _RequiredState.Op.Alpha;

        if ( equationSeparate ) {
            glBlendEquationSeparatei( _Slot,
                                      BlendEquationConvertionLUT[ _RequiredState.Op.ColorRGB ],
                                      BlendEquationConvertionLUT[ _RequiredState.Op.Alpha ] );
        } else {
            glBlendEquationi( _Slot,
                              BlendEquationConvertionLUT[ _RequiredState.Op.ColorRGB ] );
        }
    }

    if ( isFunctionChanged ) {

        bool funcSeparate = _RequiredState.Func.SrcFactorRGB != _RequiredState.Func.SrcFactorAlpha
            || _RequiredState.Func.DstFactorRGB != _RequiredState.Func.DstFactorAlpha;

        if ( funcSeparate ) {
            glBlendFuncSeparatei( _Slot,
                                  BlendFuncConvertionLUT[ _RequiredState.Func.SrcFactorRGB ],
                                  BlendFuncConvertionLUT[ _RequiredState.Func.DstFactorRGB ],
                                  BlendFuncConvertionLUT[ _RequiredState.Func.SrcFactorAlpha ],
                                  BlendFuncConvertionLUT[ _RequiredState.Func.DstFactorAlpha ] );
        } else {
            glBlendFunci( _Slot,
                          BlendFuncConvertionLUT[ _RequiredState.Func.SrcFactorRGB ],
                          BlendFuncConvertionLUT[ _RequiredState.Func.DstFactorRGB ] );
        }
    }
}

// Compare render target blending and change all slots if different
static void SetRenderTargetSlotsBlending( SRenderTargetBlendingInfo const & _CurrentState,
                                          SRenderTargetBlendingInfo const & _RequiredState,
                                          bool _NeedReset )
{
    bool isEquationChanged = _NeedReset || !BlendCompareEquation( _RequiredState.Op, _CurrentState.Op );
    bool isFunctionChanged = _NeedReset || !BlendCompareFunction( _RequiredState.Func, _CurrentState.Func );

    // Change only modified blending states

    if ( _NeedReset || _CurrentState.bBlendEnable != _RequiredState.bBlendEnable ) {
        if ( _RequiredState.bBlendEnable ) {
            glEnable( GL_BLEND );
        } else {
            glDisable( GL_BLEND );
        }
    }

    if ( _NeedReset || _CurrentState.ColorWriteMask != _RequiredState.ColorWriteMask ) {
        if ( _RequiredState.ColorWriteMask == COLOR_WRITE_RGBA ) {
            glColorMask( 1, 1, 1, 1 );
        } else if ( _RequiredState.ColorWriteMask == COLOR_WRITE_DISABLED ) {
            glColorMask( 0, 0, 0, 0 );
        } else {
            glColorMask( !!(_RequiredState.ColorWriteMask & COLOR_WRITE_R_BIT),
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_G_BIT),
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_B_BIT),
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_A_BIT) );
        }
    }

    if ( isEquationChanged ) {

        bool equationSeparate = _RequiredState.Op.ColorRGB != _RequiredState.Op.Alpha;

        if ( equationSeparate ) {
            glBlendEquationSeparate( BlendEquationConvertionLUT[ _RequiredState.Op.ColorRGB ],
                                     BlendEquationConvertionLUT[ _RequiredState.Op.Alpha ] );
        } else {
            glBlendEquation( BlendEquationConvertionLUT[ _RequiredState.Op.ColorRGB ] );
        }
    }

    if ( isFunctionChanged ) {

        bool funcSeparate = _RequiredState.Func.SrcFactorRGB != _RequiredState.Func.SrcFactorAlpha
            || _RequiredState.Func.DstFactorRGB != _RequiredState.Func.DstFactorAlpha;

        if ( funcSeparate ) {
            glBlendFuncSeparate( BlendFuncConvertionLUT[ _RequiredState.Func.SrcFactorRGB ],
                                 BlendFuncConvertionLUT[ _RequiredState.Func.DstFactorRGB ],
                                 BlendFuncConvertionLUT[ _RequiredState.Func.SrcFactorAlpha ],
                                 BlendFuncConvertionLUT[ _RequiredState.Func.DstFactorAlpha ] );
        } else {
            glBlendFunc( BlendFuncConvertionLUT[ _RequiredState.Func.SrcFactorRGB ],
                         BlendFuncConvertionLUT[ _RequiredState.Func.DstFactorRGB ] );
        }
    }
}

void AImmediateContextGLImpl::BindPipeline( IPipeline * _Pipeline, int _Subpass )
{
    VerifyContext();

    AN_ASSERT( _Pipeline != nullptr );

    if ( CurrentPipeline == _Pipeline ) {
        // TODO: cache drawbuffers
        if ( CurrentSubpass != _Subpass ) {
            CurrentSubpass = _Subpass;
            BindRenderPassSubPass( /*_Pipeline->pRenderPass*/CurrentRenderPass, _Subpass );
        }
        return;
    }

    CurrentPipeline = static_cast< APipelineGLImpl * >( _Pipeline );

    GLuint pipelineId = CurrentPipeline->GetHandleNativeGL();

    glBindProgramPipeline( pipelineId );

    if ( CurrentVAO != CurrentPipeline->VAO ) {
        CurrentVAO = CurrentPipeline->VAO;
        glBindVertexArray( CurrentVAO->Handle );
        //GLogger.Printf( "Binding vao %d\n", CurrentVAO->Handle );
        
    } else {
        //GLogger.Printf( "caching vao binding %d\n", CurrentVAO->Handle );
    }

    //
    // Set render pass
    //

    if ( CurrentSubpass != _Subpass ) {
        CurrentSubpass = _Subpass;
        BindRenderPassSubPass( /*CurrentPipeline->pRenderPass*/CurrentRenderPass, _Subpass );
    }

    //
    // Set input assembly
    //

    if ( CurrentPipeline->PrimitiveTopology == GL_PATCHES ) {
        if ( NumPatchVertices != CurrentPipeline->NumPatchVertices ) {
            glPatchParameteri( GL_PATCH_VERTICES, CurrentPipeline->NumPatchVertices );   // Sinse GL v4.0
            NumPatchVertices = ( uint8_t )CurrentPipeline->NumPatchVertices;
        }
    }

    if ( bPrimitiveRestartEnabled != CurrentPipeline->bPrimitiveRestartEnabled ) {
        if ( CurrentPipeline->bPrimitiveRestartEnabled ) {
            // GL_PRIMITIVE_RESTART_FIXED_INDEX if from GL_ARB_ES3_compatibility
            // Enables primitive restarting with a fixed index.
            // If enabled, any one of the draw commands which transfers a set of generic attribute array elements
            // to the GL will restart the primitive when the index of the vertex is equal to the fixed primitive index
            // for the specified index type.
            // The fixed index is equal to 2n−1 where n is equal to 8 for GL_UNSIGNED_BYTE,
            // 16 for GL_UNSIGNED_SHORT and 32 for GL_UNSIGNED_INT.
            glEnable( GL_PRIMITIVE_RESTART_FIXED_INDEX );
        } else {
            glDisable( GL_PRIMITIVE_RESTART_FIXED_INDEX );
        }

        bPrimitiveRestartEnabled = CurrentPipeline->bPrimitiveRestartEnabled;
    }

    //
    // Set blending state
    //

    // Compare blending states
    if ( Binding.BlendState != CurrentPipeline->BlendingState ) {
        SBlendingStateInfo const & desc = *CurrentPipeline->BlendingState;

        if ( desc.bIndependentBlendEnable ) {
            for ( int i = 0 ; i < MAX_COLOR_ATTACHMENTS ; i++ ) {
                SRenderTargetBlendingInfo const & rtDesc = desc.RenderTargetSlots[ i ];
                SetRenderTargetSlotBlending( i, BlendState.RenderTargetSlots[ i ], rtDesc );
                memcpy( &BlendState.RenderTargetSlots[ i ], &rtDesc, sizeof( rtDesc ) );
            }
        } else {
            SRenderTargetBlendingInfo const & rtDesc = desc.RenderTargetSlots[ 0 ];
            bool needReset = BlendState.bIndependentBlendEnable;
            SetRenderTargetSlotsBlending( BlendState.RenderTargetSlots[ 0 ], rtDesc, needReset );
            for ( int i = 0 ; i < MAX_COLOR_ATTACHMENTS ; i++ ) {
                memcpy( &BlendState.RenderTargetSlots[ i ], &rtDesc, sizeof( rtDesc ) );
            }
        }

        BlendState.bIndependentBlendEnable = desc.bIndependentBlendEnable;

        if ( BlendState.bSampleAlphaToCoverage != desc.bSampleAlphaToCoverage ) {
            if ( desc.bSampleAlphaToCoverage ) {
                glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
            } else {
                glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
            }

            BlendState.bSampleAlphaToCoverage = desc.bSampleAlphaToCoverage;
        }

        if ( BlendState.LogicOp != desc.LogicOp ) {
            if ( desc.LogicOp == LOGIC_OP_COPY ) {
                if ( bLogicOpEnabled ) {
                    glDisable( GL_COLOR_LOGIC_OP );
                    bLogicOpEnabled = false;
                }
            } else {
                if ( !bLogicOpEnabled ) {
                    glEnable( GL_COLOR_LOGIC_OP );
                    bLogicOpEnabled = true;
                }
                glLogicOp( LogicOpLUT[ desc.LogicOp ] );
            }

            BlendState.LogicOp = desc.LogicOp;
        }

        Binding.BlendState = CurrentPipeline->BlendingState;
    }

    //
    // Set rasterizer state
    //

    if ( Binding.RasterizerState != CurrentPipeline->RasterizerState ) {
        SRasterizerStateInfo const & desc = *CurrentPipeline->RasterizerState;

        if ( RasterizerState.FillMode != desc.FillMode ) {
            glPolygonMode( GL_FRONT_AND_BACK, FillModeLUT[ desc.FillMode ] );
            RasterizerState.FillMode = desc.FillMode;
        }

        if ( RasterizerState.CullMode != desc.CullMode ) {
            if ( desc.CullMode == POLYGON_CULL_DISABLED ) {
                glDisable( GL_CULL_FACE );
            } else {
                if ( RasterizerState.CullMode == POLYGON_CULL_DISABLED ) {
                    glEnable( GL_CULL_FACE );
                }
                if ( CullFace != CullModeLUT[desc.CullMode] ) {
                    CullFace = CullModeLUT[desc.CullMode];
                    glCullFace( CullFace );
                }
            }
            RasterizerState.CullMode = desc.CullMode;
        }

        if ( RasterizerState.bScissorEnable != desc.bScissorEnable ) {
            if ( desc.bScissorEnable ) {
                glEnable( GL_SCISSOR_TEST );
            } else {
                glDisable( GL_SCISSOR_TEST );
            }
            RasterizerState.bScissorEnable = desc.bScissorEnable;
        }

        if ( RasterizerState.bMultisampleEnable != desc.bMultisampleEnable ) {
            if ( desc.bMultisampleEnable ) {
                glEnable( GL_MULTISAMPLE );
            } else {
                glDisable( GL_MULTISAMPLE );
            }
            RasterizerState.bMultisampleEnable = desc.bMultisampleEnable;
        }

        if ( RasterizerState.bRasterizerDiscard != desc.bRasterizerDiscard ) {
            if ( desc.bRasterizerDiscard ) {
                glEnable( GL_RASTERIZER_DISCARD );
            } else {
                glDisable( GL_RASTERIZER_DISCARD );
            }
            RasterizerState.bRasterizerDiscard = desc.bRasterizerDiscard;
        }

        if ( RasterizerState.bAntialiasedLineEnable != desc.bAntialiasedLineEnable ) {
            if ( desc.bAntialiasedLineEnable ) {
                glEnable( GL_LINE_SMOOTH );
            } else {
                glDisable( GL_LINE_SMOOTH );
            }
            RasterizerState.bAntialiasedLineEnable = desc.bAntialiasedLineEnable;
        }

        if ( RasterizerState.bDepthClampEnable != desc.bDepthClampEnable ) {
            if ( desc.bDepthClampEnable ) {
                glEnable( GL_DEPTH_CLAMP );
            } else {
                glDisable( GL_DEPTH_CLAMP );
            }
            RasterizerState.bDepthClampEnable = desc.bDepthClampEnable;
        }

        if ( RasterizerState.DepthOffset.Slope != desc.DepthOffset.Slope
            || RasterizerState.DepthOffset.Bias != desc.DepthOffset.Bias
            || RasterizerState.DepthOffset.Clamp != desc.DepthOffset.Clamp ) {

            PolygonOffsetClampSafe( desc.DepthOffset.Slope, desc.DepthOffset.Bias, desc.DepthOffset.Clamp );

            RasterizerState.DepthOffset.Slope = desc.DepthOffset.Slope;
            RasterizerState.DepthOffset.Bias = desc.DepthOffset.Bias;
            RasterizerState.DepthOffset.Clamp = desc.DepthOffset.Clamp;
        }

        if ( RasterizerState.bFrontClockwise != desc.bFrontClockwise ) {
            glFrontFace( desc.bFrontClockwise ? GL_CW : GL_CCW );
            RasterizerState.bFrontClockwise = desc.bFrontClockwise;
        }

        Binding.RasterizerState = CurrentPipeline->RasterizerState;
    }

    //
    // Set depth stencil state
    //
#if 0
    if ( Binding.DepthStencilState == CurrentPipeline->DepthStencilState ) {

        if ( StencilRef != _StencilRef ) {
            // Update stencil ref

            DepthStencilStateInfo const & desc = CurrentPipeline->DepthStencilState;

            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFuncSeparate( GL_FRONT_AND_BACK,
                                       ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                       _StencilRef,
                                       desc.StencilReadMask );
            } else {
                glStencilFuncSeparate( GL_FRONT,
                                       ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                       _StencilRef,
                                       desc.StencilReadMask );

                glStencilFuncSeparate( GL_BACK,
                                       ComparisonFuncLUT[desc.BackFace.StencilFunc],
                                       _StencilRef,
                                       desc.StencilReadMask );
            }
        }
    }
#endif

    if ( Binding.DepthStencilState != CurrentPipeline->DepthStencilState ) {
        SDepthStencilStateInfo const & desc = *CurrentPipeline->DepthStencilState;

        if ( DepthStencilState.bDepthEnable != desc.bDepthEnable ) {
            if ( desc.bDepthEnable ) {
                glEnable( GL_DEPTH_TEST );
            } else {
                glDisable( GL_DEPTH_TEST );
            }

            DepthStencilState.bDepthEnable = desc.bDepthEnable;
        }

        if ( DepthStencilState.DepthWriteMask != desc.DepthWriteMask ) {
            glDepthMask( desc.DepthWriteMask );
            DepthStencilState.DepthWriteMask = desc.DepthWriteMask;
        }

        if ( DepthStencilState.DepthFunc != desc.DepthFunc ) {
            glDepthFunc( ComparisonFuncLUT[ desc.DepthFunc ] );
            DepthStencilState.DepthFunc = desc.DepthFunc;
        }

        if ( DepthStencilState.bStencilEnable != desc.bStencilEnable ) {
            if ( desc.bStencilEnable ) {
                glEnable( GL_STENCIL_TEST );
            } else {
                glDisable( GL_STENCIL_TEST );
            }

            DepthStencilState.bStencilEnable = desc.bStencilEnable;
        }

        if ( DepthStencilState.StencilWriteMask != desc.StencilWriteMask ) {
            glStencilMask( desc.StencilWriteMask );
            DepthStencilState.StencilWriteMask = desc.StencilWriteMask;
        }

        if ( DepthStencilState.StencilReadMask != desc.StencilReadMask
             || DepthStencilState.FrontFace.StencilFunc != desc.FrontFace.StencilFunc
             || DepthStencilState.BackFace.StencilFunc != desc.BackFace.StencilFunc
             /*|| StencilRef != _StencilRef*/ ) {

#if 0
            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                               StencilRef,//_StencilRef,
                               desc.StencilReadMask );
            } else
#endif
            {
                if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                    glStencilFuncSeparate( GL_FRONT_AND_BACK,
                                           ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                           StencilRef,//_StencilRef,
                                           desc.StencilReadMask );
                } else {
                    glStencilFuncSeparate( GL_FRONT,
                                           ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                           StencilRef,//_StencilRef,
                                           desc.StencilReadMask );

                    glStencilFuncSeparate( GL_BACK,
                                           ComparisonFuncLUT[ desc.BackFace.StencilFunc ],
                                           StencilRef,//_StencilRef,
                                           desc.StencilReadMask );
                }
            }

            DepthStencilState.StencilReadMask = desc.StencilReadMask;
            DepthStencilState.FrontFace.StencilFunc = desc.FrontFace.StencilFunc;
            DepthStencilState.BackFace.StencilFunc = desc.BackFace.StencilFunc;
            //StencilRef = _StencilRef;
        }

        bool frontStencilChanged = ( DepthStencilState.FrontFace.StencilFailOp != desc.FrontFace.StencilFailOp
                                     || DepthStencilState.FrontFace.DepthFailOp != desc.FrontFace.DepthFailOp
                                     || DepthStencilState.FrontFace.DepthPassOp != desc.FrontFace.DepthPassOp );

        bool backStencilChanged = ( DepthStencilState.BackFace.StencilFailOp != desc.BackFace.StencilFailOp
                                     || DepthStencilState.BackFace.DepthFailOp != desc.BackFace.DepthFailOp
                                     || DepthStencilState.BackFace.DepthPassOp != desc.BackFace.DepthPassOp );

        if ( frontStencilChanged || backStencilChanged ) {
            bool isSame = desc.FrontFace.StencilFailOp == desc.BackFace.StencilFailOp
                       && desc.FrontFace.DepthFailOp == desc.BackFace.DepthFailOp
                       && desc.FrontFace.DepthPassOp == desc.BackFace.DepthPassOp;

            if ( isSame ) {
                glStencilOpSeparate( GL_FRONT_AND_BACK,
                                     StencilOpLUT[ desc.FrontFace.StencilFailOp ],
                                     StencilOpLUT[ desc.FrontFace.DepthFailOp ],
                                     StencilOpLUT[ desc.FrontFace.DepthPassOp ] );

                memcpy( &DepthStencilState.FrontFace, &desc.FrontFace, sizeof( desc.FrontFace ) );
                memcpy( &DepthStencilState.BackFace, &desc.BackFace, sizeof( desc.FrontFace ) );
            } else {

                if ( frontStencilChanged ) {
                    glStencilOpSeparate( GL_FRONT,
                                         StencilOpLUT[ desc.FrontFace.StencilFailOp ],
                                         StencilOpLUT[ desc.FrontFace.DepthFailOp ],
                                         StencilOpLUT[ desc.FrontFace.DepthPassOp ] );

                    memcpy( &DepthStencilState.FrontFace, &desc.FrontFace, sizeof( desc.FrontFace ) );
                }

                if ( backStencilChanged ) {
                    glStencilOpSeparate( GL_BACK,
                                         StencilOpLUT[ desc.BackFace.StencilFailOp ],
                                         StencilOpLUT[ desc.BackFace.DepthFailOp ],
                                         StencilOpLUT[ desc.BackFace.DepthPassOp ] );

                    memcpy( &DepthStencilState.BackFace, &desc.BackFace, sizeof( desc.FrontFace ) );
                }
            }
        }

        Binding.DepthStencilState = CurrentPipeline->DepthStencilState;
    }

    //
    // Set sampler state
    //

    glBindSamplers( 0, CurrentPipeline->NumSamplerObjects, CurrentPipeline->SamplerObjects ); // 4.4 or GL_ARB_multi_bind

    //for ( int i = 0 ; i < CurrentPipeline->NumSamplerObjects ; i++ ) {
    //    glBindSampler( i, CurrentPipeline->SamplerObjects[i] ); // 3.2 or GL_ARB_sampler_objects
    //}
}

void AImmediateContextGLImpl::BindRenderPassSubPass( ARenderPassGLImpl const * _RenderPass, int _Subpass )
{
    VerifyContext();

    if ( Binding.DrawFramebufferUID == DefaultFramebuffer->GetUID() ) {
        //glDrawBuffer(GL_BACK);
        //glNamedFramebufferDrawBuffer( 0, GL_BACK );
        return;
    }

    const GLuint framebufferId = Binding.DrawFramebuffer;

    AN_ASSERT( _RenderPass != nullptr );
    AN_ASSERT( _Subpass < _RenderPass->NumSubpasses );

    SRenderSubpass const * subpass = &_RenderPass->Subpasses[_Subpass];

    if ( subpass->NumColorAttachments > 0 ) {
        for ( uint32_t i = 0 ; i < subpass->NumColorAttachments ; i++ ) {
            Attachments[ i ] = GL_COLOR_ATTACHMENT0 + subpass->ColorAttachmentRefs[i].Attachment;
        }

        glNamedFramebufferDrawBuffers( framebufferId, subpass->NumColorAttachments, Attachments );
    } else {
        glNamedFramebufferDrawBuffer( framebufferId, GL_NONE );
    }
}

void AImmediateContextGLImpl::BindVertexBuffer( unsigned int _InputSlot,
                                                IBuffer const * _VertexBuffer,
                                                unsigned int _Offset )
{
    AN_ASSERT( _InputSlot < MAX_VERTEX_BUFFER_SLOTS );

    VertexBufferUIDs[_InputSlot] = _VertexBuffer ? _VertexBuffer->GetUID() : 0;
    VertexBufferHandles[_InputSlot] = _VertexBuffer ? _VertexBuffer->GetHandleNativeGL() : 0;
    VertexBufferOffsets[_InputSlot] = _Offset;
}

void AImmediateContextGLImpl::BindVertexBuffers( unsigned int _StartSlot,
                                                 unsigned int _NumBuffers,
                                                 IBuffer * const * _VertexBuffers,
                                                 uint32_t const * _Offsets )
{
    AN_ASSERT( _StartSlot + _NumBuffers <= MAX_VERTEX_BUFFER_SLOTS );

    if ( _VertexBuffers ) {
        for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            int slot = _StartSlot + i;

            VertexBufferUIDs[slot] = _VertexBuffers[i] ? _VertexBuffers[i]->GetUID() : 0;
            VertexBufferHandles[slot] = _VertexBuffers[i] ? _VertexBuffers[i]->GetHandleNativeGL() : 0;
            VertexBufferOffsets[slot] = _Offsets ? _Offsets[i] : 0;
        }
    }
    else {
        for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            int slot = _StartSlot + i;

            VertexBufferUIDs[slot] = 0;
            VertexBufferHandles[slot] = 0;
            VertexBufferOffsets[slot] = 0;
        }
    }
}

#if 0




void AImmediateContextGLImpl::BindVertexBuffers( unsigned int _StartSlot,
                                                 unsigned int _NumBuffers,
                                                 IBuffer * const * _VertexBuffers,
                                                 uint32_t const * _Offsets )
{
    VerifyContext();

    AN_ASSERT( CurrentVAO != nullptr );

    GLuint id = CurrentVAO->Handle;

    static_assert( sizeof( CurrentVAO->VertexBindingsStrides[0] ) == sizeof( GLsizei ), "Wrong type size" );

    if ( _StartSlot + _NumBuffers > pDevice->MaxVertexBufferSlots ) {
        GLogger.Printf( "BindVertexBuffers: StartSlot + NumBuffers > MaxVertexBufferSlots\n" );
        return;
    }

    bool bModified = false;

    if ( _VertexBuffers ) {

        for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
            unsigned int slot = _StartSlot + i;

            uint32_t uid = _VertexBuffers[i] ? _VertexBuffers[i]->GetUID() : 0;
            uint32_t offset = _Offsets ? _Offsets[i] : 0;

            bModified = CurrentVAO->VertexBufferUIDs[ slot ] != uid || CurrentVAO->VertexBufferOffsets[ slot ] != offset;

            CurrentVAO->VertexBufferUIDs[ slot ] = uid;
            CurrentVAO->VertexBufferOffsets[ slot ] = offset;
        }

        if ( !bModified ) {
            return;
        }

        if ( _NumBuffers == 1 )
        {
            GLuint vertexBufferId = _VertexBuffers[0] ? _VertexBuffers[0]->GetHandleNativeGL() : 0;
            glVertexArrayVertexBuffer( id, _StartSlot, vertexBufferId, CurrentVAO->VertexBufferOffsets[ _StartSlot ], CurrentVAO->VertexBindingsStrides[ _StartSlot ] );
        }
        else
        {
            // Convert input parameters to OpenGL format
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                TmpHandles[ i ] = _VertexBuffers[i] ? _VertexBuffers[i]->GetHandleNativeGL : 0;
                TmpPointers[ i ] = CurrentVAO->VertexBufferOffsets[ _StartSlot + i ];
            }
            glVertexArrayVertexBuffers( id, _StartSlot, _NumBuffers, TmpHandles, TmpPointers, reinterpret_cast< const GLsizei * >( &CurrentVAO->VertexBindingsStrides[ _StartSlot ] ) );
        }
    } else {

        for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
            unsigned int slot = _StartSlot + i;

            uint32_t uid = 0;
            uint32_t offset = 0;

            bModified = CurrentVAO->VertexBufferUIDs[ slot ] != uid || CurrentVAO->VertexBufferOffsets[ slot ] != offset;

            CurrentVAO->VertexBufferUIDs[ slot ] = uid;
            CurrentVAO->VertexBufferOffsets[ slot ] = offset;
        }

        if ( !bModified ) {
            return;
        }

        if ( _NumBuffers == 1 ) {
            glVertexArrayVertexBuffer( id, _StartSlot, 0, 0, 16 ); // From OpenGL specification
        } else {
            glVertexArrayVertexBuffers( id, _StartSlot, _NumBuffers, nullptr, nullptr, nullptr );
        }
    }
}
#endif

void AImmediateContextGLImpl::BindIndexBuffer( IBuffer const * _IndexBuffer,
                                               INDEX_TYPE _Type,
                                               unsigned int _Offset )
{
    IndexBufferType = IndexTypeLUT[ _Type ];
    IndexBufferOffset = _Offset;
    IndexBufferTypeSizeOf = IndexTypeSizeOfLUT[ _Type ];
    IndexBufferUID = _IndexBuffer ? _IndexBuffer->GetUID() : 0;
    IndexBufferHandle = _IndexBuffer ? _IndexBuffer->GetHandleNativeGL() : 0;
}

IResourceTable * AImmediateContextGLImpl::GetRootResourceTable()
{
    return RootResourceTable;
}

void AImmediateContextGLImpl::BindResourceTable( IResourceTable * _ResourceTable )
{
    IResourceTable * tbl = _ResourceTable ? _ResourceTable : RootResourceTable.GetObject();

    CurrentResourceTable = static_cast< AResourceTableGLImpl * >( tbl );
}

#if 0
void AImmediateContextGLImpl::BindResourceTable( SResourceTable const * _ResourceTable )
{
    // TODO: Cache resource tables

    VerifyContext();

    SResourceBufferBinding const * buffers = _ResourceTable->GetBuffers();
    //SResourceTextureBinding const * textures = _ResourceTable->GetTextures();
    //SResourceImageBinding const * images = _ResourceTable->GetImages();
    int numBuffers = _ResourceTable->GetNumBuffers();
    //int numTextures = _ResourceTable->GetNumTextures();
    //int numImages = _ResourceTable->GetNumImages();
    int slot;

    slot = 0;
    for ( SResourceBufferBinding const * binding = buffers ; binding < &buffers[numBuffers] ; binding++, slot++ ) {
        GLenum target = BufferTargetLUT[ binding->BufferType ].Target;

        IDeviceObject const * native = binding->pBuffer;

        uint32_t bufferUid = native ? native->GetUID() : 0;

        if ( BufferBindingUIDs[ slot ] != bufferUid || binding->BindingSize > 0 ) {
            BufferBindingUIDs[ slot ] = bufferUid;

            if ( bufferUid && binding->BindingSize > 0 ) {
                glBindBufferRange( target, slot, native->GetHandleNativeGL(), binding->BindingOffset, binding->BindingSize ); // 3.0 or GL_ARB_uniform_buffer_object
            } else {
                glBindBufferBase( target, slot, native->GetHandleNativeGL() ); // 3.0 or GL_ARB_uniform_buffer_object
            }
        }
    }
#if 0
    slot = 0;
    for ( SResourceTextureBinding const * binding = textures ; binding < &textures[numTextures] ; binding++, slot++ ) {
        GLuint textureId;
        uint32_t textureUid;

        IDeviceObject const * native = binding->pTexture;

        if ( native ) {
            textureId = native->GetHandleNativeGL();
            textureUid = native->GetUID();
        } else {
            textureId = 0;
            textureUid = 0;
        }

        if ( TextureBindings[ slot ] != textureUid ) {
            TextureBindings[ slot ] = textureUid;

            glBindTextureUnit( slot, textureId ); // 4.5
        }
    }

    slot = 0;
    for ( SResourceImageBinding const * binding = images ; binding < &images[numImages] ; binding++, slot++ ) {
        IDeviceObject const * native = binding->pTexture;

        GLuint id = native ? native->GetHandleNativeGL() : 0;

        glBindImageTexture( slot,
                            id,
                            binding->Lod,
                            binding->bLayered,
                            binding->LayerIndex,
                            ImageAccessModeLUT[ binding->AccessMode ],
                            InternalFormatLUT[ binding->TextureFormat ].InternalFormat ); // 4.2
    }
#endif
}
#endif

#if 0
void ImmediateContext::SetBuffers( BUFFER_TYPE _BufferType,
                                unsigned int _StartSlot,
                                unsigned int _NumBuffers,
                                /* optional */ IBuffer * const * _Buffers,
                                /* optional */ unsigned int * _RangeOffsets,
                                /* optional */ unsigned int * _RangeSizes )
{
    VerifyContext();

    // _StartSlot + _NumBuffers must be < storage->MaxBufferBindings[_BufferType]

    GLenum target = BufferTargetLUT[ _BufferType ].Target;

    if ( !_Buffers ) {
        glBindBuffersBase( target, _StartSlot, _NumBuffers, nullptr ); // 4.4 or GL_ARB_multi_bind
        return;
    }

    if ( _RangeOffsets && _RangeSizes ) {
        if ( _NumBuffers == 1 ) {
            glBindBufferRange( target, _StartSlot, ( size_t )_Buffers[0]->GetHandle(), _RangeOffsets[0], _RangeSizes[0] ); // 3.0 or GL_ARB_uniform_buffer_object
        } else {
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                TmpHandles[ i ] = ( size_t )_Buffers[ i ]->GetHandle();
                TmpPointers[ i ] = _RangeOffsets[ i ];
                TmpPointers2[ i ] = _RangeSizes[ i ];
            }
            glBindBuffersRange( target, _StartSlot, _NumBuffers, TmpHandles, TmpPointers, TmpPointers2 ); // 4.4 or GL_ARB_multi_bind

            // Or  Since 3.0 or GL_ARB_uniform_buffer_object
            //if ( _Buffers != nullptr) {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferRange( target, _StartSlot + i, _Buffers[i]->GetHandleNativeGL(), _RangeOffsets​[i], _RangeSizes[i] );
            //    }
            //} else {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, 0 );
            //    }
            //}
        }
    } else {
        if ( _NumBuffers == 1 ) {
            glBindBufferBase( target, _StartSlot, ( size_t )_Buffers[0]->GetHandle() ); // 3.0 or GL_ARB_uniform_buffer_object
        } else {
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                TmpHandles[ i ] = ( size_t )_Buffers[i]->GetHandle();
            }
            glBindBuffersBase( target, _StartSlot, _NumBuffers, TmpHandles ); // 4.4 or GL_ARB_multi_bind

            // Or  Since 3.0 or GL_ARB_uniform_buffer_object
            //if ( _Buffers != nullptr) {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, _Buffers[i]->GetHandleNativeGL() );
            //    }
            //} else {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, 0 );
            //    }
            //}
        }
    }
}

void ImmediateContext::SetTextureUnit( unsigned int _Unit, /* optional */ Texture const * _Texture )
{
    VerifyContext();

    // Unit must be < MaxCombinedTextureImageUnits

    glBindTextureUnit( _Unit, ( size_t )_Texture->GetHandle() ); // 4.5

    /*
    // Other path
    GLuint texture = ( size_t )_Texture->GetHandle();
    glBindTextures( _Unit, 1, &texture ); // 4.4
    */

    /*
    // Other path
    glActiveTexture( GL_TEXTURE0 + _Unit );
    if ( texture ) {
        glBindTexture( target, ( size_t )_Texture->GetHandle() );
    } else {
        for (target in all supported targets) {
            glBindTexture( target, 0 );
        }
    }
    */
}

void ImmediateContext::SetTextureUnits( unsigned int _FirstUnit,
                                        unsigned int _NumUnits,
                                        /* optional */ Texture * const * _Textures )
{
    VerifyContext();

    // _FirstUnit + _NumUnits must be < MaxCombinedTextureImageUnits

    if ( _Textures ) {
        for ( unsigned int i = 0 ; i < _NumUnits ; i++ ) {
            TmpHandles[ i ] = ( size_t )_Textures[ i ]->GetHandle();
        }

        glBindTextures(	_FirstUnit, _NumUnits, TmpHandles ); // 4.4
    } else {
        glBindTextures(	_FirstUnit, _NumUnits, nullptr ); // 4.4
    }


    /*
    // Other path
    for (i = 0; i < _NumUnits; i++) {
        GLuint texture;
        if (textures == nullptr) {
            texture = 0;
        } else {
            texture = textures[i];
        }
        glActiveTexture(GL_TEXTURE0 + _FirstUnit + i);
        if (texture != 0) {
            GLenum target = // target of textures[i] ;
            glBindTexture(target, textures[i]);
        } else {
            for (target in all supported targets) {
                glBindTexture(target, 0);
            }
        }
    }
    */
}

bool ImmediateContext::SetImageUnit( unsigned int _Unit,
                                  /* optional */ Texture * _Texture,
                                  unsigned int _Lod,
                                  bool _AllLayers,
                                  unsigned int _LayerIndex,  // Array index for texture arrays, depth for 3D textures or cube face for cubemaps
                                  // For cubemap arrays: arrayLength = floor( _LayerIndex / 6 ), face = _LayerIndex % 6
                                  IMAGE_ACCESS_MODE _AccessMode,
                                  INTERNAL_PIXEL_FORMAT _InternalFormat )
{
    VerifyContext();

    if ( !*InternalFormatLUT[ _InternalFormat ].ShaderImageFormatQualifier ) {
        return false;
    }

    // Unit must be < ResourceStoragesPtr->MaxImageUnits
    glBindImageTexture( _Unit,
                        ( size_t )_Texture->GetHandle(),
                        _Lod,
                        _AllLayers,
                        _LayerIndex,
                        ImageAccessModeLUT[ _AccessMode ],
                        InternalFormatLUT[ _InternalFormat ].InternalFormat ); // 4.2

    return true;
}

void ImmediateContext::SetImageUnits( unsigned int _FirstUnit,
                                      unsigned int _NumUnits,
                                      /* optional */ Texture * const * _Textures )
{
    VerifyContext();

    // _FirstUnit + _NumUnits must be < ResourceStoragesPtr->MaxImageUnits

    if ( _Textures ) {
        for ( unsigned int i = 0 ; i < _NumUnits ; i++ ) {
            TmpHandles[ i ] = ( size_t )_Textures[ i ]->GetHandle();
        }

        glBindImageTextures( _FirstUnit, _NumUnits, TmpHandles ); // 4.4
    } else {
        glBindImageTextures( _FirstUnit, _NumUnits, nullptr ); // 4.4
    }

    /*
    // Other path:
    for (i = 0; i < _NumUnits; i++) {
        if (textures == nullptr || textures[i] = 0) {
            glBindImageTexture(_FirstUnit + i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8); // 4.2
        } else {
            glBindImageTexture(_FirstUnit + i, textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, lookupInternalFormat(textures[i])); // 4.2
        }
    }
    */
}

#endif

#define INVERT_VIEWPORT_Y(Viewport) ( Binding.DrawFramebufferHeight - (Viewport)->Y - (Viewport)->Height )

void AImmediateContextGLImpl::SetViewport( SViewport const & _Viewport )
{
    VerifyContext();

    if ( memcmp( CurrentViewport, &_Viewport, sizeof( CurrentViewport ) ) != 0 ) {
        if ( ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT ) {
            glViewport( (GLint)_Viewport.X,
                        (GLint)INVERT_VIEWPORT_Y(&_Viewport),
                        (GLsizei)_Viewport.Width,
                        (GLsizei)_Viewport.Height );
        } else {
            glViewport( (GLint)_Viewport.X,
                        (GLint)_Viewport.Y,
                        (GLsizei)_Viewport.Width,
                        (GLsizei)_Viewport.Height );
        }
        memcpy( CurrentViewport, &_Viewport, sizeof( CurrentViewport ) );
    }

    if ( memcmp( CurrentDepthRange, &_Viewport.MinDepth, sizeof( CurrentDepthRange ) ) != 0 ) {
        glDepthRangef( _Viewport.MinDepth, _Viewport.MaxDepth ); // Since GL v4.1

        memcpy( CurrentDepthRange, &_Viewport.MinDepth, sizeof( CurrentDepthRange ) );
    }
}

void AImmediateContextGLImpl::SetViewportArray( uint32_t _NumViewports, SViewport const * _Viewports )
{
    SetViewportArray( 0, _NumViewports, _Viewports );
}

void AImmediateContextGLImpl::SetViewportArray( uint32_t _FirstIndex, uint32_t _NumViewports, SViewport const * _Viewports )
{
    VerifyContext();

    #define MAX_VIEWPORT_DATA 1024
    static_assert( sizeof( float ) * 2 == sizeof( double ), "ImmediateContext::SetViewportArray type check" );
    constexpr uint32_t MAX_VIEWPORTS = MAX_VIEWPORT_DATA >> 2;
    float viewportData[ MAX_VIEWPORT_DATA ];

    _NumViewports = std::min( MAX_VIEWPORTS, _NumViewports );

    bool bInvertY = ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    float * pViewportData = viewportData;
    for ( SViewport const * viewport = _Viewports ; viewport < &_Viewports[_NumViewports] ; viewport++, pViewportData += 4 ) {
        pViewportData[ 0 ] = viewport->X;
        pViewportData[ 1 ] = bInvertY ? INVERT_VIEWPORT_Y( viewport ) : viewport->Y;
        pViewportData[ 2 ] = viewport->Width;
        pViewportData[ 3 ] = viewport->Height;
    }
    glViewportArrayv( _FirstIndex, _NumViewports, viewportData );

    double * pDepthRangeData = reinterpret_cast< double * >( &viewportData[0] );
    for ( SViewport const * viewport = _Viewports ; viewport < &_Viewports[_NumViewports] ; viewport++, pDepthRangeData += 2 ) {
        pDepthRangeData[ 0 ] = viewport->MinDepth;
        pDepthRangeData[ 1 ] = viewport->MaxDepth;
    }
    glDepthRangeArrayv( _FirstIndex, _NumViewports, reinterpret_cast< double * >( &viewportData[0] ) );
}

void AImmediateContextGLImpl::SetViewportIndexed( uint32_t _Index, SViewport const & _Viewport )
{
    VerifyContext();

    bool bInvertY = ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;
    float viewportData[4] = { _Viewport.X, bInvertY ? INVERT_VIEWPORT_Y(&_Viewport) : _Viewport.Y, _Viewport.Width, _Viewport.Height };
    glViewportIndexedfv( _Index, viewportData );
    glDepthRangeIndexed( _Index, _Viewport.MinDepth, _Viewport.MaxDepth );
}

void AImmediateContextGLImpl::SetScissor( /* optional */ SRect2D const & _Scissor )
{
    VerifyContext();

    CurrentScissor = _Scissor;

    bool bInvertY = ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    glScissor( CurrentScissor.X,
               bInvertY ? Binding.DrawFramebufferHeight - CurrentScissor.Y - CurrentScissor.Height : CurrentScissor.Y,
               CurrentScissor.Width,
               CurrentScissor.Height );
}

void AImmediateContextGLImpl::SetScissorArray( uint32_t _NumScissors, SRect2D const * _Scissors )
{
    SetScissorArray( 0, _NumScissors, _Scissors );
}

void AImmediateContextGLImpl::SetScissorArray( uint32_t _FirstIndex, uint32_t _NumScissors, SRect2D const * _Scissors )
{
    VerifyContext();

    #define MAX_SCISSOR_DATA 1024
    constexpr uint32_t MAX_SCISSORS = MAX_VIEWPORT_DATA >> 2;
    GLint scissorData[ MAX_SCISSOR_DATA ];

    _NumScissors = std::min( MAX_SCISSORS, _NumScissors );

    bool bInvertY = ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    GLint * pScissorData = scissorData;
    for ( SRect2D const * scissor = _Scissors ; scissor < &_Scissors[_NumScissors] ; scissor++, pScissorData += 4 ) {
        pScissorData[ 0 ] = scissor->X;
        pScissorData[ 1 ] = bInvertY ? INVERT_VIEWPORT_Y( scissor ) : scissor->Y;
        pScissorData[ 2 ] = scissor->Width;
        pScissorData[ 3 ] = scissor->Height;
    }
    glScissorArrayv( _FirstIndex, _NumScissors, scissorData );
}

void AImmediateContextGLImpl::SetScissorIndexed( uint32_t _Index, SRect2D const & _Scissor )
{
    VerifyContext();

    bool bInvertY = ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    int scissorData[4] = { _Scissor.X, bInvertY ? INVERT_VIEWPORT_Y(&_Scissor) : _Scissor.Y, _Scissor.Width, _Scissor.Height };
    glScissorIndexedv( _Index, scissorData );
}

void AImmediateContextGLImpl::UpdateVertexBuffers()
{
    for ( SVertexBindingInfo const * binding = CurrentVAO->Hashed.VertexBindings ; binding < &CurrentVAO->Hashed.VertexBindings[CurrentVAO->Hashed.NumVertexBindings] ; binding++ ) {
        int slot = binding->InputSlot;

        if ( CurrentVAO->VertexBufferUIDs[slot] != VertexBufferUIDs[slot] || CurrentVAO->VertexBufferOffsets[slot] != VertexBufferOffsets[slot] ) {

            glVertexArrayVertexBuffer( CurrentVAO->Handle, slot, VertexBufferHandles[slot], VertexBufferOffsets[slot], CurrentVAO->VertexBindingsStrides[slot] );

            CurrentVAO->VertexBufferUIDs[slot] = VertexBufferUIDs[slot];
            CurrentVAO->VertexBufferOffsets[slot] = VertexBufferOffsets[slot];
        } else {
            //GLogger.Printf( "Caching BindVertexBuffer %d\n", vertexBufferId );
        }
    }
}

void AImmediateContextGLImpl::UpdateVertexAndIndexBuffers()
{
    UpdateVertexBuffers();

    if ( CurrentVAO->IndexBufferUID != IndexBufferUID ) {
        glVertexArrayElementBuffer( CurrentVAO->Handle, IndexBufferHandle );
        CurrentVAO->IndexBufferUID = IndexBufferUID;

        //GLogger.Printf( "BindIndexBuffer %d\n", indexBufferId );
    } else {
        //GLogger.Printf( "Caching BindIndexBuffer %d\n", indexBufferId );
    }
}

void AImmediateContextGLImpl::UpdateShaderBindings()
{
    // TODO: memcmp CurrentPipeline->TextureBindings, CurrentResourceTable->TextureBindings2

    glBindTextures(	0, CurrentPipeline->NumSamplerObjects, CurrentResourceTable->GetTextureBindings() ); // 4.4

#if 1
    for ( int i = 0 ; i < CurrentPipeline->NumImages ; i++ ) {
        // TODO: cache image bindings (memcmp?)
        glBindImageTexture( i,
                            CurrentResourceTable->GetImageBindings()[i],
                            CurrentResourceTable->GetImageLod()[i],
                            CurrentResourceTable->GetImageLayered()[i],
                            CurrentResourceTable->GetImageLayerIndex()[i],
                            CurrentPipeline->Images[i].AccessMode,
                            CurrentPipeline->Images[i].InternalFormat ); // 4.2
    }
#else
    glBindImageTextures( 0, CurrentPipeline->NumImages, CurrentResourceTable->ImageBindings ); // 4.4
#endif

    for ( int i = 0 ; i < CurrentPipeline->NumBuffers ; i++ ) {
        if ( BufferBindingUIDs[ i ] != CurrentResourceTable->GetBufferBindingUIDs()[i]
             || BufferBindingOffsets[ i ] != CurrentResourceTable->GetBufferBindingOffsets()[i]
             || BufferBindingSizes[ i ] != CurrentResourceTable->GetBufferBindingSizes()[i] ) {

            BufferBindingUIDs[ i ] = CurrentResourceTable->GetBufferBindingUIDs()[i];
            BufferBindingOffsets[ i ] = CurrentResourceTable->GetBufferBindingOffsets()[i];
            BufferBindingSizes[ i ] = CurrentResourceTable->GetBufferBindingSizes()[i];

            if ( BufferBindingUIDs[ i ] && BufferBindingSizes[ i ] > 0 ) {
                glBindBufferRange( CurrentPipeline->Buffers[i].BufferType, i, CurrentResourceTable->GetBufferBindings()[i], BufferBindingOffsets[ i ], BufferBindingSizes[ i ] ); // 3.0 or GL_ARB_uniform_buffer_object
            } else {
                glBindBufferBase( CurrentPipeline->Buffers[i].BufferType, i, CurrentResourceTable->GetBufferBindings()[i] ); // 3.0 or GL_ARB_uniform_buffer_object
            }
        }
    }
}

void AImmediateContextGLImpl::Draw( SDrawCmd const * _Cmd )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    if ( _Cmd->InstanceCount == 0 || _Cmd->VertexCountPerInstance == 0 ) {
        return;
    }

    UpdateVertexBuffers();
    UpdateShaderBindings();

    if ( _Cmd->InstanceCount == 1 && _Cmd->StartInstanceLocation == 0 ) {
        glDrawArrays( CurrentPipeline->PrimitiveTopology, _Cmd->StartVertexLocation, _Cmd->VertexCountPerInstance ); // Since 2.0
    } else {
        if ( _Cmd->StartInstanceLocation == 0 ) {
            glDrawArraysInstanced( CurrentPipeline->PrimitiveTopology, _Cmd->StartVertexLocation, _Cmd->VertexCountPerInstance, _Cmd->InstanceCount );// Since 3.1
        } else {
            glDrawArraysInstancedBaseInstance( CurrentPipeline->PrimitiveTopology,
                _Cmd->StartVertexLocation,
                _Cmd->VertexCountPerInstance,
                _Cmd->InstanceCount,
                _Cmd->StartInstanceLocation
            ); // Since 4.2 or GL_ARB_base_instance
        }
    }
}

void AImmediateContextGLImpl::Draw( SDrawIndexedCmd const * _Cmd )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    if ( _Cmd->InstanceCount == 0 || _Cmd->IndexCountPerInstance == 0 ) {
        return;
    }

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    const GLubyte * offset = reinterpret_cast<const GLubyte *>( 0 ) + _Cmd->StartIndexLocation * IndexBufferTypeSizeOf
        + IndexBufferOffset;

    if ( _Cmd->InstanceCount == 1 && _Cmd->StartInstanceLocation == 0 ) {
        if ( _Cmd->BaseVertexLocation == 0 ) {
            glDrawElements( CurrentPipeline->PrimitiveTopology,
                            _Cmd->IndexCountPerInstance,
                            IndexBufferType,
                            offset ); // 2.0
        } else {
            glDrawElementsBaseVertex( CurrentPipeline->PrimitiveTopology,
                                      _Cmd->IndexCountPerInstance,
                                      IndexBufferType,
                                      offset,
                                      _Cmd->BaseVertexLocation
                                      ); // 3.2 or GL_ARB_draw_elements_base_vertex
        }
    } else {
        if ( _Cmd->StartInstanceLocation == 0 ) {
            if ( _Cmd->BaseVertexLocation == 0 ) {
                glDrawElementsInstanced( CurrentPipeline->PrimitiveTopology,
                                         _Cmd->IndexCountPerInstance,
                                         IndexBufferType,
                                         offset,
                                         _Cmd->InstanceCount ); // 3.1
            } else {
                glDrawElementsInstancedBaseVertex( CurrentPipeline->PrimitiveTopology,
                                                   _Cmd->IndexCountPerInstance,
                                                   IndexBufferType,
                                                   offset,
                                                   _Cmd->InstanceCount,
                                                   _Cmd->BaseVertexLocation ); // 3.2 or GL_ARB_draw_elements_base_vertex
            }
        } else {
            if ( _Cmd->BaseVertexLocation == 0 ) {
                glDrawElementsInstancedBaseInstance( CurrentPipeline->PrimitiveTopology,
                                                     _Cmd->IndexCountPerInstance,
                                                     IndexBufferType,
                                                     offset,
                                                     _Cmd->InstanceCount,
                                                     _Cmd->StartInstanceLocation ); // 4.2 or GL_ARB_base_instance
            } else {
                glDrawElementsInstancedBaseVertexBaseInstance( CurrentPipeline->PrimitiveTopology,
                                                               _Cmd->IndexCountPerInstance,
                                                               IndexBufferType,
                                                               offset,
                                                               _Cmd->InstanceCount,
                                                               _Cmd->BaseVertexLocation,
                                                               _Cmd->StartInstanceLocation ); // 4.2 or GL_ARB_base_instance
            }
        }
    }
}

void AImmediateContextGLImpl::Draw( ITransformFeedback * _TransformFeedback, unsigned int _InstanceCount, unsigned int _StreamIndex )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    if ( _InstanceCount == 0 ) {
        return;
    }

    UpdateShaderBindings();

    if ( _InstanceCount > 1 ) {
        if ( _StreamIndex == 0 ) {
            glDrawTransformFeedbackInstanced( CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL(), _InstanceCount ); // 4.2
        } else {
            glDrawTransformFeedbackStreamInstanced( CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL(), _StreamIndex, _InstanceCount ); // 4.2
        }
    } else {
        if ( _StreamIndex == 0 ) {
            glDrawTransformFeedback( CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL() ); // 4.0
        } else {
            glDrawTransformFeedbackStream( CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL(), _StreamIndex );  // 4.0
        }
    }
}

void AImmediateContextGLImpl::DrawIndirect( SDrawIndirectCmd const * _Cmd )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexBuffers();
    UpdateShaderBindings();

    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawArraysInstancedBaseInstance
    glDrawArraysIndirect( CurrentPipeline->PrimitiveTopology, _Cmd ); // Since 4.0 or GL_ARB_draw_indirect
}

void AImmediateContextGLImpl::DrawIndirect( SDrawIndexedIndirectCmd const * _Cmd )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawElementsInstancedBaseVertexBaseInstance
    glDrawElementsIndirect( CurrentPipeline->PrimitiveTopology, IndexBufferType, _Cmd ); // Since 4.0 or GL_ARB_draw_indirect
}

void AImmediateContextGLImpl::DrawIndirect( IBuffer * _DrawIndirectBuffer, unsigned int _AlignedByteOffset, bool _Indexed )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    GLuint handle = _DrawIndirectBuffer->GetHandleNativeGL();
    if ( Binding.DrawInderectBuffer != handle ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, handle );
        Binding.DrawInderectBuffer = handle;
    }

    UpdateShaderBindings();

    if ( _Indexed ) {
        UpdateVertexAndIndexBuffers();

        // This is similar glDrawElementsInstancedBaseVertexBaseInstance, but with binded INDIRECT buffer
        glDrawElementsIndirect( CurrentPipeline->PrimitiveTopology,
                                IndexBufferType,
                                reinterpret_cast< const GLubyte * >(0) + _AlignedByteOffset ); // Since 4.0 or GL_ARB_draw_indirect
    } else {
        UpdateVertexBuffers();

        // This is similar glDrawArraysInstancedBaseInstance, but with binded INDIRECT buffer
        glDrawArraysIndirect( CurrentPipeline->PrimitiveTopology,
                              reinterpret_cast< const GLubyte * >(0) + _AlignedByteOffset ); // Since 4.0 or GL_ARB_draw_indirect
    }
}

void AImmediateContextGLImpl::MultiDraw( unsigned int _DrawCount, const unsigned int * _VertexCount, const unsigned int * _StartVertexLocations )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    static_assert( sizeof( _VertexCount[0] ) == sizeof( GLsizei ), "!" );
    static_assert( sizeof( _StartVertexLocations[0] ) == sizeof( GLint ), "!" );

    UpdateVertexBuffers();
    UpdateShaderBindings();

    glMultiDrawArrays( CurrentPipeline->PrimitiveTopology, ( const GLint * )_StartVertexLocations, ( const GLsizei * )_VertexCount, _DrawCount ); // Since 2.0

    // Эквивалентный код:
    //for ( unsigned int i = 0 ; i < _DrawCount ; i++ ) {
    //    glDrawArrays( CurrentPipeline->PrimitiveTopology, _StartVertexLocation[i], _VertexCount[i] );
    //}
}

void AImmediateContextGLImpl::MultiDraw( unsigned int _DrawCount, const unsigned int * _IndexCount, const void * const * _IndexByteOffsets, const int * _BaseVertexLocations )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    static_assert( sizeof( unsigned int ) == sizeof( GLsizei ), "!" );

    // FIXME: how to apply IndexBufferOffset?

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    if ( _BaseVertexLocations ) {
        glMultiDrawElementsBaseVertex( CurrentPipeline->PrimitiveTopology,
                                       ( const GLsizei * )_IndexCount,
                                       IndexBufferType,
                                       _IndexByteOffsets,
                                       _DrawCount,
                                       _BaseVertexLocations ); // 3.2
        // Эквивалентный код:
        //    for ( int i = 0 ; i < _DrawCount ; i++ )
        //        if ( _IndexCount[i] > 0 )
        //            glDrawElementsBaseVertex( CurrentPipeline->PrimitiveTopology,
        //                                      _IndexCount[i],
        //                                      IndexBufferType,
        //                                      _IndexByteOffsets[i],
        //                                      _BaseVertexLocations[i]);
    } else {
        glMultiDrawElements( CurrentPipeline->PrimitiveTopology,
                             ( const GLsizei * )_IndexCount,
                             IndexBufferType,
                             _IndexByteOffsets,
                             _DrawCount ); // 2.0
    }
}

void AImmediateContextGLImpl::MultiDrawIndirect( unsigned int _DrawCount, SDrawIndirectCmd const * _Cmds, unsigned int _Stride )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexBuffers();
    UpdateShaderBindings();

    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawArraysInstancedBaseInstance
    glMultiDrawArraysIndirect( CurrentPipeline->PrimitiveTopology,
                               _Cmds,
                               _DrawCount,
                               _Stride ); // 4.3 or GL_ARB_multi_draw_indirect


// Эквивалентный код:
//    GLsizei n;
//    for ( i = 0 ; i < _DrawCount ; i++ ) {
//        SDrawIndirectCmd const *cmd = ( _Stride != 0 ) ?
//              (SDrawIndirectCmd const *)((uintptr)indirect + i * _Stride) : ((SDrawIndirectCmd const *)indirect + i);
//        glDrawArraysInstancedBaseInstance(mode, cmd->first, cmd->count, cmd->instanceCount, cmd->baseInstance);
//    }
}

void AImmediateContextGLImpl::MultiDrawIndirect( unsigned int _DrawCount, SDrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    glMultiDrawElementsIndirect( CurrentPipeline->PrimitiveTopology,
                                 IndexBufferType,
                                 _Cmds,
                                 _DrawCount,
                                 _Stride ); // 4.3

// Эквивалентный код:
//    GLsizei n;
//    for ( i = 0 ; n < _DrawCount ; i++ ) {
//        SDrawIndexedIndirectCmd const *cmd = ( stride != 0 ) ?
//            (SDrawIndexedIndirectCmd const *)((uintptr)indirect + n * stride) : ( (SDrawIndexedIndirectCmd const *)indirect + n );
//        glDrawElementsInstancedBaseVertexBaseInstance(CurrentPipeline->PrimitiveTopology,
//                                                      cmd->count,
//                                                      IndexBufferType,
//                                                      cmd->firstIndex + size-of-type,
//                                                      cmd->instanceCount,
//                                                      cmd->baseVertex,
//                                                      cmd->baseInstance);
//    }
}

void AImmediateContextGLImpl::DispatchCompute( unsigned int _ThreadGroupCountX,
                                               unsigned int _ThreadGroupCountY,
                                               unsigned int _ThreadGroupCountZ )
{

    VerifyContext();

    // Must be: ThreadGroupCount <= GL_MAX_COMPUTE_WORK_GROUP_COUNT

    glDispatchCompute( _ThreadGroupCountX, _ThreadGroupCountY, _ThreadGroupCountZ ); // 4.3 or GL_ARB_compute_shader
}

void AImmediateContextGLImpl::DispatchCompute( SDispatchIndirectCmd const * _Cmd )
{
    VerifyContext();

    if ( Binding.DispatchIndirectBuffer != 0 ) {
        glBindBuffer( GL_DISPATCH_INDIRECT_BUFFER, 0 );
        Binding.DispatchIndirectBuffer = 0;
    }

    glDispatchComputeIndirect( ( GLintptr ) _Cmd ); // 4.3 or GL_ARB_compute_shader

    // or
    //glDispatchCompute( _Cmd->ThreadGroupCountX, _Cmd->ThreadGroupCountY, _Cmd->ThreadGroupCountZ ); // 4.3 or GL_ARB_compute_shader
}

void AImmediateContextGLImpl::DispatchComputeIndirect( IBuffer * _DispatchIndirectBuffer,
                                                       unsigned int _AlignedByteOffset )
{
    VerifyContext();

    GLuint handle = _DispatchIndirectBuffer->GetHandleNativeGL();
    if ( Binding.DispatchIndirectBuffer != handle ) {
        glBindBuffer( GL_DISPATCH_INDIRECT_BUFFER, handle );
        Binding.DispatchIndirectBuffer = handle;
    }

    GLubyte * offset = reinterpret_cast< GLubyte * >(0) + _AlignedByteOffset;
    glDispatchComputeIndirect( ( GLintptr ) offset ); // 4.3 or GL_ARB_compute_shader
}

void AImmediateContextGLImpl::BeginQuery( IQueryPool * _QueryPool, uint32_t _QueryID, uint32_t _StreamIndex )
{
    VerifyContext();

    AQueryPoolGLImpl * queryPool = static_cast< AQueryPoolGLImpl * >( _QueryPool );

    AN_ASSERT( _QueryID < queryPool->PoolSize );
    AN_ASSERT( queryPool->QueryType != QUERY_TYPE_TIMESTAMP );

    if ( CurrentQueryUID[queryPool->QueryType] != 0 ) {
        GLogger.Printf( "AImmediateContextGLImpl::BeginQuery: missing EndQuery() for the target\n" );
        return;
    }

    CurrentQueryUID[queryPool->QueryType] = queryPool->GetUID();

    if ( _StreamIndex == 0 ) {
        glBeginQuery( TableQueryTarget[queryPool->QueryType], queryPool->IdPool[_QueryID] ); // 2.0
    } else {
        glBeginQueryIndexed( TableQueryTarget[queryPool->QueryType], queryPool->IdPool[_QueryID], _StreamIndex ); // 4.0
    }
}

void AImmediateContextGLImpl::EndQuery( IQueryPool * _QueryPool, uint32_t _StreamIndex )
{
    VerifyContext();

    AQueryPoolGLImpl * queryPool = static_cast< AQueryPoolGLImpl * >(_QueryPool);

    AN_ASSERT( queryPool->QueryType != QUERY_TYPE_TIMESTAMP );

    if ( CurrentQueryUID[queryPool->QueryType] != _QueryPool->GetUID() ) {
        GLogger.Printf( "AImmediateContextGLImpl::EndQuery: missing BeginQuery() for the target\n" );
        return;
    }

    CurrentQueryUID[queryPool->QueryType] = 0;

    if ( _StreamIndex == 0 ) {
        glEndQuery( TableQueryTarget[queryPool->QueryType] ); // 2.0
    } else {
        glEndQueryIndexed( TableQueryTarget[queryPool->QueryType], _StreamIndex ); // 4.0
    }
}

void AImmediateContextGLImpl::RecordTimeStamp( IQueryPool * _QueryPool, uint32_t _QueryID )
{
    VerifyContext();

    AQueryPoolGLImpl * queryPool = static_cast< AQueryPoolGLImpl * >(_QueryPool);

    AN_ASSERT( _QueryID < queryPool->PoolSize );

    if ( queryPool->QueryType != QUERY_TYPE_TIMESTAMP ) {
        GLogger.Printf( "AImmediateContextGLImpl::RecordTimeStamp: query pool target must be QUERY_TYPE_TIMESTAMP\n" );
        return;
    }

    glQueryCounter( queryPool->IdPool[_QueryID], GL_TIMESTAMP );
}

void AImmediateContextGLImpl::BeginConditionalRender( IQueryPool * _QueryPool, uint32_t _QueryID, CONDITIONAL_RENDER_MODE _Mode )
{
    VerifyContext();

    AQueryPoolGLImpl * queryPool = static_cast< AQueryPoolGLImpl * >(_QueryPool);

    AN_ASSERT( _QueryID < queryPool->PoolSize );
    glBeginConditionalRender( queryPool->IdPool[ _QueryID ], TableConditionalRenderMode[ _Mode ] ); // 4.4 (with some flags 3.0)
}

void AImmediateContextGLImpl::EndConditionalRender()
{
    VerifyContext();

    glEndConditionalRender();  // 3.0
}

void AImmediateContextGLImpl::CopyQueryPoolResultsAvailable( IQueryPool * _QueryPool,
                                                             uint32_t _FirstQuery,
                                                             uint32_t _QueryCount,
                                                             IBuffer * _DstBuffer,
                                                             size_t _DstOffst,
                                                             size_t _DstStride,
                                                             bool _QueryResult64Bit )
{
    VerifyContext();

    AQueryPoolGLImpl * queryPool = static_cast< AQueryPoolGLImpl * >(_QueryPool);

    AN_ASSERT( _FirstQuery + _QueryCount <= queryPool->PoolSize );

    const GLuint bufferId = _DstBuffer->GetHandleNativeGL();
    const size_t bufferSize = _DstBuffer->GetSizeInBytes();

    if ( _QueryResult64Bit ) {

        AN_ASSERT( ( _DstStride & ~(size_t)7 ) == _DstStride ); // check stride must be multiples of 8

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint64_t) > bufferSize ) {
                GLogger.Printf( "AImmediateContextGLImpl::CopyQueryPoolResultsAvailable: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectui64v( queryPool->IdPool[ _FirstQuery + index ], bufferId, GL_QUERY_RESULT_AVAILABLE, _DstOffst );  // 4.5

            _DstOffst += _DstStride;
        }
    }
    else {
        AN_ASSERT( ( _DstStride & ~(size_t)3 ) == _DstStride ); // check stride must be multiples of 4

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint32_t) > bufferSize ) {
                GLogger.Printf( "AImmediateContextGLImpl::CopyQueryPoolResultsAvailable: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectuiv( queryPool->IdPool[ _FirstQuery + index ], bufferId, GL_QUERY_RESULT_AVAILABLE, _DstOffst );  // 4.5

            _DstOffst += _DstStride;
        }
    }
}

void AImmediateContextGLImpl::CopyQueryPoolResults( IQueryPool * _QueryPool,
                                                    uint32_t _FirstQuery,
                                                    uint32_t _QueryCount,
                                                    IBuffer * _DstBuffer,
                                                    size_t _DstOffst,
                                                    size_t _DstStride,
                                                    QUERY_RESULT_FLAGS _Flags )
{
    VerifyContext();

    AQueryPoolGLImpl * queryPool = static_cast< AQueryPoolGLImpl * >(_QueryPool);

    AN_ASSERT( _FirstQuery + _QueryCount <= queryPool->PoolSize );

    const GLuint bufferId = _DstBuffer->GetHandleNativeGL();
    const size_t bufferSize = _DstBuffer->GetSizeInBytes();

    GLenum pname = ( _Flags & QUERY_RESULT_WAIT_BIT ) ? GL_QUERY_RESULT : GL_QUERY_RESULT_NO_WAIT;

    if ( _Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT ) {
        GLogger.Printf( "AImmediateContextGLImpl::CopyQueryPoolResults: ignoring flag QUERY_RESULT_WITH_AVAILABILITY_BIT. Use CopyQueryPoolResultsAvailable to get available status.\n" );
    }

    if ( _Flags & QUERY_RESULT_64_BIT ) {

        AN_ASSERT( ( _DstStride & ~(size_t)7 ) == _DstStride ); // check stride must be multiples of 8

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint64_t) > bufferSize ) {
                GLogger.Printf( "AImmediateContextGLImpl::CopyQueryPoolResults: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectui64v( queryPool->IdPool[ _FirstQuery + index ], bufferId, pname, _DstOffst ); // 4.5

            _DstOffst += _DstStride;
        }
    }
    else {
        AN_ASSERT( ( _DstStride & ~(size_t)3 ) == _DstStride ); // check stride must be multiples of 4

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint32_t) > bufferSize ) {
                GLogger.Printf( "AImmediateContextGLImpl::CopyQueryPoolResults: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectuiv( queryPool->IdPool[ _FirstQuery + index ], bufferId, pname, _DstOffst ); // 4.5

            _DstOffst += _DstStride;
        }
    }
}

void AImmediateContextGLImpl::BeginRenderPassDefaultFramebuffer( SRenderPassBegin const & _RenderPassBegin )
{
    VerifyContext();

    if ( Binding.DrawFramebufferUID != DefaultFramebuffer->GetUID() ) {
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, DefaultFramebuffer->GetHandleNativeGL() );

        Binding.DrawFramebuffer = DefaultFramebuffer->GetHandleNativeGL();
        Binding.DrawFramebufferUID = DefaultFramebuffer->GetUID();
        Binding.DrawFramebufferWidth = (unsigned short)SwapChainWidth;
        Binding.DrawFramebufferHeight = ( unsigned short )SwapChainHeight;
    }

    const int framebufferId = 0;

    bool bScissorEnabled = RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = RasterizerState.bRasterizerDiscard;

    ARenderPassGLImpl const * renderPass = static_cast< ARenderPassGLImpl const * >( _RenderPassBegin.pRenderPass );

    if ( renderPass->NumColorAttachments > 0 ) {

        SAttachmentInfo const * attachment = &renderPass->ColorAttachments[ 0 ];
        //FramebufferAttachmentInfo const * framebufferAttachment = &framebuffer->ColorAttachments[ i ];

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {

            AN_ASSERT( _RenderPassBegin.pColorClearValues != nullptr );

            SClearColorValue const * clearValue = &_RenderPassBegin.pColorClearValues[ 0 ];

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }


            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            SRenderTargetBlendingInfo const & currentState = BlendState.RenderTargetSlots[ 0 ];
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                glColorMaski( 0, 1, 1, 1, 1 );
            }

            glClearNamedFramebufferfv( framebufferId,
                                       GL_COLOR,
                                       0,
                                       clearValue->Float32 );

            // Restore color mask
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                if ( currentState.ColorWriteMask == COLOR_WRITE_DISABLED ) {
                    glColorMaski( 0, 0, 0, 0, 0 );
                } else {
                    glColorMaski( 0,
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_R_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_G_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_B_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_A_BIT) );
                }
            }
        }
    }

    if ( renderPass->bHasDepthStencilAttachment ) {
        SAttachmentInfo const * attachment = &renderPass->DepthStencilAttachment;
        //FramebufferAttachmentInfo const * framebufferAttachment = &framebuffer->DepthStencilAttachment;

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {
            SClearDepthStencilValue const * clearValue = _RenderPassBegin.pDepthStencilClearValue;

            AN_ASSERT( clearValue != nullptr );

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }

            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            if ( DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 1 );
            }

            //glClearNamedFramebufferuiv( framebufferId,
            //                            GL_STENCIL,
            //                            0,
            //                            &clearValue->Stencil );

            glClearNamedFramebufferfv( framebufferId,
                                       GL_DEPTH,
                                       0,
                                       &clearValue->Depth );

            //glClearNamedFramebufferfi( framebufferId,
            //                           GL_DEPTH_STENCIL,
            //                           0,
            //                           clearValue->Depth,
            //                           clearValue->Stencil );

            if ( DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 0 );
            }
        }
    }

    // Restore scissor test
    if ( bScissorEnabled != RasterizerState.bScissorEnable ) {
        if ( RasterizerState.bScissorEnable ) {
            glEnable( GL_SCISSOR_TEST );
        } else {
            glDisable( GL_SCISSOR_TEST );
        }
    }

    // Restore rasterizer discard
    if ( bRasterizerDiscard != RasterizerState.bRasterizerDiscard ) {
        if ( RasterizerState.bRasterizerDiscard ) {
            glEnable( GL_RASTERIZER_DISCARD );
        } else {
            glDisable( GL_RASTERIZER_DISCARD );
        }
    }
}

void AImmediateContextGLImpl::BeginRenderPass( SRenderPassBegin const & _RenderPassBegin )
{
    VerifyContext();

    ARenderPassGLImpl const * renderPass = static_cast< ARenderPassGLImpl const * >( _RenderPassBegin.pRenderPass );
    IFramebuffer const * framebuffer = _RenderPassBegin.pFramebuffer;

    AN_ASSERT( CurrentRenderPass == nullptr );

    CurrentRenderPass = renderPass;
    CurrentSubpass = -1;
    CurrentRenderPassRenderArea = _RenderPassBegin.RenderArea;
    CurrentPipeline = nullptr;

    GLuint framebufferId = framebuffer->GetHandleNativeGL();

    if ( !framebufferId ) {
        // default framebuffer
        BeginRenderPassDefaultFramebuffer( _RenderPassBegin );
        return;
    }

    if ( Binding.DrawFramebufferUID != framebuffer->GetUID() ) {
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebufferId );

        Binding.DrawFramebuffer = framebufferId;
        Binding.DrawFramebufferUID = framebuffer->GetUID();
        Binding.DrawFramebufferWidth = framebuffer->GetWidth();
        Binding.DrawFramebufferHeight = framebuffer->GetHeight();
    }

    bool bScissorEnabled = RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = RasterizerState.bRasterizerDiscard;

    SFramebufferAttachmentInfo const * framebufferColorAttachments = framebuffer->GetColorAttachments();

    //// We must set draw buffers to clear attachment :(
    //for ( uint32_t i = 0 ; i < renderPass->NumColorAttachments ; i++ ) {
    //    Attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    //}
    //glNamedFramebufferDrawBuffers( framebufferId, renderPass->NumColorAttachments, Attachments );

    static SClearColorValue defaultClearValue = {};

    for ( unsigned int i = 0 ; i < renderPass->NumColorAttachments ; i++ ) {

        SAttachmentInfo const * attachment = &renderPass->ColorAttachments[ i ];
        SFramebufferAttachmentInfo const & framebufferAttachment = framebufferColorAttachments[ i ];

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {

            // We must set draw buffers to clear attachment :(
            glNamedFramebufferDrawBuffer( framebufferId, GL_COLOR_ATTACHMENT0 + i );

            SClearColorValue const * clearValue = _RenderPassBegin.pColorClearValues
                ? &_RenderPassBegin.pColorClearValues[ i ]
                : &defaultClearValue;

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }


            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            int drawbufferNum = 0;//i;  // FIXME: is this correct?

            SRenderTargetBlendingInfo const & currentState = BlendState.RenderTargetSlots[ i ];
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                glColorMaski( drawbufferNum, 1, 1, 1, 1 );
            }

            // Clear attachment
            switch ( InternalFormatLUT[ framebufferAttachment.pTexture->GetFormat() ].ClearType ) {
            case CLEAR_TYPE_FLOAT32:
                glClearNamedFramebufferfv( framebufferId,
                                           GL_COLOR,
                                           drawbufferNum,
                                           clearValue->Float32 );
                break;
            case CLEAR_TYPE_INT32:
                glClearNamedFramebufferiv( framebufferId,
                                           GL_COLOR,
                                           drawbufferNum,
                                           clearValue->Int32 );
                break;
            case CLEAR_TYPE_UINT32:
                glClearNamedFramebufferuiv( framebufferId,
                                            GL_COLOR,
                                            drawbufferNum,
                                            clearValue->UInt32 );
                break;
            default:
                AN_ASSERT( 0 );
            }

            // Restore color mask
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                if ( currentState.ColorWriteMask == COLOR_WRITE_DISABLED ) {
                    glColorMaski( drawbufferNum, 0, 0, 0, 0 );
                } else {
                    glColorMaski( drawbufferNum,
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_R_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_G_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_B_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_A_BIT) );
                }
            }
        }
    }

    if ( renderPass->bHasDepthStencilAttachment ) {
        SAttachmentInfo const * attachment = &renderPass->DepthStencilAttachment;
        SFramebufferAttachmentInfo const & framebufferAttachment = framebuffer->GetDepthStencilAttachment();

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {
            SClearDepthStencilValue const * clearValue = _RenderPassBegin.pDepthStencilClearValue;

            AN_ASSERT( clearValue != nullptr );

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }

            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            if ( DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 1 );
            }

            // TODO: table
            switch ( InternalFormatLUT[ framebufferAttachment.pTexture->GetFormat() ].ClearType ) {
            case CLEAR_TYPE_STENCIL_ONLY:
                glClearNamedFramebufferuiv( framebufferId,
                                            GL_STENCIL,
                                            0,
                                            &clearValue->Stencil );
                break;
            case CLEAR_TYPE_DEPTH_ONLY:
                glClearNamedFramebufferfv( framebufferId,
                                           GL_DEPTH,
                                           0,
                                           &clearValue->Depth );
                break;
            case CLEAR_TYPE_DEPTH_STENCIL:
                glClearNamedFramebufferfi( framebufferId,
                                           GL_DEPTH_STENCIL,
                                           0,
                                           clearValue->Depth,
                                           clearValue->Stencil );
                break;
            default:
                AN_ASSERT( 0 );
            }

            if ( DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 0 );
            }
        }
    }

    // Restore scissor test
    if ( bScissorEnabled != RasterizerState.bScissorEnable ) {
        if ( RasterizerState.bScissorEnable ) {
            glEnable( GL_SCISSOR_TEST );
        } else {
            glDisable( GL_SCISSOR_TEST );
        }
    }

    // Restore rasterizer discard
    if ( bRasterizerDiscard != RasterizerState.bRasterizerDiscard ) {
        if ( RasterizerState.bRasterizerDiscard ) {
            glEnable( GL_RASTERIZER_DISCARD );
        } else {
            glDisable( GL_RASTERIZER_DISCARD );
        }
    }
}

void AImmediateContextGLImpl::EndRenderPass()
{
    VerifyContext();

    CurrentRenderPass = nullptr;
}

void AImmediateContextGLImpl::BindTransformFeedback( ITransformFeedback * _TransformFeedback )
{
    VerifyContext();

    // FIXME: Move transform feedback to Pipeline? Call glBindTransformFeedback in BindPipeline()?
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, _TransformFeedback->GetHandleNativeGL() );
}

void AImmediateContextGLImpl::BeginTransformFeedback( PRIMITIVE_TOPOLOGY _OutputPrimitive )
{
    VerifyContext();

    GLenum topology = GL_POINTS;

    if ( _OutputPrimitive <= PRIMITIVE_TRIANGLE_STRIP_ADJ ) {
        topology = PrimitiveTopologyLUT[ _OutputPrimitive ];
    }

    glBeginTransformFeedback( topology ); // 3.0
}

void AImmediateContextGLImpl::ResumeTransformFeedback()
{
    VerifyContext();

    glResumeTransformFeedback();
}

void AImmediateContextGLImpl::PauseTransformFeedback()
{
    VerifyContext();

    glPauseTransformFeedback();
}

void AImmediateContextGLImpl::EndTransformFeedback()
{
    VerifyContext();

    glEndTransformFeedback(); // 3.0
}

SyncObject AImmediateContextGLImpl::FenceSync()
{
    VerifyContext();

    SyncObject sync = reinterpret_cast< SyncObject >( glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
    return sync;
}

void AImmediateContextGLImpl::RemoveSync( SyncObject _Sync )
{
    VerifyContext();

    if ( _Sync ) {
        glDeleteSync( reinterpret_cast< GLsync >( _Sync ) );
    }
}

CLIENT_WAIT_STATUS AImmediateContextGLImpl::ClientWait( SyncObject _Sync, uint64_t _TimeOutNanoseconds )
{
    VerifyContext();

    static_assert( 0xFFFFFFFFFFFFFFFF == GL_TIMEOUT_IGNORED, "Constant check" );
    return static_cast< CLIENT_WAIT_STATUS >(
                glClientWaitSync( reinterpret_cast< GLsync >( _Sync ), GL_SYNC_FLUSH_COMMANDS_BIT, _TimeOutNanoseconds ) - GL_ALREADY_SIGNALED );
}

void AImmediateContextGLImpl::ServerWait( SyncObject _Sync )
{
    VerifyContext();

    glWaitSync( reinterpret_cast< GLsync >( _Sync ), 0, GL_TIMEOUT_IGNORED );
}

bool AImmediateContextGLImpl::IsSignaled( SyncObject _Sync )
{
    VerifyContext();

    GLint value;
    glGetSynciv( reinterpret_cast< GLsync >( _Sync ),
                 GL_SYNC_STATUS,
                 sizeof( GLint ),
                 nullptr,
                 &value );
    return value == GL_SIGNALED;
}

void AImmediateContextGLImpl::Flush()
{
    VerifyContext();

    glFlush();
}

void AImmediateContextGLImpl::Barrier( int _BarrierBits )
{
    VerifyContext();

    glMemoryBarrier( _BarrierBits ); // 4.2
}

void AImmediateContextGLImpl::BarrierByRegion( int _BarrierBits )
{
    VerifyContext();

    glMemoryBarrierByRegion( _BarrierBits ); // 4.5
}

void AImmediateContextGLImpl::TextureBarrier()
{
    VerifyContext();

    glTextureBarrier(); // 4.5
}

void AImmediateContextGLImpl::DynamicState_BlendingColor( /* optional */ const float _ConstantColor[4] )
{
    VerifyContext();

    constexpr const float defaultColor[4] = { 0, 0, 0, 0 };

    // Validate blend color
    _ConstantColor = _ConstantColor ? _ConstantColor : defaultColor;

    // Apply blend color
    bool isColorChanged = !BlendCompareColor( BlendColor, _ConstantColor );
    if ( isColorChanged ) {
        glBlendColor( _ConstantColor[0], _ConstantColor[1], _ConstantColor[2], _ConstantColor[3] );
        memcpy( BlendColor, _ConstantColor, sizeof( BlendColor ) );
    }
}

void AImmediateContextGLImpl::DynamicState_SampleMask( /* optional */ const uint32_t _SampleMask[4] )
{
    VerifyContext();

    // Apply sample mask
    if ( _SampleMask ) {
        static_assert( sizeof( GLbitfield ) == sizeof( _SampleMask[0] ), "Type Sizeof check" );
        if ( _SampleMask[0] != SampleMask[0] ) { glSampleMaski( 0, _SampleMask[0] ); SampleMask[0] = _SampleMask[0]; }
        if ( _SampleMask[1] != SampleMask[1] ) { glSampleMaski( 1, _SampleMask[1] ); SampleMask[1] = _SampleMask[1]; }
        if ( _SampleMask[2] != SampleMask[2] ) { glSampleMaski( 2, _SampleMask[2] ); SampleMask[2] = _SampleMask[2]; }
        if ( _SampleMask[3] != SampleMask[3] ) { glSampleMaski( 3, _SampleMask[3] ); SampleMask[3] = _SampleMask[3]; }

        if ( !bSampleMaskEnabled ) {
            glEnable( GL_SAMPLE_MASK );
            bSampleMaskEnabled = true;
        }
    } else {
        if ( bSampleMaskEnabled ) {
            glDisable( GL_SAMPLE_MASK );
            bSampleMaskEnabled = false;
        }
    }
}

void AImmediateContextGLImpl::DynamicState_StencilRef( uint32_t _StencilRef )
{
    VerifyContext();

    AN_ASSERT( CurrentPipeline != nullptr );

    if ( Binding.DepthStencilState == CurrentPipeline->DepthStencilState ) {

        if ( StencilRef != _StencilRef ) {
            // Update stencil ref

            SDepthStencilStateInfo const & desc = *CurrentPipeline->DepthStencilState;

#if 0
            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                               _StencilRef,
                               desc.StencilReadMask );
            }
            else
#endif
            {
                if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                    glStencilFuncSeparate( GL_FRONT_AND_BACK,
                                           ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                           _StencilRef,
                                           desc.StencilReadMask );
                } else {
                    glStencilFuncSeparate( GL_FRONT,
                                           ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                           _StencilRef,
                                           desc.StencilReadMask );

                    glStencilFuncSeparate( GL_BACK,
                                           ComparisonFuncLUT[desc.BackFace.StencilFunc],
                                           _StencilRef,
                                           desc.StencilReadMask );
                }
            }

            StencilRef = _StencilRef;
        }
    }
}

void AImmediateContextGLImpl::SetLineWidth( float _Width )
{
    VerifyContext();

    glLineWidth( _Width );
}

void AImmediateContextGLImpl::CopyBuffer( IBuffer * _SrcBuffer, IBuffer * _DstBuffer )
{
    VerifyContext();

    size_t size = _SrcBuffer->GetSizeInBytes();
    AN_ASSERT( size == _DstBuffer->GetSizeInBytes() );

    glCopyNamedBufferSubData( _SrcBuffer->GetHandleNativeGL(),
                              _DstBuffer->GetHandleNativeGL(),
                              0,
                              0,
                              size ); // 4.5 or GL_ARB_direct_state_access
}

void AImmediateContextGLImpl::CopyBufferRange( IBuffer * _SrcBuffer, IBuffer * _DstBuffer, uint32_t _NumRanges, SBufferCopy const * _Ranges )
{
    VerifyContext();

    for ( SBufferCopy const * range = _Ranges ; range < &_Ranges[_NumRanges] ; range++ ) {
        glCopyNamedBufferSubData( _SrcBuffer->GetHandleNativeGL(),
                                  _DstBuffer->GetHandleNativeGL(),
                                  range->SrcOffset,
                                  range->DstOffset,
                                  range->SizeInBytes ); // 4.5 or GL_ARB_direct_state_access
    }


    /*
    // Other path:

    glBindBuffer( GL_COPY_READ_BUFFER, srcId );
    glBindBuffer( GL_COPY_WRITE_BUFFER, dstId );

    glCopyBufferSubData( GL_COPY_READ_BUFFER,
                         GL_COPY_WRITE_BUFFER,
                         _SourceOffset,
                         _DstOffset,
                         _SizeInBytes );  // 3.1
    */
}

// Only for TEXTURE_1D
bool AImmediateContextGLImpl::CopyBufferToTexture1D( IBuffer const * _SrcBuffer,
                                                     ITexture * _DstTexture,
                                                     uint16_t _Lod,
                                                     uint16_t _OffsetX,
                                                     uint16_t _DimensionX,
                                                     size_t _CompressedDataSizeInBytes, // Only for compressed images
                                                     DATA_FORMAT _Format,
                                                     size_t _SourceByteOffset,
                                                     unsigned int _Alignment )
{
    VerifyContext();

    if ( _DstTexture->GetType() != TEXTURE_1D ) {
        return false;
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _SrcBuffer->GetHandleNativeGL() );

    // TODO: check this

    GLuint textureId = _DstTexture->GetHandleNativeGL();

    UnpackAlignment( _Alignment );

    if ( _DstTexture->IsCompressed() ) {
        glCompressedTextureSubImage1D( textureId,
                                       _Lod,
                                       _OffsetX,
                                       _DimensionX,
                                       InternalFormatLUT[ _DstTexture->GetFormat() ].InternalFormat,
                                       (GLsizei)_CompressedDataSizeInBytes,
                                       ((uint8_t *)0) + _SourceByteOffset );
    } else {
        glTextureSubImage1D( textureId,
                             _Lod,
                             _OffsetX,
                             _DimensionX,
                             TypeLUT[_Format].FormatRGB,
                             TypeLUT[_Format].Type,
                             ((uint8_t *)0) + _SourceByteOffset );
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

    return true;
}

// Only for TEXTURE_2D, TEXTURE_1D_ARRAY, TEXTURE_CUBE_MAP
bool AImmediateContextGLImpl::CopyBufferToTexture2D( IBuffer const * _SrcBuffer,
                                                     ITexture * _DstTexture,
                                                     uint16_t _Lod,
                                                     uint16_t _OffsetX,
                                                     uint16_t _OffsetY,
                                                     uint16_t _DimensionX,
                                                     uint16_t _DimensionY,
                                                     uint16_t _CubeFaceIndex, // only for TEXTURE_CUBE_MAP
                                                     uint16_t _NumCubeFaces, // only for TEXTURE_CUBE_MAP
                                                     size_t _CompressedDataSizeInBytes, // Only for compressed images
                                                     DATA_FORMAT _Format,
                                                     size_t _SourceByteOffset,
                                                     unsigned int _Alignment )
{
    VerifyContext();

    if ( _DstTexture->GetType() != TEXTURE_2D && _DstTexture->GetType() != TEXTURE_1D_ARRAY && _DstTexture->GetType() != TEXTURE_CUBE_MAP ) {
        return false;
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _SrcBuffer->GetHandleNativeGL() );

    // TODO: check this

    GLuint textureId = _DstTexture->GetHandleNativeGL();

    UnpackAlignment( _Alignment );

    if ( _DstTexture->GetType() == TEXTURE_CUBE_MAP ) {

        GLint i;
        GLuint currentBinding;

        glGetIntegerv( GL_TEXTURE_BINDING_CUBE_MAP, &i );

        currentBinding = i;

        if ( currentBinding != textureId ) {
            glBindTexture( GL_TEXTURE_CUBE_MAP, textureId );
        }

        // TODO: Учитывать _NumCubeFaces!!!

        if ( _DstTexture->IsCompressed() ) {
            glCompressedTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + _CubeFaceIndex,
                                       _Lod,
                                       _OffsetX,
                                       _OffsetY,
                                       _DimensionX,
                                       _DimensionY,
                                       InternalFormatLUT[ _DstTexture->GetFormat() ].InternalFormat,
                                       (GLsizei)_CompressedDataSizeInBytes,
                                       ((uint8_t *)0) + _SourceByteOffset );
        } else {
            glTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + _CubeFaceIndex,
                             _Lod,
                             _OffsetX,
                             _OffsetY,
                             _DimensionX,
                             _DimensionY,
                             TypeLUT[_Format].FormatRGB,
                             TypeLUT[_Format].Type,
                             ((uint8_t *)0) + _SourceByteOffset );
        }

        if ( currentBinding != textureId ) {
            glBindTexture( GL_TEXTURE_CUBE_MAP, currentBinding );
        }
    } else {
        if ( _DstTexture->IsCompressed() ) {
            glCompressedTextureSubImage2D( textureId,
                                           _Lod,
                                           _OffsetX,
                                           _OffsetY,
                                           _DimensionX,
                                           _DimensionY,
                                           InternalFormatLUT[ _DstTexture->GetFormat() ].InternalFormat,
                                           (GLsizei)_CompressedDataSizeInBytes,
                                           ((uint8_t *)0) + _SourceByteOffset );
        } else {
            glTextureSubImage2D( textureId,
                                 _Lod,
                                 _OffsetX,
                                 _OffsetY,
                                 _DimensionX,
                                 _DimensionY,
                                 TypeLUT[_Format].FormatRGB,
                                 TypeLUT[_Format].Type,
                                 ((uint8_t *)0) + _SourceByteOffset );
        }
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

    return true;
}

// Only for TEXTURE_3D, TEXTURE_2D_ARRAY
bool AImmediateContextGLImpl::CopyBufferToTexture3D( IBuffer const * _SrcBuffer,
                                                     ITexture * _DstTexture,
                                                     uint16_t _Lod,
                                                     uint16_t _OffsetX,
                                                     uint16_t _OffsetY,
                                                     uint16_t _OffsetZ,
                                                     uint16_t _DimensionX,
                                                     uint16_t _DimensionY,
                                                     uint16_t _DimensionZ,
                                                     size_t _CompressedDataSizeInBytes, // Only for compressed images
                                                     DATA_FORMAT _Format,
                                                     size_t _SourceByteOffset,
                                                     unsigned int _Alignment )
{
    VerifyContext();

    if ( _DstTexture->GetType() != TEXTURE_3D && _DstTexture->GetType() != TEXTURE_2D_ARRAY ) {
        return false;
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _SrcBuffer->GetHandleNativeGL() );

    // TODO: check this

    GLuint textureId = _DstTexture->GetHandleNativeGL();

    UnpackAlignment( _Alignment );

    if ( _DstTexture->IsCompressed() ) {
        glCompressedTextureSubImage3D( textureId,
                                       _Lod,
                                       _OffsetX,
                                       _OffsetY,
                                       _OffsetZ,
                                       _DimensionX,
                                       _DimensionY,
                                       _DimensionZ,
                                       InternalFormatLUT[ _DstTexture->GetFormat() ].InternalFormat,
                                       (GLsizei)_CompressedDataSizeInBytes,
                                       ((uint8_t *)0) + _SourceByteOffset );
    } else {
        glTextureSubImage3D( textureId,
                             _Lod,
                             _OffsetX,
                             _OffsetY,
                             _OffsetZ,
                             _DimensionX,
                             _DimensionY,
                             _DimensionZ,
                             TypeLUT[_Format].FormatRGB,
                             TypeLUT[_Format].Type,
                             ((uint8_t *)0) + _SourceByteOffset );
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

    return true;
}

// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP
bool AImmediateContextGLImpl::CopyBufferToTexture( IBuffer const * _SrcBuffer,
                                                   ITexture * _DstTexture,
                                                   STextureRect const & _Rectangle,
                                                   DATA_FORMAT _Format,
                                                   size_t _CompressedDataSizeInBytes,       // for compressed images
                                                   size_t _SourceByteOffset,
                                                   unsigned int _Alignment )
{
    VerifyContext();

    // FIXME: what about multisample textures?

    switch ( _DstTexture->GetType() ) {
    case TEXTURE_1D:
        return CopyBufferToTexture1D( _SrcBuffer,
                                      _DstTexture,
                                      _Rectangle.Offset.Lod,
                                      _Rectangle.Offset.X,
                                      _Rectangle.Dimension.X,
                                      _CompressedDataSizeInBytes,
                                      _Format,
                                      _SourceByteOffset,
                                      _Alignment );
    case TEXTURE_1D_ARRAY:
    case TEXTURE_2D:
        return CopyBufferToTexture2D( _SrcBuffer,
                                      _DstTexture,
                                      _Rectangle.Offset.Lod,
                                      _Rectangle.Offset.X,
                                      _Rectangle.Offset.Y,
                                      _Rectangle.Dimension.X,
                                      _Rectangle.Dimension.Y,
                                      0,
                                      0,
                                      _CompressedDataSizeInBytes,
                                      _Format,
                                      _SourceByteOffset,
                                      _Alignment );
    case TEXTURE_2D_ARRAY:
    case TEXTURE_3D:
        return CopyBufferToTexture3D( _SrcBuffer,
                                      _DstTexture,
                                      _Rectangle.Offset.Lod,
                                      _Rectangle.Offset.X,
                                      _Rectangle.Offset.Y,
                                      _Rectangle.Offset.Z,
                                      _Rectangle.Dimension.X,
                                      _Rectangle.Dimension.Y,
                                      _Rectangle.Dimension.Z,
                                      _CompressedDataSizeInBytes,
                                      _Format,
                                      _SourceByteOffset,
                                      _Alignment );
    case TEXTURE_CUBE_MAP:
        return CopyBufferToTexture2D( _SrcBuffer,
                                      _DstTexture,
                                      _Rectangle.Offset.Lod,
                                      _Rectangle.Offset.X,
                                      _Rectangle.Offset.Y,
                                      _Rectangle.Dimension.X,
                                      _Rectangle.Dimension.Y,
                                      _Rectangle.Offset.Z,
                                      _Rectangle.Dimension.Z,
                                      _CompressedDataSizeInBytes,
                                      _Format,
                                      _SourceByteOffset,
                                      _Alignment );
    case TEXTURE_CUBE_MAP_ARRAY:
        // FIXME: ???
        //return CopyBufferToTexture3D( static_cast< Buffer const * >( _SrcBuffer ),
        //                              static_cast< Texture * >( _DstTexture ),
        //                              _Rectangle.Offset.Lod,
        //                              _Rectangle.Offset.X,
        //                              _Rectangle.Offset.Y,
        //                              _Rectangle.Offset.Z,
        //                              _Rectangle.Dimension.X,
        //                              _Rectangle.Dimension.Y,
        //                              _Rectangle.Dimension.Z,
        //                              _CompressedDataSizeInBytes,
        //                              _Format,
        //                              _SourceByteOffset );
        return false;
    case TEXTURE_RECT_GL:
        // FIXME: ???
        return false;
    default:
        break;
    }

    return false;
}

void AImmediateContextGLImpl::CopyTextureToBuffer( ITexture const * _SrcTexture,
                                                   IBuffer * _DstBuffer,
                                                   STextureRect const & _Rectangle,
                                                   DATA_FORMAT _Format,
                                                   size_t _SizeInBytes,
                                                   size_t _DstByteOffset,
                                                   unsigned int _Alignment )
{
    VerifyContext();

    glBindBuffer( GL_PIXEL_PACK_BUFFER, _DstBuffer->GetHandleNativeGL() );

    // TODO: check this

    GLuint textureId = _SrcTexture->GetHandleNativeGL();

    PackAlignment( _Alignment );

    if ( _SrcTexture->IsCompressed() ) {
        glGetCompressedTextureSubImage( textureId,
                                        _Rectangle.Offset.Lod,
                                        _Rectangle.Offset.X,
                                        _Rectangle.Offset.Y,
                                        _Rectangle.Offset.Z,
                                        _Rectangle.Dimension.X,
                                        _Rectangle.Dimension.Y,
                                        _Rectangle.Dimension.Z,
                                        (GLsizei)_SizeInBytes,
                                        ((uint8_t *)0) + _DstByteOffset );

    } else {
        glGetTextureSubImage( textureId,
                              _Rectangle.Offset.Lod,
                              _Rectangle.Offset.X,
                              _Rectangle.Offset.Y,
                              _Rectangle.Offset.Z,
                              _Rectangle.Dimension.X,
                              _Rectangle.Dimension.Y,
                              _Rectangle.Dimension.Z,
                              TypeLUT[_Format].FormatRGB,
                              TypeLUT[_Format].Type,
                              (GLsizei)_SizeInBytes,
                              ((uint8_t *)0) + _DstByteOffset );
    }

    glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
}

void AImmediateContextGLImpl::CopyTextureRect( ITexture const * _SrcTexture,
                                               ITexture * _DstTexture,
                                               uint32_t _NumCopies,
                                               TextureCopy const * Copies )
{
    VerifyContext();

    // TODO: check this

    GLenum srcTarget = TextureTargetLUT[ _SrcTexture->GetType() ].Target;
    GLenum dstTarget = TextureTargetLUT[ _DstTexture->GetType() ].Target;
    GLuint srcId = _SrcTexture->GetHandleNativeGL();
    GLuint dstId = _DstTexture->GetHandleNativeGL();

    if ( _SrcTexture->IsMultisample() ) {
        if ( srcTarget == GL_TEXTURE_2D ) srcTarget = GL_TEXTURE_2D_MULTISAMPLE;
        if ( srcTarget == GL_TEXTURE_2D_ARRAY ) srcTarget = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    }
    if ( _DstTexture->IsMultisample() ) {
        if ( dstTarget == GL_TEXTURE_2D ) dstTarget = GL_TEXTURE_2D_MULTISAMPLE;
        if ( dstTarget == GL_TEXTURE_2D_ARRAY ) dstTarget = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    }

    for ( TextureCopy const * copy = Copies ; copy < &Copies[_NumCopies] ; copy++ ) {
        glCopyImageSubData( srcId,
                            srcTarget,
                            copy->SrcRect.Offset.Lod,
                            copy->SrcRect.Offset.X,
                            copy->SrcRect.Offset.Y,
                            copy->SrcRect.Offset.Z,
                            dstId,
                            dstTarget,
                            copy->DstOffset.Lod,
                            copy->DstOffset.X,
                            copy->DstOffset.Y,
                            copy->DstOffset.Z,
                            copy->SrcRect.Dimension.X,
                            copy->SrcRect.Dimension.Y,
                            copy->SrcRect.Dimension.Z );
    }
}

bool AImmediateContextGLImpl::CopyFramebufferToTexture( IFramebuffer const * _SrcFramebuffer,
                                                        ITexture * _DstTexture,
                                                        FRAMEBUFFER_ATTACHMENT _Attachment,
                                                        STextureOffset const & _Offset,
                                                        SRect2D const & _SrcRect,
                                                        unsigned int _Alignment )
{            // Specifies alignment of destination data
    VerifyContext();

    AFramebufferGLImpl const * framebuffer = static_cast< AFramebufferGLImpl const * >( _SrcFramebuffer );

    if ( !framebuffer->ChooseReadBuffer( _Attachment ) ) {
        GLogger.Printf( "AImmediateContextGLImpl::CopyFramebufferToTexture: invalid framebuffer attachment\n" );
        return false;
    }

    PackAlignment( _Alignment );

    BindReadFramebuffer( framebuffer );

    // TODO: check this function

    if ( _DstTexture->IsMultisample() ) {
        switch ( _DstTexture->GetType() ) {
        case TEXTURE_2D:
        case TEXTURE_2D_ARRAY:
            // FIXME: в спецификации про multisample-типы ничего не сказано
            return false;
        default:;
        }
    }

    switch ( _DstTexture->GetType() ) {
    case TEXTURE_1D:
    {
        glCopyTextureSubImage1D( _DstTexture->GetHandleNativeGL(),
                                 _Offset.Lod,
                                 _Offset.X,
                                 _SrcRect.X,
                                 _SrcRect.Y,
                                 _SrcRect.Width );
        break;
    }
    case TEXTURE_1D_ARRAY:
    case TEXTURE_2D:
    {
        glCopyTextureSubImage2D( _DstTexture->GetHandleNativeGL(),
                                 _Offset.Lod,
                                 _Offset.X,
                                 _Offset.Y,
                                 _SrcRect.X,
                                 _SrcRect.Y,
                                 _SrcRect.Width,
                                 _SrcRect.Height );
        break;
    }
    case TEXTURE_2D_ARRAY:
    case TEXTURE_3D:
    {
        glCopyTextureSubImage3D( _DstTexture->GetHandleNativeGL(),
                                 _Offset.Lod,
                                 _Offset.X,
                                 _Offset.Y,
                                 _Offset.Z,
                                 _SrcRect.X,
                                 _SrcRect.Y,
                                 _SrcRect.Width,
                                 _SrcRect.Height );
        break;
    }
    case TEXTURE_CUBE_MAP:
    {
        // FIXME: в спецификации не сказано, как с помощью glCopyTextureSubImage2D
        // скопировать в грань кубической текстуры, поэтому используем обходной путь
        // через glCopyTexSubImage2D

        GLint currentBinding;
        GLint id = _DstTexture->GetHandleNativeGL();

        glGetIntegerv( GL_TEXTURE_BINDING_CUBE_MAP, &currentBinding );
        if ( currentBinding != id ) {
            glBindTexture( GL_TEXTURE_CUBE_MAP, id );
        }

        int face = _Offset.Z < 6 ? _Offset.Z : 5; // cubemap face
        glCopyTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                             _Offset.Lod,
                             _Offset.X,
                             _Offset.Y,
                             _SrcRect.X,
                             _SrcRect.Y,
                             _SrcRect.Width,
                             _SrcRect.Height );

        if ( currentBinding != id ) {
            glBindTexture( GL_TEXTURE_CUBE_MAP, currentBinding );
        }
        break;
    }
    case TEXTURE_RECT_GL:
    {
        glCopyTextureSubImage2D( _DstTexture->GetHandleNativeGL(),
                                 0,
                                 _Offset.X,
                                 _Offset.Y,
                                 _SrcRect.X,
                                 _SrcRect.Y,
                                 _SrcRect.Width,
                                 _SrcRect.Height );
        break;
    }
    case TEXTURE_CUBE_MAP_ARRAY:
        // FIXME: в спецификации про этот тип ничего не сказано
        return false;
    }

    return true;
}

void AImmediateContextGLImpl::CopyFramebufferToBuffer( IFramebuffer const * _SrcFramebuffer,
                                                       IBuffer * _DstBuffer,
                                                       FRAMEBUFFER_ATTACHMENT _Attachment,
                                                       SRect2D const & _SrcRect,
                                                       FRAMEBUFFER_CHANNEL _FramebufferChannel,
                                                       FRAMEBUFFER_OUTPUT _FramebufferOutput,
                                                       COLOR_CLAMP _ColorClamp,
                                                       size_t _SizeInBytes,
                                                       size_t _DstByteOffset,
                                                       unsigned int _Alignment )
{
    VerifyContext();

    AFramebufferGLImpl const * framebuffer = static_cast< AFramebufferGLImpl const * >( _SrcFramebuffer );

    // TODO: check this

    if ( !framebuffer->ChooseReadBuffer( _Attachment ) ) {
        GLogger.Printf( "AImmediateContextGLImpl::CopyFramebufferToBuffer: invalid framebuffer attachment\n" );
        return;
    }

    BindReadFramebuffer( framebuffer );

    PackAlignment( _Alignment );

    glBindBuffer( GL_PIXEL_PACK_BUFFER, _DstBuffer->GetHandleNativeGL() );

    ClampReadColor( _ColorClamp );

    glReadnPixels( _SrcRect.X,
                   _SrcRect.Y,
                   _SrcRect.Width,
                   _SrcRect.Height,
                   FramebufferChannelLUT[ _FramebufferChannel ],
                   FramebufferOutputLUT[ _FramebufferOutput ],
                   (GLsizei)_SizeInBytes,
                   ((uint8_t *)0) + _DstByteOffset );
    glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
}

bool AImmediateContextGLImpl::BlitFramebuffer( IFramebuffer const * _SrcFramebuffer,
                                               FRAMEBUFFER_ATTACHMENT _SrcAttachment,
                                               uint32_t _NumRectangles,
                                               SBlitRectangle const * _Rectangles,
                                               FRAMEBUFFER_MASK _Mask,
                                               bool _LinearFilter )
{
    VerifyContext();

    AFramebufferGLImpl const * framebuffer = static_cast< AFramebufferGLImpl const * >( _SrcFramebuffer );

    GLbitfield mask = 0;

    if ( _Mask & FB_MASK_COLOR ) {
        mask |= GL_COLOR_BUFFER_BIT;

        if ( !framebuffer->ChooseReadBuffer( _SrcAttachment ) ) {
            GLogger.Printf( "AImmediateContextGLImpl::BlitFramebuffer: invalid framebuffer attachment\n" );
            return false;
        }
    }

    if ( _Mask & FB_MASK_DEPTH ) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if ( _Mask & FB_MASK_STENCIL ) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    BindReadFramebuffer( framebuffer );

    GLenum filter = _LinearFilter ? GL_LINEAR : GL_NEAREST;

    for ( SBlitRectangle const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
        glBlitFramebuffer( rect->SrcX,
                           rect->SrcY,
                           rect->SrcX + rect->SrcWidth,
                           rect->SrcY + rect->SrcHeight,
                           rect->DstX,
                           rect->DstY,
                           rect->DstX + rect->DstWidth,
                           rect->DstY + rect->DstHeight,
                           mask,
                           filter );
    }

    return true;
}

void AImmediateContextGLImpl::ClearBuffer( IBuffer * _Buffer, BUFFER_VIEW_PIXEL_FORMAT _InternalFormat, DATA_FORMAT _Format, SClearValue const * _ClearValue )
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    TableInternalPixelFormat const * format = &InternalFormatLUT[ _InternalFormat ];

    glClearNamedBufferData( _Buffer->GetHandleNativeGL(),
                            format->InternalFormat,
                            TypeLUT[_Format].FormatRGB,
                            TypeLUT[_Format].Type,
                            _ClearValue ); // 4.5 or GL_ARB_direct_state_access

    if ( RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }

    // It can be also replaced by glClearBufferData
}

void AImmediateContextGLImpl::ClearBufferRange( IBuffer * _Buffer, BUFFER_VIEW_PIXEL_FORMAT _InternalFormat, uint32_t _NumRanges, SBufferClear const * _Ranges, DATA_FORMAT _Format, const SClearValue * _ClearValue )
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    TableInternalPixelFormat const * format = &InternalFormatLUT[ _InternalFormat ];

    for ( SBufferClear const * range = _Ranges ; range < &_Ranges[_NumRanges] ; range++ ) {
        glClearNamedBufferSubData( _Buffer->GetHandleNativeGL(),
                                   format->InternalFormat,
                                   range->Offset,
                                   range->SizeInBytes,
                                   TypeLUT[_Format].FormatRGB,
                                   TypeLUT[_Format].Type,
                                   _ClearValue ); // 4.5 or GL_ARB_direct_state_access
    }

    if ( RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }

    // It can be also replaced by glClearBufferSubData
}

void AImmediateContextGLImpl::ClearTexture( ITexture * _Texture, uint16_t _Lod, DATA_FORMAT _Format, SClearValue const * _ClearValue )
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    GLenum format;

    switch ( _Texture->GetFormat() ) {
    case TEXTURE_FORMAT_STENCIL1:
    case TEXTURE_FORMAT_STENCIL4:
    case TEXTURE_FORMAT_STENCIL8:
    case TEXTURE_FORMAT_STENCIL16:
        format = GL_STENCIL_INDEX;
        break;
    case TEXTURE_FORMAT_DEPTH16:
    case TEXTURE_FORMAT_DEPTH24:
    case TEXTURE_FORMAT_DEPTH32:
        format = GL_DEPTH_COMPONENT;
        break;
    case TEXTURE_FORMAT_DEPTH24_STENCIL8:
    case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
        format = GL_DEPTH_STENCIL;
        break;
    default:
        format = TypeLUT[_Format].FormatRGB;
        break;
    };

    glClearTexImage( _Texture->GetHandleNativeGL(),
                     _Lod,
                     format,
                     TypeLUT[_Format].Type,
                     _ClearValue );

    if ( RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }
}

void AImmediateContextGLImpl::ClearTextureRect( ITexture * _Texture,
                                                uint32_t _NumRectangles,
                                                STextureRect const * _Rectangles,
                                                DATA_FORMAT _Format,
                                                /* optional */ SClearValue const * _ClearValue )
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    GLenum format;

    switch ( _Texture->GetFormat() ) {
    case TEXTURE_FORMAT_STENCIL1:
    case TEXTURE_FORMAT_STENCIL4:
    case TEXTURE_FORMAT_STENCIL8:
    case TEXTURE_FORMAT_STENCIL16:
        format = GL_STENCIL_INDEX;
        break;
    case TEXTURE_FORMAT_DEPTH16:
    case TEXTURE_FORMAT_DEPTH24:
    case TEXTURE_FORMAT_DEPTH32:
        format = GL_DEPTH_COMPONENT;
        break;
    case TEXTURE_FORMAT_DEPTH24_STENCIL8:
    case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
        format = GL_DEPTH_STENCIL;
        break;
    default:
        format = TypeLUT[_Format].FormatRGB;
        break;
    };

    for ( STextureRect const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
        glClearTexSubImage( _Texture->GetHandleNativeGL(),
                            rect->Offset.Lod,
                            rect->Offset.X,
                            rect->Offset.Y,
                            rect->Offset.Z,
                            rect->Dimension.X,
                            rect->Dimension.Y,
                            rect->Dimension.Z,
                            format,
                            TypeLUT[_Format].Type,
                            _ClearValue );
    }

    if ( RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }
}

void AImmediateContextGLImpl::ClearFramebufferAttachments( IFramebuffer * _Framebuffer,
                                                           /* optional */ unsigned int * _ColorAttachments,
                                                           /* optional */ unsigned int _NumColorAttachments,
                                                           /* optional */ SClearColorValue const * _ColorClearValues,
                                                           /* optional */ SClearDepthStencilValue const * _DepthStencilClearValue,
                                                           /* optional */ SRect2D const * _Rect )
{
    VerifyContext();

    AFramebufferGLImpl const * framebuffer = static_cast< AFramebufferGLImpl const * >( _Framebuffer );

    AN_ASSERT( _NumColorAttachments <= framebuffer->GetNumColorAttachments() );

    GLuint framebufferId = framebuffer->GetHandleNativeGL();

    AN_ASSERT( framebufferId );

    bool bScissorEnabled = RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = RasterizerState.bRasterizerDiscard;
    SRect2D scissorRect;

    // If clear rect was not specified, use renderpass render area
    if ( !_Rect && CurrentRenderPass ) {
        _Rect = &CurrentRenderPassRenderArea;
    }

    if ( _Rect ) {
        if ( !bScissorEnabled ) {
            glEnable( GL_SCISSOR_TEST );
            bScissorEnabled = true;
        }

        // Save current scissor rectangle
        scissorRect = CurrentScissor;

        // Set new scissor rectangle
        SetScissor( *_Rect );
    } else {
        if ( bScissorEnabled ) {
            glDisable( GL_SCISSOR_TEST );
            bScissorEnabled = false;
        }
    }

    if ( bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
        bRasterizerDiscard = false;
    }

    if ( _ColorAttachments ) {
        // We must set draw buffers to clear attachment :(
        for ( unsigned int i = 0 ; i < _NumColorAttachments ; i++ ) {
            unsigned int attachmentIndex = _ColorAttachments[i];

            Attachments[i] = GL_COLOR_ATTACHMENT0 + attachmentIndex;
        }
        glNamedFramebufferDrawBuffers( framebufferId, _NumColorAttachments, Attachments );

        // Mark subpass to reset draw buffers
        CurrentSubpass = -1;

        for ( unsigned int i = 0 ; i < _NumColorAttachments ; i++ ) {

            unsigned int attachmentIndex = _ColorAttachments[ i ];

            AN_ASSERT( attachmentIndex < framebuffer->GetNumColorAttachments() );
            AN_ASSERT( _ColorClearValues );

            SFramebufferAttachmentInfo const & framebufferAttachment = framebuffer->GetColorAttachments()[ attachmentIndex ];

            SClearColorValue const * clearValue = &_ColorClearValues[ i ];

            SRenderTargetBlendingInfo const & currentState = BlendState.RenderTargetSlots[ attachmentIndex ];
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                glColorMaski( i, 1, 1, 1, 1 );
            }

            // Clear attchment
            switch ( InternalFormatLUT[ framebufferAttachment.pTexture->GetFormat() ].ClearType ) {
            case CLEAR_TYPE_FLOAT32:
                glClearNamedFramebufferfv( framebufferId,
                                           GL_COLOR,
                                           i,
                                           clearValue->Float32 );
                break;
            case CLEAR_TYPE_INT32:
                glClearNamedFramebufferiv( framebufferId,
                                           GL_COLOR,
                                           i,
                                           clearValue->Int32 );
                break;
            case CLEAR_TYPE_UINT32:
                glClearNamedFramebufferuiv( framebufferId,
                                            GL_COLOR,
                                            i,
                                            clearValue->UInt32 );
                break;
            default:
                AN_ASSERT( 0 );
            }

            // Restore color mask
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                if ( currentState.ColorWriteMask == COLOR_WRITE_DISABLED ) {
                    glColorMaski( i, 0, 0, 0, 0 );
                } else {
                    glColorMaski( i,
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_R_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_G_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_B_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_A_BIT) );
                }
            }
        }
    }

    if ( _DepthStencilClearValue ) {

        AN_ASSERT( framebuffer->HasDepthStencilAttachment() );

        SFramebufferAttachmentInfo const & framebufferAttachment = framebuffer->GetDepthStencilAttachment();

        // TODO: table
        switch ( InternalFormatLUT[ framebufferAttachment.pTexture->GetFormat() ].ClearType ) {
        case CLEAR_TYPE_STENCIL_ONLY:
            glClearNamedFramebufferuiv( framebufferId,
                                        GL_STENCIL,
                                        0,
                                        &_DepthStencilClearValue->Stencil );
            break;
        case CLEAR_TYPE_DEPTH_ONLY:
            glClearNamedFramebufferfv( framebufferId,
                                       GL_DEPTH,
                                       0,
                                       &_DepthStencilClearValue->Depth );
            break;
        case CLEAR_TYPE_DEPTH_STENCIL:
            glClearNamedFramebufferfi( framebufferId,
                                       GL_DEPTH_STENCIL,
                                       0,
                                       _DepthStencilClearValue->Depth,
                                       _DepthStencilClearValue->Stencil );
            break;
        default:
            AN_ASSERT( 0 );
        }
    }

    // Restore scissor test
    if ( bScissorEnabled != RasterizerState.bScissorEnable ) {
        if ( RasterizerState.bScissorEnable ) {
            glEnable( GL_SCISSOR_TEST );
        } else {
            glDisable( GL_SCISSOR_TEST );
        }
    }

    // Restore scissor rect
    if ( _Rect ) {
        SetScissor( scissorRect );
    }

    // Restore rasterizer discard
    if ( bRasterizerDiscard != RasterizerState.bRasterizerDiscard ) {
        if ( RasterizerState.bRasterizerDiscard ) {
            glEnable( GL_RASTERIZER_DISCARD );
        } else {
            glDisable( GL_RASTERIZER_DISCARD );
        }
    }
}

void AImmediateContextGLImpl::BindReadFramebuffer( IFramebuffer const * Framebuffer )
{
    if ( Binding.ReadFramebufferUID == Framebuffer->GetUID() ) {
        return;
    }

    GLuint framebufferId = Framebuffer->GetHandleNativeGL();
    glBindFramebuffer( GL_READ_FRAMEBUFFER, framebufferId );

    Binding.ReadFramebufferUID = Framebuffer->GetUID();
}

void AImmediateContextGLImpl::UnbindFramebuffer( IFramebuffer const * Framebuffer )
{
    if ( Binding.DrawFramebufferUID == Framebuffer->GetUID() ) {
        Binding.DrawFramebufferUID = DefaultFramebuffer->GetUID();
        Binding.DrawFramebuffer = DefaultFramebuffer->GetHandleNativeGL();

        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, DefaultFramebuffer->GetHandleNativeGL() );
    }
    if ( Binding.ReadFramebufferUID == Framebuffer->GetUID() ) {
        Binding.ReadFramebufferUID = DefaultFramebuffer->GetUID();

        glBindFramebuffer( GL_READ_FRAMEBUFFER, DefaultFramebuffer->GetHandleNativeGL() );
    }
}

void AImmediateContextGLImpl::NotifyRenderPassDestroyed( ARenderPassGLImpl const * RenderPass )
{
    if ( CurrentRenderPass == RenderPass ) {
        GLogger.Printf( "AImmediateContextGLImpl::NotifyRenderPassDestroyed: destroying render pass without EndRenderPass()\n" );
        CurrentRenderPass = nullptr;
    }
}

}
