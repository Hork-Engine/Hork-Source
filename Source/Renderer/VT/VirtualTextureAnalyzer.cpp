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

#include "VirtualTextureAnalyzer.h"
#include "QuadTree.h"
#include "../RenderLocal.h"

#include <Core/ScopedTimer.h>

AVirtualTextureFeedbackAnalyzer::AVirtualTextureFeedbackAnalyzer()
    : SwapIndex( 0 )
    , Bindings( nullptr )
    , NumBindings( 0 )
    , QueueLoadPos( 0 )
    , bStopStreamThread( false )

{
    Platform::ZeroMem( Textures, sizeof( Textures ) );
    Platform::ZeroMem( QuedPages, sizeof( QuedPages ) );

    StreamThread.Routine = StreamThreadMain;
    StreamThread.Data = this;
    StreamThread.Start();
}

AVirtualTextureFeedbackAnalyzer::~AVirtualTextureFeedbackAnalyzer()
{
    bStopStreamThread.Store( true );

    // Awake stream thread
    PageSubmitEvent.Signal();

    StreamThreadStopped.Wait();

    ClearQueue();

    for ( int i = 0 ; i < VT_MAX_TEXTURE_UNITS ; i++ ) {
        for ( int j = 0 ; j < 2 ; j++ ) {
            if ( Textures[j][i] ) {
                Textures[j][i]->RemoveRef();
                Textures[j][i] = nullptr;
            }
        }
    }
}

void AVirtualTextureFeedbackAnalyzer::StreamThreadMain( void * pData )
{
    AVirtualTextureFeedbackAnalyzer * analyzer = (AVirtualTextureFeedbackAnalyzer *)pData;
    analyzer->StreamThreadMain();
}

void AVirtualTextureFeedbackAnalyzer::WaitForNewPages()
{
    PageSubmitEvent.Wait();
}

void AVirtualTextureFeedbackAnalyzer::StreamThreadMain()
{
    SPageDesc quedPage;

    while ( !bStopStreamThread.Load() ) {
        // Fetch page
        {
            AMutexGurad criticalSection( EnqueLock );

            LOG("Fetch page\n");

            QueueLoadPos = QueueLoadPos & (MAX_QUEUE_LENGTH - 1);
            quedPage = QuedPages[QueueLoadPos];

            Platform::ZeroMem( &QuedPages[QueueLoadPos], sizeof( SPageDesc ) );

            QueueLoadPos++;
        }

        AVirtualTexture * pTexture = quedPage.pTexture;

        if ( !pTexture ) {
            // Reached end of queue
            LOG("WaitForNewPages\n");
            WaitForNewPages();
            continue;
        }

        //if ( pTexture->IsPendingRemove() ) {
        //    pTexture->RemoveRef();
        //    continue;
        //}

        int64_t time = Platform::SysMilliseconds();

        THashMap< uint32_t, int64_t > & streamedPages = pTexture->StreamedPages;
        auto it = streamedPages.Find( quedPage.PageIndex );
        if ( it != streamedPages.End() ) {
            if ( it->second + 1000 < time ) {
                LOG("Re-load page\n");
                it->second = time;
            }
            else {
                // Page already loaded. Fetch next page
                LOG("Page already loaded\n");
                continue;
            }
        }
        else {
            streamedPages[quedPage.PageIndex] = time;
        }

        LOG("Load\n");

        SFileOffset physAddress = pTexture->GetPhysAddress( quedPage.PageIndex );

        HK_ASSERT( physAddress != 0 );

        AVirtualTextureCache::SPageTransfer * transfer = pTexture->pCache->CreatePageTransfer();

        transfer->PageIndex = quedPage.PageIndex;
        transfer->pTexture = quedPage.pTexture;

        pTexture->ReadPage( physAddress, transfer->Layers );

        //int32_t pagePayLoad = Platform::SysMilliseconds() - time;

        //LOG( "pagePayLoad {} msec\n", pagePayLoad );

        // Wait for test
        //AThread::WaitSeconds( 1 );
        //AThread::WaitMilliseconds(500);

        pTexture->pCache->MakePageTransferVisible( transfer );
    }

    StreamThreadStopped.Signal();
}

void AVirtualTextureFeedbackAnalyzer::ClearQueue()
{
    for ( int i = 0 ; i < MAX_QUEUE_LENGTH ; i++ ) {
        int p = ( QueueLoadPos + i ) & ( MAX_QUEUE_LENGTH - 1 );
        SPageDesc * quedPage = &QuedPages[p];

        if ( !quedPage->pTexture ) {
            // end of queue
            break;
        }

        // Remove outdated page from queue
        quedPage->pTexture->RemoveRef();
        quedPage->pTexture = nullptr;
    }

    QueueLoadPos = 0;
}

void AVirtualTextureFeedbackAnalyzer::SubmitPages( TVector< SPageDesc > const & Pages )
{
    HK_ASSERT( Pages.Size() < MAX_QUEUE_LENGTH );

    AMutexGurad criticalSection( EnqueLock );

    ClearQueue();

    // Refresh queue
    Platform::Memcpy( QuedPages, Pages.ToPtr(), Pages.Size() * sizeof( QuedPages[0] ) );
    for ( int i = 0 ; i < Pages.Size() ; i++ ) {
        SPageDesc * quedPage = &QuedPages[i];
        quedPage->pTexture->AddRef();
    }

    if ( Pages.Size() > 0 ) {
        PageSubmitEvent.Signal();
    }
}

