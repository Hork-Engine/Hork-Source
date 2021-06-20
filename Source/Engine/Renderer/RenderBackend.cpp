/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "RenderLocal.h"
#include "Material.h"
#include "CanvasRenderer.h"
#include "VT/VirtualTextureFeedback.h"
#include "IrradianceGenerator.h"
#include "EnvProbeGenerator.h"
#include "CubemapGenerator.h"
#include "AtmosphereRenderer.h"
#include "VXGIVoxelizer.h"

#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/ScopedTimeCheck.h>

#include <Core/Public/WindowsDefs.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/Logger.h>
#include <Core/Public/Image.h>

#include <SDL.h>

ARuntimeVariable r_FrameGraphDebug( _CTS( "r_FrameGraphDebug" ), _CTS( "0" ) );
ARuntimeVariable r_RenderSnapshot( _CTS( "r_RenderSnapshot" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable r_DebugRenderMode( _CTS( "r_DebugRenderMode" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable r_BloomScale( _CTS( "r_BloomScale" ), _CTS( "1" ) );
ARuntimeVariable r_Bloom( _CTS( "r_Bloom" ), _CTS( "1" ) );
ARuntimeVariable r_BloomParam0( _CTS( "r_BloomParam0" ), _CTS( "0.5" ) );
ARuntimeVariable r_BloomParam1( _CTS( "r_BloomParam1" ), _CTS( "0.3" ) );
ARuntimeVariable r_BloomParam2( _CTS( "r_BloomParam2" ), _CTS( "0.04" ) );
ARuntimeVariable r_BloomParam3( _CTS( "r_BloomParam3" ), _CTS( "0.01" ) );
ARuntimeVariable r_ToneExposure( _CTS( "r_ToneExposure" ), _CTS( "0.4" ) );
ARuntimeVariable r_Brightness( _CTS( "r_Brightness" ), _CTS( "1" ) );
ARuntimeVariable r_TessellationLevel( _CTS( "r_TessellationLevel" ), _CTS( "0.05" ) );
ARuntimeVariable r_MotionBlur( _CTS( "r_MotionBlur" ), _CTS( "1" ) );
ARuntimeVariable r_SSLR( _CTS( "r_SSLR" ), _CTS( "1" ), 0, _CTS( "Required to rebuld materials to apply" ) );
ARuntimeVariable r_SSLRMaxDist( _CTS( "r_SSLRMaxDist" ), _CTS( "10" ) );
ARuntimeVariable r_SSLRSampleOffset( _CTS( "r_SSLRSampleOffset" ), _CTS( "0.1" ) );
ARuntimeVariable r_HBAO( _CTS( "r_HBAO" ), _CTS( "1" ), 0, _CTS( "Required to rebuld materials to apply" ) );
ARuntimeVariable r_FXAA( _CTS( "r_FXAA" ), _CTS( "1" ) );
ARuntimeVariable r_ShowGPUTime( _CTS( "r_ShowGPUTime" ), _CTS( "0" ) );

void TestVT();

static void LoadSPIRV( void ** BinaryCode, size_t * BinarySize )
{
    // TODO

    *BinaryCode = nullptr;
    *BinarySize = 0;
}

ARenderBackend::ARenderBackend()
{
    using namespace RenderCore;

    GLogger.Printf( "Initializing render backend...\n" );

    GDevice = GRuntime.GetRenderDevice();

    GDevice->GetImmediateContext( &rcmd );

    rtbl = rcmd->GetRootResourceTable();

    // Store in global pointers
    GStreamBuffer = GRuntime.GetStreamedMemoryGPU()->GetBufferGPU();

    InitMaterialSamplers();

    FrameGraph = MakeRef< AFrameGraph >( GDevice );
    FrameRenderer = MakeRef< AFrameRenderer >();
    CanvasRenderer = MakeRef< ACanvasRenderer >();

    GCircularBuffer = MakeRef< ACircularBuffer >( 2 * 1024 * 1024 ); // 2MB
    //GFrameConstantBuffer = MakeRef< AFrameConstantBuffer >( 2 * 1024 * 1024 ); // 2MB

//#define QUERY_TIMESTAMP

    SQueryPoolCreateInfo timeQueryCI;
#ifdef QUERY_TIMESTAMP
    timeQueryCI.QueryType = QUERY_TYPE_TIMESTAMP;
    timeQueryCI.PoolSize = 3;
    GDevice->CreateQueryPool( timeQueryCI, &TimeStamp1 );
    GDevice->CreateQueryPool( timeQueryCI, &TimeStamp2 );
#else
    timeQueryCI.QueryType = QUERY_TYPE_TIME_ELAPSED;
    timeQueryCI.PoolSize = 3;
    GDevice->CreateQueryPool( timeQueryCI, &TimeQuery );
#endif

    // Create sphere mesh for cubemap rendering
    GSphereMesh = MakeRef< ASphereMesh >();

    // Create screen aligned quad
    {
        constexpr Float2 saqVertices[4] = {
            { Float2( -1.0f,  1.0f ) },
            { Float2( 1.0f,  1.0f ) },
            { Float2( -1.0f, -1.0f ) },
            { Float2( 1.0f, -1.0f ) }
        };

        RenderCore::SBufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.SizeInBytes = sizeof( saqVertices );
        GDevice->CreateBuffer( bufferCI, saqVertices, &GSaq );

        GSaq->SetDebugName( "Screen aligned quad" );
    }

    // Create white texture
    {
        GDevice->CreateTexture( RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_RGBA8, RenderCore::STextureResolution2D( 1, 1 ) ),
                                &GWhiteTexture );
        RenderCore::STextureRect rect = {};
        rect.Dimension.X = 1;
        rect.Dimension.Y = 1;
        rect.Dimension.Z = 1;
        const byte data[4] = { 0xff, 0xff, 0xff, 0xff };
        GWhiteTexture->WriteRect( rect, RenderCore::FORMAT_UBYTE4, sizeof( data ), 4, data );
    }

    // Create cluster lookup 3D texture
    GDevice->CreateTexture( RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_RG32UI,
                                                     RenderCore::STextureResolution3D( MAX_FRUSTUM_CLUSTERS_X,
                                                                                       MAX_FRUSTUM_CLUSTERS_Y,
                                                                                       MAX_FRUSTUM_CLUSTERS_Z ) ), &GClusterLookup );
    // Create item buffer
    {
#if 0
        // FIXME: Use SSBO?
        RenderCore::SBufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = RenderCore::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = MAX_TOTAL_CLUSTER_ITEMS * sizeof( SClusterPackedIndex );
        GDevice->CreateBuffer( bufferCI, nullptr, &GClusterItemBuffer );

        GClusterItemBuffer->SetDebugName( "Cluster item buffer" );

        RenderCore::SBufferViewCreateInfo bufferViewCI = {};
        bufferViewCI.Format = RenderCore::BUFFER_VIEW_PIXEL_FORMAT_R32UI;
        GDevice->CreateBufferView( bufferViewCI, GClusterItemBuffer, &GClusterItemTBO );
#else
        RenderCore::SBufferViewCreateInfo bufferViewCI = {};
        bufferViewCI.Format = RenderCore::BUFFER_VIEW_PIXEL_FORMAT_R32UI;
        TRef< RenderCore::IBuffer > buffer( GStreamBuffer );
        GDevice->CreateBufferView( bufferViewCI, buffer, &GClusterItemTBO );
#endif
    }

    FeedbackAnalyzerVT = MakeRef< AVirtualTextureFeedbackAnalyzer >();
    GFeedbackAnalyzerVT = FeedbackAnalyzerVT;

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////

    {
        VXGIVoxelizer vox;
        vox.Render();
    }

    TRef< RenderCore::ITexture > skybox;
    {
        AAtmosphereRenderer atmosphereRenderer;
        atmosphereRenderer.Render( 512, Float3( -0.5f, -2, -10 ), &skybox );
#if 0
        RenderCore::STextureRect rect = {};
        rect.Dimension.X = rect.Dimension.Y = skybox->GetWidth();
        rect.Dimension.Z = 1;

        int hunkMark = GHunkMemory.SetHunkMark();
        float * data = (float *)GHunkMemory.Alloc( 512*512*3*sizeof( *data ) );
        //byte * ucdata = (byte *)GHunkMemory.Alloc( 512*512*3 );
        for ( int i = 0 ; i < 6 ; i++ ) {
            rect.Offset.Z = i;
            skybox->ReadRect( rect, RenderCore::FORMAT_FLOAT3, 512*512*3*sizeof( *data ), 1, data ); // TODO: check half float

            //FlipImageY( data, 512, 512, 3*sizeof(*data), 512*3*sizeof( *data ) );

            for ( int p = 0; p<512*512*3; p += 3 ) {
                StdSwap( data[p], data[p+2] );
                //ucdata[p] = Math::Clamp( data[p] * 255.0f, 0.0f, 255.0f );
                //ucdata[p+1] = Math::Clamp( data[p+1] * 255.0f, 0.0f, 255.0f );
                //ucdata[p+2] = Math::Clamp( data[p+2] * 255.0f, 0.0f, 255.0f );
            }

            AFileStream f;
            //f.OpenWrite( Core::Fmt( "skyface%d.png", i ) );
            //WritePNG( f, rect.Dimension.X, rect.Dimension.Y, 3, ucdata, rect.Dimension.X * 3 );

            f.OpenWrite( Core::Fmt( "xskyface%d.hdr", i ) );
            WriteHDR( f, rect.Dimension.X, rect.Dimension.Y, 3, data );
        }
        GHunkMemory.ClearToMark( hunkMark );
#endif
    }

#if 1
    TRef< RenderCore::ITexture > cubemap;
    TRef< RenderCore::ITexture > cubemap2;
    {
        const char * Cubemap[6] = {
            "DarkSky/rt.tga",
            "DarkSky/lt.tga",
            "DarkSky/up.tga",
            "DarkSky/dn.tga",
            "DarkSky/bk.tga",
            "DarkSky/ft.tga"
        };
        const char * Cubemap2[6] = {
            "DarkSky/rt.tga",
            "DarkSky/lt.tga",
            "DarkSky/up.tga",
            "DarkSky/dn.tga",
            "DarkSky/bk.tga",
            "DarkSky/ft.tga"
        };
        AImage rt, lt, up, dn, bk, ft;
        AImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
        rt.Load( Cubemap[0], nullptr, IMAGE_PF_BGR32F );
        lt.Load( Cubemap[1], nullptr, IMAGE_PF_BGR32F );
        up.Load( Cubemap[2], nullptr, IMAGE_PF_BGR32F );
        dn.Load( Cubemap[3], nullptr, IMAGE_PF_BGR32F );
        bk.Load( Cubemap[4], nullptr, IMAGE_PF_BGR32F );
        ft.Load( Cubemap[5], nullptr, IMAGE_PF_BGR32F );
#if 0
        const float HDRI_Scale = 4.0f;
        const float HDRI_Pow = 1.1f;
#else
        const float HDRI_Scale = 2;
        const float HDRI_Pow = 1;
#endif
        for ( int i = 0 ; i < 6 ; i++ ) {
            float * HDRI = (float*)cubeFaces[i]->GetData();
            int count = cubeFaces[i]->GetWidth()*cubeFaces[i]->GetHeight()*3;
            for ( int j = 0; j < count ; j += 3 ) {
                HDRI[j] = Math::Pow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 1] = Math::Pow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 2] = Math::Pow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );


//HDRI[j + 2]= HDRI[j + 1]= HDRI[j]=0.01f;
            }
        }
        int w = cubeFaces[0]->GetWidth();
        RenderCore::STextureCreateInfo cubemapCI = {};
        cubemapCI.Type = RenderCore::TEXTURE_CUBE_MAP;
        cubemapCI.Format = RenderCore::TEXTURE_FORMAT_RGB32F;
        cubemapCI.Resolution.TexCubemap.Width = w;
        cubemapCI.NumLods = 1;
        GDevice->CreateTexture( cubemapCI, &cubemap );
        for ( int face = 0 ; face < 6 ; face++ ) {
            float * pSrc = (float *)cubeFaces[face]->GetData();

            RenderCore::STextureRect rect = {};
            rect.Offset.Z = face;
            rect.Dimension.X = w;
            rect.Dimension.Y = w;
            rect.Dimension.Z = 1;

            cubemap->WriteRect( rect, RenderCore::FORMAT_FLOAT3, w*w*3*sizeof( float ), 1, pSrc );
        }
        rt.Load( Cubemap2[0], nullptr, IMAGE_PF_BGR32F );
        lt.Load( Cubemap2[1], nullptr, IMAGE_PF_BGR32F );
        up.Load( Cubemap2[2], nullptr, IMAGE_PF_BGR32F );
        dn.Load( Cubemap2[3], nullptr, IMAGE_PF_BGR32F );
        bk.Load( Cubemap2[4], nullptr, IMAGE_PF_BGR32F );
        ft.Load( Cubemap2[5], nullptr, IMAGE_PF_BGR32F );
        w = cubeFaces[0]->GetWidth();
        cubemapCI.Resolution.TexCubemap.Width = w;
        cubemapCI.NumLods = 1;
        GDevice->CreateTexture( cubemapCI, &cubemap2 );
        for ( int face = 0 ; face < 6 ; face++ ) {
            float * pSrc = (float *)cubeFaces[face]->GetData();

//Core::Memset(pSrc,0, w*w*3*sizeof( float ) );
            for ( int y=0;y<w;y++ )
                for ( int x=0;x<w;x++ )
                    pSrc[y*w*3 + x*3 + 0 ] = pSrc[y*w*3 + x*3 + 1] = pSrc[y*w*3 + x*3 + 2] = 0.02f;

            RenderCore::STextureRect rect = {};
            rect.Offset.Z = face;
            rect.Dimension.X = w;
            rect.Dimension.Y = w;
            rect.Dimension.Z = 1;

            cubemap2->WriteRect( rect, RenderCore::FORMAT_FLOAT3, w*w*3*sizeof( float ), 1, pSrc );
        }
    }

    RenderCore::ITexture * cubemaps[2] = { cubemap2,cubemap/*skybox*/ };
