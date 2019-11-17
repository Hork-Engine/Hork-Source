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

#include <Engine/Resource/Public/AssetImporter.h>
#include <Engine/Resource/Public/Asset.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/Core/Public/Guid.h>
#include <Engine/Core/Public/Logger.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define MAX_MEMORY_GLTF     (16<<20) // enough?

struct ALinearAllocator {

    ALinearAllocator() {

    }

    ~ALinearAllocator() {
        GLogger.Printf( "Total allocs: %d\n", totalAllocs );
        Purge();
    }

    void * Alloc( size_t _SizeInBytes );

    void Purge() {
        for ( memchunk_t * chunk = chunks ; chunk ; ) {
            memchunk_t * next = chunk->next;
            GHeapMemory.HeapFree( chunk );
            chunk = next;
        }
        chunks = nullptr;
    }

private:

    struct memchunk_t {
        size_t totalAllocated;
        memchunk_t * next;
    };

    size_t totalAllocs = 0;
    memchunk_t * chunks = nullptr;
};

void * ALinearAllocator::Alloc( size_t _SizeInBytes ) {
    memchunk_t * chunk = chunks;

    if ( !chunk || chunk->totalAllocated + _SizeInBytes >= MAX_MEMORY_GLTF ) {
        size_t chunkSize = Math::Max< size_t >( _SizeInBytes, MAX_MEMORY_GLTF );
        chunk = (memchunk_t*)GHeapMemory.HeapAlloc( chunkSize + sizeof( memchunk_t ), 1 );
        chunk->next = chunks;
        chunk->totalAllocated = 0;
        chunks = chunk;
        totalAllocs++;
    }

    void * ptr = reinterpret_cast< byte * >( chunk + 1 ) + chunk->totalAllocated;
    chunk->totalAllocated += _SizeInBytes;

    return ptr;
}

static void * cgltf_alloc( void * user, cgltf_size size ) {
    ALinearAllocator & Allocator = *static_cast< ALinearAllocator * >( user );
    return Allocator.Alloc( size );
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

        memcpy( ptr, position, sizeof( float ) * 3 );

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

        memcpy( ptr, temp.Transposed().ToPtr(), sizeof( Float3x4 ) );

        ptr += stride;
    }
}

static void unpack_weights( cgltf_accessor * acc, SMeshVertexJoint * weights ) {
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

static void unpack_joints( cgltf_accessor * acc, SMeshVertexJoint * weights ) {
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

    AN_Assert( animtimes->count > 0 );

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
                    AN_Assert( fract >= 0.0f && fract <= 1.0f );
                    vec = p0.Lerp( p1, fract );
                }
            } else if ( sampler->interpolation == cgltf_interpolation_type_step ) {
                cgltf_accessor_read_float( animdata, t, ( float * )vec.ToPtr(), 3 );
            } else if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
                float dur = nt - ct;
                float fract = ( dur == 0.0f ) ? 0.0f : ( frameTime - ct ) / dur;
                AN_Assert( fract >= 0.0f && fract <= 1.0f );

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

    AN_Assert( animtimes->count > 0 );

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
                    AN_Assert( fract >= 0.0f && fract <= 1.0f );
                    q = p0.Slerp( p1, fract ).Normalized();
                }
            } else if ( sampler->interpolation == cgltf_interpolation_type_step ) {
                cgltf_accessor_read_float( animdata, t, ( float * )q.ToPtr(), 4 );
            } else if ( sampler->interpolation == cgltf_interpolation_type_cubic_spline ) {
                float dur = nt - ct;
                float fract = ( dur == 0.0f ) ? 0.0f : ( frameTime - ct ) / dur;
                AN_Assert( fract >= 0.0f && fract <= 1.0f );

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

//static int FindJoint( cgltf_skin * Skin, cgltf_node * Node ) {
//    for ( int i = 0; i < Skin->joints_count; i++ ) {
//        if ( Skin->joints[ i ] == Node ) {
//            return i;
//        }
//    }
//    return -1;
//}

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

bool AAssetImporter::ImportGLTF( SAssetImportSettings const & InSettings ) {
    bool ret = false;
    AString const & Source = InSettings.ImportFile;

    m_Settings = InSettings;

    m_Path = InSettings.ImportFile;
    m_Path.StripFilename();
    m_Path += "/";

    AFileStream f;
    if ( !f.OpenRead( Source ) ) {
        GLogger.Printf( "Couldn't open %s\n", Source.CStr() );
        return false;
    }

    size_t size = f.SizeInBytes();

    int hunkMark = GHunkMemory.SetHunkMark();

    void * buf = GHunkMemory.HunkMemory( size, 1 );
    f.ReadBuffer( buf, size );

    //void * memoryBuffer = GHunkMemory.HunkMemory( MAX_MEMORY_GLTF, 1 );
    ALinearAllocator allocator;

    cgltf_options options;

    memset( &options, 0, sizeof( options ) );

    options.memory_alloc = cgltf_alloc;
    options.memory_free = cgltf_free;
    options.memory_user_data = &allocator;//memoryBuffer;

    cgltf_data * data = NULL;

    cgltf_result result = cgltf_parse( &options, buf, size, &data );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s : %s\n", Source.CStr(), GetErrorString( result ) );
        goto fin;
    }

    result = cgltf_validate( data );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s : %s\n", Source.CStr(), GetErrorString( result ) );
        goto fin;
    }

    result = cgltf_load_buffers( &options, data, m_Path.CStr() );
    if ( result != cgltf_result_success ) {
        GLogger.Printf( "Couldn't load %s buffers : %s\n", Source.CStr(), GetErrorString( result ) );
        goto fin;
    }

    ret = ReadGLTF( data );

    WriteAssets();

fin:

    //cgltf_free( data );

    GHunkMemory.ClearToMark( hunkMark );

    return ret;
}

void AAssetImporter::ReadSkeleton( cgltf_node * node, int parentIndex ) {
    SJoint & joint = m_Joints.Append();
    Float4x4 localTransform;

    cgltf_node_transform_local( node, (float *)localTransform.ToPtr() );
    joint.LocalTransform = Float3x4( localTransform.Transposed() );

    if ( node->name ) {
        AString::CopySafe( joint.Name, sizeof( joint.Name ), node->name );
    } else {
        AString::CopySafe( joint.Name, sizeof( joint.Name ), AString::Fmt( "unnamed_%d", m_Joints.Size()-1 ) );
    }

    GLogger.Printf( "ReadSkeleton: %s\n", node->name );

    joint.Parent = parentIndex;

    // HACK: store joint index at camera pointer
    node->camera = (cgltf_camera *)(size_t)m_Joints.Size();

    parentIndex = m_Joints.Size() - 1;

    for ( int i = 0 ; i < node->children_count ; i++ ) {
        ReadSkeleton( node->children[i], parentIndex );
    }
}

