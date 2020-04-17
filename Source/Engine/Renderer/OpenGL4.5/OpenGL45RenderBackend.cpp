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

#include "OpenGL45RenderBackend.h"
#include "OpenGL45GPUSync.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45ShadowMapRT.h"
#include "OpenGL45Material.h"
#include "OpenGL45CanvasPassRenderer.h"
#include "OpenGL45ShadowMapPassRenderer.h"
#include "OpenGL45DepthPassRenderer.h"
#include "OpenGL45ColorPassRenderer.h"
#include "OpenGL45WireframePassRenderer.h"
#include "OpenGL45DebugDrawPassRenderer.h"
#include "OpenGL45BrightPassRenderer.h"
#include "OpenGL45PostprocessPassRenderer.h"
#include "OpenGL45FxaaPassRenderer.h"

#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/Runtime.h>
#include <Core/Public/WindowsDefs.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/Logger.h>
#include <Core/Public/Image.h>

#include "GHI/GL/glew.h"

#ifdef AN_OS_WIN32
#include "GHI/GL/wglew.h"
#endif

#ifdef AN_OS_LINUX
#include <GL/glxew.h>
#endif

#include <SDL.h>

ARuntimeVariable RVSwapInterval( _CTS("SwapInterval"), _CTS("0"), 0, _CTS("1 - enable vsync, 0 - disable vsync, -1 - tearing") );
ARuntimeVariable RVRenderSnapshot( _CTS("RenderSnapshot"), _CTS( "0" ), VAR_CHEAT );

extern thread_local char LogBuffer[16384]; // Use existing log buffer

namespace GHI {

static State * CurrentState;
void SetCurrentState( State * _State ) {
    CurrentState = _State;
}

State * GetCurrentState() {
    return CurrentState;
}

ghi_state_t * ghi_get_current_state() { // experemental
    return nullptr;
}

void LogPrintf( const char * _Format, ... ) {
    va_list VaList;
    va_start( VaList, _Format );
    Core::VSprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    GLogger.Print( LogBuffer );
}

}

namespace OpenGL45 {

ARenderBackend GOpenGL45RenderBackend;

static GHI::TEXTURE_PIXEL_FORMAT PixelFormatTable[256];
static GHI::INTERNAL_PIXEL_FORMAT InternalPixelFormatTable[256];

static SDL_Window * WindowHandle;
static SDL_GLContext WindowCtx;
static bool bSwapControl = false;
static bool bSwapControlTear = false;

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

static void DebugMessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * userParam ) {
    const char * sourceStr;
    switch ( source ) {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "WINDOW SYSTEM";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "SHADER COMPILER";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "THIRD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "APPLICATION";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "OTHER";
        break;
    default:
        sourceStr = "UNKNOWN";
        break;
    }

    const char * typeStr;
    switch ( type ) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEFINED BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "MISC";
        break;
    case GL_DEBUG_TYPE_MARKER:
        typeStr = "MARKER";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        typeStr = "PUSH GROUP";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        typeStr = "POP GROUP";
        break;
    default:
        typeStr = "UNKNOWN";
        break;
    }

    const char * severityStr;
    switch ( severity ) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityStr = "NOTIFICATION";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;
    default:
        severityStr = "UNKNOWN";
        break;
    }

    GLogger.Printf( "-----------------------------------\n"
                    "%s %s\n"
                    "%s: %s (Id %d)\n"
                    "-----------------------------------\n",
                    sourceStr, typeStr, severityStr, message, id );
}

