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

#include <World/Public/Components/InputComponent.h>
#include <World/Public/World.h>
#include <Runtime/Public/Runtime.h>
#include <Core/Public/Logger.h>
#include <Core/Public/HashFunc.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META( AInputAxis )
AN_CLASS_META( AInputAction )
AN_CLASS_META( AInputMappings )

AN_BEGIN_CLASS_META( AInputComponent )
AN_ATTRIBUTE_( bIgnoreKeyboardEvents, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreMouseEvents, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreJoystickEvents, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreCharEvents, AF_DEFAULT )
AN_ATTRIBUTE_( ControllerId, AF_DEFAULT )
AN_END_CLASS_META()

struct AInputComponentStatic {
    const char * KeyNames[ MAX_KEYBOARD_BUTTONS ];
    const char * MouseButtonNames[ MAX_MOUSE_BUTTONS ];
    const char * MouseAxisNames[ MAX_MOUSE_AXES ];
    const char * DeviceNames[ MAX_INPUT_DEVICES ];
    const char * JoystickButtonNames[ MAX_JOYSTICK_BUTTONS ];
    const char * JoystickAxisNames[ MAX_JOYSTICK_AXES ];
    const char * ModifierNames[ MAX_MODIFIERS ];
    const char * ControllerNames[ MAX_INPUT_CONTROLLERS ];

