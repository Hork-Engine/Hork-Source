/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/RenderCore/ShaderModule.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

class DeviceGLImpl;

class ShaderModuleGLImpl final : public IShaderModule
{
public:
    ShaderModuleGLImpl(DeviceGLImpl* _Device, ShaderBinaryData const* _BinaryData);
    ShaderModuleGLImpl(DeviceGLImpl* _Device, SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources);
    ~ShaderModuleGLImpl();

    static bool CreateShaderBinaryData(DeviceGLImpl* _Device,
                                       SHADER_TYPE _ShaderType,
                                       unsigned int _NumSources,
                                       const char* const* _Sources,
                                       ShaderBinaryData* _BinaryData);

    static void DestroyShaderBinaryData(DeviceGLImpl* _Device,
                                        ShaderBinaryData* _BinaryData);

private:
    static unsigned int CreateShaderProgram(SHADER_TYPE _ShaderType,
                                            int _NumStrings,
                                            const char* const* _Strings,
                                            bool bBinaryRetrievable);

    unsigned int CreateShaderProgramBin(ShaderBinaryData const* _BinaryData);
};

} // namespace RenderCore

HK_NAMESPACE_END