void ARenderBackend::Initialize( SVideoMode const & _VideoMode ) {
    using namespace GHI;

    GLogger.Printf( "Initializing OpenGL backend...\n" );

    SDL_InitSubSystem( SDL_INIT_VIDEO );

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
    //    SDL_GL_SetAttribute( SDL_GL_CONTEXT_RELEASE_BEHAVIOR,  );
    //    SDL_GL_SetAttribute( SDL_GL_CONTEXT_RESET_NOTIFICATION,  );
    //    SDL_GL_SetAttribute( SDL_GL_CONTEXT_NO_ERROR,  );
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
//    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,  );
//    SDL_GL_SetAttribute( SDL_GL_RETAINED_BACKING,  );

//    glfwWindowHint( GLFW_AUX_BUFFERS, 0 );

//    glfwWindowHint( GLFW_REFRESH_RATE, _Info.RefreshRate );

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;
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

    WindowHandle = SDL_CreateWindow( _VideoMode.Title, x, y, _VideoMode.Width, _VideoMode.Height, flags );
    if ( !WindowHandle ) {
        CriticalError( "Failed to create main window\n" );
    }

    WindowCtx = SDL_GL_CreateContext( WindowHandle );
    if ( !WindowCtx ) {
        CriticalError( "Failed to initialize OpenGL context\n" );
    }

    SDL_GL_MakeCurrent( WindowHandle, WindowCtx );
    SDL_GL_SetSwapInterval( 0 );

    glewExperimental = true;
    GLenum result = glewInit();
    if ( result != GLEW_OK ) {
        CriticalError( "Failed to load OpenGL functions\n" );
    }

    // One important thing left to mention is that GLEW has a long-existing bug
    // where calling glewInit() always sets the GL_INVALID_ENUM error flag and
    // thus the first glGetError will always return an error code which can throw
    // you completely off guard.
    // To fix this it's advised to simply call glGetError after glewInit to clear the flag.
    (void)glGetError();

#ifdef AN_DEBUG
    GLint contextFlags;
    glGetIntegerv( GL_CONTEXT_FLAGS, &contextFlags );
    if ( contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT ) {
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageCallback( DebugMessageCallback, NULL );
    }
#endif

    const char * vendorString = (const char *)glGetString( GL_VENDOR );
    vendorString = vendorString ? vendorString : AString::NullCString();

    const char * adapterString = (const char *)glGetString( GL_RENDERER );
    adapterString = adapterString ? adapterString : "Unknown";

    const char * driverVersion = (const char *)glGetString( GL_VERSION );
    driverVersion = driverVersion ? driverVersion : "Unknown";

    GLogger.Printf( "Graphics vendor: %s\n", vendorString );
    GLogger.Printf( "Graphics adapter: %s\n", adapterString );
    GLogger.Printf( "Driver version: %s\n", driverVersion );

#if 0
    SMemoryInfo gpuMemoryInfo = GetGPUMemoryInfo();
    if ( gpuMemoryInfo.TotalAvailableMegabytes > 0 && gpuMemoryInfo.CurrentAvailableMegabytes > 0 ) {
        GLogger.Printf( "Total available GPU memory: %d Megs\n", gpuMemoryInfo.TotalAvailableMegabytes );
        GLogger.Printf( "Current available GPU memory: %d Megs\n", gpuMemoryInfo.CurrentAvailableMegabytes );
    }
#endif

#if defined( AN_OS_WIN32 )
    bSwapControl = !!WGLEW_EXT_swap_control;
    bSwapControlTear = !!WGLEW_EXT_swap_control_tear;
#elif defined( AN_OS_LINUX )
    bSwapControl = !!GLXEW_EXT_swap_control || !!GLXEW_MESA_swap_control || !!GLXEW_SGI_swap_control;
    bSwapControlTear = !!GLXEW_EXT_swap_control_tear;
#else
#error "Swap control tear checking not implemented on current platform"
#endif

    AllocatorCallback allocator;

    allocator.Allocate = GHIImport_Allocate;
    allocator.Deallocate = GHIImport_Deallocate;

    GDevice.Initialize( &allocator, GHIImport_Hash );

    UniformBufferOffsetAlignment = GDevice.GetUniformBufferOffsetAlignment();

    StateCreateInfo stateCreateInfo = {};
    stateCreateInfo.ClipControl = CLIP_CONTROL_DIRECTX;
    stateCreateInfo.ViewportOrigin = VIEWPORT_ORIGIN_TOP_LEFT;

    GState.Initialize( &GDevice, stateCreateInfo );

    SetCurrentState( &GState );

    glClearColor( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT );
    SDL_GL_SwapWindow( WindowHandle );

    Core::ZeroMem( PixelFormatTable, sizeof( PixelFormatTable ) );
    Core::ZeroMem( InternalPixelFormatTable, sizeof( InternalPixelFormatTable ) );

    PixelFormatTable[TEXTURE_PF_R8_SIGNED] = PIXEL_FORMAT_BYTE_R;
    PixelFormatTable[TEXTURE_PF_RG8_SIGNED] = PIXEL_FORMAT_BYTE_RG;
    PixelFormatTable[TEXTURE_PF_BGR8_SIGNED] = PIXEL_FORMAT_BYTE_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA8_SIGNED] = PIXEL_FORMAT_BYTE_BGRA;

    PixelFormatTable[TEXTURE_PF_R8] = PIXEL_FORMAT_UBYTE_R;
    PixelFormatTable[TEXTURE_PF_RG8] = PIXEL_FORMAT_UBYTE_RG;
    PixelFormatTable[TEXTURE_PF_BGR8] = PIXEL_FORMAT_UBYTE_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA8] = PIXEL_FORMAT_UBYTE_BGRA;

    PixelFormatTable[TEXTURE_PF_BGR8_SRGB] = PIXEL_FORMAT_UBYTE_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA8_SRGB] = PIXEL_FORMAT_UBYTE_BGRA;

    PixelFormatTable[TEXTURE_PF_R16_SIGNED] = PIXEL_FORMAT_SHORT_R;
    PixelFormatTable[TEXTURE_PF_RG16_SIGNED] = PIXEL_FORMAT_SHORT_RG;
    PixelFormatTable[TEXTURE_PF_BGR16_SIGNED] = PIXEL_FORMAT_SHORT_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA16_SIGNED] = PIXEL_FORMAT_SHORT_BGRA;

    PixelFormatTable[TEXTURE_PF_R16] = PIXEL_FORMAT_USHORT_R;
    PixelFormatTable[TEXTURE_PF_RG16] = PIXEL_FORMAT_USHORT_RG;
    PixelFormatTable[TEXTURE_PF_BGR16] = PIXEL_FORMAT_USHORT_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA16] = PIXEL_FORMAT_USHORT_BGRA;

    PixelFormatTable[TEXTURE_PF_R32_SIGNED] = PIXEL_FORMAT_INT_R;
    PixelFormatTable[TEXTURE_PF_RG32_SIGNED] = PIXEL_FORMAT_INT_RG;
    PixelFormatTable[TEXTURE_PF_BGR32_SIGNED] = PIXEL_FORMAT_INT_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA32_SIGNED] = PIXEL_FORMAT_INT_BGRA;

    PixelFormatTable[TEXTURE_PF_R32] = PIXEL_FORMAT_UINT_R;
    PixelFormatTable[TEXTURE_PF_RG32] = PIXEL_FORMAT_UINT_RG;
    PixelFormatTable[TEXTURE_PF_BGR32] = PIXEL_FORMAT_UINT_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA32] = PIXEL_FORMAT_UINT_BGRA;

    PixelFormatTable[TEXTURE_PF_R16F] = PIXEL_FORMAT_HALF_R;
    PixelFormatTable[TEXTURE_PF_RG16F] = PIXEL_FORMAT_HALF_RG;
    PixelFormatTable[TEXTURE_PF_BGR16F] = PIXEL_FORMAT_HALF_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA16F] = PIXEL_FORMAT_HALF_BGRA;

    PixelFormatTable[TEXTURE_PF_R32F] = PIXEL_FORMAT_FLOAT_R;
    PixelFormatTable[TEXTURE_PF_RG32F] = PIXEL_FORMAT_FLOAT_RG;
    PixelFormatTable[TEXTURE_PF_BGR32F] = PIXEL_FORMAT_FLOAT_BGR;
    PixelFormatTable[TEXTURE_PF_BGRA32F] = PIXEL_FORMAT_FLOAT_BGRA;

    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGB_DXT1] = PIXEL_FORMAT_COMPRESSED_RGB_DXT1;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_DXT1] = PIXEL_FORMAT_COMPRESSED_RGBA_DXT1;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_DXT3] = PIXEL_FORMAT_COMPRESSED_RGBA_DXT3;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_DXT5] = PIXEL_FORMAT_COMPRESSED_RGBA_DXT5;

    PixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_DXT1] = PIXEL_FORMAT_COMPRESSED_SRGB_DXT1;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT1] = PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_DXT1;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT3] = PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_DXT3;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT5] = PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_DXT5;

    PixelFormatTable[TEXTURE_PF_COMPRESSED_RED_RGTC1] = PIXEL_FORMAT_COMPRESSED_RED_RGTC1;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_RG_RGTC2] = PIXEL_FORMAT_COMPRESSED_RG_RGTC2;

    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_BPTC_UNORM] = PIXEL_FORMAT_COMPRESSED_RGBA_BPTC_UNORM;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM] = PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT] = PIXEL_FORMAT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
    PixelFormatTable[TEXTURE_PF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT] = PIXEL_FORMAT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;

    // FIXME: ?
    //    INTERNAL_PIXEL_FORMAT_R8_SNORM,       // RED   s8
    //    INTERNAL_PIXEL_FORMAT_RG8_SNORM,      // RG    s8  s8
    //    INTERNAL_PIXEL_FORMAT_RGB8_SNORM,     // RGB   s8  s8  s8
    //    INTERNAL_PIXEL_FORMAT_RGBA8_SNORM,    // RGBA  s8  s8  s8  s8
    InternalPixelFormatTable[TEXTURE_PF_R8_SIGNED] = INTERNAL_PIXEL_FORMAT_R8I;
    InternalPixelFormatTable[TEXTURE_PF_RG8_SIGNED] = INTERNAL_PIXEL_FORMAT_RG8I;
    InternalPixelFormatTable[TEXTURE_PF_BGR8_SIGNED] = INTERNAL_PIXEL_FORMAT_RGB8I;
    InternalPixelFormatTable[TEXTURE_PF_BGRA8_SIGNED] = INTERNAL_PIXEL_FORMAT_RGBA8I;

    //    INTERNAL_PIXEL_FORMAT_R8UI,           // RED   ui8
    //    INTERNAL_PIXEL_FORMAT_RG8UI,          // RG    ui8   ui8
    //    INTERNAL_PIXEL_FORMAT_RGB8UI,         // RGB   ui8  ui8  ui8
    //    INTERNAL_PIXEL_FORMAT_RGBA8UI,        // RGBA  ui8  ui8  ui8  ui8
    InternalPixelFormatTable[TEXTURE_PF_R8] = INTERNAL_PIXEL_FORMAT_R8;
    InternalPixelFormatTable[TEXTURE_PF_RG8] = INTERNAL_PIXEL_FORMAT_RG8;
    InternalPixelFormatTable[TEXTURE_PF_BGR8] = INTERNAL_PIXEL_FORMAT_RGB8;
    InternalPixelFormatTable[TEXTURE_PF_BGRA8] = INTERNAL_PIXEL_FORMAT_RGBA8;

    InternalPixelFormatTable[TEXTURE_PF_BGR8_SRGB] = INTERNAL_PIXEL_FORMAT_SRGB8;
    InternalPixelFormatTable[TEXTURE_PF_BGRA8_SRGB] = INTERNAL_PIXEL_FORMAT_SRGB8_ALPHA8;

    // FIXME: ?
    //    INTERNAL_PIXEL_FORMAT_R16_SNORM,      // RED   s16
    //    INTERNAL_PIXEL_FORMAT_RG16_SNORM,     // RG    s16 s16
    //    INTERNAL_PIXEL_FORMAT_RGB16_SNORM,    // RGB   s16  s16  s16
    //    INTERNAL_PIXEL_FORMAT_RGBA16_SNORM,   // RGBA  s16  s16  s16  s16
    InternalPixelFormatTable[TEXTURE_PF_R16_SIGNED] = INTERNAL_PIXEL_FORMAT_R16I;
    InternalPixelFormatTable[TEXTURE_PF_RG16_SIGNED] = INTERNAL_PIXEL_FORMAT_RG16I;
    InternalPixelFormatTable[TEXTURE_PF_BGR16_SIGNED] = INTERNAL_PIXEL_FORMAT_RGB16I;
    InternalPixelFormatTable[TEXTURE_PF_BGRA16_SIGNED] = INTERNAL_PIXEL_FORMAT_RGBA16I;

    // FIXME: ?
    //    INTERNAL_PIXEL_FORMAT_R16,            // RED   16
    //    INTERNAL_PIXEL_FORMAT_RG16,           // RG    16  16
    //    INTERNAL_PIXEL_FORMAT_RGB16,          // RGB   16  16  16
    //    INTERNAL_PIXEL_FORMAT_RGBA16,         // RGBA  16  16  16  16
    InternalPixelFormatTable[TEXTURE_PF_R16] = INTERNAL_PIXEL_FORMAT_R16UI;
    InternalPixelFormatTable[TEXTURE_PF_RG16] = INTERNAL_PIXEL_FORMAT_RG16UI;
    InternalPixelFormatTable[TEXTURE_PF_BGR16] = INTERNAL_PIXEL_FORMAT_RGB16UI;
    InternalPixelFormatTable[TEXTURE_PF_BGRA16] = INTERNAL_PIXEL_FORMAT_RGBA16UI;

    InternalPixelFormatTable[TEXTURE_PF_R32_SIGNED] = INTERNAL_PIXEL_FORMAT_R32I;
    InternalPixelFormatTable[TEXTURE_PF_RG32_SIGNED] = INTERNAL_PIXEL_FORMAT_RG32I;
    InternalPixelFormatTable[TEXTURE_PF_BGR32_SIGNED] = INTERNAL_PIXEL_FORMAT_RGB32I;
    InternalPixelFormatTable[TEXTURE_PF_BGRA32_SIGNED] = INTERNAL_PIXEL_FORMAT_RGBA32I;

    InternalPixelFormatTable[TEXTURE_PF_R32] = INTERNAL_PIXEL_FORMAT_R32UI;
    InternalPixelFormatTable[TEXTURE_PF_RG32] = INTERNAL_PIXEL_FORMAT_RG32UI;
    InternalPixelFormatTable[TEXTURE_PF_BGR32] = INTERNAL_PIXEL_FORMAT_RGB32UI;
    InternalPixelFormatTable[TEXTURE_PF_BGRA32] = INTERNAL_PIXEL_FORMAT_RGBA32UI;

    InternalPixelFormatTable[TEXTURE_PF_R16F] = INTERNAL_PIXEL_FORMAT_R16F;
    InternalPixelFormatTable[TEXTURE_PF_RG16F] = INTERNAL_PIXEL_FORMAT_RG16F;
    InternalPixelFormatTable[TEXTURE_PF_BGR16F] = INTERNAL_PIXEL_FORMAT_RGB16F;
    InternalPixelFormatTable[TEXTURE_PF_BGRA16F] = INTERNAL_PIXEL_FORMAT_RGBA16F;

    InternalPixelFormatTable[TEXTURE_PF_R32F] = INTERNAL_PIXEL_FORMAT_R32F;
    InternalPixelFormatTable[TEXTURE_PF_RG32F] = INTERNAL_PIXEL_FORMAT_RG32F;
    InternalPixelFormatTable[TEXTURE_PF_BGR32F] = INTERNAL_PIXEL_FORMAT_RGB32F;
    InternalPixelFormatTable[TEXTURE_PF_BGRA32F] = INTERNAL_PIXEL_FORMAT_RGBA32F;

    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGB_DXT1] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB_S3TC_DXT1;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_DXT1] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT1;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_DXT3] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT3;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_DXT5] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT5;

    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_DXT1] = INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_S3TC_DXT1;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT1] = INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT1;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT3] = INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT3;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_DXT5] = INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_S3TC_DXT5;

    // FIXME: ?
    // INTERNAL_PIXEL_FORMAT_COMPRESSED_SIGNED_RED_RGTC1
    // INTERNAL_PIXEL_FORMAT_COMPRESSED_SIGNED_RG_RGTC2
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RED_RGTC1] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RED_RGTC1;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RG_RGTC2] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RG_RGTC2;

    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGBA_BPTC_UNORM] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGBA_BPTC_UNORM;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM] = INTERNAL_PIXEL_FORMAT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
    InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT] = INTERNAL_PIXEL_FORMAT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;

    // Unused formats:
    //    INTERNAL_PIXEL_FORMAT_R3_G3_B2,       // RGB   3  3  2
    //    INTERNAL_PIXEL_FORMAT_RGB4,           // RGB   4  4  4
    //    INTERNAL_PIXEL_FORMAT_RGB5,           // RGB   5  5  5
    //    INTERNAL_PIXEL_FORMAT_RGB10,          // RGB   10  10  10
    //    INTERNAL_PIXEL_FORMAT_RGB12,          // RGB   12  12  12
    //    INTERNAL_PIXEL_FORMAT_RGBA2,          // RGB   2  2  2  2
    //    INTERNAL_PIXEL_FORMAT_RGBA4,          // RGB   4  4  4  4
    //    INTERNAL_PIXEL_FORMAT_RGB5_A1,        // RGBA  5  5  5  1
    //    INTERNAL_PIXEL_FORMAT_RGB10_A2,       // RGBA  10  10  10  2
    //    INTERNAL_PIXEL_FORMAT_RGB10_A2UI,     // RGBA  ui10  ui10  ui10  ui2
    //    INTERNAL_PIXEL_FORMAT_RGBA12,         // RGBA  12  12  12  12
    //    INTERNAL_PIXEL_FORMAT_R11F_G11F_B10F, // RGB   f11  f11  f10
    //    INTERNAL_PIXEL_FORMAT_RGB9_E5,        // RGB   9  9  9     5


    GRenderTarget.Initialize();
    GShadowMapRT.Initialize();
    GShadowMapPassRenderer.Initialize();
    GDepthPassRenderer.Initialize();
    GColorPassRenderer.Initialize();
    GWireframePassRenderer.Initialize();
    GDebugDrawPassRenderer.Initialize();
    GBrightPassRenderer.Initialize();
    GPostprocessPassRenderer.Initialize();
    GFxaaPassRenderer.Initialize();
    GCanvasPassRenderer.Initialize();
    GFrameResources.Initialize();

    // TODO: dither.png encode to Base85 and move to source code?
    AImage image;
    if ( !image.Load( "dither.png", nullptr, IMAGE_PF_R ) ) {
        CriticalError( "Couldn't load dither.png\n" );
    }
    GHI::TextureStorageCreateInfo texStorageCI = {};
    texStorageCI.Type = GHI::TEXTURE_2D;
    texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_R8;
    texStorageCI.Resolution.Tex2D.Width = image.Width;
    texStorageCI.Resolution.Tex2D.Height = image.Height;
    texStorageCI.NumLods = 1;
    DitherTexture.InitializeStorage( texStorageCI );
    DitherTexture.Write( 0, GHI::PIXEL_FORMAT_UBYTE_R, image.Width * image.Height * 1, 1, image.pRawData );
}