    int DeviceButtonLimits[ MAX_INPUT_DEVICES ];
    //SJoystick Joysticks[ MAX_JOYSTICKS_COUNT ];
    float JoystickAxisState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];

    AInputComponentStatic() {
        #define InitKey( _Key ) KeyNames[ _Key ] = AN_STRINGIFY( _Key ) + 4
        #define InitButton( _Button ) MouseButtonNames[ _Button ] = AN_STRINGIFY( _Button ) + 6
        #define InitMouseAxis( _Axis ) MouseAxisNames[ _Axis - MOUSE_AXIS_BASE ] = AN_STRINGIFY( _Axis ) + 6
        #define InitDevice( _DevId ) DeviceNames[ _DevId ] = AN_STRINGIFY( _DevId ) + 3
        #define InitJoyButton( _Button ) JoystickButtonNames[ _Button - JOY_BUTTON_BASE ] = AN_STRINGIFY( _Button ) + 4
        #define InitJoyAxis( _Axis ) JoystickAxisNames[ _Axis - JOY_AXIS_BASE ] = AN_STRINGIFY( _Axis ) + 4
        #define InitModifier( _Modifier ) ModifierNames[ _Modifier ] = AN_STRINGIFY( _Modifier ) + 4
        #define InitController( _Controller ) ControllerNames[ _Controller ] = AN_STRINGIFY( _Controller ) + 11

        DeviceButtonLimits[ ID_KEYBOARD ] = MAX_KEYBOARD_BUTTONS;
        DeviceButtonLimits[ ID_MOUSE ] = MAX_MOUSE_BUTTONS + MAX_MOUSE_AXES;
        for ( int i = ID_JOYSTICK_1 ; i <= ID_JOYSTICK_16 ; i++ ) {
            DeviceButtonLimits[ i ] = MAX_JOYSTICK_BUTTONS + MAX_JOYSTICK_AXES;
        }

        for ( int i = 0 ; i < MAX_KEYBOARD_BUTTONS ; i++ ) {
            KeyNames[ i ] = "";
        }

        for ( int i = 0 ; i < MAX_MOUSE_BUTTONS ; i++ ) {
            MouseButtonNames[ i ] = "";
        }

        for ( int i = 0 ; i < MAX_MOUSE_AXES ; i++ ) {
            MouseAxisNames[ i ] = "";
        }

        for ( int i = 0 ; i < MAX_INPUT_DEVICES ; i++ ) {
            DeviceNames[ i ] = "";
        }

        for ( int i = 0 ; i < MAX_JOYSTICK_BUTTONS ; i++ ) {
            JoystickButtonNames[ i ] = "";
        }

        for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
            JoystickAxisNames[ i ] = "";
        }

        InitKey( KEY_SPACE );
        InitKey( KEY_APOSTROPHE );
        InitKey( KEY_COMMA );
        InitKey( KEY_MINUS );
        InitKey( KEY_PERIOD );
        InitKey( KEY_SLASH );
        InitKey( KEY_0 );
        InitKey( KEY_1 );
        InitKey( KEY_2 );
        InitKey( KEY_3 );
        InitKey( KEY_4 );
        InitKey( KEY_5 );
        InitKey( KEY_6 );
        InitKey( KEY_7 );
        InitKey( KEY_8 );
        InitKey( KEY_9 );
        InitKey( KEY_SEMICOLON );
        InitKey( KEY_EQUAL );
        InitKey( KEY_A );
        InitKey( KEY_B );
        InitKey( KEY_C );
        InitKey( KEY_D );
        InitKey( KEY_E );
        InitKey( KEY_F );
        InitKey( KEY_G );
        InitKey( KEY_H );
        InitKey( KEY_I );
        InitKey( KEY_J );
        InitKey( KEY_K );
        InitKey( KEY_L );
        InitKey( KEY_M );
        InitKey( KEY_N );
        InitKey( KEY_O );
        InitKey( KEY_P );
        InitKey( KEY_Q );
        InitKey( KEY_R );
        InitKey( KEY_S );
        InitKey( KEY_T );
        InitKey( KEY_U );
        InitKey( KEY_V );
        InitKey( KEY_W );
        InitKey( KEY_X );
        InitKey( KEY_Y );
        InitKey( KEY_Z );
        InitKey( KEY_LEFT_BRACKET );
        InitKey( KEY_BACKSLASH );
        InitKey( KEY_RIGHT_BRACKET );
        InitKey( KEY_GRAVE_ACCENT );
        InitKey( KEY_ESCAPE );
        InitKey( KEY_ENTER );
        InitKey( KEY_TAB );
        InitKey( KEY_BACKSPACE );
        InitKey( KEY_INSERT );
        InitKey( KEY_DELETE );
        InitKey( KEY_RIGHT );
        InitKey( KEY_LEFT );
        InitKey( KEY_DOWN );
        InitKey( KEY_UP );
        InitKey( KEY_PAGE_UP );
        InitKey( KEY_PAGE_DOWN );
        InitKey( KEY_HOME );
        InitKey( KEY_END );
        InitKey( KEY_CAPS_LOCK );
        InitKey( KEY_SCROLL_LOCK );
        InitKey( KEY_NUM_LOCK );
        InitKey( KEY_PRINT_SCREEN );
        InitKey( KEY_PAUSE );
        InitKey( KEY_F1 );
        InitKey( KEY_F2 );
        InitKey( KEY_F3 );
        InitKey( KEY_F4 );
        InitKey( KEY_F5 );
        InitKey( KEY_F6 );
        InitKey( KEY_F7 );
        InitKey( KEY_F8 );
        InitKey( KEY_F9 );
        InitKey( KEY_F10 );
        InitKey( KEY_F11 );
        InitKey( KEY_F12 );
        InitKey( KEY_F13 );
        InitKey( KEY_F14 );
        InitKey( KEY_F15 );
        InitKey( KEY_F16 );
        InitKey( KEY_F17 );
        InitKey( KEY_F18 );
        InitKey( KEY_F19 );
        InitKey( KEY_F20 );
        InitKey( KEY_F21 );
        InitKey( KEY_F22 );
        InitKey( KEY_F23 );
        InitKey( KEY_F24 );
        InitKey( KEY_KP_0 );
        InitKey( KEY_KP_1 );
        InitKey( KEY_KP_2 );
        InitKey( KEY_KP_3 );
        InitKey( KEY_KP_4 );
        InitKey( KEY_KP_5 );
        InitKey( KEY_KP_6 );
        InitKey( KEY_KP_7 );
        InitKey( KEY_KP_8 );
        InitKey( KEY_KP_9 );
        InitKey( KEY_KP_DECIMAL );
        InitKey( KEY_KP_DIVIDE );
        InitKey( KEY_KP_MULTIPLY );
        InitKey( KEY_KP_SUBTRACT );
        InitKey( KEY_KP_ADD );
        InitKey( KEY_KP_ENTER );
        InitKey( KEY_KP_EQUAL );
        InitKey( KEY_LEFT_SHIFT );
        InitKey( KEY_LEFT_CONTROL );
        InitKey( KEY_LEFT_ALT );
        InitKey( KEY_LEFT_SUPER );
        InitKey( KEY_RIGHT_SHIFT );
        InitKey( KEY_RIGHT_CONTROL );
        InitKey( KEY_RIGHT_ALT );
        InitKey( KEY_RIGHT_SUPER );
        InitKey( KEY_MENU );

        InitButton( MOUSE_BUTTON_LEFT );
        InitButton( MOUSE_BUTTON_RIGHT );
        InitButton( MOUSE_BUTTON_MIDDLE );
        InitButton( MOUSE_BUTTON_4 );
        InitButton( MOUSE_BUTTON_5 );
        InitButton( MOUSE_BUTTON_6 );
        InitButton( MOUSE_BUTTON_7 );
        InitButton( MOUSE_BUTTON_8 );

        InitButton( MOUSE_WHEEL_UP );
        InitButton( MOUSE_WHEEL_DOWN );
        InitButton( MOUSE_WHEEL_LEFT );
        InitButton( MOUSE_WHEEL_RIGHT );

        InitMouseAxis( MOUSE_AXIS_X );
        InitMouseAxis( MOUSE_AXIS_Y );

        InitDevice( ID_KEYBOARD );
        InitDevice( ID_MOUSE );
        InitDevice( ID_JOYSTICK_1 );
        InitDevice( ID_JOYSTICK_2 );
        InitDevice( ID_JOYSTICK_3 );
        InitDevice( ID_JOYSTICK_4 );
        InitDevice( ID_JOYSTICK_5 );
        InitDevice( ID_JOYSTICK_6 );
        InitDevice( ID_JOYSTICK_7 );
        InitDevice( ID_JOYSTICK_8 );
        InitDevice( ID_JOYSTICK_9 );
        InitDevice( ID_JOYSTICK_10 );
        InitDevice( ID_JOYSTICK_11 );
        InitDevice( ID_JOYSTICK_12 );
        InitDevice( ID_JOYSTICK_13 );
        InitDevice( ID_JOYSTICK_14 );
        InitDevice( ID_JOYSTICK_15 );
        InitDevice( ID_JOYSTICK_16 );

        InitJoyButton( JOY_BUTTON_1 );
        InitJoyButton( JOY_BUTTON_2 );
        InitJoyButton( JOY_BUTTON_3 );
        InitJoyButton( JOY_BUTTON_4 );
        InitJoyButton( JOY_BUTTON_5 );
        InitJoyButton( JOY_BUTTON_6 );
        InitJoyButton( JOY_BUTTON_7 );
        InitJoyButton( JOY_BUTTON_8 );
        InitJoyButton( JOY_BUTTON_9 );
        InitJoyButton( JOY_BUTTON_10 );
        InitJoyButton( JOY_BUTTON_11 );
        InitJoyButton( JOY_BUTTON_12 );
        InitJoyButton( JOY_BUTTON_13 );
        InitJoyButton( JOY_BUTTON_14 );
        InitJoyButton( JOY_BUTTON_15 );
        InitJoyButton( JOY_BUTTON_16 );
        InitJoyButton( JOY_BUTTON_17 );
        InitJoyButton( JOY_BUTTON_18 );
        InitJoyButton( JOY_BUTTON_19 );
        InitJoyButton( JOY_BUTTON_20 );
        InitJoyButton( JOY_BUTTON_21 );
        InitJoyButton( JOY_BUTTON_22 );
        InitJoyButton( JOY_BUTTON_23 );
        InitJoyButton( JOY_BUTTON_24 );
        InitJoyButton( JOY_BUTTON_25 );
        InitJoyButton( JOY_BUTTON_26 );
        InitJoyButton( JOY_BUTTON_27 );
        InitJoyButton( JOY_BUTTON_28 );
        InitJoyButton( JOY_BUTTON_29 );
        InitJoyButton( JOY_BUTTON_30 );
        InitJoyButton( JOY_BUTTON_31 );
        InitJoyButton( JOY_BUTTON_32 );

        InitJoyAxis( JOY_AXIS_1 );
        InitJoyAxis( JOY_AXIS_2 );
        InitJoyAxis( JOY_AXIS_3 );
        InitJoyAxis( JOY_AXIS_4 );
        InitJoyAxis( JOY_AXIS_5 );
        InitJoyAxis( JOY_AXIS_6 );
        InitJoyAxis( JOY_AXIS_7 );
        InitJoyAxis( JOY_AXIS_8 );
        InitJoyAxis( JOY_AXIS_9 );
        InitJoyAxis( JOY_AXIS_10 );
        InitJoyAxis( JOY_AXIS_11 );
        InitJoyAxis( JOY_AXIS_12 );
        InitJoyAxis( JOY_AXIS_13 );
        InitJoyAxis( JOY_AXIS_14 );
        InitJoyAxis( JOY_AXIS_15 );
        InitJoyAxis( JOY_AXIS_16 );
        InitJoyAxis( JOY_AXIS_17 );
        InitJoyAxis( JOY_AXIS_18 );
        InitJoyAxis( JOY_AXIS_19 );
        InitJoyAxis( JOY_AXIS_20 );
        InitJoyAxis( JOY_AXIS_21 );
        InitJoyAxis( JOY_AXIS_22 );
        InitJoyAxis( JOY_AXIS_23 );
        InitJoyAxis( JOY_AXIS_24 );
        InitJoyAxis( JOY_AXIS_25 );
        InitJoyAxis( JOY_AXIS_26 );
        InitJoyAxis( JOY_AXIS_27 );
        InitJoyAxis( JOY_AXIS_28 );
        InitJoyAxis( JOY_AXIS_29 );
        InitJoyAxis( JOY_AXIS_30 );
        InitJoyAxis( JOY_AXIS_31 );
        InitJoyAxis( JOY_AXIS_32 );

        InitModifier( KMOD_SHIFT );
        InitModifier( KMOD_CONTROL );
        InitModifier( KMOD_ALT );
        InitModifier( KMOD_SUPER );
        InitModifier( KMOD_CAPS_LOCK );
        InitModifier( KMOD_NUM_LOCK );

        InitController( CONTROLLER_PLAYER_1 );
        InitController( CONTROLLER_PLAYER_2 );
        InitController( CONTROLLER_PLAYER_3 );
        InitController( CONTROLLER_PLAYER_4 );
        InitController( CONTROLLER_PLAYER_5 );
        InitController( CONTROLLER_PLAYER_6 );
        InitController( CONTROLLER_PLAYER_7 );
        InitController( CONTROLLER_PLAYER_8 );
        InitController( CONTROLLER_PLAYER_9 );
        InitController( CONTROLLER_PLAYER_10 );
        InitController( CONTROLLER_PLAYER_11 );
        InitController( CONTROLLER_PLAYER_12 );
        InitController( CONTROLLER_PLAYER_13 );
        InitController( CONTROLLER_PLAYER_14 );
        InitController( CONTROLLER_PLAYER_15 );
        InitController( CONTROLLER_PLAYER_16 );

        //Core::ZeroMem( Joysticks, sizeof( Joysticks ) );
        Core::ZeroMem( JoystickAxisState, sizeof( JoystickAxisState ) );

        //for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        //    Joysticks[i].Id = i;
        //}
    }
};

