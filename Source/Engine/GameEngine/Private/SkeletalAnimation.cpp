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

#include <Engine/GameEngine/Public/SkeletalAnimation.h>
#include <Engine/GameEngine/Public/Actor.h>
#include <Engine/GameEngine/Public/World.h>
#include <Engine/GameEngine/Public/GameEngine.h>
#include <Engine/GameEngine/Public/DebugDraw.h>
#include <Engine/GameEngine/Public/MeshAsset.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

#include <BulletSoftBody/btSoftBody.h>
#include "BulletCompatibility/BulletCompatibility.h"

AN_CLASS_META_NO_ATTRIBS( FSkeleton )
AN_CLASS_META_NO_ATTRIBS( FSkeletonAnimation )
AN_CLASS_META_NO_ATTRIBS( FSocketDef )
AN_CLASS_META_NO_ATTRIBS( FSkinnedComponent )

///////////////////////////////////////////////////////////////////////////////////////////////////////

FSkeleton::FSkeleton() {
}

FSkeleton::~FSkeleton() {
    Purge();
}

void FSkeleton::Purge() {
    Joints.Clear();

    for ( FSkeletonAnimation * animation : Animations ) {
        animation->Skeleton = nullptr;
        animation->RemoveRef();
    }

    for ( FSocketDef * socket : Sockets ) {
        socket->RemoveRef();
    }

    // TODO: notify components about changes
}

void FSkeleton::Initialize( FJoint * _Joints, int _JointsCount, BvAxisAlignedBox const & _BindposeBounds ) {
    Purge();

    Joints.ResizeInvalidate( _JointsCount );
    memcpy( Joints.ToPtr(), _Joints, sizeof( *_Joints ) * _JointsCount );

    BindposeBounds = _BindposeBounds;

    // TODO: notify components about changes
}

void FSkeleton::InitializeDefaultObject() {
    Purge();
}

bool FSkeleton::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    FFileStream f;

    if ( !f.OpenRead( _Path ) ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    FSkeletonAsset asset;
    asset.Read( f );

    Initialize( asset.Joints.ToPtr(), asset.Joints.Size(), asset.BindposeBounds );
    for ( int i = 0; i < asset.Animations.size(); i++ ) {
        FSkeletonAnimation * sanim = CreateAnimation();
        FSkeletalAnimationAsset const & animAsset = asset.Animations[ i ];
        sanim->Initialize( animAsset.FrameCount, animAsset.FrameDelta,
                           animAsset.Transforms.ToPtr(), animAsset.Transforms.Size(),
                           animAsset.AnimatedJoints.ToPtr(), animAsset.AnimatedJoints.Size(), animAsset.Bounds.ToPtr() );
    }

    return true;
}

int FSkeleton::FindJoint( const char * _Name ) const {
    for ( int j = 0 ; j < Joints.Size() ; j++ ) {
        if ( !FString::Icmp( Joints[j].Name, _Name ) ) {
            return j;
        }
    }
    return -1;
}

FSkeletonAnimation * FSkeleton::CreateAnimation() {
    FSkeletonAnimation * animation = NewObject< FSkeletonAnimation >();
    animation->AddRef();
    animation->Skeleton = this;
    Animations.Append( animation );

    // TODO: notify components about changes

    return animation;
}

FSocketDef * FSkeleton::FindSocket( const char * _Name ) {
    for ( FSocketDef * socket : Sockets ) {
        if ( socket->GetName().Icmp( _Name ) ) {
            return socket;
        }
    }
    return nullptr;
}

FSocketDef * FSkeleton::CreateSocket( const char * _Name, int _JointIndex ) {
    if ( _JointIndex < 0 || _JointIndex >= Joints.Size() ) {
        return nullptr;
    }

    FSocketDef * socket = FindSocket( _Name );
    if ( socket ) {
        // Socket already exists
        return nullptr;
    }

    socket = NewObject< FSocketDef >();
    socket->AddRef();
    socket->SetName( _Name );
    socket->JointIndex = _JointIndex;
    Sockets.Append( socket );

    // TODO: notify components about changes

    return socket;
}