void AVirtualTextureFeedbackAnalyzer::Begin(AStreamedMemoryGPU* StreamedMemory)
{
    unsigned int maxBlockSize = GDevice->GetDeviceCaps( RenderCore::DEVICE_CAPS_CONSTANT_BUFFER_MAX_BLOCK_SIZE );;

    size_t size = VT_MAX_TEXTURE_UNITS * sizeof( SVirtualTextureUnit );
    if ( size > maxBlockSize ) {
        LOG("AVirtualTextureFeedbackAnalyzer::Begin: constant buffer max block size hit\n");
    }

    size_t offset = StreamedMemory->AllocateConstant(size);

    rtbl->BindBuffer( 6, GStreamBuffer, offset, size );

    Bindings    = (SVirtualTextureUnit*)StreamedMemory->Map(offset);
    NumBindings = 0;

    for ( int i = 0 ; i < VT_MAX_TEXTURE_UNITS ; i++ ) {
        if ( Textures[SwapIndex][i] ) {
            Textures[SwapIndex][i]->RemoveRef();
            Textures[SwapIndex][i] = nullptr;
        }
    }
}

// Max 11 lods, 256 units
// RGBA8
// 11111111 11111111 1111 11  11 11111111
// -------- -------- ---- --  -- --------
// X_low    Y_low    Lod  Yh  Xh Un
HK_FORCEINLINE void VT_FeedbackUnpack_RGBA8_11LODS_256UNITS( SFeedbackData const *_Data,
                                                             int & _PageX, int & _PageY, int & _Lod,
                                                             int & _TextureUnit )
{
    _PageX = _Data->Byte3 | ((_Data->Byte1 & 3) << 8);
    _PageY = _Data->Byte2 | ((_Data->Byte1 & 12) << 6);
    _Lod = _Data->Byte1 >> 4;
    _TextureUnit = _Data->Byte0;
}

void AVirtualTextureFeedbackAnalyzer::End()
{
    SwapIndex = (SwapIndex + 1) & 1;

    DecodePages();

    SubmitPages( PendingPages );

    Feedbacks.Clear();
}

