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
class MGMaterialGraph;
class AMaterialBuildContext;

enum EMGNodeType
{
    AT_Unknown = 0,
    AT_Float1,
    AT_Float2,
    AT_Float3,
    AT_Float4,
    AT_Bool1,
    AT_Bool2,
    AT_Bool3,
    AT_Bool4
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
    MATERIAL_PASS_NORMALS,
    MATERIAL_PASS_SHADOWMAP,
    MATERIAL_PASS_MAX
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
    MGNodeInput * Specular;
    MGNodeInput * Opacity;

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

class MGArithmeticFunction1 : public MGNode {
    AN_CLASS( MGArithmeticFunction1, MGNode )

public:
    MGNodeInput * Value;
    MGNodeOutput * Result;

protected:
    enum EArithmeticFunction {
        Saturate,
        Sin,
        Cos,
        Fract,
        Negate,
        Normalize
    };

    MGArithmeticFunction1();
    MGArithmeticFunction1( EArithmeticFunction _Function, const char * _Name = "ArithmeticFunction1" );

    void Compute( AMaterialBuildContext & _Context ) override;

    EArithmeticFunction Function;
};

class MGArithmeticFunction2 : public MGNode {
    AN_CLASS( MGArithmeticFunction2, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeOutput * Result;

protected:
    enum EArithmeticFunction {
        Add,
        Sub,
        Mul,
        Div,
        Step,
        Pow,
        Mod,
        Min,
        Max
    };

    MGArithmeticFunction2();
    MGArithmeticFunction2( EArithmeticFunction _Function, const char * _Name = "ArithmeticFunction2" );

    void Compute( AMaterialBuildContext & _Context ) override;

    EArithmeticFunction Function;
};

class MGArithmeticFunction3 : public MGNode {
    AN_CLASS( MGArithmeticFunction3, MGNode )

public:
    MGNodeInput * ValueA;
    MGNodeInput * ValueB;
    MGNodeInput * ValueC;
    MGNodeOutput * Result;

protected:
    enum EArithmeticFunction {
        Mad,
        Lerp,
        Clamp
    };

    MGArithmeticFunction3();
    MGArithmeticFunction3( EArithmeticFunction _Function, const char * _Name = "ArithmeticFunction3" );

    void Compute( AMaterialBuildContext & _Context ) override;

