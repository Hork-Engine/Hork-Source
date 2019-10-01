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

#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/World.h>
#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/HashFunc.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META( FInputAxis )
AN_CLASS_META( FInputAction )
AN_CLASS_META( FInputMappings )

AN_BEGIN_CLASS_META( FInputComponent )
//AN_ATTRIBUTE_( ReceiveInputMask, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreKeyboardEvents, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreMouseEvents, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreJoystickEvents, AF_DEFAULT )
AN_ATTRIBUTE_( bIgnoreCharEvents, AF_DEFAULT )
AN_ATTRIBUTE_( ControllerId, AF_DEFAULT )
AN_END_CLASS_META()

struct FInputComponentStatic {
    const char * KeyNames[ MAX_KEYBOARD_BUTTONS ];
    const char * MouseButtonNames[ MAX_MOUSE_BUTTONS ];
    const char * MouseAxisNames[ MAX_MOUSE_AXES ];
    const char * DeviceNames[ MAX_INPUT_DEVICES ];
    const char * JoystickButtonNames[ MAX_JOYSTICK_BUTTONS ];
    const char * JoystickAxisNames[ MAX_JOYSTICK_AXES ];
    const char * ModifierNames[ MAX_MODIFIERS ];
    const char * ControllerNames[ MAX_INPUT_CONTROLLERS ];

    int DeviceButtonLimits[ MAX_INPUT_DEVICES ];
    FJoystick Joysticks[ MAX_JOYSTICKS_COUNT ];
    float JoystickAxisState[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];

    FInputComponentStatic() {
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
        InitKey( KEY_WORLD_1 );
        InitKey( KEY_WORLD_2 );
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
        InitKey( KEY_F25 );
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

        InitModifier( MOD_SHIFT );
        InitModifier( MOD_CONTROL );
        InitModifier( MOD_ALT );
        InitModifier( MOD_SUPER );
        InitModifier( MOD_CAPS_LOCK );
        InitModifier( MOD_NUM_LOCK );

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

        ZeroMem( Joysticks, sizeof( Joysticks ) );
        ZeroMem( JoystickAxisState, sizeof( JoystickAxisState ) );

        for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
            Joysticks[i].Id = i;
        }
    }
};

static FInputComponentStatic Static;

FInputComponent * FInputComponent::InputComponents = nullptr;
FInputComponent * FInputComponent::InputComponentsTail = nullptr;

const char * FInputHelper::TranslateDevice( int _DevId ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return "UNKNOWN";
    }
    return Static.DeviceNames[ _DevId ];
}

const char * FInputHelper::TranslateModifier( int _Modifier ) {
    if ( _Modifier < 0 || _Modifier > MOD_LAST ) {
        return "UNKNOWN";
    }
    return Static.ModifierNames[ _Modifier ];
}

const char * FInputHelper::TranslateDeviceKey( int _DevId, int _Key ) {
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

const char * FInputHelper::TranslateController( int _ControllerId ) {
    if ( _ControllerId < 0 || _ControllerId >= MAX_INPUT_CONTROLLERS ) {
        return "UNKNOWN";
    }
    return Static.ControllerNames[ _ControllerId ];
}

int FInputHelper::LookupDevice( const char * _Device ) {
    for ( int i = 0 ; i < MAX_INPUT_DEVICES ; i++ ) {
        if ( !FString::Icmp( Static.DeviceNames[i], _Device ) ) {
            return i;
        }
    }
    return -1;
}

int FInputHelper::LookupModifier( const char * _Modifier ) {
    for ( int i = 0 ; i < MAX_MODIFIERS ; i++ ) {
        if ( !FString::Icmp( Static.ModifierNames[i], _Modifier ) ) {
            return i;
        }
    }
    return -1;
}

int FInputHelper::LookupDeviceKey( int _DevId, const char * _Key ) {
    switch ( _DevId ) {
    case ID_KEYBOARD:
        for ( int i = 0 ; i < MAX_KEYBOARD_BUTTONS ; i++ ) {
            if ( !FString::Icmp( Static.KeyNames[i], _Key ) ) {
                return i;
            }
        }
        return -1;
    case ID_MOUSE:
        for ( int i = 0 ; i < MAX_MOUSE_BUTTONS ; i++ ) {
            if ( !FString::Icmp( Static.MouseButtonNames[i], _Key ) ) {
                return MOUSE_BUTTON_BASE + i;
            }
        }
        for ( int i = 0 ; i < MAX_MOUSE_AXES ; i++ ) {
            if ( !FString::Icmp( Static.MouseAxisNames[i], _Key ) ) {
                return MOUSE_AXIS_BASE + i;
            }
        }
        return -1;
    }
    if ( _DevId >= ID_JOYSTICK_1 && _DevId <= ID_JOYSTICK_16 ) {
        for ( int i = 0 ; i < MAX_JOYSTICK_BUTTONS ; i++ ) {
            if ( !FString::Icmp( Static.JoystickButtonNames[i], _Key ) ) {
                return JOY_BUTTON_BASE + i;
            }
        }
        for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
            if ( !FString::Icmp( Static.JoystickAxisNames[i], _Key ) ) {
                return JOY_AXIS_BASE + i;
            }
        }
    }
    return -1;
}

