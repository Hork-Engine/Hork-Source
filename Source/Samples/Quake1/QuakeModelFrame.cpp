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

#include "QuakeModelFrame.h"
#include <Engine/GameThread/Public/GameEngine.h>

AN_BEGIN_CLASS_META( FQuakeModelFrame )
AN_END_CLASS_META()

static Float3 QuakeNormals[] = {
{-0.525731f, 0.000000f, 0.850651f},
{-0.442863f, 0.238856f, 0.864188f},
{-0.295242f, 0.000000f, 0.955423f},
{-0.309017f, 0.500000f, 0.809017f},
{-0.162460f, 0.262866f, 0.951056f},
{0.000000f, 0.000000f, 1.000000f},
{0.000000f, 0.850651f, 0.525731f},
{-0.147621f, 0.716567f, 0.681718f},
{0.147621f, 0.716567f, 0.681718f},
{0.000000f, 0.525731f, 0.850651f},
{0.309017f, 0.500000f, 0.809017f},
{0.525731f, 0.000000f, 0.850651f},
{0.295242f, 0.000000f, 0.955423f},
{0.442863f, 0.238856f, 0.864188f},
{0.162460f, 0.262866f, 0.951056f},
{-0.681718f, 0.147621f, 0.716567f},
{-0.809017f, 0.309017f, 0.500000f},
{-0.587785f, 0.425325f, 0.688191f},
{-0.850651f, 0.525731f, 0.000000f},
{-0.864188f, 0.442863f, 0.238856f},
{-0.716567f, 0.681718f, 0.147621f},
{-0.688191f, 0.587785f, 0.425325f},
{-0.500000f, 0.809017f, 0.309017f},
{-0.238856f, 0.864188f, 0.442863f},
{-0.425325f, 0.688191f, 0.587785f},
{-0.716567f, 0.681718f, -0.147621f},
{-0.500000f, 0.809017f, -0.309017f},
{-0.525731f, 0.850651f, 0.000000f},
{0.000000f, 0.850651f, -0.525731f},
{-0.238856f, 0.864188f, -0.442863f},
{0.000000f, 0.955423f, -0.295242f},
{-0.262866f, 0.951056f, -0.162460f},
{0.000000f, 1.000000f, 0.000000f},
{0.000000f, 0.955423f, 0.295242f},
{-0.262866f, 0.951056f, 0.162460f},
{0.238856f, 0.864188f, 0.442863f},
{0.262866f, 0.951056f, 0.162460f},
{0.500000f, 0.809017f, 0.309017f},
{0.238856f, 0.864188f, -0.442863f},
{0.262866f, 0.951056f, -0.162460f},
{0.500000f, 0.809017f, -0.309017f},
{0.850651f, 0.525731f, 0.000000f},
{0.716567f, 0.681718f, 0.147621f},
{0.716567f, 0.681718f, -0.147621f},
{0.525731f, 0.850651f, 0.000000f},
{0.425325f, 0.688191f, 0.587785f},
{0.864188f, 0.442863f, 0.238856f},
{0.688191f, 0.587785f, 0.425325f},
{0.809017f, 0.309017f, 0.500000f},
{0.681718f, 0.147621f, 0.716567f},
{0.587785f, 0.425325f, 0.688191f},
{0.955423f, 0.295242f, 0.000000f},
{1.000000f, 0.000000f, 0.000000f},
{0.951056f, 0.162460f, 0.262866f},
{0.850651f, -0.525731f, 0.000000f},
{0.955423f, -0.295242f, 0.000000f},
{0.864188f, -0.442863f, 0.238856f},
{0.951056f, -0.162460f, 0.262866f},
{0.809017f, -0.309017f, 0.500000f},
{0.681718f, -0.147621f, 0.716567f},
{0.850651f, 0.000000f, 0.525731f},
{0.864188f, 0.442863f, -0.238856f},
{0.809017f, 0.309017f, -0.500000f},
{0.951056f, 0.162460f, -0.262866f},
{0.525731f, 0.000000f, -0.850651f},
{0.681718f, 0.147621f, -0.716567f},
{0.681718f, -0.147621f, -0.716567f},
{0.850651f, 0.000000f, -0.525731f},
{0.809017f, -0.309017f, -0.500000f},
{0.864188f, -0.442863f, -0.238856f},
{0.951056f, -0.162460f, -0.262866f},
{0.147621f, 0.716567f, -0.681718f},
{0.309017f, 0.500000f, -0.809017f},
{0.425325f, 0.688191f, -0.587785f},
{0.442863f, 0.238856f, -0.864188f},
{0.587785f, 0.425325f, -0.688191f},
{0.688191f, 0.587785f, -0.425325f},
{-0.147621f, 0.716567f, -0.681718f},
{-0.309017f, 0.500000f, -0.809017f},
{0.000000f, 0.525731f, -0.850651f},
{-0.525731f, 0.000000f, -0.850651f},
{-0.442863f, 0.238856f, -0.864188f},
{-0.295242f, 0.000000f, -0.955423f},
{-0.162460f, 0.262866f, -0.951056f},
{0.000000f, 0.000000f, -1.000000f},
{0.295242f, 0.000000f, -0.955423f},
{0.162460f, 0.262866f, -0.951056f},
{-0.442863f, -0.238856f, -0.864188f},
{-0.309017f, -0.500000f, -0.809017f},
{-0.162460f, -0.262866f, -0.951056f},
{0.000000f, -0.850651f, -0.525731f},
{-0.147621f, -0.716567f, -0.681718f},
{0.147621f, -0.716567f, -0.681718f},
{0.000000f, -0.525731f, -0.850651f},
{0.309017f, -0.500000f, -0.809017f},
{0.442863f, -0.238856f, -0.864188f},
{0.162460f, -0.262866f, -0.951056f},
{0.238856f, -0.864188f, -0.442863f},
{0.500000f, -0.809017f, -0.309017f},
{0.425325f, -0.688191f, -0.587785f},
{0.716567f, -0.681718f, -0.147621f},
{0.688191f, -0.587785f, -0.425325f},
{0.587785f, -0.425325f, -0.688191f},
{0.000000f, -0.955423f, -0.295242f},
{0.000000f, -1.000000f, 0.000000f},
{0.262866f, -0.951056f, -0.162460f},
{0.000000f, -0.850651f, 0.525731f},
{0.000000f, -0.955423f, 0.295242f},
{0.238856f, -0.864188f, 0.442863f},
{0.262866f, -0.951056f, 0.162460f},
{0.500000f, -0.809017f, 0.309017f},
{0.716567f, -0.681718f, 0.147621f},
{0.525731f, -0.850651f, 0.000000f},
{-0.238856f, -0.864188f, -0.442863f},
{-0.500000f, -0.809017f, -0.309017f},
{-0.262866f, -0.951056f, -0.162460f},
{-0.850651f, -0.525731f, 0.000000f},
{-0.716567f, -0.681718f, -0.147621f},
{-0.716567f, -0.681718f, 0.147621f},
{-0.525731f, -0.850651f, 0.000000f},
{-0.500000f, -0.809017f, 0.309017f},
{-0.238856f, -0.864188f, 0.442863f},
{-0.262866f, -0.951056f, 0.162460f},
{-0.864188f, -0.442863f, 0.238856f},
{-0.809017f, -0.309017f, 0.500000f},
{-0.688191f, -0.587785f, 0.425325f},
{-0.681718f, -0.147621f, 0.716567f},
{-0.442863f, -0.238856f, 0.864188f},
{-0.587785f, -0.425325f, 0.688191f},
{-0.309017f, -0.500000f, 0.809017f},
{-0.147621f, -0.716567f, 0.681718f},
{-0.425325f, -0.688191f, 0.587785f},
{-0.162460f, -0.262866f, 0.951056f},
{0.442863f, -0.238856f, 0.864188f},
{0.162460f, -0.262866f, 0.951056f},
{0.309017f, -0.500000f, 0.809017f},
{0.147621f, -0.716567f, 0.681718f},
{0.000000f, -0.525731f, 0.850651f},
{0.425325f, -0.688191f, 0.587785f},
{0.587785f, -0.425325f, 0.688191f},
{0.688191f, -0.587785f, 0.425325f},
{-0.955423f, 0.295242f, 0.000000f},
{-0.951056f, 0.162460f, 0.262866f},
{-1.000000f, 0.000000f, 0.000000f},
{-0.850651f, 0.000000f, 0.525731f},
{-0.955423f, -0.295242f, 0.000000f},
{-0.951056f, -0.162460f, 0.262866f},
{-0.864188f, 0.442863f, -0.238856f},
{-0.951056f, 0.162460f, -0.262866f},
{-0.809017f, 0.309017f, -0.500000f},
{-0.864188f, -0.442863f, -0.238856f},
{-0.951056f, -0.162460f, -0.262866f},
{-0.809017f, -0.309017f, -0.500000f},
{-0.681718f, 0.147621f, -0.716567f},
{-0.681718f, -0.147621f, -0.716567f},
{-0.850651f, 0.000000f, -0.525731f},
{-0.688191f, 0.587785f, -0.425325f},
{-0.587785f, 0.425325f, -0.688191f},
{-0.425325f, 0.688191f, -0.587785f},
{-0.425325f, -0.688191f, -0.587785f},
{-0.587785f, -0.425325f, -0.688191f},
{-0.688191f, -0.587785f, -0.425325f}
};

