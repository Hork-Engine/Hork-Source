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

VirtualTextureFile::VirtualTextureFile(const char* FileName)
{
    SFileOffset fileOffset;
    uint32_t version;
    byte tmp;

    FileHeaderSize = 0;
    TextureResolution = 0;
    TextureResolutionLog2 = 0;

    if (!FileHandle.OpenRead(FileName))
    {
        LOG("VirtualTextureFile::ctor: couldn't open {}\n", FileName);
        return;
    }

    fileOffset = 0;

    // read version
    FileHandle.Read(&version, sizeof(version), fileOffset);
    fileOffset += sizeof(version);

    if (version != VT_FILE_ID)
    {
        FileHandle.Close();
        return;
    }

    // read num Layers
    FileHandle.Read(&tmp, sizeof(byte), fileOffset);
    fileOffset += sizeof(byte);

    Layers.Resize(tmp);
    int LayerOffset = 0;
    for (int i = 0; i < Layers.Size(); i++)
    {

        FileHandle.Read(&Layers[i].SizeInBytes, sizeof(Layers[i].SizeInBytes), fileOffset);
        fileOffset += sizeof(Layers[i].SizeInBytes);

        FileHandle.Read(&Layers[i].PageDataFormat, sizeof(Layers[i].PageDataFormat), fileOffset);
        fileOffset += sizeof(Layers[i].PageDataFormat);

        //FileHandle.Read( &Layers[i].NumChannels, sizeof( Layers[i].NumChannels ), fileOffset );
        //fileOffset += sizeof( Layers[i].NumChannels );

        Layers[i].Offset = LayerOffset;

        LayerOffset += Layers[i].SizeInBytes;
    }
    PageSizeInBytes = LayerOffset;

    // read page width
    FileHandle.Read(&PageResolutionB, sizeof(PageResolutionB), fileOffset);
    fileOffset += sizeof(PageResolutionB);

    // read page info table
    fileOffset += PageInfoTable.Read(&FileHandle, fileOffset);

    // read page address tables
    fileOffset += AddressTable.Read(&FileHandle, fileOffset);

    FileHeaderSize = fileOffset;

    TextureResolution = (1u << (AddressTable.NumLods - 1)) * PageResolutionB;
    TextureResolutionLog2 = Math::Log2(TextureResolution);
}

VirtualTextureFile::~VirtualTextureFile()
{
}

