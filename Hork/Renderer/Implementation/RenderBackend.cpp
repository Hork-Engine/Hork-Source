/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "RenderLocal.h"
#include "CanvasRenderer.h"
#include "FrameRenderer.h"

#include <Hork/Core/ConsoleVar.h>
#include <Hork/Core/ScopedTimer.h>

#include <Hork/Core/WindowsDefs.h>
#include <Hork/Core/Logger.h>
#include <Hork/Core/Profiler.h>

#include <Hork/Image/ImageEncoders.h>

#include <Hork/RenderUtils/BRDFGenerator.h>
#include <Hork/MaterialGraph/MaterialCompiler.h>

HK_NAMESPACE_BEGIN

using namespace RHI;

ConsoleVar r_FrameGraphDebug("r_FrameGraphDebug"_s, "0"_s);
ConsoleVar r_DebugRenderMode("r_DebugRenderMode"_s, "0"_s, CVAR_CHEAT);
ConsoleVar r_BloomScale("r_BloomScale"_s, "1"_s);
ConsoleVar r_Bloom("r_Bloom"_s, "1"_s);
ConsoleVar r_BloomParam0("r_BloomParam0"_s, "0.5"_s);
ConsoleVar r_BloomParam1("r_BloomParam1"_s, "0.3"_s);
ConsoleVar r_BloomParam2("r_BloomParam2"_s, "0.04"_s);
ConsoleVar r_BloomParam3("r_BloomParam3"_s, "0.01"_s);
ConsoleVar r_ExposureScale("r_ExposureScale"_s, "1.0"_s);
ConsoleVar r_Brightness("r_Brightness"_s, "1"_s);
ConsoleVar r_TessellationLevel("r_TessellationLevel"_s, "0.05"_s);
ConsoleVar r_SSLR("r_SSLR"_s, "1"_s, 0, "Required to rebuld materials to apply"_s);
ConsoleVar r_SSLRMaxDist("r_SSLRMaxDist"_s, "10"_s);
ConsoleVar r_SSLRSampleOffset("r_SSLRSampleOffset"_s, "0.1"_s);
ConsoleVar r_HBAO("r_HBAO"_s, "1"_s, 0, "Required to rebuld materials to apply"_s);
ConsoleVar r_ShowGPUTime("r_ShowGPUTime"_s, "0"_s);

void TestVT();

