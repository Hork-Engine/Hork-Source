/*

Angie Engine Source Code

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

#include "OpenGL45Material.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45CanvasPassRenderer.h"
#include "OpenGL45DepthPassRenderer.h"
#include "OpenGL45ColorPassRenderer.h"
#include "OpenGL45WireframePassRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45ShaderBuiltin.h"

namespace OpenGL45 {

void FDepthPass::Create( const char * _VertexSourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned ) {
    using namespace GHI;

    PipelineCreateInfo pipelineCI = {};

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    BlendingStateInfo bsd;
    bsd.SetDefaults();
    //bsd.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();
    dssd.DepthFunc = CMPFUNC_GREATER;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( FMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( FMeshVertexJoint );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    static const VertexAttribInfo vertexAttribsSkinned[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, TexCoord )
        },
        {
            "InTangent",
            2,              // location
            0,              // buffer input slot
            VAT_FLOAT4,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Tangent )
        },
        {
            "InNormal",
            3,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Normal )
        },
        {
            "InJointIndices",
            4,              // location
            1,              // buffer input slot
            VAT_UBYTE4,
            VAM_INTEGER,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertexJoint, JointIndices )
        },
        {
            "InJointWeights",
            5,              // location
            1,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertexJoint, JointWeights )
        }
    };

    static const VertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, TexCoord )
        },
        {
            "InTangent",
            2,              // location
            0,              // buffer input slot
            VAT_FLOAT4,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Tangent )
        },
        {
            "InNormal",
            3,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Normal )
        }
    };

    if ( _Skinned ) {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribsSkinned );
        pipelineCI.pVertexAttribs = vertexAttribsSkinned;
    } else {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    ShaderModule vertexShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
    GShaderSources.Add( _VertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo stages[1] = { vs };

    pipelineCI.NumStages = 1;
    pipelineCI.pStages = stages;

    pipelineCI.pRenderPass = GDepthPassRenderer.GetRenderPass();
    pipelineCI.Subpass = 0;

    Initialize( pipelineCI );
}

void FWireframePass::Create( const char * _VertexSourceCode, const char * _GeometrySourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned ) {
    using namespace GHI;

    PipelineCreateInfo pipelineCI = {};

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    BlendingStateInfo bsd;
    bsd.SetDefaults();
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();

    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( FMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( FMeshVertexJoint );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if ( _Skinned ) {
        static const VertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_FLOAT2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, TexCoord )
            },
            {
                "InTangent",
                2,              // location
                0,              // buffer input slot
                VAT_FLOAT4,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Tangent )
            },
            {
                "InNormal",
                3,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Normal )
            },
            {
                "InJointIndices",
                4,              // location
                1,              // buffer input slot
                VAT_UBYTE4,
                VAM_INTEGER,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertexJoint, JointIndices )
            },
            {
                "InJointWeights",
                5,              // location
                1,              // buffer input slot
                VAT_UBYTE4N,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertexJoint, JointWeights )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    } else {
        static const VertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_FLOAT2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, TexCoord )
            },
            {
                "InTangent",
                2,              // location
                0,              // buffer input slot
                VAT_FLOAT4,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Tangent )
            },
            {
                "InNormal",
                3,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Normal )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    ShaderModule vertexShaderModule, geometryShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
    GShaderSources.Add( _VertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _GeometrySourceCode );
    GShaderSources.Build( GEOMETRY_SHADER, &geometryShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _FragmentSourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo gs = {};
    gs.Stage = SHADER_STAGE_GEOMETRY_BIT;
    gs.pModule = &geometryShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[3] = { vs, gs, fs };

    pipelineCI.NumStages = 3;
    pipelineCI.pStages = stages;

    pipelineCI.pRenderPass = GWireframePassRenderer.GetRenderPass();
    pipelineCI.Subpass = 0;

    Initialize( pipelineCI );
}

void FColorPassHUD::Create( const char * _VertexSourceCode, const char * _FragmentSourceCode ) {
    using namespace GHI;

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYFON_CULL_DISABLED;
    rsd.bScissorEnable = true;

    BlendingStateInfo bsd;
    bsd.SetDefaults();
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const VertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FHUDDrawVert, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FHUDDrawVert, TexCoord )
        },
        {
            "InColor",
            2,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FHUDDrawVert, Color )
        }
    };


    FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    ShaderModule vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
    GShaderSources.Add( _VertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( _FragmentSourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[2] = { vs, fs };

    pipelineCI.NumStages = 2;
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( FHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.pRenderPass = GCanvasPassRenderer.GetRenderPass();
    pipelineCI.Subpass = 0;

    Initialize( pipelineCI );
}

void FColorPass::Create( const char * _VertexSourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _Unlit, bool _Skinned, bool _DepthTest ) {
    using namespace GHI;
    PipelineCreateInfo pipelineCI = {};

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    BlendingStateInfo bsd;
    bsd.SetDefaults();

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();

#ifdef DEPTH_PREPASS
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
    dssd.DepthFunc = CMPFUNC_EQUAL;
#else
    dssd.DepthFunc = CMPFUNC_GREATER;
#endif
    dssd.bDepthEnable = _DepthTest;

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderModule vertexShaderModule, fragmentShaderModule;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[2] = { vs, fs };

    pipelineCI.NumStages = 2;
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( FMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( FMeshVertexJoint );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    if ( _Skinned ) {
        static const VertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_FLOAT2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, TexCoord )
            },
            {
                "InTangent",
                2,              // location
                0,              // buffer input slot
                VAT_FLOAT4,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Tangent )
            },
            {
                "InNormal",
                3,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Normal )
            },
            {
                "InJointIndices",
                4,              // location
                1,              // buffer input slot
                VAT_UBYTE4,
                VAM_INTEGER,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertexJoint, JointIndices )
            },
            {
                "InJointWeights",
                5,              // location
                1,              // buffer input slot
                VAT_UBYTE4N,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertexJoint, JointWeights )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;

        FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

        GShaderSources.Clear();
        GShaderSources.Add( "#version 450\n" );
        GShaderSources.Add( UniformStr );
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define SKINNED_MESH\n" );
        if ( _Unlit ) {
            GShaderSources.Add( "#define UNLIT\n" );
        }
        GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
        GShaderSources.Add( _VertexSourceCode );
        GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#version 450\n" );
        GShaderSources.Add( UniformStr );
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define SKINNED_MESH\n" );
        if ( _Unlit ) {
            GShaderSources.Add( "#define UNLIT\n" );
        }
        GShaderSources.Add( _FragmentSourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

        pipelineCI.NumVertexBindings = 2;
    } else {
        static const VertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_FLOAT2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, TexCoord )
            },
            {
                "InTangent",
                2,              // location
                0,              // buffer input slot
                VAT_FLOAT4,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Tangent )
            },
            {
                "InNormal",
                3,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                GHI_STRUCT_OFS( FMeshVertex, Normal )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;

        FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

        GShaderSources.Clear();
        GShaderSources.Add( "#version 450\n" );
        GShaderSources.Add( UniformStr );
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        if ( _Unlit ) {
            GShaderSources.Add( "#define UNLIT\n" );
        }
        GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
        GShaderSources.Add( _VertexSourceCode );
        GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#version 450\n" );
        GShaderSources.Add( UniformStr );
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        if ( _Unlit ) {
            GShaderSources.Add( "#define UNLIT\n" );
        }
        GShaderSources.Add( _FragmentSourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

        pipelineCI.NumVertexBindings = 1;
    }

    pipelineCI.pVertexBindings = vertexBinding;
    pipelineCI.pRenderPass = GColorPassRenderer.GetRenderPass();
    pipelineCI.Subpass = 0;

    Initialize( pipelineCI );
}


void FColorPassLightmap::Create( const char * _VertexSourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _DepthTest ) {
    using namespace GHI;

    PipelineCreateInfo pipelineCI = {};

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    BlendingStateInfo bsd;
    bsd.SetDefaults();

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();

#ifdef DEPTH_PREPASS
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
    dssd.DepthFunc = CMPFUNC_EQUAL;
#else
    dssd.DepthFunc = CMPFUNC_GREATER;
#endif
    dssd.bDepthEnable = _DepthTest;

    static const VertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, TexCoord )
        },
        {
            "InTangent",
            2,              // location
            0,              // buffer input slot
            VAT_FLOAT4,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Tangent )
        },
        {
            "InNormal",
            3,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Normal )
        },
        {
            "InLightmapTexCoord",
            4,              // location
            1,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshLightmapUV, TexCoord )
        }
    };

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    ShaderModule vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_LIGHTMAP\n" );
    GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
    GShaderSources.Add( _VertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_LIGHTMAP\n" );
    GShaderSources.Add( _FragmentSourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[2] = { vs, fs };

    pipelineCI.NumStages = 2;
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( FMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( FMeshLightmapUV );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.pRenderPass = GColorPassRenderer.GetRenderPass();
    pipelineCI.Subpass = 0;

    Initialize( pipelineCI );
}

void FColorPassVertexLight::Create( const char * _VertexSourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _DepthTest ) {
    using namespace GHI;
    PipelineCreateInfo pipelineCI = {};

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    BlendingStateInfo bsd;
    bsd.SetDefaults();

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();

#ifdef DEPTH_PREPASS
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
    dssd.DepthFunc = CMPFUNC_EQUAL;
#else
    dssd.DepthFunc = CMPFUNC_GREATER;
#endif
    dssd.bDepthEnable = _DepthTest;

    static const VertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, TexCoord )
        },
        {
            "InTangent",
            2,              // location
            0,              // buffer input slot
            VAT_FLOAT4,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Tangent )
        },
        {
            "InNormal",
            3,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertex, Normal )
        },
        {
            "InVertexLight",
            4,              // location
            1,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( FMeshVertexLight, VertexLight )
        }
    };

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    FString vertexAttribsShaderString = ShaderStringForVertexAttribs< FString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    ShaderModule vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_VERTEX_LIGHT\n" );
    GShaderSources.Add( vertexAttribsShaderString.ToConstChar() );
    GShaderSources.Add( _VertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#version 450\n" );
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_VERTEX_LIGHT\n" );
    GShaderSources.Add( _FragmentSourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[2] = { vs, fs };

    pipelineCI.NumStages = 2;
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( FMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( FMeshVertexLight );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.pRenderPass = GColorPassRenderer.GetRenderPass();
    pipelineCI.Subpass = 0;

    Initialize( pipelineCI );
}

}
