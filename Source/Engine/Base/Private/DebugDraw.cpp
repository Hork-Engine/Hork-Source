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

#include <Engine/Base/Public/DebugDraw.h>
#include <Engine/Core/Public/Color.h>

#define PRIMITIVE_RESTART_INDEX    0xffffffff

FDebugDraw::FDebugDraw() {
}

FDebugDraw::~FDebugDraw() {
}

void FDebugDraw::Reset() {
    CurrentColor = 0xffffffff;
    bDepthTest = false;

    FRenderFrame * frameData = GRuntime.GetFrameData();

    Vertices = &frameData->DbgVertices;
    Indices = &frameData->DbgIndices;
    Cmds = &frameData->DbgCmds;

    Vertices->Clear();
    Indices->Clear();
    Cmds->Clear();

    FirstVertex = 0;
    FirstIndex = 0;
    bSplit = false;
}

void FDebugDraw::SetDepthTest( bool _DepthTest ) {
    bDepthTest = _DepthTest;
}

void FDebugDraw::SetColor( uint32_t _Color ) {
    CurrentColor = _Color;
}

void FDebugDraw::SetColor( FColor4 const & _Color ) {
    CurrentColor = _Color.GetDWord();
}

void FDebugDraw::SetAlpha( float _Alpha ) {
    CurrentColor &= 0x00ffffff;
    CurrentColor |= FMath::Clamp( FMath::ToIntFast( _Alpha * 255 ), 0, 255 ) << 24;
}

void FDebugDraw::SplitCommands() {
    bSplit = true;
}

FDebugDrawCmd & FDebugDraw::SetDrawCmd( EDebugDrawCmd _Type ) {

    // Create first cmd
    if ( Cmds->IsEmpty() || bSplit ) {
        FDebugDrawCmd & cmd = Cmds->Append();
        cmd.Type = _Type;
        cmd.FirstVertex = FirstVertex;
        cmd.FirstIndex = FirstIndex;
        cmd.NumVertices = 0;
        cmd.NumIndices = 0;
        bSplit = false;
        return cmd;
    }

    // If last cmd has no indices, use it
    if ( Cmds->Last().NumIndices == 0 ) {
        FDebugDrawCmd & cmd = Cmds->Last();
        cmd.Type = _Type;
        cmd.FirstVertex = FirstVertex;
        cmd.FirstIndex = FirstIndex;
        cmd.NumVertices = 0;
        return cmd;
    }

    // If last cmd has other type
    if ( Cmds->Last().Type != _Type ) {
        FDebugDrawCmd & cmd = Cmds->Append();
        cmd.Type = _Type;
        cmd.FirstVertex = FirstVertex;
        cmd.FirstIndex = FirstIndex;
        cmd.NumVertices = 0;
        cmd.NumIndices = 0;
        return cmd;
    }

    return Cmds->Last();
}

void FDebugDraw::PrimitiveReserve( int _NumVertices, int _NumIndices ) {
    Vertices->Resize( Vertices->Size() + _NumVertices );
    Indices->Resize( Indices->Size() + _NumIndices );
}

void FDebugDraw::DrawPoint( Float3 const & _Position ) {
    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS );

    PrimitiveReserve( 1, 1 );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    verts->Position = _Position;
    verts->Color = CurrentColor;

    indices[0] = FirstVertex;

    FirstVertex++;
    FirstIndex++;

    cmd.NumVertices++;
    cmd.NumIndices++;
}

void FDebugDraw::DrawPoints( Float3 const * _Points, int _NumPoints, int _Stride ) {
    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS );

    PrimitiveReserve( _NumPoints, _NumPoints );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    byte * pPoints = ( byte * )_Points;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++, indices++ ) {
        verts->Position = *( Float3 * )pPoints;
        verts->Color = CurrentColor;
        *indices = FirstVertex + i;

        pPoints += _Stride;
    }

    FirstVertex += _NumPoints;
    FirstIndex += _NumPoints;

    cmd.NumVertices += _NumPoints;
    cmd.NumIndices += _NumPoints;
}