#else

    Texture cubemap;
    {
        AImage img;

        //img.Load( "052_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );
        //img.Load( "059_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );
        img.Load( "087_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );

        RenderCore::ITexture source;
        RenderCore::TextureStorageCreateInfo createInfo = {};
        createInfo.Type = RenderCore::TEXTURE_2D;
        createInfo.Format = RenderCore::TEXTURE_FORMAT_RGB16F;
        createInfo.Resolution.Tex2D.Width = img.GetWidth();
        createInfo.Resolution.Tex2D.Height = img.GetHeight();
        createInfo.NumLods = 1;
        source = GDevice->CreateTexture( createInfo );
        source->Write( 0, RenderCore::PIXEL_FORMAT_HALF_RGB, img.GetWidth()*img.GetHeight()*3*2, 1, img.GetData() );

        const int cubemapResoultion = 1024;

        ACubemapGenerator cubemapGenerator;
        cubemapGenerator.Initialize();
        cubemapGenerator.Generate( cubemap, RenderCore::TEXTURE_FORMAT_RGB16F, cubemapResoultion, &source );

#if 0
        RenderCore::TextureRect rect;
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
            cubemap.ReadRect( rect, RenderCore::FORMAT_FLOAT3, cubemapResoultion*cubemapResoultion*3*sizeof( float ), 1, data );
            f.OpenWrite( Core::Fmt( "nightsky_%d.hdr", i ) );
            WriteHDR( f, cubemapResoultion, cubemapResoultion, 3, (float*)data );
        }
        GHeapMemory.Free( data );
#endif
    }

    Texture * cubemaps[] = { &cubemap };
