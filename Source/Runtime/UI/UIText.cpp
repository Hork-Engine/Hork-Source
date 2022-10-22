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

#include "UIText.h"

Float2 UIText::GetTextBoxSize(float breakRowWidth) const
{
    FontStyle style;
    style.FontSize      = FontSize;
    style.FontBlur      = FontBlur;
    style.LetterSpacing = LetterSpacing;
    style.LineHeight    = LineHeight;

    if (Font)
        return Font->GetTextBoxSize(style, breakRowWidth, Text);
    else
        return ACanvas::GetDefaultFont()->GetTextBoxSize(style, breakRowWidth, Text);
}

void UIText::Draw(ACanvas& canvas, Float2 const& boxMins, Float2 const& boxMaxs)
{
    FontStyle fontStyle;
    fontStyle.FontSize      = FontSize;
    fontStyle.LetterSpacing = LetterSpacing;
    fontStyle.LineHeight    = LineHeight;

    canvas.FontFace(Font);

    if (bDropShadow)
    {
        fontStyle.FontBlur = ShadowBlur;
        canvas.FillColor(Color4(0, 0, 0, Color.A));
        canvas.TextBox(fontStyle, boxMins + ShadowOffset, boxMaxs + ShadowOffset, HAlignment, VAlignment, bWrap, Text);
    }

    fontStyle.FontBlur = FontBlur;
    canvas.FillColor(Color);
    canvas.TextBox(fontStyle, boxMins, boxMaxs, HAlignment, VAlignment, bWrap, Text);
}
