/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

#include <Engine/Core/Public/BaseTypes.h>

namespace FCore {

// Robert Sedgwicks Algorithms in C book
ANGIE_API unsigned int RSHash( const char * _Str, int _Length );

// Justin Sobels bitwise hash function
ANGIE_API unsigned int JSHash( const char * _Str, int _Length );

// Peter J. Weinberger hash algorithm
ANGIE_API unsigned int PJWHash( const char * _Str, int _Length );

// Similar to the PJW Hash function, but tweaked for 32-bit processors. Its the hash function widely used on most UNIX systems.
ANGIE_API unsigned int ELFHash( const char * _Str, int _Length );

// Brian Kernighan and Dennis Ritchie's (Book "The C Programming Language")
ANGIE_API unsigned int BKDRHash( const char * _Str, int _Length );

// Algorithm is used in the open source SDBM project
ANGIE_API unsigned int SDBMHash( const char * _Str, int _Length );

// Algorithm produced by Professor Daniel J. Bernstein
ANGIE_API unsigned int DJBHash( const char * _Str, int _Length );

// Donald E. Knuth algorithm (The Art Of Computer Programming Volume 3)
ANGIE_API unsigned int DEKHash( const char * _Str, int _Length );

// Arash Partow algorithm
ANGIE_API unsigned int APHash( const char * _Str, int _Length );

}

