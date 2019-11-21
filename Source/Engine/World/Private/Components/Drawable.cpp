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

#include <World/Public/Components/Drawable.h>
#include <World/Public/World.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META( ADrawable )

ADrawable::ADrawable() {
    RenderingGroup = RENDERING_GROUP_DEFAULT;
    Bounds.Clear();
    WorldBounds.Clear();
    bWorldBoundsDirty = true;
    OverrideBoundingBox.Clear();
}

void ADrawable::ForceOverrideBounds( bool _OverrideBounds ) {
    if ( bOverrideBounds == _OverrideBounds ) {
        return;
    }

    bOverrideBounds = _OverrideBounds;
    MarkWorldBoundsDirty();
}

void ADrawable::SetBoundsOverride( BvAxisAlignedBox const & _Bounds ) {
    OverrideBoundingBox = _Bounds;
    if ( bOverrideBounds ) {
        MarkWorldBoundsDirty();
    }
}

BvAxisAlignedBox const & ADrawable::GetBounds() const {

    if ( bOverrideBounds ) {
        return OverrideBoundingBox;
    }

    if ( bLazyBoundsUpdate ) {
        // Some components like skinned mesh has lazy bounds update
        const_cast< ADrawable * >( this )->OnLazyBoundsUpdate();
    }

    return Bounds;
}

BvAxisAlignedBox const & ADrawable::GetWorldBounds() const {

    // Update and get bounding box
    BvAxisAlignedBox const & boundingBox = GetBounds();

    if ( bWorldBoundsDirty ) {
        WorldBounds = boundingBox.Transform( GetWorldTransformMatrix() );
        bWorldBoundsDirty = false;
    }

    return WorldBounds;
}

void ADrawable::OnTransformDirty() {
    Super::OnTransformDirty();

    MarkWorldBoundsDirty();
}

void ADrawable::InitializeComponent() {
    AWorld * pWorld = GetWorld();
    ARenderWorld & RenderWorld = pWorld->GetRenderWorld();

    Super::InitializeComponent();

    pWorld->UpdateDrawableArea( this );

    RenderWorld.AddDrawable( this );
}

void ADrawable::DeinitializeComponent() {
    AWorld * pWorld = GetWorld();
    ARenderWorld & RenderWorld = pWorld->GetRenderWorld();

    Super::DeinitializeComponent();

    pWorld->UpdateDrawableArea( this );

    RenderWorld.RemoveDrawable( this );
}

void ADrawable::MarkWorldBoundsDirty() {
    bWorldBoundsDirty = true;

    if ( IsInitialized() )
    {
        GetWorld()->UpdateDrawableArea( this );
    }
}

void ADrawable::ForceOutdoor( bool _OutdoorSurface ) {
    if ( bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    bIsOutdoor = _OutdoorSurface;

    if ( IsInitialized() )
    {
        GetWorld()->UpdateDrawableArea( this );
    }
}
