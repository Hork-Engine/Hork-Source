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

#include "VirtualTexturePhysCache.h"
#include "VirtualTexture.h"
#include "QuadTree.h"

#include "../RenderLocal.h"

#include <Platform/Platform.h>

#define PAGE_STREAM_PBO

using namespace RenderCore;

ARuntimeVariable r_ResetCacheVT(_CTS("r_ResetCacheVT"), _CTS("0"));

AVirtualTextureCache::AVirtualTextureCache(SVirtualTextureCacheCreateInfo const& CreateInfo)
{
    AN_ASSERT(CreateInfo.PageResolutionB > VT_PAGE_BORDER_WIDTH * 2 && CreateInfo.PageResolutionB <= 512);

    PageResolutionB = CreateInfo.PageResolutionB;

    uint32_t maxPageCacheCapacity = GDevice->GetDeviceCaps(DEVICE_CAPS_MAX_TEXTURE_SIZE) / PageResolutionB;

    PageCacheCapacityX = Math::Clamp(CreateInfo.PageCacheCapacityX, MIN_PAGE_CACHE_CAPACITY, maxPageCacheCapacity);
    PageCacheCapacityY = Math::Clamp(CreateInfo.PageCacheCapacityY, MIN_PAGE_CACHE_CAPACITY, maxPageCacheCapacity);

    PageCacheCapacity = PageCacheCapacityX * PageCacheCapacityY;
    if (PageCacheCapacity > 4096)
    {
        PageCacheCapacityX = PageCacheCapacityY = 64;
        PageCacheCapacity                       = 4096;
    }

    PhysPageInfo.Resize(PageCacheCapacity);
    PhysPageInfoSorted.Resize(PageCacheCapacity);

    for (int i = 0; i < PageCacheCapacity; i++)
    {
        PhysPageInfo[i].Time        = 0;
        PhysPageInfo[i].PageIndex   = 0;
        PhysPageInfo[i].pTexture    = 0;
        PhysPageInfoSorted[i].pInfo = &PhysPageInfo[i];
    }

    int physCacheWidth  = PageCacheCapacityX * PageResolutionB;
    int physCacheHeight = PageCacheCapacityY * PageResolutionB;

    PageSizeInBytes = 0;
    AlignedSize     = 0;

    PhysCacheLayers.resize(CreateInfo.NumLayers);
    LayerInfo.resize(CreateInfo.NumLayers);
    for (int i = 0; i < CreateInfo.NumLayers; i++)
    {
        GDevice->CreateTexture(STextureDesc()
                                   .SetFormat(CreateInfo.pLayers[i].TextureFormat)
                                   .SetResolution(STextureResolution2D(physCacheWidth, physCacheHeight))
                                   .SetBindFlags(BIND_SHADER_RESOURCE),
                               &PhysCacheLayers[i]);
        PhysCacheLayers[i]->SetDebugName("Virtual texture phys cache layer");
        LayerInfo[i] = CreateInfo.pLayers[i];

        size_t size = LayerInfo[i].PageSizeInBytes; //ITexture::GetPixelFormatSize( PageResolutionB, PageResolutionB, LayerInfo[i].UploadFormat );
        PageSizeInBytes += size;

        AlignedSize += Align(size, 16);
    }

    TotalCachedPages = 0;

    LRUTime = 0;

    PageTranslationOffsetAndScale.X = (float)VT_PAGE_BORDER_WIDTH / PageResolutionB / PageCacheCapacityX;
    PageTranslationOffsetAndScale.Y = (float)VT_PAGE_BORDER_WIDTH / PageResolutionB / PageCacheCapacityY;
    PageTranslationOffsetAndScale.Z = (float)(PageResolutionB - VT_PAGE_BORDER_WIDTH * 2) / PageResolutionB / PageCacheCapacityX;
    PageTranslationOffsetAndScale.W = (float)(PageResolutionB - VT_PAGE_BORDER_WIDTH * 2) / PageResolutionB / PageCacheCapacityY;

    SPipelineResourceLayout resourceLayout;

    SSamplerDesc nearestSampler;
    nearestSampler.Filter   = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &nearestSampler;

    CreateFullscreenQuadPipeline(&DrawCachePipeline, "drawvtcache.vert", "drawvtcache.frag", &resourceLayout);

#ifdef PAGE_STREAM_PBO
    SBufferDesc bufferCI           = {};
    bufferCI.bImmutableStorage     = true;
    bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)(
        IMMUTABLE_MAP_WRITE | IMMUTABLE_MAP_PERSISTENT | IMMUTABLE_MAP_COHERENT);

    bufferCI.SizeInBytes = AlignedSize * MAX_UPLOADS_PER_FRAME;

    GLogger.Printf("Virtual texture cache transfer buffer size: %d kb\n", bufferCI.SizeInBytes >> 10);

    GDevice->CreateBuffer(bufferCI, nullptr, &TransferBuffer);
    TransferBuffer->SetDebugName("Virtual texture page transfer buffer");

    pTransferData = (byte*)rcmd->MapBuffer(TransferBuffer,
                                           MAP_TRANSFER_WRITE,
                                           MAP_INVALIDATE_ENTIRE_BUFFER,
                                           MAP_PERSISTENT_COHERENT,
                                           false,
                                           false);
    AN_ASSERT(IsAlignedPtr(pTransferData, 16));

    TransferDataOffset = 0;
    TransferAllocPoint = 0;
    TransferFreePoint.Store(MAX_UPLOADS_PER_FRAME);

    for (int i = 0; i < MAX_UPLOADS_PER_FRAME; i++)
    {
        PageTransfer[i].Fence  = nullptr;
        PageTransfer[i].Offset = AlignedSize * i;
    }
