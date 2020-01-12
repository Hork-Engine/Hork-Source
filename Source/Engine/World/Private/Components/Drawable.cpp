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

#include <World/Public/Components/Drawable.h>
#include <World/Public/World.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META( ADrawable )

ADrawable::ADrawable() {
    Bounds.Clear();
    WorldBounds.Clear();
    OverrideBoundingBox.Clear();

    Primitive.Owner = this;
    Primitive.Type = VSD_PRIMITIVE_BOX;
    Primitive.VisGroup = VISIBILITY_GROUP_DEFAULT;
    Primitive.QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
    //Primitive.bMovable = true;
}

void ADrawable::SetVisibilityGroup( int InVisibilityGroup ) {
    Primitive.VisGroup = InVisibilityGroup;
}

int ADrawable::GetVisibilityGroup() const {
    return Primitive.VisGroup;
}

void ADrawable::SetVisible( bool _Visible ) {
    if ( _Visible ) {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    } else {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

bool ADrawable::IsVisible() const {
    return !!( Primitive.QueryGroup & VSD_QUERY_MASK_VISIBLE );
}

void ADrawable::SetHiddenInLightPass( bool _HiddenInLightPass ) {
    if ( _HiddenInLightPass ) {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    } else {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
}

bool ADrawable::IsHiddenInLightPass() const {
    return !(Primitive.QueryGroup & VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS);
}

void ADrawable::SetQueryGroup( int _UserQueryGroup ) {
    Primitive.QueryGroup |= _UserQueryGroup & 0xffff0000;
}

void ADrawable::SetFaceCull( bool bFaceCull ) {
    Primitive.bFaceCull = bFaceCull;
}

bool ADrawable::GetFaceCull() const {
    return Primitive.bFaceCull;
}

void ADrawable::SetFacePlane( PlaneF const & _Plane ) {
    Primitive.Face = _Plane;
}

PlaneF const & ADrawable::GetFacePlane() const {
    return Primitive.Face;
}

void ADrawable::ForceOverrideBounds( bool _OverrideBounds ) {
    if ( bOverrideBounds == _OverrideBounds ) {
        return;
    }

    bOverrideBounds = _OverrideBounds;

    UpdateWorldBounds();
}

void ADrawable::SetBoundsOverride( BvAxisAlignedBox const & _Bounds ) {
    OverrideBoundingBox = _Bounds;

    if ( bOverrideBounds ) {
        UpdateWorldBounds();
    }
}

BvAxisAlignedBox const & ADrawable::GetBounds() const {

    if ( bOverrideBounds ) {
        return OverrideBoundingBox;
    }

//    // TODO: Remove this!!!
//    if ( bLazyBoundsUpdate ) {
//        // Some components like skinned mesh has lazy bounds update
//        const_cast< ADrawable * >( this )->OnLazyBoundsUpdate();
//    }

    return Bounds;
}

BvAxisAlignedBox const & ADrawable::GetWorldBounds() const {
    return WorldBounds;
}

void ADrawable::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void ADrawable::InitializeComponent() {
    AWorld * pWorld = GetWorld();
    ARenderWorld & RenderWorld = pWorld->GetRenderWorld();

    Super::InitializeComponent();

    pWorld->AddPrimitive( &Primitive );

    RenderWorld.AddDrawable( this );

    UpdateWorldBounds();
}

void ADrawable::DeinitializeComponent() {
    AWorld * pWorld = GetWorld();
    ARenderWorld & RenderWorld = pWorld->GetRenderWorld();

    Super::DeinitializeComponent();

    pWorld->RemovePrimitive( &Primitive );

    RenderWorld.RemoveDrawable( this );
}

void ADrawable::UpdateWorldBounds() {
    BvAxisAlignedBox const & boundingBox = GetBounds();

    WorldBounds = boundingBox.Transform( GetWorldTransformMatrix() );

    Primitive.Box = WorldBounds;

    if ( IsInitialized() )
    {
        GetWorld()->MarkPrimitive( &Primitive );
    }
}

void ADrawable::ForceOutdoor( bool _OutdoorSurface ) {
    if ( Primitive.bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    Primitive.bIsOutdoor = _OutdoorSurface;

    if ( IsInitialized() )
    {
        GetWorld()->MarkPrimitive( &Primitive );
    }
}

bool ADrawable::IsOutdoor() const {
    return Primitive.bIsOutdoor;
}

void ADrawable::SetMovable( bool _Movable ) {
    if ( Primitive.bMovable == _Movable ) {
        return;
    }

    Primitive.bMovable = _Movable;

    if ( IsInitialized() )
    {
        GetWorld()->MarkPrimitive( &Primitive );
    }
}

bool ADrawable::IsMovable() const {
    return Primitive.bMovable;
}
