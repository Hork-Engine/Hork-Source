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

#include "UILayout.h"
#include "UIWidget.h"

HK_NAMESPACE_BEGIN

Float2 UIBoxLayout::MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    Float2 layoutSize = self->m_AdjustedSize;

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        Float2 horizontal = MeasureHorizontal(paddedSize, child);
        Float2 vertical   = MeasureVertical(paddedSize, child);

        Float2 requiredSize = child->MeasureLayout(HAlignment != HALIGNMENT_STRETCH, VAlignment != VALIGNMENT_STRETCH, {horizontal[1], vertical[1]});

        requiredSize.X += horizontal[0];
        requiredSize.Y += vertical[0];

        layoutSize.X = Math::Max(layoutSize.X, requiredSize.X);
        layoutSize.Y = Math::Max(layoutSize.Y, requiredSize.Y);
    }

    return layoutSize;
}

void UIBoxLayout::ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
{
    UIWidgetGeometry& geometry = self->m_Geometry;

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        ArrangeHorizontal(geometry, child);
        ArrangeVertical(geometry, child);

        if ((!bAutoWidth && child->m_Geometry.Mins.X >= geometry.PaddedMaxs.X) || (!bAutoHeight && child->m_Geometry.Mins.Y >= geometry.PaddedMaxs.Y))
            continue;

        child->ArrangeChildren(HAlignment != HALIGNMENT_STRETCH, VAlignment != VALIGNMENT_STRETCH);
    }
}

Float2 UIBoxLayout::MeasureHorizontal(Float2 const& size, UIWidget* widget)
{
    switch (HAlignment)
    {
        case HALIGNMENT_NONE:
            return {widget->Position.X, widget->Size.X};
        case HALIGNMENT_LEFT:
        case HALIGNMENT_RIGHT:
        case HALIGNMENT_CENTER:
            return {0.0f, widget->Size.X};
        case HALIGNMENT_STRETCH:
            return {0.0f, size.X};
    }
    HK_ASSERT(0);
    return {0.0f, 0.0f};
}

Float2 UIBoxLayout::MeasureVertical(Float2 const& size, UIWidget* widget)
{
    switch (VAlignment)
    {
        case VALIGNMENT_NONE:
            return {widget->Position.Y, widget->Size.Y};
        case VALIGNMENT_TOP:
        case VALIGNMENT_BOTTOM:
        case VALIGNMENT_CENTER:
            return {0.0f, widget->Size.Y};
        case VALIGNMENT_STRETCH:
            return {0.0f, size.Y};
    }
    HK_ASSERT(0);
    return {0.0f, 0.0f};
}

float UIBoxLayout::ArrangeHorizontal(UIWidgetGeometry const& layoutGeometry, UIWidget* widget)
{
    switch (HAlignment)
    {
        case HALIGNMENT_NONE:
            widget->m_Geometry.Mins.X = layoutGeometry.PaddedMins.X + widget->Position.X;
            break;
        case HALIGNMENT_LEFT:
            widget->m_Geometry.Mins.X = layoutGeometry.PaddedMins.X;
            break;
        case HALIGNMENT_RIGHT:
            widget->m_Geometry.Mins.X = layoutGeometry.PaddedMaxs.X - widget->m_MeasuredSize.X;
            break;
        case HALIGNMENT_CENTER:
            widget->m_Geometry.Mins.X = layoutGeometry.PaddedMins.X;
            widget->m_Geometry.Mins.X += (layoutGeometry.PaddedMaxs.X - layoutGeometry.PaddedMins.X - widget->m_MeasuredSize.X) * 0.5f;
            break;
        case HALIGNMENT_STRETCH:
            widget->m_Geometry.Mins.X = layoutGeometry.PaddedMins.X;
            break;
        default:
            HK_ASSERT(0);
    }

    if (HAlignment == HALIGNMENT_STRETCH)
    {
        widget->m_Geometry.Maxs.X = layoutGeometry.PaddedMaxs.X;
    }
    else
    {
        widget->m_Geometry.Maxs.X = widget->m_Geometry.Mins.X + widget->m_MeasuredSize.X;
    }

    return 0.0f;
}

