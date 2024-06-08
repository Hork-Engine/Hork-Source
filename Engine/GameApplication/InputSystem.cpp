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

struct InputSystemStatic
{
    TArray<const char*, MAX_KEYBOARD_BUTTONS>  KeyNames;
    TArray<const char*, MAX_MOUSE_BUTTONS>     MouseButtonNames;
    TArray<const char*, MAX_MOUSE_AXES>        MouseAxisNames;
    TArray<const char*, MAX_INPUT_DEVICES>     DeviceNames;
    TArray<const char*, MAX_JOYSTICK_BUTTONS>  JoystickButtonNames;
    TArray<const char*, MAX_JOYSTICK_AXES>     JoystickAxisNames;
    TArray<const char*, MAX_MODIFIERS>         ModifierNames;
    TArray<const char*, MAX_INPUT_CONTROLLERS> ControllerNames;

    TArray<int, MAX_INPUT_DEVICES>                                DeviceButtonLimits;
    TArray<TArray<float, MAX_JOYSTICK_AXES>, MAX_JOYSTICKS_COUNT> JoystickAxisState;

    InputSystemStatic()
    {
#define InitKey(Key)                KeyNames[Key] = HK_STRINGIFY(Key) + 4
#define InitKey2(Key, Name)         KeyNames[Key] = Name
#define InitButton(Button, Name)    MouseButtonNames[Button] = Name
#define InitMouseAxis(Axis, Name)   MouseAxisNames[Axis - MOUSE_AXIS_BASE] = Name
#define InitDevice(DeviceId)        DeviceNames[DeviceId] = HK_STRINGIFY(DeviceId) + 3
#define InitJoyButton(Button, Name) JoystickButtonNames[Button - JOY_BUTTON_BASE] = Name
#define InitJoyAxis(Axis, Name)     JoystickAxisNames[Axis - JOY_AXIS_BASE] = Name
#define InitModifier(Modifier)      ModifierNames[Modifier] = HK_STRINGIFY(Modifier) + 4
#define InitController(Controller)  ControllerNames[Controller] = HK_STRINGIFY(Controller) + 11

        DeviceButtonLimits[ID_KEYBOARD] = MAX_KEYBOARD_BUTTONS;
        DeviceButtonLimits[ID_MOUSE]    = MAX_MOUSE_BUTTONS + MAX_MOUSE_AXES;
        for (int i = ID_JOYSTICK_1; i <= ID_JOYSTICK_16; i++)
        {
            DeviceButtonLimits[i] = MAX_JOYSTICK_BUTTONS + MAX_JOYSTICK_AXES;
        }

        for (int i = 0; i < KeyNames.Size(); i++)
        {
            KeyNames[i] = "";
        }

        for (int i = 0; i < MouseButtonNames.Size(); i++)
        {
            MouseButtonNames[i] = "";
        }

        for (int i = 0; i < MouseAxisNames.Size(); i++)
        {
            MouseAxisNames[i] = "";
        }

        for (int i = 0; i < DeviceNames.Size(); i++)
        {
            DeviceNames[i] = "";
        }

        for (int i = 0; i < JoystickButtonNames.Size(); i++)
        {
            JoystickButtonNames[i] = "";
        }

        for (int i = 0; i < JoystickAxisNames.Size(); i++)
        {
            JoystickAxisNames[i] = "";
        }

        InitKey2(KEY_SPACE, "Space");
        InitKey2(KEY_APOSTROPHE, "'");
        InitKey2(KEY_COMMA, ",");
        InitKey2(KEY_MINUS, "-");
        InitKey2(KEY_PERIOD, "Period");
        InitKey2(KEY_SLASH, "/");
        InitKey(KEY_0);
        InitKey(KEY_1);
        InitKey(KEY_2);
        InitKey(KEY_3);
        InitKey(KEY_4);
        InitKey(KEY_5);
        InitKey(KEY_6);
        InitKey(KEY_7);
        InitKey(KEY_8);
        InitKey(KEY_9);
        InitKey2(KEY_SEMICOLON, ";");
        InitKey2(KEY_EQUAL, "=");
        InitKey(KEY_A);
        InitKey(KEY_B);
        InitKey(KEY_C);
        InitKey(KEY_D);
        InitKey(KEY_E);
        InitKey(KEY_F);
        InitKey(KEY_G);
        InitKey(KEY_H);
        InitKey(KEY_I);
        InitKey(KEY_J);
        InitKey(KEY_K);
        InitKey(KEY_L);
        InitKey(KEY_M);
        InitKey(KEY_N);
        InitKey(KEY_O);
        InitKey(KEY_P);
        InitKey(KEY_Q);
        InitKey(KEY_R);
        InitKey(KEY_S);
        InitKey(KEY_T);
        InitKey(KEY_U);
        InitKey(KEY_V);
        InitKey(KEY_W);
        InitKey(KEY_X);
        InitKey(KEY_Y);
        InitKey(KEY_Z);
        InitKey2(KEY_LEFT_BRACKET, "{");
        InitKey2(KEY_BACKSLASH, "\\");
        InitKey2(KEY_RIGHT_BRACKET, "}");
        InitKey2(KEY_GRAVE_ACCENT, "`");
        InitKey2(KEY_ESCAPE, "Escape");
        InitKey2(KEY_ENTER, "Enter");
        InitKey2(KEY_TAB, "Tab");
        InitKey2(KEY_BACKSPACE, "Backspace");
        InitKey2(KEY_INSERT, "Insert");
        InitKey2(KEY_DELETE, "Del");
        InitKey2(KEY_RIGHT, "Right");
        InitKey2(KEY_LEFT, "Left");
        InitKey2(KEY_DOWN, "Down");
        InitKey2(KEY_UP, "Up");
        InitKey2(KEY_PAGE_UP, "Page Up");
        InitKey2(KEY_PAGE_DOWN, "Page Down");
        InitKey2(KEY_HOME, "Home");
        InitKey2(KEY_END, "End");
        InitKey2(KEY_CAPS_LOCK, "Caps Lock");
        InitKey2(KEY_SCROLL_LOCK, "Scroll Lock");
        InitKey2(KEY_NUM_LOCK, "Num Lock");
        InitKey2(KEY_PRINT_SCREEN, "Print Screen");
        InitKey2(KEY_PAUSE, "Pause");
        InitKey(KEY_F1);
        InitKey(KEY_F2);
        InitKey(KEY_F3);
        InitKey(KEY_F4);
        InitKey(KEY_F5);
        InitKey(KEY_F6);
        InitKey(KEY_F7);
        InitKey(KEY_F8);
        InitKey(KEY_F9);
        InitKey(KEY_F10);
        InitKey(KEY_F11);
        InitKey(KEY_F12);
        InitKey(KEY_F13);
        InitKey(KEY_F14);
        InitKey(KEY_F15);
        InitKey(KEY_F16);
        InitKey(KEY_F17);
        InitKey(KEY_F18);
        InitKey(KEY_F19);
        InitKey(KEY_F20);
        InitKey(KEY_F21);
        InitKey(KEY_F22);
        InitKey(KEY_F23);
        InitKey(KEY_F24);
        InitKey2(KEY_KP_0, "Num 0");
        InitKey2(KEY_KP_1, "Num 1");
        InitKey2(KEY_KP_2, "Num 2");
        InitKey2(KEY_KP_3, "Num 3");
        InitKey2(KEY_KP_4, "Num 4");
        InitKey2(KEY_KP_5, "Num 5");
        InitKey2(KEY_KP_6, "Num 6");
        InitKey2(KEY_KP_7, "Num 7");
        InitKey2(KEY_KP_8, "Num 8");
        InitKey2(KEY_KP_9, "Num 9");
        InitKey2(KEY_KP_DECIMAL, "Num Decimal");
        InitKey2(KEY_KP_DIVIDE, "Num /");
        InitKey2(KEY_KP_MULTIPLY, "Num *");
        InitKey2(KEY_KP_SUBTRACT, "Num -");
        InitKey2(KEY_KP_ADD, "Num +");
        InitKey2(KEY_KP_ENTER, "Num Enter");
        InitKey2(KEY_KP_EQUAL, "Num =");
        InitKey2(KEY_LEFT_SHIFT, "L. Shift");
        InitKey2(KEY_LEFT_CONTROL, "L. Ctrl");
        InitKey2(KEY_LEFT_ALT, "L. Alt");
        InitKey2(KEY_LEFT_SUPER, "L. Super");
        InitKey2(KEY_RIGHT_SHIFT, "R. Shift");
        InitKey2(KEY_RIGHT_CONTROL, "R. Ctrl");
        InitKey2(KEY_RIGHT_ALT, "R. Alt");
        InitKey2(KEY_RIGHT_SUPER, "R. Super");
        InitKey2(KEY_MENU, "Menu");

        InitButton(MOUSE_BUTTON_LEFT, "LBM");
        InitButton(MOUSE_BUTTON_RIGHT, "RBM");
        InitButton(MOUSE_BUTTON_MIDDLE, "MBM");
        InitButton(MOUSE_BUTTON_4, "MB4");
        InitButton(MOUSE_BUTTON_5, "MB5");
        InitButton(MOUSE_BUTTON_6, "MB6");
        InitButton(MOUSE_BUTTON_7, "MB7");
        InitButton(MOUSE_BUTTON_8, "MB8");

        InitButton(MOUSE_WHEEL_UP, "Wheel Up");
        InitButton(MOUSE_WHEEL_DOWN, "Wheel Down");
        InitButton(MOUSE_WHEEL_LEFT, "Wheel Left");
        InitButton(MOUSE_WHEEL_RIGHT, "Wheel Right");

        InitMouseAxis(MOUSE_AXIS_X, "Mouse Axis X");
        InitMouseAxis(MOUSE_AXIS_Y, "Mouse Axis Y");

        InitDevice(ID_KEYBOARD);
        InitDevice(ID_MOUSE);
        InitDevice(ID_JOYSTICK_1);
        InitDevice(ID_JOYSTICK_2);
        InitDevice(ID_JOYSTICK_3);
        InitDevice(ID_JOYSTICK_4);
        InitDevice(ID_JOYSTICK_5);
        InitDevice(ID_JOYSTICK_6);
        InitDevice(ID_JOYSTICK_7);
        InitDevice(ID_JOYSTICK_8);
        InitDevice(ID_JOYSTICK_9);
        InitDevice(ID_JOYSTICK_10);
        InitDevice(ID_JOYSTICK_11);
        InitDevice(ID_JOYSTICK_12);
        InitDevice(ID_JOYSTICK_13);
        InitDevice(ID_JOYSTICK_14);
        InitDevice(ID_JOYSTICK_15);
        InitDevice(ID_JOYSTICK_16);

        InitJoyButton(JOY_BUTTON_1, "Joy Btn 1");
        InitJoyButton(JOY_BUTTON_2, "Joy Btn 2");
        InitJoyButton(JOY_BUTTON_3, "Joy Btn 3");
        InitJoyButton(JOY_BUTTON_4, "Joy Btn 4");
        InitJoyButton(JOY_BUTTON_5, "Joy Btn 5");
        InitJoyButton(JOY_BUTTON_6, "Joy Btn 6");
        InitJoyButton(JOY_BUTTON_7, "Joy Btn 7");
        InitJoyButton(JOY_BUTTON_8, "Joy Btn 8");
        InitJoyButton(JOY_BUTTON_9, "Joy Btn 9");
        InitJoyButton(JOY_BUTTON_10, "Joy Btn 10");
        InitJoyButton(JOY_BUTTON_11, "Joy Btn 11");
        InitJoyButton(JOY_BUTTON_12, "Joy Btn 12");
        InitJoyButton(JOY_BUTTON_13, "Joy Btn 13");
        InitJoyButton(JOY_BUTTON_14, "Joy Btn 14");
        InitJoyButton(JOY_BUTTON_15, "Joy Btn 15");
        InitJoyButton(JOY_BUTTON_16, "Joy Btn 16");
        InitJoyButton(JOY_BUTTON_17, "Joy Btn 17");
        InitJoyButton(JOY_BUTTON_18, "Joy Btn 18");
        InitJoyButton(JOY_BUTTON_19, "Joy Btn 19");
        InitJoyButton(JOY_BUTTON_20, "Joy Btn 20");
        InitJoyButton(JOY_BUTTON_21, "Joy Btn 21");
        InitJoyButton(JOY_BUTTON_22, "Joy Btn 22");
        InitJoyButton(JOY_BUTTON_23, "Joy Btn 23");
        InitJoyButton(JOY_BUTTON_24, "Joy Btn 24");
        InitJoyButton(JOY_BUTTON_25, "Joy Btn 25");
        InitJoyButton(JOY_BUTTON_26, "Joy Btn 26");
        InitJoyButton(JOY_BUTTON_27, "Joy Btn 27");
        InitJoyButton(JOY_BUTTON_28, "Joy Btn 28");
        InitJoyButton(JOY_BUTTON_29, "Joy Btn 29");
        InitJoyButton(JOY_BUTTON_30, "Joy Btn 30");
        InitJoyButton(JOY_BUTTON_31, "Joy Btn 31");
        InitJoyButton(JOY_BUTTON_32, "Joy Btn 32");

        InitJoyAxis(JOY_AXIS_1, "Joy Axis 1");
        InitJoyAxis(JOY_AXIS_2, "Joy Axis 2");
        InitJoyAxis(JOY_AXIS_3, "Joy Axis 3");
        InitJoyAxis(JOY_AXIS_4, "Joy Axis 4");
        InitJoyAxis(JOY_AXIS_5, "Joy Axis 5");
        InitJoyAxis(JOY_AXIS_6, "Joy Axis 6");
        InitJoyAxis(JOY_AXIS_7, "Joy Axis 7");
        InitJoyAxis(JOY_AXIS_8, "Joy Axis 8");
        InitJoyAxis(JOY_AXIS_9, "Joy Axis 9");
        InitJoyAxis(JOY_AXIS_10, "Joy Axis 10");
        InitJoyAxis(JOY_AXIS_11, "Joy Axis 11");
        InitJoyAxis(JOY_AXIS_12, "Joy Axis 12");
        InitJoyAxis(JOY_AXIS_13, "Joy Axis 13");
        InitJoyAxis(JOY_AXIS_14, "Joy Axis 14");
        InitJoyAxis(JOY_AXIS_15, "Joy Axis 15");
        InitJoyAxis(JOY_AXIS_16, "Joy Axis 16");
        InitJoyAxis(JOY_AXIS_17, "Joy Axis 17");
        InitJoyAxis(JOY_AXIS_18, "Joy Axis 18");
        InitJoyAxis(JOY_AXIS_19, "Joy Axis 19");
        InitJoyAxis(JOY_AXIS_20, "Joy Axis 20");
        InitJoyAxis(JOY_AXIS_21, "Joy Axis 21");
        InitJoyAxis(JOY_AXIS_22, "Joy Axis 22");
        InitJoyAxis(JOY_AXIS_23, "Joy Axis 23");
        InitJoyAxis(JOY_AXIS_24, "Joy Axis 24");
        InitJoyAxis(JOY_AXIS_25, "Joy Axis 25");
        InitJoyAxis(JOY_AXIS_26, "Joy Axis 26");
        InitJoyAxis(JOY_AXIS_27, "Joy Axis 27");
        InitJoyAxis(JOY_AXIS_28, "Joy Axis 28");
        InitJoyAxis(JOY_AXIS_29, "Joy Axis 29");
        InitJoyAxis(JOY_AXIS_30, "Joy Axis 30");
        InitJoyAxis(JOY_AXIS_31, "Joy Axis 31");
        InitJoyAxis(JOY_AXIS_32, "Joy Axis 32");

        InitModifier(KEY_MOD_SHIFT);
        InitModifier(KEY_MOD_CONTROL);
        InitModifier(KEY_MOD_ALT);
        InitModifier(KEY_MOD_SUPER);
        InitModifier(KEY_MOD_CAPS_LOCK);
        InitModifier(KEY_MOD_NUM_LOCK);

        InitController(CONTROLLER_PLAYER_1);
        InitController(CONTROLLER_PLAYER_2);
        InitController(CONTROLLER_PLAYER_3);
        InitController(CONTROLLER_PLAYER_4);
        InitController(CONTROLLER_PLAYER_5);
        InitController(CONTROLLER_PLAYER_6);
        InitController(CONTROLLER_PLAYER_7);
        InitController(CONTROLLER_PLAYER_8);
        InitController(CONTROLLER_PLAYER_9);
        InitController(CONTROLLER_PLAYER_10);
        InitController(CONTROLLER_PLAYER_11);
        InitController(CONTROLLER_PLAYER_12);
        InitController(CONTROLLER_PLAYER_13);
        InitController(CONTROLLER_PLAYER_14);
        InitController(CONTROLLER_PLAYER_15);
        InitController(CONTROLLER_PLAYER_16);

        Core::ZeroMem(JoystickAxisState.ToPtr(), sizeof(JoystickAxisState));
    }
};

