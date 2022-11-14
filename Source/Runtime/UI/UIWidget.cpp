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

#include <Core/ConsoleVar.h>
#include <Geometry/BV/BvIntersect.h>
#include <Runtime/FrameLoop.h>

#include "UIWidget.h"
#include "UIScroll.h"
#include "UIManager.h"

AConsoleVar ui_showLayout("ui_showLayout"s, "0"s);

extern int UIVisibilityFrame;

UIWidget::UIWidget() :
    bAutoWidth{false},
    bAutoHeight{false},
    bTransparent{false},
    bDisabled{false},
    bExclusive{false},
    bNoInput{false},
    bStayBackground{false},
    bStayForeground{false},
    bPopup{false},
    bShortcutsAllowed{true},
    bAllowDrag{false}
{
    Cursor = GUIManager->ArrowCursor();
}

UIWidget::~UIWidget()
{
    for (UIWidget* widget : Children)
        widget->RemoveRef();
}

UIWidget* UIWidget::GetMaster()
{
    UIWidget* widget = this;
    while (widget->Parent)
        widget = widget->Parent;
    return widget;
}

void UIWidget::UpdateVisibility()
{
    VisFrame = UIVisibilityFrame;
}

UIWidget* UIWidget::Trace(float x, float y)
{
    if (HitTest(x, y))
    {
        if (BvPointInRect(Geometry.PaddedMins, Geometry.PaddedMaxs, x, y))
        {
            for (int i = (int)Children.Size() - 1; i >= 0; --i)
            {
                UIWidget* widget = Children[i]->Trace(x, y);
                if (widget)
                {
                    return widget;
                }
            }
        }

        if (!bTransparent)
            return this;
    }

    return {};
}

bool UIWidget::HitTest(float x, float y) const
{
    if (VisFrame != UIVisibilityFrame)
        return {};

    HK_ASSERT(Visibility == UI_WIDGET_VISIBILITY_VISIBLE);
    HK_ASSERT(Geometry.Mins.X < Geometry.Maxs.X && Geometry.Mins.Y < Geometry.Maxs.Y);

    if (!BvPointInRect(Geometry.Mins, Geometry.Maxs, x, y))
        return false;

    if (HitShape)
    {
        return HitShape->IsOverlap(Geometry, x, y);
    }
    return true;
}

void UIWidget::OnKeyEvent(SKeyEvent const& event, double timeStamp)
{
}

void UIWidget::OnMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
}

void UIWidget::OnDblClickEvent(int buttonKey, Float2 const& clickPos, uint64_t clickTime)
{
}

void UIWidget::ScrollSelfDelta(float delta)
{
    UIScroll* scroll = FindScrollWidget();
    if (scroll)
    {
        scroll->ScrollDelta(Float2(0.0f, delta));
    }
}

void UIWidget::OnMouseWheelEvent(SMouseWheelEvent const& event, double timeStamp)
{
    if (event.WheelY < 0)
    {
        ScrollSelfDelta(-20);
    }
    else if (event.WheelY > 0)
    {
        ScrollSelfDelta(20);
    }
}

void UIWidget::OnMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp)
{
}

void UIWidget::OnJoystickButtonEvent(SJoystickButtonEvent const& event, double timeStamp)
{
}

void UIWidget::OnJoystickAxisEvent(SJoystickAxisEvent const& event, double timeStamp)
{
}

void UIWidget::OnCharEvent(SCharEvent const& event, double timeStamp)
{
}

void UIWidget::OnDragEvent(Float2& position)
{
}

void UIWidget::OnFocusLost()
{
}

void UIWidget::OnFocusReceive()
{
}

void UIWidget::OnWindowHovered(bool bHovered)
{
    #if 0
    if (_Hovered)
    {
        Desktop->SetCursor(DRAW_CURSOR_ARROW);
    }

    E_OnHovered.Dispatch(_Hovered);

    if (_Hovered)
    {
        ShowTooltip();
    }
    else
    {
        HideTooltip();
    }
    #endif
}

void UIWidget::Draw(ACanvas& canvas)
{
}

void UIWidget::PostDraw(ACanvas& canvas)
{
}

void UIWidget::DrawBackground(ACanvas& canvas)
{
    if (Background)
        DrawBrush(canvas, Background);
}

void UIWidget::DrawForeground(ACanvas& canvas)
{
    if (Foreground)
        DrawBrush(canvas, Foreground);
}

