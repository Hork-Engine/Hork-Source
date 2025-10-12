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

#include <Hork/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

template <typename T>
class IntrusiveRef
{
public:
    IntrusiveRef() noexcept : m_RawPtr(nullptr) {}
    IntrusiveRef(T* p) : m_RawPtr(p)
    {
        if (p)
            IntrusiveRef_AddRef(m_RawPtr);
    }
    IntrusiveRef(IntrusiveRef const& rhs) : m_RawPtr(rhs.m_RawPtr)
    {
        if (m_RawPtr)
            IntrusiveRef_AddRef(m_RawPtr);
    }
    IntrusiveRef(IntrusiveRef&& rhs) noexcept :
        m_RawPtr(rhs.m_RawPtr)
    {
        rhs.m_RawPtr = nullptr;
    }

    template <typename U>
    IntrusiveRef(IntrusiveRef<U> const& rhs) : m_RawPtr(rhs.RawPtr())
    {
        if (m_RawPtr)
            IntrusiveRef_AddRef(m_RawPtr);
    }

    ~IntrusiveRef()
    {
        if (m_RawPtr)
            IntrusiveRef_RemoveRef(m_RawPtr);
    }

    IntrusiveRef& operator=(IntrusiveRef&& rhs) noexcept
    {
        if HK_LIKELY(this != &rhs)
        {
            if (m_RawPtr)
                IntrusiveRef_RemoveRef(m_RawPtr);
            m_RawPtr = rhs.m_RawPtr;
            rhs.m_RawPtr = nullptr;
        }
        return *this;
    }

    IntrusiveRef& operator=(T* rhs)
    {
        if (HK_LIKELY(m_RawPtr != rhs))
            IntrusiveRef(rhs).Swap(*this);
        return *this;
    }

    IntrusiveRef& operator=(IntrusiveRef const& rhs)
    {
        if (HK_LIKELY(this != &rhs))
            IntrusiveRef(rhs).Swap(*this);
        return *this;
    }

    template <typename U> IntrusiveRef& operator=(IntrusiveRef<U> const& rhs)
    {
        if (HK_LIKELY(m_RawPtr != rhs.RawPtr()))
            IntrusiveRef(rhs).Swap(*this);
        return *this;
    }

    operator bool() const noexcept
    {
        return m_RawPtr != nullptr;
    }

    bool operator!() const noexcept
    {
        return m_RawPtr == nullptr;
    }

    void Reset()
    {
        if (m_RawPtr)
        {
            IntrusiveRef_RemoveRef(m_RawPtr);
            m_RawPtr = nullptr;
        }
    }

    void Reset(T* rhs)
    {
        if (HK_LIKELY(m_RawPtr != rhs))
            IntrusiveRef(rhs).Swap(*this);
    }

    T& operator*() const noexcept
    {
        HK_ASSERT(m_RawPtr);
        return *m_RawPtr;
    }

    T* operator->() const noexcept
    {
        HK_ASSERT(m_RawPtr);
        return m_RawPtr;
    }

    T* RawPtr() const noexcept
    {
        return m_RawPtr;
    }

    void Swap(IntrusiveRef& rhs) noexcept
    {
        std::swap(m_RawPtr, rhs.m_RawPtr);
    }

private:
    T* m_RawPtr;
};

template<class T, class U>
bool operator==(IntrusiveRef<T> const& a, IntrusiveRef<U> const& b)
{
    return a.RawPtr() == b.RawPtr();
}

template<class T, class U>
bool operator!=(IntrusiveRef<T> const& a, IntrusiveRef<U> const& b)
{
    return a.RawPtr() != b.RawPtr();
}

template<class T, class U>
bool operator==(IntrusiveRef<T> const& a, U* b)
{
    return a.RawPtr() == b;
}

template<class T, class U>
bool operator!=(IntrusiveRef<T> const& a, U* b)
{
    return a.RawPtr() != b;
}

template<class T, class U>
bool operator==(T* a, IntrusiveRef<U> const& b)
{
    return a == b.RawPtr();
}

template<class T, class U>
bool operator!=(T* a, IntrusiveRef<U> const& b)
{
    return a != b.RawPtr();
}

template<class T>
bool operator==(IntrusiveRef<T> const& a, std::nullptr_t)
{
    return a.RawPtr() == nullptr;
}

template<class T>
bool operator==(std::nullptr_t, IntrusiveRef<T> const& a)
{
    return a.RawPtr() == nullptr;
}

template<class T>
bool operator!=(IntrusiveRef<T> const& a, std::nullptr_t)
{
    return a.RawPtr() != nullptr;
}

template<class T>
bool operator!=(std::nullptr_t, IntrusiveRef<T> const& a)
{
    return a.RawPtr() != nullptr;
}

template<class T, class U>
bool operator<(IntrusiveRef<T> const & a, IntrusiveRef<U> const & b)
{
    return std::less<T*>()(a.RawPtr(), b.RawPtr());
}

template<class T, class U>
IntrusiveRef<T> static_pointer_cast(IntrusiveRef<U> const& r)
{
    return IntrusiveRef<T>(static_cast<T*>(r.RawPtr()));
}

template<class T, class U>
IntrusiveRef<T> const_pointer_cast(IntrusiveRef<U> const& r)
{
    return IntrusiveRef<T>(const_cast<T*>(r.RawPtr()));
}


template<class T, class U>
IntrusiveRef<T> dynamic_pointer_cast(IntrusiveRef<U> const& r)
{
    return IntrusiveRef<T>(dynamic_cast<T*>(r.RawPtr()));
}

namespace Core
{

template<class T>
void Swap(IntrusiveRef<T>& a, IntrusiveRef<T>& b)
{
    a.Swap(b);
}

}

HK_NAMESPACE_END
