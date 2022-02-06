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

#include <Platform/BaseTypes.h>

#include <unordered_map>
#include <unordered_set>

template <typename T>
struct SUnorderedMapHasher
{
    std::size_t operator()(T const& Key) const
    {
        return HashTraits::Hash(Key);
    }
};

template <typename Key, typename Value>
using TStdHashMap = std::unordered_map<Key, Value, SUnorderedMapHasher<Key>>;

template <typename Key>
using TStdHashSet = std::unordered_set<Key, SUnorderedMapHasher<Key>>;

namespace HashTraits
{

HK_FORCEINLINE std::size_t Hash(uint64_t Key)
{
    return std::hash<uint64_t>()(Key);
}

template <typename T>
HK_FORCEINLINE std::size_t Hash(T const& Key)
{
    return Key.Hash();
}

} // namespace HashTraits
