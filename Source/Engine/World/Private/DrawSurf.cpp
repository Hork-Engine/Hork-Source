#include <Engine/World/Public/DrawSurf.h>
#include <Engine/World/Public/SkeletalAnimation.h>
#include <Engine/World/Public/World.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META_NO_ATTRIBS( FDrawSurf )

//AN_SCENE_COMPONENT_BEGIN_DECL( FDrawSurf, CCF_HIDDEN_IN_EDITOR )
//AN_ATTRIBUTE( "Rendering Layers", FProperty( FDrawSurf::RENDERING_LAYERS_DEFAULT ), SetRenderingLayers, GetRenderingLayers, "Set rendering layer(s)", AF_BITFIELD )
//AN_SCENE_COMPONENT_END_DECL

FDrawSurf * FDrawSurf::DirtyList = nullptr;
FDrawSurf * FDrawSurf::DirtyListTail = nullptr;

FDrawSurf::FDrawSurf() {
    RenderingLayers = RENDERING_LAYERS_DEFAULT;
    Bounds.Clear();
    WorldBounds.Clear();
    bWorldBoundsDirty = true;
    OverrideBoundingBox.Clear();
}

void FDrawSurf::InitializeComponent() {
    Super::InitializeComponent();

    MarkAreaDirty();
}

void FDrawSurf::EndPlay() {
    Super::EndPlay();

    // remove from dirty list
    IntrusiveRemoveFromList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );
}

void FDrawSurf::MarkAreaDirty() {
    // add to dirty list
    if ( !IntrusiveIsInList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail ) ) {
        IntrusiveAddToList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );
    }
}

void FDrawSurf::MarkWorldBoundsDirty() {
    bWorldBoundsDirty = true;

    MarkAreaDirty();
}

void FDrawSurf::ForceOutdoorSurface( bool _OutdoorSurface ) {
    if ( bIsOutdoorSurface == _OutdoorSurface ) {
        return;
    }

    bIsOutdoorSurface = _OutdoorSurface;
    MarkAreaDirty();
}

void FDrawSurf::ForceOverrideBounds( bool _OverrideBounds ) {
    if ( bOverrideBounds == _OverrideBounds ) {
        return;
    }

    bOverrideBounds = _OverrideBounds;
    MarkWorldBoundsDirty();
}

void FDrawSurf::SetBoundsOverride( BvAxisAlignedBox const & _Bounds ) {
    OverrideBoundingBox = _Bounds;
    if ( bOverrideBounds ) {
        MarkWorldBoundsDirty();
    }
}

BvAxisAlignedBox const & FDrawSurf::GetBounds() const {

    if ( bOverrideBounds ) {
        return OverrideBoundingBox;
    }

    if ( bSkinnedMesh ) {
        // Skinned mesh has lazy bounds update
        static_cast< FSkinnedComponent * >( const_cast< FDrawSurf * >( this ) )->UpdateBounds();
    }

    return Bounds;
}

BvAxisAlignedBox const & FDrawSurf::GetWorldBounds() const {
    if ( bWorldBoundsDirty ) {
        WorldBounds = GetBounds().Transform( GetWorldTransformMatrix() );
        bWorldBoundsDirty = false;
    }

    return WorldBounds;
}

void FDrawSurf::OnTransformDirty() {
    Super::OnTransformDirty();

    MarkWorldBoundsDirty();
}
