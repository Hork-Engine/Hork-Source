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

#pragma once

#include "IO.h"

enum EMipmapEdgeMode
{
    MIPMAP_EDGE_CLAMP = 1,
    MIPMAP_EDGE_REFLECT = 2,
    MIPMAP_EDGE_WRAP = 3,
    MIPMAP_EDGE_ZERO = 4,
};

enum EMipmapFilter
{
    /** A trapezoid w/1-pixel wide ramps, same result as box for integer scale ratios */
    MIPMAP_FILTER_BOX = 1,
    /** On upsampling, produces same results as bilinear texture filtering */
    MIPMAP_FILTER_TRIANGLE = 2,
    /** The cubic b-spline (aka Mitchell-Netrevalli with B=1,C=0), gaussian-esque */
    MIPMAP_FILTER_CUBICBSPLINE = 3,
    /** An interpolating cubic spline */
    MIPMAP_FILTER_CATMULLROM = 4,
    /** Mitchell-Netrevalli filter with B=1/3, C=1/3 */
    MIPMAP_FILTER_MITCHELL = 5
};

/** Software mipmap generator */
struct SSoftwareMipmapGenerator
{
    const void * SourceImage = nullptr;
    int Width = 0;
    int Height = 0;
    int NumChannels = 0;
    EMipmapEdgeMode EdgeMode = MIPMAP_EDGE_WRAP;
    EMipmapFilter Filter = MIPMAP_FILTER_MITCHELL;
    bool bLinearSpace = false;
    bool bPremultipliedAlpha = false;
    bool bHDRI = false;
};

struct SImageMipmapConfig {
    EMipmapEdgeMode EdgeMode = MIPMAP_EDGE_WRAP;
    EMipmapFilter Filter = MIPMAP_FILTER_MITCHELL;
    bool bPremultipliedAlpha = false;
};

/** Image loader */
class AImage {
public:
    void * pRawData;
    int Width;
    int Height;
    int NumChannels;
    bool bHDRI;
    bool bLinearSpace;
    bool bHalf; // Only for HDRI images
    int NumLods;

    AImage();
    ~AImage();

    // Load image as byte*
    bool LoadLDRI( const char * _Path, bool _SRGB, SImageMipmapConfig const * _MipmapGen, int _NumDesiredChannels = 0 );
    bool LoadLDRI( IBinaryStream & _Stream, bool _SRGB, SImageMipmapConfig const * _MipmapGen, int _NumDesiredChannels = 0 );

    // Load image as float* in linear space
    bool LoadHDRI( const char * _Path, bool _HalfFloat, SImageMipmapConfig const * _MipmapGen, int _NumDesiredChannels = 0 );
    bool LoadHDRI( IBinaryStream & _Stream, bool _HalfFloat, SImageMipmapConfig const * _MipmapGen, int _NumDesiredChannels = 0 );

    void FromRawDataLDRI( const byte * _Data, int _Width, int _Height, int _NumChannels, bool _SRGB, SImageMipmapConfig const * _MipmapGen );

    void Free();
};

/*

Utilites

*/

/** Flip image horizontally */
void FlipImageX( void * _ImageData, int _Width, int _Height, int _BytesPerPixel, int _BytesPerLine );

/** Flip image vertically */
void FlipImageY( void * _ImageData, int _Width, int _Height, int _BytesPerPixel, int _BytesPerLine );

/** Convert linear image to premultiplied alpha sRGB */
void LinearToPremultipliedAlphaSRGB( const float * SourceImage,
                                     int Width,
                                     int Height,
                                     bool bOverbright,
                                     float fOverbright,
                                     bool bReplaceAlpha,
                                     float fReplaceAlpha,
                                     byte * sRGB );

/** Calculate required size in bytes for mipmapped image */
void ComputeRequiredMemorySize( SSoftwareMipmapGenerator const & _Config, int & _RequiredMemory, int & _NumLods );

/** Generate image mipmaps */
void GenerateMipmaps( SSoftwareMipmapGenerator const & _Config, void * _Data );

/** Write image in PNG format */
bool WritePNG( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData, int _BytesPerLine );

/** Write image in BMP format */
bool WriteBMP( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData );

/** Write image in TGA format */
bool WriteTGA( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData );

/** Write image in JPG format */
bool WriteJPG( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const void * _ImageData, int _Quality );

/** Write image in HDR format */
bool WriteHDR( IBinaryStream & _Stream, int _Width, int _Height, int _NumChannels, const float * _ImageData );
