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

#pragma once

#include "ImmediateContext.h"
#include "BufferView.h"
#include "Texture.h"
#include "SparseTexture.h"
#include "Sampler.h"
#include "Query.h"
#include "Pipeline.h"
#include "ShaderModule.h"
#include "TransformFeedback.h"
#include "SwapChain.h"

#include <Core/Public/Hash.h>
#include <Core/Public/PodVector.h>
#include <Core/Public/Ref.h>

namespace RenderCore
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

struct SAllocatorCallback
{
    void* (*Allocate)(size_t _BytesCount);
    void (*Deallocate)(void* _Bytes);
};

class IDevice : public ARefCounted
{
public:
    ~IDevice();

    virtual void CreateImmediateContext(SImmediateContextDesc const& Desc, TRef<IImmediateContext>* ppImmediateContext) = 0;

    virtual void CreateSwapChain(SDL_Window* pWindow, TRef<ISwapChain>* ppSwapChain) = 0;

    virtual void CreatePipeline(SPipelineDesc const& Desc, TRef<IPipeline>* ppPipeline) = 0;

    virtual void CreateShaderFromBinary(SShaderBinaryData const* _BinaryData, TRef<IShaderModule>* ppShaderModule)                                         = 0;
    virtual void CreateShaderFromCode(SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources, TRef<IShaderModule>* ppShaderModule) = 0;

    virtual void CreateBuffer(SBufferDesc const& Desc, const void* _SysMem, TRef<IBuffer>* ppBuffer) = 0;

    virtual void CreateTexture(STextureDesc const& Desc, TRef<ITexture>* ppTexture) = 0;

    /** FEATURE_SPARSE_TEXTURES must be supported */
    virtual void CreateSparseTexture(SSparseTextureDesc const& Desc, TRef<ISparseTexture>* ppTexture) = 0;

    virtual void CreateTransformFeedback(STransformFeedbackDesc const& Desc, TRef<ITransformFeedback>* ppTransformFeedback) = 0;

    virtual void CreateQueryPool(SQueryPoolDesc const& Desc, TRef<IQueryPool>* ppQueryPool) = 0;

    /** FEATURE_BINDLESS_TEXTURE must be supported */
    virtual void GetBindlessSampler(ITexture* pTexture, SSamplerDesc const& Desc, TRef<IBindlessSampler>* ppBindlessSampler) = 0;

    virtual void CreateResourceTable(TRef<IResourceTable>* ppResourceTable) = 0;

    virtual bool CreateShaderBinaryData(SHADER_TYPE        _ShaderType,
                                        unsigned int       _NumSources,
                                        const char* const* _Sources,
                                        SShaderBinaryData* _BinaryData) = 0;

    virtual void DestroyShaderBinaryData(SShaderBinaryData* _BinaryData) = 0;

    /** Get total available GPU memory in kB. FEATURE_GPU_MEMORY_INFO must be supported */
    virtual int32_t GetGPUMemoryTotalAvailable() = 0;

    /** Get current available GPU memory in kB. FEATURE_GPU_MEMORY_INFO must be supported */
    virtual int32_t GetGPUMemoryCurrentAvailable() = 0;

    virtual bool EnumerateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int* NumPageSizes, int* PageSizesX, int* PageSizesY, int* PageSizesZ) = 0;

    virtual bool ChooseAppropriateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int* PageSizeIndex, int* PageSizeX = nullptr, int* PageSizeY = nullptr, int* PageSizeZ = nullptr) = 0;

    virtual bool LookupImageFormat(const char* _FormatQualifier, TEXTURE_FORMAT* _Format) = 0;

    virtual const char* LookupImageFormatQualifier(TEXTURE_FORMAT _Format) = 0;

    virtual SAllocatorCallback const& GetAllocator() const = 0;

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

#ifdef AN_DEBUG
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

    TArray<unsigned int, DEVICE_CAPS_MAX> DeviceCaps = {};
    TArray<bool, FEATURE_MAX>             FeatureSupport = {};

private:
    TArray<int, DEVICE_OBJECT_TYPE_MAX> ObjectCounters = {};

#ifdef AN_DEBUG
    IDeviceObject* ListHead = nullptr;
    IDeviceObject* ListTail = nullptr;
#endif
    friend class IDeviceObject;
};

typedef int (*HashCallback)(const unsigned char* _Data, int _Size);

void CreateLogicalDevice(SImmediateContextDesc const& Desc,
                         SAllocatorCallback const*    Allocator,
                         HashCallback                 Hash,
                         TRef<IDevice>*               ppDevice,
                         TRef<IImmediateContext>*     ppImmediateContext);

} // namespace RenderCore
