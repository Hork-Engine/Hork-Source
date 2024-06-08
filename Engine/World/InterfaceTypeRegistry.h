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
