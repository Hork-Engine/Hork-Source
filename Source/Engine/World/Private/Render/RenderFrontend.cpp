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
#include <World/Public/Components/AnalyticLightComponent.h>
#include <World/Public/Components/IBLComponent.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/Widgets/WDesktop.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/ScopedTimeCheck.h>
#include <Runtime/Public/VertexMemoryGPU.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

#include "VSD.h"
#include "LightVoxelizer.h"

ARuntimeVariable RVFixFrustumClusters( _CTS( "FixFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVRenderView( _CTS( "RenderView" ), _CTS( "1" ), VAR_CHEAT );
ARuntimeVariable RVRenderSurfaces( _CTS( "RenderSurfaces" ), _CTS( "1" ), VAR_CHEAT );
ARuntimeVariable RVRenderMeshes( _CTS( "RenderMeshes" ), _CTS( "1" ), VAR_CHEAT );
ARuntimeVariable RVResolutionScaleX( _CTS( "ResolutionScaleX" ), _CTS( "1" ) );
ARuntimeVariable RVResolutionScaleY( _CTS( "ResolutionScaleY" ), _CTS( "1" ) );

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

    PhotometricProfiles = CreateInstanceOf< ATexture >();
    PhotometricProfiles->Initialize1DArray( TEXTURE_PF_R8, 1, 256, 256 );
}

void ARenderFrontend::Deinitialize() {
    VSD_Deinitialize();

    Lights.Free();
    IBLs.Free();
    VisPrimitives.Free();
    VisSurfaces.Free();

    DebugDraw.Free();

    FrameData.Instances.Free();
    FrameData.TranslucentInstances.Free();
    FrameData.ShadowInstances.Free();
    FrameData.DirectionalLights.Free();

    PhotometricProfiles.Reset();
}

void ARenderFrontend::Render( ACanvas * InCanvas ) {
    FrameData.FrameNumber = FrameNumber = GRuntime.SysFrameNumber();
    FrameData.DrawListHead = nullptr;
    FrameData.DrawListTail = nullptr;

    Stat.FrontendTime = GRuntime.SysMilliseconds();
    Stat.PolyCount = 0;
    Stat.ShadowMapPolyCount = 0;

    MaxViewportWidth = 1;
    MaxViewportHeight = 1;
    NumViewports = 0;

    RenderCanvas( InCanvas );

    //RenderImgui();

    FrameData.AllocSurfaceWidth = MaxViewportWidth;
    FrameData.AllocSurfaceHeight = MaxViewportHeight;
    FrameData.CanvasWidth = InCanvas->Width;
    FrameData.CanvasHeight = InCanvas->Height;
    FrameData.NumViews = NumViewports;
    FrameData.Instances.Clear();
    FrameData.TranslucentInstances.Clear();
    FrameData.ShadowInstances.Clear();
    FrameData.DirectionalLights.Clear();
    FrameData.ShadowCascadePoolSize = 0;
    FrameData.StreamBuffer = GStreamedMemoryGPU.GetBufferGPU();

    DebugDraw.Reset();

    for ( int i = 0 ; i < NumViewports ; i++ ) {
        RenderView( i );
    }

    //int64_t t = GRuntime.SysMilliseconds();
    for ( int i = 0 ; i < NumViewports ; i++ ) {
        SRenderView * view = &FrameData.RenderViews[i];

        StdSort( FrameData.Instances.Begin() + view->FirstInstance,
                 FrameData.Instances.Begin() + ( view->FirstInstance + view->InstanceCount ),
                 InstanceSortFunction );

        StdSort( FrameData.TranslucentInstances.Begin() + view->FirstTranslucentInstance,
                 FrameData.TranslucentInstances.Begin() + (view->FirstTranslucentInstance + view->TranslucentInstanceCount),
                 InstanceSortFunction );

        StdSort( FrameData.ShadowInstances.Begin() + view->FirstShadowInstance,
                 FrameData.ShadowInstances.Begin() + (view->FirstShadowInstance + view->ShadowInstanceCount),
                 ShadowInstanceSortFunction );

    }
    //GLogger.Printf( "Sort instances time %d instances count %d\n", GRuntime.SysMilliseconds() - t, FrameData.Instances.Size() + FrameData.ShadowInstances.Size() );

    if ( DebugDraw.CommandsCount() > 0 ) {
        FrameData.DbgCmds = DebugDraw.GetCmds().ToPtr();
        FrameData.DbgVertexStreamOffset = GStreamedMemoryGPU.AllocateVertex( DebugDraw.GetVertices().Size() * sizeof( SDebugVertex ), DebugDraw.GetVertices().ToPtr() );
        FrameData.DbgIndexStreamOffset = GStreamedMemoryGPU.AllocateVertex( DebugDraw.GetIndices().Size() * sizeof( unsigned short ), DebugDraw.GetIndices().ToPtr() );
    }

    Stat.FrontendTime = GRuntime.SysMilliseconds() - Stat.FrontendTime;
}

void ARenderFrontend::RenderView( int _Index ) {
    SViewport const * viewport = Viewports[ _Index ];
    ARenderingParameters * RP = viewport->RenderingParams;
    ACameraComponent * camera = viewport->Camera;
    AWorld * world = camera->GetWorld();
    SRenderView * view = &FrameData.RenderViews[_Index];

    view->GameRunningTimeSeconds = world->GetRunningTimeMicro() * 0.000001;
    view->GameplayTimeSeconds = world->GetGameplayTimeMicro() * 0.000001;
    view->GameplayTimeStep = world->IsPaused() ? 0.0f : Math::Max( GRuntime.SysFrameDuration() * 0.000001f, 0.0001f );
    view->ViewIndex = _Index;
    view->Width = Align( (size_t)(viewport->Width * RVResolutionScaleX.GetFloat()), 2 );
    view->Height = Align( (size_t)(viewport->Height * RVResolutionScaleY.GetFloat()), 2 );

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

    view->ViewProjection = view->ProjectionMatrix * view->ViewMatrix;
    view->ViewSpaceToWorldSpace = view->ViewMatrix.Inversed(); // TODO: Check with ViewInverseFast
    view->ClipSpaceToWorldSpace = view->ViewSpaceToWorldSpace * view->InverseProjectionMatrix;
    
    if ( RP )
    {
        view->BackgroundColor = RP->BackgroundColor.GetRGB();
        view->bClearBackground = RP->bClearBackground;
        view->bWireframe = RP->bWireframe;
        if ( RP->bVignetteEnabled ) {
            view->VignetteColorIntensity = RP->VignetteColorIntensity;
            view->VignetteOuterRadiusSqr = RP->VignetteOuterRadiusSqr;
            view->VignetteInnerRadiusSqr = RP->VignetteInnerRadiusSqr;
        } else {
            view->VignetteColorIntensity.W = 0;
        }

        if ( RP->IsColorGradingEnabled() ) {
            view->ColorGradingLUT = RP->GetColorGradingLUT() ? RP->GetColorGradingLUT()->GetGPUResource() : NULL;
            view->CurrentColorGradingLUT = RP->GetCurrentColorGradingLUT()->GetGPUResource();
            view->ColorGradingAdaptationSpeed = RP->GetColorGradingAdaptationSpeed();

            // Procedural color grading
            view->ColorGradingGrain = RP->GetColorGradingGrain();
            view->ColorGradingGamma = RP->GetColorGradingGamma();
            view->ColorGradingLift = RP->GetColorGradingLift();
            view->ColorGradingPresaturation = RP->GetColorGradingPresaturation();
            view->ColorGradingTemperatureScale = RP->GetColorGradingTemperatureScale();
            view->ColorGradingTemperatureStrength = RP->GetColorGradingTemperatureStrength();
            view->ColorGradingBrightnessNormalization = RP->GetColorGradingBrightnessNormalization();

        } else {
            view->ColorGradingLUT = NULL;
            view->CurrentColorGradingLUT = NULL;
            view->ColorGradingAdaptationSpeed = 0;
        }

        view->CurrentExposure = RP->GetCurrentExposure()->GetGPUResource();
    }
    else
    {
        view->BackgroundColor = Float3( 1.0f );
        view->bClearBackground = true;
        view->bWireframe = false;
        view->VignetteColorIntensity.W = 0;
        view->ColorGradingLUT = NULL;
        view->CurrentColorGradingLUT = NULL;
        view->ColorGradingAdaptationSpeed = 0;
        view->CurrentExposure = NULL;
    }

    view->PhotometricProfiles = PhotometricProfiles->GetGPUResource();

    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;
    view->FirstInstance = FrameData.Instances.Size();
    view->InstanceCount = 0;
    view->FirstTranslucentInstance = FrameData.TranslucentInstances.Size();
    view->TranslucentInstanceCount = 0;
    view->FirstShadowInstance = FrameData.ShadowInstances.Size();
    view->ShadowInstanceCount = 0;
    view->FirstDirectionalLight = FrameData.DirectionalLights.Size();
    view->NumDirectionalLights = 0;
    view->FirstDebugDrawCommand = 0;
    view->DebugDrawCommandCount = 0;

    if ( !RVRenderView ) {
        return;
    }

    if ( camera )
    {
        world->E_OnPrepareRenderFrontend.Dispatch( camera, FrameNumber );

        RenderDef.FrameNumber = FrameNumber;
        RenderDef.View = view;
        RenderDef.Frustum = &camera->GetFrustum();
        RenderDef.VisibilityMask = RP ? RP->VisibilityMask : ~0;
        RenderDef.PolyCount = 0;
        RenderDef.ShadowMapPolyCount = 0;

        ARenderWorld & renderWorld = world->GetRenderWorld();

        AddRenderInstances( &renderWorld );

        CreateDirectionalLightCascades( view );

        AddDirectionalShadowmapInstances( &renderWorld );

        Stat.PolyCount += RenderDef.PolyCount;
        Stat.ShadowMapPolyCount += RenderDef.ShadowMapPolyCount;

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
    drawList->VertexStreamOffset = GStreamedMemoryGPU.AllocateVertex( sizeof( SHUDDrawVert ) * srcList->VtxBuffer.Size, srcList->VtxBuffer.Data );
    drawList->IndexStreamOffset = GStreamedMemoryGPU.AllocateIndex( sizeof( unsigned short ) * srcList->IdxBuffer.Size, srcList->IdxBuffer.Data );

    // Allocate commands
    drawList->Commands = ( SHUDDrawCmd * )GRuntime.AllocFrameMem( sizeof( SHUDDrawCmd ) * srcList->CmdBuffer.Size );
    if ( !drawList->Commands ) {
        return;
    }

    drawList->CommandsCount = 0;

    // Parse ImDrawCmd, create HUDDrawCmd-s
    SHUDDrawCmd * dstCmd = drawList->Commands;
    for ( ImDrawCmd const & cmd : srcList->CmdBuffer )
    {
        // TextureId can contain a viewport index, material instance or gpu texture
        if ( !cmd.TextureId )
        {
            GLogger.Printf( "ARenderFrontend::RenderCanvas: invalid command (TextureId==0)\n" );
            continue;
        }

        Core::Memcpy( &dstCmd->ClipMins, &cmd.ClipRect, sizeof( Float4 ) );
        dstCmd->IndexCount = cmd.ElemCount;
        dstCmd->StartIndexLocation = cmd.IdxOffset;
        dstCmd->BaseVertexLocation = cmd.VtxOffset;
        dstCmd->Type = (EHUDDrawCmd)( cmd.BlendingState & 0xff );
        dstCmd->Blending = (EColorBlending)( ( cmd.BlendingState >> 8 ) & 0xff );
        dstCmd->SamplerType = (EHUDSamplerType)( ( cmd.BlendingState >> 16 ) & 0xff );

        switch ( dstCmd->Type ) {
        case HUD_DRAW_CMD_VIEWPORT: {
            // Check MAX_RENDER_VIEWS limit
            if ( NumViewports >= MAX_RENDER_VIEWS )
            {
                GLogger.Printf( "ARenderFrontend::RenderCanvas: MAX_RENDER_VIEWS hit\n" );
                continue;
            }

            // Unpack viewport
            SViewport const * viewport = &InCanvas->GetViewports()[ (size_t)cmd.TextureId - 1 ];

            // Compute viewport index in array of viewports
            dstCmd->ViewportIndex = NumViewports++;

            // Save pointer to viewport to array of viewports
            Viewports[ dstCmd->ViewportIndex ] = viewport;

            // Calc max viewport size
            MaxViewportWidth = Math::Max( MaxViewportWidth, viewport->Width );
            MaxViewportHeight = Math::Max( MaxViewportHeight, viewport->Height );

            break;
        }

        case HUD_DRAW_CMD_MATERIAL: {
            // Unpack material instance
            AMaterialInstance * materialInstance = static_cast< AMaterialInstance * >( cmd.TextureId );

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
                continue;
            }

            // Update material frame data
            dstCmd->MaterialFrameData = materialInstance->PreRenderUpdate( FrameNumber );

            if ( !dstCmd->MaterialFrameData )
            {
                // Out of frame memory?
                continue;
            }

            break;
        }

        case HUD_DRAW_CMD_TEXTURE:
        case HUD_DRAW_CMD_ALPHA: {
            // Unpack texture
            dstCmd->Texture = (ATextureGPU *)cmd.TextureId;
            break;
        }

        default:
            GLogger.Printf( "ARenderFrontend::RenderCanvas: unknown command type\n" );
            continue;
        }

        // Switch to next cmd
        dstCmd++;
        drawList->CommandsCount++;
    }

    // Add drawList
    SHUDDrawList * prev = FrameData.DrawListTail;
    drawList->pNext = nullptr;
    FrameData.DrawListTail = drawList;
    if ( prev ) {
        prev->pNext = drawList;
    } else {
        FrameData.DrawListHead = drawList;
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
    Core::Memcpy( drawList->Vertices, srcList->VtxBuffer.Data, bytesCount );

    bytesCount = sizeof( unsigned short ) * drawList->IndicesCount;
    drawList->Indices = ( unsigned short * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Indices ) {
        return;
    }

    Core::Memcpy( drawList->Indices, srcList->IdxBuffer.Data, bytesCount );

    bytesCount = sizeof( SHUDDrawCmd ) * drawList->CommandsCount;
    drawList->Commands = ( SHUDDrawCmd * )GRuntime.AllocFrameMem( bytesCount );
    if ( !drawList->Commands ) {
        return;
    }

    int startIndexLocation = 0;

    SHUDDrawCmd * dstCmd = drawList->Commands;
    for ( const ImDrawCmd * pCmd = srcList->CmdBuffer.begin() ; pCmd != srcList->CmdBuffer.end() ; pCmd++ ) {

        Core::Memcpy( &dstCmd->ClipMins, &pCmd->ClipRect, sizeof( Float4 ) );
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

            dstCmd->MaterialFrameData = materialInstance->PreRenderUpdate( FrameNumber );
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

    SHUDDrawList * prev = FrameData.DrawListTail;
    drawList->pNext = nullptr;
    FrameData.DrawListTail = drawList;
    if ( prev ) {
        prev->pNext = drawList;
    } else {
        FrameData.DrawListHead = drawList;
    }
}

#endif

void ARenderFrontend::QueryVisiblePrimitives( ARenderWorld * InWorld ) {
    SVisibilityQuery query;

    for ( int i = 0 ; i < 6 ; i++ ) {
        query.FrustumPlanes[i] = &(*RenderDef.Frustum)[i];
    }
    query.ViewPosition = RenderDef.View->ViewPosition;
    query.ViewRightVec = RenderDef.View->ViewRightVec;
    query.ViewUpVec = RenderDef.View->ViewUpVec;
    query.VisibilityMask = RenderDef.VisibilityMask;
    query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;// | VSD_QUERY_MASK_SHADOW_CAST;

    VSD_QueryVisiblePrimitives( InWorld->GetOwnerWorld(), VisPrimitives, VisSurfaces, &VisPass, query );
}

void ARenderFrontend::AddRenderInstances( ARenderWorld * InWorld )
{
    AScopedTimeCheck TimeCheck( "AddRenderInstances" );

    SRenderView * view = RenderDef.View;
    ADrawable * drawable;
    AAnalyticLightComponent * light;
    AIBLComponent * ibl;

    Lights.Clear();
    IBLs.Clear();

    QueryVisiblePrimitives( InWorld  );

    for ( SPrimitiveDef * primitive : VisPrimitives ) {

        // TODO: Replace upcasting by something better (virtual function?)

        if ( nullptr != (drawable = Upcast< ADrawable >( primitive->Owner )) ) {
            AddDrawable( drawable );
            continue;
        }

        if ( nullptr != (light = Upcast< AAnalyticLightComponent >( primitive->Owner )) ) {
            Lights.Append( light );

            APhotometricProfile * profile = light->GetPhotometricProfile();
            if ( profile ) {
                profile->WritePhotometricData( PhotometricProfiles, FrameNumber );
            }
            continue;
        }

        if ( nullptr != (ibl = Upcast< AIBLComponent >( primitive->Owner )) ) {
            IBLs.Append( ibl );
            continue;
        }

        GLogger.Printf( "Unhandles primitive\n" );
    }

    if ( RVRenderSurfaces && !VisSurfaces.IsEmpty() ) {
        struct SSortFunction {
            bool operator() ( SSurfaceDef const * _A, SSurfaceDef const * _B ) {
                return (_A->SortKey < _B->SortKey);
            }
        } SortFunction;

        StdSort( VisSurfaces.ToPtr(), VisSurfaces.ToPtr() + VisSurfaces.Size(), SortFunction );

        AddSurfaces( VisSurfaces.ToPtr(), VisSurfaces.Size() );
    }

    // Add directional lights
    for ( ADirectionalLightComponent * dirlight = InWorld->GetDirectionalLights() ; dirlight ; dirlight = dirlight->GetNext() ) {

        if ( view->NumDirectionalLights > MAX_DIRECTIONAL_LIGHTS ) {
            GLogger.Printf( "MAX_DIRECTIONAL_LIGHTS hit\n" );
            break;
        }

        if ( !dirlight->IsEnabled() ) {
            continue;
        }

        SDirectionalLightDef * lightDef = (SDirectionalLightDef *)GRuntime.AllocFrameMem( sizeof( SDirectionalLightDef ) );
        if ( !lightDef ) {
            break;
        }

        FrameData.DirectionalLights.Append( lightDef );

        lightDef->ColorAndAmbientIntensity = dirlight->GetEffectiveColor();
        lightDef->Matrix = dirlight->GetWorldRotation().ToMatrix();
        lightDef->MaxShadowCascades = dirlight->GetMaxShadowCascades();
        lightDef->RenderMask = ~0;//dirlight->RenderingGroup;
        lightDef->NumCascades = 0;  // this will be calculated later
        lightDef->FirstCascade = 0; // this will be calculated later
        lightDef->bCastShadow = dirlight->bCastShadow;

        view->NumDirectionalLights++;
    }

    //GLogger.Printf( "FrameLightData %f KB\n", sizeof( SFrameLightData ) / 1024.0f );
    if ( !RVFixFrustumClusters ) {
        GLightVoxelizer.Voxelize( &FrameData,
                                  view,
                                  Lights.ToPtr(),
                                  Lights.Size(),
                                  IBLs.ToPtr(),
                                  IBLs.Size() );
    }
}

void ARenderFrontend::AddDrawable( ADrawable * InComponent ) {
    switch ( InComponent->GetDrawableType() ) {
    case DRAWABLE_STATIC_MESH:
        AddStaticMesh( static_cast< AMeshComponent * >(InComponent) );
        break;
    case DRAWABLE_SKINNED_MESH:
        AddSkinnedMesh( static_cast< ASkinnedComponent * >(InComponent) );
        break;
    case DRAWABLE_PROCEDURAL_MESH:
        AddProceduralMesh( static_cast< AProceduralMeshComponent * >(InComponent) );
        break;
    default:
        break;
    }
}

void ARenderFrontend::AddStaticMesh( AMeshComponent * InComponent ) {
    AIndexedMesh * mesh = InComponent->GetMesh();

    if ( !RVRenderMeshes ) {
        return;
    }

    InComponent->PreRenderUpdate( &RenderDef );

    Float3x4 const & componentWorldTransform = InComponent->GetWorldTransformMatrix();

    Float4x4 instanceMatrix = RenderDef.View->ViewProjection * componentWorldTransform; // TODO: optimize: parallel, sse, check if transformable

    ALevel * level = InComponent->GetLevel();

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    bool bHasLightmap = InComponent->LightmapUVChannel
            && InComponent->LightmapBlock >= 0
            && InComponent->LightmapBlock < level->Lightmaps.Size();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        AIndexedMeshSubpart * subpart = subparts[subpartIndex];

        AMaterialInstance * materialInstance = InComponent->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->PreRenderUpdate( FrameNumber );

        // Add render instance
        SRenderInstance * instance = (SRenderInstance *)GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
        if ( !instance ) {
            return;
        }

        if ( material->IsTranslucent() ) {
            FrameData.TranslucentInstances.Append( instance );
            RenderDef.View->TranslucentInstanceCount++;
        } else {
            FrameData.Instances.Append( instance );
            RenderDef.View->InstanceCount++;
        }

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU( &instance->VertexBuffer, &instance->VertexBufferOffset );
        mesh->GetIndexBufferGPU( &instance->IndexBuffer, &instance->IndexBufferOffset );
        mesh->GetWeightsBufferGPU( &instance->WeightsBuffer, &instance->WeightsBufferOffset );

        if ( bHasLightmap ) {
            InComponent->LightmapUVChannel->GetVertexBufferGPU( &instance->LightmapUVChannel, &instance->LightmapUVOffset );
            instance->LightmapOffset = InComponent->LightmapOffset;
            instance->Lightmap = level->Lightmaps[InComponent->LightmapBlock]->GetGPUResource();
        } else {
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
        }

        if ( InComponent->VertexLightChannel ) {
            InComponent->VertexLightChannel->GetVertexBufferGPU( &instance->VertexLightChannel, &instance->VertexLightOffset );
        } else {
            instance->VertexLightChannel = nullptr;
        }

        instance->IndexCount = subpart->GetIndexCount();
        instance->StartIndexLocation = subpart->GetFirstIndex();
        instance->BaseVertexLocation = subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->Matrix = instanceMatrix;

        if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
            instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix * InComponent->GetWorldRotation().ToMatrix();
        }

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey =
            ((uint64_t)(InComponent->RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)mesh/*instance->VertexBuffer*/ ) & 0xffffu) << 8u);

        RenderDef.PolyCount += instance->IndexCount / 3;
    }
}

