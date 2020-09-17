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

#include "VirtualTexturePhysCache.h"
#include "VirtualTexture.h"
#include "QuadTree.h"

#include "../RenderCommon.h"

#include <Runtime/Public/Runtime.h>

#define PAGE_STREAM_PBO

ARuntimeVariable r_ResetCacheVT( _CTS("r_ResetCacheVT"), _CTS("0") );

AVirtualTextureCache::AVirtualTextureCache( SVirtualTextureCacheCreateInfo const & CreateInfo )
{
    using namespace RenderCore;

    AN_ASSERT( CreateInfo.PageResolutionB > VT_PAGE_BORDER_WIDTH * 2 && CreateInfo.PageResolutionB <= 512 );

    PageResolutionB = CreateInfo.PageResolutionB;

    uint32_t maxPageCacheCapacity = GDevice->GetDeviceCaps( DEVICE_CAPS_MAX_TEXTURE_SIZE ) / PageResolutionB;

    PageCacheCapacityX = Math::Clamp( CreateInfo.PageCacheCapacityX, MIN_PAGE_CACHE_CAPACITY, maxPageCacheCapacity );
    PageCacheCapacityY = Math::Clamp( CreateInfo.PageCacheCapacityY, MIN_PAGE_CACHE_CAPACITY, maxPageCacheCapacity );

    PageCacheCapacity = PageCacheCapacityX * PageCacheCapacityY;
    if ( PageCacheCapacity > 4096 ) {
        PageCacheCapacityX = PageCacheCapacityY = 64;
        PageCacheCapacity = 4096;
    }

    PageCacheInfo.Resize( PageCacheCapacity );
    SortedCacheInfo.Resize( PageCacheCapacity );

    for ( int i = 0 ; i < PageCacheCapacity ; i++ ) {
        PageCacheInfo[i].Time = 0;
        PageCacheInfo[i].PageIndex = 0;
        PageCacheInfo[i].pTexture = 0;
        SortedCacheInfo[i].pInfo = &PageCacheInfo[i];
    }

    int physCacheWidth = PageCacheCapacityX * PageResolutionB;
    int physCacheHeight = PageCacheCapacityY * PageResolutionB;

    PageSizeInBytes = 0;
    AlignedSize = 0;

    PhysCacheLayers.resize( CreateInfo.NumLayers );
    LayerInfo.resize( CreateInfo.NumLayers );
    for ( int i = 0 ; i < CreateInfo.NumLayers ; i++ ) {
        GDevice->CreateTexture( MakeTexture( CreateInfo.pLayers[i].TextureFormat, STextureResolution2D( physCacheWidth, physCacheHeight ) ), &PhysCacheLayers[i] );
        LayerInfo[i] = CreateInfo.pLayers[i];

        size_t size = LayerInfo[i].PageSizeInBytes;//RenderCore::ITexture::GetPixelFormatSize( PageResolutionB, PageResolutionB, LayerInfo[i].UploadFormat );
        PageSizeInBytes += size;

        AlignedSize += Align( size, 16 );
    }

    FilledPages = 0;

    LRUTime = 0;

    PageTranslationOffsetAndScale.X = (float)VT_PAGE_BORDER_WIDTH / PageResolutionB / PageCacheCapacityX;
    PageTranslationOffsetAndScale.Y = (float)VT_PAGE_BORDER_WIDTH / PageResolutionB / PageCacheCapacityY;
    PageTranslationOffsetAndScale.Z = (float)(PageResolutionB - VT_PAGE_BORDER_WIDTH*2) / PageResolutionB / PageCacheCapacityX;
    PageTranslationOffsetAndScale.W = (float)(PageResolutionB - VT_PAGE_BORDER_WIDTH*2) / PageResolutionB / PageCacheCapacityY;

    SPipelineResourceLayout resourceLayout;

    SSamplerInfo nearestSampler;
    nearestSampler.Filter = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &nearestSampler;

    CreateFullscreenQuadPipeline( &DrawCachePipeline, "drawvtcache.vert", "drawvtcache.frag", &resourceLayout );

#ifdef PAGE_STREAM_PBO
    RenderCore::SBufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)(
        RenderCore::IMMUTABLE_MAP_WRITE
        | RenderCore::IMMUTABLE_MAP_PERSISTENT
        | RenderCore::IMMUTABLE_MAP_COHERENT
        );
    
    bufferCI.SizeInBytes = AlignedSize * MAX_UPLOADS_PER_FRAME;

    GLogger.Printf( "Virtual texture cache transfer buffer size: %d kb\n", bufferCI.SizeInBytes >> 10 );

    GDevice->CreateBuffer( bufferCI, nullptr, &TransferBuffer );

    pTransferData = (byte *)TransferBuffer->Map( RenderCore::MAP_TRANSFER_WRITE,
                                                 RenderCore::MAP_INVALIDATE_ENTIRE_BUFFER,
                                                 RenderCore::MAP_PERSISTENT_COHERENT,
                                                 false,
                                                 false );
    AN_ASSERT( IsAlignedPtr( pTransferData, 16 ) );

    TransferDataOffset = 0;
    TransferAllocPoint = 0;
    TransferFreePoint.Store( MAX_UPLOADS_PER_FRAME );

    for ( int i = 0 ; i < MAX_UPLOADS_PER_FRAME ; i++ ) {
        PageTransfer[i].Fence = nullptr;
        PageTransfer[i].Offset = AlignedSize * i;
    }
