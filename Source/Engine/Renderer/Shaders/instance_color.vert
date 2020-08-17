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

// Built-in samplers
#include "$COLOR_PASS_VERTEX_SAMPLERS$"

// Built-in varyings
#include "$COLOR_PASS_VERTEX_OUTPUT_VARYINGS$"

#ifdef USE_LIGHTMAP
layout( location = COLOR_PASS_VARYING_BAKED_LIGHT ) out vec2 VS_LightmapTexCoord;
#endif

#ifdef USE_VERTEX_LIGHT
layout( location = COLOR_PASS_VARYING_BAKED_LIGHT ) out vec3 VS_VertexLight;
#endif

#if defined COMPUTE_TBN
layout( location = COLOR_PASS_VARYING_TANGENT  ) out vec3 VS_T;
layout( location = COLOR_PASS_VARYING_BINORMAL ) out vec3 VS_B;
#endif

#if defined COMPUTE_TBN || defined TESSELLATION_METHOD
layout( location = COLOR_PASS_VARYING_NORMAL   ) out vec3 VS_N;
layout( location = COLOR_PASS_VARYING_POSITION ) out vec3 VS_Position;
#endif

#ifdef USE_VIRTUAL_TEXTURE
layout( location = COLOR_PASS_VARYING_VT_TEXCOORD ) out vec2 VS_TexCoordVT;
#endif

#if defined( WITH_MOTION_BLUR ) && !defined( TRANSLUCENT ) && defined( ALLOW_MOTION_BLUR )
layout( location = COLOR_PASS_VARYING_VERTEX_POSITION_CURRENT ) out vec4 VS_VertexPos;
layout( location = COLOR_PASS_VARYING_VERTEX_POSITION_PREVIOUS ) out vec4 VS_VertexPosP;
#endif

#if defined SKINNED_MESH

    layout( binding = 2, std140 ) uniform JointTransforms
    {
        vec4 Transform[ 256 * 3 ];   // MAX_JOINTS = 256
    };
    
    #ifdef PER_BONE_MOTION_BLUR
    layout( binding = 7, std140 ) uniform JointTransformsP
    {
        vec4 TransformP[ 256 * 3 ];   // MAX_JOINTS = 256
    };
    #endif

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

    // Vertex in model space
    VertexPosition.x = dot( JointTransform0, SrcPosition );
    VertexPosition.y = dot( JointTransform1, SrcPosition );
    VertexPosition.z = dot( JointTransform2, SrcPosition );

    // Normal in model space
    VertexNormal.x = dot( vec3(JointTransform0), InNormal );
    VertexNormal.y = dot( vec3(JointTransform1), InNormal );
    VertexNormal.z = dot( vec3(JointTransform2), InNormal );
    
    #if defined COMPUTE_TBN || defined TESSELLATION_METHOD
    vec4 Normal = vec4( VertexNormal, 0.0 );
    // Transform normal from model space to viewspace
    VS_N.x = dot( ModelNormalToViewSpace0, Normal );
    VS_N.y = dot( ModelNormalToViewSpace1, Normal );
    VS_N.z = dot( ModelNormalToViewSpace2, Normal );
    VS_N = normalize( VS_N );    
    #endif
    
    #ifdef COMPUTE_TBN
    // Tangent in model space
    Normal.x = dot( vec3(JointTransform0), InTangent.xyz );
    Normal.y = dot( vec3(JointTransform1), InTangent.xyz );
    Normal.z = dot( vec3(JointTransform2), InTangent.xyz );

    // Transform tangent from model space to viewspace
    VS_T.x = dot( ModelNormalToViewSpace0, Normal );
    VS_T.y = dot( ModelNormalToViewSpace1, Normal );
    VS_T.z = dot( ModelNormalToViewSpace2, Normal );
    VS_T = normalize( VS_T );

    // Compute binormal in viewspace
    VS_B = normalize( cross( VS_N, VS_T ) ) * InTangent.w;
    #endif  // COMPUTE_TBN

    #if defined( WITH_MOTION_BLUR ) && !defined( TRANSLUCENT ) && defined( ALLOW_MOTION_BLUR )
        #ifdef PER_BONE_MOTION_BLUR
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
        #else
        vec4 VertexPositionP = VertexPosition;
        #endif
    #endif
    /////////////////////////////////////
    
#else

    VertexPosition = InPosition;
    VertexNormal = InNormal;
    
    #if defined COMPUTE_TBN || defined TESSELLATION_METHOD
    // Transform normal from model space to viewspace
    VS_N.x = dot( ModelNormalToViewSpace0, vec4( InNormal, 0.0 ) );
    VS_N.y = dot( ModelNormalToViewSpace1, vec4( InNormal, 0.0 ) );
    VS_N.z = dot( ModelNormalToViewSpace2, vec4( InNormal, 0.0 ) );
    VS_N = normalize( VS_N );
    #endif
    
    #ifdef COMPUTE_TBN
    // Transform tangent from model space to viewspace
    VS_T.x = dot( ModelNormalToViewSpace0, InTangent );
    VS_T.y = dot( ModelNormalToViewSpace1, InTangent );
    VS_T.z = dot( ModelNormalToViewSpace2, InTangent );
    VS_T = normalize( VS_T );

    // Compute binormal in viewspace
    VS_B = normalize( cross( VS_N, VS_T ) ) * InTangent.w;
    #endif // COMPUTE_TBN

#endif

#ifdef USE_LIGHTMAP
    VS_LightmapTexCoord = InLightmapTexCoord * LightmapOffset.zw + LightmapOffset.xy;
#endif

#ifdef USE_VERTEX_LIGHT
    
    // Decode RGBE
#if 0
    VS_VertexLight = vec3(InVertexLight.xyz) * ( InVertexLight[3] != 0 ? ldexp( 1.0, int(InVertexLight.w) - (128 + 8) ) : 0.0 );
#else
    vec4 c = vec4(InVertexLight) * (1.0/255.0); // normalize uvec4
    VS_VertexLight = c.rgb * exp2( (c.a*255.0)-128.0 );
#endif

#endif

#ifdef USE_VIRTUAL_TEXTURE
    VS_TexCoordVT = saturate( InTexCoord * VTScale + VTOffset );
#endif
 
    // Built-in material code
    #include "$COLOR_PASS_VERTEX_CODE$"
    
    vec4 TransformedPosition = TransformMatrix * FinalVertexPos;

#if defined( WITH_MOTION_BLUR ) && !defined( TRANSLUCENT ) && defined( ALLOW_MOTION_BLUR )
    VS_VertexPos = TransformedPosition;
    #ifdef SKINNED_MESH
        VS_VertexPosP = TransformMatrixP * VertexPositionP; // NOTE: We can't apply vertex deform to it!
    #else
        VS_VertexPosP = TransformMatrixP * FinalVertexPos;
    #endif
#endif

#if defined COMPUTE_TBN || defined TESSELLATION_METHOD
    // Position in view space
    VS_Position = vec3( InverseProjectionMatrix * TransformedPosition );
#endif

#ifndef TESSELLATION_METHOD
    gl_Position = TransformedPosition;
#   if defined WEAPON_DEPTH_HACK
        gl_Position.z += 0.1;
#   elif defined SKYBOX_DEPTH_HACK
        gl_Position.z = 0.0;
#   endif
#endif
}
