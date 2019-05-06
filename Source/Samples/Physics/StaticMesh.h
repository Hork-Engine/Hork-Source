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

#include <Engine/World/Public/Actor.h>
#include <Engine/World/Public/IndexedMesh.h>
#include <Engine/World/Public/MeshComponent.h>
//#include <Engine/World/Public/Material.h>

class FBoxActor : public FActor {
    AN_ACTOR( FBoxActor, FActor )

protected:
    FBoxActor();

private:
    FMeshComponent * MeshComponent;
};

class FStaticBoxActor : public FActor {
    AN_ACTOR( FStaticBoxActor, FActor )

protected:
    FStaticBoxActor();

private:
    FMeshComponent * MeshComponent;
};

class FSphereActor : public FActor {
    AN_ACTOR( FSphereActor, FActor )

protected:
    FSphereActor();

private:
    FMeshComponent * MeshComponent;
};

class FCylinderActor : public FActor {
    AN_ACTOR( FCylinderActor, FActor )

protected:
    FCylinderActor();

private:
    FMeshComponent * MeshComponent;
};


class FComposedActor : public FActor {
    AN_ACTOR( FComposedActor, FActor )

protected:
    FComposedActor();

private:
    FMeshComponent * MeshComponent;
};

class FBoxTrigger : public FActor {
    AN_ACTOR( FBoxTrigger, FActor )

protected:
    FBoxTrigger();

protected:
    void BeginPlay() override;
    void EndPlay() override;

private:
    void OnBeginOverlap( FOverlapEvent const & _Event );
    void OnEndOverlap( FOverlapEvent const & _Event );
    void OnUpdateOverlap( FOverlapEvent const & _Event );

    FMeshComponent * MeshComponent;
};
