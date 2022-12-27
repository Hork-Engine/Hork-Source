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

#include "Skinning.h"

HK_NAMESPACE_BEGIN

namespace Geometry
{

BvAxisAlignedBox CalcBindposeBounds(MeshVertex const*     Vertices,
                                    MeshVertexSkin const* Weights,
                                    int                    VertexCount,
                                    MeshSkin const*           Skin,
                                    SkeletonJoint*                Joints,
                                    int                    JointsCount)
{
    Float3x4 absoluteTransforms[MAX_SKELETON_JOINTS + 1];
    Float3x4 vertexTransforms[MAX_SKELETON_JOINTS];

    BvAxisAlignedBox BindposeBounds;

    BindposeBounds.Clear();

    absoluteTransforms[0].SetIdentity();
    for (unsigned int j = 0; j < JointsCount; j++)
    {
        SkeletonJoint const& joint = Joints[j];

        absoluteTransforms[j + 1] = absoluteTransforms[joint.Parent + 1] * joint.LocalTransform;
    }

    for (unsigned int j = 0; j < Skin->JointIndices.Size(); j++)
    {
        int jointIndex = Skin->JointIndices[j];

        vertexTransforms[j] = absoluteTransforms[jointIndex + 1] * Skin->OffsetMatrices[j];
    }

    for (int v = 0; v < VertexCount; v++)
    {
        Float4 const           position = Float4(Vertices[v].Position, 1.0f);
        MeshVertexSkin const& w        = Weights[v];

        const float weights[4] = {w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f};

        Float4 const* t = &vertexTransforms[0][0];

        BindposeBounds.AddPoint(
            Math::Dot((t[w.JointIndices[0] * 3 + 0] * weights[0] + t[w.JointIndices[1] * 3 + 0] * weights[1] + t[w.JointIndices[2] * 3 + 0] * weights[2] + t[w.JointIndices[3] * 3 + 0] * weights[3]), position),

            Math::Dot((t[w.JointIndices[0] * 3 + 1] * weights[0] + t[w.JointIndices[1] * 3 + 1] * weights[1] + t[w.JointIndices[2] * 3 + 1] * weights[2] + t[w.JointIndices[3] * 3 + 1] * weights[3]), position),

            Math::Dot((t[w.JointIndices[0] * 3 + 2] * weights[0] + t[w.JointIndices[1] * 3 + 2] * weights[1] + t[w.JointIndices[2] * 3 + 2] * weights[2] + t[w.JointIndices[3] * 3 + 2] * weights[3]), position));
    }

    return BindposeBounds;
}

void CalcBoundingBoxes(MeshVertex const*         Vertices,
                       MeshVertexSkin const*     Weights,
                       int                        VertexCount,
                       MeshSkin const*               Skin,
                       SkeletonJoint const*              Joints,
                       int                        NumJoints,
                       uint32_t                   FrameCount,
                       AnimationChannel const*   Channels,
                       int                        ChannelsCount,
                       Transform const*          Transforms,
                       TVector<BvAxisAlignedBox>& Bounds)
{
    Float3x4          absoluteTransforms[MAX_SKELETON_JOINTS + 1];
    TVector<Float3x4> relativeTransforms[MAX_SKELETON_JOINTS];
    Float3x4          vertexTransforms[MAX_SKELETON_JOINTS];

    Bounds.ResizeInvalidate(FrameCount);

    for (int i = 0; i < ChannelsCount; i++)
    {
        AnimationChannel const& anim = Channels[i];

        relativeTransforms[anim.JointIndex].ResizeInvalidate(FrameCount);

        for (int frameNum = 0; frameNum < FrameCount; frameNum++)
        {

            Transform const& transform = Transforms[anim.TransformOffset + frameNum];

            transform.ComputeTransformMatrix(relativeTransforms[anim.JointIndex][frameNum]);
        }
    }

    for (int frameNum = 0; frameNum < FrameCount; frameNum++)
    {

        BvAxisAlignedBox& bounds = Bounds[frameNum];

        bounds.Clear();

        absoluteTransforms[0].SetIdentity();
        for (unsigned int j = 0; j < NumJoints; j++)
        {
            SkeletonJoint const& joint = Joints[j];

            Float3x4 const& parentTransform = absoluteTransforms[joint.Parent + 1];

            if (relativeTransforms[j].IsEmpty())
            {
                absoluteTransforms[j + 1] = parentTransform * joint.LocalTransform;
            }
            else
            {
                absoluteTransforms[j + 1] = parentTransform * relativeTransforms[j][frameNum];
            }
        }

        for (unsigned int j = 0; j < Skin->JointIndices.Size(); j++)
        {
            int jointIndex = Skin->JointIndices[j];

            vertexTransforms[j] = absoluteTransforms[jointIndex + 1] * Skin->OffsetMatrices[j];
        }

        for (int v = 0; v < VertexCount; v++)
        {
            Float4 const           position = Float4(Vertices[v].Position, 1.0f);
            MeshVertexSkin const& w        = Weights[v];

            const float weights[4] = {w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f};

            Float4 const* t = &vertexTransforms[0][0];

            bounds.AddPoint(
                Math::Dot((t[w.JointIndices[0] * 3 + 0] * weights[0] + t[w.JointIndices[1] * 3 + 0] * weights[1] + t[w.JointIndices[2] * 3 + 0] * weights[2] + t[w.JointIndices[3] * 3 + 0] * weights[3]), position),

                Math::Dot((t[w.JointIndices[0] * 3 + 1] * weights[0] + t[w.JointIndices[1] * 3 + 1] * weights[1] + t[w.JointIndices[2] * 3 + 1] * weights[2] + t[w.JointIndices[3] * 3 + 1] * weights[3]), position),

                Math::Dot((t[w.JointIndices[0] * 3 + 2] * weights[0] + t[w.JointIndices[1] * 3 + 2] * weights[1] + t[w.JointIndices[2] * 3 + 2] * weights[2] + t[w.JointIndices[3] * 3 + 2] * weights[3]), position));
        }
    }
}

} // namespace Geometry

HK_NAMESPACE_END
