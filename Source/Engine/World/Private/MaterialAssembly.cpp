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

#include <Engine/World/Public/MaterialAssembly.h>

static constexpr const char * AssemblyTypeStr[] = {
    "vec4",     // AT_Unknown
    "float",    // AT_Float1
    "vec2",     // AT_Float2
    "vec3",     // AT_Float3
    "vec4"      // AT_Float4
};

static FString EvaluateVectorCast( FString const & _Expression, EAssemblyType _TypeFrom, EAssemblyType _TypeTo, float _DefX, float _DefY, float _DefZ, float _DefW ) {

    if ( _TypeFrom == _TypeTo || _TypeTo == AT_Unknown ) {
        return _Expression;
    }

    switch( _TypeFrom ) {
    case AT_Unknown:
        switch ( _TypeTo ) {
        case AT_Float1:
            return Float(_DefX).ToString();
        case AT_Float2:
            return "vec2( " + Float(_DefX).ToString() + ", " + Float(_DefY).ToString() + " )";
        case AT_Float3:
            return "vec3( " + Float(_DefX).ToString() + ", " + Float(_DefY).ToString() + ", " + Float(_DefZ).ToString() + " )";
        case AT_Float4:
            return "vec4( " + Float(_DefX).ToString() + ", " + Float(_DefY).ToString() + ", " + Float(_DefZ).ToString() + ", " + Float(_DefW).ToString() + " )";
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
            return "vec3( " + _Expression + ", " + Float(_DefZ).ToString() + " )";
        case AT_Float4:
            return "vec4( " + _Expression + ", " + Float(_DefZ).ToString() + ", " + Float(_DefW).ToString() + " )";
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
            return "vec4( " + _Expression + ", " + Float(_DefW).ToString() + " )";
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

void FMaterialBuildContext::GenerateSourceCode( FAssemblyBlockOutput * _Slot, FString const & _Expression, bool _AddBrackets ) {
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

AN_CLASS_META_NO_ATTRIBS( FAssemblyBlockOutput )

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FAssemblyBlockInput )

FAssemblyBlockInput::FAssemblyBlockInput() {
}

void FAssemblyBlockInput::Connect( FAssemblyBlock * _Block, const char * _Slot ) {
    Block = _Block;
    Slot = _Slot;
}

void FAssemblyBlockInput::Disconnect() {
    Block = nullptr;
    Slot.Clear();
}

FAssemblyBlockOutput * FAssemblyBlockInput::GetConnection() {
    if ( !Block ) {
        return nullptr;
    }

    FAssemblyBlockOutput * out = Block->FindOutput( Slot.ToConstChar() );
    //out->Usages++;

    return out;
}

int FAssemblyBlockInput::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( Name ).ToConstChar() );

    if ( Block ) {
        _Doc.AddStringField( object, "Slot", _Doc.ProxyBuffer.NewString( Slot ).ToConstChar() );
        _Doc.AddStringField( object, "Block", _Doc.ProxyBuffer.NewString( Block->GetGUID().ToString() ).ToConstChar() );
    }

    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FAssemblyNextStageVariable )

void FAssemblyNextStageVariable::Connect( FAssemblyBlock * _Block, const char * _Slot ) {
    Block = _Block;
    Slot = _Slot;
}

void FAssemblyNextStageVariable::Disconnect() {
    Block = nullptr;
    Slot.Clear();
}

FAssemblyBlockOutput * FAssemblyNextStageVariable::GetConnection() {
    if ( !Block ) {
        return nullptr;
    }

    return Block->FindOutput( Slot.ToConstChar() );
}

int FAssemblyNextStageVariable::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( Name ).ToConstChar() );

    if ( Block ) {
        _Doc.AddStringField( object, "Slot", _Doc.ProxyBuffer.NewString( Slot ).ToConstChar() );
        _Doc.AddStringField( object, "Block", _Doc.ProxyBuffer.NewString( Block->GetGUID().ToString() ).ToConstChar() );
    }

    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( FAssemblyBlock )
AN_ATTRIBUTE_( Location, AF_DEFAULT )
AN_END_CLASS_META()

FAssemblyBlock::FAssemblyBlock() {
    GUID.Generate();
}

FAssemblyBlock::~FAssemblyBlock() {
    for ( FAssemblyBlockInput * In : Inputs ) {
        In->RemoveRef();
    }

    for ( FAssemblyBlockOutput * Out : Outputs ) {
        Out->RemoveRef();
    }
}

FAssemblyBlockInput * FAssemblyBlock::NewInput( const char * _Name ) {
    FAssemblyBlockInput * In = NewObject< FAssemblyBlockInput >();
    In->AddRef();
    In->SetName( _Name );
    Inputs.Append( In );
    return In;
}

FAssemblyBlockOutput * FAssemblyBlock::NewOutput( const char * _Name, EAssemblyType _Type ) {
    FAssemblyBlockOutput * Out = NewObject< FAssemblyBlockOutput >();
    Out->AddRef();
    Out->SetName( _Name );
    Out->Type = _Type;
    Outputs.Append( Out );
    return Out;
}

FAssemblyBlockOutput * FAssemblyBlock::FindOutput( const char * _Name ) {
    for ( FAssemblyBlockOutput * Out : Outputs ) {
        if ( !Out->GetName().Cmp( _Name ) ) {
            return Out;
        }
    }
    FMaterialStageBlock * stageBlock = dynamic_cast< FMaterialStageBlock * >( this );
    if ( stageBlock ) {
        for ( FAssemblyNextStageVariable * Out : stageBlock->NextStageVariables ) {
            if ( !Out->GetName().Cmp( _Name ) ) {
                return Out;
            }
        }
    }
    return nullptr;
}

