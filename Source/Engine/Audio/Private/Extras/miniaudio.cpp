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

#include <Core/Public/BaseTypes.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#define MINIAUDIO_IMPLEMENTATION
//#define MA_PREFER_SSE2
#include "miniaudio.h"

// The stb_vorbis implementation must come after the implementation of miniaudio.
#ifdef AN_COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4701 )
#endif

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#ifdef AN_COMPILER_MSVC
#pragma warning( pop )
#endif
