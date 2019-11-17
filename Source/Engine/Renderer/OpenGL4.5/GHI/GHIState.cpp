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

#include "GHIState.h"
#include "GHIDevice.h"
#include "GHIBuffer.h"
#include "GHIPipeline.h"
#include "GHIVertexArrayObject.h"
#include "LUT.h"

#include <algorithm>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <GL/glew.h>

#define DEFAULT_STENCIL_REF 0

namespace GHI {

State * State::StateHead = nullptr, * State::StateTail = nullptr; // FIXME: thread_local?

#define IntrusiveAddToList( _object, _next, _prev, _head, _tail ) \
{ \
    _object->_prev = _tail; \
    _object->_next = nullptr; \
    _tail = _object; \
    if ( _object->_prev ) { \
        _object->_prev->_next = _object; \
    } else { \
        _head = _object; \
    } \
}

#define IntrusiveRemoveFromList( _object, _next, _prev, _head, _tail ) \
{ \
    auto * __next = _object->_next; \
    auto * __prev = _object->_prev; \
     \
    if ( __next || __prev || _object == _head ) { \
        if ( __next ) { \
            __next->_prev = __prev; \
        } else { \
            _tail = __prev; \
        } \
        if ( __prev ) { \
            __prev->_next = __next; \
        } else { \
            _head = __next; \
        } \
        _object->_next = nullptr; \
        _object->_prev = nullptr; \
    } \
}

State::State() {
    pDevice = nullptr;
}

void State::Initialize( Device * _Device, StateCreateInfo const & _CreateInfo ) {
    pDevice = _Device;

    //ScreenWidth = ScreenHeight = 512;

    unsigned int maxTemporaryHandles = _Device->MaxVertexBufferSlots; // TODO: check if 0 ???

    maxTemporaryHandles = std::max( maxTemporaryHandles, _Device->MaxCombinedTextureImageUnits );
    maxTemporaryHandles = std::max( maxTemporaryHandles, _Device->MaxImageUnits );
    maxTemporaryHandles = std::max( maxTemporaryHandles, _Device->MaxBufferBindings[ UNIFORM_BUFFER ] );

    static_assert( sizeof( GLuint ) == sizeof( *TmpHandles ), "Sizeof check" );
    static_assert( sizeof( GLintptr ) == sizeof( *TmpPointers ), "Sizeof check" );

    TmpHandles = ( GLuint * )_Device->Allocator.Allocate( sizeof( GLuint ) * maxTemporaryHandles );
    TmpPointers = ( GLintptr * )_Device->Allocator.Allocate( sizeof( GLintptr ) * maxTemporaryHandles*2 );
    TmpPointers2 = TmpPointers + maxTemporaryHandles;

    memset( BufferBindings, 0, sizeof( BufferBinding ) );
    memset( SampleBindings, 0, sizeof( SampleBindings ) );
    memset( TextureBindings, 0, sizeof( TextureBindings ) );

    CurrentPipeline = NULL;
    CurrentVAO = NULL;
    NumPatchVertices = 0;

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
    memset( &BlendState, 0, sizeof( BlendState ) );
    BlendState.bIndependentBlendEnable = false;
    BlendState.bSampleAlphaToCoverage = false;
    BlendState.LogicOp = LOGIC_OP_COPY;
    bLogicOpEnabled = false;
    for ( int i = 0 ; i < MAX_COLOR_ATTACHMENTS ; i++ ) {
        BlendState.RenderTargetSlots[i].SetDefaults();
    }
    glColorMask( 1,1,1,1 );
    glDisable( GL_BLEND );
    glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
    glBlendFunc( GL_ONE, GL_ZERO );
    glBlendEquation( GL_FUNC_ADD );
    glBlendColor( 0, 0, 0, 0 );
    glDisable( GL_COLOR_LOGIC_OP );
    glLogicOp( GL_COPY );
    memset( BlendColor, 0, sizeof( BlendColor ) );
    SampleMask[ 0 ] = 0xffffffff;
    SampleMask[ 1 ] = 0;
    SampleMask[ 2 ] = 0;
    SampleMask[ 3 ] = 0;
    for ( int i = 0 ; i < 4 ; i++ ) {
        glSampleMaski( i, SampleMask[ i ] );
    }
    bSampleMaskEnabled = false;
    glDisable( GL_SAMPLE_MASK );

    // Init default rasterizer state
    RasterizerState.SetDefaults();
    glDisable( GL_POLYGON_OFFSET_FILL );
    PolygonOffsetClampSafe( 0, 0, 0 );
    glDisable( GL_DEPTH_CLAMP );
    glDisable( GL_LINE_SMOOTH );
    glDisable( GL_RASTERIZER_DISCARD );
    glDisable( GL_MULTISAMPLE );
    glDisable( GL_SCISSOR_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glFrontFace( GL_CCW );
    // GL_POLYGON_SMOOTH
    // If enabled, draw polygons with proper filtering. Otherwise, draw aliased polygons.
    // For correct antialiased polygons, an alpha buffer is needed and the polygons must be sorted front to back.
    glDisable( GL_POLYGON_SMOOTH ); // Smooth polygons have some artifacts
    bPolygonOffsetEnabled = false;

    // Init default depth-stencil state
    DepthStencilState.SetDefaults();
    StencilRef = DEFAULT_STENCIL_REF;
    glEnable( GL_DEPTH_TEST );
    glDepthMask( 1 );
    glDepthFunc( GL_LESS );
    glDisable( GL_STENCIL_TEST );
    glStencilMask( DEFAULT_STENCIL_WRITE_MASK );
    glStencilOpSeparate( GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP );
    glStencilFuncSeparate( GL_ALWAYS, GL_ALWAYS, StencilRef, DEFAULT_STENCIL_READ_MASK );

    ColorClamp = COLOR_CLAMP_ALWAYS_OFF;
    glClampColor( GL_CLAMP_READ_COLOR, GL_FALSE );

    bPrimitiveRestartEnabled = false;

    CurrentRenderPass = nullptr;
    Binding.ReadFramebuffer = (unsigned int)~0;
    Binding.DrawFramebuffer = (unsigned int)~0;
    SwapChainWidth = 512;
    SwapChainHeight = 512;

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

    TotalPipelines = 0;
    TotalRenderPasses = 0;
    TotalFramebuffers = 0;
    TotalTransformFeedbacks = 0;
    TotalQueryPools = 0;

    _Device->TotalStates++;

    IntrusiveAddToList( this, Next, Prev, StateHead, StateTail );
}

void State::Deinitialize() {
    assert( pDevice != nullptr );

    glBindVertexArray( 0 );
    for ( VertexArrayObject * vao : VAOCache ) {
        glDeleteVertexArrays( 1, &vao->Handle );
        pDevice->Allocator.Deallocate( vao );
    }
    VAOCache.Free();
    VAOHash.Free();

    pDevice->Allocator.Deallocate( TmpHandles );
    pDevice->Allocator.Deallocate( TmpPointers );

    IntrusiveRemoveFromList( this, Next, Prev, StateHead, StateTail );

    pDevice->TotalStates--;

    pDevice = nullptr;
}

void State::SetSwapChainResolution( int _Width, int _Height ) {
    SwapChainWidth = _Width;
    SwapChainHeight = _Height;

    if ( Binding.DrawFramebuffer == 0 ) {
        Binding.DrawFramebufferWidth = ( unsigned short )SwapChainWidth;
        Binding.DrawFramebufferHeight = ( unsigned short )SwapChainHeight;
    }
}

void State::PolygonOffsetClampSafe( float _Slope, int _Bias, float _Clamp ) {
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

void State::PackAlignment( unsigned int _Alignment ) {
    if ( PixelStore.PackAlignment != _Alignment ) {
        glPixelStorei( GL_PACK_ALIGNMENT, _Alignment );
        PixelStore.PackAlignment = _Alignment;
    }
}

void State::UnpackAlignment( unsigned int _Alignment ) {
    if ( PixelStore.UnpackAlignment != _Alignment ) {
        glPixelStorei( GL_UNPACK_ALIGNMENT, _Alignment );
        PixelStore.UnpackAlignment = _Alignment;
    }
}

void State::ClampReadColor( COLOR_CLAMP _ColorClamp ) {
    if ( ColorClamp != _ColorClamp ) {
        glClampColor( GL_CLAMP_READ_COLOR, ColorClampLUT[ _ColorClamp ] );
        ColorClamp = _ColorClamp;
    }
}

VertexArrayObject * State::CachedVAO( VertexBindingInfo const * pVertexBindings,
                                      uint32_t NumVertexBindings,
                                      VertexAttribInfo const * pVertexAttribs,
                                      uint32_t NumVertexAttribs ) {
    VertexArrayObjectHashedData hashed;

    memset( &hashed, 0, sizeof( hashed ) );

    hashed.NumVertexBindings = NumVertexBindings;
    if ( hashed.NumVertexBindings > MAX_VERTEX_BINDINGS ) {
        hashed.NumVertexBindings = MAX_VERTEX_BINDINGS;

        LogPrintf( "Warning: NumVertexBindings > MAX_VERTEX_BINDINGS\n" );
    }
    memcpy( hashed.VertexBindings, pVertexBindings, sizeof( hashed.VertexBindings[0] ) * hashed.NumVertexBindings );

    hashed.NumVertexAttribs = NumVertexAttribs;
    if ( hashed.NumVertexAttribs > MAX_VERTEX_ATTRIBS ) {
        hashed.NumVertexAttribs = MAX_VERTEX_ATTRIBS;

        LogPrintf( "Warning: NumVertexAttribs > MAX_VERTEX_ATTRIBS\n" );
    }
    memcpy( hashed.VertexAttribs, pVertexAttribs, sizeof( hashed.VertexAttribs[0] ) * hashed.NumVertexAttribs );

    int hash = pDevice->Hash( ( unsigned char * )&hashed, sizeof( hashed ) );

    int i = VAOHash.First( hash );
    for ( ; i != -1 ; i = VAOHash.Next( i ) ) {

        VertexArrayObject * vao = VAOCache[ i ];

        if ( !memcmp( &vao->Hashed, &hashed, sizeof( vao->Hashed ) ) ) {
            //LogPrintf( "Caching VAO\n" );
            return vao;
        }
    }

    VertexArrayObject * vao = static_cast< VertexArrayObject * >( pDevice->Allocator.Allocate( sizeof( VertexArrayObject ) ) );

    memcpy( &vao->Hashed, &hashed, sizeof( vao->Hashed ) );

    vao->IndexBufferUID = 0;

    memset( vao->VertexBufferUIDs, 0, sizeof( vao->VertexBufferUIDs ) );
    memset( vao->VertexBufferOffsets, 0, sizeof( vao->VertexBufferOffsets ) );

    i = VAOCache.Size();

    VAOHash.Insert( hash, i );
    VAOCache.Append( vao );

    //LogPrintf( "Total VAOs %d\n", i+1 );

    glCreateVertexArrays( 1, &vao->Handle );
    if ( !vao->Handle ) {
        LogPrintf( "Pipeline::Initialize: couldn't create vertex array object\n" );
        //return NULL;
    }

    memset( vao->VertexBindingsStrides, 0, sizeof( vao->VertexBindingsStrides ) );
    for ( VertexBindingInfo const * binding = hashed.VertexBindings ; binding < &hashed.VertexBindings[hashed.NumVertexBindings] ; binding++ ) {
        assert( binding->InputSlot < MAX_VERTEX_BUFFER_SLOTS );

        if ( binding->InputSlot >= pDevice->MaxVertexBufferSlots ) {
            LogPrintf( "Pipeline::Initialize: binding->InputSlot >= MaxVertexBufferSlots\n" );
        }

        if ( binding->Stride > pDevice->MaxVertexAttribStride ) {
            LogPrintf( "Pipeline::Initialize: binding->Stride > MaxVertexAttribStride\n" );
        }

        vao->VertexBindingsStrides[ binding->InputSlot ] = binding->Stride;
    }

    for ( VertexAttribInfo const * attrib = hashed.VertexAttribs ; attrib < &hashed.VertexAttribs[hashed.NumVertexAttribs] ; attrib++ ) {

        // glVertexAttribFormat, glVertexAttribBinding, glVertexBindingDivisor - v4.3 or GL_ARB_vertex_attrib_binding

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

        for ( VertexBindingInfo const * binding = hashed.VertexBindings ; binding < &hashed.VertexBindings[hashed.NumVertexBindings] ; binding++ ) {
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

}
