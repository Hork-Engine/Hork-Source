/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include "Engine.h"
#include "TerrainView.h"
#include "EnvironmentMap.h"

#include "World/World.h"
#include "World/CameraComponent.h"
#include "World/SkinnedComponent.h"
#include "World/DirectionalLightComponent.h"
#include "World/PunctualLightComponent.h"
#include "World/EnvironmentProbe.h"
#include "World/TerrainComponent.h"

#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Platform/Profiler.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_FixFrustumClusters("r_FixFrustumClusters"s, "0"s, CVAR_CHEAT);
ConsoleVar r_RenderView("r_RenderView"s, "1"s, CVAR_CHEAT);
ConsoleVar r_RenderSurfaces("r_RenderSurfaces"s, "1"s, CVAR_CHEAT);
ConsoleVar r_RenderMeshes("r_RenderMeshes"s, "1"s, CVAR_CHEAT);
ConsoleVar r_RenderTerrain("r_RenderTerrain"s, "1"s, CVAR_CHEAT);
ConsoleVar r_ResolutionScaleX("r_ResolutionScaleX"s, "1"s);
ConsoleVar r_ResolutionScaleY("r_ResolutionScaleY"s, "1"s);
ConsoleVar r_RenderLightPortals("r_RenderLightPortals"s, "1"s);
ConsoleVar r_VertexLight("r_VertexLight"s, "0"s);
ConsoleVar r_MotionBlur("r_MotionBlur"s, "1"s);

extern ConsoleVar r_HBAO;
extern ConsoleVar r_HBAODeinterleaved;

ConsoleVar com_DrawFrustumClusters("com_DrawFrustumClusters"s, "0"s, CVAR_CHEAT);

static constexpr int TerrainTileSize = 256; //32;//256;

RenderFrontend::RenderFrontend()
{
    m_TerrainMesh = MakeRef<TerrainMesh>(TerrainTileSize);

    GEngine->GetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(256, 256))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_PhotometricProfiles);

    m_PhotometricProfiles->SetDebugName("Photometric Profiles");
}

RenderFrontend::~RenderFrontend()
{
}

struct InstanceSortFunction
{
    bool operator()(RenderInstance const* _A, RenderInstance* _B)
    {
        return _A->SortKey < _B->SortKey;
    }
} InstanceSortFunction;

struct ShadowInstanceSortFunction
{
    bool operator()(ShadowRenderInstance const* _A, ShadowRenderInstance* _B)
    {
        return _A->SortKey < _B->SortKey;
    }
} ShadowInstanceSortFunction;

void RenderFrontend::Render(FrameLoop* InFrameLoop, Canvas* InCanvas)
{
    HK_PROFILER_EVENT("Render frontend");

    m_FrameLoop = InFrameLoop;

    m_FrameData.FrameNumber = m_FrameNumber = m_FrameLoop->SysFrameNumber();

    m_Stat.FrontendTime = Platform::SysMilliseconds();
    m_Stat.PolyCount = 0;
    m_Stat.ShadowMapPolyCount = 0;

    TVector<WorldRenderView*> const& renderViews = InFrameLoop->GetRenderViews();

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    m_FrameData.CanvasDrawData = InCanvas->GetDrawData();

    if (m_FrameData.CanvasDrawData->VertexCount > 0)
        m_FrameData.CanvasVertexData = streamedMemory->AllocateVertex(m_FrameData.CanvasDrawData->VertexCount * sizeof(CanvasVertex), m_FrameData.CanvasDrawData->Vertices);
    else
        m_FrameData.CanvasVertexData = 0;

    m_FrameData.CanvasWidth = InCanvas->GetWidth();
    m_FrameData.CanvasHeight = InCanvas->GetHeight();

    const Float2 orthoMins(0.0f, (float)m_FrameData.CanvasHeight);
    const Float2 orthoMaxs((float)m_FrameData.CanvasWidth, 0.0f);
    m_FrameData.CanvasOrthoProjection = Float4x4::Ortho2DCC(orthoMins, orthoMaxs);

    m_FrameData.Instances.Clear();
    m_FrameData.TranslucentInstances.Clear();
    m_FrameData.OutlineInstances.Clear();
    m_FrameData.ShadowInstances.Clear();
    m_FrameData.LightPortals.Clear();
    m_FrameData.DirectionalLights.Clear();
    m_FrameData.LightShadowmaps.Clear();
    m_FrameData.TerrainInstances.Clear();

    //m_FrameData.ShadowCascadePoolSize = 0;
    m_DebugDraw.Reset();

    // Allocate views
    m_FrameData.NumViews = renderViews.Size();
    m_FrameData.RenderViews = (RenderViewData*)m_FrameLoop->AllocFrameMem(sizeof(RenderViewData) * m_FrameData.NumViews);

    for (int i = 0; i < m_FrameData.NumViews; i++)
    {
        RenderView(i);
    }

    //int64_t t = m_FrameLoop->SysMilliseconds();

    for (RenderViewData* view = m_FrameData.RenderViews; view < &m_FrameData.RenderViews[m_FrameData.NumViews]; view++)
    {
        std::sort(m_FrameData.Instances.Begin() + view->FirstInstance,
                  m_FrameData.Instances.Begin() + (view->FirstInstance + view->InstanceCount),
                  InstanceSortFunction);

        std::sort(m_FrameData.TranslucentInstances.Begin() + view->FirstTranslucentInstance,
                  m_FrameData.TranslucentInstances.Begin() + (view->FirstTranslucentInstance + view->TranslucentInstanceCount),
                  InstanceSortFunction);
    }
    //LOG( "Sort instances time {} instances count {}\n", m_FrameLoop->SysMilliseconds() - t, m_FrameData.Instances.Size() + m_FrameData.ShadowInstances.Size() );

    if (m_DebugDraw.CommandsCount() > 0)
    {
        m_FrameData.DbgCmds = m_DebugDraw.GetCmds().ToPtr();
        m_FrameData.DbgVertexStreamOffset = streamedMemory->AllocateVertex(m_DebugDraw.GetVertices().Size() * sizeof(DebugVertex), m_DebugDraw.GetVertices().ToPtr());
        m_FrameData.DbgIndexStreamOffset = streamedMemory->AllocateIndex(m_DebugDraw.GetIndices().Size() * sizeof(unsigned short), m_DebugDraw.GetIndices().ToPtr());
    }

    m_Stat.FrontendTime = Platform::SysMilliseconds() - m_Stat.FrontendTime;
}

