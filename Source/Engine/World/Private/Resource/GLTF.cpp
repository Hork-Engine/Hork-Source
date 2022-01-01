#if 0
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

#include <World/Public/Resource/GLTF.h>

#include <Core/Public/Logger.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

struct SContextGLTF {
    cgltf_data * Data;
    bool bSkeletal;
    TPodVector< cgltf_node * > NodeToJoint;
};

#define MAX_MEMORY_GLTF     (16<<20) // enough?

static size_t totalAllocatedGLTF = 0;

static void * cgltf_alloc( void * user, cgltf_size size ) {
    if ( totalAllocatedGLTF + size >= MAX_MEMORY_GLTF ) {
        GLogger.Printf( "Error: glTF takes too much memory (total usage %.1f MB)\n", totalAllocatedGLTF / ( 1024.0f*1024.0f ) );
        return nullptr;
    }

    void * ptr = ( byte * )user + totalAllocatedGLTF;
    totalAllocatedGLTF += size;

    return ptr;
}

static void cgltf_free( void * user, void * ptr ) {
}

static void unpack_vec2_or_vec3( cgltf_accessor * acc, Float3 * output, size_t stride ) {
    int num_elements;
    float position[ 3 ];

    if ( !acc ) {
        return;
    }

    if ( acc->type == cgltf_type_vec2 ) {
        num_elements = 2;
    } else if ( acc->type == cgltf_type_vec3 ) {
        num_elements = 3;
    } else {
        return;
    }

    position[ 2 ] = 0;

    byte * ptr = ( byte * )output;

    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, position, num_elements );

        Core::Memcpy( ptr, position, sizeof( float ) * 3 );

        ptr += stride;
    }
}

static void unpack_vec2( cgltf_accessor * acc, Float2 * output, size_t stride ) {
    if ( !acc || acc->type != cgltf_type_vec2 ) {
        return;
    }

    byte * ptr = ( byte * )output;

    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, ( float * )ptr, 2 );

        ptr += stride;
    }
}

static void unpack_vec3( cgltf_accessor * acc, Float3 * output, size_t stride ) {
    if ( !acc || acc->type != cgltf_type_vec3 ) {
        return;
    }

    byte * ptr = ( byte * )output;

    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, ( float * )ptr, 3 );

        ptr += stride;
    }
}

static void unpack_vec4( cgltf_accessor * acc, Float4 * output, size_t stride ) {
    if ( !acc || acc->type != cgltf_type_vec4 ) {
        return;
    }

    byte * ptr = ( byte * )output;

    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, ( float * )ptr, 4 );

        ptr += stride;
    }
}

static void unpack_quat( cgltf_accessor * acc, Quat * output, size_t stride ) {
    if ( !acc || acc->type != cgltf_type_vec4 ) {
        return;
    }

    byte * ptr = ( byte * )output;

    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, ( float * )ptr, 4 );

        ptr += stride;
    }
}

static void unpack_mat4( cgltf_accessor * acc, Float4x4 * output, size_t stride ) {
    if ( !acc || acc->type != cgltf_type_mat4 ) {
        return;
    }

    byte * ptr = ( byte * )output;

    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, ( float * )ptr, 16 );

        ptr += stride;
    }
}

static void unpack_mat4_to_mat3x4( cgltf_accessor * acc, Float3x4 * output, size_t stride ) {
    if ( !acc || acc->type != cgltf_type_mat4 ) {
        return;
    }

    byte * ptr = ( byte * )output;

    Float4x4 temp;
    for ( int i = 0; i < acc->count; i++ ) {
        cgltf_accessor_read_float( acc, i, ( float * )temp.ToPtr(), 16 );

        Core::Memcpy( ptr, temp.Transposed().ToPtr(), sizeof( Float3x4 ) );

        ptr += stride;
    }
}

static void unpack_weights( cgltf_accessor * acc, SMeshVertexSkin * weights ) {
    float weight[ 4 ];

    if ( !acc || acc->type != cgltf_type_vec4 ) {
        return;
    }

    for ( int i = 0; i < acc->count; i++, weights++ ) {
        cgltf_accessor_read_float( acc, i, weight, 4 );

        const float invSum = 255.0f / ( weight[0] + weight[1] + weight[2] + weight[3] );

        weights->JointWeights[ 0 ] = Math::Clamp< int >( weight[ 0 ] * invSum, 0, 255 );
        weights->JointWeights[ 1 ] = Math::Clamp< int >( weight[ 1 ] * invSum, 0, 255 );
        weights->JointWeights[ 2 ] = Math::Clamp< int >( weight[ 2 ] * invSum, 0, 255 );
        weights->JointWeights[ 3 ] = Math::Clamp< int >( weight[ 3 ] * invSum, 0, 255 );
    }
}

