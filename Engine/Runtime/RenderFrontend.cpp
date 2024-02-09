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
#include "GameApplication.h"

#include <Engine/Core/Profiler.h>
#include <Engine/Core/Platform.h>

#include <Engine/ECSRuntime/Components/CameraComponent.h>
#include <Engine/ECSRuntime/Components/FinalTransformComponent.h>
#include <Engine/ECSRuntime/World.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_FixFrustumClusters("r_FixFrustumClusters"s, "0"s, CVAR_CHEAT);
ConsoleVar r_RenderView("r_RenderView"s, "1"s, CVAR_CHEAT);
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

    GameApplication::GetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(256, 256))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_PhotometricProfiles);

    m_PhotometricProfiles->SetDebugName("Photometric Profiles");
}

void RenderFrontend::Render(FrameLoop* frameLoop, Canvas* canvas)
{
    HK_PROFILER_EVENT("Render frontend");

    TVector<WorldRenderView*> const& renderViews = frameLoop->GetRenderViews();
    StreamedMemoryGPU* streamedMemory = frameLoop->GetStreamedMemoryGPU();

    m_FrameLoop = frameLoop;
    m_FrameNumber = frameLoop->SysFrameNumber();
    m_DebugDraw.Reset();

    m_Stat.FrontendTime = Core::SysMilliseconds();
    m_Stat.PolyCount = 0;
    m_Stat.ShadowMapPolyCount = 0;

    m_FrameData.FrameNumber = m_FrameNumber;

    m_FrameData.pCanvasDrawData = canvas->GetDrawData();
    if (m_FrameData.pCanvasDrawData->VertexCount > 0)
        m_FrameData.CanvasVertexData = streamedMemory->AllocateVertex(m_FrameData.pCanvasDrawData->VertexCount * sizeof(CanvasVertex), m_FrameData.pCanvasDrawData->Vertices);
    else
        m_FrameData.CanvasVertexData = 0;

    m_FrameData.Instances.Clear();
    m_FrameData.TranslucentInstances.Clear();
    m_FrameData.OutlineInstances.Clear();
    m_FrameData.ShadowInstances.Clear();
    m_FrameData.LightPortals.Clear();
    m_FrameData.DirectionalLights.Clear();
    m_FrameData.LightShadowmaps.Clear();
    m_FrameData.TerrainInstances.Clear();

    m_FrameData.NumViews = renderViews.Size();
    m_FrameData.RenderViews = (RenderViewData*)frameLoop->AllocFrameMem(sizeof(RenderViewData) * m_FrameData.NumViews);

    for (int i = 0; i < m_FrameData.NumViews; i++)
    {
        WorldRenderView* worldRenderView = renderViews[i];
        RenderViewData* view = &m_FrameData.RenderViews[i];

        RenderView(worldRenderView, view);
    }

    SortRenderInstances();

    if (m_DebugDraw.CommandsCount() > 0)
    {
        m_FrameData.DbgCmds = m_DebugDraw.GetCmds().ToPtr();
        m_FrameData.DbgVertexStreamOffset = streamedMemory->AllocateVertex(m_DebugDraw.GetVertices().Size() * sizeof(DebugVertex), m_DebugDraw.GetVertices().ToPtr());
        m_FrameData.DbgIndexStreamOffset = streamedMemory->AllocateIndex(m_DebugDraw.GetIndices().Size() * sizeof(unsigned short), m_DebugDraw.GetIndices().ToPtr());
    }
    else
    {
        m_FrameData.DbgCmds = nullptr;
        m_FrameData.DbgVertexStreamOffset = 0;
        m_FrameData.DbgIndexStreamOffset = 0;
    }

    m_Stat.FrontendTime = Core::SysMilliseconds() - m_Stat.FrontendTime;
}

void RenderFrontend::ClearRenderView(RenderViewData* view)
{
    Core::ZeroMem(view, sizeof(*view));
}

