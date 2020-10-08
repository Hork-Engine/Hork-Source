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

#ifndef UNPACK_CLUSTER_H
#define UNPACK_CLUSTER_H

void UnpackCluster( out uint numProbes, out uint numDecals, out uint numLights, out uint firstIndex )
{
    // TODO: Move scale and bias to uniforms
    #define NUM_CLUSTERS_Z 24
    const float znear = 0.0125f;
    const float zfar = 512;
    const int nearOffset = 20;
    const float scale = ( NUM_CLUSTERS_Z + nearOffset ) / log2( zfar / znear );
    const float bias = -log2( znear ) * scale - nearOffset;

    // Calc cluster index
    const float linearDepth = -VS_Position.z;
    const float slice = max( 0.0, floor( log2( linearDepth ) * scale + bias ) );
    const ivec3 clusterIndex = ivec3( InNormalizedScreenCoord.x * 16, InNormalizedScreenCoord.y * 8, slice );

    // Fetch cluster header
    const uvec2 header = texelFetch( ClusterLookup, clusterIndex, 0 ).xy;

    // Unpack header data
    firstIndex = header.x;
    numProbes = header.y & 0xff;
    numDecals = ( header.y >> 8 ) & 0xff;
    numLights = ( header.y >> 16 ) & 0xff;
    //unused = ( header.y >> 24 ) & 0xff; // can be used in future
}

#endif