void ARenderFrontend::AddSkinnedMesh( ASkinnedComponent * InComponent ) {
    AIndexedMesh * mesh = InComponent->GetMesh();

    if ( !RVRenderMeshes ) {
        return;
    }

    InComponent->PreRenderUpdate( &RenderDef );

    size_t skeletonOffset = 0;
    size_t skeletonSize = 0;

    InComponent->GetSkeletonHandle( skeletonOffset, skeletonSize );

    Float3x4 const & componentWorldTransform = InComponent->GetWorldTransformMatrix();

    Float4x4 instanceMatrix = RenderDef.View->ViewProjection * componentWorldTransform; // TODO: optimize: parallel, sse, check if transformable

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        AIndexedMeshSubpart * subpart = subparts[subpartIndex];

        AMaterialInstance * materialInstance = InComponent->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->PreRenderUpdate( FrameNumber );

        // Add render instance
        SRenderInstance * instance = (SRenderInstance *)GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
        if ( !instance ) {
            return;
        }

        if ( material->IsTranslucent() ) {
            FrameData.TranslucentInstances.Append( instance );
            RenderDef.View->TranslucentInstanceCount++;
        } else {
            FrameData.Instances.Append( instance );
            RenderDef.View->InstanceCount++;
        }

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;
        //instance->VertexBuffer = mesh->GetVertexBufferGPU();
        //instance->IndexBuffer = mesh->GetIndexBufferGPU();
        //instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

        mesh->GetVertexBufferGPU( &instance->VertexBuffer, &instance->VertexBufferOffset );
        mesh->GetIndexBufferGPU( &instance->IndexBuffer, &instance->IndexBufferOffset );
        mesh->GetWeightsBufferGPU( &instance->WeightsBuffer, &instance->WeightsBufferOffset );

        instance->LightmapUVChannel = nullptr;
        instance->Lightmap = nullptr;
        instance->VertexLightChannel = nullptr;
        instance->IndexCount = subpart->GetIndexCount();
        instance->StartIndexLocation = subpart->GetFirstIndex();
        instance->BaseVertexLocation = subpart->GetBaseVertex();
        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonSize = skeletonSize;
        instance->Matrix = instanceMatrix;

        if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
            instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix * InComponent->GetWorldRotation().ToMatrix();
        }

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey =
            ((uint64_t)(InComponent->RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)mesh/*instance->VertexBuffer*/ ) & 0xffffu) << 8u);

        RenderDef.PolyCount += instance->IndexCount / 3;
    }
}

