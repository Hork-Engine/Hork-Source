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

#include <Core/Public/String.h>
#include <Core/Public/PodStack.h>

#include <RenderCore/Device.h>

// TODO:
// 1. Optimize. Very slow framegraph rebuilding in debug mode.
// 2. Convert passes to subpasses if possible
// 3. Destroy unused framebuffers and textures (after some time?)

class AFrameGraph;
class ARenderTask;
class ACustomTask;
class ARenderPass;

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

class AFrameGraphResourceProxy
{
public:
    explicit AFrameGraphResourceProxy( AFrameGraph & InFrameGraph, const char * InName, ARenderTask * InRenderTask );

    AFrameGraphResourceProxy( AFrameGraphResourceProxy const & ) = delete;
    AFrameGraphResourceProxy( AFrameGraphResourceProxy && ) = default;

    virtual ~AFrameGraphResourceProxy() = default;

    AFrameGraphResourceProxy & operator=( AFrameGraphResourceProxy const & ) = delete;
    AFrameGraphResourceProxy & operator=( AFrameGraphResourceProxy && ) = default;

    void SetResourceCapture( bool InbCaptured )
    {
        bCaptured = InbCaptured;
    }

    std::size_t GetId() const
    {
        return Id;
    }

    const char * GetName() const
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
    const char * Name;
    ARenderTask const * Creator;
    TPodVector< ARenderTask const * > Readers;
    TPodVector< ARenderTask const * > Writers;
    std::size_t ResourceRefs;
    bool bCaptured;
};

template< typename TResourceCreateInfo, typename TResource >
class TFrameGraphResourceProxy : public AFrameGraphResourceProxy
{
public:
    using CreateInfoType = TResourceCreateInfo;
    using ResourceType = TResource;

    // Construct internal (transient) resource
    explicit TFrameGraphResourceProxy( AFrameGraph & InFrameGraph, const char * Name, ARenderTask * InRenderTask, TResourceCreateInfo const & InCreateInfo )
        : AFrameGraphResourceProxy( InFrameGraph, Name, InRenderTask ), CreateInfo( InCreateInfo ), pResource( nullptr )
    {
    }

    // Construct external resource
    explicit TFrameGraphResourceProxy( AFrameGraph & InFrameGraph, const char * Name, TResourceCreateInfo const & InCreateInfo, TResource * InResource )
        : AFrameGraphResourceProxy( InFrameGraph, Name, nullptr ), CreateInfo( InCreateInfo ), pResource( InResource )
    {
        AN_ASSERT( InResource );
    }

    TFrameGraphResourceProxy( TFrameGraphResourceProxy const & ) = delete;
    TFrameGraphResourceProxy( TFrameGraphResourceProxy && ) = default;
    ~TFrameGraphResourceProxy() = default;

    TFrameGraphResourceProxy & operator=( TFrameGraphResourceProxy const & ) = delete;
    TFrameGraphResourceProxy & operator=( TFrameGraphResourceProxy && ) = default;

    ResourceType * Actual() const
    {
        return pResource;
    }

    TResourceCreateInfo const & GetCreateInfo() const { return CreateInfo; }

protected:
    void Realize( AFrameGraph & InFrameGraph ) override
    {
        if ( IsTransient() )
        {
            AN_ASSERT( !pResource );

            //GLogger.Printf( "--- Realize %s ---\n", GetName() );
            pResource = RealizeResource< CreateInfoType, ResourceType >( InFrameGraph, CreateInfo );
        }
    }

    void Derealize( AFrameGraph & InFrameGraph ) override
    {
        if ( IsTransient() && pResource ) {
            //GLogger.Printf( "Derealize %s\n", GetName() );
            DerealizeResource< ResourceType >( InFrameGraph, pResource );
        }
    }

private:
    TResourceCreateInfo CreateInfo;
    TResource * pResource;
};

using AFrameGraphTexture = TFrameGraphResourceProxy< RenderCore::STextureCreateInfo, RenderCore::ITexture >;

using AFrameGraphBufferView = TFrameGraphResourceProxy< RenderCore::SBufferViewCreateInfo, RenderCore::IBufferView >;

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
using ATaskFunction = std::function< void( ACustomTask const & ) >;