    EArithmeticFunction Function;
};

class MGSaturate : public MGArithmeticFunction1 {
    AN_CLASS( MGSaturate, MGArithmeticFunction1 )

protected:
    MGSaturate() : Super( Saturate, "Saturate" ) {}
};

class MGSinusNode : public MGArithmeticFunction1 {
    AN_CLASS( MGSinusNode, MGArithmeticFunction1 )

protected:
    MGSinusNode() : Super( Sin, "Sin" ) {}
};

class MGCosinusNode : public MGArithmeticFunction1 {
    AN_CLASS( MGCosinusNode, MGArithmeticFunction1 )

protected:
    MGCosinusNode() : Super( Cos, "Cos" ) {}
};

class MGFractNode : public MGArithmeticFunction1 {
    AN_CLASS( MGFractNode, MGArithmeticFunction1 )

protected:
    MGFractNode() : Super( Fract, "Fract" ) {}
};

class MGNegateNode : public MGArithmeticFunction1 {
    AN_CLASS( MGNegateNode, MGArithmeticFunction1 )

protected:
    MGNegateNode() : Super( Negate, "Negate" ) {}
};

class MGNormalizeNode : public MGArithmeticFunction1 {
    AN_CLASS( MGNormalizeNode, MGArithmeticFunction1 )

protected:
    MGNormalizeNode() : Super( Normalize, "Normalize" ) {}
};

class MGMulNode : public MGArithmeticFunction2 {
    AN_CLASS( MGMulNode, MGArithmeticFunction2 )

protected:
    MGMulNode() : Super( Mul, "Mul A * B" ) {}
};

class MGDivNode : public MGArithmeticFunction2 {
    AN_CLASS( MGDivNode, MGArithmeticFunction2 )

protected:
    MGDivNode() : Super( Div, "Div A / B" ) {}
};

class MGAddNode : public MGArithmeticFunction2 {
    AN_CLASS( MGAddNode, MGArithmeticFunction2 )

protected:
    MGAddNode() : Super( Add, "Add A + B" ) {}
};

class MGSubNode : public MGArithmeticFunction2 {
    AN_CLASS( MGSubNode, MGArithmeticFunction2 )

protected:
    MGSubNode() : Super( Sub, "Sub A - B" ) {}
};

class MGMADNode : public MGArithmeticFunction3 {
    AN_CLASS( MGMADNode, MGArithmeticFunction3 )

protected:
    MGMADNode() : Super( Mad, "MAD A * B + C" ) {}
};

class MGStepNode : public MGArithmeticFunction2 {
    AN_CLASS( MGStepNode, MGArithmeticFunction2 )

protected:
    MGStepNode() : Super( Step, "Step( A, B )" ) {}
};

class MGPowNode : public MGArithmeticFunction2 {
    AN_CLASS( MGPowNode, MGArithmeticFunction2 )

protected:
    MGPowNode() : Super( Pow, "Pow A^B" ) {}
};

class MGModNode : public MGArithmeticFunction2 {
    AN_CLASS( MGModNode, MGArithmeticFunction2 )

protected:
    MGModNode() : Super( Mod, "Mod (A,B)" ) {}
};

class MGMin : public MGArithmeticFunction2 {
    AN_CLASS( MGMin, MGArithmeticFunction2 )

protected:
    MGMin() : Super( Min, "Min" ) {}
};

class MGMax : public MGArithmeticFunction2 {
    AN_CLASS( MGMax, MGArithmeticFunction2 )

protected:
    MGMax() : Super( Max, "Max" ) {}
};

class MGLerpNode : public MGArithmeticFunction3 {
    AN_CLASS( MGLerpNode, MGArithmeticFunction3 )

protected:
    MGLerpNode() : Super( Lerp, "Lerp( A, B, C )" ) {}
};

class MGClamp : public MGArithmeticFunction3 {
    AN_CLASS( MGClamp, MGArithmeticFunction3 )

protected:
    MGClamp() : Super( Clamp, "Clamp" ) {}
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

class MGBooleanNode : public MGNode {
    AN_CLASS( MGBooleanNode, MGNode )

public:
    MGNodeOutput * OutValue;
    bool bValue;

protected:
    MGBooleanNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBoolean2Node : public MGNode {
    AN_CLASS( MGBoolean2Node, MGNode )

public:
    MGNodeOutput * OutValue;
    Bool2 bValue;

protected:
    MGBoolean2Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBoolean3Node : public MGNode {
    AN_CLASS( MGBoolean3Node, MGNode )

public:
    MGNodeOutput * OutValue;
    Bool3 bValue;

protected:
    MGBoolean3Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBoolean4Node : public MGNode {
    AN_CLASS( MGBoolean4Node, MGNode )

public:
    MGNodeOutput * OutValue;
    Bool4 bValue;

protected:
    MGBoolean4Node();

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

    bool bSwappedToBGR = false;
    ETextureColorSpace ColorSpace = TEXTURE_COLORSPACE_RGBA;

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

    ENormalMapCompression Compression = NM_XYZ;

protected:
    MGNormalSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

// NOTE: This is singleton node. Don't allow to create more than one ParallaxMapSampler per material
class MGParallaxMapSampler : public MGNode {
    AN_CLASS( MGParallaxMapSampler, MGNode )

public:
    MGNodeInput * TextureSlot;
    MGNodeInput * TexCoord;
    MGNodeInput * DisplacementScale;
    MGNodeInput * SelfShadowing;
    MGNodeOutput * ParallaxCorrectedTexCoord;

protected:
    MGParallaxMapSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSamplerVT : public MGNode {
    AN_CLASS( MGSamplerVT, MGNode )

public:
    int TextureLayer;
    MGNodeOutput * R;
    MGNodeOutput * G;
    MGNodeOutput * B;
    MGNodeOutput * A;
    MGNodeOutput * RGB;
    MGNodeOutput * RGBA;

    bool bSwappedToBGR = false;
    ETextureColorSpace ColorSpace = TEXTURE_COLORSPACE_RGBA;

protected:
    MGSamplerVT();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGNormalSamplerVT : public MGNode {
    AN_CLASS( MGNormalSamplerVT, MGNode )

public:
    int TextureLayer;
    MGNodeOutput * X;
    MGNodeOutput * Y;
    MGNodeOutput * Z;
    MGNodeOutput * XYZ;

    ENormalMapCompression Compression = NM_XYZ;

protected:
    MGNormalSamplerVT();

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

class MGInNormal : public MGNode {
    AN_CLASS( MGInNormal, MGNode )

public:
    MGNodeOutput * Value;

protected:
    MGInNormal();

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

enum EParallaxTechnique
{
    PARALLAX_TECHNIQUE_DISABLED = 0,
    PARALLAX_TECHNIQUE_POM      = 1,  // Parallax occlusion mapping
    PARALLAX_TECHNIQUE_RPM      = 2   // Relief Parallax Mapping
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
    EColorBlending      Blending = COLOR_BLENDING_DISABLED;
    EMaterialDepthHack  DepthHack = MATERIAL_DEPTH_HACK_NONE;
    float               MotionBlurScale = 1.0f;
    bool                bDepthTest = true; // Experemental
    bool                bTranslucent = false;
    bool                bNoLightmap = false;
    bool                bAllowScreenSpaceReflections = true;
    bool                bPerBoneMotionBlur = true;
    bool                bUseVirtualTexture = false;
    EParallaxTechnique  ParallaxTechnique = PARALLAX_TECHNIQUE_RPM;

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
    void BuildData( SMaterialDef & Def );

protected:
    AMaterialBuilder();
    ~AMaterialBuilder();

    AString SamplersString( int _MaxTextureSlot ) const;
};