static void unpack_joints( cgltf_accessor * acc, SMeshVertexSkin * weights ) {
    float indices[ 4 ];

    if ( !acc || acc->type != cgltf_type_vec4 ) {
        return;
    }

    for ( int i = 0; i < acc->count; i++, weights++ ) {
        cgltf_accessor_read_float( acc, i, indices, 4 );

        weights->JointIndices[ 0 ] = Math::Clamp< int >( indices[0], 0, ASkeleton::MAX_JOINTS );
        weights->JointIndices[ 1 ] = Math::Clamp< int >( indices[1], 0, ASkeleton::MAX_JOINTS );
        weights->JointIndices[ 2 ] = Math::Clamp< int >( indices[2], 0, ASkeleton::MAX_JOINTS );
        weights->JointIndices[ 3 ] = Math::Clamp< int >( indices[3], 0, ASkeleton::MAX_JOINTS );
    }
}

static void sample_vec3( cgltf_animation_sampler * sampler, float frameTime, Float3 & vec ) {
    cgltf_accessor * animtimes = sampler->input;
    cgltf_accessor * animdata = sampler->output;

    float ft0, ftN, ct, nt;

    AN_ASSERT( animtimes->count > 0 );

    cgltf_accessor_read_float( animtimes, 0, &ft0, 1 );

    if ( animtimes->count == 1 || frameTime <= ft0 ) {

        if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
            cgltf_accessor_read_float( animdata, 0 * 3 + 1, ( float * )vec.ToPtr(), 3 );
        } else {
            cgltf_accessor_read_float( animdata, 0, ( float * )vec.ToPtr(), 3 );
        }

        return;
    }

    cgltf_accessor_read_float( animtimes, animtimes->count - 1, &ftN, 1 );

    if ( frameTime >= ftN ) {

        if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
            cgltf_accessor_read_float( animdata, ( animtimes->count - 1 ) * 3 + 1, ( float * )vec.ToPtr(), 3 );
        } else {
            cgltf_accessor_read_float( animdata, animtimes->count - 1, ( float * )vec.ToPtr(), 3 );
        }

        return;
    }

    ct = ft0;

    for ( int t = 0; t < ( int )animtimes->count - 1; t++, ct = nt ) {

        cgltf_accessor_read_float( animtimes, t + 1, &nt, 1 );

        if ( ct <= frameTime && nt > frameTime ) {
            if ( sampler->interpolation == cgltf_interpolation_type_linear ) {
                if ( frameTime == ct ) {
                    cgltf_accessor_read_float( animdata, t, ( float * )vec.ToPtr(), 3 );
                } else {
                    Float3 p0, p1;
                    cgltf_accessor_read_float( animdata, t, ( float * )p0.ToPtr(), 3 );
                    cgltf_accessor_read_float( animdata, t + 1, ( float * )p1.ToPtr(), 3 );
                    float dur = nt - ct;
                    float fract = ( frameTime - ct ) / dur;
                    AN_ASSERT( fract >= 0.0f && fract < 1.0f );
                    vec = p0.Lerp( p1, fract );
                }
            } else if ( sampler->interpolation == cgltf_interpolation_type_step ) {
                cgltf_accessor_read_float( animdata, t, ( float * )vec.ToPtr(), 3 );
            } else if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
                float dur = nt - ct;
                float fract = ( dur == 0.0f ) ? 0.0f : ( frameTime - ct ) / dur;
                AN_ASSERT( fract >= 0.0f && fract < 1.0f );

                Float3 p0,m0,m1,p1;

                cgltf_accessor_read_float( animdata, t * 3 + 1, ( float * )p0.ToPtr(), 3 );
                cgltf_accessor_read_float( animdata, t * 3 + 2, ( float * )m0.ToPtr(), 3 );
                cgltf_accessor_read_float( animdata, ( t + 1 ) * 3, ( float * )m1.ToPtr(), 3 );
                cgltf_accessor_read_float( animdata, ( t + 1 ) * 3 + 1, ( float * )p1.ToPtr(), 3 );

                m0 *= dur;
                m1 *= dur;

                vec = Math::HermiteCubicSpline( p0, m0, p1, m1, fract );
            }
            break;
        }
    }
}

