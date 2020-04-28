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

    layout( location = 0 ) out flat int VS_InstanceID;

#   ifdef SHADOW_MASKING
        layout( location = 1 ) out vec2 VS_TexCoord;
#   endif

    layout( binding = 3, std140 ) uniform ShadowMatrixBuffer {
        mat4 CascadeViewProjection[ MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES ];
        mat4 ShadowMapMatrices[ MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES ];
    };

#endif

#ifdef MATERIAL_PASS_NORMALS
   layout( location = 0 ) out vec4 VS_Normal;
#endif

#if defined SKINNED_MESH

    layout( binding = 2, std140 ) uniform JointTransforms
    {
        vec4 Transform[ 256 * 3 ];   // MAX_JOINTS = 256
    };

#endif // SKINNED_MESH

void main() {
#ifdef SKINNED_MESH
    const vec4 SrcPosition = vec4( InPosition, 1.0 );

    const vec4
    JointTransform0 = Transform[ InJointIndices[0] * 3 + 0 ] * InJointWeights[0]
                    + Transform[ InJointIndices[1] * 3 + 0 ] * InJointWeights[1]
                    + Transform[ InJointIndices[2] * 3 + 0 ] * InJointWeights[2]
                    + Transform[ InJointIndices[3] * 3 + 0 ] * InJointWeights[3];
    const vec4
    JointTransform1 = Transform[ InJointIndices[0] * 3 + 1 ] * InJointWeights[0]
                    + Transform[ InJointIndices[1] * 3 + 1 ] * InJointWeights[1]
                    + Transform[ InJointIndices[2] * 3 + 1 ] * InJointWeights[2]
                    + Transform[ InJointIndices[3] * 3 + 1 ] * InJointWeights[3];
    const vec4
    JointTransform2 = Transform[ InJointIndices[0] * 3 + 2 ] * InJointWeights[0]
                    + Transform[ InJointIndices[1] * 3 + 2 ] * InJointWeights[1]
                    + Transform[ InJointIndices[2] * 3 + 2 ] * InJointWeights[2]
                    + Transform[ InJointIndices[3] * 3 + 2 ] * InJointWeights[3];

    vec3 Position;
    Position.x = dot( JointTransform0, SrcPosition );
    Position.y = dot( JointTransform1, SrcPosition );
    Position.z = dot( JointTransform2, SrcPosition );
    #define GetVertexPosition() Position
#else
    #define GetVertexPosition() InPosition
#endif

    // Built-in material code (shadowmap)

#   ifdef MATERIAL_PASS_SHADOWMAP
#       include "$SHADOWMAP_PASS_VERTEX_CODE$"
        gl_Position = CascadeViewProjection[ gl_InstanceID ] * gl_Position;
        VS_InstanceID = gl_InstanceID;
#       ifdef SHADOW_MASKING
            VS_TexCoord = InTexCoord;
#       endif
#   endif

    // Built-in material code (depth)

#   ifdef MATERIAL_PASS_DEPTH
#       include "$DEPTH_PASS_VERTEX_CODE$"
#       if defined WEAPON_DEPTH_HACK
            gl_Position.z += 0.1;
#       elif defined SKYBOX_DEPTH_HACK
            gl_Position.z = 0.0;
#       endif
#   endif

    // Built-in material code (wireframe)

#   ifdef MATERIAL_PASS_WIREFRAME
#       include "$WIREFRAME_PASS_VERTEX_CODE$"
#   endif

    // Built-in material code (wireframe)

#   ifdef MATERIAL_PASS_NORMALS
#       include "$NORMALS_PASS_VERTEX_CODE$"

        const float MAGNITUDE = 0.04;
        
        mat4 modelMatrix = InverseProjectionMatrix * TransformMatrix;
        mat3 normalMatrix = mat3( transpose(inverse(modelMatrix)) );
        mat4 projectionMatrix = inverse(InverseProjectionMatrix); // TODO: add to view uniforms!
        
        vec3 n, v;
#ifdef SKINNED_MESH
        n.x = dot( vec3(JointTransform0), InNormal );
        n.y = dot( vec3(JointTransform1), InNormal );
        n.z = dot( vec3(JointTransform2), InNormal );
        n = normalMatrix * n;
#else
        n = normalMatrix * InNormal;
#endif      
        n = normalize( n ) * MAGNITUDE;
        
        v = vec3( modelMatrix * vec4( GetVertexPosition(), 1.0 ) );
        
        VS_Normal = projectionMatrix * vec4( v + n, 1.0 );

#   endif
}
