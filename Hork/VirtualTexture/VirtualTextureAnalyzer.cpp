/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/RHI/Common/VertexMemoryGPU.h>
#include <Hork/Core/ScopedTimer.h>

HK_NAMESPACE_BEGIN

VirtualTextureFeedbackAnalyzer::VirtualTextureFeedbackAnalyzer(RHI::IDevice* device) :
    m_Device(device), m_SwapIndex(0), m_Bindings(nullptr), m_NumBindings(0), m_QueueLoadPos(0), m_StopStreamThread(false)

{
    Core::ZeroMem(m_Textures, sizeof(m_Textures));
    Core::ZeroMem(m_QuedPages, sizeof(m_QuedPages));

    m_StreamThread = Thread(
        [this]()
        {
            StreamThreadMain();
        });
}

VirtualTextureFeedbackAnalyzer::~VirtualTextureFeedbackAnalyzer()
{
    m_StopStreamThread.Store(true);

    // Awake stream thread
    m_PageSubmitEvent.Signal();

    m_StreamThreadStopped.Wait();

    ClearQueue();

    for (int i = 0; i < VT_MAX_TEXTURE_UNITS; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (m_Textures[j][i])
            {
                m_Textures[j][i]->RemoveRef();
                m_Textures[j][i] = nullptr;
            }
        }
    }
}

void VirtualTextureFeedbackAnalyzer::WaitForNewPages()
{
    m_PageSubmitEvent.Wait();
}

void VirtualTextureFeedbackAnalyzer::StreamThreadMain()
{
    VTPageDesc quedPage;

    while (!m_StopStreamThread.Load())
    {
        // Fetch page
        {
            MutexGuard criticalSection(m_EnqueLock);

            //LOG("Fetch page\n");

            m_QueueLoadPos = m_QueueLoadPos & (MAX_QUEUE_LENGTH - 1);
            quedPage = m_QuedPages[m_QueueLoadPos];

            Core::ZeroMem(&m_QuedPages[m_QueueLoadPos], sizeof(VTPageDesc));

            m_QueueLoadPos++;
        }

        VirtualTexture* pTexture = quedPage.pTexture;

        if (!pTexture)
        {
            // Reached end of queue
            //LOG("WaitForNewPages\n");
            WaitForNewPages();
            continue;
        }

        //if ( pTexture->IsPendingRemove() ) {
        //    pTexture->RemoveRef();
        //    continue;
        //}

        int64_t time = Core::SysMilliseconds();

        HashMap<uint32_t, int64_t>& streamedPages = pTexture->m_StreamedPages;
        auto it = streamedPages.Find(quedPage.PageIndex);
        if (it != streamedPages.End())
        {
            if (it->second + 1000 < time)
            {
                LOG("Re-load page\n");
                it->second = time;
            }
            else
            {
                // Page already loaded. Fetch next page
                LOG("Page already loaded\n");
                continue;
            }
        }
        else
        {
            streamedPages[quedPage.PageIndex] = time;
        }

        LOG("Load\n");

        VTFileOffset physAddress = pTexture->GetPhysAddress(quedPage.PageIndex);

        HK_ASSERT(physAddress != 0);

        VirtualTextureCache::PageTransfer* transfer = pTexture->m_Cache->CreatePageTransfer();

        transfer->PageIndex = quedPage.PageIndex;
        transfer->pTexture = quedPage.pTexture;

        pTexture->ReadPage(physAddress, transfer->Layers);

        //int32_t pagePayLoad = Core::SysMilliseconds() - time;

        //LOG( "pagePayLoad {} msec\n", pagePayLoad );

        // Wait for test
        //Thread::WaitSeconds( 1 );
        //Thread::WaitMilliseconds(500);

        pTexture->m_Cache->MakePageTransferVisible(transfer);
    }

    m_StreamThreadStopped.Signal();
}

void VirtualTextureFeedbackAnalyzer::ClearQueue()
{
    for (int i = 0; i < MAX_QUEUE_LENGTH; i++)
    {
        int p = (m_QueueLoadPos + i) & (MAX_QUEUE_LENGTH - 1);
        VTPageDesc* quedPage = &m_QuedPages[p];

        if (!quedPage->pTexture)
        {
            // end of queue
            break;
        }

        // Remove outdated page from queue
        quedPage->pTexture->RemoveRef();
        quedPage->pTexture = nullptr;
    }

    m_QueueLoadPos = 0;
}

void VirtualTextureFeedbackAnalyzer::SubmitPages(Vector<VTPageDesc> const& pages)
{
    HK_ASSERT(pages.Size() < MAX_QUEUE_LENGTH);

    MutexGuard criticalSection(m_EnqueLock);

    ClearQueue();

    // Refresh queue
    Core::Memcpy(m_QuedPages, pages.ToPtr(), pages.Size() * sizeof(m_QuedPages[0]));
    for (int i = 0; i < pages.Size(); i++)
    {
        VTPageDesc* quedPage = &m_QuedPages[i];
        quedPage->pTexture->AddRef();
    }

    if (pages.Size() > 0)
    {
        m_PageSubmitEvent.Signal();
    }
}

