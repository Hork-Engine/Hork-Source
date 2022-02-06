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


out gl_PerVertex
{
    vec4 gl_Position;
};

// Built-in samplers
#include "$VXGI_VOXELIZE_PASS_VERTEX_SAMPLERS$"

// Built-in varyings
#include "$VXGI_VOXELIZE_PASS_VERTEX_OUTPUT_VARYINGS$"

#if defined SKINNED_MESH

    layout( binding = 2, std140 ) uniform JointTransforms
    {
        vec4 Transform[ 256 * 3 ];   // MAX_JOINTS = 256
    };
    
#endif // SKINNED_MESH

void main() {
    vec3 VertexPosition;
    vec3 VertexNormal;
    
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

    // Vertex in model space
    VertexPosition.x = dot( JointTransform0, SrcPosition );
    VertexPosition.y = dot( JointTransform1, SrcPosition );
    VertexPosition.z = dot( JointTransform2, SrcPosition );

    // Normal in model space
    VertexNormal.x = dot( vec3(JointTransform0), InNormal );
    VertexNormal.y = dot( vec3(JointTransform1), InNormal );
    VertexNormal.z = dot( vec3(JointTransform2), InNormal );
    
#else

    VertexPosition = InPosition;
    VertexNormal = InNormal;
    
#endif

    // Built-in material code
    #include "$VXGI_VOXELIZE_PASS_VERTEX_CODE$"
    
    vec4 TransformedPosition = TransformMatrix * FinalVertexPos;

    gl_Position = TransformedPosition;
//#   if defined WEAPON_DEPTH_HACK
//        gl_Position.z += 0.1;
//#   elif defined SKYBOX_DEPTH_HACK
//        gl_Position.z = 0.0;
//#   endif
}
