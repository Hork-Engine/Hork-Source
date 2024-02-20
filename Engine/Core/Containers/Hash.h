/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

/*

THashMap
TStringHashMap
TNameHash
THashSet

*/

#include <Engine/Core/String.h>
#include <Engine/Core/HashFunc.h>

HK_NAMESPACE_BEGIN

template <typename T>
struct Hasher
{
    std::size_t operator()(T const& Key) const
    {
        return HashTraits::Hash(Key);
    }
};

struct NameHasher
{
    std::size_t operator()(StringView ResourcePath) const
    {
        return ResourcePath.HashCaseInsensitive();
    }

    struct Compare
    {
        bool operator()(StringView Lhs, StringView Rhs) const
        {
            return !Lhs.Icmp(Rhs);
        }
    };
};

template <typename Key, typename Val, typename Hash = Hasher<Key>, typename Predicate = eastl::equal_to<Key>, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_HASH_MAP>, bool bCacheHashCode = false>
class THashMap : public eastl::hashtable<Key, eastl::pair<const Key, Val>, Allocator, eastl::use_first<eastl::pair<const Key, Val>>, Predicate, Hash, eastl::mod_range_hashing, eastl::default_ranged_hash, eastl::prime_rehash_policy, bCacheHashCode, true, true>
{
public:
    using Super            = eastl::hashtable<Key, eastl::pair<const Key, Val>, Allocator, eastl::use_first<eastl::pair<const Key, Val>>, Predicate, Hash, eastl::mod_range_hashing, eastl::default_ranged_hash, eastl::prime_rehash_policy, bCacheHashCode, true, true>;
    using Iterator         = typename Super::iterator;
    using ConstIterator    = typename Super::const_iterator;
    using SizeType         = typename Super::size_type;
    using InsertReturnType = typename Super::insert_return_type;
    using HashCode         = typename Super::hash_code_t;
    using AllocatorType    = typename Super::allocator_type;
    using ValueType        = typename Super::value_type;

    /** Default constructor. */
    explicit THashMap(AllocatorType const& allocator = AllocatorType("HashMap")) :
        Super(0, Hash(), eastl::mod_range_hashing(), eastl::default_ranged_hash(), Predicate(), eastl::use_first<eastl::pair<const Key, Val>>(), allocator)
    {}

    /** Constructor which creates an empty container, but start with nBucketCount buckets.
        We default to a small nBucketCount value, though the user really should manually
        specify an appropriate value in order to prevent memory from being reallocated. */
    explicit THashMap(SizeType nBucketCount, const Hash& hashFunction = Hash(), const Predicate& predicate = Predicate(), AllocatorType const& allocator = AllocatorType("HashMap")) :
        Super(nBucketCount, hashFunction, eastl::mod_range_hashing(), eastl::default_ranged_hash(), predicate, eastl::use_first<eastl::pair<const Key, Val>>(), allocator)
    {}

    THashMap(THashMap const& x) :
        Super(x)
    {}

    THashMap(THashMap&& x) :
        Super(eastl::move(x))
    {}

    THashMap(THashMap&& x, AllocatorType const& allocator) :
        Super(eastl::move(x), allocator)
    {}

    /** initializer_list - based constructor.
        Allows for initializing with brace values (e.g. THashMap<int, char*> hm = { {3,"c"}, {4,"d"}, {5,"e"} }; ) */
    THashMap(std::initializer_list<ValueType> ilist, SizeType nBucketCount = 0, Hash const& hashFunction = Hash(), Predicate const& predicate = Predicate(), AllocatorType const& allocator = AllocatorType("HashMap")) :
        Super(ilist.begin(), ilist.end(), nBucketCount, hashFunction, eastl::mod_range_hashing(), eastl::default_ranged_hash(), predicate, eastl::use_first<eastl::pair<const Key, Val>>(), allocator)
    {}

