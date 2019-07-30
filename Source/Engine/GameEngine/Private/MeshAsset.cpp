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

#include <Engine/GameEngine/Public/MeshAsset.h>

#define FMT_FILE_TYPE_MESH      1
#define FMT_FILE_TYPE_SKELETON  2
#define FMT_VERSION_MESH        1
#define FMT_VERSION_SKELETON    1

AN_FORCEINLINE char * ParseTag( char * _Buf, const char * _Tag ) {
    int n = strlen( _Tag );
    if ( !FString::CmpN( _Buf, _Tag, n ) ) {
        _Buf += n;
        return _Buf;
    }
    return nullptr;
}

AN_FORCEINLINE char * ParseName( char * _Buf, char ** _Name ) {
    char * s = _Buf;
    while ( *s && *s != '\"' ) {
        s++;
    }
    char * name = s;
    if ( *s ) {
        s++;
        name++;
    }
    while ( *s && *s != '\"' ) {
        s++;
    }
    if ( *s ) {
        *s = 0;
        s++;
    }
    *_Name = name;
    return s;
}

static bool ReadFileFormat( FFileStream & f, int * _Format, int * _Version ) {
    char buf[1024];
    char * s;

    if ( !f.Gets( buf, sizeof( buf ) ) || nullptr == ( s = ParseTag( buf, "format " ) ) ) {
        GLogger.Printf( "Expected format description\n" );
        return false;
    }

    if ( sscanf( s, "%d %d", _Format, _Version ) != 2 ) {
        GLogger.Printf( "Expected format type and version\n" );
        return false;
    }
    return true;
}

void FMeshAsset::Clear() {
    Subparts.clear();
    Textures.clear();
    Materials.Clear();
    Vertices.Clear();
    Indices.Clear();
    Weights.Clear();
}

void FMeshAsset::Read( FFileStream & f ) {
    char buf[1024];
    char * s;
    int format, version;

    Clear();

    if ( !ReadFileFormat( f, &format, &version ) ) {
        return;
    }

    if ( format != FMT_FILE_TYPE_MESH ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_MESH );
        return;
    }

    if ( version != FMT_VERSION_MESH ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_MESH );
        return;
    }

    while ( f.Gets( buf, sizeof( buf ) ) ) {
        if ( nullptr != ( s = ParseTag( buf, "textures " ) ) ) {
            int numTextures = 0;
            sscanf( s, "%d", &numTextures );
            Textures.resize( numTextures );
            for ( int i = 0 ; i < numTextures ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }
                for ( s = buf ; *s && *s != '\n' ; s++ ) {} *s = 0;
                Textures[i].FileName = buf;
            }
        } else if ( nullptr != ( s = ParseTag( buf, "materials " ) ) ) {
            int numMaterials = 0;
            sscanf( s, "%d", &numMaterials );
            Materials.ResizeInvalidate( numMaterials );
            for ( int i = 0 ; i < numMaterials ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }
                if ( nullptr != ( s = ParseTag( buf, "maps " ) ) ) {
                    Materials[i].NumTextures = 0;
                    sscanf( s, "%d", &Materials[i].NumTextures );
                    for ( int j = 0 ; j < Materials[i].NumTextures ; j++ ) {
                        if ( !f.Gets( buf, sizeof( buf ) ) ) {
                            GLogger.Printf( "Unexpected EOF\n" );
                            return;
                        }
                        Materials[i].Textures[j] = Int().FromString( buf );
                    }
                }
            }
        } else if ( nullptr != ( s = ParseTag( buf, "subparts " ) ) ) {
            int numSubparts = 0;
            sscanf( s, "%d", &numSubparts );
            Subparts.resize( numSubparts );
            for ( int i = 0 ; i < numSubparts ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                char * name;
                s = ParseName( buf, &name );

                FSubpart & subpart = Subparts[i];
                subpart.Name = name;
                sscanf( s, "%d %d %d %d %d ( %f %f %f ) ( %f %f %f )", &subpart.BaseVertex, &subpart.VertexCount, &subpart.FirstIndex, &subpart.IndexCount, &subpart.Material,
                        &subpart.BoundingBox.Mins.X.Value, &subpart.BoundingBox.Mins.Y.Value, &subpart.BoundingBox.Mins.Z.Value,
                        &subpart.BoundingBox.Maxs.X.Value, &subpart.BoundingBox.Maxs.Y.Value, &subpart.BoundingBox.Maxs.Z.Value );
            }
        } else if ( nullptr != ( s = ParseTag( buf, "verts " ) ) ) {
            int numVerts = 0;
            sscanf( s, "%d", &numVerts );
            Vertices.ResizeInvalidate( numVerts );
            for ( int i = 0 ; i < numVerts ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                FMeshVertex & v = Vertices[i];

                sscanf( buf, "( %f %f %f ) ( %f %f ) ( %f %f %f ) %f ( %f %f %f )\n",
                        &v.Position.X.Value,&v.Position.Y.Value,&v.Position.Z.Value,
                        &v.TexCoord.X.Value,&v.TexCoord.Y.Value,
                        &v.Tangent.X.Value,&v.Tangent.Y.Value,&v.Tangent.Z.Value,
                        &v.Handedness,
                        &v.Normal.X.Value,&v.Normal.Y.Value,&v.Normal.Z.Value );
            }
        } else if ( nullptr != ( s = ParseTag( buf, "indices " ) ) ) {
            int numIndices = 0;
            sscanf( s, "%d", &numIndices );
            Indices.ResizeInvalidate( numIndices );
            for ( int i = 0 ; i < numIndices ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                Indices[i] = UInt().FromString( buf );
            }
        } else if ( nullptr != ( s = ParseTag( buf, "weights " ) ) ) {
            int numWeights = 0;
            sscanf( s, "%d", &numWeights );
            Weights.ResizeInvalidate( numWeights );
            for ( int i = 0 ; i < numWeights ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                FMeshVertexJoint & w = Weights[i];

                int d[8];

                sscanf( buf, "%d %d %d %d %d %d %d %d", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7] );

                for ( int n = 0 ; n < 8 ; n++ ) {
                    d[n] = FMath::Max( d[n], 0 );
                    d[n] = FMath::Min( d[n], 255 );
                }

                w.JointIndices[0] = d[0];
                w.JointIndices[1] = d[1];
                w.JointIndices[2] = d[2];
                w.JointIndices[3] = d[3];
                w.JointWeights[0] = d[4];
                w.JointWeights[1] = d[5];
                w.JointWeights[2] = d[6];
                w.JointWeights[3] = d[7];
            }
        } else {
            GLogger.Printf( "Unknown tag1\n" );
        }
    }

    if ( Weights.Size() > 0 && Vertices.Size() != Weights.Size() ) {
        GLogger.Printf( "Warning: num weights != num vertices\n" );
    }
}

