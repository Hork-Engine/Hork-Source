#include "Resource_MaterialInstance.h"

HK_NAMESPACE_BEGIN

MaterialInstance::~MaterialInstance()
{
}

void MaterialInstance::SetTexture(uint32_t slot, TextureHandle handle)
{
    if (slot < MAX_MATERIAL_TEXTURES)
        m_Textures[slot] = handle;
    else
        LOG("MaterialInstance::SetTexture: Invalid texture slot {}\n", slot);
}

TextureHandle MaterialInstance::GetTexture(uint32_t slot) const
{
    if (slot < MAX_MATERIAL_TEXTURES)
        return m_Textures[slot];
    LOG("MaterialInstance::GetTexture: Invalid texture slot {}\n", slot);
    return {};
}

void MaterialInstance::SetConstant(uint32_t index, float Value)
{
    if (index < MAX_MATERIAL_UNIFORMS)
        m_Constants[index] = Value;
    else
        LOG("MaterialInstance::SetConstant: Invalid index {}\n", index);
}

float MaterialInstance::GetConstant(uint32_t index) const
{
    if (index < MAX_MATERIAL_UNIFORMS)
        return m_Constants[index];
    LOG("MaterialInstance::GetConstant: Invalid index {}\n", index);
    return 0.0f;
}
void MaterialInstance::SetVector(uint32_t index, Float4 const& Value)
{
    if (index < MAX_MATERIAL_UNIFORM_VECTORS)
        *(Float4*)&m_Constants[index * 4] = Value;
    else
        LOG("MaterialInstance::SetVector: Invalid index {}\n", index);
}

Float4 const& MaterialInstance::GetVector(uint32_t index) const
{
    if (index < MAX_MATERIAL_UNIFORM_VECTORS)
        return *(Float4*)&m_Constants[index * 4];
    LOG("MaterialInstance::GetVector: Invalid index {}\n", index);
    return Float4::Zero();
}

HK_NAMESPACE_END
