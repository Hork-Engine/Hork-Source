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

#include "Utilites.h"
#include "TangentSpace.h"

HK_NAMESPACE_BEGIN

void CreateBoxMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale)
{
    constexpr unsigned int indices[6 * 6] =
        {
            0, 1, 2, 2, 3, 0,                                                 // front face
            4, 5, 6, 6, 7, 4,                                                 // back face
            5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face
            1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
            3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
            1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
        };

    Vertices.ResizeInvalidate(24);
    Indices.ResizeInvalidate(36);

    Core::Memcpy(Indices.ToPtr(), indices, sizeof(indices));

    const Float3 halfSize = Extents * 0.5f;

    Bounds.Mins  = -halfSize;
    Bounds.Maxs = halfSize;

    Float3 const& mins = Bounds.Mins;
    Float3 const& maxs = Bounds.Maxs;

    MeshVertex* pVerts = Vertices.ToPtr();

    Half zero = 0.0f;
    Half pos  = 1.0f;
    Half neg  = -1.0f;

    pVerts[0 + 8 * 0].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[0 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[1 + 8 * 0].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[1 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[2 + 8 * 0].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[2 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[3 + 8 * 0].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[3 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[4 + 8 * 0].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[4 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[5 + 8 * 0].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[5 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[6 + 8 * 0].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[6 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[7 + 8 * 0].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[7 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[0 + 8 * 1].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[0 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[1 + 8 * 1].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[1 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 1].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[2 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[3 + 8 * 1].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[3 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[4 + 8 * 1].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[4 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[5 + 8 * 1].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[5 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[6 + 8 * 1].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[6 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[7 + 8 * 1].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[7 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[1 + 8 * 2].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[1 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[0 + 8 * 2].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[0 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[5 + 8 * 2].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[5 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[4 + 8 * 2].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[4 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);


    pVerts[3 + 8 * 2].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[3 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 2].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[2 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[7 + 8 * 2].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[7 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[6 + 8 * 2].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[6 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateSphereMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    int          x, y;
    float        verticalAngle, horizontalAngle;
    unsigned int quad[4];

    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 4);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 4);

    Vertices.ResizeInvalidate((NumHorizontalSubdivs + 1) * (NumVerticalSubdivs + 1));
    Indices.ResizeInvalidate(NumHorizontalSubdivs * NumVerticalSubdivs * 6);

    Bounds.Mins.X = Bounds.Mins.Y = Bounds.Mins.Z = -Radius;
    Bounds.Maxs.X = Bounds.Maxs.Y = Bounds.Maxs.Z = Radius;

    MeshVertex* pVert = Vertices.ToPtr();

    const float verticalStep    = Math::_PI / NumVerticalSubdivs;
    const float horizontalStep  = Math::_2PI / NumHorizontalSubdivs;
    const float verticalScale   = 1.0f / NumVerticalSubdivs;
    const float horizontalScale = 1.0f / NumHorizontalSubdivs;

    for (y = 0, verticalAngle = -Math::_HALF_PI; y <= NumVerticalSubdivs; y++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position = Float3(scaledR * c, scaledH, scaledR * s);
            pVert->SetTexCoord(Float2(1.0f - static_cast<float>(x) * horizontalScale, 1.0f - static_cast<float>(y) * verticalScale) * TexCoordScale);
            pVert->SetNormal(r * c, h, r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = Indices.ToPtr();
    for (y = 0; y < NumVerticalSubdivs; y++)
    {
        const int y2 = y + 1;
        for (x = 0; x < NumHorizontalSubdivs; x++)
        {
            const int x2 = x + 1;

            quad[0] = y * (NumHorizontalSubdivs + 1) + x;
            quad[1] = y2 * (NumHorizontalSubdivs + 1) + x;
            quad[2] = y2 * (NumHorizontalSubdivs + 1) + x2;
            quad[3] = y * (NumHorizontalSubdivs + 1) + x2;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreatePlaneMeshXZ(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale)
{
    Vertices.ResizeInvalidate(4);
    Indices.ResizeInvalidate(6);

    const float halfWidth  = Width * 0.5f;
    const float halfHeight = Height * 0.5f;

    const MeshVertex Verts[4] = {
        MakeMeshVertex(Float3(-halfWidth, 0, -halfHeight), Float2(0, 0), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0)),
        MakeMeshVertex(Float3(-halfWidth, 0, halfHeight), Float2(0, TexCoordScale.Y), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0)),
        MakeMeshVertex(Float3(halfWidth, 0, halfHeight), Float2(TexCoordScale.X, TexCoordScale.Y), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0)),
        MakeMeshVertex(Float3(halfWidth, 0, -halfHeight), Float2(TexCoordScale.X, 0), Float3(0, 0, 1), 1.0f, Float3(0, 1, 0))};

    Core::Memcpy(Vertices.ToPtr(), &Verts, 4 * sizeof(MeshVertex));

    constexpr unsigned int indices[6] = {0, 1, 2, 2, 3, 0};
    Core::Memcpy(Indices.ToPtr(), &indices, sizeof(indices));

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());

    Bounds.Mins.X  = -halfWidth;
    Bounds.Mins.Y  = -0.001f;
    Bounds.Mins.Z  = -halfHeight;
    Bounds.Maxs.X  = halfWidth;
    Bounds.Maxs.Y  = 0.001f;
    Bounds.Maxs.Z  = halfHeight;
}

void CreatePlaneMeshXY(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Width, float Height, Float2 const& TexCoordScale)
{
    Vertices.ResizeInvalidate(4);
    Indices.ResizeInvalidate(6);

    const float halfWidth  = Width * 0.5f;
    const float halfHeight = Height * 0.5f;

    const MeshVertex Verts[4] = {
        MakeMeshVertex(Float3(-halfWidth, -halfHeight, 0), Float2(0, TexCoordScale.Y), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1)),
        MakeMeshVertex(Float3(halfWidth, -halfHeight, 0), Float2(TexCoordScale.X, TexCoordScale.Y), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1)),
        MakeMeshVertex(Float3(halfWidth, halfHeight, 0), Float2(TexCoordScale.X, 0), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1)),
        MakeMeshVertex(Float3(-halfWidth, halfHeight, 0), Float2(0, 0), Float3(0, 0, 0), 1.0f, Float3(0, 0, 1))};

    Core::Memcpy(Vertices.ToPtr(), &Verts, 4 * sizeof(MeshVertex));

    constexpr unsigned int indices[6] = {0, 1, 2, 2, 3, 0};
    Core::Memcpy(Indices.ToPtr(), &indices, sizeof(indices));

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());

    Bounds.Mins.X  = -halfWidth;
    Bounds.Mins.Y  = -halfHeight;
    Bounds.Mins.Z  = -0.001f;
    Bounds.Maxs.X  = halfWidth;
    Bounds.Maxs.Y  = halfHeight;
    Bounds.Maxs.Z  = 0.001f;
}

void CreatePatchMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Corner00, Float3 const& Corner10, Float3 const& Corner01, Float3 const& Corner11, float TexCoordScale, bool bTwoSided, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 2);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 2);

    const float scaleX = 1.0f / (float)(NumHorizontalSubdivs - 1);
    const float scaleY = 1.0f / (float)(NumVerticalSubdivs - 1);

    const int vertexCount = NumHorizontalSubdivs * NumVerticalSubdivs;
    const int indexCount  = (NumHorizontalSubdivs - 1) * (NumVerticalSubdivs - 1) * 6;

    Float3 normal = Math::Cross(Corner10 - Corner00, Corner01 - Corner00).Normalized();

    Half normalNative[3];
    normalNative[0] = normal.X;
    normalNative[1] = normal.Y;
    normalNative[2] = normal.Z;

    Vertices.ResizeInvalidate(bTwoSided ? vertexCount << 1 : vertexCount);
    Indices.ResizeInvalidate(bTwoSided ? indexCount << 1 : indexCount);

    MeshVertex*  pVert    = Vertices.ToPtr();
    unsigned int* pIndices = Indices.ToPtr();

    for (int y = 0; y < NumVerticalSubdivs; ++y)
    {
        const float  lerpY = y * scaleY;
        const Float3 py0   = Math::Lerp(Corner00, Corner01, lerpY);
        const Float3 py1   = Math::Lerp(Corner10, Corner11, lerpY);
        const float  ty    = lerpY * TexCoordScale;

        for (int x = 0; x < NumHorizontalSubdivs; ++x)
        {
            const float lerpX = x * scaleX;

            pVert->Position = Math::Lerp(py0, py1, lerpX);
            pVert->SetTexCoord(lerpX * TexCoordScale, ty);
            pVert->SetNormal(normalNative[0], normalNative[1], normalNative[2]);

            ++pVert;
        }
    }

    if (bTwoSided)
    {
        normal = -normal;

        normalNative[0] = normal.X;
        normalNative[1] = normal.Y;
        normalNative[2] = normal.Z;

        for (int y = 0; y < NumVerticalSubdivs; ++y)
        {
            const float  lerpY = y * scaleY;
            const Float3 py0   = Math::Lerp(Corner00, Corner01, lerpY);
            const Float3 py1   = Math::Lerp(Corner10, Corner11, lerpY);
            const float  ty    = lerpY * TexCoordScale;

            for (int x = 0; x < NumHorizontalSubdivs; ++x)
            {
                const float lerpX = x * scaleX;

                pVert->Position = Math::Lerp(py0, py1, lerpX);
                pVert->SetTexCoord(lerpX * TexCoordScale, ty);
                pVert->SetNormal(normalNative[0], normalNative[1], normalNative[2]);

                ++pVert;
            }
        }
    }

    for (int y = 0; y < NumVerticalSubdivs; ++y)
    {

        const int index0 = y * NumHorizontalSubdivs;
        const int index1 = (y + 1) * NumHorizontalSubdivs;

        for (int x = 0; x < NumHorizontalSubdivs; ++x)
        {
            const int quad00 = index0 + x;
            const int quad01 = index0 + x + 1;
            const int quad10 = index1 + x;
            const int quad11 = index1 + x + 1;

            if ((x + 1) < NumHorizontalSubdivs && (y + 1) < NumVerticalSubdivs)
            {
                *pIndices++ = quad00;
                *pIndices++ = quad10;
                *pIndices++ = quad11;
                *pIndices++ = quad11;
                *pIndices++ = quad01;
                *pIndices++ = quad00;
            }
        }
    }

    if (bTwoSided)
    {
        for (int y = 0; y < NumVerticalSubdivs; ++y)
        {

            const int index0 = vertexCount + y * NumHorizontalSubdivs;
            const int index1 = vertexCount + (y + 1) * NumHorizontalSubdivs;

            for (int x = 0; x < NumHorizontalSubdivs; ++x)
            {
                const int quad00 = index0 + x;
                const int quad01 = index0 + x + 1;
                const int quad10 = index1 + x;
                const int quad11 = index1 + x + 1;

                if ((x + 1) < NumHorizontalSubdivs && (y + 1) < NumVerticalSubdivs)
                {
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

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());

    Bounds.Clear();
    Bounds.AddPoint(Corner00);
    Bounds.AddPoint(Corner01);
    Bounds.AddPoint(Corner10);
    Bounds.AddPoint(Corner11);
}

void CreateCylinderMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs)
{
    int          i, j;
    float        angle;
    unsigned int quad[4];

    NumSubdivs = Math::Max(NumSubdivs, 4);

    const float invSubdivs = 1.0f / NumSubdivs;
    const float angleStep  = Math::_2PI * invSubdivs;
    const float halfHeight = Height * 0.5f;

    Vertices.ResizeInvalidate(6 * (NumSubdivs + 1));
    Indices.ResizeInvalidate(3 * NumSubdivs * 6);

    Bounds.Mins.X  = -Radius;
    Bounds.Mins.Z = -Radius;
    Bounds.Mins.Y  = -halfHeight;

    Bounds.Maxs.X  = Radius;
    Bounds.Maxs.Z = Radius;
    Bounds.Maxs.Y  = halfHeight;

    MeshVertex* pVerts = Vertices.ToPtr();

    int firstVertex = 0;

    Half pos  = 1.0f;
    Half neg  = -1.0f;
    Half zero = 0.0f;

    for (j = 0; j <= NumSubdivs; j++)
    {
        pVerts[firstVertex + j].Position = Float3(0.0f, -halfHeight, 0.0f);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, pos, zero);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0; j <= NumSubdivs; j++)
    {
        pVerts[firstVertex + j].Position = Float3(0.0f, halfHeight, 0.0f);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, pos, zero);
    }
    firstVertex += NumSubdivs + 1;

    // generate indices

    unsigned int* pIndices = Indices.ToPtr();

    firstVertex = 0;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < NumSubdivs; j++)
        {
            quad[3] = firstVertex + j;
            quad[2] = firstVertex + j + 1;
            quad[1] = firstVertex + j + 1 + (NumSubdivs + 1);
            quad[0] = firstVertex + j + (NumSubdivs + 1);

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
        firstVertex += (NumSubdivs + 1) * 2;
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateConeMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumSubdivs)
{
    int          i, j;
    float        angle;
    unsigned int quad[4];

    NumSubdivs = Math::Max(NumSubdivs, 4);

    const float invSubdivs = 1.0f / NumSubdivs;
    const float angleStep  = Math::_2PI * invSubdivs;
    const float halfHeight = Height * 0.5f;

    Vertices.ResizeInvalidate(4 * (NumSubdivs + 1));
    Indices.ResizeInvalidate(2 * NumSubdivs * 6);

    Bounds.Mins.X  = -Radius;
    Bounds.Mins.Z = -Radius;
    Bounds.Mins.Y  = -halfHeight;

    Bounds.Maxs.X  = Radius;
    Bounds.Maxs.Z = Radius;
    Bounds.Maxs.Y  = halfHeight;

    Half neg = -1.0f;
    Half zero = 0.0f;

    MeshVertex* pVerts = Vertices.ToPtr();

    int firstVertex = 0;

    for (j = 0; j <= NumSubdivs; j++)
    {
        pVerts[firstVertex + j].Position = Float3(0, -halfHeight, 0);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 0.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(zero, neg, zero);
        ;
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(Radius * c, -halfHeight, Radius * s);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 1.0f) * TexCoordScale);
        pVerts[firstVertex + j].SetNormal(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    Float3 vx;
    Float3 vy(0, halfHeight, 0);
    Float3 v;
    for (j = 0, angle = 0; j <= NumSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        pVerts[firstVertex + j].Position = Float3(0, halfHeight, 0);
        pVerts[firstVertex + j].SetTexCoord(Float2(1.0f - j * invSubdivs, 0.0f) * TexCoordScale);

        vx = Float3(c, 0.0f, s);
        v  = vy - vx;
        pVerts[firstVertex + j].SetNormal(Math::Cross(Math::Cross(v, vx), v).Normalized());

        angle += angleStep;
    }
    firstVertex += NumSubdivs + 1;

    HK_ASSERT(firstVertex == Vertices.Size());

    // generate indices

    unsigned int* pIndices = Indices.ToPtr();

    firstVertex = 0;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < NumSubdivs; j++)
        {
            quad[3] = firstVertex + j;
            quad[2] = firstVertex + j + 1;
            quad[1] = firstVertex + j + 1 + (NumSubdivs + 1);
            quad[0] = firstVertex + j + (NumSubdivs + 1);

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
        firstVertex += (NumSubdivs + 1) * 2;
    }

    HK_ASSERT(pIndices == Indices.ToPtr() + Indices.Size());

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateCapsuleMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float Height, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs)
{
    int          x, y, tcY;
    float        verticalAngle, horizontalAngle;
    const float  halfHeight = Height * 0.5f;
    unsigned int quad[4];

    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 4);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 4);

    const int halfVerticalSubdivs = NumVerticalSubdivs >> 1;

    Vertices.ResizeInvalidate((NumHorizontalSubdivs + 1) * (NumVerticalSubdivs + 1) * 2);
    Indices.ResizeInvalidate(NumHorizontalSubdivs * (NumVerticalSubdivs + 1) * 6);

    Bounds.Mins.X = Bounds.Mins.Z = -Radius;
    Bounds.Mins.Y                 = -Radius - halfHeight;
    Bounds.Maxs.X = Bounds.Maxs.Z = Radius;
    Bounds.Maxs.Y                 = Radius + halfHeight;

    MeshVertex* pVert = Vertices.ToPtr();

    const float verticalStep    = Math::_PI / NumVerticalSubdivs;
    const float horizontalStep  = Math::_2PI / NumHorizontalSubdivs;
    const float verticalScale   = 1.0f / (NumVerticalSubdivs + 1);
    const float horizontalScale = 1.0f / NumHorizontalSubdivs;

    tcY = 0;

    for (y = 0, verticalAngle = -Math::_HALF_PI; y <= halfVerticalSubdivs; y++, tcY++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        const float posY    = scaledH - halfHeight;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->SetTexCoord((1.0f - static_cast<float>(x) * horizontalScale) * TexCoordScale,
                               (1.0f - static_cast<float>(tcY) * verticalScale) * TexCoordScale);
            pVert->SetNormal(r * c, h, r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for (y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++, tcY++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        const float posY    = scaledH + halfHeight;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->SetTexCoord((1.0f - static_cast<float>(x) * horizontalScale) * TexCoordScale,
                               (1.0f - static_cast<float>(tcY) * verticalScale) * TexCoordScale);
            pVert->SetNormal(r * c, h, r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = Indices.ToPtr();
    for (y = 0; y <= NumVerticalSubdivs; y++)
    {
        const int y2 = y + 1;
        for (x = 0; x < NumHorizontalSubdivs; x++)
        {
            const int x2 = x + 1;

            quad[0] = y * (NumHorizontalSubdivs + 1) + x;
            quad[1] = y2 * (NumHorizontalSubdivs + 1) + x;
            quad[2] = y2 * (NumHorizontalSubdivs + 1) + x2;
            quad[3] = y * (NumHorizontalSubdivs + 1) + x2;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateSkyboxMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, Float3 const& Extents, float TexCoordScale)
{
    constexpr unsigned int indices[6 * 6] =
        {
            0, 1, 2, 2, 3, 0,                                                 // front face
            4, 5, 6, 6, 7, 4,                                                 // back face
            5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face

            1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
            3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
            1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
        };

    Vertices.ResizeInvalidate(24);
    Indices.ResizeInvalidate(36);

    for (int i = 0; i < 36; i += 3)
    {
        Indices[i]      = indices[i + 2];
        Indices[i + 1]  = indices[i + 1];
        Indices[i + 2]  = indices[i];
    }

    const Float3 halfSize = Extents * 0.5f;

    Bounds.Mins  = -halfSize;
    Bounds.Maxs = halfSize;

    Float3 const& mins = Bounds.Mins;
    Float3 const& maxs = Bounds.Maxs;

    MeshVertex* pVerts = Vertices.ToPtr();

    Half     zero = 0.0f;
    Half     pos  = 1.0f;
    Half     neg  = -1.0f;

    pVerts[0 + 8 * 0].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[0 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[1 + 8 * 0].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[1 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[2 + 8 * 0].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[2 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[3 + 8 * 0].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 0].SetNormal(zero, zero, neg);
    pVerts[3 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[4 + 8 * 0].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[4 + 8 * 0].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[5 + 8 * 0].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[5 + 8 * 0].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[6 + 8 * 0].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[6 + 8 * 0].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[7 + 8 * 0].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 0].SetNormal(zero, zero, pos);
    pVerts[7 + 8 * 0].SetTexCoord(Float2(0, 0) * TexCoordScale);


    pVerts[0 + 8 * 1].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[0 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[1 + 8 * 1].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[1 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 1].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[2 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[3 + 8 * 1].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[3 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[4 + 8 * 1].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[4 + 8 * 1].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[5 + 8 * 1].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[5 + 8 * 1].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[6 + 8 * 1].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 1].SetNormal(pos, zero, zero);
    pVerts[6 + 8 * 1].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[7 + 8 * 1].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 1].SetNormal(neg, zero, zero);
    pVerts[7 + 8 * 1].SetTexCoord(Float2(1, 0) * TexCoordScale);


    pVerts[1 + 8 * 2].Position = Float3(maxs.X, mins.Y, maxs.Z); // 1
    pVerts[1 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[1 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[0 + 8 * 2].Position = Float3(mins.X, mins.Y, maxs.Z); // 0
    pVerts[0 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[0 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    pVerts[5 + 8 * 2].Position = Float3(mins.X, mins.Y, mins.Z); // 5
    pVerts[5 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[5 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[4 + 8 * 2].Position = Float3(maxs.X, mins.Y, mins.Z); // 4
    pVerts[4 + 8 * 2].SetNormal(zero, pos, zero);
    pVerts[4 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);


    pVerts[3 + 8 * 2].Position = Float3(mins.X, maxs.Y, maxs.Z); // 3
    pVerts[3 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[3 + 8 * 2].SetTexCoord(Float2(0, 1) * TexCoordScale);

    pVerts[2 + 8 * 2].Position = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    pVerts[2 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[2 + 8 * 2].SetTexCoord(Float2(1, 1) * TexCoordScale);

    pVerts[7 + 8 * 2].Position = Float3(maxs.X, maxs.Y, mins.Z); // 7
    pVerts[7 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[7 + 8 * 2].SetTexCoord(Float2(1, 0) * TexCoordScale);

    pVerts[6 + 8 * 2].Position = Float3(mins.X, maxs.Y, mins.Z); // 6
    pVerts[6 + 8 * 2].SetNormal(zero, neg, zero);
    pVerts[6 + 8 * 2].SetTexCoord(Float2(0, 0) * TexCoordScale);

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

void CreateSkydomeMesh(TVertexBufferCPU<MeshVertex>& Vertices, TIndexBufferCPU<unsigned int>& Indices, BvAxisAlignedBox& Bounds, float Radius, float TexCoordScale, int NumVerticalSubdivs, int NumHorizontalSubdivs, bool bHemisphere)
{
    int          x, y;
    float        verticalAngle, horizontalAngle;
    unsigned int quad[4];

    NumVerticalSubdivs    = Math::Max(NumVerticalSubdivs, 4);
    NumHorizontalSubdivs = Math::Max(NumHorizontalSubdivs, 4);

    Vertices.ResizeInvalidate((NumHorizontalSubdivs + 1) * (NumVerticalSubdivs + 1));
    Indices.ResizeInvalidate(NumHorizontalSubdivs * NumVerticalSubdivs * 6);

    Bounds.Mins.X = Bounds.Mins.Y = Bounds.Mins.Z = -Radius;
    Bounds.Maxs.X = Bounds.Maxs.Y = Bounds.Maxs.Z = Radius;

    MeshVertex* pVert = Vertices.ToPtr();

    const float verticalRange   = bHemisphere ? Math::_HALF_PI : Math::_PI;
    const float verticalStep    = verticalRange / NumVerticalSubdivs;
    const float horizontalStep  = Math::_2PI / NumHorizontalSubdivs;
    const float verticalScale   = 1.0f / NumVerticalSubdivs;
    const float horizontalScale = 1.0f / NumHorizontalSubdivs;

    for (y = 0, verticalAngle = bHemisphere ? 0 : -Math::_HALF_PI; y <= NumVerticalSubdivs; y++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        const float scaledH = h * Radius;
        const float scaledR = r * Radius;
        for (x = 0, horizontalAngle = 0; x <= NumHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pVert->Position = Float3(scaledR * c, scaledH, scaledR * s);
            pVert->SetTexCoord(Float2(1.0f - static_cast<float>(x) * horizontalScale, 1.0f - static_cast<float>(y) * verticalScale) * TexCoordScale);
            pVert->SetNormal(-r * c, -h, -r * s);
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = Indices.ToPtr();
    for (y = 0; y < NumVerticalSubdivs; y++)
    {
        const int y2 = y + 1;
        for (x = 0; x < NumHorizontalSubdivs; x++)
        {
            const int x2 = x + 1;

            quad[0] = y * (NumHorizontalSubdivs + 1) + x;
            quad[1] = y * (NumHorizontalSubdivs + 1) + x2;
            quad[2] = y2 * (NumHorizontalSubdivs + 1) + x2;
            quad[3] = y2 * (NumHorizontalSubdivs + 1) + x;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(Vertices.ToPtr(), Vertices.Size(), Indices.ToPtr(), Indices.Size());
}

HK_NAMESPACE_END
