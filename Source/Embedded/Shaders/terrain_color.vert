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

#include "base/viewuniforms.glsl"
#include "terrain_common.glsl"

layout( location = 0 ) out vec3 VS_N;
layout( location = 1 ) out vec3 VS_Position;
layout( location = 2 ) out vec3 VS_PositionWS;
layout( location = 3 ) out vec4 VS_Color;
layout( location = 4 ) out vec4 VS_UVAlpha;

layout( binding = 0 ) uniform sampler2DArray ClipmapArray;
layout( binding = 1 ) uniform sampler2DArray NormalMapArray;

vec2 CalcHeight( ivec2 uv )
{
    return texelFetch( ClipmapArray, ivec3( uv, VertexScaleAndTranslate.y ), 0 ).xy;
}

void CalcVertex( out ivec2 VertexOffset, out ivec2 TexelOffset )
{
    VertexOffset = InPosition * VertexScaleAndTranslate.x + VertexScaleAndTranslate.zw;

//#ifdef TERRAIN_CLIP_BOUNDS
//    VertexOffset = clamp( VertexOffset, TerrainClip.xy, TerrainClip.zw );
//#endif

    TexelOffset = (( VertexOffset + TexcoordOffset ) / VertexScaleAndTranslate.x) & ivec2(TILE_SIZE-1);

    //TexelOffset = InPosition + (VertexTranslate + TexcoordOffset) / VertexScaleAndTranslate.x
    //                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ prebake into one variable TexcoordOffset
}

void main()
{
    ivec2 vertexOffset, texelOffset;
    CalcVertex( vertexOffset, texelOffset );

    vec2 h = CalcHeight( texelOffset );
    float hf = h.x;
    float hc = h.y;
    
    vec3 position = vec3( vertexOffset.x, hf, vertexOffset.y );
    
    float alpha = CalcTransitionAlpha3D( position, VertexScaleAndTranslate.x );

    position.y = mix( hf, hc, alpha );
    
    gl_Position = LocalViewProjection * vec4( position, 1.0 );
    
    // Position in view space
    VS_Position = vec3( InverseProjectionMatrix * gl_Position );
    
    // Position in terrain space
    VS_PositionWS = position;
    
#ifdef TERRAIN_CLIP_BOUNDS
    ClipTerrainBounds( position );
#endif

    VS_Color = QuadColor * VertexScaleAndTranslate.x;
    
    VS_UVAlpha.xy = vec2((( vertexOffset + TexcoordOffset ) / VertexScaleAndTranslate.x)) * (1.0/(TILE_SIZE));
    //VS_UVAlpha.xy = vec2(texelOffset) * (1.0/(TILE_SIZE));
    VS_UVAlpha.z = float(VertexScaleAndTranslate.y);
    VS_UVAlpha.w = alpha;
    
#if 1
    vec4 packedNormal = texelFetch( NormalMapArray, ivec3( texelOffset, VertexScaleAndTranslate.y ), 0 ).bgra;    
    vec4 temp = packedNormal * 2.0 - 1.0;
    vec2 normal_xz = mix( temp.xy, temp.zw, alpha );
    vec4 n = vec4( vec3( normal_xz, sqrt( 1.0 - dot( normal_xz, normal_xz ) ) ).xzy, 0.0 );
    VS_N.x = dot( ModelNormalToViewSpace0, n );
    VS_N.y = dot( ModelNormalToViewSpace1, n );
    VS_N.z = dot( ModelNormalToViewSpace2, n );
    
    //nAvg = vec3( mix( packedNormal.xy, packedNormal.zw, alpha ), 1.0 );
    //nAvg = normalize( nAvg * 2.0 - 1.0 ).xzy;
    //n = vec4( nAvg, 0.0 );
    //VS_N2.x = dot( ModelNormalToViewSpace0, n );
    //VS_N2.y = dot( ModelNormalToViewSpace1, n );
    //VS_N2.z = dot( ModelNormalToViewSpace2, n );
#endif
}
