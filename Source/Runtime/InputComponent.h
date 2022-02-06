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

#include "ActorComponent.h"
#include "InputDefs.h"
#include "Resource.h"

#include <Platform/Utf8.h>
#include <Containers/Array.h>
#include <Containers/StdHash.h>
#include <Core/ConsoleVar.h>

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
    ID_UNDEFINED      = 0xffff
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

struct SInputDeviceKey
{
    uint16_t DeviceId;
    uint16_t KeyId;

    int Hash() const
    {
        return Core::MurMur3Hash32(*(const int32_t*)&DeviceId);
    }

    bool operator==(SInputDeviceKey const& Rhs) const
    {
        return DeviceId == Rhs.DeviceId && KeyId == Rhs.KeyId;
    }
};

class AInputMappings : public AResource
{
    HK_CLASS(AInputMappings, AResource)

public:
    struct SMapping
    {
        AString Name;
        int     NameHash;
        float   AxisScale;
        uint8_t ModMask;
        uint8_t ControllerId;
        bool    bAxis;
    };

    struct SAxisMapping
    {
        uint16_t DeviceId;
        uint16_t KeyId;
        float    AxisScale;
        uint8_t  ControllerId;
    };

    AInputMappings()
    {}

    void MapAxis(AStringView AxisName, SInputDeviceKey const& DeviceKey, float AxisScale, int ControllerId);
    void UnmapAxis(SInputDeviceKey const& DeviceKey);

    void MapAction(AStringView ActionName, SInputDeviceKey const& DeviceKey, int ModMask, int ControllerId);
    void UnmapAction(SInputDeviceKey const& DeviceKey, int ModMask);

    void UnmapAll();

    TStdHashMap<SInputDeviceKey, TStdVector<SMapping>> const& GetMappings() const { return Mappings; }

    TStdHashMap<AString, TPodVector<SAxisMapping>> const& GetAxisMappings() const { return AxisMappings; }

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStream& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(const char* Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/InputMappings/Default"; }

private:
    void InitializeFromDocument(ADocument const& Document);

    TStdHashMap<SInputDeviceKey, TStdVector<SMapping>> Mappings;
    TStdHashMap<AString, TPodVector<SAxisMapping>>     AxisMappings;
};

class AInputComponent : public AActorComponent
{
    HK_COMPONENT(AInputComponent, AActorComponent)

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
    void SetInputMappings(AInputMappings* Mappings);

    /** Get input mappints config */
    AInputMappings* GetInputMappings() const;

    /** Bind axis to class method */
    template <typename T>
    void BindAxis(AStringView Axis, T* Object, void (T::*Method)(float), bool bExecuteEvenWhenPaused = false)
    {
        BindAxis(Axis, {Object, Method}, bExecuteEvenWhenPaused);
    }

    /** Bind axis to function */
    void BindAxis(AStringView Axis, TCallback<void(float)> const& Callback, bool bExecuteEvenWhenPaused = false);

    /** Unbind axis */
    void UnbindAxis(AStringView Axis);

    /** Bind action to class method */
    template <typename T>
    void BindAction(AStringView Action, int Event, T* Object, void (T::*Method)(), bool bExecuteEvenWhenPaused = false)
    {
        BindAction(Action, Event, {Object, Method}, bExecuteEvenWhenPaused);
    }

    /** Bind action to function */
    void BindAction(AStringView Action, int Event, TCallback<void()> const& Callback, bool bExecuteEvenWhenPaused = false);

    /** Unbind action */
    void UnbindAction(AStringView Action);

    /** Unbind all */
    void UnbindAll();

    /** Set callback for input characters */
    template <typename T>
    void SetCharacterCallback(T* Object, void (T::*Method)(SWideChar, int, double), bool bExecuteEvenWhenPaused = false)
    {
        SetCharacterCallback({Object, Method}, bExecuteEvenWhenPaused);
    }

    /** Set callback for input characters */
    void SetCharacterCallback(TCallback<void(SWideChar, int, double)> const& Callback, bool bExecuteEvenWhenPaused = false);

    void UnsetCharacterCallback();

    void UpdateAxes(float TimeStep);

    bool IsKeyDown(uint16_t Key) const { return GetButtonState({ID_KEYBOARD, Key}); }
    bool IsMouseDown(uint16_t Button) const { return GetButtonState({ID_MOUSE, Button}); }
    bool IsJoyDown(int JoystickId, uint16_t Button) const;

