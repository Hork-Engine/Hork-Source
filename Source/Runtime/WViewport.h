/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

class APlayerController;

class WViewport : public WWidget
{
    AN_CLASS(WViewport, WWidget)

public:
    WViewport& SetPlayerController(APlayerController* _PlayerController);

    WViewport();
    ~WViewport();

protected:
    void OnTransformDirty() override;

    void OnKeyEvent(struct SKeyEvent const& _Event, double _TimeStamp) override;

    void OnMouseButtonEvent(struct SMouseButtonEvent const& _Event, double _TimeStamp) override;

    void OnMouseWheelEvent(struct SMouseWheelEvent const& _Event, double _TimeStamp) override;

    void OnMouseMoveEvent(struct SMouseMoveEvent const& _Event, double _TimeStamp) override;

    void OnJoystickButtonEvent(struct SJoystickButtonEvent const& _Event, double _TimeStamp) override;

    void OnJoystickAxisEvent(struct SJoystickAxisEvent const& _Event, double _TimeStamp) override;

    void OnCharEvent(struct SCharEvent const& _Event, double _TimeStamp) override;

    void OnFocusLost() override;

    void OnFocusReceive() override;

    void OnDrawEvent(ACanvas& _Canvas) override;

private:
    TRef<APlayerController> PlayerController;
};
