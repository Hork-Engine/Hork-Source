/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Platform/Public/Path.h>
#include <Platform/Public/Memory/Memory.h>

namespace Platform
{

int CmpPath(const char* _Path1, const char* _Path2)
{
    char c1, c2;

    AN_ASSERT(_Path1 && _Path2);

    do {
        c1 = *_Path1++;
        c2 = *_Path2++;

        if (c1 != c2)
        {
            if (c1 >= 'a' && c1 <= 'z')
            {
                c1 -= ('a' - 'A');
            }
            else if (c1 == '\\')
            {
                c1 = '/';
            }
            if (c2 >= 'a' && c2 <= 'z')
            {
                c2 -= ('a' - 'A');
            }
            else if (c2 == '\\')
            {
                c2 = '/';
            }
            if (c1 != c2)
            {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while (c1);

    return 0;
}

int CmpPathN(const char* _Path1, const char* _Path2, int _Num)
{
    char c1, c2;

    AN_ASSERT(_Path1 && _Path2 && _Num >= 0);

    do {
        if (!_Num--)
        {
            return 0;
        }

        c1 = *_Path1++;
        c2 = *_Path2++;

        if (c1 != c2)
        {
            if (c1 >= 'a' && c1 <= 'z')
            {
                c1 -= ('a' - 'A');
            }
            else if (c1 == '\\')
            {
                c1 = '/';
            }
            if (c2 >= 'a' && c2 <= 'z')
            {
                c2 -= ('a' - 'A');
            }
            else if (c2 == '\\')
            {
                c2 = '/';
            }
            if (c1 != c2)
            {
                return (int)((uint8_t)c1 - (uint8_t)c2);
            }
        }
    } while (c1);

    return 0;
}

void FixSeparator(char* _Path)
{
    if (!_Path)
    {
        return;
    }
    while (*_Path)
    {
        if (*_Path == '\\')
        {
            *_Path = '/';
        }
        _Path++;
    }
}

int FixPath(char* _Path, int _Length)
{
    if (!_Path)
    {
        return 0;
    }
    char* s = _Path;
    char* t;
    char  tmp;
    int   num;
    int   ofs;
    int   stack[1024];
    int   sp = 0;
#ifdef AN_OS_LINUX
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
            Memmove(s, s + num, _Length - (s + num - _Path) + 1); // + 1 for trailing zero
            _Length -= num;
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
            Memmove(_Path + ofs, s, _Length - (s - _Path) + 1); // + 1 for trailing zero
            _Length -= (s - _Path) - ofs;
            s = _Path + ofs;
            // parse next dir
            continue;
        }
        else
        {
            *t = tmp;
            // save offset
            if (sp == AN_ARRAY_SIZE(stack))
            {
                // stack overflow, can't resolve path
                AN_ASSERT(0);
                return 0;
            }
            stack[sp++] = int(s - _Path);
            // go to next dir
            s = t;
            if (!*s)
            {
                break;
            }
            s++;
        }
    }
    return _Length;
}

int FixPath(char* _Path)
{
    return FixPath(_Path, strlen(_Path));
}

int FindPath(const char* _Path)
{
    int         len = strlen(_Path);
    const char* p   = _Path + len;
    while (--p >= _Path)
    {
        if (IsPathSeparator(*p))
        {
            return p - _Path + 1;
        }
    }
    return 0;
}

int FindExt(const char* _Path)
{
    int         len = strlen(_Path);
    const char* p   = _Path + len;
    while (--p >= _Path && !IsPathSeparator(*p))
    {
        if (*p == '.')
        {
            return p - _Path;
        }
    }
    return len;
}

int FindExtWithoutDot(const char* _Path)
{
    int         len = strlen(_Path);
    const char* p   = _Path + len;
    while (--p >= _Path && !IsPathSeparator(*p))
    {
        if (*p == '.')
        {
            return p - _Path + 1;
        }
    }
    return len;
}

bool IsPathSeparator(char _Char)
{
#ifdef AN_OS_WIN32
    return _Char == '/' || _Char == '\\';
#else
    return _Char == '/';
#endif
}

} // namespace Platform
