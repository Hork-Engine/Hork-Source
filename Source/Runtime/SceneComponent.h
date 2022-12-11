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

#pragma once

#include "ActorComponent.h"

#include <Geometry/Transform.h>

class SceneComponent;

using ChildComponents = TSmallVector<SceneComponent*, 8>;

class SocketDef;
class SkinnedComponent;

struct SceneSocket
{
    /** Socket resource object */
    SocketDef* SocketDef;
    /** Skinned mesh if socket is attached to joint */
    SkinnedComponent* SkinnedMesh;
    /** Evaluate socket transform */
    Float3x4 EvaluateTransform() const;
};

/**

SceneComponent

Base class for all actor components that have its position, rotation and scale

*/
class SceneComponent : public ActorComponent
{
    HK_COMPONENT(SceneComponent, ActorComponent)

public:
    /** Attach to parent component */
    void AttachTo(SceneComponent* _Parent, StringView _Socket = {}, bool _KeepWorldTransform = false);

    /** Detach from parent component */
    void Detach(bool _KeepWorldTransform = false);

    /** Detach all childs */
    void DetachChilds(bool _bRecursive = false, bool _KeepWorldTransform = false);

    /** Is component parent of specified child */
    bool IsChild(SceneComponent* _Child, bool _Recursive) const;

    /** Is component root */
    bool IsRoot() const;

    /** Find child by name */
    SceneComponent* FindChild(StringView _UniqueName, bool _Recursive);

    /** Get reference to array of child components */
    ChildComponents const& GetChildren() const { return m_Children; }

    /** Get parent component */
    SceneComponent* GetParent() const { return m_AttachParent; }

    /** Get socket index by name */
    int FindSocket(StringView _Name) const;

    /** Get socket transform matrix */
    Float3x4 GetSocketTransform(int _SocketIndex) const;

    /** Get attached socket */
    int GetAttachedSocket() const { return m_SocketIndex; }

    /** Is component attached to socket */
    bool IsAttachedToSocket() const { return m_SocketIndex >= 0; }

    /** Set ignore parent position */
    void SetAbsolutePosition(bool _AbsolutePosition);

    bool IsAbsolutePosition() const { return m_bAbsolutePosition; }

    /** Set ignore parent rotation */
    void SetAbsoluteRotation(bool _AbsoluteRotation);

    bool IsAbsoluteRotation() const { return m_bAbsoluteRotation; }

    /** Set ignore parent scale */
    void SetAbsoluteScale(bool _AbsoluteScale);

    bool IsAbsoluteScale() const { return m_bAbsoluteScale; }

    /** Set local position */
    void SetPosition(Float3 const& _Position);

    /** Set local position */
    void SetPosition(float _X, float _Y, float _Z);

    /** Set local rotation */
    void SetRotation(Quat const& _Rotation);

    /** Set local rotation */
    void SetAngles(Angl const& _Angles);

    /** Set local rotation */
    void SetAngles(float _Pitch, float _Yaw, float _Roll);

    /** Set scale */
    void SetScale(Float3 const& _Scale);

    /** Set scale */
    void SetScale(float _X, float _Y, float _Z);

    /** Set scale */
    void SetScale(float _ScaleXYZ);

    /** Set local transform */
    void SetTransform(Float3 const& _Position, Quat const& _Rotation);

    /** Set local transform */
    void SetTransform(Float3 const& _Position, Quat const& _Rotation, Float3 const& _Scale);

    /** Set local transform */
    void SetTransform(Transform const& _Transform);

    /** Set local transform */
    void SetTransform(SceneComponent const* _Transform);

    /** Set world position */
    void SetWorldPosition(Float3 const& _Position);

    /** Set world position */
    void SetWorldPosition(float _X, float _Y, float _Z);

    /** Set world rotation */
    void SetWorldRotation(Quat const& _Rotation);

    /** Set world scale */
    void SetWorldScale(Float3 const& _Scale);