void ARenderBackend::Deinitialize() {
    GLogger.Printf( "Deinitializing OpenGL backend...\n" );

    DitherTexture.Deinitialize();
    GRenderTarget.Deinitialize();
    GShadowMapRT.Deinitialize();
    GShadowMapPassRenderer.Deinitialize();
    GDepthPassRenderer.Deinitialize();
    GColorPassRenderer.Deinitialize();
    GWireframePassRenderer.Deinitialize();
    GDebugDrawPassRenderer.Deinitialize();
    GBrightPassRenderer.Deinitialize();
    GPostprocessPassRenderer.Deinitialize();
    GFxaaPassRenderer.Deinitialize();
    GCanvasPassRenderer.Deinitialize();
    GFrameResources.Deinitialize();
    GOpenGL45GPUSync.Release();

    GState.Deinitialize();
    GDevice.Deinitialize();

    SDL_GL_DeleteContext( WindowCtx );
    SDL_DestroyWindow( WindowHandle );
    SDL_QuitSubSystem( SDL_INIT_VIDEO );
    WindowHandle = nullptr;

    GLogger.Printf( "TotalAllocatedGHI: %d\n", TotalAllocatedGHI );
}

void * ARenderBackend::GetMainWindow() {
    return WindowHandle;
}

void ARenderBackend::WaitGPU() {
    GOpenGL45GPUSync.Wait();
}

