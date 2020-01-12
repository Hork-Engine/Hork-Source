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

#include <World/Public/Components/DirectionalLightComponent.h>
#include <World/Public/Base/DebugRenderer.h>

struct STemperatureToColor {

    static constexpr float MIN_TEMPERATURE = 1000.0f;
    static constexpr float MAX_TEMPERATURE = 40000.0f;

    static constexpr float LumensToEnergy = 4.0f * 16.0f / 10000.0f;

    // Convert temperature in Kelvins to RGB color. Assume temperature is range between 1000 and 40000.
    // Based on code by Benjamin 'BeRo' Rosseaux
    static Float3 GetRGBFromTemperature( float _Temperature ) {
        Float3 Result;

        if ( _Temperature <= 6500.0f ) {
            Result.X = 1.0f;
            Result.Y = -2902.1955373783176f / ( 1669.5803561666639f + _Temperature ) + 1.3302673723350029f;
            Result.Z = -8257.7997278925690f / ( 2575.2827530017594f + _Temperature ) + 1.8993753891711275f;

            Result.Z = Math::Max< float >( 0.0f, Result.Z );
        } else {
            Result.X = 1745.0425298314172f / ( -2666.3474220535695f + _Temperature ) + 0.55995389139931482f;
            Result.Y = 1216.6168361476490f / ( -2173.1012343082230f + _Temperature ) + 0.70381203140554553f;
            Result.Z = -8257.7997278925690f / ( 2575.2827530017594f + _Temperature ) + 1.8993753891711275f;

            Result.X = Math::Min< float >( 1.0f, Result.X );
            Result.Z = Math::Min< float >( 1.0f, Result.Z );
        }

        return Result;
    }

    // Convert temperature in Kelvins to RGB color. Assume temperature is range between 1000 and 40000.
    // Based on code from http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
    static Float3 GetRGBFromTemperature2( float _Temperature ) {
        Float3 Result;
        float Value;

        // All calculations require _Temperature / 100, so only do the conversion once
        _Temperature *= 0.01f;

        // Calculate each color in turn

        if ( _Temperature <= 66 ) {
            Result.X = 1.0f;

            // Note: the R-squared value for this approximation is .996
            Value = (99.4708025861/255.0) * log(_Temperature) - (161.1195681661/255.0);
            Result.Y = Math::Min( 1.0f, Value );//Math::Clamp( Value, 0.0f, 1.0f );

        } else {
            // Note: the R-squared value for this approximation is .988
            Value = ( 329.698727446 / 255.0 ) * pow(_Temperature - 60, -0.1332047592);
            Result.X = Math::Min( 1.0f, Value );//Math::Clamp( Value, 0.0f, 1.0f );

            // Note: the R-squared value for this approximation is .987
            Value = (288.1221695283/255.0) * pow(_Temperature - 60, -0.0755148492);
            Result.Y = Value;//Math::Clamp( Value, 0.0f, 1.0f );
        }

        if ( _Temperature >= 66 ) {
            Result.Z = 1.0f;
        } else if ( _Temperature <= 19 ) {
            Result.Z = 0.0f;
        } else {
            // Note: the R-squared value for this approximation is .998
            Value = (138.5177312231/255.0) * log(_Temperature - 10) - (305.0447927307/255.0);
            Result.Z = Math::Max( 0.0f, Value );//Math::Clamp( Value, 0.0f, 1.0f );
        }

        return Result;
    }

#if 0
    // Convert temperature in Kelvins to RGB color. Assume temperature is range between 1000 and 10000.
    // Based on Urho sources
    static Float3 GetRGBFromTemperature( float _Temperature ) {
        // Approximate Planckian locus in CIE 1960 UCS
        float u = (0.860117757f + 1.54118254e-4f * _Temperature + 1.28641212e-7f * _Temperature * _Temperature) /
            (1.0f + 8.42420235e-4f * _Temperature + 7.08145163e-7f * _Temperature * _Temperature);
        float v = (0.317398726f + 4.22806245e-5f * _Temperature + 4.20481691e-8f * _Temperature * _Temperature) /
            (1.0f - 2.89741816e-5f * _Temperature + 1.61456053e-7f * _Temperature * _Temperature);

        float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
        float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
        float z = 1.0f - x - y;

        float y_ = 1.0f;
        float x_ = y_ / y * x;
        float z_ = y_ / y * z;

        Float3 Color;
        Color.X = 3.2404542f * x_ + -1.5371385f * y_ + -0.4985314f * z_;
        Color.Y = -0.9692660f * x_ + 1.8760108f * y_ + 0.0415560f * z_;
        Color.Z = 0.0556434f * x_ + -0.2040259f * y_ + 1.0572252f * z_;

        return Color;
    }
#endif

    static float GetLightEnergyFromLumens( float _Lumens ) {
        return _Lumens * LumensToEnergy;
    }
};