#endif
}

AVirtualTextureCache::~AVirtualTextureCache()
{
#ifdef PAGE_STREAM_PBO
    TransferBuffer->Unmap();
#endif

    for ( SPageTransfer * transfer = PageTransfer ; transfer < &PageTransfer[MAX_UPLOADS_PER_FRAME] ; transfer++ ) {
        rcmd->RemoveSync( transfer->Fence );
    }

    for ( AVirtualTexture * texture : VirtualTextures ) {
        delete texture;
    }
}

AVirtualTexture * AVirtualTextureCache::CreateVirtualTexture( const char * FileName )
{
    AVirtualTexture * texture = new AVirtualTexture( FileName, this );

    if ( !texture->IsLoaded() ) {
        delete texture;
        return nullptr;
    }

    VirtualTextures.Append( texture );

    return texture;
}

void AVirtualTextureCache::RemoveVirtualTexture( AVirtualTexture * Texture ) {
    AN_ASSERT( Texture->pCache == this );

    if ( Texture->pCache != this ) {
        GLogger.Printf( "AVirtualTextureCache::RemoveVirtualTexture: invalid texture\n" );
        return;
    }

    if ( !Texture->bPendingRemove ) {
        Texture->bPendingRemove = true;
        Texture->RemoveRef();
    }
}

AVirtualTextureCache::SPageTransfer * AVirtualTextureCache::CreatePageTransfer()
{
    AN_ASSERT( LayerInfo.size() > 0 );

    // TODO: break if thread was stopped
    do {
        int freePoint = TransferFreePoint.Load();

        if ( TransferAllocPoint + 1 <= freePoint ) {
            size_t allocPoint = TransferAllocPoint % MAX_UPLOADS_PER_FRAME;
            size_t offset = allocPoint * AlignedSize;

            SPageTransfer * transfer = &PageTransfer[allocPoint];

            for ( int i = 0 ; i < LayerInfo.size() ; i++ ) {        
                transfer->Layers[i] = pTransferData + offset;
                offset += Align( LayerInfo[i].PageSizeInBytes, 16 );
            }

            TransferAllocPoint++;
            return transfer;
        }

        PageTransferEvent.Wait();
    }
    while ( 1 );
}

void AVirtualTextureCache::MakePageTransferVisible( SPageTransfer * Transfer )
{
    ASyncGuard criticalSection( UploadMutex );
    Uploads.Append( Transfer );
}

bool AVirtualTextureCache::LockUploads()
{
    UploadMutex.BeginScope();
    if ( Uploads.IsEmpty() ) {
        UploadMutex.EndScope();
        return false;
    }
    return true;
}

void AVirtualTextureCache::UnlockUploads()
{
    Uploads.Clear();
    UploadMutex.EndScope();
}

void AVirtualTextureCache::ResetCache() {
    FilledPages = 0;
    LRUTime = 0;

    for ( int i = 0 ; i < PageCacheCapacity ; i++ ) {
        if ( PageCacheInfo[i].pTexture ) {
            PageCacheInfo[i].pTexture->MakePageNonResident( PageCacheInfo[i].PageIndex );
        }
        PageCacheInfo[i].Time = 0;
        PageCacheInfo[i].PageIndex = 0;
        PageCacheInfo[i].pTexture = 0;
        SortedCacheInfo[i].pInfo = &PageCacheInfo[i];
    }

    for ( AVirtualTexturePtr texture : VirtualTextures ) {
        texture->PendingUpdateLRU.Clear();
        texture->CommitPageResidency();
    }
}

