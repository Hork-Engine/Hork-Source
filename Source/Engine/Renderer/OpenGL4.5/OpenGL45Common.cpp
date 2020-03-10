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

#include "OpenGL45Common.h"
#include <Core/Public/Color.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace OpenGL45 {

GHI::Device          GDevice;
GHI::State           GState;
GHI::CommandBuffer   Cmd;
SRenderFrame *       GFrameData;
SRenderView *        GRenderView;

void SaveSnapshot( GHI::Texture & _Texture ) {

    const int w = _Texture.GetWidth();
    const int h = _Texture.GetHeight();
    const int bpp = 3;
    const int size = w * h * bpp;

    int hunkMark = GHunkMemory.SetHunkMark();

    byte * data = (byte *)GHunkMemory.Alloc( size );

#if 0
    _Texture.Read( 0, GHI::PIXEL_FORMAT_BYTE_RGB, size, 1, data );
#else
    float * fdata = (float *)GHunkMemory.Alloc( size*sizeof(float) );
    _Texture.Read( 0, GHI::PIXEL_FORMAT_FLOAT_RGB, size*sizeof(float), 1, fdata );
    // to sRGB
    for ( int i = 0 ; i < size ; i++ ) {
        data[i] = ConvertToSRGB( fdata[i] )*255.0f;
    }
#endif

    // Flip vertical
    int halfHeight = h >> 1;
    for ( int i = 0 ; i < halfHeight ; i++ ) {
        byte * line1 = &data[i*w*bpp];
        byte * line2 = &data[(h-i-1)*w*bpp];
        for ( int x = 0 ; x < w ; x++ ) {
            StdSwap( line1[0], line2[0] );
            StdSwap( line1[1], line2[1] );
            StdSwap( line1[2], line2[2] );
            line1 += bpp;
            line2 += bpp;
        }
    }

    static int n = 0;
    if ( n == 0 ) {
        Core::MakeDir( "snapshots", false );
    }

    stbi_write_png( Core::Fmt( "snapshots/%d.png", n ), w, h, bpp, data, w*bpp );

    n++;

    GHunkMemory.ClearToMark( hunkMark );
}


}
