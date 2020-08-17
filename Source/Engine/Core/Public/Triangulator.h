/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "CoreMath.h"
#include "PodArray.h"
#include "Std.h"
#include "LinearAllocator.h"

class ATriangulatorBase {
public:
    void SetBoundary( bool _Flag );

protected:
    ATriangulatorBase();
    virtual ~ATriangulatorBase();

    typedef void ( *SCallback )();

    void SetNormal( Double3 const & _Normal );
    void SetCallback( uint32_t _Name, SCallback _CallbackFunc );

    void BeginPolygon( void * _Data );
    void EndPolygon();
    void BeginContour();
    void EndContour();
    void ProcessVertex( Double3 & _Vertex, const void * _Data );

    enum {
        CB_BEGIN_DATA = 100106,
        CB_END_DATA = 100108,
        CB_VERTEX_DATA = 100107,
        CB_COMBINE_DATA = 100111
    };

private:
    // Tesselator handle
    void * Tesselator;
};

namespace TriangulatorTraits
{

template< typename TContourVertexType >
void ContourVertexPosition( Double3 & _Dst, TContourVertexType const & _Src );

template< typename TTriangleVertexType >
void TriangleVertexPosition( Double3 & _Dst, TTriangleVertexType const & _Src );

template< typename TTriangleVertexType >
void CombineVertex( TTriangleVertexType & _OutputVertex,
                    Double3 const & _Position,
                    float const * _Weights,
                    TTriangleVertexType const & _V0,
                    TTriangleVertexType const & _V1,
                    TTriangleVertexType const & _V2,
                    TTriangleVertexType const & _V3 );

template< typename TContourVertexType, typename TTriangleVertexType >
void CopyVertex( TTriangleVertexType & _Dst, TContourVertexType const & _Src );

}

template< typename TContourVertexType, typename TTriangleVertexType >
class TTriangulator : ATriangulatorBase {
public:
    struct SPolygon
    {
        TContourVertexType * OuterContour;
        int OuterContourVertexCount;
        TStdVector< std::pair< TContourVertexType *, int > > HoleContours;
        Double3 Normal;
    };

    TTriangulator( TStdVector< TTriangleVertexType > * pOutputStreamVertices, TPodArray< unsigned int > * pOutputStreamIndices );

    void Triangulate( SPolygon const * Polygon );

private:
    static void OnBeginData( uint32_t _Topology, void * _PolygonData );
    static void OnEndData( void * _PolygonData );
    static void OnVertexData( void * _Data, void * _PolygonData );
    static void OnCombineData( double _Position[3], void * _Data[4], float _Weight[4], void ** _OutData, void * _PolygonData );

    unsigned int FindOrCreateVertex( TTriangleVertexType *  _Vertex );

    // Output indices pointer
    TPodArray< unsigned int > * pIndexStream;

    // Output vertices pointer
    TStdVector< TTriangleVertexType > * pVertexStream;

    // Offset to vertex stream
    int VertexOffset;

    // Current filling contour
    TPodArray< TTriangleVertexType * > PrimitiveIndices;
    int CurrentTopology;

    // Vertex cache
    TPodArray< TTriangleVertexType * > VertexCache;

    // Temporary allocated verts
    TLinearAllocator< Align( sizeof( TTriangleVertexType ), 16 ) * 1024 > VertexAllocator;
    TPodArray< TTriangleVertexType * > AllocatedVerts;
};

template< typename TContourVertexType, typename TTriangleVertexType >
TTriangulator< TContourVertexType, TTriangleVertexType >::TTriangulator( TStdVector< TTriangleVertexType > * pOutputStreamVertices, TPodArray< unsigned int > * pOutputStreamIndices )
    : pIndexStream( pOutputStreamIndices )
    , pVertexStream( pOutputStreamVertices )
{
    SetCallback( CB_BEGIN_DATA, (SCallback)OnBeginData );
    SetCallback( CB_END_DATA, (SCallback)OnEndData );
    SetCallback( CB_VERTEX_DATA, (SCallback)OnVertexData );
    SetCallback( CB_COMBINE_DATA, (SCallback)OnCombineData );
}

template< typename TContourVertexType, typename TTriangleVertexType >
void TTriangulator< TContourVertexType, TTriangleVertexType >::OnBeginData( uint32_t _Topology, void * _PolygonData )
{
    TTriangulator< TContourVertexType, TTriangleVertexType > * tr = static_cast< TTriangulator< TContourVertexType, TTriangleVertexType > * >(_PolygonData);

    tr->PrimitiveIndices.Clear();
    tr->CurrentTopology = _Topology;
}

