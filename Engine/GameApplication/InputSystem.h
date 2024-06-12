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

#include <Engine/Core/Containers/Array.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Math/VectorMath.h>

#include "InputMappings.h"

HK_NAMESPACE_BEGIN

enum class InputEvent : uint8_t
{
    OnPress,
    OnRelease
};

class InputSystem
{
public:
                            InputSystem();

    void                    SetInputMappings(InputMappings* mappings);

    void                    ResetKeyState();

    void                    SetKeyState(VirtualKey virtualKey, InputEvent event, KeyModifierMask modMask);
    bool                    IsKeyDown(VirtualKey virtualKey) const;

    void                    SetMouseAxisState(float x, float y);

    float                   GetMouseMoveX() const { return m_MouseAxisState[m_MouseIndex].X; }
    float                   GetMouseMoveY() const { return m_MouseAxisState[m_MouseIndex].Y; }

    void                    SetCursorPosition(Float2 const& position)   { m_CursorPosition = position; }
    Float2 const&           GetCursorPosition() const                   { return m_CursorPosition; }

    void                    NotifyUnicodeCharacter(WideChar unicodeCharacter, KeyModifierMask modMask);

    void                    NewFrame();
    void                    Tick(float timeStep);

    struct Action
    {
        StringID            Name;
        PlayerController    Owner;
        bool                IsPressed;
    };

    struct Axis
    {
        StringID            Name;
        PlayerController    Owner;
        float               Power;
    };

    struct Char
    {
        WideChar            Char;
        KeyModifierMask     ModMask;
    };

    Vector<Action> const&   GetActionPool() const   { return m_ActionPool; }
    Vector<Axis> const&     GetAxisPool() const     { return m_AxisPool; }
    Vector<Char> const&     GetChars() const        { return m_Chars; }

private:
    void                    AddAxis(StringID name, PlayerController owner, float value);

    Ref<InputMappings>      m_InputMappings;

    struct PressedKey
    {
        VirtualKey          VirtKey;
        VirtualMapping      VirtMapping;
        bool                IsBinded;
    };

    static constexpr int MAX_PRESSED_KEYS = 128;
    using PressedKeysArray  = Array<PressedKey, MAX_PRESSED_KEYS>;

    PressedKeysArray        m_PressedKeys;
    int                     m_NumPressedKeys = 0;

    using KeyStateArray     = Array<int8_t, VirtualKeyTableSize>;
    KeyStateArray           m_KeyStateMap;

    Array<Float2, 2>        m_MouseAxisState;
    int                     m_MouseIndex = 0;
    Float2                  m_CursorPosition;

    Vector<Action>          m_ActionPool;
    Vector<Axis>            m_AxisPool;
    Vector<Char>            m_Chars;
};

extern ConsoleVar           in_MouseSensitivity;
extern ConsoleVar           in_MouseSensX;
extern ConsoleVar           in_MouseSensY;
extern ConsoleVar           in_MouseFilter;
extern ConsoleVar           in_MouseInvertY;
extern ConsoleVar           in_MouseAccel;

HK_NAMESPACE_END
