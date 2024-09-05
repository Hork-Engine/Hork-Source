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

#include "AudioMixer.h"
#include "HRTF.h"
#include "Freeverb.h"
#include "AudioDecoder.h"

#include <Engine/Core/Logger.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

HK_NAMESPACE_BEGIN

ConsoleVar Snd_MixAhead("Snd_MixAhead"_s, "0.1"_s);
ConsoleVar Snd_VolumeRampSize("Snd_VolumeRampSize"_s, "16"_s);
ConsoleVar Snd_HRTF("Snd_HRTF"_s, "1"_s);

#if 0
ConsoleVar Rev_RoomSize("Rev_RoomSize"s, "0.5"_s);
ConsoleVar Rev_Damp("Rev_Damp"_s, "0.5"_s);
ConsoleVar Rev_Wet("Rev_Wet"_s, "0.33"_s);
ConsoleVar Rev_Dry("Rev_Dry"_s, "1"_s);
ConsoleVar Rev_Width("Rev_Width"_s, "1"_s);
#endif

namespace
{
    // u8 to s32 sample convertion
    struct SampleLookup8BitTable
    {
        int Data[32][256];

        int16_t ToShort[256];

        SampleLookup8BitTable()
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

    const SampleLookup8BitTable SampleLookup8Bit;
}

AudioMixer::AudioMixer(AudioDevice* device) :
    m_Device(device), m_DeviceRawPtr(device), m_IsAsync(false), m_RenderFrame(0)
{
    m_Hrtf = MakeUnique<AudioHRTF>(m_DeviceRawPtr->GetSampleRate());
    m_ReverbFilter = MakeUnique<Freeverb>(m_DeviceRawPtr->GetSampleRate());

    m_Tracks = nullptr;
    m_TracksTail = nullptr;
    m_PendingList = nullptr;
    m_PendingListTail = nullptr;

    m_TotalTracks.StoreRelaxed(0);
    m_NumActiveTracks.StoreRelaxed(0);
}

AudioMixer::~AudioMixer()
{
    StopAsync();

    // Add pendings (if any)
    AddPendingTracks();

    // Free tracks
    AudioTrack* next;
    for (AudioTrack* track = m_Tracks; track; track = next)
    {
        next = track->Next;
        track->RemoveRef();
    }

    AudioTrack::sFreePool();
}

void AudioMixer::StartAsync()
{
    m_IsAsync = true;
    m_DeviceRawPtr->SetMixerCallback([this](uint8_t* transferBuffer, int transferBufferSizeInFrames, int FrameNum, int minFramesToRender)
                                     { UpdateAsync(transferBuffer, transferBufferSizeInFrames, FrameNum, minFramesToRender); });
}

void AudioMixer::StopAsync()
{
    m_IsAsync = false;
    m_DeviceRawPtr->SetMixerCallback(nullptr);
}

void AudioMixer::SubmitTracks(AudioMixerSubmitQueue& submitQueue)
{
    SpinLockGuard lockGuard(m_SubmitLock);

    auto& tracks = submitQueue.GetTracks();
    for (int i = 0; i < tracks.Size(); i++)
    {
        AudioTrack* track = tracks[i];

        HK_ASSERT(!INTRUSIVE_EXISTS(track, Next, Prev, m_PendingList, m_PendingListTail));
        INTRUSIVE_ADD(track, Next, Prev, m_PendingList, m_PendingListTail);

        track->AddRef();

        track->Volume[0] = track->Volume_LOCK[0];
        track->Volume[1] = track->Volume_LOCK[1];
        track->bVirtual = track->Volume[0] == 0 && track->Volume[1] == 0;
        track->LocalDir = track->LocalDir_LOCK;
    }

    submitQueue.Clear();
}

void AudioMixer::AddPendingTracks()
{
    AudioTrack* submittedTracks = nullptr;

    {
        SpinLockGuard lockGuard(m_SubmitLock);

        submittedTracks = m_PendingList;

        INTRUSIVE_MERGE(Next, Prev,
                        m_Tracks, m_TracksTail,
                        m_PendingList, m_PendingListTail);
    }

    int count = 0;
    for (AudioTrack* track = submittedTracks; track; track = track->Next)
    {
        if (track->pDecoder && !track->bVirtual)
        {
            track->pDecoder->SeekToFrame(track->PlaybackPos.Load());
        }
        count++;
    }

    m_TotalTracks.Add(count);
}

void AudioMixer::RejectTrack(AudioTrack* track)
{
    INTRUSIVE_REMOVE(track, Next, Prev, m_Tracks, m_TracksTail);
    track->RemoveRef();
    m_TotalTracks.Decrement();
}

void AudioMixer::Update()
{
    if (m_IsAsync)
    {
        LOG("AudioMixer::Update: mixer is running in async thread\n");
        return;
    }

    int64_t frameNum;

    m_TransferBuffer = m_DeviceRawPtr->MapTransferBuffer(&frameNum);

    if (m_RenderFrame < frameNum)
    {
        LOG("AudioMixer::Update: Missing frames {}\n", frameNum - m_RenderFrame);

        m_RenderFrame = frameNum;
    }

    int framesToRender = Snd_MixAhead.GetFloat() * m_DeviceRawPtr->GetSampleRate();
    framesToRender = Math::Clamp(framesToRender, 0, m_DeviceRawPtr->GetTransferBufferSizeInFrames());

    int64_t endFrame = frameNum + framesToRender;

    RenderTracks(endFrame);

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

void AudioMixer::UpdateAsync(uint8_t* transferBuffer, int transferBufferSizeInFrames, int frameNum, int minFramesToRender)
{
    m_TransferBuffer = transferBuffer;

    if (m_RenderFrame < frameNum)
    {
        m_RenderFrame = frameNum;
    }

#if 0
    int framesToRender = Snd_MixAhead.GetFloat() * m_DeviceRawPtr->GetSampleRate();
    framesToRender = Math::Clamp( framesToRender, minFramesToRender, transferBufferSizeInFrames );
#else
    int framesToRender = minFramesToRender;
#endif

    int64_t endFrame = frameNum + framesToRender;

    RenderTracks(endFrame);
}

void AudioMixer::RenderTracks(int64_t endFrame)
{
    int numActiveTracks = m_NumActiveTracks.Load();

    if (m_RenderFrame < endFrame)
    {
        numActiveTracks = 0;
    }

    AddPendingTracks();

    while (m_RenderFrame < endFrame)
    {
        int64_t end = endFrame;
        if (endFrame - m_RenderFrame > m_RenderBufferSize)
        {
            end = m_RenderFrame + m_RenderBufferSize;
        }

        int frameCount = end - m_RenderFrame;

        Core::ZeroMem(m_RenderBuffer, frameCount * sizeof(SamplePair));

        AudioTrack* next;
        for (AudioTrack* track = m_Tracks; track; track = next)
        {
            next = track->Next;

            if (track->GetRefCount() == 1)
            {
                // Track was removed from main thread
                RejectTrack(track);
                continue;
            }

            bool bSeek = false;

            {
                SpinLockGuard guard(track->Lock);
                m_NewVol[0] = track->bPaused_LOCK ? 0 : track->Volume_LOCK[0];
                m_NewVol[1] = track->bPaused_LOCK ? 0 : track->Volume_LOCK[1];
                m_NewDir = track->LocalDir_LOCK;
                m_SpatializedTrack = track->bSpatializedStereo_LOCK;
                m_TrackPaused = track->bPaused_LOCK;
                m_PlaybackPos = track->PlaybackPos.Load();
                if (track->PlaybackPos_LOCK >= 0)
                {
                    bSeek = track->PlaybackPos_LOCK != m_PlaybackPos;
                    m_PlaybackPos = track->PlaybackPos_LOCK;
                    track->PlaybackPos_LOCK = -1;
                }
            }

            if (bSeek && !track->bVirtual && track->pDecoder)
            {
                track->pDecoder->SeekToFrame(m_PlaybackPos);
            }

            if (m_NewVol[0] == 0 && m_NewVol[1] == 0 && track->Volume[0] == 0 && track->Volume[1] == 0)
            {
                if (!track->bVirtual)
                {
                    bool bLooped = track->GetLoopStart() >= 0;
                    if (track->bVirtualizeWhenSilent || bLooped || m_TrackPaused)
                    {
                        track->bVirtual = true;
                    }
                    else
                    {
                        track->Stopped.Store(true);
                        RejectTrack(track);
                        continue;
                    }
                }
            }
            else
            {
                // Devirtualize
                if (track->bVirtual)
                {
                    if (track->pDecoder)
                    {
                        track->pDecoder->SeekToFrame(m_PlaybackPos);
                    }
                    track->bVirtual = false;
                }
            }

            if (!track->bVirtual)
            {
                numActiveTracks++;
            }

            if (m_TrackPaused && track->bVirtual)
            {
                // Only virtual Tracks is really paused
                track->PlaybackEnd = 0;
                continue;
            }

            // Playing is just started or unpaused
            if (track->PlaybackEnd == 0)
            {
                track->PlaybackEnd = m_RenderFrame + (track->FrameCount - m_PlaybackPos);
            }

            if (track->pDecoder)
            {
                RenderStream(track, end);
            }
            else
            {
                RenderTrack(track, end);
            }

            track->PlaybackPos.Store(m_PlaybackPos);
        }

        WriteToTransferBuffer((int const*)m_RenderBuffer, end);
        m_RenderFrame = end;
    }

    m_NumActiveTracks.Store(numActiveTracks);
}

namespace
{
    void ConvertFramesToMonoF32(const void* inFrames, int frameCount, int sampleBits, int channels, float* outFrames)
    {
        if (sampleBits == 8)
        {
            // Lookup at max volume
            int const*  lookup     = SampleLookup8Bit.Data[31];
            const float intToFloat = 1.0f / 256 / 32767;

            uint8_t const* frames = (uint8_t const*)inFrames;

            // Mono
            if (channels == 1)
            {
                for (int i = 0; i < frameCount; i++)
                {
                    outFrames[i] = lookup[frames[i]] * intToFloat;
                }
                return;
            }

            // Combine stereo channels
            for (int i = 0; i < frameCount; i++)
            {
                outFrames[i] = (lookup[frames[0]] + lookup[frames[1]]) * (intToFloat * 0.5f); // average
                frames += 2;
            }
            return;
        }

        if (sampleBits == 16)
        {
            const float intToFloat = 1.0f / 32767;

            int16_t const* frames = (int16_t const*)inFrames;

            // Mono
            if (channels == 1)
            {
                for (int i = 0; i < frameCount; i++)
                {
                    outFrames[i] = frames[i] * intToFloat;
                }
                return;
            }

            // Combine stereo channels
            for (int i = 0; i < frameCount; i++)
            {
                outFrames[i] = ((int)frames[0] + (int)frames[1]) * (intToFloat * 0.5f); // average
                frames += 2;
            }
            return;
        }

        if (sampleBits == 32)
        {
            float const* frames = (float const*)inFrames;

            // Mono
            if (channels == 1)
            {
                Core::Memcpy(outFrames, inFrames, frameCount * sizeof(float));
                return;
            }

            // Combine stereo channels
            for (int i = 0; i < frameCount; i++)
            {
                outFrames[i] = (frames[0] + frames[1]) * 0.5f; // average
                frames += 2;
            }
            return;
        }

        // Should never happen, but just in case...
        HK_ASSERT(0);
    }
}

// Read frames from current playback position and convert to f32 format.
void AudioMixer::ReadFramesF32(AudioTrack* track, int framesToRead, int historyExtraFrames, float* inFrames)
{
    int frameCount = track->FrameCount;
    const byte* pRawSamples = (const byte*)track->GetFrames();
    int stride = track->SampleStride;
    int sampleBits = track->SampleBits;
    int channels = track->Channels;
    int inloop = track->GetLoopStart() >= 0 ? track->LoopsCount : 0;
    int start = inloop ? track->GetLoopStart() : 0;
    float* frames = inFrames + historyExtraFrames;

    for (int from = m_PlaybackPos; historyExtraFrames > 0;)
    {
        int framesToCopy;
        if (from - historyExtraFrames < start)
        {
            framesToCopy = from - start;
        }
        else
        {
            framesToCopy = historyExtraFrames;
        }

        historyExtraFrames -= framesToCopy;

        ConvertFramesToMonoF32(pRawSamples + (from - framesToCopy) * stride, framesToCopy, sampleBits, channels, inFrames + historyExtraFrames);

        if (!inloop && historyExtraFrames > 0)
        {
            Core::ZeroMem(inFrames, historyExtraFrames * sizeof(float));
            break;
        }

        from = frameCount;
        inloop--;
    }

    int p = m_PlaybackPos;
    while (framesToRead > 0)
    {
        const byte* samples = pRawSamples + p * stride;

        int framesToCopy = frameCount - p;
        if (framesToCopy > framesToRead)
        {
            framesToCopy = framesToRead;
        }

        framesToRead -= framesToCopy;

        ConvertFramesToMonoF32(samples, framesToCopy, sampleBits, channels, frames);

        frames += framesToCopy;
        p += framesToCopy;

        if (p >= frameCount)
        {
            if (track->GetLoopStart() >= 0)
            {
                p = track->GetLoopStart();
            }
            else
            {
                Core::ZeroMem(frames, framesToRead * sizeof(float));
                break;
            }
        }
    }
}

void AudioMixer::RenderTrack(AudioTrack* track, int64_t endFrame)
{
    int64_t frameNum = m_RenderFrame;
    int clipFrameCount = track->FrameCount;
    byte const* pRawSamples = (byte const*)track->GetFrames();
    int stride = track->SampleStride;

    while (frameNum < endFrame)
    {
        int frameCount;
        if (track->PlaybackEnd < endFrame)
        {
            frameCount = track->PlaybackEnd - frameNum;
        }
        else
        {
            frameCount = endFrame - frameNum;
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
                if (!track->bVirtual)
                {
                    void const* frames = pRawSamples + m_PlaybackPos * stride;

                    SamplePair* buffer = &m_RenderBuffer[frameNum - m_RenderFrame];

                    if (Snd_HRTF && track->bSpatializedStereo_LOCK)
                    {
                        RenderFramesHRTF(track, framesToRender, buffer);
                    }
                    else
                    {
                        RenderFrames(track, frames, framesToRender, buffer);
                    }

                    track->Volume[0] = m_NewVol[0];
                    track->Volume[1] = m_NewVol[1];
                }

                m_PlaybackPos += framesToRender;
            }

            frameNum += frameCount;
        }

        if (frameNum >= track->PlaybackEnd)
        {
            if (track->GetLoopStart() >= 0)
            {
                m_PlaybackPos = track->GetLoopStart();
                track->PlaybackEnd = frameNum + (clipFrameCount - m_PlaybackPos);
                track->LoopsCount++;
            }
            else
            {
                m_PlaybackPos = clipFrameCount;
                break;
            }
        }
    }
}

void AudioMixer::RenderStream(AudioTrack* track, int64_t endFrame)
{
    int64_t frameNum = m_RenderFrame;
    int clipFrameCount = track->FrameCount;
    int stride = track->SampleStride;

    while (frameNum < endFrame)
    {
        int frameCount;
        if (track->PlaybackEnd < endFrame)
        {
            frameCount = track->PlaybackEnd - frameNum;
        }
        else
        {
            frameCount = endFrame - frameNum;
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

            if (!track->bVirtual)
            {
                m_TempFrames.ResizeInvalidate(framesToRender * stride);

                framesToRender = track->pDecoder->ReadFrames(m_TempFrames.ToPtr(), framesToRender, framesToRender * stride);

                if (framesToRender > 0)
                {
                    RenderFrames(track, m_TempFrames.ToPtr(), framesToRender, &m_RenderBuffer[frameNum - m_RenderFrame]);

                    track->Volume[0] = m_NewVol[0];
                    track->Volume[1] = m_NewVol[1];
                }
            }

            m_PlaybackPos += framesToRender;

            frameNum += frameCount;
        }

        if (frameNum >= track->PlaybackEnd)
        {
            if (track->GetLoopStart() >= 0)
            {
                if (!track->bVirtual)
                {
                    track->pDecoder->SeekToFrame(track->GetLoopStart());
                }
                m_PlaybackPos = track->GetLoopStart();
                track->PlaybackEnd = frameNum + (clipFrameCount - m_PlaybackPos);
                track->LoopsCount++;
            }
            else
            {
                m_PlaybackPos = clipFrameCount;
                break;
            }
        }
    }
}

void AudioMixer::MakeVolumeRamp(const int curVol[2], const int newVol[2], int frameCount, int scale)
{
    if (curVol[0] == newVol[0] && curVol[1] == newVol[1])
    {
        m_VolumeRampSize = 0;
        return;
    }

    m_VolumeRampSize = Math::Min3((int)HK_ARRAY_SIZE(m_VolumeRampL), frameCount, Snd_VolumeRampSize.GetInteger());
    if (m_VolumeRampSize < 0)
    {
        m_VolumeRampSize = 0;
        return;
    }

    float increment0 = (float)(newVol[0] - curVol[0]) / (m_VolumeRampSize * scale);
    float increment1 = (float)(newVol[1] - curVol[1]) / (m_VolumeRampSize * scale);

    float lvolf = (float)curVol[0] / scale;
    float rvolf = (float)curVol[1] / scale;

    for (int i = 0; i < m_VolumeRampSize; i++)
    {
        lvolf += increment0;
        rvolf += increment1;

        m_VolumeRampL[i] = lvolf;
        m_VolumeRampR[i] = rvolf;
    }
}

void AudioMixer::RenderFramesHRTF(AudioTrack* track, int frameCount, SamplePair* buffer)
{
    int total = frameCount;

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
    ReadFramesF32(track, total, historyExtraFrames, m_FramesF32.ToPtr());

    // Reallocate (if need) container for filtered samples
    m_StreamF32.ResizeInvalidate(sizeof(SamplePair) * total);

    // Apply HRTF filter
    Float3 dir;
    m_Hrtf->ApplyHRTF(track->LocalDir, m_NewDir, m_FramesF32.ToPtr(), total, (float*)m_StreamF32.ToPtr(), dir);
    track->LocalDir = dir;

    // Make volume ramp
    m_VolumeRampSize = 0;
    if (track->Volume[0] != m_NewVol[0] || track->Volume[1] != m_NewVol[1])
    {
        m_VolumeRampSize = Math::Min3((int)HK_ARRAY_SIZE(m_VolumeRampL), frameCount, Snd_VolumeRampSize.GetInteger());
        if (m_VolumeRampSize > 0)
        {
            float scale = 256.0f / m_Hrtf->GetFilterSize();
            float increment0 = (float)(m_NewVol[0] - track->Volume[0]) / m_VolumeRampSize * scale;
            float lvolf = (float)track->Volume[0] * scale;
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
    for ( int i = m_VolumeRampSize ; i < frameCount; i++ ) {
        pStreamF32[i].Chanf[0] = pStreamF32[i].Chanf[0] * vol;
        pStreamF32[i].Chanf[1] = pStreamF32[i].Chanf[1] * vol;
    }

    float * out = (float *)malloc( frameCount * 2 * sizeof( float ) );

    ReverbFilter->ProcessReplace( &pStreamF32[0].Chanf[0], &pStreamF32[0].Chanf[1], &out[0], &out[1], frameCount, 2 );

    // Mix with output stream
    for ( int i = 0 ; i < frameCount; i++ ) {
        buffer[i].Chan[0] += out[i*2] ;
        buffer[i].Chan[1] += out[i*2+1];
    }

    free( out );
#else
    // Mix with output stream
    float vol = float(65536 / 256) * m_NewVol[0] / m_Hrtf->GetFilterSize();
    for (int i = 0; i < m_VolumeRampSize; i++)
    {
        buffer[i].Chan[0] += pStreamF32[i].Chanf[0] * m_VolumeRampL[i];
        buffer[i].Chan[1] += pStreamF32[i].Chanf[1] * m_VolumeRampL[i];
    }
    for (int i = m_VolumeRampSize; i < frameCount; i++)
    {
        buffer[i].Chan[0] += pStreamF32[i].Chanf[0] * vol;
        buffer[i].Chan[1] += pStreamF32[i].Chanf[1] * vol;
    }
#endif
}

void AudioMixer::RenderFrames(AudioTrack* track, const void* inFrames, int frameCount, SamplePair* buffer)
{
    int sampleBits = track->SampleBits;
    int channels = track->Channels;

    // Render 8bit audio
    if (sampleBits == 8)
    {
        uint8_t const* frames = (uint8_t const*)inFrames;

#if 0
        int lvol = Math::Min( m_NewVol[0] / 256, 255 );
        int rvol = Math::Min( m_NewVol[1] / 256, 255 );

        int const * ls = SampleLookup8Bit.Data[lvol >> 3];
        int const * rs = SampleLookup8Bit.Data[rvol >> 3];

        // Mono
        if ( channels == 1 ) {
            MakeVolumeRamp( track->Volume, m_NewVol, frameCount, 256 );

            for ( int i = 0; i < m_VolumeRampSize; i++ ) {
                buffer[i].Chan[0] += ls[frames[i]]*m_VolumeRampL[i];
                buffer[i].Chan[1] += rs[frames[i]]*m_VolumeRampR[i];
            }

            for ( int i = m_VolumeRampSize ; i < frameCount; i++ ) {
                buffer[i].Chan[0] += ls[frames[i]];
                buffer[i].Chan[1] += rs[frames[i]];
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

            MakeVolumeRamp(track->Volume, m_NewVol, frameCount, 256);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                buffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[i]] * m_VolumeRampL[i];
                buffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[i]] * m_VolumeRampR[i];
            }

            for (int i = m_VolumeRampSize; i < frameCount; i++)
            {
                buffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[i]] * lvol;
                buffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[i]] * rvol;
            }
            return;
        }
#endif

