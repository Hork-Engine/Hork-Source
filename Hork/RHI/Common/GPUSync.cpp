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

#include "GPUSync.h"

#include <Hork/RHI/Common/Device.h>

HK_NAMESPACE_BEGIN

using namespace RHI;

GPUSync::GPUSync(RHI::IImmediateContext* _pImmediateContext) :
    pImmediateContext(_pImmediateContext)
{
}

GPUSync::~GPUSync()
{
}

void GPUSync::SetEvent()
{
    if (Texture)
    {
        pImmediateContext->GenerateTextureMipLevels(Texture);
    }
}

void GPUSync::Wait()
{
    if (!Texture)
    {
        byte data[2 * 2 * 4];
        Core::Memset(data, 128, sizeof(data));

        IDevice* device = pImmediateContext->GetDevice();

        device->CreateTexture(TextureDesc()
                                  .SetFormat(TEXTURE_FORMAT_RGBA8_UNORM)
                                  .SetResolution(TextureResolution2D(2, 2))
                                  .SetMipLevels(2),
                              &Texture);

        pImmediateContext->WriteTexture(Texture, 0, sizeof(data), 1, data);

        device->CreateTexture(TextureDesc()
                                  .SetFormat(TEXTURE_FORMAT_RGBA8_UNORM)
                                  .SetResolution(TextureResolution2D(1, 1))
                                  .SetMipLevels(1),
                              &Staging);
    }
    else
    {
        TextureCopy copy = {};

        copy.SrcRect.Offset.MipLevel = 1;
        copy.SrcRect.Offset.X        = 0;
        copy.SrcRect.Offset.Y        = 0;
        copy.SrcRect.Offset.Z        = 0;
        copy.SrcRect.Dimension.X     = 1;
        copy.SrcRect.Dimension.Y     = 1;
        copy.SrcRect.Dimension.Z     = 1;
        copy.DstOffset.MipLevel      = 0;
        copy.DstOffset.X             = 0;
        copy.DstOffset.Y             = 0;
        copy.DstOffset.Z             = 0;

        pImmediateContext->CopyTextureRect(Texture, Staging, 1, &copy);

        byte data[4];
        pImmediateContext->ReadTexture(Staging, 0, 4, 4, data);
    }
}

HK_NAMESPACE_END
