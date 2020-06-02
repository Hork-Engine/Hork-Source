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
#include "OpenGL45Common.h"

#include <stack>

using namespace GHI;

ARuntimeVariable RVFrameGraphDebug( _CTS( "FrameGraphDebug" ), _CTS( "0" ) );

namespace OpenGL45 {

template<>
GHI::Texture * RealizeResource( AFrameGraph & InFrameGraph, GHI::TextureStorageCreateInfo const & Info )
{
    // Find free appropriate texture
    for ( auto it = InFrameGraph.FreeTextures.begin() ; it != InFrameGraph.FreeTextures.end() ; it++ ) {
        GHI::Texture * tex = *it;

        if ( tex->GetType() == Info.Type
             && tex->GetInternalPixelFormat() == Info.InternalFormat
             && tex->GetResoulution() == Info.Resolution
             && tex->GetSamplesCount() == Info.Multisample.NumSamples
             && tex->FixedSampleLocations() == Info.Multisample.bFixedSampleLocations
             && tex->GetSwizzle() == Info.Swizzle
             && tex->GetStorageNumLods() == Info.NumLods )
        {
            //GLogger.Printf( "Reusing existing texture\n" );
            InFrameGraph.FreeTextures.erase( it );
            return tex;
        }
    }

    // Create new texture
    //GLogger.Printf( "Create new texture\n" );
    InFrameGraph.Textures.emplace_back( std::make_unique< GHI::Texture >() );
    GHI::Texture * texture = InFrameGraph.Textures.back().get();
    texture->InitializeStorage( Info );
    return texture;
}

template<>
void DerealizeResource( AFrameGraph & InFrameGraph, GHI::Texture * InResource )
{
    InFrameGraph.FreeTextures.push_back( InResource );
}

void AFrameGraph::Build()
{
    AN_ASSERT( CapturedResources.empty() );

    RegisterResources();

    for ( std::unique_ptr< ARenderTask > & renderPass : RenderTasks ) {
        renderPass->RefCount = renderPass->ProducedResources.size() + renderPass->WriteResources.size()
            + renderPass->ReadWriteResources.size();
    }

    for ( AFrameGraphResourceBase * resource : Resources ) {
        resource->RefCount = resource->Readers.size();

        if ( resource->IsCaptured() ) {
            CapturedResources.push_back( resource );
        }
    }

    std::stack< AFrameGraphResourceBase * > unreferencedResources;

    for ( AFrameGraphResourceBase * resource : Resources ) {
        if ( resource->RefCount == 0 && resource->IsTransient() && !resource->IsCaptured() ) {
            unreferencedResources.push( resource );
        }
    }

    while ( !unreferencedResources.empty() ) {
        AFrameGraphResourceBase * unreferencedResource = unreferencedResources.top();
        unreferencedResources.pop();

        ARenderTask * creator = const_cast< ARenderTask * >( unreferencedResource->Creator );
        if ( creator->RefCount > 0 ) {
            creator->RefCount--;
        }

        if ( creator->RefCount == 0 && !creator->bCulled ) {
            for ( AFrameGraphResourceBase * readResource : creator->ReadResources ) {
                if ( readResource->RefCount > 0 )
                    readResource->RefCount--;
                if ( readResource->RefCount == 0 && readResource->IsTransient() )
                    unreferencedResources.push( readResource );
            }
        }

        for ( ARenderTask const * c_writer : unreferencedResource->Writers ) {
            ARenderTask * writer = const_cast< ARenderTask * >( c_writer );
            if ( writer->RefCount > 0 ) {
                writer->RefCount--;
            }
            if ( writer->RefCount == 0 && !writer->bCulled ) {
                for ( AFrameGraphResourceBase * readResource : writer->ReadResources ) {
                    if ( readResource->RefCount > 0 )
                        readResource->RefCount--;
                    if ( readResource->RefCount == 0 && readResource->IsTransient() )
                        unreferencedResources.push( readResource );
                }
            }
        }
    }

    Timeline.clear();

    std::vector< AFrameGraphResourceBase * > realizedResources, derealizedResources;

    for ( std::unique_ptr< ARenderTask > & task : RenderTasks ) {
        if ( task->RefCount == 0 && !task->bCulled ) {
            continue;
        }

        realizedResources.clear();
        derealizedResources.clear();

        for ( auto & resource : task->ProducedResources ) {
            realizedResources.push_back( const_cast< AFrameGraphResourceBase * >( resource.get() ) );
            if ( resource->Readers.empty() && resource->Writers.empty() && !resource->IsCaptured() ) {
                derealizedResources.push_back( const_cast<AFrameGraphResourceBase*>(resource.get()) );
            }
        }

        auto resourcesRW = task->ReadResources;
        resourcesRW.insert( resourcesRW.end(), task->WriteResources.begin(), task->WriteResources.end() );
        resourcesRW.insert( resourcesRW.end(), task->ReadWriteResources.begin(), task->ReadWriteResources.end() );
        for ( AFrameGraphResourceBase * resource : resourcesRW ) {
            if ( !resource->IsTransient() || resource->IsCaptured() ) {
                continue;
            }

            bool bValid = false;
            std::size_t lastIndex = 0;
            if ( !resource->Readers.empty() )
            {
                auto lastReader = std::find_if(
                    RenderTasks.begin(),
                    RenderTasks.end  (),
                    [&resource]( std::unique_ptr< ARenderTask > const & iteratee )
                {
                    return iteratee.get() == resource->Readers.back();
                } );
                if ( lastReader != RenderTasks.end() )
                {
                    bValid = true;
                    lastIndex = std::distance( RenderTasks.begin(), lastReader );
                }
            }
            if ( !resource->Writers.empty() )
            {
                auto lastWriter = std::find_if(
                    RenderTasks.begin(),
                    RenderTasks.end  (),
                    [&resource]( std::unique_ptr< ARenderTask > const & iteratee )
                {
                    return iteratee.get() == resource->Writers.back();
                } );
                if ( lastWriter != RenderTasks.end() )
                {
                    bValid = true;
                    lastIndex = std::max( lastIndex, std::size_t( std::distance( RenderTasks.begin(), lastWriter ) ) );
                }
            }

            if ( bValid && RenderTasks[lastIndex] == task ) {
                derealizedResources.push_back( const_cast< AFrameGraphResourceBase * >( resource ) );
            }
        }

        Timeline.emplace_back( STimelineStep{ task.get(), realizedResources, derealizedResources } );

        STimelineStep & step = Timeline.back();

        // Resolve pointers
        for ( AFrameGraphResourceBase * resource : step.RealizedResources ) {
            resource->Realize( *this );
        }
        // Initialize task
        task->Create( *this );
        // Resolve pointers
        for ( AFrameGraphResourceBase * resource : step.DerealizedResources ) {
            resource->Derealize( *this );
        }
    }
}

void AFrameGraph::Execute()
{
    for ( STimelineStep & step : Timeline ) {
        //for ( AFrameGraphResourceBase * resource : step.RealizedResources ) {
        //    resource->Realize( *this );
        //}

        step.RenderTask->Execute( *this );

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
        for ( AFrameGraphResourceBase * resource : step.RealizedResources ) {
            GLogger.Printf( "Realize %s\n", resource->GetName().CStr() );
        }

        GLogger.Printf( "Execute %s\n", step.RenderTask->GetName().CStr() );

        for ( AFrameGraphResourceBase * resource : step.DerealizedResources ) {
            GLogger.Printf( "Derealize %s\n", resource->GetName().CStr() );
        }
    }
    GLogger.Printf( "--------------------------------\n" );
}

void AFrameGraph::ExportGraphviz( std::string const & FileName )
{
    AFileStream f;

    if ( !f.OpenWrite( FileName.c_str() ) ) {
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
                  resource->GetName().CStr(), resource->GetName().CStr(), resource->RefCount, resource->GetId(),
                  color );
    }
    f.Printf( "\n" );

