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

#include <Engine/Core/ConsoleVar.h>
#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/GameApplication/FrameLoop.h>

#include "UIWidget.h"
#include "UIScroll.h"
#include "UIManager.h"

HK_NAMESPACE_BEGIN

ConsoleVar ui_showLayout("ui_showLayout"_s, "0"_s);

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
    for (UIWidget* widget : m_Children)
        widget->RemoveRef();
}

UIWidget* UIWidget::GetMaster()
{
    UIWidget* widget = this;
    while (widget->m_Parent)
        widget = widget->m_Parent;
    return widget;
}

UIWidget* UIWidget::GetParent()
{
    return m_Parent;
}

void UIWidget::UpdateVisibility()
{
    m_VisFrame = UIVisibilityFrame;
}

UIWidget* UIWidget::Trace(float x, float y)
{
    if (HitTest(x, y))
    {
        if (BvPointInRect(m_Geometry.PaddedMins, m_Geometry.PaddedMaxs, x, y))
        {
            for (int i = (int)m_Children.Size() - 1; i >= 0; --i)
            {
                UIWidget* widget = m_Children[i]->Trace(x, y);
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
    if (m_VisFrame != UIVisibilityFrame)
        return {};

    HK_ASSERT(Visibility == UI_WIDGET_VISIBILITY_VISIBLE);
    HK_ASSERT(m_Geometry.Mins.X < m_Geometry.Maxs.X && m_Geometry.Mins.Y < m_Geometry.Maxs.Y);

    if (!BvPointInRect(m_Geometry.Mins, m_Geometry.Maxs, x, y))
        return false;

    if (HitShape)
    {
        return HitShape->IsOverlap(m_Geometry, x, y);
    }
    return true;
}

void UIWidget::OnKeyEvent(KeyEvent const& event)
{
}

void UIWidget::OnMouseButtonEvent(MouseButtonEvent const& event)
{
}

void UIWidget::OnDblClickEvent(VirtualKey buttonKey, Float2 const& clickPos, uint64_t clickTime)
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

void UIWidget::OnMouseWheelEvent(MouseWheelEvent const& event)
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

void UIWidget::OnMouseMoveEvent(MouseMoveEvent const& event)
{
}

void UIWidget::OnGamepadButtonEvent(GamepadKeyEvent const& event)
{
}

void UIWidget::OnGamepadAxisMotionEvent(GamepadAxisMotionEvent const& event)
{
}

void UIWidget::OnCharEvent(CharEvent const& event)
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
    E_OnHovered.Invoke(bHovered);
}

void UIWidget::Draw(Canvas& canvas)
{
}

void UIWidget::PostDraw(Canvas& canvas)
{
}

void UIWidget::DrawBackground(Canvas& canvas)
{
    if (Background)
        DrawBrush(canvas, Background);
}

void UIWidget::DrawForeground(Canvas& canvas)
{
    if (Foreground)
        DrawBrush(canvas, Foreground);
}

void UIWidget::Draw(Canvas& canvas, Float2 const& clipMins, Float2 const& clipMaxs, float alpha)
{
    if (m_VisFrame != UIVisibilityFrame)
        return;

    HK_ASSERT(Visibility == UI_WIDGET_VISIBILITY_VISIBLE);
    HK_ASSERT(m_Geometry.Mins.X < m_Geometry.Maxs.X && m_Geometry.Mins.Y < m_Geometry.Maxs.Y);

    Float2 cmins, cmaxs;

    cmins.X = Math::Max(m_Geometry.Mins.X, clipMins.X);
    cmins.Y = Math::Max(m_Geometry.Mins.Y, clipMins.Y);

    cmaxs.X = Math::Min(m_Geometry.Maxs.X, clipMaxs.X);
    cmaxs.Y = Math::Min(m_Geometry.Maxs.Y, clipMaxs.Y);

    if (cmins.X >= cmaxs.X || cmins.Y >= cmaxs.Y)
        return;

    alpha *= Opacity;

    canvas.Scissor(cmins, cmaxs);
    canvas.GlobalAlpha(alpha);

    DrawBackground(canvas);

    Draw(canvas);

    if (ui_showLayout)
    {
        canvas.DrawRect(m_Geometry.PaddedMins - 0.5f, m_Geometry.PaddedMaxs + 0.5f, Color4::sGreen(), 0.5f);
    }

    Float2 paddedMins, paddedMaxs;

    paddedMins.X = Math::Max(m_Geometry.PaddedMins.X, clipMins.X);
    paddedMins.Y = Math::Max(m_Geometry.PaddedMins.Y, clipMins.Y);

    paddedMaxs.X = Math::Min(m_Geometry.PaddedMaxs.X, clipMaxs.X);
    paddedMaxs.Y = Math::Min(m_Geometry.PaddedMaxs.Y, clipMaxs.Y);

    if (paddedMins.X >= paddedMaxs.X || paddedMins.Y >= paddedMaxs.Y)
        return;

    for (UIWidget* child : m_Children)
    {
        child->Draw(canvas, paddedMins, paddedMaxs, alpha);
    }

    canvas.Scissor(cmins, cmaxs);

    DrawForeground(canvas);

    PostDraw(canvas);
}

UIDesktop* UIWidget::GetDesktop() const
{
    UIWidget* w = const_cast<UIWidget*>(this);

    while (1)
    {
        if (!w->m_Parent)
            return w->m_Desktop;

        w = w->m_Parent;
    }
    return nullptr;
}

UIWidget& UIWidget::BringOnTop(bool bRecursiveForParents)
{
    if (!m_Parent && !m_Desktop)
        return *this;

    auto& widgets = m_Parent ? m_Parent->m_Children : m_Desktop->m_Widgets;

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

    if (bRecursiveForParents && m_Parent)
    {
        m_Parent->BringOnTop(bRecursiveForParents);
    }

    return *this;
}

UIWidget& UIWidget::AddWidget(UIWidget* widget)
{
    HK_ASSERT(widget);

    if (widget->m_Parent == this)
    {
        return *this;
    }

    if (widget->m_Parent)
    {
        UIWidget* oldParent = widget->m_Parent;

        oldParent->m_Children.Remove(oldParent->m_Children.IndexOf(widget));
        oldParent->m_LayoutSlots.Remove(oldParent->m_LayoutSlots.IndexOf(widget));
    }
    else
        widget->AddRef();

    widget->m_Parent = this;

    m_Children.InsertAt(0, widget);

    widget->BringOnTop(false);

    m_LayoutSlots.Add(widget);

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
    if (!m_Parent)
        return;

    //if (Desktop)
    //{
    //    LostFocus_r(Desktop);
    //    UpdateDesktop_r(nullptr);
    //}

    m_Parent->m_Children.Remove(m_Parent->m_Children.IndexOf(this));
    m_Parent->m_LayoutSlots.Remove(m_Parent->m_LayoutSlots.IndexOf(this));

    m_Parent = nullptr;

    RemoveRef();
}

void UIWidget::ForwardKeyEvent(KeyEvent const& event)
{
    OnKeyEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnKeyEvent(event);
    }
}

bool UIWidget::OnChildrenMouseButtonEvent(MouseButtonEvent const& event)
{
    return false;
}

bool UIWidget::OverrideMouseButtonEvent(MouseButtonEvent const& event)
{
    UIWidget* parent = m_Parent;
    if (!parent)
        return false;

    if (parent->OverrideMouseButtonEvent(event))
        return true;

    return parent->OnChildrenMouseButtonEvent(event);
}

void UIWidget::ForwardMouseButtonEvent(MouseButtonEvent const& event)
{
    if (OverrideMouseButtonEvent(event))
        return;

    OnMouseButtonEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnMouseButtonEvent(event);
    }
}

