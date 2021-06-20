/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "Alloc.h"

#include <thread>
//#include <array>

#ifdef AN_CPP20
#define AN_NODISCARD [[nodiscard]]
#else
#define AN_NODISCARD
#endif

template< typename T >
struct TStdZoneAllocator {
    typedef T value_type;

    TStdZoneAllocator() = default;
    template< typename U > constexpr TStdZoneAllocator( TStdZoneAllocator< U > const & ) noexcept {}

    AN_NODISCARD T * allocate( std::size_t _Count ) noexcept {
        AN_ASSERT( _Count <= TStdNumericLimits< std::size_t >::max() / sizeof( T ) );
        
        return static_cast< T * >( GZoneMemory.Alloc( _Count * sizeof( T ) ) );
    }

    void deallocate( T * _Bytes, std::size_t _Count ) noexcept {
        GZoneMemory.Free( _Bytes );
    }
};
template< typename T, typename U > bool operator==( TStdZoneAllocator< T > const &, TStdZoneAllocator< U > const & ) { return true; }
template< typename T, typename U > bool operator!=( TStdZoneAllocator< T > const &, TStdZoneAllocator< U > const & ) { return false; }

template< typename T >
struct TStdHeapAllocator {
    typedef T value_type;

    TStdHeapAllocator() = default;
    template< typename U > constexpr TStdHeapAllocator( TStdHeapAllocator< U > const & ) noexcept {}

    AN_NODISCARD T * allocate( std::size_t _Count ) noexcept {
        AN_ASSERT( _Count <= TStdNumericLimits< std::size_t >::max() / sizeof( T ) );

        return static_cast< T * >( GHeapMemory.Alloc( _Count * sizeof( T ) ) );
    }

    void deallocate( T * _Bytes, std::size_t _Count ) noexcept {
        GHeapMemory.Free( _Bytes );
    }
};
template< typename T, typename U > bool operator==( TStdHeapAllocator< T > const &, TStdHeapAllocator< U > const & ) { return true; }
template< typename T, typename U > bool operator!=( TStdHeapAllocator< T > const &, TStdHeapAllocator< U > const & ) { return false; }

using AStdString = std::basic_string< char, std::char_traits< char >, TStdZoneAllocator< char > >;

template< typename T > class TStdVector : public std::vector< T, TStdZoneAllocator< T > >
{
public:
    using Super = std::vector< T, TStdZoneAllocator< T > >;

    TStdVector()
    {
    }

    TStdVector( std::initializer_list< T > const & _InitializerList )
    : Super( _InitializerList )
    {
    }

    int Size() const { return Super::size(); }
    T * ToPtr() { return Super::data(); }
    T const * ToPtr() const { return Super::data(); }
    void Clear() { Super::clear(); }
    void ResizeInvalidate( int _Size ) {
        Super::clear();
        Super::resize( _Size );
    }
    void Resize( int _Size ) { Super::resize( _Size ); }
    void Reserve( int _Capacity ) { Super::reserve( _Capacity ); }
    void ReserveInvalidate( int _Capacity ) { Super::reserve( _Capacity ); }
    int Capacity() const { return Super::capacity(); }
    void Free() { Super::clear(); Super::shrink_to_fit(); }
    bool IsEmpty() const { return Super::empty(); }
    void Append( T const & _X ) { Super::push_back( _X ); }
    void ShrinkToFit() { Super::shrink_to_fit(); }
};

//template< typename T > using TStdVector = std::vector< T, TStdZoneAllocator< T > >;
template< typename T > using TStdVectorDefault = std::vector< T >;
template< typename T > using TStdVectorZone = std::vector< T, TStdZoneAllocator< T > >;
template< typename T > using TStdVectorHeap = std::vector< T, TStdHeapAllocator< T > >;

template< typename T > using TStdNumericLimits = std::numeric_limits< T >;

using AStdThread = std::thread;

//template< typename T, size_t Sz >
//using TStdArray = std::array< T, Sz >;

// Type wrap
#define TStdFunction   std::function

// Function wrap
#define StdSort     std::sort
#define StdFind     std::find
#define StdChrono   std::chrono
#define StdSqrt     std::sqrt
#define StdForward  std::forward
#define StdSwap     std::swap
#define StdMax      std::max
#define StdMin      std::min

#define StdUniquePtr std::unique_ptr
#define StdMakeUnique std::make_unique