    void SetButtonState(SInputDeviceKey const& DeviceKey, int Action, int ModMask, double TimeStamp);

    /** Return is button pressed or not */
    bool GetButtonState(SInputDeviceKey const& DeviceKey) const;

    void UnpressButtons();

    void SetMouseAxisState(float X, float Y);

    float GetMouseMoveX() const { return MouseAxisState[MouseIndex].X; }
    float GetMouseMoveY() const { return MouseAxisState[MouseIndex].Y; }

    float GetMouseAxisState(int Axis);

    void NotifyUnicodeCharacter(SWideChar UnicodeCharacter, int ModMask, double TimeStamp);

    AInputComponent* GetNext() { return Next; }
    AInputComponent* GetPrev() { return Prev; }

    static void SetJoystickAxisState(int Joystick, int Axis, float Value);

    static float GetJoystickAxisState(int Joystick, int Axis);

    static AInputComponent* GetInputComponents() { return InputComponents; }

protected:
    struct SAxisBinding
    {
        /** Axis name */
        AString Name;
        /** Binding callback */
        TCallback<void(float)> Callback;
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
        TCallback<void()> Callback[2];
        /** Execute binding even when paused */
        bool bExecuteEvenWhenPaused;
    };

    struct SPressedKey
    {
        uint16_t Key;
        int16_t  AxisBinding;
        int16_t  ActionBinding;
        float    AxisScale;
        uint8_t  DeviceId;

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
    ~AInputComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    /** Return axis binding or -1 if axis is not binded */
    int GetAxisBinding(AInputMappings::SMapping const& Mapping) const;

    /** Return action binding or -1 if action is not binded */
    int GetActionBinding(AInputMappings::SMapping const& Mapping) const;

    TRef<AInputMappings> InputMappings;

    THash<>                  AxisBindingsHash;
    TStdVector<SAxisBinding> AxisBindings;
    int                      BindingVersion = 0;

    THash<>                    ActionBindingsHash;
    TStdVector<SActionBinding> ActionBindings;

    /** Array of pressed keys */
    TArray<SPressedKey, MAX_PRESSED_KEYS> PressedKeys    = {};
    int                                   NumPressedKeys = 0;

    // Index to PressedKeys array or -1 if button is up
    TArray<int8_t*, MAX_INPUT_DEVICES>                                DeviceButtonDown;
    TArray<int8_t, MAX_KEYBOARD_BUTTONS>                              KeyboardButtonDown;
    TArray<int8_t, MAX_MOUSE_BUTTONS>                                 MouseButtonDown;
    TArray<TArray<int8_t, MAX_JOYSTICK_BUTTONS>, MAX_JOYSTICKS_COUNT> JoystickButtonDown;

    TArray<Float2, 2> MouseAxisState;
    int               MouseIndex = 0;

    TCallback<void(SWideChar, int, double)> CharacterCallback;
    bool                                    bCharacterCallbackExecuteEvenWhenPaused = false;

    // Global list of input components
    AInputComponent*        Next = nullptr;
    AInputComponent*        Prev = nullptr;
    static AInputComponent* InputComponents;
    static AInputComponent* InputComponentsTail;
};

class AInputHelper final
{
public:
    /** Translate device to string */
    static const char* TranslateDevice(uint16_t DeviceId);

    /** Translate modifier to string */
    static const char* TranslateModifier(int Modifier);

    /** Translate key code to string */
    static const char* TranslateDeviceKey(SInputDeviceKey const& DeviceKey);

    /** Translate key owner player to string */
    static const char* TranslateController(int ControllerId);

    /** Lookup device from string */
    static uint16_t LookupDevice(AStringView Device);

    /** Lookup modifier from string */
    static int LookupModifier(AStringView Modifier);

    /** Lookup key code from string */
    static uint16_t LookupDeviceKey(uint16_t DeviceId, AStringView Key);

    /** Lookup key owner player from string */
    static int LookupController(AStringView ControllerId);
};

extern AConsoleVar in_MouseSensitivity;
extern AConsoleVar in_MouseSensX;
extern AConsoleVar in_MouseSensY;
extern AConsoleVar in_MouseFilter;
extern AConsoleVar in_MouseInvertY;
extern AConsoleVar in_MouseAccel;
