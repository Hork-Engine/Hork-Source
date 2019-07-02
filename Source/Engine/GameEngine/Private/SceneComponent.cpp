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

#include <Engine/GameEngine/Public/SceneComponent.h>
#include <Engine/GameEngine/Public/SkeletalAnimation.h>
#include <Engine/GameEngine/Public/Actor.h>
#include <Engine/GameEngine/Public/World.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/BV/BvIntersect.h>

#include <algorithm>    // find

AN_BEGIN_CLASS_META( FSceneComponent )
AN_ATTRIBUTE_( Position, AF_DEFAULT )
AN_ATTRIBUTE_( Rotation, AF_DEFAULT )
AN_ATTRIBUTE_( Scale, AF_DEFAULT )
AN_END_CLASS_META()

FSceneComponent::FSceneComponent() {
    Rotation = Quat( 1,0,0,0 );
    Scale = Float3( 1 );
    bTransformDirty = true;
}

void FSceneComponent::DeinitializeComponent() {
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

void FSceneComponent::AttachTo( FSceneComponent * _Parent, const char * _Socket, bool _KeepWorldTransform ) {
    _AttachTo( _Parent, _KeepWorldTransform );

    if ( _Socket && AttachParent ) {
        int socketIndex = AttachParent->FindSocket( _Socket );
        if ( SocketIndex != socketIndex ) {
            SocketIndex = socketIndex;
            MarkTransformDirty();
        }
    }
}

void FSceneComponent::_AttachTo( FSceneComponent * _Parent, bool _KeepWorldTransform ) {
    if ( AttachParent == _Parent ) {
        // Already attached
        return;
    }

    if ( _Parent == this ) {
        GLogger.Printf( "FSceneComponent::Attach: Parent and child are same objects\n" );
        return;
    }

    if ( !_Parent ) {
        // No parent
        Detach( _KeepWorldTransform );
        return;
    }

    if ( _Parent->GetParentActor() != GetParentActor() ) {
        GLogger.Printf( "FSceneComponent::Attach: Parent and child are in different actors\n" );
        return;
    }

    if ( IsChild( _Parent, true ) ) {
        // Object have desired parent in childs
        GLogger.Printf( "FSceneComponent::Attach: Recursive attachment\n" );
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

    GLogger.Printf( "%s attached to %s\n", FinalClassName(), _Parent->FinalClassName() );
}

void FSceneComponent::Detach( bool _KeepWorldTransform ) {
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
    GLogger.Printf( "%s detached from %s\n", FinalClassName(), AttachParent->FinalClassName() );
    AttachParent = nullptr;
    SocketIndex = -1;
    MarkTransformDirty();
}

void FSceneComponent::DetachChilds( bool _bRecursive, bool _KeepWorldTransform ) {
    while ( !Childs.IsEmpty() ) {
        FSceneComponent * child = Childs.Last();
        child->Detach( _KeepWorldTransform );
        if ( _bRecursive ) {
            child->DetachChilds( true, _KeepWorldTransform );
        }
    }
}

bool FSceneComponent::IsChild( FSceneComponent * _Child, bool _Recursive ) const {
    for ( FSceneComponent * child : Childs ) {
        if ( child == _Child || ( _Recursive && child->IsChild( _Child, true ) ) ) {
            return true;
        }
    }
    return false;
}

bool FSceneComponent::IsRoot() const {
    return GetParentActor()->RootComponent == this;
}

FSceneComponent * FSceneComponent::FindChild( const char * _UniqueName, bool _Recursive ) {
    for ( FSceneComponent * child : Childs ) {
        if ( !child->GetName().Icmp( _UniqueName ) ) {
            return child;
        }
    }

    if ( _Recursive ) {
        for ( FSceneComponent * child : Childs ) {
            FSceneComponent * rec = child->FindChild( _UniqueName, true );
            if ( rec ) {
                return rec;
            }
        }
    }
    return nullptr;
}

int FSceneComponent::FindSocket( const char * _Name ) const {
    for ( int socketIndex = 0 ; socketIndex < Sockets.Length() ; socketIndex++ ) {
        if ( !Sockets[socketIndex].SocketDef->GetName().Icmp( _Name ) ) {
            return socketIndex;
        }
    }
    GLogger.Printf( "Socket not found %s\n", _Name );
    return -1;
}

void FSceneComponent::MarkTransformDirty() {
    FSceneComponent * node = this;
    FSceneComponent * nextNode;
    int numChilds;

    while ( 1 ) {

        if ( node->bTransformDirty ) {
            // Нода уже помечена как "грязная", поэтому помечать
            // дочерние нет смысла, т.к. они уже помечены
            return;
        }

        node->bTransformDirty = true;

#ifdef FUTURE
        // TODO: здесь можно инициировать событие "On Mark Dirty"
        int NumListeners = node->Listeners.Length();
        for ( int i = 0 ; i < NumListeners ; ) {
            FActorComponent * Component = node->Listeners.ToPtr()[ i ];
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

        numChilds = node->Childs.Length();
        if ( numChilds > 0 ) {

            // Обновить дочерние узлы, причем первый дочерний узел можно обновить
            // не углубляясь в рекурсию
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

void FSceneComponent::SetAbsolutePosition( bool _AbsolutePosition ) {
    if ( bAbsolutePosition != _AbsolutePosition ) {
        bAbsolutePosition = _AbsolutePosition;

        MarkTransformDirty();
    }
}

void FSceneComponent::SetAbsoluteRotation( bool _AbsoluteRotation ) {
    if ( bAbsoluteRotation != _AbsoluteRotation ) {
        bAbsoluteRotation = _AbsoluteRotation;

        MarkTransformDirty();
    }
}

void FSceneComponent::SetAbsoluteScale( bool _AbsoluteScale ) {
    if ( bAbsoluteScale != _AbsoluteScale ) {
        bAbsoluteScale = _AbsoluteScale;

        MarkTransformDirty();
    }
}

void FSceneComponent::SetPosition( Float3 const & _Position ) {

    Position = _Position;

    MarkTransformDirty();
}

void FSceneComponent::SetPosition( float const & _X, float const & _Y, float const & _Z ) {

    Position.X = _X;
    Position.Y = _Y;
    Position.Z = _Z;

    MarkTransformDirty();
}

void FSceneComponent::SetRotation( Quat const & _Rotation ) {

    Rotation = _Rotation;

    MarkTransformDirty();
}

void FSceneComponent::SetAngles( Angl const & _Angles ) {
    Rotation = _Angles.ToQuat();

    MarkTransformDirty();
}

void FSceneComponent::SetAngles( float const & _Pitch, float const & _Yaw, float const & _Roll ) {
    Rotation = Angl( _Pitch, _Yaw, _Roll ).ToQuat();

    MarkTransformDirty();
}

void FSceneComponent::SetScale( Float3 const & _Scale ) {

    Scale = _Scale;

    MarkTransformDirty();
}

void FSceneComponent::SetScale( float const & _X, float const & _Y, float const & _Z ) {

    Scale.X = _X;
    Scale.Y = _Y;
    Scale.Z = _Z;

    MarkTransformDirty();
}

void FSceneComponent::SetScale( float const & _ScaleXYZ ) {

    Scale.X = Scale.Y = Scale.Z = _ScaleXYZ;

    MarkTransformDirty();
}

void FSceneComponent::SetTransform( Float3 const & _Position, Quat const & _Rotation ) {

    Position = _Position;
    Rotation = _Rotation;

    MarkTransformDirty();
}

void FSceneComponent::SetTransform( Float3 const & _Position, Quat const & _Rotation, Float3 const & _Scale ) {

    Position = _Position;
    Rotation = _Rotation;
    Scale = _Scale;

    MarkTransformDirty();
}

void FSceneComponent::SetTransform( FTransform const & _Transform ) {
    SetTransform( _Transform.Position, _Transform.Rotation, _Transform.Scale );
}

void FSceneComponent::SetTransform( FSceneComponent const * _Transform ) {

    Position = _Transform->Position;
    Rotation = _Transform->Rotation;
    Scale = _Transform->Scale;

    MarkTransformDirty();
}

void FSceneComponent::SetWorldPosition( Float3 const & _Position ) {
    if ( AttachParent ) {
        Float3x4 ParentTransformInverse = AttachParent->ComputeWorldTransformInverse();

        SetPosition( ParentTransformInverse * _Position );  // TODO: check
    } else {
        SetPosition( _Position );
    }
}

void FSceneComponent::SetWorldPosition( float const & _X, float const & _Y, float const & _Z ) {
    SetWorldPosition( Float3( _X, _Y, _Z ) );
}

void FSceneComponent::SetWorldRotation( Quat const & _Rotation ) {
    SetRotation( AttachParent ? AttachParent->ComputeWorldRotationInverse() * _Rotation : _Rotation );
}

void FSceneComponent::SetWorldScale( Float3 const & _Scale ) {
    SetScale( AttachParent ? _Scale / AttachParent->GetWorldScale() : _Scale );
}

void FSceneComponent::SetWorldScale( float const & _X, float const & _Y, float const & _Z ) {
    SetWorldScale( Float3( _X, _Y, _Z ) );
}

void FSceneComponent::SetWorldTransform( Float3 const & _Position, Quat const & _Rotation ) {

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

void FSceneComponent::SetWorldTransform( Float3 const & _Position, Quat const & _Rotation, Float3 const & _Scale ) {

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

void FSceneComponent::SetWorldTransform( FTransform const & _Transform ) {
    SetWorldTransform( _Transform.Position, _Transform.Rotation, _Transform.Scale );
}

Float3 const & FSceneComponent::GetPosition() const {

    return Position;

}

const Quat & FSceneComponent::GetRotation() const {

    return Rotation;

}

Angl FSceneComponent::GetAngles() const {
    Angl Angles;
    Rotation.ToAngles( Angles.Pitch, Angles.Yaw, Angles.Roll );
    Angles.Pitch = FMath::Degrees( Angles.Pitch );
    Angles.Yaw = FMath::Degrees( Angles.Yaw );
    Angles.Roll = FMath::Degrees( Angles.Roll );
    return Angles;
}

float FSceneComponent::GetPitch() const {
    return FMath::Degrees( Rotation.Pitch() );
}

float FSceneComponent::GetYaw() const {
    return FMath::Degrees( Rotation.Yaw() );
}

float FSceneComponent::GetRoll() const {
    return FMath::Degrees( Rotation.Roll() );
}

Float3 FSceneComponent::GetRightVector() const {
    //return FMath::ToMat3(Rotation)[0];

    float qyy(Rotation.Y * Rotation.Y);
    float qzz(Rotation.Z * Rotation.Z);
    float qxz(Rotation.X * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qwy(Rotation.W * Rotation.Y);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( 1 - 2 * (qyy +  qzz), 2 * (qxy + qwz), 2 * (qxz - qwy) );
}

Float3 FSceneComponent::GetLeftVector() const {
    //return -FMath::ToMat3(Rotation)[0];

    float qyy(Rotation.Y * Rotation.Y);
    float qzz(Rotation.Z * Rotation.Z);
    float qxz(Rotation.X * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qwy(Rotation.W * Rotation.Y);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( -1 + 2 * (qyy +  qzz), -2 * (qxy + qwz), -2 * (qxz - qwy) );
}

Float3 FSceneComponent::GetUpVector() const {
    //return FMath::ToMat3(Rotation)[1];

    float qxx(Rotation.X * Rotation.X);
    float qzz(Rotation.Z * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( 2 * (qxy - qwz), 1 - 2 * (qxx +  qzz), 2 * (qyz + qwx) );
}

Float3 FSceneComponent::GetDownVector() const {
    //return -FMath::ToMat3(Rotation)[1];

    float qxx(Rotation.X * Rotation.X);
    float qzz(Rotation.Z * Rotation.Z);
    float qxy(Rotation.X * Rotation.Y);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwz(Rotation.W * Rotation.Z);

    return Float3( -2 * (qxy - qwz), -1 + 2 * (qxx +  qzz), -2 * (qyz + qwx) );
}

Float3 FSceneComponent::GetBackVector() const {
    //return FMath::ToMat3(Rotation)[2];

    float qxx(Rotation.X * Rotation.X);
    float qyy(Rotation.Y * Rotation.Y);
    float qxz(Rotation.X * Rotation.Z);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwy(Rotation.W * Rotation.Y);

    return Float3( 2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx +  qyy) );
}

Float3 FSceneComponent::GetForwardVector() const {
    //return -FMath::ToMat3(Rotation)[2];

    float qxx(Rotation.X * Rotation.X);
    float qyy(Rotation.Y * Rotation.Y);
    float qxz(Rotation.X * Rotation.Z);
    float qyz(Rotation.Y * Rotation.Z);
    float qwx(Rotation.W * Rotation.X);
    float qwy(Rotation.W * Rotation.Y);

    return Float3( -2 * (qxz + qwy), -2 * (qyz - qwx), -1 + 2 * (qxx +  qyy) );
}

void FSceneComponent::GetVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const {
    //if ( _Right ) *_Right = FMath::ToMat3(Rotation)[0];
    //if ( _Up ) *_Up = FMath::ToMat3(Rotation)[1];
    //if ( _Back ) *_Back = FMath::ToMat3(Rotation)[2];
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

Float3 FSceneComponent::GetWorldRightVector() const {
    const Quat & R = GetWorldRotation();

    float qyy(R.Y * R.Y);
    float qzz(R.Z * R.Z);
    float qxz(R.X * R.Z);
    float qxy(R.X * R.Y);
    float qwy(R.W * R.Y);
    float qwz(R.W * R.Z);

    return Float3( 1 - 2 * (qyy +  qzz), 2 * (qxy + qwz), 2 * (qxz - qwy) );
}

Float3 FSceneComponent::GetWorldLeftVector() const {
    const Quat & R = GetWorldRotation();

    float qyy(R.Y * R.Y);
    float qzz(R.Z * R.Z);
    float qxz(R.X * R.Z);
    float qxy(R.X * R.Y);
    float qwy(R.W * R.Y);
    float qwz(R.W * R.Z);

    return Float3( -1 + 2 * (qyy +  qzz), -2 * (qxy + qwz), -2 * (qxz - qwy) );
}

Float3 FSceneComponent::GetWorldUpVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qzz(R.Z * R.Z);
    float qxy(R.X * R.Y);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwz(R.W * R.Z);

    return Float3( 2 * (qxy - qwz), 1 - 2 * (qxx +  qzz), 2 * (qyz + qwx) );
}

Float3 FSceneComponent::GetWorldDownVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qzz(R.Z * R.Z);
    float qxy(R.X * R.Y);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwz(R.W * R.Z);

    return Float3( -2 * (qxy - qwz), -1 + 2 * (qxx +  qzz), -2 * (qyz + qwx) );
}

Float3 FSceneComponent::GetWorldBackVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qyy(R.Y * R.Y);
    float qxz(R.X * R.Z);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwy(R.W * R.Y);

    return Float3( 2 * (qxz + qwy), 2 * (qyz - qwx), 1 - 2 * (qxx +  qyy) );
}

Float3 FSceneComponent::GetWorldForwardVector() const {
    const Quat & R = GetWorldRotation();

    float qxx(R.X * R.X);
    float qyy(R.Y * R.Y);
    float qxz(R.X * R.Z);
    float qyz(R.Y * R.Z);
    float qwx(R.W * R.X);
    float qwy(R.W * R.Y);

    return Float3( -2 * (qxz + qwy), -2 * (qyz - qwx), -1 + 2 * (qxx +  qyy) );
}

void FSceneComponent::GetWorldVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const {
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

Float3 const & FSceneComponent::GetScale() const {

    return Scale;

}

Float3 FSceneComponent::GetWorldPosition() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldTransformMatrix.DecomposeTranslation();
}

Quat const & FSceneComponent::GetWorldRotation() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldRotation;
}

Float3 FSceneComponent::GetWorldScale() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldTransformMatrix.DecomposeScale();
}

Float3x4 const & FSceneComponent::GetWorldTransformMatrix() const {

    if ( bTransformDirty ) {
        ComputeWorldTransform();
    }

    return WorldTransformMatrix;
}

void FSceneComponent::ComputeLocalTransformMatrix( Float3x4 & _LocalTransformMatrix ) const {
    _LocalTransformMatrix.Compose( Position, Rotation.ToMatrix(), Scale );
}

void FSceneComponent::ComputeWorldTransform() const {
    if ( AttachParent ) {
        if ( SocketIndex >= 0 && SocketIndex < AttachParent->Sockets.Length() ) {
            FSocket * Socket = &AttachParent->Sockets[SocketIndex];

            Float3x4 const & JointTransform = Socket->Parent->GetJointTransform( Socket->SocketDef->JointIndex );

            Quat JointRotation;
            JointRotation.FromMatrix( JointTransform.DecomposeRotation() );

            WorldRotation = bAbsoluteRotation ? Rotation : AttachParent->GetWorldRotation() * JointRotation * Rotation;

#if 0
            // Transform with shrinking

            Float3x4 LocalTransformMatrix;
            ComputeLocalTransformMatrix( LocalTransformMatrix );
            WorldTransformMatrix = AttachParent->GetWorldTransformMatrix() * JointTransform * LocalTransformMatrix;
#else
            // Take relative to parent position and rotation. Position is scaled by parent.
            WorldTransformMatrix.Compose( bAbsolutePosition ? Position : AttachParent->GetWorldTransformMatrix() * JointTransform * Position,
                                          WorldRotation.ToMatrix(),
                                          bAbsoluteScale ? Scale : Scale * AttachParent->GetWorldScale() * JointTransform.DecomposeScale() );
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

Float3x4 FSceneComponent::ComputeWorldTransformInverse() const {
    Float3x4 const & WorldTransform = GetWorldTransformMatrix();

    return WorldTransform.Inversed();
}

Quat FSceneComponent::ComputeWorldRotationInverse() const {
    return GetWorldRotation().Inversed();
}

Float3 FSceneComponent::RayToObjectSpaceCoord2D( Float3 const & _RayStart, Float3 const & _RayDir ) const {
    // Convert ray to object space
    //Float3x4 const & WorldTransform = GetWorldTransformMatrix();
    Float3x4 WorldTransformInverse = ComputeWorldTransformInverse();
    RayF ObjectSpaceRay;
    ObjectSpaceRay.Start = WorldTransformInverse * _RayStart;
    //ObjectSpaceRay.Dir = WorldTransformInverse * _RayDir;
    //ObjectSpaceRay.Dir = FMath::Normalize( WorldTransformInverse * ( _RayStart + _RayDir ) - ObjectSpaceRay.start );
    ObjectSpaceRay.Dir = ( WorldTransformInverse * ( _RayStart + _RayDir*64000.0f ) - ObjectSpaceRay.Start ).Normalized();

    // Find intersection with plane
    PlaneF Plane( 0,0,1,0 );
    Float Dist;
    if ( !FMath::Intersects( Plane, ObjectSpaceRay, Dist ) ) {
        Dist = 0;
    }

    // Retrieve plane intersection coord in object space
    Float3 ObjectSpaceCoord = ObjectSpaceRay.Start + ObjectSpaceRay.Dir * Dist;

    return ObjectSpaceCoord;
}

Float2 FSceneComponent::RayToWorldCooord2D( Float3 const & _RayStart, Float3 const & _RayDir ) const {
    return Float2( _RayToWorldCooord2D( _RayStart, _RayDir ) );
}

Float3 FSceneComponent::_RayToWorldCooord2D( Float3 const & _RayStart, Float3 const & _RayDir ) const {
    // Retrieve plane intersection coord in object space
    Float3 ObjectSpaceCoord = RayToObjectSpaceCoord2D( _RayStart, _RayDir );

    // Convert object space coord to world space
    Float3x4 const WorldTransform = GetWorldTransformMatrix();
    Float3 WorldSpaceCoord = WorldTransform * ObjectSpaceCoord;

    return WorldSpaceCoord;
#if 0
    // Find intersection with plane
    PlaneF Plane( 0,0,1,0 );
    float Dist;
    FMath::Intersects( Plane, FRay( _RayStart, _RayDir ), Dist );

    // Retrieve plane intersection coord in world space
    Float3 WorldSpaceCoord = _RayStart + _RayDir * Dist;

    return Float2( WorldSpaceCoord );
#endif
}

void FSceneComponent::TurnRightFPS( float _DeltaAngleRad ) {
    TurnLeftFPS( -_DeltaAngleRad );
}

void FSceneComponent::TurnLeftFPS( float _DeltaAngleRad ) {
    TurnAroundAxis( _DeltaAngleRad, Float3( 0,1,0 ) );
}

void FSceneComponent::TurnUpFPS( float _DeltaAngleRad ) {
    TurnAroundAxis( _DeltaAngleRad, GetRightVector() );
}

void FSceneComponent::TurnDownFPS( float _DeltaAngleRad ) {
    TurnUpFPS( -_DeltaAngleRad );
}

void FSceneComponent::TurnAroundAxis( float _DeltaAngleRad, Float3 const & _NormalizedAxis ) {
    float s, c;

    FMath::RadSinCos( _DeltaAngleRad * 0.5f, s, c );

    Rotation = Quat( c, s * _NormalizedAxis.X, s * _NormalizedAxis.Y, s * _NormalizedAxis.Z ) * Rotation;

    MarkTransformDirty();
}

void FSceneComponent::TurnAroundVector( float _DeltaAngleRad, Float3 const & _Vector ) {
    TurnAroundAxis( _DeltaAngleRad, _Vector.Normalized() );
}

void FSceneComponent::StepRight( float _Units ) {
    Step( GetRightVector() * _Units );
}

void FSceneComponent::StepLeft( float _Units ) {
    Step( GetLeftVector() * _Units );
}

void FSceneComponent::StepUp( float _Units ) {
    Step( GetUpVector() * _Units );
}

void FSceneComponent::StepDown( float _Units ) {
    Step( GetDownVector() * _Units );
}

void FSceneComponent::StepBack( float _Units ) {
    Step( GetBackVector() * _Units );
}

void FSceneComponent::StepForward( float _Units ) {
    Step( GetForwardVector() * _Units );
}

void FSceneComponent::Step( Float3 const & _Vector ) {
    Position += _Vector;

    MarkTransformDirty();
}
