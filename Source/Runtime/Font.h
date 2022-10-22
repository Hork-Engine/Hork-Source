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

#pragma once

#include "Texture.h"

class AFontStash : public ARefCounted
{
public:
    AFontStash();
    ~AFontStash();

    struct FONScontext* GetImpl()
    {
        return m_pImpl;
    }

    bool ReallocTexture();
    void UpdateTexture();

    RenderCore::ITexture* GetTexture()
    {
        return m_FontImages[m_FontImageIdx];
    }

    void Cleanup();

private:
    enum
    {
        MAX_FONT_IMAGES = 4,
        MAX_FONTIMAGE_SIZE = 2048,
        INITIAL_FONTIMAGE_SIZE = 512
    };
    struct FONScontext*        m_pImpl{};
    TRef<RenderCore::ITexture> m_FontImages[MAX_FONT_IMAGES];
    int                        m_FontImageIdx{};
};

struct TextBounds
{
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;

    float Width() const
    {
        return MaxX - MinX;
    }

    float Height() const
    {
        return MaxY - MinY;
    }
};

//struct TextLineBounds
//{
//    float MinY;
//    float MaxY;
//};

struct TextMetrics
{
    float Ascender;
    float Descender;
    float LineHeight;
};

struct TextRow
{
    /** Pointer to the input text where the row starts. */
    const char* Start;

    /** Pointer to the input text where the row ends(one past the last character). */
    const char* End;

    /** Pointer to the beginning of the next row. */
    const char* Next;

    /** Logical width of the row. */
    float Width;

    /** Actual bounds of the row.Logical with and bounds can differ because of kerning and some parts over extending. */
    float MinX, MaxX;

    AStringView GetStringView() const { return AStringView(Start, End); }
};

struct FontStyle
{
    float FontSize{14};
    float FontBlur{0};

    /** Letter spacing. */
    float LetterSpacing{0};

    /** Proportional line height. The line height is specified as multiple of font size. */
    float LineHeight{1};
};

struct GlyphPosition
{
    /** Position of the glyph in the input string. */
    const char* Str;

    /** The x - coordinate of the logical glyph position. */
    float X;

    /** The bounds of the glyph shape. */
    float MinX, MaxX;
};

class AFont : public AResource
{
    HK_CLASS(AFont, AResource)

public:
    AFont();
    ~AFont();

    int GetId() const
    {
        return m_FontId;
    }

    /** Measures the specified text string (see TextBounds structure).
    Returns horizontal advance of the text. */
    float GetTextBounds(FontStyle const& fontStyle, AStringView text, TextBounds& bounds);

    /** Returns horizontal advance of the text. */
    float GetTextAdvance(FontStyle const& fontStyle, AStringView text);

    /** Measures the size of specified multi-text string */
    Float2 GetTextBoxSize(FontStyle const& fontStyle, float breakRowWidth, AStringView text) const;

    //void CalcLineBounds(float fontSize, int align, float y, TextLineBounds& bounds);

    /** Returns the vertical metrics based on the current text style. */
    void GetTextMetrics(FontStyle const& fontStyle, TextMetrics& metrics);

    float GetCharAdvance(FontStyle const& fontStyle, WideChar ch) const;
    
    /** Calculates the glyph x positions of the specified text.
    Measured values are returned in local coordinate space. */
    int GetTextGlyphPositions(FontStyle const& fontStyle, AStringView text, GlyphPosition* positions, int maxPositions);

    /** Breaks the specified text into lines.
    White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
    Words longer than the max width are slit at nearest character (i.e. no hyphenation). */
    int TextBreakLines(FontStyle const& fontStyle, AStringView text, float breakRowWidth, TextRow* rows, int maxRows) const;

    bool AddFallbackFont(AFont* fallbackFont);
    void ResetFallbackFonts();

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& _Stream) override;

    /** Create internal resource */
    void LoadInternalResource(AStringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Fonts/Default"; }

private:
    int                  m_FontId{};
    mutable TRef<AFontStash> m_FontStash;
    HeapBlob             m_Blob;
    TVector<TRef<AFont>> m_Fallbacks;
};

void GetTextMetrics(TextMetrics& metrics);