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

#include <Engine/GameEngine/Public/RenderFrontend.h>
#include <Engine/GameEngine/Public/SceneComponent.h>
#include <Engine/GameEngine/Public/CameraComponent.h>
#include <Engine/GameEngine/Public/MeshComponent.h>
#include <Engine/GameEngine/Public/SkeletalAnimation.h>
#include <Engine/GameEngine/Public/World.h>
#include <Engine/GameEngine/Public/Canvas.h>
#include <Engine/GameEngine/Public/PlayerController.h>
#include <Engine/GameEngine/Public/GameEngine.h>

#include <Engine/Runtime/Public/Runtime.h>

#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

FRenderFrontend & GRenderFrontend = FRenderFrontend::Inst();

extern FCanvas GCanvas;

static FViewport * Viewports[MAX_RENDER_VIEWS];
static int NumViewports = 0;
static int MaxViewportWidth = 0;
static int MaxViewportHeight = 0;

#define MAX_HULL_POINTS 128

FRenderFrontend::FRenderFrontend() {
}

void FRenderFrontend::Initialize() {
    for ( int i = 0 ; i < 2 ; i++ ) {
        Polygon[i] = FConvexHull::Create( MAX_HULL_POINTS );
    }
}

void FRenderFrontend::Deinitialize() {
    for ( int i = 0 ; i < 2 ; i++ ) {
        FConvexHull::Destroy( Polygon[i] );
    }
}

// FUTURE: void FRenderFrontend::BuildFrameData( TPodArray< FWindow * > & _Windows ) {
void FRenderFrontend::BuildFrameData() {

    ImDrawData * drawData = ImGui::GetDrawData();

    CurFrameData = GRuntime.GetFrameData();

    CurFrameData->FrameNumber = GGameEngine.GetFrameNumber();

    FRenderProxy::FreeDeadProxies();

    FrontendTime = GRuntime.SysMilliseconds();

    MaxViewportWidth = 0;
    MaxViewportHeight = 0;

    NumViewports = 0;

    PolyCount = 0;

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
    if ( GGameEngine.IsWindowVisible() ) {
        VisMarker++;
        WriteDrawList( &GCanvas );

        // -------------------------------------
        if ( drawData && drawData->CmdListsCount > 0 ) {
            // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
            int fb_width = drawData->DisplaySize.x * drawData->FramebufferScale.x;
            int fb_height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
            if ( fb_width == 0 || fb_height == 0 ) {
                return;
            }

            if ( drawData->FramebufferScale.x != 1.0f || drawData->FramebufferScale.y != 1.0f ) {
                drawData->ScaleClipRects( drawData->FramebufferScale );
            }

            bool bDrawMouseCursor = true;

            if ( bDrawMouseCursor ) {
                ImGuiMouseCursor cursor = ImGui::GetCurrentContext()->MouseCursor;

                ImDrawList * drawList = drawData->CmdLists[ drawData->CmdListsCount - 1 ];
                if ( cursor != ImGuiMouseCursor_None )
                {
                    AN_Assert( cursor > ImGuiMouseCursor_None && cursor < ImGuiMouseCursor_COUNT );

                    const ImU32 col_shadow = IM_COL32( 0, 0, 0, 48 );
                    const ImU32 col_border = IM_COL32( 0, 0, 0, 255 );          // Black
                    const ImU32 col_fill = IM_COL32( 255, 255, 255, 255 );    // White

                    Float2 pos = GGameEngine.GetCursorPosition();
                    float scale = 1.0f;

                    ImFontAtlas* font_atlas = drawList->_Data->Font->ContainerAtlas;
                    Float2 offset, size, uv[ 4 ];
                    if ( font_atlas->GetMouseCursorTexData( cursor, (ImVec2*)&offset, (ImVec2*)&size, (ImVec2*)&uv[ 0 ], (ImVec2*)&uv[ 2 ] ) ) {
                        pos -= offset;
                        const ImTextureID tex_id = font_atlas->TexID;
                        drawList->PushClipRectFullScreen();
                        drawList->PushTextureID( tex_id );
                        drawList->AddImage( tex_id, pos + Float2( 1, 0 )*scale, pos + Float2( 1, 0 )*scale + size*scale, uv[ 2 ], uv[ 3 ], col_shadow );
                        drawList->AddImage( tex_id, pos + Float2( 2, 0 )*scale, pos + Float2( 2, 0 )*scale + size*scale, uv[ 2 ], uv[ 3 ], col_shadow );
                        drawList->AddImage( tex_id, pos, pos + size*scale, uv[ 2 ], uv[ 3 ], col_border );
                        drawList->AddImage( tex_id, pos, pos + size*scale, uv[ 0 ], uv[ 1 ], col_fill );
                        drawList->PopTextureID();
                        drawList->PopClipRect();
                    }
                }
            }

            for ( int n = 0; n < drawData->CmdListsCount ; n++ ) {
                WriteDrawList( drawData->CmdLists[ n ] );
            }
        }
    }
#endif

    CurFrameData->AllocSurfaceWidth = MaxViewportWidth;
    CurFrameData->AllocSurfaceHeight = MaxViewportHeight;
    CurFrameData->CanvasWidth = GCanvas.Width;
    CurFrameData->CanvasHeight = GCanvas.Height;
    CurFrameData->NumViews = NumViewports;
    CurFrameData->Instances.Clear();
    CurFrameData->DbgVertices.Clear();
    CurFrameData->DbgIndices.Clear();
    CurFrameData->DbgCmds.Clear();

    DebugDraw.Reset();

    UpdateSurfaceAreas();

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }

    FrontendTime = GRuntime.SysMilliseconds() - FrontendTime;
}

