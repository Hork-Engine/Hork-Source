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

#pragma once

#include "UIBrush.h"

HK_NAMESPACE_BEGIN

struct UIWidgetGeometry;

class UIDecorator : public UIObject
{
    UI_CLASS(UIDecorator, UIObject)

public:
    virtual void DrawInactive(Canvas& canvas, UIWidgetGeometry const& geometry) = 0;
    virtual void DrawActive(Canvas& canvas, UIWidgetGeometry const& geometry)   = 0;
    virtual void DrawHovered(Canvas& canvas, UIWidgetGeometry const& geometry)  = 0;
    virtual void DrawSelected(Canvas& canvas, UIWidgetGeometry const& geometry) = 0;
    virtual void DrawDisabled(Canvas& canvas, UIWidgetGeometry const& geometry) = 0;
};

class UIBrushDecorator : public UIDecorator
{
    UI_CLASS(UIBrushDecorator, UIObject)

public:
    Ref<UIBrush> InactiveBrush;
    Ref<UIBrush> ActiveBrush;
    Ref<UIBrush> HoverBrush;
    Ref<UIBrush> SelectedBrush;
    Ref<UIBrush> DisabledBrush;

    UIBrushDecorator& WithInactiveBrush(UIBrush* inactiveBrush)
    {
        InactiveBrush = inactiveBrush;
        return *this;
    }

    UIBrushDecorator& WithActiveBrush(UIBrush* activeBrush)
    {
        ActiveBrush = activeBrush;
        return *this;
    }

    UIBrushDecorator& WithHoverBrush(UIBrush* hoverBrush)
    {
        HoverBrush = hoverBrush;
        return *this;
    }

    UIBrushDecorator& WithSelectedBrush(UIBrush* selectedBrush)
    {
        SelectedBrush = selectedBrush;
        return *this;
    }

    UIBrushDecorator& WithDisabledBrush(UIBrush* disabledBrush)
    {
        DisabledBrush = disabledBrush;
        return *this;
    }
    
    void DrawInactive(Canvas& canvas, UIWidgetGeometry const& geometry) override;
    void DrawActive(Canvas& canvas, UIWidgetGeometry const& geometry) override;
    void DrawHovered(Canvas& canvas, UIWidgetGeometry const& geometry) override;
    void DrawSelected(Canvas& canvas, UIWidgetGeometry const& geometry) override;
    void DrawDisabled(Canvas& canvas, UIWidgetGeometry const& geometry) override;
};

HK_NAMESPACE_END