void ARenderFrontend::AddProceduralMesh( AProceduralMeshComponent * InComponent ) {
    // TODO

    if ( !RVRenderMeshes ) {
        return;
    }

    InComponent->PreRenderUpdate( &RenderDef );

    AProceduralMesh * mesh = InComponent->GetMesh();

    if ( !mesh ) {
        return;
    }

    mesh->PreRenderUpdate( &RenderDef );

    if ( mesh->IndexCache.IsEmpty() ) {
        return;
    }

    Float3x4 const & componentWorldTransform = InComponent->GetWorldTransformMatrix();

    Float4x4 instanceMatrix = RenderDef.View->ViewProjection * componentWorldTransform; // TODO: optimize: parallel, sse, check if transformable

    //ALevel * level = InComponent->GetLevel();

    //AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    //bool bHasLightmap = InComponent->LightmapUVChannel
    //        && InComponent->LightmapBlock >= 0
    //        && InComponent->LightmapBlock < level->Lightmaps.Size();

//    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

//        AIndexedMeshSubpart * subpart = subparts[subpartIndex];

        AMaterialInstance * materialInstance = InComponent->GetMaterialInstance();
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->PreRenderUpdate( FrameNumber );

        // Add render instance
        SRenderInstance * instance = (SRenderInstance *)GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
        if ( !instance ) {
            return;
        }

        if ( material->IsTranslucent() ) {
            FrameData.TranslucentInstances.Append( instance );
            RenderDef.View->TranslucentInstanceCount++;
        } else {
            FrameData.Instances.Append( instance );
            RenderDef.View->InstanceCount++;
        }

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU( &instance->VertexBuffer, &instance->VertexBufferOffset );
        mesh->GetIndexBufferGPU( &instance->IndexBuffer, &instance->IndexBufferOffset );

        instance->WeightsBuffer = nullptr;
        instance->WeightsBufferOffset = 0;

        //if ( bHasLightmap ) {
        //    InComponent->LightmapUVChannel->GetVertexBufferGPU( &instance->LightmapUVChannel, &instance->LightmapUVOffset );
        //    instance->LightmapOffset = InComponent->LightmapOffset;
        //    instance->Lightmap = level->Lightmaps[InComponent->LightmapBlock]->GetGPUResource();
        //} else {
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
        //}

        //if ( InComponent->VertexLightChannel ) {
        //    InComponent->VertexLightChannel->GetVertexBufferGPU( &instance->VertexLightChannel, &instance->VertexLightOffset );
        //} else {
            instance->VertexLightChannel = nullptr;
        //}

        instance->IndexCount = mesh->IndexCache.Size();//subpart->GetIndexCount();
        instance->StartIndexLocation = 0;//subpart->GetFirstIndex();
        instance->BaseVertexLocation = 0;//subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->Matrix = instanceMatrix;

        if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
            instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix * InComponent->GetWorldRotation().ToMatrix();
        }

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey =
            ((uint64_t)(InComponent->RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)mesh ) & 0xffffu) << 8u);

        RenderDef.PolyCount += instance->IndexCount / 3;

    //}
}

