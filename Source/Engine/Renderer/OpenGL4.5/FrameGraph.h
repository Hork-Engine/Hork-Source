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

#pragma once

#include <Core/Public/String.h>
#include "GHI/GHIRenderPass.h"
#include "GHI/GHICommandBuffer.h"

// TODO: Delete unused textures and framebuffers

namespace OpenGL45 {

class AFrameGraph;
class ARenderTask;
class ACustomTask;
class ARenderPass;

struct Backbuffer
{
    // Dummy structure
};

struct BackbufferCreateInfo
{
    // Dummy structure
};

template< typename TResourceCreateInfo, typename TResource >
TResource * RealizeResource( AFrameGraph & InFrameGraph, TResourceCreateInfo const & Info );

template< typename TResource >
void DerealizeResource( AFrameGraph & InFrameGraph, TResource * InResource );

enum EResourceAcces
{
    RESOURCE_ACCESS_READ,
    RESOURCE_ACCESS_WRITE,
    RESOURCE_ACCESS_READ_WRITE
};

class AFrameGraphResourceBase
{
public:
    explicit AFrameGraphResourceBase( AFrameGraph & InFrameGraph, AString const & InName, ARenderTask * InRenderTask );

    AFrameGraphResourceBase( AFrameGraphResourceBase const & ) = delete;
    AFrameGraphResourceBase( AFrameGraphResourceBase && ) = default;

    virtual ~AFrameGraphResourceBase() = default;

    AFrameGraphResourceBase & operator=( AFrameGraphResourceBase const & ) = delete;
    AFrameGraphResourceBase & operator=( AFrameGraphResourceBase && ) = default;

    void SetResourceCapture( bool InbCaptured )
    {
        bCaptured = InbCaptured;
    }

    std::size_t GetId() const
    {
        return Id;
    }

    AString const & GetName() const
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
    friend class ARenderTask;

    virtual void Realize( AFrameGraph & InFrameGraph ) = 0;
    virtual void Derealize( AFrameGraph & InFrameGraph ) = 0;

    std::size_t Id;
    AString Name;
    ARenderTask const * Creator;
    std::vector< ARenderTask const * > Readers;
    std::vector< ARenderTask const * > Writers;
    std::size_t RefCount;
    bool bCaptured;
};

template< typename TResourceCreateInfo, typename TResource >
class TFrameGraphResource : public AFrameGraphResourceBase
{
public:
    using CreateInfoType = TResourceCreateInfo;
    using ResourceType = TResource;

    // Construct internal (transient) resource
    explicit TFrameGraphResource( AFrameGraph & InFrameGraph, AString const & Name, ARenderTask * InRenderTask, TResourceCreateInfo const & InCreateInfo )
        : AFrameGraphResourceBase( InFrameGraph, Name, InRenderTask ), CreateInfo( InCreateInfo ), pResource( nullptr )
    {
    }

    // Construct external resource
    explicit TFrameGraphResource( AFrameGraph & InFrameGraph, AString const & Name, TResourceCreateInfo const & InCreateInfo, TResource * InResource )
        : AFrameGraphResourceBase( InFrameGraph, Name, nullptr ), CreateInfo( InCreateInfo ), pResource( InResource )
    {
        AN_ASSERT( InResource );
    }

    TFrameGraphResource( TFrameGraphResource const & ) = delete;
    TFrameGraphResource( TFrameGraphResource && ) = default;
    ~TFrameGraphResource() = default;

    TFrameGraphResource & operator=( TFrameGraphResource const & ) = delete;
    TFrameGraphResource & operator=( TFrameGraphResource && ) = default;

    ResourceType * Actual() const
    {
        return pResource;
    }

protected:
    void Realize( AFrameGraph & InFrameGraph ) override
    {
        if ( IsTransient() )
        {
            AN_ASSERT( !pResource );

            //GLogger.Printf( "--- Realize %s ---\n", GetName().CStr() );
            pResource = RealizeResource< CreateInfoType, ResourceType >( InFrameGraph, CreateInfo );
        }
    }

    void Derealize( AFrameGraph & InFrameGraph ) override
    {
        if ( IsTransient() && pResource ) {
            //GLogger.Printf( "Derealize %s\n", GetName().CStr() );
            DerealizeResource< ResourceType >( InFrameGraph, pResource );
        }
    }

private:
    TResourceCreateInfo CreateInfo;
    TResource * pResource;
};

using AFrameGraphTextureStorage = TFrameGraphResource< GHI::TextureStorageCreateInfo, GHI::Texture >;

class ARenderArea
{
public:
    int X;
    int Y;
    int Width;
    int Height;

