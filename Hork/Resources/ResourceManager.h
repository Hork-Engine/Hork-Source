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

#include "ResourceCache.h"

#include <Hork/Core/IO.h>
#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/UniqueRef.h>

#include <queue>

HK_NAMESPACE_BEGIN

/// Приоритеты асинхронного выполнения задачи
enum class TaskPriority
{
    Low,
    Normal,
    High
};

/// Результаты ожидания
enum class WaitResult
{
    Success,
    Timeout,
    InvalidID
};

struct ThreadPoolQueueStats
{
    size_t      HighPriority;
    size_t      NormalPriority;
    size_t      LowPriority;

    size_t      Total() const { return HighPriority + NormalPriority + LowPriority; }
};

struct ArchiveInfo
{
    String      FileName;
    uint32_t    FileCount;
    uint32_t    TotalSize;
};

struct ActiveTasksStats
{
    size_t      HighPriority;
    size_t      NormalPriority;
    size_t      LowPriority;

    size_t      Total() const { return HighPriority + NormalPriority + LowPriority; }
};

template <typename Type, typename Tag>
class TaggedID
{
public:
                        TaggedID() = default;
    explicit            TaggedID(Type val) : Value(val) {}

    Type const&         Get() const { return Value; }

private:
    Type                Value{};
};

struct ResourceTaskTag {};
using ResourceTaskID = TaggedID<uint64_t, ResourceTaskTag>;

enum RESOURCE_BATCH : uint32_t
{
    BATCH_DEFAULT = 0
};

class ResourceManager final : public Noncopyable
{
public:
                                ResourceManager();
                                ~ResourceManager();

    /// Добавляет архив. Проверка на дублирование одинаковых архивов не осуществляется.
    bool                        AddResourcePack(StringView fileName);
    /// Возвращает информацию о добавленных архивах.
    Vector<ArchiveInfo>         GetResourcePackInfo() const;

    /// Ищет или создает пустой ресурс.
    template <typename ResourceType>
    IntrusiveRef<ResourceType>  Acquire(StringView name) { return GetCache<ResourceType>().Acquire(name); }

    /// Поиск ресурса по имени. Если ресурс не найден, то возвращается nullptr.
    template <typename ResourceType>
    IntrusiveRef<ResourceType>  Find(StringView name) { return GetCache<ResourceType>().Find(name); }

    /// Загружает ресурс из файла. Если ресурс уже загружен ранее, то возвращается указатель на него.
    template <typename ResourceType>
    IntrusiveRef<ResourceType>  Load(StringView name) { return GetCache<ResourceType>().Load(name, ResourceLoad::UseCachedResource); }

    void                        ForceLoad(ResourceRef resource, StringView name);

    /// Запускает асинхронную загрузку ресураса.
    /// ID существующей задачи или ID новой задачи. В случае ошибки возвращает 0.
    template <typename ResourceType>
    ResourceTaskID              LoadAsync(uint32_t batchId, IntrusiveRef<ResourceType> resource, StringView name, TaskPriority priority = TaskPriority::Normal);

    /// Запускает асинхронную загрузку ресураса.
    /// ID существующей задачи или ID новой задачи. В случае ошибки возвращает 0.
    template <typename ResourceType>
    ResourceTaskID              LoadAsync(uint32_t batchId, IntrusiveRef<ResourceType> resource, TaskPriority priority = TaskPriority::Normal);

    /// Запускает асинхронную загрузку ресураса. Возвращает указатель на ресурс. Если ресурс уже загружен, то не загружает заново.
    template <typename ResourceType>
    IntrusiveRef<ResourceType>  LoadAsync(uint32_t batchId, StringView name, TaskPriority priority = TaskPriority::Normal);

    /// Ожидает выполнение задачи.
    WaitResult                  WaitForTask(ResourceTaskID taskId, float timeoutMsec = 0);

    /// Ожидает выполнение всех задач в батче.
    WaitResult                  WaitForBatch(uint32_t batchId, float timeoutMsec = 0);

    /// Ожидает выполнение всех задач в менеджере ресурсов.
    WaitResult                  WaitForAll(float timeoutMsec = 0);

    /// Возвращает статус задачи (true - completed, false - задача либо не существует, либо еще в процессе обработки)
    bool                        IsTaskCompleted(ResourceTaskID taskId) const;

    /// Возвращает количество активных задач в батче.
    size_t                      GetBatchRemainingTaskCount(uint32_t batchId) const;

    /// Возвращает прогресс выполненных задач в батче от 0 до 1
    float                       GetBatchProgress(uint32_t batchId) const;

    /// Обрабатывает выполненные задачи.
    /// При истичении таймаута функция завершается. Это необходимо, чтобы "размазывать" обработку большого количества задач по кадрам.
    void                        ProcessCompletedLoads(int64_t endTimeMiscroseconds);

    /// Очищает неиспользуемые ресурсы.
    void                        PurgeUnusedResources();

    // Обновление ресурсного менеджера. Вызывается на каждом кадре.
    void                        Update(float timeoutMsec);

    /// Ищет и открывает файл в файловой системе и паках.
    File                        OpenFile(StringView name) const;

    /// Статичтика по очередям в пуле потоков.
    ThreadPoolQueueStats        GetQueueStats() const;

    /// Статистика по активным задачам и батчам.
    ActiveTasksStats            GetActiveTasksStats() const;
    size_t                      GetActiveTasksCount() const;
    size_t                      GetActiveBatchesCount() const;

private:
    // Структура для завершенных задач
    struct CompletedTask
    {
        size_t          TaskId;
        std::unique_ptr<void, void(*)(void*)> ResourceData;

        bool            Success() const { return ResourceData != nullptr; }

                        CompletedTask(size_t id, std::unique_ptr<void, void(*)(void*)> data)
                            : TaskId(id), ResourceData(std::move(data)) {}
    };

    // Структура для активных задач
    struct LoadTask
    {
        size_t          TaskId;
        ResourceRef     Resource;
        ResourceTypeID  TypeId;
        TaskPriority    Priority;

        bool            operator<(const LoadTask& other) const { return TaskId < other.TaskId; }
        bool            operator<(size_t id) const { return TaskId < id; }
    };

    struct BatchInfo
    {
        HashSet<size_t> TaskIds;
        size_t          TaskCount;
    };

    /// Регистрация типа ресурса. 
    template <typename ResourceType>
    void                        RegisterResourceType();

    template <typename ResourceType>
    ResourceCache<ResourceType>& GetCache();

    /// Find file in resource packs
    bool                        FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const;

    size_t                      LoadAsyncImpl(uint32_t batchId, ResourceTypeID typeId, ResourceRef resource, StringView name, TaskPriority priority);

    Vector<LoadTask>::Iterator  FindTask(size_t taskId);
    Vector<LoadTask>::ConstIterator FindTask(size_t taskId) const;

    Vector<UniqueRef<ResourceCacheBase>> m_Caches;
    Vector<Archive>             m_ResourcePacks;
    Vector<String>              m_ResourcePackNames;
    UniqueRef<class PriorityThreadPool> m_ThreadPool;

    Vector<LoadTask>            m_ActiveLoads;
    HashMap<uint32_t, UniqueRef<BatchInfo>> m_BatchHash;
    
    // Очередь завершенных задач
    std::queue<CompletedTask>   m_CompletedQueue[2];
    int                         m_CompletedQueueWrite = 0;
    mutable std::mutex          m_QueueMutex;

    size_t                      m_NextTaskId = 0;
};

HK_NAMESPACE_END

#include "ResourceManager.inl"
