/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Widgets/Public/WWidget.h>
#include <Engine/Widgets/Public/WScroll.h>
#include <Engine/Widgets/Public/WDesktop.h>
#include <Engine/Runtime/Public/Runtime.h>

AN_CLASS_META( WWidget )

WWidget::WWidget() {
    Size = Float2( 32, 32 );
    Visibility = WIDGET_VISIBILITY_VISIBLE;
    ColumnsCount = RowsCount = 1;
    bTransformDirty = true;
    bLayoutDirty = true;
    Margin = Float4(2,2,2,2);
}

WWidget::~WWidget() {
    RemoveDecorates();

    if ( bFocus ) {
        AN_Assert( Desktop );
        AN_Assert( Desktop->GetFocusWidget() == this );
        Desktop->SetFocusWidget( nullptr );
    }

    for ( WWidget * child : Childs ) {
        child->Parent = nullptr;
        child->MarkTransformDirty(); // FIXME: mark anyway?
        child->RemoveRef();
    }
}

WWidget & WWidget::SetParent( WWidget * _Parent ) {
    if ( IsRoot() ) {
        return *this;
    }

    if ( Parent == _Parent ) {
        return *this;
    }

    Unparent();

    if ( !_Parent ) {
        return *this;
    }

    Parent = _Parent;

    UpdateDesktop_r( _Parent->Desktop );

    AddRef();
    _Parent->Childs.Insert( 0, this );

    BringOnTop( false );

    Parent->LayoutSlots.Append( this );
    Parent->MarkVHLayoutDirty();

    if ( Parent->bAutoWidth || Parent->bAutoHeight ) {
        Parent->MarkTransformDirty();
    }

    MarkTransformDirty();

    return *this;
}

void WWidget::UpdateDesktop_r( WDesktop * _Desktop ) {
    //if ( Desktop == _Desktop ) {
    //    return;
    //}

    Desktop = _Desktop;
    for ( WWidget * child : Childs ) {
        child->UpdateDesktop_r( _Desktop );
    }
}

void WWidget::LostFocus_r( WDesktop * _Desktop ) {
    if ( bFocus ) {
        AN_Assert( _Desktop->GetFocusWidget() == this );
        _Desktop->SetFocusWidget( nullptr );
        return;
    }

    for ( WWidget * child : Childs ) {
        child->LostFocus_r( _Desktop );
    }
}

WWidget & WWidget::Unparent() {
    if ( IsRoot() ) {
        return *this;
    }

    if ( !Parent ) {
        return *this;
    }

    if ( Desktop ) {
        LostFocus_r( Desktop );
        UpdateDesktop_r( nullptr );
    }

    Parent->Childs.Remove( Parent->Childs.IndexOf( this ) );
    Parent->LayoutSlots.Remove( Parent->LayoutSlots.IndexOf( this ) );
    Parent->MarkVHLayoutDirty();
    if ( Parent->bAutoWidth || Parent->bAutoHeight ) {
        Parent->MarkTransformDirty();
    }
    Parent = nullptr;

    MarkTransformDirty();
    RemoveRef();

    return *this;
}

void WWidget::RemoveWidgets() {
    while ( !Childs.IsEmpty() ) {
        Childs.Last()->Unparent();
    }
}

bool WWidget::IsRoot() const {
    return Desktop && Desktop->Root == this;
}

WWidget * WWidget::GetRoot() {
    return Desktop ? Desktop->Root.GetObject() : nullptr;
}

WWidget & WWidget::SetStyle( EWidgetStyle _Style ) {
    bool bBringOnTop = false;

    int style = _Style;

    // Оба стиля не могут существовать одновременно
    if ( style & WIDGET_STYLE_FOREGROUND ) {
        style &= ~WIDGET_STYLE_BACKGROUND;
    }
    if ( style & WIDGET_STYLE_POPUP ) {
        style &= ~WIDGET_STYLE_BACKGROUND;
    }

    if ( ( style & WIDGET_STYLE_FOREGROUND ) && !( Style & WIDGET_STYLE_FOREGROUND ) ) {
        bBringOnTop = true;
    }

    Style = (EWidgetStyle)style;

    if ( bBringOnTop ) {
        BringOnTop();
    }

    return *this;
}

WWidget & WWidget::SetStyle( int _Style ) {
    return SetStyle( (EWidgetStyle)_Style );
}

WWidget & WWidget::SetFocus() {
    if ( !Desktop ) {
        return *this;
    }

    Desktop->SetFocusWidget( this );

    return *this;
}

bool WWidget::IsFocus() const {
    return bFocus;
}

WWidget & WWidget::SetPosition( float _X, float _Y ) {
    return SetPosition( Float2( _X, _Y ) );
}

WWidget & WWidget::SetPosition( Float2 const & _Position ) {
    Position = ( _Position + 0.5f ).Floor();
    MarkTransformDirty();
    return *this;
}

WWidget & WWidget::SetDektopPosition( float _X, float _Y ) {
    return SetDektopPosition( Float2( _X, _Y ) );
}

WWidget & WWidget::SetDektopPosition( Float2 const & _Position ) {
    Float2 position = _Position;
    if ( Parent ) {
        position -= Parent->GetDesktopPosition() + Parent->Margin.Shuffle2< sXY >();
    }
    return SetPosition( position );
}

WWidget & WWidget::SetSize( float _Width, float _Height ) {
    return SetSize( Float2( _Width, _Height ) );
}

WWidget & WWidget::SetSize( Float2 const & _Size ) {

    Float2 sz = ( _Size + 0.5f ).Floor();

    if ( sz.X < 0.0f ) {
        sz.X = 0.0f;
    }
    if ( sz.Y < 0.0f ) {
        sz.Y = 0.0f;
    }

    Size = sz;
    MarkTransformDirty();

    return *this;
}

WWidget & WWidget::SetMinSize( float _Width, float _Height ) {
    return SetMinSize( Float2( _Width, _Height ) );
}

WWidget & WWidget::SetMinSize( Float2 const & _Size ) {
    MinSize = _Size;

    if ( MinSize.X < 0.0f ) {
        MinSize.X = 0.0f;
    }
    if ( MinSize.Y < 0.0f ) {
        MinSize.Y = 0.0f;
    }

    MarkTransformDirty();

    return *this;
}

WWidget & WWidget::SetMaxSize( float _Width, float _Height ) {
    return SetMaxSize( Float2( _Width, _Height ) );
}

WWidget & WWidget::SetMaxSize( Float2 const & _Size ) {
    MaxSize = _Size;

    if ( MaxSize.X < 0.0f ) {
        MaxSize.X = 0.0f;
    }
    if ( MaxSize.Y < 0.0f ) {
        MaxSize.Y = 0.0f;
    }

    MarkTransformDirty();

    return *this;
}

WWidget & WWidget::SetShape( Float2 const * _Vertices, int _NumVertices ) {
    if ( _NumVertices > 0 ) {
        Shape.ResizeInvalidate( _NumVertices );
        memcpy( Shape.ToPtr(), _Vertices, _NumVertices * sizeof( Shape[0] ) );
    } else {
        Shape.Clear();
    }

    return *this;
}

