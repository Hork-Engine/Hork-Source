/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "RenderFrontend.h"
#include "World.h"
#include "TerrainView.h"
#include "CameraComponent.h"
#include "SkinnedComponent.h"
#include "DirectionalLightComponent.h"
#include "AnalyticLightComponent.h"
#include "EnvironmentProbe.h"
#include "TerrainComponent.h"
#include "PlayerController.h"
#include "Engine.h"
#include "EnvironmentMap.h"

#include <Core/IntrusiveLinkedListMacro.h>
#include <Core/ScopedTimer.h>

AConsoleVar r_FixFrustumClusters("r_FixFrustumClusters"s, "0"s, CVAR_CHEAT);
AConsoleVar r_RenderView("r_RenderView"s, "1"s, CVAR_CHEAT);
AConsoleVar r_RenderSurfaces("r_RenderSurfaces"s, "1"s, CVAR_CHEAT);
AConsoleVar r_RenderMeshes("r_RenderMeshes"s, "1"s, CVAR_CHEAT);
AConsoleVar r_RenderTerrain("r_RenderTerrain"s, "1"s, CVAR_CHEAT);
AConsoleVar r_ResolutionScaleX("r_ResolutionScaleX"s, "1"s);
AConsoleVar r_ResolutionScaleY("r_ResolutionScaleY"s, "1"s);
AConsoleVar r_RenderLightPortals("r_RenderLightPortals"s, "1"s);
AConsoleVar r_VertexLight("r_VertexLight"s, "0"s);
AConsoleVar r_MotionBlur("r_MotionBlur"s, "1"s);

extern AConsoleVar r_HBAO;
extern AConsoleVar r_HBAODeinterleaved;

AConsoleVar com_DrawFrustumClusters("com_DrawFrustumClusters"s, "0"s, CVAR_CHEAT);

static constexpr int TerrainTileSize = 256; //32;//256;

ARenderFrontend::ARenderFrontend()
{
    TerrainMesh = MakeRef<ATerrainMesh>(TerrainTileSize);

    GEngine->GetRenderDevice()->CreateTexture(RenderCore::STextureDesc{}
                                                  .SetResolution(RenderCore::STextureResolution1DArray(256, 256))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &PhotometricProfiles);

    PhotometricProfiles->SetDebugName("Photometric Profiles");
}

ARenderFrontend::~ARenderFrontend()
{
}

struct SInstanceSortFunction
{
    bool operator()(SRenderInstance const* _A, SRenderInstance* _B)
    {
        return _A->SortKey < _B->SortKey;
    }
} InstanceSortFunction;

struct SShadowInstanceSortFunction
{
    bool operator()(SShadowRenderInstance const* _A, SShadowRenderInstance* _B)
    {
        return _A->SortKey < _B->SortKey;
    }
} ShadowInstanceSortFunction;

void ARenderFrontend::Render(AFrameLoop* InFrameLoop, ACanvas* InCanvas)
{
    FrameLoop = InFrameLoop;

    FrameData.FrameNumber = FrameNumber = FrameLoop->SysFrameNumber();

    Stat.FrontendTime       = Platform::SysMilliseconds();
    Stat.PolyCount          = 0;
    Stat.ShadowMapPolyCount = 0;

    TVector<WorldRenderView*> const& renderViews = InFrameLoop->GetRenderViews();

    AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

    FrameData.CanvasDrawData = InCanvas->GetDrawData();

    if (FrameData.CanvasDrawData->VertexCount > 0)
        FrameData.CanvasVertexData = streamedMemory->AllocateVertex(FrameData.CanvasDrawData->VertexCount * sizeof(CanvasVertex), FrameData.CanvasDrawData->Vertices);
    else
        FrameData.CanvasVertexData = 0;

    FrameData.CanvasWidth  = InCanvas->GetWidth();
    FrameData.CanvasHeight = InCanvas->GetHeight();

    const Float2 orthoMins(0.0f, (float)FrameData.CanvasHeight);
    const Float2 orthoMaxs((float)FrameData.CanvasWidth, 0.0f);
    FrameData.CanvasOrthoProjection = Float4x4::Ortho2DCC(orthoMins, orthoMaxs);

    FrameData.Instances.Clear();
    FrameData.TranslucentInstances.Clear();
    FrameData.OutlineInstances.Clear();
    FrameData.ShadowInstances.Clear();
    FrameData.LightPortals.Clear();
    FrameData.DirectionalLights.Clear();
    FrameData.LightShadowmaps.Clear();
    FrameData.TerrainInstances.Clear();

    //FrameData.ShadowCascadePoolSize = 0;
    DebugDraw.Reset();

    // Allocate views
    FrameData.NumViews    = renderViews.Size();
    FrameData.RenderViews = (SRenderView*)FrameLoop->AllocFrameMem(sizeof(SRenderView) * FrameData.NumViews);

    for (int i = 0; i < FrameData.NumViews; i++)
    {
        RenderView(i);
    }

    //int64_t t = FrameLoop->SysMilliseconds();

    for (SRenderView* view = FrameData.RenderViews; view < &FrameData.RenderViews[FrameData.NumViews]; view++)
    {
        std::sort(FrameData.Instances.Begin() + view->FirstInstance,
                  FrameData.Instances.Begin() + (view->FirstInstance + view->InstanceCount),
                  InstanceSortFunction);

        std::sort(FrameData.TranslucentInstances.Begin() + view->FirstTranslucentInstance,
                  FrameData.TranslucentInstances.Begin() + (view->FirstTranslucentInstance + view->TranslucentInstanceCount),
                  InstanceSortFunction);
    }
    //LOG( "Sort instances time {} instances count {}\n", FrameLoop->SysMilliseconds() - t, FrameData.Instances.Size() + FrameData.ShadowInstances.Size() );

    if (DebugDraw.CommandsCount() > 0)
    {
        FrameData.DbgCmds               = DebugDraw.GetCmds().ToPtr();
        FrameData.DbgVertexStreamOffset = streamedMemory->AllocateVertex(DebugDraw.GetVertices().Size() * sizeof(SDebugVertex), DebugDraw.GetVertices().ToPtr());
        FrameData.DbgIndexStreamOffset = streamedMemory->AllocateIndex(DebugDraw.GetIndices().Size() * sizeof(unsigned short), DebugDraw.GetIndices().ToPtr());
    }

    Stat.FrontendTime = Platform::SysMilliseconds() - Stat.FrontendTime;
}

