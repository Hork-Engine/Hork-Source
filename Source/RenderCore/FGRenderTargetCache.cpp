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

#include <RenderCore/FGRenderTargetCache.h>

namespace RenderCore
{

FGRenderTargetCache::FGRenderTargetCache(IDevice* pDevice) :
    pDevice(pDevice)
{}

ITexture* FGRenderTargetCache::Acquire(STextureDesc const& TextureDesc)
{
    // Find free appropriate texture
    for (auto it = FreeTextures.Begin(); it != FreeTextures.End(); it++)
    {
        ITexture* tex = *it;

        if (tex->GetDesc() == TextureDesc)
        {
            //LOG( "Reusing existing texture\n" );
            FreeTextures.Erase(it);
            return tex;
        }
    }

    // Create new texture
    //LOG( "Create new texture ( in use {}, free {} )\n", Textures.Size()+1, FreeTextures.Size() );
    TRef<ITexture> texture;
    pDevice->CreateTexture(TextureDesc, &texture);
    Textures.Append(texture);
    return texture;
}

void FGRenderTargetCache::Release(ITexture* pTexture)
{
    auto numMipLevels = pTexture->GetDesc().NumMipLevels;
    for (decltype(numMipLevels) mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
    {
        pTexture->Invalidate(mipLevel);
    }
    FreeTextures.Append(pTexture);
}

} // namespace RenderCore