void ARenderFrontend::AddDirectionalShadowmap_StaticMesh( AMeshComponent * InComponent ) {
    if ( !RVRenderMeshes ) {
        return;
    }

    InComponent->PreRenderUpdate( &RenderDef );

    AIndexedMesh * mesh = InComponent->GetMesh();

    Float3x4 const & instanceMatrix = InComponent->GetWorldTransformMatrix();

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        // FIXME: check subpart bounding box here

        AIndexedMeshSubpart * subpart = subparts[subpartIndex];

        AMaterialInstance * materialInstance = InComponent->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        // Prevent rendering of instances with disabled shadow casting
        if ( material->GetGPUResource()->bNoCastShadow ) {
            continue;
        }

        SMaterialFrameData * materialInstanceFrameData = materialInstance->PreRenderUpdate( FrameNumber );

        // Add render instance
        SShadowRenderInstance * instance = (SShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( SShadowRenderInstance ) );
        if ( !instance ) {
            break;
        }

        FrameData.ShadowInstances.Append( instance );

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU( &instance->VertexBuffer, &instance->VertexBufferOffset );
        mesh->GetIndexBufferGPU( &instance->IndexBuffer, &instance->IndexBufferOffset );
        mesh->GetWeightsBufferGPU( &instance->WeightsBuffer, &instance->WeightsBufferOffset );

        instance->IndexCount = subpart->GetIndexCount();
        instance->StartIndexLocation = subpart->GetFirstIndex();
        instance->BaseVertexLocation = subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->WorldTransformMatrix = instanceMatrix;
        instance->CascadeMask = 0xffff; // TODO: Calculate!!!

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey = ((uint64_t)(InComponent->RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)mesh ) & 0xffffu) << 8u);

        RenderDef.View->ShadowInstanceCount++;

        RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

