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

#include <Core/Public/CoreMath.h>
#include <Core/Public/PodVector.h>

#include <RenderCore/FrameGraph/FrameGraph.h>

#include "VT.h"

class AVirtualTexture;

struct SVirtualTextureCacheLayerInfo
{
    /** Pixel format on GPU */
    RenderCore::TEXTURE_FORMAT TextureFormat;
    /** Upload pixel format */
    RenderCore::DATA_FORMAT UploadFormat;
    /** Page size in bytes for this layer */
    size_t PageSizeInBytes;
};

struct SVirtualTextureCacheCreateInfo
{
    uint32_t PageCacheCapacityX;
    uint32_t PageCacheCapacityY;

    SVirtualTextureCacheLayerInfo * pLayers;
    uint8_t NumLayers;

    uint16_t PageResolutionB;
};

constexpr uint32_t MIN_PAGE_CACHE_CAPACITY = 8;

class AVirtualTextureCache : public ARefCounted
{
public:
    AVirtualTextureCache( SVirtualTextureCacheCreateInfo const & CreateInfo );
    virtual ~AVirtualTextureCache();

    bool CreateTexture( const char * FileName, TRef< AVirtualTexture > * ppTexture );
    //void DestroyTexture( TRef< AVirtualTexture > * ppTexture );

    /** Cache horizontal capacity */
    uint32_t GetPageCacheCapacityX() const { return PageCacheCapacityX; }

    /** Cache vertical capacity */
    uint32_t GetPageCacheCapacityY() const { return PageCacheCapacityY; }

    /** Cache total capacity */
    uint32_t GetPageCacheCapacity() const { return PageCacheCapacity; }

    Float4 const & GetPageTranslationOffsetAndScale() const { return PageTranslationOffsetAndScale; }

    /** Page layers in texture memory */
    std::vector< TRef< RenderCore::ITexture > > & GetLayers() { return PhysCacheLayers; }

    /** Called on every frame */
    void Update();

    void ResetCache();

    struct SPageTransfer
    {
        size_t Offset;
        RenderCore::SyncObject Fence;
        AVirtualTexture * pTexture;
        uint32_t PageIndex;
        byte * Layers[VT_MAX_LAYERS];
    };

    /** Called by async thread to create new page transfer */
    SPageTransfer * CreatePageTransfer();

    /** Called by async thread when page was streamed */
    void MakePageTransferVisible( SPageTransfer * Transfer );

    /** Draw cache for debugging */
    void Draw( AFrameGraph & FrameGraph, AFrameGraphTexture * RenderTarget, int LayerIndex );

private:
    bool LockTransfers();

    void UnlockTransfers();

    void TransferPageData( SPageTransfer * Transfer, int PhysPageIndex );

    void DiscardTransfers( SPageTransfer ** Transfers, int Count );

    void WaitForFences();

    /** Physical page cache */
    std::vector< TRef< RenderCore::ITexture > > PhysCacheLayers;
    std::vector< SVirtualTextureCacheLayerInfo > LayerInfo;

    using AVirtualTexturePtr = AVirtualTexture *;

    TPodVector< AVirtualTexturePtr > VirtualTextures;

    /** Physical page info */
    struct SPhysPageInfo
    {
        /** Time of last request */
        int64_t Time;

        /** Absolute page index */
        uint32_t PageIndex;

        /** Virtual texture */
        AVirtualTexture * pTexture;
    };

    /** Physical pages sorted by time */
    struct SPhysPageInfoSorted
    {
        SPhysPageInfo * pInfo;
    };

    /** Physical page infos */
    TPodVector< SPhysPageInfo > PhysPageInfo;

    /** Physical page infos sorted by time */
    TPodVector< SPhysPageInfoSorted > PhysPageInfoSorted;

    uint32_t PageCacheCapacityX;
    uint32_t PageCacheCapacityY;
    uint32_t PageCacheCapacity;
    uint16_t PageResolutionB;
    size_t PageSizeInBytes;
    size_t AlignedSize;
    int TotalCachedPages;

    Float4 PageTranslationOffsetAndScale;

    int64_t LRUTime;

    TPodVector< SPageTransfer * > Transfers;
    AMutex TransfersMutex;

    enum { MAX_UPLOADS_PER_FRAME = 64 };
    TRef< RenderCore::IBuffer > TransferBuffer;
    byte * pTransferData;
    size_t TransferDataOffset;
    int TransferAllocPoint;
    AAtomicInt TransferFreePoint;
    SPageTransfer PageTransfer[MAX_UPLOADS_PER_FRAME];
    ASyncEvent PageTransferEvent;

    // For debugging
    TRef< RenderCore::IPipeline > DrawCachePipeline;
};
