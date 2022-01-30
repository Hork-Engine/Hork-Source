/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "WSlider.h"
#include "WDesktop.h"
#include "FrameLoop.h"
#include "InputDefs.h"

AN_CLASS_META( WSlider )

WSlider::WSlider() {
    Action = A_NONE;
    DragCursor = 0.0f;
    MinValue = 0;
    MaxValue = 1;
    Step = 0;
    Value = 0;
    SliderWidth = 12;
    bVerticalOrientation = false;
    bUpdateGeometry = true;
    BackgroundColor = Color4(0.4f, 0.4f, 0.4f, 1.0f);
    SliderColor = Color4::White();
    LineColor = Color4::White();
}

WSlider::~WSlider() {

}

WSlider & WSlider::SetValue( float _Value ) {
    float NewValue = Math::Clamp( Step > 0.0f ? Math::Snap( _Value, Step ) : _Value, MinValue, MaxValue );

    if ( Value != NewValue ) {
        Value = NewValue;
        bUpdateGeometry = true;

        E_OnUpdateValue.Dispatch( Value );
    }

    return *this;
}

WSlider & WSlider::SetMaxValue( float _MaxValue ) {
    MaxValue = _MaxValue;

    // Correct min value
    if ( MinValue > MaxValue ) {
        MinValue = MaxValue;
    }

    // Update value
    SetValue( Value );

    return *this;
}

WSlider & WSlider::SetMinValue( float _MinValue ) {
    MinValue = _MinValue;

    // Correct max value
    if ( MaxValue < MinValue ) {
        MaxValue = MinValue;
    }

    // Update value
    SetValue( Value );

    return *this;
}

WSlider & WSlider::SetStep( float _Step ) {
    Step = _Step;
    return *this;
}

WSlider & WSlider::SetSliderWidth( float _Width ) {
    SliderWidth = Math::Max( _Width, 1.0f );
    bUpdateGeometry = true;
    return *this;
}

WSlider & WSlider::SetVerticalOrientation( bool _VerticalOrientation ) {
    if ( bVerticalOrientation != _VerticalOrientation ) {
        bVerticalOrientation = _VerticalOrientation;
        bUpdateGeometry = true;
    }
    return *this;
}

WSlider & WSlider::SetBackgroundColor( Color4 const & _Color ) {
    BackgroundColor = _Color;
    return *this;
}

WSlider & WSlider::SetSliderColor( Color4 const & _Color ) {
    SliderColor = _Color;
    return *this;
}

WSlider & WSlider::SetLineColor( Color4 const & _Color ) {
    LineColor = _Color;
    return *this;
}

void WSlider::UpdateSliderGeometry() {
    bUpdateGeometry = false;

    Float2 mins, maxs;

    GetDesktopRect( mins, maxs, false );

    Platform::ZeroMem( &Geometry, sizeof( Geometry ) );

    if ( bVerticalOrientation ) {
        float AvailableWidth = maxs.Y - mins.Y;
        float SliderActualWidth = Math::Min( AvailableWidth / 4.0f, SliderWidth );
        float SliderHalfWidth = SliderActualWidth * 0.5f;

        Geometry.BgMins = mins;
        Geometry.BgMaxs = maxs;

        Geometry.BgMins.Y += SliderHalfWidth;
        Geometry.BgMaxs.Y -= SliderHalfWidth;

        float SliderBarSize = Geometry.BgMaxs.Y - Geometry.BgMins.Y;
        float SliderPos = ( Value - MinValue ) / ( MaxValue - MinValue ) * SliderBarSize;

        Geometry.SliderMins.X = Geometry.BgMins.X;
        Geometry.SliderMins.Y = Geometry.BgMins.Y + SliderPos - SliderHalfWidth;

        Geometry.SliderMaxs.X = Geometry.BgMaxs.X;
        Geometry.SliderMaxs.Y = Geometry.SliderMins.Y + SliderActualWidth;
    } else {
        float AvailableWidth = maxs.X - mins.X;
        float SliderActualWidth = Math::Min( AvailableWidth / 4.0f, SliderWidth );
        float SliderHalfWidth = SliderActualWidth * 0.5f;

        Geometry.BgMins = mins;
        Geometry.BgMaxs = maxs;

        Geometry.BgMins.X += SliderHalfWidth;
        Geometry.BgMaxs.X -= SliderHalfWidth;

        float SliderBarSize = Geometry.BgMaxs.X - Geometry.BgMins.X;
        float SliderPos = ( Value - MinValue ) / ( MaxValue - MinValue ) * SliderBarSize;

        Geometry.SliderMins.X = Geometry.BgMins.X + SliderPos - SliderHalfWidth;
        Geometry.SliderMins.Y = Geometry.BgMins.Y;

        Geometry.SliderMaxs.X = Geometry.SliderMins.X + SliderActualWidth;
        Geometry.SliderMaxs.Y = Geometry.BgMaxs.Y;
    }
}