#endif
}

AVirtualTextureCache::~AVirtualTextureCache()
{
#ifdef PAGE_STREAM_PBO
    rcmd->UnmapBuffer(TransferBuffer);
#endif

    if (LockTransfers())
    {
        for (SPageTransfer* transfer : Transfers)
        {
            transfer->pTexture->RemoveRef();
        }
        UnlockTransfers();
    }

    for (SPageTransfer* transfer = PageTransfer; transfer < &PageTransfer[MAX_UPLOADS_PER_FRAME]; transfer++)
    {
        rcmd->RemoveSync(transfer->Fence);
    }

    for (AVirtualTexture* texture : VirtualTextures)
    {
        texture->RemoveRef();
    }
}

bool AVirtualTextureCache::CreateTexture(const char* FileName, TRef<AVirtualTexture>* ppTexture)
{
    ppTexture->Reset();

    TRef<AVirtualTexture> pTexture = MakeRef<AVirtualTexture>(FileName, this);
    if (!pTexture->IsLoaded())
    {
        return false;
    }

    *ppTexture = pTexture;

    pTexture->AddRef();

    VirtualTextures.Append(pTexture.GetObject());

    return true;
}

//void AVirtualTextureCache::DestroyTexture( TRef< AVirtualTexture > * ppTexture ) {
//    AVirtualTexture * pTexture = *ppTexture;

//    AN_ASSERT( pTexture && pTexture->pCache == this );

//    if ( !pTexture || pTexture->pCache != this ) {
//        GLogger.Printf( "AVirtualTextureCache::DestroyTexture: invalid texture\n" );
//        return;
//    }

//    pTexture->bPendingRemove = true;
//}

AVirtualTextureCache::SPageTransfer* AVirtualTextureCache::CreatePageTransfer()
{
    AN_ASSERT(LayerInfo.size() > 0);

    // TODO: break if thread was stopped
    do {
        int freePoint = TransferFreePoint.Load();

        if (TransferAllocPoint + 1 <= freePoint)
        {
            size_t allocPoint = TransferAllocPoint % MAX_UPLOADS_PER_FRAME;
            size_t offset     = allocPoint * AlignedSize;

            SPageTransfer* transfer = &PageTransfer[allocPoint];

            for (int i = 0; i < LayerInfo.size(); i++)
            {
                transfer->Layers[i] = pTransferData + offset;
                offset += Align(LayerInfo[i].PageSizeInBytes, 16);
            }

            TransferAllocPoint++;
            return transfer;
        }

        PageTransferEvent.Wait();
    } while (1);
}

void AVirtualTextureCache::MakePageTransferVisible(SPageTransfer* Transfer)
{
    AMutexGurad criticalSection(TransfersMutex);
    Transfers.Append(Transfer);
}

bool AVirtualTextureCache::LockTransfers()
{
    TransfersMutex.Lock();
    if (Transfers.IsEmpty())
    {
        TransfersMutex.Unlock();
        return false;
    }
    return true;
}

void AVirtualTextureCache::UnlockTransfers()
{
    Transfers.Clear();
    TransfersMutex.Unlock();
}

void AVirtualTextureCache::ResetCache()
{
    TotalCachedPages = 0;
    LRUTime          = 0;

    for (int i = 0; i < PageCacheCapacity; i++)
    {
        if (PhysPageInfo[i].pTexture)
        {
            PhysPageInfo[i].pTexture->MakePageNonResident(PhysPageInfo[i].PageIndex);
        }
        PhysPageInfo[i].Time        = 0;
        PhysPageInfo[i].PageIndex   = 0;
        PhysPageInfo[i].pTexture    = 0;
        PhysPageInfoSorted[i].pInfo = &PhysPageInfo[i];
    }

    for (AVirtualTexturePtr texture : VirtualTextures)
    {
        texture->PendingUpdateLRU.Clear();
        texture->CommitPageResidency();
    }
}

