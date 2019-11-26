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

#include <World/Public/Render/RenderFrontend.h>
#include <World/Public/World.h>
#include <World/Public/Components/CameraComponent.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/InputComponent.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Widgets/WDesktop.h>
#include <Runtime/Public/Runtime.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

ARenderFrontend & GRenderFrontend = ARenderFrontend::Inst();

ARenderFrontend::ARenderFrontend() {
    FrameNumber = 0;
}

struct SInstanceSortFunction {
    bool operator() ( SRenderInstance const * _A, SRenderInstance * _B ) {

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

struct SShadowInstanceSortFunction {
    bool operator() ( SShadowRenderInstance const * _A, SShadowRenderInstance * _B ) {

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

void ARenderFrontend::Render( ACanvas * InCanvas ) {
    FrameData = GRuntime.GetFrameData();

    FrameData->FrameNumber = FrameNumber;
    FrameData->DrawListHead = nullptr;
    FrameData->DrawListTail = nullptr;

    Stat.FrontendTime = GRuntime.SysMilliseconds();
    Stat.PolyCount = 0;
    Stat.ShadowMapPolyCount = 0;

    MaxViewportWidth = 0;
    MaxViewportHeight = 0;
    NumViewports = 0;

    VisMarker++;

    RenderCanvas( InCanvas );

    //RenderImgui();

    FrameData->AllocSurfaceWidth = MaxViewportWidth;
    FrameData->AllocSurfaceHeight = MaxViewportHeight;
    FrameData->CanvasWidth = InCanvas->Width;
    FrameData->CanvasHeight = InCanvas->Height;
    FrameData->NumViews = NumViewports;
    FrameData->Instances.Clear();
    FrameData->ShadowInstances.Clear();
    FrameData->DirectionalLights.Clear();
    FrameData->Lights.Clear();
    FrameData->ShadowCascadePoolSize = 0;

    DebugDraw.Reset();

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }

    Stat.FrontendTime = GRuntime.SysMilliseconds() - Stat.FrontendTime;

    FrameNumber++;
}

void ARenderFrontend::RenderView( int _Index ) {
    SViewport const * viewport = Viewports[ _Index ];
    APlayerController * controller = viewport->PlayerController;
    ARenderingParameters * RP = controller->GetRenderingParameters();
    AWorld * world = controller->GetWorld();
    SRenderView * view = &FrameData->RenderViews[_Index];

    VisMarker++;

    view->GameRunningTimeSeconds = world->GetRunningTimeMicro() * 0.000001;
    view->GameplayTimeSeconds = world->GetGameplayTimeMicro() * 0.000001;
    view->ViewIndex = _Index;
    view->Width = viewport->Width;
    view->Height = viewport->Height;

    ACameraComponent * camera = nullptr;
    APawn * pawn = controller->GetPawn();
    if ( pawn )
    {
        camera = pawn->GetPawnCamera();
    }

    if ( camera )
    {
        view->ViewPosition = camera->GetWorldPosition();
        view->ViewRotation = camera->GetWorldRotation();
        view->ViewRightVec = camera->GetWorldRightVector();
        view->ViewUpVec = camera->GetWorldUpVector();
        view->ViewDir = camera->GetWorldForwardVector();
        view->ViewMatrix = camera->GetViewMatrix();
        view->ViewZNear = camera->GetZNear();
        view->ViewZFar = camera->GetZFar();
        view->ViewOrthoMins = camera->GetOrthoMins();
        view->ViewOrthoMaxs = camera->GetOrthoMaxs();
        camera->GetEffectiveFov( view->ViewFovX, view->ViewFovY );
        view->bPerspective = camera->IsPerspective();
        view->MaxVisibleDistance = camera->GetZFar(); // TODO: расчитать дальность до самой дальней точки на экране (по баундингам static&skinned mesh)
        view->NormalToViewMatrix = Float3x3( view->ViewMatrix );
        view->ProjectionMatrix = camera->GetProjectionMatrix();
        view->InverseProjectionMatrix = camera->IsPerspective() ?
                    view->ProjectionMatrix.PerspectiveProjectionInverseFast()
                  : view->ProjectionMatrix.OrthoProjectionInverseFast();
        camera->MakeClusterProjectionMatrix( view->ClusterProjectionMatrix );
    }

    view->ModelviewProjection = view->ProjectionMatrix * view->ViewMatrix;
    view->ViewSpaceToWorldSpace = view->ViewMatrix.Inversed();
    view->ClipSpaceToWorldSpace = view->ViewSpaceToWorldSpace * view->InverseProjectionMatrix;
    
    view->BackgroundColor = RP ? RP->BackgroundColor.GetRGB() : Float3(1.0f);
    view->bClearBackground = RP ? RP->bClearBackground : true;
    view->bWireframe = RP ? RP->bWireframe : false;
    view->FirstInstance = FrameData->Instances.Size();
    view->InstanceCount = 0;
    view->FirstShadowInstance = FrameData->ShadowInstances.Size();
    view->ShadowInstanceCount = 0;
    view->FirstDirectionalLight = FrameData->DirectionalLights.Size();
    view->NumDirectionalLights = 0;
    view->FirstLight = FrameData->Lights.Size();
    view->NumLights = 0;
    view->FirstDebugDrawCommand = 0;
    view->DebugDrawCommandCount = 0;

    if ( camera )
    {
        world->E_OnPrepareRenderFrontend.Dispatch( camera, VisMarker );

        // Generate debug draw commands
        if ( RP && RP->bDrawDebug )
        {
            DebugDraw.BeginRenderView( view );
            world->DrawDebug( &DebugDraw );
            DebugDraw.EndRenderView();
        }

        SRenderFrontendDef def;
        def.View = view;
        def.Frustum = &camera->GetFrustum();
        def.RenderingMask = RP ? RP->RenderingMask : ~0;
        def.VisMarker = VisMarker;
        def.PolyCount = 0;
        def.ShadowMapPolyCount = 0;

        world->GetRenderWorld().RenderFrontend_AddInstances( &def );
        world->GetRenderWorld().RenderFrontend_AddDirectionalShadowmapInstances( &def );

        Stat.PolyCount += def.PolyCount;
        Stat.ShadowMapPolyCount += def.ShadowMapPolyCount;

        //int64_t t = GRuntime.SysMilliseconds();

        StdSort( FrameData->Instances.Begin() + view->FirstInstance,
                 FrameData->Instances.Begin() + ( view->FirstInstance + view->InstanceCount ),
                 InstanceSortFunction );

        StdSort( FrameData->ShadowInstances.Begin() + view->FirstShadowInstance,
                 FrameData->ShadowInstances.Begin() + (view->FirstShadowInstance + view->ShadowInstanceCount),
                 ShadowInstanceSortFunction );

        //GLogger.Printf( "Sort instances time %d instances count %d\n", GRuntime.SysMilliseconds() - t, RV->InstanceCount );
    }
}

void ARenderFrontend::RenderCanvas( ACanvas * InCanvas ) {
    SRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = &InCanvas->GetDrawList();

    if ( srcList->VtxBuffer.empty() ) {
        return;
    }

    // Allocate draw list
    SHUDDrawList * drawList = ( SHUDDrawList * )GRuntime.AllocFrameMem( sizeof( SHUDDrawList ) );
    if ( !drawList ) {
        return;
    }

    // Copy vertex data
    drawList->VerticesCount = srcList->VtxBuffer.size();
    int bytesCount = sizeof( SHUDDrawVert ) * drawList->VerticesCount;
    drawList->Vertices = ( SHUDDrawVert * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Vertices ) {
        return;
    }
    memcpy( drawList->Vertices, srcList->VtxBuffer.Data, bytesCount );

    // Copy index data
    drawList->IndicesCount = srcList->IdxBuffer.size();
    bytesCount = sizeof( unsigned short ) * drawList->IndicesCount;
    drawList->Indices = ( unsigned short * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Indices ) {
        return;
    }
    memcpy( drawList->Indices, srcList->IdxBuffer.Data, bytesCount );

    // Allocate commands
    drawList->CommandsCount = srcList->CmdBuffer.size();
    bytesCount = sizeof( SHUDDrawCmd ) * drawList->CommandsCount;
    drawList->Commands = ( SHUDDrawCmd * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Commands ) {
        return;
    }

    int startIndexLocation = 0;

    // Parse ImDrawCmd, create HUDDrawCmd-s
    SHUDDrawCmd * dstCmd = drawList->Commands;
    for ( const ImDrawCmd * pCmd = srcList->CmdBuffer.begin() ; pCmd != srcList->CmdBuffer.end() ; pCmd++ ) {

        // Copy clip rect
        memcpy( &dstCmd->ClipMins, &pCmd->ClipRect, sizeof( Float4 ) );

        // Copy index buffer offsets
        dstCmd->IndexCount = pCmd->ElemCount;
        dstCmd->StartIndexLocation = startIndexLocation;

        // Unpack command type
        dstCmd->Type = (EHUDDrawCmd)( pCmd->BlendingState & 0xff );

        // Unpack blending type
        dstCmd->Blending = (EColorBlending)( ( pCmd->BlendingState >> 8 ) & 0xff );

        // Unpack texture sampler type
        dstCmd->SamplerType = (EHUDSamplerType)( ( pCmd->BlendingState >> 16 ) & 0xff );

        // Calc index location for next command
        startIndexLocation += pCmd->ElemCount;

        // TextureId can contain a viewport index, material instance or gpu texture
        if ( !pCmd->TextureId )
        {
            GLogger.Printf( "ARenderFrontend::RenderCanvas: invalid command (TextureId==0)\n" );

            drawList->CommandsCount--;

            continue;
        }

        switch ( dstCmd->Type )
        {
        case HUD_DRAW_CMD_VIEWPORT:
        {
            // Check MAX_RENDER_VIEWS limit
            if ( NumViewports >= MAX_RENDER_VIEWS )
            {
                GLogger.Printf( "ARenderFrontend::RenderCanvas: MAX_RENDER_VIEWS hit\n" );

                drawList->CommandsCount--;

                continue;
            }

            // Unpack viewport
            SViewport const * viewport = &InCanvas->GetViewports()[ (size_t)pCmd->TextureId - 1 ];

            // Compute viewport index in array of viewports
            dstCmd->ViewportIndex = NumViewports++;

            // Save pointer to viewport to array of viewports
            Viewports[ dstCmd->ViewportIndex ] = viewport;

            // Calc max viewport size
            MaxViewportWidth = Math::Max( MaxViewportWidth, viewport->Width );
            MaxViewportHeight = Math::Max( MaxViewportHeight, viewport->Height );

            // Switch to next cmd
            dstCmd++;

            break;
        }

        case HUD_DRAW_CMD_MATERIAL:
        {
            // Unpack material instance
            AMaterialInstance * materialInstance = static_cast< AMaterialInstance * >( pCmd->TextureId );

            // In normal case materialInstance never be null
            AN_ASSERT( materialInstance );

            // Get material
            AMaterial * material = materialInstance->GetMaterial();

            // GetMaterial never return null
            AN_ASSERT( material );

            // Check material type
            if ( material->GetType() != MATERIAL_TYPE_HUD )
            {
                GLogger.Printf( "ARenderFrontend::RenderCanvas: expected MATERIAL_TYPE_HUD\n" );

                drawList->CommandsCount--;

                continue;
            }

            // Update material frame data
            dstCmd->MaterialFrameData = materialInstance->RenderFrontend_Update( VisMarker );

            if ( !dstCmd->MaterialFrameData )
            {
                // Out of frame memory?

                drawList->CommandsCount--;

                continue;
            }

            // Switch to next cmd
            dstCmd++;

            break;
        }
        case HUD_DRAW_CMD_TEXTURE:
        case HUD_DRAW_CMD_ALPHA:
        {
            // Unpack texture
            dstCmd->Texture = (ATextureGPU *)pCmd->TextureId;

            // Switch to next cmd
            dstCmd++;
            break;
        }
        default:
            AN_ASSERT( 0 );
            break;
        }
    }

    // Add drawList to common list
    SHUDDrawList * prev = frameData->DrawListTail;
    drawList->pNext = nullptr;
    frameData->DrawListTail = drawList;
    if ( prev ) {
        prev->pNext = drawList;
    } else {
        frameData->DrawListHead = drawList;
    }
}

#if 0

void ARenderFrontend::RenderImgui() {
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
                AN_ASSERT( cursor > ImGuiMouseCursor_None && cursor < ImGuiMouseCursor_COUNT );

                const ImU32 col_shadow = IM_COL32( 0, 0, 0, 48 );
                const ImU32 col_border = IM_COL32( 0, 0, 0, 255 );          // Black
                const ImU32 col_fill = IM_COL32( 255, 255, 255, 255 );    // White

                Float2 pos = AInputComponent::GetCursorPosition();
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

void ARenderFrontend::RenderImgui( ImDrawList const * _DrawList ) {
    SRenderFrame * frameData = GRuntime.GetFrameData();

    ImDrawList const * srcList = _DrawList;

    if ( srcList->VtxBuffer.empty() ) {
        return;
    }

    SHUDDrawList * drawList = ( SHUDDrawList * )GRuntime.AllocFrameMem( sizeof( SHUDDrawList ) );
    if ( !drawList ) {
        return;
    }

    drawList->VerticesCount = srcList->VtxBuffer.size();
    drawList->IndicesCount = srcList->IdxBuffer.size();
    drawList->CommandsCount = srcList->CmdBuffer.size();

    int bytesCount = sizeof( SHUDDrawVert ) * drawList->VerticesCount;
    drawList->Vertices = ( SHUDDrawVert * )GRuntime.AllocFrameMem( bytesCount );
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

    bytesCount = sizeof( SHUDDrawCmd ) * drawList->CommandsCount;
    drawList->Commands = ( SHUDDrawCmd * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Commands ) {
        return;
    }

    int startIndexLocation = 0;

    SHUDDrawCmd * dstCmd = drawList->Commands;
    for ( const ImDrawCmd * pCmd = srcList->CmdBuffer.begin() ; pCmd != srcList->CmdBuffer.end() ; pCmd++ ) {

        memcpy( &dstCmd->ClipMins, &pCmd->ClipRect, sizeof( Float4 ) );
        dstCmd->IndexCount = pCmd->ElemCount;
        dstCmd->StartIndexLocation = startIndexLocation;
        dstCmd->Type = (EHUDDrawCmd)( pCmd->BlendingState & 0xff );
        dstCmd->Blending = (EColorBlending)( ( pCmd->BlendingState >> 8 ) & 0xff );
        dstCmd->SamplerType = (EHUDSamplerType)( ( pCmd->BlendingState >> 16 ) & 0xff );

        startIndexLocation += pCmd->ElemCount;

        AN_ASSERT( pCmd->TextureId );

        switch ( dstCmd->Type ) {
        case HUD_DRAW_CMD_VIEWPORT:
        {
            drawList->CommandsCount--;
            continue;
        }

        case HUD_DRAW_CMD_MATERIAL:
        {
            AMaterialInstance * materialInstance = static_cast< AMaterialInstance * >( pCmd->TextureId );
            AN_ASSERT( materialInstance );

            AMaterial * material = materialInstance->GetMaterial();
            AN_ASSERT( material );

            if ( material->GetType() != MATERIAL_TYPE_HUD ) {
                drawList->CommandsCount--;
                continue;
            }

            dstCmd->MaterialFrameData = materialInstance->RenderFrontend_Update( VisMarker );
            AN_ASSERT( dstCmd->MaterialFrameData );

            dstCmd++;

            break;
        }
        case HUD_DRAW_CMD_TEXTURE:
        case HUD_DRAW_CMD_ALPHA:
        {
            dstCmd->Texture = (ATextureGPU *)pCmd->TextureId;
            dstCmd++;
            break;
        }
        default:
            AN_ASSERT( 0 );
            break;
        }
    }

    SHUDDrawList * prev = frameData->DrawListTail;
    drawList->pNext = nullptr;
    frameData->DrawListTail = drawList;
    if ( prev ) {
        prev->pNext = drawList;
    } else {
        frameData->DrawListHead = drawList;
    }
}

#endif
