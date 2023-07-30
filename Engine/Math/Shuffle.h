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

HK_NAMESPACE_BEGIN

constexpr int SHUFFLE(const int& x, const int& y, const int& z, const int& w)
{
    return (((x & 3) << 6) | ((y & 3) << 4) | ((z & 3) << 2) | (w & 3));
}

constexpr int sXX = SHUFFLE(0, 0, 0, 0);
constexpr int sXY = SHUFFLE(0, 1, 0, 0);
constexpr int sXZ = SHUFFLE(0, 2, 0, 0);
constexpr int sXW = SHUFFLE(0, 3, 0, 0);
constexpr int sYX = SHUFFLE(1, 0, 0, 0);
constexpr int sYY = SHUFFLE(1, 1, 0, 0);
constexpr int sYZ = SHUFFLE(1, 2, 0, 0);
constexpr int sYW = SHUFFLE(1, 3, 0, 0);
constexpr int sZX = SHUFFLE(2, 0, 0, 0);
constexpr int sZY = SHUFFLE(2, 1, 0, 0);
constexpr int sZZ = SHUFFLE(2, 2, 0, 0);
constexpr int sZW = SHUFFLE(2, 3, 0, 0);
constexpr int sWX = SHUFFLE(3, 0, 0, 0);
constexpr int sWY = SHUFFLE(3, 1, 0, 0);
constexpr int sWZ = SHUFFLE(3, 2, 0, 0);
constexpr int sWW = SHUFFLE(3, 3, 0, 0);

constexpr int sXXX = SHUFFLE(0, 0, 0, 0);
constexpr int sXXY = SHUFFLE(0, 0, 1, 0);
constexpr int sXXZ = SHUFFLE(0, 0, 2, 0);
constexpr int sXXW = SHUFFLE(0, 0, 3, 0);
constexpr int sXYX = SHUFFLE(0, 1, 0, 0);
constexpr int sXYY = SHUFFLE(0, 1, 1, 0);
constexpr int sXYZ = SHUFFLE(0, 1, 2, 0);
constexpr int sXYW = SHUFFLE(0, 1, 3, 0);
constexpr int sXZX = SHUFFLE(0, 2, 0, 0);
constexpr int sXZY = SHUFFLE(0, 2, 1, 0);
constexpr int sXZZ = SHUFFLE(0, 2, 2, 0);
constexpr int sXZW = SHUFFLE(0, 2, 3, 0);
constexpr int sYXX = SHUFFLE(1, 0, 0, 0);
constexpr int sYXY = SHUFFLE(1, 0, 1, 0);
constexpr int sYXZ = SHUFFLE(1, 0, 2, 0);
constexpr int sYXW = SHUFFLE(1, 0, 3, 0);
constexpr int sYYX = SHUFFLE(1, 1, 0, 0);
constexpr int sYYY = SHUFFLE(1, 1, 1, 0);
constexpr int sYYZ = SHUFFLE(1, 1, 2, 0);
constexpr int sYYW = SHUFFLE(1, 1, 3, 0);
constexpr int sYZX = SHUFFLE(1, 2, 0, 0);
constexpr int sYZY = SHUFFLE(1, 2, 1, 0);
constexpr int sYZZ = SHUFFLE(1, 2, 2, 0);
constexpr int sYZW = SHUFFLE(1, 2, 3, 0);
constexpr int sZXX = SHUFFLE(2, 0, 0, 0);
constexpr int sZXY = SHUFFLE(2, 0, 1, 0);
constexpr int sZXZ = SHUFFLE(2, 0, 2, 0);
constexpr int sZXW = SHUFFLE(2, 0, 3, 0);
constexpr int sZYX = SHUFFLE(2, 1, 0, 0);
constexpr int sZYY = SHUFFLE(2, 1, 1, 0);
constexpr int sZYZ = SHUFFLE(2, 1, 2, 0);
constexpr int sZYW = SHUFFLE(2, 1, 3, 0);
constexpr int sZZX = SHUFFLE(2, 2, 0, 0);
constexpr int sZZY = SHUFFLE(2, 2, 1, 0);
constexpr int sZZZ = SHUFFLE(2, 2, 2, 0);
constexpr int sZZW = SHUFFLE(2, 2, 3, 0);
constexpr int sWXX = SHUFFLE(3, 0, 0, 0);
constexpr int sWXY = SHUFFLE(3, 0, 1, 0);
constexpr int sWXZ = SHUFFLE(3, 0, 2, 0);
constexpr int sWXW = SHUFFLE(3, 0, 3, 0);
constexpr int sWYX = SHUFFLE(3, 1, 0, 0);
constexpr int sWYY = SHUFFLE(3, 1, 1, 0);
constexpr int sWYZ = SHUFFLE(3, 1, 2, 0);
constexpr int sWYW = SHUFFLE(3, 1, 3, 0);
constexpr int sWZX = SHUFFLE(3, 2, 0, 0);
constexpr int sWZY = SHUFFLE(3, 2, 1, 0);
constexpr int sWZZ = SHUFFLE(3, 2, 2, 0);
constexpr int sWZW = SHUFFLE(3, 2, 3, 0);