int FInputHelper::LookupController( const char * _Controller ) {
    for ( int i = 0 ; i < MAX_INPUT_CONTROLLERS ; i++ ) {
        if ( !FString::Icmp( Static.ControllerNames[i], _Controller ) ) {
            return i;
        }
    }
    return -1;
}

FInputComponent::FInputComponent() {
    DeviceButtonDown[ ID_KEYBOARD ] = KeyboardButtonDown;
    DeviceButtonDown[ ID_MOUSE ] = MouseButtonDown;
    for ( int i = 0 ; i < MAX_JOYSTICKS_COUNT ; i++ ) {
        DeviceButtonDown[ ID_JOYSTICK_1 + i ] = JoystickButtonDown[ i ];
    }
    Memset( KeyboardButtonDown, 0xff, sizeof( KeyboardButtonDown ) );
    Memset( MouseButtonDown, 0xff, sizeof( MouseButtonDown ) );
    Memset( JoystickButtonDown, 0xff, sizeof( JoystickButtonDown ) );

    MouseAxisStateX = MouseAxisStateY = 0;

    IntrusiveAddToList( this, Next, Prev, InputComponents, InputComponentsTail );
}

void FInputComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    IntrusiveRemoveFromList( this, Next, Prev, InputComponents, InputComponentsTail );

    InputMappings = nullptr;
}

void FInputComponent::SetInputMappings( FInputMappings * _InputMappings ) {
    InputMappings = _InputMappings;
}

FInputMappings * FInputComponent::GetInputMappings() {
    return InputMappings;
}

void FInputComponent::UpdateAxes( float _Fract, float _TimeStep ) {
    AN_Assert( _Fract > 0.0f );
    AN_Assert( _Fract <= 1.0f );

    if ( !InputMappings ) {
        return;
    }

    bool bPaused = GetWorld()->IsPaused();

    _Fract *= _TimeStep;

    for ( FAxisBinding & binding : AxisBindings ) {
        binding.AxisScale = 0.0f;
    }

    for ( FPressedKey * key = PressedKeys ; key < &PressedKeys[ NumPressedKeys ] ; key++ ) {
        if ( key->AxisBinding != -1 ) {
            AxisBindings[ key->AxisBinding ].AxisScale += key->AxisScale * _Fract;
        } else {
            // Key is pressed, but has no binding
        }
    }

    const TPodArray< FInputAxis * > & inputAxes = InputMappings->GetAxes();
    for ( int i = 0 ; i < inputAxes.Size() ; i++ ) {
        FInputAxis * inputAxis = inputAxes[i];

        int axisBinding = GetAxisBinding( inputAxis );
        if ( axisBinding == -1 ) {
            // axis is not binded
            continue;
        }

        FAxisBinding & binding = AxisBindings[ axisBinding ];
        if ( bPaused && !binding.bExecuteEvenWhenPaused ) {
            continue;
        }

        if ( !bIgnoreJoystickEvents ) {
            for ( int joyNum = 0 ; joyNum < MAX_JOYSTICKS_COUNT ; joyNum++ ) {
                if ( !Static.Joysticks[joyNum].bConnected ) {
                    continue;
                }

                const uint32_t joystickAxes = inputAxis->GetJoystickAxes( joyNum );
                for ( int joystickAxis = 0 ; joystickAxis < MAX_JOYSTICK_AXES ; joystickAxis++ ) {

                    if ( joystickAxes & ( 1 << joystickAxis ) ) {
                        FInputMappings::FMapping & mapping = InputMappings->JoystickAxisMappings[ joyNum ][ joystickAxis ];

                        AN_Assert( mapping.AxisOrActionIndex == i );

                        if ( mapping.ControllerId == ControllerId ) {
                            binding.AxisScale += Static.JoystickAxisState[ joyNum ][ joystickAxis ] * mapping.AxisScale * _Fract;
                        }
                    }
                }
            }
        }

        if ( !bIgnoreMouseEvents ) {
            const byte mouseAxes = inputAxis->GetMouseAxes();
            for ( int mouseAxis = 0 ; mouseAxis < MAX_MOUSE_AXES ; mouseAxis++ ) {
                if ( mouseAxes & ( 1 << mouseAxis ) ) {
                    FInputMappings::FMapping & mapping = InputMappings->MouseAxisMappings[ mouseAxis ];

                    AN_Assert( mapping.AxisOrActionIndex == i );

                    if ( mapping.ControllerId == ControllerId ) {
                        binding.AxisScale += (&MouseAxisStateX)[ mouseAxis ] * mapping.AxisScale;
                    }
                }
            }
        }

        //if ( binding.AxisScale != 0.0f ) {
            binding.Callback( binding.AxisScale );
        //}
    }
}

