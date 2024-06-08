#pragma once

#include <Engine/Core/Delegate.h>
#include <Engine/Core/StringID.h>
#include <Engine/Core/Containers/Vector.h>

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
    TSmallVector<uint32_t, 4>   Prerequisites;

    template <typename ComponentType>
    void AddPrerequisiteComponent()
    {
        Prerequisites.Add(ComponentTypeRegistry::GetComponentTypeID<ComponentType>());
    }

    template <typename InterfaceType>
    void AddPrerequisiteInterface()
    {
        // Use high bit to mark interface
        Prerequisites.Add(InterfaceTypeRegistry::GetInterfaceTypeID<InterfaceType>() | (1 << 31));
    }
};

struct TickFunction
{
    TickFunctionDesc            Desc;
    TickGroup                   Group;
    Delegate<void()>            Delegate;
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
