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

#include "BufferView.h"
#include <Core/Public/PodVector.h>

namespace RenderCore
{

class AFrameGraph;
class FGRenderTaskBase;

template <typename TResourceDesc, typename TResource>
TResource* AcquireResource(AFrameGraph& FrameGraph, TResourceDesc const& ResourceDesc);

template <typename TResource>
void ReleaseResource(AFrameGraph& FrameGraph, TResource* Resource);

enum FG_RESOURCE_ACCESS
{
    FG_RESOURCE_ACCESS_READ,
    FG_RESOURCE_ACCESS_WRITE,
    FG_RESOURCE_ACCESS_READ_WRITE
};

class FGResourceProxyBase
{
public:
    explicit FGResourceProxyBase(std::size_t ResourceId, const char* Name, FGRenderTaskBase* RenderTask) :
        Id(ResourceId), Name(Name), Creator(RenderTask), ResourceRefs(0), bCaptured(false)
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

protected:
    friend class AFrameGraph;
    friend class FGRenderTaskBase;

    virtual void Acquire(AFrameGraph& FrameGraph) = 0;
    virtual void Release(AFrameGraph& FrameGraph) = 0;

    const std::size_t                   Id;
    const char*                         Name;
    FGRenderTaskBase const*             Creator;
    TPodVector<FGRenderTaskBase const*> Readers;
    TPodVector<FGRenderTaskBase const*> Writers;
    int                                 ResourceRefs;
    bool                                bCaptured;
};

template <typename TResourceDesc, typename TResource>
class FGResourceProxy : public FGResourceProxyBase
{
public:
    using ResourceDesc = TResourceDesc;
    using ResourceType = TResource;

    // Construct internal (transient) resource
    explicit FGResourceProxy(std::size_t ResourceId, const char* Name, FGRenderTaskBase* RenderTask, TResourceDesc const& ResourceDesc) :
        FGResourceProxyBase(ResourceId, Name, RenderTask), Desc(ResourceDesc), pResource(nullptr)
    {}

    // Construct external resource
    explicit FGResourceProxy(std::size_t ResourceId, const char* Name, TResource* InResource) :
        FGResourceProxyBase(ResourceId, Name, nullptr), Desc(InResource ? InResource->GetDesc() : TResourceDesc()), pResource(InResource)
    {
        //AN_ASSERT(InResource);
    }

    FGResourceProxy(FGResourceProxy const&) = delete;
    FGResourceProxy(FGResourceProxy&&)      = default;
    ~FGResourceProxy()                      = default;

    FGResourceProxy& operator=(FGResourceProxy const&) = delete;
    FGResourceProxy& operator=(FGResourceProxy&&) = default;

    ResourceType* Actual() const
    {
        return pResource;
    }

    TResourceDesc const& GetResourceDesc() const
    {
        return Desc;
    }

protected:
    void Acquire(AFrameGraph& FrameGraph) override
    {
        if (IsTransient())
        {
            AN_ASSERT(!pResource);

            //GLogger.Printf( "--- Acquire %s ---\n", GetName() );
            pResource = AcquireResource<ResourceDesc, ResourceType>(FrameGraph, Desc);
        }
    }

    void Release(AFrameGraph& FrameGraph) override
    {
        if (IsTransient() && pResource)
        {
            //GLogger.Printf( "Release %s\n", GetName() );
            ReleaseResource<ResourceType>(FrameGraph, pResource);
        }
    }

private:
    TResourceDesc Desc;
    TResource*    pResource;
};

//using FGTextureProxy = FGResourceProxy<STextureDesc, ITexture>;

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

    //// Construct texture view
    //explicit FGTextureProxy(std::size_t ResourceId, const char* Name, ITextureView* InResourceView) :
    //    Super(ResourceId, Name, InResourceView->GetTexture()), pTextureView(InResourceView)
    //{}

    //ITextureView* pTextureView{};
};

//using AFrameGraphTextureView = FGResourceProxy<STextureDesc, ITextureView>;
using FGBufferViewProxy = FGResourceProxy<SBufferViewDesc, IBufferView>;

} // namespace RenderCore
