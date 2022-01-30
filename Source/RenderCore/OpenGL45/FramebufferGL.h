/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "TextureGLImpl.h"
#include <RenderCore/StaticLimits.h>

namespace RenderCore
{

struct SFramebufferDescGL
{
    uint16_t       Width                   = 0;
    uint16_t       Height                  = 0;
    uint16_t       NumColorAttachments     = 0;
    ITextureView** pColorAttachments       = nullptr;
    ITextureView*  pDepthStencilAttachment = nullptr;

    SFramebufferDescGL() = default;

    SFramebufferDescGL(uint16_t       Width,
                       uint16_t       Height,
                       uint16_t       NumColorAttachments,
                       ITextureView** pColorAttachments,
                       ITextureView*  pDepthStencilAttachment) :
        Width(Width), Height(Height), NumColorAttachments(NumColorAttachments), pColorAttachments(pColorAttachments), pDepthStencilAttachment(pDepthStencilAttachment)
    {
    }
};

class AFramebufferGL
{
public:
    AFramebufferGL(SFramebufferDescGL const& Desc, int Hash);
    ~AFramebufferGL();

    unsigned int GetHandleNativeGL() const { return FramebufferId; }

    bool IsDefault() const { return FramebufferId == 0; }

    uint16_t GetWidth() const { return Width; }

    uint16_t GetHeight() const { return Height; }

    uint16_t GetNumColorAttachments() const { return NumColorAttachments; }

    TWeakRef<ITextureView> const* GetColorAttachments() const { return &RTVs[0]; }

    bool HasDepthStencilAttachment() const { return bHasDepthStencilAttachment; }

    ITextureView const* GetDepthStencilAttachment() const { return pDSV; }

    bool IsAttachmentsOutdated() const;

    int GetHash() const { return Hash; }

    bool CompareWith(SFramebufferDescGL const& InDesc) const
    {
        if (InDesc.Width != Width ||
            InDesc.Height != Height ||
            InDesc.NumColorAttachments != NumColorAttachments ||
            !!InDesc.pDepthStencilAttachment != bHasDepthStencilAttachment)
        {
            return false;
        }

        if (bHasDepthStencilAttachment)
        {
            if (pDSV.IsExpired())
                return false;

            if (InDesc.pDepthStencilAttachment->GetUID() != pDSV->GetUID())
                return false;
        }

        for (int a = 0; a < NumColorAttachments; a++)
        {
            if (RTVs[a].IsExpired())
                return false;

            if (InDesc.pColorAttachments[a]->GetUID() != RTVs[a]->GetUID())
                return false;
        }
        return true;
    }

private:
    unsigned int FramebufferId = 0;

    int Hash;

    uint16_t Width;
    uint16_t Height;

    uint16_t               NumColorAttachments;
    TWeakRef<ITextureView> RTVs[MAX_COLOR_ATTACHMENTS];

    bool                   bHasDepthStencilAttachment;
    TWeakRef<ITextureView> pDSV;
};

} // namespace RenderCore
