/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "MaterialSamplers.h"

HK_NAMESPACE_BEGIN

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;

MaterialSamplers::MaterialSamplers()
{
    using namespace RenderCore;
    LightmapSampler.Filter = FILTER_LINEAR;
    LightmapSampler.AddressU = SAMPLER_ADDRESS_WRAP;
    LightmapSampler.AddressV = SAMPLER_ADDRESS_WRAP;
    LightmapSampler.AddressW = SAMPLER_ADDRESS_WRAP;

    ReflectSampler.Filter = FILTER_MIPMAP_BILINEAR;
    ReflectSampler.AddressU = SAMPLER_ADDRESS_BORDER;
    ReflectSampler.AddressV = SAMPLER_ADDRESS_BORDER;
    ReflectSampler.AddressW = SAMPLER_ADDRESS_BORDER;

    ReflectDepthSampler.Filter = FILTER_NEAREST;
    ReflectDepthSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    ReflectDepthSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    ReflectDepthSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    VirtualTextureSampler.Filter = FILTER_LINEAR;
    VirtualTextureSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    VirtualTextureSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    VirtualTextureSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    VirtualTextureIndirectionSampler.Filter = FILTER_MIPMAP_NEAREST;
    VirtualTextureIndirectionSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    VirtualTextureIndirectionSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    VirtualTextureIndirectionSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    ShadowDepthSamplerPCF.Filter = FILTER_LINEAR;
    ShadowDepthSamplerPCF.AddressU = SAMPLER_ADDRESS_MIRROR; //SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCF.AddressV = SAMPLER_ADDRESS_MIRROR; //SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCF.AddressW = SAMPLER_ADDRESS_MIRROR; //SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCF.MipLODBias = 0;
    //ShadowDepthSamplerPCF.ComparisonFunc = CMPFUNC_LEQUAL;
    ShadowDepthSamplerPCF.ComparisonFunc = CMPFUNC_LESS;
    ShadowDepthSamplerPCF.bCompareRefToTexture = true;

    ShadowDepthSamplerVSM.Filter = FILTER_LINEAR;
    ShadowDepthSamplerVSM.AddressU = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerVSM.AddressV = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerVSM.AddressW = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerVSM.MipLODBias = 0;
    ShadowDepthSamplerVSM.BorderColor[0] = VSM_ClearValue.X;
    ShadowDepthSamplerVSM.BorderColor[1] = VSM_ClearValue.Y;
    ShadowDepthSamplerVSM.BorderColor[2] = VSM_ClearValue.Z;
    ShadowDepthSamplerVSM.BorderColor[3] = VSM_ClearValue.W;

    ShadowDepthSamplerEVSM.Filter = FILTER_LINEAR;
    ShadowDepthSamplerEVSM.AddressU = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerEVSM.AddressV = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerEVSM.AddressW = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerEVSM.MipLODBias = 0;
    ShadowDepthSamplerEVSM.BorderColor[0] = EVSM_ClearValue.X;
    ShadowDepthSamplerEVSM.BorderColor[1] = EVSM_ClearValue.Y;
    ShadowDepthSamplerEVSM.BorderColor[2] = EVSM_ClearValue.Z;
    ShadowDepthSamplerEVSM.BorderColor[3] = EVSM_ClearValue.W;

    ShadowDepthSamplerPCSS0.AddressU = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCSS0.AddressV = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCSS0.AddressW = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCSS0.MipLODBias = 0;
    //ShadowDepthSamplerPCSS0.BorderColor[0] = ShadowDepthSamplerPCSS0.BorderColor[1] = ShadowDepthSamplerPCSS0.BorderColor[2] = ShadowDepthSamplerPCSS0.BorderColor[3] = 1.0f;
    // Find blocker point sampler
    ShadowDepthSamplerPCSS0.Filter = FILTER_NEAREST; //FILTER_LINEAR;
                                                     //ShadowDepthSamplerPCSS0.ComparisonFunc = CMPFUNC_GREATER;//CMPFUNC_GEQUAL;
                                                     //ShadowDepthSamplerPCSS0.CompareRefToTexture = true;

    // PCF_Sampler
    ShadowDepthSamplerPCSS1.AddressU = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCSS1.AddressV = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCSS1.AddressW = SAMPLER_ADDRESS_BORDER;
    ShadowDepthSamplerPCSS1.MipLODBias = 0;
    ShadowDepthSamplerPCSS1.Filter = FILTER_LINEAR; //GHI_Filter_Min_LinearMipmapLinear_Mag_Linear; // D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR  Is the same?
    ShadowDepthSamplerPCSS1.ComparisonFunc = CMPFUNC_LESS;
    ShadowDepthSamplerPCSS1.bCompareRefToTexture = true;
    ShadowDepthSamplerPCSS1.BorderColor[0] = ShadowDepthSamplerPCSS1.BorderColor[1] = ShadowDepthSamplerPCSS1.BorderColor[2] = ShadowDepthSamplerPCSS1.BorderColor[3] = 1.0f; // FIXME?

    OmniShadowMapSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    OmniShadowMapSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    OmniShadowMapSampler.AddressW = SAMPLER_ADDRESS_CLAMP;
    OmniShadowMapSampler.Filter = FILTER_LINEAR;

    IESSampler.Filter = FILTER_LINEAR;
    IESSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    IESSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    IESSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    ClusterLookupSampler.Filter = FILTER_NEAREST;
    ClusterLookupSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    ClusterLookupSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    ClusterLookupSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SSAOSampler.Filter = FILTER_NEAREST;
    SSAOSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    SSAOSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    SSAOSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    LookupBRDFSampler.Filter = FILTER_LINEAR;
    LookupBRDFSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    LookupBRDFSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    LookupBRDFSampler.AddressW = SAMPLER_ADDRESS_CLAMP;
}

MaterialSamplers g_MaterialSamplers;

HK_NAMESPACE_END
