/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Platform/Memory/Memory.h>

struct WeakRefCounter
{
    void* Object;
    int   RefCount;

    void* operator new(size_t SizeInBytes)
    {
        return Allocators::HeapMemoryAllocator<HEAP_MISC>().allocate(SizeInBytes);
    }
    void operator delete(void* Ptr)
    {
        Allocators::HeapMemoryAllocator<HEAP_MISC>().deallocate(Ptr);
    }
};

class ARefCounted
{
private:
    int m_RefCount{1};

    WeakRefCounter* m_WeakRefCounter{};

public:
    ARefCounted() = default;

    virtual ~ARefCounted()
    {
        if (m_WeakRefCounter)
        {
            m_WeakRefCounter->Object = nullptr;
        }
    }

    /** Non-copyable pattern */
    ARefCounted(ARefCounted const&) = delete;

    /** Non-copyable pattern */
    ARefCounted& operator=(ARefCounted const&) = delete;

    /** Add reference */
    inline void AddRef()
    {
        ++m_RefCount;
    }

    /** Remove reference */
    inline void RemoveRef()
    {
        if (--m_RefCount == 0)
        {
            delete this;
            return;
        }

        HK_ASSERT(m_RefCount > 0);
    }

    /** Reference count */
    int GetRefCount() const
    {
        return m_RefCount;
    }

    /** Set weakref counter. Used by TWeakRef */
    void SetWeakRefCounter(WeakRefCounter* _RefCounter)
    {
        m_WeakRefCounter = _RefCounter;
    }

    /** Get weakref counter. Used by TWeakRef */
    WeakRefCounter* GetWeakRefCounter()
    {
        return m_WeakRefCounter;
    }
};


/**

SInterlockedRef

Reference counter is interlocked variable.

*/
struct SInterlockedRef
{
    HK_FORBID_COPY(SInterlockedRef)

private:
    /** Reference counter */
    AAtomicInt m_RefCount{1};

public:
    SInterlockedRef() = default;

    virtual ~SInterlockedRef() = default;

    /** Add reference. */
    HK_FORCEINLINE void AddRef()
    {
        m_RefCount.Increment();
    }

    /** Remove reference. */
    HK_FORCEINLINE void RemoveRef()
    {
        if (m_RefCount.Decrement() == 0)
        {
            delete this;
        }
    }

    /** Reference count */
    HK_FORCEINLINE int GetRefCount() const
    {
        return m_RefCount.Load();
    }
};


/**

TRef

Shared pointer

*/
template <typename T>
class TRef final
{
public:
    using ReferencedType = T;

    TRef() = default;

    TRef(TRef<T> const& rhs) :
        m_Object(rhs.m_Object)
    {
        if (m_Object)
        {
            m_Object->AddRef();
        }
    }

    explicit TRef(T* pObject) :
        m_Object(pObject)
    {
        if (m_Object)
        {
            m_Object->AddRef();
        }
    }

    TRef(TRef&& rhs) :
        m_Object(rhs.m_Object)
    {
        rhs.m_Object = nullptr;
    }

    ~TRef()
    {
        if (m_Object)
        {
            m_Object->RemoveRef();
        }
    }

    T* GetObject()
    {
        return m_Object;
    }

    T const* GetObject() const
    {
        return m_Object;
    }

    //    operator bool() const
    //    {
    //        return m_Object != nullptr;
    //    }

    operator T*() const
    {
        return m_Object;
    }

    T& operator*() const
    {
        HK_ASSERT_(m_Object, "TRef");
        return *m_Object;
    }

    T* operator->()
    {
        HK_ASSERT_(m_Object, "TRef");
        return m_Object;
    }

    T const* operator->() const
    {
        HK_ASSERT_(m_Object, "TRef");
        return m_Object;
    }

    void Reset()
    {
        if (m_Object)
        {
            m_Object->RemoveRef();
            m_Object = nullptr;
        }
    }

    TRef<T>& operator=(TRef<T> const& rhs)
    {
        this->operator=(rhs.m_Object);
        return *this;
    }

    TRef<T>& operator=(T* pObject)
    {
        if (m_Object == pObject)
        {
            return *this;
        }
        if (m_Object)
        {
            m_Object->RemoveRef();
        }
        m_Object = pObject;
        if (m_Object)
        {
            m_Object->AddRef();
        }
        return *this;
    }