FSocketDef * FSkeleton::CreateSocket( const char * _Name, const char * _JointName ) {
    return CreateSocket( _Name, FindJoint( _JointName ) );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

FSkeletonAnimation::FSkeletonAnimation() {
    FrameCount = 0;
    FrameDelta = 0;
    FrameRate = 60.0f;
}

FSkeletonAnimation::~FSkeletonAnimation() {
}

void FSkeletonAnimation::Initialize( int _FrameCount, float _FrameDelta, FJointTransform const * _Transforms, int _TransformsCount, FJointAnimation const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds ) {
    AN_Assert( _TransformsCount == _FrameCount * _NumAnimatedJoints );

    AnimatedJoints.ResizeInvalidate( _NumAnimatedJoints );
    memcpy( AnimatedJoints.ToPtr(), _AnimatedJoints, sizeof( AnimatedJoints[0] ) * _NumAnimatedJoints );

    Transforms.ResizeInvalidate( _TransformsCount );
    memcpy( Transforms.ToPtr(), _Transforms, sizeof( Transforms[0] ) * _TransformsCount );

    Bounds.ResizeInvalidate( _FrameCount );
    memcpy( Bounds.ToPtr(), _Bounds, sizeof( Bounds[0] ) * _FrameCount );

    ChannelsMap.ResizeInvalidate( Skeleton->GetJoints().Size() );

    for ( int i = 0; i < ChannelsMap.Size(); i++ ) {
        ChannelsMap[ i ] = (unsigned short)-1;
    }

    for ( int i = 0; i < AnimatedJoints.Size(); i++ ) {
        ChannelsMap[ AnimatedJoints[ i ].JointIndex ] = i;
    }

    FrameCount = _FrameCount;
    FrameDelta = _FrameDelta;
    FrameRate = 1.0f / _FrameDelta;
    DurationInSeconds = ( FrameCount - 1 ) * FrameDelta;
    DurationNormalizer = 1.0f / DurationInSeconds;    
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

FSkinnedComponent::FSkinnedComponent() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_Skeleton >();
    RenderProxy->SetOwner( this );

    bUpdateControllers = true;
    //bWorldBoundsDirty = true;
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

    FWorld * world = GetParentActor()->GetWorld();

    world->UnregisterSkinnedMesh( this );

    RenderProxy->KillProxy();
}

void FSkinnedComponent::OnLazyBoundsUpdate() {
    UpdateBounds();
}

void FSkinnedComponent::ReallocateRenderProxy() {
    FRenderFrame * frameData = GRuntime.GetFrameData();
    FRenderProxy_Skeleton::FrameData & data = RenderProxy->Data[frameData->WriteIndex];
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
        TPodArray< FSkeletonAnimation * > const & animations = Skeleton->GetAnimations();

        int numJoints = joints.Size();

        ReallocateRenderProxy();

        AbsoluteTransforms.ResizeInvalidate( numJoints + 1 );  // + 1 for root
        AbsoluteTransforms[0].SetIdentity();

        RelativeTransforms.ResizeInvalidate( numJoints );
        for ( int i = 0 ; i < numJoints ; i++ ) {
            RelativeTransforms[ i ] = joints[ i ].LocalTransform;
        }

        AnimControllers.ResizeInvalidate( animations.Size() );
        for ( int i = 0 ; i < animations.Size() ; i++ ) {
            AnimControllers[i].Blend = 0;
            AnimControllers[i].Frame = 0;
            AnimControllers[i].NextFrame = 0;
            AnimControllers[i].TimeLine = 0;
            AnimControllers[i].PlayMode = ANIMATION_PLAY_CLAMP;
            AnimControllers[i].Quantizer = 0.0f;
            AnimControllers[i].Weight = 1;
            AnimControllers[i].bEnabled = true;
        }

        TPodArray< FSocketDef * > const & socketDef = Skeleton->GetSockets();
        Sockets.ResizeInvalidate( socketDef.Size() );
        for ( int i = 0 ; i < socketDef.Size() ; i++ ) {
            socketDef[i]->AddRef();
            Sockets[i].SocketDef = socketDef[i];
            Sockets[i].Parent = this;
        }

    } else {
        AbsoluteTransforms.Clear();
        RelativeTransforms.Clear();
        AnimControllers.Clear();
        Sockets.Clear();
    }

    bUpdateControllers = true;
}

