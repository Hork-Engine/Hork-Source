/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Core/String.h>
#include <Geometry/VectorMath.h>
#include <Geometry/Quat.h>

enum VARIANT_TYPE : uint32_t
{
    VARIANT_UNDEFINED,
    VARIANT_BOOLEAN,
    VARIANT_INT8,
    VARIANT_INT16,
    VARIANT_INT32,
    VARIANT_INT64,
    VARIANT_UINT8,
    VARIANT_UINT16,
    VARIANT_UINT32,
    VARIANT_UINT64,
    VARIANT_FLOAT32,
    VARIANT_FLOAT64,
    VARIANT_FLOAT2,
    VARIANT_FLOAT3,
    VARIANT_FLOAT4,
    VARIANT_FLOAT2X2,
    VARIANT_FLOAT3X3,
    VARIANT_FLOAT3X4,
    VARIANT_FLOAT4X4,
    VARIANT_QUAT,
    VARIANT_STRING,
    VARIANT_RESOURCE_REF,
    VARIANT_ENUM
};

struct SResourceRef
{
    uint32_t ResourceType;
    uint64_t ResourceId;
};

HK_FORMAT_DEF_(SResourceRef, "( {} {} )", v.ResourceType, v.ResourceId)

struct SEnumDef
{
    int64_t     Value;
    const char* HumanReadableName;
};

template <typename T>
SEnumDef const* EnumDefinition();

HK_INLINE const char* FindEnumValue(SEnumDef const* EnumDef, int64_t EnumValue)
{
    for (SEnumDef const* e = EnumDef ; e->HumanReadableName ; e++)
    {
        if (e->Value == EnumValue)
            return e->HumanReadableName;
    }
    return "[Undefined]";
}

HK_INLINE int64_t EnumFromString(SEnumDef const* EnumDef, AStringView String)
{
    for (SEnumDef const* e = EnumDef ; e->HumanReadableName ; e++)
    {
        if (String == e->HumanReadableName)
            return e->Value;
    }
    return 0;
}