        // Spatialized stereo
        if (m_SpatializedTrack)
        {
            // Combine stereo channels

#if 0

            MakeVolumeRamp( track->Volume, m_NewVol, frameCount, 512 );

            for ( int i = 0; i < m_VolumeRampSize; i++ ) {
                buffer[i].Chan[0] += ( ls[frames[0]] + ls[frames[1]] ) * m_VolumeRampL[i];
                buffer[i].Chan[1] += ( rs[frames[0]] + rs[frames[1]] ) * m_VolumeRampR[i];

                frames += 2;
            }

            for ( int i = m_VolumeRampSize ; i < frameCount; i++ ) {
                buffer[i].Chan[0] += ( ls[frames[0]] + ls[frames[1]] ) / 2;
                buffer[i].Chan[1] += ( rs[frames[0]] + rs[frames[1]] ) / 2;

                frames += 2;
            }

#else

            lvol /= 512;
            rvol /= 512;

            MakeVolumeRamp(track->Volume, m_NewVol, frameCount, 512);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                buffer[i].Chan[0] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * m_VolumeRampL[i];
                buffer[i].Chan[1] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * m_VolumeRampR[i];

                frames += 2;
            }

            for (int i = m_VolumeRampSize; i < frameCount; i++)
            {
                buffer[i].Chan[0] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * lvol;
                buffer[i].Chan[1] += (SampleLookup8Bit.ToShort[frames[0]] + SampleLookup8Bit.ToShort[frames[1]]) * rvol;

                frames += 2;
            }
#endif

