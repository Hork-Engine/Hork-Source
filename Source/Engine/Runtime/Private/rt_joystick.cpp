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

#include "rt_joystick.h"
#include "rt_event.h"

#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Runtime/Public/InputDefs.h>

#include <Engine/Core/Public/Math.h>

#include <GLFW/glfw3.h>

static FJoystick Joysticks[ MAX_JOYSTICKS_COUNT ];
static FString JoystickNames[ MAX_JOYSTICKS_COUNT ];
static FThreadSync JoystickNameUpdateSync;
static unsigned char JoystickButtonState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];
static float JoystickAxisState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];

static void RegisterJoystick( int _Joystick );
static void UnregisterJoystick( int _Joystick );

extern int rt_InputEventCount;

void rt_InitializeJoysticks() {
    memset( Joysticks, 0, sizeof( Joysticks ) );
    memset( JoystickButtonState, 0, sizeof( JoystickButtonState ) );
    memset( JoystickAxisState, 0, sizeof( JoystickAxisState ) );

    for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        Joysticks[i].Id = i;
    }

    glfwSetJoystickCallback( []( int _Joystick, int _Event ) {
        if ( _Event == GLFW_CONNECTED ) {
            RegisterJoystick( _Joystick );
        } else if ( _Event == GLFW_DISCONNECTED ) {
            UnregisterJoystick( _Joystick );
        }
    }
    );
}

void rt_DeinitializeJoysticks() {

    glfwSetJoystickCallback( nullptr );
}

static void RegisterJoystick( int _Joystick ) {
    FJoystick & joystick = Joysticks[ _Joystick ];

    JoystickNameUpdateSync.BeginScope();
    JoystickNames[ _Joystick ] = glfwGetJoystickName( _Joystick );
    JoystickNameUpdateSync.EndScope();

    int numAxes, numButtons;
    glfwGetJoystickAxes( _Joystick, &numAxes );
    glfwGetJoystickButtons( _Joystick, &numButtons );
    joystick.NumAxes = FMath::Min( numAxes, MAX_JOYSTICK_AXES );
    joystick.NumButtons = FMath::Min( numButtons, MAX_JOYSTICK_BUTTONS );
    joystick.bGamePad = glfwJoystickIsGamepad( _Joystick );
    joystick.bConnected = true;

    memset( JoystickButtonState[_Joystick], 0, sizeof( JoystickButtonState[0][0] ) * joystick.NumButtons );
    memset( JoystickAxisState[_Joystick], 0, sizeof( JoystickAxisState[0][0] ) * joystick.NumAxes );

    FEvent * event = rt_SendEvent();
    event->TimeStamp = GRuntime.SysSeconds_d();
    event->Type = ET_JoystickStateEvent;
    event->Data.JoystickStateEvent.Joystick = _Joystick;
    event->Data.JoystickStateEvent.NumAxes = joystick.NumAxes;
    event->Data.JoystickStateEvent.NumButtons = joystick.NumButtons;
    event->Data.JoystickStateEvent.bGamePad = joystick.bGamePad;
    event->Data.JoystickStateEvent.bConnected = joystick.bConnected;
}

static void UnregisterJoystick( int _Joystick ) {
    FJoystick & joystick = Joysticks[ _Joystick ];

    const double timeStamp = GRuntime.SysSeconds_d();

    for ( int i = 0 ; i < joystick.NumAxes ; i++ ) {
        if ( JoystickAxisState[_Joystick][i] != 0.0f ) {
            FEvent * event = rt_SendEvent();
            event->Type = ET_JoystickAxisEvent;
            event->TimeStamp = timeStamp;
            event->Data.JoystickAxisEvent.Joystick = _Joystick;
            event->Data.JoystickAxisEvent.Axis = i;
            event->Data.JoystickAxisEvent.Value = 0.0f;
            ++rt_InputEventCount;
        }
    }

    for ( int i = 0 ; i < joystick.NumButtons ; i++ ) {
        if ( JoystickButtonState[_Joystick][i] ) {
            FEvent * event = rt_SendEvent();
            event->Type = ET_JoystickButtonEvent;
            event->TimeStamp = timeStamp;
            event->Data.JoystickButtonEvent.Joystick = _Joystick;
            event->Data.JoystickButtonEvent.Button = i;
            event->Data.JoystickButtonEvent.Action = IE_Release;
            ++rt_InputEventCount;
        }
    }

    joystick.bConnected = false;

    FEvent * event = rt_SendEvent();
    event->Type = ET_JoystickStateEvent;
    event->TimeStamp = timeStamp;
    event->Data.JoystickStateEvent.Joystick = _Joystick;
    event->Data.JoystickStateEvent.NumAxes = joystick.NumAxes;
    event->Data.JoystickStateEvent.NumButtons = joystick.NumButtons;
    event->Data.JoystickStateEvent.bGamePad = joystick.bGamePad;
    event->Data.JoystickStateEvent.bConnected = joystick.bConnected;
}

void rt_PollJoystickEvents() {
    int count;
    int joyNum = 0;

    const double timeStamp = GRuntime.SysSeconds_d();

    for ( FJoystick & joystick : Joysticks ) {
        if ( !joystick.bConnected ) {
            joyNum++;
            continue;
        }

        const float * axes = glfwGetJoystickAxes( joyNum, &count );
        if ( axes ) {
            count = FMath::Min< int >( joystick.NumAxes, count );
            for ( int i = 0 ; i < count ; i++ ) {
                if ( axes[i] != JoystickAxisState[joyNum][i] ) {
                    JoystickAxisState[joyNum][i] = axes[i];

                    FEvent * event = rt_SendEvent();
                    event->Type = ET_JoystickAxisEvent;
                    event->TimeStamp = timeStamp;
                    event->Data.JoystickAxisEvent.Joystick = joyNum;
                    event->Data.JoystickAxisEvent.Axis = i;
                    event->Data.JoystickAxisEvent.Value = axes[ i ];
                    ++rt_InputEventCount;
                }
            }
        }

        const unsigned char * buttons = glfwGetJoystickButtons( joyNum, &count );
        if ( buttons ) {
            count = FMath::Min< int >( joystick.NumButtons, count );
            for ( int i = 0 ; i < count ; i++ ) {
                if ( buttons[i] != JoystickButtonState[joyNum][i] ) {
                    JoystickButtonState[joyNum][i] = buttons[i];

                    FEvent * event = rt_SendEvent();
                    event->Type = ET_JoystickButtonEvent;
                    event->TimeStamp = timeStamp;
                    event->Data.JoystickButtonEvent.Joystick = joyNum;
                    event->Data.JoystickButtonEvent.Button = i;
                    event->Data.JoystickButtonEvent.Action = buttons[ i ];
                    ++rt_InputEventCount;
                }
            }
        }

        joyNum++;
    }
}

FString rt_GetJoystickName( int _Joystick ) {
    JoystickNameUpdateSync.BeginScope();
    FString joystickName = JoystickNames[ _Joystick ];
    JoystickNameUpdateSync.EndScope();
    return joystickName;
}