bool FAssemblyBlock::Build( FMaterialBuildContext & _Context ) {
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

void FAssemblyBlock::ResetConnections( FMaterialBuildContext const & _Context ) {
    if ( !bTouched ) {
        return;
    }

    bTouched = false;

    for ( FAssemblyBlockInput * in : Inputs ) {
        FAssemblyBlockOutput * out = in->GetConnection();
        if ( out ) {
            FAssemblyBlock * block = in->ConnectedBlock();
            block->ResetConnections( _Context );
            out->Usages[_Context.GetStage()] = 0;
        }
    }
}

void FAssemblyBlock::TouchConnections( FMaterialBuildContext const & _Context ) {
    if ( bTouched ) {
        return;
    }

    bTouched = true;

    for ( FAssemblyBlockInput * in : Inputs ) {
        FAssemblyBlockOutput * out = in->GetConnection();
        if ( out ) {
            FAssemblyBlock * block = in->ConnectedBlock();
            block->TouchConnections( _Context );
            out->Usages[_Context.GetStage()]++;
        }
    }
}

int FAssemblyBlock::Serialize( FDocument & _Doc ) {
#if 0
    int object = _Doc.CreateObjectValue();
    _Doc.AddStringField( object, "ClassName", FinalClassName() );
#else
    int object = Super::Serialize(_Doc);
#endif

    _Doc.AddStringField( object, "GUID", _Doc.ProxyBuffer.NewString( GUID.ToString() ).ToConstChar() );

    if ( !Inputs.IsEmpty() ) {
        int array = _Doc.AddArray( object, "Inputs" );
        for ( FAssemblyBlockInput * in : Inputs ) {
            int inputObject = in->Serialize( _Doc );
            _Doc.AddValueToField( array, inputObject );
        }
    }
    return object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialStageBlock )

FMaterialStageBlock::~FMaterialStageBlock() {
    for ( FAssemblyNextStageVariable * nsv : NextStageVariables ) {
        nsv->RemoveRef();
    }
}

FAssemblyNextStageVariable * FMaterialStageBlock::AddNextStageVariable( const char * _Name, EAssemblyType _Type ) {
    if ( FindOutput( _Name ) ) {
        return nullptr;
    }

    FAssemblyNextStageVariable * nsv = NewObject< FAssemblyNextStageVariable >();
    nsv->AddRef();
    nsv->SetName( _Name );
    nsv->Expression = "nsv_" + NsvPrefix + Int(NextStageVariables.Length()).ToString() + "_" + nsv->GetName();
    nsv->Type = _Type;
    NextStageVariables.Append( nsv );

    return nsv;
}

FAssemblyNextStageVariable * FMaterialStageBlock::FindNextStageVariable( const char * _Name ) {
    for ( FAssemblyNextStageVariable * Out : NextStageVariables ) {
        if ( !Out->GetName().Cmp( _Name ) ) {
            return Out;
        }
    }
    return nullptr;
}

FString FMaterialStageBlock::NSV_OutputSection() {
    FString s;
    uint32_t location = 0;
    for ( FAssemblyNextStageVariable * nsv : NextStageVariables ) {
        s += "layout( location = " + UInt(location++).ToString() + " ) out " + AssemblyTypeStr[nsv->Type] + " " + nsv->Expression + ";\n";
    }
    return s;
}

FString FMaterialStageBlock::NSV_InputSection() {
    FString s;
    uint32_t location = 0;
    for ( FAssemblyNextStageVariable * nsv : NextStageVariables ) {
        s += "layout( location = " + UInt(location++).ToString() + " ) in " + AssemblyTypeStr[nsv->Type] + " " + nsv->Expression + ";\n";
    }
    return s;
}

int FMaterialStageBlock::Serialize( FDocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    if ( !NextStageVariables.IsEmpty() ) {
        int array = _Doc.AddArray( object, "NSV" );
        for ( FAssemblyNextStageVariable * nsv : NextStageVariables ) {
            int nsvObject = nsv->Serialize( _Doc );
            _Doc.AddValueToField( array, nsvObject );
        }
    }

    return object;
}

void FMaterialStageBlock::Compute( FMaterialBuildContext & _Context ) {
    for ( FAssemblyNextStageVariable * nsv : NextStageVariables ) {

        FAssemblyBlockOutput * connection = nsv->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialVertexStage )

FMaterialVertexStage::FMaterialVertexStage() {
    Name = "Material Vertex Stage";
    Stages = VERTEX_STAGE_BIT;
    NsvPrefix = "VS";

    Position = NewInput( "Position" );
}

FMaterialVertexStage::~FMaterialVertexStage() {

}

void FMaterialVertexStage::Compute( FMaterialBuildContext & _Context ) {

    if ( _Context.GetMaterialPass() == MATERIAL_PASS_COLOR ) {

        // Super class adds nsv_ definition. Currently nsv_ variables supported only for MATERIAL_PASS_COLOR.
        Super::Compute( _Context );
    }

    FAssemblyBlockOutput * positionCon = Position->GetConnection();

    bool bValid = true;

    bNoVertexDeform = true;

    if ( positionCon && Position->ConnectedBlock()->Build( _Context ) ) {

        if ( positionCon->Expression != "GetVertexPosition()" ) {
            bNoVertexDeform = false;
        }

        switch( positionCon->Type ) {
        case AT_Float1:
            _Context.SourceCode += "gl_Position = ProjectTranslateViewMatrix * vec4(" + positionCon->Expression + ", 0.0, 0.0, 1.0 );\n";
            break;
        case AT_Float2:
            _Context.SourceCode += "gl_Position = ProjectTranslateViewMatrix * vec4(" + positionCon->Expression + ", 0.0, 1.0 );\n";
            break;
        case AT_Float3:
            _Context.SourceCode += "gl_Position = ProjectTranslateViewMatrix * vec4(" + positionCon->Expression + ", 1.0 );\n";
            break;
        case AT_Float4:
            _Context.SourceCode += "gl_Position = ProjectTranslateViewMatrix * (" + positionCon->Expression + ");\n";
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

        _Context.SourceCode += "gl_Position = ProjectTranslateViewMatrix * vec4( GetVertexPosition(), 1.0 );\n";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialFragmentStage )

FMaterialFragmentStage::FMaterialFragmentStage() {
    Name = "Material Fragment Stage";
    Stages = FRAGMENT_STAGE_BIT;
    NsvPrefix = "FS";

    Color = NewInput( "Color" );
}

FMaterialFragmentStage::~FMaterialFragmentStage() {

}

void FMaterialFragmentStage::Compute( FMaterialBuildContext & _Context ) {

    Super::Compute( _Context );

    FAssemblyBlockOutput * colorCon = Color->GetConnection();

    bool bValid = true;

    if ( colorCon && Color->ConnectedBlock()->Build( _Context ) ) {

        switch( colorCon->Type ) {
        case AT_Float1:
            _Context.SourceCode += "FS_FragColor = vec4(" + colorCon->Expression + ", 0.0, 0.0, 1.0 );\n";
            break;
        case AT_Float2:
            _Context.SourceCode += "FS_FragColor = vec4(" + colorCon->Expression + ", 0.0, 1.0 );\n";
            break;
        case AT_Float3:
            _Context.SourceCode += "FS_FragColor = vec4(" + colorCon->Expression + ", 1.0 );\n";
            break;
        case AT_Float4:
            _Context.SourceCode += "FS_FragColor = " + colorCon->Expression + ";\n";
            break;
        default:
            bValid = false;
            break;
        }

    } else {
        bValid = false;
    }

    if ( !bValid ) {
        GLogger.Printf( "%s: Invalid input type\n", Name.ToConstChar() );

        _Context.SourceCode += "FS_FragColor = vec4(1);\n";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialProjectionBlock )

FMaterialProjectionBlock::FMaterialProjectionBlock() {
    Name = "Projection";
    Stages = VERTEX_STAGE_BIT;

    Vector = NewInput( "Vector" );
    Result = NewOutput( "Result", AT_Float4 );
}

void FMaterialProjectionBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * vectorCon = Vector->GetConnection();

    if ( vectorCon && Vector->ConnectedBlock()->Build( _Context ) ) {
        switch( vectorCon->Type ) {
        case AT_Float1:
            _Context.GenerateSourceCode( Result, "ProjectTranslateViewMatrix * vec4( " + vectorCon->Expression + ", 0.0, 0.0, 1.0 )", true );
            break;
        case AT_Float2:
            _Context.GenerateSourceCode( Result, "ProjectTranslateViewMatrix * vec4( " + vectorCon->Expression + ", 0.0, 1.0 )", true );
            break;
        case AT_Float3:
            _Context.GenerateSourceCode( Result, "ProjectTranslateViewMatrix * vec4( " + vectorCon->Expression + ", 1.0 )", true );
            break;
        case AT_Float4:
            _Context.GenerateSourceCode( Result, "ProjectTranslateViewMatrix * " + vectorCon->Expression, true );
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

AN_CLASS_META_NO_ATTRIBS( FMaterialLengthBlock )

FMaterialLengthBlock::FMaterialLengthBlock() {
    Name = "Length";
    Stages = ANY_STAGE_BIT;

    Value = NewInput( "Value" );
    Result = NewOutput( "Result", AT_Float1 );
}

void FMaterialLengthBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Value->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialNormalizeBlock )

FMaterialNormalizeBlock::FMaterialNormalizeBlock() {
    Name = "Normalize";
    Stages = ANY_STAGE_BIT;

    Value = NewInput( "Value" );
    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialNormalizeBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Value->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialDecomposeVectorBlock )

FMaterialDecomposeVectorBlock::FMaterialDecomposeVectorBlock() {
    Name = "Decompose Vector";
    Stages = ANY_STAGE_BIT;

    Vector = NewInput( "Vector" );
    X = NewOutput( "X", AT_Float1 );
    Y = NewOutput( "Y", AT_Float1 );
    Z = NewOutput( "Z", AT_Float1 );
    W = NewOutput( "W", AT_Float1 );
}

void FMaterialDecomposeVectorBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Vector->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialMakeVectorBlock )

FMaterialMakeVectorBlock::FMaterialMakeVectorBlock() {
    Name = "Make Vector";
    Stages = ANY_STAGE_BIT;

    X = NewInput( "X" );
    Y = NewInput( "Y" );
    Z = NewInput( "Z" );
    W = NewInput( "W" );
    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialMakeVectorBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * XConnection = X->GetConnection();
    FAssemblyBlockOutput * YConnection = Y->GetConnection();
    FAssemblyBlockOutput * ZConnection = Z->GetConnection();
    FAssemblyBlockOutput * WConnection = W->GetConnection();

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

    Result->Type = EAssemblyType( AT_Float1 + numComponents - 1 );

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

AN_CLASS_META_NO_ATTRIBS( FMaterialNegateBlock )

FMaterialNegateBlock::FMaterialNegateBlock() {
    Name = "Negate";
    Stages = ANY_STAGE_BIT;

    Value = NewInput( "Value" );
    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialNegateBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;

        _Context.GenerateSourceCode( Result, "-" + inputConnection->Expression, true );
    } else {
        Result->Type = AT_Float1;
        Result->Expression = "0.0";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialFractBlock )

FMaterialFractBlock::FMaterialFractBlock() {
    Name = "Fract";
    Stages = ANY_STAGE_BIT;

    Value = NewInput( "Value" );
    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialFractBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Value->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialSinusBlock )

FMaterialSinusBlock::FMaterialSinusBlock() {
    Name = "Sin";
    Stages = ANY_STAGE_BIT;

    Value = NewInput( "Value" );
    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialSinusBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;
        _Context.GenerateSourceCode( Result, "sin( " + inputConnection->Expression + " )", false );
    } else {
        Result->Type = AT_Float4;
        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialCosinusBlock )

FMaterialCosinusBlock::FMaterialCosinusBlock() {
    Name = "Cos";
    Stages = ANY_STAGE_BIT;

    Value = NewInput( "Value" );
    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialCosinusBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * inputConnection = Value->GetConnection();

    if ( inputConnection && Value->ConnectedBlock()->Build( _Context ) ) {
        Result->Type = inputConnection->Type;
        _Context.GenerateSourceCode( Result, "cos( " + inputConnection->Expression + " )", false );
    } else {
        Result->Type = AT_Float4;
        _Context.GenerateSourceCode( Result, "vec4( 0.0 )", false );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialArithmeticBlock )

FMaterialArithmeticBlock::FMaterialArithmeticBlock() {
    Stages = ANY_STAGE_BIT;

    ValueA = NewInput( "A" );
    ValueB = NewInput( "B" );

    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialArithmeticBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * connectionA = ValueA->GetConnection();
    FAssemblyBlockOutput * connectionB = ValueB->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialMulBlock )
AN_CLASS_META_NO_ATTRIBS( FMaterialDivBlock )
AN_CLASS_META_NO_ATTRIBS( FMaterialAddBlock )
AN_CLASS_META_NO_ATTRIBS( FMaterialSubBlock )

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialMADBlock )

FMaterialMADBlock::FMaterialMADBlock() {
    Name = "MAD A * B + C";
    Stages = ANY_STAGE_BIT;

    ValueA = NewInput( "A" );
    ValueB = NewInput( "B" );
    ValueC = NewInput( "C" );

    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialMADBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * connectionA = ValueA->GetConnection();
    FAssemblyBlockOutput * connectionB = ValueB->GetConnection();
    FAssemblyBlockOutput * connectionC = ValueC->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialStepBlock )

FMaterialStepBlock::FMaterialStepBlock() {
    Name = "Step( A, B )";
    Stages = ANY_STAGE_BIT;

    ValueA = NewInput( "A" );
    ValueB = NewInput( "B" );

    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialStepBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * connectionA = ValueA->GetConnection();
    FAssemblyBlockOutput * connectionB = ValueB->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialPowBlock )

FMaterialPowBlock::FMaterialPowBlock() {
    Name = "Pow A^B";
    Stages = ANY_STAGE_BIT;

    ValueA = NewInput( "A" );
    ValueB = NewInput( "B" );

    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialPowBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * connectionA = ValueA->GetConnection();
    FAssemblyBlockOutput * connectionB = ValueB->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialLerpBlock )

FMaterialLerpBlock::FMaterialLerpBlock() {
    Name = "Lerp( A, B, C )";
    Stages = ANY_STAGE_BIT;

    ValueA = NewInput( "A" );
    ValueB = NewInput( "B" );
    ValueC = NewInput( "C" );

    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialLerpBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * connectionA = ValueA->GetConnection();
    FAssemblyBlockOutput * connectionB = ValueB->GetConnection();
    FAssemblyBlockOutput * connectionC = ValueC->GetConnection();

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

AN_BEGIN_CLASS_META( FMaterialFloatBlock )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

FMaterialFloatBlock::FMaterialFloatBlock() {
    Name = "Float";
    Stages = ANY_STAGE_BIT;
    OutValue = NewOutput( "Value", AT_Float1 );
}

void FMaterialFloatBlock::Compute( FMaterialBuildContext & _Context ) {
    OutValue->Expression = Float(Value).ToString();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( FMaterialFloat2Block )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

FMaterialFloat2Block::FMaterialFloat2Block() {
    Name = "Float2";
    Stages = ANY_STAGE_BIT;
    OutValue = NewOutput( "Value", AT_Float2 );
}

void FMaterialFloat2Block::Compute( FMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec2( " + Value.X.ToString() + ", " + Value.Y.ToString() + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( FMaterialFloat3Block )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

FMaterialFloat3Block::FMaterialFloat3Block() {
    Name = "Float3";
    Stages = ANY_STAGE_BIT;
    OutValue = NewOutput( "Value", AT_Float3 );
}

void FMaterialFloat3Block::Compute( FMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec3( " + Value.X.ToString() + ", " + Value.Y.ToString() + ", " + Value.Z.ToString() + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_BEGIN_CLASS_META( FMaterialFloat4Block )
AN_ATTRIBUTE_( Value, AF_DEFAULT )
AN_END_CLASS_META()

FMaterialFloat4Block::FMaterialFloat4Block() {
    Name = "Float4";
    Stages = ANY_STAGE_BIT;
    OutValue = NewOutput( "Value", AT_Float4 );
}

void FMaterialFloat4Block::Compute( FMaterialBuildContext & _Context ) {
    _Context.GenerateSourceCode( OutValue, "vec4( " + Value.X.ToString() + ", " + Value.Y.ToString() + ", " + Value.Z.ToString() + ", " + Value.W.ToString() + " )", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialTextureSlotBlock )

FMaterialTextureSlotBlock::FMaterialTextureSlotBlock() {
    Name = "Texture Slot";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT;

    TextureType = TEXTURE_2D;
    Filter = TEXTURE_FILTER_LINEAR;
    AddressU = TEXTURE_ADDRESS_WRAP;
    AddressV = TEXTURE_ADDRESS_WRAP;
    AddressW = TEXTURE_ADDRESS_WRAP;
    MipLODBias = 0;
    Anisotropy = 16;
    MinLod = -1000;
    MaxLod = 1000;

    SlotIndex = -1;

    Value = NewOutput( "Value", AT_Unknown );
}

void FMaterialTextureSlotBlock::Compute( FMaterialBuildContext & _Context ) {
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

AN_CLASS_META_NO_ATTRIBS( FMaterialUniformAddress )

FMaterialUniformAddress::FMaterialUniformAddress() {
    Name = "Texture Slot";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT;

    Type = AT_Float4;
    Address = 0;

    Value = NewOutput( "Value", Type );
}

void FMaterialUniformAddress::Compute( FMaterialBuildContext & _Context ) {
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

AN_BEGIN_CLASS_META( FMaterialSamplerBlock )
AN_ATTRIBUTE_( bSwappedToBGR, AF_DEFAULT )
AN_END_CLASS_META()

FMaterialSamplerBlock::FMaterialSamplerBlock() {
    Name = "Texture Sampler";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT;

    TextureSlot = NewInput( "TextureSlot" );
    TexCoord = NewInput( "TexCoord" );
    R = NewOutput( "R", AT_Float1 );
    G = NewOutput( "G", AT_Float1 );
    B = NewOutput( "B", AT_Float1 );
    A = NewOutput( "A", AT_Float1 );
    RGBA = NewOutput( "RGBA", AT_Float4 );
}

static const char * ChooseSampleFunction( FMaterialSamplerBlock::EColorSpace _ColorSpace ) {
    switch ( _ColorSpace ) {
    case FMaterialSamplerBlock::COLORSPACE_RGBA:
        return "texture";
    case FMaterialSamplerBlock::COLORSPACE_SRGB_ALPHA:
        return "texture_srgb_alpha";
    case FMaterialSamplerBlock::COLORSPACE_YCOCG:
        return "texture_ycocg";
    default:
        return "texture";
    }
}

#if 0
static const char * ChooseSampleFunction( ETextureColorSpace _ColorSpace ) {
    switch ( _ColorSpace ) {
    case TEXTURE_COLORSPACE_RGBA:
        return "texture";
    case TEXTURE_COLORSPACE_sRGB_ALPHA:
        return "texture_srgb_alpha";
    case TEXTURE_COLORSPACE_YCOCG:
        return "texture_ycocg";
    case TEXTURE_COLORSPACE_NM_XY:
        return "texture_nm_xy";
    case TEXTURE_COLORSPACE_NM_XYZ:
        return "texture_nm_xyz";
    case TEXTURE_COLORSPACE_NM_SPHEREMAP:
        return "texture_nm_spheremap";
    case TEXTURE_COLORSPACE_NM_XY_STEREOGRAPHIC:
        return "texture_nm_stereographic";
    case TEXTURE_COLORSPACE_NM_XY_PARABOLOID:
        return "texture_nm_paraboloid";
    case TEXTURE_COLORSPACE_NM_XY_QUARTIC:
        return "texture_nm_quartic";
    case TEXTURE_COLORSPACE_NM_FLOAT:
        return "texture_nm_float";
    case TEXTURE_COLORSPACE_NM_DXT5:
        return "texture_nm_dxt5";
    case TEXTURE_COLORSPACE_GRAYSCALED:
        return "texture_grayscaled";
    case TEXTURE_COLORSPACE_RGBA_INT:
        return "texture_rgba_int";
    case TEXTURE_COLORSPACE_RGBA_UINT:
        return "texture_rgba_uint";
    default:
        return "texture";
    }
}
#endif

void FMaterialSamplerBlock::Compute( FMaterialBuildContext & _Context ) {
    bool bValid = false;

    FAssemblyBlockOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        FAssemblyBlock * block = TextureSlot->ConnectedBlock();
        if ( block->FinalClassId() == FMaterialTextureSlotBlock::ClassId() && block->Build( _Context ) ) {

            FMaterialTextureSlotBlock * texSlot = static_cast< FMaterialTextureSlotBlock * >( block );

            EAssemblyType sampleType = AT_Float2;

            // TODO: table?
            switch( texSlot->TextureType ) {
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
            case TEXTURE_RECT:
                sampleType = AT_Float2;
                break;
            default:
                AN_Assert(0);
            };

            Int slotIndex = texSlot->GetSlotIndex();
            if ( slotIndex != -1 ) {

                FAssemblyBlockOutput * texCoordCon = TexCoord->GetConnection();

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

AN_BEGIN_CLASS_META( FMaterialNormalSamplerBlock )
AN_END_CLASS_META()

FMaterialNormalSamplerBlock::FMaterialNormalSamplerBlock() {
    Name = "Normal Sampler";
    Stages = VERTEX_STAGE_BIT | FRAGMENT_STAGE_BIT;

    TextureSlot = NewInput( "TextureSlot" );
    TexCoord = NewInput( "TexCoord" );
    X = NewOutput( "X", AT_Float1 );
    Y = NewOutput( "Y", AT_Float1 );
    Z = NewOutput( "Z", AT_Float1 );
    XYZ = NewOutput( "XYZ", AT_Float3 );
}

static const char * ChooseSampleFunction( FMaterialNormalSamplerBlock::ENormalCompression _Compression ) {
    switch ( _Compression ) {
    case FMaterialNormalSamplerBlock::NM_XYZ:
        return "texture_nm_xyz";
    case FMaterialNormalSamplerBlock::NM_XY:
        return "texture_nm_xy";
    case FMaterialNormalSamplerBlock::NM_SPHEREMAP:
        return "texture_nm_spheremap";
    case FMaterialNormalSamplerBlock::NM_STEREOGRAPHIC:
        return "texture_nm_stereographic";
    case FMaterialNormalSamplerBlock::NM_PARABOLOID:
        return "texture_nm_paraboloid";
    case FMaterialNormalSamplerBlock::NM_QUARTIC:
        return "texture_nm_quartic";
    case FMaterialNormalSamplerBlock::NM_FLOAT:
        return "texture_nm_float";
    case FMaterialNormalSamplerBlock::NM_DXT5:
        return "texture_nm_dxt5";
    default:
        return "texture_nm_xyz";
    }
}

void FMaterialNormalSamplerBlock::Compute( FMaterialBuildContext & _Context ) {
    bool bValid = false;

    FAssemblyBlockOutput * texSlotCon = TextureSlot->GetConnection();
    if ( texSlotCon ) {

        FAssemblyBlock * block = TextureSlot->ConnectedBlock();
        if ( block->FinalClassId() == FMaterialTextureSlotBlock::ClassId() && block->Build( _Context ) ) {

            FMaterialTextureSlotBlock * texSlot = static_cast< FMaterialTextureSlotBlock * >( block );

            EAssemblyType sampleType = AT_Float2;

            // TODO: table?
            switch( texSlot->TextureType ) {
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
            case TEXTURE_RECT:
                sampleType = AT_Float2;
                break;
            default:
                AN_Assert(0);
            };

            Int slotIndex = texSlot->GetSlotIndex();
            if ( slotIndex != -1 ) {

                FAssemblyBlockOutput * texCoordCon = TexCoord->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialInFragmentCoordBlock )

FMaterialInFragmentCoordBlock::FMaterialInFragmentCoordBlock() {
    Name = "InFragmentCoord";
    Stages = FRAGMENT_STAGE_BIT;

    FAssemblyBlockOutput * OutValue = NewOutput( "Value", AT_Float4 );
    OutValue->Expression = "gl_FragCoord";

    FAssemblyBlockOutput * OutValueX = NewOutput( "X", AT_Float1 );
    OutValueX->Expression = "gl_FragCoord.x";

    FAssemblyBlockOutput * OutValueY = NewOutput( "Y", AT_Float1 );
    OutValueY->Expression = "gl_FragCoord.y";

    FAssemblyBlockOutput * OutValueZ = NewOutput( "Z", AT_Float1 );
    OutValueZ->Expression = "gl_FragCoord.z";

    FAssemblyBlockOutput * OutValueW = NewOutput( "W", AT_Float1 );
    OutValueW->Expression = "gl_FragCoord.w";

    FAssemblyBlockOutput * OutValueXY = NewOutput( "Position", AT_Float2 );
    OutValueXY->Expression = "gl_FragCoord.xy";

}

void FMaterialInFragmentCoordBlock::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialInPositionBlock )

FMaterialInPositionBlock::FMaterialInPositionBlock() {
    Name = "InPosition";
    Stages = VERTEX_STAGE_BIT;

    Value = NewOutput( "Value", AT_Unknown );
    //Value->Expression = "InPosition";
}

void FMaterialInPositionBlock::Compute( FMaterialBuildContext & _Context ) {

    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
        Value->Type = AT_Float2;
    } else {
        Value->Type = AT_Float3;
    }

    _Context.GenerateSourceCode( Value, "GetVertexPosition()", false );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialInColorBlock )

FMaterialInColorBlock::FMaterialInColorBlock() {
    Name = "InColor";
    Stages = VERTEX_STAGE_BIT;

    Value = NewOutput( "Value", AT_Float4 );
}

void FMaterialInColorBlock::Compute( FMaterialBuildContext & _Context ) {
    if ( _Context.GetMaterialType() == MATERIAL_TYPE_HUD ) {
        Value->Expression = "InColor";
    } else {
        Value->Expression = "vec4(1.0)";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialInTexCoordBlock )

FMaterialInTexCoordBlock::FMaterialInTexCoordBlock() {
    Name = "InTexCoord";
    Stages = VERTEX_STAGE_BIT;

    FAssemblyBlockOutput * OutValue = NewOutput( "Value", AT_Float2 );
    OutValue->Expression = "InTexCoord";
}

void FMaterialInTexCoordBlock::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
AN_CLASS_META_NO_ATTRIBS( FMaterialInLightmapTexCoordBlock )

FMaterialInLightmapTexCoordBlock::FMaterialInLightmapTexCoordBlock() {
    Name = "InLightmapTexCoord";
    Stages = VERTEX_STAGE_BIT;

    FAssemblyBlockOutput * OutValue = NewOutput( "Value", AT_Float2 );
    OutValue->Expression = "InLightmapTexCoord";
}

void FMaterialInLightmapTexCoordBlock::Compute( FMaterialBuildContext & _Context ) {
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialInTimerBlock )

FMaterialInTimerBlock::FMaterialInTimerBlock() {
    Name = "InTimer";
    Stages = ANY_STAGE_BIT;

    FAssemblyBlockOutput * ValGameRunningTimeSeconds = NewOutput( "GameRunningTimeSeconds", AT_Float1 );
    ValGameRunningTimeSeconds->Expression = "Timers.x";

    FAssemblyBlockOutput * ValGameplayTimeSeconds = NewOutput( "GameplayTimeSeconds", AT_Float1 );
    ValGameplayTimeSeconds->Expression = "Timers.y";
}

void FMaterialInTimerBlock::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialInViewPositionBlock )

FMaterialInViewPositionBlock::FMaterialInViewPositionBlock() {
    Name = "InViewPosition";
    Stages = ANY_STAGE_BIT;

    FAssemblyBlockOutput * Val = NewOutput( "Value", AT_Float3 );
    Val->Expression = "ViewPostion.xyz";
}

void FMaterialInViewPositionBlock::Compute( FMaterialBuildContext & _Context ) {
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialCondLessBlock )
// TODO: add greater, lequal, gequal, equal, not equal

FMaterialCondLessBlock::FMaterialCondLessBlock() {
    Name = "Cond A < B";
    Stages = ANY_STAGE_BIT;

    ValueA = NewInput( "A" );
    ValueB = NewInput( "B" );

    True = NewInput( "True" );
    False = NewInput( "False" );

    Result = NewOutput( "Result", AT_Unknown );
}

void FMaterialCondLessBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * connectionA = ValueA->GetConnection();
    FAssemblyBlockOutput * connectionB = ValueB->GetConnection();
    FAssemblyBlockOutput * connectionTrue = True->GetConnection();
    FAssemblyBlockOutput * connectionFalse = False->GetConnection();

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

AN_CLASS_META_NO_ATTRIBS( FMaterialAtmosphereBlock )

FMaterialAtmosphereBlock::FMaterialAtmosphereBlock() {
    Name = "Atmosphere Scattering";
    Stages = ANY_STAGE_BIT;

    Dir = NewInput( "Dir" );
    Result = NewOutput( "Result", AT_Float4 );
}

void FMaterialAtmosphereBlock::Compute( FMaterialBuildContext & _Context ) {
    FAssemblyBlockOutput * dirConnection = Dir->GetConnection();

    if ( dirConnection && Dir->ConnectedBlock()->Build( _Context ) ) {
        _Context.GenerateSourceCode( Result, "vec4( atmosphere( normalize(" + dirConnection->Expression + "), normalize(vec3(0.5,0.5,-1)) ), 1.0 )", false );
    } else {
        Result->Expression = "vec4( 0.0 )";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialBuilder )

FMaterialBuilder::FMaterialBuilder() {

}

FMaterialBuilder::~FMaterialBuilder() {
    for ( FMaterialTextureSlotBlock * sampler : TextureSlots ) {
        sampler->RemoveRef();
    }
}

void FMaterialBuilder::RegisterTextureSlot( FMaterialTextureSlotBlock * _Slot ) {
    if ( TextureSlots.Length() >= MAX_MATERIAL_TEXTURES ) { // -1 for slot reserved for lightmap
        GLogger.Printf( "FMaterialBuilder::RegisterTextureSlot: MAX_MATERIAL_TEXTURES hit\n");
        return;
    }
    _Slot->AddRef();
    _Slot->SlotIndex = TextureSlots.Length();
    TextureSlots.Append( _Slot );
}

FString FMaterialBuilder::SamplersString( int _MaxtextureSlot ) const {
    FString s;
    FString bindingStr;

    for ( FMaterialTextureSlotBlock * slot : TextureSlots ) {
        if ( slot->GetSlotIndex() <= _MaxtextureSlot ) {
            bindingStr = UInt(slot->GetSlotIndex()).ToString();
            s += "layout( binding = " + bindingStr + " ) uniform " + GetShaderType( slot->TextureType ) + " tslot_" + bindingStr + ";\n";
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

FMaterial * FMaterialBuilder::Build() {
    FString VertexSrc;
    FString FragmentSrc;
    FString GeometrySrc;
    FMaterialBuildContext context;
    bool bHasTextures[MATERIAL_PASS_MAX];
    bool bVertexTextureFetch = false;
    int lightmapSlot = 0;
    int maxTextureSlot = -1;
    int maxUniformAddress = -1;
    bool bNoVertexDeform = true;

    memset( bHasTextures, 0, sizeof( bHasTextures ) );

    FString prebuildVertexShader =
        "#ifdef SKINNED_MESH\n"
        "    const vec4 SrcPosition = vec4( InPosition, 1.0 );\n"

        "    const vec4\n"
        "    JointTransform0 = Transform[ InJointIndices[0] * 3 + 0 ] * InJointWeights[0]\n"
        "                    + Transform[ InJointIndices[1] * 3 + 0 ] * InJointWeights[1]\n"
        "                    + Transform[ InJointIndices[2] * 3 + 0 ] * InJointWeights[2]\n"
        "                    + Transform[ InJointIndices[3] * 3 + 0 ] * InJointWeights[3];\n"
        "    const vec4\n"
        "    JointTransform1 = Transform[ InJointIndices[0] * 3 + 1 ] * InJointWeights[0]\n"
        "                    + Transform[ InJointIndices[1] * 3 + 1 ] * InJointWeights[1]\n"
        "                    + Transform[ InJointIndices[2] * 3 + 1 ] * InJointWeights[2]\n"
        "                    + Transform[ InJointIndices[3] * 3 + 1 ] * InJointWeights[3];\n"
        "    const vec4\n"
        "    JointTransform2 = Transform[ InJointIndices[0] * 3 + 2 ] * InJointWeights[0]\n"
        "                    + Transform[ InJointIndices[1] * 3 + 2 ] * InJointWeights[1]\n"
        "                    + Transform[ InJointIndices[2] * 3 + 2 ] * InJointWeights[2]\n"
        "                    + Transform[ InJointIndices[3] * 3 + 2 ] * InJointWeights[3];\n"

        "    vec3 Position;\n"
        "    Position.x = dot( JointTransform0, SrcPosition );\n"
        "    Position.y = dot( JointTransform1, SrcPosition );\n"
        "    Position.z = dot( JointTransform2, SrcPosition );\n"
        "    #define GetVertexPosition() Position\n"
        "#else\n"
        "    #define GetVertexPosition() InPosition\n"
        "#endif\n";

    FString prebuildVertexShaderColorPass =
        "#ifndef UNLIT\n"
        "#ifdef SKINNED_MESH\n"
        "    vec4 Normal;\n"

             // Normal in model space
        "    Normal.x = dot( vec3(JointTransform0), InNormal );\n"
        "    Normal.y = dot( vec3(JointTransform1), InNormal );\n"
        "    Normal.z = dot( vec3(JointTransform2), InNormal );\n"

             // Transform normal from model space to viewspace
        "    VS_N.x = dot( ModelNormalToViewSpace0, Normal );\n"
        "    VS_N.y = dot( ModelNormalToViewSpace1, Normal );\n"
        "    VS_N.z = dot( ModelNormalToViewSpace2, Normal );\n"
        "    VS_N = normalize( VS_N );\n"

             // Tangent in model space
        "    Normal.x = dot( vec3(JointTransform0), InTangent.xyz );\n"
        "    Normal.y = dot( vec3(JointTransform1), InTangent.xyz );\n"
        "    Normal.z = dot( vec3(JointTransform2), InTangent.xyz );\n"

             // Transform tangent from model space to viewspace
        "    VS_T.x = dot( ModelNormalToViewSpace0, Normal );\n"
        "    VS_T.y = dot( ModelNormalToViewSpace1, Normal );\n"
        "    VS_T.z = dot( ModelNormalToViewSpace2, Normal );\n"
        "    VS_T = normalize( VS_T );\n"

             // Compute binormal in viewspace
        "    VS_B = normalize( cross( VS_N, VS_T ) ) * InTangent.w;\n"// * ( handedness - 1.0 );

        "#else\n"

             // Transform normal from model space to viewspace
        "    VS_N.x = dot( ModelNormalToViewSpace0, vec4( InNormal, 0.0 ) );\n"
        "    VS_N.y = dot( ModelNormalToViewSpace1, vec4( InNormal, 0.0 ) );\n"
        "    VS_N.z = dot( ModelNormalToViewSpace2, vec4( InNormal, 0.0 ) );\n"

             // Transform tangent from model space to viewspace
        "    VS_T.x = dot( ModelNormalToViewSpace0, InTangent );\n"
        "    VS_T.y = dot( ModelNormalToViewSpace1, InTangent );\n"
        "    VS_T.z = dot( ModelNormalToViewSpace2, InTangent );\n"

             // Compute binormal in viewspace
        "    VS_B = normalize( cross( VS_N, VS_T ) ) * InTangent.w;\n"// * ( handedness - 1.0 );

        "#endif\n"
        "#endif\n";

    GenerateBuiltinSource( VertexSrc );
    GenerateBuiltinSource( FragmentSrc );

    FragmentSrc += AtmosphereShader;

    VertexSrc +=
            "out gl_PerVertex\n"
            "{\n"
            "    vec4 gl_Position;\n"
//          "    float gl_PointSize;\n"
//          "    float gl_ClipDistance[];\n"
            "};\n"

            "#ifdef SKINNED_MESH\n"
            "layout( binding = 2, std140 ) uniform JointTransforms\n"
            "{\n"
            "    vec4 Transform[ 256 * 3 ];\n"   // MAX_JOINTS = 256
            "};\n"
            "#endif\n";

    // Create depth pass
    context.Reset( MaterialType, MATERIAL_PASS_DEPTH );
    {
        // Depth pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );
        VertexSrc += "#ifdef MATERIAL_PASS_DEPTH\n";
        VertexSrc += SamplersString( context.MaxTextureSlot );
        VertexSrc += "void main() {\n";
        VertexSrc += prebuildVertexShader;
        VertexSrc += context.SourceCode + "}\n";
        VertexSrc += "#endif\n";

        bHasTextures[ MATERIAL_PASS_DEPTH ] = context.bHasTextures;
        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        bVertexTextureFetch |= context.bHasTextures;

        // TODO: Для MATERIAL_PASS_DEPTH и MATERIAL_PASS_WIREFRAME проверить:
        // если нет никакой деформации вершины, то использовать шейдер/пайплайн по умолчанию
        // для данного прохода. Это необходимо для оптимизации количества переключения пайплайнов.
    }

    // Create color pass
    context.Reset( MaterialType, MATERIAL_PASS_COLOR );
    {
        // Color pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );

        bNoVertexDeform = VertexStage->bNoVertexDeform;

        bHasTextures[ MATERIAL_PASS_COLOR ] |= context.bHasTextures;
        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        bVertexTextureFetch |= context.bHasTextures;

        int locationIndex = VertexStage->NumNextStageVariables();

        UInt bakedLightLocation = locationIndex++;
        UInt tangentLocation = locationIndex++;
        UInt binormalLocation = locationIndex++;
        UInt normalLocation = locationIndex++;

        VertexSrc += "#ifdef MATERIAL_PASS_COLOR\n";
        VertexSrc += SamplersString( context.MaxTextureSlot );
        VertexSrc += VertexStage->NSV_OutputSection();
        VertexSrc += "#ifdef USE_LIGHTMAP\n"
                     "layout( location = " + bakedLightLocation.ToString() + " ) out vec2 VS_LightmapTexCoord;\n"
                     "#endif\n"
                     "#ifdef USE_VERTEX_LIGHT\n"
                     "layout( location = " + bakedLightLocation.ToString() + " ) out vec3 VS_VertexLight;\n"
                     "#endif\n"
                     "#ifndef UNLIT\n"
                     "layout( location = " + tangentLocation.ToString() + " ) out vec3 VS_T;\n"
                     "layout( location = " + binormalLocation.ToString() + " ) out vec3 VS_B;\n"
                     "layout( location = " + normalLocation.ToString() + " ) out vec3 VS_N;\n"
                     "#endif\n"  // UNLIT
                     "void main() {\n"
                     + prebuildVertexShader
                     + prebuildVertexShaderColorPass +
                     "#ifdef USE_LIGHTMAP\n"
                     "    VS_LightmapTexCoord = InLightmapTexCoord * LightmapOffset.zw + LightmapOffset.xy;\n"
                     "#endif\n"
                     "#ifdef USE_VERTEX_LIGHT\n"
                     "    VS_VertexLight = pow( InVertexLight.xyz, vec3(2.2) ) * (4.0*InVertexLight.w);\n"
                     "#endif\n"
                     + context.SourceCode +
                     "}\n"
                     "#endif\n";

        // Color pass. Fragment stage
        context.SetStage( FRAGMENT_STAGE );
        FragmentStage->ResetConnections( context );
        FragmentStage->TouchConnections( context );
        FragmentStage->Build( context );

        bHasTextures[ MATERIAL_PASS_COLOR ] |= context.bHasTextures;
        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        lightmapSlot = context.MaxTextureSlot + 1;

        FragmentSrc += "#ifdef MATERIAL_PASS_COLOR\n"
                       "layout( location = 0 ) out vec4 FS_FragColor;\n"
                       + SamplersString( context.MaxTextureSlot )
                       + VertexStage->NSV_InputSection() +
                       "#ifdef USE_LIGHTMAP\n"
                       "layout( binding = " + Int(lightmapSlot).ToString() + " ) uniform sampler2D tslot_lightmap;\n"
                       "layout( location = " + bakedLightLocation.ToString() + " ) in vec2 VS_LightmapTexCoord;\n"
                       "#endif\n"
                       "#ifdef USE_VERTEX_LIGHT\n"
                       "layout( location = " + bakedLightLocation.ToString() + " ) in vec3 VS_VertexLight;\n"
                       "#endif\n"
                       "#ifndef UNLIT\n"
                       "layout( location = " + tangentLocation.ToString() + " ) in vec3 VS_T;\n"
                       "layout( location = " + binormalLocation.ToString() + " ) in vec3 VS_B;\n"
                       "layout( location = " + normalLocation.ToString() + " ) in vec3 VS_N;\n"
                       "#endif\n"  // UNLIT
                       "void main() {\n"
                       + context.SourceCode +
                       "#ifdef USE_LIGHTMAP\n"
                       //"FS_FragColor = FS_FragColor * pow( texture( tslot_lightmap, VS_LightmapTexCoord ).r, 2.2 );\n"
                       "FS_FragColor = FS_FragColor * vec4(texture( tslot_lightmap, VS_LightmapTexCoord ).rgb,1.0);\n"
                       "#endif\n"
                       "#ifdef USE_VERTEX_LIGHT\n"
                       "FS_FragColor = FS_FragColor * vec4(VS_VertexLight,1.0);\n"
                       "#endif\n"

// input lag test
//"float t=FS_FragColor.r;"
//"for (int i=0;i<30000;i++) t+=sin(t)+cos(t);\n"
//"FS_FragColor.a *= t;\n"

                       // Normal debugging
                       //"#ifndef UNLIT\n"
                       //"FS_FragColor = vec4(VS_N,1.0);\n"
                       //"#endif\n"

                       // Manual SRGB conversion
                       //"FS_FragColor = pow( FS_FragColor, vec4( vec3( 1.0/2.2 ), 1 ) );\n"

                       "}\n"
                       "#endif\n";
    }

    // Create wireframe pass
    context.Reset( MaterialType, MATERIAL_PASS_WIREFRAME );
    {
        // Wireframe pass. Vertex stage
        context.SetStage( VERTEX_STAGE );
        VertexStage->ResetConnections( context );
        VertexStage->TouchConnections( context );
        VertexStage->Build( context );

        bHasTextures[ MATERIAL_PASS_WIREFRAME ] = context.bHasTextures;
        maxTextureSlot = FMath::Max( maxTextureSlot, context.MaxTextureSlot );
        maxUniformAddress = FMath::Max( maxUniformAddress, context.MaxUniformAddress );

        bVertexTextureFetch |= context.bHasTextures;

        VertexSrc += "#ifdef MATERIAL_PASS_WIREFRAME\n";
        VertexSrc += SamplersString( context.MaxTextureSlot );
        VertexSrc += "void main() {\n";
        VertexSrc += prebuildVertexShader;
        VertexSrc += context.SourceCode;
        VertexSrc += "}\n"
                     "#endif\n";

        // Wireframe pass. Geometry stage
        GeometrySrc += "#ifdef MATERIAL_PASS_WIREFRAME\n"
                       "in gl_PerVertex {\n"
                       "    vec4 gl_Position;\n"
//                       "    float gl_PointSize;\n"
//                       "    float gl_ClipDistance[];\n"
                       "} gl_in[];\n"
                       "out gl_PerVertex {\n"
                       "  vec4 gl_Position;\n"
//                       "  float gl_PointSize;\n"
//                       "  float gl_ClipDistance[];\n"
                       "};\n"
                       "layout(triangles) in;\n"
                       "layout(triangle_strip, max_vertices = 3) out;\n"
                       "layout( location = 0 ) out vec3 GS_Barycentric;\n"
                       "void main() {\n"
                       "  gl_Position = gl_in[ 0 ].gl_Position;\n"
                       "  GS_Barycentric = vec3( 1, 0, 0 );\n"
                       "  EmitVertex();\n"
                       "  gl_Position = gl_in[ 1 ].gl_Position;\n"
                       "  GS_Barycentric = vec3( 0, 1, 0 );\n"
                       "  EmitVertex();\n"
                       "  gl_Position = gl_in[ 2 ].gl_Position;\n"
                       "  GS_Barycentric = vec3( 0, 0, 1 );\n"
                       "  EmitVertex();\n"
                       "  EndPrimitive();\n"
                       "}\n"
                       "#endif\n";

        // Wireframe pass. Fragment stage
        FragmentSrc += "#ifdef MATERIAL_PASS_WIREFRAME\n"
                       "layout( location = 0 ) out vec4 FS_FragColor;\n"
                       "layout( location = 0 ) in vec3 GS_Barycentric;\n"
                       "void main() {\n"
                       "  const vec4 Color = vec4(1,1,1,0.5);\n"
                       "  const float LineWidth = 1.5;\n"
                       "  vec3 SmoothStep = smoothstep( vec3( 0.0 ), fwidth( GS_Barycentric ) * LineWidth, GS_Barycentric );\n"
                       "  FS_FragColor = Color;\n"
                       "  FS_FragColor.a *= 1.0 - min( min( SmoothStep.x, SmoothStep.y ), SmoothStep.z );\n"
                       "}\n"
                       "#endif\n";
    }

    GLogger.Print( "=== vertex ===\n" );
    GLogger.Print( VertexSrc.ToConstChar() );
    GLogger.Print( "==============\n" );
    //GLogger.Print( "=== fragment ===\n" );
    //GLogger.Print( FragmentSrc.ToConstChar() );
    //GLogger.Print( "==============\n" );

    int vertexSourceLength = VertexSrc.Length()+1;
    int fragmentSourceLength = FragmentSrc.Length()+1;
    int geometrySourceLength = GeometrySrc.Length()+1;

    int size = sizeof( FMaterialBuildData )
            - sizeof( FMaterialBuildData::ShaderData )
            + vertexSourceLength
            + fragmentSourceLength
            + geometrySourceLength;

    FMaterialBuildData * buildData = ( FMaterialBuildData * )GMainMemoryZone.AllocCleared( size, 1 );

    buildData->Size = size;
    buildData->Type = MaterialType;
    buildData->Facing = MaterialFacing;
    buildData->LightmapSlot = lightmapSlot;
    buildData->bVertexTextureFetch = bVertexTextureFetch;
    buildData->bNoVertexDeform = bNoVertexDeform;
    //AN_Assert(bNoVertexDeform);

    buildData->NumUniformVectors = maxUniformAddress >= 0 ? maxUniformAddress / 4 + 1 : 0;

    buildData->NumSamplers = maxTextureSlot + 1;

    for ( int i = 0 ; i < buildData->NumSamplers ; i++ ) {
        FSamplerDesc & desc = buildData->Samplers[i];
        FMaterialTextureSlotBlock * textureSlot = TextureSlots[i];

        desc.TextureType = textureSlot->TextureType;
        desc.Filter = textureSlot->Filter;
        desc.AddressU = textureSlot->AddressU;
        desc.AddressV = textureSlot->AddressV;
        desc.AddressW = textureSlot->AddressW;
        desc.MipLODBias = textureSlot->MipLODBias;
        desc.Anisotropy = textureSlot->Anisotropy;
        desc.MinLod = textureSlot->MinLod;
        desc.MaxLod = textureSlot->MaxLod;
    }

    int offset = 0;

    buildData->VertexSourceOffset = offset;
    buildData->VertexSourceLength = vertexSourceLength;

    offset += vertexSourceLength;

    buildData->FragmentSourceOffset = offset;
    buildData->FragmentSourceLength = fragmentSourceLength;

    offset += fragmentSourceLength;

    buildData->GeometrySourceOffset = offset;
    buildData->GeometrySourceLength = geometrySourceLength;

    memcpy( &buildData->ShaderData[buildData->VertexSourceOffset], VertexSrc.ToConstChar(), vertexSourceLength );
    memcpy( &buildData->ShaderData[buildData->FragmentSourceOffset], FragmentSrc.ToConstChar(), fragmentSourceLength );
    memcpy( &buildData->ShaderData[buildData->GeometrySourceOffset], GeometrySrc.ToConstChar(), geometrySourceLength );

    FMaterial * material = NewObject< FMaterial >();
    material->Initialize( buildData );

    GMainMemoryZone.Dealloc( buildData );

    return material;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

AN_CLASS_META_NO_ATTRIBS( FMaterialProject )

FMaterialProject::FMaterialProject() {

}

FMaterialProject::~FMaterialProject() {
    for ( FAssemblyBlock * block : Blocks ) {
        block->RemoveRef();
    }
}

int FMaterialProject::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    if ( !Blocks.IsEmpty() ) {
        int array = _Doc.AddArray( object, "Blocks" );

        for ( FAssemblyBlock * block : Blocks ) {
            int blockObject = block->Serialize( _Doc );
            _Doc.AddValueToField( array, blockObject );
        }
    }

    return object;
}
