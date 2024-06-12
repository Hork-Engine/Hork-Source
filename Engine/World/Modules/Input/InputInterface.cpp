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

#include "InputInterface.h"

#include <Engine/Core/ConsoleVar.h>
#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void InputInterface::Initialize()
{
    TickFunction f;
    f.Desc.Name.FromString("Update Input");
    f.Desc.TickEvenWhenPaused = true;
    f.Group = TickGroup::Update;
    f.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
    f.Delegate.Bind(this, &InputInterface::Update);
    RegisterTickFunction(f);
}

void InputInterface::Deinitialize()
{}

void InputInterface::Update()
{
    if (m_IsActive)
    {
        auto& actionPool = GameApplication::GetInputSystem().GetActionPool();
        for (auto& action : actionPool)
        {
            InvokeAction(action.Name, action.IsPressed ? InputEvent::OnPress : InputEvent::OnRelease, action.Owner);
        }

        auto& axisPool = GameApplication::GetInputSystem().GetAxisPool();
        for (auto& axis : axisPool)
        {
            InvokeAxis(axis.Name, axis.Power, axis.Owner);
        }

        auto& chars = GameApplication::GetInputSystem().GetChars();
        for (auto& ch : chars)
        {
            for (int controller = 0; controller < int(PlayerController::MAX_PLAYER_CONTROLLERS); ++controller)
            {
                if (m_Bindings[controller].m_CharacterCallbackExecuteEvenWhenPaused || !GetWorld()->GetTick().IsPaused)
                    m_Bindings[controller].m_CharacterCallback.Invoke(GetWorld(), ch.Char, ch.ModMask);
            }
        }
    }
}

void InputInterface::InvokeAction(StringID name, InputEvent event, PlayerController controller)
{
    auto& bindings = m_Bindings[ToUnderlying(controller)].m_Bindings;
    auto it = bindings.Find(name);
    if (it == bindings.End())
        return;

    auto& binding = it->second;
    if (!binding.ExecuteEvenWhenPaused[ToUnderlying(event)] && GetWorld()->GetTick().IsPaused)
        return;

    binding.ActionBinding[ToUnderlying(event)].Invoke(GetWorld());
}

void InputInterface::InvokeAxis(StringID name, float power, PlayerController controller)
{
    auto& bindings = m_Bindings[ToUnderlying(controller)].m_Bindings;
    auto it = bindings.Find(name);
    if (it == bindings.End())
        return;

    auto& binding = it->second;
    if (!binding.ExecuteEvenWhenPaused[0] && GetWorld()->GetTick().IsPaused)
        return;

    binding.AxisBinding.Invoke(GetWorld(), power);
}

void InputInterface::UnbindAll(PlayerController controller)
{
    m_Bindings[ToUnderlying(controller)].Clear();
}

HK_NAMESPACE_END
