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

#include "OpenGL45PostprocessPassRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"
#include "OpenGL45RenderBackend.h"

#include <Core/Public/CriticalError.h>

using namespace GHI;

namespace OpenGL45 {

APostprocessPassRenderer GPostprocessPassRenderer;

void APostprocessPassRenderer::Initialize() {
    RenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 1;

    AttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassCI.pColorAttachments = &colorAttachment;
    renderPassCI.pDepthStencilAttachment = NULL;

    AttachmentRef colorAttachmentRef = {};
    colorAttachmentRef.Attachment = 0;

    SubpassInfo subpass = {};
    subpass.NumColorAttachments = 1;
    subpass.pColorAttachmentRefs = &colorAttachmentRef;

    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpass;

    PostprocessPass.Initialize( renderPassCI );

    CreatePipeline();
    CreateSampler();
}

void APostprocessPassRenderer::Deinitialize() {
    PostprocessPass.Deinitialize();
    PostprocessPipeline.Deinitialize();
}

void APostprocessPassRenderer::CreatePipeline() {
    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

    BlendingStateInfo bsd;
    bsd.SetDefaults();

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
            0
        }
    };

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    ShaderModule vertexShaderModule, fragmentShaderModule;

    const char * vertexSourceCode = R"(
        out gl_PerVertex
        {
            vec4 gl_Position;
        };
        layout( location = 0 ) noperspective out vec4 VS_TexCoord;
        layout( location = 1 ) flat out float VS_Exposure;
        layout( binding = 6 ) uniform sampler2D luminanceTexture;

        void main() {
          gl_Position = vec4( InPosition, 0.0, 1.0 );
          VS_TexCoord.xy = InPosition * 0.5 + 0.5;
          VS_TexCoord.xy *= Timers.zw;
          VS_TexCoord.y = 1.0 - VS_TexCoord.y;
          VS_TexCoord.zw = InPosition * vec2(0.5,-0.5) + 0.5;
          VS_Exposure = texelFetch( luminanceTexture, ivec2( 0 ), 0 ).x;
          VS_Exposure = PostprocessAttrib.y / VS_Exposure;
//!!!          VS_Exposure = PostprocessAttrib.y / pow(VS_Exposure,2);
        }
    )";

    const char * tonamappingSource = R"(

    // Convert linear RGB into a CIE xyY (xy = chroma, Y = luminance).
    vec3 RGB2xyY( in vec3 rgb ) {
        const mat3 RGB2XYZ = mat3
        (
            0.4124, 0.3576, 0.1805,
            0.2126, 0.7152, 0.0722,
            0.0193, 0.1192, 0.9505
        );

        vec3 XYZ = RGB2XYZ * rgb;

        // XYZ to xyY
        return vec3( XYZ.xy / max( XYZ.x + XYZ.y + XYZ.z, 1e-10 ), XYZ.y );
    }

    // Convert a CIE xyY value into linear RGB.
    vec3 xyY2RGB( in vec3 xyY ) {
        const mat3 XYZ2RGB = mat3
        (
            3.2406, -1.5372, -0.4986,
            -0.9689, 1.8758, 0.0415,
            0.0557, -0.2040, 1.0570
        );

        // xyY to XYZ
        float z_div_y = xyY.z / max( xyY.y, 1e-10 );
        return XYZ2RGB * vec3( z_div_y * xyY.x, xyY.z, z_div_y * (1.0 - xyY.x - xyY.y) );
    }

    vec3 ToneLinear( in vec3 Color, in float Exposure ) {
        /*
        vec3 xyY = RGB2xyY( Color );

        xyY.z *= Exposure;

        return xyY2RGB( xyY );
        */
        return Color * Exposure;
    }

    vec3 ToneReinhard( in vec3 Color, in float Exposure, in float WhitePoint ) {
        vec3 xyY = RGB2xyY( Color );

        float Lp = xyY.z * Exposure;
        xyY.z = Lp * (1.0f + Lp / (WhitePoint * WhitePoint)) / (1.0f + Lp);

        return xyY2RGB( xyY );
    }

    vec3 ACESFilm( in vec3 Color, in float Exposure )
    {
        float a = 2.51f;
        float b = 0.03f;
        float c = 2.43f;
        float d = 0.59f;
        float e = 0.14f;
        vec3 x = Color * Exposure;
        return clamp( (x*(a*x+b))/(x*(c*x+d)+e), vec3(0), vec3(1) );
    }

    )";

    const char * fragmentSourceCode = R"(
        layout( location = 0 ) noperspective in vec4 VS_TexCoord;
        layout( location = 1 ) flat in float VS_Exposure;

        layout( location = 0 ) out vec4 FS_FragColor;

        layout( binding = 0 ) uniform sampler2D imageTexture;
        layout( binding = 1 ) uniform sampler2D Smp_Dither;
        layout( binding = 2 ) uniform sampler2D Smp_Bloom2;
        layout( binding = 3 ) uniform sampler2D Smp_Bloom8;
        layout( binding = 4 ) uniform sampler2D Smp_Bloom32;
        layout( binding = 5 ) uniform sampler2D Smp_Bloom128;

        vec4 ENCODE_SRGB( in vec4 LinearValue ) {
        #ifdef SRGB_GAMMA_APPROX
          return pow( LinearValue, vec4( 1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2, 1.0 ) );
        #else
          const vec4 Shift = vec4( -0.055, -0.055, -0.055, 0.0 );
          const vec4 Scale = vec4( 1.055, 1.055, 1.055, 1.0 );
          const vec4 Pow = vec4( 1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4, 1.0 );
          const vec4 Scale2 = vec4( 12.92, 12.92, 12.92, 1.0 );
          return mix( Scale * pow( LinearValue, Pow ) + Shift, LinearValue * Scale2, step( LinearValue, vec4( 0.0031308 ) ) );
        #endif
        }

        vec3 ENCODE_SRGB( in vec3 LinearValue ) {
        #ifdef SRGB_GAMMA_APPROX
          return pow( LinearValue, vec3( 1.0 / 2.2 ) );
        #else
          return mix( 1.055 * pow( LinearValue, vec3( 1.0 / 2.4 ) ) - 0.055, LinearValue * 12.92, step( LinearValue, vec3( 0.0031308 ) ) );
        #endif
        }

        float ENCODE_SRGB( in float LinearValue ) {
        #ifdef SRGB_GAMMA_APPROX
          return pow( LinearValue, 1.0/2.2 );
        #else
          return mix( 1.055 * pow( LinearValue, 1.0 / 2.4 ) - 0.055, LinearValue * 12.92, step( LinearValue, 0.0031308 ) );
        #endif
        }

        vec4 CalcBloom() {
            vec2 tc = VS_TexCoord.zw;
            vec4 dither = vec4( (texture( Smp_Dither, tc*3.141592 ).r-0.5)*2.0 );

            vec4 bloom0[4];
            vec4 bloom1[4];

            bloom0[0] = texture( Smp_Bloom2, tc );
            bloom0[1] = texture( Smp_Bloom8, tc );
            bloom0[2] = texture( Smp_Bloom32, tc );
            bloom0[3] = texture( Smp_Bloom128, tc );

            bloom1[0] = texture( Smp_Bloom2, tc+dither.xz   / 512.0 );
            bloom1[1] = texture( Smp_Bloom8, tc+dither.zx   / 128.0 );
            bloom1[2] = texture( Smp_Bloom32, tc+dither.xy  /  32.0 );
            bloom1[3] = texture( Smp_Bloom128, tc+dither.yz /   8.0 );

            return mat4(
                bloom0[0]+clamp( bloom1[0]-bloom0[0], -1.0/256.0, +1.0/256.0 ),
                bloom0[1]+clamp( bloom1[1]-bloom0[1], -1.0/256.0, +1.0/256.0 ),
                bloom0[2]+clamp( bloom1[2]-bloom0[2], -1.0/256.0, +1.0/256.0 ),
                bloom0[3]+clamp( bloom1[3]-bloom0[3], -1.0/256.0, +1.0/256.0 )
            ) * PostprocessBloomMix;

        //    return mat4(
        //        bloom0[0],
        //        bloom0[1],
        //        bloom0[2],
        //        bloom0[3]
        //    ) * PostprocessBloomMix;

        }

        void main() {
          FS_FragColor = texture( imageTexture, VS_TexCoord.xy );

        // Bloom
          if ( PostprocessAttrib.x > 0.0 ) {
            FS_FragColor += CalcBloom();
          }

        // Debug bloom
 //       FS_FragColor = CalcBloom();

        

        // Tonemapping
          if ( PostprocessAttrib.y > 0.0 ) {
            if ( VS_TexCoord.x > 0.5 ) {
              FS_FragColor.rgb = ToneLinear( FS_FragColor.rgb, VS_Exposure );
            } else {
              FS_FragColor.rgb = ACESFilm( FS_FragColor.rgb, VS_Exposure );
            }
          }

        //FS_FragColor = vec4( texture( Smp_Dither, VS_TexCoord.zw ) );

        // Vignette
        //  if ( VignetteColorIntensity.a > 0.0 ) {
        //    vec2 VignetteOffset = VS_TexCoord.zw - 0.5;
        //    float LengthSqr = dot( VignetteOffset, VignetteOffset );
        //    float VignetteShade = smoothstep( VignetteOuterInnerRadiusSqr.x, VignetteOuterInnerRadiusSqr.y, LengthSqr );
        //    FS_FragColor.rgb = mix( VignetteColorIntensity.rgb, FS_FragColor.rgb, mix( 1.0, VignetteShade, VignetteColorIntensity.a ) );
        //  }

        // Apply brightness
          FS_FragColor.rgb *= VignetteOuterInnerRadiusSqr.z;

        // Pack pixel luminance to alpha channel for FXAA algorithm
          const vec3 RGB_TO_GRAYSCALE = vec3( 0.2125, 0.7154, 0.0721 );
          FS_FragColor.a = PostprocessAttrib.w > 0.0 ? ENCODE_SRGB( clamp( dot( FS_FragColor.rgb, RGB_TO_GRAYSCALE ), 0.0, 1.0 ) ) : 1.0;

        //// Debug output
        //  if ( VS_TexCoord.x < 0.05 && VS_TexCoord.y < 0.05 ) {
        //      FS_FragColor = vec4( VS_Exposure, VS_Exposure, VS_Exposure, 1.0 );
        //  }

        }
    )";

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( tonamappingSource );
    GShaderSources.Add( fragmentSourceCode );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
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

    pipelineCI.NumStages = AN_ARRAY_SIZE( stages );
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.pRenderPass = &PostprocessPass;
    pipelineCI.Subpass = 0;

    PostprocessPipeline.Initialize( pipelineCI );
}

void APostprocessPassRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    PostprocessSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressV = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressW = SAMPLER_ADDRESS_WRAP;
    DitherSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    BloomSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    LuminanceSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void APostprocessPassRenderer::Render() {
    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &PostprocessPass;
    renderPassBegin.pFramebuffer = &GRenderTarget.GetPostprocessFramebuffer();
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width = GRenderView->Width;
    renderPassBegin.RenderArea.Height = GRenderView->Height;
    renderPassBegin.pColorClearValues = NULL;
    renderPassBegin.pDepthStencilClearValue = NULL;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = GRenderView->Width;
    vp.Height = GRenderView->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawCmd drawCmd;
    drawCmd.VertexCountPerInstance = 4;
    drawCmd.InstanceCount = 1;
    drawCmd.StartVertexLocation = 0;
    drawCmd.StartInstanceLocation = 0;
    
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.GetFramebufferTexture();
    GFrameResources.SamplerBindings[0].pSampler = PostprocessSampler;

    GFrameResources.TextureBindings[1].pTexture = &GOpenGL45RenderBackend.GetDitherTexture();
    GFrameResources.SamplerBindings[1].pSampler = DitherSampler;

    GFrameResources.TextureBindings[2].pTexture = &GRenderTarget.GetBloomTexture().Texture[0];
    GFrameResources.SamplerBindings[2].pSampler = BloomSampler;

    GFrameResources.TextureBindings[3].pTexture = &GRenderTarget.GetBloomTexture().Textures_2[0];
    GFrameResources.SamplerBindings[3].pSampler = BloomSampler;

    GFrameResources.TextureBindings[4].pTexture = &GRenderTarget.GetBloomTexture().Textures_4[0];
    GFrameResources.SamplerBindings[4].pSampler = BloomSampler;

    GFrameResources.TextureBindings[5].pTexture = &GRenderTarget.GetBloomTexture().Textures_6[0];
    GFrameResources.SamplerBindings[5].pSampler = BloomSampler;

    GFrameResources.TextureBindings[6].pTexture = &GRenderTarget.AdaptiveLuminance;
    GFrameResources.SamplerBindings[6].pSampler = LuminanceSampler; // whatever (used texelFetch)
    
    Cmd.BindPipeline( &PostprocessPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}

}