static AInputComponentStatic Static;

AInputComponent * AInputComponent::InputComponents = nullptr;
AInputComponent * AInputComponent::InputComponentsTail = nullptr;

const char * AInputHelper::TranslateDevice( int _DevId ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return "UNKNOWN";
    }
    return Static.DeviceNames[ _DevId ];
}

const char * AInputHelper::TranslateModifier( int _Modifier ) {
    if ( _Modifier < 0 || _Modifier > KMOD_LAST ) {
        return "UNKNOWN";
    }
    return Static.ModifierNames[ _Modifier ];
}

const char * AInputHelper::TranslateDeviceKey( int _DevId, int _Key ) {
    switch ( _DevId ) {
    case ID_KEYBOARD:
        if ( _Key < 0 || _Key > KEY_LAST ) {
            return "UNKNOWN";
        }
        return Static.KeyNames[ _Key ];
    case ID_MOUSE:
        if ( _Key >= MOUSE_AXIS_BASE ) {
            if ( _Key > MOUSE_AXIS_LAST ) {
                return "UNKNOWN";
            }
            return Static.MouseAxisNames[ _Key - MOUSE_AXIS_BASE ];
        }
        if ( _Key < MOUSE_BUTTON_BASE || _Key > MOUSE_BUTTON_LAST ) {
            return "UNKNOWN";
        }
        return Static.MouseButtonNames[ _Key - MOUSE_BUTTON_BASE ];
    }
    if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 ) {
        if ( _Key >= JOY_AXIS_BASE ) {
            if ( _Key > JOY_AXIS_LAST ) {
                return "UNKNOWN";
            }
            return Static.JoystickAxisNames[ _Key - JOY_AXIS_BASE ];
        }

        if ( _Key < JOY_BUTTON_BASE || _Key > JOY_BUTTON_LAST ) {
            return "UNKNOWN";
        }
        return Static.JoystickButtonNames[ _Key - JOY_BUTTON_BASE ];
    }
    return "UNKNOWN";
}

const char * AInputHelper::TranslateController( int _ControllerId ) {
    if ( _ControllerId < 0 || _ControllerId >= MAX_INPUT_CONTROLLERS ) {
        return "UNKNOWN";
    }
    return Static.ControllerNames[ _ControllerId ];
}

int AInputHelper::LookupDevice( const char * _Device ) {
    for ( int i = 0 ; i < MAX_INPUT_DEVICES ; i++ ) {
        if ( !Core::Stricmp( Static.DeviceNames[i], _Device ) ) {
            return i;
        }
    }
    return -1;
}

int AInputHelper::LookupModifier( const char * _Modifier ) {
    for ( int i = 0 ; i < MAX_MODIFIERS ; i++ ) {
        if ( !Core::Stricmp( Static.ModifierNames[i], _Modifier ) ) {
            return i;
        }
    }
    return -1;
}

int AInputHelper::LookupDeviceKey( int _DevId, const char * _Key ) {
    switch ( _DevId ) {
    case ID_KEYBOARD:
        for ( int i = 0 ; i < MAX_KEYBOARD_BUTTONS ; i++ ) {
            if ( !Core::Stricmp( Static.KeyNames[i], _Key ) ) {
                return i;
            }
        }
        return -1;
    case ID_MOUSE:
        for ( int i = 0 ; i < MAX_MOUSE_BUTTONS ; i++ ) {
            if ( !Core::Stricmp( Static.MouseButtonNames[i], _Key ) ) {
                return MOUSE_BUTTON_BASE + i;
            }
        }
        for ( int i = 0 ; i < MAX_MOUSE_AXES ; i++ ) {
            if ( !Core::Stricmp( Static.MouseAxisNames[i], _Key ) ) {
                return MOUSE_AXIS_BASE + i;
            }
        }
        return -1;
    }
    if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 ) {
        for ( int i = 0 ; i < MAX_JOYSTICK_BUTTONS ; i++ ) {
            if ( !Core::Stricmp( Static.JoystickButtonNames[i], _Key ) ) {
                return JOY_BUTTON_BASE + i;
            }
        }
        for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
            if ( !Core::Stricmp( Static.JoystickAxisNames[i], _Key ) ) {
                return JOY_AXIS_BASE + i;
            }
        }
    }
    return -1;
}

int AInputHelper::LookupController( const char * _Controller ) {
    for ( int i = 0 ; i < MAX_INPUT_CONTROLLERS ; i++ ) {
        if ( !Core::Stricmp( Static.ControllerNames[i], _Controller ) ) {
            return i;
        }
    }
    return -1;
}

AInputComponent::AInputComponent() {
    DeviceButtonDown[ ID_KEYBOARD ] = KeyboardButtonDown;
    DeviceButtonDown[ ID_MOUSE ] = MouseButtonDown;
    for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        DeviceButtonDown[ ID_JOYSTICK_1 + i ] = JoystickButtonDown[ i ];
    }
    Core::Memset( KeyboardButtonDown, 0xff, sizeof( KeyboardButtonDown ) );
    Core::Memset( MouseButtonDown, 0xff, sizeof( MouseButtonDown ) );
    Core::Memset( JoystickButtonDown, 0xff, sizeof( JoystickButtonDown ) );

    INTRUSIVE_ADD( this, Next, Prev, InputComponents, InputComponentsTail );
}

void AInputComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    INTRUSIVE_REMOVE( this, Next, Prev, InputComponents, InputComponentsTail );

    InputMappings = nullptr;
}

void AInputComponent::SetInputMappings( AInputMappings * _InputMappings ) {
    InputMappings = _InputMappings;
}

AInputMappings * AInputComponent::GetInputMappings() {
    return InputMappings;
}

void AInputComponent::UpdateAxes( float _TimeStep ) {
    if ( !InputMappings ) {
        return;
    }

    bool bPaused = GetWorld()->IsPaused();

    for ( SAxisBinding & binding : AxisBindings ) {
        binding.AxisScale = 0.0f;
    }

    for ( SPressedKey * key = PressedKeys ; key < &PressedKeys[ NumPressedKeys ] ; key++ ) {
        if ( key->HasAxis() ) {
            AxisBindings[ key->AxisBinding ].AxisScale += key->AxisScale * _TimeStep;
        }
    }

    TPodArray< AInputAxis * > const & inputAxes = InputMappings->GetAxes();
    for ( int i = 0 ; i < inputAxes.Size() ; i++ ) {
        AInputAxis * inputAxis = inputAxes[i];

        int axisBinding = GetAxisBinding( inputAxis );
        if ( axisBinding == -1 ) {
            // axis is not binded
            continue;
        }

        SAxisBinding & binding = AxisBindings[ axisBinding ];
        if ( bPaused && !binding.bExecuteEvenWhenPaused ) {
            continue;
        }

        if ( !bIgnoreJoystickEvents ) {
            for ( int joyNum = 0 ; joyNum < MAX_JOYSTICKS_COUNT ; joyNum++ ) {
                //if ( !Static.Joysticks[joyNum].bConnected ) {
                //    continue;
                //}

                const uint32_t joystickAxes = inputAxis->GetJoystickAxes( joyNum );
                for ( int joystickAxis = 0 ; joystickAxis < MAX_JOYSTICK_AXES ; joystickAxis++ ) {

                    if ( joystickAxes & ( 1 << joystickAxis ) ) {
                        AInputMappings::SMapping & mapping = InputMappings->JoystickAxisMappings[ joyNum ][ joystickAxis ];

                        AN_ASSERT( mapping.AxisOrActionIndex == i );

                        if ( mapping.ControllerId == ControllerId ) {
                            binding.AxisScale += Static.JoystickAxisState[ joyNum ][ joystickAxis ] * mapping.AxisScale * _TimeStep;
                        }
                    }
                }
            }
        }

        if ( !bIgnoreMouseEvents ) {
            const uint8_t mouseAxes = inputAxis->GetMouseAxes();
            for ( int mouseAxis = 0 ; mouseAxis < MAX_MOUSE_AXES ; mouseAxis++ ) {
                if ( mouseAxes & ( 1 << mouseAxis ) ) {
                    AInputMappings::SMapping & mapping = InputMappings->MouseAxisMappings[ mouseAxis ];

                    AN_ASSERT( mapping.AxisOrActionIndex == i );

                    if ( mapping.ControllerId == ControllerId ) {
                        binding.AxisScale += (&MouseAxisStateX)[ mouseAxis ] * mapping.AxisScale;
                    }
                }
            }
        }

        binding.Callback( binding.AxisScale );
    }

    // Reset mouse axes
    MouseAxisStateX = 0;
    MouseAxisStateY = 0;
}

