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

#include <Platform/BaseTypes.h>

/** Max levels of detail */
constexpr int QUADTREE_MAX_LODS_32 = 16;

/** Max levels of detail */
constexpr int QUADTREE_MAX_LODS_64 = 32;

struct SQuadTreeRemapTable
{
    SQuadTreeRemapTable()
    {
        // FIXME: This can be precomputed at compile-time

        uint32_t value = 0;
        uint64_t value64 = 0;

        for ( int level = 0 ; level < QUADTREE_MAX_LODS_32 ; level++ )
        {
            Rel2Abs[level] = value;
            value += 1u << (level << 1);
        }

        for ( int level = 0 ; level < QUADTREE_MAX_LODS_64 ; level++ )
        {
            Rel2Abs64[level] = value64;
            value64 += uint64_t( 1 ) << (level << 1);
        }
    }

    /** Table for conversion from relative to absolute indices */
    uint32_t Rel2Abs[QUADTREE_MAX_LODS_32];
    uint64_t Rel2Abs64[QUADTREE_MAX_LODS_64];
};

extern const SQuadTreeRemapTable QuadTreeRemapTable;

AN_FORCEINLINE uint32_t QuadTreeCalcLodNodes( int Lod ) {
    return 1u << (Lod << 1);
}

AN_FORCEINLINE uint64_t QuadTreeCalcLodNodes64( int Lod ) {
    return uint64_t( 1 ) << (Lod << 1);
}

AN_FORCEINLINE uint32_t QuadTreeRelativeToAbsoluteIndex( uint32_t RelIndex, int Lod ) {
    return RelIndex + QuadTreeRemapTable.Rel2Abs[Lod];
}

AN_FORCEINLINE uint64_t QuadTreeRelativeToAbsoluteIndex64( uint64_t RelIndex, int Lod ) {
    return RelIndex + QuadTreeRemapTable.Rel2Abs64[Lod];
}

AN_FORCEINLINE uint32_t QuadTreeAbsoluteToRelativeIndex( uint32_t AbsIndex, int Lod ) {
    return AbsIndex - QuadTreeRemapTable.Rel2Abs[Lod];
}

AN_FORCEINLINE uint64_t QuadTreeAbsoluteToRelativeIndex64( uint64_t AbsIndex, int Lod ) {
    return AbsIndex - QuadTreeRemapTable.Rel2Abs64[Lod];
}

AN_FORCEINLINE void QuadTreeGetXYFromRelative( int & X, int & Y, uint32_t RelIndex, int Lod ) {
    X = RelIndex & ((1u << Lod)-1);
    Y = RelIndex >> Lod;
}

AN_FORCEINLINE void QuadTreeGetXYFromRelative64( int64_t & X, int64_t & Y, uint64_t RelIndex, int Lod ) {
    X = RelIndex & ((uint64_t( 1 ) << Lod)-1);
    Y = RelIndex >> Lod;
}

AN_FORCEINLINE int QuadTreeGetXFromRelative( uint32_t RelIndex, int Lod ) {
    return RelIndex & ((1u << Lod)-1);
}

AN_FORCEINLINE int QuadTreeGetYFromRelative( uint32_t RelIndex, int Lod ) {
    return RelIndex >> Lod;
}

AN_FORCEINLINE int64_t QuadTreeGetXFromRelative64( uint64_t RelIndex, int Lod ) {
    return RelIndex & ((uint64_t( 1 ) << Lod)-1);
}

AN_FORCEINLINE int64_t QuadTreeGetYFromRelative64( uint64_t RelIndex, int Lod ) {
    return RelIndex >> Lod;
}

AN_FORCEINLINE uint32_t QuadTreeGetRelativeFromXY( int X, int Y, int Lod ) {
    return X + (Y << Lod);
}

AN_FORCEINLINE uint64_t QuadTreeGetRelativeFromXY64( int64_t X, int64_t Y, int Lod ) {
    return X + (Y << Lod);
}

AN_FORCEINLINE uint32_t QuadTreeGetParentFromRelative( uint32_t RelIndex, int Lod ) {
    return ((RelIndex & ((1u << Lod)-1)) >> 1) + ((RelIndex >> (Lod+1)) << (Lod-1))  + QuadTreeRemapTable.Rel2Abs[Lod-1];
}

AN_FORCEINLINE uint64_t QuadTreeGetParentFromRelative64( uint64_t RelIndex, int Lod ) {
    return ((RelIndex & ((uint64_t( 1 ) << Lod)-1)) >> 1) + ((RelIndex >> (Lod+1)) << (Lod-1))  + QuadTreeRemapTable.Rel2Abs64[Lod-1];
}

AN_INLINE int QuadTreeCalcLod64( uint64_t AbsIndex ) {
    uint64_t totalNodes = 0;
    for ( int lod = 0 ; lod < QUADTREE_MAX_LODS_64 ; lod++ ) {
        totalNodes += QuadTreeCalcLodNodes64( lod );
        if ( AbsIndex < totalNodes ) {
            return lod;
        }
    }
    return -1;
}