AN_INLINE bool IsTriangleValid( Double3 const & _a, Double3 const & _b, Double3 const & _c )
{
    double tmp1 = _c.X - _a.X;
    double tmp2 = _b.X - _a.X;
    return Math::Abs( tmp1 * (_b.Y - _a.Y) - tmp2 * (_c.Y - _a.Y) ) > 0.0001
        || Math::Abs( tmp1 * (_b.Z - _a.Z) - tmp2 * (_c.Z - _a.Z) ) > 0.0001;
}

template< typename TContourVertexType, typename TTriangleVertexType >
void TTriangulator< TContourVertexType, TTriangleVertexType >::OnEndData( void * _PolygonData )
{
    TTriangulator< TContourVertexType, TTriangleVertexType > * tr = static_cast< TTriangulator< TContourVertexType, TTriangleVertexType > * >(_PolygonData);

    if ( tr->PrimitiveIndices.Size() > 2 ) {
        const int TRIANGLES = 0x0004;
        const int TRIANGLE_STRIP = 0x0005;
        const int TRIANGLE_FAN = 0x0006;

        int numIndices = tr->PrimitiveIndices.Size();
        Double3 v[3];

        if ( tr->CurrentTopology == TRIANGLES ) {
            for ( int j = 0 ; j < numIndices ; j += 3 ) {
                TriangulatorTraits::TriangleVertexPosition( v[0], *tr->PrimitiveIndices[j+0] );
                TriangulatorTraits::TriangleVertexPosition( v[1], *tr->PrimitiveIndices[j+1] );
                TriangulatorTraits::TriangleVertexPosition( v[2], *tr->PrimitiveIndices[j+2] );

                if ( IsTriangleValid( v[0], v[1], v[2] ) ) {
                    tr->pIndexStream->Append( tr->VertexOffset + tr->FindOrCreateVertex( tr->PrimitiveIndices[j+0] ) );
                    tr->pIndexStream->Append( tr->VertexOffset + tr->FindOrCreateVertex( tr->PrimitiveIndices[j+1] ) );
                    tr->pIndexStream->Append( tr->VertexOffset + tr->FindOrCreateVertex( tr->PrimitiveIndices[j+2] ) );
                }
            }
        } else if ( tr->CurrentTopology == TRIANGLE_FAN ) {
            unsigned int ind[3] = { std::numeric_limits< unsigned int >::max(), 0, 0 };

            TriangulatorTraits::TriangleVertexPosition( v[0], *tr->PrimitiveIndices[0] );
            TriangulatorTraits::TriangleVertexPosition( v[1], *tr->PrimitiveIndices[1] );

            for ( int j = 0 ; j < numIndices-2 ; j++ ) {
                TriangulatorTraits::TriangleVertexPosition( v[2], *tr->PrimitiveIndices[j+2] );

                if ( IsTriangleValid( v[0], v[1], v[2] ) ) {
                    if ( ind[0] == std::numeric_limits< unsigned int >::max() ) {
                        ind[0] = tr->VertexOffset + tr->FindOrCreateVertex( tr->PrimitiveIndices[0] );
                    }

                    ind[1] = tr->VertexOffset + tr->FindOrCreateVertex( tr->PrimitiveIndices[j+1] );
                    ind[2] = tr->VertexOffset + tr->FindOrCreateVertex( tr->PrimitiveIndices[j+2] );

                    tr->pIndexStream->Append( ind[0] );
                    tr->pIndexStream->Append( ind[1] );
                    tr->pIndexStream->Append( ind[2] );
                }

                v[1] = v[2];
            }
        } else if ( tr->CurrentTopology == TRIANGLE_STRIP ) {
            TTriangleVertexType * Vertex[3];
            for ( int j = 0 ; j < numIndices-2 ; j++ ) {
                Vertex[0] = tr->PrimitiveIndices[j + (j&1)];
                Vertex[1] = tr->PrimitiveIndices[j - (j&1) + 1];
                Vertex[2] = tr->PrimitiveIndices[j + 2];

                TriangulatorTraits::TriangleVertexPosition( v[0], *Vertex[0] );
                TriangulatorTraits::TriangleVertexPosition( v[1], *Vertex[1] );
                TriangulatorTraits::TriangleVertexPosition( v[2], *Vertex[2] );

                if ( IsTriangleValid( v[0], v[1], v[2] ) ) {
                    tr->pIndexStream->Append( tr->VertexOffset + tr->FindOrCreateVertex( Vertex[0] ) );
                    tr->pIndexStream->Append( tr->VertexOffset + tr->FindOrCreateVertex( Vertex[1] ) );
                    tr->pIndexStream->Append( tr->VertexOffset + tr->FindOrCreateVertex( Vertex[2] ) );
                }
            }
        }

        tr->PrimitiveIndices.Clear();
    }
}

