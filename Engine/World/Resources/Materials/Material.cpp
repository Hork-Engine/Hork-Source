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

#include "Material.h"

#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

Material::Material(StringView name) :
    m_Name(name)
{}

void Material::SetTexture(uint32_t slot, TextureHandle handle)
{
    if (slot < MAX_MATERIAL_TEXTURES)
        m_Textures[slot] = handle;
    else
        LOG("Material::SetTexture: Invalid texture slot {}\n", slot);
}

TextureHandle Material::GetTexture(uint32_t slot) const
{
    if (slot < MAX_MATERIAL_TEXTURES)
        return m_Textures[slot];
    LOG("Material::GetTexture: Invalid texture slot {}\n", slot);
    return {};
}

void Material::SetConstant(uint32_t index, float Value)
{
    if (index < MAX_MATERIAL_UNIFORMS)
        m_Constants[index] = Value;
    else
        LOG("Material::SetConstant: Invalid index {}\n", index);
}

float Material::GetConstant(uint32_t index) const
{
    if (index < MAX_MATERIAL_UNIFORMS)
        return m_Constants[index];
    LOG("Material::GetConstant: Invalid index {}\n", index);
    return 0.0f;
}

void Material::SetVector(uint32_t index, Float4 const& Value)
{
    if (index < MAX_MATERIAL_UNIFORM_VECTORS)
        *(Float4*)&m_Constants[index * 4] = Value;
    else
        LOG("Material::SetVector: Invalid index {}\n", index);
}

Float4 const& Material::GetVector(uint32_t index) const
{
    if (index < MAX_MATERIAL_UNIFORM_VECTORS)
        return *(Float4*)&m_Constants[index * 4];
    LOG("Material::GetVector: Invalid index {}\n", index);
    return Float4::Zero();
}

MaterialFrameData* Material::PreRender(int frameNumber)
{
    if (m_VisFrame == frameNumber)
        return m_FrameData;

    MaterialResource* resource = GameApplication::GetResourceManager().TryGet(m_Resource);
    if (!resource)
        return nullptr;

    m_FrameData = (MaterialFrameData*)GameApplication::GetFrameLoop().AllocFrameMem(sizeof(MaterialFrameData));
    m_VisFrame = frameNumber;

    m_FrameData->Material    = resource->GetGpuMaterial();
    m_FrameData->NumTextures = resource->GetTextureCount();

    HK_ASSERT(m_FrameData->NumTextures <= MAX_MATERIAL_TEXTURES);

    for (int i = 0, count = m_FrameData->NumTextures; i < count; ++i)
    {
        TextureHandle texHandle = m_Textures[i];

        TextureResource* texture = GameApplication::GetResourceManager().TryGet(texHandle);
        if (!texture)
        {
            m_FrameData = nullptr;
            return nullptr;
        }

        m_FrameData->Textures[i] = texture->GetTextureGPU();
        //HK_ASSERT(m_FrameData->Textures[i]);
        if (!m_FrameData->Textures[i])
        {
            m_FrameData = nullptr;
            return nullptr;
        }
    }

    m_FrameData->NumUniformVectors = resource->GetUniformVectorCount();
    Core::Memcpy(m_FrameData->UniformVectors, m_Constants, sizeof(Float4) * m_FrameData->NumUniformVectors);

    return m_FrameData;
}

HK_NAMESPACE_END