void RenderFrontend::RenderView(int _Index)
{
    WorldRenderView* worldRenderView = m_FrameLoop->GetRenderViews()[_Index];
    CameraComponent* camera = worldRenderView->m_pCamera;
    World* world = camera->GetWorld();
    RenderViewData* view = &m_FrameData.RenderViews[_Index];
    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();
    TextureView* renderTextureView = worldRenderView->GetTextureView();

    uint32_t width = renderTextureView->GetWidth();
    uint32_t height = renderTextureView->GetHeight();

    view->GameRunningTimeSeconds = world->GetRunningTimeMicro() * 0.000001;
    view->GameplayTimeSeconds = world->GetGameplayTimeMicro() * 0.000001;
    view->GameplayTimeStep = world->IsPaused() ? 0.0f : Math::Max(m_FrameLoop->SysFrameDuration() * 0.000001f, 0.0001f);
    view->ViewIndex = _Index;
    //view->Width = Align( (size_t)(viewport->Width * r_ResolutionScaleX.GetFloat()), 2 );
    //view->Height = Align( (size_t)(viewport->Height * r_ResolutionScaleY.GetFloat()), 2 );
    view->WidthP = worldRenderView->m_ScaledWidth;
    view->HeightP = worldRenderView->m_ScaledHeight;
    view->Width = worldRenderView->m_ScaledWidth = width * r_ResolutionScaleX.GetFloat();
    view->Height = worldRenderView->m_ScaledHeight = height * r_ResolutionScaleY.GetFloat();
    view->WidthR = width;
    view->HeightR = height;

    if (camera)
    {
        view->ViewPosition = camera->GetWorldPosition();
        view->ViewRotation = camera->GetWorldRotation();
        view->ViewRightVec = camera->GetWorldRightVector();
        view->ViewUpVec = camera->GetWorldUpVector();
        view->ViewDir = camera->GetWorldForwardVector();
        view->ViewMatrix = camera->GetViewMatrix();
        view->ProjectionMatrix = camera->GetProjectionMatrix();

        view->ViewMatrixP = worldRenderView->m_ViewMatrix;
        view->ProjectionMatrixP = worldRenderView->m_ProjectionMatrix;

        worldRenderView->m_ViewMatrix = view->ViewMatrix;
        worldRenderView->m_ProjectionMatrix = view->ProjectionMatrix;

        view->ViewZNear = camera->GetZNear();
        view->ViewZFar = camera->GetZFar();
        view->ViewOrthoMins = camera->GetOrthoMins();
        view->ViewOrthoMaxs = camera->GetOrthoMaxs();
        camera->GetEffectiveFov(view->ViewFovX, view->ViewFovY);
        view->bPerspective = camera->IsPerspective();
        view->MaxVisibleDistance = camera->GetZFar(); // TODO: расчитать дальность до самой дальней точки на экране (по баундингам static&skinned mesh)
        view->NormalToViewMatrix = Float3x3(view->ViewMatrix);

        view->InverseProjectionMatrix = camera->IsPerspective() ?
            view->ProjectionMatrix.PerspectiveProjectionInverseFast() :
            view->ProjectionMatrix.OrthoProjectionInverseFast();
        camera->MakeClusterProjectionMatrix(view->ClusterProjectionMatrix);

        view->ClusterViewProjection = view->ClusterProjectionMatrix * view->ViewMatrix; // TODO: try to optimize with ViewMatrix.ViewInverseFast() * ProjectionMatrix.ProjectionInverseFast()
        view->ClusterViewProjectionInversed = view->ClusterViewProjection.Inversed();
    }

    view->ViewProjection = view->ProjectionMatrix * view->ViewMatrix;
    view->ViewProjectionP = view->ProjectionMatrixP * view->ViewMatrixP;
    view->ViewSpaceToWorldSpace = view->ViewMatrix.Inversed(); // TODO: Check with ViewInverseFast
    view->ClipSpaceToWorldSpace = view->ViewSpaceToWorldSpace * view->InverseProjectionMatrix;
    view->BackgroundColor = Float3(worldRenderView->BackgroundColor.R, worldRenderView->BackgroundColor.G, worldRenderView->BackgroundColor.B);
    view->bClearBackground = worldRenderView->bClearBackground;
    view->bWireframe = worldRenderView->bWireframe;
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

        view->ColorGradingLUT = params->GetLUT() ? params->GetLUT()->GetGPUResource() : nullptr;
        view->CurrentColorGradingLUT = worldRenderView->GetCurrentColorGradingLUT()->GetGPUResource();
        view->ColorGradingAdaptationSpeed = params->GetAdaptationSpeed();

        // Procedural color grading
        view->ColorGradingGrain = params->GetGrain();
        view->ColorGradingGamma = params->GetGamma();
        view->ColorGradingLift = params->GetLift();
        view->ColorGradingPresaturation = params->GetPresaturation();
        view->ColorGradingTemperatureScale = params->GetTemperatureScale();
        view->ColorGradingTemperatureStrength = params->GetTemperatureStrength();
        view->ColorGradingBrightnessNormalization = params->GetBrightnessNormalization();
    }
    else
    {
        view->ColorGradingLUT = NULL;
        view->CurrentColorGradingLUT = NULL;
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

    view->PhotometricProfiles = m_PhotometricProfiles;

    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;
    view->FirstInstance = m_FrameData.Instances.Size();
    view->InstanceCount = 0;
    view->FirstTranslucentInstance = m_FrameData.TranslucentInstances.Size();
    view->TranslucentInstanceCount = 0;
    view->FirstOutlineInstance = m_FrameData.OutlineInstances.Size();
    view->OutlineInstanceCount = 0;
    //view->FirstLightPortal = m_FrameData.LightPortals.Size();
    //view->LightPortalsCount = 0;
    //view->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
    //view->ShadowInstanceCount = 0;
    view->FirstDirectionalLight = m_FrameData.DirectionalLights.Size();
    view->NumDirectionalLights = 0;
    view->FirstDebugDrawCommand = 0;
    view->DebugDrawCommandCount = 0;

    view->FrameNumber = worldRenderView->m_FrameNum;

    size_t size = MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof(Float4x4);

    view->ShadowMapMatricesStreamHandle = streamedMemory->AllocateConstant(size, nullptr);
    view->ShadowMapMatrices = (Float4x4*)streamedMemory->Map(view->ShadowMapMatricesStreamHandle);

    size_t numFrustumClusters = MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y * MAX_FRUSTUM_CLUSTERS_Z;

    view->ClusterLookupStreamHandle = streamedMemory->AllocateConstant(numFrustumClusters * sizeof(ClusterHeader), nullptr);
    view->ClusterLookup = (ClusterHeader*)streamedMemory->Map(view->ClusterLookupStreamHandle);

    view->FirstTerrainInstance = m_FrameData.TerrainInstances.Size();
    view->TerrainInstanceCount = 0;

    if (!r_RenderView || !camera)
    {
        return;
    }

    world->E_OnPrepareRenderFrontend.Dispatch(camera, m_FrameNumber);

    m_RenderDef.FrameNumber = m_FrameNumber;
    m_RenderDef.View = view;
    m_RenderDef.Frustum = &camera->GetFrustum();
    m_RenderDef.VisibilityMask = worldRenderView ? worldRenderView->VisibilityMask : VISIBILITY_GROUP_ALL;
    m_RenderDef.PolyCount = 0;
    m_RenderDef.ShadowMapPolyCount = 0;
    m_RenderDef.StreamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    m_WorldRenderView = worldRenderView;

    // Update local frame number
    worldRenderView->m_FrameNum++;

    QueryVisiblePrimitives(world);

    EnvironmentMap* pEnvironmentMap = world->GetGlobalEnvironmentMap();

    if (pEnvironmentMap)
    {
        view->GlobalIrradianceMap = pEnvironmentMap->GetIrradianceHandle();
        view->GlobalReflectionMap = pEnvironmentMap->GetReflectionHandle();
    }
    else
    {
        if (!m_DummyEnvironmentMap)
        {
            m_DummyEnvironmentMap = Resource::CreateDefault<EnvironmentMap>();
        }
        view->GlobalIrradianceMap = m_DummyEnvironmentMap->GetIrradianceHandle();
        view->GlobalReflectionMap = m_DummyEnvironmentMap->GetReflectionHandle();
    }

    // Generate debug draw commands
    if (worldRenderView->bDrawDebug)
    {
        m_DebugDraw.BeginRenderView(view, m_VisPass);
        world->DrawDebug(&m_DebugDraw);

        if (com_DrawFrustumClusters)
        {
            m_LightVoxelizer.DrawVoxels(&m_DebugDraw);
        }
    }

    AddRenderInstances(world);

    AddDirectionalShadowmapInstances(world);

    m_Stat.PolyCount += m_RenderDef.PolyCount;
    m_Stat.ShadowMapPolyCount += m_RenderDef.ShadowMapPolyCount;

    if (worldRenderView->bDrawDebug)
    {
        for (auto& it : worldRenderView->m_TerrainViews)
        {
            it.second->DrawDebug(&m_DebugDraw, m_TerrainMesh);
        }

        m_DebugDraw.EndRenderView();
    }
}