void AVirtualTextureCache::Update()
{
    static int maxPendingLRUs = 0;

    if (r_ResetCacheVT)
    {
        ResetCache();
        r_ResetCacheVT = false;
    }

    WaitForFences();

    if (!LockTransfers())
    {
        // no pages to upload
        for (AVirtualTexturePtr texture : VirtualTextures)
        {
            maxPendingLRUs = Math::Max(maxPendingLRUs, texture->PendingUpdateLRU.Size());
            texture->PendingUpdateLRU.Clear(); // No need to update LRU
        }
        //GLogger.Printf( "maxPendingLRUs %d\n", maxPendingLRUs );
        return;
    }

    // Update LRU

    int64_t time = ++LRUTime;

    for (AVirtualTexturePtr texture : VirtualTextures)
    {
        maxPendingLRUs = Math::Max(maxPendingLRUs, texture->PendingUpdateLRU.Size());
        for (int i = 0; i < texture->PendingUpdateLRU.Size(); i++)
        {
            uint32_t absIndex = texture->PendingUpdateLRU[i];

            AN_ASSERT(texture->PIT[absIndex] & PF_CACHED);

            const uint16_t* pageIndirection = texture->GetIndirectionData();

            int offset                = pageIndirection[absIndex] & 0x0fff;
            PhysPageInfo[offset].Time = time;
        }
        texture->PendingUpdateLRU.Clear();
    }

    //GLogger.Printf( "maxPendingLRUs %d\n", maxPendingLRUs );

    int numFirstReservedPages = 0; //1;  // first lod always must be in cache
    int currentCacheCapacity  = Math::Min<int>(PageCacheCapacity - numFirstReservedPages, Transfers.Size());

    SPhysPageInfoSorted* pFirstPhysPage = &PhysPageInfoSorted[numFirstReservedPages];

    if (TotalCachedPages < PageCacheCapacity)
    {
        pFirstPhysPage = &PhysPageInfoSorted[TotalCachedPages];
    }
    else
    {
        // Sort cache info by time to move outdated pages to begin of the array
        struct
        {
            bool operator()(SPhysPageInfoSorted const& a, SPhysPageInfoSorted const& b)
            {
                return a.pInfo->Time < b.pInfo->Time;
            }
        } PhysicalPageTimeCompare;
        std::sort(pFirstPhysPage, PhysPageInfoSorted.End(), PhysicalPageTimeCompare);
    }

    SPhysPageInfoSorted* pLastPhysPage = pFirstPhysPage + currentCacheCapacity;

    int d_duplicates = 0; // Count of double streamed pages (for debugging)
    int d_uploaded   = 0; // Count of uploaded pages (for debugging)

    int fetchIndex = 0;

    int64_t uploadStartTime = Platform::SysMicroseconds();

    for (SPhysPageInfoSorted* physPage = pFirstPhysPage;
         physPage < pLastPhysPage && fetchIndex < Transfers.Size(); ++fetchIndex)
    {
        SPageTransfer* transfer = Transfers[fetchIndex];

        AVirtualTexture* pTexture = transfer->pTexture;

        if (pTexture->PIT[transfer->PageIndex] & PF_CACHED)
        {
            // Page is loaded twice.
            d_duplicates++;
            DiscardTransfers(&transfer, 1);
            continue;
        }

        // Clear space for the page
        if (physPage->pInfo->pTexture)
        {
            if (physPage->pInfo->Time + 4 >= time)
            {
                GLogger.Printf("AVirtualTextureCache::UploadPages: texture cache thrashing\n");
                // TODO: move uploaded pages to temporary memory for fast re-upload later
                DiscardTransfers(&Transfers[fetchIndex], Transfers.Size() - fetchIndex);
                break;
            }

            // debug: invalid case if physical page reserved for lod0
            //if ( pageIndex == 0 ) {
            //    si++;
            //    continue;
            //}

            physPage->pInfo->pTexture->MakePageNonResident(physPage->pInfo->PageIndex);
        }

        physPage->pInfo->Time      = time;
        physPage->pInfo->PageIndex = transfer->PageIndex;
        physPage->pInfo->pTexture  = pTexture;

        // get new cached page index
        int physPageIndex = physPage->pInfo - PhysPageInfo.ToPtr();
        AN_ASSERT(physPageIndex < PageCacheCapacity);

        TransferPageData(transfer, physPageIndex);

        pTexture->MakePageResident(transfer->PageIndex, physPageIndex);
        pTexture->RemoveRef();

        physPage++;
        d_uploaded++;
        TotalCachedPages++;
    }

    if (d_duplicates > 0)
    {
        GLogger.Printf("Double streamed %d times\n", d_duplicates);
    }

    GLogger.Printf("Streamed per frame %d, uploaded %d, time %d microsec\n", Transfers.Size(), d_uploaded, Platform::SysMicroseconds() - uploadStartTime);

    UnlockTransfers();

    for (int texIndex = VirtualTextures.Size() - 1; texIndex >= 0; texIndex--)
    {
        AVirtualTexturePtr texture = VirtualTextures[texIndex];

        texture->CommitPageResidency();

        if (texture->GetRefCount() == 1)
        {

            // Remove texture from the cache
            for (int i = 0; i < PageCacheCapacity; i++)
            {
                if (PhysPageInfo[i].pTexture == texture)
                {
                    PhysPageInfo[i].pTexture->MakePageNonResident(PhysPageInfo[i].PageIndex);
                }
                PhysPageInfo[i].Time      = 0;
                PhysPageInfo[i].PageIndex = 0;
                PhysPageInfo[i].pTexture  = 0;
            }

            texture->RemoveRef();

            VirtualTextures.Remove(texIndex);
        }
    }
}

