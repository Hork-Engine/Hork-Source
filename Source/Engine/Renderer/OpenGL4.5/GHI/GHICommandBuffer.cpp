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

#include "GHICommandBuffer.h"
#include "GHITransformFeedback.h"
#include "GHIState.h"
#include "GHIVertexArrayObject.h"
#include "LUT.h"

#include <assert.h>
#include <cmath>
#include <memory.h>
#include <limits>
#include <algorithm>
#include <GL/glew.h>

namespace GHI {

static GLenum Attachments[ MAX_COLOR_ATTACHMENTS ];

static bool BlendCompareEquation( RenderTargetBlendingInfo::Operation_t const & _Mode1,
                                  RenderTargetBlendingInfo::Operation_t const & _Mode2 ) {
    return _Mode1.ColorRGB == _Mode2.ColorRGB && _Mode1.Alpha == _Mode2.Alpha;
}

static bool BlendCompareFunction( RenderTargetBlendingInfo::Function_t const & _Func1,
                                  RenderTargetBlendingInfo::Function_t const & _Func2 ) {
    return _Func1.SrcFactorRGB == _Func2.SrcFactorRGB
            && _Func1.DstFactorRGB == _Func2.DstFactorRGB
            && _Func1.SrcFactorAlpha == _Func2.SrcFactorAlpha
            && _Func1.DstFactorAlpha == _Func2.DstFactorAlpha;
}

static bool BlendCompareColor( const float _Color1[4], const float _Color2[4] ) {
    return     std::fabs( _Color1[0] - _Color2[0] ) < 0.000001f
            && std::fabs( _Color1[1] - _Color2[1] ) < 0.000001f
            && std::fabs( _Color1[2] - _Color2[2] ) < 0.000001f
            && std::fabs( _Color1[3] - _Color2[3] ) < 0.000001f;
}

// Compare render target blending at specified slot and change if different
static void SetRenderTargetSlotBlending( int _Slot,
                                         RenderTargetBlendingInfo const & _CurrentState,
                                         RenderTargetBlendingInfo const & _RequiredState ) {

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
static void SetRenderTargetSlotsBlending( RenderTargetBlendingInfo const & _CurrentState,
                                          RenderTargetBlendingInfo const & _RequiredState,
                                          bool _NeedReset ) {

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

CommandBuffer::CommandBuffer() {

}

CommandBuffer::~CommandBuffer() {
    Deinitialize();
}

void CommandBuffer::Initialize() {

}

void CommandBuffer::Deinitialize() {

}

void CommandBuffer::BindPipeline( Pipeline * _Pipeline ) {
    State * state = GetCurrentState();

    assert( _Pipeline != NULL );

    if ( state->CurrentPipeline == _Pipeline ) {
        // TODO: cache subpass
        BindRenderPassSubPass( state, _Pipeline->pRenderPass, _Pipeline->Subpass );
        return;
    }

    state->CurrentPipeline = _Pipeline;

    GLuint pipelineId = GL_HANDLE( _Pipeline->GetHandle() );

    glBindProgramPipeline( pipelineId );

    if ( state->CurrentVAO != _Pipeline->VAO ) {
        glBindVertexArray( _Pipeline->VAO->Handle );
        //LogPrintf( "Binding vao %d\n", _Pipeline->VAO->Handle );
        state->CurrentVAO = _Pipeline->VAO;
    } else {
        //LogPrintf( "caching vao binding %d\n", _Pipeline->VAO->Handle );
    }

    //
    // Set render pass
    //

    BindRenderPassSubPass( state, _Pipeline->pRenderPass, _Pipeline->Subpass );

    //
    // Set input assembly
    //

    if ( _Pipeline->PrimitiveTopology == GL_PATCHES ) {
        if ( state->NumPatchVertices != _Pipeline->NumPatchVertices ) {
            glPatchParameteri( GL_PATCH_VERTICES, _Pipeline->NumPatchVertices );   // Sinse GL v4.0
            state->NumPatchVertices = ( uint8_t )_Pipeline->NumPatchVertices;
        }
    }

    if ( state->bPrimitiveRestartEnabled != _Pipeline->bPrimitiveRestartEnabled ) {
        if ( _Pipeline->bPrimitiveRestartEnabled ) {
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

        state->bPrimitiveRestartEnabled = _Pipeline->bPrimitiveRestartEnabled;
    }

    //
    // Set blending state
    //

    // Compare blending states
    if ( state->Binding.BlendState != _Pipeline->BlendingState ) {
        BlendingStateInfo const & desc = *_Pipeline->BlendingState;

        if ( desc.bIndependentBlendEnable ) {
            for ( int i = 0 ; i < MAX_COLOR_ATTACHMENTS ; i++ ) {
                RenderTargetBlendingInfo const & rtDesc = desc.RenderTargetSlots[ i ];
                SetRenderTargetSlotBlending( i, state->BlendState.RenderTargetSlots[ i ], rtDesc );
                memcpy( &state->BlendState.RenderTargetSlots[ i ], &rtDesc, sizeof( rtDesc ) );
            }
        } else {
            RenderTargetBlendingInfo const & rtDesc = desc.RenderTargetSlots[ 0 ];
            bool needReset = state->BlendState.bIndependentBlendEnable;
            SetRenderTargetSlotsBlending( state->BlendState.RenderTargetSlots[ 0 ], rtDesc, needReset );
            for ( int i = 0 ; i < MAX_COLOR_ATTACHMENTS ; i++ ) {
                memcpy( &state->BlendState.RenderTargetSlots[ i ], &rtDesc, sizeof( rtDesc ) );
            }
        }

        state->BlendState.bIndependentBlendEnable = desc.bIndependentBlendEnable;

        if ( state->BlendState.bSampleAlphaToCoverage != desc.bSampleAlphaToCoverage ) {
            if ( desc.bSampleAlphaToCoverage ) {
                glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
            } else {
                glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
            }

            state->BlendState.bSampleAlphaToCoverage = desc.bSampleAlphaToCoverage;
        }

        if ( state->BlendState.LogicOp != desc.LogicOp ) {
            if ( desc.LogicOp == LOGIC_OP_COPY ) {
                if ( state->bLogicOpEnabled ) {
                    glDisable( GL_COLOR_LOGIC_OP );
                    state->bLogicOpEnabled = false;
                }
            } else {
                if ( !state->bLogicOpEnabled ) {
                    glEnable( GL_COLOR_LOGIC_OP );
                    state->bLogicOpEnabled = true;
                }
                glLogicOp( LogicOpLUT[ desc.LogicOp ] );
            }

            state->BlendState.LogicOp = desc.LogicOp;
        }

        state->Binding.BlendState = _Pipeline->BlendingState;
    }

    //
    // Set rasterizer state
    //

    if ( state->Binding.RasterizerState != _Pipeline->RasterizerState ) {
        RasterizerStateInfo const & desc = *_Pipeline->RasterizerState;

        if ( state->RasterizerState.FillMode != desc.FillMode ) {
            glPolygonMode( GL_FRONT_AND_BACK, FillModeLUT[ desc.FillMode ] );
            state->RasterizerState.FillMode = desc.FillMode;
        }

        if ( state->RasterizerState.CullMode != desc.CullMode ) {
            if ( desc.CullMode == POLYGON_CULL_DISABLED ) {
                glDisable( GL_CULL_FACE );
            } else {
                if ( state->RasterizerState.CullMode == POLYGON_CULL_DISABLED ) {
                    glEnable( GL_CULL_FACE );
                }
                glCullFace( CullModeLUT[ desc.CullMode ] );
            }
            state->RasterizerState.CullMode = desc.CullMode;
        }

        if ( state->RasterizerState.bScissorEnable != desc.bScissorEnable ) {
            if ( desc.bScissorEnable ) {
                glEnable( GL_SCISSOR_TEST );
            } else {
                glDisable( GL_SCISSOR_TEST );
            }
            state->RasterizerState.bScissorEnable = desc.bScissorEnable;
        }

        if ( state->RasterizerState.bMultisampleEnable != desc.bMultisampleEnable ) {
            if ( desc.bMultisampleEnable ) {
                glEnable( GL_MULTISAMPLE );
            } else {
                glDisable( GL_MULTISAMPLE );
            }
            state->RasterizerState.bMultisampleEnable = desc.bMultisampleEnable;
        }

        if ( state->RasterizerState.bRasterizerDiscard != desc.bRasterizerDiscard ) {
            if ( desc.bRasterizerDiscard ) {
                glEnable( GL_RASTERIZER_DISCARD );
            } else {
                glDisable( GL_RASTERIZER_DISCARD );
            }
            state->RasterizerState.bRasterizerDiscard = desc.bRasterizerDiscard;
        }

        if ( state->RasterizerState.bAntialiasedLineEnable != desc.bAntialiasedLineEnable ) {
            if ( desc.bAntialiasedLineEnable ) {
                glEnable( GL_LINE_SMOOTH );
            } else {
                glDisable( GL_LINE_SMOOTH );
            }
            state->RasterizerState.bAntialiasedLineEnable = desc.bAntialiasedLineEnable;
        }

        if ( state->RasterizerState.bDepthClampEnable != desc.bDepthClampEnable ) {
            if ( desc.bDepthClampEnable ) {
                glEnable( GL_DEPTH_CLAMP );
            } else {
                glDisable( GL_DEPTH_CLAMP );
            }
            state->RasterizerState.bDepthClampEnable = desc.bDepthClampEnable;
        }

        if ( state->RasterizerState.DepthOffset.Slope != desc.DepthOffset.Slope
            || state->RasterizerState.DepthOffset.Bias != desc.DepthOffset.Bias
            || state->RasterizerState.DepthOffset.Clamp != desc.DepthOffset.Clamp ) {

            state->PolygonOffsetClampSafe( desc.DepthOffset.Slope, desc.DepthOffset.Bias, desc.DepthOffset.Clamp );

            state->RasterizerState.DepthOffset.Slope = desc.DepthOffset.Slope;
            state->RasterizerState.DepthOffset.Bias = desc.DepthOffset.Bias;
            state->RasterizerState.DepthOffset.Clamp = desc.DepthOffset.Clamp;
        }

        if ( state->RasterizerState.bFrontClockwise != desc.bFrontClockwise ) {
            glFrontFace( desc.bFrontClockwise ? GL_CW : GL_CCW );
            state->RasterizerState.bFrontClockwise = desc.bFrontClockwise;
        }

        state->Binding.RasterizerState = _Pipeline->RasterizerState;
    }

    //
    // Set depth stencil state
    //
#if 0
    if ( state->Binding.DepthStencilState == _Pipeline->DepthStencilState ) {

        if ( state->StencilRef != _StencilRef ) {
            // Update stencil ref

            DepthStencilStateInfo const & desc = _Pipeline->DepthStencilState;

            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( __ComparisonFunc[ desc.FrontFace.StencilFunc ],
                               _StencilRef,
                               desc.StencilReadMask );
            } else {
                glStencilFuncSeparate( __ComparisonFunc[ desc.FrontFace.StencilFunc ],
                                       __ComparisonFunc[ desc.BackFace.StencilFunc ],
                                       _StencilRef,
                                       desc.StencilReadMask );
            }
        }
    }
#endif

    if ( state->Binding.DepthStencilState != _Pipeline->DepthStencilState ) {
        DepthStencilStateInfo const & desc = *_Pipeline->DepthStencilState;

        if ( state->DepthStencilState.bDepthEnable != desc.bDepthEnable ) {
            if ( desc.bDepthEnable ) {
                glEnable( GL_DEPTH_TEST );
            } else {
                glDisable( GL_DEPTH_TEST );
            }

            state->DepthStencilState.bDepthEnable = desc.bDepthEnable;
        }

        if ( state->DepthStencilState.DepthWriteMask != desc.DepthWriteMask ) {
            glDepthMask( desc.DepthWriteMask );
            state->DepthStencilState.DepthWriteMask = desc.DepthWriteMask;
        }

        if ( state->DepthStencilState.DepthFunc != desc.DepthFunc ) {
            glDepthFunc( ComparisonFuncLUT[ desc.DepthFunc ] );
            state->DepthStencilState.DepthFunc = desc.DepthFunc;
        }

        if ( state->DepthStencilState.bStencilEnable != desc.bStencilEnable ) {
            if ( desc.bStencilEnable ) {
                glEnable( GL_STENCIL_TEST );
            } else {
                glDisable( GL_STENCIL_TEST );
            }

            state->DepthStencilState.bStencilEnable = desc.bStencilEnable;
        }

        if ( state->DepthStencilState.StencilWriteMask != desc.StencilWriteMask ) {
            glStencilMask( desc.StencilWriteMask );
            state->DepthStencilState.StencilWriteMask = desc.StencilWriteMask;
        }

        if ( state->DepthStencilState.StencilReadMask != desc.StencilReadMask
             || state->DepthStencilState.FrontFace.StencilFunc != desc.FrontFace.StencilFunc
             || state->DepthStencilState.BackFace.StencilFunc != desc.BackFace.StencilFunc
             /*|| state->StencilRef != _StencilRef*/ ) {

            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                               state->StencilRef,//_StencilRef,
                               desc.StencilReadMask );
            } else {
                glStencilFuncSeparate( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                                       ComparisonFuncLUT[ desc.BackFace.StencilFunc ],
                                       state->StencilRef,//_StencilRef,
                                       desc.StencilReadMask );
            }

            state->DepthStencilState.StencilReadMask = desc.StencilReadMask;
            state->DepthStencilState.FrontFace.StencilFunc = desc.FrontFace.StencilFunc;
            state->DepthStencilState.BackFace.StencilFunc = desc.BackFace.StencilFunc;
            //state->StencilRef = _StencilRef;
        }

        bool frontStencilChanged = ( state->DepthStencilState.FrontFace.StencilFailOp != desc.FrontFace.StencilFailOp
                                     || state->DepthStencilState.FrontFace.DepthFailOp != desc.FrontFace.DepthFailOp
                                     || state->DepthStencilState.FrontFace.DepthPassOp != desc.FrontFace.DepthPassOp );

        bool backStencilChanged = ( state->DepthStencilState.BackFace.StencilFailOp != desc.BackFace.StencilFailOp
                                     || state->DepthStencilState.BackFace.DepthFailOp != desc.BackFace.DepthFailOp
                                     || state->DepthStencilState.BackFace.DepthPassOp != desc.BackFace.DepthPassOp );

        if ( frontStencilChanged || backStencilChanged ) {
            bool isSame = desc.FrontFace.StencilFailOp == desc.BackFace.StencilFailOp
                       && desc.FrontFace.DepthFailOp == desc.BackFace.DepthFailOp
                       && desc.FrontFace.DepthPassOp == desc.BackFace.DepthPassOp;

            if ( isSame ) {
                glStencilOpSeparate( GL_FRONT_AND_BACK,
                                     StencilOpLUT[ desc.FrontFace.StencilFailOp ],
                                     StencilOpLUT[ desc.FrontFace.DepthFailOp ],
                                     StencilOpLUT[ desc.FrontFace.DepthPassOp ] );

                memcpy( &state->DepthStencilState.FrontFace, &desc.FrontFace, sizeof( desc.FrontFace ) );
                memcpy( &state->DepthStencilState.BackFace, &desc.BackFace, sizeof( desc.FrontFace ) );
            } else {

                if ( frontStencilChanged ) {
                    glStencilOpSeparate( GL_FRONT,
                                         StencilOpLUT[ desc.FrontFace.StencilFailOp ],
                                         StencilOpLUT[ desc.FrontFace.DepthFailOp ],
                                         StencilOpLUT[ desc.FrontFace.DepthPassOp ] );

                    memcpy( &state->DepthStencilState.FrontFace, &desc.FrontFace, sizeof( desc.FrontFace ) );
                }

                if ( backStencilChanged ) {
                    glStencilOpSeparate( GL_BACK,
                                         StencilOpLUT[ desc.BackFace.StencilFailOp ],
                                         StencilOpLUT[ desc.BackFace.DepthFailOp ],
                                         StencilOpLUT[ desc.BackFace.DepthPassOp ] );

                    memcpy( &state->DepthStencilState.BackFace, &desc.BackFace, sizeof( desc.FrontFace ) );
                }
            }
        }

        state->Binding.DepthStencilState = _Pipeline->DepthStencilState;
    }
}

void CommandBuffer::BindRenderPassSubPass( State * _State, RenderPass * _RenderPass, int _Subpass ) {

    assert( _RenderPass != nullptr );
    assert( _Subpass < _RenderPass->NumSubpasses );

    RenderSubpass const * subpass = &_RenderPass->Subpasses[ _Subpass ];

    const GLuint framebufferId = _State->Binding.DrawFramebuffer;

    if ( subpass->NumColorAttachments > 0 ) {
        for ( uint32_t i = 0 ; i < subpass->NumColorAttachments ; i++ ) {
            Attachments[ i ] = GL_COLOR_ATTACHMENT0 + subpass->ColorAttachmentRefs[i].Attachment;
        }

        glNamedFramebufferDrawBuffers( framebufferId, subpass->NumColorAttachments, Attachments );
    } else {
        glNamedFramebufferDrawBuffer( framebufferId, GL_NONE );
    }
}

void CommandBuffer::BindVertexBuffer( unsigned int _InputSlot,
                                      Buffer const * _VertexBuffer,
                                      unsigned int _Offset ) {
    State * state = GetCurrentState();
    VertexArrayObject * vao = state->CurrentVAO;

    assert( vao != NULL );
    assert( _InputSlot < MAX_VERTEX_BUFFER_SLOTS );

    GLuint vertexBufferId = _VertexBuffer ? GL_HANDLE( _VertexBuffer->GetHandle() ) : 0;
    uint32_t uid = _VertexBuffer ? _VertexBuffer->UID : 0;

    if ( vao->VertexBufferUIDs[ _InputSlot ] != uid || vao->VertexBufferOffsets[ _InputSlot ] != _Offset ) {

        glVertexArrayVertexBuffer( vao->Handle, _InputSlot, vertexBufferId, _Offset, vao->VertexBindingsStrides[ _InputSlot ] );

        vao->VertexBufferUIDs[ _InputSlot ] = uid;
        vao->VertexBufferOffsets[ _InputSlot ] = _Offset;

        //LogPrintf( "BindVertexBuffer %d\n", vertexBufferId );
    } else {
        //LogPrintf( "Caching BindVertexBuffer %d\n", vertexBufferId );
    }
}

void CommandBuffer::BindVertexBuffers( unsigned int _StartSlot,
                                       unsigned int _NumBuffers,
                                       Buffer * const * _VertexBuffers,
                                       const uint32_t * _Offsets ) {
    State * state = GetCurrentState();
    VertexArrayObject * vao = state->CurrentVAO;

    assert( vao != NULL );

    GLuint id = vao->Handle;

    static_assert( sizeof( vao->VertexBindingsStrides[0] ) == sizeof( GLsizei ), "Wrong type size" );

    if ( _StartSlot + _NumBuffers > state->pDevice->MaxVertexBufferSlots ) {
        LogPrintf( "BindVertexBuffers: StartSlot + NumBuffers > MaxVertexBufferSlots\n" );
        return;
    }

    bool bModified = false;

    if ( _VertexBuffers ) {

        for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
            unsigned int slot = _StartSlot + i;

            uint32_t uid = _VertexBuffers[i] ? _VertexBuffers[i]->UID : 0;
            uint32_t offset = _Offsets ? _Offsets[i] : 0;

            bModified = vao->VertexBufferUIDs[ slot ] != uid || vao->VertexBufferOffsets[ slot ] != offset;

            vao->VertexBufferUIDs[ slot ] = uid;
            vao->VertexBufferOffsets[ slot ] = offset;
        }

        if ( !bModified ) {
            return;
        }

        if ( _NumBuffers == 1 )
        {
            GLuint vertexBufferId = _VertexBuffers[0] ? GL_HANDLE( _VertexBuffers[0]->GetHandle() ) : 0;
            glVertexArrayVertexBuffer( id, _StartSlot, vertexBufferId, vao->VertexBufferOffsets[ _StartSlot ], vao->VertexBindingsStrides[ _StartSlot ] );
        }
        else
        {
            // Convert input parameters to OpenGL format
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                state->TmpHandles[ i ] = _VertexBuffers[i] ? GL_HANDLE( _VertexBuffers[i]->GetHandle() ) : 0;
                state->TmpPointers[ i ] = vao->VertexBufferOffsets[ _StartSlot + i ];
            }
            glVertexArrayVertexBuffers( id, _StartSlot, _NumBuffers, state->TmpHandles, state->TmpPointers, reinterpret_cast< const GLsizei * >( &vao->VertexBindingsStrides[ _StartSlot ] ) );
        }
    } else {

        for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
            unsigned int slot = _StartSlot + i;

            uint32_t uid = 0;
            uint32_t offset = 0;

            bModified = vao->VertexBufferUIDs[ slot ] != uid || vao->VertexBufferOffsets[ slot ] != offset;

            vao->VertexBufferUIDs[ slot ] = uid;
            vao->VertexBufferOffsets[ slot ] = offset;
        }

        if ( !bModified ) {
            return;
        }

        if ( _NumBuffers == 1 ) {
            glVertexArrayVertexBuffer( id, _StartSlot, 0, 0, 16 ); // From OpenGL specification
        } else {
            glVertexArrayVertexBuffers( id, _StartSlot, _NumBuffers, NULL, NULL, NULL );
        }
    }
}