WWidget & WWidget::SetDragShape( Float2 const * _Vertices, int _NumVertices ) {
    if ( _NumVertices > 0 ) {
        DragShape.ResizeInvalidate( _NumVertices );
        memcpy( DragShape.ToPtr(), _Vertices, _NumVertices * sizeof( DragShape[0] ) );
    } else {
        DragShape.Clear();
    }

    return *this;
}

WWidget & WWidget::SetMargin( float _Left, float _Top, float _Right, float _Bottom ) {
    return SetMargin( Float4( _Left, _Top, _Right, _Bottom ) );
}

WWidget & WWidget::SetMargin( Float4 const & _Margin ) {
    Margin = _Margin;
    if ( Margin.X < 0.0f ) {
        Margin.X = 0.0f;
    }
    if ( Margin.Y < 0.0f ) {
        Margin.Y = 0.0f;
    }
    if ( Margin.Z < 0.0f ) {
        Margin.Z = 0.0f;
    }
    if ( Margin.W < 0.0f ) {
        Margin.W = 0.0f;
    }
    MarkTransformDirtyChilds();
    return *this;
}

WWidget & WWidget::SetHorizontalAlignment( EWidgetAlignment _Alignment ) {
    HorizontalAlignment = _Alignment;
    MarkTransformDirty();
    return *this;
}

WWidget & WWidget::SetVerticalAlignment( EWidgetAlignment _Alignment ) {
    VerticalAlignment = _Alignment;
    MarkTransformDirty();
    return *this;
}

WWidget & WWidget::SetLayout( EWidgetLayout _Layout ) {
    if ( Layout != _Layout ) {
        Layout = _Layout;
        bLayoutDirty = true;
        MarkTransformDirtyChilds();
    }
    return *this;
}

WWidget & WWidget::SetGridOffset( int _Column, int _Row ) {
    Column = _Column;
    Row = _Row;
    MarkTransformDirty();
    return *this;
}

WWidget & WWidget::AddDecorate( WDecorate * _Decorate ) {
    if ( _Decorate ) {
        if ( _Decorate->Owner ) {
            _Decorate->Owner->RemoveDecorate( _Decorate );
        }
        Decorates.Append( _Decorate );
        _Decorate->Owner = this;
        _Decorate->AddRef();
    }
    return *this;
}

WWidget & WWidget::RemoveDecorate( WDecorate * _Decorate ) {

    int index = Decorates.IndexOf( _Decorate );
    if ( index != -1 ) {
        Decorates.Remove( index );
        _Decorate->Owner = nullptr;
        _Decorate->RemoveRef();
    }

    return *this;
}

WWidget & WWidget::RemoveDecorates() {
    for ( WDecorate * d : Decorates ) {
        d->Owner = nullptr;
        d->RemoveRef();
    }
    Decorates.Clear();
    return *this;
}

Float2 const & WWidget::GetPosition() const {
    return Position;
}

AN_FORCEINLINE void ClampWidgetSize( Float2 & _InOut, Float2 const & _Min, Float2 const & _Max ) {
    if ( _InOut.X < _Min.X ) {
        _InOut.X = _Min.X;
    }
    if ( _InOut.Y < _Min.Y ) {
        _InOut.Y = _Min.Y;
    }
    if ( _Max.X > 0.0f ) {
        if ( _InOut.X > _Max.X ) {
            _InOut.X = _Max.X;
        }
    }
    if ( _Max.Y > 0.0f ) {
        if ( _InOut.Y > _Max.Y ) {
            _InOut.Y = _Max.Y;
        }
    }
}

void WWidget::UpdateTransformIfDirty() {
    if ( bTransformDirty ) {
        UpdateTransform();
    }
}

static void ApplyHorizontalAlignment( EWidgetAlignment _HorizontalAlignment, Float2 const & _AvailSize, Float2 & _Size, Float2 & _Pos ) {
    switch ( _HorizontalAlignment ) {
    case WIDGET_ALIGNMENT_STRETCH:
        _Pos.X = 0;
        _Size.X = _AvailSize.X;
        break;
    case WIDGET_ALIGNMENT_LEFT:
        _Pos.X = 0;
        break;
    case WIDGET_ALIGNMENT_RIGHT:
        _Pos.X = _AvailSize.X - _Size.X;
        break;
    case WIDGET_ALIGNMENT_CENTER: {
        float center = _AvailSize.X * 0.5f;
        _Pos.X = center - _Size.X * 0.5f;
        break;
        }
    default:
        break;
    }
}

static void ApplyVerticalAlignment( EWidgetAlignment _VerticalAlignment, Float2 const & _AvailSize, Float2 & _Size, Float2 & _Pos ) {
    switch ( _VerticalAlignment ) {
    case WIDGET_ALIGNMENT_STRETCH:
        _Pos.Y = 0;
        _Size.Y = _AvailSize.Y;
        break;
    case WIDGET_ALIGNMENT_TOP:
        _Pos.Y = 0;
        break;
    case WIDGET_ALIGNMENT_BOTTOM:
        _Pos.Y = _AvailSize.Y - _Size.Y;
        break;
    case WIDGET_ALIGNMENT_CENTER: {
        float center = _AvailSize.Y * 0.5f;
        _Pos.Y = center - _Size.Y * 0.5f;
        break;
        }
    default:
        break;
    }
}

void WWidget::UpdateTransform() {
    bTransformDirty = false;

    if ( !Parent ) {
        ActualPosition = Position;
        ActualSize.X = CalcContentWidth();
        ActualSize.Y = CalcContentHeight();
        ClampWidgetSize( ActualSize, MinSize, MaxSize );
        return;
    }

    Float2 curPos = Position;
    Float2 curSize;

    curSize.X = CalcContentWidth();
    curSize.Y = CalcContentHeight();

    ClampWidgetSize( curSize, MinSize, MaxSize );

    Float2 availSize;
    Float2 localOffset(0.0f);

    switch ( Parent->Layout ) {
    case WIDGET_LAYOUT_EXPLICIT:
    case WIDGET_LAYOUT_HORIZONTAL:
    case WIDGET_LAYOUT_HORIZONTAL_WRAP:
    case WIDGET_LAYOUT_VERTICAL:
    case WIDGET_LAYOUT_VERTICAL_WRAP:
    case WIDGET_LAYOUT_IMAGE:
    case WIDGET_LAYOUT_CUSTOM:
        availSize = Parent->GetAvailableSize();
        break;
    case WIDGET_LAYOUT_GRID: {
        Float2 cellMins, cellMaxs;
        Parent->GetCellRect( Column, Row, cellMins, cellMaxs );
        availSize = cellMaxs - cellMins;
        localOffset = cellMins;
        break;
        }
    default:
        AN_Assert( 0 );
        availSize = Parent->GetAvailableSize();
        break;
    }

    if ( !IsMaximized() ) {
        switch ( Parent->Layout ) {
        case WIDGET_LAYOUT_EXPLICIT:
        case WIDGET_LAYOUT_GRID:
            ApplyHorizontalAlignment( HorizontalAlignment, availSize, curSize, curPos );
            ApplyVerticalAlignment( VerticalAlignment, availSize, curSize, curPos );
            break;

        case WIDGET_LAYOUT_IMAGE: {
            Float2 const & imageSize = Parent->GetImageSize();
            Float2 scale = availSize / imageSize;

            curPos = ( curPos * scale + 0.5f ).Floor();
            curSize = ( curSize * scale + 0.5f ).Floor();

            ApplyHorizontalAlignment( HorizontalAlignment, availSize, curSize, curPos );
            ApplyVerticalAlignment( VerticalAlignment, availSize, curSize, curPos );
            break;
            }

        case WIDGET_LAYOUT_HORIZONTAL:
        case WIDGET_LAYOUT_HORIZONTAL_WRAP:
        case WIDGET_LAYOUT_VERTICAL:
        case WIDGET_LAYOUT_VERTICAL_WRAP:
            Parent->UpdateLayoutIfDirty();

            curPos = LayoutOffset;

            if ( Parent->Layout == WIDGET_LAYOUT_HORIZONTAL ) {
                ApplyVerticalAlignment( VerticalAlignment, availSize, curSize, curPos );
            }
            if ( Parent->Layout == WIDGET_LAYOUT_VERTICAL ) {
                ApplyHorizontalAlignment( HorizontalAlignment, availSize, curSize, curPos );
            }
            break;

        case WIDGET_LAYOUT_CUSTOM:
            AdjustSizeAndPosition( availSize, curSize, curPos );
            break;

        default:
            AN_Assert( 0 );
        }

        if ( bClampWidth && curPos.X + curSize.X > availSize.X ) {
            curSize.X = Math::Max( 0.0f, availSize.X - curPos.X );
        }
        if ( bClampWidth && curPos.Y + curSize.Y > availSize.Y ) {
            curSize.Y = Math::Max( 0.0f, availSize.Y - curPos.Y );
        }

        curPos += localOffset;

    } else {
        curPos = localOffset;
        curSize = availSize;
    }

    // from local to desktop
    ActualPosition = curPos + Parent->GetClientPosition();
    ActualSize = curSize;
}