static InputSystemStatic Static;

static bool ValidateDeviceKey(InputDeviceKey const& DeviceKey)
{
    if (DeviceKey.DeviceId >= MAX_INPUT_DEVICES)
    {
        LOG("ValidateDeviceKey: invalid device ID\n");
        return false;
    }

    if (DeviceKey.KeyId >= Static.DeviceButtonLimits[DeviceKey.DeviceId])
    {
        LOG("ValidateDeviceKey: invalid key ID\n");
        return false;
    }

    return true;
}

InputSystem::InputSystem()
{
    m_DeviceButtonDown[ID_KEYBOARD] = m_KeyboardButtonDown.ToPtr();
    m_DeviceButtonDown[ID_MOUSE] = m_MouseButtonDown.ToPtr();
    for (int i = 0; i < MAX_JOYSTICKS_COUNT; i++)
    {
        m_DeviceButtonDown[ID_JOYSTICK_1 + i] = m_JoystickButtonDown[i].ToPtr();

        Core::Memset(m_JoystickButtonDown[i].ToPtr(), 0xff, sizeof(m_JoystickButtonDown[0]));
    }

    Core::Memset(m_KeyboardButtonDown.ToPtr(), 0xff, sizeof(m_KeyboardButtonDown));
    Core::Memset(m_MouseButtonDown.ToPtr(), 0xff, sizeof(m_MouseButtonDown));

    m_MouseAxisState[0].Clear();
    m_MouseAxisState[1].Clear();
}

