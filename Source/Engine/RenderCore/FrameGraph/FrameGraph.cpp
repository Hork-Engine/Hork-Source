/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <Core/Public/IO.h>
#include <Core/Public/Logger.h>

#include <Runtime/Public/RuntimeVariable.h>

using namespace RenderCore;

ARuntimeVariable RVFrameGraphDebug( _CTS( "FrameGraphDebug" ), _CTS( "0" ) );

template<>
RenderCore::ITexture * RealizeResource( AFrameGraph & InFrameGraph, RenderCore::STextureCreateInfo const & Info )
{
    // Find free appropriate texture
    for ( auto it = InFrameGraph.FreeTextures.Begin() ; it != InFrameGraph.FreeTextures.End() ; it++ ) {
        RenderCore::ITexture * tex = *it;

        if ( tex->GetType() == Info.Type
             && tex->GetFormat() == Info.Format
             && tex->GetResoulution() == Info.Resolution
             && tex->GetNumSamples() == Info.Multisample.NumSamples
             && tex->FixedSampleLocations() == Info.Multisample.bFixedSampleLocations
             && tex->GetSwizzle() == Info.Swizzle
             && tex->GetNumLods() == Info.NumLods )
        {
            //GLogger.Printf( "Reusing existing texture\n" );
            InFrameGraph.FreeTextures.Erase( it );
            return tex;
        }
    }

    // Create new texture
    GLogger.Printf( "Create new texture ( in use %d, free %d )\n", InFrameGraph.Textures.Size()+1, InFrameGraph.FreeTextures.Size() );
    TRef< RenderCore::ITexture > texture;
    InFrameGraph.GetDevice()->CreateTexture( Info, &texture );
    InFrameGraph.Textures.Append( texture );
    return texture;
}

template<>
void DerealizeResource( AFrameGraph & InFrameGraph, RenderCore::ITexture * InResource )
{
    InFrameGraph.FreeTextures.Append( InResource );
}

template<>
RenderCore::IBufferView * RealizeResource( AFrameGraph & InFrameGraph, RenderCore::SBufferViewCreateInfo const & Info )
{
    AN_ASSERT( 0 );
    return nullptr;
}

template<>
void DerealizeResource( AFrameGraph & InFrameGraph, RenderCore::IBufferView * InResource )
{
    AN_ASSERT( 0 );
}

