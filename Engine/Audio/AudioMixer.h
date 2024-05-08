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

#pragma once

#include "AudioDevice.h"
#include "AudioTrack.h"

#include <Engine/Core/Containers/ArrayView.h>
#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

class AudioMixerSubmitQueue final
{
    HK_FORBID_COPY(AudioMixerSubmitQueue)

public:
    AudioMixerSubmitQueue() = default;

    ~AudioMixerSubmitQueue()
    {
        Clear();
    }

    void Clear()
    {
        for (AudioTrack* track : m_Tracks)
            track->RemoveRef();
        m_Tracks.Clear();
    }

    // TODO: В будущем, для многопоточки можно использовать вектор фиксированный длины и атомарный счетчик,
    // либо для каждого потока создать свой вектор или экземпляр AudioMixerSubmitQueue
    void Add(AudioTrack* track)
    {
        m_Tracks.Add(track);
        track->AddRef();
    }

    TVector<AudioTrack*> const& GetTracks()
    {
        return m_Tracks;
    }

private:
    TVector<AudioTrack*> m_Tracks;
};

class AudioMixer
{
    HK_FORBID_COPY(AudioMixer)

public:
    AudioMixer(AudioDevice* _Device);
    virtual ~AudioMixer();

    /// Add tracks to mixer thread
    void SubmitTracks(AudioMixerSubmitQueue& submitQueue);

    /// Get current active tracks
    int GetNumActiveTracks() const
    {
        return m_NumActiveTracks.Load();
    }

    /// Get number of not active (virtual) tracks
    int GetNumVirtualTracks() const
    {
        return m_TotalTracks.Load() - m_NumActiveTracks.Load();
    }

    /// Get total count of tracks
    int GetTotalTracks() const
    {
        return m_TotalTracks.Load();
    }

    /// Start async mixing
    void StartAsync();

    /// Stop async mixing
    void StopAsync();

    /// Perform mixing in main thread
    void Update();

    bool IsAsync() const
    {
        return m_bAsync;
    }

private:
    struct SamplePair
    {
        union
        {
            int32_t Chan[2];
            float Chanf[2];
        };
    };

    void UpdateAsync(uint8_t* pTransferBuffer, int TransferBufferSizeInFrames, int FrameNum, int MinFramesToRender);

    // This fuction adds pending tracks to list
    void AddPendingTracks();
    void RejectTrack(AudioTrack* track);
    void RenderTracks(int64_t EndFrame);
    void RenderTrack(AudioTrack* track, int64_t EndFrame);
    void RenderStream(AudioTrack* track, int64_t EndFrame);
    void RenderFramesHRTF(AudioTrack* track, int FrameCount, SamplePair* pBuffer);
    void RenderFrames(AudioTrack* track, const void* pFrames, int FrameCount, SamplePair* pBuffer);
    void WriteToTransferBuffer(int const* pSamples, int64_t EndFrame);
    void MakeVolumeRamp(const int CurVol[2], const int NewVol[2], int FrameCount, int Scale);
    void ReadFramesF32(AudioTrack* track, int FramesToRead, int HistoryExtraFrames, float* pFrames);

    TUniqueRef<class AudioHRTF> m_Hrtf;
    TUniqueRef<class Freeverb> m_ReverbFilter;

    alignas(16) SamplePair m_RenderBuffer[2048];
    const int m_RenderBufferSize = HK_ARRAY_SIZE(m_RenderBuffer);

    TRef<AudioDevice> m_pDevice;
    AudioDevice* m_DeviceRawPtr;
    uint8_t* m_pTransferBuffer;
    bool m_bAsync;
    int64_t m_RenderFrame;
    AtomicInt m_NumActiveTracks;
    AtomicInt m_TotalTracks;

    AudioTrack* m_Tracks;
    AudioTrack* m_TracksTail;
    AudioTrack* m_PendingList;
    AudioTrack* m_PendingListTail;

    SpinLock m_SubmitLock;

    // For current mixing track
    int m_NewVol[2];
    Float3 m_NewDir;
    bool m_bSpatializedTrack;
    bool m_bTrackPaused;
    int m_PlaybackPos;
    int m_VolumeRampL[1024];
    int m_VolumeRampR[1024];
    int m_VolumeRampSize;

    TVector<uint8_t> m_TempFrames;
    TVector<float> m_FramesF32;
    TVector<SamplePair> m_StreamF32;
};

extern ConsoleVar Snd_HRTF;

HK_NAMESPACE_END