template< typename TContourVertexType, typename TTriangleVertexType >
unsigned int TTriangulator< TContourVertexType, TTriangleVertexType >::FindOrCreateVertex( TTriangleVertexType *  _Vertex )
{
    int VerticesCount = VertexCache.Size();
    for ( int i = 0 ; i < VerticesCount ; i++ ) {
        if ( VertexCache[i] == _Vertex ) {
            return i;
        }
    }
    VertexCache.Append( _Vertex );
    return VertexCache.Size() - 1;
}

template< typename TContourVertexType, typename TTriangleVertexType >
void TTriangulator< TContourVertexType, TTriangleVertexType >::OnVertexData( void * _Data, void * _PolygonData )
{
    TTriangulator< TContourVertexType, TTriangleVertexType > * tr = static_cast< TTriangulator< TContourVertexType, TTriangleVertexType > * >(_PolygonData);

    tr->PrimitiveIndices.Append( static_cast< TTriangleVertexType * >(_Data)/*tr->VertexOffset + tr->FindOrCreateVertex( static_cast< TTriangleVertexType * >( _Data ) )*/ );
}

template< typename TContourVertexType, typename TTriangleVertexType >
void TTriangulator< TContourVertexType, TTriangleVertexType >::OnCombineData( double _Position[3], void * _Data[4], float _Weight[4], void ** _OutData, void * _PolygonData )
{
    TTriangulator< TContourVertexType, TTriangleVertexType > * tr = static_cast< TTriangulator< TContourVertexType, TTriangleVertexType > * >(_PolygonData);

    void * data = tr->VertexAllocator.Allocate( sizeof( TTriangleVertexType ) );
    TTriangleVertexType * v = new( data ) TTriangleVertexType;

    TriangulatorTraits::CombineVertex< TTriangleVertexType >(
        *v,
        *((const Double3 *)&_Position[0]),
        const_cast< const float * >(_Weight),
        *static_cast< TTriangleVertexType * >(_Data[0]),
        *static_cast< TTriangleVertexType * >(_Data[1]),
        *static_cast< TTriangleVertexType * >(_Data[2]),
        *static_cast< TTriangleVertexType * >(_Data[3]) );

    *_OutData = v;
    tr->VertexCache.Append( v );
    tr->AllocatedVerts.Append( v );
}

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4189)
#endif

template< typename TContourVertexType, typename TTriangleVertexType >
void TTriangulator< TContourVertexType, TTriangleVertexType >::Triangulate( SPolygon const * Polygon )
{
    Double3 tmpPosition;

    VertexOffset = pVertexStream->Size();

    SetNormal( Polygon->Normal );

    BeginPolygon( this );

    BeginContour();
    TContourVertexType const * outerContour = Polygon->OuterContour;
    TContourVertexType const * outerContourEnd = Polygon->OuterContour + Polygon->OuterContourVertexCount;
    while ( outerContour < outerContourEnd ) {
        TriangulatorTraits::ContourVertexPosition( tmpPosition, *outerContour );
        ProcessVertex( tmpPosition, outerContour );
        outerContour++;
    }
    EndContour();

    for ( int i = 0 ; i < Polygon->HoleContours.Size() ; i++ ) {
        TContourVertexType const * holeContour = Polygon->HoleContours[i].first;
        TContourVertexType const * holeContourEnd = Polygon->HoleContours[i].first + Polygon->HoleContours[i].second;

        BeginContour();
        while ( holeContour < holeContourEnd ) {
            TriangulatorTraits::ContourVertexPosition( tmpPosition, *holeContour );
            ProcessVertex( tmpPosition, holeContour );
            holeContour++;
        }
        EndContour();
    }

    EndPolygon();

    // Fill vertices
    pVertexStream->Resize( VertexOffset + VertexCache.Size() );
    TTriangleVertexType * pVertex = pVertexStream->ToPtr() + VertexOffset;
    for ( TTriangleVertexType const * v : VertexCache ) {
        TriangulatorTraits::CopyVertex( *pVertex, *v );
        pVertex++;
    }

    // Call dtor for allocated vertices
    for ( TTriangleVertexType * v : AllocatedVerts ) {
        v->~TTriangleVertexType();
    }

    VertexCache.Clear();
    AllocatedVerts.Clear();
    VertexAllocator.Free();
}

#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif
