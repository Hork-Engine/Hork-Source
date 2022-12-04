/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "SceneComponent.h"
#include "SkinnedComponent.h"
#include "Actor.h"
#include "World.h"

#include <Core/ConsoleVar.h>
#include <Platform/Logger.h>

#include <algorithm> // find

AConsoleVar com_DrawSockets("com_DrawSockets"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(ASceneComponent)

ASceneComponent::ASceneComponent() :
    m_bAbsolutePosition(false), m_bAbsoluteRotation(false), m_bAbsoluteScale(false)
{
}

void ASceneComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    AActor* owner = GetOwnerActor();

    HK_ASSERT(owner);

    if (!owner)
    {
        return;
    }

    if (!owner->IsPendingKill())
    {
        Detach();
        DetachChilds();
    }
    else
    {
        // Detach only other actors
        for (ASceneComponent** childIt = m_Children.Begin(); childIt != m_Children.End();)
        {
            ASceneComponent* child = *childIt;
            if (child->GetOwnerActor() != owner)
            {
                child->Detach(true);
            }
            else
            {
                childIt++;
            }
        }
    }

    if (owner->GetRootComponent() == this)
    {
        owner->ResetRootComponent();
    }
}

void ASceneComponent::AttachTo(ASceneComponent* _Parent, AStringView _Socket, bool _KeepWorldTransform)
{
    _AttachTo(_Parent, _KeepWorldTransform);

    if (!_Socket.IsEmpty() && m_AttachParent)
    {
        int socketIndex = m_AttachParent->FindSocket(_Socket);
        if (m_SocketIndex != socketIndex)
        {
            m_SocketIndex = socketIndex;
            MarkTransformDirty();
        }
    }
}

void ASceneComponent::_AttachTo(ASceneComponent* _Parent, bool _KeepWorldTransform)
{
    if (m_AttachParent == _Parent)
    {
        // Already attached
        return;
    }

    if (_Parent == this)
    {
        LOG("ASceneComponent::Attach: Parent and child are same objects\n");
        return;
    }

    if (!_Parent)
    {
        // No parent
        Detach(_KeepWorldTransform);
        return;
    }

#if 0
    if (_Parent->GetParentActor() != GetParentActor())
    {
        LOG("ASceneComponent::Attach: Parent and child are in different actors\n");
        return;
    }
#endif

    if (IsChild(_Parent, true))
    {
        // Object have desired parent in childs
        LOG("ASceneComponent::Attach: Recursive attachment\n");
        return;
    }

    Float3 worldPosition = GetWorldPosition();
    Quat   worldRotation = GetWorldRotation();
    Float3 worldScale    = GetWorldScale();

    if (m_AttachParent)
    {
        auto it = std::find(m_AttachParent->m_Children.Begin(), m_AttachParent->m_Children.End(), this);
        if (it != m_AttachParent->m_Children.End())
        {
            m_AttachParent->m_Children.Erase(it);
        }
    }

    _Parent->m_Children.Add(this);
    m_AttachParent = _Parent;

    if (_KeepWorldTransform)
    {
        SetWorldTransform(worldPosition, worldRotation, worldScale);
    }
    else
    {
        MarkTransformDirty();
    }
}

void ASceneComponent::Detach(bool _KeepWorldTransform)
{
    if (!m_AttachParent)
    {
        return;
    }

    Float3 worldPosition = GetWorldPosition();
    Quat   worldRotation = GetWorldRotation();
    Float3 worldScale    = GetWorldScale();

    auto it = std::find(m_AttachParent->m_Children.Begin(), m_AttachParent->m_Children.End(), this);
    if (it != m_AttachParent->m_Children.End())
    {
        m_AttachParent->m_Children.Erase(it);
    }
    m_AttachParent = nullptr;
    m_SocketIndex  = -1;

    if (!IsPendingKill())
    {
        if (_KeepWorldTransform)
        {
            SetWorldTransform(worldPosition, worldRotation, worldScale);
        }
        else
        {
            MarkTransformDirty();
        }
    }
}

void ASceneComponent::DetachChilds(bool _bRecursive, bool _KeepWorldTransform)
{
    while (!m_Children.IsEmpty())
    {
        ASceneComponent* child = m_Children.Last();
        child->Detach(_KeepWorldTransform);
        if (_bRecursive)
        {
            child->DetachChilds(true, _KeepWorldTransform);
        }
    }
}

