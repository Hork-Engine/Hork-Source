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

#include "GHIPipeline.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "GHIShader.h"
#include "GHIVertexArrayObject.h"
#include "LUT.h"

#include <assert.h>
#include <memory.h>
#include <GL/glew.h>

namespace GHI {

Pipeline::Pipeline() {
    pDevice = nullptr;
    Handle = nullptr;
}

Pipeline::~Pipeline() {
    Deinitialize();
}

void Pipeline::Initialize( PipelineCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();
    Device * device = state->pDevice;

    Deinitialize();

    GLuint pipelineId;

    if ( !device->IsHalfFloatVertexSupported() ) {
        // Check half float vertex type

        for ( VertexAttribInfo const * desc = _CreateInfo.pVertexAttribs ; desc < &_CreateInfo.pVertexAttribs[_CreateInfo.NumVertexAttribs] ; desc++ ) {
            if ( desc->TypeOfComponent() == COMPONENT_HALF ) {
                LogPrintf( "Pipeline::Initialize: Half floats not supported by current hardware\n" );
            }
        }
    }

    glCreateProgramPipelines( 1, &pipelineId );

    if ( !pipelineId ) {
        LogPrintf( "Pipeline::Initialize: couldn't create program pipeline\n" );
        //return NULL;
    }

    for ( ShaderStageInfo const * stageDesc = _CreateInfo.pStages ; stageDesc < &_CreateInfo.pStages[ _CreateInfo.NumStages ] ; stageDesc++ ) {
        glUseProgramStages( pipelineId, stageDesc->Stage, GL_HANDLE( stageDesc->pModule->GetHandle() ) );
    }

    glValidateProgramPipeline( pipelineId ); // 4.1

    Handle = ( void * )( size_t )pipelineId;

    IndexBufferType = 0;
    IndexBufferTypeSizeOf = 0;
    IndexBufferOffset = 0;

    PrimitiveTopology = GL_TRIANGLES; // Use triangles by default

    if ( _CreateInfo.pInputAssembly->Topology <= PRIMITIVE_TRIANGLE_STRIP_ADJ ) {
        PrimitiveTopology = PrimitiveTopologyLUT[ _CreateInfo.pInputAssembly->Topology ];
    } else if (  _CreateInfo.pInputAssembly->Topology >= PRIMITIVE_PATCHES_1 ) {
        PrimitiveTopology = GL_PATCHES;
        NumPatchVertices = _CreateInfo.pInputAssembly->Topology - PRIMITIVE_PATCHES_1 + 1;  // Must be < GL_MAX_PATCH_VERTICES
    }

    bPrimitiveRestartEnabled = _CreateInfo.pInputAssembly->bPrimitiveRestart;

    VAO = state->CachedVAO( _CreateInfo.pVertexBindings, _CreateInfo.NumVertexBindings,
                            _CreateInfo.pVertexAttribs, _CreateInfo.NumVertexAttribs );

    BlendingState = device->CachedBlendingState( *_CreateInfo.pBlending );
    RasterizerState = device->CachedRasterizerState( *_CreateInfo.pRasterizer );
    DepthStencilState = device->CachedDepthStencilState( *_CreateInfo.pDepthStencil );

    pRenderPass = _CreateInfo.pRenderPass;
    Subpass = _CreateInfo.Subpass;

    state->TotalPipelines++;

    pDevice = state->GetDevice();
}

void Pipeline::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    State * state = GetCurrentState();

    GLuint pipelineId = GL_HANDLE( Handle );

    glDeleteProgramPipelines( 1, &pipelineId );

    state->TotalPipelines--;

    pDevice = nullptr;
    Handle = nullptr;
}

void RenderTargetBlendingInfo::SetBlendingPreset( BLENDING_PRESET _Preset ) {
    switch ( _Preset ) {
    case BLENDING_ALPHA:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_ALPHA;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_INV_SRC_ALPHA;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_COLOR_ADD:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_ONE;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_MULTIPLY:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_DST_COLOR;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ZERO;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_SOURCE_TO_DEST:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_COLOR;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_ADD_MUL:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_INV_DST_COLOR;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_ADD_ALPHA:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_ALPHA;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_NO_BLEND:
    default:
        bBlendEnable = false;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_ONE;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ZERO;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    }
}

}