void ARenderFrontend::RenderView(int _Index)
{
    WorldRenderView*      worldRenderView = FrameLoop->GetRenderViews()[_Index];
    ACameraComponent*     camera          = worldRenderView->m_pCamera;
    AWorld*               world          = camera->GetWorld();
    SRenderView*          view           = &FrameData.RenderViews[_Index];
    AStreamedMemoryGPU*   streamedMemory = FrameLoop->GetStreamedMemoryGPU();
    ATextureView*         renderTextureView = worldRenderView->GetTextureView();

    uint32_t width = renderTextureView->GetWidth();
    uint32_t height = renderTextureView->GetHeight();

    view->GameRunningTimeSeconds = world->GetRunningTimeMicro() * 0.000001;
    view->GameplayTimeSeconds    = world->GetGameplayTimeMicro() * 0.000001;
    view->GameplayTimeStep       = world->IsPaused() ? 0.0f : Math::Max(FrameLoop->SysFrameDuration() * 0.000001f, 0.0001f);
    view->ViewIndex              = _Index;
    //view->Width = Align( (size_t)(viewport->Width * r_ResolutionScaleX.GetFloat()), 2 );
    //view->Height = Align( (size_t)(viewport->Height * r_ResolutionScaleY.GetFloat()), 2 );
    view->WidthP  = worldRenderView->m_ScaledWidth;
    view->HeightP = worldRenderView->m_ScaledHeight;
    view->Width = worldRenderView->m_ScaledWidth = width * r_ResolutionScaleX.GetFloat();
    view->Height = worldRenderView->m_ScaledHeight = height * r_ResolutionScaleY.GetFloat();
    view->WidthR                                   = width;
    view->HeightR                                  = height;

    if (camera)
    {
        view->ViewPosition     = camera->GetWorldPosition();
        view->ViewRotation     = camera->GetWorldRotation();
        view->ViewRightVec     = camera->GetWorldRightVector();
        view->ViewUpVec        = camera->GetWorldUpVector();
        view->ViewDir          = camera->GetWorldForwardVector();
        view->ViewMatrix       = camera->GetViewMatrix();
        view->ProjectionMatrix = camera->GetProjectionMatrix();

        view->ViewMatrixP       = worldRenderView->m_ViewMatrix;
        view->ProjectionMatrixP = worldRenderView->m_ProjectionMatrix;

        worldRenderView->m_ViewMatrix     = view->ViewMatrix;
        worldRenderView->m_ProjectionMatrix = view->ProjectionMatrix;

        view->ViewZNear     = camera->GetZNear();
        view->ViewZFar      = camera->GetZFar();
        view->ViewOrthoMins = camera->GetOrthoMins();
        view->ViewOrthoMaxs = camera->GetOrthoMaxs();
        camera->GetEffectiveFov(view->ViewFovX, view->ViewFovY);
        view->bPerspective       = camera->IsPerspective();
        view->MaxVisibleDistance = camera->GetZFar(); // TODO: расчитать дальность до самой дальней точки на экране (по баундингам static&skinned mesh)
        view->NormalToViewMatrix = Float3x3(view->ViewMatrix);

        view->InverseProjectionMatrix = camera->IsPerspective() ?
            view->ProjectionMatrix.PerspectiveProjectionInverseFast() :
            view->ProjectionMatrix.OrthoProjectionInverseFast();
        camera->MakeClusterProjectionMatrix(view->ClusterProjectionMatrix);

        view->ClusterViewProjection         = view->ClusterProjectionMatrix * view->ViewMatrix; // TODO: try to optimize with ViewMatrix.ViewInverseFast() * ProjectionMatrix.ProjectionInverseFast()
        view->ClusterViewProjectionInversed = view->ClusterViewProjection.Inversed();
    }

    view->ViewProjection        = view->ProjectionMatrix * view->ViewMatrix;
    view->ViewProjectionP       = view->ProjectionMatrixP * view->ViewMatrixP;
    view->ViewSpaceToWorldSpace = view->ViewMatrix.Inversed(); // TODO: Check with ViewInverseFast
    view->ClipSpaceToWorldSpace = view->ViewSpaceToWorldSpace * view->InverseProjectionMatrix;
    view->BackgroundColor       = Float3(worldRenderView->BackgroundColor.R, worldRenderView->BackgroundColor.G, worldRenderView->BackgroundColor.B);
    view->bClearBackground      = worldRenderView->bClearBackground;
    view->bWireframe            = worldRenderView->bWireframe;
    if (worldRenderView->Vignette)
    {
        view->VignetteColorIntensity = worldRenderView->Vignette->ColorIntensity;
        view->VignetteOuterRadiusSqr = worldRenderView->Vignette->OuterRadiusSqr;
        view->VignetteInnerRadiusSqr = worldRenderView->Vignette->InnerRadiusSqr;
    }
    else
    {
        view->VignetteColorIntensity.W = 0;
    }

    if (worldRenderView->ColorGrading)
    {
        ColorGradingParameters* params = worldRenderView->ColorGrading;

        view->ColorGradingLUT             = params->GetLUT() ? params->GetLUT()->GetGPUResource() : nullptr;
        view->CurrentColorGradingLUT      = worldRenderView->GetCurrentColorGradingLUT()->GetGPUResource();
        view->ColorGradingAdaptationSpeed = params->GetAdaptationSpeed();

        // Procedural color grading
        view->ColorGradingGrain                   = params->GetGrain();
        view->ColorGradingGamma                   = params->GetGamma();
        view->ColorGradingLift                    = params->GetLift();
        view->ColorGradingPresaturation           = params->GetPresaturation();
        view->ColorGradingTemperatureScale        = params->GetTemperatureScale();
        view->ColorGradingTemperatureStrength     = params->GetTemperatureStrength();
        view->ColorGradingBrightnessNormalization = params->GetBrightnessNormalization();
    }
    else
    {
        view->ColorGradingLUT             = NULL;
        view->CurrentColorGradingLUT      = NULL;
        view->ColorGradingAdaptationSpeed = 0;
    }

    view->CurrentExposure = worldRenderView->GetCurrentExposure()->GetGPUResource();

    // TODO: Do not initialize light&depth textures if screen space reflections disabled
    view->LightTexture = worldRenderView->AcquireLightTexture();
    view->DepthTexture = worldRenderView->AcquireDepthTexture();
    view->RenderTarget = worldRenderView->AcquireRenderTarget();

    if (r_HBAO && r_HBAODeinterleaved)
    {
        view->HBAOMaps = worldRenderView->AcquireHBAOMaps();
    }
    else
    {
        worldRenderView->ReleaseHBAOMaps();
        view->HBAOMaps = nullptr;
    }

    view->bAllowHBAO = worldRenderView->bAllowHBAO;
    view->bAllowMotionBlur = worldRenderView->bAllowMotionBlur && r_MotionBlur;
    view->AntialiasingType = worldRenderView->AntialiasingType;

    view->VTFeedback = &worldRenderView->m_VTFeedback;

    view->PhotometricProfiles = PhotometricProfiles;

    view->NumShadowMapCascades     = 0;
    view->NumCascadedShadowMaps    = 0;
    view->FirstInstance            = FrameData.Instances.Size();
    view->InstanceCount            = 0;
    view->FirstTranslucentInstance = FrameData.TranslucentInstances.Size();
    view->TranslucentInstanceCount = 0;
    view->FirstOutlineInstance     = FrameData.OutlineInstances.Size();
    view->OutlineInstanceCount     = 0;
    //view->FirstLightPortal = FrameData.LightPortals.Size();
    //view->LightPortalsCount = 0;
    //view->FirstShadowInstance = FrameData.ShadowInstances.Size();
    //view->ShadowInstanceCount = 0;
    view->FirstDirectionalLight = FrameData.DirectionalLights.Size();
    view->NumDirectionalLights  = 0;
    view->FirstDebugDrawCommand = 0;
    view->DebugDrawCommandCount = 0;

    view->FrameNumber = worldRenderView->m_FrameNum;

    size_t size = MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof(Float4x4);

    view->ShadowMapMatricesStreamHandle = streamedMemory->AllocateConstant(size, nullptr);
    view->ShadowMapMatrices             = (Float4x4*)streamedMemory->Map(view->ShadowMapMatricesStreamHandle);

    size_t numFrustumClusters = MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y * MAX_FRUSTUM_CLUSTERS_Z;

    view->ClusterLookupStreamHandle = streamedMemory->AllocateConstant(numFrustumClusters * sizeof(SClusterHeader), nullptr);
    view->ClusterLookup             = (SClusterHeader*)streamedMemory->Map(view->ClusterLookupStreamHandle);

    view->FirstTerrainInstance = FrameData.TerrainInstances.Size();
    view->TerrainInstanceCount = 0;

    if (!r_RenderView || !camera)
    {
        return;
    }

    world->E_OnPrepareRenderFrontend.Dispatch(camera, FrameNumber);

    RenderDef.FrameNumber        = FrameNumber;
    RenderDef.View               = view;
    RenderDef.Frustum            = &camera->GetFrustum();
    RenderDef.VisibilityMask     = worldRenderView ? worldRenderView->VisibilityMask : VISIBILITY_GROUP_ALL;
    RenderDef.PolyCount          = 0;
    RenderDef.ShadowMapPolyCount = 0;
    RenderDef.StreamedMemory     = FrameLoop->GetStreamedMemoryGPU();

    m_WorldRenderView = worldRenderView;

    // Update local frame number
    worldRenderView->m_FrameNum++;

    QueryVisiblePrimitives(world);

    AEnvironmentMap* pEnvironmentMap = world->GetGlobalEnvironmentMap();

    if (pEnvironmentMap)
    {
        view->GlobalIrradianceMap = pEnvironmentMap->GetIrradianceHandle();
        view->GlobalReflectionMap = pEnvironmentMap->GetReflectionHandle();
    }
    else
    {
        if (!DummyEnvironmentMap)
        {
            DummyEnvironmentMap = AResource::CreateDefault<AEnvironmentMap>();
        }
        view->GlobalIrradianceMap = DummyEnvironmentMap->GetIrradianceHandle();
        view->GlobalReflectionMap = DummyEnvironmentMap->GetReflectionHandle();
    }

    // Generate debug draw commands
    if (worldRenderView->bDrawDebug)
    {
        DebugDraw.BeginRenderView(view, VisPass);
        world->DrawDebug(&DebugDraw);

        if (com_DrawFrustumClusters)
        {
            LightVoxelizer.DrawVoxels(&DebugDraw);
        }
    }

    AddRenderInstances(world);

    AddDirectionalShadowmapInstances(world);

    Stat.PolyCount += RenderDef.PolyCount;
    Stat.ShadowMapPolyCount += RenderDef.ShadowMapPolyCount;

    if (worldRenderView->bDrawDebug)
    {
        for (auto& it : worldRenderView->m_TerrainViews)
        {
            it.second->DrawDebug(&DebugDraw, TerrainMesh);
        }

        DebugDraw.EndRenderView();
    }
}

