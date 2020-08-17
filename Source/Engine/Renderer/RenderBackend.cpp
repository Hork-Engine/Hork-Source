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

#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/ScopedTimeCheck.h>

#include <Core/Public/WindowsDefs.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/Logger.h>
#include <Core/Public/Image.h>

#include <SDL.h>

ARuntimeVariable RVRenderSnapshot( _CTS("RenderSnapshot"), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVRebuildGraph( _CTS( "RebuildGraph" ), _CTS( "1" ) );

void TestVT();

ARenderBackend GOpenGL45RenderBackend;

static RenderCore::DATA_FORMAT PixelFormatTable[256];
static RenderCore::TEXTURE_FORMAT InternalPixelFormatTable[256];

static SDL_Window * WindowHandle;

//
// GHI import
//

static int TotalAllocatedGHI = 0;

static int GHIImport_Hash( const unsigned char * _Data, int _Size ) {
    return Core::Hash( ( const char * )_Data, _Size );
}

static void * GHIImport_Allocate( size_t _BytesCount ) {
    TotalAllocatedGHI++;
    return GZoneMemory.Alloc( _BytesCount );
}

static void GHIImport_Deallocate( void * _Bytes ) {
    TotalAllocatedGHI--;
    GZoneMemory.Free( _Bytes );
}

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
    } else {
        if ( _VideoMode.bCentrized ) {
            x = SDL_WINDOWPOS_CENTERED;
            y = SDL_WINDOWPOS_CENTERED;
        } else {
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

void ARenderBackend::Initialize( SVideoMode const & _VideoMode ) {
    using namespace RenderCore;

    GLogger.Printf( "Initializing render backend...\n" );

    WindowHandle = CreateGenericWindow( _VideoMode );
    if ( !WindowHandle ) {
        CriticalError( "Failed to create main window\n" );
    }

    SImmediateContextCreateInfo stateCreateInfo = {};
    stateCreateInfo.ClipControl = CLIP_CONTROL_DIRECTX;
    stateCreateInfo.ViewportOrigin = VIEWPORT_ORIGIN_TOP_LEFT;
    stateCreateInfo.Window = WindowHandle;

    SAllocatorCallback allocator;

    allocator.Allocate = GHIImport_Allocate;
    allocator.Deallocate = GHIImport_Deallocate;

    CreateLogicalDevice( stateCreateInfo, &allocator, GHIImport_Hash, &GDevice );
    GDevice->GetImmediateContext( &rcmd );

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

    FrameGraph = MakeRef< AFrameGraph >( GDevice );
    FrameRenderer = MakeRef< AFrameRenderer >();
    CanvasRenderer = MakeRef< ACanvasRenderer >();

    GFrameResources.Initialize();

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
}

void ARenderBackend::Deinitialize() {
    GLogger.Printf( "Deinitializing render backend...\n" );

    CanvasRenderer.Reset();
    FrameRenderer.Reset();
    FrameGraph.Reset();
    VTWorkflow.Reset();

    GFrameResources.Deinitialize();
    GPUSync.Release();

    GDevice.Reset();

    DestroyGenericWindow( WindowHandle );
    WindowHandle = nullptr;

    GLogger.Printf( "TotalAllocatedGHI: %d\n", TotalAllocatedGHI );
}

void * ARenderBackend::GetMainWindow() {
    return WindowHandle;
}

void ARenderBackend::WaitGPU() {
    GPUSync.Wait();
}

void ARenderBackend::SetGPUEvent() {
    GPUSync.SetEvent();
}

void * ARenderBackend::FenceSync() {
    return rcmd->FenceSync();
}

void ARenderBackend::RemoveSync( void * _Sync ) {
    rcmd->RemoveSync( (RenderCore::SyncObject)_Sync );
}

void ARenderBackend::WaitSync( void * _Sync ) {
    const uint64_t timeOutNanoseconds = 1;
    if ( _Sync ) {
        RenderCore::CLIENT_WAIT_STATUS status;
        do {
            status = rcmd->ClientWait( (RenderCore::SyncObject)_Sync, timeOutNanoseconds );
        } while ( status != RenderCore::CLIENT_WAIT_ALREADY_SIGNALED && status != RenderCore::CLIENT_WAIT_CONDITION_SATISFIED );
    }
}

void ARenderBackend::ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) {
    RenderCore::SRect2D rect;
    rect.X = _X;
    rect.Y = _Y;
    rect.Width = _Width;
    rect.Height = _Height;

    RenderCore::IFramebuffer * framebuffer = rcmd->GetDefaultFramebuffer();

    framebuffer->Read( RenderCore::FB_BACK_DEFAULT, rect, RenderCore::FB_CHANNEL_RGBA, RenderCore::FB_UBYTE, RenderCore::COLOR_CLAMP_ON, _SizeInBytes, _Alignment, _SysMem );
}

ATextureGPU * ARenderBackend::CreateTexture( IGPUResourceOwner * _Owner ) {
    return CreateResource< ATextureGPU >( _Owner );
}

void ARenderBackend::DestroyTexture( ATextureGPU * _Texture ) {
    DestroyResource( _Texture );
}

static void SetTextureSwizzle( ETexturePixelFormat _PixelFormat, RenderCore::STextureSwizzle & _Swizzle ) {
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

void ARenderBackend::InitializeTexture1D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_1D;
    textureCI.Resolution.Tex1D.Width = _Width;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::InitializeTexture1DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_1D_ARRAY;
    textureCI.Resolution.Tex1DArray.Width = _Width;
    textureCI.Resolution.Tex1DArray.NumLayers = _ArraySize;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::InitializeTexture2D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_2D;
    textureCI.Resolution.Tex2D.Width = _Width;
    textureCI.Resolution.Tex2D.Height = _Height;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::InitializeTexture2DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_2D_ARRAY;
    textureCI.Resolution.Tex2DArray.Width = _Width;
    textureCI.Resolution.Tex2DArray.Height = _Height;
    textureCI.Resolution.Tex2DArray.NumLayers = _ArraySize;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::InitializeTexture3D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_3D;
    textureCI.Resolution.Tex3D.Width = _Width;
    textureCI.Resolution.Tex3D.Height = _Height;
    textureCI.Resolution.Tex3D.Depth = _Depth;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::InitializeTextureCubemap( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP;
    textureCI.Resolution.TexCubemap.Width = _Width;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::InitializeTextureCubemapArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    RenderCore::STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Resolution.TexCubemapArray.Width = _Width;
    textureCI.Resolution.TexCubemapArray.NumLayers = _ArraySize;
    textureCI.Format = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    GDevice->CreateTexture( textureCI, &_Texture->pTexture );
}

void ARenderBackend::WriteTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) {
    RenderCore::ITexture * texture = GPUTextureHandle( _Texture );

    RenderCore::STextureRect rect;
    rect.Offset.X = _Rectangle.Offset.X;
    rect.Offset.Y = _Rectangle.Offset.Y;
    rect.Offset.Z = _Rectangle.Offset.Z;
    rect.Offset.Lod = _Rectangle.Offset.Lod;
    rect.Dimension.X = _Rectangle.Dimension.X;
    rect.Dimension.Y = _Rectangle.Dimension.Y;
    rect.Dimension.Z = _Rectangle.Dimension.Z;

    texture->WriteRect( rect, PixelFormatTable[_PixelFormat], _SizeInBytes, _Alignment, _SysMem );
}

void ARenderBackend::ReadTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) {
    RenderCore::ITexture * texture = GPUTextureHandle( _Texture );

    RenderCore::STextureRect rect;
    rect.Offset.X = _Rectangle.Offset.X;
    rect.Offset.Y = _Rectangle.Offset.Y;
    rect.Offset.Z = _Rectangle.Offset.Z;
    rect.Offset.Lod = _Rectangle.Offset.Lod;
    rect.Dimension.X = _Rectangle.Dimension.X;
    rect.Dimension.Y = _Rectangle.Dimension.Y;
    rect.Dimension.Z = _Rectangle.Dimension.Z;

    texture->ReadRect( rect, PixelFormatTable[_PixelFormat], _SizeInBytes, _Alignment, _SysMem );
}