bool ASceneComponent::IsChild(ASceneComponent* _Child, bool _Recursive) const
{
    for (ASceneComponent* child : m_Children)
    {
        if (child == _Child || (_Recursive && child->IsChild(_Child, true)))
        {
            return true;
        }
    }
    return false;
}

bool ASceneComponent::IsRoot() const
{
    AActor* owner = GetOwnerActor();
    return owner && owner->GetRootComponent() == this;
}

ASceneComponent* ASceneComponent::FindChild(AStringView _UniqueName, bool _Recursive)
{
    for (ASceneComponent* child : m_Children)
    {
        if (!child->GetObjectName().Icmp(_UniqueName))
        {
            return child;
        }
    }

    if (_Recursive)
    {
        for (ASceneComponent* child : m_Children)
        {
            ASceneComponent* rec = child->FindChild(_UniqueName, true);
            if (rec)
            {
                return rec;
            }
        }
    }
    return nullptr;
}

int ASceneComponent::FindSocket(AStringView _Name) const
{
    for (int socketIndex = 0; socketIndex < m_Sockets.Size(); socketIndex++)
    {
        if (!m_Sockets[socketIndex].SocketDef->Name.Icmp(_Name))
        {
            return socketIndex;
        }
    }
    LOG("Socket not found {}\n", _Name);
    return -1;
}

void ASceneComponent::MarkTransformDirty()
{
    ASceneComponent* node = this;
    ASceneComponent* nextNode;
    int              numChilds;

    while (1)
    {

        if (node->m_bTransformDirty)
        {
            return;
        }

        node->m_bTransformDirty = true;

#ifdef FUTURE
        // TODO: здесь можно инициировать событие "On Mark Dirty"
        int NumListeners = node->Listeners.Length();
        for (int i = 0; i < NumListeners;)
        {
            AActorComponent* Component = node->Listeners.ToPtr()[i];
            if (Component)
            {
                Component->OnTransformDirty(node);
                ++i;
            }
            else
            {
                node->Listeners.RemoveSwap(i);
                --NumListeners;
            }
        }
#endif
        node->OnTransformDirty();

        numChilds = node->m_Children.Size();
        if (numChilds > 0)
        {

            nextNode = node->m_Children[0];

            for (int i = 1; i < numChilds; i++)
            {
                node->m_Children[i]->MarkTransformDirty();
            }

            node = nextNode;
        }
        else
        {
            return;
        }
    }
}

void ASceneComponent::SetAbsolutePosition(bool _AbsolutePosition)
{
    if (m_bAbsolutePosition != _AbsolutePosition)
    {
        m_bAbsolutePosition = _AbsolutePosition;

        MarkTransformDirty();
    }
}

void ASceneComponent::SetAbsoluteRotation(bool _AbsoluteRotation)
{
    if (m_bAbsoluteRotation != _AbsoluteRotation)
    {
        m_bAbsoluteRotation = _AbsoluteRotation;

        MarkTransformDirty();
    }
}

void ASceneComponent::SetAbsoluteScale(bool _AbsoluteScale)
{
    if (m_bAbsoluteScale != _AbsoluteScale)
    {
        m_bAbsoluteScale = _AbsoluteScale;

        MarkTransformDirty();
    }
}

void ASceneComponent::SetPosition(Float3 const& _Position)
{
    m_Position = _Position;

    MarkTransformDirty();
}

void ASceneComponent::SetPosition(float _X, float _Y, float _Z)
{
    m_Position.X = _X;
    m_Position.Y = _Y;
    m_Position.Z = _Z;

    MarkTransformDirty();
}

void ASceneComponent::SetRotation(Quat const& _Rotation)
{
    m_Rotation = _Rotation;

    MarkTransformDirty();
}

void ASceneComponent::SetAngles(Angl const& _Angles)
{
    m_Rotation = _Angles.ToQuat();

    MarkTransformDirty();
}

void ASceneComponent::SetAngles(float _Pitch, float _Yaw, float _Roll)
{
    m_Rotation = Angl(_Pitch, _Yaw, _Roll).ToQuat();

    MarkTransformDirty();
}

void ASceneComponent::SetScale(Float3 const& _Scale)
{
    m_Scale = _Scale;

    MarkTransformDirty();
}

