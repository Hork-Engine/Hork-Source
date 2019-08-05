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

#include <Engine/World/Public/Components/CameraComponent.h>
#include <Engine/World/Public/DebugDraw.h>

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/ConvexHull.h>

#include <Engine/Runtime/Public/Runtime.h>

#define DEFAULT_PROJECTION FCameraComponent::PERSPECTIVE
#define DEFAULT_ZNEAR 0.04f//0.1f
#define DEFAULT_ZFAR 99999.0f
#define DEFAULT_FOVX 90.0f
#define DEFAULT_FOVY 90.0f
#define DEFAULT_ASPECT_RATIO (4.0f / 3.0f)
#define DEFAULT_PERSPECTIVE_ADJUST FCameraComponent::ADJ_FOV_X_ASPECT_RATIO
#define DEFAULT_ORTHO_RECT Float4(-1,1,-1,1)

AN_CLASS_META_NO_ATTRIBS( FCameraComponent )

#if 0
AN_SCENE_COMPONENT_BEGIN_DECL( FCameraComponent, CCF_DEFAULT )

AN_ATTRIBUTE( "Projection", FProperty( DEFAULT_PROJECTION ), SetProjection, GetProjection, "Ortho\0Perspective\0\0Camera projection type", AF_ENUM )
AN_ATTRIBUTE( "ZNear", FProperty( DEFAULT_ZNEAR ), SetZNear, GetZNear, "Camera near plane", AF_DEFAULT )
AN_ATTRIBUTE( "ZFar", FProperty( DEFAULT_ZFAR ), SetZFar, GetZFar, "Camera far plane", AF_DEFAULT )
AN_ATTRIBUTE( "FovX", FProperty( DEFAULT_FOVX ), SetFovX, GetFovX, "Camera fov X", AF_DEFAULT )
AN_ATTRIBUTE( "FovY", FProperty( DEFAULT_FOVY ), SetFovY, GetFovY, "Camera fov Y", AF_DEFAULT )
AN_ATTRIBUTE( "Aspect Ratio", FProperty( DEFAULT_ASPECT_RATIO ), SetAspectRatio, GetAspectRatio, "Camera perspective aspect ratio. E.g. 4/3, 16/9", AF_DEFAULT )
AN_ATTRIBUTE( "Perspective Adjust", FProperty( DEFAULT_PERSPECTIVE_ADJUST ), SetPerspectiveAdjust, GetPerspectiveAdjust, "FovX / AspectRatio\0FovX / FovY\0\0Perspective projection adjust", AF_ENUM )
AN_ATTRIBUTE( "OrthoRect", FProperty( DEFAULT_ORTHO_RECT ), SetOrthoRect, GetOrthoRect, "Camera ortho rect", AF_DEFAULT )

AN_SCENE_COMPONENT_END_DECL
#endif

FCameraComponent::FCameraComponent() {
    Projection = DEFAULT_PROJECTION;
    ZNear = DEFAULT_ZNEAR;
    ZFar = DEFAULT_ZFAR;
    FovX = DEFAULT_FOVX;
    FovY = DEFAULT_FOVY;
    AspectRatio = DEFAULT_ASPECT_RATIO;
    //Width = 512;
    //Height = 512;
    Adjust = DEFAULT_PERSPECTIVE_ADJUST;
    OrthoRect = DEFAULT_ORTHO_RECT;
    ViewMatrixDirty = true;
    FrustumDirty = true;
    ProjectionDirty = true;
}

