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

#include <World/Public/Components/SceneComponent.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/World.h>
#include <Core/Public/Logger.h>
#include <Core/Public/BV/BvIntersect.h>
#include <Runtime/Public/RuntimeVariable.h>

#include <algorithm>    // find

ARuntimeVariable com_DrawSockets( _CTS( "com_DrawSockets" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ASceneComponent )

ASceneComponent::ASceneComponent()
    : Position( 0 )
    , Rotation( 1,0,0,0 )
    , Scale( 1 )
    , bTransformDirty( true )
    , AttachParent( nullptr )
    , SocketIndex( 0 )
    , bAbsolutePosition( false )
    , bAbsoluteRotation( false )
    , bAbsoluteScale( false )
{
}

void ASceneComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    if ( GetParentActor() && !GetParentActor()->IsPendingKill() ) {
        Detach();
        DetachChilds();
    }

    if ( GetParentActor() ) {
        if ( GetParentActor()->RootComponent == this ) {
            GetParentActor()->RootComponent = nullptr;
        }
    }
}

void ASceneComponent::AttachTo( ASceneComponent * _Parent, const char * _Socket, bool _KeepWorldTransform ) {
    _AttachTo( _Parent, _KeepWorldTransform );

    if ( _Socket && AttachParent ) {
        int socketIndex = AttachParent->FindSocket( _Socket );
        if ( SocketIndex != socketIndex ) {
            SocketIndex = socketIndex;
            MarkTransformDirty();
        }
    }
}

void ASceneComponent::_AttachTo( ASceneComponent * _Parent, bool _KeepWorldTransform ) {
    if ( IsSame( AttachParent, _Parent ) ) {
        // Already attached
        return;
    }

    if ( _Parent == this ) {
        GLogger.Printf( "ASceneComponent::Attach: Parent and child are same objects\n" );
        return;
    }

    if ( !_Parent ) {
        // No parent
        Detach( _KeepWorldTransform );
        return;
    }

    if ( !IsSame( _Parent->GetParentActor(), GetParentActor() ) ) {
        GLogger.Printf( "ASceneComponent::Attach: Parent and child are in different actors\n" );
        return;
    }

    if ( IsChild( _Parent, true ) ) {
        // Object have desired parent in childs
        GLogger.Printf( "ASceneComponent::Attach: Recursive attachment\n" );
        return;
    }

    if ( AttachParent ) {
        auto it = StdFind( AttachParent->Childs.Begin(), AttachParent->Childs.End(), this );
        if ( it != AttachParent->Childs.End() ) {
            AttachParent->Childs.Erase( it );

            if ( _KeepWorldTransform ) {
#ifdef FUTURE
                SetTransform( AttachParent->GetWorldTransformMatrix() * GetPosition(),
                              AttachParent->GetWorldRotation() * GetRotation(),
                              AttachParent->GetWorldScale() * GetScale() );
#endif
            }
        }
    }

    _Parent->Childs.Append( this );
    AttachParent = _Parent;

    if ( _KeepWorldTransform ) {
#ifdef FUTURE
        SetTransform( AttachParent->ComputeWorldTransformInverse() * GetPosition(),
                      AttachParent->ComputeWorldRotationInverse() * GetRotation(),
                      GetScale() / AttachParent->GetWorldScale() );
#endif
    } else {
        MarkTransformDirty();
    }
}

void ASceneComponent::Detach( bool _KeepWorldTransform ) {
    if ( !AttachParent ) {
        return;
    }
    auto it = StdFind( AttachParent->Childs.Begin(), AttachParent->Childs.End(), this );
    if ( it != AttachParent->Childs.End() ) {
        AttachParent->Childs.Erase( it );

        if ( _KeepWorldTransform ) {
#ifdef FUTURE
            SetTransform( AttachParent->GetWorldTransformMatrix() * GetPosition(),
                          AttachParent->GetWorldRotation() * GetRotation(),
                          AttachParent->GetWorldScale() * GetScale() );
#endif
        }
    }
    AttachParent = nullptr;
    SocketIndex = -1;
    MarkTransformDirty();
}