void ARenderFrontend::QueryVisiblePrimitives(AWorld* InWorld)
{
    SVisibilityQuery query;

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &(*RenderDef.Frustum)[i];
    }
    query.ViewPosition   = RenderDef.View->ViewPosition;
    query.ViewRightVec   = RenderDef.View->ViewRightVec;
    query.ViewUpVec      = RenderDef.View->ViewUpVec;
    query.VisibilityMask = RenderDef.VisibilityMask;
    query.QueryMask      = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS; // | VSD_QUERY_MASK_SHADOW_CAST;

    InWorld->QueryVisiblePrimitives(VisPrimitives, VisSurfaces, &VisPass, query);
}

void ARenderFrontend::QueryShadowCasters(AWorld* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, TPodVector<SPrimitiveDef*>& Primitives, TPodVector<SSurfaceDef*>& Surfaces)
{
    SVisibilityQuery query;
    BvFrustum        frustum;

    frustum.FromMatrix(LightViewProjection, true);

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &frustum[i];
    }
    query.ViewPosition   = LightPosition;
    query.ViewRightVec   = LightBasis[0];
    query.ViewUpVec      = LightBasis[1];
    query.VisibilityMask = RenderDef.VisibilityMask;
    query.QueryMask      = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_SHADOW_CAST;
#if 0
#    if 0
    Float3 clipBox[8] =
    {
        Float3(-1,-1, 0),
        Float3(-1,-1, 1),
        Float3(-1, 1, 0),
        Float3(-1, 1, 1),
        Float3( 1,-1, 0),
        Float3( 1,-1, 1),
        Float3( 1, 1, 0),
        Float3( 1, 1, 1)
    };

    Float4x4 inversed = LightViewProjection.Inversed();
    for ( int i = 0 ; i < 8 ; i++ ) {
        clipBox[i] = Float3( inversed * Float4( clipBox[i], 1.0f ) );
    }

    DebugDraw.SetDepthTest( false );
    DebugDraw.SetColor(Color4(1,1,0,1));
    DebugDraw.DrawLine( clipBox, 8 );
#    else
    Float3 vectorTR;
    Float3 vectorTL;
    Float3 vectorBR;
    Float3 vectorBL;
    Float3 origin = LightPosition;
    Float3 v[4];
    Float3 faces[4][3];
    float lightRadius = 4;
    float rayLength = lightRadius / Math::Cos( Math::_PI/4.0f );

    frustum.CornerVector_TR( vectorTR );
    frustum.CornerVector_TL( vectorTL );
    frustum.CornerVector_BR( vectorBR );
    frustum.CornerVector_BL( vectorBL );

    v[0] = origin + vectorTR * rayLength;
    v[1] = origin + vectorBR * rayLength;
    v[2] = origin + vectorBL * rayLength;
    v[3] = origin + vectorTL * rayLength;

    // top
    faces[0][0] = origin;
    faces[0][1] = v[0];
    faces[0][2] = v[3];

    // left
    faces[1][0] = origin;
    faces[1][1] = v[3];
    faces[1][2] = v[2];

    // bottom
    faces[2][0] = origin;
    faces[2][1] = v[2];
    faces[2][2] = v[1];

    // right
    faces[3][0] = origin;
    faces[3][1] = v[1];
    faces[3][2] = v[0];

    DebugDraw.SetDepthTest( true );

    DebugDraw.SetColor( Color4( 0, 1, 1, 1 ) );
    DebugDraw.DrawLine( origin, v[0] );
    DebugDraw.DrawLine( origin, v[3] );
    DebugDraw.DrawLine( origin, v[1] );
    DebugDraw.DrawLine( origin, v[2] );
    DebugDraw.DrawLine( v, 4, true );

    DebugDraw.SetColor( Color4( 1, 1, 1, 0.3f ) );
    DebugDraw.DrawTriangles( &faces[0][0], 4, sizeof( Float3 ), false );
    DebugDraw.DrawConvexPoly( v, 4, false );
#    endif
#endif
    InWorld->QueryVisiblePrimitives(Primitives, Surfaces, nullptr, query);
}

void ARenderFrontend::AddRenderInstances(AWorld* InWorld)
{
    AScopedTimer TimeCheck("AddRenderInstances");

    SRenderView*             view = RenderDef.View;
    ADrawable*               drawable;
    ATerrainComponent*       terrain;
    AAnalyticLightComponent* light;
    AEnvironmentProbe*       envProbe;
    AStreamedMemoryGPU*      streamedMemory = FrameLoop->GetStreamedMemoryGPU();
    ALightingSystem&         lightingSystem = InWorld->LightingSystem;

    VisLights.Clear();
    VisEnvProbes.Clear();

    for (SPrimitiveDef* primitive : VisPrimitives)
    {
        // TODO: Replace upcasting by something better (virtual function?)

        if (nullptr != (drawable = Upcast<ADrawable>(primitive->Owner)))
        {
            AddDrawable(drawable);
            continue;
        }

        if (nullptr != (terrain = Upcast<ATerrainComponent>(primitive->Owner)))
        {
            AddTerrain(terrain);
            continue;
        }

        if (nullptr != (light = Upcast<AAnalyticLightComponent>(primitive->Owner)))
        {
            if (!light->IsEnabled())
            {
                continue;
            }

            if (VisLights.Size() < MAX_LIGHTS)
            {
                VisLights.Add(light);
            }
            else
            {
                LOG("MAX_LIGHTS hit\n");
            }
            continue;
        }

        if (nullptr != (envProbe = Upcast<AEnvironmentProbe>(primitive->Owner)))
        {
            if (!envProbe->IsEnabled())
            {
                continue;
            }

            if (VisEnvProbes.Size() < MAX_PROBES)
            {
                VisEnvProbes.Add(envProbe);
            }
            else
            {
                LOG("MAX_PROBES hit\n");
            }
            continue;
        }

        LOG("Unhandled primitive\n");
    }

    if (r_RenderSurfaces && !VisSurfaces.IsEmpty())
    {
        struct SSortFunction
        {
            bool operator()(SSurfaceDef const* _A, SSurfaceDef const* _B)
            {
                return (_A->SortKey < _B->SortKey);
            }
        } SortFunction;

        std::sort(VisSurfaces.ToPtr(), VisSurfaces.ToPtr() + VisSurfaces.Size(), SortFunction);

        AddSurfaces(VisSurfaces.ToPtr(), VisSurfaces.Size());
    }

    // Add directional lights
    view->NumShadowMapCascades  = 0;
    view->NumCascadedShadowMaps = 0;
    for (TListIterator<ADirectionalLightComponent> dirlight(lightingSystem.DirectionalLights); dirlight; dirlight++)
    {
        if (view->NumDirectionalLights >= MAX_DIRECTIONAL_LIGHTS)
        {
            LOG("MAX_DIRECTIONAL_LIGHTS hit\n");
            break;
        }

        if (!dirlight->IsEnabled())
        {
            continue;
        }

        SDirectionalLightInstance* instance = (SDirectionalLightInstance*)FrameLoop->AllocFrameMem(sizeof(SDirectionalLightInstance));

        FrameData.DirectionalLights.Add(instance);

        dirlight->AddShadowmapCascades(FrameLoop->GetStreamedMemoryGPU(), view, &instance->ViewProjStreamHandle, &instance->FirstCascade, &instance->NumCascades);

        view->NumCascadedShadowMaps += instance->NumCascades > 0 ? 1 : 0; // Just statistics

        instance->ColorAndAmbientIntensity = dirlight->GetEffectiveColor();
        instance->Matrix                   = dirlight->GetWorldRotation().ToMatrix3x3();
        instance->MaxShadowCascades        = dirlight->GetMaxShadowCascades();
        instance->RenderMask               = ~0; //dirlight->RenderingGroup;
        instance->ShadowmapIndex           = -1;
        instance->ShadowCascadeResolution  = dirlight->GetShadowCascadeResolution();

        view->NumDirectionalLights++;
    }

    LightVoxelizer.Reset();

    // Allocate lights
    view->NumPointLights          = VisLights.Size();
    view->PointLightsStreamSize   = sizeof(SLightParameters) * view->NumPointLights;
    view->PointLightsStreamHandle = view->PointLightsStreamSize > 0 ? streamedMemory->AllocateConstant(view->PointLightsStreamSize, nullptr) : 0;
    view->PointLights             = (SLightParameters*)streamedMemory->Map(view->PointLightsStreamHandle);
    view->FirstOmnidirectionalShadowMap = FrameData.LightShadowmaps.Size();
    view->NumOmnidirectionalShadowMaps  = 0;

    int maxOmnidirectionalShadowMaps = GEngine->GetRenderBackend()->MaxOmnidirectionalShadowMapsPerView();

    for (int i = 0; i < view->NumPointLights; i++)
    {
        light = VisLights[i];

        light->PackLight(view->ViewMatrix, view->PointLights[i]);

        if (view->NumOmnidirectionalShadowMaps < maxOmnidirectionalShadowMaps)
        {
            if (AddLightShadowmap(light, view->PointLights[i].Radius))
            {
                view->PointLights[i].ShadowmapIndex = view->NumOmnidirectionalShadowMaps;
                view->NumOmnidirectionalShadowMaps++;
            }
            else
                view->PointLights[i].ShadowmapIndex = -1;
        }
        else
        {
            LOG("maxOmnidirectionalShadowMaps hit\n");
        }

        APhotometricProfile* profile = light->GetPhotometricProfile();
        if (profile)
        {
            profile->WritePhotometricData(PhotometricProfiles, FrameNumber);
        }

        SItemInfo* info = LightVoxelizer.AllocItem();
        info->Type      = ITEM_TYPE_LIGHT;
        info->ListIndex = i;

        BvAxisAlignedBox const& AABB = light->GetWorldBounds();
        info->Mins                   = AABB.Mins;
        info->Maxs                   = AABB.Maxs;

        if (LightVoxelizer.IsSSE())
        {
            info->ClipToBoxMatSSE = light->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        }
        else
        {
            info->ClipToBoxMat = light->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        }
    }

    // Allocate probes
    view->NumProbes         = VisEnvProbes.Size();
    view->ProbeStreamSize   = sizeof(SProbeParameters) * view->NumProbes;
    view->ProbeStreamHandle = view->ProbeStreamSize > 0 ?
        streamedMemory->AllocateConstant(view->ProbeStreamSize, nullptr) :
        0;
    view->Probes            = (SProbeParameters*)streamedMemory->Map(view->ProbeStreamHandle);

    for (int i = 0; i < view->NumProbes; i++)
    {
        envProbe = VisEnvProbes[i];

        envProbe->PackProbe(view->ViewMatrix, view->Probes[i]);

        SItemInfo* info = LightVoxelizer.AllocItem();
        info->Type      = ITEM_TYPE_PROBE;
        info->ListIndex = i;

        BvAxisAlignedBox const& AABB = envProbe->GetWorldBounds();
        info->Mins                   = AABB.Mins;
        info->Maxs                   = AABB.Maxs;

        if (LightVoxelizer.IsSSE())
        {
            info->ClipToBoxMatSSE = envProbe->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        }
        else
        {
            info->ClipToBoxMat = envProbe->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        }
    }

    if (!r_FixFrustumClusters)
    {
        LightVoxelizer.Voxelize(FrameLoop->GetStreamedMemoryGPU(), view);
    }
}