void FCameraComponent::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    if ( GDebugDrawFlags.bDrawCameraFrustum ) {
        Float3 vectorTR;
        Float3 vectorTL;
        Float3 vectorBR;
        Float3 vectorBL;
        Float3 origin = GetWorldPosition();
        Float3 v[4];
        Float3 faces[4][3];
        float rayLength = 32;

        Frustum.CornerVector_TR( vectorTR );
        Frustum.CornerVector_TL( vectorTL );
        Frustum.CornerVector_BR( vectorBR );
        Frustum.CornerVector_BL( vectorBL );

        v[0] = origin + vectorTR * rayLength;
        v[1] = origin + vectorBR * rayLength;
        v[2] = origin + vectorBL * rayLength;
        v[3] = origin + vectorTL * rayLength;      
        
        // top
        faces[0][0] = origin;
        faces[0][1] = v[0];
        faces[0][2] = v[3];

        // left
        faces[1][0] = origin;
        faces[1][1] = v[3];
        faces[1][2] = v[2];

        // bottom
        faces[2][0] = origin;
        faces[2][1] = v[2];
        faces[2][2] = v[1];

        // right
        faces[3][0] = origin;
        faces[3][1] = v[1];
        faces[3][2] = v[0];

        _DebugDraw->SetDepthTest( true );

        _DebugDraw->SetColor( 0, 1, 1, 1 );
        _DebugDraw->DrawLine( origin, v[0] );
        _DebugDraw->DrawLine( origin, v[3] );
        _DebugDraw->DrawLine( origin, v[1] );
        _DebugDraw->DrawLine( origin, v[2] );
        _DebugDraw->DrawLine( v, 4, true );

        _DebugDraw->SetColor( 1, 1, 1, 0.3f );
        _DebugDraw->DrawTriangles( &faces[0][0], 4, sizeof( Float3 ), false );
        _DebugDraw->DrawConvexPoly( v, 4, false );
    }
}

void FCameraComponent::SetProjection( int _Projection ) {
    if ( Projection == _Projection ) {
        return;
    }
    Projection = _Projection;
    ProjectionDirty = true;
}

void FCameraComponent::SetZNear( float _ZNear ) {
    ZNear = _ZNear;
    ProjectionDirty = true;
}

void FCameraComponent::SetZFar( float _ZFar ) {
    ZFar = _ZFar;
    ProjectionDirty = true;
}

void FCameraComponent::SetFovX( float _FieldOfView ) {
    FovX = _FieldOfView;
    ProjectionDirty = true;
}

void FCameraComponent::SetFovY( float _FieldOfView ) {
    FovY = _FieldOfView;
    ProjectionDirty = true;
}

void FCameraComponent::SetAspectRatio( float _AspectRatio ) {
    AspectRatio = _AspectRatio;
    ProjectionDirty = true;
}

void FCameraComponent::SetMonitorAspectRatio( const FPhysicalMonitor * _Monitor ) {
    const float MonitorAspectRatio = ( float )_Monitor->PhysicalWidthMM / _Monitor->PhysicalHeightMM;
    SetAspectRatio( MonitorAspectRatio );
}

//void FCameraComponent::SetWindowAspectRatio( const FVirtualDisplay * _Window ) {
//    const int w = _Window->GetWidth();
//    const int h = _Window->GetHeight();
//    const float WindowAspectRatio = h > 0 ? ( float )w / h : DEFAULT_ASPECT_RATIO;
//    SetAspectRatio( WindowAspectRatio );
//}

void FCameraComponent::SetPerspectiveAdjust( int _Adjust ) {
    if ( Adjust != _Adjust ) {
        Adjust = _Adjust;
        ProjectionDirty = true;
    }
}

void FCameraComponent::GetEffectiveFov( float & _FovX, float & _FovY ) const {
    _FovX = FMath::Radians( FovX );

    switch ( Adjust ) {
        //case ADJ_FOV_X_VIEW_SIZE:
        //{
        //    const float TanHalfFovX = tan( _FovX * 0.5f );
        //    _FovY = atan2( static_cast< float >( Height ), Width / TanHalfFovX ) * 2.0f;
        //    break;
        //}
        case ADJ_FOV_X_ASPECT_RATIO:
        {
            const float TanHalfFovX = tan( _FovX * 0.5f );

            _FovY = atan2( 1.0f, AspectRatio / TanHalfFovX ) * 2.0f;
            break;
        }
        case ADJ_FOV_X_FOV_Y:
        {
            _FovY = FMath::Radians( FovY );
            break;
        }
    }
}

void FCameraComponent::SetOrthoRect( const Float4 & _OrthoRect ) {
    OrthoRect = _OrthoRect;
    ProjectionDirty = true;
}

void FCameraComponent::SetOrthoRect( float _Left, float _Right, float _Bottom, float _Top ) {
    OrthoRect.X = _Left;
    OrthoRect.Y = _Right;
    OrthoRect.Z = _Bottom;
    OrthoRect.W = _Top;
    ProjectionDirty = true;
}

