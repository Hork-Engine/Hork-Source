/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "Alloc.h"
#include "Logger.h"

/*

TPodQueue

Queue for POD types

*/
template< typename T, int MAX_QUEUE_LENGTH = 256, bool FIXED_LENGTH = true, typename Allocator = AZoneAllocator >
class TPodQueue final {
public:
    enum { TYPE_SIZEOF = sizeof( T ) };

    TPodQueue();
    TPodQueue( TPodQueue const & _Queue );
    ~TPodQueue();

    T * Head() const;
    T * Tail() const;
    T * Push();
    T * Pop();
    T * PopFront();
    bool IsEmpty() const;
    void Clear();
    void Free();
    int Size() const;
    int MaxSize() const;

    TPodQueue & operator=( TPodQueue const & _Queue );

private:
    T   StaticData[ MAX_QUEUE_LENGTH ];
    T * pQueue;
    int QueueHead;
    int QueueTail;
    int MaxQueueLength;
};

template< typename T >
using TPodQueueLite = TPodQueue< T, 1, false >;

#define TPodQueueTemplateDecorate \
    template< typename T, int MAX_QUEUE_LENGTH, bool FIXED_LENGTH, typename Allocator > AN_FORCEINLINE

#define TPodQueueTemplate \
    TPodQueue< T, MAX_QUEUE_LENGTH, FIXED_LENGTH, Allocator >

TPodQueueTemplateDecorate
TPodQueueTemplate::TPodQueue()
    : pQueue(StaticData), QueueHead(0), QueueTail(0), MaxQueueLength(MAX_QUEUE_LENGTH) {
    static_assert( IsPowerOfTwoConstexpr( MAX_QUEUE_LENGTH ), "Queue length must be power of two" );
}

TPodQueueTemplateDecorate
TPodQueueTemplate::TPodQueue( TPodQueueTemplate const & _Queue ) {
    if ( _Queue.MaxQueueLength > MAX_QUEUE_LENGTH ) {
        MaxQueueLength = _Queue.MaxQueueLength;
        pQueue = ( T * )Allocator::Inst().Alloc1( TYPE_SIZEOF * MaxQueueLength );
    } else {
        MaxQueueLength = MAX_QUEUE_LENGTH;
        pQueue = StaticData;
    }

    const int queueLength = _Queue.Size();
    if ( queueLength == _Queue.MaxQueueLength || _Queue.QueueTail == 0 ) {
        memcpy( pQueue, _Queue.pQueue, TYPE_SIZEOF * queueLength );
        QueueHead = _Queue.QueueHead;
        QueueTail = _Queue.QueueTail;
    } else {
        const int WrapMask = _Queue.MaxQueueLength - 1;
        for ( int i = 0 ; i < queueLength ; i++ ) {
            pQueue[i] = _Queue.pQueue[ ( i + _Queue.QueueTail ) & WrapMask ];
        }
        QueueHead = queueLength;
        QueueTail = 0;
    }
}

TPodQueueTemplateDecorate
TPodQueueTemplate::~TPodQueue() {
    if ( pQueue != StaticData ) {
        Allocator::Inst().Dealloc( pQueue );
    }
}

TPodQueueTemplateDecorate
TPodQueueTemplate & TPodQueueTemplate::operator=( TPodQueueTemplate const & _Queue ) {

    // Resize queue
    if ( _Queue.Size() > MaxQueueLength ) {
        if ( pQueue != StaticData ) {
            Allocator::Inst().Dealloc( pQueue );
        }
        if ( _Queue.MaxQueueLength > MAX_QUEUE_LENGTH ) {
            MaxQueueLength = _Queue.MaxQueueLength;
            pQueue = ( T * )Allocator::Inst().Alloc1( TYPE_SIZEOF * MaxQueueLength );
        } else {
            MaxQueueLength = MAX_QUEUE_LENGTH;
            pQueue = StaticData;
        }
    }

    // Copy
    const int queueLength = _Queue.Size();
    if ( queueLength == _Queue.MaxQueueLength || _Queue.QueueTail == 0 ) {
        memcpy( pQueue, _Queue.pQueue, TYPE_SIZEOF * queueLength );
        QueueHead = _Queue.QueueHead;
        QueueTail = _Queue.QueueTail;
    } else {
        const int WrapMask = _Queue.MaxQueueLength - 1;
        for ( int i = 0 ; i < queueLength ; i++ ) {
            pQueue[i] = _Queue.pQueue[ ( i + _Queue.QueueTail ) & WrapMask ];
        }
        QueueHead = queueLength;
        QueueTail = 0;
    }

    return *this;
}