void AVirtualTextureFeedbackAnalyzer::DecodePages()
{
    // TODO:
    // 1. Wait for analyzer thread from previous frame
    // 2. Copy texture bindings and feedback data pointers
    // 3. Run analyzer thread
    // FIXME: This is so fast. Do we really need to separate this for several threads?

    //#define WITH_PENDING_FLAG

    PendingPages.Clear();
#if 1
    if ( NumBindings == 0 ) {
        return;
    }

    AVirtualTexture **pTextureBindings = Textures[SwapIndex];

    AScopedTimer timecheck("AVirtualTextureFeedbackAnalyzer::DecodePage");

    int numIterations = 0; // for debug
    int feedbackSize = 0; // for debug

    for ( SFeedbackChain & feedback : Feedbacks )
    {
        const SFeedbackData * pData = (const SFeedbackData *)feedback.Data;
        const SFeedbackData * pEnd = (const SFeedbackData *)feedback.Data + feedback.Size;

        int x, y, lod, unit;
        int duplicates = 0;

        feedbackSize += feedback.Size;

        for ( ; pData < pEnd ; ++pData )
        {
            if ( ((const uint32_t*)pData)[0] == ((const uint32_t*)pData)[1]
                 && pData + 1 < pEnd )
            {
                // skip duplicates
                duplicates++;
                continue;
            }

            numIterations++;

            int refs = duplicates + 1;

            duplicates = 0;

            // Decode page
            VT_FeedbackUnpack_RGBA8_11LODS_256UNITS( pData, x, y, lod, unit );

            AVirtualTexture * pTexture = pTextureBindings[unit];
            if ( !pTexture ) {
                // No texture binded to unit
                continue;
            }

            if ( lod >= pTexture->GetStoredLods() ) {
                continue;
            }

            // Calculate page index
            uint32_t relIndex = QuadTreeGetRelativeFromXY( x, y, lod );
            uint32_t absIndex = QuadTreeRelativeToAbsoluteIndex( relIndex, lod );

            if ( !QuadTreeIsIndexValid( absIndex, lod ) ) {
                // Index is invalid. Something wrong with decoding.
                continue;
            }

            // Correct mip level
            int maxLod = pTexture->PIT[absIndex] >> 4;
            if ( maxLod < lod ) {
                int diff = lod - maxLod;
                x >>= diff;
                y >>= diff;
                relIndex = QuadTreeGetRelativeFromXY( x, y, maxLod );
                absIndex = QuadTreeRelativeToAbsoluteIndex( relIndex, maxLod );
                lod = maxLod;
            }

            byte * pageInfo = &pTexture->PIT[absIndex];

            if ( *pageInfo & PF_CACHED ) {
                pTexture->UpdateLRU( absIndex );
                continue;
            }

            // check parent cached
            while ( lod > 0 ) {
                unsigned int parentAbsolute = QuadTreeGetParentFromRelative( relIndex, lod );
                byte * parentInfo = &pTexture->PIT[parentAbsolute];
                if ( *parentInfo & PF_CACHED ) {
                    // Parent already in cache
                    break;
                }
                --lod;
                pageInfo = parentInfo;
                absIndex = parentAbsolute;
                relIndex = QuadTreeAbsoluteToRelativeIndex( parentAbsolute, lod );
            }

#if 1
            uint32_t hash = *reinterpret_cast< const uint32_t * >(pData);
#else
            // Pack textureUnit, lod, x, y to uint32
            pageX = QuadTreeGetXFromRelative( relIndex, pageLod );
            pageY = QuadTreeGetYFromRelative( relIndex, pageLod );
            uint32_t hash = (textureUnit << 24)
                    | ( pageLod << 20 )
                    | ( pageX & 0x300 ) << 10
                                           | ( pageY & 0x300 ) << 8
                                           | ( pageX & 0xff ) << 8
                                           | ( pageY & 0xff );
#endif

            // Create list of unique not cached pages

            #ifdef WITH_PENDING_FLAG
            if ( !(*pageInfo & PF_PENDING) )
            {
                *pageInfo |= PF_PENDING;

                auto& pageDesc     = PendingPages.Add();
                pageDesc.pTexture = pTexture;
                pageDesc.Hash = hash;
                pageDesc.Refs = 1;
                pageDesc.PageIndex = absIndex;
                pageDesc.Lod = pageLod;

                PendingPagesHash.Insert( hash, PendingPages.Size() - 1 );
            }
            else
            {
                for ( int i = PendingPagesHash.First( hash ) ; i != -1 ; i = PendingPagesHash.Next( i ) ) {
                    if ( PendingPages[i].Hash == hash ) {
                        PendingPages[i].Refs++;
                        break;
                    }
                }
            }
            #else
            auto it = PendingPageSet.Find(hash);
            if (it != PendingPageSet.End())
            {
                auto n = it->second;
                PendingPages[n].Refs += refs;
            }
            else
            {
                auto& pageDesc     = PendingPages.Add();
                pageDesc.pTexture  = pTexture;
                pageDesc.Hash      = hash;
                pageDesc.Refs      = refs;
                pageDesc.PageIndex = absIndex;
                //pageDesc.Lod = lod;

                PendingPageSet[hash] = PendingPages.Size() - 1;
            }
            #endif
        }
    }

    //LOG( "Num iterations: {}, uniqe pages {}, buffer size {}\n", numIterations, PendingPages.Size(), feedbackSize );

    if ( !PendingPages.IsEmpty() ) {
        PendingPageSet.Clear();

        #ifdef WITH_PENDING_FLAG
        // Unset pending flag
        for ( SPageDesc & page : PendingPages ) {
            page.pTexture->PIT[page.PageIndex] &= ~PF_PENDING;
        }
        #endif

        struct {
            bool operator() ( SPageDesc const & a, SPageDesc const & b ) {
                return a.Refs > b.Refs;
            }
        } SortByRefs;

        std::sort( PendingPages.Begin(), PendingPages.End(), SortByRefs );

        const int MAX_PENDING_PAGES = 100; // TODO: Set from console variable

        int numPendingPages = Math::Min3< int >( MAX_PENDING_PAGES, MAX_QUEUE_LENGTH, PendingPages.Size() );
        PendingPages.Resize( numPendingPages );
    }
#endif
}

void AVirtualTextureFeedbackAnalyzer::AddFeedbackData( int FeedbackSize, const void * FeedbackData )
{
    SFeedbackChain& feedback = Feedbacks.Add();
    feedback.Size = FeedbackSize;
    feedback.Data = FeedbackData;
}

void AVirtualTextureFeedbackAnalyzer::BindTexture( int Unit, AVirtualTexture * Texture )
{
    HK_ASSERT( Unit >= 0 && Unit < VT_MAX_TEXTURE_UNITS );
    if ( Texture )
    {
        Texture->AddRef();
        if ( Textures[SwapIndex][Unit] ) {
            Textures[SwapIndex][Unit]->RemoveRef();
        }
        Textures[SwapIndex][Unit] = Texture;

        Bindings[Unit].MaxLod = Texture->GetStoredLods() - 1;
        Bindings[Unit].Log2Size = Texture->GetTextureResolutionLog2();

        NumBindings++;
    }
    else
    {
        if ( Textures[SwapIndex][Unit] ) {
            Textures[SwapIndex][Unit]->RemoveRef();
            Textures[SwapIndex][Unit] = nullptr;
        }

        Bindings[Unit].MaxLod = 0;
        Bindings[Unit].Log2Size = 0;
    }
}

AVirtualTexture * AVirtualTextureFeedbackAnalyzer::GetTexture( int Unit )
{
    HK_ASSERT( Unit >= 0 && Unit < VT_MAX_TEXTURE_UNITS );
    return Textures[SwapIndex][Unit];
}
