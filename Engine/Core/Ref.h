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

#include "Memory.h"

HK_NAMESPACE_BEGIN

struct WeakRefCounter
{
    void* RawPtr;
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

class RefCounted
{
private:
    int m_RefCount{1};

    WeakRefCounter* m_WeakRefCounter{};

public:
    RefCounted() = default;

    virtual ~RefCounted()
    {
        if (m_WeakRefCounter)
        {
            m_WeakRefCounter->RawPtr = nullptr;
        }
    }

    /** Non-copyable pattern */
    RefCounted(RefCounted const&) = delete;

    /** Non-copyable pattern */
    RefCounted& operator=(RefCounted const&) = delete;

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

    /** Set weakref counter. Used by WeakRef */
    void SetWeakRefCounter(WeakRefCounter* _RefCounter)
    {
        m_WeakRefCounter = _RefCounter;
    }

    /** Get weakref counter. Used by WeakRef */
    WeakRefCounter* GetWeakRefCounter()
    {
        return m_WeakRefCounter;
    }
};


/**

InterlockedRef

Reference counter is interlocked variable.

*/
struct InterlockedRef : public Noncopyable
{
private:
    /** Reference counter */
    AtomicInt m_RefCount{1};

public:
    InterlockedRef() = default;

    virtual ~InterlockedRef() = default;

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

Ref

Shared pointer

*/
template <typename T>
class Ref final
{
public:
    using ReferencedType = T;

    Ref() = default;

    Ref(Ref<T> const& rhs) :
        m_RawPtr(rhs.m_RawPtr)
    {
        if (m_RawPtr)
            m_RawPtr->AddRef();
    }

    template <typename U>
    Ref(Ref<U> const& rhs) :
        m_RawPtr(rhs.m_RawPtr)
    {
        if (m_RawPtr)
            m_RawPtr->AddRef();
    }

    explicit Ref(T* rhs) :
        m_RawPtr(rhs)
    {
        if (m_RawPtr)
            m_RawPtr->AddRef();
    }

    Ref(Ref&& rhs) noexcept :
        m_RawPtr(rhs.m_RawPtr)
    {
        rhs.m_RawPtr = nullptr;
    }

    ~Ref()
    {
        if (m_RawPtr)
            m_RawPtr->RemoveRef();
    }

    T* RawPtr()
    {
        return m_RawPtr;
    }

    T const* RawPtr() const
    {
        return m_RawPtr;
    }

    operator T*() const
    {
        return m_RawPtr;
    }

    T& operator*() const
    {
        HK_ASSERT(m_RawPtr);
        return *m_RawPtr;
    }

    T* operator->()
    {
        HK_ASSERT(m_RawPtr);
        return m_RawPtr;
    }

    T const* operator->() const
    {
        HK_ASSERT(m_RawPtr);
        return m_RawPtr;
    }

    void Reset()
    {
        if (m_RawPtr)
        {
            m_RawPtr->RemoveRef();
            m_RawPtr = nullptr;
        }
    }

    Ref<T>& operator=(Ref<T> const& rhs)
    {
        this->operator=(rhs.m_RawPtr);
        return *this;
    }

    template <typename U>
    Ref<T>& operator=(Ref<U> const& rhs)
    {
        this->operator=(rhs.m_RawPtr);
        return *this;
    }

    Ref<T>& operator=(T* rhs)
    {
        if (m_RawPtr == rhs)
            return *this;
        if (m_RawPtr)
            m_RawPtr->RemoveRef();
        m_RawPtr = rhs;
        if (m_RawPtr)
            m_RawPtr->AddRef();
        return *this;
    }

    Ref<T>& operator=(Ref&& rhs) noexcept
    {
        if (m_RawPtr == rhs.m_RawPtr)
            return *this;
        if (m_RawPtr)
            m_RawPtr->RemoveRef();
        m_RawPtr = rhs.m_RawPtr;
        rhs.m_RawPtr = nullptr;
        return *this;
    }

