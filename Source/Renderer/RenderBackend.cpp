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

#include "RenderLocal.h"
#include "Material.h"
#include "CanvasRenderer.h"
#include "VT/VirtualTextureFeedback.h"
#include "IrradianceGenerator.h"
#include "EnvProbeGenerator.h"
#include "CubemapGenerator.h"
#include "AtmosphereRenderer.h"
#include "BRDFGenerator.h"
#include "VXGIVoxelizer.h"

#include <Core/ConsoleVar.h>
#include <Core/ScopedTimer.h>

#include <Platform/WindowsDefs.h>
#include <Platform/Logger.h>
#include <Core/Image.h>

#include <SDL.h>

using namespace RenderCore;

AConsoleVar r_FrameGraphDebug(_CTS("r_FrameGraphDebug"), _CTS("0"));
AConsoleVar r_RenderSnapshot(_CTS("r_RenderSnapshot"), _CTS("0"), CVAR_CHEAT);
AConsoleVar r_DebugRenderMode(_CTS("r_DebugRenderMode"), _CTS("0"), CVAR_CHEAT);
AConsoleVar r_BloomScale(_CTS("r_BloomScale"), _CTS("1"));
AConsoleVar r_Bloom(_CTS("r_Bloom"), _CTS("1"));
AConsoleVar r_BloomParam0(_CTS("r_BloomParam0"), _CTS("0.5"));
AConsoleVar r_BloomParam1(_CTS("r_BloomParam1"), _CTS("0.3"));
AConsoleVar r_BloomParam2(_CTS("r_BloomParam2"), _CTS("0.04"));
AConsoleVar r_BloomParam3(_CTS("r_BloomParam3"), _CTS("0.01"));
AConsoleVar r_ToneExposure(_CTS("r_ToneExposure"), _CTS("0.4"));
AConsoleVar r_Brightness(_CTS("r_Brightness"), _CTS("1"));
AConsoleVar r_TessellationLevel(_CTS("r_TessellationLevel"), _CTS("0.05"));
AConsoleVar r_MotionBlur(_CTS("r_MotionBlur"), _CTS("1"));
AConsoleVar r_SSLR(_CTS("r_SSLR"), _CTS("1"), 0, _CTS("Required to rebuld materials to apply"));
AConsoleVar r_SSLRMaxDist(_CTS("r_SSLRMaxDist"), _CTS("10"));
AConsoleVar r_SSLRSampleOffset(_CTS("r_SSLRSampleOffset"), _CTS("0.1"));
AConsoleVar r_HBAO(_CTS("r_HBAO"), _CTS("1"), 0, _CTS("Required to rebuld materials to apply"));
AConsoleVar r_FXAA(_CTS("r_FXAA"), _CTS("1"));
AConsoleVar r_ShowGPUTime(_CTS("r_ShowGPUTime"), _CTS("0"));

void TestVT();

static void LoadSPIRV(void** BinaryCode, size_t* BinarySize)
{
    // TODO

    *BinaryCode = nullptr;
    *BinarySize = 0;
}