#endif



    {
        AEnvProbeGenerator envProbeGenerator;
        envProbeGenerator.GenerateArray( 7, AN_ARRAY_SIZE( cubemaps ), cubemaps, &GPrefilteredMap );
        RenderCore::SSamplerInfo samplerCI;
        samplerCI.Filter = RenderCore::FILTER_MIPMAP_BILINEAR;
        samplerCI.bCubemapSeamless = true;

//!!!!!!!!!!!!
        GDevice->GetBindlessSampler( GPrefilteredMap, samplerCI, &GPrefilteredMapBindless );

        //TRef< RenderCore::IBindlessSampler > smp;
        //GDevice->GetBindlessSampler( PrefilteredMap, samplerCI, &smp );

        GPrefilteredMapBindless->MakeResident();
    }

    {
        AIrradianceGenerator irradianceGenerator;
        irradianceGenerator.GenerateArray( AN_ARRAY_SIZE( cubemaps ), cubemaps, &GIrradianceMap );
        RenderCore::SSamplerInfo samplerCI;
        samplerCI.Filter = RenderCore::FILTER_LINEAR;
        samplerCI.bCubemapSeamless = true;

        GDevice->GetBindlessSampler( GIrradianceMap, samplerCI, &GIrradianceMapBindless );
        GIrradianceMapBindless->MakeResident();
    }

    SVirtualTextureCacheLayerInfo layer;
    layer.TextureFormat = RenderCore::TEXTURE_FORMAT_SRGB8;
    layer.UploadFormat = RenderCore::FORMAT_UBYTE3;
    layer.PageSizeInBytes = 128*128*3;

    SVirtualTextureCacheCreateInfo createInfo;
    createInfo.PageCacheCapacityX = 32;
    createInfo.PageCacheCapacityY = 32;
    createInfo.PageResolutionB = 128;
    createInfo.NumLayers = 1;
    createInfo.pLayers = &layer;
    PhysCacheVT = MakeRef< AVirtualTextureCache >( createInfo );

    GPhysCacheVT = PhysCacheVT;

    //::TestVT();
    //PhysCacheVT->CreateTexture( "Test.vt3", &TestVT );