constexpr float AnimationQuantizer = 8.0f;

void FixQuakeNormals() {
    for ( int i = 0 ; i < AN_ARRAY_LENGTH( QuakeNormals ) ; i++ ) {
         FCore::SwapArgs( QuakeNormals[ i ][ 1 ], QuakeNormals[ i ][ 2 ] );
         FCore::SwapArgs( QuakeNormals[ i ][ 0 ], QuakeNormals[ i ][ 2 ] );
     }
}

FQuakeModelFrame::FQuakeModelFrame() {
    Mesh = NewObject< FIndexedMesh >();
    VSDPasses = VSD_PASS_DEFAULT | VSD_PASS_CUSTOM_VISIBLE_STEP;
}

void FQuakeModelFrame::BeginPlay() {
    Super::BeginPlay();

    SetMesh( Mesh );
}

void FQuakeModelFrame::EndPlay() {
    Super::EndPlay();
}

void FQuakeModelFrame::DecompressFrame( int _FrameIndex0, int _FrameIndex1, float _Lerp ) {
    AN_Assert( Model );

    if ( _FrameIndex0 < 0 || _FrameIndex0 >= Model->Frames.Size() ) {
        GLogger.Printf( "FQuakeModelFrame::DecompressFrame: invalid frame num\n" );
        return;
    }
    if ( _FrameIndex1 < 0 || _FrameIndex1 >= Model->Frames.Size() ) {
        GLogger.Printf( "FQuakeModelFrame::DecompressFrame: invalid frame num\n" );
        return;
    }

    FMeshVertex * pVertices = Mesh->GetVertices();

    QFrame & frame0 = Model->Frames[ _FrameIndex0 ];
    QFrame & frame1 = Model->Frames[ _FrameIndex1 ];

    int verticesCount = Model->VerticesCount;

    int poseNumA = PoseNum % frame0.NumPoses;
    int poseNumB = PoseNum % frame1.NumPoses;

    QCompressedVertex * compressedVertA = frame0.Vertices + poseNumA * verticesCount;
    QCompressedVertex * compressedVertB = frame1.Vertices + poseNumB * verticesCount;

    Float3 const & scale = Model->Scale;
    Float3 const & translate = Model->Translate;

    for ( int i = 0; i < verticesCount; i++, pVertices++, compressedVertA++, compressedVertB++ ) {
        pVertices->Position[ 0 ] = FMath::Lerp( compressedVertA->Position[ 0 ], compressedVertB->Position[ 0 ], _Lerp );
        pVertices->Position[ 1 ] = FMath::Lerp( compressedVertA->Position[ 1 ], compressedVertB->Position[ 1 ], _Lerp );
        pVertices->Position[ 2 ] = FMath::Lerp( compressedVertA->Position[ 2 ], compressedVertB->Position[ 2 ], _Lerp );

        pVertices->Position = pVertices->Position * scale + translate;

        pVertices->Normal = QuakeNormals[ compressedVertA->NormalIndex ].Lerp( QuakeNormals[ compressedVertB->NormalIndex ], _Lerp );// .Normalized();

        pVertices->TexCoord = Model->Texcoords[ i ];

        // TODO: calc tangent space?
    }

    Mesh->SendVertexDataToGPU( Model->VerticesCount, 0 );
}