bool AAssetImporter::ReadGLTF( cgltf_data * Data ) {
    m_Data = Data;
    m_bSkeletal = Data->skins_count > 0 && m_Settings.bImportSkinning;

    m_Vertices.Clear();
    m_Weights.Clear();
    m_Indices.Clear();
    m_Meshes.Clear();
    m_Animations.clear();
    m_Textures.Clear();
    m_Materials.clear();
    m_Joints.Clear();
    m_BindposeBounds.Clear();
    m_Skin.JointIndices.Clear();
    m_Skin.OffsetMatrices.Clear();

    GLogger.Printf( "%d scenes\n", Data->scenes_count );
    GLogger.Printf( "%d skins\n", Data->skins_count );
    GLogger.Printf( "%d meshes\n", Data->meshes_count );
    GLogger.Printf( "%d nodes\n", Data->nodes_count );
    GLogger.Printf( "%d cameras\n", Data->cameras_count );
    GLogger.Printf( "%d lights\n", Data->lights_count );
    GLogger.Printf( "%d materials\n", Data->materials_count );

    if ( Data->extensions_used_count > 0 ) {
        GLogger.Printf( "Used extensions:\n" );
        for ( int i = 0; i < Data->extensions_used_count; i++ ) {
            GLogger.Printf( "    %s\n", Data->extensions_used[i] );
        }
    }

    if ( Data->extensions_required_count > 0 ) {
        GLogger.Printf( "Required extensions:\n" );
        for ( int i = 0; i < Data->extensions_required_count; i++ ) {
            GLogger.Printf( "    %s\n", Data->extensions_required[i] );
        }
    }

    if ( m_Settings.bImportTextures ) {
        m_Textures.Resize( Data->images_count );
        for ( int i = 0; i < Data->images_count; i++ ) {
            m_Textures[i].GUID.Generate();
            m_Textures[i].Image = &Data->images[i];
        }
    }

    if ( m_Settings.bImportMaterials ) {
        m_Materials.resize( Data->materials_count );
        for ( int i = 0; i < Data->materials_count; i++ ) {
            ReadMaterial( &Data->materials[i], m_Materials[i] );
        }
    }

    for ( int i = 0; i < Data->scenes_count; i++ ) {
        cgltf_scene * scene = &Data->scene[i];

        GLogger.Printf( "Scene \"%s\" nodes %d\n", scene->name, scene->nodes_count );

        for ( int n = 0; n < scene->nodes_count; n++ ) {
            cgltf_node * node = scene->nodes[n];

            ReadNode_r( node );
        }
    }

    if ( m_bSkeletal ) {

        if ( Data->skins ) {
            // FIXME: Only one skin per file supported now
            // TODO: for ( int i = 0; i < Data->skins_count; i++ ) {
            cgltf_skin * skin = &Data->skins[0];

            m_SkeletonGUID.Generate();

            m_Joints.Clear();
#if 0
            ReadSkeleton( skin->skeleton );
#else

            int rootsCount = 0;
            for ( int n = 0; n < Data->nodes_count; n++ ) {
                if ( !Data->nodes[n].parent ) {
                    rootsCount++;
                }
            }

            int parentIndex = -1;

            if ( rootsCount > 1 ) {
                // Add root node

                SJoint & joint = m_Joints.Append();

                joint.LocalTransform.SetIdentity();
                AString::CopySafe( joint.Name, sizeof( joint.Name ), AString::Fmt( "generated_root" ) );

                joint.Parent = -1;

                parentIndex = 0;
            }

            for ( int n = 0; n < Data->nodes_count; n++ ) {
                if ( !Data->nodes[n].parent ) {
                    ReadSkeleton( &Data->nodes[n], parentIndex );
                }
            }
#endif

//            for ( int n = 0; n < Data->nodes_count; n++ ) {
//                GLogger.Printf( "NODE: %s\n", Data->nodes[n].name );
//            }

            // Apply scaling by changing local joint position
            if ( m_Settings.Scale != 1.0f ) {
                Float3 transl, scale;
                Float3x3 rot;
                //Float3x4 scaleMatrix = Float3x4::Scale( Float3( m_Settings.Scale ) );
                for ( int i = 0 ; i < m_Joints.Size() ; i++ ) {
                    SJoint * joint = &m_Joints[i];

                    // Scale skeleton joints
                    joint->LocalTransform.DecomposeAll( transl, rot, scale );
                    joint->LocalTransform.Compose( transl * m_Settings.Scale, rot, scale );
                }
            }

            // Apply rotation to root node
            if ( !m_Joints.IsEmpty() ) {
                Float3x4 rotation = Float3x4( m_Settings.Rotation.ToMatrix().Transposed() );
                SJoint * joint = &m_Joints[0];
                joint->LocalTransform = rotation * joint->LocalTransform;
            }

            // Read skin
            m_Skin.JointIndices.Resize( skin->joints_count );
            m_Skin.OffsetMatrices.Resize( skin->joints_count );

            unpack_mat4_to_mat3x4( skin->inverse_bind_matrices, m_Skin.OffsetMatrices.ToPtr(), sizeof( m_Skin.OffsetMatrices[0] ) );

            Float3x4 scaleMatrix = Float3x4::Scale( Float3( m_Settings.Scale ) );

            Float3x4 rotationInverse = Float3x4( m_Settings.Rotation.ToMatrix().Inversed().Transposed() );

            for ( int i = 0 ; i < skin->joints_count ; i++ ) {
                cgltf_node * jointNode = skin->joints[i];

                // Scale offset matrix
                m_Skin.OffsetMatrices[i] = scaleMatrix * m_Skin.OffsetMatrices[i] * scaleMatrix.Inversed() * rotationInverse;

                // Map skin onto joints
                m_Skin.JointIndices[i] = -1;

                // HACK: get joint index from camera pointer
                int nodeIndex = jointNode->camera ? (size_t)jointNode->camera - 1 : m_Joints.Size();
                if ( nodeIndex >= m_Joints.Size() ) {
                    GLogger.Print( "Invalid skin\n" );
                    m_Skin.JointIndices[i] = 0;
                } else {
                    m_Skin.JointIndices[i] = nodeIndex;
                }
            }

            m_BindposeBounds = CalcBindposeBounds( m_Vertices.ToPtr(), m_Weights.ToPtr(), m_Vertices.Size(), &m_Skin, m_Joints.ToPtr(), m_Joints.Size() );

            GLogger.Printf( "Total skeleton nodes %d\n", m_Joints.Size() );
            GLogger.Printf( "Total skinned nodes %d\n", m_Skin.JointIndices.Size() );
        }

        if ( !m_Joints.IsEmpty() && m_Settings.bImportAnimations ) {
            ReadAnimations( Data );
        }
    }

    return true;
}

#if 0
#include <Engine/MaterialGraph/Public/MaterialGraph.h>

static ETextureFilter ChooseSamplerFilter( int MinFilter, int MagFilter ) {
    // TODO:...

    return TEXTURE_FILTER_MIPMAP_TRILINEAR;
}

static ETextureAddress ChooseSamplerWrap( int Wrap ) {
    // TODO:...

    return TEXTURE_ADDRESS_WRAP;
}
#endif

AAssetImporter::TextureInfo * AAssetImporter::FindTextureImage( cgltf_texture const * Texture ) {
    if ( !Texture ) {
        return nullptr;
    }
    for ( TextureInfo & texInfo : m_Textures ) {
        if ( texInfo.Image == Texture->image ) {
            return &texInfo;
        }
    }
    return nullptr;
}

void AAssetImporter::SetTextureProps( TextureInfo * Info, const char * Name, bool SRGB ) {
    if ( Info ) {
        Info->bSRGB = SRGB;

        if ( !Info->Image->name || !*Info->Image->name ) {
            Info->Image->name = const_cast< char * >( Name );
        }
    }
}

