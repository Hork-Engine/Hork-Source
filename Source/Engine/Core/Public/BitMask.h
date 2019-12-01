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

#include "PodArray.h"

/**

TBitMask

Variable size bit mask

*/
template< int BASE_CAPACITY_IN_BITS = 1024, int GRANULARITY_IN_BITS = 1024 >
class TBitMask {
public:
    using T = uint32_t;
    static constexpr int BitCount = sizeof( T ) * 8;
    static constexpr int BitWrapMask = BitCount - 1;
    static constexpr int BitExponent = 5;
    static constexpr int BaseCapacityInBytes = BASE_CAPACITY_IN_BITS / BitCount + 1;
    static constexpr int BaseGranularityInBytes = BASE_CAPACITY_IN_BITS / BitCount + 1;

    TBitMask() : NumBits(0) {}
    TBitMask( TBitMask const & _BitMask ) : Bits(_BitMask.Bits), NumBits(_BitMask.NumBits) {}
    ~TBitMask() {}

    void Clear() {
        Bits.Clear();
        NumBits = 0;
    }

    void Free() {
        Bits.Free();
        NumBits = 0;
    }

    void ShrinkToFit() {
        Bits.ShrinkToFit();
    }

    void Reserve( int _NewCapacity ) {
        Bits.Reserve( _NewCapacity / BitCount + 1 );
    }

    void ReserveInvalidate( int _NewCapacity ) {
        Bits.ReserveInvalidate( _NewCapacity / BitCount + 1 );
    }

    bool IsEmpty() const {
        return NumBits == 0;
    }

    T * ToPtr() { return Bits.ToPtr(); }
    T const * ToPtr() const { return Bits.ToPtr(); }

    TBitMask & operator=( TBitMask const & _BitMask ) {
        Bits = _BitMask.Bits;
        NumBits = _BitMask.NumBits;
    }

    void Resize( int _NumBits ) {
        const int oldsize = Bits.Size();
        const int newsize = _NumBits / BitCount + 1;

        Bits.Resize( newsize );

        if ( _NumBits > NumBits ) {
            memset( Bits.ToPtr() + oldsize, 0, ( newsize - oldsize ) * sizeof( T ) );

            const int clearBitsInWord = oldsize * BitCount;
            for ( int i = NumBits ; i < clearBitsInWord ; i++ ) {
                Bits[i >> BitExponent] &= ~(1 << (i & BitWrapMask));
            }
        }
        NumBits = _NumBits;
    }

    void ResizeInvalidate( int _NumBits ) {
        Bits.ResizeInvalidate( _NumBits / BitCount + 1 );
        NumBits = _NumBits;
    }

    int Size() const {
        return NumBits;
    }

    int Reserved() const {
        return Bits.Reserved() * BitCount;
    }

    void MarkAll() {
        Bits.Memset( 0xff );
    }

    void UnmarkAll() {
        Bits.Memset( 0 );
    }

    void Mark( int _BitIndex ) {
        if ( _BitIndex >= Size() ) {
            Resize( _BitIndex + 1 );
        }
        Bits[_BitIndex >> BitExponent] |= 1 << (_BitIndex & BitWrapMask);
    }

    void Unmark( int _BitIndex ) {
        if ( _BitIndex < Size() ) {
            Bits[_BitIndex >> BitExponent] &= ~(1 << (_BitIndex & BitWrapMask));
        }
    }

    bool IsMarked( int _BitIndex ) const {
        return _BitIndex < Size() && (Bits[_BitIndex >> BitExponent] & (1 << (_BitIndex & BitWrapMask)));
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteUInt32( NumBits );
        _Stream.WriteArrayUInt32( Bits );
    }

    void Read( IStreamBase & _Stream ) {
        NumBits = _Stream.ReadUInt32();
        _Stream.ReadArrayUInt32( Bits );
    }

private:
    TPodArray< T, BaseCapacityInBytes, BaseGranularityInBytes > Bits;
    int NumBits;
};
