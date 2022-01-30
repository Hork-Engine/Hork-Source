/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
#include "StdVector.h"

template <int HashBucketsCount = 1024, typename Allocator = AZoneAllocator> // must be power of two
class THash final
{
public:
    /** Change it to set up granularity. Granularity must be >= 1 */
    int Granularity = 1024;

    /** Change it to reallocate indices */
    int NumBucketIndices = 0;

    THash() :
        HashBuckets(const_cast<int*>(InvalidHashIndex)),
        IndexChain(const_cast<int*>(InvalidHashIndex))
    {
        static_assert(IsPowerOfTwo(HashBucketsCount), "Buckets count must be power of two");
    }

    ~THash()
    {
        if (HashBuckets != InvalidHashIndex)
        {
            Allocator::Inst().Free(HashBuckets);
        }

        if (IndexChain != InvalidHashIndex)
        {
            Allocator::Inst().Free(IndexChain);
        }
    }

    THash(THash const& Rhs) :
        Granularity(Rhs.Granularity),
        NumBucketIndices(Rhs.NumBucketIndices),
        HashBuckets(const_cast<int*>(InvalidHashIndex)),
        IndexChain(const_cast<int*>(InvalidHashIndex)),
        LookupMask(Rhs.LookupMask),
        IndexChainLength(Rhs.IndexChainLength)        
    {
        if (Rhs.IndexChain != InvalidHashIndex)
        {
            IndexChain = (int*)Allocator::Inst().Alloc(IndexChainLength * sizeof(*IndexChain));
            Platform::Memcpy(IndexChain, Rhs.IndexChain, IndexChainLength * sizeof(*IndexChain));
        }

        if (Rhs.HashBuckets != InvalidHashIndex)
        {
            HashBuckets = (int*)Allocator::Inst().Alloc(HashBucketsCount * sizeof(*HashBuckets));
            Platform::Memcpy(HashBuckets, Rhs.HashBuckets, HashBucketsCount * sizeof(*HashBuckets));
        }
    }

    THash(THash&& Rhs) noexcept :
        Granularity(Rhs.Granularity),
        NumBucketIndices(Rhs.NumBucketIndices),
        HashBuckets(Rhs.HashBuckets),
        IndexChain(Rhs.IndexChain),
        LookupMask(Rhs.LookupMask),
        IndexChainLength(Rhs.IndexChainLength)
    {
        HashBuckets      = const_cast<int*>(InvalidHashIndex);
        IndexChain       = const_cast<int*>(InvalidHashIndex);
        LookupMask       = 0;
        IndexChainLength = 0;
    }

    THash& operator=(THash const& Rhs)
    {
        Granularity      = Rhs.Granularity;
        NumBucketIndices = Rhs.NumBucketIndices;
        LookupMask       = Rhs.LookupMask;
        IndexChainLength = Rhs.IndexChainLength;

        if (Rhs.IndexChain != InvalidHashIndex)
        {
            if (IndexChain == InvalidHashIndex)
                IndexChain = (int*)Allocator::Inst().Alloc(IndexChainLength * sizeof(*IndexChain));
            else
                IndexChain = (int*)Allocator::Inst().Realloc(IndexChain, IndexChainLength * sizeof(*IndexChain), false);

            Platform::Memcpy(IndexChain, Rhs.IndexChain, IndexChainLength * sizeof(*IndexChain));
        }
        else
        {
            if (IndexChain != InvalidHashIndex)
            {
                Allocator::Inst().Free(IndexChain);
                IndexChain = const_cast<int*>(InvalidHashIndex);
            }
        }        

        if (Rhs.HashBuckets != InvalidHashIndex)
        {
            if (HashBuckets == InvalidHashIndex)
            {
                // first allocation
                HashBuckets = (int*)Allocator::Inst().Alloc(HashBucketsCount * sizeof(*HashBuckets));
            }

            Platform::Memcpy(HashBuckets, Rhs.HashBuckets, HashBucketsCount * sizeof(*HashBuckets));
        }
        else
        {
            if (HashBuckets != InvalidHashIndex)
            {
                Allocator::Inst().Free(HashBuckets);
                HashBuckets = const_cast<int*>(InvalidHashIndex);
            }
        }

        return *this;
    }

