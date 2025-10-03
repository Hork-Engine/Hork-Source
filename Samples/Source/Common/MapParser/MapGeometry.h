/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#pragma once

#include "MapParser.h"

#include <Hork/Geometry/VertexFormat.h>

HK_NAMESPACE_BEGIN

class MapGeometry
{
public:
    struct Surface
    {
        int             FirstVert;
        int             VertexCount;
        int             FirstIndex;
        int             IndexCount;
        uint32_t        Material;
    };

    struct ClipHull
    {
        int             FirstVert;
        int             VertexCount;
        int             FirstIndex;
        int             IndexCount;
    };

    struct Entity
    {
        int             FirstSurface;
        int             SurfaceCount;

        int             FirstClipHull;
        int             ClipHullCount;
    };

    void                Build(MapParser const& parser);

    Vector<Surface> const&     GetSurfaces() const { return m_Surfaces; }
    Vector<MeshVertex> const&  GetVertices() const { return m_Vertices; }
    Vector<uint32_t> const&    GetIndices() const { return m_Indices; }
    Vector<Float3> const&      GetClipVertices() const { return m_ClipVertices; }
    Vector<uint32_t> const&    GetClipIndices() const { return m_ClipIndices; }
    Vector<ClipHull> const&    GetClipHulls() const { return m_ClipHulls; }
    Vector<Entity> const&      GetEntities() const { return m_Entities; }

private:
    struct FaceInfo
    {
        int             FaceNum;
        int             Material;
        MapParser::Brush const* Brush;
    };

    void                ExtractSurfaces(Vector<FaceInfo> const& faceInfos, Vector<MapParser::BrushFace> const& faces);
    void                ExtractClipHull(MapParser::Brush const& brush, Vector<MapParser::BrushFace> const& faces);

    Vector<Surface>     m_Surfaces;
    Vector<MeshVertex>  m_Vertices;
    Vector<uint32_t>    m_Indices;
    Vector<Float3>      m_ClipVertices;
    Vector<uint32_t>    m_ClipIndices;
    Vector<ClipHull>    m_ClipHulls;
    Vector<Entity>      m_Entities;
};

HK_NAMESPACE_END