void ASceneComponent::DetachChilds( bool _bRecursive, bool _KeepWorldTransform ) {
    while ( !Childs.IsEmpty() ) {
        ASceneComponent * child = Childs.Last();
        child->Detach( _KeepWorldTransform );
        if ( _bRecursive ) {
            child->DetachChilds( true, _KeepWorldTransform );
        }
    }
}

bool ASceneComponent::IsChild( ASceneComponent * _Child, bool _Recursive ) const {
    for ( ASceneComponent * child : Childs ) {
        if ( IsSame( child, _Child ) || ( _Recursive && child->IsChild( _Child, true ) ) ) {
            return true;
        }
    }
    return false;
}

bool ASceneComponent::IsRoot() const {
    return GetParentActor()->RootComponent == this;
}

ASceneComponent * ASceneComponent::FindChild( const char * _UniqueName, bool _Recursive ) {
    for ( ASceneComponent * child : Childs ) {
        if ( !child->GetObjectName().Icmp( _UniqueName ) ) {
            return child;
        }
    }

    if ( _Recursive ) {
        for ( ASceneComponent * child : Childs ) {
            ASceneComponent * rec = child->FindChild( _UniqueName, true );
            if ( rec ) {
                return rec;
            }
        }
    }
    return nullptr;
}

int ASceneComponent::FindSocket( const char * _Name ) const {
    for ( int socketIndex = 0 ; socketIndex < Sockets.Size() ; socketIndex++ ) {
        if ( !Sockets[socketIndex].SocketDef->GetObjectName().Icmp( _Name ) ) {
            return socketIndex;
        }
    }
    GLogger.Printf( "Socket not found %s\n", _Name );
    return -1;
}

void ASceneComponent::MarkTransformDirty() {
    ASceneComponent * node = this;
    ASceneComponent * nextNode;
    int numChilds;

    while ( 1 ) {

        if ( node->bTransformDirty ) {
            return;
        }

        node->bTransformDirty = true;

#ifdef FUTURE
        // TODO: здесь можно инициировать событие "On Mark Dirty"
        int NumListeners = node->Listeners.Length();
        for ( int i = 0 ; i < NumListeners ; ) {
            AActorComponent * Component = node->Listeners.ToPtr()[ i ];
            if ( Component ) {
                Component->OnTransformDirty( node );
                ++i;
            } else {
                node->Listeners.RemoveSwap( i );
                --NumListeners;
            }
        }
#endif
        node->OnTransformDirty();

        numChilds = node->Childs.Size();
        if ( numChilds > 0 ) {

            nextNode = node->Childs[ 0 ];

            for ( int i = 1 ; i < numChilds ; i++ ) {
                node->Childs[ i ]->MarkTransformDirty();
            }

            node = nextNode;
        } else {
            return;
        }
    }
}

void ASceneComponent::SetAbsolutePosition( bool _AbsolutePosition ) {
    if ( bAbsolutePosition != _AbsolutePosition ) {
        bAbsolutePosition = _AbsolutePosition;

        MarkTransformDirty();
    }
}

void ASceneComponent::SetAbsoluteRotation( bool _AbsoluteRotation ) {
    if ( bAbsoluteRotation != _AbsoluteRotation ) {
        bAbsoluteRotation = _AbsoluteRotation;

        MarkTransformDirty();
    }
}

void ASceneComponent::SetAbsoluteScale( bool _AbsoluteScale ) {
    if ( bAbsoluteScale != _AbsoluteScale ) {
        bAbsoluteScale = _AbsoluteScale;

        MarkTransformDirty();
    }
}

void ASceneComponent::SetPosition( Float3 const & _Position ) {

    Position = _Position;

    MarkTransformDirty();
}

