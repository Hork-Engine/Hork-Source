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

#include "SceneComponent.h"

#include <Core/Public/BV/BvFrustum.h>

enum ECameraProjection : uint8_t
{
    CAMERA_PROJ_ORTHO_RECT,
    CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO,
    CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y,
    CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO,
    CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO
};

class ACameraComponent : public ASceneComponent {
    AN_COMPONENT( ACameraComponent, ASceneComponent )

public:
    /** Set view projection */
    void SetProjection( ECameraProjection _Projection );

    /** Near clip distance */
    void SetZNear( float _ZNear );

    /** Far clip distance */
    void SetZFar( float _ZFar );

    /** Viewport aspect ratio. For example 4/3, 16/9 */
    void SetAspectRatio( float _AspectRatio );

    /** Horizontal FOV for perspective projection */
    void SetFovX( float _FieldOfView );

    /** Vertical FOV for perspective projection */
    void SetFovY( float _FieldOfView );

    /** Rectangle for orthogonal projection */
    void SetOrthoRect( Float2 const & _Mins, Float2 const & _Maxs );

    /** Zoom for orthogonal projection */
    void SetOrthoZoom( float _Zoom );

    ECameraProjection GetProjection() const { return Projection; }

    bool IsPerspective() const {
        return Projection == CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO
            || Projection == CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO
            || Projection == CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y;
    }

    bool IsOrthographic() const { return Projection == CAMERA_PROJ_ORTHO_RECT || Projection == CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO; }

    float GetZNear() const { return ZNear; }

    float GetZFar() const { return ZFar; }

    float GetAspectRatio() const { return AspectRatio; }

    float GetFovX() const { return FovX; }

    float GetFovY() const { return FovY; }

    /** Computes real camera field of view in radians for perspective projection */
    void GetEffectiveFov( float & _FovX, float & _FovY ) const;

    Float2 const & GetOrthoMins() const { return OrthoMins; }

    Float2 const & GetOrthoMaxs() const { return OrthoMaxs; }

    Float4x4 const & GetProjectionMatrix() const;

    Float4x4 const & GetViewMatrix() const;

    Float3x3 const & GetBillboardMatrix() const;

    BvFrustum const & GetFrustum() const;

    /** NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight */
    void MakeRay( float _NormalizedX, float _NormalizedY, Float3 & _RayStart, Float3 & _RayEnd ) const;

    static void MakeRay( Float4x4 const & _ModelViewProjectionInversed, float _NormalizedX, float _NormalizedY, Float3 & _RayStart, Float3 & _RayEnd );

    /** Compute ortho rect based on aspect ratio and zoom */
    static void MakeOrthoRect( float _CameraAspectRatio, float _Zoom, Float2 & _Mins, Float2 & _Maxs );

    void MakeClusterProjectionMatrix( Float4x4 & _ProjectionMatrix/*, const float _ClusterZNear, const float _ClusterZFar*/ ) const;

protected:
    ACameraComponent();

    void OnCreateAvatar() override;

    void OnTransformDirty() override;

    void DrawDebug( ADebugRenderer * InRenderer ) override;

private:
    float               FovX;
    float               FovY;
    float               ZNear;
    float               ZFar;
    float               AspectRatio;
    Float2              OrthoMins;
    Float2              OrthoMaxs;
    float               OrthoZoom;
    mutable Float4x4    ViewMatrix;
    mutable Float3x3    BillboardMatrix;    
    mutable Float4x4    ProjectionMatrix;    
    mutable BvFrustum   Frustum;
    ECameraProjection   Projection;
    mutable bool        bViewMatrixDirty;
    mutable bool        bProjectionDirty;
    mutable bool        bFrustumDirty;
};
