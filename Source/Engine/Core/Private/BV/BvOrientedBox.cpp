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

#include <Core/Public/BV/BvOrientedBox.h>

void BvOrientedBox::GetVertices( Float3 _Vertices[8] ) const {
    _Vertices[0] = Orient * Float3( -HalfSize.X, HalfSize.Y, -HalfSize.Z ) + Center;
    _Vertices[1] = Orient * Float3(  HalfSize.X, HalfSize.Y, -HalfSize.Z ) + Center;
    _Vertices[2] = Orient * Float3(  HalfSize.X, HalfSize.Y,  HalfSize.Z ) + Center;
    _Vertices[3] = Orient * Float3( -HalfSize.X, HalfSize.Y,  HalfSize.Z ) + Center;
    _Vertices[4] = Orient * Float3( -HalfSize.X, -HalfSize.Y, -HalfSize.Z ) + Center;
    _Vertices[5] = Orient * Float3(  HalfSize.X, -HalfSize.Y, -HalfSize.Z ) + Center;
    _Vertices[6] = Orient * Float3(  HalfSize.X, -HalfSize.Y,  HalfSize.Z ) + Center;
    _Vertices[7] = Orient * Float3( -HalfSize.X, -HalfSize.Y,  HalfSize.Z ) + Center;
}
