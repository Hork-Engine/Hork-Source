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

#include <Core/Public/Triangulator.h>

#include "glutess/glutess.h"

ATriangulatorBase::ATriangulatorBase() {
    Tesselator = gluNewTess();
}

ATriangulatorBase::~ATriangulatorBase() {
    gluDeleteTess( static_cast< GLUtesselator * >(Tesselator) );
}

void ATriangulatorBase::SetCallback( uint32_t _Name, SCallback _CallbackFunc ) {
    gluTessCallback( static_cast< GLUtesselator * >(Tesselator), _Name, _CallbackFunc );
}

void ATriangulatorBase::SetBoundary( bool _Flag ) {
    gluTessProperty( static_cast< GLUtesselator * >(Tesselator), GLU_TESS_BOUNDARY_ONLY, _Flag );
}

void ATriangulatorBase::SetNormal( Double3 const & _Normal ) {
    gluTessNormal( static_cast< GLUtesselator * >(Tesselator), _Normal.X, _Normal.Y, _Normal.Z );
}

void ATriangulatorBase::BeginPolygon( void * _Data ) {
    gluTessBeginPolygon( static_cast< GLUtesselator * >(Tesselator), _Data );
}

void ATriangulatorBase::EndPolygon() {
    gluTessEndPolygon( static_cast< GLUtesselator * >(Tesselator) );
}

void ATriangulatorBase::BeginContour() {
    gluTessBeginContour( static_cast< GLUtesselator * >(Tesselator) );
}

void ATriangulatorBase::EndContour() {
    gluTessEndContour( static_cast< GLUtesselator * >(Tesselator) );
}

void ATriangulatorBase::ProcessVertex( Double3 & _Vertex, const void * _Data ) {
    gluTessVertex( static_cast< GLUtesselator * >(Tesselator), &_Vertex.X, const_cast< void * >(_Data) );
}