void AFrameGraph::Build()
{
    AN_ASSERT( CapturedResources.IsEmpty() );

    // Remove outdated framebuffers
    for ( int i = 0 ; i < FramebufferCache.Size() ; ) {
        if ( FramebufferCache[i]->IsAttachmentsOutdated() ) {
            FramebufferCache.erase( FramebufferCache.begin() + i );
            FramebufferHash.RemoveIndex( FramebufferHashes[i], i );
            FramebufferHashes.Erase( FramebufferHashes.begin() + i );
            continue;
        }
        i++;
    }

    RegisterResources();

    for ( std::unique_ptr< ARenderTask > & task : RenderTasks ) {
        task->ResourceRefs = task->ProducedResources.Size() + task->WriteResources.Size()
            + task->ReadWriteResources.Size();
    }

    for ( AFrameGraphResourceProxy * resource : Resources ) {
        resource->ResourceRefs = resource->Readers.Size();

        if ( resource->IsCaptured() ) {
            CapturedResources.Append( resource );
        }
    }

    UnreferencedResources.Clear();

    for ( AFrameGraphResourceProxy * resource : Resources ) {
        if ( resource->ResourceRefs == 0 && resource->IsTransient() && !resource->IsCaptured() ) {
            UnreferencedResources.Push( resource );
        }
    }

    AFrameGraphResourceProxy * unreferencedResource;

    while ( UnreferencedResources.Pop( &unreferencedResource ) ) {

        ARenderTask * creator = const_cast< ARenderTask * >( unreferencedResource->Creator );
        if ( creator->ResourceRefs > 0 ) {
            creator->ResourceRefs--;
        }

        if ( creator->ResourceRefs == 0 && !creator->bCulled ) {
            for ( AFrameGraphResourceProxy * readResource : creator->ReadResources ) {
                if ( readResource->ResourceRefs > 0 )
                    readResource->ResourceRefs--;
                if ( readResource->ResourceRefs == 0 && readResource->IsTransient() )
                    UnreferencedResources.Push( readResource );
            }
        }

        for ( ARenderTask const * c_writer : unreferencedResource->Writers ) {
            ARenderTask * writer = const_cast< ARenderTask * >( c_writer );
            if ( writer->ResourceRefs > 0 ) {
                writer->ResourceRefs--;
            }
            if ( writer->ResourceRefs == 0 && !writer->bCulled ) {
                for ( AFrameGraphResourceProxy * readResource : writer->ReadResources ) {
                    if ( readResource->ResourceRefs > 0 )
                        readResource->ResourceRefs--;
                    if ( readResource->ResourceRefs == 0 && readResource->IsTransient() )
                        UnreferencedResources.Push( readResource );
                }
            }
        }
    }

    Timeline.Clear();

    RealizedResources.Clear();
    DerealizedResources.Clear();

    //resourcesRW.ReserveInvalidate( 1024 );

    for ( std::unique_ptr< ARenderTask > & task : RenderTasks ) {
        if ( task->ResourceRefs == 0 && !task->bCulled ) {
            continue;
        }

        int firstRealizedResource = RealizedResources.Size();
        int firstDerealizedResource = DerealizedResources.Size();

        for ( auto & resource : task->ProducedResources ) {
            RealizedResources.Append( const_cast< AFrameGraphResourceProxy * >( resource.get() ) );
            if ( resource->Readers.IsEmpty() && resource->Writers.IsEmpty() && !resource->IsCaptured() ) {
                DerealizedResources.Append( const_cast<AFrameGraphResourceProxy*>(resource.get()) );
            }
        }

        ResourcesRW = task->ReadResources;
        ResourcesRW.Append( task->WriteResources );
        ResourcesRW.Append( task->ReadWriteResources );

        for ( AFrameGraphResourceProxy * resource : ResourcesRW ) {
            if ( !resource->IsTransient() || resource->IsCaptured() ) {
                continue;
            }

            bool bValid = false;
            std::size_t lastIndex = 0;
            if ( !resource->Readers.IsEmpty() )
            {
                auto lastReader = std::find_if(
                    RenderTasks.begin(),
                    RenderTasks.end  (),
                    [&resource]( std::unique_ptr< ARenderTask > const & iteratee )
                {
                    return iteratee.get() == resource->Readers.Last();
                } );
                if ( lastReader != RenderTasks.end() )
                {
                    bValid = true;
                    lastIndex = std::distance( RenderTasks.begin(), lastReader );
                }
            }
            if ( !resource->Writers.IsEmpty() )
            {
                auto lastWriter = std::find_if(
                    RenderTasks.begin(),
                    RenderTasks.end  (),
                    [&resource]( std::unique_ptr< ARenderTask > const & iteratee )
                {
                    return iteratee.get() == resource->Writers.Last();
                } );
                if ( lastWriter != RenderTasks.end() )
                {
                    bValid = true;
                    lastIndex = std::max( lastIndex, std::size_t( std::distance( RenderTasks.begin(), lastWriter ) ) );
                }
            }

            if ( bValid && RenderTasks[lastIndex] == task ) {
                DerealizedResources.Append( const_cast< AFrameGraphResourceProxy * >( resource ) );
            }
        }

        int numRealizedResources = RealizedResources.Size() - firstRealizedResource;
        int numDerealizedResources = DerealizedResources.Size() - firstDerealizedResource;

        STimelineStep & step = Timeline.Append();

        step.RenderTask = task.get();
        step.FirstRealizedResource = firstRealizedResource;
        step.NumRealizedResources = numRealizedResources;
        step.FirstDerealizedResource = firstDerealizedResource;
        step.NumDerealizedResources = numDerealizedResources;

        // Resolve pointers
        for ( int i = 0 ; i < numRealizedResources ; i++ ) {
            AFrameGraphResourceProxy * resource = RealizedResources[ firstRealizedResource + i ];
            resource->Realize( *this );
        }
        // Initialize task
        task->Create( *this );
        // Resolve pointers
        for ( int i = 0 ; i < numDerealizedResources ; i++ ) {
            AFrameGraphResourceProxy * resource = DerealizedResources[ firstDerealizedResource + i ];
            resource->Derealize( *this );
        }
    }
}