void AInputComponent::SetButtonState( int _DevId, int _Button, int _Action, int _ModMask, double _TimeStamp ) {
    AN_ASSERT( _DevId >= 0 && _DevId < MAX_INPUT_DEVICES );

    if ( _DevId == ID_KEYBOARD && bIgnoreKeyboardEvents ) {
        return;
    }
    if ( _DevId == ID_MOUSE && bIgnoreMouseEvents ) {
        return;
    }
    if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 && bIgnoreJoystickEvents ) {
        return;
    }

    char * ButtonIndex = DeviceButtonDown[ _DevId ];

#ifdef AN_DEBUG
    switch ( _DevId ) {
    case ID_KEYBOARD:
        AN_ASSERT( _Button < MAX_KEYBOARD_BUTTONS );
        break;
    case ID_MOUSE:
        AN_ASSERT( _Button < MAX_MOUSE_BUTTONS );
        break;
    default:
        AN_ASSERT( _Button < MAX_JOYSTICK_BUTTONS );
        break;
    }
#endif

    if ( _Action == IA_PRESS ) {
        if ( ButtonIndex[ _Button ] == -1 ) {
            if ( NumPressedKeys < MAX_PRESSED_KEYS ) {
                SPressedKey & pressedKey = PressedKeys[ NumPressedKeys ];
                pressedKey.DevId = _DevId;
                pressedKey.Key = _Button;
                pressedKey.AxisBinding = -1;
                pressedKey.ActionBinding = -1;
                pressedKey.AxisScale = 0;

                if ( InputMappings ) {
                    const AInputMappings::SMapping * mapping;
                    if ( _DevId == ID_KEYBOARD ) {
                        mapping = &InputMappings->KeyboardMappings[ _Button ];
                    } else if ( _DevId == ID_MOUSE ) {
                        mapping = &InputMappings->MouseMappings[ _Button ];
                    } else {
                        mapping = &InputMappings->JoystickMappings[ _DevId - ID_JOYSTICK_1 ][ _Button ];
                    }

                    if ( mapping->ControllerId == ControllerId && mapping->AxisOrActionIndex != -1 ) {
                        if ( mapping->bAxis ) {
                            pressedKey.AxisScale = mapping->AxisScale;

                            AInputAxis * inputAxis = InputMappings->GetAxes()[ mapping->AxisOrActionIndex ];

                            pressedKey.AxisBinding = GetAxisBinding( inputAxis );

                        } else {
                            if ( ( _ModMask & mapping->ModMask ) == mapping->ModMask ) {
                                AInputAction * inputAction = InputMappings->GetActions()[ mapping->AxisOrActionIndex ];
                                pressedKey.ActionBinding = GetActionBinding( inputAction );
                            }
                        }
                    }
                }

                ButtonIndex[ _Button ] = NumPressedKeys;

                NumPressedKeys++;

                if ( pressedKey.ActionBinding != -1 ) {

                    SActionBinding & binding = ActionBindings[ pressedKey.ActionBinding ];

                    if ( GetWorld()->IsPaused() && !binding.bExecuteEvenWhenPaused ) {
                        pressedKey.ActionBinding = -1;
                    } else {
                        binding.Callback[ IA_PRESS ]();
                    }
                }

            } else {
                GLogger.Printf( "MAX_PRESSED_KEYS hit\n" );
            }
        } else {

            // Button is repressed

            //AN_ASSERT( 0 );

        }
    } else if ( _Action == IA_RELEASE ) {
        if ( ButtonIndex[ _Button ] != -1 ) {

            int index = ButtonIndex[ _Button ];

            int actionBinding = PressedKeys[ index ].ActionBinding;

            DeviceButtonDown[ PressedKeys[ index ].DevId ][ PressedKeys[ index ].Key ] = -1;

            if ( index != NumPressedKeys - 1 ) {
                // Move last array element to position "index"
                PressedKeys[ index ] = PressedKeys[ NumPressedKeys - 1 ];

                // Refresh index of moved element
                DeviceButtonDown[ PressedKeys[ index ].DevId ][ PressedKeys[ index ].Key ] = index;
            }

            // Pop back
            NumPressedKeys--;
            AN_ASSERT( NumPressedKeys >= 0 );

            if ( actionBinding != -1 /*&& !PressedKeys[ index ].bMarkedReleased*/ ) {
                ActionBindings[ actionBinding ].Callback[ IA_RELEASE ]();
            }
        }
    }
}

bool AInputComponent::GetButtonState( int _DevId, int _Button ) const {
    AN_ASSERT( _DevId >= 0 && _DevId < MAX_INPUT_DEVICES );

    return DeviceButtonDown[ _DevId ][ _Button ] != -1;
}

void AInputComponent::UnpressButtons() {
    double timeStamp = GRuntime.SysSeconds_d();
    for ( int i = 0 ; i < MAX_KEYBOARD_BUTTONS ; i++ ) {
        SetButtonState( ID_KEYBOARD, i, IA_RELEASE, 0, timeStamp );
    }
    for ( int i = 0 ; i < MAX_MOUSE_BUTTONS ; i++ ) {
        SetButtonState( ID_MOUSE, i, IA_RELEASE, 0, timeStamp );
    }
    for ( int j = 0 ; j < MAX_JOYSTICKS_COUNT ; j++ ) {
        for ( int i = 0 ; i < MAX_JOYSTICK_BUTTONS ; i++ ) {
            SetButtonState( ID_JOYSTICK_1 + j, i, IA_RELEASE, 0, timeStamp );
        }
    }
}

//bool AInputComponent::IsJoyDown( SJoystick const * _Joystick, int _Button ) const {
//    return GetButtonState( ID_JOYSTICK_1 + _Joystick->Id, _Button );
//}

bool AInputComponent::IsJoyDown( int _JoystickId, int _Button ) const {
    return GetButtonState( ID_JOYSTICK_1 + _JoystickId, _Button );
}

void AInputComponent::NotifyUnicodeCharacter( SWideChar _UnicodeCharacter, int _ModMask, double _TimeStamp ) {
    if ( bIgnoreCharEvents ) {
        return;
    }

    if ( !CharacterCallback.IsValid() ) {
        return;
    }

    if ( GetWorld()->IsPaused() && !bCharacterCallbackExecuteEvenWhenPaused ) {
        return;
    }

    CharacterCallback( _UnicodeCharacter, _ModMask, _TimeStamp );
}

