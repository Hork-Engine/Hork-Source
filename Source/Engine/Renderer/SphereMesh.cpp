/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "SphereMesh.h"
#include "RenderLocal.h"

#include <Core/Public/PodVector.h>
#include <Core/Public/CoreMath.h>

using namespace RenderCore;

ASphereMesh::ASphereMesh( int _HDiv, int _VDiv )
{
    const int numVerts = _VDiv * (_HDiv - 1) + 2;
    const int numIndices = (_HDiv - 1) * (_VDiv - 1) * 6;
    int i, j;
    float a1, a2;

    AN_ASSERT_( numVerts < 65536, "Too many vertices" );

    TPodVector< Float3 > vertices;
    TPodVector< unsigned short > indices;

    vertices.Resize( numVerts );
    indices.Resize( numIndices );

    for ( i = 0, a1 = Math::_PI / _HDiv; i < (_HDiv - 1); i++ ) {
        float y, r;
        Math::SinCos( a1, r, y );
        for ( j = 0, a2 = 0; j < _VDiv; j++ ) {
            float s, c;
            Math::SinCos( a2, s, c );
            vertices[i*_VDiv + j] = Float3( r*c, -y, r*s );
            a2 += Math::_2PI / (_VDiv - 1);
        }
        a1 += Math::_PI / _HDiv;
    }
    vertices[(_HDiv - 1)*_VDiv + 0] = Float3( 0, -1, 0 );
    vertices[(_HDiv - 1)*_VDiv + 1] = Float3( 0, 1, 0 );

    // generate indices
    unsigned short *pIndices = indices.ToPtr();
    for ( i = 0; i < _HDiv; i++ ) {
        for ( j = 0; j < _VDiv - 1; j++ ) {
            unsigned short i2 = i + 1;
            unsigned short j2 = (j == _VDiv - 1) ? 0 : j + 1;
            if ( i == (_HDiv - 2) ) {
                *pIndices++ = (i*_VDiv + j2);
                *pIndices++ = (i*_VDiv + j);
                *pIndices++ = ((_HDiv - 1)*_VDiv + 1);

            }
            else if ( i == (_HDiv - 1) ) {
                *pIndices++ = (0 * _VDiv + j);
                *pIndices++ = (0 * _VDiv + j2);
                *pIndices++ = ((_HDiv - 1)*_VDiv + 0);

            }
            else {
                int quad[4] = { i*_VDiv + j, i*_VDiv + j2, i2*_VDiv + j2, i2*_VDiv + j };
                *pIndices++ = quad[3];
                *pIndices++ = quad[2];
                *pIndices++ = quad[1];
                *pIndices++ = quad[1];
                *pIndices++ = quad[0];
                *pIndices++ = quad[3];
            }
        }
    }

    SBufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;

    bufferCI.SizeInBytes = sizeof( Float3 ) * vertices.Size();
    GDevice->CreateBuffer( bufferCI, vertices.ToPtr(), &VertexBuffer );

    VertexBuffer->SetDebugName( "Sphere mesh vertex buffer" );

    bufferCI.SizeInBytes = sizeof( unsigned short ) * indices.Size();
    GDevice->CreateBuffer( bufferCI, indices.ToPtr(), &IndexBuffer );

    VertexBuffer->SetDebugName( "Sphere mesh index buffer" );

    IndexCount = indices.Size();
}