void ARenderFrontend::AddDirectionalShadowmap_SkinnedMesh( ASkinnedComponent * InComponent ) {
    if ( !RVRenderMeshes ) {
        return;
    }

    InComponent->PreRenderUpdate( &RenderDef );

    AIndexedMesh * mesh = InComponent->GetMesh();

    size_t skeletonOffset = 0;
    size_t skeletonSize = 0;

    InComponent->GetSkeletonHandle( skeletonOffset, skeletonSize );

    Float3x4 const & instanceMatrix = InComponent->GetWorldTransformMatrix();

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        // FIXME: check subpart bounding box here

        AIndexedMeshSubpart * subpart = subparts[subpartIndex];

        AMaterialInstance * materialInstance = InComponent->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        // Prevent rendering of instances with disabled shadow casting
        if ( material->GetGPUResource()->bNoCastShadow ) {
            continue;
        }

        SMaterialFrameData * materialInstanceFrameData = materialInstance->PreRenderUpdate( FrameNumber );

        // Add render instance
        SShadowRenderInstance * instance = (SShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( SShadowRenderInstance ) );
        if ( !instance ) {
            break;
        }

        FrameData.ShadowInstances.Append( instance );

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU( &instance->VertexBuffer, &instance->VertexBufferOffset );
        mesh->GetIndexBufferGPU( &instance->IndexBuffer, &instance->IndexBufferOffset );
        mesh->GetWeightsBufferGPU( &instance->WeightsBuffer, &instance->WeightsBufferOffset );

        instance->IndexCount = subpart->GetIndexCount();
        instance->StartIndexLocation = subpart->GetFirstIndex();
        instance->BaseVertexLocation = subpart->GetBaseVertex();

        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonSize = skeletonSize;
        instance->WorldTransformMatrix = instanceMatrix;
        instance->CascadeMask = 0xffff; // TODO: Calculate!!!

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey = ((uint64_t)(InComponent->RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)mesh ) & 0xffffu) << 8u);

        RenderDef.View->ShadowInstanceCount++;

        RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