void VirtualTextureFeedbackAnalyzer::Begin(StreamedMemoryGPU* streamedMemory, RHI::IBuffer* streamBuffer, RHI::IResourceTable* rtbl)
{
    unsigned int maxBlockSize = m_Device->GetDeviceCaps(RHI::DEVICE_CAPS_CONSTANT_BUFFER_MAX_BLOCK_SIZE);
    ;

    size_t size = VT_MAX_TEXTURE_UNITS * sizeof(VTUnit);
    if (size > maxBlockSize)
    {
        LOG("VirtualTextureFeedbackAnalyzer::Begin: constant buffer max block size hit\n");
    }

    size_t offset = streamedMemory->AllocateConstant(size);

    rtbl->BindBuffer(6, streamBuffer, offset, size);

    m_Bindings = (VTUnit*)streamedMemory->Map(offset);
    m_NumBindings = 0;

    for (int i = 0; i < VT_MAX_TEXTURE_UNITS; i++)
    {
        if (m_Textures[m_SwapIndex][i])
        {
            m_Textures[m_SwapIndex][i]->RemoveRef();
            m_Textures[m_SwapIndex][i] = nullptr;
        }
    }
}

// Max 11 lods, 256 units
// RGBA8
// 11111111 11111111 1111 11  11 11111111
// -------- -------- ---- --  -- --------
// X_low    Y_low    Lod  Yh  Xh Un
HK_FORCEINLINE void VT_FeedbackUnpack_RGBA8_11LODS_256UNITS(VTFeedbackData const* _Data,
                                                            int& _PageX,
                                                            int& _PageY,
                                                            int& _Lod,
                                                            int& _TextureUnit)
{
    _PageX = _Data->Byte3 | ((_Data->Byte1 & 3) << 8);
    _PageY = _Data->Byte2 | ((_Data->Byte1 & 12) << 6);
    _Lod = _Data->Byte1 >> 4;
    _TextureUnit = _Data->Byte0;
}

void VirtualTextureFeedbackAnalyzer::End()
{
    m_SwapIndex = (m_SwapIndex + 1) & 1;

    DecodePages();

    SubmitPages(m_PendingPages);

    m_Feedbacks.Clear();
}

