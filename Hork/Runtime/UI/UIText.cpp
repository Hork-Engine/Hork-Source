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

#include "UIText.h"
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void UIText::ApplyTextChanges()
{
    m_BreakRowWidth = -1;
}

Float2 UIText::GetTextBoxSize(float breakRowWidth) const
{
    if (m_BreakRowWidth != breakRowWidth)
    {
        FontStyle style;
        style.FontSize      = m_FontSize;
        style.FontBlur      = m_FontBlur;
        style.LetterSpacing = m_LetterSpacing;
        style.LineHeight    = m_LineHeight;

        Canvas& canvas = UIManager::sInstance().GetCanvas();
        canvas.FontFace(m_Font);

        m_CachedSize = canvas.GetTextBoxSize(style, breakRowWidth, Text);

        // Update m_BreakRowWidth only if font is valid.
        if (canvas.IsValidFont(m_Font))
            m_BreakRowWidth = breakRowWidth;
    }
    return m_CachedSize;
}

void UIText::Draw(Canvas& canvas, Float2 const& boxMins, Float2 const& boxMaxs)
{
    FontStyle fontStyle;
    fontStyle.FontSize      = m_FontSize;
    fontStyle.LetterSpacing = m_LetterSpacing;
    fontStyle.LineHeight    = m_LineHeight;

    canvas.FontFace(m_Font);

    if (m_bDropShadow)
    {
        fontStyle.FontBlur = m_ShadowBlur;
        canvas.FillColor(Color4(0, 0, 0, m_Color.A));
        canvas.TextBox(fontStyle, boxMins + m_ShadowOffset, boxMaxs + m_ShadowOffset, m_AlignmentFlags, m_bWordWrap, Text);
    }

    fontStyle.FontBlur = m_FontBlur;
    canvas.FillColor(m_Color);
    canvas.TextBox(fontStyle, boxMins, boxMaxs, m_AlignmentFlags, m_bWordWrap, Text);
}

HK_NAMESPACE_END
