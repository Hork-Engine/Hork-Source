/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

class ALevel;
struct SRenderFrontendDef;

enum ERenderOrder
{
    RENDER_ORDER_WEAPON = 0,
    RENDER_ORDER_DEFAULT = 1,
    RENDER_ORDER_SKYBOX = 255,
};

enum EVSDPass
{
    VSD_PASS_IGNORE     = 0,
    VSD_PASS_ALL        = ~0,
    VSD_PASS_PORTALS    = 1,
    VSD_PASS_FACE_CULL  = 2,
    VSD_PASS_BOUNDS     = 4,
    VSD_PASS_CUSTOM_VISIBLE_STEP = 8,
    VSD_PASS_VIS_MARKER = 16,
    VSD_PASS_DEFAULT    = VSD_PASS_PORTALS | VSD_PASS_BOUNDS,
};

struct SAreaLink
{
    ALevel * Level;
    int AreaNum;
    int Index;
};

using AAreaLinks = TPodArray< SAreaLink, 4 >;

/**

ADrawable

Base class for drawing surfaces

*/
class ANGIE_API ADrawable : public APhysicalBody {
    AN_COMPONENT( ADrawable, APhysicalBody )

    friend class ALevel;
    friend class AWorld;
    friend class ARenderWorld;

public:
    enum { RENDERING_GROUP_DEFAULT = 1 };

    /** Rendering group to filter mesh during rendering */
    int RenderingGroup;

    int RenderingOrder = RENDER_ORDER_DEFAULT;

    /** Visible surface determination alogrithm */
    int VSDPasses = VSD_PASS_DEFAULT;

    /** Marker for VSD_PASS_VIS_MARKER */
    int VisMarker;

    /** Internal. Used by frontend to filter rendered meshes. */
    int RenderMark;

    /** Used for VSD_FACE_CULL */
    PlaneF FacePlane;

    /** Render during main pass */
    bool bLightPass;

    /** Helper. Return true if surface is skinned mesh */
    bool IsSkinnedMesh() const { return bSkinnedMesh; }

    /** Force using bounding box specified by SetBoundsOverride() */
    void ForceOverrideBounds( bool _OverrideBounds );

    /** Set bounding box to override object bounds */
    void SetBoundsOverride( BvAxisAlignedBox const & _Bounds );

    /** Get overrided bounding box in local space */
    BvAxisAlignedBox const & GetBoundsOverride() const { return OverrideBoundingBox; }

    /** Get current local bounds */
    BvAxisAlignedBox const & GetBounds() const;

    /** Get current bounds in world space */
    BvAxisAlignedBox const & GetWorldBounds() const;

    void ForceOutdoor( bool _OutdoorSurface );

    bool IsOutdoor() const { return bIsOutdoor; }

    // TODO: SetVisible, IsVisible ?

    ADrawable * GetNextDrawable() { return Next; }
    ADrawable * GetPrevDrawable() { return Prev; }

    /** Used for VSD_PASS_CUSTOM_VISIBLE_STEP algorithm */
    virtual void RenderFrontend_CustomVisibleStep( SRenderFrontendDef * _Def, bool & _OutVisibleFlag ) {}

protected:
    ADrawable();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;

    virtual void OnLazyBoundsUpdate() {}

    void MarkWorldBoundsDirty();

    mutable BvAxisAlignedBox Bounds;
    mutable BvAxisAlignedBox WorldBounds;
    mutable bool            bWorldBoundsDirty;
    BvAxisAlignedBox        OverrideBoundingBox;
    bool                    bOverrideBounds;

    AAreaLinks              InArea; // list of intersected areas
    bool                    bLazyBoundsUpdate;
    bool                    bIsOutdoor;
    bool                    bSkinnedMesh;

    ADrawable * Next;
    ADrawable * Prev;

    ADrawable * NextUpdDrawable;
    ADrawable * PrevUpdDrawable;

    //static ADrawable * DrawableUpdateList;
    //static ADrawable * DrawableUpdateListTail;
};
