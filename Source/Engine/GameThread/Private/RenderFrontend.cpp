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

#include <Engine/GameThread/Public/RenderFrontend.h>
#include <Engine/GameThread/Public/EngineInstance.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/CameraComponent.h>
#include <Engine/World/Public/Components/SkinnedComponent.h>
#include <Engine/World/Public/Actors/PlayerController.h>
#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

FRenderFrontend & GRenderFrontend = FRenderFrontend::Inst();

extern FCanvas GCanvas;

FRenderFrontend::FRenderFrontend() {
}

void FRenderFrontend::Initialize() {
}

void FRenderFrontend::Deinitialize() {
}

struct FInstanceSortFunction {
    bool operator() ( FRenderInstance const * _A, FRenderInstance * _B ) {

        // Sort by render order
        if ( _A->RenderingOrder < _B->RenderingOrder ) {
            return true;
        }

        if ( _A->RenderingOrder > _B->RenderingOrder ) {
            return false;
        }

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
        if ( _A->VertexBuffer < _B->VertexBuffer ) {
            return true;
        }

        if ( _A->VertexBuffer > _B->VertexBuffer ) {
            return false;
        }

        return false;
    }
} InstanceSortFunction;

struct FShadowInstanceSortFunction {
    bool operator() ( FShadowRenderInstance const * _A, FShadowRenderInstance * _B ) {

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
        if ( _A->VertexBuffer < _B->VertexBuffer ) {
            return true;
        }

        if ( _A->VertexBuffer > _B->VertexBuffer ) {
            return false;
        }

        return false;
    }
} ShadowInstanceSortFunction;

void FRenderFrontend::Render() {
    CurFrameData = GRuntime.GetFrameData();

    CurFrameData->FrameNumber = GEngine.GetFrameNumber();
    CurFrameData->DrawListHead = nullptr;
    CurFrameData->DrawListTail = nullptr;

    Stat.FrontendTime = GRuntime.SysMilliseconds();
    Stat.PolyCount = 0;
    Stat.ShadowMapPolyCount = 0;

    MaxViewportWidth = 0;
    MaxViewportHeight = 0;
    NumViewports = 0;

    if ( GEngine.IsWindowVisible() )
    {
        VisMarker++;

        // Render canvas
        RenderCanvas( &GCanvas );

        // Draw cursor just right before rendering to reduce input latency
        WDesktop * desktop = GEngine.GetDesktop();
        if ( desktop && desktop->IsCursorVisible() )
        {
            GCanvas.Begin( GCanvas.Width, GCanvas.Height );
            desktop->DrawCursor( GCanvas );
            GCanvas.End();

            // Render cursor
            RenderCanvas( &GCanvas );
        }

        //RenderImgui();
    }

    CurFrameData->AllocSurfaceWidth = MaxViewportWidth;
    CurFrameData->AllocSurfaceHeight = MaxViewportHeight;
    CurFrameData->CanvasWidth = GCanvas.Width;
    CurFrameData->CanvasHeight = GCanvas.Height;
    CurFrameData->NumViews = NumViewports;
    CurFrameData->Instances.Clear();
    CurFrameData->ShadowInstances.Clear();
    CurFrameData->DirectionalLights.Clear();
    CurFrameData->Lights.Clear();
    CurFrameData->ShadowCascadePoolSize = 0;
    CurFrameData->DbgVertices.Clear();
    CurFrameData->DbgIndices.Clear();
    CurFrameData->DbgCmds.Clear();

    DebugDraw.Reset();

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }

    Stat.FrontendTime = GRuntime.SysMilliseconds() - Stat.FrontendTime;
}

