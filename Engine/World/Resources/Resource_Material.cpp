#include "Resource_Material.h"

#include <Engine/Renderer/ShaderLoader.h>

HK_NAMESPACE_BEGIN

MaterialResource::MaterialResource(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    Read(stream, resManager);
}

MaterialResource::~MaterialResource()
{
}

bool MaterialResource::Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    m_pCompiledMaterial = MakeRef<CompiledMaterial>(stream);

    m_Shader = LoadShader("material.glsl", m_pCompiledMaterial->Shaders);

    return true;
}

void MaterialResource::Write(IBinaryStreamWriteInterface& stream, ResourceManager* resManager)
{
    stream.WriteUInt32(MakeResourceMagic(Type, Version));

    m_pCompiledMaterial->Write(stream);
}

void MaterialResource::Upload()
{
    m_GpuMaterial = MakeRef<MaterialGPU>(m_pCompiledMaterial, m_Shader);

    m_Shader.Free();
}

HK_NAMESPACE_END
