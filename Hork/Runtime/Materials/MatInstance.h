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

#pragma once

#include <Hork/Resources/Material.h>
#include <Hork/Resources/Texture.h>

#include <Hork/Renderer/RenderDefs.h>

HK_NAMESPACE_BEGIN

/*

---------------------------------------------------------------------------------------------------

                                                              Material resource
                                                   ________________________________________
                                                  |                                        |
                       
MaterialGraph   ---->  Code injections
  (nodes)
  
                             +               ---->  SPIR-V code ---->    GpuMaterial
                                                                         (pipelines)
                       material.glsl
                       
                       
                             +
                       
                       Permutation predefines

 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ | ^^^^^^^^^^^^^^^^^^^^^^^^
                          OFFLINE                                 |        RUNTIME

---------------------------------------------------------------------------------------------------


Material Manager                          +------ MatInstance 0  = resource handle, textures, constants
                                          |
    Material library 0     ---------------+------ ....
    Material library 1                    |
                                          +------ MatInstance N
    ....

*/

class MatInstance final : public IntrusiveRefCounter<MatInstance>
{
public:
    void                    SetResource(MaterialRef resource) { m_Resource = std::move(resource); }
    Material*               GetResource() const { return m_Resource.RawPtr(); }

    void                    SetTexture(uint32_t slot, TextureRef handle);
    TextureRef              GetTexture(uint32_t slot) const;

    void                    SetConstant(uint32_t index, float Value);
    float                   GetConstant(uint32_t index) const;

    void                    SetVector(uint32_t index, Float4 const& Value);
    Float4 const&           GetVector(uint32_t index) const;

    MaterialFrameData*      PreRender(int frameNumber);

private:
    MaterialRef             m_Resource;
    TextureRef              m_Textures[MAX_MATERIAL_TEXTURES];
    float                   m_Constants[MAX_MATERIAL_UNIFORMS] = {};
    MaterialFrameData*      m_FrameData{};
    int                     m_VisFrame = -1;
};

using MatInstanceRef = IntrusiveRef<MatInstance>;

HK_NAMESPACE_END
