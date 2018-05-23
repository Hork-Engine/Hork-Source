/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

#include "Float.h"

class FColorHSL;
class FColorCMYK;
class FColorRGB;

class FColorHSL final {
public:
    Float3 Color;

    Float & operator[]( const int & _Index ) {
        return Color[ _Index ];
    }

    const Float & operator[]( const int & _Index ) const {
        return Color[ _Index ];
    }

    Bool operator==( const FColorHSL & _Other ) const { return Color == _Other.Color; }
    Bool operator!=( const FColorHSL & _Other ) const { return Color != _Other.Color; }

    Float & Hue() { return Color[0]; }
    Float & Saturation() { return Color[1]; }
    Float & Lightness() { return Color[2]; }

    void FromRGB( const FColorRGB & _RGB );
};

class FColorCMYK final {
public:
    Float3 Color;
    Float  K;

    Float & operator[]( const int & _Index ) {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Color.X)[ _Index ];
    }

    const Float & operator[]( const int & _Index ) const {
        AN_ASSERT( _Index >= 0 && _Index < 4, "Index out of range" );
        return (&Color.X)[ _Index ];
    }

    Bool operator==( const FColorCMYK & _Other ) const { return Color == _Other.Color; }
    Bool operator!=( const FColorCMYK & _Other ) const { return Color != _Other.Color; }

    Float & Cyan() { return Color[0]; }
    Float & Magenta() { return Color[1]; }
    Float & Yellow() { return Color[2]; }
    Float & Key() { return K; }

    void FromRGB( const FColorRGB & _RGB );
};

class FColorRGB final {
public:
    Float3 Color;

    Float & operator[]( const int & _Index ) {
        return Color[ _Index ];
    }

    const Float & operator[]( const int & _Index ) const {
        return Color[ _Index ];
    }

    Bool operator==( const FColorRGB & _Other ) const { return Color == _Other.Color; }
    Bool operator!=( const FColorRGB & _Other ) const { return Color != _Other.Color; }

    void FromHSL( const FColorHSL & _HSL );
    void FromCMYK( const FColorCMYK & _CMYK );

    void SetHue( const Float & _Hue ) {
        PackHSLValue( 0, _Hue );
    }

    Float GetHue() const {
        return UnpackHSLValue( 0 );
    }

    void ScaleHue( const Float & _Scale ) {
        ScaleHSLValue( 0, _Scale );
    }

    void SetSaturation( const Float & _Saturation ) {
        PackHSLValue( 1, _Saturation );
    }

    Float GetSaturation() const {
        return UnpackHSLValue( 1 );
    }

    void ScaleSaturation( const Float & _Scale ) {
        ScaleHSLValue( 1, _Scale );
    }

    void SetLightness( const Float & _Lightness ) {
        PackHSLValue( 2, _Lightness );
    }

    Float GetLightness() const {
        return UnpackHSLValue( 2 );
    }

    void ScaleLightness( const Float & _Scale ) {
        ScaleHSLValue( 2, _Scale );
    }

private:
    void PackHSLValue( const int & _Index, const Float & _Value ) {
        FColorHSL HSL;
        HSL.FromRGB( *this );
        HSL.Color[_Index] = _Value.Saturate();
        FromHSL( HSL );
    }

    Float UnpackHSLValue( const int & _Index ) const {
        FColorHSL HSL;
        HSL.FromRGB( *this );
        return HSL.Color[_Index];
    }

    void ScaleHSLValue( const int & _Index, const Float & _Value ) {
        FColorHSL HSL;
        HSL.FromRGB( *this );
        HSL.Color[_Index] = ( HSL.Color[_Index] * _Value ).Saturate();
        FromHSL( HSL );
    }
};