void FInputComponent::SetButtonState( int _DevId, int _Button, int _Action, int _ModMask, double _TimeStamp ) {
    AN_Assert( _DevId >= 0 && _DevId < MAX_INPUT_DEVICES );

    char * ButtonIndex = DeviceButtonDown[ _DevId ];

#ifdef AN_DEBUG
    switch ( _DevId ) {
    case ID_KEYBOARD:
        AN_Assert( _Button < MAX_KEYBOARD_BUTTONS );
        break;
    case ID_MOUSE:
        AN_Assert( _Button < MAX_MOUSE_BUTTONS );
        break;
    default:
        AN_Assert( _Button < MAX_JOYSTICK_BUTTONS );
        break;
    }
#endif

    if ( _Action == IE_Press ) {
        if ( ButtonIndex[ _Button ] == -1 ) {
            if ( NumPressedKeys < MAX_PRESSED_KEYS ) {
                FPressedKey & pressedKey = PressedKeys[ NumPressedKeys ];
                pressedKey.DevId = _DevId;
                pressedKey.Key = _Button;
                pressedKey.AxisBinding = -1;
                pressedKey.ActionBinding = -1;
                pressedKey.AxisScale = 0;

                if ( InputMappings ) {
                    const FInputMappings::FMapping * mapping;
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

                            FInputAxis * inputAxis = InputMappings->GetAxes()[ mapping->AxisOrActionIndex ];

                            pressedKey.AxisBinding = GetAxisBinding( inputAxis );

                        } else {
                            if ( ( _ModMask & mapping->ModMask ) == mapping->ModMask ) {
                                FInputAction * inputAction = InputMappings->GetActions()[ mapping->AxisOrActionIndex ];
                                pressedKey.ActionBinding = GetActionBinding( inputAction );
                            }
                        }
                    }
                }

                ButtonIndex[ _Button ] = NumPressedKeys;

                NumPressedKeys++;

                if ( pressedKey.ActionBinding != -1 ) {

                    FActionBinding & binding = ActionBindings[ pressedKey.ActionBinding ];

                    if ( GetWorld()->IsPaused() && !binding.bExecuteEvenWhenPaused ) {
                        pressedKey.ActionBinding = -1;
                    } else {
                        binding.Callback[ IE_Press ]();
                    }
                }

            } else {
                GLogger.Printf( "MAX_PRESSED_KEYS hit\n" );
            }
        } else {

            // Button is repressed

            //AN_Assert( 0 );

        }
    } else if ( _Action == IE_Release ) {
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

            if ( actionBinding != -1 /*&& !PressedKeys[ index ].bMarkedReleased*/ ) {
                ActionBindings[ actionBinding ].Callback[ IE_Release ]();
            }
        }
    }
}

bool FInputComponent::GetButtonState( int _DevId, int _Button ) const {
    AN_Assert( _DevId >= 0 && _DevId < MAX_INPUT_DEVICES );

    return DeviceButtonDown[ _DevId ][ _Button ] != -1;
}

bool FInputComponent::IsJoyDown( const FJoystick * _Joystick, int _Button ) const {
    return GetButtonState( ID_JOYSTICK_1 + _Joystick->Id, _Button );
}

void FInputComponent::NotifyUnicodeCharacter( FWideChar _UnicodeCharacter, int _ModMask, double _TimeStamp ) {
    if ( !CharacterCallback.IsValid() ) {
        return;
    }

    if ( GetWorld()->IsPaused() && !bCharacterCallbackExecuteEvenWhenPaused ) {
        return;
    }

    CharacterCallback( _UnicodeCharacter, _ModMask, _TimeStamp );
}

void FInputComponent::SetMouseAxisState( float _X, float _Y ) {
    MouseAxisStateX = _X;
    MouseAxisStateY = _Y;
}

float FInputComponent::GetMouseAxisState( int _Axis ) {
    return (&MouseAxisStateX)[_Axis];
}

