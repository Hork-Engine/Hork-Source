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

#pragma once

#include "Resource.h"
#include <Audio/AudioStream.h>
#include <Audio/AudioBuffer.h>
#include <Audio/AudioDecoder.h>

enum ESoundStreamType
{
    /** Short sound effects. Most used. */
    SOUND_STREAM_DISABLED,

    /** Decode audio data with small chunks during playback. Use it for music. */
    SOUND_STREAM_MEMORY,

    /** Load and decode audio data with small chunks from the hard drive during playback.
    Only use it for very large audio tracks or don't use it at all.
    NOTE: Not supported now. Reserved for future.
    */
    SOUND_STREAM_FILE
};

struct SSoundCreateInfo
{
    ESoundStreamType StreamType = SOUND_STREAM_DISABLED;
    bool             bForce8Bit = false;
    bool             bForceMono = false;
};

class ASoundResource : public AResource
{
    HK_CLASS(ASoundResource, AResource)

public:
    /** Initialize from memory */
    bool InitializeFromMemory(const char* _Path, const void* _SysMem, size_t _SizeInBytes, SSoundCreateInfo const* _pCreateInfo = nullptr);

    /** Create streaming instance */
    bool CreateStreamInstance(TRef<SAudioStream>* ppInterface);

    /** Purge audio data */
    void Purge();

    /** Sample rate in hertz */
    int GetFrequency() const;

    /** Bits per sample (8 or 16) */
    int GetSampleBits() const;

    /** Sample size in bytes */
    int GetSampleWidth() const;

    /** Stride between frames. In bytes */
    int GetSampleStride() const;

    /** 1 for mono, 2 for stereo */
    int GetChannels() const;

    /** Is mono track */
    bool IsMono() const { return GetChannels() == 1; }

    /** Is stereo track */
    bool IsStereo() const { return GetChannels() == 2; }

    /** Audio length in frames */
    int GetFrameCount() const;

    /** Audio duration in seconds */
    float GetDurationInSecounds() const;

    ESoundStreamType GetStreamType() const;

    /** File name */
    AString const& GetFileName() const { return FileName; }

    /** Audio buffer. Null for streamed audio */
    SAudioBuffer* GetAudioBuffer() { return pBuffer.GetObject(); }

    /** File data for streaming */
    SFileInMemory* GetFileInMemory() { return pFileInMemory.GetObject(); }

    /** Internal. Used by audio system to determine that audio data changed. */
    int GetRevision() const { return Revision; }

    ASoundResource();
    ~ASoundResource();

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStream& _Stream) override;

    /** Create internal resource */
    void LoadInternalResource(const char* _Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Sound/Default"; }

private:
    TRef<SAudioBuffer>  pBuffer;
    TRef<SFileInMemory> pFileInMemory;
    ESoundStreamType    CurStreamType = SOUND_STREAM_DISABLED;
    SAudioFileInfo      AudioFileInfo;
    float               DurationInSeconds = 0.0f;
    int                 Revision;
    AString             FileName;
};
