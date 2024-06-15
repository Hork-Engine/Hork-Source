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
        //BvAxisAlignedBox            BoundingBox;
        //BvAxisAlignedBox            WorldBoundingBox;
    };

    MeshHandle              m_Resource;
    Vector<Surface>         m_Surfaces;
    Ref<SkeletonPose>       m_Pose;
    Ref<ProceduralMesh_ECS> m_ProceduralData;
    bool                    m_Outline = false;
    bool                    m_CastShadow = true;
    uint32_t                m_CascadeMask = 0;

    void                    UpdateBoundingBox();

    void                    DrawDebug(DebugRenderer& renderer);
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

    void BeginPlay()
    {
        m_RenderTransform = GetOwner()->GetWorldTransformMatrix();
        m_RotationMatrix = GetOwner()->GetWorldRotation().ToMatrix3x3();
    }

    void PreRender(PreRenderContext const& context)
    {}

    Float3x4 const& GetRenderTransform() const
    {
        return m_RenderTransform;
    }

    Float3x4 const& GetRenderTransformPrev() const
    {
        return m_RenderTransform;
    }

    Float3x3 const& GetRotationMatrix() const
    {
        return m_RotationMatrix;
    }

private:
    Float3x4 m_RenderTransform;
    Float3x3 m_RotationMatrix;
};

class DynamicMeshComponent : public MeshComponent
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    // Call to skip transform interpolation on this frame (useful for teleporting objects without smooth transition)
    void SkipInterpolation()
    {
        m_Transform[0].Position = m_Transform[1].Position = GetOwner()->GetWorldPosition();
        m_Transform[0].Rotation = m_Transform[1].Rotation = GetOwner()->GetWorldRotation();
        m_Transform[0].Scale    = m_Transform[1].Scale    = GetOwner()->GetWorldScale();

        m_LastFrame = 0;
    }

    void PostTransform()
    {
        auto index = GetWorld()->GetTick().StateIndex;

        m_Transform[index].Position = GetOwner()->GetWorldPosition();
        m_Transform[index].Rotation = GetOwner()->GetWorldRotation();
        m_Transform[index].Scale    = GetOwner()->GetWorldScale();
    }

    void BeginPlay()
    {
        m_Transform[0].Position = m_Transform[1].Position = GetOwner()->GetWorldPosition();
        m_Transform[0].Rotation = m_Transform[1].Rotation = GetOwner()->GetWorldRotation();
        m_Transform[0].Scale    = m_Transform[1].Scale    = GetOwner()->GetWorldScale();

        m_RenderTransform[0].Compose(m_Transform[0].Position, m_Transform[0].Rotation.ToMatrix3x3(), m_Transform[0].Scale);
        m_RenderTransform[1] = m_RenderTransform[0];
    }

    // Update before rendering once per frame
    void PreRender(PreRenderContext const& context)
    {
        if (m_LastFrame == context.FrameNum)
            return;  // already called for this frame

        Float3 position = Math::Lerp (m_Transform[context.Prev].Position, m_Transform[context.Cur].Position, context.Frac);
        Quat   rotation = Math::Slerp(m_Transform[context.Prev].Rotation, m_Transform[context.Cur].Rotation, context.Frac);
        Float3 scale    = Math::Lerp (m_Transform[context.Prev].Scale,    m_Transform[context.Cur].Scale,    context.Frac);

        m_RotationMatrix = rotation.ToMatrix3x3();

        m_RenderTransform[context.FrameNum & 1].Compose(position, m_RotationMatrix, scale);

        if (m_LastFrame + 1 != context.FrameNum)
            m_RenderTransform[(context.FrameNum + 1) & 1] = m_RenderTransform[context.FrameNum & 1];

        m_LastFrame = context.FrameNum;
    }

    Float3x4 const& GetRenderTransform() const
    {
        return m_RenderTransform[m_LastFrame & 1];
    }

    Float3x4 const& GetRenderTransformPrev() const
    {
        return m_RenderTransform[(m_LastFrame + 1) & 1];
    }

    Float3x3 const& GetRotationMatrix() const
    {
        return m_RotationMatrix;
    }

private:
    Transform m_Transform[2];
    Float3x4  m_RenderTransform[2];
    Float3x3  m_RotationMatrix;
    uint32_t  m_LastFrame{0};
};

HK_NAMESPACE_END