    ARenderArea()
        : X( 0 ), Y( 0 ), Width( 0 ), Height( 0 )
    {
    }
};

using AConditionFunction = std::function< bool() >;
using ARecordFunction = std::function< void( ARenderPass const &, int ) >;

class ASubpassInfo
{
public:
    ASubpassInfo()
    {
    }

    ASubpassInfo( std::initializer_list< GHI::AttachmentRef > const & ColorAttachmentRefs, ARecordFunction RecordFunction )
        : Refs( ColorAttachmentRefs ), Function( RecordFunction )
    {
    }

    std::vector< GHI::AttachmentRef > Refs;
    ARecordFunction Function;
};

class ARenderTask
{
public:
    explicit ARenderTask( AFrameGraph * InpFrameGraph, AString const & InName )
        : pFrameGraph( InpFrameGraph )
        , Name( InName )
        , RefCount( 0 )
        , bCulled( false )
    {
    }

    ARenderTask( ARenderTask const & ) = delete;
    ARenderTask( ARenderTask && ) = default;
    virtual ~ARenderTask() = default;

    ARenderTask & operator=( ARenderTask const & ) = delete;
    ARenderTask & operator=( ARenderTask && ) = default;

    AString const & GetName() const { return Name; }

    std::vector< std::unique_ptr< AFrameGraphResourceBase > > const & GetProducedResources() const
    {
        return ProducedResources;
    }

protected:
    friend class AFrameGraph;

    virtual void Create( AFrameGraph & FrameGraph ) = 0;
    virtual void Execute( AFrameGraph & FrameGraph ) = 0;

    template< typename TResourceDecl, typename TResourceCreateInfo >
    void AddNewResource( AString const & Name, TResourceCreateInfo const & CreateInfo, TResourceDecl ** ppResource = nullptr )
    {
        static_assert(std::is_same< typename TResourceDecl::CreateInfoType, TResourceCreateInfo >::value, "Invalid TResourceCreateInfo");

        ProducedResources.emplace_back( std::make_unique< TResourceDecl >( *pFrameGraph, Name, this, CreateInfo ) );

        if ( ppResource ) {
            *ppResource = static_cast< TResourceDecl * >(ProducedResources.back().get());
        }
    }

    template< typename TResourceDecl >
    void AddResource( TResourceDecl * InResource, EResourceAcces Access )
    {
        switch ( Access )
        {
        case RESOURCE_ACCESS_READ:
            InResource->Readers.push_back( this );
            ReadResources.push_back( InResource );
            break;
        case RESOURCE_ACCESS_WRITE:
            InResource->Writers.push_back( this );
            WriteResources.push_back( InResource );
            break;
        case RESOURCE_ACCESS_READ_WRITE:
            InResource->Readers.push_back( this );
            InResource->Writers.push_back( this );
            ReadWriteResources.push_back( InResource );
            break;
        }
    }

    AFrameGraph * pFrameGraph;
    AString Name;
    std::vector< std::unique_ptr< AFrameGraphResourceBase > > ProducedResources;
    std::vector< AFrameGraphResourceBase * > ReadResources;
    std::vector< AFrameGraphResourceBase * > WriteResources;
    std::vector< AFrameGraphResourceBase * > ReadWriteResources;
    std::size_t RefCount;
    bool bCulled;
};

class ACustomTask : public ARenderTask
{
    using Super = ARenderTask;

public:
    ACustomTask( AFrameGraph * pFrameGraph, AString const & InName )
        : ARenderTask( pFrameGraph, InName )
    {
    }

    ACustomTask( ACustomTask const & ) = delete;
    ACustomTask( ACustomTask && ) = default;
    virtual ~ACustomTask() = default;

    ACustomTask & operator=( ACustomTask const & ) = delete;
    ACustomTask & operator=( ACustomTask && ) = default;

    template< typename TResourceDecl, typename TResourceCreateInfo >
    ACustomTask & AddNewResource( AString const & Name, TResourceCreateInfo const & CreateInfo, TResourceDecl ** ppResource = nullptr )
    {
        Super::AddNewResource( Name, CreateInfo, ppResource );
        return *this;
    }

    template< typename TResourceDecl >
    ACustomTask & AddResource( TResourceDecl * InResource, EResourceAcces Access )
    {
        Super::AddResource( InResource, Access );
        return *this;
    }

protected:
    void Create( AFrameGraph & FrameGraph ) override {

    }

    void Execute( AFrameGraph & FrameGraph ) override {

    }
};

class ARenderPass : public ARenderTask
{
    using Super = ARenderTask;

public:
    ARenderPass( AFrameGraph * pFrameGraph, AString const & InName )
        : ARenderTask( pFrameGraph, InName )
        , pRenderArea( &RenderArea )
        , ClearDepthStencilValue( GHI::MakeClearDepthStencilValue( 0, 0 ) )
        , ConditionFunction( []() { return true; } )
    {
    }