void RenderFrontend::RenderView(WorldRenderView* worldRenderView, RenderViewData* view)
{
    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();
    World* world = worldRenderView->GetWorld();

    ECS::EntityView cameraEntityView = world->GetEntityView(worldRenderView->GetCamera());

    auto* camera = cameraEntityView.GetComponent<CameraComponent_ECS>();
    auto* cameraTransform = cameraEntityView.GetComponent<FinalTransformComponent>();

    if (!r_RenderView || !camera || !cameraTransform)
    {
        ClearRenderView(view);
        return;
    }

    uint32_t width = worldRenderView->GetWidth();
    uint32_t height = worldRenderView->GetHeight();

    view->FrameNumber = worldRenderView->m_FrameNum;    

    view->WidthP  = worldRenderView->m_ScaledWidth;
    view->HeightP = worldRenderView->m_ScaledHeight;
    view->Width   = worldRenderView->m_ScaledWidth = width * r_ResolutionScaleX.GetFloat();
    view->Height  = worldRenderView->m_ScaledHeight = height * r_ResolutionScaleY.GetFloat();
    view->WidthR  = width;
    view->HeightR = height;

    // FIXME: float overflow
    view->GameRunningTimeSeconds = world->GetFrame().RunningTime;
    view->GameplayTimeSeconds = world->GetFrame().VariableTime;
    view->GameplayTimeStep = world->bPaused ? 0.0f : Math::Max(world->GetFrame().VariableTimeStep, 0.0001f);

    Quat cameraRotation = cameraTransform->Rotation * camera->Rotation;

    Float3x3 billboardMatrix = cameraRotation.ToMatrix3x3();

    Float3x3 basis = billboardMatrix.Transposed();
    Float3 origin = basis * (-cameraTransform->Position);

    Float4x4 viewMatrix;
    viewMatrix[0] = Float4(basis[0], 0.0f);
    viewMatrix[1] = Float4(basis[1], 0.0f);
    viewMatrix[2] = Float4(basis[2], 0.0f);
    viewMatrix[3] = Float4(origin, 1.0f); 

    float fovx, fovy;
    camera->GetEffectiveFov(fovx, fovy);

    view->ViewPosition       = cameraTransform->Position;
    view->ViewRotation       = cameraRotation;
    view->ViewRightVec       = cameraRotation.XAxis();
    view->ViewUpVec          = cameraRotation.YAxis();
    view->ViewDir            = -cameraRotation.ZAxis();
    view->ViewMatrix         = viewMatrix;
    view->ProjectionMatrix   = camera->GetProjectionMatrix();
    view->ViewMatrixP        = worldRenderView->m_ViewMatrix;
    view->ProjectionMatrixP  = worldRenderView->m_ProjectionMatrix;
    view->ViewZNear          = camera->GetZNear();
    view->ViewZFar           = camera->GetZFar();
    view->ViewOrthoMins      = camera->GetOrthoMins();
    view->ViewOrthoMaxs      = camera->GetOrthoMaxs();
    view->ViewFovX           = fovx;
    view->ViewFovY           = fovy;
    view->bPerspective       = camera->IsPerspective();
    view->MaxVisibleDistance = camera->GetZFar(); // TODO: расчитать дальность до самой дальней точки на экране (по баундингам static&skinned mesh)
    view->NormalToViewMatrix = Float3x3(view->ViewMatrix);

    view->InverseProjectionMatrix = camera->IsPerspective() ?
            view->ProjectionMatrix.PerspectiveProjectionInverseFast() :
            view->ProjectionMatrix.OrthoProjectionInverseFast();
    camera->MakeClusterProjectionMatrix(view->ClusterProjectionMatrix);

    view->ClusterViewProjection = view->ClusterProjectionMatrix * view->ViewMatrix; // TODO: try to optimize with ViewMatrix.ViewInverseFast() * ProjectionMatrix.ProjectionInverseFast()
    view->ClusterViewProjectionInversed = view->ClusterViewProjection.Inversed();

    worldRenderView->m_ViewMatrix = view->ViewMatrix;
    worldRenderView->m_ProjectionMatrix = view->ProjectionMatrix;

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

        TextureHandle lut = params->GetLUT();
        TextureResource* lutTexture = m_ResourceManager->TryGet(lut);

        view->ColorGradingLUT = lutTexture ? lutTexture->GetTextureGPU() : nullptr;
        view->CurrentColorGradingLUT = worldRenderView->GetCurrentColorGradingLUT();
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
        view->ColorGradingLUT = nullptr;
        view->CurrentColorGradingLUT = nullptr;
        view->ColorGradingAdaptationSpeed = 0;
    }

    view->CurrentExposure = worldRenderView->GetCurrentExposure();

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

    size_t size = MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof(Float4x4);

    view->ShadowMapMatricesStreamHandle = streamedMemory->AllocateConstant(size, nullptr);
    view->ShadowMapMatrices = (Float4x4*)streamedMemory->Map(view->ShadowMapMatricesStreamHandle);

    size_t numFrustumClusters = MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y * MAX_FRUSTUM_CLUSTERS_Z;

    view->ClusterLookupStreamHandle = streamedMemory->AllocateConstant(numFrustumClusters * sizeof(ClusterHeader), nullptr);
    view->ClusterLookup = (ClusterHeader*)streamedMemory->Map(view->ClusterLookupStreamHandle);

    view->FirstTerrainInstance = m_FrameData.TerrainInstances.Size();
    view->TerrainInstanceCount = 0;

    BvFrustum frustum;
    frustum.FromMatrix(view->ViewProjection, true);

    m_RenderDef.FrameNumber = m_FrameNumber;
    m_RenderDef.View = view;
    m_RenderDef.Frustum = &frustum;
    m_RenderDef.VisibilityMask = worldRenderView->VisibilityMask;
    m_RenderDef.PolyCount = 0;
    m_RenderDef.ShadowMapPolyCount = 0;
    m_RenderDef.StreamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    m_WorldRenderView = worldRenderView;

    // Update local frame number
    worldRenderView->m_FrameNum++;

    QueryVisiblePrimitives(world);

    //EnvironmentMap* pEnvironmentMap = world->GetGlobalEnvironmentMap(); TODO

    //if (pEnvironmentMap)
    //{
    //    view->GlobalIrradianceMap = pEnvironmentMap->GetIrradianceHandle();
    //    view->GlobalReflectionMap = pEnvironmentMap->GetReflectionHandle();
    //}
    //else
    {
        #if 0// todo
        if (!m_DummyEnvironmentMap)
        {
            m_DummyEnvironmentMap = Resource::CreateDefault<EnvironmentMap>();
        }
        view->GlobalIrradianceMap = m_DummyEnvironmentMap->GetIrradianceHandle();
        view->GlobalReflectionMap = m_DummyEnvironmentMap->GetReflectionHandle();
        #else
        view->GlobalIrradianceMap = 0;
        view->GlobalReflectionMap = 0;
        #endif
    }

    // Generate debug draw commands
    if (worldRenderView->bDrawDebug)
    {
        m_DebugDraw.BeginRenderView(view, m_VisPass);

        world->DrawDebug(m_DebugDraw);

        if (com_DrawFrustumClusters)
        {
            m_LightVoxelizer.DrawVoxels(&m_DebugDraw);
        }
    }

    AddRenderInstances(world); // TODO

    m_Stat.PolyCount += m_RenderDef.PolyCount;
    m_Stat.ShadowMapPolyCount += m_RenderDef.ShadowMapPolyCount;

    if (worldRenderView->bDrawDebug)
    {
        //TODO
        //for (auto& it : worldRenderView->m_TerrainViews)
        //{
        //    it.second->DrawDebug(&m_DebugDraw, m_TerrainMesh);
        //}

        m_DebugDraw.EndRenderView();
    }
}

