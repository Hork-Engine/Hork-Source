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

#include <Engine/MaterialGraph/Public/MaterialGraph.h>
#include <Engine/Core/Public/Logger.h>

static constexpr const char * AssemblyTypeStr[] = {
    "vec4",     // AT_Unknown
    "float",    // AT_Float1
    "vec2",     // AT_Float2
    "vec3",     // AT_Float3
    "vec4"      // AT_Float4
};

static FString EvaluateVectorCast( FString const & _Expression, EMGNodeType _TypeFrom, EMGNodeType _TypeTo, float _DefX, float _DefY, float _DefZ, float _DefW ) {

    if ( _TypeFrom == _TypeTo || _TypeTo == AT_Unknown ) {
        return _Expression;
    }

    switch( _TypeFrom ) {
    case AT_Unknown:
        switch ( _TypeTo ) {
        case AT_Float1:
            return FMath::ToString( _DefX );
        case AT_Float2:
            return "vec2( " + FMath::ToString( _DefX ) + ", " + FMath::ToString( _DefY ) + " )";
        case AT_Float3:
            return "vec3( " + FMath::ToString( _DefX ) + ", " + FMath::ToString( _DefY ) + ", " + FMath::ToString( _DefZ ) + " )";
        case AT_Float4:
            return "vec4( " + FMath::ToString( _DefX ) + ", " + FMath::ToString( _DefY ) + ", " + FMath::ToString( _DefZ ) + ", " + FMath::ToString( _DefW ) + " )";
        default:
            break;
        }
        break;
    case AT_Float1:
        switch ( _TypeTo ) {
        case AT_Float2:
            return "vec2( " + _Expression + " )";
        case AT_Float3:
            return "vec3( " + _Expression + " )";
        case AT_Float4:
            return "vec4( " + _Expression + " )";
        default:
            break;
        }
        break;
    case AT_Float2:
        switch ( _TypeTo ) {
        case AT_Float1:
            return _Expression + ".x";
        case AT_Float3:
            return "vec3( " + _Expression + ", " + FMath::ToString( _DefZ ) + " )";
        case AT_Float4:
            return "vec4( " + _Expression + ", " + FMath::ToString( _DefZ ) + ", " + FMath::ToString( _DefW ) + " )";
        default:
            break;
        }
        break;
    case AT_Float3:
        switch ( _TypeTo ) {
        case AT_Float1:
            return _Expression + ".x";
        case AT_Float2:
            return _Expression + ".xy";
        case AT_Float4:
            return "vec4( " + _Expression + ", " + FMath::ToString( _DefW ) + " )";
        default:
            break;
        }
        break;
    case AT_Float4:
        switch ( _TypeTo ) {
        case AT_Float1:
            return _Expression + ".x";
        case AT_Float2:
            return _Expression + ".xy";
        case AT_Float3:
            return _Expression + ".xyz";
        default:
            break;
        }
        break;
    }

    AN_Assert(0);

    return _Expression;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int FMaterialBuildContext::BuildSerial = 1;

FString FMaterialBuildContext::GenerateVariableName() const {
    return FString( "v" ) + Int( VariableName++ ).ToString();
}

void FMaterialBuildContext::GenerateSourceCode( MGNodeOutput * _Slot, FString const & _Expression, bool _AddBrackets ) {
    if ( _Slot->Usages[Stage] > 1 ) {
        _Slot->Expression = GenerateVariableName();
        SourceCode += "const " + FString( AssemblyTypeStr[ _Slot->Type ] ) + " " + _Slot->Expression + " = " + _Expression + ";\n";
    } else {
        if ( _AddBrackets ) {
            _Slot->Expression = "( " + _Expression + " )";
        } else {
            _Slot->Expression = _Expression;
        }
    }
}

void FMaterialBuildContext::SetStage( EMaterialStage _Stage ) {
    VariableName = 0;
    Stage = _Stage;
    SourceCode.Clear();
    bHasTextures = false;
    MaxTextureSlot = -1;
    MaxUniformAddress = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGNodeOutput )

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGNodeInput )

MGNodeInput::MGNodeInput() {
}

void MGNodeInput::Connect( MGNode * _Block, const char * _Slot ) {
    Block = _Block;
    Slot = _Slot;
}

void MGNodeInput::Disconnect() {
    Block = nullptr;
    Slot.Clear();
}

MGNodeOutput * MGNodeInput::GetConnection() {
    if ( !Block ) {
        return nullptr;
    }

    MGNodeOutput * out = Block->FindOutput( Slot.ToConstChar() );
    //out->Usages++;

    return out;
}

int MGNodeInput::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( Name ).ToConstChar() );

    if ( Block ) {
        _Doc.AddStringField( object, "Slot", _Doc.ProxyBuffer.NewString( Slot ).ToConstChar() );
        _Doc.AddStringField( object, "Block", _Doc.ProxyBuffer.NewString( Block->GetGUID().ToString() ).ToConstChar() );
    }

    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGNextStageVariable )

void MGNextStageVariable::Connect( MGNode * _Block, const char * _Slot ) {
    Block = _Block;
    Slot = _Slot;
}

void MGNextStageVariable::Disconnect() {
    Block = nullptr;
    Slot.Clear();
}

MGNodeOutput * MGNextStageVariable::GetConnection() {
    if ( !Block ) {
        return nullptr;
    }

    return Block->FindOutput( Slot.ToConstChar() );
}

int MGNextStageVariable::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( Name ).ToConstChar() );

    if ( Block ) {
        _Doc.AddStringField( object, "Slot", _Doc.ProxyBuffer.NewString( Slot ).ToConstChar() );
        _Doc.AddStringField( object, "Block", _Doc.ProxyBuffer.NewString( Block->GetGUID().ToString() ).ToConstChar() );
    }

    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGNode )
AN_ATTRIBUTE_( Location, AF_DEFAULT )
AN_END_CLASS_META()

MGNode::MGNode() {
    GUID.Generate();
}

MGNode::~MGNode() {
    for ( MGNodeInput * In : Inputs ) {
        In->RemoveRef();
    }

    for ( MGNodeOutput * Out : Outputs ) {
        Out->RemoveRef();
    }
}

MGNodeInput * MGNode::AddInput( const char * _Name ) {
    MGNodeInput * In = NewObject< MGNodeInput >();
    In->AddRef();
    In->SetName( _Name );
    Inputs.Append( In );
    return In;
}

MGNodeOutput * MGNode::AddOutput( const char * _Name, EMGNodeType _Type ) {
    MGNodeOutput * Out = NewObject< MGNodeOutput >();
    Out->AddRef();
    Out->SetName( _Name );
    Out->Type = _Type;
    Outputs.Append( Out );
    return Out;
}

MGNodeOutput * MGNode::FindOutput( const char * _Name ) {
    for ( MGNodeOutput * Out : Outputs ) {
        if ( !Out->GetName().Cmp( _Name ) ) {
            return Out;
        }
    }
    MGMaterialStage * stageBlock = dynamic_cast< MGMaterialStage * >( this );
    if ( stageBlock ) {
        for ( MGNextStageVariable * Out : stageBlock->NextStageVariables ) {
            if ( !Out->GetName().Cmp( _Name ) ) {
                return Out;
            }
        }
    }
    return nullptr;
}

bool MGNode::Build( FMaterialBuildContext & _Context ) {
    if ( Serial == _Context.GetBuildSerial() ) {
        return true;
    }

    if ( !(Stages & _Context.GetStageMask() ) ) {
        return false;
    }

    Serial = _Context.GetBuildSerial();

    Compute( _Context );
    return true;
}

void MGNode::ResetConnections( FMaterialBuildContext const & _Context ) {
    if ( !bTouched ) {
        return;
    }

    bTouched = false;

    for ( MGNodeInput * in : Inputs ) {
        MGNodeOutput * out = in->GetConnection();
        if ( out ) {
            MGNode * block = in->ConnectedBlock();
            block->ResetConnections( _Context );
            out->Usages[_Context.GetStage()] = 0;
        }
    }
}

