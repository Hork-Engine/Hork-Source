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

#include "TextureViewGLImpl.h"
#include <Containers/Public/StdHash.h>

namespace RenderCore {

class ADeviceGLImpl;

class ATextureGLImpl final : public ITexture
{
public:
    ATextureGLImpl(ADeviceGLImpl* pDevice, STextureDesc const& TextureDesc, bool bDummyTexture = false);
    ~ATextureGLImpl();

    void MakeBindlessSamplerResident(BindlessHandle Handle, bool bResident) override;

    bool IsBindlessSamplerResident(BindlessHandle Handle) override;

    BindlessHandle GetBindlessSampler(SSamplerDesc const& SamplerDesc) override;

    ITextureView* GetTextureView(STextureViewDesc const& TextureViewDesc) override;

    void GetMipLevelInfo(uint16_t MipLevel, STextureMipLevelInfo* pInfo) const;

    // TODO: Move Invalidate to FrameGraph
    void Invalidate(uint16_t MipLevel) override;
    void InvalidateRect( uint32_t _NumRectangles, STextureRect const * _Rectangles ) override;

    bool IsDummyTexture() const { return bDummyTexture; }

private:
    void CreateDefaultViews();

    TStdHashMap<STextureViewDesc, TRef<ATextureViewGLImpl>> Views;

    TStdHashSet<uint64_t> BindlessSamplers;

    // Dummy texture is used for default color and depth buffers
    bool bDummyTexture{};
};

}
