/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "InputDefs.h"
#include <Engine/Core/Delegate.h>
#include <Engine/Core/Containers/Array.h>
#include <Engine/Core/Containers/Hash.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Math/VectorMath.h>

HK_NAMESPACE_BEGIN

constexpr int MAX_PRESSED_KEYS = 128;
constexpr int MAX_INPUT_CONTROLLERS = 16;

enum INPUT_DEVICE
{
    /* Input devices */
    ID_KEYBOARD = 0,
    ID_MOUSE = 1,
    ID_JOYSTICK_1 = 2,
    ID_JOYSTICK_2 = 3,
    ID_JOYSTICK_3 = 4,
    ID_JOYSTICK_4 = 5,
    ID_JOYSTICK_5 = 6,
    ID_JOYSTICK_6 = 7,
    ID_JOYSTICK_7 = 8,
    ID_JOYSTICK_8 = 9,
    ID_JOYSTICK_9 = 10,
    ID_JOYSTICK_10 = 11,
    ID_JOYSTICK_11 = 12,
    ID_JOYSTICK_12 = 13,
    ID_JOYSTICK_13 = 14,
    ID_JOYSTICK_14 = 15,
    ID_JOYSTICK_15 = 16,
    ID_JOYSTICK_16 = 17,
    MAX_INPUT_DEVICES = 18,
    ID_UNDEFINED = 0xffff
};

/* Player controllers */
enum CONTROLLER
{
    CONTROLLER_PLAYER_1 = 0,
    CONTROLLER_PLAYER_2 = 1,
    CONTROLLER_PLAYER_3 = 2,
    CONTROLLER_PLAYER_4 = 3,
    CONTROLLER_PLAYER_5 = 4,
    CONTROLLER_PLAYER_6 = 5,
    CONTROLLER_PLAYER_7 = 6,
    CONTROLLER_PLAYER_8 = 7,
    CONTROLLER_PLAYER_9 = 8,
    CONTROLLER_PLAYER_10 = 9,
    CONTROLLER_PLAYER_11 = 10,
    CONTROLLER_PLAYER_12 = 11,
    CONTROLLER_PLAYER_13 = 12,
    CONTROLLER_PLAYER_14 = 13,
    CONTROLLER_PLAYER_15 = 14,
    CONTROLLER_PLAYER_16 = 15
};

struct InputDeviceKey
{
    uint16_t DeviceId;
    uint16_t KeyId;

    uint32_t Hash() const
    {
        return HashTraits::Murmur3Hash32(*(const int32_t*)&DeviceId);
    }

    bool operator==(InputDeviceKey const& Rhs) const
    {
        return DeviceId == Rhs.DeviceId && KeyId == Rhs.KeyId;
    }
};

class InputMappings : public RefCounted
{
public:
    struct Mapping
    {
        String Name;
        int NameHash;
        float AxisScale;
        uint8_t ModMask;
        uint8_t ControllerId;
        bool bAxis;
    };

    struct AxisMapping
    {
        uint16_t DeviceId;
        uint16_t KeyId;
        float AxisScale;
        uint8_t ControllerId;
    };

    InputMappings() = default;

    void MapAxis(StringView AxisName, InputDeviceKey const& DeviceKey, float AxisScale, int ControllerId);
    void UnmapAxis(InputDeviceKey const& DeviceKey);

    void MapAction(StringView ActionName, InputDeviceKey const& DeviceKey, int ModMask, int ControllerId);
    void UnmapAction(InputDeviceKey const& DeviceKey, int ModMask);

    void UnmapAll();

    THashMap<InputDeviceKey, TVector<Mapping>> const& GetMappings() const { return m_Mappings; }

    TNameHash<TVector<AxisMapping>> const& GetAxisMappings() const { return m_AxisMappings; }

protected:
    ///** Load resource from file */
    //bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    ///** Create internal resource */
    //void LoadInternalResource(StringView Path) override;

    //const char* GetDefaultResourcePath() const override { return "/Default/InputMappings/Default"; }

private:
    //void InitializeFromDocument(DocumentValue const& Document);

    THashMap<InputDeviceKey, TVector<Mapping>> m_Mappings;
    TNameHash<TVector<AxisMapping>> m_AxisMappings;
};

class InputState
{
public:
    //struct AxisState
    //{
    //    Hk::String Name;
    //    float Scale;
    //};

    struct Action
    {
        Hk::String Name;
        bool bPressed;
    };

    using PlayerIndex = int;

    float GetAxisScale(StringView axis, PlayerIndex playerNum) const
    {
        auto& scale = m_AxisScale[playerNum];
        auto it = scale.Find(axis);
        return it == scale.End() ? 0.0f : it->second;
    }

    TArray<TNameHash<float>, MAX_INPUT_CONTROLLERS> m_AxisScale;
    TArray<Hk::TVector<Action>, MAX_INPUT_CONTROLLERS> m_ActionPool;
};

class InputSystem
{
public:
    /** Filter keyboard events */
    bool bIgnoreKeyboardEvents = false;

    /** Filter mouse events */
    bool bIgnoreMouseEvents = false;

