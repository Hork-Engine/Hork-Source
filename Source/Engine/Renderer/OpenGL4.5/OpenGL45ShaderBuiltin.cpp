/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "OpenGL45ShaderSource.h"

namespace OpenGL45 {

const char * UniformStr =
"layout( binding = 0, std140 ) uniform UniformBuffer0\n"
"{\n"
"    vec4 Timers;\n"
"    vec4 ViewPostion;\n"
"};\n"
"layout( binding = 1, std140 ) uniform UniformBuffer1\n"
"{\n"
"    mat4 ProjectTranslateViewMatrix;\n"
"    vec4 ModelNormalToViewSpace0;\n"
"    vec4 ModelNormalToViewSpace1;\n"
"    vec4 ModelNormalToViewSpace2;\n"
"    vec4 LightmapOffset;\n"
"    vec4 uaddr_0;\n"
"    vec4 uaddr_1;\n"
"    vec4 uaddr_2;\n"
"    vec4 uaddr_3;\n"
"};\n";

}
