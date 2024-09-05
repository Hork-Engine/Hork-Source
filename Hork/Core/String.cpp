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

#include "String.h"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#include <stb/stb_sprintf.h>

HK_NAMESPACE_BEGIN

namespace Core
{

int Stricmp(const char* _S1, const char* _S2)
{
    char c1, c2;

    HK_ASSERT(_S1 && _S2);

    do {
        c1 = *_S1++;
        c2 = *_S2++;

        if (c1 != c2)
        {
            if (c1 >= 'a' && c1 <= 'z')
            {
                c1 -= ('a' - 'A');
            }
            if (c2 >= 'a' && c2 <= 'z')
            {
                c2 -= ('a' - 'A');
            }
            if (c1 != c2)
            {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while (c1);

    return 0;
}

int StricmpN(const char* _S1, const char* _S2, int _Num)
{
    char c1, c2;

    HK_ASSERT(_S1 && _S2 && _Num >= 0);

    do {
        if (!_Num--)
        {
            return 0;
        }

        c1 = *_S1++;
        c2 = *_S2++;

        if (c1 != c2)
        {
            if (c1 >= 'a' && c1 <= 'z')
            {
                c1 -= ('a' - 'A');
            }
            if (c2 >= 'a' && c2 <= 'z')
            {
                c2 -= ('a' - 'A');
            }
            if (c1 != c2)
            {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while (c1);

    return 0;
}

int Strcmp(const char* _S1, const char* _S2)
{
    HK_ASSERT(_S1 && _S2);

    while (*_S1 == *_S2)
    {
        if (!*_S1)
        {
            return 0;
        }
        _S1++;
        _S2++;
    }
    return (int)((uint8_t)*_S1 - (uint8_t)*_S2);
}

int StrcmpN(const char* _S1, const char* _S2, int _Num)
{
    char c1, c2;

    HK_ASSERT(_S1 && _S2 && _Num >= 0);

    do {
        if (!_Num--)
        {
            return 0;
        }

        c1 = *_S1++;
        c2 = *_S2++;

        if (c1 != c2)
        {
            return (int)((uint8_t)c1 - (uint8_t)c2);
        }

    } while (c1);

    return 0;
}

int Sprintf(char* _Buffer, size_t _Size, const char* _Format, ...)
{
    int result;
    va_list va;

    va_start(va, _Format);
    result = stbsp_vsnprintf(_Buffer, _Size, _Format, va);
    va_end(va);

    return result;
}

int VSprintf(char* _Buffer, size_t _Size, const char* _Format, va_list _VaList)
{
    HK_ASSERT(_Buffer && _Format);

    return stbsp_vsnprintf(_Buffer, _Size, _Format, _VaList);
}

void Strcat(char* _Dest, size_t _Size, const char* _Src)
{
    if (!_Dest || !_Src)
    {
        return;
    }
    //#ifdef HK_COMPILER_MSVC
    //    strcat_s( _Dest, _Size, _Src );
    //#else
    size_t destLength = Strlen(_Dest);
    if (destLength >= _Size)
    {
        return;
    }
    Strcpy(_Dest + destLength, _Size - destLength, _Src);
    //#endif
}

void StrcatN(char* _Dest, size_t _Size, const char* _Src, int _Num)
{
    if (!_Dest || !_Src)
    {
        return;
    }

    size_t destLength = Strlen(_Dest);
    if (destLength >= _Size)
    {
        return;
    }

    int len = Strlen(_Src);
    if (len > _Num)
    {
        len = _Num;
    }

    StrcpyN(_Dest + destLength, _Size - destLength, _Src, _Num);
}

void Strcpy(char* _Dest, size_t _Size, const char* _Src)
{
    if (!_Dest)
    {
        return;
    }
    if (!_Src)
    {
        _Src = "";
    }
    //#ifdef HK_COMPILER_MSVC
    //    strcpy_s( _Dest, _Size, _Src );
    //#else
    if (_Size > 0)
    {
        while (*_Src && --_Size != 0)
        {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
    //#endif
}

void StrcpyN(char* _Dest, size_t _Size, const char* _Src, int _Num)
{
    if (!_Dest)
    {
        return;
    }
    if (!_Src)
    {
        _Src = "";
    }
    //#ifdef HK_COMPILER_MSVC
    //    strncpy_s( _Dest, _Size, _Src, _Num );
    //#else
    if (_Size > 0 && _Num > 0)
    {
        while (*_Src && --_Size != 0 && --_Num != -1)
        {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
    //#endif
}

char* ToLower(char* _Str)
{
    char* p = _Str;
    if (p)
    {
        while (*p)
        {
            *p = ::tolower(*p);
            p++;
        }
    }
    return _Str;
}

char ToLower(char Ch)
{
    return ::tolower(Ch);
}

char* ToUpper(char* _Str)
{
    char* p = _Str;
    if (p)
    {
        while (*p)
        {
            *p = ::toupper(*p);
            p++;
        }
    }
    return _Str;
}

char ToUpper(char Ch)
{
    return ::toupper(Ch);
}

int Strlen(const char* _Str)
{
    if (!_Str)
    {
        return 0;
    }
    const char* p = _Str;
    while (*p)
    {
        p++;
    }
    return p - _Str;

    //return (int)strlen( _Str );
}

int StrContains(const char* _String, char _Ch)
{
    if (_String)
    {
        for (const char* s = _String; *s; s++)
        {
            if (*s == _Ch)
            {
                return s - _String;
            }
        }
    }
    return -1;
}

int Substring(const char* _Str, const char* _SubStr)
{
    if (!_Str || !_SubStr)
    {
        return -1;
    }
    const char* s = strstr(_Str, _SubStr);
    if (!s)
    {
        return -1;
    }
    return (int)(s - _Str);
}

int SubstringIcmp(const char* _Str, const char* _SubStr)
{
    if (!_Str || !_SubStr)
    {
        return -1;
    }
    const char* s = _Str;
    int length = Strlen(_SubStr);
    while (*s)
    {
        if (StricmpN(s, _SubStr, length) == 0)
        {
            return (int)(s - _Str);
        }
        ++s;
    }
    return -1;
}

uint32_t HexToUInt32(const char* _Str, int _Len)
{
    uint32_t value = 0;

    if (!_Str)
    {
        return 0;
    }

    for (int i = std::max(0, _Len - 8); i < _Len; i++)
    {
        uint32_t ch = _Str[i];
        if (ch >= 'A' && ch <= 'F')
        {
            value = (value << 4) + ch - 'A' + 10;
        }
        else if (ch >= 'a' && ch <= 'f')
        {
            value = (value << 4) + ch - 'a' + 10;
        }
        else if (ch >= '0' && ch <= '9')
        {
            value = (value << 4) + ch - '0';
        }
        else
        {
            return value;
        }
    }

    return value;
}

uint64_t HexToUInt64(const char* _Str, int _Len)
{
    uint64_t value = 0;

    if (!_Str)
    {
        return 0;
    }

    for (int i = std::max(0, _Len - 16); i < _Len; i++)
    {
        uint64_t ch = _Str[i];
        if (ch >= 'A' && ch <= 'F')
        {
            value = (value << 4) + ch - 'A' + 10;
        }
        else if (ch >= 'a' && ch <= 'f')
        {
            value = (value << 4) + ch - 'a' + 10;
        }
        else if (ch >= '0' && ch <= '9')
        {
            value = (value << 4) + ch - '0';
        }
        else
        {
            return value;
        }
    }

    return value;
}

int FixPathInplace(char* path, int length)
{
    if (!path)
    {
        return 0;
    }
    char* s = path;
    char* t;
    char tmp;
    int num;
    int ofs;
    int stack[1024];
    int sp = 0;
#ifdef HK_OS_LINUX
    bool root = *s == '/' || *s == '\\';
#else
    bool root = false;
#endif
    while (*s)
    {
        // skip multiple series of '/'
        num = 0;
        while (*(s + num) == '/' || *(s + num) == '\\')
        {
            num++;
        }
        if (num > 0)
        {
            if (root)
            {
                // Keep '/' at start of the path
                num--;
                *s = '/'; // replace '\\' by '/' (if any)
                s++;
            }
            Core::Memmove(s, s + num, length - (s + num - path) + 1); // + 1 for trailing zero
            length -= num;
        }
        root = false;
        // get dir name
        t = s;
        while (*t && *t != '/' && *t != '\\')
        {
            t++;
        }
        tmp = *t;
        if (tmp == '\\')
        {
            tmp = '/';
        }
        *t = 0;
        // check if "up-dir"
        if (!strcmp(s, ".."))
        {
            *t = tmp;
            // skip ".." or "../"
            s += 2;
            if (*s == '/')
            {
                s++;
            }
            if (sp == 0)
            {
                // no offset, parse next dir
                continue;
            }
            ofs = stack[--sp];
            // cut off pice of path
            Core::Memmove(path + ofs, s, length - (s - path) + 1); // + 1 for trailing zero
            length -= (s - path) - ofs;
            s = path + ofs;
            // parse next dir
            continue;
        }
        else
        {
            *t = tmp;
            // save offset
            if (sp == HK_ARRAY_SIZE(stack))
            {
                // stack overflow, can't resolve path
                HK_ASSERT(0);
                return 0;
            }
            stack[sp++] = int(s - path);
            // go to next dir
            s = t;
            if (!*s)
            {
                break;
            }
            s++;
        }
    }
    return length;
}

int FixPathInplace(char* path)
{
    return FixPathInplace(path, StringLength(path));
}

} // namespace Core

HK_NAMESPACE_END
