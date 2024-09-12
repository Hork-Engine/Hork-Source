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

#include "AudioSource.h"

#include <miniaudio/miniaudio.h>

HK_NAMESPACE_BEGIN

AudioSource::AudioSource(int inFrameCount, int inSampleRate, int inSampleBits, int inChannels, HeapBlob blob) :
    m_Blob(std::move(blob))
{
    m_FrameCount = inFrameCount;
    m_Channels = inChannels;
    m_SampleBits = inSampleBits;
    m_SampleStride = (m_SampleBits >> 3) << (m_Channels - 1);
    m_SampleRate = inSampleRate;
    m_bIsEncoded = true;
    m_Frames = nullptr;
}

AudioSource::AudioSource(int inFrameCount, int inSampleRate, int inSampleBits, int inChannels, const void* inFrames)
{
    m_FrameCount = inFrameCount;
    m_Channels = inChannels;
    m_SampleBits = inSampleBits;
    m_SampleStride = (m_SampleBits >> 3) << (m_Channels - 1);
    m_SampleRate = inSampleRate;
    m_Blob.Reset(m_FrameCount * m_SampleStride, inFrames);
    m_Frames = m_Blob.GetData();
    m_bIsEncoded = false;
}


namespace
{
    size_t Read(ma_decoder* pDecoder, void* pBufferOut, size_t bytesToRead)
    {
        IBinaryStreamReadInterface* file = (IBinaryStreamReadInterface*)pDecoder->pUserData;
        return file->Read(pBufferOut, bytesToRead);
    }

    ma_bool32 Seek(ma_decoder* pDecoder, ma_int64 byteOffset, ma_seek_origin origin)
    {
        IBinaryStreamReadInterface* file = (IBinaryStreamReadInterface*)pDecoder->pUserData;
        switch (origin)
        {
        case ma_seek_origin_start:
            return file->SeekSet(byteOffset);
        case ma_seek_origin_current:
            return file->SeekCur(byteOffset);
        case ma_seek_origin_end: // Not used by decoders.
            return file->SeekEnd(byteOffset);
        }
        return false;
    }
} // namespace

bool DecodeAudio(IBinaryStreamReadInterface& inStream, AudioResample const& inResample, Ref<AudioSource>& outSource)
{
    ma_decoder_config config = ma_decoder_config_init(inResample.bForce8Bit ? ma_format_u8 : ma_format_s16, inResample.bForceMono ? 1 : 0, inResample.SampleRate);

    ma_decoder decoder;
    ma_result result = ma_decoder_init(Read, Seek, &inStream, &config, &decoder);
    if (result != MA_SUCCESS)
    {
        LOG("DecodeAudio: failed to load {}\n", inStream.GetName());
        return false;
    }

    MemoryHeap& tempHeap = Core::GetHeapAllocator<HEAP_TEMP>();

    const size_t tempSize = 8192;
    byte* temp = (byte*)tempHeap.Alloc(tempSize);

    ma_uint64 totalFramesRead = 0;
    ma_uint64 framesCapacity = 0;
    void* pFrames = nullptr;
    const int sampleBits = inResample.bForce8Bit ? 8 : 16;
    const int stride = (sampleBits >> 3) * decoder.outputChannels;
    const ma_uint64 framesToReadRightNow = tempSize / stride;
    for (;;)
    {
        ma_uint64 framesJustRead = ma_decoder_read_pcm_frames(&decoder, temp, framesToReadRightNow);
        if (framesJustRead == 0)
            break;

        ma_uint64 currentFrameCount = totalFramesRead + framesJustRead;

        // resize dest buffer
        if (framesCapacity < currentFrameCount)
        {
            ma_uint64 newFramesCap = framesCapacity * 2;
            if (newFramesCap < currentFrameCount)
                newFramesCap = currentFrameCount;
            ma_uint64 newFramesBufferSize = newFramesCap * stride;
            if (newFramesBufferSize > MA_SIZE_MAX)
                break;
            pFrames = tempHeap.Realloc(pFrames, (size_t)newFramesBufferSize, 16);
            framesCapacity = newFramesCap;
        }

        // Copy frames
        Core::Memcpy((ma_int8*)pFrames + totalFramesRead * stride, temp, (size_t)(framesJustRead * stride));
        totalFramesRead += framesJustRead;

        // Check EOF
        if (framesJustRead != framesToReadRightNow)
        {
            // EOF reached
            break;
        }
    }

    tempHeap.Free(temp);
    ma_decoder_uninit(&decoder);

    if (totalFramesRead == 0)
        return false;

    outSource = MakeRef<AudioSource>(totalFramesRead, inResample.SampleRate, sampleBits, decoder.outputChannels, pFrames);

    tempHeap.Free(pFrames);

    return true;
}

bool ReadAudioInfo(IBinaryStreamReadInterface& inStream, AudioResample const& inResample, AudioFileInfo* outInfo)
{
    ma_decoder_config config = ma_decoder_config_init(inResample.bForce8Bit ? ma_format_u8 : ma_format_s16, inResample.bForceMono ? 1 : 0, inResample.SampleRate);

    ma_decoder decoder;
    ma_result result = ma_decoder_init(Read, Seek, &inStream, &config, &decoder);
    if (result != MA_SUCCESS)
    {
        LOG("ReadAudioInfo: failed to load {}\n", inStream.GetName());
        return false;
    }

    outInfo->Channels = decoder.outputChannels;
    outInfo->SampleBits = inResample.bForce8Bit ? 8 : 16;
    outInfo->FrameCount = ma_decoder_get_length_in_pcm_frames(&decoder); // For MP3's, this will decode the entire file

    // ma_decoder_get_length_in_pcm_frames will always return 0 for Vorbis decoders.
    // This is due to a limitation with stb_vorbis in push mode which is what miniaudio
    // uses internally.
    if (outInfo->FrameCount == 0)
    { // stb_vorbis :(

        MemoryHeap& tempHeap = Core::GetHeapAllocator<HEAP_TEMP>();

        const size_t tempSize = 8192;
        byte* temp = (byte*)tempHeap.Alloc(tempSize);

        ma_uint64 totalFramesRead = 0;
        const int stride = (outInfo->SampleBits >> 3) * outInfo->Channels;
        const ma_uint64 framesToReadRightNow = tempSize / stride;
        for (;;)
        {
            ma_uint64 framesJustRead = ma_decoder_read_pcm_frames(&decoder, temp, framesToReadRightNow);
            if (framesJustRead == 0)
                break;
            totalFramesRead += framesJustRead;
            // Check EOF
            if (framesJustRead != framesToReadRightNow)
                break;
        }
        outInfo->FrameCount = totalFramesRead;

        tempHeap.Free(temp);
    }

    ma_decoder_uninit(&decoder);

    return outInfo->FrameCount > 0;
}

HK_NAMESPACE_END
