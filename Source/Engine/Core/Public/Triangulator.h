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

template< typename ContourVertex >
void ContourVertexPosition( Double3 & _Dst, ContourVertex const & _Src );

template< typename TriangleVertex >
void TriangleVertexPosition( Double3 & _Dst, TriangleVertex const & _Src );

template< typename TriangleVertex >
void CombineVertex( TriangleVertex & _OutputVertex,
                    Double3 const & _Position,
                    float const * _Weights,
                    TriangleVertex const & _V0,
                    TriangleVertex const & _V1,
                    TriangleVertex const & _V2,
                    TriangleVertex const & _V3 );

template< typename ContourVertex, typename TriangleVertex >
void CopyVertex( TriangleVertex & _Dst, ContourVertex const & _Src );

}

template< typename ContourVertex, typename TriangleVertex >
class TTriangulator : ATriangulatorBase {
public:
    struct SPolygon
    {
        ContourVertex * OuterContour;
        int OuterContourVertexCount;
        TStdVector< std::pair< ContourVertex *, int > > HoleContours;
        Double3 Normal;
    };

    TTriangulator( TStdVector< TriangleVertex > * pOutputStreamVertices, TPodArray< unsigned int > * pOutputStreamIndices );

    void Triangulate( SPolygon const * Polygon );

private:
    static void OnBeginData( uint32_t _Topology, void * _PolygonData );
    static void OnEndData( void * _PolygonData );
    static void OnVertexData( void * _Data, void * _PolygonData );
    static void OnCombineData( double _Position[3], void * _Data[4], float _Weight[4], void ** _OutData, void * _PolygonData );
    static bool IsTriangleValid( Double3 const & _a, Double3 const & _b, Double3 const & _c );

    unsigned int FindOrCreateVertex( TriangleVertex *  _Vertex );

    // Output indices pointer
    TPodArray< unsigned int > * pIndexStream;

    // Output vertices pointer
    TStdVector< TriangleVertex > * pVertexStream;

    // Offset to vertex stream
    int VertexOffset;

    // Current filling contour
    TPodArray< TriangleVertex * > PrimitiveIndices;
    int CurrentTopology;

    // Vertex cache
    TPodArray< TriangleVertex * > VertexCache;

    // Temporary allocated verts
    TLinearAllocator< Align( sizeof( TriangleVertex ), 16 ) * 1024 > VertexAllocator;
    TPodArray< TriangleVertex * > AllocatedVerts;
};

template< typename ContourVertex, typename TriangleVertex >
TTriangulator< ContourVertex, TriangleVertex >::TTriangulator( TStdVector< TriangleVertex > * pOutputStreamVertices, TPodArray< unsigned int > * pOutputStreamIndices )
    : pIndexStream( pOutputStreamIndices )
    , pVertexStream( pOutputStreamVertices )
{
    SetCallback( CB_BEGIN_DATA, (SCallback)OnBeginData );
    SetCallback( CB_END_DATA, (SCallback)OnEndData );
    SetCallback( CB_VERTEX_DATA, (SCallback)OnVertexData );
    SetCallback( CB_COMBINE_DATA, (SCallback)OnCombineData );
}

template< typename ContourVertex, typename TriangleVertex >
void TTriangulator< ContourVertex, TriangleVertex >::OnBeginData( uint32_t _Topology, void * _PolygonData )
{
    TTriangulator< ContourVertex, TriangleVertex > * tr = static_cast< TTriangulator< ContourVertex, TriangleVertex > * >(_PolygonData);

    tr->PrimitiveIndices.Clear();
    tr->CurrentTopology = _Topology;
}

template< typename ContourVertex, typename TriangleVertex >
bool TTriangulator< ContourVertex, TriangleVertex >::IsTriangleValid( Double3 const & _a, Double3 const & _b, Double3 const & _c )
{
    double tmp1 = _c.X - _a.X;
    double tmp2 = _b.X - _a.X;
    return Math::Abs( tmp1 * (_b.Y - _a.Y) - tmp2 * (_c.Y - _a.Y) ) > 0.0001
        || Math::Abs( tmp1 * (_b.Z - _a.Z) - tmp2 * (_c.Z - _a.Z) ) > 0.0001;
}

