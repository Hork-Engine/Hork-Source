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

#include "Memory.h"

HK_NAMESPACE_BEGIN

struct WeakRefCounter
{
    void* RawPtr;
    int   RefCount;

    void* operator new(size_t sizeInBytes)
    {
        return Allocators::HeapMemoryAllocator<HEAP_MISC>().allocate(sizeInBytes);
    }
    void operator delete(void* Ptr)
    {
        Allocators::HeapMemoryAllocator<HEAP_MISC>().deallocate(Ptr);
    }
};

// DEPRECATED. Use IntrusiveRef whenever possible.
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

    /// Non-copyable pattern
    RefCounted(RefCounted const&) = delete;

    /// Non-copyable pattern
    RefCounted& operator=(RefCounted const&) = delete;

    /// Add reference
    inline void AddRef()
    {
        ++m_RefCount;
    }

    /// Remove reference
    inline void RemoveRef()
    {
        if (--m_RefCount == 0)
        {
            delete this;
            return;
        }

        HK_ASSERT(m_RefCount > 0);
    }

    /// Reference count
    int GetRefCount() const
    {
        return m_RefCount;
    }

    /// Set weakref counter. Used by WeakRef
    void SetWeakRefCounter(WeakRefCounter* refCounter)
    {
        m_WeakRefCounter = refCounter;
    }

    /// Get weakref counter. Used by WeakRef
    WeakRefCounter* GetWeakRefCounter()
    {
        return m_WeakRefCounter;
    }
};


/**

InterlockedRef

Reference counter is interlocked variable.

DEPRECATED. Use IntrusiveRef whenever possible.

*/
struct InterlockedRef : public Noncopyable
{
private:
    /// Reference counter
    AtomicInt m_RefCount{1};

public:
    InterlockedRef() = default;

    virtual ~InterlockedRef() = default;

    /// Add reference.
    HK_FORCEINLINE void AddRef()
    {
        m_RefCount.Increment();
    }

    /// Remove reference.
    HK_FORCEINLINE void RemoveRef()
    {
        if (m_RefCount.Decrement() == 0)
        {
            delete this;
        }
    }

    /// Reference count
    HK_FORCEINLINE int GetRefCount() const
    {
        return m_RefCount.Load();
    }
};


/**

Ref

Shared pointer

DEPRECATED. Use IntrusiveRef whenever possible.

*/
template <typename T>
class Ref final
{
public:
    using ReferencedType = T;

    Ref() noexcept = default;

    Ref(Ref<T> const& rhs) noexcept :
        m_RawPtr(rhs.m_RawPtr)
    {
        if (m_RawPtr)
            m_RawPtr->AddRef();
    }

    template <typename U>
    Ref(Ref<U> const& rhs) noexcept :
        m_RawPtr(rhs.m_RawPtr)
    {
        if (m_RawPtr)
            m_RawPtr->AddRef();
    }

    explicit Ref(T* rhs) noexcept :
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

    T* RawPtr() noexcept
    {
        return m_RawPtr;
    }

    T const* RawPtr() const noexcept
    {
        return m_RawPtr;
    }

    operator T*() const noexcept
    {
        return m_RawPtr;
    }

    T& operator*() const noexcept
    {
        HK_ASSERT(m_RawPtr);
        return *m_RawPtr;
    }

    T* operator->() noexcept
    {
        HK_ASSERT(m_RawPtr);
        return m_RawPtr;
    }

    T const* operator->() const noexcept
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
        if HK_LIKELY(m_RawPtr != rhs)
        {
            if (m_RawPtr)
                m_RawPtr->RemoveRef();
            m_RawPtr = rhs;
            if (m_RawPtr)
                m_RawPtr->AddRef();
        }
        return *this;
    }

    Ref<T>& operator=(Ref&& rhs) noexcept
    {
        if HK_LIKELY(this != &rhs)
        {
            if (m_RawPtr)
                m_RawPtr->RemoveRef();
            m_RawPtr = rhs.m_RawPtr;
            rhs.m_RawPtr = nullptr;
        }
        return *this;
    }

    void Attach(T* ptr)
    {
        if HK_LIKELY(m_RawPtr != ptr)
        {
            if (m_RawPtr)
                m_RawPtr->RemoveRef();
            m_RawPtr = ptr;
        }
    }

    T* Detach() noexcept
    {
        T* ptr = m_RawPtr;
        m_RawPtr = nullptr;
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
    void ResetWeakRef(T* rawPtr)
    {
        T* Cur = m_WeakRefCounter ? (T*)m_WeakRefCounter->RawPtr : nullptr;

        if (Cur == rawPtr)
            return;

        RemoveWeakRef<T>();

        if (!rawPtr)
            return;

        m_WeakRefCounter = rawPtr->GetWeakRefCounter();
        if (!m_WeakRefCounter)
        {
            m_WeakRefCounter = AllocateWeakRefCounter();
            m_WeakRefCounter->RawPtr = rawPtr;
            m_WeakRefCounter->RefCount = 1;
            rawPtr->SetWeakRefCounter(m_WeakRefCounter);
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
        if HK_LIKELY(this != &rhs)
        {
            Reset();

            m_WeakRefCounter      = rhs.m_WeakRefCounter;
            rhs.m_WeakRefCounter = nullptr;
        }

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
    Ref<T> ptr;
    ptr.Attach(new T(std::forward<Args>(args)...));
    return ptr;
}

HK_NAMESPACE_END
