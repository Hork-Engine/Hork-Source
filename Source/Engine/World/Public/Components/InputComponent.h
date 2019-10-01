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

#pragma once

#include <Engine/World/Public/Components/ActorComponent.h>

#include <Engine/Runtime/Public/InputDefs.h>
#include <Engine/Core/Public/Utf8.h>

#define MAX_PRESSED_KEYS       128
#define MAX_AXIS_BINDINGS      1024
#define MAX_ACTION_BINDINGS    1024
#define MAX_INPUT_CONTROLLERS  16

enum {
    /* Input devices */
    ID_KEYBOARD         = 0,
    ID_MOUSE            = 1,
    ID_JOYSTICK_1       = 2,
    ID_JOYSTICK_2       = 3,
    ID_JOYSTICK_3       = 4,
    ID_JOYSTICK_4       = 5,
    ID_JOYSTICK_5       = 6,
    ID_JOYSTICK_6       = 7,
    ID_JOYSTICK_7       = 8,
    ID_JOYSTICK_8       = 9,
    ID_JOYSTICK_9       = 10,
    ID_JOYSTICK_10      = 11,
    ID_JOYSTICK_11      = 12,
    ID_JOYSTICK_12      = 13,
    ID_JOYSTICK_13      = 14,
    ID_JOYSTICK_14      = 15,
    ID_JOYSTICK_15      = 16,
    ID_JOYSTICK_16      = 17,
    MAX_INPUT_DEVICES   = 18,

    /* Player controllers */
    CONTROLLER_PLAYER_1        = 0,
    CONTROLLER_PLAYER_2        = 1,
    CONTROLLER_PLAYER_3        = 2,
    CONTROLLER_PLAYER_4        = 3,
    CONTROLLER_PLAYER_5        = 4,
    CONTROLLER_PLAYER_6        = 5,
    CONTROLLER_PLAYER_7        = 6,
    CONTROLLER_PLAYER_8        = 7,
    CONTROLLER_PLAYER_9        = 8,
    CONTROLLER_PLAYER_10       = 9,
    CONTROLLER_PLAYER_11       = 10,
    CONTROLLER_PLAYER_12       = 11,
    CONTROLLER_PLAYER_13       = 12,
    CONTROLLER_PLAYER_14       = 13,
    CONTROLLER_PLAYER_15       = 14,
    CONTROLLER_PLAYER_16       = 15
};

class FInputAxis;
class FInputAction;
class FInputMappings;
class FInputComponent;

class ANGIE_API FInputAxis final : public FBaseObject {
    AN_CLASS( FInputAxis, FBaseObject )

    friend class FInputMappings;
public:
    int GetNameHash() const { return NameHash; }

    void Map( int _DevId, int _KeyToken, float _AxisScale, int _ControllerId );

    byte GetMouseAxes() const { return MappedMouseAxes; }
    uint32_t GetJoystickAxes( int _Joystick ) const { return MappedJoystickAxes[ _Joystick ]; }

private:
    FInputAxis();

    int NameHash;

    FInputMappings * Parent;

    // Keys mapped to this axis
    TPodArray< unsigned short, 8 > MappedKeys[MAX_INPUT_DEVICES];

    // Mouse axes mapped to this axis
    byte MappedMouseAxes;

    // Joystick axes mapped to this axis
    uint32_t MappedJoystickAxes[MAX_JOYSTICKS_COUNT];

    int IndexInArrayOfAxes;
};

class ANGIE_API FInputAction final : public FBaseObject {
    AN_CLASS( FInputAction, FBaseObject )

    friend class FInputMappings;
public:
    int GetNameHash() const { return NameHash; }

    void Map( int _DevId, int _KeyToken, int _ModMask, int _ControllerId );

private:
    FInputAction() {}

    int NameHash;

    FInputMappings * Parent;

    // Keys mapped to this action
    TPodArray< unsigned short, 8 > MappedKeys[MAX_INPUT_DEVICES];

    int IndexInArrayOfActions;
};

class ANGIE_API FInputMappings final : public FBaseObject {
    AN_CLASS( FInputMappings, FBaseObject )

    friend class FInputAxis;
    friend class FInputAction;
    friend class FInputComponent;

public:
    int Serialize( FDocument & _Doc ) override;

