#pragma once

#include "BaseObject.h"
#include "AudioDecoderInterface.h"

enum ESoundStreamType {
    SST_NonStreamed,
    SST_FileStreamed,
    SST_MemoryStreamed
};

enum { AUDIO_MIN_PCM_BUFFER_SIZE = 1024 * 24 };
enum { AUDIO_MAX_PCM_BUFFER_SIZE = 1024 * 256 };

class FAudioClip : public FBaseObject {
    AN_CLASS( FAudioClip, FBaseObject )

public:

    ESoundStreamType StreamType;

    // Initialize default object representation
    void InitializeDefaultObject() override;

    // Initialize object from file
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    // Initialize object from data
    bool InitializeFromData( const char * _Path, IAudioDecoderInterface * _Decoder, const byte * _Data, size_t _DataLength );

    IAudioStreamInterface * CreateAudioStreamInstance();

    // Purge audio data
    void Purge();

    int GetFrequency() const;

    int GetBitsPerSample() const;

    int GetChannels() const;

    int GetSamplesCount() const;

    float GetDurationInSecounds() const;

    ESoundStreamType GetStreamType() const;

    // Set buffer size in samples for streamed audio
    void SetBufferSize( int _BufferSizeInSamples );

    // Get current buffer size in samples for streamed audio
    int GetBufferSize() const;

    IAudioDecoderInterface * GetDecoderInterface() { return Decoder; }

    const byte * GetEncodedData() const { return EncodedData; }
    size_t GetEncodedDataLength() const { return EncodedDataLength; }

    FString const & GetFileName() const { return FileName; }

    int GetFormat() const { return Format; }

    unsigned int GetBufferId() const { return BufferId; }

    int GetSerialId() const { return SerialId; }

protected:
    FAudioClip();
    ~FAudioClip();

private:
    unsigned int BufferId;
    ESoundStreamType CurStreamType;
    int    Frequency;
    int    BitsPerSample;
    int    Channels;
    int    SamplesCount;
    float  DurationInSeconds;
    int    Format;
    FSize  BufferSize;
    byte * EncodedData;
    size_t EncodedDataLength;
    bool   bLoaded;
    TRef< IAudioDecoderInterface > Decoder;

    // Serial id нужен для того, чтобы объекты, использующие ресурс могли узнать, что
    // ресурс перезагружен
    int SerialId;

    FString FileName;
};
