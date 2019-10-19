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

#include "OpenGL45GPUSync.h"

using namespace GHI;

namespace OpenGL45 {

FGPUSync GOpenGL45GPUSync;

void FGPUSync::Wait() {
    if ( !bCreated ) {
        bCreated = true;

        TextureCreateInfo textureCI = {};
        textureCI.Type = GHI::TEXTURE_2D;
        textureCI.Resolution.Tex2D.Width = 2;
        textureCI.Resolution.Tex2D.Height = 2;
        textureCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGBA8;

        byte data[2*2*4];
        memset( data, 128, sizeof( data ) );

        TextureInitialData initial;
        initial.PixelFormat = PIXEL_FORMAT_UBYTE_RGBA;
        initial.SysMem = data;
        initial.Alignment = 1;
        initial.SizeInBytes = sizeof( data );

        Texture.Initialize( textureCI, &initial );

        textureCI.Resolution.Tex2D.Width = 1;
        textureCI.Resolution.Tex2D.Height = 1;

        Staging.Initialize( textureCI );
    } else {
        TextureCopy copy = {};

        copy.SrcRect.Offset.Lod = 1;
        copy.SrcRect.Offset.X = 0;
        copy.SrcRect.Offset.Y = 0;
        copy.SrcRect.Offset.Z = 0;
        copy.SrcRect.Dimension.X = 1;
        copy.SrcRect.Dimension.Y = 1;
        copy.SrcRect.Dimension.Z = 1;
        copy.DstOffset.Lod = 0;
        copy.DstOffset.X = 0;
        copy.DstOffset.Y = 0;
        copy.DstOffset.Z = 0;

        Cmd.CopyTextureRect( Texture, Staging, 1, &copy );

        byte data[4];
        Staging.Read( 0,  PIXEL_FORMAT_UBYTE_RGBA, 4, 1, data );

        //GLogger.Printf("Data %d %d %d %d\n",data[0],data[1],data[2],data[3]);
    }
}

void FGPUSync::SetEvent() {
    //if ( !GSyncGPU.Load() ) {
    //    return;
    //}

    if ( bCreated ) {
        Texture.GenerateLods();
    }
}

void FGPUSync::Release() {
    if ( bCreated ) {
        bCreated = false;

        Texture.Deinitialize();
        Staging.Deinitialize();
    }
}

}
