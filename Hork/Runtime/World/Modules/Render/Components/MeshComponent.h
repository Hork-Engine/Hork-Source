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

#include <Hork/Runtime/World/Component.h>
#include <Hork/Runtime/World/Modules/Render/ProceduralMesh.h>
#include <Hork/Runtime/World/Modules/Skeleton/Components/SkeletonPoseComponent.h>
#include <Hork/Runtime/Materials/Material.h>
#include <Hork/Runtime/World/GameObject.h>
#include <Hork/Runtime/World/World.h>

#include <Hork/Resources/Resource_Mesh.h>

HK_NAMESPACE_BEGIN

class MeshComponent : public Component
{
public:
    void                        SetMesh(MeshHandle handle) { m_Resource = handle; }
    MeshHandle                  GetMesh() const { return m_Resource; }

    void                        SetProceduralMesh(ProceduralMesh* proceduralMesh) { m_ProceduralData = proceduralMesh; }
    ProceduralMesh*             GetProceduralMesh() { return m_ProceduralData; }

    void                        SetMaterial(Material* material);
    void                        SetMaterial(uint32_t index, Material* material);
    Material*                   GetMaterial(uint32_t index);
    void                        SetMaterialCount(uint32_t count);
    uint32_t                    GetMaterialCount() const;

    // NOTE: In the future the outline can be achieved using post-processing materials
    void                        SetOutline(bool enable) { m_Outline = enable; }
    bool                        HasOutline() const { return m_Outline; }

    void                        SetCastShadow(bool castShadow) { m_CastShadow = castShadow; }
    bool                        IsCastShadow() const { return m_CastShadow; }

    void                        SetCascadeMask(uint32_t cascadeMask) { m_CascadeMask = cascadeMask; }
    uint32_t                    GetCascadeMask() const { return m_CascadeMask; }

    void                        SetVisibilityLayer(uint8_t layer) { m_VisibilityLayer = Math::Min(layer, uint8_t(31)); }
    uint8_t                     GetVisibilityLayer() const { return m_VisibilityLayer; }

    void                        SetLocalBoundingBox(BvAxisAlignedBox const& boundingBox);
    BvAxisAlignedBox const&     GetLocalBoundingBox() const { return m_LocalBoundingBox; }

    /// The bounding box is updated in BeginPlay for static and dynamic meshes, and at every update before rendering for dynamic meshes.
    BvAxisAlignedBox const&     GetWorldBoundingBox() const { return m_WorldBoundingBox; }

    /// Force update for world bonding box
    void                        UpdateWorldBoundingBox();

    void                        DrawDebug(DebugRenderer& renderer);

protected:
    MeshHandle                  m_Resource;
    Vector<Ref<Material>>       m_Materials; // NOTE: pointers will be replaced by handles!
    Ref<ProceduralMesh>         m_ProceduralData;
    uint8_t                     m_VisibilityLayer = 0;
    bool                        m_Outline = false;
    bool                        m_CastShadow = true;
    uint32_t                    m_CascadeMask = 0;
    BvAxisAlignedBox            m_LocalBoundingBox;
    BvAxisAlignedBox            m_WorldBoundingBox;
};

class StaticMeshComponent : public MeshComponent
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    // Internal

    void                        BeginPlay();
    void                        PreRender(struct PreRenderContext const& context) {}

    Float3x4 const&             GetRenderTransform() const { return m_RenderTransform; }
    Float3x4 const&             GetRenderTransformPrev() const { return m_RenderTransform; }

    Float3x3 const&             GetRotationMatrix() const { return m_RotationMatrix; }

private:
    Float3x4                    m_RenderTransform;
    Float3x3                    m_RotationMatrix;
};

class DynamicMeshComponent : public MeshComponent
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    /// Call to skip transform interpolation on this frame (useful for teleporting objects without smooth transition)
    void                        SkipInterpolation();
    
    // Internal

    void                        BeginPlay();
    void                        PostTransform();
    void                        PreRender(PreRenderContext const& context);

    Float3x4 const&             GetRenderTransform() const { return m_RenderTransform[m_LastFrame & 1]; }
    Float3x4 const&             GetRenderTransformPrev() const { return m_RenderTransform[(m_LastFrame + 1) & 1]; }

    Float3x3 const&             GetRotationMatrix() const { return m_RotationMatrix; }

    void                        DrawDebug(DebugRenderer& renderer);

    struct StreamBuffer
    {
        // GPU memory offset/size for the mesh skin
        size_t      Offset;
        size_t      OffsetP;
        size_t      Size;
    };

    struct SkinningData
    {
        Ref<SkeletonPose>       Pose;
        // Skinning matrices from previous frame
        Vector<Float3x4>        SkinningMatrices;
        Vector<StreamBuffer>    StreamBuffers;
    };

    SkinningData const&         GetSkinningData() const { return m_SkinningData; }

private:
    void                        UpdateSkinningMatrices();

    Handle32<SkeletonPoseComponent> m_PoseComponent;
    Transform                   m_Transform[2];
    Float3x4                    m_RenderTransform[2];
    Float3x3                    m_RotationMatrix;
    uint32_t                    m_LastFrame{0};
    SkinningData                m_SkinningData;
};

namespace TickGroup_PostTransform
{
    template <>
    HK_INLINE void InitializeTickFunction<DynamicMeshComponent>(TickFunctionDesc& desc)
    {
        desc.TickEvenWhenPaused = true;
    }
}

HK_NAMESPACE_END
