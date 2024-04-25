#pragma once

#include <Engine/Core/Thread.h>

#include <queue>

HK_NAMESPACE_BEGIN

template <typename T>
class ThreadSafeQueue
{
public:
    void Push(T const& v)
    {
        MutexGuard lock(m_Mutex);
        m_Data.push(v);
    }

    bool TryPop(T& v)
    {
        MutexGuard lock(m_Mutex);

        if (m_Data.empty())
            return false;

        v = std::move(m_Data.front());
        m_Data.pop();

        return true;
    }

private:
    std::queue<T> m_Data;
    Mutex m_Mutex;
};

HK_NAMESPACE_END