void ARenderFrontend::AddDirectionalShadowmap_ProceduralMesh( AProceduralMeshComponent * InComponent ) {
    // TODO

    if ( !RVRenderMeshes ) {
        return;
    }

    InComponent->PreRenderUpdate( &RenderDef );

    AProceduralMesh * mesh = InComponent->GetMesh();

    if ( !mesh ) {
        return;
    }

    mesh->PreRenderUpdate( &RenderDef );

    if ( mesh->IndexCache.IsEmpty() ) {
        return;
    }

    Float3x4 const & componentWorldTransform = InComponent->GetWorldTransformMatrix();

    //ALevel * level = InComponent->GetLevel();

    //AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    //bool bHasLightmap = InComponent->LightmapUVChannel
    //        && InComponent->LightmapBlock >= 0
    //        && InComponent->LightmapBlock < level->Lightmaps.Size();

//    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

//        AIndexedMeshSubpart * subpart = subparts[subpartIndex];

        AMaterialInstance * materialInstance = InComponent->GetMaterialInstance();
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->PreRenderUpdate( FrameNumber );

        // Add render instance
        SShadowRenderInstance * instance = (SShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( SShadowRenderInstance ) );
        if ( !instance ) {
            return;
        }

        FrameData.ShadowInstances.Append( instance );

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU( &instance->VertexBuffer, &instance->VertexBufferOffset );
        mesh->GetIndexBufferGPU( &instance->IndexBuffer, &instance->IndexBufferOffset );

        instance->WeightsBuffer = nullptr;
        instance->WeightsBufferOffset = 0;

        instance->IndexCount = mesh->IndexCache.Size();//subpart->GetIndexCount();
        instance->StartIndexLocation = 0;//subpart->GetFirstIndex();
        instance->BaseVertexLocation = 0;//subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->WorldTransformMatrix = componentWorldTransform;
        instance->CascadeMask = 0xffff; // TODO: Calculate!!!

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey =
            ((uint64_t)(InComponent->RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)mesh ) & 0xffffu) << 8u);

        RenderDef.View->ShadowInstanceCount++;

        RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;

    //}
}

