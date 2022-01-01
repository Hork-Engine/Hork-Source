/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "IO.h"

struct AGUID
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

    bool operator==(AGUID const& _Other) const
    {
        return Hi == _Other.Hi && Lo == _Other.Lo;
    }

    bool operator!=(AGUID const& _Other) const
    {
        return Hi != _Other.Hi || Lo != _Other.Lo;
    }

    // String conversions
    AString ToString() const
    {
        TSprintfBuffer<37> buffer;
        return buffer.Sprintf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                              HiBytes[0], HiBytes[1], HiBytes[2], HiBytes[3],
                              HiBytes[4], HiBytes[5],
                              HiBytes[6], HiBytes[7],
                              LoBytes[0], LoBytes[1],
                              LoBytes[2], LoBytes[3], LoBytes[4], LoBytes[5], LoBytes[6], LoBytes[7]);
    }

    const char* CStr() const
    {
        return Core::Fmt("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                         HiBytes[0], HiBytes[1], HiBytes[2], HiBytes[3],
                         HiBytes[4], HiBytes[5],
                         HiBytes[6], HiBytes[7],
                         LoBytes[0], LoBytes[1],
                         LoBytes[2], LoBytes[3], LoBytes[4], LoBytes[5], LoBytes[6], LoBytes[7]);
    }

    AGUID& FromString(AString const& _String)
    {
        return FromString(_String.CStr());
    }

    AGUID& FromString(const char* _String);

    const byte* GetBytes() const { return &HiBytes[0]; }
    byte*       GetBytes() { return &HiBytes[0]; }

    // Byte serialization
    void Write(IBinaryStream& _Stream) const
    {
        _Stream.WriteUInt64(Hi);
        _Stream.WriteUInt64(Lo);
    }

    void Read(IBinaryStream& _Stream)
    {
        Hi = _Stream.ReadUInt64();
        Lo = _Stream.ReadUInt64();
    }
};
