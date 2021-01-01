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

#pragma once

#include "GameModuleCallback.h"

struct SEntryDecl;

/** Engine interface */
class IEngineInterface
{
public:
    virtual ~IEngineInterface() {}

    /** Run the engine */
    virtual void Run( SEntryDecl const & _EntryDecl ) = 0;

    /** Print callback */
    virtual void Print( const char * _Message ) = 0;

    virtual void OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnMouseWheelEvent( struct SMouseWheelEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnJoystickAxisEvent( struct SJoystickAxisEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnJoystickButtonEvent( struct SJoystickButtonEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnCharEvent( struct SCharEvent const & _Event, double _TimeStamp ) = 0;

    virtual void OnWindowVisible( bool _Visible ) = 0;

    virtual void OnCloseEvent() = 0;

    virtual void OnResize() = 0;
};

extern IEngineInterface * CreateEngineInstance();
extern void DestroyEngineInstance();
