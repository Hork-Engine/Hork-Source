/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#pragma once

#include <Engine/Runtime/Public/RenderBackend.h>
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

// binormal = cross( normal, tangent ) * handedness
AN_FORCEINLINE float CalcHandedness( Float3 const & _Tangent, Float3 const & _Binormal, Float3 const & _Normal ) {
    return ( _Normal.Cross( _Tangent ).Dot( _Binormal ) < 0.0f ) ? -1.0f : 1.0f;
}

AN_FORCEINLINE Float3 CalcBinormal( Float3 const & _Tangent, Float3 const & _Normal, float _Handedness ) {
    return _Normal.Cross( _Tangent ).Normalized() * _Handedness;
}

AN_FORCEINLINE void CalcTangentSpace( FMeshVertex * _VertexArray, unsigned int _NumVerts,
                                      unsigned int const * _IndexArray, unsigned int _NumIndices ) {
    Float3 binormal, tangent;

    TPodArray< Float3 > Binormals;
    Binormals.ResizeInvalidate( _NumVerts );

    for ( int i = 0 ; i < _NumVerts ; i++ ) {
        _VertexArray[ i ].Tangent = Float3( 0 );
        Binormals[ i ] = Float3( 0 );
    }

    for ( unsigned int i = 0 ; i < _NumIndices ; i += 3 ) {
        unsigned int a = _IndexArray[ i ];
        unsigned int b = _IndexArray[ i + 1 ];
        unsigned int c = _IndexArray[ i + 2 ];

        Float3 e1 = _VertexArray[ b ].Position - _VertexArray[ a ].Position;
        Float3 e2 = _VertexArray[ c ].Position - _VertexArray[ a ].Position;
        Float2 et1 = _VertexArray[ b ].TexCoord - _VertexArray[ a ].TexCoord;
        Float2 et2 = _VertexArray[ c ].TexCoord - _VertexArray[ a ].TexCoord;

        float denom = et1.X*et2.Y - et1.Y*et2.X;
        float scale = ( fabsf( denom ) < 0.0001f ) ? 1.0f : ( 1.0 / denom );
        tangent = ( e1 * et2.Y - e2 * et1.Y ) * scale;
        binormal = ( e2 * et1.X - e1 * et2.X ) * scale;

        _VertexArray[ a ].Tangent += tangent;
        _VertexArray[ b ].Tangent += tangent;
        _VertexArray[ c ].Tangent += tangent;

        Binormals[ a ] += binormal;
        Binormals[ b ] += binormal;
        Binormals[ c ] += binormal;
    }

    for ( int i = 0 ; i < _NumVerts ; i++ ) {
        const Float3 & n = _VertexArray[ i ].Normal;
        const Float3 & t = _VertexArray[ i ].Tangent;

        _VertexArray[ i ].Tangent = ( t - n * FMath::Dot( n, t ) ).Normalized();

        _VertexArray[ i ].Handedness = CalcHandedness( t, Binormals[ i ].Normalized(), n );
    }
}

struct FBoxShape {

