/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "WCommon.h"
#include "WDecorate.h"

/*

TODO:

Widgets:
WCheckBox
WRadioButton
WDropList
WList
WTable
WTree
WPropertyEdit
WMenuBar
WMenuPopup
WSpinBox
WTab
WMessageBox
WTooltip
WSplitView?
etc

Other:
Allow KEY_TAB navigation
window shadows

*/

class WDesktop;
class WScroll;

template< typename... TArgs >
struct TWidgetEvent : TEvent< TArgs... > {};

using AWidgetShape = TPodVector< Float2, 4 >;

#define WNew(Type) (*CreateInstanceOf< Type >())
#define WNewAssign(Val,Type) (*(Val = CreateInstanceOf< Type >()))

class WWidget : public ABaseObject {
    AN_CLASS( WWidget, ABaseObject )

    friend class WDesktop;

public:
    static constexpr int MAX_COLUMNS = 32;
    static constexpr int MAX_ROWS = 128;

    /** Add child widget */
    template< typename T >
    T * AddWidget() {
        T * w = CreateInstanceOf< T >();
        w->SetParent( this );
        return w;
    }

    /** Add child widget */
    WWidget & AddWidget( WWidget * _Widget ) {
        _Widget->SetParent( this );
        return *this;
    }

    /** Remove all childs */
    void RemoveWidgets();

    /** Add widget decoration */
    template< typename T >
    WWidget & AddDecorate() {
        WDecorate * d = CreateInstanceOf< T >();
        return AddDecorate( d );
    }

    /** Add widget decoration */
    WWidget & AddDecorate( WDecorate * _Decorate );

    /** Remove widget decoration */
    WWidget & RemoveDecorate( WDecorate * _Decorate );

    /** Remove all widget decorations */
    WWidget & RemoveDecorates();

    /** Helper. Add child widget */
    WWidget & operator[]( WWidget & _Widget ) {
        return AddWidget( &_Widget );
    }

    /** Helper. Add child decorate */
    WWidget & operator[]( WDecorate & _Decorate ) {
        return AddDecorate( &_Decorate );
    }

    /** Set widget parent */
    WWidget & SetParent( WWidget * _Parent );

    /** Unset widget parent */
    WWidget & Unparent();

    /** Get desktop */
    WDesktop * GetDesktop() { return Desktop; }

    /** Get parent */
    WWidget * GetParent() { return Parent; }

    /** Get childs */
    TPodVector< WWidget * > const & GetChilds() const { return Childs; }

    /** Set input focus */
    WWidget & SetFocus();

    /** Set widget style */
    WWidget & SetStyle( EWidgetStyle _Style );

    /** Set widget style */
    WWidget & SetStyle( int _Style );

    /** Widget location */
    WWidget & SetPosition( float _X, float _Y );

    /** Widget location */
    WWidget & SetPosition( Float2 const & _Position );

    /** Widget location on desktop */
    WWidget & SetDektopPosition( float _X, float _Y );

    /** Widget location on desktop */
    WWidget & SetDektopPosition( Float2 const & _Position );

    /** Widget size */
    WWidget & SetSize( float _Width, float _Height );

    /** Widget size */
    WWidget & SetSize( Float2 const & _Size );

    /** Widget min size */
    WWidget & SetMinSize( float _Width, float _Height );

    /** Widget min size */
    WWidget & SetMinSize( Float2 const & _Size );

    /** Widget max size */
    WWidget & SetMaxSize( float _Width, float _Height );

    /** Widget max size */
    WWidget & SetMaxSize( Float2 const & _Size );

    /** Custom clickable area */
    WWidget & SetShape( Float2 const * _Vertices, int _NumVertices );

    /** Custom drag area */
    WWidget & SetDragShape( Float2 const * _Vertices, int _NumVertices );

    /** Determines the padding of the client area within the widget */
    WWidget & SetMargin( float _Left, float _Top, float _Right, float _Bottom );

    /** Determines the padding of the client area within the widget */
    WWidget & SetMargin( Float4 const & _Margin );

    /** Determines horizontal location of the widget within its parent */
    WWidget & SetHorizontalAlignment( EWidgetAlignment _Alignment );

