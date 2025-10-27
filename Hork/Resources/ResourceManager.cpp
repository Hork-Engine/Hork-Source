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

#include "ResourceManager.h"
#include "Resource.h"

#include <Hork/Core/CoreApplication.h>
#include <Hork/Core/Platform.h>

HK_NAMESPACE_BEGIN

class PriorityThreadPool
{
public:
    explicit PriorityThreadPool(size_t numThreads)
    {
        for (size_t i = 0; i < numThreads; ++i)
        {
            m_Workers.EmplaceBack([this]
                                  { WorkerThread(); });
        }
    }

    ~PriorityThreadPool()
    {
        Stop();
    }

    template <typename F>
    void Enqueue(TaskPriority priority, F&& f)
    {
        {
            std::unique_lock lock(m_QueueMutex);

            switch (priority)
            {
                case TaskPriority::High:
                    m_HighPriorityQueue.emplace(std::forward<F>(f));
                    break;
                case TaskPriority::Low:
                    m_LowPriorityQueue.emplace(std::forward<F>(f));
                    break;
                case TaskPriority::Normal:
                default:
                    m_NormalPriorityQueue.emplace(std::forward<F>(f));
                    break;
            }
        }

        m_Condition.notify_one();
    }

    void Stop()
    {
        {
            std::unique_lock lock(m_QueueMutex);
            m_Stop = true;
        }

        m_Condition.notify_all();

        for (auto& worker : m_Workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    ThreadPoolQueueStats GetQueueStats() const
    {
        std::unique_lock lock(m_QueueMutex);
        return {
            m_HighPriorityQueue.size(),
            m_NormalPriorityQueue.size(),
            m_LowPriorityQueue.size()};
    }

    size_t GetTotalQueueSize() const
    {
        return GetQueueStats().Total();
    }

private:
    void WorkerThread()
    {
        while (true)
        {
            std::function<void()> task;

            {
                std::unique_lock lock(m_QueueMutex);
                m_Condition.wait(lock, [this]
                                 { return m_Stop || !m_HighPriorityQueue.empty() ||
                                         !m_NormalPriorityQueue.empty() || !m_LowPriorityQueue.empty(); });

                if (m_Stop && m_HighPriorityQueue.empty() &&
                    m_NormalPriorityQueue.empty() && m_LowPriorityQueue.empty())
                {
                    return;
                }

                if (!m_HighPriorityQueue.empty())
                {
                    task = std::move(m_HighPriorityQueue.front());
                    m_HighPriorityQueue.pop();
                }
                else if (!m_NormalPriorityQueue.empty())
                {
                    task = std::move(m_NormalPriorityQueue.front());
                    m_NormalPriorityQueue.pop();
                }
                else if (!m_LowPriorityQueue.empty())
                {
                    task = std::move(m_LowPriorityQueue.front());
                    m_LowPriorityQueue.pop();
                }
            }

            if (task)
            {
                task();
            }
        }
    }

private:
    Vector<std::thread>                 m_Workers;

    std::queue<std::function<void()>>   m_HighPriorityQueue;
    std::queue<std::function<void()>>   m_NormalPriorityQueue;
    std::queue<std::function<void()>>   m_LowPriorityQueue;

    mutable std::mutex                  m_QueueMutex;
    std::condition_variable             m_Condition;
    bool                                m_Stop = false;
};

ResourceManager::ResourceManager()
{
    const int numHardwareThreads = std::thread::hardware_concurrency();

    int numWorkerThreads;
    if (numHardwareThreads < 2)
        numWorkerThreads = 1;
    else if (numHardwareThreads <= 4)
        numWorkerThreads = 2;
    else if (numHardwareThreads <= 8)
        numWorkerThreads = 4;
    else
        numWorkerThreads = 6;

    m_ThreadPool = MakeUnique<PriorityThreadPool>(numWorkerThreads);

    m_Caches.Resize(ResourceRTTR::GetTypesCount());

    Core::TraverseDirectory(CoreApplication::sGetRootPath(), false,
        [this](StringView fileName, bool isDirectory)
        {
            if (!isDirectory && PathUtils::sCompareExt(fileName, ".resources"))
            {
                AddResourcePack(fileName);
            }
        });
}

ResourceManager::~ResourceManager()
{
    m_ThreadPool.Reset();
}

void ResourceManager::PurgeUnusedResources()
{
    for (auto& cache : m_Caches)
    {
        if (cache)
            cache->PurgeUnusedResources();
    }
}

namespace
{
    int64_t GetEndTimeForTimeout(float timeoutMsec)
    {
        if (timeoutMsec <= 0.0f)
            return 0;
        return Core::SysMicroseconds() + (timeoutMsec * 1000.0);
    }
}

void ResourceManager::Update(float timeoutMsec)
{
    int64_t endTimeMiscroseconds = GetEndTimeForTimeout(timeoutMsec);

    PurgeUnusedResources(); // TODO: Add timeout?

    if (endTimeMiscroseconds > 0 && Core::SysMicroseconds() >= endTimeMiscroseconds)
        return;

    ProcessCompletedLoads(endTimeMiscroseconds);
}

bool ResourceManager::AddResourcePack(StringView fileName)
{
    // TODO: Для корректной работы с архивами в многопоточке, необходимо для каждого потока
    // держать отдельный экземпляр архива. Нужно уточнить и учесть этот момент.

    Archive archive = Archive::sOpen(fileName, true);
    if (!archive)
        return false;

    m_ResourcePacks.EmplaceBack(std::move(archive));
    m_ResourcePackNames.EmplaceBack(fileName);
    return true;
}

Vector<ArchiveInfo> ResourceManager::GetResourcePackInfo() const
{
    Vector<ArchiveInfo> infos(m_ResourcePacks.Size());
    for (uint32_t i = 0, count = m_ResourcePacks.Size(); i < count; ++i)
    {
        infos[i].FileName = m_ResourcePackNames[i];
        infos[i].FileCount = m_ResourcePacks[i].GetNumFiles();
        infos[i].TotalSize = m_ResourcePacks[i].GetTotalSize();
    }

    return infos;
}

bool ResourceManager::FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const
{
    *pResourcePackIndex = -1;
    pFileHandle->Reset();

    for (int i = m_ResourcePacks.Size() - 1; i >= 0; i--)
    {
        Archive const& pack = m_ResourcePacks[i];

        FileHandle handle = pack.LocateFile(fileName);
        if (handle.IsValid())
        {
            *pResourcePackIndex = i;
            *pFileHandle = handle;
            return true;
        }
    }
    return false;
}

File ResourceManager::OpenFile(StringView name) const
{
    if (!name.IcmpN("/Root/", 6))
    {
        name = name.TruncateHead(6);

        // try to load from file system
        String fileSystemPath = CoreApplication::sGetRootPath() + name;
        if (Core::IsFileExists(fileSystemPath))
        {
            return File::sOpenRead(fileSystemPath);
        }

        // try to load from resource pack
        int resourcePack;
        FileHandle fileHandle;
        if (FindFile(name, &resourcePack, &fileHandle))
        {
            return File::sOpenRead(fileHandle, m_ResourcePacks[resourcePack]);
        }

        LOG("File not found /Root/{}\n", name);
        return {};
    }

    if (!name.IcmpN("/FS/", 4))
    {
        name = name.TruncateHead(4);

        return File::sOpenRead(name);
    }

    if (!name.IcmpN("/Embedded/", 10))
    {
        name = name.TruncateHead(10);

        return File::sOpenRead(name, CoreApplication::sGetEmbeddedArchive());
    }

    LOG("Invalid path \"{}\"\n", name);
    return {};
}

void ResourceManager::ForceLoad(ResourceRef resource, StringView name)
{
    auto file = OpenFile(name);
    if (file)
        resource->Load(file);
}

size_t ResourceManager::LoadAsyncImpl(uint32_t batchId, ResourceTypeID typeId, ResourceRef resource, StringView name, TaskPriority priority)
{
    HK_ASSERT(resource != nullptr);
    if (!resource)
    {
        LOG("ResourceManager::LoadAsync: Resource pointer cannot be null\n");
        return 0;
    }

    if (!m_Caches[typeId])
    {
        LOG("ResourceManager::LoadAsync: Resource type {} is not registered. Use RegisterResourceType<ResourceType>() to register resource type.\n", typeId);
        return 0;
    }

    auto& batch = m_BatchHash[batchId];
    if (!batch)
        batch = MakeUnique<BatchInfo>();

    // Ищем существующую незавершенную задачу для этого же ресурса
    for (const auto& task : m_ActiveLoads)
    {
        if (task.Resource == resource)
        {
            if (batch->TaskIds.Insert(task.TaskId).second)
                batch->TaskCount++;
            return task.TaskId;
        }
    }

    // Создаем новую задачу
    size_t taskId = m_NextTaskId++;
    m_ActiveLoads.Add({taskId, std::move(resource), typeId, priority});

    batch->TaskIds.Insert(taskId);
    batch->TaskCount++;

    String fn(name);

    m_ThreadPool->Enqueue(priority, [this, taskId, fn, typeId]
        {
            //LOG("Start task {}, file {}\n", taskId, fn);
            if (File file = OpenFile(fn))
            {
                //LOG("File opened\n");

                auto data = m_Caches[typeId]->BeginAsyncLoad(file);

                std::lock_guard lock(m_QueueMutex);
                m_CompletedQueue[m_CompletedQueueWrite].emplace(taskId, std::move(data));
            }
            else
            {
                //LOG("File error\n");

                std::unique_ptr<void, void(*)(void*)> data(nullptr, [](void* ptr){});

                std::lock_guard lock(m_QueueMutex);
                m_CompletedQueue[m_CompletedQueueWrite].emplace(taskId, std::move(data));
            }
        });

    return taskId;
}

bool SleepLittle(int64_t endTimeMiscroseconds)
{
    if (endTimeMiscroseconds > 0)
    {
        // Ожидание по таймауту
        
        int64_t curtime = Core::SysMicroseconds();
        if (curtime >= endTimeMiscroseconds)
            return true; // timeout

        auto remainingMilliseconds = static_cast<double>(endTimeMiscroseconds - curtime) * 0.001;

        if (remainingMilliseconds > 10)
        {
            Thread::sWaitMilliseconds(1);
        }
        else if (remainingMilliseconds > 1)
        {
            Thread::sWaitMicroseconds(300);
        }
        else
        {
            Thread::sWaitMicroseconds(100);
        }
    }
    else
    {
        // Бесконечное ожидание - фиксированные короткие паузы
        Thread::sWaitMicroseconds(300);
    }

    return false;
}

WaitResult ResourceManager::WaitForTask(ResourceTaskID taskId, float timeoutMsec)
{
    if (taskId.Get() >= m_NextTaskId)
        return WaitResult::InvalidID;

    if (m_ActiveLoads.IsEmpty())
        return WaitResult::Success;

    int64_t endTimeMiscroseconds = GetEndTimeForTimeout(timeoutMsec);

    while (true)
    {
        ProcessCompletedLoads(endTimeMiscroseconds);

        if (IsTaskCompleted(taskId))
            return WaitResult::Success;

        if (SleepLittle(endTimeMiscroseconds))
            return WaitResult::Timeout;
    }

    return WaitResult::Success;
}

WaitResult ResourceManager::WaitForBatch(uint32_t batchId, float timeoutMsec)
{
    if (m_ActiveLoads.IsEmpty())
        return WaitResult::Success;

    if (m_BatchHash.Count(batchId) == 0)
        return WaitResult::Success;

    int64_t endTimeMiscroseconds = GetEndTimeForTimeout(timeoutMsec);

    while (true)
    {
        ProcessCompletedLoads(endTimeMiscroseconds);

        if (GetBatchRemainingTaskCount(batchId) == 0)
            return WaitResult::Success;

        if (SleepLittle(endTimeMiscroseconds))
            return WaitResult::Timeout;
    }

    return WaitResult::Success;
}

WaitResult ResourceManager::WaitForAll(float timeoutMsec)
{
    if (m_ActiveLoads.IsEmpty())
        return WaitResult::Success;

    int64_t endTimeMiscroseconds = GetEndTimeForTimeout(timeoutMsec);

    while (true)
    {
        ProcessCompletedLoads(endTimeMiscroseconds);

        if (m_ActiveLoads.IsEmpty())
            return WaitResult::Success;

        if (SleepLittle(endTimeMiscroseconds))
            return WaitResult::Timeout;
    }

    return WaitResult::Success;
}

bool ResourceManager::IsTaskCompleted(ResourceTaskID taskId) const
{
    // Если задача не найдена в m_ActiveLoads, но taskId < m_NextTaskId,
    // значит задача существовала и была завершена/удалена

    return taskId.Get() < m_NextTaskId && FindTask(taskId.Get()) == m_ActiveLoads.end();
}

size_t ResourceManager::GetBatchRemainingTaskCount(uint32_t batchId) const
{
    auto it = m_BatchHash.Find(batchId);
    if (it == m_BatchHash.End())
        return 0;
    return it->second->TaskIds.Size();
}

float ResourceManager::GetBatchProgress(uint32_t batchId) const
{
    auto it = m_BatchHash.Find(batchId);
    if (it == m_BatchHash.End())
        return 1; // Batch is not created or all tasks completed

    auto& batch = it->second;
    HK_ASSERT(!batch->TaskIds.IsEmpty());

    size_t completed = batch->TaskCount - batch->TaskIds.Size();
    return static_cast<float>(completed) / batch->TaskCount;
}

void ResourceManager::ProcessCompletedLoads(int64_t endTimeMiscroseconds)
{
    // Забираем все завершенные задачи из очереди
    std::queue<CompletedTask>* completedQueueRead;
    {
        std::lock_guard lock(m_QueueMutex);

        completedQueueRead = &m_CompletedQueue[m_CompletedQueueWrite];
        m_CompletedQueueWrite = (m_CompletedQueueWrite + 1) & 1;
    }

    size_t completedTasks = 0;

    // Обрабатываем каждую завершенную задачу
    while (!completedQueueRead->empty())
    {
        auto& completed = completedQueueRead->front();
        auto it = FindTask(completed.TaskId);

        HK_ASSERT(it != m_ActiveLoads.end());
        if (it != m_ActiveLoads.end())
        {
            if (completed.Success())
            {
                // Успешная загрузка
                HK_ASSERT(it->Resource != nullptr);
                HK_ASSERT(completed.ResourceData != nullptr);
                if (it->Resource->IsPurged())
                    m_Caches[it->TypeId]->InitFromData(it->Resource, std::move(completed.ResourceData));
                else
                    LOG("Resource {} already was loaded\n", it->Resource->GetName());
            }

            m_ActiveLoads.Erase(it);
            completedTasks++;
        }

        completedQueueRead->pop();

        if (endTimeMiscroseconds > 0 && Core::SysMicroseconds() >= endTimeMiscroseconds)
            break;
    }

    if (!completedQueueRead->empty())
    {
        //LOG("ProcessCompletedLoads timeout\n");

        std::lock_guard lock(m_QueueMutex);

        std::queue<CompletedTask>* completedQueueWrite = &m_CompletedQueue[m_CompletedQueueWrite];
        
        while (!completedQueueRead->empty())
        {
            // TODO: Добавлять не в конец, а в начало!
            completedQueueWrite->push(std::move(completedQueueRead->front()));
            completedQueueRead->pop();
        }
    }

    // Очищаем завершенные батчи
    if (completedTasks != 0)
    {
        size_t removedTasks = 0;

        for (auto batchIt = m_BatchHash.begin(); batchIt != m_BatchHash.end() && removedTasks < completedTasks; )
        {
            auto& batch = batchIt->second;

            for (auto taskIt = batch->TaskIds.begin(); taskIt != batch->TaskIds.end(); )
            {
                size_t taskId = *taskIt;
                if (IsTaskCompleted(ResourceTaskID(taskId)))
                {
                    taskIt = batch->TaskIds.erase(taskIt);
                    removedTasks++;
                    if (removedTasks == completedTasks)
                        break;
                }
                else
                    taskIt++;
            }

            if (batch->TaskIds.IsEmpty()) // batch completed
                batchIt = m_BatchHash.erase(batchIt);
            else
                batchIt++;
        }
    }
}

ThreadPoolQueueStats ResourceManager::GetQueueStats() const
{
    return m_ThreadPool->GetQueueStats();
}

ActiveTasksStats ResourceManager::GetActiveTasksStats() const
{
    ActiveTasksStats stats = {};
    for (const auto& task : m_ActiveLoads)
    {
        switch (task.Priority)
        {
            case TaskPriority::High: stats.HighPriority++; break;
            case TaskPriority::Normal: stats.NormalPriority++; break;
            case TaskPriority::Low: stats.LowPriority++; break;
        }
    }
    return stats;
}

size_t ResourceManager::GetActiveTasksCount() const 
{ 
    return m_ActiveLoads.Size();
}

size_t ResourceManager::GetActiveBatchesCount() const
{
    return m_BatchHash.Size();
}

Vector<ResourceManager::LoadTask>::ConstIterator ResourceManager::FindTask(size_t taskId) const
{
    auto it = std::lower_bound(m_ActiveLoads.begin(), m_ActiveLoads.end(), taskId);
    if (it != m_ActiveLoads.end() && it->TaskId == taskId)
    {
        return it;
    }
    return m_ActiveLoads.end();
}

Vector<ResourceManager::LoadTask>::Iterator ResourceManager::FindTask(size_t taskId)
{
    auto it = std::lower_bound(m_ActiveLoads.begin(), m_ActiveLoads.end(), taskId);
    if (it != m_ActiveLoads.end() && it->TaskId == taskId)
    {
        return it;
    }
    return m_ActiveLoads.end();
}

HK_NAMESPACE_END
