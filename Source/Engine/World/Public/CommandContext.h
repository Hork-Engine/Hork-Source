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

#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <World/Public/Base/BaseObject.h>

class ACommandContext : public IRuntimeCommandContext {
    AN_FORBID_COPY( ACommandContext )

public:
    ACommandContext();
    ~ACommandContext();

    void AddCommand( const char * _Name, TCallback< void( ARuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment = "" );

    void RemoveCommand( const char * _Name );

    void RemoveCommands();

    int CompleteString( const char * _Str, int _StrLen, AString & _Result );

    void Print( const char * _Str, int _StrLen );

protected:
    //
    // IRuntimeCommandContext implementation
    //

    void ExecuteCommand( ARuntimeCommandProcessor const & _Proc ) override;

private:
    class ARuntimeCommand {
    public:
        ARuntimeCommand( const char * _Name, TCallback< void( ARuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment )
            : Name( _Name )
            , Comment( _Comment )
            , Callback( _Callback )
        {
        }

        void Override( TCallback< void( ARuntimeCommandProcessor const & ) > const & _Callback, const char * _Comment ) {
            Comment = _Comment;
            Callback = _Callback;
        }

        AString const & GetName() const { return Name; }

        AString const & GetComment() const { return Comment; }

        void Execute( ARuntimeCommandProcessor const & _Proc ) { Callback( _Proc ); }

    private:
        AString Name;
        AString Comment;
        TCallback< void( ARuntimeCommandProcessor const & ) > Callback;
    };

    TStdVector< ARuntimeCommand > Commands;
};
