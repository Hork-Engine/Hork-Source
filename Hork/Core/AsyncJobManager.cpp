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

#include "AsyncJobManager.h"
#include "Logger.h"
#include "Profiler.h"

HK_NAMESPACE_BEGIN

constexpr int AsyncJobManager::MAX_WORKER_THREADS;
constexpr int AsyncJobManager::MAX_JOB_LISTS;

AsyncJobManager::AsyncJobManager(int numWorkerThreads, int numJobLists)
{
    if (numWorkerThreads > MAX_WORKER_THREADS)
    {
        LOG("AsyncJobManager::Initialize: NumWorkerThreads > MAX_WORKER_THREADS\n");
        numWorkerThreads = MAX_WORKER_THREADS;
    }
    else if (numWorkerThreads <= 0)
    {
        numWorkerThreads = MAX_WORKER_THREADS;
    }

    HK_ASSERT(numJobLists >= 1 && numJobLists <= MAX_JOB_LISTS);

    LOG("Initializing async job manager ( {} worker threads, {} job lists )\n", numWorkerThreads, numJobLists);

    m_IsTerminated = false;

    m_NumJobLists = numJobLists;
    for (int i = 0; i < m_NumJobLists; i++)
    {
        m_JobList[i].m_JobManager = this;
    }

    m_TotalJobs.Store(0);

    m_NumWorkerThreads = numWorkerThreads;
    for (int i = 0; i < m_NumWorkerThreads; i++)
    {
        m_WorkerThread[i] = Thread(
            [this](int ThreadId)
            {
                _HK_PROFILER_THREAD("Worker");
                WorkerThreadRoutine(ThreadId);
            },
            i);
    }
}

AsyncJobManager::~AsyncJobManager()
{
    LOG("Deinitializing async job manager\n");

    NotifyThreads();

    for (int i = 0; i < m_NumJobLists; i++)
    {
        m_JobList[i].Wait();
        m_JobList[i].m_JobPool.Free();
    }

    m_IsTerminated = true;
    NotifyThreads();

    for (int i = 0; i < m_NumWorkerThreads; i++)
    {
        m_WorkerThread[i].Join();
    }
}

void AsyncJobManager::NotifyThreads()
{
    for (int i = 0; i < m_NumWorkerThreads; i++)
    {
        m_EventNotify[i].Signal();
    }
}

void AsyncJobManager::WorkerThreadRoutine(int threadId)
{
    AsyncJob job = {};
    bool haveJob;

#ifdef HK_ACTIVE_THREADS_COUNTERS
    NumActiveThreads.Increment();
#endif

    while (!m_IsTerminated)
    {
        HK_PROFILER_EVENT("Worker loop");

#ifdef HK_ACTIVE_THREADS_COUNTERS
        m_NumActiveThreads.Decrement();
#endif

        //LOG( "Thread waiting {}\n", threadId );

        m_EventNotify[threadId].Wait();

#ifdef HK_ACTIVE_THREADS_COUNTERS
        m_NumActiveThreads.Increment();
#endif

        for (int currentList = 0; m_TotalJobs.Load() > 0; currentList++)
        {
            int fetchIndex = (threadId + currentList) % m_NumJobLists;

            AsyncJobList* jobList = &m_JobList[fetchIndex];

            // Check if list have a jobs
            if (jobList->m_FetchCount.Load() > 0)
            {
                haveJob = false;

                // fetch job
                {
                    MutexGuard syncGuard(jobList->m_SubmitSync);

                    //if ( jobList->m_FetchLock.Increment() == 1 )
                    //{
                    if (jobList->m_SubmittedJobs)
                    {
                        job = *jobList->m_SubmittedJobs;
                        jobList->m_SubmittedJobs = job.Next;
                        haveJob = true;

                        jobList->m_FetchCount.Decrement();
                        m_TotalJobs.Decrement();
                    }
                    //    jobList->m_FetchLock.Decrement();
                    //} else {
                    //    jobList->m_FetchLock.Decrement();
                    //}
                }

                if (haveJob)
                {
                    job.Callback(job.Data);

                    // Check if this was last processed job in the list
                    if (jobList->m_SubmittedJobsCount.Decrement() == 0)
                    {
                        MutexGuard syncGuard(jobList->m_SubmitSync);

                        // Check for new submits
                        if (!jobList->m_SubmittedJobs && jobList->m_SubmittedJobsCount.Load() == 0)
                        {

                            // Check if already signalled from other thread
                            if (!jobList->m_IsSignalled)
                            {
                                jobList->m_IsSignalled = true;
                                jobList->m_EventDone.Signal();
                            }
                        }
                    }
                }
            }
        }
    }

#ifdef HK_ACTIVE_THREADS_COUNTERS
    NumActiveThreads.Decrement();
#endif

    LOG("Terminating worker thread ({})\n", threadId);
}

