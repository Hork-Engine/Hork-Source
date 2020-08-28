/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

out gl_PerVertex
{
    vec4 gl_Position;
};

#if defined MATERIAL_PASS_SHADOWMAP

#   include "$SHADOWMAP_PASS_VERTEX_OUTPUT_VARYINGS$"
#   include "$SHADOWMAP_PASS_VERTEX_SAMPLERS$"

#   if defined TESSELLATION_METHOD && defined DISPLACEMENT_AFFECT_SHADOW
        layout( location = SHADOWMAP_PASS_VARYING_POSITION ) out vec3 VS_Position;
        layout( location = SHADOWMAP_PASS_VARYING_NORMAL ) out vec3 VS_N;
#   endif

//    layout( location = SHADOWMAP_PASS_VARYING_INSTANCE_ID ) out flat int VS_InstanceID;
#endif

#ifdef MATERIAL_PASS_DEPTH
#   include "$DEPTH_PASS_VERTEX_OUTPUT_VARYINGS$"
#   include "$DEPTH_PASS_VERTEX_SAMPLERS$"
#   ifdef TESSELLATION_METHOD
        layout( location = DEPTH_PASS_VARYING_POSITION ) out vec3 VS_Position;
        layout( location = DEPTH_PASS_VARYING_NORMAL ) out vec3 VS_N;
#   endif
#   if defined( DEPTH_WITH_VELOCITY_MAP )
        layout( location = DEPTH_PASS_VARYING_VERTEX_POSITION_CURRENT ) out vec4 VS_VertexPos;
        layout( location = DEPTH_PASS_VARYING_VERTEX_POSITION_PREVIOUS ) out vec4 VS_VertexPosP;
#   endif
#endif

#ifdef MATERIAL_PASS_WIREFRAME
#   include "$WIREFRAME_PASS_VERTEX_OUTPUT_VARYINGS$"
#   include "$WIREFRAME_PASS_VERTEX_SAMPLERS$"
#   ifdef TESSELLATION_METHOD
        layout( location = WIREFRAME_PASS_VARYING_POSITION ) out vec3 VS_Position;
        layout( location = WIREFRAME_PASS_VARYING_NORMAL ) out vec3 VS_N;
#   endif
#endif

#ifdef MATERIAL_PASS_NORMALS
#   include "$NORMALS_PASS_VERTEX_SAMPLERS$"
    layout( location = 0 ) out vec4 VS_Normal;
#endif

#ifdef MATERIAL_PASS_FEEDBACK
    layout( location = 0 ) out vec2 VS_TexCoordVT;
#endif

#if defined SKINNED_MESH

    layout( binding = 2, std140 ) uniform JointTransforms
    {
        vec4 Transform[ 256 * 3 ];   // MAX_JOINTS = 256
    };

#   if defined( DEPTH_WITH_VELOCITY_MAP ) && defined( PER_BONE_MOTION_BLUR )
    layout( binding = 7, std140 ) uniform JointTransformsP
    {
        vec4 TransformP[ 256 * 3 ];   // MAX_JOINTS = 256
    };
#   endif

#endif // SKINNED_MESH