Float2 WWidget::GetDesktopPosition() const {
    const_cast< WWidget * >( this )->UpdateTransformIfDirty();

    return ActualPosition;
}

Float2 WWidget::GetClientPosition() const {
    return GetDesktopPosition() + Margin.Shuffle2< sXY >();
}

Float2 WWidget::GetCurrentSize() const {
    const_cast< WWidget * >( this )->UpdateTransformIfDirty();

    return ActualSize;
}

float WWidget::GetAvailableWidth() const {
    Float2 sz = GetCurrentSize();
    return Math::Max( sz.X - Margin.X - Margin.Z, 0.0f );
}

float WWidget::GetAvailableHeight() const {
    Float2 sz = GetCurrentSize();
    return Math::Max( sz.Y - Margin.Y - Margin.W, 0.0f );
}

Float2 WWidget::GetAvailableSize() const {
    Float2 sz = GetCurrentSize();
    return Float2( Math::Max( sz.X - Margin.X - Margin.Z, 0.0f ),
                   Math::Max( sz.Y - Margin.Y - Margin.W, 0.0f ) );
}

void WWidget::GetDesktopRect( Float2 & _Mins, Float2 & _Maxs, bool _Margin ) {
    _Mins = GetDesktopPosition();
    _Maxs = _Mins + GetCurrentSize();
    if ( _Margin ) {
        _Mins.X += Margin.X;
        _Mins.Y += Margin.Y;
        _Maxs.X -= Margin.Z;
        _Maxs.Y -= Margin.W;
    }
}

void WWidget::FromClientToDesktop( Float2 & InOut ) const {
    InOut += GetClientPosition();
}

void WWidget::FromDesktopToClient( Float2 & InOut ) const {
    InOut -= GetClientPosition();
}

void WWidget::FromDesktopToWidget( Float2 & InOut ) const {
    InOut -= GetDesktopPosition();
}

void WWidget::GetGridOffset( int & _Column, int & _Row ) const {
    _Column = Column;
    _Row = Row;
}

WWidget & WWidget::SetVisibility( EWidgetVisibility _Visibility ) {
    if ( Visibility != _Visibility ) {
        Visibility = _Visibility;

        // Mark transforms only for collapsed and visible widgets
        if ( Visibility != WIDGET_VISIBILITY_INVISIBLE ) {
            if ( Parent ) {
                // Mark all childs in parent widget to update collapsed/uncollapsed visibility
                Parent->bLayoutDirty = true;
                Parent->MarkTransformDirtyChilds();
            } else {
                MarkTransformDirty();
            }
        }
    }
    return *this;
}

WWidget & WWidget::SetMaximized() {
    if ( bMaximized ) {
        return *this;
    }

    bMaximized = true;

    // TODO: bMaximizedAnimation = true

    MarkTransformDirty();

    return *this;
}

WWidget & WWidget::SetNormal() {
    if ( bMaximized ) {
        bMaximized = false;
        MarkTransformDirty();
    }

    return *this;
}

bool WWidget::IsMaximized() const {
    return bMaximized;
}

bool WWidget::IsDisabled() const {
    for ( WWidget const * w = this ; w && !w->IsRoot() ; w = w->Parent ) {
        if ( w->bDisabled ) {
            return true;
        }
    }
    return false;
}

WWidget & WWidget::BringOnTop( bool _RecursiveForParents ) {
    if ( !Parent ) {
        return *this;
    }

    if ( !(Style & WIDGET_STYLE_BACKGROUND) ) {
        if ( Style & WIDGET_STYLE_FOREGROUND ) {

            if ( AN_HASFLAG( Style, WIDGET_STYLE_POPUP ) ) {
                // bring on top
                if ( Parent->Childs.Last() != this ) {
                    Parent->Childs.Remove( Parent->Childs.IndexOf( this ) );
                    Parent->Childs.Append( this );
                }
            } else {
                int i;

                // skip popup widgets
                for ( i = Parent->Childs.Size() - 1 ; i >= 0 && AN_HASFLAG( Parent->Childs[ i ]->GetStyle(), WIDGET_STYLE_POPUP ) ; i-- ) {}

                // bring before popup widgets
                if ( i >= 0 && Parent->Childs[i] != this ) {
                    int index = Parent->Childs.IndexOf( this );
                    Parent->Childs.Remove( index );
                    Parent->Childs.Insert( i, this );
                }
            }
        } else {
            int i;

            // skip foreground widgets
            for ( i = Parent->Childs.Size() - 1 ; i >= 0 && AN_HASFLAG( Parent->Childs[ i ]->GetStyle(), WIDGET_STYLE_FOREGROUND ) ; i-- ) {}

            // bring before foreground widgets
            if ( i >= 0 && Parent->Childs[i] != this ) {
                int index = Parent->Childs.IndexOf( this );
                Parent->Childs.Remove( index );
                Parent->Childs.Insert( i, this );
            }
        }
    }

    if ( _RecursiveForParents ) {
        Parent->BringOnTop();
    }

    return *this;
}

bool WWidget::IsHovered( Float2 const & _Position ) const {
    if ( !Desktop ) {
        return false;
    }

    WWidget * w = Desktop->GetWidgetUnderCursor( _Position );
    return w == this;
}

bool WWidget::IsHoveredByCursor() const {
    if ( !Desktop ) {
        return false;
    }

    WWidget * w = Desktop->GetWidgetUnderCursor( Desktop->GetCursorPosition() );
    return w == this;
}

AN_FORCEINLINE void ApplyMargins( Float2 & _Mins, Float2 & _Maxs, Float4 const & _Margins ) {
    _Mins.X += _Margins.X;
    _Mins.Y += _Margins.Y;
    _Maxs.X -= _Margins.Z;
    _Maxs.Y -= _Margins.W;
}