void FInputComponent::SetJoystickState( int _Joystick, int _NumAxes, int _NumButtons, bool _bGamePad, bool _bConnected ) {
    Static.Joysticks[_Joystick].NumAxes = _NumAxes;
    Static.Joysticks[_Joystick].NumButtons = _NumButtons;
    Static.Joysticks[_Joystick].bGamePad = _bGamePad;
    Static.Joysticks[_Joystick].bConnected = _bConnected;
}

void FInputComponent::SetJoystickButtonState( int _Joystick, int _Button, int _Action, double _TimeStamp ) {
    IntrusiveForEach( component, InputComponents, Next ) {
        if ( !component->bIgnoreJoystickEvents ) {
            component->SetButtonState( ID_JOYSTICK_1 + _Joystick, _Button, _Action, 0, _TimeStamp );
        }
    }
}

void FInputComponent::SetJoystickAxisState( int _Joystick, int _Axis, float _Value ) {
    Static.JoystickAxisState[_Joystick][_Axis] = _Value;
}

float FInputComponent::GetJoystickAxisState( int _Joystick, int _Axis ) {
    return Static.JoystickAxisState[_Joystick][_Axis];
}

const FJoystick * FInputComponent::GetJoysticks() {
    return Static.Joysticks;
}

void FInputComponent::BindAxis( const char * _Axis, TCallback< void( float ) > const & _Callback, bool _ExecuteEvenWhenPaused ) {
    int hash = FCore::HashCase( _Axis, FString::Length( _Axis ) );

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
    FAxisBinding binding;
    binding.Name = _Axis;
    binding.Callback = _Callback;
    binding.AxisScale = 0.0f;
    binding.bExecuteEvenWhenPaused = _ExecuteEvenWhenPaused;
    AxisBindings.push_back( binding );
}

void FInputComponent::UnbindAxis( const char * _Axis ) {
    int hash = FCore::HashCase( _Axis, FString::Length( _Axis ) );

    for ( int i = AxisBindingsHash.First( hash ) ; i != -1 ; i = AxisBindingsHash.Next( i ) ) {
        if ( !AxisBindings[i].Name.Icmp( _Axis ) ) {
            AxisBindingsHash.RemoveIndex( hash, i );
            auto it = AxisBindings.begin() + i;
            AxisBindings.erase( it );

            for ( FPressedKey * pressedKey = PressedKeys ; pressedKey < &PressedKeys[ NumPressedKeys ] ; pressedKey++ ) {
                if ( pressedKey->AxisBinding == i ) {
                    pressedKey->AxisBinding = -1;
                }
            }

            return;
        }
    }
}

void FInputComponent::BindAction( const char * _Action, int _Event, TCallback< void() > const & _Callback, bool _ExecuteEvenWhenPaused ) {
    if ( _Event != IE_Press && _Event != IE_Release ) {
        GLogger.Printf( "FInputComponent::BindAction: expected IE_Press or IE_Release event\n" );
        return;
    }

    int hash = FCore::HashCase( _Action, FString::Length( _Action ) );

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
    FActionBinding binding;
    binding.Name = _Action;
    binding.Callback[ _Event ] = _Callback;
    binding.bExecuteEvenWhenPaused = _ExecuteEvenWhenPaused;
    ActionBindings.push_back( binding );
}

void FInputComponent::UnbindAction( const char * _Action ) {
    int hash = FCore::HashCase( _Action, FString::Length( _Action ) );

    for ( int i = ActionBindingsHash.First( hash ) ; i != -1 ; i = ActionBindingsHash.Next( i ) ) {
        if ( !ActionBindings[i].Name.Icmp( _Action ) ) {
            ActionBindingsHash.RemoveIndex( hash, i );
            auto it = ActionBindings.begin() + i;
            ActionBindings.erase( it );

            for ( FPressedKey * pressedKey = PressedKeys ; pressedKey < &PressedKeys[ NumPressedKeys ] ; pressedKey++ ) {
                if ( pressedKey->ActionBinding == i ) {
                    pressedKey->ActionBinding = -1;
                }
            }

            return;
        }
    }
}

void FInputComponent::UnbindAll() {
    AxisBindingsHash.Clear();
    AxisBindings.clear();

    ActionBindingsHash.Clear();
    ActionBindings.clear();

    for ( FPressedKey * pressedKey = PressedKeys ; pressedKey < &PressedKeys[ NumPressedKeys ] ; pressedKey++ ) {
        pressedKey->AxisBinding = -1;
        pressedKey->ActionBinding = -1;
    }
}

void FInputComponent::SetCharacterCallback( TCallback< void( FWideChar, int, double ) > const & _Callback, bool _ExecuteEvenWhenPaused ) {
    CharacterCallback = _Callback;
    bCharacterCallbackExecuteEvenWhenPaused = _ExecuteEvenWhenPaused;
}