    ARenderPass( ARenderPass const & ) = delete;
    ARenderPass( ARenderPass && ) = default;
    virtual ~ARenderPass() = default;

    ARenderPass & operator=( ARenderPass const & ) = delete;
    ARenderPass & operator=( ARenderPass && ) = default;

    template< typename TResourceDecl, typename TResourceCreateInfo >
    ARenderPass & AddNewResource( AString const & Name, TResourceCreateInfo const & CreateInfo, TResourceDecl ** ppResource = nullptr )
    {
        Super::AddNewResource( Name, CreateInfo, ppResource );
        return *this;
    }

    template< typename TResourceDecl >
    ARenderPass & AddResource( TResourceDecl * InResource, EResourceAcces Access )
    {
        Super::AddResource( InResource, Access );
        return *this;
    }

    struct STextureAttachment
    {
        AString Name;
        AFrameGraphTextureStorage * Resource;
        GHI::TextureStorageCreateInfo CreateInfo;
        GHI::AttachmentInfo Info;
        bool bCreateNewResource;

        STextureAttachment()
            : Resource( nullptr )
        {

        }

        STextureAttachment( AFrameGraphTextureStorage * InResource, GHI::AttachmentInfo const & InInfo )
            : Resource( InResource )
            , Info( InInfo )
            , bCreateNewResource( false )
        {

        }

        STextureAttachment( AString const & InName, GHI::TextureStorageCreateInfo const & InCreateInfo, GHI::AttachmentInfo const & InInfo )
            : Name( InName )
            , Resource( nullptr )
            , CreateInfo( InCreateInfo )
            , Info( InInfo )
            , bCreateNewResource( true )
        {

        }
    };

    ARenderPass & SetColorAttachments( std::initializer_list< STextureAttachment > const & InColorAttachments )
    {
        ColorAttachments = InColorAttachments;

        for ( STextureAttachment & attachment : ColorAttachments )
        {
            if ( attachment.bCreateNewResource )
            {
                AddNewResource( attachment.Name, attachment.CreateInfo, &attachment.Resource );
            }
            else
            {
                AddResource( attachment.Resource, RESOURCE_ACCESS_WRITE );
            }
        }

        return *this;
    }

    ARenderPass & SetDepthStencilAttachment( STextureAttachment const & Info )
    {
        DepthStencilAttachment = Info;
        bHasDepthStencilAttachment = true;

        if ( DepthStencilAttachment.bCreateNewResource )
        {
            AddNewResource( DepthStencilAttachment.Name, DepthStencilAttachment.CreateInfo, &DepthStencilAttachment.Resource );
        }
        else
        {
            AddResource( DepthStencilAttachment.Resource, RESOURCE_ACCESS_READ_WRITE );
        }

        return *this;
    }

    ARenderPass & SetRenderArea( int X, int Y, int W, int H )
    {
        RenderArea.X = X;
        RenderArea.Y = Y;
        RenderArea.Width = W;
        RenderArea.Height = H;
        return *this;
    }

    ARenderPass & SetRenderArea( int W, int H )
    {
        RenderArea.Width = W;
        RenderArea.Height = H;
        return *this;
    }

    // Dynamic rendering area allows to change rendering area without rebuilding the framegraph.
    ARenderPass & SetDynamicRenderArea( ARenderArea * InRenderArea )
    {
        pRenderArea = InRenderArea;
        return *this;
    }

    ARenderPass & SetCondition( AConditionFunction InConditionFunction )
    {
        ConditionFunction = InConditionFunction;
        return *this;
    }

    ARenderPass & AddSubpass( std::initializer_list< GHI::AttachmentRef > const & ColorAttachmentRefs, ARecordFunction RecordFunction )
    {
        Subpasses.emplace_back( ColorAttachmentRefs, RecordFunction );
        return *this;
    }

    ARenderPass & SetClearColors( std::initializer_list< GHI::ClearColorValue > const & Values )
    {
        ClearValues = Values;
        return *this;
    }

    ARenderPass & SetDepthStencilClearValue( GHI::ClearDepthStencilValue const & Value )
    {
        ClearDepthStencilValue = Value;
        return *this;
    }

    // Getters

    ARenderArea const & GetRenderArea() const
    {
        return *pRenderArea;
    }

    std::vector< ASubpassInfo > const & GetSubpasses() const
    {
        return Subpasses;
    }

    std::vector< STextureAttachment > const & GetColorAttachments() const
    {
        return ColorAttachments;
    }

    STextureAttachment const & GetDepthStencilAttachment() const
    {
        return DepthStencilAttachment;
    }

