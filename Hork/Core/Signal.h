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

#include "Containers/Vector.h"
#include "Delegate.h"

HK_NAMESPACE_BEGIN

template <typename... Args>
struct Signal
{
    Signal() = default;
    Signal(Signal const& rhs) = default;
    Signal(Signal&& rhs) = default;

    Signal& operator=(Signal const& rhs) = default;
    Signal& operator=(Signal&& rhs) = default;

    template <typename T>
    void Add(T* pthis, void (T::*method)(Args...))
    {
        m_Delegates.EmplaceBack(pthis, method);
    }

    template <typename T>
    void Remove(T* pthis, void (T::*method)(Args...))
    {
        auto i = m_Delegates.IndexOf(MakeDelegate(pthis, method));
        if (i != Core::NPOS)
            m_Delegates.Remove(i);
    }

    void RemoveAll()
    {
        m_Delegates.Clear();
    }

    bool HasCallbacks() const
    {
        return !m_Delegates.IsEmpty();
    }

    operator bool() const
    {
        return HasCallbacks();
    }

    void Dispatch(Args&&... args)
    {
        for (auto it = m_Delegates.Begin(); it != m_Delegates.End();)
            (*(it++))(std::forward<Args>(args)...);
    }

private:
    Vector<Delegate<void(Args...)>> m_Delegates;
};

HK_NAMESPACE_END