InputSystem::~InputSystem()
{
}

void InputSystem::SetInputMappings(InputMappings* Mappings)
{
    m_InputMappings = Mappings;
}

InputMappings* InputSystem::GetInputMappings() const
{
    return m_InputMappings;
}

void InputSystem::NewFrame()
{
    for (auto& actionPool : m_InputState.m_ActionPool)
        actionPool.Clear();

    for (auto& axisHash : m_InputState.m_AxisScale)
        for (auto& pair : axisHash)
            pair.second = 0.0f;
}

void InputSystem::Tick(float TimeStep)
{
    if (!m_InputMappings)
        return;

    auto& AxisScale = m_InputState.m_AxisScale;

    //bool bPaused = GetWorld()->IsPaused();

    //for (auto& it : m_AxisBindingsHash)
    //{
    //    it.second.AxisScale = 0.0f;
    //}

    for (PressedKey* key = m_PressedKeys.ToPtr(); key < &m_PressedKeys[m_NumPressedKeys]; key++)
    {
        if (key->BindingType == BINDING_TYPE::AXIS)
        {
            AxisScale[key->ControllerId][key->Binding] += key->AxisScale * TimeStep;
            //auto it = m_AxisBindingsHash.Find(key->Binding);
            //if (it != m_AxisBindingsHash.End())
            //    it->second.AxisScale += key->AxisScale * TimeStep;
        }
    }

    Float2 mouseDelta;

    if (in_MouseFilter)
        mouseDelta = (m_MouseAxisState[0] + m_MouseAxisState[1]) * 0.5f;
    else
        mouseDelta = m_MouseAxisState[m_MouseIndex];

    if (in_MouseInvertY)
        mouseDelta.Y = -mouseDelta.Y;

    float timeStepMsec     = Math::Max(TimeStep * 1000, 200);
    float mouseInputRate   = mouseDelta.Length() / timeStepMsec;
    float mouseCurrentSens = in_MouseSensitivity.GetFloat() + mouseInputRate * in_MouseAccel.GetFloat();
    float mouseSens[2]     = {in_MouseSensX.GetFloat() * mouseCurrentSens, in_MouseSensY.GetFloat() * mouseCurrentSens};

    for (auto mappingIt = m_InputMappings->GetAxisMappings().Begin(); mappingIt != m_InputMappings->GetAxisMappings().End(); mappingIt++)
    {
        //AxisBinding& binding = it.second;

        //if (bPaused && !binding.bExecuteEvenWhenPaused)
        //{
        //    continue;
        //}

        //auto mappingIt = lockedMappings->GetAxisMappings().Find(it.first);
        //if (mappingIt == lockedMappings->GetAxisMappings().End())
        //{
        //    continue;
        //}

        //auto& scale = AxisScale[mappingIt->first];

        //memset(AXIS, 0, sizeof(AXIS));

        //AXIS[ControllerId] += binding.AxisScale;

        auto const& axisMappings = mappingIt->second;
        for (auto const& mapping : axisMappings)
        {
            //if (mapping.ControllerId != ControllerId)
            //    continue;

            if (mapping.DeviceId == ID_MOUSE)
            {
                if (mapping.KeyId >= MOUSE_AXIS_BASE)
                {
                    int mouseAxis = mapping.KeyId - MOUSE_AXIS_BASE;
                    float delta = mouseDelta[mouseAxis];

                    if (delta)
                        AxisScale[mapping.ControllerId][mappingIt->first] += delta * (mapping.AxisScale * mouseSens[mouseAxis]);
                }
            }
            else if (mapping.DeviceId >= ID_JOYSTICK_1 && mapping.DeviceId <= ID_JOYSTICK_16)
            {
                int joyNum = mapping.DeviceId - ID_JOYSTICK_1;

                if (mapping.KeyId >= JOY_AXIS_BASE)
                {
                    int joystickAxis = mapping.KeyId - JOY_AXIS_BASE;
                    float delta = Static.JoystickAxisState[joyNum][joystickAxis];

                    if (delta)
                        AxisScale[mapping.ControllerId][mappingIt->first] += delta * mapping.AxisScale * TimeStep;
                }
            }
        }

        //if (AXIS[ControllerId] != 0)
        //{
        //    auto& axisState = m_InputState.m_ActiveAxes[ControllerId].Add();
        //    axisState.Name = it.first;
        //    axisState.Scale = AXIS[ControllerId];
        //}

        //binding.Callback(binding.AxisScale);
    }

    // Reset mouse axes
    m_MouseIndex ^= 1;
    m_MouseAxisState[m_MouseIndex].Clear();
}