void AAssetImporter::ReadMaterial( cgltf_material * Material, MaterialInfo & Info ) {

    Info.GUID.Generate();
    Info.Material = Material;
    Info.DefaultMaterial = "/Default/Materials/Unlit";
    Info.NumTextures = 0;
    memset( Info.Uniforms, 0, sizeof(Info.Uniforms) );

    if ( Material->unlit && m_Settings.bAllowUnlitMaterials ) {
        Info.DefaultMaterial = "/Default/Materials/Unlit";

        Info.NumTextures = 1;
        Info.DefaultTexture[0] = "/Default/Textures/BaseColorWhite";

        if ( Material->has_pbr_metallic_roughness ) {
            Info.Textures[0] = FindTextureImage( Material->pbr_metallic_roughness.base_color_texture.texture );
        } else if ( Material->has_pbr_specular_glossiness ) {
            Info.Textures[0] = FindTextureImage( Material->pbr_specular_glossiness.diffuse_texture.texture );
        } else {
            Info.Textures[0] = nullptr;
        }       

        SetTextureProps( Info.Textures[0], "Texture_BaseColor", true );

        // TODO: create material graph

#if 0
        cgltf_texture_view * texView;

        if ( Material->has_pbr_metallic_roughness ) {
            texView = &Material->pbr_metallic_roughness.base_color_texture;
        } else if ( Material->has_pbr_specular_glossiness ) {
            texView = &Material->pbr_specular_glossiness.diffuse_texture;
        } else {
            texView = nullptr;
        }

        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        if ( texView ) {
            MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();

            MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();

            MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
            texCoord->Connect( inTexCoordBlock, "Value" );

            MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();

            cgltf_sampler * sampler = texView->texture->sampler;
            diffuseTexture->SamplerDesc.Filter = ChooseSamplerFilter( sampler->min_filter, sampler->mag_filter );
            diffuseTexture->SamplerDesc.AddressU = ChooseSamplerWrap( sampler->wrap_s );
            diffuseTexture->SamplerDesc.AddressV = ChooseSamplerWrap( sampler->wrap_t );

            MGSampler * textureSampler = graph->AddNode< MGSampler >();
            textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
            textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

            MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
            materialFragmentStage->Color->Connect( textureSampler, "RGBA" );

            graph->VertexStage = materialVertexStage;
            graph->FragmentStage = materialFragmentStage;
            graph->MaterialType = MATERIAL_TYPE_UNLIT;
            graph->MaterialFacing = Material->double_sided ? MATERIAL_FACE_FRONT_AND_BACK : MATERIAL_FACE_FRONT;
            graph->RegisterTextureSlot( diffuseTexture );

            Info.Graph = graph;
        } else {
            Info.Graph = nullptr; // Will be used default material
        }
#endif


    } else if ( Material->has_pbr_metallic_roughness ) {

        Info.NumTextures = 5;
        Info.DefaultTexture[0] = "/Default/Textures/BaseColorWhite"; // base color
        Info.DefaultTexture[1] = "/Default/Textures/White"; // metallic&roughness
        Info.DefaultTexture[2] = "/Default/Textures/Normal";// normal
        Info.DefaultTexture[3] = "/Default/Textures/White"; // occlusion
        Info.DefaultTexture[4] = "/Default/Textures/Black"; // emissive

        bool emissiveFactor = Material->emissive_factor[0] > 0.0f
            || Material->emissive_factor[1] > 0.0f
            || Material->emissive_factor[2] > 0.0f;

        bool factor = Material->pbr_metallic_roughness.base_color_factor[0] < 1.0f
            || Material->pbr_metallic_roughness.base_color_factor[1] < 1.0f
            || Material->pbr_metallic_roughness.base_color_factor[2] < 1.0f
            || Material->pbr_metallic_roughness.base_color_factor[3] < 1.0f
            || Material->pbr_metallic_roughness.metallic_factor < 1.0f
            || Material->pbr_metallic_roughness.roughness_factor < 1.0f
            || emissiveFactor;

        if ( emissiveFactor ) {
            Info.DefaultTexture[4] = "/Default/Textures/White"; // emissive
        }

        if ( factor ) {
            Info.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactor";

            Info.Uniforms[0] = Material->pbr_metallic_roughness.base_color_factor[0];
            Info.Uniforms[1] = Material->pbr_metallic_roughness.base_color_factor[1];
            Info.Uniforms[2] = Material->pbr_metallic_roughness.base_color_factor[2];
            Info.Uniforms[3] = Material->pbr_metallic_roughness.base_color_factor[3];

            Info.Uniforms[4] = Material->pbr_metallic_roughness.metallic_factor;
            Info.Uniforms[5] = Material->pbr_metallic_roughness.roughness_factor;
            Info.Uniforms[6] = 0;
            Info.Uniforms[7] = 0;

            Info.Uniforms[8]  = Material->emissive_factor[0];
            Info.Uniforms[9]  = Material->emissive_factor[1];
            Info.Uniforms[10] = Material->emissive_factor[2];

        } else {
            Info.DefaultMaterial = "/Default/Materials/PBRMetallicRoughness";
        }

        Info.Textures[0] = FindTextureImage( Material->pbr_metallic_roughness.base_color_texture.texture );
        Info.Textures[1] = FindTextureImage( Material->pbr_metallic_roughness.metallic_roughness_texture.texture );
        Info.Textures[2] = FindTextureImage( Material->normal_texture.texture );
        Info.Textures[3] = FindTextureImage( Material->occlusion_texture.texture );
        Info.Textures[4] = FindTextureImage( Material->emissive_texture.texture );

        SetTextureProps( Info.Textures[0], "Texture_BaseColor", true );
        SetTextureProps( Info.Textures[1], "Texture_MetallicRoughness", false );
        SetTextureProps( Info.Textures[2], "Texture_Normal", false );
        SetTextureProps( Info.Textures[3], "Texture_Occlusion", true );
        SetTextureProps( Info.Textures[4], "Texture_Emissive", true );

        // TODO: create material graph

        //cgltf_pbr_metallic_roughness & tex = Material->pbr_metallic_roughness;

        // TODO: create pbr material

    } else if ( Material->has_pbr_specular_glossiness ) {

        GLogger.Printf( "Warning: pbr specular glossiness workflow is not supported yet\n" );

        Info.NumTextures = 5;
        Info.DefaultTexture[0] = "/Default/Textures/BaseColorWhite"; // diffuse
        Info.DefaultTexture[1] = "/Default/Textures/White"; // specular&glossiness
        Info.DefaultTexture[2] = "/Default/Textures/Normal";// normal
        Info.DefaultTexture[3] = "/Default/Textures/White"; // occlusion
        Info.DefaultTexture[4] = "/Default/Textures/Black"; // emissive

        bool emissiveFactor = Material->emissive_factor[0] > 0.0f
            || Material->emissive_factor[1] > 0.0f
            || Material->emissive_factor[2] > 0.0f;

        bool factor = Material->pbr_specular_glossiness.diffuse_factor[0] < 1.0f
            || Material->pbr_specular_glossiness.diffuse_factor[1] < 1.0f
            || Material->pbr_specular_glossiness.diffuse_factor[2] < 1.0f
            || Material->pbr_specular_glossiness.diffuse_factor[3] < 1.0f
            || Material->pbr_specular_glossiness.specular_factor[0] < 1.0f
            || Material->pbr_specular_glossiness.glossiness_factor < 1.0f
            || emissiveFactor;

        if ( emissiveFactor ) {
            Info.DefaultTexture[4] = "/Default/Textures/White"; // emissive
        }

        if ( factor ) {
            Info.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactor";
            //Info.DefaultMaterial = "/Default/Materials/PBRSpecularGlossinessFactor";

            Info.Uniforms[0] = Material->pbr_specular_glossiness.diffuse_factor[0];
            Info.Uniforms[1] = Material->pbr_specular_glossiness.diffuse_factor[1];
            Info.Uniforms[2] = Material->pbr_specular_glossiness.diffuse_factor[2];
            Info.Uniforms[3] = Material->pbr_specular_glossiness.diffuse_factor[3];

            Info.Uniforms[4] = Material->pbr_specular_glossiness.specular_factor[0];
            Info.Uniforms[5] = Material->pbr_specular_glossiness.glossiness_factor;
            Info.Uniforms[6] = 0;
            Info.Uniforms[7] = 0;

            Info.Uniforms[8]  = Material->emissive_factor[0];
            Info.Uniforms[9]  = Material->emissive_factor[1];
            Info.Uniforms[10] = Material->emissive_factor[2];

        } else {
            Info.DefaultMaterial = "/Default/Materials/PBRMetallicRoughness";
            //Info.DefaultMaterial = "/Default/Materials/PBRSpecularGlossiness";
        }

        Info.Textures[0] = FindTextureImage( Material->pbr_specular_glossiness.diffuse_texture.texture );
        Info.Textures[1] = FindTextureImage( Material->pbr_specular_glossiness.specular_glossiness_texture.texture );
        Info.Textures[2] = FindTextureImage( Material->normal_texture.texture );
        Info.Textures[3] = FindTextureImage( Material->occlusion_texture.texture );
        Info.Textures[4] = FindTextureImage( Material->emissive_texture.texture );

        SetTextureProps( Info.Textures[0], "Texture_Diffuse", true );
        SetTextureProps( Info.Textures[1], "Texture_SpecularGlossiness", false );
        SetTextureProps( Info.Textures[2], "Texture_Normal", false );
        SetTextureProps( Info.Textures[3], "Texture_Occlusion", true );
        SetTextureProps( Info.Textures[4], "Texture_Emissive", true );
    }
}

