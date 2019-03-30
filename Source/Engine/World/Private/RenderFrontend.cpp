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

#include <Engine/World/Public/RenderFrontend.h>
#include <Engine/World/Public/SceneComponent.h>
#include <Engine/World/Public/CameraComponent.h>
#include <Engine/World/Public/StaticMeshComponent.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/PlayerController.h>
#include <Engine/World/Public/GameMaster.h>

#include <Engine/Runtime/Public/Runtime.h>

#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

FRenderFrontend & GRenderFrontend = FRenderFrontend::Inst();

static FViewport * Viewports[MAX_RENDER_VIEWS];
static int NumViewports = 0;
static int MaxViewportWidth = 0;
static int MaxViewportHeight = 0;

FRenderFrontend::FRenderFrontend()
    //: DrawList( &DrawListShared )
{
}

void FRenderFrontend::Initialize() {
}

void FRenderFrontend::Deinitialize() {
    //DrawList.ClearFreeMemory();
}

// FUTURE: void FRenderFrontend::BuildFrameData( TPodArray< FWindow * > & _Windows ) {
void FRenderFrontend::BuildFrameData( FCanvas * _Canvas ) {

    CurFrameData = GRuntime.GetFrameData();

    CurFrameData->TickNumber = GGameMaster.GetTickNumber() - 1;
    CurFrameData->TimeStampMicro = GGameMaster.GetTickTimeStamp();
    CurFrameData->GameRunningTimeSeconds = GGameMaster.GetRunningTimeMicro() * 0.000001;
    CurFrameData->GameplayTimeSeconds = GGameMaster.GetGameplayTimeMicro() * 0.000001;

    FRenderProxy::FreeDeadProxies();

    MaxViewportWidth = 0;
    MaxViewportHeight = 0;

    NumViewports = 0;

#if 0
    // TODO: FUTURE:
    for ( FWindow * window : _Windows ) {
        if ( !window->IsVisible() ) {
            continue;
        }
        FCanvas * canvas = window->GetCanvas();

        window->DrawCanvas();

        VisMarker++;
        WriteDrawList( canvas->DrawList );
    }
#else
    if ( GGameMaster.IsWindowVisible() ) {
        VisMarker++;
        WriteDrawList( _Canvas );
    }
#endif

    CurFrameData->AllocSurfaceWidth = MaxViewportWidth;
    CurFrameData->AllocSurfaceHeight = MaxViewportHeight;
    CurFrameData->CanvasWidth = _Canvas->Width;
    CurFrameData->CanvasHeight = _Canvas->Height;
    CurFrameData->NumViews = NumViewports;
    CurFrameData->Instances.Clear();
    CurFrameData->DbgVertices.Clear();
    CurFrameData->DbgIndices.Clear();
    CurFrameData->DbgCmds.Clear();

    DebugDraw.Reset();

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }
}

struct FInstanceSortFunction {
    bool operator() ( FRenderInstance const * _A, FRenderInstance * _B ) {

        // Sort by material
        if ( _A->Material < _B->Material ) {
            return true;
        }

        if ( _A->Material > _B->Material ) {
            return false;
        }

        // Sort by material instance
        if ( _A->MaterialInstance < _B->MaterialInstance ) {
            return true;
        }

        if ( _A->MaterialInstance > _B->MaterialInstance ) {
            return false;
        }

        // Sort by vertex cache
        if ( _A->MeshRenderProxy < _B->MeshRenderProxy ) {
            return true;
        }

        if ( _A->MeshRenderProxy > _B->MeshRenderProxy ) {
            return false;
        }

        return false;
    }
} InstanceSortFunction;

//static void InstanceSortTest() {
//    TPodArray< FRenderInstance > Instances;

//    for ( int i = 0 ; i < 100 ; i++ ) {
//        FRenderInstance & instance = Instances.Append();
//        instance->Material = rand()%10;
//        instance->MaterialInstance = rand()%10;
//        instance->MeshRenderProxy = rand()%10;
//    }

//    StdSort( Instances.Begin(), Instances.End(), InstanceSortFunction );
//    for ( int i = 0 ; i < 100 ; i++ ) {
//        FRenderInstance & instance = Instances[i];
//        GLogger.Printf( "%u\t%u\t%u\n", instance->Material, instance->MaterialInstance, instance->MeshRenderProxy );
//    }
//}