void ARenderBackend::SetGPUEvent() {
    GOpenGL45GPUSync.SetEvent();
}

void * ARenderBackend::FenceSync() {
    return Cmd.FenceSync();
}

void ARenderBackend::RemoveSync( void * _Sync ) {
    if ( _Sync ) {
        Cmd.RemoveSync( (GHI::SyncObject)_Sync );
    }
}

void ARenderBackend::WaitSync( void * _Sync ) {
    const uint64_t timeOutNanoseconds = 1;
    if ( _Sync ) {
        GHI::CLIENT_WAIT_STATUS status;
        do {
            status = Cmd.ClientWait( (GHI::SyncObject)_Sync, timeOutNanoseconds );
        } while ( status != GHI::CLIENT_WAIT_ALREADY_SIGNALED && status != GHI::CLIENT_WAIT_CONDITION_SATISFIED );
    }
}

void ARenderBackend::ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) {
    GHI::Rect2D rect;
    rect.X = _X;
    rect.Y = _Y;
    rect.Width = _Width;
    rect.Height = _Height;

    GHI::Framebuffer * framebuffer = GState.GetDefaultFramebuffer();

    framebuffer->Read( GHI::FB_BACK_DEFAULT, rect, GHI::FB_RGBA_CHANNEL, GHI::FB_UBYTE, GHI::COLOR_CLAMP_ON, _SizeInBytes, _Alignment, _SysMem );
}

