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

#include <Engine/Renderer/RenderDefs.h>
#include <Engine/ShaderUtils/ShaderLoader.h>

HK_NAMESPACE_BEGIN

class MaterialBinary;

class MaterialCode
{
public:
    /// Material type (Unlit,baselight,pbr,etc)
    MATERIAL_TYPE               Type = MATERIAL_TYPE_PBR;

    /// Blending mode (FIXME: only for UNLIT materials?)
    BLENDING_MODE               Blending = COLOR_BLENDING_DISABLED;

    TESSELLATION_METHOD         TessellationMethod = TESSELLATION_DISABLED;

    RENDERING_PRIORITY          RenderingPriority = RENDERING_PRIORITY_DEFAULT;

    /// Have vertex deformation in vertex stage. This flag allow renderer to optimize pipeline switching
    /// during rendering.
    bool                        HasVertexDeform : 1;

    /// Experimental. Depth testing.
    bool                        DepthTest_EXPERIMENTAL : 1;

    /// Disable shadow casting (for specific materials like skybox or first person shooter weapon)
    bool                        NoCastShadow : 1;

    /// Enable alpha masking
    bool                        HasAlphaMasking : 1;

    /// Enable shadow map masking
    bool                        HasShadowMapMasking : 1;

    /// Use tessellation for shadow maps
    bool                        DisplacementAffectShadow : 1;

    /// Apply fake shadows. Used with parallax technique
    //bool                      ParallaxMappingSelfShadowing : 1;

    /// Translusent materials with alpha test
    bool                        IsTranslucent : 1;

    /// Disable backface culling
    bool                        IsTwoSided : 1;

    /// Material code
    Vector<CodeBlock>           CodeBlocks;

    /// Lightmap binding unit
    uint32_t                    LightmapSlot{};

    /// Texture binding count for different passes. This allow renderer to optimize sampler/texture bindings
    /// during rendering.
    int                         DepthPassTextureCount{};
    int                         LightPassTextureCount{};
    int                         WireframePassTextureCount{};
    int                         NormalsPassTextureCount{};
    int                         ShadowMapPassTextureCount{};

    int                         NumUniformVectors{};

    /// Material samplers
    StaticVector<TextureSampler, MAX_MATERIAL_TEXTURES> Samplers;

                                MaterialCode();

    void                        Read(IBinaryStreamReadInterface& stream);
    void                        Write(IBinaryStreamWriteInterface& stream) const;

    void                        AddCodeBlock(String sourceName, String sourceCode);

    /// Translate source code to SpirV
    UniqueRef<MaterialBinary>   Translate();
};


HK_NAMESPACE_END