void AFrameGraph::Execute( RenderCore::IImmediateContext * Rcmd )
{
    for ( STimelineStep & step : Timeline ) {
        //for ( AFrameGraphResourceBase * resource : step.RealizedResources ) {
        //    resource->Realize( *this );
        //}

        step.RenderTask->Execute( *this, Rcmd );

        //for ( AFrameGraphResourceBase * resource : step.DerealizedResources ) {
        //    resource->Derealize( *this );
        //}
    }

    if ( RVFrameGraphDebug ) {
        Debug();
    }
}

void AFrameGraph::Debug()
{
    GLogger.Printf( "---------- FrameGraph ----------\n" );
    for ( STimelineStep & step : Timeline ) {
        for ( int i = 0 ; i < step.NumRealizedResources ; i++ ) {
            AFrameGraphResourceProxy * resource = RealizedResources[ step.FirstRealizedResource + i ];
            GLogger.Printf( "Realize %s\n", resource->GetName() );
        }

        GLogger.Printf( "Execute %s\n", step.RenderTask->GetName() );

        for ( int i = 0 ; i < step.NumDerealizedResources ; i++ ) {
            AFrameGraphResourceProxy * resource = DerealizedResources[ step.FirstDerealizedResource + i ];
            GLogger.Printf( "Derealize %s\n", resource->GetName() );
        }
    }
    GLogger.Printf( "--------------------------------\n" );
}

void AFrameGraph::ExportGraphviz( const char * FileName )
{
    AFileStream f;

    if ( !f.OpenWrite( FileName ) ) {
        return;
    }

    f.Printf( "digraph framegraph \n{\n" );
    f.Printf( "rankdir = LR\n" );
    f.Printf( "bgcolor = black\n\n" );
    f.Printf( "node [shape=rectangle, fontname=\"helvetica\", fontsize=12]\n\n" );

    for ( auto & resource : Resources ) {
        const char * color;
        if ( resource->IsCaptured() ) {
            color = "yellow";
        } else if ( resource->IsTransient() ) {
            color = "skyblue";
        } else {
            color = "steelblue";
        }
        f.Printf( "\"%s\" [label=\"%s\\nRefs: %d\\nID: %d\", style=filled, fillcolor=%s]\n",
                  resource->GetName(), resource->GetName(), resource->ResourceRefs, resource->GetId(),
                  color );
    }
    f.Printf( "\n" );

    for ( auto & render_task : RenderTasks ) {
        f.Printf( "\"%s\" [label=\"%s\\nRefs: %d\", style=filled, fillcolor=darkorange]\n",
                  render_task->GetName(), render_task->GetName(), render_task->ResourceRefs );

        if ( !render_task->ProducedResources.IsEmpty() )
        {
            f.Printf( "\"%s\" -> { ", render_task->GetName() );
            for ( auto & resource : render_task->ProducedResources ) {
                f.Printf( "\"%s\" ", resource->GetName() );
            }
            f.Printf( "} [color=seagreen]\n" );
        }

        if ( !render_task->WriteResources.IsEmpty() )
        {
            f.Printf( "\"%s\" -> { ", render_task->GetName() );
            for ( auto & resource : render_task->WriteResources ) {
                f.Printf( "\"%s\" ", resource->GetName() );
            }
            f.Printf( "} [color=gold]\n" );
        }
    }
    f.Printf( "\n" );

    for ( auto & resource : Resources ) {
        f.Printf( "\"%s\" -> { ", resource->GetName() );
        for ( auto& render_task : resource->Readers ) {
            f.Printf( "\"%s\" ", render_task->GetName() );
        }
        f.Printf( "} [color=skyblue]\n" );
    }
    f.Printf( "}" );
}