ATextureGPU * ARenderBackend::CreateTexture( IGPUResourceOwner * _Owner ) {
    ATextureGPU * texture = CreateResource< ATextureGPU >( _Owner );
    texture->pHandleGPU = GZoneMemory.ClearedAlloc( sizeof( GHI::Texture ) );
    new (texture->pHandleGPU) GHI::Texture;
    return texture;
}

void ARenderBackend::DestroyTexture( ATextureGPU * _Texture ) {
    using namespace GHI;
    GHI::Texture * texture = GPUTextureHandle( _Texture );
    texture->~Texture();
    GZoneMemory.Free( _Texture->pHandleGPU );
    DestroyResource( _Texture );
}

static void SetTextureSwizzle( ETexturePixelFormat _PixelFormat, GHI::TextureSwizzle & _Swizzle ) {
    switch ( STexturePixelFormat( _PixelFormat ).NumComponents() ) {
    case 1:
        // Apply texture swizzle for one channel textures
        _Swizzle.R = GHI::TEXTURE_SWIZZLE_R;
        _Swizzle.G = GHI::TEXTURE_SWIZZLE_R;
        _Swizzle.B = GHI::TEXTURE_SWIZZLE_R;
        _Swizzle.A = GHI::TEXTURE_SWIZZLE_R;
        break;
#if 0
    case 2:
        // Apply texture swizzle for two channel textures
        _Swizzle.R = GHI::TEXTURE_SWIZZLE_R;
        _Swizzle.G = GHI::TEXTURE_SWIZZLE_G;
        _Swizzle.B = GHI::TEXTURE_SWIZZLE_R;
        _Swizzle.A = GHI::TEXTURE_SWIZZLE_G;
        break;
    case 3:
        // Apply texture swizzle for three channel textures
        _Swizzle.R = GHI::TEXTURE_SWIZZLE_R;
        _Swizzle.G = GHI::TEXTURE_SWIZZLE_G;
        _Swizzle.B = GHI::TEXTURE_SWIZZLE_B;
        _Swizzle.A = GHI::TEXTURE_SWIZZLE_ONE;
        break;
#endif
    }
}

