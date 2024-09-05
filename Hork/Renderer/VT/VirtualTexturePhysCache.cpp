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

#include "VirtualTexturePhysCache.h"
#include "VirtualTexture.h"
#include "QuadTree.h"

#include "../RenderLocal.h"

#include <Hork/Core/Platform.h>

#define PAGE_STREAM_PBO

HK_NAMESPACE_BEGIN

using namespace RenderCore;

ConsoleVar r_ResetCacheVT("r_ResetCacheVT"_s, "0"_s);

VirtualTextureCache::VirtualTextureCache(VTCacheCreateInfo const& CreateInfo)
{
    HK_ASSERT(CreateInfo.PageResolutionB > VT_PAGE_BORDER_WIDTH * 2 && CreateInfo.PageResolutionB <= 512);

    m_PageResolutionB = CreateInfo.PageResolutionB;

    uint32_t maxPageCacheCapacity = GDevice->GetDeviceCaps(DEVICE_CAPS_MAX_TEXTURE_SIZE) / m_PageResolutionB;

    m_PageCacheCapacityX = Math::Clamp(CreateInfo.PageCacheCapacityX, MIN_PAGE_CACHE_CAPACITY, maxPageCacheCapacity);
    m_PageCacheCapacityY = Math::Clamp(CreateInfo.PageCacheCapacityY, MIN_PAGE_CACHE_CAPACITY, maxPageCacheCapacity);

    m_PageCacheCapacity = m_PageCacheCapacityX * m_PageCacheCapacityY;
    if (m_PageCacheCapacity > 4096)
    {
        m_PageCacheCapacityX = m_PageCacheCapacityY = 64;
        m_PageCacheCapacity = 4096;
    }

    m_PhysPageInfo.Resize(m_PageCacheCapacity);
    m_PhysPageInfoSorted.Resize(m_PageCacheCapacity);

    for (int i = 0; i < m_PageCacheCapacity; i++)
    {
        m_PhysPageInfo[i].Time = 0;
        m_PhysPageInfo[i].PageIndex = 0;
        m_PhysPageInfo[i].pTexture = 0;
        m_PhysPageInfoSorted[i].pInfo = &m_PhysPageInfo[i];
    }

    int physCacheWidth = m_PageCacheCapacityX * m_PageResolutionB;
    int physCacheHeight = m_PageCacheCapacityY * m_PageResolutionB;

    m_PageSizeInBytes = 0;
    m_AlignedSize = 0;

    m_PhysCacheLayers.Resize(CreateInfo.NumLayers);
    m_LayerInfo.Resize(CreateInfo.NumLayers);
    for (int i = 0; i < CreateInfo.NumLayers; i++)
    {
        GDevice->CreateTexture(TextureDesc()
                                   .SetFormat(CreateInfo.pLayers[i].TextureFormat)
                                   .SetResolution(TextureResolution2D(physCacheWidth, physCacheHeight))
                                   .SetBindFlags(BIND_SHADER_RESOURCE),
                               &m_PhysCacheLayers[i]);
        m_PhysCacheLayers[i]->SetDebugName("Virtual texture phys cache layer");
        m_LayerInfo[i] = CreateInfo.pLayers[i];

        size_t size = m_LayerInfo[i].PageSizeInBytes; //ITexture::GetPixelFormatSize( PageResolutionB, PageResolutionB, LayerInfo[i].UploadFormat );
        m_PageSizeInBytes += size;

        m_AlignedSize += Align(size, 16);
    }

    m_TotalCachedPages = 0;

    m_LRUTime = 0;

    m_PageTranslationOffsetAndScale.X = (float)VT_PAGE_BORDER_WIDTH / m_PageResolutionB / m_PageCacheCapacityX;
    m_PageTranslationOffsetAndScale.Y = (float)VT_PAGE_BORDER_WIDTH / m_PageResolutionB / m_PageCacheCapacityY;
    m_PageTranslationOffsetAndScale.Z = (float)(m_PageResolutionB - VT_PAGE_BORDER_WIDTH * 2) / m_PageResolutionB / m_PageCacheCapacityX;
    m_PageTranslationOffsetAndScale.W = (float)(m_PageResolutionB - VT_PAGE_BORDER_WIDTH * 2) / m_PageResolutionB / m_PageCacheCapacityY;

    PipelineResourceLayout resourceLayout;

    SamplerDesc nearestSampler;
    nearestSampler.Filter = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &nearestSampler;

    ShaderFactory::sCreateFullscreenQuadPipeline(&m_DrawCachePipeline, "drawvtcache.vert", "drawvtcache.frag", &resourceLayout);

#ifdef PAGE_STREAM_PBO
    BufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)(IMMUTABLE_MAP_WRITE | IMMUTABLE_MAP_PERSISTENT | IMMUTABLE_MAP_COHERENT);

    bufferCI.SizeInBytes = m_AlignedSize * MAX_UPLOADS_PER_FRAME;

    LOG("Virtual texture cache transfer buffer size: {} kb\n", bufferCI.SizeInBytes >> 10);

    GDevice->CreateBuffer(bufferCI, nullptr, &m_TransferBuffer);
    m_TransferBuffer->SetDebugName("Virtual texture page transfer buffer");

    m_pTransferData = (byte*)rcmd->MapBuffer(m_TransferBuffer,
                                           MAP_TRANSFER_WRITE,
                                           MAP_INVALIDATE_ENTIRE_BUFFER,
                                           MAP_PERSISTENT_COHERENT,
                                           false,
                                           false);
    HK_ASSERT(IsAlignedPtr(m_pTransferData, 16));

    m_TransferDataOffset = 0;
    m_TransferAllocPoint = 0;
    m_TransferFreePoint.Store(MAX_UPLOADS_PER_FRAME);

    for (int i = 0; i < MAX_UPLOADS_PER_FRAME; i++)
    {
        m_PageTransfer[i].Fence = nullptr;
        m_PageTransfer[i].Offset = m_AlignedSize * i;
    }