void FRenderFrontend::RenderView( int _Index ) {
    FViewport const * viewport = Viewports[ _Index ];
    FPlayerController * controller = viewport->PlayerController;
    FCameraComponent * camera = controller->GetViewCamera();
    FRenderingParameters * RP = controller->GetRenderingParameters();

    World = camera->GetWorld();

    RV = &CurFrameData->RenderViews[_Index];

    RV->GameRunningTimeSeconds = World->GetRunningTimeMicro() * 0.000001;
    RV->GameplayTimeSeconds = World->GetGameplayTimeMicro() * 0.000001;

    RV->ViewIndex = _Index;
    RV->Width = viewport->Width;
    RV->Height = viewport->Height;
    RV->ViewPosition = camera->GetWorldPosition();
    RV->ViewRotation = camera->GetWorldRotation();
    RV->ViewRightVec = camera->GetWorldRightVector();
    RV->ViewUpVec = camera->GetWorldUpVector();
    RV->ViewDir = camera->GetWorldForwardVector();
    RV->ViewMatrix = camera->GetViewMatrix();
    RV->ViewZNear = camera->GetZNear();
    RV->ViewZFar = camera->GetZFar();
    RV->ViewOrthoMins = camera->GetOrthoMins();
    RV->ViewOrthoMaxs = camera->GetOrthoMaxs();
    camera->GetEffectiveFov( RV->ViewFovX, RV->ViewFovY );
    RV->bPerspective = camera->IsPerspective();
    RV->MaxVisibleDistance = camera->GetZFar(); // TODO: расчитать дальность до самой дальней точки на экране (по баундингам static&skinned mesh)
    RV->NormalToViewMatrix = Float3x3( RV->ViewMatrix );
    RV->ProjectionMatrix = camera->GetProjectionMatrix();
    RV->InverseProjectionMatrix = camera->IsPerspective() ?
                RV->ProjectionMatrix.PerspectiveProjectionInverseFast()
              : RV->ProjectionMatrix.OrthoProjectionInverseFast();
    RV->ModelviewProjection = RV->ProjectionMatrix * RV->ViewMatrix;
    RV->ViewSpaceToWorldSpace = RV->ViewMatrix.Inversed();
    RV->ClipSpaceToWorldSpace = RV->ViewSpaceToWorldSpace * RV->InverseProjectionMatrix;
    camera->MakeClusterProjectionMatrix( RV->ClusterProjectionMatrix );
    RV->BackgroundColor = RP ? RP->BackgroundColor.GetRGB() : Float3(1.0f);
    RV->bClearBackground = RP ? RP->bClearBackground : true;
    RV->bWireframe = RP ? RP->bWireframe : false;
    RV->FirstInstance = CurFrameData->Instances.Size();
    RV->InstanceCount = 0;
    RV->FirstShadowInstance = CurFrameData->ShadowInstances.Size();
    RV->ShadowInstanceCount = 0;
    RV->FirstDirectionalLight = CurFrameData->DirectionalLights.Size();
    RV->NumDirectionalLights = 0;
    RV->FirstLight = CurFrameData->Lights.Size();
    RV->NumLights = 0;

    VisMarker++;

    if ( RP && RP->bDrawDebug ) {
        World->DrawDebug( &DebugDraw, CurFrameData->FrameNumber );
        RV->FirstDbgCmd = World->GetFirstDebugDrawCommand();
        RV->DbgCmdCount = World->GetDebugDrawCommandCount();
    } else {
        RV->FirstDbgCmd = 0;
        RV->DbgCmdCount = 0;
    }

    // TODO: call this once per frame!
    controller->VisitViewActors();

    FRenderFrontendDef def;

    def.View = RV;
    def.Frustum = &camera->GetFrustum();
    def.RenderingMask = RP ? RP->RenderingMask : ~0;
    def.VisMarker = VisMarker;
    def.PolyCount = 0;
    def.ShadowMapPolyCount = 0;

    World->RenderFrontend_AddInstances( &def );
    World->RenderFrontend_AddDirectionalShadowmapInstances( &def );

    Stat.PolyCount += def.PolyCount;
    Stat.ShadowMapPolyCount += def.ShadowMapPolyCount;

    //int64_t t = GRuntime.SysMilliseconds();
    StdSort( CurFrameData->Instances.Begin() + RV->FirstInstance,
             CurFrameData->Instances.Begin() + ( RV->FirstInstance + RV->InstanceCount ),
             InstanceSortFunction );

    StdSort( CurFrameData->ShadowInstances.Begin() + RV->FirstShadowInstance,
             CurFrameData->ShadowInstances.Begin() + (RV->FirstShadowInstance + RV->ShadowInstanceCount),
             ShadowInstanceSortFunction );

    //GLogger.Printf( "Sort instances time %d instances count %d\n", GRuntime.SysMilliseconds() - t, RV->InstanceCount );
}