    TRef<T>& operator=(TRef&& rhs)
    {
        if (m_Object == rhs.m_Object)
        {
            return *this;
        }
        if (m_Object)
        {
            m_Object->RemoveRef();
        }
        m_Object     = rhs.m_Object;
        rhs.m_Object = nullptr;
        return *this;
    }

    void Attach(T* rhs)
    {
        if (m_Object == rhs)
            return;
        if (m_Object)
            m_Object->RemoveRef();
        m_Object = rhs;
    }

    T* Detach()
    {
        T* ptr = m_Object;
        m_Object = nullptr;
        return ptr;
    }

private:
    T* m_Object{};
};


class WeakReference
{
protected:
    WeakReference() = default;

    template <typename T>
    void ResetWeakRef(T* pObject)
    {
        T* Cur = m_WeakRefCounter ? (T*)m_WeakRefCounter->Object : nullptr;

        if (Cur == pObject)
        {
            return;
        }

        RemoveWeakRef<T>();

        if (!pObject)
        {
            return;
        }

        m_WeakRefCounter = pObject->GetWeakRefCounter();
        if (!m_WeakRefCounter)
        {
            m_WeakRefCounter           = AllocateWeakRefCounter();
            m_WeakRefCounter->Object   = pObject;
            m_WeakRefCounter->RefCount = 1;
            pObject->SetWeakRefCounter(m_WeakRefCounter);
        }
        else
        {
            m_WeakRefCounter->RefCount++;
        }
    }

    template <typename T>
    void RemoveWeakRef()
    {
        if (m_WeakRefCounter)
        {
            if (--m_WeakRefCounter->RefCount == 0)
            {
                if (m_WeakRefCounter->Object)
                {
                    ((T*)m_WeakRefCounter->Object)->SetWeakRefCounter(nullptr);
                }
                DeallocateWeakRefCounter(m_WeakRefCounter);
            }
            m_WeakRefCounter = nullptr;
        }
    }

    WeakRefCounter* m_WeakRefCounter{};

private:
    WeakRefCounter* AllocateWeakRefCounter();

    void DeallocateWeakRefCounter(WeakRefCounter* pRefCounter);
};

/**

TWeakRef

Weak pointer

*/
template <typename T>
class TWeakRef final : public WeakReference
{
public:
    TWeakRef() = default;

    TWeakRef(TWeakRef<T> const& rhs)
    {
        ResetWeakRef(rhs.IsExpired() ? nullptr : const_cast<T*>(rhs.GetObject()));
    }

    TWeakRef(TRef<T> const& rhs)
    {
        ResetWeakRef(const_cast<T*>(rhs.GetObject()));
    }

    explicit TWeakRef(T* pObject)
    {
        ResetWeakRef(pObject);
    }

    TWeakRef(TWeakRef<T>&& rhs)
    {
        m_WeakRefCounter      = rhs.m_WeakRefCounter;
        rhs.m_WeakRefCounter = nullptr;
    }

    ~TWeakRef()
    {
        RemoveWeakRef<T>();
    }

    TRef<T> ToStrongRef() const
    {
        return TRef<T>(const_cast<T*>(GetObject()));
    }

    T* GetObject()
    {
        return m_WeakRefCounter ? static_cast<T*>(m_WeakRefCounter->Object) : nullptr;
    }

    T const* GetObject() const
    {
        return m_WeakRefCounter ? static_cast<T*>(m_WeakRefCounter->Object) : nullptr;
    }

    //    operator bool() const
    //    {
    //        return !IsExpired();
    //    }

    operator T*() const
    {
        return const_cast<T*>(GetObject());
    }

    T& operator*() const
    {
        HK_ASSERT_(!IsExpired(), "TWeakRef");
        return *GetObject();
    }

    T* operator->()
    {
        HK_ASSERT_(!IsExpired(), "TWeakRef");
        return GetObject();
    }

    T const* operator->() const
    {
        HK_ASSERT_(!IsExpired(), "TWeakRef");
        return GetObject();
    }

    bool IsExpired() const
    {
        return !m_WeakRefCounter || static_cast<T*>(m_WeakRefCounter->Object) == nullptr;
    }

    void Reset()
    {
        RemoveWeakRef<T>();
    }

    void operator=(T* pObject)
    {
        ResetWeakRef(pObject);
    }

