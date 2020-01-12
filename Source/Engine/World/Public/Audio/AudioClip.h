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

#pragma once

#include <World/Public/Base/Resource.h>
#include <World/Public/Audio/AudioDecoderInterface.h>

enum ESoundStreamType {
    SOUND_STREAM_DISABLED,
    SOUND_STREAM_FILE,
    SOUND_STREAM_MEMORY
};

enum { AUDIO_MIN_PCM_BUFFER_SIZE = 1024 * 24 };
enum { AUDIO_MAX_PCM_BUFFER_SIZE = 1024 * 256 };

class AAudioClip : public AResource {
    AN_CLASS( AAudioClip, AResource )

public:
    ESoundStreamType StreamType;

    /** Initialize object from data */
    bool InitializeFromData( const char * _Path, IAudioDecoderInterface * _Decoder, const byte * _Data, size_t _DataLength );

    IAudioStreamInterface * CreateAudioStreamInstance();

    /** Purge audio data */
    void Purge();

    int GetFrequency() const;

    int GetBitsPerSample() const;

    int GetChannels() const;

    int GetSamplesCount() const;

    float GetDurationInSecounds() const;

    ESoundStreamType GetStreamType() const;

    /** Set buffer size in samples for streamed audio */
    void SetBufferSize( int _BufferSizeInSamples );

    /** Get current buffer size in samples for streamed audio */
    int GetBufferSize() const;

    IAudioDecoderInterface * GetDecoderInterface() { return Decoder; }

    const byte * GetEncodedData() const { return EncodedData; }
    size_t GetEncodedDataLength() const { return EncodedDataLength; }

    AString const & GetFileName() const { return FileName; }

    int GetFormat() const { return Format; }

    unsigned int GetBufferId() const { return BufferId; }

    int GetSerialId() const { return SerialId; }

protected:
    AAudioClip();
    ~AAudioClip();

    /** Load resource from file */
    bool LoadResource( AString const & _Path ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/Sound/Default"; }

private:
    unsigned int BufferId;
    ESoundStreamType CurStreamType;
    int    Frequency;
    int    BitsPerSample;
    int    Channels;
    int    SamplesCount;
    float  DurationInSeconds;
    int    Format;
    int    BufferSize;
    byte * EncodedData;
    size_t EncodedDataLength;
    bool   bLoaded;
    TRef< IAudioDecoderInterface > Decoder;

    // Serial id нужен для того, чтобы объекты, использующие ресурс могли узнать, что
    // ресурс перезагружен
    int SerialId;

    AString FileName;
};