#endif
}

VirtualTextureCache::~VirtualTextureCache()
{
#ifdef PAGE_STREAM_PBO
    rcmd->UnmapBuffer(m_TransferBuffer);
#endif

    if (LockTransfers())
    {
        for (PageTransfer* transfer : m_Transfers)
        {
            transfer->pTexture->RemoveRef();
        }
        UnlockTransfers();
    }

    for (PageTransfer* transfer = m_PageTransfer; transfer < &m_PageTransfer[MAX_UPLOADS_PER_FRAME]; transfer++)
    {
        rcmd->RemoveSync(transfer->Fence);
    }

    for (VirtualTexture* texture : m_VirtualTextures)
    {
        texture->RemoveRef();
    }
}

bool VirtualTextureCache::CreateTexture(const char* FileName, Ref<VirtualTexture>* ppTexture)
{
    ppTexture->Reset();

    Ref<VirtualTexture> pTexture = MakeRef<VirtualTexture>(FileName, this);
    if (!pTexture->IsLoaded())
    {
        return false;
    }

    *ppTexture = pTexture;

    pTexture->AddRef();

    m_VirtualTextures.Add(pTexture.RawPtr());

    return true;
}

//void VirtualTextureCache::DestroyTexture( Ref< VirtualTexture > * ppTexture ) {
//    VirtualTexture * pTexture = *ppTexture;

//    HK_ASSERT( pTexture && pTexture->pCache == this );

//    if ( !pTexture || pTexture->pCache != this ) {
//        LOG( "VirtualTextureCache::DestroyTexture: invalid texture\n" );
//        return;
//    }

//    pTexture->bPendingRemove = true;
//}

VirtualTextureCache::PageTransfer* VirtualTextureCache::CreatePageTransfer()
{
    HK_ASSERT(m_LayerInfo.Size() > 0);

    // TODO: break if thread was stopped
    do {
        int freePoint = m_TransferFreePoint.Load();

        if (m_TransferAllocPoint + 1 <= freePoint)
        {
            size_t allocPoint = m_TransferAllocPoint % MAX_UPLOADS_PER_FRAME;
            size_t offset = allocPoint * m_AlignedSize;

            PageTransfer* transfer = &m_PageTransfer[allocPoint];

            for (int i = 0; i < m_LayerInfo.Size(); i++)
            {
                transfer->Layers[i] = m_pTransferData + offset;
                offset += Align(m_LayerInfo[i].PageSizeInBytes, 16);
            }

            m_TransferAllocPoint++;
            return transfer;
        }

        m_PageTransferEvent.Wait();
    } while (1);
}

void VirtualTextureCache::MakePageTransferVisible(PageTransfer* Transfer)
{
    MutexGuard criticalSection(m_TransfersMutex);
    m_Transfers.Add(Transfer);
}

bool VirtualTextureCache::LockTransfers()
{
    m_TransfersMutex.Lock();
    if (m_Transfers.IsEmpty())
    {
        m_TransfersMutex.Unlock();
        return false;
    }
    return true;
}

void VirtualTextureCache::UnlockTransfers()
{
    m_Transfers.Clear();
    m_TransfersMutex.Unlock();
}

void VirtualTextureCache::ResetCache()
{
    m_TotalCachedPages = 0;
    m_LRUTime = 0;

    for (int i = 0; i < m_PageCacheCapacity; i++)
    {
        if (m_PhysPageInfo[i].pTexture)
        {
            m_PhysPageInfo[i].pTexture->MakePageNonResident(m_PhysPageInfo[i].PageIndex);
        }
        m_PhysPageInfo[i].Time = 0;
        m_PhysPageInfo[i].PageIndex = 0;
        m_PhysPageInfo[i].pTexture = 0;
        m_PhysPageInfoSorted[i].pInfo = &m_PhysPageInfo[i];
    }

    for (VirtualTexturePtr texture : m_VirtualTextures)
    {
        texture->PendingUpdateLRU.Clear();
        texture->CommitPageResidency();
    }
}

