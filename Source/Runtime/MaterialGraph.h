/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "BaseObject.h"
#include <Core/Guid.h>
#include <Image/Image.h>
#include <Renderer/RenderDefs.h>

class MGNode;
class MGMaterialGraph;
class AMaterialBuildContext;

enum MG_VALUE_TYPE
{
    MG_VALUE_TYPE_UNDEFINED = 0,
    MG_VALUE_TYPE_FLOAT1,
    MG_VALUE_TYPE_FLOAT2,
    MG_VALUE_TYPE_FLOAT3,
    MG_VALUE_TYPE_FLOAT4,
    MG_VALUE_TYPE_BOOL1,
    MG_VALUE_TYPE_BOOL2,
    MG_VALUE_TYPE_BOOL3,
    MG_VALUE_TYPE_BOOL4
};

enum MG_UNIFORM_TYPE
{
    MG_UNIFORM_TYPE_UNDEFINED = 0,
    MG_UNIFORM_TYPE_FLOAT1,
    MG_UNIFORM_TYPE_FLOAT2,
    MG_UNIFORM_TYPE_FLOAT3,
    MG_UNIFORM_TYPE_FLOAT4
};

enum PARALLAX_TECHNIQUE
{
    PARALLAX_TECHNIQUE_DISABLED = 0,
    PARALLAX_TECHNIQUE_POM      = 1, // Parallax occlusion mapping
    PARALLAX_TECHNIQUE_RPM      = 2  // Relief Parallax Mapping
};

class MGOutput
{
public:
    AString       Expression;
    MG_VALUE_TYPE Type   = MG_VALUE_TYPE_UNDEFINED;
    int           Usages = 0;

    MGOutput(AStringView Name, MG_VALUE_TYPE Type = MG_VALUE_TYPE_UNDEFINED) :
        m_Name(Name), Type(Type)
    {}

    AString const& GetName() const
    {
        return m_Name;
    }

private:
    AString m_Name;
    MGNode* m_Owner = nullptr;

    friend class MGNode;
};

class MGInput
{
public:
    MGInput(AStringView Name) :
        m_Name(Name)
    {}

    AString const& GetName() const
    {
        return m_Name;
    }

    MGOutput* GetConnection()
    {
        return m_Slot;
    }

    MGNode* ConnectedNode()
    {
        return m_SlotNode;
    }

private:
    AString      m_Name;
    MGOutput*    m_Slot = nullptr;
    TRef<MGNode> m_SlotNode;

    friend class MGNode;
};

class MGNode : public ABaseObject
{
    HK_CLASS(MGNode, ABaseObject)

    friend class MGMaterialGraph;

public:
    Float2 Location;

    MGNode(AStringView Name = "Unnamed");

    uint32_t GetId() const { return m_ID; }

    MGNode& BindInput(AStringView InputSlot, MGNode* Node);
    MGNode& BindInput(AStringView InputSlot, MGNode& Node);
    MGNode& BindInput(AStringView InputSlot, MGOutput& pSlot);
    MGNode& BindInput(AStringView InputSlot, MGOutput* pSlot);

    void UnbindInput(AStringView InputSlot);

    MGOutput* operator[](AStringView Name)
    {
        return FindOutput(Name);
    }

    MGOutput* FindOutput(AStringView _Name);

    bool Build(AMaterialBuildContext& Context);

    void ResetConnections(AMaterialBuildContext const& Context);
    void TouchConnections(AMaterialBuildContext const& Context);

    TVector<MGInput*> const&  GetInputs() const { return m_Inputs; }
    TVector<MGOutput*> const& GetOutputs() const { return m_Outputs; }

    void ParseProperties(ADocValue const* document);

protected:
    void SetInputs(std::initializer_list<MGInput*> Inputs);
    void SetOutputs(std::initializer_list<MGOutput*> Outputs);

    void SetSlots(std::initializer_list<MGInput*> Inputs, std::initializer_list<MGOutput*> Outputs)
    {
        SetInputs(Inputs);
        SetOutputs(Outputs);
    }

    virtual void Compute(AMaterialBuildContext& Context) {}

private:
    uint32_t           m_ID{};
    AString            m_Name;
    TVector<MGInput*>  m_Inputs;
    TVector<MGOutput*> m_Outputs;
    int                m_Serial{};
    bool               m_bTouched{};
};

