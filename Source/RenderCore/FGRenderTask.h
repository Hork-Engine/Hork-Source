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

#pragma once

#include "FGResource.h"
#include <Containers/Vector.h>

namespace RenderCore
{

enum FG_RENDER_TASK_PROXY_TYPE : uint8_t
{
    FG_RENDER_TASK_PROXY_TYPE_UNKNOWN,
    FG_RENDER_TASK_PROXY_TYPE_RENDER_PASS,
    FG_RENDER_TASK_PROXY_TYPE_CUSTOM
};

class AFrameGraph;

class FGRenderTaskBase
{
public:
    explicit FGRenderTaskBase(AFrameGraph* pFrameGraph, const char* Name, FG_RENDER_TASK_PROXY_TYPE ProxyType) :
        pFrameGraph(pFrameGraph), Name(Name), ResourceRefs(0), bCulled(false), ProxyType(ProxyType)
    {
    }

    virtual ~FGRenderTaskBase() = default;

    FGRenderTaskBase(FGRenderTaskBase const&) = delete;
    FGRenderTaskBase(FGRenderTaskBase&&)      = default;

    FGRenderTaskBase& operator=(FGRenderTaskBase const&) = delete;
    FGRenderTaskBase& operator=(FGRenderTaskBase&&) = default;

    const char* GetName() const { return Name; }

    TVector<std::unique_ptr<FGResourceProxyBase>> const& GetProducedResources() const
    {
        return ProducedResources;
    }

    template <typename TResourceProxy>
    void AddResource(TResourceProxy* pResource, FG_RESOURCE_ACCESS Access)
    {
        switch (Access)
        {
            case FG_RESOURCE_ACCESS_READ:
                pResource->Readers.Add(this);
                ReadResources.Add(pResource);
                break;
            case FG_RESOURCE_ACCESS_WRITE:
                pResource->Writers.Add(this);
                WriteResources.Add(pResource);
                break;
            case FG_RESOURCE_ACCESS_READ_WRITE:
                pResource->Readers.Add(this);
                pResource->Writers.Add(this);
                ReadWriteResources.Add(pResource);
                break;
        }
    }

    FG_RENDER_TASK_PROXY_TYPE GetProxyType() const
    {
        return ProxyType;
    }

protected:
    friend class AFrameGraph;

    AFrameGraph*                                  pFrameGraph;
    const char*                                   Name;
    TVector<std::unique_ptr<FGResourceProxyBase>> ProducedResources;
    TPodVector<FGResourceProxyBase*>              ReadResources;
    TPodVector<FGResourceProxyBase*>              WriteResources;
    TPodVector<FGResourceProxyBase*>              ReadWriteResources;
    int                                           ResourceRefs;
    bool                                          bCulled;
    FG_RENDER_TASK_PROXY_TYPE                     ProxyType;
};

std::size_t FG_GenerateResourceId(AFrameGraph* pFrameGraph);

template <typename TRenderTaskClass>
class FGRenderTask : public FGRenderTaskBase
{
public:
    explicit FGRenderTask(AFrameGraph* pFrameGraph, const char* Name, FG_RENDER_TASK_PROXY_TYPE ProxyType) :
        FGRenderTaskBase(pFrameGraph, Name, ProxyType)
    {
    }

    template <typename TResourceProxy, typename TResourceDesc>
    TRenderTaskClass& AddNewResource(const char* Name, TResourceDesc const& ResourceDesc, TResourceProxy** ppResource = nullptr)
    {
        static_assert(std::is_same<typename TResourceProxy::ResourceDesc, TResourceDesc>::value, "Invalid TResourceDesc");

        ProducedResources.EmplaceBack(std::make_unique<TResourceProxy>(FG_GenerateResourceId(pFrameGraph), Name, this, ResourceDesc));

        if (ppResource)
        {
            *ppResource = static_cast<TResourceProxy*>(ProducedResources.Last().get());
        }
        return static_cast<TRenderTaskClass&>(*this);
    }

    template <typename TResourceProxy>
    TRenderTaskClass& AddResource(TResourceProxy* pResource, FG_RESOURCE_ACCESS Access)
    {
        FGRenderTaskBase::AddResource(pResource, Access);
        return static_cast<TRenderTaskClass&>(*this);
    }
};

} // namespace RenderCore
