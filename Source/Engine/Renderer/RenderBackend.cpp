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

#include "RenderBackend.h"
#include "GPUSync.h"
#include "Material.h"
#include "CanvasRenderer.h"
#include "VT/VirtualTextureFeedback.h"
#include "IrradianceGenerator.h"
#include "EnvProbeGenerator.h"
#include "CubemapGenerator.h"
#include "AtmosphereRenderer.h"

#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/ScopedTimeCheck.h>

#include <Core/Public/WindowsDefs.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/Logger.h>
#include <Core/Public/Image.h>

#include <SDL.h>

ARuntimeVariable r_SwapInterval( _CTS( "r_SwapInterval" ), _CTS( "0" ), 0, _CTS( "1 - enable vsync, 0 - disable vsync, -1 - tearing" ) );
ARuntimeVariable r_FrameGraphDebug( _CTS( "r_FrameGraphDebug" ), _CTS( "0" ) );
ARuntimeVariable r_RenderSnapshot( _CTS( "r_RenderSnapshot" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable r_UpdateFrameGraph( _CTS( "r_UpdateFrameGraph" ), _CTS( "1" ) );
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
ARuntimeVariable r_SSLR( _CTS( "r_SSLR" ), _CTS( "1" ) );
ARuntimeVariable r_SSLRMaxDist( _CTS( "r_SSLRMaxDist" ), _CTS( "10" ) );
ARuntimeVariable r_SSLRSampleOffset( _CTS( "r_SSLRSampleOffset" ), _CTS( "0.1" ) );
ARuntimeVariable r_HBAO( _CTS( "r_HBAO" ), _CTS( "1" ) );
ARuntimeVariable r_FXAA( _CTS( "r_FXAA" ), _CTS( "1" ) );
ARuntimeVariable r_ShowGPUTime( _CTS( "r_ShowGPUTime" ), _CTS( "0" ) );

void TestVT();

ARenderBackend GRenderBackendLocal;

static RenderCore::DATA_FORMAT PixelFormatTable[256];
static RenderCore::TEXTURE_FORMAT InternalPixelFormatTable[256];

static SDL_Window * WindowHandle;

static int TotalAllocatedRenderCore = 0;

static SDL_Window * CreateGenericWindow( SVideoMode const & _VideoMode )
{
    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;

    bool bOpenGL = true;

    SDL_InitSubSystem( SDL_INIT_VIDEO );

    if ( bOpenGL ) {
        flags |= SDL_WINDOW_OPENGL;

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 5 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_EGL, 0 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, 0 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS,
                             SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
#ifdef AN_DEBUG
                             | SDL_GL_CONTEXT_DEBUG_FLAG
#endif
        );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0 );
        SDL_GL_SetAttribute( SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1 );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_RELEASE_BEHAVIOR,  );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_RESET_NOTIFICATION,  );
        //SDL_GL_SetAttribute( SDL_GL_CONTEXT_NO_ERROR,  );
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0 );
        SDL_GL_SetAttribute( SDL_GL_STEREO, 0 );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );
        //SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,  );
        //SDL_GL_SetAttribute( SDL_GL_RETAINED_BACKING,  );
    }

    int x, y;

    if ( _VideoMode.bFullscreen ) {
        flags |= SDL_WINDOW_FULLSCREEN;// | SDL_WINDOW_BORDERLESS;
        x = y = 0;
    }
    else {
        if ( _VideoMode.bCentrized ) {
            x = SDL_WINDOWPOS_CENTERED;
            y = SDL_WINDOWPOS_CENTERED;
        }
        else {
            x = _VideoMode.WindowedX;
            y = _VideoMode.WindowedY;
        }
    }

    return SDL_CreateWindow( _VideoMode.Title, x, y, _VideoMode.Width, _VideoMode.Height, flags );
}

static void DestroyGenericWindow( SDL_Window * Window )
{
    if ( Window ) {
        SDL_DestroyWindow( Window );
        SDL_QuitSubSystem( SDL_INIT_VIDEO );
    }
}

