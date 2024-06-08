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

#include "IO.h"
#include "HashFunc.h"

HK_NAMESPACE_BEGIN

struct GUID
{
    union
    {
        uint64_t Hi;
        byte     HiBytes[8];
    };
    union
    {
        uint64_t Lo;
        byte     LoBytes[8];
    };

    void Clear()
    {
        Hi = Lo = 0;
    }

    void Generate();

    bool operator==(GUID const& Rhs) const
    {
        return Hi == Rhs.Hi && Lo == Rhs.Lo;
    }

    bool operator!=(GUID const& Rhs) const
    {
        return Hi != Rhs.Hi || Lo != Rhs.Lo;
    }

    // String conversions
    String ToString() const
    {
        TSprintfBuffer<37> buffer;
        return buffer.Sprintf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                              HiBytes[0], HiBytes[1], HiBytes[2], HiBytes[3],
                              HiBytes[4], HiBytes[5],
                              HiBytes[6], HiBytes[7],
                              LoBytes[0], LoBytes[1],
                              LoBytes[2], LoBytes[3], LoBytes[4], LoBytes[5], LoBytes[6], LoBytes[7]);
    }

    GUID& FromString(StringView String);

    const byte* GetBytes() const { return &HiBytes[0]; }
    byte*       GetBytes() { return &HiBytes[0]; }

    uint32_t Hash() const
    {
        return HashTraits::Murmur3Hash64(Hi, HashTraits::Murmur3Hash64(Lo));
    }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.WriteUInt64(Hi);
        Stream.WriteUInt64(Lo);
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        Hi = Stream.ReadUInt64();
        Lo = Stream.ReadUInt64();
    }
};

HK_NAMESPACE_END
