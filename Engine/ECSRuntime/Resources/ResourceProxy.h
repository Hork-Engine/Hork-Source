#pragma once

#include <Engine/Core/Containers/Vector.h>
#include <Engine/Core/String.h>
#include <Engine/Core/Ref.h>
#include <Engine/Core/Logger.h>

#include "ResourceBase.h"

HK_NAMESPACE_BEGIN

enum RESOURCE_STATE : uint8_t
{
    // Resource is unitialized / free
    RESOURCE_STATE_FREE,
    // Resource queued for loading
    RESOURCE_STATE_LOAD,
    // Resource loaded and ready to use
    RESOURCE_STATE_READY,
    // The resource was not loaded correctly (an error occurred while loading)
    RESOURCE_STATE_INVALID
};

enum RESOURCE_FLAGS : uint8_t
{
    RESOURCE_FLAG_PROCEDURAL = HK_BIT(0)
};

struct ResourceArea;
class ResourceManager;

class ResourceProxy final : Noncopyable
{
    friend class ResourceManager;

public:
    StringView GetName() const
    {
        return m_Name;
    }

    bool IsReady() const
    {
        return m_State == RESOURCE_STATE_READY;
    }

    RESOURCE_STATE GetState() const
    {
        return m_State;
    }

    bool IsProcedural() const
    {
        return !!(m_Flags & RESOURCE_FLAG_PROCEDURAL);
    }

private:
    // Called by resource manager on main thread to upload data to GPU.
    void Upload()
    {
        m_Resource->Upload();
    }

    // Called by resource manager on main thread to purge resource data (CPU and GPU).
    void Purge()
    {
        m_Resource.Reset();

        LOG("Purged {}\n", m_Name);
    }

    bool HasData() const
    {
        return m_Resource;
    }

    // NOTE: Resource can be used only if m_State == RESOURCE_STATE_READY.
    TUniqueRef<ResourceBase> m_Resource;

    // Updated by resource manager in main thread.
    int m_UseCount{};

    // Resource name/path. Immutable.
    StringView m_Name;

    // Updated by resource manager in main thread.
    // Used to notify areas when resource loaded/unloaded.
    // FIXME: Use linked list?
    TVector<ResourceArea*> m_Areas;

    // Updated by the resource manager on the main thread during Update(), so it can be read from any worker thread.
    RESOURCE_STATE m_State{RESOURCE_STATE_FREE};

    RESOURCE_FLAGS m_Flags{};
};

HK_NAMESPACE_END
