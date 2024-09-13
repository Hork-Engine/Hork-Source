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

#include "VirtualTexture.h"
#include "QuadTree.h"

HK_NAMESPACE_BEGIN

using namespace RHI;

#define USE_PBO

VirtualTexture::VirtualTexture(const char* fileName, VirtualTextureCache* cache) :
    VirtualTextureFile(fileName),
    m_Context(cache->GetDevice()->GetImmediateContext())
{
    m_PIT = nullptr;
    m_IndirectionDataRAM = nullptr;
    m_Cache = nullptr;
    m_NumLods = 0;

    if (m_FileHandle.IsInvalid())
    {
        return;
    }

    // TODO: Check cache format compatibility
    m_Cache = cache;

    //if ( !IsCompatibleFormat( cache ) ) {
    //    LOG( "VirtualTexture::Load: incompatible file format\n" );

    //    Purge();
    //    return false;
    //}

    HK_ASSERT(m_AddressTable.NumLods <= VT_MAX_LODS);

    m_PIT = m_PageInfoTable.Data;

    m_NumLods = m_AddressTable.NumLods;

    auto device = m_Cache->GetDevice();

#ifdef USE_PBO
    BufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)(IMMUTABLE_MAP_READ | IMMUTABLE_MAP_WRITE
                                                               //| IMMUTABLE_MAP_CLIENT_STORAGE
                                                               | IMMUTABLE_MAP_PERSISTENT | IMMUTABLE_MAP_COHERENT);
    bufferCI.SizeInBytes = sizeof(m_IndirectionDataRAM[0]) * m_AddressTable.TotalPages;
    device->CreateBuffer(bufferCI, nullptr, &m_IndirectionData);
    m_IndirectionData->SetDebugName("Virtual texture indirection data");
#else
    m_IndirectionDataRAM = (uint16_t*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(m_IndirectionDataRAM[0]) * m_AddressTable.TotalPages, 0, true);
#endif

    Core::ZeroMem(m_DirtyLods, sizeof(m_DirtyLods));

    // Init indirection table
    if (m_NumLods > 0)
    {
        uint32_t indirectionTableSize = 1u << (m_NumLods - 1);

        device->CreateTexture(
            TextureDesc()
                .SetFormat(TEXTURE_FORMAT_RG8_UNORM)
                .SetResolution(TextureResolution2D(indirectionTableSize, indirectionTableSize))
                .SetMipLevels(m_NumLods)
                .SetBindFlags(BIND_SHADER_RESOURCE),
            &m_IndirectionTexture);

        m_IndirectionTexture->SetDebugName("Indirection texture");

        const ClearValue clearValue[2] = {0, 0};

        for (int level = 0; level < m_NumLods; level++)
        {
            m_Context->ClearTexture(m_IndirectionTexture, level, FORMAT_UBYTE2, clearValue);
        }
    }
}

VirtualTexture::~VirtualTexture()
{
#ifdef USE_PBO
    UnmapIndirectionData();
#else
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_IndirectionDataRAM);
    m_IndirectionDataRAM = nullptr;
#endif
}

bool VirtualTexture::IsLoaded() const
{
    return !m_FileHandle.IsInvalid();
}

const uint16_t* VirtualTexture::GetIndirectionData()
{
#ifdef USE_PBO
    MapIndirectionData();
#endif
    return m_IndirectionDataRAM;
}

void VirtualTexture::MapIndirectionData()
{
#ifdef USE_PBO
    if (!m_IndirectionDataRAM)
    {
        m_IndirectionDataRAM = (uint16_t*)m_Context->MapBuffer(m_IndirectionData,
                                                               MAP_TRANSFER_RW,
                                                               MAP_NO_INVALIDATE,
                                                               MAP_PERSISTENT_COHERENT,
                                                               false,
                                                               false);
    }
#endif
}

void VirtualTexture::UnmapIndirectionData()
{
#ifdef USE_PBO
    if (m_IndirectionDataRAM)
    {
        m_Context->UnmapBuffer(m_IndirectionData);
        m_IndirectionDataRAM = nullptr;
    }
#endif
}

#if 0
void VirtualTexture::UpdateBranch( uint32_t pageIndex, uint16_t bits16, int maxDeep ) {
    UpdateBranch_r( QuadTreeCalcLod64( pageIndex ), pageIndex, bits16, maxDeep );
}
#endif

void VirtualTexture::UpdateBranch_r(int lod, uint32_t pageIndex, uint16_t bits16, int maxDeep)
{
    // NOTE: This function must be VERY fast!

    if (--maxDeep <= 0)
    {
        return;
    }

    if (!(m_PIT[pageIndex] & PF_CACHED))
    {

        //if ( m_IndirectionDataRAM[ pageIndex ] != bits16 )
        {

            m_IndirectionDataRAM[pageIndex] = bits16;

            ++m_DirtyLods[lod];

            if (lod + 1 < m_NumLods)
            {
                uint32_t div = 1 << lod;
                uint32_t relativeNode = pageIndex - GQuadTreeRemapTable.Rel2Abs[lod];
                uint32_t chil0 = ((relativeNode >> lod) << (lod + 2)) + ((relativeNode & (div - 1)) << 1) + GQuadTreeRemapTable.Rel2Abs[lod + 1];
                uint32_t chil2 = chil0 + (div << 1);

                ++lod;

                UpdateBranch_r(lod, chil0, bits16, maxDeep);
                UpdateBranch_r(lod, chil0 + 1, bits16, maxDeep);
                UpdateBranch_r(lod, chil2, bits16, maxDeep);
                UpdateBranch_r(lod, chil2 + 1, bits16, maxDeep);
            }
        }
        //else {
        //    LOG( "VirtualTexture::UpdateBranch_r: optimizing..." );
        //}
    }
}

