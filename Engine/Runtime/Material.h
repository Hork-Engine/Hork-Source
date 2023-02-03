/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "Texture.h"
#include "VirtualTextureResource.h"

#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Renderer/GpuMaterial.h>

HK_NAMESPACE_BEGIN

class MaterialInstance;

/**

Material

*/
class Material : public Resource
{
    HK_CLASS(Material, Resource)

public:
    Material();
    Material(CompiledMaterial* pCompiledMaterial);

    ~Material();

    /** Create a new material instance. */
    MaterialInstance* Instantiate();

    /** Find texture slot by name */
    uint32_t GetTextureSlotByName(StringView Name) const;

    /** Find constant offset by name */
    uint32_t GetConstantOffsetByName(StringView Name) const;

    uint32_t NumTextureSlots() const;

    uint32_t NumUniformVectors() const { return m_pCompiledMaterial->NumUniformVectors; }

    MATERIAL_TYPE GetType() const { return m_pCompiledMaterial->Type; }

    BLENDING_MODE GetBlendingMode() const { return m_pCompiledMaterial->Blending; }

    TESSELLATION_METHOD GetTessellationMethod() const { return m_pCompiledMaterial->TessellationMethod; }

    uint8_t GetRenderingPriority() const { return m_pCompiledMaterial->RenderingPriority; }

    /** Have vertex deformation in vertex stage. This flag allow renderer to optimize pipeline switching
    during rendering. */
    bool HasVertexDeform() const { return m_pCompiledMaterial->bHasVertexDeform; }

    /** Experimental. Depth testing. */
    bool IsDepthTestEnabled() const { return m_pCompiledMaterial->bDepthTest_EXPERIMENTAL; }

    /** Shadow casting */
    bool IsShadowCastEnabled() const { return !m_pCompiledMaterial->bNoCastShadow; }

    /** Alpha masking */
    bool IsAlphaMaskingEnabled() const { return m_pCompiledMaterial->bAlphaMasking;  }

    /** Shadow map masking */
    bool IsShadowMapMaskingEnabled() const { return m_pCompiledMaterial->bShadowMapMasking; }

    /** Tessellation for shadow maps */
    bool IsDisplacementAffectShadow() const { return m_pCompiledMaterial->bDisplacementAffectShadow; }

    /** Is translusent */
    bool IsTranslucent() const { return m_pCompiledMaterial->bTranslucent; }

    /** Is face culling disabled */
    bool IsTwoSided() const { return m_pCompiledMaterial->bTwoSided; }

    MaterialGPU* GetGPUResource() { return m_GpuMaterial; }

    void UpdateGpuMaterial()
    {
        m_GpuMaterial = MakeRef<MaterialGPU>(m_pCompiledMaterial);
    }

    static void UpdateGpuMaterials()
    {
        for (TListIterator<Material> it(m_MaterialRegistry); it; it++)
        {
            it->UpdateGpuMaterial();
        }
    }

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView _Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Materials/Unlit"; }

private:
    TRef<MaterialGPU>      m_GpuMaterial;
    TRef<CompiledMaterial> m_pCompiledMaterial;

    TLink<Material>        Link;
    static TList<Material> m_MaterialRegistry;

    friend struct TList<Material>;
    friend struct TListIterator<Material>;
};


/**

Material Instance

*/
class MaterialInstance : public Resource
{
    HK_CLASS(MaterialInstance, Resource)

public:
    MaterialInstance();
    MaterialInstance(Material* pMaterial);

    void SetTexture(StringView Name, Texture* pView);
    void SetTexture(StringView Name, TextureView* pTexture);

    void SetTexture(uint32_t Slot, Texture* pView);
    void SetTexture(uint32_t Slot, TextureView* pTexture);

    TextureView* GetTexture(StringView Name);
    TextureView* GetTexture(uint32_t Slot);

    void UnsetTextures();

    void SetConstant(StringView Name, float Value);

    void SetConstant(uint32_t Offset, float Value);

    float GetConstant(StringView Name) const;

    float GetConstant(uint32_t Offset) const;

    void SetVector(StringView Name, Float4 const& Value);

    void SetVector(uint32_t Offset, Float4 const& Value);

    Float4 const& GetVector(StringView Name) const;

    Float4 const& GetVector(uint32_t Offset) const;

    uint32_t GetTextureSlotByName(StringView Name) const;

    uint32_t GetConstantOffsetByName(StringView Name) const;

    uint32_t NumTextureSlots() const;

    /** Get material. Never return null. */
    Material* GetMaterial() const;

    // Experimental:
    void SetVirtualTexture(VirtualTextureResource* VirtualTex);

    /** Internal. Used by render frontend */
    MaterialFrameData* PreRenderUpdate(class FrameLoop* FrameLoop, int FrameNumber);

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/MaterialInstance/Default"; }

    bool LoadTextVersion(IBinaryStreamReadInterface& Stream);

private:
    TRef<Material>               m_pMaterial;
    TRef<TextureView>            m_Textures[MAX_MATERIAL_TEXTURES];
    TRef<VirtualTextureResource> m_VirtualTexture;
    union
    {
        /** Instance uniforms */
        float m_Uniforms[MAX_MATERIAL_UNIFORMS];

        /** Instance uniform vectors */
        Float4 m_UniformVectors[MAX_MATERIAL_UNIFORM_VECTORS];
    };
    MaterialFrameData* m_FrameData = nullptr;
    int                m_VisFrame  = -1;
};

HK_NAMESPACE_END
