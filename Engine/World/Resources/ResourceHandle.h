#pragma once

#include "ResourceID.h"

HK_NAMESPACE_BEGIN

template <typename T>
struct ResourceHandle
{
    ResourceID ID;

    ResourceHandle() = default;

    explicit ResourceHandle(ResourceID id) :
        ID(id)
    {
        HK_ASSERT(id.GetType() == T::Type);
    }

    operator ResourceID() const
    {
        return ID;
    }

    operator bool() const
    {
        return ID.operator bool();
    }

    bool IsValid() const
    {
        return ID.IsValid();
    }
};

HK_NAMESPACE_END
