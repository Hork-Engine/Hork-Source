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

#include "BaseTypes.h"
#include "HashFunc.h"

HK_NAMESPACE_BEGIN

template <typename Entity>
class Handle32
{
    union
    {
        uint32_t m_Handle{};

        struct
        {
            uint32_t m_ID : 20;  // 0...1048575
            uint32_t m_Version : 12; // 0...4095
        };
    };

public:
    Handle32() = default;
    Handle32(Handle32 const& rhs) = default;
    Handle32(Handle32&& rhs) = default;

    static const uint32_t MAX_ID = 1u << 20;
    static const uint32_t MAX_VERSION = 1u << 12;

    Handle32(uint32_t id, uint32_t version) :
        m_ID(id), m_Version(version)
    {
        HK_ASSERT(id < MAX_ID);
        HK_ASSERT(version < MAX_VERSION);
    }

    Handle32(std::nullptr_t) :
        m_Handle(0)
    {}

    explicit Handle32(uint32_t handle) :
        m_Handle(handle)
    {}

    Handle32& operator=(Handle32 const& rhs) = default;

    bool operator==(Handle32 const& rhs) const
    {
        return m_Handle == rhs.m_Handle;
    }

    bool operator!=(Handle32 const& rhs) const
    {
        return m_Handle != rhs.m_Handle;
    }

    operator bool() const
    {
        return m_Handle != 0;
    }

    bool operator<(Handle32 const& rhs) const
    {
        return m_Handle < rhs.m_Handle;
    }

    bool operator>(Handle32 const& rhs) const
    {
        return m_Handle > rhs.m_Handle;
    }

    operator uint32_t() const
    {
        return m_Handle;
    }

    uint32_t GetVersion() const
    {
        return m_Version;
    }

    uint32_t GetID() const
    {
        return m_ID;
    }

    uint32_t ToUInt32() const
    {
        return m_Handle;
    }

    uint32_t Hash() const
    {
        return HashTraits::Hash(m_Handle);
    }
};

template <typename Entity>
class Handle64
{
    uint64_t m_Handle{};

public:
    Handle64() = default;
    Handle64(Handle64 const& rhs) = default;
    Handle64(Handle64&& rhs) = default;

    Handle64(uint32_t id, uint32_t version) :
        m_Handle(uint64_t(id) | (uint64_t(version) << 32))
    {}

    Handle64(std::nullptr_t) :
        m_Handle(0)
    {}

    explicit Handle64(uint64_t handle) :
        m_Handle(handle)
    {}

    Handle64& operator=(Handle64 const& rhs) = default;

    bool operator==(Handle64 const& rhs) const
    {
        return m_Handle == rhs.m_Handle;
    }

    bool operator!=(Handle64 const& rhs) const
    {
        return m_Handle != rhs.m_Handle;
    }

    operator bool() const
    {
        return m_Handle != 0;
    }

    operator uint64_t() const
    {
        return m_Handle;
    }

    uint32_t GetVersion() const
    {
        return m_Handle >> 32;
    }

    uint32_t GetID() const
    {
        return m_Handle & 0xffffffff;
    }

    uint64_t ToUInt64() const
    {
        return m_Handle;
    }

    uint32_t Hash() const
    {
        return HashTraits::Hash(m_Handle);
    }
};

template <typename T>
using Handle = Handle64<T>;

HK_NAMESPACE_END