    static FInputMappings * LoadMappings( FDocument const & _Document, int _FieldsHead );

    // Load axes form document data
    void LoadAxes( FDocument const & _Document, int _FieldsHead );

    // Load actions form document data
    void LoadActions( FDocument const & _Document, int _FieldsHead );

    FInputAxis * AddAxis( const char * _Name );
    FInputAction * AddAction( const char * _Name );

    FInputAxis * FindAxis( const char * _AxisName );
    FInputAction * FindAction( const char * _ActionName );

    void MapAxis( const char * _AxisName, int _DevId, int _KeyToken, float _AxisScale, int _ControllerId );
    void MapAction( const char * _ActionName, int _DevId, int _KeyToken, int _ModMask, int _ControllerId );
    void Unmap( int _DevId, int _KeyToken );
    void UnmapAll();

    const TPodArray< FInputAxis * > & GetAxes() const { return Axes; }
    const TPodArray< FInputAction * > & GetActions() const { return Actions; }

private:
    FInputMappings();
    ~FInputMappings();

    struct FMapping {
        int AxisOrActionIndex;      // -1 if not mapped
        float AxisScale;
        byte ControllerId;
        bool bAxis;
        byte ModMask;
    };

    FMapping * GetMapping( int _DevId, int _KeyToken );

    // All known axes
    TPodArray< FInputAxis * > Axes;

    // All known actions
    TPodArray< FInputAction * > Actions;

    FMapping KeyboardMappings[ MAX_KEYBOARD_BUTTONS ];
    FMapping MouseMappings[ MAX_MOUSE_BUTTONS ];
    FMapping MouseAxisMappings[ MAX_MOUSE_AXES ];
    FMapping JoystickMappings[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];
    FMapping JoystickAxisMappings[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];
};

class ANGIE_API FInputComponent final : public FActorComponent {
    AN_COMPONENT( FInputComponent, FActorComponent )

public:

    // Set displays to receive input
    //int ReceiveInputMask = RI_ALL_DISPLAYS;

    // Filter keyboard events
    bool bIgnoreKeyboardEvents;

    // Filter mouse events
    bool bIgnoreMouseEvents;

    // Filter joystick events
    bool bIgnoreJoystickEvents;

    // Filter character events
    bool bIgnoreCharEvents;

    bool bActive = true;

    int ControllerId;

    // Set input mappings config
    void SetInputMappings( FInputMappings * _InputMappings );

    // Get input mappints config
    FInputMappings * GetInputMappings();