void FRenderFrontend::UpdateSurfaceAreas() {
    FSpatialObject * next;
    for ( FSpatialObject * surf = FSpatialObject::DirtyList ; surf ; surf = next ) {

        next = surf->NextDirty;

        FWorld * world = surf->GetWorld();

        for ( FLevel * level : world->ArrayOfLevels ) {
            level->RemoveSurfaceAreas( surf );
        }

        for ( FLevel * level : world->ArrayOfLevels ) {
            level->AddSurfaceAreas( surf );
            //GLogger.Printf( "Update actor %s\n", surf->GetParentActor()->GetNameConstChar() );
        }

        surf->PrevDirty = surf->NextDirty = nullptr;
    }
    FSpatialObject::DirtyList = FSpatialObject::DirtyListTail = nullptr;
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

        // Sort by mesh
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

    _Instance->FrameData->NumUniformVectors = _Instance->Material->GetNumUniformVectors();
    memcpy( _Instance->FrameData->UniformVectors, _Instance->UniformVectors, sizeof(Float4)*_Instance->FrameData->NumUniformVectors );
}

void FRenderFrontend::AddInstances() {
#if 1
    for ( FLevel * level : World->ArrayOfLevels ) {

        // Update view node
        ViewArea = level->FindArea( ViewOrigin );

        // Cull invisible objects
        CullLevelInstances( level );
    }
#else
    PlaneF AreaFrustum[4];
    int PlanesCount;
    AreaFrustum[0] = (*Frustum)[0];
    AreaFrustum[1] = (*Frustum)[1];
    AreaFrustum[2] = (*Frustum)[2];
    AreaFrustum[3] = (*Frustum)[3];
    PlanesCount = 4;
    for ( FMeshComponent * component = World->MeshList ; component ; component = component->GetNextMesh() ) {
        AddSurface( component, AreaFrustum, PlanesCount );
    }
#ifdef FUTURE
    for ( FLightComponent * component = World->LightList ; component ; component = component->GetNextLight() ) {
        AddLight( component, AreaFrustum, PlanesCount );
    }
    for ( FEnvCaptureComponent * component = World->EnvCaptureList ; component ; component = component->GetNextEnvCapture() ) {
        AddEnvCapture( component, AreaFrustum, PlanesCount );
    }
#endif
#endif
}