void ASceneComponent::SetPosition( float const & _X, float const & _Y, float const & _Z ) {

    Position.X = _X;
    Position.Y = _Y;
    Position.Z = _Z;

    MarkTransformDirty();
}

void ASceneComponent::SetRotation( Quat const & _Rotation ) {

    Rotation = _Rotation;

    MarkTransformDirty();
}

void ASceneComponent::SetAngles( Angl const & _Angles ) {
    Rotation = _Angles.ToQuat();

    MarkTransformDirty();
}

void ASceneComponent::SetAngles( float const & _Pitch, float const & _Yaw, float const & _Roll ) {
    Rotation = Angl( _Pitch, _Yaw, _Roll ).ToQuat();

    MarkTransformDirty();
}

void ASceneComponent::SetScale( Float3 const & _Scale ) {

    Scale = _Scale;

    MarkTransformDirty();
}

void ASceneComponent::SetScale( float const & _X, float const & _Y, float const & _Z ) {

    Scale.X = _X;
    Scale.Y = _Y;
    Scale.Z = _Z;

    MarkTransformDirty();
}

void ASceneComponent::SetScale( float const & _ScaleXYZ ) {

    Scale.X = Scale.Y = Scale.Z = _ScaleXYZ;

    MarkTransformDirty();
}

void ASceneComponent::SetTransform( Float3 const & _Position, Quat const & _Rotation ) {

    Position = _Position;
    Rotation = _Rotation;

    MarkTransformDirty();
}

void ASceneComponent::SetTransform( Float3 const & _Position, Quat const & _Rotation, Float3 const & _Scale ) {

    Position = _Position;
    Rotation = _Rotation;
    Scale = _Scale;

    MarkTransformDirty();
}

void ASceneComponent::SetTransform( STransform const & _Transform ) {
    SetTransform( _Transform.Position, _Transform.Rotation, _Transform.Scale );
}

void ASceneComponent::SetTransform( ASceneComponent const * _Transform ) {

    Position = _Transform->Position;
    Rotation = _Transform->Rotation;
    Scale = _Transform->Scale;

    MarkTransformDirty();
}

void ASceneComponent::SetWorldPosition( Float3 const & _Position ) {
    if ( AttachParent ) {
        Float3x4 ParentTransformInverse = AttachParent->ComputeWorldTransformInverse();

        SetPosition( ParentTransformInverse * _Position );  // TODO: check
    } else {
        SetPosition( _Position );
    }
}

void ASceneComponent::SetWorldPosition( float const & _X, float const & _Y, float const & _Z ) {
    SetWorldPosition( Float3( _X, _Y, _Z ) );
}

void ASceneComponent::SetWorldRotation( Quat const & _Rotation ) {
    SetRotation( AttachParent ? AttachParent->ComputeWorldRotationInverse() * _Rotation : _Rotation );
}

void ASceneComponent::SetWorldScale( Float3 const & _Scale ) {
    SetScale( AttachParent ? _Scale / AttachParent->GetWorldScale() : _Scale );
}

void ASceneComponent::SetWorldScale( float const & _X, float const & _Y, float const & _Z ) {
    SetWorldScale( Float3( _X, _Y, _Z ) );
}

void ASceneComponent::SetWorldTransform( Float3 const & _Position, Quat const & _Rotation ) {

    if ( AttachParent ) {
        Float3x4 ParentTransformInverse = AttachParent->ComputeWorldTransformInverse();

        Position = ParentTransformInverse * _Position;  // TODO: check
        Rotation = AttachParent->ComputeWorldRotationInverse() * _Rotation;
    } else {
        Position = _Position;
        Rotation = _Rotation;
    }

    MarkTransformDirty();

}