void ASceneComponent::SetScale(float _X, float _Y, float _Z)
{
    m_Scale.X = _X;
    m_Scale.Y = _Y;
    m_Scale.Z = _Z;

    MarkTransformDirty();
}

void ASceneComponent::SetScale(float _ScaleXYZ)
{
    m_Scale.X = m_Scale.Y = m_Scale.Z = _ScaleXYZ;

    MarkTransformDirty();
}

void ASceneComponent::SetTransform(Float3 const& _Position, Quat const& _Rotation)
{
    m_Position = _Position;
    m_Rotation = _Rotation;

    MarkTransformDirty();
}

void ASceneComponent::SetTransform(Float3 const& _Position, Quat const& _Rotation, Float3 const& _Scale)
{
    m_Position = _Position;
    m_Rotation = _Rotation;
    m_Scale    = _Scale;

    MarkTransformDirty();
}

void ASceneComponent::SetTransform(STransform const& _Transform)
{
    SetTransform(_Transform.Position, _Transform.Rotation, _Transform.Scale);
}

void ASceneComponent::SetTransform(ASceneComponent const* _Transform)
{
    m_Position = _Transform->m_Position;
    m_Rotation = _Transform->m_Rotation;
    m_Scale    = _Transform->m_Scale;

    MarkTransformDirty();
}

void ASceneComponent::SetWorldPosition(Float3 const& _Position)
{
    if (m_AttachParent && !m_bAbsolutePosition)
    {
        Float3x4 ParentTransformInverse = m_AttachParent->ComputeWorldTransformInverse();

        SetPosition(ParentTransformInverse * _Position); // TODO: check
    }
    else
    {
        SetPosition(_Position);
    }
}

void ASceneComponent::SetWorldPosition(float _X, float _Y, float _Z)
{
    SetWorldPosition(Float3(_X, _Y, _Z));
}

void ASceneComponent::SetWorldRotation(Quat const& _Rotation)
{
    SetRotation(m_AttachParent && !m_bAbsoluteRotation ? m_AttachParent->ComputeWorldRotationInverse() * _Rotation : _Rotation);
}

void ASceneComponent::SetWorldScale(Float3 const& _Scale)
{
    SetScale(m_AttachParent && !m_bAbsoluteScale ? _Scale / m_AttachParent->GetWorldScale() : _Scale);
}

void ASceneComponent::SetWorldScale(float _X, float _Y, float _Z)
{
    SetWorldScale(Float3(_X, _Y, _Z));
}

void ASceneComponent::SetWorldTransform(Float3 const& _Position, Quat const& _Rotation)
{
    if (m_AttachParent)
    {
        m_Position = m_bAbsolutePosition ? _Position : m_AttachParent->ComputeWorldTransformInverse() * _Position; // TODO: check
        m_Rotation = m_bAbsoluteRotation ? _Rotation : m_AttachParent->ComputeWorldRotationInverse() * _Rotation;
    }
    else
    {
        m_Position = _Position;
        m_Rotation = _Rotation;
    }

    MarkTransformDirty();
}

void ASceneComponent::SetWorldTransform(Float3 const& _Position, Quat const& _Rotation, Float3 const& _Scale)
{
    if (m_AttachParent)
    {
        m_Position = m_bAbsolutePosition ? _Position : m_AttachParent->ComputeWorldTransformInverse() * _Position; // TODO: check
        m_Rotation = m_bAbsoluteRotation ? _Rotation : m_AttachParent->ComputeWorldRotationInverse() * _Rotation;
        m_Scale    = m_bAbsoluteScale ? _Scale : _Scale / m_AttachParent->GetWorldScale();
    }
    else
    {
        m_Position = _Position;
        m_Rotation = _Rotation;
        m_Scale    = _Scale;
    }

    MarkTransformDirty();
}

void ASceneComponent::SetWorldTransform(STransform const& _Transform)
{
    SetWorldTransform(_Transform.Position, _Transform.Rotation, _Transform.Scale);
}

Float3 const& ASceneComponent::GetPosition() const
{
    return m_Position;
}

Quat const& ASceneComponent::GetRotation() const
{
    return m_Rotation;
}

