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

float AFont::GetTextBounds(FontStyle const& fontStyle, AStringView text, TextBounds& bounds)
{
    FONScontext* fs = m_FontStash->GetImpl();

    float scale = GEngine->GetRetinaScale().X;
    float invscale = 1.0f / scale;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    fonsSetSpacing(fs, fontStyle.LetterSpacing * scale);
    fonsSetBlur(fs, fontStyle.FontBlur * scale);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetFont(fs, m_FontId);

    float width = fonsTextBounds(fs, 0, 0, text.Begin(), text.End(), &bounds.MinX);

    fonsLineBounds(fs, 0, &bounds.MinY, &bounds.MaxY);

    bounds.MinX *= invscale;
    bounds.MinY *= invscale;
    bounds.MaxX *= invscale;
    bounds.MaxY *= invscale;

    return width * invscale;
}

Float2 AFont::GetTextBoxSize(FontStyle const& fontStyle, float breakRowWidth, AStringView text) const
{
    FONScontext* fs = m_FontStash->GetImpl();

    static TextRow rows[128];
    float          scale    = GEngine->GetRetinaScale().X;
    float          invscale = 1.0f / scale;
    int            nrows;

    float lineh = 0, rminy = 0, rmaxy = 0;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    fonsSetSpacing(fs, fontStyle.LetterSpacing* scale);
    fonsSetBlur(fs, fontStyle.FontBlur * scale);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetFont(fs, m_FontId);

    fonsVertMetrics(fs, nullptr, nullptr, &lineh);
    lineh *= invscale;
    lineh *= fontStyle.LineHeight;    
    
    fonsLineBounds(fs, 0, &rminy, &rmaxy);

    rminy *= invscale;
    rmaxy *= invscale;

    float minx = 0;
    float maxx = 0;
    float miny = 0;
    float maxy = 0;
    float y    = 0;

    while ((nrows = TextBreakLines(fontStyle, text, breakRowWidth, rows, HK_ARRAY_SIZE(rows))) > 0)
    {
        for (int i = 0; i < nrows; i++)
        {
            TextRow* row = &rows[i];

            minx = Math::Min(minx, row->MinX);
            maxx = Math::Max(maxx, row->MaxX);

            miny = Math::Min(miny, y + rminy);
            maxy = Math::Max(maxy, y + rmaxy);

            y += lineh;
        }
        text = AStringView(rows[nrows - 1].Next, text.End());
    }

    return Float2(maxx - minx, maxy - miny);
}

enum NVGcodepointType
{
    NVG_SPACE,
    NVG_NEWLINE,
    NVG_CHAR,
    NVG_CJK_CHAR,
};