ARenderBackend::ARenderBackend(RenderCore::IDevice* pDevice)
{
    GLogger.Printf("Initializing render backend...\n");

    GDevice = pDevice;
    rcmd = GDevice->GetImmediateContext();
    rtbl = rcmd->GetRootResourceTable();

    InitMaterialSamplers();

    FrameGraph = MakeRef<AFrameGraph>(GDevice);

    FrameRenderer  = MakeRef<AFrameRenderer>();
    CanvasRenderer = MakeRef<ACanvasRenderer>();

    GCircularBuffer = MakeRef<ACircularBuffer>(2 * 1024 * 1024); // 2MB
    //GFrameConstantBuffer = MakeRef< AFrameConstantBuffer >( 2 * 1024 * 1024 ); // 2MB

    //#define QUERY_TIMESTAMP

    SQueryPoolDesc timeQueryCI;
#ifdef QUERY_TIMESTAMP
    timeQueryCI.QueryType = QUERY_TYPE_TIMESTAMP;
    timeQueryCI.PoolSize  = 3;
    GDevice->CreateQueryPool(timeQueryCI, &TimeStamp1);
    GDevice->CreateQueryPool(timeQueryCI, &TimeStamp2);
#else
    timeQueryCI.QueryType = QUERY_TYPE_TIME_ELAPSED;
    timeQueryCI.PoolSize  = 3;
    GDevice->CreateQueryPool(timeQueryCI, &TimeQuery);
#endif

    // Create sphere mesh for cubemap rendering
    GSphereMesh = MakeRef<ASphereMesh>();

    // Create screen aligned quad
    {
        constexpr Float2 saqVertices[4] = {
            {Float2(-1.0f, 1.0f)},
            {Float2(1.0f, 1.0f)},
            {Float2(-1.0f, -1.0f)},
            {Float2(1.0f, -1.0f)}};

        SBufferDesc bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.SizeInBytes       = sizeof(saqVertices);
        GDevice->CreateBuffer(bufferCI, saqVertices, &GSaq);

        GSaq->SetDebugName("Screen aligned quad");
    }

    // Create white texture
    {
        GDevice->CreateTexture(STextureDesc()
                                   .SetFormat(TEXTURE_FORMAT_RGBA8)
                                   .SetResolution(STextureResolution2D(1, 1))
                                   .SetBindFlags(BIND_SHADER_RESOURCE),
                               &GWhiteTexture);
        STextureRect rect  = {};
        rect.Dimension.X   = 1;
        rect.Dimension.Y   = 1;
        rect.Dimension.Z   = 1;
        const byte data[4] = {0xff, 0xff, 0xff, 0xff};
        GWhiteTexture->WriteRect(rect, FORMAT_UBYTE4, sizeof(data), 4, data);
    }

    // Create cluster lookup 3D texture
    GDevice->CreateTexture(STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RG32UI)
                               .SetResolution(STextureResolution3D(MAX_FRUSTUM_CLUSTERS_X,
                                                                   MAX_FRUSTUM_CLUSTERS_Y,
                                                                   MAX_FRUSTUM_CLUSTERS_Z))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &GClusterLookup);


    FeedbackAnalyzerVT  = MakeRef<AVirtualTextureFeedbackAnalyzer>();
    GFeedbackAnalyzerVT = FeedbackAnalyzerVT;

    {
        ABRDFGenerator generator;
        generator.Render(&GLookupBRDF);
    }

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////
    #if 0
    {
        VXGIVoxelizer vox;
        vox.Render();
    }

    TRef<ITexture> skybox;
    {
        AAtmosphereRenderer atmosphereRenderer;
        atmosphereRenderer.Render(512, Float3(-0.5f, -2, -10), &skybox);
#if 0
        STextureRect rect = {};
        rect.Dimension.X = rect.Dimension.Y = skybox->GetWidth();
        rect.Dimension.Z = 1;

        int hunkMark = GHunkMemory.SetHunkMark();
        float * data = (float *)GHunkMemory.Alloc( 512*512*3*sizeof( *data ) );
        //byte * ucdata = (byte *)GHunkMemory.Alloc( 512*512*3 );
        for ( int i = 0 ; i < 6 ; i++ ) {
            rect.Offset.Z = i;
            skybox->ReadRect( rect, FORMAT_FLOAT3, 512*512*3*sizeof( *data ), 1, data ); // TODO: check half float

            //FlipImageY( data, 512, 512, 3*sizeof(*data), 512*3*sizeof( *data ) );

            for ( int p = 0; p<512*512*3; p += 3 ) {
                std::swap( data[p], data[p+2] );
                //ucdata[p] = Math::Clamp( data[p] * 255.0f, 0.0f, 255.0f );
                //ucdata[p+1] = Math::Clamp( data[p+1] * 255.0f, 0.0f, 255.0f );
                //ucdata[p+2] = Math::Clamp( data[p+2] * 255.0f, 0.0f, 255.0f );
            }

            AFileStream f;
            //f.OpenWrite( Platform::Fmt( "skyface%d.png", i ) );
            //WritePNG( f, rect.Dimension.X, rect.Dimension.Y, 3, ucdata, rect.Dimension.X * 3 );

            f.OpenWrite( Platform::Fmt( "xskyface%d.hdr", i ) );
            WriteHDR( f, rect.Dimension.X, rect.Dimension.Y, 3, data );
        }
        GHunkMemory.ClearToMark( hunkMark );
#endif
    }

