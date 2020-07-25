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

#pragma once

#include <RenderCore/Buffer.h>

namespace RenderCore {

class ADeviceGLImpl;

class ABufferGLImpl final : public IBuffer
{
    friend class ADeviceGLImpl;
    friend class AImmediateContextGLImpl;

public:
    ABufferGLImpl( ADeviceGLImpl * _Device, SBufferCreateInfo const & _CreateInfo, const void * _SysMem = nullptr );
    ~ABufferGLImpl();

    bool Realloc( size_t _NewByteLength, const void * _SysMem = nullptr ) override;

    bool Orphan() override;

    void Read( void * _SysMem ) override;

    void ReadRange( size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) override;

    void Write( const void * _SysMem ) override;

    void WriteRange( size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) override;

    void * MapRange( size_t _RangeOffset,
                     size_t _RangeLength,
                     MAP_TRANSFER _ClientServerTransfer,
                     MAP_INVALIDATE _Invalidate = MAP_NO_INVALIDATE,
                     MAP_PERSISTENCE _Persistence = MAP_NON_PERSISTENT,
                     bool _FlushExplicit = false,
                     bool _Unsynchronized = false ) override;

    void * Map( MAP_TRANSFER _ClientServerTransfer,
                MAP_INVALIDATE _Invalidate = MAP_NO_INVALIDATE,
                MAP_PERSISTENCE _Persistence = MAP_NON_PERSISTENT,
                bool _FlushExplicit = false,
                bool _Unsynchronized = false ) override;

    void Unmap() override;

    void * GetMapPointer() override;

    void Invalidate() override;

    void InvalidateRange( size_t _RangeOffset, size_t _RangeLength ) override;

    void FlushMappedRange( size_t _RangeOffset, size_t _RangeLength ) override;

private:
    ADeviceGLImpl * pDevice;
};

}
