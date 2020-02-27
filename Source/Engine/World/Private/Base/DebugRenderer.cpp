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

#include <World/Public/Base/DebugRenderer.h>
#include <Core/Public/Color.h>
#include <Core/Public/CriticalError.h>
#include <Runtime/Public/Runtime.h>

//#define PRIMITIVE_RESTART_INDEX    0xffffffff
#define PRIMITIVE_RESTART_INDEX    0xffff
#define MAX_PRIMITIVE_VERTS        0xfffe

ADebugRenderer::ADebugRenderer() {
}

ADebugRenderer::~ADebugRenderer() {
}

void ADebugRenderer::Free() {
    Reset();

    Vertices.Free();
    Indices.Free();
    Cmds.Free();
}

void ADebugRenderer::Reset() {
    CurrentColor = 0xffffffff;
    bDepthTest = false;

    Vertices.Clear();
    Indices.Clear();
    Cmds.Clear();

    FirstVertex = 0;
    FirstIndex = 0;
    bSplit = false;

    pView = nullptr;
}

void ADebugRenderer::BeginRenderView( SRenderView * InView, int InVisPass ) {
    AN_ASSERT( !pView );

    pView = InView;
    pView->FirstDebugDrawCommand = CommandsCount();
    pView->DebugDrawCommandCount = 0;
    VisPass = InVisPass;
    SplitCommands();
}

void ADebugRenderer::EndRenderView() {
    AN_ASSERT( pView );

    pView->DebugDrawCommandCount = CommandsCount() - pView->FirstDebugDrawCommand;
    pView = nullptr;
}

void ADebugRenderer::SetDepthTest( bool _DepthTest ) {
    bDepthTest = _DepthTest;
}

void ADebugRenderer::SetColor( uint32_t _Color ) {
    CurrentColor = _Color;
}

void ADebugRenderer::SetColor( AColor4 const & _Color ) {
    CurrentColor = _Color.GetDWord();
}

void ADebugRenderer::SetAlpha( float _Alpha ) {
    CurrentColor &= 0x00ffffff;
    CurrentColor |= Math::Clamp( Math::ToIntFast( _Alpha * 255 ), 0, 255 ) << 24;
}

void ADebugRenderer::SplitCommands() {
    bSplit = true;
}

void ADebugRenderer::PrimitiveReserve( EDebugDrawCmd _CmdName, int _NumVertices, int _NumIndices, SDebugDrawCmd ** _Cmd, SDebugVertex ** _Verts, unsigned short ** _Indices ) {
    if ( _NumVertices > MAX_PRIMITIVE_VERTS ) {
        // TODO: split to several primitives
        CriticalError( "ADebugRenderer::PrimitiveReserve: primitive has too many vertices\n" );
    }

    if ( !Cmds.IsEmpty() ) {
        SDebugDrawCmd & cmd = Cmds.Last();

        if ( cmd.NumVertices + _NumVertices > MAX_PRIMITIVE_VERTS ) {
            SplitCommands();
        }
    }

    Vertices.Resize( Vertices.Size() + _NumVertices );
    Indices.Resize( Indices.Size() + _NumIndices );

    *_Verts = Vertices.ToPtr() + FirstVertex;
    *_Indices = Indices.ToPtr() + FirstIndex;

    if ( Cmds.IsEmpty() || bSplit ) {
        // Create first cmd or split
        SDebugDrawCmd & cmd = Cmds.Append();
        cmd.Type = _CmdName;
        cmd.FirstVertex = FirstVertex;
        cmd.FirstIndex = FirstIndex;
        cmd.NumVertices = 0;
        cmd.NumIndices = 0;
        bSplit = false;
    } else if ( Cmds.Last().NumIndices == 0 ) {
        // If last cmd has no indices, use it
        SDebugDrawCmd & cmd = Cmds.Last();
        cmd.Type = _CmdName;
        cmd.FirstVertex = FirstVertex;
        cmd.FirstIndex = FirstIndex;
        cmd.NumVertices = 0;
    } else if ( Cmds.Last().Type != _CmdName ) {
        // If last cmd has other type, create a new cmd
        SDebugDrawCmd & cmd = Cmds.Append();
        cmd.Type = _CmdName;
        cmd.FirstVertex = FirstVertex;
        cmd.FirstIndex = FirstIndex;
        cmd.NumVertices = 0;
        cmd.NumIndices = 0;
    }

    *_Cmd = &Cmds.Last();

    FirstVertex += _NumVertices;
    FirstIndex += _NumIndices;
}

