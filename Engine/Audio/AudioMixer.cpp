/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "AudioMixer.h"
#include "HRTF.h"
#include "Freeverb.h"
#include "AudioDecoder.h"

#include <Engine/Core/Platform/Logger.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

HK_NAMESPACE_BEGIN

ConsoleVar Snd_MixAhead("Snd_MixAhead"s, "0.1"s);
ConsoleVar Snd_VolumeRampSize("Snd_VolumeRampSize"s, "16"s);
ConsoleVar Snd_HRTF("Snd_HRTF"s, "1"s);

#if 0
ConsoleVar Rev_RoomSize( "Rev_RoomSize"s,"0.5"s );
ConsoleVar Rev_Damp(  "Rev_Damp"s,  "0.5"s );
ConsoleVar Rev_Wet(  "Rev_Wet"s,  "0.33"s );
ConsoleVar Rev_Dry(  "Rev_Dry"s,  "1"s );
ConsoleVar Rev_Width(  "Rev_Width"s,  "1"s );
#endif

// u8 to s32 sample convertion
struct SampleLookup8Bit
{
    int Data[32][256];

    int16_t ToShort[256];

    SampleLookup8Bit()
    {
        for (int v = 0; v < 32; v++)
        {
            int vol = v * 8 * 256;
            for (int s = 0; s < 256; s++)
            {
                Data[v][(s + 128) & 0xff] = ((s < 128) ? s : s - 256) * vol;
            }
        }

        for (int s = 0; s < 256; s++)
        {
            ToShort[(s + 128) & 0xff] = ((s < 128) ? s : s - 256) * 255;
        }
    }
};

static const SampleLookup8Bit SampleLookup8Bit;

AudioMixer::AudioMixer(AudioDevice* _Device) :
    m_pDevice(_Device), m_DeviceRawPtr(_Device), m_bAsync(false), m_RenderFrame(0)
{
    m_Hrtf = MakeUnique<AudioHRTF>(m_DeviceRawPtr->GetSampleRate());
    m_ReverbFilter = MakeUnique<Freeverb>(m_DeviceRawPtr->GetSampleRate());

    m_Channels = nullptr;
    m_ChannelsTail = nullptr;
    m_PendingList = nullptr;
    m_PendingListTail = nullptr;

    m_TotalChannels.StoreRelaxed(0);
    m_NumActiveChannels.StoreRelaxed(0);
}

AudioMixer::~AudioMixer()
{
    StopAsync();

    // Add pendings (if any)
    AddPendingChannels();

    // Free channels
    AudioChannel* next;
    for (AudioChannel* chan = m_Channels; chan; chan = next)
    {
        next = chan->Next;
        chan->RemoveRef();
    }

    AudioChannel::FreePool();
}

void AudioMixer::StartAsync()
{
    m_bAsync = true;
    m_DeviceRawPtr->SetMixerCallback([this](uint8_t* pTransferBuffer, int TransferBufferSizeInFrames, int FrameNum, int MinFramesToRender)
                                     { UpdateAsync(pTransferBuffer, TransferBufferSizeInFrames, FrameNum, MinFramesToRender); });
}

void AudioMixer::StopAsync()
{
    m_bAsync = false;
    m_DeviceRawPtr->SetMixerCallback(nullptr);
}

void AudioMixer::SubmitChannel(AudioChannel* Channel)
{
    Channel->AddRef();

    SpinLockGuard lockGuard(m_SubmitLock);

    HK_ASSERT(!INTRUSIVE_EXISTS(Channel, Next, Prev, m_PendingList, m_PendingListTail));
    INTRUSIVE_ADD(Channel, Next, Prev, m_PendingList, m_PendingListTail);
}

void AudioMixer::AddPendingChannels()
{
    AudioChannel* submittedChannels = nullptr;

    {
        SpinLockGuard lockGuard(m_SubmitLock);

        submittedChannels = m_PendingList;

        INTRUSIVE_MERGE(Next, Prev,
                        m_Channels, m_ChannelsTail,
                        m_PendingList, m_PendingListTail);
    }

    int count = 0;
    for (AudioChannel* chan = submittedChannels; chan; chan = chan->Next)
    {
        if (chan->pStream && !chan->bVirtual)
        {
            chan->pStream->SeekToFrame(chan->PlaybackPos.Load());
        }
        count++;
    }

    m_TotalChannels.Add(count);
}

void AudioMixer::RejectChannel(AudioChannel* Channel)
{
    INTRUSIVE_REMOVE(Channel, Next, Prev, m_Channels, m_ChannelsTail);
    Channel->RemoveRef();
    m_TotalChannels.Decrement();
}

