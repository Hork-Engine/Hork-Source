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

#include <Engine/Runtime/Public/RuntimeVariable.h>
#include <Engine/Runtime/Public/RuntimeCommandProcessor.h>
#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/CoreMath.h>

static FRuntimeVariable * GlobalVars = nullptr;
static bool GVariableAllocated = false;

FRuntimeVariable * FRuntimeVariable::GlobalVariableList() {
    return GlobalVars;
}

FRuntimeVariable * FRuntimeVariable::FindVariable( const char * _Name ) {
    for ( FRuntimeVariable * var = GlobalVars ; var ; var = var->GetNext() ) {
        if ( !FString::Icmp( var->GetName(), _Name ) ) {
            return var;
        }
    }
    return nullptr;
}

void FRuntimeVariable::AllocateVariables() {
    for ( FRuntimeVariable * var = GlobalVars ; var ; var = var->Next ) {
        var->Value = var->DefaultValue;
        var->I32 = Int().FromString( var->Value );
        var->F32 = Float().FromString( var->Value );
    }
    GVariableAllocated = true;
}

void FRuntimeVariable::FreeVariables() {
    for ( FRuntimeVariable * var = GlobalVars ; var ; var = var->Next ) {
        var->Value.Free();
        var->LatchedValue.Free();
    }
    GlobalVars = nullptr;
}

FRuntimeVariable::FRuntimeVariable( const char * _Name,
                                    const char * _Value,
                                    uint16_t _Flags,
                                    const char * _Comment )
    : Name( _Name ? _Name : "" )
    , DefaultValue( _Value ? _Value : "" )
    , Comment( _Comment ? _Comment : "" )
    , Flags( _Flags )
{
    AN_Assert( !GVariableAllocated );
    AN_Assert( FRuntimeCommandProcessor::IsValidCommandName( Name ) );

    FRuntimeVariable * head = GlobalVars;
    Next = head;
    GlobalVars = this;
}

FRuntimeVariable::~FRuntimeVariable() {
    FRuntimeVariable * prev = nullptr;
    for ( FRuntimeVariable * var = GlobalVars ; var ; var = var->Next ) {
        if ( var == this ) {
            if ( prev ) {
                prev->Next = var->Next;
            } else {
                GlobalVars = var->Next;
            }
            break;
        }
        prev = var;
    }
}

bool FRuntimeVariable::CanChangeValue() const {
    if ( Flags & VAR_READONLY ) {
        GLogger.Printf( "%s is readonly\n", Name );
        return false;
    }

    if ( ( Flags & VAR_CHEAT ) && !GRuntime.bCheatsAllowed ) {
        GLogger.Printf( "%s is cheat protected\n", Name );
        return false;
    }

    if ( ( Flags & VAR_SERVERONLY ) && !GRuntime.bServerActive ) {
        GLogger.Printf( "%s can be changed by server only\n", Name );
        return false;
    }

    if ( ( Flags & VAR_NOINGAME ) && GRuntime.bInGameStatus ) {
        GLogger.Printf( "%s can't be changed in game\n", Name );
        return false;
    }

    return true;
}

void FRuntimeVariable::SetString( const char * _String ) {
    if ( !CanChangeValue() ) {
        return;
    }

    bool bApply = Value.Cmp( _String ) != 0;
    if ( !bApply ) {
        // Value is not changed
        return;
    }

    if ( Flags & VAR_LATCHED ) {
        GLogger.Printf( "%s restart required to change value\n", Name );

        LatchedValue = _String;
    } else {
        ForceString( _String );
    }
}

void FRuntimeVariable::SetString( FString const & _String ) {
    SetString( _String.ToConstChar() );
}

void FRuntimeVariable::SetBool( bool _Bool ) {
    SetString( _Bool ? "1" : "0" );
}

void FRuntimeVariable::SetInteger( int32_t _Integer ) {
    SetString( Int(_Integer).ToString() );
}

void FRuntimeVariable::SetFloat( float _Float ) {
    SetString( Float(_Float).ToString() );
}

void FRuntimeVariable::ForceString( const char * _String ) {
    Value = _String;
    I32 = Int().FromString( Value );
    F32 = Float().FromString( Value );
    LatchedValue.Clear();
    MarkModified();
}

void FRuntimeVariable::ForceString( FString const & _String ) {
    ForceString( _String.ToConstChar() );
}

void FRuntimeVariable::ForceBool( bool _Bool ) {
    ForceString( _Bool ? "1" : "0" );
}

void FRuntimeVariable::ForceInteger( int32_t _Integer ) {
    ForceString( Int(_Integer).ToString() );
}

void FRuntimeVariable::ForceFloat( float _Float ) {
    ForceString( Float(_Float).ToString() );
}

void FRuntimeVariable::SetLatched() {
    if ( !(Flags & VAR_LATCHED) ) {
        return;
    }

    if ( LatchedValue.IsEmpty() ) {
        return;
    }

    if ( !CanChangeValue() ) {
        return;
    }

    ForceString( LatchedValue );
}

void FRuntimeVariable::Print() {
    GLogger.Printf( "    %s", Name );

    if ( *Comment ) {
        GLogger.Printf( " (%s)", Comment );
    }

    GLogger.Printf( "\n        [CURRENT \"%s\"]  [DEFAULT \"%s\"]", Value.ToConstChar(), DefaultValue );

    if ( ( Flags & VAR_LATCHED ) && !LatchedValue.IsEmpty() ) {
        GLogger.Printf( "  [LATCHED \"%s\"]\n", LatchedValue.ToConstChar() );
    } else {
        GLogger.Print( "\n" );
    }

    if ( Flags & (VAR_LATCHED|VAR_READONLY|VAR_NOSAVE|VAR_CHEAT|VAR_SERVERONLY|VAR_NOINGAME) ) {
        GLogger.Print( "        [FLAGS");
        if ( Flags & VAR_LATCHED ) { GLogger.Print( " LATCHED" ); }
        if ( Flags & VAR_READONLY ) { GLogger.Print( " READONLY" ); }
        if ( Flags & VAR_NOSAVE ) { GLogger.Print( " NOSAVE" ); }
        if ( Flags & VAR_CHEAT ) { GLogger.Print( " CHEAT" ); }
        if ( Flags & VAR_SERVERONLY ) { GLogger.Print( " SERVERONLY" ); }
        if ( Flags & VAR_NOINGAME ) { GLogger.Print( " NOINGAME" ); }
        GLogger.Print( "]\n" );
    }
}
