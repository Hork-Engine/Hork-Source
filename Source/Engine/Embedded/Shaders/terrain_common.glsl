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

#define TILE_SIZE 256
#define TERRAIN_CLIP_BOUNDS
#define TERRAIN_LIGHT_MODEL_UNLIT 0
#define TERRAIN_LIGHT_MODEL_BASELIGHT 1
#define TERRAIN_LIGHT_MODEL_PBR 2
#define TERRAIN_LIGHT_MODEL TERRAIN_LIGHT_MODEL_PBR

#ifdef VERTEX_SHADER
out gl_PerVertex
{
    vec4 gl_Position;
#ifdef TERRAIN_CLIP_BOUNDS
    float gl_ClipDistance[];
#endif
};
#endif

layout( binding = 1, std140 ) uniform DrawCall
{
    mat4 LocalViewProjection;
    vec4 ModelNormalToViewSpace0;
    vec4 ModelNormalToViewSpace1;
    vec4 ModelNormalToViewSpace2;
    vec4 ViewPositionAndHeight;
    ivec4 TerrainClip;
};

#ifdef VERTEX_SHADER

float CalcTransitionAlpha( vec2 VertexPos, float VertexScale )
{
    const float gridsize = TILE_SIZE-2;
    const float transitionWidth = gridsize * 0.1;
    const vec2 alpha = clamp( (abs(VertexPos - ViewPositionAndHeight.xz) / VertexScale - (gridsize*0.5-transitionWidth))/(transitionWidth-1), vec2(0.0), vec2(1.0) );
    return max( alpha.x, alpha.y );
}

float CalcTransitionAlpha3D( vec3 VertexPos, float VertexScale )
{
    const float gridsize = TILE_SIZE-2;
    const float transitionWidth = gridsize * 0.1;
    #if 1
    const float gridExtent = gridsize * VertexScale;
    const float viewH = ViewPositionAndHeight.w;
    const vec2 t = (abs(VertexPos.xz - ViewPositionAndHeight.xz) / VertexScale - (gridsize*0.5-transitionWidth))/(transitionWidth-1);
    const float h = viewH * 2.5f / gridExtent * 2 - 1;
    const vec3 alpha = clamp( vec3( t, h ), vec3(0.0), vec3(1.0) );
    return max( max( alpha.x, alpha.y ), alpha.z );
    #else
    return CalcTransitionAlpha(VertexPos.xz, ViewPositionAndHeight.xz, VertexScale);
    #endif
}

#ifdef TERRAIN_CLIP_BOUNDS
void ClipTerrainBounds( vec3 Position )
{
    const vec4 plane0 = vec4( 1, 0, 0, TerrainClip.x );  // minsX
    const vec4 plane1 = vec4( 0, 0, 1, TerrainClip.y );  // minsZ
    const vec4 plane2 = vec4(-1, 0, 0, TerrainClip.z );  // maxsX    
    const vec4 plane3 = vec4( 0, 0,-1, TerrainClip.w );  // maxsZ
    vec4 p = vec4( Position, 1.0 );
    gl_ClipDistance[0] = dot( p, plane0 );
    gl_ClipDistance[1] = dot( p, plane1 );
    gl_ClipDistance[2] = dot( p, plane2 );
    gl_ClipDistance[3] = dot( p, plane3 );
}
#endif

#endif