void CommandBuffer::BindIndexBuffer( Buffer const * _IndexBuffer,
                                     INDEX_TYPE _Type,
                                     unsigned int _Offset ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    pipeline->IndexBufferType = IndexTypeLUT[ _Type ];
    pipeline->IndexBufferOffset = _Offset;
    pipeline->IndexBufferTypeSizeOf = IndexTypeSizeOfLUT[ _Type ];

    GLuint indexBufferId = _IndexBuffer ? GL_HANDLE( _IndexBuffer->GetHandle() ) : 0;
    uint32_t uid = _IndexBuffer ? _IndexBuffer->UID : 0;

    if ( pipeline->VAO->IndexBufferUID != uid ) {
        glVertexArrayElementBuffer( pipeline->VAO->Handle, indexBufferId );
        pipeline->VAO->IndexBufferUID = uid;

        //LogPrintf( "BindIndexBuffer %d\n", indexBufferId );
    } else {
        //LogPrintf( "Caching BindIndexBuffer %d\n", indexBufferId );
    }
}

void CommandBuffer::BindShaderResources( ShaderResources const * _Resources ) {
    State * state = GetCurrentState();

    for ( ShaderBufferBinding * slot = _Resources->Buffers ; slot < &_Resources->Buffers[_Resources->NumBuffers] ; slot++ ) {

        assert( slot->SlotIndex < MAX_BUFFER_SLOTS );

        GLenum target = BufferTargetLUT[ slot->BufferType ].Target;

        unsigned int id = slot->pBuffer ? GL_HANDLE( slot->pBuffer->GetHandle() ) : 0;

        if ( state->BufferBindings[ slot->SlotIndex ] != id || slot->BindingSize > 0 ) {
            state->BufferBindings[ slot->SlotIndex ] = id;

            if ( id && slot->BindingSize > 0 ) {
                glBindBufferRange( target, slot->SlotIndex, id, slot->BindingOffset, slot->BindingSize ); // 3.0 or GL_ARB_uniform_buffer_object
            } else {
                glBindBufferBase( target, slot->SlotIndex, id ); // 3.0 or GL_ARB_uniform_buffer_object
            }
        }
    }

    for ( ShaderSamplerBinding * slot = _Resources->Samplers ; slot < &_Resources->Samplers[_Resources->NumSamplers] ; slot++ ) {

        assert( slot->SlotIndex < MAX_SAMPLER_SLOTS );

        unsigned int id = GL_HANDLE( slot->pSampler );

        if ( state->SampleBindings[ slot->SlotIndex ] != id ) {
            state->SampleBindings[ slot->SlotIndex ] = id;

            glBindSampler( slot->SlotIndex, id ); // 3.2 or GL_ARB_sampler_objects
        }
    }

    for ( ShaderTextureBinding * slot = _Resources->Textures ; slot < &_Resources->Textures[_Resources->NumTextures] ; slot++ ) {

        assert( slot->SlotIndex < MAX_SAMPLER_SLOTS );

        unsigned int id = slot->pTexture ? GL_HANDLE( slot->pTexture->GetHandle() ) : 0;

        if ( state->TextureBindings[ slot->SlotIndex ] != id ) {
            state->TextureBindings[ slot->SlotIndex ] = id;

            glBindTextureUnit( slot->SlotIndex, id ); // 4.5
        }
    }

    for ( ShaderImageBinding * slot = _Resources->Images ; slot < &_Resources->Images[_Resources->NumImages] ; slot++ ) {

        assert( slot->SlotIndex < MAX_SAMPLER_SLOTS );

        // FIXME: Slot must be < Device->MaxImageUnits?

        unsigned int id = slot->pTexture ? GL_HANDLE( slot->pTexture->GetHandle() ) : 0;

        glBindImageTexture( slot->SlotIndex,
                            id,
                            slot->Lod,
                            slot->bLayered,
                            slot->LayerIndex,
                            ImageAccessModeLUT[ slot->AccessMode ],
                            InternalFormatLUT[ slot->InternalFormat ].InternalFormat ); // 4.2
    }
}