void ARenderBackend::Initialize( SVideoMode const & _VideoMode )
{
    using namespace RenderCore;

    GLogger.Printf( "Initializing render backend...\n" );

    WindowHandle = CreateGenericWindow( _VideoMode );
    if ( !WindowHandle ) {
        CriticalError( "Failed to create main window\n" );
    }

    SImmediateContextCreateInfo stateCreateInfo = {};
    stateCreateInfo.ClipControl = CLIP_CONTROL_DIRECTX;
    stateCreateInfo.ViewportOrigin = VIEWPORT_ORIGIN_TOP_LEFT;
    stateCreateInfo.SwapInterval = r_SwapInterval.GetInteger();
    stateCreateInfo.Window = WindowHandle;

    SAllocatorCallback allocator;

    allocator.Allocate =
        []( size_t _BytesCount )
        {
            TotalAllocatedRenderCore++;
            return GZoneMemory.Alloc( _BytesCount );
        };

    allocator.Deallocate =
        []( void * _Bytes )
        {
            TotalAllocatedRenderCore--;
            GZoneMemory.Free( _Bytes );
        };

    CreateLogicalDevice( stateCreateInfo,
                         &allocator,
                         []( const unsigned char * _Data, int _Size )
                         {
                             return Core::Hash( ( const char * )_Data, _Size );
                         },
                         &GDevice );

    GDevice->GetImmediateContext( &rcmd );

    rtbl = rcmd->GetRootResourceTable();

    UniformBufferOffsetAlignment = GDevice->GetDeviceCaps( DEVICE_CAPS_UNIFORM_BUFFER_OFFSET_ALIGNMENT );

    Core::ZeroMem( PixelFormatTable, sizeof( PixelFormatTable ) );
    Core::ZeroMem( InternalPixelFormatTable, sizeof( InternalPixelFormatTable ) );

    PixelFormatTable[TEXTURE_PF_R8_SNORM] = FORMAT_BYTE1;
    PixelFormatTable[TEXTURE_PF_RG8_SNORM] = FORMAT_BYTE2;
    PixelFormatTable[TEXTURE_PF_BGR8_SNORM] = FORMAT_BYTE3;
    PixelFormatTable[TEXTURE_PF_BGRA8_SNORM] = FORMAT_BYTE4;

    PixelFormatTable[TEXTURE_PF_R8_UNORM] = FORMAT_UBYTE1;
    PixelFormatTable[TEXTURE_PF_RG8_UNORM] = FORMAT_UBYTE2;
    PixelFormatTable[TEXTURE_PF_BGR8_UNORM] = FORMAT_UBYTE3;
    PixelFormatTable[TEXTURE_PF_BGRA8_UNORM] = FORMAT_UBYTE4;

    PixelFormatTable[TEXTURE_PF_BGR8_SRGB] = FORMAT_UBYTE3;
    PixelFormatTable[TEXTURE_PF_BGRA8_SRGB] = FORMAT_UBYTE4;

    PixelFormatTable[TEXTURE_PF_R16I] = FORMAT_SHORT1;
    PixelFormatTable[TEXTURE_PF_RG16I] = FORMAT_SHORT2;
    PixelFormatTable[TEXTURE_PF_BGR16I] = FORMAT_SHORT3;
    PixelFormatTable[TEXTURE_PF_BGRA16I] = FORMAT_SHORT4;

    PixelFormatTable[TEXTURE_PF_R16UI] = FORMAT_USHORT1;
    PixelFormatTable[TEXTURE_PF_RG16UI] = FORMAT_USHORT2;
    PixelFormatTable[TEXTURE_PF_BGR16UI] = FORMAT_USHORT3;
    PixelFormatTable[TEXTURE_PF_BGRA16UI] = FORMAT_USHORT4;

    PixelFormatTable[TEXTURE_PF_R32I] = FORMAT_INT1;
    PixelFormatTable[TEXTURE_PF_RG32I] = FORMAT_INT2;
    PixelFormatTable[TEXTURE_PF_BGR32I] = FORMAT_INT3;
    PixelFormatTable[TEXTURE_PF_BGRA32I] = FORMAT_INT4;

    PixelFormatTable[TEXTURE_PF_R32I] = FORMAT_UINT1;
    PixelFormatTable[TEXTURE_PF_RG32UI] = FORMAT_UINT2;
    PixelFormatTable[TEXTURE_PF_BGR32UI] = FORMAT_UINT3;
    PixelFormatTable[TEXTURE_PF_BGRA32UI] = FORMAT_UINT4;

    PixelFormatTable[TEXTURE_PF_R16F] = FORMAT_HALF1;
    PixelFormatTable[TEXTURE_PF_RG16F] = FORMAT_HALF2;
    PixelFormatTable[TEXTURE_PF_BGR16F] = FORMAT_HALF3;
    PixelFormatTable[TEXTURE_PF_BGRA16F] = FORMAT_HALF4;

    PixelFormatTable[TEXTURE_PF_R32F] = FORMAT_FLOAT1;
    PixelFormatTable[TEXTURE_PF_RG32F] = FORMAT_FLOAT2;
    PixelFormatTable[TEXTURE_PF_BGR32F] = FORMAT_FLOAT3;
    PixelFormatTable[TEXTURE_PF_BGRA32F] = FORMAT_FLOAT4;

    PixelFormatTable[TEXTURE_PF_R11F_G11F_B10F] = FORMAT_FLOAT3;

    InternalPixelFormatTable[TEXTURE_PF_R8_SNORM] = TEXTURE_FORMAT_R8_SNORM;
    InternalPixelFormatTable[TEXTURE_PF_RG8_SNORM] = TEXTURE_FORMAT_RG8_SNORM;
    InternalPixelFormatTable[TEXTURE_PF_BGR8_SNORM] = TEXTURE_FORMAT_RGB8_SNORM;
    InternalPixelFormatTable[TEXTURE_PF_BGRA8_SNORM] = TEXTURE_FORMAT_RGBA8_SNORM;

    InternalPixelFormatTable[TEXTURE_PF_R8_UNORM] = TEXTURE_FORMAT_R8;
    InternalPixelFormatTable[TEXTURE_PF_RG8_UNORM] = TEXTURE_FORMAT_RG8;
    InternalPixelFormatTable[TEXTURE_PF_BGR8_UNORM] = TEXTURE_FORMAT_RGB8;
    InternalPixelFormatTable[TEXTURE_PF_BGRA8_UNORM] = TEXTURE_FORMAT_RGBA8;

    InternalPixelFormatTable[TEXTURE_PF_BGR8_SRGB] = TEXTURE_FORMAT_SRGB8;
    InternalPixelFormatTable[TEXTURE_PF_BGRA8_SRGB] = TEXTURE_FORMAT_SRGB8_ALPHA8;

    InternalPixelFormatTable[TEXTURE_PF_R16I] = TEXTURE_FORMAT_R16I;
    InternalPixelFormatTable[TEXTURE_PF_RG16I] = TEXTURE_FORMAT_RG16I;
    InternalPixelFormatTable[TEXTURE_PF_BGR16I] = TEXTURE_FORMAT_RGB16I;
    InternalPixelFormatTable[TEXTURE_PF_BGRA16I] = TEXTURE_FORMAT_RGBA16I;

    InternalPixelFormatTable[TEXTURE_PF_R16UI] = TEXTURE_FORMAT_R16UI;
    InternalPixelFormatTable[TEXTURE_PF_RG16UI] = TEXTURE_FORMAT_RG16UI;
    InternalPixelFormatTable[TEXTURE_PF_BGR16UI] = TEXTURE_FORMAT_RGB16UI;
    InternalPixelFormatTable[TEXTURE_PF_BGRA16UI] = TEXTURE_FORMAT_RGBA16UI;

    InternalPixelFormatTable[TEXTURE_PF_R32I] = TEXTURE_FORMAT_R32I;
    InternalPixelFormatTable[TEXTURE_PF_RG32I] = TEXTURE_FORMAT_RG32I;
    InternalPixelFormatTable[TEXTURE_PF_BGR32I] = TEXTURE_FORMAT_RGB32I;
    InternalPixelFormatTable[TEXTURE_PF_BGRA32I] = TEXTURE_FORMAT_RGBA32I;

    InternalPixelFormatTable[TEXTURE_PF_R32I] = TEXTURE_FORMAT_R32UI;
    InternalPixelFormatTable[TEXTURE_PF_RG32UI] = TEXTURE_FORMAT_RG32UI;
    InternalPixelFormatTable[TEXTURE_PF_BGR32UI] = TEXTURE_FORMAT_RGB32UI;
    InternalPixelFormatTable[TEXTURE_PF_BGRA32UI] = TEXTURE_FORMAT_RGBA32UI;

    InternalPixelFormatTable[TEXTURE_PF_R16F] = TEXTURE_FORMAT_R16F;
    InternalPixelFormatTable[TEXTURE_PF_RG16F] = TEXTURE_FORMAT_RG16F;
    InternalPixelFormatTable[TEXTURE_PF_BGR16F] = TEXTURE_FORMAT_RGB16F;
    InternalPixelFormatTable[TEXTURE_PF_BGRA16F] = TEXTURE_FORMAT_RGBA16F;

    InternalPixelFormatTable[TEXTURE_PF_R32F] = TEXTURE_FORMAT_R32F;
    InternalPixelFormatTable[TEXTURE_PF_RG32F] = TEXTURE_FORMAT_RG32F;
    InternalPixelFormatTable[TEXTURE_PF_BGR32F] = TEXTURE_FORMAT_RGB32F;
    InternalPixelFormatTable[TEXTURE_PF_BGRA32F] = TEXTURE_FORMAT_RGBA32F;

    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC1_RGB]        = TEXTURE_FORMAT_COMPRESSED_BC1_RGB;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC1_SRGB]       = TEXTURE_FORMAT_COMPRESSED_BC1_SRGB;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC2_RGBA]       = TEXTURE_FORMAT_COMPRESSED_BC2_RGBA;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC2_SRGB_ALPHA] = TEXTURE_FORMAT_COMPRESSED_BC2_SRGB_ALPHA;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC3_RGBA]       = TEXTURE_FORMAT_COMPRESSED_BC3_RGBA;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC3_SRGB_ALPHA] = TEXTURE_FORMAT_COMPRESSED_BC3_SRGB_ALPHA;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC4_R]          = TEXTURE_FORMAT_COMPRESSED_BC4_R;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC4_R_SIGNED]   = TEXTURE_FORMAT_COMPRESSED_BC4_R_SIGNED;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC5_RG]         = TEXTURE_FORMAT_COMPRESSED_BC5_RG;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC5_RG_SIGNED]  = TEXTURE_FORMAT_COMPRESSED_BC5_RG_SIGNED;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC6H]           = TEXTURE_FORMAT_COMPRESSED_BC6H;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC6H_SIGNED]    = TEXTURE_FORMAT_COMPRESSED_BC6H_SIGNED;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC7_RGBA]       = TEXTURE_FORMAT_COMPRESSED_BC7_RGBA;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC7_SRGB_ALPHA] = TEXTURE_FORMAT_COMPRESSED_BC7_SRGB_ALPHA;

    InternalPixelFormatTable[TEXTURE_PF_R11F_G11F_B10F] = TEXTURE_FORMAT_R11F_G11F_B10F;

    InitMaterialSamplers();

    FrameGraph = MakeRef< AFrameGraph >( GDevice );
    FrameRenderer = MakeRef< AFrameRenderer >();
    CanvasRenderer = MakeRef< ACanvasRenderer >();

    GConstantBuffer = MakeRef< ACircularBuffer >( 2 * 1024 * 1024 ); // 2MB
    GFrameConstantBuffer = MakeRef< AFrameConstantBuffer >( 2 * 1024 * 1024 ); // 2MB

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
        // FIXME: Use SSBO?
        RenderCore::SBufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = RenderCore::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = sizeof( SFrameLightData::ItemBuffer );
        GDevice->CreateBuffer( bufferCI, nullptr, &GClusterItemBuffer );

        RenderCore::SBufferViewCreateInfo bufferViewCI = {};
        bufferViewCI.Format = RenderCore::BUFFER_VIEW_PIXEL_FORMAT_R32UI;
        GDevice->CreateBufferView( bufferViewCI, GClusterItemBuffer, &GClusterItemTBO );
    }

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////

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
            "ClearSky/rt.bmp",
            "ClearSky/lt.bmp",
            "ClearSky/up.bmp",
            "ClearSky/dn.bmp",
            "ClearSky/bk.bmp",
            "ClearSky/ft.bmp"
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

            RenderCore::STextureRect rect = {};
            rect.Offset.Z = face;
            rect.Dimension.X = w;
            rect.Dimension.Y = w;
            rect.Dimension.Z = 1;

            cubemap2->WriteRect( rect, RenderCore::FORMAT_FLOAT3, w*w*3*sizeof( float ), 1, pSrc );
        }
    }

    RenderCore::ITexture * cubemaps[2] = { /*cubemap*/skybox, cubemap2 };
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
    VTWorkflow = MakeRef< SVirtualTextureWorkflow >( createInfo );

    //::TestVT();
    TestVT = VTWorkflow->PhysCache.CreateVirtualTexture( "Test.vt3" );

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
    TPodArray<int> pageSizeX; pageSizeX.Resize( numPageSizes );
    TPodArray<int> pageSizeY; pageSizeY.Resize( numPageSizes );
    TPodArray<int> pageSizeZ; pageSizeZ.Resize( numPageSizes );
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
}

