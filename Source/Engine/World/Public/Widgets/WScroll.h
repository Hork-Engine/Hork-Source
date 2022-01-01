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

#include "WWidget.h"

struct SScrollbarGeometry {
    bool bDrawHScrollbar;
    bool bDrawVScrollbar;

    Float2 HScrollbarMins;
    Float2 HScrollbarMaxs;

    Float2 LeftButtonMins;
    Float2 LeftButtonMaxs;

    Float2 RightButtonMins;
    Float2 RightButtonMaxs;

    Float2 HSliderBgMins;
    Float2 HSliderBgMaxs;

    Float2 HSliderMins;
    Float2 HSliderMaxs;

    Float2 VScrollbarMins;
    Float2 VScrollbarMaxs;

    Float2 UpButtonMins;
    Float2 UpButtonMaxs;

    Float2 DownButtonMins;
    Float2 DownButtonMaxs;

    Float2 VSliderBgMins;
    Float2 VSliderBgMaxs;

    Float2 VSliderMins;
    Float2 VSliderMaxs;

    Float2 ContentSize;
    Float2 ContentPosition;

    Float2 ViewSize;
};

class WScroll : public WWidget {
    AN_CLASS( WScroll, WWidget )

public:
    WScroll & SetContentWidget( WWidget * _Content );
    WScroll & SetContentWidget( WWidget & _Content );
    WScroll & SetAutoScrollH( bool _AutoScroll );
    WScroll & SetAutoScrollV( bool _AutoScroll );
    WScroll & SetScrollbarSize( float _Size );
    WScroll & SetButtonWidth( float _Width );
    WScroll & SetShowButtons( bool _ShowButtons );
    WScroll & SetSliderRounding( float _Rounding );
    WScroll & SetBackgroundColor( Color4 const & _Color );
    WScroll & SetButtonColor( Color4 const & _Color );
    WScroll & SetSliderBackgroundColor( Color4 const & _Color );
    WScroll & SetSliderColor( Color4 const & _Color );

    WWidget * GetContentWidget();

    float GetScrollbarSize() const {
        return ScrollbarSize;
    }

    void ScrollHome();

    void ScrollEnd();

    void ScrollDelta( Float2 const & _Offset );

    void SetScrollPosition( Float2 const & _Position );

    Float2 GetScrollPosition() const;

    bool IsVerticalScrollAllowed() const;

protected:
    WScroll();
    ~WScroll();

    // You can override OnDrawEvent and use GetScrollbarGeometry to
    // draw your own style scrollbar.
    SScrollbarGeometry const & GetScrollbarGeometry() const;

    void Update( float _TimeStep );

    void OnTransformDirty() override;

    void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) override;

    void OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) override;

    void OnFocusReceive() override;

    // There is no Tick() function for widget yet. So if you override OnDrawEvent don't forget to call Update()
    void OnDrawEvent( ACanvas & _Canvas ) override;

private:
    void UpdateMargin();
    void UpdateScrollbarGeometry();
    void UpdateScrollbarGeometryIfDirty();
    void UpdateScrolling( float _TimeStep );
    void MoveHSlider( float _Vec );
    void MoveVSlider( float _Vec );

    enum EScrollAction { A_NONE, A_SCROLL_LEFT, A_SCROLL_RIGHT, A_SCROLL_UP, A_SCROLL_DOWN, A_SCROLL_HSLIDER, A_SCROLL_VSLIDER };

    Color4 BackgroundColor;
    Color4 ButtonColor;
    Color4 SliderBackgroundColor;
    Color4 SliderColor;

    TRef< WWidget > Content;
    int Action;
    float DragCursor;
    SScrollbarGeometry Geometry;
    float ScrollbarSize;
    float ButtonWidth;
    float SliderRounding;
    bool bAutoScrollH;
    bool bAutoScrollV;
    bool bShowButtons;
    bool bUpdateGeometry;
};
