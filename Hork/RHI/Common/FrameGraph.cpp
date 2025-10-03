/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "FrameGraph.h"
#include <Hork/Core/IO.h>

HK_NAMESPACE_BEGIN

namespace RHI
{

void FrameGraph::Build()
{
    HK_ASSERT(CapturedResources.IsEmpty());

    //pFramebufferCache->CleanupOutdatedFramebuffers();

    RegisterResources();

    for (std::unique_ptr<FGRenderTaskBase>& task : RenderTasks)
    {
        task->ResourceRefs = task->ProducedResources.Size() + task->WriteResources.Size() + task->ReadWriteResources.Size();
    }

    for (FGResourceProxyBase* resource : Resources)
    {
        resource->ResourceRefs = resource->Readers.Size();

        if (resource->IsCaptured())
        {
            CapturedResources.Add(resource);
        }
    }

    UnreferencedResources.Clear();

    for (FGResourceProxyBase* resource : Resources)
    {
        if (resource->ResourceRefs == 0 && resource->IsTransient() && !resource->IsCaptured())
        {
            UnreferencedResources.Push(resource);
        }
    }

    FGResourceProxyBase* unreferencedResource;

    while (UnreferencedResources.Pop(unreferencedResource))
    {

        FGRenderTaskBase* creator = const_cast<FGRenderTaskBase*>(unreferencedResource->Creator);
        if (creator->ResourceRefs > 0)
        {
            creator->ResourceRefs--;
        }

        if (creator->ResourceRefs == 0 && !creator->bCulled)
        {
            for (FGResourceProxyBase* readResource : creator->ReadResources)
            {
                if (readResource->ResourceRefs > 0)
                    readResource->ResourceRefs--;
                if (readResource->ResourceRefs == 0 && readResource->IsTransient())
                    UnreferencedResources.Push(readResource);
            }
        }

        for (FGRenderTaskBase const* c_writer : unreferencedResource->Writers)
        {
            FGRenderTaskBase* writer = const_cast<FGRenderTaskBase*>(c_writer);
            if (writer->ResourceRefs > 0)
            {
                writer->ResourceRefs--;
            }
            if (writer->ResourceRefs == 0 && !writer->bCulled)
            {
                for (FGResourceProxyBase* readResource : writer->ReadResources)
                {
                    if (readResource->ResourceRefs > 0)
                        readResource->ResourceRefs--;
                    if (readResource->ResourceRefs == 0 && readResource->IsTransient())
                        UnreferencedResources.Push(readResource);
                }
            }
        }
    }

    Timeline.Clear();

    AcquiredResources.Clear();
    ReleasedResources.Clear();

    //resourcesRW.ReserveInvalidate( 1024 );

    for (std::unique_ptr<FGRenderTaskBase>& task : RenderTasks)
    {
        if (task->ResourceRefs == 0 && !task->bCulled)
        {
            continue;
        }

        int firstAcquiredResource = AcquiredResources.Size();
        int firstReleasedResource = ReleasedResources.Size();

        for (auto& resource : task->ProducedResources)
        {
            AcquiredResources.Add(const_cast<FGResourceProxyBase*>(resource.get()));
            if (resource->Readers.IsEmpty() && resource->Writers.IsEmpty() && !resource->IsCaptured())
            {
                ReleasedResources.Add(const_cast<FGResourceProxyBase*>(resource.get()));
            }
        }

        ResourcesRW.Clear();
        ResourcesRW.Add(task->ReadResources);
        ResourcesRW.Add(task->WriteResources);
        ResourcesRW.Add(task->ReadWriteResources);

        for (FGResourceProxyBase* resource : ResourcesRW)
        {
            if (!resource->IsTransient() || resource->IsCaptured())
            {
                continue;
            }

            bool        bValid    = false;
            std::size_t lastIndex = 0;
            if (!resource->Readers.IsEmpty())
            {
                auto lastReader = std::find_if(
                    RenderTasks.begin(),
                    RenderTasks.end(),
                    [&resource](std::unique_ptr<FGRenderTaskBase> const& it)
                    {
                        return it.get() == resource->Readers.Last();
                    });
                if (lastReader != RenderTasks.end())
                {
                    bValid    = true;
                    lastIndex = std::distance(RenderTasks.begin(), lastReader);
                }
            }
            if (!resource->Writers.IsEmpty())
            {
                auto lastWriter = std::find_if(
                    RenderTasks.begin(),
                    RenderTasks.end(),
                    [&resource](std::unique_ptr<FGRenderTaskBase> const& it)
                    {
                        return it.get() == resource->Writers.Last();
                    });
                if (lastWriter != RenderTasks.end())
                {
                    bValid    = true;
                    lastIndex = std::max(lastIndex, std::size_t(std::distance(RenderTasks.begin(), lastWriter)));
                }
            }

            if (bValid && RenderTasks[lastIndex] == task)
            {
                ReleasedResources.Add(const_cast<FGResourceProxyBase*>(resource));
            }
        }

        int numAcquiredResources = AcquiredResources.Size() - firstAcquiredResource;
        int numReleasedResources = ReleasedResources.Size() - firstReleasedResource;

        TimelineStep& step = Timeline.Add();

        step.RenderTask            = task.get();
        step.FirstAcquiredResource = firstAcquiredResource;
        step.NumAcquiredResources  = numAcquiredResources;
        step.FirstReleasedResource = firstReleasedResource;
        step.NumReleasedResources  = numReleasedResources;
    }
}

void FrameGraph::Debug()
{
    LOG("---------- FrameGraph ----------\n");
    for (TimelineStep& step : Timeline)
    {
        for (int i = 0; i < step.NumAcquiredResources; i++)
        {
            FGResourceProxyBase* resource = AcquiredResources[step.FirstAcquiredResource + i];
            LOG("Acquire {}\n", resource->GetName());
        }

        LOG("Execute {}\n", step.RenderTask->GetName());

        for (int i = 0; i < step.NumReleasedResources; i++)
        {
            FGResourceProxyBase* resource = ReleasedResources[step.FirstReleasedResource + i];
            LOG("Release {}\n", resource->GetName());
        }
    }
    LOG("--------------------------------\n");
}

void FrameGraph::ExportGraphviz(StringView FileName)
{
    File f = File::sOpenWrite(FileName);
    if (!f)
    {
        return;
    }

    f.FormattedPrint("digraph framegraph \n{{\n");
    f.FormattedPrint("rankdir = LR\n");
    f.FormattedPrint("bgcolor = black\n\n");
    f.FormattedPrint("node [shape=rectangle, fontname=\"helvetica\", fontsize=12]\n\n");

    for (auto& resource : Resources)
    {
        const char* color;
        if (resource->IsCaptured())
        {
            color = "yellow";
        }
        else if (resource->IsTransient())
        {
            color = "skyblue";
        }
        else
        {
            color = "steelblue";
        }
        f.FormattedPrint("\"{}\" [label=\"{}\\nRefs: {}\\nID: {}\", style=filled, fillcolor={}]\n",
                 resource->GetName(), resource->GetName(), resource->ResourceRefs, resource->GetId(),
                 color);
    }
    f.FormattedPrint("\n");

    for (auto& task : RenderTasks)
    {
        f.FormattedPrint("\"{}\" [label=\"{}\\nRefs: {}\", style=filled, fillcolor=darkorange]\n",
                 task->GetName(), task->GetName(), task->ResourceRefs);

        if (!task->ProducedResources.IsEmpty())
        {
            f.FormattedPrint("\"{}\" -> {{ ", task->GetName());
            for (auto& resource : task->ProducedResources)
            {
                f.FormattedPrint("\"{}\" ", resource->GetName());
            }
            f.FormattedPrint("}} [color=seagreen]\n");
        }

        if (!task->WriteResources.IsEmpty())
        {
            f.FormattedPrint("\"{}\" -> {{ ", task->GetName());
            for (auto& resource : task->WriteResources)
            {
                f.FormattedPrint("\"{}\" ", resource->GetName());
            }
            f.FormattedPrint("}} [color=gold]\n");
        }
    }
    f.FormattedPrint("\n");

    for (auto& resource : Resources)
    {
        f.FormattedPrint("\"{}\" -> {{ ", resource->GetName());
        for (auto& task : resource->Readers)
        {
            f.FormattedPrint("\"{}\" ", task->GetName());
        }
        f.FormattedPrint("}} [color=skyblue]\n");
    }
    f.FormattedPrint("}}");
}

void FrameGraph::ReleaseCapturedResources()
{
    for (FGResourceProxyBase* resourceProxy : CapturedResources)
    {
        switch (resourceProxy->GetProxyType())
        {
            case DEVICE_OBJECT_TYPE_TEXTURE:
                pRenderTargetCache->Release(static_cast<ITexture*>(resourceProxy->pDeviceObject));
                break;
            default:
                HK_ASSERT(0);
        }
    }
}

} // namespace RHI

HK_NAMESPACE_END
