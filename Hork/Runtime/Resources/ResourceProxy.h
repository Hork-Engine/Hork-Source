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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/String.h>
#include <Hork/Core/Ref.h>
#include <Hork/Core/Logger.h>

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
    UniqueRef<ResourceBase> m_Resource;

    // Updated by resource manager in main thread.
    int m_UseCount{};

    // Resource name/path. Immutable.
    StringView m_Name;

    // Updated by resource manager in main thread.
    // Used to notify areas when resource loaded/unloaded.
    // FIXME: Use linked list?
    Vector<ResourceArea*> m_Areas;

    // Updated by the resource manager on the main thread during Update(), so it can be read from any worker thread.
    RESOURCE_STATE m_State{RESOURCE_STATE_FREE};

    RESOURCE_FLAGS m_Flags{};
};

HK_NAMESPACE_END