void InputSystem::SetButtonState(InputDeviceKey const& DeviceKey, int Action, int ModMask)
{
    if (!ValidateDeviceKey(DeviceKey))
        return;

    if (DeviceKey.DeviceId == ID_KEYBOARD && DeviceKey.KeyId >= MAX_KEYBOARD_BUTTONS)
    {
        LOG("InputSystem::SetButtonState: Invalid key\n");
        return;
    }
    if (DeviceKey.DeviceId == ID_MOUSE && DeviceKey.KeyId >= MAX_MOUSE_BUTTONS)
    {
        LOG("InputSystem::SetButtonState: Invalid mouse button\n");
        return;
    }
    if (DeviceKey.DeviceId >= ID_JOYSTICK_1 && DeviceKey.DeviceId <= ID_JOYSTICK_16 && DeviceKey.KeyId >= MAX_JOYSTICK_BUTTONS)
    {
        LOG("InputSystem::SetButtonState: Invalid joystick button\n");
        return;
    }

    if (DeviceKey.DeviceId == ID_KEYBOARD && bIgnoreKeyboardEvents)
    {
        return;
    }
    if (DeviceKey.DeviceId == ID_MOUSE && bIgnoreMouseEvents)
    {
        return;
    }
    if (DeviceKey.DeviceId >= ID_JOYSTICK_1 && DeviceKey.DeviceId <= ID_JOYSTICK_16 && bIgnoreJoystickEvents)
    {
        return;
    }

    int8_t* buttonIndex = m_DeviceButtonDown[DeviceKey.DeviceId];

    //TCallback<void()> callback;

    if (Action == IA_PRESS)
    {
        if (buttonIndex[DeviceKey.KeyId] == -1)
        {
            if (m_NumPressedKeys < MAX_PRESSED_KEYS)
            {
                PressedKey& pressedKey = m_PressedKeys[m_NumPressedKeys];
                pressedKey.DeviceId      = DeviceKey.DeviceId;
                pressedKey.Key           = DeviceKey.KeyId;
                pressedKey.Unbind();

                if (m_InputMappings)
                {
                    auto it = m_InputMappings->GetMappings().Find(DeviceKey);

                    if (it != m_InputMappings->GetMappings().End())
                    {
                        auto const& mappings = it->second;

                        bool bUseActionMapping = false;

                        // Find action mapping with modifiers
                        for (InputMappings::Mapping const& mapping : mappings)
                        {
                            //if (mapping.ControllerId != ControllerId)
                            //{
                            //    continue;
                            //}
                            if (mapping.bAxis)
                            {
                                continue;
                            }
                            // Filter by key modifiers
                            if (ModMask != mapping.ModMask)
                            {
                                continue;
                            }

                            pressedKey.BindAction(mapping.Name);
                            pressedKey.ControllerId = mapping.ControllerId;
                            bUseActionMapping        = true;
                            break;
                        }

                        // Find action without modifiers
                        if (!bUseActionMapping)
                        {
                            for (InputMappings::Mapping const& mapping : mappings)
                            {
                                //if (mapping.ControllerId != ControllerId)
                                //{
                                //    continue;
                                //}
                                if (mapping.bAxis)
                                {
                                    continue;
                                }
                                // Filter by key modifiers
                                if (0 != mapping.ModMask)
                                {
                                    continue;
                                }

                                pressedKey.BindAction(mapping.Name);
                                pressedKey.ControllerId = mapping.ControllerId;
                                bUseActionMapping        = true;
                                break;
                            }
                        }

                        // Find axis mapping
                        if (!bUseActionMapping)
                        {
                            for (InputMappings::Mapping const& mapping : mappings)
                            {
                                //if (mapping.ControllerId != ControllerId)
                                //{
                                //    continue;
                                //}
                                if (!mapping.bAxis)
                                {
                                    continue;
                                }

                                pressedKey.BindAxis(mapping.Name, mapping.AxisScale);
                                pressedKey.ControllerId = mapping.ControllerId;
                                break;
                            }
                        }
                    }
                }

                buttonIndex[DeviceKey.KeyId] = m_NumPressedKeys;

                m_NumPressedKeys++;

                if (pressedKey.BindingType == BINDING_TYPE::ACTION)
                {
                    auto& action = m_InputState.m_ActionPool[pressedKey.ControllerId].Add();
                    action.Name = pressedKey.Binding;
                    action.bPressed = true;
                    //auto it = m_ActionBindingsHash.Find(pressedKey.Binding);
                    //if (it != m_ActionBindingsHash.End())
                    //{
                    //    ActionBinding& binding = it->second;
                    //    //if (GetWorld()->IsPaused() && !binding.bExecuteEvenWhenPaused)
                    //    //{
                    //    //    pressedKey.Unbind();
                    //    //}
                    //    //else
                    //    {
                    //        callback = binding.Callback[IA_PRESS];
                    //    }
                    //}
                }
            }
            else
            {
                LOG("MAX_PRESSED_KEYS hit\n");
            }
        }
        else
        {
            // Button is repressed

            //HK_ASSERT( 0 );
        }
    }
    else if (Action == IA_RELEASE)
    {
        if (buttonIndex[DeviceKey.KeyId] != -1)
        {
            int index = buttonIndex[DeviceKey.KeyId];
            PressedKey& pressedKey = m_PressedKeys[index];

            if (pressedKey.BindingType == BINDING_TYPE::ACTION)
            {
                auto& action = m_InputState.m_ActionPool[pressedKey.ControllerId].Add();
                action.Name = pressedKey.Binding;
                action.bPressed = false;

                //auto it = m_ActionBindingsHash.Find(pressedKey.Binding);
                //if (it != m_ActionBindingsHash.End())
                //{
                //    callback = it->second.Callback[IA_RELEASE];
                //}
            }

            m_DeviceButtonDown[pressedKey.DeviceId][pressedKey.Key] = -1;

            if (index != m_NumPressedKeys - 1)
            {
                // Move last array element to position "index"
                m_PressedKeys[index] = m_PressedKeys[m_NumPressedKeys - 1];

                // Refresh index of moved element
                m_DeviceButtonDown[m_PressedKeys[index].DeviceId][m_PressedKeys[index].Key] = index;
            }

            // Pop back
            m_NumPressedKeys--;
            HK_ASSERT(m_NumPressedKeys >= 0);
        }
    }

    //callback();
}