void ARenderFrontend::AddDirectionalShadowmapInstances( ARenderWorld * InWorld ) {
    if ( !RenderDef.View->NumShadowMapCascades ) {
        return;
    }

    // Create shadow instances

    for ( ADrawable * component = InWorld->GetShadowCasters() ; component ; component = component->GetNextShadowCaster() ) {

        // TODO: Perform culling for each shadow cascade, set CascadeMask

        if ( (component->GetVisibilityGroup() & RenderDef.VisibilityMask) == 0 ) {
            continue;
        }

        switch ( component->GetDrawableType() ) {
        case DRAWABLE_STATIC_MESH:
            AddDirectionalShadowmap_StaticMesh( static_cast< AMeshComponent * >( component ) );
            break;
        case DRAWABLE_SKINNED_MESH:
            AddDirectionalShadowmap_SkinnedMesh( static_cast< ASkinnedComponent * >(component) );
            break;
        case DRAWABLE_PROCEDURAL_MESH:
            AddDirectionalShadowmap_ProceduralMesh( static_cast< AProceduralMeshComponent * >(component) );
            break;
        default:
            break;
        }
    }

    // Add static shadow casters
    AWorld * world = InWorld->GetOwnerWorld();
    for ( ALevel * level : world->GetArrayOfLevels() ) {

        // TODO: Perform culling for each shadow cascade, set CascadeMask

        if ( level->ShadowCasterVerts.IsEmpty() ) {
            continue;
        }

        // Add render instance
        SShadowRenderInstance * instance = (SShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( SShadowRenderInstance ) );
        if ( !instance ) {
            break;
        }

        FrameData.ShadowInstances.Append( instance );

        instance->Material = nullptr;
        instance->MaterialInstance = nullptr;
        instance->VertexBuffer = level->GetShadowCasterVB();
        instance->VertexBufferOffset = 0;
        instance->IndexBuffer = level->GetShadowCasterIB();
        instance->IndexBufferOffset = 0;
        instance->WeightsBuffer = nullptr;
        instance->WeightsBufferOffset = 0;
        instance->IndexCount = level->ShadowCasterIndices.Size();
        instance->StartIndexLocation = 0;
        instance->BaseVertexLocation = 0;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->WorldTransformMatrix.SetIdentity();
        instance->CascadeMask = 0xffff; // TODO: Calculate!!!

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey = 0;/*  ((uint64_t)(component->RenderingOrder & 0xffu) << 56u)
                            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
                            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
                            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->VertexBuffer ) & 0xffffu) << 8u);*/

        RenderDef.View->ShadowInstanceCount++;

        RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

AN_FORCEINLINE bool CanMergeSurfaces( SSurfaceDef const * InFirst, SSurfaceDef const * InSecond ) {
    return (    InFirst->Model->Id == InSecond->Model->Id
             && InFirst->LightmapBlock == InSecond->LightmapBlock
             && InFirst->MaterialIndex == InSecond->MaterialIndex
             && InFirst->RenderingOrder == InSecond->RenderingOrder );
}

void ARenderFrontend::AddSurfaces( SSurfaceDef * const * Surfaces, int SurfaceCount ) {
    if ( !SurfaceCount ) {
        return;
    }

    int totalVerts = 0;
    int totalIndices = 0;
    for ( int i = 0 ; i < VisSurfaces.Size() ; i++ ) {
        SSurfaceDef const * surfDef = VisSurfaces[i];

        totalVerts += surfDef->NumVertices;
        totalIndices += surfDef->NumIndices;
    }

    if ( totalVerts == 0 || totalIndices < 3 ) {
        // Degenerate surfaces
        return;
    }

    SurfaceStream.VertexAddr = GStreamedMemoryGPU.AllocateVertex( totalVerts * sizeof( SMeshVertex ), nullptr );
    SurfaceStream.VertexLightAddr = GStreamedMemoryGPU.AllocateVertex( totalVerts * sizeof( SMeshVertexLight ), nullptr );
    SurfaceStream.VertexUVAddr = GStreamedMemoryGPU.AllocateVertex( totalVerts * sizeof( SMeshVertexUV ), nullptr );
    SurfaceStream.IndexAddr = GStreamedMemoryGPU.AllocateIndex( totalIndices * sizeof( unsigned int ), nullptr );

    SMeshVertex * vertices = (SMeshVertex *)GStreamedMemoryGPU.Map( SurfaceStream.VertexAddr );
    SMeshVertexLight * vertexLight = (SMeshVertexLight *)GStreamedMemoryGPU.Map( SurfaceStream.VertexLightAddr );
    SMeshVertexUV * vertexUV = (SMeshVertexUV *)GStreamedMemoryGPU.Map( SurfaceStream.VertexUVAddr );
    unsigned int * indices = (unsigned int *)GStreamedMemoryGPU.Map( SurfaceStream.IndexAddr );

    int numVerts = 0;
    int numIndices = 0;
    int firstIndex = 0;

    SSurfaceDef const * merge = Surfaces[0];
    ABrushModel const * model = merge->Model;

    for ( int i = 0 ; i < SurfaceCount ; i++ ) {
        SSurfaceDef const * surfDef = Surfaces[i];

        if ( !CanMergeSurfaces( merge, surfDef ) ) {

            // Flush merged surfaces
            AddSurface( model->ParentLevel,
                        model->SurfaceMaterials[merge->MaterialIndex],
                        merge->LightmapBlock,
                        numIndices - firstIndex,
                        firstIndex,
                        merge->RenderingOrder );

            merge = surfDef;
            model = merge->Model;
            firstIndex = numIndices;
        }

        SMeshVertex const * srcVerts = model->Vertices.ToPtr() + surfDef->FirstVertex;
        SMeshVertexUV const * srcLM = model->LightmapVerts.ToPtr() + surfDef->FirstVertex;
        SMeshVertexLight const * srcVL = model->VertexLight.ToPtr() + surfDef->FirstVertex;
        unsigned int const * srcIndices = model->Indices.ToPtr() + surfDef->FirstIndex;

        // NOTE: Here we can perform CPU transformation for surfaces (modify texCoord, color, or vertex position)

        AN_ASSERT( surfDef->FirstVertex + surfDef->NumVertices <= model->VertexLight.Size() );
        AN_ASSERT( surfDef->FirstIndex + surfDef->NumIndices <= model->Indices.Size() );

        Core::Memcpy( vertices    + numVerts, srcVerts, sizeof( SMeshVertex ) * surfDef->NumVertices );
        Core::Memcpy( vertexUV    + numVerts, srcLM   , sizeof( SMeshVertexUV ) * surfDef->NumVertices );
        Core::Memcpy( vertexLight + numVerts, srcVL   , sizeof( SMeshVertexLight ) * surfDef->NumVertices );

        for ( int ind = 0 ; ind < surfDef->NumIndices ; ind++ ) {
            *indices++ = numVerts + srcIndices[ind];
        }

        numVerts += surfDef->NumVertices;
        numIndices += surfDef->NumIndices;
    }

    // Flush merged surfaces
    AddSurface( model->ParentLevel,
                model->SurfaceMaterials[merge->MaterialIndex],
                merge->LightmapBlock,
                numIndices - firstIndex,
                firstIndex,
                merge->RenderingOrder );

    AN_ASSERT( numVerts == totalVerts );
    AN_ASSERT( numIndices == totalIndices );
}

void ARenderFrontend::AddSurface( ALevel * Level, AMaterialInstance * MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex, int _RenderingOrder ) {
    AMaterial * material = MaterialInstance->GetMaterial();
    SMaterialFrameData * materialInstanceFrameData = MaterialInstance->PreRenderUpdate( FrameNumber );

    // Add render instance
    SRenderInstance * instance = ( SRenderInstance * )GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
    if ( !instance ) {
        return;
    }

    if ( material->IsTranslucent() ) {
        FrameData.TranslucentInstances.Append( instance );
        RenderDef.View->TranslucentInstanceCount++;
    } else {
        FrameData.Instances.Append( instance );
        RenderDef.View->InstanceCount++;
    }

    instance->Material = material->GetGPUResource();
    instance->MaterialInstance = materialInstanceFrameData;

    GStreamedMemoryGPU.GetPhysicalBufferAndOffset( SurfaceStream.VertexAddr, &instance->VertexBuffer, &instance->VertexBufferOffset );
    GStreamedMemoryGPU.GetPhysicalBufferAndOffset( SurfaceStream.IndexAddr, &instance->IndexBuffer, &instance->IndexBufferOffset );

    instance->WeightsBuffer = nullptr;

    instance->LightmapOffset.X = 0;
    instance->LightmapOffset.Y = 0;
    instance->LightmapOffset.Z = 1;
    instance->LightmapOffset.W = 1;
    if ( _LightmapBlock >= 0 && _LightmapBlock < Level->Lightmaps.Size() ) {
        instance->Lightmap = Level->Lightmaps[ _LightmapBlock ]->GetGPUResource();

        GStreamedMemoryGPU.GetPhysicalBufferAndOffset( SurfaceStream.VertexUVAddr, &instance->LightmapUVChannel, &instance->LightmapUVOffset );
    } else {
        instance->Lightmap = nullptr;
        instance->LightmapUVChannel = nullptr;
    }

    GStreamedMemoryGPU.GetPhysicalBufferAndOffset( SurfaceStream.VertexLightAddr, &instance->VertexLightChannel, &instance->VertexLightOffset );

    instance->IndexCount = _NumIndices;
    instance->StartIndexLocation = _FirstIndex;
    instance->BaseVertexLocation = 0;
    instance->SkeletonOffset = 0;
    instance->SkeletonSize = 0;
    instance->Matrix = RenderDef.View->ViewProjection;

    if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
        instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix;
    }

    // Generate sort key.
    // NOTE: 8 bits are still unused. We can use it in future.
    instance->SortKey =
            ((uint64_t)(_RenderingOrder & 0xffu) << 56u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
            | ((uint64_t)(Core::PHHash64( (uint64_t)SurfaceStream.VertexAddr/*instance->VertexBuffer*/ ) & 0xffffu) << 8u);

    RenderDef.PolyCount += instance->IndexCount / 3;
}