            return;
        }

        // Background music/etc
#if 0
        MakeVolumeRamp( track->Volume, m_NewVol, frameCount, 256 );
        for ( int i = 0; i < m_VolumeRampSize; i++ ) {
            buffer[i].Chan[0] += ls[frames[0]] * m_VolumeRampL[i];
            buffer[i].Chan[1] += rs[frames[1]] * m_VolumeRampR[i];

            frames += 2;
        }
        for ( int i = m_VolumeRampSize ; i < frameCount; i++ ) {
            buffer[i].Chan[0] += ls[frames[0]];
            buffer[i].Chan[1] += rs[frames[1]];

            frames += 2;
        }
#else
        lvol /= 256;
        rvol /= 256;

        MakeVolumeRamp(track->Volume, m_NewVol, frameCount, 256);
        for (int i = 0; i < m_VolumeRampSize; i++)
        {
            buffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[0]] * m_VolumeRampL[i];
            buffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[1]] * m_VolumeRampR[i];

            frames += 2;
        }
        for (int i = m_VolumeRampSize; i < frameCount; i++)
        {
            buffer[i].Chan[0] += SampleLookup8Bit.ToShort[frames[0]] * lvol;
            buffer[i].Chan[1] += SampleLookup8Bit.ToShort[frames[1]] * rvol;

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

        int16_t const* frames = (int16_t const*)inFrames;

        // Mono
        if (channels == 1)
        {
            lvol /= 256;
            rvol /= 256;

            MakeVolumeRamp(track->Volume, m_NewVol, frameCount, 256);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                buffer[i].Chan[0] += frames[i] * m_VolumeRampL[i];
                buffer[i].Chan[1] += frames[i] * m_VolumeRampR[i];
            }

            for (int i = m_VolumeRampSize; i < frameCount; i++)
            {
                buffer[i].Chan[0] += frames[i] * lvol;
                buffer[i].Chan[1] += frames[i] * rvol;
            }
            return;
        }

        // Spatialized stereo
        if (m_SpatializedTrack)
        {
            // Combine stereo channels

            // Downscale the volume twice
            lvol /= 512;
            rvol /= 512;

            MakeVolumeRamp(track->Volume, m_NewVol, frameCount, 512);

            for (int i = 0; i < m_VolumeRampSize; i++)
            {
                buffer[i].Chan[0] += ((int)frames[0] + (int)frames[1]) * m_VolumeRampL[i];
                buffer[i].Chan[1] += ((int)frames[0] + (int)frames[1]) * m_VolumeRampR[i];

                frames += 2;
            }

            for (int i = m_VolumeRampSize; i < frameCount; i++)
            {
                buffer[i].Chan[0] += ((int)frames[0] + (int)frames[1]) * lvol;
                buffer[i].Chan[1] += ((int)frames[0] + (int)frames[1]) * rvol;

                frames += 2;
            }

            return;
        }

        // Background music/etc
        lvol /= 256;
        rvol /= 256;
        MakeVolumeRamp(track->Volume, m_NewVol, frameCount, 256);
        for (int i = 0; i < m_VolumeRampSize; i++)
        {
            buffer[i].Chan[0] += frames[0] * m_VolumeRampL[i];
            buffer[i].Chan[1] += frames[1] * m_VolumeRampR[i];

            frames += 2;
        }
        for (int i = m_VolumeRampSize; i < frameCount; i++)
        {
            buffer[i].Chan[0] += frames[0] * lvol;
            buffer[i].Chan[1] += frames[1] * rvol;

            frames += 2;
        }
        return;
    }

    // TODO: Add code pass for 32F ?
}