ABufferGPU * ARenderBackend::CreateBuffer( IGPUResourceOwner * _Owner ) {
    return CreateResource< ABufferGPU >( _Owner );
}

void ARenderBackend::DestroyBuffer( ABufferGPU * _Buffer ) {
    DestroyResource( _Buffer );
}

void ARenderBackend::InitializeBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes ) {
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

        GDevice->CreateBuffer( bufferCI, nullptr, &_Buffer->pBuffer );
    } else {
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

        GDevice->CreateBuffer( bufferCI, nullptr, &_Buffer->pBuffer );
    }
}

void * ARenderBackend::InitializePersistentMappedBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes ) {
    //RenderCore::Buffer * buffer = GPUBufferHandle( _Buffer );

    RenderCore::SBufferCreateInfo bufferCI = {};

    bufferCI.SizeInBytes = _SizeInBytes;

    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)
            ( RenderCore::IMMUTABLE_MAP_WRITE | RenderCore::IMMUTABLE_MAP_PERSISTENT | RenderCore::IMMUTABLE_MAP_COHERENT );
    bufferCI.bImmutableStorage = true;

    GDevice->CreateBuffer( bufferCI, nullptr, &_Buffer->pBuffer );

    void * pMappedMemory = _Buffer->pBuffer->Map( RenderCore::MAP_TRANSFER_WRITE,
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

void ARenderBackend::WriteBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) {
    RenderCore::IBuffer * buffer = GPUBufferHandle( _Buffer );

    buffer->WriteRange( _ByteOffset, _SizeInBytes, _SysMem );
}