AsyncJobList::AsyncJobList()
{
}

AsyncJobList::~AsyncJobList()
{
    Wait();
}

void AsyncJobList::SetMaxParallelJobs(int maxParallelJobs)
{
    HK_ASSERT(m_JobPool.IsEmpty());

    m_JobPool.Clear();
    m_JobPool.Reserve(maxParallelJobs);
}

void AsyncJobList::AddJob(void (*callback)(void*), void* data)
{
    if (m_JobPool.Size() == m_JobPool.Capacity())
    {
        LOG("Warning: AsyncJobList::AddJob: job pool overflow, use SetMaxParallelJobs to reserve proper pool size (current size {})\n", m_JobPool.Capacity());

        SubmitAndWait();
        SetMaxParallelJobs(m_JobPool.Capacity() * 2);
    }

    AsyncJob& job = m_JobPool.Add();
    job.Callback = callback;
    job.Data = data;
    job.Next = m_JobList;
    m_JobList = &job;
    m_NumPendingJobs++;
}

void AsyncJobList::Submit()
{
    m_JobManager->SubmitJobList(this);
}

void AsyncJobManager::SubmitJobList(AsyncJobList* jobList)
{
    if (!jobList->m_NumPendingJobs)
    {
        return;
    }

    AsyncJob* headJob = &jobList->m_JobPool[jobList->m_JobPool.Size() - jobList->m_NumPendingJobs];
    HK_ASSERT(headJob->Next == nullptr);

    // lock section
    {
        MutexGuard syncGuard(jobList->m_SubmitSync);

        headJob->Next = jobList->m_SubmittedJobs;
        jobList->m_SubmittedJobs = jobList->m_JobList;

        jobList->m_SubmittedJobsCount.Add(jobList->m_NumPendingJobs);
        jobList->m_FetchCount.Add(jobList->m_NumPendingJobs);

        m_TotalJobs.Add(jobList->m_NumPendingJobs);

        jobList->m_IsSignalled = false;
    }

    NotifyThreads();
    jobList->m_JobList = nullptr;
    jobList->m_NumPendingJobs = 0;
}

void AsyncJobList::Wait()
{
    int jobsCount = m_JobPool.Size() - m_NumPendingJobs;

    if (jobsCount > 0)
    {
        while (!m_IsSignalled)
        {
            m_EventDone.Wait();
        }

        HK_ASSERT(m_SubmittedJobsCount.Load() == 0);
        HK_ASSERT(m_FetchCount.Load() == 0);
        HK_ASSERT(m_SubmittedJobs == nullptr);

        if (m_NumPendingJobs > 0)
        {
            LOG("Warning: AsyncJobList::Wait: NumPendingJobs > 0\n");

            m_JobPool.RemoveRange(0, jobsCount);

            m_JobList = m_JobPool.ToPtr() + size_t(m_NumPendingJobs - 1);
            for (int i = 1; i < m_NumPendingJobs; i++)
            {
                m_JobPool[i].Next = &m_JobPool[i - 1];
            }
        }
        else
        {
            m_JobPool.Clear();
        }
    }
}

void AsyncJobList::SubmitAndWait()
{
    Submit();
    Wait();
}

//void FirstJob( void * data ) {
//    for ( int i = 0 ; i < 32 ; i++ ) {
//        GLogger.Printf( "FirstJob: Processing %d (%d) th %d\n", (size_t)data&0xf, i, (size_t)data>>16 );
//        Thread::WaitMilliseconds(1);
//    }
//}

//void SecondJob( void * data ) {
//    for ( int i = 0 ; i < 32 ; i++ ) {
//        GLogger.Printf( "SecondJob: Processing %d (%d) th %d\n", (size_t)data&0xf, i, (size_t)data>>16 );
//        Thread::WaitMilliseconds(1);
//    }
//}

//void AsyncJobManagerTest() {

//    AsyncJobManager jobManager;

//    jobManager.Initialize( 4, 2 );

//    AsyncJobList * first = jobManager.GetAsyncJobList( 0 );
//    AsyncJobList * second = jobManager.GetAsyncJobList( 1 );

//    first->AddJob( FirstJob, ( void * )0 );
//    first->AddJob( FirstJob, ( void * )1 );

//    second->AddJob( SecondJob, ( void * )0 );
//    second->AddJob( SecondJob, ( void * )1 );

//    first->Submit();
//    second->Submit();

//    first->Wait();
//    second->Wait();

//    jobManager.Deinitialize();
//}

HK_NAMESPACE_END