void AAssetImporter::ReadNode_r( cgltf_node * Node ) {

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

    if ( m_Settings.bImportMeshes || m_Settings.bImportSkinning || m_Settings.bImportAnimations )
    {
        ReadMesh( Node );
    }

    for ( int n = 0; n < Node->children_count; n++ ) {
        cgltf_node * child = Node->children[n];

        ReadNode_r( child );
    }
}

void AAssetImporter::ReadMesh( cgltf_node * Node ) {
    cgltf_mesh * mesh = Node->mesh;

    if ( !mesh ) {
        return;
    }

    Float3x4 globalTransform;
    Float3x3 normalMatrix;
    Float4x4 temp;
    cgltf_node_transform_world( Node, (float *)temp.ToPtr() );
    Float3x4 rotation = Float3x4( m_Settings.Rotation.ToMatrix().Transposed() );
    globalTransform = rotation * Float3x4( temp.Transposed() );
    globalTransform.DecomposeNormalMatrix( normalMatrix );

    ReadMesh( mesh, Float3x4::Scale( Float3( m_Settings.Scale ) ) * globalTransform, normalMatrix );
}

void AAssetImporter::ReadMesh( cgltf_mesh * Mesh, Float3x4 const & GlobalTransform, Float3x3 const & NormalMatrix ) {
    struct ASortFunction {
        bool operator() ( cgltf_primitive const & _A, cgltf_primitive const & _B ) {
            return (_A.material < _B.material);
        }
    } SortFunction;

    StdSort( &Mesh->primitives[0], &Mesh->primitives[Mesh->primitives_count], SortFunction );

    cgltf_material * material = nullptr;

    MeshInfo * meshInfo = nullptr;

    for ( int i = 0; i < Mesh->primitives_count; i++ ) {
        cgltf_primitive * prim = &Mesh->primitives[i];

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
            cgltf_attribute * attrib = &prim->attributes[a];

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

        if ( !material || material != prim->material || !m_Settings.bMergePrimitives ) {

            meshInfo = &m_Meshes.Append();
            meshInfo->GUID.Generate();
            meshInfo->BaseVertex = m_Vertices.Size();
            meshInfo->FirstIndex = m_Indices.Size();
            meshInfo->VertexCount = 0;
            meshInfo->IndexCount = 0;
            meshInfo->Mesh = Mesh;
            meshInfo->Material = prim->material;
            meshInfo->BoundingBox.Clear();

            material = prim->material;
        }

        int firstVert = m_Vertices.Size();
        m_Vertices.Resize( firstVert + vertexCount );

        int vertexOffset = firstVert - meshInfo->BaseVertex;

        int firstIndex = m_Indices.Size();
        int indexCount;
        if ( prim->indices ) {
            indexCount = prim->indices->count;
            m_Indices.Resize( firstIndex + indexCount );
            unsigned int * pInd = m_Indices.ToPtr() + firstIndex;
            for ( int index = 0; index < indexCount; index++, pInd++ ) {
                *pInd = vertexOffset + cgltf_accessor_read_index( prim->indices, index );
            }
        } else {
            indexCount = vertexCount;
            m_Indices.Resize( firstIndex + indexCount );
            unsigned int * pInd = m_Indices.ToPtr() + firstIndex;
            for ( int index = 0; index < indexCount; index++, pInd++ ) {
                *pInd = vertexOffset + index;
            }
        }

        unpack_vec2_or_vec3( position, &m_Vertices[firstVert].Position, sizeof( SMeshVertex ) );

        if ( texcoord ) {
            unpack_vec2( texcoord, &m_Vertices[firstVert].TexCoord, sizeof( SMeshVertex ) );
        } else {
            for ( int v = 0; v < vertexCount; v++ ) {
                m_Vertices[firstVert + v].TexCoord.Clear();
            }
        }

        if ( normal && (normal->type == cgltf_type_vec2 || normal->type == cgltf_type_vec3) && normal->count == vertexCount ) {
            unpack_vec2_or_vec3( normal, &m_Vertices[firstVert].Normal, sizeof( SMeshVertex ) );

            SMeshVertex * pVert = m_Vertices.ToPtr() + firstVert;
            for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                pVert->Normal.NormalizeSelf();
            }

        } else {
            // TODO: compute normals

            GLogger.Printf( "Warning: no normals\n" );

            for ( int v = 0; v < vertexCount; v++ ) {
                m_Vertices[firstVert + v].Normal = Float3( 0, 1, 0 );
            }
        }

        if ( tangent && (tangent->type == cgltf_type_vec4) && tangent->count == vertexCount ) {
            unpack_vec4( tangent, (Float4 *)&m_Vertices[firstVert].Tangent, sizeof( SMeshVertex ) );

            //CalcTangentSpace( m_Vertices.ToPtr() + meshInfo->BaseVertex, m_Vertices.Size() - meshInfo->BaseVertex, m_Indices.ToPtr() + firstIndex, indexCount );
        } else {

            if ( texcoord ) {
                CalcTangentSpace( m_Vertices.ToPtr() + meshInfo->BaseVertex, m_Vertices.Size() - meshInfo->BaseVertex, m_Indices.ToPtr() + firstIndex, indexCount );
            } else {
                SMeshVertex * pVert = m_Vertices.ToPtr() + firstVert;
                for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                    pVert->Tangent.X = 1;
                    pVert->Tangent.Y = 0;
                    pVert->Tangent.Z = 0;
                    pVert->Handedness = 0;
                }
            }

        }

        if ( weights && (weights->type == cgltf_type_vec4) && weights->count == vertexCount
            && joints && (joints->type == cgltf_type_vec4) && joints->count == vertexCount ) {
            m_Weights.Resize( m_Vertices.Size() );
            unpack_weights( weights, &m_Weights[firstVert] );
            unpack_joints( joints, &m_Weights[firstVert] );
        }

        AN_UNUSED( color );

        SMeshVertex * pVert = m_Vertices.ToPtr() + firstVert;

        if ( /*m_Settings.bSingleModel && */!m_bSkeletal )
        {
            for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                // Pretransform vertices
                pVert->Position = Float3( GlobalTransform * pVert->Position );
                pVert->Normal = NormalMatrix * pVert->Normal;
                pVert->Tangent = NormalMatrix * pVert->Tangent;

                // Calc bounding box
                meshInfo->BoundingBox.AddPoint( pVert->Position );
            }
        }
        else
        {
            Float3x3 rotation = m_Settings.Rotation.ToMatrix();
            for ( int v = 0; v < vertexCount; v++, pVert++ ) {
                pVert->Position = m_Settings.Scale * Float3( rotation * pVert->Position );
                pVert->Normal = rotation * pVert->Normal;
                pVert->Tangent = rotation * pVert->Tangent;

                // Calc bounding box
                meshInfo->BoundingBox.AddPoint( pVert->Position );
            }
        }

        meshInfo->VertexCount += vertexCount;
        meshInfo->IndexCount += indexCount;

        //cgltf_morph_target* targets;
        //cgltf_size targets_count;
    }

    //for ( int i = 0; i < mesh->weights_count; i++ ) {
    //    cgltf_float * w = &mesh->weights[ i ];
    //}

    GLogger.Printf( "Subparts %d, Primitives %d\n", m_Meshes.Size(), Mesh->primitives_count );

    if ( m_bSkeletal ) {
        int numWeights = m_Weights.Size();
        int numVertices = m_Vertices.Size();
        if ( numWeights != numVertices ) {

            GLogger.Printf( "Warning: invalid mesh (num weights != num vertices)\n" );

            m_Weights.Resize( numVertices );

            // Clear
            int count = numVertices - numWeights;
            for ( int i = 0 ; i < count ; i++ ) {
                for ( int j = 0 ; j < 4 ; j++ ) {
                    m_Weights[ numWeights + i ].JointIndices[j] = 0;
                    m_Weights[ numWeights + i ].JointWeights[j] = 0;
                }
            }
        }
    }
}

