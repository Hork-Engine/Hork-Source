/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <Runtime/Public/Runtime.h>
#include <Core/Public/Logger.h>
#include <Core/Public/CoreMath.h>

static ARuntimeVariable * GlobalVars = nullptr;
static bool GVariableAllocated = false;

ARuntimeVariable * ARuntimeVariable::GlobalVariableList() {
    return GlobalVars;
}

ARuntimeVariable * ARuntimeVariable::FindVariable( const char * _Name ) {
    for ( ARuntimeVariable * var = GlobalVars ; var ; var = var->GetNext() ) {
        if ( !Core::Stricmp( var->GetName(), _Name ) ) {
            return var;
        }
    }
    return nullptr;
}

void ARuntimeVariable::AllocateVariables() {
    for ( ARuntimeVariable * var = GlobalVars ; var ; var = var->Next ) {
        var->Value = var->DefaultValue;
        var->I32 = Math::ToInt< int32_t >( var->Value );
        var->F32 = Math::ToFloat( var->Value );
    }
    GVariableAllocated = true;
}

void ARuntimeVariable::FreeVariables() {
    for ( ARuntimeVariable * var = GlobalVars ; var ; var = var->Next ) {
        var->Value.Free();
        var->LatchedValue.Free();
    }
    GlobalVars = nullptr;
}

ARuntimeVariable::ARuntimeVariable( const char * _Name,
                                    const char * _Value,
                                    uint16_t _Flags,
                                    const char * _Comment )
    : Name( _Name ? _Name : "" )
    , DefaultValue( _Value ? _Value : "" )
    , Comment( _Comment ? _Comment : "" )
    , Flags( _Flags )
{
    AN_ASSERT( !GVariableAllocated );
    AN_ASSERT( ARuntimeCommandProcessor::IsValidCommandName( Name ) );

    ARuntimeVariable * head = GlobalVars;
    Next = head;
    GlobalVars = this;
}

ARuntimeVariable::~ARuntimeVariable() {
    ARuntimeVariable * prev = nullptr;
    for ( ARuntimeVariable * var = GlobalVars ; var ; var = var->Next ) {
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

bool ARuntimeVariable::CanChangeValue() const {
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

void ARuntimeVariable::SetString( const char * _String ) {
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

void ARuntimeVariable::SetString( AString const & _String ) {
    SetString( _String.CStr() );
}

void ARuntimeVariable::SetBool( bool _Bool ) {
    SetString( _Bool ? "1" : "0" );
}

void ARuntimeVariable::SetInteger( int32_t _Integer ) {
    SetString( Math::ToString( _Integer ) );
}

void ARuntimeVariable::SetFloat( float _Float ) {
    SetString( Math::ToString( _Float ) );
}

void ARuntimeVariable::ForceString( const char * _String ) {
    Value = _String;
    I32 = Math::ToInt< int32_t >( Value );
    F32 = Math::ToFloat( Value );
    LatchedValue.Clear();
    MarkModified();
}

void ARuntimeVariable::ForceString( AString const & _String ) {
    ForceString( _String.CStr() );
}

void ARuntimeVariable::ForceBool( bool _Bool ) {
    ForceString( _Bool ? "1" : "0" );
}

void ARuntimeVariable::ForceInteger( int32_t _Integer ) {
    ForceString( Math::ToString( _Integer ) );
}

void ARuntimeVariable::ForceFloat( float _Float ) {
    ForceString( Math::ToString( _Float ) );
}

void ARuntimeVariable::SetLatched() {
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

void ARuntimeVariable::Print() {
    GLogger.Printf( "    %s", Name );

    if ( *Comment ) {
        GLogger.Printf( " (%s)", Comment );
    }

    GLogger.Printf( "\n        [CURRENT \"%s\"]  [DEFAULT \"%s\"]", Value.CStr(), DefaultValue );

    if ( ( Flags & VAR_LATCHED ) && !LatchedValue.IsEmpty() ) {
        GLogger.Printf( "  [LATCHED \"%s\"]\n", LatchedValue.CStr() );
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
