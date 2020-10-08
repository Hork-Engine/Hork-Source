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

#include "VT.h"
#include <RenderCore/DeviceObject.h>

class AVirtualTextureFile : public ARefCounted
{
public:
    AVirtualTextureFile( const char * FileName );
    ~AVirtualTextureFile();

    /** Resolution of virtual texture in pixels */
    uint32_t GetTextureResolution() const { return TextureResolution; }

    /** log2( TextureResolution ) */
    uint32_t GetTextureResolutionLog2() const { return TextureResolutionLog2; }

    /** Get page resolution width borders */
    int GetPageResolutionB() const { return PageResolutionB; }

    /** Get single page size in bytes */
    size_t GetPageSizeInBytes() const { return PageSizeInBytes; }

    int GetNumLayers() const { return Layers.Size(); }

    /** Read page from file. Can be used from stream thread */
    SFileOffset ReadPage( uint64_t PhysAddress, byte * PageData, int LayerIndex ) const;

    /** Read page from file. Can be used from stream thread */
    SFileOffset ReadPage( uint64_t PhysAddress, byte * PageData[] ) const;

    /** Read page physical address. Can be used from stream thread */
    SFileOffset GetPhysAddress( uint32_t PageIndex ) const;

protected:
    mutable SVirtualTextureFileHandle FileHandle;
    SFileOffset FileHeaderSize;
    int PageResolutionB;
    SVirtualTexturePIT PageInfoTable;
    SVirtualTextureAddressTable AddressTable;

    struct SLayer {
        int     SizeInBytes;
        int     PageDataFormat;
        //int     NumChannels;
        int     Offset; // Layers[i].Offset = Layers[ i - 1 ].Offset + Layers[ i - 1 ].SizeInBytes
    };

    TPodArray< SLayer > Layers;
    size_t PageSizeInBytes; // PageSizeInBytes = Layer[0].SizeInBytes + Layer[1].SizeInBytes + ... + Layer[Layers.size()-1].SizeInBytes

    /** Resolution of virtual texture in pixels */
    uint32_t TextureResolution;

    /** log2( TextureResolution ) */
    uint32_t TextureResolutionLog2;
};
