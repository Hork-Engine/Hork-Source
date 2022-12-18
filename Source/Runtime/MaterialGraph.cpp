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

#include "MaterialGraph.h"
#include <Platform/Logger.h>
#include <Core/Parse.h>

template <>
EnumDef const* EnumDefinition<TEXTURE_TYPE>()
{
    static const EnumDef EnumDef[] = {
        {TEXTURE_1D, "1D"},
        {TEXTURE_1D_ARRAY, "1D Array"},
        {TEXTURE_2D, "2D"},
        {TEXTURE_2D_ARRAY, "2D Array"},
        {TEXTURE_3D, "3D"},
        {TEXTURE_CUBE, "Cube"},
        {TEXTURE_CUBE_ARRAY, "Cube Array"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<MATERIAL_TYPE>()
{
    static const EnumDef EnumDef[] = {
        {MATERIAL_TYPE_UNLIT, "Unlit"},
        {MATERIAL_TYPE_BASELIGHT, "BaseLight"},
        {MATERIAL_TYPE_PBR, "PBR"},
        {MATERIAL_TYPE_HUD, "HUD"},
        {MATERIAL_TYPE_POSTPROCESS, "Postprocess"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<TESSELLATION_METHOD>()
{
    static const EnumDef EnumDef[] = {
        {TESSELLATION_DISABLED, "Disabled"},
        {TESSELLATION_FLAT, "Flat"},
        {TESSELLATION_PN, "PN"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<RENDERING_PRIORITY>()
{
    static const EnumDef EnumDef[] = {
        {RENDERING_PRIORITY_WEAPON, "Weapon"},
        {RENDERING_PRIORITY_FOLIAGE, "Foliage"},
        {RENDERING_PRIORITY_DEFAULT, "Default"},
        {RENDERING_PRIORITY_RESERVED3, "Reserved3"},
        {RENDERING_PRIORITY_RESERVED4, "Reserved4"},
        {RENDERING_PRIORITY_RESERVED5, "Reserved5"},
        {RENDERING_PRIORITY_RESERVED6, "Reserved6"},
        {RENDERING_PRIORITY_RESERVED7, "Reserved7"},
        {RENDERING_PRIORITY_RESERVED8, "Reserved8"},
        {RENDERING_PRIORITY_RESERVED9, "Reserved9"},
        {RENDERING_PRIORITY_RESERVED10, "Reserved10"},
        {RENDERING_PRIORITY_RESERVED11, "Reserved11"},
        {RENDERING_PRIORITY_RESERVED12, "Reserved12"},
        {RENDERING_PRIORITY_RESERVED13, "Reserved13"},
        {RENDERING_PRIORITY_RESERVED14, "Reserved14"},
        {RENDERING_PRIORITY_SKYBOX, "Skybox"},
        {0, nullptr}};
    return EnumDef;
};

template <>
EnumDef const* EnumDefinition<BLENDING_MODE>()
{
    static const EnumDef EnumDef[] = {
        {COLOR_BLENDING_ALPHA, "Alpha"},
        {COLOR_BLENDING_DISABLED, "Disabled"},
        {COLOR_BLENDING_PREMULTIPLIED_ALPHA, "Premultiplied Alpha"},
        {COLOR_BLENDING_COLOR_ADD, "Color Add"},
        {COLOR_BLENDING_MULTIPLY, "Multiply"},
        {COLOR_BLENDING_SOURCE_TO_DEST, "Source To Dest"},
        {COLOR_BLENDING_ADD_MUL, "Add Multiply"},
        {COLOR_BLENDING_ADD_ALPHA, "Add Alpha"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<PARALLAX_TECHNIQUE>()
{
    static const EnumDef EnumDef[] = {
        {PARALLAX_TECHNIQUE_DISABLED, "Disabled"},
        {PARALLAX_TECHNIQUE_POM, "POM"},
        {PARALLAX_TECHNIQUE_RPM, "RPM"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<MATERIAL_DEPTH_HACK>()
{
    static const EnumDef EnumDef[] = {
        {MATERIAL_DEPTH_HACK_NONE, "None"},
        {MATERIAL_DEPTH_HACK_WEAPON, "Weapon"},
        {MATERIAL_DEPTH_HACK_SKYBOX, "Skybox"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<TEXTURE_FILTER>()
{
    static const EnumDef EnumDef[] = {
        {TEXTURE_FILTER_LINEAR, "Linear"},
        {TEXTURE_FILTER_NEAREST, "Nearest"},
        {TEXTURE_FILTER_MIPMAP_NEAREST, "Mipmap Nearest"},
        {TEXTURE_FILTER_MIPMAP_BILINEAR, "Bilinear"},
        {TEXTURE_FILTER_MIPMAP_NLINEAR, "NLinear"},
        {TEXTURE_FILTER_MIPMAP_TRILINEAR, "Trilinear"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<TEXTURE_ADDRESS>()
{
    static const EnumDef EnumDef[] = {
        {TEXTURE_ADDRESS_WRAP, "Wrap"},
        {TEXTURE_ADDRESS_MIRROR, "Mirror"},
        {TEXTURE_ADDRESS_CLAMP, "Clamp"},
        {TEXTURE_ADDRESS_BORDER, "Border"},
        {TEXTURE_ADDRESS_MIRROR_ONCE, "Mirror Once"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<MG_VALUE_TYPE>()
{
    static const EnumDef EnumDef[] = {
        {MG_VALUE_TYPE_UNDEFINED, "Undefined"},
        {MG_VALUE_TYPE_FLOAT1, "float"},
        {MG_VALUE_TYPE_FLOAT2, "float2"},
        {MG_VALUE_TYPE_FLOAT3, "float3"},
        {MG_VALUE_TYPE_FLOAT4, "float4"},
        {MG_VALUE_TYPE_BOOL1, "bool"},
        {MG_VALUE_TYPE_BOOL2, "bool2"},
        {MG_VALUE_TYPE_BOOL3, "bool3"},
        {MG_VALUE_TYPE_BOOL4, "bool4"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<MG_UNIFORM_TYPE>()
{
    static const EnumDef EnumDef[] = {
        {MG_UNIFORM_TYPE_UNDEFINED, "Undefined"},
        {MG_UNIFORM_TYPE_FLOAT1, "float"},
        {MG_UNIFORM_TYPE_FLOAT2, "float2"},
        {MG_UNIFORM_TYPE_FLOAT3, "float3"},
        {MG_UNIFORM_TYPE_FLOAT4, "float4"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<TEXTURE_COLOR_SPACE>()
{
    static const EnumDef EnumDef[] = {
        {TEXTURE_COLOR_SPACE_RGBA, "RGBA"},
        {TEXTURE_COLOR_SPACE_SRGB_ALPHA, "SRGBA"},
        {TEXTURE_COLOR_SPACE_YCOCG, "YCoCg"},
        {TEXTURE_COLOR_SPACE_GRAYSCALED, "Grayscaled"},
        {0, nullptr}};
    return EnumDef;
}
template <>
EnumDef const* EnumDefinition<NORMAL_MAP_PACK>()
{
    static const EnumDef EnumDef[] = {
        {NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE, "RGBx"},
        {NORMAL_MAP_PACK_RG_BC5_COMPATIBLE, "RG"},
        {NORMAL_MAP_PACK_SPHEREMAP_BC5_COMPATIBLE, "Spheremap (RG)"},
        {NORMAL_MAP_PACK_STEREOGRAPHIC_BC5_COMPATIBLE, "Stereographic (RG)"},
        {NORMAL_MAP_PACK_PARABOLOID_BC5_COMPATIBLE, "Paraboloid (RG)"},
        {NORMAL_MAP_PACK_RGBA_BC3_COMPATIBLE, "xGBR"},
        {0, nullptr}};
    return EnumDef;
}

enum MATERIAL_STAGE
{
    MATERIAL_STAGE_VERTEX,
    MATERIAL_STAGE_TESSELLATION_CONTROL,
    MATERIAL_STAGE_TESSELLATION_EVAL,
    MATERIAL_STAGE_GEOMETRY,
    MATERIAL_STAGE_DEPTH,
    MATERIAL_STAGE_LIGHT,
    MATERIAL_STAGE_SHADOWCAST
};

struct StageVarying
{
    StageVarying(StringView Name, StringView Source, MG_VALUE_TYPE Type) :
        VaryingName(Name), VaryingSource(Source), VaryingType(Type), RefCount(0)
    {}

    String       VaryingName;
    String       VaryingSource;
    MG_VALUE_TYPE VaryingType;
    int           RefCount;
};

class MaterialBuildContext
{
public:
    String            SourceCode;
    int               MaxTextureSlot{-1};
    int               MaxUniformAddress{-1};
    int               ParallaxSampler{-1};
    bool              bHasVertexDeform{};
    bool              bHasDisplacement{};
    bool              bHasAlphaMask{};
    bool              bHasShadowMask{};
    TVector<StageVarying> InputVaryings;
    int               Serial{};

    MaterialBuildContext(MGMaterialGraph const* Graph, MATERIAL_STAGE Stage) :
        m_Graph(Graph), m_Stage(Stage)
    {}

    int GetBuildSerial() const
    {
        return Serial;
    }

    String GenerateVariableName() const;

    void GenerateSourceCode(MGOutput& Slot, StringView Expression, bool bAddBrackets);

    MATERIAL_STAGE GetStage() const
    {
        return m_Stage;
    }

    MATERIAL_TYPE GetMaterialType() const
    {
        return m_Graph->MaterialType;
    }

    MGMaterialGraph const* GetGraph()
    {
        return m_Graph;
    }

private:
    mutable int            m_VariableName{};
    MATERIAL_STAGE         m_Stage;
    MGMaterialGraph const* m_Graph;
};

static constexpr const char* VariableTypeStr[] = {
    "vec4",  // MG_VALUE_TYPE_UNDEFINED
    "float", // MG_VALUE_TYPE_FLOAT1
    "vec2",  // MG_VALUE_TYPE_FLOAT2
    "vec3",  // MG_VALUE_TYPE_FLOAT3
    "vec4",  // MG_VALUE_TYPE_FLOAT4
    "bool",  // MG_VALUE_TYPE_BOOL1
    "bvec2", // MG_VALUE_TYPE_BOOL2
    "bvec3", // MG_VALUE_TYPE_BOOL3
    "bvec4", // MG_VALUE_TYPE_BOOL4
};

enum MG_VECTOR_TYPE
{
    MG_VECTOR_UNDEFINED,
    MG_VECTOR_VEC1,
    MG_VECTOR_VEC2,
    MG_VECTOR_VEC3,
    MG_VECTOR_VEC4
};

enum MG_COMPONENT_TYPE
{
    MG_COMPONENT_UNKNOWN,
    MG_COMPONENT_FLOAT,
    MG_COMPONENT_BOOL
};

static bool IsTypeComponent(MG_VALUE_TYPE InType)
{
    switch (InType)
    {
        case MG_VALUE_TYPE_FLOAT1:
        case MG_VALUE_TYPE_BOOL1:
            return true;
        default:
            break;
    }
    return false;
}

static bool IsTypeVector(MG_VALUE_TYPE InType)
{
    return !IsTypeComponent(InType);
}

static MG_COMPONENT_TYPE GetTypeComponent(MG_VALUE_TYPE InType)
{
    switch (InType)
    {
        case MG_VALUE_TYPE_FLOAT1:
        case MG_VALUE_TYPE_FLOAT2:
        case MG_VALUE_TYPE_FLOAT3:
        case MG_VALUE_TYPE_FLOAT4:
            return MG_COMPONENT_FLOAT;
        case MG_VALUE_TYPE_BOOL1:
        case MG_VALUE_TYPE_BOOL2:
        case MG_VALUE_TYPE_BOOL3:
        case MG_VALUE_TYPE_BOOL4:
            return MG_COMPONENT_BOOL;
        default:
            break;
    }
    return MG_COMPONENT_UNKNOWN;
}

static MG_VECTOR_TYPE GetTypeVector(MG_VALUE_TYPE Type)
{
    switch (Type)
    {
        case MG_VALUE_TYPE_FLOAT1:
        case MG_VALUE_TYPE_BOOL1:
            return MG_VECTOR_VEC1;
        case MG_VALUE_TYPE_FLOAT2:
        case MG_VALUE_TYPE_BOOL2:
            return MG_VECTOR_VEC2;
        case MG_VALUE_TYPE_FLOAT3:
        case MG_VALUE_TYPE_BOOL3:
            return MG_VECTOR_VEC3;
        case MG_VALUE_TYPE_FLOAT4:
        case MG_VALUE_TYPE_BOOL4:
            return MG_VECTOR_VEC4;
        default:
            break;
    }
    return MG_VECTOR_UNDEFINED;
}

static bool IsArithmeticType(MG_VALUE_TYPE Type)
{
    switch (GetTypeComponent(Type))
    {
        case MG_COMPONENT_FLOAT:
            return true;
        default:
            break;
    }

    return false;
}

static MG_VALUE_TYPE ToFloatType(MG_VALUE_TYPE Type)
{
    switch (Type)
    {
        case MG_VALUE_TYPE_FLOAT1:
        case MG_VALUE_TYPE_FLOAT2:
        case MG_VALUE_TYPE_FLOAT3:
        case MG_VALUE_TYPE_FLOAT4:
            return Type;
        case MG_VALUE_TYPE_BOOL1:
            return MG_VALUE_TYPE_FLOAT1;
        case MG_VALUE_TYPE_BOOL2:
            return MG_VALUE_TYPE_FLOAT2;
        case MG_VALUE_TYPE_BOOL3:
            return MG_VALUE_TYPE_FLOAT3;
        case MG_VALUE_TYPE_BOOL4:
            return MG_VALUE_TYPE_FLOAT4;
        default:
            break;
    }
    return MG_VALUE_TYPE_FLOAT1;
}

enum MG_VECTOR_CAST_FLAGS : uint8_t
{
    MG_VECTOR_CAST_UNDEFINED   = 0,
    MG_VECTOR_CAST_IDENTITY_X  = HK_BIT(0),
    MG_VECTOR_CAST_IDENTITY_Y  = HK_BIT(1),
    MG_VECTOR_CAST_IDENTITY_Z  = HK_BIT(2),
    MG_VECTOR_CAST_IDENTITY_W  = HK_BIT(3),
    MG_VECTOR_CAST_EXPAND_VEC1 = HK_BIT(4),
};

HK_FLAG_ENUM_OPERATORS(MG_VECTOR_CAST_FLAGS)

static String MakeVectorCast(String const& Expression, MG_VALUE_TYPE TypeFrom, MG_VALUE_TYPE TypeTo, MG_VECTOR_CAST_FLAGS VectorCastFlags = MG_VECTOR_CAST_UNDEFINED)
{
    if (TypeFrom == TypeTo || TypeTo == MG_VALUE_TYPE_UNDEFINED)
    {
        return Expression;
    }

    MG_COMPONENT_TYPE componentFrom = GetTypeComponent(TypeFrom);
    MG_COMPONENT_TYPE componentTo   = GetTypeComponent(TypeTo);

    bool bSameComponentType = componentFrom == componentTo;

    const char* zero = "0";
    const char* one  = "1";
    switch (componentTo)
    {
        case MG_COMPONENT_FLOAT:
            zero = "0.0";
            one  = "1.0";
            break;
        case MG_COMPONENT_BOOL:
            zero = "false";
            one  = "true";
            break;
        default:
            HK_ASSERT(0);
    }

    String defX = VectorCastFlags & MG_VECTOR_CAST_IDENTITY_X ? one : zero;
    String defY = VectorCastFlags & MG_VECTOR_CAST_IDENTITY_Y ? one : zero;
    String defZ = VectorCastFlags & MG_VECTOR_CAST_IDENTITY_Z ? one : zero;
    String defW = VectorCastFlags & MG_VECTOR_CAST_IDENTITY_W ? one : zero;

    MG_VECTOR_TYPE vecType = GetTypeVector(TypeFrom);

    switch (vecType)
    {
        case MG_VECTOR_UNDEFINED:
            switch (TypeTo)
            {
                case MG_VALUE_TYPE_FLOAT1:
                    return defX;
                case MG_VALUE_TYPE_FLOAT2:
                    return "vec2( " + defX + ", " + defY + " )";
                case MG_VALUE_TYPE_FLOAT3:
                    return "vec3( " + defX + ", " + defY + ", " + defZ + " )";
                case MG_VALUE_TYPE_FLOAT4:
                    return "vec4( " + defX + ", " + defY + ", " + defZ + ", " + defW + " )";
                case MG_VALUE_TYPE_BOOL1:
                    return defX;
                case MG_VALUE_TYPE_BOOL2:
                    return "bvec2( " + defX + ", " + defY + " )";
                case MG_VALUE_TYPE_BOOL3:
                    return "bvec3( " + defX + ", " + defY + ", " + defZ + " )";
                case MG_VALUE_TYPE_BOOL4:
                    return "bvec4( " + defX + ", " + defY + ", " + defZ + ", " + defW + " )";
                default:
                    break;
            }
            break;
        case MG_VECTOR_VEC1:
            if (VectorCastFlags & MG_VECTOR_CAST_EXPAND_VEC1)
            {
                // Conversion like: vecN( in, in, ... )
                switch (TypeTo)
                {
                    case MG_VALUE_TYPE_FLOAT1:
                        return bSameComponentType ? Expression : "float( " + Expression + " )";
                    case MG_VALUE_TYPE_FLOAT2:
                        return bSameComponentType ? "vec2( " + Expression + " )" : "vec2( float(" + Expression + ") )";
                    case MG_VALUE_TYPE_FLOAT3:
                        return bSameComponentType ? "vec3( " + Expression + " )" : "vec3( float(" + Expression + ") )";
                    case MG_VALUE_TYPE_FLOAT4:
                        return bSameComponentType ? "vec4( " + Expression + " )" : "vec4( float(" + Expression + ") )";
                    case MG_VALUE_TYPE_BOOL1:
                        return bSameComponentType ? Expression : "bool(" + Expression + ")";
                    case MG_VALUE_TYPE_BOOL2:
                        return bSameComponentType ? "bvec2( " + Expression + " )" : "bvec2( bool(" + Expression + ") )";
                    case MG_VALUE_TYPE_BOOL3:
                        return bSameComponentType ? "bvec3( " + Expression + " )" : "bvec3( bool(" + Expression + ") )";
                    case MG_VALUE_TYPE_BOOL4:
                        return bSameComponentType ? "bvec4( " + Expression + " )" : "bvec4( bool(" + Expression + ") )";
                    default:
                        break;
                }
                break;
            }
            else
            {
                // Conversion like: vecN( in, defY, defZ, defW )
                switch (TypeTo)
                {
                    case MG_VALUE_TYPE_FLOAT1:
                        return bSameComponentType ? Expression : "float( " + Expression + " )";
                    case MG_VALUE_TYPE_FLOAT2:
                        return bSameComponentType ? "vec2( " + Expression + ", " + defY + " )" : "vec2( float(" + Expression + "), " + defY + " )";
                    case MG_VALUE_TYPE_FLOAT3:
                        return bSameComponentType ? "vec3( " + Expression + ", " + defY + ", " + defZ + " )" : "vec3( float(" + Expression + "), " + defY + ", " + defZ + " )";
                    case MG_VALUE_TYPE_FLOAT4:
                        return bSameComponentType ? "vec4( " + Expression + ", " + defY + ", " + defZ + ", " + defW + " )" : "vec4( float(" + Expression + "), " + defY + ", " + defZ + ", " + defW + " )";
                    case MG_VALUE_TYPE_BOOL1:
                        return bSameComponentType ? Expression : "bool( " + Expression + " )";
                    case MG_VALUE_TYPE_BOOL2:
                        return bSameComponentType ? "bvec2( " + Expression + ", " + defY + " )" : "bvec2( bool(" + Expression + "), " + defY + " )";
                    case MG_VALUE_TYPE_BOOL3:
                        return bSameComponentType ? "bvec3( " + Expression + ", " + defY + ", " + defZ + " )" : "bvec3( bool(" + Expression + "), " + defY + ", " + defZ + " )";
                    case MG_VALUE_TYPE_BOOL4:
                        return bSameComponentType ? "bvec4( " + Expression + ", " + defY + ", " + defZ + ", " + defW + " )" : "bvec4( bool(" + Expression + "), " + defY + ", " + defZ + ", " + defW + " )";
                    default:
                        break;
                }
                break;
            }
        case MG_VECTOR_VEC2:
            switch (TypeTo)
            {
                case MG_VALUE_TYPE_FLOAT1:
                    return bSameComponentType ? Expression + ".x" : "float( " + Expression + ".x )";
                case MG_VALUE_TYPE_FLOAT2:
                    return bSameComponentType ? Expression : "vec2( " + Expression + " )";
                case MG_VALUE_TYPE_FLOAT3:
                    return bSameComponentType ? "vec3( " + Expression + ", " + defZ + " )" : "vec3( vec2(" + Expression + "), " + defZ + " )";
                case MG_VALUE_TYPE_FLOAT4:
                    return bSameComponentType ? "vec4( " + Expression + ", " + defZ + ", " + defW + " )" : "vec4( vec2(" + Expression + "), " + defZ + ", " + defW + " )";
                case MG_VALUE_TYPE_BOOL1:
                    return bSameComponentType ? Expression + ".x" : "bool(" + Expression + ".x )";
                case MG_VALUE_TYPE_BOOL2:
                    return bSameComponentType ? Expression : "bvec2( " + Expression + " )";
                case MG_VALUE_TYPE_BOOL3:
                    return bSameComponentType ? "bvec3( " + Expression + ", " + defZ + " )" : "bvec3( bvec2(" + Expression + "), " + defZ + " )";
                case MG_VALUE_TYPE_BOOL4:
                    return bSameComponentType ? "bvec4( " + Expression + ", " + defZ + ", " + defW + " )" : "bvec4( bvec2(" + Expression + "), " + defZ + ", " + defW + " )";
                default:
                    break;
            }
            break;
        case MG_VECTOR_VEC3:
            switch (TypeTo)
            {
                case MG_VALUE_TYPE_FLOAT1:
                    return bSameComponentType ? Expression + ".x" : "float( " + Expression + ".x )";
                case MG_VALUE_TYPE_FLOAT2:
                    return bSameComponentType ? Expression + ".xy" : "vec2( " + Expression + ".xy )";
                case MG_VALUE_TYPE_FLOAT3:
                    return bSameComponentType ? Expression : "vec3( " + Expression + " )";
                case MG_VALUE_TYPE_FLOAT4:
                    return bSameComponentType ? "vec4( " + Expression + ", " + defW + " )" : "vec4( vec3(" + Expression + "), " + defW + " )";
                case MG_VALUE_TYPE_BOOL1:
                    return bSameComponentType ? Expression + ".x" : "bool(" + Expression + ".x)";
                case MG_VALUE_TYPE_BOOL2:
                    return bSameComponentType ? Expression + ".xy" : "bvec2(" + Expression + ".xy)";
                case MG_VALUE_TYPE_BOOL3:
                    return bSameComponentType ? Expression : "bvec3( " + Expression + " )";
                case MG_VALUE_TYPE_BOOL4:
                    return bSameComponentType ? "bvec4( " + Expression + ", " + defW + " )" : "bvec4( bvec3(" + Expression + "), " + defW + " )";
                default:
                    break;
            }
            break;
        case MG_VECTOR_VEC4:
            switch (TypeTo)
            {
                case MG_VALUE_TYPE_FLOAT1:
                    return bSameComponentType ? Expression + ".x" : "float( " + Expression + ".x )";
                case MG_VALUE_TYPE_FLOAT2:
                    return bSameComponentType ? Expression + ".xy" : "vec2( " + Expression + ".xy )";
                case MG_VALUE_TYPE_FLOAT3:
                    return bSameComponentType ? Expression + ".xyz" : "vec3( " + Expression + ".xyz )";
                case MG_VALUE_TYPE_FLOAT4:
                    return bSameComponentType ? Expression : "vec4( " + Expression + " )";
                case MG_VALUE_TYPE_BOOL1:
                    return bSameComponentType ? Expression + ".x" : "bool(" + Expression + ".x )";
                case MG_VALUE_TYPE_BOOL2:
                    return bSameComponentType ? Expression + ".xy" : "bvec2(" + Expression + ".xy )";
                case MG_VALUE_TYPE_BOOL3:
                    return bSameComponentType ? Expression + ".xyz" : "bvec3(" + Expression + ".xyz )";
                case MG_VALUE_TYPE_BOOL4:
                    return bSameComponentType ? Expression : "bvec4(" + Expression + ")";
                default:
                    break;
            }
            break;
    }

    HK_ASSERT(0);

    return Expression;
}

static const char* MakeEmptyVector(MG_VALUE_TYPE InType)
{
    switch (InType)
    {
        case MG_VALUE_TYPE_FLOAT1:
            return "0.0";
        case MG_VALUE_TYPE_FLOAT2:
            return "vec2( 0.0 )";
        case MG_VALUE_TYPE_FLOAT3:
            return "vec3( 0.0 )";
        case MG_VALUE_TYPE_FLOAT4:
            return "vec4( 0.0 )";
        case MG_VALUE_TYPE_BOOL1:
            return "false";
        case MG_VALUE_TYPE_BOOL2:
            return "bvec2( false )";
        case MG_VALUE_TYPE_BOOL3:
            return "bvec3( false )";
        case MG_VALUE_TYPE_BOOL4:
            return "bvec4( false )";
        default:
            break;
    };
    return "0.0";
}

static const char* MakeDefaultNormal()
{
    return "vec3( 0.0, 0.0, 1.0 )";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

String MaterialBuildContext::GenerateVariableName() const
{
    return String("v") + Core::ToString(m_VariableName++);
}

void MaterialBuildContext::GenerateSourceCode(MGOutput& Slot, StringView Expression, bool bAddBrackets)
{
    if (Slot.Usages > 1)
    {
        Slot.Expression = GenerateVariableName();
        SourceCode += "const " + String(VariableTypeStr[Slot.Type]) + " " + Slot.Expression + " = " + Expression + ";\n";
    }
    else
    {
        if (bAddBrackets)
        {
            Slot.Expression = "( " + Expression + " )";
        }
        else
        {
            Slot.Expression = Expression;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGNode)
HK_PROPERTY_DIRECT(Location, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

HK_BEGIN_CLASS_META(MGSingleton)
HK_END_CLASS_META()

MGNode::MGNode(StringView Name) :
    m_Name(Name)
{}

void MGNode::ParseProperties(DocumentValue const* document)
{
    PropertyList properties;

    GetProperties(properties);

    for (Property const* prop : properties)
    {
        StringView value = document->GetString(prop->GetName());
        if (!value)
            continue;

        prop->SetValue(this, value);
    }
}

void MGNode::SetInputs(std::initializer_list<MGInput*> Inputs)
{
    m_Inputs = Inputs;
}

void MGNode::SetOutputs(std::initializer_list<MGOutput*> Outputs)
{
    m_Outputs = Outputs;
    for (MGOutput* output : m_Outputs)
    {
        output->m_Owner = this;
    }
}

MGNode& MGNode::BindInput(StringView InputSlot, MGNode* Node)
{
    if (Node)
        return BindInput(InputSlot, *Node);

    UnbindInput(InputSlot);
    return *this;
}

MGNode& MGNode::BindInput(StringView InputSlot, MGNode& Node)
{
    if (Node.m_Outputs.IsEmpty())
    {
        LOG("MGNode::BindInput: Node '{}' has no output slots\n", Node.m_Name);
        return *this;
    }
    return BindInput(InputSlot, Node.m_Outputs[0]);
}

MGNode& MGNode::BindInput(StringView InputSlot, MGOutput& pSlot)
{
    return BindInput(InputSlot, &pSlot);
}
MGNode& MGNode::BindInput(StringView InputSlot, MGOutput* pSlot)
{
    for (auto input : m_Inputs)
    {
        if (!InputSlot.Icmp(input->GetName()))
        {
            if (!pSlot)
            {
                // Unbind
                input->m_Slot = nullptr;
                input->m_SlotNode.Reset();
                return *this;
            }

            MGNode* node = pSlot->m_Owner;
            if (!node)
            {
                LOG("MGNode::BindInput: Node output is incomplete\n");
                return *this;
            }
            input->m_Slot     = pSlot;
            input->m_SlotNode = node;
            return *this;
        }
    }
    LOG("MGNode::Input: Unknown input slot {}\n", InputSlot);
    return *this;
}

void MGNode::UnbindInput(StringView InputSlot)
{
    BindInput(InputSlot, (MGOutput*)nullptr);
}

MGOutput* MGNode::FindOutput(StringView _Name)
{
    for (MGOutput* Out : m_Outputs)
    {
        if (!Out->GetName().Icmp(_Name))
        {
            return Out;
        }
    }
    return nullptr;
}

bool MGNode::Build(MaterialBuildContext& Context)
{
    if (m_Serial == Context.GetBuildSerial())
    {
        return true;
    }

    //if ( !(Stages & Context.GetStageMask() ) ) {
    //    return false;
    //}

    m_Serial = Context.GetBuildSerial();

    Compute(Context);
    return true;
}

void MGNode::ResetConnections(MaterialBuildContext const& Context)
{
    if (!m_bTouched)
    {
        return;
    }

    m_bTouched = false;

    for (MGInput* in : m_Inputs)
    {
        MGOutput* out = in->GetConnection();
        if (out)
        {
            MGNode* node = in->ConnectedNode();
            node->ResetConnections(Context);
            out->Usages = 0;
        }
    }
}

void MGNode::TouchConnections(MaterialBuildContext const& Context)
{
    if (m_bTouched)
    {
        return;
    }

    m_bTouched = true;

    for (MGInput* in : m_Inputs)
    {
        MGOutput* out = in->GetConnection();
        if (out)
        {
            MGNode* node = in->ConnectedNode();
            node->TouchConnections(Context);
            out->Usages++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static String MakeExpression(MaterialBuildContext& Context, MGInput& Input, MG_VALUE_TYPE DesiredType, StringView DefaultExpression, MG_VECTOR_CAST_FLAGS VectorCastFlags = MG_VECTOR_CAST_UNDEFINED)
{
    MGOutput* connection = Input.GetConnection();

    if (connection && Input.ConnectedNode()->Build(Context))
    {
        return MakeVectorCast(connection->Expression, connection->Type, DesiredType, VectorCastFlags);
    }

    return DefaultExpression; //MakeEmptyVector(DesiredType);
}

void MGMaterialGraph::Compute(MaterialBuildContext& Context)
{
    Super::Compute(Context);

    switch (Context.GetStage())
    {
        case MATERIAL_STAGE_VERTEX:
            ComputeVertexStage(Context);
            break;
        case MATERIAL_STAGE_TESSELLATION_CONTROL:
            ComputeTessellationControlStage(Context);
            break;
        case MATERIAL_STAGE_TESSELLATION_EVAL:
            ComputeTessellationEvalStage(Context);
            break;
        case MATERIAL_STAGE_GEOMETRY:
            break;
        case MATERIAL_STAGE_DEPTH:
            ComputeDepthStage(Context);
            break;
        case MATERIAL_STAGE_LIGHT:
            ComputeLightStage(Context);
            break;
        case MATERIAL_STAGE_SHADOWCAST:
            ComputeShadowCastStage(Context);
            break;
    }
}

void MGMaterialGraph::ComputeVertexStage(MaterialBuildContext& Context)
{
    MGOutput* positionCon = VertexDeform.GetConnection();

    Context.bHasVertexDeform = false;

    //String TransformMatrix;
    //if ( Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
    //    TransformMatrix = "OrthoProjection";
    //} else {
    //    TransformMatrix = "TransformMatrix";
    //}

    if (positionCon && VertexDeform.ConnectedNode()->Build(Context))
    {
        if (positionCon->Expression != "VertexPosition")
        {
            Context.bHasVertexDeform = true;
        }

        String expression = MakeVectorCast(positionCon->Expression, positionCon->Type, MG_VALUE_TYPE_FLOAT4, MG_VECTOR_CAST_IDENTITY_W);

        Context.SourceCode += "vec4 FinalVertexPos = " + expression + ";\n";
    }
    else
    {
        Context.SourceCode += "vec4 FinalVertexPos = vec4( VertexPosition, 1.0 );\n";
    }
}

void MGMaterialGraph::ComputeDepthStage(MaterialBuildContext& Context)
{
    ComputeAlphaMask(Context);
}

void MGMaterialGraph::ComputeLightStage(MaterialBuildContext& Context)
{
    String expression;

    // Color
    expression = MakeExpression(Context, Color, MG_VALUE_TYPE_FLOAT4, MakeEmptyVector(MG_VALUE_TYPE_FLOAT4), MG_VECTOR_CAST_EXPAND_VEC1);
    Context.SourceCode += "vec4 BaseColor = " + expression + ";\n";

    if (Context.GetMaterialType() == MATERIAL_TYPE_PBR || Context.GetMaterialType() == MATERIAL_TYPE_BASELIGHT)
    {
        // Normal
        expression = MakeExpression(Context, Normal, MG_VALUE_TYPE_FLOAT3, MakeDefaultNormal());
        Context.SourceCode += "vec3 MaterialNormal = " + expression + ";\n";

        // Emissive
        expression = MakeExpression(Context, Emissive, MG_VALUE_TYPE_FLOAT3, MakeEmptyVector(MG_VALUE_TYPE_FLOAT3), MG_VECTOR_CAST_EXPAND_VEC1);
        Context.SourceCode += "vec3 MaterialEmissive = " + expression + ";\n";

        // Specular
        expression = MakeExpression(Context, Specular, MG_VALUE_TYPE_FLOAT3, MakeEmptyVector(MG_VALUE_TYPE_FLOAT3), MG_VECTOR_CAST_EXPAND_VEC1);
        Context.SourceCode += "vec3 MaterialSpecular = " + expression + ";\n";

        // Ambient Light
        expression = MakeExpression(Context, AmbientLight, MG_VALUE_TYPE_FLOAT3, MakeEmptyVector(MG_VALUE_TYPE_FLOAT3), MG_VECTOR_CAST_EXPAND_VEC1);
        Context.SourceCode += "vec3 MaterialAmbientLight = " + expression + ";\n";
    }

    if (Context.GetMaterialType() == MATERIAL_TYPE_PBR)
    {
        // Metallic
        expression = MakeExpression(Context, Metallic, MG_VALUE_TYPE_FLOAT1, MakeEmptyVector(MG_VALUE_TYPE_FLOAT1));
        Context.SourceCode += "float MaterialMetallic = saturate( " + expression + " );\n";

        // Roughness
        expression = MakeExpression(Context, Roughness, MG_VALUE_TYPE_FLOAT1, "1.0");
        Context.SourceCode += "float MaterialRoughness = saturate( " + expression + " );\n";

        // Ambient Occlusion
        expression = MakeExpression(Context, AmbientOcclusion, MG_VALUE_TYPE_FLOAT1, "1.0");
        Context.SourceCode += "float MaterialAmbientOcclusion = saturate( " + expression + " );\n";
    }

    // Opacity
    if (Context.GetGraph()->bTranslucent)
    {
        expression = MakeExpression(Context, Opacity, MG_VALUE_TYPE_FLOAT1, "1.0");
        Context.SourceCode += "float Opacity = saturate( " + expression + " );\n";
    }
    else
    {
        Context.SourceCode += "const float Opacity = 1.0;\n";
    }

    // NOTE. It is not necessary to calculate an alpha mask for opaque materials, but there may still be cases where translucent materials use an alpha test.
    if (bTranslucent)
        ComputeAlphaMask(Context);
}

void MGMaterialGraph::ComputeShadowCastStage(MaterialBuildContext& Context)
{
    MGOutput* con = ShadowMask.GetConnection();

    if (con && ShadowMask.ConnectedNode()->Build(Context))
    {
        Context.bHasShadowMask = true;

        String cutoff = Core::ToString(AlphaMaskCutOff); // FIXME: Add ShadowMaskCutOff?

        switch (con->Type)
        {
            case MG_VALUE_TYPE_FLOAT1:
                Context.SourceCode += "if ( " + con->Expression + " < " + cutoff + " ) discard;\n";
                break;
            case MG_VALUE_TYPE_FLOAT2:
            case MG_VALUE_TYPE_FLOAT3:
            case MG_VALUE_TYPE_FLOAT4:
                Context.SourceCode += "if ( " + con->Expression + ".x < " + cutoff + " ) discard;\n";
                break;
            case MG_VALUE_TYPE_BOOL1:
                Context.SourceCode += "if ( " + con->Expression + " == false ) discard;\n";
                break;
            case MG_VALUE_TYPE_BOOL2:
            case MG_VALUE_TYPE_BOOL3:
            case MG_VALUE_TYPE_BOOL4:
                Context.SourceCode += "if ( " + con->Expression + ".x == false ) discard;\n";
                break;
            default:
                break;
        }
    }
}

void MGMaterialGraph::ComputeTessellationControlStage(MaterialBuildContext& Context)
{
    MGOutput* tessFactorCon = TessellationFactor.GetConnection();

    if (tessFactorCon && TessellationFactor.ConnectedNode()->Build(Context))
    {
        String expression = MakeVectorCast(tessFactorCon->Expression, tessFactorCon->Type, MG_VALUE_TYPE_FLOAT1);

        Context.SourceCode += "float TessellationFactor = " + expression + ";\n";
    }
    else
    {
        Context.SourceCode += "const float TessellationFactor = 1.0;\n";
    }
}

void MGMaterialGraph::ComputeTessellationEvalStage(MaterialBuildContext& Context)
{
    MGOutput* displacementCon = Displacement.GetConnection();

    Context.bHasDisplacement = false;

    if (displacementCon && Displacement.ConnectedNode()->Build(Context))
    {
        Context.bHasDisplacement = true;

        String expression = MakeVectorCast(displacementCon->Expression, displacementCon->Type, MG_VALUE_TYPE_FLOAT1);

        Context.SourceCode += "float Displacement = " + expression + ";\n";
    }
    else
    {
        Context.SourceCode += "const float Displacement = 0.0;\n";
    }
}

void MGMaterialGraph::ComputeAlphaMask(MaterialBuildContext& Context)
{
    MGOutput* con = AlphaMask.GetConnection();

    if (con && AlphaMask.ConnectedNode()->Build(Context))
    {
        Context.bHasAlphaMask = true;

        String cutoff = Core::ToString(AlphaMaskCutOff);

        switch (con->Type)
        {
            case MG_VALUE_TYPE_FLOAT1:
                Context.SourceCode += "if ( " + con->Expression + " < " + cutoff + " ) discard;\n";
                break;
            case MG_VALUE_TYPE_FLOAT2:
            case MG_VALUE_TYPE_FLOAT3:
            case MG_VALUE_TYPE_FLOAT4:
                Context.SourceCode += "if ( " + con->Expression + ".x < " + cutoff + " ) discard;\n";
                break;
            case MG_VALUE_TYPE_BOOL1:
                Context.SourceCode += "if ( " + con->Expression + " == false ) discard;\n";
                break;
            case MG_VALUE_TYPE_BOOL2:
            case MG_VALUE_TYPE_BOOL3:
            case MG_VALUE_TYPE_BOOL4:
                Context.SourceCode += "if ( " + con->Expression + ".x == false ) discard;\n";
                break;
            default:
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
HK_CLASS_META( MGProjectionNode )

MGProjectionNode::MGProjectionNode() : Super( "Projection" ) {
    Vector = AddInput( "Vector" );
    Result = AddOutput( "Result", MG_VALUE_TYPE_FLOAT4 );
}

void MGProjectionNode::Compute( MaterialBuildContext & Context ) {
    String expression = "TransformMatrix * " + MakeExpression( Context, Vector, MG_VALUE_TYPE_FLOAT4, MakeEmptyVector( MG_VALUE_TYPE_FLOAT4 ), MG_VECTOR_CAST_IDENTITY_W );

    Context.GenerateSourceCode( Result, expression, true );
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGLength)

MGLength::MGLength() :
    Super("Length")
{
    SetSlots({&Value}, {&Result});
}

void MGLength::Compute(MaterialBuildContext& Context)
{
    MGOutput* inputConnection = Value.GetConnection();

    if (inputConnection && Value.ConnectedNode()->Build(Context))
    {
        MG_VALUE_TYPE type = ToFloatType(inputConnection->Type);

        String expression = MakeVectorCast(inputConnection->Expression, inputConnection->Type, type);

        if (type == MG_VALUE_TYPE_FLOAT1)
        {
            Context.GenerateSourceCode(Result, expression, false);
        }
        else
        {
            Context.GenerateSourceCode(Result, "length( " + expression + " )", false);
        }
    }
    else
    {
        Result.Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGDecomposeVector)

MGDecomposeVector::MGDecomposeVector() :
    Super("Decompose Vector")
{
    SetSlots({&Vector}, {&X, &Y, &Z, &W});
}

void MGDecomposeVector::Compute(MaterialBuildContext& Context)
{
    MGOutput* inputConnection = Vector.GetConnection();

    if (inputConnection && Vector.ConnectedNode()->Build(Context))
    {
        MG_COMPONENT_TYPE componentType = GetTypeComponent(inputConnection->Type);

        const char* zero = "0";

        switch (componentType)
        {
            case MG_COMPONENT_FLOAT:
                X.Type = Y.Type = Z.Type = W.Type = MG_VALUE_TYPE_FLOAT1;
                zero                              = "0.0";
                break;
            case MG_COMPONENT_BOOL:
                X.Type = Y.Type = Z.Type = W.Type = MG_VALUE_TYPE_BOOL1;
                zero                              = "false";
                break;
            default:
                HK_ASSERT(0);
                break;
        }

        switch (GetTypeVector(inputConnection->Type))
        {
            case MG_VECTOR_VEC1: {
                Context.GenerateSourceCode(X, inputConnection->Expression, false);
                Y.Expression = zero;
                Z.Expression = zero;
                W.Expression = zero;
                break;
            }
            case MG_VECTOR_VEC2: {
                String temp = "temp_" + Context.GenerateVariableName();
                Context.SourceCode += "const " + String(VariableTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
                X.Expression = temp + ".x";
                Y.Expression = temp + ".y";
                Z.Expression = zero;
                W.Expression = zero;
            }
            break;
            case MG_VECTOR_VEC3: {
                String temp = "temp_" + Context.GenerateVariableName();
                Context.SourceCode += "const " + String(VariableTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
                X.Expression = temp + ".x";
                Y.Expression = temp + ".y";
                Z.Expression = temp + ".z";
                W.Expression = zero;
            }
            break;
            case MG_VECTOR_VEC4: {
                String temp = "temp_" + Context.GenerateVariableName();
                Context.SourceCode += "const " + String(VariableTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
                X.Expression = temp + ".x";
                Y.Expression = temp + ".y";
                Z.Expression = temp + ".z";
                W.Expression = temp + ".w";
            }
            break;
            default:
                HK_ASSERT(0);
                break;
        }
    }
    else
    {
        X.Type = Y.Type = Z.Type = W.Type = MG_VALUE_TYPE_FLOAT1;
        X.Expression                      = "0.0";
        Y.Expression                      = "0.0";
        Z.Expression                      = "0.0";
        W.Expression                      = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGMakeVector)

MGMakeVector::MGMakeVector() :
    Super("Make Vector")
{
    SetSlots({&X, &Y, &Z, &W}, {&Result});
}

void MGMakeVector::Compute(MaterialBuildContext& Context)
{
    MGOutput* XConnection = X.GetConnection();
    MGOutput* YConnection = Y.GetConnection();
    MGOutput* ZConnection = Z.GetConnection();
    MGOutput* WConnection = W.GetConnection();

    bool XValid = XConnection && X.ConnectedNode()->Build(Context) && IsTypeComponent(XConnection->Type);
    bool YValid = YConnection && Y.ConnectedNode()->Build(Context) && IsTypeComponent(YConnection->Type);
    bool ZValid = ZConnection && Z.ConnectedNode()->Build(Context) && IsTypeComponent(ZConnection->Type);
    bool WValid = WConnection && W.ConnectedNode()->Build(Context) && IsTypeComponent(WConnection->Type);

    int numComponents = 4;
    if (!WValid)
    {
        numComponents--;
        if (!ZValid)
        {
            numComponents--;
            if (!YValid)
            {
                numComponents--;
                if (!XValid)
                {
                    numComponents--;
                }
            }
        }
    }

    if (numComponents == 0)
    {
        Result.Type       = MG_VALUE_TYPE_FLOAT1;
        Result.Expression = "0.0";
        return;
    }

    if (numComponents == 1)
    {
        Result.Type = XConnection->Type;
        Context.GenerateSourceCode(Result, XConnection->Expression, false);
        return;
    }

    // Result type is depends on first valid component type
    MG_VALUE_TYPE resultType;
    if (XValid)
    {
        resultType = XConnection->Type;
    }
    else if (YValid)
    {
        resultType = YConnection->Type;
    }
    else if (ZValid)
    {
        resultType = ZConnection->Type;
    }
    else if (WValid)
    {
        resultType = WConnection->Type;
    }
    else
    {
        resultType = MG_VALUE_TYPE_FLOAT1;
        HK_ASSERT(0);
    }
    Result.Type = MG_VALUE_TYPE(resultType + numComponents - 1);

    String resultTypeStr;
    String defaultVal = "0";
    switch (resultType)
    {
        case MG_VALUE_TYPE_FLOAT1:
            resultTypeStr = "float";
            defaultVal    = "0.0";
            break;
        case MG_VALUE_TYPE_BOOL1:
            resultTypeStr = "bool";
            defaultVal    = "false";
            break;
        default:
            HK_ASSERT(0);
    }

    String typeCastX = XValid ?
        ((resultType == XConnection->Type) ? XConnection->Expression : (resultTypeStr + "(" + XConnection->Expression + ")")) :
        defaultVal;

    String typeCastY = YValid ?
        ((resultType == YConnection->Type) ? YConnection->Expression : (resultTypeStr + "(" + YConnection->Expression + ")")) :
        defaultVal;

    String typeCastZ = ZValid ?
        ((resultType == ZConnection->Type) ? ZConnection->Expression : (resultTypeStr + "(" + ZConnection->Expression + ")")) :
        defaultVal;

    String typeCastW = WValid ?
        ((resultType == WConnection->Type) ? WConnection->Expression : (resultTypeStr + "(" + WConnection->Expression + ")")) :
        defaultVal;

    switch (Result.Type)
    {
        case MG_VALUE_TYPE_FLOAT2:
            Context.GenerateSourceCode(Result,
                                        "vec2( " + typeCastX + ", " + typeCastY + " )",
                                        false);
            break;
        case MG_VALUE_TYPE_FLOAT3:
            Context.GenerateSourceCode(Result,
                                        "vec3( " + typeCastX + ", " + typeCastY + ", " + typeCastZ + " )",
                                        false);
            break;
        case MG_VALUE_TYPE_FLOAT4:
            Context.GenerateSourceCode(Result,
                                        "vec4( " + typeCastX + ", " + typeCastY + ", " + typeCastZ + ", " + typeCastW + " )",
                                        false);
            break;
        case MG_VALUE_TYPE_BOOL2:
            Context.GenerateSourceCode(Result,
                                        "bvec2( " + typeCastX + ", " + typeCastY + " )",
                                        false);
            break;
        case MG_VALUE_TYPE_BOOL3:
            Context.GenerateSourceCode(Result,
                                        "bvec3( " + typeCastX + ", " + typeCastY + ", " + typeCastZ + " )",
                                        false);
            break;
        case MG_VALUE_TYPE_BOOL4:
            Context.GenerateSourceCode(Result,
                                        "bvec4( " + typeCastX + ", " + typeCastY + ", " + typeCastZ + ", " + typeCastW + " )",
                                        false);
            break;
        default:
            HK_ASSERT(0);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGArithmeticFunction1)
HK_CLASS_META(MGSaturate)
HK_CLASS_META(MGSinus)
HK_CLASS_META(MGCosinus)
HK_CLASS_META(MGFract)
HK_CLASS_META(MGNegate)
HK_CLASS_META(MGNormalize)

MGArithmeticFunction1::MGArithmeticFunction1()
{
    HK_ASSERT(0);
}

MGArithmeticFunction1::MGArithmeticFunction1(FUNCTION _Function, StringView Name) :
    Super(Name), m_Function(_Function)
{
    SetSlots({&Value}, {&Result});
}

void MGArithmeticFunction1::Compute(MaterialBuildContext& Context)
{
    MGOutput* connectionA = Value.GetConnection();

    if (connectionA && Value.ConnectedNode()->Build(Context))
    {

        // Result type depends on input type
        if (!IsArithmeticType(connectionA->Type))
        {
            Result.Type = ToFloatType(connectionA->Type);
        }
        else
        {
            Result.Type = connectionA->Type;
        }

        String expressionA = MakeVectorCast(connectionA->Expression, connectionA->Type, Result.Type);

        String expression;

        switch (m_Function)
        {
            case Saturate:
                expression = "saturate( " + expressionA + " )";
                break;
            case Sin:
                expression = "sin( " + expressionA + " )";
                break;
            case Cos:
                expression = "cos( " + expressionA + " )";
                break;
            case Fract:
                expression = "fract( " + expressionA + " )";
                break;
            case Negate:
                expression = "(-" + expressionA + ")";
                break;
            case Normalize:
                if (Result.Type == MG_VALUE_TYPE_FLOAT1)
                {
                    expression = "1.0";
                    //} else if ( Result.Type == AT_Integer1 ) { // If there will be integer types
                    //    expression = "1";
                }
                else
                {
                    expression = "normalize( " + expressionA + " )";
                }
                break;
            default:
                HK_ASSERT(0);
        }

        Context.GenerateSourceCode(Result, expression, false);
    }
    else
    {
        Result.Type = MG_VALUE_TYPE_FLOAT4;

        Context.GenerateSourceCode(Result, MakeEmptyVector(Result.Type), false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGArithmeticFunction2)
HK_CLASS_META(MGMul)
HK_CLASS_META(MGDiv)
HK_CLASS_META(MGAdd)
HK_CLASS_META(MGSub)
HK_CLASS_META(MGStep)
HK_CLASS_META(MGPow)
HK_CLASS_META(MGMod)
HK_CLASS_META(MGMin)
HK_CLASS_META(MGMax)

MGArithmeticFunction2::MGArithmeticFunction2()
{
    HK_ASSERT(0);
}

MGArithmeticFunction2::MGArithmeticFunction2(FUNCTION Function, StringView Name) :
    Super(Name), m_Function(Function)
{
    SetSlots({&ValueA, &ValueB}, {&Result});
}

void MGArithmeticFunction2::Compute(MaterialBuildContext& Context)
{
    MGOutput* connectionA = ValueA.GetConnection();
    MGOutput* connectionB = ValueB.GetConnection();

    if (connectionA && ValueA.ConnectedNode()->Build(Context) && connectionB && ValueB.ConnectedNode()->Build(Context))
    {

        // Result type depends on input type
        if (!IsArithmeticType(connectionA->Type))
        {
            Result.Type = ToFloatType(connectionA->Type);
        }
        else
        {
            Result.Type = connectionA->Type;
        }

        String expressionA = MakeVectorCast(connectionA->Expression, connectionA->Type, Result.Type);
        String expressionB = MakeVectorCast(connectionB->Expression, connectionB->Type, Result.Type, MG_VECTOR_CAST_EXPAND_VEC1);

        String expression;

        switch (m_Function)
        {
            case Add:
                expression = "(" + expressionA + " + " + expressionB + ")";
                break;
            case Sub:
                expression = "(" + expressionA + " - " + expressionB + ")";
                break;
            case Mul:
                expression = "(" + expressionA + " * " + expressionB + ")";
                break;
            case Div:
                expression = "(" + expressionA + " / " + expressionB + ")";
                break;
            case Step:
                expression = "step( " + expressionA + ", " + expressionB + " )";
                break;
            case Pow:
                expression = "pow( " + expressionA + ", " + expressionB + " )";
                break;
            case Mod:
                expression = "mod( " + expressionA + ", " + expressionB + " )";
                break;
            case Min:
                expression = "min( " + expressionA + ", " + expressionB + " )";
                break;
            case Max:
                expression = "max( " + expressionA + ", " + expressionB + " )";
                break;
            default:
                HK_ASSERT(0);
        }

        Context.GenerateSourceCode(Result, expression, false);
    }
    else
    {
        Result.Type = MG_VALUE_TYPE_FLOAT4;

        Context.GenerateSourceCode(Result, MakeEmptyVector(Result.Type), false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGArithmeticFunction3)
HK_CLASS_META(MGMAD)
HK_CLASS_META(MGLerp)
HK_CLASS_META(MGClamp)

MGArithmeticFunction3::MGArithmeticFunction3()
{
    HK_ASSERT(0);
}

MGArithmeticFunction3::MGArithmeticFunction3(FUNCTION Function, StringView Name) :
    Super(Name), m_Function(Function)
{
    SetSlots({&ValueA, &ValueB, &ValueC}, {&Result});
}

void MGArithmeticFunction3::Compute(MaterialBuildContext& Context)
{
    MGOutput* connectionA = ValueA.GetConnection();
    MGOutput* connectionB = ValueB.GetConnection();
    MGOutput* connectionC = ValueC.GetConnection();

    if (connectionA && ValueA.ConnectedNode()->Build(Context) && connectionB && ValueB.ConnectedNode()->Build(Context) && connectionC && ValueC.ConnectedNode()->Build(Context))
    {

        // Result type depends on input type
        if (!IsArithmeticType(connectionA->Type))
        {
            Result.Type = ToFloatType(connectionA->Type);
        }
        else
        {
            Result.Type = connectionA->Type;
        }

        String expressionA = MakeVectorCast(connectionA->Expression, connectionA->Type, Result.Type);
        String expressionB = MakeVectorCast(connectionB->Expression, connectionB->Type, Result.Type, MG_VECTOR_CAST_EXPAND_VEC1);
        String expressionC = MakeVectorCast(connectionC->Expression, connectionC->Type, Result.Type, MG_VECTOR_CAST_EXPAND_VEC1);

        String expression;

        switch (m_Function)
        {
            case Mad:
                expression = "(" + expressionA + " * " + expressionB + " + " + expressionC + ")";
                break;
            case Lerp:
                expression = "mix( " + expressionA + ", " + expressionB + ", " + expressionC + " )";
                break;
            case Clamp:
                expression = "clamp( " + expressionA + ", " + expressionB + ", " + expressionC + " )";
                break;
            default:
                HK_ASSERT(0);
        }

        Context.GenerateSourceCode(Result, expression, false);
    }
    else
    {
        Result.Type = MG_VALUE_TYPE_FLOAT4;
        Context.GenerateSourceCode(Result, MakeEmptyVector(Result.Type), false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGSpheremapCoord)

MGSpheremapCoord::MGSpheremapCoord() :
    Super("Spheremap Coord")
{
    SetSlots({&Dir}, {&TexCoord});
}

void MGSpheremapCoord::Compute(MaterialBuildContext& Context)
{
    MGOutput* connectionDir = Dir.GetConnection();

    if (connectionDir && Dir.ConnectedNode()->Build(Context))
    {
        String expressionDir = MakeVectorCast(connectionDir->Expression, connectionDir->Type, MG_VALUE_TYPE_FLOAT3);

        Context.GenerateSourceCode(TexCoord, "builtin_spheremap_coord( " + expressionDir + " )", true);
    }
    else
    {
        Context.GenerateSourceCode(TexCoord, MakeEmptyVector(MG_VALUE_TYPE_FLOAT2), false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGLuminance)

MGLuminance::MGLuminance() :
    Super("Luminance")
{
    SetSlots({&LinearColor}, {&Luminance});
}

void MGLuminance::Compute(MaterialBuildContext& Context)
{
    MGOutput* connectionColor = LinearColor.GetConnection();

    if (connectionColor && LinearColor.ConnectedNode()->Build(Context))
    {
        String expressionColor = MakeVectorCast(connectionColor->Expression, connectionColor->Type, MG_VALUE_TYPE_FLOAT4, MG_VECTOR_CAST_EXPAND_VEC1);

        Context.GenerateSourceCode(Luminance, "builtin_luminance( " + expressionColor + " )", false);
    }
    else
    {
        Context.GenerateSourceCode(Luminance, MakeEmptyVector(MG_VALUE_TYPE_FLOAT1), false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGPI)

MGPI::MGPI() :
    Super("PI")
{
    SetSlots({}, {&Value});
}

void MGPI::Compute(MaterialBuildContext& Context)
{
    Value.Expression = "3.1415926";
}

HK_CLASS_META(MG2PI)

MG2PI::MG2PI() :
    Super("2PI")
{
    SetSlots({}, {&Value});
}

void MG2PI::Compute(MaterialBuildContext& Context)
{
    Value.Expression = "6.2831853";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGBoolean)
HK_PROPERTY_DIRECT(bValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGBoolean::MGBoolean(bool v) :
    Super("Boolean"),
    bValue(v)
{
    SetSlots({}, {&Value});
}

void MGBoolean::Compute(MaterialBuildContext& Context)
{
    Value.Expression = Core::ToString(bValue);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGBoolean2)
HK_PROPERTY_DIRECT(bValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGBoolean2::MGBoolean2(Bool2 const& v) :
    Super("Boolean2"),
    bValue(v)
{
    SetSlots({}, {&Value});
}

void MGBoolean2::Compute(MaterialBuildContext& Context)
{
    Context.GenerateSourceCode(Value, "bvec2( " + Core::ToString(bValue.X) + ", " + Core::ToString(bValue.Y) + " )", false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGBoolean3)
HK_PROPERTY_DIRECT(bValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGBoolean3::MGBoolean3(Bool3 const& v) :
    Super("Boolean3"),
    bValue(v)
{
    SetSlots({}, {&Value});
}

void MGBoolean3::Compute(MaterialBuildContext& Context)
{
    Context.GenerateSourceCode(Value, "bvec3( " + Core::ToString(bValue.X) + ", " + Core::ToString(bValue.Y) + ", " + Core::ToString(bValue.Z) + " )", false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGBoolean4)
HK_PROPERTY_DIRECT(bValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGBoolean4::MGBoolean4(Bool4 const& v) :
    Super("Boolean4"),
    bValue(v)
{
    SetSlots({}, {&Value});
}

void MGBoolean4::Compute(MaterialBuildContext& Context)
{
    Context.GenerateSourceCode(Value, "bvec4( " + Core::ToString(bValue.X) + ", " + Core::ToString(bValue.Y) + ", " + Core::ToString(bValue.Z) + ", " + Core::ToString(bValue.W) + " )", false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGFloat)
HK_PROPERTY_DIRECT(fValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGFloat::MGFloat(float fValue) :
    Super("Float"),
    fValue(fValue)
{
    SetSlots({}, {&Value});
}

void MGFloat::Compute(MaterialBuildContext& Context)
{
    Value.Expression = Core::ToString(fValue);
    if (Value.Expression.Contains('.') == -1)
    {
        Value.Expression += ".0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGFloat2)
HK_PROPERTY_DIRECT(fValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGFloat2::MGFloat2(Float2 const& fValue) :
    Super("Float2"),
    fValue(fValue)
{
    SetSlots({}, {&Value});
}

void MGFloat2::Compute(MaterialBuildContext& Context)
{
    Context.GenerateSourceCode(Value, "vec2( " + Core::ToString(fValue.X) + ", " + Core::ToString(fValue.Y) + " )", false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGFloat3)
HK_PROPERTY_DIRECT(fValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGFloat3::MGFloat3(Float3 const& fValue) :
    Super("Float3"),
    fValue(fValue)
{
    SetSlots({}, {&Value});
}

void MGFloat3::Compute(MaterialBuildContext& Context)
{
    Context.GenerateSourceCode(Value, "vec3( " + Core::ToString(fValue.X) + ", " + Core::ToString(fValue.Y) + ", " + Core::ToString(fValue.Z) + " )", false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGFloat4)
HK_PROPERTY_DIRECT(fValue, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGFloat4::MGFloat4(Float4 const& fValue) :
    Super("Float4"),
    fValue(fValue)
{
    SetSlots({}, {&Value});
}

void MGFloat4::Compute(MaterialBuildContext& Context)
{
    Context.GenerateSourceCode(Value, "vec4( " + Core::ToString(fValue.X) + ", " + Core::ToString(fValue.Y) + ", " + Core::ToString(fValue.Z) + ", " + Core::ToString(fValue.W) + " )", false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGTextureSlot)
HK_PROPERTY_DIRECT(TextureType, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(Filter, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(AddressU, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(AddressV, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(AddressW, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(MipLODBias, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(Anisotropy, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(MinLod, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(MaxLod, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGTextureSlot::MGTextureSlot() :
    Super("Texture Slot")
{
    SetSlots({}, {&Value});
}

void MGTextureSlot::Compute(MaterialBuildContext& Context)
{
    if (GetSlotIndex() >= 0)
    {
        Value.Expression = "tslot_" + Core::ToString(GetSlotIndex());

        Context.MaxTextureSlot = Math::Max(Context.MaxTextureSlot, GetSlotIndex());
    }
    else
    {
        Value.Expression.Clear();
    }
}

static constexpr const char* TextureTypeToShaderSampler[][2] = {
    {"sampler1D", "float"},
    {"sampler1DArray", "vec2"},
    {"sampler2D", "vec2"},
    {"sampler2DArray", "vec3"},
    {"sampler3D", "vec3"},
    {"samplerCube", "vec3"},
    {"samplerCubeArray", "vec4"}};

static const char* GetShaderType(TEXTURE_TYPE _Type)
{
    return TextureTypeToShaderSampler[_Type][0];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGUniformAddress)
HK_PROPERTY_DIRECT(UniformType, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(Address, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGUniformAddress::MGUniformAddress() :
    Super("Uniform Address")
{
    SetSlots({}, {&Value});
}

void MGUniformAddress::Compute(MaterialBuildContext& Context)
{
    if (Address >= 0)
    {
        int addr     = Math::Clamp(Address, 0, 15);
        int location = addr / 4;

        Value.Expression = "uaddr_" + Core::ToString(location);
        switch (UniformType)
        {
            case MG_UNIFORM_TYPE_FLOAT1:
                Value.Type = MG_VALUE_TYPE_FLOAT1;
                switch (addr & 3)
                {
                    case 0: Value.Expression += ".x"; break;
                    case 1: Value.Expression += ".y"; break;
                    case 2: Value.Expression += ".z"; break;
                    case 3: Value.Expression += ".w"; break;
                }
                break;

            case MG_UNIFORM_TYPE_FLOAT2:
                Value.Type = MG_VALUE_TYPE_FLOAT2;
                switch (addr & 3)
                {
                    case 0: Value.Expression += ".xy"; break;
                    case 1: Value.Expression += ".yz"; break;
                    case 2: Value.Expression += ".zw"; break;
                    case 3: Value.Expression += ".ww"; break; // FIXME: error?
                }
                break;

            case MG_UNIFORM_TYPE_FLOAT3:
                Value.Type = MG_VALUE_TYPE_FLOAT3;
                switch (addr & 3)
                {
                    case 0: Value.Expression += ".xyz"; break;
                    case 1: Value.Expression += ".yzw"; break;
                    case 2: Value.Expression += ".www"; break; // FIXME: error?
                    case 3: Value.Expression += ".www"; break; // FIXME: error?
                }
                break;

            case MG_UNIFORM_TYPE_FLOAT4:
                Value.Type = MG_VALUE_TYPE_FLOAT4;
                switch (addr & 3)
                {
                    case 1: Value.Expression += ".yzww"; break; // FIXME: error?
                    case 2: Value.Expression += ".wwww"; break; // FIXME: error?
                    case 3: Value.Expression += ".wwww"; break; // FIXME: error?
                }
                break;

            default:
                LOG("Unknown uniform type\n");

                Value.Type = MG_VALUE_TYPE_FLOAT4;
                switch (addr & 3)
                {
                    case 1: Value.Expression += ".yzww"; break; // FIXME: error?
                    case 2: Value.Expression += ".wwww"; break; // FIXME: error?
                    case 3: Value.Expression += ".wwww"; break; // FIXME: error?
                }
                break;
        }

        Context.MaxUniformAddress = Math::Max(Context.MaxUniformAddress, location);
    }
    else
    {
        Value.Expression.Clear();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGTextureLoad)
HK_PROPERTY_DIRECT(bSwappedToBGR, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(ColorSpace, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGTextureLoad::MGTextureLoad() :
    Super("Texture Sampler")
{
    SetSlots({&Texture, &TexCoord}, {&RGBA, &RGB, &R, &G, &B, &A});
}

static const char* ChooseSampleFunction(TEXTURE_COLOR_SPACE _ColorSpace)
{
    switch (_ColorSpace)
    {
        case TEXTURE_COLOR_SPACE_RGBA:
            return "texture";
        case TEXTURE_COLOR_SPACE_SRGB_ALPHA:
            return "texture_srgb_alpha";
        case TEXTURE_COLOR_SPACE_YCOCG:
            return "texture_ycocg";
        case TEXTURE_COLOR_SPACE_GRAYSCALED:
            return "texture_grayscaled";
        default:
            return "texture";
    }
}

void MGTextureLoad::Compute(MaterialBuildContext& Context)
{
    bool bValid = false;

    MGOutput* texSlotCon = Texture.GetConnection();
    if (texSlotCon)
    {

        MGNode* node = Texture.ConnectedNode();
        if (node->FinalClassId() == MGTextureSlot::ClassId() && node->Build(Context))
        {

            MGTextureSlot* texSlot = static_cast<MGTextureSlot*>(node);

            MG_VALUE_TYPE sampleType = MG_VALUE_TYPE_FLOAT2;

            // TODO: table?
            switch (texSlot->TextureType)
            {
                case TEXTURE_1D:
                    sampleType = MG_VALUE_TYPE_FLOAT1;
                    break;
                case TEXTURE_1D_ARRAY:
                    sampleType = MG_VALUE_TYPE_FLOAT2;
                    break;
                case TEXTURE_2D:
                    sampleType = MG_VALUE_TYPE_FLOAT2;
                    break;
                case TEXTURE_2D_ARRAY:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                case TEXTURE_3D:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                case TEXTURE_CUBE:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                case TEXTURE_CUBE_ARRAY:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                default:
                    HK_ASSERT(0);
            };

            int32_t slotIndex = texSlot->GetSlotIndex();
            if (slotIndex != -1)
            {

                MGOutput* texCoordCon = TexCoord.GetConnection();

                if (texCoordCon && TexCoord.ConnectedNode()->Build(Context))
                {

                    const char* swizzleStr = bSwappedToBGR ? ".bgra" : "";

                    const char* sampleFunc = ChooseSampleFunction(ColorSpace);

                    RGBA.Expression = Context.GenerateVariableName();
                    Context.SourceCode += "const vec4 " + RGBA.Expression + " = " + sampleFunc + "( tslot_" + Core::ToString(slotIndex) + ", " + MakeVectorCast(texCoordCon->Expression, texCoordCon->Type, sampleType) + " )" + swizzleStr + ";\n";
                    bValid = true;
                }
            }
        }
    }

    if (bValid)
    {
        R.Expression   = RGBA.Expression + ".r";
        G.Expression   = RGBA.Expression + ".g";
        B.Expression   = RGBA.Expression + ".b";
        A.Expression   = RGBA.Expression + ".a";
        RGB.Expression = RGBA.Expression + ".rgb";
    }
    else
    {
        Context.GenerateSourceCode(RGBA, MakeEmptyVector(MG_VALUE_TYPE_FLOAT4), false);

        R.Expression   = "0.0";
        G.Expression   = "0.0";
        B.Expression   = "0.0";
        A.Expression   = "0.0";
        RGB.Expression = "vec3(0.0)";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGNormalLoad)
HK_PROPERTY_DIRECT(Pack, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGNormalLoad::MGNormalLoad() :
    Super("Normal Sampler")
{
    SetSlots({&Texture, &TexCoord}, {&XYZ, &X, &Y, &Z});
}

static const char* ChooseSampleFunction(NORMAL_MAP_PACK Pack)
{
    switch (Pack)
    {
        case NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE:
            return "texture_nm_xyz";
        case NORMAL_MAP_PACK_RG_BC5_COMPATIBLE:
            return "texture_nm_xy";
        case NORMAL_MAP_PACK_SPHEREMAP_BC5_COMPATIBLE:
            return "texture_nm_spheremap";
        case NORMAL_MAP_PACK_STEREOGRAPHIC_BC5_COMPATIBLE:
            return "texture_nm_stereographic";
        case NORMAL_MAP_PACK_PARABOLOID_BC5_COMPATIBLE:
            return "texture_nm_paraboloid";
        //case NM_QUARTIC:
        //    return "texture_nm_quartic";
        //case NM_FLOAT:
        //    return "texture_nm_float";
        case NORMAL_MAP_PACK_RGBA_BC3_COMPATIBLE:
            return "texture_nm_dxt5";
        default:
            return "texture_nm_xyz";
    }
}

void MGNormalLoad::Compute(MaterialBuildContext& Context)
{
    bool bValid = false;

    MGOutput* texSlotCon = Texture.GetConnection();
    if (texSlotCon)
    {

        MGNode* node = Texture.ConnectedNode();
        if (node->FinalClassId() == MGTextureSlot::ClassId() && node->Build(Context))
        {

            MGTextureSlot* texSlot = static_cast<MGTextureSlot*>(node);

            MG_VALUE_TYPE sampleType = MG_VALUE_TYPE_FLOAT2;

            // TODO: table?
            switch (texSlot->TextureType)
            {
                case TEXTURE_1D:
                    sampleType = MG_VALUE_TYPE_FLOAT1;
                    break;
                case TEXTURE_1D_ARRAY:
                    sampleType = MG_VALUE_TYPE_FLOAT2;
                    break;
                case TEXTURE_2D:
                    sampleType = MG_VALUE_TYPE_FLOAT2;
                    break;
                case TEXTURE_2D_ARRAY:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                case TEXTURE_3D:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                case TEXTURE_CUBE:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                case TEXTURE_CUBE_ARRAY:
                    sampleType = MG_VALUE_TYPE_FLOAT3;
                    break;
                default:
                    HK_ASSERT(0);
            };

            int32_t slotIndex = texSlot->GetSlotIndex();
            if (slotIndex != -1)
            {

                MGOutput* texCoordCon = TexCoord.GetConnection();

                if (texCoordCon && TexCoord.ConnectedNode()->Build(Context))
                {

                    const char* sampleFunc = ChooseSampleFunction(Pack);

                    XYZ.Expression = Context.GenerateVariableName();
                    Context.SourceCode += "const vec3 " + XYZ.Expression + " = " + sampleFunc + "( tslot_" + Core::ToString(slotIndex) + ", " + MakeVectorCast(texCoordCon->Expression, texCoordCon->Type, sampleType) + " );\n";
                    bValid = true;
                }
            }
        }
    }

    if (bValid)
    {
        X.Expression = XYZ.Expression + ".x";
        Y.Expression = XYZ.Expression + ".y";
        Z.Expression = XYZ.Expression + ".z";
    }
    else
    {
        Context.GenerateSourceCode(XYZ, MakeDefaultNormal(), false);

        X.Expression = "0.0";
        Y.Expression = "0.0";
        Z.Expression = "0.0";
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGParallaxMapLoad)
HK_END_CLASS_META()

MGParallaxMapLoad::MGParallaxMapLoad() :
    Super("Parallax Map Sampler")
{
    SetSlots({&Texture, &TexCoord, &DisplacementScale}, {&Result});
}

void MGParallaxMapLoad::Compute(MaterialBuildContext& Context)
{
    bool bValid = false;

    MGOutput* texSlotCon = Texture.GetConnection();
    if (texSlotCon)
    {
        MGNode* node = Texture.ConnectedNode();
        if (node->FinalClassId() == MGTextureSlot::ClassId() && node->Build(Context))
        {

            MGTextureSlot* texSlot = static_cast<MGTextureSlot*>(node);

            if (texSlot->TextureType == TEXTURE_2D)
            {
                int32_t slotIndex = texSlot->GetSlotIndex();
                if (slotIndex != -1)
                {

                    MGOutput* texCoordCon = TexCoord.GetConnection();

                    if (texCoordCon && TexCoord.ConnectedNode()->Build(Context))
                    {

                        String sampler  = "tslot_" + Core::ToString(slotIndex);
                        String texCoord = MakeVectorCast(texCoordCon->Expression, texCoordCon->Type, MG_VALUE_TYPE_FLOAT2);

                        String   displacementScale;
                        MGOutput* displacementScaleCon = DisplacementScale.GetConnection();
                        if (displacementScaleCon && DisplacementScale.ConnectedNode()->Build(Context))
                        {
                            displacementScale = MakeVectorCast(displacementScaleCon->Expression, displacementScaleCon->Type, MG_VALUE_TYPE_FLOAT1);
                        }
                        else
                        {
                            displacementScale = "0.05";
                        }

                        //String selfShadowing;
                        //MGOutput * selfShadowingCon = SelfShadowing.GetConnection();
                        //if ( selfShadowingCon && SelfShadowing.ConnectedNode()->Build( Context ) ) {
                        //    selfShadowing = MakeVectorCast( selfShadowingCon->Expression, selfShadowingCon->Type, MG_VALUE_TYPE_BOOL1 );
                        //} else {
                        //    selfShadowing = MakeEmptyVector( MG_VALUE_TYPE_BOOL1 );
                        //}

                        Result.Expression = Context.GenerateVariableName();
                        Context.SourceCode += "const vec2 " + Result.Expression +
                            " = ParallaxMapping( " + texCoord + ", " + displacementScale + " );\n";

                        bValid = true;

                        Context.ParallaxSampler = slotIndex;
                    }
                }
            }
        }
    }

    if (!bValid)
    {
        Context.GenerateSourceCode(Result, MakeEmptyVector(MG_VALUE_TYPE_FLOAT2), false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGVirtualTextureLoad)
HK_PROPERTY_DIRECT(TextureLayer, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(ColorSpace, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bSwappedToBGR, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGVirtualTextureLoad::MGVirtualTextureLoad() :
    Super("Virtual Texture Sampler")
{
    SetSlots({}, {&R, &G, &B, &A, &RGB, &RGBA});
}

void MGVirtualTextureLoad::Compute(MaterialBuildContext& Context)
{
    const char* swizzleStr = bSwappedToBGR ? ".bgra" : "";
    const char* sampleFunc = ChooseSampleFunction(ColorSpace);

    RGBA.Expression = Context.GenerateVariableName();

    Context.SourceCode +=
        "const vec4 " + RGBA.Expression + " = " + sampleFunc + "( vt_PhysCache" + Core::ToString(TextureLayer) + ", InPhysicalUV )" + swizzleStr + ";\n";

    R.Expression   = RGBA.Expression + ".r";
    G.Expression   = RGBA.Expression + ".g";
    B.Expression   = RGBA.Expression + ".b";
    A.Expression   = RGBA.Expression + ".a";
    RGB.Expression = RGBA.Expression + ".rgb";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGVirtualTextureNormalLoad)
HK_PROPERTY_DIRECT(TextureLayer, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(Pack, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGVirtualTextureNormalLoad::MGVirtualTextureNormalLoad() :
    Super("Virtual Texture Normal Sampler")
{
    SetSlots({}, {&X, &Y, &Z, &XYZ});
}

void MGVirtualTextureNormalLoad::Compute(MaterialBuildContext& Context)
{
    const char* sampleFunc = ChooseSampleFunction(Pack);

    XYZ.Expression = Context.GenerateVariableName();

    Context.SourceCode +=
        "const vec3 " + XYZ.Expression + " = " + sampleFunc + "( vt_PhysCache" + Core::ToString(TextureLayer) + ", InPhysicalUV );\n";

    X.Expression = XYZ.Expression + ".x";
    Y.Expression = XYZ.Expression + ".y";
    Z.Expression = XYZ.Expression + ".z";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInFragmentCoord)

MGInFragmentCoord::MGInFragmentCoord() :
    Super("InFragmentCoord")
{
    SetSlots({}, {&Value, &X, &Y, &Z, &W, &XY});

    Value.Expression = "gl_FragCoord";
    X.Expression     = "gl_FragCoord.x";
    Y.Expression     = "gl_FragCoord.y";
    Z.Expression     = "gl_FragCoord.z";
    W.Expression     = "gl_FragCoord.w";
    XY.Expression    = "gl_FragCoord.xy";
}

void MGInFragmentCoord::Compute(MaterialBuildContext& Context)
{
    // TODO: Case for vertex stage
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInPosition)

MGInPosition::MGInPosition() :
    Super("InPosition")
{
    SetSlots({}, {&Value});
}

void MGInPosition::Compute(MaterialBuildContext& Context)
{
    if (Context.GetMaterialType() == MATERIAL_TYPE_HUD)
    {
        Value.Type = MG_VALUE_TYPE_FLOAT2;
    }
    else
    {
        Value.Type = MG_VALUE_TYPE_FLOAT3;
    }

    if (Context.GetStage() != MATERIAL_STAGE_VERTEX)
    {
        Context.InputVaryings.Add(StageVarying("V_Position", "VertexPosition", Value.Type));

        Value.Expression = "V_Position";
    }
    else
    {
        Context.GenerateSourceCode(Value, "VertexPosition", false);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInNormal)

MGInNormal::MGInNormal() :
    Super("InNormal")
{
    SetSlots({}, {&Value});
}

void MGInNormal::Compute(MaterialBuildContext& Context)
{
    //if ( Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
    //    Value.Type = MG_VALUE_TYPE_FLOAT2;
    //} else {
    //    Value.Type = MG_VALUE_TYPE_FLOAT3;
    //}

    if (Context.GetStage() != MATERIAL_STAGE_VERTEX)
    {
        Context.InputVaryings.Add(StageVarying("V_Normal", "VertexNormal", Value.Type));

        Value.Expression = "V_Normal";
    }
    else
    {
        Value.Expression = "VertexNormal";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInColor)

MGInColor::MGInColor() :
    Super("InColor")
{
    SetSlots({}, {&Value});
}

void MGInColor::Compute(MaterialBuildContext& Context)
{
    if (Context.GetMaterialType() == MATERIAL_TYPE_HUD)
    {
        if (Context.GetStage() != MATERIAL_STAGE_VERTEX)
        {
            Context.InputVaryings.Add(StageVarying("V_Color", "InColor", Value.Type));

            Value.Expression = "V_Color";
        }
        else
        {

            Value.Expression = "InColor";
        }
    }
    else
    {
        Value.Expression = "vec4(1.0)";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInTexCoord)

MGInTexCoord::MGInTexCoord() :
    Super("InTexCoord")
{
    SetSlots({}, {&Value});
}

void MGInTexCoord::Compute(MaterialBuildContext& Context)
{
    if (Context.GetStage() != MATERIAL_STAGE_VERTEX)
    {
        Context.InputVaryings.Add(StageVarying("V_TexCoord", "InTexCoord", Value.Type));

        Value.Expression = "V_TexCoord";
    }
    else
    {
        Value.Expression = "InTexCoord";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInTimer)

MGInTimer::MGInTimer() :
    Super("InTimer")
{
    SetSlots({}, {&GameRunningTimeSeconds, &GameplayTimeSeconds});

    GameRunningTimeSeconds.Expression = "GameRunningTimeSeconds";

    GameplayTimeSeconds.Expression = "GameplayTimeSeconds";
}

void MGInTimer::Compute(MaterialBuildContext& Context)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGInViewPosition)

MGInViewPosition::MGInViewPosition() :
    Super("InViewPosition")
{
    SetSlots({}, {&Value});

    Value.Expression = "ViewPosition.xyz";
}

void MGInViewPosition::Compute(MaterialBuildContext& Context)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGCondLess)
// TODO: add greater, lequal, gequal, equal, not equal

MGCondLess::MGCondLess() :
    Super("Cond A < B")
{
    SetSlots({&ValueA, &ValueB, &True, &False}, {&Result});
}

void MGCondLess::Compute(MaterialBuildContext& Context)
{
    MGOutput* connectionA     = ValueA.GetConnection();
    MGOutput* connectionB     = ValueB.GetConnection();
    MGOutput* connectionTrue  = True.GetConnection();
    MGOutput* connectionFalse = False.GetConnection();

    String expression;

    if (connectionA && connectionB && connectionTrue && connectionFalse && ValueA.ConnectedNode()->Build(Context) && ValueB.ConnectedNode()->Build(Context) && True.ConnectedNode()->Build(Context) && False.ConnectedNode()->Build(Context))
    {
        if (connectionA->Type != connectionB->Type || connectionTrue->Type != connectionFalse->Type || !IsArithmeticType(connectionA->Type))
        {
            Result.Type = MG_VALUE_TYPE_FLOAT4;
            expression  = MakeEmptyVector(MG_VALUE_TYPE_FLOAT4);
        }
        else
        {
            Result.Type = connectionTrue->Type;

            String cond;

            if (connectionA->Type == MG_VALUE_TYPE_FLOAT1)
            {
                cond = "step( " + connectionB->Expression + ", " + connectionA->Expression + " )";

                expression = "mix( " + connectionTrue->Expression + ", " + connectionFalse->Expression + ", " + cond + " )";
            }
            else
            {
                if (Result.Type == MG_VALUE_TYPE_FLOAT1)
                {
                    cond = "float( all( lessThan( " + connectionA->Expression + ", " + connectionB->Expression + " ) ) )";
                }
                else
                {
                    cond = String(VariableTypeStr[Result.Type]) + "( float( all( lessThan( " + connectionA->Expression + ", " + connectionB->Expression + " ) ) ) )";
                }

                expression = "mix( " + connectionFalse->Expression + ", " + connectionTrue->Expression + ", " + cond + " )";
            }
        }
    }
    else
    {
        Result.Type = MG_VALUE_TYPE_FLOAT4;
        expression  = MakeEmptyVector(MG_VALUE_TYPE_FLOAT4);
    }

    Context.GenerateSourceCode(Result, expression, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_CLASS_META(MGAtmosphere)

MGAtmosphere::MGAtmosphere() :
    Super("Atmosphere Scattering")
{
    SetSlots({&Dir}, {&Result});
}

void MGAtmosphere::Compute(MaterialBuildContext& Context)
{
    MGOutput* dirConnection = Dir.GetConnection();

    if (dirConnection && Dir.ConnectedNode()->Build(Context))
    {
        String expression = MakeVectorCast(dirConnection->Expression, dirConnection->Type, MG_VALUE_TYPE_FLOAT3);
        Context.GenerateSourceCode(Result, "vec4( atmosphere( normalize(" + expression + "), normalize(vec3(0.5,0.5,-1)) ), 1.0 )", false);
    }
    else
    {
        Result.Expression = MakeEmptyVector(MG_VALUE_TYPE_FLOAT4);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////


String MGMaterialGraph::SamplersString(int MaxtextureSlot)
{
    String s;
    String bindingStr;

    for (MGTextureSlot* slot : GetTextures())
    {
        if (slot->GetSlotIndex() <= MaxtextureSlot)
        {
            bindingStr = Core::ToString(slot->GetSlotIndex());
            s += "layout( binding = " + bindingStr + " ) uniform " + GetShaderType(slot->TextureType) + " tslot_" + bindingStr + ";\n";
        }
    }
    return s;
}

static const char* texture_srgb_alpha =
    "vec4 texture_srgb_alpha( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec4 color = texture( sampler, texCoord );\n"
    "#ifdef SRGB_GAMMA_APPROX\n"
    "  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );\n"
    "#else\n"
    "  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );\n"
    "  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );\n"
    "  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );\n"
    "  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );\n"
    "  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );\n"
    //LinearVal.x = ( sRGBValue.x <= 0.04045 ) ? sRGBValue.x / 12.92 : pow( ( sRGBValue.x + 0.055 ) / 1.055, 2.4 );
    //LinearVal.y = ( sRGBValue.y <= 0.04045 ) ? sRGBValue.y / 12.92 : pow( ( sRGBValue.y + 0.055 ) / 1.055, 2.4 );
    //LinearVal.z = ( sRGBValue.z <= 0.04045 ) ? sRGBValue.z / 12.92 : pow( ( sRGBValue.z + 0.055 ) / 1.055, 2.4 );
    //LinearVal.w = sRGBValue.w;
    //return LinearVal;
    "#endif\n"
    "}\n";

static const char* texture_ycocg =
    "vec4 texture_ycocg( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec4 ycocg = texture( sampler, texCoord );\n"
    "  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;\n"
    "  ycocg.z = 1.0 / ycocg.z;\n"
    "  ycocg.xy *= ycocg.z;\n"
    "  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),\n"
    "                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),\n"
    "                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),\n"
    "                     1.0 );\n"
    "#ifdef SRGB_GAMMA_APPROX\n"
    "  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );\n"
    "#else\n"
    "  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );\n"
    "  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );\n"
    "  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );\n"
    "  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );\n"
    "  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );\n"
    "#endif\n"
    "}\n";

static const char* texture_grayscaled =
    "vec4 texture_grayscaled( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  return vec4( texture( sampler, texCoord ).r );\n"
    "}\n";

static const char* texture_nm_xyz =
    "vec3 texture_nm_xyz( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;\n"
    "}\n";

static const char* texture_nm_xy =
    "vec3 texture_nm_xy( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;\n"
    "  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );\n"
    "  return decodedN;\n"
    "}\n";

static const char* texture_nm_spheremap =
    "vec3 texture_nm_spheremap( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;\n"
    "  float f = dot( fenc, fenc );\n"
    "  vec3 decodedN;\n"
    "  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );\n"
    "  decodedN.z = 1.0 - f / 2.0;\n"
    "  return decodedN;\n"
    "}\n";

static const char* texture_nm_stereographic =
    "vec3 texture_nm_stereographic( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec3 decodedN;\n"
    "  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;\n"
    "  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );\n"
    "  decodedN.xy *= denom;\n"
    "  decodedN.z = denom - 1.0;\n"
    "  return decodedN;\n"
    "}\n";

static const char* texture_nm_paraboloid =
    "vec3 texture_nm_paraboloid( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec3 decodedN;\n"
    "  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;\n"
    "  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );\n"
    //n = normalize( n );
    "  return decodedN;\n"
    "}\n";

static const char* texture_nm_quartic =
    "vec3 texture_nm_quartic( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec3 decodedN;\n"
    "  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;\n"
    "  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );\n"
    //n = normalize( n );
    "  return decodedN;\n"
    "}\n";

static const char* texture_nm_float =
    "vec3 texture_nm_float( in %s sampler, in %s texCoord )\n"
    "{\n"
    "  vec3 decodedN;\n"
    "  decodedN.xy = texture( sampler, texCoord ).xy;\n"
    "  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );\n"
    "  return decodedN;\n"
    "}\n";

static const char* texture_nm_dxt5 =
    "vec3 texture_nm_dxt5( in %s sampler, in %s texCoord )\n"
    "{\n"
//  vec4 bump = texture( sampler, texCoord );
//	specularPage.xyz = ScaleSpecular( specularPage.xyz, bump.z );
//	float powerFactor = bump.x;
#if 1
    "  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;\n"
    "  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );\n"
    "  decodedN = normalize( decodedN );\n"
#else
    "  vec3 decodedN = texture( sampler, texCoord ).wyz  * 2.0 - 1.0;\n"
    "  decodedN.z = sqrt( 1.0 - Saturate( dot( decodedN.xy, decodedN.xy ) ) );\n"
#endif
    "  return decodedN;\n"
    "}\n";

static const char* builtin_spheremap_coord =
    "vec2 builtin_spheremap_coord( in vec3 dir ) {\n"
    "  vec2 uv = vec2( atan( dir.z, dir.x ), asin( dir.y ) );\n"
    "  return uv * vec2(0.1591, 0.3183) + 0.5;\n"
    "}\n";

static const char* builtin_luminance =
    "float builtin_luminance( in vec3 color ) {\n"
    "  return dot( color, vec3( 0.2126, 0.7152, 0.0722 ) );\n"
    "}\n"
    "float builtin_luminance( in vec4 color ) {\n"
    "  return dot( color, vec4( 0.2126, 0.7152, 0.0722, 0.0 ) );\n"
    "}\n";

static const char* builtin_saturate =
    "%s builtin_saturate( in %s color ) {\n"
    "  return clamp( color, %s(0.0), %s(1.0) );\n"
    "}\n";

static void GenerateBuiltinSource()
{
    TSprintfBuffer<4096> format;

    String builtin;
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_srgb_alpha, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_ycocg, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_grayscaled, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_xyz, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_xy, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_spheremap, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_stereographic, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_paraboloid, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_quartic, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_float, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }
    for (int i = 0; i < TEXTURE_TYPE_MAX; i++)
    {
        builtin += format.Sprintf(texture_nm_dxt5, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1]);
    }

    builtin += builtin_spheremap_coord;
    builtin += builtin_luminance;

    for (int i = MG_VALUE_TYPE_FLOAT1; i <= MG_VALUE_TYPE_FLOAT4; i++)
    {
        builtin += format.Sprintf(builtin_saturate, VariableTypeStr[i], VariableTypeStr[i], VariableTypeStr[i], VariableTypeStr[i]);
    }

    File f = File::OpenWrite("material_builtin.glsl");
    if (f)
        f.Write(builtin.CStr(), builtin.Size());
}

static void WriteDebugShaders(TVector<MaterialSource> const& Shaders)
{
    File f = File::OpenWrite("debug.glsl");
    if (!f)
    {
        return;
    }

    for (MaterialSource const& shader : Shaders)
    {
        f.FormattedPrint("//----------------------------------\n// {}\n//----------------------------------\n", shader.SourceName);
        f.FormattedPrint("{}\n", shader.Code);
    }
}

static String GenerateOutputVaryingsCode(TVector<StageVarying> const& Varyings, StringView Prefix, bool bArrays)
{
    String  s;
    uint32_t location = 0;
    for (StageVarying const& v : Varyings)
    {
        if (v.RefCount > 0)
        {
            s += "layout( location = " + Core::ToString(location) + " ) out " + VariableTypeStr[v.VaryingType] + " " + Prefix + v.VaryingName;
            if (bArrays)
                s += "[]";
            s += ";\n";
        }
        location++;
    }
    return s;
}

static String GenerateInputVaryingsCode(TVector<StageVarying> const& Varyings, StringView Prefix, bool bArrays)
{
    String  s;
    uint32_t location = 0;
    for (StageVarying const& v : Varyings)
    {
        if (v.RefCount > 0)
        {
            s += "layout( location = " + Core::ToString(location) + " ) in " + VariableTypeStr[v.VaryingType] + " " + Prefix + v.VaryingName;
            if (bArrays)
                s += "[]";
            s += ";\n";
        }
        location++;
    }
    return s;
}

#if 0
static void MergeVaryings( TVector< StageVarying > const & A, TVector< StageVarying > const & B, TVector< StageVarying > & Result )
{
    Result = A;
    for ( int i = 0 ; i < B.Size() ; i++ ) {
        bool bMatch = false;
        for ( int j = 0 ; j < A.Size() ; j++ ) {
            if ( B[i].VaryingName == A[j].VaryingName ) {
                bMatch = true;
                break;
            }
        }
        if ( !bMatch ) {
            Result.Add( B[i] );
        }
    }
}

static bool FindVarying( TVector< StageVarying > const & Varyings, StageVarying const & Var )
{
    for ( int i = 0 ; i < Varyings.Size() ; i++ ) {
        if ( Var.VaryingName == Varyings[i].VaryingName ) {
            return true;
        }
    }
    return false;
}
#endif

static void AddVaryings(TVector<StageVarying>& InOutResult, TVector<StageVarying> const& B)
{
    if (InOutResult.IsEmpty())
    {
        InOutResult = B;
        for (int i = 0; i < InOutResult.Size(); i++)
        {
            InOutResult[i].RefCount = 1;
        }
        return;
    }

    for (int i = 0; i < B.Size(); i++)
    {
        bool bMatch = false;
        for (int j = 0; j < InOutResult.Size(); j++)
        {
            if (B[i].VaryingName == InOutResult[j].VaryingName)
            {
                bMatch = true;
                InOutResult[j].RefCount++;
                break;
            }
        }
        if (!bMatch)
        {
            InOutResult.Add(B[i]);
        }
    }
}

static void RemoveVaryings(TVector<StageVarying>& InOutResult, TVector<StageVarying> const& B)
{
    for (int i = 0; i < B.Size(); i++)
    {
        for (int j = 0; j < InOutResult.Size(); j++)
        {
            if (B[i].VaryingName == InOutResult[j].VaryingName)
            {
                InOutResult[j].RefCount--;
                break;
            }
        }
    }
}

struct MaterialStageTransition
{
    TVector<StageVarying> Varyings;
    int               MaxTextureSlot;
    int               MaxUniformAddress;

    String VS_OutputVaryingsCode;
    String VS_CopyVaryingsCode;

    String TCS_OutputVaryingsCode;
    String TCS_InputVaryingsCode;
    String TCS_CopyVaryingsCode;

    String TES_OutputVaryingsCode;
    String TES_InputVaryingsCode;
    String TES_CopyVaryingsCode;

    String GS_OutputVaryingsCode;
    String GS_InputVaryingsCode;
    String GS_CopyVaryingsCode;

    String FS_InputVaryingsCode;
    String FS_CopyVaryingsCode;
};

void MGMaterialGraph::CreateStageTransitions(MaterialStageTransition&    Transition,
                                             MaterialBuildContext const* VertexStage,
                                             MaterialBuildContext const* TessControlStage,
                                             MaterialBuildContext const* TessEvalStage,
                                             MaterialBuildContext const* GeometryStage,
                                             MaterialBuildContext const* FragmentStage)
{
    HK_ASSERT(VertexStage != nullptr);

    TVector<StageVarying>& varyings = Transition.Varyings;

    varyings.Clear();

    Transition.MaxTextureSlot    = VertexStage->MaxTextureSlot;
    Transition.MaxUniformAddress = VertexStage->MaxUniformAddress;

    if (FragmentStage)
    {
        AddVaryings(varyings, FragmentStage->InputVaryings);

        Transition.MaxTextureSlot    = Math::Max(Transition.MaxTextureSlot, FragmentStage->MaxTextureSlot);
        Transition.MaxUniformAddress = Math::Max(Transition.MaxUniformAddress, FragmentStage->MaxUniformAddress);
    }

    if (GeometryStage)
    {
        AddVaryings(varyings, GeometryStage->InputVaryings);

        Transition.MaxTextureSlot    = Math::Max(Transition.MaxTextureSlot, GeometryStage->MaxTextureSlot);
        Transition.MaxUniformAddress = Math::Max(Transition.MaxUniformAddress, GeometryStage->MaxUniformAddress);
    }

    if (TessEvalStage && TessControlStage)
    {
        AddVaryings(varyings, TessEvalStage->InputVaryings);
        AddVaryings(varyings, TessControlStage->InputVaryings);

        Transition.MaxTextureSlot    = Math::Max(Transition.MaxTextureSlot, TessEvalStage->MaxTextureSlot);
        Transition.MaxUniformAddress = Math::Max(Transition.MaxUniformAddress, TessEvalStage->MaxUniformAddress);

        Transition.MaxTextureSlot    = Math::Max(Transition.MaxTextureSlot, TessControlStage->MaxTextureSlot);
        Transition.MaxUniformAddress = Math::Max(Transition.MaxUniformAddress, TessControlStage->MaxUniformAddress);
    }

    for (StageVarying const& v : varyings)
    {
        Transition.VS_CopyVaryingsCode += "VS_" + v.VaryingName + " = " + v.VaryingSource + ";\n";
    }

    Transition.VS_OutputVaryingsCode = GenerateOutputVaryingsCode(varyings, "VS_", false);

    const char* lastPrefix = "VS_";

    if (TessEvalStage && TessControlStage)
    {
        Transition.TCS_InputVaryingsCode = GenerateInputVaryingsCode(varyings, "VS_", true);
        RemoveVaryings(varyings, TessControlStage->InputVaryings);

        if (TessellationMethod == TESSELLATION_FLAT)
        {
            Transition.TCS_OutputVaryingsCode = GenerateOutputVaryingsCode(varyings, "TCS_", true);

            for (StageVarying const& v : varyings)
            {
                if (v.RefCount == 0)
                {
                    Transition.TCS_CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    Transition.TCS_CopyVaryingsCode += " " + v.VaryingName + " = VS_" + v.VaryingName + "[gl_InvocationID];\n";
                }
                else
                {
                    Transition.TCS_CopyVaryingsCode += "TCS_" + v.VaryingName + "[gl_InvocationID] = VS_" + v.VaryingName + "[gl_InvocationID];\n";
                    Transition.TCS_CopyVaryingsCode += "#define " + v.VaryingName + " VS_" + v.VaryingName + "[gl_InvocationID]\n";
                }
            }

            Transition.TES_InputVaryingsCode = GenerateInputVaryingsCode(varyings, "TCS_", true);
            RemoveVaryings(varyings, TessEvalStage->InputVaryings);
            Transition.TES_OutputVaryingsCode = GenerateOutputVaryingsCode(varyings, "TES_", false);

            for (StageVarying const& v : varyings)
            {
                if (v.RefCount == 0)
                {
                    Transition.TES_CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    Transition.TES_CopyVaryingsCode += " ";
                }
                Transition.TES_CopyVaryingsCode += "TES_" + v.VaryingName +
                    " = gl_TessCoord.x * TCS_" + v.VaryingName + "[0]" +
                    " + gl_TessCoord.y * TCS_" + v.VaryingName + "[1]" +
                    " + gl_TessCoord.z * TCS_" + v.VaryingName + "[2];\n";
            }
        }
        else
        { // TESSELLATION_PN

            String patchLocationStr;

            int patchLocation = 0;
            Transition.TCS_OutputVaryingsCode.Clear();
            for (StageVarying const& v : varyings)
            {
                if (v.RefCount > 0)
                {
                    Transition.TCS_OutputVaryingsCode += "layout( location = " + Core::ToString(patchLocation) + " ) out patch " + VariableTypeStr[v.VaryingType] + " TCS_" + v.VaryingName + "[3];\n";
                    patchLocation += 3;
                }
            }
            patchLocationStr = "#define PATCH_LOCATION " + Core::ToString(patchLocation);
            Transition.TCS_OutputVaryingsCode += patchLocationStr;

            for (StageVarying const& v : varyings)
            {
                if (v.RefCount == 0)
                {
                    Transition.TCS_CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    Transition.TCS_CopyVaryingsCode += " " + v.VaryingName + " = VS_" + v.VaryingName + "[0];\n";
                }
            }

            Transition.TCS_CopyVaryingsCode += "for ( int i = 0 ; i < 3 ; i++ ) {\n";
            for (StageVarying const& v : varyings)
            {
                if (v.RefCount > 0)
                {
                    Transition.TCS_CopyVaryingsCode += "TCS_" + v.VaryingName + "[i] = VS_" + v.VaryingName + "[i];\n";
                    Transition.TCS_CopyVaryingsCode += "#define " + v.VaryingName + " VS_" + v.VaryingName + "[0]\n";
                }
            }
            Transition.TCS_CopyVaryingsCode += "}\n";

            patchLocation = 0;
            Transition.TES_InputVaryingsCode.Clear();
            for (StageVarying const& v : varyings)
            {
                if (v.RefCount > 0)
                {
                    //Transition.TES_InputVaryingsCode += String( VariableTypeStr[v.VaryingType] ) + " " + v.VaryingName + "[3];\n";
                    Transition.TES_InputVaryingsCode += "layout( location = " + Core::ToString(patchLocation) + " ) in patch " + VariableTypeStr[v.VaryingType] + " TCS_" + v.VaryingName + "[3];\n";
                    patchLocation += 3;
                }
            }
            Transition.TES_InputVaryingsCode += patchLocationStr;

            RemoveVaryings(varyings, TessEvalStage->InputVaryings);
            Transition.TES_OutputVaryingsCode = GenerateOutputVaryingsCode(varyings, "TES_", false);

            for (StageVarying const& v : varyings)
            {
                if (v.RefCount == 0)
                {
                    Transition.TES_CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    Transition.TES_CopyVaryingsCode += " ";
                }
                Transition.TES_CopyVaryingsCode += "TES_" + v.VaryingName +
                    " = gl_TessCoord.x * TCS_" + v.VaryingName + "[0]" +
                    " + gl_TessCoord.y * TCS_" + v.VaryingName + "[1]" +
                    " + gl_TessCoord.z * TCS_" + v.VaryingName + "[2];\n";
            }
        }

        for (StageVarying const& v : TessEvalStage->InputVaryings)
        {
            Transition.TES_CopyVaryingsCode += "#define " + v.VaryingName + " TES_" + v.VaryingName + "\n";
        }

        lastPrefix = "TES_";
    }

    if (GeometryStage)
    {
        Transition.GS_InputVaryingsCode = GenerateInputVaryingsCode(varyings, lastPrefix, true);
        RemoveVaryings(varyings, GeometryStage->InputVaryings);
        Transition.GS_OutputVaryingsCode = GenerateOutputVaryingsCode(varyings, "GS_", false);

        for (StageVarying const& v : varyings)
        {
            if (v.RefCount > 0)
            {
                Transition.GS_CopyVaryingsCode += "GS_" + v.VaryingName + " = " + lastPrefix + v.VaryingName + "[i];\n";
            }
        }

        lastPrefix = "GS_";
    }

    if (FragmentStage)
    {
        Transition.FS_InputVaryingsCode = GenerateInputVaryingsCode(varyings, lastPrefix, false);
        RemoveVaryings(varyings, FragmentStage->InputVaryings);

        if (*lastPrefix)
        {
            for (StageVarying const& v : FragmentStage->InputVaryings)
            {
                Transition.FS_InputVaryingsCode += "#define " + v.VaryingName + " " + lastPrefix + v.VaryingName + "\n";
            }
        }
    }

#ifdef HK_DEBUG
    for (StageVarying const& v : varyings)
    {
        HK_ASSERT(v.RefCount == 0);
    }
#endif

#if 0
    File f = File::OpenWrite("debug2.glsl");
    if (f)
    {
        f.Printf("VERTEX VARYINGS:\n%s\n", VertexStage->OutputVaryingsCode.CStr());

        if (TessEvalStage && TessControlStage)
        {
            f.Printf("TCS INPUT VARYINGS:\n%s\n", TessControlStage->InputVaryingsCode.CStr());
            f.Printf("TCS OUTPUT VARYINGS:\n%s\n", TessControlStage->OutputVaryingsCode.CStr());
            f.Printf("TCS COPY VARYINGS:\n%s\n", TessControlStage->CopyVaryingsCode.CStr());

            f.Printf("TES INPUT VARYINGS:\n%s\n", TessEvalStage->InputVaryingsCode.CStr());
            f.Printf("TES OUTPUT VARYINGS:\n%s\n", TessEvalStage->OutputVaryingsCode.CStr());
            f.Printf("TES COPY VARYINGS:\n%s\n", TessEvalStage->CopyVaryingsCode.CStr());
        }

        if (GeometryStage)
        {
            f.Printf("GS INPUT VARYINGS:\n%s\n", GeometryStage->InputVaryingsCode.CStr());
            f.Printf("GS OUTPUT VARYINGS:\n%s\n", GeometryStage->OutputVaryingsCode.CStr());
            f.Printf("GS COPY VARYINGS:\n%s\n", GeometryStage->CopyVaryingsCode.CStr());
        }

        if (FragmentStage)
        {
            f.Printf("FS INPUT VARYINGS:\n%s\n", FragmentStage->InputVaryingsCode.CStr());
            f.Printf("FS COPY VARYINGS:\n%s\n", FragmentStage->CopyVaryingsCode.CStr());
        }
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

HK_BEGIN_CLASS_META(MGMaterialGraph)
HK_PROPERTY_DIRECT(MaterialType, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(TessellationMethod, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(RenderingPriority, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(Blending, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(ParallaxTechnique, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(DepthHack, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(MotionBlurScale, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(AlphaMaskCutOff, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bDepthTest, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bTranslucent, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bTwoSided, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bNoLightmap, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bAllowScreenSpaceReflections, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bAllowScreenAmbientOcclusion, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bAllowShadowReceive, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bDisplacementAffectShadow, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bParallaxMappingSelfShadowing, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bPerBoneMotionBlur, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bUseVirtualTexture, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

MGMaterialGraph::MGMaterialGraph() :
    Super("Material Graph")
{
    SetSlots({&Color, &Normal, &Metallic, &Roughness, &AmbientOcclusion, &AmbientLight, &Emissive, &Specular, &Opacity, &VertexDeform, &AlphaMask, &ShadowMask, &Displacement, &TessellationFactor}, {});
}

MGMaterialGraph::~MGMaterialGraph()
{
    for (MGNode* node : m_Nodes)
    {
        node->RemoveRef();
    }
}

void MGMaterialGraph::CompileStage(MaterialBuildContext& ctx)
{
    static int BuildSerial = 0;

    ctx.Serial = ++BuildSerial;

    ResetConnections(ctx);
    TouchConnections(ctx);
    Build(ctx);
}

TRef<CompiledMaterial> MGMaterialGraph::Compile()
{
    int n = 0;
    for (auto textureSlot : GetTextures())
    {
        if (!textureSlot)
        {
            LOG("Uninitialized texture slot {}\n", n);
            return {};
        }
        n++;
    }

    int maxUniformAddress = -1;

    TRef<CompiledMaterial> material = MakeRef<CompiledMaterial>();

    material->Type                      = MaterialType;
    material->Blending                  = Blending;
    material->TessellationMethod        = TessellationMethod;
    material->RenderingPriority         = RENDERING_PRIORITY_DEFAULT;
    material->bDepthTest_EXPERIMENTAL   = bDepthTest;
    material->bDisplacementAffectShadow = bDisplacementAffectShadow;
    //material->bParallaxMappingSelfShadowing = bParallaxMappingSelfShadowing;
    material->bTranslucent      = bTranslucent;
    material->bTwoSided         = bTwoSided;
    material->bAlphaMasking     = false;
    material->bShadowMapMasking = false;
    material->bHasVertexDeform  = false;
    material->bNoCastShadow     = false;
    material->LightmapSlot      = 0;

    String predefines;

    switch (MaterialType)
    {
        case MATERIAL_TYPE_UNLIT:
            predefines += "#define MATERIAL_TYPE_UNLIT\n";
            break;
        case MATERIAL_TYPE_BASELIGHT:
            predefines += "#define MATERIAL_TYPE_BASELIGHT\n";
            break;
        case MATERIAL_TYPE_PBR:
            predefines += "#define MATERIAL_TYPE_PBR\n";
            break;
        case MATERIAL_TYPE_HUD:
            predefines += "#define MATERIAL_TYPE_HUD\n";
            break;
        case MATERIAL_TYPE_POSTPROCESS:
            predefines += "#define MATERIAL_TYPE_POSTPROCESS\n";
            break;
        default:
            HK_ASSERT(0);
    }

    switch (TessellationMethod)
    {
        case TESSELLATION_FLAT:
            predefines += "#define TESSELLATION_METHOD TESSELLATION_FLAT\n";
            break;
        case TESSELLATION_PN:
            predefines += "#define TESSELLATION_METHOD TESSELLATION_PN\n";
            break;
        default:
            break;
    }

    material->RenderingPriority = RenderingPriority;

    if (DepthHack == MATERIAL_DEPTH_HACK_WEAPON)
    {
        predefines += "#define WEAPON_DEPTH_HACK\n";
        material->bNoCastShadow     = true;
        material->RenderingPriority = RENDERING_PRIORITY_WEAPON;

        if (RenderingPriority != RENDERING_PRIORITY_DEFAULT && RenderingPriority != RENDERING_PRIORITY_WEAPON)
            LOG("MATERIAL_DEPTH_HACK_WEAPON overrides RenderingPriority with RENDERING_PRIORITY_WEAPON.\n");
    }
    else if (DepthHack == MATERIAL_DEPTH_HACK_SKYBOX)
    {
        predefines += "#define SKYBOX_DEPTH_HACK\n";
        material->bNoCastShadow     = true;
        material->RenderingPriority = RENDERING_PRIORITY_SKYBOX;

        if (RenderingPriority != RENDERING_PRIORITY_DEFAULT && RenderingPriority != RENDERING_PRIORITY_SKYBOX)
            LOG("MATERIAL_DEPTH_HACK_SKYBOX overrides RenderingPriority with RENDERING_PRIORITY_SKYBOX.\n");
    }

    if (bTranslucent)
    {
        predefines += "#define TRANSLUCENT\n";
    }

    if (bTwoSided)
    {
        predefines += "#define TWOSIDED\n";
    }

    if (bNoLightmap)
    {
        predefines += "#define NO_LIGHTMAP\n";
    }

    if (bAllowScreenSpaceReflections)
    {
        predefines += "#define ALLOW_SSLR\n";
    }

    if (bAllowScreenAmbientOcclusion)
    {
        predefines += "#define ALLOW_SSAO\n";
    }

    if (bAllowShadowReceive)
    {
        predefines += "#define ALLOW_SHADOW_RECEIVE\n";
    }

    if (bDisplacementAffectShadow)
    {
        predefines += "#define DISPLACEMENT_AFFECT_SHADOW\n";
    }

    if (bParallaxMappingSelfShadowing)
    {
        predefines += "#define PARALLAX_SELF_SHADOW\n";
    }

    if (bPerBoneMotionBlur)
    {
        predefines += "#define PER_BONE_MOTION_BLUR\n";
    }

    if (MotionBlurScale > 0.0f && !bTranslucent)
    {
        predefines += "#define ALLOW_MOTION_BLUR\n";
    }

    predefines += "#define MOTION_BLUR_SCALE " + Core::ToString(Math::Saturate(MotionBlurScale)) + "\n";

    if (bUseVirtualTexture)
    {
        predefines += "#define USE_VIRTUAL_TEXTURE\n";
        predefines += "#define VT_LAYERS 1\n"; // TODO: Add based on material
    }

    if (!bDepthTest /*|| bTranslucent */)
    {
        material->bNoCastShadow = true;
    }

    if (Blending == COLOR_BLENDING_PREMULTIPLIED_ALPHA)
    {
        predefines += "#define PREMULTIPLIED_ALPHA\n";
    }

    bool bTess = TessellationMethod != TESSELLATION_DISABLED;

    // TODO:
    // if there is no specific vertex deformation / tessellation / alpha masking or other non-trivial material feature,
    // then use default shader/pipeline for corresponding material pass.
    // This is optimization of switching between pipelines.
    // material->bDepthPassDefaultPipeline
    // material->bShadowMapPassDefaultPipeline
    // material->bWireframePassDefaultPipeline
    // material->bNormalsPassDefaultPipeline

    // Create depth pass
    {
        MaterialBuildContext    vertexCtx(this, MATERIAL_STAGE_VERTEX);
        MaterialBuildContext    tessControlCtx(this, MATERIAL_STAGE_TESSELLATION_CONTROL);
        MaterialBuildContext    tessEvalCtx(this, MATERIAL_STAGE_TESSELLATION_EVAL);
        MaterialBuildContext    depthCtx(this, MATERIAL_STAGE_DEPTH);
        MaterialStageTransition trans;

        CompileStage(vertexCtx);
        CompileStage(depthCtx);

        if (bTess)
        {
            CompileStage(tessControlCtx);
            CompileStage(tessEvalCtx);
        }

        CreateStageTransitions(trans,
                               &vertexCtx,
                               bTess ? &tessControlCtx : nullptr,
                               bTess ? &tessEvalCtx : nullptr,
                               nullptr,
                               /*depthCtx.bHasAlphaMask ? */ &depthCtx /* : nullptr*/);

        material->bHasVertexDeform      = vertexCtx.bHasVertexDeform;
        material->bAlphaMasking         = depthCtx.bHasAlphaMask;
        material->DepthPassTextureCount = trans.MaxTextureSlot + 1;
        maxUniformAddress               = Math::Max(maxUniformAddress, trans.MaxUniformAddress);

        int locationIndex = trans.Varyings.Size();

        predefines += "#define DEPTH_PASS_VARYING_POSITION " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define DEPTH_PASS_VARYING_NORMAL " + Core::ToString(locationIndex++) + "\n";

        predefines += "#define DEPTH_PASS_VARYING_VERTEX_POSITION_CURRENT " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define DEPTH_PASS_VARYING_VERTEX_POSITION_PREVIOUS " + Core::ToString(locationIndex++) + "\n";

        material->AddShader("$DEPTH_PASS_VERTEX_OUTPUT_VARYINGS$", trans.VS_OutputVaryingsCode);
        material->AddShader("$DEPTH_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$DEPTH_PASS_VERTEX_CODE$", vertexCtx.SourceCode + trans.VS_CopyVaryingsCode);

        material->AddShader("$DEPTH_PASS_TCS_INPUT_VARYINGS$", trans.TCS_InputVaryingsCode);
        material->AddShader("$DEPTH_PASS_TCS_OUTPUT_VARYINGS$", trans.TCS_OutputVaryingsCode);
        material->AddShader("$DEPTH_PASS_TCS_SAMPLERS$", SamplersString(tessControlCtx.MaxTextureSlot));
        material->AddShader("$DEPTH_PASS_TCS_COPY_VARYINGS$", trans.TCS_CopyVaryingsCode);
        material->AddShader("$DEPTH_PASS_TCS_CODE$", tessControlCtx.SourceCode);

        material->AddShader("$DEPTH_PASS_TES_INPUT_VARYINGS$", trans.TES_InputVaryingsCode);
        material->AddShader("$DEPTH_PASS_TES_OUTPUT_VARYINGS$", trans.TES_OutputVaryingsCode);
        material->AddShader("$DEPTH_PASS_TES_SAMPLERS$", SamplersString(tessEvalCtx.MaxTextureSlot));
        material->AddShader("$DEPTH_PASS_TES_INTERPOLATE$", trans.TES_CopyVaryingsCode);
        material->AddShader("$DEPTH_PASS_TES_CODE$", tessEvalCtx.SourceCode);

        material->AddShader("$DEPTH_PASS_FRAGMENT_INPUT_VARYINGS$", trans.FS_InputVaryingsCode);
        material->AddShader("$DEPTH_PASS_FRAGMENT_SAMPLERS$", SamplersString(depthCtx.MaxTextureSlot));
        material->AddShader("$DEPTH_PASS_FRAGMENT_CODE$", depthCtx.SourceCode);
    }

    // Create shadowmap pass
    {
        MaterialBuildContext    vertexCtx(this, MATERIAL_STAGE_VERTEX);
        MaterialBuildContext    tessControlCtx(this, MATERIAL_STAGE_TESSELLATION_CONTROL);
        MaterialBuildContext    tessEvalCtx(this, MATERIAL_STAGE_TESSELLATION_EVAL);
        MaterialBuildContext    geometryCtx(this, MATERIAL_STAGE_GEOMETRY);
        MaterialBuildContext    shadowCtx(this, MATERIAL_STAGE_SHADOWCAST);
        MaterialStageTransition trans;

        CompileStage(vertexCtx);
        CompileStage(shadowCtx);

        bool bTessShadowmap = (TessellationMethod == TESSELLATION_PN) || (TessellationMethod == TESSELLATION_FLAT && bDisplacementAffectShadow);

        if (bTessShadowmap)
        {
            CompileStage(tessControlCtx);
            CompileStage(tessEvalCtx);
        }

        CreateStageTransitions(trans,
                               &vertexCtx,
                               bTessShadowmap ? &tessControlCtx : nullptr,
                               bTessShadowmap ? &tessEvalCtx : nullptr,
                               &geometryCtx,
                               shadowCtx.bHasShadowMask ? &shadowCtx : nullptr);

        material->bShadowMapMasking         = shadowCtx.bHasShadowMask;
        material->ShadowMapPassTextureCount = trans.MaxTextureSlot + 1;
        maxUniformAddress                   = Math::Max(maxUniformAddress, trans.MaxUniformAddress);

        int locationIndex = trans.Varyings.Size();

        //predefines += "#define SHADOWMAP_PASS_VARYING_INSTANCE_ID " + Core::ToString( locationIndex++ ) + "\n";
        predefines += "#define SHADOWMAP_PASS_VARYING_POSITION " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define SHADOWMAP_PASS_VARYING_NORMAL " + Core::ToString(locationIndex++) + "\n";

        material->AddShader("$SHADOWMAP_PASS_VERTEX_OUTPUT_VARYINGS$", trans.VS_OutputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$SHADOWMAP_PASS_VERTEX_CODE$", vertexCtx.SourceCode + trans.VS_CopyVaryingsCode);

        material->AddShader("$SHADOWMAP_PASS_TCS_INPUT_VARYINGS$", trans.TCS_InputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_TCS_OUTPUT_VARYINGS$", trans.TCS_OutputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_TCS_SAMPLERS$", SamplersString(tessControlCtx.MaxTextureSlot));
        material->AddShader("$SHADOWMAP_PASS_TCS_COPY_VARYINGS$", trans.TCS_CopyVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_TCS_CODE$", tessControlCtx.SourceCode);

        material->AddShader("$SHADOWMAP_PASS_TES_INPUT_VARYINGS$", trans.TES_InputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_TES_OUTPUT_VARYINGS$", trans.TES_OutputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_TES_SAMPLERS$", SamplersString(tessEvalCtx.MaxTextureSlot));
        material->AddShader("$SHADOWMAP_PASS_TES_INTERPOLATE$", trans.TES_CopyVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_TES_CODE$", tessEvalCtx.SourceCode);

        material->AddShader("$SHADOWMAP_PASS_GEOMETRY_INPUT_VARYINGS$", trans.GS_InputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_GEOMETRY_OUTPUT_VARYINGS$", trans.GS_OutputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_GEOMETRY_COPY_VARYINGS$", trans.GS_CopyVaryingsCode);

        material->AddShader("$SHADOWMAP_PASS_FRAGMENT_INPUT_VARYINGS$", trans.FS_InputVaryingsCode);
        material->AddShader("$SHADOWMAP_PASS_FRAGMENT_SAMPLERS$", SamplersString(shadowCtx.MaxTextureSlot));
        material->AddShader("$SHADOWMAP_PASS_FRAGMENT_CODE$", shadowCtx.SourceCode);
    }

    // Create omnidirectional shadowmap pass
    {
        MaterialBuildContext    vertexCtx(this, MATERIAL_STAGE_VERTEX);
        MaterialBuildContext    tessControlCtx(this, MATERIAL_STAGE_TESSELLATION_CONTROL);
        MaterialBuildContext    tessEvalCtx(this, MATERIAL_STAGE_TESSELLATION_EVAL);
        MaterialBuildContext    shadowCtx(this, MATERIAL_STAGE_SHADOWCAST);
        MaterialStageTransition trans;

        CompileStage(vertexCtx);
        CompileStage(shadowCtx);

        bool bTessShadowmap = (TessellationMethod == TESSELLATION_PN) || (TessellationMethod == TESSELLATION_FLAT && bDisplacementAffectShadow);

        if (bTessShadowmap)
        {
            CompileStage(tessControlCtx);
            CompileStage(tessEvalCtx);
        }

        CreateStageTransitions(trans,
                               &vertexCtx,
                               bTessShadowmap ? &tessControlCtx : nullptr,
                               bTessShadowmap ? &tessEvalCtx : nullptr,
                               nullptr,
                               shadowCtx.bHasShadowMask ? &shadowCtx : nullptr);

        // NOTE: bShadowMapMasking and ShadowMapPassTextureCount should be same as in directional shadow map pass
        material->bShadowMapMasking         = shadowCtx.bHasShadowMask;
        material->ShadowMapPassTextureCount = trans.MaxTextureSlot + 1;
        maxUniformAddress                   = Math::Max(maxUniformAddress, trans.MaxUniformAddress);

        int locationIndex = trans.Varyings.Size();

        //predefines += "#define OMNI_SHADOWMAP_PASS_VARYING_INSTANCE_ID " + Core::ToString( locationIndex++ ) + "\n";
        predefines += "#define OMNI_SHADOWMAP_PASS_VARYING_POSITION " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define OMNI_SHADOWMAP_PASS_VARYING_NORMAL " + Core::ToString(locationIndex++) + "\n";

        material->AddShader("$OMNI_SHADOWMAP_PASS_VERTEX_OUTPUT_VARYINGS$", trans.VS_OutputVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$OMNI_SHADOWMAP_PASS_VERTEX_CODE$", vertexCtx.SourceCode + trans.VS_CopyVaryingsCode);

        material->AddShader("$OMNI_SHADOWMAP_PASS_TCS_INPUT_VARYINGS$", trans.TCS_InputVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_TCS_OUTPUT_VARYINGS$", trans.TCS_OutputVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_TCS_SAMPLERS$", SamplersString(tessControlCtx.MaxTextureSlot));
        material->AddShader("$OMNI_SHADOWMAP_PASS_TCS_COPY_VARYINGS$", trans.TCS_CopyVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_TCS_CODE$", tessControlCtx.SourceCode);

        material->AddShader("$OMNI_SHADOWMAP_PASS_TES_INPUT_VARYINGS$", trans.TES_InputVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_TES_OUTPUT_VARYINGS$", trans.TES_OutputVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_TES_SAMPLERS$", SamplersString(tessEvalCtx.MaxTextureSlot));
        material->AddShader("$OMNI_SHADOWMAP_PASS_TES_INTERPOLATE$", trans.TES_CopyVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_TES_CODE$", tessEvalCtx.SourceCode);

        material->AddShader("$OMNI_SHADOWMAP_PASS_FRAGMENT_INPUT_VARYINGS$", trans.FS_InputVaryingsCode);
        material->AddShader("$OMNI_SHADOWMAP_PASS_FRAGMENT_SAMPLERS$", SamplersString(shadowCtx.MaxTextureSlot));
        material->AddShader("$OMNI_SHADOWMAP_PASS_FRAGMENT_CODE$", shadowCtx.SourceCode);
    }

    // Create light pass
    {
        MaterialBuildContext    vertexCtx(this, MATERIAL_STAGE_VERTEX);
        MaterialBuildContext    tessControlCtx(this, MATERIAL_STAGE_TESSELLATION_CONTROL);
        MaterialBuildContext    tessEvalCtx(this, MATERIAL_STAGE_TESSELLATION_EVAL);
        MaterialBuildContext    lightCtx(this, MATERIAL_STAGE_LIGHT);
        MaterialStageTransition trans;

        CompileStage(vertexCtx);
        CompileStage(lightCtx);

        if (bTess)
        {
            CompileStage(tessControlCtx);
            CompileStage(tessEvalCtx);
        }

        CreateStageTransitions(trans,
                               &vertexCtx,
                               bTess ? &tessControlCtx : nullptr,
                               bTess ? &tessEvalCtx : nullptr,
                               nullptr,
                               &lightCtx);

        material->LightPassTextureCount = trans.MaxTextureSlot + 1;
        maxUniformAddress               = Math::Max(maxUniformAddress, trans.MaxUniformAddress);

        int locationIndex = trans.Varyings.Size();

        predefines += "#define COLOR_PASS_VARYING_BAKED_LIGHT " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define COLOR_PASS_VARYING_TANGENT " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define COLOR_PASS_VARYING_BINORMAL " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define COLOR_PASS_VARYING_NORMAL " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define COLOR_PASS_VARYING_POSITION " + Core::ToString(locationIndex++) + "\n";

        if (bUseVirtualTexture)
        {
            predefines += "#define COLOR_PASS_VARYING_VT_TEXCOORD " + Core::ToString(locationIndex++) + "\n";
        }

        material->LightmapSlot = lightCtx.MaxTextureSlot + 1;

        predefines += "#define COLOR_PASS_TEXTURE_LIGHTMAP " + Core::ToString(material->LightmapSlot) + "\n";

        if (lightCtx.ParallaxSampler != -1)
        {
            switch (ParallaxTechnique)
            {
                case PARALLAX_TECHNIQUE_POM:
                    predefines += "#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_POM\n";
                    break;
                case PARALLAX_TECHNIQUE_RPM:
                    predefines += "#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_RPM\n";
                    break;
                case PARALLAX_TECHNIQUE_DISABLED:
                    predefines += "#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_DISABLED\n";
                    break;
                default:
                    predefines += "#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_DISABLED\n";
                    HK_ASSERT(0);
                    break;
            }

            predefines += "#define PARALLAX_SAMPLER tslot_" + Core::ToString(lightCtx.ParallaxSampler) + "\n";
        }
        else
        {
            predefines += "#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_DISABLED\n";
        }

        material->AddShader("$COLOR_PASS_VERTEX_OUTPUT_VARYINGS$", trans.VS_OutputVaryingsCode);
        material->AddShader("$COLOR_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$COLOR_PASS_VERTEX_CODE$", vertexCtx.SourceCode + trans.VS_CopyVaryingsCode);

        material->AddShader("$COLOR_PASS_TCS_INPUT_VARYINGS$", trans.TCS_InputVaryingsCode);
        material->AddShader("$COLOR_PASS_TCS_OUTPUT_VARYINGS$", trans.TCS_OutputVaryingsCode);
        material->AddShader("$COLOR_PASS_TCS_SAMPLERS$", SamplersString(tessControlCtx.MaxTextureSlot));
        material->AddShader("$COLOR_PASS_TCS_COPY_VARYINGS$", trans.TCS_CopyVaryingsCode);
        material->AddShader("$COLOR_PASS_TCS_CODE$", tessControlCtx.SourceCode);

        material->AddShader("$COLOR_PASS_TES_INPUT_VARYINGS$", trans.TES_InputVaryingsCode);
        material->AddShader("$COLOR_PASS_TES_OUTPUT_VARYINGS$", trans.TES_OutputVaryingsCode);
        material->AddShader("$COLOR_PASS_TES_SAMPLERS$", SamplersString(tessEvalCtx.MaxTextureSlot));
        material->AddShader("$COLOR_PASS_TES_INTERPOLATE$", trans.TES_CopyVaryingsCode);
        material->AddShader("$COLOR_PASS_TES_CODE$", tessEvalCtx.SourceCode);

        material->AddShader("$COLOR_PASS_FRAGMENT_INPUT_VARYINGS$", trans.FS_InputVaryingsCode);
        material->AddShader("$COLOR_PASS_FRAGMENT_SAMPLERS$", SamplersString(lightCtx.MaxTextureSlot));
        material->AddShader("$COLOR_PASS_FRAGMENT_CODE$", lightCtx.SourceCode);
    }

    // Create outline pass
    {
        MaterialBuildContext    vertexCtx(this, MATERIAL_STAGE_VERTEX);
        MaterialBuildContext    tessControlCtx(this, MATERIAL_STAGE_TESSELLATION_CONTROL);
        MaterialBuildContext    tessEvalCtx(this, MATERIAL_STAGE_TESSELLATION_EVAL);
        MaterialBuildContext    depthCtx(this, MATERIAL_STAGE_DEPTH);
        MaterialStageTransition trans;

        CompileStage(vertexCtx);
        CompileStage(depthCtx);

        if (bTess)
        {
            CompileStage(tessControlCtx);
            CompileStage(tessEvalCtx);
        }

        CreateStageTransitions(trans,
                               &vertexCtx,
                               bTess ? &tessControlCtx : nullptr,
                               bTess ? &tessEvalCtx : nullptr,
                               nullptr,
                               &depthCtx);

        //material->bHasVertexDeform = vertexCtx.bHasVertexDeform;
        //material->bAlphaMasking = depthCtx.bHasAlphaMask;
        //material->DepthPassTextureCount = trans.MaxTextureSlot + 1;
        maxUniformAddress = Math::Max(maxUniformAddress, trans.MaxUniformAddress);

        int locationIndex = trans.Varyings.Size();

        predefines += "#define OUTLINE_PASS_VARYING_POSITION " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define OUTLINE_PASS_VARYING_NORMAL " + Core::ToString(locationIndex++) + "\n";

        material->AddShader("$OUTLINE_PASS_VERTEX_OUTPUT_VARYINGS$", trans.VS_OutputVaryingsCode);
        material->AddShader("$OUTLINE_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$OUTLINE_PASS_VERTEX_CODE$", vertexCtx.SourceCode + trans.VS_CopyVaryingsCode);

        material->AddShader("$OUTLINE_PASS_TCS_INPUT_VARYINGS$", trans.TCS_InputVaryingsCode);
        material->AddShader("$OUTLINE_PASS_TCS_OUTPUT_VARYINGS$", trans.TCS_OutputVaryingsCode);
        material->AddShader("$OUTLINE_PASS_TCS_SAMPLERS$", SamplersString(tessControlCtx.MaxTextureSlot));
        material->AddShader("$OUTLINE_PASS_TCS_COPY_VARYINGS$", trans.TCS_CopyVaryingsCode);
        material->AddShader("$OUTLINE_PASS_TCS_CODE$", tessControlCtx.SourceCode);

        material->AddShader("$OUTLINE_PASS_TES_INPUT_VARYINGS$", trans.TES_InputVaryingsCode);
        material->AddShader("$OUTLINE_PASS_TES_OUTPUT_VARYINGS$", trans.TES_OutputVaryingsCode);
        material->AddShader("$OUTLINE_PASS_TES_SAMPLERS$", SamplersString(tessEvalCtx.MaxTextureSlot));
        material->AddShader("$OUTLINE_PASS_TES_INTERPOLATE$", trans.TES_CopyVaryingsCode);
        material->AddShader("$OUTLINE_PASS_TES_CODE$", tessEvalCtx.SourceCode);

        material->AddShader("$OUTLINE_PASS_FRAGMENT_INPUT_VARYINGS$", trans.FS_InputVaryingsCode);
        material->AddShader("$OUTLINE_PASS_FRAGMENT_SAMPLERS$", SamplersString(depthCtx.MaxTextureSlot));
        material->AddShader("$OUTLINE_PASS_FRAGMENT_CODE$", depthCtx.SourceCode);
    }

    // Create wireframe pass
    {
        MaterialBuildContext    vertexCtx(this, MATERIAL_STAGE_VERTEX);
        MaterialBuildContext    tessControlCtx(this, MATERIAL_STAGE_TESSELLATION_CONTROL);
        MaterialBuildContext    tessEvalCtx(this, MATERIAL_STAGE_TESSELLATION_EVAL);
        MaterialBuildContext    geometryCtx(this, MATERIAL_STAGE_GEOMETRY);
        MaterialStageTransition trans;

        CompileStage(vertexCtx);

        if (bTess)
        {
            CompileStage(tessControlCtx);
            CompileStage(tessEvalCtx);
        }

        CreateStageTransitions(trans,
                               &vertexCtx,
                               bTess ? &tessControlCtx : nullptr,
                               bTess ? &tessEvalCtx : nullptr,
                               &geometryCtx,
                               nullptr);

        material->WireframePassTextureCount = trans.MaxTextureSlot + 1;
        maxUniformAddress                   = Math::Max(maxUniformAddress, trans.MaxUniformAddress);

        int locationIndex = trans.Varyings.Size();

        predefines += "#define WIREFRAME_PASS_VARYING_POSITION " + Core::ToString(locationIndex++) + "\n";
        predefines += "#define WIREFRAME_PASS_VARYING_NORMAL " + Core::ToString(locationIndex++) + "\n";

        material->AddShader("$WIREFRAME_PASS_VERTEX_OUTPUT_VARYINGS$", trans.VS_OutputVaryingsCode);
        material->AddShader("$WIREFRAME_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$WIREFRAME_PASS_VERTEX_CODE$", vertexCtx.SourceCode + trans.VS_CopyVaryingsCode);

        material->AddShader("$WIREFRAME_PASS_TCS_INPUT_VARYINGS$", trans.TCS_InputVaryingsCode);
        material->AddShader("$WIREFRAME_PASS_TCS_OUTPUT_VARYINGS$", trans.TCS_OutputVaryingsCode);
        material->AddShader("$WIREFRAME_PASS_TCS_SAMPLERS$", SamplersString(tessControlCtx.MaxTextureSlot));
        material->AddShader("$WIREFRAME_PASS_TCS_COPY_VARYINGS$", trans.TCS_CopyVaryingsCode);
        material->AddShader("$WIREFRAME_PASS_TCS_CODE$", tessControlCtx.SourceCode);

        material->AddShader("$WIREFRAME_PASS_TES_INPUT_VARYINGS$", trans.TES_InputVaryingsCode);
        //material->AddShader( "$WIREFRAME_PASS_TES_OUTPUT_VARYINGS$", trans.TES_OutputVaryingsCode );
        material->AddShader("$WIREFRAME_PASS_TES_SAMPLERS$", SamplersString(tessEvalCtx.MaxTextureSlot));
        material->AddShader("$WIREFRAME_PASS_TES_INTERPOLATE$", trans.TES_CopyVaryingsCode);
        material->AddShader("$WIREFRAME_PASS_TES_CODE$", tessEvalCtx.SourceCode);

        //material->AddShader( "$WIREFRAME_PASS_GEOMETRY_INPUT_VARYINGS$", geometryCtx.InputVaryingsCode );
        //material->AddShader( "$WIREFRAME_PASS_GEOMETRY_OUTPUT_VARYINGS$", geometryCtx.OutputVaryingsCode );
        //material->AddShader( "$WIREFRAME_PASS_GEOMETRY_COPY_VARYINGS$", geometryCtx.CopyVaryingsCode );
    }

    // Create normals debugging pass
    {
        // Normals pass. Vertex stage
        MaterialBuildContext vertexCtx(this, MATERIAL_STAGE_VERTEX);
        CompileStage(vertexCtx);

        material->NormalsPassTextureCount = vertexCtx.MaxTextureSlot + 1;

        maxUniformAddress = Math::Max(maxUniformAddress, vertexCtx.MaxUniformAddress);

        material->AddShader("$NORMALS_PASS_VERTEX_SAMPLERS$", SamplersString(vertexCtx.MaxTextureSlot));
        material->AddShader("$NORMALS_PASS_VERTEX_CODE$", vertexCtx.SourceCode);
    }

    if (material->bHasVertexDeform)
    {
        predefines += "#define HAS_VERTEX_DEFORM\n";
    }

    material->AddShader("$PREDEFINES$", predefines);

    material->NumUniformVectors = maxUniformAddress + 1;

    int numSamplers{};
    numSamplers = 0;
    numSamplers = Math::Max(numSamplers, material->DepthPassTextureCount);
    numSamplers = Math::Max(numSamplers, material->LightPassTextureCount);
    numSamplers = Math::Max(numSamplers, material->WireframePassTextureCount);
    numSamplers = Math::Max(numSamplers, material->NormalsPassTextureCount);
    numSamplers = Math::Max(numSamplers, material->ShadowMapPassTextureCount);

    material->Samplers.Resize(numSamplers);
    for (int i = 0; i < numSamplers; i++)
    {
        MGTextureSlot* textureSlot = GetTextures()[i];

        material->Samplers[i].TextureType = textureSlot->TextureType;
        material->Samplers[i].Filter      = textureSlot->Filter;
        material->Samplers[i].AddressU    = textureSlot->AddressU;
        material->Samplers[i].AddressV    = textureSlot->AddressV;
        material->Samplers[i].AddressW    = textureSlot->AddressW;
        material->Samplers[i].MipLODBias  = textureSlot->MipLODBias;
        material->Samplers[i].Anisotropy  = textureSlot->Anisotropy;
        material->Samplers[i].MinLod      = textureSlot->MinLod;
        material->Samplers[i].MaxLod      = textureSlot->MaxLod;
    }

#if 0
    WriteDebugShaders( material->Shaders );
#endif

    return material;
}

enum MG_NODE_FLAGS
{
    MG_NODE_DEFAULT = 0,
    MG_NODE_SINGLETON = HK_BIT(0)
};
HK_FLAG_ENUM_OPERATORS(MG_NODE_FLAGS)
class MGNodeRegistry
{
public:
    struct NodeType
    {
        ClassMeta const* NodeClass{};
        MG_NODE_FLAGS     Flags{};

        operator bool() const
        {
            return NodeClass != nullptr;
        }
    };
    TStringHashMap<NodeType> m_Types;

    void Register(ClassMeta const& NodeClass, StringView Name, MG_NODE_FLAGS Flags = MG_NODE_DEFAULT)
    {
        HK_ASSERT(NodeClass.IsSubclassOf<MGNode>());
        m_Types[Name] = {&NodeClass, Flags};
    }

    NodeType FindType(StringView Name) const
    {
        auto it = m_Types.Find(Name);
        if (it == m_Types.End())
            return {};
        return it->second;
    }

    MGNodeRegistry()
    {
        Register(MGSaturate::GetClassMeta(), "Saturate");
        Register(MGSinus::GetClassMeta(), "Sinus");
        Register(MGCosinus::GetClassMeta(), "Cosinus");
        Register(MGFract::GetClassMeta(), "Fract");
        Register(MGNegate::GetClassMeta(), "Negate");
        Register(MGNormalize::GetClassMeta(), "Normalize");
        Register(MGMul::GetClassMeta(), "Mul");
        Register(MGDiv::GetClassMeta(), "Div");
        Register(MGAdd::GetClassMeta(), "Add");
        Register(MGSub::GetClassMeta(), "Sub");
        Register(MGMAD::GetClassMeta(), "MAD");
        Register(MGStep::GetClassMeta(), "Step");
        Register(MGPow::GetClassMeta(), "Pow");
        Register(MGMod::GetClassMeta(), "Mod");
        Register(MGMin::GetClassMeta(), "Min");
        Register(MGMax::GetClassMeta(), "Max");
        Register(MGLerp::GetClassMeta(), "Lerp");
        Register(MGClamp::GetClassMeta(), "Clamp");
        Register(MGLength::GetClassMeta(), "Length");
        Register(MGDecomposeVector::GetClassMeta(), "DecomposeVector");
        Register(MGMakeVector::GetClassMeta(), "MakeVector");
        Register(MGSpheremapCoord::GetClassMeta(), "SpheremapCoord");
        Register(MGLuminance::GetClassMeta(), "Luminance");
        Register(MGPI::GetClassMeta(), "PI");
        Register(MG2PI::GetClassMeta(), "2PI");
        Register(MGBoolean::GetClassMeta(), "Boolean");
        Register(MGBoolean2::GetClassMeta(), "Boolean2");
        Register(MGBoolean3::GetClassMeta(), "Boolean3");
        Register(MGBoolean4::GetClassMeta(), "Boolean4");
        Register(MGFloat::GetClassMeta(), "Float");
        Register(MGFloat2::GetClassMeta(), "Float2");
        Register(MGFloat3::GetClassMeta(), "Float3");
        Register(MGFloat4::GetClassMeta(), "Float4");
        Register(MGTextureSlot::GetClassMeta(), "TextureSlot");
        Register(MGUniformAddress::GetClassMeta(), "UniformAddress");
        Register(MGTextureLoad::GetClassMeta(), "TextureLoad");
        Register(MGNormalLoad::GetClassMeta(), "NormalLoad");
        Register(MGParallaxMapLoad::GetClassMeta(), "ParallaxMapLoad", MG_NODE_SINGLETON);
        Register(MGVirtualTextureLoad::GetClassMeta(), "VirtualTextureLoad");
        Register(MGVirtualTextureNormalLoad::GetClassMeta(), "VirtualTextureNormalLoad");
        Register(MGInFragmentCoord::GetClassMeta(), "InFragmentCoord", MG_NODE_SINGLETON);
        Register(MGInPosition::GetClassMeta(), "InPosition", MG_NODE_SINGLETON);
        Register(MGInNormal::GetClassMeta(), "InNormal", MG_NODE_SINGLETON);
        Register(MGInColor::GetClassMeta(), "InColor", MG_NODE_SINGLETON);
        Register(MGInTexCoord::GetClassMeta(), "InTexCoord", MG_NODE_SINGLETON);
        Register(MGInTimer::GetClassMeta(), "InTimer", MG_NODE_SINGLETON);
        Register(MGInViewPosition::GetClassMeta(), "InViewPosition", MG_NODE_SINGLETON);
        Register(MGCondLess::GetClassMeta(), "CondLess");
        Register(MGAtmosphere::GetClassMeta(), "Atmosphere");
    }

    TVector<StringView> GetTypes() const
    {
        TVector<StringView> types;
        for (auto& it : m_Types)
        {
            types.Add(it.first);
        }
        std::sort(types.Begin(), types.End(), [](StringView Lhs, StringView Rhs)
                  { return Lhs.Icmp(Rhs); });
        return types;
    }
};

MGNodeRegistry GMaterialNodeRegistry;

MGNode* MGMaterialGraph::Add(StringView Name)
{
    MGNodeRegistry::NodeType nodeType = GMaterialNodeRegistry.FindType(Name);
    if (!nodeType)
    {
        LOG("Unknown node class {}\n", Name);
        return nullptr;
    }

    ClassMeta const* nodeClass = nodeType.NodeClass;

    if (nodeType.Flags & MG_NODE_SINGLETON)
    {
        for (MGNode* node : m_Nodes)
        {
            if (node->FinalClassId() == nodeClass->ClassId)
            {
                return node;
            }
        }
    }

    MGNode* node = static_cast<MGNode*>(nodeClass->CreateInstance());
    m_Nodes.Add(node);
    node->AddRef();
    node->m_ID = ++m_NodeIdGen;
    return node;
}

MGTextureSlot* MGMaterialGraph::GetTexture(uint32_t Slot)
{
    if (Slot >= MAX_MATERIAL_TEXTURES)
    {
        LOG("MGMaterialGraph::GetTexture: MAX_MATERIAL_TEXTURES hit\n");
        return nullptr;
    }

    while (m_TextureSlots.Size() <= Slot)
        m_TextureSlots.Add(nullptr);

    if (!m_TextureSlots[Slot])
    {
        m_TextureSlots[Slot] = NewObj<MGTextureSlot>();
        m_TextureSlots[Slot]->AddRef();
        m_TextureSlots[Slot]->m_ID        = ++m_NodeIdGen;
        m_TextureSlots[Slot]->m_SlotIndex = Slot;

        m_Nodes.Add(m_TextureSlots[Slot]);
    }

    return m_TextureSlots[Slot];
}

MGMaterialGraph* MGMaterialGraph::LoadFromFile(IBinaryStreamReadInterface& Stream)
{
    if (!Stream.IsValid())
        return nullptr;

    String documentData = Stream.AsString();

    DocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = documentData.CStr();
    deserializeInfo.bInsitu       = true;

    Document document;
    document.DeserializeFromString(deserializeInfo);

    if (document.GetInt32("version") != 1)
    {
        LOG("MGMaterialGraph::LoadFromFile: unknown version\n");
        return nullptr;
    }

    MGMaterialGraph* graph = NewObj<MGMaterialGraph>();

   
    auto mTextureSlots = document.FindMember("textures");

    TStringHashMap<uint32_t> textureSlots;

    if (mTextureSlots)
    {
        uint32_t slot = 0;
        for (auto object = mTextureSlots->GetArrayValues(); object; object = object->GetNext())
        {
            auto* textureSlot = graph->GetTexture(slot);

            textureSlot->ParseProperties(object);

            slot++;

            StringView id = object->GetString("id");
            if (id)
            {
                if (textureSlots.Contains(id))
                {
                    LOG("Texture redefinition {}\n", id);
                    continue;
                }

                textureSlots[id] = slot - 1;
            }
        }
    }

    struct NodeInfo
    {
        DocumentValue const* Object;
        MGNode*          Node;
    };

    TStringHashMap<NodeInfo> nodes;

    nodes["__root__"].Node   = graph;
    nodes["__root__"].Object = &document;

    auto mNodes = document.FindMember("nodes");
    for (auto object = mNodes->GetArrayValues(); object; object = object->GetNext())
    {
        auto id = object->GetString("id");
        if (!id)
        {
            LOG("Invalid node id\n");
            continue;
        }

        if (nodes.Contains(id))
        {
            LOG("Node with id {} already exists\n", id);
            continue;
        }

        StringView nodeType = object->GetString("type");

        MGNode* node = graph->Add(nodeType);
        if (!node)
        {
            LOG("Unknown node type {}\n", nodeType);
            continue;
        }

        NodeInfo& info = nodes[id];

        info.Object = object;
        info.Node   = node;        
    }

    TVector<StringView> vector;
    vector.Reserve(4);

    // Resolve connections
    for (auto& it : nodes)
    {
        NodeInfo& info = it.second;
        MGNode*   node = info.Node;

        node->ParseProperties(info.Object);
        
        TVector<MGInput*> const& inputs = node->GetInputs();

        for (MGInput* input : inputs)
        {
            StringView connection = info.Object->GetString("$" + input->GetName());

            if (!connection)
            {
                // undefined
                continue;
            }

            // Parse explicit number
            if (connection[0] == '=')
            {
                StringView number = connection.GetSubstring(1);

                if (ParseVector(number, vector) && !vector.IsEmpty() && vector.Size() <= 4)
                {
                    bool bIsBoolean = vector[0].Compare("true") || vector[0].Compare("false");
                    int  n          = vector.Size();
                    Float4 v;

                    for (int i = 0; i < n; i++)
                        v[i] = Core::ParseFloat(vector[i]);

                    switch (n)
                    {
                        case 1:
                            if (bIsBoolean)
                                node->BindInput(input->GetName(), graph->Add2<MGBoolean>(v[0]));
                            else
                                node->BindInput(input->GetName(), graph->Add2<MGFloat>(v[0]));
                            break;
                        case 2:
                            if (bIsBoolean)
                                node->BindInput(input->GetName(), graph->Add2<MGBoolean2>(Bool2(v[0], v[1])));
                            else
                                node->BindInput(input->GetName(), graph->Add2<MGFloat2>(Float2(v)));
                            break;
                        case 3:
                            if (bIsBoolean)
                                node->BindInput(input->GetName(), graph->Add2<MGBoolean3>(Bool3(v[0], v[1], v[2])));
                            else
                                node->BindInput(input->GetName(), graph->Add2<MGFloat3>(Float3(v)));
                            break;
                        case 4:
                            if (bIsBoolean)
                                node->BindInput(input->GetName(), graph->Add2<MGBoolean4>(Bool4(v[0], v[1], v[2], v[3])));
                            else
                                node->BindInput(input->GetName(), graph->Add2<MGFloat4>(v));
                            break;
                    }
                }
                else
                    LOG("Invalid value {}\n", number);
                continue;
            }

            // Parse node_name.output
            StringSizeType n = connection.FindCharacter('.');
            if (n != -1)
            {
                StringView connectedNode = connection.TruncateTail(n);
                StringView output        = connection.TruncateHead(n + 1);

                auto nit = nodes.Find(connectedNode);
                if (nit == nodes.End())
                {
                    LOG("Node {} not found\n", connectedNode);
                    continue;
                }

                MGOutput* out = nit->second.Node->FindOutput(output);
                if (!out)
                {
                    LOG("Node {} doesn't contain {} output\n", connectedNode, output);
                    continue;
                }

                node->BindInput(input->GetName(), out);
                continue;
            }

            // Parse node_name with default output
            auto nit = nodes.Find(connection);
            if (nit != nodes.End())
            {
                node->BindInput(input->GetName(), nit->second.Node);
                continue;
            }

            // Parse texture slot
            auto texit = textureSlots.Find(connection);
            if (texit != textureSlots.End())
            {
                node->BindInput(input->GetName(), graph->GetTexture(texit->second));
                continue;
            }

            LOG("Node {} not found\n", connection);
        }
    }

    return graph;
}
