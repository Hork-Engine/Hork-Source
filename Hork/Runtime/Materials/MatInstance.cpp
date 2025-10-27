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

#include "MatInstance.h"

#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void MatInstance::SetTexture(uint32_t slot, TextureRef handle)
{
    if (slot < MAX_MATERIAL_TEXTURES)
        m_Textures[slot] = std::move(handle);
    else
        LOG("MatInstance::SetTexture: Invalid texture slot {}\n", slot);
}

TextureRef MatInstance::GetTexture(uint32_t slot) const
{
    if (slot < MAX_MATERIAL_TEXTURES)
        return m_Textures[slot];
    LOG("MatInstance::GetTexture: Invalid texture slot {}\n", slot);
    return {};
}

void MatInstance::SetConstant(uint32_t index, float Value)
{
    if (index < MAX_MATERIAL_UNIFORMS)
        m_Constants[index] = Value;
    else
        LOG("MatInstance::SetConstant: Invalid index {}\n", index);
}

float MatInstance::GetConstant(uint32_t index) const
{
    if (index < MAX_MATERIAL_UNIFORMS)
        return m_Constants[index];
    LOG("MatInstance::GetConstant: Invalid index {}\n", index);
    return 0.0f;
}

void MatInstance::SetVector(uint32_t index, Float4 const& Value)
{
    if (index < MAX_MATERIAL_UNIFORM_VECTORS)
        *(Float4*)&m_Constants[index * 4] = Value;
    else
        LOG("MatInstance::SetVector: Invalid index {}\n", index);
}

Float4 const& MatInstance::GetVector(uint32_t index) const
{
    if (index < MAX_MATERIAL_UNIFORM_VECTORS)
        return *(Float4*)&m_Constants[index * 4];
    LOG("MatInstance::GetVector: Invalid index {}\n", index);
    return Float4::sZero();
}

MaterialFrameData* MatInstance::PreRender(int frameNumber)
{
    if (m_VisFrame == frameNumber)
        return m_FrameData;

    if (!m_Resource || m_Resource->IsPurged())
        return nullptr;

    m_FrameData = (MaterialFrameData*)GameApplication::sGetFrameLoop().AllocFrameMem(sizeof(MaterialFrameData));
    m_VisFrame = frameNumber;

    m_FrameData->Material    = m_Resource->GetGpuMaterial();
    m_FrameData->NumTextures = m_Resource->GetTextureCount();

    HK_ASSERT(m_FrameData->NumTextures <= MAX_MATERIAL_TEXTURES);

    for (int i = 0, count = m_FrameData->NumTextures; i < count; ++i)
    {
        TextureRef& texture = m_Textures[i];

        if (!texture || texture->IsPurged())
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

    m_FrameData->NumUniformVectors = m_Resource->GetUniformVectorCount();
    Core::Memcpy(m_FrameData->UniformVectors, m_Constants, sizeof(Float4) * m_FrameData->NumUniformVectors);

    return m_FrameData;
}

HK_NAMESPACE_END
