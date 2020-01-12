/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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
#include <World/Public/Level.h>

struct SRenderFrontendDef;

enum ERenderOrder
{
    RENDER_ORDER_WEAPON = 0,
    RENDER_ORDER_DEFAULT = 1,
    RENDER_ORDER_SKYBOX = 255
};

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
    int RenderingOrder = RENDER_ORDER_DEFAULT;

    /** Visibility group to filter drawables during rendering */
    void SetVisibilityGroup( int InVisibilityGroup );

    int GetVisibilityGroup() const;

    void SetVisible( bool _Visible );

    bool IsVisible() const;

    /** Set hidden during main render pass */
    void SetHiddenInLightPass( bool _HiddenInLightPass );

    bool IsHiddenInLightPass() const;

    void SetQueryGroup( int _UserQueryGroup );

    void SetFaceCull( bool bFaceCull );

    bool GetFaceCull() const;

    /** Used for VSD_FACE_CULL */
    void SetFacePlane( PlaneF const & _Plane );

    PlaneF const & GetFacePlane() const;

    /** Helper. Return true if surface is skinned mesh */
    bool IsSkinnedMesh() const { return bSkinnedMesh; }

    /** Force using bounding box specified by SetBoundsOverride() */
    void ForceOverrideBounds( bool _OverrideBounds );

    /** Set bounding box to override object bounds */
    void SetBoundsOverride( BvAxisAlignedBox const & _Bounds );

    void ForceOutdoor( bool _OutdoorSurface );

    bool IsOutdoor() const;

    void SetMovable( bool _Movable );

    bool IsMovable() const;

    bool HasPreRenderUpdate() const { return bPreRenderUpdate; }

    /** Get overrided bounding box in local space */
    BvAxisAlignedBox const & GetBoundsOverride() const { return OverrideBoundingBox; }

    /** Get current local bounds */
    BvAxisAlignedBox const & GetBounds() const;

    /** Get current bounds in world space */
    BvAxisAlignedBox const & GetWorldBounds() const;

    SPrimitiveDef const * GetPrimitive() const { return &Primitive; }

    ADrawable * GetNextDrawable() { return Next; }
    ADrawable * GetPrevDrawable() { return Prev; }

    /** Called before rendering if bPreRenderUpdate is true */
    virtual void OnPreRenderUpdate( SRenderFrontendDef * _Def ) {}

protected:
    ADrawable();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;

    //virtual void OnLazyBoundsUpdate() {}

    void UpdateWorldBounds();

    mutable BvAxisAlignedBox Bounds;
    mutable BvAxisAlignedBox WorldBounds;
    BvAxisAlignedBox        OverrideBoundingBox;
    bool                    bOverrideBounds : 1;
    bool                    bLazyBoundsUpdate : 1;
    bool                    bSkinnedMesh : 1;
    bool                    bPreRenderUpdate : 1;

    ADrawable * Next;
    ADrawable * Prev;

    //TRef< VSDPrimitive >    Primitive;
    SPrimitiveDef Primitive;
};