static void sample_quat( cgltf_animation_sampler * sampler, float frameTime, Quat & q ) {
    cgltf_accessor * animtimes = sampler->input;
    cgltf_accessor * animdata = sampler->output;

    float ft0, ftN, ct, nt;

    AN_ASSERT( animtimes->count > 0 );

    cgltf_accessor_read_float( animtimes, 0, &ft0, 1 );

    if ( animtimes->count == 1 || frameTime <= ft0 ) {

        if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
            cgltf_accessor_read_float( animdata, 0 * 3 + 1, ( float * )q.ToPtr(), 4 );
        } else {
            cgltf_accessor_read_float( animdata, 0, ( float * )q.ToPtr(), 4 );
        }

        return;
    }

    cgltf_accessor_read_float( animtimes, animtimes->count - 1, &ftN, 1 );

    if ( frameTime >= ftN ) {

        if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
            cgltf_accessor_read_float( animdata, ( animtimes->count - 1 ) * 3 + 1, ( float * )q.ToPtr(), 4 );
        } else {
            cgltf_accessor_read_float( animdata, animtimes->count - 1, ( float * )q.ToPtr(), 4 );
        }

        return;
    }

    ct = ft0;

    for ( int t = 0; t < ( int )animtimes->count - 1; t++, ct = nt ) {

        cgltf_accessor_read_float( animtimes, t + 1, &nt, 1 );

        if ( ct <= frameTime && nt > frameTime ) {
            if ( sampler->interpolation == cgltf_interpolation_type_linear ) {
                if ( frameTime == ct ) {
                    cgltf_accessor_read_float( animdata, t, ( float * )q.ToPtr(), 4 );
                } else {
                    Quat p0, p1;
                    cgltf_accessor_read_float( animdata, t, ( float * )p0.ToPtr(), 4 );
                    cgltf_accessor_read_float( animdata, t + 1, ( float * )p1.ToPtr(), 4 );
                    float dur = nt - ct;
                    float fract = ( frameTime - ct ) / dur;
                    AN_ASSERT( fract >= 0.0f && fract < 1.0f );
                    q = p0.Slerp( p1, fract ).Normalized();
                }
            } else if ( sampler->interpolation == cgltf_interpolation_type_step ) {
                cgltf_accessor_read_float( animdata, t, ( float * )q.ToPtr(), 4 );
            } else if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
                float dur = nt - ct;
                float fract = ( dur == 0.0f ) ? 0.0f : ( frameTime - ct ) / dur;
                AN_ASSERT( fract >= 0.0f && fract < 1.0f );

                Quat p0,m0,m1,p1;

                cgltf_accessor_read_float( animdata, t * 3 + 1, ( float * )p0.ToPtr(), 4 );
                cgltf_accessor_read_float( animdata, t * 3 + 2, ( float * )m0.ToPtr(), 4 );
                cgltf_accessor_read_float( animdata, ( t + 1 ) * 3, ( float * )m1.ToPtr(), 4 );
                cgltf_accessor_read_float( animdata, ( t + 1 ) * 3 + 1, ( float * )p1.ToPtr(), 4 );

                m0 *= dur;
                m1 *= dur;

                p0.NormalizeSelf();
                m0.NormalizeSelf();
                m1.NormalizeSelf();
                p1.NormalizeSelf();

                q = Math::HermiteCubicSpline( p0, m0, p1, m1, fract );
                q.NormalizeSelf();
            }
            break;
        }
    }
}