void ARenderFrontend::AddDrawable(ADrawable* InComponent)
{
    switch (InComponent->GetDrawableType())
    {
        case DRAWABLE_STATIC_MESH:
            AddStaticMesh(static_cast<AMeshComponent*>(InComponent));
            break;
        case DRAWABLE_SKINNED_MESH:
            AddSkinnedMesh(static_cast<ASkinnedComponent*>(InComponent));
            break;
        case DRAWABLE_PROCEDURAL_MESH:
            AddProceduralMesh(static_cast<AProceduralMeshComponent*>(InComponent));
            break;
        default:
            break;
    }
}

void ARenderFrontend::AddTerrain(ATerrainComponent* InComponent)
{
    SRenderView* view = RenderDef.View;

    if (!r_RenderTerrain)
    {
        return;
    }

    ATerrain* terrainResource = InComponent->GetTerrain();
    if (!terrainResource)
    {
        return;
    }

    ATerrainView* terrainView;

    auto it = m_WorldRenderView->m_TerrainViews.find(terrainResource->Id);
    if (it == m_WorldRenderView->m_TerrainViews.end())
    {
        terrainView                               = new ATerrainView(TerrainTileSize);
        m_WorldRenderView->m_TerrainViews[terrainResource->Id] = terrainView;
    }
    else
    {
        terrainView = it->second;
    }

    // Terrain world rotation
    Float3x3 rotation;
    rotation = InComponent->GetWorldRotation().ToMatrix3x3();

    // Terrain inversed transform
    Float3x4 const& terrainWorldTransformInv = InComponent->GetTerrainWorldTransformInversed();

    // Camera position in terrain space
    Float3 localViewPosition = terrainWorldTransformInv * view->ViewPosition;

    // Camera rotation in terrain space
    Float3x3 localRotation = rotation.Transposed() * view->ViewRotation.ToMatrix3x3();

    Float3x3 basis  = localRotation.Transposed();
    Float3   origin = basis * (-localViewPosition);

    Float4x4 localViewMatrix;
    localViewMatrix[0] = Float4(basis[0], 0.0f);
    localViewMatrix[1] = Float4(basis[1], 0.0f);
    localViewMatrix[2] = Float4(basis[2], 0.0f);
    localViewMatrix[3] = Float4(origin, 1.0f);

    Float4x4 localMVP = view->ProjectionMatrix * localViewMatrix;

    BvFrustum localFrustum;
    localFrustum.FromMatrix(localMVP, true);

    // Update resource
    terrainView->SetTerrain(terrainResource);
    // Update view
    terrainView->Update(FrameLoop->GetStreamedMemoryGPU(), TerrainMesh, localViewPosition, localFrustum);

    if (terrainView->GetIndirectBufferDrawCount() == 0)
    {
        // Everything was culled
        return;
    }

    STerrainRenderInstance* instance = (STerrainRenderInstance*)FrameLoop->AllocFrameMem(sizeof(STerrainRenderInstance));

    FrameData.TerrainInstances.Add(instance);

    instance->VertexBuffer               = TerrainMesh->GetVertexBufferGPU();
    instance->IndexBuffer                = TerrainMesh->GetIndexBufferGPU();
    instance->InstanceBufferStreamHandle = terrainView->GetInstanceBufferStreamHandle();
    instance->IndirectBufferStreamHandle = terrainView->GetIndirectBufferStreamHandle();
    instance->IndirectBufferDrawCount    = terrainView->GetIndirectBufferDrawCount();
    instance->Clipmaps                   = terrainView->GetClipmapArray();
    instance->Normals                    = terrainView->GetNormalMapArray();
    instance->ViewPositionAndHeight.X    = localViewPosition.X;
    instance->ViewPositionAndHeight.Y    = localViewPosition.Y;
    instance->ViewPositionAndHeight.Z    = localViewPosition.Z;
    instance->ViewPositionAndHeight.W    = terrainView->GetViewHeight();
    instance->LocalViewProjection        = localMVP;
    instance->ModelNormalToViewSpace     = view->NormalToViewMatrix * rotation;
    instance->ClipMin                    = terrainResource->GetClipMin();
    instance->ClipMax                    = terrainResource->GetClipMax();

    view->TerrainInstanceCount++;
}

