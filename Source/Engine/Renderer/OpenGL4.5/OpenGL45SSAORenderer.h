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

#pragma once

#include "OpenGL45Common.h"

namespace OpenGL45 {

class ASSAORenderer {
public:
    ASSAORenderer();

    AFrameGraphTextureStorage * AddPasses( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * LinearDepth, AFrameGraphTextureStorage * NormalTexture );

private:
    AFrameGraphTextureStorage * AddDeinterleaveDepthPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * LinearDepth );

    AFrameGraphTextureStorage * AddCacheAwareAOPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * DeinterleaveDepthArray, AFrameGraphTextureStorage * NormalTexture );

    AFrameGraphTextureStorage * AddReinterleavePass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * SSAOTextureArray );

    AFrameGraphTextureStorage * AddAOBlurPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * SSAOTexture, AFrameGraphTextureStorage * LinearDepth );

    void ResizeAO( int Width, int Height );

    void CreateSamplers();

    enum { HBAO_RANDOM_SIZE = 4 };
    enum { HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE*HBAO_RANDOM_SIZE };

    int AOWidth = 0;
    int AOHeight = 0;
    int AOQuarterWidth = 0;
    int AOQuarterHeight = 0;

    GHI::Texture SSAODeinterleaveDepthArray;
    GHI::Texture SSAODeinterleaveDepthView[HBAO_RANDOM_ELEMENTS];

    GHI::Pipeline Pipe;
    GHI::Pipeline Pipe_ORTHO;
    GHI::Pipeline CacheAwarePipe;
    GHI::Pipeline CacheAwarePipe_ORTHO;
    GHI::Pipeline BlurPipe;
    GHI::Sampler DepthSampler;
    GHI::Sampler LinearDepthSampler;
    GHI::Sampler NormalSampler;
    GHI::Sampler BlurSampler;
    GHI::Sampler NearestSampler;
    GHI::Sampler RandomMapSampler;
    GHI::Texture RandomMap;
    GHI::Pipeline DeinterleavePipe;
    GHI::Pipeline ReinterleavePipe;
    Float3 hbaoRandom[HBAO_RANDOM_ELEMENTS];
};

}
