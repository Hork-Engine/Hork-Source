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

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

#include "$VXGI_VOXELIZE_PASS_GEOMETRY_INPUT_VARYINGS$"
#include "$VXGI_VOXELIZE_PASS_GEOMETRY_OUTPUT_VARYINGS$"

layout( location = VXGI_VOXELIZE_PASS_GEOMETRY_OUTPUT_VARYINGS_OFFSET ) flat out uint GS_DomInd;
layout( location = VXGI_VOXELIZE_PASS_GEOMETRY_OUTPUT_VARYINGS_OFFSET+1 ) out vec3 GS_Normal;
//layout( location = VXGI_VOXELIZE_PASS_GEOMETRY_OUTPUT_VARYINGS_OFFSET+2 ) out vec4 GS_ShadowCoord;

layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;

struct SceneParams {
	mat4 MTOmatrix[3];
	mat4 MTWmatrix;
	mat4 MTShadowMatrix;
	vec3 lightDir;
	uint voxelDraw;
	uint view;
	uint voxelRes;
	uint voxelLayer;
	uint numMipLevels;
	uint mipLevel;
};

layout (std140, binding = 1) uniform SceneBuffer { // FIXME: Binding
	SceneParams scene;
};

vec3 CalculateNormal(vec3 a, vec3 b, vec3 c) {
	vec3 ab = b - a;
	vec3 ac = c - a;
	return normalize(cross(ab,ac));
}

void main() {
	vec3 dir = CalculateNormal(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
	GS_DomInd = dir;
	dir = abs(dir);
	float maxComponent = max(dir.x, max(dir.y, dir.z));
	uint ind = maxComponent == dir.x ? 0 : maxComponent == dir.y ? 1 : 2;
	GS_DomInd = ind;

	gl_Position = scene.MTOmatrix[ind] * gl_in[0].gl_Position;
	//shadowCoord = scene.MTShadowMatrix * scene.MTOmatrix[2] * gl_in[0].gl_Position;
#   include "$VXGI_VOXELIZE_PASS_GEOMETRY_COPY_VARYINGS$"
	EmitVertex();
	
	gl_Position = scene.MTOmatrix[ind] * gl_in[1].gl_Position;
	//shadowCoord = scene.MTShadowMatrix * scene.MTOmatrix[2] * gl_in[1].gl_Position;
#   include "$VXGI_VOXELIZE_PASS_GEOMETRY_COPY_VARYINGS$"
	EmitVertex();

	gl_Position = scene.MTOmatrix[ind] * gl_in[2].gl_Position;
	//shadowCoord = scene.MTShadowMatrix * scene.MTOmatrix[2] * gl_in[2].gl_Position;
#   include "$VXGI_VOXELIZE_PASS_GEOMETRY_COPY_VARYINGS$"
	EmitVertex();

	EndPrimitive();
}