bool InputSystem::GetButtonState(InputDeviceKey const& DeviceKey) const
{
    if (!ValidateDeviceKey(DeviceKey))
        return false;

    if (DeviceKey.DeviceId == ID_KEYBOARD && DeviceKey.KeyId >= MAX_KEYBOARD_BUTTONS)
    {
        LOG("InputSystem::GetButtonState: Invalid key\n");
        return false;
    }
    if (DeviceKey.DeviceId == ID_MOUSE && DeviceKey.KeyId >= MAX_MOUSE_BUTTONS)
    {
        LOG("InputSystem::GetButtonState: Invalid mouse button\n");
        return false;
    }
    if (DeviceKey.DeviceId >= ID_JOYSTICK_1 && DeviceKey.DeviceId <= ID_JOYSTICK_16 && DeviceKey.KeyId >= MAX_JOYSTICK_BUTTONS)
    {
        LOG("InputSystem::GetButtonState: Invalid joystick button\n");
        return false;
    }

    return m_DeviceButtonDown[DeviceKey.DeviceId][DeviceKey.KeyId] != -1;
}

void InputSystem::UnpressButtons()
{
    for (uint16_t i = 0; i < MAX_KEYBOARD_BUTTONS; i++)
    {
        SetButtonState({ID_KEYBOARD, i}, IA_RELEASE, 0);
    }
    for (uint16_t i = 0; i < MAX_MOUSE_BUTTONS; i++)
    {
        SetButtonState({ID_MOUSE, i}, IA_RELEASE, 0);
    }
    for (uint16_t j = 0; j < MAX_JOYSTICKS_COUNT; j++)
    {
        for (uint16_t i = 0; i < MAX_JOYSTICK_BUTTONS; i++)
        {
            SetButtonState({uint16_t(ID_JOYSTICK_1 + j), i}, IA_RELEASE, 0);
        }
    }
}