void ARenderBackend::ReadBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) {
    RenderCore::IBuffer * buffer = GPUBufferHandle( _Buffer );

    buffer->ReadRange( _ByteOffset, _SizeInBytes, _SysMem );
}

void ARenderBackend::OrphanBuffer( ABufferGPU * _Buffer ) {
    RenderCore::IBuffer * buffer = GPUBufferHandle( _Buffer );

    buffer->Orphan();
}

AMaterialGPU * ARenderBackend::CreateMaterial( IGPUResourceOwner * _Owner ) {
    return CreateResource< AMaterialGPU >( _Owner );
}

void ARenderBackend::DestroyMaterial( AMaterialGPU * _Material ) {
    using namespace RenderCore;

    DestroyResource( _Material );
}

void ARenderBackend::InitializeMaterial( AMaterialGPU * _Material, SMaterialDef const * _Def ) {
    using namespace RenderCore;

    _Material->MaterialType = _Def->Type;
    _Material->LightmapSlot = _Def->LightmapSlot;
    _Material->bDepthPassTextureFetch     = _Def->bDepthPassTextureFetch;
    _Material->bLightPassTextureFetch     = _Def->bLightPassTextureFetch;
    _Material->bWireframePassTextureFetch = _Def->bWireframePassTextureFetch;
    _Material->bNormalsPassTextureFetch   = _Def->bNormalsPassTextureFetch;
    _Material->bShadowMapPassTextureFetch = _Def->bShadowMapPassTextureFetch;
    _Material->NumSamplers      = _Def->NumSamplers;

    POLYGON_CULL cullMode = _Def->bTwoSided ? POLYGON_CULL_DISABLED : POLYGON_CULL_FRONT;

    AString code = LoadShader( "material.glsl", _Def->Shaders );

    //{
    //    AFileStream fs;
    //    fs.OpenWrite( "test.txt" );
    //    fs.WriteBuffer( code.CStr(), code.Length() );
    //}

    bool bTessellation = _Def->TessellationMethod != TESSELLATION_DISABLED;
    bool bTessellationShadowMap = bTessellation && _Def->bDisplacementAffectShadow;

    switch ( _Material->MaterialType ) {
    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:
    case MATERIAL_TYPE_UNLIT: {
        for ( int i = 0 ; i < 2 ; i++ ) {
            bool bSkinned = !!i;
            CreateDepthPassPipeline( &_Material->DepthPass[i], code.CStr(), _Def->bAlphaMasking, cullMode, bSkinned, bTessellation );
            CreateLightPassPipeline( &_Material->LightPass[i], code.CStr(), cullMode, bSkinned, _Def->bDepthTest_EXPERIMENTAL, _Def->bTranslucent, _Def->Blending, bTessellation );
            CreateWireframePassPipeline( &_Material->WireframePass[i], code.CStr(), cullMode, bSkinned, bTessellation );
            CreateNormalsPassPipeline( &_Material->NormalsPass[i], code.CStr(), bSkinned );
            CreateShadowMapPassPipeline( &_Material->ShadowPass[i], code.CStr(), _Def->bShadowMapMasking, _Def->bTwoSided, bSkinned, bTessellationShadowMap );
            CreateFeedbackPassPipeline( &_Material->FeedbackPass[i], code.CStr(), cullMode, bSkinned );
        }

        if ( _Material->MaterialType != MATERIAL_TYPE_UNLIT ) {
            CreateLightPassLightmapPipeline( &_Material->LightPassLightmap, code.CStr(), cullMode, _Def->bDepthTest_EXPERIMENTAL, _Def->bTranslucent, _Def->Blending, bTessellation );
            CreateLightPassVertexLightPipeline( &_Material->LightPassVertexLight, code.CStr(), cullMode, _Def->bDepthTest_EXPERIMENTAL, _Def->bTranslucent, _Def->Blending, bTessellation );
        }
        break;
    }
    case MATERIAL_TYPE_HUD:
    case MATERIAL_TYPE_POSTPROCESS: {
        CreateHUDPipeline( &_Material->HUDPipeline, code.CStr() );
        break;
    }
    default:
        AN_ASSERT( 0 );
    }

    SSamplerCreateInfo samplerCI;

    samplerCI.Filter = FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressV = SAMPLER_ADDRESS_WRAP;
    samplerCI.AddressW = SAMPLER_ADDRESS_WRAP;
    samplerCI.MipLODBias = 0;
    samplerCI.MaxAnisotropy = 0;
    samplerCI.ComparisonFunc = CMPFUNC_LEQUAL;
    samplerCI.bCompareRefToTexture = false;
    samplerCI.BorderColor[0] = 0;
    samplerCI.BorderColor[1] = 0;
    samplerCI.BorderColor[2] = 0;
    samplerCI.BorderColor[3] = 0;
    samplerCI.MinLOD = -1000;
    samplerCI.MaxLOD = 1000;

    Core::ZeroMem( _Material->pSampler, sizeof( _Material->pSampler ) );

    static constexpr SAMPLER_FILTER SamplerFilterLUT[] = {
        FILTER_LINEAR,
        FILTER_NEAREST,
        FILTER_MIPMAP_NEAREST,
        FILTER_MIPMAP_BILINEAR,
        FILTER_MIPMAP_NLINEAR,
        FILTER_MIPMAP_TRILINEAR
    };

    static constexpr SAMPLER_ADDRESS_MODE SamplerAddressLUT[] = {
        SAMPLER_ADDRESS_WRAP,
        SAMPLER_ADDRESS_MIRROR,
        SAMPLER_ADDRESS_CLAMP,
        SAMPLER_ADDRESS_BORDER,
        SAMPLER_ADDRESS_MIRROR_ONCE
    };

    for ( int i = 0 ; i < _Def->NumSamplers ; i++ ) {
        STextureSampler const * desc = &_Def->Samplers[i];

        samplerCI.Filter = SamplerFilterLUT[ desc->Filter ];
        samplerCI.AddressU = SamplerAddressLUT[ desc->AddressU ];
        samplerCI.AddressV = SamplerAddressLUT[ desc->AddressV ];
        samplerCI.AddressW = SamplerAddressLUT[ desc->AddressW ];
        samplerCI.MipLODBias = desc->MipLODBias;
        samplerCI.MaxAnisotropy = desc->Anisotropy;
        samplerCI.MinLOD = desc->MinLod;
        samplerCI.MaxLOD = desc->MaxLod;
        samplerCI.bCubemapSeamless = true; // FIXME: use desc->bCubemapSeamless ?

        GDevice->GetOrCreateSampler( samplerCI, &_Material->pSampler[i] );
    }
}

