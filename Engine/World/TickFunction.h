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

#include <Engine/Core/Delegate.h>
#include <Engine/Core/StringID.h>
#include <Engine/Core/Containers/Vector.h>
#include "ComponentRTTR.h"
#include "InterfaceRTTR.h"

HK_NAMESPACE_BEGIN

enum class TickGroup : uint8_t
{
    Update          = 0,
    FixedUpdate     = 1,
    PhysicsUpdate   = 2,
    PostTransform   = 3,
    LateUpdate      = 4
};

struct TickFunctionDesc
{
    StringID                    Name;
    bool                        TickEvenWhenPaused = false;
    SmallVector<uint32_t, 4>    Prerequisites;

    template <typename ComponentType>
    void AddPrerequisiteComponent()
    {
        Prerequisites.Add(ComponentRTTR::TypeID<ComponentType>);
    }

    template <typename InterfaceType>
    void AddPrerequisiteInterface()
    {
        // Use high bit to mark interface
        Prerequisites.Add(InterfaceRTTR::TypeID<InterfaceType> | (1 << 31));
    }
};

struct TickFunction
{
    TickFunctionDesc            Desc;
    TickGroup                   Group;
    Hk::Delegate<void()>        Delegate;
    uint32_t                    OwnerTypeID;
};

namespace TickGroup_Update
{
    template <typename ComponentType>
    HK_INLINE void InitializeTickFunction(TickFunctionDesc& desc)
    {}
}

namespace TickGroup_FixedUpdate
{
    template <typename ComponentType>
    HK_INLINE void InitializeTickFunction(TickFunctionDesc& desc)
    {}
}

namespace TickGroup_PhysicsUpdate
{
    template <typename ComponentType>
    HK_INLINE void InitializeTickFunction(TickFunctionDesc& desc)
    {}
}

namespace TickGroup_PostTransform
{
    template <typename ComponentType>
    HK_INLINE void InitializeTickFunction(TickFunctionDesc& desc)
    {}
}

namespace TickGroup_LateUpdate
{
    template <typename ComponentType>
    HK_INLINE void InitializeTickFunction(TickFunctionDesc& desc)
    {}
}

HK_NAMESPACE_END
