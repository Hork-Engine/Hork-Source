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

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Resource/Public/Texture.h>
#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/World/Public/Components/MeshComponent.h>
#include <Engine/World/Public/BSP.h>

#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class FTexture;

struct QSkin {
    int32_t Group;        // 0 = single, 1 = group
    FTexture * Texture;
};

struct QCompressedVertex {
    unsigned char Position[ 3 ];
    unsigned char NormalIndex;
};

struct QFrame {
    int FirstPose;
    int NumPoses;
    QCompressedVertex Mins;
    QCompressedVertex Maxs;

    QCompressedVertex * Vertices; // pointer to first pose vertices

    // For simple frame
    char Name[ 16 ];
};

struct QPakHeader {
    unsigned char magic[ 4 ];
    int32_t diroffset;
    int32_t dirsize;
};

struct QBSPEntry {
    int32_t offset;
    int32_t size;
};

struct QTexinfoExt {
    float       vecs[2][4];
    int         textureIndex;
};

struct QEdge {
    unsigned short vertex0;
    unsigned short vertex1;
};

struct QLightmapGroup {
    int TextureIndex;
    int LightmapBlock;
};

class FQuakePack {
public:
    FQuakePack();

    bool Load( const char * _PackFile );
    bool LoadPalette( unsigned * _Palette );

    bool FindEntry( const char * _Name, int32_t * _Offset, int32_t * _Size );
    void Read( int32_t _Offset, int32_t _Size, byte * _Data );

private:
    FFileStream File;
    QPakHeader PackHeader;
    int NumEntries;
};

class FQuakeModel : public FBaseObject {
    AN_CLASS( FQuakeModel, FBaseObject )

public:
    TPodArray< QSkin > Skins;
    TPodArray< QFrame > Frames;
    TPodArray< QCompressedVertex > CompressedVertices;
    Float3 Scale;
    Float3 Translate;
    TPodArray< Float2 > Texcoords;
    TPodArray< unsigned int > Indices;
    int VerticesCount;

    bool FromData( const byte * _Data, const unsigned * _Palette );
    bool LoadFromPack( FQuakePack * _Pack, const unsigned * _Palette, const char * _ModelFile );
    void Purge();

protected:
    FQuakeModel() {}
    ~FQuakeModel();

private:
    FTexture * LoadSkin( const byte * _Data, int _Width, int _Height, const unsigned * _Palette );
};

struct QEntity {
    const char * ClassName;
    Float3 Origin;
    float Angle;
};

struct QTexture {
    FTexture * Object;
    QTexture * Next;
    QTexture * AltNext;
    int NumFrames;
    int FrameTimeMin, FrameTimeMax;
};

struct FQuakeBSPModel {
    BvAxisAlignedBox BoundingBox;
    Float3 Origin;
    int FirstSurf;
    int NumSurfaces;
    int Node;
};

class FQuakeBSP : public FBaseObject {
    AN_CLASS( FQuakeBSP, FBaseObject )

public:
    TPodArray< QTexture >   Textures;
    TPodArray< QLightmapGroup > LightmapGroups;
    TPodArray< FQuakeBSPModel > Models;

    FString                 EntitiesString;
    TPodArray< QEntity >    Entities;

    FBinarySpaceData        BSP;

    BvAxisAlignedBox        Bounds;



    bool FromData( FLevel * _Level, const byte * _Data, const unsigned * _Palette );
    bool LoadFromPack( FLevel * _Level, FQuakePack * _Pack, const unsigned * _Palette, const char * _MapFile );
    void Purge();

    void UpdateSurfaceLight( FLevel * _Level, FSurfaceDef * _Surf );

protected:
    FQuakeBSP() {}
    ~FQuakeBSP();

private:
    void ReadPlanes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadTexInfos( const byte * _Data, QBSPEntry const & _Entry );
    void ReadTextures( const byte * _Data, const unsigned * _Palette, QBSPEntry const & _Entry );
    void ReadFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadLFaces( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadLeafs( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadNodes( FLevel * _Level, const byte * _Data, QBSPEntry const & _Entry );
    void ReadClipnodes( const byte * _Data, QBSPEntry const & _Entry );
    void ReadEntities( const byte * _Data, QBSPEntry const & _Entry );
    void ReadModels( const byte * _Data, QBSPEntry const & _Entry );
    void SetParent_r( FLevel * _Level, FBinarySpaceNode * _Node, FBinarySpaceNode * _Parent );
    int GetLightmapGroup( int _TextureIndex, int _LightmapBlock );

    // Temporary data. Used for loading.
    TPodArray< QTexinfoExt >TexInfos;
    Float3 const *          SrcVertices;
    QEdge const *           Edges;
    int const *             ledges;
    int                     LeafsCount;
};


#include <Engine/Audio/Public/AudioClip.h>
#include <Engine/Audio/Public/AudioSystem.h>

class FQuakeAudio : public FAudioClip {
public:
    bool LoadFromPack( FQuakePack * _Pack, const unsigned * _Palette, const char * _FileName ) {
        int32_t offset;
        int32_t size;

        if ( !_Pack->FindEntry( _FileName, &offset, &size ) ) {
            return false;
        }

        byte * data = ( byte * )GMainHunkMemory.HunkMemory( size, 1 );

        _Pack->Read( offset, size, data );

        IAudioDecoderInterface * decoder = GAudioSystem.FindDecoder( _FileName );

        StreamType = SST_NonStreamed;

        InitializeFromData( _FileName, decoder, data, size );

        GMainHunkMemory.ClearLastHunk();

        return true;
    }
};
