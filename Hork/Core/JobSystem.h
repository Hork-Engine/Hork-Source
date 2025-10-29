/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystem.h>

#include "Color.h"

HK_NAMESPACE_BEGIN

namespace JobSystem
{
    /// Maximum amount of jobs to allow
    constexpr int MAX_JOBS      = 2048;

    /// Maximum amount of barriers to allow
    constexpr int MAX_BARRIERS  = 8;

    using JobFunction = std::function<void()>;

    using JPH::JobHandle;
    using Barrier = JPH::JobSystem::Barrier;

    struct DispatchArgs
    {
        uint32_t GroupX;
        uint32_t GroupY;
        uint32_t GroupZ;
    };

    using DispatchFunction = std::function<void(DispatchArgs)>;

    void            Initialize();
    void            Deinitialize();
    JobHandle       CreateJob(const char *jobName, Color4 const& color, const JobFunction &jobFunction, uint32_t numDependencies = 0);
    Barrier*        CreateBarrier();
    void            DestroyBarrier(Barrier* barrier);
    void            WaitForJobs(Barrier* barrier);
    int             GetMaxConcurrency();
    void            DispatchBarrier(Barrier* barrier, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, DispatchFunction function);
    void            Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, DispatchFunction function);
    JPH::JobSystem* GetImpl();
}

HK_NAMESPACE_END
