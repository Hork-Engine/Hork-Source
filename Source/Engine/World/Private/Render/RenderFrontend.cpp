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

#include <World/Public/Render/RenderFrontend.h>
#include <World/Public/World.h>
#include <World/Public/Components/CameraComponent.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/DirectionalLightComponent.h>
#include <World/Public/Components/PointLightComponent.h>
#include <World/Public/Components/SpotLightComponent.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Widgets/WDesktop.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/ScopedTimeCheck.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

#include "VSD.h"
#include "LightVoxelizer.h"

constexpr int MAX_SURFACE_VERTS = 32768*16;
constexpr int MAX_SURFACE_INDICES = 32768*16;

ARuntimeVariable RVFixFrustumClusters( _CTS( "FixFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );

ARenderFrontend & GRenderFrontend = ARenderFrontend::Inst();

ARenderFrontend::ARenderFrontend() {
}

struct SInstanceSortFunction {
    bool operator() ( SRenderInstance const * _A, SRenderInstance * _B ) {
        return _A->SortKey < _B->SortKey;
    }
} InstanceSortFunction;

struct SShadowInstanceSortFunction {
    bool operator() ( SShadowRenderInstance const * _A, SShadowRenderInstance * _B ) {
        return _A->SortKey < _B->SortKey;
    }
} ShadowInstanceSortFunction;

void ARenderFrontend::Initialize() {
    VSD_Initialize();

    BatchMesh = CreateInstanceOf< AIndexedMesh >();
    BatchMesh->Initialize( MAX_SURFACE_VERTS, MAX_SURFACE_INDICES, 1, false, true );
    BatchLightmapUV = BatchMesh->CreateLightmapUVChannel();
    BatchVertexLight = BatchMesh->CreateVertexLightChannel();
}

void ARenderFrontend::Deinitialize() {
    VSD_Deinitialize();

    PointLights.Free();
    VisPrimitives.Free();
    VisSurfaces.Free();

    BatchMesh.Reset();
    BatchLightmapUV.Reset();
    BatchVertexLight.Reset();
}

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
    //FrameData->Lights.Clear();
    FrameData->ShadowCascadePoolSize = 0;

    DebugDraw.Reset();

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }

    //int64_t t = GRuntime.SysMilliseconds();
    for ( int i = 0 ; i < NumViewports ; i++ ) {
        SRenderView * view = &FrameData->RenderViews[i];

        StdSort( FrameData->Instances.Begin() + view->FirstInstance,
                 FrameData->Instances.Begin() + ( view->FirstInstance + view->InstanceCount ),
                 InstanceSortFunction );

        StdSort( FrameData->ShadowInstances.Begin() + view->FirstShadowInstance,
                 FrameData->ShadowInstances.Begin() + (view->FirstShadowInstance + view->ShadowInstanceCount),
                 ShadowInstanceSortFunction );

    }
    //GLogger.Printf( "Sort instances time %d instances count %d\n", GRuntime.SysMilliseconds() - t, FrameData->Instances.Size() + FrameData->ShadowInstances.Size() );

    Stat.FrontendTime = GRuntime.SysMilliseconds() - Stat.FrontendTime;

    FrameNumber++;
}

