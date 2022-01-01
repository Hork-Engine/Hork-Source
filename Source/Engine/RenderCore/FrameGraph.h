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

#include "FGRenderPass.h"
#include "FGCustomTask.h"
#include "FGRenderTargetCache.h"

#include <Core/Public/String.h>
#include <Containers/Public/PodStack.h>

#include <RenderCore/Device.h>

// TODO:
// 1. Optimize. Very slow framegraph rebuilding in debug mode.
// 2. Destroy unused framebuffers and textures (after some time?)

namespace RenderCore
{

class AFrameGraph : public ARefCounted
{
public:
    AFrameGraph(IDevice* pDevice, FGRenderTargetCache* pRenderTargetCache = nullptr) :
        pDevice(pDevice),
        pRenderTargetCache(pRenderTargetCache ? pRenderTargetCache : MakeRef<FGRenderTargetCache>(pDevice))
    {
    }

    ~AFrameGraph()
    {
        ReleaseCapturedResources();
    }

    IDevice* GetDevice() { return pDevice; }

    void Clear()
    {
        ReleaseCapturedResources();
        CapturedResources.Clear();
        ExternalResources.Clear();
        Resources.Clear();
        RenderTasks.Clear();
        IdGenerator = 0;
    }

    template <typename TTask>
    TTask& AddTask(const char* Name)
    {
        RenderTasks.emplace_back(std::make_unique<TTask>(this, Name));
        return *static_cast<TTask*>(RenderTasks.back().get());
    }

    template <typename T>
    T* AddExternalResource(const char* Name, typename T::ResourceType* Resource)
    {
        ExternalResources.emplace_back(std::make_unique<T>(GenerateResourceId(), Name, Resource));
        return static_cast<T*>(ExternalResources.back().get());
    }

    void Build();

    void Debug();

    void ExportGraphviz(const char* FileName);

    std::size_t GenerateResourceId() const
    {
        return IdGenerator++;
    }

    FGRenderTargetCache* GetRenderTargetCache()
    {
        return pRenderTargetCache;
    }

    struct STimelineStep
    {
        FGRenderTaskBase* RenderTask;
        int               FirstAcquiredResource;
        int               NumAcquiredResources;
        int               FirstReleasedResource;
        int               NumReleasedResources;
    };

    TPodVector<STimelineStep> const& GetTimeline() const
    {
        return Timeline;
    }

    TPodVector<FGResourceProxyBase*> const& GetAcquiredResources() const
    {
        return AcquiredResources;
    }

    TPodVector<FGResourceProxyBase*> const& GetReleasedResources() const
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
                Resources.Append(resourcePtr.get());
            }
        }

        for (auto& resourcePtr : ExternalResources)
        {
            Resources.Append(resourcePtr.get());
        }
    }

    void ReleaseCapturedResources();

    TRef<IDevice>             pDevice;
    TRef<FGRenderTargetCache> pRenderTargetCache;

    TStdVector<std::unique_ptr<FGRenderTaskBase>>    RenderTasks;
    TStdVector<std::unique_ptr<FGResourceProxyBase>> ExternalResources;
    TPodVector<FGResourceProxyBase*>              Resources; // all resources
    TPodVector<FGResourceProxyBase*>              CapturedResources;

    TPodVector<STimelineStep>        Timeline;
    TPodVector<FGResourceProxyBase*> AcquiredResources, ReleasedResources;

    // Temporary data. Used for building
    TPodStack<FGResourceProxyBase*>  UnreferencedResources;
    TPodVector<FGResourceProxyBase*> ResourcesRW;

    mutable std::size_t IdGenerator = 0;
};

AN_FORCEINLINE std::size_t FG_GenerateResourceId(AFrameGraph* pFrameGraph)
{
    return pFrameGraph->GenerateResourceId();
}

} // namespace RenderCore