float UIBoxLayout::ArrangeVertical(UIWidgetGeometry const& layoutGeometry, UIWidget* widget)
{
    switch (VAlignment)
    {
        case VALIGNMENT_NONE:
            widget->m_Geometry.Mins.Y = layoutGeometry.PaddedMins.Y + widget->Position.Y;
            break;
        case VALIGNMENT_TOP:
            widget->m_Geometry.Mins.Y = layoutGeometry.PaddedMins.Y;
            break;
        case VALIGNMENT_BOTTOM:
            widget->m_Geometry.Mins.Y = layoutGeometry.PaddedMaxs.Y - widget->m_MeasuredSize.Y;
            break;
        case VALIGNMENT_CENTER:
            widget->m_Geometry.Mins.Y = layoutGeometry.PaddedMins.Y;
            widget->m_Geometry.Mins.Y += (layoutGeometry.PaddedMaxs.Y - layoutGeometry.PaddedMins.Y - widget->m_MeasuredSize.Y) * 0.5f;
            break;
        case VALIGNMENT_STRETCH:
            widget->m_Geometry.Mins.Y = layoutGeometry.PaddedMins.Y;
            break;
        default:
            HK_ASSERT(0);
    }

    if (VAlignment == VALIGNMENT_STRETCH)
    {
        widget->m_Geometry.Maxs.Y = layoutGeometry.PaddedMaxs.Y;
    }
    else
    {
        widget->m_Geometry.Maxs.Y = widget->m_Geometry.Mins.Y + widget->m_MeasuredSize.Y;
    }

    return 0.0f;
}

Float2 UIGridLayout::MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    Float2 layoutSize = self->m_AdjustedSize;

    uint32_t numColumns = ColumnWidth.Size();
    uint32_t numRows    = RowWidth.Size();

    if (numColumns > 0 && numRows > 0)
    {
        float verticalSpacing   = VSpacing * (numRows - 1);
        float horizontalSpacing = HSpacing * (numColumns - 1);

        float sx = bNormalizedColumnWidth && !bAutoWidth ? Math::Max(0.0f, paddedSize.X - horizontalSpacing) : 1;
        float sy = bNormalizedRowWidth && !bAutoHeight ? Math::Max(0.0f, paddedSize.Y - verticalSpacing) : 1;

        float width = 0;
        for (uint32_t col = 0; col < numColumns; ++col)
        {
            width += ColumnWidth[col] * sx;
        }
        width += (numColumns - 1) * horizontalSpacing;

        float height = 0;
        for (uint32_t row = 0; row < numRows; ++row)
        {
            height += RowWidth[row] * sy;
        }
        height += (numRows - 1) * verticalSpacing;

        layoutSize.X = Math::Max(layoutSize.X, width);
        layoutSize.Y = Math::Max(layoutSize.Y, height);

        for (UIWidget* child : self->m_LayoutSlots)
        {
            if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
                continue;

            if (child->GridOffset.ColumnIndex >= numColumns ||
                child->GridOffset.RowIndex >= numRows)
                continue;

            float w = ColumnWidth[child->GridOffset.ColumnIndex] * sx;
            float h = RowWidth[child->GridOffset.RowIndex] * sy;

            if (w <= 0.0f || h <= 0.0f)
                continue;

            child->MeasureLayout(false, false, {w, h});
        }
    }

    return layoutSize;
}

void UIGridLayout::ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
{
    UIWidgetGeometry& geometry = self->m_Geometry;

    uint32_t numColumns = ColumnWidth.Size();
    uint32_t numRows    = RowWidth.Size();

    ColumnOffset.Resize(numColumns);
    RowOffset.Resize(numRows);

    float verticalSpacing   = VSpacing * (numRows - 1);
    float horizontalSpacing = HSpacing * (numColumns - 1);

    float sx = bNormalizedColumnWidth && !bAutoWidth ? Math::Max(0.0f, geometry.PaddedMaxs.X - geometry.PaddedMins.X - horizontalSpacing) : 1;
    float sy = bNormalizedRowWidth && !bAutoHeight ? Math::Max(0.0f, geometry.PaddedMaxs.Y - geometry.PaddedMins.Y - verticalSpacing) : 1;

    float offset = geometry.PaddedMins.X;
    for (uint32_t col = 0; col < numColumns; ++col)
    {
        ColumnOffset[col] = offset;
        offset += ColumnWidth[col] * sx + HSpacing;
    }

    offset = geometry.PaddedMins.Y;
    for (uint32_t row = 0; row < numRows; ++row)
    {
        RowOffset[row] = offset;
        offset += RowWidth[row] * sy + VSpacing;
    }

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        if (child->GridOffset.ColumnIndex >= numColumns ||
            child->GridOffset.RowIndex >= numRows)
            continue;

        child->m_Geometry.Mins.X = ColumnOffset[child->GridOffset.ColumnIndex];
        child->m_Geometry.Mins.Y = RowOffset[child->GridOffset.RowIndex];
        child->m_Geometry.Maxs   = child->m_Geometry.Mins + child->m_MeasuredSize;

        child->ArrangeChildren(false, false);
    }
}

