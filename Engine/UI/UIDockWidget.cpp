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

#include "UIDockWidget.h"
#include "UIDockContainer.h"
#include "UIManager.h"

#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/GameApplication/FrameLoop.h>

HK_NAMESPACE_BEGIN

UIDockWidget::UIDockWidget(UIDockContainer* container) :
    m_Container(container)
{
    Padding = UIPadding(0);

    bAllowDrag = true;

    Layout = UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_STRETCH, UIBoxLayout::VALIGNMENT_STRETCH);
}

bool UIDockWidget::OnChildrenMouseButtonEvent(MouseButtonEvent const& event, double timeStamp)
{
    if (event.Button == 0 && event.Action == IA_PRESS)
    {
        UIDesktop* desktop = GetDesktop();
        if (desktop)
        {
            Float2 mins = Float2(m_Geometry.Maxs.X - 10, m_Geometry.Mins.Y);
            Float2 maxs = mins + 10.0f;

            Float2 p = GUIManager->CursorPosition;
            if (BvPointInRect(mins, maxs, p.X, p.Y))
            {
                desktop->SetDragWidget(this);
                return true;
            }
        }
    }
    return false;
}

void UIDockWidget::PostDraw(Canvas& canvas)
{
    Float2 mins = Float2(m_Geometry.Maxs.X - 10, m_Geometry.Mins.Y);
    Float2 maxs = mins + 10.0f;

    canvas.DrawTriangleFilled(mins,
                              Float2(maxs.X, mins.Y),
                              maxs,
                              Color4(0, 0, 0, 0.9f));
}

HK_NAMESPACE_END
