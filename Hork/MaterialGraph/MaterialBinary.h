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

#pragma once

#include <Hork/Renderer/RenderDefs.h>

HK_NAMESPACE_BEGIN

class MaterialBinary
{
public:
    struct Shader
    {
        Shader() = default;
        Shader(RHI::SHADER_TYPE type, HeapBlob blob) :
            m_Type(type), m_Blob(std::move(blob))
        {}

        RHI::SHADER_TYPE m_Type;
        HeapBlob m_Blob;

        void Read(IBinaryStreamReadInterface& stream);
        void Write(IBinaryStreamWriteInterface& stream) const;
    };

    enum class VertexFormat
    {
        StaticMesh,
        SkinnedMesh,
        StaticMesh_Lightmap,
        StaticMesh_VertexLight
    };

    struct MaterialPassData
    {
        MaterialPass::Type              Type;
        RHI::POLYGON_CULL               CullMode = RHI::POLYGON_CULL_BACK;
        RHI::COMPARISON_FUNCTION        DepthFunc = RHI::CMPFUNC_LESS;
        bool                            DepthWrite = true;
        bool                            DepthTest = true;
        RHI::PRIMITIVE_TOPOLOGY         Topology = RHI::PRIMITIVE_TRIANGLES;
        Vector<RHI::BufferInfo>         BufferBindings;
        Vector<RHI::RenderTargetBlendingInfo>    RenderTargets;
        Vector<RHI::SamplerDesc>        Samplers;

        VertexFormat                    VertFormat = VertexFormat::StaticMesh;

        uint32_t                        VertexShader = ~0u;
        uint32_t                        FragmentShader = ~0u;
        uint32_t                        TessControlShader = ~0u;
        uint32_t                        TessEvalShader = ~0u;
        uint32_t                        GeometryShader = ~0u;

        void Read(IBinaryStreamReadInterface& stream);
        void Write(IBinaryStreamWriteInterface& stream) const;
    };

    MATERIAL_TYPE           MaterialType{MATERIAL_TYPE_PBR};
    bool                    IsCastShadow = false;
    bool                    IsTranslucent = false;
    RENDERING_PRIORITY      RenderingPriority = RENDERING_PRIORITY_DEFAULT;
    int                     TextureCount = 0;
    int                     UniformVectorCount = 0;
    int                     LightmapSlot{};
    int                     DepthPassTextureCount{};
    int                     LightPassTextureCount{};
    int                     WireframePassTextureCount{};
    int                     NormalsPassTextureCount{};
    int                     ShadowMapPassTextureCount{};
    Vector<Shader>          Shaders; // SpirV or Raw sources
    Vector<MaterialPassData>Passes;

    uint32_t                AddShader(RHI::SHADER_TYPE shaderType, HeapBlob blob);

    void                    Read(IBinaryStreamReadInterface& stream);
    void                    Write(IBinaryStreamWriteInterface& stream) const;
};

HK_NAMESPACE_END
