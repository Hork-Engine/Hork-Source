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

#include <Hork/Runtime/World/ComponentBinding.h>
#include <Hork/Runtime/GameApplication/InputSystem.h>

HK_NAMESPACE_BEGIN

enum class ExecuteMode : uint8_t
{
    Default,
    ExecuteEvenWhenPaused
};

class InputBindings
{
    friend class InputInterface;

public:
    void            Clear();

    template <typename T>
    void            BindAxis(StringView name, T* component, void (T::*method)(float), ExecuteMode mode = ExecuteMode::Default);

    template <typename T>
    void            BindAction(StringView name, T* component, void (T::*method)(), InputEvent event, ExecuteMode mode = ExecuteMode::Default);

    template <typename T>
    void            BindCharacterInput(T* component, void (T::*method)(WideChar, KeyModifierMask), ExecuteMode mode = ExecuteMode::Default);
    void            UnbindCharacterInput();

private:
    struct Binding
    {
        ComponentBinding<void(float)> AxisBinding;
        ComponentBinding<void()>      ActionBinding[2];
        bool                          ExecuteEvenWhenPaused[2];
    };

    HashMap<StringID, Binding>        m_Bindings;

    ComponentBinding<void(WideChar, KeyModifierMask)> m_CharacterCallback;
    bool                              m_CharacterCallbackExecuteEvenWhenPaused = false;
};

HK_INLINE void InputBindings::Clear()
{
    m_Bindings.Clear();
}

template <typename T>
HK_INLINE void InputBindings::BindAxis(StringView name, T* component, void (T::*method)(float), ExecuteMode mode)
{
    auto& binding = m_Bindings[StringID(name)];
    binding.AxisBinding.Bind(component, method);
    binding.ActionBinding[0].Clear();
    binding.ActionBinding[1].Clear();
    binding.ExecuteEvenWhenPaused[0] = mode == ExecuteMode::ExecuteEvenWhenPaused;
}

template <typename T>
HK_INLINE void InputBindings::BindAction(StringView name, T* component, void (T::*method)(), InputEvent event, ExecuteMode mode)
{
    auto& binding = m_Bindings[StringID(name)];
    binding.AxisBinding.Clear();
    binding.ActionBinding[ToUnderlying(event)].Bind(component, method);
    binding.ExecuteEvenWhenPaused[ToUnderlying(event)] = mode == ExecuteMode::ExecuteEvenWhenPaused;
}

template <typename T>
HK_INLINE void InputBindings::BindCharacterInput(T* component, void (T::*method)(WideChar, KeyModifierMask), ExecuteMode mode)
{
    m_CharacterCallback.Bind(component, method);
    m_CharacterCallbackExecuteEvenWhenPaused = mode == ExecuteMode::ExecuteEvenWhenPaused;
}

HK_INLINE void InputBindings::UnbindCharacterInput()
{
    m_CharacterCallback.Clear();
}

HK_NAMESPACE_END
