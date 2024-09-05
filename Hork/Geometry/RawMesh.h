
/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/SmallString.h>
#include <Hork/Math/Quat.h>
#include <Hork/Geometry/VertexFormat.h>
#include <Hork/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

constexpr uint32_t MAX_SKELETON_JOINTS = 1024;

enum class RawMeshLoadFlags : uint32_t
{
    Surfaces = 1,
    Skins = 2,
    Skeleton = 4,
    Animation = 8,
    SingleAnimation = 16,
    All = 0xffffffff
};

HK_FLAG_ENUM_OPERATORS(RawMeshLoadFlags)

struct RawAnimation
{
    struct Channel
    {
        enum class ChannelType
        {
            Translation,
            Rotation,
            Scale,
            Weights // WIP
        };

        enum class InterpolationType
        {
            Linear,
            Step,
            CubicSpline
        };

        /// Type of data stored in the channel
        ChannelType                 Type;

        /// Data interpolation type
        InterpolationType           Interpolation;

        /// The index of the joint to which the channel belongs
        uint16_t                    JointIndex;

        /// Keyframes: e.g. translations : xyzxyzxyz...
        Vector<float>               Data;

        /// Timestamps, in seconds
        Vector<float>               Timestamps;
    };

    String                          Name;
    Vector<Channel>                 Channels;

    /// Default sample rate (optional)
    float                           SampleRate = 0;
};

struct RawSkeleton
{
    struct Joint
    {
        int16_t                     Parent;
        SmallString                 Name;
        Float3                      Position;
        Quat                        Rotation;
        Float3                      Scale;
    };

    Vector<Joint>                   Joints;
};

class RawMesh
{
public:
    struct Skin
    {
        Vector<uint16_t>            JointRemaps;
        Vector<Float3x4>            InverseBindPoses;

        /// Returns the number of joints used to skin the mesh
        int GetJointCount() const
        {
            return static_cast<int>(InverseBindPoses.Size());
        }

        /// Returns the highest joint number used in the skeleton
        int HighestJointIndex() const
        {
            return JointRemaps.Size() != 0 ? static_cast<int>(JointRemaps.Last()) : 0;
        }
    };

    struct Surface
    {
        /// Surface vertex positions
        Vector<Float3>              Positions;

        /// Primary UV channel
        Vector<Float2>              TexCoords;

        /// Secondary UV channel. Can be used to store lightmap UV.
        Vector<Float2>              TexCoords2;

        /// Per-vertex normals
        Vector<Float3>              Normals;

        /// Per-vertex tangents
        Vector<Float4>              Tangents;

        /// Skinning vertices are used in skinning calculations. The data contains the indices
        /// of the joints from the surface skin that affect the vertex and the weights indicating
        /// how strongly the joint influences the vertex.
        Vector<SkinVertex>          SkinVerts;

        /// Triangle indices
        Vector<uint32_t>            Indices;

        /// Surface skin.
        RawMesh::Skin*              Skin{};

        /// The joint index associated with the surface.
        uint16_t                    JointIndex{};

        /// Non-skinned surfaces are stored with pretransformed vertices. For animated
        /// meshes its required to perform animation in local space, so we use InverseTransform
        /// to transform vertices back to local space.

        /// Non-skinned surfaces are stored with pre-transformed vertices. Animated meshes need
        /// to animate in local space, so we use InverseTransform to transform the vertices back to local space.
        Float3x4                    InverseTransform;

        /// Surface bounding box. For skinned surfaces, the bounding box is calculated for the resting pose.
        /// To calculate an accurate bounding box for skinning vertices, you need to pre-process the skinning
        /// and calculate the bounding box after all animations have been applied.
        BvAxisAlignedBox            BoundingBox;

        /// Checking whether the surface has skinning information
        bool IsSkinned() const
        {
            return Skin != nullptr && !SkinVerts.IsEmpty();
        }
    };

    Vector<UniqueRef<Surface>>      Surfaces;
    Vector<UniqueRef<Skin>>         Skins;
    RawSkeleton                     Skeleton;
    Vector<UniqueRef<RawAnimation>> Animations;

    Surface*                        AllocSurface();
    Skin*                           AllocSkin();
    RawAnimation*                   AllocAnimation();
    void                            Purge();

    bool                            Load(StringView filename, RawMeshLoadFlags flags = RawMeshLoadFlags::All);

    /// Load mesh from Wavefront OBJ format
    bool                            LoadOBJ(IBinaryStreamReadInterface& stream, RawMeshLoadFlags flags = RawMeshLoadFlags::All);

    /// Load mesh from GLTF/GLB format
    bool                            LoadGLTF(IBinaryStreamReadInterface& stream, RawMeshLoadFlags flags = RawMeshLoadFlags::All);

    /// Load mesh from FBX format
    bool                            LoadFBX(IBinaryStreamReadInterface& stream, RawMeshLoadFlags flags = RawMeshLoadFlags::All);

    void                            CreateBox(Float3 const& extents, float texCoordScale = 1.0f);
    void                            CreateSphere(float radius, float texCoordScale = 1.0f, int numVerticalSubdivs = 32, int numHorizontalSubdivs = 32);
    void                            CreatePlaneXZ(float width, float height, Float2 const& texCoordScale = Float2(1));
    void                            CreatePlaneXY(float width, float height, Float2 const& texCoordScale = Float2(1));
    void                            CreatePatch(Float3 const& corner00, Float3 const& corner10, Float3 const& corner01, Float3 const& corner11, float texCoordScale, bool isTwoSided, int numVerticalSubdivs, int numHorizontalSubdivs);
    void                            CreateCylinder(float radius, float height, float texCoordScale = 1.0f, int numSubdivs = 32);
    void                            CreateCone(float radius, float height, float texCoordScale = 1.0f, int numSubdivs = 32);
    void                            CreateCapsule(float radius, float height, float texCoordScale = 1.0f, int numVerticalSubdivs = 6, int numHorizontalSubdivs = 8);
    void                            CreateSkybox(Float3 const& extents, float texCoordScale = 1.0f);
    void                            CreateSkydome(float radius, float texCoordScale = 1.0f, int numVerticalSubdivs = 32, int numHorizontalSubdivs = 32, bool isHemisphere = true);

    BvAxisAlignedBox                CalcBoundingBox() const;
};

HK_NAMESPACE_END
