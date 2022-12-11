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
#include <Core/Parse.h>
#include <Geometry/VectorMath.h>
#include <Geometry/Quat.h>

enum VARIANT_TYPE : uint32_t
{
    VARIANT_UNDEFINED,
    VARIANT_BOOLEAN,
    VARIANT_BOOL2,
    VARIANT_BOOL3,
    VARIANT_BOOL4,
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

struct ResourceRef
{
    uint32_t ResourceType;
    uint64_t ResourceId;
};

HK_FORMAT_DEF_(ResourceRef, "( {} {} )", v.ResourceType, v.ResourceId)

struct EnumDef
{
    int64_t     Value;
    const char* HumanReadableName;
};

template <typename T>
EnumDef const* EnumDefinition();

HK_INLINE const char* FindEnumValue(EnumDef const* enumDef, int64_t enumValue)
{
    for (EnumDef const* e = enumDef; e->HumanReadableName; e++)
    {
        if (e->Value == enumValue)
            return e->HumanReadableName;
    }
    return "[Undefined]";
}

HK_INLINE int64_t EnumFromString(EnumDef const* enumDef, StringView string)
{
    for (EnumDef const* e = enumDef; e->HumanReadableName; e++)
    {
        if (string == e->HumanReadableName)
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

template <> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Bool2>()
{
    return VARIANT_BOOL2;
}

template <> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Bool3>()
{
    return VARIANT_BOOL3;
}

template <> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<Bool4>()
{
    return VARIANT_BOOL4;
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

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<String>()
{
    return VARIANT_STRING;
}

template<> HK_FORCEINLINE VARIANT_TYPE DeduceVariantType<ResourceRef>()
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
EnumDef const* GetVariantEnum(typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
{
    return nullptr;
}

template <typename T>
EnumDef const* GetVariantEnum(typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    return EnumDefinition<T>();
}

}


/** Variant is used to store property value */
class Variant final
{
    static constexpr size_t SizeOf =
        std::max({sizeof(bool),
                  sizeof(Bool2),
                  sizeof(Bool3),
                  sizeof(Bool4),
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
                  sizeof(String),
                  sizeof(ResourceRef)});

    struct EnumType
    {
        uint8_t EnumData[8];
        int64_t EnumValue;
        EnumDef const* EnumDef;
    };

    VARIANT_TYPE m_Type{VARIANT_UNDEFINED};

    union
    {
        alignas(16) uint8_t m_RawData[SizeOf];
        EnumType m_EnumType;
    };

public:
    Variant() = default;

    ~Variant()
    {
        Reset();
    }

    Variant(Variant const& Rhs) :
        m_Type(Rhs.m_Type)
    {
        switch (Rhs.m_Type)
        {
            case VARIANT_STRING:
                new (m_RawData) String;
                *(String*)&m_RawData[0] = *(String*)&Rhs.m_RawData[0];
                break;
            case VARIANT_ENUM:
                m_EnumType = Rhs.m_EnumType;
                break;
            case VARIANT_UNDEFINED:
                break;
            default:
                Platform::Memcpy(m_RawData, Rhs.m_RawData, sizeof(m_RawData));
                break;
        }
    }

    Variant& operator=(Variant const& Rhs)
    {
        Reset();

        m_Type = Rhs.m_Type;

        switch (Rhs.m_Type)
        {
            case VARIANT_STRING:
                new (m_RawData) String;
                *(String*)&m_RawData[0] = *(String*)&Rhs.m_RawData[0];
                break;
            case VARIANT_ENUM:
                m_EnumType = Rhs.m_EnumType;
                break;
            case VARIANT_UNDEFINED:
                break;
            default:
                Platform::Memcpy(m_RawData, Rhs.m_RawData, sizeof(m_RawData));
                break;
        }
        return *this;
    }

    Variant(Variant&& Rhs) noexcept :
        m_Type(Rhs.m_Type)
    {
        switch (Rhs.m_Type)
        {
            case VARIANT_STRING:
                new (m_RawData) String(std::move(*(String*)&Rhs.m_RawData[0]));
                break;
            case VARIANT_ENUM:
                m_EnumType = Rhs.m_EnumType;
                break;
            case VARIANT_UNDEFINED:
                break;
            default:
                Platform::Memcpy(m_RawData, Rhs.m_RawData, sizeof(m_RawData));
                break;
        }

        Rhs.m_Type = VARIANT_UNDEFINED;
    }

    Variant& operator=(Variant&& Rhs) noexcept
    {
        Reset();

        m_Type = Rhs.m_Type;

        switch (Rhs.m_Type)
        {
            case VARIANT_STRING:
                new (m_RawData) String(std::move(*(String*)&Rhs.m_RawData[0]));
                break;
            case VARIANT_ENUM:
                m_EnumType = Rhs.m_EnumType;
                break;
            case VARIANT_UNDEFINED:
                break;
            default:
                Platform::Memcpy(m_RawData, Rhs.m_RawData, sizeof(m_RawData));
                break;
        }

        Rhs.m_Type = VARIANT_UNDEFINED;
        return *this;
    }

    Variant(const char *Rhs) :
        m_Type(VARIANT_STRING)
    {
        new (m_RawData) String;

        *(String*)&m_RawData[0] = Rhs;
    }

    Variant(StringView Rhs) :
        m_Type(VARIANT_STRING)
    {
        new (m_RawData) String;

        *(String*)&m_RawData[0] = Rhs;
    }

    template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
    Variant(T const& Rhs) :
        m_Type(VariantTraits::DeduceVariantType<T>())
    {
        if (std::is_same<T, String>())
        {
            new (m_RawData) String;
        }
        *(T*)&m_RawData[0] = Rhs;
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
    Variant(T const& Rhs) :
        m_Type(VARIANT_ENUM)
    {
        static_assert(sizeof(m_EnumType.EnumData) >= sizeof(T), "The enum type size must not exceed 64 bytes.");

        *(T*)&m_EnumType.EnumData[0] = Rhs;
        m_EnumType.EnumValue         = Rhs;
        m_EnumType.EnumDef           = EnumDefinition<T>();
    }

    Variant(VARIANT_TYPE Type, EnumDef const* EnumDef, StringView String)
    {
        SetFromString(Type, EnumDef, String);
    }

    Variant& operator=(const char* Rhs)
    {
        if (m_Type != VARIANT_STRING)
        {
            new (m_RawData) String;
        }

        m_Type = VARIANT_STRING;

        *(String*)&m_RawData[0] = Rhs;
        return *this;
    }

    Variant& operator=(StringView Rhs)
    {
        if (m_Type != VARIANT_STRING)
        {
            new (m_RawData) String;
        }

        m_Type = VARIANT_STRING;

        *(String*)&m_RawData[0] = Rhs;
        return *this;
    }

    template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
    Variant& operator=(T const& Rhs)
    {
        if (std::is_same<T, String>())
        {
            if (m_Type != VARIANT_STRING)
            {
                new (m_RawData) String;
            }
        }
        else
        {
            if (m_Type == VARIANT_STRING)
                ((String*)&m_RawData[0])->~String();
        }

        m_Type = VariantTraits::DeduceVariantType<T>();

        *(T*)&m_RawData[0] = Rhs;
        return *this;
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
    Variant& operator=(T const& Rhs)
    {
        if (m_Type == VARIANT_STRING)
            ((String*)&m_RawData[0])->~String();

        m_Type = VARIANT_ENUM;

        static_assert(sizeof(m_EnumType.EnumData) >= sizeof(T), "The enum type size must not exceed 64 bytes.");

        *(T*)&m_EnumType.EnumData[0] = Rhs;
        m_EnumType.EnumValue         = Rhs;
        m_EnumType.EnumDef           = EnumDefinition<T>();

        return *this;
    }

    template <typename T, std::enable_if_t<!std::is_enum<T>::value, bool> = true>
    T* Get() const
    {
        if (VariantTraits::DeduceVariantType<T>() != m_Type)
            return {};

        return (T*)&m_RawData[0];
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool> = true>
    T* Get() const
    {
        if (EnumDefinition<T>() != m_EnumType.EnumDef)
            return {};

        return (T*)&m_EnumType.EnumData[0];
    }

    VARIANT_TYPE GetType() const
    {
        return m_Type;
    }

    void SetFromString(VARIANT_TYPE Type, EnumDef const* EnumDef, StringView String);

    void Reset()
    {
        if (m_Type == VARIANT_STRING)
            ((String*)&m_RawData[0])->~String();
        m_Type = VARIANT_UNDEFINED;
    }

    String ToString() const;

private:
    void SetEnum(EnumDef const* EnumDef, int64_t EnumValue)
    {
        if (m_Type == VARIANT_STRING)
            ((String*)&m_RawData[0])->~String();

        m_Type = VARIANT_ENUM;

        *(int64_t*)&m_EnumType.EnumData[0] = EnumValue;
        m_EnumType.EnumValue               = EnumValue;
        m_EnumType.EnumDef                 = EnumDef;
    }
};

HK_FORMAT_DEF_TO_STRING(Variant)


// TODO: Move to Parse.h?

#include <Containers/Vector.h>

HK_INLINE StringView GetToken(StringView& Token, StringView String, bool bCrossLine = true)
{
    const char* p   = String.Begin();
    const char* end = String.End();

    Token = "";

    // skip space
    for (;;)
    {
        if (p == end)
        {
            return StringView(p, (StringSizeType)(end - p));
        }

        if (*p == '\n' && !bCrossLine)
        {
            LOG("Unexpected new line\n");
            return StringView(p, (StringSizeType)(end - p));
        }

        if (*p > 32)
            break;

        p++;
    }

    const char* token = p;
    while (p < end)
    {
        if (*p == '\n')
        {
            if (!bCrossLine)
                LOG("Unexpected new line\n");
            break;
        }

        if (*p <= 32)
            break;

        if (*p == '(' || *p == ')')
        {
            ++p;
            break;
        }

        ++p;
    }

    Token = StringView(token, p);

    return StringView(p, (StringSizeType)(end - p), String.IsNullTerminated());
}


template <typename VectorType>
HK_INLINE VectorType ParseVector(StringView String, StringView* NewString = nullptr)
{
    VectorType v;

    StringView token;
    StringView tmp;

    if (!NewString)
        NewString = &tmp;

    StringView& s = *NewString;

    s = GetToken(token, String);
    if (!token.Compare("("))
    {
        LOG("Expected '('\n");
        return v;
    }

    for (int i = 0; i < v.NumComponents(); i++)
    {
        s = GetToken(token, s);
        if (token.IsEmpty())
        {
            LOG("Expected value\n");
            return v;
        }

        using ElementType = std::remove_reference_t<decltype(v[i])>;

        v[i] = Core::ParseNumber<ElementType>(token);
    }

    s = GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("Expected ')'\n");
    }

    return v;
}

// Parses a vector of variable length. Returns false if there were errors.
HK_INLINE bool ParseVector(StringView String, TVector<StringView>& v)
{
    StringView token;

    v.Clear();

    StringView s = GetToken(token, String);
    if (!token.Compare("("))
    {
        v.Add(token);
        return true;
    }

    do
    {
        s = GetToken(token, s);
        if (token.IsEmpty())
        {
            LOG("ParseVector: Expected value\n");
            return false;
        }

        if (token.Compare(")"))
        {
            return true;
        }

        v.Add(token);
    } while (1);

    s = GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("ParseVector: Expected ')'\n");
        return false;
    }

    return true;
}

template <typename MatrixType>
HK_INLINE MatrixType ParseMatrix(StringView String)
{
    MatrixType matrix(1);

    StringView token;
    StringView s = String;

    s = GetToken(token, s);
    if (!token.Compare("("))
    {
        LOG("Expected '('\n");
        return matrix;
    }

    for (int i = 0; i < matrix.NumComponents(); i++)
    {
        using ElementType = std::remove_reference_t<decltype(matrix[i])>;

        matrix[i] = ParseVector<ElementType>(s, &s);
    }

    s = GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("Expected ')'\n");
    }

    return matrix;
}
