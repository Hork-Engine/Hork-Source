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

#pragma once

#include <Core/Ref.h>
#include <Core/String.h>

namespace RenderCore
{

class IDevice;

enum DEVICE_OBJECT_PROXY_TYPE : uint8_t
{
    DEVICE_OBJECT_TYPE_UNKNOWN,

    DEVICE_OBJECT_TYPE_IMMEDIATE_CONTEXT,

    DEVICE_OBJECT_TYPE_BUFFER,
    DEVICE_OBJECT_TYPE_BUFFER_VIEW,

    DEVICE_OBJECT_TYPE_TEXTURE,
    DEVICE_OBJECT_TYPE_TEXTURE_VIEW,

    DEVICE_OBJECT_TYPE_SPARSE_TEXTURE,

    DEVICE_OBJECT_TYPE_PIPELINE,
    DEVICE_OBJECT_TYPE_SHADER_MODULE,
    DEVICE_OBJECT_TYPE_TRANSFORM_FEEDBACK,
    DEVICE_OBJECT_TYPE_QUERY_POOL,
    DEVICE_OBJECT_TYPE_RESOURCE_TABLE,

    DEVICE_OBJECT_TYPE_SWAP_CHAIN,

    DEVICE_OBJECT_TYPE_WINDOW,

    DEVICE_OBJECT_TYPE_MAX
};

class IDeviceObject : public RefCounted
{
public:
    IDeviceObject(IDevice* pDevice, DEVICE_OBJECT_PROXY_TYPE ProxyType, bool bInternalDeviceObject = false);

    ~IDeviceObject();

    DEVICE_OBJECT_PROXY_TYPE GetProxyType() const { return ProxyType; }

    void SetDebugName(StringView _DebugName)
    {
#ifdef HK_DEBUG
        DebugName = _DebugName;
#endif
    }

    const char* GetDebugName() const
    {
#ifdef HK_DEBUG
        return DebugName.CStr();
#else
        return "";
#endif
    }

#ifdef HK_DEBUG
    IDeviceObject* GetNext_DEBUG()
    {
        return Next;
    }
#endif

    uint32_t GetUID() const
    {
        return UID;
    }

    bool IsValid() const { return Handle != nullptr; }

    void* GetHandle() const { return Handle; }

    uint64_t GetHandleNativeGL() const { return HandleUI64; }

    IDevice* GetDevice() { return pDevice; }

protected:
    void SetHandle(void* InHandle)
    {
        Handle = InHandle;
    }

    void SetHandleNativeGL(uint64_t NativeHandle)
    {
        HandleUI64 = NativeHandle;
    }

private:
    uint32_t UID;
    union
    {
        void* Handle;

        /** Use 64-bit integer for bindless handle compatibility */
        uint64_t HandleUI64 = 0;
    };
    DEVICE_OBJECT_PROXY_TYPE ProxyType;
    IDevice* pDevice;
    bool bInternalDeviceObject;
#ifdef HK_DEBUG
    String DebugName;

    IDeviceObject* Next = nullptr;
    IDeviceObject* Prev = nullptr;
#endif
};

} // namespace RenderCore
