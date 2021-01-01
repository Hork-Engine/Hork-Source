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

#pragma once

#include <Core/Public/Ref.h>

#ifdef AN_DEBUG
#include <Core/Public/String.h>
#endif

namespace RenderCore {

class IDevice;

class IDeviceObject : public ARefCounted
{
public:
    IDeviceObject( IDevice * Device );

    ~IDeviceObject();

    void SetDebugName( const char * _DebugName )
    {
#ifdef AN_DEBUG
        DebugName = _DebugName;
#endif
    }

    const char * GetDebugName() const
    {
#ifdef AN_DEBUG
        return DebugName.CStr();
#else
        return "";
#endif
    }

#ifdef AN_DEBUG
    IDeviceObject * GetNext_DEBUG() { return Next; }
#endif

    uint32_t GetUID() const { return UID; }

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
    uint32_t UID;
    union
    {
        void * Handle;

        /** Use 64-bit integer for bindless handle compatibility */
        uint64_t HandleUI64;
    };
#ifdef AN_DEBUG
    AString DebugName;

    IDevice * pDevice;

    IDeviceObject * Next;
    IDeviceObject * Prev;
#endif
};

}