void VirtualTextureFeedbackAnalyzer::DecodePages()
{
    // TODO:
    // 1. Wait for analyzer thread from previous frame
    // 2. Copy texture bindings and feedback data pointers
    // 3. Run analyzer thread
    // FIXME: This is so fast. Do we really need to separate this for several threads?

    //#define WITH_PENDING_FLAG

    m_PendingPages.Clear();
#if 1
    if (m_NumBindings == 0)
    {
        return;
    }

    VirtualTexture** pTextureBindings = m_Textures[m_SwapIndex];

    ScopedTimer timecheck("VirtualTextureFeedbackAnalyzer::DecodePage");

    int numIterations = 0; // for debug
    int feedbackSize = 0;  // for debug

    for (VTFeedbackChain& feedback : m_Feedbacks)
    {
        const VTFeedbackData* pData = (const VTFeedbackData*)feedback.Data;
        const VTFeedbackData* pEnd = (const VTFeedbackData*)feedback.Data + feedback.Size;

        int x, y, lod, unit;
        int duplicates = 0;

        feedbackSize += feedback.Size;

        for (; pData < pEnd; ++pData)
        {
            if (((const uint32_t*)pData)[0] == ((const uint32_t*)pData)[1] && pData + 1 < pEnd)
            {
                // skip duplicates
                duplicates++;
                continue;
            }

            numIterations++;

            int refs = duplicates + 1;

            duplicates = 0;

            // Decode page
            VT_FeedbackUnpack_RGBA8_11LODS_256UNITS(pData, x, y, lod, unit);

            VirtualTexture* pTexture = pTextureBindings[unit];
            if (!pTexture)
            {
                // No texture binded to unit
                continue;
            }

            if (lod >= pTexture->GetStoredLods())
            {
                continue;
            }

            // Calculate page index
            uint32_t relIndex = QuadTreeGetRelativeFromXY(x, y, lod);
            uint32_t absIndex = QuadTreeRelativeToAbsoluteIndex(relIndex, lod);

            if (!QuadTreeIsIndexValid(absIndex, lod))
            {
                // Index is invalid. Something wrong with decoding.
                continue;
            }

            // Correct mip level
            int maxLod = pTexture->m_PIT[absIndex] >> 4;
            if (maxLod < lod)
            {
                int diff = lod - maxLod;
                x >>= diff;
                y >>= diff;
                relIndex = QuadTreeGetRelativeFromXY(x, y, maxLod);
                absIndex = QuadTreeRelativeToAbsoluteIndex(relIndex, maxLod);
                lod = maxLod;
            }

            byte* pageInfo = &pTexture->m_PIT[absIndex];

            if (*pageInfo & PF_CACHED)
            {
                pTexture->UpdateLRU(absIndex);
                continue;
            }

            // check parent cached
            while (lod > 0)
            {
                unsigned int parentAbsolute = QuadTreeGetParentFromRelative(relIndex, lod);
                byte* parentInfo = &pTexture->m_PIT[parentAbsolute];
                if (*parentInfo & PF_CACHED)
                {
                    // Parent already in cache
                    break;
                }
                --lod;
                pageInfo = parentInfo;
                absIndex = parentAbsolute;
                relIndex = QuadTreeAbsoluteToRelativeIndex(parentAbsolute, lod);
            }

#    if 1
            uint32_t hash = *reinterpret_cast<const uint32_t*>(pData);
#    else
            // Pack textureUnit, lod, x, y to uint32
            pageX = QuadTreeGetXFromRelative(relIndex, pageLod);
            pageY = QuadTreeGetYFromRelative(relIndex, pageLod);
            uint32_t hash = (textureUnit << 24) | (pageLod << 20) | (pageX & 0x300) << 10 | (pageY & 0x300) << 8 | (pageX & 0xff) << 8 | (pageY & 0xff);
#    endif

            // Create list of unique not cached pages

#    ifdef WITH_PENDING_FLAG
            if (!(*pageInfo & PF_PENDING))
            {
                *pageInfo |= PF_PENDING;

                auto& pageDesc = m_PendingPages.Add();
                pageDesc.pTexture = pTexture;
                pageDesc.Hash = hash;
                pageDesc.Refs = 1;
                pageDesc.PageIndex = absIndex;
                pageDesc.Lod = pageLod;

                PendingPagesHash.Insert(hash, m_PendingPages.Size() - 1);
            }
            else
            {
                for (int i = PendingPagesHash.First(hash); i != -1; i = PendingPagesHash.Next(i))
                {
                    if (m_PendingPages[i].Hash == hash)
                    {
                        m_PendingPages[i].Refs++;
                        break;
                    }
                }
            }
#    else
            auto it = m_PendingPageSet.Find(hash);
            if (it != m_PendingPageSet.End())
            {
                auto n = it->second;
                m_PendingPages[n].Refs += refs;
            }
            else
            {
                auto& pageDesc = m_PendingPages.Add();
                pageDesc.pTexture = pTexture;
                pageDesc.Hash = hash;
                pageDesc.Refs = refs;
                pageDesc.PageIndex = absIndex;
                //pageDesc.Lod = lod;

                m_PendingPageSet[hash] = m_PendingPages.Size() - 1;
            }
#    endif
        }
    }

    //LOG( "Num iterations: {}, uniqe pages {}, buffer size {}\n", numIterations, m_PendingPages.Size(), feedbackSize );

    if (!m_PendingPages.IsEmpty())
    {
        m_PendingPageSet.Clear();

#    ifdef WITH_PENDING_FLAG
        // Unset pending flag
        for (VTPageDesc& page : m_PendingPages)
        {
            page.pTexture->m_PIT[page.PageIndex] &= ~PF_PENDING;
        }
#    endif

        struct
        {
            bool operator()(VTPageDesc const& a, VTPageDesc const& b)
            {
                return a.Refs > b.Refs;
            }
        } SortByRefs;

        std::sort(m_PendingPages.Begin(), m_PendingPages.End(), SortByRefs);

        const int MAX_PENDING_PAGES = 100; // TODO: Set from console variable

        int numPendingPages = Math::Min3<int>(MAX_PENDING_PAGES, MAX_QUEUE_LENGTH, m_PendingPages.Size());
        m_PendingPages.Resize(numPendingPages);
    }
#endif
}

void VirtualTextureFeedbackAnalyzer::AddFeedbackData(int feedbackSize, const void* feedbackData)
{
    VTFeedbackChain& feedback = m_Feedbacks.Add();
    feedback.Size = feedbackSize;
    feedback.Data = feedbackData;
}

void VirtualTextureFeedbackAnalyzer::BindTexture(int unit, VirtualTexture* texture)
{
    HK_ASSERT(unit >= 0 && unit < VT_MAX_TEXTURE_UNITS);
    if (texture)
    {
        texture->AddRef();
        if (m_Textures[m_SwapIndex][unit])
        {
            m_Textures[m_SwapIndex][unit]->RemoveRef();
        }
        m_Textures[m_SwapIndex][unit] = texture;

        m_Bindings[unit].MaxLod = texture->GetStoredLods() - 1;
        m_Bindings[unit].Log2Size = texture->GetTextureResolutionLog2();

        m_NumBindings++;
    }
    else
    {
        if (m_Textures[m_SwapIndex][unit])
        {
            m_Textures[m_SwapIndex][unit]->RemoveRef();
            m_Textures[m_SwapIndex][unit] = nullptr;
        }

        m_Bindings[unit].MaxLod = 0;
        m_Bindings[unit].Log2Size = 0;
    }
}

VirtualTexture* VirtualTextureFeedbackAnalyzer::GetTexture(int unit)
{
    HK_ASSERT(unit >= 0 && unit < VT_MAX_TEXTURE_UNITS);
    return m_Textures[m_SwapIndex][unit];
}

HK_NAMESPACE_END
