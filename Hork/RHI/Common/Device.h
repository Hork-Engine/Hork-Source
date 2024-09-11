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

#pragma once

#include "ImmediateContext.h"
#include "BufferView.h"
#include "Texture.h"
#include "SparseTexture.h"
#include "Query.h"
#include "Pipeline.h"
#include "ShaderModule.h"
#include "TransformFeedback.h"
#include "SwapChain.h"
#include "GenericWindow.h"

#include <Hork/Core/Ref.h>
#include <Hork/Core/Containers/Hash.h>
#include <Hork/Core/Containers/Array.h>

HK_NAMESPACE_BEGIN

namespace RHI
{

enum FEATURE_TYPE
{
    FEATURE_HALF_FLOAT_VERTEX,
    FEATURE_HALF_FLOAT_PIXEL,
    FEATURE_TEXTURE_ANISOTROPY,
    FEATURE_SPARSE_TEXTURES,
    FEATURE_BINDLESS_TEXTURE,
    FEATURE_SWAP_CONTROL,
    FEATURE_SWAP_CONTROL_TEAR,
    FEATURE_GPU_MEMORY_INFO,
    FEATURE_SPIR_V,

    FEATURE_MAX
};

enum DEVICE_CAPS
{
    DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE,
    DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT,

    DEVICE_CAPS_CONSTANT_BUFFER_OFFSET_ALIGNMENT,
    DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,

    DEVICE_CAPS_MAX_TEXTURE_SIZE,
    DEVICE_CAPS_MAX_TEXTURE_LAYERS,
    DEVICE_CAPS_MAX_SPARSE_TEXTURE_LAYERS,
    DEVICE_CAPS_MAX_TEXTURE_ANISOTROPY,
    DEVICE_CAPS_MAX_PATCH_VERTICES,
    DEVICE_CAPS_MAX_VERTEX_BUFFER_SLOTS,
    DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE,
    DEVICE_CAPS_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET,
    DEVICE_CAPS_MAX_CONSTANT_BUFFER_BINDINGS,
    DEVICE_CAPS_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
    DEVICE_CAPS_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,
    DEVICE_CAPS_MAX_TRANSFORM_FEEDBACK_BUFFERS,

    DEVICE_CAPS_CONSTANT_BUFFER_MAX_BLOCK_SIZE,

    DEVICE_CAPS_MAX
};

enum GRAPHICS_VENDOR
{
    VENDOR_UNKNOWN,
    VENDOR_NVIDIA,
    VENDOR_ATI,
    VENDOR_INTEL
};

struct AllocatorCallback
{
    void* (*Allocate)(size_t _BytesCount);
    void (*Deallocate)(void* _Bytes);
};

class IDevice : public RefCounted
{
public:
    ~IDevice();

    virtual IImmediateContext* GetImmediateContext() = 0;

    virtual void GetOrCreateMainWindow(WindowSettings const& windowSettings, Ref<IGenericWindow>* ppWindow) = 0;

    virtual void CreateGenericWindow(WindowSettings const& windowSettings, Ref<IGenericWindow>* ppWindow) = 0;

    virtual void CreateSwapChain(IGenericWindow* pWindow, Ref<ISwapChain>* ppSwapChain) = 0;

    virtual void CreatePipeline(PipelineDesc const& Desc, Ref<IPipeline>* ppPipeline) = 0;

    virtual void CreateShaderFromBinary(ShaderBinaryData const* _BinaryData, Ref<IShaderModule>* ppShaderModule)                                         = 0;
    virtual void CreateShaderFromCode(SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources, Ref<IShaderModule>* ppShaderModule) = 0;

    virtual void CreateBuffer(BufferDesc const& Desc, const void* _SysMem, Ref<IBuffer>* ppBuffer) = 0;

    virtual void CreateTexture(TextureDesc const& Desc, Ref<ITexture>* ppTexture) = 0;

    /// FEATURE_SPARSE_TEXTURES must be supported
    virtual void CreateSparseTexture(SparseTextureDesc const& Desc, Ref<ISparseTexture>* ppTexture) = 0;

    virtual void CreateTransformFeedback(TransformFeedbackDesc const& Desc, Ref<ITransformFeedback>* ppTransformFeedback) = 0;

    virtual void CreateQueryPool(QueryPoolDesc const& Desc, Ref<IQueryPool>* ppQueryPool) = 0;

    virtual void CreateResourceTable(Ref<IResourceTable>* ppResourceTable) = 0;

    virtual bool CreateShaderBinaryData(SHADER_TYPE        _ShaderType,
                                        unsigned int       _NumSources,
                                        const char* const* _Sources,
                                        ShaderBinaryData* _BinaryData) = 0;

    virtual void DestroyShaderBinaryData(ShaderBinaryData* _BinaryData) = 0;

    /// Get total available GPU memory in kB. FEATURE_GPU_MEMORY_INFO must be supported
    virtual int32_t GetGPUMemoryTotalAvailable() = 0;

    /// Get current available GPU memory in kB. FEATURE_GPU_MEMORY_INFO must be supported
    virtual int32_t GetGPUMemoryCurrentAvailable() = 0;

    virtual bool EnumerateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int* NumPageSizes, int* PageSizesX, int* PageSizesY, int* PageSizesZ) = 0;

    virtual bool ChooseAppropriateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int* PageSizeIndex, int* PageSizeX = nullptr, int* PageSizeY = nullptr, int* PageSizeZ = nullptr) = 0;

    virtual AllocatorCallback const& GetAllocator() const = 0;

    GRAPHICS_VENDOR GetGraphicsVendor() const
    {
        return GraphicsVendor;
    }

    bool IsFeatureSupported(FEATURE_TYPE FeatureType) const
    {
        return FeatureSupport[FeatureType];
    }

    unsigned int GetDeviceCaps(DEVICE_CAPS DevCaps) const
    {
        return DeviceCaps[DevCaps];
    }

    int GetObjectCount(DEVICE_OBJECT_PROXY_TYPE ProxyType) const
    {
        return ObjectCounters[ProxyType];
    }

#ifdef HK_DEBUG
    IDeviceObject* GetDeviceObjects_DEBUG()
    {
        return ListHead;
    }

    IDeviceObject* FindDeviceObject_DEBUG(uint64_t UID)
    {
        for (IDeviceObject* object = ListHead; object; object = object->GetNext_DEBUG())
        {
            if (object->GetUID() == UID)
            {
                return object;
            }
        }
        return nullptr;
    }
#endif

protected:
    GRAPHICS_VENDOR GraphicsVendor = VENDOR_UNKNOWN;

    Array<unsigned int, DEVICE_CAPS_MAX> DeviceCaps = {};
    Array<bool, FEATURE_MAX>             FeatureSupport = {};

private:
    Array<int, DEVICE_OBJECT_TYPE_MAX> ObjectCounters = {};

#ifdef HK_DEBUG
    IDeviceObject* ListHead = nullptr;
    IDeviceObject* ListTail = nullptr;
#endif
    friend class IDeviceObject;
};

} // namespace RHI

HK_NAMESPACE_END