class ASubpassInfo
{
public:
    ASubpassInfo()
    {
    }

    ASubpassInfo( std::initializer_list< RenderCore::SAttachmentRef > const & ColorAttachmentRefs, ARecordFunction RecordFunction )
        : Refs( ColorAttachmentRefs ), Function( RecordFunction )
    {
    }

    TStdVector< RenderCore::SAttachmentRef > Refs;
    ARecordFunction Function;
};

class ARenderTask
{
public:
    explicit ARenderTask( AFrameGraph * InFrameGraph, const char * InName )
        : pFrameGraph( InFrameGraph )
        , Name( InName )
        , ResourceRefs( 0 )
        , bCulled( false )
    {
    }

    ARenderTask( ARenderTask const & ) = delete;
    ARenderTask( ARenderTask && ) = default;
    virtual ~ARenderTask() = default;

    ARenderTask & operator=( ARenderTask const & ) = delete;
    ARenderTask & operator=( ARenderTask && ) = default;

    const char * GetName() const { return Name; }

    TStdVector< StdUniquePtr< AFrameGraphResourceProxy > > const & GetProducedResources() const
    {
        return ProducedResources;
    }

protected:
    friend class AFrameGraph;

    virtual void Create( AFrameGraph & FrameGraph ) = 0;
    virtual void Execute( AFrameGraph & FrameGraph, RenderCore::IImmediateContext * Rcmd ) = 0;

    template< char... NameChars, typename TResourceDecl, typename TResourceCreateInfo >
    void AddNewResource( const char * Name, TResourceCreateInfo const & CreateInfo, TResourceDecl ** ppResource = nullptr )
    {
        static_assert(std::is_same< typename TResourceDecl::CreateInfoType, TResourceCreateInfo >::value, "Invalid TResourceCreateInfo");

        ProducedResources.emplace_back( StdMakeUnique< TResourceDecl >( *pFrameGraph, Name, this, CreateInfo ) );

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
            InResource->Readers.Append( this );
            ReadResources.Append( InResource );
            break;
        case RESOURCE_ACCESS_WRITE:
            InResource->Writers.Append( this );
            WriteResources.Append( InResource );
            break;
        case RESOURCE_ACCESS_READ_WRITE:
            InResource->Readers.Append( this );
            InResource->Writers.Append( this );
            ReadWriteResources.Append( InResource );
            break;
        }
    }

    AFrameGraph * pFrameGraph;
    const char * Name;
    TStdVector< StdUniquePtr< AFrameGraphResourceProxy > > ProducedResources;
    TPodVector< AFrameGraphResourceProxy * > ReadResources;
    TPodVector< AFrameGraphResourceProxy * > WriteResources;
    TPodVector< AFrameGraphResourceProxy * > ReadWriteResources;
    std::size_t ResourceRefs;
    bool bCulled;
};

class ACustomTask : public ARenderTask
{
    using Super = ARenderTask;

public:
    ACustomTask( AFrameGraph * pFrameGraph, const char * InName )
        : ARenderTask( pFrameGraph, InName )
    {
    }

    ACustomTask( ACustomTask const & ) = delete;
    ACustomTask( ACustomTask && ) = default;
    virtual ~ACustomTask() = default;

    ACustomTask & operator=( ACustomTask const & ) = delete;
    ACustomTask & operator=( ACustomTask && ) = default;

    template< char... NameChars, typename TResourceDecl, typename TResourceCreateInfo >
    ACustomTask & AddNewResource( const char * Name, TResourceCreateInfo const & CreateInfo, TResourceDecl ** ppResource = nullptr )
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

    ACustomTask & SetFunction( ATaskFunction RecordFunction )
    {
        Function = RecordFunction;
        return *this;
    }

protected:
    void Create( AFrameGraph & FrameGraph ) override {

    }

    void Execute( AFrameGraph & FrameGraph, RenderCore::IImmediateContext * Rcmd ) override {
        Function( *this );
    }

private:
    ATaskFunction Function;
};

class ARenderPass : public ARenderTask
{
    using Super = ARenderTask;

public:
    ARenderPass( AFrameGraph * pFrameGraph, const char * InName )
        : ARenderTask( pFrameGraph, InName )
        , pRenderArea( &RenderArea )
        , ClearDepthStencilValue( RenderCore::MakeClearDepthStencilValue( 0, 0 ) )
        , ConditionFunction( []() { return true; } )
    {
    }

