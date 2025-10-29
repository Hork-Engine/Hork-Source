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

#include "JobSystem.h"
#include "UniqueRef.h"
#include "Thread.h"
#include "Logger.h"

#include <Jolt/Core/JobSystemThreadPool.h>

HK_NAMESPACE_BEGIN

namespace JobSystem
{

    UniqueRef<JPH::JobSystemThreadPool> g_JobSystemThreadPool;

    void Initialize()
    {
        int numThreads = Math::Max(1, Thread::NumHardwareThreads - 1);
        LOG("Job system thread count {}\n", numThreads);

        g_JobSystemThreadPool = MakeUnique<JPH::JobSystemThreadPool>(MAX_JOBS, MAX_BARRIERS, numThreads);
    }

    void Deinitialize()
    {
        g_JobSystemThreadPool.Reset();
    }

    JobHandle CreateJob(const char *jobName, Color4 const& color, const JobFunction &jobFunction, uint32_t numDependencies)
    {
        HK_ASSERT(g_JobSystemThreadPool != nullptr);
        return g_JobSystemThreadPool->CreateJob(jobName, JPH::Color(color.GetDWord()), jobFunction, numDependencies);
    }

    Barrier* CreateBarrier()
    {
        HK_ASSERT(g_JobSystemThreadPool != nullptr);
        return g_JobSystemThreadPool->CreateBarrier();
    }

    void DestroyBarrier(Barrier* barrier)
    {
        HK_ASSERT(g_JobSystemThreadPool != nullptr);
        g_JobSystemThreadPool->DestroyBarrier(barrier);
    }

    void WaitForJobs(Barrier* barrier)
    {
        HK_ASSERT(g_JobSystemThreadPool != nullptr);
        g_JobSystemThreadPool->WaitForJobs(barrier);
    }

    int GetMaxConcurrency()
    {
        HK_ASSERT(g_JobSystemThreadPool != nullptr);
        return g_JobSystemThreadPool->GetMaxConcurrency();
    }

    void DispatchBarrier(Barrier* barrier, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, DispatchFunction function)
    {
        DispatchArgs args;

        for (uint32_t z = 0; z < numGroupsZ; ++z)
            for (uint32_t y = 0; y < numGroupsY; ++y)
                for (uint32_t x = 0; x < numGroupsX; ++x)
                {
                    args.GroupX = x;
                    args.GroupY = y;
                    args.GroupZ = z;

                    JobHandle job = CreateJob("DispatchGroup", Color4::sWhite(), [args, function]()
                        {
                            function(args);
                        });

                    barrier->AddJob(job);
                }
    }

    void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, DispatchFunction function)
    {
        auto barrier = CreateBarrier();

        DispatchBarrier(barrier, numGroupsX, numGroupsY, numGroupsZ, function);

        WaitForJobs(barrier);
        DestroyBarrier(barrier);
    }

    JPH::JobSystem* GetImpl()
    {
        return g_JobSystemThreadPool.RawPtr();
    }

}

HK_NAMESPACE_END