    THash& operator=(THash&& Rhs) noexcept
    {
        if (HashBuckets != InvalidHashIndex)
        {
            Allocator::Inst().Free(HashBuckets);
        }

        if (IndexChain != InvalidHashIndex)
        {
            Allocator::Inst().Free(IndexChain);
        }

        Granularity      = Rhs.Granularity;
        NumBucketIndices = Rhs.NumBucketIndices;
        HashBuckets      = Rhs.HashBuckets;
        IndexChain       = Rhs.IndexChain;
        LookupMask       = Rhs.LookupMask;
        IndexChainLength = Rhs.IndexChainLength;

        Rhs.HashBuckets      = const_cast<int*>(InvalidHashIndex);
        Rhs.IndexChain       = const_cast<int*>(InvalidHashIndex);
        Rhs.LookupMask       = 0;
        Rhs.IndexChainLength = 0;

        return *this;
    }

    void Clear()
    {
        if (HashBuckets != InvalidHashIndex)
        {
            Platform::Memset(HashBuckets, 0xff, HashBucketsCount * sizeof(*HashBuckets));
        }
    }

    void Free()
    {
        if (HashBuckets != InvalidHashIndex)
        {
            Allocator::Inst().Free(HashBuckets);
            HashBuckets = const_cast<int*>(InvalidHashIndex);
        }

        if (IndexChain != InvalidHashIndex)
        {
            Allocator::Inst().Free(IndexChain);
            IndexChain = const_cast<int*>(InvalidHashIndex);
        }

        LookupMask       = 0;
        IndexChainLength = 0;
    }

    void Insert(int _Key, int _Index)
    {

        if (HashBuckets == InvalidHashIndex)
        {
            // first allocation
            //HashBuckets = ( int * )Allocator::Inst().AllocCleared1( HashBucketsCount * sizeof( *HashBuckets ), 0xffffffffffffffff );
            HashBuckets = (int*)Allocator::Inst().Alloc(HashBucketsCount * sizeof(*HashBuckets));
            Platform::Memset(HashBuckets, 0xff, HashBucketsCount * sizeof(*HashBuckets));
            LookupMask = ~0;
        }

        if (NumBucketIndices > IndexChainLength)
        {
            GrowIndexChain(NumBucketIndices);
        }

        if (_Index >= IndexChainLength)
        {
            // Resize index chain

            int       newIndexChainLength = _Index + 1;
            const int mod                 = newIndexChainLength % Granularity;
            newIndexChainLength           = mod == 0 ? newIndexChainLength : newIndexChainLength + Granularity - mod;

            GrowIndexChain(newIndexChainLength);
        }

        _Key &= HashWrapAroundMask;
        IndexChain[_Index] = HashBuckets[_Key];
        HashBuckets[_Key]  = _Index;
    }

    void Remove(int _Key, int _Index)
    {
        AN_ASSERT(_Index < IndexChainLength);

        if (HashBuckets == InvalidHashIndex)
        {
            // Hash not allocated yet
            return;
        }

        _Key &= HashWrapAroundMask;
        if (HashBuckets[_Key] == _Index)
        {
            HashBuckets[_Key] = IndexChain[_Index];
        }
        else
        {
            for (int i = HashBuckets[_Key]; i != -1; i = IndexChain[i])
            {
                if (IndexChain[i] == _Index)
                {
                    IndexChain[i] = IndexChain[_Index];
                    break;
                }
            }
        }

        IndexChain[_Index] = -1;
    }

    void InsertIndex(int _Key, int _Index)
    {
        if (HashBuckets != InvalidHashIndex)
        {
            int max = _Index;
            for (int i = 0; i < HashBucketsCount; i++)
            {
                if (HashBuckets[i] >= _Index)
                {
                    HashBuckets[i]++;
                    max = std::max(max, HashBuckets[i]);
                }
            }
            for (int i = 0; i < IndexChainLength; i++)
            {
                if (IndexChain[i] >= _Index)
                {
                    IndexChain[i]++;
                    max = std::max(max, IndexChain[i]);
                }
            }
            if (max >= IndexChainLength)
            {
                int       newIndexChainLength = max + 1;
                const int mod                 = newIndexChainLength % Granularity;
                newIndexChainLength           = mod == 0 ? newIndexChainLength : newIndexChainLength + Granularity - mod;

                GrowIndexChain(newIndexChainLength);
            }
            for (int i = max; i > _Index; i--)
            {
                IndexChain[i] = IndexChain[i - 1];
            }
            IndexChain[_Index] = -1;
        }
        Insert(_Key, _Index);
    }