void FInputComponent::UnsetCharacterCallback() {
    CharacterCallback.Clear();
}

int FInputComponent::GetAxisBinding( const char * _Axis ) const {
    return GetAxisBindingHash( _Axis, FCore::HashCase( _Axis, FString::Length( _Axis ) ) );
}

int FInputComponent::GetAxisBinding( const FInputAxis * _Axis ) const {
    return GetAxisBindingHash( _Axis->GetName().ToConstChar(), _Axis->GetNameHash() );
}

int FInputComponent::GetAxisBindingHash( const char * _Axis, int _Hash ) const {
    for ( int i = AxisBindingsHash.First( _Hash ) ; i != -1 ; i = AxisBindingsHash.Next( i ) ) {
        if ( !AxisBindings[i].Name.Icmp( _Axis ) ) {
            return i;
        }
    }
    return -1;
}

int FInputComponent::GetActionBinding( const char * _Action ) const {
    return GetActionBindingHash( _Action, FCore::HashCase( _Action, FString::Length( _Action ) ) );
}

int FInputComponent::GetActionBinding( const FInputAction * _Action ) const {
    return GetActionBindingHash( _Action->GetName().ToConstChar(), _Action->GetNameHash() );
}

int FInputComponent::GetActionBindingHash( const char * _Action, int _Hash ) const {
    for ( int i = ActionBindingsHash.First( _Hash ) ; i != -1 ; i = ActionBindingsHash.Next( i ) ) {
        if ( !ActionBindings[i].Name.Icmp( _Action ) ) {
            return i;
        }
    }
    return -1;
}

FInputMappings::FInputMappings() {
    Memset( KeyboardMappings, 0xff, sizeof( KeyboardMappings ) );
    Memset( MouseMappings, 0xff, sizeof( MouseMappings ) );
    Memset( MouseAxisMappings, 0xff, sizeof( MouseAxisMappings ) );
    Memset( JoystickMappings, 0xff, sizeof( JoystickMappings ) );
    Memset( JoystickAxisMappings, 0xff, sizeof( JoystickAxisMappings ) );
}

FInputMappings::~FInputMappings() {
    for ( int i = 0 ; i < Axes.Size() ; i++ ) {
        Axes[i]->RemoveRef();
    }
    for ( int i = 0 ; i < Actions.Size() ; i++ ) {
        Actions[i]->RemoveRef();
    }
}

