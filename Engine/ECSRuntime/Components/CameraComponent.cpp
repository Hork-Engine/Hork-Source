#include "CameraComponent.h"

#include <Engine/Renderer/RenderDefs.h>

HK_NAMESPACE_BEGIN

void CameraComponent_ECS::SetProjection(CAMERA_PROJECTION_TYPE _Projection)
{
    if (m_Projection != _Projection)
    {
        m_Projection = _Projection;
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::SetZNear(float _ZNear)
{
    if (m_ZNear != _ZNear)
    {
        m_ZNear = _ZNear;
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::SetZFar(float _ZFar)
{
    if (m_ZFar != _ZFar)
    {
        m_ZFar = _ZFar;
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::SetFovX(float _FieldOfView)
{
    if (m_FovX != _FieldOfView)
    {
        m_FovX = _FieldOfView;
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::SetFovY(float _FieldOfView)
{
    if (m_FovY != _FieldOfView)
    {
        m_FovY = _FieldOfView;
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::SetAspectRatio(float _AspectRatio)
{
    if (m_AspectRatio != _AspectRatio)
    {
        m_AspectRatio = _AspectRatio;
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::GetEffectiveFov(float& _FovX, float& _FovY) const
{
    _FovX = 0;
    _FovY = 0;
    switch (m_Projection)
    {
        case CAMERA_PROJ_ORTHO_RECT:
        case CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO:
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y:
            _FovX = Math::Radians(m_FovX);
            _FovY = Math::Radians(m_FovY);
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO:
            _FovX = Math::Radians(m_FovX);
            _FovY = std::atan2(1.0f, m_AspectRatio / std::tan(_FovX * 0.5f)) * 2.0f;
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO:
            _FovY = Math::Radians(m_FovY);
            _FovX = std::atan(std::tan(_FovY * 0.5f) * m_AspectRatio) * 2.0f;
            break;
    }
}

void CameraComponent_ECS::SetOrthoRect(Float2 const& _Mins, Float2 const& _Maxs)
{
    m_OrthoMins = _Mins;
    m_OrthoMaxs = _Maxs;

    if (IsOrthographic())
    {
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::SetOrthoZoom(float _Zoom)
{
    m_OrthoZoom = _Zoom;

    if (IsOrthographic())
    {
        m_bProjectionDirty = true;
    }
}

void CameraComponent_ECS::MakeOrthoRect(float _CameraAspectRatio, float _Zoom, Float2& _Mins, Float2& _Maxs)
{
    if (_CameraAspectRatio > 0.0f)
    {
        const float Z = _Zoom != 0.0f ? 1.0f / _Zoom : 0.0f;
        _Maxs.X = Z;
        _Maxs.Y = Z / _CameraAspectRatio;
        _Mins = -_Maxs;
    }
    else
    {
        _Mins.X = -1;
        _Mins.Y = -1;
        _Maxs.X = 1;
        _Maxs.Y = 1;
    }
}

void CameraComponent_ECS::MakeClusterProjectionMatrix(Float4x4& _ProjectionMatrix) const
{
    // TODO: if ( ClusterProjectionDirty ...

    switch (m_Projection)
    {
        case CAMERA_PROJ_ORTHO_RECT:
            _ProjectionMatrix = Float4x4::OrthoRevCC(m_OrthoMins, m_OrthoMaxs, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        case CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO: {
            Float2 orthoMins, orthoMaxs;
            CameraComponent_ECS::MakeOrthoRect(m_AspectRatio, 1.0f / m_OrthoZoom, orthoMins, orthoMaxs);
            _ProjectionMatrix = Float4x4::OrthoRevCC(orthoMins, orthoMaxs, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        }
        case CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y:
            _ProjectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), Math::Radians(m_FovY), FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO:
            _ProjectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), m_AspectRatio, 1.0f, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        case CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO:
            _ProjectionMatrix = Float4x4::PerspectiveRevCC_Y(Math::Radians(m_FovY), m_AspectRatio, 1.0f, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
    }
}

Float4x4 const& CameraComponent_ECS::GetProjectionMatrix() const
{
    if (m_bProjectionDirty)
    {
        switch (m_Projection)
        {
            case CAMERA_PROJ_ORTHO_RECT:
                m_ProjectionMatrix = Float4x4::OrthoRevCC(m_OrthoMins, m_OrthoMaxs, m_ZNear, m_ZFar);
                break;
            case CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO: {
                Float2 orthoMins, orthoMaxs;
                CameraComponent_ECS::MakeOrthoRect(m_AspectRatio, 1.0f / m_OrthoZoom, orthoMins, orthoMaxs);
                m_ProjectionMatrix = Float4x4::OrthoRevCC(orthoMins, orthoMaxs, m_ZNear, m_ZFar);
                break;
            }
            case CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y:
                m_ProjectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), Math::Radians(m_FovY), m_ZNear, m_ZFar);
                break;
            case CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO:
                m_ProjectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), m_AspectRatio, 1.0f, m_ZNear, m_ZFar);
                break;
            case CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO:
                m_ProjectionMatrix = Float4x4::PerspectiveRevCC_Y(Math::Radians(m_FovY), m_AspectRatio, 1.0f, m_ZNear, m_ZFar);
                break;
        }

        m_bProjectionDirty = false;
        m_bFrustumDirty = true;
    }

    return m_ProjectionMatrix;
}

//void CameraComponent_ECS::MakeRay(float _NormalizedX, float _NormalizedY, Float3& _RayStart, Float3& _RayEnd) const
//{
//    // Update projection matrix
//    GetProjectionMatrix();
//
//    // Update view matrix
//    GetViewMatrix();
//
//    // TODO: cache ModelViewProjectionInversed
//    Float4x4 ModelViewProjectionInversed = (m_ProjectionMatrix * m_ViewMatrix).Inversed();
//    // TODO: try to optimize with m_ViewMatrix.ViewInverseFast() * m_ProjectionMatrix.ProjectionInverseFast()
//
//    MakeRay(ModelViewProjectionInversed, _NormalizedX, _NormalizedY, _RayStart, _RayEnd);
//}

void CameraComponent_ECS::MakeRay(Float4x4 const& _ModelViewProjectionInversed, float _NormalizedX, float _NormalizedY, Float3& _RayStart, Float3& _RayEnd)
{
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

//BvFrustum const& CameraComponent_ECS::GetFrustum() const
//{
//    // Update projection matrix
//    GetProjectionMatrix();
//
//    // Update view matrix
//    GetViewMatrix();
//
//    if (m_bFrustumDirty)
//    {
//        m_Frustum.FromMatrix(m_ProjectionMatrix * m_ViewMatrix, true);
//
//        m_bFrustumDirty = false;
//    }
//
//    return m_Frustum;
//}

//Float4x4 const& CameraComponent_ECS::GetViewMatrix() const
//{
//    //if (m_bViewMatrixDirty)
//    {
//        m_BillboardMatrix = GetWorldRotation().ToMatrix3x3();
//
//        Float3x3 basis = m_BillboardMatrix.Transposed();
//        Float3 origin = basis * (-GetWorldPosition());
//
//        m_ViewMatrix[0] = Float4(basis[0], 0.0f);
//        m_ViewMatrix[1] = Float4(basis[1], 0.0f);
//        m_ViewMatrix[2] = Float4(basis[2], 0.0f);
//        m_ViewMatrix[3] = Float4(origin, 1.0f);
//
//        //m_bViewMatrixDirty = false;
//        m_bFrustumDirty = true;
//    }
//
//    return m_ViewMatrix;
//}

//Float3x3 const& CameraComponent_ECS::GetBillboardMatrix() const
//{
//    // Update billboard matrix
//    GetViewMatrix();
//
//    return m_BillboardMatrix;
//}

#if 0
void CameraComponent_ECS::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawCameraFrustum)
    {
        Float3 vectorTR;
        Float3 vectorTL;
        Float3 vectorBR;
        Float3 vectorBL;
        Float3 origin = GetWorldPosition();
        Float3 v[4];
        Float3 faces[4][3];
        float rayLength = 32;

        m_Frustum.CornerVector_TR(vectorTR);
        m_Frustum.CornerVector_TL(vectorTL);
        m_Frustum.CornerVector_BR(vectorBR);
        m_Frustum.CornerVector_BL(vectorBL);

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

        InRenderer->SetDepthTest(true);

        InRenderer->SetColor(Color4(0, 1, 1, 1));
        InRenderer->DrawLine(origin, v[0]);
        InRenderer->DrawLine(origin, v[3]);
        InRenderer->DrawLine(origin, v[1]);
        InRenderer->DrawLine(origin, v[2]);
        InRenderer->DrawLine(v, true);

        InRenderer->SetColor(Color4(1, 1, 1, 0.3f));
        InRenderer->DrawTriangles(&faces[0][0], 4, sizeof(Float3), false);
        InRenderer->DrawConvexPoly(v, false);
    }
}
#endif

HK_NAMESPACE_END
