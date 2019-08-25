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

#include <Engine/Resource/Public/Animation.h>
#include <Engine/Resource/Public/Skeleton.h>
#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/Resource/Public/Asset.h>

#include <Engine/Core/Public/Logger.h>

AN_CLASS_META( FAnimation )

FAnimation::FAnimation() {
    FrameCount = 0;
    FrameDelta = 0;
    FrameRate = 60.0f;
    DurationNormalizer = 1.0f;
}

FAnimation::~FAnimation() {
}

void FAnimation::Purge() {
    Channels.Clear();
    Transforms.Clear();
    Bounds.Clear();
    MinNodeIndex = 0;
    MaxNodeIndex = 0;
    ChannelsMap.Clear();
    FrameCount = 0;
    FrameDelta = 0;
    FrameRate = 0;
    DurationInSeconds = 0;
    DurationNormalizer = 1.0f;
}

void FAnimation::Initialize( int _FrameCount, float _FrameDelta, FChannelTransform const * _Transforms, int _TransformsCount, FAnimationChannel const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds ) {
    AN_Assert( _TransformsCount == _FrameCount * _NumAnimatedJoints );

    Channels.ResizeInvalidate( _NumAnimatedJoints );
    memcpy( Channels.ToPtr(), _AnimatedJoints, sizeof( Channels[0] ) * _NumAnimatedJoints );

    Transforms.ResizeInvalidate( _TransformsCount );
    memcpy( Transforms.ToPtr(), _Transforms, sizeof( Transforms[0] ) * _TransformsCount );

    Bounds.ResizeInvalidate( _FrameCount );
    memcpy( Bounds.ToPtr(), _Bounds, sizeof( Bounds[0] ) * _FrameCount );

    if ( !Channels.IsEmpty() ) {
        MinNodeIndex = Int().MaxValue();
        MaxNodeIndex = 0;

        for ( int i = 0; i < Channels.Size(); i++ ) {
            MinNodeIndex = FMath::Min( MinNodeIndex, Channels[ i ].NodeIndex );
            MaxNodeIndex = FMath::Max( MaxNodeIndex, Channels[ i ].NodeIndex );
        }

        int mapSize = MaxNodeIndex - MinNodeIndex + 1;

        ChannelsMap.ResizeInvalidate( mapSize );

        for ( int i = 0; i < mapSize; i++ ) {
            ChannelsMap[ i ] = ( unsigned short )-1;
        }

        for ( int i = 0; i < Channels.Size(); i++ ) {
            ChannelsMap[ Channels[ i ].NodeIndex - MinNodeIndex ] = i;
        }
    } else {
        MinNodeIndex = 0;
        MaxNodeIndex = 0;

        ChannelsMap.Clear();
    }

    FrameCount = _FrameCount;
    FrameDelta = _FrameDelta;
    FrameRate = 1.0f / _FrameDelta;
    DurationInSeconds = ( FrameCount - 1 ) * FrameDelta;
    DurationNormalizer = 1.0f / DurationInSeconds;
}

void FAnimation::InitializeDefaultObject() {
    Purge();
}