void UIWidget::Draw(ACanvas& canvas, Float2 const& clipMins, Float2 const& clipMaxs, float alpha)
{
    if (VisFrame != UIVisibilityFrame)
        return;

    HK_ASSERT(Visibility == UI_WIDGET_VISIBILITY_VISIBLE);
    HK_ASSERT(Geometry.Mins.X < Geometry.Maxs.X && Geometry.Mins.Y < Geometry.Maxs.Y);

    Float2 cmins, cmaxs;

    cmins.X = Math::Max(Geometry.Mins.X, clipMins.X);
    cmins.Y = Math::Max(Geometry.Mins.Y, clipMins.Y);

    cmaxs.X = Math::Min(Geometry.Maxs.X, clipMaxs.X);
    cmaxs.Y = Math::Min(Geometry.Maxs.Y, clipMaxs.Y);

    if (cmins.X >= cmaxs.X || cmins.Y >= cmaxs.Y)
        return;

    alpha *= Opacity;

    canvas.Scissor(cmins, cmaxs);
    canvas.GlobalAlpha(alpha);

    DrawBackground(canvas);

    Draw(canvas);

    if (ui_showLayout)
    {
        canvas.DrawRect(Geometry.PaddedMins-0.5f, Geometry.PaddedMaxs+0.5f, Color4::Green(), 0.5f);
    }

    Float2 paddedMins, paddedMaxs;

    paddedMins.X = Math::Max(Geometry.PaddedMins.X, clipMins.X);
    paddedMins.Y = Math::Max(Geometry.PaddedMins.Y, clipMins.Y);

    paddedMaxs.X = Math::Min(Geometry.PaddedMaxs.X, clipMaxs.X);
    paddedMaxs.Y = Math::Min(Geometry.PaddedMaxs.Y, clipMaxs.Y);

    if (paddedMins.X >= paddedMaxs.X || paddedMins.Y >= paddedMaxs.Y)
        return;

    for (UIWidget* child : Children)
    {
        child->Draw(canvas, paddedMins, paddedMaxs, alpha);
    }

    canvas.Scissor(cmins, cmaxs);

    DrawForeground(canvas);

    PostDraw(canvas);
}

UIDesktop* UIWidget::GetDesktop()
{
    UIWidget* w = this;

    while (1)
    {
        if (!w->Parent)
            return w->Desktop;

        w = w->Parent;
    }
    return nullptr;
}

UIWidget& UIWidget::BringOnTop(bool bRecursiveForParents)
{
    if (!Parent && !Desktop)
        return *this;

    auto& widgets = Parent ? Parent->Children : Desktop->m_Widgets;

    if (!bStayBackground)
    {
        if (bStayForeground || bExclusive || bPopup)
        {
            if (bPopup)
            {
                // bring on top
                if (widgets.Last() != this)
                {
                    widgets.Remove(widgets.IndexOf(this));
                    widgets.Add(this);
                }
            }
            else
            {
                int i;

                // skip popup widgets
                for (i = (int)widgets.Size() - 1; i >= 0 && widgets[i]->bPopup; i--) {}

                // bring before popup widgets
                if (i >= 0 && widgets[i] != this)
                {
                    auto index = widgets.IndexOf(this);
                    widgets.Remove(index);
                    widgets.InsertAt(i, this);
                }
            }
        }
        else
        {
            int i;

            // skip foreground widgets
            for (i = (int)widgets.Size() - 1; i >= 0 && (widgets[i]->bStayForeground || widgets[i]->bExclusive || widgets[i]->bPopup); i--) {}

            // bring before foreground widgets
            if (i >= 0 && widgets[i] != this)
            {
                auto index = widgets.IndexOf(this);
                widgets.Remove(index);
                widgets.InsertAt(i, this);
            }
        }
    }

    if (bRecursiveForParents && Parent)
    {
        Parent->BringOnTop(bRecursiveForParents);
    }

    return *this;
}

UIWidget& UIWidget::AddWidget(UIWidget* widget)
{
    HK_ASSERT(widget);

    if (widget->Parent == this)
    {
        return *this;
    }

    if (widget->Parent)
    {
        UIWidget* oldParent = widget->Parent;

        oldParent->Children.Remove(oldParent->Children.IndexOf(widget));
        oldParent->LayoutSlots.Remove(oldParent->LayoutSlots.IndexOf(widget));
    }
    else
        widget->AddRef();

    widget->Parent = this;

    //widget->UpdateDesktop_r(_Parent->Desktop);

    Children.InsertAt(0, widget);

    widget->BringOnTop(false);

    LayoutSlots.Add(widget);

    return *this;
}

UIWidget& UIWidget::AddWidgets(std::initializer_list<UIWidget*> list)
{
    for (auto it : list)
        AddWidget(it);
    return *this;
}

void UIWidget::Detach()
{
    if (!Parent)
        return;

    //if (Desktop)
    //{
    //    LostFocus_r(Desktop);
    //    UpdateDesktop_r(nullptr);
    //}

    Parent->Children.Remove(Parent->Children.IndexOf(this));
    Parent->LayoutSlots.Remove(Parent->LayoutSlots.IndexOf(this));

    Parent = nullptr;

    RemoveRef();
}

void UIWidget::ForwardKeyEvent(SKeyEvent const& event, double timeStamp)
{
    OnKeyEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnKeyEvent(event, timeStamp);
    }
}

