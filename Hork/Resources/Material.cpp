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

#include "Material.h"

#include <Hork/MaterialGraph/MaterialGraph.h>
#include <Hork/MaterialGraph/MaterialCompiler.h>

HK_NAMESPACE_BEGIN

extern RHI::IDevice* g_RenderDevice;

UniqueRef<MaterialBinary> Material::BeginAsyncLoad(IBinaryStreamReadInterface& stream)
{
    StringView extension = PathUtils::sGetExt(stream.GetName());

    if (!extension.Icmp(".mg"))
    {
        auto graph = MaterialGraph::sLoad(stream);
        if (!graph)
            return {};

#ifdef HK_DEBUG
        bool debugMode = true;
#else
        bool debugMode = false;
#endif

        auto materialCode = graph->Build();
        if (!materialCode)
            return {};

        MaterialCode::TranslationParams translationParams;
        translationParams.IsDebugMode = debugMode;

        return materialCode->Translate(translationParams);
    }

    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return {};
    }

    auto materialData = MakeUnique<MaterialBinary>();
    materialData->Read(stream);

    return materialData;
}

void Material::InitFromData(UniqueRef<MaterialBinary> data)
{
    if (!data)
        return;

    m_Binary = std::move(data);
    m_GpuMaterial = CompileMaterial(g_RenderDevice, *m_Binary.RawPtr());

    m_IsPurged = false;
}

void Material::Load(IBinaryStreamReadInterface& stream)
{
    auto tempData = BeginAsyncLoad(stream);
    if (tempData)
        InitFromData(std::move(tempData));
}

void Material::Write(IBinaryStreamWriteInterface& stream)
{
    if (!m_Binary)
        return;

    stream.WriteUInt32(MakeResourceMagic(Type, Version));

    m_Binary->Write(stream);
}

void Material::Purge()
{
    m_Binary.Reset();
   
    m_IsPurged = true;
}

bool Material::IsCastShadow() const
{
    return m_Binary ? m_Binary->IsCastShadow : false;
}

bool Material::IsTranslucent() const
{
    return m_Binary ? m_Binary->IsTranslucent : false;
}

RENDERING_PRIORITY Material::GetRenderingPriority() const
{
    return m_Binary ? m_Binary->RenderingPriority : RENDERING_PRIORITY_DEFAULT;
}

uint32_t Material::GetTextureCount() const
{
    return m_Binary ? m_Binary->TextureCount : 0;
}

uint32_t Material::GetUniformVectorCount() const
{
    return m_Binary ? m_Binary->UniformVectorCount : 0;
}

HK_NAMESPACE_END
