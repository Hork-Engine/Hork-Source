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

#include <Engine/World/Public/SceneComponent.h>

#include "QuakeModel.h"

class FQuakeModelFrame : public FMeshComponent {
    AN_COMPONENT( FQuakeModelFrame, FMeshComponent )

public:
    void SetModel( FQuakeModel * _Model );
    void SetFrame( int _FrameIndex0, int _FrameIndex1, float _Lerp );
    void SetPose( int _PoseNum );

protected:

    void BeginPlay();
    void EndPlay();
    void OnCustomVisibleStep( class FCameraComponent * _Camera, bool & _OutVisibleFlag );

protected:
    FQuakeModelFrame();

private:
    void DecompressFrame( int _FrameIndex0, int _FrameIndex1, float _Lerp );
    void DecompressFrame( int _FrameIndex );
    void UpdateBounds();

    TRefHolder< FQuakeModel > Model;
    TRefHolder< FIndexedMesh > Mesh;
    int Frames[2];
    int PoseNum;
    float Lerp;
    bool bDirty;
};
