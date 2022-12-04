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

#include "SoundResource.h"
#include "Engine.h"

#include <Platform/Memory/Memory.h>
#include <Platform/Logger.h>

#include <Core/Parse.h>

#include <Audio/AudioDevice.h>
#include <Audio/AudioDecoder.h>

static int RevisionGen = 0;

HK_CLASS_META(ASoundResource)

ASoundResource::ASoundResource()
{
    m_Revision = ++RevisionGen;

    Platform::ZeroMem(&m_AudioFileInfo, sizeof(m_AudioFileInfo));
}

ASoundResource::~ASoundResource()
{
    Purge();
}

int ASoundResource::GetFrequency() const
{
    AAudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

    // The frequency always matches the playback system
    return device->GetSampleRate();
}

int ASoundResource::GetSampleBits() const
{
    return m_AudioFileInfo.SampleBits;
}

int ASoundResource::GetSampleWidth() const
{
    return m_AudioFileInfo.SampleBits >> 3;
}

int ASoundResource::GetSampleStride() const
{
    return (m_AudioFileInfo.SampleBits >> 3) << (m_AudioFileInfo.Channels - 1);
}

int ASoundResource::GetChannels() const
{
    return m_AudioFileInfo.Channels;
}

int ASoundResource::GetFrameCount() const
{
    return m_AudioFileInfo.FrameCount;
}

float ASoundResource::GetDurationInSecounds() const
{
    return m_DurationInSeconds;
}

SOUND_STREAM_TYPE ASoundResource::GetStreamType() const
{
    return m_CurStreamType;
}

void ASoundResource::LoadInternalResource(AStringView _Path)
{
    Purge();

    // TODO: ...
}

bool ASoundResource::LoadResource(IBinaryStreamReadInterface& Stream)
{
    Purge();

    AString const& fn        = Stream.GetName();
    AStringView    extension = PathUtils::GetExt(fn);

    if (!extension.Icmp(".sound"))
    {
        AString text = Stream.AsString();

        SDocumentDeserializeInfo deserializeInfo;
        deserializeInfo.pDocumentData = text.CStr();
        deserializeInfo.bInsitu       = true;

        ADocument doc;
        doc.DeserializeFromString(deserializeInfo);

        ADocMember* soundMemb = doc.FindMember("Sound");
        if (!soundMemb)
        {
            LOG("ASoundResource::LoadResource: invalid sound\n");
            return false;
        }

        auto soundFile = soundMemb->GetStringView();
        if (soundFile.IsEmpty())
        {
            LOG("ASoundResource::LoadResource: invalid sound\n");
            return false;
        }

        ABinaryResource* soundBinary = AResource::CreateFromFile<ABinaryResource>(soundFile);

        if (!soundBinary->GetSizeInBytes())
        {
            LOG("ASoundResource::LoadResource: invalid sound\n");
            return false;
        }

        SSoundCreateInfo createInfo;

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

        pFileInMemory = MakeRef< SFileInMemory >( pHeapPtr, sizeInBytes );

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

bool ASoundResource::InitializeFromMemory(AStringView _Path, BlobRef Memory, SSoundCreateInfo const& _CreateInfo)
{
    AAudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

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
            if (!CreateAudioBuffer(AFile::OpenRead(_Path, Memory.GetData(), Memory.Size()).ReadInterface(), &m_AudioFileInfo, deviceSampleRate, mono, _CreateInfo.bForce8Bit, &m_pBuffer))
            {
                return false;
            }
            break;
        }
        case SOUND_STREAM_MEMORY: {
            if (!LoadAudioFile(AFile::OpenRead(_Path, Memory.GetData(), Memory.Size()).ReadInterface(), &m_AudioFileInfo, deviceSampleRate, mono, _CreateInfo.bForce8Bit, nullptr))
            {
                return false;
            }
            void* pHeapPtr = Platform::GetHeapAllocator<HEAP_AUDIO_DATA>().Alloc(Memory.Size());
            Platform::Memcpy(pHeapPtr, Memory.GetData(), Memory.Size());

            m_pFileInMemory = MakeRef<SFileInMemory>(pHeapPtr, Memory.Size());
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
bool ASoundResource::CreateAudioStreamInstance( TRef< IAudioStream > * ppInterface )
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

bool ASoundResource::CreateStreamInstance(TRef<SAudioStream>* ppInterface)
{
    if (m_CurStreamType != SOUND_STREAM_MEMORY || !m_pFileInMemory)
    {
        return false;
    }

    *ppInterface = MakeRef<SAudioStream>(m_pFileInMemory, GetFrameCount(), GetFrequency(), GetSampleBits(), GetChannels());
    return true;
}

void ASoundResource::Purge()
{
    m_pBuffer.Reset();
    m_pFileInMemory.Reset();
    m_DurationInSeconds = 0;

    // Mark resource was changed
    m_Revision = ++RevisionGen;
}
