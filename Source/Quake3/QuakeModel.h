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

#include <Engine/World/Public/BaseObject.h>
#include <Engine/World/Public/Mesh/Texture.h>
#include <Engine/World/Public/StaticMeshComponent.h>
#include <Engine/World/Public/Mesh/StaticMesh.h>
#include <Engine/World/Public/Level.h>

#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

//class FTexture;


//struct QTexinfoExt {
//    float       vecs[2][4];
//    int         textureIndex;
//};

//struct QEdge {
//    unsigned short vertex0;
//    unsigned short vertex1;
//};

struct QLightmapGroup {
    int TextureIndex;
    int LightmapBlock;
};



//struct QEntity {
//    const char * ClassName;
//    Float3 Origin;
//    float Angle;
//};

//struct QTexture {
//    FTexture * Object;
//    QTexture * Next;
//    QTexture * AltNext;
//    int NumFrames;
//    int FrameTimeMin, FrameTimeMax;
//};

struct QBSPEntry {
    int32_t offset;
    int32_t size;
};

typedef struct {
    int			cluster;
    int			contents;//area;
    int 		mins[3];
    int 		maxs[3];
    int			firstmarksurface;
    int         nummarksurfaces;
    int			firstBrush;
    int         numBrushes;
} QLeaf;

typedef struct {
    int			planenum;
    int		    children[2];
    int		    mins[3];
    int		    maxs[3];
} QNode;

class FQuakeBSP : public FBaseObject {
    AN_CLASS( FQuakeBSP, FBaseObject )

public:
    TPodArray< FTexture * >   Textures;
    TPodArray< QLightmapGroup > LightmapGroups;

//    FString                 EntitiesString;
//    TPodArray< QEntity >    Entities;

    FBinarySpaceData        BSP;

    bool FromData( FLevel * _Level, const byte * _Data );
    void Purge();

    void UpdateSurfaceLight( FLevel * _Level, FSurfaceDef * _Surf );

protected:
    FQuakeBSP() {}
    ~FQuakeBSP();

private:
    void ReadLightmaps( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadPlanes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _VertexEntry, QBSPEntry const & _IndexEntry, QBSPEntry const & _ShaderEntry, QBSPEntry const & _FaceEntry );
    void ReadLFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadLeafs( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry, int _VisRowSize );
    void ReadNodes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void SetParent_r( FLevel * _Level, FBinarySpaceNode * _Node, FBinarySpaceNode * _Parent );
    int GetLightmapGroup( int _TextureIndex, int _LightmapBlock );
    FTexture * LoadTexture( const char * _FileName );
    FTexture * LoadSky();

    // Temporary data. Used for loading.
    int                     LeafsCount;
};