#if 0
void CommandBuffer::SetBuffers( BUFFER_TYPE _BufferType,
                                unsigned int _StartSlot,
                                unsigned int _NumBuffers,
                                /* optional */ Buffer * const * _Buffers,
                                /* optional */ unsigned int * _RangeOffsets,
                                /* optional */ unsigned int * _RangeSizes ) {
    State * state = GetCurrentState();

    // _StartSlot + _NumBuffers must be < storage->MaxBufferBindings[_BufferType]

    GLenum target = BufferTargetLUT[ _BufferType ].Target;

    if ( !_Buffers ) {
        glBindBuffersBase( target, _StartSlot, _NumBuffers, NULL ); // 4.4 or GL_ARB_multi_bind
        return;
    }

    if ( _RangeOffsets && _RangeSizes ) {
        if ( _NumBuffers == 1 ) {
            glBindBufferRange( target, _StartSlot, ( size_t )_Buffers[0]->GetHandle(), _RangeOffsets[0], _RangeSizes[0] ); // 3.0 or GL_ARB_uniform_buffer_object
        } else {
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                state->TmpHandles[ i ] = ( size_t )_Buffers[ i ]->GetHandle();
                state->TmpPointers[ i ] = _RangeOffsets[ i ];
                state->TmpPointers2[ i ] = _RangeSizes[ i ];
            }
            glBindBuffersRange( target, _StartSlot, _NumBuffers, state->TmpHandles, state->TmpPointers, state->TmpPointers2 ); // 4.4 or GL_ARB_multi_bind

            // Or  Since 3.0 or GL_ARB_uniform_buffer_object
            //if ( _Buffers != NULL) {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferRange( target, _StartSlot + i, GL_HANDLER_FROM_RESOURCE( _Buffers[i] ), _RangeOffsets​[i], _RangeSizes[i] );
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
                state->TmpHandles[ i ] = ( size_t )_Buffers[i]->GetHandle();
            }
            glBindBuffersBase( target, _StartSlot, _NumBuffers, state->TmpHandles ); // 4.4 or GL_ARB_multi_bind

            // Or  Since 3.0 or GL_ARB_uniform_buffer_object
            //if ( _Buffers != NULL) {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, GL_HANDLER_FROM_RESOURCE( _Buffers[i] ) );
            //    }
            //} else {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, 0 );
            //    }
            //}
        }
    }
}

void CommandBuffer::SetSampler( unsigned int _Slot,
                                /* optional */ Sampler * const _Sampler ) {
    SetSamplers( _Slot, 1, &_Sampler );
}

void CommandBuffer::SetSamplers( unsigned int _StartSlot,
                                 unsigned int _NumSamplers,
                                 /* optional */ Sampler * const * _Samplers ) {

    State * state = GetCurrentState();

    // _StartSlot + _NumSamples must be < MaxCombinedTextureImageUnits

    if ( !_Samplers ) {
        glBindSamplers( _StartSlot, _NumSamplers, NULL ); // 4.4 or GL_ARB_multi_bind
        return;
    }

    if ( _NumSamplers == 1 ) {
        glBindSampler( _StartSlot, ( size_t )_Samplers[0]->GetHandle() ); // 3.2 or GL_ARB_sampler_objects
    } else {
        for ( unsigned int i = 0 ; i < _NumSamplers ; i++ ) {
            state->TmpHandles[ i ] = ( size_t )_Samplers[i]->GetHandle();
        }
        glBindSamplers( _StartSlot, _NumSamplers, state->TmpHandles ); // 4.4 or GL_ARB_multi_bind

        // Or  Since 3.2 or GL_ARB_sampler_objects
        //if ( _Samplers != NULL) {
        //    for ( int i = 0 ; i < _NumSamplers ; i++ ) {
        //        glBindSampler( _StartSlot + i, GL_HANDLER_FROM_RESOURCE( _Samplers[i] ) );
        //    }
        //} else {
        //    for ( int i = 0 ; i < _NumSamplers ; i++ ) {
        //        glBindSampler( _StartSlot + i, 0 );
        //    }
        //}
    }
}

