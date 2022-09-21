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

#include "Font.h"
#include "EmbeddedResources.h"
#include "Engine.h"

#include "nanovg/fontstash.h"

HK_CLASS_META(AFont)

AFontStash::AFontStash()
{
    using namespace RenderCore;

    FONSparams fontParams = {};
    
    fontParams.width  = INITIAL_FONTIMAGE_SIZE;
    fontParams.height = INITIAL_FONTIMAGE_SIZE;
    fontParams.flags  = FONS_ZERO_TOPLEFT;

    m_pImpl = fonsCreateInternal(&fontParams);
    if (!m_pImpl)
    {
        CriticalError("Failed to create font stash\n");
    }

    auto* pDevice = GEngine->GetRenderDevice();

    // Create font texture
    pDevice->CreateTexture(STextureDesc{}
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetResolution(STextureResolution2D(fontParams.width, fontParams.height))
                               .SetSwizzle(STextureSwizzle(TEXTURE_SWIZZLE_ONE, TEXTURE_SWIZZLE_ONE, TEXTURE_SWIZZLE_ONE, TEXTURE_SWIZZLE_R))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &m_FontImages[0]);
    if (!m_FontImages[0])
    {
        CriticalError("Failed to create font texture\n");
    }
}

AFontStash::~AFontStash()
{
    if (m_pImpl)
        fonsDeleteInternal(m_pImpl);
}

bool AFontStash::ReallocTexture()
{
    int iw, ih;

    UpdateTexture();

    if (m_FontImageIdx >= MAX_FONT_IMAGES - 1)
        return false;

    // if next fontImage already have a texture
    if (m_FontImages[m_FontImageIdx + 1])
    {
        iw = m_FontImages[m_FontImageIdx + 1]->GetDesc().Resolution.Width;
        ih = m_FontImages[m_FontImageIdx + 1]->GetDesc().Resolution.Height;
    }
    else
    {
        // calculate the new font image size and create it
        iw = m_FontImages[m_FontImageIdx]->GetDesc().Resolution.Width;
        ih = m_FontImages[m_FontImageIdx]->GetDesc().Resolution.Height;

        if (iw > ih)
            ih *= 2;
        else
            iw *= 2;

        if (iw > MAX_FONTIMAGE_SIZE || ih > MAX_FONTIMAGE_SIZE)
            iw = ih = MAX_FONTIMAGE_SIZE;

        auto* pDevice = GEngine->GetRenderDevice();

        using namespace RenderCore;

        // Create font texture
        pDevice->CreateTexture(STextureDesc{}
                                   .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                   .SetResolution(STextureResolution2D(iw, ih))
                                   .SetSwizzle(STextureSwizzle(TEXTURE_SWIZZLE_ONE, TEXTURE_SWIZZLE_ONE, TEXTURE_SWIZZLE_ONE, TEXTURE_SWIZZLE_R))
                                   .SetBindFlags(BIND_SHADER_RESOURCE),
                               &m_FontImages[m_FontImageIdx + 1]);

        if (!m_FontImages[m_FontImageIdx + 1])
        {
            CriticalError("Failed to create font texture\n");
        }
    }
    ++m_FontImageIdx;
    fonsResetAtlas(m_pImpl, iw, ih);
    return true;
}

void AFontStash::UpdateTexture()
{
    using namespace RenderCore;

    int dirty[4];

    if (fonsValidateTexture(m_pImpl, dirty))
    {
        ITexture* fontImage = m_FontImages[m_FontImageIdx];

        if (fontImage)
        {
            int                  iw, ih;
            const uint8_t*       data = fonsGetTextureData(m_pImpl, &iw, &ih);
            int                  x    = dirty[0];
            int                  y    = dirty[1];
            int                  w    = dirty[2] - dirty[0];
            int                  h    = dirty[3] - dirty[1];

            STextureDesc const& desc = fontImage->GetDesc();

            auto pixelWidthBytes = GetTextureFormatInfo(desc.Format).BytesPerBlock;

            const uint8_t* pData = data + (y * (desc.Resolution.Width * pixelWidthBytes)) + (x * pixelWidthBytes);

            RenderCore::STextureRect rect;
            rect.Offset.X    = x;
            rect.Offset.Y    = y;
            rect.Dimension.X = w;
            rect.Dimension.Y = h;
            rect.Dimension.Z = 1;

            fontImage->WriteRect(rect, desc.Resolution.Width * desc.Resolution.Height * pixelWidthBytes, 1, pData, desc.Resolution.Width * pixelWidthBytes);
        }
    }
}

