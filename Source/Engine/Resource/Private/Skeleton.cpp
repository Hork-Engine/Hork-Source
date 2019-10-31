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

#include <Engine/Resource/Public/Skeleton.h>
#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/Resource/Public/Asset.h>

#include <Engine/Core/Public/Logger.h>

//AN_CLASS_META( FSocketDef )
AN_CLASS_META( FSkeleton )

///////////////////////////////////////////////////////////////////////////////////////////////////////

FSkeleton::FSkeleton() {
}

FSkeleton::~FSkeleton() {
    Purge();
}

void FSkeleton::Purge() {
    Joints.Clear();

//    for ( FSocketDef * socket : Sockets ) {
//        socket->RemoveRef();
//    }

//    Sockets.Clear();

    // TODO: notify components about changes
}

void FSkeleton::Initialize( FJoint * _Joints, int _JointsCount, BvAxisAlignedBox const & _BindposeBounds ) {
    Purge();

    Joints.ResizeInvalidate( _JointsCount );
    memcpy( Joints.ToPtr(), _Joints, sizeof( *_Joints ) * _JointsCount );

    BindposeBounds = _BindposeBounds;

    // TODO: notify components about changes
}

void FSkeleton::InitializeInternalResource( const char * _InternalResourceName ) {
    Purge();

    //SetName( _InternalResourceName );
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

//FSocketDef * FSkeleton::FindSocket( const char * _Name ) {
//    for ( FSocketDef * socket : Sockets ) {
//        if ( socket->GetName().Icmp( _Name ) ) {
//            return socket;
//        }
//    }
//    return nullptr;
//}

//void FSkeleton::AddSocket( FSocketDef * _Socket ) {
//    _Socket->AddRef();
//    Sockets.Append( _Socket );

//    // TODO: notify components about changes
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void FSkeletonAsset::Clear() {
    Joints.Clear();
    BindposeBounds.Clear();
}

void FSkeletonAsset::Read( FFileStream & f ) {
    char buf[1024];
    char * s;
    int format, version;

    Clear();

    if ( !AssetReadFormat( f, &format, &version ) ) {
        return;
    }

    if ( format != FMT_FILE_TYPE_SKELETON ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_SKELETON );
        return;
    }

    if ( version != FMT_VERSION_SKELETON ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_SKELETON );
        return;
    }

    while ( f.Gets( buf, sizeof( buf ) ) ) {
        if ( nullptr != ( s = AssetParseTag( buf, "joints " ) ) ) {
            int numJoints = 0;
            sscanf( s, "%d", &numJoints );
            Joints.ResizeInvalidate( numJoints );
            for ( int jointIndex = 0 ; jointIndex < numJoints ; jointIndex++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                char * name;
                s = AssetParseName( buf, &name );

                FJoint & joint = Joints[jointIndex];
                FString::CopySafe( joint.Name, sizeof( joint.Name ), name );

                joint.LocalTransform.SetIdentity();

                sscanf( s, "%d ( ( %f %f %f %f ) ( %f %f %f %f ) ( %f %f %f %f ) ) ( ( %f %f %f %f ) ( %f %f %f %f ) ( %f %f %f %f ) )", &joint.Parent,
                        &joint.OffsetMatrix[0][0], &joint.OffsetMatrix[0][1], &joint.OffsetMatrix[0][2], &joint.OffsetMatrix[0][3],
                        &joint.OffsetMatrix[1][0], &joint.OffsetMatrix[1][1], &joint.OffsetMatrix[1][2], &joint.OffsetMatrix[1][3],
                        &joint.OffsetMatrix[2][0], &joint.OffsetMatrix[2][1], &joint.OffsetMatrix[2][2], &joint.OffsetMatrix[2][3],
                        &joint.LocalTransform[0][0], &joint.LocalTransform[0][1], &joint.LocalTransform[0][2], &joint.LocalTransform[0][3],
                        &joint.LocalTransform[1][0], &joint.LocalTransform[1][1], &joint.LocalTransform[1][2], &joint.LocalTransform[1][3],
                        &joint.LocalTransform[2][0], &joint.LocalTransform[2][1], &joint.LocalTransform[2][2], &joint.LocalTransform[2][3] );
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "bindpose_bounds " ) ) ) {

            sscanf( s, "( %f %f %f ) ( %f %f %f )",
                    &BindposeBounds.Mins.X, &BindposeBounds.Mins.Y, &BindposeBounds.Mins.Z,
                    &BindposeBounds.Maxs.X, &BindposeBounds.Maxs.Y, &BindposeBounds.Maxs.Z );

        } else {
            GLogger.Printf( "Unknown tag '%s'\n", buf );
        }
    }
}

void FSkeletonAsset::Write( FFileStream & f ) {
    f.Printf( "format %d %d\n", FMT_FILE_TYPE_SKELETON, FMT_VERSION_SKELETON );
    f.Printf( "joints %d\n", Joints.Size() );
    for ( FJoint & joint : Joints ) {
        f.Printf( "\"%s\" %d %s %s\n",
                  joint.Name, joint.Parent,
                  joint.OffsetMatrix.ToString().ToConstChar(),
                  joint.LocalTransform.ToString().ToConstChar() );
    }
    f.Printf( "bindpose_bounds %s %s\n", BindposeBounds.Mins.ToString().ToConstChar(), BindposeBounds.Maxs.ToString().ToConstChar() );
}

void FSkeletonAsset::CalcBindposeBounds( FMeshAsset const * InMeshData ) {
    Float3x4 absoluteTransforms[FSkeleton::MAX_JOINTS+1];
    Float3x4 vertexTransforms[FSkeleton::MAX_JOINTS];

    BindposeBounds.Clear();

    absoluteTransforms[0].SetIdentity();
    for ( unsigned int j = 0 ; j < Joints.Size() ; j++ ) {
        FJoint const & joint = Joints[ j ];

        absoluteTransforms[ j + 1 ] = absoluteTransforms[ joint.Parent + 1 ] * joint.LocalTransform;

        vertexTransforms[ j ] = absoluteTransforms[ j + 1 ] * joint.OffsetMatrix;
    }

    for ( int v = 0 ; v < InMeshData->Vertices.Size() ; v++ ) {
        Float4 const position = Float4( InMeshData->Vertices[v].Position, 1.0f );
        FMeshVertexJoint const & w = InMeshData->Weights[v];

        const float weights[4] = { w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f };

        Float4 const * t = &vertexTransforms[0][0];

        BindposeBounds.AddPoint(
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