    for ( auto & render_task : RenderTasks ) {
        f.Printf( "\"%s\" [label=\"%s\\nRefs: %d\", style=filled, fillcolor=darkorange]\n",
                  render_task->GetName().CStr(), render_task->GetName().CStr(), render_task->RefCount );

        if ( !render_task->ProducedResources.empty() )
        {
            f.Printf( "\"%s\" -> { ", render_task->GetName().CStr() );
            for ( auto & resource : render_task->ProducedResources ) {
                f.Printf( "\"%s\" ", resource->GetName().CStr() );
            }
            f.Printf( "} [color=seagreen]\n" );
        }

        if ( !render_task->WriteResources.empty() )
        {
            f.Printf( "\"%s\" -> { ", render_task->GetName().CStr() );
            for ( auto & resource : render_task->WriteResources ) {
                f.Printf( "\"%s\" ", resource->GetName().CStr() );
            }
            f.Printf( "} [color=gold]\n" );
        }
    }
    f.Printf( "\n" );

    for ( auto & resource : Resources ) {
        f.Printf( "\"%s\" -> { ", resource->GetName().CStr() );
        for ( auto& render_task : resource->Readers ) {
            f.Printf( "\"%s\" ", render_task->GetName().CStr() );
        }
        f.Printf( "} [color=skyblue]\n" );
    }
    f.Printf( "}" );
}