void FSkinnedComponent::SetControllerTimeline( int _Controller, float _Timeline, EAnimationPlayMode _PlayMode, float _Quantizer ) {
    if ( _Controller >= AnimControllers.Size() ) {
        GLogger.Printf( "Unknown animation controller\n" );
        return;
    }

    AnimControllers[_Controller].TimeLine = _Timeline;
    AnimControllers[_Controller].PlayMode = _PlayMode;
    AnimControllers[_Controller].Quantizer = FMath::Min( _Quantizer, 1.0f );

    // TODO: quantizer можно выбирать динамически в зависимости от удаленности камеры от объекта

    bUpdateControllers = true;
}

void FSkinnedComponent::SetControllerWeight( int _Controller, float _Weight ) {
    if ( _Controller >= AnimControllers.Size() ) {
        GLogger.Printf( "Unknown animation controller\n" );
        return;
    }

    AnimControllers[_Controller].Weight = _Weight;
}

void FSkinnedComponent::SetControllerEnabled( int _Controller, bool _Enabled ) {
    if ( _Controller >= AnimControllers.Size() ) {
        GLogger.Printf( "Unknown animation controller\n" );
        return;
    }

    AnimControllers[_Controller].bEnabled = _Enabled;
}

void FSkinnedComponent::SetTimelineBroadcast( float _Timeline, EAnimationPlayMode _PlayMode, float _Quantizer ) {
    const float quantizer = FMath::Min( _Quantizer, 1.0f );
    for ( int i = 0 ; i < AnimControllers.Size() ; i++ ) {
        FAnimationController * controller = &AnimControllers[ i ];
        controller->TimeLine = _Timeline;
        controller->PlayMode = _PlayMode;
        controller->Quantizer = quantizer;
    }

    bUpdateControllers = true;
}

void FSkinnedComponent::AddTimeDelta( int _Controller, float _TimeDelta ) {
    if ( _Controller >= AnimControllers.Size() ) {
        GLogger.Printf( "Unknown animation controller\n" );
        return;
    }

    AnimControllers[_Controller].TimeLine += _TimeDelta;

    bUpdateControllers = true;
}

void FSkinnedComponent::AddTimeDeltaBroadcast( float _TimeDelta ) {
    for ( int ch = 0 ; ch < AnimControllers.Size() ; ch++ ) {
        FAnimationController * controller = &AnimControllers[ ch ];
        controller->TimeLine += _TimeDelta;
    }

    bUpdateControllers = true;
}

