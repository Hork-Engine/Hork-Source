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

#include <Engine/Runtime/Public/RenderBackend.h>
#include <Engine/Runtime/Public/RuntimeVariable.h>
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>
#include <Engine/Core/Public/BV/BvOrientedBox.h>

class FDebugDraw {
    AN_FORBID_COPY( FDebugDraw )

public:
    FDebugDraw();

    ~FDebugDraw();

    void Reset();

    void SetDepthTest( bool _DepthTest );

    void SetColor( uint32_t _Color );

    void SetColor( FColor4 const & _Color );

    void SetAlpha( float _Alpha );

    void DrawPoint( Float3 const & _Position );

    void DrawPoints( Float3 const * _Points, int _NumPoints, int _Stride );

    void DrawLine( Float3 const & _P0, Float3 const & _P1 );

    void DrawDottedLine( Float3 const & _P0, Float3 const & _P1, float _Step );

    void DrawLine( Float3 const * _Points, int _NumPoints, bool _Closed = false );

    void DrawConvexPoly( Float3 const * _Points, int _NumPoints, bool _TwoSided = false );

    void DrawTriangleSoup( Float3 const * _Points, int _NumPoints, int _Stride, unsigned int const * _Indices, int _NumIndices, bool _TwoSided = false );
    void DrawTriangleSoupWireframe( Float3 const * _Points, int _Stride, unsigned int const * _Indices, int _NumIndices );

    void DrawTriangle( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2, bool _TwoSided = false );

    void DrawTriangles( Float3 const * _Triangles, int _NumTriangles, int _Stride, bool _TwoSided = false );

    void DrawBox( Float3 const & _Position, Float3 const & _HalfExtents );

    void DrawBoxFilled( Float3 const & _Position, Float3 const & _HalfExtents, bool _TwoSided = false );

    void DrawOrientedBox( Float3 const & _Position, Float3x3 const & _Orientation, Float3 const & _HalfExtents );

    void DrawOrientedBoxFilled( Float3 const & _Position, Float3x3 const & _Orientation, Float3 const & _HalfExtents, bool _TwoSided = false );

    void DrawSphere( Float3 const & _Position, float _Radius );

    void DrawOrientedSphere( Float3 const & _Position, Float3x3 const & _Orientation, float _Radius );

    // TODO: DrawSphereFilled

    void DrawSpherePatch( Float3 const & _Position, Float3 const & _Up, Float3 const & _Right, float _Radius, float _MinTh, float _MaxTh, float _MinPs, float _MaxPs, float _StepDegrees = 10.0f, bool _DrawCenter = true );

    void DrawCircle( Float3 const & _Position, Float3 const & _UpVector, const float & _Radius );

    void DrawCircleFilled( Float3 const & _Position, Float3 const & _UpVector, const float & _Radius, bool _TwoSided = false );

    void DrawCone( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _HalfAngle );

    // TODO: void DrawConeFilled( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _HalfAngle, bool _TwoSided = false );

    void DrawCylinder( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _Height );

    // TODO: void DrawCylinderFilled( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _Height, bool _TwoSided = false );

    void DrawCapsule( Float3 const & _Position, Float3x3 const & _Orientation, float _Radius, float _Height, int _UpAxis );

    // TODO: DrawCapsuleFilled

    void DrawAABB( BvAxisAlignedBox const & _AABB );

    void DrawOBB( BvOrientedBox const & _OBB );

    void DrawAxis( const Float3x4 & _TransformMatrix, bool _Normalized );

    void DrawAxis( Float3 const & _Origin, Float3 const & _XVec, Float3 const & _YVec, Float3 const & _ZVec, Float3 const & _Scale = Float3(1) );

    void DrawPlane( PlaneF const & _Plane, float _Length = 100.0f );

    void DrawPlane( Float3 const & _Normal, float _D, float _Length = 100.0f );

    void DrawPlaneFilled( PlaneF const & _Plane, float _Length = 100.0f, bool _TwoSided = false );

    void DrawPlaneFilled( Float3 const & _Normal, float _D, float _Length = 100.0f, bool _TwoSided = false );

    void SplitCommands();

    int CommandsCount() const { return Cmds->Size(); }

private:
    FDebugDrawCmd & SetDrawCmd( EDebugDrawCmd _Type );
    void PrimitiveReserve( int _NumVertices, int _NumIndices );

    FArrayOfDebugVertices * Vertices;
    FArrayOfDebugIndices * Indices;
    FArrayOfDebugDrawCmds * Cmds;
    uint32_t CurrentColor;
    int FirstVertex;
    int FirstIndex;
    bool bDepthTest;
    bool bSplit;
};

extern FRuntimeVariable RVDrawCollisionModel;
extern FRuntimeVariable RVDrawCollisionBounds;
extern FRuntimeVariable RVDrawCollisionShapeWireframe;
extern FRuntimeVariable RVDrawCenterOfMass;
extern FRuntimeVariable RVDrawContactPoints;
extern FRuntimeVariable RVDrawConstraints;
extern FRuntimeVariable RVDrawConstraintLimits;
extern FRuntimeVariable RVDrawNavMeshBVTree;
extern FRuntimeVariable RVDrawNavMeshNodes;
extern FRuntimeVariable RVDrawNavMeshWithClosedList;
extern FRuntimeVariable RVDrawNavMeshTileBounds;
extern FRuntimeVariable RVDrawCameraFrustum;
extern FRuntimeVariable RVDrawLevelAreaBounds;
extern FRuntimeVariable RVDrawLevelIndoorBounds;
extern FRuntimeVariable RVDrawLevelPortals;
extern FRuntimeVariable RVDrawSkeleton;
extern FRuntimeVariable RVDrawSkeletonSockets;
extern FRuntimeVariable RVDrawSoftmeshFaces;
extern FRuntimeVariable RVDrawMeshBounds;
extern FRuntimeVariable RVDrawRootComponentAxis;
extern FRuntimeVariable RVDrawLights;