    void operator=(TRef<T> const& rhs)
    {
        ResetWeakRef(const_cast<T*>(rhs.GetObject()));
    }

    void operator=(TWeakRef<T> const& rhs)
    {
        ResetWeakRef(rhs.IsExpired() ? nullptr : const_cast<T*>(rhs.GetObject()));
    }

    TWeakRef<T>& operator=(TWeakRef<T>&& rhs)
    {
        if (*this == rhs)
        {
            return *this;
        }

        Reset();

        m_WeakRefCounter      = rhs.m_WeakRefCounter;
        rhs.m_WeakRefCounter = nullptr;

        return *this;
    }
};


template <typename T, typename U>
HK_FORCEINLINE bool operator==(TRef<T> const& lhs, TRef<U> const& rhs) { return lhs.GetObject() == rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(TRef<T> const& lhs, TRef<U> const& rhs) { return lhs.GetObject() != rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(TRef<T> const& lhs, TWeakRef<U> const& rhs) { return lhs.GetObject() == rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(TRef<T> const& lhs, TWeakRef<U> const& rhs) { return lhs.GetObject() != rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(TWeakRef<T> const& lhs, TRef<U> const& rhs) { return lhs.GetObject() == rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(TWeakRef<T> const& lhs, TRef<U> const& rhs) { return lhs.GetObject() != rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(TWeakRef<T> const& lhs, TWeakRef<U> const& rhs) { return lhs.GetObject() == rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(TWeakRef<T> const& lhs, TWeakRef<U> const& rhs) { return lhs.GetObject() != rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(T const* lhs, TWeakRef<U> const& rhs) { return lhs == rhs.GetObject(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(T const* lhs, TWeakRef<U> const& rhs) { return lhs != rhs.GetObject(); }


template <typename T, typename... Args>
inline TRef<T> MakeRef(Args&&... args)
{
    // create object (refcount=1)
    T* x = new T(std::forward<Args>(args)...);
    // handle reference (refcount=2)
    TRef<T> ref(x);
    // keep refcount=1
    x->RemoveRef();
    return ref;
}

template <typename T>
HK_FORCEINLINE void CheckedDelete(T* Ptr)
{
    using type_must_be_complete = char[sizeof(T) ? 1 : -1];
    (void)sizeof(type_must_be_complete);
    delete Ptr;
}

template <typename T>
class TUniqueRef
{
public:
    TUniqueRef() = default;

    explicit TUniqueRef(T* InPtr) :
        Object(InPtr)
    {}

    TUniqueRef(TUniqueRef<T> const&) = delete;
    TUniqueRef& operator=(TUniqueRef<T> const&) = delete;

    template <typename T2>
    TUniqueRef(TUniqueRef<T2>&& rhs) :
        Object(rhs.Detach())
    {}

    ~TUniqueRef()
    {
        Reset();
    }

    template <typename T2>
    TUniqueRef& operator=(TUniqueRef<T2>&& rhs)
    {
        Reset(rhs.Detach());
        return *this;
    }

    T* operator->() const
    {
        HK_ASSERT(Object);
        return Object;
    }

    T& operator*() const
    {
        HK_ASSERT(Object);
        return *Object;
    }

    template <typename U>
    bool operator==(TUniqueRef<U> const& rhs)
    {
        return Object == rhs.Object;
    }

    template <typename U>
    bool operator!=(TUniqueRef<U> const& rhs)
    {
        return Object != rhs.Object;
    }

    operator bool() const
    {
        return Object != nullptr;
    }

    T* Detach()
    {
        T* ptr = Object;
        Object = nullptr;
        return ptr;
    }

    T* GetObject() const
    {
        return Object;
    }

    void Reset(T* InPtr = nullptr)
    {
        CheckedDelete(Object);
        Object = InPtr;
    }

private:
    T* Object{};
};

template <typename T, typename... Args>
TUniqueRef<T> MakeUnique(Args&&... args)
{
    return TUniqueRef<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
TRef<T> GetSharedInstance()
{
    static TWeakRef<T> weakPtr;
    TRef<T>            strongPtr;

    if (weakPtr.IsExpired())
    {
        strongPtr = MakeRef<T>();
        weakPtr   = strongPtr;
    }
    else
    {
        strongPtr = weakPtr;
    }
    return strongPtr;
}