void RenderFrontend::SortRenderInstances()
{
    struct InstanceSortFunction
    {
        bool operator()(RenderInstance const* a, RenderInstance const* b) const
        {
            return a->SortKey < b->SortKey;
        }
    } instanceSortFunction;

    for (RenderViewData* view = m_FrameData.RenderViews; view < &m_FrameData.RenderViews[m_FrameData.NumViews]; view++)
    {
        std::sort(m_FrameData.Instances.Begin() + view->FirstInstance,
                  m_FrameData.Instances.Begin() + (view->FirstInstance + view->InstanceCount),
                  instanceSortFunction);

        std::sort(m_FrameData.TranslucentInstances.Begin() + view->FirstTranslucentInstance,
                  m_FrameData.TranslucentInstances.Begin() + (view->FirstTranslucentInstance + view->TranslucentInstanceCount),
                  instanceSortFunction);
    }
}

void RenderFrontend::SortShadowInstances(LightShadowmap const* shadowMap)
{
    struct ShadowInstanceSortFunction
    {
        bool operator()(ShadowRenderInstance const* a, ShadowRenderInstance const* b)
        {
            return a->SortKey < b->SortKey;
        }
    } shadowInstanceSortFunction;

    std::sort(m_FrameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
              m_FrameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
              shadowInstanceSortFunction);
}

void RenderFrontend::QueryVisiblePrimitives(World* world)
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

    //world->QueryVisiblePrimitives(m_VisPrimitives, &m_VisPass, query);
}

