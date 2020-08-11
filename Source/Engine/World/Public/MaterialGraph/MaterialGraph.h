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

#define MG_CLASS( ThisClass, SuperClass ) \
    AN_CLASS( ThisClass, SuperClass ) \
    public: \
        static bool IsSingleton() { return false; } \
    private:

#define MG_SINGLETON( ThisClass, SuperClass ) \
    AN_CLASS( ThisClass, SuperClass ) \
    public: \
        static bool IsSingleton() { return true; } \
    private:

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

class MGOutput : public ABaseObject {
    MG_CLASS( MGOutput, ABaseObject )

public:
    AString Expression;
    EMGNodeType Type = AT_Unknown;
    int Usages = 0;
    MGNode * GetOwner() { return Owner; }

protected:
    MGOutput() {}
    ~MGOutput() {}

private:
    MGNode * Owner;

    friend class MGNode;
};

class MGInput : public ABaseObject {
    MG_CLASS( MGInput, ABaseObject )

public:
    void Connect( MGNode * pNode, const char * SlotName ); // deprecated
    void Connect( MGOutput * pSlot );

    void Disconnect();

    MGOutput * GetConnection();

    MGNode * ConnectedNode() { return Slot ? Slot->GetOwner() : nullptr; }

    int Serialize( ADocument & _Doc ) override;

protected:
    TRef< MGOutput > Slot;

    MGInput();
    ~MGInput();
};

class MGNode : public ABaseObject {
    MG_CLASS( MGNode, ABaseObject )

    friend class MGMaterialGraph;
public:
    Float2 Location;        // node xy location for editing

    //AGUID const & GetGUID() const { return GUID; }
    uint32_t GetId() const { return ID; }

    MGOutput * FindOutput( const char * _Name );

    bool Build( AMaterialBuildContext & _Context );

    void ResetConnections( AMaterialBuildContext const & _Context );
    void TouchConnections( AMaterialBuildContext const & _Context );

    int Serialize( ADocument & _Doc ) override;

protected:
    MGNode( const char * _Name = "Node" );
    ~MGNode();

    MGInput * AddInput( const char * _Name );

    MGOutput * AddOutput( const char * _Name, EMGNodeType _Type = AT_Unknown );

    virtual void Compute( AMaterialBuildContext & _Context ) {}

private:
    //AGUID GUID;
    uint32_t ID;
    TPodArray< MGInput *, 4 > Inputs;
    TPodArray< MGOutput *, 1 > Outputs;
    int Serial;
    bool bTouched;
    bool bSingleton;
};

class MGArithmeticFunction1 : public MGNode {
    MG_CLASS( MGArithmeticFunction1, MGNode )

public:
    MGInput * Value;
    MGOutput * Result;

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
    MG_CLASS( MGArithmeticFunction2, MGNode )

public:
    MGInput * ValueA;
    MGInput * ValueB;
    MGOutput * Result;

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
    MG_CLASS( MGArithmeticFunction3, MGNode )

public:
    MGInput * ValueA;
    MGInput * ValueB;
    MGInput * ValueC;
    MGOutput * Result;

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
    MG_CLASS( MGSaturate, MGArithmeticFunction1 )

protected:
    MGSaturate() : Super( Saturate, "Saturate" ) {}
};

class MGSinusNode : public MGArithmeticFunction1 {
    MG_CLASS( MGSinusNode, MGArithmeticFunction1 )

protected:
    MGSinusNode() : Super( Sin, "Sin" ) {}
};

class MGCosinusNode : public MGArithmeticFunction1 {
    MG_CLASS( MGCosinusNode, MGArithmeticFunction1 )

protected:
    MGCosinusNode() : Super( Cos, "Cos" ) {}
};

class MGFractNode : public MGArithmeticFunction1 {
    MG_CLASS( MGFractNode, MGArithmeticFunction1 )

protected:
    MGFractNode() : Super( Fract, "Fract" ) {}
};

class MGNegateNode : public MGArithmeticFunction1 {
    MG_CLASS( MGNegateNode, MGArithmeticFunction1 )

protected:
    MGNegateNode() : Super( Negate, "Negate" ) {}
};

class MGNormalizeNode : public MGArithmeticFunction1 {
    MG_CLASS( MGNormalizeNode, MGArithmeticFunction1 )

protected:
    MGNormalizeNode() : Super( Normalize, "Normalize" ) {}
};

class MGMulNode : public MGArithmeticFunction2 {
    MG_CLASS( MGMulNode, MGArithmeticFunction2 )

protected:
    MGMulNode() : Super( Mul, "Mul A * B" ) {}
};

