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
    Primitive.QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS | VSD_QUERY_MASK_SHADOW_CAST;

    bAllowRaycast = false;
    bCastShadow = true;

    //Primitive.bMovable = true;

    VisFrame = -1;
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

void ADrawable::SetSurfaceFlags( uint8_t Flags ) {
    Primitive.Flags = Flags;
}

uint8_t ADrawable::GetSurfaceFlags() const {
    return Primitive.Flags;
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
    return bOverrideBounds ? OverrideBoundingBox : Bounds;
}

BvAxisAlignedBox const & ADrawable::GetWorldBounds() const {
    return WorldBounds;
}

void ADrawable::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void ADrawable::InitializeComponent() {
    Super::InitializeComponent();

    GetLevel()->AddPrimitive( &Primitive );

    UpdateWorldBounds();

    if ( bCastShadow )
    {
        GetWorld()->GetRenderWorld().AddShadowCaster( this );
    }
}

void ADrawable::DeinitializeComponent() {
    Super::DeinitializeComponent();

    GetLevel()->RemovePrimitive( &Primitive );

    if ( bCastShadow )
    {
        GetWorld()->GetRenderWorld().RemoveShadowCaster( this );
    }
}

void ADrawable::SetCastShadow( bool _CastShadow ) {
    if ( bCastShadow == _CastShadow )
    {
        return;
    }

    bCastShadow = _CastShadow;

    if ( bCastShadow )
    {
        Primitive.QueryGroup |= VSD_QUERY_MASK_SHADOW_CAST;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_NO_SHADOW_CAST;
    } else
    {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_SHADOW_CAST;
        Primitive.QueryGroup |= VSD_QUERY_MASK_NO_SHADOW_CAST;
    }

    if ( IsInitialized() )
    {
        ARenderWorld & RenderWorld = GetWorld()->GetRenderWorld();

        if ( bCastShadow )
        {
            RenderWorld.AddShadowCaster( this );
        } else
        {
            RenderWorld.RemoveShadowCaster( this );
        }
    }
}

void ADrawable::UpdateWorldBounds() {
    BvAxisAlignedBox const & boundingBox = GetBounds();

    WorldBounds = boundingBox.Transform( GetWorldTransformMatrix() );

    Primitive.Box = WorldBounds;

    if ( IsInitialized() )
    {
        GetLevel()->MarkPrimitive( &Primitive );
    }
}

void ADrawable::ForceOutdoor( bool _OutdoorSurface ) {
    if ( Primitive.bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    Primitive.bIsOutdoor = _OutdoorSurface;

    if ( IsInitialized() )
    {
        GetLevel()->MarkPrimitive( &Primitive );
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
        GetLevel()->MarkPrimitive( &Primitive );
    }
}

bool ADrawable::IsMovable() const {
    return Primitive.bMovable;
}

void ADrawable::PreRenderUpdate( SRenderFrontendDef const * _Def ) {
    if ( VisFrame != _Def->FrameNumber ) {
        VisFrame = _Def->FrameNumber;

        OnPreRenderUpdate( _Def );
    }
}

bool ADrawable::Raycast( Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits, int & ClosestHit ) {
    if ( !Primitive.RaycastCallback ) {
        return false;
    }

    Hits.Clear();

    return Primitive.RaycastCallback( &Primitive, InRayStart, InRayEnd, Hits, ClosestHit );
}

bool ADrawable::RaycastClosest( Float3 const & InRayStart, Float3 const & InRayEnd, STriangleHitResult & Hit ) {
    if ( !Primitive.RaycastClosestCallback ) {
        return false;
    }

    Hit.Location = InRayEnd;
    Hit.Distance = ( InRayStart - InRayEnd ).Length();

    SMeshVertex const * pVertices;
    TRef< AMaterialInstance > material;

    if ( !Primitive.RaycastClosestCallback( &Primitive, InRayStart, Hit.Location, Hit.UV, Hit.Distance, &pVertices, Hit.Indices, material ) ) {
        return false;
    }

    Hit.Material = material;

    Float3 const & v0 = pVertices[Hit.Indices[0]].Position;
    Float3 const & v1 = pVertices[Hit.Indices[1]].Position;
    Float3 const & v2 = pVertices[Hit.Indices[2]].Position;

    Float3x4 const & transform = GetWorldTransformMatrix();

    // calc triangle vertices
    Float3 tv0 = transform * v0;
    Float3 tv1 = transform * v1;
    Float3 tv2 = transform * v2;

    // calc normal
    Hit.Normal = Math::Cross( tv1-tv0, tv2-tv0 ).Normalized();

    return true;
}