//#define SPARSE_TEXTURE_TEST
#ifdef SPARSE_TEXTURE_TEST
#if 0
    {
    SSparseTextureCreateInfo sparseTextureCI = MakeSparseTexture( TEXTURE_FORMAT_RGBA8, STextureResolution2D( 2048, 2048 ) );
    TRef< ISparseTexture > sparseTexture;
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
    GDevice->EnumerateSparseTexturePageSize( SPARSE_TEXTURE_2D_ARRAY, TEXTURE_FORMAT_RGBA8, &numPageSizes, nullptr, nullptr, nullptr );
    TPodVector<int> pageSizeX; pageSizeX.Resize( numPageSizes );
    TPodVector<int> pageSizeY; pageSizeY.Resize( numPageSizes );
    TPodVector<int> pageSizeZ; pageSizeZ.Resize( numPageSizes );
    GDevice->EnumerateSparseTexturePageSize( SPARSE_TEXTURE_2D_ARRAY, TEXTURE_FORMAT_RGBA8, &numPageSizes, pageSizeX.ToPtr(), pageSizeY.ToPtr(), pageSizeZ.ToPtr() );
    for ( int i = 0 ; i < numPageSizes ; i++ ) {
        GLogger.Printf( "Sparse page size %d %d %d\n", pageSizeX[i], pageSizeY[i], pageSizeZ[i] );
    }
    }
#endif
    int maxLayers = GDevice->GetDeviceCaps( DEVICE_CAPS_MAX_TEXTURE_LAYERS );
    int texSize = 1024;
    int n = texSize;
    int numLods = 1;
    while ( n >>= 1 ) {
        numLods++;
    }
    SSparseTextureCreateInfo sparseTextureCI = MakeSparseTexture( TEXTURE_FORMAT_RGBA8, STextureResolution2DArray( texSize, texSize, maxLayers ), STextureSwizzle(), numLods );

    TRef< ISparseTexture > sparseTexture;
    GDevice->CreateSparseTexture( sparseTextureCI, &sparseTexture );

#if 0
    int pageSizeX = sparseTexture->GetPageSizeX();
    int pageSizeY = sparseTexture->GetPageSizeY();
    int sz = pageSizeX*pageSizeY*4;

    byte * mem = (byte*)StackAlloc( sz );
    Core::ZeroMem( mem, sz );

    sparseTexture->CommitPage( 0, 0, 0, 0, FORMAT_UBYTE4, sz, 1, mem );