void FRenderFrontend::UpdateMaterialInstanceFrameData( FMaterialInstance * _Instance ) {
    if ( _Instance->VisMarker == VisMarker ) {
        return;
    }

    _Instance->VisMarker = VisMarker;

    _Instance->FrameData = ( FMaterialInstanceFrameData * )CurFrameData->AllocFrameData( sizeof(FMaterialInstanceFrameData) );
    if ( !_Instance->FrameData ) {
        return;
    }

    _Instance->FrameData->Material = _Instance->Material->GetRenderProxy();

    FRenderProxy_Texture ** textures = _Instance->FrameData->Textures;
    _Instance->FrameData->NumTextures = 0;

    for ( int i = 0 ; i < MAX_MATERIAL_TEXTURES ; i++ ) {
        if ( _Instance->Textures[i] ) {

            FRenderProxy_Texture * textureProxy = _Instance->Textures[i]->GetRenderProxy();

            if ( textureProxy->IsSubmittedToRenderThread() ) {
                textures[i] = textureProxy;
                _Instance->FrameData->NumTextures = i + 1;
            } else {
                textures[i] = 0;
            }
        } else {
            textures[i] = 0;
        }
    }
}

void FRenderFrontend::WriteStaticMeshInstances() {
    bool bVisible = true;

    for ( FStaticMeshComponent * component = World->StaticMeshList
          ; component ; component = component->NextWorldMesh() ) {

        // TODO: check frustum and boundig boxes here?

        //BvAxisAlignedBox const & bounds = component->GetWorldBounds();


        bVisible = true;

        if ( component->VSDPasses & VSD_PASS_PORTALS ) {

            // TODO: check portals
            //bVisible = ...;

            if ( !bVisible ) {
                continue;
            }
        }

        if ( component->VSDPasses & VSD_PASS_BOUNDS ) {

            // TODO: check bounds

            bVisible = Frustum->CheckAABB( component->GetWorldBounds() );

            if ( !bVisible ) {
                continue;
            }
        }

        if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

            component->OnCustomVisibleStep( Camera, bVisible );

            if ( !bVisible ) {
                continue;
            }

        }

        if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {

            bVisible = component->VisMarker == VisMarker;

            if ( !bVisible ) {
                continue;
            }
        }

        FIndexedMeshSubpart * mesh = component->GetMeshSubpart();
        if ( !mesh ) {
            continue;
        }

        FRenderProxy * proxy = mesh->GetParent()->GetRenderProxy();

        FMaterialInstance * materialInstance = component->GetMaterialInstance();
        if ( !materialInstance || !materialInstance->Material /*|| !materialInstance->GetRenderProxy()*/ ) {
            //materialInstance = DefaultMaterial; // TODO
            continue;
        }

        UpdateMaterialInstanceFrameData( materialInstance );

        FActor * actor = component->GetParentActor();
        FLevel * level = actor->GetLevel();

        // Add render instance
        FRenderInstance * instance = ( FRenderInstance * )CurFrameData->AllocFrameData( sizeof( FRenderInstance ) );
        if ( !instance ) {
            continue;
        }
        CurFrameData->Instances.Append( instance );
        instance->Material = materialInstance->Material->GetRenderProxy();
        instance->MaterialInstance = materialInstance->FrameData;
        instance->MeshRenderProxy = proxy;

        if ( component->LightmapUVChannel ) {
            instance->LightmapUVChannel = component->LightmapUVChannel->GetRenderProxy();
            instance->LightmapOffset = component->LightmapOffset;
        } else {
            instance->LightmapUVChannel = nullptr;
        }

        if ( component->VertexLightChannel ) {
            instance->VertexLightChannel = component->VertexLightChannel->GetRenderProxy();
        } else {
            instance->VertexLightChannel = nullptr;
        }

        if ( component->LightmapBlock >= 0 && component->LightmapBlock < level->Lightmaps.Length() ) {
            instance->Lightmap = level->Lightmaps[ component->LightmapBlock ]->GetRenderProxy();
        } else {
            instance->Lightmap = nullptr; // TODO: white image?
        }

        if ( component->bUseDynamicRange ) {
            instance->IndexCount = component->DynamicRangeIndexCount;
            instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
            instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
        } else {
            instance->IndexCount = mesh->IndexCount;
            instance->StartIndexLocation = mesh->FirstIndex;
            instance->BaseVertexLocation = mesh->FirstVertex;
        }

        // TODO: Do it for non-indexed mesh
        //instance->VertexCount = component->GetVertexCount();
        //if ( !instance->VertexCount ) {
        //    instance->VertexCount = mesh->GetVertexCount();
        //}
        //instance->StartVertexLocation = component->GetStartVertexLocation();

        instance->Matrix = RV->ModelviewProjection * component->GetWorldTransformMatrix(); // TODO: optimize: parallel, sse, check if transformable

        //instance->ModelNormalToViewSpace = RV->NormalToView * component->GetWorldRotation().ToMatrix();

        RV->InstanceCount++;
    }
}

