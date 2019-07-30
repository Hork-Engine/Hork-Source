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

#include <Engine/GameEngine/Public/SpatialObject.h>
#include <Engine/GameEngine/Public/World.h>

#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_BEGIN_CLASS_META( FSpatialObject )
AN_END_CLASS_META()

FSpatialObject * FSpatialObject::DirtyList = nullptr;
FSpatialObject * FSpatialObject::DirtyListTail = nullptr;

FSpatialObject::FSpatialObject() {
    Bounds.Clear();
    WorldBounds.Clear();
    bWorldBoundsDirty = true;
    OverrideBoundingBox.Clear();
}

void FSpatialObject::ForceOverrideBounds( bool _OverrideBounds ) {
    if ( bOverrideBounds == _OverrideBounds ) {
        return;
    }

    bOverrideBounds = _OverrideBounds;
    MarkWorldBoundsDirty();
}

void FSpatialObject::SetBoundsOverride( BvAxisAlignedBox const & _Bounds ) {
    OverrideBoundingBox = _Bounds;
    if ( bOverrideBounds ) {
        MarkWorldBoundsDirty();
    }
}

BvAxisAlignedBox const & FSpatialObject::GetBounds() const {

    if ( bOverrideBounds ) {
        return OverrideBoundingBox;
    }

    if ( bLazyBoundsUpdate ) {
        // Some components like skinned mesh has lazy bounds update
        const_cast< FSpatialObject * >( this )->OnLazyBoundsUpdate();
    }

    return Bounds;
}

BvAxisAlignedBox const & FSpatialObject::GetWorldBounds() const {

    // Update bounding box
    GetBounds();

    if ( bWorldBoundsDirty ) {
        WorldBounds = GetBounds().Transform( GetWorldTransformMatrix() );
        bWorldBoundsDirty = false;
    }

    return WorldBounds;
}

void FSpatialObject::OnTransformDirty() {
    Super::OnTransformDirty();

    MarkWorldBoundsDirty();
}

void FSpatialObject::InitializeComponent() {
    Super::InitializeComponent();

    MarkAreaDirty();
}

void FSpatialObject::DeinitializeComponent() {
    Super::DeinitializeComponent();

    // remove from dirty list
    IntrusiveRemoveFromList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );

    // FIXME: Is it right way to remove surface areas here?
    FWorld * world = GetWorld();
    for ( FLevel * level : world->GetArrayOfLevels() ) {
        level->RemoveSurfaceAreas( this );
    }
}

void FSpatialObject::MarkAreaDirty() {
    // add to dirty list
    if ( !IntrusiveIsInList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail ) ) {
        IntrusiveAddToList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );
    }
}

void FSpatialObject::MarkWorldBoundsDirty() {
    bWorldBoundsDirty = true;

    MarkAreaDirty();
}

void FSpatialObject::ForceOutdoor( bool _OutdoorSurface ) {
    if ( bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    bIsOutdoor = _OutdoorSurface;
    MarkAreaDirty();
}