Angl ASceneComponent::GetAngles() const
{
    Angl Angles;
    m_Rotation.ToAngles(Angles.Pitch, Angles.Yaw, Angles.Roll);
    Angles.Pitch = Math::Degrees(Angles.Pitch);
    Angles.Yaw   = Math::Degrees(Angles.Yaw);
    Angles.Roll  = Math::Degrees(Angles.Roll);
    return Angles;
}

float ASceneComponent::GetPitch() const
{
    return Math::Degrees(m_Rotation.Pitch());
}

float ASceneComponent::GetYaw() const
{
    return Math::Degrees(m_Rotation.Yaw());
}

float ASceneComponent::GetRoll() const
{
    return Math::Degrees(m_Rotation.Roll());
}

Float3 ASceneComponent::GetRightVector() const
{
    return m_Rotation.XAxis();
}

Float3 ASceneComponent::GetLeftVector() const
{
    return -m_Rotation.XAxis();
}

Float3 ASceneComponent::GetUpVector() const
{
    return m_Rotation.YAxis();
}

Float3 ASceneComponent::GetDownVector() const
{
    return -m_Rotation.YAxis();
}

Float3 ASceneComponent::GetBackVector() const
{
    return m_Rotation.ZAxis();
}

Float3 ASceneComponent::GetForwardVector() const
{
    return -m_Rotation.ZAxis();
}

void ASceneComponent::GetVectors(Float3* _Right, Float3* _Up, Float3* _Back) const
{
    float qxx(m_Rotation.X * m_Rotation.X);
    float qyy(m_Rotation.Y * m_Rotation.Y);
    float qzz(m_Rotation.Z * m_Rotation.Z);
    float qxz(m_Rotation.X * m_Rotation.Z);
    float qxy(m_Rotation.X * m_Rotation.Y);
    float qyz(m_Rotation.Y * m_Rotation.Z);
    float qwx(m_Rotation.W * m_Rotation.X);
    float qwy(m_Rotation.W * m_Rotation.Y);
    float qwz(m_Rotation.W * m_Rotation.Z);

    if (_Right)
    {
        _Right->X = 1 - 2 * (qyy + qzz);
        _Right->Y = 2 * (qxy + qwz);
        _Right->Z = 2 * (qxz - qwy);
    }

    if (_Up)
    {
        _Up->X = 2 * (qxy - qwz);
        _Up->Y = 1 - 2 * (qxx + qzz);
        _Up->Z = 2 * (qyz + qwx);
    }

    if (_Back)
    {
        _Back->X = 2 * (qxz + qwy);
        _Back->Y = 2 * (qyz - qwx);
        _Back->Z = 1 - 2 * (qxx + qyy);
    }
}

Float3 ASceneComponent::GetWorldRightVector() const
{
    return GetWorldRotation().XAxis();
}

Float3 ASceneComponent::GetWorldLeftVector() const
{
    return -GetWorldRotation().XAxis();
}

Float3 ASceneComponent::GetWorldUpVector() const
{
    return GetWorldRotation().YAxis();
}

Float3 ASceneComponent::GetWorldDownVector() const
{
    return -GetWorldRotation().YAxis();
}

Float3 ASceneComponent::GetWorldBackVector() const
{
    return GetWorldRotation().ZAxis();
}

Float3 ASceneComponent::GetWorldForwardVector() const
{
    return -GetWorldRotation().ZAxis();
}

void ASceneComponent::GetWorldVectors(Float3* _Right, Float3* _Up, Float3* _Back) const
{
    const Quat& R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qyy(R.Y * R.Y);
    float qzz(R.Z * R.Z);
    float qxz(R.X * R.Z);
    float qxy(R.X * R.Y);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwy(R.W * R.Y);
    float qwz(R.W * R.Z);

    if (_Right)
    {
        _Right->X = 1 - 2 * (qyy + qzz);
        _Right->Y = 2 * (qxy + qwz);
        _Right->Z = 2 * (qxz - qwy);
    }

    if (_Up)
    {
        _Up->X = 2 * (qxy - qwz);
        _Up->Y = 1 - 2 * (qxx + qzz);
        _Up->Z = 2 * (qyz + qwx);
    }

    if (_Back)
    {
        _Back->X = 2 * (qxz + qwy);
        _Back->Y = 2 * (qyz - qwx);
        _Back->Z = 1 - 2 * (qxx + qyy);
    }
}