    /** An input bucket count of <= 1 causes the bucket count to be equal to the number of
        elements in the input range. */
    template <typename ForwardIterator>
    THashMap(ForwardIterator first, ForwardIterator last, SizeType nBucketCount = 0, Hash const& hashFunction = Hash(), Predicate const& predicate = Predicate(), AllocatorType const& allocator = AllocatorType("HashMap")) :
        Super(first, last, nBucketCount, hashFunction, eastl::mod_range_hashing(), eastl::default_ranged_hash(), predicate, eastl::use_first<eastl::pair<const Key, Val>>(), allocator)
    {}

    HK_FORCEINLINE THashMap& operator=(const THashMap& x)
    {
        return static_cast<THashMap&>(Super::operator=(x));
    }

    HK_FORCEINLINE THashMap& operator=(std::initializer_list<ValueType> ilist)
    {
        return static_cast<THashMap&>(Super::operator=(ilist));
    }

    HK_FORCEINLINE THashMap& operator=(THashMap&& x)
    {
        return static_cast<THashMap&>(Super::operator=(eastl::move(x)));
    }

    /** This is an extension to the C++ standard.We insert a default - constructed
        element with the given key. The reason for this is that we can avoid the
        potentially expensive operation of creating and/or copying a Val
        object on the stack. */
    HK_FORCEINLINE InsertReturnType Insert(Key const& key)
    {
        return Super::DoInsertKey(eastl::true_type(), key);
    }

    Val& At(Key const& k)
    {
        Iterator it = Super::find(k);
        HK_ASSERT(it != Super::end());
        return it->second;
    }

    Val const& At(Key const& k) const
    {
        ConstIterator it = Super::find(k);
        HK_ASSERT(it != Super::end());
        return it->second;
    }

    HK_FORCEINLINE InsertReturnType Insert(Key&& key)
    {
        return Super::DoInsertKey(eastl::true_type(), eastl::move(key));
    }

    HK_FORCEINLINE Val& operator[](Key const& key)
    {
        return (*Super::DoInsertKey(eastl::true_type(), key).first).second;
    }

    HK_FORCEINLINE Val& operator[](Key&& key)
    {
        // The Standard states that this function "inserts the value ValueType(std::move(key), Val())"
        return (*Super::DoInsertKey(eastl::true_type(), eastl::move(key)).first).second;
    }

    HK_FORCEINLINE void Reserve(SizeType elementCount)
    {
        Super::reserve(elementCount);
    }

    HK_FORCEINLINE SizeType Size() const
    {
        return Super::size();
    }

    HK_FORCEINLINE void Clear()
    {
        Super::clear();
    }

    HK_FORCEINLINE SizeType Count(Key const& k) const
    {
        return Super::count(k);
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return Super::empty();
    }

    template <class... Args> HK_FORCEINLINE InsertReturnType TryEmplace(Key const& k, Args&&... args)
    {
        return Super::try_emplace(k, std::forward<Args>(args)...);
    }

    template <class... Args> HK_FORCEINLINE InsertReturnType TryEmplace(Key&& k, Args&&... args)
    {
        return Super::try_emplace(std::move(k), std::forward<Args>(args)...);
    }

    template <class... Args> HK_FORCEINLINE InsertReturnType Emplace(Args&&... args)
    {
        return Super::emplace(std::forward<Args>(args)...);
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator i)
    {
        return Super::erase(i);
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator first, ConstIterator last)
    {
        return Super::erase(first, last);
    }

    HK_FORCEINLINE SizeType Erase(Key const& k)
    {
        return Super::erase(k);
    }

    HK_FORCEINLINE Iterator Find(Key const& k)
    {
        return Super::find(k);
    }

    HK_FORCEINLINE ConstIterator Find(Key const& k) const
    {
        return Super::find(k);
    }

    HK_FORCEINLINE Iterator FindByHash(Key const& k, HashCode c)
    {
        return Super::find_by_hash(k, c);
    }

    HK_FORCEINLINE ConstIterator FindByHash(Key const& k, HashCode c) const
    {
        return Super::find_by_hash(k, c);
    }

    HK_FORCEINLINE bool Contains(Key const& k) const
    {
        return Super::find(k) != Super::end();
    }