void WWidget::Draw_r( ACanvas & _Canvas, Float2 const & _ClipMins, Float2 const & _ClipMaxs ) {
    if ( !IsVisible() ) {
        return;
    }

    Float2 rectMins, rectMaxs;
    Float2 mins, maxs;

    GetDesktopRect( rectMins, rectMaxs, false );

    mins.X = Math::Max( rectMins.X, _ClipMins.X );
    mins.Y = Math::Max( rectMins.Y, _ClipMins.Y );

    maxs.X = Math::Min( rectMaxs.X, _ClipMaxs.X );
    maxs.Y = Math::Min( rectMaxs.Y, _ClipMaxs.Y );

    if ( mins.X >= maxs.X || mins.Y >= maxs.Y ) {
        return; // invalid rect
    }

    _Canvas.PushClipRect( mins, maxs );

    OnDrawEvent( _Canvas );

    _Canvas.PopClipRect();

    ApplyMargins( rectMins, rectMaxs, GetMargin() );

    mins.X = Math::Max( rectMins.X, _ClipMins.X );
    mins.Y = Math::Max( rectMins.Y, _ClipMins.Y );

    maxs.X = Math::Min( rectMaxs.X, _ClipMaxs.X );
    maxs.Y = Math::Min( rectMaxs.Y, _ClipMaxs.Y );

    if ( mins.X >= maxs.X || mins.Y >= maxs.Y ) {
        return; // invalid rect
    }

    if ( Layout == WIDGET_LAYOUT_GRID ) {
        Float2 cellMins, cellMaxs;
        Float2 clientPos = GetClientPosition();

        // TODO: Draw grid borders here

        for ( WWidget * child : GetChilds() ) {
            int columnIndex;
            int rowIndex;

            child->GetGridOffset( columnIndex, rowIndex );

            GetCellRect( columnIndex, rowIndex, cellMins, cellMaxs );
            cellMins += clientPos;
            cellMaxs += clientPos;

            cellMins.X = Math::Max( cellMins.X, mins.X );
            cellMins.Y = Math::Max( cellMins.Y, mins.Y );

            cellMaxs.X = Math::Min( cellMaxs.X, maxs.X );
            cellMaxs.Y = Math::Min( cellMaxs.Y, maxs.Y );

            if ( cellMins.X >= cellMaxs.X || cellMins.Y >= cellMaxs.Y ) {
                continue;
            }

            child->Draw_r( _Canvas, cellMins, cellMaxs );
        }
    } else {
        for ( WWidget * child : Childs ) {
            child->Draw_r( _Canvas, mins, maxs );
        }
    }
}

void WWidget::OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp ) {

}

void WWidget::OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) {

}

void WWidget::OnDblClickEvent( int _ButtonKey, Float2 const & _ClickPos, uint64_t _ClickTime ) {

}

WScroll * WWidget::FindScrollWidget() {
    for ( WWidget * p = Parent ; p ; p = p->Parent ) {
        WScroll * scroll = Upcast< WScroll >( p );
        if ( scroll ) {
            return scroll;
        }
    }
    return nullptr;
}

void WWidget::ScrollSelfDelta( float _Delta ) {
    WScroll * scroll = FindScrollWidget();
    if ( scroll ) {
        scroll->ScrollDelta( Float2( 0.0f, _Delta ) );
    }
}

void WWidget::OnMouseWheelEvent( SMouseWheelEvent const & _Event, double _TimeStamp ) {
    if ( _Event.WheelY < 0 ) {
        ScrollSelfDelta( -20 );
    }
    else if ( _Event.WheelY > 0 ) {
        ScrollSelfDelta( 20 );
    }
}

void WWidget::OnMouseMoveEvent( SMouseMoveEvent const & _Event, double _TimeStamp ) {

}

void WWidget::OnCharEvent( SCharEvent const & _Event, double _TimeStamp ) {

}

void WWidget::OnDragEvent( Float2 & _Position ) {

}

void WWidget::OnFocusLost() {

}

void WWidget::OnFocusReceive() {

}

void WWidget::OnWindowHovered( bool _Hovered ) {
    if ( _Hovered ) {
        Desktop->SetCursor( DRAW_CURSOR_ARROW );
    }
}

void WWidget::OnDrawEvent( ACanvas & _Canvas ) {
    DrawDecorates( _Canvas );
}

void WWidget::OnTransformDirty() {
    //GLogger.Printf( "Widget transform dirty\n" );
}

void WWidget::AdjustSizeAndPosition( Float2 const & _AvailableSize, Float2 & _Size, Float2 & _Position ) {

}

void WWidget::DrawDecorates( ACanvas & _Canvas ) {
    for ( WDecorate * d : Decorates ) {
        d->OnDrawEvent( _Canvas );
    }
}

void WWidget::MarkGridLayoutDirty() {
    if ( Layout == WIDGET_LAYOUT_GRID ) {
        bLayoutDirty = true;
        MarkTransformDirtyChilds();
    }
}

void WWidget::MarkVHLayoutDirty() {
    if ( Layout == WIDGET_LAYOUT_HORIZONTAL
         || Layout == WIDGET_LAYOUT_HORIZONTAL_WRAP
         || Layout == WIDGET_LAYOUT_VERTICAL
         || Layout == WIDGET_LAYOUT_VERTICAL_WRAP ) {
        bLayoutDirty = true;
        MarkTransformDirtyChilds();
    }
}

void WWidget::MarkImageLayoutDirty() {
    if ( Layout == WIDGET_LAYOUT_IMAGE ) {
        bLayoutDirty = true;
        MarkTransformDirtyChilds();
    }
}

WWidget & WWidget::SetGridSize( int _ColumnsCount, int _RowsCount ) {
    ColumnsCount = Math::Clamp( _ColumnsCount, 1, MAX_COLUMNS );
    RowsCount = Math::Clamp( _RowsCount, 1, MAX_ROWS );
    MarkGridLayoutDirty();
    return *this;
}

WWidget & WWidget::SetColumnWidth( int _ColumnIndex, float _Width ) {
    if ( _ColumnIndex < 0 || _ColumnIndex >= ColumnsCount ) {
        return *this;
    }

    int oldSize = Columns.Size();

    if ( oldSize <= _ColumnIndex ) {
        Columns.Resize( _ColumnIndex + 1 );

        memset( Columns.ToPtr() + oldSize, 0, ( Columns.Size() - oldSize ) * sizeof( Columns[0] ) );
    }

    Columns[ _ColumnIndex ].Size = Math::Max( _Width, 0.0f );

    MarkGridLayoutDirty();

    return *this;
}

WWidget & WWidget::SetRowWidth( int _RowIndex, float _Width ) {
    if ( _RowIndex < 0 || _RowIndex >= RowsCount ) {
        return *this;
    }

    int oldSize = Rows.Size();

    if ( oldSize <= _RowIndex ) {
        Rows.Resize( _RowIndex + 1 );

        memset( Rows.ToPtr() + oldSize, 0, ( Rows.Size() - oldSize ) * sizeof( Rows[0] ) );
    }

    Rows[ _RowIndex ].Size = Math::Max( _Width, 0.0f );

    MarkGridLayoutDirty();

    return *this;
}