void ASceneComponent::SetWorldTransform( Float3 const & _Position, Quat const & _Rotation, Float3 const & _Scale ) {

    if ( AttachParent ) {
        Float3x4 ParentTransformInverse = AttachParent->ComputeWorldTransformInverse();

        Position = ParentTransformInverse * _Position;  // TODO: check
        Rotation = AttachParent->ComputeWorldRotationInverse() * _Rotation;
        Scale =  _Scale / AttachParent->GetWorldScale();
    } else {
        Position = _Position;
        Rotation = _Rotation;
        Scale = _Scale;
    }

    MarkTransformDirty();

}

void ASceneComponent::SetWorldTransform( STransform const & _Transform ) {
    SetWorldTransform( _Transform.Position, _Transform.Rotation, _Transform.Scale );
}

Float3 const & ASceneComponent::GetPosition() const {

    return Position;

}

Quat const & ASceneComponent::GetRotation() const {

    return Rotation;

}

Angl ASceneComponent::GetAngles() const {
    Angl Angles;
    Rotation.ToAngles( Angles.Pitch, Angles.Yaw, Angles.Roll );
    Angles.Pitch = Math::Degrees( Angles.Pitch );
    Angles.Yaw = Math::Degrees( Angles.Yaw );
    Angles.Roll = Math::Degrees( Angles.Roll );
    return Angles;
}

float ASceneComponent::GetPitch() const {
    return Math::Degrees( Rotation.Pitch() );
}

float ASceneComponent::GetYaw() const {
    return Math::Degrees( Rotation.Yaw() );
}

float ASceneComponent::GetRoll() const {
    return Math::Degrees( Rotation.Roll() );
}

Float3 ASceneComponent::GetRightVector() const {
    //return Math::ToMat3(Rotation)[0];

    float qyy(Rotation.Y * Rotation.Y);
    float qzz(Rotation.Z * Rotation.Z);
    float qxz(Rotation.X * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qwy(Rotation.W * Rotation.Y);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( 1 - 2 * (qyy +  qzz), 2 * (qxy + qwz), 2 * (qxz - qwy) );
}

Float3 ASceneComponent::GetLeftVector() const {
    //return -Math::ToMat3(Rotation)[0];

    float qyy(Rotation.Y * Rotation.Y);
    float qzz(Rotation.Z * Rotation.Z);
    float qxz(Rotation.X * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qwy(Rotation.W * Rotation.Y);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( -1 + 2 * (qyy +  qzz), -2 * (qxy + qwz), -2 * (qxz - qwy) );
}

Float3 ASceneComponent::GetUpVector() const {
    //return Math::ToMat3(Rotation)[1];

    float qxx(Rotation.X * Rotation.X);
    float qzz(Rotation.Z * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( 2 * (qxy - qwz), 1 - 2 * (qxx +  qzz), 2 * (qyz + qwx) );
}

Float3 ASceneComponent::GetDownVector() const {
    //return -Math::ToMat3(Rotation)[1];

    float qxx(Rotation.X * Rotation.X);
    float qzz(Rotation.Z * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( -2 * (qxy - qwz), -1 + 2 * (qxx +  qzz), -2 * (qyz + qwx) );
}

Float3 ASceneComponent::GetBackVector() const {
    //return Math::ToMat3(Rotation)[2];

    float qxx(Rotation.X * Rotation.X);
    float qyy(Rotation.Y * Rotation.Y);
    float qxz(Rotation.X * Rotation.Z);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwy(Rotation.W * Rotation.Y);

    return Float3( 2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx +  qyy) );
}

Float3 ASceneComponent::GetForwardVector() const {
    //return -Math::ToMat3(Rotation)[2];

    float qxx(Rotation.X * Rotation.X);
    float qyy(Rotation.Y * Rotation.Y);
    float qxz(Rotation.X * Rotation.Z);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwy(Rotation.W * Rotation.Y);

    return Float3( -2 * (qxz + qwy), -2 * (qyz - qwx), -1 + 2 * (qxx +  qyy) );
}

void ASceneComponent::GetVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const {
    //if ( _Right ) *_Right = Math::ToMat3(Rotation)[0];
    //if ( _Up ) *_Up = Math::ToMat3(Rotation)[1];
    //if ( _Back ) *_Back = Math::ToMat3(Rotation)[2];
    //return;

    float qxx(Rotation.X * Rotation.X);
    float qyy(Rotation.Y * Rotation.Y);
    float qzz(Rotation.Z * Rotation.Z);
    float qxz(Rotation.X * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwy(Rotation.W * Rotation.Y);
    float qwz(Rotation.W * Rotation.Z);

    if ( _Right ) {
        _Right->X = 1 - 2 * (qyy +  qzz);
        _Right->Y = 2 * (qxy + qwz);
        _Right->Z = 2 * (qxz - qwy);
    }

    if ( _Up ) {
        _Up->X = 2 * (qxy - qwz);
        _Up->Y = 1 - 2 * (qxx +  qzz);
        _Up->Z = 2 * (qyz + qwx);
    }

    if ( _Back ) {
        _Back->X = 2 * (qxz + qwy);
        _Back->Y = 2 * (qyz - qwx);
        _Back->Z = 1 - 2 * (qxx +  qyy);
    }
}

Float3 ASceneComponent::GetWorldRightVector() const {
    const Quat & R = GetWorldRotation();

    float qyy(R.Y * R.Y);
    float qzz(R.Z * R.Z);
    float qxz(R.X * R.Z);
    float qxy(R.X * R.Y);
    float qwy(R.W * R.Y);
    float qwz(R.W * R.Z);

    return Float3( 1 - 2 * (qyy +  qzz), 2 * (qxy + qwz), 2 * (qxz - qwy) );
}

Float3 ASceneComponent::GetWorldLeftVector() const {
    const Quat & R = GetWorldRotation();

    float qyy(R.Y * R.Y);
    float qzz(R.Z * R.Z);
    float qxz(R.X * R.Z);
    float qxy(R.X * R.Y);
    float qwy(R.W * R.Y);
    float qwz(R.W * R.Z);

    return Float3( -1 + 2 * (qyy +  qzz), -2 * (qxy + qwz), -2 * (qxz - qwy) );
}

Float3 ASceneComponent::GetWorldUpVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qzz(R.Z * R.Z);
    float qxy(R.X * R.Y);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwz(R.W * R.Z);

    return Float3( 2 * (qxy - qwz), 1 - 2 * (qxx +  qzz), 2 * (qyz + qwx) );
}

Float3 ASceneComponent::GetWorldDownVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qzz(R.Z * R.Z);
    float qxy(R.X * R.Y);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwz(R.W * R.Z);

    return Float3( -2 * (qxy - qwz), -1 + 2 * (qxx +  qzz), -2 * (qyz + qwx) );
}

Float3 ASceneComponent::GetWorldBackVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qyy(R.Y * R.Y);
    float qxz(R.X * R.Z);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwy(R.W * R.Y);

    return Float3( 2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx +  qyy) );
}

Float3 ASceneComponent::GetWorldForwardVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qyy(R.Y * R.Y);
    float qxz(R.X * R.Z);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwy(R.W * R.Y);

    return Float3( -2 * (qxz + qwy), -2 * (qyz - qwx), -1 + 2 * (qxx +  qyy) );
}

void ASceneComponent::GetWorldVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qyy(R.Y * R.Y);
    float qzz(R.Z * R.Z);
    float qxz(R.X * R.Z);
    float qxy(R.X * R.Y);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwy(R.W * R.Y);
    float qwz(R.W * R.Z);

    if ( _Right ) {
        _Right->X = 1 - 2 * (qyy +  qzz);
        _Right->Y = 2 * (qxy + qwz);
        _Right->Z = 2 * (qxz - qwy);
    }

    if ( _Up ) {
        _Up->X = 2 * (qxy - qwz);
        _Up->Y = 1 - 2 * (qxx +  qzz);
        _Up->Z = 2 * (qyz + qwx);
    }

    if ( _Back ) {
        _Back->X = 2 * (qxz + qwy);
        _Back->Y = 2 * (qyz - qwx);
        _Back->Z = 1 - 2 * (qxx +  qyy);
    }
}

