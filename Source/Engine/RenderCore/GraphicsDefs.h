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

#include <Core/Public/Ref.h>

struct SDL_Window;

namespace RenderCore {

/**

Base interface

*/
class IObjectInterface
{
public:
    IObjectInterface()
    {
        static uint32_t UnqiueIdGen = 0;
        RefCount = 0;
        UID = ++UnqiueIdGen;
    }

    virtual ~IObjectInterface() {}

    /** Non-copyable pattern */
    IObjectInterface( IObjectInterface const & ) = delete;

    /** Non-copyable pattern */
    IObjectInterface & operator=( IObjectInterface const & ) = delete;

    uint32_t GetUID() const { return UID; }

    /** Add reference */
    void AddRef();

    /** Remove reference */
    void RemoveRef();

    /** Set weakref counter. Used by TWeakRef */
    void SetWeakRefCounter( SWeakRefCounter * _RefCounter ) { WeakRefCounter = _RefCounter; }

    /** Get weakref counter. Used by TWeakRef */
    SWeakRefCounter * GetWeakRefCounter() { return WeakRefCounter; }

    void * operator new( size_t size )
    {
        return GZoneMemory.Alloc( size );
    }

    void operator delete( void * p )
    {
        GZoneMemory.Free( p );
    }

private:
    int RefCount;
    uint32_t UID;
    SWeakRefCounter * WeakRefCounter = nullptr;
};

inline void IObjectInterface::AddRef()
{
    ++RefCount;
}

inline void IObjectInterface::RemoveRef()
{
    if ( RefCount == 1 ) {
        RefCount = 0;
        delete this;
    } else if ( RefCount > 0 ) {
        --RefCount;
    }
}

class IDeviceObject : public IObjectInterface
{
public:
    IDeviceObject()
        : Handle( nullptr )
    {
    }

    bool IsValid() const { return Handle != nullptr; }

    void * GetHandle() const { return Handle; }

    uint64_t GetHandleNativeGL() const { return HandleUI64; }

protected:    
    void SetHandle( void * InHandle )
    {
        Handle = InHandle;
    }

    void SetHandleNativeGL( uint64_t NativeHandle )
    {
        HandleUI64 = NativeHandle;
    }

private:
    union
    {
        void * Handle;

        /** Use 64-bit integer for bindless handle compatibility */
        uint64_t HandleUI64;
    };
};

struct SAllocatorCallback
{
    void * ( *Allocate )( size_t _BytesCount );
    void ( *Deallocate )( void * _Bytes );
};

typedef int ( *HashCallback )( const unsigned char * _Data, int _Size );

/**

Base limits

*/
enum
{
    // TODO: get from hardware
    MAX_VERTEX_BUFFER_SLOTS = 32,
    MAX_BUFFER_SLOTS        = 32,
    MAX_SAMPLER_SLOTS       = 32, // MaxCombinedTextureImageUnits
    MAX_IMAGE_SLOTS         = 32,
    MAX_COLOR_ATTACHMENTS   = 8,
    MAX_SUBPASS_COUNT       = 16,
    MAX_VERTEX_BINDINGS     = 16,
    MAX_VERTEX_ATTRIBS      = 16
};

enum COLOR_CLAMP : uint8_t
{
    /** Clamping is always off, no matter what the format​ or type​ parameters of the read pixels call. */
    COLOR_CLAMP_OFF,
    /** Clamping is always on, no matter what the format​ or type​ parameters of the read pixels call. */
    COLOR_CLAMP_ON,
    /** Clamping is only on if the type of the image being read is a normalized signed or unsigned value. */
    COLOR_CLAMP_FIXED_ONLY
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

struct SRect2D
{
    uint16_t X;
    uint16_t Y;
    uint16_t Width;
    uint16_t Height;
};

enum DATA_FORMAT : uint8_t
{
    FORMAT_BYTE1,
    FORMAT_BYTE2,
    FORMAT_BYTE3,
    FORMAT_BYTE4,

    FORMAT_UBYTE1,
    FORMAT_UBYTE2,
    FORMAT_UBYTE3,
    FORMAT_UBYTE4,

    FORMAT_SHORT1,
    FORMAT_SHORT2,
    FORMAT_SHORT3,
    FORMAT_SHORT4,

    FORMAT_USHORT1,
    FORMAT_USHORT2,
    FORMAT_USHORT3,
    FORMAT_USHORT4,

    FORMAT_INT1,
    FORMAT_INT2,
    FORMAT_INT3,
    FORMAT_INT4,

    FORMAT_UINT1,
    FORMAT_UINT2,
    FORMAT_UINT3,
    FORMAT_UINT4,

    FORMAT_HALF1,
    FORMAT_HALF2,
    FORMAT_HALF3,
    FORMAT_HALF4,

    FORMAT_FLOAT1,
    FORMAT_FLOAT2,
    FORMAT_FLOAT3,
    FORMAT_FLOAT4
};

}
