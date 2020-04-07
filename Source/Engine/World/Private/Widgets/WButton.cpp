/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Widgets/WButton.h>
#include <Runtime/Public/Runtime.h>

AN_CLASS_META( WButton )

WButton::WButton() {
    State = ST_RELEASED;
}

WButton::~WButton() {
}

void WButton::OnMouseButtonEvent( SMouseButtonEvent const & _Event, double _TimeStamp ) {
    if ( _Event.Action == IA_PRESS ) {
        if ( _Event.Button == 0 ) {
            State = ST_PRESSED;
        }
    } else if ( _Event.Action == IA_RELEASE ) {
        if ( _Event.Button == 0 && State == ST_PRESSED && IsHoveredByCursor() ) {

            State = ST_RELEASED;

            E_OnButtonClick.Dispatch();
        } else {
            State = ST_RELEASED;
        }
    }
}

void WButton::OnDrawEvent( ACanvas & _Canvas ) {
    AColor4 bgColor;

    if ( IsHoveredByCursor() && !IsDisabled() ) {
        if ( IsPressed() ) {
            bgColor = AColor4( 0.6f,0.6f,0.6f,1 );
        } else {
            bgColor = AColor4( 0.5f,0.5f,0.5f,1 );
        }
    } else {
        bgColor = AColor4(0.4f,0.4f,0.4f);
    }

    Float2 mins, maxs;

    GetDesktopRect( mins, maxs, true );

    _Canvas.DrawRectFilled( mins, maxs, bgColor );
}


AN_CLASS_META( WTextButton )

WTextButton::WTextButton() {
    Color = AColor4(0.4f,0.4f,0.4f);
    HoverColor = AColor4( 0.5f,0.5f,0.5f,1 );
    PressedColor = AColor4( 0.6f,0.6f,0.6f,1 );
    TextColor = AColor4::White();
    BorderColor = Float4(0,0,0,0.5f);
    Rounding = 8;
    RoundingCorners = CORNER_ROUND_ALL;
    BorderThickness = 1;
}

WTextButton::~WTextButton() {
}

WTextButton & WTextButton::SetText( const char * _Text ) {
    Text = _Text;
    return *this;
}

WTextButton & WTextButton::SetColor( AColor4 const & _Color ) {
    Color = _Color;
    return *this;
}

WTextButton & WTextButton::SetHoverColor( AColor4 const & _Color ) {
    HoverColor = _Color;
    return *this;
}

WTextButton & WTextButton::SetPressedColor( AColor4 const & _Color ) {
    PressedColor = _Color;
    return *this;
}

WTextButton & WTextButton::SetTextColor( AColor4 const & _Color ) {
    TextColor = _Color;
    return *this;
}

WTextButton & WTextButton::SetBorderColor( AColor4 const & _Color ) {
    BorderColor = _Color;
    return *this;
}

WTextButton & WTextButton::SetRounding( float _Rounding ) {
    Rounding = _Rounding;
    return *this;
}

WTextButton & WTextButton::SetRoundingCorners( EDrawCornerFlags _RoundingCorners ) {
    RoundingCorners = _RoundingCorners;
    return *this;
}

WTextButton & WTextButton::SetBorderThickness( float _BorderThickness ) {
    BorderThickness = _BorderThickness;
    return *this;
}

void WTextButton::OnDrawEvent( ACanvas & _Canvas ) {
    AColor4 bgColor;

    if ( IsHoveredByCursor() && !IsDisabled() ) {
        if ( IsPressed() ) {
            bgColor = PressedColor;
        } else {
            bgColor = HoverColor;
        }
    } else {
        bgColor = Color;
    }

    Float2 mins, maxs;

    GetDesktopRect( mins, maxs, true );

    AFont * font = ACanvas::GetDefaultFont();

    float width = GetAvailableWidth();
    float height = GetAvailableHeight();

    Float2 size = font->CalcTextSizeA( font->GetFontSize(), width, 0, Text.Begin(), Text.End() );

    _Canvas.DrawRectFilled( mins, maxs, bgColor, Rounding, RoundingCorners );
    if ( BorderThickness > 0.0f ) {
        _Canvas.DrawRect( mins, maxs, BorderColor, Rounding, RoundingCorners, BorderThickness );
    }
    _Canvas.DrawTextUTF8( mins + Float2( width - size.X, height - size.Y ) * 0.5f, TextColor, Text.Begin(), Text.End() );
}


AN_CLASS_META( WImageButton )

WImageButton::WImageButton() {
}

WImageButton::~WImageButton() {
}

WImageButton & WImageButton::SetImage( ATexture * _Image ) {
    Image = _Image;
    return *this;
}

WImageButton & WImageButton::SetHoverImage( ATexture * _Image ) {
    HoverImage = _Image;
    return *this;
}

WImageButton & WImageButton::SetPressedImage( ATexture * _Image ) {
    PressedImage = _Image;
    return *this;
}

void WImageButton::OnDrawEvent( ACanvas & _Canvas ) {
    ATexture * bgImage;

    if ( IsHoveredByCursor() && !IsDisabled() ) {
        if ( IsPressed() ) {
            bgImage = PressedImage;
        } else {
            bgImage = HoverImage;
        }
    } else {
        bgImage = Image;
    }

    if ( bgImage ) {
        Float2 mins, maxs;

        GetDesktopRect( mins, maxs, true );

        _Canvas.DrawTexture( bgImage, mins.X, mins.Y, maxs.X-mins.X, maxs.Y-mins.Y );
    }
}
