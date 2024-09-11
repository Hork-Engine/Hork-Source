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

#include "MapParser.h"

#include "../Lexer/Lexer.h"
#include <Hork/Core/Parse.h>

HK_NAMESPACE_BEGIN

namespace
{

constexpr float MapCoordToMeters = 1.0f / 32.0f;

HK_FORCEINLINE Float3 ConvertMapCoord(Float3 const& p)
{
    Float3 result;
    result.X = -p.X;
    result.Y = p.Z;
    result.Z = p.Y;
    return result * MapCoordToMeters;
}

//HK_FORCEINLINE Float3 ConvertMapNormal(Float3 const& n)
//{
//    Float3 result;
//    result.X = -n.X;
//    result.Y = n.Z;
//    result.Z = n.Y;
//    return result;
//}

constexpr Float3 baseaxis[18] =
{
    Float3(0, 0, 1), Float3(1, 0, 0), Float3(0, -1, 0),  // floor
    Float3(0, 0, -1), Float3(1, 0, 0), Float3(0, -1, 0), // ceiling
    Float3(1, 0, 0), Float3(0, 1, 0), Float3(0, 0, -1),  // west wall
    Float3(-1, 0, 0), Float3(0, 1, 0), Float3(0, 0, -1), // east wall
    Float3(0, 1, 0), Float3(1, 0, 0), Float3(0, 0, -1),  // south wall
    Float3(0, -1, 0), Float3(1, 0, 0), Float3(0, 0, -1)  // north wall
};

void TextureAxisFromPlane(PlaneF const& plane, Float3& xv, Float3& yv)
{
    float best = 0;
    int bestaxis = 0;

    for (int i = 0; i < 6; i++)
    {
        float dp = Math::Dot(plane.Normal, baseaxis[i * 3]);
        if (dp > best + 0.0001f)
        {
            best = dp;
            bestaxis = i;
        }
    }

    xv = baseaxis[bestaxis * 3 + 1];
    yv = baseaxis[bestaxis * 3 + 2];
}

void CalcTextureVecs(PlaneF const& plane, Float2 const& shift, float rotate, Float2 const& scale, float mappingVecs[2][4])
{
    Float3 vecs[2];
    int sv, tv;
    float sinv, cosv;

    TextureAxisFromPlane(plane, vecs[0], vecs[1]);

    // rotate axis
    if (rotate == 0)
    {
        sinv = 0;
        cosv = 1;
    }
    else if (rotate == 90)
    {
        sinv = 1;
        cosv = 0;
    }
    else if (rotate == 180)
    {
        sinv = 0;
        cosv = -1;
    }
    else if (rotate == 270)
    {
        sinv = -1;
        cosv = 0;
    }
    else
    {
        Math::DegSinCos(rotate, sinv, cosv);
    }

    if (vecs[0][0])
        sv = 0;
    else if (vecs[0][1])
        sv = 1;
    else
        sv = 2;

    if (vecs[1][0])
        tv = 0;
    else if (vecs[1][1])
        tv = 1;
    else
        tv = 2;

    for (int i = 0; i < 2; i++)
    {
        float ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
        float nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
        vecs[i][sv] = ns;
        vecs[i][tv] = nt;

        *(Float3*)(&mappingVecs[i][0]) = vecs[i] / scale[i];
        mappingVecs[i][3] = shift[i];
    }
}

uint32_t AddMaterial(const char* name, Vector<MapParser::Material>& materials)
{
    uint32_t index = 0;
    for (auto& material : materials)
    {
        if (!Core::Strcmp(material.Name, name))
            return index;
        index++;
    }

    Core::Strcpy(materials.EmplaceBack().Name, sizeof(materials.EmplaceBack().Name), name);
    return index;
}

}

void MapParser::Parse(const char* buffer)
{
    Lexer lex;

    lex.SetName("Map");
    lex.SetSource(buffer);
    lex.AddOperator("{");
    lex.AddOperator("}");
    lex.AddOperator("(");
    lex.AddOperator(")");

    m_Entities.Clear();

    while (1)
    {
        auto err = lex.NextToken();
        if (err != Lexer::ErrorCode::No)
            break;

        const char* token = lex.Token();
        if (*token == '{')
        {
            ParseEntity(m_Entities.EmplaceBack(), lex);
        }
    }
}