void ARenderPass::Create( AFrameGraph & FrameGraph )
{
    GHI::RenderPassCreateInfo renderPassCI = {};

    std::vector< GHI::AttachmentInfo > attachmentInfos;
    attachmentInfos.resize( ColorAttachments.size() );
    for ( int i = 0 ; i < ColorAttachments.size() ; i++ ) {
        attachmentInfos[i] = ColorAttachments[i].Info;
    }

    renderPassCI.NumColorAttachments = attachmentInfos.size();
    renderPassCI.pColorAttachments = attachmentInfos.data();
    renderPassCI.pDepthStencilAttachment = bHasDepthStencilAttachment ? &DepthStencilAttachment.Info : nullptr;
    renderPassCI.NumSubpasses = Subpasses.size();

    std::vector< GHI::SubpassInfo > subpassesTemp;
    subpassesTemp.resize( renderPassCI.NumSubpasses );

    for ( int i = 0 ; i < renderPassCI.NumSubpasses ; i++ ) {
        subpassesTemp[i].NumColorAttachments = Subpasses[i].Refs.size();
        subpassesTemp[i].pColorAttachmentRefs = Subpasses[i].Refs.data();
    }
    renderPassCI.pSubpasses = subpassesTemp.data();

    while ( ClearValues.size() < renderPassCI.NumColorAttachments ) {
        ClearValues.push_back( MakeClearColorValue( 0, 0, 0, 0 ) );
    }

    Handle.Initialize( renderPassCI );

    pFramebuffer = FrameGraph.GetFramebuffer( ColorAttachments, bHasDepthStencilAttachment ? &DepthStencilAttachment : nullptr );
}

GHI::Framebuffer * AFrameGraph::GetFramebuffer( std::vector< ARenderPass::STextureAttachment > const & ColorAttachments,
                                                ARenderPass::STextureAttachment const * DepthStencilAttachment ) {

    GHI::FramebufferCreateInfo framebufferCI = {};
    std::vector< FramebufferAttachmentInfo > colorAttachments;
    FramebufferAttachmentInfo depthStencilAttachment;

    colorAttachments.resize( ColorAttachments.size() );

    framebufferCI.NumColorAttachments = colorAttachments.size();
    framebufferCI.pColorAttachments = colorAttachments.data();

    // Make hash
    int hash = 0;
    int n = 0;
    for ( ARenderPass::STextureAttachment const & attachment : ColorAttachments ) {
        hash = Core::PHHash32( attachment.Resource->GetId(), hash );
        GHI::Texture * texture = attachment.Resource->Actual();
        colorAttachments[n].pTexture = texture;
        colorAttachments[n].Type = ATTACH_TEXTURE; // TODO: add ATTACH_LAYER path
        colorAttachments[n].LayerNum = 0; // TODO
        colorAttachments[n].LodNum = 0; // TODO
        framebufferCI.Width = texture->GetWidth();
        framebufferCI.Height = texture->GetHeight();
        n++;
    }
    if ( DepthStencilAttachment ) {
        hash = Core::PHHash32( DepthStencilAttachment->Resource->GetId(), hash );
        GHI::Texture * texture = DepthStencilAttachment->Resource->Actual();
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
        GHI::Framebuffer * framebuffer = FramebufferCache[i].get();

        // Compare framebuffers
        if ( framebuffer->GetWidth() != framebufferCI.Width
             || framebuffer->GetHeight() != framebufferCI.Height
             || framebuffer->GetNumColorAttachments() != framebufferCI.NumColorAttachments
             || framebuffer->HasDepthStencilAttachment() != !!DepthStencilAttachment ) {
            continue;
        }

        if ( DepthStencilAttachment &&
             memcmp( &depthStencilAttachment, &framebuffer->GetDepthStencilAttachment(), sizeof( depthStencilAttachment ) ) ) {
            continue;
        }
        
        if ( !memcmp( framebufferCI.pColorAttachments, framebuffer->GetColorAttachments(),
                      framebufferCI.NumColorAttachments * sizeof( FramebufferAttachmentInfo ) ) )
        {
            return framebuffer;
        }
    }

    // create new framebuffer

    i = FramebufferCache.size();

    FramebufferHash.Insert( hash, i );
    FramebufferCache.emplace_back( std::make_unique< GHI::Framebuffer >() );

    GHI::Framebuffer * framebuffer = FramebufferCache.back().get();

    framebuffer->Initialize( framebufferCI );

    return framebuffer;
}

void ARenderPass::Execute( AFrameGraph & FrameGraph ) {
    if ( !ConditionFunction() ) {
        return;
    }

    RenderPassBegin renderPassBegin = {};
    renderPassBegin.pRenderPass = &Handle;
    renderPassBegin.pFramebuffer = pFramebuffer;
    renderPassBegin.RenderArea.X = pRenderArea->X;
    renderPassBegin.RenderArea.Y = pRenderArea->Y;
    renderPassBegin.RenderArea.Width = pRenderArea->Width;
    renderPassBegin.RenderArea.Height = pRenderArea->Height;
    renderPassBegin.pColorClearValues = GetClearValues().data();
    renderPassBegin.pDepthStencilClearValue = HasDepthStencilAttachment() ? &GetClearDepthStencilValue() : NULL;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = pRenderArea->X;
    vp.Y = pRenderArea->Y;
    vp.Width = pRenderArea->Width;
    vp.Height = pRenderArea->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    int subpassIndex = 0;
    for ( ASubpassInfo const & Subpass : GetSubpasses() ) {
        Subpass.Function( *this, subpassIndex );
        subpassIndex++;
    }

    Cmd.EndRenderPass();
}

}
