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

#include "JoystickManager.h"
#include "RuntimeEvents.h"

#include <Runtime/Public/InputDefs.h>
#include <Core/Public/CoreMath.h>

#include <GLFW/glfw3.h>

AJoystickManager & GJoystickManager = AJoystickManager::Inst();

static SJoystick Joysticks[ MAX_JOYSTICKS_COUNT ];
static AString JoystickNames[ MAX_JOYSTICKS_COUNT ];
static unsigned char JoystickButtonState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];
static float JoystickAxisState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];

static void RegisterJoystick( int _Joystick );
static void UnregisterJoystick( int _Joystick );

AJoystickManager::AJoystickManager() {

}

void AJoystickManager::Initialize() {
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

void AJoystickManager::Deinitialize() {
    glfwSetJoystickCallback( nullptr );

    for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        JoystickNames[ i ].Free();
    }
}

static void RegisterJoystick( int _Joystick ) {
    SJoystick & joystick = Joysticks[ _Joystick ];

    JoystickNames[ _Joystick ] = glfwGetJoystickName( _Joystick );
    int numAxes, numButtons;
    glfwGetJoystickAxes( _Joystick, &numAxes );
    glfwGetJoystickButtons( _Joystick, &numButtons );
    joystick.NumAxes = Math::Min( numAxes, MAX_JOYSTICK_AXES );
    joystick.NumButtons = Math::Min( numButtons, MAX_JOYSTICK_BUTTONS );
    joystick.bGamePad = glfwJoystickIsGamepad( _Joystick );
    joystick.bConnected = true;

    memset( JoystickButtonState[_Joystick], 0, sizeof( JoystickButtonState[0][0] ) * joystick.NumButtons );
    memset( JoystickAxisState[_Joystick], 0, sizeof( JoystickAxisState[0][0] ) * joystick.NumAxes );

    SEvent * event = GRuntimeEvents.Push();
    event->TimeStamp = GRuntime.SysSeconds_d();
    event->Type = ET_JoystickStateEvent;
    event->Data.JoystickStateEvent.Joystick = _Joystick;
    event->Data.JoystickStateEvent.NumAxes = joystick.NumAxes;
    event->Data.JoystickStateEvent.NumButtons = joystick.NumButtons;
    event->Data.JoystickStateEvent.bGamePad = joystick.bGamePad;
    event->Data.JoystickStateEvent.bConnected = joystick.bConnected;
}

static void UnregisterJoystick( int _Joystick ) {
    SJoystick & joystick = Joysticks[ _Joystick ];

    const double timeStamp = GRuntime.SysSeconds_d();

    for ( int i = 0 ; i < joystick.NumAxes ; i++ ) {
        if ( JoystickAxisState[_Joystick][i] != 0.0f ) {
            SEvent * event = GRuntimeEvents.Push();
            event->Type = ET_JoystickAxisEvent;
            event->TimeStamp = timeStamp;
            event->Data.JoystickAxisEvent.Joystick = _Joystick;
            event->Data.JoystickAxisEvent.Axis = i;
            event->Data.JoystickAxisEvent.Value = 0.0f;
            GInputEventsCount++;
        }
    }

    for ( int i = 0 ; i < joystick.NumButtons ; i++ ) {
        if ( JoystickButtonState[_Joystick][i] ) {
            SEvent * event = GRuntimeEvents.Push();
            event->Type = ET_JoystickButtonEvent;
            event->TimeStamp = timeStamp;
            event->Data.JoystickButtonEvent.Joystick = _Joystick;
            event->Data.JoystickButtonEvent.Button = i;
            event->Data.JoystickButtonEvent.Action = IE_Release;
            GInputEventsCount++;
        }
    }

    joystick.bConnected = false;

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_JoystickStateEvent;
    event->TimeStamp = timeStamp;
    event->Data.JoystickStateEvent.Joystick = _Joystick;
    event->Data.JoystickStateEvent.NumAxes = joystick.NumAxes;
    event->Data.JoystickStateEvent.NumButtons = joystick.NumButtons;
    event->Data.JoystickStateEvent.bGamePad = joystick.bGamePad;
    event->Data.JoystickStateEvent.bConnected = joystick.bConnected;
}

void AJoystickManager::PollEvents() {
    int count;
    int joyNum = 0;

    const double timeStamp = GRuntime.SysSeconds_d();

    for ( SJoystick & joystick : Joysticks ) {
        if ( !joystick.bConnected ) {
            joyNum++;
            continue;
        }

        const float * axes = glfwGetJoystickAxes( joyNum, &count );
        if ( axes ) {
            count = Math::Min< int >( joystick.NumAxes, count );
            for ( int i = 0 ; i < count ; i++ ) {
                if ( axes[i] != JoystickAxisState[joyNum][i] ) {
                    JoystickAxisState[joyNum][i] = axes[i];

                    SEvent * event = GRuntimeEvents.Push();
                    event->Type = ET_JoystickAxisEvent;
                    event->TimeStamp = timeStamp;
                    event->Data.JoystickAxisEvent.Joystick = joyNum;
                    event->Data.JoystickAxisEvent.Axis = i;
                    event->Data.JoystickAxisEvent.Value = axes[ i ];
                    GInputEventsCount++;
                }
            }
        }

        const unsigned char * buttons = glfwGetJoystickButtons( joyNum, &count );
        if ( buttons ) {
            count = Math::Min< int >( joystick.NumButtons, count );
            for ( int i = 0 ; i < count ; i++ ) {
                if ( buttons[i] != JoystickButtonState[joyNum][i] ) {
                    JoystickButtonState[joyNum][i] = buttons[i];

                    SEvent * event = GRuntimeEvents.Push();
                    event->Type = ET_JoystickButtonEvent;
                    event->TimeStamp = timeStamp;
                    event->Data.JoystickButtonEvent.Joystick = joyNum;
                    event->Data.JoystickButtonEvent.Button = i;
                    event->Data.JoystickButtonEvent.Action = buttons[ i ];
                    GInputEventsCount++;
                }
            }
        }

        joyNum++;
    }
}

AString AJoystickManager::GetJoystickName( int _Joystick ) {
    return JoystickNames[ _Joystick ];
}