void AInputComponent::SetMouseAxisState( float _X, float _Y ) {
    if ( bIgnoreMouseEvents ) {
        return;
    }

    MouseAxisStateX += _X * MouseSensitivity;
    MouseAxisStateY += _Y * MouseSensitivity;
}

float AInputComponent::GetMouseAxisState( int _Axis ) {
    return (&MouseAxisStateX)[_Axis];
}

//void AInputComponent::SetJoystickState( int _Joystick, int _NumAxes, int _NumButtons, bool _bGamePad, bool _bConnected ) {
//    Static.Joysticks[_Joystick].NumAxes = _NumAxes;
//    Static.Joysticks[_Joystick].NumButtons = _NumButtons;
//    Static.Joysticks[_Joystick].bGamePad = _bGamePad;
//    Static.Joysticks[_Joystick].bConnected = _bConnected;
//}

void AInputComponent::SetJoystickAxisState( int _Joystick, int _Axis, float _Value ) {
    Static.JoystickAxisState[_Joystick][_Axis] = _Value;
}

float AInputComponent::GetJoystickAxisState( int _Joystick, int _Axis ) {
    return Static.JoystickAxisState[_Joystick][_Axis];
}

//const SJoystick * AInputComponent::GetJoysticks() {
//    return Static.Joysticks;
//}

void AInputComponent::BindAxis( const char * _Axis, TCallback< void( float ) > const & _Callback, bool _ExecuteEvenWhenPaused ) {
    int hash = Core::HashCase( _Axis, Core::Strlen( _Axis ) );

    for ( int i = AxisBindingsHash.First( hash ) ; i != -1 ; i = AxisBindingsHash.Next( i ) ) {
        if ( !AxisBindings[i].Name.Icmp( _Axis ) ) {
            AxisBindings[i].Callback = _Callback;
            return;
        }
    }

    if ( AxisBindings.size() >= MAX_AXIS_BINDINGS ) {
        GLogger.Printf( "MAX_AXIS_BINDINGS hit\n" );
        return;
    }

    AxisBindingsHash.Insert( hash, AxisBindings.size() );
    SAxisBinding binding;
    binding.Name = _Axis;
    binding.Callback = _Callback;
    binding.AxisScale = 0.0f;
    binding.bExecuteEvenWhenPaused = _ExecuteEvenWhenPaused;
    AxisBindings.Append( binding );
}

void AInputComponent::UnbindAxis( const char * _Axis ) {
    int hash = Core::HashCase( _Axis, Core::Strlen( _Axis ) );

    for ( int i = AxisBindingsHash.First( hash ) ; i != -1 ; i = AxisBindingsHash.Next( i ) ) {
        if ( !AxisBindings[i].Name.Icmp( _Axis ) ) {
            AxisBindingsHash.RemoveIndex( hash, i );
            auto it = AxisBindings.begin() + i;
            AxisBindings.erase( it );

            for ( SPressedKey * pressedKey = PressedKeys ; pressedKey < &PressedKeys[ NumPressedKeys ] ; pressedKey++ ) {
                if ( pressedKey->AxisBinding == i ) {
                    pressedKey->AxisBinding = -1;
                }
            }

            return;
        }
    }
}

void AInputComponent::BindAction( const char * _Action, int _Event, TCallback< void() > const & _Callback, bool _ExecuteEvenWhenPaused ) {
    if ( _Event != IA_PRESS && _Event != IA_RELEASE ) {
        GLogger.Printf( "AInputComponent::BindAction: expected IE_Press or IE_Release event\n" );
        return;
    }

    int hash = Core::HashCase( _Action, Core::Strlen( _Action ) );

    for ( int i = ActionBindingsHash.First( hash ) ; i != -1 ; i = ActionBindingsHash.Next( i ) ) {
        if ( !ActionBindings[i].Name.Icmp( _Action ) ) {
            ActionBindings[ i ].Callback[ _Event ] = _Callback;
            return;
        }
    }

    if ( ActionBindings.size() >= MAX_ACTION_BINDINGS ) {
        GLogger.Printf( "MAX_ACTION_BINDINGS hit\n" );
        return;
    }

    ActionBindingsHash.Insert( hash, ActionBindings.size() );
    SActionBinding binding;
    binding.Name = _Action;
    binding.Callback[ _Event ] = _Callback;
    binding.bExecuteEvenWhenPaused = _ExecuteEvenWhenPaused;
    ActionBindings.Append( binding );
}

void AInputComponent::UnbindAction( const char * _Action ) {
    int hash = Core::HashCase( _Action, Core::Strlen( _Action ) );

    for ( int i = ActionBindingsHash.First( hash ) ; i != -1 ; i = ActionBindingsHash.Next( i ) ) {
        if ( !ActionBindings[i].Name.Icmp( _Action ) ) {
            ActionBindingsHash.RemoveIndex( hash, i );
            auto it = ActionBindings.begin() + i;
            ActionBindings.erase( it );

            for ( SPressedKey * pressedKey = PressedKeys ; pressedKey < &PressedKeys[ NumPressedKeys ] ; pressedKey++ ) {
                if ( pressedKey->ActionBinding == i ) {
                    pressedKey->ActionBinding = -1;
                }
            }

            return;
        }
    }
}

void AInputComponent::UnbindAll() {
    AxisBindingsHash.Clear();
    AxisBindings.Clear();

    ActionBindingsHash.Clear();
    ActionBindings.Clear();

    for ( SPressedKey * pressedKey = PressedKeys ; pressedKey < &PressedKeys[ NumPressedKeys ] ; pressedKey++ ) {
        pressedKey->AxisBinding = -1;
        pressedKey->ActionBinding = -1;
    }
}

void AInputComponent::SetCharacterCallback( TCallback< void( SWideChar, int, double ) > const & _Callback, bool _ExecuteEvenWhenPaused ) {
    CharacterCallback = _Callback;
    bCharacterCallbackExecuteEvenWhenPaused = _ExecuteEvenWhenPaused;
}

void AInputComponent::UnsetCharacterCallback() {
    CharacterCallback.Clear();
}

int AInputComponent::GetAxisBinding( const char * _Axis ) const {
    return GetAxisBindingHash( _Axis, Core::HashCase( _Axis, Core::Strlen( _Axis ) ) );
}

int AInputComponent::GetAxisBinding( const AInputAxis * _Axis ) const {
    return GetAxisBindingHash( _Axis->GetObjectName().CStr(), _Axis->GetNameHash() );
}

int AInputComponent::GetAxisBindingHash( const char * _Axis, int _Hash ) const {
    for ( int i = AxisBindingsHash.First( _Hash ) ; i != -1 ; i = AxisBindingsHash.Next( i ) ) {
        if ( !AxisBindings[i].Name.Icmp( _Axis ) ) {
            return i;
        }
    }
    return -1;
}

int AInputComponent::GetActionBinding( const char * _Action ) const {
    return GetActionBindingHash( _Action, Core::HashCase( _Action, Core::Strlen( _Action ) ) );
}

int AInputComponent::GetActionBinding( const AInputAction * _Action ) const {
    return GetActionBindingHash( _Action->GetObjectName().CStr(), _Action->GetNameHash() );
}

int AInputComponent::GetActionBindingHash( const char * _Action, int _Hash ) const {
    for ( int i = ActionBindingsHash.First( _Hash ) ; i != -1 ; i = ActionBindingsHash.Next( i ) ) {
        if ( !ActionBindings[i].Name.Icmp( _Action ) ) {
            return i;
        }
    }
    return -1;
}

