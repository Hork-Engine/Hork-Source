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

#include "VirtualTextureFile.h"
#include "QuadTree.h"

#include <Hork/Core/Logger.h>
#include <Hork/Core/BaseMath.h>

HK_NAMESPACE_BEGIN

VirtualTextureFile::VirtualTextureFile(const char* fileName)
{
    VTFileOffset fileOffset;
    uint32_t version;
    byte tmp;

    m_FileHeaderSize = 0;
    m_TextureResolution = 0;
    m_TextureResolutionLog2 = 0;

    if (!m_FileHandle.OpenRead(fileName))
    {
        LOG("VirtualTextureFile::ctor: couldn't open {}\n", fileName);
        return;
    }

    fileOffset = 0;

    // read version
    m_FileHandle.Read(&version, sizeof(version), fileOffset);
    fileOffset += sizeof(version);

    if (version != VT_FILE_ID)
    {
        m_FileHandle.Close();
        return;
    }

    // read num layers
    m_FileHandle.Read(&tmp, sizeof(byte), fileOffset);
    fileOffset += sizeof(byte);

    m_Layers.Resize(tmp);
    int LayerOffset = 0;
    for (int i = 0; i < m_Layers.Size(); i++)
    {

        m_FileHandle.Read(&m_Layers[i].SizeInBytes, sizeof(m_Layers[i].SizeInBytes), fileOffset);
        fileOffset += sizeof(m_Layers[i].SizeInBytes);

        m_FileHandle.Read(&m_Layers[i].PageDataFormat, sizeof(m_Layers[i].PageDataFormat), fileOffset);
        fileOffset += sizeof(m_Layers[i].PageDataFormat);

        //m_FileHandle.Read( &m_Layers[i].NumChannels, sizeof( m_Layers[i].NumChannels ), fileOffset );
        //fileOffset += sizeof( m_Layers[i].NumChannels );

        m_Layers[i].Offset = LayerOffset;

        LayerOffset += m_Layers[i].SizeInBytes;
    }
    m_PageSizeInBytes = LayerOffset;

    // read page width
    m_FileHandle.Read(&m_PageResolutionB, sizeof(m_PageResolutionB), fileOffset);
    fileOffset += sizeof(m_PageResolutionB);

    // read page info table
    fileOffset += m_PageInfoTable.Read(&m_FileHandle, fileOffset);

    // read page address tables
    fileOffset += m_AddressTable.Read(&m_FileHandle, fileOffset);

    m_FileHeaderSize = fileOffset;

    m_TextureResolution = (1u << (m_AddressTable.NumLods - 1)) * m_PageResolutionB;
    m_TextureResolutionLog2 = Math::Log2(m_TextureResolution);
}

VirtualTextureFile::~VirtualTextureFile()
{
}

