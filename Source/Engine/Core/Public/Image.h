/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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
    int AlphaChannel = -1;
    EMipmapEdgeMode EdgeMode = MIPMAP_EDGE_WRAP;
    EMipmapFilter Filter = MIPMAP_FILTER_MITCHELL;
    bool bLinearSpace = false;
    bool bPremultipliedAlpha = false;
    bool bHDRI = false;
};

struct SImageMipmapConfig
{
    EMipmapEdgeMode EdgeMode = MIPMAP_EDGE_WRAP;
    EMipmapFilter Filter = MIPMAP_FILTER_MITCHELL;
    bool bPremultipliedAlpha = false;
};

enum EImagePixelFormat
{
    IMAGE_PF_AUTO,
    IMAGE_PF_AUTO_GAMMA2,
    IMAGE_PF_AUTO_16F,
    IMAGE_PF_AUTO_32F,
    IMAGE_PF_R,
    IMAGE_PF_R16F,
    IMAGE_PF_R32F,
    IMAGE_PF_RG,
    IMAGE_PF_RG16F,
    IMAGE_PF_RG32F,
    IMAGE_PF_RGB,
    IMAGE_PF_RGB_GAMMA2,
    IMAGE_PF_RGB16F,
    IMAGE_PF_RGB32F,    
    IMAGE_PF_RGBA,
    IMAGE_PF_RGBA_GAMMA2,
    IMAGE_PF_RGBA16F,
    IMAGE_PF_RGBA32F,
    IMAGE_PF_BGR,
    IMAGE_PF_BGR_GAMMA2,
    IMAGE_PF_BGR16F,
    IMAGE_PF_BGR32F,
    IMAGE_PF_BGRA,
    IMAGE_PF_BGRA_GAMMA2,
    IMAGE_PF_BGRA16F,
    IMAGE_PF_BGRA32F
};

/** Image loader */
class AImage {
public:
    AImage();
    ~AImage();

    bool Load( const char * _Path, SImageMipmapConfig const * _MipmapGen, EImagePixelFormat _PixelFormat = IMAGE_PF_AUTO_GAMMA2 );
    bool Load( IBinaryStream & _Stream, SImageMipmapConfig const * _MipmapGen, EImagePixelFormat _PixelFormat = IMAGE_PF_AUTO_GAMMA2 );

    /** Source data must be float* or byte* according to specified pixel format */
    void FromRawData( const void * _Source, int _Width, int _Height, SImageMipmapConfig const * _MipmapGen, EImagePixelFormat _PixelFormat );

    void Free();

    void * GetData() const { return pRawData; }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    int GetNumLods() const { return NumLods; }
    EImagePixelFormat GetPixelFormat() const { return PixelFormat; }

private:
    void FromRawData( const void * _Source, int _Width, int _Height, SImageMipmapConfig const * _MipmapGen, EImagePixelFormat _PixelFormat, bool bReuseSourceBuffer );

    void * pRawData;
    int Width;
    int Height;
    int NumLods;
    EImagePixelFormat PixelFormat;
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

enum EImageDataType
{
    IMAGE_DATA_TYPE_UINT8,
    IMAGE_DATA_TYPE_UINT16,
    IMAGE_DATA_TYPE_UINT32,
    IMAGE_DATA_TYPE_FLOAT
};

struct SImageResizeDesc
{
    /** Source image */
    const void * pImage;
    /** Source image width */
    int Width;
    /** Source image height */
    int Height;
    /** Source image channels count*/
    int NumChannels;
    /** Source image alpha channel index. Use -1 if image has no alpha channel. */
    int AlphaChannel;
    /** Image data type */
    EImageDataType DataType;
    /** Set this flag if your image has premultiplied alpha. Otherwise, will be
    used alpha-weighted resampling (effectively premultiplying, resampling, then unpremultiplying). */
    bool bPremultipliedAlpha;
    /** Is your image in linear color space or sRGB. */
    bool bLinearSpace;
    /** Scaling edge mode for horizontal axis */
    EMipmapEdgeMode HorizontalEdgeMode;
    /** Scaling edge mode for vertical axis */
    EMipmapEdgeMode VerticalEdgeMode;
    /** Scaling filter for horizontal axis */
    EMipmapFilter HorizontalFilter;
    /** Scaling filter for vertical axis */
    EMipmapFilter VerticalFilter;
    /** Scaled image width */
    int ScaledWidth;
    /** Scaled image height */
    int ScaledHeight;
};

/** Scale image */
void ResizeImage( SImageResizeDesc const & InDesc, void * pScaledImage );

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
