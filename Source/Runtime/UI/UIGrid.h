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

struct UIGridSplitter
{
    enum TYPE
    {
        UNDEFINED,
        COLUMN,
        ROW
    };

    TYPE     Type{UNDEFINED};
    uint32_t Index{};
    Float2   Mins;
    Float2   Maxs;

    operator bool() const
    {
        return Type != UNDEFINED;
    }
};

class UIGrid : public UIWidget
{
    UI_CLASS(UIGrid, UIWidget)

public:
    bool bResizableCells = true;

    UIGrid(uint32_t NumColumns, uint32_t NumRows);

    UIGridSplitter TraceSplitter(float x, float y) const;

protected:
    void OnMouseButtonEvent(struct SMouseButtonEvent const& event, double timeStamp) override;

    void OnMouseMoveEvent(struct SMouseMoveEvent const& event, double timeStamp) override;

    void Draw(ACanvas& cv) override;

private:
    TRef<UIGridLayout> m_Layout;
    UIGridSplitter     m_Splitter;
    Float2             m_DragStart;
    float              m_StartWidth;
};
