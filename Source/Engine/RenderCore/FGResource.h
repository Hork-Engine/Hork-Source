/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "BufferView.h"
#include <Containers/Public/PodVector.h>

namespace RenderCore
{

class FGRenderTaskBase;

enum FG_RESOURCE_ACCESS
{
    FG_RESOURCE_ACCESS_READ,
    FG_RESOURCE_ACCESS_WRITE,
    FG_RESOURCE_ACCESS_READ_WRITE
};

class FGResourceProxyBase
{
public:
    explicit FGResourceProxyBase(std::size_t ResourceId, const char* Name, FGRenderTaskBase* RenderTask, DEVICE_OBJECT_PROXY_TYPE ProxyType) :
        Id(ResourceId), Name(Name), Creator(RenderTask), ResourceRefs(0), bCaptured(false), ProxyType(ProxyType)
    {}

    FGResourceProxyBase(FGResourceProxyBase const&) = delete;
    FGResourceProxyBase(FGResourceProxyBase&&)      = default;

    virtual ~FGResourceProxyBase() = default;

    FGResourceProxyBase& operator=(FGResourceProxyBase const&) = delete;
    FGResourceProxyBase& operator=(FGResourceProxyBase&&) = default;

    void SetResourceCapture(bool InbCaptured)
    {
        bCaptured = InbCaptured;
    }

    std::size_t GetId() const
    {
        return Id;
    }

    const char* GetName() const
    {
        return Name;
    }

    bool IsTransient() const
    {
        return Creator != nullptr;
    }

    bool IsCaptured() const
    {
        return bCaptured;
    }

    DEVICE_OBJECT_PROXY_TYPE GetProxyType() const
    {
        return ProxyType;
    }

    void SetDeviceObject(IDeviceObject* DeviceObject)
    {
        pDeviceObject = DeviceObject;
    }

    IDeviceObject* GetDeviceObject() const
    {
        return pDeviceObject;
    }

private:
    const std::size_t                   Id;
    const char*                         Name;
    FGRenderTaskBase const*             Creator;
    TPodVector<FGRenderTaskBase const*> Readers;
    TPodVector<FGRenderTaskBase const*> Writers;
    int                                 ResourceRefs;
    bool                                bCaptured;
    DEVICE_OBJECT_PROXY_TYPE            ProxyType{DEVICE_OBJECT_TYPE_UNKNOWN};
    IDeviceObject*                      pDeviceObject{nullptr};

    friend class AFrameGraph;
    friend class FGRenderTaskBase;
};

template <typename TResourceDesc, typename TResource>
class FGResourceProxy : public FGResourceProxyBase
{
public:
    using ResourceDesc = TResourceDesc;
    using ResourceType = TResource;

    // Construct internal (transient) resource
    explicit FGResourceProxy(std::size_t ResourceId, const char* Name, FGRenderTaskBase* RenderTask, TResourceDesc const& ResourceDesc) :
        FGResourceProxyBase(ResourceId, Name, RenderTask, TResource::PROXY_TYPE), Desc(ResourceDesc)
    {}

    // Construct external resource
    explicit FGResourceProxy(std::size_t ResourceId, const char* Name, TResource* InResource) :
        FGResourceProxyBase(ResourceId, Name, nullptr, TResource::PROXY_TYPE), Desc(InResource->GetDesc())
    {
        SetDeviceObject(InResource);
    }

    FGResourceProxy(FGResourceProxy const&) = delete;
    FGResourceProxy(FGResourceProxy&&)      = default;
    ~FGResourceProxy()                      = default;

    FGResourceProxy& operator=(FGResourceProxy const&) = delete;
    FGResourceProxy& operator=(FGResourceProxy&&) = default;

    ResourceType* Actual() const
    {
        return static_cast<ResourceType*>(GetDeviceObject());
    }

    TResourceDesc const& GetResourceDesc() const
    {
        return Desc;
    }

private:
    TResourceDesc Desc;
};

class FGTextureProxy : public FGResourceProxy<STextureDesc, ITexture>
{
    using Super = FGResourceProxy<STextureDesc, ITexture>;

public:
    // Construct internal (transient) resource
    explicit FGTextureProxy(std::size_t ResourceId, const char* Name, FGRenderTaskBase* RenderTask, STextureDesc const& ResourceDesc) :
        Super(ResourceId, Name, RenderTask, ResourceDesc)
    {}

    // Construct external resource
    explicit FGTextureProxy(std::size_t ResourceId, const char* Name, ITexture* InResource) :
        Super(ResourceId, Name, InResource)
    {}
};

class FGBufferViewProxy : public FGResourceProxy<SBufferViewDesc, IBufferView>
{
    using Super = FGResourceProxy<SBufferViewDesc, IBufferView>;

public:
    // Construct internal (transient) resource
    explicit FGBufferViewProxy(std::size_t ResourceId, const char* Name, FGRenderTaskBase* RenderTask, SBufferViewDesc const& ResourceDesc) :
        Super(ResourceId, Name, RenderTask, ResourceDesc)
    {}

    // Construct external resource
    explicit FGBufferViewProxy(std::size_t ResourceId, const char* Name, IBufferView* InResource) :
        Super(ResourceId, Name, InResource)
    {}
};

} // namespace RenderCore