void ARenderPass::Create( AFrameGraph & FrameGraph )
{
    RenderCore::SRenderPassCreateInfo renderPassCI = {};

    RenderCore::SAttachmentInfo attachmentInfos[MAX_COLOR_ATTACHMENTS];

    AN_ASSERT( ColorAttachments.Size() <= MAX_COLOR_ATTACHMENTS );

    for ( int i = 0 ; i < ColorAttachments.Size() ; i++ ) {
        attachmentInfos[i] = ColorAttachments[i].Info;
    }

    renderPassCI.NumColorAttachments = ColorAttachments.Size();
    renderPassCI.pColorAttachments = attachmentInfos;
    renderPassCI.pDepthStencilAttachment = bHasDepthStencilAttachment ? &DepthStencilAttachment.Info : nullptr;
    renderPassCI.NumSubpasses = Subpasses.Size();

    TPodArray< RenderCore::SSubpassInfo > subpassesTemp;
    subpassesTemp.Resize( renderPassCI.NumSubpasses );

    for ( int i = 0 ; i < renderPassCI.NumSubpasses ; i++ ) {
        subpassesTemp[i].NumColorAttachments = Subpasses[i].Refs.Size();
        subpassesTemp[i].pColorAttachmentRefs = Subpasses[i].Refs.data();
    }
    renderPassCI.pSubpasses = subpassesTemp.ToPtr();

    while ( ClearValues.Size() < renderPassCI.NumColorAttachments ) {
        ClearValues.Append( MakeClearColorValue( 0.0f, 0.0f, 0.0f, 0.0f ) );
    }

    // TODO: cache renderpasses?
    FrameGraph.GetDevice()->CreateRenderPass( renderPassCI, &Handle );

    pFramebuffer = FrameGraph.GetFramebuffer( GetName(),
                                              ColorAttachments,
                                              bHasDepthStencilAttachment ? &DepthStencilAttachment : nullptr );
}

static AN_FORCEINLINE bool CompareAttachment( SFramebufferAttachmentInfo const & a, SFramebufferAttachmentInfo const & b )
{
    return a.pTexture->GetUID() == b.pTexture->GetUID()
            && a.Type == b.Type
            && a.LayerNum == b.LayerNum
            && a.LodNum == b.LodNum;
}