#if 0
void VirtualTexture::UpdateChildsBranch( uint32_t pageIndex, uint16_t bits16, int maxDeep ) {
    UpdateChildsBranch_r( QuadTreeCalcLod64( pageIndex ), pageIndex, bits16, maxDeep );
}
#endif

void VirtualTexture::UpdateChildsBranch_r(int lod, uint32_t pageIndex, uint16_t bits16, int maxDeep)
{
    ++m_DirtyLods[lod];

    if (lod + 1 < m_NumLods)
    {
        uint32_t div = 1 << lod;
        uint32_t relativeNode = pageIndex - GQuadTreeRemapTable.Rel2Abs[lod];
        uint32_t chil0 = ((relativeNode >> lod) << (lod + 2)) + ((relativeNode & (div - 1)) << 1) + GQuadTreeRemapTable.Rel2Abs[lod + 1];
        uint32_t chil2 = chil0 + (div << 1);

        ++lod;

        UpdateBranch_r(lod, chil0, bits16, maxDeep);
        UpdateBranch_r(lod, chil0 + 1, bits16, maxDeep);
        UpdateBranch_r(lod, chil2, bits16, maxDeep);
        UpdateBranch_r(lod, chil2 + 1, bits16, maxDeep);
    }
}

void VirtualTexture::UpdateAllBranches()
{
    uint32_t relativeIndex;
    uint32_t parentIndex;
    uint32_t pageIndex = 0;
    uint32_t lastIndex = 0;

    MapIndirectionData();

    for (int lod = 0; lod < m_NumLods; lod++)
    {
        lastIndex += QuadTreeCalcLodNodes(lod);

        while (pageIndex < lastIndex)
        {
            if (!(m_PIT[pageIndex] & PF_CACHED))
            {
                // FIXME: is there better way to find parent?
                relativeIndex = pageIndex - GQuadTreeRemapTable.Rel2Abs[lod];
                parentIndex = QuadTreeGetParentFromRelative(relativeIndex, lod);
                m_IndirectionDataRAM[pageIndex] = m_IndirectionDataRAM[parentIndex];
            }
            pageIndex++;
        }
    }
}

void VirtualTexture::CommitPageResidency()
{
    TextureRect rect;

    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Dimension.Z = 1;

    for (int level = 0; level < m_NumLods; level++)
    {
        if (m_DirtyLods[level] > 0)
        {
            uint32_t page = QuadTreeRelativeToAbsoluteIndex(0, level);
            int size = 1 << level;

            rect.Offset.MipLevel = m_NumLods - level - 1;
            rect.Dimension.X = rect.Dimension.Y = size;

            UnmapIndirectionData();

            // TODO: Update only changed pixels

#ifdef USE_PBO
            m_Context->CopyBufferToTexture(m_IndirectionData,
                                           m_IndirectionTexture,
                                           rect,
                                           FORMAT_UBYTE2,
                                           0,
                                           page * sizeof(m_IndirectionDataRAM[0]),
                                           2);
#else
            size_t sizeInBytes = size * size * 2;

            m_IndirectionTexture.WriteRect(rect,
                                           FORMAT_UBYTE2,
                                           sizeInBytes,
                                           1,
                                           &m_IndirectionDataRAM[page]);
#endif
        }

        m_DirtyLods[level] = false;
    }
}

void VirtualTexture::UpdateLRU(uint32_t absIndex)
{
    HK_ASSERT(m_Cache);

    // NOTE: Assume that texture is registered in cache.
    // We don't check even validness of absIndex
    // Checks are disabled for performance issues
    m_PendingUpdateLRU.Add(absIndex);
}

void VirtualTexture::MakePageResident(uint32_t absIndex, int physPageIndex)
{
    MapIndirectionData();

    int lod = QuadTreeCalcLod64(absIndex);

    m_PIT[absIndex] |= PF_CACHED;

    uint16_t bits16 = physPageIndex | (lod << 12);
    m_IndirectionDataRAM[absIndex] = bits16;

    UpdateChildsBranch_r(lod,
                         absIndex,
                         bits16,
                         m_AddressTable.NumLods);
}

void VirtualTexture::MakePageNonResident(uint32_t absIndex)
{
    MapIndirectionData();

    HK_ASSERT(m_PIT[absIndex] & PF_CACHED);

    m_PIT[absIndex] &= ~PF_CACHED;

    int lod = QuadTreeCalcLod64(absIndex);
    uint32_t relativeIndex = QuadTreeAbsoluteToRelativeIndex(absIndex, lod);

    if (lod > 0)
    {
        uint32_t parent = QuadTreeGetParentFromRelative(relativeIndex, lod);

        UpdateBranch_r(lod,
                       absIndex,
                       m_IndirectionDataRAM[parent],
                       m_AddressTable.NumLods);
    }
    else
    {
        UpdateBranch_r(lod,
                       absIndex,
                       0,
                       m_AddressTable.NumLods);
    }
}

HK_NAMESPACE_END
