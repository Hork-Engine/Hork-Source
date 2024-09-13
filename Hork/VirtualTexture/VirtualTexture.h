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

#pragma once

#include "VirtualTexturePhysCache.h"
#include "VirtualTextureFile.h"

#include <unordered_map>

HK_NAMESPACE_BEGIN

class VirtualTexture : public VirtualTextureFile
{
    friend class                VirtualTextureCache;
    friend class                VirtualTextureFeedbackAnalyzer;

public:
                                VirtualTexture(const char* fileName, VirtualTextureCache* cache);
                                ~VirtualTexture();

    bool                        IsLoaded() const;

    void                        MakePageResident(uint32_t absIndex, int physPageIndex);

    void                        MakePageNonResident(uint32_t absIndex);

    /// Update indirection table on GPU
    void                        CommitPageResidency();

    /// Update LRU time for cached page. Note, page must be in cache and texture must be registered,
    /// and absIndex must be valid. If not, behavior is undefined
    void                        UpdateLRU(uint32_t absIndex);

    /// Get page indirection data in format:
    /// [xxxxyyyyyyyyyyyy]
    /// xxxx - level of detail
    /// yyyyyyyyyyyy - position in physical cache
    const uint16_t*             GetIndirectionData();

    /// Get page indirection texture
    RHI::ITexture*              GetIndirectionTexture() { return m_IndirectionTexture; }

    /// Actual number of texture mipmaps
    uint32_t                    GetStoredLods() const { return m_NumLods; }

    /// Total number of stored lods
    uint32_t                    GetNumLods() const { return m_NumLods; }

private:
    /// Recursively updates quadtree branch
    void                        UpdateBranch_r(int lod, uint32_t pageIndex, uint16_t bits16, int maxDeep);

    /// Recursively updates quadtree branch
    void                        UpdateChildsBranch_r(int lod, uint32_t pageIndex, uint16_t bits16, int maxDeep);

    /// Update full quad tree
    void                        UpdateAllBranches();

    void                        MapIndirectionData();
    void                        UnmapIndirectionData();

    RHI::IImmediateContext*     m_Context;

    /// Total number of stored lods
    uint32_t                    m_NumLods;

    /// Table of indirection
    Ref<RHI::ITexture>          m_IndirectionTexture;

    /// [xxxxyyyyyyyyyyyy]
    /// xxxx - level of detail
    /// yyyyyyyyyyyy - position in physical cache
    /// Max pages in cache may reach to 4096
    /// Duplicates Indirection texture in video memory
    Ref<RHI::IBuffer>           m_IndirectionData;
    uint16_t*                   m_IndirectionDataRAM;

    int                         m_DirtyLods[VT_MAX_LODS];

    /// Page info table
    /// [xxxxyyyy]
    /// xxxx - max LOD
    /// yyyy - PageFlags4bit
    byte*                       m_PIT;

    // Used only by cache to update page LRU
    Vector<uint32_t>            m_PendingUpdateLRU;

    // Used only from stream thread to mark streamed pages
    HashMap<uint32_t, int64_t>  m_StreamedPages;

    VirtualTextureCache*        m_Cache;
};

HK_NAMESPACE_END
