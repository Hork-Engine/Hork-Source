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

#include "RenderDefs.h"

#include "VT/VirtualTextureAnalyzer.h"
#include "VT/VirtualTextureFeedback.h"
#include "VT/VirtualTexturePhysCache.h"

#include "CanvasRenderer.h"
#include "FrameRenderer.h"

#include <Runtime/Public/VertexMemoryGPU.h>

class ARenderBackend : public ARefCounted
{
public:
    ARenderBackend();
    ~ARenderBackend();

    void RenderFrame( SRenderFrame * pFrameData );

    void InitializeMaterial( AMaterialGPU * _Material, SMaterialDef const * _Def );

private:
    void RenderView( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture );
    void SetViewConstants();
    void UploadShaderResources();

    TRef< AFrameGraph > FrameGraph;
    AFrameRenderer::SFrameGraphCaptured CapturedResources;

    TRef< ACanvasRenderer > CanvasRenderer;
    TRef< AFrameRenderer > FrameRenderer;

    TRef< RenderCore::IQueryPool > TimeQuery;

    TRef< RenderCore::IQueryPool > TimeStamp1;
    TRef< RenderCore::IQueryPool > TimeStamp2;

    TRef< AVirtualTextureFeedbackAnalyzer > FeedbackAnalyzerVT;
    TRef< AVirtualTextureCache > PhysCacheVT;

    TRef< RenderCore::IPipeline > TerrainDepthPipeline;
    TRef< RenderCore::IPipeline > TerrainLightPipeline;
    TRef< RenderCore::IPipeline > TerrainWireframePipeline;

    // Just for test
    TRef< AVirtualTexture > TestVT;
};
