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

#include "TextureView.h"
#include "Texture.h"

namespace RenderCore
{

ITextureView::ITextureView(STextureViewDesc const& TextureViewDesc, ITexture* pTexture) :
    IDeviceObject(pTexture->GetDevice(), PROXY_TYPE), Desc(TextureViewDesc), pTexture(pTexture)
{
#ifdef HK_DEBUG
    STextureDesc const& textureDesc = pTexture->GetDesc();

    HK_ASSERT(TextureViewDesc.ViewType != TEXTURE_VIEW_UNDEFINED);

    HK_ASSERT(TextureViewDesc.FirstMipLevel >= 0 && TextureViewDesc.NumMipLevels >= 1);
    HK_ASSERT(TextureViewDesc.FirstMipLevel + TextureViewDesc.NumMipLevels <= textureDesc.NumMipLevels);

    HK_ASSERT(TextureViewDesc.NumSlices > 0);

    if (TextureViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
    {
        // clang-format off
        HK_ASSERT_(
            ((textureDesc.Type == TEXTURE_1D)             && (TextureViewDesc.Type == TEXTURE_1D || TextureViewDesc.Type == TEXTURE_1D_ARRAY)) ||
            ((textureDesc.Type == TEXTURE_1D_ARRAY)       && (TextureViewDesc.Type == TEXTURE_1D || TextureViewDesc.Type == TEXTURE_1D_ARRAY)) ||
            ((textureDesc.Type == TEXTURE_2D)             && ((TextureViewDesc.Type == TEXTURE_2D || TextureViewDesc.Type == TEXTURE_2D_ARRAY))) ||
            ((textureDesc.Type == TEXTURE_2D_ARRAY)       && ((TextureViewDesc.Type == TEXTURE_2D || TextureViewDesc.Type == TEXTURE_2D_ARRAY))) ||
            ((textureDesc.Type == TEXTURE_3D)             && (TextureViewDesc.Type == TEXTURE_3D)) ||
            ((textureDesc.Type == TEXTURE_CUBE_MAP)       && (TextureViewDesc.Type == TEXTURE_CUBE_MAP || TextureViewDesc.Type == TEXTURE_2D || TextureViewDesc.Type == TEXTURE_2D_ARRAY || TextureViewDesc.Type == TEXTURE_CUBE_MAP_ARRAY)) ||
            ((textureDesc.Type == TEXTURE_CUBE_MAP_ARRAY) && (TextureViewDesc.Type == TEXTURE_CUBE_MAP || TextureViewDesc.Type == TEXTURE_2D || TextureViewDesc.Type == TEXTURE_2D_ARRAY || TextureViewDesc.Type == TEXTURE_CUBE_MAP_ARRAY)),
            "ITextureView::ctor: incompatible texture types");
        // clang-format on

        HK_ASSERT(TextureViewDesc.FirstSlice + TextureViewDesc.NumSlices <= pTexture->GetSliceCount(TextureViewDesc.FirstMipLevel));
    }
    else if (TextureViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET || TextureViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL)
    {
        //HK_ASSERT(textureDesc.Type == TextureViewDesc.Type);
        HK_ASSERT(textureDesc.Format == TextureViewDesc.Format);

        HK_ASSERT(TextureViewDesc.NumMipLevels == 1);
        HK_ASSERT(TextureViewDesc.NumSlices == 1 || TextureViewDesc.NumSlices == pTexture->GetSliceCount(TextureViewDesc.FirstMipLevel));
    }
    else if (TextureViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS)
    {
        HK_ASSERT(textureDesc.Type == TextureViewDesc.Type);

        HK_ASSERT(TextureViewDesc.NumSlices == 1 || TextureViewDesc.NumSlices == pTexture->GetSliceCount(TextureViewDesc.FirstMipLevel));
    }

    // TODO: Check internal formats compatibility
#endif
}

uint32_t ITextureView::GetWidth() const
{
    return Math::Max(1u, pTexture->GetWidth() >> Desc.FirstMipLevel);
}

uint32_t ITextureView::GetHeight() const
{
    return Math::Max(1u, pTexture->GetHeight() >> Desc.FirstMipLevel);
}

} // namespace RenderCore