AN_FORCEINLINE void FColorHSL::FromRGB( const FColorRGB & _RGB ) {
    Float Max, Min;
    Float3 RGB = _RGB.Color * 255.0f;

    FMath::MinMax( RGB[0], RGB[1], RGB[2], Min, Max );

    Float Range = Max - Min;

    Float f = ( Range == 0.0f ) ? 0.0f : 60.0f / Range;

    if ( Max == RGB[0] ) {
        // R 360
        if ( RGB[1] < RGB[2] ) {
            Color[0] = ( 360.0f + f * ( RGB[1] - RGB[2] ) ) / 360.0f;
        } else {
            Color[0] = (          f * ( RGB[1] - RGB[2] ) ) / 360.0f;
        }
    } else if ( Max == RGB[1] ) {
        // G 120 degrees
        Color[0] = ( 120.0f + f * ( RGB[2] - RGB[0] ) ) / 360.0f;
    } else if ( Max == RGB[2] ) {
        // B 240 degrees
        Color[0] = ( 240.0f + f * ( RGB[0] - RGB[1] ) ) / 360.0f;
    } else {
        Color[0] = 0.0f;
    }

    Color[1] = Max == 0.0f ? Float(0.0f) : Range / Max;
    Color[2] = Max / 255.0f;

    Color = Color.Saturate();
}

AN_FORCEINLINE void FColorCMYK::FromRGB( const FColorRGB & _RGB ) {
    const Float MaxComponent = FMath::Max( FMath::Max( _RGB.Color[0], _RGB.Color[1] ), _RGB.Color[2] );

    Color = ( MaxComponent - _RGB.Color ) / MaxComponent;
    K = 1.0f - MaxComponent;
}

AN_FORCEINLINE void FColorRGB::FromHSL( const FColorHSL & _HSL ) {
    Float Max = _HSL[2];
    Float Min = ( 1.0f - _HSL[1] ) * _HSL[2];

    Float f = Max - Min;

    if ( _HSL[0] >= 0.0f && _HSL[0] <= 1.0f / 6.0f ) {
        Color[0] = Max;
        Color[1] = Min + _HSL[0] * f * 6.0f;
        Color[2] = Min;
        return;
    }

    if ( _HSL[0] <= 1.0f / 3.0f ) {
        Color[0] = Max - ( _HSL[0] - 1.0f / 6.0f ) * f * 6.0f;
        Color[1] = Max;
        Color[2] = Min;
        return;
    }

    if ( _HSL[0] <= 0.5f ) {
        Color[0] = Min;
        Color[1] = Max;
        Color[2] = Min + ( _HSL[0] - 1.0f / 3.0f ) * f * 6.0f;
        return;
    }

    if ( _HSL[0] <= 2.0f / 3.0f ) {
        Color[0] = Min;
        Color[1] = Max - ( _HSL[0] - 0.5f ) * f * 6.0f;
        Color[2] = Max;
        return;
    }

    if ( _HSL[0] <= 5.0f / 6.0f ) {
        Color[0] = Min + ( _HSL[0] - 2.0f / 3.0f ) * f * 6.0f;
        Color[1] = Min;
        Color[2] = Max;
        return;
    }

    if ( _HSL[0] <= 1.0f ) {
        Color[0] = Max;
        Color[1] = Min;
        Color[2] = Max - ( _HSL[0] - 5.0f / 6.0f ) * f * 6.0f;
        return;
    }

    Color[0] = Color[1] = Color[2] = 0.0f;
}

AN_FORCEINLINE void FColorRGB::FromCMYK( const FColorCMYK & _CMYK ) {
    Color = ( 1.0f - _CMYK.Color ) * ( 1.0f - _CMYK.K );
}