void AudioMixer::Update()
{
    if (m_bAsync)
    {
        LOG("AudioMixer::Update: mixer is running in async thread\n");
        return;
    }

    int64_t frameNum;

    m_pTransferBuffer = m_DeviceRawPtr->MapTransferBuffer(&frameNum);

    if (m_RenderFrame < frameNum)
    {
        LOG("AudioMixer::Update: Missing frames {}\n", frameNum - m_RenderFrame);

        m_RenderFrame = frameNum;
    }

    int framesToRender = Snd_MixAhead.GetFloat() * m_DeviceRawPtr->GetSampleRate();
    framesToRender = Math::Clamp(framesToRender, 0, m_DeviceRawPtr->GetTransferBufferSizeInFrames());

    int64_t endFrame = frameNum + framesToRender;

    RenderChannels(endFrame);

    m_DeviceRawPtr->UnmapTransferBuffer();

#if 0
    // Update reverb
    if ( Rev_RoomSize.IsModified() ) {
        ReverbFilter->SetRoomSize( Rev_RoomSize.GetFloat() );
        Rev_RoomSize.UnmarkModified();
    }
    if ( Rev_Damp.IsModified() ) {
        ReverbFilter->SetDamp( Rev_Damp.GetFloat() );
        Rev_Damp.UnmarkModified();
    }
    if ( Rev_Wet.IsModified() ) {
        ReverbFilter->SetWet( Rev_Wet.GetFloat() );
        Rev_Wet.UnmarkModified();
    }
    if ( Rev_Dry.IsModified() ) {
        ReverbFilter->SetDry( Rev_Dry.GetFloat() );
        Rev_Dry.UnmarkModified();
    }
    if ( Rev_Width.IsModified() ) {
        ReverbFilter->SetWidth( Rev_Width.GetFloat() );
        Rev_Width.UnmarkModified();
    }
#endif
}

void AudioMixer::UpdateAsync(uint8_t* _pTransferBuffer, int TransferBufferSizeInFrames, int FrameNum, int MinFramesToRender)
{
    m_pTransferBuffer = _pTransferBuffer;

    if (m_RenderFrame < FrameNum)
    {
        m_RenderFrame = FrameNum;
    }

#if 0
    int framesToRender = Snd_MixAhead.GetFloat() * m_DeviceRawPtr->GetSampleRate();
    framesToRender = Math::Clamp( framesToRender, MinFramesToRender, TransferBufferSizeInFrames );
#else
    int framesToRender = MinFramesToRender;
#endif

    int64_t endFrame = FrameNum + framesToRender;

    RenderChannels(endFrame);
}

void AudioMixer::RenderChannels(int64_t EndFrame)
{
    int numActiveChan = m_NumActiveChannels.Load();

    if (m_RenderFrame < EndFrame)
    {
        numActiveChan = 0;
    }

    AddPendingChannels();

    while (m_RenderFrame < EndFrame)
    {
        int64_t end = EndFrame;
        if (EndFrame - m_RenderFrame > m_RenderBufferSize)
        {
            end = m_RenderFrame + m_RenderBufferSize;
        }

        int frameCount = end - m_RenderFrame;

        Platform::ZeroMem(m_RenderBuffer, frameCount * sizeof(SamplePair));

        AudioChannel* next;
        for (AudioChannel* chan = m_Channels; chan; chan = next)
        {
            next = chan->Next;

            if (chan->GetRefCount() == 1)
            {
                // Channel was removed from main thread
                RejectChannel(chan);
                continue;
            }

            bool bSeek = false;

            {
                SpinLockGuard guard(chan->Lock);
                m_NewVol[0] = chan->bPaused_COMMIT ? 0 : chan->Volume_COMMIT[0];
                m_NewVol[1] = chan->bPaused_COMMIT ? 0 : chan->Volume_COMMIT[1];
                m_NewDir = chan->LocalDir_COMMIT;
                m_bSpatializedChannel = chan->bSpatializedStereo_COMMIT;
                m_bChannelPaused = chan->bPaused_COMMIT;
                m_PlaybackPos = chan->PlaybackPos.Load();
                if (chan->PlaybackPos_COMMIT >= 0)
                {
                    bSeek = chan->PlaybackPos_COMMIT != m_PlaybackPos;
                    m_PlaybackPos = chan->PlaybackPos_COMMIT;
                    chan->PlaybackPos_COMMIT = -1;
                }
            }

            if (bSeek && !chan->bVirtual && chan->pStream)
            {
                chan->pStream->SeekToFrame(m_PlaybackPos);
            }

            if (m_NewVol[0] == 0 && m_NewVol[1] == 0 && chan->Volume[0] == 0 && chan->Volume[1] == 0)
            {
                if (!chan->bVirtual)
                {
                    bool bLooped = chan->GetLoopStart() >= 0;
                    if (chan->bVirtualizeWhenSilent || bLooped || m_bChannelPaused)
                    {
                        chan->bVirtual = true;
                    }
                    else
                    {
                        chan->Stopped.Store(true);
                        RejectChannel(chan);
                        continue;
                    }
                }
            }
            else
            {
                // Devirtualize
                if (chan->bVirtual)
                {
                    if (chan->pStream)
                    {
                        chan->pStream->SeekToFrame(m_PlaybackPos);
                    }
                    chan->bVirtual = false;
                }
            }

            if (!chan->bVirtual)
            {
                numActiveChan++;
            }

            if (m_bChannelPaused && chan->bVirtual)
            {
                // Only virtual channels is really paused
                chan->PlaybackEnd = 0;
                continue;
            }

            // Playing is just started or unpaused
            if (chan->PlaybackEnd == 0)
            {
                chan->PlaybackEnd = m_RenderFrame + (chan->FrameCount - m_PlaybackPos);
            }

            if (chan->pStream)
            {
                RenderStream(chan, end);
            }
            else
            {
                RenderChannel(chan, end);
            }

            chan->PlaybackPos.Store(m_PlaybackPos);
        }

        WriteToTransferBuffer((int const*)m_RenderBuffer, end);
        m_RenderFrame = end;
    }

    m_NumActiveChannels.Store(numActiveChan);
}

