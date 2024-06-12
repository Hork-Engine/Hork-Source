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

#include <Engine/Core/StringID.h>
#include <Engine/Core/Containers/Hash.h>
#include <Engine/Core/Ref.h>

#include "VirtualKey.h"

HK_NAMESPACE_BEGIN

enum class PlayerController : uint8_t
{
    _1,
    _2,
    _3,
    _4,

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
    struct VirtualInput
    {
        VirtualKeyOrAxis VirtKey;
        uint16_t         ModMask;

        bool operator==(VirtualInput const& rhs) const
        {
            return VirtKey == rhs.VirtKey && ModMask == rhs.ModMask;
        }

        bool operator!=(VirtualInput const& rhs) const
        {
            return !operator==(rhs);
        }

        uint32_t Hash() const
        {
            return HashTraits::Hash(uint32_t(VirtKey.GetData()) | (uint32_t(ModMask) << 16));
        }
    };

public:
    void                    Clear();

    void                    MapAxis(StringView name, VirtualKeyOrAxis virtualKey, float power, PlayerController owner);

    void                    MapAction(StringView name, VirtualKeyOrAxis virtualKey, KeyModifierMask modMask, PlayerController owner);

    bool                    GetMapping(VirtualKeyOrAxis virtualKey, KeyModifierMask modMask, VirtualMapping& virtMapping);

    HashMap<VirtualInput, VirtualMapping> m_VirtMapping;
};

HK_NAMESPACE_END