namespace FColorSpace {

AN_FORCEINLINE Int SetRedBits( Int _Color, byte _Red ) {
    return (_Color & 0x00ffffff) | (_Red<<24);
}

AN_FORCEINLINE Int SetGreenBits( Int _Color, byte _Green ) {
    return (_Color & 0xff00ffff) | (_Green<<16);
}

AN_FORCEINLINE Int SetBlueBits( Int _Color, byte _Blue ) {
    return (_Color & 0xffff00ff) | (_Blue<<8);
}

AN_FORCEINLINE Int SetAlphaBits( Int _Color, byte _Alpha ) {
    return (_Color & 0xffffff00) | _Alpha;
}

AN_FORCEINLINE Int SetRedBits_Swapped( Int _Color, byte _Red ) {
    return (_Color & 0xffffff00) | _Red;
}

AN_FORCEINLINE Int SetGreenBits_Swapped( Int _Color, byte _Green ) {
    return (_Color & 0xffff00ff) | (_Green<<8);
}

AN_FORCEINLINE Int SetBlueBits_Swapped( Int _Color, byte _Blue ) {
    return (_Color & 0xff00ffff) | (_Blue<<16);
}

AN_FORCEINLINE Int SetAlphaBits_Swapped( Int _Color, byte _Alpha ) {
    return (_Color & 0x00ffffff) | (_Alpha<<24);

}
AN_FORCEINLINE byte GetRedBits( Int _Color ) {
    return (_Color >> 24) & 0xff;
}

AN_FORCEINLINE byte GetGreenBits( Int _Color ) {
    return (_Color >> 16) & 0xff;
}

AN_FORCEINLINE byte GetBlueBits( Int _Color ) {
    return (_Color >> 8) & 0xff;
}

AN_FORCEINLINE byte GetAlphaBits( Int _Color ) {
    return _Color & 0xff;
}

AN_FORCEINLINE byte GetRedBits_Swapped( Int _Color ) {
    return _Color & 0xff;
}

AN_FORCEINLINE byte GetGreenBits_Swapped( Int _Color ) {
    return (_Color >> 8) & 0xff;
}

AN_FORCEINLINE byte GetBlueBits_Swapped( Int _Color ) {
    return (_Color >> 16) & 0xff;
}

AN_FORCEINLINE byte GetAlphaBits_Swapped( Int _Color ) {
    return (_Color >> 24) & 0xff;
}

AN_FORCEINLINE float GetRedBitsNorm( Int _Color ) {
    return GetRedBits( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetGreenBitsNorm( Int _Color ) {
    return GetGreenBits( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetBlueBitsNorm( Int _Color ) {
    return GetBlueBits( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetAlphaBitsNorm( Int _Color ) {
    return GetAlphaBits( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetRedBitsNorm_Swapped( Int _Color ) {
    return GetRedBits_Swapped( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetGreenBitsNorm_Swapped( Int _Color ) {
    return GetGreenBits_Swapped( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetBlueBitsNorm_Swapped( Int _Color ) {
    return GetBlueBits_Swapped( _Color ) / 255.0f;
}

AN_FORCEINLINE float GetAlphaBitsNorm_Swapped( Int _Color ) {
    return GetAlphaBits_Swapped( _Color ) / 255.0f;
}

AN_FORCEINLINE Int SwapToBGR( Int _Color ) {
    return SetBlueBits( SetRedBits( _Color, GetBlueBits( _Color ) ), GetRedBits( _Color ) );
}

AN_FORCEINLINE Int PackRGBAToDWord( const byte _RGBA[4] ) {
    return (int(_RGBA[0]) << 24) | (int(_RGBA[1]) << 16) | (int(_RGBA[2]) << 8) | int(_RGBA[3]);
}

AN_FORCEINLINE Int PackRGBAToDWord( const byte & _R, const byte & _G, const byte & _B, const byte & _A ) {
    return (int(_R) << 24) | (int(_G) << 16) | (int(_B) << 8) | int(_A);
}

AN_FORCEINLINE Int PackRGBAToDWord_Swapped( const byte _RGBA[4] ) {
    return (int(_RGBA[3]) << 24) | (int(_RGBA[2]) << 16) | (int(_RGBA[1]) << 8) | int(_RGBA[0]);
}

AN_FORCEINLINE Int PackRGBAToDWord_Swapped( const byte & _R, const byte & _G, const byte & _B, const byte & _A ) {
    return (int(_A) << 24) | (int(_B) << 16) | (int(_G) << 8) | int(_R);
}

AN_FORCEINLINE Int PackNRGBAToDWord( const Float4 & _Color ) {
    return (_Color.W.Saturate() * 255.0f).ToIntFast()
        | ((_Color.Z.Saturate() * 255.0f).ToIntFast() << 8)
        | ((_Color.Y.Saturate() * 255.0f).ToIntFast() << 16)
        | ((_Color.X.Saturate() * 255.0f).ToIntFast() << 24);
}

AN_FORCEINLINE Int PackNRGBAToDWord( const Float & _R, const Float & _G, const Float & _B, const Float & _A ) {
    return (_A.Saturate() * 255.0f).ToIntFast()
        | ((_B.Saturate() * 255.0f).ToIntFast() << 8)
        | ((_G.Saturate() * 255.0f).ToIntFast() << 16)
        | ((_R.Saturate() * 255.0f).ToIntFast() << 24);
}

AN_FORCEINLINE Int PackNRGBAToDWord_Swapped( const Float4 & _Color ) {
    return (_Color.X.Saturate() * 255.0f).ToIntFast()
        | ((_Color.Y.Saturate() * 255.0f).ToIntFast() << 8)
        | ((_Color.Z.Saturate() * 255.0f).ToIntFast() << 16)
        | ((_Color.W.Saturate() * 255.0f).ToIntFast() << 24);
}

AN_FORCEINLINE Int PackNRGBAToDWord_Swapped( const Float & _R, const Float & _G, const Float & _B, const Float & _A ) {
    return (_R.Saturate() * 255.0f).ToIntFast()
        | ((_G.Saturate() * 255.0f).ToIntFast() << 8)
        | ((_B.Saturate() * 255.0f).ToIntFast() << 16)
        | ((_A.Saturate() * 255.0f).ToIntFast() << 24);
}

AN_FORCEINLINE Float4 UnpackNRGBAFromDWord( const Int & _Color ) {
    return Float4( GetRedBitsNorm( _Color ),
                   GetGreenBitsNorm( _Color ),
                   GetBlueBitsNorm( _Color ),
                   GetAlphaBitsNorm( _Color ) );
}

AN_FORCEINLINE Float4 UnpackNRGBAFromDWord_Swapped( const Int & _Color ) {
    return Float4( GetAlphaBitsNorm( _Color ),
                   GetBlueBitsNorm( _Color ),
                   GetGreenBitsNorm( _Color ),
                   GetRedBitsNorm( _Color ) );
}

AN_FORCEINLINE unsigned short PackRGBTo565( const byte _RGB[3] ) {
    return ( ( _RGB[ 0 ] >> 3 ) << 11 ) | ( ( _RGB[ 1 ] >> 2 ) << 5 ) | ( _RGB[ 2 ] >> 3 );
}

AN_FORCEINLINE void UnpackRGBFrom565( unsigned short _565, byte _RGB[3] ) {
    _RGB[0] = byte( ( ( _565 >> 8 ) & ( ( ( 1 << ( 8 - 3 ) ) - 1 ) << 3 ) ) | ( ( _565 >> 13 ) & ((1<<3)-1) ) );
    _RGB[1] = byte( ( ( _565 >> 3 ) & ( ( ( 1 << ( 8 - 2 ) ) - 1 ) << 2 ) ) | ( ( _565 >>  9 ) & ((1<<2)-1) ) );
    _RGB[2] = byte( ( ( _565 << 3 ) & ( ( ( 1 << ( 8 - 3 ) ) - 1 ) << 3 ) ) | ( ( _565 >>  2 ) & ((1<<3)-1) ) );
}

AN_FORCEINLINE void RGBAToYCoCgAlpha( const byte _RGBA[4], byte _YCoCgAlpha[4] ) {
    int r = _RGBA[0];
    int g = _RGBA[1];
    int b = _RGBA[2];
    _YCoCgAlpha[0] = Int( ( ( r + ( g << 1 ) +  b ) + 2 ) >> 2 ).Clamp( 0, 255 );
    _YCoCgAlpha[1] = Int( ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 ) + 128 ).Clamp( 0, 255 );
    _YCoCgAlpha[2] = Int( ( ( ( -r + ( g << 1 ) - b ) + 2 ) >> 2 ) + 128 ).Clamp( 0, 255 );
    _YCoCgAlpha[3] = _RGBA[3];
}

AN_FORCEINLINE void RGBToYCoCg( const byte _RGB[3], byte _YCoCg[3] ) {
    int r = _RGB[0];
    int g = _RGB[1];
    int b = _RGB[2];
    _YCoCg[0] = Int( ( ( r + ( g << 1 ) +  b ) + 2 ) >> 2 ).Clamp( 0, 255 );
    _YCoCg[1] = Int( ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 ) + 128 ).Clamp( 0, 255 );
    _YCoCg[2] = Int( ( ( ( -r + ( g << 1 ) - b ) + 2 ) >> 2 ) + 128 ).Clamp( 0, 255 );
}

AN_FORCEINLINE void YCoCgAlphaToRGBA( const byte _YCoCgAlpha[4], byte _RGBA[4] ) {
    int y  = _YCoCgAlpha[0];
    int co = _YCoCgAlpha[1] - 128;
    int cg = _YCoCgAlpha[2] - 128;
    _RGBA[0] = Int( y + ( co - cg ) ).Clamp( 0, 255 );
    _RGBA[1] = Int( y + ( cg ) ).Clamp( 0, 255 );
    _RGBA[2] = Int( y + ( - co - cg ) ).Clamp( 0, 255 );
    _RGBA[3] = _YCoCgAlpha[3];
}

AN_FORCEINLINE void YCoCgToRGB( const byte _YCoCg[3], byte _RGB[3] ) {
    int y  = _YCoCg[0];
    int co = _YCoCg[1] - 128;
    int cg = _YCoCg[2] - 128;
    _RGB[0] = Int( y + ( co - cg ) ).Clamp( 0, 255 );
    _RGB[1] = Int( y + ( cg ) ).Clamp( 0, 255 );
    _RGB[2] = Int( y + ( - co - cg ) ).Clamp( 0, 255 );
}

AN_FORCEINLINE void RGBToCoCg_Y( const byte _RGB[3], byte _CoCg_Y[4] ) {
    int r = _RGB[0];
    int g = _RGB[1];
    int b = _RGB[2];

    _CoCg_Y[0] = Int( ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 ) + 128 ).Clamp( 0, 255 );
    _CoCg_Y[1] = Int( ( ( ( -r + ( g << 1) - b ) + 2 ) >> 2 ) + 128 ).Clamp( 0, 255 );
    _CoCg_Y[2] = 0;
    _CoCg_Y[3] = Int( ( ( r + ( g << 1 ) +  b ) + 2 ) >> 2 ).Clamp( 0, 255 );
}

AN_FORCEINLINE void CoCg_YToRGB( const byte _CoCg_Y[4], byte _RGB[3] ) {
    int y  = _CoCg_Y[3];
    int co = _CoCg_Y[0] - 128;
    int cg = _CoCg_Y[1] - 128;
    _RGB[0] = Int( y + ( co - cg ) ).Clamp( 0, 255 );
    _RGB[1] = Int( y + ( cg ) ).Clamp( 0, 255 );
    _RGB[2] = Int( y + ( - co - cg ) ).Clamp( 0, 255 );
}

} // namespace FColorSpace
