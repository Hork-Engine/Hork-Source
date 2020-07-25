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

#include "VirtualTexture.h"
#include "QuadTree.h"
#include "../RenderCommon.h"

#define USE_PBO

AVirtualTexture::AVirtualTexture( const char * FileName, AVirtualTextureCache * Cache )
    : AVirtualTextureFile( FileName )
{
    PIT = nullptr;
    pIndirectionData = nullptr;
    pCache = nullptr;
    NumLods = 0;
    bPendingRemove = false;
    RefCount = 0;

    if ( FileHandle.IsInvalid() ) {
        return;
    }

    // TODO: Check cache format compatibility
    pCache = Cache;

    //if ( !IsCompatibleFormat( Cache ) ) {
    //    GLogger.Printf( "AVirtualTexture::Load: incompatible file format\n" );

    //    Purge();
    //    return false;
    //}

    AN_ASSERT( AddressTable.NumLods <= VT_MAX_LODS );

    PIT = PageInfoTable.Data;

    NumLods = AddressTable.NumLods;

#ifdef USE_PBO
    RenderCore::SBufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (RenderCore::IMMUTABLE_STORAGE_FLAGS)(
        RenderCore::IMMUTABLE_MAP_READ
        | RenderCore::IMMUTABLE_MAP_WRITE
        //| RenderCore::IMMUTABLE_MAP_CLIENT_STORAGE
        | RenderCore::IMMUTABLE_MAP_PERSISTENT
        | RenderCore::IMMUTABLE_MAP_COHERENT
        );
    bufferCI.SizeInBytes = sizeof( pIndirectionData[0] ) * AddressTable.TotalPages;
    GDevice->CreateBuffer( bufferCI, nullptr, &IndirectionData );
#else
    pIndirectionData = (uint16_t *)GHeapMemory.ClearedAlloc( sizeof( pIndirectionData[0] ) * AddressTable.TotalPages );
#endif

    Core::ZeroMem( bDirtyLods, sizeof( bDirtyLods ) );

    // Init indirection table
    if ( NumLods > 0 ) {
        uint32_t indirectionTableSize = 1u << (NumLods - 1);

        GDevice->CreateTexture(
            RenderCore::MakeTexture(
                RenderCore::TEXTURE_FORMAT_RG8,
                RenderCore::STextureResolution2D( indirectionTableSize, indirectionTableSize ),
                RenderCore::STextureMultisampleInfo(),
                RenderCore::STextureSwizzle( RenderCore::TEXTURE_SWIZZLE_IDENTITY,
                                     RenderCore::TEXTURE_SWIZZLE_IDENTITY,
                                     RenderCore::TEXTURE_SWIZZLE_IDENTITY,
                                     RenderCore::TEXTURE_SWIZZLE_IDENTITY ),
                NumLods ), &IndirectionTexture );

        const RenderCore::SClearValue clearValue[2] = { 0,0 };

        for ( int level = 0 ; level < NumLods ; level++ ) {
            rcmd->ClearTexture( IndirectionTexture, level, RenderCore::FORMAT_UBYTE2, clearValue );
        }
    }

    AddRef();
}

AVirtualTexture::~AVirtualTexture()
{
#ifdef USE_PBO
    UnmapIndirectionData();
#else
    GHeapMemory.Free( pIndirectionData );
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
        pIndirectionData = (uint16_t *)IndirectionData->Map( RenderCore::MAP_TRANSFER_RW,
                                          RenderCore::MAP_NO_INVALIDATE,
                                          RenderCore::MAP_PERSISTENT_COHERENT,
                                          false,
                                          false );
    }
#endif
}

void AVirtualTexture::UnmapIndirectionData()
{
#ifdef USE_PBO
    if ( pIndirectionData ) {
        IndirectionData->Unmap();
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
        //    GLogger.Printf( "AVirtualTexture::UpdateBranch_r: optimizing..." );
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
    RenderCore::STextureRect rect;

    rect.Offset.X = 0;
    rect.Offset.Y = 0;
    rect.Offset.Z = 0;
    rect.Dimension.Z = 1;

    for ( int level = 0 ; level < NumLods ; level++ ) {
        if ( bDirtyLods[level] > 0 ) {
            uint32_t page = QuadTreeRelativeToAbsoluteIndex( 0, level );
            int size = 1 << level;

            rect.Offset.Lod = NumLods - level - 1;
            rect.Dimension.X = rect.Dimension.Y = size;

            UnmapIndirectionData();

            // TODO: Update only changed pixels

#ifdef USE_PBO
            rcmd->CopyBufferToTexture( IndirectionData,
                                       IndirectionTexture,
                                       rect,
                                       RenderCore::FORMAT_UBYTE2,
                                       0,
                                       page * sizeof( pIndirectionData[0] ),
                                       2 );
#else
            size_t sizeInBytes = size * size * 2;

            IndirectionTexture.WriteRect( rect,
                                          RenderCore::FORMAT_UBYTE2,
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
    AN_ASSERT( pCache );

    // NOTE: Assume that texture is registered in cache.
    // We don't check even validness of AbsIndex
    // Checks are disabled for performance issues
    PendingUpdateLRU.Append( AbsIndex );
}

void AVirtualTexture::MakePageResident( uint32_t AbsIndex, int CacheCellIndex )
{
    MapIndirectionData();

    int lod = QuadTreeCalcLod64( AbsIndex );

    PIT[AbsIndex] |= PF_CACHED;

    uint16_t bits16 = CacheCellIndex | (lod << 12);
    pIndirectionData[AbsIndex] = bits16;

    UpdateChildsBranch_r( lod,
                          AbsIndex,
                          bits16,
                          AddressTable.NumLods );
}

void AVirtualTexture::MakePageNonResident( uint32_t AbsIndex )
{
    MapIndirectionData();

    AN_ASSERT( PIT[AbsIndex] & PF_CACHED );

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
