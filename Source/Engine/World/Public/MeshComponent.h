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

#include "DrawSurf.h"
#include "Texture.h"
#include "Material.h"
#include "IndexedMesh.h"

class FCameraComponent;

enum EVSDPass {
    VSD_PASS_IGNORE     = 0,
    VSD_PASS_ALL        = ~0,
    VSD_PASS_PORTALS    = 1,
    VSD_PASS_FACE_CULL  = 2,
    VSD_PASS_BOUNDS     = 4,
    VSD_PASS_CUSTOM_VISIBLE_STEP = 8,
    VSD_PASS_VIS_MARKER = 16,
    VSD_PASS_DEFAULT    = VSD_PASS_PORTALS | VSD_PASS_BOUNDS,
};

/*

FMeshComponent

Mesh component without skinning

*/
class ANGIE_API FMeshComponent : public FDrawSurf {
    AN_COMPONENT( FMeshComponent, FDrawSurf )

    friend class FWorld;

public:
    // Visible surface determination alogrithm
    int             VSDPasses = VSD_PASS_DEFAULT;

    // Marker for VSD_PASS_VIS_MARKER
    int             VisMarker;

    // Lightmap atlas index
    int             LightmapBlock;

    // Lighmap channel UV offset and scale
    Float4          LightmapOffset;

    // Lightmap UV channel
    TRefHolder< FLightmapUV >   LightmapUVChannel;

    // Baked vertex light channel
    TRefHolder< FVertexLight >  VertexLightChannel;

    // Force using dynamic range
    bool            bUseDynamicRange;

    // Dynamic range property
    unsigned int    DynamicRangeIndexCount;

    // Dynamic range property
    unsigned int    DynamicRangeStartIndexLocation;

    // Dynamic range property
    int             DynamicRangeBaseVertexLocation;

    // Flipbook animation page offset
    unsigned int    SubpartBaseVertexOffset;

    // Render during light pass
    bool            bLightPass;

    // Cast shadow
    bool            bShadowCast;

    // Receive shadow
    //bool            bShadowReceive;

    // Use specific shadow pass from material
    //bool            bMaterialShadowPass;

    // Render mesh to custom depth-stencil buffer. Render target must have custom depth-stencil buffer enabled
    bool            bCustomDepthStencilPass;

    // Custom depth stencil value for the mesh
    byte            CustomDepthStencilValue;

    // Force ignoring component position/rotation/scale. FIXME: move this to super class SceneComponent?
    bool            bNoTransform;

    // Internal. Used by frontend to filter rendered meshes.
    int             RenderMark;

    // Used for VSD_FACE_CULL
    PlaneF          FacePlane;

    // Set indexed mesh for the component
    void SetMesh( FIndexedMesh * _Mesh );

    // Set indexed mesh for the component
    void SetMesh( const char * _Mesh );

    // Set indexed mesh for the component
    void SetMesh( FString const & _Mesh ) { SetMesh( _Mesh.ToConstChar() ); }

    // Get indexed mesh
    FIndexedMesh * GetMesh() const { return Mesh; }

    // Unset materials
    void ClearMaterials();

    // Set materials from mesh resource
    void SetDefaultMaterials();

    // Set material instance for subpart of the mesh
    void SetMaterialInstance( int _SubpartIndex, FMaterialInstance * _Instance );

    // Get material instance of subpart of the mesh
    FMaterialInstance * GetMaterialInstance( int _SubpartIndex ) const;

    // Set material instance for subpart of the mesh
    void SetMaterialInstance( FMaterialInstance * _Instance ) { SetMaterialInstance( 0, _Instance ); }

    // Get material instance of subpart of the mesh
    FMaterialInstance * GetMaterialInstance() const { return GetMaterialInstance( 0 ); }

    // Iterate meshes in parent world
    FMeshComponent * GetNextMesh() { return Next; }
    FMeshComponent * GetPrevMesh() { return Prev; }

    // Used for VSD_PASS_CUSTOM_VISIBLE_STEP algorithm
    virtual void OnCustomVisibleStep( FCameraComponent * _Camera, bool & _OutVisibleFlag ) {}

protected:
    FMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    FCollisionBodyComposition const & DefaultBodyComposition() const override;

    virtual void OnMeshChanged() {}

private:
    void NotifyMeshChanged();

    FMeshComponent * Next;
    FMeshComponent * Prev;

    TRefHolder< FIndexedMesh > Mesh;
    TPodArray< FMaterialInstance *, 1 > Materials;
};