TPodQueueTemplateDecorate
T * TPodQueueTemplate::Head() const {
    if ( IsEmpty() ) {
        return nullptr;
    }
    return pQueue[ ( QueueHead - 1 ) & ( MaxQueueLength - 1 ) ];
}

TPodQueueTemplateDecorate
T * TPodQueueTemplate::Tail() const {
    if ( IsEmpty() ) {
        return nullptr;
    }
    return pQueue[ QueueTail & ( MaxQueueLength - 1 ) ];
}

TPodQueueTemplateDecorate
T * TPodQueueTemplate::Push() {

    if ( QueueHead - QueueTail < MaxQueueLength ) {
        QueueHead++;
        return &pQueue[ ( QueueHead - 1 ) & ( MaxQueueLength - 1 ) ];
    }

    if ( FIXED_LENGTH ) {
        GLogger.Printf( "TStaticQueue::Push: queue overflow\n" );
        QueueTail++;
        QueueHead++;
        return &pQueue[ ( QueueHead - 1 ) & ( MaxQueueLength - 1 ) ];
    }

    const int WrapMask = MaxQueueLength - 1;

    MaxQueueLength <<= 1;

    const int queueLength = Size();
    if ( QueueTail == 0 ) {
        if ( pQueue == StaticData ) {
            pQueue = ( T * )Allocator::Inst().Alloc1( TYPE_SIZEOF * MaxQueueLength );
            memcpy( pQueue, StaticData, TYPE_SIZEOF * queueLength );
        } else {
            pQueue = ( T * )Allocator::Inst().Extend1( pQueue, TYPE_SIZEOF * queueLength, TYPE_SIZEOF * MaxQueueLength, true );
        }
    } else {
        T * data = ( T * )Allocator::Inst().Alloc1( TYPE_SIZEOF * MaxQueueLength );
        for ( int i = 0 ; i < queueLength ; i++ ) {
            data[i] = pQueue[ ( i + QueueTail ) & WrapMask ];
        }
        QueueHead = queueLength;
        QueueTail = 0;
        if ( pQueue != StaticData ) {
            Allocator::Inst().Dealloc( pQueue );
        }
        pQueue = data;
    }

    QueueHead++;
    return &pQueue[ ( QueueHead - 1 ) & ( MaxQueueLength - 1 ) ];
}

TPodQueueTemplateDecorate
T * TPodQueueTemplate::Pop() {
    if ( QueueHead > QueueTail ) {
        QueueTail++;
        return &pQueue[ ( QueueTail - 1 ) & ( MaxQueueLength - 1 ) ];
    }
    return nullptr;
}

TPodQueueTemplateDecorate
T * TPodQueueTemplate::PopFront() {
    if ( QueueHead > QueueTail ) {
        QueueHead--;
        return &pQueue[ QueueHead & ( MaxQueueLength - 1 ) ];
    }
    return nullptr;
}

TPodQueueTemplateDecorate
bool TPodQueueTemplate::IsEmpty() const {
    return QueueHead == QueueTail;
}

TPodQueueTemplateDecorate
void TPodQueueTemplate::Clear() {
    QueueHead = QueueTail = 0;
}

TPodQueueTemplateDecorate
void TPodQueueTemplate::Free() {
    Clear();
    if ( pQueue != StaticData ) {
        Allocator::Inst().Dealloc( pQueue );
        pQueue = StaticData;
    }
    MaxQueueLength = MAX_QUEUE_LENGTH;
}

TPodQueueTemplateDecorate
int TPodQueueTemplate::Size() const {
    return QueueHead - QueueTail;
}

TPodQueueTemplateDecorate
int TPodQueueTemplate::MaxSize() const {
    return MaxQueueLength;
}