static void ConvertFramesToMonoF32(const void* pFramesIn, int FrameCount, int SampleBits, int Channels, float* pFramesOut)
{
    if (SampleBits == 8)
    {
        // Lookup at max volume
        int const*  lookup     = SampleLookup8Bit.Data[31];
        const float intToFloat = 1.0f / 256 / 32767;

        uint8_t const* frames = (uint8_t const*)pFramesIn;

        // Mono
        if (Channels == 1)
        {
            for (int i = 0; i < FrameCount; i++)
            {
                pFramesOut[i] = lookup[frames[i]] * intToFloat;
            }
            return;
        }

        // Combine stereo channels
        for (int i = 0; i < FrameCount; i++)
        {
            pFramesOut[i] = (lookup[frames[0]] + lookup[frames[1]]) * (intToFloat * 0.5f); // average
            frames += 2;
        }
        return;
    }

    if (SampleBits == 16)
    {
        const float intToFloat = 1.0f / 32767;

        int16_t const* frames = (int16_t const*)pFramesIn;

        // Mono
        if (Channels == 1)
        {
            for (int i = 0; i < FrameCount; i++)
            {
                pFramesOut[i] = frames[i] * intToFloat;
            }
            return;
        }

        // Combine stereo channels
        for (int i = 0; i < FrameCount; i++)
        {
            pFramesOut[i] = ((int)frames[0] + (int)frames[1]) * (intToFloat * 0.5f); // average
            frames += 2;
        }
        return;
    }

    if (SampleBits == 32)
    {
        float const* frames = (float const*)pFramesIn;

        // Mono
        if (Channels == 1)
        {
            Platform::Memcpy(pFramesOut, pFramesIn, FrameCount * sizeof(float));
            return;
        }

        // Combine stereo channels
        for (int i = 0; i < FrameCount; i++)
        {
            pFramesOut[i] = (frames[0] + frames[1]) * 0.5f; // average
            frames += 2;
        }
        return;
    }

    // Should never happen, but just in case...
    HK_ASSERT(0);
}