    void Attach(T* ptr)
    {
        if (m_RawPtr == ptr)
            return;
        if (m_RawPtr)
            m_RawPtr->RemoveRef();
        m_RawPtr = ptr;
    }

    T* Detach()
    {
        T* ptr = m_RawPtr;
        m_RawPtr = nullptr;
        return ptr;
    }

    static Ref<T> Create(T* rhs)
    {
        Ref<T> ptr;
        ptr.Attach(rhs);
        return ptr;
    }

private:
    T* m_RawPtr{};

    template <typename U> friend class Ref;
};


class WeakReference
{
protected:
    WeakReference() = default;

    template <typename T>
    void ResetWeakRef(T* RawPtr)
    {
        T* Cur = m_WeakRefCounter ? (T*)m_WeakRefCounter->RawPtr : nullptr;

        if (Cur == RawPtr)
            return;

        RemoveWeakRef<T>();

        if (!RawPtr)
            return;

        m_WeakRefCounter = RawPtr->GetWeakRefCounter();
        if (!m_WeakRefCounter)
        {
            m_WeakRefCounter = AllocateWeakRefCounter();
            m_WeakRefCounter->RawPtr = RawPtr;
            m_WeakRefCounter->RefCount = 1;
            RawPtr->SetWeakRefCounter(m_WeakRefCounter);
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
                if (m_WeakRefCounter->RawPtr)
                {
                    ((T*)m_WeakRefCounter->RawPtr)->SetWeakRefCounter(nullptr);
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

WeakRef

Weak pointer

*/
template <typename T>
class WeakRef final : public WeakReference
{
public:
    WeakRef() = default;

    WeakRef(WeakRef<T> const& rhs)
    {
        ResetWeakRef(rhs.IsExpired() ? nullptr : const_cast<T*>(rhs.RawPtr()));
    }

    WeakRef(Ref<T> const& rhs)
    {
        ResetWeakRef(const_cast<T*>(rhs.RawPtr()));
    }

    explicit WeakRef(T* rhs)
    {
        ResetWeakRef(rhs);
    }

    WeakRef(WeakRef<T>&& rhs) noexcept
    {
        m_WeakRefCounter      = rhs.m_WeakRefCounter;
        rhs.m_WeakRefCounter = nullptr;
    }

    ~WeakRef()
    {
        RemoveWeakRef<T>();
    }

    Ref<T> ToStrongRef() const
    {
        return Ref<T>(const_cast<T*>(RawPtr()));
    }

    T* RawPtr()
    {
        return m_WeakRefCounter ? static_cast<T*>(m_WeakRefCounter->RawPtr) : nullptr;
    }

    T const* RawPtr() const
    {
        return m_WeakRefCounter ? static_cast<T*>(m_WeakRefCounter->RawPtr) : nullptr;
    }

    operator T*() const
    {
        return const_cast<T*>(RawPtr());
    }

    T& operator*() const
    {
        HK_ASSERT(!IsExpired());
        return *RawPtr();
    }

    T* operator->()
    {
        HK_ASSERT(!IsExpired());
        return RawPtr();
    }

    T const* operator->() const
    {
        HK_ASSERT(!IsExpired());
        return RawPtr();
    }

    bool IsExpired() const
    {
        return !m_WeakRefCounter || static_cast<T*>(m_WeakRefCounter->RawPtr) == nullptr;
    }

    void Reset()
    {
        RemoveWeakRef<T>();
    }

    void operator=(T* rhs)
    {
        ResetWeakRef(rhs);
    }

    void operator=(Ref<T> const& rhs)
    {
        ResetWeakRef(const_cast<T*>(rhs.RawPtr()));
    }

    void operator=(WeakRef<T> const& rhs)
    {
        ResetWeakRef(rhs.IsExpired() ? nullptr : const_cast<T*>(rhs.RawPtr()));
    }

    WeakRef<T>& operator=(WeakRef<T>&& rhs) noexcept
    {
        if (*this == rhs)
            return *this;

        Reset();

        m_WeakRefCounter      = rhs.m_WeakRefCounter;
        rhs.m_WeakRefCounter = nullptr;

        return *this;
    }
};


template <typename T, typename U>
HK_FORCEINLINE bool operator==(Ref<T> const& lhs, Ref<U> const& rhs) { return lhs.RawPtr() == rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(Ref<T> const& lhs, Ref<U> const& rhs) { return lhs.RawPtr() != rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(Ref<T> const& lhs, WeakRef<U> const& rhs) { return lhs.RawPtr() == rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(Ref<T> const& lhs, WeakRef<U> const& rhs) { return lhs.RawPtr() != rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(WeakRef<T> const& lhs, Ref<U> const& rhs) { return lhs.RawPtr() == rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(WeakRef<T> const& lhs, Ref<U> const& rhs) { return lhs.RawPtr() != rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(WeakRef<T> const& lhs, WeakRef<U> const& rhs) { return lhs.RawPtr() == rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(WeakRef<T> const& lhs, WeakRef<U> const& rhs) { return lhs.RawPtr() != rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator==(T const* lhs, WeakRef<U> const& rhs) { return lhs == rhs.RawPtr(); }

template <typename T, typename U>
HK_FORCEINLINE bool operator!=(T const* lhs, WeakRef<U> const& rhs) { return lhs != rhs.RawPtr(); }


template <typename T, typename... Args>
inline Ref<T> MakeRef(Args&&... args)
{
    return Ref<T>::Create(new T(std::forward<Args>(args)...));
}

template <typename T>
HK_FORCEINLINE void CheckedDelete(T* Ptr)
{
    using type_must_be_complete = char[sizeof(T) ? 1 : -1];
    (void)sizeof(type_must_be_complete);
    delete Ptr;
}

template <typename T>
class UniqueRef
{
public:
    UniqueRef() = default;

    explicit UniqueRef(T* InPtr) :
        m_RawPtr(InPtr)
    {}

    UniqueRef(UniqueRef<T> const&) = delete;
    UniqueRef& operator=(UniqueRef<T> const&) = delete;

    template <typename U>
    UniqueRef(UniqueRef<U>&& rhs) :
        m_RawPtr(rhs.Detach())
    {}

    ~UniqueRef()
    {
        CheckedDelete(m_RawPtr);
    }

    template <typename U>
    UniqueRef& operator=(UniqueRef<U>&& rhs)
    {
        Attach(rhs.Detach());
        return *this;
    }

    T* operator->() const
    {
        HK_ASSERT(m_RawPtr);
        return m_RawPtr;
    }

    T& operator*() const
    {
        HK_ASSERT(m_RawPtr);
        return *m_RawPtr;
    }

    template <typename U>
    bool operator==(UniqueRef<U> const& rhs)
    {
        return m_RawPtr == rhs.m_RawPtr;
    }

    template <typename U>
    bool operator!=(UniqueRef<U> const& rhs)
    {
        return m_RawPtr != rhs.m_RawPtr;
    }

    operator bool() const
    {
        return m_RawPtr != nullptr;
    }

    T* RawPtr() const
    {
        return m_RawPtr;
    }

    void Reset()
    {
        CheckedDelete(m_RawPtr);
        m_RawPtr = nullptr;
    }

    void Attach(T* ptr)
    {
        CheckedDelete(m_RawPtr);
        m_RawPtr = ptr;
    }

    T* Detach()
    {
        T* ptr = m_RawPtr;
        m_RawPtr = nullptr;
        return ptr;
    }

private:
    T* m_RawPtr{};
};

template <typename T, typename... Args>
UniqueRef<T> MakeUnique(Args&&... args)
{
    return UniqueRef<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
Ref<T> GetSharedInstance()
{
    static WeakRef<T> weakPtr;
    Ref<T>            strongPtr;

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

HK_NAMESPACE_END
