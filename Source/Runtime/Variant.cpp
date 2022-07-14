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

#include "Variant.h"

#include <Platform/Logger.h>

AStringView GetToken(char* Token, size_t TokenSize, AStringView String, bool bCrossLine = true)
{
    const char* p = String.Begin();
    const char* end = String.End();

    Token[0] = 0;

    // skip space
    while (*p <= 32 && *p >= 0)
    {
        if (p == end)
        {
            return AStringView(p, (StringSizeType)(end-p));
        }

        if (*p++ == '\n')
        {
            if (!bCrossLine)
            {
                LOG("Unexpected new line\n");
                return AStringView(p, (StringSizeType)(end - p));
            }
        }
    }

    // copy token
    char* token_p = Token;
    while (!(*p <= 32 && *p >= 0))
    {
        if (p == end)
        {
            break;
        }

        if (*p == '\n')
        {
            p++;
            if (!bCrossLine)
            {
                LOG("Unexpected new line\n");
                break;
            }
            continue;
        }

        if ((*p == '(' || *p == ')') && Token[0])
            break;

        if (token_p == &Token[TokenSize - 1])
        {
            LOG("Token buffer overflow\n");
            break;
        }

        *token_p++ = *p++;

        if (Token[0] == '(' || Token[0] == ')')
            break;
    }
    *token_p = 0;

    return AStringView(p, (StringSizeType)(end - p), String.IsNullTerminated());
}


template <typename VectorType>
VectorType ParseVector(AStringView String, AStringView *NewString = nullptr)
{
    VectorType v;

    char token[128];

    AStringView tmp;

    if (!NewString)
        NewString = &tmp;

    AStringView& s = *NewString;

    s = GetToken(token, sizeof(token), String);
    if (Platform::Strcmp(token, "("))
    {
        LOG("Expected '('\n");
        return v;
    }

    for (int i = 0 ; i < v.NumComponents() ; i++)
    {
        s = GetToken(token, sizeof(token), s);
        if (!token[0])
        {
            LOG("Expected value\n");
            return v;
        }

        using ElementType = std::remove_reference_t<decltype(v[i])>;

        v[i] = Core::ParseNumber<ElementType>(token);
    }

    s = GetToken(token, sizeof(token), s);
    if (Platform::Strcmp(token, ")"))
    {
        LOG("Expected ')'\n");
    }

    return v;
}


template <typename MatrixType>
MatrixType ParseMatrix(AStringView String)
{
    MatrixType matrix(1);

    char token[128];

    AStringView s = String;

    s = GetToken(token, sizeof(token), s);
    if (Platform::Strcmp(token, "("))
    {
        LOG("Expected '('\n");
        return matrix;
    }

    for (int i = 0 ; i < matrix.NumComponents() ; i++)
    {
        using ElementType = std::remove_reference_t<decltype(matrix[i])>;

        matrix[i] = ParseVector<ElementType>(s, &s);
    }

    s = GetToken(token, sizeof(token), s);
    if (Platform::Strcmp(token, ")"))
    {
        LOG("Expected ')'\n");
    }

    return matrix;
}

SResourceRef StringToResourceRef(AStringView String)
{
    SResourceRef ref;

    char token[128];

    AStringView s = String;

    s = GetToken(token, sizeof(token), s);
    if (Platform::Strcmp(token, "("))
    {
        LOG("Expected '('\n");
        return {};
    }

    s = GetToken(token, sizeof(token), s);
    if (!token[0])
    {
        LOG("Expected resource type\n");
        return {};
    }

    ref.ResourceType = Core::ParseUInt32(token);

    s = GetToken(token, sizeof(token), s);
    if (!token[0])
    {
        LOG("Expected resource id\n");
        return {};
    }

    ref.ResourceId = Core::ParseUInt64(token);

    s = GetToken(token, sizeof(token), s);
    if (Platform::Strcmp(token, ")"))
    {
        LOG("Expected ')'\n");
    }

    return ref;
}

