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

#include <Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

void BvAxisAlignedBox::GetVertices(Float3 vertices[8]) const
{
    vertices[0]  = Float3(Mins.X, Maxs.Y, Mins.Z);
    vertices[1]  = Float3(Maxs.X, Maxs.Y, Mins.Z);
    vertices[2]  = Maxs;
    vertices[3]  = Float3(Mins.X, Maxs.Y, Maxs.Z);
    vertices[4]  = Mins;
    vertices[5]  = Float3(Maxs.X, Mins.Y, Mins.Z);
    vertices[6]  = Float3(Maxs.X, Mins.Y, Maxs.Z);
    vertices[7]  = Float3(Mins.X, Mins.Y, Maxs.Z);
}

void BvAxisAlignedBox::GetFaceVertices(int faceNum, Float3 vertices[4]) const
{
    switch (faceNum)
    {
        case 0:
            // +X
            vertices[0] = Float3(Maxs.X, Maxs.Y, Mins.Z);
            vertices[1] = Float3(Maxs.X, Maxs.Y, Maxs.Z);
            vertices[2] = Float3(Maxs.X, Mins.Y, Maxs.Z);
            vertices[3] = Float3(Maxs.X, Mins.Y, Mins.Z);
            break;
        case 1:
            // -X
            vertices[0] = Float3(Mins.X, Maxs.Y, Maxs.Z);
            vertices[1] = Float3(Mins.X, Maxs.Y, Mins.Z);
            vertices[2] = Float3(Mins.X, Mins.Y, Mins.Z);
            vertices[3] = Float3(Mins.X, Mins.Y, Maxs.Z);
            break;
        case 2:
            // +Y
            vertices[0] = Float3(Mins.X, Maxs.Y, Maxs.Z);
            vertices[1] = Float3(Maxs.X, Maxs.Y, Maxs.Z);
            vertices[2] = Float3(Maxs.X, Maxs.Y, Mins.Z);
            vertices[3] = Float3(Mins.X, Maxs.Y, Mins.Z);
            break;
        case 3:
            // -Y
            vertices[0] = Float3(Maxs.X, Mins.Y, Maxs.Z);
            vertices[1] = Float3(Mins.X, Mins.Y, Maxs.Z);
            vertices[2] = Float3(Mins.X, Mins.Y, Mins.Z);
            vertices[3] = Float3(Maxs.X, Mins.Y, Mins.Z);
            break;
        case 4:
            // +Z
            vertices[0] = Float3(Mins.X, Mins.Y, Maxs.Z);
            vertices[1] = Float3(Maxs.X, Mins.Y, Maxs.Z);
            vertices[2] = Float3(Maxs.X, Maxs.Y, Maxs.Z);
            vertices[3] = Float3(Mins.X, Maxs.Y, Maxs.Z);
            break;
        case 5:
            // -Z
            vertices[0] = Float3(Maxs.X, Mins.Y, Mins.Z);
            vertices[1] = Float3(Mins.X, Mins.Y, Mins.Z);
            vertices[2] = Float3(Mins.X, Maxs.Y, Mins.Z);
            vertices[3] = Float3(Maxs.X, Maxs.Y, Mins.Z);
            break;
    }
}

HK_NAMESPACE_END
