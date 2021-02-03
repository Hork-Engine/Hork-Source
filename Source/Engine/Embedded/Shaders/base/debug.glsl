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

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG_RENDER_MODE
#define DEBUG_FULLBRIGHT   1
#define DEBUG_NORMAL       2
#define DEBUG_METALLIC     3
#define DEBUG_ROUGHNESS    4
#define DEBUG_AMBIENT      5
#define DEBUG_EMISSION     6
#define DEBUG_LIGHTMAP     7
#define DEBUG_VERTEX_LIGHT 8
#define DEBUG_DIRLIGHT     9
#define DEBUG_POINTLIGHT   10
#define DEBUG_TEXCOORDS    11
#define DEBUG_TEXNORMAL    12
#define DEBUG_TBN_NORMAL   13
#define DEBUG_TBN_TANGENT  14
#define DEBUG_TBN_BINORMAL 15
#define DEBUG_SPECULAR     16
#define DEBUG_BLOOM        17
#define DEBUG_BLOOMTEX1    18
#define DEBUG_BLOOMTEX2    19
#define DEBUG_BLOOMTEX3    20
#define DEBUG_BLOOMTEX4    21
#define DEBUG_EXPOSURE     22
#define DEBUG_AMBIENT_LIGHT 23
#define DEBUG_LIGHT_CASCADES 24
#define DEBUG_VT_BORDERS   25
#define DEBUG_VELOCITY     26
#endif

#endif // DEBUG_H
