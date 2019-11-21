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

#include <World/Public/Components/CameraComponent.h>
#include <World/Public/Base/DebugDraw.h>

#include <Core/Public/Logger.h>
#include <Core/Public/ConvexHull.h>

#include <Runtime/Public/Runtime.h>

#define DEFAULT_PROJECTION CAMERA_PROJ_PERSPECTIVE
#define DEFAULT_ZNEAR 0.04f//0.1f
#define DEFAULT_ZFAR 99999.0f
#define DEFAULT_FOVX 90.0f
#define DEFAULT_FOVY 90.0f
#define DEFAULT_ASPECT_RATIO (4.0f / 3.0f)
#define DEFAULT_PERSPECTIVE_ADJUST CAMERA_ADJUST_FOV_X_ASPECT_RATIO

ARuntimeVariable RVDrawCameraFrustum( _CTS( "DrawCameraFrustum" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawFrustumClusters( _CTS( "DrawFrustumClusters" ), _CTS( "1" ), VAR_CHEAT );

AN_CLASS_META( ACameraComponent )

ACameraComponent::ACameraComponent() {
    Projection = DEFAULT_PROJECTION;
    ZNear = DEFAULT_ZNEAR;
    ZFar = DEFAULT_ZFAR;
    FovX = DEFAULT_FOVX;
    FovY = DEFAULT_FOVY;
    AspectRatio = DEFAULT_ASPECT_RATIO;
    Adjust = DEFAULT_PERSPECTIVE_ADJUST;
    OrthoMins.X = -1;
    OrthoMins.Y = -1;
    OrthoMaxs.X = 1;
    OrthoMaxs.Y = 1;
    bViewMatrixDirty = true;
    bFrustumDirty = true;
    bProjectionDirty = true;
}

void ACameraComponent::DrawDebug( ADebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    if ( RVDrawCameraFrustum ) {
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

        _DebugDraw->SetColor( AColor4( 0, 1, 1, 1 ) );
        _DebugDraw->DrawLine( origin, v[0] );
        _DebugDraw->DrawLine( origin, v[3] );
        _DebugDraw->DrawLine( origin, v[1] );
        _DebugDraw->DrawLine( origin, v[2] );
        _DebugDraw->DrawLine( v, 4, true );

        _DebugDraw->SetColor( AColor4( 1, 1, 1, 0.3f ) );
        _DebugDraw->DrawTriangles( &faces[0][0], 4, sizeof( Float3 ), false );
        _DebugDraw->DrawConvexPoly( v, 4, false );
    }

    if ( RVDrawFrustumClusters ) {
        Float3 clusterMins;
        Float3 clusterMaxs;
        Float4 p[ 8 ];
        Float3 lineP[ 8 ];
        Float4x4 projMat;

        MakeClusterProjectionMatrix( projMat/*, ZNear, ZFar*/ );

        Float4x4 ViewProj = projMat * GetViewMatrix();
        Float4x4 ViewProjInv = ViewProj.Inversed();

        float * zclip = GFrustumSlice.ZClip;

        for ( int sliceIndex = 0 ; sliceIndex < SFrustumSlice::NUM_CLUSTERS_Z ; sliceIndex++ ) {

            clusterMins.Z = zclip[ sliceIndex + 1 ];
            clusterMaxs.Z = zclip[ sliceIndex ];

            for ( int clusterY = 0 ; clusterY < SFrustumSlice::NUM_CLUSTERS_Y ; clusterY++ ) {

                clusterMins.Y = clusterY * GFrustumSlice.DeltaY - 1.0f;
                clusterMaxs.Y = clusterMins.Y + GFrustumSlice.DeltaY;

                for ( int clusterX = 0 ; clusterX < SFrustumSlice::NUM_CLUSTERS_X ; clusterX++ ) {

                    clusterMins.X = clusterX * GFrustumSlice.DeltaX - 1.0f;
                    clusterMaxs.X = clusterMins.X + GFrustumSlice.DeltaX;

                    if (   GFrustumSlice.Clusters[ sliceIndex ][ clusterY ][ clusterX ].LightsCount > 0
                        || GFrustumSlice.Clusters[ sliceIndex ][ clusterY ][ clusterX ].DecalsCount > 0
                        || GFrustumSlice.Clusters[ sliceIndex ][ clusterY ][ clusterX ].ProbesCount > 0 ) {
                        p[ 0 ] = Float4( clusterMins.X, clusterMins.Y, clusterMins.Z, 1.0f );
                        p[ 1 ] = Float4( clusterMaxs.X, clusterMins.Y, clusterMins.Z, 1.0f );
                        p[ 2 ] = Float4( clusterMaxs.X, clusterMaxs.Y, clusterMins.Z, 1.0f );
                        p[ 3 ] = Float4( clusterMins.X, clusterMaxs.Y, clusterMins.Z, 1.0f );
                        p[ 4 ] = Float4( clusterMaxs.X, clusterMins.Y, clusterMaxs.Z, 1.0f );
                        p[ 5 ] = Float4( clusterMins.X, clusterMins.Y, clusterMaxs.Z, 1.0f );
                        p[ 6 ] = Float4( clusterMins.X, clusterMaxs.Y, clusterMaxs.Z, 1.0f );
                        p[ 7 ] = Float4( clusterMaxs.X, clusterMaxs.Y, clusterMaxs.Z, 1.0f );
                        for ( int i = 0 ; i < 8 ; i++ ) {
                            p[ i ] = ViewProjInv * p[ i ];
                            const float Denom = 1.0f / p[ i ].W;
                            lineP[ i ].X = p[ i ].X * Denom;
                            lineP[ i ].Y = p[ i ].Y * Denom;
                            lineP[ i ].Z = p[ i ].Z * Denom;
                        }
                        //if ( RVClusterSSE )//if ( RVReverseNegativeZ )
                        //    _DebugDraw->SetColor( AColor4( 0, 0, 1 ) );
                        //else
                            _DebugDraw->SetColor( AColor4( 1, 0, 0 ) );
                        _DebugDraw->DrawLine( lineP, 4, true );
                        _DebugDraw->DrawLine( lineP + 4, 4, true );
                        _DebugDraw->DrawLine( lineP[ 0 ], lineP[ 5 ] );
                        _DebugDraw->DrawLine( lineP[ 1 ], lineP[ 4 ] );
                        _DebugDraw->DrawLine( lineP[ 2 ], lineP[ 7 ] );
                        _DebugDraw->DrawLine( lineP[ 3 ], lineP[ 6 ] );
                    }
                }
            }
        }
    }
}

void ACameraComponent::SetProjection( ECameraProjection _Projection ) {
    if ( Projection != _Projection ) {
        Projection = _Projection;
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetZNear( float _ZNear ) {
    if ( ZNear != _ZNear ) {
        ZNear = _ZNear;
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetZFar( float _ZFar ) {
    if ( ZFar != _ZFar ) {
        ZFar = _ZFar;
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetFovX( float _FieldOfView ) {
    if ( FovX != _FieldOfView ) {
        FovX = _FieldOfView;
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetFovY( float _FieldOfView ) {
    if ( FovY != _FieldOfView ) {
        FovY = _FieldOfView;
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetAspectRatio( float _AspectRatio ) {
    if ( AspectRatio != _AspectRatio ) {
        AspectRatio = _AspectRatio;
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetMonitorAspectRatio( const SPhysicalMonitor * _Monitor ) {
    const float MonitorAspectRatio = ( float )_Monitor->PhysicalWidthMM / _Monitor->PhysicalHeightMM;
    SetAspectRatio( MonitorAspectRatio );
}

//void ACameraComponent::SetWindowAspectRatio( FWindow const * _Window ) {
//    const int w = _Window->GetWidth();
//    const int h = _Window->GetHeight();
//    const float WindowAspectRatio = h > 0 ? ( float )w / h : DEFAULT_ASPECT_RATIO;
//    SetAspectRatio( WindowAspectRatio );
//}

void ACameraComponent::SetPerspectiveAdjust( ECameraPerspectiveAdjust _Adjust ) {
    if ( Adjust != _Adjust ) {
        Adjust = _Adjust;
        bProjectionDirty = true;
    }
}

void ACameraComponent::GetEffectiveFov( float & _FovX, float & _FovY ) const {
    _FovX = Math::Radians( FovX );

    switch ( Adjust ) {
        //case ADJ_FOV_X_VIEW_SIZE:
        //{
        //    const float TanHalfFovX = tan( _FovX * 0.5f );
        //    _FovY = atan2( static_cast< float >( Height ), Width / TanHalfFovX ) * 2.0f;
        //    break;
        //}
        case CAMERA_ADJUST_FOV_X_ASPECT_RATIO:
        {
            const float TanHalfFovX = tan( _FovX * 0.5f );

            _FovY = atan2( 1.0f, AspectRatio / TanHalfFovX ) * 2.0f;
            break;
        }
        case CAMERA_ADJUST_FOV_X_FOV_Y:
        {
            _FovY = Math::Radians( FovY );
            break;
        }
    }
}

void ACameraComponent::SetOrthoRect( Float2 const & _Mins, Float2 const & _Maxs ) {
    OrthoMins = _Mins;
    OrthoMaxs = _Maxs;

    if ( IsOrthographic() ) {
        bProjectionDirty = true;
    }
}

void ACameraComponent::MakeOrthoRect( float _CameraAspectRatio, float _Zoom, Float2 & _Mins, Float2 & _Maxs ) {
    if ( _CameraAspectRatio > 0.0f ) {
        const float Z = _Zoom != 0.0f ? 1.0f / _Zoom : 0.0f;
        _Maxs.X = Z;
        _Maxs.Y = Z / _CameraAspectRatio;
        _Mins = -_Maxs;
    } else {
        _Mins.X = -1;
        _Mins.Y = -1;
        _Maxs.X = 1;
        _Maxs.Y = 1;
    }
}

void ACameraComponent::OnTransformDirty() {
    bViewMatrixDirty = true;
}

void ACameraComponent::MakeClusterProjectionMatrix( Float4x4 & _ProjectionMatrix/*, const float _ClusterZNear, const float _ClusterZFar*/ ) const {
    const float _ClusterZNear = GFrustumSlice.ZNear;
    const float _ClusterZFar = GFrustumSlice.ZFar;

    // TODO: if ( ClusterProjectionDirty ...
    if ( Projection == CAMERA_PROJ_PERSPECTIVE ) {
        switch ( Adjust ) {
            //case CAMERA_ADJUST_FOV_X_VIEW_SIZE:
            //{
            //    _ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), float( Width ), float( Height ), _ClusterZNear, _ClusterZFar );
            //    break;
            //}
            case CAMERA_ADJUST_FOV_X_ASPECT_RATIO:
            {
                _ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), AspectRatio, 1.0f, _ClusterZNear, _ClusterZFar );
                break;
            }
            case CAMERA_ADJUST_FOV_X_FOV_Y:
            {
                _ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), Math::Radians( FovY ), _ClusterZNear, _ClusterZFar );
                break;
            }
        }
    } else {
        _ProjectionMatrix = Float4x4::OrthoRevCC( OrthoMins, OrthoMaxs, _ClusterZNear, _ClusterZFar );
    }
}

Float4x4 const & ACameraComponent::GetProjectionMatrix() const {

    if ( bProjectionDirty ) {

        if ( Projection == CAMERA_PROJ_PERSPECTIVE ) {
            switch ( Adjust ) {
                //case CAMERA_ADJUST_FOV_X_VIEW_SIZE:
                //{
                //    ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), float( Width ), float( Height ), ZNear, ZFar );
                //    break;
                //}
                case CAMERA_ADJUST_FOV_X_ASPECT_RATIO:
                {
                    ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), AspectRatio, 1.0f, ZNear, ZFar );
                    break;
                }
                case CAMERA_ADJUST_FOV_X_FOV_Y:
                {
                    ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), Math::Radians( FovY ), ZNear, ZFar );
                    break;
                }
            }
        } else {
            ProjectionMatrix = Float4x4::OrthoRevCC( OrthoMins, OrthoMaxs, ZNear, ZFar );
        }

        bProjectionDirty = false;
        bFrustumDirty = true;
    }

    return ProjectionMatrix;
}

void ACameraComponent::MakeRay( float _NormalizedX, float _NormalizedY, Float3 & _RayStart, Float3 & _RayEnd ) const {
    // Update projection matrix
    GetProjectionMatrix();

    // Update view matrix
    GetViewMatrix();

    // TODO: cache ModelViewProjectionInversed
    Float4x4 ModelViewProjectionInversed = ( ProjectionMatrix * ViewMatrix ).Inversed();

    MakeRay( ModelViewProjectionInversed, _NormalizedX, _NormalizedY, _RayStart, _RayEnd );
}

void ACameraComponent::MakeRay( Float4x4 const & _ModelViewProjectionInversed, float _NormalizedX, float _NormalizedY, Float3 & _RayStart, Float3 & _RayEnd ) {
    float x = 2.0f * _NormalizedX - 1.0f;
    float y = 2.0f * _NormalizedY - 1.0f;
#if 0
    Float4 near(x, y, 1, 1.0f);
    Float4 far(x, y, 0, 1.0f);
    _RayStart.X = _ModelViewProjectionInversed[0][0] * near[0] + _ModelViewProjectionInversed[1][0] * near[1] + _ModelViewProjectionInversed[2][0] * near[2] + _ModelViewProjectionInversed[3][0];
    _RayStart.Y = _ModelViewProjectionInversed[0][1] * near[0] + _ModelViewProjectionInversed[1][1] * near[1] + _ModelViewProjectionInversed[2][1] * near[2] + _ModelViewProjectionInversed[3][1];
    _RayStart.Z = _ModelViewProjectionInversed[0][2] * near[0] + _ModelViewProjectionInversed[1][2] * near[1] + _ModelViewProjectionInversed[2][2] * near[2] + _ModelViewProjectionInversed[3][2];
    _RayStart /= (_ModelViewProjectionInversed[0][3] * near[0] + _ModelViewProjectionInversed[1][3] * near[1] + _ModelViewProjectionInversed[2][3] * near[2] + _ModelViewProjectionInversed[3][3]);
    _RayEnd.X = _ModelViewProjectionInversed[0][0] * far[0] + _ModelViewProjectionInversed[1][0] * far[1] + _ModelViewProjectionInversed[2][0] * far[2] + _ModelViewProjectionInversed[3][0];
    _RayEnd.Y = _ModelViewProjectionInversed[0][1] * far[0] + _ModelViewProjectionInversed[1][1] * far[1] + _ModelViewProjectionInversed[2][1] * far[2] + _ModelViewProjectionInversed[3][1];
    _RayEnd.Z = _ModelViewProjectionInversed[0][2] * far[0] + _ModelViewProjectionInversed[1][2] * far[1] + _ModelViewProjectionInversed[2][2] * far[2] + _ModelViewProjectionInversed[3][2];
    _RayEnd /= (_ModelViewProjectionInversed[0][3] * far[0] + _ModelViewProjectionInversed[1][3] * far[1] + _ModelViewProjectionInversed[2][3] * far[2] + _ModelViewProjectionInversed[3][3]);
#else
    // Same code
    _RayEnd.X = _ModelViewProjectionInversed[0][0] * x + _ModelViewProjectionInversed[1][0] * y + _ModelViewProjectionInversed[3][0];
    _RayEnd.Y = _ModelViewProjectionInversed[0][1] * x + _ModelViewProjectionInversed[1][1] * y + _ModelViewProjectionInversed[3][1];
    _RayEnd.Z = _ModelViewProjectionInversed[0][2] * x + _ModelViewProjectionInversed[1][2] * y + _ModelViewProjectionInversed[3][2];
    _RayStart.X = _RayEnd.X + _ModelViewProjectionInversed[2][0];
    _RayStart.Y = _RayEnd.Y + _ModelViewProjectionInversed[2][1];
    _RayStart.Z = _RayEnd.Z + _ModelViewProjectionInversed[2][2];
    float div = _ModelViewProjectionInversed[0][3] * x + _ModelViewProjectionInversed[1][3] * y + _ModelViewProjectionInversed[3][3];
    _RayEnd /= div;
    div += _ModelViewProjectionInversed[2][3];
    _RayStart /= div;
#endif
}

BvFrustum const & ACameraComponent::GetFrustum() const {

    // Update projection matrix
    GetProjectionMatrix();

    // Update view matrix
    GetViewMatrix();

    if ( bFrustumDirty ) {
        Frustum.FromMatrix( ProjectionMatrix * ViewMatrix );

        bFrustumDirty = false;
    }

    return Frustum;
}

Float4x4 const & ACameraComponent::GetViewMatrix() const {
    if ( bViewMatrixDirty ) {
        BillboardMatrix = GetWorldRotation().ToMatrix();

        Float3x3 Basis = BillboardMatrix.Transposed();
        Float3 Origin = Basis * (-GetWorldPosition());

        ViewMatrix[0] = Float4( Basis[0], 0.0f );
        ViewMatrix[1] = Float4( Basis[1], 0.0f );
        ViewMatrix[2] = Float4( Basis[2], 0.0f );
        ViewMatrix[3] = Float4( Origin, 1.0f );

        bViewMatrixDirty = false;
        bFrustumDirty = true;
    }

    return ViewMatrix;
}

Float3x3 const & ACameraComponent::GetBillboardMatrix() const {
    // Update billboard matrix
    GetViewMatrix();

    return BillboardMatrix;
}
