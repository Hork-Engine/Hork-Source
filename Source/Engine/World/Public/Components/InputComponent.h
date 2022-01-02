/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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
#include <Runtime/Public/RuntimeVariable.h>

#include <Platform/Public/Utf8.h>
#include <Containers/Public/Array.h>

constexpr int MAX_PRESSED_KEYS      = 128;
constexpr int MAX_AXIS_BINDINGS     = 1024;
constexpr int MAX_ACTION_BINDINGS   = 1024;
constexpr int MAX_INPUT_CONTROLLERS = 16;

enum INPUT_DEVICE
{
    /* Input devices */
    ID_KEYBOARD       = 0,
    ID_MOUSE          = 1,
    ID_JOYSTICK_1     = 2,
    ID_JOYSTICK_2     = 3,
    ID_JOYSTICK_3     = 4,
    ID_JOYSTICK_4     = 5,
    ID_JOYSTICK_5     = 6,
    ID_JOYSTICK_6     = 7,
    ID_JOYSTICK_7     = 8,
    ID_JOYSTICK_8     = 9,
    ID_JOYSTICK_9     = 10,
    ID_JOYSTICK_10    = 11,
    ID_JOYSTICK_11    = 12,
    ID_JOYSTICK_12    = 13,
    ID_JOYSTICK_13    = 14,
    ID_JOYSTICK_14    = 15,
    ID_JOYSTICK_15    = 16,
    ID_JOYSTICK_16    = 17,
    MAX_INPUT_DEVICES = 18,
};

/* Player controllers */
enum CONTROLLER
{
    CONTROLLER_PLAYER_1  = 0,
    CONTROLLER_PLAYER_2  = 1,
    CONTROLLER_PLAYER_3  = 2,
    CONTROLLER_PLAYER_4  = 3,
    CONTROLLER_PLAYER_5  = 4,
    CONTROLLER_PLAYER_6  = 5,
    CONTROLLER_PLAYER_7  = 6,
    CONTROLLER_PLAYER_8  = 7,
    CONTROLLER_PLAYER_9  = 8,
    CONTROLLER_PLAYER_10 = 9,
    CONTROLLER_PLAYER_11 = 10,
    CONTROLLER_PLAYER_12 = 11,
    CONTROLLER_PLAYER_13 = 12,
    CONTROLLER_PLAYER_14 = 13,
    CONTROLLER_PLAYER_15 = 14,
    CONTROLLER_PLAYER_16 = 15
};

class AInputAxis;
class AInputAction;
class AInputMappings;
class AInputComponent;

class AInputAxis : public ABaseObject
{
    AN_CLASS( AInputAxis, ABaseObject )

    friend class AInputMappings;

public:
    AInputAxis();

    int GetNameHash() const { return NameHash; }

    void Map( int _DevId, int _KeyToken, float _AxisScale, int _ControllerId );

    //uint8_t GetMouseAxes() const { return MappedMouseAxes; }
    //uint32_t GetJoystickAxes( int _Joystick ) const { return MappedJoystickAxes[ _Joystick ]; }

private:
    int NameHash = 0;

    AInputMappings * Parent = nullptr;

    /** Keys mapped to this axis */
    //TPodVector< unsigned short, 8 > MappedKeys[MAX_INPUT_DEVICES];

    /** Mouse axes mapped to this axis */
    //uint8_t MappedMouseAxes = 0;

    /** Joystick axes mapped to this axis */
    //uint32_t MappedJoystickAxes[MAX_JOYSTICKS_COUNT];

    int IndexInArrayOfAxes = 0;
};

class AInputAction : public ABaseObject
{
    AN_CLASS( AInputAction, ABaseObject )

    friend class AInputMappings;

public:
    AInputAction() {}

    int GetNameHash() const { return NameHash; }

    void Map( int _DevId, int _KeyToken, int _ModMask, int _ControllerId );

private:
    int NameHash = 0;

    AInputMappings * Parent = nullptr;

    /** Keys mapped to this action */
    //TPodVector< unsigned short, 8 > MappedKeys[MAX_INPUT_DEVICES];

    int IndexInArrayOfActions = 0;
};

class AInputMappings : public ABaseObject
{
    AN_CLASS( AInputMappings, ABaseObject )

