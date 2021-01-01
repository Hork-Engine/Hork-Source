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

#include <Core/Public/PodArray.h>

//#define AN_ACTIVE_THREADS_COUNTERS

/** Job for job list */
struct SAsyncJob
{
    /** Callback for the job */
    void (*Callback)( void * );
    /** Data that will be passed for the job */
    void *Data;
    /** Pointer to the next job in job list */
    SAsyncJob * Next;
};

class AAsyncJobManager;

/** Job list */
class AAsyncJobList final
{
    AN_FORBID_COPY( AAsyncJobList )

    friend class AAsyncJobManager;

public:
    /** Set job pool size (max jobs for the list) */
    void SetMaxParallelJobs( int _MaxParallelJobs );

    /** Get job pool size */
    int GetMaxParallelJobs() const;

    /** Add job to the list */
    void AddJob( void (*_Callback)( void * ), void * _Data );

    /** Submit jobs to worker threads */
    void Submit();

    /** Block current thread while jobs are in working threads */
    void Wait();

    /** Submit jobs to worker threads and block current thread while jobs are in working threads */
    void SubmitAndWait();

private:
    AAsyncJobList();
    ~AAsyncJobList();

    AAsyncJobManager * JobManager;

    TPodArray< SAsyncJob, 1024 > JobPool;
    SAsyncJob * JobList;
    int NumPendingJobs;

    SAsyncJob * SubmittedJobs;
    AThreadSync SubmitSync;

    AAtomicInt SubmittedJobsCount;
    AAtomicInt FetchCount;
    //AAtomicInt FetchLock;

    ASyncEvent EventDone;
    bool       bSignalled;      // FIXME: must be atomic?
};

AN_FORCEINLINE int AAsyncJobList::GetMaxParallelJobs() const {
    return JobPool.Capacity();
}

/** Job manager */
class AAsyncJobManager final
{
    AN_FORBID_COPY( AAsyncJobManager )

public:
    static constexpr int MAX_WORKER_THREADS = 4;
    static constexpr int MAX_JOB_LISTS = 4;

    AAsyncJobManager();

    /** Initialize job manager. Set worker threads count and create job lists */
    void Initialize( int _NumWorkerThreads, int _NumJobLists );

    /** Shutdown job manager */
    void Deinitialize();

    void SubmitJobList( AAsyncJobList * InJobList );

    /** Wakeup worker threads for the new jobs */
    void NotifyThreads();

    /** Get job list by the index */
    AAsyncJobList * GetAsyncJobList( int _Index ) { AN_ASSERT( _Index >= 0 && _Index < NumJobLists ); return &JobList[_Index]; }

    /** Get worker threads count */
    int GetNumWorkerThreads() const { return NumWorkerThreads; }

#ifdef AN_ACTIVE_THREADS_COUNTERS
    int GetNumActiveThreads() const { return NumActiveThreads.Load(); }
#endif

private:

    static void _WorkerThreadRoutine( void * _Data );

    void WorkerThreadRoutine( int _ThreadId );

    AThread     WorkerThread[MAX_WORKER_THREADS];
    int         NumWorkerThreads;

#ifdef AN_ACTIVE_THREADS_COUNTERS
    AAtomicInt  NumActiveThreads;
#endif

    ASyncEvent  EventNotify[MAX_WORKER_THREADS];

    AAsyncJobList JobList[MAX_JOB_LISTS];
    int         NumJobLists;

    AAtomicInt  TotalJobs;

    struct SContext {
        AAsyncJobManager * JobManager;
        int ThreadId;
    };

    SContext    Contexts[MAX_WORKER_THREADS];

    bool        bTerminated;
};
