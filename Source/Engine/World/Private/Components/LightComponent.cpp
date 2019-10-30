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

#include <Engine/World/Public/Components/LightComponent.h>
#include <Engine/Base/Public/DebugDraw.h>

#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_BEGIN_CLASS_META( FClusteredObject )
AN_END_CLASS_META()

FClusteredObject * FClusteredObject::DirtyList = nullptr;
FClusteredObject * FClusteredObject::DirtyListTail = nullptr;

FClusteredObject::FClusteredObject() {
    RenderingGroup = RENDERING_GROUP_DEFAULT;
}

BvSphereSSE const & FClusteredObject::GetSphereWorldBounds() const {
    return SphereWorldBounds;
}

BvAxisAlignedBox const & FClusteredObject::GetAABBWorldBounds() const {
    return AABBWorldBounds;
}

BvOrientedBox const & FClusteredObject::GetOBBWorldBounds() const {
    return OBBWorldBounds;
}

Float4x4 const & FClusteredObject::GetOBBTransformInverse() const {
    return OBBTransformInverse;
}

void FClusteredObject::InitializeComponent() {
    Super::InitializeComponent();

    MarkAreaDirty();
}

void FClusteredObject::DeinitializeComponent() {
    Super::DeinitializeComponent();
#if 0
    // remove from dirty list
    IntrusiveRemoveFromList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );

    // FIXME: Is it right way to remove surface areas here?
    FWorld * world = GetWorld();
    for ( FLevel * level : world->GetArrayOfLevels() ) {
        level->RemoveSurfaceAreas( this );
    }
#endif
}

void FClusteredObject::MarkAreaDirty() {
#if 0
    // add to dirty list
    if ( !IntrusiveIsInList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail ) ) {
        IntrusiveAddToList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );
    }
#endif
}

void FClusteredObject::ForceOutdoor( bool _OutdoorSurface ) {
    if ( bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    bIsOutdoor = _OutdoorSurface;
    MarkAreaDirty();
}

void FClusteredObject::_UpdateSurfaceAreas() {
#if 0
    FClusteredObject * next;
    for ( FClusteredObject * surf = DirtyList; surf; surf = next ) {

        next = surf->NextDirty;

        FWorld * world = surf->GetWorld();

        for ( FLevel * level : world->GetArrayOfLevels() ) {
            level->RemoveSurfaceAreas( surf );
        }

        for ( FLevel * level : world->GetArrayOfLevels() ) {
            level->AddSurfaceAreas( surf );
            //GLogger.Printf( "Update actor %s\n", surf->GetParentActor()->GetNameConstChar() );
        }

        surf->PrevDirty = surf->NextDirty = nullptr;
    }
#endif
    DirtyList = DirtyListTail = nullptr;
}