    static void CreateMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, const Float3 & _Size, float _TexCoordScale ) {
        const unsigned int indices[ 6 * 6 ] =
        {   0,1,2,2,3,0, // front face
            4,5,6,6,7,4, // back face

            5 + 8 * 1,0 + 8 * 1,3 + 8 * 1,3 + 8 * 1,6 + 8 * 1,5 + 8 * 1, // left face
            1 + 8 * 1,4 + 8 * 1,7 + 8 * 1,7 + 8 * 1,2 + 8 * 1,1 + 8 * 1, // right face

            3 + 8 * 2,2 + 8 * 2,7 + 8 * 2,7 + 8 * 2,6 + 8 * 2,3 + 8 * 2, // top face
            1 + 8 * 2,0 + 8 * 2,5 + 8 * 2,5 + 8 * 2,4 + 8 * 2,1 + 8 * 2, // bottom face
        };

        _Vertices.ResizeInvalidate( 24 );
        _Indices.ResizeInvalidate( 36 );

        memcpy( _Indices.ToPtr(), indices, sizeof( indices ) );

        Float3 HalfSize = _Size * 0.5f;

        _Bounds.Mins = -HalfSize;
        _Bounds.Maxs = HalfSize;

        const Float3 & Mins = _Bounds.Mins;
        const Float3 & Maxs = _Bounds.Maxs;

        FMeshVertex * pVerts = _Vertices.ToPtr();

        pVerts[ 0 + 8 * 0 ].Position = Float3( Mins.X, Mins.Y, Maxs.Z ); // 0
        pVerts[ 0 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
        pVerts[ 0 + 8 * 0 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

        pVerts[ 1 + 8 * 0 ].Position = Float3( Maxs.X, Mins.Y, Maxs.Z ); // 1
        pVerts[ 1 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
        pVerts[ 1 + 8 * 0 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

        pVerts[ 2 + 8 * 0 ].Position = Float3( Maxs.X, Maxs.Y, Maxs.Z ); // 2
        pVerts[ 2 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
        pVerts[ 2 + 8 * 0 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

        pVerts[ 3 + 8 * 0 ].Position = Float3( Mins.X, Maxs.Y, Maxs.Z ); // 3
        pVerts[ 3 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
        pVerts[ 3 + 8 * 0 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;


        pVerts[ 4 + 8 * 0 ].Position = Float3( Maxs.X, Mins.Y, Mins.Z ); // 4
        pVerts[ 4 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
        pVerts[ 4 + 8 * 0 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

        pVerts[ 5 + 8 * 0 ].Position = Float3( Mins.X, Mins.Y, Mins.Z ); // 5
        pVerts[ 5 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
        pVerts[ 5 + 8 * 0 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

        pVerts[ 6 + 8 * 0 ].Position = Float3( Mins.X, Maxs.Y, Mins.Z ); // 6
        pVerts[ 6 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
        pVerts[ 6 + 8 * 0 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

        pVerts[ 7 + 8 * 0 ].Position = Float3( Maxs.X, Maxs.Y, Mins.Z ); // 7
        pVerts[ 7 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
        pVerts[ 7 + 8 * 0 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;


        pVerts[ 0 + 8 * 1 ].Position = Float3( Mins.X, Mins.Y, Maxs.Z ); // 0
        pVerts[ 0 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
        pVerts[ 0 + 8 * 1 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

        pVerts[ 1 + 8 * 1 ].Position = Float3( Maxs.X, Mins.Y, Maxs.Z ); // 1
        pVerts[ 1 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
        pVerts[ 1 + 8 * 1 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

        pVerts[ 2 + 8 * 1 ].Position = Float3( Maxs.X, Maxs.Y, Maxs.Z ); // 2
        pVerts[ 2 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
        pVerts[ 2 + 8 * 1 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

        pVerts[ 3 + 8 * 1 ].Position = Float3( Mins.X, Maxs.Y, Maxs.Z ); // 3
        pVerts[ 3 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
        pVerts[ 3 + 8 * 1 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;


        pVerts[ 4 + 8 * 1 ].Position = Float3( Maxs.X, Mins.Y, Mins.Z ); // 4
        pVerts[ 4 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
        pVerts[ 4 + 8 * 1 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

        pVerts[ 5 + 8 * 1 ].Position = Float3( Mins.X, Mins.Y, Mins.Z ); // 5
        pVerts[ 5 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
        pVerts[ 5 + 8 * 1 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

        pVerts[ 6 + 8 * 1 ].Position = Float3( Mins.X, Maxs.Y, Mins.Z ); // 6
        pVerts[ 6 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
        pVerts[ 6 + 8 * 1 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

        pVerts[ 7 + 8 * 1 ].Position = Float3( Maxs.X, Maxs.Y, Mins.Z ); // 7
        pVerts[ 7 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
        pVerts[ 7 + 8 * 1 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;


        pVerts[ 1 + 8 * 2 ].Position = Float3( Maxs.X, Mins.Y, Maxs.Z ); // 1
        pVerts[ 1 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
        pVerts[ 1 + 8 * 2 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

        pVerts[ 0 + 8 * 2 ].Position = Float3( Mins.X, Mins.Y, Maxs.Z ); // 0
        pVerts[ 0 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
        pVerts[ 0 + 8 * 2 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

        pVerts[ 5 + 8 * 2 ].Position = Float3( Mins.X, Mins.Y, Mins.Z ); // 5
        pVerts[ 5 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
        pVerts[ 5 + 8 * 2 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

        pVerts[ 4 + 8 * 2 ].Position = Float3( Maxs.X, Mins.Y, Mins.Z ); // 4
        pVerts[ 4 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
        pVerts[ 4 + 8 * 2 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;


        pVerts[ 3 + 8 * 2 ].Position = Float3( Mins.X, Maxs.Y, Maxs.Z ); // 3
        pVerts[ 3 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
        pVerts[ 3 + 8 * 2 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

        pVerts[ 2 + 8 * 2 ].Position = Float3( Maxs.X, Maxs.Y, Maxs.Z ); // 2
        pVerts[ 2 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
        pVerts[ 2 + 8 * 2 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

        pVerts[ 7 + 8 * 2 ].Position = Float3( Maxs.X, Maxs.Y, Mins.Z ); // 7
        pVerts[ 7 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
        pVerts[ 7 + 8 * 2 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

        pVerts[ 6 + 8 * 2 ].Position = Float3( Mins.X, Maxs.Y, Mins.Z ); // 6
        pVerts[ 6 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
        pVerts[ 6 + 8 * 2 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

        CalcTangentSpace( pVerts, _Vertices.Length(), _Indices.ToPtr(), _Indices.Length() );
    }
};

struct FSphereShape {
    static void CreateMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _HDiv, int _VDiv ) {
        int x, y;
        float VerticalAngle, HorizontalAngle;
        const float RadWidth = _Radius;
        const float RadHeight = _Radius;
        unsigned int Quad[ 4 ];

        _Vertices.ResizeInvalidate( ( _VDiv + 1 )*( _HDiv + 1 ) );
        _Indices.ResizeInvalidate( _VDiv * _HDiv * 6 );

        _Bounds.Mins.X = _Bounds.Mins.Y = _Bounds.Mins.Z = -_Radius;
        _Bounds.Maxs.X = _Bounds.Maxs.Y = _Bounds.Maxs.Z = _Radius;

        FMeshVertex * pVerts = _Vertices.ToPtr();
        FMeshVertex * pVert;

        const float VerticalStep = FMath::_PI / _HDiv;
        const float HorizontalStep = FMath::_2PI / _VDiv;
        const float VerticalScale = 1.0f / _VDiv;
        const float HorizontalScale = 1.0f / _HDiv;

        for ( y = 0, VerticalAngle = -FMath::_HALF_PI ; y <= _HDiv ; y++ ) {
            float h, r;
            FMath::RadSinCos( VerticalAngle, h, r );
            h *= RadHeight;
            r *= RadWidth;
            for ( x = 0, HorizontalAngle = 0 ; x <= _VDiv ; x++ ) {
                float s, c;
                FMath::RadSinCos( HorizontalAngle, s, c );
                pVert = pVerts + ( y*(_VDiv+1) + x );
                pVert->Position = Float3( r*c, h, r*s );
                pVert->TexCoord = Float2( 1.0f - static_cast< float >( x ) * VerticalScale, 1.0f - static_cast< float >( y ) * HorizontalScale ) * _TexCoordScale;
                pVert->Normal = pVert->Position / _Radius;
                HorizontalAngle += HorizontalStep;
            }
            VerticalAngle += VerticalStep;
        }

        unsigned int * pIndices = _Indices.ToPtr();
        for ( y = 0 ; y < _HDiv ; y++ ) {
            int y2 = y + 1;
            for ( x = 0 ; x < _VDiv ; x++ ) {
                int x2 = x + 1;

                Quad[ 0 ] = y  * (_VDiv+1) + x;
                Quad[ 1 ] = y2 * (_VDiv+1) + x;
                Quad[ 2 ] = y2 * (_VDiv+1) + x2;
                Quad[ 3 ] = y  * (_VDiv+1) + x2;
                
                *pIndices++ = Quad[ 0 ];
                *pIndices++ = Quad[ 1 ];
                *pIndices++ = Quad[ 2 ];
                *pIndices++ = Quad[ 2 ];
                *pIndices++ = Quad[ 3 ];
                *pIndices++ = Quad[ 0 ];
            }
        }

        CalcTangentSpace( pVerts, _Vertices.Length(), _Indices.ToPtr(), _Indices.Length() );
    }

};

struct FPlaneShape {
    static void CreateMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale ) {
        _Vertices.ResizeInvalidate( 4 );
        _Indices.ResizeInvalidate( 6 );

        float HalfWidth = _Width * 0.5f;
        float HalfHeight = _Height * 0.5f;

        const FMeshVertex Verts[ 4 ] = {
            { Float3( -HalfWidth,0,-HalfHeight ), Float2( 0,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
            { Float3( -HalfWidth,0,HalfHeight ), Float2( 0,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
            { Float3( HalfWidth,0,HalfHeight ), Float2( _TexCoordScale,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
            { Float3( HalfWidth,0,-HalfHeight ), Float2( _TexCoordScale,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) }
        };

        memcpy( _Vertices.ToPtr(), &Verts, 4 * sizeof( FMeshVertex ) );

        const unsigned int Indices[ 6 ] = { 0,1,2,2,3,0 };
        memcpy( _Indices.ToPtr(), &Indices, sizeof( Indices ) );

        CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Length(), _Indices.ToPtr(), _Indices.Length() );

        _Bounds.Mins.X = -HalfWidth;
        _Bounds.Mins.Y = 0.0f;
        _Bounds.Mins.Z = -HalfHeight;
        _Bounds.Maxs.X = HalfWidth;
        _Bounds.Maxs.Y = 0.0f;
        _Bounds.Maxs.Z = HalfHeight;
    }
};

struct FPatchShape {
    static void CreateMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds,
        Float3 const & Corner00,
        Float3 const & Corner10,
        Float3 const & Corner01,
        Float3 const & Corner11,
        int resx, int resy,
        float _TexCoordScale,
        bool _TwoSided ) {

        if ( resx < 2 ) {
            resx = 2;
        }

        if ( resy < 2 ) {
            resy = 2;
        }

        const int vertexCount = resx * resy;
        const int indexCount = ( resx - 1 ) * ( resy - 1 ) * 6;

        Float3 normal = ( Corner10 - Corner00 ).Cross( Corner01 - Corner00 ).Normalized();

        _Vertices.ResizeInvalidate( _TwoSided ? vertexCount<<1 : vertexCount );
        _Indices.ResizeInvalidate( _TwoSided ? indexCount<<1 : indexCount );

        FMeshVertex * pVert = _Vertices.ToPtr();
        unsigned int * pIndices = _Indices.ToPtr();

        for ( int y = 0; y < resy; ++y ) {
            const float lerpY = y / ( float )( resy - 1 );
            const Float3 py0 = Corner00.Lerp( Corner01, lerpY );
            const Float3 py1 = Corner10.Lerp( Corner11, lerpY );
            const float ty = lerpY * _TexCoordScale;

            for ( int x = 0; x < resx; ++x ) {
                const float lerpX = x / ( float )( resx - 1 );

                pVert->Position = py0.Lerp( py1, lerpX );
                pVert->TexCoord.X = lerpX * _TexCoordScale;
                pVert->TexCoord.Y = ty;
                pVert->Normal = normal;

                ++pVert;
            }
        }

        if ( _TwoSided ) {
            normal = -normal;

            for ( int y = 0; y < resy; ++y ) {
                const float lerpY = y / ( float )( resy - 1 );
                const Float3 py0 = Corner00.Lerp( Corner01, lerpY );
                const Float3 py1 = Corner10.Lerp( Corner11, lerpY );
                const float ty = lerpY * _TexCoordScale;

                for ( int x = 0; x < resx; ++x ) {
                    const float lerpX = x / ( float )( resx - 1 );

                    pVert->Position = py0.Lerp( py1, lerpX );
                    pVert->TexCoord.X = lerpX * _TexCoordScale;
                    pVert->TexCoord.Y = ty;
                    pVert->Normal = normal;

                    ++pVert;
                }
            }
        }

        for ( int y = 0; y < resy; ++y ) {

            int index0 = y*resx;
            int index1 = (y+1)*resx;

            for ( int x = 0; x < resx; ++x ) {
                int quad00 = index0 + x;
                int quad01 = index0 + x + 1;
                int quad10 = index1 + x;
                int quad11 = index1 + x + 1;

                if ( ( x + 1 ) < resx && ( y + 1 ) < resy ) {
                    *pIndices++ = quad00;
                    *pIndices++ = quad10;
                    *pIndices++ = quad11;
                    *pIndices++ = quad11;
                    *pIndices++ = quad01;
                    *pIndices++ = quad00;
                }
            }
        }

        if ( _TwoSided ) {
            for ( int y = 0; y < resy; ++y ) {

                int index0 = vertexCount + y*resx;
                int index1 = vertexCount + (y+1)*resx;

                for ( int x = 0; x < resx; ++x ) {
                    int quad00 = index0 + x;
                    int quad01 = index0 + x + 1;
                    int quad10 = index1 + x;
                    int quad11 = index1 + x + 1;

                    if ( ( x + 1 ) < resx && ( y + 1 ) < resy ) {
                        *pIndices++ = quad00;
                        *pIndices++ = quad01;
                        *pIndices++ = quad11;
                        *pIndices++ = quad11;
                        *pIndices++ = quad10;
                        *pIndices++ = quad00;
                    }
                }
            }
        }

        CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Length(), _Indices.ToPtr(), _Indices.Length() );

        _Bounds.Clear();
        _Bounds.AddPoint( Corner00 );
        _Bounds.AddPoint( Corner01 );
        _Bounds.AddPoint( Corner10 );
        _Bounds.AddPoint( Corner11 );
    }
};

struct FCylinderShape {
    static void CreateMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _VDiv ) {
        int i, j;
        float Angle;
        const float RadWidth = _Radius;
        const float RadHeight = _Height * 0.5f;
        const float InvRadius = 1.0f / _Radius;
        unsigned int Quad[ 4 ];        

        _Vertices.ResizeInvalidate( 6 * ( _VDiv + 1 ) );
        _Indices.ResizeInvalidate( 3 * _VDiv * 6 );

        _Bounds.Mins.X = -RadWidth;
        _Bounds.Mins.Z = -RadWidth;
        _Bounds.Mins.Y = -RadHeight;

        _Bounds.Maxs.X = RadWidth;
        _Bounds.Maxs.Z = RadWidth;
        _Bounds.Maxs.Y = RadHeight;

        FMeshVertex * pVerts = _Vertices.ToPtr();

        int FirstVertex = 0;

        // Rings

        for ( j = 0 ; j <= _VDiv ; j++ ) {
            pVerts[ FirstVertex + j ].Position = Float3( 0.0f, -RadHeight, 0.0f );
            pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 0.0f ) * _TexCoordScale;
            pVerts[ FirstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
        }
        FirstVertex += _VDiv + 1;

        for ( j = 0, Angle = 0 ; j <= _VDiv ; j++ ) {
            float s, c;
            FMath::RadSinCos( Angle, s, c );
            pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, -RadHeight, RadWidth*s );
            pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 1.0f ) * _TexCoordScale;
            pVerts[ FirstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
            Angle += FMath::_2PI / ( _VDiv );
        }
        FirstVertex += _VDiv + 1;

        for ( j = 0, Angle = 0 ; j <= _VDiv ; j++ ) {
            float s, c;
            FMath::RadSinCos( Angle, s, c );
            pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, -RadHeight, RadWidth*s );
            pVerts[ FirstVertex + j ].TexCoord = Float2( 1.0f - j / float( _VDiv ), 1.0f ) * _TexCoordScale;
            pVerts[ FirstVertex + j ].Normal = Float3( pVerts[ FirstVertex + j ].Position.X * InvRadius, 0.0f, pVerts[ FirstVertex + j ].Position.Z * InvRadius );
            Angle += FMath::_2PI / ( _VDiv );
        }
        FirstVertex += _VDiv + 1;

        for ( j = 0, Angle = 0 ; j <= _VDiv ; j++ ) {
            float s, c;
            FMath::RadSinCos( Angle, s, c );
            pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, RadHeight, RadWidth*s );
            pVerts[ FirstVertex + j ].TexCoord = Float2( 1.0f - j / float( _VDiv ), 0.0f ) * _TexCoordScale;
            pVerts[ FirstVertex + j ].Normal = Float3( pVerts[ FirstVertex + j ].Position.X * InvRadius, 0.0f, pVerts[ FirstVertex + j ].Position.Z * InvRadius );
            Angle += FMath::_2PI / ( _VDiv );
        }
        FirstVertex += _VDiv + 1;

        for ( j = 0, Angle = 0 ; j <= _VDiv ; j++ ) {
            float s, c;
            FMath::RadSinCos( Angle, s, c );
            pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, RadHeight, RadWidth*s );
            pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 0.0f ) * _TexCoordScale;
            pVerts[ FirstVertex + j ].Normal = Float3( 0, 1.0f, 0 );
            Angle += FMath::_2PI / ( _VDiv );
        }
        FirstVertex += _VDiv + 1;

        for ( j = 0 ; j <= _VDiv ; j++ ) {
            pVerts[ FirstVertex + j ].Position = Float3( 0.0f, RadHeight, 0.0f );
            pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 1.0f ) * _TexCoordScale;
            pVerts[ FirstVertex + j ].Normal = Float3( 0, 1.0f, 0 );
        }
        FirstVertex += _VDiv + 1;

        AN_Assert( FirstVertex == _Vertices.Length() );

        // generate indices

        unsigned int * pIndices = _Indices.ToPtr();

        _Indices.Memset(0);

        FirstVertex = 0;
        for ( i = 0 ; i < 3 ; i++ ) {
            for ( j = 0 ; j < _VDiv ; j++ ) {
                Quad[3] = FirstVertex + j;
                Quad[2] = FirstVertex + j+1;
                Quad[1] = FirstVertex + j+1+(_VDiv+1);
                Quad[0] = FirstVertex + j+(_VDiv+1);

                *pIndices++ = Quad[0];
                *pIndices++ = Quad[1];
                *pIndices++ = Quad[2];
                *pIndices++ = Quad[2];
                *pIndices++ = Quad[3];
                *pIndices++ = Quad[0];
            }
            FirstVertex += ( _VDiv + 1 ) * 2;
        }

        AN_Assert( FirstVertex == _Vertices.Length() );

        CalcTangentSpace( pVerts, _Vertices.Length(), _Indices.ToPtr(), _Indices.Length() );
    }
};
