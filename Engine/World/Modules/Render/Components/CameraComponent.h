#pragma once

#include <Engine/World/Component.h>

#include <Engine/Math/Quat.h>
#include <Engine/Geometry/BV/BvFrustum.h>

HK_NAMESPACE_BEGIN

enum class CameraProjection : uint8_t
{
    OrthoRect,
    OrthoZoomWithAspectRatio,
    PerspectiveFovProvided,
    PerspectiveFovXWithAspectRatio,
    PerspectiveFovYWithAspectRatio
};

class CameraComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    // TODO: Add offset position and offset rotation?
    //Float3                Position;
    //Quat                  Rotation;

    /// Set view projection
    void                    SetProjection(CameraProjection projection);

    /// Near clip distance
    void                    SetZNear(float zNear);

    /// Far clip distance
    void                    SetZFar(float zFar);

    /// Viewport aspect ratio. For example 4/3, 16/9
    void                    SetAspectRatio(float aspectRatio);

    /// Horizontal FOV for perspective projection
    void                    SetFovX(float fov);

    /// Vertical FOV for perspective projection
    void                    SetFovY(float fov);

    /// Rectangle for orthogonal projection
    void                    SetOrthoRect(Float2 const& mins, Float2 const& maxs);

    /// Zoom for orthogonal projection
    void                    SetOrthoZoom(float zoom);

    CameraProjection        GetProjection() const { return m_Projection; }

    bool                    IsPerspective() const { return m_Projection == CameraProjection::PerspectiveFovProvided || m_Projection == CameraProjection::PerspectiveFovXWithAspectRatio || m_Projection == CameraProjection::PerspectiveFovYWithAspectRatio; }

    bool                    IsOrthographic() const { return m_Projection == CameraProjection::OrthoRect || m_Projection == CameraProjection::OrthoZoomWithAspectRatio; }

    float                   GetZNear() const { return m_ZNear; }

    float                   GetZFar() const { return m_ZFar; }

    float                   GetAspectRatio() const { return m_AspectRatio; }

    float                   GetFovX() const { return m_FovX; }

    float                   GetFovY() const { return m_FovY; }

    /// Computes real camera field of view in radians for perspective projection
    void                    GetEffectiveFov(float& fovX, float& fovY) const;

    Float2 const&           GetOrthoMins() const { return m_OrthoMins; }

    Float2 const&           GetOrthoMaxs() const { return m_OrthoMaxs; }

    Float4x4 const&         GetProjectionMatrix() const;

    //Float4x4 const&       GetViewMatrix() const;

    //Float3x3 const&       GetBillboardMatrix() const;

    //BvFrustum const&      GetFrustum() const;

    /// NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight
    //void                  MakeRay(float normalizedX, float normalizedY, Float3& rayStart, Float3& rayEnd) const;

    static void             MakeRay(Float4x4 const& modelViewProjectionInversed, float normalizedX, float normalizedY, Float3& rayStart, Float3& rayEnd);

    /// Compute ortho rect based on aspect ratio and zoom
    static void             MakeOrthoRect(float aspectRatio, float zoom, Float2& mins, Float2& maxs);

    void                    MakeClusterProjectionMatrix(Float4x4& projectionMatrix /*, float clusterZNear, float clusterZFar*/) const;

private:
    CameraProjection        m_Projection = CameraProjection::PerspectiveFovYWithAspectRatio;
    float                   m_FovX = 90.0f;
    float                   m_FovY = 90.0f;
    float                   m_ZNear = 0.04f;
    float                   m_ZFar = 99999.0f;
    float                   m_AspectRatio = 1.0f;
    Float2                  m_OrthoMins{-1, -1};
    Float2                  m_OrthoMaxs{1, 1};
    float                   m_OrthoZoom = 30;

    // Cache

    //mutable Float4x4      m_ViewMatrix;
    //mutable Float3x3      m_BillboardMatrix;
    mutable Float4x4        m_ProjectionMatrix;
    mutable BvFrustum       m_Frustum;
    //mutable bool          m_ViewMatrixDirty = true;
    mutable bool            m_ProjectionDirty = true;
    mutable bool            m_FrustumDirty = true;
};

HK_NAMESPACE_END