void FRenderFrontend::RenderView( int _Index ) {
    FViewport * viewport = Viewports[ _Index ];
    FPlayerController * controller = viewport->PlayerController;
    FCameraComponent * camera = controller->GetViewCamera();
    FRenderingParameters * rparams = controller->GetRenderingParameters();

    Camera = camera;
    World = camera->GetWorld();
    Frustum = &camera->GetFrustum();

    RV = &CurFrameData->RenderViews[_Index];

    RV->ViewIndex = _Index;
    RV->Width = viewport->Width;
    RV->Height = viewport->Height;
    RV->ViewPostion = Camera->GetWorldPosition();
    RV->ViewRotation = Camera->GetWorldRotation();
    RV->ViewMatrix = Camera->GetViewMatrix();
    RV->NormalToViewMatrix = Float3x3( RV->ViewMatrix );
    RV->ProjectionMatrix = Camera->GetProjectionMatrix();
    RV->InverseProjectionMatrix = Camera->IsPerspective() ?
                RV->ProjectionMatrix.PerspectiveProjectionInverseFast()
              : RV->ProjectionMatrix.OrthoProjectionInverseFast();
    RV->ModelviewProjection = RV->ProjectionMatrix * RV->ViewMatrix;
    RV->ViewSpaceToWorldSpace = RV->ViewMatrix.Inversed();
    RV->ClipSpaceToWorldSpace = RV->ViewSpaceToWorldSpace * RV->InverseProjectionMatrix;
    RV->BackgroundColor = rparams->BackgroundColor;
    RV->bClearBackground = rparams->bClearBackground;
    RV->bWireframe = rparams->bWireframe;
    RV->PresentCmd = 0;
    RV->FirstInstance = CurFrameData->Instances.Length();
    RV->InstanceCount = 0;

    VisMarker++;

    if ( rparams->bDebugDraw ) {
        // Generate world debug geomtry once per frame
        if ( World->VisFrame != GGameMaster.GetFrameNumber() ) {
            World->VisFrame = GGameMaster.GetFrameNumber();
            World->GenerateDebugDrawGeometry( &DebugDraw );
        }
        RV->FirstDbgCmd = World->GetFirstDebugDrawCommand();
        RV->DbgCmdCount = World->GetDebugDrawCommandCount();
    } else {
        RV->FirstDbgCmd = 0;
        RV->DbgCmdCount = 0;
    }

    // TODO: call this once per frame!
    controller->VisitViewActors();

    WriteStaticMeshInstances();

    //int64_t t = GRuntime.SysMilliseconds();
    StdSort( CurFrameData->Instances.Begin() + RV->FirstInstance,
             CurFrameData->Instances.Begin() + ( RV->FirstInstance + RV->InstanceCount ),
             InstanceSortFunction );
    //GLogger.Printf( "Sort instances time %d instances count %d\n", GRuntime.SysMilliseconds() - t, RV->InstanceCount );
}