void FCameraComponent::ComputeRect( float _OrthoZoom, float * _Left, float * _Right, float * _Bottom, float * _Top ) const {
    float Z = 1.0f / _OrthoZoom;
    if ( _Left ) {
        *_Left = -Z;
    }
    if ( _Right ) {
        *_Right = Z;
    }
    if ( _Bottom ) {
        *_Bottom = -Z / AspectRatio;
    }
    if ( _Top ) {
        *_Top = Z / AspectRatio;
    }
}

void FCameraComponent::OnTransformDirty() {
    ViewMatrixDirty = true;
}

void FCameraComponent::ComputeClusterProjectionMatrix( Float4x4 & _Matrix, const float _ClusterZNear, const float _ClusterZFar ) const {
    // TODO: if ( ClusterProjectionDirty ...
    if ( Projection == PERSPECTIVE ) {
        switch ( Adjust ) {
            //case ADJ_FOV_X_VIEW_SIZE:
            //{
            //    _Matrix = Float4x4::PerspectiveRevCC( FMath::Radians( FovX ), float( Width ), float( Height ), _ClusterZNear, _ClusterZFar );
            //    break;
            //}
            case ADJ_FOV_X_ASPECT_RATIO:
            {
                _Matrix = Float4x4::PerspectiveRevCC( FMath::Radians( FovX ), AspectRatio, 1.0f, _ClusterZNear, _ClusterZFar );
                break;
            }
            case ADJ_FOV_X_FOV_Y:
            {
                _Matrix = Float4x4::PerspectiveRevCC( FMath::Radians( FovX ), FMath::Radians( FovY ), _ClusterZNear, _ClusterZFar );
                break;
            }
        }
    } else {
        _Matrix = Float4x4::OrthoRevCC( OrthoRect.X, OrthoRect.Y, OrthoRect.Z, OrthoRect.W, _ClusterZNear, _ClusterZFar );
    }
}

Float4x4 const & FCameraComponent::GetProjectionMatrix() const {

    if ( ProjectionDirty ) {

        if ( Projection == PERSPECTIVE ) {
            switch ( Adjust ) {
                //case ADJ_FOV_X_VIEW_SIZE:
                //{
                //    ProjectionMatrix = Float4x4::PerspectiveRevCC( FMath::Radians( FovX ), float( Width ), float( Height ), ZNear, ZFar );
                //    break;
                //}
                case ADJ_FOV_X_ASPECT_RATIO:
                {
                    ProjectionMatrix = Float4x4::PerspectiveRevCC( FMath::Radians( FovX ), AspectRatio, 1.0f, ZNear, ZFar );
                    break;
                }
                case ADJ_FOV_X_FOV_Y:
                {
                    ProjectionMatrix = Float4x4::PerspectiveRevCC( FMath::Radians( FovX ), FMath::Radians( FovY ), ZNear, ZFar );
                    break;
                }
            }
        } else {
            ProjectionMatrix = Float4x4::OrthoRevCC( OrthoRect.X, OrthoRect.Y, OrthoRect.Z, OrthoRect.W, ZNear, ZFar );
        }

        //Float4 v1 = ProjectionMatrix * Float4(0,0,-ZFar,1.0f);
        //v1/=v1.W;
        //Float4 v2 = ProjectionMatrix * Float4(0,0,-ZNear,1.0f);
        //v2/=v2.W;
        //Out() << v1<<v2;

        ProjectionDirty = false;
        FrustumDirty = true;
    }

    return ProjectionMatrix;
}