int FInputMappings::Serialize( FDocument & _Doc ) {
    int object = Super::Serialize( _Doc );
    if ( !Axes.IsEmpty() ) {
        int axes = _Doc.AddArray( object, "Axes" );

        for ( FInputAxis const * axis : Axes ) {
            FString & axisName = _Doc.ProxyBuffer.NewString( axis->GetName() );

            for ( int deviceId = 0 ; deviceId < MAX_INPUT_DEVICES ; deviceId++ ) {
                if ( !axis->MappedKeys[ deviceId ].IsEmpty() ) {
                    const char * deviceName = FInputHelper::TranslateDevice( deviceId );
                    for ( unsigned short key : axis->MappedKeys[ deviceId ] ) {
                        FMapping * mapping = GetMapping( deviceId, key );
                        int axisObject = _Doc.CreateObjectValue();
                        _Doc.AddStringField( axisObject, "Name", axisName.ToConstChar() );
                        _Doc.AddStringField( axisObject, "Device", deviceName );
                        _Doc.AddStringField( axisObject, "Key", FInputHelper::TranslateDeviceKey( deviceId, key ) );
                        _Doc.AddStringField( axisObject, "Scale", _Doc.ProxyBuffer.NewString( FMath::ToString( mapping->AxisScale ) ).ToConstChar() );
                        _Doc.AddStringField( axisObject, "Owner", FInputHelper::TranslateController( mapping->ControllerId ) );
                        _Doc.AddValueToField( axes, axisObject );
                    }
                }
            }

            if ( axis->MappedMouseAxes ) {
                const char * deviceName = FInputHelper::TranslateDevice( ID_MOUSE );
                for ( int i = 0 ; i < MAX_MOUSE_AXES ; i++ ) {
                    if ( axis->MappedMouseAxes & ( 1 << i ) ) {
                        FMapping * mapping = GetMapping( ID_MOUSE, MOUSE_AXIS_BASE + i );
                        int axisObject = _Doc.CreateObjectValue();
                        _Doc.AddStringField( axisObject, "Name", axisName.ToConstChar() );
                        _Doc.AddStringField( axisObject, "Device", deviceName );
                        _Doc.AddStringField( axisObject, "Key", FInputHelper::TranslateDeviceKey( ID_MOUSE, MOUSE_AXIS_BASE + i ) );
                        _Doc.AddStringField( axisObject, "Scale", _Doc.ProxyBuffer.NewString( FMath::ToString( mapping->AxisScale ) ).ToConstChar() );
                        _Doc.AddStringField( axisObject, "Owner", FInputHelper::TranslateController( mapping->ControllerId ) );
                        _Doc.AddValueToField( axes, axisObject );
                    }
                }
            }

            for ( int joyId = 0 ; joyId < MAX_JOYSTICKS_COUNT ; joyId++ ) {
                if ( axis->MappedJoystickAxes[ joyId ] ) {
                    const char * deviceName = FInputHelper::TranslateDevice( ID_JOYSTICK_1 + joyId );
                    for ( int i = 0 ; i < MAX_JOYSTICK_AXES ; i++ ) {
                        if ( axis->MappedJoystickAxes[ joyId ] & ( 1 << i ) ) {
                            FMapping * mapping = GetMapping( ID_JOYSTICK_1 + joyId, JOY_AXIS_BASE + i );
                            int axisObject = _Doc.CreateObjectValue();
                            _Doc.AddStringField( axisObject, "Name", axisName.ToConstChar() );
                            _Doc.AddStringField( axisObject, "Device", deviceName );
                            _Doc.AddStringField( axisObject, "Key", FInputHelper::TranslateDeviceKey( ID_JOYSTICK_1 + joyId, JOY_AXIS_BASE + i ) );
                            _Doc.AddStringField( axisObject, "Scale", _Doc.ProxyBuffer.NewString( FMath::ToString( mapping->AxisScale ) ).ToConstChar() );
                            _Doc.AddStringField( axisObject, "Owner", FInputHelper::TranslateController( mapping->ControllerId ) );
                            _Doc.AddValueToField( axes, axisObject );
                        }
                    }
                }
            }
        }
    }

    if ( !Actions.IsEmpty() ) {
        int actions = _Doc.AddArray( object, "Actions" );

        for ( FInputAction const * action : Actions ) {

            FString & actionName = _Doc.ProxyBuffer.NewString( action->GetName() );

            for ( int deviceId = 0 ; deviceId < MAX_INPUT_DEVICES ; deviceId++ ) {
                if ( !action->MappedKeys[ deviceId ].IsEmpty() ) {
                    const char * deviceName = FInputHelper::TranslateDevice( deviceId );
                    for ( unsigned short key : action->MappedKeys[ deviceId ] ) {
                        FMapping * mapping = GetMapping( deviceId, key );
                        int actionObject = _Doc.CreateObjectValue();
                        _Doc.AddStringField( actionObject, "Name", actionName.ToConstChar() );
                        _Doc.AddStringField( actionObject, "Device", deviceName );
                        _Doc.AddStringField( actionObject, "Key", FInputHelper::TranslateDeviceKey( deviceId, key ) );
                        _Doc.AddStringField( actionObject, "Owner", FInputHelper::TranslateController( mapping->ControllerId ) );
                        if ( mapping->ModMask ) {
                            _Doc.AddStringField( actionObject, "ModMask", _Doc.ProxyBuffer.NewString( FMath::ToString( mapping->ModMask ) ).ToConstChar() );
                        }
                        _Doc.AddValueToField( actions, actionObject );
                    }
                }
            }
        }
    }

    return object;
}

FInputMappings * FInputMappings::LoadMappings( FDocument const & _Document, int _FieldsHead ) {
    FDocumentField * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "FInputMappings::LoadMappings: invalid class\n" );
        return nullptr;
    }

    FDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    FObjectFactory const * factory = FInputMappings::ClassMeta().Factory();

    FClassMeta const * classMeta = factory->LookupClass( classNameValue->Token.ToString().ToConstChar() );
    if ( !classMeta ) {
        GLogger.Printf( "FInputMappings::LoadMappings: invalid class \"%s\"\n", classNameValue->Token.ToString().ToConstChar() );
        return nullptr;
    }

    FInputMappings * inputMappings = static_cast< FInputMappings * >( classMeta->CreateInstance() );

    // Load attributes
    inputMappings->LoadAttributes( _Document, _FieldsHead );

    // Load axes
    FDocumentField * axesArray = _Document.FindField( _FieldsHead, "Axes" );
    if ( axesArray ) {
        inputMappings->LoadAxes( _Document, ( size_t )( axesArray - &_Document.Fields[0] ) );
    }

    // Load actions
    FDocumentField * actionsArray = _Document.FindField( _FieldsHead, "Actions" );
    if ( actionsArray ) {
        inputMappings->LoadActions( _Document, ( size_t )( actionsArray - &_Document.Fields[0] ) );
    }

    return inputMappings;
}