void FRenderFrontend::RenderView( int _Index ) {
    FViewport * viewport = Viewports[ _Index ];
    FPlayerController * controller = viewport->PlayerController;
    FCameraComponent * camera = controller->GetViewCamera();

    RP = controller->GetRenderingParameters();
    Camera = camera;
    World = camera->GetWorld();
    Frustum = &camera->GetFrustum();
    ViewOrigin = camera->GetWorldPosition();

    RV = &CurFrameData->RenderViews[_Index];

    RV->GameRunningTimeSeconds = World->GetRunningTimeMicro() * 0.000001;
    RV->GameplayTimeSeconds = World->GetGameplayTimeMicro() * 0.000001;

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
    RV->BackgroundColor = RP->BackgroundColor;
    RV->bClearBackground = RP->bClearBackground;
    RV->bWireframe = RP->bWireframe;
    RV->PresentCmd = 0;
    RV->FirstInstance = CurFrameData->Instances.Length();
    RV->InstanceCount = 0;

    VisMarker++;

    if ( RP->bDrawDebug ) {
        World->DrawDebug( &DebugDraw );
        RV->FirstDbgCmd = World->GetFirstDebugDrawCommand();
        RV->DbgCmdCount = World->GetDebugDrawCommandCount();
    } else {
        RV->FirstDbgCmd = 0;
        RV->DbgCmdCount = 0;
    }

    // FIXME: where is better to call this?
    //controller->UpdateAudioListener();

    // TODO: call this once per frame!
    controller->VisitViewActors();

    AddInstances();

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

        AN_Assert( pCmd->TextureId );

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
            AN_Assert( 0 );
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

void FRenderFrontend::WriteDrawList( ImDrawList const * _DrawList ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = _DrawList;

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

        AN_Assert( pCmd->TextureId );

        switch ( dstCmd->Type ) {
        case CANVAS_DRAW_CMD_VIEWPORT:
        {
            drawList->CommandsCount--;
            continue;
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
            AN_Assert( 0 );
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


static int Dbg_SkippedByVisFrame;
static int Dbg_SkippedByPlaneOffset;
static int Dbg_CulledBySurfaceBounds;
static int Dbg_CulledByDotProduct;
static int Dbg_CulledByLightBounds;
static int Dbg_CulledByEnvCaptureBounds;
static int Dbg_ClippedPortals;
static int Dbg_PassedPortals;
static int Dbg_StackDeep;

#define MAX_PORTAL_STACK 64

struct FPortalScissor {
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;
};

struct FPortalStack {
    PlaneF AreaFrustum[4];
    int PlanesCount;
    FAreaPortal const * Portal;
    FPortalScissor Scissor;
};

static FPortalStack PortalStack[ MAX_PORTAL_STACK ];
static int PortalStackPos;

static Float3 RightVec;
static Float3 UpVec;
static PlaneF ViewPlane;
static float ViewZNear;
static Float3 ViewCenter;

static float ClipDistances[MAX_HULL_POINTS];
static EPlaneSide ClipSides[MAX_HULL_POINTS];

//#define DEBUG_SCISSORS

#ifdef DEBUG_SCISSORS
static TArray< FPortalScissor > DebugScissors;
#endif

#ifdef FUTURE
void FSpatialTreeComponent::DrawDebug( FCameraComponent * _Camera, EDebugDrawFlags::Type _DebugDrawFlags, FPrimitiveBatchComponent * _DebugDraw ) {

    _DebugDraw->SetZTest( false );
#ifdef DEBUG_SCISSORS
    //for ( const FPortalScissor & Scissor : DebugScissors ) {
    for ( int i = 0 ; i < DebugScissors.Length() ; i++ ) {
        const FPortalScissor & Scissor = DebugScissors[i];

        Float3 Org = _Camera->GetNode()->GetWorldPosition();

        Float3 Center = Org + ViewPlane.Normal * ViewZNear;
        Float3 Corners[ 4 ];
        Float3 RightMin = RightVec * Scissor.MinX + Center;
        Float3 RightMax = RightVec * Scissor.MaxX + Center;
        Float3 UpMin = UpVec * Scissor.MinY;
        Float3 UpMax = UpVec * Scissor.MaxY;
        Corners[ 0 ] = RightMin + UpMin;
        Corners[ 1 ] = RightMax + UpMin;
        Corners[ 2 ] = RightMax + UpMax;
        Corners[ 3 ] = RightMin + UpMax;

        _DebugDraw->SetPrimitive( P_LineLoop );
        _DebugDraw->SetColor( 0, 0, 1, 1 );
        _DebugDraw->EmitPoint( Corners[ 0 ] );
        _DebugDraw->EmitPoint( Corners[ 1 ] );
        _DebugDraw->EmitPoint( Corners[ 2 ] );
        _DebugDraw->EmitPoint( Corners[ 3 ] );
        _DebugDraw->Flush();
    }
#endif
#if 1
    if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_SPATIAL_PORTALS ) {
        if ( ViewArea >= 0 && ViewArea < Areas.Length() ) {

            FSpatialAreaComponent * pLeaf = Areas[ ViewArea ];
            FAreaPortal * Portals = pLeaf->PortalList;

            if ( Portals ) {
                if ( !(_DebugDrawFlags & EDebugDrawFlags::DRAW_SPATIAL_AREA_BOUNDS) ) {
                    _DebugDraw->SetColor( 1, 0, 1, 1 );
                    _DebugDraw->DrawAABB( pLeaf->Bounds );
                }

                // Draw portal border
                _DebugDraw->SetPolygonMode( FPrimitiveBatchComponent::PM_Solid );

                // Draw portal polygon
                _DebugDraw->SetPrimitive( P_TriangleFan );
                _DebugDraw->SetColor( 0, 1, 1, 0.2f );
                for ( FAreaPortal * P = Portals; P ; P = P->Next ) {
                    for ( int i = 0 ; i < P->Hull.Length() ; i++ ) {
                        _DebugDraw->EmitPoint( P->Hull[ i ] );
                    }
                    _DebugDraw->Flush();
                }
            }
        }
    }
#endif
}
#endif

AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, Float3 const & _Mins, Float3 const & _Maxs ) {
    bool inside = true;
    for ( PlaneF const * p = _Planes ; p < _Planes + _Count ; p++ ) {
        inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X )
                  + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y )
                  + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z )
                  + p->D ) > 0;
    }
    return !inside;
}

AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, BvAxisAlignedBox const & _AABB ) {
    return Cull( _Planes, _Count, _AABB.Mins, _AABB.Maxs );
}

AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, BvSphereSSE const & _Sphere ) {
    bool cull = false;
    for ( const PlaneF * p = _Planes ; p < _Planes + _Count ; p++ ) {
        if ( FMath::Dot( p->Normal, _Sphere.Center ) + p->D <= -_Sphere.Radius ) {
            cull = true;
        }
    }
    return cull;
}

static bool ClipPolygonOptimized( FConvexHull const * _In, FConvexHull * _Out, const PlaneF & _Plane, const float _Epsilon ) {
    int Front = 0;
    int Back = 0;
    int i;
    float Dist;

    assert( _In->NumPoints + 4 <= MAX_HULL_POINTS );

    // Определить с какой стороны находится каждая точка исходного полигона
    for ( i = 0 ; i < _In->NumPoints ; i++ ) {
        Dist = _In->Points[i].Dot( _Plane.Normal ) + _Plane.D;

        ClipDistances[ i ] = Dist;

        if ( Dist > _Epsilon ) {
            ClipSides[ i ] = EPlaneSide::Front;
            Front++;
        } else if ( Dist < -_Epsilon ) {
            ClipSides[ i ] = EPlaneSide::Back;
            Back++;
        } else {
            ClipSides[ i ] = EPlaneSide::On;
        }
    }

    if ( !Front ) {
        // Все точки находятся по заднюю сторону плоскости
        _Out->NumPoints = 0;
        return true;
    }

    if ( !Back ) {
        // Все точки находятся по фронтальную сторону плоскости
        return false;
    }

    _Out->NumPoints = 0;

    ClipSides[i] = ClipSides[0];
    ClipDistances[i] = ClipDistances[0];

    for ( i = 0 ; i < _In->NumPoints ; i++ ) {
        Float3 const & v = _In->Points[i];

        if ( ClipSides[ i ] == EPlaneSide::On ) {
            _Out->Points[_Out->NumPoints++] =  v;
            continue;
        }

        if ( ClipSides[ i ] == EPlaneSide::Front ) {
            _Out->Points[_Out->NumPoints++] =  v;
        }

        EPlaneSide NextSide = ClipSides[ i + 1 ];

        if ( NextSide == EPlaneSide::On || NextSide == ClipSides[ i ] ) {
            continue;
        }

        Float3 & NewVertex = _Out->Points[_Out->NumPoints++];

        NewVertex = _In->Points[ ( i + 1 ) % _In->NumPoints ];

        Dist = ClipDistances[ i ] / ( ClipDistances[ i ] - ClipDistances[ i + 1 ] );
        //for ( int j = 0 ; j < 3 ; j++ ) {
        //	if ( _Plane.Normal[ j ] == 1 ) {
        //		NewVertex[ j ] = -_Plane.D;
        //	} else if ( _Plane.Normal[ j ] == -1 ) {
        //		NewVertex[ j ] = _Plane.D;
        //	} else {
        //		NewVertex[ j ] = v[ j ] + Dist * ( NewVertex[j] - v[j] );
        //	}
        //}
        NewVertex = v + Dist * ( NewVertex - v );
    }

    return true;
}

