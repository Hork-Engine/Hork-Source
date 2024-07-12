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

#include "Resource_Material.h"

#include "Materials/MaterialGraph/MaterialGraph.h"
#include <Engine/Renderer/ShaderLoader.h>

HK_NAMESPACE_BEGIN

UniqueRef<MaterialResource> MaterialResource::Load(IBinaryStreamReadInterface& stream)
{
    UniqueRef<MaterialResource> resource = MakeUnique<MaterialResource>();
    if (!resource->Read(stream))
        return {};
    return resource;
}

bool MaterialResource::Read(IBinaryStreamReadInterface& stream)
{
    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    m_CompiledGraph = MakeRef<CompiledMaterial>(stream);
    m_Shader = LoadShader("material.glsl", m_CompiledGraph->Shaders);
    return true;
}

void MaterialResource::Write(IBinaryStreamWriteInterface& stream)
{
    stream.WriteUInt32(MakeResourceMagic(Type, Version));

    m_CompiledGraph->Write(stream);
}

void MaterialResource::Upload()
{
    m_GpuMaterial = MakeRef<MaterialGPU>(m_CompiledGraph, m_Shader);
    m_Shader.Free();
}

bool MaterialResource::IsCastShadow() const
{
    return !m_CompiledGraph->bNoCastShadow;
}

bool MaterialResource::IsTranslucent() const
{
    return m_CompiledGraph->bTranslucent;
}

RENDERING_PRIORITY MaterialResource::GetRenderingPriority() const
{
    return m_CompiledGraph->RenderingPriority;
}

uint32_t MaterialResource::GetTextureCount() const
{
    return m_CompiledGraph->Samplers.Size();
}

uint32_t MaterialResource::GetUniformVectorCount() const
{
    return m_CompiledGraph->NumUniformVectors;
}

UniqueRef<MaterialResource> MaterialResourceBuilder::Build(MaterialGraph& graph)
{
    UniqueRef<MaterialResource> material;
    material->m_CompiledGraph = graph.Compile();
    material->m_Shader = LoadShader("material.glsl", material->m_CompiledGraph->Shaders);
    return material;
}

HK_NAMESPACE_END
