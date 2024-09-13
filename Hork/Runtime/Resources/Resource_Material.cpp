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

#include <Hork/MaterialGraph/MaterialGraph.h>
#include <Hork/MaterialGraph/MaterialCompiler.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

UniqueRef<MaterialResource> MaterialResource::sLoad(IBinaryStreamReadInterface& stream)
{
    StringView extension = PathUtils::sGetExt(stream.GetName());

    if (!extension.Icmp(".mg"))
    {
        auto graph = MaterialGraph::sLoad(stream);
        if (!graph)
            return {};

        return MaterialResourceBuilder().Build(*graph.RawPtr());
    }

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

    m_Binary = MakeUnique<MaterialBinary>();
    m_Binary->Read(stream);

    return true;
}

void MaterialResource::Write(IBinaryStreamWriteInterface& stream)
{
    stream.WriteUInt32(MakeResourceMagic(Type, Version));

    m_Binary->Write(stream);
}

void MaterialResource::Upload()
{
    if (m_Binary)
        m_GpuMaterial = CompileMaterial(GameApplication::sGetRenderDevice(), *m_Binary.RawPtr());
}

bool MaterialResource::IsCastShadow() const
{
    return m_Binary->IsCastShadow;
}

bool MaterialResource::IsTranslucent() const
{
    return m_Binary->IsTranslucent;
}

RENDERING_PRIORITY MaterialResource::GetRenderingPriority() const
{
    return m_Binary->RenderingPriority;
}

uint32_t MaterialResource::GetTextureCount() const
{
    return m_Binary->TextureCount;
}

uint32_t MaterialResource::GetUniformVectorCount() const
{
    return m_Binary->UniformVectorCount;
}

UniqueRef<MaterialResource> MaterialResourceBuilder::Build(MaterialGraph& graph)
{
    auto materialCode = graph.Build();
    if (!materialCode)
        return {};

    auto material = MakeUnique<MaterialResource>();
    material->m_Binary = materialCode->Translate();

    return material;
}

HK_NAMESPACE_END