void ARenderBackend::InitializeTexture1D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_1D;
    textureCI.Resolution.Tex1D.Width = _Width;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTexture1DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_1D_ARRAY;
    textureCI.Resolution.Tex1DArray.Width = _Width;
    textureCI.Resolution.Tex1DArray.NumLayers = _ArraySize;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTexture2D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_2D;
    textureCI.Resolution.Tex2D.Width = _Width;
    textureCI.Resolution.Tex2D.Height = _Height;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTexture2DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_2D_ARRAY;
    textureCI.Resolution.Tex2DArray.Width = _Width;
    textureCI.Resolution.Tex2DArray.Height = _Height;
    textureCI.Resolution.Tex2DArray.NumLayers = _ArraySize;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTexture3D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_3D;
    textureCI.Resolution.Tex3D.Width = _Width;
    textureCI.Resolution.Tex3D.Height = _Height;
    textureCI.Resolution.Tex3D.Depth = _Depth;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTextureCubemap( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_CUBE_MAP;
    textureCI.Resolution.TexCubemap.Width = _Width;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTextureCubemapArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Resolution.TexCubemapArray.Width = _Width;
    textureCI.Resolution.TexCubemapArray.NumLayers = _ArraySize;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::InitializeTexture2DNPOT( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_RECT;
    textureCI.Resolution.TexRect.Width = _Width;
    textureCI.Resolution.TexRect.Height = _Height;
    textureCI.InternalFormat = InternalPixelFormatTable[_PixelFormat];
    textureCI.NumLods = _NumLods;

    SetTextureSwizzle( _PixelFormat, textureCI.Swizzle );

    texture->InitializeStorage( textureCI );
}

void ARenderBackend::WriteTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) {
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureRect rect;
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
    GHI::Texture * texture = GPUTextureHandle( _Texture );

    GHI::TextureRect rect;
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
    ABufferGPU * buffer = CreateResource< ABufferGPU >( _Owner );
    buffer->pHandleGPU = GZoneMemory.ClearedAlloc( sizeof( GHI::Buffer ) );
    new (buffer->pHandleGPU) GHI::Buffer;
    return buffer;
}

void ARenderBackend::DestroyBuffer( ABufferGPU * _Buffer ) {
    using namespace GHI;
    GHI::Buffer * buffer = GPUBufferHandle( _Buffer );
    buffer->~Buffer();
    GZoneMemory.Free( _Buffer->pHandleGPU );
    DestroyResource( _Buffer );
}

void ARenderBackend::InitializeBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes ) {
    GHI::Buffer * buffer = GPUBufferHandle( _Buffer );

    GHI::BufferCreateInfo bufferCI = {};

    bufferCI.SizeInBytes = _SizeInBytes;

    const bool bDynamicStorage = false;
    if ( bDynamicStorage ) {
#if 1
        // Seems to be faster
        bufferCI.ImmutableStorageFlags = GHI::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;

        buffer->Initialize( bufferCI );
#else
        bufferCI.MutableClientAccess = GHI::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
        bufferCI.MutableUsage = GHI::MUTABLE_STORAGE_STREAM;
        bufferCI.ImmutableStorageFlags = (GHI::IMMUTABLE_STORAGE_FLAGS)0;
        bufferCI.bImmutableStorage = false;

        buffer->Initialize( bufferCI );
#endif
    } else {
#if 1
        // Mutable storage with flag MUTABLE_STORAGE_STATIC is much faster during rendering (tested on NVidia GeForce GTX 770)
        bufferCI.MutableClientAccess = GHI::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
        bufferCI.MutableUsage = GHI::MUTABLE_STORAGE_STATIC;
        bufferCI.ImmutableStorageFlags = (GHI::IMMUTABLE_STORAGE_FLAGS)0;
        bufferCI.bImmutableStorage = false;
#else
        bufferCI.ImmutableStorageFlags = GHI::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;
#endif

        buffer->Initialize( bufferCI );
    }
}

void * ARenderBackend::InitializePersistentMappedBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes ) {
    GHI::Buffer * buffer = GPUBufferHandle( _Buffer );

    GHI::BufferCreateInfo bufferCI = {};

    bufferCI.SizeInBytes = _SizeInBytes;

    bufferCI.ImmutableStorageFlags = (GHI::IMMUTABLE_STORAGE_FLAGS)
            ( GHI::IMMUTABLE_MAP_WRITE | GHI::IMMUTABLE_MAP_PERSISTENT | GHI::IMMUTABLE_MAP_COHERENT );
    bufferCI.bImmutableStorage = true;

    buffer->Initialize( bufferCI );

    void * pMappedMemory = buffer->Map( GHI::MAP_TRANSFER_WRITE,
                                        GHI::MAP_NO_INVALIDATE,//GHI::MAP_INVALIDATE_ENTIRE_BUFFER,
                                        GHI::MAP_PERSISTENT_COHERENT,
                                        false, // flush explicit
                                        false  // unsynchronized
                                      );

    if ( !pMappedMemory ) {
        CriticalError( "ARenderBackend::InitializePersistentMappedBuffer: cannot initialize persistent mapped buffer size %d\n", _SizeInBytes );
    }

    return pMappedMemory;
}