void FQuakeModelFrame::DecompressFrame( int _FrameIndex ) {
    AN_Assert( Model );

    if ( _FrameIndex < 0 || _FrameIndex >= Model->Frames.Size() ) {
        GLogger.Printf( "FQuakeModelFrame::DecompressFrame: invalid frame num\n" );
        return;
    }

    FMeshVertex * pVertices = Mesh->GetVertices();

    QFrame & frame = Model->Frames[ _FrameIndex ];

    QCompressedVertex * compressedVert = frame.Vertices + ( PoseNum % frame.NumPoses ) * Model->VerticesCount;

    for ( int i = 0 ; i < Model->VerticesCount ; i++, pVertices++, compressedVert++ ) {
        pVertices->Position[0] = Model->Scale[0] * ( float )compressedVert->Position[0] + Model->Translate[0];
        pVertices->Position[1] = Model->Scale[1] * ( float )compressedVert->Position[1] + Model->Translate[1];
        pVertices->Position[2] = Model->Scale[2] * ( float )compressedVert->Position[2] + Model->Translate[2];

        pVertices->Normal = QuakeNormals[ compressedVert->NormalIndex ];

        pVertices->TexCoord = Model->Texcoords[i];

        // TODO: calc tangent space?
    }

    Mesh->SendVertexDataToGPU( Model->VerticesCount, 0 );
}

