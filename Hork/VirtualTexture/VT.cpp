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

#include "VT.h"
#include "QuadTree.h"

#include <Hork/Core/BaseMath.h>

#include <fcntl.h>

#ifdef HK_OS_WIN32

#    include <Hork/Core/WindowsDefs.h>

#    include <io.h>

HK_NAMESPACE_BEGIN

bool VTFileHandle::OpenRead(StringView fileName)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, fileName.ToPtr(), fileName.Size(), NULL, 0);
    if (0 == n)
    {
        return false;
    }

    wchar_t* wFilename = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, fileName.ToPtr(), fileName.Size(), wFilename, n);

    Handle = CreateFileW(wFilename,
                         GENERIC_READ,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                         NULL);

    return Handle != INVALID_HANDLE_VALUE;
}

bool VTFileHandle::OpenWrite(StringView fileName)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, fileName.ToPtr(), fileName.Size(), NULL, 0);
    if (0 == n)
    {
        return false;
    }

    wchar_t* wFilename = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, fileName.ToPtr(), fileName.Size(), wFilename, n);

    Handle = CreateFileW(wFilename,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                         NULL);

    return Handle != INVALID_HANDLE_VALUE;
}

void VTFileHandle::Seek(uint64_t _offset)
{
    // FILE_BEGIN - seek set
    // FILE_CURRENT - seek cur
    // FILE_END - seek end
    DWORD whence = FILE_BEGIN;

    LARGE_INTEGER offset;
    offset.QuadPart = _offset;

    if (!SetFilePointerEx(Handle, offset, &offset, whence))
    {
        // error
        HK_ASSERT(0);
    }
}

void VTFileHandle::Close()
{
    if (Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Handle);
        Handle = INVALID_HANDLE_VALUE;
    }
}

void VTFileHandle::Read(void* data, unsigned int size, uint64_t offset)
{
    DWORD numberOfBytesRead;

    Seek(offset);

    BOOL r = ReadFile(
        Handle,
        data,
        size,
        &numberOfBytesRead,
        NULL);

    HK_ASSERT(r != FALSE);
    HK_ASSERT(numberOfBytesRead == size);
    HK_UNUSED(r);
}

void VTFileHandle::Write(const void* data, unsigned int size, uint64_t offset)
{
    DWORD numberOfBytesWritten;

    Seek(offset);

    BOOL r = WriteFile(
        Handle,
        data,
        size,
        &numberOfBytesWritten,
        NULL);

    HK_ASSERT(r != FALSE);
    HK_ASSERT(numberOfBytesWritten == size);
    HK_UNUSED(r);
}

HK_NAMESPACE_END

#else

#    include <unistd.h>

HK_NAMESPACE_BEGIN

bool VTFileHandle::OpenRead(StringView fileName)
{
    iHandle = open(fileName.IsNullTerminated() ? fileName.Begin() : String(fileName).CStr(), O_LARGEFILE | /*O_BINARY |*/ O_RDONLY);
    return iHandle >= 0;
}

bool VTFileHandle::OpenWrite(StringView fileName)
{
    iHandle = open(fileName.IsNullTerminated() ? fileName.Begin() : String(fileName).CStr(), O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC, 0664);
    return iHandle >= 0;
}

void VTFileHandle::Close()
{
    if (iHandle >= 0)
    {
        close(iHandle);
        iHandle = -1;
    }
}

void VTFileHandle::Read(void* data, unsigned int size, uint64_t offset)
{
    auto r = pread64(iHandle, data, size, offset);
    HK_UNUSED(r);
}

void VTFileHandle::Write(const void* data, unsigned int size, uint64_t offset)
{
    auto r = pwrite64(iHandle, data, size, offset);
    HK_UNUSED(r);
}

HK_NAMESPACE_END

#endif

HK_NAMESPACE_BEGIN

VirtualTexturePIT::VirtualTexturePIT()
{
    Data = NULL;
    NumPages = 0;
    WritePages = 0;
}

VirtualTexturePIT::~VirtualTexturePIT()
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(Data);
}

