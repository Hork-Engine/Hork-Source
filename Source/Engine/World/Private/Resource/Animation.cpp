/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <World/Public/Resource/Animation.h>
#include <World/Public/Resource/Skeleton.h>
#include <World/Public/Resource/IndexedMesh.h>
#include <World/Public/Resource/Asset.h>

#include <Platform/Public/Logger.h>

AN_CLASS_META( ASkeletalAnimation )

ASkeletalAnimation::ASkeletalAnimation() {
}

ASkeletalAnimation::~ASkeletalAnimation() {
}

void ASkeletalAnimation::Purge() {
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

void ASkeletalAnimation::Initialize( int _FrameCount, float _FrameDelta, STransform const * _Transforms, int _TransformsCount, SAnimationChannel const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds ) {
    AN_ASSERT( _TransformsCount == _FrameCount * _NumAnimatedJoints );

    Channels.ResizeInvalidate( _NumAnimatedJoints );
    Platform::Memcpy( Channels.ToPtr(), _AnimatedJoints, sizeof( Channels[0] ) * _NumAnimatedJoints );

    Transforms.ResizeInvalidate( _TransformsCount );
    Platform::Memcpy( Transforms.ToPtr(), _Transforms, sizeof( Transforms[0] ) * _TransformsCount );

    Bounds.ResizeInvalidate( _FrameCount );
    Platform::Memcpy( Bounds.ToPtr(), _Bounds, sizeof( Bounds[0] ) * _FrameCount );

    if ( !Channels.IsEmpty() ) {
        MinNodeIndex = Math::MaxValue< int32_t >();
        MaxNodeIndex = 0;

        for ( int i = 0; i < Channels.Size(); i++ ) {
            MinNodeIndex = Math::Min( MinNodeIndex, Channels[ i ].JointIndex );
            MaxNodeIndex = Math::Max( MaxNodeIndex, Channels[ i ].JointIndex );
        }

        int mapSize = MaxNodeIndex - MinNodeIndex + 1;

        ChannelsMap.ResizeInvalidate( mapSize );

        for ( int i = 0; i < mapSize; i++ ) {
            ChannelsMap[ i ] = ( unsigned short )-1;
        }

        for ( int i = 0; i < Channels.Size(); i++ ) {
            ChannelsMap[ Channels[ i ].JointIndex - MinNodeIndex ] = i;
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

    bIsAnimationValid = _FrameCount > 0 && !Channels.IsEmpty();
}

void ASkeletalAnimation::LoadInternalResource( const char * _Path ) {
    Purge();
}

bool ASkeletalAnimation::LoadResource( IBinaryStream & Stream ) {
    AString guid;

    TPodVector< SAnimationChannel > channels;
    TPodVector< STransform > transforms;
    TPodVector< BvAxisAlignedBox > bounds;

    uint32_t fileFormat = Stream.ReadUInt32();

    if ( fileFormat != FMT_FILE_TYPE_ANIMATION ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_ANIMATION );
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if ( fileVersion != FMT_VERSION_ANIMATION ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_ANIMATION );
        return false;
    }

    Stream.ReadObject( guid );

    float frameDelta = Stream.ReadFloat();
    uint32_t frameCount = Stream.ReadUInt32();
    Stream.ReadArrayOfStructs( channels );
    Stream.ReadArrayOfStructs( transforms );
    Stream.ReadArrayOfStructs( bounds );

    Initialize( frameCount, frameDelta,
                transforms.ToPtr(), transforms.Size(),
                channels.ToPtr(), channels.Size(), bounds.ToPtr() );

    return true;
}