void ARenderFrontend::AddStaticMesh(AMeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&RenderDef);

    Float3x4 const& componentWorldTransform  = InComponent->GetRenderTransformMatrix(RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix  = RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = RenderDef.View->ViewProjectionP * componentWorldTransformP;

    Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    ALevel* level = InComponent->GetLevel();
    ALevelLighting* lighting = level->Lighting;

    AIndexedMesh* mesh = InComponent->GetMesh();
    AIndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    bool bHasLightmap = (lighting &&
                         InComponent->bHasLightmap &&
                         InComponent->LightmapBlock < lighting->Lightmaps.Size() &&
                         !r_VertexLight &&
                         mesh->HasLightmapUVs());

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0, count = subparts.Size(); subpartIndex < count; subpartIndex++)
        {
            AIndexedMeshSubpart* subpart = subparts[subpartIndex];

            AMaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            SMaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(FrameLoop, FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            AMaterial* material = materialInstance->GetMaterial();

            // Add render instance
            SRenderInstance* instance = (SRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SRenderInstance));

            if (material->IsTranslucent())
            {
                FrameData.TranslucentInstances.Add(instance);
                RenderDef.View->TranslucentInstanceCount++;
            }
            else
            {
                FrameData.Instances.Add(instance);
                RenderDef.View->InstanceCount++;
            }

            if (InComponent->bOutline)
            {
                FrameData.OutlineInstances.Add(instance);
                RenderDef.View->OutlineInstanceCount++;
            }

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;

            mesh->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
            mesh->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
            mesh->GetWeightsBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

            if (bHasLightmap)
            {
                mesh->GetLightmapUVsGPU(&instance->LightmapUVChannel, &instance->LightmapUVOffset);
                instance->LightmapOffset = InComponent->LightmapOffset;
                instance->Lightmap = lighting->Lightmaps[InComponent->LightmapBlock];
            }
            else
            {
                instance->LightmapUVChannel = nullptr;
                instance->Lightmap = nullptr;
            }

            if (InComponent->bHasVertexLight)
            {
                AVertexLight* vertexLight = level->GetVertexLight(InComponent->VertexLightChannel);
                if (vertexLight && vertexLight->GetVertexCount() == mesh->GetVertexCount())
                {
                    vertexLight->GetVertexBufferGPU(&instance->VertexLightChannel, &instance->VertexLightOffset);
                }
            }
            else
            {
                instance->VertexLightChannel = nullptr;
            }

            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
            instance->SkeletonOffset = 0;
            instance->SkeletonOffsetMB = 0;
            instance->SkeletonSize = 0;
            instance->Matrix = instanceMatrix;
            instance->MatrixP = instanceMatrixP;
            instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix * worldRotation;

            uint8_t priority = material->GetRenderingPriority();
            if (InComponent->GetMotionBehavior() != MB_STATIC)
            {
                priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
            }

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            RenderDef.PolyCount += instance->IndexCount / 3;
        }
    }
}

void ARenderFrontend::AddSkinnedMesh(ASkinnedComponent* InComponent)
{
    AIndexedMesh* mesh = InComponent->GetMesh();

    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&RenderDef);

    size_t skeletonOffset   = 0;
    size_t skeletonOffsetMB = 0;
    size_t skeletonSize     = 0;

    InComponent->GetSkeletonHandle(skeletonOffset, skeletonOffsetMB, skeletonSize);

    Float3x4 const& componentWorldTransform  = InComponent->GetRenderTransformMatrix(RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix  = RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = RenderDef.View->ViewProjectionP * componentWorldTransformP;

    Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    AIndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++)
        {
            AIndexedMeshSubpart* subpart = subparts[subpartIndex];

            AMaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            SMaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(FrameLoop, FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            AMaterial* material = materialInstance->GetMaterial();

            // Add render instance
            SRenderInstance* instance = (SRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SRenderInstance));

            if (material->IsTranslucent())
            {
                FrameData.TranslucentInstances.Add(instance);
                RenderDef.View->TranslucentInstanceCount++;
            }
            else
            {
                FrameData.Instances.Add(instance);
                RenderDef.View->InstanceCount++;
            }

            if (InComponent->bOutline)
            {
                FrameData.OutlineInstances.Add(instance);
                RenderDef.View->OutlineInstanceCount++;
            }

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;
            //instance->VertexBuffer = mesh->GetVertexBufferGPU();
            //instance->IndexBuffer = mesh->GetIndexBufferGPU();
            //instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

            mesh->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
            mesh->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
            mesh->GetWeightsBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
            instance->VertexLightChannel = nullptr;
            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex();
            instance->SkeletonOffset = skeletonOffset;
            instance->SkeletonOffsetMB = skeletonOffsetMB;
            instance->SkeletonSize = skeletonSize;
            instance->Matrix = instanceMatrix;
            instance->MatrixP = instanceMatrixP;
            instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix * worldRotation;

            uint8_t priority = material->GetRenderingPriority();

            // Skinned meshes are always dynamic
            priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            RenderDef.PolyCount += instance->IndexCount / 3;
        }
    }
}

void ARenderFrontend::AddProceduralMesh(AProceduralMeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&RenderDef);

    AProceduralMesh* mesh = InComponent->GetMesh();
    if (!mesh)
    {
        return;
    }

    mesh->PreRenderUpdate(&RenderDef);

    if (mesh->IndexCache.IsEmpty())
    {
        return;
    }

    Float3x4 const& componentWorldTransform  = InComponent->GetRenderTransformMatrix(RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix  = RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = RenderDef.View->ViewProjectionP * componentWorldTransformP;

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        AMaterialInstance* materialInstance = meshRender->GetMaterial();
        HK_ASSERT(materialInstance);

        SMaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(FrameLoop, FrameNumber);
        if (!materialInstanceFrameData)
            return;

        AMaterial* material = materialInstance->GetMaterial();

        // Add render instance
        SRenderInstance* instance = (SRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SRenderInstance));

        if (material->IsTranslucent())
        {
            FrameData.TranslucentInstances.Add(instance);
            RenderDef.View->TranslucentInstanceCount++;
        }
        else
        {
            FrameData.Instances.Add(instance);
            RenderDef.View->InstanceCount++;
        }

        if (InComponent->bOutline)
        {
            FrameData.OutlineInstances.Add(instance);
            RenderDef.View->OutlineInstanceCount++;
        }

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU(RenderDef.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
        mesh->GetIndexBufferGPU(RenderDef.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

        instance->WeightsBuffer = nullptr;
        instance->WeightsBufferOffset = 0;
        instance->LightmapUVChannel = nullptr;
        instance->Lightmap = nullptr;
        instance->VertexLightChannel = nullptr;
        instance->IndexCount = mesh->IndexCache.Size();
        instance->StartIndexLocation = 0;
        instance->BaseVertexLocation = 0;
        instance->SkeletonOffset = 0;
        instance->SkeletonOffsetMB = 0;
        instance->SkeletonSize = 0;
        instance->Matrix = instanceMatrix;
        instance->MatrixP = instanceMatrixP;
        instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix * InComponent->GetWorldRotation().ToMatrix3x3();

        uint8_t priority = material->GetRenderingPriority();
        if (InComponent->GetMotionBehavior() != MB_STATIC)
        {
            priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
        }

        instance->GenerateSortKey(priority, (uint64_t)mesh);

        RenderDef.PolyCount += instance->IndexCount / 3;
    }
}

void ARenderFrontend::AddShadowmap_StaticMesh(SLightShadowmap* ShadowMap, AMeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&RenderDef);

    AIndexedMesh* mesh = InComponent->GetMesh();

    Float3x4 const& instanceMatrix = InComponent->GetWorldTransformMatrix();

    AIndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++)
        {
            // FIXME: check subpart bounding box here

            AIndexedMeshSubpart* subpart = subparts[subpartIndex];

            AMaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            AMaterial* material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if (!material->IsShadowCastEnabled())
            {
                continue;
            }

            SMaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(FrameLoop, FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            // Add render instance
            SShadowRenderInstance* instance = (SShadowRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SShadowRenderInstance));

            FrameData.ShadowInstances.Add(instance);

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;

            mesh->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
            mesh->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
            mesh->GetWeightsBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
            instance->SkeletonOffset = 0;
            instance->SkeletonSize = 0;
            instance->WorldTransformMatrix = instanceMatrix;
            instance->CascadeMask = InComponent->CascadeMask;

            uint8_t priority = material->GetRenderingPriority();

            // Dynamic/Static geometry priority is doesn't matter for shadowmap pass
            //if ( InComponent->GetMotionBehavior() != MB_STATIC ) {
            //    priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
            //}

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            ShadowMap->ShadowInstanceCount++;

            RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }
    }
}

void ARenderFrontend::AddShadowmap_SkinnedMesh(SLightShadowmap* ShadowMap, ASkinnedComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&RenderDef);

    AIndexedMesh* mesh = InComponent->GetMesh();

    size_t skeletonOffset   = 0;
    size_t skeletonOffsetMB = 0;
    size_t skeletonSize     = 0;

    InComponent->GetSkeletonHandle(skeletonOffset, skeletonOffsetMB, skeletonSize);

    Float3x4 const& instanceMatrix = InComponent->GetWorldTransformMatrix();

    AIndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++)
        {
            // FIXME: check subpart bounding box here

            AIndexedMeshSubpart* subpart = subparts[subpartIndex];

            AMaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            AMaterial* material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if (!material->IsShadowCastEnabled())
            {
                continue;
            }

            SMaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(FrameLoop, FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            // Add render instance
            SShadowRenderInstance* instance = (SShadowRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SShadowRenderInstance));

            FrameData.ShadowInstances.Add(instance);

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;

            mesh->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
            mesh->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
            mesh->GetWeightsBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex();

            instance->SkeletonOffset = skeletonOffset;
            instance->SkeletonSize = skeletonSize;
            instance->WorldTransformMatrix = instanceMatrix;
            instance->CascadeMask = InComponent->CascadeMask;

            uint8_t priority = material->GetRenderingPriority();

            // Dynamic/Static geometry priority is doesn't matter for shadowmap pass
            //priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            ShadowMap->ShadowInstanceCount++;

            RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }
    }
}

