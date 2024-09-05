/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "Containers/Vector.h"
#include "Ref.h"

HK_NAMESPACE_BEGIN

//#define HK_ACTIVE_THREADS_COUNTERS

/// Job for job list
struct AsyncJob
{
    /// Callback for the job
    void (*Callback)(void*);
    /// Data that will be passed for the job
    void* Data;
    /// Pointer to the next job in job list
    AsyncJob* Next;
};

class AsyncJobManager;

/// Job list
class AsyncJobList final : public Noncopyable
{
    friend class AsyncJobManager;

public:
    /// Set job pool size (max jobs for the list)
    void SetMaxParallelJobs(int maxParallelJobs);

    /// Get job pool size
    int GetMaxParallelJobs() const;

    /// Add job to the list
    void AddJob(void (*callback)(void*), void* data);

    /// Submit jobs to worker threads
    void Submit();

    /// Block current thread while jobs are in working threads
    void Wait();

    /// Submit jobs to worker threads and block current thread while jobs are in working threads
    void SubmitAndWait();

private:
    AsyncJobList();
    ~AsyncJobList();

    AsyncJobManager* m_JobManager{nullptr};

    SmallVector<AsyncJob, 1024> m_JobPool;
    AsyncJob*                   m_JobList{nullptr};
    int                         m_NumPendingJobs{0};

    AsyncJob* m_SubmittedJobs{nullptr};
    Mutex     m_SubmitSync;

    AtomicInt m_SubmittedJobsCount{0};
    AtomicInt m_FetchCount{0};
    //AtomicInt m_FetchLock{0};

    SyncEvent m_EventDone;
    bool      m_IsSignalled{false}; // FIXME: must be atomic?
};

HK_FORCEINLINE int AsyncJobList::GetMaxParallelJobs() const
{
    return m_JobPool.Capacity();
}

/// Job manager
class AsyncJobManager final : public Noncopyable
{
public:
    static constexpr int MAX_WORKER_THREADS = 4;
    static constexpr int MAX_JOB_LISTS      = 4;

    /// Initialize job manager. Set worker threads count and create job lists
    AsyncJobManager(int numWorkerThreads, int numJobLists);

    ~AsyncJobManager();

    void SubmitJobList(AsyncJobList* jobList);

    /// Wakeup worker threads for the new jobs
    void NotifyThreads();

    /// Get job list by the index
    AsyncJobList* GetAsyncJobList(int index)
    {
        HK_ASSERT(index >= 0 && index < m_NumJobLists);
        return &m_JobList[index];
    }

    /// Get worker threads count
    int GetNumWorkerThreads() const { return m_NumWorkerThreads; }

#ifdef HK_ACTIVE_THREADS_COUNTERS
    int GetNumActiveThreads() const
    {
        return NumActiveThreads.Load();
    }
#endif

private:
    void WorkerThreadRoutine(int threadId);

    Thread m_WorkerThread[MAX_WORKER_THREADS];
    int    m_NumWorkerThreads{0};

#ifdef HK_ACTIVE_THREADS_COUNTERS
    AtomicInt m_NumActiveThreads{0};
#endif

    SyncEvent m_EventNotify[MAX_WORKER_THREADS];

    AsyncJobList m_JobList[MAX_JOB_LISTS];
    int          m_NumJobLists{0};

    AtomicInt m_TotalJobs{0};

    bool m_IsTerminated{false};
};

HK_NAMESPACE_END