void UIWidget::ForwardDblClickEvent(VirtualKey buttonKey, Float2 const& clickPos, uint64_t clickTime)
{
    OnDblClickEvent(buttonKey, clickPos, clickTime);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnDblClickEvent(buttonKey, clickPos, clickTime);
    }
}

void UIWidget::ForwardMouseWheelEvent(MouseWheelEvent const& event)
{
    OnMouseWheelEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnMouseWheelEvent(event);
    }
}

void UIWidget::ForwardMouseMoveEvent(MouseMoveEvent const& event)
{
    OnMouseMoveEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnMouseMoveEvent(event);
    }
}

void UIWidget::ForwardGamepadButtonEvent(GamepadKeyEvent const& event)
{
    OnGamepadButtonEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnGamepadButtonEvent(event);
    }
}

void UIWidget::ForwardGamepadAxisMotionEvent(GamepadAxisMotionEvent const& event)
{
    OnGamepadAxisMotionEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnGamepadAxisMotionEvent(event);
    }
}

void UIWidget::ForwardCharEvent(CharEvent const& event)
{
    OnCharEvent(event);

    if (ShareInputs)
    {
        for (WeakRef<UIWidget> const& w : ShareInputs->GetWidgets())
            if (w && w != this)
                const_cast<WeakRef<UIWidget>&>(w)->OnCharEvent(event);
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

    bool autoW = bAutoWidth && bAllowAutoWidth;
    bool autoH = bAutoHeight && bAllowAutoHeight;

    Float2 layoutSize = Layout->MeasureLayout(this, autoW, autoH, size);

    if (autoW)
        m_MeasuredSize.X = layoutSize.X + Padding.Left + Padding.Right;
    else
        m_MeasuredSize.X = size.X;

    if (autoH)
        m_MeasuredSize.Y = layoutSize.Y + Padding.Top + Padding.Bottom;
    else
        m_MeasuredSize.Y = size.Y;

    return m_MeasuredSize;
}

void UIWidget::ArrangeChildren(bool bAllowAutoWidth, bool bAllowAutoHeight)
{
    if (!Layout)
        Layout = UINew(UIBoxLayout);

    bool autoW = bAutoWidth && bAllowAutoWidth;
    bool autoH = bAutoHeight && bAllowAutoHeight;

    if (autoW)
        m_Geometry.Maxs.X = m_Geometry.Mins.X + m_MeasuredSize.X;

    if (autoH)
        m_Geometry.Maxs.Y = m_Geometry.Mins.Y + m_MeasuredSize.Y;

    m_Geometry.UpdatePadding(Padding);

    if (Visibility != UI_WIDGET_VISIBILITY_VISIBLE)
        return;

    if (m_Geometry.Mins.X >= m_Geometry.Maxs.X || m_Geometry.Mins.Y >= m_Geometry.Maxs.Y)
        return;

    UpdateVisibility();

    if (m_Geometry.IsTiny())
    {
        return;
    }

    Layout->ArrangeChildren(this, autoW, autoH);
}

void UIWidget::AdjustSize(Float2 const& size)
{
    m_AdjustedSize.X = Math::Max(0.0f, size.X - Padding.Left - Padding.Right);
    m_AdjustedSize.Y = Math::Max(0.0f, size.Y - Padding.Top - Padding.Bottom);
}

void UIWidget::DrawBrush(Canvas& canvas, UIBrush* brush)
{
    Hk::DrawBrush(canvas, m_Geometry.Mins, m_Geometry.Maxs, {}, brush);
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
    for (UIWidget* p = m_Parent; p; p = p->m_Parent)
    {
        UIScroll* scroll = dynamic_cast<UIScroll*>(p);
        if (scroll && scroll->CanScroll())
        {
            return scroll;
        }
    }
    return nullptr;
}

bool UIWidget::HasFocus() const
{
    UIDesktop* desktop = GetDesktop();
    return desktop && desktop->m_FocusWidget == this;
}

UIWidget& UIWidget::SetFocus()
{
    UIDesktop* desktop = GetDesktop();
    if (!desktop)
    {
        m_bSetFocusOnAddToDesktop = true;
        return *this;
    }

    m_bSetFocusOnAddToDesktop = false;

    desktop->SetFocusWidget(this);

    return *this;
}

bool UIWidget::IsDisabled() const
{
    if (bDisabled)
        return true;
    return m_Parent ? m_Parent->IsDisabled() : false;
}

void UIWidget::SetLayer(StringView name)
{
    SetLayer(GetLayerNum(name));
}

void UIWidget::SetLayer(int layerNum)
{
    Layer = layerNum;
}

int UIWidget::GetLayerNum(StringView name) const
{
    int n = 0;
    for (auto w : m_LayoutSlots)
    {
        if (w->m_Name == name)
            return n;
        ++n;
    }
    return -1;
}

UIWidget* UIWidget::FindChildren(StringView name)
{
    for (auto w : m_Children)
    {
        if (w->m_Name == name)
            return w;
    }

    return nullptr;
}

HK_NAMESPACE_END