void ARenderBackend::WriteBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) {
    GHI::Buffer * buffer = GPUBufferHandle( _Buffer );

    buffer->WriteRange( _ByteOffset, _SizeInBytes, _SysMem );
}

void ARenderBackend::ReadBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) {
    GHI::Buffer * buffer = GPUBufferHandle( _Buffer );

    buffer->ReadRange( _ByteOffset, _SizeInBytes, _SysMem );
}

void ARenderBackend::OrphanBuffer( ABufferGPU * _Buffer ) {
    GHI::Buffer * buffer = GPUBufferHandle( _Buffer );

    buffer->Orphan();
}

AMaterialGPU * ARenderBackend::CreateMaterial( IGPUResourceOwner * _Owner ) {
    return CreateResource< AMaterialGPU >( _Owner );
}

void ARenderBackend::DestroyMaterial( AMaterialGPU * _Material ) {
    using namespace GHI;

    AShadeModelLit * Lit = (AShadeModelLit *)_Material->ShadeModel.Lit;
    AShadeModelUnlit* Unlit = (AShadeModelUnlit *)_Material->ShadeModel.Unlit;
    AShadeModelHUD * HUD = (AShadeModelHUD *)_Material->ShadeModel.HUD;

    if ( Lit ) {
        Lit->~AShadeModelLit();
        GZoneMemory.Free( Lit );
    }

    if ( Unlit ) {
        Unlit->~AShadeModelUnlit();
        GZoneMemory.Free( Unlit );
    }

    if ( HUD ) {
        HUD->~AShadeModelHUD();
        GZoneMemory.Free( HUD );
    }

    DestroyResource( _Material );
}

void ARenderBackend::InitializeMaterial( AMaterialGPU * _Material, SMaterialDef const * _BuildData ) {
    using namespace GHI;

    _Material->MaterialType = _BuildData->Type;
    _Material->LightmapSlot = _BuildData->LightmapSlot;
    _Material->bDepthPassTextureFetch     = _BuildData->bDepthPassTextureFetch;
    _Material->bColorPassTextureFetch     = _BuildData->bColorPassTextureFetch;
    _Material->bWireframePassTextureFetch = _BuildData->bWireframePassTextureFetch;
    _Material->bShadowMapPassTextureFetch = _BuildData->bShadowMapPassTextureFetch;
    _Material->bHasVertexDeform = _BuildData->bHasVertexDeform;
    _Material->bNoCastShadow    = _BuildData->bNoCastShadow;
    _Material->bShadowMapMasking= _BuildData->bShadowMapMasking;
    _Material->NumSamplers      = _BuildData->NumSamplers;

//    static constexpr POLYGON_CULL PolygonCullLUT[] = {
//        POLYGON_CULL_FRONT,
//        POLYGON_CULL_BACK,
//        POLYGON_CULL_DISABLED
//    };

    POLYGON_CULL cullMode = POLYGON_CULL_FRONT;//PolygonCullLUT[_BuildData->Facing];

    AShadeModelLit   * Lit   = (AShadeModelLit *)_Material->ShadeModel.Lit;
    AShadeModelUnlit * Unlit = (AShadeModelUnlit *)_Material->ShadeModel.Unlit;
    AShadeModelHUD   * HUD   = (AShadeModelHUD *)_Material->ShadeModel.HUD;

    if ( Lit ) {
        Lit->~AShadeModelLit();
        GZoneMemory.Free( Lit );
        _Material->ShadeModel.Lit = nullptr;
    }

    if ( Unlit ) {
        Unlit->~AShadeModelUnlit();
        GZoneMemory.Free( Unlit );
        _Material->ShadeModel.Unlit = nullptr;
    }

    if ( HUD ) {
        HUD->~AShadeModelHUD();
        GZoneMemory.Free( HUD );
        _Material->ShadeModel.HUD = nullptr;
    }

    AString code = LoadShader( "material.glsl", _BuildData->Shaders );

    //{
    //    AFileStream fs;
    //    fs.OpenWrite( "test.txt" );
    //    fs.WriteBuffer( code.CStr(), code.Length() + 1 );
    //}

    switch ( _Material->MaterialType ) {
    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT: {
        void * pMem = GZoneMemory.Alloc( sizeof( AShadeModelLit ) );
        Lit = new (pMem) AShadeModelLit();
        _Material->ShadeModel.Lit = Lit;

        Lit->ColorPassSimple.Create( code.CStr(), cullMode, false, _BuildData->bDepthTest_EXPEREMENTAL, _BuildData->bTranslucent, _BuildData->Blending );
        Lit->ColorPassSkinned.Create( code.CStr(), cullMode, true, _BuildData->bDepthTest_EXPEREMENTAL, _BuildData->bTranslucent, _BuildData->Blending );

        Lit->ColorPassLightmap.Create( code.CStr(), cullMode, _BuildData->bDepthTest_EXPEREMENTAL, _BuildData->bTranslucent, _BuildData->Blending );
        Lit->ColorPassVertexLight.Create( code.CStr(), cullMode, _BuildData->bDepthTest_EXPEREMENTAL, _BuildData->bTranslucent, _BuildData->Blending );

        Lit->DepthPass.Create( code.CStr(), cullMode, false );
        Lit->DepthPassSkinned.Create( code.CStr(), cullMode, true );

        Lit->WireframePass.Create( code.CStr(), cullMode, false );
        Lit->WireframePassSkinned.Create( code.CStr(), cullMode, true );

        Lit->ShadowPass.Create( code.CStr(), _BuildData->bShadowMapMasking, false );
        Lit->ShadowPassSkinned.Create( code.CStr(), _BuildData->bShadowMapMasking, true );
        break;
    }

    case MATERIAL_TYPE_UNLIT: {
        void * pMem = GZoneMemory.Alloc( sizeof( AShadeModelUnlit ) );
        Unlit = new (pMem) AShadeModelUnlit();
        _Material->ShadeModel.Unlit = Unlit;

        Unlit->ColorPassSimple.Create( code.CStr(), cullMode, false, _BuildData->bDepthTest_EXPEREMENTAL, _BuildData->bTranslucent, _BuildData->Blending );
        Unlit->ColorPassSkinned.Create( code.CStr(), cullMode, true, _BuildData->bDepthTest_EXPEREMENTAL, _BuildData->bTranslucent, _BuildData->Blending );

        Unlit->DepthPass.Create( code.CStr(), cullMode, false );
        Unlit->DepthPassSkinned.Create( code.CStr(), cullMode, true );

        Unlit->WireframePass.Create( code.CStr(), cullMode, false );
        Unlit->WireframePassSkinned.Create( code.CStr(), cullMode, true );

        Unlit->ShadowPass.Create( code.CStr(), _BuildData->bShadowMapMasking, false );
        Unlit->ShadowPassSkinned.Create( code.CStr(), _BuildData->bShadowMapMasking, true );
        break;
    }

    case MATERIAL_TYPE_HUD:
    case MATERIAL_TYPE_POSTPROCESS: {
        void * pMem = GZoneMemory.Alloc( sizeof( AShadeModelHUD ) );
        HUD = new (pMem) AShadeModelHUD();
        _Material->ShadeModel.HUD = HUD;
        HUD->ColorPassHUD.Create( code.CStr() );
        break;
    }

    default:
        AN_ASSERT( 0 );
    }

    SamplerCreateInfo samplerCI;

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

    for ( int i = 0 ; i < _BuildData->NumSamplers ; i++ ) {
        STextureSampler const * desc = &_BuildData->Samplers[i];

        samplerCI.Filter = SamplerFilterLUT[ desc->Filter ];
        samplerCI.AddressU = SamplerAddressLUT[ desc->AddressU ];
        samplerCI.AddressV = SamplerAddressLUT[ desc->AddressV ];
        samplerCI.AddressW = SamplerAddressLUT[ desc->AddressW ];
        samplerCI.MipLODBias = desc->MipLODBias;
        samplerCI.MaxAnisotropy = desc->Anisotropy;
        samplerCI.MinLOD = desc->MinLod;
        samplerCI.MaxLOD = desc->MaxLod;
        samplerCI.bCubemapSeamless = true; // FIXME: use desc->bCubemapSeamless ?

        _Material->pSampler[i] = GDevice.GetOrCreateSampler( samplerCI );
    }
}