Float3 const & ASceneComponent::GetScale() const {

    return Scale;

}

Float3 ASceneComponent::GetWorldPosition() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldTransformMatrix.DecomposeTranslation();
}

Quat const & ASceneComponent::GetWorldRotation() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldRotation;
}

Float3 ASceneComponent::GetWorldScale() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldTransformMatrix.DecomposeScale();
}

Float3x4 const & ASceneComponent::GetWorldTransformMatrix() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldTransformMatrix;
}

void ASceneComponent::ComputeLocalTransformMatrix( Float3x4 & _LocalTransformMatrix ) const {
    _LocalTransformMatrix.Compose( Position, Rotation.ToMatrix(), Scale );
}

Float3x4 SSocket::EvaluateTransform() const {
    Float3x4 transform;

    if ( SkinnedMesh )
    {
        Float3x4 const & jointTransform = SkinnedMesh->GetJointTransform( SocketDef->JointIndex );

        Quat jointRotation;
        jointRotation.FromMatrix( jointTransform.DecomposeRotation() );

        Float3 jointScale = jointTransform.DecomposeScale();

        Quat worldRotation = jointRotation * SocketDef->Rotation;

        transform.Compose( jointTransform * SocketDef->Position, worldRotation.ToMatrix(), SocketDef->Scale * jointScale );
    }
    else
    {
        transform.Compose( SocketDef->Position, SocketDef->Rotation.ToMatrix(), SocketDef->Scale );
    }

    return transform;
}

Float3x4 ASceneComponent::GetSocketTransform( int _SocketIndex ) const {
    if ( SocketIndex < 0 || SocketIndex >= AttachParent->Sockets.Size() )
    {
        return Float3x4::Identity();
    }

    return Sockets[_SocketIndex].EvaluateTransform();
}

void ASceneComponent::ComputeWorldTransform() const {
    // TODO: optimize

    if ( AttachParent ) {
        if ( SocketIndex >= 0 && SocketIndex < AttachParent->Sockets.Size() ) {

            Float3x4 const & SocketTransform = AttachParent->Sockets[ SocketIndex ].EvaluateTransform();

            Quat SocketRotation;
            SocketRotation.FromMatrix( SocketTransform.DecomposeRotation() );

            WorldRotation = bAbsoluteRotation ? Rotation : AttachParent->GetWorldRotation() * SocketRotation * Rotation;

#if 0
            // Transform with shrinking

            Float3x4 LocalTransformMatrix;
            ComputeLocalTransformMatrix( LocalTransformMatrix );
            WorldTransformMatrix = AttachParent->GetWorldTransformMatrix() * SocketTransform * LocalTransformMatrix;
#else
            // Take relative to parent position and rotation. Position is scaled by parent.
            WorldTransformMatrix.Compose( bAbsolutePosition ? Position : AttachParent->GetWorldTransformMatrix() * SocketTransform * Position,
                                          WorldRotation.ToMatrix(),
                                          bAbsoluteScale ? Scale : Scale * AttachParent->GetWorldScale() * SocketTransform.DecomposeScale() );
#endif
            
        } else {

            WorldRotation = bAbsoluteRotation ? Rotation : AttachParent->GetWorldRotation() * Rotation;

#if 0
            // Transform with shrinking

            Float3x4 LocalTransformMatrix;
            ComputeLocalTransformMatrix( LocalTransformMatrix );
            WorldTransformMatrix = AttachParent->GetWorldTransformMatrix() * LocalTransformMatrix;
#else
#if 0
            // Take only relative to parent position and rotation. Position is not scaled by parent.
            Float3x4 positionAndRotation;
            positionAndRotation.Compose( AttachParent->GetWorldPosition(), AttachParent->GetWorldRotation().ToMatrix() );

            WorldTransformMatrix.Compose( positionAndRotation * Position, WorldRotation.ToMatrix(), Scale );
#else
            // Take relative to parent position and rotation. Position is scaled by parent.
            WorldTransformMatrix.Compose( bAbsolutePosition ? Position : AttachParent->GetWorldTransformMatrix() * Position,
                                          WorldRotation.ToMatrix(),
                                          bAbsoluteScale ? Scale : Scale * AttachParent->GetWorldScale() );
#endif
#endif
        }

    } else {
        ComputeLocalTransformMatrix( WorldTransformMatrix );
        WorldRotation = Rotation;
    }

    bTransformDirty = false;
}