#else
    int sz = texSize*texSize*4;

    byte * mem = (byte*)malloc( sz );
    Core::ZeroMem( mem, sz );

    GLogger.Printf( "\tTotal available after create: %d Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10 );

    for ( int i = 0 ; i < 10 ; i++ ) {
        RenderCore::STextureRect rect;
        rect.Offset.Lod = 0;
        rect.Offset.X = 0;
        rect.Offset.Y = 0;
        rect.Offset.Z = 0;
        rect.Dimension.X = texSize;
        rect.Dimension.Y = texSize;
        rect.Dimension.Z = 1;
        sparseTexture->CommitRect( rect, FORMAT_UBYTE4, sz, 1, mem );
        GLogger.Printf( "\tTotal available after commit: %d Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10 );
        sparseTexture->UncommitRect( rect );
        GLogger.Printf( "\tTotal available after uncommit: %d Megs\n", GDevice->GetGPUMemoryCurrentAvailable() >> 10 );
    }
    free(mem);
#endif
#endif

    // Test SPIR-V
    TRef< IShaderModule > shaderModule;
    SShaderBinaryData binaryData;
    binaryData.ShaderType = VERTEX_SHADER;
    binaryData.BinaryFormat = SHADER_BINARY_FORMAT_SPIR_V_ARB;
    LoadSPIRV( &binaryData.BinaryCode, &binaryData.BinarySize );
    GDevice->CreateShaderFromBinary( &binaryData, &shaderModule );



    CreateTerrainMaterialDepth( &TerrainDepthPipeline );
    GTerrainDepthPipeline = TerrainDepthPipeline;

    CreateTerrainMaterialLight( &TerrainLightPipeline );
    GTerrainLightPipeline = TerrainLightPipeline;

    CreateTerrainMaterialWireframe( &TerrainWireframePipeline );
    GTerrainWireframePipeline = TerrainWireframePipeline;
}

ARenderBackend::~ARenderBackend()
{
    GLogger.Printf( "Deinitializing render backend...\n" );

    //SDL_SetRelativeMouseMode( SDL_FALSE );
    //AVirtualTexture * vt = TestVT.GetObject();
    //TestVT.Reset();
    PhysCacheVT.Reset();
    FeedbackAnalyzerVT.Reset();
    //GLogger.Printf( "VT ref count %d\n", vt->GetRefCount() );

    GCircularBuffer.Reset();
    GWhiteTexture.Reset();
    GSphereMesh.Reset();
    GSaq.Reset();
    GClusterLookup.Reset();
    GClusterItemTBO.Reset();
    GClusterItemBuffer.Reset();
    GPrefilteredMapBindless.Reset();
    GIrradianceMapBindless.Reset();
    GPrefilteredMap.Reset();
    GIrradianceMap.Reset();
}

#if 0
void ARenderBackend::InitializeBuffer( TRef< RenderCore::IBuffer > * ppBuffer, size_t _SizeInBytes )
{
    RenderCore::SBufferCreateInfo bufferCI = {};

    bufferCI.SizeInBytes = _SizeInBytes;

    const bool bDynamicStorage = false;
    if ( bDynamicStorage ) {
#if 1
        // Seems to be faster
        bufferCI.ImmutableStorageFlags = RenderCore::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;
#else
        bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
        bufferCI.MutableUsage = RenderCore::MUTABLE_STORAGE_STREAM;
        bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)0;
        bufferCI.bImmutableStorage = false;
#endif

        GDevice->CreateBuffer( bufferCI, nullptr, ppBuffer );
    }
    else {
#if 1
        // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering (tested on NVidia GeForce GTX 770)
        bufferCI.MutableClientAccess = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
        bufferCI.MutableUsage = RenderCore::MUTABLE_STORAGE_STATIC;
        bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)0;
        bufferCI.bImmutableStorage = false;
#else
        bufferCI.ImmutableStorageFlags = RenderCore::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;
#endif

        GDevice->CreateBuffer( bufferCI, nullptr, ppBuffer );
    }
}
#endif

int ARenderBackend::ClusterPackedIndicesAlignment() const
{
    return GDevice->GetDeviceCaps( RenderCore::DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT );
}

