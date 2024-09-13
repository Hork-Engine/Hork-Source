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

#pragma once

#include <Hork/RHI/Common/FrameGraph.h>

HK_NAMESPACE_BEGIN

class SSAORenderer
{
public:
    SSAORenderer();

    void AddPasses(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* LinearDepth, RHI::FGTextureProxy* NormalTexture, RHI::FGTextureProxy** ppSSAOTexture);

private:
    void AddDeinterleaveDepthPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* LinearDepth, RHI::FGTextureProxy** ppSSAOTexture);

    void AddCacheAwareAOPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* DeinterleaveDepthArray, RHI::FGTextureProxy* NormalTexture, RHI::FGTextureProxy** ppDeinterleaveDepthArray);

    void AddReinterleavePass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* SSAOTextureArray, RHI::FGTextureProxy** ppSSAOTexture);

    void AddSimpleAOPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* LinearDepth, RHI::FGTextureProxy* NormalTexture, RHI::FGTextureProxy** ppSSAOTexture);

    void AddAOBlurPass(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* SSAOTexture, RHI::FGTextureProxy* LinearDepth, RHI::FGTextureProxy** ppBluredSSAO);

    enum
    {
        HBAO_RANDOM_SIZE = 4
    };
    enum
    {
        HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE
    };

    Ref<RHI::IPipeline> Pipe;
    Ref<RHI::IPipeline> Pipe_ORTHO;
    Ref<RHI::IPipeline> CacheAwarePipe;
    Ref<RHI::IPipeline> CacheAwarePipe_ORTHO;
    Ref<RHI::IPipeline> BlurPipe;
    Ref<RHI::ITexture>  RandomMap;
    Ref<RHI::IPipeline> DeinterleavePipe;
    Ref<RHI::IPipeline> ReinterleavePipe;
};

HK_NAMESPACE_END
