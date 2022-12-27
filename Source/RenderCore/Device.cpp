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

#include "Device.h"
#include "OpenGL45/DeviceGLImpl.h"

HK_NAMESPACE_BEGIN

namespace RenderCore
{

IDevice::~IDevice()
{
#ifdef HK_DEBUG
    for (IDeviceObject* pObject = ListHead; pObject; pObject=pObject->GetNext_DEBUG())
    {
        LOG("Uninitialized resource: '{}'\n", pObject->GetDebugName());
    }
    HK_ASSERT(ListHead == nullptr);

    TArray<const char*, DEVICE_OBJECT_TYPE_MAX> name =
        {
            "DEVICE_OBJECT_TYPE_UNKNOWN",

            "DEVICE_OBJECT_TYPE_IMMEDIATE_CONTEXT",

            "DEVICE_OBJECT_TYPE_BUFFER",
            "DEVICE_OBJECT_TYPE_BUFFER_VIEW",

            "DEVICE_OBJECT_TYPE_TEXTURE",
            "DEVICE_OBJECT_TYPE_TEXTURE_VIEW",

            "DEVICE_OBJECT_TYPE_SPARSE_TEXTURE",

            "DEVICE_OBJECT_TYPE_PIPELINE",
            "DEVICE_OBJECT_TYPE_SHADER_MODULE",
            "DEVICE_OBJECT_TYPE_TRANSFORM_FEEDBACK",
            "DEVICE_OBJECT_TYPE_QUERY_POOL",
            "DEVICE_OBJECT_TYPE_RESOURCE_TABLE",

            "DEVICE_OBJECT_TYPE_SWAP_CHAIN",
    
            "DEVICE_OBJECT_TYPE_WINDOW"};

    for (int i = 0; i < DEVICE_OBJECT_TYPE_MAX; i++)
    {
        LOG("Object count {}: {}\n", name[i], GetObjectCount((DEVICE_OBJECT_PROXY_TYPE)i));
    }
#endif
}

void CreateLogicalDevice(const char*              Backend,
                         AllocatorCallback const* pAllocator,
                         TRef<IDevice>*           ppDevice)
{
    if (!Platform::Stricmp(Backend, "OpenGL 4.5"))
    {
        *ppDevice = MakeRef<DeviceGLImpl>(pAllocator);
    }
    else
    {
        *ppDevice = nullptr;
    }
}

} // namespace RenderCore

HK_NAMESPACE_END