WWidget & WWidget::SetFitColumns( bool _FitColumns ) {
    if ( bFitColumns != _FitColumns ) {
        bFitColumns = _FitColumns;
        MarkGridLayoutDirty();
    }
    return *this;
}

WWidget & WWidget::SetFitRows( bool _FitRows ) {
    if ( bFitRows != _FitRows ) {
        bFitRows = _FitRows;
        MarkGridLayoutDirty();
    }
    return *this;
}

void WWidget::GetCellRect( int _ColumnIndex, int _RowIndex, Float2 & _Mins, Float2 & _Maxs ) const {
    const_cast< WWidget * >( this )->UpdateLayoutIfDirty();

    int numColumns = Math::Min( ColumnsCount, Columns.Size() );
    int numRows = Math::Min( RowsCount, Rows.Size() );

    if ( _ColumnIndex < 0 || _RowIndex < 0 || numColumns == 0 || numRows == 0 ) {
        _Mins = _Maxs = Float2(0.0f);
        return;
    }

    int col = _ColumnIndex, row = _RowIndex;

    _Mins.X = Columns[ col >= numColumns ? numColumns - 1 : col ].Offset;
    _Mins.Y = Rows[ row >= numRows ? numRows - 1 : row ].Offset;
    _Maxs.X = _Mins.X + ( col >= numColumns ? 0.0f : Columns[ col ].ActualSize );
    _Maxs.Y = _Mins.Y + ( row >= numRows ? 0.0f : Rows[ row ].ActualSize );
}

WWidget & WWidget::SetAutoWidth( bool _AutoWidth ) {
    if ( bAutoWidth != _AutoWidth ) {
        bAutoWidth = _AutoWidth;

        WWidget * root = GetRoot();
        if ( root ) {
            root->MarkTransformDirtyChilds();
        }
    }
    return *this;
}

WWidget & WWidget::SetAutoHeight( bool _AutoHeight ) {
    if ( bAutoHeight != _AutoHeight ) {
        bAutoHeight = _AutoHeight;

        WWidget * root = GetRoot();
        if ( root ) {
            root->MarkTransformDirtyChilds();
        }
    }
    return *this;
}

WWidget & WWidget::SetClampWidth( bool _ClampWidth ) {
    if ( bClampWidth != _ClampWidth ) {
        bClampWidth = _ClampWidth;
        MarkTransformDirty();
    }
    return *this;
}

WWidget & WWidget::SetClampHeight( bool _ClampHeight ) {
    if ( bClampHeight != _ClampHeight ) {
        bClampHeight = _ClampHeight;
        MarkTransformDirty();
    }
    return *this;
}

WWidget & WWidget::SetHorizontalPadding( float _Padding ) {
    HorizontalPadding = _Padding;
    MarkVHLayoutDirty();
    return *this;
}

WWidget & WWidget::SetVerticalPadding( float _Padding ) {
    VerticalPadding = _Padding;
    MarkVHLayoutDirty();
    return *this;
}

WWidget & WWidget::SetImageSize( float _Width, float _Height ) {
    return SetImageSize( Float2( _Width, _Height ) );
}

WWidget & WWidget::SetImageSize( Float2 const & _ImageSize ) {
    ImageSize = _ImageSize;
    if ( ImageSize.X < 1 ) {
        ImageSize.X = 1;
    }
    if ( ImageSize.Y < 1 ) {
        ImageSize.Y = 1;
    }
    MarkImageLayoutDirty();
    return *this;
}

void WWidget::GetLayoutRect( Float2 & _Mins, Float2 & _Maxs ) const {
    if ( !Parent ) {
        _Mins.Clear();
        _Maxs.Clear();
        return;
    }

    switch ( Parent->Layout ) {
    case WIDGET_LAYOUT_EXPLICIT:
    case WIDGET_LAYOUT_HORIZONTAL:
    case WIDGET_LAYOUT_HORIZONTAL_WRAP:
    case WIDGET_LAYOUT_VERTICAL:
    case WIDGET_LAYOUT_VERTICAL_WRAP:
    case WIDGET_LAYOUT_IMAGE:
    case WIDGET_LAYOUT_CUSTOM:
        Parent->GetDesktopRect( _Mins, _Maxs, true );
        break;
    case WIDGET_LAYOUT_GRID: {
        Parent->GetCellRect( Column, Row, _Mins, _Maxs );

        Float2 pos = Parent->GetClientPosition();
        _Mins += pos;
        _Maxs += pos;
        }
        break;
    default:
        AN_Assert( 0 );
        Parent->GetDesktopRect( _Mins, _Maxs, true );
        break;
    }
}

void WWidget::MarkTransformDirty() {

    if ( Parent && ( Parent->bAutoWidth || Parent->bAutoHeight ) ) {
        Parent->MarkTransformDirty();
        return;
    }

    MarkTransformDirty_r();
}

void WWidget::MarkTransformDirty_r() {
    WWidget * node = this;
    WWidget * nextNode;
    int numChilds;
    while ( !node->bTransformDirty ) {
        node->bTransformDirty = true;
        node->bLayoutDirty = true;
        node->OnTransformDirty();
        numChilds = node->Childs.Size();
        if ( numChilds > 0 ) {
            nextNode = node->Childs[ 0 ];
            for ( int i = 1 ; i < numChilds ; i++ ) {
                node->Childs[ i ]->MarkTransformDirty_r();
            }
            node = nextNode;
        } else {
            return;
        }
    }
}

void WWidget::MarkTransformDirtyChilds() {
    for ( WWidget * child : Childs ) {
        child->MarkTransformDirty();
    }
}

void WWidget::UpdateLayoutIfDirty() {
    if ( bLayoutDirty ) {
        UpdateLayout();
    }
}

