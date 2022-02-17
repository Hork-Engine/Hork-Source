/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "PipelineGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "ShaderModuleGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

namespace RenderCore
{

APipelineGLImpl::APipelineGLImpl(ADeviceGLImpl* pDevice, SPipelineDesc const& Desc) :
    IPipeline(pDevice)
{
    if (!pDevice->IsFeatureSupported(FEATURE_HALF_FLOAT_VERTEX))
    {
        // Check half float vertex type

        for (SVertexAttribInfo const* desc = Desc.pVertexAttribs; desc < &Desc.pVertexAttribs[Desc.NumVertexAttribs]; desc++)
        {
            if (desc->TypeOfComponent() == COMPONENT_HALF)
            {
                LOG("APipelineGLImpl::ctor: Half floats not supported by current hardware\n");
            }
        }
    }

    pVS  = Desc.pVS;
    pTCS = Desc.pTCS;
    pTES = Desc.pTES;
    pGS  = Desc.pGS;
    pFS  = Desc.pFS;
    pCS  = Desc.pCS;

    PrimitiveTopology = GL_TRIANGLES; // Use triangles by default

    if (Desc.IA.Topology <= PRIMITIVE_TRIANGLE_STRIP_ADJ)
    {
        PrimitiveTopology = PrimitiveTopologyLUT[Desc.IA.Topology];
        NumPatchVertices  = 0;
    }
    else if (Desc.IA.Topology >= PRIMITIVE_PATCHES_1)
    {
        PrimitiveTopology = GL_PATCHES;
        NumPatchVertices  = Desc.IA.Topology - PRIMITIVE_PATCHES_1 + 1; // Must be < GL_MAX_PATCH_VERTICES

        if (NumPatchVertices > pDevice->GetDeviceCaps(DEVICE_CAPS_MAX_PATCH_VERTICES))
        {
            LOG("APipelineGLImpl::ctor: num patch vertices > DEVICE_CAPS_MAX_PATCH_VERTICES\n");
        }
    }

    pVertexLayout     = pDevice->GetVertexLayout(Desc.pVertexBindings, Desc.NumVertexBindings,
                                             Desc.pVertexAttribs, Desc.NumVertexAttribs);
    BlendingState     = pDevice->CachedBlendingState(Desc.BS);
    RasterizerState   = pDevice->CachedRasterizerState(Desc.RS);
    DepthStencilState = pDevice->CachedDepthStencilState(Desc.DSS);

    SAllocatorCallback const& allocator = pDevice->GetAllocator();

    NumSamplerObjects = Desc.ResourceLayout.NumSamplers;
    if (NumSamplerObjects > 0)
    {
        SamplerObjects = (unsigned int*)allocator.Allocate(sizeof(SamplerObjects[0]) * NumSamplerObjects);
        for (int i = 0; i < NumSamplerObjects; i++)
        {
            SamplerObjects[i] = pDevice->CachedSampler(Desc.ResourceLayout.Samplers[i]);
        }
    }
    else
    {
        SamplerObjects = nullptr;
    }

    NumImages = Desc.ResourceLayout.NumImages;
    if (NumImages > 0)
    {
        Images = (SImageInfoGL*)allocator.Allocate(sizeof(SImageInfoGL) * NumImages);
        for (int i = 0; i < NumImages; i++)
        {
            Images[i].AccessMode     = ImageAccessModeLUT[Desc.ResourceLayout.Images[i].AccessMode];
            Images[i].InternalFormat = InternalFormatLUT[Desc.ResourceLayout.Images[i].TextureFormat].InternalFormat;
        }
    }
    else
    {
        Images = nullptr;
    }

    NumBuffers = Desc.ResourceLayout.NumBuffers;
    if (NumBuffers > 0)
    {
        Buffers = (SBufferInfoGL*)allocator.Allocate(sizeof(SBufferInfoGL) * NumBuffers);
        for (int i = 0; i < NumBuffers; i++)
        {
            Buffers[i].BufferType = BufferTargetLUT[Desc.ResourceLayout.Buffers[i].BufferBinding].Target;
        }
    }
    else
    {
        Buffers = nullptr;
    }
}

APipelineGLImpl::~APipelineGLImpl()
{
    SAllocatorCallback const& allocator = GetDevice()->GetAllocator();

    if (SamplerObjects)
    {
        allocator.Deallocate(SamplerObjects);
    }

    if (Images)
    {
        allocator.Deallocate(Images);
    }

    if (Buffers)
    {
        allocator.Deallocate(Buffers);
    }
}

} // namespace RenderCore