#if 1
    TRef<ITexture> cubemap;
    TRef<ITexture> cubemap2;
    {
        const char* Cubemap[6] = {
            "DarkSky/rt.tga",
            "DarkSky/lt.tga",
            "DarkSky/up.tga",
            "DarkSky/dn.tga",
            "DarkSky/bk.tga",
            "DarkSky/ft.tga"};
        const char* Cubemap2[6] = {
            "DarkSky/rt.tga",
            "DarkSky/lt.tga",
            "DarkSky/up.tga",
            "DarkSky/dn.tga",
            "DarkSky/bk.tga",
            "DarkSky/ft.tga"};
        AImage        rt, lt, up, dn, bk, ft;
        AImage const* cubeFaces[6] = {&rt, &lt, &up, &dn, &bk, &ft};
        rt.Load(Cubemap[0], nullptr, IMAGE_PF_BGR32F);
        lt.Load(Cubemap[1], nullptr, IMAGE_PF_BGR32F);
        up.Load(Cubemap[2], nullptr, IMAGE_PF_BGR32F);
        dn.Load(Cubemap[3], nullptr, IMAGE_PF_BGR32F);
        bk.Load(Cubemap[4], nullptr, IMAGE_PF_BGR32F);
        ft.Load(Cubemap[5], nullptr, IMAGE_PF_BGR32F);
#    if 0
        const float HDRI_Scale = 4.0f;
        const float HDRI_Pow = 1.1f;
#    else
        const float HDRI_Scale = 2;
        const float HDRI_Pow   = 1;
#    endif
        for (int i = 0; i < 6; i++)
        {
            float* HDRI  = (float*)cubeFaces[i]->GetData();
            int    count = cubeFaces[i]->GetWidth() * cubeFaces[i]->GetHeight() * 3;
            for (int j = 0; j < count; j += 3)
            {
                HDRI[j]     = Math::Pow(HDRI[j + 0] * HDRI_Scale, HDRI_Pow);
                HDRI[j + 1] = Math::Pow(HDRI[j + 1] * HDRI_Scale, HDRI_Pow);
                HDRI[j + 2] = Math::Pow(HDRI[j + 2] * HDRI_Scale, HDRI_Pow);


                //HDRI[j + 2]= HDRI[j + 1]= HDRI[j]=0.01f;
            }
        }
        int          w                        = cubeFaces[0]->GetWidth();
        STextureDesc cubemapCI                = {};
        cubemapCI.SetFormat(TEXTURE_FORMAT_RGB32F);
        cubemapCI.SetResolution(STextureResolutionCubemap(w));
        cubemapCI.SetBindFlags(BIND_SHADER_RESOURCE);
        GDevice->CreateTexture(cubemapCI, &cubemap);
        for (int face = 0; face < 6; face++)
        {
            float* pSrc = (float*)cubeFaces[face]->GetData();

            STextureRect rect = {};
            rect.Offset.Z     = face;
            rect.Dimension.X  = w;
            rect.Dimension.Y  = w;
            rect.Dimension.Z  = 1;

            cubemap->WriteRect(rect, FORMAT_FLOAT3, w * w * 3 * sizeof(float), 1, pSrc);
        }
        rt.Load(Cubemap2[0], nullptr, IMAGE_PF_BGR32F);
        lt.Load(Cubemap2[1], nullptr, IMAGE_PF_BGR32F);
        up.Load(Cubemap2[2], nullptr, IMAGE_PF_BGR32F);
        dn.Load(Cubemap2[3], nullptr, IMAGE_PF_BGR32F);
        bk.Load(Cubemap2[4], nullptr, IMAGE_PF_BGR32F);
        ft.Load(Cubemap2[5], nullptr, IMAGE_PF_BGR32F);
        w                                     = cubeFaces[0]->GetWidth();
        cubemapCI.SetResolution(STextureResolutionCubemap(w));
        GDevice->CreateTexture(cubemapCI, &cubemap2);
        for (int face = 0; face < 6; face++)
        {
            float* pSrc = (float*)cubeFaces[face]->GetData();

            //Platform::Memset(pSrc,0, w*w*3*sizeof( float ) );
            for (int y = 0; y < w; y++)
                for (int x = 0; x < w; x++)
                    pSrc[y * w * 3 + x * 3 + 0] = pSrc[y * w * 3 + x * 3 + 1] = pSrc[y * w * 3 + x * 3 + 2] = 0.02f;

            STextureRect rect = {};
            rect.Offset.Z     = face;
            rect.Dimension.X  = w;
            rect.Dimension.Y  = w;
            rect.Dimension.Z  = 1;

            cubemap2->WriteRect(rect, FORMAT_FLOAT3, w * w * 3 * sizeof(float), 1, pSrc);
        }
    }

    ITexture* cubemaps[2] = {cubemap2, cubemap /*skybox*/};