    HK_FORCEINLINE void ResetLoseMemory()
    {
        Super::reset_lose_memory();
    }

    HK_FORCEINLINE SizeType GetBucketCount() const
    {
        return Super::bucket_count();
    }

    HK_FORCEINLINE SizeType GetBucketSize(SizeType n) const
    {
        return Super::bucket_size(n);
    }

    HK_FORCEINLINE void Swap(THashMap& x)
    {
        Super::swap(x);
    }

    HK_INLINE bool operator==(THashMap const& rhs) const
    {
        if (Super::size() != rhs.size())
            return false;
        for (ConstIterator it = Super::begin(), e = Super::end(), e2 = rhs.end(); it != e; ++it)
        {
            ConstIterator it2 = rhs.find(it->first);
            if ((it2 == e2) || !(*it == *it2))
                return false;
        }
        return true;
    }

    HK_INLINE bool operator!=(THashMap const& rhs) const
    {
        return !(operator==(rhs));
    }

    HK_FORCEINLINE Iterator Begin()
    {
        return Super::begin();
    }
    HK_FORCEINLINE ConstIterator Begin() const
    {
        return Super::begin();
    }
    HK_FORCEINLINE ConstIterator CBegin() const
    {
        return Super::cbegin();
    }
    HK_FORCEINLINE Iterator End()
    {
        return Super::end();
    }
    HK_FORCEINLINE ConstIterator End() const
    {
        return Super::end();
    }
    HK_FORCEINLINE ConstIterator CEnd() const
    {
        return Super::cend();
    }
};

HK_NAMESPACE_END

namespace eastl
{
template <typename Key, typename Val, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
HK_INLINE void swap(Hk::THashMap<Key, Val, Hash, Predicate, Allocator, bCacheHashCode>& lhs, Hk::THashMap<Key, Val, Hash, Predicate, Allocator, bCacheHashCode>& rhs)
{
    lhs.Swap(rhs);
}
} // namespace eastl

HK_NAMESPACE_BEGIN

template <typename Val, typename Hash = Hasher<StringView>, typename Predicate = eastl::equal_to<StringView>, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_HASH_MAP>>
class TStringHashMap : public THashMap<StringView, Val, Hash, Predicate, Allocator>
{
public:
    using Super            = THashMap<StringView, Val, Hash, Predicate, Allocator>;
    using ThisType         = TStringHashMap<Val, Hash, Predicate, Allocator>;
    using AllocatorType    = typename Super::Super::allocator_type;
    using InsertReturnType = typename Super::Super::insert_return_type;
    using Iterator         = typename Super::Super::iterator;
    using ConstIterator = typename Super::Super::const_iterator;
    using SizeType      = typename Super::Super::size_type;
    using ValueType     = typename Super::Super::value_type;
    using MappedType    = Val;

    TStringHashMap(const AllocatorType& allocator = AllocatorType()) :
        Super(allocator) {}
    TStringHashMap(const TStringHashMap& src, const AllocatorType& allocator = AllocatorType());
    ~TStringHashMap();
    void Clear();
    void Clear(bool clearBuckets);

    ThisType& operator=(const ThisType& x);

    InsertReturnType            Insert(StringView key, const Val& value);
    InsertReturnType            Insert(StringView key);
    eastl::pair<Iterator, bool> InsertOrAssign(StringView key, const Val& value);
    Iterator                    Erase(ConstIterator position);
    SizeType                    Erase(StringView key);
    MappedType&                 operator[](StringView key);

private:
    StringView strduplicate(StringView str);
};