static void ReadMesh( SContextGLTF & Ctx, cgltf_mesh * Mesh, Float3x4 const & GlobalTransform, Float3x3 const & NormalMatrix, SMeshAsset & MeshAsset ) {
    struct ASortFunction {
        bool operator() ( cgltf_primitive const & _A, cgltf_primitive const & _B ) {
            return ( _A.material < _B.material );
        }
    } SortFunction;

    std::sort( &Mesh->primitives[ 0 ], &Mesh->primitives[ Mesh->primitives_count ], SortFunction );

    cgltf_material * material = nullptr;
    SSubpart * subpart = nullptr;

    for ( int i = 0; i < Mesh->primitives_count; i++ ) {
        cgltf_primitive * prim = &Mesh->primitives[ i ];

        if ( prim->type != cgltf_primitive_type_triangles ) {
            GLogger.Printf( "Only triangle primitives supported\n" );
            continue;
        }

        cgltf_accessor * position = nullptr;
        cgltf_accessor * normal = nullptr;
        cgltf_accessor * tangent = nullptr;
        cgltf_accessor * texcoord = nullptr;
        cgltf_accessor * color = nullptr;
        cgltf_accessor * joints = nullptr;
        cgltf_accessor * weights = nullptr;

        for ( int a = 0; a < prim->attributes_count; a++ ) {
            cgltf_attribute * attrib = &prim->attributes[ a ];

            if ( attrib->data->is_sparse ) {
                GLogger.Printf( "Warning: sparsed accessors are not supported\n" );
                continue;
            }

            switch ( attrib->type ) {
            case cgltf_attribute_type_invalid:
                GLogger.Printf( "Warning: invalid attribute type\n" );
                continue;

            case cgltf_attribute_type_position:
                position = attrib->data;
                break;

            case cgltf_attribute_type_normal:
                normal = attrib->data;
                break;

            case cgltf_attribute_type_tangent:
                tangent = attrib->data;
                break;

            case cgltf_attribute_type_texcoord:
                // get first texcoord channel
                if ( !texcoord ) {
                    texcoord = attrib->data;
                }
                break;

            case cgltf_attribute_type_color:
                color = attrib->data;
                break;

            case cgltf_attribute_type_joints:
                joints = attrib->data;
                break;

            case cgltf_attribute_type_weights:
                weights = attrib->data;
                break;
            }
        }

        if ( !position ) {
            GLogger.Printf( "Warning: no positions\n" );
            continue;
        }

        if ( position->type != cgltf_type_vec2 && position->type != cgltf_type_vec3 ) {
            GLogger.Printf( "Warning: invalid vertex positions\n" );
            continue;
        }

        if ( !texcoord ) {
            GLogger.Printf( "Warning: no texcoords\n" );
        }

        if ( texcoord && texcoord->type != cgltf_type_vec2 ) {
            GLogger.Printf( "Warning: invalid texcoords\n" );
            texcoord = nullptr;
        }

        int vertexCount = position->count;
        if ( texcoord && texcoord->count != vertexCount ) {
            GLogger.Printf( "Warning: texcoord count != position count\n" );
            texcoord = nullptr;
        }

        if ( !material || material != prim->material ) {
            MeshAsset.Subparts.Append( SSubpart() );
            subpart = &MeshAsset.Subparts.back();
            subpart->BoundingBox.Clear();
            subpart->BaseVertex = MeshAsset.Vertices.Size();
            subpart->FirstIndex = MeshAsset.Indices.Size();
            subpart->VertexCount = 0;
            subpart->IndexCount = 0;
            subpart->Name = Mesh->name;
            subpart->Material = 0;
            material = prim->material;
        }

        int firstVert = MeshAsset.Vertices.Size();
        MeshAsset.Vertices.Resize( firstVert + vertexCount );

        int vertexOffset = firstVert - subpart->BaseVertex;

        int firstIndex = MeshAsset.Indices.Size();
        int indexCount;
        if ( prim->indices ) {
            indexCount = prim->indices->count;
            MeshAsset.Indices.Resize( firstIndex + indexCount );
            unsigned int * pInd = MeshAsset.Indices.ToPtr() + firstIndex;
            for ( int index = 0; index < indexCount; index++, pInd++ ) {
                *pInd = vertexOffset + cgltf_accessor_read_index( prim->indices, index );
            }
        } else {
            indexCount = vertexCount;
            MeshAsset.Indices.Resize( firstIndex + indexCount );
            unsigned int * pInd = MeshAsset.Indices.ToPtr() + firstIndex;
            for ( int index = 0; index < indexCount; index++, pInd++ ) {
                *pInd = vertexOffset + index;
            }
        }

        unpack_vec2_or_vec3( position, &MeshAsset.Vertices[ firstVert ].Position, sizeof( SMeshVertex ) );

        if ( texcoord ) {
            unpack_vec2( texcoord, &MeshAsset.Vertices[ firstVert ].TexCoord, sizeof( SMeshVertex ) );
        } else {
            for ( int v = 0; v < vertexCount; v++ ) {
                MeshAsset.Vertices[ firstVert + v ].TexCoord.Clear();
            }
        }

        if ( normal && ( normal->type == cgltf_type_vec2 || normal->type == cgltf_type_vec3 ) && normal->count == vertexCount ) {
            unpack_vec2_or_vec3( normal, &MeshAsset.Vertices[ firstVert ].Normal, sizeof( SMeshVertex ) );

            SMeshVertex * pVert = MeshAsset.Vertices.ToPtr() + firstVert;
            for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                pVert->Normal.NormalizeSelf();
            }

        } else {
            // TODO: compute normals

            GLogger.Printf( "Warning: no normals\n" );

            for ( int v = 0; v < vertexCount; v++ ) {
                MeshAsset.Vertices[ firstVert + v ].Normal = Float3( 0, 1, 0 );
            }
        }

        if ( tangent && ( tangent->type == cgltf_type_vec4 ) && tangent->count == vertexCount ) {
            unpack_vec4( tangent, (Float4 *)&MeshAsset.Vertices[ firstVert ].Tangent, sizeof( SMeshVertex ) );
        } else {
            CalcTangentSpace( MeshAsset.Vertices.ToPtr(), MeshAsset.Vertices.Size(), MeshAsset.Indices.ToPtr() + firstIndex, indexCount );
        }

        if ( weights && ( weights->type == cgltf_type_vec4 ) && weights->count == vertexCount
            && joints && ( joints->type == cgltf_type_vec4 ) && joints->count == vertexCount ) {
            MeshAsset.Weights.Resize( MeshAsset.Vertices.Size() );
            unpack_weights( weights, &MeshAsset.Weights[ firstVert ] );
            unpack_joints( joints, &MeshAsset.Weights[ firstVert ] );
        }

        AN_UNUSED( color );

        SMeshVertex * pVert = MeshAsset.Vertices.ToPtr() + firstVert;

        if ( Ctx.bSkeletal ) {
            for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                subpart->BoundingBox.AddPoint( pVert->Position );
            }
        } else {
            for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                pVert->Position = Float3( GlobalTransform * pVert->Position );                
                pVert->Normal = NormalMatrix * pVert->Normal;
                pVert->Tangent = NormalMatrix * pVert->Tangent;
                subpart->BoundingBox.AddPoint( pVert->Position );
            }
        }

        subpart->VertexCount += vertexCount;
        subpart->IndexCount += indexCount;

        //cgltf_morph_target* targets;
        //cgltf_size targets_count;
    }

    //for ( int i = 0; i < mesh->weights_count; i++ ) {
    //    cgltf_float * w = &mesh->weights[ i ];
    //}

    GLogger.Printf( "Subparts %d, Primitives %d\n", MeshAsset.Subparts.size(), Mesh->primitives_count );
}

