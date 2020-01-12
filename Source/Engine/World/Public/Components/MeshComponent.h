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

#pragma once

#include <World/Public/Resource/Texture.h>
#include <World/Public/Resource/Material.h>
#include <World/Public/Resource/IndexedMesh.h>
#include <World/Public/Base/ResourceManager.h>

#include "Drawable.h"

class ACameraComponent;

/**

AMeshComponent

Mesh component without skinning

*/
class ANGIE_API AMeshComponent : public ADrawable {
    AN_COMPONENT( AMeshComponent, ADrawable )

    friend class ARenderWorld;

public:
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

    /** Render mesh to custom depth-stencil buffer. Render target must have custom depth-stencil buffer enabled */
    bool            bCustomDepthStencilPass;

    /** Custom depth stencil value for the mesh */
    uint8_t         CustomDepthStencilValue;

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

    BvAxisAlignedBox GetSubpartWorldBounds( int _SubpartIndex ) const;

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

protected:
    AMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    ACollisionBodyComposition const & DefaultBodyComposition() const override;

    void DrawDebug( ADebugRenderer * InRenderer ) override;

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



class ABrushComponent : public ADrawable {
    AN_COMPONENT( ABrushComponent, ADrawable )

public:
    /** Brush surfaces */
    int FirstSurface;

    /** Count of the brush surfaces */
    int NumSurfaces;

    void SetModel( ABrushModel * InBrushModel )
    {
        Model = InBrushModel;
    }

    ABrushModel * GetModel() { return Model; }

protected:
    ABrushComponent();

    //ACollisionBodyComposition const & DefaultBodyComposition() const override { return Model ? Model->BodyComposition : BodyComposition; }

    void DrawDebug( ADebugRenderer * InRenderer ) override;

private:
    TRef< ABrushModel > Model;
};