void ARenderFrontend::AddShadowmap_ProceduralMesh(SLightShadowmap* ShadowMap, AProceduralMeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&RenderDef);

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        AMaterialInstance* materialInstance = meshRender->GetMaterial();
        HK_ASSERT(materialInstance);

        AMaterial* material = materialInstance->GetMaterial();

        // Prevent rendering of instances with disabled shadow casting
        if (!material->IsShadowCastEnabled())
        {
            return;
        }

        AProceduralMesh* mesh = InComponent->GetMesh();
        if (!mesh)
        {
            return;
        }

        mesh->PreRenderUpdate(&RenderDef);

        if (mesh->IndexCache.IsEmpty())
        {
            return;
        }

        SMaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(FrameLoop, FrameNumber);
        if (!materialInstanceFrameData)
            return;

        // Add render instance
        SShadowRenderInstance* instance = (SShadowRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SShadowRenderInstance));

        FrameData.ShadowInstances.Add(instance);

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU(RenderDef.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
        mesh->GetIndexBufferGPU(RenderDef.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

        instance->WeightsBuffer = nullptr;
        instance->WeightsBufferOffset = 0;

        instance->IndexCount = mesh->IndexCache.Size();
        instance->StartIndexLocation = 0;
        instance->BaseVertexLocation = 0;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->WorldTransformMatrix = InComponent->GetWorldTransformMatrix();
        instance->CascadeMask = InComponent->CascadeMask;

        uint8_t priority = material->GetRenderingPriority();

        // Dynamic/Static geometry priority is doesn't matter for shadowmap pass
        //if ( InComponent->GetMotionBehavior() != MB_STATIC ) {
        //    priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
        //}

        instance->GenerateSortKey(priority, (uint64_t)mesh);

        ShadowMap->ShadowInstanceCount++;

        RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

void ARenderFrontend::AddDirectionalShadowmapInstances(AWorld* InWorld)
{
    if (!RenderDef.View->NumShadowMapCascades)
    {
        return;
    }

    AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

    // Create shadow instances

    ShadowCasters.Clear();
    ShadowBoxes.Clear();

    ALightingSystem& lightingSystem = InWorld->LightingSystem;

    for (TListIterator<ADrawable> component(lightingSystem.ShadowCasters); component; component++)
    {
        if ((component->GetVisibilityGroup() & RenderDef.VisibilityMask) == 0)
        {
            continue;
        }
        //component->CascadeMask = 0;

        ShadowCasters.Add(*component);
        ShadowBoxes.Add(component->GetWorldBounds());
    }

    if (ShadowBoxes.IsEmpty())
        return;

    ShadowBoxes.Resize(Align(ShadowBoxes.Size(), 4));

    ShadowCasterCullResult.ResizeInvalidate(ShadowBoxes.Size() / 4);

    BvFrustum frustum;

    for (int lightIndex = 0; lightIndex < RenderDef.View->NumDirectionalLights; lightIndex++)
    {
        int lightOffset = RenderDef.View->FirstDirectionalLight + lightIndex;

        SDirectionalLightInstance* lightDef = FrameData.DirectionalLights[lightOffset];

        if (lightDef->NumCascades == 0)
        {
            continue;
        }

        lightDef->ShadowmapIndex = FrameData.LightShadowmaps.Size();

        SLightShadowmap* shadowMap = &FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal    = FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount   = 0;

        Float4x4* lightViewProjectionMatrices = (Float4x4*)streamedMemory->Map(lightDef->ViewProjStreamHandle);

        // Perform culling for each cascade
        // TODO: Do it parallel (jobs)
        for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
        {
            frustum.FromMatrix(lightViewProjectionMatrices[cascadeIndex]);

            ShadowCasterCullResult.ZeroMem();

            frustum.CullBox_SSE(ShadowBoxes.ToPtr(), ShadowCasters.Size(), &ShadowCasterCullResult[0].Result[0]);
            //frustum.CullBox_Generic( ShadowBoxes.ToPtr(), ShadowCasters.Size(), ShadowCasterCullResult.ToPtr() );

            for (int n = 0, n2 = 0; n < ShadowCasters.Size(); n+=4, n2++)
            {
                for (int t = 0 ; t < 4 && n + t < ShadowCasters.Size() ; t++)
                    ShadowCasters[n + t]->CascadeMask |= (ShadowCasterCullResult[n2].Result[t] == 0) << cascadeIndex;
            }
        }

        for (int n = 0; n < ShadowCasters.Size(); n++)
        {
            ADrawable* component = ShadowCasters[n];

            if (component->CascadeMask == 0)
            {
                continue;
            }

            switch (component->GetDrawableType())
            {
                case DRAWABLE_STATIC_MESH:
                    AddShadowmap_StaticMesh(shadowMap, static_cast<AMeshComponent*>(component));
                    break;
                case DRAWABLE_SKINNED_MESH:
                    AddShadowmap_SkinnedMesh(shadowMap, static_cast<ASkinnedComponent*>(component));
                    break;
                case DRAWABLE_PROCEDURAL_MESH:
                    AddShadowmap_ProceduralMesh(shadowMap, static_cast<AProceduralMeshComponent*>(component));
                    break;
                default:
                    break;
            }

            // Clear cascade mask for next light source
            component->CascadeMask = 0;
        }

        // Add static shadow casters
        for (ALevel* level : InWorld->GetArrayOfLevels())
        {
            ALevelLighting* lighting = level->Lighting;

            if (!lighting)
                continue;

            // TODO: Perform culling for each shadow cascade, set CascadeMask

            if (!lighting->GetShadowCasterIndexCount())
            {
                continue;
            }

            // Add render instance
            SShadowRenderInstance* instance = (SShadowRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SShadowRenderInstance));

            FrameData.ShadowInstances.Add(instance);

            instance->Material            = nullptr;
            instance->MaterialInstance    = nullptr;
            instance->VertexBuffer        = lighting->GetShadowCasterVB();
            instance->VertexBufferOffset  = 0;
            instance->IndexBuffer         = lighting->GetShadowCasterIB();
            instance->IndexBufferOffset   = 0;
            instance->WeightsBuffer       = nullptr;
            instance->WeightsBufferOffset = 0;
            instance->IndexCount          = lighting->GetShadowCasterIndexCount();
            instance->StartIndexLocation  = 0;
            instance->BaseVertexLocation  = 0;
            instance->SkeletonOffset      = 0;
            instance->SkeletonSize        = 0;
            instance->WorldTransformMatrix.SetIdentity();
            instance->CascadeMask = 0xffff; // TODO: Calculate!!!
            instance->SortKey     = 0;

            shadowMap->ShadowInstanceCount++;

            RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }

        std::sort(FrameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
                  FrameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
                  ShadowInstanceSortFunction);

        if (r_RenderLightPortals)
        {
            // Add light portals
            for (ALevel* level : InWorld->GetArrayOfLevels())
            {
                ALevelLighting* lighting = level->Lighting;

                if (!lighting)
                    continue;

                TPodVector<SLightPortalDef> const& lightPortals = lighting->GetLightPortals();

                if (lightPortals.IsEmpty())
                {
                    continue;
                }

                for (SLightPortalDef const& lightPortal : lightPortals)
                {

                    // TODO: Perform culling for each light portal
                    // NOTE: We can precompute visible geometry for static light and meshes from every light portal

                    SLightPortalRenderInstance* instance = (SLightPortalRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SLightPortalRenderInstance));

                    FrameData.LightPortals.Add(instance);

                    instance->VertexBuffer       = lighting->GetLightPortalsVB();
                    instance->VertexBufferOffset = 0;
                    instance->IndexBuffer        = lighting->GetLightPortalsIB();
                    instance->IndexBufferOffset  = 0;
                    instance->IndexCount         = lightPortal.NumIndices;
                    instance->StartIndexLocation = lightPortal.FirstIndex;
                    instance->BaseVertexLocation = 0;

                    shadowMap->LightPortalsCount++;

                    //RenderDef.LightPortalPolyCount += instance->IndexCount / 3;
                }
            }
        }
    }
}

