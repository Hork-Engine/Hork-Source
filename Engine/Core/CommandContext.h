/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Engine/Core/CommandProcessor.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Core/Delegate.h>
#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

class CommandContext : public ICommandContext
{
    HK_FORBID_COPY(CommandContext)

public:
    CommandContext();
    ~CommandContext();

    void AddCommand(GlobalStringView _Name, Delegate<void(CommandProcessor const&)> const& _Callback, GlobalStringView _Comment = ""s);

    void RemoveCommand(StringView _Name);

    void RemoveCommands();

    int CompleteString(StringView Str, String& _Result);

    void Print(StringView Str);

protected:
    //
    // ICommandContext implementation
    //

    void ExecuteCommand(CommandProcessor const& _Proc) override;

private:
    class RuntimeCommand
    {
    public:
        RuntimeCommand(GlobalStringView _Name, Delegate<void(CommandProcessor const&)> const& _Callback, GlobalStringView _Comment) :
            Name(_Name.CStr()), Comment(_Comment.CStr()), Callback(_Callback)
        {}

        void Override(Delegate<void(CommandProcessor const&)> const& _Callback, GlobalStringView _Comment)
        {
            Comment  = _Comment.CStr();
            Callback = _Callback;
        }

        const char* GetName() const { return Name; }

        const char* GetComment() const { return Comment; }

        void Execute(CommandProcessor const& _Proc) { Callback.Invoke(_Proc); }

    private:
        const char*                             Name;
        const char*                             Comment;
        Delegate<void(CommandProcessor const&)> Callback;
    };

    TVector<RuntimeCommand> Commands;
};

HK_NAMESPACE_END
