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

#include <Engine/World/Public/SkeletalAnimation.h>
#include <Engine/World/Public/Actor.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/DebugDraw.h>
#include <Engine/World/Public/MeshAsset.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

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

void FSkeleton::Initialize( FJoint * _Joints, int _JointsCount ) {
    Purge();

    Joints.ResizeInvalidate( _JointsCount );
    memcpy( Joints.ToPtr(), _Joints, sizeof( *_Joints ) * _JointsCount );

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

    Initialize( asset.Joints.ToPtr(), asset.Joints.Length() );
    for ( int i = 0; i < asset.Animations.size(); i++ ) {
        FSkeletonAnimation * sanim = CreateAnimation();
        FSkeletalAnimationAsset const & animAsset = asset.Animations[ i ];
        sanim->Initialize( animAsset.FrameCount, animAsset.FrameDelta,
            animAsset.AnimatedJoints.ToPtr(), animAsset.AnimatedJoints.Length(), animAsset.Bounds.ToPtr() );
    }

    return true;
}

int FSkeleton::FindJoint( const char * _Name ) const {
    for ( int j = 0 ; j < Joints.Length() ; j++ ) {
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
    if ( _JointIndex < 0 || _JointIndex >= Joints.Length() ) {
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

void FSkeletonAnimation::Initialize( int _FrameCount, float _FrameDelta, FJointAnimation const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds ) {
    AnimatedJoints.resize( _NumAnimatedJoints );

    for ( int i = 0 ; i < _NumAnimatedJoints ; ++i ) {
        AnimatedJoints[ i ].JointIndex = _AnimatedJoints[ i ].JointIndex;
        AnimatedJoints[ i ].Frames = _AnimatedJoints[ i ].Frames;
    }

    FrameCount = _FrameCount;
    FrameDelta = _FrameDelta;
    FrameRate = 1.0f / _FrameDelta;
    DurationInSeconds = ( FrameCount - 1 ) * FrameDelta;
    DurationNormalizer = 1.0f / DurationInSeconds;
    Bounds.Resize( _FrameCount );
    memcpy( Bounds.ToPtr(), _Bounds, FrameCount * sizeof( BvAxisAlignedBox ) );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

FSkinnedComponent::FSkinnedComponent() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_Skeleton >();
    RenderProxy->SetOwner( this );

    bUpdateChannels = true;
    //bWorldBoundsDirty = true;
    bSkinnedMesh = true;
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

void FSkinnedComponent::ReallocateRenderProxy() {
    FRenderFrame * frameData = GRuntime.GetFrameData();
    FRenderProxy_Skeleton::FrameData & data = RenderProxy->Data[frameData->SmpIndex];
    data.JointsCount = Skeleton->GetJoints().Length();
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

        int numJoints = joints.Length();

        ReallocateRenderProxy();

        AbsoluteMatrices.ResizeInvalidate( numJoints + 1 );  // + 1 for root
        AbsoluteMatrices[0].SetIdentity();

        RelativeTransforms.ResizeInvalidate( numJoints );
        for ( int i = 0 ; i < numJoints ; i++ ) {
            RelativeTransforms[ i ].SetIdentity();
        }

        AnimChannels.ResizeInvalidate( animations.Length() );
        for ( int i = 0 ; i < animations.Length() ; i++ ) {
            AnimChannels[i].Blend = 0;
            AnimChannels[i].Frame = 0;
            AnimChannels[i].NextFrame = 0;
            AnimChannels[i].TimeLine = 0;
            AnimChannels[i].PlayMode = ANIMATION_PLAY_CLAMP;
            AnimChannels[i].Quantizer = 0.0f;
        }

        TPodArray< FSocketDef * > const & socketDef = Skeleton->GetSockets();
        Sockets.ResizeInvalidate( socketDef.Length() );
        for ( int i = 0 ; i < socketDef.Length() ; i++ ) {
            socketDef[i]->AddRef();
            Sockets[i].SocketDef = socketDef[i];
            Sockets[i].Parent = this;
        }

    } else {
        AbsoluteMatrices.Clear();
        RelativeTransforms.Clear();
        AnimChannels.Clear();
        Sockets.Clear();
    }

    bUpdateChannels = true;
}

void FSkinnedComponent::SetChannelTimeline( int _Channel, float _Timeline, EAnimationPlayMode _PlayMode, float _Quantizer ) {
    if ( _Channel >= AnimChannels.Length() ) {
        GLogger.Printf( "Unknown animation channel\n" );
        return;
    }

    AnimChannels[_Channel].TimeLine = _Timeline;
    AnimChannels[_Channel].PlayMode = _PlayMode;
    AnimChannels[_Channel].Quantizer = FMath::Min( _Quantizer, 1.0f );

    // TODO: quantizer можно выбирать динамически в зависимости от удаленности камеры от объекта

    bUpdateChannels = true;
}

void FSkinnedComponent::SetTimelineBroadcast( float _Timeline, EAnimationPlayMode _PlayMode, float _Quantizer ) {
    const float quantizer = FMath::Min( _Quantizer, 1.0f );
    for ( int ch = 0 ; ch < AnimChannels.Length() ; ch++ ) {
        FAnimChannel * channel = &AnimChannels[ ch ];
        channel->TimeLine = _Timeline;
        channel->PlayMode = _PlayMode;
        channel->Quantizer = quantizer;
    }

    bUpdateChannels = true;
}

void FSkinnedComponent::AddTimeDelta( int _Channel, float _TimeDelta ) {
    if ( _Channel >= AnimChannels.Length() ) {
        GLogger.Printf( "Unknown animation channel\n" );
        return;
    }

    AnimChannels[_Channel].TimeLine += _TimeDelta;

    bUpdateChannels = true;
}

void FSkinnedComponent::AddTimeDeltaBroadcast( float _TimeDelta ) {
    for ( int ch = 0 ; ch < AnimChannels.Length() ; ch++ ) {
        FAnimChannel * channel = &AnimChannels[ ch ];
        channel->TimeLine += _TimeDelta;
    }

    bUpdateChannels = true;
}

void FSkinnedComponent::ApplyTransforms( FJointAnimation const * _AnimJoints,
                                          int _JointsCount,
                                          int _FrameIndex,
                                          TPodArray< Float3x4 > & _RelativeTransforms ) {

    for ( int j = 0 ; j < _JointsCount ; j++ ) {
        FJointAnimation const * jointAnim = _AnimJoints + j;
        jointAnim->Frames[_FrameIndex].Transform.ToMatrix( _RelativeTransforms[ jointAnim->JointIndex ] );
    }
}

void FSkinnedComponent::BlendTransforms( FJointAnimation const * _AnimJoints,
                                          int _JointsCount,
                                          int _FrameIndex1,
                                          int _FrameIndex2,
                                          float _Blend,
                                          TPodArray< Float3x4 > & _RelativeTransforms ) {

    for ( int j = 0 ; j < _JointsCount ; j++ ) {
        FJointAnimation const * jointAnim = _AnimJoints + j;

        FJointTransform const & frame1 = jointAnim->Frames[_FrameIndex1].Transform;
        FJointTransform const & frame2 = jointAnim->Frames[_FrameIndex2].Transform;

        // Retrieve blended relative transform matrix
        _RelativeTransforms[ jointAnim->JointIndex ].Compose( frame1.Position.Lerp( frame2.Position, _Blend ),
                                                              frame1.Rotation.Slerp( frame2.Rotation, _Blend ).ToMatrix(),
                                                              frame1.Scale.Lerp( frame2.Scale, _Blend ) );
    }
}

AN_FORCEINLINE float Quantize( float _Lerp, float _Quantizer ) {
    return _Quantizer > 0.0f ? floorf( _Lerp * _Quantizer ) / _Quantizer : _Lerp;
}

void FSkinnedComponent::MergeJointAnimations() {
    UpdateChannelsIfDirty();
    UpdateTransformsIfDirty();
    UpdateAbsoluteTransformsIfDirty();
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

    for ( int ch = 0 ; ch < AnimChannels.Length() ; ch++ ) {
        FAnimChannel * channel = &AnimChannels[ ch ];
        FSkeletonAnimation * anim = animations[ ch ];

        TVector< FJointAnimation > const & animJoints = anim->GetAnimatedJoints();

        if ( channel->Frame == channel->NextFrame || channel->Blend < 0.0001f ) {
            ApplyTransforms( animJoints.data(), animJoints.size(), channel->Frame, RelativeTransforms );
        } else {
            BlendTransforms( animJoints.data(), animJoints.size(), channel->Frame, channel->NextFrame, channel->Blend, RelativeTransforms );
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

    for ( int j = 0 ; j < joints.Length() ; j++ ) {
        FJoint const & joint = joints[ j ];

        // ... Update relative joints physics here ...

        AbsoluteMatrices[ j + 1 ] = AbsoluteMatrices[ joint.Parent + 1 ] * RelativeTransforms[ j ];

        // ... Update absolute joints physics here ...
    }

    bUpdateAbsoluteTransforms = false;
    bWriteTransforms = true;

    //GLogger.Printf( "Updating absolute matrices (%d)\n", GGameMaster.GetFrameNumber() );
}

void FSkinnedComponent::UpdateChannelsIfDirty() {
    if ( !bUpdateChannels ) {
        return;
    }

    UpdateChannels();
}

void FSkinnedComponent::UpdateChannels() {
    if ( !Skeleton ) {
        return;
    }

    TPodArray< FSkeletonAnimation * > const & animations = Skeleton->GetAnimations();
    Float channelTimeLine;
    int keyFrame;
    float lerp;
    int take;

    for ( int ch = 0 ; ch < AnimChannels.Length() ; ch++ ) {
        FAnimChannel * channel = &AnimChannels[ ch ];
        FSkeletonAnimation * anim = animations[ ch ];

        if ( anim->GetFrameCount() > 1 ) {

            switch ( channel->PlayMode ) {
            case ANIMATION_PLAY_CLAMP:

                // clamp and normalize 0..1
                if ( channel->TimeLine <= 0.0f ) {

                    channel->Blend = 0;
                    channel->Frame = 0;
                    channel->NextFrame = 0;

                } else if ( channel->TimeLine >= anim->GetDurationInSeconds() ) {

                    channel->Blend = 0;
                    channel->Frame = channel->NextFrame = anim->GetFrameCount() - 1;

                } else {
                    // normalize 0..1
                    channelTimeLine = channel->TimeLine * anim->GetDurationNormalizer();

                    // adjust 0...framecount-1
                    channelTimeLine = channelTimeLine * (float)( anim->GetFrameCount() - 1 );

                    keyFrame = channelTimeLine.Floor();
                    lerp = channelTimeLine.Fract();

                    channel->Frame = keyFrame;
                    channel->NextFrame = keyFrame + 1;
                    channel->Blend = Quantize( lerp, channel->Quantizer );
                }
                break;

            case ANIMATION_PLAY_WRAP:

                // normalize 0..1
#if 1
                channelTimeLine = channel->TimeLine * anim->GetDurationNormalizer();
                channelTimeLine = channelTimeLine.Fract();
#else
                channelTimeLine = fmod( channel->TimeLine, anim->GetDurationInSeconds() ) * anim->GetDurationNormalize();
                if ( channelTimeLine < 0.0f ) {
                    channelTimeLine += 1.0f;
                }
#endif

                // adjust 0...framecount-1
                channelTimeLine = channelTimeLine * (float)( anim->GetFrameCount() - 1 );

                keyFrame = channelTimeLine.Floor();
                lerp = channelTimeLine.Fract();

                if ( channel->TimeLine < 0.0f ) {
                    channel->Frame = keyFrame + 1;
                    channel->NextFrame = keyFrame;
                    channel->Blend = Quantize( 1.0f - lerp, channel->Quantizer );
                } else {
                    channel->Frame = keyFrame;
                    channel->NextFrame = channel->Frame + 1;
                    channel->Blend = Quantize( lerp, channel->Quantizer );
                }
                break;

            case ANIMATION_PLAY_MIRROR:

                // normalize 0..1
                channelTimeLine = channel->TimeLine * anim->GetDurationNormalizer();
                take = channelTimeLine.Abs().Floor();
                channelTimeLine = channelTimeLine.Fract();

                // adjust 0...framecount-1
                channelTimeLine = channelTimeLine * (float)( anim->GetFrameCount() - 1 );

                keyFrame = channelTimeLine.Floor();
                lerp = channelTimeLine.Fract();

                if ( channel->TimeLine < 0.0f ) {
                    channel->Frame = keyFrame + 1;
                    channel->NextFrame = keyFrame;
                    channel->Blend = Quantize( 1.0f - lerp, channel->Quantizer );
                } else {
                    channel->Frame = keyFrame;
                    channel->NextFrame = channel->Frame + 1;
                    channel->Blend = Quantize( lerp, channel->Quantizer );
                }

                if ( take & 1 ) {
                    channel->Frame = anim->GetFrameCount() - channel->Frame - 1;
                    channel->NextFrame = anim->GetFrameCount() - channel->NextFrame - 1;
                }
                break;
            }
        } else if ( anim->GetFrameCount() == 1 ) {
            channel->Blend = 0;
            channel->Frame = 0;
            channel->NextFrame = 0;
        }
    }

    bUpdateChannels = false;
    bUpdateBounds = true;
    bUpdateRelativeTransforms = true;
}

void FSkinnedComponent::UpdateBounds() {

    UpdateChannelsIfDirty();

    if ( !bUpdateBounds || !Skeleton ) {
        return;
    }

    bUpdateBounds = false;

    Bounds.Clear();

    TPodArray< FSkeletonAnimation * > const & animations = Skeleton->GetAnimations();

    for ( int ch = 0 ; ch < AnimChannels.Length() ; ch++ ) {
        FAnimChannel const * channel = &AnimChannels[ ch ];
        FSkeletonAnimation const * anim = animations[ ch ];
        Bounds.AddAABB( anim->GetBoundingBoxes()[ channel->Frame ] );
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

        Float3x4 * transforms = WriteJointTransforms( joints.Length(), 0 );
        if ( transforms ) {
            for ( int j = 0 ; j < joints.Length() ; j++ ) {
                transforms[ j ] = AbsoluteMatrices[ j + 1 ] * joints[ j ].JointOffsetMatrix;
            }
        }
        bWriteTransforms = false;

        //GLogger.Printf( "Write transforms (%d)\n", GGameMaster.GetFrameNumber() );
    }
}

Float3x4 const & FSkinnedComponent::GetJointTransform( int _JointIndex ) {
    if ( !Skeleton || _JointIndex < 0 || _JointIndex >= Skeleton->GetJoints().Length() ) {
        return Float3x4::Identity();
    }

    MergeJointAnimations();

    return AbsoluteMatrices[_JointIndex+1];
}

Float3x4 * FSkinnedComponent::WriteJointTransforms( int _JointsCount, int _StartJointLocation ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();
    FRenderProxy_Skeleton::FrameData & data = RenderProxy->Data[ frameData->SmpIndex ];

    if ( !_JointsCount ) {
        return nullptr;
    }

    AN_Assert( _StartJointLocation + _JointsCount <= Skeleton->GetJoints().Length() );

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
//    for ( FSocket & socket : Sockets ) {
//        Float3x4 const & transform = GetJointTransform( socket.SocketDef->JointIndex );
//        _DebugDraw->DrawAxis( GetWorldTransformMatrix() * transform, true );
//    }

    // Draw skeleton
    if ( Skeleton ) {
        _DebugDraw->SetColor( 1,0,0,1 );
        _DebugDraw->SetDepthTest( false );
        TPodArray< FJoint > const & joints = Skeleton->GetJoints();
        for ( int i = 0 ; i < joints.Length() ; i++ ) {
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