void WWidget::UpdateLayout() {
    bLayoutDirty = false;

    if ( Layout == WIDGET_LAYOUT_GRID ) {
        int numColumns = Math::Min( ColumnsCount, Columns.Size() );
        int numRows = Math::Min( RowsCount, Rows.Size() );

        if ( bAutoWidth ) {
            for ( int i = 0 ; i < numColumns ; i++ ) {
                Columns[i].ActualSize = 0;
            }
            for ( WWidget * child : Childs ) {
                if ( child->IsCollapsed() ) {
                    continue;
                }
                if ( child->Column < Columns.Size() ) {
                    float w = child->CalcContentWidth();
                    Columns[ child->Column ].ActualSize = Math::Max( Columns[ child->Column ].ActualSize, w );
                }
            }
        } else if ( bFitColumns ) {
            float sumWidth = 0;
            for ( int i = 0; i < numColumns; i++ ) {
                sumWidth += Columns[ i ].Size;
            }
            float norm = ( sumWidth > 0 ) ? GetAvailableWidth() / sumWidth : 0;
            for ( int i = 0; i < numColumns; i++ ) {
                Columns[ i ].ActualSize = Columns[ i ].Size * norm;
            }
        } else {
            for ( int i = 0; i < numColumns; i++ ) {
                Columns[ i ].ActualSize = Columns[ i ].Size;
            }
        }

        Columns[ 0 ].Offset = 0;
        for ( int i = 1 ; i < numColumns; i++ ) {
            Columns[ i ].Offset = Columns[ i - 1 ].Offset + Columns[ i - 1 ].ActualSize;
        }

        if ( bAutoHeight ) {
            for ( int i = 0 ; i < numRows ; i++ ) {
                Rows[i].ActualSize = 0;
            }
            for ( WWidget * child : Childs ) {
                if ( child->IsCollapsed() ) {
                    continue;
                }
                if ( child->Row < Rows.Size() ) {
                    float h = child->CalcContentHeight();
                    Rows[ child->Row ].ActualSize = Math::Max( Rows[ child->Row ].ActualSize, h );
                }
            }
        } else if ( bFitRows ) {
            float sumWidth = 0;
            for ( int i = 0; i < numRows; i++ ) {
                sumWidth += Rows[ i ].Size;
            }
            float norm = ( sumWidth > 0 ) ? GetAvailableHeight() / sumWidth : 0;
            for ( int i = 0; i < numRows; i++ ) {
                Rows[ i ].ActualSize = Rows[ i ].Size * norm;
            }
        } else {
            for ( int i = 0; i < RowsCount; i++ ) {
                Rows[ i ].ActualSize = Rows[ i ].Size;
            }
        }

        Rows[ 0 ].Offset = 0;
        for ( int i = 1 ; i < numRows; i++ ) {
            Rows[ i ].Offset = Rows[ i - 1 ].Offset + Rows[ i - 1 ].ActualSize;
        }
    } else if ( Layout == WIDGET_LAYOUT_HORIZONTAL || Layout == WIDGET_LAYOUT_HORIZONTAL_WRAP ) {
        float offsetX = 0;
        float offsetY = 0;
        float availWidth = 0;
        float maxHeight = 0;

        bool bCanWrap = ( Layout == WIDGET_LAYOUT_HORIZONTAL_WRAP ) && !bAutoWidth;
        if ( bCanWrap ) {
            availWidth = GetAvailableWidth();
        }

        for ( int i = 0 ; i < LayoutSlots.Size() ; i++ ) {
            WWidget * w = LayoutSlots[i];
            WWidget * next = (i == LayoutSlots.Size()-1) ? nullptr : LayoutSlots[i+1];

            if ( w->IsCollapsed() ) {
                continue;
            }

            w->LayoutOffset.X = offsetX;
            w->LayoutOffset.Y = offsetY;

            offsetX += w->CalcContentWidth() + HorizontalPadding;

            if ( bCanWrap && next ) {
                maxHeight = Math::Max( maxHeight, w->CalcContentHeight() );
                if ( offsetX + next->CalcContentWidth() >= availWidth ) {
                    offsetX = 0;
                    offsetY += maxHeight + VerticalPadding;
                    maxHeight = 0;
                }
            }
        }
    } else if ( Layout == WIDGET_LAYOUT_VERTICAL || Layout == WIDGET_LAYOUT_VERTICAL_WRAP ) {
        float offsetX = 0;
        float offsetY = 0;
        float availHeight = 0;
        float maxWidth = 0;

        bool bCanWrap = ( Layout == WIDGET_LAYOUT_VERTICAL_WRAP ) && !bAutoHeight;
        if ( bCanWrap ) {
            availHeight = GetAvailableHeight();
        }

        for ( int i = 0 ; i < LayoutSlots.Size() ; i++ ) {
            WWidget * w = LayoutSlots[i];
            WWidget * next = (i == LayoutSlots.Size()-1) ? nullptr : LayoutSlots[i+1];

            if ( w->IsCollapsed() ) {
                continue;
            }

            w->LayoutOffset.X = offsetX;
            w->LayoutOffset.Y = offsetY;

            offsetY += w->CalcContentHeight() + VerticalPadding;

            if ( bCanWrap && next ) {
                maxWidth = Math::Max( maxWidth, w->CalcContentWidth() );
                if ( offsetY + next->CalcContentHeight() >= availHeight ) {
                    offsetY = 0;
                    offsetX += maxWidth + HorizontalPadding;
                    maxWidth = 0;
                }
            }
        }
    }
}

float WWidget::CalcContentWidth() {
    float contentWidth;

    if ( !bAutoWidth ) {
        contentWidth = Size.X;
    }

    // Если установлен лэйаут WIDGET_LAYOUT_IMAGE, то размер окна устанавливается равным ImageSize.
    else if ( Layout == WIDGET_LAYOUT_IMAGE ) {
        contentWidth = ImageSize.X;
    }

    // Если установлен лэйаут WIDGET_LAYOUT_GRID, то размер окна устанавливается равным размеру сетки.
    else if ( Layout == WIDGET_LAYOUT_GRID ) {
        int numColumns = Math::Min( ColumnsCount, Columns.Size() );

        if ( numColumns == 0 ) {
            contentWidth = Size.X;
        } else {
#if 0
            contentWidth = 0;
            for ( int i = 0 ; i < numColumns ; i++ ) {
                contentWidth += Columns[ i ].Size;
            }
#else
            for ( int i = 0 ; i < numColumns ; i++ ) {
                Columns[i].ActualSize = 0;
            }
            for ( WWidget * child : Childs ) {
                if ( child->IsCollapsed() ) {
                    continue;
                }

                if ( child->Column < Columns.Size() ) {
                    float w = child->CalcContentWidth();
                    Columns[ child->Column ].ActualSize = Math::Max( Columns[ child->Column ].ActualSize, w );
                }
            }
            contentWidth = 0;
            for ( int i = 0 ; i < numColumns ; i++ ) {
                contentWidth += Columns[i].ActualSize;
            }
#endif
        }
    }
    else {
        contentWidth = 0;

        float offsetX = 0;

        for ( WWidget * child : LayoutSlots ) {
            if ( child->IsCollapsed() ) {
                continue;
            }

            float w = child->CalcContentWidth();

            float x;

            switch ( Layout ) {
            case WIDGET_LAYOUT_HORIZONTAL:
            case WIDGET_LAYOUT_HORIZONTAL_WRAP:
                x = offsetX;
                offsetX += w + HorizontalPadding;
                break;

            default:
                if ( child->HorizontalAlignment == WIDGET_ALIGNMENT_NONE ) {
                    x = child->Position.X;
                } else {
                    x = 0;
                }
                break;
            }

            contentWidth = Math::Max( contentWidth, x + w );
        }
    }

    return contentWidth + Margin.X + Margin.Z;
}

float WWidget::CalcContentHeight() {
    float contentHeight;

    if ( !bAutoHeight ) {
        contentHeight = Size.Y;
    }

    // Если установлен лэйаут WIDGET_LAYOUT_IMAGE, то размер окна устанавливается равным ImageSize.
    else if ( Layout == WIDGET_LAYOUT_IMAGE ) {
        contentHeight = ImageSize.Y;
    }

    // Если установлен лэйаут WIDGET_LAYOUT_GRID, то размер окна устанавливается равным размеру сетки.
    else if ( Layout == WIDGET_LAYOUT_GRID ) {
        int numRows = Math::Min( RowsCount, Rows.Size() );

        if ( numRows == 0 ) {
            contentHeight = Size.Y;
        } else {
#if 0
            contentHeight = 0;
            for ( int i = 0 ; i < numRows ; i++ ) {
                contentHeight += Rows[ i ].Size;
            }
#else
            for ( int i = 0 ; i < numRows ; i++ ) {
                Rows[i].ActualSize = 0;
            }
            for ( WWidget * child : Childs ) {
                if ( child->IsCollapsed() ) {
                    continue;
                }
                if ( child->Row < Rows.Size() ) {
                    float h = child->CalcContentHeight();
                    Rows[ child->Row ].ActualSize = Math::Max( Rows[ child->Row ].ActualSize, h );
                }
            }
            contentHeight = 0;
            for ( int i = 0 ; i < numRows ; i++ ) {
                contentHeight += Rows[i].ActualSize;
            }
#endif
        }
    }
    else {
        contentHeight = 0;

        float offsetY = 0;
        for ( WWidget * child : LayoutSlots ) {
            if ( child->IsCollapsed() ) {
                continue;
            }

            float h = child->CalcContentHeight();

            float y;

            switch ( Layout ) {
            case WIDGET_LAYOUT_VERTICAL:
            case WIDGET_LAYOUT_VERTICAL_WRAP:
                y = offsetY;
                offsetY += h + VerticalPadding;
                break;

            default:
                if ( child->VerticalAlignment == WIDGET_ALIGNMENT_NONE ) {
                    y = child->Position.Y;
                } else {
                    y = 0;
                }
                break;
            }

            contentHeight = Math::Max( contentHeight, y + h );
        }
    }

    return contentHeight + Margin.Y + Margin.W;
}