void CommandBuffer::SetTextureUnit( unsigned int _Unit, /* optional */ Texture const * _Texture ) {

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

void CommandBuffer::SetTextureUnits( unsigned int _FirstUnit,
                                        unsigned int _NumUnits,
                                        /* optional */ Texture * const * _Textures ) {
    State * state = GetCurrentState();

    // _FirstUnit + _NumUnits must be < MaxCombinedTextureImageUnits

    if ( _Textures ) {
        for ( unsigned int i = 0 ; i < _NumUnits ; i++ ) {
            state->TmpHandles[ i ] = ( size_t )_Textures[ i ]->GetHandle();
        }

        glBindTextures(	_FirstUnit, _NumUnits, state->TmpHandles ); // 4.4
    } else {
        glBindTextures(	_FirstUnit, _NumUnits, NULL ); // 4.4
    }


    /*
    // Other path
    for (i = 0; i < _NumUnits; i++) {
        GLuint texture;
        if (textures == NULL) {
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

bool CommandBuffer::SetImageUnit( unsigned int _Unit,
                                  /* optional */ Texture * _Texture,
                                  unsigned int _Lod,
                                  bool _AllLayers,
                                  unsigned int _LayerIndex,  // Array index for texture arrays, depth for 3D textures or cube face for cubemaps
                                  // For cubemap arrays: arrayLength = floor( _LayerIndex / 6 ), face = _LayerIndex % 6
                                  IMAGE_ACCESS_MODE _AccessMode,
                                  INTERNAL_PIXEL_FORMAT _InternalFormat ) {

    if ( !*InternalFormatLUT[ _InternalFormat ].ShaderImageFormatQualifier ) {
        return false;
    }

    // Unit must be < state->ResourceStoragesPtr->MaxImageUnits
    glBindImageTexture( _Unit,
                        ( size_t )_Texture->GetHandle(),
                        _Lod,
                        _AllLayers,
                        _LayerIndex,
                        ImageAccessModeLUT[ _AccessMode ],
                        InternalFormatLUT[ _InternalFormat ].InternalFormat ); // 4.2

    return true;
}

void CommandBuffer::SetImageUnits( unsigned int _FirstUnit,
                                      unsigned int _NumUnits,
                                      /* optional */ Texture * const * _Textures ) {

    State * state = GetCurrentState();

    // _FirstUnit + _NumUnits must be < state->ResourceStoragesPtr->MaxImageUnits

    if ( _Textures ) {
        for ( unsigned int i = 0 ; i < _NumUnits ; i++ ) {
            state->TmpHandles[ i ] = ( size_t )_Textures[ i ]->GetHandle();
        }

        glBindImageTextures( _FirstUnit, _NumUnits, state->TmpHandles ); // 4.4
    } else {
        glBindImageTextures( _FirstUnit, _NumUnits, NULL ); // 4.4
    }

    /*
    // Other path:
    for (i = 0; i < _NumUnits; i++) {
        if (textures == NULL || textures[i] = 0) {
            glBindImageTexture(_FirstUnit + i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8); // 4.2
        } else {
            glBindImageTexture(_FirstUnit + i, textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, lookupInternalFormat(textures[i])); // 4.2
        }
    }
    */
}

#endif

#define INVERT_VIEWPORT_Y(State,Viewport) ( state->Binding.DrawFramebufferHeight - (Viewport)->Y - (Viewport)->Height )

void CommandBuffer::SetViewport( Viewport const & _Viewport ) {
    State * state = GetCurrentState();

    if ( state->ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT ) {
        glViewport( (GLint)_Viewport.X,
                    (GLint)INVERT_VIEWPORT_Y(state,&_Viewport),
                    (GLsizei)_Viewport.Width,
                    (GLsizei)_Viewport.Height );
    } else {
        glViewport( (GLint)_Viewport.X,
                    (GLint)_Viewport.Y,
                    (GLsizei)_Viewport.Width,
                    (GLsizei)_Viewport.Height );
    }

    if ( glDepthRangef ) { // Since GL v4.1
        glDepthRangef( _Viewport.MinDepth, _Viewport.MaxDepth );
    } else {
        glDepthRange( _Viewport.MinDepth, _Viewport.MaxDepth );
    }
}

void CommandBuffer::SetViewportArray( uint32_t _NumViewports, Viewport const * _Viewports ) {
    SetViewportArray( 0, _NumViewports, _Viewports );
}

void CommandBuffer::SetViewportArray( uint32_t _FirstIndex, uint32_t _NumViewports, Viewport const * _Viewports ) {
    #define MAX_VIEWPORT_DATA 1024
    static_assert( sizeof( float ) * 2 == sizeof( double ), "CommandBuffer::SetViewportArray type check" );
    constexpr uint32_t MAX_VIEWPORTS = MAX_VIEWPORT_DATA >> 2;
    float viewportData[ MAX_VIEWPORT_DATA ];

    _NumViewports = std::min( MAX_VIEWPORTS, _NumViewports );

    State * state = GetCurrentState();

    bool bInvertY = state->ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    float * pViewportData = viewportData;
    for ( Viewport const * viewport = _Viewports ; viewport < &_Viewports[_NumViewports] ; viewport++, pViewportData += 4 ) {
        pViewportData[ 0 ] = viewport->X;
        pViewportData[ 1 ] = bInvertY ? INVERT_VIEWPORT_Y( state, viewport ) : viewport->Y;
        pViewportData[ 2 ] = viewport->Width;
        pViewportData[ 3 ] = viewport->Height;
    }
    glViewportArrayv( _FirstIndex, _NumViewports, viewportData );

    double * pDepthRangeData = reinterpret_cast< double * >( &viewportData[0] );
    for ( Viewport const * viewport = _Viewports ; viewport < &_Viewports[_NumViewports] ; viewport++, pDepthRangeData += 2 ) {
        pDepthRangeData[ 0 ] = viewport->MinDepth;
        pDepthRangeData[ 1 ] = viewport->MaxDepth;
    }
    glDepthRangeArrayv( _FirstIndex, _NumViewports, reinterpret_cast< double * >( &viewportData[0] ) );
}

void CommandBuffer::SetViewportIndexed( uint32_t _Index, Viewport const & _Viewport ) {
    State * state = GetCurrentState();
    bool bInvertY = state->ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;
    float viewportData[4] = { _Viewport.X, bInvertY ? INVERT_VIEWPORT_Y(state,&_Viewport) : _Viewport.Y, _Viewport.Width, _Viewport.Height };
    glViewportIndexedfv( _Index, viewportData );
    glDepthRangeIndexed( _Index, _Viewport.MinDepth, _Viewport.MaxDepth );
}

void CommandBuffer::SetScissor( /* optional */ Rect2D const & _Scissor ) {
    State * state = GetCurrentState();

    state->CurrentScissor = _Scissor;

    bool bInvertY = state->ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    glScissor( state->CurrentScissor.X,
               bInvertY ? state->Binding.DrawFramebufferHeight - state->CurrentScissor.Y - state->CurrentScissor.Height : state->CurrentScissor.Y,
               state->CurrentScissor.Width,
               state->CurrentScissor.Height );
}

void CommandBuffer::SetScissorArray( uint32_t _NumScissors, Rect2D const * _Scissors ) {
    SetScissorArray( 0, _NumScissors, _Scissors );
}

void CommandBuffer::SetScissorArray( uint32_t _FirstIndex, uint32_t _NumScissors, Rect2D const * _Scissors ) {
    #define MAX_SCISSOR_DATA 1024
    constexpr uint32_t MAX_SCISSORS = MAX_VIEWPORT_DATA >> 2;
    GLint scissorData[ MAX_SCISSOR_DATA ];

    _NumScissors = std::min( MAX_SCISSORS, _NumScissors );

    State * state = GetCurrentState();

    bool bInvertY = state->ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    GLint * pScissorData = scissorData;
    for ( Rect2D const * scissor = _Scissors ; scissor < &_Scissors[_NumScissors] ; scissor++, pScissorData += 4 ) {
        pScissorData[ 0 ] = scissor->X;
        pScissorData[ 1 ] = bInvertY ? INVERT_VIEWPORT_Y( state, scissor ) : scissor->Y;
        pScissorData[ 2 ] = scissor->Width;
        pScissorData[ 3 ] = scissor->Height;
    }
    glScissorArrayv( _FirstIndex, _NumScissors, scissorData );
}

void CommandBuffer::SetScissorIndexed( uint32_t _Index, Rect2D const & _Scissor ) {
    State * state = GetCurrentState();

    bool bInvertY = state->ViewportOrigin == VIEWPORT_ORIGIN_TOP_LEFT;

    int scissorData[4] = { _Scissor.X, bInvertY ? INVERT_VIEWPORT_Y(state,&_Scissor) : _Scissor.Y, _Scissor.Width, _Scissor.Height };
    glScissorIndexedv( _Index, scissorData );
}

void CommandBuffer::Draw( DrawCmd const * _Cmd ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( _Cmd->InstanceCount == 0 || _Cmd->VertexCountPerInstance == 0 ) {
        return;
    }

    if ( _Cmd->InstanceCount == 1 && _Cmd->StartInstanceLocation == 0 ) {
        glDrawArrays( pipeline->PrimitiveTopology, _Cmd->StartVertexLocation, _Cmd->VertexCountPerInstance ); // Since 2.0
    } else {
        if ( _Cmd->StartInstanceLocation == 0 ) {
            glDrawArraysInstanced( pipeline->PrimitiveTopology, _Cmd->StartVertexLocation, _Cmd->VertexCountPerInstance, _Cmd->InstanceCount );// Since 3.1
        } else {
            glDrawArraysInstancedBaseInstance( pipeline->PrimitiveTopology,
                _Cmd->StartVertexLocation,
                _Cmd->VertexCountPerInstance,
                _Cmd->InstanceCount,
                _Cmd->StartInstanceLocation
            ); // Since 4.2 or GL_ARB_base_instance
        }
    }
}

void CommandBuffer::Draw( DrawIndexedCmd const * _Cmd ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( _Cmd->InstanceCount == 0 || _Cmd->IndexCountPerInstance == 0 ) {
        return;
    }

    const GLubyte * offset = reinterpret_cast<const GLubyte *>( 0 ) + _Cmd->StartIndexLocation * pipeline->IndexBufferTypeSizeOf
        + pipeline->IndexBufferOffset; // FIXME: check this IndexBufferOffset

    if ( _Cmd->InstanceCount == 1 && _Cmd->StartInstanceLocation == 0 ) {
        if ( _Cmd->BaseVertexLocation == 0 ) {
            glDrawElements( pipeline->PrimitiveTopology,
                            _Cmd->IndexCountPerInstance,
                            pipeline->IndexBufferType,
                            offset ); // 2.0
        } else {
            glDrawElementsBaseVertex( pipeline->PrimitiveTopology,
                                      _Cmd->IndexCountPerInstance,
                                      pipeline->IndexBufferType,
                                      offset,
                                      _Cmd->BaseVertexLocation
                                      ); // 3.2 or GL_ARB_draw_elements_base_vertex
        }
    } else {
        if ( _Cmd->StartInstanceLocation == 0 ) {
            if ( _Cmd->BaseVertexLocation == 0 ) {
                glDrawElementsInstanced( pipeline->PrimitiveTopology,
                                         _Cmd->IndexCountPerInstance,
                                         pipeline->IndexBufferType,
                                         offset,
                                         _Cmd->InstanceCount ); // 3.1
            } else {
                glDrawElementsInstancedBaseVertex( pipeline->PrimitiveTopology,
                                                   _Cmd->IndexCountPerInstance,
                                                   pipeline->IndexBufferType,
                                                   offset,
                                                   _Cmd->InstanceCount,
                                                   _Cmd->BaseVertexLocation ); // 3.2 or GL_ARB_draw_elements_base_vertex
            }
        } else {
            if ( _Cmd->BaseVertexLocation == 0 ) {
                glDrawElementsInstancedBaseInstance( pipeline->PrimitiveTopology,
                                                     _Cmd->IndexCountPerInstance,
                                                     pipeline->IndexBufferType,
                                                     offset,
                                                     _Cmd->InstanceCount,
                                                     _Cmd->StartInstanceLocation ); // 4.2 or GL_ARB_base_instance
            } else {
                glDrawElementsInstancedBaseVertexBaseInstance( pipeline->PrimitiveTopology,
                                                               _Cmd->IndexCountPerInstance,
                                                               pipeline->IndexBufferType,
                                                               offset,
                                                               _Cmd->InstanceCount,
                                                               _Cmd->BaseVertexLocation,
                                                               _Cmd->StartInstanceLocation ); // 4.2 or GL_ARB_base_instance
            }
        }
    }
}

void CommandBuffer::Draw( TransformFeedback & _TransformFeedback, unsigned int _InstanceCount, unsigned int _StreamIndex ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( _InstanceCount == 0 ) {
        return;
    }

    if ( _InstanceCount > 1 ) {
        if ( _StreamIndex == 0 ) {
            glDrawTransformFeedbackInstanced( pipeline->PrimitiveTopology, GL_HANDLE( _TransformFeedback.GetHandle() ), _InstanceCount ); // 4.2
        } else {
            glDrawTransformFeedbackStreamInstanced( pipeline->PrimitiveTopology, GL_HANDLE( _TransformFeedback.GetHandle() ), _StreamIndex, _InstanceCount ); // 4.2
        }
    } else {
        if ( _StreamIndex == 0 ) {
            glDrawTransformFeedback( pipeline->PrimitiveTopology, GL_HANDLE( _TransformFeedback.GetHandle() ) ); // 4.0
        } else {
            glDrawTransformFeedbackStream( pipeline->PrimitiveTopology, GL_HANDLE( _TransformFeedback.GetHandle() ), _StreamIndex );  // 4.0
        }
    }
}

void CommandBuffer::DrawIndirect( DrawIndirectCmd const * _Cmd ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( state->Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        state->Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawArraysInstancedBaseInstance
    glDrawArraysIndirect( pipeline->PrimitiveTopology, _Cmd ); // Since 4.0 or GL_ARB_draw_indirect
}

void CommandBuffer::DrawIndirect( DrawIndexedIndirectCmd const * _Cmd ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( state->Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        state->Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawElementsInstancedBaseVertexBaseInstance
    glDrawElementsIndirect( pipeline->PrimitiveTopology, pipeline->IndexBufferType, _Cmd ); // Since 4.0 or GL_ARB_draw_indirect
}

void CommandBuffer::DrawIndirect( Buffer * _DrawIndirectBuffer, unsigned int _AlignedByteOffset, bool _Indexed ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    GLuint handle = GL_HANDLE( _DrawIndirectBuffer->GetHandle() );
    if ( state->Binding.DrawInderectBuffer != handle ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, handle );
        state->Binding.DrawInderectBuffer = handle;
    }

    if ( _Indexed ) {
        // This is similar glDrawElementsInstancedBaseVertexBaseInstance, but with binded INDIRECT buffer
        glDrawElementsIndirect( pipeline->PrimitiveTopology,
                                pipeline->IndexBufferType,
                                reinterpret_cast< const GLubyte * >(0) + _AlignedByteOffset ); // Since 4.0 or GL_ARB_draw_indirect
    } else {
        // This is similar glDrawArraysInstancedBaseInstance, but with binded INDIRECT buffer
        glDrawArraysIndirect( pipeline->PrimitiveTopology,
                              reinterpret_cast< const GLubyte * >(0) + _AlignedByteOffset ); // Since 4.0 or GL_ARB_draw_indirect
    }
}

void CommandBuffer::MultiDraw( unsigned int _DrawCount, const unsigned int * _VertexCount, const unsigned int * _StartVertexLocations ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    static_assert( sizeof( _VertexCount[0] ) == sizeof( GLsizei ), "!" );
    static_assert( sizeof( _StartVertexLocations[0] ) == sizeof( GLint ), "!" );

    glMultiDrawArrays( pipeline->PrimitiveTopology, ( const GLint * )_StartVertexLocations, ( const GLsizei * )_VertexCount, _DrawCount ); // Since 2.0

    // Эквивалентный код:
    //for ( unsigned int i = 0 ; i < _DrawCount ; i++ ) {
    //    glDrawArrays( pipeline->PrimitiveTopology, _StartVertexLocation[i], _VertexCount[i] );
    //}
}

void CommandBuffer::MultiDraw( unsigned int _DrawCount, const unsigned int * _IndexCount, const void * const * _IndexByteOffsets, const int * _BaseVertexLocations ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    static_assert( sizeof( unsigned int ) == sizeof( GLsizei ), "!" );

    // state->IndexBufferOffset; // FIXME: how to apply IndexBufferOffset?

    if ( _BaseVertexLocations ) {
        glMultiDrawElementsBaseVertex( pipeline->PrimitiveTopology,
                                       ( const GLsizei * )_IndexCount,
                                       pipeline->IndexBufferType,
                                       _IndexByteOffsets,
                                       _DrawCount,
                                       _BaseVertexLocations ); // 3.2
        // Эквивалентный код:
        //    for ( int i = 0 ; i < _DrawCount ; i++ )
        //        if ( _IndexCount[i] > 0 )
        //            glDrawElementsBaseVertex( pipeline->PrimitiveTopology,
        //                                      _IndexCount[i],
        //                                      state->IndexBufferType,
        //                                      _IndexByteOffsets[i],
        //                                      _BaseVertexLocations[i]);
    } else {
        glMultiDrawElements( pipeline->PrimitiveTopology,
                             ( const GLsizei * )_IndexCount,
                             pipeline->IndexBufferType,
                             _IndexByteOffsets,
                             _DrawCount ); // 2.0
    }
}

void CommandBuffer::MultiDrawIndirect( unsigned int _DrawCount, DrawIndirectCmd const * _Cmds, unsigned int _Stride ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( state->Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        state->Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawArraysInstancedBaseInstance
    glMultiDrawArraysIndirect( pipeline->PrimitiveTopology,
                               _Cmds,
                               _DrawCount,
                               _Stride); // 4.3 or GL_ARB_multi_draw_indirect


// Эквивалентный код:
//    GLsizei n;
//    for ( i = 0 ; i < _DrawCount ; i++ ) {
//        DrawIndirectCmd const *cmd = ( _Stride != 0 ) ?
//              (DrawIndirectCmd const *)((uintptr)indirect + i * _Stride) : ((GHI_DrawIndirectCmd_t const *)indirect + i);
//        glDrawArraysInstancedBaseInstance(mode, cmd->first, cmd->count, cmd->instanceCount, cmd->baseInstance);
//    }
}

void CommandBuffer::MultiDrawIndirect( unsigned int _DrawCount, DrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( state->Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        state->Binding.DrawInderectBuffer = 0;
    }

    glMultiDrawElementsIndirect( pipeline->PrimitiveTopology,
                                 pipeline->IndexBufferType,
                                 _Cmds,
                                 _DrawCount,
                                 _Stride ); // 4.3

// Эквивалентный код:
//    GLsizei n;
//    for ( i = 0 ; n < _DrawCount ; i++ ) {
//        GHI_DrawIndexedIndirectCmd_t const *cmd = ( stride != 0 ) ?
//            (GHI_DrawIndexedIndirectCmd_t const *)((uintptr)indirect + n * stride) : ( (GHI_DrawIndexedIndirectCmd_t const *)indirect + n );
//        glDrawElementsInstancedBaseVertexBaseInstance(pipeline->PrimitiveTopology,
//                                                      cmd->count,
//                                                      state->IndexBufferType,
//                                                      cmd->firstIndex + size-of-type,
//                                                      cmd->instanceCount,
//                                                      cmd->baseVertex,
//                                                      cmd->baseInstance);
//    }
}

void CommandBuffer::DispatchCompute( unsigned int _ThreadGroupCountX,
                                     unsigned int _ThreadGroupCountY,
                                     unsigned int _ThreadGroupCountZ ) {

    // Must be: ThreadGroupCount <= GL_MAX_COMPUTE_WORK_GROUP_COUNT

    glDispatchCompute( _ThreadGroupCountX, _ThreadGroupCountY, _ThreadGroupCountZ ); // 4.3 or GL_ARB_compute_shader
}

void CommandBuffer::DispatchCompute( DispatchIndirectCmd const * _Cmd ) {
    State * state = GetCurrentState();

    if ( state->Binding.DispatchIndirectBuffer != 0 ) {
        glBindBuffer( GL_DISPATCH_INDIRECT_BUFFER, 0 );
        state->Binding.DispatchIndirectBuffer = 0;
    }

    glDispatchComputeIndirect( ( GLintptr ) _Cmd ); // 4.3 or GL_ARB_compute_shader

    // or
    //glDispatchCompute( _Cmd->ThreadGroupCountX, _Cmd->ThreadGroupCountY, _Cmd->ThreadGroupCountZ ); // 4.3 or GL_ARB_compute_shader
}

void CommandBuffer::DispatchComputeIndirect( Buffer * _DispatchIndirectBuffer,
                                             unsigned int _AlignedByteOffset ) {
    State * state = GetCurrentState();

    GLuint handle = GL_HANDLE( _DispatchIndirectBuffer->GetHandle() );
    if ( state->Binding.DispatchIndirectBuffer != handle ) {
        glBindBuffer( GL_DISPATCH_INDIRECT_BUFFER, handle );
        state->Binding.DispatchIndirectBuffer = handle;
    }

    GLubyte * offset = reinterpret_cast< GLubyte * >(0) + _AlignedByteOffset;
    glDispatchComputeIndirect( ( GLintptr ) offset ); // 4.3 or GL_ARB_compute_shader
}

void CommandBuffer::BeginQuery( QueryPool * _QueryPool, uint32_t _QueryID, uint32_t _StreamIndex ) {
    assert( _QueryID < _QueryPool->CreateInfo.PoolSize );
    if ( _StreamIndex == 0 ) {
        glBeginQuery( TableQueryTarget[ _QueryPool->CreateInfo.Target ], _QueryPool->IdPool[_QueryID] ); // 2.0
    } else {
        glBeginQueryIndexed( TableQueryTarget[ _QueryPool->CreateInfo.Target ], _QueryPool->IdPool[_QueryID], _StreamIndex ); // 4.0
    }
}

void CommandBuffer::EndQuery( QueryPool * _QueryPool, uint32_t _StreamIndex ) {
    if ( _StreamIndex == 0 ) {
        glEndQuery( TableQueryTarget[ _QueryPool->CreateInfo.Target ] ); // 2.0
    } else {
        glEndQueryIndexed( TableQueryTarget[ _QueryPool->CreateInfo.Target ], _StreamIndex ); // 4.0
    }
}

void CommandBuffer::BeginConditionalRender( QueryPool * _QueryPool, uint32_t _QueryID, CONDITIONAL_RENDER_MODE _Mode ) {
    assert( _QueryID < _QueryPool->CreateInfo.PoolSize );
    glBeginConditionalRender( _QueryPool->IdPool[ _QueryID ], TableConditionalRenderMode[ _Mode ] ); // 4.4 (with some flags 3.0)
}

void CommandBuffer::EndConditionalRender() {
    glEndConditionalRender();  // 3.0
}

void CommandBuffer::CopyQueryPoolResultsAvailable( QueryPool * _QueryPool,
                                                   uint32_t _FirstQuery,
                                                   uint32_t _QueryCount,
                                                   Buffer * _DstBuffer,
                                                   size_t _DstOffst,
                                                   size_t _DstStride,
                                                   bool _QueryResult64Bit ) {

    assert( _FirstQuery + _QueryCount <= _QueryPool->CreateInfo.PoolSize );

    const GLuint bufferId = GL_HANDLE( _DstBuffer->GetHandle() );
    const size_t bufferSize = _DstBuffer->GetSizeInBytes();

    if ( _QueryResult64Bit ) {

        assert( ( _DstStride & ~(size_t)7 ) == _DstStride ); // check stride must be multiples of 8

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint64_t) > bufferSize ) {
                LogPrintf( "CommandBuffer::CopyQueryPoolResults: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectui64v( _QueryPool->IdPool[ _FirstQuery + index ], bufferId, GL_QUERY_RESULT_AVAILABLE, _DstOffst );  // 4.5

            _DstOffst += _DstStride;
        }
    } else {

        assert( ( _DstStride & ~(size_t)3 ) == _DstStride ); // check stride must be multiples of 4

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint32_t) > bufferSize ) {
                LogPrintf( "CommandBuffer::CopyQueryPoolResults: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectuiv( _QueryPool->IdPool[ _FirstQuery + index ], bufferId, GL_QUERY_RESULT_AVAILABLE, _DstOffst );  // 4.5

            _DstOffst += _DstStride;
        }
    }
}

void CommandBuffer::CopyQueryPoolResults( QueryPool * _QueryPool,
                                          uint32_t _FirstQuery,
                                          uint32_t _QueryCount,
                                          Buffer * _DstBuffer,
                                          size_t _DstOffst,
                                          size_t _DstStride,
                                          QUERY_RESULT_FLAGS _Flags ) {

    assert( _FirstQuery + _QueryCount <= _QueryPool->CreateInfo.PoolSize );

    const GLuint bufferId = GL_HANDLE( _DstBuffer->GetHandle() );
    const size_t bufferSize = _DstBuffer->GetSizeInBytes();

    GLenum pname = ( _Flags & QUERY_RESULT_WAIT_BIT ) ? GL_QUERY_RESULT : GL_QUERY_RESULT_NO_WAIT;

    if ( _Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT ) {
        LogPrintf( "CommandBuffer::CopyQueryPoolResults: ignoring flag QUERY_RESULT_WITH_AVAILABILITY_BIT. Use CopyQueryPoolResultsAvailable to get available status.\n" );
    }

    if ( _Flags & QUERY_RESULT_64_BIT ) {

        assert( ( _DstStride & ~(size_t)7 ) == _DstStride ); // check stride must be multiples of 8

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint64_t) > bufferSize ) {
                LogPrintf( "CommandBuffer::CopyQueryPoolResults: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectui64v( _QueryPool->IdPool[ _FirstQuery + index ], bufferId, pname, _DstOffst ); // 4.5

            _DstOffst += _DstStride;
        }
    } else {

        assert( ( _DstStride & ~(size_t)3 ) == _DstStride ); // check stride must be multiples of 4

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( _DstOffst + sizeof(uint32_t) > bufferSize ) {
                LogPrintf( "CommandBuffer::CopyQueryPoolResults: out of buffer size\n" );
                break;
            }

            glGetQueryBufferObjectuiv( _QueryPool->IdPool[ _FirstQuery + index ], bufferId, pname, _DstOffst ); // 4.5

            _DstOffst += _DstStride;
        }
    }
}

