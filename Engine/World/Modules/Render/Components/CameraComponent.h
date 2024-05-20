#pragma once

#include <Engine/Math/Quat.h>
#include <Engine/Geometry/BV/BvFrustum.h>
#include <Engine/World/Modules/Transform/EntityGraph.h>

HK_NAMESPACE_BEGIN

enum CAMERA_PROJECTION_TYPE : uint8_t
{
    CAMERA_PROJ_ORTHO_RECT,
    CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO,
    CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y,
    CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO,
    CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO
};

struct CameraComponent
{
    CameraComponent() = default;

    // The camera has its own position and rotation, which can be updated with a variable time step. It can be used to reduce input lag.
    Float3 OffsetPosition;
    Quat OffsetRotation;
    EntityNodeID Node;

    /** Set view projection */
    void SetProjection(CAMERA_PROJECTION_TYPE _Projection);

    /** Near clip distance */
    void SetZNear(float _ZNear);

    /** Far clip distance */
    void SetZFar(float _ZFar);

    /** Viewport aspect ratio. For example 4/3, 16/9 */
    void SetAspectRatio(float _AspectRatio);

    /** Horizontal FOV for perspective projection */
    void SetFovX(float _FieldOfView);

    /** Vertical FOV for perspective projection */
    void SetFovY(float _FieldOfView);

    /** Rectangle for orthogonal projection */
    void SetOrthoRect(Float2 const& _Mins, Float2 const& _Maxs);

    /** Zoom for orthogonal projection */
    void SetOrthoZoom(float _Zoom);

    CAMERA_PROJECTION_TYPE GetProjection() const { return m_Projection; }

    bool IsPerspective() const
    {
        return m_Projection == CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO || m_Projection == CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO || m_Projection == CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y;
    }

    bool IsOrthographic() const { return m_Projection == CAMERA_PROJ_ORTHO_RECT || m_Projection == CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO; }

    float GetZNear() const { return m_ZNear; }

    float GetZFar() const { return m_ZFar; }

    float GetAspectRatio() const { return m_AspectRatio; }

    float GetFovX() const { return m_FovX; }

    float GetFovY() const { return m_FovY; }

    /** Computes real camera field of view in radians for perspective projection */
    void GetEffectiveFov(float& _FovX, float& _FovY) const;

    Float2 const& GetOrthoMins() const { return m_OrthoMins; }

    Float2 const& GetOrthoMaxs() const { return m_OrthoMaxs; }

    Float4x4 const& GetProjectionMatrix() const;

    //Float4x4 const& GetViewMatrix() const;

    //Float3x3 const& GetBillboardMatrix() const;

    //BvFrustum const& GetFrustum() const;

    /** NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight */
    //void MakeRay(float _NormalizedX, float _NormalizedY, Float3& _RayStart, Float3& _RayEnd) const;

    static void MakeRay(Float4x4 const& _ModelViewProjectionInversed, float _NormalizedX, float _NormalizedY, Float3& _RayStart, Float3& _RayEnd);

    /** Compute ortho rect based on aspect ratio and zoom */
    static void MakeOrthoRect(float _CameraAspectRatio, float _Zoom, Float2& _Mins, Float2& _Maxs);

    void MakeClusterProjectionMatrix(Float4x4& _ProjectionMatrix /*, const float _ClusterZNear, const float _ClusterZFar*/) const;

    //void DrawDebug(DebugRenderer* InRenderer) override;


    // Internal

    float m_FovX{90.0f};
    float m_FovY{90.0f};
    float m_ZNear{0.04f};
    float m_ZFar{99999.0f};
    float m_AspectRatio{1.0f};
    Float2 m_OrthoMins{-1, -1};
    Float2 m_OrthoMaxs{1, 1};
    float m_OrthoZoom{30};
    //mutable Float4x4 m_ViewMatrix;
    mutable Float3x3 m_BillboardMatrix;
    mutable Float4x4 m_ProjectionMatrix;
    mutable BvFrustum m_Frustum;
    CAMERA_PROJECTION_TYPE m_Projection{CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO};
    //mutable bool m_bViewMatrixDirty{true};
    mutable bool m_bProjectionDirty{true};
    mutable bool m_bFrustumDirty{true};
};

HK_NAMESPACE_END
