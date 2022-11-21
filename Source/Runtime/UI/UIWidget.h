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
    UIShareInputs(std::initializer_list<UIWidget*> list);

    void Clear();

    UIShareInputs& Add(UIWidget* widget);
    UIShareInputs& Add(std::initializer_list<UIWidget*> list);

    TVector<TWeakRef<UIWidget>> const& GetWidgets() const { return m_Widgets; }

private:
    TVector<TWeakRef<UIWidget>> m_Widgets;
};

#define UINew(Type, ...)            (Type*)&(*CreateInstanceOf<Type>(__VA_ARGS__))
#define UINewAssign(Val, Type, ...) (Type*)&(*(Val = CreateInstanceOf<Type>(__VA_ARGS__)))

struct UIGridOffset
{
    uint32_t RowIndex{};
    uint32_t ColumnIndex{};

    UIGridOffset() = default;
    UIGridOffset(uint32_t rowIndex, uint32_t columnIndex) :
        RowIndex(rowIndex),
        ColumnIndex(columnIndex)
    {}

    UIGridOffset& WithRowIndex(uint32_t rowIndex)
    {
        RowIndex = rowIndex;
        return *this;
    }

    UIGridOffset& WithColumnIndex(uint32_t columnIndex)
    {
        ColumnIndex = columnIndex;
        return *this;
    }
};

class UIWidget : public UIObject
{
    UI_CLASS(UIWidget, UIObject)

public:
    UI_WIDGET_VISIBILITY Visibility = UI_WIDGET_VISIBILITY_VISIBLE;
    Float2               Position;
    Float2               Size;
    //Float2 MinSize; // TODO
    //Float2 MaxSize; // TODO
    UIPadding          Padding = UIPadding(4.0f);
    float              Opacity = 1;
    TRef<UIBaseLayout> Layout;
    TRef<UIBrush>      Background;
    TRef<UIBrush>      Foreground;
    UIGridOffset       GridOffset;
    bool               bAutoWidth : 1;
    bool               bAutoHeight : 1;
    bool               bTransparent : 1;
    bool               bDisabled : 1;
    bool               bExclusive : 1;
    bool               bNoInput : 1;
    bool               bStayBackground : 1;
    bool               bStayForeground : 1;
    bool               bPopup : 1; // TODO
    bool               bShortcutsAllowed : 1;
    bool               bAllowDrag : 1;
    // The hit shape is used to test that the widget overlaps the cursor.
    TRef<UIHitShape>    HitShape;
    TRef<UICursor>      Cursor;
    TRef<UIShareInputs> ShareInputs;
    TRef<UIWidget>     Tooltip;
    float              TooltipTime = 0.1f;

    TEvent<bool> E_OnHovered;

public:
    UIWidget();
    ~UIWidget();

    template <typename T, typename... TArgs>
    UIWidget& WithOnHovered(T* object, void (T::*method)(TArgs...))
    {
        E_OnHovered.Add(object, method);
        return *this;
    }

    UIWidget& WithVisibility(UI_WIDGET_VISIBILITY visibility)
    {
        Visibility = visibility;
        return *this;
    }

    UIWidget& WithPosition(Float2 const& position)
    {
        Position = position;
        return *this;
    }

    UIWidget& WithSize(Float2 const& size)
    {
        Size = size;
        return *this;
    }

    // TODO
    //UIWidget& WithMinSize(Float2 const& minSize)
    //{
    //    MinSize = minSize;
    //    return *this;
    //}

    // TODO
    //UIWidget& WithMaxSize(Float2 const& maxSize)
    //{
    //    MaxSize = maxSize;
    //    return *this;
    //}

    UIWidget& WithPadding(UIPadding const& padding)
    {
        Padding = padding;
        return *this;
    }

    UIWidget& WithOpacity(float opacity)
    {
        Opacity = opacity;
        return *this;
    }

    UIWidget& WithAutoWidth(bool autoWidth)
    {
        bAutoWidth = autoWidth;
        return *this;
    }

    UIWidget& WithAutoHeight(bool autoHeight)
    {
        bAutoHeight = autoHeight;
        return *this;
    }

    UIWidget& WithTransparent(bool transparent)
    {
        bTransparent = transparent;
        return *this;
    }

    UIWidget& WithDisabled(bool disabled)
    {
        bDisabled = disabled;
        return *this;
    }

    UIWidget& WithExclusive(bool exclusive)
    {
        bExclusive = exclusive;
        return *this;
    }

    UIWidget& WithNoInput(bool noInput)
    {
        bNoInput = noInput;
        return *this;
    }

