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

#include <Hork/Core/Format.h>
#include <Hork/Core/HashFunc.h>

HK_NAMESPACE_BEGIN

class ResourceID
{
    uint32_t m_Id;

public:
    ResourceID() :
        m_Id(0)
    {}

    explicit ResourceID(uint32_t id) :
        m_Id(id)
    {}

    ResourceID(uint8_t type, uint32_t index) :
        m_Id((uint32_t(type) << 24) | (index & 0xffffff))
    {}

    bool operator==(ResourceID const& rhs) const
    {
        return m_Id == rhs.m_Id;
    }

    bool operator!=(ResourceID const& rhs) const
    {
        return m_Id != rhs.m_Id;
    }

    uint8_t GetType() const
    {
        return m_Id >> 24;
    }

    uint32_t GetIndex() const
    {
        return m_Id & 0xffffff;
    }

    template <typename T>
    bool Is()
    {
        return GetType() == T::Type;
    }

    bool IsValid() const
    {
        return m_Id != 0;
    }

    operator bool() const
    {
        return m_Id != 0;
    }

    operator uint32_t() const
    {
        return m_Id;
    }

    uint32_t Hash() const
    {
        return HashTraits::Hash(m_Id);
    }
};

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::ResourceID, "[{}:{}]", v.GetType(), v.GetIndex());