static void ReadMesh( SContextGLTF & Ctx, cgltf_node * Node, SMeshAsset & MeshAsset ) {
    cgltf_mesh * mesh = Node->mesh;

    if ( !mesh ) {
        return;
    }

    Float3x4 globalTransform;
    Float3x3 normalMatrix;
    Float4x4 temp;
    cgltf_node_transform_world( Node, ( float * )temp.ToPtr() );
    globalTransform = Float3x4( temp.Transposed() );
    globalTransform.DecomposeNormalMatrix( normalMatrix );

    ReadMesh( Ctx, mesh, globalTransform, normalMatrix, MeshAsset );
}

static int FindJoint( cgltf_skin * Skin, cgltf_node * Node ) {
    for ( int i = 0; i < Skin->joints_count; i++ ) {
        if ( Skin->joints[ i ] == Node ) {
            return i;
        }
    }
    return -1;
}

static void ReadSkeleton_r( SContextGLTF & Ctx, cgltf_skin * Skin, cgltf_node * Node, int ParentNum, SSkeletonAsset & Asset ) {
    int jointIndex = FindJoint( Skin, Node );
    if ( jointIndex == -1 ) {
        for ( int i = 0; i < Node->children_count; i++ ) {
            ReadSkeleton_r( Ctx, Skin, Node->children[ i ], ParentNum, Asset );
        }
        return;
    }

    SJoint * joint = &Asset.Joints[ jointIndex ];

    if ( Node->name ) {
        Core::CopySafe( joint->Name, sizeof( joint->Name ), Node->name );
    } else {
        Core::CopySafe( joint->Name, sizeof( joint->Name ), "unnamed" );
    }

    joint->Parent = ParentNum;

    Ctx.NodeToJoint[ jointIndex ] = Node;

    Float4x4 localTransform;
    cgltf_node_transform_local( Node, ( float * )localTransform.ToPtr() );

    joint->LocalTransform = Float3x4( localTransform.Transposed() );

    for ( int i = 0; i < Node->children_count; i++ ) {
        ReadSkeleton_r( Ctx, Skin, Node->children[ i ], jointIndex, Asset );
    }
}

static void ReadSkin( SContextGLTF & Ctx, cgltf_skin * Skin, SMeshAsset const & MeshAsset, SSkeletonAsset & SkeletonAsset ) {
    SkeletonAsset.Joints.ResizeInvalidate( Skin->joints_count );
    Ctx.NodeToJoint.ResizeInvalidate( Skin->joints_count );

    if ( Skin->inverse_bind_matrices->count != Skin->joints_count ) {
        GLogger.Printf( "inverse_bind_matrices->count != joints_count\n" );
        return;
    }

    unpack_mat4_to_mat3x4( Skin->inverse_bind_matrices, SkeletonAsset.Skin.OffsetMatrices.ToPtr(), sizeof( Float3x4 ) );

    if ( Skin->skeleton ) {
        ReadSkeleton_r( Ctx, Skin, Skin->skeleton, -1, SkeletonAsset );
    }
    SkeletonAsset.CalcBindposeBounds( MeshAsset.Vertices.ToPtr(), MeshAsset.Weights.ToPtr(), MeshAsset.Vertices.Size(), &SkeletonAsset.Skin );
}

static void ReadSkin( SContextGLTF & Ctx, cgltf_node * Node, SMeshAsset const & MeshAsset, SSkeletonAsset & SkeletonAsset ) {
    cgltf_skin * skin = Node->skin;

    if ( !skin ) {
        return;
    }

    ReadSkin( Ctx, skin, MeshAsset, SkeletonAsset );
}

static void ReadNode_r( SContextGLTF & Ctx, cgltf_node * Node, SMeshAsset & MeshAsset ) {

    //GLogger.Printf( "node \"%s\"\n", Node->name );

    //if ( Node->camera ) {
    //    GLogger.Printf( "has camera\n" );
    //}

    //if ( Node->light ) {
    //    GLogger.Printf( "has light\n" );
    //}

    //if ( Node->weights ) {
    //    GLogger.Printf( "has weights %d\n", Node->weights_count );
    //}

    ReadMesh( Ctx, Node, MeshAsset );

    for ( int n = 0; n < Node->children_count; n++ ) {
        cgltf_node * child = Node->children[ n ];

        ReadNode_r( Ctx, child, MeshAsset );
    }
}


static const char * GetErrorString( cgltf_result code ) {
    switch ( code ) {
    case cgltf_result_success:
        return "No error";
    case cgltf_result_data_too_short:
        return "Data too short";
    case cgltf_result_unknown_format:
        return "Unknown format";
    case cgltf_result_invalid_json:
        return "Invalid json";
    case cgltf_result_invalid_gltf:
        return "Invalid gltf";
    case cgltf_result_invalid_options:
        return "Invalid options";
    case cgltf_result_file_not_found:
        return "File not found";
    case cgltf_result_io_error:
        return "IO error";
    case cgltf_result_out_of_memory:
        return "Out of memory";
    default:
        ;
    }

    return "Unknown error";
}

