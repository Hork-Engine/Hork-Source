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

#include "FrameGraph.h"

namespace RenderCore
{

template <>
ITexture* AcquireResource(AFrameGraph& FrameGraph, STextureDesc const& ResourceDesc)
{
    return FrameGraph.GetRenderTargetCache()->Acquire(ResourceDesc);
}

template <>
void ReleaseResource(AFrameGraph& FrameGraph, ITexture* pResource)
{
    FrameGraph.GetRenderTargetCache()->Release(pResource);
}

template <>
ITextureView* AcquireResource(AFrameGraph& pFrameGraph, STextureViewDesc const& ResourceDesc)
{
    AN_ASSERT(0);
    return nullptr;
}

template <>
void ReleaseResource(AFrameGraph& FrameGraph, ITextureView* pResource)
{
    AN_ASSERT(0);
}

template <>
IBufferView* AcquireResource(AFrameGraph& pFrameGraph, SBufferViewDesc const& ResourceDesc)
{
    AN_ASSERT(0);
    return nullptr;
}

template <>
void ReleaseResource(AFrameGraph& FrameGraph, IBufferView* pResource)
{
    AN_ASSERT(0);
}

void AFrameGraph::Build()
{
    AN_ASSERT(CapturedResources.IsEmpty());

    //pFramebufferCache->CleanupOutdatedFramebuffers();

    RegisterResources();

    for (StdUniquePtr<FGRenderTaskBase>& task : RenderTasks)
    {
        task->ResourceRefs = task->ProducedResources.Size() + task->WriteResources.Size() + task->ReadWriteResources.Size();
    }

    for (FGResourceProxyBase* resource : Resources)
    {
        resource->ResourceRefs = resource->Readers.Size();

        if (resource->IsCaptured())
        {
            CapturedResources.Append(resource);
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

    while (UnreferencedResources.Pop(&unreferencedResource))
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

    for (StdUniquePtr<FGRenderTaskBase>& task : RenderTasks)
    {
        if (task->ResourceRefs == 0 && !task->bCulled)
        {
            continue;
        }

        int firstAcquiredResource = AcquiredResources.Size();
        int firstReleasedResource = ReleasedResources.Size();

        for (auto& resource : task->ProducedResources)
        {
            AcquiredResources.Append(const_cast<FGResourceProxyBase*>(resource.get()));
            if (resource->Readers.IsEmpty() && resource->Writers.IsEmpty() && !resource->IsCaptured())
            {
                ReleasedResources.Append(const_cast<FGResourceProxyBase*>(resource.get()));
            }
        }

        ResourcesRW = task->ReadResources;
        ResourcesRW.Append(task->WriteResources);
        ResourcesRW.Append(task->ReadWriteResources);

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
                    [&resource](StdUniquePtr<FGRenderTaskBase> const& it)
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
                    [&resource](StdUniquePtr<FGRenderTaskBase> const& it)
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
                ReleasedResources.Append(const_cast<FGResourceProxyBase*>(resource));
            }
        }

        int numAcquiredResources = AcquiredResources.Size() - firstAcquiredResource;
        int numReleasedResources = ReleasedResources.Size() - firstReleasedResource;

        STimelineStep& step = Timeline.Append();

        step.RenderTask            = task.get();
        step.FirstAcquiredResource = firstAcquiredResource;
        step.NumAcquiredResources  = numAcquiredResources;
        step.FirstReleasedResource = firstReleasedResource;
        step.NumReleasedResources  = numReleasedResources;

        // Resolve pointers
        for (int i = 0; i < numAcquiredResources; i++)
        {
            FGResourceProxyBase* resource = AcquiredResources[firstAcquiredResource + i];
            resource->Acquire(*this);
        }
        // Initialize task
//        task->Create(*this);
        // Resolve pointers
        for (int i = 0; i < numReleasedResources; i++)
        {
            FGResourceProxyBase* resource = ReleasedResources[firstReleasedResource + i];
            resource->Release(*this);
        }
    }
}

//void AFrameGraph::Execute(IImmediateContext* Rcmd)
//{
//    for (STimelineStep& step : Timeline)
//    {
//        //for ( AFrameGraphResourceBase * resource : step.AcquiredResources ) {
//        //    resource->Acquire( *this );
//        //}
//
//        step.RenderTask->Execute(*this, Rcmd);
//
//        //for ( AFrameGraphResourceBase * resource : step.ReleasedResources ) {
//        //    resource->Release( *this );
//        //}
//    }
//}

void AFrameGraph::Debug()
{
    GLogger.Printf("---------- FrameGraph ----------\n");
    for (STimelineStep& step : Timeline)
    {
        for (int i = 0; i < step.NumAcquiredResources; i++)
        {
            FGResourceProxyBase* resource = AcquiredResources[step.FirstAcquiredResource + i];
            GLogger.Printf("Acquire %s\n", resource->GetName());
        }

        GLogger.Printf("Execute %s\n", step.RenderTask->GetName());

        for (int i = 0; i < step.NumReleasedResources; i++)
        {
            FGResourceProxyBase* resource = ReleasedResources[step.FirstReleasedResource + i];
            GLogger.Printf("Release %s\n", resource->GetName());
        }
    }
    GLogger.Printf("--------------------------------\n");
}

void AFrameGraph::ExportGraphviz(const char* FileName)
{
    AFileStream f;

    if (!f.OpenWrite(FileName))
    {
        return;
    }

    f.Printf("digraph framegraph \n{\n");
    f.Printf("rankdir = LR\n");
    f.Printf("bgcolor = black\n\n");
    f.Printf("node [shape=rectangle, fontname=\"helvetica\", fontsize=12]\n\n");

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
        f.Printf("\"%s\" [label=\"%s\\nRefs: %d\\nID: %d\", style=filled, fillcolor=%s]\n",
                 resource->GetName(), resource->GetName(), resource->ResourceRefs, resource->GetId(),
                 color);
    }
    f.Printf("\n");

    for (auto& task : RenderTasks)
    {
        f.Printf("\"%s\" [label=\"%s\\nRefs: %d\", style=filled, fillcolor=darkorange]\n",
                 task->GetName(), task->GetName(), task->ResourceRefs);

        if (!task->ProducedResources.IsEmpty())
        {
            f.Printf("\"%s\" -> { ", task->GetName());
            for (auto& resource : task->ProducedResources)
            {
                f.Printf("\"%s\" ", resource->GetName());
            }
            f.Printf("} [color=seagreen]\n");
        }

        if (!task->WriteResources.IsEmpty())
        {
            f.Printf("\"%s\" -> { ", task->GetName());
            for (auto& resource : task->WriteResources)
            {
                f.Printf("\"%s\" ", resource->GetName());
            }
            f.Printf("} [color=gold]\n");
        }
    }
    f.Printf("\n");

    for (auto& resource : Resources)
    {
        f.Printf("\"%s\" -> { ", resource->GetName());
        for (auto& task : resource->Readers)
        {
            f.Printf("\"%s\" ", task->GetName());
        }
        f.Printf("} [color=skyblue]\n");
    }
    f.Printf("}");
}

} // namespace RenderCore
