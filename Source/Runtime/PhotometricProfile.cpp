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

#include "PhotometricProfile.h"
#include "Texture.h"

#include <Assets/Asset.h>
#include <Platform/Logger.h>

HK_CLASS_META(APhotometricProfile)

int APhotometricProfile::m_PhotometricProfileCounter = 0;

APhotometricProfile::APhotometricProfile()
{
    Platform::ZeroMem(m_Data, sizeof(m_Data));
}

APhotometricProfile::~APhotometricProfile()
{
}

APhotometricProfile* APhotometricProfile::Create(void const* pData, float Intensity)
{
    APhotometricProfile* profile = CreateInstanceOf<APhotometricProfile>();
    profile->m_Intensity         = Intensity;
    Platform::Memcpy(profile->m_Data, pData, sizeof(profile->m_Data));
    return profile;
}

void APhotometricProfile::LoadInternalResource(AStringView Path)
{
    if (!Path.Icmp("/Default/PhotometricProfile/Default"))
    {
        m_Intensity = 1.0f;
        Platform::Memset(m_Data, 0xff, sizeof(m_Data));
        return;
    }

    LOG("Unknown internal resource {}\n", Path);

    LoadInternalResource("/Default/PhotometricProfile/Default");
}

bool APhotometricProfile::LoadResource(IBinaryStreamReadInterface& Stream)
{
    AString const& fn = Stream.GetName();
    AStringView extension = PathUtils::GetExt(fn);

    if (!extension.Icmp(".ies"))
    {
        PhotometricData data = ParsePhotometricData(Stream.AsString());
        if (!data)
            return false;

        data.ReadSamples(m_Data, m_Intensity);
    }
    else
    {
        uint32_t fileFormat;
        uint32_t fileVersion;

        fileFormat = Stream.ReadUInt32();

        if (fileFormat != ASSET_PHOTOMETRIC_PROFILE)
        {
            LOG("Expected file format {}\n", ASSET_PHOTOMETRIC_PROFILE);
            return false;
        }

        fileVersion = Stream.ReadUInt32();

        if (fileVersion != ASSET_VERSION_PHOTOMETRIC_PROFILE)
        {
            LOG("Expected file version {}\n", ASSET_VERSION_PHOTOMETRIC_PROFILE);
            return false;
        }

        m_Intensity = Stream.ReadFloat();
        Stream.Read(m_Data, sizeof(m_Data));
    }

    return true;
}

void APhotometricProfile::WritePhotometricData(RenderCore::ITexture* ProfileTexture, int FrameIndex)
{
    if (m_FrameNum == FrameIndex)
    {
        // Already updated at this frame
        return;
    }
    m_FrameNum = FrameIndex;
    if (ProfileTexture)
    {
        RenderCore::STextureRect rect;
        rect.Offset.Z    = m_PhotometricProfileCounter;
        rect.Dimension.X = PHOTOMETRIC_DATA_SIZE;
        rect.Dimension.Y = 1;
        rect.Dimension.Z = 1;
        ProfileTexture->WriteRect(rect, PHOTOMETRIC_DATA_SIZE, 4, m_Data);
        m_PhotometricProfileIndex   = m_PhotometricProfileCounter;
        m_PhotometricProfileCounter = (m_PhotometricProfileCounter + 1) & 0xff;
    }
}
