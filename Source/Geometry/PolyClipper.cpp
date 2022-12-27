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

#include <Geometry/PolyClipper.h>
#include <clipper/clipper.hpp>

namespace
{

using ClipperPoint = ClipperLib::IntPoint;
using ClipperPath  = ClipperLib::Path;
using ClipperPaths = ClipperLib::Paths;

const double CLIPPER_TO_LONG_CONVERSION_NUMBER = 1000000000.0; //1518500249; // clipper.cpp / hiRange var
const double CLIPPER_TO_DOUBLE_CONVERSION_NUMBER = 0.000000001;
const double CLIPPER_TO_FLOAT_CONVERSION_NUMBER = 0.000000001;

#define CLIPPER_DOUBLE_TO_LONG(p) (static_cast<ClipperLib::cInt>((p)*CLIPPER_TO_LONG_CONVERSION_NUMBER))
#define CLIPPER_LONG_TO_DOUBLE(p) ((p)*CLIPPER_TO_DOUBLE_CONVERSION_NUMBER)

#define CLIPPER_FLOAT_TO_LONG(p) (static_cast<ClipperLib::cInt>((p)*CLIPPER_TO_LONG_CONVERSION_NUMBER))
#define CLIPPER_LONG_TO_FLOAT(p) ((p)*CLIPPER_TO_FLOAT_CONVERSION_NUMBER)

void ConstructContour(ClipperPath const& path, Hk::ClipperContour& contour)
{
    int numPoints = path.size();

    contour.Resize(numPoints);
    for (int j = 0; j < numPoints; j++)
    {
        ClipperPoint const& point = path[j];
        contour[j].X = CLIPPER_LONG_TO_DOUBLE(point.X);
        contour[j].Y = CLIPPER_LONG_TO_DOUBLE(point.Y);
    }
}

void ComputeNode_r(ClipperLib::PolyNode const& node, Hk::TVector<Hk::ClipperPolygon>& polygons)
{
    Hk::ClipperPolygon polygon;
    Hk::ClipperContour hole;

    ConstructContour(node.Contour, polygon.Outer);

    for (int j = 0; j < (int)node.ChildCount(); j++)
    {
        ClipperLib::PolyNode const& child = *node.Childs[j];

        if (child.IsHole() && !child.IsOpen())
        {
            ConstructContour(child.Contour, hole);
            polygon.Holes.Add(hole);

            // FIXME: May hole have childs?
            HK_ASSERT(child.ChildCount() == 0);
        }
        else
        {
            if (!child.IsOpen())
            {
                ComputeNode_r(child, polygons);
            }
        }
    }

    polygons.Add(polygon);
}

void ComputeContours(ClipperLib::PolyTree const& polygonTree, Hk::TVector<Hk::ClipperPolygon>& polygons)
{
    if (polygonTree.Contour.size() > 0 && !polygonTree.IsOpen())
    {
        ComputeNode_r(polygonTree, polygons);
    }
    else
    {
        for (int i = 0; i < polygonTree.ChildCount(); i++)
        {
            ClipperLib::PolyNode& child = *polygonTree.Childs[i];

            if (child.IsHole())
            {

                // FIXME: May hole have childs?

                HK_ASSERT(0);
            }
            else
            {
                if (!child.IsOpen())
                {
                    ComputeNode_r(child, polygons);
                }
            }
        }
    }
}

}

HK_NAMESPACE_BEGIN

PolyClipper::PolyClipper() :
    m_pImpl(MakeUnique<ClipperLib::Clipper>()), m_Transform3D(Float3x3::Identity()), m_InvTransform3D(Float3x3::Identity())
{
}

PolyClipper::~PolyClipper()
{
}

void PolyClipper::SetTransform(Float3x3 const& transform3D)
{
    m_Transform3D   = transform3D;
    m_InvTransform3D = m_Transform3D.Transposed();
}

void PolyClipper::SetTransformFromNormal(Float3 const& normal)
{
    m_Transform3D[2] = normal;

    normal.ComputeBasis(m_Transform3D[0], m_Transform3D[1]);

    m_InvTransform3D = m_Transform3D.Transposed();
}

static void ConstructClipperPath(const double* pointsXYZ, int pointsCount, Float3x3 const& invTransform3D, ClipperPath& path)
{
    Double3 p;
    for (int i = 0; i < pointsCount; i++, pointsXYZ += 3)
    {
        p = invTransform3D * *(Double3*)pointsXYZ;
        path.emplace_back(ClipperPoint(CLIPPER_DOUBLE_TO_LONG(p.X), CLIPPER_DOUBLE_TO_LONG(p.Y)));
    }
}

static void ConstructClipperPath(const double* pointsXY, int pointsCount, ClipperPath& path)
{
    for (int i = 0; i < pointsCount; i++, pointsXY += 2)
    {
        path.emplace_back(ClipperPoint(CLIPPER_DOUBLE_TO_LONG(pointsXY[0]), CLIPPER_DOUBLE_TO_LONG(pointsXY[1])));
    }
}

void PolyClipper::AddSubj2D(Double2 const* points, int pointsCount, bool closed)
{
    ClipperPath Path;

    ConstructClipperPath(&points->X, pointsCount, Path);

    m_pImpl->AddPath(Path, ClipperLib::ptSubject, closed);
}

void PolyClipper::AddClip2D(Double2 const* points, int pointsCount, bool closed)
{
    ClipperPath Path;

    ConstructClipperPath(&points->X, pointsCount, Path);

    m_pImpl->AddPath(Path, ClipperLib::ptClip, closed);
}

void PolyClipper::AddSubj3D(Double3 const* points, int pointsCount, bool closed)
{
    ClipperPath Path;

    ConstructClipperPath(&points->X, pointsCount, m_InvTransform3D, Path);

    m_pImpl->AddPath(Path, ClipperLib::ptSubject, closed);
}

void PolyClipper::AddClip3D(Double3 const* points, int pointsCount, bool closed)
{
    ClipperPath Path;

    ConstructClipperPath(&points->X, pointsCount, m_InvTransform3D, Path);

    m_pImpl->AddPath(Path, ClipperLib::ptClip, closed);
}

bool PolyClipper::Execute(POLY_CLIP_TYPE clipType, TVector<ClipperPolygon>& polygons)
{
    ClipperLib::PolyTree polygonTree;

    m_pImpl->StrictlySimple(true);

    if (!m_pImpl->Execute((ClipperLib::ClipType)clipType, polygonTree, ClipperLib::pftNonZero, ClipperLib::pftNonZero))
    {
        return false;
    }

    ComputeContours(polygonTree, polygons);
    return true;
}

bool PolyClipper::Execute(POLY_CLIP_TYPE clipType, TVector<ClipperContour>& contours)
{
    ClipperPaths resultPaths;

    m_pImpl->StrictlySimple(true);

    if (!m_pImpl->Execute((ClipperLib::ClipType)clipType, resultPaths, ClipperLib::pftNonZero, ClipperLib::pftNonZero))
    {
        return false;
    }

    contours.Resize(resultPaths.size());
    for (int i = 0; i < contours.Size(); i++)
    {
        ConstructContour(resultPaths[i], contours[i]);
    }
    return true;
}

void PolyClipper::Clear()
{
    m_pImpl->Clear();
}

HK_NAMESPACE_END