void FDebugDraw::DrawLine( Float3 const & _P0, Float3 const & _P1 ) {
    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES );

    PrimitiveReserve( 2, 3 );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    verts[0].Position = _P0;
    verts[0].Color = CurrentColor;
    verts[1].Position = _P1;
    verts[1].Color = CurrentColor;

    indices[0] = FirstVertex;
    indices[1] = FirstVertex + 1;
    indices[2] = PRIMITIVE_RESTART_INDEX;

    FirstVertex += 2;
    FirstIndex += 3;

    cmd.NumVertices += 2;
    cmd.NumIndices += 3;
}

void FDebugDraw::DrawDottedLine( Float3 const & _P0, Float3 const & _P1, float _Step ) {
    Float3 vector = _P1 - _P0;
    float len = vector.Length();
    Float3 dir = vector * ( 1.0f / len );
    float position = _Step * 0.5f;

    while ( position < len ) {
        float nextPosition = position + _Step;
        if ( nextPosition > len ) {
            nextPosition = len;
        }
        DrawLine( _P0 + dir * position, _P0 + dir * nextPosition );
        position = nextPosition + _Step;
    }
}

void FDebugDraw::DrawLine( Float3 const * _Points, int _NumPoints, bool _Closed ) {
    if ( _NumPoints < 2 ) {
        return;
    }

    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES );

    int numIndices = _Closed ? _NumPoints + 2 : _NumPoints + 1;

    PrimitiveReserve( _NumPoints, numIndices );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++, indices++ ) {
        verts->Position = _Points[i];
        verts->Color = CurrentColor;
        *indices = FirstVertex + i;
    }

    if ( _Closed ) {
        *indices++ = FirstVertex;
    }

    *indices = PRIMITIVE_RESTART_INDEX;

    FirstVertex += _NumPoints;
    FirstIndex += numIndices;

    cmd.NumVertices += _NumPoints;
    cmd.NumIndices += numIndices;
}

void FDebugDraw::DrawConvexPoly( Float3 const * _Points, int _NumPoints, bool _TwoSided ) {
    if ( _NumPoints < 3 ) {
        return;
    }

    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP );

    int numTriangles = _NumPoints - 2;
    int numIndices = numTriangles * 3;

    if ( _TwoSided ) {
        numIndices <<= 1;
    }

    PrimitiveReserve( _NumPoints, numIndices );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++ ) {
        verts->Position = _Points[i];
        verts->Color = CurrentColor;
    }

    for ( int i = 0 ; i < numTriangles ; i++ ) {
        *indices++ = FirstVertex + 0;
        *indices++ = FirstVertex + i + 1;
        *indices++ = FirstVertex + i + 2;
    }

    if ( _TwoSided ) {
        for ( int i = numTriangles-1 ; i >= 0 ; i-- ) {
            *indices++ = FirstVertex + 0;
            *indices++ = FirstVertex + i + 2;
            *indices++ = FirstVertex + i + 1;
        }
    }

    FirstVertex += _NumPoints;
    FirstIndex += numIndices;

    cmd.NumVertices += _NumPoints;
    cmd.NumIndices += numIndices;
}

void FDebugDraw::DrawTriangleSoup( Float3 const * _Points, int _NumPoints, int _Stride, unsigned int const * _Indices, int _NumIndices, bool _TwoSided ) {
    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP );

    int numIndices = _NumIndices;

    if ( _TwoSided ) {
        numIndices <<= 1;
    }

    PrimitiveReserve( _NumPoints, numIndices );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    byte * pPoints = ( byte * )_Points;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++ ) {
        verts->Position = *( Float3 * )pPoints;
        verts->Color = CurrentColor;

//#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
        verts->Color = FColor4( Float4(i%255,i%255,i%255,255)/255.0f ).GetDWord();
#endif

        pPoints += _Stride;
    }

    for ( int i = 0 ; i < _NumIndices ; i++ ) {
        *indices++ = FirstVertex + _Indices[i];
    }

    if ( _TwoSided ) {
        _Indices += _NumIndices;
        for ( int i = _NumIndices-1 ; i >= 0 ; i-- ) {
            *indices++ = FirstVertex + *--_Indices;
        }
    }

    FirstVertex += _NumPoints;
    FirstIndex += numIndices;

    cmd.NumVertices += _NumPoints;
    cmd.NumIndices += numIndices;
}

