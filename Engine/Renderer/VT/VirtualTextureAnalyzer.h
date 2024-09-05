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

#include "VirtualTexture.h"

#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

constexpr int VT_MAX_TEXTURE_UNITS = 256;

struct VTFeedbackChain
{
    int Size;
    const void* Data;
};

struct VTFeedbackData
{
    byte Byte1; // B
    byte Byte2; // G
    byte Byte3; // R
    byte Byte0; // A
};

struct VTPageDesc
{
    VirtualTexture* pTexture;
    uint32_t Hash;
    uint32_t Refs;
    uint32_t PageIndex;
    //uint8_t  Lod;
};

struct VTUnit
{
    float MaxLod;
    float Log2Size;
};

class VirtualTextureFeedbackAnalyzer : public RefCounted
{
public:
    VirtualTextureFeedbackAnalyzer();
    virtual ~VirtualTextureFeedbackAnalyzer();

    void AddFeedbackData(int FeedbackSize, const void* FeedbackData);

    /// Bind texture once per frame between Begin() and End()
    void BindTexture(int Unit, VirtualTexture* Texture);

    VirtualTexture* GetTexture(int Unit);

    void Begin(class StreamedMemoryGPU* StreamedMemory);

    void End();

    bool HasBindings() const { return NumBindings > 0; }

private:
    void DecodePages();
    void ClearQueue();
    void SubmitPages(Vector<VTPageDesc> const& Pages);
    void WaitForNewPages();
    void StreamThreadMain();

    // Per-frame texture bindings
    VirtualTexture* Textures[2][VT_MAX_TEXTURE_UNITS];
    int SwapIndex;

    // Per-frame binding data for shaders
    VTUnit* Bindings;
    int NumBindings;

    // Actually feedback data is from previous frame
    Vector<VTFeedbackChain> Feedbacks;

    // Unique pages from feedback
    HashMap<uint32_t, uint32_t> PendingPageSet;
    Vector<VTPageDesc> PendingPages;

    // Page queue for async loading
    enum
    {
        MAX_QUEUE_LENGTH = 256
    };
    VTPageDesc QuedPages[MAX_QUEUE_LENGTH];
    int QueueLoadPos; // pointer to a page that will be loaded first

    Thread StreamThread;
    Mutex EnqueLock;
    SyncEvent PageSubmitEvent;
    SyncEvent StreamThreadStopped;
    AtomicBool bStopStreamThread;
};

HK_NAMESPACE_END