    UIWidget& WithStayBackground(bool stayBackground)
    {
        bStayBackground = stayBackground;
        return *this;
    }

    UIWidget& WithStayForeground(bool stayForeground)
    {
        bStayForeground = stayForeground;
        return *this;
    }

    UIWidget& WithStayPopup(bool popup)
    {
        bPopup = popup;
        return *this;
    }

    UIWidget& WithShortcutsAllowed(bool shortcutsAllowed)
    {
        bShortcutsAllowed = shortcutsAllowed;
        return *this;
    }

    UIWidget& WithAllowDrag(bool allowDrag)
    {
        bAllowDrag = allowDrag;
        return *this;
    }
    
    UIWidget& WithLayout(UIBaseLayout* layout)
    {
        Layout = layout;
        return *this;
    }

    UIWidget& WithBackground(UIBrush* background)
    {
        Background = background;
        return *this;
    }

    UIWidget& WithForeground(UIBrush* foreground)
    {
        Foreground = foreground;
        return *this;
    }

    UIWidget& WithGridOffset(UIGridOffset const& gridOffset)
    {
        GridOffset = gridOffset;
        return *this;
    }

    UIWidget& WithHitShape(UIHitShape* hitShape)
    {
        HitShape = hitShape;
        return *this;
    }

    UIWidget& WithCursor(UICursor* cursor)
    {
        Cursor = cursor;
        return *this;
    }

    UIWidget& WithShareInputs(UIShareInputs* shareInputs)
    {
        ShareInputs = shareInputs;
        return *this;
    }

    UIWidget& WithTooltip(UIWidget* tooltip)
    {
        Tooltip = tooltip;
        return *this;
    }

    UIWidget& WithTooltipTime(float tooltipTime)
    {
        TooltipTime = tooltipTime;
        return *this;
    }

    UIWidget& SetFocus();

    UIWidget& SetVisible()
    {
        Visibility = UI_WIDGET_VISIBILITY_VISIBLE;
        return *this;
    }

    UIWidget& SetInvisible()
    {
        Visibility = UI_WIDGET_VISIBILITY_INVISIBLE;
        return *this;
    }

    UIWidget& SetCollapsed()
    {
        Visibility = UI_WIDGET_VISIBILITY_COLLAPSED;
        return *this;
    }

    UIWidget& BringOnTop(bool bRecursiveForParents = true);

     /** Get widget visibility type */
    UI_WIDGET_VISIBILITY GetVisibility() const { return Visibility; }

    /** Is widget visible */
    bool IsVisible() const { return Visibility == UI_WIDGET_VISIBILITY_VISIBLE; }

    /** Is widget not visible */
    bool IsInvisible() const { return Visibility != UI_WIDGET_VISIBILITY_VISIBLE; }

    /** Is widget collapsed */
    bool IsCollapsed() const { return Visibility == UI_WIDGET_VISIBILITY_COLLAPSED; }

    bool HasFocus() const;

    virtual bool IsDisabled() const;

    /** Helper. Add a child widget */
    UIWidget& operator[](UIWidget* widget)
    {
        return AddWidget(widget);
    }

    UIWidget& AddWidget(UIWidget* widget);

    UIWidget& AddWidgets(std::initializer_list<UIWidget*> list);

    void Detach();

    UIWidget* GetMaster();

    UIWidget* GetParent();

    UIDesktop* GetDesktop() const;

    UIWidget* Trace(float x, float y);

    bool HitTest(float x, float y) const;

    TVector<UIWidget*> const& GetChildren() const { return m_Children; }

    void ScrollSelfDelta(float delta);

    bool ShouldSetFocusOnAddToDesktop() const { return m_bSetFocusOnAddToDesktop; }

    void Draw(ACanvas& canvas, Float2 const& clipMins, Float2 const& clipMaxs, float alpha);

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

    virtual void AdjustSize(Float2 const& size);

    void DrawBrush(ACanvas& canvas, UIBrush* brush);

private:
    void DrawBackground(ACanvas& canvas);
    void DrawForeground(ACanvas& canvas);

    bool OverrideMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp);

    void UpdateVisibility();

    class UIScroll* FindScrollWidget();

protected:
    TWeakRef<UIWidget> m_Parent;

public:
    TVector<UIWidget*> m_Children;
    TVector<UIWidget*> m_LayoutSlots;
    UIDesktop*         m_Desktop{};
    Float2             m_AdjustedSize;
    Float2             m_MeasuredSize;
    UIWidgetGeometry   m_Geometry;

private:
    int  m_VisFrame = 0;
    bool m_bSetFocusOnAddToDesktop{};
};