void FDebugDraw::DrawTriangleSoupWireframe( Float3 const * _Points, int _Stride, unsigned int const * _Indices, int _NumIndices ) {
    Float3 points[3];
    byte * pPoints = ( byte * )_Points;
    for ( int i = 0 ; i < _NumIndices ; i+=3 ) {
        points[0] = *(Float3 *)(pPoints + _Stride * _Indices[i+0]);
        points[1] = *(Float3 *)(pPoints + _Stride * _Indices[i+1]);
        points[2] = *(Float3 *)(pPoints + _Stride * _Indices[i+2]);
        DrawLine( points, 3, true );
    }

}

void FDebugDraw::DrawTriangle( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2, bool _TwoSided ) {
    Float3 const Points[] = { _P0, _P1, _P2 };
    DrawConvexPoly( Points, 3, _TwoSided );
}

void FDebugDraw::DrawTriangles( Float3 const * _Triangles, int _NumTriangles, int _Stride, bool _TwoSided ) {
    FDebugDrawCmd & cmd = SetDrawCmd( bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP );

    int numPoints = _NumTriangles * 3;
    int numIndices = _NumTriangles * 3;
    int totalIndices = _TwoSided ? numIndices << 1 : numIndices;

    PrimitiveReserve( numPoints, totalIndices );

    FDebugVertex * verts = Vertices->ToPtr() + FirstVertex;
    unsigned int * indices = Indices->ToPtr() + FirstIndex;

    byte * pPoints = ( byte * )_Triangles;

    for ( int i = 0 ; i < numPoints ; i++, verts++ ) {
        verts->Position = *( Float3 * )pPoints;
        verts->Color = CurrentColor;

        pPoints += _Stride;
    }

    for ( int i = 0 ; i < numIndices ; i++ ) {
        *indices++ = FirstVertex + i;
    }

    if ( _TwoSided ) {
        for ( int i = numIndices-1 ; i >= 0 ; i-- ) {
            *indices++ = FirstVertex + i;
        }
    }

    FirstVertex += numPoints;
    FirstIndex += totalIndices;

    cmd.NumVertices += numPoints;
    cmd.NumIndices += totalIndices;
}

