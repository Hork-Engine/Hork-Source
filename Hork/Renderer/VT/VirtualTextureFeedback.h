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

#include <Hork/Core/Allocators/LinearAllocator.h>
#include <Hork/Math/VectorMath.h>
#include <Hork/RenderCore/FrameGraph.h>

HK_NAMESPACE_BEGIN

class VirtualTextureFeedback
{
public:
    VirtualTextureFeedback();
    virtual ~VirtualTextureFeedback();

    void Begin(int Width, int Height);
    void End(int* pFeedbackSize, const void** ppData);

    void AddPass(RenderCore::FrameGraph& FrameGraph);
    void DrawFeedback(RenderCore::FrameGraph& FrameGraph, RenderCore::FGTextureProxy* RenderTarget);

    RenderCore::ITexture* GetFeedbackTexture() { return FeedbackTexture; }
    RenderCore::ITexture* GetFeedbackDepth() { return FeedbackDepth; }
    RenderCore::IBuffer* GetPixelBuffer() { return PixelBufferObject[SwapIndex]; }
    Float2 const& GetResolutionRatio() const { return ResolutionRatio; }

private:
    Ref<RenderCore::ITexture> FeedbackTexture;
    Ref<RenderCore::ITexture> FeedbackDepth;
    Ref<RenderCore::IBuffer> PixelBufferObject[2];
    const void* MappedData[2];
    int SwapIndex;
    Float2 ResolutionRatio;
    int FeedbackSize[2];
    Ref<RenderCore::IPipeline> DrawFeedbackPipeline;
};

HK_NAMESPACE_END