    /** Determines vertical location of the widget within its parent */
    WWidget & SetVerticalAlignment( EWidgetAlignment _Alignment );

    /** Determines layout for child widgets */
    WWidget & SetLayout( EWidgetLayout _Layout );

    /** Determines location in grid (for grid layouts) */
    WWidget & SetGridOffset( int _Column, int _Row );

    /** Determines grid size */
    WWidget & SetGridSize( int _ColumnsCount, int _RowsCount );

    /** Determines grid column size */
    WWidget & SetColumnWidth( int _ColumnIndex, float _Width );

    /** Determines grid row size */
    WWidget & SetRowWidth( int _RowIndex, float _Width );

    /** Audo adjust column sizes */
    WWidget & SetFitColumns( bool _FitColumns );

    /** Audo adjust row sizes */
    WWidget & SetFitRows( bool _FitRows );

    // Размер окна выбирается таким образом, чтобы на нем поместились все дочерние окна.
    // Если установлен лэйаут WIDGET_LAYOUT_IMAGE, то размер окна устанавливается равным ImageSize.
    // Если установлен лэйаут WIDGET_LAYOUT_GRID, то размеры ячеек зависят от вложенных окон, FitColumns/FitRows - игнорируются.
    // Если установлен лэйаут WIDGET_LAYOUT_HORIZONTAL_WRAP/WIDGET_LAYOUT_VERTICAL_WRAP, то враппинг игнорируется.
    WWidget & SetAutoWidth( bool _AutoWidth );
    WWidget & SetAutoHeight( bool _AutoHeight );

    /** Don't allow the widget size be less than parent client area */
    WWidget & SetClampWidth( bool _ClampWidth );

    /** Don't allow the widget size be less than parent client area */
    WWidget & SetClampHeight( bool _ClampHeight );

    /** Horizontal padding for horizontal layout */
    WWidget & SetHorizontalPadding( float _Padding );

    /** Vertical padding for vertical layout */
    WWidget & SetVerticalPadding( float _Padding );

    /** Image size for (for image layouts) */
    WWidget & SetImageSize( float _Width, float _Height );

    /** Image size for (for image layouts) */
    WWidget & SetImageSize( Float2 const & _ImageSize );

    /** Widget visibility type */
    WWidget & SetVisibility( EWidgetVisibility _Visibility );

    /** Helper. Set widget visible */
    WWidget & SetVisible() { return SetVisibility( WIDGET_VISIBILITY_VISIBLE ); }

    /** Helper. Set widget invisible */
    WWidget & SetInvisible() { return SetVisibility( WIDGET_VISIBILITY_INVISIBLE ); }

    /** Helper. Set widget collapsed */
    WWidget & SetCollapsed() { return SetVisibility( WIDGET_VISIBILITY_COLLAPSED ); }

    /** Set widget maximized */
    WWidget & SetMaximized();

    /** Set widget normal */
    WWidget & SetNormal();

    /** Set widget enabled */
    WWidget & SetEnabled( bool _Enabled ) { bDisabled = !_Enabled; return *this; }

    /** Is widget in focus */
    bool IsFocus() const;

    /** Is widget under specified location */
    bool IsHovered( Float2 const & _Position ) const;

    /** Is widget hovered by mouse cursor */
    bool IsHoveredByCursor() const;

    /** Get widget style */
    EWidgetStyle GetStyle() const { return Style; }

    /** Get widget position specified by user */
    Float2 const & GetPosition() const;

    /** Get position of widget (in desktop coordinates) */
    Float2 GetDesktopPosition() const;

    /** Get position of client area (in desktop coordinates) */
    Float2 GetClientPosition() const;

    /** Get widget size specified by user */
    Float2 const & GetSize() const { return Size; }

    /** Get widget min size */
    Float2 const & GetMinSize() { return MinSize; }

    /** Get widget max size */
    Float2 const & GetMaxSize() { return MaxSize; }

    /** Get widget width specified by user */
    float GetWidth() const { return Size.X; }

    /** Get widget height specified by user */
    float GetHeight() const { return Size.Y; }

    /** Get current width size */
    Float2 GetCurrentSize() const;