Float2 UIHorizontalLayout::MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    Float2 layoutSize = self->m_AdjustedSize;

    float x          = 0;
    float y          = 0;
    float lineHeight = 0;

    bool canWrap = bWrap && !bAutoWidth;
    bool stretch = !bWrap && bVStretch;

    // Pre pass: calculate max width
    float maxHeight = self->m_AdjustedSize.Y;
    if (stretch)
    {
        for (UIWidget* child : self->m_LayoutSlots)
        {
            if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
                continue;

            float w = child->Size.X;
            float h = child->Size.Y;

            Float2 requiredSize = child->MeasureLayout(!canWrap, true, {w, h});

            maxHeight = Math::Max(maxHeight, requiredSize.Y);
        }
    }

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        float w = child->Size.X;
        float h = stretch ? maxHeight : child->Size.Y;

        Float2 requiredSize = child->MeasureLayout(!canWrap, !stretch, {w, h});

        if (canWrap && x + requiredSize.X >= paddedSize.X && x > 0)
        {
            x = 0;
            y += lineHeight + VSpacing;

            lineHeight = requiredSize.Y;
        }
        else
        {
            requiredSize.X += x;

            x = requiredSize.X + HSpacing;

            lineHeight = Math::Max(lineHeight, requiredSize.Y);
        }

        //requiredSize.X += x;
        requiredSize.Y += y;

        layoutSize.X = Math::Max(layoutSize.X, requiredSize.X);
        layoutSize.Y = Math::Max(layoutSize.Y, requiredSize.Y);
    }

    return layoutSize;
}

void UIHorizontalLayout::ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
{
    UIWidgetGeometry& geometry = self->m_Geometry;

    float x          = geometry.PaddedMins.X;
    float y          = geometry.PaddedMins.Y;
    float lineHeight = 0;

    bool canWrap = bWrap && !bAutoWidth;

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        if (canWrap && x + child->m_MeasuredSize.X >= geometry.PaddedMaxs.X && x > geometry.PaddedMins.X)
        {
            x = geometry.PaddedMins.X;
            y += lineHeight + VSpacing;
            lineHeight = 0;
        }

        if ((!bAutoWidth && x >= geometry.PaddedMaxs.X) || (!bAutoHeight && y >= geometry.PaddedMaxs.Y))
            break; //continue;

        child->m_Geometry.Mins.X = x;
        child->m_Geometry.Mins.Y = y;
        child->m_Geometry.Maxs   = child->m_Geometry.Mins + child->m_MeasuredSize;

        child->ArrangeChildren(!canWrap, true);

        Float2 size = child->m_Geometry.Maxs - child->m_Geometry.Mins;

        x += size.X + HSpacing;
        lineHeight = Math::Max(lineHeight, size.Y);
    }
}

Float2 UIVerticalLayout::MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    Float2 layoutSize = self->m_AdjustedSize;

    float x         = 0;
    float y         = 0;
    float lineWidth = 0;

    bool canWrap = bWrap && !bAutoHeight;
    bool stretch = !bWrap && bHStretch;

    // Pre pass: calculate max width
    float maxWidth = self->m_AdjustedSize.X;
    if (stretch)
    {
        for (UIWidget* child : self->m_LayoutSlots)
        {
            if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
                continue;

            float w = child->Size.X;
            float h = child->Size.Y;

            Float2 requiredSize = child->MeasureLayout(true, !canWrap, {w, h});

            maxWidth = Math::Max(maxWidth, requiredSize.X);
        }
    }

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        float w = stretch ? maxWidth : child->Size.X;
        float h = child->Size.Y;

        Float2 requiredSize = child->MeasureLayout(!stretch, !canWrap, {w, h});

        if (canWrap && y + requiredSize.Y >= paddedSize.Y && y > 0)
        {
            y = 0;
            x += lineWidth + HSpacing;

            lineWidth = requiredSize.X;
        }
        else
        {
            requiredSize.Y += y;

            y = requiredSize.Y + VSpacing;

            lineWidth = Math::Max(lineWidth, requiredSize.X);
        }

        requiredSize.X += x;
        //requiredSize.Y += y;

        layoutSize.X = Math::Max(layoutSize.X, requiredSize.X);
        layoutSize.Y = Math::Max(layoutSize.Y, requiredSize.Y);
    }

    return layoutSize;
}