VTFileOffset VirtualTextureFile::GetPhysAddress(unsigned int pageIndex) const
{
    VTFileOffset physAddr;
    int pageLod = QuadTreeCalcLod64(pageIndex);
    int addrTableLod = pageLod - 4;
    if (addrTableLod < 0)
    {
        if (m_PageInfoTable.Data[pageIndex] & PF_STORED)
        { // FIXME: Is it safe to read flag from async thread? Use interlocked ops?
            physAddr = m_AddressTable.ByteOffsets[pageIndex];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        int x, y;
        unsigned int relativeIndex = QuadTreeAbsoluteToRelativeIndex(pageIndex, pageLod);
        QuadTreeGetXYFromRelative(x, y, relativeIndex, pageLod);
        unsigned int addrTableIndex = QuadTreeRelativeToAbsoluteIndex(QuadTreeGetRelativeFromXY(x >> 4, y >> 4, addrTableLod), addrTableLod);
        physAddr = m_AddressTable.Table[addrTableIndex] + m_AddressTable.ByteOffsets[pageIndex];
    }
    return physAddr * m_PageSizeInBytes + m_FileHeaderSize;
}

VTFileOffset VirtualTextureFile::ReadPage(VTFileOffset physAddress, byte* pageData, int Layer) const
{
    if (m_FileHandle.IsInvalid())
    {
        return physAddress;
    }
    physAddress += m_Layers[Layer].Offset;
    if (pageData)
    {
        m_FileHandle.Read(pageData, m_Layers[Layer].SizeInBytes, physAddress);
    }
    return physAddress;
}

VTFileOffset VirtualTextureFile::ReadPage(VTFileOffset physAddress, byte* pageData[]) const
{
    if (m_FileHandle.IsInvalid())
    {
        return physAddress;
    }
    for (int Layer = 0; Layer < m_Layers.Size(); Layer++)
    {
        if (pageData[Layer])
        {
            m_FileHandle.Read(pageData[Layer], m_Layers[Layer].SizeInBytes, physAddress);
        }
        physAddress += m_Layers[Layer].SizeInBytes;
    }
    return physAddress;
}

#if 0
void VirtualTextureFile::ReadPageEx( VTFileOffset physAddress, byte * pageData[], int Lod, EVirtualTexturePageDebug Debug ) const
{
    switch ( Debug ) {
    case VT_PAGE_SHOW_LODS1:
    {
        byte color = 255 - (byte)floor( Lod * 255.f / m_AddressTable.NumLods );
        for ( int Layer = 0 ; Layer < m_Layers.Size() ; Layer++ ) {
            if ( Layer == 0 && pageData[Layer] ) {
                FillPage( pageData[Layer], ((physAddress << 4) & 255), color, (((physAddress << 4)+128) & 255), m_PageResolutionB );
                SetPageBorder( pageData[Layer], 255, 255, 0, m_PageResolutionB );
            }
            if ( Layer > 0 && pageData[Layer] ) {
                FillPage( pageData[Layer], 0, 0, 0, m_PageResolutionB );
            }
        }
        break;
    }
    case VT_PAGE_SHOW_LODS2:
    {
        byte color = (byte)floor( Lod * 255.f / m_AddressTable.NumLods );
        for ( int Layer = 0 ; Layer < m_Layers.Size() ; Layer++ ) {
            if ( Layer == 0 && pageData[Layer] ) {
                FillPage( pageData[Layer], color, color, color, m_PageResolutionB );
                SetPageBorder( pageData[Layer], 255-color, 0, 0, m_PageResolutionB );
            }
            if ( Layer > 0 && pageData[Layer] ) {
                FillPage( pageData[Layer], 0, 0, 0, m_PageResolutionB );
            }
        }
        break;
    }
    case VT_PAGE_SHOW_BORDERS:
    {
        if ( m_FileHandle.IsInvalid() ) {
            for ( int Layer = 0 ; Layer < m_Layers.Size() ; Layer++ ) {
                if ( Layer == 0 && pageData[Layer] ) {
                    FillPage( pageData[Layer], 255, 0, 0, m_PageResolutionB );
                }
                if ( Layer > 0 && pageData[Layer] ) {
                    FillPage( pageData[Layer], 0, 0, 0, m_PageResolutionB );
                }
            }
        } else {
            ReadPage( physAddress, pageData );
        }
        if ( pageData[0] ) {
            SetPageBorder( pageData[0], 255, 255, 0, m_PageResolutionB );
        }
        break;
    }
    case VT_PAGE_SHOW_RANDOM_COLORS:
    {
        for ( int Layer = 0 ; Layer < m_Layers.Size() ; Layer++ ) {
            if ( Layer == 0 && pageData[Layer] ) {
                FillRandomColor( pageData[Layer], m_PageResolutionB );
            }
            if ( Layer > 0 && pageData[Layer] ) {
                FillPage( pageData[Layer], 0, 0, 0, m_PageResolutionB );
            }
        }
        break;
    }
    case VT_PAGE_BLEND_LODS:
    {
        if ( m_FileHandle.IsInvalid() ) {
            for ( int Layer = 0 ; Layer < m_Layers.Size() ; Layer++ ) {
                if ( Layer == 0 && pageData[Layer] ) {
                    FillPage( pageData[Layer], 255, 0, 0, m_PageResolutionB );
                }
                if ( Layer > 0 && pageData[Layer] ) {
                    FillPage( pageData[Layer], 0, 0, 0, m_PageResolutionB );
                }
            }
        } else {
            ReadPage( physAddress, pageData );
        }
        float blendFactor = float( Lod ) / m_AddressTable.NumLods;
        //            for ( size_t i=0 ; i<VT_PAGE_DATA_BYTE_LENGTH_B ; i+=3 ) {
        //                pageData[i]   *= blendFactor;
        //                pageData[i+1] *= blendFactor;
        //                pageData[i+2] *= blendFactor;
        //            }
        if ( pageData[0] ) {
            BlendPageColor( pageData[0], blendFactor, m_PageResolutionB );
            SetPageBorder( pageData[0], 255, 255, 255, m_PageResolutionB );
        }
        break;
    }
    default:
    {
        if ( m_FileHandle.IsInvalid() ) {
            for ( int Layer = 0 ; Layer < m_Layers.Size() ; Layer++ ) {
                if ( Layer == 0 && pageData[Layer] ) {
                    FillRandomColor( pageData[Layer], m_PageResolutionB );
                }
                if ( Layer > 0 && pageData[Layer] ) {
                    FillPage( pageData[Layer], 0, 0, 0, m_PageResolutionB );
                }
            }
        } else {
            ReadPage( physAddress, pageData );
        }

        break;
    }
    }
}
#endif

HK_NAMESPACE_END