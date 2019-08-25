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

#pragma once

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

struct FMeshAsset;
struct FJoint;

/*

FChannelTransform

Animation channel transform

*/
struct FChannelTransform {
    Quat   Rotation;
    Float3 Position;
    Float3 Scale;

    // Helper to convert joint transform to matrix 3x4
    void ToMatrix( Float3x4 & _Matrix ) const {
        _Matrix.Compose( Position, Rotation.ToMatrix(), Scale );
    }
};

/*

FAnimationChannel

Animation for single joint

*/
struct FAnimationChannel {
    // Node index (Joint index in skeleton)
    int NodeIndex;

    // Joint frames
    int TransformOffset;
};

/*

FAnimationAsset

Animation plain data

*/
struct FAnimationAsset {
    float FrameDelta;       // fixed time delta between frames
    int FrameCount;         // frames count, animation duration is FrameDelta * ( FrameCount - 1 )
    TPodArray< FAnimationChannel > Channels;
    TPodArray< FChannelTransform > Transforms;
    TPodArray< BvAxisAlignedBox > Bounds;
    FString Name;

    void Clear();
    void Read( FFileStream & f );
    void Write( FFileStream & f );
    void CalcBoundingBoxes( FMeshAsset const * InMeshData, FJoint const *  InJoints, int InNumJoints );
};

/*

FAnimation

Animation class

*/
class FAnimation : public FBaseObject {
    AN_CLASS( FAnimation, FBaseObject )

public:
    void Initialize( int _FrameCount, float _FrameDelta, FChannelTransform const * _Transforms, int _TransformsCount, FAnimationChannel const * _AnimatedJoints, int _NumAnimatedJoints, BvAxisAlignedBox const * _Bounds );

    // Initialize default object representation
    void InitializeDefaultObject() override;

    // Initialize object from file
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    void Purge();

    TPodArray< FAnimationChannel > const & GetChannels() const { return Channels; }
    TPodArray< FChannelTransform > const & GetTransforms() const { return Transforms; }

    unsigned short GetChannelIndex( int _NodeIndex ) const;

    int GetFrameCount() const { return FrameCount; }
    float GetFrameDelta() const { return FrameDelta; }
    float GetFrameRate() const { return FrameRate; }
    float GetDurationInSeconds() const { return DurationInSeconds; }
    float GetDurationNormalizer() const { return DurationNormalizer; }
    TPodArray< BvAxisAlignedBox > const & GetBoundingBoxes() const { return Bounds; }

protected:
    FAnimation();
    ~FAnimation();

private:
    TPodArray< FAnimationChannel > Channels;
    TPodArray< FChannelTransform > Transforms;
    TPodArray< unsigned short > ChannelsMap;
    int     MinNodeIndex;
    int     MaxNodeIndex;

    int     FrameCount;         // frames count
    float   FrameDelta;         // fixed time delta between frames
    float   FrameRate;          // frames per second (animation speed) FrameRate = 1.0 / FrameDelta
    float   DurationInSeconds;  // animation duration is FrameDelta * ( FrameCount - 1 )
    float   DurationNormalizer; // to normalize track timeline (DurationNormalizer = 1.0 / DurationInSeconds)

    TPodArray< BvAxisAlignedBox > Bounds;
};

AN_FORCEINLINE unsigned short FAnimation::GetChannelIndex( int _NodeIndex ) const {
    return ( _NodeIndex < MinNodeIndex || _NodeIndex > MaxNodeIndex ) ? (unsigned short)-1 : ChannelsMap[ _NodeIndex - MinNodeIndex ];
}