void CommandBuffer::BeginRenderPassDefaultFramebuffer( RenderPassBegin const & _RenderPassBegin ) {
    State * state = GetCurrentState();

    const int framebufferId = 0;

    if ( state->Binding.DrawFramebuffer != framebufferId ) {

        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebufferId );

        state->Binding.DrawFramebuffer = framebufferId;
        state->Binding.DrawFramebufferWidth = (unsigned short)state->SwapChainWidth;
        state->Binding.DrawFramebufferHeight = ( unsigned short )state->SwapChainHeight;
    }

    bool bScissorEnabled = state->RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = state->RasterizerState.bRasterizerDiscard;

    if ( _RenderPassBegin.pRenderPass->NumColorAttachments > 0 ) {

        AttachmentInfo const * attachment = &_RenderPassBegin.pRenderPass->ColorAttachments[ 0 ];
        //FramebufferAttachmentInfo const * framebufferAttachment = &framebuffer->ColorAttachments[ i ];

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {

            assert( _RenderPassBegin.pColorClearValues != nullptr );

            ClearColorValue const * clearValue = &_RenderPassBegin.pColorClearValues[ 0 ];

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }


            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            RenderTargetBlendingInfo const & currentState = state->BlendState.RenderTargetSlots[ 0 ];
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

    if ( _RenderPassBegin.pRenderPass->bHasDepthStencilAttachment ) {
        AttachmentInfo const * attachment = &_RenderPassBegin.pRenderPass->DepthStencilAttachment;
        //FramebufferAttachmentInfo const * framebufferAttachment = &framebuffer->DepthStencilAttachment;

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {
            ClearDepthStencilValue const * clearValue = _RenderPassBegin.pDepthStencilClearValue;

            assert( clearValue != nullptr );

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }

            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            if ( state->DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
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

            if ( state->DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 0 );
            }
        }
    }

    // Restore scissor test
    if ( bScissorEnabled != state->RasterizerState.bScissorEnable ) {
        if ( state->RasterizerState.bScissorEnable ) {
            glEnable( GL_SCISSOR_TEST );
        } else {
            glDisable( GL_SCISSOR_TEST );
        }
    }

    // Restore rasterizer discard
    if ( bRasterizerDiscard != state->RasterizerState.bRasterizerDiscard ) {
        if ( state->RasterizerState.bRasterizerDiscard ) {
            glEnable( GL_RASTERIZER_DISCARD );
        } else {
            glDisable( GL_RASTERIZER_DISCARD );
        }
    }
}

