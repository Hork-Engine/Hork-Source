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

#include "base/viewuniforms.glsl"
#include "terrain_common.glsl"

layout( binding = 0 ) uniform sampler2DArray ClipmapArray;

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
    //TexelOffset = InPosition + (VertexScaleAndTranslate.zw + TexcoordOffset) / VertexScaleAndTranslate.x
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
    
    //float alpha = CalcTransitionAlpha( position.xz, VertexScaleAndTranslate.x );
    float alpha = CalcTransitionAlpha3D( position, VertexScaleAndTranslate.x );
    
    position.y = mix( hf, hc, alpha );

#ifdef TERRAIN_CLIP_BOUNDS
    ClipTerrainBounds( position );
#endif

    gl_Position = LocalViewProjection * vec4( position, 1.0 );
}
