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

#include "DeviceObject.h"

HK_NAMESPACE_BEGIN

namespace RHI
{

enum SHADER_TYPE : uint8_t
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    TESS_CONTROL_SHADER,
    TESS_EVALUATION_SHADER,
    GEOMETRY_SHADER,
    COMPUTE_SHADER
};

enum SHADER_BINARY_FORMAT : unsigned int
{
    SHADER_BINARY_FORMAT_SPIR_V_ARB = 0x9551
};

struct ShaderBinaryData
{
    void const*  BinaryCode;
    size_t       BinarySize;
    unsigned int BinaryFormat;
    SHADER_TYPE  ShaderType;
};

class IShaderModule : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_SHADER_MODULE;

    IShaderModule(IDevice* pDevice) :
        IDeviceObject(pDevice, PROXY_TYPE)
    {}

    SHADER_TYPE GetType() const { return Type; }

protected:
    SHADER_TYPE Type;
};

} // namespace RHI

HK_NAMESPACE_END
