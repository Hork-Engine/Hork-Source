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

void InputMappings::Clear()
{
    m_VirtMapping.Clear();
}

void InputMappings::MapAxis(StringView name, VirtualKeyOrAxis virtualKey, float power, PlayerController owner)
{
    VirtualMapping& mapping = m_VirtMapping[VirtualInput{virtualKey, 0}];

    mapping.Name.FromString(name);
    mapping.IsAction = false;
    mapping.Power = power;
    mapping.Owner = owner;
}

void InputMappings::MapAction(StringView name, VirtualKeyOrAxis virtualKey, KeyModifierMask modMask, PlayerController owner)
{
    VirtualMapping& mapping = m_VirtMapping[VirtualInput{virtualKey, modMask.m_Flags}];

    mapping.Name.FromString(name);
    mapping.IsAction = true;
    mapping.Power = 0;
    mapping.Owner = owner;
}

bool InputMappings::GetMapping(VirtualKeyOrAxis virtualKey, KeyModifierMask modMask, VirtualMapping& virtMapping)
{
    auto it = m_VirtMapping.find(VirtualInput{virtualKey, modMask.m_Flags});
    if (it == m_VirtMapping.end())
        return false;
    virtMapping = it->second;
    return true;
}

HK_NAMESPACE_END