    // Bind axis to class method
    template< typename T >
    void BindAxis( const char * _Axis, T * _Object, void (T::*_Method)( float ), bool _ExecuteEvenWhenPaused = false ) {
        BindAxis( _Axis, { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    // Bind axis to function
    void BindAxis( const char * _Axis, TCallback< void( float ) > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    // Unbind axis
    void UnbindAxis( const char * _Axis );

    // Bind action to class method
    template< typename T >
    void BindAction( const char * _Action, int _Event, T * _Object, void (T::*_Method)(), bool _ExecuteEvenWhenPaused = false ) {
        BindAction( _Action, _Event, { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    // Bind action to function
    void BindAction( const char * _Action, int _Event, TCallback< void() > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    // Unbind action
    void UnbindAction( const char * _Action );

    // Unbind all
    void UnbindAll();

    // Set callback for input characters
    template< typename T >
    void SetCharacterCallback( T * _Object, void (T::*_Method)( FWideChar, int, double ), bool _ExecuteEvenWhenPaused = false ) {
        SetCharacterCallback( { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    void SetCharacterCallback( TCallback< void( FWideChar, int, double ) > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    void UnsetCharacterCallback();

    void UpdateAxes( float _Fract, float _TimeStep );

    bool IsKeyDown( int _Key ) const { return GetButtonState( ID_KEYBOARD, _Key ); }
    bool IsMouseDown( int _Button ) const { return GetButtonState( ID_MOUSE, _Button ); }
    bool IsJoyDown( const struct FJoystick * _Joystick, int _Button ) const;

    // Used by GameEngine during main game tick to notfiy input component for button press/release
    void SetButtonState( int _DevId, int _Button, int _Action, int _ModMask, double _TimeStamp );

    // Return is button pressed or not
    bool GetButtonState( int _DevId, int _Button ) const;

    void SetMouseAxisState( float _X, float _Y );

    float GetMouseMoveX() const { return MouseAxisStateX; }
    float GetMouseMoveY() const { return MouseAxisStateY; }

    float GetMouseAxisState( int _Axis );

    void NotifyUnicodeCharacter( FWideChar _UnicodeCharacter, int _ModMask, double _TimeStamp );

    FInputComponent * GetNext() { return Next; }
    FInputComponent * GetPrev() { return Prev; }

    // Used by GameEngine during main game tick to update joystick state
    static void SetJoystickState( int _Joystick, int _NumAxes, int _NumButtons, bool _bGamePad, bool _bConnected );

    // Used by GameEngine during main game tick to notfiy input component for button press/release
    static void SetJoystickButtonState( int _Joystick, int _Button, int _Action, double _TimeStamp );

    // Used by GameEngine during main game tick to update joystick axis state
    static void SetJoystickAxisState( int _Joystick, int _Axis, float _Value );

    static float GetJoystickAxisState( int _Joystick, int _Axis );

    static struct FJoystick const * GetJoysticks();

    static FInputComponent * GetInputComponents() { return InputComponents; }

protected:
    struct FAxisBinding {
        FString Name;                           // axis name
        TCallback< void( float ) > Callback;    // binding callback
        float AxisScale;                        // final axis value that will be passed to binding callback
        bool bExecuteEvenWhenPaused;
    };

    struct FActionBinding {
        FString Name;                           // action name
        TCallback< void() > Callback[2];        // binding callback
        bool bExecuteEvenWhenPaused;
    };

    struct FPressedKey {
        unsigned short Key;
        short AxisBinding;
        short ActionBinding;
        float AxisScale;
        byte DevId;
    };

    FInputComponent();

    void DeinitializeComponent() override;

    // Return axis binding or -1 if axis is not binded
    int GetAxisBinding( const char * _Axis ) const;

    // Return axis binding or -1 if axis is not binded
    int GetAxisBinding( const FInputAxis * _Axis ) const;

    // Return axis binding or -1 if axis is not binded
    int GetAxisBindingHash( const char * _Axis, int _Hash ) const;

    // Return action binding or -1 if action is not binded
    int GetActionBinding( const char * _Action ) const;

    // Return action binding or -1 if action is not binded
    int GetActionBinding( const FInputAction * _Action ) const;

    // Return action binding or -1 if action is not binded
    int GetActionBindingHash( const char * _Action, int _Hash ) const;

    TRef< FInputMappings > InputMappings;

    THash<> AxisBindingsHash;
    TVector< FAxisBinding > AxisBindings;

    THash<> ActionBindingsHash;
    TVector< FActionBinding > ActionBindings;

    // Array of pressed keys
    FPressedKey PressedKeys[ MAX_PRESSED_KEYS ];
    int NumPressedKeys;

    // Index to PressedKeys array or -1 if button is up
    char * DeviceButtonDown[ MAX_INPUT_DEVICES ];
    char KeyboardButtonDown[ MAX_KEYBOARD_BUTTONS ];
    char MouseButtonDown[ MAX_MOUSE_BUTTONS ];
    char JoystickButtonDown[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];

    float MouseAxisStateX;
    float MouseAxisStateY;

    TCallback< void( FWideChar, int, double ) > CharacterCallback;
    bool bCharacterCallbackExecuteEvenWhenPaused;

    FInputComponent * Next;
    FInputComponent * Prev;

    static FInputComponent * InputComponents;
    static FInputComponent * InputComponentsTail;
};

class ANGIE_API FInputHelper final {
public:
    // Translate device to string
    static const char * TranslateDevice( int _DevId );

    // Translate modifier to string
    static const char * TranslateModifier( int _Modifier );

    // Translate key code to string
    static const char * TranslateDeviceKey( int _DevId, int _Key );

    // Translate key owner player to string
    static const char * TranslateController( int _ControllerId );

    // Lookup device from string
    static int LookupDevice( const char * _Device );

    // Lookup modifier from string
    static int LookupModifier( const char * _Modifier );

    // Lookup key code from string
    static int LookupDeviceKey( int _DevId, const char * _Key );

    // Lookup key owner player from string
    static int LookupController( const char * _ControllerId );
};
