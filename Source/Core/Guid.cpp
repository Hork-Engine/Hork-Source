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

#include <Core/Guid.h>

#if defined(AN_OS_WIN32)
#    include <Platform/WindowsDefs.h>
#    include <objbase.h>
void AGUID::Generate()
{
    GUID id;
    CoCreateGuid(&id);
    // clang-format off
    Hi = ( static_cast< uint64_t >( ( id.Data1 >> 24 ) & 0xff ) << 56 )
             | ( static_cast< uint64_t >( ( id.Data1 >> 16 ) & 0xff ) << 48 )
             | ( static_cast< uint64_t >( ( id.Data1 >>  8 ) & 0xff ) << 40 )
             | ( static_cast< uint64_t >( ( id.Data1       ) & 0xff ) << 32 )
             | ( static_cast< uint64_t >( ( id.Data2 >>  8 ) & 0xff ) << 24 )
             | ( static_cast< uint64_t >( ( id.Data2       ) & 0xff ) << 16 )
             | ( static_cast< uint64_t >( ( id.Data3 >>  8 ) & 0xff ) << 8 )
             | ( static_cast< uint64_t >( ( id.Data3       ) & 0xff ) << 0 );
    Lo = ( static_cast< uint64_t >( id.Data4[0] ) << 56 )
             | ( static_cast< uint64_t >( id.Data4[1] ) << 48 )
             | ( static_cast< uint64_t >( id.Data4[2] ) << 40 )
             | ( static_cast< uint64_t >( id.Data4[3] ) << 32 )
             | ( static_cast< uint64_t >( id.Data4[4] ) << 24 )
             | ( static_cast< uint64_t >( id.Data4[5] ) << 16 )
             | ( static_cast< uint64_t >( id.Data4[6] ) << 8 )
             | ( static_cast< uint64_t >( id.Data4[7] ) << 0 );
    // clang-format on
}
#elif defined(AN_OS_LINUX)
#    include <uuid/uuid.h>
void AGUID::Generate()
{
    uuid_t id;
    uuid_generate(id);
    // clang-format off
    Hi = ( static_cast< uint64_t >( id[0] ) << 56 )
             | ( static_cast< uint64_t >( id[1] ) << 48 )
             | ( static_cast< uint64_t >( id[2] ) << 40 )
             | ( static_cast< uint64_t >( id[3] ) << 32 )
             | ( static_cast< uint64_t >( id[4] ) << 24 )
             | ( static_cast< uint64_t >( id[5] ) << 16 )
             | ( static_cast< uint64_t >( id[6] ) << 8 )
             | ( static_cast< uint64_t >( id[7] ) << 0 );
    Lo = ( static_cast< uint64_t >( id[8] ) << 56 )
             | ( static_cast< uint64_t >( id[9] ) << 48 )
             | ( static_cast< uint64_t >( id[10] ) << 40 )
             | ( static_cast< uint64_t >( id[11] ) << 32 )
             | ( static_cast< uint64_t >( id[12] ) << 24 )
             | ( static_cast< uint64_t >( id[13] ) << 16 )
             | ( static_cast< uint64_t >( id[14] ) << 8 )
             | ( static_cast< uint64_t >( id[15] ) << 0 );
    // clang-format on
}
#elif defined(AN_OS_APPLE)
void AGUID::Generate()
{
    auto id    = CFUUIDCreate(NULL);
    auto bytes = CFUUIDGetUUIDBytes(id);
    CFRelease(id);
    // clang-format off
    Hi = ( static_cast< uint64_t >( bytes.byte0 ) << 56 )
             | ( static_cast< uint64_t >( bytes.byte1 ) << 48 )
             | ( static_cast< uint64_t >( bytes.byte2 ) << 40 )
             | ( static_cast< uint64_t >( bytes.byte3 ) << 32 )
             | ( static_cast< uint64_t >( bytes.byte4 ) << 24 )
             | ( static_cast< uint64_t >( bytes.byte5 ) << 16 )
             | ( static_cast< uint64_t >( bytes.byte6 ) << 8 )
             | ( static_cast< uint64_t >( bytes.byte7 ) << 0 );
    Lo = ( static_cast< uint64_t >( bytes.byte8 ) << 56 )
             | ( static_cast< uint64_t >( bytes.byte9 ) << 48 )
             | ( static_cast< uint64_t >( bytes.byte10 ) << 40 )
             | ( static_cast< uint64_t >( bytes.byte11 ) << 32 )
             | ( static_cast< uint64_t >( bytes.byte12 ) << 24 )
             | ( static_cast< uint64_t >( bytes.byte13 ) << 16 )
             | ( static_cast< uint64_t >( bytes.byte14 ) << 8 )
             | ( static_cast< uint64_t >( bytes.byte15 ) << 0 );
    // clang-format on
}
#else
#    error "GenerateGUID is not implemented on current platform"
#endif

AGUID& AGUID::FromString(const char* _String)
{
    char ch;
    int  n = 0;

    Hi = Lo = 0;

    byte* bytes = GetBytes();

    for (const char* s = _String; *s && n < 32; s++)
    {
        if (*s == '-')
        {
            continue;
        }

        ch = *s;

        if (ch >= '0' && ch <= '9')
        {
            ch -= '0';
        }
        else if (ch >= 'a' && ch <= 'f')
        {
            ch = ch - 'a' + 10;
        }
        else if (ch >= 'A' && ch <= 'F')
        {
            ch = ch - 'A' + 10;
        }
        else
        {
            ch = 0;
        }

        if (n & 1)
        {
            bytes[n >> 1] |= ch;
        }
        else
        {
            bytes[n >> 1] |= ch << 4;
        }

        n++;
    }

    return *this;
}
