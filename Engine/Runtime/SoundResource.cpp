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

#include "SoundResource.h"
#include "Engine.h"

#include <Engine/Core/Platform/Memory/Memory.h>
#include <Engine/Core/Platform/Logger.h>
#include <Engine/Core/Parse.h>

#include <Engine/Audio/AudioDevice.h>
#include <Engine/Audio/AudioDecoder.h>

HK_NAMESPACE_BEGIN

static int RevisionGen = 0;

HK_CLASS_META(SoundResource)

SoundResource::SoundResource()
{
    m_Revision = ++RevisionGen;

    Platform::ZeroMem(&m_AudioFileInfo, sizeof(m_AudioFileInfo));
}

SoundResource::~SoundResource()
{
    Purge();
}

int SoundResource::GetFrequency() const
{
    AudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

    // The frequency always matches the playback system
    return device->GetSampleRate();
}

int SoundResource::GetSampleBits() const
{
    return m_AudioFileInfo.SampleBits;
}

int SoundResource::GetSampleWidth() const
{
    return m_AudioFileInfo.SampleBits >> 3;
}

int SoundResource::GetSampleStride() const
{
    return (m_AudioFileInfo.SampleBits >> 3) << (m_AudioFileInfo.Channels - 1);
}

int SoundResource::GetChannels() const
{
    return m_AudioFileInfo.Channels;
}

int SoundResource::GetFrameCount() const
{
    return m_AudioFileInfo.FrameCount;
}

float SoundResource::GetDurationInSecounds() const
{
    return m_DurationInSeconds;
}

SOUND_STREAM_TYPE SoundResource::GetStreamType() const
{
    return m_CurStreamType;
}

void SoundResource::LoadInternalResource(StringView _Path)
{
    Purge();

    // TODO: ...
}

bool SoundResource::LoadResource(IBinaryStreamReadInterface& Stream)
{
    Purge();

    String const& fn        = Stream.GetName();
    StringView    extension = PathUtils::GetExt(fn);

    if (!extension.Icmp(".sound"))
    {
        String text = Stream.AsString();

        DocumentDeserializeInfo deserializeInfo;
        deserializeInfo.pDocumentData = text.CStr();
        deserializeInfo.bInsitu       = true;

        Document doc;
        doc.DeserializeFromString(deserializeInfo);

        DocumentMember* soundMemb = doc.FindMember("Sound");
        if (!soundMemb)
        {
            LOG("SoundResource::LoadResource: invalid sound\n");
            return false;
        }

        auto soundFile = soundMemb->GetStringView();
        if (soundFile.IsEmpty())
        {
            LOG("SoundResource::LoadResource: invalid sound\n");
            return false;
        }

        BinaryResource* soundBinary = Resource::CreateFromFile<BinaryResource>(soundFile);

        if (!soundBinary->GetSizeInBytes())
        {
            LOG("SoundResource::LoadResource: invalid sound\n");
            return false;
        }

        SoundCreateInfo createInfo;

        createInfo.StreamType = doc.GetBool("bStreamed") ? SOUND_STREAM_MEMORY : SOUND_STREAM_DISABLED;
        createInfo.bForce8Bit = doc.GetBool("bForce8Bit");
        createInfo.bForceMono = doc.GetBool("bForceMono");

        return InitializeFromMemory(soundFile, BlobRef(soundBinary->GetBinaryData(), soundBinary->GetSizeInBytes()), createInfo);
    }
    else
    {
        return InitializeFromMemory(Stream.GetName(), Stream.AsBlob());
    }

#if 0
    HK_ASSERT( !pBuffer );

    FileName = Stream.GetFileName();

    CurStreamType = StreamType;

    bool mono = bForceMono || device->GetChannels() == 1;
    int deviceSampleRate = device->GetSampleRate();

    switch ( CurStreamType ) {
    case SOUND_STREAM_DISABLED:
    {
        if ( !CreateAudioBuffer( Stream, &AudioFileInfo, deviceSampleRate, mono, bForce8Bit, &pBuffer ) ) {
            return false;
        }

        break;
    }
    case SOUND_STREAM_FILE:
    {
        if ( !LoadAudioFile( Stream, &AudioFileInfo, deviceSampleRate, mono, bForce8Bit, nullptr ) ) {
            return false;
        }

        break;
    }
    case SOUND_STREAM_MEMORY:
    {
        if ( !LoadAudioFile( Stream, &AudioFileInfo, deviceSampleRate, mono, bForce8Bit, nullptr ) ) {
            return false;
        }

        Stream.SeekEnd( 0 );

        size_t sizeInBytes = Stream.GetOffset();
        void * pHeapPtr = (byte *)Platform::MemoryAllocSafe( sizeInBytes );

        Stream.Rewind();
        Stream.Read( pHeapPtr, sizeInBytes );

        pFileInMemory = MakeRef< FileInMemory >( pHeapPtr, sizeInBytes );

        break;
    }
    default:
        HK_ASSERT(0);
        return false;
    }

    DurationInSeconds = (float)GetFrameCount() / GetFrequency();

    return true;
#endif
}