void RenderFrontend::QueryVisiblePrimitives(World* InWorld)
{
    VisibilityQuery query;

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &(*m_RenderDef.Frustum)[i];
    }
    query.ViewPosition = m_RenderDef.View->ViewPosition;
    query.ViewRightVec = m_RenderDef.View->ViewRightVec;
    query.ViewUpVec = m_RenderDef.View->ViewUpVec;
    query.VisibilityMask = m_RenderDef.VisibilityMask;
    query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS; // | VSD_QUERY_MASK_SHADOW_CAST;

    InWorld->QueryVisiblePrimitives(m_VisPrimitives, m_VisSurfaces, &m_VisPass, query);
}

void RenderFrontend::QueryShadowCasters(World* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, TPodVector<PrimitiveDef*>& Primitives, TPodVector<SurfaceDef*>& Surfaces)
{
    VisibilityQuery query;
    BvFrustum frustum;

    frustum.FromMatrix(LightViewProjection, true);

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &frustum[i];
    }
    query.ViewPosition = LightPosition;
    query.ViewRightVec = LightBasis[0];
    query.ViewUpVec = LightBasis[1];
    query.VisibilityMask = m_RenderDef.VisibilityMask;
    query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_SHADOW_CAST;
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

    m_DebugDraw.SetDepthTest( false );
    m_DebugDraw.SetColor(Color4(1,1,0,1));
    m_DebugDraw.DrawLine( clipBox, 8 );
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

    m_DebugDraw.SetDepthTest( true );

    m_DebugDraw.SetColor( Color4( 0, 1, 1, 1 ) );
    m_DebugDraw.DrawLine( origin, v[0] );
    m_DebugDraw.DrawLine( origin, v[3] );
    m_DebugDraw.DrawLine( origin, v[1] );
    m_DebugDraw.DrawLine( origin, v[2] );
    m_DebugDraw.DrawLine( v, 4, true );

    m_DebugDraw.SetColor( Color4( 1, 1, 1, 0.3f ) );
    m_DebugDraw.DrawTriangles( &faces[0][0], 4, sizeof( Float3 ), false );
    m_DebugDraw.DrawConvexPoly( v, 4, false );
#    endif
#endif
    InWorld->QueryVisiblePrimitives(Primitives, Surfaces, nullptr, query);
}

