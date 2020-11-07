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

#include <World/Public/Actors/HUD.h>
#include <World/Public/Canvas.h>
#include <Core/Public/Utf8.h>

AN_CLASS_META( AHUD )

AHUD::AHUD() {

}

void AHUD::Draw( ACanvas * _Canvas, int _X, int _Y, int _W, int _H ) {
    Canvas = _Canvas;
    ViewportX = _X;
    ViewportY = _Y;
    ViewportW = _W;
    ViewportH = _H;

    DrawHUD();
}

void AHUD::DrawHUD() {

}

void AHUD::DrawText( AFont * _Font, int x, int y, AColor4 const & color, const char * _Text ) {
    const int CharacterWidth = 8;
    const int CharacterHeight = 16;

    const float scale = (float)CharacterHeight / _Font->GetFontSize();

    const char * s = _Text;
    int byteLen;
    SWideChar ch;
    int cx = x;

    Canvas->PushFont( _Font );

    while ( *s ) {
        byteLen = Core::WideCharDecodeUTF8( s, ch );
        if ( !byteLen ) {
            break;
        }

        s += byteLen;

        if ( ch == '\n' || ch == '\r' ) {
            y += CharacterHeight + 4;
            cx = x;
            continue;
        }

        if ( ch == ' ' ) {
            cx += CharacterWidth;
            continue;
        }

        Canvas->DrawWChar( ch, cx, y, scale, color );

        cx += CharacterWidth;
    }

    Canvas->PopFont();
}