void UIVerticalLayout::ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
{
    UIWidgetGeometry& geometry = self->m_Geometry;

    float x          = geometry.PaddedMins.X;
    float y          = geometry.PaddedMins.Y;
    float lineWidth  = 0;

    bool canWrap = bWrap && !bAutoHeight;

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        if (canWrap && y + child->m_MeasuredSize.Y >= geometry.PaddedMaxs.Y && y > geometry.PaddedMins.Y)
        {
            y = geometry.PaddedMins.Y;
            x += lineWidth + HSpacing;
            lineWidth = 0;
        }

        if ((!bAutoWidth && x >= geometry.PaddedMaxs.X) || (!bAutoHeight && y >= geometry.PaddedMaxs.Y))
            break; //continue;

        child->m_Geometry.Mins.X = x;
        child->m_Geometry.Mins.Y = y;
        child->m_Geometry.Maxs   = child->m_Geometry.Mins + child->m_MeasuredSize;

        child->ArrangeChildren(true, !canWrap);

        Float2 size = child->m_Geometry.Maxs - child->m_Geometry.Mins;

        y += size.Y + VSpacing;
        lineWidth = Math::Max(lineWidth, size.X);
    }
}

Float2 UIImageLayout::MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
{
    if (ImageSize.X <= 0 || ImageSize.Y <= 0)
    {
        //self->m_MeasuredSize = Float2(0.0f);
        return Float2(0.0f);
    }

    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    Float2 scale = paddedSize / ImageSize;

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        if (child->Position.X >= ImageSize.X || child->Position.Y >= ImageSize.Y)
            continue;

        if (child->Position.X + child->Size.X < 0.0f || child->Position.Y + child->Size.Y < 0.0f)
            continue;

        float w = child->Size.X * scale.X;
        float h = child->Size.Y * scale.Y;

        child->MeasureLayout(true, true, {w, h});
    }

    return paddedSize;
}

void UIImageLayout::ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
{
    UIWidgetGeometry& geometry = self->m_Geometry;

    if (ImageSize.X <= 0 || ImageSize.Y <= 0)
        return;

    Float2 scale = (geometry.PaddedMaxs - geometry.PaddedMins) / ImageSize;

    for (UIWidget* child : self->m_LayoutSlots)
    {
        if (child->Visibility == UI_WIDGET_VISIBILITY_COLLAPSED)
            continue;

        if (child->Position.X >= ImageSize.X || child->Position.Y >= ImageSize.Y)
            continue;

        if (child->Position.X + child->Size.X < 0.0f || child->Position.Y + child->Size.Y < 0.0f)
            continue;

        child->m_Geometry.Mins = geometry.PaddedMins + child->Position * scale;
        child->m_Geometry.Maxs = child->m_Geometry.Mins + child->Size * scale;

        child->ArrangeChildren(true, true);
    }
}

Float2 UIStackLayout::MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    if (self->Layer < 0 || self->Layer >= self->m_LayoutSlots.Size())
        return paddedSize;

    UIWidget* widget = self->m_LayoutSlots[self->Layer];

    if (widget->Visibility != UI_WIDGET_VISIBILITY_VISIBLE)
        return paddedSize;

    widget->MeasureLayout(false, false, {paddedSize.X, paddedSize.Y});

    return paddedSize;
}

void UIStackLayout::ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight)
{
    if (self->Layer < 0 || self->Layer >= self->m_LayoutSlots.Size())
        return;

    UIWidget* widget = self->m_LayoutSlots[self->Layer];

    if (widget->Visibility != UI_WIDGET_VISIBILITY_VISIBLE)
        return;

    UIWidgetGeometry const& geometry = self->m_Geometry;

    widget->m_Geometry.Mins = geometry.PaddedMins;
    widget->m_Geometry.Maxs = geometry.PaddedMaxs;

    widget->ArrangeChildren(false, false);
}

HK_NAMESPACE_END