void CommandBuffer::BeginRenderPass( RenderPassBegin const & _RenderPassBegin ) {
    State * state = GetCurrentState();

    RenderPass * renderPass = _RenderPassBegin.pRenderPass;
    Framebuffer * framebuffer = _RenderPassBegin.pFramebuffer;

    assert( state->CurrentRenderPass == nullptr );

    state->CurrentRenderPass = renderPass;
    state->CurrentRenderPassRenderArea = _RenderPassBegin.RenderArea;

    if ( !framebuffer ) {
        // default framebuffer
        BeginRenderPassDefaultFramebuffer( _RenderPassBegin );
        return;
    }

    GLuint framebufferId = GL_HANDLE( framebuffer->GetHandle() );

    if ( !framebufferId ) {
        LogPrintf( "Buffer::BeginRenderPass: invalid framebuffer\n" );
        return;
    }

    if ( state->Binding.DrawFramebuffer != framebufferId ) {

        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, framebufferId );

        state->Binding.DrawFramebuffer = framebufferId;
        state->Binding.DrawFramebufferWidth = framebuffer->GetWidth();
        state->Binding.DrawFramebufferHeight = framebuffer->GetHeight();
    }

    bool bScissorEnabled = state->RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = state->RasterizerState.bRasterizerDiscard;

    FramebufferAttachmentInfo const * framebufferColorAttachments = framebuffer->GetColorAttachments();

    for ( unsigned int i = 0 ; i < renderPass->NumColorAttachments ; i++ ) {

        AttachmentInfo const * attachment = &renderPass->ColorAttachments[ i ];
        FramebufferAttachmentInfo const * framebufferAttachment = &framebufferColorAttachments[ i ];

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {

            assert( _RenderPassBegin.pColorClearValues != nullptr );

            ClearColorValue const * clearValue = &_RenderPassBegin.pColorClearValues[ i ];

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }


            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            RenderTargetBlendingInfo const & currentState = state->BlendState.RenderTargetSlots[ i ];
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                glColorMaski( i, 1, 1, 1, 1 );
            }

            // We must set draw buffers to clear attachment :(
            GLenum attachments[ 1 ] = { GL_COLOR_ATTACHMENT0 + i };
            glNamedFramebufferDrawBuffers( framebufferId, 1, attachments );

            // Clear attachment
            switch ( InternalFormatLUT[ framebufferAttachment->pTexture->GetInternalPixelFormat() ].ClearType ) {
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
                assert( 0 );
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

    if ( renderPass->bHasDepthStencilAttachment ) {
        AttachmentInfo const * attachment = &renderPass->DepthStencilAttachment;
        FramebufferAttachmentInfo const & framebufferAttachment = framebuffer->GetDepthStencilAttachment();

        if ( attachment->LoadOp == ATTACHMENT_LOAD_OP_CLEAR ) {
            ClearDepthStencilValue const * clearValue = _RenderPassBegin.pDepthStencilClearValue;

            assert( clearValue != nullptr );

            if ( !bScissorEnabled ) {
                glEnable( GL_SCISSOR_TEST );
                bScissorEnabled = true;
            }

            SetScissor( _RenderPassBegin.RenderArea );

            if ( bRasterizerDiscard ) {
                glDisable( GL_RASTERIZER_DISCARD );
                bRasterizerDiscard = false;
            }

            if ( state->DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 1 );
            }

            // TODO: table
            switch ( InternalFormatLUT[ framebufferAttachment.pTexture->GetInternalPixelFormat() ].ClearType ) {
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
                assert( 0 );
            }

            if ( state->DepthStencilState.DepthWriteMask == DEPTH_WRITE_DISABLE ) {
                glDepthMask( 0 );
            }
        }
    }

    // Restore scissor test
    if ( bScissorEnabled != state->RasterizerState.bScissorEnable ) {
        if ( state->RasterizerState.bScissorEnable ) {
            glEnable( GL_SCISSOR_TEST );
        } else {
            glDisable( GL_SCISSOR_TEST );
        }
    }

    // Restore rasterizer discard
    if ( bRasterizerDiscard != state->RasterizerState.bRasterizerDiscard ) {
        if ( state->RasterizerState.bRasterizerDiscard ) {
            glEnable( GL_RASTERIZER_DISCARD );
        } else {
            glDisable( GL_RASTERIZER_DISCARD );
        }
    }
}

void CommandBuffer::EndRenderPass() {
    State * state = GetCurrentState();

    state->CurrentRenderPass = nullptr;
}

void CommandBuffer::BindTransformFeedback( TransformFeedback & _TransformFeedback ) {
    // FIXME: Move transform feedback to Pipeline? Call glBindTransformFeedback in BindPipeline()?
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, GL_HANDLE( _TransformFeedback.GetHandle() ) );
}

void CommandBuffer::BeginTransformFeedback( PRIMITIVE_TOPOLOGY _OutputPrimitive ) {
    GLenum topology = GL_POINTS;

    if ( _OutputPrimitive <= PRIMITIVE_TRIANGLE_STRIP_ADJ ) {
        topology = PrimitiveTopologyLUT[ _OutputPrimitive ];
    }

    glBeginTransformFeedback( topology ); // 3.0
}

void CommandBuffer::ResumeTransformFeedback() {
    glResumeTransformFeedback();
}

void CommandBuffer::PauseTransformFeedback() {
    glPauseTransformFeedback();
}

void CommandBuffer::EndTransformFeedback() {
    glEndTransformFeedback(); // 3.0
}

FSync CommandBuffer::FenceSync() {
    FSync sync = reinterpret_cast< FSync >( glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 ) );
    return sync;
}

void CommandBuffer::RemoveSync( FSync _Sync ) {
    glDeleteSync( reinterpret_cast< GLsync >( _Sync ) );
}

CLIENT_WAIT_STATUS CommandBuffer::ClientWait( FSync _Sync, uint64_t _TimeOutNanoseconds ) {
    static_assert( 0xFFFFFFFFFFFFFFFF == GL_TIMEOUT_IGNORED, "Constant check" );
    return static_cast< CLIENT_WAIT_STATUS >(
                glClientWaitSync( reinterpret_cast< GLsync >( _Sync ), GL_SYNC_FLUSH_COMMANDS_BIT, _TimeOutNanoseconds ) - GL_ALREADY_SIGNALED );
}

void CommandBuffer::ServerWait( FSync _Sync ) {
    glWaitSync( reinterpret_cast< GLsync >( _Sync ), 0, GL_TIMEOUT_IGNORED );
}

bool CommandBuffer::IsSignaled( FSync _Sync ) {
    GLint value;
    glGetSynciv( reinterpret_cast< GLsync >( _Sync ),
                 GL_SYNC_STATUS,
                 sizeof( GLint ),
                 NULL,
                 &value );
    return value == GL_SIGNALED;
}

void CommandBuffer::Flush() {
    glFlush();
}

void CommandBuffer::Barrier( int _BarrierBits ) {
    glMemoryBarrier( _BarrierBits ); // 4.2
}

void CommandBuffer::BarrierByRegion( int _BarrierBits ) {
    glMemoryBarrierByRegion( _BarrierBits ); // 4.5
}

void CommandBuffer::TextureBarrier() {
    glTextureBarrier(); // 4.5
}

void CommandBuffer::DynamicState_BlendingColor( /* optional */ const float _ConstantColor[4] ) {
    constexpr const float defaultColor[4] = { 0, 0, 0, 0 };

    State * state = GetCurrentState();

    // Validate blend color
    _ConstantColor = _ConstantColor ? _ConstantColor : defaultColor;

    // Apply blend color
    bool isColorChanged = !BlendCompareColor( state->BlendColor, _ConstantColor );
    if ( isColorChanged ) {
        glBlendColor( _ConstantColor[0], _ConstantColor[1], _ConstantColor[2], _ConstantColor[3] );
        memcpy( state->BlendColor, _ConstantColor, sizeof( state->BlendColor ) );
    }
}

void CommandBuffer::DynamicState_SampleMask( /* optional */ const uint32_t _SampleMask[4] ) {
    State * state = GetCurrentState();

    // Apply sample mask
    if ( _SampleMask ) {
        static_assert( sizeof( GLbitfield ) == sizeof( _SampleMask[0] ), "Type Sizeof check" );
        if ( _SampleMask[0] != state->SampleMask[0] ) { glSampleMaski( 0, _SampleMask[0] ); state->SampleMask[0] = _SampleMask[0]; }
        if ( _SampleMask[1] != state->SampleMask[1] ) { glSampleMaski( 1, _SampleMask[1] ); state->SampleMask[1] = _SampleMask[1]; }
        if ( _SampleMask[2] != state->SampleMask[2] ) { glSampleMaski( 2, _SampleMask[2] ); state->SampleMask[2] = _SampleMask[2]; }
        if ( _SampleMask[3] != state->SampleMask[3] ) { glSampleMaski( 3, _SampleMask[3] ); state->SampleMask[3] = _SampleMask[3]; }

        if ( !state->bSampleMaskEnabled ) {
            glEnable( GL_SAMPLE_MASK );
            state->bSampleMaskEnabled = true;
        }
    } else {
        if ( state->bSampleMaskEnabled ) {
            glDisable( GL_SAMPLE_MASK );
            state->bSampleMaskEnabled = false;
        }
    }
}

void CommandBuffer::DynamicState_StencilRef( uint32_t _StencilRef ) {
    State * state = GetCurrentState();
    Pipeline * pipeline = state->CurrentPipeline;

    assert( pipeline != NULL );

    if ( state->Binding.DepthStencilState == pipeline->DepthStencilState ) {

        if ( state->StencilRef != _StencilRef ) {
            // Update stencil ref

            DepthStencilStateInfo const & desc = *pipeline->DepthStencilState;

            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                               _StencilRef,
                               desc.StencilReadMask );
            } else {
                glStencilFuncSeparate( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                                       ComparisonFuncLUT[ desc.BackFace.StencilFunc ],
                                       _StencilRef,
                                       desc.StencilReadMask );
            }

            state->StencilRef = _StencilRef;
        }
    }
}

void CommandBuffer::SetLineWidth( float _Width ) {
    glLineWidth( _Width );
}

void CommandBuffer::CopyBuffer( Buffer & _SrcBuffer, Buffer & _DstBuffer ) {
    size_t byteLength = _SrcBuffer.CreateInfo.SizeInBytes;
    assert( byteLength == _DstBuffer.CreateInfo.SizeInBytes );

    glCopyNamedBufferSubData( GL_HANDLE( _SrcBuffer.GetHandle() ),
                              GL_HANDLE( _DstBuffer.GetHandle() ),
                              0,
                              0,
                              byteLength ); // 4.5 or GL_ARB_direct_state_access
}