void AFontStash::Cleanup()
{
    using namespace RenderCore;

    if (m_FontImageIdx != 0)
    {
        TRef<ITexture> fontImage = m_FontImages[m_FontImageIdx];
        int i, j;
        // delete images that smaller than current one
        if (!fontImage)
            return;
        uint32_t iw = fontImage->GetDesc().Resolution.Width;
        uint32_t ih = fontImage->GetDesc().Resolution.Height;
        for (i = j = 0; i < m_FontImageIdx; i++)
        {
            if (m_FontImages[i])
            {
                uint32_t nw = m_FontImages[i]->GetDesc().Resolution.Width;
                uint32_t nh = m_FontImages[i]->GetDesc().Resolution.Height;
                if (nw < iw || nh < ih)
                    m_FontImages[i].Reset();
                else
                    m_FontImages[j++] = m_FontImages[i];
            }
        }
        // make current font image to first
        m_FontImages[j++]    = m_FontImages[0];
        m_FontImages[0]   = fontImage;
        m_FontImageIdx       = 0;
        // clear all images after j
        for (i = j; i < MAX_FONT_IMAGES; i++)
            m_FontImages[i].Reset();
    }
}

AFont::AFont()
{
    m_FontStash = GetSharedInstance<AFontStash>();
}

AFont::~AFont()
{
    fonsRemoveFont(m_FontStash->GetImpl(), m_FontId);
}

float AFont::CalcTextBounds(float textSize, float blur, int align, float spacing, float x, float y, AStringView text, TextBounds& bounds)
{
    FONScontext* fs = m_FontStash->GetImpl();
    fonsSetSize(fs, textSize);
    fonsSetSpacing(fs, spacing);
    fonsSetBlur(fs, blur);
    fonsSetAlign(fs, align);
    fonsSetFont(fs, m_FontId);
    return fonsTextBounds(fs, x, y, text.Begin(), text.End(), &bounds.MinX);
}

void AFont::CalcLineBounds(float textSize, int align, float y, TextLineBounds& bounds)
{
    FONScontext* fs = m_FontStash->GetImpl();
    fonsSetSize(fs, textSize);
    //fonsSetSpacing(fs,spacing); don't care
    //fonsSetBlur(fs, blur); don't care
    fonsSetAlign(fs, align);
    fonsSetFont(fs, m_FontId);
    fonsLineBounds(fs, y, &bounds.MinY, &bounds.MaxY);
}

void AFont::CalcVertMetrics(float textSize, TextMetrics& metrics)
{
    FONScontext* fs = m_FontStash->GetImpl();
    fonsSetSize(fs, textSize);
    //fonsSetSpacing(fs,spacing); don't care
    //fonsSetBlur(fs, blur); don't care
    //fonsSetAlign(fs, align); don't care
    fonsSetFont(fs, m_FontId);
    fonsVertMetrics(fs, &metrics.Ascender, &metrics.Descender, &metrics.LineHeight);
}

float AFont::GetCharAdvance(WideChar ch, float textSize, float blur, float devicePixelRatio) const
{
    FONScontext* fs = m_FontStash->GetImpl();

    float scale = devicePixelRatio;

    fonsSetSize(fs, textSize * scale);
    fonsSetBlur(fs, blur * scale);
    fonsSetFont(fs, m_FontId);

    return fonsCharAdvanceCP(fs, ch) / scale;
}

void AFont::LoadInternalResource(AStringView Path)
{
    if (!Path.Icmp("/Default/Fonts/Default"))
    {
        // Load embedded ProggyClean.ttf

        // NOTE:
        // ProggyClean.ttf
        // Copyright (c) 2004, 2005 Tristan Grimmer
        // MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
        // Download and more information at http://upperbounds.net

        AFile f = AFile::OpenRead("Fonts/ProggyClean.ttf", Runtime::GetEmbeddedResources());
        if (!f)
        {
            CriticalError("Failed to create default font\n");
        }

        m_Blob = f.AsBlob();

        const int fontIndex = 0;

        m_FontId = fonsAddFontMem(m_FontStash->GetImpl(), (uint8_t*)m_Blob.GetData(), m_Blob.Size(), 0, fontIndex);
        if (m_FontId == FONS_INVALID)
        {
            CriticalError("Failed to create default font\n");
        }

        return;
    }

    LOG("Unknown internal font {}\n", Path);

    LoadInternalResource("/Default/Fonts/Default");
}

bool AFont::LoadResource(IBinaryStreamReadInterface& stream)
{
    FONScontext* fs = m_FontStash->GetImpl();

    m_Blob = stream.AsBlob();

    const int fontIndex = 0;

    m_FontId = fonsAddFontMem(fs, (uint8_t*)m_Blob.GetData(), m_Blob.Size(), 0, fontIndex);
    if (m_FontId == FONS_INVALID)
    {
        LOG("AFont::LoadResource: failed to load font {}\n", stream.GetName());
        return false;
    }

    return true;
}

bool AFont::AddFallbackFont(AFont* fallbackFont)
{
    if (!fallbackFont)
        return false;

    if (fallbackFont == this)
        return false;

    if (fonsAddFallbackFont(m_FontStash->GetImpl(), GetId(), fallbackFont->GetId()))
    {
        // Keep reference to fallback
        m_Fallbacks.Add(TRef<AFont>(fallbackFont));
        return true;
    }
    return false;
}

void AFont::ResetFallbackFonts()
{
    fonsResetFallbackFont(m_FontStash->GetImpl(), GetId());
    m_Fallbacks.Clear();
}
