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

#include "InputSystem.h"

#include <Engine/Core/Logger.h>

HK_NAMESPACE_BEGIN

ConsoleVar in_MouseSensitivity("in_MouseSensitivity"s, "6.8"s);
ConsoleVar in_MouseSensX("in_MouseSensX"s, "0.022"s);
ConsoleVar in_MouseSensY("in_MouseSensY"s, "0.022"s);
ConsoleVar in_MouseFilter("in_MouseFilter"s, "1"s);
ConsoleVar in_MouseInvertY("in_MouseInvertY"s, "0"s);
ConsoleVar in_MouseAccel("in_MouseAccel"s, "0"s);

InputSystem::InputSystem()
{
    for (int i = 0; i < m_KeyStateMap.Size(); ++i)
        m_KeyStateMap[i] = -1;
}

void InputSystem::SetInputMappings(InputMappings* mappings)
{
    m_InputMappings = mappings;
}

void InputSystem::SetKeyState(VirtualKey virtualKey, InputEvent event, KeyModifierMask modMask)
{
    auto virtualKeyNum = ToUnderlying(virtualKey);
    bool wasPressed = m_KeyStateMap[virtualKeyNum] != -1;

    if ((event == InputEvent::OnPress && wasPressed) || (event == InputEvent::OnRelease && !wasPressed))
        return;

    if (event == InputEvent::OnPress)
    {
        if (m_NumPressedKeys >= MAX_PRESSED_KEYS)
        {
            LOG("MAX_PRESSED_KEYS hit\n");
            return;
        }

        PressedKey& pressedKey = m_PressedKeys[m_NumPressedKeys];
        pressedKey.VirtKey = virtualKey;

        if (m_InputMappings)
        {
            pressedKey.IsBinded = m_InputMappings->GetMapping(virtualKey, modMask, pressedKey.VirtMapping);
            if (!pressedKey.IsBinded && modMask)
                pressedKey.IsBinded = m_InputMappings->GetMapping(virtualKey, {}, pressedKey.VirtMapping);
        }
        else
            pressedKey.IsBinded = false;

        m_KeyStateMap[virtualKeyNum] = m_NumPressedKeys;
        ++m_NumPressedKeys;

        if (pressedKey.IsBinded && pressedKey.VirtMapping.IsAction)
        {
            auto& action = m_ActionPool.EmplaceBack();
            action.Name = pressedKey.VirtMapping.Name;
            action.Owner = pressedKey.VirtMapping.Owner;
            action.IsPressed = true;
        }
    }
    else if (event == InputEvent::OnRelease)
    {
        auto index = m_KeyStateMap[virtualKeyNum];
        PressedKey& pressedKey = m_PressedKeys[index];
        if (pressedKey.IsBinded && pressedKey.VirtMapping.IsAction)
        {
            auto& action = m_ActionPool.EmplaceBack();
            action.Name = pressedKey.VirtMapping.Name;
            action.Owner = pressedKey.VirtMapping.Owner;
            action.IsPressed = false;
        }

        m_KeyStateMap[virtualKeyNum] = -1;

        if (index != m_NumPressedKeys - 1)
        {
            m_PressedKeys[index] = m_PressedKeys[m_NumPressedKeys - 1];
            m_KeyStateMap[ToUnderlying(m_PressedKeys[index].VirtKey)] = index;
        }

        --m_NumPressedKeys;
        HK_ASSERT(m_NumPressedKeys >= 0);
    }
}

void InputSystem::ResetKeyState()
{
    for (uint32_t i = 0; i < VirtualKeyTableSize; ++i)
        SetKeyState(VirtualKey(i), InputEvent::OnRelease, {});
}

bool InputSystem::IsKeyDown(VirtualKey virtualKey) const
{
    return m_KeyStateMap[ToUnderlying(virtualKey)] != -1;
}

void InputSystem::SetMouseAxisState(float x, float y)
{
    m_MouseAxisState[m_MouseIndex].X += x;
    m_MouseAxisState[m_MouseIndex].Y += y;
}

void InputSystem::NotifyUnicodeCharacter(WideChar unicodeCharacter, KeyModifierMask modMask)
{
    auto& ch = m_Chars.EmplaceBack();
    ch.Char = unicodeCharacter;
    ch.ModMask = modMask;
}

void InputSystem::NewFrame()
{
    m_ActionPool.Clear();
    m_AxisPool.Clear();
    m_Chars.Clear();
}

void InputSystem::Tick(float timeStep)
{
    if (!m_InputMappings)
        return;

    // Apply keyboard
    for (PressedKey* key = m_PressedKeys.ToPtr(); key < &m_PressedKeys[m_NumPressedKeys]; key++)
    {
        if (key->IsBinded && !key->VirtMapping.IsAction)
        {
            AddAxis(key->VirtMapping.Name, key->VirtMapping.Owner, key->VirtMapping.Power * timeStep);
        }
    }

    // Apply mouse
    Float2 mouseDelta;

    if (in_MouseFilter)
        mouseDelta = (m_MouseAxisState[0] + m_MouseAxisState[1]) * 0.5f;
    else
        mouseDelta = m_MouseAxisState[m_MouseIndex];

    if (in_MouseInvertY)
        mouseDelta.Y = -mouseDelta.Y;

    float timeStepMsec     = Math::Max(timeStep * 1000, 200);
    float mouseInputRate   = mouseDelta.Length() / timeStepMsec;
    float mouseCurrentSens = in_MouseSensitivity.GetFloat() + mouseInputRate * in_MouseAccel.GetFloat();
    float mouseSens[2]     = {in_MouseSensX.GetFloat() * mouseCurrentSens, in_MouseSensY.GetFloat() * mouseCurrentSens};

    VirtualMapping virtMapping;
    const VirtualAxis mouseVirtualAxis[2] = {VirtualAxis::MouseHorizontal, VirtualAxis::MouseVertical};
    for (int mouseAxis = 0; mouseAxis < 2; ++mouseAxis)
    {
        if (m_InputMappings->GetMapping(mouseVirtualAxis[mouseAxis], {}, virtMapping))
        {
            float delta = mouseDelta[mouseAxis];

            if (delta)
            {
                AddAxis(virtMapping.Name, virtMapping.Owner, delta * virtMapping.Power * mouseSens[mouseAxis]);
            }
        }
    }

    m_MouseIndex ^= 1;
    m_MouseAxisState[m_MouseIndex].Clear();

    // TODO: Apply gamepad
}

void InputSystem::AddAxis(StringID name, PlayerController owner, float power)
{
    for (Axis& axis : m_AxisPool)
    {
        if (axis.Name == name && axis.Owner == owner)
        {
            axis.Power += power;
            return;
        }
    }
    Axis& axis = m_AxisPool.EmplaceBack();
    axis.Name = name;
    axis.Owner = owner;
    axis.Power = power;
}

HK_NAMESPACE_END
