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

#include <Core/String.h>

const AString AString::NullStr;

void AString::GrowCapacity(int _Capacity, bool bCopyOld)
{
    HK_ASSERT(_Capacity > 0);

    if (_Capacity <= Capacity)
    {
        return;
    }

    const int mod = _Capacity % GRANULARITY;

    Capacity = mod ? _Capacity + GRANULARITY - mod : _Capacity;

    if (Data == Base)
    {
        Data = (char*)Allocator::Inst().Alloc(Capacity);
        if (bCopyOld)
        {
            Platform::Memcpy(Data, Base, Size + 1);
        }
    }
    else
    {
        Data = (char*)Allocator::Inst().Realloc(Data, Capacity, bCopyOld);
    }
}

#if 0
void AString::operator=( const char * Str )
{
    int diff;
    int i;

    if ( !Str ) {
        Clear();
        return;
    }

    if ( Str == Data ) {
        return; // copying same thing
    }

    // check if we're aliasing
    if ( Str >= Data && Str <= Data + Size ) {
        diff = Str - Data;
        HK_ASSERT( Platform::Strlen( Str ) < Size );
        for ( i = 0; Str[ i ]; i++ ) {
            Data[ i ] = Str[ i ];
        }
        Data[ i ] = '\0';
        Size -= diff;
        return;
    }

    const int newLen = Platform::Strlen( Str );
    GrowCapacity( newLen+1, false );
    Platform::Strcpy( Data, Capacity, Str );
    Size = newLen;
}
#endif

void AString::Concat(AStringView Rhs)
{
    const int newLen = Size + Rhs.Length();
    GrowCapacity(newLen + 1, true);
    Platform::Memcpy(&Data[Size], Rhs.ToPtr(), Rhs.Length());
    Size       = newLen;
    Data[Size] = '\0';
}

void AString::Concat(char Char)
{
    if (Char == 0)
    {
        return;
    }

    const int newLen = Size + 1;
    GrowCapacity(newLen + 1, true);
    Data[Size++] = Char;
    Data[Size]   = '\0';
}

void AString::Insert(AStringView Str, int Index)
{
    int i;
    if (Index < 0 || Index > Size || Str.IsEmpty())
    {
        return;
    }
    const int textLength = Str.Length();
    GrowCapacity(Size + textLength + 1, true);
    for (i = Size; i >= Index; i--)
    {
        Data[i + textLength] = Data[i];
    }
    for (i = 0; i < textLength; i++)
    {
        Data[Index + i] = Str[i];
    }
    Size += textLength;
}

void AString::Insert(char Char, int Index)
{
    if (Index < 0 || Index > Size)
    {
        return;
    }
    if (Char == 0)
    {
        return;
    }

    GrowCapacity(Size + 2, true);
    for (int i = Size; i >= Index; i--)
    {
        Data[i + 1] = Data[i];
    }
    Data[Index] = Char;
    Size++;
}

void AString::Replace(AStringView Str, int Index)
{
    if (Index < 0 || Index > Size)
    {
        return;
    }

    const int newLen = Index + Str.Length();
    GrowCapacity(newLen + 1, true);
    Platform::Memcpy(&Data[Index], Str.ToPtr(), Str.Length());
    Size       = newLen;
    Data[Size] = '\0';
}

void AString::Replace(AStringView _Substring, AStringView _NewStr)
{
    if (_Substring.IsEmpty())
    {
        return;
    }
    const int len = _Substring.Length();
    int       index;
    while (-1 != (index = FindSubstring(_Substring)))
    {
        Cut(index, len);
        Insert(_NewStr, index);
    }
}

void AString::Cut(int Index, int Count)
{
    int i;
    if (Count <= 0)
    {
        return;
    }
    if (Index < 0)
    {
        return;
    }
    if (Index >= Size)
    {
        return;
    }
    for (i = Index + Count; i < Size; i++)
    {
        Data[i - Count] = Data[i];
    }
    Size       = i - Count;
    Data[Size] = 0;
}

void AString::ClipTrailingZeros()
{
    int i;
    for (i = Size - 1; i >= 0; i--)
    {
        if (Data[i] != '0')
        {
            break;
        }
    }
    if (Data[i] == '.')
    {
        Resize(i);
    }
    else
    {
        Resize(i + 1);
    }
}

void AString::ClipPath()
{
    AString s(*this);
    char*   p = s.Data + s.Size;
    while (--p >= s.Data && !Platform::IsPathSeparator(*p))
    {
        ;
    }
    *this = ++p;
}

void AString::ClipExt()
{
    char* p = Data + Size;
    while (--p >= Data)
    {
        if (*p == '.')
        {
            *p   = '\0';
            Size = p - Data;
            break;
        }
        if (Platform::IsPathSeparator(*p))
        {
            break; // no extension
        }
    }
}

void AString::ClipFilename()
{
    char* p = Data + Size;
    while (--p > Data && !Platform::IsPathSeparator(*p))
    {
        ;
    }
    *p   = '\0';
    Size = p - Data;
}

void AString::UpdateExt(AStringView _Extension)
{
    char* p = Data + Size;
    while (--p >= Data && !Platform::IsPathSeparator(*p))
    {
        if (*p == '.')
        {
            return;
        }
    }
    (*this) += _Extension;
}

void AString::FixPath()
{
    Size = Platform::FixPath(Data, Size);
}

void AString::ToLower()
{
    Platform::ToLower(Data);
}

void AString::ToUpper()
{
    Platform::ToUpper(Data);
}


int AStringView::Icmp(AStringView Str) const
{
    const char* s1 = Data;
    const char* e1 = Data + Size;
    const char* s2 = Str.Data;
    const char* e2 = Str.Data + Str.Size;

    char c1, c2;

    do {
        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

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

int AStringView::Cmp(AStringView Str) const
{
    const char* s1 = Data;
    const char* e1 = Data + Size;
    const char* s2 = Str.Data;
    const char* e2 = Str.Data + Str.Size;

    char c1, c2;

    do {
        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

        if (c1 != c2)
        {
            return (int)((uint8_t)c1 - (uint8_t)c2);
        }
    } while (c1);

    return 0;
}

int AStringView::IcmpN(AStringView Str, int Num) const
{
    HK_ASSERT(Num >= 0);

    const char* s1 = Data;
    const char* e1 = Data + Size;
    const char* s2 = Str.Data;
    const char* e2 = Str.Data + Str.Size;

    char c1, c2;

    do {
        if (!Num--)
        {
            return 0;
        }

        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

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

int AStringView::CmpN(AStringView Str, int Num) const
{
    HK_ASSERT(Num >= 0);

    const char* s1 = Data;
    const char* e1 = Data + Size;
    const char* s2 = Str.Data;
    const char* e2 = Str.Data + Str.Size;

    char c1, c2;

    do {
        if (!Num--)
        {
            return 0;
        }

        c1 = s1 < e1 ? *s1++ : 0;
        c2 = s2 < e2 ? *s2++ : 0;

        if (c1 != c2)
        {
            return (int)((uint8_t)c1 - (uint8_t)c2);
        }
    } while (c1);

    return 0;
}