void CommandBuffer::CopyBufferRange( Buffer & _SrcBuffer, Buffer & _DstBuffer, uint32_t _NumRanges, BufferCopy const * _Ranges ) {
    for ( BufferCopy const * range = _Ranges ; range < &_Ranges[_NumRanges] ; range++ ) {
        glCopyNamedBufferSubData( GL_HANDLE( _SrcBuffer.GetHandle() ),
                                  GL_HANDLE( _DstBuffer.GetHandle() ),
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
bool CommandBuffer::CopyBufferToTexture1D( Buffer & _SrcBuffer,
                                           Texture & _DstTexture,
                                           uint16_t _Lod,
                                           uint16_t _OffsetX,
                                           uint16_t _DimensionX,
                                           size_t _CompressedDataByteLength, // Only for compressed images
                                           BUFFER_DATA_TYPE _DataType,
                                           size_t _SourceByteOffset,
                                           unsigned int _Alignment ) {
    State * state = GetCurrentState();

    if ( _DstTexture.GetType() != TEXTURE_1D ) {
        return false;
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, GL_HANDLE( _SrcBuffer.GetHandle() ) );

    // TODO: check this

    GLuint textureId = GL_HANDLE( _DstTexture.GetHandle() );
    int isCompressed;

    glGetTextureLevelParameteriv( textureId, _Lod, GL_TEXTURE_COMPRESSED, &isCompressed );

    state->UnpackAlignment( _Alignment );

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    if ( isCompressed ) {
        glCompressedTextureSubImage1D( textureId,
                                       _Lod,
                                       _OffsetX,
                                       _DimensionX,
                                       type->Format,
                                       (GLsizei)_CompressedDataByteLength,
                                       ((uint8_t *)0) + _SourceByteOffset );
    } else {
        glTextureSubImage1D( textureId,
                             _Lod,
                             _OffsetX,
                             _DimensionX,
                             type->Format,
                             type->BaseType,
                             ((uint8_t *)0) + _SourceByteOffset );
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

    return true;
}

// Only for TEXTURE_2D, TEXTURE_1D_ARRAY, TEXTURE_CUBE_MAP
bool CommandBuffer::CopyBufferToTexture2D( Buffer & _SrcBuffer,
                                           Texture & _DstTexture,
                                           uint16_t _Lod,
                                           uint16_t _OffsetX,
                                           uint16_t _OffsetY,
                                           uint16_t _DimensionX,
                                           uint16_t _DimensionY,
                                           uint16_t _CubeFaceIndex, // only for TEXTURE_CUBE_MAP
                                           uint16_t _NumCubeFaces, // only for TEXTURE_CUBE_MAP
                                           size_t _CompressedDataByteLength, // Only for compressed images
                                           BUFFER_DATA_TYPE _DataType,
                                           size_t _SourceByteOffset,
                                           unsigned int _Alignment ) {
    State * state = GetCurrentState();

    if ( _DstTexture.GetType() != TEXTURE_2D && _DstTexture.GetType() != TEXTURE_1D_ARRAY && _DstTexture.GetType() != TEXTURE_CUBE_MAP ) {
        return false;
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, GL_HANDLE( _SrcBuffer.GetHandle() ) );

    // TODO: check this

    GLuint textureId = GL_HANDLE( _DstTexture.GetHandle() );
    int isCompressed;

    glGetTextureLevelParameteriv( textureId, _Lod, GL_TEXTURE_COMPRESSED, &isCompressed );

    state->UnpackAlignment( _Alignment );

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    if ( _DstTexture.GetType() == TEXTURE_CUBE_MAP ) {

        GLint i;
        GLuint currentBinding;

        glGetIntegerv( GL_TEXTURE_BINDING_CUBE_MAP, &i );

        currentBinding = i;

        if ( currentBinding != textureId ) {
            glBindTexture( GL_TEXTURE_CUBE_MAP, textureId );
        }

        // TODO: Учитывать _NumCubeFaces!!!

        if ( isCompressed ) {
            glCompressedTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + _CubeFaceIndex,
                                       _Lod,
                                       _OffsetX,
                                       _OffsetY,
                                       _DimensionX,
                                       _DimensionY,
                                       type->Format,
                                       (GLsizei)_CompressedDataByteLength,
                                       ((uint8_t *)0) + _SourceByteOffset );
        } else {
            glTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + _CubeFaceIndex,
                             _Lod,
                             _OffsetX,
                             _OffsetY,
                             _DimensionX,
                             _DimensionY,
                             type->Format,
                             type->BaseType,
                             ((uint8_t *)0) + _SourceByteOffset );
        }

        if ( currentBinding != textureId ) {
            glBindTexture( GL_TEXTURE_CUBE_MAP, currentBinding );
        }
    } else {
        if ( isCompressed ) {
            glCompressedTextureSubImage2D( textureId,
                                           _Lod,
                                           _OffsetX,
                                           _OffsetY,
                                           _DimensionX,
                                           _DimensionY,
                                           type->Format,
                                           (GLsizei)_CompressedDataByteLength,
                                           ((uint8_t *)0) + _SourceByteOffset );
        } else {
            glTextureSubImage2D( textureId,
                                 _Lod,
                                 _OffsetX,
                                 _OffsetY,
                                 _DimensionX,
                                 _DimensionY,
                                 type->Format,
                                 type->BaseType,
                                 ((uint8_t *)0) + _SourceByteOffset );
        }
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

    return true;
}

// Only for TEXTURE_3D, TEXTURE_2D_ARRAY
bool CommandBuffer::CopyBufferToTexture3D( Buffer & _SrcBuffer,
                                           Texture & _DstTexture,
                                           uint16_t _Lod,
                                           uint16_t _OffsetX,
                                           uint16_t _OffsetY,
                                           uint16_t _OffsetZ,
                                           uint16_t _DimensionX,
                                           uint16_t _DimensionY,
                                           uint16_t _DimensionZ,
                                           size_t _CompressedDataByteLength, // Only for compressed images
                                           BUFFER_DATA_TYPE _DataType,
                                           size_t _SourceByteOffset,
                                           unsigned int _Alignment ) {
    State * state = GetCurrentState();

    if ( _DstTexture.GetType() != TEXTURE_3D && _DstTexture.GetType() != TEXTURE_2D_ARRAY ) {
        return false;
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, GL_HANDLE( _SrcBuffer.GetHandle() ) );

    // TODO: check this

    GLuint textureId = GL_HANDLE( _DstTexture.GetHandle() );
    int isCompressed;

    glGetTextureLevelParameteriv( textureId, _Lod, GL_TEXTURE_COMPRESSED, &isCompressed );

    state->UnpackAlignment( _Alignment );

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    if ( isCompressed ) {
        glCompressedTextureSubImage3D( textureId,
                                       _Lod,
                                       _OffsetX,
                                       _OffsetY,
                                       _OffsetZ,
                                       _DimensionX,
                                       _DimensionY,
                                       _DimensionZ,
                                       type->Format,
                                       (GLsizei)_CompressedDataByteLength,
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
                             type->Format,
                             type->BaseType,
                             ((uint8_t *)0) + _SourceByteOffset );
    }

    glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

    return true;
}

// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP
bool CommandBuffer::CopyBufferToTexture( Buffer & _SrcBuffer,
                                         Texture & _DstTexture,
                                         TextureRect const & _Rectangle,
                                         BUFFER_DATA_TYPE _DataType,
                                         size_t _CompressedDataByteLength,       // for compressed images
                                         size_t _SourceByteOffset,
                                         unsigned int _Alignment ) {

    // FIXME: what about multisample textures?

    switch ( _DstTexture.GetType() ) {
    case TEXTURE_1D:
        return CopyBufferToTexture1D( _SrcBuffer,
                                      _DstTexture,
                                      _Rectangle.Offset.Lod,
                                      _Rectangle.Offset.X,
                                      _Rectangle.Dimension.X,
                                      _CompressedDataByteLength,
                                      _DataType,
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
                                      _CompressedDataByteLength,
                                      _DataType,
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
                                      _CompressedDataByteLength,
                                      _DataType,
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
                                      _CompressedDataByteLength,
                                      _DataType,
                                      _SourceByteOffset,
                                      _Alignment );
    case TEXTURE_CUBE_MAP_ARRAY:
        // FIXME: ???
        //return CopyBufferToTexture3D( _SrcBuffer,
        //                              _DstTexture,
        //                              _Rectangle.Offset.Lod,
        //                              _Rectangle.Offset.X,
        //                              _Rectangle.Offset.Y,
        //                              _Rectangle.Offset.Z,
        //                              _Rectangle.Dimension.X,
        //                              _Rectangle.Dimension.Y,
        //                              _Rectangle.Dimension.Z,
        //                              _CompressedDataByteLength,
        //                              _DataType,
        //                              _SourceByteOffset );
        return false;
    case TEXTURE_RECT:
        // FIXME: ???
        return false;
    default:
        break;
    }

    return false;
}

void CommandBuffer::CopyTextureToBuffer( Texture & _SrcTexture,
                                         Buffer & _DstBuffer,
                                         TextureRect const & _Rectangle,
                                         BUFFER_DATA_TYPE _DataType,
                                         size_t _SizeInBytes,
                                         size_t _DstByteOffset,
                                         unsigned int _Alignment ) {
    State * state = GetCurrentState();

    glBindBuffer( GL_PIXEL_PACK_BUFFER, GL_HANDLE( _DstBuffer.GetHandle() ) );

    // TODO: check this

    GLuint textureId = GL_HANDLE( _SrcTexture.GetHandle() );
    int isCompressed;

    glGetTextureLevelParameteriv( textureId, _Rectangle.Offset.Lod, GL_TEXTURE_COMPRESSED, &isCompressed );

    state->PackAlignment( _Alignment );

    if ( isCompressed ) {
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
        TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

        glGetTextureSubImage( textureId,
                              _Rectangle.Offset.Lod,
                              _Rectangle.Offset.X,
                              _Rectangle.Offset.Y,
                              _Rectangle.Offset.Z,
                              _Rectangle.Dimension.X,
                              _Rectangle.Dimension.Y,
                              _Rectangle.Dimension.Z,
                              type->Format,
                              type->BaseType,
                              (GLsizei)_SizeInBytes,
                              ((uint8_t *)0) + _DstByteOffset );
    }

    glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
}

void CommandBuffer::CopyTextureRect( Texture & _SrcTexture,
                                     Texture & _DstTexture,
                                     uint32_t _NumCopies,
                                     TextureCopy const * Copies ) {
    // TODO: check this

    const GLenum srcTarget = TextureTargetLUT[ _SrcTexture.GetType() ].Target;
    const GLenum dstTarget = TextureTargetLUT[ _DstTexture.GetType() ].Target;
    const GLuint srcId = GL_HANDLE( _SrcTexture.GetHandle() );
    const GLuint dstId = GL_HANDLE( _DstTexture.GetHandle() );

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

bool CommandBuffer::CopyFramebufferToTexture( Framebuffer & _SrcFramebuffer,
                                              Texture & _DstTexture,
                                              FRAMEBUFFER_ATTACHMENT _Attachment,
                                              TextureOffset const & _Offset,
                                              Rect2D const & _SrcRect,
                                              unsigned int _Alignment ) {            // Specifies alignment of destination data
    State * state = GetCurrentState();

    if ( !_SrcFramebuffer.ChooseReadBuffer( _Attachment ) ) {
        LogPrintf( "CommandBuffer::CopyFramebufferToTexture: invalid framebuffer attachment\n" );
        return false;
    }

    state->PackAlignment( _Alignment );

    //BindingStackPointer_t binding;

    _SrcFramebuffer.BindReadFramebuffer();

    // TODO: check this function

    switch ( _DstTexture.GetType() ) {
    case TEXTURE_1D:
    {
        glCopyTextureSubImage1D( GL_HANDLE( _DstTexture.GetHandle() ),
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
        glCopyTextureSubImage2D( GL_HANDLE( _DstTexture.GetHandle() ),
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
        glCopyTextureSubImage3D( GL_HANDLE( _DstTexture.GetHandle() ),
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
        GLint id = GL_HANDLE( _DstTexture.GetHandle() );

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
    case TEXTURE_RECT:
    {
        glCopyTextureSubImage2D( GL_HANDLE( _DstTexture.GetHandle() ),
                                 0,
                                 _Offset.X,
                                 _Offset.Y,
                                 _SrcRect.X,
                                 _SrcRect.Y,
                                 _SrcRect.Width,
                                 _SrcRect.Height );
        break;
    }
    case TEXTURE_2D_MULTISAMPLE:
    case TEXTURE_2D_ARRAY_MULTISAMPLE:
    case TEXTURE_CUBE_MAP_ARRAY:
    default:
        // FIXME: в спецификации про эти форматы ничего не сказано
        return false;
    }

    return true;
}

void CommandBuffer::CopyFramebufferToBuffer( Framebuffer & _SrcFramebuffer,
                                             Buffer & _DstBuffer,
                                             FRAMEBUFFER_ATTACHMENT _Attachment,
                                             Rect2D const & _SrcRect,
                                             FRAMEBUFFER_CHANNEL _FramebufferChannel,
                                             FRAMEBUFFER_OUTPUT _FramebufferOutput,
                                             COLOR_CLAMP _ColorClamp,
                                             size_t _SizeInBytes,
                                             size_t _DstByteOffset,
                                             unsigned int _Alignment ) {
    State * state = GetCurrentState();

    // TODO: check this

    if ( !_SrcFramebuffer.ChooseReadBuffer( _Attachment ) ) {
        LogPrintf( "CommandBuffer::CopyFramebufferToBuffer: invalid framebuffer attachment\n" );
        return;
    }

    _SrcFramebuffer.BindReadFramebuffer();

    state->PackAlignment( _Alignment );

    glBindBuffer( GL_PIXEL_PACK_BUFFER, GL_HANDLE( _DstBuffer.GetHandle() ) );

    state->ClampReadColor( _ColorClamp );

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

bool CommandBuffer::BlitFramebuffer( Framebuffer & _SrcFramebuffer,
                                     FRAMEBUFFER_ATTACHMENT _SrcAttachment,
                                     uint32_t _NumRectangles,
                                     BlitRectangle const * _Rectangles,
                                     FRAMEBUFFER_MASK _Mask,
                                     bool _LinearFilter ) {
    GLbitfield mask = 0;

    if ( _Mask & FB_MASK_COLOR ) {
        mask |= GL_COLOR_BUFFER_BIT;

        if ( !_SrcFramebuffer.ChooseReadBuffer( _SrcAttachment ) ) {
            LogPrintf( "CommandBuffer::BlitFramebuffer: invalid framebuffer attachment\n" );
            return false;
        }
    }

    if ( _Mask & FB_MASK_DEPTH ) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if ( _Mask & FB_MASK_STENCIL ) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    _SrcFramebuffer.BindReadFramebuffer();

    GLenum filter = _LinearFilter ? GL_LINEAR : GL_NEAREST;

    for ( BlitRectangle const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
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

void CommandBuffer::ClearBuffer( Buffer & _Buffer, BUFFER_DATA_TYPE _DataType, const void * _ClearValue ) {
    State * state = GetCurrentState();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( state->RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    glClearNamedBufferData( GL_HANDLE( _Buffer.GetHandle() ), type->InternalFormat, type->Format, type->BaseType, _ClearValue ); // 4.5 or GL_ARB_direct_state_access

    if ( state->RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }

    // It can be also replaced by glClearBufferData
}

void CommandBuffer::ClearBufferRange( Buffer & _Buffer, BUFFER_DATA_TYPE _DataType, uint32_t _NumRanges, BufferClear const * _Ranges, const void * _ClearValue ) {
    State * state = GetCurrentState();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( state->RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    for ( BufferClear const * range = _Ranges ; range < &_Ranges[_NumRanges] ; range++ ) {
        glClearNamedBufferSubData( GL_HANDLE( _Buffer.GetHandle() ),
                                   type->InternalFormat,
                                   range->Offset,
                                   range->SizeInBytes,
                                   type->Format,
                                   type->BaseType,
                                   _ClearValue ); // 4.5 or GL_ARB_direct_state_access
    }

    if ( state->RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }

    // It can be also replaced by glClearBufferSubData
}

void CommandBuffer::ClearTexture( Texture & _Texture, uint16_t _Lod, TEXTURE_PIXEL_FORMAT _PixelFormat, const void * _ClearValue ) {
    State * state = GetCurrentState();

    GLenum pixelType = TexturePixelFormatLUT[ _PixelFormat ].PixelType;
    bool isCompressed = ( pixelType == 0 );

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( state->RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    if ( isCompressed ) {
        glClearTexImage( GL_HANDLE( _Texture.GetHandle() ),
                     _Lod,
                     GL_RED, // FIXME: or GL_RGBA?
                     GL_UNSIGNED_BYTE,
                     _ClearValue );
    } else {
        glClearTexImage( GL_HANDLE( _Texture.GetHandle() ),
                         _Lod,
                         TexturePixelFormatLUT[ _PixelFormat ].Format,
                         pixelType,
                         _ClearValue );
    }

    if ( state->RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }
}

void CommandBuffer::ClearTextureRect( Texture & _Texture,
                                      uint32_t _NumRectangles,
                                      TextureRect const * _Rectangles,
                                      TEXTURE_PIXEL_FORMAT _PixelFormat,
                                      /* optional */ const void * _ClearValue ) {
    State * state = GetCurrentState();

    GLenum pixelType = TexturePixelFormatLUT[ _PixelFormat ].PixelType;
    bool isCompressed = ( pixelType == 0 );

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if ( state->RasterizerState.bRasterizerDiscard ) {
        glDisable( GL_RASTERIZER_DISCARD );
    }

    if ( isCompressed ) {
        for ( TextureRect const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
            glClearTexSubImage( GL_HANDLE( _Texture.GetHandle() ),
                                rect->Offset.Lod,
                                rect->Offset.X,
                                rect->Offset.Y,
                                rect->Offset.Z,
                                rect->Dimension.X,
                                rect->Dimension.Y,
                                rect->Dimension.Z,
                                GL_RED, // FIXME: or GL_RGBA?
                                GL_UNSIGNED_BYTE,
                                _ClearValue );
        }
    } else {
        for ( TextureRect const * rect = _Rectangles ; rect < &_Rectangles[_NumRectangles] ; rect++ ) {
            glClearTexSubImage( GL_HANDLE( _Texture.GetHandle() ),
                                rect->Offset.Lod,
                                rect->Offset.X,
                                rect->Offset.Y,
                                rect->Offset.Z,
                                rect->Dimension.X,
                                rect->Dimension.Y,
                                rect->Dimension.Z,
                                TexturePixelFormatLUT[ _PixelFormat ].Format,
                                TexturePixelFormatLUT[ _PixelFormat ].PixelType,
                                _ClearValue );
        }
    }

    if ( state->RasterizerState.bRasterizerDiscard ) {
        glEnable( GL_RASTERIZER_DISCARD );
    }
}

void CommandBuffer::ClearFramebufferAttachments( Framebuffer & _Framebuffer,
                                                 /* optional */ unsigned int * _ColorAttachments,
                                                 /* optional */ unsigned int _NumColorAttachments,
                                                 /* optional */ ClearColorValue * _ColorClearValues,
                                                 /* optional */ ClearDepthStencilValue * _DepthStencilClearValue,
                                                 /* optional */ Rect2D const * _Rect ) {
    State * state = GetCurrentState();

    assert( _NumColorAttachments <= _Framebuffer.NumColorAttachments );

    GLuint framebufferId = GL_HANDLE( _Framebuffer.GetHandle() );

    assert( framebufferId );

    bool bScissorEnabled = state->RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = state->RasterizerState.bRasterizerDiscard;
    Rect2D scissorRect;

    // If clear rect was not specified, use renderpass render area
    if ( !_Rect && state->CurrentRenderPass ) {
        _Rect = &state->CurrentRenderPassRenderArea;
    }

    if ( _Rect ) {
        if ( !bScissorEnabled ) {
            glEnable( GL_SCISSOR_TEST );
            bScissorEnabled = true;
        }

        // Save current scissor rectangle
        scissorRect = state->CurrentScissor;

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
        for ( unsigned int i = 0 ; i < _NumColorAttachments ; i++ ) {

            unsigned int attachmentIndex = _ColorAttachments[ i ];

            assert( attachmentIndex < _Framebuffer.NumColorAttachments );
            assert( _ColorClearValues );

            FramebufferAttachmentInfo const * framebufferAttachment = &_Framebuffer.ColorAttachments[ attachmentIndex ];

            ClearColorValue const * clearValue = &_ColorClearValues[ i ];

            RenderTargetBlendingInfo const & currentState = state->BlendState.RenderTargetSlots[ attachmentIndex ];
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                glColorMaski( attachmentIndex, 1, 1, 1, 1 );
            }

            // We must set draw buffers to clear attachment :(
            GLenum attachments[ 1 ] = { GL_COLOR_ATTACHMENT0 + i };
            glNamedFramebufferDrawBuffers( framebufferId, 1, attachments );

            // Clear attchment
            switch ( InternalFormatLUT[ framebufferAttachment->pTexture->GetInternalPixelFormat() ].ClearType ) {
            case CLEAR_TYPE_FLOAT32:
                glClearNamedFramebufferfv( framebufferId,
                                           GL_COLOR,
                                           attachmentIndex,
                                           clearValue->Float32 );
                break;
            case CLEAR_TYPE_INT32:
                glClearNamedFramebufferiv( framebufferId,
                                           GL_COLOR,
                                           attachmentIndex,
                                           clearValue->Int32 );
                break;
            case CLEAR_TYPE_UINT32:
                glClearNamedFramebufferuiv( framebufferId,
                                            GL_COLOR,
                                            attachmentIndex,
                                            clearValue->UInt32 );
                break;
            default:
                assert( 0 );
            }

            // Restore color mask
            if ( currentState.ColorWriteMask != COLOR_WRITE_RGBA ) {
                if ( currentState.ColorWriteMask == COLOR_WRITE_DISABLED ) {
                    glColorMaski( attachmentIndex, 0, 0, 0, 0 );
                } else {
                    glColorMaski( attachmentIndex,
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_R_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_G_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_B_BIT),
                                  !!(currentState.ColorWriteMask & COLOR_WRITE_A_BIT) );
                }
            }
        }
    }

    if ( _DepthStencilClearValue ) {

        assert( _Framebuffer.bHasDepthStencilAttachment );

        FramebufferAttachmentInfo const * framebufferAttachment = &_Framebuffer.DepthStencilAttachment;

        // TODO: table
        switch ( InternalFormatLUT[ framebufferAttachment->pTexture->GetInternalPixelFormat() ].ClearType ) {
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
            assert( 0 );
        }
    }

    // Restore scissor test
    if ( bScissorEnabled != state->RasterizerState.bScissorEnable ) {
        if ( state->RasterizerState.bScissorEnable ) {
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
    if ( bRasterizerDiscard != state->RasterizerState.bRasterizerDiscard ) {
        if ( state->RasterizerState.bRasterizerDiscard ) {
            glEnable( GL_RASTERIZER_DISCARD );
        } else {
            glDisable( GL_RASTERIZER_DISCARD );
        }
    }
}

}
