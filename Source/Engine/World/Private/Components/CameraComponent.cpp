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

#include <World/Public/Components/CameraComponent.h>
#include <World/Public/Components/MeshComponent.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/Base/DebugRenderer.h>

//#include <Core/Public/Logger.h>
//#include <Core/Public/ConvexHull.h>

#include <Runtime/Public/RuntimeVariable.h>

#define DEFAULT_PROJECTION CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO
#define DEFAULT_ZNEAR 0.04f//0.1f
#define DEFAULT_ZFAR 99999.0f
#define DEFAULT_FOVX 100.0f
#define DEFAULT_FOVY 100.0f
#define DEFAULT_ASPECT_RATIO (4.0f / 3.0f)
#define DEFAULT_ORTHO_ZOOM 30.0f

ARuntimeVariable com_DrawCameraFrustum( _CTS( "com_DrawCameraFrustum" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ACameraComponent )

ACameraComponent::ACameraComponent() {
    FovX = DEFAULT_FOVX;
    FovY = DEFAULT_FOVY;
    ZNear = DEFAULT_ZNEAR;
    ZFar = DEFAULT_ZFAR;
    AspectRatio = DEFAULT_ASPECT_RATIO;
    OrthoMins.X = -1;
    OrthoMins.Y = -1;
    OrthoMaxs.X = 1;
    OrthoMaxs.Y = 1;
    OrthoZoom = DEFAULT_ORTHO_ZOOM;
    Projection = DEFAULT_PROJECTION;
    bViewMatrixDirty = true;
    bProjectionDirty = true;
    bFrustumDirty = true;
}

void ACameraComponent::OnCreateAvatar() {
    Super::OnCreateAvatar();

    // TODO: Create mesh or sprite for avatar
    static TStaticResourceFinder< AIndexedMesh > Mesh( _CTS( "/Default/Meshes/Box" ) );
    static TStaticResourceFinder< AMaterialInstance > MaterialInstance( _CTS( "AvatarMaterialInstance" ) );
    AMeshComponent * meshComponent = GetOwnerActor()->CreateComponent< AMeshComponent >( "CameraAvatar" );
    meshComponent->SetMotionBehavior( MB_KINEMATIC );
    meshComponent->SetCollisionGroup( CM_NOCOLLISION );
    meshComponent->SetMesh( Mesh.GetObject() );
    meshComponent->SetMaterialInstance( MaterialInstance.GetObject() );
    meshComponent->SetCastShadow( false );
    meshComponent->SetAbsoluteScale( true );
    meshComponent->SetScale( 0.5f );
    meshComponent->AttachTo( this );
    meshComponent->SetHideInEditor( true );
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

void ACameraComponent::GetEffectiveFov( float & _FovX, float & _FovY ) const {
    _FovX = 0;
    _FovY = 0;
    switch ( Projection ) {
    case CAMERA_PROJ_ORTHO_RECT:
    case CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO:
        break;
    case CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y:
        _FovX = Math::Radians( FovX );
        _FovY = Math::Radians( FovY );
        break;
    case CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO:
        _FovX = Math::Radians( FovX );
        _FovY = atan2( 1.0f, AspectRatio / tan( _FovX * 0.5f ) ) * 2.0f;
        break;
    case CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO:
        _FovY = Math::Radians( FovY );
        _FovX = atan( tan( _FovY * 0.5f ) * AspectRatio ) * 2.0f;
        break;
    }
}

void ACameraComponent::SetOrthoRect( Float2 const & _Mins, Float2 const & _Maxs ) {
    OrthoMins = _Mins;
    OrthoMaxs = _Maxs;

    if ( IsOrthographic() ) {
        bProjectionDirty = true;
    }
}

void ACameraComponent::SetOrthoZoom( float _Zoom ) {
    OrthoZoom = _Zoom;

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

void ACameraComponent::MakeClusterProjectionMatrix( Float4x4 & _ProjectionMatrix ) const {
    // TODO: if ( ClusterProjectionDirty ...

    switch ( Projection ) {
    case CAMERA_PROJ_ORTHO_RECT:
        _ProjectionMatrix = Float4x4::OrthoRevCC( OrthoMins, OrthoMaxs, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR );
        break;
    case CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO: {
        Float2 orthoMins, orthoMaxs;
        ACameraComponent::MakeOrthoRect( AspectRatio, 1.0f / OrthoZoom, orthoMins, orthoMaxs );
        _ProjectionMatrix = Float4x4::OrthoRevCC( orthoMins, orthoMaxs, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR );
        break;
    }
    case CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y:
        _ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), Math::Radians( FovY ), FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR );
        break;
    case CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO:
        _ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), AspectRatio, 1.0f, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR );
        break;
    case CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO:
        _ProjectionMatrix = Float4x4::PerspectiveRevCC_Y( Math::Radians( FovY ), AspectRatio, 1.0f, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR );
        break;
    }
}

Float4x4 const & ACameraComponent::GetProjectionMatrix() const {
    if ( bProjectionDirty ) {
        switch ( Projection ) {
        case CAMERA_PROJ_ORTHO_RECT:
            ProjectionMatrix = Float4x4::OrthoRevCC( OrthoMins, OrthoMaxs, ZNear, ZFar );
            break;
        case CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO: {
            Float2 orthoMins, orthoMaxs;
            ACameraComponent::MakeOrthoRect( AspectRatio, 1.0f / OrthoZoom, orthoMins, orthoMaxs );
            ProjectionMatrix = Float4x4::OrthoRevCC( orthoMins, orthoMaxs, ZNear, ZFar );
            break;
        }
        case CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y:
            ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), Math::Radians( FovY ), ZNear, ZFar );
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO:
            ProjectionMatrix = Float4x4::PerspectiveRevCC( Math::Radians( FovX ), AspectRatio, 1.0f, ZNear, ZFar );
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO:
            ProjectionMatrix = Float4x4::PerspectiveRevCC_Y( Math::Radians( FovY ), AspectRatio, 1.0f, ZNear, ZFar );
            break;
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
    // TODO: try to optimize with ViewMatrix.ViewInverseFast() * ProjectionMatrix.ProjectionInverseFast()

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
        Frustum.FromMatrix( ProjectionMatrix * ViewMatrix, true );

        bFrustumDirty = false;
    }

    return Frustum;
}

Float4x4 const & ACameraComponent::GetViewMatrix() const {
    if ( bViewMatrixDirty ) {
        BillboardMatrix = GetWorldRotation().ToMatrix();

        Float3x3 basis = BillboardMatrix.Transposed();
        Float3 origin = basis * (-GetWorldPosition());

        ViewMatrix[0] = Float4( basis[0], 0.0f );
        ViewMatrix[1] = Float4( basis[1], 0.0f );
        ViewMatrix[2] = Float4( basis[2], 0.0f );
        ViewMatrix[3] = Float4( origin, 1.0f );

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

void ACameraComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( com_DrawCameraFrustum ) {
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

        InRenderer->SetDepthTest( true );

        InRenderer->SetColor( AColor4( 0, 1, 1, 1 ) );
        InRenderer->DrawLine( origin, v[0] );
        InRenderer->DrawLine( origin, v[3] );
        InRenderer->DrawLine( origin, v[1] );
        InRenderer->DrawLine( origin, v[2] );
        InRenderer->DrawLine( v, 4, true );

        InRenderer->SetColor( AColor4( 1, 1, 1, 0.3f ) );
        InRenderer->DrawTriangles( &faces[0][0], 4, sizeof( Float3 ), false );
        InRenderer->DrawConvexPoly( v, 4, false );
    }
}