void VirtualTexturePIT::Create(unsigned int numPages)
{
    HK_ASSERT_(numPages > 0, "VirtualTexturePIT::create");
    NumPages = numPages;
    Core::GetHeapAllocator<HEAP_MISC>().Free(Data);
    Data = (byte*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(NumPages);
    WritePages = NumPages;
}

void VirtualTexturePIT::Clear()
{
    HK_ASSERT_(Data != NULL, "VirtualTexturePIT::clear");
    Core::ZeroMem(Data, sizeof(Data[0]) * NumPages);
}

void VirtualTexturePIT::Generate(VTPageBitfield const& bitfield, int& outStoredLods)
{
    HK_ASSERT_(Data != NULL, "VirtualTexturePIT::generate");
    HK_ASSERT_(bitfield.Size() >= NumPages, "VirtualTexturePIT::Generate");

    unsigned int lodPagesCount[QUADTREE_MAX_LODS_32];

    int numLods = QuadTreeCalcLod64(NumPages);

    Core::ZeroMem(lodPagesCount, sizeof(lodPagesCount[0]) * numLods);

    // Parse bits
    for (unsigned int i = 0; i < NumPages; i++)
    {
        if (bitfield.IsMarked(i))
        {
            Data[i] = PF_STORED;
            lodPagesCount[QuadTreeCalcLod64(i)]++;
        }
        else
        {
            Data[i] = 0;
        }
    }

    // Calc stored lods
    int storedLods;
    for (storedLods = numLods - 1; storedLods >= 0; storedLods--)
    {
        if (lodPagesCount[storedLods] > 0)
        {
            break;
        }
    }
    outStoredLods = storedLods + 1;

    WritePages = QuadTreeCalcQuadTreeNodes(outStoredLods);

    // Generate PIT
    unsigned int absoluteIndex = 0;
    int maxLod;
    for (int lod = 0; lod < outStoredLods; lod++)
    {
        unsigned int numPages = QuadTreeCalcLodNodes(lod);
        for (unsigned int page = 0; page < numPages; page++)
        {
            // Определить максимальный lod для страницы
            unsigned int pageIndex = absoluteIndex;
            maxLod = lod;
            while (!(Data[pageIndex] & PF_STORED) && pageIndex > 0)
            {
                int relativeIndex = QuadTreeAbsoluteToRelativeIndex(pageIndex, maxLod);

                // Получить парента и сохранить в pageIndex
                QuadTreeGetRelation(relativeIndex, maxLod, pageIndex);

                maxLod--;
            }
            Data[absoluteIndex] |= maxLod << 4;
            absoluteIndex++;
        }
    }
}

VTFileOffset VirtualTexturePIT::Write(VTFileHandle* file, VTFileOffset offset) const
{
    HK_ASSERT_(Data != NULL, "VirtualTexturePIT::write");
    file->Write(&WritePages, sizeof(WritePages), offset);
    offset += sizeof(WritePages);
    file->Write(Data, sizeof(Data[0]) * WritePages, offset);
    offset += sizeof(Data[0]) * WritePages;
    return offset;
}

VTFileOffset VirtualTexturePIT::Read(VTFileHandle* file, VTFileOffset offset)
{
    unsigned int tmp;
    file->Read(&tmp, sizeof(tmp), offset);
    offset += sizeof(tmp);

    Create(tmp);

    file->Read(Data, sizeof(Data[0]) * tmp, offset);
    offset += sizeof(Data[0]) * tmp;
    return offset;
}


VirtualTextureAddressTable::VirtualTextureAddressTable()
{
    ByteOffsets = NULL;
    Table = NULL;
    TableSize = 0;
    TotalPages = 0;
    NumLods = 0;
}

VirtualTextureAddressTable::~VirtualTextureAddressTable()
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(ByteOffsets);
    Core::GetHeapAllocator<HEAP_MISC>().Free(Table);
}

void VirtualTextureAddressTable::Create(int numLods)
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(ByteOffsets);
    Core::GetHeapAllocator<HEAP_MISC>().Free(Table);

    NumLods = numLods;
    TotalPages = QuadTreeCalcQuadTreeNodes(numLods);
    TableSize = numLods > 4 ? QuadTreeCalcQuadTreeNodes(numLods - 4) : 0;
    ByteOffsets = (byte*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(TotalPages);
    if (TableSize > 0)
    {
        Table = (uint32_t*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(Table[0]) * TableSize);
    }
    else
    {
        Table = NULL;
    }
}

void VirtualTextureAddressTable::Clear()
{
    HK_ASSERT_(ByteOffsets != NULL, "VirtualTextureAddressTable::clear");

    Core::ZeroMem(ByteOffsets, sizeof(ByteOffsets[0]) * TotalPages);
    if (Table)
    {
        Core::ZeroMem(Table, sizeof(Table[0]) * TableSize);
    }
}

