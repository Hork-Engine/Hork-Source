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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/Handle.h>
#include <Hork/Core/BaseMath.h>

HK_NAMESPACE_BEGIN

// 32bits Version
// id 26 bit for id inside pool + 4bits PoolID

template <typename Entity>
class HandleAllocator
{
public:
    using EntityHandle = Handle<Entity>;

    HandleAllocator();
    ~HandleAllocator();

    EntityHandle EntityAlloc();

    void EntityFreeUnlocked(EntityHandle handle);

    Entity& GetEntityRef(EntityHandle handle);

private:
    static constexpr size_t MAX_POOLS = 16;

    void AllocatePool(size_t poolNum);

    static uint32_t sMakeId(uint32_t poolNum, uint32_t index);

    static HK_INLINE size_t sGetPoolMaxEntities(size_t poolNum)
    {
        return (size_t(1) << poolNum) * 1024;
    }

    static HK_INLINE uint32_t sGetPoolNum(uint32_t id)
    {
        return (id >> 26) & 0xf;
    }

    static HK_INLINE uint32_t sGetIndex(uint32_t id)
    {
        uint32_t index = id & 0x3ffffff;
        HK_ASSERT(index > 0);
        return index - 1;
    }

    struct Pool
    {
        Entity*  Entities;
        uint32_t Total;
    };

    Pool                m_Pools[MAX_POOLS];
    int                 m_NumPools{};
    SpinLock            m_Mutex;
    Vector<uint32_t>    m_FreeList;
};

template <typename Entity>
uint32_t HandleAllocator<Entity>::sMakeId(uint32_t poolNum, uint32_t index)
{
    HK_ASSERT(index < sGetPoolMaxEntities(poolNum));
    HK_ASSERT(poolNum < HandleAllocator::MAX_POOLS);

    return (index + 1) | (poolNum << 26);
}

template <typename Entity>
HandleAllocator<Entity>::HandleAllocator()
{
    AllocatePool(0);
    m_NumPools = 1;
}

template <typename Entity>
HandleAllocator<Entity>::~HandleAllocator()
{
    for (int i = 0; i < m_NumPools; i++)
        Core::GetHeapAllocator<HEAP_MISC>().Free(m_Pools[i].Entities);
}

template <typename Entity>
void HandleAllocator<Entity>::AllocatePool(size_t poolNum)
{
    m_Pools[poolNum].Entities = (Entity*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(Entity) * sGetPoolMaxEntities(poolNum));
    m_Pools[poolNum].Total = 0;
}

template <typename Entity>
Handle<Entity> HandleAllocator<Entity>::EntityAlloc()
{
    uint32_t id;
    uint32_t version;

    {
        SpinLockGuard lockGuard(m_Mutex);

        if (!m_FreeList.IsEmpty())
        {
            id = m_FreeList.Last();
            m_FreeList.RemoveLast();

            auto poolNum = sGetPoolNum(id);
            auto index = sGetIndex(id);

            version = m_Pools[poolNum].Entities[index].Version;

            new (m_Pools[poolNum].Entities + index) Entity;
            m_Pools[poolNum].Entities[index].Version = version;
        }
        else
        {
            uint32_t poolNum = m_NumPools - 1;
            if (m_Pools[poolNum].Total >= sGetPoolMaxEntities(poolNum))
            {
                HK_ASSERT(m_NumPools < MAX_POOLS);
                if (m_NumPools == MAX_POOLS)
                {
                    // Too many entities
                    return 0;
                }

                // allocate new pool
                poolNum = m_NumPools++;

                AllocatePool(poolNum);
            }

            auto index = m_Pools[poolNum].Total++;

            id = sMakeId(poolNum, index);

            version = 1;
            new (m_Pools[poolNum].Entities + index) Entity;
            m_Pools[poolNum].Entities[index].Version = version;
        }
    }

    return EntityHandle(id, version);
}

template <typename Entity>
void HandleAllocator<Entity>::EntityFreeUnlocked(EntityHandle handle)
{
    auto version = handle.GetVersion();
    auto id = handle.GetID();
    auto poolNum = sGetPoolNum(id);
    auto index = sGetIndex(id);

    Entity& e = m_Pools[poolNum].Entities[index];

    HK_ASSERT(handle);
    HK_ASSERT(version == e.Version);
    HK_ASSERT(index < sGetPoolMaxEntities(poolNum));

    if (version != e.Version)
        return;

    version = Math::Max(1u, e.Version + 1); // Handle wrap;

    e.~Entity();
    e.Version = version;

    m_FreeList.Add(id);
}

template <typename Entity>
Entity& HandleAllocator<Entity>::GetEntityRef(EntityHandle handle)
{
    auto id = handle.GetID();
    auto poolNum = sGetPoolNum(id);
    auto index = sGetIndex(id);

    HK_ASSERT(handle);
    HK_ASSERT(index < sGetPoolMaxEntities(poolNum));

    return m_Pools[poolNum].Entities[index];
}

HK_NAMESPACE_END
