/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <Core/Public/String.h>

const AString AString::NullStr;

void AString::GrowCapacity( int _Capacity, bool _CopyOld ) {
    AN_ASSERT_( _Capacity > 0, "Invalid capacity" );

    if ( _Capacity <= Capacity ) {
        return;
    }

    const int mod = _Capacity % GRANULARITY;

    Capacity = mod ? _Capacity + GRANULARITY - mod : _Capacity;

    if ( Data == Base ) {
        Data = ( char * )Allocator::Inst().Alloc( Capacity );
        if ( _CopyOld ) {
            Core::Memcpy( Data, Base, Size + 1 );
        }
    } else {
        Data = ( char * )Allocator::Inst().Realloc( Data, Capacity, _CopyOld );
    }
}

void AString::operator=( const char * _Str ) {
    int diff;
    int i;

    if ( !_Str ) {
        Clear();
        return;
    }

    if ( _Str == Data ) {
        return; // copying same thing
    }

    // check if we're aliasing
    if ( _Str >= Data && _Str <= Data + Size ) {
        diff = _Str - Data;
        AN_ASSERT_( Core::Strlen( _Str ) < Size, "AString=" );
        for ( i = 0; _Str[ i ]; i++ ) {
            Data[ i ] = _Str[ i ];
        }
        Data[ i ] = '\0';
        Size -= diff;
        return;
    }

    const int newLen = Core::Strlen( _Str );
    GrowCapacity( newLen+1, false );
    Core::Strcpy( Data, Capacity, _Str );
    Size = newLen;
}

void AString::FromCStr( const char * _Str, int _Num ) {
    int i;

    if ( !_Str || _Num <= 0 ) {
        Clear();
        return;
    }

    if ( _Str == Data ) {
        return; // copying same thing
    }

    // check if we're aliasing
    if ( _Str >= Data && _Str <= Data + Size ) {
        //int diff = _Str - StringData;
        AN_ASSERT_( Core::Strlen(_Str ) < Size, "AString::CopyN" );
        for ( i = 0 ; _Str[ i ] && i < _Num ; i++ ) {
            Data[ i ] = _Str[ i ];
        }
        Data[ i ] = '\0';
        Size = i;
        //StringLength -= diff;
        return;
    }

    int newLen = Core::Strlen( _Str );
    if ( newLen > _Num ) {
        newLen = _Num;
    }
    GrowCapacity( newLen+1, false );
    Core::StrcpyN( Data, Capacity, _Str, newLen + 1 );
    Size = newLen;
}

void AString::Concat( AString const & _Str ) {
    int i;

    const int newLen = Size + _Str.Size;
    GrowCapacity( newLen+1, true );
    for ( i = 0; i < _Str.Size; i++ )
        Data[ Size + i ] = _Str[i];
    Size = newLen;
    Data[Size] = '\0';
}

void AString::Concat( const char * _Str ) {
    if ( !_Str ) {
        return;
    }

    const int newLen = Size + Core::Strlen( _Str );
    GrowCapacity( newLen+1, true );
    for ( int i = 0 ; _Str[i] ; i++ ) {
        Data[Size + i] = _Str[i];
    }
    Size = newLen;
    Data[Size] = '\0';
}

void AString::Concat( char _Char ) {
    if ( _Char == 0 ) {
        return;
    }

    const int newLen = Size + 1;
    GrowCapacity( newLen+1, true );
    Data[Size++] = _Char;
    Data[Size] = '\0';
}

void AString::Insert( AString const & _Str, int _Index ) {
    int i;
    if ( _Index < 0 || _Index > Size ) {
        return;
    }
    const int textLength = _Str.Length();
    GrowCapacity( Size + textLength + 1, true );
    for ( i = Size; i >= _Index; i-- ) {
        Data[ i + textLength ] = Data[ i ];
    }
    for ( i = 0; i < textLength; i++ ) {
        Data[ _Index + i ] = _Str[ i ];
    }
    Size += textLength;
}

void AString::Insert( const char * _Str, int _Index ) {
    int i;
    if ( !_Str ) {
        return;
    }
    if ( _Index < 0 || _Index > Size ) {
        return;
    }
    const int textLength = Core::Strlen( _Str );
    GrowCapacity( Size + textLength + 1, true );
    for ( i = Size; i >= _Index; i-- ) {
        Data[ i + textLength ] = Data[ i ];
    }
    for ( i = 0; i < textLength; i++ ) {
        Data[ _Index + i ] = _Str[ i ];
    }
    Size += textLength;
}

void AString::Insert( char _Char, int _Index ) {
    if ( _Index < 0 || _Index > Size ) {
        return;
    }
    if ( _Char == 0 ) {
        return;
    }

    GrowCapacity( Size + 2, true );
    for ( int i = Size ; i >= _Index; i-- ) {
        Data[i+1] = Data[i];
    }
    Data[_Index] = _Char;
    Size++;
}

