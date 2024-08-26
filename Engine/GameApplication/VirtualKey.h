/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Engine/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

enum class VirtualKey : uint16_t
{
    // Mouse
    MouseLeftBtn,
    MouseRightBtn,
    MouseMidBtn,
    Mouse4,
    Mouse5,
    Mouse6,
    Mouse7,
    Mouse8,
    MouseWheelUp,
    MouseWheelDown,
    MouseWheelLeft,
    MouseWheelRight,

    // Keyboard
    Space,
    Apostrophe, /* ' */
    Comma,      /* , */
    Minus,      /* - */
    Period,     /* . */
    Slash,      /* / */
    _0,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,
    Semicolon, /* ; */
    Equal,     /* = */
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LeftBracket,  /* [ */
    Backslash,    /* \ */
    RightBracket, /* ] */
    GraveAccent,  /* ` */
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    KP_0,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,
    KP_Decimal,
    KP_Divide,
    KP_Multiply,
    KP_Subtract,
    KP_Add,
    KP_Enter,
    KP_Equal,
    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu,
};

enum class VirtualAxis : uint16_t
{
    // Mouse
    MouseHorizontal,
    MouseVertical,
};

struct VirtualKeyDesc
{
    const char* Name;
};

struct VirtualAxisDesc
{
    const char* Name;
};

extern const VirtualKeyDesc VirtualKeyTable[];
extern const VirtualAxisDesc VirtualAxisTable[];

constexpr uint32_t VIRTUAL_KEY_COUNT = 129;
constexpr uint32_t VIRTUAL_AXIS_COUNT = 2;

struct VirtualKeyOrAxis
{
    VirtualKeyOrAxis() = default;

    VirtualKeyOrAxis(VirtualKey key) :
        m_Data(uint16_t(key))
    {}

    VirtualKeyOrAxis(VirtualAxis axis) :
        m_Data(uint16_t(axis) | (1 << 15))
    {}

    VirtualKeyOrAxis(VirtualKeyOrAxis const& rhs) = default;

    VirtualKeyOrAxis& operator=(VirtualKeyOrAxis const& rhs) = default;

    VirtualKeyOrAxis& operator=(VirtualKey key)
    {
        m_Data = uint16_t(key);
        return *this;
    }

    VirtualKeyOrAxis& operator=(VirtualAxis axis)
    {
        m_Data = uint16_t(axis) | (1 << 15);
        return *this;
    }

    bool operator==(VirtualKeyOrAxis const& rhs) const
    {
        return m_Data == rhs.m_Data;
    }

    bool operator!=(VirtualKeyOrAxis const& rhs) const
    {
        return m_Data != rhs.m_Data;
    }

    bool IsAxis() const
    {
        return (m_Data >> 15) == 1;
    }

    VirtualKey AsKey() const
    {
        if (IsAxis())
            return VirtualKey(0);
        return VirtualKey(m_Data);
    }

    VirtualAxis AsAxis() const
    {
        if (!IsAxis())
            return VirtualAxis(0);
        return VirtualAxis(m_Data & ~(1 << 15));
    }

    const char* GetName() const
    {
        return IsAxis() ? VirtualAxisTable[ToUnderlying(AsAxis())].Name : VirtualKeyTable[ToUnderlying(AsKey())].Name;
    }

    uint16_t GetData() const
    {
        return m_Data;
    }

private:
    uint16_t        m_Data = 0;
};

struct KeyModifierMask
{
    void Clear()
    {
        m_Flags = 0;
    }

    bool IsEmpty() const
    {
        return m_Flags == 0;
    }

    operator bool() const
    {
        return m_Flags != 0;
    }

    union
    {
        uint16_t m_Flags = 0;

        struct
        {
            bool Shift : 1;
            bool Control : 1;
            bool Alt : 1;
            bool Super : 1;
            bool CapsLock : 1;
            bool NumLock : 1;
        };
    };
};

enum class GamepadKey : uint16_t
{
    A,
    B,
    X,
    Y,
    Back,
    Guide,
    Start,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DPad_Up,
    DPad_Down,
    DPad_Left,
    DPad_Right,
    Misc1,    // Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button
    Paddle1,  // Xbox Elite paddle P1
    Paddle2,  // Xbox Elite paddle P3
    Paddle3,  // Xbox Elite paddle P2
    Paddle4,  // Xbox Elite paddle P4
    Touchpad, // PS4/PS5 touchpad button
    Misc2,    // Additional button
    Misc3,    // Additional button
    Misc4,    // Additional button
    Misc5,    // Additional button
    Misc6,    // Additional button
};

enum class GamepadAxis : uint16_t
{
    LeftX,
    LeftY,
    RightX,
    RightY,
    TriggerLeft,
    TriggerRight
};

struct GamepadKeyDesc
{
    const char* Name;
};

struct GamepadAxisDesc
{
    const char* Name;
};

extern const GamepadKeyDesc GamepadKeyTable[];
extern const GamepadAxisDesc GamepadAxisTable[];

constexpr uint32_t GAMEPAD_KEY_COUNT = 26;
constexpr uint32_t GAMEPAD_AXIS_COUNT = 6;

struct GamepadKeyOrAxis
{
    GamepadKeyOrAxis() = default;

    GamepadKeyOrAxis(GamepadKey key) :
        m_Data(uint16_t(key))
    {}

    GamepadKeyOrAxis(GamepadAxis axis) :
        m_Data(uint16_t(axis) | (1 << 15))
    {}

    GamepadKeyOrAxis(GamepadKeyOrAxis const& rhs) = default;

    GamepadKeyOrAxis& operator=(GamepadKeyOrAxis const& rhs) = default;

    GamepadKeyOrAxis& operator=(GamepadKey key)
    {
        m_Data = uint16_t(key);
        return *this;
    }

    GamepadKeyOrAxis& operator=(GamepadAxis axis)
    {
        m_Data = uint16_t(axis) | (1 << 15);
        return *this;
    }

    bool operator==(GamepadKeyOrAxis const& rhs) const
    {
        return m_Data == rhs.m_Data;
    }

    bool operator!=(GamepadKeyOrAxis const& rhs) const
    {
        return m_Data != rhs.m_Data;
    }

    bool IsAxis() const
    {
        return (m_Data >> 15) == 1;
    }

    GamepadKey AsKey() const
    {
        if (IsAxis())
            return GamepadKey(0);
        return GamepadKey(m_Data);
    }

    GamepadAxis AsAxis() const
    {
        if (!IsAxis())
            return GamepadAxis(0);
        return GamepadAxis(m_Data & ~(1 << 15));
    }

    const char* GetName() const
    {
        return IsAxis() ? GamepadAxisTable[ToUnderlying(AsAxis())].Name : GamepadKeyTable[ToUnderlying(AsKey())].Name;
    }

    uint16_t GetData() const
    {
        return m_Data;
    }

private:
    uint16_t        m_Data = 0;
};

HK_NAMESPACE_END
