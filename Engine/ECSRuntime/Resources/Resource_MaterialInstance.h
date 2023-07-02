#pragma once

#include "Resource_Material.h"
#include "Resource_Texture.h"

#include <Engine/Renderer/RenderDefs.h>

HK_NAMESPACE_BEGIN

// TODO: Reference counting
class MaterialInstance
{
public:
    MaterialInstance(StringView name) :
        m_Name(name)
    {}
    ~MaterialInstance();

    void SetTexture(uint32_t slot, TextureHandle handle);
    TextureHandle GetTexture(uint32_t slot) const;

    void SetConstant(uint32_t index, float Value);
    float GetConstant(uint32_t index) const;

    void SetVector(uint32_t index, Float4 const& Value);
    Float4 const& GetVector(uint32_t index) const;

    MaterialHandle GetMaterial() const { return m_Material; }

    String m_Name;

    MaterialHandle m_Material;
    TextureHandle  m_Textures[MAX_MATERIAL_TEXTURES];
    float m_Constants[MAX_MATERIAL_UNIFORMS] = {};

    MaterialFrameData* m_FrameData{};
    int m_VisFrame = -1;
};

HK_NAMESPACE_END
