#pragma once

#include "ResourceHandle.h"
#include "ResourceBase.h"

#include <Engine/Core/BinaryStream.h>

#include <Engine/Renderer/GpuMaterial.h>

HK_NAMESPACE_BEGIN

class MaterialResource : public ResourceBase
{
public:
    static const uint8_t Type = RESOURCE_MATERIAL;
    static const uint8_t Version = 1;

    MaterialResource() = default;
    MaterialResource(IBinaryStreamReadInterface& stream, class ResourceManager* resManager);
    ~MaterialResource();

    bool Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager);
    void Write(IBinaryStreamWriteInterface& stream, ResourceManager* resManager);

    void Upload() override;

    TRef<MaterialGPU> m_GpuMaterial;
    TRef<CompiledMaterial> m_pCompiledMaterial;

private:
    String m_Shader;
};

using MaterialHandle = ResourceHandle<MaterialResource>;

HK_NAMESPACE_END
