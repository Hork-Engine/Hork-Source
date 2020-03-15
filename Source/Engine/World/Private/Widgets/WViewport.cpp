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
#include <World/Public/Components/InputComponent.h>
#include <Runtime/Public/Runtime.h>

AN_CLASS_META( WViewport )

WViewport::WViewport() {
}

WViewport::~WViewport() {
}

void WViewport::OnTransformDirty() {
    Super::OnTransformDirty();
}

WViewport & WViewport::SetPlayerController( APlayerController * _PlayerController ) {
    PlayerController = _PlayerController;
    PlayerController->Viewport = this;

    return *this;
}

void WViewport::OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp ) {
    if ( PlayerController ) {
        AInputComponent * inputComponent = PlayerController->GetInputComponent();

        if ( !inputComponent->bIgnoreKeyboardEvents ) {
            inputComponent->SetButtonState( ID_KEYBOARD, _Event.Key, _Event.Action, _Event.ModMask, _TimeStamp );
        }
    }
}

void WViewport::OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) {
    if ( PlayerController ) {
        AInputComponent * inputComponent = PlayerController->GetInputComponent();

        if ( !inputComponent->bIgnoreMouseEvents ) {
            inputComponent->SetButtonState( ID_MOUSE, _Event.Button, _Event.Action, _Event.ModMask, _TimeStamp );
        }
    }
}

void WViewport::OnMouseWheelEvent( struct SMouseWheelEvent const & _Event, double _TimeStamp ) {
    if ( PlayerController ) {
        AInputComponent * inputComponent = PlayerController->GetInputComponent();

        if ( !inputComponent->bIgnoreMouseEvents ) {
            if ( _Event.WheelX < 0.0 ) {
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_LEFT, IE_Press, 0, _TimeStamp );
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_LEFT, IE_Release, 0, _TimeStamp );
            } else if ( _Event.WheelX > 0.0 ) {
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_RIGHT, IE_Press, 0, _TimeStamp );
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_RIGHT, IE_Release, 0, _TimeStamp );
            }
            if ( _Event.WheelY < 0.0 ) {
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_DOWN, IE_Press, 0, _TimeStamp );
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_DOWN, IE_Release, 0, _TimeStamp );
            } else if ( _Event.WheelY > 0.0 ) {
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_UP, IE_Press, 0, _TimeStamp );
                inputComponent->SetButtonState( ID_MOUSE, MOUSE_WHEEL_UP, IE_Release, 0, _TimeStamp );
            }
        }
    }
}

void WViewport::OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) {
    if ( PlayerController ) {
        AInputComponent * inputComponent = PlayerController->GetInputComponent();

        if ( !inputComponent->bIgnoreMouseEvents ) {
            inputComponent->SetMouseAxisState( _Event.X, _Event.Y );
        }
    }
}

void WViewport::OnCharEvent( struct SCharEvent const & _Event, double _TimeStamp ) {
    if ( PlayerController ) {
        AInputComponent * inputComponent = PlayerController->GetInputComponent();

        if ( !inputComponent->bIgnoreCharEvents ) {
            inputComponent->NotifyUnicodeCharacter( _Event.UnicodeCharacter, _Event.ModMask, _TimeStamp );
        }
    }
}

void WViewport::OnFocusLost() {
}

void WViewport::OnFocusReceive() {
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
