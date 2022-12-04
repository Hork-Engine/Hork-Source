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

#include "RenderDefs.h"

#define SHADOWMAP_PCF
//#define SHADOWMAP_PCSS
//#define SHADOWMAP_VSM
//#define SHADOWMAP_EVSM

class AShaderFactory
{
public:
    static void CreateShader(RenderCore::SHADER_TYPE ShaderType, TPodVector<const char*> Sources, TRef<RenderCore::IShaderModule>& Module);
    static void CreateShader(RenderCore::SHADER_TYPE ShaderType, const char* Source, TRef<RenderCore::IShaderModule>& Module);
    static void CreateShader(RenderCore::SHADER_TYPE ShaderType, AString const& Source, TRef<RenderCore::IShaderModule>& Module);

    static void CreateVertexShader(AStringView FileName, RenderCore::SVertexAttribInfo const* VertexAttribs, int NumVertexAttribs, TRef<RenderCore::IShaderModule>& Module);
    static void CreateTessControlShader(AStringView FileName, TRef<RenderCore::IShaderModule>& Module);
    static void CreateTessEvalShader(AStringView FileName, TRef<RenderCore::IShaderModule>& Module);
    static void CreateGeometryShader(AStringView FileName, TRef<RenderCore::IShaderModule>& Module);
    static void CreateFragmentShader(AStringView FileName, TRef<RenderCore::IShaderModule>& Module);

    static void CreateFullscreenQuadPipeline(TRef<RenderCore::IPipeline>*               ppPipeline,
                                             AStringView                                VertexShader,
                                             AStringView                                FragmentShader,
                                             RenderCore::SPipelineResourceLayout const* pResourceLayout = nullptr,
                                             RenderCore::BLENDING_PRESET                BlendingPreset  = RenderCore::BLENDING_NO_BLEND);

    static void CreateFullscreenQuadPipelineGS(TRef<RenderCore::IPipeline>*               ppPipeline,
                                               AStringView                                VertexShader,
                                               AStringView                                FragmentShader,
                                               AStringView                                GeometryShader,
                                               RenderCore::SPipelineResourceLayout const* pResourceLayout = nullptr,
                                               RenderCore::BLENDING_PRESET                BlendingPreset  = RenderCore::BLENDING_NO_BLEND);
};