void RenderFrontend::AddRenderInstances(World* InWorld)
{
    HK_PROFILER_EVENT("Add Render Instances");

    RenderViewData* view = m_RenderDef.View;
    Drawable* drawable;
    TerrainComponent* terrain;
    PunctualLightComponent* light;
    EnvironmentProbe* envProbe;
    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();
    LightingSystem& lightingSystem = InWorld->LightingSystem;

    m_VisLights.Clear();
    m_VisEnvProbes.Clear();

    for (PrimitiveDef* primitive : m_VisPrimitives)
    {
        // TODO: Replace upcasting by something better (virtual function?)

        if (nullptr != (drawable = Upcast<Drawable>(primitive->Owner)))
        {
            AddDrawable(drawable);
            continue;
        }

        if (nullptr != (terrain = Upcast<TerrainComponent>(primitive->Owner)))
        {
            AddTerrain(terrain);
            continue;
        }

        if (nullptr != (light = Upcast<PunctualLightComponent>(primitive->Owner)))
        {
            if (!light->IsEnabled())
            {
                continue;
            }

            if (m_VisLights.Size() < MAX_LIGHTS)
            {
                m_VisLights.Add(light);
            }
            else
            {
                LOG("MAX_LIGHTS hit\n");
            }
            continue;
        }

        if (nullptr != (envProbe = Upcast<EnvironmentProbe>(primitive->Owner)))
        {
            if (!envProbe->IsEnabled())
            {
                continue;
            }

            if (m_VisEnvProbes.Size() < MAX_PROBES)
            {
                m_VisEnvProbes.Add(envProbe);
            }
            else
            {
                LOG("MAX_PROBES hit\n");
            }
            continue;
        }

        LOG("Unhandled primitive\n");
    }

    if (r_RenderSurfaces && !m_VisSurfaces.IsEmpty())
    {
        struct SortFunction
        {
            bool operator()(SurfaceDef const* _A, SurfaceDef const* _B)
            {
                return (_A->SortKey < _B->SortKey);
            }
        } SortFunction;

        std::sort(m_VisSurfaces.ToPtr(), m_VisSurfaces.ToPtr() + m_VisSurfaces.Size(), SortFunction);

        AddSurfaces(m_VisSurfaces.ToPtr(), m_VisSurfaces.Size());
    }

    // Add directional lights
    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;
    for (TListIterator<DirectionalLightComponent> dirlight(lightingSystem.DirectionalLights); dirlight; dirlight++)
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

        DirectionalLightInstance* instance = (DirectionalLightInstance*)m_FrameLoop->AllocFrameMem(sizeof(DirectionalLightInstance));

        m_FrameData.DirectionalLights.Add(instance);

        dirlight->AddShadowmapCascades(m_FrameLoop->GetStreamedMemoryGPU(), view, &instance->ViewProjStreamHandle, &instance->FirstCascade, &instance->NumCascades);

        view->NumCascadedShadowMaps += instance->NumCascades > 0 ? 1 : 0; // Just statistics

        instance->ColorAndAmbientIntensity = dirlight->GetEffectiveColor();
        instance->Matrix = dirlight->GetWorldRotation().ToMatrix3x3();
        instance->MaxShadowCascades = dirlight->GetMaxShadowCascades();
        instance->RenderMask = ~0; //dirlight->RenderingGroup;
        instance->ShadowmapIndex = -1;
        instance->ShadowCascadeResolution = dirlight->GetShadowCascadeResolution();

        view->NumDirectionalLights++;
    }

    m_LightVoxelizer.Reset();

    // Allocate lights
    view->NumPointLights = m_VisLights.Size();
    view->PointLightsStreamSize = sizeof(LightParameters) * view->NumPointLights;
    view->PointLightsStreamHandle = view->PointLightsStreamSize > 0 ? streamedMemory->AllocateConstant(view->PointLightsStreamSize, nullptr) : 0;
    view->PointLights = (LightParameters*)streamedMemory->Map(view->PointLightsStreamHandle);
    view->FirstOmnidirectionalShadowMap = m_FrameData.LightShadowmaps.Size();
    view->NumOmnidirectionalShadowMaps = 0;

    int maxOmnidirectionalShadowMaps = GEngine->GetRenderBackend()->MaxOmnidirectionalShadowMapsPerView();

    for (int i = 0; i < view->NumPointLights; i++)
    {
        light = m_VisLights[i];

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

        PhotometricProfile* profile = light->GetPhotometricProfile();
        if (profile)
        {
            profile->WritePhotometricData(m_PhotometricProfiles, m_FrameNumber);
        }

        ItemInfo* info = m_LightVoxelizer.AllocItem();
        info->Type = ITEM_TYPE_LIGHT;
        info->ListIndex = i;

        BvAxisAlignedBox const& AABB = light->GetWorldBounds();
        info->Mins = AABB.Mins;
        info->Maxs = AABB.Maxs;

        if (m_LightVoxelizer.IsSSE())
        {
            info->ClipToBoxMatSSE = light->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        }
        else
        {
            info->ClipToBoxMat = light->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        }
    }

    // Allocate probes
    view->NumProbes = m_VisEnvProbes.Size();
    view->ProbeStreamSize = sizeof(ProbeParameters) * view->NumProbes;
    view->ProbeStreamHandle = view->ProbeStreamSize > 0 ?
        streamedMemory->AllocateConstant(view->ProbeStreamSize, nullptr) :
        0;
    view->Probes = (ProbeParameters*)streamedMemory->Map(view->ProbeStreamHandle);

    for (int i = 0; i < view->NumProbes; i++)
    {
        envProbe = m_VisEnvProbes[i];

        envProbe->PackProbe(view->ViewMatrix, view->Probes[i]);

        ItemInfo* info = m_LightVoxelizer.AllocItem();
        info->Type = ITEM_TYPE_PROBE;
        info->ListIndex = i;

        BvAxisAlignedBox const& AABB = envProbe->GetWorldBounds();
        info->Mins = AABB.Mins;
        info->Maxs = AABB.Maxs;

        if (m_LightVoxelizer.IsSSE())
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
        m_LightVoxelizer.Voxelize(m_FrameLoop->GetStreamedMemoryGPU(), view);
    }
}

void RenderFrontend::AddDrawable(Drawable* InComponent)
{
    switch (InComponent->GetDrawableType())
    {
        case DRAWABLE_STATIC_MESH:
            AddStaticMesh(static_cast<MeshComponent*>(InComponent));
            break;
        case DRAWABLE_SKINNED_MESH:
            AddSkinnedMesh(static_cast<SkinnedComponent*>(InComponent));
            break;
        case DRAWABLE_PROCEDURAL_MESH:
            AddProceduralMesh(static_cast<ProceduralMeshComponent*>(InComponent));
            break;
        default:
            break;
    }
}

void RenderFrontend::AddTerrain(TerrainComponent* InComponent)
{
    RenderViewData* view = m_RenderDef.View;

    if (!r_RenderTerrain)
    {
        return;
    }

    Terrain* terrainResource = InComponent->GetTerrain();
    if (!terrainResource)
    {
        return;
    }

    TerrainView* terrainView;

    auto it = m_WorldRenderView->m_TerrainViews.find(terrainResource->Id);
    if (it == m_WorldRenderView->m_TerrainViews.end())
    {
        terrainView = new TerrainView(TerrainTileSize);
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

    Float3x3 basis = localRotation.Transposed();
    Float3 origin = basis * (-localViewPosition);

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
    terrainView->Update(m_FrameLoop->GetStreamedMemoryGPU(), m_TerrainMesh, localViewPosition, localFrustum);

    if (terrainView->GetIndirectBufferDrawCount() == 0)
    {
        // Everything was culled
        return;
    }

    TerrainRenderInstance* instance = (TerrainRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(TerrainRenderInstance));

    m_FrameData.TerrainInstances.Add(instance);

    instance->VertexBuffer = m_TerrainMesh->GetVertexBufferGPU();
    instance->IndexBuffer = m_TerrainMesh->GetIndexBufferGPU();
    instance->InstanceBufferStreamHandle = terrainView->GetInstanceBufferStreamHandle();
    instance->IndirectBufferStreamHandle = terrainView->GetIndirectBufferStreamHandle();
    instance->IndirectBufferDrawCount = terrainView->GetIndirectBufferDrawCount();
    instance->Clipmaps = terrainView->GetClipmapArray();
    instance->Normals = terrainView->GetNormalMapArray();
    instance->ViewPositionAndHeight.X = localViewPosition.X;
    instance->ViewPositionAndHeight.Y = localViewPosition.Y;
    instance->ViewPositionAndHeight.Z = localViewPosition.Z;
    instance->ViewPositionAndHeight.W = terrainView->GetViewHeight();
    instance->LocalViewProjection = localMVP;
    instance->ModelNormalToViewSpace = view->NormalToViewMatrix * rotation;
    instance->ClipMin = terrainResource->GetClipMin();
    instance->ClipMax = terrainResource->GetClipMax();

    view->TerrainInstanceCount++;
}

void RenderFrontend::AddStaticMesh(MeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    Float3x4 const& componentWorldTransform = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix = m_RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = m_RenderDef.View->ViewProjectionP * componentWorldTransformP;

    Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    Level* level = InComponent->GetLevel();
    LevelLighting* lighting = level->Lighting;

    IndexedMesh* mesh = InComponent->GetMesh();
    IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

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
            IndexedMeshSubpart* subpart = subparts[subpartIndex];

            MaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            Material* material = materialInstance->GetMaterial();

            // Add render instance
            RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

            if (material->IsTranslucent())
            {
                m_FrameData.TranslucentInstances.Add(instance);
                m_RenderDef.View->TranslucentInstanceCount++;
            }
            else
            {
                m_FrameData.Instances.Add(instance);
                m_RenderDef.View->InstanceCount++;
            }

            if (InComponent->bOutline)
            {
                m_FrameData.OutlineInstances.Add(instance);
                m_RenderDef.View->OutlineInstanceCount++;
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
                VertexLight* vertexLight = level->GetVertexLight(InComponent->VertexLightChannel);
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
            instance->ModelNormalToViewSpace = m_RenderDef.View->NormalToViewMatrix * worldRotation;

            uint8_t priority = material->GetRenderingPriority();
            if (InComponent->GetMotionBehavior() != MB_STATIC)
            {
                priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
            }

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            m_RenderDef.PolyCount += instance->IndexCount / 3;
        }
    }
}

