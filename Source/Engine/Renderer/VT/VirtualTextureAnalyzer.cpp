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

#include "VirtualTextureAnalyzer.h"
#include "QuadTree.h"
#include "../RenderLocal.h"
#include <Runtime/Public/ScopedTimeCheck.h>

AVirtualTextureFeedbackAnalyzer::AVirtualTextureFeedbackAnalyzer()
    : SwapIndex( 0 )
    , Bindings( nullptr )
    , NumBindings( 0 )
    , QueueLoadPos( 0 )
    , bStopStreamThread( false )

{
    Core::ZeroMem( Textures, sizeof( Textures ) );
    Core::ZeroMem( QuedPages, sizeof( QuedPages ) );

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
            ASyncGuard criticalSection( EnqueLock );

            GLogger.Printf( "Fetch page\n" );

            QueueLoadPos = QueueLoadPos & (MAX_QUEUE_LENGTH - 1);
            quedPage = QuedPages[QueueLoadPos];

            Core::ZeroMem( &QuedPages[QueueLoadPos], sizeof( SPageDesc ) );

            QueueLoadPos++;
        }

        AVirtualTexture * pTexture = quedPage.pTexture;

        if ( !pTexture ) {
            // Reached end of queue
            GLogger.Printf( "WaitForNewPages\n" );
            WaitForNewPages();
            continue;
        }

        //if ( pTexture->IsPendingRemove() ) {
        //    pTexture->RemoveRef();
        //    continue;
        //}

        int64_t time = GRuntime.SysMilliseconds();

        // NOTE: We can't use THash now becouse our allocators are not support multithreading
        // for better performance.
        std::unordered_map< uint32_t, int64_t > & streamedPages = pTexture->StreamedPages;
        auto it = streamedPages.find( quedPage.PageIndex );
        if ( it != streamedPages.end() ) {
            if ( it->second + 1000 < time ) {
                GLogger.Printf( "Re-load page\n" );
                it->second = time;
            }
            else {
                // Page already loaded. Fetch next page
                GLogger.Printf( "Page already loaded\n" );
                continue;
            }
        }
        else {
            streamedPages[quedPage.PageIndex] = time;
        }

        GLogger.Printf( "Load\n" );

        SFileOffset physAddress = pTexture->GetPhysAddress( quedPage.PageIndex );

        AN_ASSERT( physAddress != 0 );

        AVirtualTextureCache::SPageTransfer * transfer = pTexture->pCache->CreatePageTransfer();

        transfer->PageIndex = quedPage.PageIndex;
        transfer->pTexture = quedPage.pTexture;

        pTexture->ReadPage( physAddress, transfer->Layers );

        //int32_t pagePayLoad = GRuntime.SysMilliseconds() - time;

        //GLogger.Printf( "pagePayLoad %d msec\n", pagePayLoad );

        //GRuntime.WaitSeconds( 1 );
        //GRuntime.WaitMilliseconds(500);

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

void AVirtualTextureFeedbackAnalyzer::SubmitPages( TPodArray< SPageDesc > const & Pages )
{
    AN_ASSERT( Pages.Size() < MAX_QUEUE_LENGTH );

    ASyncGuard criticalSection( EnqueLock );

    ClearQueue();

    // Refresh queue
    Core::Memcpy( QuedPages, Pages.ToPtr(), Pages.Size() * sizeof( QuedPages[0] ) );
    for ( int i = 0 ; i < Pages.Size() ; i++ ) {
        SPageDesc * quedPage = &QuedPages[i];
        quedPage->pTexture->AddRef();
    }

    if ( Pages.Size() > 0 ) {
        PageSubmitEvent.Signal();
    }
}

void AVirtualTextureFeedbackAnalyzer::Begin()
{
    unsigned int maxBlockSize = GDevice->GetDeviceCaps( RenderCore::DEVICE_CAPS_CONSTANT_BUFFER_MAX_BLOCK_SIZE );;

    size_t size = VT_MAX_TEXTURE_UNITS * sizeof( SVirtualTextureUnit );
    if ( size > maxBlockSize ) {
        GLogger.Printf( "AVirtualTextureFeedbackAnalyzer::Begin: constant buffer max block size hit\n" );
    }

    AStreamedMemoryGPU * streamedMemory = GRuntime.GetStreamedMemoryGPU();

    size_t offset = streamedMemory->AllocateConstant( size );

    rtbl->BindBuffer( 6, GStreamBuffer, offset, size );

    Bindings = (SVirtualTextureUnit *)streamedMemory->Map( offset );
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
AN_FORCEINLINE void VT_FeedbackUnpack_RGBA8_11LODS_256UNITS( SFeedbackData const *_Data,
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

    AScopedTimeCheck timecheck("AVirtualTextureFeedbackAnalyzer::DecodePage");

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

                auto & pageDesc = PendingPages.Append();
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
            int i = PendingPagesHash.First( hash );
            for ( ; i != -1 ; i = PendingPagesHash.Next( i ) ) {
                if ( PendingPages[i].Hash == hash ) {
                    //PendingPages[i].Refs++;
                    PendingPages[i].Refs += refs;
                    break;
                }
            }
            if ( i == -1 ) {
                auto & pageDesc = PendingPages.Append();
                pageDesc.pTexture = pTexture;
                pageDesc.Hash = hash;
                pageDesc.Refs = refs;
                pageDesc.PageIndex = absIndex;
                //pageDesc.Lod = lod;

                PendingPagesHash.Insert( hash, PendingPages.Size() - 1 );
            }
            #endif
        }
    }

    //GLogger.Printf( "Num iterations: %d, uniqe pages %d, buffer size %d\n", numIterations, PendingPages.Size(), feedbackSize );

    if ( !PendingPages.IsEmpty() ) {
        PendingPagesHash.Clear();

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
    SFeedbackChain & feedback = Feedbacks.Append();
    feedback.Size = FeedbackSize;
    feedback.Data = FeedbackData;
}

void AVirtualTextureFeedbackAnalyzer::BindTexture( int Unit, AVirtualTexture * Texture )
{
    AN_ASSERT( Unit >= 0 && Unit < VT_MAX_TEXTURE_UNITS );
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
    AN_ASSERT( Unit >= 0 && Unit < VT_MAX_TEXTURE_UNITS );
    return Textures[SwapIndex][Unit];
}
