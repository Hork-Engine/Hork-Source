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

#include "FGRenderPass.h"
#include "FGCustomTask.h"
#include "FGRenderTargetCache.h"

#include <Hork/Core/String.h>
#include <Hork/Core/Containers/Stack.h>

#include <Hork/RenderCore/Device.h>

// TODO:
// 1. Optimize. Very slow framegraph rebuilding in debug mode.
// 2. Destroy unused framebuffers and textures (after some time?)

HK_NAMESPACE_BEGIN

namespace RenderCore
{

class FrameGraph : public RefCounted
{
public:
    FrameGraph(IDevice* pDevice, FGRenderTargetCache* pRenderTargetCache = nullptr) :
        pDevice(pDevice),
        pRenderTargetCache(pRenderTargetCache ? pRenderTargetCache : MakeRef<FGRenderTargetCache>(pDevice))
    {
    }

    ~FrameGraph()
    {
        ReleaseCapturedResources();
    }

    IDevice* GetDevice() { return pDevice; }

    void Clear()
    {
        ReleaseCapturedResources();
        CapturedResources.Clear();
        for (auto& resource : ExternalResources)
            resource->pDeviceObject->RemoveRef();
        ExternalResources.Clear();
        Resources.Clear();
        RenderTasks.Clear();
        IdGenerator = 0;
    }

    template <typename TTask>
    TTask& AddTask(const char* Name)
    {
        RenderTasks.EmplaceBack(std::make_unique<TTask>(this, Name));
        return *static_cast<TTask*>(RenderTasks.Last().get());
    }

    template <typename T>
    T* AddExternalResource(const char* Name, typename T::ResourceType* Resource)
    {
        Resource->AddRef();
        ExternalResources.EmplaceBack(std::make_unique<T>(GenerateResourceId(), Name, Resource));
        return static_cast<T*>(ExternalResources.Last().get());
    }

    void Build();

    void Debug();

    void ExportGraphviz(StringView FileName);

    std::size_t GenerateResourceId() const
    {
        return IdGenerator++;
    }

    FGRenderTargetCache* GetRenderTargetCache()
    {
        return pRenderTargetCache;
    }

    struct TimelineStep
    {
        FGRenderTaskBase* RenderTask;
        int               FirstAcquiredResource;
        int               NumAcquiredResources;
        int               FirstReleasedResource;
        int               NumReleasedResources;
    };

    Vector<TimelineStep> const& GetTimeline() const
    {
        return Timeline;
    }

    Vector<FGResourceProxyBase*> const& GetAcquiredResources() const
    {
        return AcquiredResources;
    }

    Vector<FGResourceProxyBase*> const& GetReleasedResources() const
    {
        return ReleasedResources;
    }

private:
    void RegisterResources()
    {
        Resources.Clear();

        for (std::unique_ptr<FGRenderTaskBase>& task : RenderTasks)
        {
            for (auto& resourcePtr : task->GetProducedResources())
            {
                Resources.Add(resourcePtr.get());
            }
        }

        for (auto& resourcePtr : ExternalResources)
        {
            Resources.Add(resourcePtr.get());
        }
    }

    void ReleaseCapturedResources();

    Ref<IDevice>             pDevice;
    Ref<FGRenderTargetCache> pRenderTargetCache;

    Vector<std::unique_ptr<FGRenderTaskBase>>    RenderTasks;
    Vector<std::unique_ptr<FGResourceProxyBase>> ExternalResources;
    Vector<FGResourceProxyBase*>              Resources; // all resources
    Vector<FGResourceProxyBase*>              CapturedResources;

    Vector<TimelineStep>        Timeline;
    Vector<FGResourceProxyBase*> AcquiredResources, ReleasedResources;

    // Temporary data. Used for building
    Stack<FGResourceProxyBase*>  UnreferencedResources;
    Vector<FGResourceProxyBase*> ResourcesRW;

    mutable std::size_t IdGenerator = 0;
};

HK_FORCEINLINE std::size_t FG_GenerateResourceId(FrameGraph* pFrameGraph)
{
    return pFrameGraph->GenerateResourceId();
}

} // namespace RenderCore

HK_NAMESPACE_END