void FRenderFrontend::FlowThroughPortals_r( FLevelArea * _Area ) {
    static BvAxisAlignedBox Bounds;
    FPortalStack * PrevStack = &PortalStack[ PortalStackPos ];
    FPortalStack * Stack = PrevStack + 1;

    for ( FSpatialObject * surf : _Area->GetSurfs() ) {
        FMeshComponent * component = Upcast< FMeshComponent >(surf);

        if ( component ) {
            AddSurface( component, PrevStack->AreaFrustum, PrevStack->PlanesCount );
        } else {
            //GLogger.Printf( "Not a mesh\n" );
        }
    }

#ifdef FUTURE
    for ( FLightComponent * light : _Area->GetLights() ) {
        AddLight( light, PrevStack->AreaFrustum, PrevStack->PlanesCount );
    }

    for ( FEnvCaptureComponent * envCapture : _Area->GetEnvCaptures() ) {
        AddEnvCapture( envCapture, PrevStack->AreaFrustum, PrevStack->PlanesCount );
    }
#endif

    if ( PortalStackPos == ( MAX_PORTAL_STACK - 1 ) ) {
        GLogger.Printf( "MAX_PORTAL_STACK hit\n" );
        return;
    }

    ++PortalStackPos;

    Dbg_StackDeep = FMath::Max( Dbg_StackDeep, PortalStackPos );

    static float x, y, d;
    static Float3 Vec;
    static Float3 p;
    static Float3 RightMin;
    static Float3 RightMax;
    static Float3 UpMin;
    static Float3 UpMax;
    static Float3 Corners[ 4 ];
    static int Flip = 0;

    for ( FAreaPortal const * P = _Area->GetPortals() ; P ; P = P->Next ) {

        //if ( P->DoublePortal->VisFrame == VisMarker ) {
        //    Dbg_SkippedByVisFrame++;
        //    continue;
        //}

        d = P->Plane.Dist( ViewOrigin );
        if ( d <= 0.0f ) {
            Dbg_SkippedByPlaneOffset++;
            continue;
        }

        if ( d > 0.0f && d <= ViewZNear ) {
            // View intersecting the portal

            for ( int i = 0 ; i < PrevStack->PlanesCount ; i++ ) {
                Stack->AreaFrustum[ i ] = PrevStack->AreaFrustum[ i ];
            }
            Stack->PlanesCount = PrevStack->PlanesCount;
            Stack->Scissor = PrevStack->Scissor;

        } else {

            //for ( int i = 0 ; i < PortalStackPos ; i++ ) {
            //    if ( PortalStack[ i ].Portal == P ) {
            //        GLogger.Printf( "Recursive!\n" );
            //    }
            //}

            // Clip portal winding by view plane
            //static PolygonF Winding;
            //PolygonF * PortalWinding = &P->Hull;
            //if ( ClipPolygonOptimized( P->Hull, Winding, ViewPlane, 0.0f ) ) {
            //    if ( Winding.IsEmpty() ) {
            //        Dbg_ClippedPortals++;
            //        continue; // Culled
            //    }
            //    PortalWinding = &Winding;
            //}

            if ( !ClipPolygonOptimized( P->Hull, Polygon[ Flip ], ViewPlane, 0.0f ) ) {
                Polygon[ Flip ]->RecreateFromPoints( Polygon[ Flip ], P->Hull->Points, P->Hull->NumPoints );
            }

            if ( Polygon[ Flip ]->NumPoints >= 3 ) {
                for ( int i = 0 ; i < PrevStack->PlanesCount ; i++ ) {
                    if ( ClipPolygonOptimized( Polygon[ Flip ], Polygon[ ( Flip + 1 ) & 1 ], PrevStack->AreaFrustum[ i ], 0.0f ) ) {
                        Flip = ( Flip + 1 ) & 1;

                        if ( Polygon[ Flip ]->NumPoints < 3 ) {
                            break;
                        }
                    }
                }
            }

            FConvexHull * PortalWinding = Polygon[ Flip ];

            if ( PortalWinding->NumPoints < 3 ) {
                // Invisible
                Dbg_ClippedPortals++;
                continue;
            }

            float & MinX = Stack->Scissor.MinX;
            float & MinY = Stack->Scissor.MinY;
            float & MaxX = Stack->Scissor.MaxX;
            float & MaxY = Stack->Scissor.MaxY;

            MinX = 99999999.0f;
            MinY = 99999999.0f;
            MaxX = -99999999.0f;
            MaxY = -99999999.0f;

            for ( int i = 0 ; i < PortalWinding->NumPoints ; i++ ) {

                // Project portal vertex to view plane
                Vec = PortalWinding->Points[ i ] - ViewOrigin;

                d = FMath::Dot( ViewPlane.Normal, Vec );

                //if ( d < ViewZNear ) {
                //    assert(0);
                //}

                p = d < ViewZNear ? Vec : Vec * ( ViewZNear / d );

                // Compute relative coordinates
                x = FMath::Dot( RightVec, p );
                y = FMath::Dot( UpVec, p );

                // Compute bounds
                MinX = FMath::Min( x, MinX );
                MinY = FMath::Min( y, MinY );

                MaxX = FMath::Max( x, MaxX );
                MaxY = FMath::Max( y, MaxY );
            }

            // Clip bounds by current scissor bounds
            MinX = FMath::Max( PrevStack->Scissor.MinX, MinX );
            MinY = FMath::Max( PrevStack->Scissor.MinY, MinY );
            MaxX = FMath::Min( PrevStack->Scissor.MaxX, MaxX );
            MaxY = FMath::Min( PrevStack->Scissor.MaxY, MaxY );

            if ( MinX >= MaxX || MinY >= MaxY ) {
                // invisible
                Dbg_ClippedPortals++;
                continue; // go to next portal
            }

            // Compute 3D frustum to cull objects inside vis area
            if ( PortalWinding->NumPoints <= 4 ) {
                Stack->PlanesCount = PortalWinding->NumPoints;

                // Compute based on portal winding
                for ( int i = 0 ; i < Stack->PlanesCount ; i++ ) {
                    Stack->AreaFrustum[ i ].FromPoints( ViewOrigin, PortalWinding->Points[ ( i + 1 ) % PortalWinding->NumPoints ], PortalWinding->Points[ i ] );
                }
            } else {
                // Compute based on portal scissor
                RightMin = RightVec * MinX + ViewCenter;
                RightMax = RightVec * MaxX + ViewCenter;
                UpMin = UpVec * MinY;
                UpMax = UpVec * MaxY;
                Corners[ 0 ] = RightMin + UpMin;
                Corners[ 1 ] = RightMax + UpMin;
                Corners[ 2 ] = RightMax + UpMax;
                Corners[ 3 ] = RightMin + UpMax;

                // bottom
                p = FMath::Cross( Corners[ 1 ], Corners[ 0 ] );
                Stack->AreaFrustum[ 0 ].Normal = p * FMath::RSqrt( FMath::Dot( p, p ) );
                Stack->AreaFrustum[ 0 ].D = -FMath::Dot( Stack->AreaFrustum[ 0 ].Normal, ViewOrigin );

                // right
                p = FMath::Cross( Corners[ 2 ], Corners[ 1 ] );
                Stack->AreaFrustum[ 1 ].Normal = p * FMath::RSqrt( FMath::Dot( p, p ) );
                Stack->AreaFrustum[ 1 ].D = -FMath::Dot( Stack->AreaFrustum[ 1 ].Normal, ViewOrigin );

                // top
                p = FMath::Cross( Corners[ 3 ], Corners[ 2 ] );
                Stack->AreaFrustum[ 2 ].Normal = p * FMath::RSqrt( FMath::Dot( p, p ) );
                Stack->AreaFrustum[ 2 ].D = -FMath::Dot( Stack->AreaFrustum[ 2 ].Normal, ViewOrigin );

                // left
                p = FMath::Cross( Corners[ 0 ], Corners[ 3 ] );
                Stack->AreaFrustum[ 3 ].Normal = p * FMath::RSqrt( FMath::Dot( p, p ) );
                Stack->AreaFrustum[ 3 ].D = -FMath::Dot( Stack->AreaFrustum[ 3 ].Normal, ViewOrigin );

                Stack->PlanesCount = 4;
            }
        }

#ifdef DEBUG_SCISSORS
        DebugScissors.Append( Stack->Scissor );
#endif

        Dbg_PassedPortals++;

        Stack->Portal = P;

        P->Owner->VisMark = VisMarker;
        FlowThroughPortals_r( P->ToArea );
    }

    --PortalStackPos;
}