    ARenderPass( ARenderPass const & ) = delete;
    ARenderPass( ARenderPass && ) = default;
    virtual ~ARenderPass() = default;

    ARenderPass & operator=( ARenderPass const & ) = delete;
    ARenderPass & operator=( ARenderPass && ) = default;

    template< char... NameChars, typename TResourceDecl, typename TResourceCreateInfo >
    ARenderPass & AddNewResource( const char * Name, TResourceCreateInfo const & CreateInfo, TResourceDecl ** ppResource = nullptr )
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
        const char * Name;
        AFrameGraphTexture * Resource;
        RenderCore::STextureCreateInfo CreateInfo;
        RenderCore::SAttachmentInfo Info;
        bool bCreateNewResource;

        STextureAttachment()
            : Resource( nullptr )
        {

        }

        STextureAttachment( AFrameGraphTexture * InResource, RenderCore::SAttachmentInfo const & InInfo )
            : Resource( InResource )
            , Info( InInfo )
            , bCreateNewResource( false )
        {

        }

        STextureAttachment( const char * InName, RenderCore::STextureCreateInfo const & InCreateInfo, RenderCore::SAttachmentInfo const & InInfo )
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

    ARenderPass & AddSubpass( std::initializer_list< RenderCore::SAttachmentRef > const & ColorAttachmentRefs, ARecordFunction RecordFunction )
    {
        Subpasses.emplace_back( ColorAttachmentRefs, RecordFunction );
        return *this;
    }

    ARenderPass & SetClearColors( std::initializer_list< RenderCore::SClearColorValue > const & Values )
    {
        ClearValues = Values;
        return *this;
    }

    ARenderPass & SetDepthStencilClearValue( RenderCore::SClearDepthStencilValue const & Value )
    {
        ClearDepthStencilValue = Value;
        return *this;
    }

    // Getters

    ARenderArea const & GetRenderArea() const
    {
        return *pRenderArea;
    }

    TStdVector< ASubpassInfo > const & GetSubpasses() const
    {
        return Subpasses;
    }

    TStdVector< STextureAttachment > const & GetColorAttachments() const
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

    //RenderCore::IRenderPass & GetHandle()
    //{
    //    return Handle;
    //}

    TStdVector< RenderCore::SClearColorValue > const & GetClearValues() const
    {
        return ClearValues;
    }

    RenderCore::SClearDepthStencilValue const & GetClearDepthStencilValue() const
    {
        return ClearDepthStencilValue;
    }

    RenderCore::IFramebuffer * GetFramebuffer() const { return pFramebuffer; }

protected:
    void Create( AFrameGraph & FrameGraph ) override;
    void Execute( AFrameGraph & FrameGraph, RenderCore::IImmediateContext * Rcmd ) override;

private:
    TStdVector< STextureAttachment > ColorAttachments;
    STextureAttachment DepthStencilAttachment;
    bool bHasDepthStencilAttachment = false;
    ARenderArea RenderArea;
    ARenderArea * pRenderArea;
    TStdVector< RenderCore::SClearColorValue > ClearValues;
    RenderCore::SClearDepthStencilValue ClearDepthStencilValue;
    TStdVector< ASubpassInfo > Subpasses;
    TRef< RenderCore::IRenderPass > Handle;
    RenderCore::IFramebuffer * pFramebuffer = nullptr;
    AConditionFunction ConditionFunction;
};

class AFrameGraph : public ARefCounted
{
public:
    AFrameGraph( RenderCore::IDevice * InDevice )
        : pDevice( InDevice )
    {

    }

    ~AFrameGraph()
    {
        DerializeCapturedResources();
    }

    RenderCore::IDevice * GetDevice() { return pDevice; }

    void Clear()
    {
        DerializeCapturedResources();
        CapturedResources.Clear();
        ExternalResources.Clear();
        Resources.Clear();
        RenderTasks.Clear();
        IdGenerator = 0;
    }

    void ResetResources()
    {
        Textures.Clear();
        FreeTextures.Clear();
        FramebufferHash.Clear();
        FramebufferCache.Clear();
    }