AInputMappings::AInputMappings() {
    Core::Memset( KeyboardMappings, 0xff, sizeof( KeyboardMappings ) );
    Core::Memset( MouseMappings, 0xff, sizeof( MouseMappings ) );
    Core::Memset( MouseAxisMappings, 0xff, sizeof( MouseAxisMappings ) );
    Core::Memset( JoystickMappings, 0xff, sizeof( JoystickMappings ) );
    Core::Memset( JoystickAxisMappings, 0xff, sizeof( JoystickAxisMappings ) );
}

AInputMappings::~AInputMappings() {
    for ( int i = 0 ; i < Axes.Size() ; i++ ) {
        Axes[i]->RemoveRef();
    }
    for ( int i = 0 ; i < Actions.Size() ; i++ ) {
        Actions[i]->RemoveRef();
    }
}

int AInputMappings::Serialize( ADocument & _Doc ) {
    int object = Super::Serialize( _Doc );
    if ( !Axes.IsEmpty() ) {
        int axes = _Doc.AddArray( object, "Axes" );

        for ( AInputAxis const * axis : Axes ) {
            AString & axisName = _Doc.ProxyBuffer.NewString( axis->GetObjectName() );

            for ( int deviceId = 0 ; deviceId < MAX_INPUT_DEVICES ; deviceId++ ) {
                if ( !axis->MappedKeys[ deviceId ].IsEmpty() ) {
                    const char * deviceName = AInputHelper::TranslateDevice( deviceId );
                    for ( unsigned short key : axis->MappedKeys[ deviceId ] ) {
                        SMapping * mapping = GetMapping( deviceId, key );
                        int axisObject = _Doc.CreateObjectValue();
                        _Doc.AddStringField( axisObject, "Name", axisName.CStr() );
                        _Doc.AddStringField( axisObject, "Device", deviceName );
                        _Doc.AddStringField( axisObject, "Key", AInputHelper::TranslateDeviceKey( deviceId, key ) );
                        _Doc.AddStringField( axisObject, "Scale", _Doc.ProxyBuffer.NewString( Math::ToString( mapping->AxisScale ) ).CStr() );
                        _Doc.AddStringField( axisObject, "Owner", AInputHelper::TranslateController( mapping->ControllerId ) );
                        _Doc.AddValueToField( axes, axisObject );
                    }
                }
            }

            if ( axis->MappedMouseAxes ) {
                const char * deviceName = AInputHelper::TranslateDevice( ID_MOUSE );
                for ( int i = 0 ; i < MAX_MOUSE_AXES ; i++ ) {
                    if ( axis->MappedMouseAxes & ( 1 << i ) ) {
                        SMapping * mapping = GetMapping( ID_MOUSE, MOUSE_AXIS_BASE + i );
                        int axisObject = _Doc.CreateObjectValue();
                        _Doc.AddStringField( axisObject, "Name", axisName.CStr() );
                        _Doc.AddStringField( axisObject, "Device", deviceName );
                        _Doc.AddStringField( axisObject, "Key", AInputHelper::TranslateDeviceKey( ID_MOUSE, MOUSE_AXIS_BASE + i ) );
                        _Doc.AddStringField( axisObject, "Scale", _Doc.ProxyBuffer.NewString( Math::ToString( mapping->AxisScale ) ).CStr() );
                        _Doc.AddStringField( axisObject, "Owner", AInputHelper::TranslateController( mapping->ControllerId ) );
                        _Doc.AddValueToField( axes, axisObject );
                    }
                }
            }

            for ( int joyId = 0 ; joyId < MAX_JOYSTICKS_COUNT ; joyId++ ) {
                if ( axis->MappedJoystickAxes[ joyId ] ) {
                    const char * deviceName = AInputHelper::TranslateDevice( ID_JOYSTICK_1 + joyId );
                    for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
                        if ( axis->MappedJoystickAxes[ joyId ] & ( 1 << i ) ) {
                            SMapping * mapping = GetMapping( ID_JOYSTICK_1 + joyId, JOY_AXIS_BASE + i );
                            int axisObject = _Doc.CreateObjectValue();
                            _Doc.AddStringField( axisObject, "Name", axisName.CStr() );
                            _Doc.AddStringField( axisObject, "Device", deviceName );
                            _Doc.AddStringField( axisObject, "Key", AInputHelper::TranslateDeviceKey( ID_JOYSTICK_1 + joyId, JOY_AXIS_BASE + i ) );
                            _Doc.AddStringField( axisObject, "Scale", _Doc.ProxyBuffer.NewString( Math::ToString( mapping->AxisScale ) ).CStr() );
                            _Doc.AddStringField( axisObject, "Owner", AInputHelper::TranslateController( mapping->ControllerId ) );
                            _Doc.AddValueToField( axes, axisObject );
                        }
                    }
                }
            }
        }
    }

    if ( !Actions.IsEmpty() ) {
        int actions = _Doc.AddArray( object, "Actions" );

        for ( AInputAction const * action : Actions ) {

            AString & actionName = _Doc.ProxyBuffer.NewString( action->GetObjectName() );

            for ( int deviceId = 0 ; deviceId < MAX_INPUT_DEVICES ; deviceId++ ) {
                if ( !action->MappedKeys[ deviceId ].IsEmpty() ) {
                    const char * deviceName = AInputHelper::TranslateDevice( deviceId );
                    for ( unsigned short key : action->MappedKeys[ deviceId ] ) {
                        SMapping * mapping = GetMapping( deviceId, key );
                        int actionObject = _Doc.CreateObjectValue();
                        _Doc.AddStringField( actionObject, "Name", actionName.CStr() );
                        _Doc.AddStringField( actionObject, "Device", deviceName );
                        _Doc.AddStringField( actionObject, "Key", AInputHelper::TranslateDeviceKey( deviceId, key ) );
                        _Doc.AddStringField( actionObject, "Owner", AInputHelper::TranslateController( mapping->ControllerId ) );
                        if ( mapping->ModMask ) {
                            _Doc.AddStringField( actionObject, "ModMask", _Doc.ProxyBuffer.NewString( Math::ToString( mapping->ModMask ) ).CStr() );
                        }
                        _Doc.AddValueToField( actions, actionObject );
                    }
                }
            }
        }
    }

    return object;
}

AInputMappings * AInputMappings::LoadMappings( ADocument const & _Document, int _FieldsHead ) {
    SDocumentField * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "AInputMappings::LoadMappings: invalid class\n" );
        return nullptr;
    }

    SDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    AObjectFactory const * factory = AInputMappings::ClassMeta().Factory();

    AClassMeta const * classMeta = factory->LookupClass( classNameValue->Token.ToString().CStr() );
    if ( !classMeta ) {
        GLogger.Printf( "AInputMappings::LoadMappings: invalid class \"%s\"\n", classNameValue->Token.ToString().CStr() );
        return nullptr;
    }

    AInputMappings * inputMappings = static_cast< AInputMappings * >( classMeta->CreateInstance() );

    // Load attributes
    inputMappings->LoadAttributes( _Document, _FieldsHead );

    // Load axes
    SDocumentField * axesArray = _Document.FindField( _FieldsHead, "Axes" );
    if ( axesArray ) {
        inputMappings->LoadAxes( _Document, ( size_t )( axesArray - &_Document.Fields[0] ) );
    }

    // Load actions
    SDocumentField * actionsArray = _Document.FindField( _FieldsHead, "Actions" );
    if ( actionsArray ) {
        inputMappings->LoadActions( _Document, ( size_t )( actionsArray - &_Document.Fields[0] ) );
    }

    return inputMappings;
}