void ARenderBackend::RenderFrame( SRenderFrame * pFrameData ) {
    bool bRebuildMaterials = false;

    if ( RVMotionBlur.IsModified() ) {
        RVMotionBlur.UnmarkModified();
        bRebuildMaterials = true;
    }

    if ( RVSSLR.IsModified() ) {
        RVSSLR.UnmarkModified();
        bRebuildMaterials = true;
    }

    if ( RVSSAO.IsModified() ) {
        RVSSAO.UnmarkModified();
        bRebuildMaterials = true;
    }

    if ( bRebuildMaterials ) {
        GLogger.Printf( "Need to rebuild all materials\n" );
    }

    GFrameResources.FrameConstantBuffer->Begin();

    GFrameData = pFrameData;

    rcmd->SetSwapChainResolution( GFrameData->CanvasWidth, GFrameData->CanvasHeight );

    VTWorkflow->FeedbackAnalyzer.Begin();

    // TODO: Bind virtual textures in one place
    VTWorkflow->FeedbackAnalyzer.BindTexture( 0, TestVT );

    CanvasRenderer->Render();

    VTWorkflow->FeedbackAnalyzer.End();

    // FIXME: Move it at beggining of the frame to give more time for stream thread?
    VTWorkflow->PhysCache.Update();

    SetGPUEvent();

    RVRenderSnapshot = false;

    GFrameResources.FrameConstantBuffer->End();
}