void MapParser::ParseEntity(Entity& entity, Lexer& lex)
{
    entity.FirstBrush = m_Brushes.Size();
    entity.FirstPatch = m_Patches.Size();

    while (1)
    {
        auto err = lex.NextToken();
        if (err != Lexer::ErrorCode::No)
            break;
        const char* token = lex.Token();
        if (!*token || *token == '}')
            break;

        if (*token == '{')
        {
#if 0
            ParseBlock( entity, lex );
#else
            ParseBrush(m_Brushes.EmplaceBack(), lex);
            entity.BrushCount++;
#endif
        }
        else if (!Core::Stricmp(token, "classname"))
        {
            Core::Strcpy(entity.ClassName, sizeof(entity.ClassName), lex.ExpectString());
        }
        else if (!Core::Stricmp(token, "origin"))
        {
            err = lex.NextToken();
            if (err != Lexer::ErrorCode::No)
                break;

            token = lex.Token();
            sscanf(token, "%f %f %f", &entity.Origin.X, &entity.Origin.Y, &entity.Origin.Z);
            entity.Origin = ConvertMapCoord(entity.Origin);
        }
        else if (!Core::Stricmp(token, "target"))
        {
            Core::Strcpy(entity.Target, sizeof(entity.Target), lex.ExpectString());
        }
        else if (!Core::Stricmp(token, "targetname"))
        {
            Core::Strcpy(entity.TargetName, sizeof(entity.TargetName), lex.ExpectString());
        }
        else if (!Core::Stricmp(token, "angle"))
        {
            float a = Core::ParseFloat(lex.ExpectString());

            switch ((int)a)
            {
                case -1:
                case -2:
                    entity.VerticalAngleHack = (int)a;
                    break;
                default:
                    entity.VerticalAngleHack = 0;
                    break;
            }

            entity.Angle = Angl::sNormalize360(a - 90.0f);
        }
        else if (!Core::Stricmp(token, "lip"))
        {
            entity.Lip = Core::ParseFloat(lex.ExpectString()) * MapCoordToMeters;
        }
        else if (!Core::Stricmp(token, "speed"))
        {
            entity.Speed = Core::ParseFloat(lex.ExpectString()) * MapCoordToMeters;
        }
        else if (!Core::Stricmp(token, "wait"))
        {
            entity.Wait = Core::ParseFloat(lex.ExpectString());
        }
        else if (!Core::Stricmp(token, "spawnflags"))
        {
            entity.SpawnFlags = Core::Parse<int32_t>(lex.ExpectString());
        }
        else if (!Core::Stricmp(token, "color"))
        {
            const char* str = lex.ExpectString();
            float r = 1, g = 1, b = 1;
            std::sscanf(str, "%f %f %f", &r, &g, &b);
            entity.Color[0] = r;
            entity.Color[1] = g;
            entity.Color[2] = b;
        }
        else if (!Core::Stricmp(token, "radius"))
        {
            entity.Radius = Core::ParseFloat(lex.ExpectString());
        }
        else
        {
            // Some field
            lex.ExpectString();
        }
    }
}

void MapParser::ParseBlock(Entity& entity, Lexer& lex)
{
    while (1)
    {
        auto err = lex.NextToken();
        if (err != Lexer::ErrorCode::No)
            break;

        const char* token = lex.Token();
        if (!*token || *token == '}')
            break;

        if (!Core::Stricmp(token, "brushDef3"))
        {
            err = lex.NextToken();
            if (err != Lexer::ErrorCode::No)
                break;

            token = lex.Token();
            if (!*token || *token == '}')
                break;

            if (*token == '{')
            {
                ParseBrush(m_Brushes.EmplaceBack(), lex);
                entity.BrushCount++;
            }
        }
        else if (!Core::Stricmp(token, "patchDef3"))
        {
            // FIXME: what about patchDef2 ???
            err = lex.NextToken();
            if (err != Lexer::ErrorCode::No)
                break;

            token = lex.Token();
            if (!*token || *token == '}')
                break;

            if (*token == '{')
            {
                ParsePatch(m_Patches.EmplaceBack(), lex);
                entity.PatchCount++;
            }
        }
        else
        {
            err = lex.NextToken();
            if (err != Lexer::ErrorCode::No)
                break;

            token = lex.Token();
            if (!*token || *token == '}')
                break;

            if (*token == '{')
                lex.SkipBlock();
        }
    }
}

