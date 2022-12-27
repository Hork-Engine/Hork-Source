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

#include "UIObject.h"

HK_NAMESPACE_BEGIN

class UIWidget;

struct UIPadding
{
    float Left{};
    float Top{};
    float Right{};
    float Bottom{};

    UIPadding() = default;

    constexpr UIPadding(float padding) :
        Left(padding), Top(padding), Right(padding), Bottom(padding)
    {}

    constexpr UIPadding(float left, float top, float right, float bottom) :
        Left(left), Top(top), Right(right), Bottom(bottom)
    {}
};

struct UIWidgetGeometry
{
    Float2 Mins;
    Float2 Maxs;

    Float2 PaddedMins;
    Float2 PaddedMaxs;

    void UpdatePadding(UIPadding const& padding)
    {
        PaddedMins.X = Mins.X + padding.Left;
        PaddedMins.Y = Mins.Y + padding.Top;

        PaddedMaxs.X = Maxs.X - padding.Right;
        PaddedMaxs.Y = Maxs.Y - padding.Bottom;
    }

    bool IsTiny() const
    {
        return PaddedMins.X >= PaddedMaxs.X || PaddedMins.Y >= PaddedMaxs.Y;
    }
};

class UIBaseLayout : public UIObject
{
    UI_CLASS(UIBaseLayout, UIObject)

public:
    UIBaseLayout() = default;

protected:
    virtual Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
    {
        return Float2(0.0f);
    }

    virtual void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
    {
    }

    friend class UIWidget;
};

class UIBoxLayout : public UIBaseLayout
{
    UI_CLASS(UIBoxLayout, UIBaseLayout)

public:
    enum HORIZONTAL_ALIGNMENT
    {
        HALIGNMENT_NONE,
        HALIGNMENT_LEFT,
        HALIGNMENT_RIGHT,
        HALIGNMENT_CENTER,
        HALIGNMENT_STRETCH
    };

    enum VERTICAL_ALIGNMENT
    {
        VALIGNMENT_NONE,
        VALIGNMENT_TOP,
        VALIGNMENT_BOTTOM,
        VALIGNMENT_CENTER,
        VALIGNMENT_STRETCH
    };

    HORIZONTAL_ALIGNMENT HAlignment = HALIGNMENT_NONE;
    VERTICAL_ALIGNMENT   VAlignment = VALIGNMENT_NONE;

    UIBoxLayout(HORIZONTAL_ALIGNMENT halign = HALIGNMENT_NONE, VERTICAL_ALIGNMENT valign = VALIGNMENT_NONE) :
        HAlignment(halign), VAlignment(valign)
    {}

protected:
    Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

    void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;

private:
    Float2 MeasureHorizontal(Float2 const& size, UIWidget* widget);

    Float2 MeasureVertical(Float2 const& size, UIWidget* widget);

    float ArrangeHorizontal(UIWidgetGeometry const& layoutGeometry, UIWidget* widget);

    float ArrangeVertical(UIWidgetGeometry const& layoutGeometry, UIWidget* widget);
};

class UIGridLayout : public UIBaseLayout
{
    UI_CLASS(UIGridLayout, UIBaseLayout)

public:
    // Vector of column widths
    TVector<float> ColumnWidth;

    // Vector of row widths
    TVector<float> RowWidth;

    float VSpacing = 0;
    float HSpacing = 0;

    bool bNormalizedColumnWidth = false;
    bool bNormalizedRowWidth = false;

    bool bAutoAdjustRowSize{};

    UIGridLayout() = default;

    UIGridLayout& AddColumn(float width)
    {
        ColumnWidth.Add(width);
        return *this;
    }

    UIGridLayout& AddRow(float width)
    {
        RowWidth.Add(width);
        return *this;
    }

    UIGridLayout& WithVSpacing(float vspacing)
    {
        VSpacing = vspacing;
        return *this;
    }

    UIGridLayout& WithHSpacing(float hspacing)
    {
        HSpacing = hspacing;
        return *this;
    }

    UIGridLayout& WithNormalizedColumnWidth(bool normalizedColumnWidth)
    {
        bNormalizedColumnWidth = normalizedColumnWidth;
        return *this;
    }

    UIGridLayout& WithNormalizedRowWidth(bool normalizedRowWidth)
    {
        bNormalizedRowWidth = normalizedRowWidth;
        return *this;
    }

    UIGridLayout& WithAutoAdjustRowSize(bool autoAdjustRowSize)
    {
        bAutoAdjustRowSize = autoAdjustRowSize;
        return *this;
    }

protected:
    Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

    void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;

private:
    // Vector of column offsets
    TVector<float> ColumnOffset;

    // Vector of row offsets
    TVector<float> RowOffset;
};

class UIHorizontalLayout : public UIBaseLayout
{
    UI_CLASS(UIHorizontalLayout, UIBaseLayout)

public:
    float VSpacing{};
    float HSpacing{};

    bool bWrap{};
    bool bVStretch{};

    UIHorizontalLayout() = default;

    UIHorizontalLayout& WithVSpacing(float vspacing)
    {
        VSpacing = vspacing;
        return *this;
    }

    UIHorizontalLayout& WithHSpacing(float hspacing)
    {
        HSpacing = hspacing;
        return *this;
    }

    UIHorizontalLayout& WithWrap(bool wrap)
    {
        bWrap = wrap;
        return *this;
    }

    UIHorizontalLayout& WithVStretch(bool stretch)
    {
        bVStretch = stretch;
        return *this;
    }

protected:
    Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

    void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;
};

class UIVerticalLayout : public UIBaseLayout
{
    UI_CLASS(UIVerticalLayout, UIBaseLayout)

public:
    float VSpacing{};
    float HSpacing{};

    bool bWrap{};
    bool bHStretch{};

    UIVerticalLayout() = default;

    UIVerticalLayout& WithVSpacing(float vspacing)
    {
        VSpacing = vspacing;
        return *this;
    }

    UIVerticalLayout& WithHSpacing(float hspacing)
    {
        HSpacing = hspacing;
        return *this;
    }

    UIVerticalLayout& WithWrap(bool wrap)
    {
        bWrap = wrap;
        return *this;
    }

    UIVerticalLayout& WithHStretch(bool stretch)
    {
        bHStretch = stretch;
        return *this;
    }

protected:
    Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

    void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;
};

class UIImageLayout : public UIBaseLayout
{
    UI_CLASS(UIImageLayout, UIBaseLayout)

public:
    Float2 ImageSize = Float2(1920, 1080);

    UIImageLayout() = default;

    UIImageLayout(Float2 const& imageSize) :
        ImageSize(imageSize)
    {}

protected:
    Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

    void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;
};

class UIStackLayout : public UIBaseLayout
{
    UI_CLASS(UIStackLayout, UIBaseLayout)

public:
    UIStackLayout() = default;

protected:
    Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

    void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;
};

HK_NAMESPACE_END
