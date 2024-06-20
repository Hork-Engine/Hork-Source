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

#include "InputMappings.h"

HK_NAMESPACE_BEGIN

namespace
{
    uint32_t MakeHash(VirtualKeyOrAxis keyOrAxis, uint32_t modMask)
    {
        return HashTraits::Hash(uint32_t(keyOrAxis.GetData()) | (modMask << 16));
    }

    uint32_t MakeHash(GamepadKeyOrAxis keyOrAxis, PlayerController player)
    {
        return HashTraits::Hash(uint32_t(keyOrAxis.GetData()) | (uint32_t(player) << 16));
    }
}

void InputMappings::Clear()
{
    m_VirtMapping.Clear();
}

void InputMappings::MapAxis(PlayerController owner, StringView name, VirtualKeyOrAxis keyOrAxis, float power)
{
    VirtualMapping& mapping = m_VirtMapping[MakeHash(keyOrAxis, 0)];

    mapping.Name.FromString(name);
    mapping.IsAction = false;
    mapping.Power = power;
    mapping.Owner = owner;
}

void InputMappings::MapAction(PlayerController owner, StringView name, VirtualKey key, KeyModifierMask modMask)
{
    VirtualMapping& mapping = m_VirtMapping[MakeHash(key, modMask.m_Flags)];

    mapping.Name.FromString(name);
    mapping.IsAction = true;
    mapping.Power = 0;
    mapping.Owner = owner;
}

bool InputMappings::GetMapping(VirtualKeyOrAxis keyOrAxis, KeyModifierMask modMask, VirtualMapping& virtMapping)
{
    auto it = m_VirtMapping.find(MakeHash(keyOrAxis, modMask.m_Flags));
    if (it == m_VirtMapping.end())
        return false;
    virtMapping = it->second;
    return true;
}

void InputMappings::MapGamepadAxis(PlayerController owner, StringView name, GamepadKeyOrAxis keyOrAxis, float power)
{
    VirtualMapping& mapping = m_GamepadMapping[MakeHash(keyOrAxis, owner)];

    mapping.Name.FromString(name);
    mapping.IsAction = false;
    mapping.Power = power;
    mapping.Owner = owner;
}

void InputMappings::MapGamepadAction(PlayerController owner, StringView name, GamepadKeyOrAxis keyOrAxis)
{
    VirtualMapping& mapping = m_GamepadMapping[MakeHash(keyOrAxis, owner)];

    mapping.Name.FromString(name);
    mapping.IsAction = true;
    mapping.Power = 0;
    mapping.Owner = owner;
}

bool InputMappings::GetGamepadMapping(PlayerController owner, GamepadKeyOrAxis keyOrAxis, VirtualMapping& virtMapping)
{
    auto it = m_GamepadMapping.find(MakeHash(keyOrAxis, owner));
    if (it == m_GamepadMapping.end())
        return false;
    virtMapping = it->second;
    return true;
}

HK_NAMESPACE_END
