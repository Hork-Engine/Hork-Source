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

#include <Core/CommandProcessor.h>
#include <Core/ConsoleVar.h>

class ACommandContext : public ICommandContext
{
    HK_FORBID_COPY(ACommandContext)

public:
    ACommandContext();
    ~ACommandContext();

    void AddCommand(AGlobalStringView _Name, TCallback<void(ACommandProcessor const&)> const& _Callback, AGlobalStringView _Comment = ""s);

    void RemoveCommand(AStringView _Name);

    void RemoveCommands();

    int CompleteString(AStringView Str, AString& _Result);

    void Print(AStringView Str);

protected:
    //
    // ICommandContext implementation
    //

    void ExecuteCommand(ACommandProcessor const& _Proc) override;

private:
    class ARuntimeCommand
    {
    public:
        ARuntimeCommand(AGlobalStringView _Name, TCallback<void(ACommandProcessor const&)> const& _Callback, AGlobalStringView _Comment) :
            Name(_Name.CStr()), Comment(_Comment.CStr()), Callback(_Callback)
        {}

        void Override(TCallback<void(ACommandProcessor const&)> const& _Callback, AGlobalStringView _Comment)
        {
            Comment  = _Comment.CStr();
            Callback = _Callback;
        }

        const char* GetName() const { return Name; }

        const char* GetComment() const { return Comment; }

        void Execute(ACommandProcessor const& _Proc) { Callback(_Proc); }

    private:
        const char*                               Name;
        const char*                               Comment;
        TCallback<void(ACommandProcessor const&)> Callback;
    };

    TVector<ARuntimeCommand> Commands;
};
