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

#pragma once

#include <Engine/Runtime/Public/RuntimeCommandProcessor.h>
#include <Engine/Runtime/Public/RuntimeVariable.h>
#include <Engine/Base/Public/BaseObject.h>

class FCommandContext : public IRuntimeCommandContext {
    AN_FORBID_COPY( FCommandContext )

public:
    FCommandContext();
    ~FCommandContext();

    void AddCommand( const char * _Name, TCallback< void( FRuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment = "" );

    void RemoveCommand( const char * _Name );

    void RemoveCommands();

    int CompleteString( const char * _Str, int _StrLen, FString & _Result );

    void Print( const char * _Str, int _StrLen );

protected:
    //
    // IRuntimeCommandContext implementation
    //

    void ExecuteCommand( FRuntimeCommandProcessor const & _Proc ) override;

private:
    class FRuntimeCommand {
    public:
        FRuntimeCommand( const char * _Name, TCallback< void( FRuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment )
            : Name( _Name )
            , Comment( _Comment )
            , Callback( _Callback )
        {
        }

        void Override( TCallback< void( FRuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment ) {
            Comment = _Comment;
            Callback = _Callback;
        }

        FString const & GetName() const { return Name; }

        FString const & GetComment() const { return Comment; }

        void Execute( FRuntimeCommandProcessor const & _Proc ) { Callback( _Proc ); }

    private:
        FString Name;
        FString Comment;
        TCallback< void( FRuntimeCommandProcessor const & ) > Callback;
    };

    TStdVector< FRuntimeCommand > Commands;
};
