/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Core/Ref.h>
#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

struct WeakRefCounter;

class GCObject
{
    HK_FORBID_COPY(GCObject)

    friend class GarbageCollector;
    friend class WeakReference;

public:
    void* operator new(size_t SizeInBytes)
    {
        return Allocators::HeapMemoryAllocator<HEAP_MISC>().allocate(SizeInBytes);
    }
    void operator delete(void* Ptr)
    {
        Allocators::HeapMemoryAllocator<HEAP_MISC>().deallocate(Ptr);
    }

    GCObject();
    virtual ~GCObject();

    /** Add reference */
    void AddRef();

    /** Remove reference */
    void RemoveRef();

    int GetRefCount() const { return m_RefCount; }

    /** Set weakref counter. Used by TWeakRef */
    void SetWeakRefCounter(WeakRefCounter* pRefCounter) { m_WeakRefCounter = pRefCounter; }

    /** Get weakref counter. Used by TWeakRef */
    WeakRefCounter* GetWeakRefCounter() { return m_WeakRefCounter; }

private:
    /** Current refs count for this object */
    int m_RefCount{};

    WeakRefCounter* m_WeakRefCounter{};

    /** Used by garbage collector to add this object to garbage list */
    GCObject* m_NextGarbageObject{};
    GCObject* m_PrevGarbageObject{};
};

template <typename T, typename... TArgs>
T* NewObj(TArgs&&... Args)
{
    // Compile-time check
    GCObject* gcobj = new T(std::forward<TArgs>(Args)...);
    return static_cast<T*>(gcobj);
}

/**

Garbage collector

*/
class GarbageCollector final
{
    HK_FORBID_COPY(GarbageCollector)

    friend class GCObject;

public:
    static void Shutdown();

    /** Deallocates all collected objects */
    static void DeallocateObjects();

    static void KeepPointerAlive(GCObject* pObject);

private:
    static void AddObject(GCObject* pObject);
    static void RemoveObject(GCObject* pObject);

    static void ClearPointers();

    static GCObject* m_GarbageObjects;
    static GCObject* m_GarbageObjectsTail;

    static TVector<GCObject*> m_KeepAlivePtrs;
};

HK_NAMESPACE_END