#include <Engine/Widgets/Public/WWindow.h>
#include <Engine/Widgets/Public/WButton.h>
#include <Engine/Widgets/Public/WScroll.h>
#include <Engine/Widgets/Public/WSlider.h>
#include <Engine/Widgets/Public/WTextEdit.h>



WWidget & ScrollTest2() {
    return  WWidget::New< WWindow >()
            .SetCaptionText( "Test Scroll" )
            .SetCaptionHeight( 24 )
            .SetBackgroundColor( AColor4( 0.5f,0.5f,0.5f ) )
            .SetStyle( WIDGET_STYLE_RESIZABLE )
            .SetSize( 400, 300 )
            .SetLayout( WIDGET_LAYOUT_GRID )
            .SetGridSize( 2, 1 )
            .SetColumnWidth( 0, 270 )
            .SetColumnWidth( 1, 30 )
            .SetRowWidth( 0, 1 )
            .SetFitColumns( true )
            .SetFitRows( true )
            //.SetClampWidth(true)
            .SetAutoWidth( true )
            //.SetAutoHeight( true )
#if 0
            [
                WWidget::New()
                .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                .SetGridOffset( 0, 0 )
                .SetLayout( WIDGET_LAYOUT_IMAGE )
                .SetImageSize( 640, 480 )
                [
                    WDecorate::New< WBorderDecorate >()
                    .SetColor( 0xff00ffff )
                    .SetFillBackground( true )
                    .SetBackgroundColor( 0xff00ff00 )
                    .SetThickness( 1 )
                ]
                [
                    WDecorate::New< WTextDecorate >()
                    .SetText( "Content view" )
                    .SetColor( 0xff000000 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                ]
                [
                    WWidget::New< WButton >()
                    .SetText( "1" )
                    .SetSize( 64, 64 )
                    .SetPosition( 640-64, 480-64 )
                    //.SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    //.SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                    .SetEnabled( false )
                ]
                [
                    WWidget::New< WButton >()
                    .SetText( "2" )
                    .SetSize( 64, 64 )
                    .SetPosition( 640/2-64/2, 480/2-64/2 )
                    //.SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    //.SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
            ]
#endif
#if 1
            [
                WWidget::New()
                //.SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                .SetGridOffset( 0, 0 )
                .SetLayout( WIDGET_LAYOUT_HORIZONTAL )
                .SetHorizontalPadding( 8 )
                .SetVerticalPadding( 4 )
                .SetAutoWidth( true )
                //.SetAutoHeight( true )
                [
                    WDecorate::New< WBorderDecorate >()
                    .SetColor( AColor4( 1,1,0 ) )
                    .SetFillBackground( true )
                    .SetBackgroundColor( AColor4( 0,1,0 ) )
                    .SetThickness( 1 )
                ]
                [
                    WDecorate::New< WTextDecorate >()
                    .SetText( "Content view" )
                    .SetColor( AColor4::Black() )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "1" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "2" )
                    .SetSize( 200, 50 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "3" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                    //.SetCollapsed()
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "4" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "5" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_TOP )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "6" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_BOTTOM )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "7" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "8" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "9" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_BOTTOM )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "10" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "11" )
                    .SetSize( 100, 30 )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_CENTER )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                ]
            ]
#endif
            [
                WWidget::New()
                .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                .SetGridOffset( 1, 0 )
                .SetGridSize( 1, 3 )
                .SetColumnWidth( 0, 1 )
                .SetRowWidth( 0, 0.2f )
                .SetRowWidth( 1, 0.6f )
                .SetRowWidth( 2, 0.2f )
                .SetFitColumns( true )
                .SetFitRows( true )
                .SetLayout( WIDGET_LAYOUT_GRID )
                [
                    WDecorate::New< WBorderDecorate >()
                    .SetColor( AColor4( 1,0,0 ) )
                    .SetFillBackground( true )
                    .SetBackgroundColor( AColor4( 1,0,1 ) )
                    .SetThickness( 1 )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "Up" )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetGridOffset( 0, 0 )
                ]
                [
                    WWidget::New< WTextButton >()
                    .SetText( "Down" )
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetGridOffset( 0, 2 )
                ]
                [
                    WWidget::New()
                    .SetStyle( WIDGET_STYLE_FOREGROUND )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetGridOffset( 0, 1 )
                    .AddDecorate( &(*CreateInstanceOf< WBorderDecorate >())
                                  .SetColor( AColor4( 0,1,0 ) )
                                  .SetFillBackground( true )
                                  .SetBackgroundColor( AColor4::Black() )
                                  .SetThickness( 1 )
                                  .SetRounding( 0 )
                                  .SetRoundingCorners( CORNER_ROUND_NONE ) )
                ]
            ];
}