void VirtualTextureCache::Update()
{
    static size_t maxPendingLRUs = 0;

    if (r_ResetCacheVT)
    {
        ResetCache();
        r_ResetCacheVT = false;
    }

    WaitForFences();

    if (!LockTransfers())
    {
        // no pages to upload
        for (VirtualTexturePtr texture : m_VirtualTextures)
        {
            maxPendingLRUs = Math::Max(maxPendingLRUs, texture->PendingUpdateLRU.Size());
            texture->PendingUpdateLRU.Clear(); // No need to update LRU
        }
        //LOG( "maxPendingLRUs {}\n", maxPendingLRUs );
        return;
    }

    // Update LRU

    int64_t time = ++m_LRUTime;

    for (VirtualTexturePtr texture : m_VirtualTextures)
    {
        maxPendingLRUs = Math::Max(maxPendingLRUs, texture->PendingUpdateLRU.Size());
        for (int i = 0; i < texture->PendingUpdateLRU.Size(); i++)
        {
            uint32_t absIndex = texture->PendingUpdateLRU[i];

            HK_ASSERT(texture->PIT[absIndex] & PF_CACHED);

            const uint16_t* pageIndirection = texture->GetIndirectionData();

            int offset = pageIndirection[absIndex] & 0x0fff;
            m_PhysPageInfo[offset].Time = time;
        }
        texture->PendingUpdateLRU.Clear();
    }

    //LOG( "maxPendingLRUs {}\n", maxPendingLRUs );

    int numFirstReservedPages = 0; //1;  // first lod always must be in cache
    int currentCacheCapacity = Math::Min<int>(m_PageCacheCapacity - numFirstReservedPages, m_Transfers.Size());

    PhysPageInfoSorted* pFirstPhysPage = &m_PhysPageInfoSorted[numFirstReservedPages];

    if (m_TotalCachedPages < m_PageCacheCapacity)
    {
        pFirstPhysPage = &m_PhysPageInfoSorted[m_TotalCachedPages];
    }
    else
    {
        // Sort cache info by time to move outdated pages to begin of the array
        struct
        {
            bool operator()(PhysPageInfoSorted const& a, PhysPageInfoSorted const& b)
            {
                return a.pInfo->Time < b.pInfo->Time;
            }
        } PhysicalPageTimeCompare;
        std::sort(pFirstPhysPage, m_PhysPageInfoSorted.End(), PhysicalPageTimeCompare);
    }

    PhysPageInfoSorted* pLastPhysPage = pFirstPhysPage + currentCacheCapacity;

    int d_duplicates = 0; // Count of double streamed pages (for debugging)
    int d_uploaded = 0;   // Count of uploaded pages (for debugging)

    int fetchIndex = 0;

    int64_t uploadStartTime = Core::SysMicroseconds();

    for (PhysPageInfoSorted* physPage = pFirstPhysPage;
         physPage < pLastPhysPage && fetchIndex < m_Transfers.Size(); ++fetchIndex)
    {
        PageTransfer* transfer = m_Transfers[fetchIndex];

        VirtualTexture* pTexture = transfer->pTexture;

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
                LOG("VirtualTextureCache::UploadPages: texture cache thrashing\n");
                // TODO: move uploaded pages to temporary memory for fast re-upload later
                DiscardTransfers(&m_Transfers[fetchIndex], m_Transfers.Size() - fetchIndex);
                break;
            }

            // debug: invalid case if physical page reserved for lod0
            //if ( pageIndex == 0 ) {
            //    si++;
            //    continue;
            //}

            physPage->pInfo->pTexture->MakePageNonResident(physPage->pInfo->PageIndex);
        }

        physPage->pInfo->Time = time;
        physPage->pInfo->PageIndex = transfer->PageIndex;
        physPage->pInfo->pTexture = pTexture;

        // get new cached page index
        int physPageIndex = physPage->pInfo - m_PhysPageInfo.ToPtr();
        HK_ASSERT(physPageIndex < m_PageCacheCapacity);

        TransferPageData(transfer, physPageIndex);

        pTexture->MakePageResident(transfer->PageIndex, physPageIndex);
        pTexture->RemoveRef();

        physPage++;
        d_uploaded++;
        m_TotalCachedPages++;
    }

    if (d_duplicates > 0)
    {
        LOG("Double streamed {} times\n", d_duplicates);
    }

    LOG("Streamed per frame {}, uploaded {}, time {} microsec\n", m_Transfers.Size(), d_uploaded, Core::SysMicroseconds() - uploadStartTime);

    UnlockTransfers();

    for (int texIndex = m_VirtualTextures.Size() - 1; texIndex >= 0; texIndex--)
    {
        VirtualTexturePtr texture = m_VirtualTextures[texIndex];

        texture->CommitPageResidency();

        if (texture->GetRefCount() == 1)
        {

            // Remove texture from the cache
            for (int i = 0; i < m_PageCacheCapacity; i++)
            {
                if (m_PhysPageInfo[i].pTexture == texture)
                {
                    m_PhysPageInfo[i].pTexture->MakePageNonResident(m_PhysPageInfo[i].PageIndex);
                }
                m_PhysPageInfo[i].Time = 0;
                m_PhysPageInfo[i].PageIndex = 0;
                m_PhysPageInfo[i].pTexture = 0;
            }

            texture->RemoveRef();

            m_VirtualTextures.Remove(texIndex);
        }
    }
}

