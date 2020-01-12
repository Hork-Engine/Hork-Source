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

#include <World/Public/Components/ActorComponent.h>

#include <Runtime/Public/InputDefs.h>
#include <Core/Public/Utf8.h>

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

class AInputAxis;
class AInputAction;
class AInputMappings;
class AInputComponent;

class ANGIE_API AInputAxis final : public ABaseObject {
    AN_CLASS( AInputAxis, ABaseObject )

    friend class AInputMappings;
public:
    int GetNameHash() const { return NameHash; }

    void Map( int _DevId, int _KeyToken, float _AxisScale, int _ControllerId );

    uint8_t GetMouseAxes() const { return MappedMouseAxes; }
    uint32_t GetJoystickAxes( int _Joystick ) const { return MappedJoystickAxes[ _Joystick ]; }

private:
    AInputAxis();

    int NameHash;

    AInputMappings * Parent;

    /** Keys mapped to this axis */
    TPodArray< unsigned short, 8 > MappedKeys[MAX_INPUT_DEVICES];

    /** Mouse axes mapped to this axis */
    uint8_t MappedMouseAxes;

    /** Joystick axes mapped to this axis */
    uint32_t MappedJoystickAxes[MAX_JOYSTICKS_COUNT];

    int IndexInArrayOfAxes;
};

class ANGIE_API AInputAction final : public ABaseObject {
    AN_CLASS( AInputAction, ABaseObject )

    friend class AInputMappings;
public:
    int GetNameHash() const { return NameHash; }

    void Map( int _DevId, int _KeyToken, int _ModMask, int _ControllerId );

private:
    AInputAction() {}

    int NameHash;

    AInputMappings * Parent;

    /** Keys mapped to this action */
    TPodArray< unsigned short, 8 > MappedKeys[MAX_INPUT_DEVICES];

    int IndexInArrayOfActions;
};

class ANGIE_API AInputMappings final : public ABaseObject {
    AN_CLASS( AInputMappings, ABaseObject )

    friend class AInputAxis;
    friend class AInputAction;
    friend class AInputComponent;

public:
    int Serialize( ADocument & _Doc ) override;

    static AInputMappings * LoadMappings( ADocument const & _Document, int _FieldsHead );

    // Load axes form document data
    void LoadAxes( ADocument const & _Document, int _FieldsHead );

    // Load actions form document data
    void LoadActions( ADocument const & _Document, int _FieldsHead );

    AInputAxis * AddAxis( const char * _Name );
    AInputAction * AddAction( const char * _Name );

    AInputAxis * FindAxis( const char * _AxisName );
    AInputAction * FindAction( const char * _ActionName );

    void MapAxis( const char * _AxisName, int _DevId, int _KeyToken, float _AxisScale, int _ControllerId );
    void MapAction( const char * _ActionName, int _DevId, int _KeyToken, int _ModMask, int _ControllerId );
    void Unmap( int _DevId, int _KeyToken );
    void UnmapAll();

    const TPodArray< AInputAxis * > & GetAxes() const { return Axes; }
    const TPodArray< AInputAction * > & GetActions() const { return Actions; }

private:
    AInputMappings();
    ~AInputMappings();

    struct SMapping {
        int AxisOrActionIndex;      // -1 if not mapped
        float AxisScale;
        uint8_t ControllerId;
        bool bAxis;
        byte ModMask;
    };

    SMapping * GetMapping( int _DevId, int _KeyToken );

    /** All known axes */
    TPodArray< AInputAxis * > Axes;

    /** All known actions */
    TPodArray< AInputAction * > Actions;

    SMapping KeyboardMappings[ MAX_KEYBOARD_BUTTONS ];
    SMapping MouseMappings[ MAX_MOUSE_BUTTONS ];
    SMapping MouseAxisMappings[ MAX_MOUSE_AXES ];
    SMapping JoystickMappings[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];
    SMapping JoystickAxisMappings[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_AXES ];
};

class ANGIE_API AInputComponent final : public AActorComponent {
    AN_COMPONENT( AInputComponent, AActorComponent )

public:

    // Set displays to receive input
    //int ReceiveInputMask = RI_ALL_DISPLAYS;

    /** Filter keyboard events */
    bool bIgnoreKeyboardEvents;

    /** Filter mouse events */
    bool bIgnoreMouseEvents;

    /** Filter joystick events */
    bool bIgnoreJoystickEvents;

    /** Filter character events */
    bool bIgnoreCharEvents;

    bool bActive = true;

    int ControllerId;

    /** Set input mappings config */
    void SetInputMappings( AInputMappings * _InputMappings );

    /** Get input mappints config */
    AInputMappings * GetInputMappings();