namespace
{

    void WriteSamples_FLOAT32(int const* in, float* out, int count)
    {
        const float scale = 1.0f / 256 / 32767;

        __m128 minVal = _mm_set_ss(-1.0f);
        __m128 maxVal = _mm_set_ss(1.0f);

        for (int i = 0; i < count; i += 2)
        {
            float val1 = static_cast<float>(in[i]) * scale;
            float val2 = static_cast<float>(in[i + 1]) * scale;

            _mm_store_ss(&out[i], _mm_min_ss(_mm_max_ss(_mm_set_ss(val1), minVal), maxVal));
            _mm_store_ss(&out[i + 1], _mm_min_ss(_mm_max_ss(_mm_set_ss(val2), minVal), maxVal));
        }
    }

    void WriteSamples_FLOAT32_Mono(int const* in, float* out, int count)
    {
        const float scale = 1.0f / 256 / 32767;

        __m128 minVal = _mm_set_ss(-1.0f);
        __m128 maxVal = _mm_set_ss(1.0f);

        while (count--)
        {
            float val = static_cast<float>(*in) * scale;

            _mm_store_ss(out, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), minVal), maxVal));

            out++;
            in += 2;
        }
    }

    void WriteSamples_INT16_Mono(int const* in, short* out, int count)
    {
        int v;

        while (count--)
        {
            v = *in / 256;
            if (v > 32767)
                v = 32767;
            else if (v < -32768)
                v = -32768;
            *out++ = v;
            in += 2;
        }
    }

    void WriteSamples_INT16(int const* in, short* out, int count)
    {
        int v;

        for (int i = 0; i < count; i += 2)
        {
            v = in[i] / 256;
            if (v > 32767)
                out[i] = 32767;
            else if (v < -32768)
                out[i] = -32768;
            else
                out[i] = v;

            v = in[i + 1] / 256;
            if (v > 32767)
                out[i + 1] = 32767;
            else if (v < -32768)
                out[i + 1] = -32768;
            else
                out[i + 1] = v;
        }
    }
}