    /** Get client area width */
    float GetAvailableWidth() const;

    /** Get client area height */
    float GetAvailableHeight() const;

    /** Get client area size */
    Float2 GetAvailableSize() const;

    /** Get rectangle for the widget (in desktop coordinates) */
    void GetDesktopRect( Float2 & _Mins, Float2 & _Maxs, bool _Margin );

    /** Get rectangle for specified cell (in local coordinates) */
    void GetCellRect( int _ColumnIndex, int _RowIndex, Float2 & _Mins, Float2 & _Maxs ) const;

    /** Get parent layout area (in desktop coordinates) */
    void GetLayoutRect( Float2 & _Mins, Float2 & _Maxs ) const;

    /** Custom clickable area */
    AWidgetShape const & GetShape() const { return Shape; }

    /** Custom drag area */
    AWidgetShape const & GetDragShape() const { return DragShape; }

    /** Padding of the client area within the widget */
    Float4 const & GetMargin() const { return Margin; }

    /** Horizontal location of the widget within its parent */
    EWidgetAlignment GetHorizontalAlignment() const { return HorizontalAlignment; }

    /** Vertical location of the widget within its parent */
    EWidgetAlignment GetVerticalAlignment() const { return VerticalAlignment; }

    /** Layout for child widgets */
    EWidgetLayout GetLayout() const { return Layout; }

    /** Horizontal padding for horizontal layout */
    float GetHorizontalPadding() const { return HorizontalPadding; }

    /** Vertical padding for vertical layout */
    float GetVerticalPadding() const { return VerticalPadding; }

    /** Image size for (for image layouts) */
    Float2 const & GetImageSize() const { return ImageSize; }

    /** Get location in parents grid */
    void GetGridOffset( int & _Column, int & _Row ) const;

    /** Get widget visibility type */
    EWidgetVisibility GetVisibility() const { return Visibility; }

    /** Is widget visible */
    bool IsVisible() const { return Visibility == WIDGET_VISIBILITY_VISIBLE; }

    /** Is widget not visible */
    bool IsInvisible() const { return Visibility != WIDGET_VISIBILITY_VISIBLE; }

    /** Is widget collapsed */
    bool IsCollapsed() const { return Visibility == WIDGET_VISIBILITY_COLLAPSED; }

    /** Is widget maximized */
    bool IsMaximized() const;

    /** Is widget disabled */
    bool IsDisabled() const;

    /** From client to desktop coordinate */
    void FromClientToDesktop( Float2 & InOut ) const;

    /** From desktop to client coordinate */
    void FromDesktopToClient( Float2 & InOut ) const;

    /** From desktop to widget coordinate */
    void FromDesktopToWidget( Float2 & InOut ) const;

    /** Get cursor position relative to top-left corner */
    Float2 GetLocalCursorPosition() const;

    /** Set widget on top of other widgets */
    WWidget & BringOnTop( bool _RecursiveForParents = true );

    virtual bool IsShortcutsAllowed() const { return true; }

protected:
    WWidget();
    ~WWidget();

    virtual void OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp );

    virtual void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp );

    virtual void OnDblClickEvent( int _ButtonKey, Float2 const & _ClickPos, uint64_t _ClickTime );

    virtual void OnMouseWheelEvent( struct SMouseWheelEvent const & _Event, double _TimeStamp );

    virtual void OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp );

    virtual void OnJoystickButtonEvent( struct SJoystickButtonEvent const & _Event, double _TimeStamp );

    virtual void OnJoystickAxisEvent( struct SJoystickAxisEvent const & _Event, double _TimeStamp );

    virtual void OnCharEvent( struct SCharEvent const & _Event, double _TimeStamp );

    virtual void OnDragEvent( Float2 & _Position );

    virtual void OnFocusLost();

    virtual void OnFocusReceive();

    virtual void OnWindowHovered( bool _Hovered );

    virtual void OnDrawEvent( ACanvas & _Canvas );

    virtual void OnTransformDirty();

    virtual void AdjustSizeAndPosition( Float2 const & _AvailableSize, Float2 & _Size, Float2 & _Position );

    //virtual void OnTimerEvent( uint64_t _TimerID, float _FrameTimeDelta );

    // TODO: Add joystick events?

    void DrawDecorates( ACanvas & _Canvas );

    void ScrollSelfDelta( float _Delta );

    WScroll * FindScrollWidget();