Float3x4 ASceneComponent::ComputeWorldTransformInverse() const {
    Float3x4 const & WorldTransform = GetWorldTransformMatrix();

    return WorldTransform.Inversed();
}

Quat ASceneComponent::ComputeWorldRotationInverse() const {
    return GetWorldRotation().Inversed();
}

void ASceneComponent::TurnRightFPS( float _DeltaAngleRad ) {
    TurnLeftFPS( -_DeltaAngleRad );
}

void ASceneComponent::TurnLeftFPS( float _DeltaAngleRad ) {
    TurnAroundAxis( _DeltaAngleRad, Float3( 0,1,0 ) );
}

void ASceneComponent::TurnUpFPS( float _DeltaAngleRad ) {
    TurnAroundAxis( _DeltaAngleRad, GetRightVector() );
}

void ASceneComponent::TurnDownFPS( float _DeltaAngleRad ) {
    TurnUpFPS( -_DeltaAngleRad );
}

void ASceneComponent::TurnAroundAxis( float _DeltaAngleRad, Float3 const & _NormalizedAxis ) {
    float s, c;

    Math::SinCos( _DeltaAngleRad * 0.5f, s, c );

    Rotation = Quat( c, s * _NormalizedAxis.X, s * _NormalizedAxis.Y, s * _NormalizedAxis.Z ) * Rotation;

    MarkTransformDirty();
}

void ASceneComponent::TurnAroundVector( float _DeltaAngleRad, Float3 const & _Vector ) {
    TurnAroundAxis( _DeltaAngleRad, _Vector.Normalized() );
}

void ASceneComponent::StepRight( float _Units ) {
    Step( GetRightVector() * _Units );
}

void ASceneComponent::StepLeft( float _Units ) {
    Step( GetLeftVector() * _Units );
}

void ASceneComponent::StepUp( float _Units ) {
    Step( GetUpVector() * _Units );
}

void ASceneComponent::StepDown( float _Units ) {
    Step( GetDownVector() * _Units );
}

void ASceneComponent::StepBack( float _Units ) {
    Step( GetBackVector() * _Units );
}

void ASceneComponent::StepForward( float _Units ) {
    Step( GetForwardVector() * _Units );
}

void ASceneComponent::Step( Float3 const & _Vector ) {
    Position += _Vector;

    MarkTransformDirty();
}

void ASceneComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    // Draw sockets
    if ( com_DrawSockets ) {
        Float3x4 transform;
        Float3 worldScale;
        Quat worldRotation;
        Quat r;
        for ( SSocket & socket : Sockets ) {
            transform = socket.EvaluateTransform();

#if 1
            Float3x4 const & worldTransform = GetWorldTransformMatrix();
            worldRotation.FromMatrix( worldTransform.DecomposeRotation() );
            worldScale = worldTransform.DecomposeScale();
            r.FromMatrix( transform.DecomposeRotation() );
            worldRotation = worldRotation * r;
            transform.Compose( worldTransform * transform.DecomposeTranslation(),
                               worldRotation.ToMatrix(),
                               transform.DecomposeScale() * worldScale );
            InRenderer->DrawAxis( transform, true );
#else
            InRenderer->DrawAxis( GetWorldTransformMatrix() * transform, true );
#endif
        }
    }
}
