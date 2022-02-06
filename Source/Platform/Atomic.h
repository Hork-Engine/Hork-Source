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

#include "BaseTypes.h"

#include <atomic>

template <typename T>
class TAtomic
{
    HK_FORBID_COPY(TAtomic)

public:
    using AtomicType = T;

    HK_FORCEINLINE TAtomic() {}
    HK_FORCEINLINE explicit TAtomic(AtomicType const& _i) :
        i(_i) {}

    HK_FORCEINLINE AtomicType LoadRelaxed() const
    {
        return i.load(std::memory_order_relaxed);
    }

    HK_FORCEINLINE void StoreRelaxed(AtomicType _i)
    {
        i.store(_i, std::memory_order_relaxed);
    }

    HK_FORCEINLINE AtomicType Load() const
    {
        return i.load(std::memory_order_acquire);
    }

    HK_FORCEINLINE void Store(AtomicType _i)
    {
        i.store(_i, std::memory_order_release);
    }

    HK_FORCEINLINE AtomicType Increment()
    {
        return i.fetch_add(1, std::memory_order_acquire) + 1;
    }

    HK_FORCEINLINE AtomicType FetchIncrement()
    {
        return i.fetch_add(1, std::memory_order_acquire);
    }

    HK_FORCEINLINE AtomicType Decrement()
    {
        return i.fetch_sub(1, std::memory_order_release) - 1;
    }

    HK_FORCEINLINE AtomicType FetchDecrement()
    {
        return i.fetch_sub(1, std::memory_order_release);
    }

    HK_FORCEINLINE AtomicType Add(AtomicType _i)
    {
        return i.fetch_add(_i) + _i;
    }

    HK_FORCEINLINE AtomicType FetchAdd(AtomicType _i)
    {
        return i.fetch_add(_i);
    }

    HK_FORCEINLINE AtomicType Sub(AtomicType _i)
    {
        return i.fetch_sub(_i) - _i;
    }

    HK_FORCEINLINE AtomicType FetchSub(AtomicType _i)
    {
        return i.fetch_sub(_i);
    }

    HK_FORCEINLINE AtomicType And(AtomicType _i)
    {
        return i.fetch_and(_i) & _i;
    }

    AtomicType FetchAnd(AtomicType _i)
    {
        return i.fetch_and(_i);
    }

    HK_FORCEINLINE AtomicType Or(AtomicType _i)
    {
        return i.fetch_or(_i) | _i;
    }

    HK_FORCEINLINE AtomicType FetchOr(AtomicType _i)
    {
        return i.fetch_or(_i);
    }

    HK_FORCEINLINE AtomicType Xor(AtomicType _i)
    {
        return i.fetch_xor(_i) ^ _i;
    }

    HK_FORCEINLINE AtomicType FetchXor(AtomicType _i)
    {
        return i.fetch_xor(_i);
    }

    HK_FORCEINLINE AtomicType Exchange(AtomicType _Exchange)
    {
        return i.exchange(_Exchange);
    }

    HK_FORCEINLINE bool CompareExchangeStrong(AtomicType& _Exp, AtomicType _Value)
    {
        return i.compare_exchange_strong(_Exp, _Value);
    }

    HK_FORCEINLINE bool CompareExchangeWeak(AtomicType& _Exp, AtomicType _Value)
    {
        return i.compare_exchange_weak(_Exp, _Value);
    }

private:
    std::atomic<AtomicType> i;
};

using AAtomicShort = TAtomic<int16_t>;
using AAtomicInt   = TAtomic<int32_t>;
using AAtomicLong  = TAtomic<int64_t>;
using AAtomicBool  = TAtomic<bool>;