void RenderFrontend::QueryShadowCasters(World* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, TVector<PrimitiveDef*>& Primitives)
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
    //InWorld->QueryVisiblePrimitives(Primitives, nullptr, query);
}

void RenderFrontend::AddRenderInstances(World* world)
{
    HK_PROFILER_EVENT("Add Render Instances");

    RenderViewData* view = m_RenderDef.View;
    //TerrainComponent* terrain;
    //PunctualLightComponent* light;
    //EnvironmentProbe* envProbe;
    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();
    //LightingSystem& lightingSystem = InWorld->LightingSystem;

    m_VisLights.Clear();
    m_VisEnvProbes.Clear();

    world->AddDrawables(m_RenderDef, m_FrameData);

    //for (PrimitiveDef* primitive : m_VisPrimitives)
    //{
        // TODO: Replace upcasting by something better (virtual function?)

        //if (nullptr != (drawable = Upcast<Drawable>(primitive->Owner)))
        //{
        //    AddDrawable(drawable);
        //    continue;
        //}

        //if (nullptr != (terrain = Upcast<TerrainComponent>(primitive->Owner)))
        //{
        //    AddTerrain(terrain);
        //    continue;
        //}

        //if (nullptr != (light = Upcast<PunctualLightComponent>(primitive->Owner)))
        //{
        //    if (!light->IsEnabled())
        //    {
        //        continue;
        //    }

        //    if (m_VisLights.Size() < MAX_LIGHTS)
        //    {
        //        m_VisLights.Add(light);
        //    }
        //    else
        //    {
        //        LOG("MAX_LIGHTS hit\n");
        //    }
        //    continue;
        //}

        //if (nullptr != (envProbe = Upcast<EnvironmentProbe>(primitive->Owner)))
        //{
        //    if (!envProbe->IsEnabled())
        //    {
        //        continue;
        //    }

        //    if (m_VisEnvProbes.Size() < MAX_PROBES)
        //    {
        //        m_VisEnvProbes.Add(envProbe);
        //    }
        //    else
        //    {
        //        LOG("MAX_PROBES hit\n");
        //    }
        //    continue;
        //}

        //LOG("Unhandled primitive\n");
    //}

    // Add directional lights
    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;

    world->AddDirectionalLight(m_RenderDef, m_FrameData);

    m_LightVoxelizer.Reset();

    // Allocate lights
    view->NumPointLights = m_VisLights.Size();
    view->PointLightsStreamSize = sizeof(LightParameters) * view->NumPointLights;
    view->PointLightsStreamHandle = view->PointLightsStreamSize > 0 ? streamedMemory->AllocateConstant(view->PointLightsStreamSize, nullptr) : 0;
    view->PointLights = (LightParameters*)streamedMemory->Map(view->PointLightsStreamHandle);
    view->FirstOmnidirectionalShadowMap = m_FrameData.LightShadowmaps.Size();
    view->NumOmnidirectionalShadowMaps = 0;

    //int maxOmnidirectionalShadowMaps = GameApplication::GetRenderBackend().MaxOmnidirectionalShadowMapsPerView();

    //for (int i = 0; i < view->NumPointLights; i++)
    //{
    //    light = m_VisLights[i];

    //    light->PackLight(view->ViewMatrix, view->PointLights[i]);

    //    if (view->NumOmnidirectionalShadowMaps < maxOmnidirectionalShadowMaps)
    //    {
    //        if (AddLightShadowmap(light, view->PointLights[i].Radius))
    //        {
    //            view->PointLights[i].ShadowmapIndex = view->NumOmnidirectionalShadowMaps;
    //            view->NumOmnidirectionalShadowMaps++;
    //        }
    //        else
    //            view->PointLights[i].ShadowmapIndex = -1;
    //    }
    //    else
    //    {
    //        LOG("maxOmnidirectionalShadowMaps hit\n");
    //    }

    //    PhotometricProfile* profile = light->GetPhotometricProfile();
    //    if (profile)
    //    {
    //        profile->WritePhotometricData(m_PhotometricProfiles, m_FrameNumber);
    //    }

    //    ItemInfo* info = m_LightVoxelizer.AllocItem();
    //    info->Type = ITEM_TYPE_LIGHT;
    //    info->ListIndex = i;

    //    BvAxisAlignedBox const& AABB = light->GetWorldBounds();
    //    info->Mins = AABB.Mins;
    //    info->Maxs = AABB.Maxs;

    //    if (m_LightVoxelizer.IsSSE())
    //    {
    //        info->ClipToBoxMatSSE = light->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
    //    }
    //    else
    //    {
    //        info->ClipToBoxMat = light->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
    //    }
    //}

    // Allocate probes
    view->NumProbes = m_VisEnvProbes.Size();
    view->ProbeStreamSize = sizeof(ProbeParameters) * view->NumProbes;
    view->ProbeStreamHandle = view->ProbeStreamSize > 0 ?
        streamedMemory->AllocateConstant(view->ProbeStreamSize, nullptr) :
        0;
    view->Probes = (ProbeParameters*)streamedMemory->Map(view->ProbeStreamHandle);

    //for (int i = 0; i < view->NumProbes; i++)
    //{
    //    envProbe = m_VisEnvProbes[i];

    //    envProbe->PackProbe(view->ViewMatrix, view->Probes[i]);

    //    ItemInfo* info = m_LightVoxelizer.AllocItem();
    //    info->Type = ITEM_TYPE_PROBE;
    //    info->ListIndex = i;

    //    BvAxisAlignedBox const& AABB = envProbe->GetWorldBounds();
    //    info->Mins = AABB.Mins;
    //    info->Maxs = AABB.Maxs;

    //    if (m_LightVoxelizer.IsSSE())
    //    {
    //        info->ClipToBoxMatSSE = envProbe->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
    //    }
    //    else
    //    {
    //        info->ClipToBoxMat = envProbe->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
    //    }
    //}

    if (!r_FixFrustumClusters)
    {
        m_LightVoxelizer.Voxelize(m_FrameLoop->GetStreamedMemoryGPU(), view);
    }
}
#if 0
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
#endif

