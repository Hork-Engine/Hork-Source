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

#include "UILayout.h"
#include "UIHitShape.h"
#include "UIBrush.h"
#include "UICursor.h"

enum UI_WIDGET_VISIBILITY
{
    UI_WIDGET_VISIBILITY_VISIBLE,   // The widget will appear normally
    UI_WIDGET_VISIBILITY_INVISIBLE, // The widget will be not visible, but will take up space in the layout
    UI_WIDGET_VISIBILITY_COLLAPSED  // The widget will be not visible and will take no space in the layout
};

class UIDesktop;

class UIShareInputs : public UIObject
{
    UI_CLASS(UIShareInputs, UIObject)

public:
    UIShareInputs() = default;

    void Clear();

    void Add(UIWidget* widget);

    TVector<TWeakRef<UIWidget>> const& GetWidgets() const { return m_Widgets; }

private:
    TVector<TWeakRef<UIWidget>> m_Widgets;
};

#define UINew(Type, ...)            (*CreateInstanceOf<Type>(__VA_ARGS__))
#define UINewAssign(Val, Type, ...) (*(Val = CreateInstanceOf<Type>(__VA_ARGS__)))

struct UIGridOffset
{
    uint32_t RowIndex{};
    uint32_t ColumnIndex{};
};

class UIWidget : public UIObject
{
    UI_CLASS(UIWidget, UIObject)

public:
    UI_WIDGET_VISIBILITY Visibility = UI_WIDGET_VISIBILITY_VISIBLE;
    Float2            Position;
    Float2            Size;
    //Float2 MinSize; // TODO
    //Float2 MaxSize; // TODO
    UIPadding Padding = UIPadding(4.0f);
    float              Opacity = 1;
    bool               bAutoWidth{false};
    bool               bAutoHeight{false};
    bool               bTransparent{false};
    bool               bDisabled{false};
    bool               bExclusive{false};
    bool               bNoInput{false};
    bool               bBackground{false};
    bool               bForeground{false};
    bool               bPopup{false};
    bool               bShortcutsAllowed{true};
    bool               bAllowDrag{false};
    TRef<UIBaseLayout> Layout;
    TRef<UIBrush>      Background;
    TRef<UIBrush>      Foreground;
    UIGridOffset       GridOffset;

    // The hit shape is used to test that the widget overlaps the cursor.
    TRef<UIHitShape>   HitShape;
    TRef<UICursor>     MouseCursor;
    TRef<UIShareInputs> ShareInputs;
    //TRef<UIWidget>     Tooltip; // TODO
    //flota              TooltipTime = 0.1f; // TODO

    // Internal

    TWeakRef<UIWidget> Parent;
    TVector<UIWidget*> Children;
    TVector<UIWidget*> LayoutSlots;
    UIDesktop*         Desktop{};

    Color4 DebugColor = Color4(0,0,0,0);

    Float2 AdjustedSize;
    Float2 MeasuredSize;

    UIWidgetGeometry Geometry;

    int VisFrame = 0;

    UIWidget();
    ~UIWidget();

    UIWidget& SetHitShape(UIHitShape* hitShape);

    UIWidget& SetShareInputs(UIShareInputs* shareInputs);

     /** Get widget visibility type */
    UI_WIDGET_VISIBILITY GetVisibility() const { return Visibility; }

    /** Is widget visible */
    bool IsVisible() const { return Visibility == UI_WIDGET_VISIBILITY_VISIBLE; }

    /** Is widget not visible */
    bool IsInvisible() const { return Visibility != UI_WIDGET_VISIBILITY_VISIBLE; }

    /** Is widget collapsed */
    bool IsCollapsed() const { return Visibility == UI_WIDGET_VISIBILITY_COLLAPSED; }

    UIWidget& AddWidget(UIWidget* widget);
    void Detach();

    UIWidget* GetMaster();

    void UpdateVisibility();

    UIWidget* Trace(float x, float y);

    bool HitTest(float x, float y) const;

    UIDesktop* GetDesktop();

    UIWidget& BringOnTop(bool bRecursiveForParents = true);

    void Draw(ACanvas& canvas, Float2 const& clipMins, Float2 const& clipMaxs, float alpha);

    void DrawBackground(ACanvas& canvas);
    void DrawForeground(ACanvas& canvas);

    virtual bool IsDisabled() const
    {
        if (bDisabled)
            return true;
        return Parent ? Parent->IsDisabled() : false;
    }

    void ForwardKeyEvent(struct SKeyEvent const& event, double timeStamp);

    void ForwardMouseButtonEvent(struct SMouseButtonEvent const& event, double timeStamp);

    void ForwardDblClickEvent(int buttonKey, Float2 const& clickPos, uint64_t clickTime);

    void ForwardMouseWheelEvent(struct SMouseWheelEvent const& event, double timeStamp);

    void ForwardMouseMoveEvent(struct SMouseMoveEvent const& event, double timeStamp);

    void ForwardJoystickButtonEvent(struct SJoystickButtonEvent const& event, double timeStamp);

    void ForwardJoystickAxisEvent(struct SJoystickAxisEvent const& event, double timeStamp);

    void ForwardCharEvent(struct SCharEvent const& event, double timeStamp);

    void ForwardDragEvent(Float2& position);

    void ForwardFocusEvent(bool bFocus);

    void ForwardHoverEvent(bool bHovered);

    Float2 MeasureLayout(bool bAllowAutoWidth, bool bAllowAutoHeight, Float2 const& size);

    void ArrangeChildren(bool bAllowAutoWidth, bool bAllowAutoHeight);

    virtual void AdjustSize(Float2 const& size)
    {
        AdjustedSize.X = Math::Max(0.0f, size.X - Padding.Left - Padding.Right);
        AdjustedSize.Y = Math::Max(0.0f, size.Y - Padding.Top - Padding.Bottom);
    }

protected:
    virtual void OnKeyEvent(struct SKeyEvent const& event, double timeStamp);

    virtual void OnMouseButtonEvent(struct SMouseButtonEvent const& event, double timeStamp);

    virtual bool OnChildrenMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp);

    virtual void OnDblClickEvent(int buttonKey, Float2 const& clickPos, uint64_t clickTime);

    virtual void OnMouseWheelEvent(struct SMouseWheelEvent const& event, double timeStamp);

    virtual void OnMouseMoveEvent(struct SMouseMoveEvent const& event, double timeStamp);

    virtual void OnJoystickButtonEvent(struct SJoystickButtonEvent const& event, double timeStamp);

    virtual void OnJoystickAxisEvent(struct SJoystickAxisEvent const& event, double timeStamp);

    virtual void OnCharEvent(struct SCharEvent const& event, double timeStamp);

    virtual void OnDragEvent(Float2& position);

    virtual void OnFocusLost();

    virtual void OnFocusReceive();

    virtual void OnWindowHovered(bool bHovered);

    virtual void Draw(ACanvas& _Canvas);

    virtual void PostDraw(ACanvas& _Canvas);

private:
    bool OverrideMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp);
};