void ARenderBackend::Deinitialize()
{
    GLogger.Printf( "Deinitializing render backend...\n" );

    TimeQuery.Reset();
    TimeStamp1.Reset();
    TimeStamp2.Reset();
    CanvasRenderer.Reset();
    FrameRenderer.Reset();
    FrameGraph.Reset();
    VTWorkflow.Reset();

    GConstantBuffer.Reset();
    GFrameConstantBuffer.Reset();
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

    GPUSync.Release();

    GDevice.Reset();

    DestroyGenericWindow( WindowHandle );
    WindowHandle = nullptr;

    GLogger.Printf( "TotalAllocatedRenderCore: %d\n", TotalAllocatedRenderCore );
}

void * ARenderBackend::GetMainWindow()
{
    return WindowHandle;
}

void ARenderBackend::WaitGPU()
{
    GPUSync.Wait();
}

void ARenderBackend::SetGPUEvent()
{
    GPUSync.SetEvent();
}

void * ARenderBackend::FenceSync()
{
    return rcmd->FenceSync();
}

void ARenderBackend::RemoveSync( void * _Sync )
{
    rcmd->RemoveSync( (RenderCore::SyncObject)_Sync );
}

void ARenderBackend::WaitSync( void * _Sync )
{
    const uint64_t timeOutNanoseconds = 1;
    if ( _Sync ) {
        RenderCore::CLIENT_WAIT_STATUS status;
        do {
            status = rcmd->ClientWait( (RenderCore::SyncObject)_Sync, timeOutNanoseconds );
        } while ( status != RenderCore::CLIENT_WAIT_ALREADY_SIGNALED && status != RenderCore::CLIENT_WAIT_CONDITION_SATISFIED );
    }
}

