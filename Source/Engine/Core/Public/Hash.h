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

template< int HashBucketsCount = 1024 > // must be power of two
class THash final {
    AN_FORBID_COPY( THash )

    using Allocator = FAllocator;
public:
    // Change it to set up granularity. Granularity must be >= 1
    int   Granularity = 1024;

    // Change it to reallocate indices
    int   NumBucketIndices = 0;

    THash() {
        static_assert( IsPowerOfTwoConstexpr( HashBucketsCount ), "Buckets count must be power of two" );

        HashBuckets = const_cast< int * >( InvalidHashIndex );
        IndexChain = const_cast< int * >( InvalidHashIndex );
    }

    ~THash() {
        if ( HashBuckets != InvalidHashIndex ) {
            Allocator::Dealloc( HashBuckets );
        }

        if ( IndexChain != InvalidHashIndex ) {
            Allocator::Dealloc( IndexChain );
        }
    }

    void Clear() {
        if ( HashBuckets != InvalidHashIndex ) {
            ClearMemory8( HashBuckets, 0xffffffffffffffff, HashBucketsCount * sizeof( *HashBuckets ) );
        }
    }

    void Free() {
        if ( HashBuckets != InvalidHashIndex ) {
            Allocator::Dealloc( HashBuckets );
            HashBuckets = const_cast< int * >( InvalidHashIndex );
        }

        if ( IndexChain != InvalidHashIndex ) {
            Allocator::Dealloc( IndexChain );
            IndexChain = const_cast< int * >( InvalidHashIndex );
        }

        LookupMask = 0;
        IndexChainLength = 0;
    }

    void Insert( int _Key, int _Index ) {

        if ( HashBuckets == InvalidHashIndex ) {
            // first allocation
            HashBuckets = ( int * )Allocator::AllocCleared< 1 >( HashBucketsCount * sizeof( *HashBuckets ), 0xffffffffffffffff );
            LookupMask = ~0;
        }

        if ( NumBucketIndices > IndexChainLength ) {
            GrowIndexChain( NumBucketIndices );
        }

        if ( _Index >= IndexChainLength ) {
            // Resize index chain

            int newIndexChainLength = _Index + 1;
            const int mod = newIndexChainLength % Granularity;
            newIndexChainLength = mod == 0 ? newIndexChainLength : newIndexChainLength + Granularity - mod;

            GrowIndexChain( newIndexChainLength );
        }

        _Key &= HashWrapAroundMask;
        IndexChain[ _Index ] = HashBuckets[ _Key ];
        HashBuckets[ _Key ] = _Index;
    }

    void Remove( int _Key, int _Index ) {
        AN_Assert( _Index < IndexChainLength );

        if ( HashBuckets == InvalidHashIndex ) {
            // Hash not allocated yet
            return;
        }

        _Key &= HashWrapAroundMask;
        if ( HashBuckets[_Key] == _Index ) {
            HashBuckets[_Key] = IndexChain[_Index];
        } else {
            for ( int i = HashBuckets[_Key] ; i != -1 ; i = IndexChain[i] ) {
                if ( IndexChain[i] == _Index ) {
                    IndexChain[i] = IndexChain[_Index];
                    break;
                }
            }
        }

        IndexChain[_Index] = -1;
    }

    void InsertIndex( int _Key, int _Index ) {
        if ( HashBuckets != InvalidHashIndex ) {
            int max = _Index;
            for ( int i = 0 ; i < HashBucketsCount ; i++ ) {
                if ( HashBuckets[i] >= _Index ) {
                    HashBuckets[i]++;
                    max = FMath_Max( max, HashBuckets[i] );
                }
            }
            for ( int i = 0 ; i < IndexChainLength ; i++ ) {
                if ( IndexChain[i] >= _Index ) {
                    IndexChain[i]++;
                    max = FMath_Max( max, IndexChain[i] );
                }
            }
            if ( max >= IndexChainLength ) {
                int newIndexChainLength = max + 1;
                const int mod = newIndexChainLength % Granularity;
                newIndexChainLength = mod == 0 ? newIndexChainLength : newIndexChainLength + Granularity - mod;

                GrowIndexChain( newIndexChainLength );
            }
            for ( int i = max ; i > _Index ; i-- ) {
                IndexChain[i] = IndexChain[i-1];
            }
            IndexChain[_Index] = -1;
        }
        Insert( _Key, _Index );
    }

    void RemoveIndex( int _Key, int _Index ) {
        Remove( _Key, _Index );

        if ( HashBuckets == InvalidHashIndex ) {
            return;
        }

        int max = _Index;
        for ( int i = 0 ; i < HashBucketsCount ; i++ ) {
            if ( HashBuckets[i] >= _Index ) {
                max = FMath_Max( max, HashBuckets[i] );
                HashBuckets[i]--;
            }
        }
        for ( int i = 0 ; i < IndexChainLength ; i++ ) {
            if ( IndexChain[i] >= _Index ) {
                max = FMath_Max( max, IndexChain[i] );
                IndexChain[i]--;
            }
        }
        for ( int i = _Index ; i < max ; i++ ) {
            IndexChain[i] = IndexChain[i+1];
        }
        IndexChain[max] = -1;
    }

    int First( int _Key ) const {
        return HashBuckets[ _Key & HashWrapAroundMask & LookupMask ];
    }

    int Next( int _Index ) const {
        AN_Assert( _Index < IndexChainLength );
        return IndexChain[ _Index & LookupMask ];
    }

    bool IsAllocated() const {
        return /*HashBuckets != nullptr && */HashBuckets != InvalidHashIndex;
    }
private:

    void GrowIndexChain( int _NewIndexChainLength ) {
        if ( IndexChain == InvalidHashIndex ) {
            IndexChain = ( int * )Allocator::AllocCleared< 1 >( _NewIndexChainLength * sizeof( *IndexChain ), 0xffffffffffffffff );
        } else {
            IndexChain = ( int * )Allocator::ExtendCleared< 1 >( IndexChain, IndexChainLength * sizeof( *IndexChain ), _NewIndexChainLength * sizeof( *IndexChain ), true, 0xffffffffffffffff );
        }
        IndexChainLength = _NewIndexChainLength;
    }

    int * HashBuckets;
    int * IndexChain;
    int   LookupMask = 0;           // Always 0 or ~0
    int   IndexChainLength = 0;

    constexpr static const int HashWrapAroundMask = HashBucketsCount - 1;
    constexpr static const int InvalidHashIndex[1] = { -1 };
};

template< int HashBucketsCount >
constexpr const int THash< HashBucketsCount >::InvalidHashIndex[1];

ANGIE_TEMPLATE template class ANGIE_API THash<>;