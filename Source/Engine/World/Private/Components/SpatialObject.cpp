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

#include <Engine/World/Public/Components/SpatialObject.h>
#include <Engine/World/Public/World.h>

#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_BEGIN_CLASS_META( ASpatialObject )
AN_END_CLASS_META()

ASpatialObject * ASpatialObject::DirtyList = nullptr;
ASpatialObject * ASpatialObject::DirtyListTail = nullptr;

ASpatialObject::ASpatialObject() {
    Bounds.Clear();
    WorldBounds.Clear();
    bWorldBoundsDirty = true;
    OverrideBoundingBox.Clear();
}

void ASpatialObject::ForceOverrideBounds( bool _OverrideBounds ) {
    if ( bOverrideBounds == _OverrideBounds ) {
        return;
    }

    bOverrideBounds = _OverrideBounds;
    MarkWorldBoundsDirty();
}

void ASpatialObject::SetBoundsOverride( BvAxisAlignedBox const & _Bounds ) {
    OverrideBoundingBox = _Bounds;
    if ( bOverrideBounds ) {
        MarkWorldBoundsDirty();
    }
}

BvAxisAlignedBox const & ASpatialObject::GetBounds() const {

    if ( bOverrideBounds ) {
        return OverrideBoundingBox;
    }

    if ( bLazyBoundsUpdate ) {
        // Some components like skinned mesh has lazy bounds update
        const_cast< ASpatialObject * >( this )->OnLazyBoundsUpdate();
    }

    return Bounds;
}

BvAxisAlignedBox const & ASpatialObject::GetWorldBounds() const {

    // Update bounding box
    GetBounds();

    if ( bWorldBoundsDirty ) {
        WorldBounds = GetBounds().Transform( GetWorldTransformMatrix() );
        bWorldBoundsDirty = false;
    }

    return WorldBounds;
}

void ASpatialObject::OnTransformDirty() {
    Super::OnTransformDirty();

    MarkWorldBoundsDirty();
}

void ASpatialObject::InitializeComponent() {
    Super::InitializeComponent();

    MarkAreaDirty();
}

void ASpatialObject::DeinitializeComponent() {
    Super::DeinitializeComponent();

    // remove from dirty list
    INTRUSIVE_REMOVE( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );

    // FIXME: Is it right way to remove surface areas here?
    AWorld * world = GetWorld();
    for ( ALevel * level : world->GetArrayOfLevels() ) {
        level->RemoveSurfaceAreas( this );
    }
}

void ASpatialObject::MarkAreaDirty() {
    // add to dirty list
    if ( !INTRUSIVE_EXISTS( this, NextDirty, PrevDirty, DirtyList, DirtyListTail ) ) {
        INTRUSIVE_ADD( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );
    }
}

void ASpatialObject::MarkWorldBoundsDirty() {
    bWorldBoundsDirty = true;

    if ( IsInitialized() ) {
        MarkAreaDirty();
    }
}

void ASpatialObject::ForceOutdoor( bool _OutdoorSurface ) {
    if ( bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    bIsOutdoor = _OutdoorSurface;

    if ( IsInitialized() ) {
        MarkAreaDirty();
    }
}

void ASpatialObject::_UpdateSurfaceAreas() {
    ASpatialObject * next;
    for ( ASpatialObject * surf = DirtyList ; surf ; surf = next ) {

        next = surf->NextDirty;

        AWorld * world = surf->GetWorld();

        for ( ALevel * level : world->GetArrayOfLevels() ) {
            level->RemoveSurfaceAreas( surf );
        }

        for ( ALevel * level : world->GetArrayOfLevels() ) {
            level->AddSurfaceAreas( surf );
            //GLogger.Printf( "Update actor %s\n", surf->GetParentActor()->GetNameConstChar() );
        }

        surf->PrevDirty = surf->NextDirty = nullptr;
    }
    DirtyList = DirtyListTail = nullptr;
}