    friend class AInputAxis;
    friend class AInputAction;
    friend class AInputComponent;

public:
    AInputMappings();
    ~AInputMappings();

    TRef< ADocObject > Serialize() override;

    static AInputMappings * LoadMappings( ADocObject const * pObject );

    // Load axes form document data
    void LoadAxes( ADocMember const * ArrayOfAxes );

    // Load actions form document data
    void LoadActions( ADocMember const * ArrayOfActions );

    AInputAxis *   AddAxis( const char * _Name );
    AInputAction * AddAction( const char * _Name );

    AInputAxis *   FindAxis( const char * _AxisName );
    AInputAction * FindAction( const char * _ActionName );

    void MapAxis( const char * _AxisName, int _DevId, int _KeyToken, float _AxisScale, int _ControllerId );
    void UnmapAxis( int _DevId, int _KeyToken );

    void MapAction( const char * _ActionName, int _DevId, int _KeyToken, int _ModMask, int _ControllerId );
    void UnmapAction( int _DevId, int _KeyToken, int _ModMask );

    void UnmapAll();

    const TStdVector< TRef< AInputAxis > > &   GetAxes() const { return Axes; }
    const TStdVector< TRef< AInputAction > > & GetActions() const { return Actions; }

private:
    struct SMapping
    {
        int     AxisOrActionIndex;
        float   AxisScale;
        uint8_t ControllerId;
        bool    bAxis;
        byte    ModMask;
    };

    using AArrayOfMappings = TPodVector< SMapping >;

    AArrayOfMappings * GetKeyMappings( int _DevId, int _KeyToken );

    /** All known axes */
    TStdVector< TRef<AInputAxis> > Axes;

    /** All known actions */
    TStdVector< TRef<AInputAction> > Actions;

    TArray< AArrayOfMappings, MAX_KEYBOARD_BUTTONS > KeyboardMappings;
    TArray< AArrayOfMappings, MAX_MOUSE_BUTTONS >    MouseMappings;
    TArray< AArrayOfMappings, MAX_MOUSE_AXES >       MouseAxisMappings;

    struct SJoystickMappings
    {
        TArray< AArrayOfMappings, MAX_JOYSTICK_BUTTONS > ButtonMappings;
        TArray< AArrayOfMappings, MAX_JOYSTICK_AXES >    AxisMappings;
    };

    TArray< SJoystickMappings, MAX_JOYSTICKS_COUNT > JoystickMappings;
};

class AInputComponent : public AActorComponent
{
    AN_COMPONENT( AInputComponent, AActorComponent )

public:
    /** Filter keyboard events */
    bool bIgnoreKeyboardEvents = false;

    /** Filter mouse events */
    bool bIgnoreMouseEvents = false;

    /** Filter joystick events */
    bool bIgnoreJoystickEvents = false;

    /** Filter character events */
    bool bIgnoreCharEvents = false;

    int ControllerId = 0;

    /** Set input mappings config */
    void SetInputMappings( AInputMappings * _InputMappings );

    /** Get input mappints config */
    AInputMappings * GetInputMappings();