class MGDivNode : public MGArithmeticFunction2 {
    MG_CLASS( MGDivNode, MGArithmeticFunction2 )

protected:
    MGDivNode() : Super( Div, "Div A / B" ) {}
};

class MGAddNode : public MGArithmeticFunction2 {
    MG_CLASS( MGAddNode, MGArithmeticFunction2 )

protected:
    MGAddNode() : Super( Add, "Add A + B" ) {}
};

class MGSubNode : public MGArithmeticFunction2 {
    MG_CLASS( MGSubNode, MGArithmeticFunction2 )

protected:
    MGSubNode() : Super( Sub, "Sub A - B" ) {}
};

class MGMADNode : public MGArithmeticFunction3 {
    MG_CLASS( MGMADNode, MGArithmeticFunction3 )

protected:
    MGMADNode() : Super( Mad, "MAD A * B + C" ) {}
};

class MGStepNode : public MGArithmeticFunction2 {
    MG_CLASS( MGStepNode, MGArithmeticFunction2 )

protected:
    MGStepNode() : Super( Step, "Step( A, B )" ) {}
};

class MGPowNode : public MGArithmeticFunction2 {
    MG_CLASS( MGPowNode, MGArithmeticFunction2 )

protected:
    MGPowNode() : Super( Pow, "Pow A^B" ) {}
};

class MGModNode : public MGArithmeticFunction2 {
    MG_CLASS( MGModNode, MGArithmeticFunction2 )

protected:
    MGModNode() : Super( Mod, "Mod (A,B)" ) {}
};

class MGMin : public MGArithmeticFunction2 {
    MG_CLASS( MGMin, MGArithmeticFunction2 )

protected:
    MGMin() : Super( Min, "Min" ) {}
};

class MGMax : public MGArithmeticFunction2 {
    MG_CLASS( MGMax, MGArithmeticFunction2 )

protected:
    MGMax() : Super( Max, "Max" ) {}
};

class MGLerpNode : public MGArithmeticFunction3 {
    MG_CLASS( MGLerpNode, MGArithmeticFunction3 )

protected:
    MGLerpNode() : Super( Lerp, "Lerp( A, B, C )" ) {}
};

class MGClamp : public MGArithmeticFunction3 {
    MG_CLASS( MGClamp, MGArithmeticFunction3 )

protected:
    MGClamp() : Super( Clamp, "Clamp" ) {}
};

#if 0
class MGProjectionNode : public MGNode {
    MG_CLASS( MGProjectionNode, MGNode )

public:
    MGInput * Vector;
    MGOutput * Result;

protected:
    MGProjectionNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};
#endif