Float3 const& ASceneComponent::GetScale() const
{
    return m_Scale;
}

Float3 ASceneComponent::GetWorldPosition() const
{
    if (m_bTransformDirty)
    {
        ComputeWorldTransform();
    }

    return m_WorldTransformMatrix.DecomposeTranslation();
}

Quat const& ASceneComponent::GetWorldRotation() const
{
    if (m_bTransformDirty)
    {
        ComputeWorldTransform();
    }

    return m_WorldRotation;
}

Float3 ASceneComponent::GetWorldScale() const
{
    if (m_bTransformDirty)
    {
        ComputeWorldTransform();
    }

    return m_WorldTransformMatrix.DecomposeScale();
}

Float3x4 const& ASceneComponent::GetWorldTransformMatrix() const
{
    if (m_bTransformDirty)
    {
        ComputeWorldTransform();
    }

    return m_WorldTransformMatrix;
}

void ASceneComponent::ComputeLocalTransformMatrix(Float3x4& _LocalTransformMatrix) const
{
    _LocalTransformMatrix.Compose(m_Position, m_Rotation.ToMatrix3x3(), m_Scale);
}

Float3x4 SceneSocket::EvaluateTransform() const
{
    Float3x4 transform;

    if (SkinnedMesh)
    {
        Float3x4 const& jointTransform = SkinnedMesh->GetJointTransform(SocketDef->JointIndex);

        Quat jointRotation;
        jointRotation.FromMatrix(jointTransform.DecomposeRotation());

        Float3 jointScale = jointTransform.DecomposeScale();

        Quat worldRotation = jointRotation * SocketDef->Rotation;

        transform.Compose(jointTransform * SocketDef->Position, worldRotation.ToMatrix3x3(), SocketDef->Scale * jointScale);
    }
    else
    {
        transform.Compose(SocketDef->Position, SocketDef->Rotation.ToMatrix3x3(), SocketDef->Scale);
    }

    return transform;
}

Float3x4 ASceneComponent::GetSocketTransform(int _SocketIndex) const
{
    if (m_SocketIndex < 0 || m_SocketIndex >= m_AttachParent->m_Sockets.Size())
    {
        return Float3x4::Identity();
    }

    return m_Sockets[_SocketIndex].EvaluateTransform();
}

void ASceneComponent::ComputeWorldTransform() const
{
    // TODO: optimize

    if (m_AttachParent)
    {
        if (m_SocketIndex >= 0 && m_SocketIndex < m_AttachParent->m_Sockets.Size())
        {

            Float3x4 const& SocketTransform = m_AttachParent->m_Sockets[m_SocketIndex].EvaluateTransform();

            Quat SocketRotation;
            SocketRotation.FromMatrix(SocketTransform.DecomposeRotation());

            m_WorldRotation = m_bAbsoluteRotation ? m_Rotation : m_AttachParent->GetWorldRotation() * SocketRotation * m_Rotation;

#if 0
            // Transform with shrinking

            Float3x4 LocalTransformMatrix;
            ComputeLocalTransformMatrix( LocalTransformMatrix );
            m_WorldTransformMatrix = m_AttachParent->GetWorldTransformMatrix() * SocketTransform * LocalTransformMatrix;
#else
            // Take relative to parent position and rotation. Position is scaled by parent.
            m_WorldTransformMatrix.Compose(m_bAbsolutePosition ? m_Position : m_AttachParent->GetWorldTransformMatrix() * SocketTransform * m_Position,
                                           m_WorldRotation.ToMatrix3x3(),
                                           m_bAbsoluteScale ? m_Scale : m_Scale * m_AttachParent->GetWorldScale() * SocketTransform.DecomposeScale());
#endif
        }
        else
        {

            m_WorldRotation = m_bAbsoluteRotation ? m_Rotation : m_AttachParent->GetWorldRotation() * m_Rotation;

#if 0
            // Transform with shrinking

            Float3x4 LocalTransformMatrix;
            ComputeLocalTransformMatrix( LocalTransformMatrix );
            m_WorldTransformMatrix = m_AttachParent->GetWorldTransformMatrix() * LocalTransformMatrix;
#else
#    if 0
            // Take only relative to parent position and rotation. Position is not scaled by parent.
            Float3x4 positionAndRotation;
            positionAndRotation.Compose( m_AttachParent->GetWorldPosition(), m_AttachParent->GetWorldRotation().ToMatrix3x3() );

            m_WorldTransformMatrix.Compose( positionAndRotation * m_Position, m_WorldRotation.ToMatrix3x3(), m_Scale );
#    else
            // Take relative to parent position and rotation. Position is scaled by parent.
            m_WorldTransformMatrix.Compose(m_bAbsolutePosition ? m_Position : m_AttachParent->GetWorldTransformMatrix() * m_Position,
                                           m_WorldRotation.ToMatrix3x3(),
                                           m_bAbsoluteScale ? m_Scale : m_Scale * m_AttachParent->GetWorldScale());
#    endif
#endif
        }
    }
    else
    {
        ComputeLocalTransformMatrix(m_WorldTransformMatrix);
        m_WorldRotation = m_Rotation;
    }

    m_bTransformDirty = false;
}