// Read frames from current playback position and convert to f32 format.
void AudioMixer::ReadFramesF32(AudioChannel* Chan, int FramesToRead, int HistoryExtraFrames, float* pFrames)
{
    int         frameCount  = Chan->FrameCount;
    const byte* pRawSamples = (const byte*)Chan->GetFrames();
    int         stride      = Chan->SampleStride;
    int         sampleBits  = Chan->SampleBits;
    int         channels    = Chan->Channels;
    int         inloop      = Chan->GetLoopStart() >= 0 ? Chan->LoopsCount : 0;
    int         start       = inloop ? Chan->GetLoopStart() : 0;
    float*      frames      = pFrames + HistoryExtraFrames;

    for (int from = m_PlaybackPos; HistoryExtraFrames > 0;)
    {
        int framesToCopy;
        if (from - HistoryExtraFrames < start)
        {
            framesToCopy = from - start;
        }
        else
        {
            framesToCopy = HistoryExtraFrames;
        }

        HistoryExtraFrames -= framesToCopy;

        ConvertFramesToMonoF32(pRawSamples + (from - framesToCopy) * stride, framesToCopy, sampleBits, channels, pFrames + HistoryExtraFrames);

        if (!inloop && HistoryExtraFrames > 0)
        {
            Platform::ZeroMem(pFrames, HistoryExtraFrames * sizeof(float));
            break;
        }

        from = frameCount;
        inloop--;
    }

    int p = m_PlaybackPos;
    while (FramesToRead > 0)
    {
        const byte* samples = pRawSamples + p * stride;

        int framesToCopy = frameCount - p;
        if (framesToCopy > FramesToRead)
        {
            framesToCopy = FramesToRead;
        }

        FramesToRead -= framesToCopy;

        ConvertFramesToMonoF32(samples, framesToCopy, sampleBits, channels, frames);

        frames += framesToCopy;
        p += framesToCopy;

        if (p >= frameCount)
        {
            if (Chan->GetLoopStart() >= 0)
            {
                p = Chan->GetLoopStart();
            }
            else
            {
                Platform::ZeroMem(frames, FramesToRead * sizeof(float));
                break;
            }
        }
    }
}

void AudioMixer::RenderChannel(AudioChannel* Chan, int64_t EndFrame)
{
    int64_t frameNum = m_RenderFrame;
    int         clipFrameCount = Chan->FrameCount;
    byte const* pRawSamples    = (byte const*)Chan->GetFrames();
    int         stride         = Chan->SampleStride;

    while (frameNum < EndFrame)
    {
        int frameCount;
        if (Chan->PlaybackEnd < EndFrame)
        {
            frameCount = Chan->PlaybackEnd - frameNum;
        }
        else
        {
            frameCount = EndFrame - frameNum;
        }

        if (frameCount > 0)
        {
            int framesToRender;
            if (m_PlaybackPos + frameCount <= clipFrameCount)
            {
                framesToRender = frameCount;
            }
            else
            {
                // Should never happen
                framesToRender = clipFrameCount - m_PlaybackPos;
            }

            if (framesToRender > 0)
            {
                if (!Chan->bVirtual)
                {
                    void const* pFrames = pRawSamples + m_PlaybackPos * stride;

                    SamplePair* pBuffer = &m_RenderBuffer[frameNum - m_RenderFrame];

                    if (Snd_HRTF && Chan->bSpatializedStereo_COMMIT)
                    {
                        RenderFramesHRTF(Chan, framesToRender, pBuffer);
                    }
                    else
                    {
                        RenderFrames(Chan, pFrames, framesToRender, pBuffer);
                    }

                    Chan->Volume[0] = m_NewVol[0];
                    Chan->Volume[1] = m_NewVol[1];
                }

                m_PlaybackPos += framesToRender;
            }

            frameNum += frameCount;
        }

        if (frameNum >= Chan->PlaybackEnd)
        {
            if (Chan->GetLoopStart() >= 0)
            {
                m_PlaybackPos = Chan->GetLoopStart();
                Chan->PlaybackEnd = frameNum + (clipFrameCount - m_PlaybackPos);
                Chan->LoopsCount++;
            }
            else
            {
                m_PlaybackPos = clipFrameCount;
                break;
            }
        }
    }
}

void AudioMixer::RenderStream(AudioChannel* Chan, int64_t EndFrame)
{
    int64_t frameNum = m_RenderFrame;
    int     clipFrameCount = Chan->FrameCount;
    int     stride         = Chan->SampleStride;

    while (frameNum < EndFrame)
    {
        int frameCount;
        if (Chan->PlaybackEnd < EndFrame)
        {
            frameCount = Chan->PlaybackEnd - frameNum;
        }
        else
        {
            frameCount = EndFrame - frameNum;
        }

        if (frameCount > 0)
        {
            int framesToRender;
            if (m_PlaybackPos + frameCount <= clipFrameCount)
            {
                framesToRender = frameCount;
            }
            else
            {
                // Should never happen
                framesToRender = clipFrameCount - m_PlaybackPos;
            }

            if (!Chan->bVirtual)
            {
                m_TempFrames.ResizeInvalidate(framesToRender * stride);

                framesToRender = Chan->pStream->ReadFrames(m_TempFrames.ToPtr(), framesToRender, framesToRender * stride);

                if (framesToRender > 0)
                {
                    RenderFrames(Chan, m_TempFrames.ToPtr(), framesToRender, &m_RenderBuffer[frameNum - m_RenderFrame]);

                    Chan->Volume[0] = m_NewVol[0];
                    Chan->Volume[1] = m_NewVol[1];
                }
            }

            m_PlaybackPos += framesToRender;

            frameNum += frameCount;
        }

        if (frameNum >= Chan->PlaybackEnd)
        {
            if (Chan->GetLoopStart() >= 0)
            {
                if (!Chan->bVirtual)
                {
                    Chan->pStream->SeekToFrame(Chan->GetLoopStart());
                }
                m_PlaybackPos = Chan->GetLoopStart();
                Chan->PlaybackEnd = frameNum + (clipFrameCount - m_PlaybackPos);
                Chan->LoopsCount++;
            }
            else
            {
                m_PlaybackPos = clipFrameCount;
                break;
            }
        }
    }
}

