/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "UIGrid.h"
#include "UIManager.h"
#include <Hork/Runtime/GameApplication/FrameLoop.h>
#include <Hork/Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

UIGrid::UIGrid(uint32_t NumColumns, uint32_t NumRows)
{
    auto gridLayout = UINew(UIGridLayout);

    gridLayout->ColumnWidth.Resize(NumColumns);
    gridLayout->RowWidth.Resize(NumRows);

    for (uint32_t i = 0; i < NumColumns; ++i)
        gridLayout->ColumnWidth[i] = 0;

    for (uint32_t i = 0; i < NumRows; ++i)
        gridLayout->RowWidth[i] = 0;

    Layout = gridLayout;
    m_Layout = gridLayout;
}

UIGridSplitter UIGrid::TraceSplitter(float x, float y) const
{
    UIGridSplitter result;

    float w = 4;

    uint32_t numColumns = m_Layout->ColumnWidth.Size();
    uint32_t numRows    = m_Layout->RowWidth.Size();

    float verticalSpacing   = m_Layout->VSpacing * (numRows - 1);
    float horizontalSpacing = m_Layout->HSpacing * (numColumns - 1);

    float sx = m_Layout->bNormalizedColumnWidth && !bAutoWidth ? Math::Max(0.0f, m_Geometry.PaddedMaxs.X - m_Geometry.PaddedMins.X - horizontalSpacing) : 1;
    float sy = m_Layout->bNormalizedRowWidth && !bAutoHeight ? Math::Max(0.0f, m_Geometry.PaddedMaxs.Y - m_Geometry.PaddedMins.Y - verticalSpacing) : 1;

    float offset = m_Geometry.PaddedMins.X;
    for (uint32_t col = 0; col < numColumns; ++col)
    {
        if (col > 0)
        {
            Float2 mins;
            Float2 maxs;

            mins.X = offset - w - m_Layout->HSpacing * 0.5f;
            maxs.X = offset + w - m_Layout->HSpacing * 0.5f;

            mins.Y = m_Geometry.PaddedMins.Y;
            maxs.Y = m_Geometry.PaddedMaxs.Y;

            if (BvPointInRect(mins, maxs, x, y))
            {
                result.Type  = UIGridSplitter::COLUMN;
                result.Index = col - 1;
                result.Mins  = mins;
                result.Maxs  = maxs;
                return result;
            }
        }

        offset += m_Layout->ColumnWidth[col] * sx + m_Layout->HSpacing;
    }

    offset = m_Geometry.PaddedMins.Y;
    for (uint32_t row = 0; row < numRows; ++row)
    {
        if (row > 0)
        {
            Float2 mins;
            Float2 maxs;

            mins.Y = offset - w - m_Layout->VSpacing * 0.5f;
            maxs.Y = offset + w - m_Layout->VSpacing * 0.5f;

            mins.X = m_Geometry.PaddedMins.X;
            maxs.X = m_Geometry.PaddedMaxs.X;

            if (BvPointInRect(mins, maxs, x, y))
            {
                result.Type  = UIGridSplitter::ROW;
                result.Index = row - 1;
                result.Mins  = mins;
                result.Maxs  = maxs;
                return result;
            }
        }

        offset += m_Layout->RowWidth[row] * sy + m_Layout->VSpacing;
    }

    return result;
}

void UIGrid::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (event.Button == VirtualKey::MouseLeftBtn)
    {
        if (event.Action == InputAction::Pressed)
        {
            auto const& cursorPosition = UIManager::sInstance().CursorPosition;

            m_Splitter  = UIGrid::TraceSplitter(cursorPosition.X, cursorPosition.Y);
            if (m_Splitter)
            {
                m_DragStart = cursorPosition;
                m_StartWidth = m_Layout->RowWidth[m_Splitter.Index]; // TODO also for columns
            }
        }
        else
            m_Splitter.Type = UIGridSplitter::UNDEFINED;
    }
}

void UIGrid::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    
}

void UIGrid::Draw(Canvas& cv)
{
    if (!bResizableCells)
        return;

    auto const& cursorPosition = UIManager::sInstance().CursorPosition;

    if (m_Splitter)
    {
        uint32_t numColumns = m_Layout->ColumnWidth.Size();
        uint32_t numRows    = m_Layout->RowWidth.Size();

        float verticalSpacing   = m_Layout->VSpacing * (numRows - 1);
        float horizontalSpacing = m_Layout->HSpacing * (numColumns - 1);

        float sx = m_Layout->bNormalizedColumnWidth && !bAutoWidth ? Math::Max(0.0f, m_Geometry.PaddedMaxs.X - m_Geometry.PaddedMins.X - horizontalSpacing) : 1;
        float sy = m_Layout->bNormalizedRowWidth && !bAutoHeight ? Math::Max(0.0f, m_Geometry.PaddedMaxs.Y - m_Geometry.PaddedMins.Y - verticalSpacing) : 1;

        HK_UNUSED(sx);

        //float p = m_DragStart.Y * sy;

        float p = cursorPosition.Y / sy;
        m_Layout->RowWidth[m_Splitter.Index] = p;
        m_Layout->RowWidth[m_Splitter.Index+1] = 1.0f - p;

        //float dragDelta = cursorPosition.Y - m_DragStart.Y;

        //cv.DrawRectFilled(m_Splitter.Mins, m_Splitter.Maxs, Color4::Orange());
    }
    else
    {
        auto splitter = UIGrid::TraceSplitter(cursorPosition.X, cursorPosition.Y);
        cv.DrawRectFilled(splitter.Mins, splitter.Maxs, Color4::sOrange());
    }
}

HK_NAMESPACE_END
