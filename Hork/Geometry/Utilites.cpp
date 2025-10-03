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

#include "Utilites.h"
#include "TangentSpace.h"

HK_NAMESPACE_BEGIN

namespace Geometry
{

void CreateBoxMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, Float3 const& extents, float texCoordScale)
{
    constexpr unsigned int _indices[6 * 6] =
        {
            0, 1, 2, 2, 3, 0,                                                 // front face
            4, 5, 6, 6, 7, 4,                                                 // back face
            5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face
            1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
            3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
            1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
        };

    positions.Resize(24);
    texCoords.Resize(24);
    normals.Resize(24);
    tangents.Resize(24);
    indices.Resize(36);

    Core::Memcpy(indices.ToPtr(), _indices, sizeof(_indices));

    Float3 halfSize = extents * 0.5f;

    bounds.Mins  = -halfSize;
    bounds.Maxs = halfSize;

    Float3 const& mins = bounds.Mins;
    Float3 const& maxs = bounds.Maxs;

    positions[0 + 8 * 0] = Float3(mins.X, mins.Y, maxs.Z); // 0
    normals[0 + 8 * 0] = Float3(0, 0, 1);
    texCoords[0 + 8 * 0] = Float2(Float2(0, 1) * texCoordScale);

    positions[1 + 8 * 0] = Float3(maxs.X, mins.Y, maxs.Z); // 1
    normals[1 + 8 * 0] = Float3(0, 0, 1);
    texCoords[1 + 8 * 0] = Float2(Float2(1, 1) * texCoordScale);

    positions[2 + 8 * 0] = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    normals[2 + 8 * 0] = Float3(0, 0, 1);
    texCoords[2 + 8 * 0] = Float2(Float2(1, 0) * texCoordScale);

    positions[3 + 8 * 0] = Float3(mins.X, maxs.Y, maxs.Z); // 3
    normals[3 + 8 * 0] = Float3(0, 0, 1);
    texCoords[3 + 8 * 0] = Float2(Float2(0, 0) * texCoordScale);


    positions[4 + 8 * 0] = Float3(maxs.X, mins.Y, mins.Z); // 4
    normals[4 + 8 * 0] = Float3(0, 0, -1);
    texCoords[4 + 8 * 0] = Float2(Float2(0, 1) * texCoordScale);

    positions[5 + 8 * 0] = Float3(mins.X, mins.Y, mins.Z); // 5
    normals[5 + 8 * 0] = Float3(0, 0, -1);
    texCoords[5 + 8 * 0] = Float2(Float2(1, 1) * texCoordScale);

    positions[6 + 8 * 0] = Float3(mins.X, maxs.Y, mins.Z); // 6
    normals[6 + 8 * 0] = Float3(0, 0, -1);
    texCoords[6 + 8 * 0] = Float2(Float2(1, 0) * texCoordScale);

    positions[7 + 8 * 0] = Float3(maxs.X, maxs.Y, mins.Z); // 7
    normals[7 + 8 * 0] = Float3(0, 0, -1);
    texCoords[7 + 8 * 0] = Float2(Float2(0, 0) * texCoordScale);


    positions[0 + 8 * 1] = Float3(mins.X, mins.Y, maxs.Z); // 0
    normals[0 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[0 + 8 * 1] = Float2(Float2(1, 1) * texCoordScale);

    positions[1 + 8 * 1] = Float3(maxs.X, mins.Y, maxs.Z); // 1
    normals[1 + 8 * 1] = Float3(1, 0, 0);
    texCoords[1 + 8 * 1] = Float2(Float2(0, 1) * texCoordScale);

    positions[2 + 8 * 1] = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    normals[2 + 8 * 1] = Float3(1, 0, 0);
    texCoords[2 + 8 * 1] = Float2(Float2(0, 0) * texCoordScale);

    positions[3 + 8 * 1] = Float3(mins.X, maxs.Y, maxs.Z); // 3
    normals[3 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[3 + 8 * 1] = Float2(Float2(1, 0) * texCoordScale);


    positions[4 + 8 * 1] = Float3(maxs.X, mins.Y, mins.Z); // 4
    normals[4 + 8 * 1] = Float3(1, 0, 0);
    texCoords[4 + 8 * 1] = Float2(Float2(1, 1) * texCoordScale);

    positions[5 + 8 * 1] = Float3(mins.X, mins.Y, mins.Z); // 5
    normals[5 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[5 + 8 * 1] = Float2(Float2(0, 1) * texCoordScale);

    positions[6 + 8 * 1] = Float3(mins.X, maxs.Y, mins.Z); // 6
    normals[6 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[6 + 8 * 1] = Float2(Float2(0, 0) * texCoordScale);

    positions[7 + 8 * 1] = Float3(maxs.X, maxs.Y, mins.Z); // 7
    normals[7 + 8 * 1] = Float3(1, 0, 0);
    texCoords[7 + 8 * 1] = Float2(Float2(1, 0) * texCoordScale);


    positions[1 + 8 * 2] = Float3(maxs.X, mins.Y, maxs.Z); // 1
    normals[1 + 8 * 2] = Float3(0, -1, 0);
    texCoords[1 + 8 * 2] = Float2(Float2(1, 0) * texCoordScale);

    positions[0 + 8 * 2] = Float3(mins.X, mins.Y, maxs.Z); // 0
    normals[0 + 8 * 2] = Float3(0, -1, 0);
    texCoords[0 + 8 * 2] = Float2(Float2(0, 0) * texCoordScale);

    positions[5 + 8 * 2] = Float3(mins.X, mins.Y, mins.Z); // 5
    normals[5 + 8 * 2] = Float3(0, -1, 0);
    texCoords[5 + 8 * 2] = Float2(Float2(0, 1) * texCoordScale);

    positions[4 + 8 * 2] = Float3(maxs.X, mins.Y, mins.Z); // 4
    normals[4 + 8 * 2] = Float3(0, -1, 0);
    texCoords[4 + 8 * 2] = Float2(Float2(1, 1) * texCoordScale);


    positions[3 + 8 * 2] = Float3(mins.X, maxs.Y, maxs.Z); // 3
    normals[3 + 8 * 2] = Float3(0, 1, 0);
    texCoords[3 + 8 * 2] = Float2(Float2(0, 1) * texCoordScale);

    positions[2 + 8 * 2] = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    normals[2 + 8 * 2] = Float3(0, 1, 0);
    texCoords[2 + 8 * 2] = Float2(Float2(1, 1) * texCoordScale);

    positions[7 + 8 * 2] = Float3(maxs.X, maxs.Y, mins.Z); // 7
    normals[7 + 8 * 2] = Float3(0, 1, 0);
    texCoords[7 + 8 * 2] = Float2(Float2(1, 0) * texCoordScale);

    positions[6 + 8 * 2] = Float3(mins.X, maxs.Y, mins.Z); // 6
    normals[6 + 8 * 2] = Float3(0, 1, 0);
    texCoords[6 + 8 * 2] = Float2(Float2(0, 0) * texCoordScale);

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

void CreateSphereMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs)
{
    int          x, y;
    float        verticalAngle, horizontalAngle;
    unsigned int quad[4];

    numVerticalSubdivs    = Math::Max(numVerticalSubdivs, 4);
    numHorizontalSubdivs = Math::Max(numHorizontalSubdivs, 4);

    int vertexCount = (numHorizontalSubdivs + 1) * (numVerticalSubdivs + 1);

    positions.Resize(vertexCount);
    texCoords.Resize(vertexCount);
    normals.Resize(vertexCount);
    tangents.Resize(vertexCount);

    indices.Resize(numHorizontalSubdivs * numVerticalSubdivs * 6);

    bounds.Mins.X = bounds.Mins.Y = bounds.Mins.Z = -radius;
    bounds.Maxs.X = bounds.Maxs.Y = bounds.Maxs.Z = radius;

    Float3* pPos = positions.ToPtr();
    Float2* pTC = texCoords.ToPtr();
    Float3* pNormal = normals.ToPtr();

    float verticalStep    = Math::_PI / numVerticalSubdivs;
    float horizontalStep  = Math::_2PI / numHorizontalSubdivs;
    float verticalScale   = 1.0f / numVerticalSubdivs;
    float horizontalScale = 1.0f / numHorizontalSubdivs;

    for (y = 0, verticalAngle = -Math::_HALF_PI; y <= numVerticalSubdivs; y++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        float scaledH = h * radius;
        float scaledR = r * radius;
        for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            *pPos++ = Float3(scaledR * c, scaledH, scaledR * s);
            *pTC++ = Float2(1.0f - static_cast<float>(x) * horizontalScale, 1.0f - static_cast<float>(y) * verticalScale) * texCoordScale;
            *pNormal++ = Float3(r * c, h, r * s);
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = indices.ToPtr();
    for (y = 0; y < numVerticalSubdivs; y++)
    {
        int y2 = y + 1;
        for (x = 0; x < numHorizontalSubdivs; x++)
        {
            int x2 = x + 1;

            quad[0] = y * (numHorizontalSubdivs + 1) + x;
            quad[1] = y2 * (numHorizontalSubdivs + 1) + x;
            quad[2] = y2 * (numHorizontalSubdivs + 1) + x2;
            quad[3] = y * (numHorizontalSubdivs + 1) + x2;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

void CreatePlaneMeshXZ(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float width, float height, Float2 const& texCoordScale)
{
    positions.Resize(4);
    texCoords.Resize(4);
    normals.Resize(4);
    tangents.Resize(4);

    indices.Resize(6);

    float halfWidth  = width * 0.5f;
    float halfHeight = height * 0.5f;

    positions[0] = Float3(-halfWidth, 0, -halfHeight);
    texCoords[0] = Float2(0, 0);
    tangents[0] = Float4(0, 0, 1, 1.0f);
    normals[0] = Float3(0, 1, 0);
    positions[1] = Float3(-halfWidth, 0, halfHeight);
    texCoords[1] = Float2(0, texCoordScale.Y);
    tangents[1] = Float4(0, 0, 1, 1.0f);
    normals[1] = Float3(0, 1, 0);
    positions[2] = Float3(halfWidth, 0, halfHeight);
    texCoords[2] = Float2(texCoordScale.X, texCoordScale.Y);
    tangents[2] = Float4(0, 0, 1, 1.0f);
    normals[2] = Float3(0, 1, 0);
    positions[3] = Float3(halfWidth, 0, -halfHeight);
    texCoords[3] = Float2(texCoordScale.X, 0);
    tangents[3] = Float4(0, 0, 1, 1.0f);
    normals[3] = Float3(0, 1, 0);

    constexpr unsigned int _indices[6] = {0, 1, 2, 2, 3, 0};
    Core::Memcpy(indices.ToPtr(), &_indices, sizeof(_indices));

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());

    bounds.Mins.X  = -halfWidth;
    bounds.Mins.Y  = -0.001f;
    bounds.Mins.Z  = -halfHeight;
    bounds.Maxs.X  = halfWidth;
    bounds.Maxs.Y  = 0.001f;
    bounds.Maxs.Z  = halfHeight;
}

void CreatePlaneMeshXY(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float width, float height, Float2 const& texCoordScale)
{
    positions.Resize(4);
    texCoords.Resize(4);
    normals.Resize(4);
    tangents.Resize(4);

    indices.Resize(6);

    float halfWidth  = width * 0.5f;
    float halfHeight = height * 0.5f;

    positions[0] = Float3(-halfWidth, -halfHeight, 0);
    texCoords[0] = Float2(0, texCoordScale.Y);
    tangents[0] = Float4(0, 0, 0, 1.0f);
    normals[0] = Float3(0, 0, 1);
    positions[1] = Float3(halfWidth, -halfHeight, 0);
    texCoords[1] = Float2(texCoordScale.X, texCoordScale.Y);
    tangents[1] = Float4(0, 0, 0, 1.0f);
    normals[1] = Float3(0, 0, 1);
    positions[2] = Float3(halfWidth, halfHeight, 0);
    texCoords[2] = Float2(texCoordScale.X, 0);
    tangents[2] = Float4(0, 0, 0, 1.0f);
    normals[2] = Float3(0, 0, 1);
    positions[3] = Float3(-halfWidth, halfHeight, 0);
    texCoords[3] = Float2(0, 0);
    tangents[3] = Float4(0, 0, 0, 1.0f);
    normals[3] = Float3(0, 0, 1);

    constexpr unsigned int _indices[6] = {0, 1, 2, 2, 3, 0};
    Core::Memcpy(indices.ToPtr(), &_indices, sizeof(_indices));

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());

    bounds.Mins.X  = -halfWidth;
    bounds.Mins.Y  = -halfHeight;
    bounds.Mins.Z  = -0.001f;
    bounds.Maxs.X  = halfWidth;
    bounds.Maxs.Y  = halfHeight;
    bounds.Maxs.Z  = 0.001f;
}

void CreatePatchMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, Float3 const& corner00, Float3 const& corner10, Float3 const& corner01, Float3 const& corner11, float texCoordScale, bool isTwoSided, int numVerticalSubdivs, int numHorizontalSubdivs)
{
    numVerticalSubdivs    = Math::Max(numVerticalSubdivs, 2);
    numHorizontalSubdivs = Math::Max(numHorizontalSubdivs, 2);

    float scaleX = 1.0f / (float)(numHorizontalSubdivs - 1);
    float scaleY = 1.0f / (float)(numVerticalSubdivs - 1);

    int vertexCount = numHorizontalSubdivs * numVerticalSubdivs;
    int indexCount  = (numHorizontalSubdivs - 1) * (numVerticalSubdivs - 1) * 6;

    Float3 normal = Math::Cross(corner10 - corner00, corner01 - corner00).Normalized();

    Half normalNative[3];
    normalNative[0] = normal.X;
    normalNative[1] = normal.Y;
    normalNative[2] = normal.Z;

    int totalVerts = isTwoSided ? vertexCount << 1 : vertexCount;
    positions.Resize(totalVerts);
    texCoords.Resize(totalVerts);
    normals.Resize(totalVerts);
    tangents.Resize(totalVerts);
    indices.Resize(isTwoSided ? indexCount << 1 : indexCount);

    Float3* pPos = positions.ToPtr();
    Float2* pTC = texCoords.ToPtr();
    Float3* pNormal = normals.ToPtr();
    unsigned int* pIndices = indices.ToPtr();

    for (int y = 0; y < numVerticalSubdivs; ++y)
    {
        float  lerpY = y * scaleY;
        Float3 py0   = Math::Lerp(corner00, corner01, lerpY);
        Float3 py1   = Math::Lerp(corner10, corner11, lerpY);
        float  ty    = lerpY * texCoordScale;

        for (int x = 0; x < numHorizontalSubdivs; ++x)
        {
            float lerpX = x * scaleX;

            *pPos++ = Math::Lerp(py0, py1, lerpX);
            *pTC++ = Float2(lerpX * texCoordScale, ty);
            *pNormal++ = Float3(normalNative[0], normalNative[1], normalNative[2]);
        }
    }

    if (isTwoSided)
    {
        normal = -normal;

        normalNative[0] = normal.X;
        normalNative[1] = normal.Y;
        normalNative[2] = normal.Z;

        for (int y = 0; y < numVerticalSubdivs; ++y)
        {
            float  lerpY = y * scaleY;
            Float3 py0   = Math::Lerp(corner00, corner01, lerpY);
            Float3 py1   = Math::Lerp(corner10, corner11, lerpY);
            float  ty    = lerpY * texCoordScale;

            for (int x = 0; x < numHorizontalSubdivs; ++x)
            {
                float lerpX = x * scaleX;

                *pPos++ = Math::Lerp(py0, py1, lerpX);
                *pTC++ = Float2(lerpX * texCoordScale, ty);
                *pNormal++ = Float3(normalNative[0], normalNative[1], normalNative[2]);
            }
        }
    }

    for (int y = 0; y < numVerticalSubdivs; ++y)
    {

        int index0 = y * numHorizontalSubdivs;
        int index1 = (y + 1) * numHorizontalSubdivs;

        for (int x = 0; x < numHorizontalSubdivs; ++x)
        {
            int quad00 = index0 + x;
            int quad01 = index0 + x + 1;
            int quad10 = index1 + x;
            int quad11 = index1 + x + 1;

            if ((x + 1) < numHorizontalSubdivs && (y + 1) < numVerticalSubdivs)
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

    if (isTwoSided)
    {
        for (int y = 0; y < numVerticalSubdivs; ++y)
        {

            int index0 = vertexCount + y * numHorizontalSubdivs;
            int index1 = vertexCount + (y + 1) * numHorizontalSubdivs;

            for (int x = 0; x < numHorizontalSubdivs; ++x)
            {
                int quad00 = index0 + x;
                int quad01 = index0 + x + 1;
                int quad10 = index1 + x;
                int quad11 = index1 + x + 1;

                if ((x + 1) < numHorizontalSubdivs && (y + 1) < numVerticalSubdivs)
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

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());

    bounds.Clear();
    bounds.AddPoint(corner00);
    bounds.AddPoint(corner01);
    bounds.AddPoint(corner10);
    bounds.AddPoint(corner11);
}

void CreateCylinderMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float height, float texCoordScale, int numSubdivs)
{
    int          i, j;
    float        angle;
    unsigned int quad[4];

    numSubdivs = Math::Max(numSubdivs, 4);

    float invSubdivs = 1.0f / numSubdivs;
    float angleStep  = Math::_2PI * invSubdivs;
    float halfHeight = height * 0.5f;

    int totalVerts = 6 * (numSubdivs + 1);
    positions.Resize(totalVerts);
    texCoords.Resize(totalVerts);
    normals.Resize(totalVerts);
    tangents.Resize(totalVerts);
    indices.Resize(3 * numSubdivs * 6);

    bounds.Mins.X  = -radius;
    bounds.Mins.Z = -radius;
    bounds.Mins.Y  = -halfHeight;

    bounds.Maxs.X  = radius;
    bounds.Maxs.Z = radius;
    bounds.Maxs.Y  = halfHeight;

    int firstVertex = 0;

    for (j = 0; j <= numSubdivs; j++)
    {
        positions[firstVertex + j] = Float3(0.0f, -halfHeight, 0.0f);
        texCoords[firstVertex + j] = Float2(j * invSubdivs, 0.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(0, -1, 0);
    }
    firstVertex += numSubdivs + 1;

    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(radius * c, -halfHeight, radius * s);
        texCoords[firstVertex + j] = Float2(j * invSubdivs, 1.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(0, -1, 0);
        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(radius * c, -halfHeight, radius * s);
        texCoords[firstVertex + j] = Float2(1.0f - j * invSubdivs, 1.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(radius * c, halfHeight, radius * s);
        texCoords[firstVertex + j] = Float2(1.0f - j * invSubdivs, 0.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(radius * c, halfHeight, radius * s);
        texCoords[firstVertex + j] = Float2(j * invSubdivs, 0.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(0, 1, 0);
        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    for (j = 0; j <= numSubdivs; j++)
    {
        positions[firstVertex + j] = Float3(0.0f, halfHeight, 0.0f);
        texCoords[firstVertex + j] = Float2(j * invSubdivs, 1.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(0, 1, 0);
    }
    firstVertex += numSubdivs + 1;

    // generate indices

    unsigned int* pIndices = indices.ToPtr();

    firstVertex = 0;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < numSubdivs; j++)
        {
            quad[3] = firstVertex + j;
            quad[2] = firstVertex + j + 1;
            quad[1] = firstVertex + j + 1 + (numSubdivs + 1);
            quad[0] = firstVertex + j + (numSubdivs + 1);

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
        firstVertex += (numSubdivs + 1) * 2;
    }

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

void CreateConeMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float height, float texCoordScale, int numSubdivs)
{
    int          i, j;
    float        angle;
    unsigned int quad[4];

    numSubdivs = Math::Max(numSubdivs, 4);

    float invSubdivs = 1.0f / numSubdivs;
    float angleStep  = Math::_2PI * invSubdivs;
    float halfHeight = height * 0.5f;

    int totalVerts = 4 * (numSubdivs + 1);

    positions.Resize(totalVerts);
    texCoords.Resize(totalVerts);
    normals.Resize(totalVerts);
    tangents.Resize(totalVerts);
    indices.Resize(2 * numSubdivs * 6);

    bounds.Mins.X  = -radius;
    bounds.Mins.Z = -radius;
    bounds.Mins.Y  = -halfHeight;

    bounds.Maxs.X  = radius;
    bounds.Maxs.Z = radius;
    bounds.Maxs.Y  = halfHeight;

    int firstVertex = 0;

    for (j = 0; j <= numSubdivs; j++)
    {
        positions[firstVertex + j] = Float3(0, -halfHeight, 0);
        texCoords[firstVertex + j] = Float2(j * invSubdivs, 0.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(0, -1, 0);
    }
    firstVertex += numSubdivs + 1;

    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(radius * c, -halfHeight, radius * s);
        texCoords[firstVertex + j] = Float2(j * invSubdivs, 1.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(0, -1, 0);
        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(radius * c, -halfHeight, radius * s);
        texCoords[firstVertex + j] = Float2(1.0f - j * invSubdivs, 1.0f) * texCoordScale;
        normals[firstVertex + j] = Float3(c, 0.0f, s);
        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    Float3 vx;
    Float3 vy(0, halfHeight, 0);
    Float3 v;
    for (j = 0, angle = 0; j <= numSubdivs; j++)
    {
        float s, c;
        Math::SinCos(angle, s, c);
        positions[firstVertex + j] = Float3(0, halfHeight, 0);
        texCoords[firstVertex + j] = Float2(1.0f - j * invSubdivs, 0.0f) * texCoordScale;

        vx = Float3(c, 0.0f, s);
        v  = vy - vx;
        normals[firstVertex + j] = Float3(Math::Cross(Math::Cross(v, vx), v).Normalized());

        angle += angleStep;
    }
    firstVertex += numSubdivs + 1;

    HK_ASSERT(firstVertex == totalVerts);

    // generate indices

    unsigned int* pIndices = indices.ToPtr();

    firstVertex = 0;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < numSubdivs; j++)
        {
            quad[3] = firstVertex + j;
            quad[2] = firstVertex + j + 1;
            quad[1] = firstVertex + j + 1 + (numSubdivs + 1);
            quad[0] = firstVertex + j + (numSubdivs + 1);

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
        firstVertex += (numSubdivs + 1) * 2;
    }

    HK_ASSERT(pIndices == indices.ToPtr() + indices.Size());

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

void CreateCapsuleMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float height, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs)
{
    int          x, y, tcY;
    float        verticalAngle, horizontalAngle;
    float        halfHeight = height * 0.5f;
    unsigned int quad[4];

    numVerticalSubdivs    = Math::Max(numVerticalSubdivs, 4);
    numHorizontalSubdivs = Math::Max(numHorizontalSubdivs, 4);

    int halfVerticalSubdivs = numVerticalSubdivs >> 1;

    int totalVerts = (numHorizontalSubdivs + 1) * (numVerticalSubdivs + 1) * 2;
    positions.Resize(totalVerts);
    texCoords.Resize(totalVerts);
    normals.Resize(totalVerts);
    tangents.Resize(totalVerts);

    indices.Resize(numHorizontalSubdivs * (numVerticalSubdivs + 1) * 6);

    bounds.Mins.X = bounds.Mins.Z = -radius;
    bounds.Mins.Y                 = -radius - halfHeight;
    bounds.Maxs.X = bounds.Maxs.Z = radius;
    bounds.Maxs.Y                 = radius + halfHeight;

    Float3* pPos = positions.ToPtr();
    Float2* pTC = texCoords.ToPtr();
    Float3* pNormal = normals.ToPtr();

    float verticalStep    = Math::_PI / numVerticalSubdivs;
    float horizontalStep  = Math::_2PI / numHorizontalSubdivs;
    float verticalScale   = 1.0f / (numVerticalSubdivs + 1);
    float horizontalScale = 1.0f / numHorizontalSubdivs;

    tcY = 0;

    for (y = 0, verticalAngle = -Math::_HALF_PI; y <= halfVerticalSubdivs; y++, tcY++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        float scaledH = h * radius;
        float scaledR = r * radius;
        float posY    = scaledH - halfHeight;
        for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pPos->X = scaledR * c;
            pPos->Y = posY;
            pPos->Z = scaledR * s;
            pPos++;
            *pTC++ = Float2((1.0f - static_cast<float>(x) * horizontalScale) * texCoordScale,
                            (1.0f - static_cast<float>(tcY) * verticalScale) * texCoordScale);
            *pNormal++ = Float3(r * c, h, r * s);
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for (y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++, tcY++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        float scaledH = h * radius;
        float scaledR = r * radius;
        float posY    = scaledH + halfHeight;
        for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            pPos->X = scaledR * c;
            pPos->Y = posY;
            pPos->Z = scaledR * s;
            pPos++;
            *pTC++ = Float2((1.0f - static_cast<float>(x) * horizontalScale) * texCoordScale,
                            (1.0f - static_cast<float>(tcY) * verticalScale) * texCoordScale);
            *pNormal++ = Float3(r * c, h, r * s);
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = indices.ToPtr();
    for (y = 0; y <= numVerticalSubdivs; y++)
    {
        int y2 = y + 1;
        for (x = 0; x < numHorizontalSubdivs; x++)
        {
            int x2 = x + 1;

            quad[0] = y * (numHorizontalSubdivs + 1) + x;
            quad[1] = y2 * (numHorizontalSubdivs + 1) + x;
            quad[2] = y2 * (numHorizontalSubdivs + 1) + x2;
            quad[3] = y * (numHorizontalSubdivs + 1) + x2;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

void CreateSkyboxMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, Float3 const& extents, float texCoordScale)
{
    constexpr unsigned int _indices[6 * 6] =
        {
            0, 1, 2, 2, 3, 0,                                                 // front face
            4, 5, 6, 6, 7, 4,                                                 // back face
            5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face

            1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
            3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
            1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
        };

    positions.Resize(24);
    texCoords.Resize(24);
    normals.Resize(24);
    tangents.Resize(24);
    indices.Resize(36);

    for (int i = 0; i < 36; i += 3)
    {
        indices[i]      = _indices[i + 2];
        indices[i + 1]  = _indices[i + 1];
        indices[i + 2]  = _indices[i];
    }

    Float3 halfSize = extents * 0.5f;

    bounds.Mins  = -halfSize;
    bounds.Maxs = halfSize;

    Float3 const& mins = bounds.Mins;
    Float3 const& maxs = bounds.Maxs;

    positions[0 + 8 * 0] = Float3(mins.X, mins.Y, maxs.Z); // 0
    normals[0 + 8 * 0] = Float3(0, 0, -1);
    texCoords[0 + 8 * 0] = Float2(0, 1) * texCoordScale;

    positions[1 + 8 * 0] = Float3(maxs.X, mins.Y, maxs.Z); // 1
    normals[1 + 8 * 0] = Float3(0, 0, -1);
    texCoords[1 + 8 * 0] = Float2(1, 1) * texCoordScale;

    positions[2 + 8 * 0] = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    normals[2 + 8 * 0] = Float3(0, 0, -1);
    texCoords[2 + 8 * 0] = Float2(1, 0) * texCoordScale;

    positions[3 + 8 * 0] = Float3(mins.X, maxs.Y, maxs.Z); // 3
    normals[3 + 8 * 0] = Float3(0, 0, -1);
    texCoords[3 + 8 * 0] = Float2(0, 0) * texCoordScale;


    positions[4 + 8 * 0] = Float3(maxs.X, mins.Y, mins.Z); // 4
    normals[4 + 8 * 0] = Float3(0, 0, 1);
    texCoords[4 + 8 * 0] = Float2(0, 1) * texCoordScale;

    positions[5 + 8 * 0] = Float3(mins.X, mins.Y, mins.Z); // 5
    normals[5 + 8 * 0] = Float3(0, 0, 1);
    texCoords[5 + 8 * 0] = Float2(1, 1) * texCoordScale;

    positions[6 + 8 * 0] = Float3(mins.X, maxs.Y, mins.Z); // 6
    normals[6 + 8 * 0] = Float3(0, 0, 1);
    texCoords[6 + 8 * 0] = Float2(1, 0) * texCoordScale;

    positions[7 + 8 * 0] = Float3(maxs.X, maxs.Y, mins.Z); // 7
    normals[7 + 8 * 0] = Float3(0, 0, 1);
    texCoords[7 + 8 * 0] = Float2(0, 0) * texCoordScale;


    positions[0 + 8 * 1] = Float3(mins.X, mins.Y, maxs.Z); // 0
    normals[0 + 8 * 1] = Float3(1, 0, 0);
    texCoords[0 + 8 * 1] = Float2(1, 1) * texCoordScale;

    positions[1 + 8 * 1] = Float3(maxs.X, mins.Y, maxs.Z); // 1
    normals[1 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[1 + 8 * 1] = Float2(0, 1) * texCoordScale;

    positions[2 + 8 * 1] = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    normals[2 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[2 + 8 * 1] = Float2(0, 0) * texCoordScale;

    positions[3 + 8 * 1] = Float3(mins.X, maxs.Y, maxs.Z); // 3
    normals[3 + 8 * 1] = Float3(1, 0, 0);
    texCoords[3 + 8 * 1] = Float2(1, 0) * texCoordScale;


    positions[4 + 8 * 1] = Float3(maxs.X, mins.Y, mins.Z); // 4
    normals[4 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[4 + 8 * 1] = Float2(1, 1) * texCoordScale;

    positions[5 + 8 * 1] = Float3(mins.X, mins.Y, mins.Z); // 5
    normals[5 + 8 * 1] = Float3(1, 0, 0);
    texCoords[5 + 8 * 1] = Float2(0, 1) * texCoordScale;

    positions[6 + 8 * 1] = Float3(mins.X, maxs.Y, mins.Z); // 6
    normals[6 + 8 * 1] = Float3(1, 0, 0);
    texCoords[6 + 8 * 1] = Float2(0, 0) * texCoordScale;

    positions[7 + 8 * 1] = Float3(maxs.X, maxs.Y, mins.Z); // 7
    normals[7 + 8 * 1] = Float3(-1, 0, 0);
    texCoords[7 + 8 * 1] = Float2(1, 0) * texCoordScale;


    positions[1 + 8 * 2] = Float3(maxs.X, mins.Y, maxs.Z); // 1
    normals[1 + 8 * 2] = Float3(0, 1, 0);
    texCoords[1 + 8 * 2] = Float2(1, 0) * texCoordScale;

    positions[0 + 8 * 2] = Float3(mins.X, mins.Y, maxs.Z); // 0
    normals[0 + 8 * 2] = Float3(0, 1, 0);
    texCoords[0 + 8 * 2] = Float2(0, 0) * texCoordScale;

    positions[5 + 8 * 2] = Float3(mins.X, mins.Y, mins.Z); // 5
    normals[5 + 8 * 2] = Float3(0, 1, 0);
    texCoords[5 + 8 * 2] = Float2(0, 1) * texCoordScale;

    positions[4 + 8 * 2] = Float3(maxs.X, mins.Y, mins.Z); // 4
    normals[4 + 8 * 2] = Float3(0, 1, 0);
    texCoords[4 + 8 * 2] = Float2(1, 1) * texCoordScale;


    positions[3 + 8 * 2] = Float3(mins.X, maxs.Y, maxs.Z); // 3
    normals[3 + 8 * 2] = Float3(0, -1, 0);
    texCoords[3 + 8 * 2] = Float2(0, 1) * texCoordScale;

    positions[2 + 8 * 2] = Float3(maxs.X, maxs.Y, maxs.Z); // 2
    normals[2 + 8 * 2] = Float3(0, -1, 0);
    texCoords[2 + 8 * 2] = Float2(1, 1) * texCoordScale;

    positions[7 + 8 * 2] = Float3(maxs.X, maxs.Y, mins.Z); // 7
    normals[7 + 8 * 2] = Float3(0, -1, 0);
    texCoords[7 + 8 * 2] = Float2(1, 0) * texCoordScale;

    positions[6 + 8 * 2] = Float3(mins.X, maxs.Y, mins.Z); // 6
    normals[6 + 8 * 2] = Float3(0, -1, 0);
    texCoords[6 + 8 * 2] = Float2(0, 0) * texCoordScale;

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

void CreateSkydomeMesh(Vector<Float3>& positions, Vector<Float2>& texCoords, Vector<Float3>& normals, Vector<Float4>& tangents, Vector<unsigned int>& indices, BvAxisAlignedBox& bounds, float radius, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs, bool isHemisphere)
{
    int          x, y;
    float        verticalAngle, horizontalAngle;
    unsigned int quad[4];

    numVerticalSubdivs    = Math::Max(numVerticalSubdivs, 4);
    numHorizontalSubdivs = Math::Max(numHorizontalSubdivs, 4);

    int totalVerts = (numHorizontalSubdivs + 1) * (numVerticalSubdivs + 1);
    positions.Resize(totalVerts);
    texCoords.Resize(totalVerts);
    normals.Resize(totalVerts);
    tangents.Resize(totalVerts);

    indices.Resize(numHorizontalSubdivs * numVerticalSubdivs * 6);

    bounds.Mins.X = bounds.Mins.Y = bounds.Mins.Z = -radius;
    bounds.Maxs.X = bounds.Maxs.Y = bounds.Maxs.Z = radius;

    Float3* pPos = positions.ToPtr();
    Float2* pTC = texCoords.ToPtr();
    Float3* pNormal = normals.ToPtr();

    float verticalRange   = isHemisphere ? Math::_HALF_PI : Math::_PI;
    float verticalStep    = verticalRange / numVerticalSubdivs;
    float horizontalStep  = Math::_2PI / numHorizontalSubdivs;
    float verticalScale   = 1.0f / numVerticalSubdivs;
    float horizontalScale = 1.0f / numHorizontalSubdivs;

    for (y = 0, verticalAngle = isHemisphere ? 0 : -Math::_HALF_PI; y <= numVerticalSubdivs; y++)
    {
        float h, r;
        Math::SinCos(verticalAngle, h, r);
        float scaledH = h * radius;
        float scaledR = r * radius;
        for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
        {
            float s, c;
            Math::SinCos(horizontalAngle, s, c);
            *pPos++ = Float3(scaledR * c, scaledH, scaledR * s);
            *pTC++ = Float2(1.0f - static_cast<float>(x) * horizontalScale, 1.0f - static_cast<float>(y) * verticalScale) * texCoordScale;
            *pNormal++ = Float3(-r * c, -h, -r * s);
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int* pIndices = indices.ToPtr();
    for (y = 0; y < numVerticalSubdivs; y++)
    {
        int y2 = y + 1;
        for (x = 0; x < numHorizontalSubdivs; x++)
        {
            int x2 = x + 1;

            quad[0] = y * (numHorizontalSubdivs + 1) + x;
            quad[1] = y * (numHorizontalSubdivs + 1) + x2;
            quad[2] = y2 * (numHorizontalSubdivs + 1) + x2;
            quad[3] = y2 * (numHorizontalSubdivs + 1) + x;

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    Geometry::CalcTangentSpace(positions.ToPtr(), texCoords.ToPtr(), normals.ToPtr(), tangents.ToPtr(), indices.ToPtr(), indices.Size());
}

}

HK_NAMESPACE_END