    /** Set world scale */
    void SetWorldScale(float _X, float _Y, float _Z);

    /** Set world transform */
    void SetWorldTransform(Float3 const& _Position, Quat const& _Rotation);

    /** Set world transform */
    void SetWorldTransform(Float3 const& _Position, Quat const& _Rotation, Float3 const& _Scale);

    /** Set world transform */
    void SetWorldTransform(Transform const& _Transform);

    /** Get local position */
    Float3 const& GetPosition() const;

    /** Get local rotation */
    Quat const& GetRotation() const;

    Angl  GetAngles() const;
    float GetPitch() const;
    float GetYaw() const;
    float GetRoll() const;

    Float3 GetRightVector() const;
    Float3 GetLeftVector() const;
    Float3 GetUpVector() const;
    Float3 GetDownVector() const;
    Float3 GetBackVector() const;
    Float3 GetForwardVector() const;
    void   GetVectors(Float3* _Right, Float3* _Up, Float3* _Back) const;

    Float3 GetWorldRightVector() const;
    Float3 GetWorldLeftVector() const;
    Float3 GetWorldUpVector() const;
    Float3 GetWorldDownVector() const;
    Float3 GetWorldBackVector() const;
    Float3 GetWorldForwardVector() const;
    void   GetWorldVectors(Float3* _Right, Float3* _Up, Float3* _Back) const;

    /** Get scale */
    Float3 const& GetScale() const;

    /** Get world position */
    Float3 GetWorldPosition() const;

    /** Get world rotation */
    Quat const& GetWorldRotation() const;

    /** Get world scale */
    Float3 GetWorldScale() const;

    /** Mark to recompute transforms */
    void MarkTransformDirty();

    /** Compute component local transform matrix */
    void ComputeLocalTransformMatrix(Float3x4& _LocalTransformMatrix) const;

    /** Get transposed world transform matrix */
    Float3x4 const& GetWorldTransformMatrix() const;

    Float3x4 ComputeWorldTransformInverse() const;
    Quat     ComputeWorldRotationInverse() const;

    /** First person shooter rotations */
    void TurnRightFPS(float _DeltaAngleRad);
    void TurnLeftFPS(float _DeltaAngleRad);
    void TurnUpFPS(float _DeltaAngleRad);
    void TurnDownFPS(float _DeltaAngleRad);

    /** Rotations */
    void TurnAroundAxis(float _DeltaAngleRad, Float3 const& _NormalizedAxis);
    void TurnAroundVector(float _DeltaAngleRad, Float3 const& _Vector);

    /** Move */
    void StepRight(float _Units);
    void StepLeft(float _Units);
    void StepUp(float _Units);
    void StepDown(float _Units);
    void StepBack(float _Units);
    void StepForward(float _Units);
    void Step(Float3 const& _Vector);

protected:
    SceneComponent();

    void DeinitializeComponent() override;

    void DrawDebug(DebugRenderer* InRenderer) override;

    virtual void OnTransformDirty() {}

    using ArrayOfSockets = TVector<SceneSocket>;
    ArrayOfSockets m_Sockets;

private:
    void _AttachTo(SceneComponent* _Parent, bool _KeepWorldTransform);

    void ComputeWorldTransform() const;

    Float3           m_Position{0, 0, 0};
    Quat             m_Rotation{1, 0, 0, 0};
    Float3           m_Scale{1, 1, 1};
    mutable Float3x4 m_WorldTransformMatrix; // Transposed world transform matrix
    mutable Quat     m_WorldRotation{1, 0, 0, 0};
    mutable bool     m_bTransformDirty{true};
    ChildComponents  m_Children;
    SceneComponent*  m_AttachParent{nullptr};
    int              m_SocketIndex{0};
    bool             m_bAbsolutePosition : 1;
    bool             m_bAbsoluteRotation : 1;
    bool             m_bAbsoluteScale : 1;
};