SegmentF FCameraComponent::GetRay( float _NormalizedX, float _NormalizedY ) const {
    SegmentF RaySegment;

    // Update projection matrix
    GetProjectionMatrix();

    // Update view matrix
    GetViewMatrix();

    Float4x4 ModelViewProjectionInversed = ( ProjectionMatrix * ViewMatrix ).Inversed();

    float x = 2.0f * _NormalizedX - 1.0f;
    float y = 2.0f * _NormalizedY - 1.0f;

#if 0
    Float4 near(x, y, 1, 1.0f);
    Float4 far(x, y, 0, 1.0f);
    RaySegment.Start.X = ModelViewProjectionInversed[0][0] * near[0] + ModelViewProjectionInversed[1][0] * near[1] + ModelViewProjectionInversed[2][0] * near[2] + ModelViewProjectionInversed[3][0];
    RaySegment.Start.Y = ModelViewProjectionInversed[0][1] * near[0] + ModelViewProjectionInversed[1][1] * near[1] + ModelViewProjectionInversed[2][1] * near[2] + ModelViewProjectionInversed[3][1];
    RaySegment.Start.Z = ModelViewProjectionInversed[0][2] * near[0] + ModelViewProjectionInversed[1][2] * near[1] + ModelViewProjectionInversed[2][2] * near[2] + ModelViewProjectionInversed[3][2];
    float div          = ModelViewProjectionInversed[0][3] * near[0] + ModelViewProjectionInversed[1][3] * near[1] + ModelViewProjectionInversed[2][3] * near[2] + ModelViewProjectionInversed[3][3];
    RaySegment.Start/=div;
    RaySegment.End.X = ModelViewProjectionInversed[0][0] * far[0] + ModelViewProjectionInversed[1][0] * far[1] + ModelViewProjectionInversed[2][0] * far[2] + ModelViewProjectionInversed[3][0];
    RaySegment.End.Y = ModelViewProjectionInversed[0][1] * far[0] + ModelViewProjectionInversed[1][1] * far[1] + ModelViewProjectionInversed[2][1] * far[2] + ModelViewProjectionInversed[3][1];
    RaySegment.End.Z = ModelViewProjectionInversed[0][2] * far[0] + ModelViewProjectionInversed[1][2] * far[1] + ModelViewProjectionInversed[2][2] * far[2] + ModelViewProjectionInversed[3][2];
    div              = ModelViewProjectionInversed[0][3] * far[0] + ModelViewProjectionInversed[1][3] * far[1] + ModelViewProjectionInversed[2][3] * far[2] + ModelViewProjectionInversed[3][3];
    RaySegment.End/=div;
#else
    // Same code
    RaySegment.End.X = ModelViewProjectionInversed[0][0] * x + ModelViewProjectionInversed[1][0] * y + ModelViewProjectionInversed[3][0];
    RaySegment.End.Y = ModelViewProjectionInversed[0][1] * x + ModelViewProjectionInversed[1][1] * y + ModelViewProjectionInversed[3][1];
    RaySegment.End.Z = ModelViewProjectionInversed[0][2] * x + ModelViewProjectionInversed[1][2] * y + ModelViewProjectionInversed[3][2];
    RaySegment.Start.X = RaySegment.End.X + ModelViewProjectionInversed[2][0];
    RaySegment.Start.Y = RaySegment.End.Y + ModelViewProjectionInversed[2][1];
    RaySegment.Start.Z = RaySegment.End.Z + ModelViewProjectionInversed[2][2];

    float div = ModelViewProjectionInversed[0][3] * x + ModelViewProjectionInversed[1][3] * y + ModelViewProjectionInversed[3][3];
    RaySegment.End /= div;

    div += ModelViewProjectionInversed[2][3];
    RaySegment.Start /= div;
#endif

    return RaySegment;
}

BvFrustum const & FCameraComponent::GetFrustum() const {

    // Update projection matrix
    GetProjectionMatrix();

    // Update view matrix
    GetViewMatrix();

    if ( FrustumDirty ) {
        Frustum.FromMatrix( ProjectionMatrix * ViewMatrix );

        FrustumDirty = false;
    }

    return Frustum;
}

Float4x4 const & FCameraComponent::GetViewMatrix() const {
    if ( ViewMatrixDirty ) {
        BillboardMatrix = GetWorldRotation().ToMatrix();

        Float3x3 Basis = BillboardMatrix.Transposed();
        Float3 Origin = Basis * (-GetWorldPosition());

        ViewMatrix[0] = Float4( Basis[0], 0.0f );
        ViewMatrix[1] = Float4( Basis[1], 0.0f );
        ViewMatrix[2] = Float4( Basis[2], 0.0f );
        ViewMatrix[3] = Float4( Origin, 1.0f );

        ViewMatrixDirty = false;
        FrustumDirty = true;
    }

    return ViewMatrix;
}

Float3x3 const & FCameraComponent::GetBillboardMatrix() const {
    // Update billboard matrix
    GetViewMatrix();

    return BillboardMatrix;
}