    /** Filter joystick events */
    bool bIgnoreJoystickEvents = false;

    /** Filter character events */
    bool bIgnoreCharEvents = false;

    InputSystem();
    ~InputSystem();

    /** Set input mappings config */
    void SetInputMappings(InputMappings* Mappings);

    /** Get input mappints config */
    InputMappings* GetInputMappings() const;    

    /** Set callback for input characters */
    template <typename T>
    void SetCharacterCallback(T* Object, void (T::*Method)(WideChar, int), bool bExecuteEvenWhenPaused = false)
    {
        SetCharacterCallback({Object, Method}, bExecuteEvenWhenPaused);
    }

    /** Set callback for input characters */
    void SetCharacterCallback(Delegate<void(WideChar, int)> Callback, bool bExecuteEvenWhenPaused = false);

    void UnsetCharacterCallback();

    void NewFrame();

    void Tick(float TimeStep);

    bool IsKeyDown(uint16_t Key) const { return GetButtonState({ID_KEYBOARD, Key}); }
    bool IsMouseDown(uint16_t Button) const { return GetButtonState({ID_MOUSE, Button}); }
    bool IsJoyDown(int JoystickId, uint16_t Button) const;

    void SetButtonState(InputDeviceKey const& DeviceKey, int Action, int ModMask);

    /** Return is button pressed or not */
    bool GetButtonState(InputDeviceKey const& DeviceKey) const;

    void UnpressButtons();

    void SetMouseAxisState(float X, float Y);

    float GetMouseMoveX() const { return m_MouseAxisState[m_MouseIndex].X; }
    float GetMouseMoveY() const { return m_MouseAxisState[m_MouseIndex].Y; }

    float GetMouseAxisState(int Axis);

    void SetCursorPosition(Float2 const& cursorPosition) { m_CursorPosition = cursorPosition; }

    Float2 const& GetCursorPosition() const { return m_CursorPosition; }

    void NotifyUnicodeCharacter(WideChar UnicodeCharacter, int ModMask);

    static void SetJoystickAxisState(int Joystick, int Axis, float Value);

    static float GetJoystickAxisState(int Joystick, int Axis);

    InputState const& GetInputState() const
    {
        return m_InputState;
    }

protected:
    enum class BINDING_TYPE
    {
        UNDEFINED,
        AXIS,
        ACTION
    };

    struct PressedKey
    {
        uint16_t     Key;
        String       Binding;
        BINDING_TYPE BindingType;
        float        AxisScale;
        uint8_t      DeviceId;
        uint8_t      ControllerId;

        void BindAxis(StringView Axis, float Scale)
        {
            BindingType = BINDING_TYPE::AXIS;
            Binding     = Axis;
            AxisScale   = Scale;
        }

        void BindAction(StringView Action)
        {
            BindingType = BINDING_TYPE::ACTION;
            Binding     = Action;
        }

        void Unbind()
        {
            BindingType = BINDING_TYPE::UNDEFINED;
        }
    };

    TRef<InputMappings> m_InputMappings;

    /** Array of pressed keys */
    TArray<PressedKey, MAX_PRESSED_KEYS> m_PressedKeys = {};
    int m_NumPressedKeys = 0;

    // Index to PressedKeys array or -1 if button is up
    TArray<int8_t*, MAX_INPUT_DEVICES> m_DeviceButtonDown;
    TArray<int8_t, MAX_KEYBOARD_BUTTONS> m_KeyboardButtonDown;
    TArray<int8_t, MAX_MOUSE_BUTTONS> m_MouseButtonDown;
    TArray<TArray<int8_t, MAX_JOYSTICK_BUTTONS>, MAX_JOYSTICKS_COUNT> m_JoystickButtonDown;

    TArray<Float2, 2> m_MouseAxisState;
    int m_MouseIndex = 0;

    Float2 m_CursorPosition;

    Delegate<void(WideChar, int)> m_CharacterCallback;
    bool m_bCharacterCallbackExecuteEvenWhenPaused = false;

    InputState m_InputState;
};

class InputHelper final
{
public:
    /** Translate device to string */
    static const char* TranslateDevice(uint16_t DeviceId);

    /** Translate modifier to string */
    static const char* TranslateModifier(int Modifier);

    /** Translate key code to string */
    static const char* TranslateDeviceKey(InputDeviceKey const& DeviceKey);

    /** Translate key owner player to string */
    static const char* TranslateController(int ControllerId);

    /** Lookup device from string */
    static uint16_t LookupDevice(StringView Device);

    /** Lookup modifier from string */
    static int LookupModifier(StringView Modifier);

    /** Lookup key code from string */
    static uint16_t LookupDeviceKey(uint16_t DeviceId, StringView Key);

    /** Lookup key owner player from string */
    static int LookupController(StringView ControllerId);
};

extern ConsoleVar in_MouseSensitivity;
extern ConsoleVar in_MouseSensX;
extern ConsoleVar in_MouseSensY;
extern ConsoleVar in_MouseFilter;
extern ConsoleVar in_MouseInvertY;
extern ConsoleVar in_MouseAccel;

HK_NAMESPACE_END