void ADebugRenderer::DrawPoint( Float3 const & _Position ) {
    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, 1, 1, &cmd, &verts, &indices );

    verts->Position = _Position;
    verts->Color = CurrentColor;

    indices[0] = cmd->NumVertices;

    cmd->NumVertices++;
    cmd->NumIndices++;
}

void ADebugRenderer::DrawPoints( Float3 const * _Points, int _NumPoints, int _Stride ) {
    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, _NumPoints, _NumPoints, &cmd, &verts, &indices );

    byte * pPoints = ( byte * )_Points;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++, indices++ ) {
        verts->Position = *( Float3 * )pPoints;
        verts->Color = CurrentColor;
        *indices = cmd->NumVertices + i;

        pPoints += _Stride;
    }

    cmd->NumVertices += _NumPoints;
    cmd->NumIndices += _NumPoints;
}

void ADebugRenderer::DrawLine( Float3 const & _P0, Float3 const & _P1 ) {
    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, 2, 3, &cmd, &verts, &indices );

    verts[0].Position = _P0;
    verts[0].Color = CurrentColor;
    verts[1].Position = _P1;
    verts[1].Color = CurrentColor;

    indices[0] = cmd->NumVertices;
    indices[1] = cmd->NumVertices + 1;
    indices[2] = PRIMITIVE_RESTART_INDEX;

    cmd->NumVertices += 2;
    cmd->NumIndices += 3;
}

void ADebugRenderer::DrawDottedLine( Float3 const & _P0, Float3 const & _P1, float _Step ) {
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

void ADebugRenderer::DrawLine( Float3 const * _Points, int _NumPoints, bool _Closed ) {
    if ( _NumPoints < 2 ) {
        return;
    }

    int numIndices = _Closed ? _NumPoints + 2 : _NumPoints + 1;

    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, _NumPoints, numIndices, &cmd, &verts, &indices );

    for ( int i = 0 ; i < _NumPoints ; i++, verts++, indices++ ) {
        verts->Position = _Points[i];
        verts->Color = CurrentColor;
        *indices = cmd->NumVertices + i;
    }

    if ( _Closed ) {
        *indices++ = cmd->NumVertices;
    }

    *indices = PRIMITIVE_RESTART_INDEX;

    cmd->NumVertices += _NumPoints;
    cmd->NumIndices += numIndices;
}

void ADebugRenderer::DrawConvexPoly( Float3 const * _Points, int _NumPoints, bool _TwoSided ) {
    if ( _NumPoints < 3 ) {
        return;
    }

    int numTriangles = _NumPoints - 2;
    int numIndices = numTriangles * 3;

    if ( _TwoSided ) {
        numIndices <<= 1;
    }

    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, _NumPoints, numIndices, &cmd, &verts, &indices );

    for ( int i = 0 ; i < _NumPoints ; i++, verts++ ) {
        verts->Position = _Points[i];
        verts->Color = CurrentColor;
    }

    for ( int i = 0 ; i < numTriangles ; i++ ) {
        *indices++ = cmd->NumVertices + 0;
        *indices++ = cmd->NumVertices + i + 1;
        *indices++ = cmd->NumVertices + i + 2;
    }

    if ( _TwoSided ) {
        for ( int i = numTriangles-1 ; i >= 0 ; i-- ) {
            *indices++ = cmd->NumVertices + 0;
            *indices++ = cmd->NumVertices + i + 2;
            *indices++ = cmd->NumVertices + i + 1;
        }
    }

    cmd->NumVertices += _NumPoints;
    cmd->NumIndices += numIndices;
}