Float3x4 ASceneComponent::ComputeWorldTransformInverse() const
{
    Float3x4 const& WorldTransform = GetWorldTransformMatrix();

    return WorldTransform.Inversed();
}

Quat ASceneComponent::ComputeWorldRotationInverse() const
{
    return GetWorldRotation().Inversed();
}

void ASceneComponent::TurnRightFPS(float _DeltaAngleRad)
{
    TurnLeftFPS(-_DeltaAngleRad);
}

void ASceneComponent::TurnLeftFPS(float _DeltaAngleRad)
{
    TurnAroundAxis(_DeltaAngleRad, Float3(0, 1, 0));
}

void ASceneComponent::TurnUpFPS(float _DeltaAngleRad)
{
    TurnAroundAxis(_DeltaAngleRad, GetRightVector());
}

void ASceneComponent::TurnDownFPS(float _DeltaAngleRad)
{
    TurnUpFPS(-_DeltaAngleRad);
}

void ASceneComponent::TurnAroundAxis(float _DeltaAngleRad, Float3 const& _NormalizedAxis)
{
    float s, c;

    Math::SinCos(_DeltaAngleRad * 0.5f, s, c);

    m_Rotation = Quat(c, s * _NormalizedAxis.X, s * _NormalizedAxis.Y, s * _NormalizedAxis.Z) * m_Rotation;
    m_Rotation.NormalizeSelf();

    MarkTransformDirty();
}

void ASceneComponent::TurnAroundVector(float _DeltaAngleRad, Float3 const& _Vector)
{
    TurnAroundAxis(_DeltaAngleRad, _Vector.Normalized());
}

void ASceneComponent::StepRight(float _Units)
{
    Step(GetRightVector() * _Units);
}

void ASceneComponent::StepLeft(float _Units)
{
    Step(GetLeftVector() * _Units);
}

void ASceneComponent::StepUp(float _Units)
{
    Step(GetUpVector() * _Units);
}

void ASceneComponent::StepDown(float _Units)
{
    Step(GetDownVector() * _Units);
}

void ASceneComponent::StepBack(float _Units)
{
    Step(GetBackVector() * _Units);
}

void ASceneComponent::StepForward(float _Units)
{
    Step(GetForwardVector() * _Units);
}

void ASceneComponent::Step(Float3 const& _Vector)
{
    m_Position += _Vector;

    MarkTransformDirty();
}

void ASceneComponent::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    // Draw sockets
    if (com_DrawSockets)
    {
        Float3x4 transform;
        Float3   worldScale;
        Quat     worldRotation;
        Quat     r;
        for (SceneSocket& socket : m_Sockets)
        {
            transform = socket.EvaluateTransform();

#if 1
            Float3x4 const& worldTransform = GetWorldTransformMatrix();
            worldRotation.FromMatrix(worldTransform.DecomposeRotation());
            worldScale = worldTransform.DecomposeScale();
            r.FromMatrix(transform.DecomposeRotation());
            worldRotation = worldRotation * r;
            transform.Compose(worldTransform * transform.DecomposeTranslation(),
                              worldRotation.ToMatrix3x3(),
                              transform.DecomposeScale() * worldScale);
            InRenderer->DrawAxis(transform, true);
#else
            InRenderer->DrawAxis(GetWorldTransformMatrix() * transform, true);
#endif
        }
    }
}