void FMeshAsset::Write( FFileStream & f ) {
    f.Printf( "format %d %d\n", FMT_FILE_TYPE_MESH, FMT_VERSION_MESH );
    f.Printf( "textures %d\n", (int)Textures.size() );
    for ( FMaterialTexture & texture : Textures ) {
        f.Printf( "%s\n", texture.FileName.ToConstChar() );
    }
    f.Printf( "materials %d\n", Materials.Size() );
    for ( FMeshMaterial & material : Materials ) {
        f.Printf( "maps %d\n", material.NumTextures );
        for ( int i = 0 ; i < material.NumTextures ; i++ ) {
            f.Printf( "%d\n", material.Textures[i] );
        }
    }
    f.Printf( "subparts %d\n", (int)Subparts.size() );
    for ( FSubpart & subpart : Subparts ) {
        f.Printf( "\"%s\" %d %d %d %d %d %s %s\n", subpart.Name.ToConstChar(), subpart.BaseVertex, subpart.VertexCount, subpart.FirstIndex, subpart.IndexCount, subpart.Material, subpart.BoundingBox.Mins.ToString().ToConstChar(), subpart.BoundingBox.Maxs.ToString().ToConstChar() );
    }
    f.Printf( "verts %d\n", Vertices.Size() );
    for ( FMeshVertex & v : Vertices ) {
        f.Printf( "%s %s %s %f %s\n",
                  v.Position.ToString().ToConstChar(),
                  v.TexCoord.ToString().ToConstChar(),
                  v.Tangent.ToString().ToConstChar(),
                  v.Handedness,
                  v.Normal.ToString().ToConstChar() );
    }
    f.Printf( "indices %d\n", Indices.Size() );
    for ( unsigned int & i : Indices ) {
        f.Printf( "%d\n", i );
    }
    f.Printf( "weights %d\n", Weights.Size() );
    for ( FMeshVertexJoint & v : Weights ) {
        f.Printf( "%d %d %d %d %d %d %d %d\n",
                  v.JointIndices[0],v.JointIndices[1],v.JointIndices[2],v.JointIndices[3],
                v.JointWeights[0],v.JointWeights[1],v.JointWeights[2],v.JointWeights[3] );
    }
}

