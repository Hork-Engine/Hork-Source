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
#include <Engine/GameThread/Public/GameEngine.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/Components/CameraComponent.h>
#include <Engine/World/Public/Components/SkinnedComponent.h>
#include <Engine/World/Public/Actors/PlayerController.h>
#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

FRenderFrontend & GRenderFrontend = FRenderFrontend::Inst();

extern FCanvas GCanvas;

static FViewport const * Viewports[MAX_RENDER_VIEWS];
static int NumViewports = 0;
static int MaxViewportWidth = 0;
static int MaxViewportHeight = 0;

FRenderFrontend::FRenderFrontend() {
}

void FRenderFrontend::Initialize() {
}

void FRenderFrontend::Deinitialize() {
}

void FRenderFrontend::BuildFrameData() {

    CurFrameData = GRuntime.GetFrameData();

    CurFrameData->FrameNumber = GGameEngine.GetFrameNumber();
    CurFrameData->DrawListHead = nullptr;
    CurFrameData->DrawListTail = nullptr;

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

        WDesktop * desktop = GGameEngine.GetDesktop();
        if ( desktop && desktop->IsCursorVisible() ) {
            GCanvas.Begin( const_cast< FFont * >( GGameEngine.GetDefaultFont()->GetFont( 0 ) ), GCanvas.Width, GCanvas.Height );
            desktop->DrawCursor( GCanvas );
            GCanvas.End();
            WriteDrawList( &GCanvas );
        }

#if 1
        // -------------------------------------
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
#if 0
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
#endif
            for ( int n = 0; n < drawData->CmdListsCount ; n++ ) {
                WriteDrawList( drawData->CmdLists[ n ] );
            }
        }
#endif
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

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }

    FrontendTime = GRuntime.SysMilliseconds() - FrontendTime;
}

void FRenderFrontend::AddInstances( FRenderFrontendDef * _Def ) {
    for ( FLevel * level : World->GetArrayOfLevels() ) {
        level->RenderFrontend_AddInstances( _Def );
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
    RV->ViewPostion = camera->GetWorldPosition();
    RV->ViewRotation = camera->GetWorldRotation();
    RV->ViewRightVec = camera->GetWorldRightVector();
    RV->ViewUpVec = camera->GetWorldUpVector();
    RV->ViewMatrix = camera->GetViewMatrix();
    RV->NormalToViewMatrix = Float3x3( RV->ViewMatrix );
    RV->ProjectionMatrix = camera->GetProjectionMatrix();
    RV->InverseProjectionMatrix = camera->IsPerspective() ?
                RV->ProjectionMatrix.PerspectiveProjectionInverseFast()
              : RV->ProjectionMatrix.OrthoProjectionInverseFast();
    RV->ModelviewProjection = RV->ProjectionMatrix * RV->ViewMatrix;
    RV->ViewSpaceToWorldSpace = RV->ViewMatrix.Inversed();
    RV->ClipSpaceToWorldSpace = RV->ViewSpaceToWorldSpace * RV->InverseProjectionMatrix;
    RV->BackgroundColor = RP ? RP->BackgroundColor.GetRGB() : Float3(1.0f);
    RV->bClearBackground = RP ? RP->bClearBackground : true;
    RV->bWireframe = RP ? RP->bWireframe : false;
    RV->PresentCmd = 0;
    RV->FirstInstance = CurFrameData->Instances.Size();
    RV->InstanceCount = 0;

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

    AddInstances( &def );

    PolyCount += def.PolyCount;

    //int64_t t = GRuntime.SysMilliseconds();
    StdSort( CurFrameData->Instances.Begin() + RV->FirstInstance,
             CurFrameData->Instances.Begin() + ( RV->FirstInstance + RV->InstanceCount ),
             InstanceSortFunction );
    //GLogger.Printf( "Sort instances time %d instances count %d\n", GRuntime.SysMilliseconds() - t, RV->InstanceCount );
}

void FRenderFrontend::WriteDrawList( FCanvas * _Canvas ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = &_Canvas->GetDrawList();

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

            FViewport const * viewport = &_Canvas->GetViewports()[ (size_t)pCmd->TextureId - 1 ];

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
            AN_Assert( materialInstance );

            FMaterial * material = materialInstance->GetMaterial();
            AN_Assert( material );

            if ( material->GetType() != MATERIAL_TYPE_HUD ) {
                drawList->CommandsCount--;
                continue;
            }

            dstCmd->MaterialInstance = materialInstance->RenderFrontend_Update( VisMarker );

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
            AN_Assert( materialInstance );

            FMaterial * material = materialInstance->GetMaterial();
            AN_Assert( material );

            if ( material->GetType() != MATERIAL_TYPE_HUD ) {
                drawList->CommandsCount--;
                continue;
            }

            dstCmd->MaterialInstance = materialInstance->RenderFrontend_Update( VisMarker );
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