void AAssetImporter::ReadAnimations( cgltf_data * Data ) {
    m_Animations.resize( Data->animations_count );
    for ( int animIndex = 0; animIndex < Data->animations_count; animIndex++ ) {
        AnimationInfo & animation = m_Animations[animIndex];

        ReadAnimation( &Data->animations[animIndex], animation );

        CalcBoundingBoxes( m_Vertices.ToPtr(),
                           m_Weights.ToPtr(),
                           m_Vertices.Size(),
                           &m_Skin,
                           m_Joints.ToPtr(),
                           m_Joints.Size(),
                           animation.FrameCount,
                           animation.Channels.ToPtr(),
                           animation.Channels.Size(),
                           animation.Transforms.ToPtr(),
                           animation.Bounds );
    }
}

void AAssetImporter::ReadAnimation( cgltf_animation * Anim, AnimationInfo & Animation ) {
    const int framesPerSecond = 30;
    //    float gcd = 0;
    float maxDuration = 0;

    for ( int ch = 0; ch < Anim->channels_count; ch++ ) {
        cgltf_animation_channel * channel = &Anim->channels[ch];
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

    Animation.GUID.Generate();
    Animation.Name = Anim->name ? Anim->name : "Animation";
    Animation.FrameDelta = frameDelta;
    Animation.FrameCount = numFrames;         // frames count, animation duration is FrameDelta * ( FrameCount - 1 )

    for ( int ch = 0; ch < Anim->channels_count; ch++ ) {

        cgltf_animation_channel * channel = &Anim->channels[ch];
        cgltf_animation_sampler * sampler = channel->sampler;

        if ( !IsChannelValid( channel ) ) {
            continue;
        }

        // HACK: get joint index from camera pointer
        int nodeIndex = channel->target_node->camera ? (size_t)channel->target_node->camera - 1 : m_Joints.Size();
        if ( nodeIndex >= m_Joints.Size() ) {
            GLogger.Printf( "Warning: joint %s is not found\n", channel->target_node->name );
            continue;
        }

        // just for debug
        //if ( channel->target_node->name && AString::Icmp( channel->target_node->name, "L_arm_029" ) ) {
        //    continue;
        //}

        int mergedChannel;
        for ( mergedChannel = 0; mergedChannel < Animation.Channels.Size(); mergedChannel++ ) {
            if ( nodeIndex == Animation.Channels[mergedChannel].JointIndex ) {
                break;
            }
        }

        SAnimationChannel * jointAnim;

        if ( mergedChannel < Animation.Channels.Size() ) {
            jointAnim = &Animation.Channels[mergedChannel];
        } else {
            jointAnim = &Animation.Channels.Append();
            jointAnim->JointIndex = nodeIndex;
            jointAnim->TransformOffset = Animation.Transforms.Size();
            jointAnim->bHasPosition = false;
            jointAnim->bHasRotation= false;
            jointAnim->bHasScale = false;
            Animation.Transforms.Resize( Animation.Transforms.Size() + numFrames );

            Float3 position;
            Float3x3 rotation;
            Quat q;
            Float3 scale;
            m_Joints[nodeIndex].LocalTransform.DecomposeAll( position, rotation, scale );
            q.FromMatrix( rotation );
            for ( int f = 0; f < numFrames; f++ ) {
                ATransform & transform = Animation.Transforms[jointAnim->TransformOffset + f];
                transform.Position = position;
                transform.Scale = scale;
                transform.Rotation = q;
            }
        }

        switch ( channel->target_path ) {
        case cgltf_animation_path_type_translation:
            jointAnim->bHasPosition = true;

            for ( int f = 0; f < numFrames; f++ ) {
                ATransform & transform = Animation.Transforms[jointAnim->TransformOffset + f];

                sample_vec3( sampler, f * frameDelta, transform.Position );

                transform.Position *= m_Settings.Scale;
            }

            break;
        case cgltf_animation_path_type_rotation:
            jointAnim->bHasRotation = true;

            for ( int f = 0; f < numFrames; f++ ) {
                ATransform & transform = Animation.Transforms[jointAnim->TransformOffset + f];

                sample_quat( sampler, f * frameDelta, transform.Rotation );
            }

            break;
        case cgltf_animation_path_type_scale:
            jointAnim->bHasScale = true;

            for ( int f = 0; f < numFrames; f++ ) {
                ATransform & transform = Animation.Transforms[jointAnim->TransformOffset + f];

                sample_vec3( sampler, f * frameDelta, transform.Scale );
            }

            break;
        default:
            GLogger.Printf( "Warning: Unsupported target path\n" );
            break;
        }

        for ( int f = 0; f < numFrames; f++ ) {
            ATransform & transform = Animation.Transforms[jointAnim->TransformOffset + f];

            float frameTime = f * frameDelta;

            switch ( channel->target_path ) {
            case cgltf_animation_path_type_translation:
                sample_vec3( sampler, frameTime, transform.Position );
                transform.Position *= m_Settings.Scale;
                break;
            case cgltf_animation_path_type_rotation:
                sample_quat( sampler, frameTime, transform.Rotation );
                break;
            case cgltf_animation_path_type_scale:
                sample_vec3( sampler, frameTime, transform.Scale );
                break;
            default:
                GLogger.Printf( "Warning: Unsupported target path\n" );
                f = numFrames;
                break;
            }
        }
    }

    for ( int channel = 0 ; channel < Animation.Channels.Size() ; channel++ ) {
        SAnimationChannel * jointAnim = &Animation.Channels[channel];

        if ( jointAnim->JointIndex == 0 && jointAnim->bHasRotation ) {
            for ( int frameIndex = 0 ; frameIndex < numFrames ; frameIndex++ ) {

                ATransform & transform = Animation.Transforms[jointAnim->TransformOffset + frameIndex];

                transform.Rotation = m_Settings.Rotation * transform.Rotation;
            }
        }
    }
}

#if 0
Float3x4 CalcJointWorldTransform( SJoint const & InJoint, SJoint const * InJoints ) {
    if ( InJoint.Parent == -1 ) {
        return InJoint.LocalTransform;
    }
    return CalcJointWorldTransform( InJoints[InJoint.Parent], InJoints ) * InJoint.LocalTransform;
}
#endif

void AAssetImporter::WriteAssets() {

    if ( m_Settings.bImportTextures ) {
        WriteTextures();
    }

    if ( m_Settings.bImportMaterials ) {
        WriteMaterials();
    }

    if ( m_Settings.bImportSkinning ) {
        if ( m_Settings.bImportSkeleton ) {
            WriteSkeleton();
        }

        if ( m_Settings.bImportAnimations ) {
            WriteAnimations();
        }

    }

    if ( m_Settings.bImportMeshes ) {

        if ( m_Settings.bSingleModel || m_bSkeletal ) {
            WriteSingleModel();
        } else {
            WriteMeshes();
        }

    }
}

void AAssetImporter::WriteTextures() {
    for ( TextureInfo & tex : m_Textures ) {
        WriteTexture( tex );
    }
}

void AAssetImporter::WriteTexture( TextureInfo const & tex ) {
    AFileStream f;
    AString fileName = GeneratePhysicalPath( tex.Image->name && *tex.Image->name ? tex.Image->name : "Texture" );
    AString sourceFileName = m_Path + tex.Image->uri;
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileSystemPath ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return;
    }

    AImage image;
    if ( !image.LoadLDRI( sourceFileName.CStr(), tex.bSRGB, true ) ) {
        return;
    }

    STexturePixelFormat texturePixelFormat;
    if ( !GetAppropriatePixelFormat( image, texturePixelFormat ) ) {
        return;
    }

    GResourceManager.SetResourceGUID( tex.GUID, ("/Root/" + fileName).CStr() );

    uint32_t textureType = TEXTURE_2D;
    uint32_t w = image.Width, h = image.Height, d = 1, numLods = image.NumLods;

    f.WriteUInt32( FMT_FILE_TYPE_TEXTURE );
    f.WriteUInt32( FMT_VERSION_TEXTURE );
    //f.WriteString( tex.GUID.CStr() );
    f.WriteUInt32( textureType );
    f.WriteObject( texturePixelFormat );
    f.WriteUInt32( w );
    f.WriteUInt32( h );
    f.WriteUInt32( d );
    f.WriteUInt32( numLods );

    int pixelByteLength = texturePixelFormat.SizeInBytesUncompressed();
    int stride;
    byte * pSrc = (byte *)image.pRawData;
    for ( int lod = 0 ; lod < numLods ; lod++ ) {
        uint32_t lodWidth = Math::Max( 1, image.Width >> lod );
        uint32_t lodHeight = Math::Max( 1, image.Height >> lod );

        f.WriteUInt32( lodWidth );
        f.WriteUInt32( lodHeight );
        f.WriteUInt32( 1 ); // lodDepth

        stride = lodWidth * lodHeight * pixelByteLength;

        f.WriteBuffer( pSrc, stride );

        pSrc += stride;
    }

    //f.WriteUInt32( 1 ); // num source files

    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return;
    }

    f.Printf( "GUID \"%s\"\n", tex.GUID.CStr() );
    f.Printf( "Sources [ \"%s\" ]\n", sourceFileName.CStr() );