void FRenderFrontend::CullLevelInstances( FLevel * _Level ) {
    AN_Assert( ViewArea < _Level->GetAreas().Length() );

    Dbg_SkippedByVisFrame = 0;
    Dbg_SkippedByPlaneOffset = 0;
    Dbg_CulledBySurfaceBounds = 0;
    Dbg_CulledByDotProduct = 0;
    Dbg_CulledByLightBounds = 0;
    Dbg_CulledByEnvCaptureBounds = 0;
    Dbg_ClippedPortals = 0;
    Dbg_PassedPortals = 0;
    Dbg_StackDeep = 0;
#ifdef DEBUG_SCISSORS
    DebugScissors.Clear();
#endif

    RightVec = Camera->GetWorldRightVector();
    UpVec = Camera->GetWorldUpVector();
    ViewPlane = ( *Frustum )[ FPL_NEAR ];
    ViewZNear = ViewPlane.Dist( ViewOrigin );//Camera->GetZNear();
    ViewCenter = ViewPlane.Normal * ViewZNear;

    // Get corner at left-bottom of frustum
    Float3 Corner = FMath::Cross( ( *Frustum )[ FPL_BOTTOM ].Normal, ( *Frustum )[ FPL_LEFT ].Normal );

    // Project left-bottom corner to near plane
    Corner = Corner * ( ViewZNear / FMath::Dot( ViewPlane.Normal, Corner ) );

    float x = FMath::Dot( RightVec, Corner );
    float y = FMath::Dot( UpVec, Corner );

    // w = tan( half_fov_x_rad ) * znear * 2;
    // h = tan( half_fov_y_rad ) * znear * 2;

    PortalStackPos = 0;
    PortalStack[0].AreaFrustum[0] = (*Frustum)[0];
    PortalStack[0].AreaFrustum[1] = (*Frustum)[1];
    PortalStack[0].AreaFrustum[2] = (*Frustum)[2];
    PortalStack[0].AreaFrustum[3] = (*Frustum)[3];
    PortalStack[0].PlanesCount = 4;
    PortalStack[0].Portal = NULL;
    PortalStack[0].Scissor.MinX = x;
    PortalStack[0].Scissor.MinY = y;
    PortalStack[0].Scissor.MaxX = -x;
    PortalStack[0].Scissor.MaxY = -y;

    //DebugScissors.Append( PortalStack[0].Scissor );

    FlowThroughPortals_r( ViewArea >= 0 ? _Level->Areas[ ViewArea ] : _Level->OutdoorArea );

    //GLogger.Printf( "VSD: VisFrame %d\n", Dbg_SkippedByVisFrame );
    //GLogger.Printf( "VSD: PlaneOfs %d\n", Dbg_SkippedByPlaneOffset );
    //GLogger.Printf( "VSD: FaceCull %d\n", Dbg_CulledByDotProduct );
    //GLogger.Printf( "VSD: AABBCull %d\n", Dbg_CulledBySurfaceBounds );
    //GLogger.Printf( "VSD: LightCull %d\n", Dbg_CulledByLightBounds );
    //GLogger.Printf( "VSD: EnvCaptureCull %d\n", Dbg_CulledByEnvCaptureBounds );
    //GLogger.Printf( "VSD: Clipped %d\n", Dbg_ClippedPortals );
    //GLogger.Printf( "VSD: PassedPortals %d\n", Dbg_PassedPortals );
    //GLogger.Printf( "VSD: StackDeep %d\n", Dbg_StackDeep );
}

