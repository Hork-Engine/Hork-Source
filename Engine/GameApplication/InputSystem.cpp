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
#include "GameApplication.h"

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

InputSystem::~InputSystem()
{
    for (auto* gamepadState : m_PlayerGamepadState)
        delete gamepadState;
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

        if (pressedKey.IsBinded && !pressedKey.VirtMapping.IsAction)
        {
            // Add one-frame empty axis for released button
            AddAxis(pressedKey.VirtMapping.Name, pressedKey.VirtMapping.Owner, 0);
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
    for (uint32_t i = 0; i < VIRTUAL_KEY_COUNT; ++i)
        SetKeyState(VirtualKey(i), InputEvent::OnRelease, {});

    for (int playerIndex = 0; playerIndex < int(m_PlayerGamepadState.Size()); ++playerIndex)
    {
        if (m_PlayerGamepadState[playerIndex])
        {
            for (uint32_t i = 0; i < GAMEPAD_KEY_COUNT; ++i)
                SetGamepadButtonState(GamepadKey(i), InputEvent::OnRelease, PlayerController(playerIndex));
        }
    }
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

InputSystem::PlayerGamepadState* InputSystem::GetPlayerGamepadState(PlayerController player)
{
    using PlayerIndex = std::make_signed_t<std::underlying_type_t<PlayerController>>;

    auto playerIndex = PlayerIndex(player);
    HK_ASSERT(playerIndex >= 0);
    if (playerIndex < 0)
        return nullptr;

    while (m_PlayerGamepadState.Size() <= playerIndex)
        m_PlayerGamepadState.Add(nullptr);

    if (!m_PlayerGamepadState[playerIndex])
        m_PlayerGamepadState[playerIndex] = new PlayerGamepadState;

    return m_PlayerGamepadState[playerIndex];
}

void InputSystem::SetGamepadButtonState(GamepadKey key, InputEvent event, PlayerController player)
{
    PlayerGamepadState* state = GetPlayerGamepadState(player);
    if (!state)
        return;

    auto& buttonState = state->m_ButtonState[ToUnderlying(key)];
    bool wasPressed = buttonState.IsPressed;

    if ((event == InputEvent::OnPress && wasPressed) || (event == InputEvent::OnRelease && !wasPressed))
        return;

    if (event == InputEvent::OnPress)
    {
        buttonState.IsPressed = true;

        if (m_InputMappings)
        {
            buttonState.IsBinded = m_InputMappings->GetGamepadMapping(player, key, buttonState.VirtMapping);
        }
        else
            buttonState.IsBinded = false;

        if (buttonState.IsBinded && buttonState.VirtMapping.IsAction)
        {
            auto& action = m_ActionPool.EmplaceBack();
            action.Name = buttonState.VirtMapping.Name;
            action.Owner = buttonState.VirtMapping.Owner;
            action.IsPressed = true;
        }
    }
    else if (event == InputEvent::OnRelease)
    {
        buttonState.IsPressed = false;

        if (buttonState.IsBinded && buttonState.VirtMapping.IsAction)
        {
            auto& action = m_ActionPool.EmplaceBack();
            action.Name = buttonState.VirtMapping.Name;
            action.Owner = buttonState.VirtMapping.Owner;
            action.IsPressed = false;
        }

        if (buttonState.IsBinded && !buttonState.VirtMapping.IsAction)
        {
            // Add one-frame empty axis for released button
            AddAxis(buttonState.VirtMapping.Name, buttonState.VirtMapping.Owner, 0);
        }
    }
}

void InputSystem::SetGamepadAxis(GamepadAxis axis, float value, PlayerController player)
{
    PlayerGamepadState* state = GetPlayerGamepadState(player);
    if (!state)
        return;

    float oldValue = state->m_AxisState[ToUnderlying(axis)];
    state->m_AxisState[ToUnderlying(axis)] = value;

    if (m_InputMappings)
    {
        if (value == 1.0f && oldValue != 1.0f)
        {
            VirtualMapping virtMapping;
            bool isBinded = m_InputMappings->GetGamepadMapping(player, axis, virtMapping);

            if (isBinded && virtMapping.IsAction)
            {
                auto& action = m_ActionPool.EmplaceBack();
                action.Name = virtMapping.Name;
                action.Owner = virtMapping.Owner;
                action.IsPressed = true;
            }
        }
        else if (value != 1.0f && oldValue == 1.0f)
        {
            VirtualMapping virtMapping;
            bool isBinded = m_InputMappings->GetGamepadMapping(player, axis, virtMapping);

            if (isBinded && virtMapping.IsAction)
            {
                auto& action = m_ActionPool.EmplaceBack();
                action.Name = virtMapping.Name;
                action.Owner = virtMapping.Owner;
                action.IsPressed = false;
            }
        }
    }
}

void InputSystem::AddCharacter(WideChar ch, KeyModifierMask modMask)
{
    m_Chars.Add({ch, modMask});
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
            AddAxis(key->VirtMapping.Name, key->VirtMapping.Owner, key->VirtMapping.Power);
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
        float delta = mouseDelta[mouseAxis];

        if (delta == 0 && m_MousePrevDelta[mouseAxis] == 0)
            continue;

        if (m_InputMappings->GetMapping(mouseVirtualAxis[mouseAxis], {}, virtMapping))
            AddAxis(virtMapping.Name, virtMapping.Owner, delta * virtMapping.Power * mouseSens[mouseAxis]);
    }

    m_MousePrevDelta = mouseDelta;

    m_MouseIndex ^= 1;
    m_MouseAxisState[m_MouseIndex].Clear();

    // Apply gamepad

    int playerIndex = 0;
    for (auto* gamepadState : m_PlayerGamepadState)
    {
        if (gamepadState)
        {
            for (auto* buttonState = gamepadState->m_ButtonState; buttonState < &gamepadState->m_ButtonState[GAMEPAD_KEY_COUNT]; ++buttonState)
            {
                if (buttonState->IsPressed && buttonState->IsBinded && !buttonState->VirtMapping.IsAction)
                    AddAxis(buttonState->VirtMapping.Name, buttonState->VirtMapping.Owner, buttonState->VirtMapping.Power);
            }

            for (int axis = 0; axis < GAMEPAD_AXIS_COUNT; ++axis)
            {
                float delta = gamepadState->m_AxisState[axis];
                if (delta == 0 && gamepadState->m_PrevAxisState[axis] == 0)
                    continue;

                if (m_InputMappings->GetGamepadMapping(PlayerController(playerIndex), GamepadAxis(axis), virtMapping))
                    AddAxis(virtMapping.Name, virtMapping.Owner, delta * virtMapping.Power);

                gamepadState->m_PrevAxisState[axis] = delta;
            }
        }
        ++playerIndex;
    }
}

void InputSystem::AddAxis(StringID name, PlayerController owner, float power)
{
    for (Axis& axis : m_AxisPool)
    {
        if (axis.Name == name && axis.Owner == owner)
        {
            axis.Amount += power;
            return;
        }
    }
    Axis& axis = m_AxisPool.EmplaceBack();
    axis.Name = name;
    axis.Owner = owner;
    axis.Amount = power;
}

HK_NAMESPACE_END
