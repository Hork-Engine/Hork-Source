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

#include <Platform/Memory/Memory.h>

namespace Core
{
static constexpr size_t NPOS = (size_t)-1;
}


#define DEFINE_BEGIN_END(METHOD_SPECIFIERS)                \
    METHOD_SPECIFIERS Iterator begin()                     \
    {                                                      \
        return Super::begin();                             \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator begin() const          \
    {                                                      \
        return Super::begin();                             \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator cbegin() const         \
    {                                                      \
        return Super::cbegin();                            \
    }                                                      \
    METHOD_SPECIFIERS Iterator end()                       \
    {                                                      \
        return Super::end();                               \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator end() const            \
    {                                                      \
        return Super::end();                               \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator cend() const           \
    {                                                      \
        return Super::cend();                              \
    }                                                      \
    METHOD_SPECIFIERS ReverseIterator rbegin()             \
    {                                                      \
        return Super::rbegin();                            \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator rbegin() const  \
    {                                                      \
        return Super::rbegin();                            \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator crbegin() const \
    {                                                      \
        return Super::crbegin();                           \
    }                                                      \
    METHOD_SPECIFIERS ReverseIterator rend()               \
    {                                                      \
        return Super::rend();                              \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator rend() const    \
    {                                                      \
        return Super::rend();                              \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator crend() const   \
    {                                                      \
        return Super::crend();                             \
    }                                                      \
    METHOD_SPECIFIERS Iterator Begin()                     \
    {                                                      \
        return Super::begin();                             \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator Begin() const          \
    {                                                      \
        return Super::begin();                             \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator CBegin() const         \
    {                                                      \
        return Super::cbegin();                            \
    }                                                      \
    METHOD_SPECIFIERS Iterator End()                       \
    {                                                      \
        return Super::end();                               \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator End() const            \
    {                                                      \
        return Super::end();                               \
    }                                                      \
    METHOD_SPECIFIERS ConstIterator CEnd() const           \
    {                                                      \
        return Super::cend();                              \
    }                                                      \
    METHOD_SPECIFIERS ReverseIterator RBegin()             \
    {                                                      \
        return Super::rbegin();                            \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator RBegin() const  \
    {                                                      \
        return Super::rbegin();                            \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator CRBegin() const \
    {                                                      \
        return Super::crbegin();                           \
    }                                                      \
    METHOD_SPECIFIERS ReverseIterator REnd()               \
    {                                                      \
        return Super::rend();                              \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator REnd() const    \
    {                                                      \
        return Super::rend();                              \
    }                                                      \
    METHOD_SPECIFIERS ConstReverseIterator CREnd() const   \
    {                                                      \
        return Super::crend();                             \
    }


template <typename T, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_VECTOR>>
class TVector : private eastl::vector<T, Allocator>
{
public:
    using Super                = eastl::vector<T, Allocator>;
    using ThisType             = TVector<T, Allocator>;
    using ValueType            = T;
    using Pointer              = T*;
    using ConstPointer         = const T*;
    using Reference            = T&;
    using ConstReference       = const T&;
    using Iterator             = T*;
    using ConstIterator        = const T*;
    using ReverseIterator      = eastl::reverse_iterator<Iterator>;
    using ConstReverseIterator = eastl::reverse_iterator<ConstIterator>;
    using SizeType             = typename Super::size_type;
    using AllocatorType        = typename Super::allocator_type;

public:
    HK_FORCEINLINE TVector()
    {}

    HK_FORCEINLINE explicit TVector(AllocatorType const& allocator) :
        Super(allocator)
    {}

    HK_FORCEINLINE explicit TVector(SizeType n, AllocatorType const& allocator = AllocatorType("Vector")) :
        Super(n, allocator)
    {}

    HK_FORCEINLINE TVector(SizeType n, ValueType const& value, AllocatorType const& allocator = AllocatorType("Vector")) :
        Super(n, value, allocator)
    {}

    HK_FORCEINLINE TVector(ThisType const& x) :
        Super(x)
    {}

    HK_FORCEINLINE TVector(ThisType const& x, AllocatorType const& allocator) :
        Super(x, allocator)
    {}

    HK_FORCEINLINE TVector(ThisType&& x) :
        Super(std::forward<ThisType>(x))
    {}

    HK_FORCEINLINE TVector(ThisType&& x, AllocatorType const& allocator) :
        Super(std::forward<ThisType>(x), allocator)
    {}

    HK_FORCEINLINE TVector(std::initializer_list<ValueType> ilist, AllocatorType const& allocator = AllocatorType("Vector")) :
        Super(ilist, allocator)
    {}

    template <typename InputIterator>
    HK_FORCEINLINE TVector(InputIterator first, InputIterator last, AllocatorType const& allocator = AllocatorType("Vector")) :
        Super(first, last, allocator)
    {}

    HK_FORCEINLINE ThisType& operator=(ThisType const& x)
    {
        return static_cast<ThisType&>(Super::operator=(x));
    }

    HK_FORCEINLINE ThisType& operator=(std::initializer_list<ValueType> ilist)
    {
        return static_cast<ThisType&>(Super::operator=(ilist));
    }

    HK_FORCEINLINE ThisType& operator=(ThisType&& x)
    {
        return static_cast<ThisType&>(Super::operator=(std::forward<ThisType>(x)));
    }

    HK_FORCEINLINE void Swap(ThisType& x)
    {
        Super::swap(x);
    }

    HK_FORCEINLINE void Assign(SizeType n, ValueType const& value)
    {
        Super::assign(n, value);
    }

    template <typename InputIterator>
    HK_FORCEINLINE void Assign(InputIterator first, InputIterator last)
    {
        Super::assign(first, last);
    }

    HK_FORCEINLINE void Assign(std::initializer_list<ValueType> ilist)
    {
        Super::assign(ilist);
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return Super::empty();
    }

    HK_FORCEINLINE SizeType Size() const
    {
        return Super::size();
    }

    HK_FORCEINLINE SizeType Capacity() const
    {
        return Super::capacity();
    }

    HK_FORCEINLINE void Resize(SizeType n, const ValueType& value)
    {
        Super::resize(n, value);
    }

    HK_FORCEINLINE void Resize(SizeType n)
    {
        Super::resize(n);
    }

    HK_FORCEINLINE void ResizeInvalidate(SizeType n)
    {
        Super::clear();
        Super::resize(n);
    }

    HK_FORCEINLINE void Reserve(SizeType n)
    {
        Super::reserve(n);
    }

    HK_FORCEINLINE void ShrinkToFit()
    {
        Super::shrink_to_fit();
    }

    HK_FORCEINLINE Pointer ToPtr()
    {
        return Super::data();
    }

    HK_FORCEINLINE ConstPointer ToPtr() const
    {
        return Super::data();
    }

    HK_FORCEINLINE Reference operator[](SizeType n)
    {
        return Super::operator[](n);
    }

    HK_FORCEINLINE ConstReference operator[](SizeType n) const
    {
        return Super::operator[](n);
    }

    HK_FORCEINLINE Reference At(SizeType n)
    {
        return Super::at(n);
    }

    HK_FORCEINLINE ConstReference At(SizeType n) const
    {
        return Super::at(n);
    }

    HK_FORCEINLINE Reference First()
    {
        return Super::front();
    }

    HK_FORCEINLINE ConstReference First() const
    {
        return Super::front();
    }

    HK_FORCEINLINE Reference Last()
    {
        return Super::back();
    }

    HK_FORCEINLINE ConstReference Last() const
    {
        return Super::back();
    }

    HK_FORCEINLINE void Add(ValueType const& value)
    {
        Super::push_back(value);
    }

    HK_FORCEINLINE Reference Add()
    {
        return Super::push_back();
    }

    HK_FORCEINLINE void* AddUninitialized()
    {
        return Super::push_back_uninitialized();
    }

    HK_FORCEINLINE void Add(ValueType&& value)
    {
        Super::push_back(std::forward<ValueType>(value));
    }

    template <typename InputIterator>
    HK_FORCEINLINE void Add(InputIterator first, InputIterator last)
    {
        Super::insert(begin(), first, last);
    }

    template <typename RhsAllocator>
    HK_FORCEINLINE void Add(TVector<T, RhsAllocator> const& vector)
    {
        Super::insert(end(), vector.begin(), vector.end());
    }

    HK_FORCEINLINE void AddUnique(ValueType const& value)
    {
        if (eastl::find(begin(), end(), value) == end())
            Super::push_back(value);
    }

    HK_FORCEINLINE void Remove(SizeType index)
    {
        Super::erase(begin() + index);
    }

    HK_FORCEINLINE void RemoveRange(SizeType index, SizeType count)
    {
        Super::erase(begin() + index, begin() + (index + count));
    }

    HK_FORCEINLINE void RemoveFirst()
    {
        Super::erase(begin());
    }

    HK_FORCEINLINE void RemoveLast()
    {
        Super::pop_back();
    }

    template <typename Predicate>
    HK_FORCEINLINE void RemoveDuplicates(Predicate predicate)
    {
        DoRemoveDuplicates<false, Predicate>(predicate);
    }

    template <typename Predicate>
    HK_FORCEINLINE void RemoveDuplicatesUnsorted(Predicate predicate)
    {
        DoRemoveDuplicates<true, Predicate>(predicate);
    }

    /** An optimized removing of array element without memory moves. Just swaps last element with removed element. */
    HK_FORCEINLINE void RemoveUnsorted(SizeType index)
    {
        Super::erase_unsorted(begin() + index);
    }

    template <class... Args>
    HK_FORCEINLINE Iterator Emplace(ConstIterator position, Args&&... args)
    {
        return Super::emplace(position, std::forward<Args>(args)...);
    }

    template <class... Args>
    HK_FORCEINLINE Reference EmplaceBack(Args&&... args)
    {
        return Super::emplace_back(std::forward<Args>(args)...);
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, ValueType const& value)
    {
        return Super::insert(position, value);
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, SizeType n, ValueType const& value)
    {
        return Super::insert(position, n, value);
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, ValueType&& value)
    {
        return Super::insert(position, std::forward<ValueType>(value));
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, std::initializer_list<ValueType> ilist)
    {
        return Super::insert(position, ilist);
    }

    template <typename InputIterator>
    HK_FORCEINLINE Iterator Insert(ConstIterator position, InputIterator first, InputIterator last)
    {
        return Super::insert(position, first, last);
    }

    HK_FORCEINLINE void InsertAt(SizeType index, ValueType const& value)
    {
        Super::insert(begin() + index, value);
    }

    HK_FORCEINLINE void InsertAt(SizeType index, ValueType&& value)
    {
        Super::insert(begin() + index, std::forward<ValueType>(value));
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator position)
    {
        return Super::erase(position);
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator first, ConstIterator last)
    {
        return Super::erase(first, last);
    }

    /** Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position. */
    HK_FORCEINLINE Iterator EraseUnsorted(ConstIterator position)
    {
        return Super::erase_unsorted(position);
    }

    HK_FORCEINLINE ReverseIterator Erase(ConstReverseIterator position)
    {
        return Super::erase(position);
    }

    HK_FORCEINLINE ReverseIterator Erase(ConstReverseIterator first, ConstReverseIterator last)
    {
        return Super::erase(first, last);
    }

    HK_FORCEINLINE ReverseIterator EraseUnsorted(ConstReverseIterator position)
    {
        return Super::erase_unsorted(position);
    }

    HK_FORCEINLINE void Clear()
    {
        Super::clear();
    }

    HK_FORCEINLINE void Free()
    {
        Super::clear();
        Super::shrink_to_fit();
    }

    HK_FORCEINLINE void ZeroMem()
    {
        Platform::ZeroMem(ToPtr(), sizeof(ValueType) * Size());
    }

    HK_FORCEINLINE ConstIterator Find(ValueType const& value) const
    {
        return eastl::find(begin(), end(), value);
    }

    HK_FORCEINLINE bool Contains(ValueType const& value) const
    {
        return eastl::find(begin(), end(), value) != end();
    }

    template <typename Predicate>
    HK_FORCEINLINE bool Contains(ValueType const& value, Predicate predicate) const
    {
        return eastl::find(begin(), end(), value, predicate) != end();
    }

    HK_FORCEINLINE SizeType IndexOf(ValueType const& value) const
    {
        auto it = eastl::find(begin(), end(), value);
        return it == end() ? Core::NPOS : (SizeType)(it - begin());
    }

    HK_FORCEINLINE void Reverse()
    {
        eastl::reverse(begin(), end());
    }

    HK_FORCEINLINE void ReverseRange(SizeType index, SizeType count)
    {
        eastl::reverse(begin() + index, begin() + (index + count));
    }

    DEFINE_BEGIN_END(HK_FORCEINLINE)

    HK_INLINE bool operator==(ThisType const& rhs) const
    {
        return ((Super::size() == rhs.size()) && eastl::equal(begin(), end(), rhs.begin()));
    }

    HK_INLINE bool operator!=(ThisType const& rhs) const
    {
        return ((Super::size() != rhs.size()) || !eastl::equal(begin(), end(), rhs.begin()));
    }

    HK_INLINE bool operator<(ThisType const& rhs) const
    {
        return eastl::lexicographical_compare(begin(), end(), rhs.begin(), rhs.end());
    }

    HK_INLINE bool operator>(ThisType const& rhs) const
    {
        return rhs < *this;
    }

    HK_INLINE bool operator<=(ThisType const& rhs) const
    {
        return !(rhs < *this);
    }

    HK_INLINE bool operator>=(ThisType const& rhs) const
    {
        return !(*this < rhs);
    }

private:
    template <bool bUnsorted, typename Predicate>
    void DoRemoveDuplicates(Predicate predicate)
    {
        SizeType arraySize = Super::size();
        Pointer  arrayData = Super::data();
        for (SizeType i = 0; i < arraySize; i++)
        {
            for (SizeType j = arraySize - 1; j > i; j--)
            {
                if (predicate(arrayData[j], arrayData[i]))
                {
                    // duplicate found

                    if (bUnsorted)
                        Super::erase_unsorted(begin() + j);
                    else
                        Super::erase(begin() + j);
                    arraySize--;
                }
            }
        }
    }
};

namespace eastl
{

template <typename T, typename Allocator>
HK_INLINE void swap(TVector<T, Allocator>& lhs, TVector<T, Allocator>& rhs)
{
    lhs.Swap(rhs);
}

} // namespace eastl


template <typename T, size_t BaseCapacity, bool bEnableOverflow = true, typename OverflowAllocator = typename eastl::type_select<bEnableOverflow, Allocators::HeapMemoryAllocator<HEAP_VECTOR>, Allocators::EASTLDummyAllocator>::type>
class TFixedVector : private eastl::fixed_vector<T, BaseCapacity, bEnableOverflow, OverflowAllocator>
{
public:
    using Super                 = eastl::fixed_vector<T, BaseCapacity, bEnableOverflow, OverflowAllocator>;
    using FixedAllocatorType    = eastl::fixed_vector_allocator<sizeof(T), BaseCapacity, alignof(T), 0, bEnableOverflow, OverflowAllocator>;
    using OverflowAllocatorType = OverflowAllocator;
    using ThisType              = TFixedVector<T, BaseCapacity, bEnableOverflow, OverflowAllocator>;
    using SizeType              = typename Super::size_type;
    using ValueType             = typename Super::value_type;
    using Pointer               = typename Super::pointer;
    using ConstPointer          = typename Super::const_pointer;
    using Reference             = typename Super::reference;
    using ConstReference        = typename Super::const_reference;
    using Iterator              = typename Super::iterator;
    using ConstIterator         = typename Super::const_iterator;
    using ReverseIterator       = eastl::reverse_iterator<Iterator>;
    using ConstReverseIterator  = eastl::reverse_iterator<ConstIterator>;
    using AlignedBufferType     = eastl::aligned_buffer<BaseCapacity * sizeof(T), alignof(T)>;

public:
    TFixedVector() = default;

    HK_FORCEINLINE explicit TFixedVector(OverflowAllocatorType const& overflowAllocator) :
        Super(overflowAllocator)
    {}

    HK_FORCEINLINE explicit TFixedVector(SizeType n) :
        Super(n)
    {}

    HK_FORCEINLINE TFixedVector(SizeType n, ValueType const& value) :
        Super(n, value)
    {}

    HK_FORCEINLINE TFixedVector(ThisType const& x) :
        Super(x)
    {}

    HK_FORCEINLINE TFixedVector(ThisType&& x) :
        Super(std::forward<ThisType>(x))
    {}

    HK_FORCEINLINE TFixedVector(ThisType&& x, OverflowAllocatorType const& overflowAllocator) :
        Super(std::forward<ThisType>(x), overflowAllocator)
    {}

    HK_FORCEINLINE TFixedVector(std::initializer_list<T> ilist, OverflowAllocatorType const& overflowAllocator = OverflowAllocatorType("FixedVector")) :
        Super(ilist, overflowAllocator)
    {}

    template <typename InputIterator>
    HK_FORCEINLINE TFixedVector(InputIterator first, InputIterator last) :
        Super(first, last)
    {}

    HK_FORCEINLINE ThisType& operator=(ThisType const& x)
    {
        return static_cast<ThisType&>(Super::operator=(x));
    }

    HK_FORCEINLINE ThisType& operator=(std::initializer_list<T> ilist)
    {
        return static_cast<ThisType&>(Super::operator=(ilist));
    }

    HK_FORCEINLINE ThisType& operator=(ThisType&& x)
    {
        return static_cast<ThisType&>(Super::operator=(std::forward<ThisType>(x)));
    }

    HK_FORCEINLINE void Swap(ThisType& x)
    {
        Super::swap(x);
    }

    HK_FORCEINLINE void SetCapacity(SizeType n)
    {
        Super::set_capacity(n);
    }

    HK_FORCEINLINE void Clear(bool freeOverflow)
    {
        Super::clear(freeOverflow);
    }

    /** Returns the max fixed size, which is the user - supplied BaseCapacity parameter. */
    HK_FORCEINLINE SizeType MaxSize() const
    {
        return Super::max_size();
    }

    /** Returns true if the fixed space has been fully allocated.Note that if overflow is enabled, the container size can be greater than BaseCapacity but full() could return true because the fixed space may have a recently freed slot. */
    HK_FORCEINLINE bool IsFull() const
    {
        return Super::full();
    }

    /** Returns true if the allocations spilled over into the overflow allocator.Meaningful only if overflow is enabled. */
    HK_FORCEINLINE bool HasOverflowed() const
    {
        return Super::has_overflowed();
    }

    /** Returns the value of the bEnableOverflow template parameter. */
    HK_FORCEINLINE bool CanOverflow() const
    {
        return Super::can_overflow();
    }

    HK_FORCEINLINE void* AddUninitialized()
    {
        return Super::push_back_uninitialized();
    }

    HK_FORCEINLINE void Add(ValueType const& value)
    {
        Super::push_back(value);
    }

    HK_FORCEINLINE Reference Add()
    {
        return Super::push_back();
    }

    HK_FORCEINLINE void Add(ValueType&& value)
    {
        Super::push_back(std::forward<ValueType>(value));
    }

    template <typename InputIterator>
    HK_FORCEINLINE void Add(InputIterator first, InputIterator last)
    {
        Super::insert(begin(), first, last);
    }

    template <size_t RhsBaseCapacity, bool RhsEnableOverflow, typename RhsAllocator>
    HK_FORCEINLINE void Add(TFixedVector<T, RhsBaseCapacity, RhsEnableOverflow, RhsAllocator> const& vector)
    {
        Super::insert(end(), vector.begin(), vector.end());
    }

    HK_FORCEINLINE void AddUnique(ValueType const& value)
    {
        if (eastl::find(begin(), end(), value) == end())
            Super::push_back(value);
    }

    HK_FORCEINLINE void Remove(SizeType index)
    {
        Super::erase(begin() + index);
    }

    HK_FORCEINLINE void RemoveRange(SizeType index, SizeType count)
    {
        Super::erase(begin() + index, begin() + (index + count));
    }

    HK_FORCEINLINE void RemoveFirst()
    {
        Super::erase(begin());
    }

    HK_FORCEINLINE void RemoveLast()
    {
        Super::pop_back();
    }

    template <typename Predicate>
    HK_FORCEINLINE void RemoveDuplicates(Predicate predicate)
    {
        DoRemoveDuplicates<false, Predicate>(predicate);
    }

    template <typename Predicate>
    HK_FORCEINLINE void RemoveDuplicatesUnsorted(Predicate predicate)
    {
        DoRemoveDuplicates<true, Predicate>(predicate);
    }

    /** An optimized removing of array element without memory moves. Just swaps last element with removed element. */
    HK_FORCEINLINE void RemoveUnsorted(SizeType index)
    {
        Super::erase_unsorted(begin() + index);
    }

    HK_FORCEINLINE void Assign(SizeType n, ValueType const& value)
    {
        Super::assign(n, value);
    }

    template <typename InputIterator>
    HK_FORCEINLINE void Assign(InputIterator first, InputIterator last)
    {
        Super::assign(first, last);
    }

    HK_FORCEINLINE void Assign(std::initializer_list<ValueType> ilist)
    {
        Super::assign(ilist);
    }

    HK_FORCEINLINE bool IsEmpty() const
    {
        return Super::empty();
    }

    HK_FORCEINLINE SizeType Size() const
    {
        return Super::size();
    }

    HK_FORCEINLINE SizeType Capacity() const
    {
        return Super::capacity();
    }

    HK_FORCEINLINE void Resize(SizeType n, const ValueType& value)
    {
        Super::resize(n, value);
    }

    HK_FORCEINLINE void Resize(SizeType n)
    {
        Super::resize(n);
    }

    HK_FORCEINLINE void ResizeInvalidate(SizeType n)
    {
        Super::clear();
        Super::resize(n);
    }

    HK_FORCEINLINE void Reserve(SizeType n)
    {
        Super::reserve(n);
    }

    HK_FORCEINLINE void ShrinkToFit()
    {
        Super::shrink_to_fit();
    }

    HK_FORCEINLINE Pointer ToPtr()
    {
        return Super::data();
    }

    HK_FORCEINLINE ConstPointer ToPtr() const
    {
        return Super::data();
    }

    HK_FORCEINLINE Reference operator[](SizeType n)
    {
        return Super::operator[](n);
    }

    HK_FORCEINLINE ConstReference operator[](SizeType n) const
    {
        return Super::operator[](n);
    }

    HK_FORCEINLINE Reference At(SizeType n)
    {
        return Super::at(n);
    }

    HK_FORCEINLINE ConstReference At(SizeType n) const
    {
        return Super::at(n);
    }

    HK_FORCEINLINE Reference First()
    {
        return Super::front();
    }

    HK_FORCEINLINE ConstReference First() const
    {
        return Super::front();
    }

    HK_FORCEINLINE Reference Last()
    {
        return Super::back();
    }

    HK_FORCEINLINE ConstReference Last() const
    {
        return Super::back();
    }

    template <class... Args>
    HK_FORCEINLINE Iterator Emplace(ConstIterator position, Args&&... args)
    {
        return Super::emplace(position, std::forward<Args>(args)...);
    }

    template <class... Args>
    HK_FORCEINLINE Reference EmplaceBack(Args&&... args)
    {
        return Super::emplace_back(std::forward<Args>(args)...);
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, ValueType const& value)
    {
        return Super::insert(position, value);
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, SizeType n, ValueType const& value)
    {
        return Super::insert(position, n, value);
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, ValueType&& value)
    {
        return Super::insert(position, std::forward<ValueType>(value));
    }

    HK_FORCEINLINE Iterator Insert(ConstIterator position, std::initializer_list<ValueType> ilist)
    {
        return Super::insert(position, ilist);
    }

    template <typename InputIterator>
    HK_FORCEINLINE Iterator Insert(ConstIterator position, InputIterator first, InputIterator last)
    {
        return Super::insert(position, first, last);
    }

    HK_FORCEINLINE void InsertAt(SizeType index, ValueType const& value)
    {
        Super::insert(begin() + index, value);
    }

    HK_FORCEINLINE void InsertAt(SizeType index, ValueType&& value)
    {
        Super::insert(begin() + index, std::forward<ValueType>(value));
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator position)
    {
        return Super::erase(position);
    }

    HK_FORCEINLINE Iterator Erase(ConstIterator first, ConstIterator last)
    {
        return Super::erase(first, last);
    }

    /** Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position. */
    HK_FORCEINLINE Iterator EraseUnsorted(ConstIterator position)
    {
        return Super::erase_unsorted(position);
    }

    HK_FORCEINLINE ReverseIterator Erase(ConstReverseIterator position)
    {
        return Super::erase(position);
    }

    HK_FORCEINLINE ReverseIterator Erase(ConstReverseIterator first, ConstReverseIterator last)
    {
        return Super::erase(first, last);
    }

    HK_FORCEINLINE ReverseIterator EraseUnsorted(ConstReverseIterator position)
    {
        return Super::erase_unsorted(position);
    }

    HK_FORCEINLINE void Clear()
    {
        Super::clear();
    }

    HK_FORCEINLINE void Free()
    {
        Super::clear();
        Super::shrink_to_fit();
    }

    HK_FORCEINLINE void ZeroMem()
    {
        Platform::ZeroMem(ToPtr(), sizeof(ValueType) * Size());
    }

    HK_FORCEINLINE ConstIterator Find(ValueType const& value) const
    {
        return eastl::find(begin(), end(), value);
    }

    HK_FORCEINLINE bool Contains(ValueType const& value) const
    {
        return eastl::find(begin(), end(), value) != end();
    }

    template <typename Predicate>
    HK_FORCEINLINE bool Contains(ValueType const& value, Predicate predicate) const
    {
        return eastl::find(begin(), end(), value, predicate) != end();
    }

    HK_FORCEINLINE SizeType IndexOf(ValueType const& value) const
    {
        auto it = eastl::find(begin(), end(), value);
        return it == end() ? Core::NPOS : (SizeType)(it - begin());
    }

    HK_FORCEINLINE void Reverse()
    {
        eastl::reverse(begin(), end());
    }

    HK_FORCEINLINE void ReverseRange(SizeType index, SizeType count)
    {
        eastl::reverse(begin() + index, begin() + (index + count));
    }

    DEFINE_BEGIN_END(HK_FORCEINLINE)

    HK_FORCEINLINE OverflowAllocatorType const& GetOverflowAllocator() const
    {
        return Super::get_overflow_allocator();
    }

    HK_FORCEINLINE OverflowAllocatorType& GetOverflowAllocator()
    {
        return Super::get_overflow_allocator();
    }

    HK_FORCEINLINE void SetOverflowAllocator(OverflowAllocatorType const& allocator)
    {
        Super::set_overflow_allocator(allocator);
    }

    HK_INLINE bool operator==(ThisType const& rhs) const
    {
        return ((Super::size() == rhs.size()) && eastl::equal(begin(), end(), rhs.begin()));
    }

    HK_INLINE bool operator!=(ThisType const& rhs) const
    {
        return ((Super::size() != rhs.size()) || !eastl::equal(begin(), end(), rhs.begin()));
    }

    HK_INLINE bool operator<(ThisType const& rhs) const
    {
        return eastl::lexicographical_compare(begin(), end(), rhs.begin(), rhs.end());
    }

    HK_INLINE bool operator>(ThisType const& rhs) const
    {
        return rhs < *this;
    }

    HK_INLINE bool operator<=(ThisType const& rhs) const
    {
        return !(rhs < *this);
    }

    HK_INLINE bool operator>=(ThisType const& rhs) const
    {
        return !(*this < rhs);
    }

private:
    template <bool bUnsorted, typename Predicate>
    void DoRemoveDuplicates(Predicate predicate)
    {
        SizeType arraySize = Super::size();
        Pointer  arrayData = Super::data();
        for (SizeType i = 0; i < arraySize; i++)
        {
            for (SizeType j = arraySize - 1; j > i; j--)
            {
                if (predicate(arrayData[j], arrayData[i]))
                {
                    // duplicate found

                    if (bUnsorted)
                        Super::erase_unsorted(begin() + j);
                    else
                        Super::erase(begin() + j);
                    arraySize--;
                }
            }
        }
    }
};

namespace eastl
{
template <typename T, size_t BaseCapacity, bool bEnableOverflow, typename OverflowAllocator>
HK_INLINE void swap(TFixedVector<T, BaseCapacity, bEnableOverflow, OverflowAllocator>& lhs,
                    TFixedVector<T, BaseCapacity, bEnableOverflow, OverflowAllocator>& rhs)
{
    // Fixed containers use lhs special swap that can deal with excessively large buffers.
    eastl::fixed_swap(lhs, rhs);
}
} // namespace eastl

template <typename T, size_t MaxCapacity>
using TStaticVector = TFixedVector<T, MaxCapacity, false, Allocators::EASTLDummyAllocator>;

template <typename T, size_t BaseCapacity, typename OverflowAllocator = Allocators::HeapMemoryAllocator<HEAP_VECTOR>>
using TSmallVector = TFixedVector<T, BaseCapacity, true, OverflowAllocator>;

template <typename T, typename Allocator = Allocators::HeapMemoryAllocator<HEAP_VECTOR>>
using TPodVector = TFixedVector<T, 32, true, Allocator>; // TODO: Deprecated. Remove this