void FRenderFrontend::WriteDrawList( FCanvas * _Canvas ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = &_Canvas->DrawList;

    if ( srcList->VtxBuffer.empty() ) {
        return;
    }

    FDrawList * drawList = ( FDrawList * )frameData->AllocFrameData( sizeof( FDrawList ) );
    if ( !drawList ) {
        return;
    }

    drawList->VerticesCount = srcList->VtxBuffer.size();
    drawList->IndicesCount = srcList->IdxBuffer.size();
    drawList->CommandsCount = srcList->CmdBuffer.size();

    int bytesCount = sizeof( FDrawVert ) * drawList->VerticesCount;
    drawList->Vertices = ( FDrawVert * )frameData->AllocFrameData( bytesCount );
    if ( !drawList->Vertices ) {
        return;
    }
    memcpy( drawList->Vertices, srcList->VtxBuffer.Data, bytesCount );

    bytesCount = sizeof( unsigned short ) * drawList->IndicesCount;
    drawList->Indices = ( unsigned short * )frameData->AllocFrameData( bytesCount );
    if ( !drawList->Indices ) {
        return;
    }

    memcpy( drawList->Indices, srcList->IdxBuffer.Data, bytesCount );

    bytesCount = sizeof( FDrawCmd ) * drawList->CommandsCount;
    drawList->Commands = ( FDrawCmd * )frameData->AllocFrameData( bytesCount );
    if ( !drawList->Commands ) {
        return;
    }

    int firstIndex = 0;

    FDrawCmd * dstCmd = drawList->Commands;
    for ( const ImDrawCmd * pCmd = srcList->CmdBuffer.begin() ; pCmd != srcList->CmdBuffer.end() ; pCmd++ ) {

        memcpy( &dstCmd->ClipMins, &pCmd->ClipRect, sizeof( Float4 ) );
        dstCmd->IndexCount = pCmd->ElemCount;
        dstCmd->StartIndexLocation = firstIndex;
        dstCmd->Type = (FCanvasDrawCmd)( pCmd->BlendingState & 0xff );
        dstCmd->Blending = (EColorBlending)( ( pCmd->BlendingState >> 8 ) & 0xff );
        dstCmd->SamplerType = (ESamplerType)( ( pCmd->BlendingState >> 16 ) & 0xff );

        firstIndex += pCmd->ElemCount;

        assert( pCmd->TextureId );

        switch ( dstCmd->Type ) {
        case CANVAS_DRAW_CMD_VIEWPORT:
        {
            if ( NumViewports >= MAX_RENDER_VIEWS ) {
                GLogger.Printf( "FRenderFrontend: MAX_RENDER_VIEWS hit\n" );
                drawList->CommandsCount--;
                continue;
            }

            FViewport * viewport = &_Canvas->Viewports[ (size_t)pCmd->TextureId - 1 ];

            dstCmd->ViewportIndex = NumViewports++;

            Viewports[ dstCmd->ViewportIndex ] = viewport;

            MaxViewportWidth = FMath::Max( MaxViewportWidth, viewport->Width );
            MaxViewportHeight = FMath::Max( MaxViewportHeight, viewport->Height );

            dstCmd++;

            break;
        }

        case CANVAS_DRAW_CMD_MATERIAL:
        {
            FMaterialInstance * materialInstance = static_cast< FMaterialInstance * >( pCmd->TextureId );
            if ( !materialInstance->Material ) {
                drawList->CommandsCount--;
                continue;
            }

            if ( materialInstance->Material->GetType() != MATERIAL_TYPE_HUD ) {
                drawList->CommandsCount--;
                continue;
            }

            UpdateMaterialInstanceFrameData( materialInstance );

            dstCmd->MaterialInstance = materialInstance->FrameData;
            AN_Assert( dstCmd->MaterialInstance );

            dstCmd++;

            break;
        }
        case CANVAS_DRAW_CMD_TEXTURE:
        case CANVAS_DRAW_CMD_ALPHA:
        {
            dstCmd->Texture = (FRenderProxy_Texture *)pCmd->TextureId;
            if ( !dstCmd->Texture->IsSubmittedToRenderThread() ) {
                drawList->CommandsCount--;
                continue;
            }
            dstCmd++;
            break;
        }
        default:
            assert( 0 );
            break;
        }
    }

    //GLogger.Printf("WriteDrawList: %d draw calls\n", drawList->CommandsCount );

    FDrawList * prev = frameData->DrawListTail;
    drawList->Next = nullptr;
    frameData->DrawListTail = drawList;
    if ( prev ) {
        prev->Next = drawList;
    } else {
        frameData->DrawListHead = drawList;
    }
}
