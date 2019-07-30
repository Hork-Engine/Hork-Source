/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Runtime/Public/AsyncJobManager.h>
#include <Engine/Core/Public/Logger.h>

#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4701 )
#endif

constexpr int FAsyncJobManager::MAX_WORKER_THREADS;
constexpr int FAsyncJobManager::MAX_JOB_LISTS;

FAsyncJobManager::FAsyncJobManager() {
}

void FAsyncJobManager::Initialize( int _NumWorkerThreads, int _NumJobLists ) {
    if ( _NumWorkerThreads > MAX_WORKER_THREADS ) {
        GLogger.Printf( "FAsyncJobManager::Initialize: NumWorkerThreads > MAX_WORKER_THREADS\n" );
        _NumWorkerThreads = MAX_WORKER_THREADS;
    } else if ( _NumWorkerThreads <= 0 ) {
        _NumWorkerThreads = MAX_WORKER_THREADS;
    }

    AN_Assert( _NumJobLists >= 1 && _NumJobLists <= MAX_JOB_LISTS );

    GLogger.Printf( "Initializing async job manager ( %d worker threads, %d job lists )\n", _NumWorkerThreads, _NumJobLists );

    bTerminated = false;

    NumJobLists = _NumJobLists;
    for ( int i = 0 ; i < NumJobLists ; i++ ) {
        JobList[i].JobManager = this;
    }

    NumWorkerThreads = _NumWorkerThreads;
    for ( int i = 0 ; i < NumWorkerThreads ; i++ ) {
        Contexts[i].JobManager = this;
        Contexts[i].ThreadId = i;
        WorkerThread[i].Routine = _WorkerThreadRoutine;
        WorkerThread[i].Data = ( void * )&Contexts[i];
        WorkerThread[i].Start();
    }
}

void FAsyncJobManager::Deinitialize() {
    GLogger.Printf( "Deinitializing async job manager\n" );

    NotifyThreads();

    for ( int i = 0 ; i < NumJobLists ; i++ ) {
        JobList[i].Wait();
        JobList[i].JobPool.Free();
    }

    bTerminated = true;
    NotifyThreads();

    for ( int i = 0 ; i < NumWorkerThreads ; i++ ) {
        WorkerThread[i].Join();
    }
}

void FAsyncJobManager::NotifyThreads() {
    for ( int i = 0 ; i < NumWorkerThreads ; i++ ) {
        EventNotify[i].Signal();
    }
}

void FAsyncJobManager::_WorkerThreadRoutine( void * _Data ) {
    FContext * context = ( FContext * )_Data;

    context->JobManager->WorkerThreadRoutine( context->ThreadId );
}

void FAsyncJobManager::WorkerThreadRoutine( int _ThreadId ) {
    FAsyncJob job;
    bool haveJob;

#ifdef AN_ACTIVE_THREADS_COUNTERS
    NumActiveThreads.Increment();
#endif

    while ( !bTerminated ) {

#ifdef AN_ACTIVE_THREADS_COUNTERS
        NumActiveThreads.Decrement();
#endif
        EventNotify[ _ThreadId ].Wait();

#ifdef AN_ACTIVE_THREADS_COUNTERS
        NumActiveThreads.Increment();
#endif

        for ( int currentList = 0 ; currentList < NumJobLists ; currentList++ ) {

            int fetchIndex = ( _ThreadId + currentList ) %  NumJobLists;

            FAsyncJobList * jobList = &JobList[ fetchIndex ];

            do {

                haveJob = false;

                // fetch job
                {
                    FSyncGuard syncGuard( jobList->SubmitSync );

                    if ( jobList->SubmittedJobs ) {
                        job = *jobList->SubmittedJobs;
                        jobList->SubmittedJobs = job.Next;
                        haveJob = true;
                    } else {
                        if ( !jobList->bSignalled ) {
                            jobList->bSignalled = true;
                            jobList->EventDone.Signal();
                        }
                    }
                }

                if ( haveJob ) {
                    job.Callback( job.Data );
                }

            } while ( /*haveJob*/ !jobList->bSignalled );
        }
    }

#ifdef AN_ACTIVE_THREADS_COUNTERS
    NumActiveThreads.Decrement();
#endif

    GLogger.Printf( "Terminating worker thread (%d)\n", _ThreadId );
}

FAsyncJobList::FAsyncJobList() {
    JobList = nullptr;
    SubmittedJobs = nullptr;
    NumPendingJobs = 0;
    bSignalled = false;
}

FAsyncJobList::~FAsyncJobList() {
    Wait();
}

void FAsyncJobList::SetMaxParallelJobs( int _MaxParallelJobs ) {
    AN_Assert( JobPool.IsEmpty() );

    JobPool.ReserveInvalidate( _MaxParallelJobs );
}

void FAsyncJobList::AddJob( void (*_Callback)( void * ), void * _Data ) {
    if ( JobPool.Size() == JobPool.Reserved() ) {
        GLogger.Printf( "Warning: FAsyncJobList::AddJob: job pool overflow, use SetMaxParallelJobs to reserve proper pool size (current size %d)\n", JobPool.Reserved() );

        SubmitAndWait();
        SetMaxParallelJobs( JobPool.Reserved() * 2 );
    }

    FAsyncJob & job = JobPool.Append();
    job.Callback = _Callback;
    job.Data = _Data;
    job.Next = JobList;
    JobList = &job;
    NumPendingJobs++;
}

