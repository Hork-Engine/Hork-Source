/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

#include <Engine/Core/Public/PodArray.h>

//#define AN_ACTIVE_THREADS_COUNTERS

struct FAsyncJob {
    void (*Callback)( void * );
    void *Data;
    FAsyncJob * Next;
};

class FAsyncJobManager;

class FAsyncJobList final {
    AN_FORBID_COPY( FAsyncJobList )

    friend class FAsyncJobManager;

public:
    void SetMaxParallelJobs( int _MaxParallelJobs );

    int GetMaxParallelJobs() const;

    void AddJob( void (*_Callback)( void * ), void * _Data );

    void Submit();

    void Wait();

    void SubmitAndWait();

private:
    FAsyncJobList();
    ~FAsyncJobList();

    FAsyncJobManager * JobManager;

    TPodArray< FAsyncJob, 1024 > JobPool;
    FAsyncJob * JobList;
    int NumPendingJobs;

    FAsyncJob * SubmittedJobs;
    FThreadSync SubmitSync;

    FSyncEvent EventDone;
    bool       bSignalled;      // FIXME: must be atomic?
};

AN_FORCEINLINE int FAsyncJobList::GetMaxParallelJobs() const {
    return JobPool.Reserved();
}

class FAsyncJobManager final {
    AN_FORBID_COPY( FAsyncJobManager )

public:
    static constexpr int MAX_WORKER_THREADS = 4;
    static constexpr int MAX_JOB_LISTS = 4;

    FAsyncJobManager();

    void Initialize( int _NumWorkerThreads, int _NumJobLists );

    void Deinitialize();

    void NotifyThreads();

    FAsyncJobList * GetAsyncJobList( int _Index ) { AN_Assert( _Index >= 0 && _Index < NumJobLists ); return &JobList[_Index]; }

#ifdef AN_ACTIVE_THREADS_COUNTERS
    int GetNumActiveThreads() const { return NumActiveThreads.Load(); }
#endif

private:

    static void _WorkerThreadRoutine( void * _Data );

    void WorkerThreadRoutine( int _ThreadId );

    FThread     WorkerThread[MAX_WORKER_THREADS];
    int         NumWorkerThreads;

#ifdef AN_ACTIVE_THREADS_COUNTERS
    FAtomicInt  NumActiveThreads;
#endif

    FSyncEvent  EventNotify[MAX_WORKER_THREADS];

    FAsyncJobList JobList[MAX_JOB_LISTS];
    int         NumJobLists;

    struct FContext {
        FAsyncJobManager * JobManager;
        int ThreadId;
    };

    FContext    Contexts[MAX_WORKER_THREADS];

    bool        bTerminated;
};

#if 0
struct FAsyncStreamTask {
    void ( *Callback )( void * );
    void * Data;
    bool bAbort;
    FAsyncStreamTask * Next;
};

class FAsyncStreamManager final {
    AN_FORBID_COPY( FAsyncStreamManager )

public:
    void Initialize();

    void Shutdown();

    void AddTask( FAsyncStreamTask * _Task );

private:

    void Run();

    void ProcessTasks();

    FAsyncStreamTask * FetchTask();

    static void ThreadRoutine( void * _Data );

    FThread Thread;
    FThreadSync FetchSync;

    using FTaskQueue = TPodQueue< FAsyncStreamTask *, 256, false >;

    FTaskQueue Tasks;
    FSyncEvent EventNotify;

    bool bTerminate;
};
#endif
