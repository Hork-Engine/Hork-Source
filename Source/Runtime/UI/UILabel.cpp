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

#include "UILabel.h"

void UILabel::AdjustSize(Float2 const& size)
{
    Super::AdjustSize(size);

    if (!Text)
        return;    

    bool autoW = bAutoWidth && !Text->IsWordWrapEnabled();
    bool autoH = bAutoHeight;

    if (!autoW && !autoH)
        return;

    float breakRowWidth;
    if (size.X > 0.0f)
        breakRowWidth = Math::Max(0.0f, size.X - Padding.Left - Padding.Right);
    else
        breakRowWidth = Math::MaxValue<float>();    

    Float2 boxSize = Text->GetTextBoxSize(breakRowWidth);

    if (autoW)
        m_AdjustedSize.X = boxSize.X;
    if (autoH)
        m_AdjustedSize.Y = boxSize.Y;
}

void UILabel::Draw(Canvas& canvas)
{
    if (Text)
        Text->Draw(canvas, m_Geometry.PaddedMins, m_Geometry.PaddedMaxs);
}
