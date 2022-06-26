/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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
#include "../RenderLocal.h"

using namespace RenderCore;

#define USE_PBO

AVirtualTexture::AVirtualTexture( const char * FileName, AVirtualTextureCache * Cache )
    : AVirtualTextureFile( FileName )
{
    PIT = nullptr;
    pIndirectionData = nullptr;
    pCache = nullptr;
    NumLods = 0;

    if ( FileHandle.IsInvalid() ) {
        return;
    }

    // TODO: Check cache format compatibility
    pCache = Cache;

    //if ( !IsCompatibleFormat( Cache ) ) {
    //    LOG( "AVirtualTexture::Load: incompatible file format\n" );

    //    Purge();
    //    return false;
    //}

    HK_ASSERT( AddressTable.NumLods <= VT_MAX_LODS );

    PIT = PageInfoTable.Data;

    NumLods = AddressTable.NumLods;

#ifdef USE_PBO
    SBufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)(
        IMMUTABLE_MAP_READ
        | IMMUTABLE_MAP_WRITE
        //| IMMUTABLE_MAP_CLIENT_STORAGE
        | IMMUTABLE_MAP_PERSISTENT
        | IMMUTABLE_MAP_COHERENT
        );
    bufferCI.SizeInBytes = sizeof( pIndirectionData[0] ) * AddressTable.TotalPages;
    GDevice->CreateBuffer( bufferCI, nullptr, &IndirectionData );
    IndirectionData->SetDebugName( "Virtual texture indirection data" );
#else
    pIndirectionData = (uint16_t*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(pIndirectionData[0]) * AddressTable.TotalPages, 0, true);
#endif

    Platform::ZeroMem( bDirtyLods, sizeof( bDirtyLods ) );

    // Init indirection table
    if ( NumLods > 0 ) {
        uint32_t indirectionTableSize = 1u << (NumLods - 1);

        GDevice->CreateTexture(
            STextureDesc()
                .SetFormat(TEXTURE_FORMAT_RG8)
                .SetResolution(STextureResolution2D(indirectionTableSize, indirectionTableSize))
                .SetMipLevels(NumLods)
                .SetBindFlags(BIND_SHADER_RESOURCE),
            &IndirectionTexture);

        IndirectionTexture->SetDebugName( "Indirection texture" );

        const SClearValue clearValue[2] = { 0,0 };

        for ( int level = 0 ; level < NumLods ; level++ ) {
            rcmd->ClearTexture( IndirectionTexture, level, FORMAT_UBYTE2, clearValue );
        }
    }
}

AVirtualTexture::~AVirtualTexture()
{
#ifdef USE_PBO
    UnmapIndirectionData();
#else
    Platform::GetHeapAllocator<HEAP_MISC>().Free(pIndirectionData);
    pIndirectionData = nullptr;
#endif
}

bool AVirtualTexture::IsLoaded() const
{
    return !FileHandle.IsInvalid();
}

const uint16_t * AVirtualTexture::GetIndirectionData()
{
#ifdef USE_PBO
    MapIndirectionData();
#endif
    return pIndirectionData;
}

void AVirtualTexture::MapIndirectionData()
{
#ifdef USE_PBO
    if ( !pIndirectionData ) {
        pIndirectionData = (uint16_t*)rcmd->MapBuffer(IndirectionData,
                                                      MAP_TRANSFER_RW,
                                                      MAP_NO_INVALIDATE,
                                                      MAP_PERSISTENT_COHERENT,
                                                      false,
                                                      false);
    }
#endif
}

void AVirtualTexture::UnmapIndirectionData()
{
#ifdef USE_PBO
    if ( pIndirectionData ) {
        rcmd->UnmapBuffer(IndirectionData);
        pIndirectionData = nullptr;
    }
#endif
}

#if 0
void AVirtualTexture::UpdateBranch( uint32_t PageIndex, uint16_t Bits16, int MaxDeep ) {
    UpdateBranch_r( QuadTreeCalcLod64( PageIndex ), PageIndex, Bits16, MaxDeep );
}
#endif

void AVirtualTexture::UpdateBranch_r( int Lod, uint32_t PageIndex, uint16_t Bits16, int MaxDeep ) {
    // NOTE: This function must be VERY fast!

    if ( --MaxDeep <= 0 ) {
        return;
    }

    if ( !(PIT[PageIndex] & PF_CACHED) ) {

        //if ( pIndirectionData[ PageIndex ] != bits16 )
        {

            pIndirectionData[PageIndex] = Bits16;

            ++bDirtyLods[Lod];

            if ( Lod + 1 < NumLods ) {
                uint32_t div = 1 << Lod;
                uint32_t relativeNode = PageIndex - QuadTreeRemapTable.Rel2Abs[Lod];
                uint32_t chil0 = ((relativeNode >> Lod) << (Lod+2)) + ((relativeNode & (div-1)) << 1) + QuadTreeRemapTable.Rel2Abs[Lod+1];
                uint32_t chil2 = chil0 + (div << 1);

                ++Lod;

                UpdateBranch_r( Lod, chil0, Bits16, MaxDeep );
                UpdateBranch_r( Lod, chil0+1, Bits16, MaxDeep );
                UpdateBranch_r( Lod, chil2, Bits16, MaxDeep );
                UpdateBranch_r( Lod, chil2+1, Bits16, MaxDeep );
            }

        }
        //else {
        //    LOG( "AVirtualTexture::UpdateBranch_r: optimizing..." );
        //}
    }
}