AN_INLINE uint32_t QuadTreeCalcQuadTreeNodes( int NumLods ) {
    uint32_t total = 0;
    while ( --NumLods >= 0 ) {
        total += 1u << (NumLods << 1);
    }
    return total;
}

AN_INLINE uint64_t QuadTreeCalcQuadTreeNodes64( int NumLods ) {
    uint64_t total = 0;
    while ( --NumLods >= 0 ) {
        total += uint64_t( 1 ) << (NumLods << 1);
    }
    return total;
}

AN_INLINE void QuadTreeGetRelation( uint32_t RelIndex, int Lod, uint32_t & Parent, uint32_t *Children = nullptr ) {
    int     x, y;

    QuadTreeGetXYFromRelative( x, y, RelIndex, Lod );

    Parent = Lod > 0 ? QuadTreeRelativeToAbsoluteIndex( QuadTreeGetRelativeFromXY( x>>1, y>>1, Lod-1 ), Lod-1 ) : 0;

    if ( Children ) {
        uint32_t div = 1u << Lod;
        Children[0] = ((RelIndex >> Lod) << (Lod+2)) + ((RelIndex & (div-1)) << 1) + QuadTreeRemapTable.Rel2Abs[Lod+1];
        Children[1] = Children[0] + 1;
        Children[2] = Children[0] + (div << 1);
        Children[3] = Children[2] + 1;

        // Same:
        //        int childX = x << 1;
        //        int childY = y << 1;
        //        Children[0] = RelativeToAbsoluteIndex(GetRelativeFromXY( childX, childY, Lod+1 ), Lod+1);
        //        Children[1] = RelativeToAbsoluteIndex(GetRelativeFromXY( childX+1, childY, Lod+1 ), Lod+1);
        //        Children[2] = RelativeToAbsoluteIndex(GetRelativeFromXY( childX, childY+1, Lod+1 ), Lod+1);
        //        Children[3] = RelativeToAbsoluteIndex(GetRelativeFromXY( childX+1, childY+1, Lod+1 ), Lod+1);
    }
}

AN_INLINE void QuadTreeGetRelation64( uint64_t RelIndex, int Lod, uint64_t & Parent, uint64_t *Children = nullptr ) {
    int64_t     x, y;

    QuadTreeGetXYFromRelative64( x, y, RelIndex, Lod );

    Parent = Lod > 0 ? QuadTreeRelativeToAbsoluteIndex64( QuadTreeGetRelativeFromXY64( x>>1, y>>1, Lod-1 ), Lod-1 ) : 0;

    if ( Children ) {
        uint64_t div = uint64_t( 1 ) << Lod;
        Children[0] = ((RelIndex >> Lod) << (Lod+2)) + ((RelIndex & (div-1)) << 1) + QuadTreeRemapTable.Rel2Abs64[Lod+1];
        Children[1] = Children[0] + 1;
        Children[2] = Children[0] + (div << 1);
        Children[3] = Children[2] + 1;
    }
}

AN_INLINE void QuadTreeGetNodeOffset( int Lod, uint32_t node, int NumLods, unsigned short Offset[2] ) {
    uint32_t RelIndex = node - QuadTreeRemapTable.Rel2Abs[Lod];
    int diff = NumLods - Lod - 1;
    Offset[0] = (RelIndex & ((1u << Lod)-1)) << diff;
    Offset[1] = (RelIndex >> Lod) << diff;
}

AN_INLINE void QuadTreeGetNodeOffsetRel( int Lod, uint32_t RelIndex, int NumLods, unsigned short Offset[2] ) {
    int diff = NumLods - Lod - 1;
    Offset[0] = (RelIndex & ((1u << Lod)-1)) << diff;
    Offset[1] = (RelIndex >> Lod) << diff;
}

AN_INLINE void QuadTreeGetNodeOffset64( int Lod, uint64_t node, int NumLods, uint32_t Offset[2] ) {
    uint64_t RelIndex = node - QuadTreeRemapTable.Rel2Abs64[Lod];
    int diff = NumLods - Lod - 1;
    Offset[0] = (RelIndex & ((uint64_t( 1 ) << Lod)-1)) << diff;
    Offset[1] = (RelIndex >> Lod) << diff;
}

AN_INLINE void QuadTreeGetNodeOffsetRel64( int Lod, uint64_t RelIndex, int NumLods, uint32_t Offset[2] ) {
    int diff = NumLods - Lod - 1;
    Offset[0] = (RelIndex & ((uint64_t( 1 ) << Lod)-1)) << diff;
    Offset[1] = (RelIndex >> Lod) << diff;
}

AN_FORCEINLINE bool QuadTreeIsIndexValid( uint32_t AbsIndex, int Lod )
{
    return AbsIndex >= QuadTreeRemapTable.Rel2Abs[Lod] && AbsIndex < QuadTreeRemapTable.Rel2Abs[Lod+1];
}
