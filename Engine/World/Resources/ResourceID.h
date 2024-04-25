#pragma once

#include <Engine/Core/Format.h>
#include <Engine/Core/HashFunc.h>

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
