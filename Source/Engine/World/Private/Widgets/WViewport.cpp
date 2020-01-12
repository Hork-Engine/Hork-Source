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

#include <World/Public/Widgets/WViewport.h>

AN_CLASS_META( WViewport )

WViewport::WViewport() {
}

WViewport::~WViewport() {
}

void WViewport::UpdatePlayerControllerViewport() {
    Float2 mins, maxs;
    GetDesktopRect( mins, maxs, false );

    if ( PlayerController ) {
        PlayerController->SetViewport( mins, maxs - mins );
        PlayerController->SetActive( IsFocus() );
    }
}

void WViewport::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdatePlayerControllerViewport();
}

WViewport & WViewport::SetPlayerController( APlayerController * _PlayerController ) {
    PlayerController = _PlayerController;

    UpdatePlayerControllerViewport();

    return *this;
}

void WViewport::OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) {
    
}

void WViewport::OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) {
}

void WViewport::OnFocusLost() {
    if ( PlayerController ) {
        PlayerController->SetActive( false );
    }
}

void WViewport::OnFocusReceive() {
    if ( PlayerController ) {
        PlayerController->SetActive( true );
    }
}

void WViewport::OnDrawEvent( ACanvas & _Canvas ) {
    if ( PlayerController ) {
        Float2 mins, maxs;
        GetDesktopRect( mins, maxs, false );

        Float2 const & pos = mins;
        Float2 const & size = maxs-mins;
        _Canvas.DrawViewport( PlayerController, pos.X, pos.Y, size.X, size.Y, AColor4::White(), 0, -1, COLOR_BLENDING_DISABLED );
    }
}
