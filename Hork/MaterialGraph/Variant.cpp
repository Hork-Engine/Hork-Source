/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Core/Logger.h>
#include <Hork/Core/Parse.h>

HK_NAMESPACE_BEGIN

ResourceRef StringToResourceRef(StringView String)
{
    ResourceRef ref;

    StringView token;
    StringView s = String;

    s = Core::GetToken(token, s);
    if (!token.Compare("("))
    {
        LOG("Expected '('\n");
        return {};
    }

    s = Core::GetToken(token, s);
    if (token.IsEmpty())
    {
        LOG("Expected resource type\n");
        return {};
    }

    ref.ResourceType = Core::ParseUInt32(token);

    s = Core::GetToken(token, s);
    if (token.IsEmpty())
    {
        LOG("Expected resource id\n");
        return {};
    }

    ref.ResourceId = Core::ParseUInt64(token);

    s = Core::GetToken(token, s);
    if (!token.Compare(")"))
    {
        LOG("Expected ')'\n");
    }

    return ref;
}

void Variant::SetFromString(VARIANT_TYPE type, EnumDef const* enumDef, StringView string)
{
    switch (type)
    {
        case VARIANT_UNDEFINED:
            return;
        case VARIANT_BOOLEAN:
            *this = Core::ParseBool(string);
            break;
        case VARIANT_BOOL2:
            *this = Core::ParseVector<Bool2>(string);
            break;
        case VARIANT_BOOL3:
            *this = Core::ParseVector<Bool3>(string);
            break;
        case VARIANT_BOOL4:
            *this = Core::ParseVector<Bool4>(string);
            break;
        case VARIANT_INT8:
            *this = Core::ParseInt8(string);
            break;
        case VARIANT_INT16:
            *this = Core::ParseInt16(string);
            break;
        case VARIANT_INT32:
            *this = Core::ParseInt32(string);
            break;
        case VARIANT_INT64:
            *this = Core::ParseInt64(string);
            break;
        case VARIANT_UINT8:
            *this = Core::ParseUInt8(string);
            break;
        case VARIANT_UINT16:
            *this = Core::ParseUInt16(string);
            break;
        case VARIANT_UINT32:
            *this = Core::ParseUInt32(string);
            break;
        case VARIANT_UINT64:
            *this = Core::ParseUInt64(string);
            break;
        case VARIANT_FLOAT32:
            *this = Core::ParseFloat(string);
            break;
        case VARIANT_FLOAT64:
            *this = Core::ParseDouble(string);
            break;
        case VARIANT_FLOAT2:
            *this = Core::ParseVector<Float2>(string);
            break;
        case VARIANT_FLOAT3:
            *this = Core::ParseVector<Float3>(string);
            break;
        case VARIANT_FLOAT4:
            *this = Core::ParseVector<Float4>(string);
            break;
        case VARIANT_FLOAT2X2:
            *this = Core::ParseMatrix<Float2x2>(string);
            break;
        case VARIANT_FLOAT3X3:
            *this = Core::ParseMatrix<Float3x3>(string);
            break;
        case VARIANT_FLOAT3X4:
            *this = Core::ParseMatrix<Float3x4>(string);
            break;
        case VARIANT_FLOAT4X4:
            *this = Core::ParseMatrix<Float4x4>(string);
            break;
        case VARIANT_QUAT:
            *this = Core::ParseVector<Quat>(string);
            break;
        case VARIANT_STRING:
            *this = string;
            break;
        case VARIANT_RESOURCE_REF:
            *this = StringToResourceRef(string);
            break;
        case VARIANT_ENUM:
            HK_ASSERT(enumDef);
            SetEnum(enumDef, EnumFromString(enumDef, string));
            break;
        default:
            HK_ASSERT(0);
    }
}

String Variant::ToString() const
{
    switch (m_Type)
    {
        case VARIANT_UNDEFINED:
            return "";
        case VARIANT_BOOLEAN:
            return Core::ToString(*Get<bool>());
        case VARIANT_BOOL2:
            return Core::ToString(*Get<Bool2>());
        case VARIANT_BOOL3:
            return Core::ToString(*Get<Bool3>());
        case VARIANT_BOOL4:
            return Core::ToString(*Get<Bool4>());
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
            return *Get<String>();
        case VARIANT_RESOURCE_REF:
            return Core::ToString(*Get<ResourceRef>());
        case VARIANT_ENUM:
            return FindEnumValue(m_EnumType.Definition, m_EnumType.Value);
        default:
            HK_ASSERT(0);
    }
    return "";
}

HK_NAMESPACE_END
