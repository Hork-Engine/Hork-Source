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

/*
 TODO: Future optimizations: parallel, SSE
 */

#include <Engine/World/Public/Components/SkinnedComponent.h>
#include <Engine/World/Public/AnimationController.h>
#include <Engine/World/Public/Actors/Actor.h>
#include <Engine/World/Public/World.h>
#include <Engine/Base/Public/DebugDraw.h>
#include <Engine/Resource/Public/Asset.h>
#include <Engine/Resource/Public/Animation.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

#include <BulletSoftBody/btSoftBody.h>

#include <Engine/BulletCompatibility/BulletCompatibility.h>

FRuntimeVariable RVDrawSkeleton( _CTS( "DrawSkeleton" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVDrawSkeletonSockets( _CTS( "DrawSkeletonSockets" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( FSkinnedComponent )

FSkinnedComponent::FSkinnedComponent() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_Skeleton >();
    RenderProxy->SetOwner( this );

    bUpdateControllers = true;
    bSkinnedMesh = true;
    bLazyBoundsUpdate = true;
}

void FSkinnedComponent::InitializeComponent() {
    Super::InitializeComponent();

    FWorld * world = GetParentActor()->GetWorld();

    world->RegisterSkinnedMesh( this );
}

void FSkinnedComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    SetSkeleton( nullptr );

    RemoveAnimationControllers();

    FWorld * world = GetParentActor()->GetWorld();

    world->UnregisterSkinnedMesh( this );

    RenderProxy->KillProxy();
}

void FSkinnedComponent::OnLazyBoundsUpdate() {
    UpdateBounds();
}

void FSkinnedComponent::ReallocateRenderProxy() {
    FRenderProxy_Skeleton::FrameData & data = RenderProxy->Data;
    data.JointsCount = Skeleton->GetJoints().Size();
    data.Chunks = nullptr;
    data.ChunksTail = nullptr;
    data.bReallocated = true;
    RenderProxy->MarkUpdated();
}

void FSkinnedComponent::SetSkeleton( FSkeleton * _Skeleton ) {
    if ( Skeleton == _Skeleton ) {
        return;
    }

    Skeleton = _Skeleton;

    for ( FSocket & socket : Sockets ) {
        socket.SocketDef->RemoveRef();
    }

    if ( Skeleton ) {
        TPodArray< FJoint > const & joints = Skeleton->GetJoints();

        int numJoints = joints.Size();

        ReallocateRenderProxy();

        AbsoluteTransforms.ResizeInvalidate( numJoints + 1 );  // + 1 for root's parent
        AbsoluteTransforms[0].SetIdentity();

        RelativeTransforms.ResizeInvalidate( numJoints );
        for ( int i = 0 ; i < numJoints ; i++ ) {
            RelativeTransforms[ i ] = joints[ i ].LocalTransform;
        }

        TPodArray< FSocketDef * > const & socketDef = Skeleton->GetSockets();
        Sockets.ResizeInvalidate( socketDef.Size() );
        for ( int i = 0 ; i < socketDef.Size() ; i++ ) {
            socketDef[i]->AddRef();
            Sockets[i].SocketDef = socketDef[i];
            Sockets[i].Owner = this;
        }

    } else {
        AbsoluteTransforms.Clear();
        RelativeTransforms.Clear();
        Sockets.Clear();
    }

    bUpdateControllers = true;
}

void FSkinnedComponent::AddAnimationController( FAnimationController * _Controller ) {
    if ( !_Controller ) {
        return;
    }
    if ( _Controller->Owner ) {
        if ( _Controller->Owner != this ) {
            GLogger.Printf( "FSkinnedComponent::AddAnimationController: animation controller already added to other component\n" );
        }
        return;
    }
    _Controller->Owner = this;
    _Controller->AddRef();
    AnimControllers.Append( _Controller );
    bUpdateControllers = true;
}

void FSkinnedComponent::RemoveAnimationController( FAnimationController * _Controller ) {
    if ( !_Controller ) {
        return;
    }
    if ( _Controller->Owner != this ) {
        return;
    }
    for ( int i = 0; i < AnimControllers.Size(); i++ ) {
        if ( AnimControllers[ i ] == _Controller ) {
            _Controller->Owner = nullptr;
            _Controller->RemoveRef();
            AnimControllers.Remove( i );
            bUpdateControllers = true;
            return;
        }
    }
}

void FSkinnedComponent::RemoveAnimationControllers() {
    for ( FAnimationController * controller : AnimControllers ) {
        controller->Owner = nullptr;
        controller->RemoveRef();
    }
    AnimControllers.Clear();
    bUpdateControllers = true;
}

void FSkinnedComponent::SetTimeBroadcast( float _Time ) {
    for ( int i = 0; i < AnimControllers.Size(); i++ ) {
        FAnimationController * controller = AnimControllers[ i ];
        controller->SetTime( _Time );
    }
}

void FSkinnedComponent::AddTimeDeltaBroadcast( float _TimeDelta ) {
    for ( int i = 0; i < AnimControllers.Size(); i++ ) {
        FAnimationController * controller = AnimControllers[ i ];
        controller->AddTimeDelta( _TimeDelta );
    }
}

AN_FORCEINLINE float Quantize( float _Lerp, float _Quantizer ) {
    return _Quantizer > 0.0f ? FMath::Floor( _Lerp * _Quantizer ) / _Quantizer : _Lerp;
}

void FSkinnedComponent::MergeJointAnimations() {
    if ( bJointsSimulatedByPhysics ) {
        // TODO:
        if ( SoftBody && bUpdateAbsoluteTransforms ) {

            //GLogger.Printf("Update abs matrices\n");
            TPodArray< FJoint > const & joints = Skeleton->GetJoints();
            for ( int j = 0 ; j < joints.Size() ; j++ ) {
                // TODO: joint rotation from normal?
                AbsoluteTransforms[ j + 1 ].Compose( btVectorToFloat3( SoftBody->m_nodes[ j ].m_x ), Float3x3::Identity() );
            }
            bUpdateAbsoluteTransforms = false;
            bWriteTransforms = true;
        }

    } else {
        UpdateControllersIfDirty();
        UpdateTransformsIfDirty();
        UpdateAbsoluteTransformsIfDirty();
    }
}

void FSkinnedComponent::UpdateTransformsIfDirty() {
    if ( !bUpdateRelativeTransforms ) {
        return;
    }

    UpdateTransforms();
}

void FSkinnedComponent::UpdateTransforms() {
    if ( !Skeleton ) {
        return;
    }

    FJoint const * joints = Skeleton->GetJoints().ToPtr();
    int jointsCount = Skeleton->GetJoints().Size();

    TPodArray< FChannelTransform > tempTransforms;
    TPodArray< float > weights;

    tempTransforms.Resize( AnimControllers.Size() );
    weights.Resize( AnimControllers.Size() );

    memset( RelativeTransforms.ToPtr(), 0, sizeof( RelativeTransforms[0] ) * jointsCount );

    for ( int jointIndex = 0; jointIndex < jointsCount; jointIndex++ ) {

        float sumWeight = 0;

        int n = 0;

        for ( int controllerId = 0; controllerId < AnimControllers.Size(); controllerId++ ) {
            FAnimationController * controller = AnimControllers[ controllerId ];
            FAnimation * animation = controller->Animation;

            if ( !controller->bEnabled || !animation ) {
                continue;
            }

            unsigned short channelIndex = animation->GetChannelIndex( jointIndex );

            if ( channelIndex == (unsigned short)-1 ) {
                continue;
            }

            TPodArray< FAnimationChannel > const & animJoints = animation->GetChannels();

            FAnimationChannel const & jointAnim = animJoints[ channelIndex ];

            TPodArray< FChannelTransform > const & transforms = animation->GetTransforms();

            FChannelTransform & transform = tempTransforms[ n ];
            weights[ n ] = controller->Weight;
            n++;

            if ( controller->Frame == controller->NextFrame || controller->Blend < 0.0001f ) {
                transform = transforms[ jointAnim.TransformOffset + controller->Frame ];
            } else {
                FChannelTransform const & frame1 = transforms[ jointAnim.TransformOffset + controller->Frame ];
                FChannelTransform const & frame2 = transforms[ jointAnim.TransformOffset + controller->NextFrame ];

                transform.Position = frame1.Position.Lerp( frame2.Position, controller->Blend );
                transform.Rotation = frame1.Rotation.Slerp( frame2.Rotation, controller->Blend );
                transform.Scale = frame1.Scale.Lerp( frame2.Scale, controller->Blend );
            }

            sumWeight += controller->Weight;
        }

        Float3x4 & resultTransform = RelativeTransforms[ jointIndex ];

        if ( n > 0 ) {
            Float3x4 m;

            const float sumWeightReciprocal = ( sumWeight == 0.0f ) ? 0.0f : 1.0f / sumWeight;

            for ( int i = 0; i < n; i++ ) {
                const float weight = weights[i] * sumWeightReciprocal;

                tempTransforms[ i ].ToMatrix( m );

                resultTransform[ 0 ] += m[ 0 ] * weight;
                resultTransform[ 1 ] += m[ 1 ] * weight;
                resultTransform[ 2 ] += m[ 2 ] * weight;
            }
        } else {
            resultTransform = joints[jointIndex].LocalTransform;
        }
    }

    bUpdateRelativeTransforms = false;
    bUpdateAbsoluteTransforms = true;
}

void FSkinnedComponent::UpdateAbsoluteTransformsIfDirty() {
    if ( !bUpdateAbsoluteTransforms || !Skeleton ) {
        return;
    }

    TPodArray< FJoint > const & joints = Skeleton->GetJoints();

    for ( int j = 0 ; j < joints.Size() ; j++ ) {
        FJoint const & joint = joints[ j ];

        // ... Update relative joints physics here ...

        AbsoluteTransforms[ j + 1 ] = AbsoluteTransforms[ joint.Parent + 1 ] * RelativeTransforms[ j ];

        // ... Update absolute joints physics here ...
    }

    bUpdateAbsoluteTransforms = false;
    bWriteTransforms = true;
}

void FSkinnedComponent::UpdateControllersIfDirty() {
    if ( !bUpdateControllers ) {
        return;
    }

    UpdateControllers();
}

void FSkinnedComponent::UpdateControllers() {
    if ( !Skeleton ) {
        return;
    }

    float controllerTimeLine;
    int keyFrame;
    float lerp;
    int take;

    for ( int controllerId = 0 ; controllerId < AnimControllers.Size() ; controllerId++ ) {
        FAnimationController * controller = AnimControllers[ controllerId ];
        FAnimation * anim = controller->Animation;

        if ( !anim ) {
            continue;
        }

        if ( anim->GetFrameCount() > 1 ) {

            switch ( controller->PlayMode ) {
            case ANIMATION_PLAY_CLAMP:

                // clamp and normalize 0..1
                if ( controller->TimeLine <= 0.0f ) {

                    controller->Blend = 0;
                    controller->Frame = 0;
                    controller->NextFrame = 0;

                } else if ( controller->TimeLine >= anim->GetDurationInSeconds() ) {

                    controller->Blend = 0;
                    controller->Frame = controller->NextFrame = anim->GetFrameCount() - 1;

                } else {
                    // normalize 0..1
                    controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();

                    // adjust 0...framecount-1
                    controllerTimeLine = controllerTimeLine * (float)( anim->GetFrameCount() - 1 );

                    keyFrame = FMath::Floor( controllerTimeLine );
                    lerp = FMath::Fract( controllerTimeLine );

                    controller->Frame = keyFrame;
                    controller->NextFrame = keyFrame + 1;
                    controller->Blend = Quantize( lerp, controller->Quantizer );
                }
                break;

            case ANIMATION_PLAY_WRAP:

                // normalize 0..1
#if 1
                controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();
                controllerTimeLine = FMath::Fract( controllerTimeLine );
#else
                controllerTimeLine = fmod( controller->TimeLine, anim->GetDurationInSeconds() ) * anim->GetDurationNormalize();
                if ( controllerTimeLine < 0.0f ) {
                    controllerTimeLine += 1.0f;
                }
#endif

                // adjust 0...framecount-1
                controllerTimeLine = controllerTimeLine * (float)( anim->GetFrameCount() - 1 );

                keyFrame = FMath::Floor( controllerTimeLine );
                lerp = FMath::Fract( controllerTimeLine );

                if ( controller->TimeLine < 0.0f ) {
                    controller->Frame = keyFrame + 1;
                    controller->NextFrame = keyFrame;
                    controller->Blend = Quantize( 1.0f - lerp, controller->Quantizer );
                } else {
                    controller->Frame = keyFrame;
                    controller->NextFrame = controller->Frame + 1;
                    controller->Blend = Quantize( lerp, controller->Quantizer );
                }
                break;

            case ANIMATION_PLAY_MIRROR:

                // normalize 0..1
                controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();
                take = FMath::Floor( FMath::Abs( controllerTimeLine ) );
                controllerTimeLine = FMath::Fract( controllerTimeLine );

                // adjust 0...framecount-1
                controllerTimeLine = controllerTimeLine * (float)( anim->GetFrameCount() - 1 );

                keyFrame = FMath::Floor( controllerTimeLine );
                lerp = FMath::Fract( controllerTimeLine );

                if ( controller->TimeLine < 0.0f ) {
                    controller->Frame = keyFrame + 1;
                    controller->NextFrame = keyFrame;
                    controller->Blend = Quantize( 1.0f - lerp, controller->Quantizer );
                } else {
                    controller->Frame = keyFrame;
                    controller->NextFrame = controller->Frame + 1;
                    controller->Blend = Quantize( lerp, controller->Quantizer );
                }

                if ( take & 1 ) {
                    controller->Frame = anim->GetFrameCount() - controller->Frame - 1;
                    controller->NextFrame = anim->GetFrameCount() - controller->NextFrame - 1;
                }
                break;
            }
        } else if ( anim->GetFrameCount() == 1 ) {
            controller->Blend = 0;
            controller->Frame = 0;
            controller->NextFrame = 0;
        }
    }

    bUpdateControllers = false;
    bUpdateBounds = true;
    bUpdateRelativeTransforms = true;
}

void FSkinnedComponent::UpdateBounds() {
    UpdateControllersIfDirty();

    if ( !bUpdateBounds || !Skeleton ) {
        return;
    }

    bUpdateBounds = false;

    if ( AnimControllers.IsEmpty() ) {

        Bounds = Skeleton->GetBindposeBounds();

    } else {
        Bounds.Clear();
        for ( int controllerId = 0 ; controllerId < AnimControllers.Size() ; controllerId++ ) {
            FAnimationController const * controller = AnimControllers[ controllerId ];
            FAnimation * animation = controller->Animation;

            if ( !controller->bEnabled || !animation || animation->GetFrameCount() == 0 ) {
                continue;
            }

            Bounds.AddAABB( animation->GetBoundingBoxes()[ controller->Frame ] );
        }
    }

    // Mark to update world bounds
    MarkWorldBoundsDirty();
}

void FSkinnedComponent::UpdateJointTransforms() {
    if ( !Skeleton ) {
        return;
    }

    MergeJointAnimations();

    if ( bWriteTransforms ) {
        TPodArray< FJoint > const & joints = Skeleton->GetJoints();

        Float3x4 * transforms = WriteJointTransforms( joints.Size(), 0 );
        if ( transforms ) {
            for ( int j = 0 ; j < joints.Size() ; j++ ) {
                transforms[ j ] = AbsoluteTransforms[ j + 1 ] * joints[ j ].OffsetMatrix;
            }
        }
        bWriteTransforms = false;
    }
}

Float3x4 const & FSkinnedComponent::GetJointTransform( int _JointIndex ) {
    if ( !Skeleton || _JointIndex < 0 || _JointIndex >= Skeleton->GetJoints().Size() ) {
        return Float3x4::Identity();
    }

    MergeJointAnimations();

    return AbsoluteTransforms[_JointIndex+1];
}

Float3x4 * FSkinnedComponent::WriteJointTransforms( int _JointsCount, int _StartJointLocation ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();
    FRenderProxy_Skeleton::FrameData & data = RenderProxy->Data;

    if ( !_JointsCount ) {
        return nullptr;
    }

    AN_Assert( _StartJointLocation + _JointsCount <= Skeleton->GetJoints().Size() );

    FJointTransformChunk * chunk = ( FJointTransformChunk * )frameData->AllocFrameData( sizeof( FJointTransformChunk ) + sizeof( Float3x4 ) * ( _JointsCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->JointsCount = _JointsCount;
    chunk->StartJointLocation = _StartJointLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.Chunks, data.ChunksTail );

    RenderProxy->MarkUpdated();

    return &chunk->Transforms[0];
}

void FSkinnedComponent::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    // Draw sockets
    if ( RVDrawSkeletonSockets ) {
        for ( FSocket & socket : Sockets ) {
            Float3x4 const & transform = GetJointTransform( socket.SocketDef->JointIndex );
            _DebugDraw->DrawAxis( GetWorldTransformMatrix() * transform, true );
        }
    }

    // Draw skeleton
    if ( RVDrawSkeleton ) {
        if ( Skeleton ) {
            _DebugDraw->SetColor( FColor4( 1,0,0,1 ) );
            _DebugDraw->SetDepthTest( false );
            TPodArray< FJoint > const & joints = Skeleton->GetJoints();
            for ( int i = 0 ; i < joints.Size() ; i++ ) {
                FJoint const & joint = joints[i];

                Float3x4 t = GetWorldTransformMatrix() * GetJointTransform( i );
                Float3 v1 = t.DecomposeTranslation();

                _DebugDraw->DrawOrientedBox( v1, t.DecomposeRotation(), Float3(0.01f) );

                if ( joint.Parent >= 0 ) {
                    Float3 v0 = ( GetWorldTransformMatrix() * GetJointTransform( joint.Parent ) ).DecomposeTranslation();
                    _DebugDraw->DrawLine( v0, v1 );
                }
            }
        }
    }
}

/*
void FJointTransform::ToMatrix( Float3x4 & _JointTransform ) const {
    float * mat = FMath::ToPtr( _JointTransform );

    float x2 = Rotation.x + Rotation.x;
    float y2 = Rotation.y + Rotation.y;
    float z2 = Rotation.z + Rotation.z;

    float xx = Rotation.x * x2;
    float xy = Rotation.x * y2;
    float xz = Rotation.x * z2;

    float yy = Rotation.y * y2;
    float yz = Rotation.y * z2;
    float zz = Rotation.z * z2;

    float wx = Rotation.w * x2;
    float wy = Rotation.w * y2;
    float wz = Rotation.w * z2;

    mat[ 0 * 4 + 0 ] = 1.0f - ( yy + zz );
    mat[ 0 * 4 + 1 ] = xy + wz;
    mat[ 0 * 4 + 2 ] = xz - wy;

    mat[ 1 * 4 + 0 ] = xy - wz;
    mat[ 1 * 4 + 1 ] = 1.0f - ( xx + zz );
    mat[ 1 * 4 + 2 ] = yz + wx;

    mat[ 2 * 4 + 0 ] = xz + wy;
    mat[ 2 * 4 + 1 ] = yz - wx;
    mat[ 2 * 4 + 2 ] = 1.0f - ( xx + yy );

    mat[ 0 * 4 + 3 ] = Position[0];
    mat[ 1 * 4 + 3 ] = Position[1];
    mat[ 2 * 4 + 3 ] = Position[2];
}

void FJointTransform::FromMatrix( Float3x4 const & _JointTransform ) {
    float trace;
    float s;
    float t;
    int i;
    int j;
    int k;

    static const int next[3] = { 1, 2, 0 };

    const float * mat = FMath::ToPtr( _JointTransform );

    trace = mat[ 0 * 4 + 0 ] + mat[ 1 * 4 + 1 ] + mat[ 2 * 4 + 2 ];

    if ( trace > 0.0f ) {

        t = trace + 1.0f;
        s = FMath::InvSqrt( t ) * 0.5f;

        Rotation[3] = s * t;
        Rotation[0] = ( mat[ 1 * 4 + 2 ] - mat[ 2 * 4 + 1 ] ) * s;
        Rotation[1] = ( mat[ 2 * 4 + 0 ] - mat[ 0 * 4 + 2 ] ) * s;
        Rotation[2] = ( mat[ 0 * 4 + 1 ] - mat[ 1 * 4 + 0 ] ) * s;

    } else {

        i = 0;
        if ( mat[ 1 * 4 + 1 ] > mat[ 0 * 4 + 0 ] ) {
            i = 1;
        }
        if ( mat[ 2 * 4 + 2 ] > mat[ i * 4 + i ] ) {
            i = 2;
        }
        j = next[i];
        k = next[j];

        t = ( mat[ i * 4 + i ] - ( mat[ j * 4 + j ] + mat[ k * 4 + k ] ) ) + 1.0f;
        s = FMath::InvSqrt( t ) * 0.5f;

        Rotation[i] = s * t;
        Rotation[3] = ( mat[ j * 4 + k ] - mat[ k * 4 + j ] ) * s;
        Rotation[j] = ( mat[ i * 4 + j ] + mat[ j * 4 + i ] ) * s;
        Rotation[k] = ( mat[ i * 4 + k ] + mat[ k * 4 + i ] ) * s;
    }

    Position.x = mat[ 0 * 4 + 3 ];
    Position.y = mat[ 1 * 4 + 3 ];
    Position.z = mat[ 2 * 4 + 3 ];
}
*/
