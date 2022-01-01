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

namespace Core
{

/** Compare two paths */
int CmpPath(const char* _Path1, const char* _Path2);

/** Compare two paths */
int CmpPathN(const char* _Path1, const char* _Path2, int _Num);

/** Replace separator \\ to / */
void FixSeparator(char* _Path);

/** Fix path string insitu: replace separator \\ to /, skip series of /,
skip redunant sequinces of dir/../dir2 -> dir2.
Return length of optimized path. */
int FixPath(char* _Path, int _Length);

/** Fix path string insitu: replace separator \\ to /, skip series of /,
skip redunant sequinces of dir/../dir2 -> dir2.
Return length of optimized path. */
int FixPath(char* _Path);

/** Calc path length without file name */
int FindPath(const char* _Path);

/** Find file extension offset in path */
int FindExt(const char* _Path);

/** Find file extension offset (without dot) in path */
int FindExtWithoutDot(const char* _Path);

/** Check char is a path separator */
bool IsPathSeparator(char _Char);

} // namespace Core
