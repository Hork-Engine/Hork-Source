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

#include <Engine/World/Public/Actors/Actor.h>
#include <Engine/Core/Public/BV/BvFrustum.h>

#include "QuakeModel.h"

class ANGIE_API FQuakeBSPView : public FViewActor {
    AN_ACTOR( FQuakeBSPView, FViewActor )

public:
    void SetModel( FQuakeBSP * _Model );

protected:
    FQuakeBSPView();

    void BeginPlay() override;
    void Tick( float _TimeStep ) override;

    void OnView( FCameraComponent * _Camera ) override;

    void DrawDebug( FDebugDraw * _DebugDraw ) override;

private:
    void AddSurfaces();
    void AddSurface( int _NumIndices, int _FirstIndex, int _SurfIndex );
    //void GetGeometry_r( int _NodeIndex, TPodArray< Float3 > & _CollisVerts, TPodArray< unsigned int > & _CollisInd );

    TRef< FQuakeBSP > Model;
    FBinarySpaceData * BSP;
    TRef< FIndexedMesh > Mesh;
    TRef< FLightmapUV > LightmapUV;
    TPodArray< FMeshComponent * > SurfacePool;
    TPodArray< FMeshVertex > Vertices;
    TPodArray< FMeshLightmapUV > LightmapVerts;
    TPodArray< unsigned int > Indices;
    TRef< FTexture > CubemapTex;
    TRef< FAudioControlCallback > AmbientControl[4];
};