#if 0
    // Write original image
    AFileStream imageData;
    if ( imageData.OpenRead( m_Path + tex.Image->uri ) ) {
        long size = imageData.SizeInBytes();
        byte * buf = (byte *)GHeapMemory.HeapAlloc( size, 1 );
        imageData.Read( buf, size );
        f << size_t(size);
        f.Write( buf, size );
        GHeapMemory.HeapFree( buf );
    } else {
        f << size_t(0);
    }
#endif

#if 0
    // Write original image
    AFileStream imageData;
    const char * ext = &tex.Image->uri[AString::FindExt( tex.Image->uri )];
    if ( !imageData.OpenRead( m_Path + tex.Image->uri ) ) {
        return;
    }
    long size = imageData.SizeInBytes();
    byte * buf = (byte *)GHeapMemory.HeapAlloc( size, 1 );
    imageData.Read( buf, size );
    AFileStream imageOutput;
    if ( imageOutput.OpenWrite( fileName + ext ) ) {
        imageOutput.Write( buf, size );
    }
    GHeapMemory.HeapFree( buf );
#endif
}

void AAssetImporter::WriteMaterials() {
    for ( MaterialInfo & m : m_Materials ) {
        WriteMaterial( m );
    }
}

void AAssetImporter::WriteMaterial( MaterialInfo const & m ) {
    AFileStream f;
    AString fileName = GeneratePhysicalPath( "MaterialInstance" );
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileSystemPath ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return;
    }

    GResourceManager.SetResourceGUID( m.GUID, ( "/Root/" + fileName ).CStr() );

    f.WriteUInt32( FMT_FILE_TYPE_MATERIAL_INSTANCE );
    f.WriteUInt32( FMT_VERSION_MATERIAL_INSTANCE );
    //f.WriteString( m.GUID.CStr() );
    //f.WriteString( m.DefaultMaterial ); // material GUID

//    f.WriteUInt32( m.NumTextures );

//    for ( int i = 0 ; i < m.NumTextures ; i++ ) {
//        if ( m.Textures[i] ) {
//            f.WriteString( m.Textures[i]->GUID.CStr() );
//        } else {
//            f.WriteString( m.DefaultTexture[i] );
//        }
//    }

    // Uniforms
//    for ( int i = 0 ; i < 16 ; i++ ) {
//        f.WriteFloat( m.Uniforms[i] );
//    }

    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return;
    }

    f.Printf( "GUID \"%s\"\n", m.GUID.CStr() );
    f.Printf( "Material \"%s\"\n", m.DefaultMaterial );
    f.Printf( "Textures [\n" );
    for ( int i = 0 ; i < m.NumTextures ; i++ ) {
        if ( m.Textures[i] ) {
            f.Printf( "\"%s\"\n", m.Textures[i]->GUID.CStr() );
        } else {
            f.Printf( "\"%s\"\n", m.DefaultTexture[i] );
        }
    }
    f.Printf( "]\n" );
    f.Printf( "Uniforms [\n" );
    for ( int i = 0 ; i < MAX_MATERIAL_UNIFORMS ; i++ ) {
        f.Printf( "\"%s\"\n", Float(m.Uniforms[i]).ToString().CStr() );
    }
    f.Printf( "]\n" );
}

AString ValidateFileName( const char * FileName ) {
    AString ValidatedName = FileName;

    for ( int i = 0 ; i < ValidatedName.Length() ; i++ ) {
        char & Ch = ValidatedName[i];

        switch ( Ch ) {
        case ':':
        case '\\':
        case '/':
        case '?':
        case '@':
        case '$':
        case '*':
            Ch = '_';
            break;
        }
    }

    return ValidatedName;
}

AString AAssetImporter::GeneratePhysicalPath( const char * DesiredName ) {
    AString sourceName = m_Settings.ImportFile;

    sourceName.StripPath();
    sourceName.StripExt();

    AString validatedName = ValidateFileName( DesiredName );

    AString path = m_Settings.OutputPath + "/" + sourceName + "_" + validatedName;
    AString result = path + ".asset";

    int uniqueNumber = 0;

    while ( Core::IsFileExists( ( GResourceManager.GetRootPath() + result ).CStr() ) ) {
        result = path + "_" + Int( ++uniqueNumber ).ToString() + ".asset";
    }

    return result;
}

AString AAssetImporter::GetMaterialGUID( cgltf_material * Material ) {
    for ( MaterialInfo & m : m_Materials ) {
        if ( m.Material == Material ) {
            return m.GUID.ToString();
        }
    }
    return "/Default/MaterialInstance/Default";
}

void AAssetImporter::WriteSkeleton() {

    if ( !m_Joints.IsEmpty() ) {
        AFileStream f;
        AString fileName = GeneratePhysicalPath( "Skeleton" );
        AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

        if ( !f.OpenWrite( fileSystemPath ) ) {
            GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
            return;
        }

        GResourceManager.SetResourceGUID( m_SkeletonGUID, ("/Root/" + fileName).CStr() );

        f.WriteUInt32( FMT_FILE_TYPE_SKELETON );
        f.WriteUInt32( FMT_VERSION_SKELETON );
        //f.WriteString( m_SkeletonGUID.ToString() );
        f.WriteArrayOfStructs( m_Joints );
        f.WriteObject( m_BindposeBounds );

        //
        // Write meta file
        //

        AString metaFilePath = fileSystemPath;

        metaFilePath.StripExt();
        metaFilePath += ".asset_meta";

        if ( !f.OpenWrite( metaFilePath ) ) {
            GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
            return;
        }

        f.Printf( "GUID \"%s\"\n", m_SkeletonGUID.CStr() );
    }
}

void AAssetImporter::WriteAnimations() {
    for ( AnimationInfo & animation : m_Animations ) {
        WriteAnimation( animation );
    }
}

void AAssetImporter::WriteAnimation( AnimationInfo const & Animation ) {
    AFileStream f;
    AString fileName = GeneratePhysicalPath( Animation.Name.CStr() );
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileSystemPath ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return;
    }

    GResourceManager.SetResourceGUID( Animation.GUID, ("/Root/" + fileName).CStr() );

    f.WriteUInt32( FMT_FILE_TYPE_ANIMATION );
    f.WriteUInt32( FMT_VERSION_ANIMATION );
    //f.WriteString( Animation.GUID.ToString() );
    f.WriteFloat( Animation.FrameDelta );
    f.WriteUInt32( Animation.FrameCount );
    f.WriteArrayOfStructs( Animation.Channels );
    f.WriteArrayOfStructs( Animation.Transforms );
    f.WriteArrayOfStructs( Animation.Bounds );

    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return;
    }

    f.Printf( "GUID \"%s\"\n", Animation.GUID.CStr() );
}