bool InputSystem::IsJoyDown(int JoystickId, uint16_t Button) const
{
    return GetButtonState({uint16_t(ID_JOYSTICK_1 + JoystickId), Button});
}

void InputSystem::NotifyUnicodeCharacter(WideChar UnicodeCharacter, int ModMask)
{
    if (bIgnoreCharEvents)
        return;

    //if (GetWorld()->IsPaused() && !m_bCharacterCallbackExecuteEvenWhenPaused)
    //    return;
 
    m_CharacterCallback.Invoke(UnicodeCharacter, ModMask);
}

void InputSystem::SetMouseAxisState(float X, float Y)
{
    if (bIgnoreMouseEvents)
    {
        return;
    }

    m_MouseAxisState[m_MouseIndex].X += X;
    m_MouseAxisState[m_MouseIndex].Y += Y;
}

float InputSystem::GetMouseAxisState(int Axis)
{
    if (Axis < 0 || Axis > 1)
    {
        LOG("InputSystem::GetMouseAxisState: Invalid mouse axis num\n");
        return 0.0f;
    }
    return m_MouseAxisState[m_MouseIndex][Axis];
}

void InputSystem::SetJoystickAxisState(int Joystick, int Axis, float Value)
{
    if (Joystick < 0 || Joystick >= MAX_JOYSTICKS_COUNT)
    {
        LOG("InputSystem::SetJoystickAxisState: Invalid joystick num\n");
        return;
    }
    if (Axis < 0 || Axis >= MAX_JOYSTICK_AXES)
    {
        LOG("InputSystem::SetJoystickAxisState: Invalid joystick axis num\n");
        return;
    }
    Static.JoystickAxisState[Joystick][Axis] = Value;
}

float InputSystem::GetJoystickAxisState(int Joystick, int Axis)
{
    if (Joystick < 0 || Joystick >= MAX_JOYSTICKS_COUNT)
    {
        LOG("InputSystem::GetJoystickAxisState: Invalid joystick num\n");
        return 0.0f;
    }
    if (Axis < 0 || Axis >= MAX_JOYSTICK_AXES)
    {
        LOG("InputSystem::GetJoystickAxisState: Invalid joystick axis num\n");
        return 0.0f;
    }
    return Static.JoystickAxisState[Joystick][Axis];
}

void InputSystem::SetCharacterCallback(Delegate<void(WideChar, int)> Callback, bool bExecuteEvenWhenPaused)
{
    m_CharacterCallback = std::move(Callback);
    m_bCharacterCallbackExecuteEvenWhenPaused = bExecuteEvenWhenPaused;
}

void InputSystem::UnsetCharacterCallback()
{
    m_CharacterCallback.Clear();
}

