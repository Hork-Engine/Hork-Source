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

#pragma once

#include <RenderCore/FrameGraph/FrameGraph.h>

class ASSAORenderer {
public:
    ASSAORenderer();

    void AddPasses( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture * NormalTexture, AFrameGraphTexture ** ppSSAOTexture );

private:
    void AddDeinterleaveDepthPass( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture ** ppSSAOTexture );

    void AddCacheAwareAOPass( AFrameGraph & FrameGraph, AFrameGraphTexture * DeinterleaveDepthArray, AFrameGraphTexture * NormalTexture, AFrameGraphTexture ** ppDeinterleaveDepthArray );

    void AddReinterleavePass( AFrameGraph & FrameGraph, AFrameGraphTexture * SSAOTextureArray, AFrameGraphTexture ** ppSSAOTexture );

    void AddSimpleAOPass( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture * NormalTexture, AFrameGraphTexture ** ppSSAOTexture );

    void AddAOBlurPass( AFrameGraph & FrameGraph, AFrameGraphTexture * SSAOTexture, AFrameGraphTexture * LinearDepth, AFrameGraphTexture ** ppBluredSSAO );

    void ResizeAO( int Width, int Height );

    enum { HBAO_RANDOM_SIZE = 4 };
    enum { HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE*HBAO_RANDOM_SIZE };

    int AOWidth = 0;
    int AOHeight = 0;
    int AOQuarterWidth = 0;
    int AOQuarterHeight = 0;

    TRef< RenderCore::ITexture > SSAODeinterleaveDepthArray;
    TRef< RenderCore::ITexture > SSAODeinterleaveDepthView[HBAO_RANDOM_ELEMENTS];

    TRef< RenderCore::IPipeline > Pipe;
    TRef< RenderCore::IPipeline > Pipe_ORTHO;
    TRef< RenderCore::IPipeline > CacheAwarePipe;
    TRef< RenderCore::IPipeline > CacheAwarePipe_ORTHO;
    TRef< RenderCore::IPipeline > BlurPipe;
    TRef< RenderCore::ITexture > RandomMap;
    TRef< RenderCore::IPipeline > DeinterleavePipe;
    TRef< RenderCore::IPipeline > ReinterleavePipe;
};