AN_FORCEINLINE float Quantize( float _Lerp, float _Quantizer ) {
    return _Quantizer > 0.0f ? floorf( _Lerp * _Quantizer ) / _Quantizer : _Lerp;
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

    TPodArray< FSkeletonAnimation * > const & animations = Skeleton->GetAnimations();
    FJoint const * joints = Skeleton->GetJoints().ToPtr();
    int jointsCount = Skeleton->GetJoints().Size();

    TPodArray< FJointTransform > tempTransforms;
    TPodArray< float > weights;

    tempTransforms.Resize( AnimControllers.Size() );
    weights.Resize( AnimControllers.Size() );

    memset( RelativeTransforms.ToPtr(), 0, sizeof( RelativeTransforms[0] ) * jointsCount );

    for ( int jointIndex = 0; jointIndex < jointsCount; jointIndex++ ) {

        float sumWeight = 0;

        int n = 0;

        for ( int animIndex = 0; animIndex < AnimControllers.Size(); animIndex++ ) {
            FAnimationController * controller = &AnimControllers[ animIndex ];

            if ( !controller->bEnabled ) {
                continue;
            }

            FSkeletonAnimation * animation = animations[ animIndex ];

            unsigned short channelIndex = animation->GetChannelsMap()[ jointIndex ];

            if ( channelIndex == (unsigned short)-1 ) {
                continue;
            }

            TPodArray< FJointAnimation > const & animJoints = animation->GetAnimatedJoints();

            FJointAnimation const & jointAnim = animJoints[ channelIndex ];

            TPodArray< FJointTransform > const & transforms = animation->GetTransforms();

            FJointTransform & transform = tempTransforms[ n ];
            weights[ n ] = controller->Weight;
            n++;

            if ( controller->Frame == controller->NextFrame || controller->Blend < 0.0001f ) {
                transform = transforms[ jointAnim.TransformOffset + controller->Frame ];
            } else {
                FJointTransform const & frame1 = transforms[ jointAnim.TransformOffset + controller->Frame ];
                FJointTransform const & frame2 = transforms[ jointAnim.TransformOffset + controller->NextFrame ];

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

    //GLogger.Printf( "Updating absolute matrices (%d)\n", GGameEngine.GetFrameNumber() );
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

    TPodArray< FSkeletonAnimation * > const & animations = Skeleton->GetAnimations();
    Float controllerTimeLine;
    int keyFrame;
    float lerp;
    int take;

    for ( int animIndex = 0 ; animIndex < AnimControllers.Size() ; animIndex++ ) {
        FAnimationController * controller = &AnimControllers[ animIndex ];
        FSkeletonAnimation * anim = animations[ animIndex ];

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

                    keyFrame = controllerTimeLine.Floor();
                    lerp = controllerTimeLine.Fract();

                    controller->Frame = keyFrame;
                    controller->NextFrame = keyFrame + 1;
                    controller->Blend = Quantize( lerp, controller->Quantizer );
                }
                break;

            case ANIMATION_PLAY_WRAP:

                // normalize 0..1
#if 1
                controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();
                controllerTimeLine = controllerTimeLine.Fract();
#else
                controllerTimeLine = fmod( controller->TimeLine, anim->GetDurationInSeconds() ) * anim->GetDurationNormalize();
                if ( controllerTimeLine < 0.0f ) {
                    controllerTimeLine += 1.0f;
                }
#endif

                // adjust 0...framecount-1
                controllerTimeLine = controllerTimeLine * (float)( anim->GetFrameCount() - 1 );

                keyFrame = controllerTimeLine.Floor();
                lerp = controllerTimeLine.Fract();

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
                take = controllerTimeLine.Abs().Floor();
                controllerTimeLine = controllerTimeLine.Fract();

                // adjust 0...framecount-1
                controllerTimeLine = controllerTimeLine * (float)( anim->GetFrameCount() - 1 );

                keyFrame = controllerTimeLine.Floor();
                lerp = controllerTimeLine.Fract();

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

    TPodArray< FSkeletonAnimation * > const & animations = Skeleton->GetAnimations();

    if ( AnimControllers.IsEmpty() ) {

        Bounds = Skeleton->GetBindposeBounds();

    } else {
        Bounds.Clear();
        for ( int animIndex = 0 ; animIndex < AnimControllers.Size() ; animIndex++ ) {
            FAnimationController const * controller = &AnimControllers[ animIndex ];
            if ( !controller->bEnabled ) {
                continue;
            }

            FSkeletonAnimation const * anim = animations[ animIndex ];
            Bounds.AddAABB( anim->GetBoundingBoxes()[ controller->Frame ] );
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

        //GLogger.Printf( "Write transforms (%d)\n", GGameEngine.GetFrameNumber() );
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
    FRenderProxy_Skeleton::FrameData & data = RenderProxy->Data[ frameData->WriteIndex ];

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
    if ( GDebugDrawFlags.bDrawSkeletonSockets ) {
        for ( FSocket & socket : Sockets ) {
            Float3x4 const & transform = GetJointTransform( socket.SocketDef->JointIndex );
            _DebugDraw->DrawAxis( GetWorldTransformMatrix() * transform, true );
        }
    }

    // Draw skeleton
    if ( GDebugDrawFlags.bDrawSkeleton ) {
        if ( Skeleton ) {
            _DebugDraw->SetColor( 1,0,0,1 );
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
