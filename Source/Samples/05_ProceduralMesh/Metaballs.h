/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Runtime/MeshComponent.h>

struct SMetaball
{
    Float3 Position;
    float  RadiusSqr;
};

struct SGridVolume
{
    struct SGridCube
    {
        int Vertices[8];
    };

    TPodVectorHeap<float>  Values;
    TPodVectorHeap<Float3> Normals;

    TPodVectorHeap<Float3> const&    GetPositions() const { return Positions; }
    TPodVectorHeap<SGridCube> const& GetCubes() const { return Cubes; }
    BvAxisAlignedBox const&          GetBounds() const { return Bounds; }

    SGridVolume(int GridResolution, float Scale)
    {
        Positions.Resize((GridResolution + 1) * (GridResolution + 1) * (GridResolution + 1));
        Normals.Resize((GridResolution + 1) * (GridResolution + 1) * (GridResolution + 1));
        Values.Resize((GridResolution + 1) * (GridResolution + 1) * (GridResolution + 1));
        Cubes.Resize(GridResolution * GridResolution * GridResolution);

        int n = 0;
        for (int i = 0; i <= GridResolution; i++)
        {
            for (int j = 0; j <= GridResolution; j++)
            {
                for (int k = 0; k <= GridResolution; k++)
                {
                    Positions[n] = Float3(i, j, k) / GridResolution * 2 - Float3(1);
                    Positions[n] *= Scale;
                    n++;
                }
            }
        }

        n = 0;
        for (int i = 0; i < GridResolution; i++)
        {
            for (int j = 0; j < GridResolution; j++)
            {
                for (int k = 0; k < GridResolution; k++)
                {
                    Cubes[n].Vertices[0] = (i * (GridResolution + 1) + j) * (GridResolution + 1) + k;
                    Cubes[n].Vertices[1] = (i * (GridResolution + 1) + j) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[2] = (i * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[3] = (i * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k;
                    Cubes[n].Vertices[4] = ((i + 1) * (GridResolution + 1) + j) * (GridResolution + 1) + k;
                    Cubes[n].Vertices[5] = ((i + 1) * (GridResolution + 1) + j) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[6] = ((i + 1) * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[7] = ((i + 1) * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k;
                    n++;
                }
            }
        }

        Bounds.Mins = {-Scale, -Scale, -Scale};
        Bounds.Maxs = {Scale, Scale, Scale};
    }

private:
    TPodVectorHeap<Float3>    Positions;
    TPodVectorHeap<SGridCube> Cubes;
    BvAxisAlignedBox          Bounds;
};

void UpdateMetaballs(AProceduralMesh* ProcMeshResource, SMetaball const* Metaballs, int MetaballCount, float Threshold, SGridVolume& Volume);