void ARenderFrontend::RenderView( int _Index ) {
    SViewport const * viewport = Viewports[ _Index ];
    APlayerController * controller = viewport->PlayerController;
    ARenderingParameters * RP = controller->GetRenderingParameters();
    AWorld * world = controller->GetWorld();
    SRenderView * view = &FrameData->RenderViews[_Index];

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
    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;
    view->FirstInstance = FrameData->Instances.Size();
    view->InstanceCount = 0;
    view->FirstShadowInstance = FrameData->ShadowInstances.Size();
    view->ShadowInstanceCount = 0;
    view->FirstDirectionalLight = FrameData->DirectionalLights.Size();
    view->NumDirectionalLights = 0;
    //view->FirstLight = FrameData->Lights.Size();
    //view->NumLights = 0;
    view->FirstDebugDrawCommand = 0;
    view->DebugDrawCommandCount = 0;

    if ( camera )
    {
        world->E_OnPrepareRenderFrontend.Dispatch( camera, FrameNumber );

        SRenderFrontendDef def;
        def.View = view;
        def.Frustum = &camera->GetFrustum();
        def.VisibilityMask = RP ? RP->VisibilityMask : ~0;
        def.PolyCount = 0;
        def.ShadowMapPolyCount = 0;

        ARenderWorld & renderWorld = world->GetRenderWorld();

        AddLevelInstances( &renderWorld, &def );

        {
            //AScopedTimeCheck TimeCheck( "CreateDirectionalLightCascades" );

            CreateDirectionalLightCascades( FrameData, view );

            AddDirectionalShadowmapInstances( &renderWorld, &def );
        }




        Stat.PolyCount += def.PolyCount;
        Stat.ShadowMapPolyCount += def.ShadowMapPolyCount;

        // Generate debug draw commands
        if ( RP && RP->bDrawDebug )
        {
            AScopedTimeCheck TimeCheck( "DebugDraw" );
            DebugDraw.BeginRenderView( view, VisPass );
            world->DrawDebug( &DebugDraw );
            DebugDraw.EndRenderView();
        }
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
            dstCmd->MaterialFrameData = materialInstance->RenderFrontend_Update( FrameNumber );

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

            dstCmd->MaterialFrameData = materialInstance->RenderFrontend_Update( FrameNumber );
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

void ARenderFrontend::AddLevelInstances( ARenderWorld * InWorld, SRenderFrontendDef * RenderDef )
{
    SRenderFrame * frameData = GRuntime.GetFrameData();
    SRenderView * view = RenderDef->View;

#if 0
    // Add spot lights
    for ( ASpotLightComponent * light = InWorld->GetSpotLights() ; light ; light = light->GetNext() ) {

        if ( view->NumLights > MAX_LIGHTS ) {
            GLogger.Printf( "MAX_LIGHTS hit\n" );
            break;
        }

        if ( !light->IsEnabled() ) {
            continue;
        }

        // TODO: cull light

        SLightDef * lightDef = (SLightDef *)GRuntime.AllocFrameMem( sizeof( SLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->Lights.Append( lightDef );

        lightDef->bSpot = true;
        lightDef->BoundingBox = light->GetWorldBounds();
        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Position = light->GetWorldPosition();
        lightDef->RenderMask = light->RenderingGroup;
        lightDef->InnerRadius = light->GetInnerRadius();
        lightDef->OuterRadius = light->GetOuterRadius();
        lightDef->InnerConeAngle = light->GetInnerConeAngle();
        lightDef->OuterConeAngle = light->GetOuterConeAngle();
        lightDef->SpotDirection = light->GetWorldDirection();
        lightDef->SpotExponent = light->GetSpotExponent();
        lightDef->OBBTransformInverse = light->GetOBBTransformInverse();

        view->NumLights++;
    }
#endif



    {
        AScopedTimeCheck TimeCheck( "VSD_QueryVisiblePrimitives & AddDrawable" );

        SVisibilityQuery query;

        for ( int i = 0 ; i < 6 ; i++ ) {
            query.FrustumPlanes[i] = &(*RenderDef->Frustum)[i];
        }
        query.ViewPosition = RenderDef->View->ViewPosition;
        query.ViewRightVec = RenderDef->View->ViewRightVec;
        query.ViewUpVec = RenderDef->View->ViewUpVec;
        query.VisibilityMask = RenderDef->VisibilityMask;
        query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;// | VSD_QUERY_MASK_SHADOW_CAST;

        VSD_QueryVisiblePrimitives( InWorld->GetOwnerWorld(), VisPrimitives, VisSurfaces, &VisPass, query );

        AMeshComponent * mesh;
        ABrushComponent * brush;
        APointLightComponent * pointLight;

        PointLights.Clear();

        for ( SPrimitiveDef * primitive : VisPrimitives ) {

            // TODO: Replace upcasting by something better (virtual function?)

            //primitive->Owner->OnBecameVisible();

            if ( nullptr != (mesh = Upcast< AMeshComponent >( primitive->Owner )) )
            {
                if ( mesh->HasPreRenderUpdate() ) {
                    mesh->OnPreRenderUpdate( RenderDef );
                }

                AddMesh( RenderDef, mesh );
            }
            else if ( nullptr != (brush = Upcast< ABrushComponent >( primitive->Owner )) )
            {
                if ( brush->HasPreRenderUpdate() ) {
                    brush->OnPreRenderUpdate( RenderDef );
                }

                ABrushModel * model = brush->GetModel();

                for ( int i = 0 ; i < brush->NumSurfaces ; i++ ) {

                    // TODO: Cull surface?

                    SSurfaceDef * surf = &model->Surfaces[brush->FirstSurface + i];

                    surf->VisPass = VisPass;

                    VisSurfaces.Append( surf );
                }
            }
            else if ( nullptr != (pointLight = Upcast< APointLightComponent >( primitive->Owner )) )
            {
                PointLights.Append( pointLight );
            }
            else
            {
                GLogger.Printf( "Unknown drawable class\n" );
            }

            // TODO: Add other drawables (Sprite, ...)
        }

        if ( !VisSurfaces.IsEmpty() ) {
            struct SSortFunction {
                bool operator() ( SSurfaceDef const * _A, SSurfaceDef const * _B ) {
                    return ( _A->SortKey < _B->SortKey );
                }
            } SortFunction;

            StdSort( VisSurfaces.ToPtr(), VisSurfaces.ToPtr() + VisSurfaces.Size(), SortFunction );

            numVerts = 0;
            numIndices = 0;

            AddSurfaces( RenderDef, VisSurfaces.ToPtr(), VisSurfaces.Size() );

            AN_ASSERT( numVerts <= BatchMesh->GetVertexCount() );
            AN_ASSERT( numIndices <= BatchMesh->GetIndexCount() );

            if ( numVerts > 0 ) {
                BatchMesh->SendVertexDataToGPU( numVerts, 0 );
                BatchMesh->SendIndexDataToGPU( numIndices, 0 );
                BatchLightmapUV->SendVertexDataToGPU( numVerts, 0 );
                BatchVertexLight->SendVertexDataToGPU( numVerts, 0 );
            }
        }
    }

    // Add directional lights
    for ( ADirectionalLightComponent * light = InWorld->GetDirectionalLights() ; light ; light = light->GetNext() ) {

        if ( view->NumDirectionalLights > MAX_DIRECTIONAL_LIGHTS ) {
            GLogger.Printf( "MAX_DIRECTIONAL_LIGHTS hit\n" );
            break;
        }

        if ( !light->IsEnabled() ) {
            continue;
        }

        SDirectionalLightDef * lightDef = (SDirectionalLightDef *)GRuntime.AllocFrameMem( sizeof( SDirectionalLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->DirectionalLights.Append( lightDef );

        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Matrix = light->GetWorldRotation().ToMatrix();
        lightDef->MaxShadowCascades = light->GetMaxShadowCascades();
        lightDef->RenderMask = ~0;//light->RenderingGroup;
        lightDef->NumCascades = 0;  // this will be calculated later
        lightDef->FirstCascade = 0; // this will be calculated later
        lightDef->bCastShadow = light->bCastShadow;

        view->NumDirectionalLights++;
    }

    //GLogger.Printf( "FrameLightData %f KB\n", sizeof( SFrameLightData ) / 1024.0f );
    if ( !RVFixFrustumClusters ) {
        GLightVoxelizer.Voxelize( frameData, view, PointLights.ToPtr(), PointLights.Size() );
    }
}

void ARenderFrontend::AddDirectionalShadowmapInstances( ARenderWorld * InWorld, SRenderFrontendDef * RenderDef ) {
    SRenderFrame * frameData = GRuntime.GetFrameData();

    if ( !RenderDef->View->NumShadowMapCascades ) {
        return;
    }

    // TODO: Add shadow surfaces

    // Create shadow instances

    for ( AMeshComponent * component = InWorld->GetShadowCasters() ; component ; component = component->GetNextShadowCaster() ) {

        // TODO: Perform culling for each shadow cascade, set CascadeMask

        if ( (component->GetVisibilityGroup() & RenderDef->VisibilityMask) == 0 ) {
            continue;
        }

        AIndexedMesh * mesh = component->GetMesh();

        size_t skeletonOffset = 0;
        size_t skeletonSize = 0;
        if ( mesh->IsSkinned() && component->IsSkinnedMesh() ) {
            ASkinnedComponent * skeleton = static_cast< ASkinnedComponent * >(component);
            skeleton->UpdateJointTransforms( skeletonOffset, skeletonSize, frameData->FrameNumber );
        }

        Float3x4 const & instanceMatrix = component->GetWorldTransformMatrix();

        AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

        for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

            // FIXME: check subpart bounding box here

            AIndexedMeshSubpart * subpart = subparts[subpartIndex];

            AMaterialInstance * materialInstance = component->GetMaterialInstance( subpartIndex );
            AN_ASSERT( materialInstance );

            AMaterial * material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if ( material->GetGPUResource()->bNoCastShadow ) {
                continue;
            }

            SMaterialFrameData * materialInstanceFrameData = materialInstance->RenderFrontend_Update( FrameNumber );

            // Add render instance
            SShadowRenderInstance * instance = (SShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( SShadowRenderInstance ) );
            if ( !instance ) {
                break;
            }

            frameData->ShadowInstances.Append( instance );

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;
            instance->VertexBuffer = mesh->GetVertexBufferGPU();
            instance->IndexBuffer = mesh->GetIndexBufferGPU();
            instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

            if ( component->bUseDynamicRange ) {
                instance->IndexCount = component->DynamicRangeIndexCount;
                instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
                instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
            } else {
                instance->IndexCount = subpart->GetIndexCount();
                instance->StartIndexLocation = subpart->GetFirstIndex();
                instance->BaseVertexLocation = subpart->GetBaseVertex() + component->SubpartBaseVertexOffset;
            }

            instance->SkeletonOffset = skeletonOffset;
            instance->SkeletonSize = skeletonSize;
            instance->WorldTransformMatrix = instanceMatrix;
            instance->CascadeMask = 0xffff; // TODO: Calculate!!!

            // Generate sort key.
            // NOTE: 8 bits are still unused. We can use it in future.
            instance->SortKey =   ((uint64_t)(component->RenderingOrder & 0xffu) << 56u)
                                | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
                                | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
                                | ((uint64_t)(Core::PHHash64( (uint64_t)instance->VertexBuffer ) & 0xffffu) << 8u);

            RenderDef->View->ShadowInstanceCount++;

            RenderDef->ShadowMapPolyCount += instance->IndexCount / 3;

            if ( component->bUseDynamicRange ) {
                // If component uses dynamic range, mesh has actually one subpart
                break;
            }
        }
    }
}

//void ARenderFrontend::AddPointLight( APointLightComponent * InLight, SRenderFrontendDef * RenderDef ) {

//    if ( RenderDef->View->NumLights > MAX_LIGHTS ) {
//        GLogger.Printf( "MAX_LIGHTS hit\n" );
//        return;
//    }

//    SLightDef * lightDef = (SLightDef *)GRuntime.AllocFrameMem( sizeof( SLightDef ) );
//    if ( !lightDef ) {
//        return;
//    }

//    GRuntime.GetFrameData()->Lights.Append( lightDef );

//    lightDef->bSpot = false;
//    lightDef->BoundingBox = InLight->GetWorldBounds();
//    lightDef->ColorAndAmbientIntensity = InLight->GetEffectiveColor();
//    lightDef->Position = InLight->GetWorldPosition();
//    lightDef->RenderMask = InLight->RenderingGroup;
//    lightDef->InnerRadius = InLight->GetInnerRadius();
//    lightDef->OuterRadius = InLight->GetOuterRadius();
//    lightDef->OBBTransformInverse = InLight->GetOBBTransformInverse();

//    RenderDef->View->NumLights++;
//}

#ifdef FUTURE
void AddEnvCapture( AEnvCaptureComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    if ( component->RenderMark == VisMarker ) {
        return;
    }

    if ( ( component->RenderingGroup & RP->RenderingLayers ) == 0 ) {
        component->RenderMark = VisMarker;
        return;
    }

    switch ( component->GetShapeType() ) {
        case AEnvCaptureComponent::SHAPE_BOX:
        {
            // Check OBB ?
            BvAxisAlignedBox const & bounds = component->GetAABBWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByEnvCaptureBounds++;
                return;
            }
            break;
        }
        case AEnvCaptureComponent::SHAPE_SPHERE:
        {
            BvSphereSSE const & bounds = component->GetSphereWorldBounds();
            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                Dbg_CulledByEnvCaptureBounds++;
                return;
            }
            break;
        }
        case AEnvCaptureComponent::SHAPE_GLOBAL:
        {
            break;
        }
    }

    component->RenderMark = VisMarker;

    if ( component->bPreRenderUpdate ) {
        component->OnPreRenderUpdate( Camera );
    }

    // TODO: add to render view

    RV->EnvCaptureCount++;
}
#endif

AN_FORCEINLINE bool CanMergeSurfaces( SSurfaceDef const * InFirst, SSurfaceDef const * InSecond ) {
    return (    InFirst->Model->Id == InSecond->Model->Id
             && InFirst->LightmapBlock == InSecond->LightmapBlock
             && InFirst->MaterialIndex == InSecond->MaterialIndex
             && InFirst->RenderingOrder == InSecond->RenderingOrder );
}

void ARenderFrontend::AddSurfaces( SRenderFrontendDef * RenderDef, SSurfaceDef * const * Surfaces, int SurfaceCount ) {
    int batchFirstIndex = numIndices;

    AIndexedMesh * batchMesh = BatchMesh;
    ALightmapUV * batchLightmapUV = BatchLightmapUV;
    AVertexLight * batchVertexLight = BatchVertexLight;

    SMeshVertex * dstVerts = batchMesh->GetVertices();
    SMeshVertexUV * dstLM = batchLightmapUV->GetVertices();
    SMeshVertexLight * dstVL = batchVertexLight->GetVertices();
    unsigned int * dstIndices = batchMesh->GetIndices() + numIndices;

    if ( !SurfaceCount ) {
        return;
    }

    SSurfaceDef const * merge = Surfaces[0];
    ABrushModel const * model = merge->Model;

    for ( int i = 0 ; i < SurfaceCount ; i++ ) {
        SSurfaceDef const * surfDef = Surfaces[i];

        //UpdateSurfaceLight( surfDef );

        if ( !CanMergeSurfaces( merge, surfDef ) ) {

            // Flush merged surfaces
            AddSurface( RenderDef,
                        model->ParentLevel,
                        model->SurfaceMaterials[merge->MaterialIndex],
                        merge->LightmapBlock,
                        numIndices - batchFirstIndex,
                        batchFirstIndex,
                        merge->RenderingOrder );

            merge = surfDef;
            model = merge->Model;
            batchFirstIndex = numIndices;
        }

        SMeshVertex const * srcVerts = model->Vertices.ToPtr() + surfDef->FirstVertex;
        SMeshVertexUV const * srcLM = model->LightmapVerts.ToPtr() + surfDef->FirstVertex;
        SMeshVertexLight const * srcVL = model->VertexLight.ToPtr() + surfDef->FirstVertex;
        unsigned int const * srcIndices = model->Indices.ToPtr() + surfDef->FirstIndex;

        // NOTE: Here we can do a beautiful things, e.g. CPU subdivision for bezier surfaces, texcoord and vertex deformation

        AN_ASSERT( surfDef->FirstVertex + surfDef->NumVertices <= model->VertexLight.Size() );
        AN_ASSERT( surfDef->FirstIndex + surfDef->NumIndices <= model->Indices.Size() );

        {
            memcpy( dstVerts + numVerts, srcVerts, sizeof( SMeshVertex ) * surfDef->NumVertices );
            memcpy( dstLM    + numVerts, srcLM   , sizeof( SMeshVertexUV ) * surfDef->NumVertices );
            memcpy( dstVL    + numVerts, srcVL   , sizeof( SMeshVertexLight ) * surfDef->NumVertices );

            for ( int ind = 0 ; ind < surfDef->NumIndices ; ind++ ) {
                *dstIndices++ = numVerts + srcIndices[ind];
            }

            numVerts += surfDef->NumVertices;
            numIndices += surfDef->NumIndices;
        }
    }

    // Flush merged surfaces
    AddSurface( RenderDef,
                model->ParentLevel,
                model->SurfaceMaterials[merge->MaterialIndex],
                merge->LightmapBlock,
                numIndices - batchFirstIndex,
                batchFirstIndex,
                merge->RenderingOrder );
}

void ARenderFrontend::AddSurface( SRenderFrontendDef * RenderDef, ALevel * Level, AMaterialInstance * MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex, int _RenderingOrder ) {
    AIndexedMesh * mesh = BatchMesh;
    ALightmapUV * lightmapUVChannel = BatchLightmapUV;
    AVertexLight * vertexLightChannel = BatchVertexLight;

    AMaterial * material = MaterialInstance->GetMaterial();

    SMaterialFrameData * materialInstanceFrameData = MaterialInstance->RenderFrontend_Update( FrameNumber );

    // Add render instance
    SRenderInstance * instance = ( SRenderInstance * )GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
    if ( !instance ) {
        return;
    }

    GRuntime.GetFrameData()->Instances.Append( instance );

    instance->Material = material->GetGPUResource();
    instance->MaterialInstance = materialInstanceFrameData;
    instance->VertexBuffer = mesh->GetVertexBufferGPU();
    instance->IndexBuffer = mesh->GetIndexBufferGPU();
    instance->WeightsBuffer = mesh->GetWeightsBufferGPU();
    instance->LightmapOffset.X = 0;
    instance->LightmapOffset.Y = 0;
    instance->LightmapOffset.Z = 1;
    instance->LightmapOffset.W = 1;
    if ( _LightmapBlock >= 0 && _LightmapBlock < Level->Lightmaps.Size() ) {
        instance->Lightmap = Level->Lightmaps[ _LightmapBlock ]->GetGPUResource();
        instance->LightmapUVChannel = lightmapUVChannel->GetGPUResource();
    } else {
        instance->Lightmap = nullptr;
        instance->LightmapUVChannel = nullptr;
    }
    instance->VertexLightChannel = vertexLightChannel->GetGPUResource();
    instance->IndexCount = _NumIndices;
    instance->StartIndexLocation = _FirstIndex;
    instance->BaseVertexLocation = 0;
    instance->SkeletonOffset = 0;
    instance->SkeletonSize = 0;
    instance->Matrix = RenderDef->View->ModelviewProjection;

    if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
        instance->ModelNormalToViewSpace = RenderDef->View->NormalToViewMatrix;
    }

    // Generate sort key.
    // NOTE: 8 bits are still unused. We can use it in future.
    instance->SortKey =
            ((uint64_t)(_RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->VertexBuffer ) & 0xffffu) << 8u);

    RenderDef->View->InstanceCount++;

    RenderDef->PolyCount += instance->IndexCount / 3;
}

void ARenderFrontend::AddMesh( SRenderFrontendDef * RenderDef, AMeshComponent * Component ) {
    AIndexedMesh * mesh = Component->GetMesh();

    size_t skeletonOffset = 0;
    size_t skeletonSize = 0;
    if ( mesh->IsSkinned() && Component->IsSkinnedMesh() ) {
        ASkinnedComponent * skeleton = static_cast< ASkinnedComponent * >( Component );
        skeleton->UpdateJointTransforms( skeletonOffset, skeletonSize, FrameNumber );
    }

    Float3x4 const & componentWorldTransform = Component->GetWorldTransformMatrix();

    Float4x4 instanceMatrix = RenderDef->View->ModelviewProjection * componentWorldTransform; // TODO: optimize: parallel, sse, check if transformable

    AActor * actor = Component->GetParentActor();
    ALevel * level = actor->GetLevel();

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        AIndexedMeshSubpart * subpart = subparts[ subpartIndex ];

        AMaterialInstance * materialInstance = Component->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->RenderFrontend_Update( FrameNumber );

        // Add render instance
        SRenderInstance * instance = ( SRenderInstance * )GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
        if ( !instance ) {
            return;
        }

        GRuntime.GetFrameData()->Instances.Append( instance );

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;
        instance->VertexBuffer = mesh->GetVertexBufferGPU();
        instance->IndexBuffer = mesh->GetIndexBufferGPU();
        instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

        if ( Component->LightmapUVChannel && Component->LightmapBlock >= 0 && Component->LightmapBlock < level->Lightmaps.Size() ) {
            instance->LightmapUVChannel = Component->LightmapUVChannel->GetGPUResource();
            instance->LightmapOffset = Component->LightmapOffset;
            instance->Lightmap = level->Lightmaps[ Component->LightmapBlock ]->GetGPUResource();
        } else {
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
        }

        if ( Component->VertexLightChannel ) {
            instance->VertexLightChannel = Component->VertexLightChannel->GetGPUResource();
        } else {
            instance->VertexLightChannel = nullptr;
        }

        if ( Component->bUseDynamicRange ) {
            instance->IndexCount = Component->DynamicRangeIndexCount;
            instance->StartIndexLocation = Component->DynamicRangeStartIndexLocation;
            instance->BaseVertexLocation = Component->DynamicRangeBaseVertexLocation;
        } else {
            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex() + Component->SubpartBaseVertexOffset;
        }

        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonSize = skeletonSize;
        instance->Matrix = instanceMatrix;

        if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
            instance->ModelNormalToViewSpace = RenderDef->View->NormalToViewMatrix * Component->GetWorldRotation().ToMatrix();
        }

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey =
                ((uint64_t)(Component->RenderingOrder & 0xffu) << 56u)
              | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
              | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
              | ((uint64_t)(Core::PHHash64( (uint64_t)instance->VertexBuffer ) & 0xffffu) << 8u);

        RenderDef->View->InstanceCount++;

        RenderDef->PolyCount += instance->IndexCount / 3;

        if ( Component->bUseDynamicRange ) {
            // If component uses dynamic range, mesh has actually one subpart
            break;
        }
    }
}
