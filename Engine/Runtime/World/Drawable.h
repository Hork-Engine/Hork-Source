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

#include "PhysicalBody.h"
#include <Engine/Runtime/Level.h>

#include <Engine/Core/IntrusiveLinkedListMacro.h>

HK_NAMESPACE_BEGIN

struct RenderFrontendDef;

enum DRAWABLE_TYPE
{
    DRAWABLE_UNKNOWN,
    DRAWABLE_STATIC_MESH,
    DRAWABLE_SKINNED_MESH,
    DRAWABLE_PROCEDURAL_MESH
};

/**

Drawable

Base class for drawing surfaces

*/
class Drawable : public PhysicalBody
{
    HK_COMPONENT(Drawable, PhysicalBody)

public:
    TLink<Drawable> Link; // Shadow casters

    /** Render mesh to custom depth-stencil buffer. Render target must have custom depth-stencil buffer enabled */
    bool bCustomDepthStencilPass = false;

    /** Custom depth stencil value for the mesh */
    uint8_t CustomDepthStencilValue = 0;

    /** Experimental object outline */
    bool bOutline = false;

    void SetVisible(bool _Visible);

    bool IsVisible() const;

    /** Set hidden during main render pass */
    void SetHiddenInLightPass(bool _HiddenInLightPass);

    bool IsHiddenInLightPass() const;

    /** Allow mesh to cast shadows on the world */
    void SetCastShadow(bool _CastShadow);

    /** Is cast shadows enabled */
    bool IsCastShadow() const { return m_bCastShadow; }

    void SetQueryGroup(VSD_QUERY_MASK _UserQueryGroup);

    void SetSurfaceFlags(SURFACE_FLAGS Flags);

    SURFACE_FLAGS GetSurfaceFlags() const;

    /** Used for face culling */
    void SetFacePlane(PlaneF const& _Plane);

    PlaneF const& GetFacePlane() const;

    /** Helper. Return true if surface is skinned mesh */
    bool IsSkinnedMesh() const { return m_bSkinnedMesh; }

    /** Force using bounding box specified by SetBoundsOverride() */
    void ForceOverrideBounds(bool _OverrideBounds);

    /** Set bounding box to override object bounds */
    void SetBoundsOverride(BvAxisAlignedBox const& _Bounds);

    void ForceOutdoor(bool _OutdoorSurface);

    bool IsOutdoor() const;

    /** Get overrided bounding box in local space */
    BvAxisAlignedBox const& GetBoundsOverride() const { return m_OverrideBoundingBox; }

    /** Get current local bounds */
    BvAxisAlignedBox const& GetBounds() const;

    /** Get current bounds in world space */
    BvAxisAlignedBox const& GetWorldBounds() const;

    /** Allow raycasting */
    virtual void SetAllowRaycast(bool _AllowRaycast) {}

    bool IsRaycastAllowed() const { return m_bAllowRaycast; }

    /** Raycast the drawable */
    bool Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<TriangleHitResult>& Hits) const;

    /** Raycast the drawable */
    bool RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit) const;

    void SetVisibilityGroup(VISIBILITY_GROUP VisibilityGroup)
    {
        m_Primitive->SetVisibilityGroup(VisibilityGroup);
    }

    VISIBILITY_GROUP GetVisibilityGroup() const
    {
        return m_Primitive->GetVisibilityGroup();
    }

    DRAWABLE_TYPE GetDrawableType() const { return m_DrawableType; }

    /** Called before rendering. Don't call directly. */
    void PreRenderUpdate(RenderFrontendDef const* _Def);

    // Used during culling stage
    uint32_t CascadeMask = 0;

protected:
    Drawable();
    ~Drawable();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;

    void UpdateWorldBounds();

    /** Override to dynamic update mesh data */
    virtual void OnPreRenderUpdate(RenderFrontendDef const* _Def) {}

    DRAWABLE_TYPE m_DrawableType = DRAWABLE_UNKNOWN;

    PrimitiveDef* m_Primitive;

    int m_VisFrame = -1;

    mutable BvAxisAlignedBox m_Bounds;
    mutable BvAxisAlignedBox m_WorldBounds;
    BvAxisAlignedBox         m_OverrideBoundingBox;
    bool                     m_bOverrideBounds : 1;
    bool                     m_bSkinnedMesh : 1;
    bool                     m_bCastShadow : 1;
    bool                     m_bAllowRaycast : 1;
};

HK_NAMESPACE_END