void ADebugRenderer::DrawTriangleSoup( Float3 const * _Points, int _NumPoints, int _Stride, unsigned int const * _Indices, int _NumIndices, bool _TwoSided ) {
    int numIndices = _NumIndices;

    if ( _TwoSided ) {
        numIndices <<= 1;
    }

    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, _NumPoints, numIndices, &cmd, &verts, &indices );

    byte * pPoints = ( byte * )_Points;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++ ) {
        verts->Position = *( Float3 * )pPoints;
        verts->Color = CurrentColor;

//#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
        verts->Color = AColor4( Float4(i%255,i%255,i%255,255)/255.0f ).GetDWord();
#endif

        pPoints += _Stride;
    }

    for ( int i = 0 ; i < _NumIndices ; i++ ) {
        *indices++ = cmd->NumVertices + _Indices[i];
    }

    if ( _TwoSided ) {
        _Indices += _NumIndices;
        for ( int i = _NumIndices-1 ; i >= 0 ; i-- ) {
            *indices++ = cmd->NumVertices + *--_Indices;
        }
    }

    cmd->NumVertices += _NumPoints;
    cmd->NumIndices += numIndices;
}

void ADebugRenderer::DrawTriangleSoup( Float3 const * _Points, int _NumPoints, int _Stride, unsigned short const * _Indices, int _NumIndices, bool _TwoSided ) {
    int numIndices = _NumIndices;

    if ( _TwoSided ) {
        numIndices <<= 1;
    }

    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, _NumPoints, numIndices, &cmd, &verts, &indices );

    byte * pPoints = (byte *)_Points;

    for ( int i = 0 ; i < _NumPoints ; i++, verts++ ) {
        verts->Position = *(Float3 *)pPoints;
        verts->Color = CurrentColor;

        //#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
        verts->Color = AColor4( Float4( i%255, i%255, i%255, 255 )/255.0f ).GetDWord();
#endif

        pPoints += _Stride;
    }

    for ( int i = 0 ; i < _NumIndices ; i++ ) {
        *indices++ = cmd->NumVertices + _Indices[i];
    }

    if ( _TwoSided ) {
        _Indices += _NumIndices;
        for ( int i = _NumIndices-1 ; i >= 0 ; i-- ) {
            *indices++ = cmd->NumVertices + *--_Indices;
        }
    }

    cmd->NumVertices += _NumPoints;
    cmd->NumIndices += numIndices;
}

void ADebugRenderer::DrawTriangleSoupWireframe( Float3 const * _Points, int _Stride, unsigned int const * _Indices, int _NumIndices ) {
    Float3 points[3];
    byte * pPoints = ( byte * )_Points;
    for ( int i = 0 ; i < _NumIndices ; i+=3 ) {
        points[0] = *(Float3 *)(pPoints + _Stride * _Indices[i+0]);
        points[1] = *(Float3 *)(pPoints + _Stride * _Indices[i+1]);
        points[2] = *(Float3 *)(pPoints + _Stride * _Indices[i+2]);
        DrawLine( points, 3, true );
    }
}

void ADebugRenderer::DrawTriangleSoupWireframe( Float3 const * _Points, int _Stride, unsigned short const * _Indices, int _NumIndices ) {
    Float3 points[3];
    byte * pPoints = (byte *)_Points;
    for ( int i = 0 ; i < _NumIndices ; i += 3 ) {
        points[0] = *(Float3 *)(pPoints + _Stride * _Indices[i+0]);
        points[1] = *(Float3 *)(pPoints + _Stride * _Indices[i+1]);
        points[2] = *(Float3 *)(pPoints + _Stride * _Indices[i+2]);
        DrawLine( points, 3, true );
    }
}