RenderBackend::RenderBackend(RHI::IDevice* device)
{
    LOG("Initializing render backend...\n");

    GDevice = device;
    rcmd = GDevice->GetImmediateContext();
    rtbl = rcmd->GetRootResourceTable();

    m_FrameGraph = MakeRef<FrameGraph>(GDevice);

    m_FrameRenderer = MakeRef<FrameRenderer>();
    m_CanvasRenderer = MakeRef<CanvasRenderer>();

    GCircularBuffer = MakeRef<CircularBuffer>(2 * 1024 * 1024); // 2MB
    //GFrameConstantBuffer = MakeRef< FrameConstantBuffer >( 2 * 1024 * 1024 ); // 2MB

    //#define QUERY_TIMESTAMP

    QueryPoolDesc timeQueryCI;
#ifdef QUERY_TIMESTAMP
    timeQueryCI.QueryType = QUERY_TYPE_TIMESTAMP;
    timeQueryCI.PoolSize  = 3;
    GDevice->CreateQueryPool(timeQueryCI, &TimeStamp1);
    GDevice->CreateQueryPool(timeQueryCI, &TimeStamp2);
#else
    timeQueryCI.QueryType = QUERY_TYPE_TIME_ELAPSED;
    timeQueryCI.PoolSize  = 3;
    GDevice->CreateQueryPool(timeQueryCI, &m_TimeQuery);
#endif

    // Create white texture
    {
        GDevice->CreateTexture(TextureDesc()
                                   .SetFormat(TEXTURE_FORMAT_RGBA8_UNORM)
                                   .SetResolution(TextureResolution2D(1, 1))
                                   .SetBindFlags(BIND_SHADER_RESOURCE),
                               &GWhiteTexture);
        TextureRect rect  = {};
        rect.Dimension.X   = 1;
        rect.Dimension.Y   = 1;
        rect.Dimension.Z   = 1;
        const byte data[4] = {0xff, 0xff, 0xff, 0xff};
        GWhiteTexture->WriteRect(rect, sizeof(data), 4, data);
        GWhiteTexture->SetDebugName("White texture");
    }

    // Create cluster lookup 3D texture
    GDevice->CreateTexture(TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RG32_UINT)
                               .SetResolution(TextureResolution3D(MAX_FRUSTUM_CLUSTERS_X,
                                                                   MAX_FRUSTUM_CLUSTERS_Y,
                                                                   MAX_FRUSTUM_CLUSTERS_Z))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &GClusterLookup);
    GClusterLookup->SetDebugName("Cluster Lookup");


    m_FeedbackAnalyzerVT = MakeRef<VirtualTextureFeedbackAnalyzer>(GDevice);
    GFeedbackAnalyzerVT = m_FeedbackAnalyzerVT;

    {
        BRDFGenerator generator(GDevice);
        generator.Render(&GLookupBRDF);
    }

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////

    #if 0
    VTCacheLayerInfo layer;
    layer.TextureFormat   = TEXTURE_FORMAT_SRGB8;
    layer.UploadFormat    = FORMAT_UBYTE3;
    layer.PageSizeInBytes = 128 * 128 * 3;

    VTCacheCreateInfo createInfo;
    createInfo.PageCacheCapacityX = 32;
    createInfo.PageCacheCapacityY = 32;
    createInfo.PageResolutionB    = 128;
    createInfo.NumLayers          = 1;
    createInfo.pLayers            = &layer;
    PhysCacheVT                   = MakeRef<VirtualTextureCache>(createInfo);

    GPhysCacheVT = PhysCacheVT;

    #endif

    //::TestVT();
    //PhysCacheVT->CreateTexture( "Test.vt3", &TestVT );

//#define SPARSE_TEXTURE_TEST
#ifdef SPARSE_TEXTURE_TEST
#    if 0
    {
    SSparseTextureCreateInfo sparseTextureCI = MakeSparseTexture( TEXTURE_FORMAT_RGBA8_UNORM, TextureResolution2D( 2048, 2048 ) );
    Ref< ISparseTexture > sparseTexture;
    GDevice->CreateSparseTexture( sparseTextureCI, &sparseTexture );

    int pageSizeX = sparseTexture->GetPageSizeX();
    int pageSizeY = sparseTexture->GetPageSizeY();
    int sz = pageSizeX*pageSizeY*4;

    byte * mem = (byte*)StackAlloc( sz );
    Core::ZeroMem( mem, sz );

    sparseTexture->CommitPage( 0,0,0,0, FORMAT_UBYTE4, sz, 1, mem );
    }

    {
    int numPageSizes = 0;
    GDevice->EnumerateSparseTexturePageSize( SPARSE_TEXTURE_2D_ARRAY, TEXTURE_FORMAT_RGBA8_UNORM, &numPageSizes, nullptr, nullptr, nullptr );
    Vector<int> pageSizeX; pageSizeX.Resize( numPageSizes );
    Vector<int> pageSizeY; pageSizeY.Resize( numPageSizes );
    Vector<int> pageSizeZ; pageSizeZ.Resize( numPageSizes );
    GDevice->EnumerateSparseTexturePageSize( SPARSE_TEXTURE_2D_ARRAY, TEXTURE_FORMAT_RGBA8_UNORM, &numPageSizes, pageSizeX.ToPtr(), pageSizeY.ToPtr(), pageSizeZ.ToPtr() );
    for ( int i = 0 ; i < numPageSizes ; i++ ) {
        LOG( "Sparse page size {} {} {}\n", pageSizeX[i], pageSizeY[i], pageSizeZ[i] );
    }
    }