template< typename ContourVertex, typename TriangleVertex >
void TTriangulator< ContourVertex, TriangleVertex >::OnEndData( void * _PolygonData )
{
    TTriangulator< ContourVertex, TriangleVertex > * tr = static_cast< TTriangulator< ContourVertex, TriangleVertex > * >(_PolygonData);

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
            TriangleVertex * Vertex[3];
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

template< typename ContourVertex, typename TriangleVertex >
unsigned int TTriangulator< ContourVertex, TriangleVertex >::FindOrCreateVertex( TriangleVertex *  _Vertex )
{
    int verticesCount = VertexCache.Size();
    for ( int i = 0 ; i < verticesCount ; i++ ) {
        // Compare pointers
        if ( VertexCache[i] == _Vertex ) {
            return i;
        }
    }
    VertexCache.Append( _Vertex );
    return VertexCache.Size() - 1;
}

template< typename ContourVertex, typename TriangleVertex >
void TTriangulator< ContourVertex, TriangleVertex >::OnVertexData( void * _Data, void * _PolygonData )
{
    TTriangulator< ContourVertex, TriangleVertex > * tr = static_cast< TTriangulator< ContourVertex, TriangleVertex > * >(_PolygonData);

    tr->PrimitiveIndices.Append( static_cast< TriangleVertex * >(_Data)/*tr->VertexOffset + tr->FindOrCreateVertex( static_cast< TriangleVertex * >( _Data ) )*/ );
}

template< typename ContourVertex, typename TriangleVertex >
void TTriangulator< ContourVertex, TriangleVertex >::OnCombineData( double _Position[3], void * _Data[4], float _Weight[4], void ** _OutData, void * _PolygonData )
{
    TTriangulator< ContourVertex, TriangleVertex > * tr = static_cast< TTriangulator< ContourVertex, TriangleVertex > * >(_PolygonData);

    void * data = tr->VertexAllocator.Allocate( sizeof( TriangleVertex ) );
    TriangleVertex * v = new( data ) TriangleVertex;

    TriangulatorTraits::CombineVertex< TriangleVertex >(
        *v,
        *((const Double3 *)&_Position[0]),
        const_cast< const float * >(_Weight),
        *static_cast< TriangleVertex * >(_Data[0]),
        *static_cast< TriangleVertex * >(_Data[1]),
        *static_cast< TriangleVertex * >(_Data[2]),
        *static_cast< TriangleVertex * >(_Data[3]) );

    *_OutData = v;
    tr->VertexCache.Append( v );
    tr->AllocatedVerts.Append( v );
}

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4189)
#endif

template< typename ContourVertex, typename TriangleVertex >
void TTriangulator< ContourVertex, TriangleVertex >::Triangulate( SPolygon const * Polygon )
{
    Double3 tmpPosition;

    VertexOffset = pVertexStream->Size();

    SetNormal( Polygon->Normal );

    BeginPolygon( this );

    BeginContour();
    ContourVertex const * outerContour = Polygon->OuterContour;
    ContourVertex const * outerContourEnd = Polygon->OuterContour + Polygon->OuterContourVertexCount;
    while ( outerContour < outerContourEnd ) {
        TriangulatorTraits::ContourVertexPosition( tmpPosition, *outerContour );
        ProcessVertex( tmpPosition, outerContour );
        outerContour++;
    }
    EndContour();

    for ( int i = 0 ; i < Polygon->HoleContours.Size() ; i++ ) {
        ContourVertex const * holeContour = Polygon->HoleContours[i].first;
        ContourVertex const * holeContourEnd = Polygon->HoleContours[i].first + Polygon->HoleContours[i].second;

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
    TriangleVertex * pVertex = pVertexStream->ToPtr() + VertexOffset;
    for ( TriangleVertex const * v : VertexCache ) {
        TriangulatorTraits::CopyVertex( *pVertex, *v );
        pVertex++;
    }

    // Call dtor for allocated vertices
    for ( TriangleVertex * v : AllocatedVerts ) {
        v->~TriangleVertex();
    }

    VertexCache.Clear();
    AllocatedVerts.Clear();
    VertexAllocator.Free();
}

#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif
