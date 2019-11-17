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

#include "SpatialObject.h"

/*

AClusteredObject

*/
class AClusteredObject : public ASceneComponent {
    AN_COMPONENT( AClusteredObject, ASceneComponent )

    friend class ALevel;

public:
    enum { RENDERING_GROUP_DEFAULT = 1 };

    /** Rendering group to filter lights during rendering */
    int RenderingGroup;

    void ForceOutdoor( bool _OutdoorSurface );

    bool IsOutdoor() const { return bIsOutdoor; }

    BvSphereSSE const & GetSphereWorldBounds() const;
    BvAxisAlignedBox const & GetAABBWorldBounds() const;
    BvOrientedBox const & GetOBBWorldBounds() const;
    Float4x4 const & GetOBBTransformInverse() const;

    // TODO: SetVisible, IsVisible And only add visible objects to areas!!!

    void MarkAreaDirty();

    static void _UpdateSurfaceAreas();

protected:

    AClusteredObject();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    //void OnTransformDirty() override;

    BvSphere SphereWorldBounds;
    BvAxisAlignedBox AABBWorldBounds;
    BvOrientedBox OBBWorldBounds;
    Float4x4 OBBTransformInverse;

    AAreaLinks InArea; // list of intersected areas
    bool bIsOutdoor;

    AClusteredObject * NextDirty;
    AClusteredObject * PrevDirty;

    static AClusteredObject * DirtyList;
    static AClusteredObject * DirtyListTail;
};