void AudioMixer::WriteToTransferBuffer(int const* samples, int64_t endFrame)
{
    int64_t wrapMask = m_DeviceRawPtr->GetTransferBufferSizeInFrames() - 1;

    for (int64_t frameNum = m_RenderFrame; frameNum < endFrame;)
    {
        int frameOffset = frameNum & wrapMask;

        int frameCount = m_DeviceRawPtr->GetTransferBufferSizeInFrames() - frameOffset;
        if (frameNum + frameCount > endFrame)
            frameCount = endFrame - frameNum;

        frameNum += frameCount;

        if (m_DeviceRawPtr->GetChannels() == 1)
        {
            if (m_DeviceRawPtr->GetTransferFormat() == AudioTransferFormat::FLOAT32)
                WriteSamples_FLOAT32_Mono(samples, (float*)m_TransferBuffer + frameOffset, frameCount);
            else
                WriteSamples_INT16_Mono(samples, (int16_t*)m_TransferBuffer + frameOffset, frameCount);

            samples += frameCount << 1;
        }
        else
        {
            frameOffset <<= 1;
            frameCount <<= 1;

            if (m_DeviceRawPtr->GetTransferFormat() == AudioTransferFormat::FLOAT32)
                WriteSamples_FLOAT32(samples, (float*)m_TransferBuffer + frameOffset, frameCount);
            else
                WriteSamples_INT16(samples, (int16_t*)m_TransferBuffer + frameOffset, frameCount);

            samples += frameCount;
        }
    }
}

HK_NAMESPACE_END