void AInputMappings::LoadAxes( ADocument const & _Document, int _FieldsHead ) {
    SDocumentField * field = &_Document.Fields[ _FieldsHead ];
    for ( int i = field->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
        SDocumentValue * value = &_Document.Values[ i ];
        if ( value->Type != SDocumentValue::TYPE_OBJECT ) {
            continue;
        }

        SDocumentField * nameField = _Document.FindField( value->FieldsHead, "Name" );
        if ( !nameField ) {
            continue;
        }

        SDocumentField * deviceField = _Document.FindField( value->FieldsHead, "Device" );
        if ( !deviceField ) {
            continue;
        }

        SDocumentField * keyField = _Document.FindField( value->FieldsHead, "Key" );
        if ( !keyField ) {
            continue;
        }

        SDocumentField * scaleField = _Document.FindField( value->FieldsHead, "Scale" );
        if ( !scaleField ) {
            continue;
        }

        SDocumentField * ownerField = _Document.FindField( value->FieldsHead, "Owner" );
        if ( !ownerField ) {
            continue;
        }

        SToken const & name = _Document.Values[ nameField->ValuesHead ].Token;
        SToken const & device = _Document.Values[ deviceField->ValuesHead ].Token;
        SToken const & key = _Document.Values[ keyField->ValuesHead ].Token;
        SToken const & scale = _Document.Values[ scaleField->ValuesHead ].Token;
        SToken const & controller = _Document.Values[ ownerField->ValuesHead ].Token;

        int deviceId = AInputHelper::LookupDevice( device.ToString().CStr() );
        int deviceKey = AInputHelper::LookupDeviceKey( deviceId, key.ToString().CStr() );
        int controllerId = AInputHelper::LookupController( controller.ToString().CStr() );

        float scaleValue = Math::ToFloat( scale.ToString() );

        MapAxis( name.ToString().CStr(),
                 deviceId,
                 deviceKey,
                 scaleValue,
                 controllerId );
    }
}

void AInputMappings::LoadActions( ADocument const & _Document, int _FieldsHead ) {
    SDocumentField * field = &_Document.Fields[ _FieldsHead ];
    for ( int i = field->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
        SDocumentValue * value = &_Document.Values[ i ];
        if ( value->Type != SDocumentValue::TYPE_OBJECT ) {
            continue;
        }

        SDocumentField * nameField = _Document.FindField( value->FieldsHead, "Name" );
        if ( !nameField ) {
            continue;
        }

        SDocumentField * deviceField = _Document.FindField( value->FieldsHead, "Device" );
        if ( !deviceField ) {
            continue;
        }

        SDocumentField * keyField = _Document.FindField( value->FieldsHead, "Key" );
        if ( !keyField ) {
            continue;
        }

        SDocumentField * ownerField = _Document.FindField( value->FieldsHead, "Owner" );
        if ( !ownerField ) {
            continue;
        }

        int32_t modMask = 0;
        SDocumentField * modMaskField = _Document.FindField( value->FieldsHead, "ModMask" );
        if ( modMaskField ) {
            modMask = Math::ToInt< int32_t >( _Document.Values[ nameField->ValuesHead ].Token.ToString() );
        }

        SToken const & name = _Document.Values[ nameField->ValuesHead ].Token;
        SToken const & device = _Document.Values[ deviceField->ValuesHead ].Token;
        SToken const & key = _Document.Values[ keyField->ValuesHead ].Token;
        SToken const & controller = _Document.Values[ ownerField->ValuesHead ].Token;

        int deviceId = AInputHelper::LookupDevice( device.ToString().CStr() );
        int deviceKey = AInputHelper::LookupDeviceKey( deviceId, key.ToString().CStr() );
        int controllerId = AInputHelper::LookupController( controller.ToString().CStr() );

        MapAction( name.ToString().CStr(),
                   deviceId,
                   deviceKey,
                   modMask,
                   controllerId );
    }
}

AInputAxis * AInputMappings::AddAxis( const char * _Name ) {
    AInputAxis * axis = NewObject< AInputAxis >();
    axis->AddRef();
    axis->Parent = this;
    axis->IndexInArrayOfAxes = Axes.Size();
    axis->SetObjectName( _Name );
    axis->NameHash = axis->GetObjectName().HashCase();
    Axes.Append( axis );
    return axis;
}

AInputAction * AInputMappings::AddAction( const char * _Name ) {
    AInputAction * action = NewObject< AInputAction >();
    action->AddRef();
    action->Parent = this;
    action->IndexInArrayOfActions = Actions.Size();
    action->SetObjectName( _Name );
    action->NameHash = action->GetObjectName().HashCase();
    Actions.Append( action );
    return action;
}

AInputAxis * AInputMappings::FindAxis( const char * _AxisName ) {
    for ( AInputAxis * axis : Axes ) {
        if ( !axis->GetObjectName().Icmp( _AxisName ) ) {
            return axis;
        }
    }
    return nullptr;
}

AInputAction * AInputMappings::FindAction( const char * _ActionName ) {
    for ( AInputAction * action : Actions ) {
        if ( !action->GetObjectName().Icmp( _ActionName ) ) {
            return action;
        }
    }
    return nullptr;
}

void AInputMappings::MapAxis( const char * _AxisName, int _DevId, int _KeyToken, float _AxisScale, int _ControllerId ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return;
    }

    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        return;
    }

    Unmap( _DevId, _KeyToken );

    AInputAxis * axis = FindAxis( _AxisName );

    axis = ( !axis ) ? AddAxis( _AxisName ) : axis;

    AInputMappings::SMapping * mapping;

    switch ( _DevId ) {
    case ID_KEYBOARD:
        mapping = &KeyboardMappings[ _KeyToken ];
        axis->MappedKeys[_DevId].Append( _KeyToken );
        break;
    case ID_MOUSE:
        if ( _KeyToken >= MOUSE_AXIS_BASE ) {
            int axisId = _KeyToken - MOUSE_AXIS_BASE;

            mapping = &MouseAxisMappings[ axisId ];
            axis->MappedMouseAxes |= 1 << axisId;
        } else {
            mapping = &MouseMappings[ _KeyToken ];
            axis->MappedKeys[_DevId].Append( _KeyToken );
        }
        break;
    case ID_JOYSTICK_1:
    case ID_JOYSTICK_2:
    case ID_JOYSTICK_3:
    case ID_JOYSTICK_4:
    case ID_JOYSTICK_5:
    case ID_JOYSTICK_6:
    case ID_JOYSTICK_7:
    case ID_JOYSTICK_8:
    case ID_JOYSTICK_9:
    case ID_JOYSTICK_10:
    case ID_JOYSTICK_11:
    case ID_JOYSTICK_12:
    case ID_JOYSTICK_13:
    case ID_JOYSTICK_14:
    case ID_JOYSTICK_15:
    case ID_JOYSTICK_16:
    {
        int joystickId = _DevId - ID_JOYSTICK_1;
        if ( _KeyToken >= JOY_AXIS_BASE ) {
            int axisId = _KeyToken - JOY_AXIS_BASE;

            mapping = &JoystickAxisMappings[ joystickId ][ axisId ];
            axis->MappedJoystickAxes[joystickId] |= 1 << axisId;
        } else {
            mapping = &JoystickMappings[ joystickId ][ _KeyToken ];
            axis->MappedKeys[_DevId].Append( _KeyToken );
        }
        break;
    }
    default:
        AN_ASSERT(0);
        return;
    }

    mapping->AxisOrActionIndex = axis->IndexInArrayOfAxes;
    mapping->bAxis = true;
    mapping->AxisScale = _AxisScale;
    mapping->ControllerId = _ControllerId;
}