void AudioMixer::MakeVolumeRamp(const int CurVol[2], const int _NewVol[2], int FrameCount, int Scale)
{
    if (CurVol[0] == _NewVol[0] && CurVol[1] == _NewVol[1])
    {
        m_VolumeRampSize = 0;
        return;
    }

    m_VolumeRampSize = Math::Min3((int)HK_ARRAY_SIZE(m_VolumeRampL), FrameCount, Snd_VolumeRampSize.GetInteger());
    if (m_VolumeRampSize < 0)
    {
        m_VolumeRampSize = 0;
        return;
    }

    float increment0 = (float)(_NewVol[0] - CurVol[0]) / (m_VolumeRampSize * Scale);
    float increment1 = (float)(_NewVol[1] - CurVol[1]) / (m_VolumeRampSize * Scale);

    float lvolf = (float)CurVol[0] / Scale;
    float rvolf = (float)CurVol[1] / Scale;

    for (int i = 0; i < m_VolumeRampSize; i++)
    {
        lvolf += increment0;
        rvolf += increment1;

        m_VolumeRampL[i] = lvolf;
        m_VolumeRampR[i] = rvolf;
    }
}

void AudioMixer::RenderFramesHRTF(AudioChannel* Chan, int FrameCount, SamplePair* pBuffer)
{
    int total = FrameCount;

    // align length to block size
    int blocksize = HRTF_BLOCK_LENGTH;
    if (total % blocksize)
    {
        int numblocks = total / blocksize + 1;
        total         = numblocks * blocksize;
    }

    int historyExtraFrames = m_Hrtf->GetFrameCount() - 1;

    // Read frames from current playback position and convert to f32 format
    m_FramesF32.ResizeInvalidate((total + historyExtraFrames) * sizeof(float));
    ReadFramesF32(Chan, total, historyExtraFrames, m_FramesF32.ToPtr());

    // Reallocate (if need) container for filtered samples
    m_StreamF32.ResizeInvalidate(sizeof(SamplePair) * total);

    // Apply HRTF filter
    Float3 dir;
    m_Hrtf->ApplyHRTF(Chan->LocalDir, m_NewDir, m_FramesF32.ToPtr(), total, (float*)m_StreamF32.ToPtr(), dir);
    Chan->LocalDir = dir;

    // Make volume ramp
    m_VolumeRampSize = 0;
    if (Chan->Volume[0] != m_NewVol[0] || Chan->Volume[1] != m_NewVol[1])
    {
        m_VolumeRampSize = Math::Min3((int)HK_ARRAY_SIZE(m_VolumeRampL), FrameCount, Snd_VolumeRampSize.GetInteger());
        if (m_VolumeRampSize > 0)
        {
            float scale = 256.0f / m_Hrtf->GetFilterSize();
            float increment0 = (float)(m_NewVol[0] - Chan->Volume[0]) / m_VolumeRampSize * scale;
            float lvolf      = (float)Chan->Volume[0] * scale;
            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                lvolf += increment0;
                m_VolumeRampL[i] = lvolf;
            }
        }
    }

    SamplePair* pStreamF32 = m_StreamF32.ToPtr();

#if 0
    // Adjust volume
    float vol = float( 65536 / 256 ) * m_NewVol[0] / m_Hrtf->GetFilterSize();
    for ( int i = 0; i < m_VolumeRampSize; i++ ) {
        pStreamF32[i].Chanf[0] = pStreamF32[i].Chanf[0] * m_VolumeRampL[i];
        pStreamF32[i].Chanf[1] = pStreamF32[i].Chanf[1] * m_VolumeRampL[i];
    }
    for ( int i = m_VolumeRampSize ; i < FrameCount; i++ ) {
        pStreamF32[i].Chanf[0] = pStreamF32[i].Chanf[0] * vol;
        pStreamF32[i].Chanf[1] = pStreamF32[i].Chanf[1] * vol;
    }

    float * out = (float *)malloc( FrameCount * 2 * sizeof( float ) );

    ReverbFilter->ProcessReplace( &pStreamF32[0].Chanf[0], &pStreamF32[0].Chanf[1], &out[0], &out[1], FrameCount, 2 );

    // Mix with output stream
    for ( int i = 0 ; i < FrameCount; i++ ) {
        pBuffer[i].Chan[0] += out[i*2] ;
        pBuffer[i].Chan[1] += out[i*2+1];
    }

    free( out );
