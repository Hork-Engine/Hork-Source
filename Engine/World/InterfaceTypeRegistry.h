#pragma once

#include <Engine/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

using InterfaceTypeID = uint32_t;

template <typename T>
struct InterfaceID
{
public:
    // NOTE: ID is used for runtime. For static time use InterfaceTypeRegistry::StaticTimeGenerateTypeID<T>
    static const InterfaceTypeID ID;
};

struct InterfaceTypeRegistry
{
    /// Returns interface ID by type
    template <typename T>
    static InterfaceTypeID GetInterfaceTypeID()
    {
        return InterfaceID<T>::ID;
    }

    /// Total interface types count
    static size_t GetInterfaceTypesCount()
    {
        return m_IdGen;
    }

    /// Static time ID generation. Do not use at runtime.
    template <typename T>
    static InterfaceTypeID StaticTimeGenerateTypeID()
    {
        using InterfaceType = typename std::remove_const_t<T>;
        return _GenerateTypeID<InterfaceType>();
    }

private:
    template <typename T>
    static InterfaceTypeID _GenerateTypeID();

private:
    static InterfaceTypeID m_IdGen;
};

template <typename T>
const InterfaceTypeID InterfaceID<T>::ID = InterfaceTypeRegistry::StaticTimeGenerateTypeID<T>();

template <typename T>
InterfaceTypeID InterfaceTypeRegistry::_GenerateTypeID()
{
    static_assert(!std::is_const<T>::value, "The type must not be constant");

    struct IdGen
    {
        IdGen() :
            ID(m_IdGen++)
        {
        }

        const InterfaceTypeID ID;
    };

    static IdGen generator;
    return generator.ID;
}

HK_NAMESPACE_END