void AInputMappings::MapAction( const char * _ActionName, int _DevId, int _KeyToken, int _ModMask, int _ControllerId ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return;
    }

    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        return;
    }

    if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 && _KeyToken >= JOY_AXIS_BASE ) {
        // cannot map joystick axis and action!
        return;
    }

    if ( _DevId == ID_MOUSE && _KeyToken >= MOUSE_AXIS_BASE ) {
        // cannot map mouse axis and action!
        return;
    }

    Unmap( _DevId, _KeyToken );

    AInputAction * action = FindAction( _ActionName );

    action = ( !action ) ? AddAction( _ActionName ) : action;

    AInputMappings::SMapping * mapping;

    switch ( _DevId ) {
    case ID_KEYBOARD:
        mapping = &KeyboardMappings[ _KeyToken ];
        break;
    case ID_MOUSE:
        mapping = &MouseMappings[ _KeyToken ];
        break;
    case ID_JOYSTICK_1:
    case ID_JOYSTICK_2:
    case ID_JOYSTICK_3:
    case ID_JOYSTICK_4:
    case ID_JOYSTICK_5:
    case ID_JOYSTICK_6:
    case ID_JOYSTICK_7:
    case ID_JOYSTICK_8:
    case ID_JOYSTICK_9:
    case ID_JOYSTICK_10:
    case ID_JOYSTICK_11:
    case ID_JOYSTICK_12:
    case ID_JOYSTICK_13:
    case ID_JOYSTICK_14:
    case ID_JOYSTICK_15:
    case ID_JOYSTICK_16:
    {
        int joystickId = _DevId - ID_JOYSTICK_1;
        mapping = &JoystickMappings[ joystickId ][ _KeyToken ];
        break;
    }
    default:
        AN_ASSERT(0);
        return;
    }

    action->MappedKeys[_DevId].Append( _KeyToken );

    mapping->AxisOrActionIndex = action->IndexInArrayOfActions;
    mapping->bAxis = false;
    mapping->AxisScale = 0;
    mapping->ControllerId = _ControllerId;
    mapping->ModMask = _ModMask & 0xff;
}

void AInputMappings::Unmap( int _DevId, int _KeyToken ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return;
    }

    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        return;
    }

    AInputMappings::SMapping * mapping;

    switch ( _DevId ) {
    case ID_KEYBOARD:
        mapping = &KeyboardMappings[ _KeyToken ];
        break;
    case ID_MOUSE:
        if ( _KeyToken >= MOUSE_AXIS_BASE ) {
            int axisId = _KeyToken - MOUSE_AXIS_BASE;
            mapping = &MouseAxisMappings[ axisId ];
        } else {
            mapping = &MouseMappings[ _KeyToken ];
        }
        break;
    case ID_JOYSTICK_1:
    case ID_JOYSTICK_2:
    case ID_JOYSTICK_3:
    case ID_JOYSTICK_4:
    case ID_JOYSTICK_5:
    case ID_JOYSTICK_6:
    case ID_JOYSTICK_7:
    case ID_JOYSTICK_8:
    case ID_JOYSTICK_9:
    case ID_JOYSTICK_10:
    case ID_JOYSTICK_11:
    case ID_JOYSTICK_12:
    case ID_JOYSTICK_13:
    case ID_JOYSTICK_14:
    case ID_JOYSTICK_15:
    case ID_JOYSTICK_16:
    {
        int joystickId = _DevId - ID_JOYSTICK_1;
        if ( _KeyToken >= JOY_AXIS_BASE ) {
            int axisId = _KeyToken - JOY_AXIS_BASE;
            mapping = &JoystickAxisMappings[ joystickId ][ axisId ];
        } else {
            mapping = &JoystickMappings[ _DevId - ID_JOYSTICK_1 ][ _KeyToken ];
        }
        break;
    }
    default:
        AN_ASSERT(0);
        return;
    }

    if ( mapping->AxisOrActionIndex == -1 ) {
        return;
    }

    if ( mapping->bAxis ) {
        AInputAxis * axis = Axes[ mapping->AxisOrActionIndex ];

        if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 && _KeyToken >= JOY_AXIS_BASE ) {
            int joystickId = _DevId - ID_JOYSTICK_1;
            int axisId = _KeyToken - JOY_AXIS_BASE;
            axis->MappedJoystickAxes[joystickId] &= ~( 1 << axisId );
        } else if ( _DevId == ID_MOUSE && _KeyToken >= MOUSE_AXIS_BASE ) {
            int axisId = _KeyToken - MOUSE_AXIS_BASE;
            axis->MappedMouseAxes &= ~( 1 << axisId );
        } else {
            for ( auto it = axis->MappedKeys[_DevId].begin() ; it != axis->MappedKeys[_DevId].end() ; it++ ) {
                if ( *it == _KeyToken ) {
                    axis->MappedKeys[_DevId].Erase( it );
                    break;
                }
            }
        }
    } else {
        AInputAction * action = Actions[ mapping->AxisOrActionIndex ];

        for ( auto it = action->MappedKeys[_DevId].begin() ; it != action->MappedKeys[_DevId].end() ; it++ ) {
            if ( *it == _KeyToken ) {
                action->MappedKeys[_DevId].Erase( it );
                break;
            }
        }
    }

    mapping->AxisOrActionIndex = -1;
}

void AInputMappings::UnmapAll() {
    for ( int i = 0 ; i < Axes.Size() ; i++ ) {
        Axes[i]->RemoveRef();
    }
    for ( int i = 0 ; i < Actions.Size() ; i++ ) {
        Actions[i]->RemoveRef();
    }
    Axes.Clear();
    Actions.Clear();
    Core::Memset( KeyboardMappings, 0xff, sizeof( KeyboardMappings ) );
    Core::Memset( MouseMappings, 0xff, sizeof( MouseMappings ) );
    Core::Memset( MouseAxisMappings, 0xff, sizeof( MouseAxisMappings ) );
    Core::Memset( JoystickMappings, 0xff, sizeof( JoystickMappings ) );
    Core::Memset( JoystickAxisMappings, 0xff, sizeof( JoystickAxisMappings ) );
}

AInputMappings::SMapping * AInputMappings::GetMapping( int _DevId, int _KeyToken ) {
    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        AN_ASSERT( 0 );
        return nullptr;
    }

    switch ( _DevId ) {
    case ID_KEYBOARD:
        return &KeyboardMappings[ _KeyToken ];
    case ID_MOUSE:
        return _KeyToken >= MOUSE_AXIS_BASE ? &MouseAxisMappings[ _KeyToken - MOUSE_AXIS_BASE ]
                : &MouseMappings[ _KeyToken ];
    }

    if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 ) {
        int joystickId = _DevId - ID_JOYSTICK_1;
        return _KeyToken >= JOY_AXIS_BASE ? &JoystickAxisMappings[ joystickId ][ _KeyToken - JOY_AXIS_BASE ]
                    : &JoystickMappings[ joystickId ][ _KeyToken ];
    }

    AN_ASSERT(0);

    return nullptr;
}

AInputAxis::AInputAxis() {
    MappedMouseAxes = 0;
    Core::ZeroMem( MappedJoystickAxes,  sizeof( MappedJoystickAxes ) );
}

void AInputAxis::Map( int _DevId, int _KeyToken, float _AxisScale, int _ControllerId ) {
    Parent->MapAxis( GetObjectNameCStr(), _DevId, _KeyToken, _AxisScale, _ControllerId );
}

void AInputAction::Map( int _DevId, int _KeyToken, int _ModMask, int _ControllerId ) {
    Parent->MapAction( GetObjectNameCStr(), _DevId, _KeyToken, _ModMask, _ControllerId );
}