void RenderFrontend::AddSkinnedMesh(SkinnedComponent* InComponent)
{
    IndexedMesh* mesh = InComponent->GetMesh();

    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    size_t skeletonOffset = 0;
    size_t skeletonOffsetMB = 0;
    size_t skeletonSize = 0;

    InComponent->GetSkeletonHandle(skeletonOffset, skeletonOffsetMB, skeletonSize);

    Float3x4 const& componentWorldTransform = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix = m_RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = m_RenderDef.View->ViewProjectionP * componentWorldTransformP;

    Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++)
        {
            IndexedMeshSubpart* subpart = subparts[subpartIndex];

            MaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            Material* material = materialInstance->GetMaterial();

            // Add render instance
            RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

            if (material->IsTranslucent())
            {
                m_FrameData.TranslucentInstances.Add(instance);
                m_RenderDef.View->TranslucentInstanceCount++;
            }
            else
            {
                m_FrameData.Instances.Add(instance);
                m_RenderDef.View->InstanceCount++;
            }

            if (InComponent->bOutline)
            {
                m_FrameData.OutlineInstances.Add(instance);
                m_RenderDef.View->OutlineInstanceCount++;
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
            instance->ModelNormalToViewSpace = m_RenderDef.View->NormalToViewMatrix * worldRotation;

            uint8_t priority = material->GetRenderingPriority();

            // Skinned meshes are always dynamic
            priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            m_RenderDef.PolyCount += instance->IndexCount / 3;
        }
    }
}

void RenderFrontend::AddProceduralMesh(ProceduralMeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    ProceduralMesh* mesh = InComponent->GetMesh();
    if (!mesh)
    {
        return;
    }

    mesh->PreRenderUpdate(&m_RenderDef);

    if (mesh->IndexCache.IsEmpty())
    {
        return;
    }

    Float3x4 const& componentWorldTransform = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix = m_RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = m_RenderDef.View->ViewProjectionP * componentWorldTransformP;

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        MaterialInstance* materialInstance = meshRender->GetMaterial();
        HK_ASSERT(materialInstance);

        MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
        if (!materialInstanceFrameData)
            return;

        Material* material = materialInstance->GetMaterial();

        // Add render instance
        RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

        if (material->IsTranslucent())
        {
            m_FrameData.TranslucentInstances.Add(instance);
            m_RenderDef.View->TranslucentInstanceCount++;
        }
        else
        {
            m_FrameData.Instances.Add(instance);
            m_RenderDef.View->InstanceCount++;
        }

        if (InComponent->bOutline)
        {
            m_FrameData.OutlineInstances.Add(instance);
            m_RenderDef.View->OutlineInstanceCount++;
        }

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU(m_RenderDef.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
        mesh->GetIndexBufferGPU(m_RenderDef.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

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
        instance->ModelNormalToViewSpace = m_RenderDef.View->NormalToViewMatrix * InComponent->GetWorldRotation().ToMatrix3x3();

        uint8_t priority = material->GetRenderingPriority();
        if (InComponent->GetMotionBehavior() != MB_STATIC)
        {
            priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
        }

        instance->GenerateSortKey(priority, (uint64_t)mesh);

        m_RenderDef.PolyCount += instance->IndexCount / 3;
    }
}

void RenderFrontend::AddShadowmap_StaticMesh(LightShadowmap* ShadowMap, MeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    IndexedMesh* mesh = InComponent->GetMesh();

    Float3x4 const& instanceMatrix = InComponent->GetWorldTransformMatrix();

    IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++)
        {
            // FIXME: check subpart bounding box here

            IndexedMeshSubpart* subpart = subparts[subpartIndex];

            MaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            Material* material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if (!material->IsShadowCastEnabled())
            {
                continue;
            }

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            // Add render instance
            ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

            m_FrameData.ShadowInstances.Add(instance);

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

            m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }
    }
}

void RenderFrontend::AddShadowmap_SkinnedMesh(LightShadowmap* ShadowMap, SkinnedComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    IndexedMesh* mesh = InComponent->GetMesh();

    size_t skeletonOffset = 0;
    size_t skeletonOffsetMB = 0;
    size_t skeletonSize = 0;

    InComponent->GetSkeletonHandle(skeletonOffset, skeletonOffsetMB, skeletonSize);

    Float3x4 const& instanceMatrix = InComponent->GetWorldTransformMatrix();

    IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++)
        {
            // FIXME: check subpart bounding box here

            IndexedMeshSubpart* subpart = subparts[subpartIndex];

            MaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            Material* material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if (!material->IsShadowCastEnabled())
            {
                continue;
            }

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            // Add render instance
            ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

            m_FrameData.ShadowInstances.Add(instance);

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

            m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }
    }
}

