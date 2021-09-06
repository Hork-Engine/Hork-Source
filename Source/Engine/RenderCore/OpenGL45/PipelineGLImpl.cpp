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

#include "PipelineGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "ShaderModuleGLImpl.h"
#include "VertexArrayObjectGL.h"
#include "LUT.h"
#include "GL/glew.h"

namespace RenderCore {

APipelineGLImpl::APipelineGLImpl(ADeviceGLImpl* pDevice, SPipelineDesc const& Desc) :
    IPipeline(pDevice)
{
    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    GLuint pipelineId;

    if ( !pDevice->IsFeatureSupported( FEATURE_HALF_FLOAT_VERTEX ) ) {
        // Check half float vertex type

        for (SVertexAttribInfo const* desc = Desc.pVertexAttribs; desc < &Desc.pVertexAttribs[Desc.NumVertexAttribs]; desc++)
        {
            if ( desc->TypeOfComponent() == COMPONENT_HALF ) {
                GLogger.Printf( "APipelineGLImpl::ctor: Half floats not supported by current hardware\n" );
            }
        }
    }

    // TODO: create program pipeline for each context
    glCreateProgramPipelines( 1, &pipelineId );

    pVS  = Desc.pVS;
    pTCS = Desc.pTCS;
    pTES = Desc.pTES;
    pGS  = Desc.pGS;
    pFS  = Desc.pFS;
    pCS  = Desc.pCS;

    if ( pVS ) {
        glUseProgramStages( pipelineId, GL_VERTEX_SHADER_BIT, pVS->GetHandleNativeGL() );
    }
    if ( pTCS ) {
        glUseProgramStages( pipelineId, GL_TESS_CONTROL_SHADER_BIT, pTCS->GetHandleNativeGL() );
    }
    if ( pTES ) {
        glUseProgramStages( pipelineId, GL_TESS_EVALUATION_SHADER_BIT, pTES->GetHandleNativeGL() );
    }
    if ( pGS ) {
        glUseProgramStages( pipelineId, GL_GEOMETRY_SHADER_BIT, pGS->GetHandleNativeGL() );
    }
    if ( pFS ) {
        glUseProgramStages( pipelineId, GL_FRAGMENT_SHADER_BIT, pFS->GetHandleNativeGL() );
    }
    if ( pCS ) {
        glUseProgramStages( pipelineId, GL_COMPUTE_SHADER_BIT, pCS->GetHandleNativeGL() );
    }

    glValidateProgramPipeline( pipelineId ); // 4.1

    SetHandleNativeGL( pipelineId );

    PrimitiveTopology = GL_TRIANGLES; // Use triangles by default

    if (Desc.IA.Topology <= PRIMITIVE_TRIANGLE_STRIP_ADJ)
    {
        PrimitiveTopology = PrimitiveTopologyLUT[Desc.IA.Topology];
        NumPatchVertices = 0;
    }
    else if (Desc.IA.Topology >= PRIMITIVE_PATCHES_1)
    {
        PrimitiveTopology = GL_PATCHES;
        NumPatchVertices  = Desc.IA.Topology - PRIMITIVE_PATCHES_1 + 1; // Must be < GL_MAX_PATCH_VERTICES

        if ( NumPatchVertices > pDevice->GetDeviceCaps( DEVICE_CAPS_MAX_PATCH_VERTICES ) ) {
            GLogger.Printf( "APipelineGLImpl::ctor: num patch vertices > DEVICE_CAPS_MAX_PATCH_VERTICES\n" );
        }
    }

    bPrimitiveRestartEnabled = Desc.IA.bPrimitiveRestart;

    // Cache VAO for each context
    VAO = ctx->CachedVAO(Desc.pVertexBindings, Desc.NumVertexBindings,
                         Desc.pVertexAttribs, Desc.NumVertexAttribs);

    BlendingState     = pDevice->CachedBlendingState(Desc.BS);
    RasterizerState   = pDevice->CachedRasterizerState(Desc.RS);
    DepthStencilState = pDevice->CachedDepthStencilState(Desc.DSS);

    SAllocatorCallback const & allocator = pDevice->GetAllocator();

    NumSamplerObjects = Desc.ResourceLayout.NumSamplers;
    if ( NumSamplerObjects > 0 ) {
        SamplerObjects = (unsigned int *)allocator.Allocate( sizeof( SamplerObjects[0] ) * NumSamplerObjects );
        for ( int i = 0 ; i < NumSamplerObjects ; i++ ) {
            SamplerObjects[i] = pDevice->CachedSampler(Desc.ResourceLayout.Samplers[i]);
        }
    }
    else {
        SamplerObjects = nullptr;
    }

    NumImages = Desc.ResourceLayout.NumImages;
    if ( NumImages > 0 ) {
        Images = (SImageInfoGL *)allocator.Allocate( sizeof( SImageInfoGL ) * NumImages );
        for ( int i = 0 ; i < NumImages ; i++ ) {
            Images[i].AccessMode     = ImageAccessModeLUT[Desc.ResourceLayout.Images[i].AccessMode];
            Images[i].InternalFormat = InternalFormatLUT[Desc.ResourceLayout.Images[i].TextureFormat].InternalFormat;
        }
    }
    else {
        Images = nullptr;
    }

    NumBuffers = Desc.ResourceLayout.NumBuffers;
    if ( NumBuffers > 0 ) {
        Buffers = (SBufferInfoGL *)allocator.Allocate( sizeof( SBufferInfoGL ) * NumBuffers );
        for ( int i = 0 ; i < NumBuffers ; i++ ) {
            Buffers[i].BufferType = BufferTargetLUT[Desc.ResourceLayout.Buffers[i].BufferBinding].Target;
        }
    }
    else {
        Buffers = nullptr;
    }
}

APipelineGLImpl::~APipelineGLImpl() {
    GLuint pipelineId = GetHandleNativeGL();
    if ( pipelineId ) {
        glDeleteProgramPipelines( 1, &pipelineId );
    }

    SAllocatorCallback const & allocator = GetDevice()->GetAllocator();

    if ( SamplerObjects ) {
        allocator.Deallocate( SamplerObjects );
    }

    if ( Images ) {
        allocator.Deallocate( Images );
    }

    if ( Buffers ) {
        allocator.Deallocate( Buffers );
    }
}

void SRenderTargetBlendingInfo::SetBlendingPreset( BLENDING_PRESET _Preset ) {
    switch ( _Preset ) {
    case BLENDING_ALPHA:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_ALPHA;
        Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_INV_SRC_ALPHA;
        Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
        break;
    case BLENDING_PREMULTIPLIED_ALPHA:
        bBlendEnable = true;
        ColorWriteMask = COLOR_WRITE_RGBA;
        Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_ONE;
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
