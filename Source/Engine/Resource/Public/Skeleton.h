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

/**

FJoint

Joint properties

*/
struct FJoint {
    int      Parent;                 // Parent joint index. For root = -1
    Float3x4 OffsetMatrix;           // Transform vertex to joint-space
    Float3x4 LocalTransform;         // Joint local transform
    char     Name[64];               // Joint name
};

/**

FSkeletonAsset

Skeleton plain data

*/
struct FSkeletonAsset {
    TPodArray< FJoint > Joints;
    BvAxisAlignedBox BindposeBounds;

    void Clear();
    void Read( FFileStream & f );
    void Write( FFileStream & f );
    void CalcBindposeBounds( FMeshAsset const * InMeshData );
};

///**

//FSocketDef

//Socket for attaching

//*/
//class FSocketDef : public FBaseObject {
//    AN_CLASS( FSocketDef, FBaseObject )

//public:
//    Float3 Position;
//    Float3 Scale;
//    Quat Rotation;
//    int JointIndex;

//protected:
//    FSocketDef() : Position(0.0f), Scale(1.0f), Rotation(Quat::Identity()), JointIndex(-1)
//    {
//    }
//};

/**

FSkeleton

Skeleton structure

*/
class FSkeleton : public FBaseObject {
    AN_CLASS( FSkeleton, FBaseObject )

public:
    enum { MAX_JOINTS = 256 };

    void Initialize( FJoint * _Joints, int _JointsCount, BvAxisAlignedBox const & _BindposeBounds );

    /** Initialize internal resource */
    void InitializeInternalResource( const char * _InternalResourceName ) override;

    /** Initialize object from file */
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    void Purge();

    int FindJoint( const char * _Name ) const;

    TPodArray< FJoint > const & GetJoints() const { return Joints; }

    //void AddSocket( FSocketDef * _Socket );
    // TODO: RemoveSocket?

    //FSocketDef * FindSocket( const char * _Name );

    //TPodArray< FSocketDef * > const & GetSockets() const { return Sockets; }

    BvAxisAlignedBox const & GetBindposeBounds() const { return BindposeBounds; }

protected:
    FSkeleton();
    ~FSkeleton();

private:
    TPodArray< FJoint > Joints;
    //TPodArray< FSocketDef * > Sockets;
    BvAxisAlignedBox BindposeBounds;
};