void FQuakeModelFrame::SetModel( FQuakeModel * _Model ) {
    if ( Model == _Model ) {
        return;
    }
    Model = _Model;
    bDirty = true;

    if ( Model ) {
        Mesh->Initialize( Model->VerticesCount, Model->Indices.Size(), 1, false, true );
        Mesh->WriteIndexData( Model->Indices.ToPtr(), Model->Indices.Size(), 0 );
    }

    UpdateBounds();
}

void FQuakeModelFrame::SetFrame( int _FrameIndex0, int _FrameIndex1, float _Lerp ) {

    float quantizedLerp = FMath::Floor( _Lerp * AnimationQuantizer ) / AnimationQuantizer;

    if ( Frames[0] != _FrameIndex0
         || Frames[1] != _FrameIndex1
         || Lerp != quantizedLerp ) {
        bDirty = true;

        Frames[0] = _FrameIndex0;
        Frames[1] = _FrameIndex1;
        Lerp = quantizedLerp;

        UpdateBounds();
    } else {
        //GLogger.Printf( "SetFrame skip\n" );
    }
}

void FQuakeModelFrame::SetPose( int _PoseNum ) {
    if ( PoseNum != _PoseNum ) {
        bDirty = true;

        PoseNum = _PoseNum;
    }
}

void FQuakeModelFrame::UpdateBounds() {
    if ( !Model ) {
        return;
    }

#if 1
    // Get nearest bounding box
    QFrame & frame = Model->Frames[ ( Lerp < 0.5f ? Frames[0] : Frames[1] ) % Model->Frames.Size() ];

    Bounds.Mins[0] = Model->Scale[0] * ( float )frame.Mins.Position[0] + Model->Translate[0];
    Bounds.Mins[1] = Model->Scale[1] * ( float )frame.Mins.Position[1] + Model->Translate[1];
    Bounds.Mins[2] = Model->Scale[2] * ( float )frame.Mins.Position[2] + Model->Translate[2];

    Bounds.Maxs[0] = Model->Scale[0] * ( float )frame.Maxs.Position[0] + Model->Translate[0];
    Bounds.Maxs[1] = Model->Scale[1] * ( float )frame.Maxs.Position[1] + Model->Translate[1];
    Bounds.Maxs[2] = Model->Scale[2] * ( float )frame.Maxs.Position[2] + Model->Translate[2];

    if ( Bounds.Mins[0] > Bounds.Maxs[0] ) GLogger.Printf( "Bounds.Mins[0] > Bounds.Maxs[0]\n" );
    if ( Bounds.Mins[1] > Bounds.Maxs[1] ) GLogger.Printf( "Bounds.Mins[1] > Bounds.Maxs[1]\n" );
    if ( Bounds.Mins[2] > Bounds.Maxs[2] ) GLogger.Printf( "Bounds.Mins[2] > Bounds.Maxs[2]\n" );
#else
    // Get lerped bounding box
    QFrame & frame0 = Model->Frames[ Frames[0] % Model->Frames.Length() ];
    QFrame & frame1 = Model->Frames[ Frames[1] % Model->Frames.Length() ];

    Bounds.Mins[0] = Model->Scale[0] * FMath::Lerp( frame0.Mins.Position[0], frame1.Mins.Position[0], Lerp ) + Model->Translate[0];
    Bounds.Mins[1] = Model->Scale[1] * FMath::Lerp( frame0.Mins.Position[1], frame1.Mins.Position[1], Lerp ) + Model->Translate[1];
    Bounds.Mins[2] = Model->Scale[2] * FMath::Lerp( frame0.Mins.Position[2], frame1.Mins.Position[2], Lerp ) + Model->Translate[2];

    Bounds.Maxs[0] = Model->Scale[0] * FMath::Lerp( frame0.Maxs.Position[0], frame1.Maxs.Position[0], Lerp ) + Model->Translate[0];
    Bounds.Maxs[1] = Model->Scale[1] * FMath::Lerp( frame0.Maxs.Position[1], frame1.Maxs.Position[1], Lerp ) + Model->Translate[1];
    Bounds.Maxs[2] = Model->Scale[2] * FMath::Lerp( frame0.Maxs.Position[2], frame1.Maxs.Position[2], Lerp ) + Model->Translate[2];
#endif

    MarkWorldBoundsDirty();
}

void FQuakeModelFrame::RenderFrontend_CustomVisibleStep( FRenderFrontendDef * _Def, bool & _OutVisibleFlag ) {

    _OutVisibleFlag = true;

    if ( !bDirty ) {
        return;
    }

    if ( !Model ) {
        _OutVisibleFlag = false;
        return;
    }

    bDirty = false;
    
    if ( Lerp == 0.0f ) {
        DecompressFrame( Frames[ 0 ] );
    } else {
        DecompressFrame( Frames[ 0 ], Frames[ 1 ], Lerp );
    }
}
