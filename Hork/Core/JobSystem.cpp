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

    void DispatchBarrier(Barrier* barrier, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, DispatchFunction function, uint32_t maxJobs)
    {
        uint32_t totalGroups = numGroupsX * numGroupsY * numGroupsZ;
        if (totalGroups == 0)
            return;

        maxJobs = Math::Clamp(maxJobs, 1u, MAX_JOBS);

        uint32_t groupsPerJob = std::max(1u, (totalGroups + maxJobs - 1) / maxJobs);

        // Гарантируем, что не создадим больше maxJobs
        if (groupsPerJob == 1 && totalGroups > maxJobs)
            groupsPerJob = (totalGroups + maxJobs - 1) / maxJobs;

        for (uint32_t startGroupIndex = 0; startGroupIndex < totalGroups; startGroupIndex += groupsPerJob)
        {
            uint32_t endGroupIndex = std::min(startGroupIndex + groupsPerJob, totalGroups);

            JobHandle job = CreateJob("DispatchGroup", Color4::sWhite(), 
                [startGroupIndex, endGroupIndex, numGroupsX, numGroupsY, numGroupsZ, function]()
                {
                    DispatchArgs args;
                    for (uint32_t groupIndex = startGroupIndex; groupIndex < endGroupIndex; ++groupIndex)
                    {
                        args.GroupX = groupIndex % numGroupsX;
                        args.GroupY = (groupIndex / numGroupsX) % numGroupsY;
                        args.GroupZ = groupIndex / (numGroupsX * numGroupsY);

                        function(args);
                    }
                });

            barrier->AddJob(job);
        }
    }

    void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, DispatchFunction function, uint32_t maxJobs)
    {
        if (numGroupsX == 0 || numGroupsY == 0 || numGroupsZ == 0)
            return;

        auto barrier = CreateBarrier();

        DispatchBarrier(barrier, numGroupsX, numGroupsY, numGroupsZ, std::move(function), maxJobs);

        WaitForJobs(barrier);
        DestroyBarrier(barrier);
    }

    JPH::JobSystem* GetImpl()
    {
        return g_JobSystemThreadPool.RawPtr();
    }

}

HK_NAMESPACE_END
