/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Engine/Core/Containers/Vector.h>
#include <Engine/Core/String.h>

HK_NAMESPACE_BEGIN

constexpr size_t PHOTOMETRIC_DATA_SIZE = 256;

class PhotometricData
{
public:
    // Number of lamps
    int m_NumLamps{};
    // Lumens per lamp
    float m_LumensLamp{};
    // Candela multiplying factor
    float m_LampMultiplier{};
    // Photometric goniometer type 3 - Type A, 2 - Type B, 1 - Type C
    int m_GonioType{};
    // Measurement units 1 = Feet, 2 = Meters
    int m_Units{};
    // Opening width
    float m_DimWidth{};
    // Opening length
    float m_DimLength{};
    // Cavity height
    float m_DimHeight{};
    // Ballast factor
    float m_ElecBallFactor{};
    // Ballast-lamp photometric factor
    float m_ElecBlpFactor{};
    // Input watts
    float m_ElecInputWatts{};
    // Vertical angles
    TVector<float> m_VertAngles;
    // Horizontal angles
    TVector<float> m_HorzAngles;
    // Candela values
    TVector<TVector<float>> m_Candela;

    operator bool() const
    {
        return !m_VertAngles.IsEmpty() && !m_HorzAngles.IsEmpty();
    }

    void  ReadSamples(uint8_t* Data, float& Intensity) const;
    float SampleAvgVertical(float VerticalAgnle) const;
    float Sample2D(float x, float y) const;
    float SampleAvg(float x, float y) const;
};

PhotometricData ParsePhotometricData(StringView Text);

HK_NAMESPACE_END