bool FAnimation::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    FFileStream f;

    if ( !f.OpenRead( _Path ) ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    FAnimationAsset asset;
    asset.Read( f );

    Initialize( asset.FrameCount, asset.FrameDelta,
                asset.Transforms.ToPtr(), asset.Transforms.Size(),
                asset.Channels.ToPtr(), asset.Channels.Size(), asset.Bounds.ToPtr() );

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void FAnimationAsset::Clear() {
    FrameDelta = 0;
    FrameCount = 0;
    Channels.Clear();
    Transforms.Clear();
    Name.Clear();
    Bounds.Clear();
}

void FAnimationAsset::CalcBoundingBoxes( FMeshAsset const * InMeshData, FJoint const *  InJoints, int InNumJoints ) {
    Float3x4 absoluteTransforms[FSkeleton::MAX_JOINTS+1];
    TPodArray< Float3x4 > relativeTransforms[FSkeleton::MAX_JOINTS];
    Float3x4 vertexTransforms[FSkeleton::MAX_JOINTS];

    Bounds.ResizeInvalidate( FrameCount );

    for ( FAnimationChannel & anim : Channels ) {

        relativeTransforms[anim.NodeIndex].ResizeInvalidate( FrameCount );

        for ( int frameNum = 0; frameNum < FrameCount ; frameNum++ ) {

            FChannelTransform const & transform = Transforms[ anim.TransformOffset + frameNum ];

            transform.ToMatrix( relativeTransforms[anim.NodeIndex][frameNum] );
        }
    }

    for ( int frameNum = 0 ; frameNum < FrameCount ; frameNum++ ) {

        BvAxisAlignedBox & bounds = Bounds[frameNum];

        bounds.Clear();

        absoluteTransforms[0].SetIdentity();
        for ( unsigned int j = 0 ; j < InNumJoints ; j++ ) {
            FJoint const & joint = InJoints[ j ];

            Float3x4 const & parentTransform = absoluteTransforms[ joint.Parent + 1 ];

            if ( relativeTransforms[j].IsEmpty() ) {
                absoluteTransforms[ j + 1 ] = parentTransform * joint.LocalTransform;
            } else {
                absoluteTransforms[ j + 1 ] = parentTransform * relativeTransforms[j][ frameNum ];
            }

            vertexTransforms[ j ] = absoluteTransforms[ j + 1 ] * joint.OffsetMatrix;
        }

        for ( int v = 0 ; v < InMeshData->Vertices.Size() ; v++ ) {
            Float4 const position = Float4( InMeshData->Vertices[v].Position, 1.0f );
            FMeshVertexJoint const & w = InMeshData->Weights[v];

            const float weights[4] = { w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f };

            Float4 const * t = &vertexTransforms[0][0];

            bounds.AddPoint(
                    ( t[ w.JointIndices[0] * 3 + 0 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 0 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 0 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 0 ] * weights[3] ).Dot( position ),

                    ( t[ w.JointIndices[0] * 3 + 1 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 1 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 1 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 1 ] * weights[3] ).Dot( position ),

                    ( t[ w.JointIndices[0] * 3 + 2 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 2 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 2 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 2 ] * weights[3] ).Dot( position ) );
        }
    }
}

void FAnimationAsset::Read( FFileStream & f ) {
    char buf[ 1024 ];
    char * s;
    int format, version;

    Clear();

    if ( !AssetReadFormat( f, &format, &version ) ) {
        return;
    }

    if ( format != FMT_FILE_TYPE_ANIMATION ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_ANIMATION );
        return;
    }

    if ( version != FMT_VERSION_ANIMATION ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_ANIMATION );
        return;
    }

    while ( f.Gets( buf, sizeof( buf ) ) ) {
        if ( nullptr != ( s = AssetParseTag( buf, "animation " ) ) ) {

            Clear();

            char * name;
            s = AssetParseName( s, &name );

            Name = name;

            sscanf( s, "%f %d\n", &FrameDelta, &FrameCount );
        } else if ( nullptr != ( s = AssetParseTag( buf, "anim_joints " ) ) ) {
            int numJoints = 0;
            sscanf( s, "%d", &numJoints );
            Channels.ResizeInvalidate( numJoints );
            Transforms.ResizeInvalidate( numJoints * FrameCount );
            int transformOffset = 0;
            for ( int jointIndex = 0; jointIndex < numJoints; jointIndex++ ) {
                FAnimationChannel & janim = Channels[ jointIndex ];
                int numFrames = 0;

                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                sscanf( buf, "%d %d", &janim.NodeIndex, &numFrames );

                AN_Assert( numFrames == FrameCount );

                janim.TransformOffset = transformOffset;

                for ( int j = 0; j < numFrames; j++ ) {
                    FChannelTransform & transform = Transforms[ transformOffset + j ];

                    if ( !f.Gets( buf, sizeof( buf ) ) ) {
                        GLogger.Printf( "Unexpected EOF\n" );
                        return;
                    }

                    sscanf( buf, "( %f %f %f %f ) ( %f %f %f )  ( %f %f %f )",
                        &transform.Rotation.X, &transform.Rotation.Y, &transform.Rotation.Z, &transform.Rotation.W,
                        &transform.Position.X, &transform.Position.Y, &transform.Position.Z,
                        &transform.Scale.X, &transform.Scale.Y, &transform.Scale.Z );
                }

                transformOffset += numFrames;
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "bounds" ) ) ) {
            Bounds.ResizeInvalidate( FrameCount );
            for ( int frameIndex = 0; frameIndex < FrameCount; frameIndex++ ) {
                BvAxisAlignedBox & bv = Bounds[ frameIndex ];

                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                sscanf( buf, "( %f %f %f ) ( %f %f %f )",
                    &bv.Mins.X, &bv.Mins.Y, &bv.Mins.Z,
                    &bv.Maxs.X, &bv.Maxs.Y, &bv.Maxs.Z );
            }
        } else {
            GLogger.Printf( "Unknown tag '%s'\n", buf );
        }
    }
}

void FAnimationAsset::Write( FFileStream & f ) {
    f.Printf( "format %d %d\n", FMT_FILE_TYPE_ANIMATION, FMT_VERSION_ANIMATION );
    f.Printf( "animation \"%s\" %f %d\n", Name.ToConstChar(), FrameDelta, FrameCount );
    f.Printf( "anim_joints %d\n", Channels.Size() );
    for ( FAnimationChannel & janim : Channels ) {
        f.Printf( "%d %d\n", janim.NodeIndex, FrameCount );

        for ( int frame = 0; frame < FrameCount; frame++ ) {
            FChannelTransform & transform = Transforms[ janim.TransformOffset + frame ];
            f.Printf( "%s %s %s\n",
                transform.Rotation.ToString().ToConstChar(),
                transform.Position.ToString().ToConstChar(),
                transform.Scale.ToString().ToConstChar()
            );
        }
    }
    f.Printf( "bounds\n" );
    for ( BvAxisAlignedBox & bounds : Bounds ) {
        f.Printf( "%s %s\n", bounds.Mins.ToString().ToConstChar(), bounds.Maxs.ToString().ToConstChar() );
    }
}