#if 0
void AVirtualTexture::UpdateChildsBranch( uint32_t PageIndex, uint16_t Bits16, int MaxDeep ) {
    UpdateChildsBranch_r( QuadTreeCalcLod64( PageIndex ), PageIndex, Bits16, MaxDeep );
}
#endif

void AVirtualTexture::UpdateChildsBranch_r( int Lod, uint32_t PageIndex, uint16_t Bits16, int MaxDeep ) {
    ++bDirtyLods[Lod];

    if ( Lod + 1 < NumLods ) {
        uint32_t div = 1 << Lod;
        uint32_t relativeNode = PageIndex - QuadTreeRemapTable.Rel2Abs[Lod];
        uint32_t chil0 = ((relativeNode >> Lod) << (Lod+2)) + ((relativeNode & (div-1)) << 1) + QuadTreeRemapTable.Rel2Abs[Lod+1];
        uint32_t chil2 = chil0 + (div << 1);

        ++Lod;

        UpdateBranch_r( Lod, chil0, Bits16, MaxDeep );
        UpdateBranch_r( Lod, chil0+1, Bits16, MaxDeep );
        UpdateBranch_r( Lod, chil2, Bits16, MaxDeep );
        UpdateBranch_r( Lod, chil2+1, Bits16, MaxDeep );
    }
}

void AVirtualTexture::UpdateAllBranches() {
    uint32_t relativeIndex;
    uint32_t parentIndex;
    uint32_t pageIndex = 0;
    uint32_t lastIndex = 0;

    MapIndirectionData();

    for ( int lod = 0 ; lod < NumLods ; lod++ ) {

        lastIndex += QuadTreeCalcLodNodes( lod );

        while ( pageIndex < lastIndex ) {

            if ( !(PIT[pageIndex] & PF_CACHED) ) {
                // FIXME: is there better way to find parent?
                relativeIndex = pageIndex - QuadTreeRemapTable.Rel2Abs[lod];
                parentIndex = QuadTreeGetParentFromRelative( relativeIndex, lod );
                pIndirectionData[pageIndex] = pIndirectionData[parentIndex];
            }

            pageIndex++;
        }
    }
}

void AVirtualTexture::CommitPageResidency() {
    STextureRect rect;

    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Dimension.Z = 1;

    for ( int level = 0 ; level < NumLods ; level++ ) {
        if ( bDirtyLods[level] > 0 ) {
            uint32_t page = QuadTreeRelativeToAbsoluteIndex( 0, level );
            int size = 1 << level;

            rect.Offset.MipLevel = NumLods - level - 1;
            rect.Dimension.X = rect.Dimension.Y = size;

            UnmapIndirectionData();

            // TODO: Update only changed pixels

#ifdef USE_PBO
            rcmd->CopyBufferToTexture( IndirectionData,
                                       IndirectionTexture,
                                       rect,
                                       FORMAT_UBYTE2,
                                       0,
                                       page * sizeof( pIndirectionData[0] ),
                                       2 );
#else
            size_t sizeInBytes = size * size * 2;

            IndirectionTexture.WriteRect( rect,
                                          FORMAT_UBYTE2,
                                          sizeInBytes,
                                          1,
                                          &pIndirectionData[page] );
#endif
        }

        bDirtyLods[level] = false;
    }
}

void AVirtualTexture::UpdateLRU( uint32_t AbsIndex )
{
    HK_ASSERT( pCache );

    // NOTE: Assume that texture is registered in cache.
    // We don't check even validness of AbsIndex
    // Checks are disabled for performance issues
    PendingUpdateLRU.Add(AbsIndex);
}

void AVirtualTexture::MakePageResident( uint32_t AbsIndex, int PhysPageIndex )
{
    MapIndirectionData();

    int lod = QuadTreeCalcLod64( AbsIndex );

    PIT[AbsIndex] |= PF_CACHED;

    uint16_t bits16 = PhysPageIndex | (lod << 12);
    pIndirectionData[AbsIndex] = bits16;

    UpdateChildsBranch_r( lod,
                          AbsIndex,
                          bits16,
                          AddressTable.NumLods );
}

void AVirtualTexture::MakePageNonResident( uint32_t AbsIndex )
{
    MapIndirectionData();

    HK_ASSERT( PIT[AbsIndex] & PF_CACHED );

    PIT[AbsIndex] &= ~PF_CACHED;

    int lod = QuadTreeCalcLod64( AbsIndex );
    uint32_t relativeIndex = QuadTreeAbsoluteToRelativeIndex( AbsIndex, lod );

    if ( lod > 0 ) {
        uint32_t parent = QuadTreeGetParentFromRelative( relativeIndex, lod );

        UpdateBranch_r( lod,
                        AbsIndex,
                        pIndirectionData[parent],
                        AddressTable.NumLods );
    } else {
        UpdateBranch_r( lod,
                        AbsIndex,
                        0,
                        AddressTable.NumLods );
    }
}
