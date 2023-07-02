/*

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

#include "UIWidget.h"
#include <Engine/ECSRuntime/Resources/Resource_Font.h>

HK_NAMESPACE_BEGIN

class UIText : public UIObject
{
    UI_CLASS(UIText, UIObject)

public:
    // NOTE If you change the text in place, you must call ApplyTextChanges() after making the change.
    String Text;

    UIText() = default;

    UIText(StringView text, FontHandle font = {}, float fontSize = 14) :
        Text(text),
        m_Font(font),
        m_FontSize(fontSize)
    {}

    UIText& WithText(StringView text)
    {
        Text = text;
        return *this;
    }

    UIText& WithFont(FontHandle font)
    {
        m_Font = font;
        ApplyTextChanges();
        return *this;
    }

    UIText& WithFontSize(float fontSize)
    {
        m_FontSize = fontSize;
        ApplyTextChanges();
        return *this;
    }

    UIText& WithFontBlur(float fontBlur)
    {
        m_FontBlur = fontBlur;
        ApplyTextChanges();
        return *this;
    }

    UIText& WithLetterSpacing(float letterSpacing)
    {
        m_LetterSpacing = letterSpacing;
        ApplyTextChanges();
        return *this;
    }

    UIText& WithLineHeight(float lineHeight)
    {
        m_LineHeight = lineHeight;
        ApplyTextChanges();
        return *this;
    }

    UIText& WithAlignment(TEXT_ALIGNMENT_FLAGS alignment)
    {
        m_AlignmentFlags = alignment;
        return *this;
    }

    UIText& WithColor(Color4 const& color)
    {
        m_Color = color;
        return *this;
    }

    UIText& WithShadowOffset(Float2 const& shadowOffset)
    {
        m_ShadowOffset = shadowOffset;
        return *this;
    }

    UIText& WithShadowBlur(float shadowBlur)
    {
        m_ShadowBlur = shadowBlur;
        return *this;
    }

    UIText& WithWordWrap(bool wrap)
    {
        m_bWordWrap = wrap;
        return *this;
    }

    UIText& WithDropShadow(bool dropShadow)
    {
        m_bDropShadow = dropShadow;
        return *this;
    }

    bool IsWordWrapEnabled() const
    {
        return m_bWordWrap;
    }

    void ApplyTextChanges();

    Float2 GetTextBoxSize(float breakRowWidth) const;

    void Draw(Canvas& canvas, Float2 const& boxMins, Float2 const& boxMaxs);

private:
    FontHandle           m_Font;
    float                m_FontSize     = 14;
    float                m_FontBlur     = 0;
    float                m_LetterSpacing = 0;
    float                m_LineHeight    = 1; // The line height is specified as multiple of font size.
    TEXT_ALIGNMENT_FLAGS m_AlignmentFlags = TEXT_ALIGNMENT_LEFT | TEXT_ALIGNMENT_TOP;
    Color4               m_Color;
    Float2               m_ShadowOffset = Float2(2, 2);
    float                m_ShadowBlur   = 2;
    bool                 m_bWordWrap    = false;
    bool                 m_bDropShadow  = true;
    mutable Float2       m_CachedSize;
    mutable float        m_BreakRowWidth{-1};
};

HK_NAMESPACE_END