void AVirtualTextureCache::Update() {
    static int maxPendingLRUs = 0;

    if ( r_ResetCacheVT ) {
        ResetCache();
        r_ResetCacheVT = false;
    }

    WaitForFences();

    if ( !LockUploads() ) {
        // no pages to upload
        for ( AVirtualTexturePtr texture : VirtualTextures ) {
            maxPendingLRUs = Math::Max( maxPendingLRUs, texture->PendingUpdateLRU.Size() );
            texture->PendingUpdateLRU.Clear(); // No need to update LRU
        }
        //GLogger.Printf( "maxPendingLRUs %d\n", maxPendingLRUs );
        return;
    }

    // Update LRU

    int64_t time = ++LRUTime;

    for ( AVirtualTexturePtr texture : VirtualTextures ) {
        maxPendingLRUs = Math::Max( maxPendingLRUs, texture->PendingUpdateLRU.Size() );
        for ( int i = 0 ; i < texture->PendingUpdateLRU.Size() ; i++ ) {
            uint32_t absIndex = texture->PendingUpdateLRU[i];

            AN_ASSERT( texture->PIT[absIndex] & PF_CACHED );

            const uint16_t * pageIndirection = texture->GetIndirectionData();

            int offset = pageIndirection[absIndex] & 0x0fff;
            PageCacheInfo[offset].Time = time;
        }
        texture->PendingUpdateLRU.Clear();
    }

    //GLogger.Printf( "maxPendingLRUs %d\n", maxPendingLRUs );

    int numFirstReservedPages = 0;//1;  // first lod always must be in cache

    SSortedCacheInfo * sortedCachePtr = &SortedCacheInfo[numFirstReservedPages];

    if ( FilledPages < PageCacheCapacity ) {
        sortedCachePtr = &SortedCacheInfo[FilledPages];
    } else {
        // Sort cache info by time to move outdated pages to begin of the array
        struct
        {
            bool operator() ( SSortedCacheInfo const & a, SSortedCacheInfo const & b ) {
                return a.pInfo->Time < b.pInfo->Time;
            }
        } CacheInfoCompare;
        std::sort( sortedCachePtr, SortedCacheInfo.End(), CacheInfoCompare );
    }

    int currentCacheCapacity = Math::Min< int >( PageCacheCapacity - numFirstReservedPages, Uploads.Size() );
    int duplicates = 0; // Count of double streamed pages (just for debugging)
    int uploaded = 0; // Count of uploaded pages (just for debugging)
    int uploadIndex = 0;

    int64_t uploadStartTime = GRuntime.SysMicroseconds();

    for ( SSortedCacheInfo *si = sortedCachePtr ;
          si < &sortedCachePtr[currentCacheCapacity] && uploadIndex < Uploads.Size() ; ++uploadIndex )
    {
        SPageTransfer * transfer = Uploads[uploadIndex];

        AVirtualTexture * pTexture = transfer->pTexture;

        if ( pTexture->PIT[transfer->PageIndex] & PF_CACHED ) {
            // Page is loaded twice.
            duplicates++;
            DiscardTransfers( &transfer, 1 );
            continue;
        }

        // Clear space for the page
        if ( si->pInfo->pTexture ) {
            if ( si->pInfo->Time + 4 >= time ) {
                GLogger.Printf( "AVirtualTextureCache::UploadPages: texture cache thrashing\n" );
                // TODO: move uploaded pages to temporary memory for fast re-upload later
                DiscardTransfers( &Uploads[uploadIndex], Uploads.Size() - uploadIndex );
                break;
            }

            // debug: invalid case if cell reserved for lod0
            //if ( pageIndex == 0 ) {
            //    si++;
            //    continue;
            //}

            si->pInfo->pTexture->MakePageNonResident( si->pInfo->PageIndex );
        }

        si->pInfo->Time = time;
        si->pInfo->PageIndex = transfer->PageIndex;
        si->pInfo->pTexture = pTexture;

        // get new cached page index
        int cacheCellIndex = si->pInfo - PageCacheInfo.ToPtr();
        AN_ASSERT( cacheCellIndex < PageCacheCapacity );

        TransferPageData( transfer, cacheCellIndex );
        
        pTexture->MakePageResident( transfer->PageIndex, cacheCellIndex );
        pTexture->RemoveRef();

        si++;
        uploaded++;
        FilledPages++;
    }

    if ( duplicates > 0 ) {
        GLogger.Printf( "Double streamed %d times\n", duplicates );
    }

    GLogger.Printf( "Streamed per frame %d, uploaded %d, time %d microsec\n", Uploads.Size(), uploaded, GRuntime.SysMicroseconds() - uploadStartTime );

    UnlockUploads();

    for ( int texIndex = VirtualTextures.Size() - 1 ; texIndex >= 0 ; texIndex-- ) {
        AVirtualTexturePtr texture = VirtualTextures[texIndex];

        texture->CommitPageResidency();

        if ( texture->GetRefCount() == 0 ) {

            // Remove texture from the cache
            for ( int i = 0 ; i < PageCacheCapacity ; i++ ) {
                if ( PageCacheInfo[i].pTexture == texture ) {
                    PageCacheInfo[i].pTexture->MakePageNonResident( PageCacheInfo[i].PageIndex );
                }
                PageCacheInfo[i].Time = 0;
                PageCacheInfo[i].PageIndex = 0;
                PageCacheInfo[i].pTexture = 0;
            }

            delete texture;

            VirtualTextures.Remove( texIndex );
        }
    }
}