void ARenderBackend::InitializeMaterial( AMaterialGPU * Material, SMaterialDef const * Def )
{
    Material->MaterialType = Def->Type;
    Material->LightmapSlot = Def->LightmapSlot;
    Material->DepthPassTextureCount     = Def->DepthPassTextureCount;
    Material->LightPassTextureCount     = Def->LightPassTextureCount;
    Material->WireframePassTextureCount = Def->WireframePassTextureCount;
    Material->NormalsPassTextureCount   = Def->NormalsPassTextureCount;
    Material->ShadowMapPassTextureCount = Def->ShadowMapPassTextureCount;

    RenderCore::POLYGON_CULL cullMode = Def->bTwoSided ? RenderCore::POLYGON_CULL_DISABLED : RenderCore::POLYGON_CULL_FRONT;

    AString code = LoadShader( "material.glsl", Def->Shaders );

    //{
    //    AFileStream fs;
    //    fs.OpenWrite( "test.txt" );
    //    fs.WriteBuffer( code.CStr(), code.Length() );
    //}

    bool bTessellation = Def->TessellationMethod != TESSELLATION_DISABLED;
    bool bTessellationShadowMap = bTessellation && Def->bDisplacementAffectShadow;

    switch ( Material->MaterialType ) {
    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:
    case MATERIAL_TYPE_UNLIT: {
        for ( int i = 0 ; i < 2 ; i++ ) {
            bool bSkinned = !!i;

            CreateDepthPassPipeline( &Material->DepthPass[i], code.CStr(), Def->bAlphaMasking, cullMode, bSkinned, bTessellation, Def->Samplers, Def->DepthPassTextureCount );
            CreateDepthVelocityPassPipeline( &Material->DepthVelocityPass[i], code.CStr(), cullMode, bSkinned, bTessellation, Def->Samplers, Def->DepthPassTextureCount );
            CreateLightPassPipeline( &Material->LightPass[i], code.CStr(), cullMode, bSkinned, Def->bDepthTest_EXPERIMENTAL, Def->bTranslucent, Def->Blending, bTessellation, Def->Samplers, Def->LightPassTextureCount );
            CreateWireframePassPipeline( &Material->WireframePass[i], code.CStr(), cullMode, bSkinned, bTessellation, Def->Samplers, Def->WireframePassTextureCount );
            CreateNormalsPassPipeline( &Material->NormalsPass[i], code.CStr(), bSkinned, Def->Samplers, Def->NormalsPassTextureCount );
            CreateShadowMapPassPipeline( &Material->ShadowPass[i], code.CStr(), Def->bShadowMapMasking, Def->bTwoSided, bSkinned, bTessellationShadowMap, Def->Samplers, Def->ShadowMapPassTextureCount );
            CreateFeedbackPassPipeline( &Material->FeedbackPass[i], code.CStr(), cullMode, bSkinned, Def->Samplers, Def->LightPassTextureCount ); // FIXME: Add FeedbackPassTextureCount
            CreateOutlinePassPipeline( &Material->OutlinePass[i], code.CStr(), cullMode, bSkinned, bTessellation, Def->Samplers, Def->DepthPassTextureCount );
        }

        if ( Material->MaterialType != MATERIAL_TYPE_UNLIT ) {
            CreateLightPassLightmapPipeline( &Material->LightPassLightmap, code.CStr(), cullMode, Def->bDepthTest_EXPERIMENTAL, Def->bTranslucent, Def->Blending, bTessellation, Def->Samplers, Def->LightPassTextureCount );
            CreateLightPassVertexLightPipeline( &Material->LightPassVertexLight, code.CStr(), cullMode, Def->bDepthTest_EXPERIMENTAL, Def->bTranslucent, Def->Blending, bTessellation, Def->Samplers, Def->LightPassTextureCount );
        }
        break;
    }
    case MATERIAL_TYPE_HUD:
    case MATERIAL_TYPE_POSTPROCESS: {
        CreateHUDPipeline( &Material->HUDPipeline, code.CStr(), Def->Samplers, Def->NumSamplers );
        break;
    }
    default:
        AN_ASSERT( 0 );
    }
}

void ARenderBackend::RenderFrame( SRenderFrame * pFrameData )
{
    static int timeQueryFrame = 0;

    if ( r_ShowGPUTime ) {
#ifdef QUERY_TIMESTAMP
        rcmd->RecordTimeStamp( TimeStamp1, timeQueryFrame );
#else
        rcmd->BeginQuery( TimeQuery, timeQueryFrame );

        timeQueryFrame = ( timeQueryFrame + 1 ) % TimeQuery->GetPoolSize();
#endif
    }

    GFrameData = pFrameData;

    rcmd->SetSwapChainResolution( GFrameData->CanvasWidth, GFrameData->CanvasHeight );

    // Update cache at beggining of the frame to give more time for stream thread
    PhysCacheVT->Update();

    FeedbackAnalyzerVT->Begin();

    // TODO: Bind virtual textures in one place
    FeedbackAnalyzerVT->BindTexture( 0, TestVT );

    CanvasRenderer->Render( [this]( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture )
    {
        RenderView( pRenderView, ppViewTexture );
    });

    FeedbackAnalyzerVT->End();

    if ( r_ShowGPUTime ) {
#ifdef QUERY_TIMESTAMP
        rcmd->RecordTimeStamp( TimeStamp2, timeQueryFrame );

        timeQueryFrame = ( timeQueryFrame + 1 ) % TimeStamp1->GetPoolSize();

        uint64_t timeStamp1 = 0;
        uint64_t timeStamp2 = 0;
        TimeStamp2->GetResult64( timeQueryFrame, &timeStamp2, RenderCore::QUERY_RESULT_WAIT_BIT );
        TimeStamp1->GetResult64( timeQueryFrame, &timeStamp1, RenderCore::QUERY_RESULT_WAIT_BIT );

        GLogger.Printf( "GPU time %f ms\n", (double)(timeStamp2-timeStamp1) / 1000000.0 );
#else
        rcmd->EndQuery( TimeQuery );

        uint64_t timeQueryResult = 0;
        TimeQuery->GetResult64( timeQueryFrame, &timeQueryResult, RenderCore::QUERY_RESULT_WAIT_BIT );

        GLogger.Printf( "GPU time %f ms\n", (double)timeQueryResult / 1000000.0 );
#endif
    }

    r_RenderSnapshot = false;
}