static bool IsChannelValid( cgltf_animation_channel * channel ) {
    cgltf_animation_sampler * sampler = channel->sampler;

    switch ( channel->target_path ) {
    case cgltf_animation_path_type_translation:
    case cgltf_animation_path_type_rotation:
    case cgltf_animation_path_type_scale:
        break;
    case cgltf_animation_path_type_weights:
        GLogger.Printf( "Warning: animation path weights is not supported yet\n" );
        return false;
    default:
        GLogger.Printf( "Warning: unknown animation target path\n" );
        return false;
    }

    switch ( sampler->interpolation ) {
    case cgltf_interpolation_type_linear:
    case cgltf_interpolation_type_step:
    case cgltf_interpolation_type_cubic_spline:
        break;
    default:
        GLogger.Printf( "Warning: unknown interpolation type\n" );
        return false;
    }

    cgltf_accessor * animtimes = sampler->input;
    cgltf_accessor * animdata = sampler->output;

    if ( animtimes->count == 0 ) {
        GLogger.Printf( "Warning: empty channel data\n" );
        return false;
    }

    if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline && animtimes->count != animdata->count * 3 ) {
        GLogger.Printf( "Warning: invalid channel data\n" );
        return false;
    } else if ( animtimes->count != animdata->count ) {
        GLogger.Printf( "Warning: invalid channel data\n" );
        return false;
    }

    return true;
}

static void ReadAnimation( SContextGLTF & Ctx, cgltf_animation * Anim, SSkeletonAsset * SkeletonAsset, SAnimationAsset * AnimAsset ) {
    const int framesPerSecond = 30;
//    float gcd = 0;
    float maxDuration = 0;

    for ( int ch = 0; ch < Anim->channels_count; ch++ ) {
        cgltf_animation_channel * channel = &Anim->channels[ ch ];
        cgltf_animation_sampler * sampler = channel->sampler;
        cgltf_accessor * animtimes = sampler->input;

        if ( animtimes->count == 0 ) {
            continue;
        }

#if 0
        float time = 0;
        for ( int t = 0; t < animtimes->count; t++ ) {
            cgltf_accessor_read_float( animtimes, t, &time, 1 );
            gcd = Math::GreaterCommonDivisor( gcd, time );
        }
#endif
        float time = 0;
        cgltf_accessor_read_float( animtimes, animtimes->count - 1, &time, 1 );
        maxDuration = Math::Max( maxDuration, time );
    }

    int numFrames = maxDuration * framesPerSecond;
    //numFrames--;

    float frameDelta = maxDuration / numFrames;

    AnimAsset->Name = Anim->name ? Anim->name : "unnamed";
    AnimAsset->FrameDelta = frameDelta;
    AnimAsset->FrameCount = numFrames;         // frames count, animation duration is FrameDelta * ( FrameCount - 1 )

    for ( int ch = 0; ch < Anim->channels_count; ch++ ) {

        cgltf_animation_channel * channel = &Anim->channels[ ch ];
        cgltf_animation_sampler * sampler = channel->sampler;

        if ( !IsChannelValid( channel ) ) {
            continue;
        }

        SJoint * joint = nullptr;
        int jointIndex;
        for ( jointIndex = 0; jointIndex < Ctx.NodeToJoint.Size(); jointIndex++ ) {
            if ( Ctx.NodeToJoint[ jointIndex ] == channel->target_node ) {
                joint = &SkeletonAsset->Joints[ jointIndex ];
                break;
            }
        }
        if ( !joint ) {
            GLogger.Printf( "Warning: joint is not found (channel %d)\n", ch );
            continue;
        }

        int mergedChannel;
        for ( mergedChannel = 0; mergedChannel < AnimAsset->Channels.Size(); mergedChannel++ ) {
            if ( jointIndex == AnimAsset->Channels[ mergedChannel ].NodeIndex ) {
                break;
            }
        }

        SAnimationChannel * jointAnim;

        if ( mergedChannel < AnimAsset->Channels.Size() ) {
            jointAnim = &AnimAsset->Channels[ mergedChannel ];
        } else {
            jointAnim = &AnimAsset->Channels.Append();
            jointAnim->NodeIndex = jointIndex;
            jointAnim->TransformOffset = AnimAsset->Transforms.Size();
            AnimAsset->Transforms.Resize( AnimAsset->Transforms.Size() + numFrames );
            for ( int f = 0; f < numFrames; f++ ) {
                ATransform & transform = AnimAsset->Transforms[ jointAnim->TransformOffset + f ];
                transform.Position = Float3( 0 );
                transform.Scale = Float3( 1 );
                transform.Rotation.SetIdentity();
            }
        }

        for ( int f = 0; f < numFrames; f++ ) {
            ATransform & transform = AnimAsset->Transforms[ jointAnim->TransformOffset + f ];

            float frameTime = f * frameDelta;

            switch ( channel->target_path ) {
            case cgltf_animation_path_type_translation:
                sample_vec3( sampler, frameTime, transform.Position );
                break;
            case cgltf_animation_path_type_rotation:
                sample_quat( sampler, frameTime, transform.Rotation );
                break;
            case cgltf_animation_path_type_scale:
                sample_vec3( sampler, frameTime, transform.Scale );
                break;
            default:
                break;
            }
        }
    }
}

static void ReadAnimations( SContextGLTF & Ctx, cgltf_data * Data, SMeshAsset const & MeshAsset, SSkeletonAsset & SkeletonAsset, TStdVector< SAnimationAsset > & Animations ) {
    Animations.resize( Data->animations_count );
    for ( int animIndex = 0; animIndex < Data->animations_count; animIndex++ ) {
        ReadAnimation( Ctx, &Data->animations[animIndex], &SkeletonAsset, &Animations[animIndex] );

        Animations[animIndex].CalcBoundingBoxes( MeshAsset.Vertices.ToPtr(), MeshAsset.Weights.ToPtr(), MeshAsset.Vertices.Size(), SkeletonAsset.Joints.ToPtr(), SkeletonAsset.Joints.Size(), &SkeletonAsset.Skin );
    }
}

static bool ReadGLTF( cgltf_data * Data, SMeshAsset & MeshAsset, SSkeletonAsset & SkeletonAsset, TStdVector< SAnimationAsset > & Animations ) {
    SContextGLTF ctx;

    ctx.Data = Data;
    ctx.bSkeletal = Data->skins_count > 0;

    GLogger.Printf( "%d scenes\n", Data->scenes_count );
    GLogger.Printf( "%d skins\n", Data->skins_count );
    GLogger.Printf( "%d meshes\n", Data->meshes_count );
    GLogger.Printf( "%d nodes\n", Data->nodes_count );
    GLogger.Printf( "%d cameras\n", Data->cameras_count );
    GLogger.Printf( "%d lights\n", Data->lights_count );

    if ( Data->extensions_used_count > 0 ) {
        GLogger.Printf( "Used extensions:\n" );
        for ( int i = 0; i < Data->extensions_used_count; i++ ) {
            GLogger.Printf( "    %s\n", Data->extensions_used[ i ] );
        }
    }

    if ( Data->extensions_required_count > 0 ) {
        GLogger.Printf( "Required extensions:\n" );
        for ( int i = 0; i < Data->extensions_required_count; i++ ) {
            GLogger.Printf( "    %s\n", Data->extensions_required[ i ] );
        }
    }

    if ( ctx.bSkeletal ) {
        // read meshes
        //for ( int i = 0; i < Data->meshes_count; i++ ) {
        //    ReadMesh( ctx, &Data->meshes[i], Float3x4::Identity(), Float3x3::Identity(), MeshAsset );
        //}

        // read meshes hierarchy
        for ( int i = 0; i < Data->scenes_count; i++ ) {
            cgltf_scene * scene = &Data->scene[ i ];

            GLogger.Printf( "Scene \"%s\" nodes %d\n", scene->name, scene->nodes_count );

            for ( int n = 0; n < scene->nodes_count; n++ ) {
                cgltf_node * node = scene->nodes[ n ];

                ReadNode_r( ctx, node, MeshAsset );
            }
        }

        // read first skeleton
        ReadSkin( ctx, &Data->skins[ 0 ], MeshAsset, SkeletonAsset );

        ReadAnimations( ctx, Data, MeshAsset, SkeletonAsset, Animations );

    } else {
        // read meshes hierarchy
        for ( int i = 0; i < Data->scenes_count; i++ ) {
            cgltf_scene * scene = &Data->scene[ i ];

            GLogger.Printf( "Scene \"%s\" nodes %d\n", scene->name, scene->nodes_count );

            for ( int n = 0; n < scene->nodes_count; n++ ) {
                cgltf_node * node = scene->nodes[ n ];

                ReadNode_r( ctx, node, MeshAsset );
            }
        }
    }

    return true;
}