#else
    // Mix with output stream
    float vol = float(65536 / 256) * m_NewVol[0] / m_Hrtf->GetFilterSize();
    for (int i = 0; i < m_VolumeRampSize; i++)
    {
        pBuffer[i].Chan[0] += pStreamF32[i].Chanf[0] * m_VolumeRampL[i];
        pBuffer[i].Chan[1] += pStreamF32[i].Chanf[1] * m_VolumeRampL[i];
    }
    for (int i = m_VolumeRampSize; i < FrameCount; i++)
    {
        pBuffer[i].Chan[0] += pStreamF32[i].Chanf[0] * vol;
        pBuffer[i].Chan[1] += pStreamF32[i].Chanf[1] * vol;
    }
#endif
}

void AudioMixer::RenderFrames(AudioChannel* Chan, const void* pFrames, int FrameCount, SamplePair* pBuffer)
{
    int sampleBits = Chan->SampleBits;
    int channels   = Chan->Channels;

    // Render 8bit audio
    if (sampleBits == 8)
    {
        uint8_t const* frames = (uint8_t const*)pFrames;

#if 0
        int lvol = Math::Min( m_NewVol[0] / 256, 255 );
        int rvol = Math::Min( m_NewVol[1] / 256, 255 );

        int const * ls = SampleLookup8Bit.Data[lvol >> 3];
        int const * rs = SampleLookup8Bit.Data[rvol >> 3];

        // Mono
        if ( channels == 1 ) {
            MakeVolumeRamp( Chan->Volume, m_NewVol, FrameCount, 256 );

            for ( int i = 0; i < m_VolumeRampSize; i++ ) {
                pBuffer[i].Chan[0] += ls[frames[i]]*m_VolumeRampL[i];
                pBuffer[i].Chan[1] += rs[frames[i]]*m_VolumeRampR[i];
            }

            for ( int i = m_VolumeRampSize ; i < FrameCount; i++ ) {
                pBuffer[i].Chan[0] += ls[frames[i]];
                pBuffer[i].Chan[1] += rs[frames[i]];
            }
            return;
        }
#else
        int lvol = m_NewVol[0];
        int rvol = m_NewVol[1];

        // Mono
        if (channels == 1)
        {
            lvol /= 256;
            rvol /= 256;

            MakeVolumeRamp(Chan->Volume, m_NewVol, FrameCount, 256);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                pBuffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[i]] * m_VolumeRampL[i];
                pBuffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[i]] * m_VolumeRampR[i];
            }

            for (int i = m_VolumeRampSize; i < FrameCount; i++)
            {
                pBuffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[i]] * lvol;
                pBuffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[i]] * rvol;
            }
            return;
        }
#endif

        // Spatialized stereo
        if (m_bSpatializedChannel)
        {
            // Combine stereo channels

#if 0

            MakeVolumeRamp( Chan->Volume, m_NewVol, FrameCount, 512 );

            for ( int i = 0; i < m_VolumeRampSize; i++ ) {
                pBuffer[i].Chan[0] += ( ls[frames[0]] + ls[frames[1]] ) * m_VolumeRampL[i];
                pBuffer[i].Chan[1] += ( rs[frames[0]] + rs[frames[1]] ) * m_VolumeRampR[i];

                frames += 2;
            }

            for ( int i = m_VolumeRampSize ; i < FrameCount; i++ ) {
                pBuffer[i].Chan[0] += ( ls[frames[0]] + ls[frames[1]] ) / 2;
                pBuffer[i].Chan[1] += ( rs[frames[0]] + rs[frames[1]] ) / 2;

                frames += 2;
            }

#else

            lvol /= 512;
            rvol /= 512;

            MakeVolumeRamp(Chan->Volume, m_NewVol, FrameCount, 512);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                pBuffer[i].Chan[0] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * m_VolumeRampL[i];
                pBuffer[i].Chan[1] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * m_VolumeRampR[i];

                frames += 2;
            }

            for (int i = m_VolumeRampSize; i < FrameCount; i++)
            {
                pBuffer[i].Chan[0] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * lvol;
                pBuffer[i].Chan[1] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * rvol;

                frames += 2;
            }
#endif

            return;
        }

        // Background music/etc