void ARenderBackend::RenderView( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture ) {
    GRenderView = pRenderView;
    GRenderViewArea.X = 0;
    GRenderViewArea.Y = 0;
    GRenderViewArea.Width = GRenderView->Width;
    GRenderViewArea.Height = GRenderView->Height;

    bool bVirtualTexturing = VTWorkflow->FeedbackAnalyzer.HasBindings();

    if ( bVirtualTexturing ) {
        GRenderView->VTFeedback->Begin( GRenderView->Width, GRenderView->Height );
    }

    GFrameResources.UploadUniforms();

    if ( RVRebuildGraph ) {
        FrameRenderer->Render( *FrameGraph, bVirtualTexturing ? VTWorkflow.GetObject() : nullptr, CapturedResources );
    }
    
    FrameGraph->Execute( rcmd );

    *ppViewTexture = CapturedResources.FinalTexture;

    if ( bVirtualTexturing ) {
        int FeedbackSize;
        const void * FeedbackData;
        GRenderView->VTFeedback->End( &FeedbackSize, &FeedbackData );
    
        VTWorkflow->FeedbackAnalyzer.AddFeedbackData( FeedbackSize, FeedbackData );
    }
}

void OpenGL45RenderView( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture ) {
    GOpenGL45RenderBackend.RenderView( pRenderView, ppViewTexture );
}

void ARenderBackend::SwapBuffers() {
    GDevice->SwapBuffers( WindowHandle );
}