void FRenderFrontend::RenderCanvas( FCanvas * _Canvas ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = &_Canvas->GetDrawList();

    if ( srcList->VtxBuffer.empty() ) {
        return;
    }

    FHUDDrawList * drawList = ( FHUDDrawList * )GRuntime.AllocFrameMem( sizeof( FHUDDrawList ) );
    if ( !drawList ) {
        return;
    }

    drawList->VerticesCount = srcList->VtxBuffer.size();
    drawList->IndicesCount = srcList->IdxBuffer.size();
    drawList->CommandsCount = srcList->CmdBuffer.size();

    int bytesCount = sizeof( FHUDDrawVert ) * drawList->VerticesCount;
    drawList->Vertices = ( FHUDDrawVert * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Vertices ) {
        return;
    }
    memcpy( drawList->Vertices, srcList->VtxBuffer.Data, bytesCount );

    bytesCount = sizeof( unsigned short ) * drawList->IndicesCount;
    drawList->Indices = ( unsigned short * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Indices ) {
        return;
    }

    memcpy( drawList->Indices, srcList->IdxBuffer.Data, bytesCount );

    bytesCount = sizeof( FHUDDrawCmd ) * drawList->CommandsCount;
    drawList->Commands = ( FHUDDrawCmd * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Commands ) {
        return;
    }

    int firstIndex = 0;

    FHUDDrawCmd * dstCmd = drawList->Commands;
    for ( const ImDrawCmd * pCmd = srcList->CmdBuffer.begin() ; pCmd != srcList->CmdBuffer.end() ; pCmd++ ) {

        memcpy( &dstCmd->ClipMins, &pCmd->ClipRect, sizeof( Float4 ) );
        dstCmd->IndexCount = pCmd->ElemCount;
        dstCmd->StartIndexLocation = firstIndex;
        dstCmd->Type = (EHUDDrawCmd)( pCmd->BlendingState & 0xff );
        dstCmd->Blending = (EColorBlending)( ( pCmd->BlendingState >> 8 ) & 0xff );
        dstCmd->SamplerType = (EHUDSamplerType)( ( pCmd->BlendingState >> 16 ) & 0xff );

        firstIndex += pCmd->ElemCount;

        AN_Assert( pCmd->TextureId );

        switch ( dstCmd->Type ) {
        case HUD_DRAW_CMD_VIEWPORT:
        {
            if ( NumViewports >= MAX_RENDER_VIEWS ) {
                GLogger.Printf( "FRenderFrontend: MAX_RENDER_VIEWS hit\n" );
                drawList->CommandsCount--;
                continue;
            }

            FViewport const * viewport = &_Canvas->GetViewports()[ (size_t)pCmd->TextureId - 1 ];

            dstCmd->ViewportIndex = NumViewports++;

            Viewports[ dstCmd->ViewportIndex ] = viewport;

            MaxViewportWidth = FMath::Max( MaxViewportWidth, viewport->Width );
            MaxViewportHeight = FMath::Max( MaxViewportHeight, viewport->Height );

            dstCmd++;

            break;
        }

        case HUD_DRAW_CMD_MATERIAL:
        {
            FMaterialInstance * materialInstance = static_cast< FMaterialInstance * >( pCmd->TextureId );
            AN_Assert( materialInstance );

            FMaterial * material = materialInstance->GetMaterial();
            AN_Assert( material );

            if ( material->GetType() != MATERIAL_TYPE_HUD ) {
                drawList->CommandsCount--;
                continue;
            }

            dstCmd->MaterialFrameData = materialInstance->RenderFrontend_Update( VisMarker );

            AN_Assert( dstCmd->MaterialFrameData );

            dstCmd++;

            break;
        }
        case HUD_DRAW_CMD_TEXTURE:
        case HUD_DRAW_CMD_ALPHA:
        {
            dstCmd->Texture = (FTextureGPU *)pCmd->TextureId;
            dstCmd++;
            break;
        }
        default:
            AN_Assert( 0 );
            break;
        }
    }

    FHUDDrawList * prev = frameData->DrawListTail;
    drawList->pNext = nullptr;
    frameData->DrawListTail = drawList;
    if ( prev ) {
        prev->pNext = drawList;
    } else {
        frameData->DrawListHead = drawList;
    }
}