void ARenderBackend::RenderFrame( SRenderFrame * _FrameData ) {
    GFrameData = _FrameData;

    GState.SetSwapChainResolution( GFrameData->CanvasWidth, GFrameData->CanvasHeight );

    glEnable( GL_FRAMEBUFFER_SRGB );

    GRenderTarget.ReallocSurface( GFrameData->AllocSurfaceWidth, GFrameData->AllocSurfaceHeight );

    // Calc canvas projection matrix
    const Float2 orthoMins( 0.0f, (float)GFrameData->CanvasHeight );
    const Float2 orthoMaxs( (float)GFrameData->CanvasWidth, 0.0f );
    GFrameResources.ViewUniformBufferUniformData.OrthoProjection = Float4x4::Ortho2DCC( orthoMins, orthoMaxs );
    GFrameResources.ViewUniformBuffer.WriteRange(
        GHI_STRUCT_OFS( SViewUniformBuffer, OrthoProjection ),
        sizeof( GFrameResources.ViewUniformBufferUniformData.OrthoProjection ),
        &GFrameResources.ViewUniformBufferUniformData.OrthoProjection );

    GCanvasPassRenderer.RenderInstances();

    SetGPUEvent();

    RVRenderSnapshot = false;
}

void ARenderBackend::RenderView( SRenderView * _RenderView ) {
    GRenderView = _RenderView;

    GFrameResources.UploadUniforms();

    GShadowMapPassRenderer.RenderInstances();

#ifdef DEPTH_PREPASS
    GDepthPassRenderer.RenderInstances();
#endif
    GColorPassRenderer.RenderInstances();

    GBrightPassRenderer.Render( GRenderTarget.GetFramebufferTexture() );

    GPostprocessPassRenderer.Render();

    GFxaaPassRenderer.Render();

    if ( GRenderView->bWireframe ) {
        GWireframePassRenderer.RenderInstances( &GRenderTarget.GetFxaaFramebuffer() );
    }

    if ( GRenderView->DebugDrawCommandCount > 0 ) {
        GDebugDrawPassRenderer.RenderInstances( &GRenderTarget.GetFxaaFramebuffer() );
    }
}

void OpenGL45RenderView( SRenderView * _RenderView ) {
    GOpenGL45RenderBackend.RenderView( _RenderView );
}

void ARenderBackend::SwapBuffers() {
    if ( bSwapControl ) {
        int i = Math::Clamp( RVSwapInterval.GetInteger(), -1, 1 );
        if ( i == -1 && !bSwapControlTear ) {
            // Tearing not supported
            i = 0;
        }
        SDL_GL_SetSwapInterval( i );
    }

    SDL_GL_SwapWindow( WindowHandle );
}

}
