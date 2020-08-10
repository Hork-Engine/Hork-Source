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

#include <World/Public/MaterialGraph/MaterialGraph.h>
#include <Core/Public/Logger.h>
#include <Core/Public/CriticalError.h>

enum EMaterialStage
{
    VERTEX_STAGE,
    TESSELLATION_CONTROL_STAGE,
    TESSELLATION_EVAL_STAGE,
    GEOMETRY_STAGE,
    FRAGMENT_STAGE,
    SHADOWCAST_STAGE    
};

struct SVarying
{
    SVarying( const char * Name, const char * Source, EMGNodeType Type )
        : VaryingName( Name )
        , VaryingSource( Source )
        , VaryingType( Type )
        , RefCount( 0 )
    {
    }
    AString VaryingName;
    AString VaryingSource;
    EMGNodeType VaryingType;
    int RefCount;
};

class AMaterialBuildContext
{
public:
    mutable AString SourceCode;
    AString OutputVaryingsCode;
    AString InputVaryingsCode;
    AString CopyVaryingsCode;
    bool bHasTextures;
    int MaxTextureSlot;
    int MaxUniformAddress;
    bool bParallaxSampler;
    bool bHasVertexDeform;
    bool bHasDisplacement;
    TStdVector< SVarying > InputVaryings;
    int Serial;

    AMaterialBuildContext( MGMaterialGraph const * _Graph, EMaterialStage _Stage ) {
        Graph = _Graph;
        VariableName = 0;
        Stage = _Stage;
        bHasTextures = false;
        MaxTextureSlot = -1;
        MaxUniformAddress = -1;
        bParallaxSampler = false;
        bHasVertexDeform = false;
        bHasDisplacement = false;
    }

    int GetBuildSerial() const { return Serial; }

    AString GenerateVariableName() const;
    void GenerateSourceCode( MGOutput * _Slot, AString const & _Expression, bool _AddBrackets );

    EMaterialStage GetStage() const { return Stage; }

    EMaterialType GetMaterialType() const { return Graph->MaterialType; }

    MGMaterialGraph const * GetGraph() { return Graph; }

private:
    mutable int VariableName = 0;
    EMaterialStage Stage;
    MGMaterialGraph const * Graph;
};

static constexpr const char * VariableTypeStr[] = {
    "vec4",     // AT_Unknown
    "float",    // AT_Float1
    "vec2",     // AT_Float2
    "vec3",     // AT_Float3
    "vec4",     // AT_Float4
    "bool",     // AT_Bool1
    "bvec2",    // AT_Bool2
    "bvec3",    // AT_Bool3
    "bvec4",    // AT_Bool4
};

enum EMGVectorType
{
    VEC_UNKNOWN,
    VEC1,
    VEC2,
    VEC3,
    VEC4
};

enum EMGComponentType
{
    COMP_UNKNOWN,
    COMP_FLOAT,
    COMP_BOOL
};

static bool IsTypeComponent( EMGNodeType InType )
{
    switch ( InType ) {
    case AT_Float1:
    case AT_Bool1:
        return true;
    default:
        break;
    }
    return false;
}

static bool IsTypeVector( EMGNodeType InType )
{
    return !IsTypeComponent( InType );
}

static EMGComponentType GetTypeComponent( EMGNodeType InType )
{
    switch ( InType ) {
    case AT_Float1:
    case AT_Float2:
    case AT_Float3:
    case AT_Float4:
        return COMP_FLOAT;
    case AT_Bool1:
    case AT_Bool2:
    case AT_Bool3:
    case AT_Bool4:
        return COMP_BOOL;
    default:
        break;
    }
    return COMP_UNKNOWN;
}

static EMGVectorType GetTypeVector( EMGNodeType InType )
{
    switch ( InType ) {
    case AT_Float1:
    case AT_Bool1:
        return VEC1;
    case AT_Float2:
    case AT_Bool2:
        return VEC2;
    case AT_Float3:
    case AT_Bool3:
        return VEC3;
    case AT_Float4:
    case AT_Bool4:
        return VEC4;
    default:
        break;
    }
    return VEC_UNKNOWN;
}

static bool IsArithmeticType( EMGNodeType InType )
{
    switch ( GetTypeComponent( InType ) ) {
    case COMP_FLOAT:
        return true;
    default:
        break;
    }

    return false;
}

static EMGNodeType ToFloatType( EMGNodeType InType )
{
    switch ( InType ) {
    case AT_Float1:
    case AT_Float2:
    case AT_Float3:
    case AT_Float4:
        return InType;
    case AT_Bool1:
        return AT_Float1;
    case AT_Bool2:
        return AT_Float2;
    case AT_Bool3:
        return AT_Float3;
    case AT_Bool4:
        return AT_Float4;
    default:
        break;
    }
    return AT_Float1;
}

enum EVectorCastFlags
{
    VECTOR_CAST_IDENTITY_X = AN_BIT( 0 ),
    VECTOR_CAST_IDENTITY_Y = AN_BIT( 1 ),
    VECTOR_CAST_IDENTITY_Z = AN_BIT( 2 ),
    VECTOR_CAST_IDENTITY_W = AN_BIT( 3 ),
    VECTOR_CAST_EXPAND_VEC1 = AN_BIT( 4 ),
};

static AString MakeVectorCast( AString const & _Expression, EMGNodeType _TypeFrom, EMGNodeType _TypeTo,
                               int VectorCastFlags = 0 )
{
    if ( _TypeFrom == _TypeTo || _TypeTo == AT_Unknown ) {
        return _Expression;
    }

    EMGComponentType componentFrom = GetTypeComponent( _TypeFrom );
    EMGComponentType componentTo = GetTypeComponent( _TypeTo );

    bool bSameComponentType = componentFrom == componentTo;

    const char * zero = "0";
    const char * one = "1";
    switch ( componentTo ) {
    case COMP_FLOAT:
        zero = "0.0";
        one = "1.0";
        break;
    case COMP_BOOL:
        zero = "false";
        one = "true";
        break;
    default:
        AN_ASSERT( 0 );
    }

    AString defX = VectorCastFlags & VECTOR_CAST_IDENTITY_X ? one : zero;
    AString defY = VectorCastFlags & VECTOR_CAST_IDENTITY_Y ? one : zero;
    AString defZ = VectorCastFlags & VECTOR_CAST_IDENTITY_Z ? one : zero;
    AString defW = VectorCastFlags & VECTOR_CAST_IDENTITY_W ? one : zero;

    EMGVectorType vecType = GetTypeVector( _TypeFrom );

    switch( vecType ) {
    case VEC_UNKNOWN:
        switch ( _TypeTo ) {
        case AT_Float1:
            return defX;
        case AT_Float2:
            return "vec2( " + defX + ", " + defY + " )";
        case AT_Float3:
            return "vec3( " + defX + ", " + defY + ", " + defZ + " )";
        case AT_Float4:
            return "vec4( " + defX + ", " + defY + ", " + defZ + ", " + defW + " )";
        case AT_Bool1:
            return defX;
        case AT_Bool2:
            return "bvec2( " + defX + ", " + defY + " )";
        case AT_Bool3:
            return "bvec3( " + defX + ", " + defY + ", " + defZ + " )";
        case AT_Bool4:
            return "bvec4( " + defX + ", " + defY + ", " + defZ + ", " + defW + " )";
        default:
            break;
        }
        break;
    case VEC1:
        if ( VectorCastFlags & VECTOR_CAST_EXPAND_VEC1 )
        {
            // Conversion like: vecN( in, in, ... )
            switch ( _TypeTo ) {
            case AT_Float1:
                return bSameComponentType
                    ? _Expression
                    : "float( " + _Expression + " )";
            case AT_Float2:
                return bSameComponentType
                    ? "vec2( " + _Expression + " )"
                    : "vec2( float(" + _Expression + ") )";
            case AT_Float3:
                return bSameComponentType
                    ? "vec3( " + _Expression + " )"
                    : "vec3( float(" + _Expression + ") )";
            case AT_Float4:
                return bSameComponentType
                    ? "vec4( " + _Expression + " )"
                    : "vec4( float(" + _Expression + ") )";
            case AT_Bool1:
                return bSameComponentType
                    ? _Expression
                    : "bool(" + _Expression + ")";
            case AT_Bool2:
                return bSameComponentType
                    ? "bvec2( " + _Expression + " )"
                    : "bvec2( bool(" + _Expression + ") )";
            case AT_Bool3:
                return bSameComponentType
                    ? "bvec3( " + _Expression + " )"
                    : "bvec3( bool(" + _Expression + ") )";
            case AT_Bool4:
                return bSameComponentType
                    ? "bvec4( " + _Expression + " )"
                    : "bvec4( bool(" + _Expression + ") )";
            default:
                break;
            }
            break;
        }
        else
        {
            // Conversion like: vecN( in, defY, defZ, defW )
            switch ( _TypeTo ) {
            case AT_Float1:
                return bSameComponentType
                    ? _Expression
                    : "float( " + _Expression + " )";
            case AT_Float2:
                return bSameComponentType
                    ? "vec2( " + _Expression + ", " + defY + " )"
                    : "vec2( float(" + _Expression + "), " + defY + " )";
            case AT_Float3:
                return bSameComponentType
                    ? "vec3( " + _Expression + ", " + defY +", " + defZ + " )"
                    : "vec3( float(" + _Expression + "), " + defY +", " + defZ + " )";
            case AT_Float4:
                return bSameComponentType
                    ? "vec4( " + _Expression + ", " + defY +", " + defZ + ", " + defW + " )"
                    : "vec4( float(" + _Expression + "), " + defY +", " + defZ + ", " + defW + " )";
            case AT_Bool1:
                return bSameComponentType
                    ? _Expression
                    : "bool( " + _Expression + " )";
            case AT_Bool2:
                return bSameComponentType
                    ? "bvec2( " + _Expression + ", " + defY + " )"
                    : "bvec2( bool(" + _Expression + "), " + defY + " )";
            case AT_Bool3:
                return bSameComponentType
                    ? "bvec3( " + _Expression + ", " + defY +", " + defZ + " )"
                    : "bvec3( bool(" + _Expression + "), " + defY +", " + defZ + " )";
            case AT_Bool4:
                return bSameComponentType
                    ? "bvec4( " + _Expression + ", " + defY +", " + defZ + ", " + defW + " )"
                    : "bvec4( bool(" + _Expression + "), " + defY +", " + defZ + ", " + defW + " )";
            default:
                break;
            }
            break;
        }
    case VEC2:
        switch ( _TypeTo ) {
        case AT_Float1:
            return bSameComponentType
                ? _Expression + ".x"
                : "float( " + _Expression + ".x )";
        case AT_Float2:
            return bSameComponentType
                ? _Expression
                : "vec2( " + _Expression + " )";
        case AT_Float3:
            return bSameComponentType
                ? "vec3( " + _Expression + ", " + defZ + " )"
                : "vec3( vec2(" + _Expression + "), " + defZ + " )";
        case AT_Float4:
            return bSameComponentType
                ? "vec4( " + _Expression + ", " + defZ + ", " + defW + " )"
                : "vec4( vec2(" + _Expression + "), " + defZ + ", " + defW + " )";
        case AT_Bool1:
            return bSameComponentType
                ? _Expression + ".x"
                : "bool(" + _Expression + ".x )";
        case AT_Bool2:
            return bSameComponentType
                ? _Expression
                : "bvec2( " + _Expression + " )";
        case AT_Bool3:
            return bSameComponentType
                ? "bvec3( " + _Expression + ", " + defZ + " )"
                : "bvec3( bvec2(" + _Expression + "), " + defZ + " )";
        case AT_Bool4:
            return bSameComponentType
                ? "bvec4( " + _Expression + ", " + defZ + ", " + defW + " )"
                : "bvec4( bvec2(" + _Expression + "), " + defZ + ", " + defW + " )";
        default:
            break;
        }
        break;
    case VEC3:
        switch ( _TypeTo ) {
        case AT_Float1:
            return bSameComponentType
                ? _Expression + ".x"
                : "float( " + _Expression + ".x )";
        case AT_Float2:
            return bSameComponentType
                ? _Expression + ".xy"
                : "vec2( " + _Expression + ".xy )";
        case AT_Float3:
            return bSameComponentType
                ? _Expression
                : "vec3( " + _Expression + " )";
        case AT_Float4:
            return bSameComponentType
                ? "vec4( " + _Expression + ", " + defW + " )"
                : "vec4( vec3(" + _Expression + "), " + defW + " )";
        case AT_Bool1:
            return bSameComponentType
                ? _Expression + ".x"
                : "bool(" + _Expression + ".x)";
        case AT_Bool2:
            return bSameComponentType
                ? _Expression + ".xy"
                : "bvec2(" + _Expression + ".xy)";
        case AT_Bool3:
            return bSameComponentType
                ? _Expression
                : "bvec3( " + _Expression + " )";
        case AT_Bool4:
            return bSameComponentType
                ? "bvec4( " + _Expression + ", " + defW + " )"
                : "bvec4( bvec3(" + _Expression + "), " + defW + " )";
        default:
            break;
        }
        break;
    case VEC4:
        switch ( _TypeTo ) {
        case AT_Float1:
            return bSameComponentType
                ? _Expression + ".x"
                : "float( " + _Expression + ".x )";
        case AT_Float2:
            return bSameComponentType
                ? _Expression + ".xy"
                : "vec2( " + _Expression + ".xy )";
        case AT_Float3:
            return bSameComponentType
                ? _Expression + ".xyz"
                : "vec3( " + _Expression + ".xyz )";
        case AT_Float4:
            return bSameComponentType
                ? _Expression
                : "vec4( " + _Expression + " )";
        case AT_Bool1:
            return bSameComponentType
                ? _Expression + ".x"
                : "bool(" + _Expression + ".x )";
        case AT_Bool2:
            return bSameComponentType
                ? _Expression + ".xy"
                : "bvec2(" + _Expression + ".xy )";
        case AT_Bool3:
            return bSameComponentType
                ? _Expression + ".xyz"
                : "bvec3(" + _Expression + ".xyz )";
        case AT_Bool4:
            return bSameComponentType
                ? _Expression
                : "bvec4(" + _Expression + ")";
        default:
            break;
        }
        break;
    }

    AN_ASSERT(0);

    return _Expression;
}