void main() {
    vec3 VertexPosition;
    vec3 VertexNormal;
    
#ifdef SKINNED_MESH
    const vec4 SrcPosition = vec4( InPosition, 1.0 );

    vec4
    JointTransform0 = Transform[ InJointIndices[0] * 3 + 0 ] * InJointWeights[0]
                    + Transform[ InJointIndices[1] * 3 + 0 ] * InJointWeights[1]
                    + Transform[ InJointIndices[2] * 3 + 0 ] * InJointWeights[2]
                    + Transform[ InJointIndices[3] * 3 + 0 ] * InJointWeights[3];
    vec4
    JointTransform1 = Transform[ InJointIndices[0] * 3 + 1 ] * InJointWeights[0]
                    + Transform[ InJointIndices[1] * 3 + 1 ] * InJointWeights[1]
                    + Transform[ InJointIndices[2] * 3 + 1 ] * InJointWeights[2]
                    + Transform[ InJointIndices[3] * 3 + 1 ] * InJointWeights[3];
    vec4
    JointTransform2 = Transform[ InJointIndices[0] * 3 + 2 ] * InJointWeights[0]
                    + Transform[ InJointIndices[1] * 3 + 2 ] * InJointWeights[1]
                    + Transform[ InJointIndices[2] * 3 + 2 ] * InJointWeights[2]
                    + Transform[ InJointIndices[3] * 3 + 2 ] * InJointWeights[3];
                    
    VertexPosition.x = dot( JointTransform0, SrcPosition );
    VertexPosition.y = dot( JointTransform1, SrcPosition );
    VertexPosition.z = dot( JointTransform2, SrcPosition );
    
    VertexNormal.x = dot( vec3(JointTransform0), InNormal );
    VertexNormal.y = dot( vec3(JointTransform1), InNormal );
    VertexNormal.z = dot( vec3(JointTransform2), InNormal );
#else
    VertexPosition = InPosition;
    VertexNormal = InNormal;
#endif

    // Built-in material code (shadowmap)

#   ifdef MATERIAL_PASS_SHADOWMAP
#       include "$SHADOWMAP_PASS_VERTEX_CODE$"
#       if defined TESSELLATION_METHOD && defined DISPLACEMENT_AFFECT_SHADOW
            // Position in world space
            VS_Position = vec3( TransformMatrix * FinalVertexPos );
            
            mat3 NormalMatrix = mat3( transpose(inverse(TransformMatrix)) ); // TODO: Uniform?
            
            // Transform normal from model space to world space
            VS_N = normalize( NormalMatrix * VertexNormal );
#       else
            gl_Position = TransformMatrix * FinalVertexPos;
//            gl_Position = CascadeViewProjection[ gl_InstanceID ] * gl_Position;
#       endif
        //VS_InstanceID = gl_InstanceID;
#   endif

    // Built-in material code (depth)

#   ifdef MATERIAL_PASS_DEPTH

#       if defined ( SKINNED_MESH )
#           if defined( DEPTH_WITH_VELOCITY_MAP )
#               if defined( PER_BONE_MOTION_BLUR )
                    JointTransform0 = TransformP[ InJointIndices[0] * 3 + 0 ] * InJointWeights[0]
                            + TransformP[ InJointIndices[1] * 3 + 0 ] * InJointWeights[1]
                            + TransformP[ InJointIndices[2] * 3 + 0 ] * InJointWeights[2]
                            + TransformP[ InJointIndices[3] * 3 + 0 ] * InJointWeights[3];
                    JointTransform1 = TransformP[ InJointIndices[0] * 3 + 1 ] * InJointWeights[0]
                            + TransformP[ InJointIndices[1] * 3 + 1 ] * InJointWeights[1]
                            + TransformP[ InJointIndices[2] * 3 + 1 ] * InJointWeights[2]
                            + TransformP[ InJointIndices[3] * 3 + 1 ] * InJointWeights[3];
                    JointTransform2 = TransformP[ InJointIndices[0] * 3 + 2 ] * InJointWeights[0]
                            + TransformP[ InJointIndices[1] * 3 + 2 ] * InJointWeights[1]
                            + TransformP[ InJointIndices[2] * 3 + 2 ] * InJointWeights[2]
                            + TransformP[ InJointIndices[3] * 3 + 2 ] * InJointWeights[3];

                    vec4 VertexPositionP;
                    VertexPositionP.x = dot( JointTransform0, SrcPosition );
                    VertexPositionP.y = dot( JointTransform1, SrcPosition );
                    VertexPositionP.z = dot( JointTransform2, SrcPosition );
                    VertexPositionP.w = 1.0;
#               else
                    vec4 VertexPositionP = VertexPosition;
#               endif
#           endif
#       endif

#       include "$DEPTH_PASS_VERTEX_CODE$"

        vec4 TransformedPosition = TransformMatrix * FinalVertexPos;

#       if defined( DEPTH_WITH_VELOCITY_MAP )
            VS_VertexPos = TransformedPosition;
#           ifdef SKINNED_MESH
                VS_VertexPosP = TransformMatrixP * VertexPositionP; // NOTE: We can't apply vertex deform to it!
#           else
                VS_VertexPosP = TransformMatrixP * FinalVertexPos;
#           endif
#       endif

#       ifdef TESSELLATION_METHOD
            // Position in view space
            VS_Position = vec3( InverseProjectionMatrix * TransformedPosition );
            
            // Transform normal from model space to viewspace
            VS_N.x = dot( ModelNormalToViewSpace0, vec4( VertexNormal, 0.0 ) );
            VS_N.y = dot( ModelNormalToViewSpace1, vec4( VertexNormal, 0.0 ) );
            VS_N.z = dot( ModelNormalToViewSpace2, vec4( VertexNormal, 0.0 ) );
            VS_N = normalize( VS_N );
#       else
            gl_Position = TransformedPosition;
#           if defined WEAPON_DEPTH_HACK
                gl_Position.z += 0.1;
#           elif defined SKYBOX_DEPTH_HACK
                gl_Position.z = 0.0;
#           endif
#       endif
#   endif

    // Built-in material code (wireframe)

#   ifdef MATERIAL_PASS_WIREFRAME
#       include "$WIREFRAME_PASS_VERTEX_CODE$"
#       ifdef TESSELLATION_METHOD
            // Position in view space
            VS_Position = vec3( InverseProjectionMatrix * ( TransformMatrix * FinalVertexPos ) );
            
            // Transform normal from model space to viewspace
            VS_N.x = dot( ModelNormalToViewSpace0, vec4( VertexNormal, 0.0 ) );
            VS_N.y = dot( ModelNormalToViewSpace1, vec4( VertexNormal, 0.0 ) );
            VS_N.z = dot( ModelNormalToViewSpace2, vec4( VertexNormal, 0.0 ) );
            VS_N = normalize( VS_N );
            
            // Position in model space. Use this if subdivision in screen space
            //VS_Position = FinalVertexPos.xyz;
#       else
            gl_Position = TransformMatrix * FinalVertexPos;
#       endif
#   endif

    // Built-in material code (wireframe)

#   ifdef MATERIAL_PASS_NORMALS
#       include "$NORMALS_PASS_VERTEX_CODE$"
        gl_Position = TransformMatrix * FinalVertexPos;

        const float MAGNITUDE = 0.04;
        
        mat4 modelMatrix = InverseProjectionMatrix * TransformMatrix;
        mat3 normalMatrix = mat3( transpose(inverse(modelMatrix)) );
        
        vec3 n = normalMatrix * VertexNormal;
        n = normalize( n ) * MAGNITUDE;
        
        vec3 v = vec3( modelMatrix * vec4( VertexPosition, 1.0 ) ); // FIXME: this is not correct for materials with vertex deforms
        
        VS_Normal = ProjectionMatrix * vec4( v + n, 1.0 );

#   endif

    // Built-in material code (feedback)

#   ifdef MATERIAL_PASS_FEEDBACK
        gl_Position = TransformMatrix * vec4( VertexPosition, 1.0 ); // FIXME: this is not correct for materials with vertex deforms
        VS_TexCoordVT = saturate(InTexCoord * VTScale + VTOffset);
#       if defined WEAPON_DEPTH_HACK
            gl_Position.z += 0.1;
#       elif defined SKYBOX_DEPTH_HACK
            gl_Position.z = 0.0;
#       endif
#   endif
}
