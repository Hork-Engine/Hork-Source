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

#include <Engine/Resource/Public/Texture.h>
#include <Engine/Resource/Public/Material.h>
#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/Resource/Public/ResourceManager.h>

#include "DrawSurf.h"

class ACameraComponent;

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

/**

AMeshComponent

Mesh component without skinning

*/
class ANGIE_API AMeshComponent : public ADrawSurf {
    AN_COMPONENT( AMeshComponent, ADrawSurf )

    friend class AWorld;

public:
    /** Visible surface determination alogrithm */
    int             VSDPasses = VSD_PASS_DEFAULT;

    /** Marker for VSD_PASS_VIS_MARKER */
    int             VisMarker;

    /** Lightmap atlas index */
    int             LightmapBlock;

    /** Lighmap channel UV offset and scale */
    Float4          LightmapOffset;

    /** Lightmap UV channel */
    TRef< ALightmapUV >   LightmapUVChannel;

    /** Baked vertex light channel */
    TRef< AVertexLight >  VertexLightChannel;

    /** Force using dynamic range */
    bool            bUseDynamicRange;

    /** Dynamic range property */
    unsigned int    DynamicRangeIndexCount;

    /** Dynamic range property */
    unsigned int    DynamicRangeStartIndexLocation;

    /** Dynamic range property */
    int             DynamicRangeBaseVertexLocation;

    /** Flipbook animation page offset */
    unsigned int    SubpartBaseVertexOffset;

    /** Render during main pass */
    bool            bLightPass;

    /** Render mesh to custom depth-stencil buffer. Render target must have custom depth-stencil buffer enabled */
    bool            bCustomDepthStencilPass;

    /** Custom depth stencil value for the mesh */
    byte            CustomDepthStencilValue;

    /** Force ignoring component position/rotation/scale. FIXME: move this to super class SceneComponent? */
    bool            bNoTransform;

    /** Internal. Used by frontend to filter rendered meshes. */
    int             RenderMark;

    /** Used for VSD_FACE_CULL */
    PlaneF          FacePlane;

    bool            bOverrideMeshMaterials = true;

    /** Set indexed mesh for the component */
    void SetMesh( AIndexedMesh * _Mesh );

    /** Helper. Set indexed mesh by alias */
    template< char... Chars >
    void SetMesh( TCompileTimeString<Chars...> const & _Alias ) {
        static TStaticResourceFinder< AIndexedMesh > Resource( _Alias );
        SetMesh( Resource.GetObject() );
    }

    /** Get indexed mesh. Never return null */
    AIndexedMesh * GetMesh() const { return Mesh; }

    /** Unset materials */
    void ClearMaterials();

    /** Set materials from mesh resource */
    void CopyMaterialsFromMeshResource();

    /** Set material instance for subpart of the mesh */
    void SetMaterialInstance( int _SubpartIndex, AMaterialInstance * _Instance );

    /** Helper. Set material instance by alias */
    template< char... Chars >
    void SetMaterialInstance( int _SubpartIndex, TCompileTimeString<Chars...> const & _Alias ) {
        static TStaticResourceFinder< AMaterialInstance > Resource( _Alias );
        SetMaterialInstance( _SubpartIndex, Resource.GetObject() );
    }

    /** Get material instance of subpart of the mesh. Never return null. */
    AMaterialInstance * GetMaterialInstance( int _SubpartIndex ) const;

    /** Set material instance for subpart of the mesh */
    void SetMaterialInstance( AMaterialInstance * _Instance ) { SetMaterialInstance( 0, _Instance ); }

    /** Helper. Set material instance by alias */
    template< char... Chars >
    void SetMaterialInstance( TCompileTimeString<Chars...> const & _Alias ) {
        static TStaticResourceFinder< AMaterialInstance > Resource( _Alias );
        SetMaterialInstance( 0, Resource.GetObject() );
    }

    /** Get material instance of subpart of the mesh. Never return null. */
    AMaterialInstance * GetMaterialInstance() const { return GetMaterialInstance( 0 ); }

    /** Allow mesh to cast shadows on the world */
    void SetCastShadow( bool _CastShadow );

    /** Is cast shadows enabled */
    bool IsCastShadow() const { return bCastShadow; }

    /** Iterate meshes in parent world */
    AMeshComponent * GetNextMesh() { return Next; }
    AMeshComponent * GetPrevMesh() { return Prev; }

    /** Iterate shadow casters in parent world */
    AMeshComponent * GetNextShadowCaster() { return NextShadowCaster; }
    AMeshComponent * GetPrevShadowCaster() { return PrevShadowCaster; }

    /** Used for VSD_PASS_CUSTOM_VISIBLE_STEP algorithm */
    virtual void RenderFrontend_CustomVisibleStep( SRenderFrontendDef * _Def, bool & _OutVisibleFlag ) {}

protected:
    AMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    ACollisionBodyComposition const & DefaultBodyComposition() const override;

    void DrawDebug( ADebugDraw * _DebugDraw ) override;

    virtual void OnMeshChanged() {}

private:
    void NotifyMeshChanged();

    AMaterialInstance * GetMaterialInstanceUnsafe( int _SubpartIndex ) const;

    AMeshComponent * Next;
    AMeshComponent * Prev;

    AMeshComponent * NextShadowCaster;
    AMeshComponent * PrevShadowCaster;

    TRef< AIndexedMesh > Mesh;
    TPodArray< AMaterialInstance *, 1 > Materials;

    bool bCastShadow;
};
