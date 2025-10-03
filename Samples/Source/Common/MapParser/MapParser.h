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

#include <Hork/Math/Plane.h>
#include <Hork/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

class Lexer;

class MapParser final
{
public:
    struct Entity
    {
        char                ClassName[256] = "Unknown";
        char                Target[256] = "";
        char                TargetName[256] = "";
        Float3              Origin;
        float               Angle = 0;
        Float3              Color = Float3(1,1,1);
        float               Radius = 40;
        char                VerticalAngleHack = 0;
        float               Lip = -0.2f;
        //int               ModelIndex;
        float               Wait = 3;
        float               Speed = 10;
        int                 SpawnFlags = 0;
        int                 FirstBrush = 0;
        int                 BrushCount = 0;
        int                 FirstPatch = 0;
        int                 PatchCount = 0;
    };

    struct Material
    {
        char                Name[256];
    };

    struct Brush
    {
        int                 FirstFace;
        int                 FaceCount;
    };

    struct BrushFace
    {
        PlaneF              Plane;
        float               TexVecs[2][4];
        uint32_t            Material;
    };

    struct Patch
    {
        int                 FirstVert;
        int                 VertexCount;
        uint32_t            Material;
    };

    struct PatchVertex
    {
        Float3              Position;
        Float2              Texcoord;
    };

    void                    Parse(const char* buffer);

    int                     FindEntity(const char* className) const;

    Vector<Entity> const&       GetEntities() const { return m_Entities; }
    Vector<Brush> const&        GetBrushes() const { return m_Brushes; }
    Vector<BrushFace> const&    GetFaces() const { return m_Faces; }
    Vector<Patch> const&        GetPatches() const { return m_Patches; }
    Vector<PatchVertex> const&  GetPatchVertices() const { return m_PatchVertices; }


private:
    void                    ParseEntity(Entity& entity, Lexer& lex);
    void                    ParseBlock(Entity& entity, Lexer& lex);
    bool                    ParseBrush(Brush& brush, Lexer& lex);
    bool                    ParsePatch(Patch& patch, Lexer& lex);

    Vector<Entity>          m_Entities;
    Vector<Brush>           m_Brushes;
    Vector<BrushFace>       m_Faces;
    Vector<Patch>           m_Patches;
    Vector<PatchVertex>     m_PatchVertices;
    Vector<Material>        m_Materials;
};


HK_NAMESPACE_END
