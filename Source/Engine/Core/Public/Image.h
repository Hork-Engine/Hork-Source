/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

/** Software mipmap generator */
struct ASoftwareMipmapGenerator {
    void * SourceImage;
    int Width;
    int Height;
    int NumChannels;
    bool bLinearSpace;
    bool bHDRI;

    void ComputeRequiredMemorySize( int & _RequiredMemory, int & _NumLods ) const;
    void GenerateMipmaps( void * _Data );
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
    bool LoadLDRI( const char * _Path, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );
    bool LoadLDRI( IStreamBase & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );

    // Load image as float* in linear space
    bool LoadHDRI( const char * _Path, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );
    bool LoadHDRI( IStreamBase & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );

    void FromRawDataLDRI( const byte * _Data, int _Width, int _Height, int _NumChannels, bool _SRGB, bool _GenerateMipmaps );

    void Free();
};