#else

    Texture cubemap;
    {
        AImage img;

        //img.Load( "052_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );
        //img.Load( "059_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );
        img.Load("087_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F);

        ITexture                 source;
        TextureStorageCreateInfo createInfo = {};
        createInfo.Type                     = TEXTURE_2D;
        createInfo.Format                   = TEXTURE_FORMAT_RGB16F;
        createInfo.Resolution.Tex2D.Width   = img.GetWidth();
        createInfo.Resolution.Tex2D.Height  = img.GetHeight();
        createInfo.NumMipLevels             = 1;
        source                              = GDevice->CreateTexture(createInfo);
        source->Write(0, PIXEL_FORMAT_HALF_RGB, img.GetWidth() * img.GetHeight() * 3 * 2, 1, img.GetData());

        const int cubemapResoultion = 1024;

        ACubemapGenerator cubemapGenerator;
        cubemapGenerator.Initialize();
        cubemapGenerator.Generate(cubemap, TEXTURE_FORMAT_RGB16F, cubemapResoultion, &source);

#    if 0
        TextureRect rect;
        rect.Offset.Lod = 0;
        rect.Offset.X = 0;
        rect.Offset.Y = 0;
        rect.Dimension.X = cubemapResoultion;
        rect.Dimension.Y = cubemapResoultion;
        rect.Dimension.Z = 1;
        void * data = GHeapMemory.Alloc( cubemapResoultion*cubemapResoultion*3*sizeof( float ) );
        AFileStream f;
        for ( int i = 0 ; i < 6 ; i++ ) {
            rect.Offset.Z = i;
            cubemap.ReadRect( rect, FORMAT_FLOAT3, cubemapResoultion*cubemapResoultion*3*sizeof( float ), 1, data );
            f.OpenWrite( Platform::Fmt( "nightsky_%d.hdr", i ) );
            WriteHDR( f, cubemapResoultion, cubemapResoultion, 3, (float*)data );
        }
        GHeapMemory.Free( data );
#    endif
    }

    Texture* cubemaps[] = {&cubemap};
#endif



    {
        AEnvProbeGenerator envProbeGenerator;
        envProbeGenerator.GenerateArray(7, HK_ARRAY_SIZE(cubemaps), cubemaps, &GPrefilteredMap);
        SSamplerDesc samplerCI;
        samplerCI.Filter           = FILTER_MIPMAP_BILINEAR;
        samplerCI.bCubemapSeamless = true;

        GPrefilteredMapBindless = GPrefilteredMap->GetBindlessSampler(samplerCI);
        GPrefilteredMap->MakeBindlessSamplerResident(GPrefilteredMapBindless, true);
    }

    {
        AIrradianceGenerator irradianceGenerator;
        irradianceGenerator.GenerateArray(HK_ARRAY_SIZE(cubemaps), cubemaps, &GIrradianceMap);
        SSamplerDesc samplerCI;
        samplerCI.Filter           = FILTER_LINEAR;
        samplerCI.bCubemapSeamless = true;

        GIrradianceMapBindless = GIrradianceMap->GetBindlessSampler(samplerCI);
        GIrradianceMap->MakeBindlessSamplerResident(GIrradianceMapBindless, true);
    }

    SVirtualTextureCacheLayerInfo layer;
    layer.TextureFormat   = TEXTURE_FORMAT_SRGB8;
    layer.UploadFormat    = FORMAT_UBYTE3;
    layer.PageSizeInBytes = 128 * 128 * 3;

    SVirtualTextureCacheCreateInfo createInfo;
    createInfo.PageCacheCapacityX = 32;
    createInfo.PageCacheCapacityY = 32;
    createInfo.PageResolutionB    = 128;
    createInfo.NumLayers          = 1;
    createInfo.pLayers            = &layer;
    PhysCacheVT                   = MakeRef<AVirtualTextureCache>(createInfo);

    GPhysCacheVT = PhysCacheVT;

    #endif

    //::TestVT();
    //PhysCacheVT->CreateTexture( "Test.vt3", &TestVT );

//#define SPARSE_TEXTURE_TEST
#ifdef SPARSE_TEXTURE_TEST
#    if 0
    {
    SSparseTextureCreateInfo sparseTextureCI = MakeSparseTexture( TEXTURE_FORMAT_RGBA8, STextureResolution2D( 2048, 2048 ) );
    TRef< ISparseTexture > sparseTexture;
    GDevice->CreateSparseTexture( sparseTextureCI, &sparseTexture );

    int pageSizeX = sparseTexture->GetPageSizeX();
    int pageSizeY = sparseTexture->GetPageSizeY();
    int sz = pageSizeX*pageSizeY*4;

    byte * mem = (byte*)StackAlloc( sz );
    Platform::ZeroMem( mem, sz );

    sparseTexture->CommitPage( 0,0,0,0, FORMAT_UBYTE4, sz, 1, mem );
    }

    {
    int numPageSizes = 0;
    GDevice->EnumerateSparseTexturePageSize( SPARSE_TEXTURE_2D_ARRAY, TEXTURE_FORMAT_RGBA8, &numPageSizes, nullptr, nullptr, nullptr );
    TPodVector<int> pageSizeX; pageSizeX.Resize( numPageSizes );
    TPodVector<int> pageSizeY; pageSizeY.Resize( numPageSizes );
    TPodVector<int> pageSizeZ; pageSizeZ.Resize( numPageSizes );
    GDevice->EnumerateSparseTexturePageSize( SPARSE_TEXTURE_2D_ARRAY, TEXTURE_FORMAT_RGBA8, &numPageSizes, pageSizeX.ToPtr(), pageSizeY.ToPtr(), pageSizeZ.ToPtr() );
    for ( int i = 0 ; i < numPageSizes ; i++ ) {
        GLogger.Printf( "Sparse page size %d %d %d\n", pageSizeX[i], pageSizeY[i], pageSizeZ[i] );
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
    SSparseTextureCreateInfo sparseTextureCI = MakeSparseTexture(TEXTURE_FORMAT_RGBA8, STextureResolution2DArray(texSize, texSize, maxLayers), STextureSwizzle(), numLods);

    TRef<ISparseTexture> sparseTexture;
    GDevice->CreateSparseTexture(sparseTextureCI, &sparseTexture);

#    if 0
    int pageSizeX = sparseTexture->GetPageSizeX();
    int pageSizeY = sparseTexture->GetPageSizeY();
    int sz = pageSizeX*pageSizeY*4;

    byte * mem = (byte*)StackAlloc( sz );
    Platform::ZeroMem( mem, sz );

    sparseTexture->CommitPage( 0, 0, 0, 0, FORMAT_UBYTE4, sz, 1, mem );
#    else
    int sz = texSize * texSize * 4;

    byte* mem = (byte*)malloc(sz);
    Platform::ZeroMem(mem, sz);

    GLogger.Printf("\tTotal available after create: %d Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10);

    for (int i = 0; i < 10; i++)
    {
        STextureRect rect;
        rect.Offset.Lod = 0;
        rect.Offset.X = 0;
        rect.Offset.Y = 0;
        rect.Offset.Z = 0;
        rect.Dimension.X = texSize;
        rect.Dimension.Y = texSize;
        rect.Dimension.Z = 1;
        sparseTexture->CommitRect(rect, FORMAT_UBYTE4, sz, 1, mem);
        GLogger.Printf("\tTotal available after commit: %d Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10);
        sparseTexture->UncommitRect(rect);
        GLogger.Printf("\tTotal available after uncommit: %d Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10);
    }
    free(mem);
#    endif
#endif

    #if 0
    // Test SPIR-V
    TRef<IShaderModule> shaderModule;
    SShaderBinaryData   binaryData;
    binaryData.ShaderType   = VERTEX_SHADER;
    binaryData.BinaryFormat = SHADER_BINARY_FORMAT_SPIR_V_ARB;
    LoadSPIRV(&binaryData.BinaryCode, &binaryData.BinarySize);
    GDevice->CreateShaderFromBinary(&binaryData, &shaderModule);
    #endif


    CreateTerrainMaterialDepth(&TerrainDepthPipeline);
    GTerrainDepthPipeline = TerrainDepthPipeline;

    CreateTerrainMaterialLight(&TerrainLightPipeline);
    GTerrainLightPipeline = TerrainLightPipeline;

    CreateTerrainMaterialWireframe(&TerrainWireframePipeline);
    GTerrainWireframePipeline = TerrainWireframePipeline;
}

ARenderBackend::~ARenderBackend()
{
    GLogger.Printf("Deinitializing render backend...\n");

    //SDL_SetRelativeMouseMode( SDL_FALSE );
    //AVirtualTexture * vt = TestVT.GetObject();
    //TestVT.Reset();
    PhysCacheVT.Reset();
    FeedbackAnalyzerVT.Reset();
    //GLogger.Printf( "VT ref count %d\n", vt->GetRefCount() );

    GCircularBuffer.Reset();
    GWhiteTexture.Reset();
    GLookupBRDF.Reset();
    GSphereMesh.Reset();
    GSaq.Reset();
    GClusterLookup.Reset();
    GClusterItemTBO.Reset();
    GClusterItemBuffer.Reset();
    GPrefilteredMap.Reset();
    GIrradianceMap.Reset();
}

#if 0
void ARenderBackend::InitializeBuffer( TRef< IBuffer > * ppBuffer, size_t _SizeInBytes )
{
    SBufferDesc bufferCI = {};

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

int ARenderBackend::ClusterPackedIndicesAlignment() const
{
    return GDevice->GetDeviceCaps(DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT);
}

void ARenderBackend::InitializeMaterial(AMaterialGPU* Material, SMaterialDef const* Def)
{
    Material->MaterialType              = Def->Type;
    Material->LightmapSlot              = Def->LightmapSlot;
    Material->DepthPassTextureCount     = Def->DepthPassTextureCount;
    Material->LightPassTextureCount     = Def->LightPassTextureCount;
    Material->WireframePassTextureCount = Def->WireframePassTextureCount;
    Material->NormalsPassTextureCount   = Def->NormalsPassTextureCount;
    Material->ShadowMapPassTextureCount = Def->ShadowMapPassTextureCount;

    POLYGON_CULL cullMode = Def->bTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;

    AString code = LoadShader("material.glsl", Def->Shaders);

    //{
    //    AFileStream fs;
    //    fs.OpenWrite( "test.txt" );
    //    fs.WriteBuffer( code.CStr(), code.Length() );
    //}

    bool bTessellation          = Def->TessellationMethod != TESSELLATION_DISABLED;
    bool bTessellationShadowMap = bTessellation && Def->bDisplacementAffectShadow;

    switch (Material->MaterialType)
    {
        case MATERIAL_TYPE_PBR:
        case MATERIAL_TYPE_BASELIGHT:
        case MATERIAL_TYPE_UNLIT: {
            for (int i = 0; i < 2; i++)
            {
                bool bSkinned = !!i;

                CreateDepthPassPipeline(&Material->DepthPass[i], code.CStr(), Def->bAlphaMasking, cullMode, bSkinned, bTessellation, Def->Samplers, Def->DepthPassTextureCount);
                CreateDepthVelocityPassPipeline(&Material->DepthVelocityPass[i], code.CStr(), cullMode, bSkinned, bTessellation, Def->Samplers, Def->DepthPassTextureCount);
                CreateLightPassPipeline(&Material->LightPass[i], code.CStr(), cullMode, bSkinned, Def->bDepthTest_EXPERIMENTAL, Def->bTranslucent, Def->Blending, bTessellation, Def->Samplers, Def->LightPassTextureCount);
                CreateWireframePassPipeline(&Material->WireframePass[i], code.CStr(), cullMode, bSkinned, bTessellation, Def->Samplers, Def->WireframePassTextureCount);
                CreateNormalsPassPipeline(&Material->NormalsPass[i], code.CStr(), bSkinned, Def->Samplers, Def->NormalsPassTextureCount);
                CreateShadowMapPassPipeline(&Material->ShadowPass[i], code.CStr(), Def->bShadowMapMasking, Def->bTwoSided, bSkinned, bTessellationShadowMap, Def->Samplers, Def->ShadowMapPassTextureCount);
                CreateFeedbackPassPipeline(&Material->FeedbackPass[i], code.CStr(), cullMode, bSkinned, Def->Samplers, Def->LightPassTextureCount); // FIXME: Add FeedbackPassTextureCount
                CreateOutlinePassPipeline(&Material->OutlinePass[i], code.CStr(), cullMode, bSkinned, bTessellation, Def->Samplers, Def->DepthPassTextureCount);
            }

            if (Material->MaterialType != MATERIAL_TYPE_UNLIT)
            {
                CreateLightPassLightmapPipeline(&Material->LightPassLightmap, code.CStr(), cullMode, Def->bDepthTest_EXPERIMENTAL, Def->bTranslucent, Def->Blending, bTessellation, Def->Samplers, Def->LightPassTextureCount);
                CreateLightPassVertexLightPipeline(&Material->LightPassVertexLight, code.CStr(), cullMode, Def->bDepthTest_EXPERIMENTAL, Def->bTranslucent, Def->Blending, bTessellation, Def->Samplers, Def->LightPassTextureCount);
            }
            break;
        }
        case MATERIAL_TYPE_HUD:
        case MATERIAL_TYPE_POSTPROCESS: {
            CreateHUDPipeline(&Material->HUDPipeline, code.CStr(), Def->Samplers, Def->NumSamplers);
            break;
        }
        default:
            HK_ASSERT(0);
    }
}

void ARenderBackend::RenderFrame(AStreamedMemoryGPU* StreamedMemory, ITexture* pBackBuffer, SRenderFrame* pFrameData)
{
    static int timeQueryFrame = 0;

    GStreamedMemory = StreamedMemory;
    GStreamBuffer = StreamedMemory->GetBufferGPU();

    // Create item buffer
    if (!GClusterItemTBO)
    {
#if 0
        // FIXME: Use SSBO?
        SBufferDesc bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = MAX_TOTAL_CLUSTER_ITEMS * sizeof( SClusterPackedIndex );
        GDevice->CreateBuffer( bufferCI, nullptr, &GClusterItemBuffer );

        GClusterItemBuffer->SetDebugName( "Cluster item buffer" );

        SBufferViewDesc bufferViewCI = {};
        bufferViewCI.Format = BUFFER_VIEW_PIXEL_FORMAT_R32UI;
        GClusterItemBuffer->CreateView( bufferViewCI, &GClusterItemTBO );
#else
        SBufferViewDesc bufferViewCI = {};
        bufferViewCI.Format          = BUFFER_VIEW_PIXEL_FORMAT_R32UI;

        GStreamBuffer->CreateView(bufferViewCI, &GClusterItemTBO);
#endif
    }

    if (r_ShowGPUTime)
    {
#ifdef QUERY_TIMESTAMP
        rcmd->RecordTimeStamp(TimeStamp1, timeQueryFrame);
#else
        rcmd->BeginQuery(TimeQuery, timeQueryFrame);

        timeQueryFrame = (timeQueryFrame + 1) % TimeQuery->GetPoolSize();
#endif
    }

    GFrameData = pFrameData;

    FrameGraph->Clear();

    //rcmd->SetSwapChainResolution( GFrameData->CanvasWidth, GFrameData->CanvasHeight );

    // Update cache at beggining of the frame to give more time for stream thread
    if (PhysCacheVT)
        PhysCacheVT->Update();

    FeedbackAnalyzerVT->Begin(StreamedMemory);

    // TODO: Bind virtual textures in one place
    FeedbackAnalyzerVT->BindTexture(0, TestVT);

    GRenderViewContext.clear();
    GRenderViewContext.resize(GFrameData->NumViews);

    TPodVector<FGTextureProxy*> pRenderViewTexture(GFrameData->NumViews);
    for (int i = 0; i < GFrameData->NumViews; i++)
    {
        SRenderView* pRenderView = &GFrameData->RenderViews[i];

        RenderView(i, pRenderView, &pRenderViewTexture[i]);
        HK_ASSERT(pRenderViewTexture[i] != nullptr);
    }

    CanvasRenderer->Render(*FrameGraph, pRenderViewTexture, pBackBuffer);

    FrameGraph->Build();
    //FrameGraph->ExportGraphviz("frame.graphviz");
    rcmd->ExecuteFrameGraph(FrameGraph);

    if (r_FrameGraphDebug)
    {
        FrameGraph->Debug();
    }

    FeedbackAnalyzerVT->End();

    if (r_ShowGPUTime)
    {
#ifdef QUERY_TIMESTAMP
        rcmd->RecordTimeStamp(TimeStamp2, timeQueryFrame);

        timeQueryFrame = (timeQueryFrame + 1) % TimeStamp1->GetPoolSize();

        uint64_t timeStamp1 = 0;
        uint64_t timeStamp2 = 0;
        rcmd->GetQueryPoolResult64(TimeStamp2, timeQueryFrame, &timeStamp2, QUERY_RESULT_WAIT_BIT);
        rcmd->GetQueryPoolResult64(TimeStamp1, timeQueryFrame, &timeStamp1, QUERY_RESULT_WAIT_BIT);

        GLogger.Printf("GPU time %f ms\n", (double)(timeStamp2 - timeStamp1) / 1000000.0);
#else
        rcmd->EndQuery(TimeQuery);

        uint64_t timeQueryResult = 0;
        rcmd->GetQueryPoolResult64(TimeQuery, timeQueryFrame, &timeQueryResult, QUERY_RESULT_WAIT_BIT);

        GLogger.Printf("GPU time %f ms\n", (double)timeQueryResult / 1000000.0);
#endif
    }

    r_RenderSnapshot = false;

    GStreamedMemory = nullptr;
    GStreamBuffer = nullptr;
}

void ARenderBackend::SetViewConstants(int ViewportIndex)
{
    size_t offset = GStreamedMemory->AllocateConstant(sizeof(SViewConstantBuffer));

    SViewConstantBuffer* pViewCBuf = (SViewConstantBuffer*)GStreamedMemory->Map(offset);

    pViewCBuf->OrthoProjection         = GFrameData->CanvasOrthoProjection;
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

    pViewCBuf->GlobalIrradianceMap = GRenderView->GlobalIrradianceMap;
    pViewCBuf->GlobalReflectionMap = GRenderView->GlobalReflectionMap;

    pViewCBuf->DynamicResolutionRatioX  = (float)GRenderView->Width / GFrameData->RenderTargetMaxWidth;
    pViewCBuf->DynamicResolutionRatioY  = (float)GRenderView->Height / GFrameData->RenderTargetMaxHeight;
    pViewCBuf->DynamicResolutionRatioPX = (float)GRenderView->WidthP / GFrameData->RenderTargetMaxWidthP;
    pViewCBuf->DynamicResolutionRatioPY = (float)GRenderView->HeightP / GFrameData->RenderTargetMaxHeightP;

    pViewCBuf->FeedbackBufferResolutionRatio = GRenderView->VTFeedback->GetResolutionRatio();

    if (PhysCacheVT)
    {
        pViewCBuf->VTPageCacheCapacity.X           = (float)PhysCacheVT->GetPageCacheCapacityX();
        pViewCBuf->VTPageCacheCapacity.Y           = (float)PhysCacheVT->GetPageCacheCapacityY();
        pViewCBuf->VTPageTranslationOffsetAndScale = PhysCacheVT->GetPageTranslationOffsetAndScale();
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
                                            r_BloomParam3.GetFloat()) *
        r_BloomScale.GetFloat();

    pViewCBuf->BloomEnabled                = r_Bloom;                   // TODO: Get from GRenderView
    pViewCBuf->ToneMappingExposure         = r_ToneExposure.GetFloat(); // TODO: Get from GRenderView
    pViewCBuf->ColorGrading                = GRenderView->CurrentColorGradingLUT ? 1.0f : 0.0f;
    pViewCBuf->FXAA                        = r_FXAA;
    pViewCBuf->VignetteColorIntensity      = GRenderView->VignetteColorIntensity;
    pViewCBuf->VignetteOuterRadiusSqr      = GRenderView->VignetteOuterRadiusSqr;
    pViewCBuf->VignetteInnerRadiusSqr      = GRenderView->VignetteInnerRadiusSqr;
    pViewCBuf->ColorGradingAdaptationSpeed = GRenderView->ColorGradingAdaptationSpeed;
    pViewCBuf->ViewBrightness              = Math::Saturate(r_Brightness.GetFloat());

    pViewCBuf->SSLRSampleOffset  = r_SSLRSampleOffset.GetFloat();
    pViewCBuf->SSLRMaxDist       = r_SSLRMaxDist.GetFloat();
    pViewCBuf->IsPerspective     = float(GRenderView->bPerspective);
    pViewCBuf->TessellationLevel = r_TessellationLevel.GetFloat() * Math::Lerp((float)GRenderView->Width, (float)GRenderView->Height, 0.5f);

    pViewCBuf->PrefilteredMapSampler = GPrefilteredMapBindless;
    pViewCBuf->IrradianceMapSampler  = GIrradianceMapBindless;

    pViewCBuf->DebugMode = r_DebugRenderMode.GetInteger();

    pViewCBuf->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //GLogger.Printf( "GRenderView->FirstDirectionalLight: %d\n", GRenderView->FirstDirectionalLight );

    for (int i = 0; i < GRenderView->NumDirectionalLights; i++)
    {
        SDirectionalLightInstance* light = GFrameData->DirectionalLights[GRenderView->FirstDirectionalLight + i];

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

void ARenderBackend::UploadShaderResources(int ViewportIndex)
{
    SetViewConstants(ViewportIndex);

    // Bind light buffer
    rtbl->BindBuffer(4, GStreamBuffer, GRenderView->PointLightsStreamHandle, GRenderView->PointLightsStreamSize);

    // Bind IBL buffer
    rtbl->BindBuffer(5, GStreamBuffer, GRenderView->ProbeStreamHandle, GRenderView->ProbeStreamSize);

    // Copy cluster data

#if 1
    // Perform copy from stream buffer on GPU side
    STextureRect rect = {};
    rect.Dimension.X  = MAX_FRUSTUM_CLUSTERS_X;
    rect.Dimension.Y  = MAX_FRUSTUM_CLUSTERS_Y;
    rect.Dimension.Z  = MAX_FRUSTUM_CLUSTERS_Z;
    rcmd->CopyBufferToTexture(GStreamBuffer, GClusterLookup, rect, FORMAT_UINT2, 0, GRenderView->ClusterLookupStreamHandle, 1);
#else
    GClusterLookup->Write(0,
                          FORMAT_UINT2,
                          sizeof(SClusterHeader) * MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y * MAX_FRUSTUM_CLUSTERS_Z,
                          1,
                          GRenderView->LightData.ClusterLookup);
#endif

#if 1
    // Perform copy from stream buffer on GPU side
    if (GRenderView->ClusterPackedIndexCount > 0)
    {
#    if 0
        SBufferCopy range;
        range.SrcOffset = GRenderView->ClusterPackedIndicesStreamHandle;
        range.DstOffset = 0;
        range.SizeInBytes = sizeof( SClusterPackedIndex ) * GRenderView->ClusterPackedIndexCount;
        rcmd->CopyBufferRange( GStreamBuffer, GClusterItemBuffer, 1, &range );
#    else
        size_t offset = GRenderView->ClusterPackedIndicesStreamHandle;
        size_t sizeInBytes = sizeof(SClusterPackedIndex) * GRenderView->ClusterPackedIndexCount;
        GClusterItemTBO->SetRange(offset, sizeInBytes);
#    endif
    }
#else
    GClusterItemBuffer->WriteRange(0,
                                   sizeof(SClusterItemOffset) * GRenderView->LightData.NumClusterItems,
                                   GRenderView->LightData.ItemBuffer);
#endif
}

void ARenderBackend::RenderView(int ViewportIndex, SRenderView* pRenderView, FGTextureProxy** ppViewTexture)
{
    HK_ASSERT(pRenderView->Width > 0);
    HK_ASSERT(pRenderView->Height > 0);

    *ppViewTexture = nullptr;

    GRenderView            = pRenderView;
    GRenderViewArea.X      = 0;
    GRenderViewArea.Y      = 0;
    GRenderViewArea.Width  = pRenderView->Width;
    GRenderViewArea.Height = pRenderView->Height;

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ACustomTask& task = FrameGraph->AddTask<ACustomTask>("Setup render view");
    FGBufferViewProxy*bufferView = FrameGraph->AddExternalResource<FGBufferViewProxy>("hack hack", GClusterItemTBO);
    task.AddResource(bufferView, FG_RESOURCE_ACCESS_WRITE);
    task.SetFunction([=](ACustomTaskContext const& Task)
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

    

    bool bVirtualTexturing = FeedbackAnalyzerVT->HasBindings();

    // !!!!!!!!!!! FIXME: move outside of framegraph filling
    if (bVirtualTexturing)
    {
        pRenderView->VTFeedback->Begin(pRenderView->Width, pRenderView->Height);
    }

    FrameRenderer->Render(*FrameGraph, bVirtualTexturing, PhysCacheVT, ppViewTexture);

    // !!!!!!!!!!! FIXME: move outside of framegraph filling
    if (bVirtualTexturing)
    {
        int         FeedbackSize;
        const void* FeedbackData;
        pRenderView->VTFeedback->End(&FeedbackSize, &FeedbackData);

        FeedbackAnalyzerVT->AddFeedbackData(FeedbackSize, FeedbackData);
    }
}
