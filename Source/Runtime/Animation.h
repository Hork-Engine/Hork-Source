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

#include "Resource.h"
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/Transform.h>


/**

SAnimationChannel

Animation for single joint

*/
struct SAnimationChannel {
    /** Joint index in skeleton */
    int32_t JointIndex;

    /** Joint frames */
    int32_t TransformOffset;

    bool bHasPosition : 1;
    bool bHasRotation : 1;
    bool bHasScale : 1;

    void Read( IBinaryStream & _Stream ) {
        JointIndex = _Stream.ReadInt32();
        TransformOffset = _Stream.ReadInt32();

        uint8_t bitMask = _Stream.ReadUInt8();

        bHasPosition = (bitMask>>0) & 1;
        bHasRotation = (bitMask>>1) & 1;
        bHasScale    = (bitMask>>2) & 1;
    }

    void Write( IBinaryStream & _Stream ) const {
        _Stream.WriteInt32( JointIndex );
        _Stream.WriteInt32( TransformOffset );
        _Stream.WriteUInt8(   ( uint8_t(bHasPosition) << 0 )
                            | ( uint8_t(bHasRotation) << 1 )
                            | ( uint8_t(bHasScale   ) << 2 ) );
    }
};

/**

ASkeletalAnimation

Animation class

*/
class ASkeletalAnimation : public AResource {
    AN_CLASS( ASkeletalAnimation, AResource )

public:
    ASkeletalAnimation();
    ~ASkeletalAnimation();

    void Initialize( int _FrameCount, float _FrameDelta, STransform const * _Transforms, int _TransformsCount, SAnimationChannel const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds );

    void Purge();

    TPodVector< SAnimationChannel > const & GetChannels() const { return Channels; }
    TPodVector< STransform > const & GetTransforms() const { return Transforms; }

    unsigned short GetChannelIndex( int _JointIndex ) const;

    int GetFrameCount() const { return FrameCount; }
    float GetFrameDelta() const { return FrameDelta; }
    float GetFrameRate() const { return FrameRate; }
    float GetDurationInSeconds() const { return DurationInSeconds; }
    float GetDurationNormalizer() const { return DurationNormalizer; }
    TPodVector< BvAxisAlignedBox > const & GetBoundingBoxes() const { return Bounds; }
    bool IsValid() const { return bIsAnimationValid; }

protected:
    /** Load resource from file */
    bool LoadResource( IBinaryStream & Stream ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/Animation/Default"; }

private:
    TPodVector< SAnimationChannel > Channels;
    TPodVector< STransform > Transforms;
    TPodVector< unsigned short > ChannelsMap;
    TPodVector< BvAxisAlignedBox > Bounds;
    int     MinNodeIndex = 0;
    int     MaxNodeIndex = 0;
    int     FrameCount = 0;     // frames count
    float   FrameDelta = 0;     // fixed time delta between frames
    float   FrameRate = 60;     // frames per second (animation speed) FrameRate = 1.0 / FrameDelta
    float   DurationInSeconds = 0;  // animation duration is FrameDelta * ( FrameCount - 1 )
    float   DurationNormalizer = 1; // to normalize track timeline (DurationNormalizer = 1.0 / DurationInSeconds)
    bool    bIsAnimationValid = false;
};

AN_FORCEINLINE unsigned short ASkeletalAnimation::GetChannelIndex( int _JointIndex ) const {
    return ( _JointIndex < MinNodeIndex || _JointIndex > MaxNodeIndex ) ? (unsigned short)-1 : ChannelsMap[ _JointIndex - MinNodeIndex ];
}