const char* InputHelper::TranslateDevice(uint16_t DeviceId)
{
    if (DeviceId >= MAX_INPUT_DEVICES)
    {
        return "UNKNOWN";
    }
    return Static.DeviceNames[DeviceId];
}

const char* InputHelper::TranslateModifier(int Modifier)
{
    if (Modifier < 0 || Modifier > KEY_MOD_LAST)
    {
        return "UNKNOWN";
    }
    return Static.ModifierNames[Modifier];
}

const char* InputHelper::TranslateDeviceKey(InputDeviceKey const& DeviceKey)
{
    switch (DeviceKey.DeviceId)
    {
        case ID_KEYBOARD:
            if (DeviceKey.KeyId > KEY_LAST)
            {
                return "UNKNOWN";
            }
            return Static.KeyNames[DeviceKey.KeyId];
        case ID_MOUSE:
            if (DeviceKey.KeyId >= MOUSE_AXIS_BASE)
            {
                if (DeviceKey.KeyId > MOUSE_AXIS_LAST)
                {
                    return "UNKNOWN";
                }
                return Static.MouseAxisNames[DeviceKey.KeyId - MOUSE_AXIS_BASE];
            }
            if (DeviceKey.KeyId < MOUSE_BUTTON_BASE || DeviceKey.KeyId > MOUSE_BUTTON_LAST)
            {
                return "UNKNOWN";
            }
            return Static.MouseButtonNames[DeviceKey.KeyId - MOUSE_BUTTON_BASE];
    }
    if (DeviceKey.DeviceId >= ID_JOYSTICK_1 && DeviceKey.DeviceId <= ID_JOYSTICK_16)
    {
        if (DeviceKey.KeyId >= JOY_AXIS_BASE)
        {
            if (DeviceKey.KeyId > JOY_AXIS_LAST)
            {
                return "UNKNOWN";
            }
            return Static.JoystickAxisNames[DeviceKey.KeyId - JOY_AXIS_BASE];
        }

        if (DeviceKey.KeyId < JOY_BUTTON_BASE || DeviceKey.KeyId > JOY_BUTTON_LAST)
        {
            return "UNKNOWN";
        }
        return Static.JoystickButtonNames[DeviceKey.KeyId - JOY_BUTTON_BASE];
    }
    return "UNKNOWN";
}

const char* InputHelper::TranslateController(int ControllerId)
{
    if (ControllerId < 0 || ControllerId >= MAX_INPUT_CONTROLLERS)
    {
        return "UNKNOWN";
    }
    return Static.ControllerNames[ControllerId];
}

uint16_t InputHelper::LookupDevice(StringView Device)
{
    for (uint16_t i = 0; i < MAX_INPUT_DEVICES; i++)
    {
        if (!Device.Icmp(Static.DeviceNames[i]))
        {
            return i;
        }
    }
    return ID_UNDEFINED;
}

int InputHelper::LookupModifier(StringView Modifier)
{
    for (int i = 0; i < MAX_MODIFIERS; i++)
    {
        if (!Modifier.Icmp(Static.ModifierNames[i]))
        {
            return i;
        }
    }
    return -1;
}

uint16_t InputHelper::LookupDeviceKey(uint16_t DeviceId, StringView Key)
{
    switch (DeviceId)
    {
        case ID_KEYBOARD:
            for (int i = 0; i < MAX_KEYBOARD_BUTTONS; i++)
            {
                if (!Key.Icmp(Static.KeyNames[i]))
                {
                    return i;
                }
            }
            return KEY_UNDEFINED;
        case ID_MOUSE:
            for (int i = 0; i < MAX_MOUSE_BUTTONS; i++)
            {
                if (!Key.Icmp(Static.MouseButtonNames[i]))
                {
                    return MOUSE_BUTTON_BASE + i;
                }
            }
            for (int i = 0; i < MAX_MOUSE_AXES; i++)
            {
                if (!Key.Icmp(Static.MouseAxisNames[i]))
                {
                    return MOUSE_AXIS_BASE + i;
                }
            }
            return KEY_UNDEFINED;
    }
    if (DeviceId >= ID_JOYSTICK_1 && DeviceId <= ID_JOYSTICK_16)
    {
        for (int i = 0; i < MAX_JOYSTICK_BUTTONS; i++)
        {
            if (!Key.Icmp(Static.JoystickButtonNames[i]))
            {
                return JOY_BUTTON_BASE + i;
            }
        }
        for (int i = 0; i < MAX_JOYSTICK_AXES; i++)
        {
            if (!Key.Icmp(Static.JoystickAxisNames[i]))
            {
                return JOY_AXIS_BASE + i;
            }
        }
    }
    return KEY_UNDEFINED;
}

int InputHelper::LookupController(StringView Controller)
{
    for (int i = 0; i < MAX_INPUT_CONTROLLERS; i++)
    {
        if (!Controller.Icmp(Static.ControllerNames[i]))
        {
            return i;
        }
    }
    return -1;
}