int AFont::TextBreakLines(FontStyle const& fontStyle, AStringView text, float breakRowWidth, TextRow* rows, int maxRows, bool bKeepSpaces) const
{
    FONScontext* fs       = m_FontStash->GetImpl();
    AFontStash*  stash = m_FontStash;

    float        scale    = GEngine->GetRetinaScale().X;
    float        invscale = 1.0f / scale;

    //float        scale    = nvg__getFontScale(state) * ctx->devicePxRatio;
    //float        invscale = 1.0f / scale;

    FONStextIter iter, prevIter;
    FONSquad     q;
    int          nrows      = 0;
    float        rowStartX  = 0;
    float        rowWidth   = 0;
    float        rowMinX    = 0;
    float        rowMaxX    = 0;
    const char*  rowStart   = NULL;
    const char*  rowEnd     = NULL;
    const char*  wordStart  = NULL;
    float        wordStartX = 0;
    float        wordMinX   = 0;
    const char*  breakEnd   = NULL;
    float        breakWidth = 0;
    float        breakMaxX  = 0;
    int          type = NVG_SPACE, ptype = NVG_SPACE;
    unsigned int pcodepoint = 0;

    if (maxRows == 0) return 0;

    if (text.IsEmpty()) return 0;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    fonsSetSpacing(fs, fontStyle.LetterSpacing * scale);
    fonsSetBlur(fs, fontStyle.FontBlur * scale);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetFont(fs, m_FontId);

    breakRowWidth *= scale;

    fonsTextIterInit(fs, &iter, 0, 0, text.Begin(), text.End(), FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(fs, &iter, &q))
    {
        if (iter.prevGlyphIndex < 0 && stash->ReallocTexture())
        { // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(fs, &iter, &q); // try again
        }
        prevIter = iter;
        switch (iter.codepoint)
        {
            case 9:      // \t
            case 11:     // \v
            case 12:     // \f
            case 32:     // space
            case 0x00a0: // NBSP
                type = NVG_SPACE;
                break;
            case 10: // \n
                type = pcodepoint == 13 ? NVG_SPACE : NVG_NEWLINE;
                break;
            case 13: // \r
                type = pcodepoint == 10 ? NVG_SPACE : NVG_NEWLINE;
                break;
            case 0x0085: // NEL
                type = NVG_NEWLINE;
                break;
            default:
                if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) ||
                    (iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) ||
                    (iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
                    (iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) ||
                    (iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) ||
                    (iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
                    type = NVG_CJK_CHAR;
                else
                    type = NVG_CHAR;
                break;
        }

        if (type == NVG_NEWLINE)
        {
            // Always handle new lines.
            rows[nrows].Start = rowStart != NULL ? rowStart : iter.str;
            rows[nrows].End   = rowEnd != NULL ? rowEnd : iter.str;
            rows[nrows].Width = rowWidth * invscale;
            rows[nrows].MinX  = rowMinX * invscale;
            rows[nrows].MaxX  = rowMaxX * invscale;
            rows[nrows].Next  = iter.next;
            nrows++;
            if (nrows >= maxRows)
                return nrows;
            // Set null break point
            breakEnd   = rowStart;
            breakWidth = 0.0;
            breakMaxX  = 0.0;
            // Indicate to skip the white space at the beginning of the row.
            rowStart = NULL;
            rowEnd   = NULL;
            rowWidth = 0;
            rowMinX = rowMaxX = 0;
        }
        else
        {
            if (rowStart == NULL)
            {
                // Skip white space until the beginning of the line
                if (type == NVG_CHAR || type == NVG_CJK_CHAR || (bKeepSpaces && type == NVG_SPACE))
                {
                    // The current char is the row so far
                    rowStartX  = iter.x;
                    rowStart   = iter.str;
                    rowEnd     = iter.next;
                    rowWidth   = iter.nextx - rowStartX; // q.x1 - rowStartX;
                    rowMinX    = q.x0 - rowStartX;
                    rowMaxX    = q.x1 - rowStartX;
                    wordStart  = iter.str;
                    wordStartX = iter.x;
                    wordMinX   = q.x0 - rowStartX;
                    // Set null break point
                    breakEnd   = rowStart;
                    breakWidth = 0.0;
                    breakMaxX  = 0.0;
                }
            }
            else
            {
                float nextWidth = iter.nextx - rowStartX;

                // track last non-white space character
                if (type == NVG_CHAR || type == NVG_CJK_CHAR || (bKeepSpaces && type == NVG_SPACE))
                {
                    rowEnd   = iter.next;
                    rowWidth = iter.nextx - rowStartX;
                    rowMaxX  = q.x1 - rowStartX;
                }
                // track last end of a word
                if (((ptype == NVG_CHAR || ptype == NVG_CJK_CHAR) && type == NVG_SPACE) || type == NVG_CJK_CHAR)
                {
                    breakEnd   = iter.str;
                    breakWidth = rowWidth;
                    breakMaxX  = rowMaxX;
                }
                // track last beginning of a word
                if ((ptype == NVG_SPACE && (type == NVG_CHAR || type == NVG_CJK_CHAR)) || type == NVG_CJK_CHAR)
                {
                    wordStart  = iter.str;
                    wordStartX = iter.x;
                    wordMinX   = q.x0 - rowStartX;
                }

                // Break to new line when a character is beyond break width.
                if ((type == NVG_CHAR || type == NVG_CJK_CHAR) && nextWidth > breakRowWidth)
                {
                    // The run length is too long, need to break to new line.
                    if (breakEnd == rowStart)
                    {
                        // The current word is longer than the row length, just break it from here.
                        rows[nrows].Start = rowStart;
                        rows[nrows].End   = iter.str;
                        rows[nrows].Width = rowWidth * invscale;
                        rows[nrows].MinX  = rowMinX * invscale;
                        rows[nrows].MaxX  = rowMaxX * invscale;
                        rows[nrows].Next  = iter.str;
                        nrows++;
                        if (nrows >= maxRows)
                            return nrows;
                        rowStartX  = iter.x;
                        rowStart   = iter.str;
                        rowEnd     = iter.next;
                        rowWidth   = iter.nextx - rowStartX;
                        rowMinX    = q.x0 - rowStartX;
                        rowMaxX    = q.x1 - rowStartX;
                        wordStart  = iter.str;
                        wordStartX = iter.x;
                        wordMinX   = q.x0 - rowStartX;
                    }
                    else
                    {
                        // Break the line from the end of the last word, and start new line from the beginning of the new.
                        rows[nrows].Start = rowStart;
                        rows[nrows].End   = breakEnd;
                        rows[nrows].Width = breakWidth * invscale;
                        rows[nrows].MinX  = rowMinX * invscale;
                        rows[nrows].MaxX  = breakMaxX * invscale;
                        rows[nrows].Next  = wordStart;
                        nrows++;
                        if (nrows >= maxRows)
                            return nrows;
                        rowStartX = wordStartX;
                        rowStart  = wordStart;
                        rowEnd    = iter.next;
                        rowWidth  = iter.nextx - rowStartX;
                        rowMinX   = wordMinX;
                        rowMaxX   = q.x1 - rowStartX;
                        // No change to the word start
                    }
                    // Set null break point
                    breakEnd   = rowStart;
                    breakWidth = 0.0;
                    breakMaxX  = 0.0;
                }
            }
        }

        pcodepoint = iter.codepoint;
        ptype      = type;
    }

    // Break the line from the end of the last word, and start new line from the beginning of the new.
    if (rowStart != NULL)
    {
        rows[nrows].Start = rowStart;
        rows[nrows].End   = rowEnd;
        rows[nrows].Width = rowWidth * invscale;
        rows[nrows].MinX  = rowMinX * invscale;
        rows[nrows].MaxX  = rowMaxX * invscale;
        rows[nrows].Next  = text.End();
        nrows++;
    }

    return nrows;
}

int AFont::TextLineCount(FontStyle const& fontStyle, AStringView text, float breakRowWidth) const
{
    FONScontext* fs    = m_FontStash->GetImpl();
    AFontStash*  stash = m_FontStash;

    float scale    = GEngine->GetRetinaScale().X;

    FONStextIter iter, prevIter;
    FONSquad     q;
    int          nrows      = 0;
    float        rowStartX  = 0;
    float        rowWidth   = 0;
    float        rowMaxX    = 0;
    const char*  rowStart   = NULL;
    const char*  rowEnd     = NULL;
    const char*  wordStart  = NULL;
    float        wordStartX = 0;
    float        wordMinX   = 0;
    const char*  breakEnd   = NULL;
    float        breakWidth = 0;
    float        breakMaxX  = 0;
    int          type = NVG_SPACE, ptype = NVG_SPACE;
    unsigned int pcodepoint = 0;

    if (text.IsEmpty()) return 0;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    fonsSetSpacing(fs, fontStyle.LetterSpacing * scale);
    fonsSetBlur(fs, fontStyle.FontBlur * scale);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetFont(fs, m_FontId);

    breakRowWidth *= scale;

    fonsTextIterInit(fs, &iter, 0, 0, text.Begin(), text.End(), FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(fs, &iter, &q))
    {
        if (iter.prevGlyphIndex < 0 && stash->ReallocTexture())
        { // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(fs, &iter, &q); // try again
        }
        prevIter = iter;
        switch (iter.codepoint)
        {
            case 9:      // \t
            case 11:     // \v
            case 12:     // \f
            case 32:     // space
            case 0x00a0: // NBSP
                type = NVG_SPACE;
                break;
            case 10: // \n
                type = pcodepoint == 13 ? NVG_SPACE : NVG_NEWLINE;
                break;
            case 13: // \r
                type = pcodepoint == 10 ? NVG_SPACE : NVG_NEWLINE;
                break;
            case 0x0085: // NEL
                type = NVG_NEWLINE;
                break;
            default:
                if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) ||
                    (iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) ||
                    (iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
                    (iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) ||
                    (iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) ||
                    (iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
                    type = NVG_CJK_CHAR;
                else
                    type = NVG_CHAR;
                break;
        }

        if (type == NVG_NEWLINE)
        {
            // Always handle new lines.
            nrows++;
            // Set null break point
            breakEnd   = rowStart;
            breakWidth = 0.0;
            breakMaxX  = 0.0;
            // Indicate to skip the white space at the beginning of the row.
            rowStart = NULL;
            rowEnd   = NULL;
            rowWidth = 0;
            rowMaxX = 0;
        }
        else
        {
            if (rowStart == NULL)
            {
                // Skip white space until the beginning of the line
                if (type == NVG_CHAR || type == NVG_CJK_CHAR)
                {
                    // The current char is the row so far
                    rowStartX  = iter.x;
                    rowStart   = iter.str;
                    rowEnd     = iter.next;
                    rowWidth   = iter.nextx - rowStartX; // q.x1 - rowStartX;
                    rowMaxX    = q.x1 - rowStartX;
                    wordStart  = iter.str;
                    wordStartX = iter.x;
                    wordMinX   = q.x0 - rowStartX;
                    // Set null break point
                    breakEnd   = rowStart;
                    breakWidth = 0.0;
                    breakMaxX  = 0.0;
                }
            }
            else
            {
                float nextWidth = iter.nextx - rowStartX;

                // track last non-white space character
                if (type == NVG_CHAR || type == NVG_CJK_CHAR)
                {
                    rowEnd   = iter.next;
                    rowWidth = iter.nextx - rowStartX;
                    rowMaxX  = q.x1 - rowStartX;
                }
                // track last end of a word
                if (((ptype == NVG_CHAR || ptype == NVG_CJK_CHAR) && type == NVG_SPACE) || type == NVG_CJK_CHAR)
                {
                    breakEnd   = iter.str;
                    breakWidth = rowWidth;
                    breakMaxX  = rowMaxX;
                }
                // track last beginning of a word
                if ((ptype == NVG_SPACE && (type == NVG_CHAR || type == NVG_CJK_CHAR)) || type == NVG_CJK_CHAR)
                {
                    wordStart  = iter.str;
                    wordStartX = iter.x;
                    wordMinX   = q.x0 - rowStartX;
                }

                // Break to new line when a character is beyond break width.
                if ((type == NVG_CHAR || type == NVG_CJK_CHAR) && nextWidth > breakRowWidth)
                {
                    nrows++;

                    // The run length is too long, need to break to new line.
                    if (breakEnd == rowStart)
                    {
                        // The current word is longer than the row length, just break it from here.
                        rowStartX  = iter.x;
                        rowStart   = iter.str;
                        rowEnd     = iter.next;
                        rowWidth   = iter.nextx - rowStartX;
                        rowMaxX    = q.x1 - rowStartX;
                        wordStart  = iter.str;
                        wordStartX = iter.x;
                        wordMinX   = q.x0 - rowStartX;
                    }
                    else
                    {
                        // Break the line from the end of the last word, and start new line from the beginning of the new.
                        rowStartX = wordStartX;
                        rowStart  = wordStart;
                        rowEnd    = iter.next;
                        rowWidth  = iter.nextx - rowStartX;
                        rowMaxX   = q.x1 - rowStartX;
                        // No change to the word start
                    }
                    // Set null break point
                    breakEnd   = rowStart;
                    breakWidth = 0.0;
                    breakMaxX  = 0.0;
                }
            }
        }

        pcodepoint = iter.codepoint;
        ptype      = type;
    }

    // Break the line from the end of the last word, and start new line from the beginning of the new.
    if (rowStart != NULL)
    {
        nrows++;
    }

    return nrows;
}

//void AFont::CalcLineBounds(float fontSize, int align, float y, TextLineBounds& bounds)
//{
//    FONScontext* fs = m_FontStash->GetImpl();
//
//    float scale = GEngine->GetRetinaScale().X;
//
//    fonsSetSize(fs, fontSize * scale);
//    //fonsSetSpacing(fs,spacing * scale); don't care
//    //fonsSetBlur(fs, blur * scale); don't care
//    fonsSetAlign(fs, align);
//    fonsSetFont(fs, m_FontId);
//    fonsLineBounds(fs, y, &bounds.MinY, &bounds.MaxY);
//}

void AFont::GetTextMetrics(FontStyle const& fontStyle, TextMetrics& metrics) const
{
    FONScontext* fs = m_FontStash->GetImpl();

    float scale = GEngine->GetRetinaScale().X;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    //fonsSetSpacing(fs, letterSpacing * scale);
    //fonsSetBlur(fs, fontBlur * scale);
    //fonsSetAlign(fs, textAlign);
    fonsSetFont(fs, m_FontId);

    fonsVertMetrics(fs, &metrics.Ascender, &metrics.Descender, &metrics.LineHeight);
}

float AFont::GetCharAdvance(FontStyle const& fontStyle, WideChar ch) const
{
    FONScontext* fs = m_FontStash->GetImpl();

    float scale = GEngine->GetRetinaScale().X;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    fonsSetBlur(fs, fontStyle.FontBlur * scale);
    fonsSetFont(fs, m_FontId);

    return fonsCharAdvanceCP(fs, ch) / scale;
}

float AFont::GetTextAdvance(FontStyle const& fontStyle, AStringView text)
{
    TextBounds dummy;
    return GetTextBounds(fontStyle, text, dummy);
}

int AFont::GetTextGlyphPositions(FontStyle const& fontStyle, AStringView text, GlyphPosition* positions, int maxPositions)
{
    FONScontext* fs       = m_FontStash->GetImpl();
    float        scale    = GEngine->GetRetinaScale().X;
    float        invscale = 1.0f / scale;
    FONStextIter iter, prevIter;
    FONSquad     q;
    int          npos = 0;

    if (text.IsEmpty())
        return 0;

    fonsSetSize(fs, fontStyle.FontSize * scale);
    fonsSetSpacing(fs, fontStyle.LetterSpacing * scale);
    fonsSetBlur(fs, fontStyle.FontBlur * scale);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetFont(fs, m_FontId);

    fonsTextIterInit(fs, &iter, 0, 0, text.Begin(), text.End(), FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(fs, &iter, &q))
    {
        if (iter.prevGlyphIndex < 0 && m_FontStash->ReallocTexture())
        { // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(fs, &iter, &q); // try again
        }
        prevIter             = iter;
        positions[npos].Str  = iter.str;
        positions[npos].X    = iter.x * invscale;
        positions[npos].MinX = Math::Min(iter.x, q.x0) * invscale;
        positions[npos].MaxX = Math::Max(iter.nextx, q.x1) * invscale;
        npos++;
        if (npos >= maxPositions)
            break;
    }

    return npos;
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
