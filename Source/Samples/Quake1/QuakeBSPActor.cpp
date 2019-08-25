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
#include <Engine/GameThread/Public/GameEngine.h>
#include <Engine/GameThread/Public/RenderFrontend.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/Resource/Public/Asset.h>
#include <Engine/World/Public/Components/CameraComponent.h>

#undef free

AN_CLASS_META( FQuakeBSPView )

FQuakeBSPView::FQuakeBSPView() {
    Mesh = NewObject< FIndexedMesh >();

    for ( int i = 0 ; i < 4 ; i++ ) {
        AmbientControl[ i ] = NewObject< FAudioControlCallback >();
        AmbientControl[ i ]->VolumeScale = 0;
    }

    bCanEverTick = true;
}

void FQuakeBSPView::BeginPlay() {
    Super::BeginPlay();

    const char * fileName[4] = {
        "sound/ambience/water1.wav",
        "sound/ambience/wind2.wav",
        "sound/ambience/swamp1.wav",
        "sound/ambience/swamp2.wav"
    };

    FSoundSpawnParameters ambient;

    ambient.Location = AUDIO_STAY_BACKGROUND;
    ambient.Priority = AUDIO_CHANNEL_PRIORITY_AMBIENT;
    ambient.bVirtualizeWhenSilent = true;
    ambient.Volume = 0.5f;// 0.02f;
    ambient.Pitch = 1;
    ambient.bLooping = true;
    ambient.bStopWhenInstigatorDead = true;

    for ( int i = 0 ; i < 4 ; i++ ) {
        ambient.ControlCallback = AmbientControl[i];

        FAudioClip * clip = GGameModule->LoadQuakeResource< FQuakeAudio >( fileName[i] );
        GAudioSystem.PlaySound( clip, this, &ambient );
    }
}

void FQuakeBSPView::SetModel( FQuakeBSP * _Model ) {
    Model = _Model;

    BSP = &Model->BSP;

    for ( FMeshComponent * surf : SurfacePool ) {
        surf->Destroy();
    }

    SurfacePool.ResizeInvalidate( Model->LightmapGroups.Size() );
    Vertices.ResizeInvalidate( BSP->Vertices.Size() );
    LightmapVerts.ResizeInvalidate( BSP->Vertices.Size() );
    Indices.ResizeInvalidate( BSP->Indices.Size() );

    Mesh->Initialize( BSP->Vertices.Size(), BSP->Indices.Size(), 1, false, true );

    LightmapUV = Mesh->CreateLightmapUVChannel();

    for ( int i = 0 ; i < SurfacePool.Size() ; i++ ) {
        QLightmapGroup * lightmapGroup = &Model->LightmapGroups[i];

        FMeshComponent * surf = AddComponent< FMeshComponent >( FString::Fmt( "bsp_surf%d", i ) );
        surf->SetMesh( Mesh );
        surf->VSDPasses = VSD_PASS_VIS_MARKER;
        surf->LightmapUVChannel = LightmapUV;
        surf->bUseDynamicRange = true;
        surf->bNoTransform = true;
        surf->RegisterComponent();
        SurfacePool[i] = surf;



        FMaterialInstance * materialInstance = NewObject< FMaterialInstance >();

        FString const & name = Model->Textures[ lightmapGroup->TextureIndex ].Object->GetName();

        if ( !name.CmpN( "sky", 3 ) ) {
            materialInstance->Material = GetResource< FMaterial >( "SkyMaterial" );
        } else if ( name[ 0 ] == '*' ) {
            materialInstance->Material = GetResource< FMaterial >( "WaterMaterial" );
        } else {
            materialInstance->Material = GetResource< FMaterial >( "WallMaterial" );

            surf->LightmapBlock = lightmapGroup->LightmapBlock;
        }

        surf->SetMaterialInstance( materialInstance );
    }
}

void FQuakeBSPView::Tick( float _TimeStep ) {
    Super::Tick( _TimeStep );

    if ( !Model ) {
        return;
    }

    int leaf = BSP->FindLeaf( GAudioSystem.GetListenerPosition() );

    if ( leaf < 0 ) {
        for ( int i = 0 ; i < 4 ; i++ ) {
            AmbientControl[ i ]->VolumeScale = 0.0f;
        }
        return;
    }

    const float step = _TimeStep;
    for ( int i = 0 ; i < 4 ; i++ ) {
        byte type = BSP->Leafs[leaf].AmbientType[i];
        float volume = (float)BSP->Leafs[leaf].AmbientVolume[i]/255.0f;

        FAudioControlCallback * control = AmbientControl[ type ];

        if ( control->VolumeScale < volume ) {
            control->VolumeScale += step;

            if ( control->VolumeScale > volume ) {
                control->VolumeScale = volume;
            }
        } else if ( control->VolumeScale > volume ) {
            control->VolumeScale -= step;

            if ( control->VolumeScale < 0 ) {
                control->VolumeScale = 0;
            }
        }
    }
}

void FQuakeBSPView::OnView( FCameraComponent * _Camera ) {
    if ( !Model ) {
        return;
    }

    BSP->PerformVSD( _Camera->GetWorldPosition(), _Camera->GetFrustum(), true );

    AddSurfaces();
}

void FQuakeBSPView::AddSurfaces() {
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

    AN_Assert( numVerts <= Vertices.Size() );
    AN_Assert( numIndices <= Indices.Size() );

    if ( numVerts > 0 ) {
        Mesh->WriteVertexData( Vertices.ToPtr(), numVerts, 0 );
        Mesh->WriteIndexData( Indices.ToPtr(), numIndices, 0 );
        LightmapUV->WriteVertexData( LightmapVerts.ToPtr(), numVerts, 0 );
    }
}

void FQuakeBSPView::AddSurface( int _NumIndices, int _FirstIndex, int _GroupIndex ) {
    FMeshComponent * surf = SurfacePool[_GroupIndex];
    FMaterialInstance * matInst = surf->GetMaterialInstance();

    QTexture * texture = &Model->Textures[ Model->LightmapGroups[ _GroupIndex ].TextureIndex ];

    if ( texture->AltNext ) {
        texture = texture->AltNext;
    }

    if ( texture->NumFrames == 0 ) {
        matInst->SetTexture( 0, texture->Object );
    } else {
        int frameNum = (GetWorld()->GetGameplayTimeMicro()>>17) % texture->NumFrames;
        QTexture * frame = texture;

        while ( frame->FrameTimeMin > frameNum || frame->FrameTimeMax <= frameNum ) {

            frame = frame->Next;
        }
        matInst->SetTexture( 0, frame->Object );
    }

    surf->DynamicRangeIndexCount = _NumIndices;
    surf->DynamicRangeStartIndexLocation = _FirstIndex;

    surf->VisMarker = GRenderFrontend.GetVisMarker();
}

void FQuakeBSPView::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

#if 0
    if ( Model ) {
        _DebugDraw->SetDepthTest( true );
        _DebugDraw->SetColor( 0,1,1,0.1f);
        for ( int i = 1 ; i < Model->Models.Length() ; i++ ) {
            _DebugDraw->DrawBoxFilled( Model->Models[i].BoundingBox.Center(), Model->Models[i].BoundingBox.HalfSize() );
        }

        _DebugDraw->SetDepthTest( false );
        _DebugDraw->SetColor( 0,1,1,0.3f);
        for ( int i = 1 ; i < Model->Models.Length() ; i++ ) {
            _DebugDraw->DrawBox( Model->Models[i].BoundingBox.Center(), Model->Models[i].BoundingBox.HalfSize() );
        }
    }
#endif
}
