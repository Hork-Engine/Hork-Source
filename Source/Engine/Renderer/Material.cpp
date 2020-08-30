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

#include "Material.h"

using namespace RenderCore;

void CreateDepthPassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, bool _AlphaMasking, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GEQUAL;//CMPFUNC_GREATER;

    SBlendingStateInfo & bs = pipelineCI.BS;
    bs.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    static const SVertexAttribInfo vertexAttribsSkinned[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
        {
            "InJointIndices",
            5,              // location
            1,              // buffer input slot
            VAT_UBYTE4,
            VAM_INTEGER,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointIndices )
        },
        {
            "InJointWeights",
            6,              // location
            1,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointWeights )
        }
    };

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        }
    };

    if ( _Skinned ) {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribsSkinned );
        pipelineCI.pVertexAttribs = vertexAttribsSkinned;
    } else {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule, tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    if ( _AlphaMasking ) {
        TRef< IShaderModule > fragmentShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.pFS = fragmentShaderModule;
    }

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateDepthVelocityPassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, bool _AlphaMasking, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GEQUAL;//CMPFUNC_GREATER;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    static const SVertexAttribInfo vertexAttribsSkinned[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
        {
            "InJointIndices",
            5,              // location
            1,              // buffer input slot
            VAT_UBYTE4,
            VAM_INTEGER,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointIndices )
        },
        {
            "InJointWeights",
            6,              // location
            1,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointWeights )
        }
    };

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        }
    };

    if ( _Skinned ) {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribsSkinned );
        pipelineCI.pVertexAttribs = vertexAttribsSkinned;
    } else {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
    GShaderSources.Add( "#define DEPTH_WITH_VELOCITY_MAP\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    pipelineCI.pVS = vertexShaderModule;

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule, tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
        GShaderSources.Add( "#define DEPTH_WITH_VELOCITY_MAP\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
        GShaderSources.Add( "#define DEPTH_WITH_VELOCITY_MAP\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    TRef< IShaderModule > fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_DEPTH\n" );
    GShaderSources.Add( "#define DEPTH_WITH_VELOCITY_MAP\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    pipelineCI.pFS = fragmentShaderModule;

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateWireframePassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SBlendingStateInfo & bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if ( _Skinned ) {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            },
            {
                "InJointIndices",
                5,              // location
                1,              // buffer input slot
                VAT_UBYTE4,
                VAM_INTEGER,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointIndices )
            },
            {
                "InJointWeights",
                6,              // location
                1,              // buffer input slot
                VAT_UBYTE4N,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointWeights )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    } else {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule, tessControlShaderModule, tessEvalShaderModule, geometryShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    if ( _Tessellation )
    {
        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_WIREFRAME\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;        
    }

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pGS = geometryShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateNormalsPassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, bool _Skinned ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.bScissorEnable = SCISSOR_TEST;

    SBlendingStateInfo & bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    if ( _Skinned ) {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            },
            {
                "InJointIndices",
                5,              // location
                1,              // buffer input slot
                VAT_UBYTE4,
                VAM_INTEGER,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointIndices )
            },
            {
                "InJointWeights",
                6,              // location
                1,              // buffer input slot
                VAT_UBYTE4N,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointWeights )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    } else {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule, geometryShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_NORMALS\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_NORMALS\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_NORMALS\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_POINTS;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pGS = geometryShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateHUDPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_DISABLED;
    rsd.bScissorEnable = true;

    SBlendingStateInfo & bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, TexCoord )
        },
        {
            "InColor",
            2,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, Color )
        }
    };


    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

static RenderCore::BLENDING_PRESET GetBlendingPreset( EColorBlending _Blending ) {
    switch ( _Blending ) {
    case COLOR_BLENDING_ALPHA:
        return RenderCore::BLENDING_ALPHA;
    case COLOR_BLENDING_DISABLED:
        return RenderCore::BLENDING_NO_BLEND;
    case COLOR_BLENDING_PREMULTIPLIED_ALPHA:
        return RenderCore::BLENDING_PREMULTIPLIED_ALPHA;
    case COLOR_BLENDING_COLOR_ADD:
        return RenderCore::BLENDING_COLOR_ADD;
    case COLOR_BLENDING_MULTIPLY:
        return RenderCore::BLENDING_MULTIPLY;
    case COLOR_BLENDING_SOURCE_TO_DEST:
        return RenderCore::BLENDING_SOURCE_TO_DEST;
    case COLOR_BLENDING_ADD_MUL:
        return RenderCore::BLENDING_ADD_MUL;
    case COLOR_BLENDING_ADD_ALPHA:
        return RenderCore::BLENDING_ADD_ALPHA;
    default:
        AN_ASSERT( 0 );
    }
    return RenderCore::BLENDING_NO_BLEND;
}

void CreateLightPassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _DepthTest, bool _Translucent, EColorBlending _Blending, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SBlendingStateInfo & bsd = pipelineCI.BS;
    if ( _Translucent ) {        
        bsd.RenderTargetSlots[0].SetBlendingPreset( GetBlendingPreset( _Blending ) );
    }

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;

