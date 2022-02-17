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

#include <Audio/AudioDevice.h>
#include <Audio/AudioDecoder.h>

static int RevisionGen = 0;

HK_CLASS_META(ASoundResource)

ASoundResource::ASoundResource()
{
    Revision = ++RevisionGen;

    Platform::ZeroMem(&AudioFileInfo, sizeof(AudioFileInfo));
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
    return AudioFileInfo.SampleBits;
}

int ASoundResource::GetSampleWidth() const
{
    return AudioFileInfo.SampleBits >> 3;
}

int ASoundResource::GetSampleStride() const
{
    return (AudioFileInfo.SampleBits >> 3) << (AudioFileInfo.Channels - 1);
}

int ASoundResource::GetChannels() const
{
    return AudioFileInfo.Channels;
}

int ASoundResource::GetFrameCount() const
{
    return AudioFileInfo.FrameCount;
}

float ASoundResource::GetDurationInSecounds() const
{
    return DurationInSeconds;
}

ESoundStreamType ASoundResource::GetStreamType() const
{
    return CurStreamType;
}

void ASoundResource::LoadInternalResource(const char* _Path)
{
    Purge();

    // TODO: ...
}

bool ASoundResource::LoadResource(IBinaryStream& Stream)
{
    Purge();

    if (!Platform::Stricmp(Stream.GetFileName() + Platform::FindExt(Stream.GetFileName()), ".sound"))
    {
        AString text;
        text.FromFile(Stream);

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

        AString fontFile = soundMemb->GetString();
        if (fontFile.IsEmpty())
        {
            LOG("ASoundResource::LoadResource: invalid sound\n");
            return false;
        }

        ABinaryResource* soundBinary = CreateInstanceOf<ABinaryResource>();
        soundBinary->InitializeFromFile(fontFile.CStr());

        if (!soundBinary->GetSizeInBytes())
        {
            LOG("ASoundResource::LoadResource: invalid sound\n");
            return false;
        }

        SSoundCreateInfo createInfo;

        ADocMember* member;

        member                = doc.FindMember("bStreamed");
        createInfo.StreamType = member ? (Math::ToBool(member->GetString()) ? SOUND_STREAM_MEMORY : SOUND_STREAM_DISABLED) : SOUND_STREAM_DISABLED;

        member                = doc.FindMember("bForce8Bit");
        createInfo.bForce8Bit = member ? Math::ToBool(member->GetString()) : false;

        member                = doc.FindMember("bForceMono");
        createInfo.bForceMono = member ? Math::ToBool(member->GetString()) : false;

        return InitializeFromMemory(fontFile.CStr(), soundBinary->GetBinaryData(), soundBinary->GetSizeInBytes(), &createInfo);
    }
    else
    {
        size_t size = Stream.SizeInBytes();
        void*  data = GHunkMemory.Alloc(size);
        Stream.Read(data, size);
        bool bResult = InitializeFromMemory(Stream.GetFileName(), data, size);
        GHunkMemory.ClearLastHunk();
        return bResult;
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
        void * pHeapPtr = (byte *)GHeapMemory.Alloc( sizeInBytes );

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

bool ASoundResource::InitializeFromMemory(const char* _Path, const void* _SysMem, size_t _SizeInBytes, SSoundCreateInfo const* _pCreateInfo)
{
    static const SSoundCreateInfo defaultCI;
    if (!_pCreateInfo)
    {
        _pCreateInfo = &defaultCI;
    }

    AAudioDevice* device = GEngine->GetAudioSystem()->GetPlaybackDevice();

    Purge();

    HK_ASSERT(!pBuffer);

    FileName = _Path;

    CurStreamType = _pCreateInfo->StreamType;
    if (CurStreamType == SOUND_STREAM_FILE)
    {
        CurStreamType = SOUND_STREAM_MEMORY;

        LOG("Using MemoryStreamed instead of FileStreamed as the file data is already in memory\n");
    }

    bool mono             = _pCreateInfo->bForceMono || device->GetChannels() == 1;
    int  deviceSampleRate = device->GetSampleRate();

    switch (CurStreamType)
    {
        case SOUND_STREAM_DISABLED: {
            AMemoryStream f;

            if (!f.OpenRead(_Path, _SysMem, _SizeInBytes))
            {
                return false;
            }

            if (!CreateAudioBuffer(f, &AudioFileInfo, deviceSampleRate, mono, _pCreateInfo->bForce8Bit, &pBuffer))
            {
                return false;
            }

            break;
        }
        case SOUND_STREAM_MEMORY: {
            AMemoryStream f;

            if (!f.OpenRead(_Path, _SysMem, _SizeInBytes))
            {
                return false;
            }

            if (!LoadAudioFile(f, &AudioFileInfo, deviceSampleRate, mono, _pCreateInfo->bForce8Bit, nullptr))
            {
                return false;
            }

            void* pHeapPtr = GHeapMemory.Alloc(_SizeInBytes);
            Platform::Memcpy(pHeapPtr, _SysMem, _SizeInBytes);

            pFileInMemory = MakeRef<SFileInMemory>(pHeapPtr, _SizeInBytes);

            break;
        }
        default:
            HK_ASSERT(0);
            return false;
    }

    DurationInSeconds = (float)GetFrameCount() / GetFrequency();

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
    if (CurStreamType != SOUND_STREAM_MEMORY || !pFileInMemory)
    {
        return false;
    }

    *ppInterface = MakeRef<SAudioStream>(pFileInMemory, GetFrameCount(), GetFrequency(), GetSampleBits(), GetChannels());
    return true;
}

void ASoundResource::Purge()
{
    pBuffer.Reset();
    pFileInMemory.Reset();
    DurationInSeconds = 0;

    // Mark resource was changed
    Revision = ++RevisionGen;
}
