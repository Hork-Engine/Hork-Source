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

#include <Engine/Core/Platform.h>
#include <Engine/Core/WindowsDefs.h>
#include <Engine/Core/Logger.h>
#include <Engine/RenderCore/GenericWindow.h>

#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "SwapChainGLImpl.h"
#include "BufferGLImpl.h"
#include "BufferViewGLImpl.h"
#include "TextureGLImpl.h"
#include "SparseTextureGLImpl.h"
#include "TransformFeedbackGLImpl.h"
#include "QueryGLImpl.h"
#include "PipelineGLImpl.h"
#include "ShaderModuleGLImpl.h"
#include "GenericWindowGLImpl.h"

#include "LUT.h"

#include "GL/glew.h"
#ifdef HK_OS_WIN32
#    include "GL/wglew.h"
#endif
#ifdef HK_OS_LINUX
#    include "GL/glxew.h"
#endif

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

struct SamplerInfo
{
    SamplerDesc Desc;
    unsigned int Id;
};

static const char* FeatureName[] =
    {
        "FEATURE_HALF_FLOAT_VERTEX",
        "FEATURE_HALF_FLOAT_PIXEL",
        "FEATURE_TEXTURE_ANISOTROPY",
        "FEATURE_SPARSE_TEXTURES",
        "FEATURE_BINDLESS_TEXTURE",
        "FEATURE_SWAP_CONTROL",
        "FEATURE_SWAP_CONTROL_TEAR",
        "FEATURE_GPU_MEMORY_INFO",
        "FEATURE_SPIR_V"};

static const char* DeviceCapName[] =
    {
        "DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE",
        "DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT",

        "DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT",
        "DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT",

        "DEVICE_CAPS_MAX_TEXTURE_SIZE",
        "DEVICE_CAPS_MAX_TEXTURE_LAYERS",
        "DEVICE_CAPS_MAX_SPARSE_TEXTURE_LAYERS",
        "DEVICE_CAPS_MAX_TEXTURE_ANISOTROPY",
        "DEVICE_CAPS_MAX_PATCH_VERTICES",
        "DEVICE_CAPS_MAX_VERTEX_BUFFER_SLOTS",
        "DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE",
        "DEVICE_CAPS_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET",
        "DEVICE_CAPS_MAX_CONSTANT_BUFFER_BINDINGS",
        "DEVICE_CAPS_MAX_SHADER_STORAGE_BUFFER_BINDINGS",
        "DEVICE_CAPS_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS",
        "DEVICE_CAPS_MAX_TRANSFORM_FEEDBACK_BUFFERS",
        "DEVICE_CAPS_CONSTANT_BUFFER_MAX_BLOCK_SIZE"};

static int GL_GetInteger(GLenum pname)
{
    GLint i = 0;
    glGetIntegerv(pname, &i);
    return i;
}

static int64_t GL_GetInteger64(GLenum pname)
{
    if (glGetInteger64v)
    {
        int64_t i = 0;
        glGetInteger64v(pname, &i);
        return i;
    }

    return GL_GetInteger(pname);
}

static float GL_GetFloat(GLenum pname)
{
    float f;
    glGetFloatv(pname, &f);
    return f;
}

static bool FindExtension(const char* _Extension)
{
    GLint numExtensions = 0;

    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (int i = 0; i < numExtensions; i++)
    {
        const char* ExtI = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (ExtI && Core::Strcmp(ExtI, _Extension) == 0)
        {
            return true;
        }
    }
    return false;
}

static void* Allocate(size_t _BytesCount)
{
    return Core::GetHeapAllocator<HEAP_RHI>().Alloc(_BytesCount);
}

static void Deallocate(void* _Bytes)
{
    Core::GetHeapAllocator<HEAP_RHI>().Free(_Bytes);
}

static constexpr AllocatorCallback DefaultAllocator = { Allocate, Deallocate };