void FAsyncJobList::Submit() {
    if ( !NumPendingJobs ) {
        return;
    }

    FAsyncJob * headJob = &JobPool[ JobPool.Size() - NumPendingJobs ];
    AN_Assert( headJob->Next == nullptr );

    // lock section
    {
        FSyncGuard syncGuard( SubmitSync );

        headJob->Next = SubmittedJobs;
        SubmittedJobs = JobList;

        bSignalled = false;
    }

    JobManager->NotifyThreads();
    JobList = nullptr;
    NumPendingJobs = 0;
}

void FAsyncJobList::Wait() {
    int jobsCount = JobPool.Size() - NumPendingJobs;

    if ( jobsCount > 0 ) {
        EventDone.Wait();

        AN_Assert( SubmittedJobs == nullptr );

        if ( NumPendingJobs > 0 ) {

            GLogger.Printf( "Warning: FAsyncJobList::Wait: NumPendingJobs > 0\n" );

            JobPool.Remove( 0, jobsCount );

            JobList = JobPool.ToPtr() + ( NumPendingJobs - 1 );
            for ( int i = 1 ; i < NumPendingJobs ; i++ ) {
                JobPool[i].Next = &JobPool[i - 1];
            }

        } else {
            JobPool.Clear();
        }
    }
}

void FAsyncJobList::SubmitAndWait() {
    Submit();
    Wait();
}

//#include <Engine/Runtime/Public/Runtime.h>
//void FirstJob( void * _Data ) {
//    for ( int i = 0 ; i < 32 ; i++ ) {
//        GLogger.Printf( "FirstJob: Processing %d (%d) th %d\n", (size_t)_Data&0xf, i, (size_t)_Data>>16 );
//        GRuntime.WaitMilliseconds(1);
//    }
//}

//void SecondJob( void * _Data ) {
//    for ( int i = 0 ; i < 32 ; i++ ) {
//        GLogger.Printf( "SecondJob: Processing %d (%d) th %d\n", (size_t)_Data&0xf, i, (size_t)_Data>>16 );
//        GRuntime.WaitMilliseconds(1);
//    }
//}

//void AsyncJobManagerTest() {

//    FAsyncJobManager jobManager;

//    jobManager.Initialize( 4, 2 );

//    FAsyncJobList * first = jobManager.GetAsyncJobList( 0 );
//    FAsyncJobList * second = jobManager.GetAsyncJobList( 1 );

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



#if 0
void FAsyncStreamManager::Initialize() {
    bTerminate = false;
    Thread.Routine = ThreadRoutine;
    Thread.Data = this;
    Thread.Start();
}

void FAsyncStreamManager::Shutdown() {
    bTerminate = true;
    EventNotify.Signal();
    Thread.Join();
}

void FAsyncStreamManager::AddTask( FAsyncStreamTask * _Task ) {
    {
        FSyncGuard syncGuard( FetchSync );
        *Tasks.Push() = _Task;
    }

    EventNotify.Signal();
}

void FAsyncStreamManager::Run() {
    while ( !bTerminate ) {
        EventNotify.Wait();

        ProcessTasks();
    }
    ProcessTasks();
}

void FAsyncStreamManager::ProcessTasks() {
    FAsyncStreamTask * task;

    while ( nullptr != ( task = FetchTask() ) ) {
        task->Callback( task->Data );
    }
}

FAsyncStreamTask * FAsyncStreamManager::FetchTask() {
    FSyncGuard syncGuard( FetchSync );

    if ( Tasks.IsEmpty() ) {
        return nullptr;
    }

    return *Tasks.Pop();
}

void FAsyncStreamManager::ThreadRoutine( void * _Data ) {
    ( FAsyncStreamManager * )(_Data)->Run();
}


FAsyncStreamManager GStreamManager;

class FTexture {
public:

    FString Texture;

    FSyncEvent LoadCompleted;
    FAtomicInt AsyncRun;

    FAsyncStreamTask * Tasks;

    FTexture() {

    }

    ~FTexture() {

    }

    static void TaskCallback( void * _Data ) {
        FTexture * textureObject = ( FTexture * )( _Data );

        LoadTexture( textureObject->Texture.ToConstChar() );

        textureObject->bLoaded = true;

        LoadCompleted.Signal();
    }

    void LoadAsync( const char * _Texture ) {
        Texture = _Texture;

        if ( AsyncRun.FetchIncrement() > 0 ) {
            // already in task manager

            FAsyncStreamTask * task = GMainMemoryZone.Alloc( sizeof( FAsyncStreamTask ), 1 );

            task->Next = Tasks;
            Tasks = task;
        }

        Task.Callback = TaskCallback;
        Task.Data = this;
        Task.bAbort = false;

        GStreamManager.AddTask( &Task );
    }

    void LoadWait() {
        if ( !bLoaded ) {
            LoadCompleted.Wait();
        }
    }

    bool IsLoaded() const {
        return bLoaded;
    }
};

void AsyncTaskManagerTest() {
    GStreamManager.Initialize();

    FTexture * textureObject = new FTextureStreamTask;

    textureObject->LoadAsync( "test.bmp" );
    textureObject->LoadWait();

    GStreamManager.Shutdown();
}
#endif
