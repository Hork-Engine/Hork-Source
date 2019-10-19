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


#include <Engine/World/Public/CommandContext.h>
#include <Engine/Core/Public/Logger.h>

FCommandContext::FCommandContext() {

}

FCommandContext::~FCommandContext() {

}

void FCommandContext::ExecuteCommand( FRuntimeCommandProcessor const & _Proc ) {
    AN_Assert( _Proc.GetArgsCount() > 0 );

    const char * name = _Proc.GetArg( 0 );

    for ( FRuntimeCommand & cmd : Commands ) {
        if ( !cmd.GetName().Icmp( name ) ) {
            cmd.Execute( _Proc );
            return;
        }
    }

    FRuntimeVariable * var;
    if ( nullptr != (var = FRuntimeVariable::FindVariable( name )) ) {
        if ( _Proc.GetArgsCount() < 2 ) {
            var->Print();
        } else {
            var->SetString( _Proc.GetArg( 1 ) );
        }
        return;
    }

    GLogger.Printf( "Unknown command \"%s\"\n", name );
}

void FCommandContext::AddCommand( const char * _Name, TCallback< void( FRuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment ) {
    if ( !FRuntimeCommandProcessor::IsValidCommandName( _Name ) ) {
        GLogger.Printf( "FCommandContext::AddCommand: invalid command name\n" );
        return;
    }

    if ( FRuntimeVariable::FindVariable( _Name ) ) {
        GLogger.Printf( "Name conflict: %s already registered as variable\n", _Name );
        return;
    }

    for ( FRuntimeCommand & cmd : Commands ) {
        if ( !cmd.GetName().Icmp( _Name ) ) {
            GLogger.Printf( "Overriding %s command\n", _Name );

            cmd.Override( _Callback, _Comment );
            return;
        }
    }

    // Register new command
    Commands.emplace_back( _Name, _Callback, _Comment );
}

void FCommandContext::RemoveCommand( const char * _Name ) {
    for ( TStdVector< FRuntimeCommand >::iterator it = Commands.begin() ; it != Commands.end() ; it++ ) {
        FRuntimeCommand & command = *it;

        if ( !command.GetName().Icmp( _Name ) ) {
            Commands.erase( it );
            return;
        }
    }
}

void FCommandContext::RemoveCommands() {
    Commands.clear();
}

namespace  {

AN_FORCEINLINE bool CompareChar( char _Ch1, char _Ch2 ) {
    return tolower( _Ch1 ) == tolower( _Ch2 );
}

}

int FCommandContext::CompleteString( const char * _Str, int _StrLen, FString & _Result ) {
    int count = 0;

    _Result.Clear();

    // skip whitespaces
    while ( ( ( *_Str < 32 && *_Str > 0 ) || *_Str == ' ' || *_Str == '\t' ) && _StrLen > 0 ) {
        _Str++;
        _StrLen--;
    }

    if ( _StrLen <= 0 || !*_Str )  {
        return 0;
    }

    for ( FRuntimeCommand const & cmd : Commands ) {
        if ( !cmd.GetName().IcmpN( _Str, _StrLen ) ) {
            int n;
            int minLen = FMath::Min( cmd.GetName().Length(), _Result.Length() );
            for ( n = 0 ; n < minLen && CompareChar( cmd.GetName()[n], _Result[n] ) ; n++ ) {}
            if ( n == 0 ) {
                _Result = cmd.GetName();
            } else {
                _Result.Resize( n );
            }
            count++;
        }
    }

    for ( FRuntimeVariable * var = FRuntimeVariable::GlobalVariableList() ; var ; var = var->GetNext() ) {
        if ( !FString::IcmpN( var->GetName(), _Str, _StrLen ) ) {
            int n;
            int minLen = FMath::Min( FString::Length( var->GetName() ), _Result.Length() );
            for ( n = 0 ; n < minLen && CompareChar( var->GetName()[n], _Result[n] ) ; n++ ) {}
            if ( n == 0 ) {
                _Result = var->GetName();
            } else {
                _Result.Resize( n );
            }
            count++;
        }
    }

    return count;
}

void FCommandContext::Print( const char * _Str, int _StrLen ) {
    TPodArray< FRuntimeCommand * > cmds;
    TPodArray< FRuntimeVariable * > vars;

    if ( _StrLen > 0 ) {
        for ( FRuntimeCommand & cmd : Commands ) {
            if ( !cmd.GetName().IcmpN( _Str, _StrLen ) ) {
                cmds.Append( &cmd );
            }
        }

        struct {
            bool operator() ( FRuntimeCommand const * _A, FRuntimeCommand * _B ) {
                return _A->GetName().Icmp( _B->GetName() ) < 0;
            }
        } CmdSortFunction;

        StdSort( cmds.Begin(), cmds.End(), CmdSortFunction );

        for ( FRuntimeVariable * var = FRuntimeVariable::GlobalVariableList() ; var ; var = var->GetNext() ) {
            if ( !FString::IcmpN( var->GetName(), _Str, _StrLen ) ) {
                vars.Append( var );
            }
        }

        struct {
            bool operator() ( FRuntimeVariable const * _A, FRuntimeVariable * _B ) {
                return FString::Icmp( _A->GetName(), _B->GetName() ) < 0;
            }
        } VarSortFunction;

        StdSort( vars.Begin(), vars.End(), VarSortFunction );

        GLogger.Printf( "Total commands found: %d\n"
                        "Total variables found: %d\n", cmds.Size(), vars.Size() );
        for ( FRuntimeCommand * cmd : cmds ) {
            if ( !cmd->GetComment().IsEmpty() ) {
                GLogger.Printf( "    %s (%s)\n", cmd->GetName().ToConstChar(), cmd->GetComment().ToConstChar() );
            } else{
                GLogger.Printf( "    %s\n", cmd->GetName().ToConstChar() );
            }
        }
        for ( FRuntimeVariable * var : vars ) {
            if ( *var->GetComment() ) {
                GLogger.Printf( "    %s \"%s\" (%s)\n", var->GetName(), var->GetValue().ToConstChar(), var->GetComment() );
            } else {
                GLogger.Printf( "    %s \"%s\"\n", var->GetName(), var->GetValue().ToConstChar() );
            }
        }
    }
}
