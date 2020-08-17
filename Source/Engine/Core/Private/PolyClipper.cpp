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

#include <Core/Public/PolyClipper.h>
#include "clipper/clipper.hpp"

using SClipperPoint = ClipperLib::IntPoint;
using SClipperPath = ClipperLib::Path;
using SClipperPaths = ClipperLib::Paths;

static const double CLIPPER_TO_LONG_CONVERSION_NUMBER = 1000000000.0;//1518500249; // clipper.cpp / hiRange var
static const double CLIPPER_TO_DOUBLE_CONVERSION_NUMBER = 0.000000001;
static const double CLIPPER_TO_FLOAT_CONVERSION_NUMBER = 0.000000001;

#define CLIPPER_DOUBLE_TO_LONG(p)     ( static_cast< ClipperLib::cInt >( ( p ) * CLIPPER_TO_LONG_CONVERSION_NUMBER ) )
#define CLIPPER_LONG_TO_DOUBLE(p)     ( ( p ) * CLIPPER_TO_DOUBLE_CONVERSION_NUMBER )

#define CLIPPER_FLOAT_TO_LONG(p)     ( static_cast< ClipperLib::cInt >( ( p ) * CLIPPER_TO_LONG_CONVERSION_NUMBER ) )
#define CLIPPER_LONG_TO_FLOAT(p)     ( ( p ) * CLIPPER_TO_FLOAT_CONVERSION_NUMBER )

APolyClipper::APolyClipper()
    : pClipper( std::make_unique< ClipperLib::Clipper >() )
    , Transform3D( Float3x3::Identity() )
    , InvTransform3D( Float3x3::Identity() )
{
}

APolyClipper::~APolyClipper()
{
}

void APolyClipper::SetTransform( Float3x3 const & _Transform3D )
{
    Transform3D = _Transform3D;
    InvTransform3D = Transform3D.Transposed();
}

void APolyClipper::SetTransformFromNormal( Float3 const & _Normal )
{
    Transform3D[2] = _Normal;

    _Normal.ComputeBasis( Transform3D[0], Transform3D[1] );

    InvTransform3D = Transform3D.Transposed();
}

static void ConstructClipperPath( const double * _PointsXYZ, int _PointsCount, Float3x3 const & _InvTransform3D, SClipperPath & _Path )
{
    Double3 p;
    for ( int i = 0 ; i < _PointsCount ; i++, _PointsXYZ += 3 ) {
        p = _InvTransform3D * *(Double3 *)_PointsXYZ;
        _Path.emplace_back( SClipperPoint( CLIPPER_DOUBLE_TO_LONG( p.X ), CLIPPER_DOUBLE_TO_LONG( p.Y ) ) );
    }
}

static void ConstructClipperPath( const double * _PointsXY, int _PointsCount, SClipperPath & _Path )
{
    for ( int i = 0 ; i < _PointsCount ; i++, _PointsXY += 2 ) {
        _Path.emplace_back( SClipperPoint( CLIPPER_DOUBLE_TO_LONG( _PointsXY[0] ), CLIPPER_DOUBLE_TO_LONG( _PointsXY[1] ) ) );
    }
}

void APolyClipper::AddSubj2D( Double2 const * _Points, int _NumPoints, bool _Closed )
{
    SClipperPath Path;

    ConstructClipperPath( &_Points->X, _NumPoints, Path );

    pClipper->AddPath( Path, ClipperLib::ptSubject, _Closed );
}

void APolyClipper::AddClip2D( Double2 const * _Points, int _NumPoints, bool _Closed )
{
    SClipperPath Path;

    ConstructClipperPath( &_Points->X, _NumPoints, Path );

    pClipper->AddPath( Path, ClipperLib::ptClip, _Closed );
}

void APolyClipper::AddSubj3D( Double3 const * _Points, int _NumPoints, bool _Closed )
{
    SClipperPath Path;

    ConstructClipperPath( &_Points->X, _NumPoints, InvTransform3D, Path );

    pClipper->AddPath( Path, ClipperLib::ptSubject, _Closed );
}

void APolyClipper::AddClip3D( Double3 const * _Points, int _NumPoints, bool _Closed )
{
    SClipperPath Path;

    ConstructClipperPath( &_Points->X, _NumPoints, InvTransform3D, Path );

    pClipper->AddPath( Path, ClipperLib::ptClip, _Closed );
}

static void ConstructContour( SClipperPath const & _Path, AClipperContour & _Contour )
{
    int numPoints = _Path.size();

    _Contour.Resize( numPoints );
    for ( int j = 0 ; j < numPoints ; j++ ) {
        SClipperPoint const & point = _Path[j];
        _Contour[j].X = CLIPPER_LONG_TO_DOUBLE( point.X );
        _Contour[j].Y = CLIPPER_LONG_TO_DOUBLE( point.Y );
    }
}

static void ComputeNode_r( ClipperLib::PolyNode const & _Node, TStdVector< SClipperPolygon > & _Polygons ) {
    SClipperPolygon polygon;
    AClipperContour hole;

    ConstructContour( _Node.Contour, polygon.Outer );

    for ( int j = 0 ; j < (int)_Node.ChildCount() ; j++ ) {
        ClipperLib::PolyNode const & child = *_Node.Childs[j];

        if ( child.IsHole() && !child.IsOpen() ) {
            ConstructContour( child.Contour, hole );
            polygon.Holes.Append( hole );

            // FIXME: May hole have childs?
            AN_ASSERT( child.ChildCount() == 0 );

        } else {
            if ( !child.IsOpen() ) {
                ComputeNode_r( child, _Polygons );
            }
        }
    }

    _Polygons.Append( polygon );
}

static void ComputeContours( ClipperLib::PolyTree const & _PolygonTree, TStdVector< SClipperPolygon > & _Polygons )
{
    if ( _PolygonTree.Contour.size() > 0 && !_PolygonTree.IsOpen() ) {
        ComputeNode_r( _PolygonTree, _Polygons );
    } else {
        for ( int i = 0 ; i < _PolygonTree.ChildCount() ; i++ ) {
            ClipperLib::PolyNode & child = *_PolygonTree.Childs[i];

            if ( child.IsHole() ) {

                // FIXME: May hole have childs?

                AN_ASSERT( 0 );

            } else {
                if ( !child.IsOpen() ) {
                    ComputeNode_r( child, _Polygons );
                }
            }
        }
    }
}

bool APolyClipper::Execute( EClipType _ClipType, TStdVector< SClipperPolygon > & _Polygons )
{
    ClipperLib::PolyTree polygonTree;

    pClipper->StrictlySimple( true );

    if ( !pClipper->Execute( (ClipperLib::ClipType)_ClipType, polygonTree, ClipperLib::pftNonZero, ClipperLib::pftNonZero ) ) {
        return false;
    }

    ComputeContours( polygonTree, _Polygons );
    return true;
}

bool APolyClipper::Execute( EClipType _ClipType, TStdVector< AClipperContour > & _Contours )
{
    SClipperPaths resultPaths;

    pClipper->StrictlySimple( true );

    if ( !pClipper->Execute( (ClipperLib::ClipType)_ClipType, resultPaths, ClipperLib::pftNonZero, ClipperLib::pftNonZero ) ) {
        return false;
    }

    _Contours.Resize( resultPaths.size() );
    for ( int i = 0 ; i < _Contours.Size() ; i++ ) {
        ConstructContour( resultPaths[i], _Contours[i] );
    }
    return true;
}

void APolyClipper::Clear()
{
    pClipper->Clear();
}