class MGSingleton : public MGNode
{
    HK_CLASS(MGSingleton, MGNode)

public:
    MGSingleton() = default;
    MGSingleton(AStringView Name) :
        Super(Name)
    {}
    ~MGSingleton() = default;
};

using MGNodeRef = MGNode&;

class MGArithmeticFunction1 : public MGNode
{
    HK_CLASS(MGArithmeticFunction1, MGNode)

    MGInput  Value{"Value"};
    MGOutput Result{"Result", MG_VALUE_TYPE_UNDEFINED};

public:
    MGArithmeticFunction1();

protected:
    enum FUNCTION
    {
        Saturate,
        Sin,
        Cos,
        Fract,
        Negate,
        Normalize
    };

    MGArithmeticFunction1(FUNCTION _Function, AStringView _Name = "ArithmeticFunction1");

    void Compute(AMaterialBuildContext& Context) override;

    FUNCTION m_Function{};
};

class MGArithmeticFunction2 : public MGNode
{
    HK_CLASS(MGArithmeticFunction2, MGNode)

    MGInput  ValueA{"A"};
    MGInput  ValueB{"B"};
    MGOutput Result{"Result", MG_VALUE_TYPE_UNDEFINED};

public:
    MGArithmeticFunction2();

protected:
    enum FUNCTION
    {
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

    MGArithmeticFunction2(FUNCTION Function, AStringView Name = "ArithmeticFunction2");

    void Compute(AMaterialBuildContext& Context) override;

    FUNCTION m_Function;
};

class MGArithmeticFunction3 : public MGNode
{
    HK_CLASS(MGArithmeticFunction3, MGNode)

    MGInput  ValueA{"A"};
    MGInput  ValueB{"B"};
    MGInput  ValueC{"C"};
    MGOutput Result{"Result", MG_VALUE_TYPE_UNDEFINED};

public:
    enum FUNCTION
    {
        Mad,
        Lerp,
        Clamp
    };

    MGArithmeticFunction3();

protected:
    MGArithmeticFunction3(FUNCTION Function, AStringView Name = "ArithmeticFunction3");

    void Compute(AMaterialBuildContext& Context) override;

    FUNCTION m_Function;
};

class MGSaturate : public MGArithmeticFunction1
{
    HK_CLASS(MGSaturate, MGArithmeticFunction1)

public:
    MGSaturate() :
        Super(Saturate, "Saturate") {}
};

class MGSinus : public MGArithmeticFunction1
{
    HK_CLASS(MGSinus, MGArithmeticFunction1)

public:
    MGSinus() :
        Super(Sin, "Sin") {}
};

class MGCosinus : public MGArithmeticFunction1
{
    HK_CLASS(MGCosinus, MGArithmeticFunction1)

public:
    MGCosinus() :
        Super(Cos, "Cos") {}
};

class MGFract : public MGArithmeticFunction1
{
    HK_CLASS(MGFract, MGArithmeticFunction1)

public:
    MGFract() :
        Super(Fract, "Fract") {}
};

class MGNegate : public MGArithmeticFunction1
{
    HK_CLASS(MGNegate, MGArithmeticFunction1)

public:
    MGNegate() :
        Super(Negate, "Negate") {}
};

class MGNormalize : public MGArithmeticFunction1
{
    HK_CLASS(MGNormalize, MGArithmeticFunction1)

public:
    MGNormalize() :
        Super(Normalize, "Normalize") {}
};

class MGMul : public MGArithmeticFunction2
{
    HK_CLASS(MGMul, MGArithmeticFunction2)

public:
    MGMul() :
        Super(Mul, "Mul A * B") {}
};

class MGDiv : public MGArithmeticFunction2
{
    HK_CLASS(MGDiv, MGArithmeticFunction2)

public:
    MGDiv() :
        Super(Div, "Div A / B") {}
};

class MGAdd : public MGArithmeticFunction2
{
    HK_CLASS(MGAdd, MGArithmeticFunction2)

public:
    MGAdd() :
        Super(Add, "Add A + B") {}
};

class MGSub : public MGArithmeticFunction2
{
    HK_CLASS(MGSub, MGArithmeticFunction2)

public:
    MGSub() :
        Super(Sub, "Sub A - B") {}
};

class MGMAD : public MGArithmeticFunction3
{
    HK_CLASS(MGMAD, MGArithmeticFunction3)

public:
    MGMAD() :
        Super(Mad, "MAD A * B + C") {}
};

class MGStep : public MGArithmeticFunction2
{
    HK_CLASS(MGStep, MGArithmeticFunction2)

public:
    MGStep() :
        Super(Step, "Step( A, B )") {}
};

class MGPow : public MGArithmeticFunction2
{
    HK_CLASS(MGPow, MGArithmeticFunction2)

public:
    MGPow() :
        Super(Pow, "Pow A^B") {}
};

class MGMod : public MGArithmeticFunction2
{
    HK_CLASS(MGMod, MGArithmeticFunction2)

public:
    MGMod() :
        Super(Mod, "Mod (A,B)") {}
};

class MGMin : public MGArithmeticFunction2
{
    HK_CLASS(MGMin, MGArithmeticFunction2)

public:
    MGMin() :
        Super(Min, "Min") {}
};

class MGMax : public MGArithmeticFunction2
{
    HK_CLASS(MGMax, MGArithmeticFunction2)

public:
    MGMax() :
        Super(Max, "Max") {}
};

class MGLerp : public MGArithmeticFunction3
{
    HK_CLASS(MGLerp, MGArithmeticFunction3)

public:
    MGLerp() :
        Super(Lerp, "Lerp( A, B, C )") {}
};

class MGClamp : public MGArithmeticFunction3
{
    HK_CLASS(MGClamp, MGArithmeticFunction3)

public:
    MGClamp() :
        Super(Clamp, "Clamp") {}
};

class MGLength : public MGNode
{
    HK_CLASS(MGLength, MGNode)