void MGNode::TouchConnections( FMaterialBuildContext const & _Context ) {
    if ( bTouched ) {
        return;
    }

    bTouched = true;

    for ( MGNodeInput * in : Inputs ) {
        MGNodeOutput * out = in->GetConnection();
        if ( out ) {
            MGNode * block = in->ConnectedBlock();
            block->TouchConnections( _Context );
            out->Usages[_Context.GetStage()]++;
        }
    }
}

int MGNode::Serialize( FDocument & _Doc ) {
#if 0
    int object = _Doc.CreateObjectValue();
    _Doc.AddStringField( object, "ClassName", FinalClassName() );
#else
    int object = Super::Serialize(_Doc);
#endif

    _Doc.AddStringField( object, "GUID", _Doc.ProxyBuffer.NewString( GUID.ToString() ).ToConstChar() );

    if ( !Inputs.IsEmpty() ) {
        int array = _Doc.AddArray( object, "Inputs" );
        for ( MGNodeInput * in : Inputs ) {
            int inputObject = in->Serialize( _Doc );
            _Doc.AddValueToField( array, inputObject );
        }
    }
    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMaterialStage )

MGMaterialStage::~MGMaterialStage() {
    for ( MGNextStageVariable * nsv : NextStageVariables ) {
        nsv->RemoveRef();
    }
}

MGNextStageVariable * MGMaterialStage::AddNextStageVariable( const char * _Name, EMGNodeType _Type ) {
    if ( FindOutput( _Name ) ) {
        return nullptr;
    }

    MGNextStageVariable * nsv = NewObject< MGNextStageVariable >();
    nsv->AddRef();
    nsv->SetName( _Name );
    nsv->Expression = "nsv_" + NsvPrefix + Int(NextStageVariables.Size()).ToString() + "_" + nsv->GetName();
    nsv->Type = _Type;
    NextStageVariables.Append( nsv );

    return nsv;
}

MGNextStageVariable * MGMaterialStage::FindNextStageVariable( const char * _Name ) {
    for ( MGNextStageVariable * Out : NextStageVariables ) {
        if ( !Out->GetName().Cmp( _Name ) ) {
            return Out;
        }
    }
    return nullptr;
}

FString MGMaterialStage::NSV_OutputSection() {
    FString s;
    uint32_t location = 0;
    for ( MGNextStageVariable * nsv : NextStageVariables ) {
        s += "layout( location = " + UInt(location++).ToString() + " ) out " + AssemblyTypeStr[nsv->Type] + " " + nsv->Expression + ";\n";
    }
    return s;
}

FString MGMaterialStage::NSV_InputSection() {
    FString s;
    uint32_t location = 0;
    for ( MGNextStageVariable * nsv : NextStageVariables ) {
        s += "layout( location = " + UInt(location++).ToString() + " ) in " + AssemblyTypeStr[nsv->Type] + " " + nsv->Expression + ";\n";
    }
    return s;
}

int MGMaterialStage::Serialize( FDocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    if ( !NextStageVariables.IsEmpty() ) {
        int array = _Doc.AddArray( object, "NSV" );
        for ( MGNextStageVariable * nsv : NextStageVariables ) {
            int nsvObject = nsv->Serialize( _Doc );
            _Doc.AddValueToField( array, nsvObject );
        }
    }

    return object;
}