void FRenderFrontend::RenderImgui() {
    ImDrawData * drawData = ImGui::GetDrawData();
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

                Float2 pos = GEngine.GetCursorPosition();
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
            RenderImgui( drawData->CmdLists[ n ] );
        }
    }
}

void FRenderFrontend::RenderImgui( ImDrawList const * _DrawList ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = _DrawList;

    if ( srcList->VtxBuffer.empty() ) {
        return;
    }

    FHUDDrawList * drawList = ( FHUDDrawList * )GRuntime.AllocFrameMem( sizeof( FHUDDrawList ) );
    if ( !drawList ) {
        return;
    }

    drawList->VerticesCount = srcList->VtxBuffer.size();
    drawList->IndicesCount = srcList->IdxBuffer.size();
    drawList->CommandsCount = srcList->CmdBuffer.size();

    int bytesCount = sizeof( FHUDDrawVert ) * drawList->VerticesCount;
    drawList->Vertices = ( FHUDDrawVert * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Vertices ) {
        return;
    }
    memcpy( drawList->Vertices, srcList->VtxBuffer.Data, bytesCount );

    bytesCount = sizeof( unsigned short ) * drawList->IndicesCount;
    drawList->Indices = ( unsigned short * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Indices ) {
        return;
    }

    memcpy( drawList->Indices, srcList->IdxBuffer.Data, bytesCount );

    bytesCount = sizeof( FHUDDrawCmd ) * drawList->CommandsCount;
    drawList->Commands = ( FHUDDrawCmd * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Commands ) {
        return;
    }

    int firstIndex = 0;

    FHUDDrawCmd * dstCmd = drawList->Commands;
    for ( const ImDrawCmd * pCmd = srcList->CmdBuffer.begin() ; pCmd != srcList->CmdBuffer.end() ; pCmd++ ) {

        memcpy( &dstCmd->ClipMins, &pCmd->ClipRect, sizeof( Float4 ) );
        dstCmd->IndexCount = pCmd->ElemCount;
        dstCmd->StartIndexLocation = firstIndex;
        dstCmd->Type = (EHUDDrawCmd)( pCmd->BlendingState & 0xff );
        dstCmd->Blending = (EColorBlending)( ( pCmd->BlendingState >> 8 ) & 0xff );
        dstCmd->SamplerType = (EHUDSamplerType)( ( pCmd->BlendingState >> 16 ) & 0xff );

        firstIndex += pCmd->ElemCount;

        AN_Assert( pCmd->TextureId );

        switch ( dstCmd->Type ) {
        case HUD_DRAW_CMD_VIEWPORT:
        {
            drawList->CommandsCount--;
            continue;
        }

        case HUD_DRAW_CMD_MATERIAL:
        {
            FMaterialInstance * materialInstance = static_cast< FMaterialInstance * >( pCmd->TextureId );
            AN_Assert( materialInstance );

            FMaterial * material = materialInstance->GetMaterial();
            AN_Assert( material );

            if ( material->GetType() != MATERIAL_TYPE_HUD ) {
                drawList->CommandsCount--;
                continue;
            }

            dstCmd->MaterialFrameData = materialInstance->RenderFrontend_Update( VisMarker );
            AN_Assert( dstCmd->MaterialFrameData );

            dstCmd++;

            break;
        }
        case HUD_DRAW_CMD_TEXTURE:
        case HUD_DRAW_CMD_ALPHA:
        {
            dstCmd->Texture = (FTextureGPU *)pCmd->TextureId;
            dstCmd++;
            break;
        }
        default:
            AN_Assert( 0 );
            break;
        }
    }

    FHUDDrawList * prev = frameData->DrawListTail;
    drawList->pNext = nullptr;
    frameData->DrawListTail = drawList;
    if ( prev ) {
        prev->pNext = drawList;
    } else {
        frameData->DrawListHead = drawList;
    }
}