namespace VariantTraits
{

template<typename T> VARIANT_TYPE DeduceVariantType();

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<bool>()
{
    return VARIANT_BOOLEAN;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<int8_t>()
{
    return VARIANT_INT8;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<int16_t>()
{
    return VARIANT_INT16;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<int32_t>()
{
    return VARIANT_INT32;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<int64_t>()
{
    return VARIANT_INT64;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<uint8_t>()
{
    return VARIANT_UINT8;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<uint16_t>()
{
    return VARIANT_UINT16;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<uint32_t>()
{
    return VARIANT_UINT32;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<uint64_t>()
{
    return VARIANT_UINT64;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<float>()
{
    return VARIANT_FLOAT32;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<double>()
{
    return VARIANT_FLOAT64;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float2>()
{
    return VARIANT_FLOAT2;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float3>()
{
    return VARIANT_FLOAT3;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float4>()
{
    return VARIANT_FLOAT4;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float2x2>()
{
    return VARIANT_FLOAT2X2;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float3x3>()
{
    return VARIANT_FLOAT3X3;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float3x4>()
{
    return VARIANT_FLOAT3X4;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Float4x4>()
{
    return VARIANT_FLOAT4X4;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Quat>()
{
    return VARIANT_QUAT;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<AString>()
{
    return VARIANT_STRING;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<SResourceRef>()
{
    return VARIANT_RESOURCE_REF;
}

template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
HK_FORCEINLINE VARIANT_TYPE GetVariantType()
{
    return DeduceVariantType<T>();
}

template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
HK_FORCEINLINE VARIANT_TYPE GetVariantType()
{
    return VARIANT_ENUM;
}

template <typename T>
SEnumDef const* GetVariantEnum(typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
{
    return nullptr;
}

template <typename T>
SEnumDef const* GetVariantEnum(typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    return EnumDefinition<T>();
}

}


/** Variant is used to store property value */
class AVariant final
{
    static constexpr size_t SizeOf =
        std::max({sizeof(bool),
                  sizeof(int8_t),
                  sizeof(int16_t),
                  sizeof(int32_t),
                  sizeof(int64_t),
                  sizeof(uint8_t),
                  sizeof(uint16_t),
                  sizeof(uint32_t),
                  sizeof(uint64_t),
                  sizeof(float),
                  sizeof(double),
                  sizeof(Float2),
                  sizeof(Float3),
                  sizeof(Float4),
                  sizeof(Float2x2),
                  sizeof(Float3x3),
                  sizeof(Float3x4),
                  sizeof(Float4x4),
                  sizeof(Quat),
                  sizeof(AString),
                  sizeof(SResourceRef)});

    struct SEnumType
    {
        uint8_t EnumData[8];
        int64_t EnumValue;
        SEnumDef const* EnumDef;
    };

    VARIANT_TYPE m_Type{VARIANT_UNDEFINED};

    union
    {
        alignas(16) uint8_t RawData[SizeOf];
        SEnumType EnumType;
    };

public:
    AVariant()
    {}

    ~AVariant()
    {
        if (m_Type == VARIANT_STRING)
            ((AString*)&RawData[0])->~AString();
    }

    AVariant(const char *Rhs) :
        m_Type(VARIANT_STRING)
    {
        new (RawData) AString;

        *(AString*)&RawData[0] = Rhs;
    }

    AVariant(AStringView Rhs) :
        m_Type(VARIANT_STRING)
    {
        new (RawData) AString;

        *(AString*)&RawData[0] = Rhs;
    }

    template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
    AVariant(T const& Rhs) :
        m_Type(VariantTraits::DeduceVariantType<T>())
    {
        if (std::is_same<T, AString>())
        {
            new(RawData) AString;
        }
        *(T*)&RawData[0] = Rhs;
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
    AVariant(T const& Rhs) :
        m_Type(VARIANT_ENUM)
    {
        static_assert(sizeof(EnumType.EnumData) >= sizeof(T), "The enum type size must not exceed 64 bytes.");

        *(T*)&EnumType.EnumData[0] = Rhs;
        EnumType.EnumValue = Rhs;
        EnumType.EnumDef = EnumDefinition<T>();
    }

    AVariant(VARIANT_TYPE Type, SEnumDef const* EnumDef, AStringView String)
    {
        SetFromString(Type, EnumDef, String);
    }

    AVariant& operator=(const char* Rhs)
    {
        if (m_Type != VARIANT_STRING)
        {
            new (RawData) AString;
        }

        m_Type = VARIANT_STRING;

        *(AString*)&RawData[0] = Rhs;
        return *this;
    }

    AVariant& operator=(AStringView Rhs)
    {
        if (m_Type != VARIANT_STRING)
        {
            new (RawData) AString;
        }

        m_Type = VARIANT_STRING;

        *(AString*)&RawData[0] = Rhs;
        return *this;
    }

    template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
    AVariant& operator=(T const& Rhs)
    {
        if (std::is_same<T, AString>())
        {
            if (m_Type != VARIANT_STRING)
            {
                new(RawData) AString;
            }
        }
        else
        {
            if (m_Type == VARIANT_STRING)
                ((AString*)&RawData[0])->~AString();
        }

        m_Type = VariantTraits::DeduceVariantType<T>();

        *(T*)&RawData[0] = Rhs;
        return *this;
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
    AVariant& operator=(T const& Rhs)
    {
        if (m_Type == VARIANT_STRING)
            ((AString*)&RawData[0])->~AString();

        m_Type = VARIANT_ENUM;

        static_assert(sizeof(EnumType.EnumData) >= sizeof(T), "The enum type size must not exceed 64 bytes.");

        *(T*)&EnumType.EnumData[0] = Rhs;
        EnumType.EnumValue = Rhs;
        EnumType.EnumDef = EnumDefinition<T>();

        return *this;
    }

    template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
    T* Get() const
    {
        if (VariantTraits::DeduceVariantType<T>() != m_Type)
            return {};

        return (T*)&RawData[0];
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
    T* Get() const
    {
        if (EnumDefinition<T>() != EnumType.EnumDef)
            return {};

        return (T*)&EnumType.EnumData[0];
    }

    VARIANT_TYPE GetType() const
    {
        return m_Type;
    }

    void SetFromString(VARIANT_TYPE Type, SEnumDef const* EnumDef, AStringView String);

    AString ToString() const;
};

HK_FORMAT_DEF_TO_STRING(AVariant)
