/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

namespace GHI {

#define	GHI_STRUCT_OFS(type, a)  offsetof( type, a )

class NonCopyable {
public:
    NonCopyable( NonCopyable const & ) = delete;
    NonCopyable & operator=( NonCopyable const & ) = delete;
    NonCopyable() {}
};

class IObjectInterface {
public:
    virtual ~IObjectInterface() {}
};

struct AllocatorCallback {
    void * ( *Allocate )( size_t _BytesCount );
    void ( *Deallocate )( void * _Bytes );
};

typedef int ( *HashCallback )( const unsigned char * _Data, int _Size );

enum {
    // TODO: get from hardware
    MAX_VERTEX_BUFFER_SLOTS = 32,
    MAX_BUFFER_SLOTS        = 32,
    MAX_SAMPLER_SLOTS       = 16,
    MAX_COLOR_ATTACHMENTS   = 8,
    MAX_SUBPASS_COUNT       = 16,
    MAX_VERTEX_BINDINGS     = 16,
    MAX_VERTEX_ATTRIBS      = 16
};

enum COLOR_CLAMP : uint8_t
{
    COLOR_CLAMP_OFF,    // Clamping is always off, no matter what the format​ or type​ parameters of the read pixels call.
    COLOR_CLAMP_ON,     // Clamping is always on, no matter what the format​ or type​ parameters of the read pixels call.
    COLOR_CLAMP_FIXED_ONLY     // Clamping is only on if the type of the image being read is a normalized signed or unsigned value.
};

enum COMPARISON_FUNCTION : uint8_t
{
    CMPFUNC_NEVER          = 0,
    CMPFUNC_LESS           = 1,
    CMPFUNC_EQUAL          = 2,
    CMPFUNC_LEQUAL         = 3,
    CMPFUNC_GREATER        = 4,
    CMPFUNC_NOT_EQUAL      = 5,
    CMPFUNC_GEQUAL         = 6,
    CMPFUNC_ALWAYS         = 7
};

struct Rect2D {
    uint16_t X;
    uint16_t Y;
    uint16_t Width;
    uint16_t Height;
};

void LogPrintf( const char * _Format, ... );

}