void AVirtualTextureCache::TransferPageData( SPageTransfer * Transfer, int CacheCellIndex )
{
    int offsetX = CacheCellIndex & (PageCacheCapacityX-1);
    int offsetY = CacheCellIndex / PageCacheCapacityX;

    RenderCore::STextureRect rect;

    rect.Offset.Lod = 0;
    rect.Offset.X = offsetX * PageResolutionB;
    rect.Offset.Y = offsetY * PageResolutionB;
    rect.Offset.Z = 0;

    rect.Dimension.X = PageResolutionB;
    rect.Dimension.Y = PageResolutionB;
    rect.Dimension.Z = 1;

    size_t offset = Transfer->Offset;

    for ( int layerIndex = 0 ; layerIndex < PhysCacheLayers.size() ; layerIndex++ ) {
#ifdef PAGE_STREAM_PBO
        rcmd->CopyBufferToTexture( TransferBuffer, PhysCacheLayers[layerIndex], rect, LayerInfo[layerIndex].UploadFormat, 0, offset, 1 );

        offset += Align( LayerInfo[layerIndex].PageSizeInBytes, 16 );
#else
        PhysCacheLayers[layerIndex]->WriteRect( rect,
                                                LayerInfo[layerIndex].UploadFormat,
                                                LayerInfo[layerIndex].PageSizeInBytes,
                                                1,
                                                Page->Layers[layerIndex] );
#endif
    }

    WaitForFences();

    Transfer->Fence = rcmd->FenceSync();
}

void AVirtualTextureCache::DiscardTransfers( SPageTransfer ** Transfers, int Count )
{
    if ( Count > 0 ) {
        RenderCore::SyncObject Fence = rcmd->FenceSync();

        for ( int i = 0 ; i < Count ; i++ ) {
            Transfers[i]->Fence = Fence;
            Transfers[i]->pTexture->RemoveRef();
        }
    }
}

void AVirtualTextureCache::WaitForFences()
{
    const uint64_t timeOutNanoseconds = 1;

    int freePoint = TransferFreePoint.Load();
    for ( int i = 0 ; i < MAX_UPLOADS_PER_FRAME ; i++ ) {
        freePoint = freePoint % MAX_UPLOADS_PER_FRAME;
        if ( !PageTransfer[freePoint].Fence ) {
            break;
        }

        RenderCore::CLIENT_WAIT_STATUS status = rcmd->ClientWait( PageTransfer[freePoint].Fence, timeOutNanoseconds );

        if ( status == RenderCore::CLIENT_WAIT_ALREADY_SIGNALED || status == RenderCore::CLIENT_WAIT_CONDITION_SATISFIED )
        {
            rcmd->RemoveSync( PageTransfer[freePoint].Fence );
            PageTransfer[freePoint].Fence = nullptr;
            freePoint = TransferFreePoint.Increment();
            PageTransferEvent.Signal();
        } else {
            break;
        }
    }
}

void AVirtualTextureCache::Draw( AFrameGraph & FrameGraph, AFrameGraphTexture * RenderTarget, int LayerIndex )
{
    if ( LayerIndex < 0 || LayerIndex >= PhysCacheLayers.size() ) {
        return;
    }

    RenderCore::ITexture * texture = PhysCacheLayers[LayerIndex];

    AFrameGraphTexture * CacheTexture_R = FrameGraph.AddExternalResource(
        "VT Cache",
        RenderCore::STextureCreateInfo(),
        texture
    );

    ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "VT Draw Cache" );

    float scale = (float)GRenderView->Width / texture->GetWidth();
    
    pass.SetRenderArea( (float)texture->GetWidth()*scale*0.5f,
                        (float)texture->GetHeight()*scale*0.5f );

    pass.AddResource( CacheTexture_R, RESOURCE_ACCESS_READ );

    pass.SetColorAttachments(
    {
        {
            RenderTarget,
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_LOAD )
        }
    }
    );

    pass.AddSubpass( { 0 }, // color attachment refs
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;
        rtbl->BindTexture( 0, CacheTexture_R->Actual() );

        DrawSAQ( DrawCachePipeline );
    } );
}