WWidget & ScrollTest() {

    WWidget & contentWidget =
            WWidget::New()
            .SetLayout( WIDGET_LAYOUT_VERTICAL )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
            //.SetAutoWidth( true )
            .SetAutoHeight( true )
            //.SetMaxSize( 128, 128 )
            .SetPosition( 0, 0 )
            [
                WDecorate::New< WBorderDecorate >()
                .SetColor( AColor4( 0.5f,0.5f,0.5f,0.5f ) )
                .SetFillBackground( true )
                .SetBackgroundColor( AColor4( 0.3f,0.3f,0.3f ) )
                .SetThickness( 1 )
            ]
//            [
//                ScrollTest2()
//            ]
    ;

    contentWidget[
            WWidget::New< WSlider >()
            .SetMinValue( 30 )
            .SetMaxValue( 100 )
            .SetStep( 10 )
            .SetSize( 400, 32 )
            //.SetVerticalOrientation( true )
            //.SetSize( 32, 400 )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
            .SetStyle( WIDGET_STYLE_BACKGROUND )
            ];

    contentWidget[
        WWidget::New< WScroll >()
            
            .SetAutoScrollH( true )
            .SetAutoScrollV( true )
            .SetScrollbarSize( 12 )
            .SetButtonWidth( 12 )
            .SetShowButtons( true )
            .SetSliderRounding( 4 )
            .SetContentWidget
            (
                WWidget::New< WTextEdit >()
                //.SetSize( 0, 0 )
                //.SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                .SetStyle( WIDGET_STYLE_BACKGROUND )
            )
            .SetSize( 400, 600 )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
            //.SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
            .SetStyle( WIDGET_STYLE_BACKGROUND )
    ];

    for (int i =0;i<100;i++ ) {
        contentWidget[
                WWidget::New< WTextButton >()
                .SetText( AString::Fmt( "test button %d", i ) )
                .SetSize( 400, 32 )
                .SetHorizontalAlignment( WIDGET_ALIGNMENT_CENTER )
                .SetStyle( WIDGET_STYLE_BACKGROUND )
                ];
    }

    return WWidget::New< WWindow >()
                .SetCaptionText( "Test Scroll" )
                .SetCaptionHeight( 24 )
                .SetBackgroundColor( AColor4( 0.5f,0.5f,0.5f ) )
                .SetStyle( WIDGET_STYLE_RESIZABLE )
                .SetLayout( WIDGET_LAYOUT_EXPLICIT )
                .SetSize( 320,240 )
                .SetMaximized()
                //.SetAutoWidth( true )
                //.SetAutoHeight( true )
                [
                    WWidget::New< WScroll >()
                    .SetAutoScrollH( true )
                    .SetAutoScrollV( true )
                    .SetScrollbarSize( 12 )
                    .SetButtonWidth( 12 )
                    .SetShowButtons( true )
                    .SetSliderRounding( 4 )
                    .SetContentWidget
                    (
                        contentWidget
                    )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                ]
            ;

#if 0

    WWidget & scrollView =
            WWidget::New()
            .SetStyle( WIDGET_STYLE_TRANSPARENT )
            .SetLayout( WIDGET_LAYOUT_EXPLICIT )
            .SetSize( 300, 250 )
            .SetPosition( 0, 0 )
            .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
            .SetMargin( 4,4,20,20 )
            [
                contentWidget
            ];

    WWidget & scrollBarV =
            WWidget::New< WScrollBar >()
            .SetView( &scrollView )
            .SetContent( &contentWidget )
            .SetSliderRounding( 4 )
            .SetShowButtons( true )
            .SetOrientation( SCROLL_BAR_VERTICAL )
            .SetSize( 12, 250 )
            .SetPosition( 0, 0 )
            .SetStyle( WIDGET_STYLE_FOREGROUND )
            .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_RIGHT )
            .SetMargin(4,4,4,4);

    WWidget & scrollBarH =
            WWidget::New< WScrollBar >()
            .SetView( &scrollView )
            .SetContent( &contentWidget )
            .SetSliderRounding( 4 )
            .SetShowButtons( true )
            .SetOrientation( SCROLL_BAR_HORIZONTAL )
            .SetSize( 250, 12 )
            .SetPosition( 0, 0 )
            .SetStyle( WIDGET_STYLE_FOREGROUND )
            .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
            .SetVerticalAlignment( WIDGET_ALIGNMENT_BOTTOM )
            .SetMargin(4,4,4,4);

    return WWidget::New< WWindow >()
                .SetCaptionText( "Test Scroll" )
                .SetCaptionHeight( 24 )
                .SetBackgroundColor( AColor4( 0.5f,0.5f,0.5f ) )
                .SetStyle( WIDGET_STYLE_RESIZABLE )
                .SetLayout( WIDGET_LAYOUT_EXPLICIT )
                .SetAutoWidth( true )
                .SetAutoHeight( true )
                [
                    scrollView
                ]
                [
                    WWidget::New()
                    .SetStyle( WIDGET_STYLE_TRANSPARENT )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetMargin(0,0,0,20)
                    [
                        scrollBarV
                    ]
                ]
                [
                    WWidget::New()
                    .SetStyle( WIDGET_STYLE_TRANSPARENT )
                    .SetHorizontalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetVerticalAlignment( WIDGET_ALIGNMENT_STRETCH )
                    .SetMargin(0,0,20,0)
                    [
                        scrollBarH
                    ]
                ]
            ;
#endif
}


#if 0
AN_CLASS_META( WMenuItem )

WMenuItem::WMenuItem() {
    State = ST_RELEASED;
    Color = AColor4::White();
    HoverColor = AColor4( 1,1,0.5f,1 );
    PressedColor = AColor4( 1,1,0.2f,1 );
    TextColor = AColor4::Black();
    BorderColor = AColor4::Black();
    Rounding = 8;
    RoundingCorners = CORNER_ROUND_ALL;
    BorderThickness = 1;
}

WMenuItem::~WMenuItem() {
}

WMenuItem & WMenuItem::SetText( const char * _Text ) {
    Text = _Text;
    return *this;
}

WMenuItem & WMenuItem::SetColor( AColor4 const & _Color ) {
    Color = _Color;
    return *this;
}

WMenuItem & WMenuItem::SetHoverColor( AColor4 const & _Color ) {
    HoverColor = _Color;
    return *this;
}

WMenuItem & WMenuItem::SetPressedColor( AColor4 const & _Color ) {
    PressedColor = _Color;
    return *this;
}

WMenuItem & WMenuItem::SetTextColor( AColor4 const & _Color ) {
    TextColor = _Color;
    return *this;
}

WMenuItem & WMenuItem::SetBorderColor( AColor4 const & _Color ) {
    BorderColor = _Color;
    return *this;
}

WMenuItem & WMenuItem::SetRounding( float _Rounding ) {
    Rounding = _Rounding;
    return *this;
}

WMenuItem & WMenuItem::SetRoundingCorners( EDrawCornerFlags _RoundingCorners ) {
    RoundingCorners = _RoundingCorners;
    return *this;
}

WMenuItem & WMenuItem::SetBorderThickness( float _BorderThickness ) {
    BorderThickness = _BorderThickness;
    return *this;
}

void WMenuItem::OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) {
    if ( _Event.Action == IE_Press ) {
        if ( _Event.Button == 0 ) {
            State = ST_PRESSED;
        }
    } else if ( _Event.Action == IE_Release ) {
        if ( _Event.Button == 0 && State == ST_PRESSED && IsHoveredByCursor() ) {

            State = ST_RELEASED;

            E_OnButtonClick.Dispatch();
        } else {
            State = ST_RELEASED;
        }
    }
}

void WMenuItem::OnDrawEvent( ACanvas & _Canvas ) {
    AColor4 bgColor;

    if ( IsHoveredByCursor() && !IsDisabled() ) {
        if ( State == ST_PRESSED ) {
            bgColor = PressedColor;
        } else {
            bgColor = HoverColor;
        }
    } else {
        bgColor = Color;
    }

    Float2 mins, maxs;

    GetDesktopRect( mins, maxs, true );

    FFontNew * font = ACanvas::GetDefaultFont();

    float width = GetClientWidth();
    float height = GetClientHeight();

    Float2 size = font->CalcTextSizeA( font->FontSize, width, 0, Text.Begin(), Text.End() );

    _Canvas.DrawRectFilled( mins, maxs, bgColor, Rounding, RoundingCorners );
    if ( BorderThickness > 0.0f ) {
        _Canvas.DrawRect( mins, maxs, BorderColor, Rounding, RoundingCorners, BorderThickness );
    }
    _Canvas.DrawTextUTF8( mins + Float2( width - size.X, height - size.Y ) * 0.5f, TextColor, Text.Begin(), Text.End() );
}
#endif