void MGMaterialStage::Compute( FMaterialBuildContext & _Context ) {
    for ( MGNextStageVariable * nsv : NextStageVariables ) {

        MGNodeOutput * connection = nsv->GetConnection();

        FString const & nsvName = nsv->Expression;

//        if ( _Context.GetMaterialPass() != MATERIAL_PASS_COLOR ) {
//            _Context.SourceCode += FString( AssemblyTypeStr[nsv->Type] ) + " ";
//        }

        if ( connection && nsv->ConnectedBlock()->Build( _Context ) ) {

            if ( nsv->Type == connection->Type ) {
                _Context.SourceCode += nsvName + " = " + connection->Expression + ";\n";
            } else {
                switch ( nsv->Type ) {
                case AT_Float1:
                    _Context.SourceCode += nsvName + " = " + connection->Expression + ".x;\n";
                    break;
                case AT_Float2:
                    _Context.SourceCode += nsvName + " = vec2( " + connection->Expression + " );\n";
                    break;
                case AT_Float3:
                    _Context.SourceCode += nsvName + " = vec3( " + connection->Expression + " );\n";
                    break;
                case AT_Float4:
                    _Context.SourceCode += nsvName + " = vec4( " + connection->Expression + " );\n";
                    break;
                default:
                    GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );
                    break;
                }
            }
        } else {
            switch ( nsv->Type ) {
            case AT_Float1:
                _Context.SourceCode += nsvName + " = 0.0;\n";
                break;
            case AT_Float2:
                _Context.SourceCode += nsvName + " = vec2( 0.0 );\n";
                break;
            case AT_Float3:
                _Context.SourceCode += nsvName + " = vec3( 0.0 );\n";
                break;
            case AT_Float4:
                _Context.SourceCode += nsvName + " = vec4( 0.0 );\n";
                break;
            default:
                GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGVertexStage )

MGVertexStage::MGVertexStage() {
    Name = "Material Vertex Stage";
    Stages = VERTEX_STAGE_BIT;
    NsvPrefix = "VS";

    Position = AddInput( "Position" );
}

MGVertexStage::~MGVertexStage() {

}

void MGVertexStage::Compute( FMaterialBuildContext & _Context ) {

    if ( _Context.GetMaterialPass() == MATERIAL_PASS_COLOR ) {

        // Super class adds nsv_ definition. Currently nsv_ variables supported only for MATERIAL_PASS_COLOR.
        Super::Compute( _Context );
    }

    MGNodeOutput * positionCon = Position->GetConnection();

    bool bValid = true;

    bHasVertexDeform = false;

    FString TransformMatrix;
    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
        TransformMatrix = "OrthoProjection";
    } else {
        TransformMatrix = "TransformMatrix";
    }

    if ( positionCon && Position->ConnectedBlock()->Build( _Context ) ) {

        if ( positionCon->Expression != "GetVertexPosition()" ) {
            bHasVertexDeform = true;
        }

        switch( positionCon->Type ) {
        case AT_Float1:
            _Context.SourceCode += "gl_Position = " + TransformMatrix + " * vec4(" + positionCon->Expression + ", 0.0, 0.0, 1.0 );\n";
            break;
        case AT_Float2:
            _Context.SourceCode += "gl_Position = " + TransformMatrix + " * vec4(" + positionCon->Expression + ", 0.0, 1.0 );\n";
            break;
        case AT_Float3:
            _Context.SourceCode += "gl_Position = " + TransformMatrix + " * vec4(" + positionCon->Expression + ", 1.0 );\n";
            break;
        case AT_Float4:
            _Context.SourceCode += "gl_Position = " + TransformMatrix + " * (" + positionCon->Expression + ");\n";
            break;
        default:
            bValid = false;
            break;
        }

    } else {
        bValid = false;
    }

    if ( !bValid ) {
        //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

        _Context.SourceCode += "gl_Position = " + TransformMatrix + " * vec4( GetVertexPosition(), 1.0 );\n";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGFragmentStage )

MGFragmentStage::MGFragmentStage() {
    Name = "Material Fragment Stage";
    Stages = FRAGMENT_STAGE_BIT;
    NsvPrefix = "FS";

    Color = AddInput( "Color" );
    Normal = AddInput( "Normal" );
    Metallic = AddInput( "Metallic" );
    Roughness = AddInput( "Roughness" );
    Ambient = AddInput( "Ambient" );
    Emissive = AddInput( "Emissive" );
}

MGFragmentStage::~MGFragmentStage() {

}

void MGFragmentStage::Compute( FMaterialBuildContext & _Context ) {

    Super::Compute( _Context );

    // Color
    {
        MGNodeOutput * colorCon = Color->GetConnection();

        bool bValid = true;

        if ( colorCon && Color->ConnectedBlock()->Build( _Context ) ) {

            switch( colorCon->Type ) {
            case AT_Float1:
                _Context.SourceCode += "vec4 BaseColor = vec4(" + colorCon->Expression + ", 0.0, 0.0, 1.0 );\n";
                break;
            case AT_Float2:
                _Context.SourceCode += "vec4 BaseColor = vec4(" + colorCon->Expression + ", 0.0, 1.0 );\n";
                break;
            case AT_Float3:
                _Context.SourceCode += "vec4 BaseColor = vec4(" + colorCon->Expression + ", 1.0 );\n";
                break;
            case AT_Float4:
                _Context.SourceCode += "vec4 BaseColor = " + colorCon->Expression + ";\n";
                break;
            default:
                bValid = false;
                break;
            }

        } else {
            bValid = false;
        }

        if ( !bValid ) {
            //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

            _Context.SourceCode += "vec4 BaseColor = vec4(1);\n";
        }
    }


    if ( _Context.GetMaterialType() == MATERIAL_TYPE_PBR || _Context.GetMaterialType() == MATERIAL_TYPE_BASELIGHT ) {
        // Normal
        {
            MGNodeOutput * normalCon = Normal->GetConnection();

            bool bValid = true;

            if ( normalCon && Normal->ConnectedBlock()->Build( _Context ) ) {

                switch ( normalCon->Type ) {
                case AT_Float3:
                    _Context.SourceCode += "vec3 MaterialNormal = " + normalCon->Expression + ";\n";
                    break;
                case AT_Float4:
                    _Context.SourceCode += "vec3 MaterialNormal = vec3(" + normalCon->Expression + ");\n";
                    break;
                default:
                    bValid = false;
                    break;
                }

            } else {
                bValid = false;
            }

            if ( !bValid ) {
                //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

                _Context.SourceCode += "vec3 MaterialNormal = vec3(0,0,1);\n";
            }
        }

        // Emissive
        {
            MGNodeOutput * emissiveCon = Emissive->GetConnection();

            bool bValid = true;

            if ( emissiveCon && Emissive->ConnectedBlock()->Build( _Context ) ) {

                switch ( emissiveCon->Type ) {
                case AT_Float1:
                    _Context.SourceCode += "vec3 MaterialEmissive = vec3(" + emissiveCon->Expression + ", 0.0, 0.0 );\n";
                    break;
                case AT_Float2:
                    _Context.SourceCode += "vec3 MaterialEmissive = vec3(" + emissiveCon->Expression + ", 0.0 );\n";
                    break;
                case AT_Float3:
                    _Context.SourceCode += "vec3 MaterialEmissive = " + emissiveCon->Expression + ";\n";
                    break;
                case AT_Float4:
                    _Context.SourceCode += "vec3 MaterialEmissive = " + emissiveCon->Expression + ".xyz;\n";
                    break;
                default:
                    bValid = false;
                    break;
                }

            } else {
                bValid = false;
            }

            if ( !bValid ) {
                //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

                _Context.SourceCode += "vec3 MaterialEmissive = vec3(0);\n";
            }
        }
    }


    if ( _Context.GetMaterialType() == MATERIAL_TYPE_PBR ) {

        // Metallic
        {
            MGNodeOutput * metallicCon = Metallic->GetConnection();

            bool bValid = true;

            if ( metallicCon && Metallic->ConnectedBlock()->Build( _Context ) ) {

                switch ( metallicCon->Type ) {
                case AT_Float1:
                    _Context.SourceCode += "float MaterialMetallic = " + metallicCon->Expression + ";\n";
                    break;
                case AT_Float2:
                case AT_Float3:
                case AT_Float4:
                    _Context.SourceCode += "float MaterialMetallic = " + metallicCon->Expression + ".x;\n";
                    break;
                default:
                    bValid = false;
                    break;
                }

            } else {
                bValid = false;
            }

            if ( !bValid ) {
                //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

                _Context.SourceCode += "float MaterialMetallic = 0;\n";
            }
        }

        // Roughness
        {
            MGNodeOutput * roughnessCon = Roughness->GetConnection();

            bool bValid = true;

            if ( roughnessCon && Roughness->ConnectedBlock()->Build( _Context ) ) {

                switch ( roughnessCon->Type ) {
                case AT_Float1:
                    _Context.SourceCode += "float MaterialRoughness = " + roughnessCon->Expression + ";\n";
                    break;
                case AT_Float2:
                case AT_Float3:
                case AT_Float4:
                    _Context.SourceCode += "float MaterialRoughness = " + roughnessCon->Expression + ".x;\n";
                    break;
                default:
                    bValid = false;
                    break;
                }

            } else {
                bValid = false;
            }

            if ( !bValid ) {
                //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

                _Context.SourceCode += "float MaterialRoughness = 1;\n";
            }
        }

        // Ambient
        {
            MGNodeOutput * ambientCon = Ambient->GetConnection();

            bool bValid = true;

            if ( ambientCon && Ambient->ConnectedBlock()->Build( _Context ) ) {

                switch ( ambientCon->Type ) {
                case AT_Float1:
                    _Context.SourceCode += "float MaterialAmbient = " + ambientCon->Expression + ";\n";
                    break;
                case AT_Float2:
                case AT_Float3:
                case AT_Float4:
                    _Context.SourceCode += "float MaterialAmbient = " + ambientCon->Expression + ".x;\n";
                    break;
                default:
                    bValid = false;
                    break;
                }

            } else {
                bValid = false;
            }

            if ( !bValid ) {
                //GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

                _Context.SourceCode += "float MaterialAmbient = 1;\n";
            }
        }

    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGShadowCastStage )

MGShadowCastStage::MGShadowCastStage() {
    Name = "Material Shadow Cast Stage";
    Stages = SHADOWCAST_STAGE_BIT;
    NsvPrefix = "FS";

    ShadowMask = AddInput( "ShadowMask" );
}

MGShadowCastStage::~MGShadowCastStage() {

}

void MGShadowCastStage::Compute( FMaterialBuildContext & _Context ) {

    Super::Compute( _Context );

    // Shadow Mask
    {
        MGNodeOutput * con = ShadowMask->GetConnection();

        if ( con && ShadowMask->ConnectedBlock()->Build( _Context ) ) {
            switch( con->Type ) {
            case AT_Float1:
                _Context.SourceCode += "if ( " + con->Expression + " <= 0.0 ) discard;\n";
                break;
            case AT_Float2:
            case AT_Float3:
            case AT_Float4:
                _Context.SourceCode += "if ( " + con->Expression + ".x <= 0.0 ) discard;\n";
                break;
            default:
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGProjectionNode )

MGProjectionNode::MGProjectionNode() {
    Name = "Projection";
    Stages = VERTEX_STAGE_BIT;

    Vector = AddInput( "Vector" );
    Result = AddOutput( "Result", AT_Float4 );
}

void MGProjectionNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * vectorCon = Vector->GetConnection();

    if ( vectorCon && Vector->ConnectedBlock()->Build( _Context ) ) {
        switch( vectorCon->Type ) {
        case AT_Float1:
            _Context.GenerateSourceCode( Result, "TransformMatrix * vec4( " + vectorCon->Expression + ", 0.0, 0.0, 1.0 )", true );
            break;
        case AT_Float2:
            _Context.GenerateSourceCode( Result, "TransformMatrix * vec4( " + vectorCon->Expression + ", 0.0, 1.0 )", true );
            break;
        case AT_Float3:
            _Context.GenerateSourceCode( Result, "TransformMatrix * vec4( " + vectorCon->Expression + ", 1.0 )", true );
            break;
        case AT_Float4:
            _Context.GenerateSourceCode( Result, "TransformMatrix * " + vectorCon->Expression, true );
            break;
        default:
            _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
            break;
        }
    } else {
        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }    
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGLengthNode )

MGLengthNode::MGLengthNode() {
    Name = "Length";
    Stages = ANY_STAGE_BIT;

    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Float1 );
}

void MGLengthNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        if ( inputConnection->Type == AT_Float1 ) {
            _Context.GenerateSourceCode( Result, inputConnection->Expression, false );
        } else {
            _Context.GenerateSourceCode( Result, "length( " + inputConnection->Expression + " )", false );
        }
    } else {
        Result->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGNormalizeNode )

MGNormalizeNode::MGNormalizeNode() {
    Name = "Normalize";
    Stages = ANY_STAGE_BIT;

    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGNormalizeNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;
        if ( inputConnection->Type == AT_Float1 ) {
            Result->Expression = "1.0";
        } else {
            _Context.GenerateSourceCode( Result, "normalize( " + inputConnection->Expression + " )", false );
        }
    } else {
        Result->Type = AT_Float4;
        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGDecomposeVectorNode )

MGDecomposeVectorNode::MGDecomposeVectorNode() {
    Name = "Decompose Vector";
    Stages = ANY_STAGE_BIT;

    Vector = AddInput( "Vector" );
    X = AddOutput( "X", AT_Float1 );
    Y = AddOutput( "Y", AT_Float1 );
    Z = AddOutput( "Z", AT_Float1 );
    W = AddOutput( "W", AT_Float1 );
}

void MGDecomposeVectorNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Vector->GetConnection();

    if ( inputConnection && Vector->ConnectedBlock()->Build( _Context ) ) {

        switch ( inputConnection->Type ) {
        case AT_Float1:
            _Context.GenerateSourceCode( X, inputConnection->Expression, false );
            //X->Expression = temp;
            Y->Expression = "0.0";
            Z->Expression = "0.0";
            W->Expression = "0.0";
            break;
        case AT_Float2:
            {
            FString temp = "temp_" + _Context.GenerateVariableName();
            _Context.SourceCode += "const " + FString(AssemblyTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
            X->Expression = temp + ".x";
            Y->Expression = temp + ".y";
            Z->Expression = "0.0";
            W->Expression = "0.0";
            }
            break;
        case AT_Float3:
            {
            FString temp = "temp_" + _Context.GenerateVariableName();
            _Context.SourceCode += "const " + FString(AssemblyTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
            X->Expression = temp + ".x";
            Y->Expression = temp + ".y";
            Z->Expression = temp + ".z";
            W->Expression = "0.0";
            }
            break;
        case AT_Float4:
            {
            FString temp = "temp_" + _Context.GenerateVariableName();
            _Context.SourceCode += "const " + FString(AssemblyTypeStr[inputConnection->Type]) + " " + temp + " = " + inputConnection->Expression + ";\n";
            X->Expression = temp + ".x";
            Y->Expression = temp + ".y";
            Z->Expression = temp + ".z";
            W->Expression = temp + ".w";
            }
            break;
        default:
            X->Expression = "0.0";
            Y->Expression = "0.0";
            Z->Expression = "0.0";
            W->Expression = "0.0";
            break;
        }

    } else {
        X->Expression = "0.0";
        Y->Expression = "0.0";
        Z->Expression = "0.0";
        W->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMakeVectorNode )

MGMakeVectorNode::MGMakeVectorNode() {
    Name = "Make Vector";
    Stages = ANY_STAGE_BIT;

    X = AddInput( "X" );
    Y = AddInput( "Y" );
    Z = AddInput( "Z" );
    W = AddInput( "W" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGMakeVectorNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * XConnection = X->GetConnection();
    MGNodeOutput * YConnection = Y->GetConnection();
    MGNodeOutput * ZConnection = Z->GetConnection();
    MGNodeOutput * WConnection = W->GetConnection();

    bool XValid = XConnection && X->ConnectedBlock()->Build( _Context ) && XConnection->Type == AT_Float1;
    bool YValid = YConnection && Y->ConnectedBlock()->Build( _Context ) && YConnection->Type == AT_Float1;
    bool ZValid = ZConnection && Z->ConnectedBlock()->Build( _Context ) && ZConnection->Type == AT_Float1;
    bool WValid = WConnection && W->ConnectedBlock()->Build( _Context ) && WConnection->Type == AT_Float1;

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
        Result->Type = AT_Float1;
        _Context.GenerateSourceCode( Result, XConnection->Expression, false );
        return;
    }

    Result->Type = EMGNodeType( AT_Float1 + numComponents - 1 );

    switch ( Result->Type ) {
    case AT_Float2:

        _Context.GenerateSourceCode( Result,
                  "vec2( "
                + (XValid ? XConnection->Expression : FString("0.0"))
                + ", "
                + (YValid ? YConnection->Expression : FString("0.0"))
                + " )",
                  false );
        break;

    case AT_Float3:
        _Context.GenerateSourceCode( Result,
                  "vec3( "
                + (XValid ? XConnection->Expression : FString("0.0"))
                + ", "
                + (YValid ? YConnection->Expression : FString("0.0"))
                + ", "
                + (ZValid ? ZConnection->Expression : FString("0.0"))
                + " )",
                  false );
        break;

    case AT_Float4:
        _Context.GenerateSourceCode( Result,
                  "vec4( "
                + (XValid ? XConnection->Expression : FString("0.0"))
                + ", "
                + (YValid ? YConnection->Expression : FString("0.0"))
                + ", "
                + (ZValid ? ZConnection->Expression : FString("0.0"))
                + ", "
                + (WValid ? WConnection->Expression : FString("0.0"))
                + " )",
                  false );
        break;
    default:
        AN_Assert( 0 );
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGNegateNode )

MGNegateNode::MGNegateNode() {
    Name = "Negate";
    Stages = ANY_STAGE_BIT;

    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGNegateNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;

        _Context.GenerateSourceCode( Result, "-" + inputConnection->Expression, true );
    } else {
        Result->Type = AT_Float1;
        Result->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGFractNode )

MGFractNode::MGFractNode() {
    Name = "Fract";
    Stages = ANY_STAGE_BIT;

    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGFractNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Value->GetConnection();

    FString expression;

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;

        expression = "fract( " + inputConnection->Expression + " )";
    } else {
        Result->Type = AT_Float4;

        expression = "vec4( 0.0 )";
    }

    _Context.GenerateSourceCode( Result, expression, false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGSinusNode )

MGSinusNode::MGSinusNode() {
    Name = "Sin";
    Stages = ANY_STAGE_BIT;

    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGSinusNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;
        _Context.GenerateSourceCode( Result, "sin( " + inputConnection->Expression + " )", false );
    } else {
        Result->Type = AT_Float4;
        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGCosinusNode )

MGCosinusNode::MGCosinusNode() {
    Name = "Cos";
    Stages = ANY_STAGE_BIT;

    Value = AddInput( "Value" );
    Result = AddOutput( "Result", AT_Unknown );
}

void MGCosinusNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;
        _Context.GenerateSourceCode( Result, "cos( " + inputConnection->Expression + " )", false );
    } else {
        Result->Type = AT_Float4;
        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGArithmeticNode )

MGArithmeticNode::MGArithmeticNode() {
    Stages = ANY_STAGE_BIT;

    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGArithmeticNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * connectionA = ValueA->GetConnection();
    MGNodeOutput * connectionB = ValueB->GetConnection();

    constexpr const char * table[] = { " + ", " - ", " * ", " / " };

    const char * op = table[ArithmeticOp];

    if ( connectionA && ValueA->ConnectedBlock()->Build( _Context )
         && connectionB && ValueB->ConnectedBlock()->Build( _Context ) ) {

        Result->Type = connectionA->Type;

        if ( connectionA->Type != connectionB->Type && connectionB->Type != AT_Float1 ) {
            _Context.GenerateSourceCode( Result, connectionA->Expression + op + EvaluateVectorCast( connectionB->Expression, connectionB->Type, Result->Type, 0,0,0,0 ), true );
        } else {
            _Context.GenerateSourceCode( Result, connectionA->Expression + op + connectionB->Expression, true );
        }

    } else {
        Result->Type = AT_Float4;

        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMulNode )
AN_CLASS_META( FMaterialDivBlock )
AN_CLASS_META( MGAddNode )
AN_CLASS_META( MGSubNode )

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMADNode )

MGMADNode::MGMADNode() {
    Name = "MAD A * B + C";
    Stages = ANY_STAGE_BIT;

    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );
    ValueC = AddInput( "C" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGMADNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * connectionA = ValueA->GetConnection();
    MGNodeOutput * connectionB = ValueB->GetConnection();
    MGNodeOutput * connectionC = ValueC->GetConnection();

    if ( connectionA && ValueA->ConnectedBlock()->Build( _Context )
         && connectionB && ValueB->ConnectedBlock()->Build( _Context )
         && connectionC && ValueC->ConnectedBlock()->Build( _Context ) ) {

        Result->Type = connectionA->Type;

        FString expression;

        if ( connectionA->Type != connectionB->Type && connectionB->Type != AT_Float1 ) {
            expression = connectionA->Expression + " * " + EvaluateVectorCast( connectionB->Expression, connectionB->Type, Result->Type, 0,0,0,0 ) + " + ";
        } else {
            expression = connectionA->Expression + " * " + connectionB->Expression + " + ";
        }

        if ( connectionA->Type != connectionC->Type && connectionC->Type != AT_Float1 ) {
            expression += EvaluateVectorCast( connectionC->Expression, connectionC->Type, Result->Type, 0,0,0,0 );
        } else {
            expression += connectionC->Expression;
        }

        _Context.GenerateSourceCode( Result, expression, true );

    } else {
        Result->Type = AT_Float4;

        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGStepNode )

MGStepNode::MGStepNode() {
    Name = "Step( A, B )";
    Stages = ANY_STAGE_BIT;

    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGStepNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * connectionA = ValueA->GetConnection();
    MGNodeOutput * connectionB = ValueB->GetConnection();

    FString expression;

    if ( connectionA && ValueA->ConnectedBlock()->Build( _Context )
         && connectionB && ValueB->ConnectedBlock()->Build( _Context ) ) {

        Result->Type = connectionA->Type;

        if ( connectionA->Type != connectionB->Type ) {
            expression = "step( " + connectionA->Expression + ", " + EvaluateVectorCast( connectionB->Expression, connectionB->Type, Result->Type, 0,0,0,0 ) + " )";
        } else {
            expression = "step( " + connectionA->Expression + ", " + connectionB->Expression + " )";
        }

    } else {
        Result->Type = AT_Float4;
        expression = "vec4(0.0)";
    }

    _Context.GenerateSourceCode( Result, expression, false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGPowNode )

MGPowNode::MGPowNode() {
    Name = "Pow A^B";
    Stages = ANY_STAGE_BIT;

    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGPowNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * connectionA = ValueA->GetConnection();
    MGNodeOutput * connectionB = ValueB->GetConnection();

    FString expression;

    if ( connectionA && ValueA->ConnectedBlock()->Build( _Context )
         && connectionB && ValueB->ConnectedBlock()->Build( _Context ) ) {

        Result->Type = connectionA->Type;

        if ( connectionA->Type != connectionB->Type ) {
            expression = "pow( " + connectionA->Expression + ", " + EvaluateVectorCast( connectionB->Expression, connectionB->Type, Result->Type, 0,0,0,0 ) + " )";
        } else {
            expression = "pow( " + connectionA->Expression + ", " + connectionB->Expression + " )";
        }

    } else {
        Result->Type = AT_Float4;

        expression = "vec4( 0.0 )";
    }

    _Context.GenerateSourceCode( Result, expression, false );
}
///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGLerpNode )

MGLerpNode::MGLerpNode() {
    Name = "Lerp( A, B, C )";
    Stages = ANY_STAGE_BIT;

    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );
    ValueC = AddInput( "C" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGLerpNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * connectionA = ValueA->GetConnection();
    MGNodeOutput * connectionB = ValueB->GetConnection();
    MGNodeOutput * connectionC = ValueC->GetConnection();

    if ( connectionA && ValueA->ConnectedBlock()->Build( _Context )
         && connectionB && ValueB->ConnectedBlock()->Build( _Context )
         && connectionC && ValueC->ConnectedBlock()->Build( _Context ) ) {

        Result->Type = connectionA->Type;

        FString expression =
                "mix( " + connectionA->Expression + ", " + EvaluateVectorCast( connectionB->Expression, connectionB->Type, connectionA->Type, 0,0,0,0 ) + ", "
                + EvaluateVectorCast( connectionC->Expression, connectionC->Type, connectionA->Type, 0,0,0,0 ) + " )";

        _Context.GenerateSourceCode( Result, expression, true );

    } else {
        Result->Type = AT_Float4;

        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloatNode )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloatNode::MGFloatNode() {
    Name = "Float";
    Stages = ANY_STAGE_BIT;
    OutValue = AddOutput( "Value", AT_Float1 );
}

void MGFloatNode::Compute( FMaterialBuildContext & _Context ) {
    OutValue->Expression = FMath::ToString( Value );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloat2Node )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloat2Node::MGFloat2Node() {
    Name = "Float2";
    Stages = ANY_STAGE_BIT;
    OutValue = AddOutput( "Value", AT_Float2 );
}

void MGFloat2Node::Compute( FMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec2( " + FMath::ToString( Value.X ) + ", " + FMath::ToString( Value.Y ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloat3Node )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloat3Node::MGFloat3Node() {
    Name = "Float3";
    Stages = ANY_STAGE_BIT;
    OutValue = AddOutput( "Value", AT_Float3 );
}

void MGFloat3Node::Compute( FMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec3( " + FMath::ToString( Value.X ) + ", " + FMath::ToString( Value.Y ) + ", " + FMath::ToString( Value.Z ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGFloat4Node )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

MGFloat4Node::MGFloat4Node() {
    Name = "Float4";
    Stages = ANY_STAGE_BIT;
    OutValue = AddOutput( "Value", AT_Float4 );
}

void MGFloat4Node::Compute( FMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec4( " + FMath::ToString( Value.X ) + ", " + FMath::ToString( Value.Y ) + ", " + FMath::ToString( Value.Z ) + ", " + FMath::ToString( Value.W ) + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGTextureSlot )

MGTextureSlot::MGTextureSlot() {
    Name = "Texture Slot";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT | SHADOWCAST_STAGE_BIT;

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

void MGTextureSlot::Compute( FMaterialBuildContext & _Context ) {
    if ( GetSlotIndex() >= 0 ) {
        Value->Expression = "tslot_" + UInt( GetSlotIndex() ).ToString();

        _Context.bHasTextures = true;
        _Context.MaxTextureSlot = FMath::Max( _Context.MaxTextureSlot, GetSlotIndex() );

    } else {
        Value->Expression.Clear();
    }
}

static constexpr const char * TextureTypeToShaderSampler[][2] = {
    { "sampler1D", "float" },
    { "sampler1DArray", "vec2" },
    { "sampler2D", "vec2" },
    //"sampler2DMS", "vec2" },
    { "sampler2DArray", "vec3" },
    //"sampler2DMSArray", "vec3" },
    { "sampler3D", "vec3" },
    { "samplerCube", "vec3" },
    { "samplerCubeArray", "vec4" },
    { "sampler2DRect", "vec2" }
};

static const char * GetShaderType( ETextureType _Type ) {
    return TextureTypeToShaderSampler[ _Type ][ 0 ];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGUniformAddress )

MGUniformAddress::MGUniformAddress() {
    Name = "Texture Slot";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT | SHADOWCAST_STAGE_BIT;

    Type = AT_Float4;
    Address = 0;

    Value = AddOutput( "Value", Type );
}

void MGUniformAddress::Compute( FMaterialBuildContext & _Context ) {
    if ( Address >= 0 ) {

        int addr = Int(Address).Clamp( 0, 15 );
        int location = addr / 4;

        Value->Type = Type;
        Value->Expression = "uaddr_" + Int(location).ToString();
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
                AN_Assert( 0 );
                break;
        }

        _Context.MaxUniformAddress = FMath::Max( _Context.MaxUniformAddress, location );

    } else {
        Value->Expression.Clear();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGSampler )
AN_ATTRIBUTE_( bSwappedToBGR, AF_DEFAULT )
AN_END_CLASS_META()

MGSampler::MGSampler() {
    Name = "Texture Sampler";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT | SHADOWCAST_STAGE_BIT;

    TextureSlot = AddInput( "TextureSlot" );
    TexCoord = AddInput( "TexCoord" );
    R = AddOutput( "R", AT_Float1 );
    G = AddOutput( "G", AT_Float1 );
    B = AddOutput( "B", AT_Float1 );
    A = AddOutput( "A", AT_Float1 );
    RGBA = AddOutput( "RGBA", AT_Float4 );
}

static const char * ChooseSampleFunction( ETextureColorSpace _ColorSpace ) {
    switch ( _ColorSpace ) {
    case TEXTURE_COLORSPACE_RGBA:
        return "texture";
    case TEXTURE_COLORSPACE_SRGB_ALPHA:
        return "texture_srgb_alpha";
    case TEXTURE_COLORSPACE_YCOCG:
        return "texture_ycocg";
    default:
        return "texture";
    }
}

void MGSampler::Compute( FMaterialBuildContext & _Context ) {
    bool bValid = false;

    MGNodeOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        MGNode * block = TextureSlot->ConnectedBlock();
        if ( block->FinalClassId() == MGTextureSlot::ClassId() && block->Build( _Context ) ) {

            MGTextureSlot * texSlot = static_cast< MGTextureSlot * >( block );

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
            case TEXTURE_2DNPOT:
                sampleType = AT_Float2;
                break;
            default:
                AN_Assert(0);
            };

            Int slotIndex = texSlot->GetSlotIndex();
            if ( slotIndex != -1 ) {

                MGNodeOutput * texCoordCon = TexCoord->GetConnection();

                if ( texCoordCon && TexCoord->ConnectedBlock()->Build( _Context ) ) {

                    const char * swizzleStr = bSwappedToBGR ? ".bgra" : "";

                    const char * sampleFunc = ChooseSampleFunction( ColorSpace );

                    RGBA->Expression = _Context.GenerateVariableName();
                    _Context.SourceCode += "const vec4 " + RGBA->Expression + " = " + sampleFunc + "( tslot_" + slotIndex.ToString() + ", " + EvaluateVectorCast( texCoordCon->Expression, texCoordCon->Type, sampleType, 0, 0, 0, 0 ) + " )" + swizzleStr + ";\n";
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
    } else {
        _Context.GenerateSourceCode( RGBA, "vec4( 0.0 )", false );

        R->Expression = "0.0";
        G->Expression = "0.0";
        B->Expression = "0.0";
        A->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( MGNormalSampler )
AN_END_CLASS_META()

MGNormalSampler::MGNormalSampler() {
    Name = "Normal Sampler";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT | SHADOWCAST_STAGE_BIT;

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

void MGNormalSampler::Compute( FMaterialBuildContext & _Context ) {
    bool bValid = false;

    MGNodeOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        MGNode * block = TextureSlot->ConnectedBlock();
        if ( block->FinalClassId() == MGTextureSlot::ClassId() && block->Build( _Context ) ) {

            MGTextureSlot * texSlot = static_cast< MGTextureSlot * >( block );

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
            case TEXTURE_2DNPOT:
                sampleType = AT_Float2;
                break;
            default:
                AN_Assert(0);
            };

            Int slotIndex = texSlot->GetSlotIndex();
            if ( slotIndex != -1 ) {

                MGNodeOutput * texCoordCon = TexCoord->GetConnection();

                if ( texCoordCon && TexCoord->ConnectedBlock()->Build( _Context ) ) {

                    const char * sampleFunc = ChooseSampleFunction( Compression );

                    XYZ->Expression = _Context.GenerateVariableName();
                    _Context.SourceCode += "const vec3 " + XYZ->Expression + " = " + sampleFunc + "( tslot_" + slotIndex.ToString() + ", " + EvaluateVectorCast( texCoordCon->Expression, texCoordCon->Type, sampleType, 0,0,0,0 ) + " );\n";
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
        _Context.GenerateSourceCode( XYZ, "vec3( 0.0, 0.0, 1.0 )", false );

        X->Expression = "0.0";
        Y->Expression = "0.0";
        Z->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInFragmentCoord )

MGInFragmentCoord::MGInFragmentCoord() {
    Name = "InFragmentCoord";
    Stages = FRAGMENT_STAGE_BIT | SHADOWCAST_STAGE_BIT;

    MGNodeOutput * OutValue = AddOutput( "Value", AT_Float4 );
    OutValue->Expression = "gl_FragCoord";

    MGNodeOutput * OutValueX = AddOutput( "X", AT_Float1 );
    OutValueX->Expression = "gl_FragCoord.x";

    MGNodeOutput * OutValueY = AddOutput( "Y", AT_Float1 );
    OutValueY->Expression = "gl_FragCoord.y";

    MGNodeOutput * OutValueZ = AddOutput( "Z", AT_Float1 );
    OutValueZ->Expression = "gl_FragCoord.z";

    MGNodeOutput * OutValueW = AddOutput( "W", AT_Float1 );
    OutValueW->Expression = "gl_FragCoord.w";

    MGNodeOutput * OutValueXY = AddOutput( "Position", AT_Float2 );
    OutValueXY->Expression = "gl_FragCoord.xy";

}

void MGInFragmentCoord::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInPosition )

MGInPosition::MGInPosition() {
    Name = "InPosition";
    Stages = VERTEX_STAGE_BIT;

    Value = AddOutput( "Value", AT_Unknown );
    //Value->Expression = "InPosition";
}

void MGInPosition::Compute( FMaterialBuildContext & _Context ) {

    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
        Value->Type = AT_Float2;
    } else {
        Value->Type = AT_Float3;
    }

    _Context.GenerateSourceCode( Value, "GetVertexPosition()", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInColor )

MGInColor::MGInColor() {
    Name = "InColor";
    Stages = VERTEX_STAGE_BIT;

    Value = AddOutput( "Value", AT_Float4 );
}

void MGInColor::Compute( FMaterialBuildContext & _Context ) {
    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
        Value->Expression = "InColor";
    } else {
        Value->Expression = "vec4(1.0)";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInTexCoord )

MGInTexCoord::MGInTexCoord() {
    Name = "InTexCoord";
    Stages = VERTEX_STAGE_BIT | SHADOWCAST_STAGE_BIT;

    MGNodeOutput * OutValue = AddOutput( "Value", AT_Float2 );
    OutValue->Expression = "InTexCoord";
}

void MGInTexCoord::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
AN_CLASS_META( FMaterialInLightmapTexCoordBlock )

FMaterialInLightmapTexCoordBlock::FMaterialInLightmapTexCoordBlock() {
    Name = "InLightmapTexCoord";
    Stages = VERTEX_STAGE_BIT;

    MGNodeOutput * OutValue = NewOutput( "Value", AT_Float2 );
    OutValue->Expression = "InLightmapTexCoord";
}

void FMaterialInLightmapTexCoordBlock::Compute( FMaterialBuildContext & _Context ) {
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInTimer )

MGInTimer::MGInTimer() {
    Name = "InTimer";
    Stages = ANY_STAGE_BIT;

    MGNodeOutput * ValGameRunningTimeSeconds = AddOutput( "GameRunningTimeSeconds", AT_Float1 );
    ValGameRunningTimeSeconds->Expression = "Timers.x";

    MGNodeOutput * ValGameplayTimeSeconds = AddOutput( "GameplayTimeSeconds", AT_Float1 );
    ValGameplayTimeSeconds->Expression = "Timers.y";
}

void MGInTimer::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGInViewPosition )

MGInViewPosition::MGInViewPosition() {
    Name = "InViewPosition";
    Stages = ANY_STAGE_BIT;

    MGNodeOutput * Val = AddOutput( "Value", AT_Float3 );
    Val->Expression = "ViewPostion.xyz";
}

void MGInViewPosition::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGCondLess )
// TODO: add greater, lequal, gequal, equal, not equal

MGCondLess::MGCondLess() {
    Name = "Cond A < B";
    Stages = ANY_STAGE_BIT;

    ValueA = AddInput( "A" );
    ValueB = AddInput( "B" );

    True = AddInput( "True" );
    False = AddInput( "False" );

    Result = AddOutput( "Result", AT_Unknown );
}

void MGCondLess::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * connectionA = ValueA->GetConnection();
    MGNodeOutput * connectionB = ValueB->GetConnection();
    MGNodeOutput * connectionTrue = True->GetConnection();
    MGNodeOutput * connectionFalse = False->GetConnection();

    FString expression;

    if ( connectionA
         && connectionB
         && connectionTrue
         && connectionFalse
         && ValueA->ConnectedBlock()->Build( _Context )
         && ValueB->ConnectedBlock()->Build( _Context )
         && True->ConnectedBlock()->Build( _Context )
         && False->ConnectedBlock()->Build( _Context ) ) {

        if ( connectionA->Type != connectionB->Type
             || connectionTrue->Type != connectionFalse->Type ) {
            Result->Type = AT_Float4;

            expression = "vec4( 0.0 )";
        } else {
            Result->Type = connectionTrue->Type;

            FString cond;

            if ( connectionA->Type == AT_Float1 ) {
                cond = "step( " + connectionB->Expression + ", " + connectionA->Expression + " )";

                expression = "mix( " + connectionTrue->Expression + ", " + connectionFalse->Expression + ", " + cond + " )";
            } else {

                if ( Result->Type == AT_Float1 ) {
                    cond = "float( all( lessThan( " + connectionA->Expression + ", " + connectionB->Expression + " ) ) )";
                } else {
                    cond = FString( AssemblyTypeStr[ Result->Type ] ) + "( float( all( lessThan( " + connectionA->Expression + ", " + connectionB->Expression + " ) ) ) )";
                }

                expression = "mix( " + connectionFalse->Expression + ", " + connectionTrue->Expression + ", " + cond + " )";
            }
        }

    } else {
        Result->Type = AT_Float4;

        expression = "vec4( 0.0 )";
    }

    _Context.GenerateSourceCode( Result, expression, false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGAtmosphereNode )

MGAtmosphereNode::MGAtmosphereNode() {
    Name = "Atmosphere Scattering";
    Stages = ANY_STAGE_BIT;

    Dir = AddInput( "Dir" );
    Result = AddOutput( "Result", AT_Float4 );
}

void MGAtmosphereNode::Compute( FMaterialBuildContext & _Context ) {
    MGNodeOutput * dirConnection = Dir->GetConnection();

    if ( dirConnection && Dir->ConnectedBlock()->Build( _Context ) ) {
        _Context.GenerateSourceCode( Result, "vec4( atmosphere( normalize(" + dirConnection->Expression + "), normalize(vec3(0.5,0.5,-1)) ), 1.0 )", false );
    } else {
        Result->Expression = "vec4( 0.0 )";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( FMaterialBuilder )

FMaterialBuilder::FMaterialBuilder() {

}

FMaterialBuilder::~FMaterialBuilder() {
    for ( MGTextureSlot * sampler : TextureSlots ) {
        sampler->RemoveRef();
    }
}

void FMaterialBuilder::RegisterTextureSlot( MGTextureSlot * _Slot ) {
    if ( TextureSlots.Size() >= MAX_MATERIAL_TEXTURES ) { // -1 for slot reserved for lightmap
        GLogger.Printf( "FMaterialBuilder::RegisterTextureSlot: MAX_MATERIAL_TEXTURES hit\n");
        return;
    }
    _Slot->AddRef();
    _Slot->SlotIndex = TextureSlots.Size();
    TextureSlots.Append( _Slot );
}

FString FMaterialBuilder::SamplersString( int _MaxtextureSlot ) const {
    FString s;
    FString bindingStr;

    for ( MGTextureSlot * slot : TextureSlots ) {
        if ( slot->GetSlotIndex() <= _MaxtextureSlot ) {
            bindingStr = UInt(slot->GetSlotIndex()).ToString();
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

static void GenerateBuiltinSource( FString & _BuiltIn ) {
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_srgb_alpha, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_ycocg, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_xyz, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_xy, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_spheremap, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_stereographic, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_paraboloid, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_quartic, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_float, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
    for ( int i = 0 ; i < TEXTURE_TYPE_MAX ; i++ ) {
        _BuiltIn += FString::Fmt( texture_nm_dxt5, TextureTypeToShaderSampler[i][0], TextureTypeToShaderSampler[i][1] );
    }
}

#if 0
static const char * AtmosphereShader = AN_STRINGIFY(
//
// Atmosphere scattering
// based on https://github.com/wwwtyro/glsl-atmosphere in public domain
// Very slow for realtime, but can by used to generate skybox
\n#define iSteps 16\n
\n#define jSteps 8\n

\n#define PI          3.1415926\n

vec2 rsi(vec3 r0, vec3 rd, float sr) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b*b) - 4.0*a*c;
    if (d < 0.0) return vec2(1e5,-1e5);
    return vec2(
        (-b - sqrt(d))/(2.0*a),
        (-b + sqrt(d))/(2.0*a)
    );
}

vec3 atmosphere(vec3 r, vec3 r0, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g) {
    // Calculate the step size of the primary ray.
    vec2 p = rsi(r0, r, rAtmos);
    if (p.x > p.y) return vec3(0,0,0);
    p.y = min(p.y, rsi(r0, r, rPlanet).x);
    float iStepSize = (p.y - p.x) / float(iSteps);

    // Initialize the primary ray time.
    float iTime = 0.0;

    // Initialize accumulators for Rayleigh and Mie scattering.
    vec3 totalRlh = vec3(0,0,0);
    vec3 totalMie = vec3(0,0,0);

    // Initialize optical depth accumulators for the primary ray.
    float iOdRlh = 0.0;
    float iOdMie = 0.0;

    // Calculate the Rayleigh and Mie phases.
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    // Sample the primary ray.
    for (int i = 0; i < iSteps; i++) {

        // Calculate the primary ray sample position.
        vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);

        // Calculate the height of the sample.
        float iHeight = length(iPos) - rPlanet;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
        float odStepMie = exp(-iHeight / shMie) * iStepSize;

        // Accumulate optical depth.
        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        // Calculate the step size of the secondary ray.
        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

        // Initialize the secondary ray time.
        float jTime = 0.0;

        // Initialize optical depth accumulators for the secondary ray.
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        // Sample the secondary ray.
        for (int j = 0; j < jSteps; j++) {

            // Calculate the secondary ray sample position.
            vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

            // Calculate the height of the sample.
            float jHeight = length(jPos) - rPlanet;

            // Accumulate the optical depth.
            jOdRlh += exp(-jHeight / shRlh) * jStepSize;
            jOdMie += exp(-jHeight / shMie) * jStepSize;

            // Increment the secondary ray time.
            jTime += jStepSize;
        }

        // Calculate attenuation.
        vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

        // Accumulate scattering.
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;

        // Increment the primary ray time.
        iTime += iStepSize;

    }

    // Calculate and return the final color.
    return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}

vec3 atmosphere( in vec3 _RayDirNormalized, in vec3 _SunPosNormalized ) {

                return vec3(0.2,0.3,1)*(_RayDirNormalized.y*0.5+0.5)*2;
    //return atmosphere(
    //    _RayDirNormalized,              // normalized ray direction
    //    vec3(0,6372e3,0),               // ray origin
    //    _SunPosNormalized,              // position of the sun
    //    22.0,                           // intensity of the sun
    //    6371e3,                         // radius of the planet in meters
    //    6471e3,                         // radius of the atmosphere in meters
    //    vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
    //    21e-6,                          // Mie scattering coefficient
    //    8e3,                            // Rayleigh scale height
    //    1.2e3,                          // Mie scale height
    //    0.758                           // Mie preferred scattering direction
    //);
}\n

);
#endif

FMaterial * FMaterialBuilder::Build() {

    FMaterialBuildData * buildData = BuildData();

    FMaterial * material = NewObject< FMaterial >();
    material->Initialize( buildData );

    GZoneMemory.Dealloc( buildData );

    return material;
}

FMaterialBuildData * FMaterialBuilder::BuildData() {
    FMaterialBuildContext context;
    bool bDepthPassTextureFetch = false;
    bool bColorPassTextureFetch = false;
    bool bWireframePassTextureFetch = false;
    bool bShadowMapPassTextureFetch = false;
    bool bShadowMapMasking = false;
    bool bNoCastShadow = false;
    UInt lightmapSlot = 0;
    int maxTextureSlot = -1;
    int maxUniformAddress = -1;
    bool bHasVertexDeform = false;

    // Load base shader script. TODO: Do it once!
    FFileStream f;
    if ( !f.OpenRead( "Shader.glsl" ) ) {
        CriticalError( "Failed to load Shader.glsl\n" );
    }
    size_t len = f.Length();
    char * baseShader = (char *)GHunkMemory.HunkMemory( len + 1, 1 );
    f.Read( baseShader, len );
    baseShader[len] = 0;
    FString code = baseShader;
    GHunkMemory.ClearLastHunk();
    //-------------------------------------------------

    FString buildinSource;
    FString predefines;

    GenerateBuiltinSource( buildinSource );

    switch ( MaterialType ) {
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
        AN_Assert( 0 );
    }

    if ( DepthHack == MATERIAL_DEPTH_HACK_WEAPON ) {
        predefines += "#define WEAPON_DEPTH_HACK\n";
        bNoCastShadow = true;
    } else if ( DepthHack == MATERIAL_DEPTH_HACK_SKYBOX ) {
        predefines += "#define SKYBOX_DEPTH_HACK\n";
        bNoCastShadow = true;
    }

    if ( !bDepthTest ) {
        bNoCastShadow = true;
    }

    code.Replace( "$BUILTIN_CODE$", buildinSource.ToConstChar() );

    if ( !VertexStage ) {
        VertexStage = NewObject< MGVertexStage >();
    }

    // Create depth pass
    context.Reset( MaterialType, MATERIAL_PASS_DEPTH );
    {
        // Depth pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );

        code.Replace( "$DEPTH_PASS_SAMPLERS$", SamplersString( context.MaxTextureSlot ).ToConstChar() );
        code.Replace( "$DEPTH_PASS_VERTEX_CODE$", context.SourceCode.ToConstChar() );

        bDepthPassTextureFetch = context.bHasTextures;

        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        // TODO: Для MATERIAL_PASS_DEPTH, MATERIAL_PASS_SHADOWMAP и MATERIAL_PASS_WIREFRAME проверить:
        // если нет никакой деформации вершины, то использовать шейдер/пайплайн по умолчанию
        // для данного прохода. Это необходимо для оптимизации количества переключения пайплайнов.
    }

    // Create shadowmap pass
    context.Reset( MaterialType, MATERIAL_PASS_SHADOWMAP );
    {
        // Shadowmap pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );

        code.Replace( "$SHADOWMAP_PASS_SAMPLERS$", SamplersString( context.MaxTextureSlot ).ToConstChar() );
        code.Replace( "$SHADOWMAP_PASS_VERTEX_CODE$", context.SourceCode.ToConstChar() );

        bShadowMapPassTextureFetch |= context.bHasTextures;

        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        if ( ShadowCastStage ) {
            context.SetStage( SHADOWCAST_STAGE );
            ShadowCastStage->ResetConnections( context );
            ShadowCastStage->TouchConnections( context );
            ShadowCastStage->Build( context );

            bShadowMapMasking = !context.SourceCode.IsEmpty();

            code.Replace( "$SHADOWMAP_PASS_FRAGMENT_CODE$", context.SourceCode.ToConstChar() );

            if ( bShadowMapMasking ) {
                bShadowMapPassTextureFetch |= context.bHasTextures;

                code.Replace( "$SHADOWMAP_PASS_FRAGMENT_SAMPLERS$", SamplersString( context.MaxTextureSlot ).ToConstChar() );

                maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
                maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );
            } else {
                code.Replace( "$SHADOWMAP_PASS_FRAGMENT_SAMPLERS$", "" );
            }
        } else {
            code.Replace( "$SHADOWMAP_PASS_FRAGMENT_CODE$", "" );
            code.Replace( "$SHADOWMAP_PASS_FRAGMENT_SAMPLERS$", "" );
        }
    }

    // Create color pass
    context.Reset( MaterialType, MATERIAL_PASS_COLOR );
    {
        // Color pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );

        bHasVertexDeform = VertexStage->HasVertexDeform();

        bColorPassTextureFetch = context.bHasTextures;

        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        int locationIndex = VertexStage->NumNextStageVariables();

        UInt bakedLightLocation = locationIndex++;
        UInt tangentLocation    = locationIndex++;
        UInt binormalLocation   = locationIndex++;
        UInt normalLocation     = locationIndex++;
        UInt positionLocation   = locationIndex++;

        predefines += "#define BAKED_LIGHT_LOCATION " + bakedLightLocation.ToString() + "\n";
        predefines += "#define TANGENT_LOCATION "     + tangentLocation.ToString()    + "\n";
        predefines += "#define BINORMAL_LOCATION "    + binormalLocation.ToString()   + "\n";
        predefines += "#define NORMAL_LOCATION "      + normalLocation.ToString()     + "\n";
        predefines += "#define POSITION_LOCATION "    + positionLocation.ToString()   + "\n";

        code.Replace( "$COLOR_PASS_VERTEX_OUTPUT_VARYINGS$", VertexStage->NSV_OutputSection().ToConstChar() );
        code.Replace( "$COLOR_PASS_VERTEX_SAMPLERS$", SamplersString( context.MaxTextureSlot ).ToConstChar() );
        code.Replace( "$COLOR_PASS_VERTEX_CODE$", context.SourceCode.ToConstChar() );

        // Color pass. Fragment stage
        context.SetStage( FRAGMENT_STAGE );
        if ( !FragmentStage ) {
            FragmentStage = NewObject< MGFragmentStage >();
        }
        FragmentStage->ResetConnections( context );
        FragmentStage->TouchConnections( context );
        FragmentStage->Build( context );

        bColorPassTextureFetch |= context.bHasTextures;

        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        lightmapSlot = context.MaxTextureSlot + 1;

        predefines += "#define LIGHTMAP_SLOT " + lightmapSlot.ToString() + "\n";

        code.Replace( "$COLOR_PASS_FRAGMENT_INPUT_VARYINGS$", VertexStage->NSV_InputSection().ToConstChar() );
        code.Replace( "$COLOR_PASS_FRAGMENT_SAMPLERS$", SamplersString( context.MaxTextureSlot ).ToConstChar() );
        code.Replace( "$COLOR_PASS_FRAGMENT_CODE$", context.SourceCode.ToConstChar() );
    }

    // Create wireframe pass
    context.Reset( MaterialType, MATERIAL_PASS_WIREFRAME );
    {
        // Wireframe pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );

        bWireframePassTextureFetch = context.bHasTextures;

        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        code.Replace( "$WIREFRAME_PASS_SAMPLERS$", SamplersString( context.MaxTextureSlot ).ToConstChar() );
        code.Replace( "$WIREFRAME_PASS_VERTEX_CODE$", context.SourceCode.ToConstChar() );
    }

    // TODO: Shadowmap pass?

    code.Replace( "$PREDEFINES$", predefines.ToConstChar() );

    GLogger.Print( "=== shader ===\n" );
    GLogger.Print( code.ToConstChar() );
    GLogger.Print( "==============\n" );

    int sizeInBytes = sizeof( FMaterialBuildData ) - sizeof( FMaterialBuildData::ShaderData ) + code.Length();

    FMaterialBuildData * data = ( FMaterialBuildData * )GZoneMemory.AllocCleared( sizeInBytes, 1 );

    data->SizeInBytes         = sizeInBytes;
    data->Type                = MaterialType;
    data->Facing              = MaterialFacing;
    data->LightmapSlot        = lightmapSlot;
    data->bDepthPassTextureFetch     = bDepthPassTextureFetch;
    data->bColorPassTextureFetch     = bColorPassTextureFetch;
    data->bWireframePassTextureFetch = bWireframePassTextureFetch;
    data->bShadowMapPassTextureFetch = bShadowMapPassTextureFetch;
    data->bHasVertexDeform    = bHasVertexDeform;
    data->bDepthTest          = bDepthTest;
    data->bNoCastShadow       = bNoCastShadow;
    data->bShadowMapMasking   = bShadowMapMasking;
    data->NumUniformVectors   = maxUniformAddress >= 0 ? maxUniformAddress / 4 + 1 : 0;
    data->NumSamplers         = maxTextureSlot + 1;

    for ( int i = 0 ; i < data->NumSamplers ; i++ ) {
        data->Samplers[i] = TextureSlots[i]->SamplerDesc;
    }

    memcpy( data->ShaderData, code.ToConstChar(), code.Length() + 1 );

    return data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META( MGMaterialGraph )

MGMaterialGraph::MGMaterialGraph() {

}

MGMaterialGraph::~MGMaterialGraph() {
    for ( MGNode * node : Nodes ) {
        node->RemoveRef();
    }
}

int MGMaterialGraph::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    if ( !Nodes.IsEmpty() ) {
        int array = _Doc.AddArray( object, "Blocks" );

        for ( MGNode * node : Nodes ) {
            int blockObject = node->Serialize( _Doc );
            _Doc.AddValueToField( array, blockObject );
        }
    }

    return object;
}