HK_FORCEINLINE bool CanMergeSurfaces(SSurfaceDef const* InFirst, SSurfaceDef const* InSecond)
{
    return (InFirst->Model == InSecond->Model && InFirst->LightmapBlock == InSecond->LightmapBlock && InFirst->MaterialIndex == InSecond->MaterialIndex
            /*&& InFirst->RenderingOrder == InSecond->RenderingOrder*/);
}

HK_FORCEINLINE bool CanMergeSurfacesShadowmap(SSurfaceDef const* InFirst, SSurfaceDef const* InSecond)
{
    return (InFirst->Model == InSecond->Model && InFirst->MaterialIndex == InSecond->MaterialIndex
            /*&& InFirst->RenderingOrder == InSecond->RenderingOrder*/);
}

void ARenderFrontend::AddSurfaces(SSurfaceDef* const* Surfaces, int SurfaceCount)
{
    if (!SurfaceCount)
    {
        return;
    }

    int totalVerts   = 0;
    int totalIndices = 0;
    for (int i = 0; i < SurfaceCount; i++)
    {
        SSurfaceDef const* surfDef = Surfaces[i];

        totalVerts += surfDef->NumVertices;
        totalIndices += surfDef->NumIndices;
    }

    if (totalVerts == 0 || totalIndices < 3)
    {
        // Degenerate surfaces
        return;
    }

    AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

    SurfaceStream.VertexAddr      = streamedMemory->AllocateVertex(totalVerts * sizeof(SMeshVertex), nullptr);
    SurfaceStream.VertexLightAddr = streamedMemory->AllocateVertex(totalVerts * sizeof(SMeshVertexLight), nullptr);
    SurfaceStream.VertexUVAddr    = streamedMemory->AllocateVertex(totalVerts * sizeof(SMeshVertexUV), nullptr);
    SurfaceStream.IndexAddr       = streamedMemory->AllocateIndex(totalIndices * sizeof(unsigned int), nullptr);

    SMeshVertex*      vertices    = (SMeshVertex*)streamedMemory->Map(SurfaceStream.VertexAddr);
    SMeshVertexLight* vertexLight = (SMeshVertexLight*)streamedMemory->Map(SurfaceStream.VertexLightAddr);
    SMeshVertexUV*    vertexUV    = (SMeshVertexUV*)streamedMemory->Map(SurfaceStream.VertexUVAddr);
    unsigned int*     indices     = (unsigned int*)streamedMemory->Map(SurfaceStream.IndexAddr);

    int numVerts   = 0;
    int numIndices = 0;
    int firstIndex = 0;

    SSurfaceDef const* merge = Surfaces[0];
    ABrushModel const* model = merge->Model;

    for (int i = 0; i < SurfaceCount; i++)
    {
        SSurfaceDef const* surfDef = Surfaces[i];

        if (!CanMergeSurfaces(merge, surfDef))
        {
            // Flush merged surfaces
            AddSurface(model->ParentLevel,
                       model->SurfaceMaterials[merge->MaterialIndex],
                       merge->LightmapBlock,
                       numIndices - firstIndex,
                       firstIndex /*,
                        merge->RenderingOrder*/
            );

            merge      = surfDef;
            model      = merge->Model;
            firstIndex = numIndices;
        }

        SMeshVertex const*      srcVerts   = model->Vertices.ToPtr() + surfDef->FirstVertex;
        SMeshVertexUV const*    srcLM      = model->LightmapVerts.ToPtr() + surfDef->FirstVertex;
        SMeshVertexLight const* srcVL      = model->VertexLight.ToPtr() + surfDef->FirstVertex;
        unsigned int const*     srcIndices = model->Indices.ToPtr() + surfDef->FirstIndex;

        // NOTE: Here we can perform CPU transformation for surfaces (modify texCoord, color, or vertex position)

        HK_ASSERT(surfDef->FirstVertex + surfDef->NumVertices <= model->VertexLight.Size());
        HK_ASSERT(surfDef->FirstIndex + surfDef->NumIndices <= model->Indices.Size());

        Platform::Memcpy(vertices + numVerts, srcVerts, sizeof(SMeshVertex) * surfDef->NumVertices);
        Platform::Memcpy(vertexUV + numVerts, srcLM, sizeof(SMeshVertexUV) * surfDef->NumVertices);
        Platform::Memcpy(vertexLight + numVerts, srcVL, sizeof(SMeshVertexLight) * surfDef->NumVertices);

        for (int ind = 0; ind < surfDef->NumIndices; ind++)
        {
            *indices++ = numVerts + srcIndices[ind];
        }

        numVerts += surfDef->NumVertices;
        numIndices += surfDef->NumIndices;
    }

    // Flush merged surfaces
    AddSurface(model->ParentLevel,
               model->SurfaceMaterials[merge->MaterialIndex],
               merge->LightmapBlock,
               numIndices - firstIndex,
               firstIndex /*,
                merge->RenderingOrder*/
    );

    HK_ASSERT(numVerts == totalVerts);
    HK_ASSERT(numIndices == totalIndices);
}

void ARenderFrontend::AddShadowmapSurfaces(SLightShadowmap* ShadowMap, SSurfaceDef* const* Surfaces, int SurfaceCount)
{
    if (!SurfaceCount)
    {
        return;
    }

    int totalVerts   = 0;
    int totalIndices = 0;
    for (int i = 0; i < SurfaceCount; i++)
    {
        SSurfaceDef const* surfDef = Surfaces[i];

        if (!surfDef->Model->SurfaceMaterials[surfDef->MaterialIndex]->GetMaterial()->IsShadowCastEnabled())
        {
            continue;
        }

        totalVerts += surfDef->NumVertices;
        totalIndices += surfDef->NumIndices;
    }

    if (totalVerts == 0 || totalIndices < 3)
    {
        // Degenerate surfaces
        return;
    }

    AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

    SurfaceStream.VertexAddr = streamedMemory->AllocateVertex(totalVerts * sizeof(SMeshVertex), nullptr);
    SurfaceStream.IndexAddr  = streamedMemory->AllocateIndex(totalIndices * sizeof(unsigned int), nullptr);

    SMeshVertex*  vertices = (SMeshVertex*)streamedMemory->Map(SurfaceStream.VertexAddr);
    unsigned int* indices  = (unsigned int*)streamedMemory->Map(SurfaceStream.IndexAddr);

    int numVerts   = 0;
    int numIndices = 0;
    int firstIndex = 0;

    SSurfaceDef const* merge = Surfaces[0];
    ABrushModel const* model = merge->Model;

    for (int i = 0; i < SurfaceCount; i++)
    {
        SSurfaceDef const* surfDef = Surfaces[i];

        if (!surfDef->Model->SurfaceMaterials[surfDef->MaterialIndex]->GetMaterial()->IsShadowCastEnabled())
        {
            continue;
        }

        if (!CanMergeSurfacesShadowmap(merge, surfDef))
        {

            // Flush merged surfaces
            AddShadowmapSurface(ShadowMap,
                                model->SurfaceMaterials[merge->MaterialIndex],
                                numIndices - firstIndex,
                                firstIndex /*,
                                 merge->RenderingOrder*/
            );

            merge      = surfDef;
            model      = merge->Model;
            firstIndex = numIndices;
        }

        SMeshVertex const*  srcVerts   = model->Vertices.ToPtr() + surfDef->FirstVertex;
        unsigned int const* srcIndices = model->Indices.ToPtr() + surfDef->FirstIndex;

#if 0
        DebugDraw.SetDepthTest( false );
        DebugDraw.SetColor( Color4( 1, 1, 0, 1 ) );
        DebugDraw.DrawTriangleSoupWireframe( &srcVerts->Position, sizeof( SMeshVertex ),
                                             srcIndices, surfDef->NumIndices );
        //DebugDraw.SetColor( Color4( 0, 1, 0, 1 ) );
        //DebugDraw.DrawAABB( surfDef->Bounds );
#endif

        // NOTE: Here we can perform CPU transformation for surfaces (modify texCoord, color, or vertex position)

        HK_ASSERT(surfDef->FirstVertex + surfDef->NumVertices <= model->Vertices.Size());
        HK_ASSERT(surfDef->FirstIndex + surfDef->NumIndices <= model->Indices.Size());

        Platform::Memcpy(vertices + numVerts, srcVerts, sizeof(SMeshVertex) * surfDef->NumVertices);

        for (int ind = 0; ind < surfDef->NumIndices; ind++)
        {
            *indices++ = numVerts + srcIndices[ind];
        }

        numVerts += surfDef->NumVertices;
        numIndices += surfDef->NumIndices;
    }

    // Flush merged surfaces
    AddShadowmapSurface(ShadowMap,
                        model->SurfaceMaterials[merge->MaterialIndex],
                        numIndices - firstIndex,
                        firstIndex /*,
                         merge->RenderingOrder*/
    );

    HK_ASSERT(numVerts == totalVerts);
    HK_ASSERT(numIndices == totalIndices);
}

