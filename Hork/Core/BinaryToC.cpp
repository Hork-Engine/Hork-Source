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

#include "BinaryToC.h"
#include "Memory.h"
#include "Logger.h"
#include "Compress.h"
#include "IO.h"

HK_NAMESPACE_BEGIN

namespace Core
{

bool BinaryToC(StringView sourceFile, StringView destFile, StringView symName, bool encodeBase85)
{
    File source = File::sOpenRead(sourceFile);
    if (!source)
    {
        LOG("Failed to open {}\n", sourceFile);
        return false;
    }

    File dest = File::sOpenWrite(destFile);
    if (!dest)
    {
        LOG("Failed to open {}\n", destFile);
        return false;
    }

    WriteBinaryToC(dest, symName, source.AsBlob(), encodeBase85);

    return true;
}

bool BinaryToCompressedC(StringView sourceFile, StringView destFile, StringView symName, bool encodeBase85)
{
    File source = File::sOpenRead(sourceFile);
    if (!source)
    {
        LOG("Failed to open {}\n", sourceFile);
        return false;
    }

    File dest = File::sOpenWrite(destFile);
    if (!dest)
    {
        LOG("Failed to open {}\n", destFile);
        return false;
    }

    HeapBlob decompressedData = source.AsBlob();

    size_t compressedSize = Core::ZMaxCompressedSize(decompressedData.Size());
    HeapBlob compressedData(compressedSize);
    Core::ZCompress((byte*)compressedData.GetData(), &compressedSize, (const byte*)decompressedData.GetData(), decompressedData.Size(), ZLIB_COMPRESS_UBER_COMPRESSION);

    WriteBinaryToC(dest, symName, BlobRef(compressedData.GetData(), compressedSize), encodeBase85);

    return true;
}

void WriteBinaryToC(IBinaryStreamWriteInterface& stream, StringView symName, BlobRef Blob, bool encodeBase85)
{
    const byte* bytes = (const byte*)Blob.GetData();
    size_t      size  = Blob.Size();

    if (encodeBase85)
    {
        stream.FormattedPrint(HK_FMT("static const char {}_Data_Base85[{}+1] =\n    \""), symName, (int)((size + 3) / 4) * 5);
        char prev_c = 0;
        for (size_t i = 0; i < size; i += 4)
        {
            uint32_t d = *(uint32_t*)(bytes + i);
            for (int j = 0; j < 5; j++, d /= 85)
            {
                unsigned int x = (d % 85) + 35;
                char         c = (x >= '\\') ? x + 1 : x;
                if (c == '?' && prev_c == '?')
                    stream.FormattedPrint(HK_FMT("\\{}"), c);
                else
                    stream.FormattedPrint(HK_FMT("{}"), c);
                prev_c = c;
            }
            if ((i % 112) == 112 - 4)
                stream.FormattedPrint(HK_FMT("\"\n    \""));
        }
        stream.FormattedPrint(HK_FMT("\";\n\n"));
    }
    else
    {
        stream.FormattedPrint(HK_FMT("static const size_t {}_Size = {};\n"), symName, (int)size);
        stream.FormattedPrint(HK_FMT("static const uint64_t {}_Data[{}] =\n{{"), symName, Align(size, 8));
        int column = 0;
        for (size_t i = 0; i < size; i += 8)
        {
            uint64_t d = *(uint64_t*)(bytes + i);
            if ((column++ % 6) == 0)
            {
                stream.FormattedPrint(HK_FMT("\n    0x{:08x}{:08x}"), Math::INT64_HIGH_INT(d), Math::INT64_LOW_INT(d));
            }
            else
            {
                stream.FormattedPrint(HK_FMT("0x{:08x}{:08x}"), Math::INT64_HIGH_INT(d), Math::INT64_LOW_INT(d));
            }
            if (i + 8 < size)
            {
                stream.FormattedPrint(HK_FMT(", "));
            }
        }
        stream.FormattedPrint(HK_FMT("\n}};\n\n"));
    }
}

} // namespace Core

HK_NAMESPACE_END
