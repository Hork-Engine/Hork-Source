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

#pragma once

#include "UIWidget.h"

HK_NAMESPACE_BEGIN

class UIDockNode;
class UIDockContainer;

enum UI_DOCK_WIDGET_AREA
{
    UI_DOCK_WIDGET_AREA_LEFT   = 1,
    UI_DOCK_WIDGET_AREA_RIGHT  = 2,
    UI_DOCK_WIDGET_AREA_TOP    = 4,
    UI_DOCK_WIDGET_AREA_BOTTOM = 8,
    UI_DOCK_WIDGET_AREA_ALL    = UI_DOCK_WIDGET_AREA_LEFT | UI_DOCK_WIDGET_AREA_RIGHT | UI_DOCK_WIDGET_AREA_TOP | UI_DOCK_WIDGET_AREA_BOTTOM
};

HK_FLAG_ENUM_OPERATORS(UI_DOCK_WIDGET_AREA)

class UIDockWidget : public UIWidget
{
    UI_CLASS(UIDockWidget, UIWidget)

public:
    UI_DOCK_WIDGET_AREA DockAreas  = UI_DOCK_WIDGET_AREA_ALL; // TODO
    bool                bAllowTabs = true;                    // TODO

    UIDockWidget(UIDockContainer* container);

    UIDockContainer* GetContainer() { return m_Container; }

protected:
    void PostDraw(Canvas& canvas) override;

    bool OnChildrenMouseButtonEvent(struct MouseButtonEvent const& event, double timeStamp);

private:
    TWeakRef<UIDockNode> m_Leaf;
    uint64_t             m_ContainerId{};
    Float2               m_DockPosition;

public:
    Float2 m_DockSize;

private:
    TWeakRef<UIDockContainer> m_Container;

    friend class UIDockContainer;
    friend class UIDockNode;
};

HK_NAMESPACE_END