static bool ReadGeometryGLTF( cgltf_data * Data, SMeshAsset & MeshAsset ) {
    SContextGLTF ctx;

    ctx.Data = Data;
    ctx.bSkeletal = Data->skins_count > 0;

    GLogger.Printf( "%d scenes\n", Data->scenes_count );
    GLogger.Printf( "%d skins\n", Data->skins_count );
    GLogger.Printf( "%d meshes\n", Data->meshes_count );
    GLogger.Printf( "%d nodes\n", Data->nodes_count );
    GLogger.Printf( "%d cameras\n", Data->cameras_count );
    GLogger.Printf( "%d lights\n", Data->lights_count );

    if ( Data->extensions_used_count > 0 ) {
        GLogger.Printf( "Used extensions:\n" );
        for ( int i = 0; i < Data->extensions_used_count; i++ ) {
            GLogger.Printf( "    %s\n", Data->extensions_used[ i ] );
        }
    }

    if ( Data->extensions_required_count > 0 ) {
        GLogger.Printf( "Required extensions:\n" );
        for ( int i = 0; i < Data->extensions_required_count; i++ ) {
            GLogger.Printf( "    %s\n", Data->extensions_required[ i ] );
        }
    }

    if ( ctx.bSkeletal ) {
        // read meshes
        //for ( int i = 0; i < Data->meshes_count; i++ ) {
        //    ReadMesh( ctx, &Data->meshes[i], Float3x4::Identity(), Float3x3::Identity(), MeshAsset );
        //}

        // read meshes hierarchy
        for ( int i = 0; i < Data->scenes_count; i++ ) {
            cgltf_scene * scene = &Data->scene[ i ];

            GLogger.Printf( "Scene \"%s\" nodes %d\n", scene->name, scene->nodes_count );

            for ( int n = 0; n < scene->nodes_count; n++ ) {
                cgltf_node * node = scene->nodes[ n ];

                ReadNode_r( ctx, node, MeshAsset );
            }
        }

    } else {
        // read meshes hierarchy
        for ( int i = 0; i < Data->scenes_count; i++ ) {
            cgltf_scene * scene = &Data->scene[ i ];

            GLogger.Printf( "Scene \"%s\" nodes %d\n", scene->name, scene->nodes_count );

            for ( int n = 0; n < scene->nodes_count; n++ ) {
                cgltf_node * node = scene->nodes[ n ];

                ReadNode_r( ctx, node, MeshAsset );
            }
        }
    }

    return true;
}

bool LoadGLTF( const char * FileName, SMeshAsset & MeshAsset, SSkeletonAsset & SkeletonAsset, TStdVector< SAnimationAsset > & Animations ) {
    bool ret = false;
    AFileStream f;

    MeshAsset.Clear();
    SkeletonAsset.Clear();
    Animations.Clear();

    AString path = FileName;
    path.ClipFilename();
    path += "/";

    if ( !f.OpenRead( FileName ) ) {
        GLogger.Printf( "Couldn't open %s\n", FileName );
        return false;
    }

    size_t size = f.SizeInBytes();

    int hunkMark = GHunkMemory.SetHunkMark();

    void * buf = GHunkMemory.HunkMemory( size, 1 );
    f.Read( buf, size );

    void * memoryBuffer = GHunkMemory.HunkMemory( MAX_MEMORY_GLTF, 1 );
    totalAllocatedGLTF = 0;

    cgltf_options options;

    Core::ZeroMem( &options, sizeof( options ) );

    options.memory_alloc = cgltf_alloc;
    options.memory_free = cgltf_free;
    options.memory_user_data = memoryBuffer;

    cgltf_data * data = NULL;

    cgltf_result result = cgltf_parse( &options, buf, size, &data );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s : %s\n", FileName, GetErrorString( result ) );
        goto fin;
    }

    result = cgltf_validate( data );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s : %s\n", FileName, GetErrorString( result ) );
        goto fin;
    }

    result = cgltf_load_buffers( &options, data, path.CStr() );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s buffers : %s\n", FileName, GetErrorString( result ) );
        goto fin;
    }

    ret = ReadGLTF( data, MeshAsset, SkeletonAsset, Animations );

fin:
    GHunkMemory.ClearToMark( hunkMark );

    return ret;
}


bool LoadGeometryGLTF( const char * FileName, SMeshAsset & MeshAsset ) {
    bool ret = false;
    AFileStream f;

    MeshAsset.Clear();

    AString path = FileName;
    path.ClipFilename();
    path += "/";

    if ( !f.OpenRead( FileName ) ) {
        GLogger.Printf( "Couldn't open %s\n", FileName );
        return false;
    }

    size_t size = f.SizeInBytes();

    int hunkMark = GHunkMemory.SetHunkMark();

    void * buf = GHunkMemory.HunkMemory( size, 1 );
    f.Read( buf, size );

    void * memoryBuffer = GHunkMemory.HunkMemory( MAX_MEMORY_GLTF, 1 );
    totalAllocatedGLTF = 0;

    cgltf_options options;

    Core::ZeroMem( &options, sizeof( options ) );

    options.memory_alloc = cgltf_alloc;
    options.memory_free = cgltf_free;
    options.memory_user_data = memoryBuffer;

    cgltf_data * data = NULL;

    cgltf_result result = cgltf_parse( &options, buf, size, &data );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s : %s\n", FileName, GetErrorString( result ) );
        goto fin;
    }

    result = cgltf_validate( data );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s : %s\n", FileName, GetErrorString( result ) );
        goto fin;
    }

    result = cgltf_load_buffers( &options, data, path.CStr() );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s buffers : %s\n", FileName, GetErrorString( result ) );
        goto fin;
    }

    ret = ReadGeometryGLTF( data, MeshAsset );

fin:
    GHunkMemory.ClearToMark( hunkMark );

    return ret;
}
#endif
