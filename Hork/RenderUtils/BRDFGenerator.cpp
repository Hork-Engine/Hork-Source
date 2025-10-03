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

#include "BRDFGenerator.h"
#include "DrawUtils.h"

#include <Hork/ShaderUtils/ShaderUtils.h>
#include <Hork/RHI/Common/FrameGraph.h>

HK_NAMESPACE_BEGIN

using namespace RHI;

const int BRDF_TEXTURE_WIDTH  = 512;
const int BRDF_TEXTURE_HEIGHT = 256;

BRDFGenerator::BRDFGenerator(IDevice* device) :
    m_Device(device)
{
    ShaderUtils::CreateFullscreenQuadPipeline(m_Device, &m_Pipeline, "gen/brdfgen.vert", "gen/brdfgen.frag");
}

void BRDFGenerator::Render(Ref<RHI::ITexture>* ppTexture)
{
    FrameGraph  frameGraph(m_Device);
    RenderPass& pass = frameGraph.AddTask<RenderPass>("BRDF generation pass");

    pass.SetRenderArea(BRDF_TEXTURE_WIDTH, BRDF_TEXTURE_HEIGHT);

    pass.SetColorAttachments(
        {{TextureAttachment("Render target texture",
                             TextureDesc{}
                                 .SetFormat(TEXTURE_FORMAT_RG16_FLOAT)
                                 .SetResolution(TextureResolution2D(BRDF_TEXTURE_WIDTH, BRDF_TEXTURE_HEIGHT))
                                 .SetBindFlags(BIND_SHADER_RESOURCE))
              .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE)}});

    pass.AddSubpass({0}, // color attachments
                    [&](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        RenderUtils::DrawSAQ(immediateCtx, m_Pipeline);
                    });

    FGTextureProxy* pTexture = pass.GetColorAttachments()[0].pResource;
    pTexture->SetResourceCapture(true);

    frameGraph.Build();
    m_Device->GetImmediateContext()->ExecuteFrameGraph(&frameGraph);

    *ppTexture = pTexture->Actual();
}

HK_NAMESPACE_END