void ARenderFrontend::AddSurface(ALevel* Level, AMaterialInstance* MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex /*, int _RenderingOrder*/)
{
    AMaterial*          material                  = MaterialInstance->GetMaterial();
    SMaterialFrameData* materialInstanceFrameData = MaterialInstance->PreRenderUpdate(FrameLoop, FrameNumber);

    if (!materialInstanceFrameData)
        return;

    // Add render instance
    SRenderInstance* instance = (SRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SRenderInstance));

    if (material->IsTranslucent())
    {
        FrameData.TranslucentInstances.Add(instance);
        RenderDef.View->TranslucentInstanceCount++;
    }
    else
    {
        FrameData.Instances.Add(instance);
        RenderDef.View->InstanceCount++;
    }

    //if ( bOutline ) {
    //    FrameData.OutlineInstances.Add( instance );
    //    RenderDef.View->OutlineInstanceCount++;
    //}

    instance->Material         = material->GetGPUResource();
    instance->MaterialInstance = materialInstanceFrameData;

    AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexAddr, &instance->VertexBuffer, &instance->VertexBufferOffset);
    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.IndexAddr, &instance->IndexBuffer, &instance->IndexBufferOffset);

    instance->WeightsBuffer = nullptr;

    instance->LightmapOffset.X = 0;
    instance->LightmapOffset.Y = 0;
    instance->LightmapOffset.Z = 1;
    instance->LightmapOffset.W = 1;

    ALevelLighting* lighting = Level->Lighting;
    if (lighting && _LightmapBlock >= 0 && _LightmapBlock < lighting->Lightmaps.Size() && !r_VertexLight)
    {
        instance->Lightmap = lighting->Lightmaps[_LightmapBlock];

        streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexUVAddr, &instance->LightmapUVChannel, &instance->LightmapUVOffset);
    }
    else
    {
        instance->Lightmap          = nullptr;
        instance->LightmapUVChannel = nullptr;
    }

    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexLightAddr, &instance->VertexLightChannel, &instance->VertexLightOffset);

    instance->IndexCount             = _NumIndices;
    instance->StartIndexLocation     = _FirstIndex;
    instance->BaseVertexLocation     = 0;
    instance->SkeletonOffset         = 0;
    instance->SkeletonOffsetMB       = 0;
    instance->SkeletonSize           = 0;
    instance->Matrix                 = RenderDef.View->ViewProjection;
    instance->MatrixP                = RenderDef.View->ViewProjectionP;
    instance->ModelNormalToViewSpace = RenderDef.View->NormalToViewMatrix;

    uint8_t priority = material->GetRenderingPriority();

    instance->GenerateSortKey(priority, SurfaceStream.VertexAddr);

    RenderDef.PolyCount += instance->IndexCount / 3;
}

void ARenderFrontend::AddShadowmapSurface(SLightShadowmap* ShadowMap, AMaterialInstance* MaterialInstance, int _NumIndices, int _FirstIndex /*, int _RenderingOrder*/)
{
    AMaterial*          material                  = MaterialInstance->GetMaterial();
    SMaterialFrameData* materialInstanceFrameData = MaterialInstance->PreRenderUpdate(FrameLoop, FrameNumber);

    if (!materialInstanceFrameData)
        return;

    // Add render instance
    SShadowRenderInstance* instance = (SShadowRenderInstance*)FrameLoop->AllocFrameMem(sizeof(SShadowRenderInstance));

    FrameData.ShadowInstances.Add(instance);

    instance->Material         = material->GetGPUResource();
    instance->MaterialInstance = materialInstanceFrameData;

    AStreamedMemoryGPU* streamedMemory = FrameLoop->GetStreamedMemoryGPU();

    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexAddr, &instance->VertexBuffer, &instance->VertexBufferOffset);
    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.IndexAddr, &instance->IndexBuffer, &instance->IndexBufferOffset);

    instance->WeightsBuffer       = nullptr;
    instance->WeightsBufferOffset = 0;
    instance->WorldTransformMatrix.SetIdentity();
    instance->IndexCount         = _NumIndices;
    instance->StartIndexLocation = _FirstIndex;
    instance->BaseVertexLocation = 0;
    instance->SkeletonOffset     = 0;
    instance->SkeletonSize       = 0;
    instance->CascadeMask        = 0xffff; // TODO?

    uint8_t priority = material->GetRenderingPriority();

    instance->GenerateSortKey(priority, SurfaceStream.VertexAddr);

    ShadowMap->ShadowInstanceCount++;

    RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
}

bool ARenderFrontend::AddLightShadowmap(AAnalyticLightComponent* Light, float Radius)
{
    if (!Light->IsCastShadow())
    {
        return false;
    }

    AWorld* world = Light->GetWorld();

    Float4x4 const* cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();
    Float4x4        projMat          = Float4x4::PerspectiveRevCC_Cube(0.1f, 1000/*Radius*/);

    Float4x4 lightViewProjection;
    Float4x4 lightViewMatrix;

    Float3 lightPos = Light->GetWorldPosition();

    ADrawable* drawable;

    int totalInstances = 0;
    int totalSurfaces  = 0;

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        Float3x3 basis  = Float3x3(cubeFaceMatrices[faceIndex]);
        Float3   origin = basis * (-lightPos);

        lightViewMatrix[0] = Float4(basis[0], 0.0f);
        lightViewMatrix[1] = Float4(basis[1], 0.0f);
        lightViewMatrix[2] = Float4(basis[2], 0.0f);
        lightViewMatrix[3] = Float4(origin, 1.0f);

        //lightViewMatrix    = cubeFaceMatrices[faceIndex];
        //lightViewMatrix[3] = Float4(Float3x3(lightViewMatrix) * -lightPos, 1.0f);

        lightViewProjection = projMat * lightViewMatrix;

        // TODO: VSD не учитывает FarPlane для кулинга - исправить это
        QueryShadowCasters(world, lightViewProjection, lightPos, Float3x3(cubeFaceMatrices[faceIndex]), VisPrimitives, VisSurfaces);

        SLightShadowmap* shadowMap = &FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal    = FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount   = 0;
        shadowMap->LightPosition       = lightPos;

        for (SPrimitiveDef* primitive : VisPrimitives)
        {
            // TODO: Replace upcasting by something better (virtual function?)

            if (nullptr != (drawable = Upcast<ADrawable>(primitive->Owner)))
            {
                drawable->CascadeMask = 1 << faceIndex;

                switch (drawable->GetDrawableType())
                {
                    case DRAWABLE_STATIC_MESH:
                        AddShadowmap_StaticMesh(shadowMap, static_cast<AMeshComponent*>(drawable));
                        break;
                    case DRAWABLE_SKINNED_MESH:
                        AddShadowmap_SkinnedMesh(shadowMap, static_cast<ASkinnedComponent*>(drawable));
                        break;
                    case DRAWABLE_PROCEDURAL_MESH:
                        AddShadowmap_ProceduralMesh(shadowMap, static_cast<AProceduralMeshComponent*>(drawable));
                        break;
                    default:
                        break;
                }

#if 0
                DebugDraw.SetDepthTest( false );
                DebugDraw.SetColor( Color4( 0, 1, 0, 1 ) );
                DebugDraw.DrawAABB( drawable->GetWorldBounds() );
#endif

                drawable->CascadeMask = 0;
            }
        }

        if (r_RenderSurfaces && !VisSurfaces.IsEmpty())
        {
            struct SSortFunction
            {
                bool operator()(SSurfaceDef const* _A, SSurfaceDef const* _B)
                {
                    return (_A->SortKey < _B->SortKey);
                }
            } SortFunction;

            std::sort(VisSurfaces.ToPtr(), VisSurfaces.ToPtr() + VisSurfaces.Size(), SortFunction);

            AddShadowmapSurfaces(shadowMap, VisSurfaces.ToPtr(), VisSurfaces.Size());

            totalSurfaces += VisSurfaces.Size();
        }

        std::sort(FrameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
                  FrameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
                  ShadowInstanceSortFunction);

        totalInstances += shadowMap->ShadowInstanceCount;
    }

    if (totalInstances == 0 && totalSurfaces == 0)
    {
        FrameData.LightShadowmaps.Resize(FrameData.LightShadowmaps.Size() - 6);
        return false;
    }

    return true;
}