    bool HasDepthStencilAttachment() const
    {
        return bHasDepthStencilAttachment;
    }

    GHI::RenderPass & GetHandle()
    {
        return Handle;
    }

    std::vector< GHI::ClearColorValue > const & GetClearValues() const
    {
        return ClearValues;
    }

    GHI::ClearDepthStencilValue const & GetClearDepthStencilValue() const
    {
        return ClearDepthStencilValue;
    }

protected:
    void Create( AFrameGraph & FrameGraph ) override;
    void Execute( AFrameGraph & FrameGraph ) override;

private:
    std::vector< STextureAttachment > ColorAttachments;
    STextureAttachment DepthStencilAttachment;
    bool bHasDepthStencilAttachment = false;
    ARenderArea RenderArea;
    ARenderArea * pRenderArea;
    std::vector< GHI::ClearColorValue > ClearValues;
    GHI::ClearDepthStencilValue ClearDepthStencilValue;
    std::vector< ASubpassInfo > Subpasses;
    GHI::RenderPass Handle;
    GHI::Framebuffer * pFramebuffer = nullptr;
    AConditionFunction ConditionFunction;
};

class AFrameGraph
{
public:
    ~AFrameGraph()
    {
        DerializeCapturedResources();
    }

    void Clear()
    {
        DerializeCapturedResources();
        CapturedResources.clear();
        ExternalResources.clear();
        Resources.clear();
        RenderTasks.clear();
        IdGenerator = 0;
    }

    void ResetResources()
    {
        Textures.clear();
        FreeTextures.clear();
        FramebufferHash.Clear();
        FramebufferCache.clear();
    }

    template< typename TTask >
    TTask & AddTask( AString const & InName )
    {
        RenderTasks.emplace_back( std::make_unique< TTask >( this, InName ) );
        return *static_cast< TTask * >( RenderTasks.back().get() );
    }

    template< typename TResourceCreateInfo, typename TResource >
    TFrameGraphResource< TResourceCreateInfo, TResource > * AddExternalResource( AString const & Name, TResourceCreateInfo const & CreateInfo, TResource * Resource = nullptr )
    {
        ExternalResources.emplace_back( std::make_unique< TFrameGraphResource< TResourceCreateInfo, TResource > >( *this, Name, CreateInfo, Resource ) );
        return static_cast< TFrameGraphResource< TResourceCreateInfo, TResource > * >(ExternalResources.back().get());
    }

    void Build();

    void Execute();

    void Debug();

    void ExportGraphviz( std::string const & FileName );

    GHI::Framebuffer * GetFramebuffer( std::vector< ARenderPass::STextureAttachment > const & ColorAttachments, ARenderPass::STextureAttachment const * DepthStencilAttachment );

    std::size_t GenerateResourceId() const {
        return IdGenerator++;
    }

    std::vector< std::unique_ptr< GHI::Texture > > Textures; // All textures
    std::vector< GHI::Texture * > FreeTextures; // Free list

private:
    void RegisterResources()
    {
        Resources.clear();

        for ( std::unique_ptr< ARenderTask > & task : RenderTasks ) {
            for ( auto & resourcePtr : task->GetProducedResources() ) {
                Resources.push_back( resourcePtr.get() );
            }
        }

        for ( auto & resourcePtr : ExternalResources ) {
            Resources.push_back( resourcePtr.get() );
        }
    }

    void DerializeCapturedResources()
    {
        for ( AFrameGraphResourceBase * resource : CapturedResources ) {
            resource->Derealize( *this );
        }
    }

    std::vector< std::unique_ptr< ARenderTask > > RenderTasks;
    std::vector< std::unique_ptr< AFrameGraphResourceBase > > ExternalResources;
    std::vector< AFrameGraphResourceBase * > Resources; // all resources
    std::vector< AFrameGraphResourceBase * > CapturedResources;

    struct STimelineStep
    {
        ARenderTask * RenderTask;
        std::vector< AFrameGraphResourceBase * > RealizedResources;
        std::vector< AFrameGraphResourceBase * > DerealizedResources;
    };

    std::vector< STimelineStep > Timeline;

    THash<> FramebufferHash;
    std::vector< std::unique_ptr< GHI::Framebuffer > > FramebufferCache;

    mutable std::size_t IdGenerator = 0;
};

AN_FORCEINLINE AFrameGraphResourceBase::AFrameGraphResourceBase( AFrameGraph & InFrameGraph, AString const & InName, ARenderTask * InRenderTask )
    : Id( InFrameGraph.GenerateResourceId() )
    , Name( InName )
    , Creator( InRenderTask )
    , RefCount( 0 )
    , bCaptured( false )
{
}

}