void FInputMappings::LoadAxes( FDocument const & _Document, int _FieldsHead ) {
    FDocumentField * field = &_Document.Fields[ _FieldsHead ];
    for ( int i = field->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
        FDocumentValue * value = &_Document.Values[ i ];
        if ( value->Type != FDocumentValue::T_Object ) {
            continue;
        }

        FDocumentField * nameField = _Document.FindField( value->FieldsHead, "Name" );
        if ( !nameField ) {
            continue;
        }

        FDocumentField * deviceField = _Document.FindField( value->FieldsHead, "Device" );
        if ( !deviceField ) {
            continue;
        }

        FDocumentField * keyField = _Document.FindField( value->FieldsHead, "Key" );
        if ( !keyField ) {
            continue;
        }

        FDocumentField * scaleField = _Document.FindField( value->FieldsHead, "Scale" );
        if ( !scaleField ) {
            continue;
        }

        FDocumentField * ownerField = _Document.FindField( value->FieldsHead, "Owner" );
        if ( !ownerField ) {
            continue;
        }

        FToken const & name = _Document.Values[ nameField->ValuesHead ].Token;
        FToken const & device = _Document.Values[ deviceField->ValuesHead ].Token;
        FToken const & key = _Document.Values[ keyField->ValuesHead ].Token;
        FToken const & scale = _Document.Values[ scaleField->ValuesHead ].Token;
        FToken const & controller = _Document.Values[ ownerField->ValuesHead ].Token;

        int deviceId = FInputHelper::LookupDevice( device.ToString().ToConstChar() );
        int deviceKey = FInputHelper::LookupDeviceKey( deviceId, key.ToString().ToConstChar() );
        int controllerId = FInputHelper::LookupController( controller.ToString().ToConstChar() );

        float scaleValue = FMath::FromString( scale.ToString() );

        MapAxis( name.ToString().ToConstChar(),
                 deviceId,
                 deviceKey,
                 scaleValue,
                 controllerId );
    }
}

void FInputMappings::LoadActions( FDocument const & _Document, int _FieldsHead ) {
    FDocumentField * field = &_Document.Fields[ _FieldsHead ];
    for ( int i = field->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
        FDocumentValue * value = &_Document.Values[ i ];
        if ( value->Type != FDocumentValue::T_Object ) {
            continue;
        }

        FDocumentField * nameField = _Document.FindField( value->FieldsHead, "Name" );
        if ( !nameField ) {
            continue;
        }

        FDocumentField * deviceField = _Document.FindField( value->FieldsHead, "Device" );
        if ( !deviceField ) {
            continue;
        }

        FDocumentField * keyField = _Document.FindField( value->FieldsHead, "Key" );
        if ( !keyField ) {
            continue;
        }

        FDocumentField * ownerField = _Document.FindField( value->FieldsHead, "Owner" );
        if ( !ownerField ) {
            continue;
        }

        Int modMask = 0;
        FDocumentField * modMaskField = _Document.FindField( value->FieldsHead, "ModMask" );
        if ( modMaskField ) {
            modMask.FromString( _Document.Values[ nameField->ValuesHead ].Token.ToString() );
        }

        FToken const & name = _Document.Values[ nameField->ValuesHead ].Token;
        FToken const & device = _Document.Values[ deviceField->ValuesHead ].Token;
        FToken const & key = _Document.Values[ keyField->ValuesHead ].Token;
        FToken const & controller = _Document.Values[ ownerField->ValuesHead ].Token;

        int deviceId = FInputHelper::LookupDevice( device.ToString().ToConstChar() );
        int deviceKey = FInputHelper::LookupDeviceKey( deviceId, key.ToString().ToConstChar() );
        int controllerId = FInputHelper::LookupController( controller.ToString().ToConstChar() );

        MapAction( name.ToString().ToConstChar(),
                   deviceId,
                   deviceKey,
                   modMask,
                   controllerId );
    }
}

FInputAxis * FInputMappings::AddAxis( const char * _Name ) {
    FInputAxis * axis = NewObject< FInputAxis >();
    axis->AddRef();
    axis->Parent = this;
    axis->IndexInArrayOfAxes = Axes.Size();
    axis->Name = _Name;
    axis->NameHash = FCore::HashCase( _Name, axis->Name.Length() );
    Axes.Append( axis );
    return axis;
}

FInputAction * FInputMappings::AddAction( const char * _Name ) {
    FInputAction * action = NewObject< FInputAction >();
    action->AddRef();
    action->Parent = this;
    action->IndexInArrayOfActions = Actions.Size();
    action->Name = _Name;
    action->NameHash = FCore::HashCase( _Name, action->Name.Length() );
    Actions.Append( action );
    return action;
}