void FRenderFrontend::AddSurface( FMeshComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    if ( component->RenderMark == VisMarker ) {
        return;
    }

    if ( ( component->RenderingGroup & RP->RenderingMask ) == 0 ) {
        component->RenderMark = VisMarker;
        return;
    }

    if ( component->VSDPasses & VSD_PASS_FACE_CULL ) {
        const bool bTwoSided = false;
        const bool bFrontSided = true;
        const float EPS = 0.25f;

        if ( !bTwoSided ) {
            PlaneF const & plane = component->FacePlane;
            float d = ViewOrigin.Dot( plane.Normal );

            bool bFaceCull = false;

            if ( bFrontSided ) {
                if ( d < -plane.D - EPS ) {
                    bFaceCull = true;
                }
            } else {
                if ( d > -plane.D + EPS ) {
                    bFaceCull = true;
                }
            }

            if ( bFaceCull ) {
                component->RenderMark = VisMarker;
                Dbg_CulledByDotProduct++;
                return;
            }
        }
    }

    if ( component->VSDPasses & VSD_PASS_BOUNDS ) {

        // TODO: use SSE cull
        BvAxisAlignedBox const & bounds = component->GetWorldBounds();

        if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
            Dbg_CulledBySurfaceBounds++;
            return;
        }
    }

    component->RenderMark = VisMarker;

    if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        bool bVisible;
        component->OnCustomVisibleStep( Camera, bVisible );

        if ( !bVisible ) {
            return;
        }
    }

    if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        bool bVisible = component->VisMarker == VisMarker;
        if ( !bVisible ) {
            return;
        }
    }

    Float4x4 tmpMatrix;
    Float4x4 * instanceMatrix;

    FIndexedMesh * mesh = component->GetMesh();
    if ( !mesh ) {
        // TODO: default mesh?
        return;
    }

    FRenderProxy_Skeleton * skeletonProxy = nullptr;
    if ( mesh->IsSkinned() && component->IsSkinnedMesh() ) {
        FSkinnedComponent * skeleton = static_cast< FSkinnedComponent * >( component );
        skeleton->UpdateJointTransforms();
        skeletonProxy = skeleton->GetRenderProxy();
        if ( !skeletonProxy->IsSubmittedToRenderThread() ) {
            skeletonProxy = nullptr;
        }
    }

    if ( component->bNoTransform ) {
        instanceMatrix = &RV->ModelviewProjection;
    } else {
        tmpMatrix = RV->ModelviewProjection * component->GetWorldTransformMatrix(); // TODO: optimize: parallel, sse, check if transformable
        instanceMatrix = &tmpMatrix;
    }

    FActor * actor = component->GetParentActor();
    FLevel * level = actor->GetLevel();

    FIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0 ; subpartIndex < subparts.Length() ; subpartIndex++ ) {

        // FIXME: check subpart bounding box here

        FIndexedMeshSubpart * subpart = subparts[subpartIndex];

        FRenderProxy_IndexedMesh * proxy = mesh->GetRenderProxy();

        FMaterialInstance * materialInstance = component->GetMaterialInstance( subpartIndex );
        if ( !materialInstance || !materialInstance->Material ) {
            //materialInstance = DefaultMaterial; // TODO
            continue;
        }

        UpdateMaterialInstanceFrameData( materialInstance );

        // Add render instance
        FRenderInstance * instance = ( FRenderInstance * )CurFrameData->AllocFrameData( sizeof( FRenderInstance ) );
        if ( !instance ) {
            return;
        }

        CurFrameData->Instances.Append( instance );

        instance->Material = materialInstance->Material->GetRenderProxy();
        instance->MaterialInstance = materialInstance->FrameData;
        instance->MeshRenderProxy = proxy;

        if ( component->LightmapUVChannel && component->LightmapBlock >= 0 && component->LightmapBlock < level->Lightmaps.Length() ) {
            instance->LightmapUVChannel = component->LightmapUVChannel->GetRenderProxy();
            instance->LightmapOffset = component->LightmapOffset;
            instance->Lightmap = level->Lightmaps[ component->LightmapBlock ]->GetRenderProxy();
        } else {
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
        }

        if ( component->VertexLightChannel ) {
            instance->VertexLightChannel = component->VertexLightChannel->GetRenderProxy();
        } else {
            instance->VertexLightChannel = nullptr;
        }

        if ( component->bUseDynamicRange ) {
            instance->IndexCount = component->DynamicRangeIndexCount;
            instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
            instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
        } else {
            instance->IndexCount = subpart->IndexCount;
            instance->StartIndexLocation = subpart->FirstIndex;
            instance->BaseVertexLocation = subpart->BaseVertex + component->SubpartBaseVertexOffset;
        }

        instance->Skeleton = skeletonProxy;
        instance->Matrix = *instanceMatrix;

        if ( materialInstance->Material->GetType() == MATERIAL_TYPE_PBR ) {
            instance->ModelNormalToViewSpace = RV->NormalToViewMatrix * component->GetWorldRotation().ToMatrix();
        }

        RV->InstanceCount++;

        PolyCount += instance->IndexCount / 3;
    }
}