#if 0
void InputMappings::InitializeFromDocument(DocumentValue const& Document)
{
    UnmapAll();

    DocumentMember const* mAxes = Document.FindMember("Axes");
    if (mAxes)
    {
        for (DocumentValue const* mAxis = mAxes->GetArrayValues(); mAxis; mAxis = mAxis->GetNext())
        {
            if (!mAxis->IsObject())
            {
                continue;
            }

            DocumentMember const* mName = mAxis->FindMember("Name");
            if (!mName)
            {
                continue;
            }

            DocumentMember const* mDevice = mAxis->FindMember("Device");
            if (!mDevice)
            {
                continue;
            }

            DocumentMember const* mKey = mAxis->FindMember("Key");
            if (!mKey)
            {
                continue;
            }

            DocumentMember const* mController = mAxis->FindMember("Controller");
            if (!mController)
            {
                continue;
            }

            auto name = mName->GetStringView();
            auto device = mDevice->GetStringView();
            auto key = mKey->GetStringView();
            auto controller = mController->GetStringView();

            uint16_t deviceId = InputHelper::LookupDevice(device);
            uint16_t deviceKey = InputHelper::LookupDeviceKey(deviceId, key);
            int controllerId = InputHelper::LookupController(controller);

            float fScale = mAxis->GetFloat("Scale", 1.0f);

            MapAxis(name, {deviceId, deviceKey}, fScale, controllerId);
        }
    }

    DocumentMember const* mActions = Document.FindMember("Actions");
    if (mActions)
    {
        for (DocumentValue const* mAction = mActions->GetArrayValues(); mAction; mAction = mAction->GetNext())
        {
            if (!mAction->IsObject())
            {
                continue;
            }

            DocumentMember const* mName = mAction->FindMember("Name");
            if (!mName)
            {
                continue;
            }

            DocumentMember const* mDevice = mAction->FindMember("Device");
            if (!mDevice)
            {
                continue;
            }

            DocumentMember const* mKey = mAction->FindMember("Key");
            if (!mKey)
            {
                continue;
            }

            DocumentMember const* mController = mAction->FindMember("Controller");
            if (!mController)
            {
                continue;
            }

            int32_t modMask = mAction->GetInt32("ModMask", 0);

            auto name = mName->GetStringView();
            auto device = mDevice->GetStringView();
            auto key = mKey->GetStringView();
            auto controller = mController->GetStringView();

            uint16_t deviceId = InputHelper::LookupDevice(device);
            uint16_t deviceKey = InputHelper::LookupDeviceKey(deviceId, key);
            int controllerId = InputHelper::LookupController(controller);

            MapAction(name, {deviceId, deviceKey}, modMask, controllerId);
        }
    }
}
#endif

void InputMappings::MapAxis(StringView AxisName, InputDeviceKey const& DeviceKey, float AxisScale, int ControllerId)
{
    if (!ValidateDeviceKey(DeviceKey))
        return;

    UnmapAxis(DeviceKey);

    Mapping mapping;
    mapping.Name = AxisName;
    mapping.NameHash = mapping.Name.HashCaseInsensitive();
    mapping.bAxis = true;
    mapping.AxisScale = AxisScale;
    mapping.ControllerId = ControllerId;
    m_Mappings[DeviceKey].Add(std::move(mapping));

    AxisMapping axisMapping;
    axisMapping.DeviceId = DeviceKey.DeviceId;
    axisMapping.KeyId = DeviceKey.KeyId;
    axisMapping.ControllerId = ControllerId;
    axisMapping.AxisScale = AxisScale;
    m_AxisMappings[AxisName].Add(axisMapping);
}

void InputMappings::UnmapAxis(InputDeviceKey const& DeviceKey)
{
    if (!ValidateDeviceKey(DeviceKey))
        return;

    auto& keyMappings = m_Mappings[DeviceKey];

    for (auto it = keyMappings.begin(); it != keyMappings.end();)
    {
        Mapping& mapping = *it;

        if (mapping.bAxis)
        {
            auto map_it = m_AxisMappings.Find(mapping.Name);
            HK_ASSERT(map_it != m_AxisMappings.End());

            auto& axisMappingsVector = map_it->second;
            for (auto axis_it = axisMappingsVector.Begin(); axis_it != axisMappingsVector.End(); axis_it++)
            {
                if (axis_it->DeviceId == DeviceKey.DeviceId && axis_it->KeyId == DeviceKey.KeyId)
                {
                    axisMappingsVector.Erase(axis_it);
                    break;
                }
            }
            if (axisMappingsVector.IsEmpty())
            {
                m_AxisMappings.Erase(map_it);
            }
            it = keyMappings.Erase(it);
        }
        else
        {
            it++;
        }
    }
}

void InputMappings::MapAction(StringView ActionName, InputDeviceKey const& DeviceKey, int ModMask, int ControllerId)
{
    if (!ValidateDeviceKey(DeviceKey))
        return;

    if (DeviceKey.DeviceId >= ID_JOYSTICK_1 && DeviceKey.DeviceId <= ID_JOYSTICK_16 && DeviceKey.KeyId >= JOY_AXIS_BASE)
    {
        LOG("Cannot map joystick axis and action\n");
        return;
    }

    if (DeviceKey.DeviceId == ID_MOUSE && DeviceKey.KeyId >= MOUSE_AXIS_BASE)
    {
        LOG("Cannot map mouse axis and action\n");
        return;
    }

    UnmapAction(DeviceKey, ModMask);

    Mapping mapping;
    mapping.Name = ActionName;
    mapping.NameHash = mapping.Name.HashCaseInsensitive();
    mapping.bAxis = false;
    mapping.AxisScale = 0;
    mapping.ControllerId = ControllerId;
    mapping.ModMask = ModMask & 0xff;
    m_Mappings[DeviceKey].Add(std::move(mapping));
}

void InputMappings::UnmapAction(InputDeviceKey const& DeviceKey, int ModMask)
{
    if (!ValidateDeviceKey(DeviceKey))
        return;

    auto& keyMappings = m_Mappings[DeviceKey];

    for (auto it = keyMappings.begin(); it != keyMappings.end();)
    {
        Mapping& mapping = *it;

        if (!mapping.bAxis && mapping.ModMask == ModMask)
        {
            it = keyMappings.Erase(it);
        }
        else
        {
            it++;
        }
    }
}

void InputMappings::UnmapAll()
{
    m_Mappings.Clear();
    m_AxisMappings.Clear();
}

HK_NAMESPACE_END