void ARenderBackend::SetViewConstants()
{
    AStreamedMemoryGPU * pStreamedMemory = GRuntime.GetStreamedMemoryGPU();
    size_t offset = pStreamedMemory->AllocateConstant( sizeof( SViewConstantBuffer ) );

    SViewConstantBuffer * pViewCBuf = (SViewConstantBuffer *)pStreamedMemory->Map( offset );

    pViewCBuf->OrthoProjection = GFrameData->CanvasOrthoProjection;
    pViewCBuf->ViewProjection = GRenderView->ViewProjection;
    pViewCBuf->ProjectionMatrix = GRenderView->ProjectionMatrix;
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
    pViewCBuf->ZNear = GRenderView->ViewZNear;
    pViewCBuf->ZFar = GRenderView->ViewZFar;

    if ( GRenderView->bPerspective ) {
        pViewCBuf->ProjectionInfo.X = -2.0f / GRenderView->ProjectionMatrix[0][0]; // (x) * (R - L)/N
        pViewCBuf->ProjectionInfo.Y = 2.0f / GRenderView->ProjectionMatrix[1][1]; // (y) * (T - B)/N
        pViewCBuf->ProjectionInfo.Z = (1.0f - GRenderView->ProjectionMatrix[2][0]) / GRenderView->ProjectionMatrix[0][0]; // L/N
        pViewCBuf->ProjectionInfo.W = -(1.0f + GRenderView->ProjectionMatrix[2][1]) / GRenderView->ProjectionMatrix[1][1]; // B/N
    }
    else {
        pViewCBuf->ProjectionInfo.X = 2.0f / GRenderView->ProjectionMatrix[0][0]; // (x) * R - L
        pViewCBuf->ProjectionInfo.Y = -2.0f / GRenderView->ProjectionMatrix[1][1]; // (y) * T - B
        pViewCBuf->ProjectionInfo.Z = -(1.0f + GRenderView->ProjectionMatrix[3][0]) / GRenderView->ProjectionMatrix[0][0]; // L
        pViewCBuf->ProjectionInfo.W = (1.0f - GRenderView->ProjectionMatrix[3][1]) / GRenderView->ProjectionMatrix[1][1]; // B
    }

    pViewCBuf->GameRunningTimeSeconds = GRenderView->GameRunningTimeSeconds;
    pViewCBuf->GameplayTimeSeconds = GRenderView->GameplayTimeSeconds;

    pViewCBuf->GlobalIrradianceMap = GRenderView->GlobalIrradianceMap;
    pViewCBuf->GlobalReflectionMap = GRenderView->GlobalReflectionMap;

    pViewCBuf->DynamicResolutionRatioX = (float)GRenderView->Width / GFrameData->RenderTargetMaxWidth;
    pViewCBuf->DynamicResolutionRatioY = (float)GRenderView->Height / GFrameData->RenderTargetMaxHeight;
    pViewCBuf->DynamicResolutionRatioPX = (float)GRenderView->WidthP / GFrameData->RenderTargetMaxWidthP;
    pViewCBuf->DynamicResolutionRatioPY = (float)GRenderView->HeightP / GFrameData->RenderTargetMaxHeightP;

    pViewCBuf->FeedbackBufferResolutionRatio = GRenderView->VTFeedback->GetResolutionRatio();
    pViewCBuf->VTPageCacheCapacity.X = (float)PhysCacheVT->GetPageCacheCapacityX();
    pViewCBuf->VTPageCacheCapacity.Y = (float)PhysCacheVT->GetPageCacheCapacityY();

    pViewCBuf->VTPageTranslationOffsetAndScale = PhysCacheVT->GetPageTranslationOffsetAndScale();

    pViewCBuf->ViewPosition = GRenderView->ViewPosition;
    pViewCBuf->TimeDelta = GRenderView->GameplayTimeStep;

    pViewCBuf->PostprocessBloomMix = Float4( r_BloomParam0.GetFloat(),
                                               r_BloomParam1.GetFloat(),
                                               r_BloomParam2.GetFloat(),
                                               r_BloomParam3.GetFloat() ) * r_BloomScale.GetFloat();

    pViewCBuf->BloomEnabled = r_Bloom;  // TODO: Get from GRenderView
    pViewCBuf->ToneMappingExposure = r_ToneExposure.GetFloat();  // TODO: Get from GRenderView
    pViewCBuf->ColorGrading = GRenderView->CurrentColorGradingLUT ? 1.0f : 0.0f;
    pViewCBuf->FXAA = r_FXAA;
    pViewCBuf->VignetteColorIntensity = GRenderView->VignetteColorIntensity;
    pViewCBuf->VignetteOuterRadiusSqr = GRenderView->VignetteOuterRadiusSqr;
    pViewCBuf->VignetteInnerRadiusSqr = GRenderView->VignetteInnerRadiusSqr;
    pViewCBuf->ColorGradingAdaptationSpeed = GRenderView->ColorGradingAdaptationSpeed;
    pViewCBuf->ViewBrightness = Math::Saturate( r_Brightness.GetFloat() );

    pViewCBuf->SSLRSampleOffset = r_SSLRSampleOffset.GetFloat();
    pViewCBuf->SSLRMaxDist = r_SSLRMaxDist.GetFloat();
    pViewCBuf->IsPerspective = float( GRenderView->bPerspective );
    pViewCBuf->TessellationLevel = r_TessellationLevel.GetFloat() * Math::Lerp( (float)GRenderView->Width, (float)GRenderView->Height, 0.5f );

    pViewCBuf->PrefilteredMapSampler = (uint64_t)GPrefilteredMapBindless->GetHandle();
    pViewCBuf->IrradianceMapSampler = (uint64_t)GIrradianceMapBindless->GetHandle();

    pViewCBuf->DebugMode = r_DebugRenderMode.GetInteger();

    pViewCBuf->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //GLogger.Printf( "GRenderView->FirstDirectionalLight: %d\n", GRenderView->FirstDirectionalLight );

    for ( int i = 0 ; i < GRenderView->NumDirectionalLights ; i++ ) {
        SDirectionalLightInstance * light = GFrameData->DirectionalLights[GRenderView->FirstDirectionalLight + i];

        pViewCBuf->LightDirs[i] = Float4( GRenderView->NormalToViewMatrix * (light->Matrix[2]), 0.0f );
        pViewCBuf->LightColors[i] = light->ColorAndAmbientIntensity;
        pViewCBuf->LightParameters[i][0] = light->RenderMask;
        pViewCBuf->LightParameters[i][1] = light->FirstCascade;
        pViewCBuf->LightParameters[i][2] = light->NumCascades;
    }

    GViewConstantBufferBindingBindingOffset = offset;
    GViewConstantBufferBindingBindingSize = sizeof( *pViewCBuf );
    rtbl->BindBuffer( 0, GStreamBuffer, GViewConstantBufferBindingBindingOffset, GViewConstantBufferBindingBindingSize );
}