#ifdef DEPTH_PREPASS
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
    if ( _Translucent ) {
        dssd.DepthFunc = CMPFUNC_GREATER;
    } else {
        dssd.DepthFunc = CMPFUNC_EQUAL;
    }
#else
    dssd.DepthFunc = CMPFUNC_GREATER;
#endif
    dssd.bDepthEnable = _DepthTest;

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    if ( _Skinned ) {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            },
            {
                "InJointIndices",
                5,              // location
                1,              // buffer input slot
                VAT_UBYTE4,
                VAM_INTEGER,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointIndices )
            },
            {
                "InJointWeights",
                6,              // location
                1,              // buffer input slot
                VAT_UBYTE4N,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointWeights )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;

        AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define SKINNED_MESH\n" );
        GShaderSources.Add( vertexAttribsShaderString.CStr() );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define SKINNED_MESH\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.NumVertexBindings = 2;
    } else {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;

        AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( vertexAttribsShaderString.CStr() );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.NumVertexBindings = 1;
    }

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule, tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateLightPassLightmapPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _DepthTest, bool _Translucent, EColorBlending _Blending, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SBlendingStateInfo & bsd = pipelineCI.BS;
    if ( _Translucent ) {
        bsd.RenderTargetSlots[0].SetBlendingPreset( GetBlendingPreset( _Blending ) );
    }

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;

#ifdef DEPTH_PREPASS
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
    if ( _Translucent ) {
        dssd.DepthFunc = CMPFUNC_GREATER;
    } else {
        dssd.DepthFunc = CMPFUNC_EQUAL;
    }
#else
    dssd.DepthFunc = CMPFUNC_GREATER;
#endif
    dssd.bDepthEnable = _DepthTest;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
        {
            "InLightmapTexCoord",
            5,              // location
            1,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexUV, TexCoord )
        }
    };

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_LIGHTMAP\n" );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_LIGHTMAP\n" );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule, tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define USE_LIGHTMAP\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define USE_LIGHTMAP\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexUV );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateLightPassVertexLightPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _DepthTest, bool _Translucent, EColorBlending _Blending, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SBlendingStateInfo & bsd = pipelineCI.BS;
    if ( _Translucent ) {
        bsd.RenderTargetSlots[0].SetBlendingPreset( GetBlendingPreset( _Blending ) );
    }

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;

#ifdef DEPTH_PREPASS
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
    if ( _Translucent ) {
        dssd.DepthFunc = CMPFUNC_GREATER;
    } else {
        dssd.DepthFunc = CMPFUNC_EQUAL;
    }
#else
    dssd.DepthFunc = CMPFUNC_GREATER;
