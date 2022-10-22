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

#include "UIWidget.h"

class UIImage : public UIWidget
{
    UI_CLASS(UIImage, UIWidget)

public:
    TRef<ATexture>   Texture;
    Color4           TintColor;
    RoundingDesc     Rounding;
    CANVAS_COMPOSITE Composite = CANVAS_COMPOSITE_SOURCE_OVER;
    bool             bStretchedX{};
    bool             bStretchedY{};
    bool             bTiledX{};
    bool             bTiledY{};
    bool             bFlipY{};
    bool             bAlphaPremultiplied{};
    bool             bNearestFilter{};
    Float2           Scale = Float2(1, 1);
    Float2           Offset;

    UIImage()
    {}

    void AdjustSize(Float2 const& size) override;

    void Draw(ACanvas& canvas) override;
};