void VirtualTextureAddressTable::Generate(VTPageBitfield const& bitfield)
{
    HK_ASSERT_(ByteOffsets != NULL, "VirtualTextureAddressTable::generate");
    HK_ASSERT_(bitfield.Size() >= TotalPages, "VirtualTexturePIT::Generate");

    // Кол-во страниц в LOD'ах от 0 до 4
    unsigned int numFirstPages = Math::Min<unsigned int>(85, TotalPages);

    // Кол-во посчитанных страниц
    unsigned int numWrittenPages = 0;

    // Заполняем byte offsets для первых четырех LOD'ов
    for (unsigned int i = 0; i < numFirstPages; i++)
    {
        if (bitfield.IsMarked(i))
        {
            ByteOffsets[i] = numWrittenPages;
            numWrittenPages++;
        }
    }

    if (TableSize)
    {
        // Заполняем byte offsets для LOD'ов > 4

        for (int lodNum = 4; lodNum < NumLods; lodNum++)
        {
            int addrTableLod = lodNum - 4;
            unsigned int numNodes = 1 << (addrTableLod + addrTableLod);

            for (unsigned int node = 0; node < numNodes; node++)
            {
                int byteOfs = 0;
                int nodeX, nodeY;
                unsigned int addrTableAbs = QuadTreeRelativeToAbsoluteIndex(node, addrTableLod);

                Table[addrTableAbs] = numWrittenPages;

                nodeX = QuadTreeGetXFromRelative(node, addrTableLod);
                nodeY = QuadTreeGetYFromRelative(node, addrTableLod);
                nodeX <<= 4;
                nodeY <<= 4;

                for (int i = 0; i < 256; i++)
                {
                    unsigned int relativeIndex = QuadTreeGetRelativeFromXY(nodeX + (i & 15), nodeY + (i >> 4), lodNum);
                    unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lodNum);

                    ByteOffsets[absoluteIndex] = byteOfs;

                    if (bitfield.IsMarked(absoluteIndex))
                    {
                        byteOfs++;
                    }
                }
                numWrittenPages += byteOfs;
            }
        }

#if 0
        int* addrTableByteOffsets = new int[AddrTableSize];

        Core::ZeroMem(addrTableByteOffsets, sizeof(int) * AddrTableSize);

        for (unsigned int i = 85; i < TotalPages; i++)
        {
            //Заполняем byte offsets для LOD'ов > 4
            if (bitfield.IsMarked(i))
            {
                unsigned int absoluteIndex;
                unsigned int relativeIndex;

                int pageLod = QuadTreeCalcLod64(i);

                relativeIndex = QuadTreeAbsoluteToRelativeIndex(i, pageLod);

                int pageX = QuadTreeGetXFromRelative(relativeIndex, pageLod);
                int pageY = QuadTreeGetYFromRelative(relativeIndex, pageLod);

                pageX >>= 4;
                pageY >>= 4;

                int lod = pageLod - 4;

                relativeIndex = QuadTreeGetRelativeFromXY(pageX, pageY, lod);
                absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lod);

                ByteOffsets[i] = addrTableByteOffsets[absoluteIndex];
                addrTableByteOffsets[absoluteIndex]++;
            }
        }

        AddrTable[0] = numWrittenPages;
        for (unsigned int i = 1; i < AddrTableSize; i++)
        {
            AddrTable[i] = AddrTable[i - 1] + addrTableByteOffsets[i - 1];
        }

        delete[] addrTableByteOffsets;

#endif
    }
}

VTFileOffset VirtualTextureAddressTable::Write(VTFileHandle* file, VTFileOffset offset) const
{
    HK_ASSERT_(ByteOffsets != NULL, "VirtualTextureAddressTable::write");

    byte tmp = NumLods;
    file->Write(&tmp, sizeof(tmp), offset);
    offset += sizeof(tmp);

    file->Write(ByteOffsets, sizeof(ByteOffsets[0]) * TotalPages, offset);
    offset += sizeof(ByteOffsets[0]) * TotalPages;

    if (Table)
    {
        file->Write(Table, sizeof(Table[0]) * TableSize, offset);
        offset += sizeof(Table[0]) * TableSize;
    }
    return offset;
}

VTFileOffset VirtualTextureAddressTable::Read(VTFileHandle* file, VTFileOffset offset)
{
    byte tmp;
    file->Read(&tmp, sizeof(tmp), offset);
    offset += sizeof(tmp);

    Create(tmp);

    file->Read(ByteOffsets, sizeof(ByteOffsets[0]) * TotalPages, offset);
    offset += sizeof(ByteOffsets[0]) * TotalPages;

    if (Table)
    {
        file->Read(Table, sizeof(Table[0]) * TableSize, offset);
        offset += sizeof(Table[0]) * TableSize;
    }
    return offset;
}

HK_NAMESPACE_END
