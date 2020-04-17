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

#pragma once

#include "BaseTypes.h"
#include <stdarg.h>

namespace Core {

//
// String comparison
//

/** Case insensitive string comparision */
int Stricmp( const char * _S1, const char * _S2 );

/** Case insensitive string comparision */
int StricmpN( const char * _S1, const char * _S2, int _Num );

/** Case sensitive string comparision */
int Strcmp( const char * _S1, const char * _S2 );

/** Case sensitive string comparision */
int StrcmpN( const char * _S1, const char * _S2, int _Num );

//
// String formatting
//

/** The output is always null-terminated and truncated if necessary.
The return value is either the number of characters stored or -1 if truncation occurs. */
int Sprintf( char * _Buffer, int _Size, const char * _Format, ... );

/** The output is always null-terminated and truncated if necessary.
The return value is either the number of characters stored or -1 if truncation occurs. */
int VSprintf( char * _Buffer, int _Size, const char * _Format, va_list _VaList );

/** Format string on stack. Be careful to use it in nested calls (max allowed nested calls = 4) */
char * Fmt( const char * _Format, ... );

//
// String modification
//

/** Concatenate strings */
void Strcat( char * _Dest, size_t _DestCapacity, const char * _Src );

/** Concatenate strings */
void StrcatN( char * _Dest, size_t _DestCapacity, const char * _Src, int _Num );

/** Copy string */
void Strcpy( char * _Dest, size_t _DestCapacity, const char * _Src );

/** Copy string */
void StrcpyN( char * _Dest, size_t _DestCapacity, const char * _Src, int _Num );

/** Convert string to lowercase */
char * ToLower( char * _Str );

/** Convert string to uppercase */
char * ToUpper( char * _Str );

//
// Misc
//

/** Calc string length */
int Strlen( const char * _Str );

/** Find char in string */
int StrContains( const char * _String, char _Ch );

/** Find substring. Return -1 if substring wasn't found. Return substring offset on success. */
int Substring( const char * _Str, const char * _SubStr );

/** Convert hex string to uint32 */
uint32_t HexToUInt32( const char * _Str, int _Len );

/** Convert hex string to uint64 */
uint64_t HexToUInt64( const char * _Str, int _Len );

//
// Path utilites
//

/** Compare two paths */
int CmpPath( const char * _Path1, const char * _Path2 );

/** Compare two paths */
int CmpPathN( const char * _Path1, const char * _Path2, int _Num );

/** Replace separator \\ to / */
void FixSeparator( char * _Path );

/** Fix path string insitu: replace separator \\ to /, skip series of /,
skip redunant sequinces of dir/../dir2 -> dir2.
Return length of optimized path. */
int FixPath( char * _Path, int _Length );

/** Fix path string insitu: replace separator \\ to /, skip series of /,
skip redunant sequinces of dir/../dir2 -> dir2.
Return length of optimized path. */
int FixPath( char * _Path );

/** Calc path length without file name */
int FindPath( const char * _Path );

/** Find file extension offset in path */
int FindExt( const char * _Path );

/** Find file extension offset (without dot) in path */
int FindExtWithoutDot( const char * _Path );

/** Check char is a path separator */
bool IsPathSeparator( char _Char );

}
