#include <Engine/World/Public/RenderableComponent.h>
#include <Engine/World/Public/World.h>
//#include <Engine/Renderer/Public/SpatialTreeComponent.h>
//#include <Engine/Scene/Public/Scene.h>

AN_CLASS_META_NO_ATTRIBS( FDrawSurf )

//AN_SCENE_COMPONENT_BEGIN_DECL( FDrawSurf, CCF_HIDDEN_IN_EDITOR )
//AN_ATTRIBUTE( "Rendering Layers", FProperty( DEFAULT_RENDERING_LAYERS ), SetRenderingLayers, GetRenderingLayers, "Set rendering layer(s)", AF_BITFIELD )
//AN_SCENE_COMPONENT_END_DECL

FDrawSurf::FDrawSurf() {
    RenderingLayers = DEFAULT_RENDERING_LAYERS;
    Bounds.Clear();
    WorldBounds.Clear();
    WorldBoundsDirty = true;
    SurfaceType = SURF_UNKNOWN;
    SurfacePlane.Clear();
}

void FDrawSurf::InitializeComponent() {
    Super::InitializeComponent();

    GetWorld()->UpdateDrawSurfAreas( this );
}

void FDrawSurf::MarkBoundsDirty() {
    //if ( !WorldBoundsDirty ) {
        WorldBoundsDirty = true;

        GetWorld()->UpdateDrawSurfAreas( this );
    //}
}

void FDrawSurf::SetBounds( BvAxisAlignedBox const & _Bounds ) {
    Bounds = _Bounds;
    MarkBoundsDirty();
}

const BvAxisAlignedBox & FDrawSurf::GetBounds() const {
    return Bounds;
}

const BvAxisAlignedBox & FDrawSurf::GetWorldBounds() const {
    if ( WorldBoundsDirty ) {

        const_cast< FDrawSurf * >( this )->OnUpdateWorldBounds();

        WorldBoundsDirty = false;
    }

    return WorldBounds;
}

void FDrawSurf::OnUpdateWorldBounds() {
    WorldBounds = Bounds;

    // Or?
    //WorldBounds = Bounds.Transform( GetNode()->GetWorldTransformMatrix() );
}

void FDrawSurf::OnTransformDirty() {
    Super::OnTransformDirty();

    MarkBoundsDirty();
}

void FDrawSurf::SetSurfaceType( ESurfaceType _Type ) {
    SurfaceType = _Type;
}

ESurfaceType FDrawSurf::GetSurfaceType() const {
    return SurfaceType;
}

void FDrawSurf::SetSurfacePlane( PlaneF const & _Plane ) {
    SurfacePlane = _Plane;
}

PlaneF const & FDrawSurf::GetSurfacePlane() const {
    return SurfacePlane;
}