template <typename Val, typename Hash, typename Predicate, typename Allocator>
TStringHashMap<Val, Hash, Predicate, Allocator>::TStringHashMap(const TStringHashMap& src, const AllocatorType& allocator) :
    Super(allocator)
{
    for (ConstIterator i = src.begin(), e = src.end(); i != e; ++i)
        Super::Super::insert(eastl::make_pair(strduplicate(i->first), i->second));
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
TStringHashMap<Val, Hash, Predicate, Allocator>::~TStringHashMap()
{
    Clear();
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
void TStringHashMap<Val, Hash, Predicate, Allocator>::Clear()
{
    AllocatorType& allocator = Super::Super::get_allocator();
    for (ConstIterator i = Super::Super::begin(), e = Super::Super::end(); i != e; ++i)
        EASTLFree(allocator, (void*)i->first.ToPtr(), 0);
    Super::Super::clear();
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
void TStringHashMap<Val, Hash, Predicate, Allocator>::Clear(bool clearBuckets)
{
    AllocatorType& allocator = Super::Super::get_allocator();
    for (ConstIterator i = Super::Super::begin(), e = Super::Super::end(); i != e; ++i)
        EASTLFree(allocator, (void*)i->first.ToPtr(), 0);
    Super::Super::clear(clearBuckets);
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
typename TStringHashMap<Val, Hash, Predicate, Allocator>::ThisType&
TStringHashMap<Val, Hash, Predicate, Allocator>::operator=(const ThisType& x)
{
    AllocatorType allocator = Super::Super::get_allocator();
    this->~ThisType();
    new (this) ThisType(x, allocator);
    return *this;
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
typename TStringHashMap<Val, Hash, Predicate, Allocator>::InsertReturnType
TStringHashMap<Val, Hash, Predicate, Allocator>::Insert(StringView key)
{
    return Insert(key, MappedType());
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
typename TStringHashMap<Val, Hash, Predicate, Allocator>::InsertReturnType
TStringHashMap<Val, Hash, Predicate, Allocator>::Insert(StringView key, const Val& value)
{
    EASTL_ASSERT(key);
    Iterator i = Super::Super::find(key);
    if (i != Super::Super::end())
    {
        InsertReturnType ret;
        ret.first  = i;
        ret.second = false;
        return ret;
    }
    return Super::Super::insert(eastl::make_pair(strduplicate(key), value));
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
eastl::pair<typename TStringHashMap<Val, Hash, Predicate, Allocator>::Iterator, bool>
TStringHashMap<Val, Hash, Predicate, Allocator>::InsertOrAssign(StringView key, const Val& value)
{
    Iterator i = Super::Super::find(key);
    if (i != Super::Super::end())
    {
        return Super::Super::insert_or_assign(i->first, value);
    }
    else
    {
        return Super::Super::insert_or_assign(strduplicate(key), value);
    }
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
typename TStringHashMap<Val, Hash, Predicate, Allocator>::Iterator
TStringHashMap<Val, Hash, Predicate, Allocator>::Erase(ConstIterator position)
{
    StringView key    = position->first;
    Iterator    result = Super::Super::erase(position);
    EASTLFree(Super::Super::get_allocator(), (void*)key.ToPtr(), 0);
    return result;
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
typename TStringHashMap<Val, Hash, Predicate, Allocator>::SizeType
TStringHashMap<Val, Hash, Predicate, Allocator>::Erase(StringView key)
{
    const Iterator it(Super::Super::find(key));

    if (it != Super::Super::end())
    {
        Erase(it);
        return 1;
    }
    return 0;
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
typename TStringHashMap<Val, Hash, Predicate, Allocator>::MappedType&
TStringHashMap<Val, Hash, Predicate, Allocator>::operator[](StringView key)
{
    using base_value_type = typename Super::Super::value_type;

    EASTL_ASSERT(!key.IsEmpty());
    Iterator i = Super::Super::find(key);
    if (i != Super::Super::end())
        return i->second;
    return Super::Super::insert(base_value_type(eastl::pair_first_construct, strduplicate(key))).first->second;
}

template <typename Val, typename Hash, typename Predicate, typename Allocator>
StringView TStringHashMap<Val, Hash, Predicate, Allocator>::strduplicate(StringView str)
{
    size_t len    = str.Size();
    char*  result = (char*)EASTLAlloc(Super::Super::get_allocator(), len + 1);
    memcpy(result, str.ToPtr(), len);
    result[len] = 0;
    return StringView(result, len, true);
}

/** Case insensitive string hash map. */
template <typename T>
using TNameHash = TStringHashMap<T, NameHasher, NameHasher::Compare>;

template <typename Val, typename Hash = Hasher<Val>, typename Predicate = eastl::equal_to<Val>, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_HASH_SET>, bool bCacheHashCode = false>
class THashSet : public eastl::hashtable<Val, Val, Allocator, eastl::use_self<Val>, Predicate, Hash, eastl::mod_range_hashing, eastl::default_ranged_hash, eastl::prime_rehash_policy, bCacheHashCode, false, true>
{
public:
    using Super            = eastl::hashtable<Val, Val, Allocator, eastl::use_self<Val>, Predicate, Hash, eastl::mod_range_hashing, eastl::default_ranged_hash, eastl::prime_rehash_policy, bCacheHashCode, false, true>;
    using Iterator         = typename Super::iterator;
    using ConstIterator    = typename Super::const_iterator;
    using SizeType         = typename Super::size_type;
    using InsertReturnType = typename Super::insert_return_type;
    using HashCode         = typename Super::hash_code_t;
    using AllocatorType    = typename Super::allocator_type;
    using ValueType        = typename Super::value_type;

    /** Default constructor. */
    explicit THashSet(AllocatorType const& allocator = AllocatorType("HashSet")) :
        Super(0, Hash(), eastl::mod_range_hashing(), eastl::default_ranged_hash(), Predicate(), eastl::use_self<Val>(), allocator)
    {}

    /** Constructor which creates an empty container, but start with nBucketCount buckets.
        We default to a small nBucketCount value, though the user really should manually
        specify an appropriate value in order to prevent memory from being reallocated. */
    explicit THashSet(SizeType nBucketCount, Hash const& hashFunction = Hash(), Predicate const& predicate = Predicate(), AllocatorType const& allocator = AllocatorType("HashSet")) :
        Super(nBucketCount, hashFunction, eastl::mod_range_hashing(), eastl::default_ranged_hash(), predicate, eastl::use_self<Val>(), allocator)
    {}

    THashSet(THashSet const& x) :
        Super(x)
    {}

    THashSet(THashSet&& x) :
        Super(eastl::move(x))
    {}

    THashSet(THashSet&& x, AllocatorType const& allocator) :
        Super(eastl::move(x), allocator)
    {}

    /** initializer_list - based constructor.
        Allows for initializing with brace values (e.g. THashSet<int> hs = { 3, 4, 5, }; ) */
    THashSet(std::initializer_list<ValueType> ilist, SizeType nBucketCount = 0, Hash const& hashFunction = Hash(), Predicate const& predicate = Predicate(), AllocatorType const& allocator = AllocatorType("HashSet")) :
        Super(ilist.begin(), ilist.end(), nBucketCount, hashFunction, eastl::mod_range_hashing(), eastl::default_ranged_hash(), predicate, eastl::use_self<Val>(), allocator)
    {}

    /** An input bucket count of <= 1 causes the bucket count to be equal to the number of
        elements in the input range. */
    template <typename FowardIterator>
    THashSet(FowardIterator first, FowardIterator last, SizeType nBucketCount = 0, const Hash& hashFunction = Hash(), Predicate const& predicate = Predicate(), AllocatorType const& allocator = AllocatorType("HashSet")) :
        Super(first, last, nBucketCount, hashFunction, eastl::mod_range_hashing(), eastl::default_ranged_hash(), predicate, eastl::use_self<Val>(), allocator)
    {}

    THashSet& operator=(THashSet const& x)
    {
        return static_cast<THashSet&>(Super::operator=(x));
    }

    THashSet& operator=(std::initializer_list<ValueType> ilist)
    {
        return static_cast<THashSet&>(Super::operator=(ilist));
    }

    THashSet& operator=(THashSet&& x)
    {
        return static_cast<THashSet&>(Super::operator=(eastl::move(x)));
    }

    HK_FORCEINLINE void Reserve(SizeType elementCount)
    {
        Super::reserve(elementCount);
    }

    HK_FORCEINLINE SizeType Size() const
    {
        return Super::size();
    }

    HK_FORCEINLINE void Clear()
    {
        Super::clear();
    }

    HK_FORCEINLINE SizeType Count(Val const& k) const
    {
        return Super::count(k);
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return Super::empty();
    }

    HK_FORCEINLINE InsertReturnType Insert(Val const& v)
    {
        return Super::insert(v);
    }

    HK_FORCEINLINE InsertReturnType Insert(Val&& v)
    {
        return Super::insert(std::move(v));
    }

    template <class... Args> HK_FORCEINLINE InsertReturnType TryEmplace(Val const& k, Args&&... args)
    {
        return Super::try_emplace(k, std::forward<Args>(args)...);
    }

    template <class... Args> HK_FORCEINLINE InsertReturnType TryEmplace(Val&& k, Args&&... args)
    {
        return Super::try_emplace(std::move(k), std::forward<Args>(args)...);
    }

    template <class... Args> HK_FORCEINLINE InsertReturnType Emplace(Args&&... args)
    {
        return Super::emplace(std::forward<Args>(args)...);
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator i)
    {
        return Super::erase(i);
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator first, ConstIterator last)
    {
        return Super::erase(first, last);
    }

    HK_FORCEINLINE SizeType Erase(Val const& v)
    {
        return Super::erase(v);
    }

    HK_FORCEINLINE Iterator Find(Val const& v)
    {
        return Super::find(v);
    }

    HK_FORCEINLINE ConstIterator Find(Val const& v) const
    {
        return Super::find(v);
    }

    HK_FORCEINLINE Iterator FindByHash(HashCode c)
    {
        return Super::find_by_hash(c);
    }

    HK_FORCEINLINE ConstIterator FindByHash(HashCode c) const
    {
        return Super::find_by_hash(c);
    }

    HK_FORCEINLINE bool Contains(Val const& v) const
    {
        return Super::find(v) != Super::end();
    }

    HK_FORCEINLINE void ResetLoseMemory()
    {
        Super::reset_lose_memory();
    }

    HK_FORCEINLINE SizeType GetBucketCount() const
    {
        return Super::bucket_count();
    }

    HK_FORCEINLINE SizeType GetBucketSize(SizeType n) const
    {
        return Super::bucket_size(n);
    }

    HK_FORCEINLINE void Swap(THashSet& x)
    {
        Super::swap(x);
    }

    HK_INLINE bool operator==(THashSet const& rhs) const
    {
        if (Super::size() != rhs.size())
            return false;
        for (ConstIterator it = Super::begin(), e = Super::end(), e2 = rhs.end(); it != e; ++it)
        {
            ConstIterator it2 = rhs.find(*it);
            if ((it2 == e2) || !(*it == *it2))
                return false;
        }
        return true;
    }

    HK_INLINE bool operator!=(THashSet const& rhs) const
    {
        return !(operator==(rhs));
    }

    HK_FORCEINLINE Iterator Begin()
    {
        return Super::begin();
    }
    HK_FORCEINLINE ConstIterator Begin() const
    {
        return Super::begin();
    }
    HK_FORCEINLINE ConstIterator CBegin() const
    {
        return Super::cbegin();
    }
    HK_FORCEINLINE Iterator End()
    {
        return Super::end();
    }
    HK_FORCEINLINE ConstIterator End() const
    {
        return Super::end();
    }
    HK_FORCEINLINE ConstIterator CEnd() const
    {
        return Super::cend();
    }
};

HK_NAMESPACE_END

namespace eastl
{
template <typename Val, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
HK_INLINE void swap(Hk::THashSet<Val, Hash, Predicate, Allocator, bCacheHashCode>& lhs, Hk::THashSet<Val, Hash, Predicate, Allocator, bCacheHashCode>& rhs)
{
    lhs.Swap(rhs);
}
} // namespace eastl
