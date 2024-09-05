/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Geometry/Triangulator.h>

#include <glutess/glutess.h>

HK_NAMESPACE_BEGIN

TriangulatorBase::TriangulatorBase()
{
    m_Tesselator = gluNewTess();
}

TriangulatorBase::~TriangulatorBase()
{
    gluDeleteTess(static_cast<GLUtesselator*>(m_Tesselator));
}

void TriangulatorBase::SetCallback(uint32_t name, SCallback callback)
{
    gluTessCallback(static_cast<GLUtesselator*>(m_Tesselator), name, callback);
}

void TriangulatorBase::SetBoundary(bool flag)
{
    gluTessProperty(static_cast<GLUtesselator*>(m_Tesselator), GLU_TESS_BOUNDARY_ONLY, flag);
}

void TriangulatorBase::SetNormal(Double3 const& normal)
{
    gluTessNormal(static_cast<GLUtesselator*>(m_Tesselator), normal.X, normal.Y, normal.Z);
}

void TriangulatorBase::BeginPolygon(void* data)
{
    gluTessBeginPolygon(static_cast<GLUtesselator*>(m_Tesselator), data);
}

void TriangulatorBase::EndPolygon()
{
    gluTessEndPolygon(static_cast<GLUtesselator*>(m_Tesselator));
}

void TriangulatorBase::BeginContour()
{
    gluTessBeginContour(static_cast<GLUtesselator*>(m_Tesselator));
}

void TriangulatorBase::EndContour()
{
    gluTessEndContour(static_cast<GLUtesselator*>(m_Tesselator));
}

void TriangulatorBase::ProcessVertex(Double3& vertex, const void* data)
{
    gluTessVertex(static_cast<GLUtesselator*>(m_Tesselator), &vertex.X, const_cast<void*>(data));
}

HK_NAMESPACE_END