void ARenderBackend::UploadShaderResources()
{
    SetViewConstants();

    // Bind light buffer
    rtbl->BindBuffer( 4, GStreamBuffer, GRenderView->PointLightsStreamHandle, GRenderView->PointLightsStreamSize );

    // Bind IBL buffer
    rtbl->BindBuffer( 5, GStreamBuffer, GRenderView->ProbeStreamHandle, GRenderView->ProbeStreamSize );

    // Copy cluster data

#if 1
    // Perform copy from stream buffer on GPU side
    RenderCore::STextureRect rect = {};
    rect.Dimension.X = MAX_FRUSTUM_CLUSTERS_X;
    rect.Dimension.Y = MAX_FRUSTUM_CLUSTERS_Y;
    rect.Dimension.Z = MAX_FRUSTUM_CLUSTERS_Z;
    rcmd->CopyBufferToTexture( GStreamBuffer, GClusterLookup, rect, RenderCore::FORMAT_UINT2, 0, GRenderView->ClusterLookupStreamHandle, 1 );
#else
    GClusterLookup->Write( 0,
                          RenderCore::FORMAT_UINT2,
                          sizeof( SClusterHeader )*MAX_FRUSTUM_CLUSTERS_X*MAX_FRUSTUM_CLUSTERS_Y*MAX_FRUSTUM_CLUSTERS_Z,
                          1,
                          GRenderView->LightData.ClusterLookup );
#endif

#if 1
    // Perform copy from stream buffer on GPU side
    if ( GRenderView->ClusterPackedIndexCount > 0 ) {
#if 0
        RenderCore::SBufferCopy range;
        range.SrcOffset = GRenderView->ClusterPackedIndicesStreamHandle;
        range.DstOffset = 0;
        range.SizeInBytes = sizeof( SClusterPackedIndex ) * GRenderView->ClusterPackedIndexCount;
        rcmd->CopyBufferRange( GStreamBuffer, GClusterItemBuffer, 1, &range );
#else
        size_t offset = GRenderView->ClusterPackedIndicesStreamHandle;
        size_t sizeInBytes = sizeof( SClusterPackedIndex ) * GRenderView->ClusterPackedIndexCount;
        GClusterItemTBO->UpdateRange( offset, sizeInBytes );
#endif
    }
#else
    GClusterItemBuffer->WriteRange( 0,
                                   sizeof( SClusterItemOffset )*GRenderView->LightData.NumClusterItems,
                                   GRenderView->LightData.ItemBuffer );
#endif
}

void ARenderBackend::RenderView( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture )
{
    GRenderView = pRenderView;
    GRenderViewArea.X = 0;
    GRenderViewArea.Y = 0;
    GRenderViewArea.Width = GRenderView->Width;
    GRenderViewArea.Height = GRenderView->Height;

    UploadShaderResources();

    rcmd->BindResourceTable( rtbl );

    bool bVirtualTexturing = FeedbackAnalyzerVT->HasBindings();

    if ( bVirtualTexturing ) {
        GRenderView->VTFeedback->Begin( GRenderView->Width, GRenderView->Height );
    }

    FrameRenderer->Render( *FrameGraph, bVirtualTexturing, PhysCacheVT, CapturedResources );
    FrameGraph->Execute( rcmd );

    if ( r_FrameGraphDebug ) {
        FrameGraph->Debug();
    }

    *ppViewTexture = CapturedResources.FinalTexture;

    if ( bVirtualTexturing ) {
        int FeedbackSize;
        const void * FeedbackData;
        GRenderView->VTFeedback->End( &FeedbackSize, &FeedbackData );
    
        FeedbackAnalyzerVT->AddFeedbackData( FeedbackSize, FeedbackData );
    }
}
