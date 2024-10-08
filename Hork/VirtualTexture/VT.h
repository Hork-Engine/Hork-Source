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

#pragma once

#include <Hork/Core/IO.h>
#include <Hork/Core/Containers/BitMask.h>

HK_NAMESPACE_BEGIN

inline constexpr short VT_FILE_VERSION = 5;
inline constexpr uint32_t VT_FILE_ID = 'V' | ('T' << 8) | (VT_FILE_VERSION << 16);
inline constexpr int VT_PAGE_BORDER_WIDTH = 4;
inline constexpr int VT_MAX_LODS = 13;
inline constexpr int VT_MAX_LAYERS = 8;

typedef size_t VTFileOffset;

using VTPageBitfield = BitMask<>;

enum VT_PAGE_FLAGS_4BIT
{
    /// Page in cache
    PF_CACHED = 1,

    /// Page pending to load from hard drive. Used during feedback analyzing
    PF_PENDING = 2, //Unused

    /// Page queued for loading from hard drive
    PF_QUEUED = 4, //Unused

    /// Page exist on hard drive
    PF_STORED = 8 // FIXME: PF_STORED is used only in constructor, therefore we can remove it and free one bit for other needs
};

struct VTFileHandle final
{
    union
    {
        void* Handle;
        int iHandle;
    };

    VTFileHandle() :
        Handle((void*)(size_t)-1)
    {
    }

    ~VTFileHandle()
    {
        Close();
    }

    bool IsInvalid() const { return Handle == (void*)(size_t)-1; }

    bool OpenRead(StringView fileName);
    bool OpenWrite(StringView fileName);
    void Close();
    void Seek(uint64_t offset);
    void Read(void* data, unsigned int size, uint64_t offset);
    void Write(const void* data, unsigned int size, uint64_t offset);
};

struct VirtualTexturePIT final
{
    VirtualTexturePIT();
    ~VirtualTexturePIT();

    void Create(unsigned int numPages);
    void Clear();
    void Generate(VTPageBitfield const& pageBitfield, int& storedLods);
    VTFileOffset Write(VTFileHandle* file, VTFileOffset offset) const;
    VTFileOffset Read(VTFileHandle* file, VTFileOffset offset);

    /// Page info table
    /// [xxxxyyyy]
    /// xxxx - max LOD
    /// yyyy - PageFlags4bit
    byte* Data;

    unsigned int NumPages;   // total size of data[]
    unsigned int WritePages; // actual size of data[]
};

struct VirtualTextureAddressTable final
{
    VirtualTextureAddressTable();
    ~VirtualTextureAddressTable();

    void Create(int numLods);
    void Clear();
    void Generate(VTPageBitfield const& pageBitfield);

    VTFileOffset Write(VTFileHandle* file, VTFileOffset offset) const;
    VTFileOffset Read(VTFileHandle* file, VTFileOffset offset);

    // Offsets relative to value form AddrTable (in pages)
    byte* ByteOffsets;

    // Address table (Quad tree, values in pages)
    unsigned int TableSize;
    uint32_t* Table;

    unsigned int TotalPages;
    int NumLods;
};

HK_NAMESPACE_END