    template< typename TTask >
    TTask & AddTask( const char * InName )
    {
        RenderTasks.emplace_back( StdMakeUnique< TTask >( this, InName ) );
        return *static_cast< TTask * >( RenderTasks.back().get() );
    }

    template< typename TResourceCreateInfo, typename TResource >
    TFrameGraphResourceProxy< TResourceCreateInfo, TResource > * AddExternalResource( const char * Name, TResourceCreateInfo const & CreateInfo, TResource * Resource = nullptr )
    {
        ExternalResources.emplace_back( StdMakeUnique< TFrameGraphResourceProxy< TResourceCreateInfo, TResource > >( *this, Name, CreateInfo, Resource ) );
        return static_cast< TFrameGraphResourceProxy< TResourceCreateInfo, TResource > * >(ExternalResources.back().get());
    }

    template< typename TResourceCreateInfo, typename TResource >
    TFrameGraphResourceProxy< TResourceCreateInfo, TResource > * AddExternalResource( const char * Name, TResourceCreateInfo const & CreateInfo, TRef< TResource > & Resource )
    {
        ExternalResources.emplace_back( StdMakeUnique< TFrameGraphResourceProxy< TResourceCreateInfo, TResource > >( *this, Name, CreateInfo, Resource.GetObject() ) );
        return static_cast< TFrameGraphResourceProxy< TResourceCreateInfo, TResource > * >(ExternalResources.back().get());
    }

    void Build();

    void Execute( RenderCore::IImmediateContext * Rcmd );

    void Debug();

    void ExportGraphviz( const char * FileName );

    RenderCore::IFramebuffer * GetFramebuffer( const char * RenderPassName, TStdVector< ARenderPass::STextureAttachment > const & ColorAttachments, ARenderPass::STextureAttachment const * DepthStencilAttachment );

    std::size_t GenerateResourceId() const {
        return IdGenerator++;
    }

    TStdVector< TRef< RenderCore::ITexture > > Textures; // All textures
    TPodVector< RenderCore::ITexture * > FreeTextures; // Free list

private:
    void RegisterResources()
    {
        Resources.Clear();

        for ( StdUniquePtr< ARenderTask > & task : RenderTasks ) {
            for ( auto & resourcePtr : task->GetProducedResources() ) {
                Resources.Append( resourcePtr.get() );
            }
        }

        for ( auto & resourcePtr : ExternalResources ) {
            Resources.Append( resourcePtr.get() );
        }
    }

    void DerializeCapturedResources()
    {
        for ( AFrameGraphResourceProxy * resource : CapturedResources ) {
            resource->Derealize( *this );
        }
    }

    RenderCore::IDevice * pDevice;

    TStdVector< StdUniquePtr< ARenderTask > > RenderTasks;
    TStdVector< StdUniquePtr< AFrameGraphResourceProxy > > ExternalResources;
    TPodVector< AFrameGraphResourceProxy * > Resources; // all resources
    TPodVector< AFrameGraphResourceProxy * > CapturedResources;

    struct STimelineStep
    {
        ARenderTask * RenderTask;
        int FirstRealizedResource;
        int NumRealizedResources;
        int FirstDerealizedResource;
        int NumDerealizedResources;
    };

    TPodVector< STimelineStep > Timeline;
    TPodVector< AFrameGraphResourceProxy * > RealizedResources, DerealizedResources;

    THash<> FramebufferHash;
    TStdVector< TRef< RenderCore::IFramebuffer > > FramebufferCache;
    TPodVector< int > FramebufferHashes;

    // Temporary data. Used for building
    TPodStack< AFrameGraphResourceProxy * > UnreferencedResources;
    TPodVector< AFrameGraphResourceProxy * > ResourcesRW;

    mutable std::size_t IdGenerator = 0;
};

AN_FORCEINLINE AFrameGraphResourceProxy::AFrameGraphResourceProxy( AFrameGraph & InFrameGraph, const char * InName, ARenderTask * InRenderTask )
    : Id( InFrameGraph.GenerateResourceId() )
    , Name( InName )
    , Creator( InRenderTask )
    , ResourceRefs( 0 )
    , bCaptured( false )
{
}