    /** Bind axis to class method */
    template< typename T >
    void BindAxis( const char * _Axis, T * _Object, void (T::*_Method)( float ), bool _ExecuteEvenWhenPaused = false ) {
        BindAxis( _Axis, { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    /** Bind axis to function */
    void BindAxis( const char * _Axis, TCallback< void( float ) > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    /** Unbind axis */
    void UnbindAxis( const char * _Axis );

    /** Bind action to class method */
    template< typename T >
    void BindAction( const char * _Action, int _Event, T * _Object, void (T::*_Method)(), bool _ExecuteEvenWhenPaused = false ) {
        BindAction( _Action, _Event, { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    /** Bind action to function */
    void BindAction( const char * _Action, int _Event, TCallback< void() > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    /** Unbind action */
    void UnbindAction( const char * _Action );

    /** Unbind all */
    void UnbindAll();

    /** Set callback for input characters */
    template< typename T >
    void SetCharacterCallback( T * _Object, void (T::*_Method)( FWideChar, int, double ), bool _ExecuteEvenWhenPaused = false ) {
        SetCharacterCallback( { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    /** Set callback for input characters */
    void SetCharacterCallback( TCallback< void( FWideChar, int, double ) > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    void UnsetCharacterCallback();

    void UpdateAxes( float _Fract, float _TimeStep );

    bool IsKeyDown( int _Key ) const { return GetButtonState( ID_KEYBOARD, _Key ); }
    bool IsMouseDown( int _Button ) const { return GetButtonState( ID_MOUSE, _Button ); }
    bool IsJoyDown( const struct SJoystick * _Joystick, int _Button ) const;

    /** Used by EngineInstance during main game tick to notfiy input component for button press/release */
    void SetButtonState( int _DevId, int _Button, int _Action, int _ModMask, double _TimeStamp );

    /** Return is button pressed or not */
    bool GetButtonState( int _DevId, int _Button ) const;

    void SetMouseAxisState( float _X, float _Y );

    float GetMouseMoveX() const { return MouseAxisStateX; }
    float GetMouseMoveY() const { return MouseAxisStateY; }

    float GetMouseAxisState( int _Axis );

    void NotifyUnicodeCharacter( FWideChar _UnicodeCharacter, int _ModMask, double _TimeStamp );

    AInputComponent * GetNext() { return Next; }
    AInputComponent * GetPrev() { return Prev; }

    /** Set cursor position */
    static void SetCursorPosition( float _X, float _Y ) { CursorPosition.X = _X; CursorPosition.Y = _Y; }

    /** Get cursor position */
    static Float2 const & GetCursorPosition() { return CursorPosition; }

    /** Used by EngineInstance during main game tick to update joystick state */
    static void SetJoystickState( int _Joystick, int _NumAxes, int _NumButtons, bool _bGamePad, bool _bConnected );

    /** Used by EngineInstance during main game tick to notfiy input component for button press/release */
    static void SetJoystickButtonState( int _Joystick, int _Button, int _Action, double _TimeStamp );

    /** Used by EngineInstance during main game tick to update joystick axis state */
    static void SetJoystickAxisState( int _Joystick, int _Axis, float _Value );

    static float GetJoystickAxisState( int _Joystick, int _Axis );

    static struct SJoystick const * GetJoysticks();

    static AInputComponent * GetInputComponents() { return InputComponents; }

protected:
    struct SAxisBinding {
        AString Name;                           // axis name
        TCallback< void( float ) > Callback;    // binding callback
        float AxisScale;                        // final axis value that will be passed to binding callback
        bool bExecuteEvenWhenPaused;
    };

    struct SActionBinding {
        AString Name;                           // action name
        TCallback< void() > Callback[2];        // binding callback
        bool bExecuteEvenWhenPaused;
    };

    struct SPressedKey {
        unsigned short Key;
        short AxisBinding;
        short ActionBinding;
        float AxisScale;
        uint8_t DevId;
    };

    AInputComponent();

    void DeinitializeComponent() override;

    /** Return axis binding or -1 if axis is not binded */
    int GetAxisBinding( const char * _Axis ) const;

    /** Return axis binding or -1 if axis is not binded */
    int GetAxisBinding( const AInputAxis * _Axis ) const;

    /** Return axis binding or -1 if axis is not binded */
    int GetAxisBindingHash( const char * _Axis, int _Hash ) const;

    /** Return action binding or -1 if action is not binded */
    int GetActionBinding( const char * _Action ) const;

    /** Return action binding or -1 if action is not binded */
    int GetActionBinding( const AInputAction * _Action ) const;

    /** Return action binding or -1 if action is not binded */
    int GetActionBindingHash( const char * _Action, int _Hash ) const;

    TRef< AInputMappings > InputMappings;

    THash<> AxisBindingsHash;
    TStdVector< SAxisBinding > AxisBindings;

    THash<> ActionBindingsHash;
    TStdVector< SActionBinding > ActionBindings;

    // Array of pressed keys
    SPressedKey PressedKeys[ MAX_PRESSED_KEYS ];
    int NumPressedKeys;

    static Float2 CursorPosition;

    // Index to PressedKeys array or -1 if button is up
    char * DeviceButtonDown[ MAX_INPUT_DEVICES ];
    char KeyboardButtonDown[ MAX_KEYBOARD_BUTTONS ];
    char MouseButtonDown[ MAX_MOUSE_BUTTONS ];
    char JoystickButtonDown[ MAX_JOYSTICKS_COUNT ][ MAX_JOYSTICK_BUTTONS ];

    float MouseAxisStateX;
    float MouseAxisStateY;

    TCallback< void( FWideChar, int, double ) > CharacterCallback;
    bool bCharacterCallbackExecuteEvenWhenPaused;

    AInputComponent * Next;
    AInputComponent * Prev;

    static AInputComponent * InputComponents;
    static AInputComponent * InputComponentsTail;
};

class ANGIE_API AInputHelper final {
public:
    /** Translate device to string */
    static const char * TranslateDevice( int _DevId );

    /** Translate modifier to string */
    static const char * TranslateModifier( int _Modifier );

    /** Translate key code to string */
    static const char * TranslateDeviceKey( int _DevId, int _Key );

    /** Translate key owner player to string */
    static const char * TranslateController( int _ControllerId );

    /** Lookup device from string */
    static int LookupDevice( const char * _Device );

    /** Lookup modifier from string */
    static int LookupModifier( const char * _Modifier );

    /** Lookup key code from string */
    static int LookupDeviceKey( int _DevId, const char * _Key );

    /** Lookup key owner player from string */
    static int LookupController( const char * _ControllerId );
};
