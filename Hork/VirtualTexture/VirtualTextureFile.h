/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "VT.h"
#include <Hork/RHI/Common/DeviceObject.h>

HK_NAMESPACE_BEGIN

class VirtualTextureFile : public RefCounted
{
public:
                                VirtualTextureFile(const char* fileName);
                                ~VirtualTextureFile();

    /// Resolution of virtual texture in pixels
    uint32_t                    GetTextureResolution() const { return m_TextureResolution; }

    /// log2( TextureResolution )
    uint32_t                    GetTextureResolutionLog2() const { return m_TextureResolutionLog2; }

    /// Get page resolution width borders
    int                         GetPageResolutionB() const { return m_PageResolutionB; }

    /// Get single page size in bytes
    size_t                      GetPageSizeInBytes() const { return m_PageSizeInBytes; }

    int                         GetNumLayers() const { return m_Layers.Size(); }

    /// Read page from file. Can be used from stream thread
    VTFileOffset                ReadPage(uint64_t physAddress, byte* pageData, int LayerIndex) const;

    /// Read page from file. Can be used from stream thread
    VTFileOffset                ReadPage(uint64_t physAddress, byte* pageData[]) const;

    /// Read page physical address. Can be used from stream thread
    VTFileOffset                GetPhysAddress(uint32_t pageIndex) const;

protected:
    mutable VTFileHandle        m_FileHandle;
    VTFileOffset                m_FileHeaderSize;
    int                         m_PageResolutionB;
    VirtualTexturePIT           m_PageInfoTable;
    VirtualTextureAddressTable  m_AddressTable;

    struct Layer
    {
        int                     SizeInBytes;
        int                     PageDataFormat;
        //int                   NumChannels;
        int                     Offset; // m_Layers[i].Offset = m_Layers[ i - 1 ].Offset + m_Layers[ i - 1 ].SizeInBytes
    };

    Vector<Layer>               m_Layers;
    size_t                      m_PageSizeInBytes; // m_PageSizeInBytes = Layer[0].SizeInBytes + Layer[1].SizeInBytes + ... + Layer[m_Layers.size()-1].SizeInBytes

    /// Resolution of virtual texture in pixels
    uint32_t                    m_TextureResolution;

    /// log2( TextureResolution )
    uint32_t                    m_TextureResolutionLog2;
};

HK_NAMESPACE_END