void AAssetImporter::WriteSingleModel() {

    if ( m_Meshes.IsEmpty() ) {
        return;
    }

    AFileStream f;
    AString fileName = GeneratePhysicalPath( "Mesh" );
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileSystemPath ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return;
    }

    AGUID GUID;
    GUID.Generate();

    GResourceManager.SetResourceGUID( GUID, ("/Root/" + fileName).CStr() );

    bool bSkinnedMesh = m_bSkeletal;

    BvAxisAlignedBox BoundingBox;
    BoundingBox.Clear();
    for ( MeshInfo const & meshInfo : m_Meshes ) {
        BoundingBox.AddAABB( meshInfo.BoundingBox );
    }

    bool bRaycastBVH = m_Settings.bGenerateRaycastBVH && !bSkinnedMesh;

    f.WriteUInt32( FMT_FILE_TYPE_MESH );
    f.WriteUInt32( FMT_VERSION_MESH );
    //f.WriteString( GUID.ToString() );
    f.WriteBool(bSkinnedMesh );
    f.WriteBool( false );         // dynamic storage
    f.WriteObject( BoundingBox );
    f.WriteArrayUInt32( m_Indices );
    f.WriteArrayOfStructs( m_Vertices );
    if ( bSkinnedMesh ) {
        f.WriteArrayOfStructs( m_Weights );
    } else {
        f.WriteUInt32( 0 ); // weights count
    }
    f.WriteBool( bRaycastBVH ); // only for static meshes
    f.WriteUInt16( m_Settings.RaycastPrimitivesPerLeaf );

    // Write subparts
    f.WriteUInt32( m_Meshes.Size() ); // subparts count
    UInt n = 0;
    for ( MeshInfo const & meshInfo : m_Meshes ) {
        if ( meshInfo.Mesh->name ) {
            f.WriteString( meshInfo.Mesh->name );
        } else {
            f.WriteString( AString( "Subpart_" ) + n.ToString() );
        }
        f.WriteInt32( meshInfo.BaseVertex );
        f.WriteUInt32( meshInfo.FirstIndex );
        f.WriteUInt32( meshInfo.VertexCount );
        f.WriteUInt32( meshInfo.IndexCount );
        //f.WriteString( GetMaterialGUID( meshInfo.Material ) );
        f.WriteObject( meshInfo.BoundingBox );

        n++;
    }

    if ( bRaycastBVH ) {

        ATreeAABB * aabbTree = CreateInstanceOf< ATreeAABB >();

        for ( MeshInfo const & meshInfo : m_Meshes ) {
            // Generate subpart BVH

            aabbTree->Initialize( m_Vertices.ToPtr(), m_Indices.ToPtr() + meshInfo.FirstIndex, meshInfo.IndexCount, meshInfo.BaseVertex, m_Settings.RaycastPrimitivesPerLeaf );

            // Write subpart BVH
            aabbTree->Write( f );
        }
    }

    f.WriteUInt32( 0 ); // sockets count

    if ( bSkinnedMesh ) {
        //f.WriteString( m_SkeletonGUID.ToString() );
        f.WriteArrayInt32( m_Skin.JointIndices );
        f.WriteArrayOfStructs( m_Skin.OffsetMatrices );
    } else {
        //f.WriteString( "/Default/Skeleton/Default" );
    }

    //if ( m_Settings.bGenerateStaticCollisions ) {
        //TPodArray< ACollisionTriangleSoupData::SSubpart > subparts;

        //subparts.Resize( m_Meshes.Size() );
        //for ( int i = 0 ; i < subparts.Size() ; i++ ) {
        //    subparts[i].BaseVertex = m_Meshes[i].BaseVertex;
        //    subparts[i].VertexCount = m_Meshes[i].VertexCount;
        //    subparts[i].FirstIndex = m_Meshes[i].FirstIndex;
        //    subparts[i].IndexCount = m_Meshes[i].IndexCount;
        //}

        //ACollisionTriangleSoupData * tris = CreateInstanceOf< ACollisionTriangleSoupData >();

        //tris->Initialize( (float *)&m_Vertices.ToPtr()->Position, sizeof( m_Vertices[0] ), m_Vertices.Size(),
        //    m_Indices.ToPtr(), m_Indices.Size(), subparts.ToPtr(), subparts.Size(), BoundingBox );

        //ACollisionTriangleSoupBVHData * bvh = CreateInstanceOf< ACollisionTriangleSoupBVHData >();
        //bvh->TrisData = tris;
        //bvh->BuildBVH();
        //bvh->Write( f );
    //}


    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return;
    }

    f.Printf( "GUID \"%s\"\n", GUID.CStr() );

    if ( bSkinnedMesh ) {
        f.Printf( "Skeleton \"%s\"\n", m_SkeletonGUID.ToString().CStr() );
    } else {
        f.Printf( "Skeleton \"%s\"\n", "/Default/Skeleton/Default" );
    }
    f.Printf( "Subparts [\n" );
    for ( MeshInfo const & meshInfo : m_Meshes ) {
        f.Printf( "\"%s\"\n", GetMaterialGUID( meshInfo.Material ).CStr() );
    }
    f.Printf( "]\n" );
}

void AAssetImporter::WriteMeshes() {
    for ( MeshInfo & meshInfo : m_Meshes ) {
        WriteMesh( meshInfo );
    }
}

void AAssetImporter::WriteMesh( MeshInfo const & Mesh ) {
    AFileStream f;
    AString fileName = GeneratePhysicalPath( Mesh.Mesh->name ? Mesh.Mesh->name : "Mesh" );
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileSystemPath ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return;
    }

    bool bSkinnedMesh = m_bSkeletal;
    AN_Assert( bSkinnedMesh == false );

    GResourceManager.SetResourceGUID( Mesh.GUID, ("/Root/" + fileName).CStr() );

    bool bRaycastBVH = m_Settings.bGenerateRaycastBVH;

    f.WriteUInt32( FMT_FILE_TYPE_MESH );
    f.WriteUInt32( FMT_VERSION_MESH );
    //f.WriteString( Mesh.GUID.ToString() );
    f.WriteBool( bSkinnedMesh );
    f.WriteBool( false );         // dynamic storage
    f.WriteObject( Mesh.BoundingBox );

    f.WriteUInt32( Mesh.IndexCount );
    unsigned int * indices = m_Indices.ToPtr() + Mesh.FirstIndex;
    for ( int i = 0 ; i < Mesh.IndexCount ; i++ ) {
        f.WriteUInt32( *indices++ );
    }

    f.WriteUInt32( Mesh.VertexCount );
    SMeshVertex * verts = m_Vertices.ToPtr() + Mesh.BaseVertex;
    for ( int i = 0 ; i < Mesh.VertexCount ; i++ ) {
        verts->Write( f );
        verts++;
    }

    if ( bSkinnedMesh ) {
        f.WriteUInt32( Mesh.VertexCount ); // weights count

        SMeshVertexJoint * weights = m_Weights.ToPtr() + Mesh.BaseVertex;
        for ( int i = 0 ; i < Mesh.VertexCount ; i++ ) {
            weights->Write( f );
            weights++;
        }
    } else {
        f.WriteUInt32( 0 ); // weights count
    }
    f.WriteBool( bRaycastBVH ); // only for static meshes
    f.WriteUInt16( m_Settings.RaycastPrimitivesPerLeaf );
    f.WriteUInt32( 1 ); // subparts count
    if ( Mesh.Mesh->name ) {
        f.WriteString( Mesh.Mesh->name );
    } else {
        f.WriteString( "Subpart_1" );
    }
    f.WriteInt32( 0 ); // base vertex
    f.WriteUInt32( 0 ); // first index
    f.WriteUInt32( Mesh.VertexCount );
    f.WriteUInt32( Mesh.IndexCount );
    //f.WriteString( GetMaterialGUID( Mesh.Material ) );
    f.WriteObject( Mesh.BoundingBox );

    if ( bRaycastBVH ) {
        // Generate subpart BVH
        ATreeAABB * aabbTree = CreateInstanceOf< ATreeAABB >();
        aabbTree->Initialize( m_Vertices.ToPtr() + Mesh.BaseVertex,
                              m_Indices.ToPtr() + Mesh.FirstIndex,
                              Mesh.IndexCount,
                              0,
                              m_Settings.RaycastPrimitivesPerLeaf );

        // Write subpart BVH
        aabbTree->Write( f );
    }

    f.WriteUInt32( 0 ); // sockets count

    if ( bSkinnedMesh ) {
        //f.WriteString( m_SkeletonGUID.ToString() );
        f.WriteArrayInt32( m_Skin.JointIndices );
        f.WriteArrayOfStructs( m_Skin.OffsetMatrices );
    } else {
        //f.WriteString( "/Default/Skeleton/Default" );
    }

    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return;
    }

    f.Printf( "GUID \"%s\"\n", Mesh.GUID.CStr() );

    if ( bSkinnedMesh ) {
        f.Printf( "Skeleton \"%s\"\n", m_SkeletonGUID.ToString().CStr() );
    } else {
        f.Printf( "Skeleton \"%s\"\n", "/Default/Skeleton/Default" );
    }
    f.Printf( "Subparts [\n" );
    f.Printf( "\"%s\"\n", GetMaterialGUID( Mesh.Material ).CStr() );
    f.Printf( "]\n" );
}

