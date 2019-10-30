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

#include <Engine/World/Public/Components/MeshComponent.h>
#include <Engine/World/Public/Actors/Actor.h>
#include <Engine/World/Public/World.h>
#include <Engine/Resource/Public/ResourceManager.h>

AN_CLASS_META( FMeshComponent )

//AN_SCENE_COMPONENT_BEGIN_DECL( FMeshComponent, CCF_HIDDEN_IN_EDITOR )
//AN_ATTRIBUTE_CONST( "Material Instance", FProperty( byte(0) ), "MaterialInstance\0\0Material instance resource", AF_RESOURCE )
//AN_ATTRIBUTE( "Shadow Cast", FProperty( true ), EnableShadowCast, IsShadowCastEnabled, "Shadow casting", AF_DEFAULT )
//AN_ATTRIBUTE( "Light Pass", FProperty( true ), EnableLightPass, IsLightPassEnabled, "Render on main pass", AF_DEFAULT )
//AN_ATTRIBUTE( "Custom Depth-Stencil Pass", FProperty( false ), EnableCustomDepthStencilPass, IsCustomDepthStencilPassEnabled, "Render mesh to custom depth-stencil buffer", AF_DEFAULT )
//AN_ATTRIBUTE( "Custom Depth-Stencil Value", FProperty( byte(0) ), SetCustomDepthStencilValue, GetCustomDepthStencilValue, "Render mesh to custom depth-stencil buffer", AF_DEFAULT )
////AN_ATTRIBUTE( "ShadowReceive", FProperty( true ), SetShadowReceive, GetShadowReceive, "Shadow receiving", AF_DEFAULT )
//AN_SCENE_COMPONENT_END_DECL

FMeshComponent::FMeshComponent() {
    bLightPass = true;
    bCastShadow = true;
    //bMaterialShadowPass = true;
    //SurfaceType = SURF_TRISOUP;
    LightmapOffset.Z = LightmapOffset.W = 1;
    //bShadowReceive = true;
}

void FMeshComponent::InitializeComponent() {
    Super::InitializeComponent();

    GetWorld()->AddMesh( this );
}

void FMeshComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    ClearMaterials();

    GetWorld()->RemoveMesh( this );
}

void FMeshComponent::SetMesh( FIndexedMesh * _Mesh ) {
    if ( Mesh == _Mesh ) {
        return;
    }

    Mesh = _Mesh;

    // Update bounding box
    if ( Mesh ) {
        Bounds = Mesh->GetBoundingBox();
    } else {
        Bounds.Clear();
    }

    NotifyMeshChanged();

    // Mark to update world bounds
    MarkWorldBoundsDirty();
}

void FMeshComponent::ClearMaterials() {
    for ( FMaterialInstance * material : Materials ) {
        if ( material ) {
            material->RemoveRef();
        }
    }
    Materials.Clear();
}

void FMeshComponent::CopyMaterialsFromMeshResource() {
    ClearMaterials();

    if ( Mesh ) {
        FIndexedMeshSubpartArray const & subparts = Mesh->GetSubparts();
        for ( int i = 0 ; i < subparts.Size() ; i++ ) {
            SetMaterialInstance( i, subparts[ i ]->GetMaterialInstance() );
        }
    }
}

void FMeshComponent::SetMaterialInstance( int _SubpartIndex, FMaterialInstance * _Instance ) {
    AN_Assert( _SubpartIndex >= 0 );

    if ( _SubpartIndex >= Materials.Size() ) {

        if ( _Instance ) {
            int n = Materials.Size();

            Materials.Resize( n + _SubpartIndex + 1 );
            for ( ; n < Materials.Size() ; n++ ) {
                Materials[n] = nullptr;
            }
            Materials[_SubpartIndex] = _Instance;
            _Instance->AddRef();
        }

        return;
    }

    if ( Materials[_SubpartIndex] ) {
        Materials[_SubpartIndex]->RemoveRef();
    }
    Materials[_SubpartIndex] = _Instance;
    if ( _Instance ) {
        _Instance->AddRef();
    }
}

FMaterialInstance * FMeshComponent::GetMaterialInstanceUnsafe( int _SubpartIndex ) const {
    if ( _SubpartIndex < 0 ) {
        return nullptr;
    }

    if ( bOverrideMeshMaterials ) {
        if ( _SubpartIndex >= Materials.Size() ) {
            return nullptr;
        }
        return Materials[_SubpartIndex];
    }

    if ( !Mesh ) {
        return nullptr;
    }

    FIndexedMeshSubpartArray const & subparts = Mesh->GetSubparts();
    if ( _SubpartIndex >= subparts.Size() ) {
        return nullptr;
    }

    return subparts[_SubpartIndex]->GetMaterialInstance();
}

FMaterialInstance * FMeshComponent::GetMaterialInstance( int _SubpartIndex ) const {
    FMaterialInstance * pInstance = GetMaterialInstanceUnsafe( _SubpartIndex );
    if ( !pInstance ) {
        static TStaticInternalResourceFinder< FMaterialInstance > DefaultInstance( _CTS( "FMaterialInstance.Default" ) );
        pInstance = DefaultInstance.GetObject();
    }
    return pInstance;
}

FCollisionBodyComposition const & FMeshComponent::DefaultBodyComposition() const {
    if ( Mesh ) {
        return Mesh->BodyComposition;
    }

    return Super::DefaultBodyComposition();
}

void FMeshComponent::NotifyMeshChanged() {
    OnMeshChanged();
}

void FMeshComponent::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    if ( Mesh ) {
        Mesh->DrawDebug( _DebugDraw );
    }
}