RenderCore::IFramebuffer * AFrameGraph::GetFramebuffer( const char * RenderPassName,
                                                        TStdVector< ARenderPass::STextureAttachment > const & ColorAttachments,
                                                        ARenderPass::STextureAttachment const * DepthStencilAttachment ) {

    RenderCore::SFramebufferCreateInfo framebufferCI = {};
    SFramebufferAttachmentInfo colorAttachments[MAX_COLOR_ATTACHMENTS];
    SFramebufferAttachmentInfo depthStencilAttachment;

    AN_ASSERT( ColorAttachments.Size() <= MAX_COLOR_ATTACHMENTS );

    framebufferCI.NumColorAttachments = ColorAttachments.Size();
    framebufferCI.pColorAttachments = colorAttachments;

    // Make hash
    int hash = 0;
    int n = 0;

    for ( ARenderPass::STextureAttachment const & attachment : ColorAttachments ) {
        hash = Core::PHHash32( attachment.Resource->Actual()->GetUID(), hash );
        RenderCore::ITexture * texture = attachment.Resource->Actual();
        colorAttachments[n].pTexture = texture;
        colorAttachments[n].Type = ATTACH_TEXTURE; // TODO: add ATTACH_LAYER path
        colorAttachments[n].LayerNum = 0; // TODO
        colorAttachments[n].LodNum = 0; // TODO
        framebufferCI.Width = texture->GetWidth();
        framebufferCI.Height = texture->GetHeight();
        n++;
    }
    if ( DepthStencilAttachment ) {
        hash = Core::PHHash32( DepthStencilAttachment->Resource->Actual()->GetUID(), hash );
        RenderCore::ITexture * texture = DepthStencilAttachment->Resource->Actual();
        depthStencilAttachment.pTexture = texture;
        depthStencilAttachment.Type = ATTACH_TEXTURE; // TODO: add ATTACH_LAYER path
        depthStencilAttachment.LayerNum = 0; // TODO
        depthStencilAttachment.LodNum = 0; // TODO
        framebufferCI.Width = texture->GetWidth();
        framebufferCI.Height = texture->GetHeight();
        framebufferCI.pDepthStencilAttachment = &depthStencilAttachment;
    } else {
        framebufferCI.pDepthStencilAttachment = nullptr;
    }

    int i = FramebufferHash.First( hash );
    for ( ; i != -1 ; i = FramebufferHash.Next( i ) ) {
        RenderCore::IFramebuffer * framebuffer = FramebufferCache[i];

        // Compare framebuffers
        if ( framebuffer->GetWidth() != framebufferCI.Width
             || framebuffer->GetHeight() != framebufferCI.Height
             || framebuffer->GetNumColorAttachments() != framebufferCI.NumColorAttachments
             || framebuffer->HasDepthStencilAttachment() != !!DepthStencilAttachment ) {
            continue;
        }

        if ( DepthStencilAttachment && !CompareAttachment( depthStencilAttachment, framebuffer->GetDepthStencilAttachment() ) ) {
            continue;
        }

        bool equal = true;
        for ( int a = 0 ; a < framebufferCI.NumColorAttachments ; a++ ) {
            if ( !CompareAttachment( framebufferCI.pColorAttachments[a], framebuffer->GetColorAttachments()[a] ) ) {
                equal = false;
                break;
            }
        }
        
        if ( equal ) {
            return framebuffer;
        }
    }

    // create new framebuffer

    i = FramebufferCache.Size();

    TRef< IFramebuffer > framebuffer;
    pDevice->CreateFramebuffer( framebufferCI, &framebuffer );

    FramebufferHash.Insert( hash, i );
    FramebufferCache.push_back( framebuffer );
    FramebufferHashes.Append( hash );

    GLogger.Printf( "Total framebuffers %d for %s hash 0x%08x\n", FramebufferCache.Size(), RenderPassName, hash );

    return framebuffer;
}

void ARenderPass::Execute( AFrameGraph & FrameGraph, RenderCore::IImmediateContext * Rcmd ) {
    if ( !ConditionFunction() ) {
        return;
    }

    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pRenderPass = Handle;
    renderPassBegin.pFramebuffer = pFramebuffer;
    renderPassBegin.RenderArea.X = pRenderArea->X;
    renderPassBegin.RenderArea.Y = pRenderArea->Y;
    renderPassBegin.RenderArea.Width = pRenderArea->Width;
    renderPassBegin.RenderArea.Height = pRenderArea->Height;
    renderPassBegin.pColorClearValues = GetClearValues().data();
    renderPassBegin.pDepthStencilClearValue = HasDepthStencilAttachment() ? &GetClearDepthStencilValue() : NULL;

    Rcmd->BeginRenderPass( renderPassBegin );

    SViewport vp;
    vp.X = pRenderArea->X;
    vp.Y = pRenderArea->Y;
    vp.Width = pRenderArea->Width;
    vp.Height = pRenderArea->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Rcmd->SetViewport( vp );

    int subpassIndex = 0;
    for ( ASubpassInfo const & Subpass : GetSubpasses() ) {
        Subpass.Function( *this, subpassIndex );
        subpassIndex++;
    }

    Rcmd->EndRenderPass();
}
