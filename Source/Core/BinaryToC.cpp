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
#include <Core/IO.h>
#include <Core/HeapBlob.h>

HK_NAMESPACE_BEGIN

namespace Core
{

bool BinaryToC(StringView SourceFile, StringView DestFile, StringView SymName, bool bEncodeBase85)
{
    File source = File::OpenRead(SourceFile);
    if (!source)
    {
        LOG("Failed to open {}\n", SourceFile);
        return false;
    }

    File dest = File::OpenWrite(DestFile);
    if (!dest)
    {
        LOG("Failed to open {}\n", DestFile);
        return false;
    }

    WriteBinaryToC(dest, SymName, source.AsBlob(), bEncodeBase85);

    return true;
}

bool BinaryToCompressedC(StringView SourceFile, StringView DestFile, StringView SymName, bool bEncodeBase85)
{
    File source = File::OpenRead(SourceFile);
    if (!source)
    {
        LOG("Failed to open {}\n", SourceFile);
        return false;
    }

    File dest = File::OpenWrite(DestFile);
    if (!dest)
    {
        LOG("Failed to open {}\n", DestFile);
        return false;
    }

    HeapBlob decompressedData = source.AsBlob();

    size_t compressedSize = Core::ZMaxCompressedSize(decompressedData.Size());
    HeapBlob compressedData(compressedSize);
    Core::ZCompress((byte*)compressedData.GetData(), &compressedSize, (const byte*)decompressedData.GetData(), decompressedData.Size(), ZLIB_COMPRESS_UBER_COMPRESSION);

    WriteBinaryToC(dest, SymName, BlobRef(compressedData.GetData(), compressedSize), bEncodeBase85);

    return true;
}

void WriteBinaryToC(IBinaryStreamWriteInterface& Stream, StringView SymName, BlobRef Blob, bool bEncodeBase85)
{
    const byte* bytes = (const byte*)Blob.GetData();
    size_t      size  = Blob.Size();

    if (bEncodeBase85)
    {
        Stream.FormattedPrint(HK_FMT("static const char {}_Data_Base85[{}+1] =\n    \""), SymName, (int)((size + 3) / 4) * 5);
        char prev_c = 0;
        for (size_t i = 0; i < size; i += 4)
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
        Stream.FormattedPrint(HK_FMT("static const size_t {}_Size = {};\n"), SymName, (int)size);
        Stream.FormattedPrint(HK_FMT("static const uint64_t {}_Data[{}] =\n{{"), SymName, Align(size, 8));
        int column = 0;
        for (size_t i = 0; i < size; i += 8)
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
            if (i + 8 < size)
            {
                Stream.FormattedPrint(HK_FMT(", "));
            }
        }
        Stream.FormattedPrint(HK_FMT("\n}};\n\n"));
    }
}

} // namespace Core

HK_NAMESPACE_END
