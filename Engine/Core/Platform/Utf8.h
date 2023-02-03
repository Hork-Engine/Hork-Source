/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "BaseTypes.h"

HK_NAMESPACE_BEGIN

typedef uint16_t WideChar;

namespace Core
{

/** Length of utf8 character in bytes */
int UTF8CharSizeInBytes(const char* _Unicode);

/** Length of utf8 string */
int UTF8StrLength(const char* _Unicode);

/** Length of utf8 string */
int UTF8StrLength(const char* _Unicode, const char* _UnicodeEnd);

/** Decode utf8 character to wide char */
int WideCharDecodeUTF8(const char* _Unicode, WideChar& _Ch);

/** Decode utf8 character to wide char */
int WideCharDecodeUTF8(const char* _Unicode, const char* _UnicodeEnd, WideChar& _Ch);

/** Decode utf8 string to wide string */
int WideStrDecodeUTF8(const char* _Unicode, WideChar* _Str, int _MaxLength);

/** Decode utf8 string to wide string */
int WideStrDecodeUTF8(const char* _Unicode, const char* _UnicodeEnd, WideChar* _Str, int _MaxLength);

/** Length of utf8 character in bytes */
HK_FORCEINLINE int WideCharUTF8Bytes(WideChar _Ch)
{
    if (_Ch < 0x80) return 1;
    if (_Ch < 0x800) return 2;
    if (_Ch >= 0xdc00 && _Ch < 0xe000) return 0;
    if (_Ch >= 0xd800 && _Ch < 0xdc00) return 4;
    return 3;
}

/** Length of utf8 string in bytes */
int WideStrUTF8Bytes(WideChar const* _Str, WideChar const* _StrEnd = nullptr);

/** Length of wide string */
int WideStrLength(WideChar const* _Str);

/** Encode wide character to utf8 char */
int WideCharEncodeUTF8(char* _Buf, int _BufSize, unsigned int _Ch);

/** Encode wide string to utf8 string */
int WideStrEncodeUTF8(char* _Buf, int _BufSize, WideChar const* _Str, WideChar const* _StrEnd = nullptr);

/** Check wide char is blank */
HK_FORCEINLINE bool WideCharIsBlank(WideChar _Ch) { return _Ch == ' ' || _Ch == '\t' || _Ch == 0x3000; }

/** Check ascii char is blank */
HK_FORCEINLINE bool CharIsBlank(char _Ch) { return _Ch == ' ' || _Ch == '\t'; }

} // namespace Core

HK_NAMESPACE_END