int MapParser::FindEntity(const char* className) const
{
    int i = 0;
    for (Entity const& ent : m_Entities)
    {
        if (!Core::Stricmp(ent.ClassName, className))
            return i;
        i++;
    }
    return -1;
}

bool MapParser::ParseBrush(Brush& brush, Lexer& lex)
{
    Float2 shift, scale;
    float rotate;
    PlaneF plane;
    Float3 p[3];

    brush.FirstFace = m_Faces.Size();
    brush.FaceCount = 0;

    while (1)
    {
        const char* token = lex.GetIdentifier();
        if (!*token || *token == '}')
            break;

        lex.PrevToken();

        // (x1 y1 z1) (x2 y2 z2) (x3 y3 z3) TEXTURE Xoffset Yoffset rotation Xscale Yscale

        if (!lex.ExpectVector(p[0])) return false;
        if (!lex.ExpectVector(p[1])) return false;
        if (!lex.ExpectVector(p[2])) return false;

        plane.FromPoints(p[0], p[1], p[2]);

        BrushFace& face = m_Faces.EmplaceBack();
        brush.FaceCount++;

        face.Material = AddMaterial(lex.GetIdentifier(), m_Materials);

        shift.X = lex.ExpectFloat();
        shift.Y = lex.ExpectFloat();
        rotate = lex.ExpectFloat();
        scale.X = lex.ExpectFloat();
        scale.Y = lex.ExpectFloat();

        if (!scale[0])
            scale[0] = 1;
        if (!scale[1])
            scale[1] = 1;

        CalcTextureVecs(plane, shift, rotate, scale, face.TexVecs);

        for (int i = 0; i < 2; i++)
        {
            Float3& v = *(Float3*)(&face.TexVecs[i]);
            v /= MapCoordToMeters;

            std::swap(v.Y, v.Z);
            v.X = -v.X;
        }

        // Recalc plane
        p[0] = ConvertMapCoord(p[0]);
        p[1] = ConvertMapCoord(p[1]);
        p[2] = ConvertMapCoord(p[2]);
        face.Plane.FromPoints(p[0], p[1], p[2]);
    }

    return true;
}

bool MapParser::ParsePatch(Patch& patch, Lexer& lex)
{
    float PatchInfo[7];
    float PositionAndTexCoord[5];

    patch.FirstVert = m_PatchVertices.Size();
    patch.VertexCount = 0;

    while (1)
    {
        lex.NextToken();
        const char* token = lex.Token();
        if (!*token || *token == '}')
            break;

        if (*token == '(')
        {
            lex.PrevToken();

            lex.ExpectVector(PatchInfo, 7);

            lex.NextToken();
            token = lex.Token();
            if (*token != '(')
                break;

            for (int j = 0; j < (int)PatchInfo[0]; j++)
            {
                lex.NextToken();
                token = lex.Token();
                if (*token != '(')
                    return false;

                for (int i = 0; i < (int)PatchInfo[1]; i++)
                {
                    lex.ExpectVector(PositionAndTexCoord, 5);

                    PatchVertex& patchVert = m_PatchVertices.EmplaceBack();
                    patchVert.Position = *(Float3*)PositionAndTexCoord;
                    patchVert.Texcoord = *(Float2*)(PositionAndTexCoord + 3);

                    patchVert.Position = ConvertMapCoord(patchVert.Position);

                    patch.VertexCount++;
                }

                lex.NextToken();
                token = lex.Token();
                if (*token != ')')
                    return false;
            }

            lex.NextToken();
            token = lex.Token();
            if (*token != ')')
                return false;
        }
        else
        {
            // texture name
            patch.Material = AddMaterial(token, m_Materials);
            lex.SkipRestOfLine();
        }
    }

    return true;
}

HK_NAMESPACE_END