    MGInput  Value{"Value"};
    MGOutput Result{"Result", MG_VALUE_TYPE_FLOAT1};

public:
    MGLength();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGDecomposeVector : public MGNode
{
    HK_CLASS(MGDecomposeVector, MGNode)

    MGInput  Vector{"Vector"};
    MGOutput X{"X", MG_VALUE_TYPE_UNDEFINED};
    MGOutput Y{"Y", MG_VALUE_TYPE_UNDEFINED};
    MGOutput Z{"Z", MG_VALUE_TYPE_UNDEFINED};
    MGOutput W{"W", MG_VALUE_TYPE_UNDEFINED};

public:
    MGDecomposeVector();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGMakeVector : public MGNode
{
    HK_CLASS(MGMakeVector, MGNode)

    MGInput  X{"X"};
    MGInput  Y{"Y"};
    MGInput  Z{"Z"};
    MGInput  W{"W"};
    MGOutput Result{"Result", MG_VALUE_TYPE_UNDEFINED};

public:
    MGMakeVector();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGSpheremapCoord : public MGNode
{
    HK_CLASS(MGSpheremapCoord, MGNode)

    MGInput  Dir{"Dir"};
    MGOutput TexCoord{"TexCoord", MG_VALUE_TYPE_FLOAT2};

public:
    MGSpheremapCoord();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGLuminance : public MGNode
{
    HK_CLASS(MGLuminance, MGNode)

    MGInput  LinearColor{"LinearColor"};
    MGOutput Luminance{"Luminance", MG_VALUE_TYPE_FLOAT1};

public:
    MGLuminance();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGPI : public MGNode
{
    HK_CLASS(MGPI, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT1};

public:
    MGPI();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MG2PI : public MGNode
{
    HK_CLASS(MG2PI, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT1};

public:
    MG2PI();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGBoolean : public MGNode
{
    HK_CLASS(MGBoolean, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_BOOL1};

public:
    bool bValue;

    MGBoolean(bool v = false);

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGBoolean2 : public MGNode
{
    HK_CLASS(MGBoolean2, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_BOOL2};

public:
    Bool2 bValue;

    MGBoolean2(Bool2 const& v = Bool2::Zero());

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGBoolean3 : public MGNode
{
    HK_CLASS(MGBoolean3, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_BOOL3};

public:
    Bool3 bValue;

    MGBoolean3(Bool3 const& v = Bool3::Zero());

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGBoolean4 : public MGNode
{
    HK_CLASS(MGBoolean4, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_BOOL4};

public:
    Bool4 bValue;

    MGBoolean4(Bool4 const& v = Bool4::Zero());

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGFloat : public MGNode
{
    HK_CLASS(MGFloat, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT1};

public:
    float fValue{};

    MGFloat(float Value = 0.0f);

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGFloat2 : public MGNode
{
    HK_CLASS(MGFloat2, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT2};

public:
    Float2 fValue;

    MGFloat2(Float2 const& Value = {});

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGFloat3 : public MGNode
{
    HK_CLASS(MGFloat3, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT3};

public:
    Float3 fValue;

    MGFloat3(Float3 const& Value = {});

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGFloat4 : public MGNode
{
    HK_CLASS(MGFloat4, MGNode)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT4};

public:
    Float4 fValue;

    MGFloat4(Float4 const& Value = {});

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGTextureSlot : public MGNode
{
    HK_CLASS(MGTextureSlot, MGNode)

    friend class MGMaterialGraph;

    MGOutput Value{"Value", MG_VALUE_TYPE_UNDEFINED};

public:
    TEXTURE_TYPE    TextureType{TEXTURE_2D};
    ETextureFilter  Filter{TEXTURE_FILTER_LINEAR};
    ETextureAddress AddressU{TEXTURE_ADDRESS_WRAP};
    ETextureAddress AddressV{TEXTURE_ADDRESS_WRAP};
    ETextureAddress AddressW{TEXTURE_ADDRESS_WRAP};
    float           MipLODBias{0};
    float           Anisotropy{16};
    float           MinLod{-1000};
    float           MaxLod{1000};

    int GetSlotIndex() const { return m_SlotIndex; }

    MGTextureSlot();

protected:
    void Compute(AMaterialBuildContext& Context) override;

private:
    int m_SlotIndex{-1};
};

class MGUniformAddress : public MGNode
{
    HK_CLASS(MGUniformAddress, MGNode)

    friend class AMaterialBuilder;

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT4};

public:
    MG_UNIFORM_TYPE UniformType{MG_UNIFORM_TYPE_FLOAT4};
    int             Address{0};

    MGUniformAddress();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGTextureLoad : public MGNode
{
    HK_CLASS(MGTextureLoad, MGNode)

    MGInput Texture{"Texture"};
    MGInput TexCoord{"TexCoord"};

    MGOutput RGBA{"RGBA", MG_VALUE_TYPE_FLOAT4};
    MGOutput RGB{"RGB", MG_VALUE_TYPE_FLOAT3};
    MGOutput R{"R", MG_VALUE_TYPE_FLOAT1};
    MGOutput G{"G", MG_VALUE_TYPE_FLOAT1};
    MGOutput B{"B", MG_VALUE_TYPE_FLOAT1};
    MGOutput A{"A", MG_VALUE_TYPE_FLOAT1};

public:
    bool               bSwappedToBGR = false;
    ETextureColorSpace ColorSpace    = TEXTURE_COLORSPACE_RGBA;

    MGTextureLoad();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGNormalLoad : public MGNode
{
    HK_CLASS(MGNormalLoad, MGNode)

    MGInput Texture{"Texture"};
    MGInput TexCoord{"TexCoord"};

    MGOutput XYZ{"XYZ", MG_VALUE_TYPE_FLOAT3};
    MGOutput X{"X", MG_VALUE_TYPE_FLOAT1};
    MGOutput Y{"Y", MG_VALUE_TYPE_FLOAT1};
    MGOutput Z{"Z", MG_VALUE_TYPE_FLOAT1};

public:
    NORMAL_MAP_PACK Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

    MGNormalLoad();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGParallaxMapLoad : public MGSingleton
{
    HK_CLASS(MGParallaxMapLoad, MGSingleton)

    MGInput Texture{"Texture"};
    MGInput TexCoord{"TexCoord"};
    MGInput DisplacementScale{"DisplacementScale"};
    //MGInput SelfShadowing{"SelfShadowing"};

    MGOutput Result{"Result", MG_VALUE_TYPE_FLOAT2};

public:
    MGParallaxMapLoad();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGVirtualTextureLoad : public MGNode
{
    HK_CLASS(MGVirtualTextureLoad, MGNode)

    MGOutput R{"R", MG_VALUE_TYPE_FLOAT1};
    MGOutput G{"G", MG_VALUE_TYPE_FLOAT1};
    MGOutput B{"B", MG_VALUE_TYPE_FLOAT1};
    MGOutput A{"A", MG_VALUE_TYPE_FLOAT1};
    MGOutput RGB{"RGB", MG_VALUE_TYPE_FLOAT3};
    MGOutput RGBA{"RGBA", MG_VALUE_TYPE_FLOAT4};

public:
    int                TextureLayer{0};
    bool               bSwappedToBGR = false;
    ETextureColorSpace ColorSpace    = TEXTURE_COLORSPACE_RGBA;

    MGVirtualTextureLoad();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGVirtualTextureNormalLoad : public MGNode
{
    HK_CLASS(MGVirtualTextureNormalLoad, MGNode)

    MGOutput X{"X", MG_VALUE_TYPE_FLOAT1};
    MGOutput Y{"Y", MG_VALUE_TYPE_FLOAT1};
    MGOutput Z{"Z", MG_VALUE_TYPE_FLOAT1};
    MGOutput XYZ{"XYZ", MG_VALUE_TYPE_FLOAT3};

public:
    int                   TextureLayer{0};
    NORMAL_MAP_PACK Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

    MGVirtualTextureNormalLoad();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGInFragmentCoord : public MGSingleton
{
    HK_CLASS(MGInFragmentCoord, MGSingleton)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT4};
    MGOutput X{"X", MG_VALUE_TYPE_FLOAT1};
    MGOutput Y{"Y", MG_VALUE_TYPE_FLOAT1};
    MGOutput Z{"Z", MG_VALUE_TYPE_FLOAT1};
    MGOutput W{"W", MG_VALUE_TYPE_FLOAT1};
    MGOutput XY{"Position", MG_VALUE_TYPE_FLOAT2};

public:
    MGInFragmentCoord();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

// Vertex position in model space
class MGInPosition : public MGSingleton
{
    HK_CLASS(MGInPosition, MGSingleton)

    MGOutput Value{"Value", MG_VALUE_TYPE_UNDEFINED};

public:
    MGInPosition();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGInNormal : public MGSingleton
{
    HK_CLASS(MGInNormal, MGSingleton)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT3};

public:
    MGInNormal();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGInColor : public MGSingleton
{
    HK_CLASS(MGInColor, MGSingleton)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT4};

public:
    MGInColor();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGInTexCoord : public MGSingleton
{
    HK_CLASS(MGInTexCoord, MGSingleton)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT2};

public:
    MGInTexCoord();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGInTimer : public MGSingleton
{
    HK_CLASS(MGInTimer, MGSingleton)

    MGOutput GameRunningTimeSeconds{"GameRunningTimeSeconds", MG_VALUE_TYPE_FLOAT1};
    MGOutput GameplayTimeSeconds{"GameplayTimeSeconds", MG_VALUE_TYPE_FLOAT1};

public:
    MGInTimer();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGInViewPosition : public MGSingleton
{
    HK_CLASS(MGInViewPosition, MGSingleton)

    MGOutput Value{"Value", MG_VALUE_TYPE_FLOAT3};

public:
    MGInViewPosition();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGCondLess : public MGNode
{
    HK_CLASS(MGCondLess, MGNode)

    MGInput  ValueA{"A"};
    MGInput  ValueB{"B"};
    MGInput  True{"True"};
    MGInput  False{"False"};
    MGOutput Result{"Result", MG_VALUE_TYPE_UNDEFINED};

public:
    MGCondLess();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

// TODO: add greater, lequal, gequal, equal, not equal

class MGAtmosphere : public MGNode
{
    HK_CLASS(MGAtmosphere, MGNode)

    MGInput  Dir{"Dir"};
    MGOutput Result{"Result", MG_VALUE_TYPE_FLOAT4};

public:
    MGAtmosphere();

protected:
    void Compute(AMaterialBuildContext& Context) override;
};

class MGMaterialGraph : public MGNode
{
    HK_CLASS(MGMaterialGraph, MGNode)

    //
    // Inputs
    //
    MGInput Color{"Color"};
    MGInput Normal{"Normal"};
    MGInput Metallic{"Metallic"};
    MGInput Roughness{"Roughness"};
    MGInput AmbientOcclusion{"AmbientOcclusion"};
    MGInput AmbientLight{"AmbientLight"};
    MGInput Emissive{"Emissive"};
    MGInput Specular{"Specular"};
    MGInput Opacity{"Opacity"};
    MGInput VertexDeform{"VertexDeform"};
    MGInput AlphaMask{"AlphaMask"};
    MGInput ShadowMask{"ShadowMask"};
    MGInput Displacement{"Displacement"};
    MGInput TessellationFactor{"TesselationFactor"};

public:
    /** Material type (Unlit,baselight,pbr,etc) */
    MATERIAL_TYPE MaterialType{MATERIAL_TYPE_UNLIT};

    /** Tessellation type */
    TESSELLATION_METHOD TessellationMethod = TESSELLATION_DISABLED;

    RENDERING_PRIORITY RenderingPriority = RENDERING_PRIORITY_DEFAULT;

    /** Blending mode */
    BLENDING_MODE Blending = COLOR_BLENDING_DISABLED;

    /** Parallax rendering technique */
    PARALLAX_TECHNIQUE ParallaxTechnique = PARALLAX_TECHNIQUE_RPM;

    /** Some materials require depth modification (e.g. first person weapon or skybox) */
    MATERIAL_DEPTH_HACK DepthHack = MATERIAL_DEPTH_HACK_NONE;

    float MotionBlurScale = 1.0f;

    float AlphaMaskCutOff = 0.5f;

    bool bDepthTest = true;

    bool bTranslucent = false;

    bool bTwoSided = false;

    bool bNoLightmap = false;

    bool bAllowScreenSpaceReflections = true;

    bool bAllowScreenAmbientOcclusion = true;

    bool bAllowShadowReceive = true;

    /** Use tessellation for shadow maps */
    bool bDisplacementAffectShadow = true;

    /** Apply fake shadows. Used with parallax technique */
    bool bParallaxMappingSelfShadowing = true;

    bool bPerBoneMotionBlur = true;

    bool bUseVirtualTexture = false;

    MGMaterialGraph();
    ~MGMaterialGraph();

    static MGMaterialGraph* LoadFromFile(IBinaryStreamReadInterface& Stream);

    template <typename T, typename... TArgs>
    T& Add2(TArgs&&... Args)
    {
        return *Add<T>(std::forward<TArgs>(Args)...);
    }

    template <typename T, typename... TArgs>
    T* Add(TArgs&&... Args)
    {
        if (T::ClassMeta().IsSubclassOf<MGSingleton>())
        {
            for (MGNode* node : m_Nodes)
            {
                if (node->FinalClassId() == T::ClassId())
                {
                    return static_cast<T*>(node);
                }
            }
        }
        MGNode* node = NewObj<T>(std::forward<TArgs>(Args)...);
        m_Nodes.Add(node);
        node->AddRef();
        node->m_ID = ++m_NodeIdGen;
        return static_cast<T*>(node);
    }

    MGNode* Add(AStringView Name);

    MGTextureSlot* GetTexture(uint32_t Slot);

    TVector<MGTextureSlot*> const& GetTextures() const { return m_TextureSlots; }

    TRef<ACompiledMaterial> Compile();

private:
    void CompileStage(class AMaterialBuildContext& ctx);

    void CreateStageTransitions(struct SMaterialStageTransition& Transition,
                                AMaterialBuildContext const*     VertexStage,
                                AMaterialBuildContext const*     TessControlStage,
                                AMaterialBuildContext const*     TessEvalStage,
                                AMaterialBuildContext const*     GeometryStage,
                                AMaterialBuildContext const*     FragmentStage);

    AString SamplersString(int MaxtextureSlot);

    void Compute(AMaterialBuildContext& Context) override;
    void ComputeVertexStage(AMaterialBuildContext& Context);
    void ComputeDepthStage(AMaterialBuildContext& Context);
    void ComputeLightStage(AMaterialBuildContext& Context);
    void ComputeShadowCastStage(AMaterialBuildContext& Context);
    void ComputeTessellationControlStage(AMaterialBuildContext& Context);
    void ComputeTessellationEvalStage(AMaterialBuildContext& Context);
    void ComputeAlphaMask(AMaterialBuildContext& Context);

    TVector<MGNode*>        m_Nodes;
    TVector<MGTextureSlot*> m_TextureSlots;
    uint32_t                m_NodeIdGen{};
};