FInputAxis * FInputMappings::FindAxis( const char * _AxisName ) {
    for ( FInputAxis * axis : Axes ) {
        if ( !axis->Name.Icmp( _AxisName ) ) {
            return axis;
        }
    }
    return nullptr;
}

FInputAction * FInputMappings::FindAction( const char * _ActionName ) {
    for ( FInputAction * action : Actions ) {
        if ( !action->Name.Icmp( _ActionName ) ) {
            return action;
        }
    }
    return nullptr;
}

void FInputMappings::MapAxis( const char * _AxisName, int _DevId, int _KeyToken, float _AxisScale, int _ControllerId ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return;
    }

    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        return;
    }

    Unmap( _DevId, _KeyToken );

    FInputAxis * axis = FindAxis( _AxisName );

    axis = ( !axis ) ? AddAxis( _AxisName ) : axis;

    FInputMappings::FMapping * mapping;

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
        AN_Assert(0);
        return;
    }

    mapping->AxisOrActionIndex = axis->IndexInArrayOfAxes;
    mapping->bAxis = true;
    mapping->AxisScale = _AxisScale;
    mapping->ControllerId = _ControllerId;
}

void FInputMappings::MapAction( const char * _ActionName, int _DevId, int _KeyToken, int _ModMask, int _ControllerId ) {
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

    FInputAction * action = FindAction( _ActionName );

    action = ( !action ) ? AddAction( _ActionName ) : action;

    FInputMappings::FMapping * mapping;

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
        AN_Assert(0);
        return;
    }

    action->MappedKeys[_DevId].Append( _KeyToken );

    mapping->AxisOrActionIndex = action->IndexInArrayOfActions;
    mapping->bAxis = false;
    mapping->AxisScale = 0;
    mapping->ControllerId = _ControllerId;
    mapping->ModMask = _ModMask & 0xff;
}

void FInputMappings::Unmap( int _DevId, int _KeyToken ) {
    if ( _DevId < 0 || _DevId >= MAX_INPUT_DEVICES ) {
        return;
    }

    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        return;
    }

    FInputMappings::FMapping * mapping;

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
        AN_Assert(0);
        return;
    }

    if ( mapping->AxisOrActionIndex == -1 ) {
        return;
    }

    if ( mapping->bAxis ) {
        FInputAxis * axis = Axes[ mapping->AxisOrActionIndex ];

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
        FInputAction * action = Actions[ mapping->AxisOrActionIndex ];

        for ( auto it = action->MappedKeys[_DevId].begin() ; it != action->MappedKeys[_DevId].end() ; it++ ) {
            if ( *it == _KeyToken ) {
                action->MappedKeys[_DevId].Erase( it );
                break;
            }
        }
    }

    mapping->AxisOrActionIndex = -1;
}

void FInputMappings::UnmapAll() {
    for ( int i = 0 ; i < Axes.Size() ; i++ ) {
        Axes[i]->RemoveRef();
    }
    for ( int i = 0 ; i < Actions.Size() ; i++ ) {
        Actions[i]->RemoveRef();
    }
    Axes.Clear();
    Actions.Clear();
    Memset( KeyboardMappings, 0xff, sizeof( KeyboardMappings ) );
    Memset( MouseMappings, 0xff, sizeof( MouseMappings ) );
    Memset( MouseAxisMappings, 0xff, sizeof( MouseAxisMappings ) );
    Memset( JoystickMappings, 0xff, sizeof( JoystickMappings ) );
    Memset( JoystickAxisMappings, 0xff, sizeof( JoystickAxisMappings ) );
}

FInputMappings::FMapping * FInputMappings::GetMapping( int _DevId, int _KeyToken ) {
    if ( _KeyToken < 0 || _KeyToken >= Static.DeviceButtonLimits[ _DevId ] ) {
        AN_Assert( 0 );
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

    AN_Assert(0);

    return nullptr;
}

FInputAxis::FInputAxis() {
    MappedMouseAxes = 0;
    ZeroMem( MappedJoystickAxes,  sizeof( MappedJoystickAxes ) );
}

void FInputAxis::Map( int _DevId, int _KeyToken, float _AxisScale, int _ControllerId ) {
    Parent->MapAxis( Name.ToConstChar(), _DevId, _KeyToken, _AxisScale, _ControllerId );
}

void FInputAction::Map( int _DevId, int _KeyToken, int _ModMask, int _ControllerId ) {
    Parent->MapAction( Name.ToConstChar(), _DevId, _KeyToken, _ModMask, _ControllerId );
}