void AVariant::SetFromString(VARIANT_TYPE Type, SEnumDef const* EnumDef, AStringView String)
{
    switch (Type)
    {
        case VARIANT_UNDEFINED:
            return;
        case VARIANT_BOOLEAN:
            *this = Core::ParseBool(String);
            break;
        case VARIANT_INT8:
            *this = Core::ParseInt8(String);
            break;
        case VARIANT_INT16:
            *this = Core::ParseInt16(String);
            break;
        case VARIANT_INT32:
            *this = Core::ParseInt32(String);
            break;
        case VARIANT_INT64:
            *this = Core::ParseInt64(String);
            break;
        case VARIANT_UINT8:
            *this = Core::ParseUInt8(String);
            break;
        case VARIANT_UINT16:
            *this = Core::ParseUInt16(String);
            break;
        case VARIANT_UINT32:
            *this = Core::ParseUInt32(String);
            break;
        case VARIANT_UINT64:
            *this = Core::ParseUInt64(String);
            break;
        case VARIANT_FLOAT32:
            *this = Core::ParseFloat(String);
            break;
        case VARIANT_FLOAT64:
            *this = Core::ParseDouble(String);
            break;
        case VARIANT_FLOAT2:
            *this = ParseVector<Float2>(String);
            break;
        case VARIANT_FLOAT3:
            *this = ParseVector<Float3>(String);
            break;
        case VARIANT_FLOAT4:
            *this = ParseVector<Float4>(String);
            break;
        case VARIANT_FLOAT2X2:
            *this = ParseMatrix<Float2x2>(String);
            break;
        case VARIANT_FLOAT3X3:
            *this = ParseMatrix<Float3x3>(String);
            break;
        case VARIANT_FLOAT3X4:
            *this = ParseMatrix<Float3x4>(String);
            break;
        case VARIANT_FLOAT4X4:
            *this = ParseMatrix<Float4x4>(String);
            break;
        case VARIANT_QUAT:
            *this = ParseVector<Quat>(String);
            break;
        case VARIANT_STRING:
            *this = String;
            break;
        case VARIANT_RESOURCE_REF:
            *this = StringToResourceRef(String);
            break;
        case VARIANT_ENUM:
            HK_ASSERT(EnumDef);
            *this = EnumFromString(EnumDef, String);
            break;
        default:
            HK_ASSERT(0);
    }
}

AString AVariant::ToString() const
{
    switch (m_Type)
    {
        case VARIANT_UNDEFINED:
            return "";
        case VARIANT_BOOLEAN:
            return Core::ToString(*Get<bool>());
        case VARIANT_INT8:
            return Core::ToString(*Get<int8_t>());
        case VARIANT_INT16:
            return Core::ToString(*Get<int16_t>());
        case VARIANT_INT32:
            return Core::ToString(*Get<int32_t>());
        case VARIANT_INT64:
            return Core::ToString(*Get<int64_t>());
        case VARIANT_UINT8:
            return Core::ToString(*Get<uint8_t>());
        case VARIANT_UINT16:
            return Core::ToString(*Get<uint16_t>());
        case VARIANT_UINT32:
            return Core::ToString(*Get<uint32_t>());
        case VARIANT_UINT64:
            return Core::ToString(*Get<uint64_t>());
        case VARIANT_FLOAT32:
            return Core::ToString(*Get<float>());
        case VARIANT_FLOAT64:
            return Core::ToString(*Get<double>());
        case VARIANT_FLOAT2:
            return Core::ToString(*Get<Float2>());
        case VARIANT_FLOAT3:
            return Core::ToString(*Get<Float3>());
        case VARIANT_FLOAT4:
            return Core::ToString(*Get<Float4>());
        case VARIANT_FLOAT2X2:
            return Core::ToString(*Get<Float2x2>());
        case VARIANT_FLOAT3X3:
            return Core::ToString(*Get<Float3x3>());
        case VARIANT_FLOAT3X4:
            return Core::ToString(*Get<Float3x4>());
        case VARIANT_FLOAT4X4:
            return Core::ToString(*Get<Float4x4>());
        case VARIANT_QUAT:
            return Core::ToString(*Get<Quat>());
        case VARIANT_STRING:
            return *Get<AString>();
        case VARIANT_RESOURCE_REF:
            return Core::ToString(*Get<SResourceRef>());
        case VARIANT_ENUM:
            return FindEnumValue(EnumType.EnumDef, EnumType.EnumValue);
        default:
            HK_ASSERT(0);
    }
    return "";
}
