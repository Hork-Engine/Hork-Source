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

#include "Resource.h"
#include <Engine/RenderCore/Texture.h>
#include <Engine/Image/PhotometricData.h>

HK_NAMESPACE_BEGIN

class Texture;

/**

PhotometricProfile

*/
class PhotometricProfile : public Resource
{
    HK_CLASS(PhotometricProfile, Resource)

public:
    PhotometricProfile();
    ~PhotometricProfile();

    static PhotometricProfile* Create(void const* pData, float Intensity);

    void SetIntensity(float Intensity) { m_Intensity = Intensity; }

    /** Get intensity scale to convert data to candelas */
    float GetIntensity() const { return m_Intensity; }

    const byte* GetPhotometricData() const { return m_Data; }

    /** Internal */
    void WritePhotometricData(RenderCore::ITexture* ProfileTexture, int FrameIndex);
    int  GetPhotometricProfileIndex() const { return m_PhotometricProfileIndex; }

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/PhotometricProfile/Default"; }

    int   m_PhotometricProfileIndex = 0;
    int   m_FrameNum                = -1;
    float m_Intensity               = 0.0f;
    byte  m_Data[PHOTOMETRIC_DATA_SIZE];

    static int m_PhotometricProfileCounter;
};

HK_NAMESPACE_END