    void RemoveIndex(int _Key, int _Index)
    {
        Remove(_Key, _Index);

        if (HashBuckets == InvalidHashIndex)
        {
            return;
        }

        int max = _Index;
        for (int i = 0; i < HashBucketsCount; i++)
        {
            if (HashBuckets[i] >= _Index)
            {
                max = std::max(max, HashBuckets[i]);
                HashBuckets[i]--;
            }
        }
        for (int i = 0; i < IndexChainLength; i++)
        {
            if (IndexChain[i] >= _Index)
            {
                max = std::max(max, IndexChain[i]);
                IndexChain[i]--;
            }
        }

        AN_ASSERT(max < IndexChainLength);

        for (int i = _Index; i < max; i++)
        {
            IndexChain[i] = IndexChain[i + 1];
        }
        IndexChain[max] = -1;
    }

    int First(int _Key) const
    {
        return HashBuckets[_Key & HashWrapAroundMask & LookupMask];
    }

    int Next(int _Index) const
    {
        AN_ASSERT(_Index < IndexChainLength);
        return IndexChain[_Index & LookupMask];
    }

    bool IsAllocated() const
    {
        return /*HashBuckets != nullptr && */ HashBuckets != InvalidHashIndex;
    }

private:
    void GrowIndexChain(int _NewIndexChainLength)
    {
        if (IndexChain == InvalidHashIndex)
        {
            IndexChain = (int*)Allocator::Inst().Alloc(_NewIndexChainLength * sizeof(*IndexChain));
            Platform::Memset(IndexChain, 0xff, _NewIndexChainLength * sizeof(*IndexChain));
        }
        else
        {
            IndexChain = (int*)Allocator::Inst().Realloc(IndexChain, _NewIndexChainLength * sizeof(*IndexChain), true);

            int* pIndexChain = IndexChain + IndexChainLength * sizeof(*IndexChain);
            Platform::Memset(pIndexChain, 0xff, (_NewIndexChainLength - IndexChainLength) * sizeof(*IndexChain));
        }
        IndexChainLength = _NewIndexChainLength;
    }

    int* HashBuckets;
    int* IndexChain;
    int  LookupMask       = 0; // Always 0 or ~0
    int  IndexChainLength = 0;

    constexpr static const int HashWrapAroundMask  = HashBucketsCount - 1;
    constexpr static const int InvalidHashIndex[1] = {-1};
};

template <int HashBucketsCount, typename Allocator>
constexpr const int THash<HashBucketsCount, Allocator>::InvalidHashIndex[1];

template <typename KeyType, typename ValueType, int HashBucketsCount = 1024, typename Allocator = AZoneAllocator>
struct THashContainer
{
    THash<HashBucketsCount, Allocator>        Hash;
    TStdVector<std::pair<KeyType, ValueType>> Container;

    std::pair<KeyType, ValueType>& operator[](int Index)
    {
        return Container[Index];
    }

    std::pair<KeyType, ValueType> const& operator[](int Index) const
    {
        return Container[Index];
    }

    bool IsEmpty() const
    {
        return Container.IsEmpty();
    }

    int First(int KeyHash) const
    {
        return Hash.First(KeyHash);
    }

    int Next(int Index) const
    {
        return Hash.Next(Index);
    }

    template <typename KeyTypeView>
    void Insert(KeyTypeView const& Key, ValueType const& Value)
    {
        int hash = Key.Hash();
        for (int i = Hash.First(hash); i != -1; i = Hash.Next(i))
        {
            if (Container[i].first == Key)
            {
                Container[i].second = Value;
                return;
            }
        }
        Hash.Insert(hash, Container.Size());
        Container.Append(std::make_pair(Key, Value));
    }
};
