﻿/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

struct FONScontext;

HK_NAMESPACE_BEGIN

class FontStash : public RefCounted
{
public:
    FontStash();
    ~FontStash();

    struct FONScontext* GetImpl()
    {
        return m_pImpl;
    }

    bool ReallocTexture();
    void UpdateTexture();

    TextureView* GetTextureView();

    void Cleanup();

private:
    enum
    {
        MAX_FONT_IMAGES = 4,
        MAX_FONTIMAGE_SIZE = 2048,
        INITIAL_FONTIMAGE_SIZE = 512
    };
    FONScontext*               m_pImpl{};
    TRef<RenderCore::ITexture> m_FontImages[MAX_FONT_IMAGES];
    int                        m_FontImageIdx{};

    class TextureViewImpl : public TextureView
    {
    public:
        TextureViewImpl(FontStash* pFontStash) :
            m_FontStash(pFontStash)
        {}

        void SetResource(RenderCore::ITexture* pResource)
        {
            m_Resource = pResource;
            m_Width    = pResource->GetDesc().Resolution.Width;
            m_Height   = pResource->GetDesc().Resolution.Height;
        }

        TRef<FontStash> m_FontStash;
    };

    TWeakRef<TextureViewImpl> m_TextureViews[MAX_FONT_IMAGES];
};

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

    /** Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending. */
    float MinX, MaxX;

    StringView GetStringView() const { return StringView(Start, End); }
};

struct TextRowW
{
    /** Pointer to the input text where the row starts. */
    const WideChar* Start;

    /** Pointer to the input text where the row ends(one past the last character). */
    const WideChar* End;

    /** Pointer to the beginning of the next row. */
    const WideChar* Next;

    /** Logical width of the row. */
    float Width;

    /** Actual bounds of the row. Logical width and bounds can differ because of kerning and some parts over extending. */
    float MinX, MaxX;

    WideStringView GetStringView() const { return WideStringView(Start, End); }
};

struct FontStyle
{
    float FontSize{14};

    // Font blur allows you to create simple text effects such as drop shadows.
    float FontBlur{0};

    /** Letter spacing. */
    float LetterSpacing{0};

    /** Proportional line height. The line height is specified as multiple of font size. */
    float LineHeight{1};
};

class Font : public Resource
{
    HK_CLASS(Font, Resource)

public:
    Font();
    ~Font();

    int GetId() const
    {
        return m_FontId;
    }

    /** Returns the vertical metrics based on the current text style. */
    void GetTextMetrics(FontStyle const& fontStyle, TextMetrics& metrics) const;

    float GetCharAdvance(FontStyle const& fontStyle, WideChar ch) const;

    /** Measures the size of specified multi-text string */
    Float2 GetTextBoxSize(FontStyle const& fontStyle, float breakRowWidth, StringView text, bool bKeepSpaces = false) const;
    Float2 GetTextBoxSize(FontStyle const& fontStyle, float breakRowWidth, WideStringView text, bool bKeepSpaces = false) const;

    /** Breaks the specified text into lines.
    White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
    Words longer than the max width are slit at nearest character (i.e. no hyphenation). */
    int TextBreakLines(FontStyle const& fontStyle, StringView text, float breakRowWidth, TextRow* rows, int maxRows, bool bKeepSpaces = false) const;
    int TextBreakLines(FontStyle const& fontStyle, WideStringView text, float breakRowWidth, TextRowW* rows, int maxRows, bool bKeepSpaces = false) const;

    int TextLineCount(FontStyle const& fontStyle, StringView text, float breakRowWidth, bool bKeepSpaces = false) const;
    int TextLineCount(FontStyle const& fontStyle, WideStringView text, float breakRowWidth, bool bKeepSpaces = false) const;

    bool AddFallbackFont(Font* fallbackFont);
    void ResetFallbackFonts();

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& _Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Fonts/Default"; }

private:
    int                 m_FontId = -1;
    mutable TRef<FontStash> m_FontStash;
    HeapBlob            m_Blob;
    TVector<TRef<Font>> m_Fallbacks;
};

HK_NAMESPACE_END