void FSkeletonAsset::Clear() {
    Joints.Clear();
    Animations.clear();
    BindposeBounds.Clear();
}

void FSkeletonAsset::Read( FFileStream & f ) {
    char buf[1024];
    char * s;
    int format, version;

    Clear();

    if ( !ReadFileFormat( f, &format, &version ) ) {
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
        if ( nullptr != ( s = ParseTag( buf, "joints " ) ) ) {
            int numJoints = 0;
            sscanf( s, "%d", &numJoints );
            Joints.ResizeInvalidate( numJoints );
            for ( int jointIndex = 0 ; jointIndex < numJoints ; jointIndex++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                char * name;
                s = ParseName( buf, &name );

                FJoint & joint = Joints[jointIndex];
                FString::CopySafe( joint.Name, sizeof( joint.Name ), name );

                joint.LocalTransform.SetIdentity();

                sscanf( s, "%d ( ( %f %f %f %f ) ( %f %f %f %f ) ( %f %f %f %f ) ) ( ( %f %f %f %f ) ( %f %f %f %f ) ( %f %f %f %f ) )", &joint.Parent,
                        &joint.OffsetMatrix[0][0].Value, &joint.OffsetMatrix[0][1].Value, &joint.OffsetMatrix[0][2].Value, &joint.OffsetMatrix[0][3].Value,
                        &joint.OffsetMatrix[1][0].Value, &joint.OffsetMatrix[1][1].Value, &joint.OffsetMatrix[1][2].Value, &joint.OffsetMatrix[1][3].Value,
                        &joint.OffsetMatrix[2][0].Value, &joint.OffsetMatrix[2][1].Value, &joint.OffsetMatrix[2][2].Value, &joint.OffsetMatrix[2][3].Value,
                        &joint.LocalTransform[0][0].Value, &joint.LocalTransform[0][1].Value, &joint.LocalTransform[0][2].Value, &joint.LocalTransform[0][3].Value,
                        &joint.LocalTransform[1][0].Value, &joint.LocalTransform[1][1].Value, &joint.LocalTransform[1][2].Value, &joint.LocalTransform[1][3].Value,
                        &joint.LocalTransform[2][0].Value, &joint.LocalTransform[2][1].Value, &joint.LocalTransform[2][2].Value, &joint.LocalTransform[2][3].Value );
            }
        } else if ( nullptr != ( s = ParseTag( buf, "bindpose_bounds " ) ) ) {

            sscanf( s, "( %f %f %f ) ( %f %f %f )",
                    &BindposeBounds.Mins.X.Value, &BindposeBounds.Mins.Y.Value, &BindposeBounds.Mins.Z.Value,
                    &BindposeBounds.Maxs.X.Value, &BindposeBounds.Maxs.Y.Value, &BindposeBounds.Maxs.Z.Value );

        } else if ( nullptr != ( s = ParseTag( buf, "animations " ) ) ) {
            int numAnimations = 0;
            sscanf( s, "%d", &numAnimations );
            Animations.resize( numAnimations );
            FSkeletalAnimationAsset * anim = NULL;
            for ( int animIndex = 0 ; ; ) {

                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                if ( nullptr != ( s = ParseTag( buf, "animation " ) ) ) {

                    if ( animIndex == numAnimations ) {
                        GLogger.Printf( "Unexpected tag\n" );
                        break;
                    }

                    anim = &Animations[animIndex++];

                    anim->Clear();

                    char * name;
                    s = ParseName( s, &name );

                    anim->Name = name;

                    sscanf( s, "%f %d\n", &anim->FrameDelta, &anim->FrameCount );

                    continue;
                }

                if ( !anim ) {
                    GLogger.Printf( "Unexpected tag\n" );
                    continue;
                }

                if ( nullptr != ( s = ParseTag( buf, "end_animation" ) ) ) {
                    if ( animIndex != numAnimations ) {
                        GLogger.Printf( "Unexpected tag\n" );
                    }
                    break;
                }

                if ( nullptr != ( s = ParseTag( buf, "anim_joints " ) ) ) {
                    int numJoints = 0;
                    sscanf( s, "%d", &numJoints );
                    anim->AnimatedJoints.ResizeInvalidate( numJoints );
                    anim->Transforms.ResizeInvalidate( numJoints * anim->FrameCount );
                    int transformOffset = 0;
                    for ( int jointIndex = 0 ; jointIndex < numJoints ; jointIndex++ ) {
                        FJointAnimation & janim = anim->AnimatedJoints[jointIndex];
                        int numFrames = 0;

                        if ( !f.Gets( buf, sizeof( buf ) ) ) {
                            GLogger.Printf( "Unexpected EOF\n" );
                            return;
                        }

                        sscanf( buf, "%d %d", &janim.JointIndex, &numFrames );

                        AN_Assert( numFrames == anim->FrameCount );

                        janim.TransformOffset = transformOffset;

                        for ( int j = 0 ; j < numFrames ; j++ ) {
                            FJointTransform & transform = anim->Transforms[ transformOffset + j ];

                            if ( !f.Gets( buf, sizeof( buf ) ) ) {
                                GLogger.Printf( "Unexpected EOF\n" );
                                return;
                            }

                            sscanf( buf, "( %f %f %f %f ) ( %f %f %f )  ( %f %f %f )",
                                    &transform.Rotation.X.Value, &transform.Rotation.Y.Value, &transform.Rotation.Z.Value, &transform.Rotation.W.Value,
                                    &transform.Position.X.Value, &transform.Position.Y.Value, &transform.Position.Z.Value,
                                    &transform.Scale.X.Value, &transform.Scale.Y.Value, &transform.Scale.Z.Value );
                        }

                        transformOffset += numFrames;
                    }
                } else if ( nullptr != ( s = ParseTag( buf, "bounds" ) ) ) {
                    anim->Bounds.ResizeInvalidate( anim->FrameCount );
                    for ( int frameIndex = 0 ; frameIndex < anim->FrameCount ; frameIndex++ ) {
                        BvAxisAlignedBox & bv = anim->Bounds[frameIndex];

                        if ( !f.Gets( buf, sizeof( buf ) ) ) {
                            GLogger.Printf( "Unexpected EOF\n" );
                            return;
                        }

                        sscanf( buf, "( %f %f %f ) ( %f %f %f )",
                                &bv.Mins.X.Value, &bv.Mins.Y.Value, &bv.Mins.Z.Value,
                                &bv.Maxs.X.Value, &bv.Maxs.Y.Value, &bv.Maxs.Z.Value );
                    }
                } else {
                    GLogger.Printf( "Unknown tag2\n" );
                }
            }
        } else {
            GLogger.Printf( "Unknown tag '%s'`\n", buf );
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

    f.Printf( "animations %d\n", (int)Animations.size() );
    for ( FSkeletalAnimationAsset & anim : Animations ) {
        f.Printf( "animation \"%s\" %f %d\n", anim.Name.ToConstChar(), anim.FrameDelta, anim.FrameCount );
        f.Printf( "anim_joints %d\n", anim.AnimatedJoints.Size() );
        for ( FJointAnimation & janim : anim.AnimatedJoints ) {
            f.Printf( "%d %d\n", janim.JointIndex, anim.FrameCount );

            for ( int frame = 0 ; frame < anim.FrameCount ; frame++ ) {
                FJointTransform & transform = anim.Transforms[ janim.TransformOffset + frame ];
                f.Printf( "%s %s %s\n",
                          transform.Rotation.ToString().ToConstChar(),
                          transform.Position.ToString().ToConstChar(),
                          transform.Scale.ToString().ToConstChar()
                          );
            }
        }
        f.Printf( "bounds\n" );
        for ( BvAxisAlignedBox & bounds : anim.Bounds ) {
            f.Printf( "%s %s\n", bounds.Mins.ToString().ToConstChar(), bounds.Maxs.ToString().ToConstChar() );
        }
    }
    f.Printf( "end_animation\n" );
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

void FSkeletalAnimationAsset::Clear() {
    FrameDelta = 0;
    FrameCount = 0;
    AnimatedJoints.Clear();
    Transforms.Clear();
    Name.Clear();
    Bounds.Clear();
}

void FSkeletalAnimationAsset::CalcBoundingBoxes( FMeshAsset const * InMeshData, FJoint const *  InJoints, int InNumJoints ) {
    Float3x4 absoluteTransforms[FSkeleton::MAX_JOINTS+1];
    TPodArray< Float3x4 > relativeTransforms[FSkeleton::MAX_JOINTS];
    Float3x4 vertexTransforms[FSkeleton::MAX_JOINTS];

    Bounds.ResizeInvalidate( FrameCount );

    for ( FJointAnimation & anim : AnimatedJoints ) {

        relativeTransforms[anim.JointIndex].ResizeInvalidate( FrameCount );

        for ( int frameNum = 0; frameNum < FrameCount ; frameNum++ ) {

            FJointTransform const & transform = Transforms[ anim.TransformOffset + frameNum ];

            transform.ToMatrix( relativeTransforms[anim.JointIndex][frameNum] );
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
