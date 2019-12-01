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

#pragma once

#include "GHI/GHIPipeline.h"
#include "GHI/GHIShader.h"
#include "GHI/GHISampler.h"
#include "GHI/GHIBuffer.h"
#include "GHI/GHITexture.h"
#include "GHI/GHICommandBuffer.h"
#include "GHI/GHIDevice.h"
#include "GHI/GHIState.h"
#include "GHI/GHIFramebuffer.h"
#include "GHI/GHIPipeline.h"
#include "GHI/GHIRenderPass.h"

#include <Runtime/Public/RenderCore.h>
#include <Runtime/Public/RuntimeVariable.h>

#define SCISSOR_TEST false
#define DEPTH_PREPASS

#define SHADOWMAP_PCF
//#define SHADOWMAP_PCSS
//#define SHADOWMAP_VSM
//#define SHADOWMAP_EVSM

extern ARuntimeVariable RVRenderSnapshot;

namespace OpenGL45 {

extern GHI::Device          GDevice;
extern GHI::State           GState;
extern GHI::CommandBuffer   Cmd;
extern SRenderFrame *       GFrameData;
extern SRenderView *        GRenderView;

AN_FORCEINLINE GHI::Buffer * GPUBufferHandle( ABufferGPU * _Buffer ) {
    return static_cast< GHI::Buffer * >( _Buffer->pHandleGPU );
}

AN_FORCEINLINE GHI::Texture * GPUTextureHandle( ATextureGPU * _Texture ) {
    return static_cast< GHI::Texture * >( _Texture->pHandleGPU );
}

void SaveSnapshot( GHI::Texture & _Texture );

}
