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

#include <Geometry/BV/BvAxisAlignedBox.h>

void BvAxisAlignedBox::GetVertices(Float3 _Vertices[8]) const
{
    _Vertices[0] = Float3(Mins.X, Maxs.Y, Mins.Z);
    _Vertices[1] = Float3(Maxs.X, Maxs.Y, Mins.Z);
    _Vertices[2] = Maxs;
    _Vertices[3] = Float3(Mins.X, Maxs.Y, Maxs.Z);
    _Vertices[4] = Mins;
    _Vertices[5] = Float3(Maxs.X, Mins.Y, Mins.Z);
    _Vertices[6] = Float3(Maxs.X, Mins.Y, Maxs.Z);
    _Vertices[7] = Float3(Mins.X, Mins.Y, Maxs.Z);
}