#if 0
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

        worldECS->AddShadows(m_RenderDef, m_FrameData, shadowMap);

        for (int n = 0; n < m_ShadowCasters.Size(); n++)
        {
            Drawable* component = m_ShadowCasters[n];

            if (component->CascadeMask == 0)
            {
                continue;
            }

            //switch (component->GetDrawableType())
            //{
            //    case DRAWABLE_STATIC_MESH:
            //        AddShadowmap_StaticMesh(shadowMap, static_cast<MeshComponent*>(component));
            //        break;
            //    default:
            //        break;
            //}

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

        SortShadowInstances(shadowMap);

        if (r_RenderLightPortals)
        {
            // Add light portals
            for (Level* level : InWorld->GetArrayOfLevels())
            {
                LevelLighting* lighting = level->Lighting;

                if (!lighting)
                    continue;

                TVector<LightPortalDef> const& lightPortals = lighting->GetLightPortals();

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
#endif

bool RenderFrontend::AddLightShadowmap(PunctualLightComponent* Light, float Radius)
{
    #if 0
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
        QueryShadowCasters(world, lightViewProjection, lightPos, Float3x3(cubeFaceMatrices[faceIndex]), m_VisPrimitives);

        LightShadowmap* shadowMap = &m_FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount = 0;
        shadowMap->LightPosition = lightPos;

        worldECS->AddShadows(m_RenderDef, m_FrameData, shadowMap);

        for (PrimitiveDef* primitive : m_VisPrimitives)
        {
            // TODO: Replace upcasting by something better (virtual function?)

            if (nullptr != (drawable = Upcast<Drawable>(primitive->Owner)))
            {
                drawable->CascadeMask = 1 << faceIndex;

                //switch (drawable->GetDrawableType())
                //{
                //    case DRAWABLE_STATIC_MESH:
                //        AddShadowmap_StaticMesh(shadowMap, static_cast<MeshComponent*>(drawable));
                //        break;
                //    default:
                //        break;
                //}

#if 0
                m_DebugDraw.SetDepthTest( false );
                m_DebugDraw.SetColor( Color4( 0, 1, 0, 1 ) );
                m_DebugDraw.DrawAABB( drawable->GetWorldBounds() );
#endif

                drawable->CascadeMask = 0;
            }
        }

        SortShadowInstances(shadowMap);

        totalInstances += shadowMap->ShadowInstanceCount;
    }

    if (totalInstances == 0)
    {
        m_FrameData.LightShadowmaps.Resize(m_FrameData.LightShadowmaps.Size() - 6);
        return false;
    }

    return true;
    #else
    return false;
    #endif
}

HK_NAMESPACE_END