void VirtualTextureCache::TransferPageData(PageTransfer* Transfer, int PhysPageIndex)
{
    int offsetX = PhysPageIndex & (m_PageCacheCapacityX - 1);
    int offsetY = PhysPageIndex / m_PageCacheCapacityX;

    TextureRect rect;

    rect.Offset.MipLevel = 0;
    rect.Offset.X = offsetX * m_PageResolutionB;
    rect.Offset.Y = offsetY * m_PageResolutionB;
    rect.Offset.Z = 0;

    rect.Dimension.X = m_PageResolutionB;
    rect.Dimension.Y = m_PageResolutionB;
    rect.Dimension.Z = 1;

    size_t offset = Transfer->Offset;

    for (int layerIndex = 0; layerIndex < m_PhysCacheLayers.Size(); layerIndex++)
    {
#ifdef PAGE_STREAM_PBO
        rcmd->CopyBufferToTexture(m_TransferBuffer, m_PhysCacheLayers[layerIndex], rect, m_LayerInfo[layerIndex].UploadFormat, 0, offset, 1);

        offset += Align(m_LayerInfo[layerIndex].PageSizeInBytes, 16);
#else
        m_PhysCacheLayers[layerIndex]->WriteRect(rect,
                                                 m_LayerInfo[layerIndex].UploadFormat,
                                                 m_LayerInfo[layerIndex].PageSizeInBytes,
                                                 1,
                                                 Page->Layers[layerIndex]);
#endif
    }

    WaitForFences();

    Transfer->Fence = rcmd->FenceSync();
}

void VirtualTextureCache::DiscardTransfers(PageTransfer** InTransfers, int Count)
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

void VirtualTextureCache::WaitForFences()
{
    const uint64_t timeOutNanoseconds = 1;

    int freePoint = m_TransferFreePoint.Load();
    for (int i = 0; i < MAX_UPLOADS_PER_FRAME; i++)
    {
        freePoint = freePoint % MAX_UPLOADS_PER_FRAME;
        if (!m_PageTransfer[freePoint].Fence)
        {
            break;
        }

        CLIENT_WAIT_STATUS status = rcmd->ClientWait(m_PageTransfer[freePoint].Fence, timeOutNanoseconds);

        if (status == CLIENT_WAIT_ALREADY_SIGNALED || status == CLIENT_WAIT_CONDITION_SATISFIED)
        {
            rcmd->RemoveSync(m_PageTransfer[freePoint].Fence);
            m_PageTransfer[freePoint].Fence = nullptr;
            freePoint = m_TransferFreePoint.Increment();
            m_PageTransferEvent.Signal();
        }
        else
        {
            break;
        }
    }
}

void VirtualTextureCache::Draw(FrameGraph& FrameGraph, FGTextureProxy* RenderTarget, int LayerIndex)
{
    if (LayerIndex < 0 || LayerIndex >= m_PhysCacheLayers.Size())
    {
        return;
    }

    ITexture* texture = m_PhysCacheLayers[LayerIndex];

    FGTextureProxy* CacheTexture_R = FrameGraph.AddExternalResource<FGTextureProxy>("VT Cache", texture);

    RenderPass& pass = FrameGraph.AddTask<RenderPass>("VT Draw Cache");

    float scale = texture->GetWidth() != 0 ? (float)GRenderView->Width / texture->GetWidth() : 0.0f;

    pass.SetRenderArea((float)texture->GetWidth() * scale * 0.5f,
                       (float)texture->GetHeight() * scale * 0.5f);

    pass.AddResource(CacheTexture_R, FG_RESOURCE_ACCESS_READ);

    pass.SetColorAttachment(
        TextureAttachment(RenderTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    pass.AddSubpass({0}, // color attachment refs
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, CacheTexture_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, m_DrawCachePipeline);
                    });
}

HK_NAMESPACE_END