void ADebugRenderer::DrawTriangle( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2, bool _TwoSided ) {
    Float3 const Points[] = { _P0, _P1, _P2 };
    DrawConvexPoly( Points, 3, _TwoSided );
}

void ADebugRenderer::DrawTriangles( Float3 const * _Triangles, int _NumTriangles, int _Stride, bool _TwoSided ) {
    int numPoints = _NumTriangles * 3;
    int numIndices = _NumTriangles * 3;
    int totalIndices = _TwoSided ? numIndices << 1 : numIndices;
    EDebugDrawCmd cmdName = bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    SDebugDrawCmd * cmd;
    SDebugVertex * verts;
    unsigned short * indices;

    PrimitiveReserve( cmdName, numPoints, totalIndices, &cmd, &verts, &indices );

    byte * pPoints = ( byte * )_Triangles;

    for ( int i = 0 ; i < numPoints ; i++, verts++ ) {
        verts->Position = *( Float3 * )pPoints;
        verts->Color = CurrentColor;

        pPoints += _Stride;
    }

    for ( int i = 0 ; i < numIndices ; i++ ) {
        *indices++ = cmd->NumVertices + i;
    }

    if ( _TwoSided ) {
        for ( int i = numIndices-1 ; i >= 0 ; i-- ) {
            *indices++ = cmd->NumVertices + i;
        }
    }

    cmd->NumVertices += numPoints;
    cmd->NumIndices += totalIndices;
}