#if 0
        MakeVolumeRamp( Chan->Volume, m_NewVol, FrameCount, 256 );
        for ( int i = 0; i < m_VolumeRampSize; i++ ) {
            pBuffer[i].Chan[0] += ls[frames[0]] * m_VolumeRampL[i];
            pBuffer[i].Chan[1] += rs[frames[1]] * m_VolumeRampR[i];

            frames += 2;
        }
        for ( int i = m_VolumeRampSize ; i < FrameCount; i++ ) {
            pBuffer[i].Chan[0] += ls[frames[0]];
            pBuffer[i].Chan[1] += rs[frames[1]];

            frames += 2;
        }
#else
        lvol /= 256;
        rvol /= 256;

        MakeVolumeRamp(Chan->Volume, m_NewVol, FrameCount, 256);
        for (int i = 0; i < m_VolumeRampSize; i++)
        {
            pBuffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[0]] * m_VolumeRampL[i];
            pBuffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[1]] * m_VolumeRampR[i];

            frames += 2;
        }
        for (int i = m_VolumeRampSize; i < FrameCount; i++)
        {
            pBuffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[0]] * lvol;
            pBuffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[1]] * rvol;

            frames += 2;
        }
#endif
        return;
    }

    // Render 16bit audio
    if (sampleBits == 16)
    {
        int lvol = m_NewVol[0];
        int rvol = m_NewVol[1];

        int16_t const* frames = (int16_t const*)pFrames;

        // Mono
        if (channels == 1)
        {
            lvol /= 256;
            rvol /= 256;

            MakeVolumeRamp(Chan->Volume, m_NewVol, FrameCount, 256);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                pBuffer[i].Chan[0] += frames[i] * m_VolumeRampL[i];
                pBuffer[i].Chan[1] += frames[i] * m_VolumeRampR[i];
            }

            for (int i = m_VolumeRampSize; i < FrameCount; i++)
            {
                pBuffer[i].Chan[0] += frames[i] * lvol;
                pBuffer[i].Chan[1] += frames[i] * rvol;
            }
            return;
        }

        // Spatialized stereo
        if (m_bSpatializedChannel)
        {
            // Combine stereo channels

            // Downscale the volume twice
            lvol /= 512;
            rvol /= 512;

            MakeVolumeRamp(Chan->Volume, m_NewVol, FrameCount, 512);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                pBuffer[i].Chan[0] += ((int)frames[0] + (int)frames[1]) * m_VolumeRampL[i];
                pBuffer[i].Chan[1] += ((int)frames[0] + (int)frames[1]) * m_VolumeRampR[i];

                frames += 2;
            }

            for (int i = m_VolumeRampSize; i < FrameCount; i++)
            {
                pBuffer[i].Chan[0] += ((int)frames[0] + (int)frames[1]) * lvol;
                pBuffer[i].Chan[1] += ((int)frames[0] + (int)frames[1]) * rvol;

                frames += 2;
            }

            return;
        }

        // Background music/etc
        lvol /= 256;
        rvol /= 256;
        MakeVolumeRamp(Chan->Volume, m_NewVol, FrameCount, 256);
        for (int i = 0; i < m_VolumeRampSize; i++)
        {
            pBuffer[i].Chan[0] += frames[0] * m_VolumeRampL[i];
            pBuffer[i].Chan[1] += frames[1] * m_VolumeRampR[i];

            frames += 2;
        }
        for (int i = m_VolumeRampSize; i < FrameCount; i++)
        {
            pBuffer[i].Chan[0] += frames[0] * lvol;
            pBuffer[i].Chan[1] += frames[1] * rvol;

            frames += 2;
        }
        return;
    }

    // TODO: Add code pass for 32F ?
}

static void WriteSamplesS8(int const* pIn, int8_t* pOut, int Count)
{
    int v;

    for (int i = 0; i < Count; i += 2)
    {
        v = pIn[i] / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;

        pOut[i] = v / 256;

        v = pIn[i + 1] / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;

        pOut[i + 1] = v / 256;
    }
}

static void WriteSamplesS8_Mono(int const* pIn, int8_t* pOut, int Count)
{
    int v;

    while (Count--)
    {
        v = *pIn / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        *pOut++ = v / 256;
        pIn += 2;
    }
}

static void WriteSamplesU8(int const* pIn, uint8_t* pOut, int Count)
{
    int v;

    for (int i = 0; i < Count; i += 2)
    {
        v = pIn[i] / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;

        pOut[i] = (v / 256) + 128;

        v = pIn[i + 1] / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;

        pOut[i + 1] = (v / 256) + 128;
    }
}