void RenderFrontend::AddShadowmap_ProceduralMesh(LightShadowmap* ShadowMap, ProceduralMeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        MaterialInstance* materialInstance = meshRender->GetMaterial();
        HK_ASSERT(materialInstance);

        Material* material = materialInstance->GetMaterial();

        // Prevent rendering of instances with disabled shadow casting
        if (!material->IsShadowCastEnabled())
        {
            return;
        }

        ProceduralMesh* mesh = InComponent->GetMesh();
        if (!mesh)
        {
            return;
        }

        mesh->PreRenderUpdate(&m_RenderDef);

        if (mesh->IndexCache.IsEmpty())
        {
            return;
        }

        MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
        if (!materialInstanceFrameData)
            return;

        // Add render instance
        ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

        m_FrameData.ShadowInstances.Add(instance);

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;

        mesh->GetVertexBufferGPU(m_RenderDef.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
        mesh->GetIndexBufferGPU(m_RenderDef.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

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

        m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

void RenderFrontend::AddDirectionalShadowmapInstances(World* InWorld)
{
    if (!m_RenderDef.View->NumShadowMapCascades)
    {
        return;
    }

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    // Create shadow instances

    m_ShadowCasters.Clear();
    m_ShadowBoxes.Clear();

    LightingSystem& lightingSystem = InWorld->LightingSystem;

    for (TListIterator<Drawable> component(lightingSystem.ShadowCasters); component; component++)
    {
        if ((component->GetVisibilityGroup() & m_RenderDef.VisibilityMask) == 0)
        {
            continue;
        }
        //component->CascadeMask = 0;

        m_ShadowCasters.Add(*component);
        m_ShadowBoxes.Add(component->GetWorldBounds());
    }

    if (m_ShadowBoxes.IsEmpty())
        return;

    m_ShadowBoxes.Resize(Align(m_ShadowBoxes.Size(), 4));

    m_ShadowCasterCullResult.ResizeInvalidate(m_ShadowBoxes.Size() / 4);

    BvFrustum frustum;

    for (int lightIndex = 0; lightIndex < m_RenderDef.View->NumDirectionalLights; lightIndex++)
    {
        int lightOffset = m_RenderDef.View->FirstDirectionalLight + lightIndex;

        DirectionalLightInstance* lightDef = m_FrameData.DirectionalLights[lightOffset];

        if (lightDef->NumCascades == 0)
        {
            continue;
        }

        lightDef->ShadowmapIndex = m_FrameData.LightShadowmaps.Size();

        LightShadowmap* shadowMap = &m_FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount = 0;

        Float4x4* lightViewProjectionMatrices = (Float4x4*)streamedMemory->Map(lightDef->ViewProjStreamHandle);

        // Perform culling for each cascade
        // TODO: Do it parallel (jobs)
        for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
        {
            frustum.FromMatrix(lightViewProjectionMatrices[cascadeIndex]);

            m_ShadowCasterCullResult.ZeroMem();

            frustum.CullBox_SSE(m_ShadowBoxes.ToPtr(), m_ShadowCasters.Size(), &m_ShadowCasterCullResult[0].Result[0]);
            //frustum.CullBox_Generic( m_ShadowBoxes.ToPtr(), m_ShadowCasters.Size(), m_ShadowCasterCullResult.ToPtr() );

            for (int n = 0, n2 = 0; n < m_ShadowCasters.Size(); n += 4, n2++)
            {
                for (int t = 0; t < 4 && n + t < m_ShadowCasters.Size(); t++)
                    m_ShadowCasters[n + t]->CascadeMask |= (m_ShadowCasterCullResult[n2].Result[t] == 0) << cascadeIndex;
            }
        }

        for (int n = 0; n < m_ShadowCasters.Size(); n++)
        {
            Drawable* component = m_ShadowCasters[n];

            if (component->CascadeMask == 0)
            {
                continue;
            }

            switch (component->GetDrawableType())
            {
                case DRAWABLE_STATIC_MESH:
                    AddShadowmap_StaticMesh(shadowMap, static_cast<MeshComponent*>(component));
                    break;
                case DRAWABLE_SKINNED_MESH:
                    AddShadowmap_SkinnedMesh(shadowMap, static_cast<SkinnedComponent*>(component));
                    break;
                case DRAWABLE_PROCEDURAL_MESH:
                    AddShadowmap_ProceduralMesh(shadowMap, static_cast<ProceduralMeshComponent*>(component));
                    break;
                default:
                    break;
            }

            // Clear cascade mask for next light source
            component->CascadeMask = 0;
        }

        // Add static shadow casters
        for (Level* level : InWorld->GetArrayOfLevels())
        {
            LevelLighting* lighting = level->Lighting;

            if (!lighting)
                continue;

            // TODO: Perform culling for each shadow cascade, set CascadeMask

            if (!lighting->GetShadowCasterIndexCount())
            {
                continue;
            }

            // Add render instance
            ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

            m_FrameData.ShadowInstances.Add(instance);

            instance->Material = nullptr;
            instance->MaterialInstance = nullptr;
            instance->VertexBuffer = lighting->GetShadowCasterVB();
            instance->VertexBufferOffset = 0;
            instance->IndexBuffer = lighting->GetShadowCasterIB();
            instance->IndexBufferOffset = 0;
            instance->WeightsBuffer = nullptr;
            instance->WeightsBufferOffset = 0;
            instance->IndexCount = lighting->GetShadowCasterIndexCount();
            instance->StartIndexLocation = 0;
            instance->BaseVertexLocation = 0;
            instance->SkeletonOffset = 0;
            instance->SkeletonSize = 0;
            instance->WorldTransformMatrix.SetIdentity();
            instance->CascadeMask = 0xffff; // TODO: Calculate!!!
            instance->SortKey = 0;

            shadowMap->ShadowInstanceCount++;

            m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }

        std::sort(m_FrameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
                  m_FrameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
                  ShadowInstanceSortFunction);

        if (r_RenderLightPortals)
        {
            // Add light portals
            for (Level* level : InWorld->GetArrayOfLevels())
            {
                LevelLighting* lighting = level->Lighting;

                if (!lighting)
                    continue;

                TPodVector<LightPortalDef> const& lightPortals = lighting->GetLightPortals();

                if (lightPortals.IsEmpty())
                {
                    continue;
                }

                for (LightPortalDef const& lightPortal : lightPortals)
                {

                    // TODO: Perform culling for each light portal
                    // NOTE: We can precompute visible geometry for static light and meshes from every light portal

                    LightPortalRenderInstance* instance = (LightPortalRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(LightPortalRenderInstance));

                    m_FrameData.LightPortals.Add(instance);

                    instance->VertexBuffer = lighting->GetLightPortalsVB();
                    instance->VertexBufferOffset = 0;
                    instance->IndexBuffer = lighting->GetLightPortalsIB();
                    instance->IndexBufferOffset = 0;
                    instance->IndexCount = lightPortal.NumIndices;
                    instance->StartIndexLocation = lightPortal.FirstIndex;
                    instance->BaseVertexLocation = 0;

                    shadowMap->LightPortalsCount++;

                    //m_RenderDef.LightPortalPolyCount += instance->IndexCount / 3;
                }
            }
        }
    }
}

HK_FORCEINLINE bool CanMergeSurfaces(SurfaceDef const* InFirst, SurfaceDef const* InSecond)
{
    return (InFirst->Model == InSecond->Model && InFirst->LightmapBlock == InSecond->LightmapBlock && InFirst->MaterialIndex == InSecond->MaterialIndex
            /*&& InFirst->RenderingOrder == InSecond->RenderingOrder*/);
}

HK_FORCEINLINE bool CanMergeSurfacesShadowmap(SurfaceDef const* InFirst, SurfaceDef const* InSecond)
{
    return (InFirst->Model == InSecond->Model && InFirst->MaterialIndex == InSecond->MaterialIndex
            /*&& InFirst->RenderingOrder == InSecond->RenderingOrder*/);
}

void RenderFrontend::AddSurfaces(SurfaceDef* const* Surfaces, int SurfaceCount)
{
    if (!SurfaceCount)
    {
        return;
    }

    int totalVerts = 0;
    int totalIndices = 0;
    for (int i = 0; i < SurfaceCount; i++)
    {
        SurfaceDef const* surfDef = Surfaces[i];

        totalVerts += surfDef->NumVertices;
        totalIndices += surfDef->NumIndices;
    }

    if (totalVerts == 0 || totalIndices < 3)
    {
        // Degenerate surfaces
        return;
    }

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    SurfaceStream.VertexAddr = streamedMemory->AllocateVertex(totalVerts * sizeof(MeshVertex), nullptr);
    SurfaceStream.VertexLightAddr = streamedMemory->AllocateVertex(totalVerts * sizeof(MeshVertexLight), nullptr);
    SurfaceStream.VertexUVAddr = streamedMemory->AllocateVertex(totalVerts * sizeof(MeshVertexUV), nullptr);
    SurfaceStream.IndexAddr = streamedMemory->AllocateIndex(totalIndices * sizeof(unsigned int), nullptr);

    MeshVertex* vertices = (MeshVertex*)streamedMemory->Map(SurfaceStream.VertexAddr);
    MeshVertexLight* vertexLight = (MeshVertexLight*)streamedMemory->Map(SurfaceStream.VertexLightAddr);
    MeshVertexUV* vertexUV = (MeshVertexUV*)streamedMemory->Map(SurfaceStream.VertexUVAddr);
    unsigned int* indices = (unsigned int*)streamedMemory->Map(SurfaceStream.IndexAddr);

    int numVerts = 0;
    int numIndices = 0;
    int firstIndex = 0;

    SurfaceDef const* merge = Surfaces[0];
    BrushModel const* model = merge->Model;

    for (int i = 0; i < SurfaceCount; i++)
    {
        SurfaceDef const* surfDef = Surfaces[i];

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

            merge = surfDef;
            model = merge->Model;
            firstIndex = numIndices;
        }

        MeshVertex const* srcVerts = model->Vertices.ToPtr() + surfDef->FirstVertex;
        MeshVertexUV const* srcLM = model->LightmapVerts.ToPtr() + surfDef->FirstVertex;
        MeshVertexLight const* srcVL = model->VertexLight.ToPtr() + surfDef->FirstVertex;
        unsigned int const* srcIndices = model->Indices.ToPtr() + surfDef->FirstIndex;

        // NOTE: Here we can perform CPU transformation for surfaces (modify texCoord, color, or vertex position)

        HK_ASSERT(surfDef->FirstVertex + surfDef->NumVertices <= model->VertexLight.Size());
        HK_ASSERT(surfDef->FirstIndex + surfDef->NumIndices <= model->Indices.Size());

        Platform::Memcpy(vertices + numVerts, srcVerts, sizeof(MeshVertex) * surfDef->NumVertices);
        Platform::Memcpy(vertexUV + numVerts, srcLM, sizeof(MeshVertexUV) * surfDef->NumVertices);
        Platform::Memcpy(vertexLight + numVerts, srcVL, sizeof(MeshVertexLight) * surfDef->NumVertices);

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

void RenderFrontend::AddShadowmapSurfaces(LightShadowmap* ShadowMap, SurfaceDef* const* Surfaces, int SurfaceCount)
{
    if (!SurfaceCount)
    {
        return;
    }

    int totalVerts = 0;
    int totalIndices = 0;
    for (int i = 0; i < SurfaceCount; i++)
    {
        SurfaceDef const* surfDef = Surfaces[i];

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

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    SurfaceStream.VertexAddr = streamedMemory->AllocateVertex(totalVerts * sizeof(MeshVertex), nullptr);
    SurfaceStream.IndexAddr = streamedMemory->AllocateIndex(totalIndices * sizeof(unsigned int), nullptr);

    MeshVertex* vertices = (MeshVertex*)streamedMemory->Map(SurfaceStream.VertexAddr);
    unsigned int* indices = (unsigned int*)streamedMemory->Map(SurfaceStream.IndexAddr);

    int numVerts = 0;
    int numIndices = 0;
    int firstIndex = 0;

    SurfaceDef const* merge = Surfaces[0];
    BrushModel const* model = merge->Model;

    for (int i = 0; i < SurfaceCount; i++)
    {
        SurfaceDef const* surfDef = Surfaces[i];

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

            merge = surfDef;
            model = merge->Model;
            firstIndex = numIndices;
        }

        MeshVertex const* srcVerts = model->Vertices.ToPtr() + surfDef->FirstVertex;
        unsigned int const* srcIndices = model->Indices.ToPtr() + surfDef->FirstIndex;

#if 0
        m_DebugDraw.SetDepthTest( false );
        m_DebugDraw.SetColor( Color4( 1, 1, 0, 1 ) );
        m_DebugDraw.DrawTriangleSoupWireframe( &srcVerts->Position, sizeof( MeshVertex ),
                                             srcIndices, surfDef->NumIndices );
        //m_DebugDraw.SetColor( Color4( 0, 1, 0, 1 ) );
        //m_DebugDraw.DrawAABB( surfDef->Bounds );
#endif

        // NOTE: Here we can perform CPU transformation for surfaces (modify texCoord, color, or vertex position)

        HK_ASSERT(surfDef->FirstVertex + surfDef->NumVertices <= model->Vertices.Size());
        HK_ASSERT(surfDef->FirstIndex + surfDef->NumIndices <= model->Indices.Size());

        Platform::Memcpy(vertices + numVerts, srcVerts, sizeof(MeshVertex) * surfDef->NumVertices);

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

void RenderFrontend::AddSurface(Level* Level, MaterialInstance* MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex /*, int _RenderingOrder*/)
{
    Material* material = MaterialInstance->GetMaterial();
    MaterialFrameData* materialInstanceFrameData = MaterialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);

    if (!materialInstanceFrameData)
        return;

    // Add render instance
    RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

    if (material->IsTranslucent())
    {
        m_FrameData.TranslucentInstances.Add(instance);
        m_RenderDef.View->TranslucentInstanceCount++;
    }
    else
    {
        m_FrameData.Instances.Add(instance);
        m_RenderDef.View->InstanceCount++;
    }

    //if ( bOutline ) {
    //    m_FrameData.OutlineInstances.Add( instance );
    //    m_RenderDef.View->OutlineInstanceCount++;
    //}

    instance->Material = material->GetGPUResource();
    instance->MaterialInstance = materialInstanceFrameData;

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexAddr, &instance->VertexBuffer, &instance->VertexBufferOffset);
    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.IndexAddr, &instance->IndexBuffer, &instance->IndexBufferOffset);

    instance->WeightsBuffer = nullptr;

    instance->LightmapOffset.X = 0;
    instance->LightmapOffset.Y = 0;
    instance->LightmapOffset.Z = 1;
    instance->LightmapOffset.W = 1;

    LevelLighting* lighting = Level->Lighting;
    if (lighting && _LightmapBlock >= 0 && _LightmapBlock < lighting->Lightmaps.Size() && !r_VertexLight)
    {
        instance->Lightmap = lighting->Lightmaps[_LightmapBlock];

        streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexUVAddr, &instance->LightmapUVChannel, &instance->LightmapUVOffset);
    }
    else
    {
        instance->Lightmap = nullptr;
        instance->LightmapUVChannel = nullptr;
    }

    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexLightAddr, &instance->VertexLightChannel, &instance->VertexLightOffset);

    instance->IndexCount = _NumIndices;
    instance->StartIndexLocation = _FirstIndex;
    instance->BaseVertexLocation = 0;
    instance->SkeletonOffset = 0;
    instance->SkeletonOffsetMB = 0;
    instance->SkeletonSize = 0;
    instance->Matrix = m_RenderDef.View->ViewProjection;
    instance->MatrixP = m_RenderDef.View->ViewProjectionP;
    instance->ModelNormalToViewSpace = m_RenderDef.View->NormalToViewMatrix;

    uint8_t priority = material->GetRenderingPriority();

    instance->GenerateSortKey(priority, SurfaceStream.VertexAddr);

    m_RenderDef.PolyCount += instance->IndexCount / 3;
}

void RenderFrontend::AddShadowmapSurface(LightShadowmap* ShadowMap, MaterialInstance* MaterialInstance, int _NumIndices, int _FirstIndex /*, int _RenderingOrder*/)
{
    Material* material = MaterialInstance->GetMaterial();
    MaterialFrameData* materialInstanceFrameData = MaterialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);

    if (!materialInstanceFrameData)
        return;

    // Add render instance
    ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

    m_FrameData.ShadowInstances.Add(instance);

    instance->Material = material->GetGPUResource();
    instance->MaterialInstance = materialInstanceFrameData;

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.VertexAddr, &instance->VertexBuffer, &instance->VertexBufferOffset);
    streamedMemory->GetPhysicalBufferAndOffset(SurfaceStream.IndexAddr, &instance->IndexBuffer, &instance->IndexBufferOffset);

    instance->WeightsBuffer = nullptr;
    instance->WeightsBufferOffset = 0;
    instance->WorldTransformMatrix.SetIdentity();
    instance->IndexCount = _NumIndices;
    instance->StartIndexLocation = _FirstIndex;
    instance->BaseVertexLocation = 0;
    instance->SkeletonOffset = 0;
    instance->SkeletonSize = 0;
    instance->CascadeMask = 0xffff; // TODO?

    uint8_t priority = material->GetRenderingPriority();

    instance->GenerateSortKey(priority, SurfaceStream.VertexAddr);

    ShadowMap->ShadowInstanceCount++;

    m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
}

