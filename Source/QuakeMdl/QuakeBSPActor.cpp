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

#include "QuakeBSPActor.h"
#include "Game.h"

#include <Engine/Core/Public/Logger.h>
#include <Engine/World/Public/CameraComponent.h>
#include <Engine/World/Public/ResourceManager.h>
#include <Engine/World/Public/GameMaster.h>

AN_CLASS_META_NO_ATTRIBS( FQuakeBSPActor )

FQuakeBSPActor::FQuakeBSPActor() {
    Mesh = NewObject< FIndexedMesh >();
}

void FQuakeBSPActor::SetModel( FQuakeBSP * _Model ) {
    Model = _Model;

    BSP = &Model->BSP;

    for ( FStaticMeshComponent * surf : SurfacePool ) {
        surf->Destroy();
    }

    SurfacePool.ResizeInvalidate( Model->LightmapGroups.Length() );
    Vertices.ResizeInvalidate( BSP->Vertices.Length() );
    LightmapVerts.ResizeInvalidate( BSP->Vertices.Length() );
    Indices.ResizeInvalidate( BSP->Indices.Length() );

    Mesh->Initialize( BSP->Vertices.Length(), BSP->Indices.Length() );

    LightmapUV = Mesh->CreateLightmapUVChannel();

    for ( int i = 0 ; i < SurfacePool.Length() ; i++ ) {
        QLightmapGroup * lightmapGroup = &Model->LightmapGroups[i];

        FStaticMeshComponent * surf = CreateComponent< FStaticMeshComponent >( FString::Fmt( "bsp_surf%d", i ) );
        surf->SetMesh( Mesh );
        surf->RegisterComponent();
        surf->VSDPasses = VSD_PASS_VIS_MARKER;
        surf->LightmapUVChannel = LightmapUV;
        surf->bUseDynamicRange = true;
        SurfacePool[i] = surf;

        FMaterialInstance * materialInstance = NewObject< FMaterialInstance >();

        FString const & name = Model->Textures[ lightmapGroup->TextureIndex ].Object->GetName();

        if ( !name.CmpN( "sky", 3 ) ) {
            materialInstance->Material = GGameModule->SkyMaterial;
        } else if ( name[ 0 ] == '*' ) {
            materialInstance->Material = GGameModule->WaterMaterial;
        } else {
            materialInstance->Material = GGameModule->WallMaterial;

            surf->LightmapBlock = lightmapGroup->LightmapBlock;
        }

        surf->SetMaterialInstance( materialInstance );
    }

//    CubemapTex = NewObject<FTexture>();
//    const Float3 dirs[6] = {
//        Float3(1,0,0),
//        Float3(-1,0,0),
//        Float3(0,1,0),
//        Float3(0,-1,0),
//        Float3(0,0,1),
//        Float3(0,0,-1)
//    };

//    CubemapTex->InitializeCubemap( TEXTURE_PF_BGR8, 1, 1 );

//    for ( int i = 0 ; i < 6 ; i++ ) {
//        byte * data = (byte*)CubemapTex->WriteTextureData( 0,0,i,1,1,0 );
//        data[0] = ( dirs[i].Z + 1.0f ) * 127.5f;
//        data[1] = ( dirs[i].Y + 1.0f ) * 127.5f;
//        data[2] = ( dirs[i].X + 1.0f ) * 127.5f;
//    }
}

void FQuakeBSPActor::OnView( FCameraComponent * _Camera ) {
    if ( !Model ) {
        return;
    }

    BSP->PerformVSD( _Camera->GetWorldPosition(), _Camera->GetFrustum(), true );

    AddSurfaces();
}

void FQuakeBSPActor::AddSurfaces() {
    int batchIndex = 0;
    int prevIndex = -1;
    int numVerts = 0;
    int numIndices = 0;
    int batchFirstIndex = 0;

    FMeshVertex const * pSrcVerts = BSP->Vertices.ToPtr();
    FMeshLightmapUV const * pSrcLM = BSP->LightmapVerts.ToPtr();
    unsigned int const * pSrcIndices = BSP->Indices.ToPtr();

    FMeshVertex * pDstVerts = Vertices.ToPtr();
    FMeshLightmapUV * pDstLM = LightmapVerts.ToPtr();
    unsigned int * pDstIndices = Indices.ToPtr();

    for ( int i = 0 ; i < BSP->NumVisSurfs ; i++ ) {
        FSurfaceDef * surfDef = BSP->VisSurfs[i];

        //Model->UpdateSurfaceLight( GetLevel(), surfDef );

        if ( prevIndex != surfDef->LightmapGroup ) {

            if ( batchIndex > 0 ) {
                AddSurface( numIndices - batchFirstIndex, batchFirstIndex, prevIndex );
            }

            prevIndex = surfDef->LightmapGroup;

            // Begin new batch
            batchIndex++;
            batchFirstIndex = numIndices;
        }

        memcpy( pDstVerts + numVerts, pSrcVerts + surfDef->FirstVertex, sizeof( FMeshVertex ) * surfDef->NumVertices );
        memcpy( pDstLM + numVerts, pSrcLM + surfDef->FirstVertex, sizeof( FMeshLightmapUV ) * surfDef->NumVertices );

        for ( int ind = 0 ; ind < surfDef->NumIndices ; ind++ ) {
            *pDstIndices++ = numVerts + pSrcIndices[ surfDef->FirstIndex + ind ];
        }

        numVerts += surfDef->NumVertices;
        numIndices += surfDef->NumIndices;
    }

    if ( batchIndex > 0 ) {
        AddSurface( numIndices - batchFirstIndex, batchFirstIndex, prevIndex );
    }

    AN_Assert( numVerts <= Vertices.Length() );
    AN_Assert( numIndices <= Indices.Length() );

    if ( numVerts > 0 ) {
        Mesh->WriteVertexData( Vertices.ToPtr(), numVerts, 0 );
        Mesh->WriteIndexData( Indices.ToPtr(), numIndices, 0 );
        LightmapUV->WriteVertexData( LightmapVerts.ToPtr(), numVerts, 0 );
    }
}

void FQuakeBSPActor::AddSurface( int _NumIndices, int _FirstIndex, int _GroupIndex ) {
    FStaticMeshComponent * surf = SurfacePool[_GroupIndex];
    FMaterialInstance * matInst = surf->GetMaterialInstance();

    QTexture * texture = &Model->Textures[ Model->LightmapGroups[ _GroupIndex ].TextureIndex ];

    if ( texture->AltNext ) {
        texture = texture->AltNext;
    }

    if ( texture->NumFrames == 0 ) {
        matInst->SetTexture( 0, texture->Object );
    } else {
        int frameNum = (GGameMaster.GetGameplayTimeMicro()>>17) % texture->NumFrames;
        QTexture * frame = texture;

        while ( frame->FrameTimeMin > frameNum || frame->FrameTimeMax <= frameNum ) {

            frame = frame->Next;
        }
        matInst->SetTexture( 0, frame->Object );
    }
//if ( !texture->Object->GetName().CmpN( "sky", 3 ) ) {
//    matInst->SetTexture( 0, CubemapTex );
//}

    surf->DynamicRangeIndexCount = _NumIndices;
    surf->DynamicRangeStartIndexLocation = _FirstIndex;

    surf->VisMarker = GRenderFrontend.GetVisMarker();
}