void ARenderBackend::ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem )
{
    RenderCore::SRect2D rect;
    rect.X = _X;
    rect.Y = _Y;
    rect.Width = _Width;
    rect.Height = _Height;

    RenderCore::IFramebuffer * framebuffer = rcmd->GetDefaultFramebuffer();

    framebuffer->Read( RenderCore::FB_BACK_DEFAULT, rect, RenderCore::FB_CHANNEL_RGBA, RenderCore::FB_UBYTE, RenderCore::COLOR_CLAMP_ON, _SizeInBytes, _Alignment, _SysMem );
}

static void SetTextureSwizzle( ETexturePixelFormat _PixelFormat, RenderCore::STextureSwizzle & _Swizzle )
{
    switch ( STexturePixelFormat( _PixelFormat ).NumComponents() ) {
    case 1:
        // Apply texture swizzle for one channel textures
        _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_R;
        break;
#if 0
    case 2:
        // Apply texture swizzle for two channel textures
        _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_G;
        _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_G;
        break;
    case 3:
        // Apply texture swizzle for three channel textures
        _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_G;
        _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_B;
        _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_ONE;
        break;
#endif
    }
}

void ARenderBackend::InitializeTexture1D( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_1D;
    textureCI.Resolution.Tex1D.Width = _Width;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::InitializeTexture1DArray( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_1D_ARRAY;
    textureCI.Resolution.Tex1DArray.Width = _Width;
    textureCI.Resolution.Tex1DArray.NumLayers = _ArraySize;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::InitializeTexture2D( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_2D;
    textureCI.Resolution.Tex2D.Width = _Width;
    textureCI.Resolution.Tex2D.Height = _Height;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::InitializeTexture2DArray( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_2D_ARRAY;
    textureCI.Resolution.Tex2DArray.Width = _Width;
    textureCI.Resolution.Tex2DArray.Height = _Height;
    textureCI.Resolution.Tex2DArray.NumLayers = _ArraySize;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::InitializeTexture3D( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_3D;
    textureCI.Resolution.Tex3D.Width = _Width;
    textureCI.Resolution.Tex3D.Height = _Height;
    textureCI.Resolution.Tex3D.Depth = _Depth;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::InitializeTextureCubemap( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP;
    textureCI.Resolution.TexCubemap.Width = _Width;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::InitializeTextureCubemapArray( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize )
{
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Resolution.TexCubemapArray.Width = _Width;
    textureCI.Resolution.TexCubemapArray.NumLayers = _ArraySize;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, ppTexture );
}

void ARenderBackend::WriteTexture( RenderCore::ITexture * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem )
{
    RenderCore::STextureRect rect;
    rect.Offset.X = _Rectangle.Offset.X;
    rect.Offset.Y = _Rectangle.Offset.Y;
    rect.Offset.Z = _Rectangle.Offset.Z;
    rect.Offset.Lod = _Rectangle.Offset.Lod;
    rect.Dimension.X = _Rectangle.Dimension.X;
    rect.Dimension.Y = _Rectangle.Dimension.Y;
    rect.Dimension.Z = _Rectangle.Dimension.Z;

    _Texture->WriteRect( rect, PixelFormatTable[_PixelFormat], _SizeInBytes, _Alignment, _SysMem );
}

void ARenderBackend::ReadTexture( RenderCore::ITexture * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem )
{
    RenderCore::STextureRect rect;
    rect.Offset.X = _Rectangle.Offset.X;
    rect.Offset.Y = _Rectangle.Offset.Y;
    rect.Offset.Z = _Rectangle.Offset.Z;
    rect.Offset.Lod = _Rectangle.Offset.Lod;
    rect.Dimension.X = _Rectangle.Dimension.X;
    rect.Dimension.Y = _Rectangle.Dimension.Y;
    rect.Dimension.Z = _Rectangle.Dimension.Z;

    _Texture->ReadRect( rect, PixelFormatTable[_PixelFormat], _SizeInBytes, _Alignment, _SysMem );
}

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

void * ARenderBackend::InitializePersistentMappedBuffer( TRef< RenderCore::IBuffer > * ppBuffer, size_t _SizeInBytes )
{
    RenderCore::SBufferCreateInfo bufferCI = {};

    bufferCI.SizeInBytes = _SizeInBytes;

    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)
            ( RenderCore::IMMUTABLE_MAP_WRITE | RenderCore::IMMUTABLE_MAP_PERSISTENT | RenderCore::IMMUTABLE_MAP_COHERENT );
    bufferCI.bImmutableStorage = true;

    GDevice->CreateBuffer( bufferCI, nullptr, ppBuffer );

    void * pMappedMemory = (*ppBuffer)->Map( RenderCore::MAP_TRANSFER_WRITE,
                                        RenderCore::MAP_NO_INVALIDATE,//RenderCore::MAP_INVALIDATE_ENTIRE_BUFFER,
                                        RenderCore::MAP_PERSISTENT_COHERENT,
                                        false, // flush explicit
                                        false  // unsynchronized
                                      );

    if ( !pMappedMemory ) {
        CriticalError( "ARenderBackend::InitializePersistentMappedBuffer: cannot initialize persistent mapped buffer size %d\n", _SizeInBytes );
    }

    return pMappedMemory;
}

void ARenderBackend::WriteBuffer( RenderCore::IBuffer * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem )
{
    _Buffer->WriteRange( _ByteOffset, _SizeInBytes, _SysMem );
}

void ARenderBackend::ReadBuffer( RenderCore::IBuffer * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem )
{
    _Buffer->ReadRange( _ByteOffset, _SizeInBytes, _SysMem );
}

void ARenderBackend::OrphanBuffer( RenderCore::IBuffer * _Buffer )
{
    _Buffer->Orphan();
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
    bool bRebuildMaterials = false;

    if ( r_SSLR.IsModified() ) {
        r_SSLR.UnmarkModified();
        bRebuildMaterials = true;
    }

    if ( r_HBAO.IsModified() ) {
        r_HBAO.UnmarkModified();
        bRebuildMaterials = true;
    }

    if ( bRebuildMaterials ) {
        GLogger.Printf( "Need to rebuild all materials\n" );
    }

    GFrameConstantBuffer->Begin();

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
    GStreamBuffer = GFrameData->StreamBuffer;

    rcmd->SetSwapChainResolution( GFrameData->CanvasWidth, GFrameData->CanvasHeight );

    VTWorkflow->FeedbackAnalyzer.Begin();

    // TODO: Bind virtual textures in one place
    VTWorkflow->FeedbackAnalyzer.BindTexture( 0, TestVT );

    CanvasRenderer->Render( [this]( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture )
    {
        RenderView( pRenderView, ppViewTexture );
    });

    VTWorkflow->FeedbackAnalyzer.End();

    // FIXME: Move it at beggining of the frame to give more time for stream thread?
    VTWorkflow->PhysCache.Update();

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

    SetGPUEvent();

    r_RenderSnapshot = false;

    GFrameConstantBuffer->End();
}

void ARenderBackend::SetViewUniforms()
{
    size_t offset = GFrameConstantBuffer->Allocate( sizeof( SViewUniformBuffer ) );

    SViewUniformBuffer * uniformData = (SViewUniformBuffer *)(GFrameConstantBuffer->GetMappedMemory() + offset);

    uniformData->OrthoProjection = GFrameData->OrthoProjection;
    uniformData->ViewProjection = GRenderView->ViewProjection;
    uniformData->ProjectionMatrix = GRenderView->ProjectionMatrix;
    uniformData->InverseProjectionMatrix = GRenderView->InverseProjectionMatrix;

    uniformData->InverseViewMatrix = GRenderView->ViewSpaceToWorldSpace;

    // Reprojection from viewspace to previous frame viewspace coordinates:
    // ViewspaceReprojection = WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    uniformData->ViewspaceReprojection = GRenderView->ViewMatrixP * GRenderView->ViewSpaceToWorldSpace;

    // Reprojection from viewspace to previous frame projected coordinates:
    // ReprojectionMatrix = ProjectionMatrixPrevFrame * WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    uniformData->ReprojectionMatrix = GRenderView->ProjectionMatrixP * uniformData->ViewspaceReprojection;

    uniformData->WorldNormalToViewSpace[0].X = GRenderView->NormalToViewMatrix[0][0];
    uniformData->WorldNormalToViewSpace[0].Y = GRenderView->NormalToViewMatrix[1][0];
    uniformData->WorldNormalToViewSpace[0].Z = GRenderView->NormalToViewMatrix[2][0];
    uniformData->WorldNormalToViewSpace[0].W = 0;

    uniformData->WorldNormalToViewSpace[1].X = GRenderView->NormalToViewMatrix[0][1];
    uniformData->WorldNormalToViewSpace[1].Y = GRenderView->NormalToViewMatrix[1][1];
    uniformData->WorldNormalToViewSpace[1].Z = GRenderView->NormalToViewMatrix[2][1];
    uniformData->WorldNormalToViewSpace[1].W = 0;

    uniformData->WorldNormalToViewSpace[2].X = GRenderView->NormalToViewMatrix[0][2];
    uniformData->WorldNormalToViewSpace[2].Y = GRenderView->NormalToViewMatrix[1][2];
    uniformData->WorldNormalToViewSpace[2].Z = GRenderView->NormalToViewMatrix[2][2];
    uniformData->WorldNormalToViewSpace[2].W = 0;

    uniformData->InvViewportSize.X = 1.0f / GRenderView->Width;
    uniformData->InvViewportSize.Y = 1.0f / GRenderView->Height;
    uniformData->ZNear = GRenderView->ViewZNear;
    uniformData->ZFar = GRenderView->ViewZFar;

    if ( GRenderView->bPerspective ) {
        uniformData->ProjectionInfo.X = -2.0f / GRenderView->ProjectionMatrix[0][0]; // (x) * (R - L)/N
        uniformData->ProjectionInfo.Y = 2.0f / GRenderView->ProjectionMatrix[1][1]; // (y) * (T - B)/N
        uniformData->ProjectionInfo.Z = (1.0f - GRenderView->ProjectionMatrix[2][0]) / GRenderView->ProjectionMatrix[0][0]; // L/N
        uniformData->ProjectionInfo.W = -(1.0f + GRenderView->ProjectionMatrix[2][1]) / GRenderView->ProjectionMatrix[1][1]; // B/N
    }
    else {
        uniformData->ProjectionInfo.X = 2.0f / GRenderView->ProjectionMatrix[0][0]; // (x) * R - L
        uniformData->ProjectionInfo.Y = -2.0f / GRenderView->ProjectionMatrix[1][1]; // (y) * T - B
        uniformData->ProjectionInfo.Z = -(1.0f + GRenderView->ProjectionMatrix[3][0]) / GRenderView->ProjectionMatrix[0][0]; // L
        uniformData->ProjectionInfo.W = (1.0f - GRenderView->ProjectionMatrix[3][1]) / GRenderView->ProjectionMatrix[1][1]; // B
    }

    uniformData->GameRunningTimeSeconds = GRenderView->GameRunningTimeSeconds;
    uniformData->GameplayTimeSeconds = GRenderView->GameplayTimeSeconds;

    uniformData->DynamicResolutionRatioX = (float)GRenderView->Width / GFrameData->AllocSurfaceWidth;
    uniformData->DynamicResolutionRatioY = (float)GRenderView->Height / GFrameData->AllocSurfaceHeight;
    uniformData->DynamicResolutionRatioPX = (float)GRenderView->WidthP / GFrameData->AllocSurfaceWidthP;
    uniformData->DynamicResolutionRatioPY = (float)GRenderView->HeightP / GFrameData->AllocSurfaceHeightP;

    uniformData->FeedbackBufferResolutionRatio = GRenderView->VTFeedback->GetResolutionRatio();
    uniformData->VTPageCacheCapacity.X = (float)GRenderBackendLocal.VTWorkflow->PhysCache.GetPageCacheCapacityX();
    uniformData->VTPageCacheCapacity.Y = (float)GRenderBackendLocal.VTWorkflow->PhysCache.GetPageCacheCapacityY();

    uniformData->VTPageTranslationOffsetAndScale = GRenderBackendLocal.VTWorkflow->PhysCache.GetPageTranslationOffsetAndScale();

    uniformData->ViewPosition = GRenderView->ViewPosition;
    uniformData->TimeDelta = GRenderView->GameplayTimeStep;

    uniformData->PostprocessBloomMix = Float4( r_BloomParam0.GetFloat(),
                                               r_BloomParam1.GetFloat(),
                                               r_BloomParam2.GetFloat(),
                                               r_BloomParam3.GetFloat() ) * r_BloomScale.GetFloat();

    uniformData->BloomEnabled = r_Bloom;  // TODO: Get from GRenderView
    uniformData->ToneMappingExposure = r_ToneExposure.GetFloat();  // TODO: Get from GRenderView
    uniformData->ColorGrading = GRenderView->CurrentColorGradingLUT ? 1.0f : 0.0f;
    uniformData->FXAA = r_FXAA;
    uniformData->VignetteColorIntensity = GRenderView->VignetteColorIntensity;
    uniformData->VignetteOuterRadiusSqr = GRenderView->VignetteOuterRadiusSqr;
    uniformData->VignetteInnerRadiusSqr = GRenderView->VignetteInnerRadiusSqr;
    uniformData->ColorGradingAdaptationSpeed = GRenderView->ColorGradingAdaptationSpeed;
    uniformData->ViewBrightness = Math::Saturate( r_Brightness.GetFloat() );

    uniformData->SSLRSampleOffset = r_SSLRSampleOffset.GetFloat();
    uniformData->SSLRMaxDist = r_SSLRMaxDist.GetFloat();
    uniformData->IsPerspective = float( GRenderView->bPerspective );
    uniformData->TessellationLevel = r_TessellationLevel.GetFloat() * Math::Lerp( (float)GRenderView->Width, (float)GRenderView->Height, 0.5f );

    uniformData->PrefilteredMapSampler = (uint64_t)GPrefilteredMapBindless->GetHandle();
    uniformData->IrradianceMapSampler = (uint64_t)GIrradianceMapBindless->GetHandle();

    uniformData->DebugMode = r_DebugRenderMode.GetInteger();

    uniformData->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //GLogger.Printf( "GRenderView->FirstDirectionalLight: %d\n", GRenderView->FirstDirectionalLight );

    for ( int i = 0 ; i < GRenderView->NumDirectionalLights ; i++ ) {
        SDirectionalLightDef * light = GFrameData->DirectionalLights[GRenderView->FirstDirectionalLight + i];

        uniformData->LightDirs[i] = Float4( GRenderView->NormalToViewMatrix * (light->Matrix[2]), 0.0f );
        uniformData->LightColors[i] = light->ColorAndAmbientIntensity;
        uniformData->LightParameters[i][0] = light->RenderMask;
        uniformData->LightParameters[i][1] = light->FirstCascade;
        uniformData->LightParameters[i][2] = light->NumCascades;
    }

    GViewUniformBufferBindingBindingOffset = offset;
    GViewUniformBufferBindingBindingSize = sizeof( *uniformData );
    rtbl->BindBuffer( 0, GFrameConstantBuffer->GetBuffer(), GViewUniformBufferBindingBindingOffset, GViewUniformBufferBindingBindingSize );
}

void ARenderBackend::UploadShaderResources()
{
    SetViewUniforms();

    // Cascade matrices
    GShadowMatrixBindingSize = MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof( Float4x4 );
    GShadowMatrixBindingOffset = GFrameConstantBuffer->Allocate( GShadowMatrixBindingSize );

    byte * pMemory = GFrameConstantBuffer->GetMappedMemory() + GShadowMatrixBindingOffset;

    Core::Memcpy( pMemory, GRenderView->ShadowMapMatrices, GRenderView->NumShadowMapCascades * sizeof( Float4x4 ) );

    // Light buffer
    size_t LightBufferBindingBindingSize = GRenderView->NumPointLights * sizeof( SClusterLight );
    size_t LightBufferBindingBindingOffset = GFrameConstantBuffer->Allocate( LightBufferBindingBindingSize );

    rtbl->BindBuffer( 4, GFrameConstantBuffer->GetBuffer(), LightBufferBindingBindingOffset, LightBufferBindingBindingSize );

    pMemory = GFrameConstantBuffer->GetMappedMemory() + LightBufferBindingBindingOffset;

    Core::Memcpy( pMemory, GRenderView->PointLights, LightBufferBindingBindingSize );

    // IBL buffer
    size_t IBLBufferBindingBindingSize = GRenderView->NumProbes * sizeof( SClusterProbe );
    size_t IBLBufferBindingBindingOffset = GFrameConstantBuffer->Allocate( IBLBufferBindingBindingSize );

    rtbl->BindBuffer( 5, GFrameConstantBuffer->GetBuffer(), IBLBufferBindingBindingOffset, IBLBufferBindingBindingSize );

    pMemory = GFrameConstantBuffer->GetMappedMemory() + IBLBufferBindingBindingOffset;

    Core::Memcpy( pMemory, GRenderView->Probes, IBLBufferBindingBindingSize );

    // Write cluster data
    GClusterLookup->Write( 0,
                          RenderCore::FORMAT_UINT2,
                          sizeof( SClusterData )*MAX_FRUSTUM_CLUSTERS_X*MAX_FRUSTUM_CLUSTERS_Y*MAX_FRUSTUM_CLUSTERS_Z,
                          1,
                          GRenderView->LightData.ClusterLookup );

    GClusterItemBuffer->WriteRange( 0,
                                   sizeof( SClusterItemBuffer )*GRenderView->LightData.TotalItems,
                                   GRenderView->LightData.ItemBuffer );
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

    bool bVirtualTexturing = VTWorkflow->FeedbackAnalyzer.HasBindings();

    if ( bVirtualTexturing ) {
        GRenderView->VTFeedback->Begin( GRenderView->Width, GRenderView->Height );
    }

    if ( r_UpdateFrameGraph ) {
        FrameRenderer->Render( *FrameGraph, bVirtualTexturing ? VTWorkflow.GetObject() : nullptr, CapturedResources );
    }
    
    FrameGraph->Execute( rcmd );

    if ( r_FrameGraphDebug ) {
        FrameGraph->Debug();
    }

    *ppViewTexture = CapturedResources.FinalTexture;

    if ( bVirtualTexturing ) {
        int FeedbackSize;
        const void * FeedbackData;
        GRenderView->VTFeedback->End( &FeedbackSize, &FeedbackData );
    
        VTWorkflow->FeedbackAnalyzer.AddFeedbackData( FeedbackSize, FeedbackData );
    }
}

void ARenderBackend::SwapBuffers()
{
    GDevice->SwapBuffers( WindowHandle, r_SwapInterval.GetInteger() );
}
