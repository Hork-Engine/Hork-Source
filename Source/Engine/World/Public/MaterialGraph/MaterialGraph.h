/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Base/BaseObject.h>
#include <Core/Public/Guid.h>

#include <World/Public/Resource/Material.h>

class MGNode;

enum EMGNodeType {
    AT_Unknown = 0,
    AT_Float1,
    AT_Float2,
    AT_Float3,
    AT_Float4
};

enum EMaterialStage {
    VERTEX_STAGE,
    FRAGMENT_STAGE,
    SHADOWCAST_STAGE,

    MAX_MATERIAL_STAGES
};

enum EMaterialStageBit {
    UNKNOWN_STAGE       = 0,
    VERTEX_STAGE_BIT    = 1 << VERTEX_STAGE,
    FRAGMENT_STAGE_BIT  = 1 << FRAGMENT_STAGE,
    SHADOWCAST_STAGE_BIT= 1 << SHADOWCAST_STAGE,

    ANY_STAGE_BIT       = ~0
};

enum EMaterialPass {
    MATERIAL_PASS_COLOR,
    MATERIAL_PASS_DEPTH,
    MATERIAL_PASS_WIREFRAME,
    MATERIAL_PASS_SHADOWMAP,
    MATERIAL_PASS_MAX
};

class MGNodeOutput;

class AMaterialBuildContext {
public:
    mutable AString SourceCode;
    bool bHasTextures;
    int MaxTextureSlot;
    int MaxUniformAddress;

    void Reset( EMaterialType _Type, EMaterialPass _Pass ) { ++BuildSerial; MaterialType = _Type; MaterialPass = _Pass; }
    int GetBuildSerial() const { return BuildSerial; }

    AString GenerateVariableName() const;
    void GenerateSourceCode( MGNodeOutput * _Slot, AString const & _Expression, bool _AddBrackets );

    void SetStage( EMaterialStage _Stage );
    EMaterialStage GetStage() const { return Stage; }
    int GetStageMask() const { return 1 << Stage; }

    EMaterialType GetMaterialType() const { return MaterialType; }
    EMaterialPass GetMaterialPass() const { return MaterialPass; }

private:
    mutable int VariableName = 0;
    static int BuildSerial;
    EMaterialStage Stage;
    EMaterialType MaterialType;
    EMaterialPass MaterialPass;
};

class MGNodeOutput : public ABaseObject {
    AN_CLASS( MGNodeOutput, ABaseObject )

public:
    AString Expression;
    EMGNodeType Type;
    int Usages[ MAX_MATERIAL_STAGES ];

protected:
    MGNodeOutput() {}
    ~MGNodeOutput() {}
};

class MGNodeInput : public ABaseObject {
    AN_CLASS( MGNodeInput, ABaseObject )

public:
    void Connect( MGNode * _Block, const char * _Slot );

    void Disconnect();

    MGNodeOutput * GetConnection();

    MGNode * ConnectedBlock() { return Block; }

    int Serialize( ADocument & _Doc ) override;

protected:
    AString Slot;
    TRef< MGNode > Block;

    MGNodeInput();
};

class MGNextStageVariable : public MGNodeOutput {
    AN_CLASS( MGNextStageVariable, MGNodeOutput )

public:
    void Connect( MGNode * _Block, const char * _Slot );

    void Disconnect();

    MGNodeOutput * GetConnection();

    MGNode * ConnectedBlock() { return Block; }

    int Serialize( ADocument & _Doc ) override;

protected:
    AString Slot;
    TRef< MGNode > Block;

    MGNextStageVariable() {}
    ~MGNextStageVariable() {}
};

class MGNode : public ABaseObject {
    AN_CLASS( MGNode, ABaseObject )

    friend class MGMaterialGraph;
public:
    Float2 Location;        // Block xy location for editing

    //AGUID const & GetGUID() const { return GUID; }
    uint32_t GetId() const { return ID; }

    MGNodeOutput * FindOutput( const char * _Name );

    bool Build( AMaterialBuildContext & _Context );

    void ResetConnections( AMaterialBuildContext const & _Context );
    void TouchConnections( AMaterialBuildContext const & _Context );