constexpr int sXXXX = SHUFFLE(0, 0, 0, 0);
constexpr int sXXXY = SHUFFLE(0, 0, 0, 1);
constexpr int sXXXZ = SHUFFLE(0, 0, 0, 2);
constexpr int sXXXW = SHUFFLE(0, 0, 0, 3);
constexpr int sXXYX = SHUFFLE(0, 0, 1, 0);
constexpr int sXXYY = SHUFFLE(0, 0, 1, 1);
constexpr int sXXYZ = SHUFFLE(0, 0, 1, 2);
constexpr int sXXYW = SHUFFLE(0, 0, 1, 3);
constexpr int sXXZX = SHUFFLE(0, 0, 2, 0);
constexpr int sXXZY = SHUFFLE(0, 0, 2, 1);
constexpr int sXXZZ = SHUFFLE(0, 0, 2, 2);
constexpr int sXXZW = SHUFFLE(0, 0, 2, 3);
constexpr int sXXWX = SHUFFLE(0, 0, 3, 0);
constexpr int sXXWY = SHUFFLE(0, 0, 3, 1);
constexpr int sXXWZ = SHUFFLE(0, 0, 3, 2);
constexpr int sXXWW = SHUFFLE(0, 0, 3, 3);
constexpr int sXYXX = SHUFFLE(0, 1, 0, 0);
constexpr int sXYXY = SHUFFLE(0, 1, 0, 1);
constexpr int sXYXZ = SHUFFLE(0, 1, 0, 2);
constexpr int sXYXW = SHUFFLE(0, 1, 0, 3);
constexpr int sXYYX = SHUFFLE(0, 1, 1, 0);
constexpr int sXYYY = SHUFFLE(0, 1, 1, 1);
constexpr int sXYYZ = SHUFFLE(0, 1, 1, 2);
constexpr int sXYYW = SHUFFLE(0, 1, 1, 3);
constexpr int sXYZX = SHUFFLE(0, 1, 2, 0);
constexpr int sXYZY = SHUFFLE(0, 1, 2, 1);
constexpr int sXYZZ = SHUFFLE(0, 1, 2, 2);
constexpr int sXYZW = SHUFFLE(0, 1, 2, 3);
constexpr int sXYWX = SHUFFLE(0, 1, 3, 0);
constexpr int sXYWY = SHUFFLE(0, 1, 3, 1);
constexpr int sXYWZ = SHUFFLE(0, 1, 3, 2);
constexpr int sXYWW = SHUFFLE(0, 1, 3, 3);
constexpr int sXZXX = SHUFFLE(0, 2, 0, 0);
constexpr int sXZXY = SHUFFLE(0, 2, 0, 1);
constexpr int sXZXZ = SHUFFLE(0, 2, 0, 2);
constexpr int sXZXW = SHUFFLE(0, 2, 0, 3);
constexpr int sXZYX = SHUFFLE(0, 2, 1, 0);
constexpr int sXZYY = SHUFFLE(0, 2, 1, 1);
constexpr int sXZYZ = SHUFFLE(0, 2, 1, 2);
constexpr int sXZYW = SHUFFLE(0, 2, 1, 3);
constexpr int sXZZX = SHUFFLE(0, 2, 2, 0);
constexpr int sXZZY = SHUFFLE(0, 2, 2, 1);
constexpr int sXZZZ = SHUFFLE(0, 2, 2, 2);
constexpr int sXZZW = SHUFFLE(0, 2, 2, 3);
constexpr int sXZWX = SHUFFLE(0, 2, 3, 0);
constexpr int sXZWY = SHUFFLE(0, 2, 3, 1);
constexpr int sXZWZ = SHUFFLE(0, 2, 3, 2);
constexpr int sXZWW = SHUFFLE(0, 2, 3, 3);
constexpr int sXWXX = SHUFFLE(0, 3, 0, 0);
constexpr int sXWXY = SHUFFLE(0, 3, 0, 1);
constexpr int sXWXZ = SHUFFLE(0, 3, 0, 2);
constexpr int sXWXW = SHUFFLE(0, 3, 0, 3);
constexpr int sXWYX = SHUFFLE(0, 3, 1, 0);
constexpr int sXWYY = SHUFFLE(0, 3, 1, 1);
constexpr int sXWYZ = SHUFFLE(0, 3, 1, 2);
constexpr int sXWYW = SHUFFLE(0, 3, 1, 3);
constexpr int sXWZX = SHUFFLE(0, 3, 2, 0);
constexpr int sXWZY = SHUFFLE(0, 3, 2, 1);
constexpr int sXWZZ = SHUFFLE(0, 3, 2, 2);
constexpr int sXWZW = SHUFFLE(0, 3, 2, 3);
constexpr int sXWWX = SHUFFLE(0, 3, 3, 0);
constexpr int sXWWY = SHUFFLE(0, 3, 3, 1);
constexpr int sXWWZ = SHUFFLE(0, 3, 3, 2);
constexpr int sXWWW = SHUFFLE(0, 3, 3, 3);
constexpr int sYXXX = SHUFFLE(1, 0, 0, 0);
constexpr int sYXXY = SHUFFLE(1, 0, 0, 1);
constexpr int sYXXZ = SHUFFLE(1, 0, 0, 2);
constexpr int sYXXW = SHUFFLE(1, 0, 0, 3);
constexpr int sYXYX = SHUFFLE(1, 0, 1, 0);
constexpr int sYXYY = SHUFFLE(1, 0, 1, 1);
constexpr int sYXYZ = SHUFFLE(1, 0, 1, 2);
constexpr int sYXYW = SHUFFLE(1, 0, 1, 3);
constexpr int sYXZX = SHUFFLE(1, 0, 2, 0);
constexpr int sYXZY = SHUFFLE(1, 0, 2, 1);
constexpr int sYXZZ = SHUFFLE(1, 0, 2, 2);
constexpr int sYXZW = SHUFFLE(1, 0, 2, 3);
constexpr int sYXWX = SHUFFLE(1, 0, 3, 0);
constexpr int sYXWY = SHUFFLE(1, 0, 3, 1);
constexpr int sYXWZ = SHUFFLE(1, 0, 3, 2);
constexpr int sYXWW = SHUFFLE(1, 0, 3, 3);
constexpr int sYYXX = SHUFFLE(1, 1, 0, 0);
constexpr int sYYXY = SHUFFLE(1, 1, 0, 1);
constexpr int sYYXZ = SHUFFLE(1, 1, 0, 2);
constexpr int sYYXW = SHUFFLE(1, 1, 0, 3);
constexpr int sYYYX = SHUFFLE(1, 1, 1, 0);
constexpr int sYYYY = SHUFFLE(1, 1, 1, 1);
constexpr int sYYYZ = SHUFFLE(1, 1, 1, 2);
constexpr int sYYYW = SHUFFLE(1, 1, 1, 3);
constexpr int sYYZX = SHUFFLE(1, 1, 2, 0);
constexpr int sYYZY = SHUFFLE(1, 1, 2, 1);
constexpr int sYYZZ = SHUFFLE(1, 1, 2, 2);
constexpr int sYYZW = SHUFFLE(1, 1, 2, 3);
constexpr int sYYWX = SHUFFLE(1, 1, 3, 0);
constexpr int sYYWY = SHUFFLE(1, 1, 3, 1);
constexpr int sYYWZ = SHUFFLE(1, 1, 3, 2);
constexpr int sYYWW = SHUFFLE(1, 1, 3, 3);
constexpr int sYZXX = SHUFFLE(1, 2, 0, 0);
constexpr int sYZXY = SHUFFLE(1, 2, 0, 1);
constexpr int sYZXZ = SHUFFLE(1, 2, 0, 2);
constexpr int sYZXW = SHUFFLE(1, 2, 0, 3);
constexpr int sYZYX = SHUFFLE(1, 2, 1, 0);
constexpr int sYZYY = SHUFFLE(1, 2, 1, 1);
constexpr int sYZYZ = SHUFFLE(1, 2, 1, 2);
constexpr int sYZYW = SHUFFLE(1, 2, 1, 3);
constexpr int sYZZX = SHUFFLE(1, 2, 2, 0);
constexpr int sYZZY = SHUFFLE(1, 2, 2, 1);
constexpr int sYZZZ = SHUFFLE(1, 2, 2, 2);
constexpr int sYZZW = SHUFFLE(1, 2, 2, 3);
constexpr int sYZWX = SHUFFLE(1, 2, 3, 0);
constexpr int sYZWY = SHUFFLE(1, 2, 3, 1);
constexpr int sYZWZ = SHUFFLE(1, 2, 3, 2);
constexpr int sYZWW = SHUFFLE(1, 2, 3, 3);
constexpr int sYWXX = SHUFFLE(1, 3, 0, 0);
constexpr int sYWXY = SHUFFLE(1, 3, 0, 1);
constexpr int sYWXZ = SHUFFLE(1, 3, 0, 2);
constexpr int sYWXW = SHUFFLE(1, 3, 0, 3);
constexpr int sYWYX = SHUFFLE(1, 3, 1, 0);
constexpr int sYWYY = SHUFFLE(1, 3, 1, 1);
constexpr int sYWYZ = SHUFFLE(1, 3, 1, 2);
constexpr int sYWYW = SHUFFLE(1, 3, 1, 3);
constexpr int sYWZX = SHUFFLE(1, 3, 2, 0);
constexpr int sYWZY = SHUFFLE(1, 3, 2, 1);
constexpr int sYWZZ = SHUFFLE(1, 3, 2, 2);
constexpr int sYWZW = SHUFFLE(1, 3, 2, 3);
constexpr int sYWWX = SHUFFLE(1, 3, 3, 0);
constexpr int sYWWY = SHUFFLE(1, 3, 3, 1);
constexpr int sYWWZ = SHUFFLE(1, 3, 3, 2);
constexpr int sYWWW = SHUFFLE(1, 3, 3, 3);
constexpr int sZXXX = SHUFFLE(2, 0, 0, 0);
constexpr int sZXXY = SHUFFLE(2, 0, 0, 1);
constexpr int sZXXZ = SHUFFLE(2, 0, 0, 2);
constexpr int sZXXW = SHUFFLE(2, 0, 0, 3);
constexpr int sZXYX = SHUFFLE(2, 0, 1, 0);
constexpr int sZXYY = SHUFFLE(2, 0, 1, 1);
constexpr int sZXYZ = SHUFFLE(2, 0, 1, 2);
constexpr int sZXYW = SHUFFLE(2, 0, 1, 3);
constexpr int sZXZX = SHUFFLE(2, 0, 2, 0);
constexpr int sZXZY = SHUFFLE(2, 0, 2, 1);
constexpr int sZXZZ = SHUFFLE(2, 0, 2, 2);
constexpr int sZXZW = SHUFFLE(2, 0, 2, 3);
constexpr int sZXWX = SHUFFLE(2, 0, 3, 0);
constexpr int sZXWY = SHUFFLE(2, 0, 3, 1);
constexpr int sZXWZ = SHUFFLE(2, 0, 3, 2);
constexpr int sZXWW = SHUFFLE(2, 0, 3, 3);
constexpr int sZYXX = SHUFFLE(2, 1, 0, 0);
constexpr int sZYXY = SHUFFLE(2, 1, 0, 1);
constexpr int sZYXZ = SHUFFLE(2, 1, 0, 2);
constexpr int sZYXW = SHUFFLE(2, 1, 0, 3);
constexpr int sZYYX = SHUFFLE(2, 1, 1, 0);
constexpr int sZYYY = SHUFFLE(2, 1, 1, 1);
constexpr int sZYYZ = SHUFFLE(2, 1, 1, 2);
constexpr int sZYYW = SHUFFLE(2, 1, 1, 3);
constexpr int sZYZX = SHUFFLE(2, 1, 2, 0);
constexpr int sZYZY = SHUFFLE(2, 1, 2, 1);
constexpr int sZYZZ = SHUFFLE(2, 1, 2, 2);
constexpr int sZYZW = SHUFFLE(2, 1, 2, 3);
constexpr int sZYWX = SHUFFLE(2, 1, 3, 0);
constexpr int sZYWY = SHUFFLE(2, 1, 3, 1);
constexpr int sZYWZ = SHUFFLE(2, 1, 3, 2);
constexpr int sZYWW = SHUFFLE(2, 1, 3, 3);
constexpr int sZZXX = SHUFFLE(2, 2, 0, 0);
constexpr int sZZXY = SHUFFLE(2, 2, 0, 1);
constexpr int sZZXZ = SHUFFLE(2, 2, 0, 2);
constexpr int sZZXW = SHUFFLE(2, 2, 0, 3);
constexpr int sZZYX = SHUFFLE(2, 2, 1, 0);
constexpr int sZZYY = SHUFFLE(2, 2, 1, 1);
constexpr int sZZYZ = SHUFFLE(2, 2, 1, 2);
constexpr int sZZYW = SHUFFLE(2, 2, 1, 3);
constexpr int sZZZX = SHUFFLE(2, 2, 2, 0);
constexpr int sZZZY = SHUFFLE(2, 2, 2, 1);
constexpr int sZZZZ = SHUFFLE(2, 2, 2, 2);
constexpr int sZZZW = SHUFFLE(2, 2, 2, 3);
constexpr int sZZWX = SHUFFLE(2, 2, 3, 0);
constexpr int sZZWY = SHUFFLE(2, 2, 3, 1);
constexpr int sZZWZ = SHUFFLE(2, 2, 3, 2);
constexpr int sZZWW = SHUFFLE(2, 2, 3, 3);
constexpr int sZWXX = SHUFFLE(2, 3, 0, 0);
constexpr int sZWXY = SHUFFLE(2, 3, 0, 1);
constexpr int sZWXZ = SHUFFLE(2, 3, 0, 2);
constexpr int sZWXW = SHUFFLE(2, 3, 0, 3);
constexpr int sZWYX = SHUFFLE(2, 3, 1, 0);
constexpr int sZWYY = SHUFFLE(2, 3, 1, 1);
constexpr int sZWYZ = SHUFFLE(2, 3, 1, 2);
constexpr int sZWYW = SHUFFLE(2, 3, 1, 3);
constexpr int sZWZX = SHUFFLE(2, 3, 2, 0);
constexpr int sZWZY = SHUFFLE(2, 3, 2, 1);
constexpr int sZWZZ = SHUFFLE(2, 3, 2, 2);
constexpr int sZWZW = SHUFFLE(2, 3, 2, 3);
constexpr int sZWWX = SHUFFLE(2, 3, 3, 0);
constexpr int sZWWY = SHUFFLE(2, 3, 3, 1);
constexpr int sZWWZ = SHUFFLE(2, 3, 3, 2);
constexpr int sZWWW = SHUFFLE(2, 3, 3, 3);
constexpr int sWXXX = SHUFFLE(3, 0, 0, 0);
constexpr int sWXXY = SHUFFLE(3, 0, 0, 1);
constexpr int sWXXZ = SHUFFLE(3, 0, 0, 2);
constexpr int sWXXW = SHUFFLE(3, 0, 0, 3);
constexpr int sWXYX = SHUFFLE(3, 0, 1, 0);
constexpr int sWXYY = SHUFFLE(3, 0, 1, 1);
constexpr int sWXYZ = SHUFFLE(3, 0, 1, 2);
constexpr int sWXYW = SHUFFLE(3, 0, 1, 3);
constexpr int sWXZX = SHUFFLE(3, 0, 2, 0);
constexpr int sWXZY = SHUFFLE(3, 0, 2, 1);
constexpr int sWXZZ = SHUFFLE(3, 0, 2, 2);
constexpr int sWXZW = SHUFFLE(3, 0, 2, 3);
constexpr int sWXWX = SHUFFLE(3, 0, 3, 0);
constexpr int sWXWY = SHUFFLE(3, 0, 3, 1);
constexpr int sWXWZ = SHUFFLE(3, 0, 3, 2);
constexpr int sWXWW = SHUFFLE(3, 0, 3, 3);
constexpr int sWYXX = SHUFFLE(3, 1, 0, 0);
constexpr int sWYXY = SHUFFLE(3, 1, 0, 1);
constexpr int sWYXZ = SHUFFLE(3, 1, 0, 2);
constexpr int sWYXW = SHUFFLE(3, 1, 0, 3);
constexpr int sWYYX = SHUFFLE(3, 1, 1, 0);
constexpr int sWYYY = SHUFFLE(3, 1, 1, 1);
constexpr int sWYYZ = SHUFFLE(3, 1, 1, 2);
constexpr int sWYYW = SHUFFLE(3, 1, 1, 3);
constexpr int sWYZX = SHUFFLE(3, 1, 2, 0);
constexpr int sWYZY = SHUFFLE(3, 1, 2, 1);
constexpr int sWYZZ = SHUFFLE(3, 1, 2, 2);
constexpr int sWYZW = SHUFFLE(3, 1, 2, 3);
constexpr int sWYWX = SHUFFLE(3, 1, 3, 0);
constexpr int sWYWY = SHUFFLE(3, 1, 3, 1);
constexpr int sWYWZ = SHUFFLE(3, 1, 3, 2);
constexpr int sWYWW = SHUFFLE(3, 1, 3, 3);
constexpr int sWZXX = SHUFFLE(3, 2, 0, 0);
constexpr int sWZXY = SHUFFLE(3, 2, 0, 1);
constexpr int sWZXZ = SHUFFLE(3, 2, 0, 2);
constexpr int sWZXW = SHUFFLE(3, 2, 0, 3);
constexpr int sWZYX = SHUFFLE(3, 2, 1, 0);
constexpr int sWZYY = SHUFFLE(3, 2, 1, 1);
constexpr int sWZYZ = SHUFFLE(3, 2, 1, 2);
constexpr int sWZYW = SHUFFLE(3, 2, 1, 3);
constexpr int sWZZX = SHUFFLE(3, 2, 2, 0);
constexpr int sWZZY = SHUFFLE(3, 2, 2, 1);
constexpr int sWZZZ = SHUFFLE(3, 2, 2, 2);
constexpr int sWZZW = SHUFFLE(3, 2, 2, 3);
constexpr int sWZWX = SHUFFLE(3, 2, 3, 0);
constexpr int sWZWY = SHUFFLE(3, 2, 3, 1);
constexpr int sWZWZ = SHUFFLE(3, 2, 3, 2);
constexpr int sWZWW = SHUFFLE(3, 2, 3, 3);
constexpr int sWWXX = SHUFFLE(3, 3, 0, 0);
constexpr int sWWXY = SHUFFLE(3, 3, 0, 1);
constexpr int sWWXZ = SHUFFLE(3, 3, 0, 2);
constexpr int sWWXW = SHUFFLE(3, 3, 0, 3);
constexpr int sWWYX = SHUFFLE(3, 3, 1, 0);
constexpr int sWWYY = SHUFFLE(3, 3, 1, 1);
constexpr int sWWYZ = SHUFFLE(3, 3, 1, 2);
constexpr int sWWYW = SHUFFLE(3, 3, 1, 3);
constexpr int sWWZX = SHUFFLE(3, 3, 2, 0);
constexpr int sWWZY = SHUFFLE(3, 3, 2, 1);
constexpr int sWWZZ = SHUFFLE(3, 3, 2, 2);
constexpr int sWWZW = SHUFFLE(3, 3, 2, 3);
constexpr int sWWWX = SHUFFLE(3, 3, 3, 0);
constexpr int sWWWY = SHUFFLE(3, 3, 3, 1);
constexpr int sWWWZ = SHUFFLE(3, 3, 3, 2);
constexpr int sWWWW = SHUFFLE(3, 3, 3, 3);

HK_NAMESPACE_END