void ADebugRenderer::DrawBox( Float3 const & _Position, Float3 const & _HalfExtents ) {
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

void ADebugRenderer::DrawBoxFilled( Float3 const & _Position, Float3 const & _HalfExtents, bool _TwoSided ) {
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

    unsigned short const indices[36] = { 0,3,2, 2,1,0, 7,4,5, 5,6,7, 3,7,6, 6,2,3, 2,6,5, 5,1,2, 1,5,4, 4,0,1, 0,4,7, 7,3,0 };

    DrawTriangleSoup( points, 8, sizeof( Float3 ), indices, 36, _TwoSided );
}

void ADebugRenderer::DrawOrientedBox( Float3 const & _Position, Float3x3 const & _Orientation, Float3 const & _HalfExtents ) {
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

void ADebugRenderer::DrawOrientedBoxFilled( Float3 const & _Position, Float3x3 const & _Orientation, Float3 const & _HalfExtents, bool _TwoSided ) {
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

    unsigned short const indices[36] = { 0,3,2, 2,1,0, 7,4,5, 5,6,7, 3,7,6, 6,2,3, 2,6,5, 5,1,2, 1,5,4, 4,0,1, 0,4,7, 7,3,0 };

    DrawTriangleSoup( points, 8, sizeof( Float3 ), indices, 36, _TwoSided );
}

void ADebugRenderer::DrawSphere( Float3 const & _Position, float _Radius ) {
    DrawOrientedSphere( _Position, Float3x3::Identity(), _Radius );
}

void ADebugRenderer::DrawOrientedSphere( Float3 const & _Position, Float3x3 const & _Orientation, float _Radius ) {
    const float stepDegrees = 30.0f;
    DrawSpherePatch( _Position, _Orientation[1], _Orientation[0], _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false );
    DrawSpherePatch( _Position, _Orientation[1], -_Orientation[0], _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false );
}

void ADebugRenderer::DrawSpherePatch( Float3 const & _Position, Float3 const & _Up, Float3 const & _Right, float _Radius,
                                  float _MinTh, float _MaxTh, float _MinPs, float _MaxPs, float _StepDegrees, bool _DrawCenter ) {
    // This function is based on code from btDebugDraw
    Float3 vA[ 74 ];
    Float3 vB[ 74 ];
    Float3 *pvA = vA, *pvB = vB, *pT;
    Float3 npole = _Position + _Up * _Radius;
    Float3 spole = _Position - _Up * _Radius;
    Float3 arcStart;
    float step = Math::Radians( _StepDegrees );
    Float3 backVec = Math::Cross( _Up, _Right );
    bool drawN = false;
    bool drawS = false;
    if ( _MinTh <= -Math::_HALF_PI ) {
        _MinTh = -Math::_HALF_PI + step;
        drawN = true;
    }
    if ( _MaxTh >= Math::_HALF_PI ) {
        _MaxTh = Math::_HALF_PI - step;
        drawS = true;
    }
    if ( _MinTh > _MaxTh ) {
        _MinTh = -Math::_HALF_PI + step;
        _MaxTh = Math::_HALF_PI - step;
        drawN = drawS = true;
    }
    int n_hor = ( int )( ( _MaxTh - _MinTh ) / step ) + 1;
    if ( n_hor < 2 ) n_hor = 2;
    float step_h = ( _MaxTh - _MinTh ) / float( n_hor - 1 );
    bool isClosed = false;
    if ( _MinPs > _MaxPs ) {
        _MinPs = -Math::_PI + step;
        _MaxPs = Math::_PI;
        isClosed = true;
    } else if ( _MaxPs - _MinPs >= Math::_2PI ) {
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
        Math::SinCos( th, sth, cth );
        sth *= _Radius;
        cth *= _Radius;
        for ( int j = 0; j < n_vert; j++ ) {
            float psi = _MinPs + float( j ) * step_v;
            float sps;
            float cps;
            Math::SinCos( psi, sps, cps );
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

void ADebugRenderer::DrawCircle( Float3 const & _Position, Float3 const & _UpVector, const float & _Radius ) {
    const int NumCirclePoints = 32;

    const Float3 v = _UpVector.Perpendicular() * _Radius;

    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i] = _Position + Float3x3::RotationAroundNormal( Angle, _UpVector ) * v;
    }

    DrawLine( points, NumCirclePoints, true );
}

void ADebugRenderer::DrawCircleFilled( Float3 const & _Position, Float3 const & _UpVector, const float & _Radius, bool _TwoSided ) {
    const int NumCirclePoints = 32;

    const Float3 v = _UpVector.Perpendicular() * _Radius;

    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i] = _Position + Float3x3::RotationAroundNormal( Angle, _UpVector ) * v;
    }

    DrawConvexPoly( points, NumCirclePoints, _TwoSided );
}

void ADebugRenderer::DrawCone( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _HalfAngle ) {
    const Float3 coneDirection = _Orientation[2];
    const Float3 v = Float3x3::RotationAroundNormal( _HalfAngle,  _Orientation[0] ) * coneDirection * _Radius;

    const int NumCirclePoints = 32;
    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i] = _Position + Float3x3::RotationAroundNormal( Angle, coneDirection ) * v;
    }

    // Draw cone circle
    DrawLine( points, NumCirclePoints, true );

    // Draw cone faces (rays)
    for ( int i = 0 ; i < NumCirclePoints ; i+=2 ) {
        DrawLine( _Position, points[i] );
    }
}