void WSlider::UpdateSliderGeometryIfDirty() {
    if ( bUpdateGeometry ) {
        UpdateSliderGeometry();
    }
}

SSliderGeometry const & WSlider::GetSliderGeometry() const{
    const_cast< WSlider * >( this )->UpdateSliderGeometryIfDirty();

    return Geometry;
}

void WSlider::OnTransformDirty() {
    Super::OnTransformDirty();

    bUpdateGeometry = true;
}

void WSlider::MoveSlider( float Vec ) {
    SSliderGeometry const & geometry = GetSliderGeometry();

    float SliderBarSize = bVerticalOrientation ? geometry.BgMaxs.Y - geometry.BgMins.Y
                                               : geometry.BgMaxs.X - geometry.BgMins.X;

    SetValue( Vec * ( MaxValue - MinValue ) / SliderBarSize + MinValue );
}

AN_FORCEINLINE bool InRect( Float2 const & _Mins, Float2 const & _Maxs, Float2 const & _Position ) {
    return _Position.X >= _Mins.X && _Position.X < _Maxs.X
            && _Position.Y >= _Mins.Y && _Position.Y < _Maxs.Y;
}

void WSlider::OnMouseButtonEvent( SMouseButtonEvent const & _Event, double _TimeStamp ) {
    Action = A_NONE;

    
    if ( _Event.Button != MOUSE_BUTTON_LEFT || _Event.Action != IA_PRESS ) {
        return;
    }

    Float2 const & CursorPos = GetDesktop()->GetCursorPosition();

    SSliderGeometry const & geometry = GetSliderGeometry();

    if ( InRect( geometry.SliderMins, geometry.SliderMaxs, CursorPos ) ) {
        Action = A_MOVE;

        float SliderBarSize = bVerticalOrientation ? geometry.BgMaxs.Y - geometry.BgMins.Y
                                                   : geometry.BgMaxs.X - geometry.BgMins.X;

        float Cursor = bVerticalOrientation ? CursorPos.Y : CursorPos.X;

        DragCursor = Cursor - ( ( Value - MinValue ) / ( MaxValue - MinValue ) * SliderBarSize );
        return;
    }

    if ( InRect( geometry.BgMins, geometry.BgMaxs, CursorPos ) ) {

        float CursorLocalOffset = bVerticalOrientation ? CursorPos.Y - geometry.BgMins.Y
                                                       : CursorPos.X - geometry.BgMins.X;

        MoveSlider( CursorLocalOffset );
        return;
    }
}

void WSlider::OnMouseMoveEvent( SMouseMoveEvent const & _Event, double _TimeStamp ) {

    if ( Action == A_MOVE ) {
        Float2 CursorPos = GetDesktop()->GetCursorPosition();

        MoveSlider( ( bVerticalOrientation ? CursorPos.Y : CursorPos.X ) - DragCursor );
    }
}

void WSlider::OnDrawEvent( ACanvas & _Canvas ) {
    SSliderGeometry const & geometry = GetSliderGeometry();

    DrawDecorates( _Canvas );

    Float2 mins, maxs;
    GetDesktopRect( mins, maxs, false );

    _Canvas.DrawRectFilled( mins, maxs, BackgroundColor );

    // Draw slider background
    if ( geometry.BgMaxs.X > geometry.BgMins.X && geometry.BgMaxs.Y > geometry.BgMins.Y ) {
        //_Canvas.DrawRectFilled( geometry.BgMins, geometry.BgMaxs, Color4(0.4f,0.4f,0.4f) );

        Float2 h = bVerticalOrientation ? Float2( ( geometry.BgMaxs.X - geometry.BgMins.X ) * 0.5f, 0.0f )
                                        : Float2( 0.0f, ( geometry.BgMaxs.Y - geometry.BgMins.Y ) * 0.5f );

        _Canvas.DrawLine( geometry.BgMins + h, geometry.BgMaxs - h, LineColor, 2.0f );
    }

    // Draw slider
    if ( geometry.SliderMaxs.X > geometry.SliderMins.X && geometry.SliderMaxs.Y > geometry.SliderMins.Y ) {
        const float SliderRounding = 4.0f;
        _Canvas.DrawRectFilled( geometry.SliderMins, geometry.SliderMaxs, SliderColor, SliderRounding );
    }
}
