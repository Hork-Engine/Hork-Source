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

#include <Hork/Core/StringID.h>
#include <Hork/Core/Containers/Hash.h>
#include <Hork/Core/Ref.h>

#include "VirtualKey.h"

HK_NAMESPACE_BEGIN

enum class PlayerController : uint8_t
{
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,

    MAX_PLAYER_CONTROLLERS
};

struct VirtualMapping
{
    StringID            Name;
    bool                IsAction;
    float               Power;
    PlayerController    Owner;
};

class InputMappings : public RefCounted
{
public:
    void                    Clear();

    void                    MapAxis(PlayerController owner, StringView name, VirtualKeyOrAxis keyOrAxis, float power = 1);

    void                    MapAction(PlayerController owner, StringView name, VirtualKey key, KeyModifierMask modMask = {});

    void                    MapGamepadAxis(PlayerController owner, StringView name, GamepadKeyOrAxis keyOrAxis, float power = 1);

    void                    MapGamepadAction(PlayerController owner, StringView name, GamepadKeyOrAxis keyOrAxis);

    bool                    GetMapping(VirtualKeyOrAxis keyOrAxis, KeyModifierMask modMask, VirtualMapping& virtMapping);

    bool                    GetGamepadMapping(PlayerController owner, GamepadKeyOrAxis keyOrAxis, VirtualMapping& virtMapping);

    HashMap<uint32_t, VirtualMapping> m_VirtMapping;
    HashMap<uint32_t, VirtualMapping> m_GamepadMapping;
};

HK_NAMESPACE_END
