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

#include <Engine/Widgets/Public/WButton.h>

AN_CLASS_META( WButton )

WButton::WButton() {
    State = ST_RELEASED;
    Color = FColor4::White();
    HoverColor = FColor4( 1,1,0.5f,1 );
    PressedColor = FColor4( 1,1,0.2f,1 );
    TextColor = FColor4::Black();
    BorderColor = FColor4::Black();
    Rounding = 8;
    RoundingCorners = CORNER_ROUND_ALL;
    BorderThickness = 1;
}

WButton::~WButton() {
}

WButton & WButton::SetText( const char * _Text ) {
    Text = _Text;
    return *this;
}

WButton & WButton::SetColor( FColor4 const & _Color ) {
    Color = _Color;
    return *this;
}

WButton & WButton::SetHoverColor( FColor4 const & _Color ) {
    HoverColor = _Color;
    return *this;
}

WButton & WButton::SetPressedColor( FColor4 const & _Color ) {
    PressedColor = _Color;
    return *this;
}

WButton & WButton::SetTextColor( FColor4 const & _Color ) {
    TextColor = _Color;
    return *this;
}

WButton & WButton::SetBorderColor( FColor4 const & _Color ) {
    BorderColor = _Color;
    return *this;
}

WButton & WButton::SetRounding( float _Rounding ) {
    Rounding = _Rounding;
    return *this;
}

WButton & WButton::SetRoundingCorners( EDrawCornerFlags _RoundingCorners ) {
    RoundingCorners = _RoundingCorners;
    return *this;
}

WButton & WButton::SetBorderThickness( float _BorderThickness ) {
    BorderThickness = _BorderThickness;
    return *this;
}

void WButton::OnMouseButtonEvent( struct FMouseButtonEvent const & _Event, double _TimeStamp ) {
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

void WButton::OnDrawEvent( FCanvas & _Canvas ) {
    FColor4 bgColor;

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

    FFont * font = _Canvas.GetDefaultFont();

    float width = GetAvailableWidth();
    float height = GetAvailableHeight();

    Float2 size = font->CalcTextSizeA( font->FontSize, width, 0, Text.Begin(), Text.End() );

    _Canvas.DrawRectFilled( mins, maxs, bgColor, Rounding, RoundingCorners );
    if ( BorderThickness > 0.0f ) {
        _Canvas.DrawRect( mins, maxs, BorderColor, Rounding, RoundingCorners, BorderThickness );
    }
    _Canvas.DrawTextUTF8( mins + Float2( width - size.X, height - size.Y ) * 0.5f, TextColor, Text.Begin(), Text.End() );
}
