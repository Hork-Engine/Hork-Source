/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Platform/Memory/Memory.h>
#include <Platform/Logger.h>

#include <Core/BinaryToC.h>
#include <Core/Compress.h>
#include <Core/BaseMath.h>

namespace Core
{

bool BinaryToC(AStringView SourceFile, AStringView DestFile, AStringView SymName, bool bEncodeBase85)
{
    AFileStream source, dest;

    if (!source.OpenRead(SourceFile))
    {
        LOG("Failed to open {}\n", SourceFile);
        return false;
    }

    if (!dest.OpenWrite(DestFile))
    {
        LOG("Failed to open {}\n", DestFile);
        return false;
    }

    size_t size = source.SizeInBytes();
    byte*  data = (byte*)Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(size);
    source.Read(data, size);

    WriteBinaryToC(dest, SymName, data, size, bEncodeBase85);

    Platform::GetHeapAllocator<HEAP_TEMP>().Free(data);

    return true;
}

bool BinaryToCompressedC(AStringView SourceFile, AStringView DestFile, AStringView SymName, bool bEncodeBase85)
{
    AFileStream source, dest;

    if (!source.OpenRead(SourceFile))
    {
        LOG("Failed to open {}\n", SourceFile);
        return false;
    }

    if (!dest.OpenWrite(DestFile))
    {
        LOG("Failed to open {}\n", DestFile);
        return false;
    }

    size_t decompressedSize = source.SizeInBytes();
    byte*  decompressedData = (byte*)Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(decompressedSize);
    source.Read(decompressedData, decompressedSize);

    size_t compressedSize = Core::ZMaxCompressedSize(decompressedSize);
    byte*  compressedData = (byte*)Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(compressedSize);
    Core::ZCompress(compressedData, &compressedSize, (byte*)decompressedData, decompressedSize, ZLIB_COMPRESS_UBER_COMPRESSION);

    WriteBinaryToC(dest, SymName, compressedData, compressedSize, bEncodeBase85);

    Platform::GetHeapAllocator<HEAP_TEMP>().Free(compressedData);
    Platform::GetHeapAllocator<HEAP_TEMP>().Free(decompressedData);

    return true;
}

void WriteBinaryToC(IBinaryStreamWriteInterface& Stream, AStringView SymName, const void* pData, size_t SizeInBytes, bool bEncodeBase85)
{
    const byte* bytes = (const byte*)pData;

    if (bEncodeBase85)
    {
        Stream.FormattedPrint(HK_FMT("static const char {}_Data_Base85[{}+1] =\n    \""), SymName, (int)((SizeInBytes + 3) / 4) * 5);
        char prev_c = 0;
        for (size_t i = 0; i < SizeInBytes; i += 4)
        {
            uint32_t d = *(uint32_t*)(bytes + i);
            for (int j = 0; j < 5; j++, d /= 85)
            {
                unsigned int x = (d % 85) + 35;
                char         c = (x >= '\\') ? x + 1 : x;
                if (c == '?' && prev_c == '?')
                    Stream.FormattedPrint(HK_FMT("\\{}"), c);
                else
                    Stream.FormattedPrint(HK_FMT("{}"), c);
                prev_c = c;
            }
            if ((i % 112) == 112 - 4)
                Stream.FormattedPrint(HK_FMT("\"\n    \""));
        }
        Stream.FormattedPrint(HK_FMT("\";\n\n"));
    }
    else
    {
        Stream.FormattedPrint(HK_FMT("static const size_t {}_Size = {};\n"), SymName, (int)SizeInBytes);
        Stream.FormattedPrint(HK_FMT("static const uint64_t {}_Data[{}] =\n{{"), SymName, Align(SizeInBytes, 8));
        int column = 0;
        for (size_t i = 0; i < SizeInBytes; i += 8)
        {
            uint64_t d = *(uint64_t*)(bytes + i);
            if ((column++ % 6) == 0)
            {
                Stream.FormattedPrint(HK_FMT("\n    0x{:08x}{:08x}"), Math::INT64_HIGH_INT(d), Math::INT64_LOW_INT(d));
            }
            else
            {
                Stream.FormattedPrint(HK_FMT("0x{:08x}{:08x}"), Math::INT64_HIGH_INT(d), Math::INT64_LOW_INT(d));
            }
            if (i + 8 < SizeInBytes)
            {
                Stream.FormattedPrint(HK_FMT(", "));
            }
        }
        Stream.FormattedPrint(HK_FMT("\n}};\n\n"));
    }
}

} // namespace Core