SFileOffset VirtualTextureFile::GetPhysAddress(unsigned int _PageIndex) const
{
    SFileOffset physAddr;
    int pageLod = QuadTreeCalcLod64(_PageIndex);
    int addrTableLod = pageLod - 4;
    if (addrTableLod < 0)
    {
        if (PageInfoTable.Data[_PageIndex] & PF_STORED)
        { // FIXME: Is it safe to read flag from async thread? Use interlocked ops?
            physAddr = AddressTable.ByteOffsets[_PageIndex];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        int x, y;
        unsigned int relativeIndex = QuadTreeAbsoluteToRelativeIndex(_PageIndex, pageLod);
        QuadTreeGetXYFromRelative(x, y, relativeIndex, pageLod);
        unsigned int addrTableIndex = QuadTreeRelativeToAbsoluteIndex(QuadTreeGetRelativeFromXY(x >> 4, y >> 4, addrTableLod), addrTableLod);
        physAddr = AddressTable.Table[addrTableIndex] + AddressTable.ByteOffsets[_PageIndex];
    }
    return physAddr * PageSizeInBytes + FileHeaderSize;
}

SFileOffset VirtualTextureFile::ReadPage(SFileOffset PhysAddress, byte* PageData, int Layer) const
{
    if (FileHandle.IsInvalid())
    {
        return PhysAddress;
    }
    PhysAddress += Layers[Layer].Offset;
    if (PageData)
    {
        FileHandle.Read(PageData, Layers[Layer].SizeInBytes, PhysAddress);
    }
    return PhysAddress;
}

SFileOffset VirtualTextureFile::ReadPage(SFileOffset PhysAddress, byte* PageData[]) const
{
    if (FileHandle.IsInvalid())
    {
        return PhysAddress;
    }
    for (int Layer = 0; Layer < Layers.Size(); Layer++)
    {
        if (PageData[Layer])
        {
            FileHandle.Read(PageData[Layer], Layers[Layer].SizeInBytes, PhysAddress);
        }
        PhysAddress += Layers[Layer].SizeInBytes;
    }
    return PhysAddress;
}

#if 0
void VirtualTextureFile::ReadPageEx( SFileOffset PhysAddress, byte * PageData[], int Lod, EVirtualTexturePageDebug Debug ) const
{
    switch ( Debug ) {
    case VT_PAGE_SHOW_LODS1:
    {
        byte color = 255 - (byte)floor( Lod * 255.f / AddressTable.NumLods );
        for ( int Layer = 0 ; Layer < Layers.Size() ; Layer++ ) {
            if ( Layer == 0 && PageData[Layer] ) {
                FillPage( PageData[Layer], ((PhysAddress << 4) & 255), color, (((PhysAddress << 4)+128) & 255), PageResolutionB );
                SetPageBorder( PageData[Layer], 255, 255, 0, PageResolutionB );
            }
            if ( Layer > 0 && PageData[Layer] ) {
                FillPage( PageData[Layer], 0, 0, 0, PageResolutionB );
            }
        }
        break;
    }
    case VT_PAGE_SHOW_LODS2:
    {
        byte color = (byte)floor( Lod * 255.f / AddressTable.NumLods );
        for ( int Layer = 0 ; Layer < Layers.Size() ; Layer++ ) {
            if ( Layer == 0 && PageData[Layer] ) {
                FillPage( PageData[Layer], color, color, color, PageResolutionB );
                SetPageBorder( PageData[Layer], 255-color, 0, 0, PageResolutionB );
            }
            if ( Layer > 0 && PageData[Layer] ) {
                FillPage( PageData[Layer], 0, 0, 0, PageResolutionB );
            }
        }
        break;
    }
    case VT_PAGE_SHOW_BORDERS:
    {
        if ( FileHandle.IsInvalid() ) {
            for ( int Layer = 0 ; Layer < Layers.Size() ; Layer++ ) {
                if ( Layer == 0 && PageData[Layer] ) {
                    FillPage( PageData[Layer], 255, 0, 0, PageResolutionB );
                }
                if ( Layer > 0 && PageData[Layer] ) {
                    FillPage( PageData[Layer], 0, 0, 0, PageResolutionB );
                }
            }
        } else {
            ReadPage( PhysAddress, PageData );
        }
        if ( PageData[0] ) {
            SetPageBorder( PageData[0], 255, 255, 0, PageResolutionB );
        }
        break;
    }
    case VT_PAGE_SHOW_RANDOM_COLORS:
    {
        for ( int Layer = 0 ; Layer < Layers.Size() ; Layer++ ) {
            if ( Layer == 0 && PageData[Layer] ) {
                FillRandomColor( PageData[Layer], PageResolutionB );
            }
            if ( Layer > 0 && PageData[Layer] ) {
                FillPage( PageData[Layer], 0, 0, 0, PageResolutionB );
            }
        }
        break;
    }
    case VT_PAGE_BLEND_LODS:
    {
        if ( FileHandle.IsInvalid() ) {
            for ( int Layer = 0 ; Layer < Layers.Size() ; Layer++ ) {
                if ( Layer == 0 && PageData[Layer] ) {
                    FillPage( PageData[Layer], 255, 0, 0, PageResolutionB );
                }
                if ( Layer > 0 && PageData[Layer] ) {
                    FillPage( PageData[Layer], 0, 0, 0, PageResolutionB );
                }
            }
        } else {
            ReadPage( PhysAddress, PageData );
        }
        float blendFactor = float( Lod ) / AddressTable.NumLods;
        //            for ( size_t i=0 ; i<VT_PAGE_DATA_BYTE_LENGTH_B ; i+=3 ) {
        //                PageData[i]   *= blendFactor;
        //                PageData[i+1] *= blendFactor;
        //                PageData[i+2] *= blendFactor;
        //            }
        if ( PageData[0] ) {
            BlendPageColor( PageData[0], blendFactor, PageResolutionB );
            SetPageBorder( PageData[0], 255, 255, 255, PageResolutionB );
        }
        break;
    }
    default:
    {
        if ( FileHandle.IsInvalid() ) {
            for ( int Layer = 0 ; Layer < Layers.Size() ; Layer++ ) {
                if ( Layer == 0 && PageData[Layer] ) {
                    FillRandomColor( PageData[Layer], PageResolutionB );
                }
                if ( Layer > 0 && PageData[Layer] ) {
                    FillPage( PageData[Layer], 0, 0, 0, PageResolutionB );
                }
            }
        } else {
            ReadPage( PhysAddress, PageData );
        }

        break;
    }
    }
}
#endif

HK_NAMESPACE_END