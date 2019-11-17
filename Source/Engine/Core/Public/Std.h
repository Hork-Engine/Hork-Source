/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#ifdef AN_CPP20
#define AN_NODISCARD [[nodiscard]]
#else
#define AN_NODISCARD
#endif

template< typename T >
struct TStdAllocator {
    typedef T value_type;

    TStdAllocator() = default;
    template< typename U > constexpr TStdAllocator( TStdAllocator< U > const & ) noexcept {}

    AN_NODISCARD T * allocate( std::size_t _Count ) noexcept {
        //if ( _Count > TStdNumericLimits< std::size_t >::max() / sizeof( T ) ) {
        //    CriticalError( "TStdAllocator: Invalid size\n" );
        //}
        return static_cast< T * >( GZoneMemory.Alloc( _Count * sizeof( T ), 1 ) );
    }

    void deallocate( T * _Bytes, std::size_t _Count ) noexcept {
        GZoneMemory.Dealloc( _Bytes );
    }
};
template< typename T, typename U > bool operator==( TStdAllocator< T > const &, TStdAllocator< U > const & ) { return true; }
template< typename T, typename U > bool operator!=( TStdAllocator< T > const &, TStdAllocator< U > const & ) { return false; }

using AStdString = std::basic_string< char, std::char_traits< char >, TStdAllocator< char > >;

template< typename T > using TStdVector = std::vector< T, TStdAllocator< T > >;

template< typename T > using TStdNumericLimits = std::numeric_limits< T >;

using AStdThread = std::thread;

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
#define StdFmod     std::fmod
#define StdSin      std::sin
#define StdCos      std::cos
#define StdFloor    std::floor

/*

enable_if_t for C++11 workaround

*/
template< bool C, class T = void >
using TEnableIf = typename std::enable_if< C, T >::type;