#ifdef FUTURE
void FRenderFrontend::AddLight( FLightComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    if ( component->RenderMark == VisMarker ) {
        return;
    }

    if ( ( component->RenderingGroup & RP->RenderingLayers ) == 0 ) {
        component->RenderMark = VisMarker;
        return;
    }

    if ( component->VSDPasses & VSD_PASS_BOUNDS ) {

        // TODO: use SSE cull
        
        switch ( Light->GetType() ) {
            case FLightComponent::T_Point:
            {
                BvSphereSSE const & bounds = Light->GetSphereWorldBounds();
                if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                    Dbg_CulledByLightBounds++;
                    return;
                }
                break;
            }
            case FLightComponent::T_Spot:
            {
                BvSphereSSE const & bounds = Light->GetSphereWorldBounds();
                if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                    Dbg_CulledByLightBounds++;
                    return;
                }
                break;
            }
            case FLightComponent::T_Direction:
            {
                break;
            }
        }
    }

    component->RenderMark = VisMarker;

    if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        bool bVisible;
        component->OnCustomVisibleStep( Camera, bVisible );

        if ( !bVisible ) {
            return;
        }
    }

    if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        bool bVisible = component->VisMarker == VisMarker;
        if ( !bVisible ) {
            return;
        }
    }

    // TODO: add to render view

    RV->LightCount++;
}

void FRenderFrontend::AddEnvCapture( FEnvCaptureComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    if ( component->RenderMark == VisMarker ) {
        return;
    }

    if ( ( component->RenderingGroup & RP->RenderingLayers ) == 0 ) {
        component->RenderMark = VisMarker;
        return;
    }

    if ( component->VSDPasses & VSD_PASS_BOUNDS ) {

        // TODO: use SSE cull
        
        switch ( component->GetShapeType() ) {
        case FEnvCaptureComponent::SHAPE_BOX:
        {
            // Check OBB ?
            BvAxisAlignedBox const & bounds = component->GetAABBWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByEnvCaptureBounds++;
                return;
            }
            break;
        }
        case FEnvCaptureComponent::SHAPE_SPHERE:
        {
            BvSphereSSE const & bounds = component->GetSphereWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByEnvCaptureBounds++;
                return;
            }
            break;
        }
        case FEnvCaptureComponent::SHAPE_GLOBAL:
        {
            break;
        }
        }
    }

    component->RenderMark = VisMarker;

    if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        bool bVisible;
        component->OnCustomVisibleStep( Camera, bVisible );

        if ( !bVisible ) {
            return;
        }
    }

    if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        bool bVisible = component->VisMarker == VisMarker;
        if ( !bVisible ) {
            return;
        }
    }

    // TODO: add to render view

    RV->EnvCaptureCount++;
}
#endif
