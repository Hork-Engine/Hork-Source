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

#include "OpenGL45ShadowMapRT.h"

#include <Engine/Runtime/Public/RuntimeVariable.h>

using namespace GHI;

#define BLUR_SCALE      1//16//2//2//4

extern ARuntimeVariable RVShadowCascadeBits;

ARuntimeVariable RVShadowCascadeResolution( _CTS( "ShadowCascadeResolution" ),  _CTS( "2048" ) );

namespace OpenGL45 {

AShadowMapRT GShadowMapRT;

void AShadowMapRT::Initialize() {
    CascadeSize = 0;
    MaxCascades = 0;
}

void AShadowMapRT::Deinitialize() {
    Framebuffer.Deinitialize();
    ShadowPoolTexture.Deinitialize();
    DepthMomentsTexture.Deinitialize();
    DepthMomentsTextureTmp.Deinitialize();
}

void AShadowMapRT::CreateFramebuffer() {
    Framebuffer.Deinitialize();
    ShadowPoolTexture.Deinitialize();
    DepthMomentsTexture.Deinitialize();
    DepthMomentsTextureTmp.Deinitialize();

    TextureStorageCreateInfo texStorageCI = {};
    texStorageCI.Type = GHI::TEXTURE_2D_ARRAY;
    texStorageCI.Resolution.Tex2DArray.Width = CascadeSize;
    texStorageCI.Resolution.Tex2DArray.Height = CascadeSize;
    texStorageCI.Resolution.Tex2DArray.NumLayers = MaxCascades;
    texStorageCI.NumLods = 1;

    if ( RVShadowCascadeBits.GetInteger() <= 16 ) {
        texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_DEPTH16;
    } else if ( RVShadowCascadeBits.GetInteger() <= 24 ) {
        texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_DEPTH24;
    } else {
        texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_DEPTH32;
    }

    #ifdef SHADOWMAP_VSM
    texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_DEPTH32;
    #endif

    ShadowPoolTexture.InitializeStorage( texStorageCI );

    #ifdef SHADOWMAP_VSM

    #ifdef SHADOWMAP_EVSM
    texStorageCI.InternalFormat = GHI_IPF_RGBA32F,
    #else
    texStorageCI.InternalFormat = GHI_IPF_RG32F,
    #endif
    DepthMomentsTexture.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2DArray.Width /= BLUR_SCALE;
    texStorageCI.Resolution.Tex2DArray.Height /= BLUR_SCALE;

    DepthMomentsTextureTmp.InitializeStorage( texStorageCI );

    #endif

    FramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = CascadeSize;
    framebufferCI.Height = CascadeSize;

    #ifdef SHADOWMAP_VSM
    FramebufferAttachmentInfo colorAttachments[2];
    colorAttachments[0].pTexture = &DepthMomentsTexture;
    colorAttachments[0].bLayered = false;
    colorAttachments[0].LayerNum = 0;
    colorAttachments[0].LodNum = 0;
    colorAttachments[1].pTexture = &DepthMomentsTextureTmp;
    colorAttachments[1].bLayered = false;
    colorAttachments[1].LayerNum = 0;
    colorAttachments[1].LodNum = 0;
    framebufferCI.pColorAttachments = colorAttachments;
    framebufferCI.NumColorAttachments = 2;
    #else
    framebufferCI.pColorAttachments = nullptr;
    framebufferCI.NumColorAttachments = 0;
    #endif

    FramebufferAttachmentInfo depthAttachment = {};
    depthAttachment.pTexture = &ShadowPoolTexture;
    depthAttachment.bLayered = false;
    depthAttachment.LayerNum = 0;
    depthAttachment.LodNum = 0;
    framebufferCI.pDepthStencilAttachment = &depthAttachment;

    Framebuffer.Initialize( framebufferCI );
}

void AShadowMapRT::Realloc( int _MaxCascades ) {
    AN_Assert( _MaxCascades > 0 );
    if ( CascadeSize != RVShadowCascadeResolution.GetInteger() || MaxCascades != _MaxCascades ) {
        CascadeSize = RVShadowCascadeResolution.GetInteger();
        MaxCascades = _MaxCascades;
        CreateFramebuffer();
    }
}

}
