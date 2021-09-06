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

#include "DeviceObject.h"

namespace RenderCore
{

enum QUERY_TYPE : uint8_t
{
    QUERY_TYPE_SAMPLES_PASSED,
    QUERY_TYPE_ANY_SAMPLES_PASSED,
    QUERY_TYPE_ANY_SAMPLES_PASSED_CONSERVATIVE,
    QUERY_TYPE_TIME_ELAPSED,
    QUERY_TYPE_TIMESTAMP,
    QUERY_TYPE_PRIMITIVES_GENERATED,
    QUERY_TYPE_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,

    QUERY_TYPE_MAX
};

enum QUERY_RESULT_FLAGS : uint32_t
{
    QUERY_RESULT_64_BIT                = 1,
    QUERY_RESULT_WAIT_BIT              = 2,
    QUERY_RESULT_WITH_AVAILABILITY_BIT = 4,
    //QUERY_RESULT_PARTIAL_BIT = 8  // TODO?
};

struct SQueryPoolDesc
{
    QUERY_TYPE QueryType;
    uint32_t   PoolSize;
};

class IQueryPool : public IDeviceObject
{
public:
    IQueryPool(IDevice* pDevice) :
        IDeviceObject(pDevice, DEVICE_OBJECT_TYPE_QUERY_POOL) {}

    QUERY_TYPE GetQueryType() const { return QueryType; }

    uint32_t GetPoolSize() const { return PoolSize; }

protected:
    QUERY_TYPE QueryType;
    uint32_t   PoolSize;
};

} // namespace RenderCore