void FDebugDraw::DrawBox( Float3 const & _Position, Float3 const & _HalfExtents ) {
    Float3 const points[8] = {
        Float3( -_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        Float3( -_HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        Float3( -_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        Float3( -_HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position
    };

    // top face
    DrawLine( points, 4, true );

    // bottom face
    DrawLine( &points[4], 4, true );

    // Edges
    DrawLine( points[0], points[4] );
    DrawLine( points[1], points[5] );
    DrawLine( points[2], points[6] );
    DrawLine( points[3], points[7] );
}

void FDebugDraw::DrawBoxFilled( Float3 const & _Position, Float3 const & _HalfExtents, bool _TwoSided ) {
    Float3 const points[8] = {
        Float3( -_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        Float3( -_HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        Float3( -_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        Float3(  _HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        Float3( -_HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position
    };

    unsigned int const indices[36] = { 0,3,2, 2,1,0, 7,4,5, 5,6,7, 3,7,6, 6,2,3, 2,6,5, 5,1,2, 1,5,4, 4,0,1, 0,4,7, 7,3,0 };

    DrawTriangleSoup( points, 8, sizeof( Float3 ), indices, 36, _TwoSided );
}

void FDebugDraw::DrawOrientedBox( Float3 const & _Position, Float3x3 const & _Orientation, Float3 const & _HalfExtents ) {
    Float3 const points[8] = {
        _Orientation * Float3( -_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        _Orientation * Float3( -_HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        _Orientation * Float3( -_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        _Orientation * Float3( -_HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position
    };

    // top face
    DrawLine( points, 4, true );

    // bottom face
    DrawLine( &points[4], 4, true );

    // Edges
    DrawLine( points[0], points[4] );
    DrawLine( points[1], points[5] );
    DrawLine( points[2], points[6] );
    DrawLine( points[3], points[7] );
}

void FDebugDraw::DrawOrientedBoxFilled( Float3 const & _Position, Float3x3 const & _Orientation, Float3 const & _HalfExtents, bool _TwoSided ) {
    Float3 const points[8] = {
        _Orientation * Float3( -_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        _Orientation * Float3( -_HalfExtents.X, _HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        _Orientation * Float3( -_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z ) + _Position,
        _Orientation * Float3(  _HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position,
        _Orientation * Float3( -_HalfExtents.X, -_HalfExtents.Y,  _HalfExtents.Z ) + _Position
    };

    unsigned int const indices[36] = { 0,3,2, 2,1,0, 7,4,5, 5,6,7, 3,7,6, 6,2,3, 2,6,5, 5,1,2, 1,5,4, 4,0,1, 0,4,7, 7,3,0 };

    DrawTriangleSoup( points, 8, sizeof( Float3 ), indices, 36, _TwoSided );
}

void FDebugDraw::DrawSphere( Float3 const & _Position, float _Radius ) {
    DrawOrientedSphere( _Position, Float3x3::Identity(), _Radius );
}

void FDebugDraw::DrawOrientedSphere( Float3 const & _Position, Float3x3 const & _Orientation, float _Radius ) {
    const float stepDegrees = 30.0f;
    DrawSpherePatch( _Position, _Orientation[1], _Orientation[0], _Radius, -FMath::_HALF_PI, FMath::_HALF_PI, -FMath::_HALF_PI, FMath::_HALF_PI, stepDegrees, false );
    DrawSpherePatch( _Position, _Orientation[1], -_Orientation[0], _Radius, -FMath::_HALF_PI, FMath::_HALF_PI, -FMath::_HALF_PI, FMath::_HALF_PI, stepDegrees, false );
}

void FDebugDraw::DrawSpherePatch( Float3 const & _Position, Float3 const & _Up, Float3 const & _Right, float _Radius,
                                  float _MinTh, float _MaxTh, float _MinPs, float _MaxPs, float _StepDegrees, bool _DrawCenter ) {
    // This function is based on code from btDebugDraw
    Float3 vA[ 74 ];
    Float3 vB[ 74 ];
    Float3 *pvA = vA, *pvB = vB, *pT;
    Float3 npole = _Position + _Up * _Radius;
    Float3 spole = _Position - _Up * _Radius;
    Float3 arcStart;
    float step = FMath::Radians( _StepDegrees );
    Float3 backVec = _Up.Cross( _Right );
    bool drawN = false;
    bool drawS = false;
    if ( _MinTh <= -FMath::_HALF_PI ) {
        _MinTh = -FMath::_HALF_PI + step;
        drawN = true;
    }
    if ( _MaxTh >= FMath::_HALF_PI ) {
        _MaxTh = FMath::_HALF_PI - step;
        drawS = true;
    }
    if ( _MinTh > _MaxTh ) {
        _MinTh = -FMath::_HALF_PI + step;
        _MaxTh = FMath::_HALF_PI - step;
        drawN = drawS = true;
    }
    int n_hor = ( int )( ( _MaxTh - _MinTh ) / step ) + 1;
    if ( n_hor < 2 ) n_hor = 2;
    float step_h = ( _MaxTh - _MinTh ) / float( n_hor - 1 );
    bool isClosed = false;
    if ( _MinPs > _MaxPs ) {
        _MinPs = -FMath::_PI + step;
        _MaxPs = FMath::_PI;
        isClosed = true;
    } else if ( _MaxPs - _MinPs >= FMath::_2PI ) {
        isClosed = true;
    } else {
        isClosed = false;
    }
    int n_vert = ( int )( ( _MaxPs - _MinPs ) / step ) + 1;
    if ( n_vert < 2 ) n_vert = 2;
    float step_v = ( _MaxPs - _MinPs ) / float( n_vert - 1 );
    for ( int i = 0; i < n_hor; i++ ) {
        float th = _MinTh + float( i ) * step_h;
        float sth;
        float cth;
        FMath::RadSinCos( th, sth, cth );
        sth *= _Radius;
        cth *= _Radius;
        for ( int j = 0; j < n_vert; j++ ) {
            float psi = _MinPs + float( j ) * step_v;
            float sps;
            float cps;
            FMath::RadSinCos( psi, sps, cps );
            pvB[ j ] = _Position + cth * cps * _Right + cth * sps * backVec + sth * _Up;
            if ( i ) {
                DrawLine( pvA[ j ], pvB[ j ] );
            } else if ( drawS ) {
                DrawLine( spole, pvB[ j ] );
            }
            if ( j ) {
                DrawLine( pvB[ j - 1 ], pvB[ j ] );
            } else {
                arcStart = pvB[ j ];
            }
            if ( ( i == ( n_hor - 1 ) ) && drawN ) {
                DrawLine( npole, pvB[ j ] );
            }

            if ( _DrawCenter ) {
                if ( isClosed ) {
                    if ( j == ( n_vert - 1 ) ) {
                        DrawLine( arcStart, pvB[ j ] );
                    }
                } else {
                    if ( ( !i || i == n_hor - 1 ) && ( !j || j == n_vert - 1 ) ) {
                        DrawLine( _Position, pvB[ j ] );
                    }
                }
            }
        }
        pT = pvA; pvA = pvB; pvB = pT;
    }
}

void FDebugDraw::DrawCircle( Float3 const & _Position, Float3 const & _UpVector, const float & _Radius ) {
    const int NumCirclePoints = 32;

    const Float3 v = _UpVector.Perpendicular() * _Radius;

    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = FMath::_2PI / NumCirclePoints * i;
        points[i] = _Position + Float3x3::RotationAroundNormal( Angle, _UpVector ) * v;
    }

    DrawLine( points, NumCirclePoints, true );
}

void FDebugDraw::DrawCircleFilled( Float3 const & _Position, Float3 const & _UpVector, const float & _Radius, bool _TwoSided ) {
    const int NumCirclePoints = 32;

    const Float3 v = _UpVector.Perpendicular() * _Radius;

    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = FMath::_2PI / NumCirclePoints * i;
        points[i] = _Position + Float3x3::RotationAroundNormal( Angle, _UpVector ) * v;
    }

    DrawConvexPoly( points, NumCirclePoints, _TwoSided );
}

void FDebugDraw::DrawCone( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _HalfAngle ) {
    const Float3 coneDirection = -_Orientation[2];
    const Float3 v = Float3x3::RotationAroundNormal( _HalfAngle,  _Orientation[0] ) * coneDirection * _Radius;

    const int NumCirclePoints = 32;
    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = FMath::_2PI / NumCirclePoints * i;
        points[i] = _Position + Float3x3::RotationAroundNormal( Angle, coneDirection ) * v;
    }

    // Draw cone circle
    DrawLine( points, NumCirclePoints, true );

    // Draw cone faces (rays)
    for ( int i = 0 ; i < NumCirclePoints ; i+=2 ) {
        DrawLine( _Position, points[i] );
    }
}

void FDebugDraw::DrawCylinder( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _Height ) {
    const Float3 upVector = _Orientation[1] * _Height;
    const Float3 v = _Orientation[0] * _Radius;
    const Float3 pos = _Position - _Orientation[1] * ( _Height * 0.5f );

    const int NumCirclePoints = 32;
    Float3 points[NumCirclePoints];

    points[0] = pos + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = FMath::_2PI / NumCirclePoints * i;
        points[i] = pos + Float3x3::RotationAroundNormal( Angle, _Orientation[1] ) * v;
    }

    // Draw bottom circle
    DrawLine( points, NumCirclePoints, true );

    // Draw faces (rays)
    for ( int i = 0 ; i < NumCirclePoints ; i+=2 ) {
        DrawLine( points[i], points[i] + upVector );
        points[i] += upVector;
        points[i+1] += upVector;
    }

    // Draw top circle
    DrawLine( points, NumCirclePoints, true );
}

void FDebugDraw::DrawCapsule( Float3 const & _Position, Float3x3 const & _Orientation, float _Radius, float _Height, int _UpAxis ) {
    AN_Assert( _UpAxis >= 0 && _UpAxis < 3 );

    const int stepDegrees = 30;
    const float halfHeight = _Height * 0.5f;

    Float3 capStart( 0.0f );
    capStart[ _UpAxis ] = -halfHeight;

    Float3 capEnd( 0.0f );
    capEnd[ _UpAxis ] = halfHeight;

    Float3 up = _Orientation.GetRow( ( _UpAxis + 1 ) % 3 );
    Float3 axis = _Orientation.GetRow( _UpAxis );

    DrawSpherePatch( _Orientation * capStart + _Position, up, -axis, _Radius, -FMath::_HALF_PI, FMath::_HALF_PI, -FMath::_HALF_PI, FMath::_HALF_PI, stepDegrees, false );
    DrawSpherePatch( _Orientation * capEnd + _Position, up, axis, _Radius, -FMath::_HALF_PI, FMath::_HALF_PI, -FMath::_HALF_PI, FMath::_HALF_PI, stepDegrees, false );

    for ( int angle = 0; angle < 360; angle += stepDegrees ) {
        float r = FMath::Radians( float( angle ) );
        capEnd[ ( _UpAxis + 1 ) % 3 ] = capStart[ ( _UpAxis + 1 ) % 3 ] = std::sin( r ) * _Radius;
        capEnd[ ( _UpAxis + 2 ) % 3 ] = capStart[ ( _UpAxis + 2 ) % 3 ] = std::cos( r ) * _Radius;
        DrawLine( _Position + _Orientation * capStart, _Position + _Orientation * capEnd );
    }
}

void FDebugDraw::DrawAABB( BvAxisAlignedBox const & _AABB ) {
    DrawBox( _AABB.Center(), _AABB.HalfSize() );
}

void FDebugDraw::DrawOBB( BvOrientedBox const & _OBB ) {
    DrawOrientedBox( _OBB.Center, _OBB.Orient, _OBB.HalfSize );
}

void FDebugDraw::DrawAxis( Float3x4 const & _TransformMatrix, bool _Normalized ) {
    Float3 Origin( _TransformMatrix[0][3], _TransformMatrix[1][3], _TransformMatrix[2][3] );
    Float3 XVec( _TransformMatrix[0][0], _TransformMatrix[1][0], _TransformMatrix[2][0] );
    Float3 YVec( _TransformMatrix[0][1], _TransformMatrix[1][1], _TransformMatrix[2][1] );
    Float3 ZVec( _TransformMatrix[0][2], _TransformMatrix[1][2], _TransformMatrix[2][2] );

    if ( _Normalized ) {
        XVec.NormalizeSelf();
        YVec.NormalizeSelf();
        ZVec.NormalizeSelf();
    }

    SetColor( FColor4( 1,0,0,1 ) );
    DrawLine( Origin, Origin + XVec );
    SetColor( FColor4( 0,1,0,1 ) );
    DrawLine( Origin, Origin + YVec );
    SetColor( FColor4( 0,0,1,1 ) );
    DrawLine( Origin, Origin + ZVec );
}

void FDebugDraw::DrawAxis( Float3 const & _Origin, Float3 const & _XVec, Float3 const & _YVec, Float3 const & _ZVec, Float3 const & _Scale ) {
    SetColor( FColor4( 1,0,0,1 ) );
    DrawLine( _Origin, _Origin + _XVec * _Scale.X );
    SetColor( FColor4( 0,1,0,1 ) );
    DrawLine( _Origin, _Origin + _YVec * _Scale.Y );
    SetColor( FColor4( 0,0,1,1 ) );
    DrawLine( _Origin, _Origin + _ZVec * _Scale.Z );
}

void FDebugDraw::DrawPlane( PlaneF const & _Plane, float _Length ) {
    DrawPlane( _Plane.Normal, _Plane.D, _Length );
}

void FDebugDraw::DrawPlane( Float3 const & _Normal, float _D, float _Length ) {
    Float3 xvec, yvec, center;

    _Normal.ComputeBasis( xvec, yvec );

    center = _Normal * _D;

    Float3 const points[4] = {
        center + ( xvec + yvec ) * _Length,
        center - ( xvec - yvec ) * _Length,
        center - ( xvec + yvec ) * _Length,
        center + ( xvec - yvec ) * _Length
    };

    DrawLine( points[0], points[2] );
    DrawLine( points[1], points[3] );
    DrawLine( points, 4, true );
}

void FDebugDraw::DrawPlaneFilled( PlaneF const & _Plane, float _Length, bool _TwoSided ) {
    DrawPlaneFilled( _Plane.Normal, _Plane.D, _Length );
}

void FDebugDraw::DrawPlaneFilled( Float3 const & _Normal, float _D, float _Length, bool _TwoSided ) {
    Float3 xvec, yvec, center;

    _Normal.ComputeBasis( xvec, yvec );

    center = _Normal * _D;

    Float3 const points[4] = {
        center + ( xvec + yvec ) * _Length,
        center - ( xvec - yvec ) * _Length,
        center - ( xvec + yvec ) * _Length,
        center + ( xvec - yvec ) * _Length
    };

    DrawConvexPoly( points, 4, _TwoSided );
}
