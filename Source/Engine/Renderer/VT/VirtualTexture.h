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

#include "VirtualTexturePhysCache.h"
#include "VirtualTextureFile.h"

#include <unordered_map>

class AVirtualTexture : public AVirtualTextureFile
{
public:
    AVirtualTexture( const char * FileName, AVirtualTextureCache * Cache );
    ~AVirtualTexture();

    bool IsLoaded() const;

    void MakePageResident( uint32_t AbsIndex, int PhysPageIndex );

    void MakePageNonResident( uint32_t AbsIndex );

    /** Update indirection table on GPU */
    void CommitPageResidency();

    /** Update LRU time for cached page. Note, page must be in cache and texture must be registered,
    and AbsIndex must be valid. If not, behavior is undefined */
    void UpdateLRU( uint32_t AbsIndex );

    /** Get page indirection data in format:
    [xxxxyyyyyyyyyyyy]
    xxxx - level of detail
    yyyyyyyyyyyy - position in physical cache
    */
    const uint16_t * GetIndirectionData();

    /** Get page indirection texture */
    RenderCore::ITexture * GetIndirectionTexture() { return IndirectionTexture; }

    /** Actual number of texture mipmaps */
    uint32_t GetStoredLods() const { return NumLods; }

    /** Total number of stored lods */
    uint32_t GetNumLods() const { return NumLods; }

private:
    /** Recursively updates quadtree branch */
    void UpdateBranch_r( int Lod, uint32_t PageIndex, uint16_t Bits16, int MaxDeep );

    /** Recursively updates quadtree branch */
    void UpdateChildsBranch_r( int Lod, uint32_t PageIndex, uint16_t Bits16, int MaxDeep );

    /** Update full quad tree */
    void UpdateAllBranches();

    void MapIndirectionData();
    void UnmapIndirectionData();

    /** Total number of stored lods */
    uint32_t NumLods;

#if 0
    /** TotalLods - StoredLods */
    uint32_t ReducedLods;

    /** Total number of all texture pages (all lods) */
    uint32_t NumPages;
#endif

    /** Table of indirection */
    TRef< RenderCore::ITexture > IndirectionTexture;

    /**
    [xxxxyyyyyyyyyyyy]
    xxxx - level of detail
    yyyyyyyyyyyy - position in physical cache
    Max pages in cache may reach to 4096
    Duplicates Indirection texture in video memory
    */
    TRef< RenderCore::IBuffer > IndirectionData;
    uint16_t * pIndirectionData;

    int bDirtyLods[VT_MAX_LODS];

    /**
    Page info table
    [xxxxyyyy]
    xxxx - max LOD
    yyyy - PageFlags4bit
    */
    byte * PIT;

    // Used only by cache to update page LRU
    TPodVector< uint32_t > PendingUpdateLRU;

    // Used only from stream thread to mark streamed pages
    std::unordered_map< uint32_t, int64_t > StreamedPages;

    AVirtualTextureCache * pCache;

    friend class AVirtualTextureCache;
    friend class AVirtualTextureFeedbackAnalyzer;
};