    /** Bind axis to class method */
    template< typename T >
    void BindAxis( const char * _Axis, T * _Object, void ( T::*_Method )( float ), bool _ExecuteEvenWhenPaused = false )
    {
        BindAxis( _Axis, { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    /** Bind axis to function */
    void BindAxis( const char * _Axis, TCallback< void( float ) > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    /** Unbind axis */
    void UnbindAxis( const char * _Axis );

    /** Bind action to class method */
    template< typename T >
    void BindAction( const char * _Action, int _Event, T * _Object, void ( T::*_Method )(), bool _ExecuteEvenWhenPaused = false )
    {
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
    void SetCharacterCallback( T * _Object, void ( T::*_Method )( SWideChar, int, double ), bool _ExecuteEvenWhenPaused = false )
    {
        SetCharacterCallback( { _Object, _Method }, _ExecuteEvenWhenPaused );
    }

    /** Set callback for input characters */
    void SetCharacterCallback( TCallback< void( SWideChar, int, double ) > const & _Callback, bool _ExecuteEvenWhenPaused = false );

    void UnsetCharacterCallback();

    void UpdateAxes( float _TimeStep );

    bool IsKeyDown( int _Key ) const { return GetButtonState( ID_KEYBOARD, _Key ); }
    bool IsMouseDown( int _Button ) const { return GetButtonState( ID_MOUSE, _Button ); }
    //bool IsJoyDown( struct SJoystick const * _Joystick, int _Button ) const;
    bool IsJoyDown( int _JoystickId, int _Button ) const;

    void SetButtonState( int _DevId, int _Button, int _Action, int _ModMask, double _TimeStamp );

    /** Return is button pressed or not */
    bool GetButtonState( int _DevId, int _Button ) const;

    void UnpressButtons();

    void SetMouseAxisState( float _X, float _Y );

    float GetMouseMoveX() const { return MouseAxisState[MouseIndex].X; }
    float GetMouseMoveY() const { return MouseAxisState[MouseIndex].Y; }

    float GetMouseAxisState( int _Axis );

    void NotifyUnicodeCharacter( SWideChar _UnicodeCharacter, int _ModMask, double _TimeStamp );

    AInputComponent * GetNext() { return Next; }
    AInputComponent * GetPrev() { return Prev; }

    //static void SetJoystickState( int _Joystick, int _NumAxes, int _NumButtons, bool _bGamePad, bool _bConnected );

    static void SetJoystickAxisState( int _Joystick, int _Axis, float _Value );

    static float GetJoystickAxisState( int _Joystick, int _Axis );

    //static struct SJoystick const * GetJoysticks();

    static AInputComponent * GetInputComponents() { return InputComponents; }

protected:
    struct SAxisBinding
    {
        /** Axis name */
        AString Name;
        /** Binding callback */
        TCallback< void( float ) > Callback;
        /** Final axis value that will be passed to binding callback */
        float AxisScale;
        /** Execute binding even when paused */
        bool bExecuteEvenWhenPaused;
    };

    struct SActionBinding
    {
        /** Action name */
        AString Name;
        /** Binding callback */
        TCallback< void() > Callback[2];
        /** Execute binding even when paused */
        bool bExecuteEvenWhenPaused;
    };

    struct SPressedKey
    {
        unsigned short Key;
        short          AxisBinding;
        short          ActionBinding;
        float          AxisScale;
        uint8_t        DevId;

        bool HasAxis() const
        {
            return AxisBinding != -1;
        }

        bool HasAction() const
        {
            return ActionBinding != -1;
        }
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

    THash<>                    AxisBindingsHash;
    TStdVector< SAxisBinding > AxisBindings;

    THash<>                      ActionBindingsHash;
    TStdVector< SActionBinding > ActionBindings;

    /** Array of pressed keys */
    TArray< SPressedKey, MAX_PRESSED_KEYS > PressedKeys = {};
    int                                     NumPressedKeys = 0;

    // Index to PressedKeys array or -1 if button is up
    TArray< int8_t *, MAX_INPUT_DEVICES >                                 DeviceButtonDown;
    TArray< int8_t, MAX_KEYBOARD_BUTTONS >                                KeyboardButtonDown;
    TArray< int8_t, MAX_MOUSE_BUTTONS >                                   MouseButtonDown;
    TArray< TArray< int8_t, MAX_JOYSTICK_BUTTONS >, MAX_JOYSTICKS_COUNT > JoystickButtonDown;

    TArray< Float2, 2 > MouseAxisState;
    int                 MouseIndex = 0;

    TCallback< void( SWideChar, int, double ) > CharacterCallback;
    bool                                        bCharacterCallbackExecuteEvenWhenPaused = false;

    // Global list of input components
    AInputComponent *        Next = nullptr;
    AInputComponent *        Prev = nullptr;
    static AInputComponent * InputComponents;
    static AInputComponent * InputComponentsTail;
};

class AInputHelper final
{
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

extern ARuntimeVariable in_MouseSensitivity;
extern ARuntimeVariable in_MouseSensX;
extern ARuntimeVariable in_MouseSensY;
extern ARuntimeVariable in_MouseFilter;
extern ARuntimeVariable in_MouseInvertY;
extern ARuntimeVariable in_MouseAccel;