void ADebugRenderer::DrawCylinder( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _Height ) {
    const Float3 upVector = _Orientation[1] * _Height;
    const Float3 v = _Orientation[0] * _Radius;
    const Float3 pos = _Position - _Orientation[1] * ( _Height * 0.5f );

    const int NumCirclePoints = 32;
    Float3 points[NumCirclePoints];

    points[0] = pos + v;
    for ( int i = 1 ; i < NumCirclePoints ; ++i ) {
        const float Angle = Math::_2PI / NumCirclePoints * i;
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

void ADebugRenderer::DrawCapsule( Float3 const & _Position, Float3x3 const & _Orientation, float _Radius, float _Height, int _UpAxis ) {
    AN_ASSERT( _UpAxis >= 0 && _UpAxis < 3 );

    const int stepDegrees = 30;
    const float halfHeight = _Height * 0.5f;

    Float3 capStart( 0.0f );
    capStart[ _UpAxis ] = -halfHeight;

    Float3 capEnd( 0.0f );
    capEnd[ _UpAxis ] = halfHeight;

    Float3 up = _Orientation.GetRow( ( _UpAxis + 1 ) % 3 );
    Float3 axis = _Orientation.GetRow( _UpAxis );

    DrawSpherePatch( _Orientation * capStart + _Position, up, -axis, _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false );
    DrawSpherePatch( _Orientation * capEnd + _Position, up, axis, _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false );

    for ( int angle = 0; angle < 360; angle += stepDegrees ) {
        float r = Math::Radians( float( angle ) );
        capEnd[ ( _UpAxis + 1 ) % 3 ] = capStart[ ( _UpAxis + 1 ) % 3 ] = Math::Sin( r ) * _Radius;
        capEnd[ ( _UpAxis + 2 ) % 3 ] = capStart[ ( _UpAxis + 2 ) % 3 ] = Math::Cos( r ) * _Radius;
        DrawLine( _Position + _Orientation * capStart, _Position + _Orientation * capEnd );
    }
}

void ADebugRenderer::DrawAABB( BvAxisAlignedBox const & _AABB ) {
    DrawBox( _AABB.Center(), _AABB.HalfSize() );
}

void ADebugRenderer::DrawOBB( BvOrientedBox const & _OBB ) {
    DrawOrientedBox( _OBB.Center, _OBB.Orient, _OBB.HalfSize );
}

void ADebugRenderer::DrawAxis( Float3x4 const & _TransformMatrix, bool _Normalized ) {
    Float3 Origin( _TransformMatrix[0][3], _TransformMatrix[1][3], _TransformMatrix[2][3] );
    Float3 XVec( _TransformMatrix[0][0], _TransformMatrix[1][0], _TransformMatrix[2][0] );
    Float3 YVec( _TransformMatrix[0][1], _TransformMatrix[1][1], _TransformMatrix[2][1] );
    Float3 ZVec( _TransformMatrix[0][2], _TransformMatrix[1][2], _TransformMatrix[2][2] );

    if ( _Normalized ) {
        XVec.NormalizeSelf();
        YVec.NormalizeSelf();
        ZVec.NormalizeSelf();
    }

    SetColor( AColor4( 1,0,0,1 ) );
    DrawLine( Origin, Origin + XVec );
    SetColor( AColor4( 0,1,0,1 ) );
    DrawLine( Origin, Origin + YVec );
    SetColor( AColor4( 0,0,1,1 ) );
    DrawLine( Origin, Origin + ZVec );
}

void ADebugRenderer::DrawAxis( Float3 const & _Origin, Float3 const & _XVec, Float3 const & _YVec, Float3 const & _ZVec, Float3 const & _Scale ) {
    SetColor( AColor4( 1,0,0,1 ) );
    DrawLine( _Origin, _Origin + _XVec * _Scale.X );
    SetColor( AColor4( 0,1,0,1 ) );
    DrawLine( _Origin, _Origin + _YVec * _Scale.Y );
    SetColor( AColor4( 0,0,1,1 ) );
    DrawLine( _Origin, _Origin + _ZVec * _Scale.Z );
}

void ADebugRenderer::DrawPlane( PlaneF const & _Plane, float _Length ) {
    DrawPlane( _Plane.Normal, _Plane.D, _Length );
}

void ADebugRenderer::DrawPlane( Float3 const & _Normal, float _D, float _Length ) {
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

void ADebugRenderer::DrawPlaneFilled( PlaneF const & _Plane, float _Length, bool _TwoSided ) {
    DrawPlaneFilled( _Plane.Normal, _Plane.D, _Length );
}

void ADebugRenderer::DrawPlaneFilled( Float3 const & _Normal, float _D, float _Length, bool _TwoSided ) {
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