static const char * MakeEmptyVector( EMGNodeType InType )
{
    switch ( InType ) {
    case AT_Float1:
        return "0.0";
    case AT_Float2:
        return "vec2( 0.0 )";
    case AT_Float3:
        return "vec3( 0.0 )";
    case AT_Float4:
        return "vec4( 0.0 )";
    case AT_Bool1:
        return "false";
    case AT_Bool2:
        return "bvec2( false )";
    case AT_Bool3:
        return "bvec3( false )";
    case AT_Bool4:
        return "bvec4( false )";
    default:
        break;
    };

    return "0.0";
}

static const char * MakeDefaultNormal() {
    return "vec3( 0.0, 0.0, 1.0 )";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AString AMaterialBuildContext::GenerateVariableName() const {
    return AString( "v" ) + Math::ToString( VariableName++ );
}

void AMaterialBuildContext::GenerateSourceCode( MGOutput * _Slot, AString const & _Expression, bool _AddBrackets ) {
    if ( _Slot->Usages > 1 ) {
        _Slot->Expression = GenerateVariableName();
        SourceCode += "const " + AString( VariableTypeStr[ _Slot->Type ] ) + " " + _Slot->Expression + " = " + _Expression + ";\n";
    } else {
        if ( _AddBrackets ) {
            _Slot->Expression = "( " + _Expression + " )";
        } else {
            _Slot->Expression = _Expression;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGOutput )

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInput )

MGInput::MGInput() {
}

void MGInput::Connect( MGNode * pNode, const char * SlotName ) {
    if ( pNode ) {
        Slot = pNode->FindOutput( SlotName );
    } else {
        Slot.Reset();
    }
}

void MGInput::Connect( MGOutput * pSlot ) {
    Slot = pSlot;
}

void MGInput::Disconnect() {
    Slot.Reset();
}

MGOutput * MGInput::GetConnection() {
    return Slot;
}

int MGInput::Serialize( ADocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( GetObjectName() ).CStr() );

    //if ( GetOwner() ) {
    //    _Doc.AddStringField( object, "Slot", _Doc.ProxyBuffer.NewString( Slot->GetObjectName() ).CStr() );
    //    _Doc.AddStringField( object, "Node", _Doc.ProxyBuffer.NewString( Math::ToString( GetOwner()->GetId() ) ).CStr() );
    //}

    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGNode )
AN_ATTRIBUTE_( Location, AF_DEFAULT )
AN_END_CLASS_META()

MGNode::MGNode( const char * _Name ) {
    ID = 0;
    Serial = 0;
    bTouched = false;
    bSingleton = false;

    SetObjectName( _Name );
}

MGNode::~MGNode() {
    for ( MGInput * In : Inputs ) {
        In->RemoveRef();
    }

    for ( MGOutput * Out : Outputs ) {
        Out->Owner = nullptr;
        Out->RemoveRef();
    }
}

MGInput * MGNode::AddInput( const char * _Name ) {
    MGInput * In = NewObject< MGInput >();
    In->AddRef();
    In->SetObjectName( _Name );
    Inputs.Append( In );
    return In;
}

MGOutput * MGNode::AddOutput( const char * _Name, EMGNodeType _Type ) {
    MGOutput * Out = NewObject< MGOutput >();
    Out->AddRef();
    Out->SetObjectName( _Name );
    Out->Type = _Type;
    Out->Owner = this;
    Outputs.Append( Out );
    return Out;
}

MGOutput * MGNode::FindOutput( const char * _Name ) {
    for ( MGOutput * Out : Outputs ) {
        if ( !Out->GetObjectName().Cmp( _Name ) ) {
            return Out;
        }
    }
    return nullptr;
}

bool MGNode::Build( AMaterialBuildContext & _Context ) {
    if ( Serial == _Context.GetBuildSerial() ) {
        return true;
    }

    //if ( !(Stages & _Context.GetStageMask() ) ) {
    //    return false;
    //}

    Serial = _Context.GetBuildSerial();

    Compute( _Context );
    return true;
}

void MGNode::ResetConnections( AMaterialBuildContext const & _Context ) {
    if ( !bTouched ) {
        return;
    }

    bTouched = false;

    for ( MGInput * in : Inputs ) {
        MGOutput * out = in->GetConnection();
        if ( out ) {
            MGNode * node = in->ConnectedNode();
            node->ResetConnections( _Context );
            out->Usages = 0;
        }
    }
}

void MGNode::TouchConnections( AMaterialBuildContext const & _Context ) {
    if ( bTouched ) {
        return;
    }

    bTouched = true;

    for ( MGInput * in : Inputs ) {
        MGOutput * out = in->GetConnection();
        if ( out ) {
            MGNode * node = in->ConnectedNode();
            node->TouchConnections( _Context );
            out->Usages++;
        }
    }
}

int MGNode::Serialize( ADocument & _Doc ) {
#if 0
    int object = _Doc.CreateObjectValue();
    _Doc.AddStringField( object, "ClassName", FinalClassName() );
#else
    int object = Super::Serialize(_Doc);
#endif

    _Doc.AddStringField( object, "ID", _Doc.ProxyBuffer.NewString( Math::ToString( ID ) ).CStr() );

    if ( !Inputs.IsEmpty() ) {
        int array = _Doc.AddArray( object, "Inputs" );
        for ( MGInput * in : Inputs ) {
            int inputObject = in->Serialize( _Doc );
            _Doc.AddValueToField( array, inputObject );
        }
    }
    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static AString MakeExpression( AMaterialBuildContext & _Context, MGInput * Input, EMGNodeType DesiredType, AString const & DefaultExpression, int VectorCastFlags = 0 )
{
    MGOutput * connection = Input->GetConnection();

    if ( connection && Input->ConnectedNode()->Build( _Context ) ) {
        return MakeVectorCast( connection->Expression, connection->Type, DesiredType, VectorCastFlags );
    }

    return DefaultExpression;//MakeEmptyVector( DesiredType );
}

void MGMaterialGraph::Compute( AMaterialBuildContext & _Context )
{
    Super::Compute( _Context );

    switch ( _Context.GetStage() ) {
    case VERTEX_STAGE:
        ComputeVertexStage( _Context );
        break;
    case TESSELLATION_CONTROL_STAGE:
        ComputeTessellationControlStage( _Context );
        break;
    case TESSELLATION_EVAL_STAGE:
        ComputeTessellationEvalStage( _Context );
        break;
    case GEOMETRY_STAGE:
        break;
    case FRAGMENT_STAGE:
        ComputeFragmentStage( _Context );
        break;
    case SHADOWCAST_STAGE:
        ComputeShadowCastStage( _Context );
        break;

    }        
}

void MGMaterialGraph::ComputeVertexStage( AMaterialBuildContext & _Context )
{
    MGOutput * positionCon = VertexDeform->GetConnection();

    _Context.bHasVertexDeform = false;

    //AString TransformMatrix;
    //if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
    //    TransformMatrix = "OrthoProjection";
    //} else {
    //    TransformMatrix = "TransformMatrix";
    //}

    if ( positionCon && VertexDeform->ConnectedNode()->Build( _Context ) ) {

        if ( positionCon->Expression != "VertexPosition" ) {
            _Context.bHasVertexDeform = true;
        }

        AString expression = MakeVectorCast( positionCon->Expression, positionCon->Type, AT_Float4, VECTOR_CAST_IDENTITY_W );

        _Context.SourceCode += "vec4 FinalVertexPos = " + expression + ";\n";

    } else {
        _Context.SourceCode += "vec4 FinalVertexPos = vec4( VertexPosition, 1.0 );\n";
    }
}

void MGMaterialGraph::ComputeFragmentStage( AMaterialBuildContext & _Context )
{
    AString expression;

    // Color
    expression = MakeExpression( _Context, Color, AT_Float4, MakeEmptyVector( AT_Float4 ), VECTOR_CAST_EXPAND_VEC1 );
    _Context.SourceCode += "vec4 BaseColor = " + expression + ";\n";

    if ( _Context.GetMaterialType() == MATERIAL_TYPE_PBR || _Context.GetMaterialType() == MATERIAL_TYPE_BASELIGHT ) {
        // Normal
        expression = MakeExpression( _Context, Normal, AT_Float3, MakeDefaultNormal() );
        _Context.SourceCode += "vec3 MaterialNormal = " + expression + ";\n";

        // Emissive
        expression = MakeExpression( _Context, Emissive, AT_Float3, MakeEmptyVector( AT_Float3 ), VECTOR_CAST_EXPAND_VEC1 );
        _Context.SourceCode += "vec3 MaterialEmissive = " + expression + ";\n";

        // Specular
        expression = MakeExpression( _Context, Specular, AT_Float3, MakeEmptyVector( AT_Float3 ), VECTOR_CAST_EXPAND_VEC1 );
        _Context.SourceCode += "vec3 MaterialSpecular = " + expression + ";\n";

        // Ambient Light
        expression = MakeExpression( _Context, AmbientLight, AT_Float3, MakeEmptyVector( AT_Float3 ), VECTOR_CAST_EXPAND_VEC1 );
        _Context.SourceCode += "vec3 MaterialAmbientLight = " + expression + ";\n";
    }

    if ( _Context.GetMaterialType() == MATERIAL_TYPE_PBR ) {
        // Metallic
        expression = MakeExpression( _Context, Metallic, AT_Float1, MakeEmptyVector( AT_Float1 ) );
        _Context.SourceCode += "float MaterialMetallic = " + expression + ";\n";

        // Roughness
        expression = MakeExpression( _Context, Roughness, AT_Float1, "1.0" );
        _Context.SourceCode += "float MaterialRoughness = " + expression + ";\n";

        // Ambient Occlusion
        expression = MakeExpression( _Context, AmbientOcclusion, AT_Float1, "1.0" );
        _Context.SourceCode += "float MaterialAmbientOcclusion = " + expression + ";\n";
    }

    // Opacity
    if ( _Context.GetGraph()->bTranslucent ) {
        expression = MakeExpression( _Context, Opacity, AT_Float1, "1.0" );
        _Context.SourceCode += "float Opacity = " + expression + ";\n";
    } else {
        _Context.SourceCode += "const float Opacity = 1.0;\n";
    }
}

void MGMaterialGraph::ComputeShadowCastStage( AMaterialBuildContext & _Context )
{
#if 0
    MGOutput * positionCon = VertexDeform->GetConnection();

    _Context.bHasVertexDeform = false;

    //AString TransformMatrix;
    //if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
    //    TransformMatrix = "OrthoProjection";
    //} else {
    //    TransformMatrix = "TransformMatrix";
    //}

    if ( positionCon && VertexDeform->ConnectedNode()->Build( _Context ) ) {

        if ( positionCon->Expression != "VertexPosition" ) {
            _Context.bHasVertexDeform = true;
        }

        AString expression = MakeVectorCast( positionCon->Expression, positionCon->Type, AT_Float4, VECTOR_CAST_IDENTITY_W );

        _Context.SourceCode += "vec4 FinalVertexPos = " + expression + ";\n";

    } else {
        _Context.SourceCode += "vec4 FinalVertexPos = vec4( VertexPosition, 1.0 );\n";
    }
#endif

    // Shadow Mask

    MGOutput * con = ShadowMask->GetConnection();

    if ( con && ShadowMask->ConnectedNode()->Build( _Context ) ) {

        switch ( con->Type ) {
        case AT_Float1:
            _Context.SourceCode += "if ( " + con->Expression + " <= 0.0 ) discard;\n";
            break;
        case AT_Float2:
        case AT_Float3:
        case AT_Float4:
            _Context.SourceCode += "if ( " + con->Expression + ".x <= 0.0 ) discard;\n";
            break;
        case AT_Bool1:
            _Context.SourceCode += "if ( " + con->Expression + " == false ) discard;\n";
            break;
        case AT_Bool2:
        case AT_Bool3:
        case AT_Bool4:
            _Context.SourceCode += "if ( " + con->Expression + ".x == false ) discard;\n";
            break;
        default:
            break;
        }
    }
}

void MGMaterialGraph::ComputeTessellationControlStage( AMaterialBuildContext & _Context )
{
    MGOutput * tessFactorCon = TessellationFactor->GetConnection();

    if ( tessFactorCon && TessellationFactor->ConnectedNode()->Build( _Context ) ) {
        AString expression = MakeVectorCast( tessFactorCon->Expression, tessFactorCon->Type, AT_Float1 );

        _Context.SourceCode += "float TessellationFactor = " + expression + ";\n";

    } else {
        _Context.SourceCode += "const float TessellationFactor = 1.0;\n";
    }
}

void MGMaterialGraph::ComputeTessellationEvalStage( AMaterialBuildContext & _Context )
{
    MGOutput * displacementCon = Displacement->GetConnection();

    _Context.bHasDisplacement = false;

    if ( displacementCon && Displacement->ConnectedNode()->Build( _Context ) ) {

        _Context.bHasDisplacement = true;

        AString expression = MakeVectorCast( displacementCon->Expression, displacementCon->Type, AT_Float1 );

        _Context.SourceCode += "float Displacement = " + expression + ";\n";

    } else {
        _Context.SourceCode += "const float Displacement = 0.0;\n";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
AN_CLASS_META( MGProjectionNode )

MGProjectionNode::MGProjectionNode() : Super( "Projection" ) {
    Vector = AddInput( "Vector" );
    Result = AddOutput( "Result", AT_Float4 );
}

void MGProjectionNode::Compute( AMaterialBuildContext & _Context ) {
    AString expression = "TransformMatrix * " + MakeExpression( _Context, Vector, AT_Float4, MakeEmptyVector( AT_Float4 ), VECTOR_CAST_IDENTITY_W );

    _Context.GenerateSourceCode( Result, expression, true );
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGLengthNode )

MGLengthNode::MGLengthNode() : Super( "Length" ) {
    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Float1 );
}

void MGLengthNode::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedNode()->Build( _Context ) ) {

        EMGNodeType type = ToFloatType( inputConnection->Type );

        AString expression = MakeVectorCast( inputConnection->Expression, inputConnection->Type, type );

        if ( type == AT_Float1 ) {
            _Context.GenerateSourceCode( Result, expression, false );
        } else {
            _Context.GenerateSourceCode( Result, "length( " + expression + " )", false );
        }
    } else {
        Result->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGDecomposeVectorNode )

MGDecomposeVectorNode::MGDecomposeVectorNode() : Super( "Decompose Vector" ) {
    Vector = AddInput( "Vector" );
    X = AddOutput( "X", AT_Unknown );
    Y = AddOutput( "Y", AT_Unknown );
    Z = AddOutput( "Z", AT_Unknown );
    W = AddOutput( "W", AT_Unknown );
}

void MGDecomposeVectorNode::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * inputConnection = Vector->GetConnection();

    if ( inputConnection && Vector->ConnectedNode()->Build( _Context ) ) {

        EMGComponentType componentType = GetTypeComponent( inputConnection->Type );

        const char * zero = "0";

        switch ( componentType ) {
        case COMP_FLOAT:
            X->Type = Y->Type = Z->Type = W->Type = AT_Float1;
            zero = "0.0";
            break;
        case COMP_BOOL:
            X->Type = Y->Type = Z->Type = W->Type = AT_Bool1;
            zero = "false";
            break;
        default:
            AN_ASSERT( 0 );
            break;
        }

        switch ( GetTypeVector( inputConnection->Type ) ) {
        case VEC1:
            {
            _Context.GenerateSourceCode( X, inputConnection->Expression, false );
            Y->Expression = zero;
            Z->Expression = zero;
            W->Expression = zero;
            break;
            }
        case VEC2:
            {
            AString temp = "temp_" + _Context.GenerateVariableName();
            _Context.SourceCode += "const " + AString( VariableTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
            X->Expression = temp + ".x";
            Y->Expression = temp + ".y";
            Z->Expression = zero;
            W->Expression = zero;
            }
            break;
        case VEC3:
            {
            AString temp = "temp_" + _Context.GenerateVariableName();
            _Context.SourceCode += "const " + AString( VariableTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
            X->Expression = temp + ".x";
            Y->Expression = temp + ".y";
            Z->Expression = temp + ".z";
            W->Expression = zero;
            }
            break;
        case VEC4:
            {
            AString temp = "temp_" + _Context.GenerateVariableName();
            _Context.SourceCode += "const " + AString( VariableTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
            X->Expression = temp + ".x";
            Y->Expression = temp + ".y";
            Z->Expression = temp + ".z";
            W->Expression = temp + ".w";
            }
            break;
        default:
            AN_ASSERT( 0 );
            break;
        }

    } else {
        X->Type = Y->Type = Z->Type = W->Type = AT_Float1;
        X->Expression = "0.0";
        Y->Expression = "0.0";
        Z->Expression = "0.0";
        W->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMakeVectorNode )

MGMakeVectorNode::MGMakeVectorNode() : Super( "Make Vector" ) {
    X = AddInput( "X" );
    Y = AddInput( "Y" );
    Z = AddInput( "Z" );
    W = AddInput( "W" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGMakeVectorNode::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * XConnection = X->GetConnection();
    MGOutput * YConnection = Y->GetConnection();
    MGOutput * ZConnection = Z->GetConnection();
    MGOutput * WConnection = W->GetConnection();

    bool XValid = XConnection && X->ConnectedNode()->Build( _Context ) && IsTypeComponent( XConnection->Type );
    bool YValid = YConnection && Y->ConnectedNode()->Build( _Context ) && IsTypeComponent( YConnection->Type );
    bool ZValid = ZConnection && Z->ConnectedNode()->Build( _Context ) && IsTypeComponent( ZConnection->Type );
    bool WValid = WConnection && W->ConnectedNode()->Build( _Context ) && IsTypeComponent( WConnection->Type );

    int numComponents = 4;
    if ( !WValid ) {
        numComponents--;
        if ( !ZValid ) {
            numComponents--;
            if ( !YValid ) {
                numComponents--;
                if ( !XValid ) {
                    numComponents--;
                }
            }
        }
    }

    if ( numComponents == 0 ) {
        Result->Type = AT_Float1;
        Result->Expression = "0.0";
        return;
    }

    if ( numComponents == 1 ) {
        Result->Type = XConnection->Type;
        _Context.GenerateSourceCode( Result, XConnection->Expression, false );
        return;
    }

    // Result type is depends on first valid component type
    EMGNodeType resultType;
    if ( XValid ) {
        resultType = XConnection->Type;
    } else if ( YValid ) {
        resultType = YConnection->Type;
    } else if ( ZValid ) {
        resultType = ZConnection->Type;
    } else if ( WValid ) {
        resultType = WConnection->Type;
    } else {
        resultType = AT_Float1;
        AN_ASSERT( 0 );
    }
    Result->Type = EMGNodeType( resultType + numComponents - 1 );

    AString resultTypeStr;
    const char * defaultVal = "0";
    switch ( resultType ) {
    case AT_Float1:
        resultTypeStr = "float";
        defaultVal = "0.0";
        break;
    case AT_Bool1:
        resultTypeStr = "bool";
        defaultVal = "false";
        break;
    default:
        AN_ASSERT( 0 );
    }

    AString typeCastX = XValid ?
        ( ( resultType == XConnection->Type ) ? XConnection->Expression : ( resultTypeStr + "(" + XConnection->Expression + ")" ) )
        : defaultVal;

    AString typeCastY = YValid ?
        ( ( resultType == YConnection->Type ) ? YConnection->Expression : ( resultTypeStr + "(" + YConnection->Expression + ")" ) )
        : defaultVal;

    AString typeCastZ = ZValid ?
        ( ( resultType == ZConnection->Type ) ? ZConnection->Expression : ( resultTypeStr + "(" + ZConnection->Expression + ")" ) )
        : defaultVal;

    AString typeCastW = WValid ?
        ( ( resultType == WConnection->Type ) ? WConnection->Expression : ( resultTypeStr + "(" + WConnection->Expression + ")" ) )
        : defaultVal;

    switch ( Result->Type ) {
    case AT_Float2:

        _Context.GenerateSourceCode( Result,
                  "vec2( "
                + typeCastX
                + ", "
                + typeCastY
                + " )",
                  false );
        break;

    case AT_Float3:
        _Context.GenerateSourceCode( Result,
                  "vec3( "
                + typeCastX
                + ", "
                + typeCastY
                + ", "
                + typeCastZ
                + " )",
                  false );
        break;

    case AT_Float4:
        _Context.GenerateSourceCode( Result,
                  "vec4( "
                + typeCastX
                + ", "
                + typeCastY
                + ", "
                + typeCastZ
                + ", "
                + typeCastW
                + " )",
                  false );
        break;

    case AT_Bool2:

        _Context.GenerateSourceCode( Result,
                  "bvec2( "
                + typeCastX
                + ", "
                + typeCastY
                + " )",
                  false );
        break;

    case AT_Bool3:
        _Context.GenerateSourceCode( Result,
                  "bvec3( "
                + typeCastX
                + ", "
                + typeCastY
                + ", "
                + typeCastZ
                + " )",
                  false );
        break;

    case AT_Bool4:
        _Context.GenerateSourceCode( Result,
                  "bvec4( "
                + typeCastX
                + ", "
                + typeCastY
                + ", "
                + typeCastZ
                + ", "
                + typeCastW
                + " )",
                  false );
        break;
    default:
        AN_ASSERT( 0 );
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGArithmeticFunction1 )
AN_CLASS_META( MGSaturate )
AN_CLASS_META( MGSinusNode )
AN_CLASS_META( MGCosinusNode )
AN_CLASS_META( MGFractNode )
AN_CLASS_META( MGNegateNode )
AN_CLASS_META( MGNormalizeNode )

MGArithmeticFunction1::MGArithmeticFunction1()
{
    AN_ASSERT( 0 );
}

MGArithmeticFunction1::MGArithmeticFunction1( EArithmeticFunction _Function, const char * _Name )
    : Super( _Name ), Function( _Function )
{
    Value = AddInput( "Value" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGArithmeticFunction1::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * connectionA = Value->GetConnection();

    if ( connectionA && Value->ConnectedNode()->Build( _Context ) ) {

        // Result type depends on input type
        if ( !IsArithmeticType( connectionA->Type ) ) {
            Result->Type = ToFloatType( connectionA->Type );
        } else {
            Result->Type = connectionA->Type;
        }

        AString expressionA = MakeVectorCast( connectionA->Expression, connectionA->Type, Result->Type );

        AString expression;

        switch ( Function ) {
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
            if ( Result->Type == AT_Float1 ) {
                expression = "1.0";
            //} else if ( Result->Type == AT_Integer1 ) { // If there will be integer types
            //    expression = "1";
            } else {
                expression = "normalize( " + expressionA + " )";
            }
            break;
        default:
            AN_ASSERT( 0 );
        }

        _Context.GenerateSourceCode( Result, expression, false );

    } else {
        Result->Type = AT_Float4;

        _Context.GenerateSourceCode( Result, MakeEmptyVector( Result->Type ), false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGArithmeticFunction2 )
AN_CLASS_META( MGMulNode )
AN_CLASS_META( MGDivNode )
AN_CLASS_META( MGAddNode )
AN_CLASS_META( MGSubNode )
AN_CLASS_META( MGStepNode )
AN_CLASS_META( MGPowNode )
AN_CLASS_META( MGModNode )
AN_CLASS_META( MGMin )
AN_CLASS_META( MGMax )

MGArithmeticFunction2::MGArithmeticFunction2()
{
    AN_ASSERT( 0 );
}

MGArithmeticFunction2::MGArithmeticFunction2( EArithmeticFunction _Function, const char * _Name )
    : Super( _Name ), Function( _Function )
{
    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGArithmeticFunction2::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * connectionA = ValueA->GetConnection();
    MGOutput * connectionB = ValueB->GetConnection();

    if ( connectionA && ValueA->ConnectedNode()->Build( _Context )
         && connectionB && ValueB->ConnectedNode()->Build( _Context ) ) {

        // Result type depends on input type
        if ( !IsArithmeticType( connectionA->Type ) ) {
            Result->Type = ToFloatType( connectionA->Type );
        } else {
            Result->Type = connectionA->Type;
        }

        AString expressionA = MakeVectorCast( connectionA->Expression, connectionA->Type, Result->Type );
        AString expressionB = MakeVectorCast( connectionB->Expression, connectionB->Type, Result->Type, VECTOR_CAST_EXPAND_VEC1 );

        AString expression;

        switch ( Function ) {
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
            AN_ASSERT( 0 );
        }

        _Context.GenerateSourceCode( Result, expression, false );

    } else {
        Result->Type = AT_Float4;

        _Context.GenerateSourceCode( Result, MakeEmptyVector( Result->Type ), false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGArithmeticFunction3 )
AN_CLASS_META( MGMADNode )
AN_CLASS_META( MGLerpNode )
AN_CLASS_META( MGClamp )

MGArithmeticFunction3::MGArithmeticFunction3()
{
    AN_ASSERT( 0 );
}

MGArithmeticFunction3::MGArithmeticFunction3( EArithmeticFunction _Function, const char * _Name )
    : Super( _Name ), Function( _Function )
{
    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );
    ValueC = AddInput( "C" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGArithmeticFunction3::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * connectionA = ValueA->GetConnection();
    MGOutput * connectionB = ValueB->GetConnection();
    MGOutput * connectionC = ValueC->GetConnection();

    if ( connectionA && ValueA->ConnectedNode()->Build( _Context )
         && connectionB && ValueB->ConnectedNode()->Build( _Context )
         && connectionC && ValueC->ConnectedNode()->Build( _Context ) ) {

        // Result type depends on input type
        if ( !IsArithmeticType( connectionA->Type ) ) {
            Result->Type = ToFloatType( connectionA->Type );
        } else {
            Result->Type = connectionA->Type;
        }

        AString expressionA = MakeVectorCast( connectionA->Expression, connectionA->Type, Result->Type );
        AString expressionB = MakeVectorCast( connectionB->Expression, connectionB->Type, Result->Type, VECTOR_CAST_EXPAND_VEC1 );
        AString expressionC = MakeVectorCast( connectionC->Expression, connectionC->Type, Result->Type, VECTOR_CAST_EXPAND_VEC1 );

        AString expression;

        switch ( Function ) {
        case Mad:
            expression = "(" + expressionA + " * " + expressionB + " + " + expressionC + ")";
            break;
        case Lerp:
            expression ="mix( " + expressionA + ", " + expressionB + ", " + expressionC + " )";
            break;
        case Clamp:
            expression = "clamp( " + expressionA + ", " + expressionB + ", " + expressionC + " )";
            break;
        default:
            AN_ASSERT( 0 );
        }

        _Context.GenerateSourceCode( Result, expression, false );

    } else {
        Result->Type = AT_Float4;
        _Context.GenerateSourceCode( Result, MakeEmptyVector( Result->Type ), false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGSpheremapCoord )

MGSpheremapCoord::MGSpheremapCoord() : Super( "Spheremap Coord" ) {
    Dir = AddInput( "Dir" );
    TexCoord = AddOutput( "TexCoord", AT_Float2 );
}

void MGSpheremapCoord::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * connectionDir = Dir->GetConnection();

    if ( connectionDir && Dir->ConnectedNode()->Build( _Context ) )
    {
        AString expressionDir = MakeVectorCast( connectionDir->Expression, connectionDir->Type, AT_Float3 );

        _Context.GenerateSourceCode( TexCoord, "builtin_spheremap_coord( " + expressionDir + " )", true );
    }
    else
    {
        _Context.GenerateSourceCode( TexCoord, MakeEmptyVector( AT_Float2 ), false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGLuminance )

MGLuminance::MGLuminance() : Super( "Luminance" ) {
    LinearColor = AddInput( "LinearColor" );
    Luminance = AddOutput( "Luminance", AT_Float1 );
}

void MGLuminance::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * connectionColor = LinearColor->GetConnection();

    if ( connectionColor && LinearColor->ConnectedNode()->Build( _Context ) )
    {
        AString expressionColor = MakeVectorCast( connectionColor->Expression, connectionColor->Type, AT_Float4, VECTOR_CAST_EXPAND_VEC1 );

        _Context.GenerateSourceCode( Luminance, "builtin_luminance( " + expressionColor + " )", false );

    } else {
        _Context.GenerateSourceCode( Luminance, MakeEmptyVector( AT_Float1 ), false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGPINode )
AN_END_CLASS_META()

MGPINode::MGPINode() : Super( "PI" ) {
    OutValue = AddOutput( "Value", AT_Float1 );
}

void MGPINode::Compute( AMaterialBuildContext & _Context ) {
    OutValue->Expression = "3.1415926";
}

AN_BEGIN_CLASS_META( MG2PINode )
AN_END_CLASS_META()

MG2PINode::MG2PINode() : Super( "2PI" ) {
    OutValue = AddOutput( "Value", AT_Float1 );
}

void MG2PINode::Compute( AMaterialBuildContext & _Context ) {
    OutValue->Expression = "6.2831853";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGBooleanNode )
//AN_ATTRIBUTE_( bValue, AF_DEFAULT )
AN_END_CLASS_META()

MGBooleanNode::MGBooleanNode() : Super( "Boolean" ) {
    OutValue = AddOutput( "Value", AT_Bool1 );
}

void MGBooleanNode::Compute( AMaterialBuildContext & _Context ) {
    OutValue->Expression = Math::ToString( bValue );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGBoolean2Node )
//AN_ATTRIBUTE_( bValue, AF_DEFAULT )
AN_END_CLASS_META()

MGBoolean2Node::MGBoolean2Node() : Super( "Boolean2" ) {
    OutValue = AddOutput( "Value", AT_Bool2 );
}

void MGBoolean2Node::Compute( AMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "bvec2( " + Math::ToString( bValue.X ) + ", " + Math::ToString( bValue.Y ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGBoolean3Node )
//AN_ATTRIBUTE_( bValue, AF_DEFAULT )
AN_END_CLASS_META()

MGBoolean3Node::MGBoolean3Node() : Super( "Boolean3" ) {
    OutValue = AddOutput( "Value", AT_Bool3 );
}

void MGBoolean3Node::Compute( AMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "bvec3( " + Math::ToString( bValue.X ) + ", " + Math::ToString( bValue.Y ) + ", " + Math::ToString( bValue.Z ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGBoolean4Node )
//AN_ATTRIBUTE_( bValue, AF_DEFAULT )
AN_END_CLASS_META()

MGBoolean4Node::MGBoolean4Node() : Super( "Boolean4" ) {
    OutValue = AddOutput( "Value", AT_Bool4 );
}

void MGBoolean4Node::Compute( AMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "bvec4( " + Math::ToString( bValue.X ) + ", " + Math::ToString( bValue.Y ) + ", " + Math::ToString( bValue.Z ) + ", " + Math::ToString( bValue.W ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloatNode )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloatNode::MGFloatNode() : Super( "Float" ) {
    OutValue = AddOutput( "Value", AT_Float1 );
}

void MGFloatNode::Compute( AMaterialBuildContext & _Context ) {
    OutValue->Expression = Math::ToString( Value );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloat2Node )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloat2Node::MGFloat2Node() : Super( "Float2" ) {
    OutValue = AddOutput( "Value", AT_Float2 );
}

void MGFloat2Node::Compute( AMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec2( " + Math::ToString( Value.X ) + ", " + Math::ToString( Value.Y ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloat3Node )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloat3Node::MGFloat3Node() : Super( "Float3" ) {
    OutValue = AddOutput( "Value", AT_Float3 );
}

void MGFloat3Node::Compute( AMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec3( " + Math::ToString( Value.X ) + ", " + Math::ToString( Value.Y ) + ", " + Math::ToString( Value.Z ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloat4Node )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloat4Node::MGFloat4Node() : Super( "Float4" ){
    OutValue = AddOutput( "Value", AT_Float4 );
}

void MGFloat4Node::Compute( AMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec4( " + Math::ToString( Value.X ) + ", " + Math::ToString( Value.Y ) + ", " + Math::ToString( Value.Z ) + ", " + Math::ToString( Value.W ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGTextureSlot )

MGTextureSlot::MGTextureSlot() : Super( "Texture Slot" ) {
    SamplerDesc.TextureType = TEXTURE_2D;
    SamplerDesc.Filter = TEXTURE_FILTER_LINEAR;
    SamplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
    SamplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
    SamplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
    SamplerDesc.MipLODBias = 0;
    SamplerDesc.Anisotropy = 16;
    SamplerDesc.MinLod = -1000;
    SamplerDesc.MaxLod = 1000;

    SlotIndex = -1;

    Value = AddOutput( "Value", AT_Unknown );
}

void MGTextureSlot::Compute( AMaterialBuildContext & _Context ) {
    if ( GetSlotIndex() >= 0 ) {
        Value->Expression = "tslot_" + Math::ToString( GetSlotIndex() );

        _Context.bHasTextures = true;
        _Context.MaxTextureSlot = Math::Max( _Context.MaxTextureSlot, GetSlotIndex() );

    } else {
        Value->Expression.Clear();
    }
}

static constexpr const char * TextureTypeToShaderSampler[][2] = {
    { "sampler1D", "float" },
    { "sampler1DArray", "vec2" },
    { "sampler2D", "vec2" },
    { "sampler2DArray", "vec3" },
    { "sampler3D", "vec3" },
    { "samplerCube", "vec3" },
    { "samplerCubeArray", "vec4" }
};

static const char * GetShaderType( ETextureType _Type ) {
    return TextureTypeToShaderSampler[ _Type ][ 0 ];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGUniformAddress )

MGUniformAddress::MGUniformAddress() : Super( "Texture Slot" ) {
    Type = AT_Float4;
    Address = 0;

    Value = AddOutput( "Value", Type );
}

void MGUniformAddress::Compute( AMaterialBuildContext & _Context ) {
    if ( Address >= 0 ) {

        int addr = Math::Clamp( Address, 0, 15 );
        int location = addr / 4;

        Value->Type = Type;
        Value->Expression = "uaddr_" + Math::ToString(location);
        switch ( Type ) {
            case AT_Float1:
                switch ( addr & 3 ) {
                    case 0: Value->Expression += ".x"; break;
                    case 1: Value->Expression += ".y"; break;
                    case 2: Value->Expression += ".z"; break;
                    case 3: Value->Expression += ".w"; break;
                }
                break;

            case AT_Float2:
                switch ( addr & 3 ) {
                    case 0: Value->Expression += ".xy"; break;
                    case 1: Value->Expression += ".yz"; break;
                    case 2: Value->Expression += ".zw"; break;
                    case 3: Value->Expression += ".ww"; break;// FIXME: error?
                }
                break;

            case AT_Float3:
                switch ( addr & 3 ) {
                    case 0: Value->Expression += ".xyz"; break;
                    case 1: Value->Expression += ".yzw"; break;
                    case 2: Value->Expression += ".www"; break;// FIXME: error?
                    case 3: Value->Expression += ".www"; break;// FIXME: error?
                }
                break;

            case AT_Float4:
                switch ( addr & 3 ) {
                    case 1: Value->Expression += ".yzww"; break;// FIXME: error?
                    case 2: Value->Expression += ".wwww"; break;// FIXME: error?
                    case 3: Value->Expression += ".wwww"; break;// FIXME: error?
                }
                break;

            default:
                AN_ASSERT( 0 );
                break;
        }

        _Context.MaxUniformAddress = Math::Max( _Context.MaxUniformAddress, location );

    } else {
        Value->Expression.Clear();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGSampler )
AN_ATTRIBUTE_( bSwappedToBGR, AF_DEFAULT )
AN_END_CLASS_META()

MGSampler::MGSampler() : Super( "Texture Sampler" ) {
    TextureSlot = AddInput( "TextureSlot" );
    TexCoord = AddInput( "TexCoord" );
    R = AddOutput( "R", AT_Float1 );
    G = AddOutput( "G", AT_Float1 );
    B = AddOutput( "B", AT_Float1 );
    A = AddOutput( "A", AT_Float1 );
    RGBA = AddOutput( "RGBA", AT_Float4 );
    RGB = AddOutput( "RGB", AT_Float3 );
}

static const char * ChooseSampleFunction( ETextureColorSpace _ColorSpace ) {
    switch ( _ColorSpace ) {
    case TEXTURE_COLORSPACE_RGBA:
        return "texture";
    case TEXTURE_COLORSPACE_SRGB_ALPHA:
        return "texture_srgb_alpha";
    case TEXTURE_COLORSPACE_YCOCG:
        return "texture_ycocg";
    case TEXTURE_COLORSPACE_GRAYSCALED:
        return "texture_grayscaled";
    default:
        return "texture";
    }
}

void MGSampler::Compute( AMaterialBuildContext & _Context ) {
    bool bValid = false;

    MGOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        MGNode * node = TextureSlot->ConnectedNode();
        if ( node->FinalClassId() == MGTextureSlot::ClassId() && node->Build( _Context ) ) {

            MGTextureSlot * texSlot = static_cast< MGTextureSlot * >( node );

            EMGNodeType sampleType = AT_Float2;

            // TODO: table?
            switch( texSlot->SamplerDesc.TextureType ) {
            case TEXTURE_1D:
                sampleType = AT_Float1;
                break;
            case TEXTURE_1D_ARRAY:
                sampleType = AT_Float2;
                break;
            case TEXTURE_2D:
                sampleType = AT_Float2;
                break;
            case TEXTURE_2D_ARRAY:
                sampleType = AT_Float3;
                break;
            case TEXTURE_3D:
                sampleType = AT_Float3;
                break;
            case TEXTURE_CUBEMAP:
                sampleType = AT_Float3;
                break;
            case TEXTURE_CUBEMAP_ARRAY:
                sampleType = AT_Float3;
                break;
            default:
                AN_ASSERT(0);
            };

            int32_t slotIndex = texSlot->GetSlotIndex();
            if ( slotIndex != -1 ) {

                MGOutput * texCoordCon = TexCoord->GetConnection();

                if ( texCoordCon && TexCoord->ConnectedNode()->Build( _Context ) ) {

                    const char * swizzleStr = bSwappedToBGR ? ".bgra" : "";

                    const char * sampleFunc = ChooseSampleFunction( ColorSpace );

                    RGBA->Expression = _Context.GenerateVariableName();
                    _Context.SourceCode += "const vec4 " + RGBA->Expression + " = "
                        + sampleFunc + "( tslot_" + Math::ToString( slotIndex ) + ", "
                        + MakeVectorCast( texCoordCon->Expression, texCoordCon->Type, sampleType ) + " )" + swizzleStr + ";\n";
                    bValid = true;
                }
            }
        }
    }

    if ( bValid ) {
        R->Expression = RGBA->Expression + ".r";
        G->Expression = RGBA->Expression + ".g";
        B->Expression = RGBA->Expression + ".b";
        A->Expression = RGBA->Expression + ".a";
        RGB->Expression = RGBA->Expression + ".rgb";
    } else {
        _Context.GenerateSourceCode( RGBA, MakeEmptyVector( AT_Float4 ), false );

        R->Expression = "0.0";
        G->Expression = "0.0";
        B->Expression = "0.0";
        A->Expression = "0.0";
        RGB->Expression = "vec3(0.0)";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGNormalSampler )
AN_END_CLASS_META()

MGNormalSampler::MGNormalSampler() : Super( "Normal Sampler" ){
    TextureSlot = AddInput( "TextureSlot" );
    TexCoord = AddInput( "TexCoord" );
    X = AddOutput( "X", AT_Float1 );
    Y = AddOutput( "Y", AT_Float1 );
    Z = AddOutput( "Z", AT_Float1 );
    XYZ = AddOutput( "XYZ", AT_Float3 );
}

static const char * ChooseSampleFunction( ENormalMapCompression _Compression ) {
    switch ( _Compression ) {
    case NM_XYZ:
        return "texture_nm_xyz";
    case NM_XY:
        return "texture_nm_xy";
    case NM_SPHEREMAP:
        return "texture_nm_spheremap";
    case NM_STEREOGRAPHIC:
        return "texture_nm_stereographic";
    case NM_PARABOLOID:
        return "texture_nm_paraboloid";
    case NM_QUARTIC:
        return "texture_nm_quartic";
    case NM_FLOAT:
        return "texture_nm_float";
    case NM_DXT5:
        return "texture_nm_dxt5";
    default:
        return "texture_nm_xyz";
    }
}

void MGNormalSampler::Compute( AMaterialBuildContext & _Context ) {
    bool bValid = false;

    MGOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        MGNode * node = TextureSlot->ConnectedNode();
        if ( node->FinalClassId() == MGTextureSlot::ClassId() && node->Build( _Context ) ) {

            MGTextureSlot * texSlot = static_cast< MGTextureSlot * >( node );

            EMGNodeType sampleType = AT_Float2;

            // TODO: table?
            switch( texSlot->SamplerDesc.TextureType ) {
            case TEXTURE_1D:
                sampleType = AT_Float1;
                break;
            case TEXTURE_1D_ARRAY:
                sampleType = AT_Float2;
                break;
            case TEXTURE_2D:
                sampleType = AT_Float2;
                break;
            case TEXTURE_2D_ARRAY:
                sampleType = AT_Float3;
                break;
            case TEXTURE_3D:
                sampleType = AT_Float3;
                break;
            case TEXTURE_CUBEMAP:
                sampleType = AT_Float3;
                break;
            case TEXTURE_CUBEMAP_ARRAY:
                sampleType = AT_Float3;
                break;
            default:
                AN_ASSERT(0);
            };

            int32_t slotIndex = texSlot->GetSlotIndex();
            if ( slotIndex != -1 ) {

                MGOutput * texCoordCon = TexCoord->GetConnection();

                if ( texCoordCon && TexCoord->ConnectedNode()->Build( _Context ) ) {

                    const char * sampleFunc = ChooseSampleFunction( Compression );

                    XYZ->Expression = _Context.GenerateVariableName();
                    _Context.SourceCode += "const vec3 " + XYZ->Expression + " = "
                        + sampleFunc + "( tslot_" + Math::ToString( slotIndex ) + ", "
                        + MakeVectorCast( texCoordCon->Expression, texCoordCon->Type, sampleType ) + " );\n";
                    bValid = true;
                }
            }
        }
    }

    if ( bValid ) {
        X->Expression = XYZ->Expression + ".x";
        Y->Expression = XYZ->Expression + ".y";
        Z->Expression = XYZ->Expression + ".z";
    } else {
        _Context.GenerateSourceCode( XYZ, MakeDefaultNormal(), false );

        X->Expression = "0.0";
        Y->Expression = "0.0";
        Z->Expression = "0.0";
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGParallaxMapSampler )
AN_END_CLASS_META()

MGParallaxMapSampler::MGParallaxMapSampler() : Super( "Parallax Map Sampler" ) {
    TextureSlot = AddInput( "TextureSlot" );
    TexCoord = AddInput( "TexCoord" );
    DisplacementScale = AddInput( "DisplacementScale" );
    SelfShadowing = AddInput( "SelfShadowing" );
    Result = AddOutput( "Result", AT_Float2 );
}

void MGParallaxMapSampler::Compute( AMaterialBuildContext & _Context ) {
    bool bValid = false;

    MGOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        MGNode * node = TextureSlot->ConnectedNode();
        if ( node->FinalClassId() == MGTextureSlot::ClassId() && node->Build( _Context ) ) {

            MGTextureSlot * texSlot = static_cast< MGTextureSlot * >(node);

            if ( texSlot->SamplerDesc.TextureType == TEXTURE_2D ) {
                int32_t slotIndex = texSlot->GetSlotIndex();
                if ( slotIndex != -1 ) {

                    MGOutput * texCoordCon = TexCoord->GetConnection();

                    if ( texCoordCon && TexCoord->ConnectedNode()->Build( _Context ) ) {

                        AString sampler = "tslot_" + Math::ToString( slotIndex );
                        AString texCoord = MakeVectorCast( texCoordCon->Expression, texCoordCon->Type, AT_Float2 );

                        AString displacementScale;
                        MGOutput * displacementScaleCon = DisplacementScale->GetConnection();
                        if ( displacementScaleCon && DisplacementScale->ConnectedNode()->Build( _Context ) ) {
                            displacementScale = MakeVectorCast( displacementScaleCon->Expression, displacementScaleCon->Type, AT_Float1 );
                        } else {
                            displacementScale = "0.05";
                        }

                        AString selfShadowing;
                        MGOutput * selfShadowingCon = SelfShadowing->GetConnection();
                        if ( selfShadowingCon && SelfShadowing->ConnectedNode()->Build( _Context ) ) {
                            selfShadowing = MakeVectorCast( selfShadowingCon->Expression, selfShadowingCon->Type, AT_Bool1 );
                        } else {
                            selfShadowing = MakeEmptyVector( AT_Bool1 );
                        }

                        Result->Expression = _Context.GenerateVariableName();
                        _Context.SourceCode += "const vec2 " + Result->Expression +
                            " = ParallaxMapping( " + sampler + ", " + texCoord + ", " + displacementScale + ", " + selfShadowing + " );\n";

                        bValid = true;

                        _Context.bParallaxSampler = true;
                    }
                }
            }
        }
    }

    if ( !bValid ) {
        _Context.GenerateSourceCode( Result, MakeEmptyVector( AT_Float2 ), false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGSamplerVT )
AN_ATTRIBUTE_( bSwappedToBGR, AF_DEFAULT )
AN_END_CLASS_META()

MGSamplerVT::MGSamplerVT() : Super( "Virtual Texture Sampler" ) {
    TextureLayer = 0;
    R = AddOutput( "R", AT_Float1 );
    G = AddOutput( "G", AT_Float1 );
    B = AddOutput( "B", AT_Float1 );
    A = AddOutput( "A", AT_Float1 );
    RGBA = AddOutput( "RGBA", AT_Float4 );
    RGB = AddOutput( "RGB", AT_Float3 );
}

void MGSamplerVT::Compute( AMaterialBuildContext & _Context ) {
    const char * swizzleStr = bSwappedToBGR ? ".bgra" : "";
    const char * sampleFunc = ChooseSampleFunction( ColorSpace );

    RGBA->Expression = _Context.GenerateVariableName();

    _Context.SourceCode +=
        "const vec4 "
        + RGBA->Expression
        + " = "
        + sampleFunc
        + "( vt_PhysCache" + Math::ToString( TextureLayer ) + ", InPhysicalUV )"
        + swizzleStr
        + ";\n";

    R->Expression = RGBA->Expression + ".r";
    G->Expression = RGBA->Expression + ".g";
    B->Expression = RGBA->Expression + ".b";
    A->Expression = RGBA->Expression + ".a";
    RGB->Expression = RGBA->Expression + ".rgb";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGNormalSamplerVT )
AN_END_CLASS_META()

MGNormalSamplerVT::MGNormalSamplerVT() : Super( "Virtual Texture Normal Sampler" ) {
    TextureLayer = 0;
    X = AddOutput( "X", AT_Float1 );
    Y = AddOutput( "Y", AT_Float1 );
    Z = AddOutput( "Z", AT_Float1 );
    XYZ = AddOutput( "XYZ", AT_Float3 );
}

void MGNormalSamplerVT::Compute( AMaterialBuildContext & _Context ) {
    const char * sampleFunc = ChooseSampleFunction( Compression );

    XYZ->Expression = _Context.GenerateVariableName();

    _Context.SourceCode +=
        "const vec3 "
        + XYZ->Expression
        + " = "
        + sampleFunc
        + "( vt_PhysCache" + Math::ToString( TextureLayer ) + ", InPhysicalUV );\n";

    X->Expression = XYZ->Expression + ".x";
    Y->Expression = XYZ->Expression + ".y";
    Z->Expression = XYZ->Expression + ".z";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInFragmentCoord )

MGInFragmentCoord::MGInFragmentCoord() : Super( "InFragmentCoord" ) {
    MGOutput * OutValue = AddOutput( "Value", AT_Float4 );
    OutValue->Expression = "gl_FragCoord";

    MGOutput * OutValueX = AddOutput( "X", AT_Float1 );
    OutValueX->Expression = "gl_FragCoord.x";

    MGOutput * OutValueY = AddOutput( "Y", AT_Float1 );
    OutValueY->Expression = "gl_FragCoord.y";

    MGOutput * OutValueZ = AddOutput( "Z", AT_Float1 );
    OutValueZ->Expression = "gl_FragCoord.z";

    MGOutput * OutValueW = AddOutput( "W", AT_Float1 );
    OutValueW->Expression = "gl_FragCoord.w";

    MGOutput * OutValueXY = AddOutput( "Position", AT_Float2 );
    OutValueXY->Expression = "gl_FragCoord.xy";

}

void MGInFragmentCoord::Compute( AMaterialBuildContext & _Context ) {
    // TODO: Case for vertex stage
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInPosition )

MGInPosition::MGInPosition() : Super( "InPosition" ) {
    Value = AddOutput( "Value", AT_Unknown );
}

void MGInPosition::Compute( AMaterialBuildContext & _Context ) {
    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
        Value->Type = AT_Float2;
    } else {
        Value->Type = AT_Float3;
    }

    if ( _Context.GetStage() != VERTEX_STAGE ) {
        _Context.InputVaryings.Append( SVarying( "V_Position", "VertexPosition", Value->Type ) );

        Value->Expression = "V_Position";
    } else {
        _Context.GenerateSourceCode( Value, "VertexPosition", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInNormal )

MGInNormal::MGInNormal() : Super( "InNormal" ) {
    Value = AddOutput( "Value", AT_Unknown );
    Value->Type = AT_Float3;
}

void MGInNormal::Compute( AMaterialBuildContext & _Context ) {

    //if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
    //    Value->Type = AT_Float2;
    //} else {
    //    Value->Type = AT_Float3;
    //}

    if ( _Context.GetStage() != VERTEX_STAGE ) {
        _Context.InputVaryings.Append( SVarying( "V_Normal", "VertexNormal", Value->Type ) );

        Value->Expression = "V_Normal";
    } else {
        Value->Expression = "VertexNormal";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInColor )

MGInColor::MGInColor() : Super( "InColor" ) {
    Value = AddOutput( "Value", AT_Float4 );
}

void MGInColor::Compute( AMaterialBuildContext & _Context ) {
    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {

        if ( _Context.GetStage() != VERTEX_STAGE ) {
            _Context.InputVaryings.Append( SVarying( "V_Color", "InColor", Value->Type ) );

            Value->Expression = "V_Color";
        } else {

            Value->Expression = "InColor";
        }

    } else {
        Value->Expression = "vec4(1.0)";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInTexCoord )

MGInTexCoord::MGInTexCoord() : Super( "InTexCoord" ) {
    Value = AddOutput( "Value", AT_Float2 );
}

void MGInTexCoord::Compute( AMaterialBuildContext & _Context ) {
    if ( _Context.GetStage() != VERTEX_STAGE ) {
        _Context.InputVaryings.Append( SVarying( "V_TexCoord", "InTexCoord", Value->Type ) );

        Value->Expression = "V_TexCoord";
    } else {
        Value->Expression = "InTexCoord";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
AN_CLASS_META( MGMaterialInLightmapTexCoord )

MGMaterialInLightmapTexCoord::MGMaterialInLightmapTexCoord() {
    Name = "InLightmapTexCoord";
    Stages = VERTEX_STAGE_BIT;

    MGOutput * OutValue = NewOutput( "Value", AT_Float2 );
    OutValue->Expression = "InLightmapTexCoord";
}

void MGMaterialInLightmapTexCoord::Compute( AMaterialBuildContext & _Context ) {
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInTimer )

MGInTimer::MGInTimer() : Super( "InTimer" ) {
    GameRunningTimeSeconds = AddOutput( "GameRunningTimeSeconds", AT_Float1 );
    GameRunningTimeSeconds->Expression = "GameRunningTimeSeconds";

    GameplayTimeSeconds = AddOutput( "GameplayTimeSeconds", AT_Float1 );
    GameplayTimeSeconds->Expression = "GameplayTimeSeconds";
}

void MGInTimer::Compute( AMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInViewPosition )

MGInViewPosition::MGInViewPosition() : Super( "InViewPosition" ) {
    MGOutput * Val = AddOutput( "Value", AT_Float3 );
    Val->Expression = "ViewPosition.xyz";
}

void MGInViewPosition::Compute( AMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGCondLess )
// TODO: add greater, lequal, gequal, equal, not equal

MGCondLess::MGCondLess() : Super( "Cond A < B" ) {
    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );

    True = AddInput( "True" );
    False = AddInput( "False" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGCondLess::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * connectionA = ValueA->GetConnection();
    MGOutput * connectionB = ValueB->GetConnection();
    MGOutput * connectionTrue = True->GetConnection();
    MGOutput * connectionFalse = False->GetConnection();

    AString expression;

    if ( connectionA
         && connectionB
         && connectionTrue
         && connectionFalse
         && ValueA->ConnectedNode()->Build( _Context )
         && ValueB->ConnectedNode()->Build( _Context )
         && True->ConnectedNode()->Build( _Context )
         && False->ConnectedNode()->Build( _Context ) )
    {
        if ( connectionA->Type != connectionB->Type || connectionTrue->Type != connectionFalse->Type
             || !IsArithmeticType( connectionA->Type ) )
        {
            Result->Type = AT_Float4;
            expression = MakeEmptyVector( AT_Float4 );
        }
        else
        {
            Result->Type = connectionTrue->Type;

            AString cond;

            if ( connectionA->Type == AT_Float1 )
            {
                cond = "step( " + connectionB->Expression + ", " + connectionA->Expression + " )";

                expression = "mix( " + connectionTrue->Expression + ", " + connectionFalse->Expression + ", " + cond + " )";
            }
            else
            {
                if ( Result->Type == AT_Float1 ) {
                    cond = "float( all( lessThan( " + connectionA->Expression + ", " + connectionB->Expression + " ) ) )";
                } else {
                    cond = AString( VariableTypeStr[ Result->Type ] ) + "( float( all( lessThan( " + connectionA->Expression + ", " + connectionB->Expression + " ) ) ) )";
                }

                expression = "mix( " + connectionFalse->Expression + ", " + connectionTrue->Expression + ", " + cond + " )";
            }
        }

    } else {
        Result->Type = AT_Float4;
        expression = MakeEmptyVector( AT_Float4 );
    }

    _Context.GenerateSourceCode( Result, expression, false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGAtmosphereNode )

MGAtmosphereNode::MGAtmosphereNode() : Super( "Atmosphere Scattering" ) {
    Dir = AddInput( "Dir" );
    Result = AddOutput( "Result", AT_Float4 );
}

void MGAtmosphereNode::Compute( AMaterialBuildContext & _Context ) {
    MGOutput * dirConnection = Dir->GetConnection();

    if ( dirConnection && Dir->ConnectedNode()->Build( _Context ) ) {
        AString expression = MakeVectorCast( dirConnection->Expression, dirConnection->Type, AT_Float3 );
        _Context.GenerateSourceCode( Result, "vec4( atmosphere( normalize(" + expression + "), normalize(vec3(0.5,0.5,-1)) ), 1.0 )", false );
    } else {
        Result->Expression = MakeEmptyVector( AT_Float4 );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////


static AString SamplersString( MGMaterialGraph const * InGraph, int MaxtextureSlot ) {
    AString s;
    AString bindingStr;

    for ( MGTextureSlot * slot : InGraph->GetTextureSlots() ) {
        if ( slot->GetSlotIndex() <= MaxtextureSlot ) {
            bindingStr = Math::ToString(slot->GetSlotIndex());
            s += "layout( binding = " + bindingStr + " ) uniform " + GetShaderType( slot->SamplerDesc.TextureType ) + " tslot_" + bindingStr + ";\n";
        }
    }
    return s;
}

static const char * texture_srgb_alpha =
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

static const char * texture_ycocg =
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

static const char * texture_grayscaled =
        "vec4 texture_grayscaled( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  return vec4( texture( sampler, texCoord ).r );\n"
        "}\n";

static const char * texture_nm_xyz =
        "vec3 texture_nm_xyz( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;\n"
        "}\n";

static const char * texture_nm_xy =
        "vec3 texture_nm_xy( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;\n"
        "  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );\n"
        "  return decodedN;\n"
        "}\n";

static const char * texture_nm_spheremap =
        "vec3 texture_nm_spheremap( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;\n"
        "  float f = dot( fenc, fenc );\n"
        "  vec3 decodedN;\n"
        "  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );\n"
        "  decodedN.z = 1.0 - f / 2.0;\n"
        "  return decodedN;\n"
        "}\n";

static const char * texture_nm_stereographic =
        "vec3 texture_nm_stereographic( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  vec3 decodedN;\n"
        "  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;\n"
        "  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );\n"
        "  decodedN.xy *= denom;\n"
        "  decodedN.z = denom - 1.0;\n"
        "  return decodedN;\n"
        "}\n";

static const char * texture_nm_paraboloid =
        "vec3 texture_nm_paraboloid( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  vec3 decodedN;\n"
        "  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;\n"
        "  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );\n"
        //n = normalize( n );
        "  return decodedN;\n"
        "}\n";

static const char * texture_nm_quartic =
        "vec3 texture_nm_quartic( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  vec3 decodedN;\n"
        "  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;\n"
        "  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );\n"
        //n = normalize( n );
        "  return decodedN;\n"
        "}\n";

static const char * texture_nm_float =
        "vec3 texture_nm_float( in %s sampler, in %s texCoord )\n"
        "{\n"
        "  vec3 decodedN;\n"
        "  decodedN.xy = texture( sampler, texCoord ).xy;\n"
        "  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );\n"
        "  return decodedN;\n"
        "}\n";

static const char * texture_nm_dxt5 =
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

static const char * builtin_spheremap_coord =
        "vec2 builtin_spheremap_coord( in vec3 dir ) {\n"
        "  vec2 uv = vec2( atan( dir.z, dir.x ), asin( dir.y ) );\n"
        "  return uv * vec2(0.1591, 0.3183) + 0.5;\n"
        "}\n";

static const char * builtin_luminance =
        "float builtin_luminance( in vec3 color ) {\n"
        "  return dot( color, vec3( 0.2126, 0.7152, 0.0722 ) );\n"
        "}\n"
        "float builtin_luminance( in vec4 color ) {\n"
        "  return dot( color, vec4( 0.2126, 0.7152, 0.0722, 0.0 ) );\n"
        "}\n";

static const char * builtin_saturate =
        "%s builtin_saturate( in %s color ) {\n"
        "  return clamp( color, %s(0.0), %s(1.0) );\n"
        "}\n";

static void GenerateBuiltinSource() {
    AString builtin;
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_srgb_alpha, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_ycocg, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_grayscaled, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_xyz, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_xy, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_spheremap, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_stereographic, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_paraboloid, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_quartic, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_float, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        builtin += Core::Fmt( texture_nm_dxt5, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }

    builtin += builtin_spheremap_coord;
    builtin += builtin_luminance;

    for ( int i = AT_Float1 ; i <= AT_Float4 ; i++ ) {
        builtin += Core::Fmt( builtin_saturate, VariableTypeStr[i], VariableTypeStr[i], VariableTypeStr[i], VariableTypeStr[i] );
    }

    AFileStream f;
    f.OpenWrite( "material_builtin.glsl" );
    f.WriteBuffer(builtin.CStr(),builtin.Length());
}

static void WriteDebugShaders( SMaterialShader const * Shaders ) {
    AFileStream f;
    if ( !f.OpenWrite( "debug.glsl" ) ) {
        return;
    }

    for ( SMaterialShader const * s = Shaders ; s ; s = s->Next ) {
        f.Printf( "//----------------------------------\n// %s\n//----------------------------------\n", s->SourceName );
        f.Printf( "%s\n", s->Code );
    }
}

static AString GenerateOutputVaryingsCode( TStdVector< SVarying > const & Varyings, const char * Prefix, bool bArrays )
{
    AString s;
    uint32_t location = 0;
    for ( SVarying const & v : Varyings ) {
        if ( v.RefCount > 0 ) {
            s += "layout( location = " + Math::ToString( location ) + " ) out " + VariableTypeStr[v.VaryingType] + " " + Prefix + v.VaryingName;
            if ( bArrays )
                s += "[]";
            s += ";\n";
        }
        location++;
    }
    return s;
}

static AString GenerateInputVaryingsCode( TStdVector< SVarying > const & Varyings, const char * Prefix, bool bArrays )
{
    AString s;
    uint32_t location = 0;
    for ( SVarying const & v : Varyings ) {
        if ( v.RefCount > 0 ) {
            s += "layout( location = " + Math::ToString( location ) + " ) in " + VariableTypeStr[v.VaryingType] + " " + Prefix + v.VaryingName;
            if ( bArrays )
                s += "[]";
            s += ";\n";
        }
        location++;
    }
    return s;
}

#if 0
static void MergeVaryings( TStdVector< SVarying > const & A, TStdVector< SVarying > const & B, TStdVector< SVarying > & Result )
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
            Result.Append( B[i] );
        }
    }
}

static bool FindVarying( TStdVector< SVarying > const & Varyings, SVarying const & Var )
{
    for ( int i = 0 ; i < Varyings.Size() ; i++ ) {
        if ( Var.VaryingName == Varyings[i].VaryingName ) {
            return true;
        }
    }
    return false;
}
#endif

static void AddVaryings( TStdVector< SVarying > & InOutResult, TStdVector< SVarying > const & B )
{
    if ( InOutResult.IsEmpty() ) {
        InOutResult = B;
        for ( int i = 0 ; i < InOutResult.Size() ; i++ ) {
            InOutResult[i].RefCount = 1;
        }
        return;
    }

    for ( int i = 0 ; i < B.Size() ; i++ ) {
        bool bMatch = false;
        for ( int j = 0 ; j < InOutResult.Size() ; j++ ) {
            if ( B[i].VaryingName == InOutResult[j].VaryingName ) {
                bMatch = true;
                InOutResult[j].RefCount++;
                break;
            }
        }
        if ( !bMatch ) {
            InOutResult.Append( B[i] );
        }
    }
}

static void RemoveVaryings( TStdVector< SVarying > & InOutResult, TStdVector< SVarying > const & B )
{
    for ( int i = 0 ; i < B.Size() ; i++ ) {
        for ( int j = 0 ; j < InOutResult.Size() ; j++ ) {
            if ( B[i].VaryingName == InOutResult[j].VaryingName ) {
                InOutResult[j].RefCount--;
                break;
            }
        }
    }
}

struct SMaterialStageTransition
{
    TStdVector< SVarying > Varyings;
    bool bHasTextures;
    int MaxTextureSlot;
    int MaxUniformAddress;
};

void MGMaterialGraph::CreateStageTransitions( SMaterialStageTransition & Transition,
                                              AMaterialBuildContext * VertexStage,
                                              AMaterialBuildContext * TessControlStage,
                                              AMaterialBuildContext * TessEvalStage,
                                              AMaterialBuildContext * GeometryStage,
                                              AMaterialBuildContext * FragmentStage )
{
    AN_ASSERT( VertexStage != nullptr );

    TStdVector< SVarying > & varyings = Transition.Varyings;

    varyings.Clear();

    Transition.bHasTextures = VertexStage->bHasTextures;
    Transition.MaxTextureSlot = VertexStage->MaxTextureSlot;
    Transition.MaxUniformAddress = VertexStage->MaxUniformAddress;

    if ( FragmentStage ) {
        AddVaryings( varyings, FragmentStage->InputVaryings );

        Transition.bHasTextures |= FragmentStage->bHasTextures;
        Transition.MaxTextureSlot = Math::Max( Transition.MaxTextureSlot, FragmentStage->MaxTextureSlot );
        Transition.MaxUniformAddress = Math::Max( Transition.MaxUniformAddress, FragmentStage->MaxUniformAddress );
    }

    if ( GeometryStage ) {
        AddVaryings( varyings, GeometryStage->InputVaryings );

        Transition.bHasTextures |= GeometryStage->bHasTextures;
        Transition.MaxTextureSlot = Math::Max( Transition.MaxTextureSlot, GeometryStage->MaxTextureSlot );
        Transition.MaxUniformAddress = Math::Max( Transition.MaxUniformAddress, GeometryStage->MaxUniformAddress );
    }

    if ( TessEvalStage && TessControlStage ) {
        AddVaryings( varyings, TessEvalStage->InputVaryings );
        AddVaryings( varyings, TessControlStage->InputVaryings );

        Transition.bHasTextures |= TessEvalStage->bHasTextures;
        Transition.MaxTextureSlot = Math::Max( Transition.MaxTextureSlot, TessEvalStage->MaxTextureSlot );
        Transition.MaxUniformAddress = Math::Max( Transition.MaxUniformAddress, TessEvalStage->MaxUniformAddress );

        Transition.bHasTextures |= TessControlStage->bHasTextures;
        Transition.MaxTextureSlot = Math::Max( Transition.MaxTextureSlot, TessControlStage->MaxTextureSlot );
        Transition.MaxUniformAddress = Math::Max( Transition.MaxUniformAddress, TessControlStage->MaxUniformAddress );
    }

    for ( SVarying const & v : varyings ) {
        VertexStage->SourceCode += "VS_" + v.VaryingName + " = " + v.VaryingSource + ";\n";
    }

    VertexStage->OutputVaryingsCode = GenerateOutputVaryingsCode( varyings, "VS_", false );

    const char * lastPrefix = "VS_";

    if ( TessEvalStage && TessControlStage ) {
        TessControlStage->InputVaryingsCode = GenerateInputVaryingsCode( varyings, "VS_", true );
        RemoveVaryings( varyings, TessControlStage->InputVaryings );

        if ( TessellationMethod == TESSELLATION_FLAT ) {
            TessControlStage->OutputVaryingsCode = GenerateOutputVaryingsCode( varyings, "TCS_", true );

            for ( SVarying const & v : varyings ) {
                if ( v.RefCount == 0 ) {
                    TessControlStage->CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    TessControlStage->CopyVaryingsCode += " " + v.VaryingName + " = VS_" + v.VaryingName + "[gl_InvocationID];\n";
                } else {
                    TessControlStage->CopyVaryingsCode += "TCS_" + v.VaryingName + "[gl_InvocationID] = VS_" + v.VaryingName + "[gl_InvocationID];\n";
                    TessControlStage->CopyVaryingsCode += "#define " + v.VaryingName + " VS_" + v.VaryingName + "[gl_InvocationID]\n";
                }
            }

            TessEvalStage->InputVaryingsCode = GenerateInputVaryingsCode( varyings, "TCS_", true );
            RemoveVaryings( varyings, TessEvalStage->InputVaryings );
            TessEvalStage->OutputVaryingsCode = GenerateOutputVaryingsCode( varyings, "TES_", false );

            for ( SVarying const & v : varyings ) {
                if ( v.RefCount == 0 ) {
                    TessEvalStage->CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    TessEvalStage->CopyVaryingsCode += " ";
                }
                TessEvalStage->CopyVaryingsCode += "TES_" + v.VaryingName +
                    " = gl_TessCoord.x * TCS_" + v.VaryingName + "[0]" +
                    " + gl_TessCoord.y * TCS_" + v.VaryingName + "[1]" +
                    " + gl_TessCoord.z * TCS_" + v.VaryingName + "[2];\n";
            }
        } else { // TESSELLATION_PN

            AString patchLocationStr;

            int patchLocation = 0;
            TessControlStage->OutputVaryingsCode.Clear();
            for ( SVarying const & v : varyings ) {
                if ( v.RefCount > 0 ) {
                    TessControlStage->OutputVaryingsCode += "layout( location = " + Math::ToString( patchLocation ) + " ) out patch " + VariableTypeStr[v.VaryingType] + " TCS_" + v.VaryingName + "[3];\n";
                    patchLocation += 3;
                }
            }
            patchLocationStr = "#define PATCH_LOCATION " + Math::ToString( patchLocation );
            TessControlStage->OutputVaryingsCode += patchLocationStr;

            for ( SVarying const & v : varyings ) {
                if ( v.RefCount == 0 ) {
                    TessControlStage->CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    TessControlStage->CopyVaryingsCode += " " + v.VaryingName + " = VS_" + v.VaryingName + "[0];\n";
                }
            }

            TessControlStage->CopyVaryingsCode += "for ( int i = 0 ; i < 3 ; i++ ) {\n";
            for ( SVarying const & v : varyings ) {
                if ( v.RefCount > 0 ) {
                    TessControlStage->CopyVaryingsCode += "TCS_" + v.VaryingName + "[i] = VS_" + v.VaryingName + "[i];\n";
                    TessControlStage->CopyVaryingsCode += "#define " + v.VaryingName + " VS_" + v.VaryingName + "[0]\n";
                }
            }
            TessControlStage->CopyVaryingsCode += "}\n";

            patchLocation = 0;
            TessEvalStage->InputVaryingsCode.Clear();
            for ( SVarying const & v : varyings ) {
                if ( v.RefCount > 0 ) {
                    //TessEvalStage->InputVaryingsCode += AString( VariableTypeStr[v.VaryingType] ) + " " + v.VaryingName + "[3];\n";
                    TessEvalStage->InputVaryingsCode += "layout( location = " + Math::ToString( patchLocation ) + " ) in patch " + VariableTypeStr[v.VaryingType] + " TCS_" + v.VaryingName + "[3];\n";
                    patchLocation += 3;
                }
            }
            TessEvalStage->InputVaryingsCode += patchLocationStr;

            RemoveVaryings( varyings, TessEvalStage->InputVaryings );
            TessEvalStage->OutputVaryingsCode = GenerateOutputVaryingsCode( varyings, "TES_", false );

            for ( SVarying const & v : varyings ) {
                if ( v.RefCount == 0 ) {
                    TessEvalStage->CopyVaryingsCode += VariableTypeStr[v.VaryingType];
                    TessEvalStage->CopyVaryingsCode += " ";
                }
                TessEvalStage->CopyVaryingsCode += "TES_" + v.VaryingName +
                    " = gl_TessCoord.x * TCS_" + v.VaryingName + "[0]" +
                    " + gl_TessCoord.y * TCS_" + v.VaryingName + "[1]" +
                    " + gl_TessCoord.z * TCS_" + v.VaryingName + "[2];\n";
            }
        }

        for ( SVarying const & v : TessEvalStage->InputVaryings ) {
            TessEvalStage->CopyVaryingsCode += "#define " + v.VaryingName + " TES_" + v.VaryingName + "\n";
        }

        lastPrefix = "TES_";
    }

    if ( GeometryStage ) {
        GeometryStage->InputVaryingsCode = GenerateInputVaryingsCode( varyings, lastPrefix, true );
        RemoveVaryings( varyings, GeometryStage->InputVaryings );
        GeometryStage->OutputVaryingsCode = GenerateOutputVaryingsCode( varyings, "GS_", true );

        for ( SVarying const & v : varyings ) {
            if ( v.RefCount > 0 ) {
                GeometryStage->CopyVaryingsCode += "GS_" + v.VaryingName + "[i] = " + lastPrefix + v.VaryingName + "[i];\n";
            }
        }

        lastPrefix = "GS_";
    }

    if ( FragmentStage ) {
        FragmentStage->InputVaryingsCode = GenerateInputVaryingsCode( varyings, lastPrefix, false );
        RemoveVaryings( varyings, FragmentStage->InputVaryings );

        if ( *lastPrefix ) {
            for ( SVarying const & v : FragmentStage->InputVaryings ) {
                FragmentStage->InputVaryingsCode += "#define " + v.VaryingName + " " + lastPrefix + v.VaryingName + "\n";
            }
        }
    }

    for ( SVarying const & v : varyings ) {
        AN_ASSERT( v.RefCount == 0 );
    }
#if 0
    AFileStream f;
    if ( !f.OpenWrite( "debug2.glsl" ) ) {
        return;
    }

    f.Printf( "VERTEX VARYINGS:\n%s\n", VertexStage->OutputVaryingsCode.CStr() );

    if ( TessEvalStage && TessControlStage ) {
        f.Printf( "TCS INPUT VARYINGS:\n%s\n", TessControlStage->InputVaryingsCode.CStr() );
        f.Printf( "TCS OUTPUT VARYINGS:\n%s\n", TessControlStage->OutputVaryingsCode.CStr() );
        f.Printf( "TCS COPY VARYINGS:\n%s\n", TessControlStage->CopyVaryingsCode.CStr() );

        f.Printf( "TES INPUT VARYINGS:\n%s\n", TessEvalStage->InputVaryingsCode.CStr() );
        f.Printf( "TES OUTPUT VARYINGS:\n%s\n", TessEvalStage->OutputVaryingsCode.CStr() );
        f.Printf( "TES COPY VARYINGS:\n%s\n", TessEvalStage->CopyVaryingsCode.CStr() );
    }

    if ( GeometryStage ) {
        f.Printf( "GS INPUT VARYINGS:\n%s\n", GeometryStage->InputVaryingsCode.CStr() );
        f.Printf( "GS OUTPUT VARYINGS:\n%s\n", GeometryStage->OutputVaryingsCode.CStr() );
        f.Printf( "GS COPY VARYINGS:\n%s\n", GeometryStage->CopyVaryingsCode.CStr() );
    }

    if ( FragmentStage ) {
        f.Printf( "FS INPUT VARYINGS:\n%s\n", FragmentStage->InputVaryingsCode.CStr() );
        f.Printf( "FS COPY VARYINGS:\n%s\n", FragmentStage->CopyVaryingsCode.CStr() );
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMaterialGraph )

MGMaterialGraph::MGMaterialGraph()
    : Super( "Material Graph" )
{
    Color = AddInput( "Color" );
    Normal = AddInput( "Normal" );
    Metallic = AddInput( "Metallic" );
    Roughness = AddInput( "Roughness" );
    AmbientOcclusion = AddInput( "AmbientOcclusion" );
    AmbientLight = AddInput( "AmbientLight" );
    Emissive = AddInput( "Emissive" );
    Specular = AddInput( "Specular" );
    Opacity = AddInput( "Opacity" );
    VertexDeform = AddInput( "VertexDeform" );
    ShadowMask = AddInput( "ShadowMask" );
    Displacement = AddInput( "Displacement" );
    TessellationFactor = AddInput( "TessellationFactor" );

    NodeIdGen = 0;
}

MGMaterialGraph::~MGMaterialGraph() {
    for ( MGTextureSlot * sampler : TextureSlots ) {
        sampler->RemoveRef();
    }

    for ( MGNode * node : Nodes ) {
        node->RemoveRef();
    }
}

void MGMaterialGraph::CompileStage( AMaterialBuildContext & ctx )
{
    static int BuildSerial = 0;

    ctx.Serial = ++BuildSerial;

    ResetConnections( ctx );
    TouchConnections( ctx );
    Build( ctx );
}

void MGMaterialGraph::RegisterTextureSlot( MGTextureSlot * _Slot ) {
    if ( TextureSlots.Size() >= MAX_MATERIAL_TEXTURES ) {
        GLogger.Printf( "AMaterialBuilder::RegisterTextureSlot: MAX_MATERIAL_TEXTURES hit\n");
        return;
    }
    _Slot->AddRef();
    _Slot->SlotIndex = TextureSlots.Size();
    TextureSlots.Append( _Slot );
}

int MGMaterialGraph::Serialize( ADocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    if ( !Nodes.IsEmpty() ) {
        int array = _Doc.AddArray( object, "Nodes" );

        for ( MGNode * node : Nodes ) {
            int nodeObject = node->Serialize( _Doc );
            _Doc.AddValueToField( array, nodeObject );
        }
    }

    return object;
}

void CompileMaterialGraph( MGMaterialGraph * InGraph, SMaterialDef * pDef )
{
    bool bDepthPassTextureFetch = false;
    bool bColorPassTextureFetch = false;
    bool bShadowMapPassTextureFetch = false;
    bool bShadowMapMasking = false;
    bool bNoCastShadow = false;
    uint32_t lightmapSlot = 0;
    int maxTextureSlot = -1;
    int maxUniformAddress = -1;
    bool bHasVertexDeform = false;

    pDef->bWireframePassTextureFetch = false;
    pDef->bNormalsPassTextureFetch = false;

    AString predefines;

    switch ( InGraph->MaterialType ) {
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
        AN_ASSERT( 0 );
    }

    switch ( InGraph->TessellationMethod ) {
    case TESSELLATION_FLAT:
        predefines += "#define TESSELLATION_METHOD TESSELLATION_FLAT\n";
        break;
    case TESSELLATION_PN:
        predefines += "#define TESSELLATION_METHOD TESSELLATION_PN\n";
        break;
    default:
        break;
    }

    if ( InGraph->DepthHack == MATERIAL_DEPTH_HACK_WEAPON ) {
        predefines += "#define WEAPON_DEPTH_HACK\n";
        bNoCastShadow = true;
    } else if ( InGraph->DepthHack == MATERIAL_DEPTH_HACK_SKYBOX ) {
        predefines += "#define SKYBOX_DEPTH_HACK\n";
        bNoCastShadow = true;
    }

    if ( InGraph->bTranslucent ) {
        predefines += "#define TRANSLUCENT\n";
    }

    if ( InGraph->bNoLightmap ) {
        predefines += "#define NO_LIGHTMAP\n";
    }

    if ( InGraph->bAllowScreenSpaceReflections ) {
        predefines += "#define ALLOW_SSLR\n";
    }

    if ( InGraph->bAllowScreenAmbientOcclusion ) {
        predefines += "#define ALLOW_SSAO\n";
    }

    if ( InGraph->bAllowShadowReceive ) {
        predefines += "#define ALLOW_SHADOW_RECEIVE\n";
    }

    if ( InGraph->bDisplacementAffectShadow ) {
        predefines += "#define DISPLACEMENT_AFFECT_SHADOW\n";
    }
    if ( InGraph->bPerBoneMotionBlur ) {
        predefines += "#define PER_BONE_MOTION_BLUR\n";
    }

    if ( InGraph->MotionBlurScale > 0.0f ) {
        predefines += "#define ALLOW_MOTION_BLUR\n";
    }

    predefines += "#define MOTION_BLUR_SCALE " + Math::ToString( Math::Saturate( InGraph->MotionBlurScale ) ) + "\n";

    if ( InGraph->bUseVirtualTexture ) {
        predefines += "#define USE_VIRTUAL_TEXTURE\n";
        predefines += "#define VT_LAYERS 1\n"; // TODO: Add based on material
    }

    if ( !InGraph->bDepthTest /*|| InGraph->bTranslucent */ ) {
        bNoCastShadow = true;
    }

    if ( InGraph->Blending == COLOR_BLENDING_PREMULTIPLIED_ALPHA ) {
        predefines += "#define PREMULTIPLIED_ALPHA\n";
    }

    // TODO: Для MATERIAL_PASS_DEPTH, MATERIAL_PASS_SHADOWMAP, MATERIAL_PASS_WIREFRAME, MATERIAL_PASS_NORMALS проверить:
    // если нет никакой деформации вершины, то использовать шейдер/пайплайн по умолчанию
    // для данного прохода. Это необходимо для оптимизации количества переключения пайплайнов.

    // Create depth pass
    {
        AMaterialBuildContext vertexCtx( InGraph, VERTEX_STAGE );
        AMaterialBuildContext tessControlCtx( InGraph, TESSELLATION_CONTROL_STAGE );
        AMaterialBuildContext tessEvalCtx( InGraph, TESSELLATION_EVAL_STAGE );
        SMaterialStageTransition trans;

        InGraph->CompileStage( vertexCtx );

        if ( InGraph->TessellationMethod != TESSELLATION_DISABLED ) {
            InGraph->CompileStage( tessControlCtx );
            InGraph->CompileStage( tessEvalCtx );
        }

        InGraph->CreateStageTransitions( trans,
                                         &vertexCtx,
                                         InGraph->TessellationMethod != TESSELLATION_DISABLED ? &tessControlCtx : nullptr,
                                         InGraph->TessellationMethod != TESSELLATION_DISABLED ? &tessEvalCtx : nullptr,
                                         nullptr,
                                         nullptr );

        bDepthPassTextureFetch = trans.bHasTextures;
        maxTextureSlot = Math::Max( maxTextureSlot, trans.MaxTextureSlot );
        maxUniformAddress = Math::Max( maxUniformAddress, trans.MaxUniformAddress );

        int locationIndex = trans.Varyings.Size();

        predefines += "#define DEPTH_PASS_VARYING_POSITION "    + Math::ToString( locationIndex++ ) + "\n";
        predefines += "#define DEPTH_PASS_VARYING_NORMAL "      + Math::ToString( locationIndex++ ) + "\n";

        pDef->AddShader( "$DEPTH_PASS_VERTEX_OUTPUT_VARYINGS$", vertexCtx.OutputVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_VERTEX_SAMPLERS$", SamplersString( InGraph, vertexCtx.MaxTextureSlot ) );
        pDef->AddShader( "$DEPTH_PASS_VERTEX_CODE$", vertexCtx.SourceCode );

        pDef->AddShader( "$DEPTH_PASS_TCS_INPUT_VARYINGS$", tessControlCtx.InputVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_TCS_OUTPUT_VARYINGS$", tessControlCtx.OutputVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_TCS_SAMPLERS$", SamplersString( InGraph, tessControlCtx.MaxTextureSlot ) );
        pDef->AddShader( "$DEPTH_PASS_TCS_COPY_VARYINGS$", tessControlCtx.CopyVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_TCS_CODE$", tessControlCtx.SourceCode );

        pDef->AddShader( "$DEPTH_PASS_TES_INPUT_VARYINGS$", tessEvalCtx.InputVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_TES_OUTPUT_VARYINGS$", tessEvalCtx.OutputVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_TES_SAMPLERS$", SamplersString( InGraph, tessEvalCtx.MaxTextureSlot ) );
        pDef->AddShader( "$DEPTH_PASS_TES_INTERPOLATE$", tessEvalCtx.CopyVaryingsCode );
        pDef->AddShader( "$DEPTH_PASS_TES_CODE$", tessEvalCtx.SourceCode );
    }

    // Create shadowmap pass
    {
        AMaterialBuildContext vertexCtx( InGraph, VERTEX_STAGE );
        AMaterialBuildContext tessControlCtx( InGraph, TESSELLATION_CONTROL_STAGE );
        AMaterialBuildContext tessEvalCtx( InGraph, TESSELLATION_EVAL_STAGE );
        AMaterialBuildContext geometryCtx( InGraph, GEOMETRY_STAGE );
        AMaterialBuildContext shadowCtx( InGraph, SHADOWCAST_STAGE );
        SMaterialStageTransition trans;

        InGraph->CompileStage( vertexCtx );
        InGraph->CompileStage( shadowCtx );

        bool bTess = InGraph->TessellationMethod != TESSELLATION_DISABLED && InGraph->bDisplacementAffectShadow;

        if ( bTess ) {
            InGraph->CompileStage( tessControlCtx );
            InGraph->CompileStage( tessEvalCtx );
        }

        InGraph->CreateStageTransitions( trans,
                                         &vertexCtx,
                                         bTess ? &tessControlCtx : nullptr,
                                         bTess ? &tessEvalCtx : nullptr,
                                         &geometryCtx,
                                         &shadowCtx );

        bShadowMapPassTextureFetch = trans.bHasTextures;
        bShadowMapMasking = !shadowCtx.SourceCode.IsEmpty();
        maxTextureSlot = Math::Max( maxTextureSlot, trans.MaxTextureSlot );
        maxUniformAddress = Math::Max( maxUniformAddress, trans.MaxUniformAddress );

        int locationIndex = trans.Varyings.Size();

        predefines += "#define SHADOWMAP_PASS_VARYING_INSTANCE_ID " + Math::ToString( locationIndex++ ) + "\n";
        predefines += "#define SHADOWMAP_PASS_VARYING_POSITION "    + Math::ToString( locationIndex++ ) + "\n";
        predefines += "#define SHADOWMAP_PASS_VARYING_NORMAL "      + Math::ToString( locationIndex++ ) + "\n";

        pDef->AddShader( "$SHADOWMAP_PASS_VERTEX_OUTPUT_VARYINGS$", vertexCtx.OutputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_VERTEX_SAMPLERS$", SamplersString( InGraph, vertexCtx.MaxTextureSlot ) );
        pDef->AddShader( "$SHADOWMAP_PASS_VERTEX_CODE$", vertexCtx.SourceCode );

        pDef->AddShader( "$SHADOWMAP_PASS_TCS_INPUT_VARYINGS$", tessControlCtx.InputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_TCS_OUTPUT_VARYINGS$", tessControlCtx.OutputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_TCS_SAMPLERS$", SamplersString( InGraph, tessControlCtx.MaxTextureSlot ) );
        pDef->AddShader( "$SHADOWMAP_PASS_TCS_COPY_VARYINGS$", tessControlCtx.CopyVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_TCS_CODE$", tessControlCtx.SourceCode );

        pDef->AddShader( "$SHADOWMAP_PASS_TES_INPUT_VARYINGS$", tessEvalCtx.InputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_TES_OUTPUT_VARYINGS$", tessEvalCtx.OutputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_TES_SAMPLERS$", SamplersString( InGraph, tessEvalCtx.MaxTextureSlot ) );
        pDef->AddShader( "$SHADOWMAP_PASS_TES_INTERPOLATE$", tessEvalCtx.CopyVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_TES_CODE$", tessEvalCtx.SourceCode );

        pDef->AddShader( "$SHADOWMAP_PASS_GEOMETRY_INPUT_VARYINGS$", geometryCtx.InputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_GEOMETRY_OUTPUT_VARYINGS$", geometryCtx.OutputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_GEOMETRY_COPY_VARYINGS$", geometryCtx.CopyVaryingsCode );

        pDef->AddShader( "$SHADOWMAP_PASS_FRAGMENT_INPUT_VARYINGS$", shadowCtx.InputVaryingsCode );
        pDef->AddShader( "$SHADOWMAP_PASS_FRAGMENT_SAMPLERS$", SamplersString( InGraph, shadowCtx.MaxTextureSlot ) );
        pDef->AddShader( "$SHADOWMAP_PASS_FRAGMENT_CODE$", shadowCtx.SourceCode );
    }

    // Create color pass
    {
        AMaterialBuildContext vertexCtx( InGraph, VERTEX_STAGE );
        AMaterialBuildContext tessControlCtx( InGraph, TESSELLATION_CONTROL_STAGE );
        AMaterialBuildContext tessEvalCtx( InGraph, TESSELLATION_EVAL_STAGE );
        AMaterialBuildContext fragmentCtx( InGraph, FRAGMENT_STAGE );
        SMaterialStageTransition trans;

        InGraph->CompileStage( vertexCtx );
        InGraph->CompileStage( fragmentCtx );

        bool bTess = InGraph->TessellationMethod != TESSELLATION_DISABLED;

        if ( bTess ) {
            InGraph->CompileStage( tessControlCtx );
            InGraph->CompileStage( tessEvalCtx );
        }

        InGraph->CreateStageTransitions( trans,
                                         &vertexCtx,
                                         bTess ? &tessControlCtx : nullptr,
                                         bTess ? &tessEvalCtx : nullptr,
                                         nullptr,
                                         &fragmentCtx );

        bHasVertexDeform = vertexCtx.bHasVertexDeform;
        bColorPassTextureFetch = trans.bHasTextures;
        maxTextureSlot = Math::Max( maxTextureSlot, trans.MaxTextureSlot );
        maxUniformAddress = Math::Max( maxUniformAddress, trans.MaxUniformAddress );

        int locationIndex = trans.Varyings.Size();

        predefines += "#define COLOR_PASS_VARYING_BAKED_LIGHT " + Math::ToString( locationIndex++ ) + "\n";
        predefines += "#define COLOR_PASS_VARYING_TANGENT "     + Math::ToString( locationIndex++ )    + "\n";
        predefines += "#define COLOR_PASS_VARYING_BINORMAL "    + Math::ToString( locationIndex++ )   + "\n";
        predefines += "#define COLOR_PASS_VARYING_NORMAL "      + Math::ToString( locationIndex++ )     + "\n";
        predefines += "#define COLOR_PASS_VARYING_POSITION "    + Math::ToString( locationIndex++ )   + "\n";
        predefines += "#define COLOR_PASS_VARYING_VERTEX_POSITION_CURRENT " + Math::ToString( locationIndex++ )   + "\n";
        predefines += "#define COLOR_PASS_VARYING_VERTEX_POSITION_PREVIOUS " + Math::ToString( locationIndex++ )   + "\n";

        if ( InGraph->bUseVirtualTexture ) {
            predefines += "#define COLOR_PASS_VARYING_VT_TEXCOORD "    + Math::ToString( locationIndex++ )   + "\n";
        }

        lightmapSlot = fragmentCtx.MaxTextureSlot + 1;

        predefines += "#define COLOR_PASS_TEXTURE_LIGHTMAP " + Math::ToString( lightmapSlot ) + "\n";

        if ( fragmentCtx.bParallaxSampler ) {
            switch ( InGraph->ParallaxTechnique ) {
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
                AN_ASSERT( 0 );
                break;
            }
        } else {
            predefines += "#define PARALLAX_TECHNIQUE PARALLAX_TECHNIQUE_DISABLED\n";
        }

        pDef->AddShader( "$COLOR_PASS_VERTEX_OUTPUT_VARYINGS$", vertexCtx.OutputVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_VERTEX_SAMPLERS$", SamplersString( InGraph, vertexCtx.MaxTextureSlot ) );
        pDef->AddShader( "$COLOR_PASS_VERTEX_CODE$", vertexCtx.SourceCode );

        pDef->AddShader( "$COLOR_PASS_TCS_INPUT_VARYINGS$", tessControlCtx.InputVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_TCS_OUTPUT_VARYINGS$", tessControlCtx.OutputVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_TCS_SAMPLERS$", SamplersString( InGraph, tessControlCtx.MaxTextureSlot ) );
        pDef->AddShader( "$COLOR_PASS_TCS_COPY_VARYINGS$", tessControlCtx.CopyVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_TCS_CODE$", tessControlCtx.SourceCode );

        pDef->AddShader( "$COLOR_PASS_TES_INPUT_VARYINGS$", tessEvalCtx.InputVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_TES_OUTPUT_VARYINGS$", tessEvalCtx.OutputVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_TES_SAMPLERS$", SamplersString( InGraph, tessEvalCtx.MaxTextureSlot ) );
        pDef->AddShader( "$COLOR_PASS_TES_INTERPOLATE$", tessEvalCtx.CopyVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_TES_CODE$", tessEvalCtx.SourceCode );

        pDef->AddShader( "$COLOR_PASS_FRAGMENT_INPUT_VARYINGS$", fragmentCtx.InputVaryingsCode );
        pDef->AddShader( "$COLOR_PASS_FRAGMENT_SAMPLERS$", SamplersString( InGraph, fragmentCtx.MaxTextureSlot ) );
        pDef->AddShader( "$COLOR_PASS_FRAGMENT_CODE$", fragmentCtx.SourceCode );
    }

    // Create wireframe pass
    {
        AMaterialBuildContext vertexCtx( InGraph, VERTEX_STAGE );
        AMaterialBuildContext tessControlCtx( InGraph, TESSELLATION_CONTROL_STAGE );
        AMaterialBuildContext tessEvalCtx( InGraph, TESSELLATION_EVAL_STAGE );
        AMaterialBuildContext geometryCtx( InGraph, GEOMETRY_STAGE );
        SMaterialStageTransition trans;

        InGraph->CompileStage( vertexCtx );

        bool bTess = InGraph->TessellationMethod != TESSELLATION_DISABLED;

        if ( bTess ) {
            InGraph->CompileStage( tessControlCtx );
            InGraph->CompileStage( tessEvalCtx );
        }

        InGraph->CreateStageTransitions( trans,
                                         &vertexCtx,
                                         bTess ? &tessControlCtx : nullptr,
                                         bTess ? &tessEvalCtx : nullptr,
                                         &geometryCtx,
                                         nullptr );

        pDef->bWireframePassTextureFetch = trans.bHasTextures;
        maxTextureSlot = Math::Max( maxTextureSlot, trans.MaxTextureSlot );
        maxUniformAddress = Math::Max( maxUniformAddress, trans.MaxUniformAddress );

        int locationIndex = trans.Varyings.Size();

        predefines += "#define WIREFRAME_PASS_VARYING_POSITION "    + Math::ToString( locationIndex++ ) + "\n";
        predefines += "#define WIREFRAME_PASS_VARYING_NORMAL "      + Math::ToString( locationIndex++ ) + "\n";

        pDef->AddShader( "$WIREFRAME_PASS_VERTEX_OUTPUT_VARYINGS$", vertexCtx.OutputVaryingsCode );
        pDef->AddShader( "$WIREFRAME_PASS_VERTEX_SAMPLERS$", SamplersString( InGraph, vertexCtx.MaxTextureSlot ) );
        pDef->AddShader( "$WIREFRAME_PASS_VERTEX_CODE$", vertexCtx.SourceCode );

        pDef->AddShader( "$WIREFRAME_PASS_TCS_INPUT_VARYINGS$", tessControlCtx.InputVaryingsCode );
        pDef->AddShader( "$WIREFRAME_PASS_TCS_OUTPUT_VARYINGS$", tessControlCtx.OutputVaryingsCode );
        pDef->AddShader( "$WIREFRAME_PASS_TCS_SAMPLERS$", SamplersString( InGraph, tessControlCtx.MaxTextureSlot ) );
        pDef->AddShader( "$WIREFRAME_PASS_TCS_COPY_VARYINGS$", tessControlCtx.CopyVaryingsCode );
        pDef->AddShader( "$WIREFRAME_PASS_TCS_CODE$", tessControlCtx.SourceCode );

        pDef->AddShader( "$WIREFRAME_PASS_TES_INPUT_VARYINGS$", tessEvalCtx.InputVaryingsCode );
        //pDef->AddShader( "$WIREFRAME_PASS_TES_OUTPUT_VARYINGS$", tessEvalCtx.OutputVaryingsCode );
        pDef->AddShader( "$WIREFRAME_PASS_TES_SAMPLERS$", SamplersString( InGraph, tessEvalCtx.MaxTextureSlot ) );
        pDef->AddShader( "$WIREFRAME_PASS_TES_INTERPOLATE$", tessEvalCtx.CopyVaryingsCode );
        pDef->AddShader( "$WIREFRAME_PASS_TES_CODE$", tessEvalCtx.SourceCode );

        //pDef->AddShader( "$WIREFRAME_PASS_GEOMETRY_INPUT_VARYINGS$", geometryCtx.InputVaryingsCode );
        //pDef->AddShader( "$WIREFRAME_PASS_GEOMETRY_OUTPUT_VARYINGS$", geometryCtx.OutputVaryingsCode );
        //pDef->AddShader( "$WIREFRAME_PASS_GEOMETRY_COPY_VARYINGS$", geometryCtx.CopyVaryingsCode );
    }

    // Create normals debugging pass
    {
        // Normals pass. Vertex stage
        AMaterialBuildContext vertexCtx( InGraph, VERTEX_STAGE );
        InGraph->CompileStage( vertexCtx );

        pDef->bNormalsPassTextureFetch = vertexCtx.bHasTextures;

        maxTextureSlot = Math::Max( maxTextureSlot, vertexCtx.MaxTextureSlot );
        maxUniformAddress = Math::Max( maxUniformAddress, vertexCtx.MaxUniformAddress );

        pDef->AddShader( "$NORMALS_PASS_VERTEX_SAMPLERS$", SamplersString( InGraph, vertexCtx.MaxTextureSlot ) );
        pDef->AddShader( "$NORMALS_PASS_VERTEX_CODE$", vertexCtx.SourceCode );
    }

    if ( bHasVertexDeform ) {
        predefines += "#define HAS_VERTEX_DEFORM\n";
    }

    pDef->AddShader( "$PREDEFINES$", predefines );

    pDef->Type = InGraph->MaterialType;
    pDef->Blending = InGraph->Blending;
    pDef->TessellationMethod = InGraph->TessellationMethod;
    pDef->LightmapSlot = lightmapSlot;
    pDef->bDepthPassTextureFetch = bDepthPassTextureFetch;
    pDef->bColorPassTextureFetch = bColorPassTextureFetch;
    pDef->bShadowMapPassTextureFetch = bShadowMapPassTextureFetch;
    pDef->bHasVertexDeform = bHasVertexDeform;
    pDef->bDepthTest_EXPERIMENTAL = InGraph->bDepthTest;
    pDef->bNoCastShadow = bNoCastShadow;
    pDef->bShadowMapMasking = bShadowMapMasking;
    pDef->bDisplacementAffectShadow = InGraph->bDisplacementAffectShadow;
    pDef->bTranslucent = InGraph->bTranslucent;
    pDef->NumUniformVectors = maxUniformAddress + 1;
    pDef->NumSamplers = maxTextureSlot + 1;

    for ( int i = 0 ; i < pDef->NumSamplers ; i++ ) {
        pDef->Samplers[i] = InGraph->GetTextureSlots()[i]->SamplerDesc;
    }

#if 0
    WriteDebugShaders( pDef->Shaders );
#endif
}
