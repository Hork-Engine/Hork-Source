/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "QueryGLImpl.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

namespace RenderCore
{

AQueryPoolGLImpl::AQueryPoolGLImpl(ADeviceGLImpl* pDevice, SQueryPoolDesc const& Desc) :
    IQueryPool(pDevice)
{
    AN_ASSERT(Desc.PoolSize > 0);

    IdPool    = nullptr;
    QueryType = Desc.QueryType;
    PoolSize  = Desc.PoolSize;

    SAllocatorCallback const& allocator = pDevice->GetAllocator();

    IdPool = (unsigned int*)allocator.Allocate(sizeof(*IdPool) * PoolSize);

    if (!IdPool)
    {
        GLogger.Printf("AQueryPoolGLImpl::ctor: out of memory\n");
        return;
    }

    // TODO: create queries for each context
    glCreateQueries(TableQueryTarget[QueryType], PoolSize, IdPool); // 4.5


    SetHandleNativeGL(IdPool[0]);
}

AQueryPoolGLImpl::~AQueryPoolGLImpl()
{
    if (!IdPool)
    {
        return;
    }

    glDeleteQueries(PoolSize, IdPool); // 4.5

    GetDevice()->GetAllocator().Deallocate(IdPool);
}

} // namespace RenderCore