bool AAssetImporter::ImportSkybox( SAssetImportSettings const & _Settings ) {
    AImage cubeFaces[6];

    m_Settings = _Settings;

    m_Settings.ImportFile = "Skybox";

    if ( !_Settings.bImportSkyboxExplicit ) {
        return false;
    }

    if ( _Settings.bSkyboxHDRI ) {
        for ( int i = 0 ; i < 6 ; i++ ) {
            if ( !cubeFaces[i].LoadHDRI( _Settings.ExplicitSkyboxFaces[i], false, false, 3 ) ) {
                return false;
            }
        }
        if ( _Settings.SkyboxHDRIScale != 1.0f || _Settings.SkyboxHDRIPow != 1.0f ) {
            for ( int i = 0 ; i < 6 ; i++ ) {
                float * HDRI = (float*)cubeFaces[i].pRawData;
                int count = cubeFaces[i].Width*cubeFaces[i].Height*3;
                for ( int j = 0; j < count ; j += 3 ) {
                    HDRI[j] = std::pow( HDRI[j + 0] * _Settings.SkyboxHDRIScale, _Settings.SkyboxHDRIPow );
                    HDRI[j + 1] = std::pow( HDRI[j + 1] * _Settings.SkyboxHDRIScale, _Settings.SkyboxHDRIPow );
                    HDRI[j + 2] = std::pow( HDRI[j + 2] * _Settings.SkyboxHDRIScale, _Settings.SkyboxHDRIPow );
                }
            }
        }
    } else {
        for ( int i = 0 ; i < 6 ; i++ ) {
            if ( !cubeFaces[i].LoadLDRI( _Settings.ExplicitSkyboxFaces[i], true, false, 3 ) ) {
                return false;
            }
        }
    }

    STexturePixelFormat texturePixelFormat;
    if ( !GetAppropriatePixelFormat( cubeFaces[0], texturePixelFormat ) ) {
        return false;
    }

    int width = cubeFaces[0].Width;
    for ( int i = 1 ; i < 6 ; i++ ) {

        // Check width
        if ( cubeFaces[i].Width != width
            || cubeFaces[i].Height != width ) {
            GLogger.Printf( "AAssetImporter::ImportSkybox: faces with different sizes\n" );
            return false;
        }

        // Check pixel format
        STexturePixelFormat facePF;
        if ( !GetAppropriatePixelFormat( cubeFaces[i], facePF ) ) {
            return false;
        }
        if ( texturePixelFormat != facePF ) {
            GLogger.Printf( "AAssetImporter::ImportSkybox: faces with different pixel formats\n" );
            return false;
        }
    }

    AFileStream f;
    AString fileName = GeneratePhysicalPath( "Texture" );
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileName ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return false;
    }

    AGUID TextureGUID;
    TextureGUID.Generate();

    GResourceManager.SetResourceGUID( TextureGUID, ("/Root/" + fileName).CStr() );

    uint32_t textureType = TEXTURE_CUBEMAP;
    uint32_t w = width, h = width, d = 6, numLods = 1;

    f.WriteUInt32( FMT_FILE_TYPE_TEXTURE );
    f.WriteUInt32( FMT_VERSION_TEXTURE );
    //f.WriteString( guid.CStr() );
    f.WriteUInt32( textureType );
    f.WriteObject( texturePixelFormat );
    f.WriteUInt32( w );
    f.WriteUInt32( h );
    f.WriteUInt32( d );
    f.WriteUInt32( numLods );

    int pixelByteLength = texturePixelFormat.SizeInBytesUncompressed();
    uint32_t lodWidth;

    int lod = 0;
    //for ( int lod = 0 ; lod < numLods ; lod++ ) {
    lodWidth = Math::Max( 1, width >> lod );

    f.WriteUInt32( lodWidth );
    f.WriteUInt32( lodWidth );
    f.WriteUInt32( 6 ); // lodDepth

    size_t size = lodWidth * lodWidth * pixelByteLength;

    for ( int face = 0 ; face < 6 ; face++ ) {
        f.WriteBuffer( cubeFaces[face].pRawData, size );
    }

    //f.WriteUInt32( 6 ); // num source files
    //for ( int i = 0 ; i < 6 ; i++ ) {
    //    f.WriteString( _Settings.ExplicitSkyboxFaces[i] ); // source file
    //}

    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return false;
    }

    f.Printf( "GUID \"%s\"\n", TextureGUID.CStr() );

    f.Printf( "Sources [\n" );
    for ( int i = 0 ; i < 6 ; i++ ) {
        f.Printf( "\"%s\"\n", _Settings.ExplicitSkyboxFaces[i] ); // source file
    }
    f.Printf( "]\n" );


    if ( m_Settings.bCreateSkyboxMaterialInstance ) {
        WriteSkyboxMaterial( TextureGUID );
    }

    return true;
}

void AAssetImporter::WriteSkyboxMaterial( AGUID const & SkyboxTextureGUID ) {
    AFileStream f;
    AString fileName = GeneratePhysicalPath( "MaterialInstance" );
    AString fileSystemPath = GResourceManager.GetRootPath() + fileName;

    if ( !f.OpenWrite( fileSystemPath ) ) {
        GLogger.Printf( "Failed to write %s\n", fileName.CStr() );
        return;
    }

    AGUID GUID;
    GUID.Generate();

    GResourceManager.SetResourceGUID( GUID, ("/Root/" + fileName).CStr() );

    f.WriteUInt32( FMT_FILE_TYPE_MATERIAL_INSTANCE );
    f.WriteUInt32( FMT_VERSION_MATERIAL_INSTANCE );
    //f.WriteString( guid.CStr() );
    //f.WriteString( "/Default/Materials/Skybox" ); // material GUID

    //f.WriteUInt32( 1 ); // textures count

    // texture
    //f.WriteString( SkyboxTextureGUID.CStr() );

    // Uniforms
//    for ( int i = 0 ; i < MAX_MATERIAL_UNIFORMS ; i++ ) {
//        f.WriteFloat( 0.0f );
//    }


    //
    // Write meta file
    //

    AString metaFilePath = fileSystemPath;

    metaFilePath.StripExt();
    metaFilePath += ".asset_meta";

    if ( !f.OpenWrite( metaFilePath ) ) {
        GLogger.Printf( "Failed to write %s meta\n", fileName.CStr() );
        return;
    }

    f.Printf( "GUID \"%s\"\n", GUID.CStr() );
    f.Printf( "Material \"%s\"\n", "/Default/Materials/Skybox" );
    f.Printf( "Textures [\n" );
    f.Printf( "\"%s\"\n", SkyboxTextureGUID.CStr() );
    f.Printf( "]\n" );
}