void AVirtualTextureCache::TransferPageData(SPageTransfer* Transfer, int PhysPageIndex)
{
    int offsetX = PhysPageIndex & (PageCacheCapacityX - 1);
    int offsetY = PhysPageIndex / PageCacheCapacityX;

    STextureRect rect;

    rect.Offset.MipLevel = 0;
    rect.Offset.X        = offsetX * PageResolutionB;
    rect.Offset.Y        = offsetY * PageResolutionB;
    rect.Offset.Z        = 0;

    rect.Dimension.X = PageResolutionB;
    rect.Dimension.Y = PageResolutionB;
    rect.Dimension.Z = 1;

    size_t offset = Transfer->Offset;

    for (int layerIndex = 0; layerIndex < PhysCacheLayers.size(); layerIndex++)
    {
#ifdef PAGE_STREAM_PBO
        rcmd->CopyBufferToTexture(TransferBuffer, PhysCacheLayers[layerIndex], rect, LayerInfo[layerIndex].UploadFormat, 0, offset, 1);

        offset += Align(LayerInfo[layerIndex].PageSizeInBytes, 16);
#else
        PhysCacheLayers[layerIndex]->WriteRect(rect,
                                               LayerInfo[layerIndex].UploadFormat,
                                               LayerInfo[layerIndex].PageSizeInBytes,
                                               1,
                                               Page->Layers[layerIndex]);
#endif
    }

    WaitForFences();

    Transfer->Fence = rcmd->FenceSync();
}

void AVirtualTextureCache::DiscardTransfers(SPageTransfer** InTransfers, int Count)
{
    if (Count > 0)
    {
        SyncObject Fence = rcmd->FenceSync();

        for (int i = 0; i < Count; i++)
        {
            InTransfers[i]->Fence = Fence;
            InTransfers[i]->pTexture->RemoveRef();
        }
    }
}

void AVirtualTextureCache::WaitForFences()
{
    const uint64_t timeOutNanoseconds = 1;

    int freePoint = TransferFreePoint.Load();
    for (int i = 0; i < MAX_UPLOADS_PER_FRAME; i++)
    {
        freePoint = freePoint % MAX_UPLOADS_PER_FRAME;
        if (!PageTransfer[freePoint].Fence)
        {
            break;
        }

        CLIENT_WAIT_STATUS status = rcmd->ClientWait(PageTransfer[freePoint].Fence, timeOutNanoseconds);

        if (status == CLIENT_WAIT_ALREADY_SIGNALED || status == CLIENT_WAIT_CONDITION_SATISFIED)
        {
            rcmd->RemoveSync(PageTransfer[freePoint].Fence);
            PageTransfer[freePoint].Fence = nullptr;
            freePoint                     = TransferFreePoint.Increment();
            PageTransferEvent.Signal();
        }
        else
        {
            break;
        }
    }
}

void AVirtualTextureCache::Draw(AFrameGraph& FrameGraph, FGTextureProxy* RenderTarget, int LayerIndex)
{
    if (LayerIndex < 0 || LayerIndex >= PhysCacheLayers.size())
    {
        return;
    }

    ITexture* texture = PhysCacheLayers[LayerIndex];

    FGTextureProxy* CacheTexture_R = FrameGraph.AddExternalResource<FGTextureProxy>("VT Cache", texture);

    ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("VT Draw Cache");

    float scale = texture->GetWidth() != 0 ? (float)GRenderView->Width / texture->GetWidth() : 0.0f;

    pass.SetRenderArea((float)texture->GetWidth() * scale * 0.5f,
                       (float)texture->GetHeight() * scale * 0.5f);

    pass.AddResource(CacheTexture_R, FG_RESOURCE_ACCESS_READ);

    pass.SetColorAttachment(
        STextureAttachment(RenderTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    pass.AddSubpass({0}, // color attachment refs
                    [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, CacheTexture_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, DrawCachePipeline);
                    });
}
