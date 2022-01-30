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

#pragma once

#include "WWidget.h"

struct SSliderGeometry {
    Float2 SliderMins;
    Float2 SliderMaxs;

    Float2 BgMins;
    Float2 BgMaxs;
};

class WSlider : public WWidget {
    AN_CLASS( WSlider, WWidget )

public:
    TWidgetEvent< float > E_OnUpdateValue;

    template< typename T, typename... TArgs >
    WSlider & SetOnUpdateValue( T * _Object, void ( T::*_Method )(TArgs...) ) {
        E_OnUpdateValue.Add( _Object, _Method );
        return *this;
    }

    WSlider & SetValue( float _Value );
    WSlider & SetMaxValue( float _MaxValue );
    WSlider & SetMinValue( float _MinValue );
    WSlider & SetStep( float _Step );
    WSlider & SetSliderWidth( float _Width );
    WSlider & SetVerticalOrientation( bool _VerticalOrientation );
    WSlider & SetBackgroundColor( Color4 const & _Color );
    WSlider & SetSliderColor( Color4 const & _Color );
    WSlider & SetLineColor( Color4 const & _Color );

    float GetValue() const { return Value; }
    float GetMinValue() const { return MinValue; }
    float GetMaxValue() const { return MaxValue; }

    WSlider();
    ~WSlider();

protected:
    // You can override OnDrawEvent and use GetSliderGeometry to
    // draw your own style slider.
    SSliderGeometry const & GetSliderGeometry() const;

    bool IsVertical() const { return bVerticalOrientation; }

    void OnTransformDirty() override;

    void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) override;

    void OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) override;

    void OnDrawEvent( ACanvas & _Canvas ) override;

private:
    void UpdateSliderGeometry();
    void UpdateSliderGeometryIfDirty();
    void MoveSlider( float _Vec );

    enum EScrollAction { A_NONE, A_DECREASE, A_INCREASE, A_MOVE };

    Color4 BackgroundColor;
    Color4 SliderColor;
    Color4 LineColor;

    int Action;
    float DragCursor;
    SSliderGeometry Geometry;
    float MinValue;
    float MaxValue;
    float Step;
    float Value;
    float SliderWidth;
    bool bVerticalOrientation;
    bool bUpdateGeometry;
};
