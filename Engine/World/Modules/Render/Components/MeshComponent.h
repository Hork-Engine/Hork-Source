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

#include <Engine/World/Component.h>
#include <Engine/World/Modules/Render/ProceduralMesh.h>
#include <Engine/World/Modules/Skeleton/SkeletonPose.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Resources/Resource_MaterialInstance.h>

#include <Engine/World/GameObject.h>
#include <Engine/World/World.h>

HK_NAMESPACE_BEGIN

class MeshComponent : public Component
{
public:
    struct Surface
    {
        Vector<MaterialInstance*>  Materials;
    };

    MeshHandle                  m_Resource;
    Vector<Surface>             m_Surfaces;
    Ref<ProceduralMesh_ECS>     m_ProceduralData;
    bool                        m_Outline = false;
    bool                        m_CastShadow = true;
    uint32_t                    m_CascadeMask = 0;

    void                        SetLocalBoundingBox(BvAxisAlignedBox const& boundingBox);
    BvAxisAlignedBox const&     GetLocalBoundingBox() const { return m_LocalBoundingBox; }

    /// The bounding box is updated in BeginPlay for static and dynamic meshes, and at every update before rendering for dynamic meshes.
    BvAxisAlignedBox const&     GetWorldBoundingBox() const { return m_WorldBoundingBox; }

    /// Force update for world bonding box
    void                        UpdateWorldBoundingBox();

    void                        DrawDebug(DebugRenderer& renderer);

protected:
    BvAxisAlignedBox            m_LocalBoundingBox;
    BvAxisAlignedBox            m_WorldBoundingBox;
};

struct PreRenderContext
{
    uint32_t FrameNum;
    uint32_t Prev;
    uint32_t Cur;
    float    Frac;
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
    void                        PreRender(PreRenderContext const& context) {}

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

    Ref<SkeletonPose>           m_Pose;

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

private:
    void                        UpdateSkinningMatrices();

    Transform                   m_Transform[2];
    Float3x4                    m_RenderTransform[2];
    Float3x3                    m_RotationMatrix;
    uint32_t                    m_LastFrame{0};
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