void AString::Replace( AString const & _Str, int _Index ) {
    if ( _Index < 0 || _Index > Size ) {
        return;
    }

    const int newLen = _Index + _Str.Size;
    GrowCapacity( newLen+1, true );
    for ( int i = 0; i < _Str.Size; i++ ) {
        Data[ _Index + i ] = _Str[i];
    }
    Size = newLen;
    Data[Size] = '\0';
}

void AString::Replace( const char * _Str, int _Index ) {
    if ( !_Str ) {
        return;
    }
    if ( _Index < 0 || _Index > Size ) {
        return;
    }

    const int newLen = _Index + Core::Strlen( _Str );
    GrowCapacity( newLen+1, true );
    for ( int i = 0 ; _Str[i] ; i++ ) {
        Data[ _Index + i ] = _Str[i];
    }
    Size = newLen;
    Data[Size] = '\0';
}

void AString::Replace( const char * _Substring, const char * _NewStr ) {
    if ( !_Substring || !_NewStr ) {
        return;
    }
    const int len = Core::Strlen( _Substring );
    int index;
    while ( -1 != (index = Substring( _Substring )) ) {
        Cut( index, len );
        Insert( _NewStr, index );
    }
}

void AString::Cut( int _Index, int _Count ) {
    int i;
    if ( _Count <= 0 ) {
        return;
    }
    if ( _Index < 0 ) {
        return;
    }
    if ( _Index >= Size ) {
        return;
    }
    for ( i = _Index + _Count ; i < Size ; i++ ) {
        Data[ i - _Count ] = Data[ i ];
    }
    Size = i - _Count;
    Data[ Size ] = 0;
}

int AString::Contains( char _Ch ) const {
    for ( const char * s = Data ; *s ; s++ ) {
        if ( *s == _Ch ) {
            return s - Data;
        }
    }
    return -1;
}

void AString::SkipTrailingZeros() {
    int i;
    for ( i = Size - 1; i >= 0 ; i-- ) {
        if ( Data[ i ] != '0' ) {
            break;
        }
    }
    if ( Data[ i ] == '.' ) {
        Resize( i );
    } else {
        Resize( i + 1 );
    }
}

void AString::StripPath() {
    AString s(*this);
    char * p = s.Data + s.Size;
    while ( --p >= s.Data && !Core::IsPathSeparator( *p ) ) {
        ;
    }
    *this = ++p;
}

int AString::FindPath() const {
    char * p = Data + Size;
    while ( --p >= Data ) {
        if ( Core::IsPathSeparator( *p ) ) {
            return p - Data + 1;
        }
    }
    return 0;
}

void AString::StripExt() {
    char * p = Data + Size;
    while ( --p >= Data ) {
        if ( *p == '.' ) {
            *p = '\0';
            Size = p - Data;
            break;
        }
        if ( Core::IsPathSeparator( *p ) ) {
            break;        // no extension
        }
    }
}

void AString::StripFilename() {
    char * p = Data + Size;
    while ( --p > Data && !Core::IsPathSeparator( *p ) ) {
        ;
    }
    *p = '\0';
    Size = p-Data;
}

bool AString::CompareExt( const char * _Ext, bool _CaseSensitive ) const {
    if ( !_Ext ) {
        return false;
    }
    char * p = Data + Size;
    const int extLen = Core::Strlen( _Ext );
    const char * ext = _Ext + extLen;
    if ( _CaseSensitive ) {
        char c1, c2;
        while ( --ext >= _Ext ) {

            if ( --p < Data ) {
                return false;
            }

            c1 = *p;
            c2 = *ext;

            if ( c1 != c2 ) {
                if ( c1 >= 'a' && c1 <= 'z' ) {
                    c1 -= ('a' - 'A');
                }
                if ( c2 >= 'a' && c2 <= 'z' ) {
                    c2 -= ('a' - 'A');
                }
                if ( c1 != c2 ) {
                    return false;
                }
            }
        }
    } else {
        while ( --ext >= _Ext ) {
            if ( --p < Data || *p != *ext ) {
                return false;
            }
        }
    }
    return true;
}

void AString::UpdateExt( const char * _Extension ) {
    char * p = Data + Size;
    while ( --p >= Data && !Core::IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return;
        }
    }
    (*this) += _Extension;
}

void AString::FixPath() {
    Size = Core::FixPath( Data, Size );
}

int AString::Substring( const char * _Substring ) const {
    return Core::Substring( Data, _Substring );
}

int AString::FindExt() const {
    char * p = Data + Size;
    while ( --p >= Data && !Core::IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - Data;
        }
    }
    return Size;
}

int AString::FindExtWithoutDot() const {
    char * p = Data + Size;
    while ( --p >= Data && !Core::IsPathSeparator( *p ) ) {
        if ( *p == '.' ) {
            return p - Data + 1;
        }
    }
    return Size;
}

void AString::ToLower() {
    Core::ToLower( Data );
}

void AString::ToUpper() {
    Core::ToUpper( Data );
}