#endif
    dssd.bDepthEnable = _DepthTest;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
        {
            "InVertexLight",
            5,              // location
            1,              // buffer input slot
            VAT_UBYTE4,//VAT_UBYTE4N,
            VAM_INTEGER,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexLight, VertexLight )
        }
    };

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_VERTEX_LIGHT\n" );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
    GShaderSources.Add( "#define USE_VERTEX_LIGHT\n" );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule, tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define USE_VERTEX_LIGHT\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_COLOR\n" );
        GShaderSources.Add( "#define USE_VERTEX_LIGHT\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexLight );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateShadowMapPassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, bool _ShadowMasking, bool _TwoSided, bool _Skinned, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.bScissorEnable = SCISSOR_TEST;
#if defined SHADOWMAP_VSM
    //Desc.CullMode = POLYGON_CULL_FRONT; // Less light bleeding
    Desc.CullMode = POLYGON_CULL_DISABLED;
#else
    //rsd.CullMode = POLYGON_CULL_BACK;
    //rsd.CullMode = POLYGON_CULL_DISABLED; // Less light bleeding
    rsd.CullMode = _TwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;
#endif
    //rsd.CullMode = POLYGON_CULL_DISABLED;

    //BlendingStateInfo & bsd = pipelineCI.BS;
    //bsd.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;  // FIXME: there is no fragment shader, so we realy need to disable color mask?
#if defined SHADOWMAP_VSM
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_NO_BLEND );
#endif

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_LESS;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    static const SVertexAttribInfo vertexAttribsSkinned[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
        {
            "InJointIndices",
            5,              // location
            1,              // buffer input slot
            VAT_UBYTE4,
            VAM_INTEGER,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointIndices )
        },
        {
            "InJointWeights",
            6,              // location
            1,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointWeights )
        }
    };

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
    };

    if ( _Skinned ) {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribsSkinned );
        pipelineCI.pVertexAttribs = vertexAttribsSkinned;
    } else {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule;
    TRef< IShaderModule > geometryShaderModule;
    TRef< IShaderModule > fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_SHADOWMAP\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    pipelineCI.pVS = vertexShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_SHADOWMAP\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShaderModule );

    pipelineCI.pGS = geometryShaderModule;

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule;
        TRef< IShaderModule > tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_SHADOWMAP\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_SHADOWMAP\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if ( _ShadowMasking || bVSM ) {
        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_SHADOWMAP\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.pFS = fragmentShaderModule;
    }

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateFeedbackPassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthWriteMask = DEPTH_WRITE_ENABLE;
    dssd.DepthFunc = CMPFUNC_GREATER;
    dssd.bDepthEnable = true;

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    if ( _Skinned ) {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            },
            {
                "InJointIndices",
                5,              // location
                1,              // buffer input slot
                VAT_UBYTE4,
                VAM_INTEGER,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointIndices )
            },
            {
                "InJointWeights",
                6,              // location
                1,              // buffer input slot
                VAT_UBYTE4N,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertexSkin, JointWeights )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;

        AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_FEEDBACK\n" );
        GShaderSources.Add( "#define SKINNED_MESH\n" );
        GShaderSources.Add( vertexAttribsShaderString.CStr() );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_FEEDBACK\n" );
        GShaderSources.Add( "#define SKINNED_MESH\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.NumVertexBindings = 2;
    } else {
        static const SVertexAttribInfo vertexAttribs[] = {
            {
                "InPosition",
                0,              // location
                0,              // buffer input slot
                VAT_FLOAT3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Position )
            },
            {
                "InTexCoord",
                1,              // location
                0,              // buffer input slot
                VAT_HALF2,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, TexCoord )
            },
            {
                "InNormal",
                2,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Normal )
            },
            {
                "InTangent",
                3,              // location
                0,              // buffer input slot
                VAT_HALF3,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Tangent )
            },
            {
                "InHandedness",
                4,              // location
                0,              // buffer input slot
                VAT_BYTE1,
                VAM_FLOAT,
                0,              // InstanceDataStepRate
                AN_OFS( SMeshVertex, Handedness )
            }
        };

        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;

        AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_FEEDBACK\n" );
        GShaderSources.Add( vertexAttribsShaderString.CStr() );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_FEEDBACK\n" );
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.NumVertexBindings = 1;
    }

    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateOutlinePassPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * _SourceCode, RenderCore::POLYGON_CULL _CullMode, bool _Skinned, bool _Tessellation ) {
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = _CullMode;
    rsd.bScissorEnable = SCISSOR_TEST;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
#if 0
    dssd.DepthFunc = CMPFUNC_GEQUAL;//CMPFUNC_GREATER;
#else
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
#endif

    //SBlendingStateInfo & bs = pipelineCI.BS;
    //bs.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

    SVertexBindingInfo vertexBinding[2] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SMeshVertex );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    vertexBinding[1].InputSlot = 1;
    vertexBinding[1].Stride = sizeof( SMeshVertexSkin );
    vertexBinding[1].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = _Skinned ? 2 : 1;
    pipelineCI.pVertexBindings = vertexBinding;

    static const SVertexAttribInfo vertexAttribsSkinned[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        },
        {
            "InJointIndices",
            5,              // location
            1,              // buffer input slot
            VAT_UBYTE4,
            VAM_INTEGER,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointIndices )
        },
        {
            "InJointWeights",
            6,              // location
            1,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertexSkin, JointWeights )
        }
    };

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_HALF2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, TexCoord )
        },
        {
            "InNormal",
            2,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Normal )
        },
        {
            "InTangent",
            3,              // location
            0,              // buffer input slot
            VAT_HALF3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Tangent )
        },
        {
            "InHandedness",
            4,              // location
            0,              // buffer input slot
            VAT_BYTE1,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SMeshVertex, Handedness )
        }
    };

    if ( _Skinned ) {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribsSkinned );
        pipelineCI.pVertexAttribs = vertexAttribsSkinned;
    } else {
        pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
        pipelineCI.pVertexAttribs = vertexAttribs;
    }

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_OUTLINE\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    if ( _Tessellation )
    {
        TRef< IShaderModule > tessControlShaderModule, tessEvalShaderModule;

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_OUTLINE\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_CONTROL_SHADER, tessControlShaderModule );

        GShaderSources.Clear();
        GShaderSources.Add( "#define MATERIAL_PASS_OUTLINE\n" );
        if ( _Skinned ) {
            GShaderSources.Add( "#define SKINNED_MESH\n" );
        }
        GShaderSources.Add( _SourceCode );
        GShaderSources.Build( TESS_EVALUATION_SHADER, tessEvalShaderModule );

        pipelineCI.pTCS = tessControlShaderModule;
        pipelineCI.pTES = tessEvalShaderModule;
    }

    TRef< IShaderModule > fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( "#define MATERIAL_PASS_OUTLINE\n" );
    if ( _Skinned ) {
        GShaderSources.Add( "#define SKINNED_MESH\n" );
    }
    GShaderSources.Add( _SourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    pipelineCI.pFS = fragmentShaderModule;

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = _Tessellation ? PRIMITIVE_PATCHES_3 : PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}