static void WriteSamplesU8_Mono(int const* pIn, uint8_t* pOut, int Count)
{
    int v;

    while (Count--)
    {
        v = *pIn / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        *pOut++ = (v / 256) + 128;
        pIn += 2;
    }
}

static void WriteSamples16(int const* pIn, short* pOut, int Count)
{
    int v;

    for (int i = 0; i < Count; i += 2)
    {
        v = pIn[i] / 256;
        if (v > 32767)
            pOut[i] = 32767;
        else if (v < -32768)
            pOut[i] = -32768;
        else
            pOut[i] = v;

        v = pIn[i + 1] / 256;
        if (v > 32767)
            pOut[i + 1] = 32767;
        else if (v < -32768)
            pOut[i + 1] = -32768;
        else
            pOut[i + 1] = v;
    }
}

static void WriteSamples16_Mono(int const* pIn, short* pOut, int Count)
{
    int v;

    while (Count--)
    {
        v = *pIn / 256;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        *pOut++ = v;
        pIn += 2;
    }
}

static void WriteSamples32(int const* pIn, float* pOut, int Count)
{
    const float scale = 1.0f / 256 / 32767;

    __m128 minVal = _mm_set_ss(-1.0f);
    __m128 maxVal = _mm_set_ss(1.0f);

    for (int i = 0; i < Count; i += 2)
    {
        float val1 = static_cast<float>(pIn[i]) * scale;
        float val2 = static_cast<float>(pIn[i + 1]) * scale;

        _mm_store_ss(&pOut[i], _mm_min_ss(_mm_max_ss(_mm_set_ss(val1), minVal), maxVal));
        _mm_store_ss(&pOut[i + 1], _mm_min_ss(_mm_max_ss(_mm_set_ss(val2), minVal), maxVal));
    }
}

static void WriteSamples32_Mono(int const* pIn, float* pOut, int Count)
{
    const float scale = 1.0f / 256 / 32767;

    __m128 minVal = _mm_set_ss(-1.0f);
    __m128 maxVal = _mm_set_ss(1.0f);

    while (Count--)
    {
        float val = static_cast<float>(*pIn) * scale;

        _mm_store_ss(pOut, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), minVal), maxVal));

        pOut++;
        pIn += 2;
    }
}

void AudioMixer::WriteToTransferBuffer(int const* pSamples, int64_t EndFrame)
{
    int64_t wrapMask = m_DeviceRawPtr->GetTransferBufferSizeInFrames() - 1;

    for (int64_t frameNum = m_RenderFrame; frameNum < EndFrame;)
    {
        int frameOffset = frameNum & wrapMask;

        int frameCount = m_DeviceRawPtr->GetTransferBufferSizeInFrames() - frameOffset;
        if (frameNum + frameCount > EndFrame)
        {
            frameCount = EndFrame - frameNum;
        }

        frameNum += frameCount;

        if (m_DeviceRawPtr->GetChannels() == 1)
        {
            switch (m_DeviceRawPtr->GetSampleBits())
            {
                case 8:
                    if (m_DeviceRawPtr->IsSigned8Bit())
                    {
                        WriteSamplesS8_Mono(pSamples, (int8_t*)m_pTransferBuffer + frameOffset, frameCount);
                    }
                    else
                    {
                        WriteSamplesU8_Mono(pSamples, (uint8_t*)m_pTransferBuffer + frameOffset, frameCount);
                    }
                    break;
                case 16:
                    WriteSamples16_Mono(pSamples, (short*)m_pTransferBuffer + frameOffset, frameCount);
                    break;
                case 32:
                    WriteSamples32_Mono(pSamples, (float*)m_pTransferBuffer + frameOffset, frameCount);
                    break;
            }

            pSamples += frameCount << 1;
        }
        else
        {
            frameOffset <<= 1;
            frameCount <<= 1;

            switch (m_DeviceRawPtr->GetSampleBits())
            {
                case 8:
                    if (m_DeviceRawPtr->IsSigned8Bit())
                    {
                        WriteSamplesS8(pSamples, (int8_t*)m_pTransferBuffer + frameOffset, frameCount);
                    }
                    else
                    {
                        WriteSamplesU8(pSamples, (uint8_t*)m_pTransferBuffer + frameOffset, frameCount);
                    }
                    break;
                case 16:
                    WriteSamples16(pSamples, (short*)m_pTransferBuffer + frameOffset, frameCount);
                    break;
                case 32:
                    WriteSamples32(pSamples, (float*)m_pTransferBuffer + frameOffset, frameCount);
                    break;
            }

            pSamples += frameCount;
        }
    }
}

HK_NAMESPACE_END
