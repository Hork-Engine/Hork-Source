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
