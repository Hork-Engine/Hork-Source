/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Platform/Public/Memory/Memory.h>
#include <Platform/Public/Logger.h>

/**

TPodQueue

Queue for POD types

*/
template <typename T, int MAX_QUEUE_LENGTH = 256, bool FIXED_LENGTH = true, typename Allocator = AZoneAllocator>
class TPodQueue final
{
public:
    enum
    {
        TYPE_SIZEOF = sizeof(T)
    };

    TPodQueue() :
        pQueue(StaticData), QueueHead(0), QueueTail(0), MaxQueueLength(MAX_QUEUE_LENGTH)
    {
        static_assert(IsPowerOfTwo(MAX_QUEUE_LENGTH), "Queue length must be power of two");
    }

    TPodQueue(TPodQueue const& _Queue)
    {
        if (_Queue.MaxQueueLength > MAX_QUEUE_LENGTH)
        {
            MaxQueueLength = _Queue.MaxQueueLength;
            pQueue         = (T*)Allocator::Inst().Alloc1(TYPE_SIZEOF * MaxQueueLength);
        }
        else
        {
            MaxQueueLength = MAX_QUEUE_LENGTH;
            pQueue         = StaticData;
        }

        const int queueLength = _Queue.Size();
        if (queueLength == _Queue.MaxQueueLength || _Queue.QueueTail == 0)
        {
            Core::Memcpy(pQueue, _Queue.pQueue, TYPE_SIZEOF * queueLength);
            QueueHead = _Queue.QueueHead;
            QueueTail = _Queue.QueueTail;
        }
        else
        {
            const int WrapMask = _Queue.MaxQueueLength - 1;
            for (int i = 0; i < queueLength; i++)
            {
                pQueue[i] = _Queue.pQueue[(i + _Queue.QueueTail) & WrapMask];
            }
            QueueHead = queueLength;
            QueueTail = 0;
        }
    }

    ~TPodQueue()
    {
        if (pQueue != StaticData)
        {
            Allocator::Inst().Free(pQueue);
        }
    }

    T* Head() const
    {
        if (IsEmpty())
        {
            return nullptr;
        }
        return pQueue[(QueueHead - 1) & (MaxQueueLength - 1)];
    }

    T* Tail() const
    {
        if (IsEmpty())
        {
            return nullptr;
        }
        return pQueue[QueueTail & (MaxQueueLength - 1)];
    }

    T* Push()
    {
        if (QueueHead - QueueTail < MaxQueueLength)
        {
            QueueHead++;
            return &pQueue[(QueueHead - 1) & (MaxQueueLength - 1)];
        }

        if (FIXED_LENGTH)
        {
            GLogger.Printf("TPodQueue::Push: queue overflow\n");
            QueueTail++;
            QueueHead++;
            return &pQueue[(QueueHead - 1) & (MaxQueueLength - 1)];
        }

        const int WrapMask = MaxQueueLength - 1;

        MaxQueueLength <<= 1;

        const int queueLength = Size();
        if (QueueTail == 0)
        {
            if (pQueue == StaticData)
            {
                pQueue = (T*)Allocator::Inst().Alloc(TYPE_SIZEOF * MaxQueueLength);
                Core::Memcpy(pQueue, StaticData, TYPE_SIZEOF * queueLength);
            }
            else
            {
                pQueue = (T*)Allocator::Inst().Realloc(pQueue, TYPE_SIZEOF * MaxQueueLength, true);
            }
        }
        else
        {
            T* data = (T*)Allocator::Inst().Alloc(TYPE_SIZEOF * MaxQueueLength);
            for (int i = 0; i < queueLength; i++)
            {
                data[i] = pQueue[(i + QueueTail) & WrapMask];
            }
            QueueHead = queueLength;
            QueueTail = 0;
            if (pQueue != StaticData)
            {
                Allocator::Inst().Free(pQueue);
            }
            pQueue = data;
        }

        QueueHead++;
        return &pQueue[(QueueHead - 1) & (MaxQueueLength - 1)];
    }

    T* Pop()
    {
        if (QueueHead > QueueTail)
        {
            QueueTail++;
            return &pQueue[(QueueTail - 1) & (MaxQueueLength - 1)];
        }
        return nullptr;
    }

    T* PopFront()
    {
        if (QueueHead > QueueTail)
        {
            QueueHead--;
            return &pQueue[QueueHead & (MaxQueueLength - 1)];
        }
        return nullptr;
    }

    bool IsEmpty() const
    {
        return QueueHead == QueueTail;
    }

    void Clear()
    {
        QueueHead = QueueTail = 0;
    }

    void Free()
    {
        Clear();
        if (pQueue != StaticData)
        {
            Allocator::Inst().Free(pQueue);
            pQueue = StaticData;
        }
        MaxQueueLength = MAX_QUEUE_LENGTH;
    }

    int Size() const
    {
        return QueueHead - QueueTail;
    }

    int Capacity() const
    {
        return MaxQueueLength;
    }

    TPodQueue& operator=(TPodQueue const& _Queue)
    {
        // Resize queue
        if (_Queue.Size() > MaxQueueLength)
        {
            if (pQueue != StaticData)
            {
                Allocator::Inst().Free(pQueue);
            }
            if (_Queue.MaxQueueLength > MAX_QUEUE_LENGTH)
            {
                MaxQueueLength = _Queue.MaxQueueLength;
                pQueue         = (T*)Allocator::Inst().Alloc1(TYPE_SIZEOF * MaxQueueLength);
            }
            else
            {
                MaxQueueLength = MAX_QUEUE_LENGTH;
                pQueue         = StaticData;
            }
        }

        // Copy
        const int queueLength = _Queue.Size();
        if (queueLength == _Queue.MaxQueueLength || _Queue.QueueTail == 0)
        {
            Core::Memcpy(pQueue, _Queue.pQueue, TYPE_SIZEOF * queueLength);
            QueueHead = _Queue.QueueHead;
            QueueTail = _Queue.QueueTail;
        }
        else
        {
            const int WrapMask = _Queue.MaxQueueLength - 1;
            for (int i = 0; i < queueLength; i++)
            {
                pQueue[i] = _Queue.pQueue[(i + _Queue.QueueTail) & WrapMask];
            }
            QueueHead = queueLength;
            QueueTail = 0;
        }

        return *this;
    }

private:
    static_assert(std::is_trivial<T>::value, "Expected POD type");

    alignas(16) T StaticData[MAX_QUEUE_LENGTH];
    T*  pQueue;
    int QueueHead;
    int QueueTail;
    int MaxQueueLength;
};

template <typename T, typename Allocator = AZoneAllocator>
using TPodQueueLite = TPodQueue<T, 1, false, Allocator>;
