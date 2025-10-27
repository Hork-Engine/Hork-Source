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

#include "BaseTypes.h"

HK_NAMESPACE_BEGIN

template <typename T>
HK_FORCEINLINE void CheckedDelete(T* Ptr)
{
    using type_must_be_complete = char[sizeof(T) ? 1 : -1];
    (void)sizeof(type_must_be_complete);
    delete Ptr;
}

template <typename T>
class UniqueRef final
{
public:
    UniqueRef() noexcept = default;

    UniqueRef(std::nullptr_t) noexcept {}
    
    UniqueRef& operator=(std::nullptr_t)
    {
        Reset();
        return *this;
    }

    explicit UniqueRef(T* ptr) noexcept :
        m_RawPtr(ptr)
    {}

    UniqueRef(UniqueRef<T> const&) = delete;
    UniqueRef& operator=(UniqueRef<T> const&) = delete;

    template <typename U>
    UniqueRef(UniqueRef<U>&& rhs) noexcept :
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

    T* operator->() const noexcept
    {
        HK_ASSERT(m_RawPtr);
        return m_RawPtr;
    }

    T& operator*() const noexcept
    {
        HK_ASSERT(m_RawPtr);
        return *m_RawPtr;
    }

    template <typename U>
    bool operator==(UniqueRef<U> const& rhs) const noexcept
    {
        return m_RawPtr == rhs.m_RawPtr;
    }

    template <typename U>
    bool operator!=(UniqueRef<U> const& rhs) const noexcept
    {
        return m_RawPtr != rhs.m_RawPtr;
    }

    operator bool() const noexcept
    {
        return m_RawPtr != nullptr;
    }

    T* RawPtr() const noexcept
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
        if HK_LIKELY(m_RawPtr != ptr)
        {
            CheckedDelete(m_RawPtr);
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
};

template <typename T, typename U>
bool operator==(UniqueRef<T> const& a, U* b)
{
    return a.RawPtr() == b;
}

template <typename T, typename U>
bool operator!=(UniqueRef<T> const& a, U* b)
{
    return a.RawPtr() != b;
}

template <typename T, typename U>
bool operator==(T* a, UniqueRef<U> const& b)
{
    return a == b.RawPtr();
}

template <typename T, typename U>
bool operator!=(T* a, UniqueRef<U> const& b)
{
    return a != b.RawPtr();
}

template <typename T>
bool operator==(UniqueRef<T> const& a, std::nullptr_t)
{
    return a.RawPtr() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, UniqueRef<T> const& a)
{
    return a.RawPtr() == nullptr;
}

template <typename T>
bool operator!=(UniqueRef<T> const& a, std::nullptr_t)
{
    return a.RawPtr() != nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, UniqueRef<T> const& a)
{
    return a.RawPtr() != nullptr;
}


template <typename T, typename... Args>
UniqueRef<T> MakeUnique(Args&&... args)
{
    return UniqueRef<T>(new T(std::forward<Args>(args)...));
}

HK_NAMESPACE_END
