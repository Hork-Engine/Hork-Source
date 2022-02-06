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

#include "instance_shadowmap_uniforms.glsl"

out gl_PerVertex
{
    vec4 gl_Position;
};

layout( location = 0 ) out flat int VS_InstanceID;

#ifdef SHADOW_MASKING
    layout( location = 1 ) out vec2 VS_TexCoord;
#endif

#if defined SKINNED_MESH
    layout( binding = 2, std140 ) uniform JointTransforms
    {
        vec4 Transform[ 256 * 3 ];   // MAX_JOINTS = 256
    };
#endif

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

        vec4 Position;
        Position.x = dot( JointTransform0, SrcPosition );
        Position.y = dot( JointTransform1, SrcPosition );
        Position.z = dot( JointTransform2, SrcPosition );
        Position.w = 1;
        gl_Position = TransformMatrix * Position;
    #else
        gl_Position = TransformMatrix * vec4( InPosition, 1.0 );
    #endif

    VS_InstanceID = gl_InstanceID;

    #ifdef SHADOW_MASKING
        VS_TexCoord = InTexCoord;
    #endif
}
