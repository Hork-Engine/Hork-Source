/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

class ATexture;

/**

APhotometricProfile

*/
class APhotometricProfile : public AResource {
    AN_CLASS( APhotometricProfile, AResource )

public:
    enum { PHOTOMETRIC_DATA_SIZE = 256 };

    void Initialize( const byte * InData, float InIntensity );

    void SetIntensity( float _Intensity ) { Intensity = _Intensity; }

    /** Get intensity scale to convert data to candelas */
    float GetIntensity() const { return Intensity; }

    const byte * GetPhotometricData() const { return Data; }

    /** Internal */
    void WritePhotometricData( ATexture * ProfileTexture, int FrameIndex );
    int GetPhotometricProfileIndex() const { return PhotometricProfileIndex; }

protected:
    APhotometricProfile();
    ~APhotometricProfile();

    /** Load resource from file */
    bool LoadResource( IBinaryStream & Stream ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/PhotometricProfile/Default"; }

    int PhotometricProfileIndex = 0;
    int FrameNum = -1;
    float Intensity = 0.0f;
    byte Data[PHOTOMETRIC_DATA_SIZE];

    static int PhotometricProfileCounter;
};