#    endif
    int maxLayers = GDevice->GetDeviceCaps(DEVICE_CAPS_MAX_TEXTURE_LAYERS);
    int texSize   = 1024;
    int n         = texSize;
    int numLods   = 1;
    while (n >>= 1)
    {
        numLods++;
    }
    SSparseTextureCreateInfo sparseTextureCI = MakeSparseTexture(TEXTURE_FORMAT_RGBA8_UNORM, TextureResolution2DArray(texSize, texSize, maxLayers), TextureSwizzle(), numLods);

    Ref<ISparseTexture> sparseTexture;
    GDevice->CreateSparseTexture(sparseTextureCI, &sparseTexture);

#    if 0
    int pageSizeX = sparseTexture->GetPageSizeX();
    int pageSizeY = sparseTexture->GetPageSizeY();
    int sz = pageSizeX*pageSizeY*4;

    byte * mem = (byte*)StackAlloc( sz );
    Core::ZeroMem( mem, sz );

    sparseTexture->CommitPage( 0, 0, 0, 0, FORMAT_UBYTE4, sz, 1, mem );
#    else
    int sz = texSize * texSize * 4;

    byte* mem = (byte*)malloc(sz);
    Core::ZeroMem(mem, sz);

    LOG("\tTotal available after create: {} Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10);

    for (int i = 0; i < 10; i++)
    {
        TextureRect rect;
        rect.Offset.Lod = 0;
        rect.Offset.X = 0;
        rect.Offset.Y = 0;
        rect.Offset.Z = 0;
        rect.Dimension.X = texSize;
        rect.Dimension.Y = texSize;
        rect.Dimension.Z = 1;
        sparseTexture->CommitRect(rect, FORMAT_UBYTE4, sz, 1, mem);
        LOG("\tTotal available after commit: {} Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10);
        sparseTexture->UncommitRect(rect);
        LOG("\tTotal available after uncommit: {} Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10);
    }
    free(mem);
#    endif
#endif

    m_TerrainDepthPipeline = CreateTerrainMaterialDepth(GDevice);
    GTerrainDepthPipeline = m_TerrainDepthPipeline;

    m_TerrainLightPipeline = CreateTerrainMaterialLight(GDevice);
    GTerrainLightPipeline = m_TerrainLightPipeline;

    m_TerrainWireframePipeline = CreateTerrainMaterialWireframe(GDevice);
    GTerrainWireframePipeline = m_TerrainWireframePipeline;
}

RenderBackend::~RenderBackend()
{
    LOG("Deinitializing render backend...\n");

    //VirtualTexture * vt = TestVT.RawPtr();
    //TestVT.Reset();
    m_PhysCacheVT.Reset();
    m_FeedbackAnalyzerVT.Reset();
    //LOG( "VT ref count {}\n", vt->GetRefCount() );

    GCircularBuffer.Reset();
    GWhiteTexture.Reset();
    GLookupBRDF.Reset();
    GClusterLookup.Reset();
    GClusterItemTBO.Reset();
    GClusterItemBuffer.Reset();
}

#if 0
void RenderBackend::InitializeBuffer( Ref< IBuffer > * ppBuffer, size_t _SizeInBytes )
{
    BufferDesc bufferCI = {};

    bufferCI.SizeInBytes = _SizeInBytes;

    const bool bDynamicStorage = false;
    if ( bDynamicStorage ) {
#    if 1
        // Seems to be faster
        bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;
#    else
        bufferCI.MutableClientAccess = MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
        bufferCI.MutableUsage = MUTABLE_STORAGE_STREAM;
        bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
        bufferCI.bImmutableStorage = false;
#    endif

        GDevice->CreateBuffer( bufferCI, nullptr, ppBuffer );
    }
    else {
#    if 1
        // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering (tested on NVidia GeForce GTX 770)
        bufferCI.MutableClientAccess = MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
        bufferCI.MutableUsage = MUTABLE_STORAGE_STATIC;
        bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
        bufferCI.bImmutableStorage = false;
#    else
        bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;
#    endif

        GDevice->CreateBuffer( bufferCI, nullptr, ppBuffer );
    }
}
#endif

int RenderBackend::ClusterPackedIndicesAlignment() const
{
    return GDevice->GetDeviceCaps(DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT);
}

int RenderBackend::MaxOmnidirectionalShadowMapsPerView() const
{
    return m_FrameRenderer->GetOmniShadowMapPool().GetSize();
}

void RenderBackend::RenderFrame(StreamedMemoryGPU* streamedMemory, ITexture* backBuffer, RenderFrameData const* frameData, CanvasDrawData const* canvasData)
{
    HK_PROFILER_EVENT("Render Backend");

    static int timeQueryFrame = 0;

    GStreamedMemory = streamedMemory;
    GStreamBuffer = streamedMemory->GetBufferGPU();

    // Create item buffer
    if (!GClusterItemTBO)
    {
#if 0
        // FIXME: Use SSBO?
        BufferDesc bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = MAX_TOTAL_CLUSTER_ITEMS * sizeof( ClusterPackedIndex );
        GDevice->CreateBuffer( bufferCI, nullptr, &GClusterItemBuffer );

        GClusterItemBuffer->SetDebugName( "Cluster item buffer" );

        BufferViewDesc bufferViewCI = {};
        bufferViewCI.Format = BUFFER_VIEW_PIXEL_FORMAT_R32UI;
        GClusterItemBuffer->CreateView( bufferViewCI, &GClusterItemTBO );
#else
        BufferViewDesc bufferViewCI = {};
        bufferViewCI.Format          = BUFFER_VIEW_PIXEL_FORMAT_R32UI;

        GStreamBuffer->CreateView(bufferViewCI, &GClusterItemTBO);
#endif
    }

    if (r_ShowGPUTime)
    {
#ifdef QUERY_TIMESTAMP
        rcmd->RecordTimeStamp(TimeStamp1, timeQueryFrame);
#else
        rcmd->BeginQuery(m_TimeQuery, timeQueryFrame);

        timeQueryFrame = (timeQueryFrame + 1) % m_TimeQuery->GetPoolSize();
#endif
    }

    GFrameData = frameData;
    GCanvasData = canvasData;

    //FrameGraph->Clear();

    //rcmd->SetSwapChainResolution( GFrameData->CanvasWidth, GFrameData->CanvasHeight );

    // Update cache at beggining of the frame to give more time for stream thread
    if (m_PhysCacheVT)
        m_PhysCacheVT->Update();

    m_FeedbackAnalyzerVT->Begin(streamedMemory, GStreamBuffer, rtbl);

    // TODO: Bind virtual textures in one place
    m_FeedbackAnalyzerVT->BindTexture(0, m_TestVT);

    GRenderViewContext.Clear();
    GRenderViewContext.Resize(GFrameData->NumViews);

    for (int i = 0; i < GFrameData->NumViews; i++)
    {
        RenderViewData* pRenderView = &GFrameData->RenderViews[i];

        if (pRenderView->Width == 0 || pRenderView->Height == 0)
            continue;

        RenderView(i, pRenderView);

        m_FrameGraph->Build();
        //m_FrameGraph->ExportGraphviz("frame.graphviz");
        rcmd->ExecuteFrameGraph(m_FrameGraph);
        m_FrameGraph->Clear();
    }

    m_CanvasRenderer->Render(*m_FrameGraph, backBuffer);

    m_FrameGraph->Build();
    //m_FrameGraph->ExportGraphviz("frame.graphviz");
    rcmd->ExecuteFrameGraph(m_FrameGraph);

    if (r_FrameGraphDebug)
    {
        m_FrameGraph->Debug();
    }

    m_FrameGraph->Clear();

    m_FeedbackAnalyzerVT->End();

    if (r_ShowGPUTime)
    {
#ifdef QUERY_TIMESTAMP
        rcmd->RecordTimeStamp(TimeStamp2, timeQueryFrame);

        timeQueryFrame = (timeQueryFrame + 1) % TimeStamp1->GetPoolSize();

        uint64_t timeStamp1 = 0;
        uint64_t timeStamp2 = 0;
        rcmd->GetQueryPoolResult64(TimeStamp2, timeQueryFrame, &timeStamp2, QUERY_RESULT_WAIT_BIT);
        rcmd->GetQueryPoolResult64(TimeStamp1, timeQueryFrame, &timeStamp1, QUERY_RESULT_WAIT_BIT);

        LOG("GPU time {} ms\n", (double)(timeStamp2 - timeStamp1) / 1000000.0);
#else
        rcmd->EndQuery(m_TimeQuery);

        uint64_t timeQueryResult = 0;
        rcmd->GetQueryPoolResult64(m_TimeQuery, timeQueryFrame, &timeQueryResult, QUERY_RESULT_WAIT_BIT);

        LOG("GPU time {} ms\n", (double)timeQueryResult / 1000000.0);
#endif
    }

    GStreamedMemory = nullptr;
    GStreamBuffer = nullptr;
}

void RenderBackend::SetViewConstants(int ViewportIndex)
{
    size_t offset = GStreamedMemory->AllocateConstant(sizeof(ViewConstantBuffer));

    ViewConstantBuffer* pViewCBuf = (ViewConstantBuffer*)GStreamedMemory->Map(offset);

    pViewCBuf->ViewProjection          = GRenderView->ViewProjection;
    pViewCBuf->ProjectionMatrix        = GRenderView->ProjectionMatrix;
    pViewCBuf->InverseProjectionMatrix = GRenderView->InverseProjectionMatrix;

    pViewCBuf->InverseViewMatrix = GRenderView->ViewSpaceToWorldSpace;

    // Reprojection from viewspace to previous frame viewspace coordinates:
    // ViewspaceReprojection = WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    pViewCBuf->ViewspaceReprojection = GRenderView->ViewMatrixP * GRenderView->ViewSpaceToWorldSpace;

    // Reprojection from viewspace to previous frame projected coordinates:
    // ReprojectionMatrix = ProjectionMatrixPrevFrame * WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    pViewCBuf->ReprojectionMatrix = GRenderView->ProjectionMatrixP * pViewCBuf->ViewspaceReprojection;

    pViewCBuf->WorldNormalToViewSpace[0].X = GRenderView->NormalToViewMatrix[0][0];
    pViewCBuf->WorldNormalToViewSpace[0].Y = GRenderView->NormalToViewMatrix[1][0];
    pViewCBuf->WorldNormalToViewSpace[0].Z = GRenderView->NormalToViewMatrix[2][0];
    pViewCBuf->WorldNormalToViewSpace[0].W = 0;

    pViewCBuf->WorldNormalToViewSpace[1].X = GRenderView->NormalToViewMatrix[0][1];
    pViewCBuf->WorldNormalToViewSpace[1].Y = GRenderView->NormalToViewMatrix[1][1];
    pViewCBuf->WorldNormalToViewSpace[1].Z = GRenderView->NormalToViewMatrix[2][1];
    pViewCBuf->WorldNormalToViewSpace[1].W = 0;

    pViewCBuf->WorldNormalToViewSpace[2].X = GRenderView->NormalToViewMatrix[0][2];
    pViewCBuf->WorldNormalToViewSpace[2].Y = GRenderView->NormalToViewMatrix[1][2];
    pViewCBuf->WorldNormalToViewSpace[2].Z = GRenderView->NormalToViewMatrix[2][2];
    pViewCBuf->WorldNormalToViewSpace[2].W = 0;

    pViewCBuf->InvViewportSize.X = 1.0f / GRenderView->Width;
    pViewCBuf->InvViewportSize.Y = 1.0f / GRenderView->Height;
    pViewCBuf->ZNear             = GRenderView->ViewZNear;
    pViewCBuf->ZFar              = GRenderView->ViewZFar;

    if (GRenderView->bPerspective)
    {
        pViewCBuf->ProjectionInfo.X = -2.0f / GRenderView->ProjectionMatrix[0][0];                                         // (x) * (R - L)/N
        pViewCBuf->ProjectionInfo.Y = 2.0f / GRenderView->ProjectionMatrix[1][1];                                          // (y) * (T - B)/N
        pViewCBuf->ProjectionInfo.Z = (1.0f - GRenderView->ProjectionMatrix[2][0]) / GRenderView->ProjectionMatrix[0][0];  // L/N
        pViewCBuf->ProjectionInfo.W = -(1.0f + GRenderView->ProjectionMatrix[2][1]) / GRenderView->ProjectionMatrix[1][1]; // B/N
    }
    else
    {
        pViewCBuf->ProjectionInfo.X = 2.0f / GRenderView->ProjectionMatrix[0][0];                                          // (x) * R - L
        pViewCBuf->ProjectionInfo.Y = -2.0f / GRenderView->ProjectionMatrix[1][1];                                         // (y) * T - B
        pViewCBuf->ProjectionInfo.Z = -(1.0f + GRenderView->ProjectionMatrix[3][0]) / GRenderView->ProjectionMatrix[0][0]; // L
        pViewCBuf->ProjectionInfo.W = (1.0f - GRenderView->ProjectionMatrix[3][1]) / GRenderView->ProjectionMatrix[1][1];  // B
    }

    pViewCBuf->GameRunningTimeSeconds = GRenderView->GameRunningTimeSeconds;
    pViewCBuf->GameplayTimeSeconds    = GRenderView->GameplayTimeSeconds;

    pViewCBuf->WorldAmbient = GRenderView->WorldAmbient;
    pViewCBuf->GlobalIrradianceMap = GRenderView->GlobalIrradianceMap;
    pViewCBuf->GlobalReflectionMap = GRenderView->GlobalReflectionMap;

    // TODO
    pViewCBuf->DynamicResolutionRatioX  = 1; //(float)GRenderView->Width / GFrameData->RenderTargetMaxWidth;
    pViewCBuf->DynamicResolutionRatioY  = 1; //(float)GRenderView->Height / GFrameData->RenderTargetMaxHeight;
    pViewCBuf->DynamicResolutionRatioPX = 1; //(float)GRenderView->WidthP / GFrameData->RenderTargetMaxWidthP;
    pViewCBuf->DynamicResolutionRatioPY = 1; //(float)GRenderView->HeightP / GFrameData->RenderTargetMaxHeightP;

    pViewCBuf->FeedbackBufferResolutionRatio = GRenderView->VTFeedback->GetResolutionRatio();

    if (m_PhysCacheVT)
    {
        pViewCBuf->VTPageCacheCapacity.X           = (float)m_PhysCacheVT->GetPageCacheCapacityX();
        pViewCBuf->VTPageCacheCapacity.Y           = (float)m_PhysCacheVT->GetPageCacheCapacityY();
        pViewCBuf->VTPageTranslationOffsetAndScale = m_PhysCacheVT->GetPageTranslationOffsetAndScale();
    }
    else
    {
        pViewCBuf->VTPageCacheCapacity.X           = 0;
        pViewCBuf->VTPageCacheCapacity.Y           = 0;
        pViewCBuf->VTPageTranslationOffsetAndScale = {0.0f, 0.0f, 1.0f, 1.0f};
    }

    pViewCBuf->ViewPosition = GRenderView->ViewPosition;
    pViewCBuf->TimeDelta    = GRenderView->GameplayTimeStep;

    pViewCBuf->PostprocessBloomMix = Float4(r_BloomParam0.GetFloat(),
                                            r_BloomParam1.GetFloat(),
                                            r_BloomParam2.GetFloat(),
                                            r_BloomParam3.GetFloat()) * r_BloomScale.GetFloat();

    pViewCBuf->BloomEnabled                = r_Bloom;                   // TODO: Get from GRenderView
    pViewCBuf->ToneMappingExposure         = GRenderView->Exposure * r_ExposureScale.GetFloat();
    pViewCBuf->ColorGrading                = GRenderView->CurrentColorGradingLUT ? 1.0f : 0.0f;
    pViewCBuf->FXAA                        = GRenderView->AntialiasingType == ANTIALIASING_FXAA;
    pViewCBuf->VignetteColorIntensity      = GRenderView->VignetteColorIntensity;
    pViewCBuf->VignetteOuterRadiusSqr      = GRenderView->VignetteOuterRadiusSqr;
    pViewCBuf->VignetteInnerRadiusSqr      = GRenderView->VignetteInnerRadiusSqr;
    pViewCBuf->ColorGradingAdaptationSpeed = GRenderView->ColorGradingAdaptationSpeed;
    pViewCBuf->ViewBrightness              = Math::Saturate(r_Brightness.GetFloat());

    pViewCBuf->SSLRSampleOffset  = r_SSLRSampleOffset.GetFloat();
    pViewCBuf->SSLRMaxDist       = r_SSLRMaxDist.GetFloat();
    pViewCBuf->IsPerspective     = float(GRenderView->bPerspective);
    pViewCBuf->TessellationLevel = r_TessellationLevel.GetFloat() * Math::Lerp((float)GRenderView->Width, (float)GRenderView->Height, 0.5f);

    pViewCBuf->DebugMode = r_DebugRenderMode.GetInteger();

    pViewCBuf->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //LOG( "GRenderView->FirstDirectionalLight: {}\n", GRenderView->FirstDirectionalLight );

    for (int i = 0; i < GRenderView->NumDirectionalLights; i++)
    {
        DirectionalLightInstance* light = GFrameData->DirectionalLights[GRenderView->FirstDirectionalLight + i];

        pViewCBuf->LightDirs[i]          = Float4(GRenderView->NormalToViewMatrix * (light->Matrix[2]), 0.0f);
        pViewCBuf->LightColors[i]        = light->ColorAndAmbientIntensity;
        pViewCBuf->LightParameters[i][0] = light->RenderMask;
        pViewCBuf->LightParameters[i][1] = light->FirstCascade;
        pViewCBuf->LightParameters[i][2] = light->NumCascades;
    }

    GRenderViewContext[ViewportIndex].ViewConstantBufferBindingBindingOffset = offset;
    GRenderViewContext[ViewportIndex].ViewConstantBufferBindingBindingSize  = sizeof(*pViewCBuf);
    rtbl->BindBuffer(0, GStreamBuffer, GRenderViewContext[ViewportIndex].ViewConstantBufferBindingBindingOffset, GRenderViewContext[ViewportIndex].ViewConstantBufferBindingBindingSize);
}

void RenderBackend::UploadShaderResources(int ViewportIndex)
{
    SetViewConstants(ViewportIndex);

    // Bind light buffer
    rtbl->BindBuffer(4, GStreamBuffer, GRenderView->PointLightsStreamHandle, GRenderView->PointLightsStreamSize);

    // Bind IBL buffer
    rtbl->BindBuffer(5, GStreamBuffer, GRenderView->ProbeStreamHandle, GRenderView->ProbeStreamSize);

    // Copy cluster data

#if 1
    // Perform copy from stream buffer on GPU side
    TextureRect rect = {};
    rect.Dimension.X  = MAX_FRUSTUM_CLUSTERS_X;
    rect.Dimension.Y  = MAX_FRUSTUM_CLUSTERS_Y;
    rect.Dimension.Z  = MAX_FRUSTUM_CLUSTERS_Z;
    rcmd->CopyBufferToTexture(GStreamBuffer, GClusterLookup, rect, FORMAT_UINT2, 0, GRenderView->ClusterLookupStreamHandle, 1);
#else
    GClusterLookup->Write(0,
                          FORMAT_UINT2,
                          sizeof(ClusterHeader) * MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y * MAX_FRUSTUM_CLUSTERS_Z,
                          1,
                          GRenderView->LightData.ClusterLookup);
#endif

#if 1
    // Perform copy from stream buffer on GPU side
    if (GRenderView->ClusterPackedIndexCount > 0)
    {
#    if 0
        BufferCopy range;
        range.SrcOffset = GRenderView->ClusterPackedIndicesStreamHandle;
        range.DstOffset = 0;
        range.SizeInBytes = sizeof( ClusterPackedIndex ) * GRenderView->ClusterPackedIndexCount;
        rcmd->CopyBufferRange( GStreamBuffer, GClusterItemBuffer, 1, &range );
#    else
        size_t offset = GRenderView->ClusterPackedIndicesStreamHandle;
        size_t sizeInBytes = sizeof(ClusterPackedIndex) * GRenderView->ClusterPackedIndexCount;
        GClusterItemTBO->SetRange(offset, sizeInBytes);
#    endif
    }
#else
    GClusterItemBuffer->WriteRange(0,
                                   sizeof(SClusterItemOffset) * GRenderView->LightData.NumClusterItems,
                                   GRenderView->LightData.ItemBuffer);
#endif
}

void RenderBackend::RenderView(int ViewportIndex, RenderViewData* pRenderView)
{
    HK_ASSERT(pRenderView->Width > 0);
    HK_ASSERT(pRenderView->Height > 0);

    GRenderView            = pRenderView;
    GRenderViewArea.X      = 0;
    GRenderViewArea.Y      = 0;
    GRenderViewArea.Width  = pRenderView->Width;
    GRenderViewArea.Height = pRenderView->Height;

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    FGCustomTask&       task       = m_FrameGraph->AddTask<FGCustomTask>("Setup render view");
    FGBufferViewProxy* bufferView = m_FrameGraph->AddExternalResource<FGBufferViewProxy>("hack hack", GClusterItemTBO);
    task.AddResource(bufferView, FG_RESOURCE_ACCESS_WRITE);
    task.SetFunction([=](FGCustomTaskContext const& Task)
                     {
                         IImmediateContext* immediateCtx = Task.pImmediateContext;
                         GRenderView = pRenderView;
                         GRenderViewArea.X      = 0;
                         GRenderViewArea.Y      = 0;
                         GRenderViewArea.Width  = pRenderView->Width;
                         GRenderViewArea.Height = pRenderView->Height;

                         UploadShaderResources(ViewportIndex);

                         immediateCtx->BindResourceTable(rtbl);
        });
    

    bool bVirtualTexturing = m_FeedbackAnalyzerVT->HasBindings();

    // !!!!!!!!!!! FIXME: move outside of framegraph filling
    if (bVirtualTexturing)
    {
        pRenderView->VTFeedback->Begin(pRenderView->Width, pRenderView->Height);
    }

    m_FrameRenderer->Render(*m_FrameGraph, bVirtualTexturing, m_PhysCacheVT);

    // !!!!!!!!!!! FIXME: move outside of framegraph filling
    if (bVirtualTexturing)
    {
        int         FeedbackSize;
        const void* FeedbackData;
        pRenderView->VTFeedback->End(&FeedbackSize, &FeedbackData);

        m_FeedbackAnalyzerVT->AddFeedbackData(FeedbackSize, FeedbackData);
    }
}

HK_NAMESPACE_END
