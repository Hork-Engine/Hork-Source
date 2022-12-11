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

#include <Containers/Vector.h>
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/VertexFormat.h>
#include <Geometry/Transform.h>

constexpr int MAX_SKELETON_JOINTS = 256;

/**

MeshSkin

*/
struct MeshSkin
{
    /** Index of the joint in skeleton */
    TVector<int32_t> JointIndices;

    /** Transform vertex to joint-space */
    TVector<Float3x4> OffsetMatrices;
};

/**

SkeletonJoint

Joint properties

*/
struct SkeletonJoint
{
    /** Parent node index. For root = -1 */
    int32_t Parent;

    /** Joint local transform */
    Float3x4 LocalTransform;

    /** Joint name */
    char Name[64];

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.WriteInt32(Parent);
        _Stream.WriteObject(LocalTransform);
        _Stream.WriteString(Name);
    }

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        Parent = _Stream.ReadInt32();
        _Stream.ReadObject(LocalTransform);
        _Stream.ReadStringToBuffer(Name, sizeof(Name));
    }
};

/**

AnimationChannel

Animation for single joint

*/
struct AnimationChannel
{
    /** Joint index in skeleton */
    int32_t JointIndex;

    /** Joint frames */
    int32_t TransformOffset;

    bool bHasPosition : 1;
    bool bHasRotation : 1;
    bool bHasScale : 1;

    void Read(IBinaryStreamReadInterface& _Stream)
    {
        JointIndex      = _Stream.ReadInt32();
        TransformOffset = _Stream.ReadInt32();

        uint8_t bitMask = _Stream.ReadUInt8();

        bHasPosition = (bitMask >> 0) & 1;
        bHasRotation = (bitMask >> 1) & 1;
        bHasScale    = (bitMask >> 2) & 1;
    }

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        _Stream.WriteInt32(JointIndex);
        _Stream.WriteInt32(TransformOffset);
        _Stream.WriteUInt8((uint8_t(bHasPosition) << 0) | (uint8_t(bHasRotation) << 1) | (uint8_t(bHasScale) << 2));
    }
};

namespace Geometry
{

BvAxisAlignedBox CalcBindposeBounds(MeshVertex const*     Vertices,
                                    MeshVertexSkin const* Weights,
                                    int                    VertexCount,
                                    MeshSkin const*        Skin,
                                    SkeletonJoint*         Joints,
                                    int                    JointsCount);

void CalcBoundingBoxes(MeshVertex const*         Vertices,
                       MeshVertexSkin const*     Weights,
                       int                        VertexCount,
                       MeshSkin const*            Skin,
                       SkeletonJoint const*       Joints,
                       int                        NumJoints,
                       uint32_t                   FrameCount,
                       AnimationChannel const*    Channels,
                       int                        ChannelsCount,
                       Transform const*          Transforms,
                       TVector<BvAxisAlignedBox>& Bounds);

} // namespace Geometry