    int Serialize( ADocument & _Doc ) override;

protected:
    int Stages;

    MGNode( const char * _Name = "Node" );
    ~MGNode();

    MGNodeInput * AddInput( const char * _Name );

    MGNodeOutput * AddOutput( const char * _Name, EMGNodeType _Type = AT_Unknown );

    virtual void Compute( AMaterialBuildContext & _Context ) {}

private:
    //AGUID GUID;
    uint32_t ID;
    TPodArray< MGNodeInput *, 4 > Inputs;
    TPodArray< MGNodeOutput *, 1 > Outputs;
    int Serial;
    bool bTouched;
};

class MGMaterialStage : public MGNode {
    AN_CLASS( MGMaterialStage, MGNode )

    friend class MGNode;

public:
    MGNextStageVariable * AddNextStageVariable( const char * _Name, EMGNodeType _Type );

    MGNextStageVariable * FindNextStageVariable( const char * _Name );

    int NumNextStageVariables() const { return NextStageVariables.Size(); }

    AString NSV_OutputSection();

    AString NSV_InputSection();

    int Serialize( ADocument & _Doc ) override;

protected:
    TPodArray< MGNextStageVariable *, 4 > NextStageVariables;
    AString NsvPrefix;

    MGMaterialStage( const char * _Name = "Material Stage" ) : Super( _Name ) {}
    ~MGMaterialStage();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGVertexStage : public MGMaterialStage {
    AN_CLASS( MGVertexStage, MGMaterialStage )

public:
    MGNodeInput * Position;
    //MGNodeInput * ShadowMask;

    bool HasVertexDeform() const { return bHasVertexDeform; }

protected:
    MGVertexStage();
    ~MGVertexStage();

    void Compute( AMaterialBuildContext & _Context ) override;

private:
    bool bHasVertexDeform;
};

class MGFragmentStage : public MGMaterialStage {
    AN_CLASS( MGFragmentStage, MGMaterialStage )

public:
    MGNodeInput * Color;
    MGNodeInput * Normal;
    MGNodeInput * Metallic;
    MGNodeInput * Roughness;
    MGNodeInput * AmbientOcclusion;
    MGNodeInput * AmbientLight; // EXPEREMENTAL! Not tested with PBR
    MGNodeInput * Emissive;

protected:
    MGFragmentStage();
    ~MGFragmentStage();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGShadowCastStage : public MGMaterialStage {
    AN_CLASS( MGShadowCastStage, MGMaterialStage )

public:
    MGNodeInput * ShadowMask;

protected:
    MGShadowCastStage();
    ~MGShadowCastStage();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGProjectionNode : public MGNode {
    AN_CLASS( MGProjectionNode, MGNode )

public:
    MGNodeInput * Vector;
    MGNodeOutput * Result;

protected:
    MGProjectionNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGLengthNode : public MGNode {
    AN_CLASS( MGLengthNode, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGLengthNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGNormalizeNode : public MGNode {
    AN_CLASS( MGNormalizeNode, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGNormalizeNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGDecomposeVectorNode : public MGNode {
    AN_CLASS( MGDecomposeVectorNode, MGNode )

public:
    MGNodeInput * Vector;
    MGNodeOutput * X;
    MGNodeOutput * Y;
    MGNodeOutput * Z;
    MGNodeOutput * W;

protected:
    MGDecomposeVectorNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGMakeVectorNode : public MGNode {
    AN_CLASS( MGMakeVectorNode, MGNode )

public:
    MGNodeInput * X;
    MGNodeInput * Y;
    MGNodeInput * Z;
    MGNodeInput * W;
    MGNodeOutput * Result;

protected:
    MGMakeVectorNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGNegateNode : public MGNode {
    AN_CLASS( MGNegateNode, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGNegateNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFractNode : public MGNode {
    AN_CLASS( MGFractNode, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGFractNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSinusNode : public MGNode {
    AN_CLASS( MGSinusNode, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGSinusNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGCosinusNode : public MGNode {
    AN_CLASS( MGCosinusNode, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGCosinusNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGArithmeticNode : public MGNode {
    AN_CLASS( MGArithmeticNode, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    MGArithmeticNode( const char * _Name = "ArithmeticNode" );

    enum EArithmeticOp {
        Add,
        Sub,
        Mul,
        Div
    };

    EArithmeticOp ArithmeticOp;

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGMulNode : public MGArithmeticNode {
    AN_CLASS( MGMulNode, MGArithmeticNode )

protected:
    MGMulNode() : Super( "Mul A * B" ) {
        ArithmeticOp = Mul;
    }
};

class MGDivNode : public MGArithmeticNode {
    AN_CLASS( MGDivNode, MGArithmeticNode )

protected:
    MGDivNode() : Super( "Div A / B" ){
        ArithmeticOp = Div;
    }
};

class MGAddNode : public MGArithmeticNode {
    AN_CLASS( MGAddNode, MGArithmeticNode )

protected:
    MGAddNode() : Super( "Add A + B" ) {
        ArithmeticOp = Add;
    }
};

class MGSubNode : public MGArithmeticNode {
    AN_CLASS( MGSubNode, MGArithmeticNode )

protected:
    MGSubNode() : Super( "Sub A - B" ) {
        ArithmeticOp = Sub;
    }
};

class MGMADNode : public MGNode {
    AN_CLASS( MGMADNode, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeInput * ValueC;
    MGNodeOutput * Result;

protected:
    MGMADNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGStepNode : public MGNode {
    AN_CLASS( MGStepNode, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    MGStepNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGPowNode : public MGNode {
    AN_CLASS( MGPowNode, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    MGPowNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGModNode : public MGNode {
    AN_CLASS( MGModNode, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    MGModNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGLerpNode : public MGNode {
    AN_CLASS( MGLerpNode, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeInput * ValueC;
    MGNodeOutput * Result;

protected:
    MGLerpNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSpheremapCoord : public MGNode {
    AN_CLASS( MGSpheremapCoord, MGNode )

public:
    MGNodeInput * Dir;
    MGNodeOutput * TexCoord;

protected:
    MGSpheremapCoord();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGLuminance : public MGNode {
    AN_CLASS( MGLuminance, MGNode )

public:
    MGNodeInput * LinearColor;
    MGNodeOutput * Luminance;

protected:
    MGLuminance();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSaturate : public MGNode {
    AN_CLASS( MGSaturate, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    MGSaturate();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGClamp : public MGNode {
    AN_CLASS( MGClamp, MGNode )

public:
    MGNodeInput * Value;
    MGNodeInput * RangeMin;
    MGNodeInput * RangeMax;
    MGNodeOutput * Result;

protected:
    MGClamp();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGMin : public MGNode {
    AN_CLASS( MGMin, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    MGMin();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGMax : public MGNode {
    AN_CLASS( MGMax, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    MGMax();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGPINode : public MGNode {
    AN_CLASS( MGPINode, MGNode )

public:
    MGNodeOutput * OutValue;

protected:
    MGPINode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MG2PINode : public MGNode {
    AN_CLASS( MG2PINode, MGNode )

public:
    MGNodeOutput * OutValue;

protected:
    MG2PINode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloatNode : public MGNode {
    AN_CLASS( MGFloatNode, MGNode )

public:
    MGNodeOutput * OutValue;
    float Value;

protected:
    MGFloatNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloat2Node : public MGNode {
    AN_CLASS( MGFloat2Node, MGNode )

public:
    MGNodeOutput * OutValue;
    Float2 Value;

protected:
    MGFloat2Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloat3Node : public MGNode {
    AN_CLASS( MGFloat3Node, MGNode )

public:
    MGNodeOutput * OutValue;
    Float3 Value;

protected:
    MGFloat3Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloat4Node : public MGNode {
    AN_CLASS( MGFloat4Node, MGNode )

public:
    MGNodeOutput * OutValue;
    Float4 Value;

protected:
    MGFloat4Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGTextureSlot : public MGNode {
    AN_CLASS( MGTextureSlot, MGNode )

    friend class MGMaterialGraph;

public:
    MGNodeOutput * Value;

    STextureSampler SamplerDesc;

    int GetSlotIndex() const { return SlotIndex; }

protected:
    MGTextureSlot();

    void Compute( AMaterialBuildContext & _Context ) override;

private:
    int SlotIndex;
};

class MGUniformAddress : public MGNode {
    AN_CLASS( MGUniformAddress, MGNode )

    friend class AMaterialBuilder;

public:
    MGNodeOutput * Value;

    EMGNodeType Type;
    int Address;

protected:
    MGUniformAddress();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSampler : public MGNode {
    AN_CLASS( MGSampler, MGNode )

public:
    MGNodeInput * TextureSlot;
    MGNodeInput * TexCoord;
    MGNodeOutput * R;
    MGNodeOutput * G;
    MGNodeOutput * B;
    MGNodeOutput * A;
    MGNodeOutput * RGB;
    MGNodeOutput * RGBA;

    bool bSwappedToBGR;
    ETextureColorSpace ColorSpace;

protected:
    MGSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGNormalSampler : public MGNode {
    AN_CLASS( MGNormalSampler, MGNode )

public:
    MGNodeInput * TextureSlot;
    MGNodeInput * TexCoord;
    MGNodeOutput * X;
    MGNodeOutput * Y;
    MGNodeOutput * Z;
    MGNodeOutput * XYZ;

    ENormalMapCompression Compression;

protected:
    MGNormalSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInFragmentCoord : public MGNode {
    AN_CLASS( MGInFragmentCoord, MGNode )

public:

protected:
    MGInFragmentCoord();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInPosition : public MGNode {
    AN_CLASS( MGInPosition, MGNode )

public:
    MGNodeOutput * Value;

protected:
    MGInPosition();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInColor : public MGNode {
    AN_CLASS( MGInColor, MGNode )

public:
    MGNodeOutput * Value;

protected:
    MGInColor();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInTexCoord : public MGNode {
    AN_CLASS( MGInTexCoord, MGNode )

public:

protected:
    MGInTexCoord();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInTimer : public MGNode {
    AN_CLASS( MGInTimer, MGNode )

public:

protected:
    MGInTimer();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInViewPosition : public MGNode {
    AN_CLASS( MGInViewPosition, MGNode )

public:

protected:
    MGInViewPosition();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGCondLess : public MGNode {
    AN_CLASS( MGCondLess, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeInput * True;
    MGNodeInput * False;
    MGNodeOutput * Result;

protected:
    MGCondLess();

    void Compute( AMaterialBuildContext & _Context ) override;
};

// TODO: add greater, lequal, gequal, equal, not equal

class MGAtmosphereNode : public MGNode {
    AN_CLASS( MGAtmosphereNode, MGNode )

public:
    MGNodeInput * Dir;
    MGNodeOutput * Result;

protected:
    MGAtmosphereNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGMaterialGraph : public ABaseObject {
    AN_CLASS( MGMaterialGraph, ABaseObject )

public:
    template< typename T >
    T * AddNode() {
        MGNode * node = NewObject< T >();
        Nodes.Append( node );
        node->AddRef();
        node->ID = ++NodeIdGen;
        return static_cast< T * >( node );
    }

    int Serialize( ADocument & _Doc ) override;

    TRef< MGVertexStage > VertexStage;
    TRef< MGFragmentStage > FragmentStage;
    TRef< MGShadowCastStage > ShadowCastStage;
    EMaterialType       MaterialType;
    EMaterialFacing     MaterialFacing = MATERIAL_FACE_FRONT;
    EMaterialDepthHack  DepthHack = MATERIAL_DEPTH_HACK_NONE;
    bool                bDepthTest = true; // Experemental

    void RegisterTextureSlot( MGTextureSlot * _Slot );

    TPodArray< MGTextureSlot * > const & GetTextureSlots() const { return TextureSlots; }

protected:
    TPodArray< MGNode * > Nodes;
    TPodArray< MGTextureSlot * > TextureSlots;
    uint32_t NodeIdGen;

    MGMaterialGraph();

    ~MGMaterialGraph();
};

class AMaterialBuilder : public ABaseObject {
    AN_CLASS( AMaterialBuilder, ABaseObject )

public:
    TRef< MGMaterialGraph > Graph;

    AMaterial * Build();
    SMaterialBuildData * BuildData();

protected:
    AMaterialBuilder();
    ~AMaterialBuilder();

    AString SamplersString( int _MaxTextureSlot ) const;
};