DeviceGLImpl::DeviceGLImpl()
{
    BufferMemoryAllocated  = 0;
    TextureMemoryAllocated = 0;

    MainWindowHandle = WindowPool.NewWindow();

    SDL_Window*   pWindow   = MainWindowHandle.Handle; //pMainWindow->GetNativeHandle();
    SDL_GLContext windowCtx = (SDL_GLContext)MainWindowHandle.GLContext; //static_cast<GenericWindowGLImpl*>(pMainWindow.RawPtr())->GetGLContext();

    SDL_GL_MakeCurrent(pWindow, windowCtx);

    const char* vendorString = (const char*)glGetString(GL_VENDOR);
    vendorString             = vendorString ? vendorString : "Unknown";

    const char* adapterString = (const char*)glGetString(GL_RENDERER);
    adapterString             = adapterString ? adapterString : "Unknown";

    const char* driverVersion = (const char*)glGetString(GL_VERSION);
    driverVersion             = driverVersion ? driverVersion : "Unknown";

    LOG("Graphics vendor: {}\n", vendorString);
    LOG("Graphics adapter: {}\n", adapterString);
    LOG("Driver version: {}\n", driverVersion);

    if (Core::SubstringIcmp(vendorString, "NVIDIA") != -1)
    {
        GraphicsVendor = VENDOR_NVIDIA;
    }
    else if (Core::SubstringIcmp(vendorString, "ATI") != -1)
    {
        GraphicsVendor = VENDOR_ATI;
    }
    else if (Core::SubstringIcmp(vendorString, "Intel") != -1)
    {
        GraphicsVendor = VENDOR_INTEL;
    }

    int numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

#if 0
    int w, h;
    SDL_GetWindowSize( Desc.Window, &w, &h );
    if ( w < 1024 ) {
        for ( int i = 0 ; i < numExtensions ; i++ ) {
            LOG( "\t{}\n", (const char *)glGetStringi( GL_EXTENSIONS, i ) );
        }
    }
    else
#endif
    {
        char      str[128];
        const int MaxExtensionLength = 40;
        for (int i = 0; i < numExtensions;)
        {
            const char* extName1 = (const char*)glGetStringi(GL_EXTENSIONS, i);

            if (i + 1 < numExtensions)
            {
                const char* extName2 = (const char*)glGetStringi(GL_EXTENSIONS, i + 1);

                int strLen1 = Core::Strlen(extName1);
                int strLen2 = Core::Strlen(extName2);

                if (strLen1 < MaxExtensionLength && strLen2 < MaxExtensionLength)
                {
                    Core::Memset(&str[strLen1], ' ', MaxExtensionLength - strLen1);
                    Core::Memcpy(str, extName1, strLen1);
                    Core::Memcpy(&str[MaxExtensionLength], extName2, strLen2);
                    str[MaxExtensionLength + strLen2] = '\0';

                    LOG(" {}\n", str);

                    i += 2;
                    continue;
                }

                // long extension name
            }

            LOG(" {}\n", extName1);
            i += 1;
        }
    }

#if 0
    MemoryInfo gpuMemoryInfo = GetGPUMemoryInfo();
    if ( gpuMemoryInfo.TotalAvailableMegabytes > 0 && gpuMemoryInfo.CurrentAvailableMegabytes > 0 ) {
        LOG( "Total available GPU memory: {} Megs\n", gpuMemoryInfo.TotalAvailableMegabytes );
        LOG( "Current available GPU memory: {} Megs\n", gpuMemoryInfo.CurrentAvailableMegabytes );
    }
#endif

    FeatureSupport[FEATURE_HALF_FLOAT_VERTEX]  = FindExtension("GL_ARB_half_float_vertex");
    FeatureSupport[FEATURE_HALF_FLOAT_PIXEL]   = FindExtension("GL_ARB_half_float_pixel");
    FeatureSupport[FEATURE_TEXTURE_ANISOTROPY] = FindExtension("GL_ARB_texture_filter_anisotropic") || FindExtension("GL_EXT_texture_filter_anisotropic");
    FeatureSupport[FEATURE_SPARSE_TEXTURES]    = FindExtension("GL_ARB_sparse_texture"); // && FindExtension( "GL_ARB_sparse_texture2" );
    FeatureSupport[FEATURE_BINDLESS_TEXTURE]   = FindExtension("GL_ARB_bindless_texture");

#if defined(HK_OS_WIN32)
    FeatureSupport[FEATURE_SWAP_CONTROL]      = !!WGLEW_EXT_swap_control;
    FeatureSupport[FEATURE_SWAP_CONTROL_TEAR] = !!WGLEW_EXT_swap_control_tear;
#elif defined(HK_OS_LINUX)
    FeatureSupport[FEATURE_SWAP_CONTROL]      = !!GLXEW_EXT_swap_control || !!GLXEW_MESA_swap_control || !!GLXEW_SGI_swap_control;
    FeatureSupport[FEATURE_SWAP_CONTROL_TEAR] = !!GLXEW_EXT_swap_control_tear;
#else
#    error "Swap control tear checking not implemented on current platform"
#endif
    FeatureSupport[FEATURE_GPU_MEMORY_INFO] = FindExtension("GL_NVX_gpu_memory_info");

    FeatureSupport[FEATURE_SPIR_V] = FindExtension("GL_ARB_gl_spirv");

    if (!FindExtension("GL_EXT_texture_compression_s3tc"))
    {
        LOG("Warning: required extension GL_EXT_texture_compression_s3tc isn't supported\n");
    }

    if (!FindExtension("GL_ARB_texture_compression_rgtc") && !FindExtension("GL_EXT_texture_compression_rgtc"))
    {
        LOG("Warning: required extension GL_ARB_texture_compression_rgtc/GL_EXT_texture_compression_rgtc isn't supported\n");
    }

    if (!FindExtension("GL_EXT_texture_compression_rgtc"))
    {
        LOG("Warning: required extension GL_EXT_texture_compression_rgtc isn't supported\n");
    }

#if 0
    unsigned int MaxCombinedTextureImageUnits = GL_GetInteger( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS );
    //MaxVertexTextureImageUnits = GL_GetInteger( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS );
    unsigned int MaxImageUnits = GL_GetInteger( GL_MAX_IMAGE_UNITS );
#endif

    DeviceCaps[DEVICE_CAPS_MAX_VERTEX_BUFFER_SLOTS]  = GL_GetInteger(GL_MAX_VERTEX_ATTRIB_BINDINGS); // TODO: check if 0 ???
    DeviceCaps[DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE] = GL_GetInteger(GL_MAX_VERTEX_ATTRIB_STRIDE);   // GL_MAX_VERTEX_ATTRIB_STRIDE sinse GL v4.4
    if (DeviceCaps[DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE] == 0)
    {
        DeviceCaps[DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE] = std::numeric_limits<unsigned int>::max(); // Set some undefined value
    }
    DeviceCaps[DEVICE_CAPS_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET] = GL_GetInteger(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET);

    DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE] = GL_GetInteger(GL_MAX_TEXTURE_BUFFER_SIZE);

    DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT] = GL_GetInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT);
    if (!DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT])
    {
        LOG("Warning: TextureBufferOffsetAlignment == 0, using default alignment (256)\n");
        DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT] = 256;
    }

    DeviceCaps[DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT] = GL_GetInteger(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    if (!DeviceCaps[DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT])
    {
        LOG("Warning: ConstantBufferOffsetAlignment == 0, using default alignment (256)\n");
        DeviceCaps[DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT] = 256;
    }

    DeviceCaps[DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT] = GL_GetInteger(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    if (!DeviceCaps[DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT])
    {
        LOG("Warning: ShaderStorageBufferOffsetAlignment == 0, using default alignment (256)\n");
        DeviceCaps[DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT] = 256;
    }

    DeviceCaps[DEVICE_CAPS_MAX_CONSTANT_BUFFER_BINDINGS]       = GL_GetInteger(GL_MAX_UNIFORM_BUFFER_BINDINGS);
    DeviceCaps[DEVICE_CAPS_MAX_SHADER_STORAGE_BUFFER_BINDINGS] = GL_GetInteger(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
    DeviceCaps[DEVICE_CAPS_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS] = GL_GetInteger(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS);
    DeviceCaps[DEVICE_CAPS_MAX_TRANSFORM_FEEDBACK_BUFFERS]     = GL_GetInteger(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS);

    DeviceCaps[DEVICE_CAPS_CONSTANT_BUFFER_MAX_BLOCK_SIZE] = GL_GetInteger(GL_MAX_UNIFORM_BLOCK_SIZE);

    if (FeatureSupport[FEATURE_TEXTURE_ANISOTROPY])
    {
        DeviceCaps[DEVICE_CAPS_MAX_TEXTURE_ANISOTROPY] = (unsigned int)GL_GetFloat(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
    }
    else
    {
        DeviceCaps[DEVICE_CAPS_MAX_TEXTURE_ANISOTROPY] = 0;
    }

    DeviceCaps[DEVICE_CAPS_MAX_TEXTURE_SIZE]          = GL_GetInteger(GL_MAX_TEXTURE_SIZE);
    DeviceCaps[DEVICE_CAPS_MAX_TEXTURE_LAYERS]        = GL_GetInteger(GL_MAX_ARRAY_TEXTURE_LAYERS);
    DeviceCaps[DEVICE_CAPS_MAX_SPARSE_TEXTURE_LAYERS] = GL_GetInteger(GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS);
    DeviceCaps[DEVICE_CAPS_MAX_PATCH_VERTICES]        = GL_GetInteger(GL_MAX_PATCH_VERTICES);

#if 0
    LOG( "GL_MAX_TESS_CONTROL_INPUT_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_CONTROL_INPUT_COMPONENTS ) );
    LOG( "GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS ) );
    LOG( "GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS {}\n", GL_GetInteger( GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS ) );
    LOG( "GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS {}\n", GL_GetInteger( GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS ) );
    LOG( "GL_MAX_TESS_GEN_LEVEL {}\n", GL_GetInteger( GL_MAX_TESS_GEN_LEVEL ) );
    LOG( "GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS ) );
    LOG( "GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS ) );
    LOG( "GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS {}\n", GL_GetInteger( GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS ) );
    LOG( "GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS {}\n", GL_GetInteger( GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS ) );
    LOG( "GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS ) );
    LOG( "GL_MAX_TESS_PATCH_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_PATCH_COMPONENTS ) );
    LOG( "GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS ) );
    LOG( "GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS {}\n", GL_GetInteger( GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS ) );
    LOG( "GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS {}\n", GL_GetInteger( GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS ) );
    LOG( "GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS {}\n", GL_GetInteger( GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS ) );
#endif
    LOG("Features:\n");
    for (int i = 0; i < FEATURE_MAX; i++)
    {
        LOG("\t{}: {}\n", FeatureName[i], FeatureSupport[i] ? "Yes" : "No");
    }

    LOG("Device caps:\n");
    for (int i = 0; i < DEVICE_CAPS_MAX; i++)
    {
        LOG("\t{}: {}\n", DeviceCapName[i], DeviceCaps[i]);
    }

    if (FeatureSupport[FEATURE_GPU_MEMORY_INFO])
    {
        int32_t dedicated     = GL_GetInteger(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX);
        int32_t totalAvail    = GL_GetInteger(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX);
        int32_t currentAvail  = GL_GetInteger(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX);
        int32_t evictionCount = GL_GetInteger(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX);
        int32_t evictedMemory = GL_GetInteger(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX);

        LOG("Video memory info:\n");
        LOG("\tDedicated: {} Megs\n", dedicated >> 10);
        LOG("\tTotal available: {} Megs\n", totalAvail >> 10);
        LOG("\tCurrent available: {} Megs\n", currentAvail >> 10);
        LOG("\tEviction count: {}\n", evictionCount);
        LOG("\tEvicted memory: {} Megs\n", evictedMemory >> 10);
    }

    Allocator = DefaultAllocator;

    // Now device is initialized so we can initialize main window context here
    MainWindowHandle.ImmediateCtx = new ImmediateContextGLImpl(this, MainWindowHandle, true);
    ImmediateContextGLImpl::MakeCurrent(MainWindowHandle.ImmediateCtx);

    #if 0
    // Clear garbage on screen
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set initial swap interval and swap the buffers
    // FIXME: Is it need to be here?
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SwapWindow(pWindow);

    WindowPool.Destroy(window);
    #endif
}

DeviceGLImpl::~DeviceGLImpl()
{
    for (auto& it : Samplers)
    {
        SamplerInfo* sampler = it.second;
        glDeleteSamplers(1, &sampler->Id);

        Allocator.Deallocate(sampler);
    }

    for (auto& it : BlendingStates)
    {
        Allocator.Deallocate(it.second);
    }

    for (auto& it : RasterizerStates)
    {
        Allocator.Deallocate(it.second);
    }

    for (auto& it : DepthStencilStates)
    {
        Allocator.Deallocate(it.second);
    }

    MainWindowHandle.ImmediateCtx->RemoveRef();
    WindowPool.Free(MainWindowHandle);

    for (auto& it : VertexLayouts)
    {
        it.second->RemoveRef();
    }
}

IImmediateContext* DeviceGLImpl::GetImmediateContext()
{
    return MainWindowHandle.ImmediateCtx;
}

void DeviceGLImpl::GetOrCreateMainWindow(WindowSettings const& windowSettings, Ref<IGenericWindow>* ppWindow)
{
    if (pMainWindow.IsExpired())
    {
        *ppWindow = MakeRef<GenericWindowGLImpl>(this, windowSettings, WindowPool, MainWindowHandle);
        pMainWindow = *ppWindow;
    }
    else
    {
        *ppWindow = pMainWindow;
    }
}

void DeviceGLImpl::CreateGenericWindow(WindowSettings const& windowSettings, Ref<IGenericWindow>* ppWindow)
{
    WindowPoolGL::WindowGL dummyHandle = {};

    *ppWindow = MakeRef<GenericWindowGLImpl>(this, windowSettings, WindowPool, dummyHandle);
}

void DeviceGLImpl::CreateSwapChain(IGenericWindow* pWindow, Ref<ISwapChain>* ppSwapChain)
{
    *ppSwapChain = MakeRef<SwapChainGLImpl>(this, static_cast<GenericWindowGLImpl*>(pWindow));
}

void DeviceGLImpl::CreatePipeline(PipelineDesc const& Desc, Ref<IPipeline>* ppPipeline)
{
    *ppPipeline = MakeRef<PipelineGLImpl>(this, Desc);
}

void DeviceGLImpl::CreateShaderFromBinary(ShaderBinaryData const* _BinaryData, Ref<IShaderModule>* ppShaderModule)
{
    *ppShaderModule = MakeRef<ShaderModuleGLImpl>(this, _BinaryData);
}

void DeviceGLImpl::CreateShaderFromCode(SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources, Ref<IShaderModule>* ppShaderModule)
{
    *ppShaderModule = MakeRef<ShaderModuleGLImpl>(this, _ShaderType, _NumSources, _Sources);
}

void DeviceGLImpl::CreateBuffer(BufferDesc const& Desc, const void* _SysMem, Ref<IBuffer>* ppBuffer)
{
    *ppBuffer = MakeRef<BufferGLImpl>(this, Desc, _SysMem);
}

void DeviceGLImpl::CreateTexture(TextureDesc const& Desc, Ref<ITexture>* ppTexture)
{
    *ppTexture = MakeRef<TextureGLImpl>(this, Desc);
}

void DeviceGLImpl::CreateSparseTexture(SparseTextureDesc const& Desc, Ref<ISparseTexture>* ppTexture)
{
    *ppTexture = MakeRef<SparseTextureGLImpl>(this, Desc);
}

void DeviceGLImpl::CreateTransformFeedback(TransformFeedbackDesc const& Desc, Ref<ITransformFeedback>* ppTransformFeedback)
{
    *ppTransformFeedback = MakeRef<TransformFeedbackGLImpl>(this, Desc);
}

void DeviceGLImpl::CreateQueryPool(QueryPoolDesc const& Desc, Ref<IQueryPool>* ppQueryPool)
{
    *ppQueryPool = MakeRef<QueryPoolGLImpl>(this, Desc);
}

void DeviceGLImpl::CreateResourceTable(Ref<IResourceTable>* ppResourceTable)
{
    *ppResourceTable = MakeRef<ResourceTableGLImpl>(this);
}


bool DeviceGLImpl::CreateShaderBinaryData(SHADER_TYPE        _ShaderType,
                                           unsigned int       _NumSources,
                                           const char* const* _Sources,
                                           ShaderBinaryData* _BinaryData)
{
    return ShaderModuleGLImpl::CreateShaderBinaryData(this, _ShaderType, _NumSources, _Sources, _BinaryData);
}

void DeviceGLImpl::DestroyShaderBinaryData(ShaderBinaryData* _BinaryData)
{
    ShaderModuleGLImpl::DestroyShaderBinaryData(this, _BinaryData);
}

AllocatorCallback const& DeviceGLImpl::GetAllocator() const
{
    return Allocator;
}

int32_t DeviceGLImpl::GetGPUMemoryTotalAvailable()
{
    if (FeatureSupport[FEATURE_GPU_MEMORY_INFO])
    {
        return GL_GetInteger(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX);
    }
    else
    {
        LOG("DeviceGLImpl::GetGPUMemoryTotalAvailable: FEATURE_GPU_MEMORY_INFO is not supported by video driver\n");
    }
    return 0;
}

int32_t DeviceGLImpl::GetGPUMemoryCurrentAvailable()
{
    if (FeatureSupport[FEATURE_GPU_MEMORY_INFO])
    {
        return GL_GetInteger(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX);
    }
    else
    {
        LOG("DeviceGLImpl::GetGPUMemoryTotalAvailable: FEATURE_GPU_MEMORY_INFO is not supported by video driver\n");
    }
    return 0;
}

VertexLayoutGL* DeviceGLImpl::GetVertexLayout(VertexBindingInfo const* pVertexBindings,
                                                uint32_t                  NumVertexBindings,
                                                VertexAttribInfo const*  pVertexAttribs,
                                                uint32_t                  NumVertexAttribs)
{
    VertexLayoutDescGL desc;

    desc.NumVertexBindings = NumVertexBindings;
    if (desc.NumVertexBindings > MAX_VERTEX_BINDINGS)
    {
        desc.NumVertexBindings = MAX_VERTEX_BINDINGS;

        LOG("DeviceGLImpl::GetVertexLayout: NumVertexBindings > MAX_VERTEX_BINDINGS\n");
    }
    Core::Memcpy(desc.VertexBindings, pVertexBindings, sizeof(desc.VertexBindings[0]) * desc.NumVertexBindings);

    desc.NumVertexAttribs = NumVertexAttribs;
    if (desc.NumVertexAttribs > MAX_VERTEX_ATTRIBS)
    {
        desc.NumVertexAttribs = MAX_VERTEX_ATTRIBS;

        LOG("DeviceGLImpl::GetVertexLayout: NumVertexAttribs > MAX_VERTEX_ATTRIBS\n");
    }
    Core::Memcpy(desc.VertexAttribs, pVertexAttribs, sizeof(desc.VertexAttribs[0]) * desc.NumVertexAttribs);

    // Clear semantic name to have proper hash key
    for (int i = 0; i < desc.NumVertexAttribs; i++)
    {
        desc.VertexAttribs[i].SemanticName = nullptr;
    }

    VertexLayoutGL*& vertexLayout = VertexLayouts[desc];
    if (vertexLayout)
    {
        //LOG("Caching vertex layout\n");
        return vertexLayout;
    }

    // Validate

    for (VertexBindingInfo const* binding = desc.VertexBindings; binding < &desc.VertexBindings[desc.NumVertexBindings]; binding++)
    {
        HK_ASSERT(binding->InputSlot < MAX_VERTEX_BUFFER_SLOTS);

        if (binding->InputSlot >= GetDeviceCaps(DEVICE_CAPS_MAX_VERTEX_BUFFER_SLOTS))
        {
            LOG("DeviceGLImpl::GetVertexLayout: binding->InputSlot >= MaxVertexBufferSlots\n");
        }

        if (binding->Stride > GetDeviceCaps(DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE))
        {
            LOG("DeviceGLImpl::GetVertexLayout: binding->Stride > MaxVertexAttribStride\n");
        }
    }

    for (VertexAttribInfo const* attrib = desc.VertexAttribs; attrib < &desc.VertexAttribs[desc.NumVertexAttribs]; attrib++)
    {
        if (attrib->Offset > GetDeviceCaps(DEVICE_CAPS_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET))
        {
            LOG("DeviceGLImpl::GetVertexLayout: attrib offset > MaxVertexAttribRelativeOffset\n");
        }
    }

    Ref<VertexLayoutGL> pVertexLayout;
    pVertexLayout = MakeRef<VertexLayoutGL>(desc);
    pVertexLayout->AddRef();
    vertexLayout = pVertexLayout;

    //LOG("Create vertex layout, total {}\n", VertexLayouts.Size());

    return pVertexLayout;
}

BlendingStateInfo const* DeviceGLImpl::CachedBlendingState(BlendingStateInfo const& _BlendingState)
{
    BlendingStateInfo*& state = BlendingStates[_BlendingState];
    if (state)
    {
        //LOG( "Caching blending state\n" );
        return state;
    }

    state = static_cast<BlendingStateInfo*>(Allocator.Allocate(sizeof(BlendingStateInfo)));
    Core::Memcpy(state, &_BlendingState, sizeof(*state));

    //LOG( "Total blending states {}\n", BlendingStates.Size() );

    return state;
}

RasterizerStateInfo const* DeviceGLImpl::CachedRasterizerState(RasterizerStateInfo const& _RasterizerState)
{
    RasterizerStateInfo*& state = RasterizerStates[_RasterizerState];
    if (state)
    {
        //LOG( "Caching rasterizer state\n" );
        return state;
    }

    state = static_cast<RasterizerStateInfo*>(Allocator.Allocate(sizeof(RasterizerStateInfo)));
    Core::Memcpy(state, &_RasterizerState, sizeof(*state));

    //LOG( "Total rasterizer states {}\n", RasterizerStates.Size() );

    return state;
}

DepthStencilStateInfo const* DeviceGLImpl::CachedDepthStencilState(DepthStencilStateInfo const& _DepthStencilState)
{
    DepthStencilStateInfo*& state = DepthStencilStates[_DepthStencilState];
    if (state)
    {
        //LOG( "Caching depth stencil state\n" );
        return state;
    }

    state = static_cast<DepthStencilStateInfo*>(Allocator.Allocate(sizeof(DepthStencilStateInfo)));
    Core::Memcpy(state, &_DepthStencilState, sizeof(*state));

    //LOG( "Total depth stencil states {}\n", DepthStencilStates.Size() );

    return state;
}

unsigned int DeviceGLImpl::CachedSampler(SamplerDesc const& SamplerDesc)
{
    SamplerInfo*& sampler = Samplers[SamplerDesc];
    if (sampler)
    {
        //LOG( "Caching sampler\n" );
        return sampler->Id;
    }

    sampler = static_cast<SamplerInfo*>(Allocator.Allocate(sizeof(SamplerInfo)));
    Core::Memcpy(&sampler->Desc, &SamplerDesc, sizeof(sampler->Desc));

    //LOG( "Total samplers {}\n", Samplers.Size() );

    // 3.3 or GL_ARB_sampler_objects

    GLuint id;

    glCreateSamplers(1, &id); // 4.5

    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, SamplerFilterModeLUT[SamplerDesc.Filter].Min);
    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, SamplerFilterModeLUT[SamplerDesc.Filter].Mag);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, SamplerAddressModeLUT[SamplerDesc.AddressU]);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, SamplerAddressModeLUT[SamplerDesc.AddressV]);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_R, SamplerAddressModeLUT[SamplerDesc.AddressW]);
    glSamplerParameterf(id, GL_TEXTURE_LOD_BIAS, SamplerDesc.MipLODBias);
    if (FeatureSupport[FEATURE_TEXTURE_ANISOTROPY] && SamplerDesc.MaxAnisotropy > 0)
    {
        glSamplerParameteri(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, Math::Clamp<unsigned int>(SamplerDesc.MaxAnisotropy, 1u, DeviceCaps[DEVICE_CAPS_MAX_TEXTURE_ANISOTROPY]));
    }
    if (SamplerDesc.bCompareRefToTexture)
    {
        glSamplerParameteri(id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    }
    glSamplerParameteri(id, GL_TEXTURE_COMPARE_FUNC, ComparisonFuncLUT[SamplerDesc.ComparisonFunc]);
    glSamplerParameterfv(id, GL_TEXTURE_BORDER_COLOR, SamplerDesc.BorderColor);
    glSamplerParameterf(id, GL_TEXTURE_MIN_LOD, SamplerDesc.MinLOD);
    glSamplerParameterf(id, GL_TEXTURE_MAX_LOD, SamplerDesc.MaxLOD);
    glSamplerParameteri(id, GL_TEXTURE_CUBE_MAP_SEAMLESS, SamplerDesc.bCubemapSeamless);

    // We can use also these functions to retrieve sampler parameters:
    //glGetSamplerParameterfv
    //glGetSamplerParameterIiv
    //glGetSamplerParameterIuiv
    //glGetSamplerParameteriv

    sampler->Id = id;

    return id;
}

bool DeviceGLImpl::EnumerateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int* NumPageSizes, int* PageSizesX, int* PageSizesY, int* PageSizesZ)
{
    GLenum target         = SparseTextureTargetLUT[Type].Target;
    GLenum internalFormat = InternalFormatLUT[Format].InternalFormat;

    HK_ASSERT(NumPageSizes != nullptr);

    *NumPageSizes = 0;

    if (!FeatureSupport[FEATURE_SPARSE_TEXTURES])
    {
        LOG("DeviceGLImpl::EnumerateSparseTexturePageSize: sparse textures are not supported by video driver\n");
        return false;
    }

    glGetInternalformativ(target, internalFormat, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, 1, NumPageSizes);

    if (PageSizesX)
    {
        glGetInternalformativ(target, internalFormat, GL_VIRTUAL_PAGE_SIZE_X_ARB, *NumPageSizes, PageSizesX);
    }

    if (PageSizesY)
    {
        glGetInternalformativ(target, internalFormat, GL_VIRTUAL_PAGE_SIZE_Y_ARB, *NumPageSizes, PageSizesY);
    }

    if (PageSizesZ)
    {
        glGetInternalformativ(target, internalFormat, GL_VIRTUAL_PAGE_SIZE_Z_ARB, *NumPageSizes, PageSizesZ);
    }

    return *NumPageSizes > 0;
}

bool DeviceGLImpl::ChooseAppropriateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int* PageSizeIndex, int* PageSizeX, int* PageSizeY, int* PageSizeZ)
{
    HK_ASSERT(PageSizeIndex != nullptr);

    *PageSizeIndex = -1;

    if (PageSizeX)
    {
        *PageSizeX = 0;
    }

    if (PageSizeY)
    {
        *PageSizeY = 0;
    }

    if (PageSizeZ)
    {
        *PageSizeZ = 0;
    }

    int pageSizes;
    EnumerateSparseTexturePageSize(Type, Format, &pageSizes, nullptr, nullptr, nullptr);

    if (pageSizes <= 0)
    {
        return false;
    }

    switch (Type)
    {
        case SPARSE_TEXTURE_2D:
        case SPARSE_TEXTURE_2D_ARRAY:
        case SPARSE_TEXTURE_CUBE_MAP:
        case SPARSE_TEXTURE_CUBE_MAP_ARRAY: {
            int* pageSizeX = (int*)HkStackAlloc(pageSizes * sizeof(int));
            int* pageSizeY = (int*)HkStackAlloc(pageSizes * sizeof(int));
            EnumerateSparseTexturePageSize(Type, Format, &pageSizes, pageSizeX, pageSizeY, nullptr);

            for (int i = 0; i < pageSizes; i++)
            {
                if ((Width % pageSizeX[i]) == 0 && (Height % pageSizeY[i]) == 0)
                {
                    *PageSizeIndex = i;
                    if (PageSizeX)
                    {
                        *PageSizeX = pageSizeX[i];
                    }
                    if (PageSizeY)
                    {
                        *PageSizeY = pageSizeY[i];
                    }
                    if (PageSizeZ)
                    {
                        *PageSizeZ = 1;
                    }
                    break;
                }
            }
            break;
        }
        case SPARSE_TEXTURE_3D: {
            int* pageSizeX = (int*)HkStackAlloc(pageSizes * sizeof(int));
            int* pageSizeY = (int*)HkStackAlloc(pageSizes * sizeof(int));
            int* pageSizeZ = (int*)HkStackAlloc(pageSizes * sizeof(int));
            EnumerateSparseTexturePageSize(Type, Format, &pageSizes, pageSizeX, pageSizeY, pageSizeZ);

            for (int i = 0; i < pageSizes; i++)
            {
                if ((Width % pageSizeX[i]) == 0 && (Height % pageSizeY[i]) == 0 && (Depth % pageSizeZ[i]) == 0)
                {
                    *PageSizeIndex = i;
                    if (PageSizeX)
                    {
                        *PageSizeX = pageSizeX[i];
                    }
                    if (PageSizeY)
                    {
                        *PageSizeY = pageSizeY[i];
                    }
                    if (PageSizeZ)
                    {
                        *PageSizeZ = pageSizeZ[i];
                    }
                    break;
                }
            }
            break;
        }
        default:
            HK_ASSERT(0);
    }

    return *PageSizeIndex != -1;
}

static void DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    const char* sourceStr;
    switch (source)
    {
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

    const char* typeStr;
    switch (type)
    {
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

    const char* severityStr;
    switch (severity)
    {
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

    if (type == GL_DEBUG_TYPE_OTHER && severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        // Do not print annoying notifications
        return;
    }

    LOG("-----------------------------------\n"
        "{} {}\n"
        "{}: {} (Id {})\n"
        "-----------------------------------\n",
        sourceStr, typeStr, severityStr, message, id);
}






WindowPoolGL::WindowPoolGL()
{
}

WindowPoolGL::~WindowPoolGL()
{
    for (WindowGL& window : Pool)
    {
        Free(window);
    }
}

WindowPoolGL::WindowGL WindowPoolGL::Create()
{
    if (!Pool.IsEmpty())
    {
        WindowGL window = Pool.Last();
        Pool.RemoveLast();
        return window;
    }

    return NewWindow();
}

WindowPoolGL::WindowGL WindowPoolGL::NewWindow()
{
    static bool bInitSDLSubsystems = true;

    if (bInitSDLSubsystems)
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_SENSOR | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);
        bInitSDLSubsystems = false;
    }

    SDL_Window*   prevWindow  = SDL_GL_GetCurrentWindow();
    SDL_GLContext prevContext = SDL_GL_GetCurrentContext();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
#ifdef HK_DEBUG
                        | SDL_GL_CONTEXT_DEBUG_FLAG
#endif
    );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, prevContext ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

    WindowGL window;
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 0);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    window.Handle = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!window.Handle)
    {
        CoreApplication::TerminateWithError("Failed to create window\n");
    }

    SDL_StartTextInput(window.Handle);

    window.GLContext = SDL_GL_CreateContext(window.Handle);
    if (!window.GLContext)
    {
        CoreApplication::TerminateWithError("Failed to initialize OpenGL context\n");
    }

    SDL_GL_MakeCurrent(window.Handle, (SDL_GLContext)window.GLContext);

    // Set glewExperimental=true to allow extension entry points to be loaded even if extension isn't present
    // in the driver's extensions string
    glewExperimental = true;

    GLenum result = glewInit();
    if (result != GLEW_OK)
    {
        CoreApplication::TerminateWithError("Failed to load OpenGL functions\n");
    }

    // GLEW has a long-existing bug where calling glewInit() always sets the GL_INVALID_ENUM error flag and
    // thus the first glGetError will always return an error code which can throw you completely off guard.
    // To fix this it's advised to simply call glGetError after glewInit to clear the flag.
    (void)glGetError();

#ifdef HK_DEBUG
    GLint contextFlags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
    if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DebugMessageCallback, NULL);
    }
#endif

    SDL_GL_MakeCurrent(prevWindow, prevContext);

    window.ImmediateCtx = nullptr;
    return window;
}

void WindowPoolGL::Destroy(WindowPoolGL::WindowGL Window)
{
    SDL_HideWindow(Window.Handle);
    Pool.Add(Window);
}

void WindowPoolGL::Free(WindowPoolGL::WindowGL Window)
{
    SDL_Window*   prevWindow  = SDL_GL_GetCurrentWindow();
    SDL_GLContext prevContext = SDL_GL_GetCurrentContext();

    SDL_StopTextInput(Window.Handle);

    if (Window.GLContext)
    {
        SDL_GL_DestroyContext((SDL_GLContext)Window.GLContext);
    }

    if (Window.Handle)
    {
        SDL_DestroyWindow(Window.Handle);
    }

    if (Window.GLContext != prevContext)
    {
        SDL_GL_MakeCurrent(prevWindow, prevContext);
    }
}

} // namespace RenderCore

HK_NAMESPACE_END