class MGLengthNode : public MGNode {
    MG_CLASS( MGLengthNode, MGNode )

public:
    MGInput * Value;
    MGOutput * Result;

protected:
    MGLengthNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGDecomposeVectorNode : public MGNode {
    MG_CLASS( MGDecomposeVectorNode, MGNode )

public:
    MGInput * Vector;
    MGOutput * X;
    MGOutput * Y;
    MGOutput * Z;
    MGOutput * W;

protected:
    MGDecomposeVectorNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGMakeVectorNode : public MGNode {
    MG_CLASS( MGMakeVectorNode, MGNode )

public:
    MGInput * X;
    MGInput * Y;
    MGInput * Z;
    MGInput * W;
    MGOutput * Result;

protected:
    MGMakeVectorNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSpheremapCoord : public MGNode {
    MG_CLASS( MGSpheremapCoord, MGNode )

public:
    MGInput * Dir;
    MGOutput * TexCoord;

protected:
    MGSpheremapCoord();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGLuminance : public MGNode {
    MG_CLASS( MGLuminance, MGNode )

public:
    MGInput * LinearColor;
    MGOutput * Luminance;

protected:
    MGLuminance();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGPINode : public MGNode {
    MG_CLASS( MGPINode, MGNode )

public:
    MGOutput * OutValue;

protected:
    MGPINode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MG2PINode : public MGNode {
    MG_CLASS( MG2PINode, MGNode )

public:
    MGOutput * OutValue;

protected:
    MG2PINode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBooleanNode : public MGNode {
    MG_CLASS( MGBooleanNode, MGNode )

public:
    MGOutput * OutValue;
    bool bValue;

protected:
    MGBooleanNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBoolean2Node : public MGNode {
    MG_CLASS( MGBoolean2Node, MGNode )

public:
    MGOutput * OutValue;
    Bool2 bValue;

protected:
    MGBoolean2Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBoolean3Node : public MGNode {
    MG_CLASS( MGBoolean3Node, MGNode )

public:
    MGOutput * OutValue;
    Bool3 bValue;

protected:
    MGBoolean3Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGBoolean4Node : public MGNode {
    MG_CLASS( MGBoolean4Node, MGNode )

public:
    MGOutput * OutValue;
    Bool4 bValue;

protected:
    MGBoolean4Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloatNode : public MGNode {
    MG_CLASS( MGFloatNode, MGNode )

public:
    MGOutput * OutValue;
    float Value;

protected:
    MGFloatNode();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloat2Node : public MGNode {
    MG_CLASS( MGFloat2Node, MGNode )

public:
    MGOutput * OutValue;
    Float2 Value;

protected:
    MGFloat2Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloat3Node : public MGNode {
    MG_CLASS( MGFloat3Node, MGNode )

public:
    MGOutput * OutValue;
    Float3 Value;

protected:
    MGFloat3Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGFloat4Node : public MGNode {
    MG_CLASS( MGFloat4Node, MGNode )

public:
    MGOutput * OutValue;
    Float4 Value;

protected:
    MGFloat4Node();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGTextureSlot : public MGNode {
    MG_CLASS( MGTextureSlot, MGNode )

    friend class MGMaterialGraph;

public:
    MGOutput * Value;

    STextureSampler SamplerDesc;

    int GetSlotIndex() const { return SlotIndex; }

protected:
    MGTextureSlot();

    void Compute( AMaterialBuildContext & _Context ) override;

private:
    int SlotIndex;
};

class MGUniformAddress : public MGNode {
    MG_CLASS( MGUniformAddress, MGNode )

    friend class AMaterialBuilder;

public:
    MGOutput * Value;

    EMGNodeType Type;
    int Address;

protected:
    MGUniformAddress();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSampler : public MGNode {
    MG_CLASS( MGSampler, MGNode )

public:
    MGInput * TextureSlot;
    MGInput * TexCoord;
    MGOutput * R;
    MGOutput * G;
    MGOutput * B;
    MGOutput * A;
    MGOutput * RGB;
    MGOutput * RGBA;

    bool bSwappedToBGR = false;
    ETextureColorSpace ColorSpace = TEXTURE_COLORSPACE_RGBA;

protected:
    MGSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGNormalSampler : public MGNode {
    MG_CLASS( MGNormalSampler, MGNode )

public:
    MGInput * TextureSlot;
    MGInput * TexCoord;
    MGOutput * X;
    MGOutput * Y;
    MGOutput * Z;
    MGOutput * XYZ;

    ENormalMapCompression Compression = NM_XYZ;

protected:
    MGNormalSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

// NOTE: This is singleton node. Don't allow to create more than one ParallaxMapSampler per material
class MGParallaxMapSampler : public MGNode {
    MG_CLASS( MGParallaxMapSampler, MGNode )

public:
    MGInput * TextureSlot;
    MGInput * TexCoord;
    MGInput * DisplacementScale;
    MGInput * SelfShadowing;
    MGOutput * Result;

protected:
    MGParallaxMapSampler();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGSamplerVT : public MGNode {
    MG_CLASS( MGSamplerVT, MGNode )

public:
    int TextureLayer;
    MGOutput * R;
    MGOutput * G;
    MGOutput * B;
    MGOutput * A;
    MGOutput * RGB;
    MGOutput * RGBA;

    bool bSwappedToBGR = false;
    ETextureColorSpace ColorSpace = TEXTURE_COLORSPACE_RGBA;

protected:
    MGSamplerVT();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGNormalSamplerVT : public MGNode {
    MG_CLASS( MGNormalSamplerVT, MGNode )

public:
    int TextureLayer;
    MGOutput * X;
    MGOutput * Y;
    MGOutput * Z;
    MGOutput * XYZ;

    ENormalMapCompression Compression = NM_XYZ;

protected:
    MGNormalSamplerVT();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInFragmentCoord : public MGNode {
    MG_SINGLETON( MGInFragmentCoord, MGNode )

public:

protected:
    MGInFragmentCoord();

    void Compute( AMaterialBuildContext & _Context ) override;
};

// Vertex position in model space
class MGInPosition : public MGNode {
    MG_SINGLETON( MGInPosition, MGNode )

public:
    MGOutput * Value;

protected:
    MGInPosition();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInNormal : public MGNode {
    MG_SINGLETON( MGInNormal, MGNode )

public:
    MGOutput * Value;

protected:
    MGInNormal();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInColor : public MGNode {
    MG_SINGLETON( MGInColor, MGNode )

public:
    MGOutput * Value;

protected:
    MGInColor();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInTexCoord : public MGNode {
    MG_SINGLETON( MGInTexCoord, MGNode )

public:
    MGOutput * Value;

protected:
    MGInTexCoord();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInTimer : public MGNode {
    MG_SINGLETON( MGInTimer, MGNode )

public:
    MGOutput * GameRunningTimeSeconds;
    MGOutput * GameplayTimeSeconds;

protected:
    MGInTimer();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGInViewPosition : public MGNode {
    MG_SINGLETON( MGInViewPosition, MGNode )

public:

protected:
    MGInViewPosition();

    void Compute( AMaterialBuildContext & _Context ) override;
};

class MGCondLess : public MGNode {
    MG_CLASS( MGCondLess, MGNode )

public:
    MGInput * ValueA;
    MGInput * ValueB;
    MGInput * True;
    MGInput * False;
    MGOutput * Result;

protected:
    MGCondLess();

    void Compute( AMaterialBuildContext & _Context ) override;
};

// TODO: add greater, lequal, gequal, equal, not equal

class MGAtmosphereNode : public MGNode {
    MG_CLASS( MGAtmosphereNode, MGNode )

public:
    MGInput * Dir;
    MGOutput * Result;

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

class MGMaterialGraph : public MGNode {
    MG_CLASS( MGMaterialGraph, MGNode )

public:
    EMaterialType       MaterialType;
    ETessellationMethod TessellationMethod = TESSELLATION_DISABLED;
    EColorBlending      Blending = COLOR_BLENDING_DISABLED;
    EParallaxTechnique  ParallaxTechnique = PARALLAX_TECHNIQUE_RPM;
    EMaterialDepthHack  DepthHack = MATERIAL_DEPTH_HACK_NONE;
    float               MotionBlurScale = 1.0f;
    bool                bDepthTest = true; // Experimental
    bool                bTranslucent = false;
    bool                bTwoSided = false;
    bool                bNoLightmap = false;
    bool                bAllowScreenSpaceReflections = true;
    bool                bAllowScreenAmbientOcclusion = true;
    bool                bAllowShadowReceive = true;
    bool                bDisplacementAffectShadow = true;
    bool                bPerBoneMotionBlur = true;
    bool                bUseVirtualTexture = false;

    //
    // Inputs
    //
    MGInput * Color;
    MGInput * Normal;
    MGInput * Metallic;
    MGInput * Roughness;
    MGInput * AmbientOcclusion;
    MGInput * AmbientLight; // EXPERIMENTAL! Not tested with PBR
    MGInput * Emissive;
    MGInput * Specular;
    MGInput * Opacity;
    MGInput * VertexDeform;
    MGInput * AlphaMask;
    MGInput * Displacement;
    MGInput * TessellationFactor;

    template< typename T >
    T * AddNode() {
        if ( T::IsSingleton() ) {
            for ( MGNode * node : Nodes ) {
                if ( node->FinalClassId() == T::ClassId() ) {
                    return static_cast< T * >( node );
                }
            }
        }
        MGNode * node = NewObject< T >();
        Nodes.Append( node );
        node->AddRef();
        node->ID = ++NodeIdGen;
        return static_cast< T * >( node );
    }

    int Serialize( ADocument & _Doc ) override;

    void RegisterTextureSlot( MGTextureSlot * _Slot );

    TPodArray< MGTextureSlot * > const & GetTextureSlots() const { return TextureSlots; }

    void CompileStage( class AMaterialBuildContext & ctx );

    void CreateStageTransitions( struct SMaterialStageTransition & Transition,
                                 AMaterialBuildContext const * VertexStage,
                                 AMaterialBuildContext const * TessControlStage,
                                 AMaterialBuildContext const * TessEvalStage,
                                 AMaterialBuildContext const * GeometryStage,
                                 AMaterialBuildContext const * FragmentStage );

protected:
    TPodArray< MGNode * > Nodes;
    TPodArray< MGTextureSlot * > TextureSlots;
    uint32_t NodeIdGen;

    MGMaterialGraph();
    ~MGMaterialGraph();

    void Compute( AMaterialBuildContext & _Context ) override;

    void ComputeVertexStage( AMaterialBuildContext & _Context );
    void ComputeDepthStage( AMaterialBuildContext & _Context );
    void ComputeLightStage( AMaterialBuildContext & _Context );
    void ComputeShadowCastStage( AMaterialBuildContext & _Context );
    void ComputeTessellationControlStage( AMaterialBuildContext & _Context );
    void ComputeTessellationEvalStage( AMaterialBuildContext & _Context );
    void ComputeAlphaMask( AMaterialBuildContext & _Context );
};

void CompileMaterialGraph( MGMaterialGraph * InGraph, SMaterialDef * pDef );
