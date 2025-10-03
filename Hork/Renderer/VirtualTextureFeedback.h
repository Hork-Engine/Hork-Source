/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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
#include <Hork/RHI/Common/FrameGraph.h>

HK_NAMESPACE_BEGIN

class VirtualTextureFeedback
{
public:
    VirtualTextureFeedback(RHI::IDevice* device);
    virtual ~VirtualTextureFeedback();

    void Begin(int Width, int Height);
    void End(int* pFeedbackSize, const void** ppData);

    void AddPass(RHI::FrameGraph& FrameGraph);
    void DrawFeedback(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy* RenderTarget);

    RHI::ITexture* GetFeedbackTexture() { return FeedbackTexture; }
    RHI::ITexture* GetFeedbackDepth() { return FeedbackDepth; }
    RHI::IBuffer* GetPixelBuffer() { return PixelBufferObject[SwapIndex]; }
    Float2 const& GetResolutionRatio() const { return ResolutionRatio; }

private:
    Ref<RHI::IDevice> Device;
    Ref<RHI::ITexture> FeedbackTexture;
    Ref<RHI::ITexture> FeedbackDepth;
    Ref<RHI::IBuffer> PixelBufferObject[2];
    const void* MappedData[2];
    int SwapIndex;
    Float2 ResolutionRatio;
    int FeedbackSize[2];
    Ref<RHI::IPipeline> DrawFeedbackPipeline;
};

HK_NAMESPACE_END