bool UIWidget::OnChildrenMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    return false;
}

bool UIWidget::OverrideMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    UIWidget* parent = Parent;
    if (!parent)
        return false;

    if (parent->OverrideMouseButtonEvent(event, timeStamp))
        return true;

    return parent->OnChildrenMouseButtonEvent(event, timeStamp);
}

void UIWidget::ForwardMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    if (OverrideMouseButtonEvent(event, timeStamp))
        return;

    OnMouseButtonEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnMouseButtonEvent(event, timeStamp);
    }
}

void UIWidget::ForwardDblClickEvent(int buttonKey, Float2 const& clickPos, uint64_t clickTime)
{
    OnDblClickEvent(buttonKey, clickPos, clickTime);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnDblClickEvent(buttonKey, clickPos, clickTime);
    }
}

void UIWidget::ForwardMouseWheelEvent(SMouseWheelEvent const& event, double timeStamp)
{
    OnMouseWheelEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnMouseWheelEvent(event, timeStamp);
    }
}

void UIWidget::ForwardMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp)
{
    OnMouseMoveEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnMouseMoveEvent(event, timeStamp);
    }
}

void UIWidget::ForwardJoystickButtonEvent(SJoystickButtonEvent const& event, double timeStamp)
{
    OnJoystickButtonEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnJoystickButtonEvent(event, timeStamp);
    }
}

void UIWidget::ForwardJoystickAxisEvent(SJoystickAxisEvent const& event, double timeStamp)
{
    OnJoystickAxisEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnJoystickAxisEvent(event, timeStamp);
    }
}

void UIWidget::ForwardCharEvent(SCharEvent const& event, double timeStamp)
{
    OnCharEvent(event, timeStamp);

    if (ShareInputs)
    {
        for (TWeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<TWeakRef<UIWidget>&>(w)->OnCharEvent(event, timeStamp);
    }
}

void UIWidget::ForwardDragEvent(Float2& position)
{
    OnDragEvent(position);
}

void UIWidget::ForwardFocusEvent(bool bFocus)
{
    if (bFocus)
        OnFocusReceive();
    else
        OnFocusLost();
}

void UIWidget::ForwardHoverEvent(bool bHovered)
{
    OnWindowHovered(bHovered);
}

Float2 UIWidget::MeasureLayout(bool bAllowAutoWidth, bool bAllowAutoHeight, Float2 const& size)
{
    if (!Layout)
        Layout = UINew(UIBoxLayout);

    AdjustSize(size);

    return Layout->MeasureLayout(this, bAutoWidth && bAllowAutoWidth, bAutoHeight && bAllowAutoHeight, size);
}

void UIWidget::ArrangeChildren(bool bAllowAutoWidth, bool bAllowAutoHeight)
{
    if (!Layout)
        Layout = UINew(UIBoxLayout);

    bool autoW = bAutoWidth && bAllowAutoWidth;
    bool autoH = bAutoHeight && bAllowAutoHeight;

    if (autoW)
        Geometry.Maxs.X = Geometry.Mins.X + MeasuredSize.X;

    if (autoH)
        Geometry.Maxs.Y = Geometry.Mins.Y + MeasuredSize.Y;

    Geometry.UpdatePadding(Padding);

    if (Visibility != UI_WIDGET_VISIBILITY_VISIBLE)
        return;

    if (Geometry.Mins.X >= Geometry.Maxs.X || Geometry.Mins.Y >= Geometry.Maxs.Y)
        return;

    UpdateVisibility();

    if (Geometry.IsTiny())
    {
        return;
    }

    Layout->ArrangeChildren(this, autoW, autoH);
}

void UIWidget::DrawBrush(ACanvas& canvas, UIBrush* brush)
{
    ::DrawBrush(canvas, Geometry.Mins, Geometry.Maxs, {}, brush);
}

UIShareInputs::UIShareInputs(std::initializer_list<UIWidget*> list)
{
    for (UIWidget* widget : list)
        Add(widget);
}

void UIShareInputs::Clear()
{
    m_Widgets.Clear();
}

UIShareInputs& UIShareInputs::Add(UIWidget* widget)
{
    for (auto& w : m_Widgets)
    {
        if (w && w->Id == widget->Id)
            return *this; // already in list
    }

    m_Widgets.Add() = widget;
    return *this;
}

UIShareInputs& UIShareInputs::Add(std::initializer_list<UIWidget*> list)
{
    for (UIWidget* widget : list)
        Add(widget);
    return *this;
}

UIScroll* UIWidget::FindScrollWidget()
{
    for (UIWidget* p = Parent; p; p = p->Parent)
    {
        UIScroll* scroll = dynamic_cast<UIScroll*>(p);
        if (scroll && scroll->CanScroll())
        {
            return scroll;
        }
    }
    return nullptr;
}