bool SoundResource::InitializeFromMemory(StringView _Path, BlobRef Memory, SoundCreateInfo const& _CreateInfo)
{
    AudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

    Purge();

    HK_ASSERT(!m_pBuffer);

    m_FileName = _Path;

    m_CurStreamType = _CreateInfo.StreamType;
    if (m_CurStreamType == SOUND_STREAM_FILE)
    {
        m_CurStreamType = SOUND_STREAM_MEMORY;

        LOG("Using MemoryStreamed instead of FileStreamed as the file data is already in memory\n");
    }

    bool mono             = _CreateInfo.bForceMono || device->GetChannels() == 1;
    int  deviceSampleRate = device->GetSampleRate();

    switch (m_CurStreamType)
    {
        case SOUND_STREAM_DISABLED: {
            if (!CreateAudioBuffer(File::OpenRead(_Path, Memory.GetData(), Memory.Size()).ReadInterface(), &m_AudioFileInfo, deviceSampleRate, mono, _CreateInfo.bForce8Bit, &m_pBuffer))
            {
                return false;
            }
            break;
        }
        case SOUND_STREAM_MEMORY: {
            if (!LoadAudioFile(File::OpenRead(_Path, Memory.GetData(), Memory.Size()).ReadInterface(), &m_AudioFileInfo, deviceSampleRate, mono, _CreateInfo.bForce8Bit, nullptr))
            {
                return false;
            }
            void* pHeapPtr = Platform::GetHeapAllocator<HEAP_AUDIO_DATA>().Alloc(Memory.Size());
            Platform::Memcpy(pHeapPtr, Memory.GetData(), Memory.Size());

            m_pFileInMemory = MakeRef<FileInMemory>(pHeapPtr, Memory.Size());
            break;
        }
        default:
            HK_ASSERT(0);
            return false;
    }

    m_DurationInSeconds = (float)GetFrameCount() / GetFrequency();

    return true;
}

#if 0
bool SoundResource::CreateAudioStreamInstance( TRef< IAudioStream > * ppInterface )
{
    if ( CurStreamType == SOUND_STREAM_DISABLED || !Decoder ) {
        return false;
    }

    Decoder->CreateAudioStream( ppInterface );

    bool bCreateResult = false;

    if ( *ppInterface ) {
        if ( CurStreamType == SOUND_STREAM_FILE ) {
            bCreateResult = (*ppInterface)->InitializeFileStream( GetFileName().CStr(),
                                                                  GetFrequency(),
                                                                  GetSampleBits(),
                                                                  GetChannels() );
        } else {
            bCreateResult = (*ppInterface)->InitializeMemoryStream( GetFileName().CStr(),
                                                                    GetFileInMemory(),
                                                                    GetFileInMemorySize(),
                                                                    GetFrequency(),
                                                                    GetSampleBits(),
                                                                    GetChannels() );
        }
    }

    if ( !bCreateResult ) {
        (*ppInterface).Reset();
        return false;
    }

    return (*ppInterface) != nullptr;
}
#endif

bool SoundResource::CreateStreamInstance(TRef<AudioStream>* ppInterface)
{
    if (m_CurStreamType != SOUND_STREAM_MEMORY || !m_pFileInMemory)
    {
        return false;
    }

    *ppInterface = MakeRef<AudioStream>(m_pFileInMemory, GetFrameCount(), GetFrequency(), GetSampleBits(), GetChannels());
    return true;
}

void SoundResource::Purge()
{
    m_pBuffer.Reset();
    m_pFileInMemory.Reset();
    m_DurationInSeconds = 0;

    // Mark resource was changed
    m_Revision = ++RevisionGen;
}

HK_NAMESPACE_END
