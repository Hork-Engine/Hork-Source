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

#include "DeviceObject.h"
#include "Device.h"

#ifdef AN_DEBUG
#include <Core/Public/IntrusiveLinkedListMacro.h>
#endif

namespace RenderCore {

IDeviceObject::IDeviceObject( IDevice * Device )
    : Handle( nullptr )
{
    static uint32_t UnqiueIdGen = 0;
    UID = ++UnqiueIdGen;
#ifdef AN_DEBUG
    pDevice = Device;
    Next = Prev = nullptr;
    INTRUSIVE_ADD( this, Next, Prev, pDevice->ListHead, pDevice->ListTail );
#endif
}

IDeviceObject::~IDeviceObject()
{
#ifdef AN_DEBUG
    INTRUSIVE_REMOVE( this, Next, Prev, pDevice->ListHead, pDevice->ListTail );
#endif
}

}
