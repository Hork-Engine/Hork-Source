/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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
#include "Level.h"

#include <Core/IntrusiveLinkedListMacro.h>

struct SRenderFrontendDef;

enum EDrawableType
{
    DRAWABLE_UNKNOWN,
    DRAWABLE_STATIC_MESH,
    DRAWABLE_SKINNED_MESH,
    DRAWABLE_PROCEDURAL_MESH
};

/**

ADrawable

Base class for drawing surfaces

*/
class ADrawable : public APhysicalBody
{
    HK_COMPONENT(ADrawable, APhysicalBody)

public:
    TLink<ADrawable> Link; // Shadow casters

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
    bool IsCastShadow() const { return bCastShadow; }

    void SetQueryGroup(VSD_QUERY_MASK _UserQueryGroup);

    void SetSurfaceFlags(uint8_t Flags);

    uint8_t GetSurfaceFlags() const;

    /** Used for face culling */
    void SetFacePlane(PlaneF const& _Plane);

    PlaneF const& GetFacePlane() const;

    /** Helper. Return true if surface is skinned mesh */
    bool IsSkinnedMesh() const { return bSkinnedMesh; }

    /** Force using bounding box specified by SetBoundsOverride() */
    void ForceOverrideBounds(bool _OverrideBounds);

    /** Set bounding box to override object bounds */
    void SetBoundsOverride(BvAxisAlignedBox const& _Bounds);

    void ForceOutdoor(bool _OutdoorSurface);

    bool IsOutdoor() const;

    /** Get overrided bounding box in local space */
    BvAxisAlignedBox const& GetBoundsOverride() const { return OverrideBoundingBox; }

    /** Get current local bounds */
    BvAxisAlignedBox const& GetBounds() const;

    /** Get current bounds in world space */
    BvAxisAlignedBox const& GetWorldBounds() const;

    /** Allow raycasting */
    virtual void SetAllowRaycast(bool _AllowRaycast) {}

    bool IsRaycastAllowed() const { return bAllowRaycast; }

    /** Raycast the drawable */
    bool Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<STriangleHitResult>& Hits) const;

    /** Raycast the drawable */
    bool RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, STriangleHitResult& Hit) const;

    void SetVisibilityGroup(VISIBILITY_GROUP VisibilityGroup)
    {
        Primitive->SetVisibilityGroup(VisibilityGroup);
    }

    VISIBILITY_GROUP GetVisibilityGroup() const
    {
        return Primitive->GetVisibilityGroup();
    }

    EDrawableType GetDrawableType() const { return DrawableType; }

    /** Called before rendering. Don't call directly. */
    void PreRenderUpdate(SRenderFrontendDef const* _Def);

    // Used during culling stage
    uint32_t CascadeMask = 0;

protected:
    ADrawable();
    ~ADrawable();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;

    void UpdateWorldBounds();

    /** Override to dynamic update mesh data */
    virtual void OnPreRenderUpdate(SRenderFrontendDef const* _Def) {}

    EDrawableType DrawableType = DRAWABLE_UNKNOWN;

    SPrimitiveDef* Primitive;

    int VisFrame = -1;

    mutable BvAxisAlignedBox Bounds;
    mutable BvAxisAlignedBox WorldBounds;
    BvAxisAlignedBox         OverrideBoundingBox;
    bool                     bOverrideBounds : 1;
    bool                     bSkinnedMesh : 1;
    bool                     bCastShadow : 1;
    bool                     bAllowRaycast : 1;
};