bool RenderFrontend::AddLightShadowmap(PunctualLightComponent* Light, float Radius)
{
    if (!Light->IsCastShadow())
    {
        return false;
    }

    World* world = Light->GetWorld();

    Float4x4 const* cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();
    Float4x4 projMat = Float4x4::PerspectiveRevCC_Cube(0.1f, 1000 /*Radius*/);

    Float4x4 lightViewProjection;
    Float4x4 lightViewMatrix;

    Float3 lightPos = Light->GetWorldPosition();

    Drawable* drawable;

    int totalInstances = 0;
    int totalSurfaces = 0;

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        Float3x3 basis = Float3x3(cubeFaceMatrices[faceIndex]);
        Float3 origin = basis * (-lightPos);

        lightViewMatrix[0] = Float4(basis[0], 0.0f);
        lightViewMatrix[1] = Float4(basis[1], 0.0f);
        lightViewMatrix[2] = Float4(basis[2], 0.0f);
        lightViewMatrix[3] = Float4(origin, 1.0f);

        //lightViewMatrix    = cubeFaceMatrices[faceIndex];
        //lightViewMatrix[3] = Float4(Float3x3(lightViewMatrix) * -lightPos, 1.0f);

        lightViewProjection = projMat * lightViewMatrix;

        // TODO: VSD не учитывает FarPlane для кулинга - исправить это
        QueryShadowCasters(world, lightViewProjection, lightPos, Float3x3(cubeFaceMatrices[faceIndex]), m_VisPrimitives, m_VisSurfaces);

        LightShadowmap* shadowMap = &m_FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount = 0;
        shadowMap->LightPosition = lightPos;

        for (PrimitiveDef* primitive : m_VisPrimitives)
        {
            // TODO: Replace upcasting by something better (virtual function?)

            if (nullptr != (drawable = Upcast<Drawable>(primitive->Owner)))
            {
                drawable->CascadeMask = 1 << faceIndex;

                switch (drawable->GetDrawableType())
                {
                    case DRAWABLE_STATIC_MESH:
                        AddShadowmap_StaticMesh(shadowMap, static_cast<MeshComponent*>(drawable));
                        break;
                    case DRAWABLE_SKINNED_MESH:
                        AddShadowmap_SkinnedMesh(shadowMap, static_cast<SkinnedComponent*>(drawable));
                        break;
                    case DRAWABLE_PROCEDURAL_MESH:
                        AddShadowmap_ProceduralMesh(shadowMap, static_cast<ProceduralMeshComponent*>(drawable));
                        break;
                    default:
                        break;
                }

#if 0
                m_DebugDraw.SetDepthTest( false );
                m_DebugDraw.SetColor( Color4( 0, 1, 0, 1 ) );
                m_DebugDraw.DrawAABB( drawable->GetWorldBounds() );
#endif

                drawable->CascadeMask = 0;
            }
        }

        if (r_RenderSurfaces && !m_VisSurfaces.IsEmpty())
        {
            struct SortFunction
            {
                bool operator()(SurfaceDef const* _A, SurfaceDef const* _B)
                {
                    return (_A->SortKey < _B->SortKey);
                }
            } SortFunction;

            std::sort(m_VisSurfaces.ToPtr(), m_VisSurfaces.ToPtr() + m_VisSurfaces.Size(), SortFunction);

            AddShadowmapSurfaces(shadowMap, m_VisSurfaces.ToPtr(), m_VisSurfaces.Size());

            totalSurfaces += m_VisSurfaces.Size();
        }

        std::sort(m_FrameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
                  m_FrameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
                  ShadowInstanceSortFunction);

        totalInstances += shadowMap->ShadowInstanceCount;
    }

    if (totalInstances == 0 && totalSurfaces == 0)
    {
        m_FrameData.LightShadowmaps.Resize(m_FrameData.LightShadowmaps.Size() - 6);
        return false;
    }

    return true;
}

HK_NAMESPACE_END
