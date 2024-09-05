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

#include <Hork/Core/ConsoleVar.h>
#include <Hork/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void InputInterface::Initialize()
{
    TickFunction tickFunc;
    tickFunc.Desc.Name.FromString("Update Input");
    tickFunc.Desc.TickEvenWhenPaused = true;
    tickFunc.Group = TickGroup::Update;
    tickFunc.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
    tickFunc.Delegate.Bind(this, &InputInterface::Update);
    RegisterTickFunction(tickFunc);
}

void InputInterface::Deinitialize()
{}

void InputInterface::Update()
{
    if (m_IsActive)
    {
        auto& actionPool = GameApplication::sGetInputSystem().GetActionPool();
        for (auto& action : actionPool)
        {
            InvokeAction(action.Name, action.IsPressed ? InputEvent::OnPress : InputEvent::OnRelease, action.Owner);
        }

        auto& axisPool = GameApplication::sGetInputSystem().GetAxisPool();
        for (auto& axis : axisPool)
        {
            InvokeAxis(axis.Name, axis.Amount, axis.Owner);
        }

        auto& chars = GameApplication::sGetInputSystem().GetChars();
        for (auto& ch : chars)
        {
            for (int playerIndex = 0; playerIndex < int(PlayerController::MAX_PLAYER_CONTROLLERS); ++playerIndex)
            {
                if (m_Bindings[playerIndex].m_CharacterCallbackExecuteEvenWhenPaused || !GetWorld()->GetTick().IsPaused)
                    m_Bindings[playerIndex].m_CharacterCallback.Invoke(GetWorld(), ch.Char, ch.ModMask);
            }
        }
    }
}

void InputInterface::InvokeAction(StringID name, InputEvent event, PlayerController player)
{
    if (player >= PlayerController::MAX_PLAYER_CONTROLLERS)
    {
        LOG("MAX_PLAYER_CONTROLLERS hit\n");
        return;
    }

    auto& bindings = m_Bindings[ToUnderlying(player)].m_Bindings;
    auto it = bindings.Find(name);
    if (it == bindings.End())
        return;

    auto& binding = it->second;
    if (!binding.ExecuteEvenWhenPaused[ToUnderlying(event)] && GetWorld()->GetTick().IsPaused)
        return;

    binding.ActionBinding[ToUnderlying(event)].Invoke(GetWorld());
}

void InputInterface::InvokeAxis(StringID name, float amount, PlayerController player)
{
    if (player >= PlayerController::MAX_PLAYER_CONTROLLERS)
    {
        LOG("MAX_PLAYER_CONTROLLERS hit\n");
        return;
    }

    auto& bindings = m_Bindings[ToUnderlying(player)].m_Bindings;
    auto it = bindings.Find(name);
    if (it == bindings.End())
        return;

    auto& binding = it->second;
    if (!binding.ExecuteEvenWhenPaused[0] && GetWorld()->GetTick().IsPaused)
        return;

    binding.AxisBinding.Invoke(GetWorld(), amount);
}

void InputInterface::UnbindAll(PlayerController player)
{
    if (player >= PlayerController::MAX_PLAYER_CONTROLLERS)
    {
        LOG("MAX_PLAYER_CONTROLLERS hit\n");
        return;
    }

    m_Bindings[ToUnderlying(player)].Clear();
}

HK_NAMESPACE_END