public:
    void MarkTransformDirty();
    void MarkTransformDirtyChilds();

private:
    void Draw_r( ACanvas & _Canvas, Float2 const & _ClipMins, Float2 const & _ClipMaxs );

    void UpdateDesktop_r( WDesktop * _Desktop );

    bool IsRoot() const;

    WWidget * GetRoot();

    void MarkTransformDirty_r();
    void MarkGridLayoutDirty();
    void MarkVHLayoutDirty();
    void MarkImageLayoutDirty();

    void UpdateTransformIfDirty();
    void UpdateTransform();
    void UpdateLayoutIfDirty();
    void UpdateLayout();

    void LostFocus_r( WDesktop * _Desktop );

    float CalcContentWidth();
    float CalcContentHeight();

    float CalcAutoWidth();
    float CalcAutoHeight();

    struct SCell {
        float Size;
        float ActualSize;
        float Offset;
    };

    WDesktop * Desktop;
    WWidget * Parent;
    TPodVector< WWidget * > Childs;
    TPodVector< WDecorate *, 2 > Decorates;
    TPodVector< WWidget * > LayoutSlots;
    AWidgetShape Shape;
    AWidgetShape DragShape;
    Float2 Position;
    Float2 Size;
    Float2 MinSize;
    Float2 MaxSize;
    Float2 ImageSize;
    Float2 ActualPosition;
    Float2 ActualSize;
    Float4 Margin;
    EWidgetStyle Style;
    EWidgetAlignment HorizontalAlignment;
    EWidgetAlignment VerticalAlignment;
    EWidgetLayout Layout;
    EWidgetVisibility Visibility;
    int Row;
    int Column;
    Float2 LayoutOffset;
    float HorizontalPadding;
    float VerticalPadding;
    int ColumnsCount;
    int RowsCount;
    TPodVector< SCell, 1 > Columns;
    TPodVector< SCell, 1 > Rows;
    bool bFitColumns;
    bool bFitRows;
    bool bAutoWidth;
    bool bAutoHeight;
    bool bClampWidth;
    bool bClampHeight;
    bool bMaximized;
    bool bDisabled;
    bool bLayoutDirty;
    bool bTransformDirty;
    bool bFocus;
    bool bSetFocusOnAddToDesktop;
};

#if 0
class WMenuItem : public WWidget {
    AN_CLASS( WMenuItem, WWidget )

public:
    TWidgetEvent<> E_OnButtonClick;

    WMenuItem & SetText( const char * _Text );
    WMenuItem & SetColor( Color4 const & _Color );
    WMenuItem & SetHoverColor( Color4 const & _Color );
    WMenuItem & SetPressedColor( Color4 const & _Color );
    WMenuItem & SetTextColor( Color4 const & _Color );
    WMenuItem & SetBorderColor( Color4 const & _Color );
    WMenuItem & SetRounding( float _Rounding );
    WMenuItem & SetRoundingCorners( EDrawCornerFlags _RoundingCorners );
    WMenuItem & SetBorderThickness( float _Thickness );

    template< typename T, typename... TArgs >
    WMenuItem & SetOnClick( T * _Object, void ( T::*_Method )(TArgs...) ) {
        E_OnButtonClick.Add( _Object, _Method );
        return *this;
    }

    bool IsPressed() const { return State == ST_PRESSED; }
    bool IsReleased() const { return State == ST_RELEASED; }

protected:
    WMenuItem();
    ~WMenuItem();

    void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) override;

    void OnDrawEvent( ACanvas & _Canvas ) override;

private:
    enum {
        ST_RELEASED = 0,
        ST_PRESSED = 1
    };

    int State;
    Color4 Color;
    Color4 HoverColor;
    Color4 PressedColor;
    Color4 TextColor;
    Color4 BorderColor;
    EDrawCornerFlags RoundingCorners;
    AString Text;
    float Rounding;
    float BorderThickness;
};
#endif
