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

#include "SceneComponent.h"

#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class ALevel;

struct SAreaLink {
    ALevel * Level;
    int AreaNum;
    int Index;
};

using AAreaLinks = TPodArray< SAreaLink, 4 >;

/*

ASpatialObject

Scene component with bounds.
Spatial objects are located in visareas of the level.

*/
class ASpatialObject : public ASceneComponent {
    AN_COMPONENT( ASpatialObject, ASceneComponent )

    friend class ALevel;

public:
    // Force using bounding box specified by SetBoundsOverride()
    void ForceOverrideBounds( bool _OverrideBounds );

    // Set bounding box to override object bounds
    void SetBoundsOverride( BvAxisAlignedBox const & _Bounds );

    // Get overrided bounding box in local space
    BvAxisAlignedBox const & GetBoundsOverride() const { return OverrideBoundingBox; }

    // Get current local bounds
    BvAxisAlignedBox const & GetBounds() const;

    // Get current bounds in world space
    BvAxisAlignedBox const & GetWorldBounds() const;

    void ForceOutdoor( bool _OutdoorSurface );

    bool IsOutdoor() const { return bIsOutdoor; }

    // TODO: SetVisible, IsVisible And only add visible objects to areas!!!

    void MarkAreaDirty();

    static void _UpdateSurfaceAreas();

protected:
    ASpatialObject();

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

    ASpatialObject * NextDirty;
    ASpatialObject * PrevDirty;

    static ASpatialObject * DirtyList;
    static ASpatialObject * DirtyListTail;
};
